#ifndef webserver_H
#define webserver_H
#include <Arduino.h>
#include <ESPAsyncWebServer.h>

void setup_WebServer();
int jsonConfigUpdate(String filePath, String &result, AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total);
#endif