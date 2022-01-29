bool load_eeprom() {
  //Start loading factory data
  eeprom_get(factory_data);
  factory_data = eeprom_buffer.as_factory_data;

  if (factory_data.signature != 0x5555) {
    Serial.println(F("Invalid factory data")); //Todo: offer to clear memory and sign or something?
    return false;
  }

  if (factory_data.hw_version.major != 1) { //This firmware supports hardware versions 1.x.x
    Serial.print(F("Hardware version "));
    Serial.print(factory_data.hw_version.major);
    Serial.println(F(" not supported"));
    return false;
  }
  set_outputs(); //Now that hardware is known, outputs can be enabled safely

  if (factory_data.bt_fw_version.major != 1) { //This firmware supports bluetooth firmware versions 1.x.x
    Serial.print(F("Bluetooth firmware version "));
    Serial.print(factory_data.hw_version.major);
    Serial.println(F(" not supported"));
    return false;
  }

  //Start loading user data
  eeprom_get(user);
  user = eeprom_buffer.as_user_data;

  user.vol_start = swap_float(user.vol_start);
  user.vol_max = swap_float(user.vol_max);
  user.power_low = swap_float(user.power_low);
  user.power_shutdown = swap_float(user.power_shutdown);

  if (user.signature != 0x5555) {
    reset_variables();
    Serial.println("Default system variables loaded");
  }

  digitalWrite(expansion_en, HIGH);   //Enable power to expansion connector

  if (user.bt_enabled) {
    digitalWrite(bt_enable, HIGH);    //Enable BT module
  }

  digitalWrite(amp_1_pdn, HIGH);      //Enable amplifier 1
  digitalWrite(amp_2_pdn, HIGH);      //Enable amplifier 2
  delay(10);
  if (!detect_device(amp_1_addr)) {
    Serial.println(F("Amplifier 1 not found"));
    return false;
  }
  if (!detect_device(amp_2_addr)) {
    Serial.println(F("Amplifier 2 not found"));
    return false;
  }

  write_single_register(amp_2, 0x6A, 0b1011);  //Set ramp clock phase of amp_2 to 90 degrees (see 7.4.3.3.1 in datasheet)

  if (!load_eeprom_user(verbose)) {
    return false;
  }

  if (user.vol_start_enabled) {
    vol = user.vol_start; //Set vol if enabled
  }
  vol_old = -104;         //Set vol_old out of range to force set_vol to run
  set_vol();              //Set volume

  analog_gain_old = 32;   //Set analog_gain_old out of range to force analog_gain_monitor to run
  analog_gain_monitor();  //Set analog gain

  eq_state = eq_state_old = digitalRead(eq_sw);     //Enable if EQ switch is closed
  eq_enabled = eq_enabled_old = digitalRead(eq_sw); //Enable if EQ switch is closed

  //Set enabled amps into play mode
  if (user.amp_1_enabled) {
    write_single_register(amp_1, 0x03, 0b11);
  }
  if (user.amp_2_enabled) {
    write_single_register(amp_2, 0x03, 0b11);
  }

  send_pulse(bt_pio_19, 400); //Play startup tone

  return true;
}

bool load_eeprom_user(bool _verbose) {

  entry_struct entry; //To load entry's into
  uint16_t entry_offset = offsetof(eeprom_layout, first_entry);
  uint8_t entry_size = sizeof(entry_struct);

  if (_verbose) {
    Serial.println("Type\tAmp\tSize\tCRC\tName"); //Print header
  }

  while (true) {

    read_eeprom(entry_offset, entry_size);
    entry = eeprom_buffer.as_entry;

    entry.crc = swap_int(entry.crc);
    entry.size = swap_int(entry.size);

    if (_verbose) {
      Serial.print(entry.type);
      Serial.print("\t");
      Serial.print(entry.amp);
      Serial.print("\t");
      Serial.print(entry.size);
      Serial.print("\t");
      Serial.print("0x");
      Serial.print(entry.crc, HEX);
      Serial.print("\t");
      Serial.println(entry.name);
    }
    
    uint16_t crc_calculated = calculate_crc(entry_offset + entry_size, entry.size);
    if (entry.crc != crc_calculated) {
      Serial.print(F("Invalid CRC, calculated value = "));
      Serial.print("0x");
      Serial.println(crc_calculated, HEX);
      return false;
    }

    return true; //Swap order below and so

    if (_verbose) { //Verbose only prints, does not execute
      continue; //Jump to next for-loop iteration
    }

  }

  //
  //    if (entry.type < 1) { //Only types 0 and 1 are supported currently
  //      uint16_t crc_calculated = calculate_crc(address_int, size_int);
  //      if (crc_int != crc_calculated) {
  //        Serial.print(F("Invalid CRC, calculated value = "));
  //        Serial.print("0x");
  //        Serial.println(crc_calculated, HEX);
  //        return false;
  //      }
  //    }
  //
  //    if (entry.type == static_cast<uint8_t>(entry_type_enum::system_variables)) {
  //      memset(&user, 0xFF, sizeof(user)); //Default all bits to 1
  //      read_eeprom(address_int, size_int);
  //
  //      //If saved data is bigger than local struct, ignore excess data.
  //      //If local struct is bigger than saved data, fill local struct partly
  //      memcpy(&user, eeprom_buffer.as_bytes, size_int); //Copy data into user runtime struct
  //
  //      //Swap byte order (saved data is big endian, processor is little endian)
  //      user.vol_start = swap_float(user.vol_start);
  //      user.vol_max = swap_float(user.vol_max);
  //      user.power_low = swap_float(user.power_low);
  //      user.power_shutdown = swap_float(user.power_shutdown);
  //
  //      system_variables_loaded = true;
  //    } else if (entry.type == static_cast<uint8_t>(entry_type_enum::dsp_default)) {
  //      //load_dsp(address_int, size_int, entry.amp);
  //      dsp_loaded = true;
  //    } //todo, load eq's (how?)
  //  }
  //
  //  if (!system_variables_loaded) {
  //    reset_variables();
  //    Serial.println(F("Default system variables loaded"));
  //  }
  //
  //  if (!dsp_loaded) {   //Overwrite these settings todo copy to configtool
  //    write_single_register(amp_dual, 0x02, 0b1000001);   //1SPW modulation, BTL output, 768k Fsw
  //    write_single_register(amp_dual, 0x53, 0b1100000);   //Set Class D bandwith to 175Khz
  //    write_single_register(amp_dual, 0x76, 0b10000);     //Enable over-temperature auto recovery
  //    write_single_register(amp_dual, 0x77, 0b111);       //Enable Cycle-By-Cycle over-current protection
  //    Serial.println(F("Default DSP settings loaded"));
  //  }

  return true;
}

void reset_variables() {
  user.amp_1_enabled = true;
  user.amp_2_enabled = true;
  user.bt_enabled = true;
  user.vol_start_enabled = true;
  user.vol_start = -30;
  user.vol_max = 0;
  user.power_low = 0;
  user.power_shutdown = 0;
  vol_reduction = 0;
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
  uint16_t end_reg = start_reg + amount;
  crc.reset();
  crc.setStartXOR(0xFFFF); //Matches CCITT guidelines
  for (uint16_t i = start_reg; i < end_reg; i += page_size) { //todo optimize
    read_eeprom(i, page_size); //Read one page
    uint8_t add_amount = min(page_size, end_reg - i); //Add a maximum of one page at a time
    crc.add(eeprom_buffer.as_bytes, add_amount);
    wdt_reset();
  }
  return crc.getCRC();
}
