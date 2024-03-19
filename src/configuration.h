#ifndef configuration_H
#define configuration_H
#include <Arduino.h>

enum device_modes {
  mode_tracker = 1,
  mode_igate = 2,
  mode_digi = 3,
};

class ConfigurationCommon {
public:
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
  ConfigurationCommon() : debug(false), deviceMode(1), metricSystem(1), tempSensorCorrection(-5.0), humiditySensorCorrection(20){
  }
  bool              debug;
  int               deviceMode;
  bool              metricSystem;
  float             tempSensorCorrection;
  float             humiditySensorCorrection;
  LoRa              lora;
};

class ConfigurationTracker {
public:
  class Beacon {
  public:
    Beacon() : callsign("NOCALL"), symbolCode("["), symbolTable("/"), path("WIDE1-1"), interval(30), gpsMode(0), aprsComment(""), statusMessage(""){
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
    IGate() : callsign("NOCALL"), aprsComment("LoRa APRS iGate"), useGPS(true), latitude(0.0), longitude(0.0) {
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
    Digi() : callsign("NOCALL"), aprsComment("LoRa APRS iGate"), useGPS(true), latitude(0.0), longitude(0.0) {
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


private:
  const String mFilePath;
};

#endif
