//Serial encoding constants
const byte special_marker = 253; //Value 253, 254 and 255 can be sent as 253 0, 253 1 and 253 2 respectively
const byte start_marker = 254;
const byte end_marker = 255;

void binary_data_handler() {
  decode_incoming_data();
  
  actual_data_count = incoming_data_count - 2; //Size excluding CRC16
  if (actual_data_count <= 0) { //If there is no actual data
    return; //Drop message
  }

  uint16_t incoming_crc = incoming_data._byte[incoming_data_count - 2] << 8 | incoming_data._byte[incoming_data_count - 1];
  uint16_t calculated_crc = crc16_CCITT(incoming_data._byte, actual_data_count);
  
  if (incoming_crc != calculated_crc) { //todo report back via serial
    bitSet(payload.function_code, 8); //Set MSB (indicates error)
    
    Serial.println("Invalid CRC"); //Todo remove serial printing
    Serial.print("Incoming CRC = ");
    Serial.println(incoming_crc, HEX);
    Serial.print("Calculated CRC = ");
    Serial.println(calculated_crc, HEX);
    return; //Drop message
  }

  binary_command_handler();
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

void prepare_outgoing_data() {
  if (payload.function_code == 1) { //Write bytes function
    outgoing_data_count = 4; //Function_code, address and amount
  } else if (payload.function_code == 2) { //Read bytes function
    outgoing_data_count = 4 + payload.with_addr.amount; //Function_code, address, amount and data bytes
  } else { //Function_code 3 and error_codes
    outgoing_data_count = 1; //Only function code
  }

  memcpy(&outgoing_data, &payload, outgoing_data_count);

  uint16_t crc = crc16_CCITT(outgoing_data, outgoing_data_count);
  outgoing_data_count += 2; //add crc bytes to count
  outgoing_data[outgoing_data_count - 2] = (crc >> 8) & 0xFF; //Fill CRC
  outgoing_data[outgoing_data_count - 1] = crc & 0xFF; //Fill CRC
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
  if (apply_settings_flag) {
    Serial.println(F("New settings applied"));
    load_eeprom(); //todo check
    apply_settings_flag = false;
  }
}
