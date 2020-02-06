#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <TimerOne.h>
#include <MsTimer2.h>

#define serialport Serial
SoftwareSerial mySerial(2, 3); // RX, TX

int temp = 1;
String command = "";
String databuffer = "";
char result = 'x'; // Stores last transission result to sync between readvalues and value alter serial terminal usage
int counter = 0;

void setup() {
  Serial.begin(115200);
  mySerial.begin(115200); 
  //Timer for bluetooth data
  Timer1.initialize(100);                     //100 - 1000
  Timer1.attachInterrupt(readValues);
  MsTimer2::set(100, buttonRead); // 500ms period
  MsTimer2::start();
  delay(100);
  serialport.write('S'); 
}

void loop() { 
  valueInitilizer();
 }  // End loop

char readValues(){ 
  result = 'x';
  while(serialport.available()){
    char c = serialport.read();
  if(counter == 1){
    command+=c;
  }
  if(c=='<'){
    counter=1;
  }
  if(c=='X'){
    counter=0;
    String value = command.substring(4,(command.length())-1);
    command  = '<' +command.substring(0,command.indexOf('>')+1);
      if(command == "<SYN>"){
        //SYNCFEATURE = value.toInt(); //will give 0 for connection and 1 for disconnection
        result = 's'; // transmission successfully Read
      }
      else if(command == "<WIF>"){
        //WIFISETUPFEATURE = value.toInt();
        result = 's'; // transmission successfully Read
      }
      else if(command == "<TEM>"){
      }
      else if(command == "<HUM>"){
      }
      else if(command == "<DE1>"){
        //Device[1] = value.toInt();
        result = 's'; // transmission successfully Read
      }
      else if(command == "<DE2>"){
       //Device[2] = value.toInt();
       result = 's'; // transmission successfully Read
      }
      else if(command == "<DE3>"){
        //Device[3] = value.toInt();
        result = 's'; // transmission successfully Read
      }
      else if(command == "<DE4>"){
        //Device[4] = value.toInt();
        result = 's'; // transmission successfully Read
      }
      serialport.flush();
      serialport.print(result);
      command = "";
  }
  } 
  return result;
}

void buttonRead(){
}

void valueInitilizer(){
  //dataPush();
  if(result!='s'){
    send_packet();
  }
}

void send_data(String command){
    databuffer += command;
}

void send_packet(){
  int x = databuffer.indexOf('X');
  if(x>0){
  String command = databuffer.substring(0,x+1);
  serialport.print(command);
  serialport.flush();
  databuffer = databuffer.substring(x+1,databuffer.length());
  }
}

