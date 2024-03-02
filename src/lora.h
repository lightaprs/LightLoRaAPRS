#ifndef lora_H
#define lora_H
#include <RadioLib.h>
#include "variant.h"
#include "configuration.h"
//#include "gps.h"

#define LORA_MAX_PAYLOAD_SIZE 256

struct RXPacketDef{
    int packetSize;
    byte payload[LORA_MAX_PAYLOAD_SIZE];
};

void setupLoRa();
void configLoRaRadio(float freq, float bandWidth, uint8_t spreadingFactor, uint8_t codingRate, bool crc, int8_t power, float currentLimit, uint16_t preambleLength);
SX126x getSX126XRadio();

void setTransmittedFlag(bool flag);
bool getTransmittedFlag();
void setReceivedFlag(bool flag);
bool getReceivedFlag();
void setupRX();

#endif