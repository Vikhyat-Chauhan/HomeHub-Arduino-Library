#include <EEPROM.h>
#include <ArduinoJson.h>
#include <DHT.h>

#define DATA_PORT Serial
#define DATA_PORT_BAUD 9600

#define ALL_RELAY_NUMBER 4
#define ALL_FAN_NUMBER 0
#define ALL_SENSOR_NUMBER 2
#define ALL_RELAY_MEMSPACE 10

int relay_pin[ALL_RELAY_NUMBER] = {8,9,10,11};

#define serialport Serial

SoftwareSerial mySerial(3, 2); // RX, TX
    
typedef struct{
    bool state;
    unsigned long value;
    boolean sleep = false;
    int memspace;
    int pin;
    byte current_state;
    byte previous_state;
    boolean change= false;
}RELAY;

typedef struct{
    bool state;
    unsigned long value;
    boolean change= false;
}FAN;

typedef struct{
    const char* type;
    unsigned long value;
    boolean change= false;
}SENSOR;

typedef struct{
    const char* NAME;
    unsigned int RELAY_NUMBER = ALL_RELAY_NUMBER;
    unsigned int FAN_NUMBER = ALL_FAN_NUMBER;
    unsigned int SENSOR_NUMBER = ALL_SENSOR_NUMBER;
    RELAY relay[ALL_RELAY_NUMBER];
    FAN fan[ALL_FAN_NUMBER];
    SENSOR sensor[ALL_SENSOR_NUMBER];
}SLAVE;

SLAVE slave;

        
//Flags


void setup() {
  Serial.begin(9600);         // start serial for debug
  //Timer for bluetooth data
  initilise_interrupts();
  initilise_structures();
  initilise_relays();
  read_saved_values();
}

void loop(){
  async_tasks();
}

void async_tasks(){
  /*auto_save();
  temp_sensor_handler();
  time_sensor_handler();
  relay_handler();
  automatic_response();
  send_packet();*/
}

void initilise_structures(){
  //Initilizing relay structures inside slave structure
  for(int i=0;i<ALL_RELAY_NUMBER;i++){
    //put relay pins in relay structures for use throught the program
    slave.relay[i].pin = relay_pin[i];
    //put relay memorys spaces in relay structures for use through the program
    slave.relay[i].memspace = ALL_RELAY_MEMSPACE + i;
    }
}

void read_saved_values(){
  //For Relay Sensor Struct
  for(int i=0;i<slave.RELAY_NUMBER;i++){
    slave.relay[i].current_state = EEPROM.read(slave.relay[i].memspace);
    slave.relay[i].previous_state = slave.relay[i].current_state;
  }
}

void initilise_interrupts(){
  Timer1.initialize(100);                     //100 - 1000
  Timer1.attachInterrupt(master_handler);
  MsTimer2::set(100, button_handler); // 500ms period
  MsTimer2::start();
}

void initilise_relays(){
    for(int i=0;i<slave.RELAY_NUMBER;i++){
        pinMode(relay_pin[i],OUTPUT);
        digitalWrite()
    }
}

void auto_save(){
  if(auto_save_flag == true){
    rom_write(301,button_type);
    auto_save_flag = false;
  }
}

void relay_handler(){
  for(int i =0;i<slave.RELAY_NUMBER;i++){
    if(slave.relay[i].sleep == false){
    //Change has occured in the state
      if(slave.relay[i].current_state != slave.relay[i].previous_state){
        slave.relay[i].change = true;
        EEPROM.write(slave.relay[i].memspace,slave.relay[i].current_state);
        slave.relay[i].previous_state = slave.relay[i].current_state;
      }
      else{ //No Change has been encountered
        slave.relay[i].change = false;
        slave.relay[i].previous_state = slave.relay[i].current_state;
      }
    digitalWrite(slave.relay[i].pin, slave.relay[i].current_state);
    }
  }
}


