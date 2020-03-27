/*
* HomeHub.cpp - An TNM Home Automation Library
* Created by Vikhyat Chauhan @ TNM on 9/11/19
* www.thenextmove.in
* Revision #10 - See readMe
*/

#include "HomeHub.h"
#include <Arduino.h>

WiFiClient _Client;
PubSubClient mqttclient(_Client);
DS3231 Clock;

HomeHub::HomeHub(){
    //Initilize Serial Lines
    HomeHub_DEBUG_PORT.begin(HomeHub_DEBUG_PORT_BAUD);
	HomeHub_SLAVE_DATA_PORT.begin(HomeHub_SLAVE_DATA_PORT_BAUD);
    //Mqtt String to char* Conversions
    Device_Id_As_Publish_Topic.toCharArray(Device_Id_In_Char_As_Publish_Topic, 22);
    Device_Id_As_Subscription_Topic.toCharArray(Device_Id_In_Char_As_Subscription_Topic, 20);
    //Mqtt connection setup
    mqtt_clientname = Device_Id_In_Char_As_Publish_Topic;
    mqttclient.setServer(mqtt_server, mqtt_port);
    mqttclient.setCallback([this] (char* topic, byte* payload, unsigned int length) { this->mqttcallback(topic, payload, length); });
    //Initiate memory system
    initiate_memory();
    //Initilize new Ticker function to run serial slave handler
    ticker = new Ticker();
    ticker->attach_ms(1, std::bind(&HomeHub::salve_serial_json_input_capture, this));
    //Retrieve saved wifi data from Rom
    master.flag.slave_handshake = true;
    retrieve_wifi_data();
	HomeHub_DEBUG_PRINT("STARTED");
}

void HomeHub::asynctasks(){
    //timesensor_handler();
    slave_input_handler();
    //Handling Wifi Setup client and Page requests in each loop/*
    if(master.flag.wifi_setup_webhandler == true){
        _wifi_data = scan_networks();
        wifi_setup_webhandler();
        if(master.flag.saved_wifi_present == true){
            end_wifi_setup();
            master.flag.saved_wifi_present = false;
        }
    }
    //Handling Mqtt looping and wifi connection fallbacks in each loop
    else if(master.flag.mqtt_webhandler == true){
        if(WiFi.status() != 3){
            if((millis() - previous_millis) > 10000){
                previous_millis = millis();
                saved_wifi_connect();
            }
        }
        else{
            if(!mqttclient.connected()){
                initiate_mqtt();
            }
            else{
                mqttclient.loop();
            }
        }
    }
    device_handler(); // Should be only placed at end of each loop since it monitors changes
    slave_output_handler();
    if(mqttclient.connected()){
        mqtt_output_handler();
    }
}

void HomeHub::slave_output_handler(){
    if(master.flag.slave_handshake == false){
        if(master.slave.change == true){
            StaticJsonDocument<600> doc;
            doc["ROLE"] = "MASTER";
            JsonObject device = doc.createNestedObject("DEVICE");
            if(master.slave.all_relay_change == true){
                if((master.slave.all_relay_lastslavecommand == false)||(master.slave.all_relay_lastmqttcommand == true)){
                JsonArray relay = device.createNestedArray("RELAY");
                for(int i=0;i<master.slave.RELAY_NUMBER;i++){
                  JsonObject relay_x = relay.createNestedObject();
                    if(master.slave.relay[i].change == true){
                        if((master.slave.relay[i].lastslavecommand == false)||(master.slave.relay[i].lastmqttcommand == true)){
                            relay_x["STATE"] = master.slave.relay[i].current_state;
                            relay_x["VALUE"] = master.slave.relay[i].current_value;
                        }
                    }
                }
            }
            }
            if(master.slave.all_fan_change == true){
                if((master.slave.all_fan_lastslavecommand == false)||(master.slave.all_fan_lastmqttcommand == true)){
                JsonArray fan = device.createNestedArray("FAN");
                for(int i=0;i<master.slave.FAN_NUMBER;i++){
                  JsonObject fan_x = fan.createNestedObject();
                    if(master.slave.fan[i].change == true){
                        if((master.slave.fan[i].lastslavecommand == false)||(master.slave.fan[i].lastmqttcommand == true)){
                            fan_x["STATE"] = master.slave.fan[i].current_state;
                            fan_x["VALUE"] = master.slave.fan[i].current_value;
                        }
                    }
                }
                }
            }
            if(master.slave.all_sensor_change == true){
                if((master.slave.all_sensor_lastslavecommand == false)||(master.slave.all_sensor_lastmqttcommand == true)){
                JsonArray sensor = device.createNestedArray("SENSOR");
                for(int i=0;i<master.slave.SENSOR_NUMBER;i++){
                  JsonObject sensor_x = sensor.createNestedObject();
                    if(master.slave.sensor[i].change == true){
                        if((master.slave.sensor[i].lastslavecommand == false)||(master.slave.sensor[i].lastmqttcommand == true)){
                            sensor_x["VALUE"] = master.slave.sensor[i].current_value;
                    }
                    }
                }
                }
            }
            //Command
            String command = slave_push_command();
            if(command != "NULL"){
              doc["COMMAND"] = command;
            }
            serializeJson(doc, HomeHub_SLAVE_DATA_PORT);
        }
        
    }
}

