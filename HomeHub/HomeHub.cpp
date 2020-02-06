/*
* HomeHub.cpp - An TNM Home Automation Library
* Created by Vikhyat Chauhan @ TNM on 9/11/19
* www.thenextmove.in
* Revision #8 - See readMe
*/

#include "HomeHub.h"
#include <Arduino.h>

WiFiClient _Client;
PubSubClient mqttclient(_Client);

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
    ticker->attach_ms(100, std::bind(&HomeHub::slave_handler, this));
    //Retrieve saved wifi data from Rom
    retrieve_wifi_data();
	HomeHub_DEBUG_PRINT("STARTED");
}

void HomeHub::asynctasks(){
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
	if (HomeHub_SLAVE_DATA_PORT.available()) {
		char c = HomeHub_SLAVE_DATA_PORT.read();
		if (_SLAVE_DATA_PORT_counter == 1) {
			_SLAVE_DATA_PORT_command += c;
		}
		if (c == '<') {
			_SLAVE_DATA_PORT_counter = 1;
		}
		if (c == 'X') {
			_SLAVE_DATA_PORT_counter = 0;
			_SLAVE_DATA_PORT_command = "<" + _SLAVE_DATA_PORT_command;
			String variable = _SLAVE_DATA_PORT_command.substring(0, 5);
			String value = _SLAVE_DATA_PORT_command.substring(5, _SLAVE_DATA_PORT_command.length() - 1);
			//Selected commands only to read by esp offline
			if (variable == "<WSP>") {
				_initiate_AP = true;
				//start_hotspot();
				_SLAVE_DATA_PORT_command = "";
			}
			else if (variable == "<WRI>") {
				int i = value.toInt();
				rom_write(2, i);
				_SLAVE_DATA_PORT_command = "";
			}
			else if (variable == "<REA>") {
				HomeHub_DEBUG_PORT.println(rom_read(2));
				_SLAVE_DATA_PORT_command = "";
			}
			//For everything that is not for ESP it will be normally sent online 
			else {
				HomeHub_DEBUG_PORT.println(_SLAVE_DATA_PORT_command);
				publish_mqtt(_SLAVE_DATA_PORT_command);
				_SLAVE_DATA_PORT_command = "";
			}
		}
	}
	//Write the buffer Command on bits per cycle basis
	if(_slave_output_buffer.length() > 0){
		char c = _slave_output_buffer.charAt(0);
		_slave_output_buffer = _slave_output_buffer.substring(1, _slave_output_buffer.length());
		HomeHub_SLAVE_DATA_PORT.write(c);
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
  Serial.println("");
  Serial.println("New client");

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
    Serial.print("Invalid request: ");
    Serial.println(req);
    return(20);
   }
  req = req.substring(addr_start + 1, addr_end);
  Serial.print("Request: ");
  Serial.println(req);
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
        Serial.println("Sending 200");
      }
      else if ( req.startsWith("/a?ssid=") ) {
        // /a?ssid=blahhhh&pass=poooo
        Serial.println("clearing eeprom");
        saved_wifi_dump();
        String qsid;
        qsid = req.substring(8,req.indexOf('&'));
        Serial.println(qsid);
        Serial.println("");
        String qpass;
        qpass = req.substring(req.lastIndexOf('=')+1);
        Serial.println(qpass);
        Serial.println("");
        
        Serial.println("writing eeprom ssid:");
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
        Serial.println("Sending 404");
      }
  client.print(s);
  Serial.println("Done with client");
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
  Serial.println("");
  Serial.println("New client");

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
    Serial.print("Invalid request: ");
    Serial.println(req);
    return(20);
   }
  req = req.substring(addr_start + 1, addr_end);
  Serial.print("Request: ");
  Serial.println(req);
  client.flush();
  String s;
  
  if (req == "/")
  {
     s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266";
     s += "<p>";
     s += "</html>\r\n\r\n";
     Serial.println("Sending 200");
  }
  else if ( req.startsWith("/cleareeprom") ) {
    s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266";
    s += "<p>Clearing the EEPROM<p>";
    s += "</html>\r\n\r\n";
    Serial.println("Sending 200");
    Serial.println("clearing eeprom");
    for (int i = 0; i < 96; ++i) {
        rom_write(i,0);// EEPROM.write(i, 0);
    }
    //EEPROM.commit();
  }
  else
  {
   s = "HTTP/1.1 404 Not Found\r\n\r\n";
   Serial.println("Sending 404");
  }
  client.print(s);
  Serial.println("Done with client");
  return(20);
}

