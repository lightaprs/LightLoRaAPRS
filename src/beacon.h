#ifndef beacon_H
#define beacon_H
#include <Arduino.h>


void setTXFlag(void);
void setRXFlag(void);
bool smartBeaconDecision();
int getSmartBeaconDeepSleepRate(double lastTxLat, double lastTxLng, unsigned long lastTxTime);
int getTXCount();
void setTXCount(uint16_t txCount);
void increaseTXCount();
String getTrackerStatusAPRSMessage();
String getTrackerLocationAPRSMessage();
String getRouterStatusAPRSMessage();
String getiGateStatusAPRSMessage();
String getiGateLocationAPRSMessage();
String getRouterLocationAPRSMessage();
void trackerLocationTX();
void trackerStatusTX();
boolean messageTX(String aprsMessage);
void routerTX(String aprsMessage);
unsigned long getTrackerLastTXTimeStamp();
bool isTrackerReadyToSleep();
void displayRegularBeaconInfo(unsigned long lastTXTime);

struct LastTXLocation {
   long millis = -300000;    
   float latitude = 0;
   float longitude = 0;
   uint16_t heading = 0;
   uint16_t speed = 0;
};

struct LastRXObject {
   String callsign ="NOCALL";    
   long millis = 0;
   float rssi = 0;
   float snr = 0;
}; 

#endif