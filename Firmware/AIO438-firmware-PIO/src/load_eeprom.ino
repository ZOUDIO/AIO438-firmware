void load_eeprom() {
  load_system_variables();
  
  if (user.bt_enabled) {
    digitalWrite(bt_enable, HIGH);    //Enable BT module
  }

  write_single_register(amp_2, 0x6A, 0b1011);  //Set ramp clock phase of amp_2 to 90 degrees (see 7.4.3.3.1 in datasheet)

  uint8_t entry_count = load_dsp_entries(!verbose);

  if(invalid_entry_stored) { //Unknown state
    return; //Do not continue with setting up the amps
  }

  if (entry_count == 0) { //If there were no entries
    write_single_register(amp_dual, 0x02, 0b1000001);   //1SPW modulation, BTL output, 768k Fsw
    write_single_register(amp_dual, 0x53, 0b1100000);   //Set Class D bandwith to 175Khz
    Serial.println(F("Default DSP settings loaded"));
  }

  write_single_register(amp_dual, 0x76, 0b10000);     //Enable over-temperature auto recovery
  write_single_register(amp_dual, 0x77, 0b111);       //Enable Cycle-By-Cycle over-current protection

  if (user.vol_start_enabled) {
    vol = user.vol_start; //Set vol if enabled
  }
  vol_old = -104;         //Set vol_old out of range to force set_vol to run
  set_vol();              //Set volume

  analog_gain_old = 32;   //Set analog_gain_old out of range to force analog_gain_monitor to run
  analog_gain_monitor();  //Set analog gain

  set_amp_output_state(1, amp_1_addr, user.amp_1_output);
  set_amp_output_state(2, amp_2_addr, user.amp_2_output);
}

void set_amp_output_state(uint8_t amp_index, uint8_t amp_addr, uint8_t output) { //Todo clean up
  uint8_t current_state = read_register(amp_addr, 0x02); //Used as mask to keep current switching frequency and modulation scheme
  if (output == static_cast<uint8_t>(amp_output_state::dual)) {
    write_single_register(amp_index, 0x02, current_state | 0b000); //BTL mode
  } else if (output == static_cast<uint8_t>(amp_output_state::single)) {
    write_single_register(amp_index, 0x02, current_state | 0b100); //PBTL mode
  } else {
    write_single_register(amp_1, 0x03, 0); //Force deep sleep
    return;
  }
  write_single_register(amp_index, 0x03, 0b11); //Play mode
}

bool load_factory_data() {
  eeprom_get(factory_data);
  factory_data = eeprom_buffer.as_factory_data;

  if (factory_data.signature != valid_signature) {
    Serial.println(F("Invalid factory data")); //Todo: offer to clear memory and sign or something?
    return false;
  }
  
  if (factory_data.hw_version.major != 1) { //This firmware supports hardware versions 1.x.x
    Serial.println(F("Hardware not supported"));
    return false;
  }
  
  if (factory_data.bt_fw_version.major != 1) { //This firmware supports bluetooth firmware versions 1.x.x
    Serial.print(F("Bluetooth firmware not supported"));
    return false;
  }

  return true;
}

void load_system_variables() { //Start loading user data
  eeprom_get(user);
  user = eeprom_buffer.as_user_data;

  user.vol_start = swap_float(user.vol_start);
  user.vol_max = swap_float(user.vol_max);
  user.power_low = swap_float(user.power_low);
  user.power_shutdown = swap_float(user.power_shutdown);

  if (user.signature != valid_signature) {
    user.bt_enabled = true;
    user.vol_start_enabled = true;
    user.vol_start = -30;
    user.vol_max = 0;
    user.power_low = 0;
    user.power_shutdown = 0;
    user.amp_1_output = static_cast<uint8_t>(amp_output_state::dual);
    user.amp_2_output = static_cast<uint8_t>(amp_output_state::dual);
    Serial.println(F("Default system variables loaded"));
  }

  uint8_t output_state_limit = static_cast<uint8_t>(amp_output_state::disable); //Limit range to 3, anything above 3 means disable
  user.amp_1_output = min(user.amp_1_output, output_state_limit);
  user.amp_2_output = min(user.amp_2_output, output_state_limit);

  vol_reduction = 0;
  
  eq_state = eq_state_old = digitalRead(eq_sw);     //Enable if EQ switch is closed
  eq_enabled = eq_enabled_old = digitalRead(eq_sw); //Enable if EQ switch is closed
}

uint8_t load_dsp_entries(bool _verbose) {
  const uint8_t entry_size = sizeof(entry_struct);
  uint16_t entry_offset = offsetof(eeprom_layout, first_entry);
  uint8_t entry_count = 0;
  bool header_printed = false;
  invalid_entry_stored = false;

  for (int i = 0; i < 32; i++) { //Read until the end of eeprom, unless loop breaks itself earlier

    if ((entry_offset + entry_size) > eeprom_size) {
      Serial.println(F("Eeprom size limit reached"));
      break;
    }

    read_eeprom(entry_offset, entry_size);
    entry_struct entry = eeprom_buffer.as_entry;

    entry.crc = swap_int(entry.crc);
    entry.size = swap_int(entry.size);

    if (entry.size < 2 || entry.size > eeprom_size) { //Invalid or empty
      break; //Exit loop
    }

    if (_verbose && !header_printed) {
      Serial.println(F("Index\tType\tAmp\tSize\tCRC\tName")); //Print header once
      header_printed = true;
    }

    if (_verbose) {
      Serial.print(i);
      Serial.print(F("\t"));
      Serial.print(entry.type);
      Serial.print(F("\t"));
      Serial.print(entry.amp);
      Serial.print(F("\t"));
      Serial.print(entry.size);
      Serial.print(F("\t"));
      Serial.print(F("0x"));
      Serial.print(entry.crc, HEX);
      Serial.print(F("\t"));
      Serial.println(entry.name);
    }

    uint16_t crc_calculated = calculate_crc(entry_offset + entry_size, entry.size);
    if (entry.crc != crc_calculated) {
      Serial.print(F("Invalid CRC, calculated value = "));
      Serial.print(F("0x"));
      Serial.println(crc_calculated, HEX);
      invalid_entry_stored = true;
      return false;
    }

    if (!_verbose) { //Verbose only prints, does not execute
      if (entry.type == static_cast<uint8_t>(entry_type_enum::dsp_default)) {
        load_dsp(entry_offset + entry_size, entry.size, entry.amp);
        entry_count++;
      } else {
        Serial.println(F("Entry type not supported"));
      }
    }

    entry_offset = entry_offset + entry_size + entry.size; //Jump to next entry
  }

  return entry_count;
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
        burst_amount = r.param; //Register plus data
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
