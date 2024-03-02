#include <Arduino.h>
#include <esp_task_wdt.h>
#include "TasksRouter.h"
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
extern ConfigurationRouter routerConfig;

#define ROUTER_STATUS_BEACON_INTERVAL 900
#define ROUTER_LOCATION_BEACON_INTERVAL 1200
static uint16_t digipeated_packet_count=0;
static float rssi;
static float snr;
static float freqError;
LastRXObject lastRX;

QueueHandle_t rxMessageQueueR;

TaskHandle_t Task_Send_RX_Packets_To_Queue_R = NULL;
TaskHandle_t Task_Send_Packets_To_RF = NULL;
TaskHandle_t Task_Router_Location_To_RF = NULL;

void setup_RouterTasks() {

    rxMessageQueueR = xQueueCreate(RX_MESSAGE_QUEUE_SIZE, LORA_MAX_PAYLOAD_SIZE);
    if (rxMessageQueueR == 0) {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "setup_RouterTasks", "Failed to create rxMessageQueueR!");  
    } else {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "setup_RouterTasks", "rxMessageQueueR created...");
    } 

    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "setup_RouterTasks", "Creating tasksendRXPacketsToQueueR...");
    xTaskCreate(tasksendRXPacketsToQueueR,"RxToQueue",10000,NULL,1,&Task_Send_RX_Packets_To_Queue_R);
    delay(100);
    esp_task_wdt_add(Task_Send_RX_Packets_To_Queue_R);
  
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "setup_RouterTasks", "Creating tasksendRXPacketsToRF...");
    xTaskCreate(tasksendRXPacketsToRF,"RxToRF",10000,NULL,1,&Task_Send_Packets_To_RF);
    delay(100);

    
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "setup_RouterTasks", "Creating tasksendRouterLocationToRF...");
    xTaskCreate(tasksendRouterLocationToRF,"RouterLocToRF",10000,NULL,1,&Task_Router_Location_To_RF);       
    delay(100);
    esp_task_wdt_add(Task_Router_Location_To_RF);
    
}

void tasksendRXPacketsToRF(void * parameter){
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "tasksendRXPacketsToRF", "xPortGetCoreID: %d", xPortGetCoreID());
  for(;;){

     RXPacketDef rxPacket;
     if(xQueueReceive(rxMessageQueueR, &(rxPacket), portMAX_DELAY))
     {      
        String packet = String((char*)rxPacket.payload);
        String loraPacket;
             
        if ((packet.substring(0, 3) == "\x3c\xff\x01") && (packet.indexOf("TCPIP") == -1) && (packet.indexOf("NOGATE") == -1)) {
          String sender = packet.substring(3,packet.indexOf(">"));
          if ((packet.indexOf("WIDE1-1") > 10) && (routerConfig.digi.callsign != sender)) {
            show_display_two_lines_big_header(sender,packet.substring(packet.indexOf(">")));
            loraPacket = packet.substring(3);
            logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "tasksendRXPacketsToRF", "Received APRS message:");                
            Serial.println(loraPacket);              
            loraPacket.replace("WIDE1-1", routerConfig.digi.callsign + "*");
            routerTX(loraPacket);
            ++digipeated_packet_count;
            lastRX.callsign = sender;
            lastRX.millis = millis();
            lastRX.rssi = rssi;
            lastRX.snr = snr;     
          }
        } else {
          Serial.println(packet);
          logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "tasksendRXPacketsToRF", "LoRa Packet Ignored (first 3 bytes or TCPIP/NOGATE/RFONLY)");
        }      
     }
    
    vTaskDelay(100/ portTICK_PERIOD_MS);
  } 
}

void tasksendRXPacketsToQueueR(void * parameter){
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "tasksendRXPacketsToQueueR", "xPortGetCoreID: %d", xPortGetCoreID());

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
          //packet was successfully received
          if(payloadSize > 0) { //ignore 0 bytese packets, more likely own RF packets

          rssi      = getSX126XRadio().getRSSI();
          snr       = getSX126XRadio().getSNR();
          freqError = getSX126XRadio().getFrequencyError();

          show_display_six_lines_big_header("Packet RX",
                      "",
                      "Packet Size:"+String(payloadSize)+ " bytes",
                      "RSSI: " + String(rssi),
                      "SNR : " + String(snr),
                      "Freq Err: " + String(freqError));       

          logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "tasksendRXPacketsToQueueR", "Received LoRa packet, payload size: %d", payloadSize);
          logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "tasksendRXPacketsToQueueR", "RSSI         : %f dBm", rssi);
          logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "tasksendRXPacketsToQueueR", "SNR          : %f dB", snr);
          logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "tasksendRXPacketsToQueueR", "Freq. Error  : %f Hz", freqError);

          RXPacketDef rxPacket;
          rxPacket.packetSize = payloadSize;
          memcpy(rxPacket.payload, payload, payloadSize);

          xQueueSend(rxMessageQueueR, (void*)&rxPacket, portMAX_DELAY);
          logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "tasksendRXPacketsToQueueR", "RX packet added to RF queue...");
        }

      } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
        // packet was received, but is malformed
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "tasksendRXPacketsToQueueR", "CRC error!");

      } else {
        // some other error occurred
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "tasksendRXPacketsToQueueR", "Listening LoRa packets failed, code:  %d", state); 
      }
    
      ledBlink(false);
    }
    
    vTaskDelay(100/ portTICK_PERIOD_MS);
  }
}

void tasksendRouterLocationToRF(void * parameter){

  logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "tasksendRouterLocationToRF", "xPortGetCoreID: %d", xPortGetCoreID());
  long last_router_loc_packet_time = -1200000;
  long last_router_status_packet_time = -800000;
  for(;;){
    esp_task_wdt_reset();
    int lastRXMinutes = 0;
    if(lastRX.millis > 0){
      lastRXMinutes = (int)(((millis() - lastRX.millis)/1000)/60);
    }

    show_display_six_lines_big_header(routerConfig.digi.callsign,
                "Last Packet:",
                lastRX.callsign,
                "RSSI: " + String(lastRX.rssi),
                "SNR : " + String(lastRX.snr),
                "Time: " + String(lastRXMinutes) + " min ago");   

    //Sending Router location to RF    
    if (routerConfig.digi.sendDigiLoc && (millis() - last_router_loc_packet_time > ROUTER_LOCATION_BEACON_INTERVAL * 1000))
    {
      show_display("\r\n  Loc TX",0,2);
      String locMessage = getRouterLocationAPRSMessage();
      routerTX(locMessage);
      last_router_loc_packet_time = millis();        
    }

    //Sending Router status message to RF 
    if (millis() - last_router_status_packet_time > ROUTER_STATUS_BEACON_INTERVAL * 1000) {
      show_display("\r\nStatus TX",0,2);
      String statusMessage = getRouterStatusAPRSMessage();
      routerTX(statusMessage);
      last_router_status_packet_time = millis();
    }    
    
    vTaskDelay(1000/ portTICK_PERIOD_MS);
  }

}
