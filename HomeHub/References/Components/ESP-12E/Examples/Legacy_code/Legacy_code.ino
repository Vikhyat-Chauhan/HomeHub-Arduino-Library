#include <ESP8266WiFi.h>
#include "SoftwareSerial.h"

#define serialport mySerial

SoftwareSerial mySerial(13, 15, false, 256);

boolean debugging = true;
String command = "";
String databuffer = "";
int counter = 0;

void setup() {
  Serial.begin(115200);
  mySerial.begin(115200);
  Serial.flush();
}


void loop() {
  //send_packet();
  readValues();
}

void readValues(){
  if(serialport.available()){
    char c = serialport.read();
  if(counter == 1){
    command+=c;
  }
  if(c=='<'){
    counter=1;
  }
  if(c=='X'){Serial.println(command);
    counter=0;
    command = "<"+command;
    String variable = command.substring(0,5);
    String value = command.substring(5,command.length()-1);
    //Selected commands only to read by esp offline
    if(variable == "<WIF>"){
      //WIFISETUPFEATURE = value.toInt();
    }
    //For everything that is not for ESP it will be normally sent online 
    else{
    }
  }
  }
  serialport.flush(); 
}

void send_data(String command){
    databuffer += command;
}

void send_packet(){
  int x = databuffer.indexOf('X');
  while( x>0){
  String command = databuffer.substring(0,x+1);
  Serial.flush();
  Serial.print(command); 
  Serial.flush();
  while(!Serial.available()){    
  }
  char c = Serial.read();
  if(c == 's'){
    databuffer = databuffer.substring(x+1,databuffer.length());
    x = databuffer.indexOf('X');
  }
  Serial.flush();
  }
}
