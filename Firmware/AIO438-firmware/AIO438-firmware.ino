/*
  ZOUDIO AIO438 firmware
  For questions mail "info@zoudio.com" or visit "www.zoudio.com"

  Microcontroller: ATMEGA328PB at 3.3V with 8MHz external crystal
  Bootloader: https://github.com/MCUdude/MiniCore with BOD set to 1.8V
    Note: V2.0.4 or higher needs a modified upload speed to work
  Recommended programmer to burn bootloader: USBasp
  Board has onboard CP2102N for communication and firmware uploads via UART
*/

/* General todo list
  - use flash strings (use some kind of general definition?)
  - include libs in src/ and use submodules
  - look at better file split
  - consolidate i/o data
  - remove unions, use memcpy?
  - pass by reference instead of value?
  - Improve error reporting to master
  - Check for naming consistency and clarity
  - Clean up comments
  - Use more approriate file types and constants(?)
  - Check function input constraints
*/

#include "src/ArduinoLibraryWire/Wire.h" //Modified Wire.h with 66 byte buffer to send 64 byte arrays with 2 byte address
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

const char model[] = "AIO438"; //Todo: put/get in/from eeprom
const char firmware[] = "1.0.0";

//Can to pass to functions
const int amp_dual = 0; //Write to both amps, todo: pack in enum, typesafety icm address?
const int amp_1 = 1;
const int amp_2 = 2;
const bool verbose = true;

//PurePathConsole constants
const byte cfg_meta_burst = 253;
const byte cfg_meta_delay = 254;

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
uint16_t runtime_crc[32];

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
bool eeprom_loaded;

ClickButton rot_button(rot_sw, HIGH); //Encoder switch
ClickButton tws_button(tws_sw, HIGH); //TrueWirelessStereo button
Rotary rot = Rotary(rot_a, rot_b);    //Encoder
CRC16 crc;

void setup() { //Todo: move pinmodes to after eeprom reading?
  pinMode(vreg_sleep, OUTPUT);  //Hard-wired
  pinMode(0, INPUT_PULLUP);     //Pull up the RX pin to avoid accidental interrupts
  Serial.begin(38400);
  Wire.begin();
  Wire.setClock(400000);
  rot.begin();

  static_assert(sizeof(entry_struct) == 64, "Entry_struct size has changed");
  static_assert(sizeof(system_variables) == 19, "System_variables size has changed");
  static_assert(offsetof(eeprom_layout, user) == 128, "User offset has changed");
  static_assert(offsetof(eeprom_layout, first_entry) == 256, "First_entry offset has changed");

  vol = user.vol_start; //Set once
}

void loop() {
  disable_system();
  /* System will be in powerdown until interrupt occurs */
  enable_system();
  while (system_enabled) {
    serial_monitor();
    power_monitor();
    if (eeprom_loaded) { //Todo check if safe
      temperature_monitor();
      analog_gain_monitor();
      aux_level_monitor();
      rotary_monitor();
      tws_monitor();
      eq_monitor();
    }
    wdt_reset(); //System will reset if loop hangs for more than 8 seconds
  }
}

void enable_system() { //Enable or disable system todo: look at sequence (also i2s)
  while (digitalRead(rot_sw) == HIGH) {}; //Continue when rotary switch is released
  Serial.println(F("Powering on..."));
  digitalWrite(vreg_sleep, HIGH);         //Set buckconverter to normal operation

  if (!detect_device(eeprom_ext)) {
    Serial.println(F("Eeprom not found"));
    return;
  }

  eeprom_loaded = load_eeprom();
  if (!eeprom_loaded) {
    Serial.println(F("Failed to load settings from memory"));
    Serial.println(F("Amplifier outputs are disabled"));
  }

  while (Serial.available() > 0) {
    Serial.read();  //Clear serial input buffer
  }
  Serial.println(F("On"));
  system_enabled = true;
}

