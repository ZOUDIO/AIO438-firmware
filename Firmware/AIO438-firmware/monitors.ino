void serial_monitor() {
  if (Serial.available() > 0) {         //If data is available
    byte incoming_byte = Serial.read();  //Read data
    if (incoming_byte == start_marker) {
      currently_receiving = true;
      temp_buffer_count = 0;
    } else if (incoming_byte == end_marker) {
      binary_data_handler();
      currently_receiving = false;
      temp_buffer_count = 0;
    } else if (currently_receiving) {              //If currently receiving and byte is not a start or end marker
      temp_buffer[temp_buffer_count] = incoming_byte; //Put byte in buffer
      temp_buffer_count++;                         //Move to next byte
    } else if (incoming_byte == '\n') {           
      temp_buffer[temp_buffer_count++] = '\0';               //Terminate the string
      for (int i = 0; i < temp_buffer_count; i++) {       //Copy binary data to char array
        incoming_data._char[i] = toupper(temp_buffer[i]);  //Make uppercase
      }
      ascii_data_handler();
      temp_buffer_count = 0;
    } else if (incoming_byte > 31 && incoming_byte < 123)  {  //Only allow printable characters
      if (temp_buffer_count < array_size) {                    //Prevent overflow
        temp_buffer[temp_buffer_count] = incoming_byte;         //Put byte in buffer
        temp_buffer_count++;
      }
    }
  }
}

void temperature_monitor() { //Check every second if the amplifier temperature is too high and reduce volume accordingly  
  static unsigned long lastTemp;
  if ((millis() - lastTemp) > 1000) {
    write_single_register(amp_dual, 0x78, 0x80);  //Reset fault register
    byte temp_register = (read_register(amp_1, 0x73) | read_register(amp_2, 0x73)); //Take the highest temperature of both amplifiers
    //Register 0x73: bit 1 = 112*C, bit 2 = 122*C, bit 3 = 134 *C, bit 4 = 146 *C
    if (bitRead(temp_register, 3)) Serial.println(F("High temperature: 146 *C"));
    else if (bitRead(temp_register, 2)) Serial.println(F("High temperature: 134 *C"));
    else if (bitRead(temp_register, 1)) Serial.println(F("High temperature: 122 *C"));
    else if (bitRead(temp_register, 0)) Serial.println(F("High temperature: 112 *C"));

    if (bitRead(temp_register, 3)) { //If bit 3 is high, reduce volume by 1dB
      vol -= 1;
      vol_reduction += 1;
    } else if (bitRead(temp_register, 2)) { //If bit 2 is high, reduce volume by 0.5dB
      vol -= 0.5;
      vol_reduction += 0.5;
    } else if (vol_reduction) { //If volume was reduced, but temperature is now okay; increase by 0.5dB
      vol += 0.5;
      vol_reduction -= 0.5;
    } //todo: absolute?

    if (vol_reduction) {
      Serial.print(F("Volume currently reduced by "));
      Serial.print(vol_reduction);
      Serial.println(F(" dB"));
    }

    lastTemp = millis();
  }
}

void rotary_monitor() {  //Check if the rotary encoder has been turned or pressed
  byte rotDirection = rot.process();
  if (rotDirection == DIR_CW) {             //Clockwise: increase volume
    vol++;
  } else if (rotDirection == DIR_CCW) {     //Counter-clockwise: decrease volume
    vol--;
    vol_reduction = 0;                       //Do not try to recover the reduction anymore
  }
  set_vol();

  rot_button.Update(); //Implement actions
  if (rot_button.clicks == 1) {             //Short single press: turn off
    system_enabled = false;
  } else if (rot_button.clicks == 2) {      //Short double press: next track
    Serial.println(F("Next track"));
  } else if (rot_button.clicks == 3) {      //Short triple press: previous track
    Serial.println(F("Previous track"));
  } else if (rot_button.clicks == -1) {     //Long press: enter pairing
    Serial.println(F("Enter pairing"));
  } else if (rot_button.clicks == -2) {     //Double press and hold: reset pairing list
    Serial.println(F("Reset pairing list"));
  }
}

void power_monitor() { //Check system voltage. Update system state, status LED and analog gain accordingly
  static unsigned long shutdownTime; //If zero: no shutdown planned
  //Input voltage divider is 1M/100K (1/11th). ADC is 10 bit (1024 values) at 3.3V
  power_voltage = analogRead(vdd_sense) * 11 * 3.3 / 1024;
  if (power_voltage < user.power_shutdown) {
    if (!shutdownTime) {  //If shutdown is not planned: plan in 10 seconds
      shutdownTime = millis() + 10000;
    } else if (millis() > shutdownTime) {
      Serial.println(F("Power shutdown"));
      shutdownTime = 0;
      system_enabled = false;
    }
  } else if (power_voltage < user.power_low) {
    shutdownTime = 0;
    set_led("RED");
  } else {
    shutdownTime = 0;
    set_led("GREEN");
  }
}

void analog_gain_monitor() {
  analog_gain = map(power_voltage, 29.5, 4.95, 0, 31); //Calculate analog gain (29.5V = 0, 4.95V = 31)
  if (abs(analog_gain - analog_gain_old) > 1) { //Only update if difference is more than 1 to avoid rapid toggling
    analog_gain = max(analog_gain, 0);  //Set absolute minimum
    analog_gain = min(analog_gain, 31); //Set absolute maximum
    write_single_register(amp_dual, 0x54, analog_gain);
    analog_gain_old = analog_gain;
  }
}

void tws_monitor() { //Check TrueWirelessStereo button
  //Not implemented yet
}

void eq_monitor() { //Check EQ button
  eq_state = digitalRead(eq_sw);    //Read state
  if (eq_state != eq_state_old) {   //If state changed
    delay(1000);                    //Wait 1 second
    eq_state = digitalRead(eq_sw);  //Read state again
    if (eq_state != eq_state_old) { //If state is still changed (toggle switch)
      eq_enabled = eq_state;        //Set EQ based on switch position
    } else {                        //If state returns to old state (momentary switch)
      eq_enabled = !eq_enabled;     //Toggle state
    }
    eq_state_old = eq_state;
  }

  if (eq_enabled != eq_enabled_old) {
    Serial.println(F("EQ not implemented yet")); //todo: enable
    
    set_led("OFF", 500);            //Blink LED to notify user if EQ is on (green) or off (red)
    if (eq_enabled) {
      set_led("GREEN", 500);
    } else {
      set_led("RED", 500);
    }
    set_led("OFF", 500);  //Led will be enabled again by power_monitor()
    eq_enabled_old = eq_enabled;
  }
}

void aux_level_monitor() {
  //Todo: implement
}
