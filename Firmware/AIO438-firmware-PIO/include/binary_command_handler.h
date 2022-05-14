#ifndef BINARY_COMMAND_HANDLER_H
#define BINARY_COMMAND_HANDLER_H

#include <Arduino.h>
#include "I2C.h"
#include "main.h"

extern char actual_data_count; 

extern bool apply_settings_flag;

void binary_command_handler();

#endif