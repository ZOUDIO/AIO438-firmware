### Low-Power
Lightweight low power library for Arduino with Atmega328PB support.
This is a fork of the <a href="https://github.com/rocketscream/Low-Power">official Low-Power library from rocketscream version 1.60</a> with added Atmega328PB support.<br/>
It is maintained by <a href="https://www.canique.com">canique.com</a> and used for its MK20 Arduino board with Atmega328PB.

Version: 1.70

Devices Supported:
* ATMega168
* ATMega328P
* ATMega328PB
* ATMega32U4
* ATMega2560
* ATMega256RFR2
* ATSAMD21G18A

####Notes:
External interrupt during standby on ATSAMD21G18A requires a patch to the <a href="https://github.com/arduino/ArduinoCore-samd">Arduino SAMD Core</a> in order for it to work. Fix is provided by this particular <a href="https://github.com/arduino/ArduinoCore-samd/pull/90">pull request</a>.