void disable_system() {
  Serial.println("Powering off...");
  set_led("OFF");
  digitalWrite(bt_enable, LOW);
  delay(1100);  //Wait for everything to power off
  digitalWrite(amp_1_pdn, LOW);
  digitalWrite(amp_2_pdn, LOW);
  digitalWrite(expansion_en, LOW);
  Serial.println(F("Off"));
  Serial.flush();
  digitalWrite(vreg_sleep, LOW);    //Set buckconverter to sleep
  attachPinChangeInterrupt(0, exit_powerdown, CHANGE);       //Wake up from serial todo null pointer function?, update lib?
  attachPinChangeInterrupt(rot_sw, exit_powerdown, CHANGE);  //Wake up from rotary switch
  wdt_disable();                    //Disable watchdog timer
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  /* System will be in powerdown until interrupt occurs */
  wdt_enable(WDTO_8S);              //Enable 8 seconds watchdog timer todo enable
  detachPinChangeInterrupt(0);       //Wake up from serial
  detachPinChangeInterrupt(rot_sw);  //Wake up from rotary switch
}

void set_outputs() { //Remainder of GPIO are default pinmode (INPUT)
  pinMode(expansion_en, OUTPUT);
  pinMode(bt_enable, OUTPUT);
  pinMode(amp_1_pdn, OUTPUT);
  pinMode(amp_2_pdn, OUTPUT);
  pinMode(bt_pio_19, OUTPUT);
  pinMode(bt_pio_20, OUTPUT);
  pinMode(bt_pio_21, OUTPUT);
  pinMode(bt_pio_22, OUTPUT);
  pinMode(led_green, OUTPUT);
  pinMode(led_red, OUTPUT);
}

void exit_powerdown() {} //Empty ISR to exit powerdown

void send_pulse(byte PIO, int duration) { //Send a pulse to a BT PIO to 'press' it (see GPIO mapping for more info) todo documentation
  digitalWrite(PIO, HIGH);
  duration = min(duration, 2000); //2000ms maximum duration to avoid hanging
  delay(duration);
  digitalWrite(PIO, LOW);
}

void set_led(String color) { //Set led color
  if (color == "RED") {
    digitalWrite(led_red, HIGH);
    digitalWrite(led_green, LOW);
  } else if (color == "GREEN") {
    digitalWrite(led_green, HIGH);
    digitalWrite(led_red, LOW);
  } else { //Off
    digitalWrite(led_green, LOW);
    digitalWrite(led_red, LOW);
  }
}

void set_led(String color, int _delay) { //Set led color and wait
  set_led(color);
  delay(_delay);
}

void set_vol() {
  if (vol != vol_old) {
    vol = max(vol, -103.5);           //Set absolute minimum
    vol = min(vol, user.vol_max);     //Set user-defined maximum
    vol = min(vol, 24);               //Set absolute maximum
    byte vol_register = 48 - 2 * vol; //Convert decibel to register value
    write_single_register(amp_dual, 0x4C, vol_register);  //Write to amps
    vol_old = vol;
  }
}

uint16_t swap_int ( const uint16_t inFloat ) { //Todo; do by reference
  uint16_t retVal;
  char *floatToConvert = ( char* ) & inFloat;
  char *returnFloat = ( char* ) & retVal;

  // swap the bytes into a temporary buffer
  returnFloat[0] = floatToConvert[1];
  returnFloat[1] = floatToConvert[0];

  return retVal;
}

float swap_float ( const float inFloat ) { //Todo; do by reference
  float retVal;
  char *floatToConvert = ( char* ) & inFloat;
  char *returnFloat = ( char* ) & retVal;

  // swap the bytes into a temporary buffer
  returnFloat[0] = floatToConvert[3];
  returnFloat[1] = floatToConvert[2];
  returnFloat[2] = floatToConvert[1];
  returnFloat[3] = floatToConvert[0];

  return retVal;
}