String HomeHub::slave_push_command(){
  if(master.flag.initiate_ap == true){
    master.flag.initiate_ap = false;
    String output_command = "WIFI_RESET";
    return output_command;
  }
  else{
    String output_command = "NULL";
    return output_command;
  }
}

void HomeHub::slave_input_handler(){
    //Proceed with receiving handshake or sync input from slave
    if(master.flag.slave_handshake == true){
        if(_slave_handshake_millis == 0.0){
            char* request_handshake = "{\"COMMAND\":\"HANDSHAKE\"}";
            HomeHub_SLAVE_DATA_PORT.print(request_handshake);
            _slave_handshake_millis = millis() + 3000;
        }
        else{
            if(millis() > _slave_handshake_millis){
                if(slave_handshake_handler() == true){
                    master.flag.slave_handshake = false;
                    HomeHub_DEBUG_PRINT("Slave Handshake Complete");
                }
                else{
                    HomeHub_DEBUG_PRINT("No Slave detected, retrying.");
                    _slave_handshake_millis = 0.0;
                }
            }
        }
    }
    //if handshake has be completed only guide to slave syncronisation command handler
    else{
        slave_sync_handler();
    }
}

bool HomeHub::slave_handshake_handler(){
    if(master.flag.received_json == true){
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, _slave_command_buffer);
        JsonObject obj = doc.as<JsonObject>();
        String role = doc["ROLE"];
        JsonArray relays = doc["DEVICE"]["RELAY"];
        JsonArray fans = doc["DEVICE"]["FAN"];
        JsonArray sensors = doc["DEVICE"]["SENSOR"];
        master.slave.NAME = doc["NAME"];
        master.slave.RELAY_NUMBER = relays.size();
        master.slave.FAN_NUMBER = fans.size();
        master.slave.SENSOR_NUMBER = sensors.size();
        int index = 0;
        for (JsonObject repo : relays){
            if(repo.containsKey("STATE")) { //Check if Key is actually present in it, or it will insert default values in places of int
                master.slave.relay[index].current_state = repo["STATE"].as<bool>();
                master.slave.relay[index].lastslavecommand = true;
            }
            if(repo.containsKey("VALUE")) {
                master.slave.relay[index].current_value = repo["VALUE"].as<int>();
                master.slave.relay[index].lastslavecommand = true;
            }
            index++;
        }
        index = 0;
        for (JsonObject repo : fans){
            if(repo.containsKey("STATE")) { //Check if Key is actually present in it, or it will insert default values in places of int
                master.slave.fan[index].current_state = repo["STATE"].as<bool>();
                master.slave.fan[index].lastslavecommand = true;
            }
            if(repo.containsKey("VALUE")) {
                master.slave.fan[index].current_value = repo["VALUE"].as<int>();
                master.slave.fan[index].lastslavecommand = true;
            }
            index++;
        }
        index = 0;
        for (JsonObject repo : sensors){
            if(repo.containsKey("TYPE")) { //Check if Key is actually present in it, or it will insert default values in places of int
                master.slave.sensor[index].type = repo["TYPE"].as<char *>();
                master.slave.sensor[index].lastslavecommand = true;
            }
            if(repo.containsKey("VALUE")) {
                master.slave.sensor[index].current_value = repo["VALUE"].as<int>();
                master.slave.sensor[index].lastslavecommand = true;
            }
            index++;
        }
        master.flag.received_json = false;
        if(role == "SLAVE"){
            return true;
        }
        return false;
        }
        else{
            return false;
        }
}

void HomeHub::slave_sync_handler(){
    if(master.flag.received_json == true){
        //Rester Last Slave Command flags to false
        for(int i=0;i<10;i++){
            master.slave.relay[i].lastslavecommand = false;
        }
        for(int i=0;i<10;i++){
            master.slave.fan[i].lastslavecommand = false;
        }
        for(int i=0;i<10;i++){
            master.slave.sensor[i].lastslavecommand = false;
        }
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, _slave_command_buffer);
        JsonObject obj = doc.as<JsonObject>();
        JsonArray relays = doc["DEVICE"]["RELAY"];
        JsonArray fans = doc["DEVICE"]["FAN"];
        JsonArray sensors = doc["DEVICE"]["SENSOR"];
        master.slave.NAME = doc["NAME"];
        int index = 0;
        for (JsonObject repo : relays){
            if(repo.containsKey("STATE")) { //Check if Key is actually present in it, or it will insert default values in places of int
                master.slave.relay[index].current_state = repo["STATE"].as<bool>();
                master.slave.relay[index].lastslavecommand = true;
            }
            if(repo.containsKey("VALUE")) {
                master.slave.relay[index].current_value = repo["VALUE"].as<int>();
                master.slave.relay[index].lastslavecommand = true;
            }
            index++;
        }
        index = 0;
        for (JsonObject repo : fans){
            if(repo.containsKey("STATE")) { //Check if Key is actually present in it, or it will insert default values in places of int
                master.slave.fan[index].current_state = repo["STATE"].as<bool>();
                master.slave.fan[index].lastslavecommand = true;
            }
            if(repo.containsKey("VALUE")) {
                master.slave.fan[index].current_value = repo["VALUE"].as<int>();
                master.slave.fan[index].lastslavecommand = true;
            }
            index++;
        }
        index = 0;
        for (JsonObject repo : sensors){
            /* No need to send the device type detials again and again after Handhshake
            if(repo.containsKey("TYPE")) { //Check if Key is actually present in it, or it will insert default values in places of int
                master.slave.sensor[index].type = repo["TYPE"].as<char *>();
                master.slave.sensor[index].lastslavecommand = true;
            }*/
            if(repo.containsKey("VALUE")) {
                master.slave.sensor[index].current_value = repo["VALUE"].as<int>();
                master.slave.sensor[index].lastslavecommand = true;
            }
            index++;
        }
    master.flag.received_json = false;
    const char* command = doc["COMMAND"];
    if(command != nullptr){
        slave_receive_command(command);
    }
  }
}

