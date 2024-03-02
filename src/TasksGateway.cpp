#include <Arduino.h>
#include <esp_task_wdt.h>
#include "TasksGateway.h"
#include "TasksCommon.h"
#include "lora.h"
#include "logger.h"
#include "beacon.h"
#include "aprsis.h"
#include "gps.h"
#include "util.h"
#include "SHTC3Sensor.h"

#include "display.h"

extern logging::Logger logger;
extern ConfigurationGateway gatewayConfig;

#define IGATE_STATUS_BEACON_INTERVAL 900
#define IGATE_LOCATION_BEACON_INTERVAL 1200

QueueHandle_t rxMessageQueueGW;

TaskHandle_t Task_Send_RX_Packets_To_Queue_GW = NULL;
TaskHandle_t Task_Send_Packets_To_APRS_IS = NULL;
TaskHandle_t Task_iGate_Location_To_APRS_IS = NULL;
TaskHandle_t Task_Check_APRS_IS_Connection = NULL;

float last_RX_RSSI = 0;
float last_RX_SNR = 0;

void setup_GatewayTasks() {

    rxMessageQueueGW = xQueueCreate(RX_MESSAGE_QUEUE_SIZE, LORA_MAX_PAYLOAD_SIZE);
    if (rxMessageQueueGW == 0) {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "setup_GatewayTasks", "Failed to create rxMessageQueueGW!");  
    } else {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "setup_GatewayTasks", "rxMessageQueueGW created...");
    } 

    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "setup_GatewayTasks", "Creating tasksendRXPacketsToQueueGW...");
    xTaskCreate(tasksendRXPacketsToQueueGW,"RxToQueue",10000,NULL,1,&Task_Send_RX_Packets_To_Queue_GW);
    delay(100);
    esp_task_wdt_add(Task_Send_RX_Packets_To_Queue_GW);

    if (gatewayConfig.wifi.active && gatewayConfig.aprs_is.active)
    {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "setup_GatewayTasks", "Creating tasksendRXPacketsToAPRSIS...");
      xTaskCreate(tasksendRXPacketsToAPRSIS,"RxToAPRSIS",10000,NULL,1,&Task_Send_Packets_To_APRS_IS);
      delay(100);

      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "setup_GatewayTasks", "Creating tasksendiGateLocationToAPRSIS...");
      xTaskCreate(tasksendiGateLocationToAPRSIS,"iGateLocToAPRSIS",10000,NULL,1,&Task_iGate_Location_To_APRS_IS);       
      delay(100);
      esp_task_wdt_add(Task_iGate_Location_To_APRS_IS);

      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "setup_GatewayTasks", "Creating taskcheckAPRSISConnection...");
      xTaskCreate(taskcheckAPRSISConnection,"CheckAPRSISCon",10000,NULL,1,&Task_Check_APRS_IS_Connection);

      delay(100);
    }
    
}

void taskcheckAPRSISConnection(void * parameter){
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "taskcheckTimeAndAPRSISConnection", "xPortGetCoreID: %d", xPortGetCoreID());

  for(;;){
    esp_task_wdt_reset();
    //checks Wi-Fi and APRS-IS connection and reconnects if needed... 
    refresh_APRS_IS_connection();
    vTaskDelay(30000/ portTICK_PERIOD_MS);
  }
}

void tasksendiGateLocationToAPRSIS(void * parameter){
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "tasksendiGateLocationToAPRSIS", "xPortGetCoreID: %d", xPortGetCoreID());
  long last_igate_loc_packet_time = -1800000;
  unsigned long last_igate_status_packet_time = 0;
  for(;;){
    esp_task_wdt_reset();

    //Sending iGate Status message to APRS-IS to check connection is stil alive.
    if (millis() - last_igate_status_packet_time > IGATE_STATUS_BEACON_INTERVAL * 1000) {

      String message = "\x3c\xff\x01" + getiGateStatusAPRSMessage();

      int payloadSize = message.length()+1;
      byte payload[payloadSize];
      message.getBytes(payload, payloadSize);

      RXPacketDef rxPacket;
      rxPacket.packetSize = payloadSize;
        
      memcpy(rxPacket.payload, payload, payloadSize);
      xQueueSend(rxMessageQueueGW, (void*)&rxPacket, portMAX_DELAY);
      last_igate_status_packet_time = millis();
    }    
    //Sending iGate location to APRS-IS    
    if (millis() - last_igate_loc_packet_time > IGATE_LOCATION_BEACON_INTERVAL * 1000)
    {
      String message = "\x3c\xff\x01" + getiGateLocationAPRSMessage();

      int payloadSize = message.length()+1;
      byte payload[payloadSize];
      message.getBytes(payload, payloadSize);

      RXPacketDef rxPacket;
      rxPacket.packetSize = payloadSize;
        
      memcpy(rxPacket.payload, payload, payloadSize);
      xQueueSend(rxMessageQueueGW, (void*)&rxPacket, portMAX_DELAY);

      last_igate_loc_packet_time = millis();
    }
    
    vTaskDelay(1000/ portTICK_PERIOD_MS);
  }
}

