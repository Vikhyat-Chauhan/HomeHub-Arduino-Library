/*
 * myFirstLibrary.cpp - An introduction to library setup
 * Created by Christian @ Core Electronics on 1/06/18
 * Revision #5 - See readMe
 */

//	The #include of Arduino.h gives this library access to the standard
//	Arduino types and constants (HIGH, digitalWrite, etc.). It's 
//	unneccesary for sketches but required for libraries as they're not
//	.ino (Arduino) files. 
#include "Arduino.h"

//	This will include the Header File so that the Source File has access
//	to the function definitions in the myFirstLibrary library.
#include "HomeHub.h" 

//	This is where the constructor Source Code appears. The '::' indicates that
//	it is part of the myFirstLibrary class and should be used for all constructors
//	and functions that are part of a class.
HomeHub::HomeHub(){

	//	The arguments of the constructor are then saved into the private variables.
	Serial.begin(HomeHub_DATA_PORT_BAUD);
	//EEPROM.begin(512);
	//if (EEPROM.read(300) == 0) { WiFiManager wifiManager;wifiManager.startConfigPortal();HomeHub_DEBUG_PRINT("Starting AP"); }
	//wifi_connect();
	HomeHub_DEBUG_PRINT("STARTED");
}

void HomeHub::asynctasks(){
	DATA_PORT_getcommand();
}

void HomeHub::DATA_PORT_getcommand(){
	if (HomeHub_DATA_PORT.available()) {
		char c = HomeHub_DATA_PORT.read();
		if (_DATA_PORT_counter == 1) {
			_DATA_PORT_command += c;
		}
		if (c == '<') {
			_DATA_PORT_counter = 1;
		}
		if (c == 'X') {
			_DATA_PORT_counter = 0;
			_DATA_PORT_command = "<" + _DATA_PORT_command;
			String variable = _DATA_PORT_command.substring(0, 5);
			String value = _DATA_PORT_command.substring(5, _DATA_PORT_command.length() - 1);
			//Selected commands only to read by esp offline
			if (variable == "<WSP>") {
				_initiate_AP = true;
				start_hotspot();
				_DATA_PORT_command = "";
			}
			else if (variable == "<DE1>") {
					/*char ToBeSent[9];//
					_DATA_PORT_command.toCharArray(ToBeSent, 9);//
					client.publish(Device_Id_In_Char_As_Publish_Topic, ToBeSent, true);*/
					_DATA_PORT_command = "";
			}
			else if (variable == "<DE2>") {
					/*char ToBeSent[9];//
					_DATA_PORT_command.toCharArray(ToBeSent, 9);//
					client.publish(Device_Id_In_Char_As_Publish_Topic, ToBeSent, true);*/
					_DATA_PORT_command = "";
			}
			//For everything that is not for ESP it will be normally sent online 
			else {
				HomeHub_DATA_PORT.println(_DATA_PORT_command);
				/*char ToBeSent[9];
				_DATA_PORT_command.toCharArray(ToBeSent, 9);
				client.publish(Device_Id_In_Char_As_Publish_Topic, ToBeSent, true);*/
				_DATA_PORT_command = "";         
			}
		}
	}
	HomeHub_DATA_PORT.flush();
}

void HomeHub::start_hotspot() {
	WiFiManager wifiManager;
	wifiManager.resetSettings();HomeHub_DEBUG_PRINT("settings reset");
	delay(1000);
	//wifiManager.startConfigPortal();debug("Starting AP");
	_initiate_AP = false;
	//EEPROM.write(300, 0);
	//EEPROM.commit();delay(200);
	ESP.reset();
}

void HomeHub::wifi_connect() {
	//WiFiManager
	//Local intialization. Once its business is done, there is no need to keep it around
	WiFiManager wifiManager;
	bool x = wifiManager.autoConnect();
	/*if (x == true) {
		if (EEPROM.read(300) == 0) {
			EEPROM.write(300, 1);EEPROM.commit();delay(100);ESP.reset();
		}
		deviceonline_tasks();
	}*/
}
