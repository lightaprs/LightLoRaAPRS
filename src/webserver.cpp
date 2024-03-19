#include "webserver.h"
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <logger.h>
#include "display.h"
//Web Update
#include <Update.h>
#include <ESPmDNS.h>
#define U_PART U_SPIFFS


AsyncWebServer server(80);

const char* ssid = "Light_Tracker_Plus";
const char* password = "Light12345";

extern logging::Logger logger;
//Web Update
size_t content_len;
int updateProgress = 0;


void pageNotFound(AsyncWebServerRequest *request) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "WebServer", "Page not found: %s",request->url().c_str());
    request->send(404, "text/plain", "Not found");
}

void setup_WebServer()
{
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "WebServer", "WebServer Mode Enabled");
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "WebServer", "Setting AP (Access Point)");
  // Connect to Wi-Fi network with SSID and password
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "WebServer", "AP IP address: ",String(IP));

  server.serveStatic("/", SPIFFS, "/www/").setDefaultFile("index.html");

  server.on("/common_conf_json_fetch", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/common-config.json", "application/json");
  });

  server.on("/tracker_conf_json_fetch", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/tracker-config.json", "application/json");
  }); 

  server.on("/gateway_conf_json_fetch", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/gateway-config.json", "application/json");
  });

  server.on("/router_conf_json_fetch", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/router-config.json", "application/json");
  });

  server.on("/post_common_conf_json_update", HTTP_POST, [](AsyncWebServerRequest *request){},NULL,[](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total){
    String result;
    int status = jsonConfigUpdate("/common-config.json", result, request, data, len, index, total);
    request->send(status,"application/json",result);  
  });

  server.on("/post_tracker_conf_json_update", HTTP_POST, [](AsyncWebServerRequest *request){},NULL,[](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total){
    String result;
    int status = jsonConfigUpdate("/tracker-config.json", result, request, data, len, index, total);
    request->send(status,"application/json",result);   
  });  

  server.on("/post_gateway_conf_json_update", HTTP_POST, [](AsyncWebServerRequest *request){},NULL,[](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total){
    String result;
    int status = jsonConfigUpdate("/gateway-config.json", result, request, data, len, index, total);
    request->send(status,"application/json",result);   
  });  

  server.on("/post_router_conf_json_update", HTTP_POST, [](AsyncWebServerRequest *request){},NULL,[](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total){
    String result;
    int status = jsonConfigUpdate("/router-config.json", result, request, data, len, index, total);
    request->send(status,"application/json",result);   
  });

  //Web Update
  server.on("/do_update", HTTP_POST,[](AsyncWebServerRequest *request) {},[](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
    handleDoUpdate(request, filename, index, data, len, final);}
  );

  server.on("/check_update_progress", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200,"application/json", "{\"result\":\""+String(updateProgress)+"\"}"); 
  });  

  server.onNotFound(pageNotFound);

  server.begin();
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "WebServer", "Web Server started. You can change configuration via Wi-Fi using following info:\r\n");
  Serial.print("SSID: ");Serial.println(ssid);
  Serial.print("Password: ");Serial.println(password);
  Serial.print("URL: http://");Serial.println(IP);
  delay(4000);
  show_display("AP Wi-Fi:","","SSID:",ssid,"Password:",password,"http://"+IP.toString());

  Update.onProgress(printProgress);
}

int jsonConfigUpdate(String filePath, String &result, AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total){

  int status = 200;

   String msg = "";
    for (size_t i = 0; i < len; i++) {
      msg += (char) data[i];
    }

    logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "WebServer", msg.c_str());

    File file = SPIFFS.open(filePath, "w");
    if (!file) {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, filePath, "Failed to open file for writing...");
      status = 500;
      result = "{\"error\":\"Failed to open file for writing...\"}";
    }

    if(file.print(msg)){
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, filePath, "File written...");
      status = 200;
      result = "{\"result\":\"Configuration successfully updated...\"}";      
   }else {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, filePath, "Failed to write...");    
      status = 500;
      result = "{\"error\":\"Failed to write configuration.\"}";            
   }

   return status;    

}

void handleDoUpdate(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index){
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, filename, "Web update starting...");
    content_len = request->contentLength();
    // if filename includes spiffs, update the spiffs partition
    int cmd = (filename.indexOf("spiffs") > -1) ? U_PART : U_FLASH;

    if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, filename, "Update error.");
      Update.printError(Serial);
    }
  }

  if (Update.write(data, len) != len) {
    Update.printError(Serial);
  }

  if (final) {
    if (!Update.end(true)){
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, filename, "Update error.");
      request->send(500,"application/json", "{\"error\":\"Update error...\"}"); 
      Update.printError(Serial);
    } else {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, filename, "Update complete.");
      request->send(200,"application/json", "{\"result\":\"Update complete, restarting...\"}"); 
      Serial.flush();
      ESP.restart();
    }
  }
}

void printProgress(size_t prg, size_t sz) {
  updateProgress = (prg*100)/content_len;
  Serial.printf("Update Progress: %d%%\n", (prg*100)/content_len);
}