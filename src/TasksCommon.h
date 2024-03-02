#ifndef TasksCommon_H
#define TasksCommon_H

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define WDT_TIMEOUT 300
#define MIN_GPS_SATS 3

//#define TX_MESSAGE_QUEUE_SIZE 10
#define RX_MESSAGE_QUEUE_SIZE 10

void setup_WatchdogTimer();

#endif