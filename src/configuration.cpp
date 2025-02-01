#include <SPIFFS.h>
#include <logger.h>
#include <ArduinoJson.h>
#include "configuration.h"
#include "display.h"

extern logging::Logger logger;

ConfigurationManagement::ConfigurationManagement(String FilePath) : mFilePath(FilePath) {
  if (!SPIFFS.begin(true)) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Configuration", "Mounting SPIFFS was not possible. Trying to format SPIFFS...");
    SPIFFS.format();
    if (!SPIFFS.begin()) {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Configuration", "Formatting SPIFFS was not okay!");
    }
  }
}

// cppcheck-suppress unusedFunction
ConfigurationCommon ConfigurationManagement::readCommonConfiguration() {
  File file = SPIFFS.open(mFilePath);
  if (!file) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Configuration", "Failed to open file for reading...");
    return ConfigurationCommon();
  }
  //DynamicJsonDocument  data(2048);
  JsonDocument data;
  DeserializationError error = deserializeJson(data, file);

  if (error) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Configuration", "Failed to read file %s, upload Filesystem Image",mFilePath.c_str());
    show_display_two_lines_big_header("ERROR", "Failed to read configuration file, upload Filesystem Image");
  }
  file.close();

    ConfigurationCommon conf;

    conf.debug                    = data["debug"] | false;
    conf.deviceMode               = data["device_mode"] | 1;
    conf.deviceModel              = data["device_model"] | 0;
    conf.metricSystem             = data["metric_system"] | true;
    conf.tempSensorCorrection     = data["temperature_sensor_correction"] | 0.0;
    conf.humiditySensorCorrection = data["humidity_sensor_correction"] | 0;

    conf.solar.disable_charging_below_temp                = data["solar"]["disable_charging_below_temp"] | -4.0;
    conf.solar.disable_digipeating_above_temp             = data["solar"]["disable_digipeating_above_temp"] | 55.0;
    conf.solar.disable_digipeating_below_volt             = data["solar"]["disable_digipeating_below_volt"] | 3.5;
    conf.solar.deep_sleep_below_volt                      = data["solar"]["deep_sleep_below_volt"] | 3.4;

    conf.display.always_on          = data["display"]["always_on"] | true;
    conf.display.display_timeout    = data["display"]["display_timeout"] | 10;
    conf.display.turn_180           = data["display"]["turn_180"] | false;

    conf.lora.frequencyRX     = data["lora"]["frequency_rx"] | 433.775;
    conf.lora.frequencyTX     = data["lora"]["frequency_tx"] | 433.775;
    conf.lora.power           = data["lora"]["power"] | 22;
    conf.lora.spreadingFactor = data["lora"]["spreading_factor"] | 12;
    conf.lora.signalBandwidth = data["lora"]["signal_bandwidth"] | 125;
    conf.lora.codingRate4     = data["lora"]["coding_rate4"] | 5;
    conf.lora.preambleLength  = data["lora"]["preamble_length"] | 8;
    conf.lora.crc             = data["lora"]["crc"] | true;

    return conf;
}

// cppcheck-suppress unusedFunction
void ConfigurationManagement::writeCommonConfiguration(ConfigurationCommon conf) {
  File file = SPIFFS.open(mFilePath, "w");
  if (!file) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Configuration", "Failed to open file for writing...");
    return;
  }
  //DynamicJsonDocument data(2048);
  JsonDocument data;

    data["debug"]                         = conf.debug;
    data["device_mode"]                   = conf.deviceMode;
    data["device_model"]                  = conf.deviceModel;
    data["metric_system"]                 = conf.metricSystem; 
    data["temperature_sensor_correction"] = conf.tempSensorCorrection;
    data["humidity_sensor_correction"]    = conf.humiditySensorCorrection;

    data["solar"]["disable_charging_below_temp"]                  = conf.solar.disable_charging_below_temp;
    data["solar"]["disable_digipeating_above_temp"]               = conf.solar.disable_digipeating_above_temp;
    data["solar"]["disable_digipeating_below_volt"]               = conf.solar.disable_digipeating_below_volt;
    data["solar"]["deep_sleep_below_volt"]                        = conf.solar.deep_sleep_below_volt;

    data["display"]["always_on"]          = conf.display.always_on;
    data["display"]["display_timeout"]    = conf.display.display_timeout;
    data["display"]["turn_180"]           = conf.display.turn_180;

    data["lora"]["frequency_rx"]          = conf.lora.frequencyRX;
    data["lora"]["frequency_tx"]          = conf.lora.frequencyTX;
    data["lora"]["power"]                 = conf.lora.power;
    data["lora"]["spreading_factor"]      = conf.lora.spreadingFactor;
    data["lora"]["signal_bandwidth"]      = conf.lora.signalBandwidth;
    data["lora"]["preamble_length"]       = conf.lora.preambleLength;
    data["lora"]["coding_rate4"]          = conf.lora.codingRate4;
    data["lora"]["crc"]                   = conf.lora.crc;

  serializeJson(data, file);
  file.close();
}

