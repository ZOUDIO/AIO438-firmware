void asciiDataHandler() {
  char *command = strtok (incomingData._char, " ");     //Commands are space-delimited

  if (!strcmp(command, "MODEL")) {
    Serial.println(MODEL);
  } else if (!strcmp(command, "FIRMWARE")) {
    Serial.println(FIRMWARE);
  } else if (!strcmp(command, "ECHO")) {
    Serial.println(F("ECHO"));
  } else if (!strcmp(command, "DISABLE")) {
    systemEnable = false;
  } else if (!strcmp(command, "VOL")) {
    command = strtok (NULL, " ");
    if (command != NULL) {  //If there is a command: set volume
      vol = atof(command);
    } else {                //Else: print volume
      Serial.println(vol);
    }
  } else if (!strcmp(command, "PULSE")) {
    byte PIO = strtol(strtok (NULL, " "), NULL, 0);
    int duration = strtol(strtok (NULL, " "), NULL, 0);
    sendPulse(PIO, duration);
  } else if (!strcmp(command, "STATUS")) {
    getStatus();
  } else if (!strcmp(command, "EEPROM")) {
    command = strtok (NULL, " ");
    if (!strcmp(command, "DUMP")) {
      eepromDump();
    } else if (!strcmp(command, "CLEAR")) {
      eepromClear();
    }
  } else {  //If command is not recognized
    Serial.print(F("nAck: "));
    Serial.println(command);
  }
}

void eepromDump() {
  Serial.println("Start dumping first 5000 bytes of eeprom...");
  for (long i = 0; i < 5000; i += 0x10) { //Go per 16 bytes (hex)
    readEeprom(i, 0x10); //Read 16 bytes
    for (int j = 0; j < 0x10; j++) {
      if (eepromBuffer.asBytes[j] < 0x10) {
        Serial.print("0"); //Pre-fix 0 for formatting
      }
      Serial.print(eepromBuffer.asBytes[j], HEX);
      Serial.print(" ");
    }
    Serial.println();
  }
  Serial.println("Finished dumping");
}

void eepromClear() {
  Serial.println("Start clearing...");
  for (long i = 0; i < EEPROM_SIZE; i += PAGE_SIZE) {
    byte allZeroes[64];
    memset(allZeroes, 0, 64);
    writeEeprom(i, PAGE_SIZE, allZeroes);
  }
  Serial.println("Finished clearing");
}


void getStatus() {
  writeRegisterDual(0x78, 0x80);      //Clear all faults
  Serial.print(F("powerVoltage = "));
  Serial.println(powerVoltage);
  Serial.print(F("Volume = "));
  Serial.println(vol);
  getStatusAmp(AMP1);
  getStatusAmp(AMP2);
  //Load eeprom
}

void getStatusAmp(int amp) {
  if (amp == AMP1) Serial.println(F("Status AMP1:"));
  if (amp == AMP2) Serial.println(F("Status AMP2:"));

  byte reg39h = readRegister(amp, 0x39);
  if (bitRead(reg39h, 0)) Serial.println(F("Invalid sampling rate"));
  if (bitRead(reg39h, 1)) Serial.println(F("Invalid SCLK"));
  if (bitRead(reg39h, 2)) Serial.println(F("Missing SCLK"));
  if (bitRead(reg39h, 3)) Serial.println(F("PLL not locked"));
  if (bitRead(reg39h, 4)) Serial.println(F("Invalid PLL rate"));
  if (bitRead(reg39h, 5)) Serial.println(F("Invalid SCLK rate"));

  byte reg68h = readRegister(amp, 0x68);
  if (reg68h == 0) Serial.println(F("State: deep sleep"));
  else if (reg68h == 1) Serial.println(F("State: sleep"));
  else if (reg68h == 2) Serial.println(F("State: high impedance"));
  else if (reg68h == 3) Serial.println(F("State: play"));

  byte reg70h = readRegister(amp, 0x70);
  if (bitRead(reg70h, 0)) Serial.println(F("Overcurrent: right channel"));
  if (bitRead(reg70h, 1)) Serial.println(F("Overcurrent: left channel"));
  if (bitRead(reg70h, 2)) Serial.println(F("DC fault: right channel"));
  if (bitRead(reg70h, 3)) Serial.println(F("DC fault: left channel"));

  byte reg71h = readRegister(amp, 0x71);
  if (bitRead(reg71h, 0)) Serial.println(F("PSU under voltage"));
  if (bitRead(reg71h, 1)) Serial.println(F("PSU over voltage"));
  if (bitRead(reg71h, 2)) Serial.println(F("Clock fault"));

  byte reg72h = readRegister(amp, 0x72);
  if (bitRead(reg72h, 0)) Serial.println(F("High temperature: shutdown"));
  if (bitRead(reg72h, 1)) Serial.println(F("CBC fault: left channel"));
  if (bitRead(reg72h, 2)) Serial.println(F("CBC fault: right channel"));

  byte reg73h = readRegister(amp, 0x73); //First 4 bits get checked every second by temperatureMonitor()
  if (bitRead(reg73h, 4)) Serial.println(F("CBC overcurrent: right channel"));
  if (bitRead(reg73h, 5)) Serial.println(F("CBC overcurrent: left channel"));
}
