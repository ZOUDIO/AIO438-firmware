enum class entry_type_enum { //Determines how the entry data is handled
  system_variables, 
  dsp_default,  //Always used
  dsp_eq_off,   //Only used when EQ is disabled
  dsp_eq_on,    //Only used when EQ is enabled
  empty = 255,  //Default value of an eeprom byte
};

struct dsp_eq_struct { //Used during runtime to know which entry contains the eq dsp instructions
  uint8_t dsp_eq_off_entry_index;
  uint8_t dsp_eq_on_entry_index;
};

struct entry_struct {
  entry_type_enum type;
  uint16_t location;
  uint16_t size; 
  uint16_t crc;
  uint8_t amp; //0 = all amps
  uint8_t rfu[7];
};

struct allocation_table {
  entry_struct entry[32];
};

struct system_variables {
  uint8_t amp_1_enable;
  uint8_t amp_2_enable;
  uint8_t vol_save_enable; //todo: implement
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

enum class model_enum { //Product model name
  AIO4CH,
  AIO438,
  empty = 255,          //Default value of an eeprom byte
};

struct factory_data_struct {
  uint16_t signature;         //Should be 0x5555
  model_enum model;             
  version_struct hw_version;
  version_struct bt_fw_version;
  uint8_t rfu[55];
};

struct eeprom_layout {
  factory_data_struct factory_data;
  allocation_table table;
  //Remainder of eeprom is reserved for data storage
};

struct variables {
  //Currently empty, but needed
};
