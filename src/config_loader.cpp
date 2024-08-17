#include "config_loader.h"
#include "logger.h"
#include "configuration.h"
#include "display.h"

bool invalidConfig = false;
extern logging::Logger logger;

void load_config(){

  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "ConfigMan", "Configurations loading...");
  show_display_print("Load configs...");

  ConfigurationManagement confmgCommon("/common-config.json");
  commonConfig = confmgCommon.readCommonConfiguration();

  if(commonConfig.debug == true){
    logger.setDebugLevel(logging::LoggerLevel::LOGGER_LEVEL_DEBUG);
  } else {
    logger.setDebugLevel(logging::LoggerLevel::LOGGER_LEVEL_INFO);
  }

  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "ConfigMan", "Messaging Configuration Loading..."); 
  ConfigurationManagement confmgMessaging("/messaging-config.json");
  messagingConfig = confmgMessaging.readMessagingConfiguration();

  if (commonConfig.deviceMode < 0 || commonConfig.deviceMode > 3) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "CommonConfig", "Unkown mode... update 'data/common-config.json' via Wi-Fi or "
                "upload it via \"Upload File System image\"!");
    show_display_two_lines_big_header("ERROR", "Unkown mode... Update Common Configuration via Wi-Fi.");
    delay(3000);
    invalidConfig = true;
  }
  if (commonConfig.deviceMode == 1) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "ConfigMan", "Tracker Mod Enabled..."); 
    ConfigurationManagement confmgTracker("/tracker-config.json");
    trackerConfig = confmgTracker.readTrackerConfiguration();
    
    if (trackerConfig.beacon.callsign.indexOf("NOCALL") > -1) {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "TrackerConfig",
                "Invalid CallSign. You have to change your settings in 'data/tracker-config.json' via Wi-Fi AP or "
                "upload it via \"Upload File System image\"!");
      show_display_two_lines_big_header("ERROR", "Invalid (NOCALL) CallSign. Update Tracker Configuration via Wi-Fi AP.");
      delay(3000);                
      invalidConfig = true;
      
    }
  } else if (commonConfig.deviceMode == 2){
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "ConfigMan", "Gateway Mod Enabled..."); 
    ConfigurationManagement confmgGateway("/gateway-config.json");
    gatewayConfig = confmgGateway.readGatewayConfiguration();
    
    if (gatewayConfig.igate.callsign.indexOf("NOCALL") > -1) {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "GatewayConfig",
                "Invalid CallSign. You have to change your settings in 'data/gateway-config.json' via Wi-Fi or "
                "upload it via \"Upload File System image\"!");
      show_display_two_lines_big_header("ERROR", "Invalid (NOCALL) CallSign. Update Gateway Configuration via Wi-Fi AP.");
      delay(3000);          
      invalidConfig = true;

    }

  if (gatewayConfig.aprs_is.passcode.equals("CHANGE_ME")){
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "GatewayConfig",
                "Invalid APRS PassCode. You have to change it in 'data/gateway-config.json' via Wi-Fi AP or "
                "upload it via \"Upload File System image\"!. To generate APRS PassCode checkout: https://apps.magicbug.co.uk/passcode/");
      show_display_two_lines_big_header("ERROR", "Invalid APRS PassCode. Update Gateway Configuration via Wi-Fi AP.");
      delay(3000);
      invalidConfig = true;         
  }

  if (gatewayConfig.wifi.ssid.equals("CHANGE_ME") || gatewayConfig.wifi.password.equals("CHANGE_ME")){
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "GatewayConfig",
                "Invalid Wi-Fi (SSID) Settings. You have to change it in 'data/gateway-config.json' via Wi-Fi AP or "
                "upload it via \"Upload File System image\"!");
      show_display_two_lines_big_header("ERROR", "Invalid Wi-Fi (SSID) Settings. Update Gateway Configuration via Wi-Fi AP.");
      delay(3000);
      invalidConfig = true;         
  }   
        
  } else if (commonConfig.deviceMode == 3){
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "ConfigMan", "Router Mod Enabled..."); 
    ConfigurationManagement confmgGateway("/router-config.json");
    routerConfig = confmgGateway.readRouterConfiguration();
    
    if (routerConfig.digi.callsign.indexOf("NOCALL") > -1) {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "RouterConfig",
                "Invalid CallSign. You have to change your settings in 'data/router-config.json' via Wi-Fi AP or "
                "upload it via \"Upload File System image\"!");
      show_display_two_lines_big_header("ERROR", "Invalid (NOCALL) CallSign. Update Router Configuration via Wi-Fi AP.");
      delay(3000);                   
      invalidConfig = true;
    }   
    
  } 

  if (!invalidConfig) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "ConfigMan", "Configurations loaded!");
    show_display_println("done.");
  }

}

bool isInvalidConfig(){
  return invalidConfig;
}