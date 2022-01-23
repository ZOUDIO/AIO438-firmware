enum class entry_type_enum { //Determines how the entry data is handled
  system_variables,
  dsp_default,  //Always used
  dsp_eq_off,   //Only used when EQ is disabled
  dsp_eq_on,    //Only used when EQ is enabled
};

//struct { //Used during runtime to know which entry contains the eq dsp instructions
//  uint8_t dsp_eq_off_entry_index;
//  uint8_t dsp_eq_on_entry_index;
//} dsp_eq;

struct entry_struct {
  uint8_t type; //Check against entry_type_enum
  uint8_t amp; //0 = all amps
  uint8_t address_msb;
  uint8_t address_lsb;
  uint8_t size_msb;
  uint8_t size_lsb;
  uint8_t crc_msb;
  uint8_t crc_lsb;
  char name[32];
  uint8_t rfu[8]; //Pad to 48 bytes
};

struct allocation_table {
  entry_struct entry[32];
};

struct system_variables {
  uint8_t amp_1_enabled : 1;
  uint8_t amp_2_enabled : 1;
  uint8_t bt_enabled : 1;
  uint8_t vol_start_enabled : 1; //If false: remember volume through on/off cycles
  float vol_start;
  float vol_max;
  float power_low;
  float power_shutdown;
} user;

struct version_struct {
  uint8_t major;
  uint8_t minor;
  uint8_t patch;
};

enum class model_enum { //Product model name
  AIO4CH,
  AIO438,
};

struct factory_data_struct {
  uint16_t signature; //Should be 0x5555
  uint8_t model;      //Check against model_enum
  version_struct hw_version;
  version_struct bt_fw_version;
} factory_data;

struct eeprom_layout {
  factory_data_struct factory_data;
  uint8_t rfu[119]; //Pad to 128 bytes
  allocation_table table;
  //Remainder of eeprom is reserved for data storage
};
