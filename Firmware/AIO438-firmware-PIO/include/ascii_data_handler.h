#ifndef ASCII_DATA_HANDLER_H
#define ASCII_DATA_HANDLER_H

#include "main.h"
#include<stdint.h>
#include "eeprom_layout.h"
#include <Arduino.h>
#include "I2C.h"
#include "load_eeprom.h"

void ascii_data_handler();
void clear_eeprom(uint16_t start_reg);
void dump_eeprom(long end_reg);
void dump_dsp();
void get_status();
void println_version_struct(version_struct _struct);
void get_status_amp(int amp);

#endif