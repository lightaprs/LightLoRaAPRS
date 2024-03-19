#include "beacon.h"
#include "lora.h"
#include "logger.h"
#include "configuration.h"
#include "gps.h"
#include "util.h"
#include "SHTC3Sensor.h"
#include "aprsis.h"
#include "display.h"

extern logging::Logger logger;
extern ConfigurationCommon commonConfig;
extern ConfigurationTracker trackerConfig;
extern ConfigurationGateway gatewayConfig;
extern ConfigurationRouter routerConfig;
extern uint8_t iGateRegion;
LastTXLocation lastTX;
static bool ready_to_sleep = false;
static uint16_t tx_count = 1;

ICACHE_RAM_ATTR void setTXFlag(void) {
  // we sent a packet, set the flag
  setTransmittedFlag(true);
}

ICACHE_RAM_ATTR void setRXFlag(void) {
  // we got a packet, set the flag
  setReceivedFlag(true);
}

//https://gist.github.com/m5mat/3a87b00ac553dc95f41467f80911c0d1
//https://www.w8wjb.com/qth/QTHHelp/English.lproj/adv-smartbeaconing.html
bool smartBeaconDecision() {

  long secs_since_beacon = (millis() - lastTX.millis) / 1000;

  int beacon_rate;
  int speed = gps.speed.isValid() ? (int)gps.speed.kmph() : 0;
  int course = gps.speed.isValid() ? (int)gps.course.deg() : 0;

  int turn_angle = abs(lastTX.heading - course);

  if (speed < trackerConfig.smartBeacon.lowSpeed) {
    beacon_rate = trackerConfig.smartBeacon.slowRate; //Slow Rate Beacon
  } else {// Moving - varies with speed
    if (speed > trackerConfig.smartBeacon.highSpeed) { 
      beacon_rate = trackerConfig.smartBeacon.fastRate; //Fast Rate Beacon
    } else {
      beacon_rate = trackerConfig.smartBeacon.fastRate * trackerConfig.smartBeacon.highSpeed / speed; //Intermediate Beacon Rate
    }
    // Corner pegging - if not stopped
    float turn_threshold = trackerConfig.smartBeacon.turnMinAngle + trackerConfig.smartBeacon.turnSlope / speed;// turn threshold speed-dependent
    if (turn_angle > turn_threshold && secs_since_beacon > trackerConfig.smartBeacon.turnMinTime ) {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "SmartBeacon", "Requesting transmit (due to turn), secs since last beacon: %d seconds", secs_since_beacon);   
      return true;//transmit beacon now 
    }
  }
  
  //if battery is low, force to increase beacon rate.
  if(trackerConfig.smartBeacon.checkBatteryVoltage == true && readBatteryVoltage() < 3.5f){
    beacon_rate = trackerConfig.smartBeacon.slowRate;
  }
  char newGrid6[9];

  calcLocator(newGrid6,gps.location.lat(),gps.location.lng(),8);

  show_display_six_lines_big_header(trackerConfig.beacon.callsign,
              "S:"+String((int)getSpeed()) + getSpeedUnit()+ " A:"+String((int)getAltitude()) + getAltitudeUnit(),
              "Locator: " + String(newGrid6),
              String(readBatteryVoltage()) + "V  T:" + String((int)getTemperature())+String(getTemperatureUnit())+" H:"+String((int)getHumidity())+"%",
              "Beacon Rate: "+ String(beacon_rate)+" secs", 
              "Last TX "+String((int)secs_since_beacon)+" secs ago");

  if (secs_since_beacon > beacon_rate) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "SmartBeacon", "Requesting transmit, secs since last beacon: %d", secs_since_beacon);
    return true;
  }

  return false;
}

