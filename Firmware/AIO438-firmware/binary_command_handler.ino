void binary_command_handler() {

  byte function_code = incoming_data._byte[0];

  uint16_t reg, amount; //Used in switch-case
  switch (function_code) {
    case 1: //Write bytes
      reg = (incoming_data._byte[1] << 8) + incoming_data._byte[2];
      amount = actual_data_count - 2; //minus addr bytes todo: check or remove
      write_eeprom_setup(reg, amount, incoming_data._byte + 2);
      break;
    case 2: //Read bytes
      reg = (incoming_data._byte[1] << 8) + incoming_data._byte[2];
      amount = actual_data_count - 2; //minus addr bytes todo: check or remove
      read_eeprom(reg, amount);
      memcpy(outgoing_data + 3, eeprom_buffer.as_bytes, amount);
      break;
    case 3: //Apply new settings
      apply_settings_flag = true; //Set flag, handled after sending reply to master
      break;
    default:
      //fault_code = illegal_function; //todo
      break;
  }//look at extra modbus options function codes
}

enum { //To give feedback to master todo: revise
  acknowledge,            //Success
  negative_acknowledge,   //Unspecified error
  illegal_function,
  illegal_data_address,
  illegal_data_value,
} fault_code;

struct test {
  uint8_t function_code;
  union payload {
    struct with_addr {
      uint16_t addr;
      uint8_t data[64];
    };
    struct without_addr {
      uint8_t data[66];
    };
  };
  union {
    uint16_t crc;
    uint8_t crc_bytes[2];
  };
} incoming_data_str;
