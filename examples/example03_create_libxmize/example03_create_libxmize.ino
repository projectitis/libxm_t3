#include <xm_t3.h>
#include "shooting_star.h"

/**
 * This example shows how to create the "libxmize" version of the XM module file. Using
 * the libxmize version saves about 90% of memory during playback. Most of the data is
 * stored and read directly from flash.
 * 
 * There are two steps to libxmize a file:
 * 
 * 1) Convert the original XM file to a header file using xm_to_h.py (or an alternative tool e.g. bin2c)
 * 2) Add the header to a sketch and pass it to xm_libxmize (this sketch)
 * 3) Make sure you have serial monitor running
 * 4) Copy/paste the contents from serial to a new .h file. This is the libxmized file!
 */

void setup() {
	// Open serial for output, then wait a couple of second for it to initialise
	Serial.begin(9600);
	delay(2000);
 	// shooting_star and shooting_star_size are both defined in shooting_star.h
 	// Running the including python script xm_to_h.py will generate this header for you
	xm_libxmize( shooting_star, shooting_star_size );
}

void loop() {
  	// We are done. Nothing to do
	delay(100);
}
