enum class entry_type_enum { //Determines how the entry data is handled
  dsp_default,  //Always used
  dsp_eq_off,   //Only used when EQ is disabled
  dsp_eq_on,    //Only used when EQ is enabled
};

struct entry_struct {
  uint16_t crc;
  uint16_t size;
  uint8_t type; //Check against entry_type_enum
  uint8_t amp; //0 = all amps
  char name[48];
  uint8_t rfu[10]; //Pad to 64 bytes
};

enum class amp_output_state {
  dual,
  single,
  disable,
};

static const char *amp_output_state_str[] = {"dual", "single", "disable"};

struct system_variables {
  uint16_t signature; //Should be 0x5555
  uint8_t amp_1_enabled : 1; //Legacy, ignored since 2.0.0
  uint8_t amp_2_enabled : 1;  //Legacy, ignored since 2.0.0
  uint8_t bt_enabled : 1;
  uint8_t vol_start_enabled : 1; //If false: remember volume through on/off cycles
  uint8_t rfu_bits : 4;
  float vol_start;
  float vol_max;
  float power_low;
  float power_shutdown;
  uint8_t amp_1_output;
  uint8_t amp_2_output;
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
  uint8_t rfu_1[119]; //Pad to 128 bytes

  system_variables user;
  uint8_t rfu_2[107]; //Pad to 128 bytes

  entry_struct first_entry;
  //Remainder of eeprom is reserved for dsp settings
};
