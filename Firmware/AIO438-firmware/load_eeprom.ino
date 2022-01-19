bool load_eeprom() {
  if (!detect_device(eeprom_ext)) {
    Serial.println(F("Eeprom not found"));
    return false;
  }

  eeprom_get(factory_data);
  factory_data = eeprom_buffer.as_factory_data;

  if (factory_data.signature != 0x5555) { //Memory is unsigned (unknown state)
    Serial.println(F("Memory not signed")); //Todo: offer to clear memory and sign or something?
    return false;
  }

  if (factory_data.hw_version.major == 0xFF) {
    Serial.println(F("Hardware version not set"));
    return false;
  } else if (factory_data.hw_version.major > 1) { //This firmware supports hardware versions 1.x.x
    Serial.println(F("Hardware version not supported"));
    return false;
  }

  if (factory_data.bt_fw_version.major == 0xFF) {
    Serial.println(F("Bluetooth firmware version not set"));
    return false;
  } else if (factory_data.bt_fw_version.major > 1) { //This firmware supports bluetooth firmware versions 1.x.x
    Serial.println(F("Bluetooth firmware version not supported"));
    return false;
  }

  const uint16_t allocation_table_offset = offsetof(eeprom_layout, table);
  const uint8_t entry_size = sizeof(entry_struct);

  for (int i = 0; i < 32; i++) { //todo: verbosity, hardcode 32?
    uint16_t entry_offset = allocation_table_offset + entry_size * i;
    read_eeprom(entry_offset, entry_size);
    entry_struct entry = eeprom_buffer.as_entry;

    uint16_t address_int = get_int(entry.address_msb, entry.address_lsb);
    uint16_t size_int = get_int(entry.size_msb, entry.size_lsb);
    uint16_t crc_int = get_int(entry.crc_msb, entry.crc_lsb);
    
    Serial.print(F("Entry "));
    Serial.print(i);
    if (entry.type == 0xFF) {
      Serial.println(F(" is empty"));
      continue; //Jump to next for-loop iteration
    } else if (entry.type == static_cast<uint8_t>(entry_type_enum::system_variables)) {
      Serial.println(F(" contains system variables"));
    } else if (entry.type == static_cast<uint8_t>(entry_type_enum::dsp_default)) {
      Serial.println(F(" contains dsp default"));
    } else if (entry.type == static_cast<uint8_t>(entry_type_enum::dsp_eq_off)) {
      Serial.println(F(" contains dsp eq on"));
    } else if (entry.type == static_cast<uint8_t>(entry_type_enum::dsp_eq_on)) {
      Serial.println(F(" contains dsp eq off"));
    } else {
      Serial.println(F(" contains unknown type"));
      return false;
    }

    Serial.print(F("Address = "));
    Serial.print(address_int);
    Serial.print(F(", size = "));
    Serial.println(size_int);

    if(size_int > 10000) {
      Serial.println("Block too large");
      return false;
    }

    uint16_t calculated_crc = calculate_crc(address_int, size_int); //todo pack into general function?
    if (calculated_crc != crc_int) {
      Serial.println(F("Invalid CRC"));
      Serial.print(F("Calculated CRC = "));
      Serial.println(calculated_crc, HEX);
      Serial.print(F("Stored CRC = "));
      Serial.println(crc_int, HEX);
      return false;
    }

    if (entry.type == static_cast<uint8_t>(entry_type_enum::system_variables)) {
      read_eeprom(address_int, size_int);
      user = eeprom_buffer.as_user_data;
    } else if (entry.type == static_cast<uint8_t>(entry_type_enum::dsp_default)) {
      load_dsp(address_int, size_int, entry.amp);
    } //todo, load eq's (how?)
  }

  return true; //Loading succesful
}

void load_dsp(int start_reg, int amount, byte amp) {
  byte burst_amount;
  int end_reg = start_reg + amount;
  for (int i = start_reg; i < end_reg; i += 2) { //Read 2 bytes (command+param) at a time
    read_eeprom(i, 2); //Get 2 bytes  (command and parameter)
    cfg_reg r = eeprom_buffer.as_cfg_reg;
    switch (r.command) {
      case cfg_meta_delay:
        delay(r.param);
        break;
      case cfg_meta_burst:
          burst_amount = r.param + 1; //Register plus data
          read_eeprom(i + 2, burst_amount); //Get bytes from eeprom
          write_multiple_registers(amp, eeprom_buffer.as_bytes, burst_amount);
          i += burst_amount; //Move past register and data
          if (i % 2 != 0) {
            i++; //todo simplify, improve
          }
          break;
      default:
        write_single_register(amp, r.command, r.param);
        break;
    }
  }

  //Go back to book 0 page 0
  write_single_register(amp_dual, 0, 0);
  write_single_register(amp_dual, 0x7F, 0);
  write_single_register(amp_dual, 0, 0);
}

uint16_t calculate_crc(int start_reg, int amount) {
  uint8_t end_reg = start_reg + amount;
  crc.restart();
  crc.setStartXOR(0xFFFF); //Matches CCITT guidelines
  for (uint16_t i = start_reg; i < end_reg; i++) {
    read_eeprom(i, page_size); //Read one page
    crc.add(eeprom_buffer.as_bytes, min(page_size, end_reg - i)); //Add a maximum of one page at a time
  }
  return crc.getCRC();
}
