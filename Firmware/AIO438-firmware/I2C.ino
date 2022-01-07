/* Legacy struct code
#define member_size(type, member) sizeof(((type *)0)->member)                                               //Get struct member size in bytes
#define eepromPut(member) writeStructToEeprom(offsetof(variables, member), member_size(variables, member))  //Save struct member on eeprom
#define eepromGet(member) read_eeprom(offsetof(variables, member), member_size(variables, member))           //Get struct member from eeprom
*/

void read_eeprom(int reg, int amount) {   //Read bytes from eeprom
  Wire.beginTransmission(eeprom_ext);
  Wire.write(reg >> 8);   // MSB
  Wire.write(reg & 255);  // LSB
  Wire.endTransmission();
  Wire.requestFrom(eeprom_ext, amount);
  for (byte i = 0; i < amount; i++ ) {
    eeprom_buffer.as_bytes[i] = Wire.read();
  }
}

void write_eeprom_setup(int member_offset, int member_size) { //Todo: some kind of checking?

  memcpy(eeprom_buffer.as_bytes, incoming_data._byte + 2, actual_data_count); //Put data in buffer so it can be sent to eeprom
  
  //If data crosses page boundary, it needs to be written in two parts to avoid roll-over, todo put in eeprom function
  byte amount_available = page_size - (member_offset % page_size);  //How much can be written on the current page
  byte amount_current_page = min(member_size, amount_available);      //Only write what fits on the current page

  write_eeprom(member_offset, amount_current_page, eeprom_buffer.as_bytes);
  
  byte amount_next_page = member_size - amount_current_page;
  if (amount_next_page > 0) { //If there is something to write
    write_eeprom(member_offset + amount_current_page, amount_next_page, eeprom_buffer.as_bytes + amount_current_page); //Write what remains on the next page
  }
}

void write_eeprom(int reg, int amount, byte data[]) {  //Write bytes to eeprom
  Wire.beginTransmission(eeprom_ext);
  Wire.write(reg >> 8);   // MSB
  Wire.write(reg & 255);  // LSB
  for (byte i = 0; i < amount; i++) {
    Wire.write(data[i]);
  }
  Wire.endTransmission();
  delay(5);
}

void write_register(byte amp, byte reg, byte data) {  //Write register and its value
  Wire.beginTransmission(amp);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}

void write_register_dual(byte reg, byte data) { //Write to both amplifiers
  write_register(amp_1, reg, data);
  write_register(amp_2, reg, data);
}

void burst_write(byte amp, byte len) {
  Wire.beginTransmission(amp);
  for (int i = 0; i < len; i++) {
    Wire.write(eeprom_buffer.as_bytes[i]);
  }
  Wire.endTransmission();
}

int read_register(int amp, byte reg) {  //Read register value
  Wire.beginTransmission(amp);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(amp, 1);
  return Wire.read();
}
