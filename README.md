# ZOUDIO AIO438-firmware

4x38W amplifier with DSP and BT

- Microcontroller: ATmega328PB
- Voltage: 3.3V
- Crystal: 8MHz (external)
- Brown-out detection level: 1.8V
- Bootloader: MiniCore (https://github.com/MCUdude/MiniCore)

USBASP is the recommended programmer to burn the bootloader
Board uses CP2102N for communication and firmware uploads via UART.
For the AIO4CH this was an external board.
The AIO4CH has this chip built-in.

This source code uses build flags to compile software for the AIO4CH and AIO438.
The main difference between the two boards is pinmapping and the bluetooth module used.

For questions mail "info@zoudio.com" or visit "www.zoudio.com"