int getSmartBeaconDeepSleepRate(double lastTxLat, double lastTxLng, unsigned long lastTxTime) {

  int beacon_rate;
  int speed =0;

  double lastTxdistance = 0;

  updateGPSData();

  if(gps.location.isValid() && gps.location.lat() != 0){
    lastTxdistance = gps.distanceBetween(gps.location.lat(), gps.location.lng(), lastTxLat, lastTxLng); //meters
  }

  double lastTxTimeDelta = (double) (millis() - lastTxTime)/1000;//seconds (average)
  
  speed = (lastTxdistance / lastTxTimeDelta) * 3.6; //kmph

  if (speed < trackerConfig.smartBeacon.lowSpeed) {
    beacon_rate = trackerConfig.smartBeacon.slowRate; //Slow Rate Beacon
  } else {// Moving - varies with speed
    if (speed > trackerConfig.smartBeacon.highSpeed) { 
      beacon_rate = trackerConfig.smartBeacon.fastRate; //Fast Rate Beacon
    } else {
      beacon_rate = trackerConfig.smartBeacon.fastRate * trackerConfig.smartBeacon.highSpeed / speed; //Intermediate Beacon Rate
    }
  }
  
  //if battery is low, force to increase beacon rate.
  if(trackerConfig.smartBeacon.checkBatteryVoltage == true && readBatteryVoltage() < 3.8f){
    beacon_rate = trackerConfig.smartBeacon.slowRate;
  }

  logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "SmartBeacon", "Deep Sleep Beacon Rate: %d", beacon_rate);  
  return beacon_rate;
}

String getRouterLocationAPRSMessage() {

  double routerLat = routerConfig.digi.latitude;
  double routerLong = routerConfig.digi.longitude;

  if(gatewayConfig.igate.useGPS && gps.location.isValid() && gps.satellites.value() > 3){
    routerLat = gps.location.lat();
    routerLong = gps.location.lng();
  }

  String latitude = createLatForAPRS(routerLat);
  String longitude = createLongForAPRS(routerLong);

  String symbolTable = "L";
  String symbolCode = "#";

  String aprsComment = routerConfig.digi.aprsComment;

  String message = routerConfig.digi.callsign +">APLIGP:!"+latitude+symbolTable+longitude+symbolCode+" "+aprsComment;

  return message;
}

String getiGateLocationAPRSMessage() {

  double iGateLat = gatewayConfig.igate.latitude;
  double iGateLong = gatewayConfig.igate.longitude;

  if(gatewayConfig.igate.useGPS && gps.location.isValid() && gps.satellites.value() > 3){
    iGateLat = gps.location.lat();
    iGateLong = gps.location.lng();
  }

  String latitude = createLatForAPRS(iGateLat);
  String longitude = createLongForAPRS(iGateLong);

  String symbolTable = "L";
  String symbolCode = "&";

  if(gatewayConfig.digi.repeatMssgOnly) {
    symbolCode = "a";
  }

  String aprsComment = gatewayConfig.igate.aprsComment;
  String message = gatewayConfig.igate.callsign +">APLIGP:!"+latitude+symbolTable+longitude+symbolCode+" "+aprsComment;

  if(gatewayConfig.igate.wx) {
    symbolCode = "_";

    int temperatureF = (int)getTemperatureF();
    char temperature_buff[4];
    sprintf(temperature_buff,"%03d", temperatureF);

    String temperature_str;
    for (uint8_t i = 0; i < 3; i++)
    {
      temperature_str += String((char)temperature_buff[i]);
    }

    int humidity = (int)getHumidity();
    char humidity_buff[3];
    sprintf(humidity_buff,"%02d", humidity);

    String humidity_str;
    for (uint8_t i = 0; i < 2; i++)
    {
      humidity_str += String((char)humidity_buff[i]);
    }    

    String weatherData = ".../...g...t"+temperature_str+"r...p...P...h"+humidity_str+"b.....";
    message = gatewayConfig.igate.callsign +">APLIGP:!"+latitude+symbolTable+longitude+symbolCode+weatherData+aprsComment;    
  } else {
    aprsComment = gatewayConfig.igate.aprsComment;
    message = gatewayConfig.igate.callsign +">APLIGP:!"+latitude+symbolTable+longitude+symbolCode+" "+aprsComment;
  }

  return message;
}

