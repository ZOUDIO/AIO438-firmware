enum class entryType { //Determines how the entry data is handled
  system_variables, 
  dsp_default,  //Always used
  dsp_eq_off,   //Only used when EQ is disabled
  dsp_eq_on,    //Only used when EQ is enabled
  empty = 255,  //255 is the default value of an eeprom byte
};

struct entryStruct {
  entryType type;
  uint16_t location;
  uint16_t size; 
  uint16_t crc;
  uint8_t amp; //0 = all amps
};

struct allocationTable {
  entryStruct entry[32];
};

struct system_variables {
  uint8_t volSaveEnable;
  float volSave;        //Non-volatile place to remember volume level between power cycles
  float volStart;
  float volMax;
  float powerLow;
  float powerShutdown;
};

struct versionStruct{
  byte major;
  byte minor;
  byte patch;
};

struct factoryData {
  versionStruct hwVersion;
  versionStruct btVersion;
  uint8_t rfu[122];
};

struct eepromLayout {
  factoryData factory;
  allocationTable table;
  //Remainder of eeprom is reserved for data storage
};

struct variables {
  //Currently empty, but needed
};
