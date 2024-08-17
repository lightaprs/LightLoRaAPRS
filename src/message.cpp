#include "message.h"
#include "logger.h"
#include "configuration.h"
#include "display.h"
#include "beacon.h"
#include "util.h"
#include "button.h"
#include "webserver.h"
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <TimeLib.h>

extern logging::Logger logger;
extern ConfigurationMessaging messagingConfig;
extern ConfigurationCommon commonConfig;
extern ConfigurationTracker trackerConfig;
extern ConfigurationGateway gatewayConfig;
extern ConfigurationRouter routerConfig;

boolean messagingServerEnabled = false;

JsonDocument messageDirectDoc;
String storedDirectMessages = "/stored-direct-messages.json";
unsigned long lastTimeDirectMssgDocSaved = 0;
static bool directMssgDocUpdated = false;

JsonDocument messageGroupDoc;
String storedGroupMessages = "/stored-group-messages.json";
unsigned long lastTimeGroupMssgDocSaved = 0;
static bool groupMssgDocUpdated = false;

JsonDocument messageBulletinDoc;
String storedBulletinMessages = "/stored-bulletin-messages.json";
unsigned long lastTimeBulletinMssgDocSaved = 0;
static bool bulletinMssgDocUpdated = false;

JsonDocument addresseesListDoc;
String storedAddresseesList = "/stored-addressees-list.json";
unsigned long lastTimeAddresseesListDocSaved = 0;
static bool addresseesListDocUpdated = false;

#define MESSAGE_DOC_SAVE_INTERVAL 60

unsigned long lastMessageActivity= 0;
#define MESSAGE_WIFI_AP_ACTIVE_TIMEOUT 600

JsonDocument tempLastMessagesDoc;
bool newMessageDisplay = false;

void setup_Messaging() {
  if(messagingConfig.active){

    File file = SPIFFS.open(storedDirectMessages);
    if (!file) {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_WARN, "Messaging", "Failed to open file direct messages for reading...");

    } else {
      DeserializationError error = deserializeJson(messageDirectDoc, file);

      if (error) {
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Messaging", "Failed to read messages file.");
      } 
    }
    file.close();

    file = SPIFFS.open(storedGroupMessages);
    if (!file) {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_WARN, "Messaging", "Failed to open file group messages for reading...");

    } else {
      DeserializationError error = deserializeJson(messageGroupDoc, file);

      if (error) {
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Messaging", "Failed to group messages file.");
      } 
    }
    file.close();

    file = SPIFFS.open(storedBulletinMessages);
    if (!file) {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_WARN, "Messaging", "Failed to open file bulletin messages for reading...");

    } else {
      DeserializationError error = deserializeJson(messageBulletinDoc, file);

      if (error) {
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Messaging", "Failed to bulletin messages file.");
      } 
    }
    file.close();

    file = SPIFFS.open(storedAddresseesList);
    if (!file) {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_WARN, "Messaging", "Failed to open file addresses list for reading...");

    } else {
      DeserializationError error = deserializeJson(addresseesListDoc, file);

      if (error) {
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Messaging", "Failed to addresses list file.");
      } 
    }
    file.close();    

    addToAddresseesList(messagingConfig.defaultGroup);

  }
}

void addToAddresseesList(String addressee){

  bool addresseeExist = false;
  JsonArray list;

  if(addresseesListDoc.isNull()) {
    list = addresseesListDoc.to<JsonArray>();
  } else {
    list = addresseesListDoc.as<JsonArray>();
  }

  for(JsonVariant v : list) {
      String callsign = v.as<String>();
      if(callsign.equals(addressee)) {
        addresseeExist = true;
      }      
  }

  if(!addresseeExist){
    list.add(addressee);
    addresseesListDocUpdated = true;
  }

  if(addresseesListDoc.size()>20){
    addresseesListDoc.remove(0);
  }

  if (addresseesListDoc.overflowed()){
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Messaging", "No enough memory to store the entire addresse list document");
      addresseesListDoc.clear();
    }

    serializeJson(addresseesListDoc, Serial);
    Serial.println("");
    printFreeMEM();    

}

String getDirectMessagesJson() {
  lastMessageActivity = millis();
  String output;
  serializeJson(messageDirectDoc, output);
  messageDirectDoc.clear();
  return output;
}

String getGroupMessagesJson() {
  lastMessageActivity = millis();
  String output;
  serializeJson(messageGroupDoc, output);
  messageGroupDoc.clear();
  return output;
}

