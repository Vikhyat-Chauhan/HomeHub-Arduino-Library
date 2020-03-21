#include "ESP8266WiFi.h"
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <EEPROM.h>

MDNSResponder mdns;
WiFiServer server(80);

String ssid_string = "TNM" + String(ESP.getChipId());
String wifi_data,esid, epass="";
bool saved_wifi_present_flag = false;

void setup() {
  Serial.begin(9600);
  EEPROM.begin(512);
  retrieve_wifi_data();
  initiate_wifi_setup();
}

void loop() {
    if(saved_wifi_present_flag == false){
      wifi_setup_webhandler(scan_networks());
    }
    else{
      end_wifi_setup();
      saved_wifi_connect();
      start_server();
      while(1){
        normal_webhandler();
      }
    }
}

int wifi_setup_webhandler(String wifi_data)
{
  // Check for any mDNS queries and send responses
  mdns.update();
  
  // Check if a client has connected
  WiFiClient client = server.available();
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
        s += wifi_data;
        s += "<form method='get' action='a'><label>SSID: </label><input name='ssid' length=32><input name='pass' length=64><input type='submit'></form>";
        s += "</html>\r\n\r\n";
        Serial.println("Sending 200");
      }
      else if ( req.startsWith("/a?ssid=") ) {
        // /a?ssid=blahhhh&pass=poooo
        Serial.println("clearing eeprom");
        for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
        String qsid; 
        qsid = req.substring(8,req.indexOf('&'));
        Serial.println(qsid);
        Serial.println("");
        String qpass;
        qpass = req.substring(req.lastIndexOf('=')+1);
        Serial.println(qpass);
        Serial.println("");
        
        Serial.println("writing eeprom ssid:");
        for (int i = 0; i < qsid.length(); ++i)
          {
            EEPROM.write(i, qsid[i]);
            Serial.print("Wrote: ");
            Serial.println(qsid[i]); 
          }
        Serial.println("writing eeprom pass:"); 
        for (int i = 0; i < qpass.length(); ++i)
          {
            EEPROM.write(32+i, qpass[i]);
            Serial.print("Wrote: ");
            Serial.println(qpass[i]); 
          }    
        EEPROM.commit();
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

int normal_webhandler()
{
  // Check for any mDNS queries and send responses
  mdns.update();
  
  // Check if a client has connected
  WiFiClient client = server.available();
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
    for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
    EEPROM.commit();
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

char retrieve_wifi_data(){
  esid = "";
  epass = "";
  for (int i = 0;i < 32;++i)
    {
      esid += char(EEPROM.read(i));
    } 
  Serial.print("SSID:");Serial.println(esid);
  epass = "";
  for(int i = 32;i < 96;++i)
    {
      epass += char(EEPROM.read(i));
    }
  Serial.print("PASS:");Serial.println(epass); 
  if(esid.length() > 1){
    return 's'; 
  }
  else{
    return 'f';
  }
}

char test_wifi() {
  Serial.println("Waiting for Wifi to connect");  
  for(int i=0;i<100;i++){ 
    delay(100);
    if(WiFi.status() == 3){
      break ;
    }
  }
  if (WiFi.status() == WL_CONNECTED){
    return('s');
  } 
  else{
    Serial.println("Connect timed out");
    return('f');
  }
} 

String scan_networks(){
  //WiFi.mode(WIFI_STA);
  //WiFi.disconnect();
  
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
  
  String wifi_data = "<ul>";
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
        if(current_wifi.charAt(j) == esid.charAt(j)){
          test_counter++;
        }
      }
      if(test_counter == current_wifi.length()){ 
        Serial.println("Saved Wifi present");
        saved_wifi_present_flag = true;
      }
    }
  wifi_data += "</ul>";
  return wifi_data;
}

void start_server(){
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.softAPIP());
          
  if(!mdns.begin("esp8266", WiFi.localIP())){
    Serial.println("Error setting up MDNS responder!");
    while(1){ 
      delay(1000);
    }
  }
  
  Serial.println("mDNS responder started");
  // Start the server
  server.begin();
  Serial.println("Server started");
}

char stop_server(){
  mdns.close();
  Serial.println("mDNS responder ended");
  // Start the server
  server.stop();
  server.close();
  Serial.println("Server ended");
  return 's';
}

char initiate_ap(){
  WiFi.mode(WIFI_AP);
  WiFi.disconnect();
  delay(100);
  //technically add finding wifi data into this
  const char* ssid = ssid_string.c_str();
  WiFi.softAP(ssid);
  Serial.println("softap");
}

char end_ap(){
  WiFi.softAPdisconnect();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  return(saved_wifi_connect());
}

char saved_wifi_connect(){
  if(retrieve_wifi_data() == 's'){
    WiFi.begin(esid.c_str(), epass.c_str());
    return (test_wifi());
  }
}

char initiate_wifi_setup(){
  wifi_data = scan_networks();
  initiate_ap();
  start_server();
}

char end_wifi_setup(){
  stop_server();
  end_ap();
}
