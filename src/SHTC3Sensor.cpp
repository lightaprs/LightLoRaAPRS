#include "SHTC3Sensor.h"
#include "SparkFun_SHTC3.h"
#include "logger.h"
#include "util.h"
#include "display.h"
#include "configuration.h"

extern logging::Logger logger;
extern ConfigurationCommon commonConfig;

//SHTC3 sensor is not calibrated and readings are a little bit higher due to interal board heat.

SHTC3 mySHTC3;

void setup_SHTC3Sensor()
{
    if(commonConfig.deviceModel == device_lightgateway_1_0) {
        return;
    }

    show_display_print("SHTC3 setup...");
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "SHTC3", "SHTC3 Temp & Humidity Sensor init...");

    SHTC3_Status_TypeDef result = mySHTC3.begin();

    if(result==SHTC3_Status_Nominal){
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "SHTC3", "SHTC3 Temp & Humidity Sensor init done!");
        show_display_println("done.");
    } else {
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "SHTC3", "SHTC3 Temp & Humidity Sensor init failed",result);
        show_display_println("failed.");
    }

}

float getTemperature()
{
    SHTC3_Status_TypeDef result = mySHTC3.update();

    if(result==SHTC3_Status_Nominal){
        if(commonConfig.metricSystem) {
            return getTemperatureC();
        } else {
            return getTemperatureC(); 
        }
                  
    } else {
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "SHTC3", "SHTC3 temperature reading failed",result);    
    }    

    return 0.0f;
}

char getTemperatureUnit() 
{
        if(commonConfig.metricSystem) {
            return 'C';
        } else {
            return 'F'; 
        }
}

float getTemperatureC()
{
    SHTC3_Status_TypeDef result = mySHTC3.update();
    //SHTC3 sensor is not calibrated and readings are a little bit higher due to interal board heat.
    if(result==SHTC3_Status_Nominal){
        return mySHTC3.toDegC() + commonConfig.tempSensorCorrection;          
    } else {
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "SHTC3", "SHTC3 temperature reading failed",result);    
    }    

    return 0.0f;
}

float getTemperatureF()
{
    SHTC3_Status_TypeDef result = mySHTC3.update();
    //SHTC3 sensor is not calibrated and readings are a little bit higher due to interal board heat.
    if(result==SHTC3_Status_Nominal){
        return mySHTC3.toDegF() + (commonConfig.tempSensorCorrection * 1.8);          
    } else {
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "SHTC3", "SHTC3 temperature reading failed",result);    
    }    

    return 0.0f;
}

float getHumidity()
{
    SHTC3_Status_TypeDef result = mySHTC3.update();
    //SHTC3 sensor is not calibrated and readings are a little bit higher due to interal board heat.
    if(result==SHTC3_Status_Nominal){
        return mySHTC3.toPercent() + commonConfig.humiditySensorCorrection;          
    } else {
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "SHTC3", "SHTC3 humidity reading failed",result);    
    }    

    return 0.0f;    
}

void printTempHumidityData(){
    if(commonConfig.metricSystem) {
        Serial.println("Corrected Temp: "+ String(getTemperatureC()) + "C Humiditiy(%): "+ String(getHumidity()));
    } else {
        Serial.println("Corrected Temp: "+ String(getTemperatureF()) + "F Humiditiy:(%) "+ String(getHumidity()));
    }
    

}