String getiGateStatusAPRSMessage(){
  
  String message = gatewayConfig.igate.callsign +">APLIGP:>";

  float rxFreq = commonConfig.lora.frequencyRX;
  char rx_freq_buff[7];
  dtostrf(rxFreq, 7, 3, rx_freq_buff);

  String rx_freq_str;
  for (uint8_t i = 0; i < 7; i++)
  {
    rx_freq_str += String((char)rx_freq_buff[i]);
  }    

  if(gatewayConfig.digi.repeatMssgOnly) {
    message += "RX/TX LoRa iGate on "+rx_freq_str+"MHz";
  } else {
    message += "RX Only LoRa iGate on "+rx_freq_str+"MHz";
  }

  if(!gatewayConfig.igate.wx) {
    float temperature = getTemperature();

    char temperature_buff[6];
    dtostrf(temperature, 6, 2, temperature_buff);

    String temperature_str;
    for (uint8_t i = 0; i < 6; i++)
    {
      temperature_str += String((char)temperature_buff[i]);
    }
    
    float humidity = getHumidity();
    char humidity_buff[5];
    dtostrf(humidity, 5, 2, humidity_buff);

    String humidity_str;
    for (uint8_t i = 0; i < 5; i++)
    {
      humidity_str += String((char)humidity_buff[i]);
    }        

    float voltage = readBatteryVoltage();
    char voltage_buff[4];
    dtostrf(voltage, 4, 2, voltage_buff);

    String voltage_str;
    for (uint8_t i = 0; i < 4; i++)
    {
      voltage_str += String((char)voltage_buff[i]);
    }    

    message += " Temp:"+temperature_str+String(getTemperatureUnit()) +", Humid:"+ humidity_str+"%";    
  }

  return message;
}

String getRouterStatusAPRSMessage(){
  
  String message = routerConfig.digi.callsign +">APLIGP:>";

    float temperature = getTemperature();

    char temperature_buff[6];
    dtostrf(temperature, 6, 2, temperature_buff);

    String temperature_str;
    for (uint8_t i = 0; i < 6; i++)
    {
      temperature_str += String((char)temperature_buff[i]);
    }
    
    float humidity = getHumidity();
    char humidity_buff[5];
    dtostrf(humidity, 5, 2, humidity_buff);

    String humidity_str;
    for (uint8_t i = 0; i < 5; i++)
    {
      humidity_str += String((char)humidity_buff[i]);
    }        

    float voltage = readBatteryVoltage();
    char voltage_buff[4];
    dtostrf(voltage, 4, 2, voltage_buff);

    String voltage_str;
    for (uint8_t i = 0; i < 4; i++)
    {
      voltage_str += String((char)voltage_buff[i]);
    }    

    message += "Temp:"+temperature_str+String(getTemperatureUnit()) +", Humid:"+ humidity_str+"%, Batt: "+voltage_str+"V";  

  return message;
}

String getTrackerStatusAPRSMessage(){
  
  String message;
  String path = "";
  if (trackerConfig.beacon.path != "") {
      path = "," + trackerConfig.beacon.path;
  }

  message += trackerConfig.beacon.callsign +">APLIGP"+path+":>"+trackerConfig.beacon.statusMessage;

  return message;
}

