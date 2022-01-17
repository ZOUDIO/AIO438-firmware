bool load_eeprom() {
  if (!detect_device(eeprom_ext)) {
    Serial.println(F("Eeprom not found"));
    return false;
  }

  reset_variables();

  eeprom_get(factory_data);
  factory_data = eeprom_buffer.as_factory_data;

  if (factory_data.signature != 0x5555) { //Memory is unsigned (unknown state)
    Serial.println(F("Memory not signed")); //Todo: offer to clear memory and sign or something?
    return false;
  }

  if (factory_data.hw_version.major == 0xFF) {
    Serial.println(F("Hardware version not set"));
    return false;
  } else if (factory_data.hw_version.major > hw_support_major) {
    Serial.println(F("Hardware version not supported"));
    return false;
  }

  if (factory_data.bt_fw_version.major == 0xFF) {
    Serial.println(F("Bluetooth firmware version not set"));
    return false;
  } else if (factory_data.bt_fw_version.major > bt_fw_support_major) {
    Serial.println(F("Bluetooth firmware version not supported"));
    return false;
  }

  uint16_t allocation_table_offset = offsetof(eeprom_layout, table);
  uint8_t entry_size = sizeof(entry_struct);

  for (int i = 0; i < 32; i++) { //todo: verbosity
    uint16_t entry_offset = allocation_table_offset + entry_size * i;
    read_eeprom(entry_offset, entry_size);
    entry_struct entry = eeprom_buffer.as_entry;

    Serial.print("Entry ");
    Serial.print(i);
    if (entry.type == static_cast<uint8_t>(entry_type_enum::empty)) {
      Serial.println(" is empty");
      continue; //Jump to next for-loop iteration
    }

    uint16_t calculated_crc = calculate_crc(entry.address, entry.size);
    if (calculated_crc != entry.crc) {
      Serial.println("Invalid CRC");
      Serial.print("Calculated CRC = ");
      Serial.println(calculated_crc, HEX);
      Serial.print("Stored CRC = ");
      Serial.println(entry.crc, HEX);
      return false;
    }

    if (entry.type == static_cast<uint8_t>(entry_type_enum::system_variables)) {
      read_eeprom(entry.address, entry.size);
      user = eeprom_buffer.as_user_data;
    } else if (entry.type == static_cast<uint8_t>(entry_type_enum::dsp_default)) {
      load_dsp(entry.address, entry.size, entry.amp);
    } //todo eq dsps
  }

  return true; //Loading succesful
}

void reset_variables() { //todo: more elegant way possible?
  user.amp_1_enabled = true;
  user.amp_2_enabled = true;
  user.vol_start_enabled = true;
  user.vol_start = -30;
  user.vol_max = 0;
  user.power_low = 0;
  user.power_shutdown = 0;

  vol = -30;        //Default: -30dB
  analog_gain = 0;  //Default: 0dB
}

void load_dsp(int start_reg, int amount, byte amp) {
  int end_reg = start_reg + amount;
  for (int i = start_reg; i < end_reg; i += 2) { //Read 2 bytes (command+param) at a time
    read_eeprom(i, 2); //Get 2 bytes  (command and parameter)
    cfg_reg r = eeprom_buffer.as_cfg_reg;
    switch (r.command) {
      case cfg_meta_delay:
        delay(r.param);
        break;
      case cfg_meta_burst: {
          byte burst_amount = r.param + 1; //Register plus data
          read_eeprom(i + 2, burst_amount); //Get bytes from eeprom
          write_multiple_registers(amp, eeprom_buffer.as_bytes, burst_amount);
          i += burst_amount; //Move past register and data
          if (i % 2 != 0) {
            i++; //todo simplify, improve
          }
          break;
        }
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
  for (uint16_t i = start_reg; i < amount; i++) {
    read_eeprom(i, page_size); //Read one page
    crc.add(eeprom_buffer.as_bytes, min(page_size, end_reg - i)); //Add one page at a time
  }
  return crc.getCRC();
}
