//#include "xm_player.h"
#include <xm_t3.h>
#include "_sunlight_.h"

void setup() {
	Serial.begin(9600);
	delay(2000); // Wait for user to enable serial logger
	xm_libxmize( _sunlight_, _sunlight__size );
}

void loop() {
	// Do nothing
	delay(100);
}
