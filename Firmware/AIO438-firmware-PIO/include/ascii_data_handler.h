#ifndef ASCII_DATA_HANDLER_H
#define ASCII_DATA_HANDLER_H

#include "main.h"
#include<stdint.h>
#include "eeprom_layout.h"
#include <Arduino.h>
#include "I2C.h"
#include "load_eeprom.h"

extern const char* model;
extern const char* firmware;

extern const int amp_dual;

extern const bool verbose;

extern const uint16_t valid_signature;

extern bool system_enabled;
extern float vol;
extern float power_voltage;
extern bool full_functionality;

extern const char *amp_output_state_str[];

void ascii_data_handler();
void clear_eeprom(uint16_t start_reg);
void dump_eeprom(long end_reg);
void dump_dsp();
void get_status();
void println_version_struct(version_struct _struct);
void get_status_amp(int amp);

#endif