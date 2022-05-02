#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>

#include "Wire.h" //Modified Wire.h with 66 byte buffer to send 64 byte arrays with 2 byte address
#include "PinChangeInt.h"     //Pin Change Interrupt library from https://github.com/GreyGnome/PinChangeInt //todo Switch to nicohood?
#include "ClickButton.h"      //Button press detection library from https://github.com/marcobrianza/ClickButton
#include "Rotary.h"           //Rotary encoder library from https://github.com/brianlow/Rotary
#include "CRC.h"              //Cyclic-Redundancy-Check library from https://github.com/RobTillaart/CRC       
#include "CRC16.h"            //Extends CRC.h 
#include "LowPower.h"         //Low power library from https://github.com/canique/Low-Power-Canique/
#include "power2.h"           //Extends LowPower.h
#include "avr/wdt.h"          //Watchdog timer library
#include "hardware.h"         //Hardware determined constants
#include "eeprom_layout.h"    //Struct definitions for eeprom storage
#include "I2C.h"
#include "load_eeprom.h"
#include "monitors.h"

const char model[] = "AIO438"; //Todo: put/get in/from eeprom
const char firmware[] = "2.0.1";

//Can to pass to functions
const int amp_dual = 0; //Write to both amps, todo: pack in enum, typesafety icm address?
const int amp_1 = 1;
const int amp_2 = 2;
const bool verbose = true;

//PurePathConsole constants
const byte cfg_meta_burst = 253;
const byte cfg_meta_delay = 254;

const uint16_t valid_signature = 0x5555;

typedef struct {
  uint8_t command;
  uint8_t param;
} cfg_reg;

union { //Used for type conversion
  byte as_bytes[64];
  float as_float;
  factory_data_struct as_factory_data;
  system_variables as_user_data; //todo: rename?
  entry_struct as_entry;
  cfg_reg as_cfg_reg;
} eeprom_buffer;

struct { //Date comes in big-endian
  uint8_t function_code; //todo: make enum
  union {
    struct {
      uint16_t address;
      uint8_t amount;
      uint8_t data[64];
    } with_addr;
    uint8_t data[67]; //Raw bytes
  };
} payload;

const byte array_size = sizeof(payload) + 2;  //Payload plus CRC16
byte temp_buffer[2 * array_size]; //Worst case scenario every value is 253 or higher, which needs two bytes to reconstruct
byte outgoing_data[array_size];   //Data before encoding

union {
  byte _byte[array_size]; //Get as byte array
  char _char[array_size]; //Get as char array
} incoming_data; //todo, with reinterpret_cast?, look at temp, incoming, outgoing etc shared memory space

byte incoming_data_count;
byte outgoing_data_count;
byte temp_buffer_count;
char actual_data_count; //Can be negative todo: see if actually needed
bool currently_receiving;

bool eq_enabled;
bool eq_enabled_old;
bool eq_state;
bool eq_state_old;
bool system_enabled;
bool apply_settings_flag; //todo: Rename?
float vol;
float vol_old;
float vol_reduction;
float power_voltage;
char analog_gain;
char analog_gain_old;
bool full_functionality;
bool invalid_entry_stored;

ClickButton rot_button(rot_sw, HIGH); //Encoder switch
ClickButton tws_button(tws_sw, HIGH); //TrueWirelessStereo button
Rotary rot = Rotary(rot_a, rot_b);    //Encoder
CRC16 crc;

void setup();
void loop();
bool enable_system();
void disable_system();
void set_outputs();
void exit_powerdown();
void send_pulse(byte PIO, int duration);
void set_led(String color);
void set_led(String color, int _delay);
void set_vol();
uint16_t swap_int ( const uint16_t inFloat );
float swap_float ( const float inFloat );

#endif