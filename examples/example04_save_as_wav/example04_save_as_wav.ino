#include <xm_t3.h>
#include "shooting_star.h"

void setup() {
	xm_player_xm( shooting_star, shooting_star_size );
	
	// Note: This can take a long time (a few minutes). I still need to optimize
	// the code that writes to SD.
	xm_player_save( "sh_star.wav", SAVETYPE_WAV );
}

void loop() {
  	// We are done. Nothing to do
	delay(100);
}
