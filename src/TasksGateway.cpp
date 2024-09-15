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

#include "display.h"

extern logging::Logger logger;
extern ConfigurationGateway gatewayConfig;
extern ConfigurationCommon commonConfig;

#define IGATE_STATUS_BEACON_INTERVAL 900
#define IGATE_LOCATION_BEACON_INTERVAL 1200

QueueHandle_t rxMessageQueueGW;

TaskHandle_t Task_Send_RX_Packets_To_Queue_GW = NULL;
TaskHandle_t Task_Send_Packets_To_APRS_IS = NULL;
TaskHandle_t Task_iGate_Location_To_APRS_IS = NULL;
TaskHandle_t Task_Check_APRS_IS_Connection = NULL;

static float rssi;
static float snr;
static float freqError;
LastRXObject lastiGateRX;

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

    if (gatewayConfig.wifi.active)
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
    checkAPRS_ISConnection();
    vTaskDelay(60000/ portTICK_PERIOD_MS);
  }
  
}

void tasksendiGateLocationToAPRSIS(void * parameter){
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "tasksendiGateLocationToAPRSIS", "xPortGetCoreID: %d", xPortGetCoreID());
  long last_igate_loc_packet_time = -1800000;
  unsigned long last_igate_status_packet_time = 0;
  for(;;){
    esp_task_wdt_reset();

    String aprisConn = "NOK";
    if(gatewayConfig.aprs_is.active && aprs_is.connected()) {
      aprisConn = "OK";
    }

    String wifiConn = "OK";
    String digiOrIP = "IP: " + WiFi.localIP().toString();
    if(WiFi.status() != WL_CONNECTED) {
      wifiConn = "NOK";
      aprisConn = "NOK";
    }   

    if((WiFi.status() != WL_CONNECTED || !gatewayConfig.aprs_is.active) && gatewayConfig.digi.repeatAllPcktsNotConn){
        digiOrIP = "Mode: Digipeater";
    }

    int lastRXMinutes = 0;
    if(lastiGateRX.millis > 0){
      lastRXMinutes = (int)(((millis() - lastiGateRX.millis)/1000)/60);
    }

    show_display_six_lines_big_header(gatewayConfig.igate.callsign,
                "WiFi: "+ wifiConn + " APRSIS: "+ aprisConn,
                digiOrIP,
                "Last RX: " + lastiGateRX.callsign,
                "RSSI:" + String((int)lastiGateRX.rssi) + " SNR:" + String((int)lastiGateRX.snr),
                "Time: " + String(lastRXMinutes) + " min ago",0);   

  int secs_since_beacon = (int)(millis() - lastiGateRX.millis) / 1000;

  if(!commonConfig.display.always_on && secs_since_beacon > commonConfig.display.display_timeout){
    display_toggle(false);
  } else {
    display_toggle(true);
  }

    //Sending iGate Status message to APRS-IS to check connection is stil alive.
    if (millis() - last_igate_status_packet_time > IGATE_STATUS_BEACON_INTERVAL * 1000) {
      if(hasLostConnection()){
        display_toggle(true);
        show_display("\r\nStatus TX",0,2);
        String statusMessage = getiGateStatusAPRSMessage();
        routerTX(statusMessage);
        last_igate_status_packet_time = millis();
      } else {
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

    }    
    //Sending iGate location to APRS-IS    
    if (millis() - last_igate_loc_packet_time > IGATE_LOCATION_BEACON_INTERVAL * 1000)
    {
      if(hasLostConnection()){
        display_toggle(true);
        show_display("\r\n  Loc TX",0,2);
        String locMessage = getiGateLocationAPRSMessage();
        routerTX(locMessage);
        last_igate_loc_packet_time = millis();
      } else {
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
        String loraPacket;

        if ((packet.substring(0, 3) == "\x3c\xff\x01") && (packet.indexOf("TCPIP") == -1) && (packet.indexOf("NOGATE") == -1)) {      
          //Checking APRS data type...
          int dataTypeIdentifierIndex = packet.indexOf(":")+1;
          String dataTypeIdentifier = packet.substring(dataTypeIdentifierIndex,dataTypeIdentifierIndex+1);
          String sender = packet.substring(3,packet.indexOf(">"));

          if((gatewayConfig.igate.callsign != sender)) {
            lastiGateRX.callsign = sender;
            lastiGateRX.millis = millis();
            lastiGateRX.rssi = rssi;
            lastiGateRX.snr = snr;    
          }

          //Sending packets to APRS-IS
          if (packet.indexOf("RFONLY") == -1) {
            Serial.println(packet.substring(3));
            sendDataToAPRSIS(packet.substring(3));             
          } 
          
          //Checking data type is message or message capable, these packets are also digipeated...
          bool containsMessagingIdentifier = dataTypeIdentifier.indexOf("=") == 0 || dataTypeIdentifier.indexOf("@") == 0 || dataTypeIdentifier.indexOf(":") ==0;
          //Checking APRS-IS connection lost, all packets are digipeated regardless data type...
          if ((hasLostConnection()) || (gatewayConfig.digi.repeatMssgOnly && containsMessagingIdentifier) || packet.indexOf("RFONLY") > -1){

          int16_t indexWIDE1_1= packet.indexOf("WIDE1-1");
          int16_t indexDigiInPath= packet.indexOf(gatewayConfig.igate.callsign);
          int16_t indexGreaterSymbol= packet.indexOf(">");
          int16_t indexColonSymbol= packet.indexOf(":");

          bool wide1_1 = indexWIDE1_1 > indexGreaterSymbol && indexWIDE1_1 < indexColonSymbol;
          bool digiInPath = indexDigiInPath > indexGreaterSymbol && indexDigiInPath < indexColonSymbol;

            if ((wide1_1 || digiInPath) && (gatewayConfig.igate.callsign != sender)) {
              show_display_two_lines_big_header(sender,packet.substring(packet.indexOf(">")));
              loraPacket = packet.substring(3);              
              Serial.println(loraPacket);
              if (digiInPath) {
                loraPacket.replace(gatewayConfig.igate.callsign, gatewayConfig.igate.callsign+"," + "*,");
                loraPacket.replace(gatewayConfig.igate.callsign, gatewayConfig.igate.callsign+":" + "*:");
              } else {
                loraPacket.replace("WIDE1-1", gatewayConfig.igate.callsign + "*");
              }                      
              routerTX(loraPacket); 
            }

          } 

        } else {
          Serial.println(packet);
          logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "tasksendRXPacketsToAPRSIS", "LoRa Packet Ignored (first 3 bytes or TCPIP/NOGATE)");
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
        rssi      = getSX126XRadio().getRSSI();
        snr       = getSX126XRadio().getSNR();
        freqError = getSX126XRadio().getFrequencyError();

        show_display_six_lines_big_header("Packet RX",
                    "",
                    "Packet Size:"+String(payloadSize)+ " bytes",
                    "RSSI: " + String(rssi),
                    "SNR : " + String(snr),
                    "Freq Err: " + String(freqError),0);        

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