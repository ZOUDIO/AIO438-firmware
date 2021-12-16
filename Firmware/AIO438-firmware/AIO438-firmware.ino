/*
  ZOUDIO AIO438 firmware
  For questions mail "info@zoudio.com" or visit "www.zoudio.com"

  Microcontroller: ATMEGA328PB at 3.3V with 8MHz external crystal
  Bootloader: https://github.com/MCUdude/MiniCore with BOD set to 1.8V
    Note: V2.0.4 or higher needs a modified upload speed to work
  Recommended programmer to burn bootloader: USBasp
  Board has onboard CP2102N for communication and firmware uploads via UART
*/

#include "src/ArduinoLibraryWire/Wire.h" //Modified Wire.h with 66 byte buffer to send 64 byte arrays with 2 byte address
#include "PinChangeInt.h"     //Pin Change Interrupt library from https://github.com/GreyGnome/PinChangeInt
#include "ClickButton.h"      //Button press detection library from https://github.com/marcobrianza/ClickButton
#include "Rotary.h"           //Rotary encoder library from https://github.com/brianlow/Rotary
#include "CRC.h"              //Cyclic-Redundancy-Check library from https://github.com/RobTillaart/CRC        
#include "CRC32.h"            //Extends CRC.h
#include "LowPower.h"         //Low power library from https://github.com/canique/Low-Power-Canique/
#include "power2.h"           //Extends LowPower.h
#include "avr/wdt.h"          //Watchdog timer library
#include "hardware.h"         //Hardware determined constants
#include "eepromLayout.h"     //Struct definitions for eeprom storage

#define member_size(type, member) sizeof(((type *)0)->member)                                                                 //Get struct member size in bytes
#define eepromPut(member) writeStructToEeprom(offsetof(variables, member), member_size(variables, member))  //Save struct member on eeprom
#define eepromGet(member) readEeprom(offsetof(variables, member), member_size(variables, member))           //Get struct member from eeprom

const char MODEL[] = "AIO438";
const char FIRMWARE[] = "0.1.0";

const bool _verbose = true; //Can be passed to functions
const bool _non_verbose = false; //Can be passed to functions
const byte arraySize = 67;  //64 bytes plus group ID, command ID and CRC8

//Serial encoding constants
const byte specialMarker = 253; //Value 253, 254 and 255 can be sent as 253 0, 253 1 and 253 2 respectively
const byte startMarker = 254;
const byte endMarker = 255;

//PurePathConsole constants
const byte CFG_META_BURST = 253;
const byte CFG_META_DELAY = 254;

typedef struct {
  uint8_t command;
  uint8_t param;
} cfg_reg;

enum { //To give feedback to master
  ACK,
  INVALID_CRC,
  INVALID_SIZE,
  UNKNOWN_COMMAND
} faultCode;

union { //Used for type conversion
  byte asBytes[64];
  float asFloat;
  userData asUserData;
  cfg_reg asCfg_reg;
} eepromBuffer;

union {
  byte _byte[arraySize]; //Get as byte array
  char _char[arraySize]; //Get as char array
} incomingData;

byte tempBuffer[2 * arraySize];     //Worst case scenario every value is 253 or higher, which needs two bytes to recreate
byte outgoingData[arraySize];       //Data before encoding

byte incomingDataCount;
byte outgoingDataCount;
byte tempBufferCount;
char actualDataCount; //Can be negative
bool currentlyReceiving;

bool eqEnable;
bool eqEnableOld;
bool eqState;
bool eqStateOld;
bool systemEnable;
float vol = -30; //Todo: remove hardcoding
float volOld;
float volReduction;
float powerVoltage;
char analogGain;
char analogGainOld;

//Structs with variables that can be saved on the eeprom
userData user;

ClickButton ROT_button(ROT_SW, HIGH); //Encoder switch
ClickButton TWS_button(TWS_SW, HIGH); //TrueWirelessStereo button
Rotary ROT = Rotary(ROT_A, ROT_B);    //Encoder
CRC32 crc_32;                         //Used in blob validation

