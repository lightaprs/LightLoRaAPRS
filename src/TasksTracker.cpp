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

extern logging::Logger logger;
extern ConfigurationTracker trackerConfig;
extern ConfigurationGateway gatewayConfig;
extern ConfigurationRouter routerConfig;
extern OneButton btn;

#define TRACKER_STATUS_BEACON_INTERVAL 1800

TaskHandle_t Task_Send_Status_Message_TX = NULL;
TaskHandle_t Task_Send_Regular_Beacon_TX = NULL;
TaskHandle_t Task_Send_Smart_Beacon_TX= NULL;
TaskHandle_t Task_Send_Deep_Sleep_Beacon_TX= NULL;

void setup_TrackerTasks(){

    if (trackerConfig.power.deepSleep)
    {
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "setup_TrackerTasks", "Creating tasksendDeepSleepBeaconTX...");
        xTaskCreate(tasksendDeepSleepBeaconTX,"tasksendDeepSleepBeaconTX",10000,NULL,1,&Task_Send_Deep_Sleep_Beacon_TX);
        esp_task_wdt_add(Task_Send_Deep_Sleep_Beacon_TX);  
    } else{

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

void tasksendStatusMessageTX(void * parameter){
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "tasksendStatusMessageTX", "xPortGetCoreID: %d", xPortGetCoreID());

  long last_tracker_status_packet_time = -3000000;
  for(;;){
    esp_task_wdt_reset();

    //Sending Router status message to RF 
    if (millis() - last_tracker_status_packet_time > TRACKER_STATUS_BEACON_INTERVAL * 1000) {
        show_display("\r\nStatus TX",0,2);
        ledBlink(true);        
        trackerStatusTX();
        ledBlink(false);
        printFreeMEM();        
      last_tracker_status_packet_time = millis();
    }    

    vTaskDelay(60000 / portTICK_PERIOD_MS); //checking status message availability every 60 seconds...
  }
}

void tasksendRegularBeaconTX(void * parameter){
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "tasksendRegularBeaconTX", "xPortGetCoreID: %d", xPortGetCoreID());

  for(;;){
    esp_task_wdt_reset();
    updateGPSData();
    unsigned long diff = millis() - getTrackerLastTXTimeStamp();

    bool ready_to_tx_time_interval = (diff > trackerConfig.beacon.interval * 1000); 
    bool ready_to_tx_location = gps.location.isValid() && gps.location.lat() != 0 && gps.satellites.value() > MIN_GPS_SATS && getTransmittedFlag();

    if (ready_to_tx_time_interval && ready_to_tx_location)
    {
        show_display("\r\n  Loc TX",0,2);
        sync_system_time_with_GPS();
        ledBlink(true);
        trackerLocationTX(); //Sending location beacon...
        ledBlink(false);
        printFreeMEM();

    }  else if (!gps.location.isValid() || !(gps.satellites.value() > MIN_GPS_SATS)) {
        gpsSearchLedBlink();
        printGPSData();
        printTempHumidityData();
        displayGPSInfo();
    } else {
        displayRegularBeaconInfo();
    }
    vTaskDelay(1000/ portTICK_PERIOD_MS);
  }
}

void tasksendSmartBeaconTX(void * parameter){
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "tasksendSmartBeaconTX", "xPortGetCoreID: %d", xPortGetCoreID());

  for(;;){
    btn.tick();
    esp_task_wdt_reset();
    updateGPSData();
    bool ready_to_tx_location = gps.location.isValid() && gps.location.lat() != 0 && gps.satellites.value() > MIN_GPS_SATS && getTransmittedFlag();

    if (ready_to_tx_location)
    {
        bool ready_to_tx_smart_beacon = smartBeaconDecision();
        
        if(ready_to_tx_smart_beacon) {
            show_display("\r\n  Loc TX",0,2);
            sync_system_time_with_GPS();
            ledBlink(true);
            trackerLocationTX(); //Sending location beacon...
            ledBlink(false);
            printFreeMEM();
        }

    } else if (!gps.location.isValid()) {
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
        sync_system_time_with_GPS();
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