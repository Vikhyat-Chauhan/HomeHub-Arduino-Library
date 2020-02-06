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

#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <DS3231.h>
#include <Wire.h>

int timesensor_date,timesensor_month,timesensor_year,timesensor_week,timesensor_hour,timesensor_minute,timesensor_second,timesensor_temp,timesensor_error_code;
bool timesensor_h12 = false;
bool timesensor_century = false;
bool timesensor_pmam;
boolean timesensor_error;
boolean timesensor_set = 1;
byte timesensor_memspace[3] = {NULL,260,261};

DS3231 Clock;

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  // Start the I2C interface
  Wire.begin();
}

void loop() {
  initilizer();
}

void initilizer(){
  timesensor();
}

void readEEPROM(){
  //For Time Sensor
  timesensor_error = EEPROM.read(timesensor_memspace[1]); // 
  timesensor_error_code = EEPROM.read(timesensor_memspace[2]); // 1: Date 2: Month 3: Year 4: Week 5:HouR 6:Minute 7: Second 8: temperature 9: Mode   
}

void timesensor(){
  if(timesensor_set == 0){
    Clock.setYear(timesensor_year);
    Clock.setMonth(timesensor_month);
    Clock.setDate(timesensor_date);
    Clock.setDoW(timesensor_week);
    Clock.setHour(timesensor_hour);
    Clock.setMinute(timesensor_minute);
    Clock.setSecond(timesensor_second);
  }
  if(timesensor_set == 1){
    timesensor_date = Clock.getDate();
      timesensor_month = Clock.getMonth(timesensor_century);
      timesensor_year = Clock.getYear();
      timesensor_week = Clock.getDoW();
      timesensor_hour = Clock.getHour(timesensor_h12,timesensor_pmam);
      timesensor_minute = Clock.getMinute();
      timesensor_second = Clock.getSecond();
      timesensor_temp = Clock.getTemperature();
      
    // 1: Date 2: Month 3: Year 4: Week 5:HouR 6:Minute 7: Second 8: temperature 9: Mode 10: Oscillator issue
    if((timesensor_date >31)||((timesensor_date <0))){
      timesensor_error_code = 1;
      timesensor_error = 0;
    }
    else if((timesensor_month >12)||((timesensor_month <0))){
      timesensor_error_code = 2;
     timesensor_error = 0;
    }
    else if((timesensor_year > 30)||((timesensor_year < 19))){
      timesensor_error_code = 3;
      timesensor_error = 0;
    }
    else if((timesensor_week >7)||((timesensor_week <0))){
      timesensor_error_code = 4;
      timesensor_error = 0;
    }
    else if((timesensor_hour >24)||((timesensor_hour <0))){
     timesensor_error_code = 5;
      timesensor_error = 0;
    }
    else if((timesensor_minute >60)||((timesensor_minute <0))){
      timesensor_error_code = 6;
      timesensor_error = 0;
    }
    else if((timesensor_second >60)||((timesensor_second <0))){
      timesensor_error_code = 7;
      timesensor_error = 0;
    }
    else if((timesensor_temp >70)||((timesensor_temp <10))){
      timesensor_error_code = 8;
      timesensor_error = 0;
    }
    //else if((Clock.oscillatorCheck())){
    //  Td_sensor.Error_code = 10;
    //  Td_sensor.Error = 0;
    //}
    else{
      timesensor_error_code = 0;
     timesensor_error = 1;
    }
    if(EEPROM.read(timesensor_memspace[1]) != timesensor_error){
        EEPROM.write(timesensor_memspace[1],timesensor_error);
        EEPROM.commit();
      } 
    if(EEPROM.read(timesensor_memspace[2]) !=  timesensor_error_code){
        EEPROM.write(timesensor_memspace[2],timesensor_error_code);
        EEPROM.commit();
      } 
  }
}

