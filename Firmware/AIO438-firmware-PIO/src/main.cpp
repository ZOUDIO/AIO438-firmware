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

#include "main.h"

const char* model = "AIO438"; //Todo: put/get in/from eeprom
const char* firmware = "2.0.1";

//Can to pass to functions
const int amp_dual = 0; //Write to both amps, todo: pack in enum, typesafety icm address?
const int amp_1 = 1;
const int amp_2 = 2;
const bool verbose = true;


const uint16_t valid_signature = 0x5555;

ClickButton rot_button(rot_sw, HIGH); //Encoder switch
ClickButton tws_button(tws_sw, HIGH); //TrueWirelessStereo button
Rotary rot = Rotary(rot_a, rot_b);//Encoder
CRC16 crc;

const byte array_size = sizeof(payload) + 2;  //Payload plus CRC16
byte temp_buffer[2 * array_size]; //Worst case scenario every value is 253 or higher, which needs two bytes to reconstruct
byte outgoing_data[array_size];   //Data before encoding

payload_struct payload;
incoming_data_union incoming_data;
eeprom_buffer_union eeprom_buffer;

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

struct factory_data_struct factory_data;
struct system_variables user;

const char *amp_output_state_str[] = {"dual", "single", "disable"};

void setup() { //Todo: move pinmodes to after eeprom reading?
  pinMode(vreg_sleep, OUTPUT);  //Hard-wired
  pinMode(0, INPUT_PULLUP);     //Pull up the RX pin to avoid accidental interrupts
  Serial.begin(38400);
  Wire.begin();
  Wire.setClock(400000);
  rot.begin();

  static_assert(sizeof(entry_struct) == 64, "Entry_struct size has changed");
  static_assert(sizeof(system_variables) == 21, "System_variables size has changed");
  static_assert(offsetof(eeprom_layout, user) == 128, "User offset has changed");
  static_assert(offsetof(eeprom_layout, first_entry) == 256, "First_entry offset has changed");

  vol = user.vol_start; //Set once
}

void loop() {
  disable_system();
  /* System will be in powerdown until interrupt occurs */
  system_enabled = enable_system();
  while (system_enabled) {
    serial_monitor();
    power_monitor();
    if (full_functionality) {
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

bool enable_system() { //Enable or disable system todo: look at sequence (also i2s)
  while (digitalRead(rot_sw) == HIGH) {}; //Continue when rotary switch is released
  Serial.println(F("Powering on..."));
  digitalWrite(vreg_sleep, HIGH);         //Set buckconverter to normal operation

  full_functionality = false;

  if (!detect_device(eeprom_ext)) {
    Serial.println(F("Eeprom not found"));
    return true;
  }

  if (!load_factory_data()) {
    return true;
  }

  set_outputs(); //Now that hardware is known, outputs can be enabled safely

  digitalWrite(expansion_en, HIGH);   //Enable power to expansion connector

  digitalWrite(amp_1_pdn, HIGH);      //Enable amplifier 1
  digitalWrite(amp_2_pdn, HIGH);      //Enable amplifier 2
  delay(10);
  if (!detect_device(amp_1_addr)) {
    Serial.println(F("Amplifier 1 not found")); //todo: do something?
  }
  if (!detect_device(amp_2_addr)) {
    Serial.println(F("Amplifier 2 not found"));
  }

  load_eeprom();

  if (!invalid_entry_stored) {
    full_functionality = true;

    send_pulse(bt_pio_19, 400); //Play startup tone
  }

  while (Serial.available() > 0) {
    Serial.read();  //Clear serial input buffer
  }
  Serial.println(F("On"));
  return true;
}

void disable_system() {
  Serial.println(F("Powering off..."));
  set_led("OFF");
  digitalWrite(bt_enable, LOW);
  delay(1100);  //Wait for everything to power off
  digitalWrite(amp_1_pdn, LOW);
  digitalWrite(amp_2_pdn, LOW);
  digitalWrite(expansion_en, LOW);
  Serial.println(F("Off"));
  Serial.flush();
  digitalWrite(vreg_sleep, LOW);    //Set buckconverter to sleep
  PCattachInterrupt(0, exit_powerdown, CHANGE);       //Wake up from serial todo null pointer function?, update lib?
  PCattachInterrupt(rot_sw, exit_powerdown, CHANGE);  //Wake up from rotary switch
  wdt_disable();                    //Disable watchdog timer
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  /* System will be in powerdown until interrupt occurs */
  wdt_enable(WDTO_8S);              //Enable 8 seconds watchdog timer
  PCdetachInterrupt(0);      //Wake up from serial
  PCdetachInterrupt(rot_sw); //Wake up from rotary switch
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
  char *floatToConvert = (char*) & inFloat;
  char *returnFloat = (char*) & retVal;

  returnFloat[0] = floatToConvert[1];
  returnFloat[1] = floatToConvert[0];

  return retVal;
}

float swap_float ( const float inFloat ) { //Todo; do by reference
  float retVal;
  char *floatToConvert = (char*) & inFloat;
  char *returnFloat = (char*) & retVal;

  returnFloat[0] = floatToConvert[3];
  returnFloat[1] = floatToConvert[2];
  returnFloat[2] = floatToConvert[1];
  returnFloat[3] = floatToConvert[0];

  return retVal;
}