// cppcheck-suppress unusedFunction
ConfigurationTracker ConfigurationManagement::readTrackerConfiguration() {
  File file = SPIFFS.open(mFilePath);
  if (!file) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Configuration", "Failed to open file for reading...");
    return ConfigurationTracker();
  }
  //DynamicJsonDocument  data(2048);
  JsonDocument data;
  DeserializationError error = deserializeJson(data, file);

  if (error) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Configuration", "Failed to read file %s, upload Filesystem Image",mFilePath.c_str());
    show_display_two_lines_big_header("ERROR", "Failed to read configuration file, upload Filesystem Image");
  }
  file.close();

    ConfigurationTracker conf;

    conf.beacon.callsign                      = data["beacon"]["callsign"].as<String>();
    conf.beacon.symbolCode                    = data["beacon"]["symbol_code"].as<String>();
    conf.beacon.symbolTable                   = data["beacon"]["symbol_table"].as<String>();
    conf.beacon.path                          = data["beacon"]["path"].as<String>();
    conf.beacon.interval                      = data["beacon"]["non_smart_interval"] | 30;
    conf.beacon.gpsMode                       = data["beacon"]["gps_mode"] | 0;
    conf.beacon.sendAltitude                  = data["beacon"]["send_altitude"] | true;
    conf.beacon.sendTXCount                   = data["beacon"]["send_tx_count"] | true;
    conf.beacon.sendTemperature               = data["beacon"]["send_temperature"] | true;
    conf.beacon.sendHumidity                  = data["beacon"]["send_humidity"] | true;
    conf.beacon.sendBatteryInfo               = data["beacon"]["send_battery_info"] | true;
    conf.beacon.sendCommentAfterXBeacons      = data["beacon"]["send_comment_after_x_beacons"] | 10;
    conf.beacon.aprsComment                   = data["beacon"]["aprs_comment"].as<String>();
    conf.beacon.statusMessage                 = data["beacon"]["status_message"].as<String>();    

    conf.smartBeacon.active               = data["smart_beacon"]["active"] | true;
    conf.smartBeacon.lowSpeed             = data["smart_beacon"]["low_speed"] | 5;
    conf.smartBeacon.slowRate             = data["smart_beacon"]["slow_rate"] | 300;
    conf.smartBeacon.highSpeed            = data["smart_beacon"]["high_speed"] | 60;
    conf.smartBeacon.fastRate             = data["smart_beacon"]["fast_rate"] | 60;
    conf.smartBeacon.turnMinAngle         = data["smart_beacon"]["turn_min_angle"] | 20;
    conf.smartBeacon.turnSlope            = data["smart_beacon"]["turn_slope"] | 180;
    conf.smartBeacon.turnMinTime          = data["smart_beacon"]["turn_min_time"] | 30;
    conf.smartBeacon.checkBatteryVoltage  = data["smart_beacon"]["check_battery_voltage"] | true;

    conf.power.deepSleep         = data["power"]["deep_sleep"] | false;

    return conf;
}

