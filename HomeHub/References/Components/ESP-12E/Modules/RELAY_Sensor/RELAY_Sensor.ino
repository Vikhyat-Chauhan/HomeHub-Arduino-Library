/*
 Code Compatible with ESP8266 01
 * Type : TNM ESP8266 Standalone 2 Switch 
 * Special Feature : #Wifi Save Feature, 
 *                   #Wifi change Feature (While disconnected from previour wifi long press pin 2, wait for available open wifi Network connect and then enter new wifi details
 *                   #Alexa Enabled.
 * Author : Vikhyat Chauhan
 *Email : vikhyat.chauhan@gmail.com
 *TNM ESP8266 Standalone 2 Switch code

 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.

 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"
  
*/

//#include <ESP8266WiFi.h>
#include <EEPROM.h>

//Relay Sensor variables
const int relay_number = 2;
int relay_memspace[3] = {NULL,100,101};
int relay_pin[3] = {NULL,12,13};
byte relay_current_state[3];
byte relay_previous_state[3];
boolean relay_change[3] = {NULL,false,false};


void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  //For Device Sensor
  for(int i=1;i<=relay_number;i++){
    pinMode(relay_pin[i],OUTPUT);
  }
  read_EEPROM();
}

void loop() {
  Serial.print(relay_current_state[1]); Serial.print("    ");Serial.println(relay_current_state[2]);
  initilizer();
}

void read_EEPROM(){
  for(int i=1;i<=relay_number;i++){
    relay_current_state[i] = EEPROM.read(relay_memspace[i]);
    relay_previous_state[i] = relay_current_state[i];
  }
}

void initilizer(){
  relay_initilizer();
}

void relay_initilizer(){
  for(int i=1;i<=relay_number;i++){
  //Change has occured in the state
  if(relay_current_state[i] != relay_previous_state[i]){
    relay_change[i] = true;
    EEPROM.write(relay_memspace[i],relay_current_state[i]);
    EEPROM.commit();
    relay_previous_state[i] = relay_current_state[i];
  }
  else{ //No Change has been encountered
    relay_change[i] = false;
    relay_previous_state[i] = relay_current_state[i];
  }
  digitalWrite(relay_pin[i], relay_current_state[i]);
  }
}

