//Serial encoding constants
const byte special_marker = 253; //Value 253, 254 and 255 can be sent as 253 0, 253 1 and 253 2 respectively
const byte start_marker = 254;
const byte end_marker = 255;

void binary_data_handler() { //Once data is received, it will be handled in the following order
  decode_incoming_data();
  check_data();
  prepare_outgoing_data();
  encode_outgoing_data();
  send_data();
  apply_settings();
}

void decode_incoming_data() {
  incoming_data_count = 0;                        //Keeps track of how many data is received
  for (int i = 0; i < temp_buffer_count; i++) {   //Go through whole temp_buffer array
    byte x = temp_buffer[i];                     //Load byte
    if (x == special_marker) {                   //If byte is special byte
      i++;                                      //Look at next byte
      x = x + temp_buffer[i];                    //Actual value is (special byte + next byte) (e.g. 253 1 equals 254)
    }
    incoming_data._byte[incoming_data_count] = x;  //Put actual byte value into incoming_data array
    incoming_data_count ++;                       //Increment data received array index
  }
}

void check_data() {
  //fault_code = ack; //ACK by default

  actual_data_count = incoming_data_count - 3; //Size is total bytes - function code - CRC16

  if (actual_data_count < 0) {
    //fault_code = invalid_size;
    return;
  }

  if(crc16(incoming_data._byte, incoming_data_count, 0x1021)) { //If total CRC is not zero, message is corrupt
    //fault_code = invalid_crc;
    return;
  }

  binary_command_handler();
}

void prepare_outgoing_data() {
  outgoing_data_count = 4; //todo: construct actual reply
  outgoing_data[0] = 0;
  outgoing_data[1] = 0;
  outgoing_data[2] = fault_code;
  uint16_t crc = crc16(incoming_data._byte, incoming_data_count - 2, 0x1021);
  outgoing_data[3] = crc << 8;
  outgoing_data[4] = crc & 0xFF;
}

void encode_outgoing_data() { //Outgoing data
  temp_buffer_count = 0;                                                //Iterate through the whole array
  for (byte i = 0; i < outgoing_data_count; i++) {                      //Go through whole outgoing_data array
    if (outgoing_data[i] >= special_marker) {                           //If data is 253, 254 or 255
      temp_buffer[temp_buffer_count] = special_marker;                    //Set current byte to 253
      temp_buffer_count++;                                              //Go to next byte
      temp_buffer[temp_buffer_count] = outgoing_data[i] - special_marker;  //Set value to (value - special_marker)
    } else {                                                          //Else:
      temp_buffer[temp_buffer_count] = outgoing_data[i];                  //Add data to buffer directly
    }
    temp_buffer_count++;
  }
}

void send_data() {
  Serial.write(start_marker);
  Serial.write(temp_buffer, temp_buffer_count);
  Serial.write(end_marker);
}

void apply_settings() {
  if(apply_settings_flag) {
    Serial.println(F("New settings applied"));
    //todo: actually load new settings
    apply_settings_flag = false;
  } 
}
