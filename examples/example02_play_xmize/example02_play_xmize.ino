#include <xm_t3.h>
#include "shooting_star_libxmize.h"

void setup() {
	// Loading the module in libxmize format results in:
	//		104 kB of flash memory used
	//		10 kB of RAM used for module
	xm_player_xmize( shooting_star_libxmize );
	xm_player_start();
}

void loop() {
	xm_player_update();
}
