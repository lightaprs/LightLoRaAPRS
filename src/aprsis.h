#ifndef aprsis_H
#define aprsis_H
#include <APRS-IS.h>

extern APRS_IS aprs_is;
static String aprs_is_server;

void setup_APRS_IS();
unsigned int aprspass(const char *callsign);
void sendDataToAPRSIS(String message);
void checkAPRS_ISConnection();
bool hasLostConnection();
#endif