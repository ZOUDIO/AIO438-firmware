bool load_eeprom() {
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

  return load_eeprom_user();
}

bool load_eeprom_user() {
  const uint16_t allocation_table_offset = offsetof(eeprom_layout, table);
  const uint8_t entry_size = sizeof(entry_struct);

  Serial.println("Entry\tType\tAmp\tAddr\tSize\tCRC\tCalculated CRC"); //Print header

  for (int i = 0; i < 2; i++) { //todo: verbosity, hardcode 32?
    uint16_t entry_offset = allocation_table_offset + entry_size * i;
    read_eeprom(entry_offset, entry_size);
    entry_struct entry = eeprom_buffer.as_entry;

    if (entry.type == 0xFF) { //Empty entry
      continue; //Jump to next for-loop iteration
    }

    uint16_t address_int = get_int(entry.address_msb, entry.address_lsb);
    uint16_t size_int = get_int(entry.size_msb, entry.size_lsb);
    uint16_t crc_int = get_int(entry.crc_msb, entry.crc_lsb);
    uint16_t crc_calculated = calculate_crc(address_int, size_int);

    Serial.print(i);
    Serial.print('\t');
    Serial.print(entry.type);
    Serial.print('\t');
    Serial.print(entry.amp);
    Serial.print('\t');
    Serial.print(address_int);
    Serial.print('\t');
    Serial.print(size_int);
    Serial.print('\t');
    Serial.print("0x");
    Serial.print(crc_int, HEX);
    Serial.print('\t');
    Serial.print("0x");
    Serial.println(crc_calculated, HEX);

    if (crc_int != crc_calculated) {
      Serial.println(F("Invalid CRC"));
      //return false;
    }

    continue;

    if (entry.type == static_cast<uint8_t>(entry_type_enum::system_variables)) {
      read_eeprom(address_int, size_int);
      user = eeprom_buffer.as_user_data;
      user.vol_start = swap_float(user.vol_start);
      user.vol_max = swap_float(user.vol_max);
      user.power_low = swap_float(user.power_low);
      user.power_shutdown = swap_float(user.power_shutdown);
    } else if (entry.type == static_cast<uint8_t>(entry_type_enum::dsp_default)) {
      //load_dsp(address_int, size_int, entry.amp);
    } //todo, load eq's (how?)
  }

  return true;
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
  uint32_t end_reg = start_reg + amount;
  crc.reset();
  crc.setStartXOR(0xFFFF); //Matches CCITT guidelines
  for (uint32_t i = start_reg; i < end_reg; i += page_size) { //todo optimize
    read_eeprom(i, page_size); //Read one page
    int add_amount = min(page_size, end_reg - i); //Add a maximum of one page at a time
    crc.add(eeprom_buffer.as_bytes, add_amount);
  }
  return crc.getCRC();
}