String getBulletinMessagesJson() {
  lastMessageActivity = millis();
  String output;
  serializeJson(messageBulletinDoc, output);
  messageBulletinDoc.clear();
  return output;
}

String getAddresseesListJson() {
  lastMessageActivity = millis();
  String output;
  serializeJson(addresseesListDoc, output);
  return output;
}

bool checkLastMessage(String raw){

    Serial.println("---------Last Message--------");
    Serial.print("Raw Data: ");Serial.println(raw);
    Serial.println("-------------------------------");

    bool messageExist = false;
    JsonArray messages = tempLastMessagesDoc.as<JsonArray>();

    for (JsonArray::iterator it=messages.begin(); it!=messages.end(); ++it) {
      if ((*it)["raw"] == raw) {
        messageExist = true;
        Serial.println("Object already exist....");
      }
    }

    if(!messageExist){
      JsonObject messageObject = tempLastMessagesDoc.add<JsonObject>();
      messageObject["raw"] = raw;
      messageObject["timestamp"] = now();
    }

    if(tempLastMessagesDoc.size()>10){
      tempLastMessagesDoc.remove(0);
    }
    
  if (tempLastMessagesDoc.overflowed()){
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Messaging", "No enough memory to store the entire last messages document");
      tempLastMessagesDoc.clear();
    }

    serializeJson(tempLastMessagesDoc, Serial);
    Serial.println("");
    printFreeMEM();       
    
    return messageExist;
}

void parseDirectMessage(String sender, String callSignNoSSID, String packet){

    int firstColonIndex = packet.indexOf(":"+callSignNoSSID);
    int secondColonIndex = packet.indexOf(":", firstColonIndex+1);

    String addressee = packet.substring(firstColonIndex+1,secondColonIndex);
    String message = packet.substring(secondColonIndex+1,packet.length());

    Serial.println("---------Direct Message--------");
    Serial.print("Sender: ");Serial.println(sender);
    Serial.print("Addressee: ");Serial.println(addressee);
    Serial.print("Message: ");Serial.println(message);
    Serial.println("-------------------------------");

    String rawMessage = sender + addressee + message;
    bool messageExist = checkLastMessage(rawMessage);

    if(!messageExist){
      JsonObject messageObject = messageDirectDoc.add<JsonObject>();
    
      messageObject["sender"] = sender;
      messageObject["addressee"] = addressee;
      messageObject["message"] = message;
      messageObject["timestamp"] = now();

      directMssgDocUpdated = true;
      addToAddresseesList(sender);
      enableWebServerMessaging(true);
      lastMessageActivity = millis();
      show_display_three_lines_big_header(sender,sender +" -> "+addressee,message,0);
      newMessageDisplay = true;
    }

   if(messageDirectDoc.size()>messagingConfig.directRXmaxCount){
      messageDirectDoc.remove(0);
    }
    

  if (messageDirectDoc.overflowed()){
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Messaging", "No enough memory to store the entire direct messages document");
      messageDirectDoc.clear();
    }

    serializeJson(messageDirectDoc, Serial);
    Serial.println("");
    printFreeMEM();
     
}

size_t getUnreadDirectMessageCount() {
  return messageDirectDoc.size();
}

bool newMessageRecieved() {
  if(newMessageDisplay) {
    newMessageDisplay = false;
    return true;
  }
  return false;
}

void parseGroupMessage(String sender, String packet){

    int firstColonIndex = packet.indexOf(":"+messagingConfig.defaultGroup);
    int secondColonIndex = packet.indexOf(":", firstColonIndex+1);

    String group = packet.substring(firstColonIndex+1,secondColonIndex);
    String message = packet.substring(secondColonIndex+1,packet.length());

    Serial.println("---------Group Message--------");
    Serial.print("Sender: ");Serial.println(sender);
    Serial.print("Group: ");Serial.println(group);
    Serial.print("Message: ");Serial.println(message);
    Serial.println("-------------------------------");

    String rawMessage = sender + group + message;
    bool messageExist = checkLastMessage(rawMessage);

    if(!messageExist){
      JsonObject messageObject = messageGroupDoc.add<JsonObject>();

      messageObject["sender"] = sender;
      messageObject["group"] = group;
      messageObject["message"] = message;
      messageObject["timestamp"] = now();
      groupMssgDocUpdated = true;
      addToAddresseesList(sender);
      enableWebServerMessaging(true);
      lastMessageActivity = millis();
      show_display_three_lines_big_header(sender,sender +" -> "+group,message,0);
      newMessageDisplay = true;
    }

    if(messageGroupDoc.size()>messagingConfig.groupRXmaxCount){
      messageGroupDoc.remove(0);
    }
    

  if (messageGroupDoc.overflowed()){
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Messaging", "No enough memory to store the entire group messages document");
      messageGroupDoc.clear();
    }

    serializeJson(messageGroupDoc, Serial);
    Serial.println("");
    printFreeMEM();       
    
}

