#include <Arduino.h>
#include <esp_heap_caps.h>
#include "util.h"
#include "SHTC3Sensor.h"
#include "logger.h"
#include "variant.h"
#include <Math.h>

extern logging::Logger logger;
bool gps_fix_search_led = true;

String fillerSpaces (String text, int maxLength) {
  int textSize = text.length();
  String filledText = text;
  if (textSize < maxLength){
    for (size_t i = 0; i < (maxLength-textSize); i++)
    {
      filledText = filledText + " ";
    }    
  }

  return filledText;
}

void gpsSearchLedBlink(){
    if(gps_fix_search_led) {
      ledBlink(true);
      gps_fix_search_led = false;
    } else {
      ledBlink(false);
      gps_fix_search_led = true;
    }    
}

void ledBlink(bool enabled) {
  if(enabled) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}

void printSensorDataReading()
{
  Serial.print(F("Battery Voltage: "));
  Serial.print(readBatteryVoltage());
  Serial.print(F(" Temperature: "));
  Serial.print(getTemperatureC());
  Serial.print(F(", Humidity: %"));
  Serial.println(getHumidity());    
}

void printFreeMEM(){

  uint32_t freeHeapBytes = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
  uint32_t totalHeapBytes = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
  float percentageHeapFree = freeHeapBytes * 100.0f / (float)totalHeapBytes;

  logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "MEMORY", "%.1f%% free - %d of %d bytes free\n", percentageHeapFree, freeHeapBytes, totalHeapBytes);  
}

double readBatteryVoltage() {
  return readVoltage(VOLTAGE_READ_PIN, ADC_MULTIPLIER);
}

double readSolarVoltage() {
  return readVoltage(SOLAR_VOLTAGE_READ_PIN, SOLAR_ADC_MULTIPLIER);
}

double readVoltage(int pin, float multiplier) {

  u_int8_t num_samples = 10;
  int sum = 0;             
  u_int8_t sample_count = 0;

  while (sample_count < num_samples) {
    sum += analogRead(pin);
    sample_count++;
    delay(10);
  }

  int reading_average = sum / num_samples;

  if(reading_average < 150 || reading_average > 4095) return 0;

  float voltage = multiplier * (-0.000000000000016 * pow(reading_average,4) + 0.000000000118171 * pow(reading_average,3)- 0.000000301211691 * pow(reading_average,2)+ 0.001109019271794 * reading_average + 0.034143524634089);

  return voltage;

}

//Haversine based bearing calculation
float calculateBearing(double lat1, double lon1, double lat2, double lon2){ 
  float y = sin(lon2-lon1) * cos(lat2);
  float x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(lon2-lon1);
  float theta = atan2(y, x);
  float bearing = fmod(360, (theta*180/PI + 360));
  
  return bearing;
}

//Source : https://github.com/barry-ha/Griduino/blob/master/grid_helper.h#L22
void calcLocator(char *result, double lat, double lon, int precision) {
    // Converts from lat/long to Maidenhead Grid Locator
    // From: https://ham.stackexchange.com/questions/221/how-can-one-convert-from-lat-long-to-grid-square
    // And:  10-digit calculator, fmaidenhead.php[158], function getGridName($lat,$long)
    /**************************** 6-digit **************************/
    // Input: char result[7];
    int o1, o2, o3, o4;   // l_o_ngitude
    int a1, a2, a3, a4;   // l_a_titude
    //double remainder;

    // longitude
    double r = (lon + 180.0) / 20.0 + 1e-7;
    o1       = (int)r;
    r        = 10.0 * (r - floor(r));
    o2       = (int)r;
    r        = 24.0 * (r - floor(r));
    o3       = (int)r;
    r        = 10.0 * (r - floor(r));
    o4       = (int)r;

    // latitude
    double t = (lat + 90.0) / 10.0 + 1e-7;
    a1       = (int)t;
    t        = 10.0 * (t - floor(t));
    a2       = (int)t;
    t        = 24.0 * (t - floor(t));
    a3       = (int)t;
    t        = 10.0 * (t - floor(t));
    a4       = (int)t;


    result[0] = (char)o1 + 'A';
    result[1] = (char)a1 + 'A';
    result[2] = (char)o2 + '0';
    result[3] = (char)a2 + '0';
    result[4] = (char)0;
    if (precision > 4) {
      result[4] = (char)o3 + 'a';
      result[5] = (char)a3 + 'a';
      result[6] = (char)0;
    }
    if (precision > 6) {
      result[6] = (char)o4 + '0';
      result[7] = (char)a4 + '0';
      result[8] = (char)0;
    }
    return;
}

String encodeHMSTimestamp(int hour, int minute, int second){

  char timestamp_buff[8];

  sprintf(timestamp_buff, "%02d", hour);
  sprintf(timestamp_buff + 2, "%02d", minute);
  sprintf(timestamp_buff + 4, "%02d", second);
  timestamp_buff[6] = 'h';

  String timestamp;
      for (uint8_t i = 0; i < 7; i++)
      {
        timestamp += String((char)timestamp_buff[i]);
      }  

  return timestamp;
}

String createLatForAPRS(double latitude) {

  // Convert and set latitude NMEA string Degree Minute Hundreths of minutes ddmm.hh[S,N].
  char lat_buff[10];
  int temp = 0;
  double dm_lat = 0.0;

  if (latitude < 0.0) {
    temp = -(int)latitude;
    dm_lat = temp * 100.0 - (latitude + temp) * 60.0;
  } else {
    temp = (int)latitude;
    dm_lat = temp * 100 + (latitude - temp) * 60.0;
  }

  dtostrf(dm_lat, 7, 2, lat_buff);

  if (dm_lat < 1000) {
    lat_buff[0] = '0';
  }

  if (latitude >= 0.0) {
    lat_buff[7] = 'N';
  } else {
    lat_buff[7] = 'S';
  }

  String latStr;
      for (uint8_t i = 0; i < 8; i++)
      {
        latStr += String((char)lat_buff[i]);
      }  

  return latStr;

}

String createLongForAPRS(double longitude) {

  // Convert and set longitude NMEA string Degree Minute Hundreths of minutes ddmm.hh[E,W].
  char long_buff[10];
  int temp = 0;
  double dm_lon = 0.0;

  if (longitude < 0.0) {
    temp = -(int)longitude;
    dm_lon = temp * 100.0 - (longitude + temp) * 60.0;
  } else {
    temp = (int)longitude;
    dm_lon = temp * 100 + (longitude - temp) * 60.0;
  }

  dtostrf(dm_lon, 8, 2, long_buff);

  if (dm_lon < 10000) {
    long_buff[0] = '0';
  }
  if (dm_lon < 1000) {
    long_buff[1] = '0';
  }

  if (longitude >= 0.0) {
    long_buff[8] = 'E';
  } else {
    long_buff[8] = 'W';
  }

  String longStr;
      for (uint8_t i = 0; i < 9; i++)
      {
        longStr += String((char)long_buff[i]);
      }  

  return longStr;
}

