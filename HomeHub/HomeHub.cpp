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
    Device_Id_As_Subscription_Topic.toCharArray(Device_Id_In_Char_As_Subscription_Topic, 12);
    //Mqtt connection setup
    mqtt_clientname = Device_Id_In_Char_As_Publish_Topic;
    mqttclient.setServer(mqtt_server, mqtt_port);
    mqttclient.setCallback([this] (char* topic, byte* payload, unsigned int length) { this->mqttcallback(topic, payload, length); });
    //Initiate Wire begin
	Wire.begin();
    //Initilize new Ticker function to run serial slave handler
    ticker = new Ticker();
    ticker->attach_ms(1, std::bind(&HomeHub::slave_handler, this));
    //Retrieve saved wifi data from Rom
    _slave_handshake_flag = true;
    retrieve_wifi_data();
	HomeHub_DEBUG_PRINT("STARTED");
}

void HomeHub::asynctasks(){
    timesensor_handler();
    if(_slave_handshake_flag == true){
        if(_slave_handshake_millis == 0.0){
            char* request_handshake = "{\"COMMAND\":\"HANDSHAKE\"}";
            HomeHub_SLAVE_DATA_PORT.println(request_handshake);
            _slave_handshake_millis = millis() + 5000;
        }
        else{
            if(millis() > _slave_handshake_millis){
                if(slave_handshake_handler() == true){
                    _slave_handshake_flag = false;
                    HomeHub_DEBUG_PRINT("Slave Handshake Complete");
                }
                else{
                    HomeHub_DEBUG_PRINT("No Slave detected, retrying.");
                    _slave_handshake_millis = 0.0;
                }
            }
        }
    }
    else{
        slave_sync_handler();
    }
    //Handling Wifi Setup client and Page requests in each loop
    if(_wifi_setup_webhandler_flag == true){
        _wifi_data = scan_networks();
        wifi_setup_webhandler();
        if(_saved_wifi_present_flag == true){
            end_wifi_setup();
        }
    }
    //Handling Mqtt looping and wifi connection fallbacks in each loop
    if(_mqtt_webhandler_flag == true){
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
}

void HomeHub::slave_handler(){
	//Read incomming bit per cycle and decode the incomming commands
    if(HomeHub_SLAVE_DATA_PORT.available()) {
        char c = HomeHub_SLAVE_DATA_PORT.read();
        if (c == '{'){
            if(_SLAVE_DATA_PORT_counter==0){
                _receiving_json = true;
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
        if(_SLAVE_DATA_PORT_counter == 0 && _receiving_json == true){
            //HomeHub_SLAVE_DATA_PORT.println(_SLAVE_DATA_PORT_command);
            _slave_command_buffer = _SLAVE_DATA_PORT_command;
            _SLAVE_DATA_PORT_command = "";
            _receiving_json = false;
            _received_json = true;
        }
    }
}

bool HomeHub::slave_handshake_handler(){
    if(_received_json == true){
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, _slave_command_buffer);
        JsonObject obj = doc.as<JsonObject>();
        String role = doc["ROLE"];
        JsonArray relays = doc["DEVICE"]["RELAY"];
        JsonArray fans = doc["DEVICE"]["FAN"];
        JsonArray sensors = doc["DEVICE"]["SENSOR"];
        slave.NAME = doc["NAME"];
        slave.RELAY_NUMBER = relays.size();
        slave.FAN_NUMBER = fans.size();
        slave.SENSOR_NUMBER = sensors.size();
        int index = 0;
        for (JsonObject repo : relays){
            slave.relay[index].state = repo["STATE"].as<bool>();
            slave.relay[index].value = repo["VALUE"].as<int>();
            index++;
        }
        index = 0;
        for (JsonObject repo : fans){
            slave.fan[index].state = repo["STATE"].as<bool>();
            slave.fan[index].value = repo["VALUE"].as<int>();
            index++;
        }
        index = 0;
        for (JsonObject repo : sensors){
            slave.sensor[index].type = repo["TYPE"].as<char *>();
            slave.sensor[index].value = repo["VALUE"].as<int>();
            index++;
        }
        _received_json = false;
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
    if(_received_json == true){
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, _slave_command_buffer);
        JsonObject obj = doc.as<JsonObject>();
        JsonArray relays = doc["DEVICE"]["RELAY"];
        JsonArray fans = doc["DEVICE"]["FAN"];
        JsonArray sensors = doc["DEVICE"]["SENSOR"];
        slave.NAME = doc["NAME"];
        slave.RELAY_NUMBER = relays.size();
        slave.FAN_NUMBER = fans.size();
        slave.SENSOR_NUMBER = sensors.size();
        int index = 0;
        for (JsonObject repo : relays){
            slave.relay[index].state = repo["STATE"].as<bool>();
            slave.relay[index].value = repo["VALUE"].as<int>();
            index++;
        }
        index = 0;
        for (JsonObject repo : fans){
            slave.fan[index].state = repo["STATE"].as<bool>();
            slave.fan[index].value = repo["VALUE"].as<int>();
            index++;
        }
        index = 0;
        for (JsonObject repo : sensors){
            slave.sensor[index].type = repo["TYPE"].as<char *>();
            slave.sensor[index].value = repo["VALUE"].as<int>();
            index++;
        }
    _received_json = false;
    const char* command = doc["COMMAND"];
    if(command != nullptr){
        slave_command_processor(command);
    }
  }
}

void HomeHub::slave_command_processor(const char* command){
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
    else{
        HomeHub_DEBUG_PRINT("Unsupported Command");
    }
}

//Custom EEPROM replacement functions
void HomeHub::rom_write(unsigned int eeaddress, byte data) {
	int rdata = data;
	Wire.beginTransmission(ROM_ADDRESS);
	Wire.write((int)(eeaddress >> 8)); // MSB
	Wire.write((int)(eeaddress & 0xFF)); // LSB
	Wire.write(rdata);
	Wire.endTransmission();
	delay(20); //Delay introduction solved the data save unreliability
}

//Custom EEPROM replacement functions
byte HomeHub::rom_read(unsigned int eeaddress) {
	byte rdata = 0xFF;
	Wire.beginTransmission(ROM_ADDRESS);
	Wire.write((int)(eeaddress >> 8)); // MSB
	Wire.write((int)(eeaddress & 0xFF)); // LSB
	Wire.endTransmission();
	Wire.requestFrom(ROM_ADDRESS, 1);
	if (Wire.available()) rdata = Wire.read();
	return rdata;
}

void HomeHub::rom_write_page(unsigned int eeaddresspage, byte* data, byte length ) {
    Wire.beginTransmission(ROM_ADDRESS);
    Wire.write((int)(eeaddresspage >> 8)); // MSB
    Wire.write((int)(eeaddresspage & 0xFF)); // LSB
    byte c;
    for ( c = 0; c < length; c++)
        Wire.write(data[c]);
    Wire.endTransmission();
}

bool HomeHub::retrieve_wifi_data(){
  _esid = "";
  _epass = "";
  for (int i = 0;i < 32;++i)
    {
      _esid += char(rom_read(_wifi_data_memspace+i)); //_esid += char(EEPROM.read(i));
    }
  HomeHub_DEBUG_PRINT("SSID:");HomeHub_DEBUG_PORT.println(_esid);
  _epass = "";
  for(int i = 32;i < 96;++i)
    {
      _epass += char(rom_read(_wifi_data_memspace+i)); //_epass += char(EEPROM.read(i));
    }
  HomeHub_DEBUG_PRINT("PASS:");HomeHub_DEBUG_PORT.println(_epass);
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
        _saved_wifi_present_flag = true;
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
        if(_boot_check_update_flag == true){
            update_device();
          }
      }
    return (connection_result);
  }
}

