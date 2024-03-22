#include <TimeLib.h>
#include "gps.h"
#include "logger.h"
#include "util.h"
#include "display.h"
#include "configuration.h"

extern logging::Logger logger;
extern ConfigurationCommon commonConfig;
extern ConfigurationTracker trackerConfig;

HardwareSerial serialGPS(GPS_SERIAL_NUM);

unsigned long gpsLastProcessedTimeStamp = 0;

bool standbyMode = false;

void setupGPS()
{   
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "GPS", "GPS init...");
  pinMode(GPS_VCC_PIN,OUTPUT); 
  GpsON;
  delay(100);
  serialGPS.begin(GPS_BAUDRATE,SERIAL_8N1,GPS_RX_PIN,GPS_TX_PIN);
  gpsLastProcessedTimeStamp = millis();
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "GPS", "GPS init done!");
}

double getSpeed() {
  if(commonConfig.metricSystem) {
      return gps.speed.isValid() ? gps.speed.kmph() : 0;
  } else {
      return gps.speed.isValid() ? gps.speed.mph() : 0; 
  }  
}
String getSpeedUnit() {

  if(commonConfig.metricSystem) {
      return "kmph";
  } else {
      return "mph"; 
  }
}

double getAltitude() {
  if(commonConfig.metricSystem) {
      return gps.altitude.isValid() ? gps.altitude.meters() : 0;
  } else {
      return gps.altitude.isValid() ? gps.altitude.feet() : 0; 
  }  
}
String getAltitudeUnit() {
  if(commonConfig.metricSystem) {
      return "m";
  } else {
      return "ft"; 
  }
}

bool tryGPSFix(int timeout) {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "GPS", "GPS enabled, waiting max %d seconds for a fix...",timeout);
      setupGPS();
      delay(1000);      
      boolean isGPSFixed = false;
      int gpsFixWaitCounter = 0;
      while (!isGPSFixed && gpsFixWaitCounter < timeout)
      {
        updateGPSData();
        displayGPSInfo();
        printGPSData();  
        isGPSFixed = gps.location.isValid() && gps.location.lat() != 0.0 && gps.date.isValid() && gps.time.isValid() && gps.satellites.value() > 2;
        ++gpsFixWaitCounter;
        gpsSearchLedBlink();      
        delay(1000);
      }
  return isGPSFixed;
}

void setupGPSMode(int gpsMode) {

  updateGPSData();
  bool mode_changed = false;

  while (!mode_changed)
  {
    mode_changed = changeGPSMode(gpsMode);
  }
  delay(1000);
}

bool changeGPSMode(u_int8_t cmdType) {
  bool result = false;

    switch (cmdType)
    {
    case GPS_MODE_NORMAL:
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "GPS", "GPS mode change request: NORMAL");
      serialGPS.println(GPS_PMTK_FR_MODE_NORMAL_REQ);
      show_display_println("GPS Mode:NORMAL");       
      break;
    case GPS_MODE_FITNESS:
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "GPS", "GPS mode change request: FITNESS");
      serialGPS.println(GPS_PMTK_FR_MODE_FITNESS_REQ);
      show_display_println("GPS Mode:FITNESS");
      break;
    case GPS_MODE_AVIATION:
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "GPS", "GPS mode change request: AVIATION");
      serialGPS.println(GPS_PMTK_FR_MODE_AVIATION_REQ);
      show_display_println("GPS Mode:AVIATION");      
      break;
    case GPS_MODE_BALLOON:
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "GPS", "GPS mode change request: BALLOON");
      serialGPS.println(GPS_PMTK_FR_MODE_BALLOON_REQ);
      show_display_println("GPS Mode:BALLOON");         
      break;
    default:
      return false;
      break;
    }
    serialGPS.flush();
    delay (250);
    String str="";
    char c;
    while (serialGPS.available() && !result) {
      c=serialGPS.read();
      str.concat(c);
      if ((str.indexOf(GPS_PMTK_FR_MODE_RESPONSE))!=-1) {
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "GPS", "Mode change acknowledged by Quectel LXX GPS module...");
        result = true;
      }      
    }
    Serial.println(str);
    Serial.println(F(""));    
    
  return result;
}

void displayGPSInfo() {

  String mode;
  if(commonConfig.deviceMode == mode_tracker){
    if(trackerConfig.smartBeacon.active) {
      mode = "Smart";
    } else {
      mode = "Normal";
    }
  } else if(commonConfig.deviceMode == mode_igate) {
    mode = "iGate";
  } else if(commonConfig.deviceMode == mode_digi) {
    mode = "Digi";
  }

  if (trackerConfig.power.deepSleep) {
    mode += " Sleep";
  }
  display_toggle(true);
  show_display_six_lines_big_header("Searching.",
              "GPS Sats: "+String(gps.satellites.value()),
              "GPS Date: " + String(gps.date.month())+"/"+String(gps.date.day())+"/"+String(gps.date.year()),
              "GPS Time: " + String(gps.time.hour())+":"+String(gps.time.minute())+":"+String(gps.time.second()),
              "Lat/Long: " + String(gps.location.lat()) +"/"+String(gps.location.lng()),
              "Mode : "+mode); 
     
}

uint8_t getRegionByLocation(double gps_lat, double gps_long) {

    if (gps_lat == 0.0 || gps_long == 0.0) {
      return no_region;
    } else if(gps_lat > 24 && gps_lat < 72 && gps_long > -180 && gps_long < -50) {
      return north_america;
    } else if (gps_lat > -56 && gps_lat < 25 && gps_long > -92 && gps_long < -34) {
      return south_america;
    } else if (gps_long > -33 && gps_long < 60) {
      return europe;
    } else if (gps_lat > 0 && gps_long > -34 && gps_long < 180) {
      return asia;
    } else if (gps_lat < 0 && gps_long > 111 && gps_long < 180) {
      return oceania;
    } else {
      return no_region;
    }
}

void updateGPSData()
{
    while (serialGPS.available() > 0)
    {
        gps.encode(serialGPS.read());
    }

    if ((millis() - gpsLastProcessedTimeStamp) > 10000 && gps.charsProcessed() < 10)
    {
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "GPS", "No GPS detected: check wiring.");
        show_display("ERROR", "No GPS detected: check wiring."); 
        while (true);
    }
    gpsLastProcessedTimeStamp = millis();

}

void sync_system_time_with_GPS(){

  if (gps.date.isValid() && gps.time.isValid()) {
    // set the Time to the latest GPS reading
    setTime(gps.time.hour(), gps.time.minute(), gps.time.second(), gps.date.day(), gps.date.month(), gps.date.year());
  }

}

void printGPSData()
{

  Serial.print(F("Sats: ")); 
  if (gps.satellites.isValid())
  {
    Serial.print(gps.satellites.value());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F(" Location: ")); 
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F(" Speed: ")); 
  if (gps.speed.isValid())
  {
    Serial.print((int)gps.speed.mph());
  }
  else
  {
    Serial.print(F("INVALID"));
  }  

  Serial.print(F(" Course: ")); 
  if (gps.course.isValid())
  {
    Serial.print((int)gps.course.deg());
  }
  else
  {
    Serial.print(F("INVALID"));
  }  

  Serial.print(F("  Date/Time: "));
  if (gps.date.isValid())
  {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F(" "));
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F("."));
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.print(gps.time.centisecond());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.println();
}