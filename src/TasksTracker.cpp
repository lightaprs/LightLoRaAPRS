#include <esp_task_wdt.h>
#include <Arduino.h>
#include "TasksTracker.h"
#include "TasksCommon.h"
#include "lora.h"
#include "logger.h"
#include "beacon.h"
#include "aprsis.h"
#include "gps.h"
#include "util.h"
#include "display.h"
#include "OneButton.h"
#include "SHTC3Sensor.h"
#include "message.h"

extern logging::Logger logger;
extern ConfigurationTracker trackerConfig;
extern ConfigurationGateway gatewayConfig;
extern ConfigurationRouter routerConfig;
extern ConfigurationMessaging messagingConfig;
extern ConfigurationCommon commonConfig;
extern OneButton btn;

#define TRACKER_STATUS_BEACON_INTERVAL 1800

static float rssi;
static float snr;
static float freqError;

QueueHandle_t rxMessageQueueTrck;

TaskHandle_t Task_Send_Status_Message_TX = NULL;
TaskHandle_t Task_Send_Regular_Beacon_TX = NULL;
TaskHandle_t Task_Send_Smart_Beacon_TX= NULL;
TaskHandle_t Task_Send_Deep_Sleep_Beacon_TX= NULL;
TaskHandle_t Task_Send_RX_Packets_To_Queue_Trck = NULL;
TaskHandle_t Task_Send_Packets_To_Messaging = NULL;
TaskHandle_t Task_Messaging_Button_Press_Check = NULL;

void setup_TrackerTasks(){

    rxMessageQueueTrck = xQueueCreate(RX_MESSAGE_QUEUE_SIZE, LORA_MAX_PAYLOAD_SIZE);
    if (rxMessageQueueTrck == 0) {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "setup_TrackerTasks", "Failed to create rxMessageQueueTrck!");  
    } else {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "setup_TrackerTasks", "rxMessageQueueTrck created...");
    } 

    if (trackerConfig.power.deepSleep)
    {
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "setup_TrackerTasks", "Creating tasksendDeepSleepBeaconTX...");
        xTaskCreate(tasksendDeepSleepBeaconTX,"tasksendDeepSleepBeaconTX",10000,NULL,1,&Task_Send_Deep_Sleep_Beacon_TX);
        esp_task_wdt_add(Task_Send_Deep_Sleep_Beacon_TX);  
    } else{

        if(messagingConfig.active){
          logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "setup_TrackerTasks", "Creating tasksendRXPacketsToQueueTrck...");
          xTaskCreate(tasksendRXPacketsToQueueTrck,"RxToQueue",10000,NULL,1,&Task_Send_RX_Packets_To_Queue_Trck);
          delay(100);

          logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "setup_TrackerTasks", "Creating tasksendRXPacketsToMessaging...");
          xTaskCreate(tasksendRXPacketsToMessaging,"RxToStore",10000,NULL,1,&Task_Send_Packets_To_Messaging);
          delay(100);

          logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "setup_TrackerTasks", "Creating taskmessagingButtonPressCheck...");
          xTaskCreate(taskmessagingButtonPressCheck,"MessagingButtonCheck",10000,NULL,1,&Task_Messaging_Button_Press_Check);
          delay(100);

          enableWebServerMessaging(true);
        }
        
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "setup_TrackerTasks", "Creating tasksendStatusMessageTX...");
        xTaskCreate(tasksendStatusMessageTX,"StatusMessageTX",10000,NULL,1,&Task_Send_Status_Message_TX);
        esp_task_wdt_add(Task_Send_Status_Message_TX);
        delay(1000);
        
        if (trackerConfig.smartBeacon.active)
        {
            logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "setup_TrackerTasks", "Creating tasksendSmartBeaconTX...");
            xTaskCreate(tasksendSmartBeaconTX,"SmartBeaconTX",10000,NULL,1,&Task_Send_Smart_Beacon_TX);
            esp_task_wdt_add(Task_Send_Smart_Beacon_TX);  
        } else {
            logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "setup_TrackerTasks", "Creating tasksendRegularBeaconTX...");
            xTaskCreate(tasksendRegularBeaconTX,"RegularBeaconTX",10000,NULL,1,&Task_Send_Regular_Beacon_TX);
            esp_task_wdt_add(Task_Send_Regular_Beacon_TX);
        }
        
    }
    delay(1000);
                
}