void HomeHub::slave_receive_command(const char* command){
    char Command[20];
    strlcpy(Command, command,20);
    if((strncmp(Command,"WIFI_SETUP_START",20) == 0)){
        HomeHub_DEBUG_PRINT("Starting Wifi Setup AP");
        initiate_wifi_setup();
    }
    else if((strncmp(Command,"WIFI_SETUP_STOP",20) == 0)){
        HomeHub_DEBUG_PRINT("Stopping Wifi Setup AP");
        end_wifi_setup();
    }
    else if((strncmp(Command,"WIFI_SAVED_CONNECT",20) == 0)){
        HomeHub_DEBUG_PRINT("Connecting to presaved Wifi");
        saved_wifi_connect();
    }
    else if((strncmp(Command,"UPDATE_DEVICE",20) == 0)){
        HomeHub_DEBUG_PRINT("Connecting to presaved Wifi");
        update_device();
    }
    else if((strncmp(Command,"ALL_LONGPRESS",20) == 0)){
        HomeHub_DEBUG_PRINT("Long Press command received from Slave");
        _esid = "";
        _epass = "";
        initiate_wifi_setup();
    }
    else{
        HomeHub_DEBUG_PRINT("Unsupported Command");
    }
}

void HomeHub::salve_serial_json_input_capture(){
    //Read incomming bit per cycle and decode the incomming commands
    if(HomeHub_SLAVE_DATA_PORT.available()) {
        char c = HomeHub_SLAVE_DATA_PORT.read();
        if (c == '{'){
            if(_SLAVE_DATA_PORT_counter==0){
                master.flag.receiving_json = true;
                _SLAVE_DATA_PORT_command = "";
            }
            _SLAVE_DATA_PORT_counter+=1;
            _SLAVE_DATA_PORT_command += c;
        }
        else if (c == '}'){
            _SLAVE_DATA_PORT_counter+=-1;
            _SLAVE_DATA_PORT_command += c;
        }
        else{
            _SLAVE_DATA_PORT_command += c;
        }
        if(_SLAVE_DATA_PORT_counter == 0 && master.flag.receiving_json == true){
            _slave_command_buffer = _SLAVE_DATA_PORT_command;
            _SLAVE_DATA_PORT_command = "";
            master.flag.receiving_json = false;
            master.flag.received_json = true;
        }
    }
}