void parseBulletinMessage(String sender, String packet){

    int firstColonIndex = packet.indexOf(":BLN");
    int secondColonIndex = packet.indexOf(":", firstColonIndex+1);

    String bulletinID = packet.substring(firstColonIndex+1,secondColonIndex);
    String message = packet.substring(secondColonIndex+1,packet.length());

    Serial.println("---------BLN Message--------");
    Serial.print("Sender: ");Serial.println(sender);
    Serial.print("BulletinID: ");Serial.println(bulletinID);
    Serial.print("Message: ");Serial.println(message);
    Serial.println("-------------------------------");

    String rawMessage =  bulletinID + message;
    bool messageExist = checkLastMessage(rawMessage);

    if(!messageExist){
      JsonObject messageObject = messageBulletinDoc.add<JsonObject>();

      messageObject["sender"] = sender;
      messageObject["blnid"] = bulletinID;
      messageObject["message"] = message;
      messageObject["timestamp"] = now();
      bulletinMssgDocUpdated = true;
      enableWebServerMessaging(true);
      lastMessageActivity = millis();
      show_display_three_lines_big_header(bulletinID,sender +" -> "+bulletinID,message,0);
      newMessageDisplay = true;      
    }
    
    if(messageBulletinDoc.size()>messagingConfig.blnRXmaxCount){
      messageBulletinDoc.remove(0);
    }
    
  if (messageBulletinDoc.overflowed()){
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Messaging", "No enough memory to store the entire bulletin messages document");
      messageBulletinDoc.clear();
    }

    serializeJson(messageBulletinDoc, Serial);
    Serial.println("");
    printFreeMEM();    

}

void processRXPackets(String callsign, String packet){
  String callSignNoSSID = callsign.substring(0,callsign.indexOf("-"));
  String sender = packet.substring(0,packet.indexOf(">"));

  if(packet.indexOf(":"+callSignNoSSID)> 0){
    parseDirectMessage(sender, callSignNoSSID, packet);
  } else if(packet.indexOf(":"+messagingConfig.defaultGroup) > 0){
    parseGroupMessage(sender, packet);  
  } else if(packet.indexOf(":BLN") > 0){
    parseBulletinMessage(sender, packet);
  } else {
    Serial.println("---------Standard Message--------");
    Serial.println(packet);
    Serial.println("-------------------------------");

    int dataTypeIdentifierIndex = packet.indexOf(":")+1;
    String dataTypeIdentifier = packet.substring(dataTypeIdentifierIndex,dataTypeIdentifierIndex+1);
    bool containsMessagingIdentifier = dataTypeIdentifier.indexOf("=") == 0 || dataTypeIdentifier.indexOf("@") == 0 || dataTypeIdentifier.indexOf(":") ==0;
    if(containsMessagingIdentifier){
      addToAddresseesList(sender);
    }
  }
  saveMessageDocs();
} 

void saveMessageDocs(){

  if(messagingConfig.storeMessages) {

    if(directMssgDocUpdated && (millis() - lastTimeDirectMssgDocSaved) > (MESSAGE_DOC_SAVE_INTERVAL * 1000)) {
        File file = SPIFFS.open(storedDirectMessages, "w");
        if (!file) {
          logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Messaging", "Failed to open direct messages for writing...");
        }
        serializeJson(messageDirectDoc, file);
        file.close();
        directMssgDocUpdated = false;
        lastTimeDirectMssgDocSaved = millis();
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "Messaging", "Direct messages json file saved...");
    }

    if(groupMssgDocUpdated && (millis() - lastTimeGroupMssgDocSaved) > (MESSAGE_DOC_SAVE_INTERVAL * 1000)) {
      File file = SPIFFS.open(storedGroupMessages, "w");
      if (!file) {
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Messaging", "Failed to open group messages for writing...");
      }
      serializeJson(messageGroupDoc, file);
      file.close();
      groupMssgDocUpdated = false;
      lastTimeGroupMssgDocSaved = millis();
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "Messaging", "Group messages json file saved...");
    }    

    if(bulletinMssgDocUpdated && (millis() - lastTimeBulletinMssgDocSaved) > (MESSAGE_DOC_SAVE_INTERVAL * 1000)) {

      File file = SPIFFS.open(storedBulletinMessages, "w");
      if (!file) {
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Messaging", "Failed to open bulletin messages for writing...");
      }
      serializeJson(messageBulletinDoc, file);
      file.close();
      bulletinMssgDocUpdated = false;
      lastTimeBulletinMssgDocSaved = millis();
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "Messaging", "Bulletin messages json file saved...");
    }

    if(addresseesListDocUpdated && (millis() - lastTimeAddresseesListDocSaved) > (MESSAGE_DOC_SAVE_INTERVAL * 1000)) {

      File file = SPIFFS.open(storedAddresseesList, "w");
      if (!file) {
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Messaging", "Failed to open addresses list for writing...");
      }
      serializeJson(addresseesListDoc, file);
      file.close();
      addresseesListDocUpdated = false;
      lastTimeAddresseesListDocSaved = millis();
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "Messaging", "Addresses list json file saved...");
    }

  }

}

