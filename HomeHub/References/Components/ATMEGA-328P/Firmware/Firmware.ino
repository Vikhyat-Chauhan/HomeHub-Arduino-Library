//#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <TimerOne.h>
#include <MsTimer2.h>
#include <DS3231.h>
#include <DHT.h> 

#define DHTTYPE DHT11   // DHT 22  (AM2302)
#define rom_address 0x57

#define serialport Serial
SoftwareSerial mySerial(3, 2); // RX, TX

int temp = 1;
String command = "";
String databuffer = "";
char result = 'x'; // Stores last transission result to sync between readvalues and value alter serial terminal usage
int counter = 0;

String output = "";

//Flags
boolean initiateAP_flag = false;
boolean sync_flag = false;
boolean auto_save_flag = false;
boolean automatic_response_flag = true;

//function activating variables
int ONTEMPFEATURE[8];
int OFFTEMPFEATURE[8];
int ONTIMEFEATURE[8];
int OFFTIMEFEATURE[8];
int ONMOTIONFEATURE = 1;
int ALLONMOTIONFEATURE = 1;
int OFFMOTIONFEATURE = 1;

//Trigger Values
int OnDeviceTempValue[8];
int OffDeviceTempValue[8];
int OnDeviceHourValue[8];
int OnDeviceMinuValue[8];
int OffDeviceHourValue[8];
int OffDeviceMinuValue[8];
int DEVICESELECT;

// Variable for external buttons long press
int button_type;
const int button_number = 4;
const int button_pin[button_number+1] = {NULL,4,5,6,7};
int button_press_counter = 0;
bool button_previous_pressed[button_number+1] = {NULL,false,false,false,false};
int button_previous_state[button_number+1] = {NULL,0,0,0,0};
unsigned long button_pressed_millis = 0;

//Relay Sensor variables
const int relay_number = 4;
boolean relay_sleep = false;
int relay_memspace[relay_number+1] = {NULL,10,11,12,13};
int relay_pin[relay_number+1] = {NULL,8,9,10,11};
byte relay_current_state[relay_number+1];
byte relay_previous_state[relay_number+1];
boolean relay_change[relay_number+1] = {NULL,false,false,false,false};

//Temperature Sensor related Struct
byte tsensor_pin = 14;
int tsensor_temp,tsensor_humi,tsensor_error_code;
boolean tsensor_change = true;
boolean tsensor_error;
byte tsensor_memspace[5] = {NULL,250,251,252,253};

//Time Sensor Variables
int timesensor_date,timesensor_month,timesensor_year,timesensor_week,timesensor_hour,timesensor_minute,timesensor_second,timesensor_temp,timesensor_error_code;
bool timesensor_h12 = false;
bool timesensor_century = false;
bool timesensor_pmam;
boolean timesensor_error;
boolean timesensor_set = 1;
byte timesensor_memspace[3] = {NULL,260,261};

DS3231 Clock;
DHT dht(tsensor_pin, DHTTYPE);

void setup() {
  //Read the saved device items
  // Start the I2C interface
  Wire.begin();
  dht.begin();
  Serial.begin(9600);         // start serial for debug
  //Timer for bluetooth data
  Timer1.initialize(100);                     //100 - 1000
  Timer1.attachInterrupt(readValues);
  MsTimer2::set(100, buttonCheck); // 500ms period
  MsTimer2::start();
  readEEPROM();
  relay_initilizer();
  //serialport.println("Initilizing HID");Serial.println("Initilizing HID");
  for(int i=1;i<=relay_number;i++){
    pinMode(relay_pin[i],OUTPUT);
  }
  for(int i=1;i<=button_number;i++){
    pinMode(button_pin[i],INPUT_PULLUP);
    button_previous_state[i] = digitalRead(button_pin[i]);
  }
}

void loop(){
  auto_save();
  tsensor();
  timesensor();
  relay_initilizer();
  automatic_response();
  send_packet();
  ontempFeature();
  offtempFeature();
  ontimeFeature();
  offtimeFeature();
}

void auto_save(){
  if(auto_save_flag == true){
    rom_write(20+DEVICESELECT,OnDeviceTempValue[DEVICESELECT]);
    rom_write(30+DEVICESELECT,OffDeviceTempValue[DEVICESELECT]);
    rom_write(40+DEVICESELECT,OnDeviceHourValue[DEVICESELECT]);
    rom_write(50+DEVICESELECT,OnDeviceMinuValue[DEVICESELECT]);
    rom_write(60+DEVICESELECT,OffDeviceHourValue[DEVICESELECT]);
    rom_write(70+DEVICESELECT,OffDeviceMinuValue[DEVICESELECT]);
    rom_write(160+DEVICESELECT,ONTEMPFEATURE[DEVICESELECT]);
    rom_write(170+DEVICESELECT,OFFTEMPFEATURE[DEVICESELECT]);
    rom_write(195+DEVICESELECT,ONTIMEFEATURE[DEVICESELECT]);
    rom_write(190+DEVICESELECT,OFFTIMEFEATURE[DEVICESELECT]);
    rom_write(200,DEVICESELECT);
    rom_write(301,button_type); 
    auto_save_flag = false;
  }
}

