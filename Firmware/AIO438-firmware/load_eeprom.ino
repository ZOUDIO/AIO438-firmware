bool load_eeprom() {
  reset_variables();

  eeprom_get(factory_data);
  factory_data = eeprom_buffer.as_factory_data;

  if (factory_data.signature != 0x5555) { //Memory is unsigned (unknown state)
    Serial.println(F("Memory not signed"));
    return false;
  }

  if (factory_data.hw_version.major == 0xFF) {
    Serial.println(F("Hardware version not set"));
    return false;
  } else if (factory_data.hw_version.major > hw_support_major) {
    Serial.println(F("Hardware version not supported")); //todo: make more verbose
    return false;
  }

  if (factory_data.bt_fw_version.major == 0xFF) {
    Serial.println(F("Bluetooth firmware version not set"));
    return false;
  } else if (factory_data.bt_fw_version.major > bt_fw_support_major) {
    Serial.println(F("Bluetooth firmware version not supported")); //todo: make more verbose
    return false;
  }

  uint16_t allocation_table_offset = offsetof(eeprom_layout, table);
  uint8_t entry_size = sizeof(entry_struct);

  for (int i = 0; i < 32; i++) { //todo: verbosity
    Serial.print(F("Loading entry "));
    Serial.println(i);
    
    uint16_t entry_offset = allocation_table_offset + entry_size * i;
    read_eeprom(entry_offset, entry_size);
    entry_struct entry = eeprom_buffer.as_entry;

    if (calculate_crc(entry.location, entry.size) != entry.crc) {
      Serial.println(F("Invalid CRC"));
      return false;
    }

    if (entry.type == entry_type_enum::system_variables) {
      read_eeprom(entry.location, entry.size);
      user = eeprom_buffer.as_user_data; //rename
    } else if (entry.type == entry_type_enum::dsp_default) {
      load_dsp(entry.location, entry.size, entry.amp);
    }
  }

  return true; //sucess
}

void load_entries() {
  
  //Go back to book 0 page 0
//  writeRegisterDual(0, 0);
//  writeRegisterDual(0x7F, 0);
//  writeRegisterDual(0, 0);
}

void reset_variables() {
  user.amp_1_enabled = true;
  user.amp_2_enabled = true;
  user.vol_start_enabled = true;
  user.vol_start = -30;
  user.vol_max = 0;
  user.power_low = 0;
  user.power_shutdown = 0;

  vol = -30;                                    //Default: -30dB
  vol_old = 0;                                   //Default: 0dB todo: force?
  analog_gain = analog_gain_old = 0;               //Default: 0dB
  eq_state = eq_state_old = digitalRead(eq_sw);     //Enable if EQ switch is closed todo verify
  eq_enabled = eq_enabled_old = digitalRead(eq_sw);  //Enable if EQ switch is closed
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