void HomeHub::device_handler(){
    //Check handshake flag to confirm that slave has actually been connected to the master module before monitoring slave changes
    if(master.flag.slave_handshake == false){
    int all_relay_change_counter = 0;
    int all_fan_change_counter = 0;
    int all_sensor_change_counter = 0;
    int all_relay_lastslavecommand_counter = 0;
    int all_fan_lastslavecommand_counter = 0;
    int all_sensor_lastslavecommand_counter = 0;
    int all_relay_lastmqttcommand_counter = 0;
    int all_fan_lastmqttcommand_counter = 0;
    int all_sensor_lastmqttcommand_counter = 0;
    
    //activate changes flag of individual relay devices
    for(int i=0;i<master.slave.RELAY_NUMBER;i++){
        if(master.slave.relay[i].lastslavecommand == true){ //Universtal counter for all relay lastslavecommand
            all_relay_lastslavecommand_counter++;
        }
        if(master.slave.relay[i].lastmqttcommand == true){ //Universtal counter for all relay lastslavecommand
            all_relay_lastmqttcommand_counter++;
        }//Universal counter for all relay changes
        if(master.slave.relay[i].current_state != master.slave.relay[i].previous_state){
            master.slave.relay[i].change = true;
            all_relay_change_counter++;
        }
        else if(master.slave.relay[i].current_value != master.slave.relay[i].previous_value){
            master.slave.relay[i].change = true;
            all_relay_change_counter++;
        }
        else{
            master.slave.relay[i].change = false;
        }
        master.slave.relay[i].previous_state = master.slave.relay[i].current_state;
        master.slave.relay[i].previous_value = master.slave.relay[i].current_value;
    }
    //activate changes flag of individual fan devices
    for(int i=0;i<master.slave.FAN_NUMBER;i++){
        if(master.slave.fan[i].lastslavecommand == true){ //Universtal counter for all fan lastslavecommand
            all_fan_lastslavecommand_counter++;
        }
        if(master.slave.fan[i].lastmqttcommand == true){ //Universtal counter for all fan lastslavecommand
            all_fan_lastmqttcommand_counter++;
        }
        if(master.slave.fan[i].current_state != master.slave.fan[i].previous_state){
            master.slave.fan[i].change = true;
            all_fan_change_counter++;
        }
        else if(master.slave.fan[i].current_value != master.slave.fan[i].previous_value){
            all_fan_change_counter++;
        }
        else{
            master.slave.fan[i].change = false;
        }
        master.slave.fan[i].previous_state = master.slave.fan[i].current_state;
        master.slave.fan[i].previous_value = master.slave.fan[i].current_value;
    }
    //activate changes flag of individual sensor devices
    for(int i=0;i<master.slave.SENSOR_NUMBER;i++){
        if(master.slave.sensor[i].lastslavecommand == true){ //Universtal counter for all sensor lastslavecommand
            all_sensor_lastslavecommand_counter++;
        }
        if(master.slave.sensor[i].lastmqttcommand == true){ //Universtal counter for all sensor lastmqttcommand
            all_sensor_lastmqttcommand_counter++;
        }
        if(master.slave.sensor[i].current_value != master.slave.sensor[i].previous_value){
            master.slave.sensor[i].change = true;
            all_sensor_change_counter++;
        }
        else{
            master.slave.sensor[i].change = false;
        }
        master.slave.sensor[i].previous_value = master.slave.sensor[i].current_value;
    }
    //activate lastslavecommand flag for relay structure wide
    if(all_relay_lastslavecommand_counter == 0){
        master.slave.all_relay_lastslavecommand = false;
    }
    else{
        master.slave.all_relay_lastslavecommand = true;
    }
    //activate lastslavecommand flag for fan structure wide
    if(all_fan_lastslavecommand_counter == 0){
        master.slave.all_fan_lastslavecommand = false;
    }
    else{
        master.slave.all_fan_lastslavecommand = true;
    }
    //activate lastslavecommand flag for sensor structure wide
    if(all_sensor_lastslavecommand_counter == 0){
        master.slave.all_sensor_lastslavecommand = false;
    }
    else{
        master.slave.all_sensor_lastslavecommand = true;
    }
    //activate lastmqttcommand flag for relay structure wide
    if(all_relay_lastmqttcommand_counter == 0){
        master.slave.all_relay_lastmqttcommand = false;
    }
    else{
        master.slave.all_relay_lastmqttcommand = true;
    }
    //activate lastmqttcommand flag for fan structure wide
    if(all_fan_lastslavecommand_counter == 0){
        master.slave.all_fan_lastmqttcommand = false;
    }
    else{
        master.slave.all_fan_lastmqttcommand = true;
    }
    //activate lastmqttcommand flag for sensor structure wide
    if(all_sensor_lastmqttcommand_counter == 0){
        master.slave.all_sensor_lastmqttcommand = false;
    }
    else{
        master.slave.all_sensor_lastmqttcommand = true;
    }
    //activate changes flag for full relay structure
    if(all_relay_change_counter == 0){
        master.slave.all_relay_change = false;
    }
    else{
        master.slave.all_relay_change = true;
    }
    //activate changes flag for full fan structure
    if(all_fan_change_counter == 0){
        master.slave.all_fan_change = false;
    }
    else{
        master.slave.all_fan_change = true;
    }
    //activate changes flag for full sensor structure
    if(all_sensor_change_counter == 0){
        master.slave.all_sensor_change = false;
    }
    else{
        master.slave.all_sensor_change = true;
    }
    //activate slave wide change variable if any device structure changes ie relay, fan or sensor
    if((master.slave.all_relay_change == false) && (master.slave.all_fan_change == false) && (master.slave.all_sensor_change == false)){
        master.slave.change = false;
    }
    else{
        master.slave.change = true;
    }
    }
}

//Initiate Memory
void HomeHub::initiate_memory(){
    if(master.flag.rom_external == false){
        EEPROM.begin(512);
    }
    else{
        Wire.begin();
    }
}

//Custom EEPROM replacement functions
void HomeHub::rom_write(unsigned int eeaddress, byte data) {
    if(master.flag.rom_external == false){
        int rdata = data;
        EEPROM.write(eeaddress,data);
        delay(20);
        EEPROM.commit();
    }else{
        int rdata = data;
        Wire.beginTransmission(ROM_ADDRESS);
        Wire.write((int)(eeaddress >> 8));      // MSB
        Wire.write((int)(eeaddress & 0xFF));    // LSB
        Wire.write(rdata);
        Wire.endTransmission();
        delay(20);                              //Delay introduction solved the data save unreliability
    }
}

//Custom EEPROM replacement functions
byte HomeHub::rom_read(unsigned int eeaddress) {
    if(master.flag.rom_external == false){
        byte rdata = EEPROM.read(eeaddress);
        return rdata;
    }else{
        byte rdata = 0xFF;
        Wire.beginTransmission(ROM_ADDRESS);
        Wire.write((int)(eeaddress >> 8)); // MSB
        Wire.write((int)(eeaddress & 0xFF)); // LSB
        Wire.endTransmission();
        Wire.requestFrom(ROM_ADDRESS, 1);
        if (Wire.available()) rdata = Wire.read();
            return rdata;
    }
}

void HomeHub::rom_write_page(unsigned int eeaddresspage, byte* data, byte length ) {
    if(master.flag.rom_external == false){
        
    }else{
        Wire.beginTransmission(ROM_ADDRESS);
        Wire.write((int)(eeaddresspage >> 8)); // MSB
        Wire.write((int)(eeaddresspage & 0xFF)); // LSB
        byte c;
        for ( c = 0; c < length; c++)
            Wire.write(data[c]);
        Wire.endTransmission();
    }
}

