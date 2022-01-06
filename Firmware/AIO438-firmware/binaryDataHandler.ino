//This file is mostly broken now, todo: fix

void binaryDataHandler() { //Once data is received, it will be handled in the following order
  decodeincomingData();
  checkData();
  prepareOutgoingData();
  encodeOutgoingData();
  sendData();
  applySettings();
}

void decodeincomingData() {
  incomingDataCount = 0;                        //Keeps track of how many data is received
  for (int i = 0; i < tempBufferCount; i++) {   //Go through whole tempBuffer array
    byte x = tempBuffer[i];                     //Load byte
    if (x == specialMarker) {                   //If byte is special byte
      i++;                                      //Look at next byte
      x = x + tempBuffer[i];                    //Actual value is (special byte + next byte) (e.g. 253 1 equals 254)
    }
    incomingData._byte[incomingDataCount] = x;  //Put actual byte value into incomingData array
    incomingDataCount ++;                       //Increment data received array index
  }
}

void checkData() {
  faultCode = ACK; //ACK by default

  actualDataCount = incomingDataCount - 3; //Size is total bytes - group ID - command ID - CRC8

  if (actualDataCount < 0) {
    faultCode = INVALID_SIZE;
    return;
  }

  if(crc8(incomingData._byte, incomingDataCount - 1, 0x7)) { //If total CRC is not zero, message is corrupt
    faultCode = INVALID_CRC;
    return;
  }

  if (actualDataCount == 0) {
    //Read back, not implemented yet
  }

  commandHandler();
}

void prepareOutgoingData() {
  outgoingDataCount = 4;
  outgoingData[0] = 0;
  outgoingData[1] = 0;
  outgoingData[2] = faultCode;
  outgoingData[3] = crc8(outgoingData, outgoingDataCount - 1, 0x7);
}

void encodeOutgoingData() { //Outgoing data
  tempBufferCount = 0;                                                //Iterate through the whole array
  for (byte i = 0; i < outgoingDataCount; i++) {                      //Go through whole outgoingData array
    if (outgoingData[i] >= specialMarker) {                           //If data is 253, 254 or 255
      tempBuffer[tempBufferCount] = specialMarker;                    //Set current byte to 253
      tempBufferCount++;                                              //Go to next byte
      tempBuffer[tempBufferCount] = outgoingData[i] - specialMarker;  //Set value to (value - specialMarker)
    } else {                                                          //Else:
      tempBuffer[tempBufferCount] = outgoingData[i];                  //Add data to buffer directly
    }
    tempBufferCount++;
  }
}

void sendData() {
  Serial.write(startMarker);
  Serial.write(tempBuffer, tempBufferCount);
  Serial.write(endMarker);
}

void applySettings() {
  if(loadEeprom) {
    Serial.println(F("New settings applied"));
    loadEeprom = false;
  } 
}