void enableWebServerMessaging(boolean enable) {

  if(messagingConfig.active && !messagingServerEnabled && enable){
    Serial.println("Enabling messaging web server....");
    setCpuFrequencyMhz(240);
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "setup_Tracker", "New CPU Frequency: %d", getCpuFrequencyMhz());     
    setup_WebServerForMessaging();
    messagingServerEnabled = true;
  } else if(messagingServerEnabled && !enable) {
    Serial.println("Disabling messaging web server....");    
    stop_WebServer();
    messagingServerEnabled = false;
    setCpuFrequencyMhz(80);
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "setup_Tracker", "New CPU Frequency: %d", getCpuFrequencyMhz());         
  }

}

void checkBootButtonForMessaging(){
  if(messagingConfig.active && !messagingServerEnabled && isButtonPressed()){
    Serial.println("Enabling messaging web server....");
    setup_WebServerForMessaging();
    messagingServerEnabled = true;
  }
  /** 
  else if(messagingServerEnabled && isButtonPressed()) {
    Serial.println("Disabling messaging web server....");
    stop_WebServer();
    messagingServerEnabled = false;
  } */

  if(messagingServerEnabled && ((millis() - lastMessageActivity) > (MESSAGE_WIFI_AP_ACTIVE_TIMEOUT * 1000))) {
    Serial.println("Disabling messaging web server....");
    stop_WebServer();
    messagingServerEnabled = false;
  }  

}

boolean sendMessage(String sender, String addressee, String message){
  millis();
  String aprsMessage="";
  String path = "";
  if (messagingConfig.path != "") {
      path = "," + messagingConfig.path;
  }

  if(message.length()>67) {
    message = message.substring(0,68);
  }

  message.replace("|"," ");
  message.replace("~"," ");
  message.replace("{"," ");

  addressee = fillerSpaces (addressee, 9);
  
  aprsMessage = sender +">APLIGP"+path+"::"+addressee+":"+message;
  Serial.println(aprsMessage);
  boolean result =  messageTX(aprsMessage);
  return result;
}

boolean sendBulletinMessage(String sender, String bulletinID, String message){

  if(!bulletinID.startsWith("BLN")){
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Messaging", "Bulletin identifier must start with BLN "); 
    return false;
  }

  String aprsMessage="";
  String path = "";
  if (messagingConfig.path != "") {
      path = "," + messagingConfig.path;
  }

  if(message.length()>67) {
    message = message.substring(0,67);
  }

  message.replace("|"," ");
  message.replace("~"," ");

  if(bulletinID.length()>9){
    bulletinID = bulletinID.substring(0,9);
  }

  bulletinID = fillerSpaces (bulletinID, 9);

  Serial.println(message);
  
  aprsMessage = sender +">APLIGP"+path+"::"+bulletinID+":"+message;
  Serial.println(aprsMessage);
  boolean result =  messageTX(aprsMessage);

  return result;
}

String getSenderCallsign() {
  String callsign = "N0CALL";
switch (commonConfig.deviceMode) {
  case mode_tracker:
    callsign = trackerConfig.beacon.callsign;
    break;
  case mode_igate:
    callsign = gatewayConfig.igate.callsign;
    break;
  case mode_digi:
    callsign = routerConfig.digi.callsign;
    break;    
  default:
    callsign = "N0CALL";
    break;
}
  return callsign;
}