// cppcheck-suppress unusedFunction
void ConfigurationManagement::writeTrackerConfiguration(ConfigurationTracker conf) {
  File file = SPIFFS.open(mFilePath, "w");
  if (!file) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Configuration", "Failed to open file for writing...");
    return;
  }
  //DynamicJsonDocument data(2048);
  JsonDocument data;

    data["beacon"]["callsign"]                    = conf.beacon.callsign;
    data["beacon"]["symbol_code"]                 = conf.beacon.symbolCode;
    data["beacon"]["symbol_table"]                = conf.beacon.symbolTable;
    data["beacon"]["path"]                        = conf.beacon.path;
    data["beacon"]["non_smart_interval"]          = conf.beacon.interval;
    data["beacon"]["gps_mode"]                    = conf.beacon.gpsMode;
    data["beacon"]["send_altitude"]               = conf.beacon.sendAltitude;
    data["beacon"]["send_tx_count"]               = conf.beacon.sendTXCount;
    data["beacon"]["send_temperature"]            = conf.beacon.sendTemperature;
    data["beacon"]["send_humidity"]               = conf.beacon.sendHumidity;
    data["beacon"]["send_battery_info"]           = conf.beacon.sendBatteryInfo;
    data["beacon"]["send_comment_after_x_beacons"]= conf.beacon.sendCommentAfterXBeacons;
    data["beacon"]["aprs_comment"]                = conf.beacon.aprsComment;
    data["beacon"]["status_message"]              = conf.beacon.statusMessage;

    data["smart_beacon"]["active"]                  = conf.smartBeacon.active;
    data["smart_beacon"]["low_speed"]               = conf.smartBeacon.lowSpeed;
    data["smart_beacon"]["slow_rate"]               = conf.smartBeacon.slowRate;
    data["smart_beacon"]["high_speed"]              = conf.smartBeacon.highSpeed;
    data["smart_beacon"]["fast_rate"]               = conf.smartBeacon.fastRate;
    data["smart_beacon"]["turn_min_angle"]          = conf.smartBeacon.turnMinAngle;
    data["smart_beacon"]["turn_slope"]              = conf.smartBeacon.turnSlope;
    data["smart_beacon"]["turn_min_time"]           = conf.smartBeacon.turnMinTime;
    data["smart_beacon"]["check_battery_voltage"]   = conf.smartBeacon.checkBatteryVoltage;

    data["power"]["deep_sleep"]        = conf.power.deepSleep;


  serializeJson(data, file);
  file.close();
}

// cppcheck-suppress unusedFunction
ConfigurationGateway ConfigurationManagement::readGatewayConfiguration() {
  File file = SPIFFS.open(mFilePath);
  if (!file) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Configuration", "Failed to open file for reading...");
    return ConfigurationGateway();
  }
  //DynamicJsonDocument  data(2048);
  JsonDocument data;
  DeserializationError error = deserializeJson(data, file);

  if (error) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Configuration", "Failed to read file %s, upload Filesystem Image",mFilePath.c_str());
    show_display_two_lines_big_header("ERROR", "Failed to read configuration file, upload Filesystem Image");
  }
  file.close();

    ConfigurationGateway conf;

    conf.igate.callsign         = data["igate"]["callsign"].as<String>();
    conf.igate.aprsComment      = data["igate"]["aprs_comment"].as<String>();
    conf.igate.useGPS           = data["igate"]["use_gps_if_possible"] | true;
    conf.igate.gpsTimeout       = data["igate"]["gps_timeout"] | 60;    
    conf.igate.latitude         = data["igate"]["latitude"] | 0.0;
    conf.igate.longitude        = data["igate"]["longitude"] | 0.0;
    conf.igate.wx               = data["igate"]["weather_station"] | true;

    conf.digi.repeatMssgOnly          = data["digi"]["repeat_aprs_messages_only"] | true;
    conf.digi.repeatAllPcktsNotConn   = data["digi"]["repeat_all_packets_if_aprsis_not_connected"] | true;

    conf.aprs_is.active         = data["aprs_is"]["active"] | true;
    conf.aprs_is.passcode       = data["aprs_is"]["passcode"].as<String>();
    conf.aprs_is.autoServer     = data["aprs_is"]["auto_server"] | true;
    conf.aprs_is.server         = data["aprs_is"]["server"].as<String>();
    conf.aprs_is.port           = data["aprs_is"]["port"] | 14580;
    conf.aprs_is.filter         = data["aprs_is"]["filter"].as<String>();

    conf.wifi.active            = data["wifi"]["active"]  | true;
    conf.wifi.ssid              = data["wifi"]["ssid"].as<String>();
    conf.wifi.password          = data["wifi"]["password"].as<String>();

    conf.network.DHCP           = data["network"]["dhcp"]  | true;
    conf.network.static_.ip.fromString(data["network"]["static"]["ip"].as<String>());
    conf.network.static_.subnet.fromString(data["network"]["static"]["subnet"].as<String>());
    conf.network.static_.gateway.fromString(data["network"]["static"]["gateway"].as<String>());
    conf.network.static_.dns1.fromString(data["network"]["static"]["dns1"].as<String>());
    conf.network.static_.dns2.fromString(data["network"]["static"]["dns2"].as<String>());

    conf.network.hostname.overwrite = data["network"]["hostname"]["overwrite"] | false;
    conf.network.hostname.name      = data["network"]["hostname"]["name"].as<String>();

    return conf;
}