bool HomeHub::retrieve_wifi_data(){
  _esid = "";
  _epass = "";
  for (int i = 0;i < 32;++i)
    {
      _esid += char(rom_read(_wifi_data_memspace+i)); //_esid += char(EEPROM.read(i));
    }
  //HomeHub_DEBUG_PRINT("SSID:");HomeHub_DEBUG_PORT.println(_esid);
  _epass = "";
  for(int i = 32;i < 96;++i)
    {
      _epass += char(rom_read(_wifi_data_memspace+i)); //_epass += char(EEPROM.read(i));
    }
 // HomeHub_DEBUG_PRINT("PASS:");HomeHub_DEBUG_PORT.println(_epass);
  if(_esid.length() > 1){
    return true;
  }
  else{
    return false;
  }
}

bool HomeHub::test_wifi() {
  HomeHub_DEBUG_PRINT("Waiting for Wifi to connect");
  for(int i=0;i<100;i++){
    delay(100);
    if(WiFi.status() == 3){
      break ;
    }
  }
  if (WiFi.status() == WL_CONNECTED){
    HomeHub_DEBUG_PRINT("Connected to Wifi Successfully");
    return true;
  }
  else{
    HomeHub_DEBUG_PRINT("Connect timed out");
    return false;
  }
}

String HomeHub::scan_networks(){
  //WiFi.mode(WIFI_STA);
  //WiFi.disconnect();
  String wifi_data = "";
  int network_available_number = WiFi.scanNetworks();
  //Serial.println("scan done");
  
  if(network_available_number == 0){
    //Serial.println("no networks found");
  }
  else
  {
    //Serial.print(network_available_number);
    //Serial.println(" networks found");
  }
  
  //Serial.println("");
  
  wifi_data = "<ul>";
  for (int i = 0; i < network_available_number; ++i)
    {
      // Print SSID and RSSI for each network found
      /*Serial.print(i + 1);Serial.print(": ");Serial.print(WiFi.SSID(i));
      Serial.print(" (");Serial.print(WiFi.RSSI(i));Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
      */
      // Print SSID and RSSI for each network found
      wifi_data += "<li>";
      wifi_data +=i + 1;
      wifi_data += ": ";
      wifi_data += WiFi.SSID(i);
      wifi_data += " (";
      wifi_data += WiFi.RSSI(i);
      wifi_data += ")";
      wifi_data += (WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*";
      wifi_data += "</li>";
      //Comparator
      int test_counter = 0;
      String current_wifi = WiFi.SSID(i);
      for(int j=0;j<current_wifi.length();j++){
        if(current_wifi.charAt(j) == _esid.charAt(j)){
          test_counter++;
        }
      }
      if(test_counter == current_wifi.length()){
        HomeHub_DEBUG_PRINT("Saved Wifi present");
        master.flag.saved_wifi_present = true;
      }
    }
    wifi_data += "</ul>";
    return wifi_data;
}

void HomeHub::start_server(){
    server = new WiFiServer(80);
    mdns = new MDNSResponder();
    
  mdns->begin("esp8266", WiFi.softAPIP());
  HomeHub_DEBUG_PRINT("mDNS responder started");
    
  server->begin(); // Web server start
  HomeHub_DEBUG_PRINT("Web Server started");
}

bool HomeHub::stop_server(){
  mdns->close();
  HomeHub_DEBUG_PRINT("mDNS responder ended");
  // Start the server
  server->stop();
  server->close();
  HomeHub_DEBUG_PRINT("Web Server ended");
  return true;
}

bool HomeHub::initiate_ap(){
  WiFi.mode(WIFI_AP);
  WiFi.disconnect();
  delay(100);
  //technically add finding wifi data into this
  const char* ssid = _ssid_string.c_str();
  WiFi.softAP(ssid);
  HomeHub_DEBUG_PRINT("softap");
}

bool HomeHub::end_ap(){
  WiFi.softAPdisconnect();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  return(saved_wifi_connect());
}

bool HomeHub::saved_wifi_connect(){
  if(retrieve_wifi_data()){
      WiFi.begin(_esid.c_str(), _epass.c_str());
      bool connection_result = test_wifi();
      if(connection_result){
          if(master.flag.boot_check_update == true){
              update_device();
          }
      }
      return (connection_result);
  }
}

bool HomeHub::manual_wifi_connect(const char* wifi, const char* pass){
    _esid = wifi;
    _epass = pass;
    WiFi.begin(wifi, pass);
    bool connection_result = test_wifi();
    if(connection_result){
        if(master.flag.boot_check_update == true){
            update_device();
        }
    }
    return (connection_result);
}

void HomeHub::save_wifi_data(String ssid, String password){
    Serial.println("Saved Wifi data");
    for (int i = 0; i < ssid.length(); ++i)
      {
        rom_write(_wifi_data_memspace+i, ssid[i]); //EEPROM.write(i, qsid[i]);
        HomeHub_DEBUG_PRINT("Wrote: ");
        HomeHub_DEBUG_PORT.println(ssid[i]);
        _esid = ssid;
      }
    HomeHub_DEBUG_PRINT("writing eeprom pass:");
    for (int i = 0; i < password.length(); ++i)
      {
        rom_write(_wifi_data_memspace+32+i, password[i]); //EEPROM.write(32+i, qpass[i]);
        HomeHub_DEBUG_PRINT("Wrote: ");
        HomeHub_DEBUG_PORT.println(password[i]);
        _epass = password;
      }
}

void HomeHub::saved_wifi_dump(){
  HomeHub_DEBUG_PRINT("clearing eeprom");
  for (int i = 0; i < 96; ++i) {
      rom_write(_wifi_data_memspace+i,0);// EEPROM.write(i, 0);
  }
  //EEPROM.commit();
}

bool HomeHub::initiate_wifi_setup(){
  _wifi_data = scan_networks();
  initiate_ap();
  start_server();
  //ticker->attach_ms(100, std::bind(&HomeHub::scan_networks, this));
  //ticker->attach_ms(1000, std::bind(&HomeHub::wifi_setup_webhandler, this));
  master.flag.wifi_setup_webhandler= true;
}

bool HomeHub::end_wifi_setup(){
  stop_server();
  end_ap();
  master.flag.wifi_setup_webhandler = false;
}

bool HomeHub::initiate_mqtt(){
    mqttclient.connect(mqtt_clientname,mqtt_serverid, mqtt_serverpass,"WillTopic",1,1,"willMessage");
    if (mqttclient.connected()) {
        mqttclient.subscribe(Device_Id_In_Char_As_Subscription_Topic);
        publish_mqtt("[LOGGED IN]");
        master.flag.mqtt_webhandler = true;
        return 's';
    } else {
        HomeHub_DEBUG_PRINT("Failed to connect to mqtt server, rc=");
        //HomeHub_DEBUG_PORT.print(mqttclient.state());
        return 'f';
    }
}

HomeHub& HomeHub::setCallback(MQTT_CALLBACK_SIGN)
{
    this->mqttCallback = mqttCallback;
    return *this;
}

void HomeHub::mqttcallback(char* topic, byte* payload, unsigned int length) {
  String pay = "";
  String Topic = topic;
  for (int i = 0; i < length; i++) {
    pay+=(char)payload[i];
  }
  LastCommand = pay;
  mqtt_input_handler(Topic, pay);
}
    
bool HomeHub::mqtt_input_handler(String topic,String payload){
    //HomeHub_DEBUG_PORT.println("Topic : "+topic);
    //HomeHub_DEBUG_PORT.println("Payload : "+payload);
    //Rester Last Slave Command flags to false
    for(int i=0;i<10;i++){
        master.slave.relay[i].lastmqttcommand = false;
    }
    for(int i=0;i<10;i++){
        master.slave.fan[i].lastmqttcommand = false;
    }
    for(int i=0;i<10;i++){
        master.slave.sensor[i].lastmqttcommand = false;
    }
    if(topic.charAt(topic.length()) != '/'){
        topic = topic + "/";
    }
    //Switching the lastmqttcommands to their default value
    //remove the chipid
    if(topic.length() > 0){
        String chipId = topic.substring(0,topic.indexOf('/',0));
        topic.remove(0,topic.indexOf('/',0)+1);
        if(topic.length() > 0){
            String sub1 = topic.substring(0,topic.indexOf('/',0));
            topic.remove(0,topic.indexOf('/',0)+1);
            if(sub1 == "relay"){
                if(topic.length() > 0){
                    String sub1 = topic.substring(0,topic.indexOf('/',0));
                    topic.remove(0,topic.indexOf('/',0)+1);
                    if(sub1 == "1"){
                        if(topic.length() > 0){
                            String sub1 = topic.substring(0,topic.indexOf('/',0));
                            topic.remove(0,topic.indexOf('/',0)+1);
                            if(sub1 == "state"){
                                master.slave.relay[0].current_state = bool(payload.toInt());
                                master.slave.relay[0].lastmqttcommand = true;
                            }
                        }
                    }
                    else if(sub1 == "2"){
                        if(topic.length() > 0){
                            String sub1 = topic.substring(0,topic.indexOf('/',0));
                            topic.remove(0,topic.indexOf('/',0)+1);
                            if(sub1 == "state"){
                                master.slave.relay[1].current_state = bool(payload.toInt());
                                master.slave.relay[1].lastmqttcommand = true;
                            }
                        }
                    }
                    else if(sub1 == "3"){
                        if(topic.length() > 0){
                            String sub1 = topic.substring(0,topic.indexOf('/',0));
                            topic.remove(0,topic.indexOf('/',0)+1);
                            if(sub1 == "state"){
                                master.slave.relay[2].current_state = bool(payload.toInt());
                                master.slave.relay[2].lastmqttcommand = true;
                            }
                        }
                    }
                    else if(sub1 == "4"){
                        if(topic.length() > 0){
                            String sub1 = topic.substring(0,topic.indexOf('/',0));
                            topic.remove(0,topic.indexOf('/',0)+1);
                            if(sub1 == "state"){
                                master.slave.relay[3].current_state = bool(payload.toInt());
                                master.slave.relay[3].lastmqttcommand = true;
                            }
                        }
                    }
                    else if(sub1 == "all"){
                        if(topic.length() > 0){
                            String sub1 = topic.substring(0,topic.indexOf('/',0));
                            topic.remove(0,topic.indexOf('/',0)+1);
                            if(sub1 == "state"){
                                for(int i=0;i<master.slave.RELAY_NUMBER;i++){
                                    master.slave.relay[i].current_state = bool(payload.toInt());
                                    master.slave.relay[i].lastmqttcommand = true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    /*
    while(topic.length() > 0){
      String structure = topic.substring(0,topic.indexOf('/',0));
      Serial.println(structure);
      topic.remove(0,topic.indexOf('/',0)+1);
    }*/
}

void HomeHub::publish_mqtt(String message)
{
    mqttclient.publish(Device_Id_In_Char_As_Publish_Topic, message.c_str(), true);
}

void HomeHub::publish_mqtt(String topic, String message)
{
    
    mqttclient.publish(topic.c_str(), message.c_str());
}

void HomeHub::mqtt_output_handler(){
    if(master.slave.change == true){
        String topic = "";
        String payload = "";
        if(master.slave.all_relay_change == true){
            for(int i=0;i<master.slave.RELAY_NUMBER;i++){
                if(master.slave.relay[i].change == true){
                    topic = Device_Id_As_Publish_Topic + "relay/" + String(i+1) + "/state/";
                    payload = String(master.slave.relay[i].current_state);
                    publish_mqtt(topic,payload);
                    topic = Device_Id_As_Publish_Topic + "relay/" + String(i+1) + "/value/";
                    payload = String(master.slave.relay[i].current_value);
                    publish_mqtt(topic,payload);
                }
            }
        }
        if(master.slave.all_fan_change == true){
          for(int i=0;i<master.slave.FAN_NUMBER;i++){
              if(master.slave.fan[i].change == true){
                  topic = Device_Id_As_Publish_Topic + "fan/" + String(i+1) + "/state/";
                  payload = String(master.slave.fan[i].current_state);
                  publish_mqtt(topic,payload);
                  topic = Device_Id_As_Publish_Topic + "fan/" + String(i+1) + "/value/";
                  payload = String(master.slave.fan[i].current_value);
                  publish_mqtt(topic,payload);
              }
          }
      }
    if(master.slave.all_sensor_change == true){
        for(int i=0;i<master.slave.SENSOR_NUMBER;i++){
            if(master.slave.sensor[i].change == true){
                //topic = Device_Id_As_Publish_Topic + "sensor/" + String(i+1) + "/" +"type/";
                //payload = master.slave.sensor[i].type;
                //publish_mqtt(topic,payload);
                topic = Device_Id_As_Publish_Topic + "sensor/" + String(i+1) + "/" +"value/";
                payload = String(master.slave.sensor[i].current_value);
                publish_mqtt(topic,payload);
            }
        }
    }
    }
}

void HomeHub::timesensor_handler(){
  if(_timesensor_set == 0){
    Clock.setYear(_timesensor_year);
    Clock.setMonth(_timesensor_month);
    Clock.setDate(_timesensor_date);
    Clock.setDoW(_timesensor_week);
    Clock.setHour(_timesensor_hour);
    Clock.setMinute(_timesensor_minute);
    Clock.setSecond(_timesensor_second);
  }
  if(_timesensor_set == 1){
      _timesensor_date = Clock.getDate();
      _timesensor_month = Clock.getMonth(_timesensor_century);
      _timesensor_year = Clock.getYear();
      _timesensor_week = Clock.getDoW();
      _timesensor_hour = Clock.getHour(_timesensor_h12,_timesensor_pmam);
      _timesensor_minute = Clock.getMinute();
      _timesensor_second = Clock.getSecond();
      _timesensor_temp = Clock.getTemperature();
      
    // 1: Date 2: Month 3: Year 4: Week 5:HouR 6:Minute 7: Second 8: temperature 9: Mode 10: Oscillator issue
    if((_timesensor_date >31)||((_timesensor_date <0))){
      _timesensor_error_code = 1;
      _timesensor_error = 0;
    }
    else if((_timesensor_month >12)||((_timesensor_month <0))){
      _timesensor_error_code = 2;
      _timesensor_error = 0;
    }
    else if((_timesensor_year > 30)||((_timesensor_year < 19))){
      _timesensor_error_code = 3;
      _timesensor_error = 0;
    }
    else if((_timesensor_week >7)||((_timesensor_week <0))){
      _timesensor_error_code = 4;
      _timesensor_error = 0;
    }
    else if((_timesensor_hour >24)||((_timesensor_hour <0))){
      _timesensor_error_code = 5;
      _timesensor_error = 0;
    }
    else if((_timesensor_minute >60)||((_timesensor_minute <0))){
      _timesensor_error_code = 6;
      _timesensor_error = 0;
    }
    else if((_timesensor_second >60)||((_timesensor_second <0))){
      _timesensor_error_code = 7;
      _timesensor_error = 0;
    }
    else if((_timesensor_temp >70)||((_timesensor_temp <10))){
      _timesensor_error_code = 8;
      _timesensor_error = 0;
    }
    //else if((Clock.oscillatorCheck())){
    //  Td_sensor.Error_code = 10;
    //  Td_sensor.Error = 0;
    //}
    else{
      _timesensor_error_code = 0;
      _timesensor_error = 1;
    }/*
    if(EEPROM.read(timesensor_memspace[1]) != timesensor_error){
        EEPROM.write(timesensor_memspace[1],timesensor_error);
      }
    if(EEPROM.read(timesensor_memspace[2]) !=  timesensor_error_code){
        EEPROM.write(timesensor_memspace[2],timesensor_error_code);
      } */
  }
}

void HomeHub::update_device()
{
    if (WiFi.status() == WL_CONNECTED){
    t_httpUpdate_return ret = ESPhttpUpdate.update(_Client, _host_update, _firmware_version);
    switch (ret) {
      case HTTP_UPDATE_FAILED:
            //HomeHub_DEBUG_PORT.println("HTTP_UPDATE_FAILD Error (%d): %s\n");//,ESPhttpUpdate.getLastError(),ESPhttpUpdate.getLastErrorString().c_str());
        break;

      case HTTP_UPDATE_NO_UPDATES:
        //HomeHub_DEBUG_PORT.println("HTTP_UPDATE_NO_UPDATES");
        break;

      case HTTP_UPDATE_OK:
        //HomeHub_DEBUG_PORT.println("HTTP_UPDATE_OK");
        break;
    }
    }
    else{
        //HomeHub_DEBUG_PRINT("Device is Offline, cant update.");
    }
}

int HomeHub::wifi_setup_webhandler()
{
  // Check for any mDNS queries and send responses
  mdns->update();
  
  // Check if a client has connected
  WiFiClient client = server->available();
  if (!client) {
    return(20);
  }
  
  //New client has connected
  HomeHub_DEBUG_PRINT("");
  HomeHub_DEBUG_PRINT("New client");

  // Wait for data from client to become available
  while(client.connected() && !client.available()){
    delay(1);
   }
  
  // Read the first line of HTTP request
  String req = client.readStringUntil('\r');
  
  // First line of HTTP request looks like "GET /path HTTP/1.1"
  // Retrieve the "/path" part by finding the spaces
  int addr_start = req.indexOf(' ');
  int addr_end = req.indexOf(' ', addr_start + 1);
  if (addr_start == -1 || addr_end == -1) {
    HomeHub_DEBUG_PRINT("Invalid request: ");
    HomeHub_DEBUG_PORT.println(req);
    return(20);
   }
  req = req.substring(addr_start + 1, addr_end);
  HomeHub_DEBUG_PRINT("Request: ");
  HomeHub_DEBUG_PORT.println(req);
  client.flush();
  String s;
      if (req == "/")
      {
        IPAddress ip = WiFi.softAPIP();
        String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
        s += ipStr;
        s += "<p>";
        s += _wifi_data;
        s += "<form method='get' action='a'><label>SSID: </label><input name='ssid' length=32><input name='pass' length=64><input type='submit'></form>";
        s += "</html>\r\n\r\n";
        HomeHub_DEBUG_PRINT("Sending 200");
      }
      else if ( req.startsWith("/a?ssid=") ) {
        // /a?ssid=blahhhh&pass=poooo
        HomeHub_DEBUG_PRINT("clearing eeprom");
        saved_wifi_dump();
        String qsid;
        qsid = req.substring(8,req.indexOf('&'));
        qsid.replace("+"," ");
        //Serial.println(qsid);
       // Serial.println("");
        String qpass;
        qpass = req.substring(req.lastIndexOf('=')+1);
          qpass.replace("+"," ");
        //Serial.println(qpass);
        //Serial.println("");
        
        HomeHub_DEBUG_PRINT("writing eeprom ssid:");
        save_wifi_data(qsid,qpass);
        //EEPROM.commit();
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 ";
        s += "Found ";
        s += req;
        s += "<p> saved to eeprom... reset to boot into new wifi</html>\r\n\r\n";
      }
      else
      {
        s = "HTTP/1.1 404 Not Found\r\n\r\n";
        HomeHub_DEBUG_PRINT("Sending 404");
      }
  client.print(s);
  HomeHub_DEBUG_PRINT("Done with client");
  return(20);
}

int HomeHub::normal_webhandler()
{
  // Check for any mDNS queries and send responses
  mdns->update();
  
  // Check if a client has connected
  WiFiClient client = server->available();
  if (!client) {
    return(20);
  }
  
  //New client has connected
  HomeHub_DEBUG_PRINT("New client");

  // Wait for data from client to become available
  while(client.connected() && !client.available()){
    delay(1);
   }
  
  // Read the first line of HTTP request
  String req = client.readStringUntil('\r');
  
  // First line of HTTP request looks like "GET /path HTTP/1.1"
  // Retrieve the "/path" part by finding the spaces
  int addr_start = req.indexOf(' ');
  int addr_end = req.indexOf(' ', addr_start + 1);
  if (addr_start == -1 || addr_end == -1) {
    HomeHub_DEBUG_PRINT("Invalid request: ");
    HomeHub_DEBUG_PORT.println(req);
    return(20);
   }
  req = req.substring(addr_start + 1, addr_end);
  HomeHub_DEBUG_PRINT("Request: ");
  HomeHub_DEBUG_PORT.println(req);
  client.flush();
  String s;
  
  if (req == "/")
  {
     s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266";
     s += "<p>";
     s += "</html>\r\n\r\n";
     HomeHub_DEBUG_PRINT("Sending 200");
  }
  else if ( req.startsWith("/cleareeprom") ) {
    s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266";
    s += "<p>Clearing the EEPROM<p>";
    s += "</html>\r\n\r\n";
    HomeHub_DEBUG_PRINT("Sending 200");
    HomeHub_DEBUG_PRINT("clearing eeprom");
    for (int i = 0; i < 96; ++i) {
        rom_write(i,0);// EEPROM.write(i, 0);
    }
    //EEPROM.commit();
  }
  else
  {
   s = "HTTP/1.1 404 Not Found\r\n\r\n";
   HomeHub_DEBUG_PRINT("Sending 404");
  }
  client.print(s);
  HomeHub_DEBUG_PRINT("Done with client");
  return(20);
}
