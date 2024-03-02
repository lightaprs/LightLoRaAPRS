#ifndef gps_H
#define gps_H

#include <TinyGPS++.h>
#include "variant.h"

#define GpsON         digitalWrite(GPS_VCC_PIN, LOW)//PNP
#define GpsOFF        digitalWrite(GPS_VCC_PIN, HIGH)

#define GPS_PMTK_STANDBY_REQ "$PMTK161,0*28"
//https://docs.rs-online.com/f450/0900766b8147dbe0.pdf
//https://www.mouser.com/datasheet/2/1052/Quectel_L80-R_GPS_Specification_V1.2-1830014.pdf
enum gps_pmtk_fr_modes {
  GPS_MODE_NORMAL,
  GPS_MODE_FITNESS,
  GPS_MODE_AVIATION,
  GPS_MODE_BALLOON
};

#define GPS_PMTK_FR_MODE_NORMAL_REQ     "$PMTK886,0*28"
#define GPS_PMTK_FR_MODE_FITNESS_REQ    "$PMTK886,1*29"
#define GPS_PMTK_FR_MODE_AVIATION_REQ   "$PMTK886,2*2A"
#define GPS_PMTK_FR_MODE_BALLOON_REQ    "$PMTK886,3*2B"

#define GPS_PMTK_FR_MODE_RESPONSE       "$PMTK001,886,3*36"

extern TinyGPSPlus gps;

void setupGPS();
bool tryGPSFix(int timeout);
void updateGPSData();
void sync_system_time_with_GPS();
void printGPSData();
uint8_t getRegionByLocation(double gps_lat, double gps_long);
void setupGPSMode(int gpsMode);
bool changeGPSMode(u_int8_t cmdType);
double getSpeed();
String getSpeedUnit();
double getAltitude();
String getAltitudeUnit();
void displayGPSInfo();

typedef enum {
  no_region,  
  north_america,
  south_america,
  europe,
  asia,
  oceania
} region;

#endif