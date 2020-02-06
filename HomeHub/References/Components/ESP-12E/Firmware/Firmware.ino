
/*
 Code Compatible with ESP12e
 * Type : TNM 12e Standalone 2 Switch 
 * Special Feature : #Wifi Save Feature, 
 *                   #Wifi change Feature (While disconnected from previour wifi long press pin 2, wait for available open wifi Network connect and then enter new wifi details
 *                   #Alexa Enabled.
 * Author : Vikhyat Chauhan
 * Email : vikhyat.chauhan@gmail.com
 * TNM ESP12e Standalone 2 Switch code

 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.

 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"
*/

/* Removes structure because it was colliding with EEPROM data storage, data was not store reliably
 * Hence, updated temperature structure with global variable based
 * Hence, updated time structure with global variable based
 * 
 *
 */

#define serialport Serial
#define debug_port Serial

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "WiFiManager.h"
#include <EEPROM.h>
#include <Ticker.h>     //Ticker Library
#include <WiFiUdp.h>
#include <DNSServer.h>
#include <functional>

String command = "";
String databuffer = "";
int counter = 0;

//Flags
bool debug_flag = true;

// Global Ap initiating variable
bool initiate_AP = true ; 

bool online_change = false;

unsigned long previous_millis = 0;

//Data Transmission Values
String error = "000";
String LastCommand = "";
String Device_Id_As_Subscription_Topic = (String)ESP.getChipId()+"ESP"; //4Switch 3Dimmer, Temperature Sensor, Motion Sensors available
String Device_Id_As_Publish_Topic = (String)ESP.getChipId()+"/{2S}";
char Device_Id_In_Char_As_Subscription_Topic[12];
char Device_Id_In_Char_As_Publish_Topic[22];

//Update these with values suitable for your network.
const char* mqtt_server = "m12.cloudmqtt.com";
const char* mqtt_clientname;
const char* mqtt_serverid = "wbynzcri";
const char* mqtt_serverpass = "uOIqIxMgf3Dl";
const int mqtt_port = 12233;

//Clients Setup
WiFiClient espClient;
PubSubClient client(espClient);
Ticker serialreader; 

void setup() {
  Serial.begin(9600);
  EEPROM.begin(512);
  serialreader.attach(0.1, initilizer);
  booting_tasks();
  //Temporary solution for wifimanager and foximo clashing 
  if(EEPROM.read(300) == 0){WiFiManager wifiManager;wifiManager.startConfigPortal();debug("Starting AP");}
  Device_Id_As_Publish_Topic.toCharArray(Device_Id_In_Char_As_Publish_Topic, 22);           //Save String Device Id to char array to be able to subscribe to the MQTT Server
  Device_Id_As_Subscription_Topic.toCharArray(Device_Id_In_Char_As_Subscription_Topic, 12); //Save String Device Id to char array to be able to subscribe to the MQTT Server
  mqtt_clientname = Device_Id_In_Char_As_Publish_Topic;
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  //Connecting to wifi
  wifi_connect(); // has internally addititon of alexa switches
}

void callback(char* topic, byte* payload, unsigned int length) {
  String pay = "";
  for (int i = 0; i < length; i++) {
    pay+=(char)payload[i];
  }
  LastCommand = pay;
  debug("Cloud Incomming data : " +pay);
  String variable = pay.substring(0,5);
  String value = pay.substring(5,pay.length()-1);
  if(variable == "<SYN>"){
    
    error = "000"; //Data Recieved and matched
  }
  String command = "<ERR>"+error+"X";
  char ToBeSent[11];
  command.toCharArray(ToBeSent,11);
  client.publish(Device_Id_In_Char_As_Publish_Topic,ToBeSent);
  debug("Published with topic : "+Device_Id_As_Publish_Topic);Serial.println("& Payload : "+command);
}

void loop() {
  if(initiate_AP == true){
    start_AP();
  }
  if(WiFi.status() != 3){
    if((millis() - previous_millis) > 10000){
      previous_millis = millis();
      wifi_connect();
    }
  }else{
  if(!client.connected()){
    reconnect();
  }
  }
}   

void readEEPROM(){
}

void initilizer(){
  client.loop();
  readValues();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    debug("Attempting MQTT connection..."); 
    // Attempt to connect
    if (client.connect(mqtt_clientname,mqtt_serverid, mqtt_serverpass)) {
      // Once connected, publish an announcement...
      client.publish(Device_Id_In_Char_As_Publish_Topic, "<ONL>0X");
      debug("Published to : "+Device_Id_As_Publish_Topic);
      // ... and resubscribe
      client.subscribe(Device_Id_In_Char_As_Subscription_Topic);
      debug(Device_Id_In_Char_As_Subscription_Topic);
    } else {
      debug("failed, rc=");
      debug(String(client.state()));
      debug(" try again in 5 seconds"); 
    }
  }
}

void deviceonline_tasks(){
  //Alexa stuff
    debug("Adding Alexa Virtual Devices");
}

void booting_tasks(){
  //Read the saved device items
  readEEPROM();
  //Syncing start sync request from slave
  Serial.print("<SYN>0X");
}

void start_AP(){
  WiFiManager wifiManager;
  wifiManager.resetSettings();debug("settings reset");
  delay(1000);
  //wifiManager.startConfigPortal();debug("Starting AP");
  initiate_AP = false;
  EEPROM.write(300,0);
  EEPROM.commit();delay(200);
  ESP.reset();
}

void wifi_connect(){
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  bool x = wifiManager.autoConnect();
  if(x == true){
    if(EEPROM.read(300) == 0){
    EEPROM.write(300,1);EEPROM.commit();delay(100);ESP.reset();
    }
    deviceonline_tasks();
  }
} 

void readValues(){
  if(Serial.available()){
    char c = Serial.read();
  if(counter == 1){
    command+=c;
  }
  if(c=='<'){
    counter=1;
  }
  if(c=='X'){
    counter=0;
    command = "<"+command;
    String variable = command.substring(0,5);
    String value = command.substring(5,command.length()-1);
    //Selected commands only to read by esp offline
    if(variable == "<WSP>"){
      initiate_AP = true;
      command = "";
    }
    else if(variable == "<DE1>"){
      if(LastCommand != command){
       char ToBeSent[9];//
        command.toCharArray(ToBeSent,9);//
        client.publish(Device_Id_In_Char_As_Publish_Topic,ToBeSent,true);
      }
      command = "";
    }
    else if(variable == "<DE2>"){
      if(LastCommand != command){
       char ToBeSent[9];//
        command.toCharArray(ToBeSent,9);//
        client.publish(Device_Id_In_Char_As_Publish_Topic,ToBeSent,true);
      }
      command = "";
    }/*
    else if(variable == "<DE3>"){
      relay_previous_state[3] = value.toInt();
      command = "";
    }
    else if(variable == "<DE4>"){
      relay_previous_state[4] = value.toInt();
      command = "";
    }*/
    //For everything that is not for ESP it will be normally sent online 
    else{
    char ToBeSent[9];
    command.toCharArray(ToBeSent,9);
    client.publish(Device_Id_In_Char_As_Publish_Topic,ToBeSent,true);
    command = "";
    }
  }
  }
  Serial.flush(); 
}

void debug(String message){
  if(debug_flag == true){
    debug_port.print("DEBUG : ");
    debug_port.println(message);
  }
}
