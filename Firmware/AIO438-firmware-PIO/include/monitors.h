#pragma once

#include <Arduino.h>
#include "main.h"
#include "binary_data_handler.h"
#include "ascii_data_handler.h"

extern Rotary rot;

extern const byte array_size;

extern byte temp_buffer_count;
extern bool currently_receiving;

extern bool eq_enabled;
extern bool eq_enabled_old;
extern bool eq_state;
extern bool system_enabled;
extern float vol;
extern float vol_reduction;
extern float power_voltage;
extern char analog_gain;
extern char analog_gain_old;

extern ClickButton rot_button;
extern ClickButton tws_button;

void serial_monitor();
void temperature_monitor();
void rotary_monitor();
void power_monitor();
void analog_gain_monitor();
void tws_monitor();
void eq_monitor();
void aux_level_monitor();