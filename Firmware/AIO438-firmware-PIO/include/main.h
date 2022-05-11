#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>

// #include "Wire.h" //Modified Wire.h with 66 byte buffer to send 64 byte arrays with 2 byte address
#include "PinChangeInt.h"     //Pin Change Interrupt library from https://github.com/asheeshr/PinChangeInt.git //todo Switch to nicohood?
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

extern const char* model; //Todo: put/get in/from eeprom
extern const char* firmware;

//Can to pass to functions
extern const int amp_dual; //Write to both amps, todo: pack in enum, typesafety icm address?
extern const int amp_1;
extern const int amp_2;
extern const bool verbose;

//PurePathConsole constants
extern const byte cfg_meta_burst;
extern const byte cfg_meta_delay;

extern const uint16_t valid_signature;

typedef struct {
  uint8_t command;
  uint8_t param;
} cfg_reg;

typedef union { //Used for type conversion
  byte as_bytes[64];
  float as_float;
  factory_data_struct as_factory_data;
  system_variables as_user_data; //todo: rename?
  entry_struct as_entry;
  cfg_reg as_cfg_reg;
} eeprom_buffer_union;

extern eeprom_buffer_union eeprom_buffer;

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

extern const byte array_size;  //Payload plus CRC16
extern byte temp_buffer[];//Worst case scenario every value is 253 or higher, which needs two bytes to reconstruct
extern byte outgoing_data[];   //Data before encoding

typedef union {
  byte _byte[sizeof(payload)+2]; //Get as byte array
  char _char[sizeof(payload)+2]; //Get as char array
} incoming_data_union; //todo, with reinterpret_cast?, look at temp, incoming, outgoing etc shared memory space

extern incoming_data_union incoming_data;

extern byte incoming_data_count;
extern byte outgoing_data_count;
extern byte temp_buffer_count;
extern char actual_data_count; //Can be negative todo: see if actually needed
extern bool currently_receiving;

extern bool eq_enabled;
extern bool eq_enabled_old;
extern bool eq_state;
extern bool eq_state_old;
extern bool system_enabled;
extern bool apply_settings_flag; //todo: Rename?
extern float vol;
extern float vol_old;
extern float vol_reduction;
extern float power_voltage;
extern char analog_gain;
extern char analog_gain_old;
extern bool full_functionality;
extern bool invalid_entry_stored;

extern ClickButton rot_button; //Encoder switch
extern ClickButton tws_button; //TrueWirelessStereo button
extern Rotary rot;    //Encoder
extern CRC16 crc;

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
void initialize_global_variables();

#endif