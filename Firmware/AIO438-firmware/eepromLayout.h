struct userData {
  float volSave;        //Non-volatile place to remember volume level between power cycles
  float volStart;
  float volMax;
  float powerLow;
  float powerShutdown;
};

struct variables {
  //Currently empty, but needed
};

struct versionStruct{
  byte major;
  byte minor;
  byte patch;
};

struct atmegaEeprom {
  versionStruct hwVersion;
};
