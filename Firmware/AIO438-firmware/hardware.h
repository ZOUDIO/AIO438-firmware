//Pinout, todo const uint?
#define bt_led_0      2   //PD2
#define bt_enable     3   //PD3
#define amp_2_pdn     4   //PD4
#define led_green     5   //PD5
#define led_red       6   //PD6
#define rot_a         7   //PD7
#define rot_b         8   //PB0
#define bt_pio_22     9   //PB1
#define bt_pio_21     10  //PB2
#define eq_sw         11  //PB3
#define rot_sw        12  //PB4
#define tws_sw        13  //PB5
#define aux_sense     14  //PC0
#define expansion_en  15  //PC1
#define bt_led_1      16  //PC2
#define vdd_sense     17  //PC3
#define amp_1_pdn     23  //PE0
#define vreg_sleep    24  //PE1
#define bt_pio_20     25  //PE2
#define bt_pio_19     26  //PE3

//I2C addresses
#define amp_1_addr    0x4D
#define amp_2_addr    0x4C
#define eeprom_ext    0x50

//Eeprom properties
#define eeprom_size   32768
#define page_size     64