String getTrackerLocationAPRSMessage() {
  
  String message;

  //Latitude and Longitude
  String path = "";
  if (trackerConfig.beacon.path != "") {
      path = "," + trackerConfig.beacon.path;
  }
  message += trackerConfig.beacon.callsign +">APLIGP"+path+":/";

  String timestamp = encodeHMSTimestamp(gps.time.hour(), gps.time.minute(), gps.time.second()); //Zulu time DHM timestamp
  message += timestamp;

  String latitude = createLatForAPRS(gps.location.lat());
  String longitude = createLongForAPRS(gps.location.lng());
  message += latitude + trackerConfig.beacon.symbolTable+longitude+trackerConfig.beacon.symbolCode;

  //Course and Speed
  char course_speed_buff[10];

  uint16_t heading = gps.course.isValid() ? (int)gps.course.deg() : 0;
  uint16_t speed = gps.speed.isValid() ? (int)gps.speed.knots() : 0;
  double altitude = gps.altitude.isValid() ? (double)gps.altitude.feet() : 0;

  sprintf(course_speed_buff, "%03d", heading);
  course_speed_buff[3] += '/';
  sprintf(course_speed_buff + 4, "%03d", speed);

  String course_speed_str;
  for (uint8_t i = 0; i <7; i++)
  {
    course_speed_str += String((char)course_speed_buff[i]);
  }
  message += course_speed_str;

  //Altitude
  String alt_str="";
  if(trackerConfig.beacon.sendAltitude){
    char alt_buff[10]; 
    alt_buff[0] = '/';
    alt_buff[1] = 'A';
    alt_buff[2] = '=';

    if (altitude > 0) {
      //for positive values
      sprintf(alt_buff + 3, "%06lu", (long)altitude);
    } else {
      //for negative values
      sprintf(alt_buff + 3, "%06ld", (long)altitude);
    }
    for (uint8_t i = 0; i <9; i++)
    {
      alt_str += String((char)alt_buff[i]);
    }
  }

  message += alt_str;

  //TXCount, Temperature, Humidity and Voltage

    String txCountComment;
    if(trackerConfig.beacon.sendTXCount){

      txCountComment = " TXC:"+String(tx_count);
    } else {
      txCountComment = "";
    }

    String temperatureComment;
    if(trackerConfig.beacon.sendTemperature){
      String temperature_str = "";
      float temperature = getTemperature();
      char temperature_buff[6];
      dtostrf(temperature, 6, 2, temperature_buff);     
      for (uint8_t i = 0; i < 6; i++)
      {
        temperature_str += String((char)temperature_buff[i]);
      }
      temperatureComment = " Temp:"+temperature_str+String(getTemperatureUnit());
    } else {
      temperatureComment = "";
    }

    String humidityComment;
    if(trackerConfig.beacon.sendHumidity){
      String humidity_str ="";
      float humidity = getHumidity();
      char humidity_buff[5];
      dtostrf(humidity, 5, 2, humidity_buff);      
      for (uint8_t i = 0; i < 5; i++)
      {
        humidity_str += String((char)humidity_buff[i]);
      }
      humidityComment =  " Humid:"+ humidity_str+"%";
    } else {
      humidityComment = "";
    }     

    String voltageComment;
    if(trackerConfig.beacon.sendBatteryInfo){
      float voltage = readBatteryVoltage();
      char voltage_buff[3];
      dtostrf(voltage, 3, 2, voltage_buff);

      String voltage_str;
      for (uint8_t i = 0; i < 3; i++)
      {
        voltage_str += String((char)voltage_buff[i]);
      }    
        voltageComment =  " Batt: "+voltage_str+"V";
    } else {
      voltageComment = "";
    }     

    message += txCountComment + temperatureComment + humidityComment + voltageComment;
    
  if(getTXCount() % trackerConfig.beacon.sendCommentAfterXBeacons == 0){
    message += " "+ trackerConfig.beacon.aprsComment;
  }
    
  return message;
}

int getTXCount() {
    return tx_count;
}

void setTXCount(uint16_t txCount){
    tx_count = txCount;
}

void increaseTXCount(){
  ++tx_count;
  if(tx_count == UINT16_MAX) {
    tx_count = 0;
  }

}

void trackerStatusTX() {

  if(getTransmittedFlag()) {
    //reset flag
    setTransmittedFlag(false);

    digitalWrite(LORA_TX_PIN,HIGH);
    digitalWrite(LORA_RX_PIN,LOW);

    int transmissionState = RADIOLIB_ERR_UNKNOWN;
    String aprsMessage;
    String header = "<\xff\x01";//Header for https://github.com/lora-aprs/LoRa_APRS_iGate compatibility

    aprsMessage = getTrackerStatusAPRSMessage();
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "LoRa", "Status Message (%s) transmitting, payload size: %d bytes", aprsMessage.c_str(),aprsMessage.length()+header.length());
  
    Serial.println(aprsMessage);    
    aprsMessage = header + aprsMessage;
    transmissionState = getSX126XRadio().transmit(aprsMessage);

    if (transmissionState == RADIOLIB_ERR_NONE) {
      // packet was successfully sent
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "LoRa", "LoRa transmission completed...");

    } else {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "LoRa", "LoRa transmission failed, code : %d", transmissionState);

    }
    getSX126XRadio().finishTransmit();

    ready_to_sleep = true;

    digitalWrite(LORA_TX_PIN,LOW);
    digitalWrite(LORA_RX_PIN,HIGH);

  }
}

