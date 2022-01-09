void binary_command_handler() {

  byte function_code = incoming_data._byte[0];

  int addr, amount; //Used in switch-case
  switch (function_code) {
    case 1: //Write bytes
      addr = (incoming_data._byte[1] << 8) + incoming_data._byte[2];
      amount = actual_data_count - 2; //minus addr bytes todo: check or remove
      write_eeprom_setup(addr, amount, incoming_data._byte + 2);
      break;
    case 2: //Read bytes
      //Similar to write, but than read (duhh)
      break;
    case 3: //Apply new settings
      apply_settings_flag = true; //Set flag, handled after sending reply to master
      break;
    default:
      fault_code = illegal_function; //todo
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
