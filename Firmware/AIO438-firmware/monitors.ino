void serialMonitor() {
  if (Serial.available() > 0) {         //If data is available
    byte incomingByte = Serial.read();  //Read data
    if (incomingByte == startMarker) {
      currentlyReceiving = true;
      tempBufferCount = 0;
    } else if (incomingByte == endMarker) {
      binaryDataHandler();
      currentlyReceiving = false;
      tempBufferCount = 0;
    } else if (currentlyReceiving) {              //If currently receiving and byte is not a start or end marker
      tempBuffer[tempBufferCount] = incomingByte; //Put byte in buffer
      tempBufferCount++;                         //Move to next byte
    } else if (incomingByte == '\n') {           
      tempBuffer[tempBufferCount] = '\0';               //Terminate the string
      for (int i = 0; i < tempBufferCount; i++) {       //Copy binary data to char array
        incomingData._char[i] = toupper(tempBuffer[i]);  //Make uppercase
      }
      asciiDataHandler();
      memset(incomingData._char, 0, arraySize); //Todo: should not be necessary, find out
      tempBufferCount = 0;
    } else if (incomingByte > 31 && incomingByte < 123)  {  //Only allow printable characters
      if (tempBufferCount < arraySize) {                    //Prevent overflow
        tempBuffer[tempBufferCount] = incomingByte;         //Put byte in buffer
        tempBufferCount++;
      }
    }
  }
}

void temperatureMonitor() { //Check every second if the amplifier temperature is too high and reduce volume accordingly  
  static unsigned long lastTemp;
  if ((millis() - lastTemp) > 1000) {
    writeRegisterDual(0x78, 0x80);  //Reset fault register
    byte tempRegister = (readRegister(AMP1, 0x73) | readRegister(AMP2, 0x73)); //Take the highest temperature of both amplifiers
    //Register 0x73: bit 1 = 112*C, bit 2 = 122*C, bit 3 = 134 *C, bit 4 = 146 *C
    if (bitRead(tempRegister, 3)) Serial.println(F("High temperature: 146 *C"));
    else if (bitRead(tempRegister, 2)) Serial.println(F("High temperature: 134 *C"));
    else if (bitRead(tempRegister, 1)) Serial.println(F("High temperature: 122 *C"));
    else if (bitRead(tempRegister, 0)) Serial.println(F("High temperature: 112 *C"));

    if (bitRead(tempRegister, 3)) { //If bit 3 is high, reduce volume by 1dB
      vol -= 1;
      volReduction += 1;
    } else if (bitRead(tempRegister, 2)) { //If bit 2 is high, reduce volume by 0.5dB
      vol -= 0.5;
      volReduction += 0.5;
    } else if (volReduction) { //If volume was reduced, but temperature is now okay; increase by 0.5dB
      vol += 0.5;
      volReduction -= 0.5;
    } //todo: absolute?

    if (volReduction) {
      Serial.print(F("Volume currently reduced by "));
      Serial.print(volReduction);
      Serial.println(F(" dB"));
    }

    lastTemp = millis();
  }
}

void rotaryMonitor() {  //Check if the rotary encoder has been turned or pressed
  byte rotDirection = ROT.process();
  if (rotDirection == DIR_CW) {             //Clockwise: increase volume
    vol++;
  } else if (rotDirection == DIR_CCW) {     //Counter-clockwise: decrease volume
    vol--;
    volReduction = 0;                       //Do not try to recover the reduction anymore
  }
  setVol();

  ROT_button.Update();
  if (ROT_button.clicks == 1) {             //Short single press: turn off
    systemEnable = false;
  } else if (ROT_button.clicks == 2) {      //Short double press: next track
    Serial.println(F("Next track"));
    //Implement
  } else if (ROT_button.clicks == 3) {      //Short triple press: previous track
    Serial.println(F("Previous track"));
    //Implement
  } else if (ROT_button.clicks == -1) {     //Long press: enter pairing
    Serial.println(F("Enter pairing"));
    //Implement
  } else if (ROT_button.clicks == -2) {     //Double press and hold: reset pairing list
    Serial.println(F("Reset pairing list"));
    //Implement
  }
}

void powerMonitor() { //Check system voltage. Update system state, status LED and analog gain accordingly
  static unsigned long shutdownTime; //If zero: no shutdown planned
  //Input voltage divider is 1M/100K (1/11th). ADC is 10 bit (1024 values) at 3.3V
  powerVoltage = analogRead(VDD_SENSE) * 11 * 3.3 / 1024;
  if (powerVoltage < user.powerShutdown) {
    if (!shutdownTime) {  //If shutdown is not planned: plan in 10 seconds
      shutdownTime = millis() + 10000;
    } else if (millis() > shutdownTime) {
      Serial.println(F("Power shutdown"));
      shutdownTime = 0;
      systemEnable = false;
    }
  } else if (powerVoltage < user.powerLow) {
    shutdownTime = 0;
    setLed("RED");
  } else {
    shutdownTime = 0;
    setLed("GREEN");
  }
}

void analogGainMonitor() {
  analogGain = map(powerVoltage, 29.5, 4.95, 0, 31); //Calculate analog gain (29.5V = 0, 4.95V = 31)
  if (abs(analogGain - analogGainOld) > 2) { //Only update if difference is more than 2 to avoid rapid toggling
    analogGain = max(analogGain, 0);  //Set absolute minimum
    analogGain = min(analogGain, 31); //Set absolute maximum
    writeRegisterDual(0x54, analogGain);
    analogGainOld = analogGain;
  }
}

void twsMonitor() { //Check TrueWirelessStereo button
  //Not implemented yet
}

void eqMonitor() { //Check EQ button
  eqState = digitalRead(EQ_SW);
  if (eqState != eqStateOld) {      //If state changed
    delay(1000);                    //Wait 1 second
    eqState = digitalRead(EQ_SW);   //Read state again
    if (eqState != eqStateOld) {    //If state is still changed (toggle switch)
      eqEnable = eqState;           //Set EQ based on switch position
    } else {                        //If state returns to old state (momentary switch)
      eqEnable = !eqEnable;         //Toggle state
    }
    eqStateOld = eqState;
  }

  if (eqEnable != eqEnableOld) {
    //Todo: implement
    Serial.println(F("EQ not available")); //Todo: enable
    
    setLed("OFF", 500);            //Blink LED to notify user if EQ is on (green) or off (red)
    if (eqEnable) {
      setLed("GREEN", 500);
    } else {
      setLed("RED", 500);
    }
    setLed("OFF", 500);
    eqEnableOld = eqEnable;
  }
}