void tasksendRXPacketsToAPRSIS(void * parameter){
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "tasksendRXPacketsToAPRSIS", "xPortGetCoreID: %d", xPortGetCoreID());
  for(;;){
     //esp_task_wdt_reset();
     RXPacketDef rxPacket;
     if(xQueueReceive(rxMessageQueueGW, &(rxPacket), portMAX_DELAY))
     {
 
        Serial.print("Received data from queue...");
        Serial.print("Packet Length: ");Serial.println(rxPacket.packetSize);
        
        String packet = String((char*)rxPacket.payload);

        if ((packet.substring(0, 3) == "\x3c\xff\x01") && (packet.indexOf("TCPIP") == -1) && (packet.indexOf("NOGATE") == -1) && (packet.indexOf("RFONLY") == -1)) {      
          Serial.println(packet.substring(3));
          sendDataToAPRSIS(packet.substring(3));
        } else {
          Serial.println(packet);
          logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "tasksendRXPacketsToAPRSIS", "LoRa Packet Ignored (first 3 bytes or TCPIP/NOGATE/RFONLY)");
        }      
     }
    
    vTaskDelay(100/ portTICK_PERIOD_MS);
  }
}

void tasksendRXPacketsToQueueGW(void * parameter){
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "tasksendRXPacketsToQueueGW", "xPortGetCoreID: %d", xPortGetCoreID());

  for(;;){
    esp_task_wdt_reset();
    if(getReceivedFlag()) {
      ledBlink(true);
      // reset flag
      setReceivedFlag(false);
      
      int payloadSize = getSX126XRadio().getPacketLength();
      byte payload[payloadSize];
      int state = getSX126XRadio().readData(payload, payloadSize);

      if (state == RADIOLIB_ERR_NONE) {
        // packet was successfully received
        float rssi      = getSX126XRadio().getRSSI();
        float snr       = getSX126XRadio().getSNR();
        float freqError = getSX126XRadio().getFrequencyError();

        last_RX_SNR = snr;
        last_RX_RSSI = rssi;

        show_display_six_lines_big_header("Packet RX",
                    "",
                    "Packet Size:"+String(payloadSize)+ " bytes",
                    "RSSI: " + String(rssi),
                    "SNR : " + String(snr),
                    "Freq Err: " + String(freqError));        

        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "tasksendRXPacketsToQueueGW", "Received LoRa packet, payload size: %d", payloadSize);
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "tasksendRXPacketsToQueueGW", "RSSI         : %f dBm", rssi);
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "tasksendRXPacketsToQueueGW", "SNR          : %f dB", snr);
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "tasksendRXPacketsToQueueGW", "Freq. Error  : %f Hz", freqError);

        RXPacketDef rxPacket;
        rxPacket.packetSize = payloadSize;
        memcpy(rxPacket.payload, payload, payloadSize);

        xQueueSend(rxMessageQueueGW, (void*)&rxPacket, portMAX_DELAY);

      } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
        // packet was received, but is malformed
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "tasksendRXPacketsToQueue", "CRC error!");

      } else {
        // some other error occurred
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "tasksendRXPacketsToQueue", "Listening LoRa packets failed, code:  %d", state); 
      }
    
      ledBlink(false);
    }
    
    vTaskDelay(100/ portTICK_PERIOD_MS);
  }
}