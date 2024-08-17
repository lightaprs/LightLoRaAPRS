
//#define OLED_SDA_PIN 3
//#define OLED_SCL_PIN 4
#define OLED_RST_PIN -1
#define SCREEN_WIDTH    128     // OLED display width, in pixels
#define SCREEN_HEIGHT   64      // OLED display height, in pixels
#define SCREEN_ADDRESS  0x3C    ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

#define BUTTON_PIN 0
#define LED_PIN 16
#define VOLTAGE_READ_PIN 1
#define ADC_MULTIPLIER 5.65
#define ADC_REFERENCE REF_3V3

//For Solar Models Only
#define SOLAR_CHARGE_DISABLE_PIN 48
#define SOLAR_VOLTAGE_READ_PIN 7
#define SOLAR_ADC_MULTIPLIER 10.33

#define GPS_SERIAL_NUM 1
#define GPS_BAUDRATE 9600
#define GPS_RX_PIN 18
#define GPS_TX_PIN 17
#define GPS_VCC_PIN 33

#define USE_SX1268 //For EBYTE E22-400M30S
//#define USE_SX1262

#define SX126X_MAX_POWER 22 //22 is the max value for SX126X modules but E22-400M30S amplifies this to 30dBm. So do not change this.

#define LORA_VCC_PIN 21
#define LORA_RX_PIN 42
#define LORA_TX_PIN 14
#define LORA_SCK_PIN 12
#define LORA_MISO_PIN 13
#define LORA_MOSI_PIN 11
#define LORA_CS_PIN 10
#define LORA_RST_PIN 9
#define LORA_IRQ_PIN 5
#define LORA_BUSY_PIN 6
