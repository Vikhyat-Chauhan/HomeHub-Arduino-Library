#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>


// WiFi network
const char* ssid     = "OnePlus 3";
const char* password = "0987654321";

ESP8266WebServer server ( 80 );



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

   server.send ( 200, "text/html", htmlResponse );  

}


void handleSave() {
  if (server.arg("hh")!= ""){
    Serial.println("Hours: " + server.arg("hh"));
  }

  if (server.arg("mm")!= ""){
    Serial.println("Minutes: " + server.arg("mm"));
  }

  if (server.arg("ss")!= ""){
    Serial.println("Seconds: " + server.arg("ss"));
  }

}


void setup() {

  // Start serial
  Serial.begin(115200);
  delay(10);

  // Connecting to a WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.on ( "/", handleRoot );
  server.on ("/save", handleSave);

  server.begin();
  Serial.println ( "HTTP server started" );


}

void loop() {
  server.handleClient();
}
