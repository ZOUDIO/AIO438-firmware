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

#ifdef AIO438 //Todo handle by flag?
  const char* model = "AIO438";
#endif
#ifdef AIO4CH
  const char* model = "AIO4CH";
#endif

const char* firmware = "2.2.2"; //Todo handle by flag?

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

struct factory_data_struct factory_data;
struct system_variables user;

const char *amp_output_state_str[] = {"dual", "single", "disable"};

void setup() {
  static_assert(sizeof(entry_struct) == 64, "Entry_struct size has changed");
  static_assert(sizeof(system_variables) == 22, "System_variables size has changed");
  static_assert(offsetof(eeprom_layout, user) == 128, "User offset has changed");
  static_assert(offsetof(eeprom_layout, first_entry) == 256, "First_entry offset has changed");
  
  pinMode(RXD, INPUT_PULLUP);  //Pull up to avoid accidental interrupts
  Serial.begin(38400);

  Wire.begin();
  Wire.setClock(400000);

  rot.begin();

  if (!detect_device(eeprom_ext)) {
    Serial.println(F("Eeprom not found"));
    return;
  }

  #ifdef AIO438 //AIO4CH has no factory data
  if (!load_factory_data()) {
    return;
  }
  #endif

  pinMode(vreg_sleep, OUTPUT);
  pinMode(bt_enable, OUTPUT);
  pinMode(amp_1_pdn, OUTPUT);
  pinMode(amp_2_pdn, OUTPUT);
  pinMode(led_green, OUTPUT);
  pinMode(led_red, OUTPUT);

  #ifdef AIO438
    pinMode(expansion_en, OUTPUT);
    pinMode(bt_pio_19, OUTPUT);
    pinMode(bt_pio_20, OUTPUT);
    pinMode(bt_pio_21, OUTPUT);
    pinMode(bt_pio_22, OUTPUT);
  #endif

  #ifdef AIO4CH
    pinMode(bt_pio_0, OUTPUT);
    pinMode(bt_pio_1, OUTPUT);
    pinMode(bt_pio_8, OUTPUT);
  #endif

  if(load_system_variables() && user.default_on == 1) {
    enable_system();
  }

  full_functionality = true;
}

void loop() {
  while (system_enabled) {
    serial_monitor();
    power_monitor();
    if(full_functionality) {
      temperature_monitor();
      analog_gain_monitor();
      aux_level_monitor();
      rotary_monitor();
      tws_monitor();
      eq_monitor();
      bt_monitor();
    }
    wdt_reset(); //System will reset if loop hangs for more than 8 seconds
  }
  disable_system();
  /* System will be in powerdown until interrupt occurs */
  enable_system();
}

void enable_system() { //Enable or disable system todo: look at sequence (also i2s)
  while (digitalRead(rot_sw) == HIGH) {}; //Continue when rotary switch is released
  Serial.println(F("Powering on..."));
  digitalWrite(vreg_sleep, HIGH);         //Set buckconverter to normal operation

  #ifdef AIO438
    digitalWrite(expansion_en, HIGH);   //Enable power to expansion connector
  #endif
  
  digitalWrite(amp_1_pdn, HIGH);      //Enable amplifier 1
  digitalWrite(amp_2_pdn, HIGH);      //Enable amplifier 2
  delay(10);

  if (!detect_device(amp_1_addr)) {
    Serial.println(F("Amplifier 1 not found")); //todo: do something?
  }
  if (!detect_device(amp_2_addr)) {
    Serial.println(F("Amplifier 2 not found"));
  }

  vol_reduction = 0;
  
  eq_state = eq_state_old = digitalRead(eq_sw);
  eq_enabled = eq_enabled_old = digitalRead(eq_sw); //Enable if EQ switch is closed

  write_single_register(amp_2, 0x6A, 0b1011);  //Set ramp clock phase of amp_2 to 90 degrees (see 7.4.3.3.1 in datasheet)

  write_single_register(amp_dual, 0x76, 0b10000);     //Enable over-temperature auto recovery
  write_single_register(amp_dual, 0x77, 0b111);       //Enable Cycle-By-Cycle over-current protection

  load_eeprom();

  if (user.vol_start_enabled) {
    vol = user.vol_start; //Set vol if enabled
  }
  vol_old = -104;         //Set vol_old out of range to force set_vol to run
  set_vol();              //Set volume

  analog_gain_old = 32;   //Set analog_gain_old out of range to force analog_gain_monitor to run
  analog_gain_monitor();  //Set analog gain

  #ifdef AIO438
    send_pulse(bt_pio_19, 250); //Play startup tone
    #endif
    #ifdef AIO4CH
    send_pulse(bt_pio_8, 100); //Play startup tone
  #endif

  while (Serial.available() > 0) {
    Serial.read();  //Clear serial input buffer
  }
  Serial.println(F("On"));
  system_enabled = true;
}

void disable_system() {
  Serial.println(F("Powering off..."));
  set_led("OFF");

  digitalWrite(bt_enable, LOW);

  delay(1100);  //Wait for everything to power off

  digitalWrite(amp_1_pdn, LOW);
  digitalWrite(amp_2_pdn, LOW);

  #ifdef AIO438
  digitalWrite(expansion_en, LOW);
  #endif

  Serial.println(F("Off"));
  Serial.flush();

  digitalWrite(vreg_sleep, LOW);    //Set buckconverter to sleep

  PCattachInterrupt(RXD, exit_powerdown, CHANGE);       //Wake up from serial todo null pointer function?, update lib?
  PCattachInterrupt(rot_sw, exit_powerdown, CHANGE);  //Wake up from rotary switch
  wdt_disable();                    //Disable watchdog timer
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  /* System will be in powerdown until interrupt occurs */
  wdt_enable(WDTO_8S);              //Enable 8 seconds watchdog timer
  PCdetachInterrupt(RXD);      //Wake up from serial
  PCdetachInterrupt(rot_sw); //Wake up from rotary switch
}

void exit_powerdown() {} //Empty ISR to exit powerdown

void send_pulse(byte PIO, int duration) { //Send a pulse to a BT PIO to 'press' it (see GPIO mapping for more info) todo documentation
  digitalWrite(PIO, HIGH);
  duration = min(duration, 2000); //2000ms maximum duration to avoid hanging
  delay(duration);
  digitalWrite(PIO, LOW);
}

void set_led(String color) { //Set led color (todo enum?)
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

