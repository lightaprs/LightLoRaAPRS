#ifndef webserver_H
#define webserver_H
#include <Arduino.h>
#include <ESPAsyncWebServer.h>

void setup_WebServer();
void setup_WebServerForMessaging();
void stop_WebServer();
int jsonConfigUpdate(String filePath, String &result, AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total);
void handleUpdate(AsyncWebServerRequest *request);
void handleDoUpdate(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final);
void printProgress(size_t prg, size_t sz);
#endif