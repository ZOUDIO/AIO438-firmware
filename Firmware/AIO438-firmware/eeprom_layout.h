enum class entry_type { //Determines how the entry data is handled
  system_variables, 
  dsp_default,  //Always used
  dsp_eq_off,   //Only used when EQ is disabled
  dsp_eq_on,    //Only used when EQ is enabled
  empty = 255,  //255 is the default value of an eeprom byte
};

struct entry_struct {
  entry_type type;
  uint16_t location;
  uint16_t size; 
  uint16_t crc;
  uint8_t amp; //0 = all amps
};

struct allocation_table {
  entry_struct entry[32];
};

struct system_variables {
  uint8_t vol_save_enable;
  float vol_save;        //Non-volatile place to remember volume level between power cycles
  float vol_start;
  float vol_max;
  float power_low;
  float power_shutdown;
};

struct version_struct{
  byte major;
  byte minor;
  byte patch;
};

struct factory_data {
  version_struct hw_version;
  version_struct bt_version;
  uint8_t rfu[122];
};

struct eepromLayout {
  factory_data factory;
  allocation_table table;
  //Remainder of eeprom is reserved for data storage
};

struct variables {
  //Currently empty, but needed
};
