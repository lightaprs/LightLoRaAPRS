#ifndef configuration_H
#define configuration_H
#include <Arduino.h>

enum device_modes {
  mode_tracker = 1,
  mode_igate = 2,
  mode_digi = 3,
};

enum device_models {
  device_lighttracker_plus_1_0 = 0,
  device_lightgateway_plus_1_0 = 1,
  device_lightgateway_1_0 = 2,  
};

class ConfigurationCommon {
public:
  class Solar {
  public:
    Solar() : disable_charging_below_temp(-4), disable_digipeating_above_temp(55.0), disable_digipeating_below_volt(3.5),deep_sleep_below_volt(3.5) {
    }
    float disable_charging_below_temp;
    float disable_digipeating_above_temp;
    float disable_digipeating_below_volt;
    float deep_sleep_below_volt;
  };  
  class Display {
  public:
    Display() : always_on(true),  display_timeout(10), turn_180(false) {
    }
    bool always_on;
    int display_timeout;
    bool turn_180;
  };  
  class LoRa {
  public:
    LoRa() : frequencyRX(433.775), frequencyTX(433.775), power(22), spreadingFactor(12), signalBandwidth(125), codingRate4(5), preambleLength(8), crc(true) {
    }
    float frequencyRX;
    float frequencyTX;
    int  power;
    int  spreadingFactor;
    float signalBandwidth;
    int  codingRate4;
    int  preambleLength;
    bool crc;

  };
  ConfigurationCommon() : debug(false), deviceMode(mode_tracker), deviceModel(device_lighttracker_plus_1_0), metricSystem(1), tempSensorCorrection(0), humiditySensorCorrection(0){
  }
  String            version;
  bool              debug;
  int               deviceMode;
  int               deviceModel;
  bool              metricSystem;
  float             tempSensorCorrection;
  float             humiditySensorCorrection;
  Solar             solar;
  Display           display;  
  LoRa              lora;
};

class ConfigurationTracker {
public:
  class Beacon {
  public:
    Beacon() : callsign("NOCALL-9"), symbolCode("["), symbolTable("/"), path("WIDE1-1"), interval(30), gpsMode(0), aprsComment(""), statusMessage(""){
    } 
    String callsign;
    String symbolCode;
    String symbolTable;
    String path;
    int interval;
    int gpsMode;
    bool sendAltitude;
    bool sendTXCount;
    bool sendTemperature;
    bool sendHumidity;
    bool sendBatteryInfo;
    int sendCommentAfterXBeacons;
    String aprsComment;    
    String statusMessage;
  };

  class SmartBeacon {
  public:
    SmartBeacon() : active(true),  lowSpeed(10), slowRate(300), highSpeed(100), fastRate(60), turnMinAngle(20), turnSlope(180), turnMinTime(30), checkBatteryVoltage(true){
    }
    bool active;
    int lowSpeed;
    int slowRate;
    int highSpeed;
    int fastRate;
    int turnMinAngle;
    int turnSlope;
    int turnMinTime;
    bool checkBatteryVoltage;
  };

  class Power {
  public:
    Power() : deepSleep(false){
    } 
    bool deepSleep;

  };

  ConfigurationTracker() {
  }
  Beacon            beacon;
  SmartBeacon       smartBeacon;
  Power             power;
       
};

class ConfigurationGateway {
public:
  class IGate {
  public:
    IGate() : callsign("NOCALL-10"), aprsComment("LoRa APRS iGate"), useGPS(true), latitude(0.0), longitude(0.0) {
    } 
    String  callsign;
    String aprsComment;
    boolean useGPS;
    int gpsTimeout;
    double latitude;
    double longitude;
    boolean wx;
  };

  class Digi {
  public:
    Digi() : repeatMssgOnly(true), repeatAllPcktsNotConn(true) {
    } 
    boolean repeatMssgOnly;
    boolean repeatAllPcktsNotConn;
  };

  class APRS_IS {
  public:
    APRS_IS() : active(true), server("euro.aprs2.net"), port(14580) {
    }

    bool   active;
    String passcode;
    bool   autoServer;
    String server;
    int    port;
    String filter;
  };

 class Wifi {
  public:
    Wifi() : active(true) {
    }
    bool active;
    String ssid;
    String password;    
  };

  class Static {
  public:
    IPAddress ip;
    IPAddress subnet;
    IPAddress gateway;
    IPAddress dns1;
    IPAddress dns2;
  };

  class Hostname {
  public:
    Hostname() : overwrite(false) {
    }

    bool   overwrite;
    String name;
  };

  class Network {
  public:
    Network() : DHCP(true) {
    }
    bool     DHCP;
    Static   static_;
    Hostname hostname;
  };

  ConfigurationGateway() {
  }
  IGate  igate;
  Digi digi;
  APRS_IS aprs_is;
  Wifi    wifi;
  Network network;
       
};

class ConfigurationRouter {
public:
  class Digi {
  public:
    Digi() : callsign("NOCALL-1"), aprsComment("LoRa APRS iGate"), useGPS(true), latitude(0.0), longitude(0.0) {
    } 
    String  callsign;
    String aprsComment;
    boolean useGPS;
    int gpsTimeout;
    double latitude;
    double longitude;
    boolean sendDigiLoc;
  };

  ConfigurationRouter() {
  }
  Digi  digi;
};

class ConfigurationMessaging {
public:
   class WifiAP {
  public:
    WifiAP() : autoEnableDisable(true) {
    }
    bool autoEnableDisable;
    String ssid;
    String password;    
  };

  ConfigurationMessaging() : active(true), defaultGroup("CQ"),storeMessages(true){
  }
  bool         active;
  String       path;  
  String       defaultGroup;
  bool         storeMessages;
  int          directRXmaxCount;
  int          groupRXmaxCount;
  int          blnRXmaxCount;
  WifiAP       wifi_ap;  
};

//ConfigurationMessaging

class ConfigurationManagement {
public:
  explicit ConfigurationManagement(String FilePath);

  ConfigurationCommon readCommonConfiguration();
  void          writeCommonConfiguration(ConfigurationCommon conf);
  ConfigurationTracker readTrackerConfiguration();
  void          writeTrackerConfiguration(ConfigurationTracker conf);
  ConfigurationGateway readGatewayConfiguration();
  void          writeGatewayConfiguration(ConfigurationGateway conf);
  ConfigurationRouter readRouterConfiguration();
  void          writeRouterConfiguration(ConfigurationRouter conf);
  ConfigurationMessaging readMessagingConfiguration();
  void          writeMessagingConfiguration(ConfigurationMessaging conf);

private:
  const String mFilePath;
};

#endif
