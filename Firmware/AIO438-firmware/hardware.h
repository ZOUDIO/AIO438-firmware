//Pinout
#define BT_LED0       2   //PD2
#define BT_ENABLE     3   //PD3
#define AMP2_PDN      4   //PD4
#define LED_GREEN     5   //PD5
#define LED_RED       6   //PD6
#define ROT_A         7   //PD7
#define ROT_B         8   //PB0
#define BT_PIO22      9   //PB1
#define BT_PIO21      10  //PB2
#define EQ_SW         11  //PB3
#define ROT_SW        12  //PB4
#define TWS_SW        13  //PB5
#define AUX_SENSE     14  //PC0
#define EXPANSION_EN  15  //PC1
#define BT_LED1       16  //PC2
#define VDD_SENSE     17  //PC3
#define AMP1_PDN      23  //PE0
#define VREG_SLEEP    24  //PE1
#define BT_PIO20      25  //PE2
#define BT_PIO19      26  //PE3

//I2C addresses
#define AMP1          0x4D
#define AMP2          0x4C
#define EEPROM_EXT    0x50

//EEPROM properties
#define EEPROM_SIZE   32768
#define PAGE_SIZE     64
