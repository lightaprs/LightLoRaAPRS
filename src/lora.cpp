#include "lora.h"
#include "logger.h"
#include "configuration.h"
#include "util.h"
#include "SHTC3Sensor.h"
#include "display.h"

extern logging::Logger logger;
extern ConfigurationCommon commonConfig;

SPIClass loraSPI(FSPI);

#ifdef USE_SX1268
    SX1268 radio = new Module(LORA_CS_PIN, LORA_IRQ_PIN, LORA_RST_PIN, LORA_BUSY_PIN,loraSPI);   
#elif USE_SX1262
    SX1262 radio = new Module(LORA_CS_PIN, LORA_IRQ_PIN, LORA_RST_PIN, LORA_BUSY_PIN,loraSPI);       
#endif

SX126x *pradio = &radio;

SX126x getSX126XRadio(){
  return *pradio;
}

volatile bool transmittedFlag = true;// flag to indicate that a packet was sent
volatile bool receivedFlag = false;// flag to indicate that a packet was received

void setTransmittedFlag(bool flag){
    transmittedFlag = flag;
}

bool getTransmittedFlag(){
  return transmittedFlag;
}

void setReceivedFlag(bool flag){
    receivedFlag = flag;
}

bool getReceivedFlag(){
  return receivedFlag;
}

void setupLoRa()
{
    show_display_print("LoRa setup...");
    pinMode(LORA_VCC_PIN,OUTPUT);  
    //pinMode(LORA_TX_PIN,OUTPUT);
    //pinMode(LORA_RX_PIN,OUTPUT);

    digitalWrite(LORA_VCC_PIN,HIGH);
    delay(500);

    logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "LoRa", "LoRa setup init...");
    loraSPI.begin(LORA_SCK_PIN,LORA_MISO_PIN,LORA_MOSI_PIN,LORA_CS_PIN);
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "LoRa", "LoRa SPI begin done!");

    // initialize LoRa Radio Module with default settings
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "LoRa", "LoRa Radio Module Initializing ... ");
    int state = radio.begin();
    if (state == RADIOLIB_ERR_NONE) {
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "LoRa", "LoRa Radio Module Initializing success... ");
    } else {
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "LoRa", "LoRa Radio Module Initializing failed... ");
        show_display("ERROR", "LoRa Radio Module Initializing failed.");        
        while (true);
    }

    float freq = commonConfig.lora.frequencyTX;
    float bandWidth = commonConfig.lora.signalBandwidth;
    uint8_t spreadingFactor = commonConfig.lora.spreadingFactor;
    uint8_t codingRate = commonConfig.lora.codingRate4;
    bool crc = commonConfig.lora.crc;
    int8_t power = commonConfig.lora.power;
    float currentLimit = 140;
    uint16_t preambleLength = commonConfig.lora.preambleLength;

    configLoRaRadio(freq,bandWidth,spreadingFactor,codingRate,crc,power,currentLimit,preambleLength);
    show_display_println("done.");

}

void configLoRaRadio(float freq, float bandWidth, uint8_t spreadingFactor, uint8_t codingRate, bool crc, int8_t power, float currentLimit, uint16_t preambleLength)
{
  char freq_buf[10];
  dtostrf(freq, 6, 3, freq_buf);

  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "LoRa", "frequency: %s", freq_buf);
  if (radio.setFrequency(freq) == RADIOLIB_ERR_INVALID_FREQUENCY) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "LoRa", "Selected frequency is invalid for this module!");
    while (true);
  }

  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "LoRa", "bandwith: %f", bandWidth);
  if (radio.setBandwidth(bandWidth) == RADIOLIB_ERR_INVALID_BANDWIDTH) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "LoRa", "Selected bandwidth is invalid for this module!");
    while (true);
  }

  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "LoRa", "spreadingFactor: %d", spreadingFactor);
  if (radio.setSpreadingFactor(spreadingFactor) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "LoRa", "Selected spreading factor is invalid for this module!");
    while (true);
  }

  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "LoRa", "codingRate: %d", codingRate);  
  if (radio.setCodingRate(codingRate) == RADIOLIB_ERR_INVALID_CODING_RATE) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "LoRa", "Selected codingRate is invalid for this module!");    
    while (true);
  }

  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "LoRa", "crc: %d", crc);  
  if (radio.setCRC(crc) == RADIOLIB_ERR_INVALID_CRC_CONFIGURATION) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "LoRa", "Selected CRC is invalid for this module!");       
    while (true);
  }

  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "LoRa", "power: %d", power);  
  if (radio.setOutputPower(power) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "LoRa", "Selected output power is invalid for this module!");
    while (true);
  }

  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "LoRa", "currentLimit: %f", currentLimit);
  if (radio.setCurrentLimit(currentLimit) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "LoRa", "Selected current limit is invalid for this module!");
    while (true);
  }

  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "LoRa", "syncWord: %d", RADIOLIB_SX126X_SYNC_WORD_PRIVATE);   
  if (radio.setSyncWord(RADIOLIB_SX126X_SYNC_WORD_PRIVATE) != RADIOLIB_ERR_NONE) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "LoRa", "Unable to set sync word!");
    while (true);
  }

  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "LoRa", "preambleLength: %d", preambleLength); 
  if (radio.setPreambleLength(preambleLength) == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "LoRa", "Selected preamble length is invalid for this module!");
    while (true);
  }

  radio.setRfSwitchPins(LORA_RX_PIN,LORA_TX_PIN);
  radio.setRxBoostedGainMode(true);

  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "LoRa", "LoRa setup done!");
    
}

void setupRX() {

  //digitalWrite(LORA_RX_PIN,HIGH);
  // start listening for LoRa packets
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "LoRa", "Starting to listen LoRa packets...");      
  int state = getSX126XRadio().startReceive();
  if (state == RADIOLIB_ERR_NONE) {
    show_display_println("done.");
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "LoRa", "Success! (Listening LoRa packets now)");
  } else {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "LoRa", "Listening LoRa packets failed, code:  %d", state);
    show_display_println("ERROR");      
    while (true);
  }

}