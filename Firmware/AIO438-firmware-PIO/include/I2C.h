#ifndef I2C
#define I2C

#include <Arduino.h>
#include "hardware.h"
#include "Wire.h"
#include "main.h"

#define member_size(type, member) sizeof(((type *)0)->member) //Get struct member size in bytes
#define eeprom_put(member) write_eeprom_setup(offsetof(eeprom_layout, member), member_size(eeprom_layout, member), eeprom_buffer.as_bytes)  //Write buffer data to to eeprom
#define eeprom_get(member) read_eeprom(offsetof(eeprom_layout, member), member_size(eeprom_layout, member)) //Fill buffer with eeprom data

bool detect_device(int addr);
void read_eeprom(int reg, int amount);
void write_eeprom_setup(int reg, int amount, byte data[]);
void write_eeprom(int reg, int amount, byte data[]);
void write_single_register(byte amp, byte reg, byte data);
void write_multiple_registers(byte amp, byte data[], byte amount);
void write_registers(byte amp, byte data[], byte amount);
int read_register(int amp_addr, byte reg);

#endif


