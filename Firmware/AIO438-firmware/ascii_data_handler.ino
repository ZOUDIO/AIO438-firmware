void ascii_data_handler() {
  char *command = strtok (incoming_data._char, " ");  //Commands are space-delimited

  if (!strcmp(command, "MODEL")) {
    Serial.println(model);
  } else if (!strcmp(command, "FIRMWARE")) {
    Serial.println(firmware);
  } else if (!strcmp(command, "ECHO")) {
    Serial.println(F("ECHO"));
  } else if (!strcmp(command, "DISABLE")) {
    system_enabled = false;
  } else if (!strcmp(command, "VOL")) {
    command = strtok (NULL, " ");
    if (command != NULL) {  //If there is a command: set volume
      vol = atof(command);
    } else {                //Else: print volume
      Serial.println(vol);
    }
  } else if (!strcmp(command, "PULSE")) {
    byte pio = strtol(strtok (NULL, " "), NULL, 0);
    int duration = strtol(strtok (NULL, " "), NULL, 0);
    send_pulse(pio, duration);
  } else if (!strcmp(command, "STATUS")) {
    get_status();
  } else if (!strcmp(command, "DUMP")) {
    command = strtok (NULL, " ");
    if (!strcmp(command, "EEPROM")) {
      long end_reg = strtol(strtok (NULL, " "), NULL, 0);
      dump_eeprom(end_reg);
    } else if (!strcmp(command, "DSP")) {
      dump_dsp();
    }
  } else {  //If command is not recognized
    Serial.println(F("nAck: "));
    for (int i = 0; i < 20; i++) {
      Serial.println(incoming_data._byte[i]);
    }
    //Serial.println(incoming_data._char);
  }
}

void dump_eeprom(long end_reg) { //todo formatting
  Serial.println("Offset\t00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F"); //Print header
  for (int i = 0; i < end_reg; i += 16) {
    read_eeprom(i, 16);
    Serial.print(i, HEX);
    Serial.print('\t');
    for (int j = 0; j < 16; j++) {
      if (eeprom_buffer.as_bytes[j] < 16) {
        Serial.print("0");
      }
      Serial.print(eeprom_buffer.as_bytes[j], HEX);
      Serial.print(" ");
    }
    Serial.println();
  }
}

void dump_dsp() {
  //todo?
}

void get_status() { //todo error checking before displaying, check for completeness
  write_single_register(amp_dual, 0x78, 0x80);  //Clear all faults

  Serial.print(F("Model = "));
  Serial.println(model);
  Serial.print(F("Firmware version = "));
  Serial.println(firmware);
  Serial.print(F("Hardware version = "));
  print_version_struct(factory_data.hw_version);
  Serial.print(F("Bluetooth firmware version = "));
  print_version_struct(factory_data.bt_fw_version);

  Serial.print(F("Amp 1 enabled = "));
  Serial.println(user.amp_1_enabled);
  Serial.print(F("Amp 2 enabled = "));
  Serial.println(user.amp_2_enabled);
  Serial.print(F("Bluetooth enabled = "));
  Serial.println(user.bt_enabled);

  Serial.print(F("Volume (dB) = "));
  Serial.println(vol);
  Serial.print(F("Start volume enabled = "));
  Serial.println(user.vol_start_enabled);
  Serial.print(F("Start volume (dB) = "));
  Serial.println(user.vol_start);
  Serial.print(F("Max volume (dB) = "));
  Serial.println(user.vol_max);

  Serial.print(F("Voltage (V) = "));
  Serial.println(power_voltage);
  Serial.print(F("Power low (V) = "));
  Serial.println(user.power_low);
  Serial.print(F("Power shutdown (V) = "));
  Serial.println(user.power_shutdown);

  Serial.println();
  Serial.println(F("Status amp 1:"));
  get_status_amp(amp_1_addr);
  Serial.println(F("Status amp 2:"));
  get_status_amp(amp_2_addr);

  Serial.println();
  //todo eeprom table report?
}

void print_version_struct(version_struct _struct) { //Print like "major.minor.patch"
  Serial.print(_struct.major);
  Serial.print(F("."));
  Serial.print(_struct.minor);
  Serial.print(F("."));
  Serial.println(_struct.patch);
}

void get_status_amp(int amp) { //todo extend? or change to pure register reporting
  byte reg39h = read_register(amp, 0x39);
  if (bitRead(reg39h, 0)) Serial.println(F("Invalid sampling rate"));
  if (bitRead(reg39h, 1)) Serial.println(F("Invalid SCLK"));
  if (bitRead(reg39h, 2)) Serial.println(F("Missing SCLK"));
  if (bitRead(reg39h, 3)) Serial.println(F("PLL not locked"));
  if (bitRead(reg39h, 4)) Serial.println(F("Invalid PLL rate"));
  if (bitRead(reg39h, 5)) Serial.println(F("Invalid SCLK rate"));

  byte reg68h = read_register(amp, 0x68);
  if (reg68h == 0) Serial.println(F("State: deep sleep"));
  else if (reg68h == 1) Serial.println(F("State: sleep"));
  else if (reg68h == 2) Serial.println(F("State: high impedance"));
  else if (reg68h == 3) Serial.println(F("State: play"));

  byte reg70h = read_register(amp, 0x70);
  if (bitRead(reg70h, 0)) Serial.println(F("Overcurrent: right channel"));
  if (bitRead(reg70h, 1)) Serial.println(F("Overcurrent: left channel"));
  if (bitRead(reg70h, 2)) Serial.println(F("DC fault: right channel"));
  if (bitRead(reg70h, 3)) Serial.println(F("DC fault: left channel"));

  byte reg71h = read_register(amp, 0x71);
  if (bitRead(reg71h, 0)) Serial.println(F("PSU under voltage"));
  if (bitRead(reg71h, 1)) Serial.println(F("PSU over voltage"));
  if (bitRead(reg71h, 2)) Serial.println(F("Clock fault"));

  byte reg72h = read_register(amp, 0x72);
  if (bitRead(reg72h, 0)) Serial.println(F("High temperature: shutdown"));
  if (bitRead(reg72h, 1)) Serial.println(F("CBC fault: left channel"));
  if (bitRead(reg72h, 2)) Serial.println(F("CBC fault: right channel"));

  byte reg73h = read_register(amp, 0x73); //First 4 bits get checked every second by temperature_monitor()
  if (bitRead(reg73h, 4)) Serial.println(F("CBC overcurrent: right channel"));
  if (bitRead(reg73h, 5)) Serial.println(F("CBC overcurrent: left channel"));
}
