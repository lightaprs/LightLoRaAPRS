#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>

#define USB_VID 0x303A
#define USB_PID 0x81E0
#define USB_MANUFACTURER "QRP Labs"
#define USB_PRODUCT "LightTracker Plus"
#define USB_SERIAL ""

#define EXTERNAL_NUM_INTERRUPTS 45
#define NUM_DIGITAL_PINS        26
#define NUM_ANALOG_INPUTS       2

#define analogInputToDigitalPin(p)  (((p)<20)?(analogChannelToDigitalPin(p)):-1)
#define digitalPinToInterrupt(p)    (((p)<48)?(p):-1)
#define digitalPinHasPWM(p)         (p < 46)

static const uint8_t TX = 43;
static const uint8_t RX = 44;

static const uint8_t SDA = 3;
static const uint8_t SCL = 4;

static const uint8_t SS    = 10;
static const uint8_t MOSI  = 11;
static const uint8_t MISO  = 13;
static const uint8_t SDO  = 11;
static const uint8_t SDI  = 13;
static const uint8_t SCK   = 12;

static const uint8_t A2 = 2;
static const uint8_t A7 = 7;

static const uint8_t D5 = 5;
static const uint8_t D6 = 6;
static const uint8_t D7 = 7;
static const uint8_t D8 = 8;
static const uint8_t D9 = 9;
static const uint8_t D14 = 14;
static const uint8_t D15 = 15;
static const uint8_t D16 = 16;
static const uint8_t D17 = 17;
static const uint8_t D18 = 18;
static const uint8_t D21 = 21;
static const uint8_t D33 = 33;
static const uint8_t D35 = 35;
static const uint8_t D36 = 36;
static const uint8_t D37 = 37;
static const uint8_t D38 = 38;
static const uint8_t D39 = 39;
static const uint8_t D40 = 40;
static const uint8_t D41 = 41;
static const uint8_t D42 = 42;
static const uint8_t D43 = 43;
static const uint8_t D44 = 44;
static const uint8_t D45 = 45;
static const uint8_t D46 = 46;
static const uint8_t D47 = 47;
static const uint8_t D48 = 48;

static const uint8_t T5 = 5;
static const uint8_t T6 = 6;
static const uint8_t T7 = 7;
static const uint8_t T8 = 8;
static const uint8_t T9 = 9;
static const uint8_t T14 = 14;

static const uint8_t VBAT_VOLTAGE = 1;

#endif /* Pins_Arduino_h */
