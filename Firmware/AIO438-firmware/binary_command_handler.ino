void command_handler() {

  byte function_code = incoming_data._byte[0];

//  switch (function_code) {
//    case 1: //Write bytes
//      int addr = incoming_data._byte[1] << incoming_data._byte[2];
//      int amount = actual_data_count - 2; //minus addr bytes
//      write_eeprom(addr, amount, incoming_data._byte[3]);
//      break;
//    case 2: //Read bytes
//      //Similar to write, but than read (duhh)
//      break;
//    case 3: //Execute eeprom
//      executeEeprom = true; //Set flag, handled after sending reply to master
//      break;
//    default:
//      // Reply nAck
//      break;
//  }

  //Do something with it

}

//PPC byte blob loader
