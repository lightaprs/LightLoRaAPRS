#include <APRS-IS.h>
#include <ESP32Ping.h>
#include "aprsis.h"
#include "gps.h"
#include "logger.h"
#include "configuration.h"
#include "display.h"


#define IGATE_PING_INTERVAL 1800
unsigned long last_igate_ping_time = 0;

extern logging::Logger logger;
extern ConfigurationGateway gatewayConfig;
uint8_t iGateRegion;

void setup_APRS_IS(){

  show_display_print("\r\nAPRS-IS conn...");
  aprs_is.setup(gatewayConfig.igate.callsign, gatewayConfig.aprs_is.passcode, "ESP32-APRS-IS", "0.2");

  aprs_is_server = gatewayConfig.aprs_is.server;

  if(gatewayConfig.aprs_is.autoServer) {

    double iGateLat = gatewayConfig.igate.latitude;
    double iGateLong = gatewayConfig.igate.longitude;

    if(gatewayConfig.igate.useGPS && gps.location.isValid() && gps.satellites.value() > 3){
      iGateLat = gps.location.lat();
      iGateLong = gps.location.lng();
    }

    iGateRegion = getRegionByLocation(iGateLat,iGateLong);

    switch (iGateRegion)
    {
    case no_region:
      aprs_is_server = gatewayConfig.aprs_is.server;
      break;
    case north_america:
      aprs_is_server = "noam.aprs2.net";
      break;    
    case south_america:
      aprs_is_server = "soam.aprs2.net";
      break;
    case europe:
      aprs_is_server = "euro.aprs2.net";
      break;      
    case asia:
      aprs_is_server = "asia.aprs2.net";
      break;
    case oceania:
      aprs_is_server = "aunz.aprs2.net";
      break;       
    default:
      break;
    }

  }
  Serial.print("Connecting to APRS-IS server: ");
  Serial.print(aprs_is_server);
  Serial.print(" on port: ");
  Serial.println(gatewayConfig.aprs_is.port);

  aprs_is.connect(aprs_is_server, gatewayConfig.aprs_is.port, gatewayConfig.aprs_is.filter);
  delay(3000);
  if (aprs_is.connected())
  {
    //aprs_is.available() > 0
    String msg_ = "";
    while(!msg_.startsWith("#"))
    {      
      msg_ = aprs_is.getMessage();
      delay(10);      
    }
    Serial.println(msg_);      
    show_display_println("done.");
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "APRS_IS", "Connected to server!");
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "APRS_IS", "APRS_IS setup completed...");
  } else {
    show_display_println("ERROR");
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "APRS_IS", "Connection failed.");
  }

}

void sendDataToAPRSIS(String message) {

    int index = message.indexOf(">");
    String sender = message.substring(0,index);
    if (aprs_is.connected()) {
        aprs_is.sendMessage(message);
        show_display_two_lines_big_header(sender,message.substring(index));
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "APRS_IS", "Message has been sent to APRS-IS...");
    } else if(WiFi.status() != WL_CONNECTED) {
        show_display("\r\nWi-Fi ERROR",0,2);
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "APRS_IS", "Wi-Fi connection lost, message delivery failed. Wi-Fi Status: %d",WiFi.status());
    } else {      
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "APRS_IS", "APRS-IS connection lost, message delivery failed...");
      show_display("\r\nReconnecting to APRS-IS",0,2);
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "APRS_IS", "Reconnecting to APRS-IS..."); 
      aprs_is.connect(aprs_is_server, gatewayConfig.aprs_is.port, gatewayConfig.aprs_is.filter);
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "APRS_IS", "Reconnected to APRS-IS...");
      delay(1000);
      if(aprs_is.connected()){
        aprs_is.sendMessage(message);
        show_display_two_lines_big_header(sender,message.substring(index));
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "APRS_IS", "Message has been sent to APRS-IS...");         
      }

    }

}

void refresh_APRS_IS_connection(){

    if (WiFi.status() != WL_CONNECTED) {
      WiFi.disconnect();
      WiFi.reconnect();
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "APRS_IS", "Reconnecting to Wi-Fi");
      while(WiFi.status() != WL_CONNECTED)
      {
        Serial.print(".");
        delay(500);
      }      
    }

    if (WiFi.status() == WL_CONNECTED && gatewayConfig.aprs_is.active && !aprs_is.connected()) {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "APRS_IS", "Reconnecting to APRS-IS..."); 
      aprs_is.connect(aprs_is_server, gatewayConfig.aprs_is.port, gatewayConfig.aprs_is.filter);
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "APRS_IS", "Reconnected to APRS-IS...");
    }

    if (millis() - last_igate_ping_time > IGATE_PING_INTERVAL * 1000) {

      bool success = Ping.ping(aprs_is_server.c_str());
      if(!success){
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "refresh_APRS_IS_connection", "APRIS-IS server %s ping failed...",aprs_is_server);
          logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "APRS_IS", "Restarting the EPS32...");
          esp_restart();
      }

    }

}