void taskmessagingButtonPressCheck(void * parameter){
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "taskmessagingButtonPressCheck", "xPortGetCoreID: %d", xPortGetCoreID());

  for(;;){
    btn.tick();
    checkBootButtonForMessaging();    
    vTaskDelay(100/ portTICK_PERIOD_MS);
  }

}

void tasksendRXPacketsToMessaging(void * parameter){
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "tasksendRXPacketsToMessaging", "xPortGetCoreID: %d", xPortGetCoreID());
  
  for(;;){
     esp_task_wdt_reset();
     RXPacketDef rxPacket;
     if(xQueueReceive(rxMessageQueueTrck, &(rxPacket), portMAX_DELAY))
     {      
        String packet = String((char*)rxPacket.payload);
        String loraPacket;
             
        if ((packet.substring(0, 3) == "\x3c\xff\x01")) {
          String sender = packet.substring(3,packet.indexOf(">"));
          if (trackerConfig.beacon.callsign != sender) {
            //show_display_two_lines_big_header(sender,packet.substring(packet.indexOf(">")));
            loraPacket = packet.substring(3);             
            Serial.println(loraPacket);
            processRXPackets(trackerConfig.beacon.callsign, loraPacket);
          }
        } else {
          Serial.println(packet);
          logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "tasksendRXPacketsToMessaging", "LoRa Packet Ignored (first 3 bytes or TCPIP/NOGATE/RFONLY)");
        }      
     }
    
    vTaskDelay(100/ portTICK_PERIOD_MS);
  } 
}

void tasksendRXPacketsToQueueTrck(void * parameter){
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "tasksendRXPacketsToQueueTrck", "xPortGetCoreID: %d", xPortGetCoreID());

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

          /**
          show_display_six_lines_big_header("Packet RX",
                      "",
                      "Packet Size:"+String(payloadSize)+ " bytes",
                      "RSSI: " + String(rssi),
                      "SNR : " + String(snr),
                      "Freq Err: " + String(freqError),0);       
           */
          logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "tasksendRXPacketsToQueueTrck", "Received LoRa packet, payload size: %d", payloadSize);
          logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "tasksendRXPacketsToQueueTrck", "RSSI         : %f dBm", rssi);
          logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "tasksendRXPacketsToQueueTrck", "SNR          : %f dB", snr);
          logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "tasksendRXPacketsToQueueTrck", "Freq. Error  : %f Hz", freqError);

          RXPacketDef rxPacket;
          rxPacket.packetSize = payloadSize;
          memcpy(rxPacket.payload, payload, payloadSize);

          xQueueSend(rxMessageQueueTrck, (void*)&rxPacket, portMAX_DELAY);
          logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "tasksendRXPacketsToQueueTrck", "RX packet added to RF queue...");
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

void tasksendStatusMessageTX(void * parameter){
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "tasksendStatusMessageTX", "xPortGetCoreID: %d", xPortGetCoreID());
  long last_tracker_status_packet_time = -3000000;
  for(;;){
    esp_task_wdt_reset();

    //Sending Router status message to RF 
    if (millis() - last_tracker_status_packet_time > TRACKER_STATUS_BEACON_INTERVAL * 1000) {
        show_display("\r\nStatus TX",0,2);
        ledBlink(true);
        if(trackerConfig.beacon.callsign.equals("TA2MUN-7")){
          sendBulletinMessage(trackerConfig.beacon.callsign, "BLN1", "General Bulletin Message Test "+String(random(100)));
        } else {
          trackerStatusTX();
        }        
        ledBlink(false);
        printFreeMEM();        
      last_tracker_status_packet_time = millis();
      
    }    

    if(trackerConfig.beacon.symbolCode=="O" || trackerConfig.beacon.gpsMode == GPS_MODE_BALLOON){
      if(gps.altitude.isValid() && gps.altitude.feet()>50000){
        setupGPSMode(GPS_MODE_BALLOON);
      }
    } else {
      sync_system_time_with_GPS();
    }

    vTaskDelay(60000 / portTICK_PERIOD_MS); //checking status message availability every 60 seconds...
  }
}

