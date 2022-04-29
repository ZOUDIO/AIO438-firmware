void binary_command_handler() {
  memcpy(&payload, incoming_data._byte, actual_data_count); //Copy incoming data into payload struct

  uint16_t addr = swap_int(payload.with_addr.address); //Convert address to little endian before using

  switch (payload.function_code) {
    case 1: //Write bytes
      write_eeprom_setup(addr, payload.with_addr.amount, payload.with_addr.data);
      break;
    case 2: //Read bytes
      read_eeprom(addr, payload.with_addr.amount);
      memcpy(&payload.with_addr.data, eeprom_buffer.as_bytes, payload.with_addr.amount); //Copy into payload for reply
      break;
    case 3: //Apply new settings
      apply_settings_flag = true; //Set flag, handled after sending reply to master
      break;
    default:
      bitSet(payload.function_code, 8); //Set MSB (indicates error)
      break;
  }
}
