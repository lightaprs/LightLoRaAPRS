#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <RadioLib.h>
#include <TimeLib.h>
#include <APRS-IS.h>
#include "variant.h"
#include "logger.h"
#include "gps.h"
#include "lora.h"
#include "util.h"
#include "SHTC3Sensor.h"
#include "configuration.h"
#include "config_loader.h"
#include "beacon.h"
#include "aprsis.h"
#include "TasksGateway.h"
#include "TasksRouter.h"
#include "TasksTracker.h"
#include "TasksCommon.h"
#include "display.h"
#include "webserver.h"
#include "button.h"

#define VERSION "1.0.5"

logging::Logger logger;
TinyGPSPlus gps;
ConfigurationCommon commonConfig;
ConfigurationTracker trackerConfig;
ConfigurationGateway gatewayConfig;
ConfigurationRouter routerConfig;
RTC_DATA_ATTR int bootCount = 0;
APRS_IS aprs_is;


void setup_I2C();
void setup_WiFi();
void print_wakeup_reason();
void print_reset_reason();
void setup_Tracker();
void setup_Gateway();
void setup_Router();

void setup() {
  Serial.begin(115200);
  setup_Button();
  pinMode(LED_PIN, OUTPUT);
  ledBlink(true);  
  setup_I2C();
  delay(7000);    
  print_reset_reason();
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "MAIN", "Light APRS Tracker & Gateway by QRP Labs");
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "MAIN", "Version: " VERSION);
  setup_Display();
  show_display("Light Tracker Plus", "Version: " VERSION, 1000);
  load_config();
  if(isButtonPressed() || isInvalidConfig()){//if BOOT button is pressed or invalid configuration
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "MAIN", "Boot button pressed or invalid config...");
    setup_WebServer();
  } else {
    setupLoRa();
    setup_WatchdogTimer();
    setup_SHTC3Sensor();
    setup_Tracker();
    setup_Gateway();
    setup_Router();
  }
  ledBlink(false);
}

void setup_Router(){
  if(commonConfig.deviceMode == mode_digi){
    show_display("Mode: Digi");
    //Disable Wi-Fi & Bluetooth
    WiFi.mode(WIFI_OFF); //Wi-Fi OFF
    btStop(); //Bluetooth OFF
    show_display_println("\r\nWi-Fi/Bluetooth OFF");
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "Wi-Fi", "Wi-Fi and Bluetooth disabled..."); 

    show_display_print("LoRa RX init...");
    getSX126XRadio().setPacketReceivedAction(setRXFlag);
    setupRX();

    if(routerConfig.digi.useGPS) {
      bool isGPSFixed = tryGPSFix(routerConfig.digi.gpsTimeout);

      if(isGPSFixed) {
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "setup_Router()", "GPS is fixed, GPS latitude and longitude values will be used for digi...");  
      } else {
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "setup_Router()", "GPS is not fixed yet, following latitude and longitude values in the conf file will be used for digi...");
        Serial.print("Latitude: ");Serial.println(routerConfig.digi.latitude);
        Serial.print("Longitude: ");Serial.println(routerConfig.digi.longitude);
      }     
    }
      GpsOFF;
      show_display_two_lines_big_header(routerConfig.digi.callsign,"digi is ready...");
      delay(1000);
      setup_RouterTasks();
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "MAIN", "Router setup completed...");
  }
}

void setup_Gateway(){
  if(commonConfig.deviceMode == mode_igate){
    show_display("Mode: iGate");
    if (gatewayConfig.wifi.active)
    {
      setup_WiFi();
    }
    
    //setup LoRa RX
    show_display_print("\r\nLoRa init...");
    getSX126XRadio().setPacketReceivedAction(setRXFlag);
    setupRX();

    if(gatewayConfig.igate.useGPS) {
      bool isGPSFixed = tryGPSFix(gatewayConfig.igate.gpsTimeout);
      if(isGPSFixed) {
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "setup_Gateway()", "GPS is fixed, GPS latitude and longitude values will be used for gateway...");  
      } else {
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "setup_Gateway()", "GPS is not fixed yet, following latitude and longitude values in the conf file will be used for gateway...");
        Serial.print("Latitude: ");Serial.println(gatewayConfig.igate.latitude);
        Serial.print("Longitude: ");Serial.println(gatewayConfig.igate.longitude);
      }
      
    } else {
      GpsOFF;
    }
    if (gatewayConfig.wifi.active && gatewayConfig.aprs_is.active && WiFi.status() == WL_CONNECTED)
    {
      setup_APRS_IS();
    }
    delay(1000);
    setup_GatewayTasks();
    delay(1000);

    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "MAIN", "iGate setup completed...");
    
  }
}
void setup_Tracker(){
  if(commonConfig.deviceMode == mode_tracker) {
    
    //Disable Wi-Fi & Bluetooth
    WiFi.mode(WIFI_OFF); //Wi-Fi OFF
    btStop(); //Bluetooth OFF
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "Wi-Fi", "Wi-Fi and Bluetooth disabled...");
    show_display_println("Wi-Fi OFF.");
    show_display_print("GPS init...");
    setupGPS();
    show_display_println("done.");
    if (trackerConfig.beacon.gpsMode != 0)
    {
      delay(1000);
      setupGPSMode(trackerConfig.beacon.gpsMode);
    } else if(trackerConfig.beacon.symbolCode=="O") {
      //if aprs symbol is balloon, we are forcing GPS module to use balloon mode. This is important for altitude limit.
      //Most likely user forgot to change GPS mode...
      setupGPSMode(GPS_MODE_BALLOON);
    }

    delay(1000);
    show_display("Beacon conf:", 500);
    //setup LoRa TX
    getSX126XRadio().setDio1Action(setTXFlag);
    if(trackerConfig.power.deepSleep) {
      show_display_println("Deep Sleep ON.");
      ++bootCount;
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "MAIN", "ESP Deep Sleep Enabled..."); 
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "MAIN", "Boot number: %d",  bootCount); 
      //Print the wakeup reason for ESP32
      print_wakeup_reason();

      if(trackerConfig.smartBeacon.active){
        show_display_println("Smart Beacon ON.");
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "MAIN", "SmartBeacon enabled but only speed data will be used for beacon interval...");
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "MAIN", "Beacon rate(interval) may vary between %d and %d seconds.", trackerConfig.smartBeacon.fastRate, trackerConfig.smartBeacon.slowRate);
      }

    } else if(trackerConfig.smartBeacon.active) {
      show_display_println("Deep Sleep OFF.");
      show_display_println("Smart Beacon ON.");
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "MAIN", "SmartBeacon enabled, speed and heading data will be used for beacon interval...");
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "MAIN", "Beacon rate(interval) may vary between %d and %d seconds.", trackerConfig.smartBeacon.fastRate, trackerConfig.smartBeacon.slowRate);
    }
    show_display_print("Tasks setup...",1000); 
    setup_TrackerTasks();
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "MAIN", "Tracker setup completed...");
  }

}