bool HomeHub::manual_wifi_connect(const char* wifi, const char* pass){
    WiFi.begin(wifi, pass);
    bool connection_result = test_wifi();
    if(connection_result){
        if(_boot_check_update_flag == true){
            update_device();
        }
    }
    return (connection_result);
}

void HomeHub::save_wifi_data(String ssid, String password){
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
  _wifi_setup_webhandler_flag = true;
}

bool HomeHub::end_wifi_setup(){
  stop_server();
  end_ap();
  _wifi_setup_webhandler_flag = false;
}

bool HomeHub::initiate_mqtt(){
    mqttclient.connect(mqtt_clientname,mqtt_serverid, mqtt_serverpass);
    
    if (mqttclient.connected()) {
        mqttclient.subscribe(Device_Id_In_Char_As_Subscription_Topic);
        publish_mqtt("[LOGGED IN]");
        _mqtt_webhandler_flag = true;
        return 's';
    } else {
        HomeHub_DEBUG_PRINT("Failed to connect to mqtt server, rc=");
        HomeHub_DEBUG_PORT.print(mqttclient.state());
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
  for (int i = 0; i < length; i++) {
    pay+=(char)payload[i];
  }
  LastCommand = pay;
  HomeHub_DEBUG_PORT.println("Cloud Incomming data : " + pay);
  String variable = pay.substring(0,5);
  String value = pay.substring(5,pay.length()-1);
  if(variable == "<SYN>"){
    error = "000"; //Data Recieved and matched
  }
  String command = "<ERR>"+error+"X";
  char ToBeSent[11];
  command.toCharArray(ToBeSent,11);
  publish_mqtt(ToBeSent);
  HomeHub_DEBUG_PORT.println("Published with topic : "+Device_Id_As_Publish_Topic);
  HomeHub_DEBUG_PORT.println("& Payload : "+command);
}
    
void HomeHub::publish_mqtt(String message)
{
    mqttclient.publish(Device_Id_In_Char_As_Publish_Topic, message.c_str(), true);
}

void HomeHub::publish_mqtt(const char* topic, String message)
{
    mqttclient.publish(topic, message.c_str());
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
            HomeHub_DEBUG_PORT.println("HTTP_UPDATE_FAILD Error (%d): %s\n");//,ESPhttpUpdate.getLastError(),ESPhttpUpdate.getLastErrorString().c_str());
        break;

      case HTTP_UPDATE_NO_UPDATES:
        HomeHub_DEBUG_PORT.println("HTTP_UPDATE_NO_UPDATES");
        break;

      case HTTP_UPDATE_OK:
        HomeHub_DEBUG_PORT.println("HTTP_UPDATE_OK");
        break;
    }
    }
    else{
        HomeHub_DEBUG_PRINT("Device is Offline, cant update.");
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
        Serial.println(qsid);
        Serial.println("");
        String qpass;
        qpass = req.substring(req.lastIndexOf('=')+1);
        Serial.println(qpass);
        Serial.println("");
        
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
