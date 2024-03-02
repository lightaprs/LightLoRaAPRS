#include <esp_task_wdt.h>
#include "TasksCommon.h"


void setup_WatchdogTimer(){
    esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
    //esp_task_wdt_add(NULL); //add current thread to WDT watch
}

