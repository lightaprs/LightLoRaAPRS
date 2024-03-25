#include <APRS-IS.h>
#include <ESP32Ping.h>
#include "aprsis.h"
#include "gps.h"
#include "logger.h"
#include "configuration.h"
#include "display.h"


#define IGATE_PING_INTERVAL 3600
unsigned long last_igate_ping_time = 0;

extern logging::Logger logger;
extern ConfigurationGateway gatewayConfig;
uint8_t iGateRegion;

void setup_APRS_IS(){

  display_clear();
  show_display_println("APRS-IS",0,2);
  show_display_print("Callsign: ");show_display_println(gatewayConfig.igate.callsign);
  show_display_print("Passcode: ");show_display_println(gatewayConfig.aprs_is.passcode);

  if(atoi(gatewayConfig.aprs_is.passcode.c_str()) == aprspass(gatewayConfig.igate.callsign.c_str())){
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

    show_display_print("Server:");show_display_println(aprs_is_server);
    Serial.print("Connecting to APRS-IS server: ");
    Serial.print(aprs_is_server);
    Serial.print(" on port: ");
    Serial.println(gatewayConfig.aprs_is.port);

    show_display_print("Connecting...");
    aprs_is.connect(aprs_is_server, gatewayConfig.aprs_is.port, gatewayConfig.aprs_is.filter);
    
    delay(3000);
    if (aprs_is.connected())
    {
      unsigned long aprsisFirstAttempt = millis();
      bool connTimeOut = false;
      String msg_ = "";
      while(!msg_.startsWith("#") && !connTimeOut)
      {      
        msg_ = aprs_is.getMessage();
        delay(10);
        connTimeOut = (millis() - aprsisFirstAttempt) > 60000;
      }
      if(!connTimeOut) {
        show_display_println("done");
        show_display_println(msg_);
        Serial.println(msg_);
        delay(2000);
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "APRS_IS", "Connected to server!");
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "APRS_IS", "APRS_IS setup completed...");
      } else {
        show_display_println("ERROR(timeout)");
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "APRS_IS", "Connection (timeout) failed.");          
      }
      

    } else {
      show_display_println("ERROR");
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "APRS_IS", "Connection failed.");
    }

  } else {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "APRS_IS", "INVALID PASSCODE.....");
    show_display_println("");
    show_display_println(" INVALID PASSCODE!!!");
    
    while(true) {
      delay(100);
    }
    

  }



}

unsigned int aprspass(const char *callsign) {
    char realcall[11];
    unsigned int hash = 0x73e2;
    int i = 0, len;

    // Find '-' and truncate callsign if necessary
    const char *stophere = strchr(callsign, '-');
    if (stophere != NULL) {
        len = stophere - callsign;
        if (len > 10)
            len = 10;
        strncpy(realcall, callsign, len);
    } else {
        strncpy(realcall, callsign, 10);
        len = strlen(callsign);
    }
    realcall[len] = '\0';

    // Convert to uppercase
    for (i = 0; i < len; i++) {
        realcall[i] = toupper(realcall[i]);
    }

    // Hash callsign two bytes at a time
    for (i = 0; i < len; i += 2) {
        hash ^= realcall[i] << 8;
        if (realcall[i + 1] != '\0')
            hash ^= realcall[i + 1];
    }

    // Mask off the high bit so number is always positive
    return hash & 0x7fff;
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
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "APRS_IS", "APRS-IS connection lost...");
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

bool hasLostConnection() {
  return gatewayConfig.digi.repeatAllPcktsNotConn && (WiFi.status() != WL_CONNECTED || !aprs_is.connected());
}

void checkAPRS_ISConnection(){

    if (WiFi.status() != WL_CONNECTED) {
      WiFi.disconnect();
      WiFi.reconnect();
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "APRS_IS", "Reconnecting to Wi-Fi");

      unsigned long wifiFirstAttempt = millis();
      bool connTimeOut = false;
      while(WiFi.status() != WL_CONNECTED && !connTimeOut)
      {
        Serial.print(".");
        delay(500);
        connTimeOut = (millis() - wifiFirstAttempt) > 10000;
      }

      if(!connTimeOut) {
        display_toggle(true);
        show_display_println("\r\nWi-Fi reconnected..");
        Serial.println("connected.");
        Serial.print("Local IP: ");
        Serial.print(WiFi.localIP());
        Serial.print(" @ ");
        Serial.print(WiFi.RSSI());
        Serial.println("dBm");
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "Wi-Fi", "Wi-Fi reconnected...");
      } else {
        display_toggle(true);
        show_display_println("\r\nWi-Fi ERROR");
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Wi-Fi", "Wi-Fi Connection failed, check SSID or password...");      
      }      
 
    }

    if (WiFi.status() == WL_CONNECTED && gatewayConfig.aprs_is.active && !aprs_is.connected()) {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "APRS_IS", "Reconnecting to APRS-IS..."); 
      aprs_is.connect(aprs_is_server, gatewayConfig.aprs_is.port, gatewayConfig.aprs_is.filter);
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "APRS_IS", "Reconnected to APRS-IS...");
    }

    if (millis() - last_igate_ping_time > IGATE_PING_INTERVAL * 1000) {

      bool success = Ping.ping(aprs_is_server.c_str());
      if(WiFi.status() == WL_CONNECTED && ((gatewayConfig.aprs_is.active && !aprs_is.connected()) || (gatewayConfig.aprs_is.active && !success))){
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "refresh_APRS_IS_connection", "APRIS-IS server %s ping failed...",aprs_is_server);
          logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "APRS_IS", "Restarting the EPS32...");
          esp_restart();
      } else {
        last_igate_ping_time = millis();
      }

    }

}