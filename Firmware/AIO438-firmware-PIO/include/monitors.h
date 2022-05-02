#ifndef MONITORS_H
#define MONITORS_H

#include <Arduino.h>
#include "main.h"
#include "binary_data_handler.h"
#include "ascii_data_handler.h"

void serial_monitor();
void temperature_monitor();
void rotary_monitor();
void power_monitor();
void analog_gain_monitor();
void tws_monitor();
void eq_monitor();
void aux_level_monitor();

#endif