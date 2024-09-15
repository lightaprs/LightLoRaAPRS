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
extern ConfigurationCommon commonConfig;

#define ROUTER_STATUS_BEACON_INTERVAL 900
#define ROUTER_LOCATION_BEACON_INTERVAL 1200
#define ROUTER_RESTART_INTERVAL 259200

int16_t statusBeaconInterval = 0;
int16_t locationBeaconInterval = 0;

static uint16_t digipeated_packet_count=0;
static float rssi;
static float snr;
static float freqError;

LastRXObject lastDigiRX;

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

          bool solarDigipeatingDisabled= false;
          
          if (commonConfig.deviceModel == device_lightgateway_plus_1_0 ) {
              solarDigipeatingDisabled = getTemperatureC() > commonConfig.solar.disable_digipeating_above_temp 
                                        || readBatteryVoltage() < commonConfig.solar.disable_digipeating_below_volt;
          }
          int16_t indexWIDE1_1= packet.indexOf("WIDE1-1");
          int16_t indexDigiInPath= packet.indexOf(routerConfig.digi.callsign);
          int16_t indexGreaterSymbol= packet.indexOf(">");
          int16_t indexColonSymbol= packet.indexOf(":");

          bool wide1_1 = indexWIDE1_1 > indexGreaterSymbol && indexWIDE1_1 < indexColonSymbol;
          bool digiInPath = indexDigiInPath > indexGreaterSymbol && indexDigiInPath < indexColonSymbol;

          if ((wide1_1 || digiInPath) && (routerConfig.digi.callsign != sender) && !solarDigipeatingDisabled) {
            show_display_two_lines_big_header(sender,packet.substring(packet.indexOf(">")));
            loraPacket = packet.substring(3);             
            Serial.println(loraPacket);
            if (digiInPath) {
              loraPacket.replace(routerConfig.digi.callsign+",", routerConfig.digi.callsign + "*,");
              loraPacket.replace(routerConfig.digi.callsign+":", routerConfig.digi.callsign + "*:");
            } else {
              loraPacket.replace("WIDE1-1", routerConfig.digi.callsign + "*");
            }
            routerTX(loraPacket);
            ++digipeated_packet_count;
            lastDigiRX.callsign = sender;
            lastDigiRX.millis = millis();
            lastDigiRX.rssi = rssi;
            lastDigiRX.snr = snr;     
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
                      "Freq Err: " + String(freqError),0);       

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
  long last_router_startup_packet_time = millis();
  for(;;){
    esp_task_wdt_reset();
    int lastRXMinutes = 0;
    if(lastDigiRX.millis > 0){
      lastRXMinutes = (int)(((millis() - lastDigiRX.millis)/1000)/60);
    }

  if(commonConfig.deviceModel == device_lightgateway_1_0) {
    show_display_six_lines_big_header(routerConfig.digi.callsign,
                String(readBatteryVoltage()) + "V",
                "Last RX: " + lastDigiRX.callsign,
                "RSSI: " + String(lastDigiRX.rssi),
                "SNR : " + String(lastDigiRX.snr),
                "Time: " + String(lastRXMinutes) + " min ago",0);   
  } else {
    show_display_six_lines_big_header(routerConfig.digi.callsign,
                String(readBatteryVoltage()) + "V  T:" + String((int)getTemperature())+String(getTemperatureUnit())+" H:"+String((int)getHumidity())+"%",
                "Last RX: " + lastDigiRX.callsign,
                "RSSI: " + String(lastDigiRX.rssi),
                "SNR : " + String(lastDigiRX.snr),
                "Time: " + String(lastRXMinutes) + " min ago",0);       
  }

  int secs_since_beacon = (int)(millis() - lastDigiRX.millis) / 1000;

  if(!commonConfig.display.always_on && secs_since_beacon > commonConfig.display.display_timeout){
    display_toggle(false);
  } else {
    display_toggle(true);
  }

    if (commonConfig.deviceModel == device_lightgateway_plus_1_0) {
      if(getTemperatureC() < commonConfig.solar.disable_charging_below_temp) {
        digitalWrite(SOLAR_CHARGE_DISABLE_PIN,HIGH);
      } else {
        digitalWrite(SOLAR_CHARGE_DISABLE_PIN,LOW);
      }      
    } 

    if (commonConfig.deviceModel == device_lightgateway_plus_1_0 && readBatteryVoltage() != 0 
      && readBatteryVoltage() < commonConfig.solar.increase_status_loc_tx_interval_below_volt ) {
        statusBeaconInterval = 3600;
        locationBeaconInterval = 3600;
    } else {
        statusBeaconInterval = ROUTER_STATUS_BEACON_INTERVAL;
        locationBeaconInterval = ROUTER_LOCATION_BEACON_INTERVAL;
    } 

    //Sending Router location to RF    
    if (routerConfig.digi.sendDigiLoc && (millis() - last_router_loc_packet_time > locationBeaconInterval * 1000))
    {
      display_toggle(true);
      show_display("\r\n  Loc TX",0,2);
      String locMessage = getRouterLocationAPRSMessage();
      routerTX(locMessage);
      last_router_loc_packet_time = millis();        
    }

    //Sending Router status message to RF 
    if (millis() - last_router_status_packet_time > statusBeaconInterval * 1000) {
      display_toggle(true);
      show_display("\r\nStatus TX",0,2);
      String statusMessage = getRouterStatusAPRSMessage();
      statusMessage += " DigiPcktC:" + String(digipeated_packet_count);
      digipeated_packet_count = 0;
      routerTX(statusMessage);
      last_router_status_packet_time = millis();
    }    
    
    if (millis() - last_router_startup_packet_time > ROUTER_RESTART_INTERVAL * 1000) {
      esp_restart();
    }       

    vTaskDelay(1000/ portTICK_PERIOD_MS);
  }

}