void ontempFeature(){
  for(int i =1;i<=relay_number;i++){
    if(tsensor_temp != 0){
      if(!ONTEMPFEATURE[i]){
        if(tsensor_temp == OnDeviceTempValue[i]){
          relay_current_state[i] = 0; 
        }
      }
    }
  }
}  

void offtempFeature(){
  for(int i =1;i<=relay_number;i++){
    if(tsensor_error != 0){
    if(!OFFTEMPFEATURE[i]){
    if(tsensor_temp == OffDeviceTempValue[i]){
        relay_current_state[i] = 1; 
    }
   }
  }
  }
}  

void ontimeFeature(){
  for(int i =1;i<=relay_number;i++){
    if(timesensor_error != 0){
    if(!ONTIMEFEATURE[i]){
    if(timesensor_hour == OnDeviceHourValue[i]){
      if(timesensor_minute == OnDeviceMinuValue[i]){
        relay_current_state[i] = 0; 
      }
    }
    }
    }
  }
} 

void offtimeFeature(){
  for(int i =1;i<=relay_number;i++){
    if(timesensor_error != 0){
    if(!OFFTIMEFEATURE[i]){
    if(timesensor_hour == OffDeviceHourValue[i]){
      if(timesensor_minute == OffDeviceMinuValue[i]){
        relay_current_state[i] = 1; 
      }
    }
    }
    }
  }
} 

void automatic_response(){
  //Place Semi Essential responses here, which can be stopped via flag for a single cycle
  if(automatic_response_flag == true){
    for(int i=1;i<=relay_number;i++){
      if(relay_change[i] == true){
        String data = "<DE";
        data = data + String(i) + ">";
        data = data + String(relay_current_state[i]);
        data = data + "X";
        send_data(data);
      }
    }
  }
  else{
    automatic_response_flag = true;
  }
  //Place Essential responses here
  if(initiateAP_flag == true){
    send_data("<WSP>0X");
    initiateAP_flag = false;
  }
  if(sync_flag == true){
    String command = "";
    //Sending Device 1 Status
    for(int i=1;i<=relay_number;i++){
      String command = "<DE";
      command = command + String(i) + ">";
      command = command + String(relay_current_state[i]);
      command = command + "X";
      send_data(command); 
    }
    //Sending Device 2 Status 
    command = "<TEM>"+String(tsensor_temp)+"X";
    send_data(command);
    //Sending Device 2 Status 
    command = "<HUM>"+String(tsensor_temp)+"X";
    send_data(command);
    //Sending Device Status 
    command = "<TEO>"+String(ONTEMPFEATURE[DEVICESELECT])+"X";
    send_data(command);
    //Sending Device Status 
    command = "<TEF>"+String(OFFTEMPFEATURE[DEVICESELECT])+"X";
    send_data(command);
    //Sending Device 2 Status 
    command = "<TIO>"+String(ONTIMEFEATURE[DEVICESELECT])+"X";
    send_data(command);
    //Sending Device 2 Status 
    command = "<TIF>"+String(OFFTIMEFEATURE[DEVICESELECT])+"X";
    send_data(command);

    sync_flag = false;
  }
  if(tsensor_change == 0){
    String data = "<TEM>"+String(tsensor_temp)+"X";
    send_data(data);        
  }
}

void relay_initilizer(){
  if(relay_sleep == false){
  for(int i=1;i<=relay_number;i++){
  //Change has occured in the state
  if(relay_current_state[i] != relay_previous_state[i]){
    relay_change[i] = true;
    //EEPROM.write(relay_memspace[i],relay_current_state[i]);
    rom_write(relay_memspace[i],relay_current_state[i]);
    relay_previous_state[i] = relay_current_state[i];
  }
  else{ //No Change has been encountered
    relay_change[i] = false;
    relay_previous_state[i] = relay_current_state[i];
  }
  digitalWrite(relay_pin[i], relay_current_state[i]);
  }
  }
}