bool HomeHub::retrieve_wifi_data(){
  _esid = "";
  _epass = "";
  for (int i = 0;i < 32;++i)
    {
      _esid += char(rom_read(i)); //_esid += char(EEPROM.read(i));
    }
  Serial.print("SSID:");Serial.println(_esid);
  _epass = "";
  for(int i = 32;i < 96;++i)
    {
      _epass += char(rom_read(i)); //_epass += char(EEPROM.read(i));
    }
  Serial.print("PASS:");Serial.println(_epass);
  if(_esid.length() > 1){
    return true;
  }
  else{
    return false;
  }
}

bool HomeHub::test_wifi() {
  Serial.println("Waiting for Wifi to connect");
  for(int i=0;i<100;i++){
    delay(100);
    if(WiFi.status() == 3){
      break ;
    }
  }
  if (WiFi.status() == WL_CONNECTED){
    return true;
  }
  else{
    Serial.println("Connect timed out");
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
        Serial.println("Saved Wifi present");
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
  Serial.println("mDNS responder started");
    
  server->begin(); // Web server start
  Serial.println("Server started");
}

bool HomeHub::stop_server(){
  mdns->close();
  Serial.println("mDNS responder ended");
  // Start the server
  server->stop();
  server->close();
  Serial.println("Server ended");
  return true;
}

bool HomeHub::initiate_ap(){
  WiFi.mode(WIFI_AP);
  WiFi.disconnect();
  delay(100);
  //technically add finding wifi data into this
  const char* ssid = _ssid_string.c_str();
  WiFi.softAP(ssid);
  Serial.println("softap");
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
    return (test_wifi());
  }
}

void HomeHub::save_wifi_data(String ssid, String password){
    for (int i = 0; i < ssid.length(); ++i)
      {
        rom_write(i, ssid[i]); //EEPROM.write(i, qsid[i]);
        Serial.print("Wrote: ");
        Serial.println(ssid[i]);
        _esid = ssid;
      }
    Serial.println("writing eeprom pass:");
    for (int i = 0; i < password.length(); ++i)
      {
        rom_write(32+i, password[i]); //EEPROM.write(32+i, qpass[i]);
        Serial.print("Wrote: ");
        Serial.println(password[i]);
        _epass = password;
      }
}

void HomeHub::saved_wifi_dump(){
  Serial.println("clearing eeprom");
  for (int i = 0; i < 96; ++i) {
      rom_write(i,0);// EEPROM.write(i, 0);
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
        Serial.print("Failed to connect to mqtt server, rc=");
        Serial.print(mqttclient.state());
        Serial.println("");
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
  Serial.println("Cloud Incomming data : " + pay);
  String variable = pay.substring(0,5);
  String value = pay.substring(5,pay.length()-1);
  if(variable == "<SYN>"){
    error = "000"; //Data Recieved and matched
  }
  String command = "<ERR>"+error+"X";
  char ToBeSent[11];
  command.toCharArray(ToBeSent,11);
  publish_mqtt(ToBeSent);
  Serial.println("Published with topic : "+Device_Id_As_Publish_Topic);
  Serial.println("& Payload : "+command);
}
    
void HomeHub::publish_mqtt(String message)
{
    mqttclient.publish(Device_Id_In_Char_As_Publish_Topic, message.c_str(), true);
}

void HomeHub::publish_mqtt(const char* topic, String message)
{
    mqttclient.publish(topic, message.c_str());
}