void loop() {

}

void setup_I2C(){
  Wire.begin(SDA,SCL);
}

void setup_WiFi(){

  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "Wi-Fi", "Wi-Fi Setup (station mode: the ESP32 connects to an access point)");
  WiFi.persistent(true);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  if (gatewayConfig.network.hostname.overwrite) {
    WiFi.setHostname(gatewayConfig.network.hostname.name.c_str());
  } else {
    WiFi.setHostname((gatewayConfig.igate.callsign).c_str());
  }


  // Configures static IP address
  if (!gatewayConfig.network.DHCP) {
    WiFi.config(gatewayConfig.network.static_.ip, gatewayConfig.network.static_.gateway, gatewayConfig.network.static_.subnet, gatewayConfig.network.static_.dns1, gatewayConfig.network.static_.dns2);
  }

    WiFi.begin(gatewayConfig.wifi.ssid, gatewayConfig.wifi.password);
    Serial.print("Waiting for WiFi: ");
    show_display_print("Wi-Fi connecting");
    Serial.print(gatewayConfig.wifi.ssid);

    unsigned long wifiFirstAttempt = millis();
    bool connTimeOut = false;
    while(WiFi.status() != WL_CONNECTED && !connTimeOut)
    {
      Serial.print(".");
      show_display_print(".");
      delay(500);
      connTimeOut = (millis() - wifiFirstAttempt) > 10000;
    }

    if(!connTimeOut) {
      show_display_println("\r\nWi-Fi connected.");
      delay(1000);
      Serial.println("connected.");
      Serial.print("Local IP: ");
      Serial.print(WiFi.localIP());
      Serial.print(" @ ");
      Serial.print(WiFi.RSSI());
      Serial.println("dBm");
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "Wi-Fi", "Wi-Fi Setup completed...");
    } else {
      show_display_println("\r\nWi-Fi ERROR");
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Wi-Fi", "Wi-Fi Connection failed, check SSID or password...");
      delay(3000);      
    }

}

void print_wakeup_reason(){
  
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  if(wakeup_reason != 0) {
    setTXCount(bootCount);
  } 

  switch(wakeup_reason)
  {    
    case ESP_SLEEP_WAKEUP_EXT0 : logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "MAIN", "Wakeup caused by external signal using RTC_IO");  break;
    case ESP_SLEEP_WAKEUP_EXT1 : logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "MAIN", "Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "MAIN", "Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "MAIN", "Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "MAIN", "Wakeup caused by ULP program"); break;
    default : logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "MAIN", "Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void print_reset_reason(){
  
  esp_reset_reason_t reset_reason = esp_reset_reason();

  //Serial.println("Printing reset reason:");
  switch (reset_reason) {
    case ESP_RST_UNKNOWN:
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "MAIN", "Reset reason can not be determined");
      break;
    case ESP_RST_POWERON:
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "MAIN", "Reset due to power-on event...");
      break;
    case ESP_RST_EXT:
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "MAIN", "Reset by external pin (not applicable for ESP32)");
      break;
    case ESP_RST_SW:
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "MAIN", "Software reset via esp_restart");
      break;
    case ESP_RST_PANIC:
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "MAIN", "Software reset due to exception/panic");
      break;      
    case ESP_RST_INT_WDT:
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "MAIN", "Reset (software or hardware) due to interrupt watchdog");
      break;  
    case ESP_RST_TASK_WDT:
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "MAIN", "Reset due to task watchdog");
      break;  
    case ESP_RST_WDT:
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "MAIN", "Reset due to other watchdogs");
      break;  
    case ESP_RST_DEEPSLEEP:
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "MAIN", "Reset after exiting deep sleep mode...");
      break;  
    case ESP_RST_BROWNOUT:
      Serial.println("Brownout reset (software or hardware)");
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "MAIN", "Brownout reset (software or hardware)");
      break;  
    case ESP_RST_SDIO:
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "MAIN", "Reset over SDIO");
      break;  

    default:
      break;
  }

}