void buttonCheck(){ 
  for(int i=1;i<=button_number;i++){
    int button_state = digitalRead(button_pin[i]);
    if(button_type == 0){
      if(button_state == 0){ //button is pressed 
       if(button_previous_pressed[i]){
         //we will skip it till it's pressed, can change device state here
       }
       else{
         button_previous_pressed[i]= true;
         button_pressed_millis = millis();
       }
      }
     else{
       if(button_previous_pressed[i]){
         if((millis() - button_pressed_millis)> 5000){
           initiateAP_flag = true;  
         }
         else{
           relay_current_state[i] = !relay_current_state[i];                //button his now left after pressing, can change device state here
           digitalWrite(relay_pin[i],relay_current_state[i]);
         }
         button_previous_pressed[i]= false;
      }
      else{
        button_previous_pressed[i]= false;
      //no button pressed for a long time
      }
    }
    }
    else{
      if(button_state != button_previous_state[i]){ //Button state changed
        if(button_press_counter == 0){                    //initiate multiple press counter
          button_pressed_millis = millis();
          button_press_counter += 1;
        }
        else if((button_press_counter >0) && (button_press_counter <7)){//Add counter 
          button_press_counter+=1;
        }
        else if(button_press_counter >= 7){                 //Button press 7 times
          if((millis() - button_pressed_millis) < 3000){//check time it took press 7 buttons, 3 sec here
            initiateAP_flag = true;
          }
          button_press_counter = 0;
        }
        relay_current_state[i] = !relay_current_state[i];                //button his now left after pressing, can change device state here
        digitalWrite(relay_pin[i],relay_current_state[i]);
        button_previous_state[i] = button_state;
      }
   }
  }  
}

void readValues(){ 
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
        sync_flag = true; //will give 0 for connection and 1 for disconnection
        result = 's'; // transmission successfully Read
      }
      else if(command == "<DE1>"){
        automatic_response_flag = false;
        relay_current_state[1] = value.toInt();
        result = 's'; // transmission successfully Read
      }
      else if(command == "<DE2>"){
        automatic_response_flag = false;
        relay_current_state[2] = value.toInt();
        result = 's'; // transmission successfully Read
      }
      else if(command == "<DE3>"){
        automatic_response_flag = false;
        relay_current_state[3] = value.toInt();
        result = 's'; // transmission successfully Read
      }
      else if(command == "<DE4>"){
        automatic_response_flag = false;
        relay_current_state[4] = value.toInt();
        result = 's'; // transmission successfully Read
      }
      else if(command == "<BUT>"){
        button_type = value.toInt();
        auto_save_flag = true;
        result = 's'; // transmission successfully Read
      }
      else if(command == "<WSP>"){
        initiateAP_flag = true; 
        result = 's'; // transmission successfully Read
      }
      else if(command == "<dse>"){
        DEVICESELECT = value.toInt();
        auto_save_flag = true;
        result = 's'; // transmission successfully Read
      }
      else if(command == "<MOO>"){
        ONMOTIONFEATURE = value.toInt();
        auto_save_flag = true;
        result = 's'; // transmission successfully Read
      }
      else if(command == "<MOF>"){
        OFFMOTIONFEATURE = value.toInt();
        auto_save_flag = true;
        result = 's'; // transmission successfully Read
      }
      else if(command == "<TEO>"){
        ONTEMPFEATURE[DEVICESELECT] = value.toInt();
        auto_save_flag = true;
        result = 's'; // transmission successfully Read
      }
      else if(command == "<TEF>"){
        OFFTEMPFEATURE[DEVICESELECT] = value.toInt();
        auto_save_flag = true;
        result = 's'; // transmission successfully Read
      }
      else if(command == "<TIO>"){
        ONTIMEFEATURE[DEVICESELECT] = value.toInt();
        auto_save_flag = true;
        result = 's'; // transmission successfully Read
      }
      else if(command == "<TIF>"){
        OFFTIMEFEATURE[DEVICESELECT] = value.toInt();
        auto_save_flag = true;
        result = 's'; // transmission successfully Read
      }
      //data Values
     else if(command == "<teo>"){
       OnDeviceTempValue[DEVICESELECT]= value.toInt();
       auto_save_flag = true;
       result = 's'; // transmission successfully Read
     }
     else if(command == "<tef>"){
       OffDeviceTempValue[DEVICESELECT]= value.toInt();
       auto_save_flag = true;
       result = 's'; // transmission successfully Read
     }
     //data Values
     else if(command == "<tho>"){
       OnDeviceHourValue[DEVICESELECT]= value.toInt();
       auto_save_flag = true;
       result = 's'; // transmission successfully Read
     }
     else if(command == "<tmo>"){
       OnDeviceMinuValue[DEVICESELECT]= value.toInt();
       auto_save_flag = true;
       result = 's'; // transmission successfully Read
     }
     else if(command == "<thf>"){
       OffDeviceHourValue[DEVICESELECT]= value.toInt();
       auto_save_flag = true;
       result = 's'; // transmission successfully Read
     }
     else if(command == "<tmf>"){
       OffDeviceMinuValue[DEVICESELECT]= value.toInt();
       auto_save_flag = true;
       result = 's'; // transmission successfully Read
     } 
     else{
     //Device Recieved data but not matched
     }
     serialport.flush();
     serialport.print(result);
     command = "";
   }
  } 
}

