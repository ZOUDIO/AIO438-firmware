void readEeprom(int reg, int amount) {   //Read bytes from eeprom
  Wire.beginTransmission(EEPROM_EXT);
  Wire.write(reg >> 8);   // MSB
  Wire.write(reg & 255);  // LSB
  Wire.endTransmission();
  Wire.requestFrom(EEPROM_EXT, amount);
  for (byte i = 0; i < amount; i++ ) {
    eepromBuffer.asBytes[i] = Wire.read();
  }
}

void writeStructToEeprom(int memberOffset, int memberSize) {

  if (currentlyReceiving) {               //If data is received via serial
    if (memberSize != actualDataCount) {  //If member size is not the same as the received data size
      faultCode = INVALID_SIZE;
      return;
    }
    memcpy(eepromBuffer.asBytes, incomingData._byte + 2, actualDataCount); //Put data in buffer so it can be sent to eeprom
  }
  
  //If data crosses page boundary, it needs to be written in two parts to avoid roll-over
  byte amountAvailable = PAGE_SIZE - (memberOffset % PAGE_SIZE);  //How much can be written on the current page
  byte amountCurrentPage = min(memberSize, amountAvailable);      //Only write what fits on the current page

  writeEeprom(memberOffset, amountCurrentPage, eepromBuffer.asBytes);
  
  byte amountNextPage = memberSize - amountCurrentPage;
  if (amountNextPage > 0) { //If there is something to write
    writeEeprom(memberOffset + amountCurrentPage, amountNextPage, eepromBuffer.asBytes + amountCurrentPage); //Write what remains on the next page
  }
}

void writeEeprom(int reg, int amount, byte data[]) {  //Write bytes to eeprom
  Wire.beginTransmission(EEPROM_EXT);
  Wire.write(reg >> 8);   // MSB
  Wire.write(reg & 255);  // LSB
  for (byte i = 0; i < amount; i++) {
    Wire.write(data[i]);
  }
  Wire.endTransmission();
  delay(5);
}

void writeRegister(byte amp, byte reg, byte data) {  //Write register and its value
  Wire.beginTransmission(amp);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}

void writeRegisterDual(byte reg, byte data) { //Write to both amplifiers
  writeRegister(AMP1, reg, data);
  writeRegister(AMP2, reg, data);
}

void burstWrite(byte amp, byte len) {
  Wire.beginTransmission(amp);
  for (int i = 0; i < len; i++) {
    Wire.write(eepromBuffer.asBytes[i]);
  }
  Wire.endTransmission();
}

int readRegister(int amp, byte reg) {  //Read register value
  Wire.beginTransmission(amp);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(amp, 1);
  return Wire.read();
}
