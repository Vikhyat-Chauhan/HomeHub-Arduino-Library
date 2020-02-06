/*
   Captive Portal by: M. Ray Burnette 20150831
   See Notes tab for original code references and compile requirements
   Sketch uses 300,640 bytes (69%) of program storage space. Maximum is 434,160 bytes.
   Global variables use 50,732 bytes (61%) of dynamic memory, leaving 31,336 bytes for local variables. Maximum is 81,920 bytes.
*/

#include <ESP8266WiFi.h>
#include "./DNSServer.h"                  // Patched lib
#include <ESP8266WebServer.h>

const byte        DNS_PORT = 53;          // Capture DNS requests on port 53
IPAddress         apIP(10, 10, 10, 1);    // Private network for server
DNSServer         dnsServer;              // Create the DNS object
ESP8266WebServer  webServer(80);          // HTTP server

String responseHTML = ""
                      "<!DOCTYPE html><html><head><title>CaptivePortal</title></head><body>"
                      "<form name=\"myform\" action=\"http://www.mydomain.com/myformhandler.cgi\" method=\"POST\"><div align=\"center\"><br><br><input type=\"text\" size=\"25\" value=\"Enter your name here!\"><br><input type=\"submit\" value=\"Send me your name!\"><br></div></form>"
  
                      "</body></html>";

char htmlResponse[3000];

void handleRoot() {
  snprintf ( htmlResponse, 3000,
"<!DOCTYPE html>\
<html lang=\"en\">\
  <head>\
    <meta charset=\"utf-8\">\
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
  </head>\
  <body>\
          <h1>Time</h1>\
          <input type='text' name='date_hh' id='date_hh' size=2 autofocus> hh \
          <input type='text' name='date_mm' id='date_mm' size=2 autofocus> mm \
          <input type='text' name='date_ss' id='date_ss' size=2 autofocus> ss \
          <div>\
          <br><button id=\"save_button\">Save</button>\
          </div>\
    <script src=\"https://ajax.googleapis.com/ajax/libs/jquery/1.11.3/jquery.min.js\"></script>\    
    <script>\
      var hh;\
      var mm;\
      var ss;\
      $('#save_button').click(function(e){\
        e.preventDefault();\
        hh = $('#date_hh').val();\
        mm = $('#date_mm').val();\
        ss = $('#date_ss').val();\        
        $.get('/save?hh=' + hh + '&mm=' + mm + '&ss=' + ss, function(data){\
          console.log(data);\
        });\
      });\      
    </script>\
  </body>\
</html>"); 
   webServer.send ( 200, "text/html", htmlResponse );  
}


void handleSave() {
  if (webServer.arg("hh")!= ""){
    Serial.println("Hours: " + webServer.arg("hh"));
  }

  if (webServer.arg("mm")!= ""){
    Serial.println("Minutes: " + webServer.arg("mm"));
  }

  if (webServer.arg("ss")!= ""){
    Serial.println("Seconds: " + webServer.arg("ss"));
  }

}

void setup() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("APName");
  Serial.begin(9600);
  // if DNSServer is started with "*" for domain name, it will reply with
  // provided IP to all DNS request
  dnsServer.start(DNS_PORT, "*", apIP);

  // replay to all requests with same HTML
  webServer.onNotFound([]() {
    //webServer.send(200, "text/html", responseHTML);
    handleRoot();
  });
  webServer.begin();
  webServer.on ( "/", handleRoot );
  webServer.on ("/save", handleSave);
}

void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();
}



