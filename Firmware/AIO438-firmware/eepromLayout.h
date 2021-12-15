struct userData {
  uint32_t valid;             //1 = true, else = false
  uint8_t volSaveEnable;      //1 = true, else = false
  float volSave;              //Non-volatile place to remember volume level between power cycles
  float volStart;
  float volMax;
  float powerLow;
  float powerShutdown;
};

struct variables {
  
};
