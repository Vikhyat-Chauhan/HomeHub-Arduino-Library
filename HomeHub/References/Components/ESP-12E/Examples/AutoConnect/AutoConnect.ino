#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <EEPROM.h>
#include <Ticker.h>

WiFiClient espClient;
Ticker ticker;

void setup() {
    // put your setup code here, to run once:
    ticker.attach(0.01, callback);
    Serial.begin(115200);
    EEPROM.begin(512);
    wifi_connect(); 
}

void callback() {
  char Command;
  while (Serial.available() > 0) {
    // get incoming byte:
    Command = Serial.read();
  }
  if(Command == 'i'){
    EEPROM.write(256,0); // Writing the wifi setup feature to activate on reboot
    EEPROM.commit();
    ESP.reset();
  }
  else if(Command == 'r'){
    ESP.reset();
  }
}

void loop() {
  if(WiFi.status() != 3){
    wifi_connect();
  }
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  if(EEPROM.read(256) == 0){
    //Allowing Ap to try connecting 
  }
  else{
    delay(2000);
    ESP.reset(); //No button pressed it will reset and again try connecting to same wifi
      }
  }
  
void wifi_connect(){
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  if(EEPROM.read(256)==0){
    wifiManager.resetSettings();
  }
  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.autoConnect("Configure Wifi");
  Serial.println("connected");
  EEPROM.write(256,1);
  EEPROM.commit();
} 