void trackerLocationTX() {

  if(getTransmittedFlag()) {
    //reset flag
    setTransmittedFlag(false);

    digitalWrite(LORA_TX_PIN,HIGH);
    digitalWrite(LORA_RX_PIN,LOW);

    int transmissionState = RADIOLIB_ERR_UNKNOWN;
    String aprsMessage;
    String header = "<\xff\x01";//Header for https://github.com/lora-aprs/LoRa_APRS_iGate compatibility

    aprsMessage = getTrackerLocationAPRSMessage();
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "LoRa", "Location Message (%s) transmitting, payload size: %d bytes", aprsMessage.c_str(),aprsMessage.length()+header.length());

    Serial.println(aprsMessage);    
    aprsMessage = header + aprsMessage;
    transmissionState = getSX126XRadio().transmit(aprsMessage);

    if (transmissionState == RADIOLIB_ERR_NONE) {
      // packet was successfully sent
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "LoRa", "LoRa transmission completed...");
    } else {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "LoRa", "LoRa transmission failed, code : %d", transmissionState);
    }

    lastTX.millis     = millis();
    lastTX.latitude   = (float)gps.location.lat();
    lastTX.longitude  = (float)gps.location.lng();
    lastTX.heading    = gps.course.isValid() ? (int)gps.course.deg() : 0;
    lastTX.speed      = gps.speed.isValid() ? (int)gps.speed.kmph() : 0;
    increaseTXCount();      
    
    getSX126XRadio().finishTransmit();

    ready_to_sleep = true;

    digitalWrite(LORA_TX_PIN,LOW);
    digitalWrite(LORA_RX_PIN,HIGH);

  }
}

void routerTX(String aprsMessage) {

  if(getTransmittedFlag()) {
    //reset flag
    setTransmittedFlag(false);

    getSX126XRadio().clearPacketReceivedAction();//Disable RX interrupt.

    digitalWrite(LORA_TX_PIN,HIGH);
    digitalWrite(LORA_RX_PIN,LOW);  

    String header = "<\xff\x01";//Header for https://github.com/lora-aprs/LoRa_APRS_iGate compatibility

    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "routerTX", "Digipeated APRS message:");         
    Serial.println(aprsMessage);    
    aprsMessage = header + aprsMessage;

    logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "LoRa", "Digi Message transmitting, payload size: %d bytes", aprsMessage.length()); 

    int transmissionState = getSX126XRadio().transmit(aprsMessage);

    if (transmissionState == RADIOLIB_ERR_NONE) {
      // packet was successfully sent
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "LoRa", "LoRa transmission completed...");

      increaseTXCount();

    } else {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "LoRa", "LoRa transmission failed, code : %d", transmissionState);
    }
    getSX126XRadio().finishTransmit();
    setTransmittedFlag(true);

    digitalWrite(LORA_TX_PIN,LOW);
    digitalWrite(LORA_RX_PIN,HIGH);

    delay(100);

    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "LoRa", "Starting to listen LoRa APRS packets again...");
    getSX126XRadio().setPacketReceivedAction(setRXFlag);  

    int state = getSX126XRadio().startReceive();
    if (state == RADIOLIB_ERR_NONE) {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "LoRa", "Success! (Listening LoRa APRS packets now)");
    } else {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "LoRa", "Listening LoRa packets failed, code:  %d", state);      
      while (true);
    }    


  }
}

unsigned long getTrackerLastTXTimeStamp(){
  return lastTX.millis;
}

bool isTrackerReadyToSleep(){
  return ready_to_sleep;
}

void displayRegularBeaconInfo() {

  char newGrid6[9];
  calcLocator(newGrid6,gps.location.lat(),gps.location.lng(),8);
  int nextTX = trackerConfig.beacon.interval - (int)((millis() - lastTX.millis)/1000);

  show_display_six_lines_big_header(trackerConfig.beacon.callsign,
              "S:"+String((int)getSpeed()) + getSpeedUnit()+ " A:"+String((int)getAltitude()) + getAltitudeUnit(),
              "Locator: " + String(newGrid6),
              String(readBatteryVoltage()) + "V  T:" + String((int)getTemperature())+String(getTemperatureUnit())+" H:"+String((int)getHumidity())+"%",
              "GPS Sats: "+String(gps.satellites.value()), 
              "Next TX: "+String(nextTX)+" secs");

}