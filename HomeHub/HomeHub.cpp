/*
 * myFirstLibrary.cpp - An introduction to library setup
 * Created by Christian @ Core Electronics on 1/06/18
 * Revision #5 - See readMe
 */

//	This will include the Header File so that the Source File has access
//	to the function definitions in the myFirstLibrary library.
#include "HomeHub.h" 

//	The #include of Arduino.h gives this library access to the standard
//	Arduino types and constants (HIGH, digitalWrite, etc.). It's 
//	unneccesary for sketches but required for libraries as they're not
//	.ino (Arduino) files. 
#include "Arduino.h"
#include <Wire.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <pgmspace.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <pgmspace.h>
#else
#error Platform not supported
#endif

//	This is where the constructor Source Code appears. The '::' indicates that
//	it is part of the myFirstLibrary class and should be used for all constructors
//	and functions that are part of a class.
HomeHub::HomeHub(){

	//	The arguments of the constructor are then saved into the private variables.
	HomeHub_DEBUG_PORT.begin(HomeHub_DEBUG_PORT_BAUD);
	HomeHub_SLAVE_DATA_PORT.begin(HomeHub_SLAVE_DATA_PORT_BAUD);
	Wire.begin();
	//rom_write(2, 2);
	//EEPROM.begin(512);
	//if (EEPROM.read(300) == 0) { WiFiManager wifiManager;wifiManager.startConfigPortal();HomeHub_DEBUG_PRINT("Starting AP"); }
	//wifi_connect();
	HomeHub_DEBUG_PRINT("STARTED");
}

void HomeHub::asynctasks(){
	slave_handler();
}

void HomeHub::slave_handler(){
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
				/*char ToBeSent[9];
				_DATA_PORT_command.toCharArray(ToBeSent, 9);
				client.publish(Device_Id_In_Char_As_Publish_Topic, ToBeSent, true);*/
				_SLAVE_DATA_PORT_command = "";
			}
		}
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
	delay(10); //Delay introduction solved the data save unreliability
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



