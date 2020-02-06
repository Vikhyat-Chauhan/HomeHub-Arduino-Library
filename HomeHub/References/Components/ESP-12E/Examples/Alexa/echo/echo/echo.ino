#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <functional>
#include "switch.h"
#include "UpnpBroadcastResponder.h"
#include "CallbackFunction.h"

// prototypes
boolean connectWifi();

//on/off callbacks 
void Relay1On();
void Relay1Off();
void Relay2On();
void Relay2Off();
void Relay3On();
void Relay3Off();
void Relay4On();
void Relay4Off();

// Change this before you flash
const char* ssid = "OnePlus 3";
const char* password = "0987654321";

boolean wifiConnected = false;

UpnpBroadcastResponder upnpBroadcastResponder;

Switch *relay1 = NULL;
Switch *relay2 = NULL;
Switch *relay3 = NULL;
Switch *relay4 = NULL;

void setup()
{
 pinMode (D1, OUTPUT);
 pinMode (D2, OUTPUT);
 pinMode (D3, OUTPUT);
 pinMode (D4, OUTPUT);
 digitalWrite (D1,HIGH);
 digitalWrite (D2,HIGH);
 digitalWrite (D3,HIGH);
 digitalWrite (D4,HIGH);
 Serial.begin(115200);
   
  // Initialise wifi connection
  wifiConnected = connectWifi();
  
  if(wifiConnected){
    upnpBroadcastResponder.beginUdpMulticast();
    
    // Define your switches here. Max 14
    // Format: Alexa invocation name, local port no, on callback, off callback
    relay1 = new Switch("Relay 1", 80, Relay1On, Relay1Off);
    relay2 = new Switch("Relay 2", 81, Relay2On, Relay2Off);
    relay3 = new Switch("Relay 3", 82, Relay3On, Relay3Off);
    relay4 = new Switch("Relay 4", 83, Relay4On, Relay4Off);

    Serial.println("Adding switches upnp broadcast responder");
    upnpBroadcastResponder.addDevice(*relay1);
    upnpBroadcastResponder.addDevice(*relay2);
    upnpBroadcastResponder.addDevice(*relay3);
    upnpBroadcastResponder.addDevice(*relay4);
  }
}
 
void loop()
{
	 if(wifiConnected){
      upnpBroadcastResponder.serverLoop();
      
      relay1->serverLoop();
      relay2->serverLoop();
      relay3->serverLoop();
      relay4->serverLoop();
	 }
}

void Relay1On() {
    Serial.print("Switch Relay 1 turn on ...");
    digitalWrite (D1,LOW); 
}

void Relay1Off() {
    Serial.print("Switch Relay 1 turn off ...");
    digitalWrite (D1,HIGH); 
}

void Relay2On() {
    Serial.print("Switch Relay 2 turn on ...");
    digitalWrite (D2,LOW); 
}

void Relay2Off() {
    Serial.print("Switch Relay 2 turn off ...");
    digitalWrite(D2,HIGH); 
}
void Relay3On() {
    Serial.print("Switch Relay 3 turn on ...");
    digitalWrite (D3,LOW); 
}

void Relay3Off() {
    Serial.print("Switch Relay 3 turn off ...");
    digitalWrite (D3,HIGH); 
}

void Relay4On() {
    Serial.print("Switch Relay 4 turn on ...");
    digitalWrite (D4,LOW); 
}

void Relay4Off() {
  Serial.print("Switch Relay 4 turn off ...");
  digitalWrite(D4,HIGH); 
}

// connect to wifi – returns true if successful or false if not
boolean connectWifi(){
  boolean state = true;
  int i = 0;
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("Connecting to WiFi");

  // Wait for connection
  Serial.print("Connecting ...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (i > 10){
      state = false;
      break;
    }
    i++;
  }
  
  if (state){
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.println("");
    Serial.println("Connection failed.");
  }
  
  return state;
}