// cppcheck-suppress unusedFunction
void ConfigurationManagement::writeGatewayConfiguration(ConfigurationGateway conf) {
  File file = SPIFFS.open(mFilePath, "w");
  if (!file) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Configuration", "Failed to open file for writing...");
    return;
  }
  //DynamicJsonDocument data(2048);
  JsonDocument data;

    data["igate"]["callsign"]             = conf.igate.callsign;
    data["igate"]["aprs_comment"]         = conf.igate.aprsComment;
    data["igate"]["use_gps_if_possible"]  = conf.igate.useGPS;
    data["igate"]["gps_timeout"]          = conf.igate.gpsTimeout;
    data["igate"]["latitude"]             = conf.igate.latitude;
    data["igate"]["longitude"]            = conf.igate.longitude;
    data["igate"]["weather_station"]      = conf.igate.wx;

    data["digi"]["repeat_aprs_messages_only"]                   = conf.digi.repeatMssgOnly;
    data["digi"]["repeat_all_packets_if_aprsis_not_connected"]  = conf.digi.repeatAllPcktsNotConn;

    data["aprs_is"]["active"]             = conf.aprs_is.active;    
    data["aprs_is"]["passcode"]           = conf.aprs_is.passcode;
    data["aprs_is"]["auto_server"]        = conf.aprs_is.autoServer;
    data["aprs_is"]["server"]             = conf.aprs_is.server;
    data["aprs_is"]["port"]               = conf.aprs_is.port;
    data["aprs_is"]["filter"]             = conf.aprs_is.filter;

    data["wifi"]["active"]                = conf.wifi.active;
    data["wifi"]["ssid"]                  = conf.wifi.ssid;
    data["wifi"]["password"]              = conf.wifi.password;

    data["network"]["DHCP"]                  = conf.network.DHCP;
    data["network"]["static"]["ip"]          = conf.network.static_.ip.toString();
    data["network"]["static"]["subnet"]      = conf.network.static_.subnet.toString();
    data["network"]["static"]["gateway"]     = conf.network.static_.gateway.toString();
    data["network"]["static"]["dns1"]        = conf.network.static_.dns1.toString();
    data["network"]["static"]["dns2"]        = conf.network.static_.dns2.toString();
    
    data["network"]["hostname"]["overwrite"] = conf.network.hostname.overwrite;
    data["network"]["hostname"]["name"]      = conf.network.hostname.name;    

  serializeJson(data, file);
  file.close();
}

