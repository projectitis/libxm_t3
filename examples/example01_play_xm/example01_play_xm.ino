#include <xm_t3.h>
#include "shooting_star.h"

void setup() {
	// Loading the module in xm format results in:
	//		106 kB of flash memory used
	//		126 kB of RAM used for module
	xm_player_xm( shooting_star, shooting_star_size );
	xm_player_start();
}

void loop() {
	xm_player_update();
}
