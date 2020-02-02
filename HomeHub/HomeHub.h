/*
 * HomeHub.h - An TNM Home Automation Library
 * Created by Vikhyat Chauhan @ TNM on 9/11/19
 * www.thenextmove.in
 * Revision #6 - See readMe
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
#define HomeHub_DEBUG_PORT Serial
#define HomeHub_DEBUG_PORT_BAUD 9600

#define HomeHub_SLAVE_DATA_PORT Serial
#define HomeHub_SLAVE_DATA_PORT_BAUD 9600

#ifdef HomeHub_DEBUG
#define HomeHub_DEBUG_PRINT(...) do {HomeHub_DEBUG_PORT.print("[HomeHub] : "); HomeHub_DEBUG_PORT.printf( __VA_ARGS__ );HomeHub_DEBUG_PORT.println("");} while (0)
#else
#define HomeHub_DEBUG_PRINT(...)
#endif

#define ROM_ADDRESS 0x57

//	The #include of Arduino.h gives this library access to the standard
//	Arduino types and constants (HIGH, digitalWrite, etc.). It's 
//	unneccesary for sketches but required for libraries as they're not
//	.ino (Arduino) files.
#include "Arduino.h"

#include <ESP8266mDNS.h>
#include <WiFiClient.h>

//const char HTTP_EN[] PROGMEM = "</div></body></html>";

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
		//Public functions are defined here
		void asynctasks();
		void rom_write(unsigned int eeaddress, byte data);
		byte rom_read(unsigned int eeaddress);
        char initiate_wifi_setup();
        char end_wifi_setup();
        char saved_wifi_connect();

	private:
        //std::unique_ptr<MDNSResponder> mdns;
        //std::unique_ptr<WiFiServer> server;
		MDNSResponder* mdns;
        WiFiServer* server;
		//	When dealing with private variables, it is common convention to place
		//	an underscore before the variable name to let a user know the variable
		//	is private.		

		//Private Variables
        String _ssid_string = "TNM" + String(ESP.getChipId());
        String _wifi_data;
        String _esid = "";
        String _epass = "";
        bool _saved_wifi_present_flag = false;

		int _SLAVE_DATA_PORT_counter = 0;
		bool _initiate_AP = false;
		String _SLAVE_DATA_PORT_command = "";
		String _slave_output_buffer = "";
    
        // DNS server
        const byte    DNS_PORT = 53;

		//Private Functions 
		void slave_handler();
        void start_server();
        char stop_server();
        char initiate_ap();
        char end_ap();
        int wifi_setup_webhandler(String _wifi_data);
        int normal_webhandler();
        char retrieve_wifi_data();
        char test_wifi();
        String scan_networks();
};

//	The end wrapping of the #ifndef Include Guard
#endif
