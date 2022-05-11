# Changes made to make this repository compatible with Platform IO

## 1. Seperate source (.cpp) and header (.h) files
  * Header file are in **include** folder; source files are in **src** folder
  * Each source file has it's own header file.
  * Most of the global variables are defined in main.h

## 2. Add different library for **PinChangeInt**.
  * The reason to do this is that https://github.com/GreyGnome/PinChangeInt library has implemented it's methods in the header file itself, hence it does not comply with Platform IO requirements.
  * The new PinChangeInt library is https://github.com/asheeshr/PinChangeInt.git.
  * The following functions have been changed in main.cpp to match with new PinChangeInt library  
      `attachPinChangeInterrupt` => `PCattachInterrupt`  
      `detachPinChangeInterrupt` => `PCdetachInterrupt`  

## 3. Moved all custom libraries to **lib** folder
  * **ArduinoLibraryWire** has been moved to lib folder

## 4. *Minor change*

```C++
union {
  byte _byte[sizeof(payload)+2]; //Get as byte array
  char _char[sizeof(payload)+2]; //Get as char array
} incoming_data; //todo, with reinterpret_cast?, look at temp, incoming, outgoing etc shared memory space

```
the buffer size of `_byte` and `_char` was depending on the variable `array_size`. Since the definition of `array_size` is in another file, size of these two buffers had to be defined from scratch.
