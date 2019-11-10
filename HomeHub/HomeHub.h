/*
 * HomeHub.h - An TNM Home Automation Library
 * Created by Vikhyat Chauhan @ TNM on 9/11/19
 * www.thenextmove.in
 * Revision #5 - See readMe 
 */

//	The #ifndef statement checks to see if the HomeHub.h
//	file isn't already defined. This is to stop double declarations
//	of any identifiers within the library. It is paired with a
//	#endif at the bottom of the header and this setup is known as 
//	an 'Include Guard'. 
#ifndef HomeHub_h

//	The #define statement defines this file as the HomeHub
//	Header File so that it can be included within the source file.                                           
#define HomeHub_h

/* Setup debug printing macros. */
// Uncomment the following HomeHub_DEBUG to enable debug output.
#define HomeHub_DEBUG

// Debug output destination can be defined externally with AC_DEBUG_PORT
#ifndef HomeHub_DEBUG_PORT
#define HomeHub_DEBUG_PORT Serial
#endif

#ifndef HomeHub_DATA_PORT
#define HomeHub_DATA_PORT Serial
#endif

#ifndef HomeHub_DATA_PORT_BAUD
#define HomeHub_DATA_PORT_BAUD 9600
#endif

#ifdef HomeHub_DEBUG
#define HomeHub_DEBUG_PRINT(...) do {HomeHub_DEBUG_PORT.print("[HomeHub] : "); HomeHub_DEBUG_PORT.printf( __VA_ARGS__ );HomeHub_DEBUG_PORT.println("");} while (0)
#else
#define HomeHub_DEBUG_PRINT(...)
#endif

//	The #include of Arduino.h gives this library access to the standard
//	Arduino types and constants (HIGH, digitalWrite, etc.). It's 
//	unneccesary for sketches but required for libraries as they're not
//	.ino (Arduino) files.
#include "Arduino.h"
#include "WiFiManager.h"
//#include "EEPROM.h"

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <pgmspace.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <pgmspace.h>
#else
#error Platform not supported
#endif

const char HTTP_EN[] PROGMEM = "</div></body></html>";

extern "C" {
#include "user_interface.h"
} 

//	The class is where all the functions for the library are stored,
//	along with all the variables required to make it operate
class HomeHub{

	//	'public:' and 'private:' refer to the security of the functions
	//	and variables listed in that set. Contents that are public can be 
	//	accessed from a sketch for use, however private contents can only be
	//	accessed from within the class itself.
	public:
	
		//	The first item in the class is known as the constructor. It shares the
		//	same name as the class and is used to create an instance of the class.
		//	It has no return type and is only used once per instance.	
		HomeHub();

		void asynctasks();

		//	Below are the functions of the class. They are the functions available
		//	in the library for a user to call.

	private:                  
		
		//	When dealing with private variables, it is common convention to place
		//	an underscore before the variable name to let a user know the variable
		//	is private.		

		//Private Variables
		int _DATA_PORT_counter = 0;
		bool _initiate_AP = false;
		String _DATA_PORT_command = "";

		//Private Functions 
		void DATA_PORT_getcommand();
		void start_hotspot();
		void wifi_connect();
};

//	The end wrapping of the #ifndef Include Guard
#endif