void readEEPROM(){
  for(int i = 21;i<=20+relay_number;i++){
    OnDeviceTempValue[i-20] = rom_read(i);
  }
  for(int i = 31;i<=30+relay_number;i++){
    OnDeviceTempValue[i-30] = rom_read(i);
  }
  for(int i = 41;i<=40+relay_number;i++){
    OnDeviceHourValue[i-40] = rom_read(i);
  }
  for(int i = 51;i<=50+relay_number;i++){
    OnDeviceMinuValue[i-50] = rom_read(i);
  }
  for(int i = 61;i<=60+relay_number;i++){
    OffDeviceHourValue[i-60] = rom_read(i);
  }
  for(int i = 71;i<=70+relay_number;i++){
    OffDeviceMinuValue[i-70] = rom_read(i);
  }
  for(int i = 161;i<=160+relay_number;i++){
    ONTEMPFEATURE[i-160] = rom_read(i);
  }
  for(int i = 171;i<=170+relay_number;i++){
    OFFTEMPFEATURE[i-170] = rom_read(i);
  }
  for(int i = 196;i<=195+relay_number;i++){
    ONTIMEFEATURE[i-195] = rom_read(i);
  }
  for(int i = 191;i<=190+relay_number;i++){
    OFFTIMEFEATURE[i-190] = rom_read(i);
  }
  //Button type setting
  button_type = rom_read(301);
  //Device Selection
  DEVICESELECT = rom_read(200);
  //For Relay Sensor
  for(int i=1;i<=relay_number;i++){
    relay_current_state[i] = rom_read(relay_memspace[i]);//EEPROM.read(relay_memspace[i]);
    relay_previous_state[i] = relay_current_state[i];
  }
  //For Temperature Sensor
  /*tsensor_temp = EEPROM.read(tsensor_memspace[1]); 
  tsensor_humi = EEPROM.read(tsensor_memspace[2]);
  tsensor_error = EEPROM.read(tsensor_memspace[3]);
  tsensor_error_code = EEPROM.read(tsensor_memspace[4]);*/
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
      rom_write(tsensor_memspace[1],temp);//EEPROM.write(tsensor_memspace[1],temp);
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
    rom_write(tsensor_memspace[3],tsensor_error);//EEPROM.write(tsensor_memspace[3],tsensor_error);
    rom_write(tsensor_memspace[4],tsensor_error_code);//EEPROM.write(tsensor_memspace[4],tsensor_error_code);
  }
  //Humidity stability function
  if(0<=humi && humi<=100){ 
    tsensor_error = 1;
    if(humi != tsensor_humi){
      tsensor_change = 0;
      rom_write(tsensor_memspace[2],humi);//EEPROM.write(tsensor_memspace[2],humi);
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
    rom_write(tsensor_memspace[3],tsensor_error);//EEPROM.write(tsensor_memspace[3],tsensor_error);
    rom_write(tsensor_memspace[4],tsensor_error_code);//EEPROM.write(tsensor_memspace[4],tsensor_error_code);
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
    }/*
    if(EEPROM.read(timesensor_memspace[1]) != timesensor_error){
        EEPROM.write(timesensor_memspace[1],timesensor_error);
      } 
    if(EEPROM.read(timesensor_memspace[2]) !=  timesensor_error_code){
        EEPROM.write(timesensor_memspace[2],timesensor_error_code);
      } */
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

//Custom EEPROM replacement functions
void rom_write(unsigned int eeaddress, byte data){
  int rdata = data;
  Wire.beginTransmission(rom_address);
  Wire.write((int)(eeaddress >> 8)); // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.write(rdata);
  Wire.endTransmission(); 
  delay(10); //Delay introduction solved the data save unreliability
}

//Custom EEPROM replacement functions
byte rom_read(unsigned int eeaddress){
  byte rdata = 0xFF;
  Wire.beginTransmission(rom_address);
  Wire.write((int)(eeaddress >> 8)); // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();
  Wire.requestFrom(rom_address,1);
  if (Wire.available()) rdata = Wire.read();
  return rdata;
}