// cppcheck-suppress unusedFunction
ConfigurationRouter ConfigurationManagement::readRouterConfiguration() {
  File file = SPIFFS.open(mFilePath);
  if (!file) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Configuration", "Failed to open file for reading...");
    return ConfigurationRouter();
  }
  //DynamicJsonDocument  data(2048);
  JsonDocument data;
  DeserializationError error = deserializeJson(data, file);

  if (error) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Configuration", "Failed to read file %s, upload Filesystem Image",mFilePath.c_str());
    show_display_two_lines_big_header("ERROR", "Failed to read configuration file, upload Filesystem Image");
  }
  file.close();

    ConfigurationRouter conf;

    conf.digi.callsign         = data["digi"]["callsign"].as<String>();
    conf.digi.aprsComment      = data["digi"]["aprs_comment"].as<String>();
    conf.digi.useGPS           = data["digi"]["use_gps_if_possible"] | false;
    conf.digi.gpsTimeout       = data["digi"]["gps_timeout"] | 60;    
    conf.digi.latitude         = data["digi"]["latitude"] | 0.0;
    conf.digi.longitude        = data["digi"]["longitude"] | 0.0;
    conf.digi.sendDigiLoc      = data["digi"]["send_digi_location"] | true;
    return conf;
}

// cppcheck-suppress unusedFunction
void ConfigurationManagement::writeRouterConfiguration(ConfigurationRouter conf) {
  File file = SPIFFS.open(mFilePath, "w");
  if (!file) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Configuration", "Failed to open file for writing...");
    return;
  }
  //DynamicJsonDocument data(2048);
  JsonDocument data;

    data["digi"]["callsign"]             = conf.digi.callsign;
    data["digi"]["aprs_comment"]         = conf.digi.aprsComment;
    data["digi"]["use_gps_if_possible"]  = conf.digi.useGPS;
    data["digi"]["gps_timeout"]          = conf.digi.gpsTimeout;
    data["digi"]["latitude"]             = conf.digi.latitude;
    data["digi"]["longitude"]            = conf.digi.longitude;
    data["digi"]["send_digi_location"]   = conf.digi.sendDigiLoc;

  serializeJson(data, file);
  file.close();
}

// cppcheck-suppress unusedFunction
ConfigurationMessaging ConfigurationManagement::readMessagingConfiguration() {
  File file = SPIFFS.open(mFilePath);
  if (!file) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Configuration", "Failed to open file for reading...");
    return ConfigurationMessaging();
  }
  //DynamicJsonDocument  data(2048);
  JsonDocument data;
  DeserializationError error = deserializeJson(data, file);

  if (error) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Configuration", "Failed to read file %s, upload Filesystem Image",mFilePath.c_str());
    show_display_two_lines_big_header("ERROR", "Failed to read configuration file, upload Filesystem Image");
  }
  file.close();

    ConfigurationMessaging conf;

    conf.active                  = data["active"] | true;
    conf.path                    = data["path"].as<String>();
    conf.defaultGroup            = data["default_group"].as<String>();
    conf.storeMessages           = data["store_messages"] | true;
    conf.directRXmaxCount        = data["direct_rx_messages_max_store_count"];
    conf.groupRXmaxCount         = data["group_rx_messages_max_store_count"];
    conf.blnRXmaxCount           = data["bln_rx_messages_max_store_count"];

    conf.wifi_ap.autoEnableDisable = data["wifi_ap"]["auto_enable_disable"]  | true;
    conf.wifi_ap.ssid              = data["wifi_ap"]["ssid"].as<String>();
    conf.wifi_ap.password          = data["wifi_ap"]["password"].as<String>();    

    return conf;
}

// cppcheck-suppress unusedFunction
void ConfigurationManagement::writeMessagingConfiguration(ConfigurationMessaging conf) {
  File file = SPIFFS.open(mFilePath, "w");
  if (!file) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Configuration", "Failed to open file for writing...");
    return;
  }
  //DynamicJsonDocument data(2048);
  JsonDocument data;

    data["active"]                                     = conf.active;
    data["path"]                                       = conf.path;
    data["default_group"]                              = conf.defaultGroup;
    data["store_messages"]                             = conf.storeMessages;
    data["direct_rx_messages_max_store_count"]         = conf.directRXmaxCount;
    data["group_rx_messages_max_store_count"]          = conf.groupRXmaxCount;
    data["bln_rx_messages_max_store_count"]            = conf.blnRXmaxCount;

    data["wifi_ap"]["autoEnableDisable"]               = conf.wifi_ap.autoEnableDisable;
    data["wifi_ap"]["ssid"]                            = conf.wifi_ap.ssid;
    data["wifi_ap"]["password"]                        = conf.wifi_ap.password;

  serializeJson(data, file);
  file.close();
}