void setup() {
  pinMode(EXPANSION_EN, OUTPUT); //Todo: determine state and behaviour
  pinMode(VREG_SLEEP, OUTPUT);
  pinMode(BT_ENABLE, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(AMP1_PDN, OUTPUT);
  pinMode(AMP2_PDN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BT_PIO19, OUTPUT);
  pinMode(BT_PIO20, OUTPUT);
  pinMode(BT_PIO21, OUTPUT);
  pinMode(BT_PIO22, OUTPUT);
  //Remainder of GPIO are default pinmode (input)
  pinMode(0, INPUT_PULLUP); //Pull up the RX pin to avoid accidental interrupts
  Serial.begin(38400);
  Wire.begin();
  Wire.setClock(400000);
  ROT.begin();
  attachPinChangeInterrupt(0, exitPowerdown, CHANGE);       //Wake up from serial
  attachPinChangeInterrupt(ROT_SW, exitPowerdown, CHANGE);  //Wake up from rotary switch

  //CRC32 settings match the defaults of java.util.zip.CRC32 used in the configtool
  crc_32.setStartXOR(0xFFFFFFFF);
  crc_32.setEndXOR(0xFFFFFFFF);
  crc_32.setReverseIn(true);
  crc_32.setReverseOut(true);

  //  static_assert(sizeof(variables) < EEPROM_SIZE, "Struct too large for EEPROM");
}

void exitPowerdown() {
  //Empty ISR to exit powerdown
}

void loop() {
  disableSystem();
  /*System will be in powerdown until interrupt occurs*/
  enableSystem();
  while (systemEnable) {
    serialMonitor();
    if (true) { //todo: Check valid state
      temperatureMonitor();
      analogGainMonitor();
      rotaryMonitor();
      powerMonitor();
      twsMonitor();
      eqMonitor();
    }
    wdt_reset(); //System will reset if loop hangs for more than 8 seconds
  }
}

void enableSystem() { //Enable or disable system
  while (digitalRead(ROT_SW) == HIGH) {}; //Continue when rotary switch is released
  Serial.println("Booting...");
  digitalWrite(VREG_SLEEP, HIGH);         //Set buckconverter to normal operation
  digitalWrite(BT_ENABLE, HIGH);          //Enable BT module
  digitalWrite(AMP1_PDN, HIGH);           //Enable amplifier 1
  digitalWrite(AMP2_PDN, HIGH);           //Enable amplifier 1
  delay(10);                              //Wait for amplifiers to boot

  volOld = 0;                                   //Default: 0dB todo: force?
  analogGain = analogGainOld = 0;               //Default: 0dB
  eqState = eqStateOld = digitalRead(EQ_SW);    //Enable if EQ switch is closed todo verify
  eqEnable = eqEnableOld = digitalRead(EQ_SW);  //Enable if EQ switch is closed

  //Check eeprom validity

  if (true) { //Todo: Check validation state
    writeRegister(AMP2, 0x6A, B1011);       //Set ramp clock phase of AMP2 to 90 degrees (see 7.4.3.3.1 in datasheet)
    delay(750);                             //Wait for I2S to start

    //Load eeprom

    //Overwrite these settings
    writeRegisterDual(0x76, B10000);        //Enable over-temperature auto recovery
    writeRegisterDual(0x77, B111);          //Enable Cycle-By-Cycle over-current protection
    writeRegisterDual(0x53, B1100000);      //Set Class D bandwith to 175Khz
    writeRegisterDual(0x51, B1110111);      //Set automute time to 10.66 seconds

    //Restore non-volatile volume if enabled
    setVol();                               //Set actual volume
    analogGainMonitor();                    //Set analog gain

    //Play startup tone

    //Set output states based on eeprom todo
    writeRegister(AMP1, 0x03, B11);
    writeRegister(AMP2, 0x03, B11);
  }

  while (Serial.available() > 0) {        //Clear serial input buffer
    Serial.read();
  }
  Serial.println(F("On"));
  systemEnable = true;
}

void disableSystem() {
  //Save non-volatile volume
  setLed("OFF", 0);
  Serial.println(F("Off"));
  Serial.flush();
  digitalWrite(BT_ENABLE, LOW);
  digitalWrite(AMP1_PDN, LOW);
  digitalWrite(AMP2_PDN, LOW);
  delay(50);                              //Wait for everything to power off
  digitalWrite(VREG_SLEEP, LOW);          //Set buckconverter to sleep

  wdt_disable();                          //Disable watchdog timer
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  /*   System will be in powerdown until interrupt occurs   */
  wdt_enable(WDTO_8S);                    //Enable 8 seconds watchdog timer todo enable
}

void sendPulse(byte PIO, int duration) { //Send a pulse to a BT PIO to 'press' it (see GPIO mapping for more info)
  digitalWrite(PIO, HIGH);
  duration = min(duration, 2000); //2000ms maximum duration to avoid hanging
  delay(duration);
  digitalWrite(PIO, LOW);
}

void setLed(String color, int _delay) { //Set led color
  if (color == "RED") {
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, LOW);
  } else if (color == "GREEN") {
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, LOW);
  } else { //Off
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);
  }

  if (_delay) {
    delay(_delay);
  }
}

void setVol() {
  if (vol != volOld) {
    vol = max(vol, -103.5);       //Set absolute minimum
    vol = min(vol, user.volMax);  //Set user-defined maximum
    vol = min(vol, 24);           //Set absolute maximum
    byte volRegister = 48 - 2 * vol;   //Convert decibel to register value
    writeRegisterDual(0x4C, volRegister);   //Write to amps
    volOld = vol;
  }
}
