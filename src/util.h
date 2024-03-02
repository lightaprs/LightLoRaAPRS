#ifndef util_H
#define util_H

union FloatAsBytes {
    float fval;
    byte bval[4];
};

union UINT16AsBytes {
    uint16_t ival;
    byte bval[2];
};

union INT16AsBytes {
    int16_t ival;
    byte bval[2];
};

void printSensorDataReading();
void printFreeMEM();
void ledBlink(bool enabled);
void gpsSearchLedBlink();
double readBatteryVoltage();
float calculateBearing(double lat1, double lon1, double lat2, double lon2);
void calcLocator(char *result, double lat, double lon, int precision);
String encodeHMSTimestamp(int hour, int minute, int second);
String createLatForAPRS(double latitude);
String createLongForAPRS(double longitude);


#endif