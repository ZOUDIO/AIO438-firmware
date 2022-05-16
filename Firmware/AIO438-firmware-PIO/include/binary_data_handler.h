#pragma once

#include <Arduino.h>
#include "I2C.h"
#include "main.h"
#include "binary_command_handler.h"
#include "load_eeprom.h"


extern byte temp_buffer[];
extern byte outgoing_data[];

extern byte incoming_data_count;
extern byte outgoing_data_count;

extern byte temp_buffer_count;
extern char actual_data_count; 

extern bool apply_settings_flag;
extern float vol;

//Serial encoding constants
const byte special_marker = 253; //Value 253, 254 and 255 can be sent as 253 0, 253 1 and 253 2 respectively
const byte start_marker = 254;
const byte end_marker = 255;

void binary_data_handler();
void decode_incoming_data();
void prepare_outgoing_data();
void encode_outgoing_data();
void send_data();
void apply_settings();