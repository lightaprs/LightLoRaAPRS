#ifndef shtc3sensor_H
#define shtc3sensor_H

#include "SparkFun_SHTC3.h"

void setup_SHTC3Sensor();
float getTemperature();
char getTemperatureUnit();
float getTemperatureC();
float getTemperatureF();
float getHumidity();
void printTempHumidityData();

#endif