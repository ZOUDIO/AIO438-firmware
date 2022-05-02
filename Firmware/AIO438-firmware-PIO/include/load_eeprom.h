#ifndef LOAD_EEPROM
#define LOAD_EEPROM

#include <Arduino.h>
#include "main.h"
#include "I2C.h"
#include "monitors.h"

void load_eeprom();
void set_amp_output_state(uint8_t amp_index, uint8_t amp_addr, uint8_t output);
bool load_factory_data();
void load_system_variables();
uint8_t load_dsp_entries(bool _verbose);
void load_dsp(int start_reg, int amount, byte amp);
uint16_t calculate_crc(int start_reg, int amount);

#endif


