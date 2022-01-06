void commandHandler() {

  byte functionCode = incomingData._byte[0];

//  switch (functionCode) {
//    case 1: //Write bytes
//      int addr = incomingData._byte[1] << incomingData._byte[2];
//      int amount = actualDataCount - 2; //minus addr bytes
//      writeEeprom(addr, amount, incomingData._byte[3]);
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
