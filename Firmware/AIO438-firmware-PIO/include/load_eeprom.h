#pragma once

#include <Arduino.h>
#include "main.h"
#include "I2C.h"
#include "monitors.h"

extern const int amp_dual;
extern const int amp_1;
extern const int amp_2;

extern const bool verbose;

extern const uint16_t valid_signature;

extern CRC16 crc;

extern bool eq_enabled;
extern bool eq_enabled_old;
extern bool eq_state;
extern bool eq_state_old;
extern float vol;
extern float vol_old;
extern float vol_reduction;
extern char analog_gain_old;
extern bool invalid_entry_stored;

void load_eeprom();
void set_amp_output_state(uint8_t amp_index, uint8_t amp_addr, uint8_t output);
bool load_factory_data();
void load_system_variables();
uint8_t load_dsp_entries(bool _verbose);
void load_dsp(int start_reg, int amount, byte amp);
uint16_t calculate_crc(int start_reg, int amount);