void tasksendRegularBeaconTX(void * parameter){
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "tasksendRegularBeaconTX", "xPortGetCoreID: %d", xPortGetCoreID());
  unsigned long lastTXTime = 0;

  for(;;){
    esp_task_wdt_reset();
    updateGPSData();
    //unsigned long diff = millis() - getTrackerLastTXTimeStamp();
    unsigned long diff = millis() - lastTXTime;

    bool ready_to_tx_time_interval = (diff > trackerConfig.beacon.interval * 1000); 
    bool ready_to_tx_location = gps.location.isValid() && gps.location.lat() != 0 && gps.satellites.value() > MIN_GPS_SATS && getTransmittedFlag();

    if (ready_to_tx_time_interval && ready_to_tx_location)
    {
        display_toggle(true);
        show_display("\r\n  Loc TX",0,2);
        ledBlink(true);
        trackerLocationTX(); //Sending location beacon...
        lastTXTime = millis();
        ledBlink(false);
        printFreeMEM();

    }  else if (!gps.location.isValid() || !(gps.satellites.value() > MIN_GPS_SATS)) {
        gpsSearchLedBlink();
        printGPSData();
        printTempHumidityData();
        displayGPSInfo();
    } else {
        displayRegularBeaconInfo(lastTXTime);           
    }
    vTaskDelay(1000/ portTICK_PERIOD_MS);
  }
}

void tasksendSmartBeaconTX(void * parameter){
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "tasksendSmartBeaconTX", "xPortGetCoreID: %d", xPortGetCoreID());

  for(;;){
    esp_task_wdt_reset();
    updateGPSData();
    bool ready_to_tx_location = gps.location.isValid() && gps.location.lat() != 0 && gps.satellites.value() > MIN_GPS_SATS && getTransmittedFlag();

    if (ready_to_tx_location)
    {
        bool ready_to_tx_smart_beacon = smartBeaconDecision();
        
        if(ready_to_tx_smart_beacon) {
            display_toggle(true);
            show_display("\r\n  Loc TX",0,2);
            ledBlink(true);
            trackerLocationTX(); //Sending location beacon...
            ledBlink(false);
            printFreeMEM();
        }

    } else if (!gps.location.isValid() && !(gps.satellites.value() > MIN_GPS_SATS)) {
        display_toggle(true);
        gpsSearchLedBlink();
        printGPSData();
        printTempHumidityData();
        displayGPSInfo();     
    }    
    vTaskDelay(1000/ portTICK_PERIOD_MS);
  }
}

void tasksendDeepSleepBeaconTX(void * parameter){
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "tasksendDeepSleepBeaconTX", "xPortGetCoreID: %d", xPortGetCoreID());

  for(;;){
    esp_task_wdt_reset();
    updateGPSData();
    bool ready_to_tx_location = gps.location.isValid() && gps.location.lat() != 0 && gps.satellites.value() > MIN_GPS_SATS && getTransmittedFlag();

    double   lastTxLat = 0.0;
    double   lastTxLng = 0.0;
    unsigned long lastTxTime  = 0;

    if (ready_to_tx_location)
    {
        show_display("\r\n  Loc TX",0,2);
        lastTxLat = gps.location.lat();
        lastTxLng = gps.location.lng();
        lastTxTime  = millis();    
        ledBlink(true);
        trackerLocationTX(); //Sending location beacon...
        ledBlink(false);
        printFreeMEM();

    } else if (!gps.location.isValid() || !(gps.satellites.value() > MIN_GPS_SATS)) {
        gpsSearchLedBlink();
        printGPSData();
        printTempHumidityData();
        displayGPSInfo();
    }

    if(ready_to_tx_location && isTrackerReadyToSleep()) {

        int beacon_tx_interval = trackerConfig.beacon.interval;
        if(trackerConfig.smartBeacon.active){
            beacon_tx_interval = getSmartBeaconDeepSleepRate(lastTxLat, lastTxLng, lastTxTime);
        }

        show_display("\r\nSleeping\r\n"+String(beacon_tx_interval)+" secs",0,2);
        GpsOFF;
        esp_sleep_enable_timer_wakeup(beacon_tx_interval * uS_TO_S_FACTOR);
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "MAIN", "ESP32 light sleep timer wakeup enabled...");      
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "MAIN", "Sleeping (deep) for %d seconds", beacon_tx_interval);
        
        Serial.flush();
        delay(100);
        display_toggle(false);
        esp_deep_sleep_start();        
    }

    vTaskDelay(1000/ portTICK_PERIOD_MS);
    }
}