/*  
 *	Name: HomeHub First Example
 *  Date Created: 9/11/2019
 *  Description: Example code using the HomeHub library.
 * 
 *		See: https://www.thenextmove.in
 */

//  Include relevant libraries
#include <HomeHub.h>

//  Create an instance of the myFirstLibrary class called 'mySetup' 
HomeHub homehub;

void setup(){
    homehub.initiate_wifi_setup();
}

void loop(){
	// Run the on function
	homehub.asynctasks();
}
