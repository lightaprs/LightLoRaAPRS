#ifndef message_H
#define message_H
#include <Arduino.h>

void setup_Messaging();
boolean sendMessage(String sender, String addressee, String message);
boolean sendBulletinMessage(String sender, String message, String bulletinID);
void checkBootButtonForMessaging();
void enableWebServerMessaging(boolean enable);
void processRXPackets(String callsign, String packet);
void parseDirectMessage(String sender, String callSignNoSSID, String packet);
void parseGroupMessage(String sender, String packet);
void saveMessageDocs();
String getSenderCallsign();
void addToAddresseesList(String addressee);
String getDirectMessagesJson();
String getGroupMessagesJson();
String getBulletinMessagesJson();
String getAddresseesListJson();
bool checkLastMessage(String line);
size_t getUnreadDirectMessageCount();
bool newMessageRecieved();
#endif