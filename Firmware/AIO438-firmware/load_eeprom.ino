void load_eeprom() {
  eeprom_get(factory_data);
  factory_data = eeprom_buffer.as_factory_data;

  if (factory_data.signature != 0x5555) { //Memory is unsigned (unknown state)
    Serial.println("Memory not signed");
    return;
  }

  if (factory_data.hw_version.major == 0xFF) {
    Serial.println("Hardware version not set");
    return;
  } else if (factory_data.hw_version.major > hw_support_major) {
    Serial.println("Hardware version not supported"); //todo: make more verbose
    return;
  }

  if (factory_data.bt_fw_version.major == 0xFF) {
    Serial.println("Bluetooth firmware version not set");
    return;
  } else if (factory_data.bt_fw_version.major > bt_fw_support_major) {
    Serial.println("Bluetooth firmware version not supported"); //todo: make more verbose
    return;
  }

  for (int i = 0; i < 32; i++) { //Todo: verbosity
    eeprom_get(table.entry[0]); //todo offset functietje bouwen
    entry_struct entry = eeprom_buffer.as_entry;

    Serial.print("Loading entry ");
    Serial.println(i);

    if (calculate_crc != 0) {
      Serial.println("Invalid CRC");
      return;
    }

    if (entry.type == entry_type_enum::system_variables) {
      read_eeprom(entry.location, entry.size);
      user = eeprom_buffer.as_user_data; //rename
    } else if (entry.type == entry_type_enum::dsp_default) {
      load_dsp(entry.location, entry.size, entry.amp);
    }
  }
}

void load_dsp(int start_reg, int size, byte amp) {
  int end_reg = start_reg + size;
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
          if (amp == 0) { //Write to all amps
            burst_write(amp_1, burst_amount);
            burst_write(amp_2, burst_amount);
          } else if (amp == 1) { //Write to amp_1
            burst_write(amp_1, burst_amount);
          } else if (amp == 2) { //Write to amp_2
            burst_write(amp_2, burst_amount);
          }
          i += burst_amount; //Move past register and data
          if (i % 2 != 0) {
            i++; //todo simplify, improve
          }
          break;
        }
      default:
        if (amp == 0) { //Write to all amps
          write_register_dual(r.command, r.param);
        } else if (amp == 1) { //Write to amp_1
          write_register(amp_1, r.command, r.param);
        } else if (amp == 2) { //Write to amp_2
          write_register(amp_2, r.command, r.param);
        }
        break;
    }
  }
}

uint16_t calculate_crc(int start_reg, int size) {
  uint8_t end_reg = start_reg + size;
  crc.restart();
  for (int i = start_reg; i < size; i++) {
    read_eeprom(i, page_size); //Read one page
    crc.add(eeprom_buffer.as_bytes, min(page_size, end_reg - i)); //Add one page at a time
  }
  return crc.getCRC();
}

//Legacy
//void clear_eeprom() { //Clear eeprom to factory default (all cells 255 (0xFF))
//  uint8_t default_value[page_size];
//  memset(default_value, 0xFF, page_size);
//  for (int i = 0; i < eeprom_size; i += page_size) {
//    write_eeprom(i, page_size, default_value);
//  }
//  Serial.println("Memory cleared");
//}
