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
#define DHTTYPE DHT22   // DHT 22  (AM2302)

#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <DHT.h>


byte tsensor_pin = 2;
int tsensor_temp,tsensor_humi,tsensor_error_code;
boolean tsensor_change = true;
boolean tsensor_error;
byte tsensor_memspace[5] = {NULL,250,251,252,253};


DHT dht(tsensor_pin, DHTTYPE); // Initialize DHT sensor for normal 16mhz Arduino

void setup() {
  dht.begin();
  Serial.begin(115200);
  EEPROM.begin(512);
}

void loop() {
  initilizer();
}

void readEEPROM(){
  //For Temperature Sensor
  tsensor_temp = EEPROM.read(tsensor_memspace[1]); 
  tsensor_humi = EEPROM.read(tsensor_memspace[2]);
  tsensor_error = EEPROM.read(tsensor_memspace[3]);
  tsensor_error_code = EEPROM.read(tsensor_memspace[4]);
}

void initilizer(){
  tsensor();
}

void tsensor(){
  //Read data and store it to variables temp
  int temp = dht.readTemperature();
  int humi = dht.readHumidity();
  bool changed_above = false;
  //Temperature stability function
  if(20<temp && temp<120){ 
    tsensor_error = 1;
    if(temp != tsensor_temp){
      tsensor_change = 0;
      changed_above = true;
      EEPROM.write(tsensor_memspace[1],temp);
      EEPROM.commit();
    }
    else{
      tsensor_change = 1;
    }
   tsensor_temp = temp; 
  }
  else{
    tsensor_temp = 0;
    if(temp<20){
      tsensor_error_code = 00;
    }
    else if(temp>120){
      tsensor_error_code = 01;
    }
    EEPROM.write(tsensor_memspace[3],tsensor_error);
    EEPROM.commit();
    EEPROM.write(tsensor_memspace[4],tsensor_error_code);
    EEPROM.commit();
  }
  //Humidity stability function
  if(0<=humi && humi<=100){ 
    tsensor_error = 1;
    if(humi != tsensor_humi){
      tsensor_change = 0;
      EEPROM.write(tsensor_memspace[2],humi);
      EEPROM.commit();
    }
    else{
      if(changed_above == false){
      tsensor_change = 1;
      }
    }
   tsensor_humi = humi; 
  }
  else{
    tsensor_humi = 0;
    if(humi<0){
      tsensor_error_code = 10;
    }
    else if(humi>100){
      tsensor_error_code = 11;
    }
    EEPROM.write(tsensor_memspace[3],tsensor_error);
    EEPROM.commit();
    EEPROM.write(tsensor_memspace[4],tsensor_error_code);
    EEPROM.commit();
  }
  
  //Send data
  if(tsensor_error == 1){    //No Error
    if(tsensor_change == 0){ //Change observed in sensor
      String data = "<TEM>" + String(tsensor_temp) + "X";//Sending data
      //data = "<HUM>" + String(Th_sensor.Humidity) + "X";//Sending data
      //dataPush(data);
    }
  }
  else{                        // Error observed in data
    //Sending Error code
  }
}

