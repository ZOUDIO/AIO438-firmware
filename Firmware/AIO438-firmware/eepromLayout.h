enum entryType {
  empty,
  factoryEntry,
  userEntry,
  dspEntry
};

struct entry {
  entryType type;
  uint16_t offset;
  uint16_t size;
  uint16_t crc;
  uint8_t amp; //0 = all
  uint8_t rfu[4];
};

struct versionStruct {
  byte major;
  byte minor;
  byte patch;
};

struct factoryDataStruct {
  versionStruct hwVersion;
  versionStruct btVersion;
};

struct userData {
  float volStart;
  float volMax;
  float powerLow;
  float powerShutdown;
};

struct variables { //fileAllocationTable
  entry file[32];
};
