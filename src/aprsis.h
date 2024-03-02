#ifndef aprsis_H
#define aprsis_H
#include <APRS-IS.h>

extern APRS_IS aprs_is;
static String aprs_is_server;

void setup_APRS_IS();
void sendDataToAPRSIS(String message);
void refresh_APRS_IS_connection();

#endif