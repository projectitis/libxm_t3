# libxm_t3
Port of libxm (by Artefact2) for Teensy 3x microprocessors

## Status

Working, but some playback bugs (see below).

## Prerequisites

Only tested on Teensy 3.6 but should work on T3.5 and T3.2 as well. You will need:
	
* A Teensy microprocessor board
* Arduino IDE with Teensyduino installed
* An amplifier or preamp circuit to power your
* Speakers or sound system
* An XM module

Loading the included example module in it's original file format results in 126k of RAM usage. However, converting it to a special format first (libxmize) allows it to be stored in flash mem, and uses a smaller RAM footprint of only 10k (around 90% saving).

## Hardware setup

Hardware settings, such as stereo/mono, pins and sample rate, are defined at the top of xm_t3.h - feel free to edit them for your setup.

### Teensy 3.5 and 3.6

Stereo output is on the two DAC pins DAC0/A21 and DAC1/A22. Connect these to your preamp and then on to your powered speakers. Connect the ground of your preamp to AGND (not GND).

The default sample rate on these boards is 48kHz.

### Teensy 3.2

This board has only one DAC output. The stereo channels are mixed and output as mono on pin DAC/A14. Connect this to your preamp and then on to your powered speakers. Connect the ground of your preamp to AGND (not GND).

The default sample rate on this board is 24kHz.

## Software setup

Very little code is required to load and play an XM module, but you'll need to convert the XM file to a special format (libxmize) first.

This is the minimal code to turn your Teensy in to a modplayer:

```
#include <xm_t3.h>
#include "shooting_star_libxmize.h"

void setup() {
	xm_player_xmize( shooting_star_libxmize );
	xm_player_start();
}

void loop() {
	xm_player_update();
}
```

## Examples

The included examples use a module called [Shooting star](https://modarchive.org/index.php?request=view_by_moduleid&query=133691) by dalexy. This mod is used solely for the purposes of demonstrating the XM player on Teensy. If you wish to use this mod in your own projects or commercially you'll need to seek the permission of the copyright holder.
	
There are several examples included:
	
* **example01_play_xm** Loads and plays an XM module in original format. 106kB of flash and 126kB of RAM used
* **example02_play_xmize** Loads and plays an XM module in libxmize format. 104kB of flash and 10kB of RAM used
* **example03_create_libxmize** Shows how to create a libxmized version of an XM mod.
* **example04_save_as_wav** Saves the XM modle to the SD card as a WAV file.

## Teensy port details

The following is a list of broad changes to the original code to support the Teensy microprocessor board:
	
* Ensure structs aligned on word boundaries to avoid unaligned reads
* Changed stdout debug functions to use serial
* Changed libxmize code to outout header file code to serial instead of save a file
* Thanks to Artefact2, wrote a libxmize loader that uses 90% less RAM for playback
* Added a player based on IntervalTimer and ring buffer

### Known bugs

* Saving to SD card is slow. Need to optimize this.

### Todo

* Implement seek (jumping to arbitrary parts of the song)
* Loading from SD card
* More examples

## Authors

* **Romain "Artefact2" Dalmaso** - *Initial work* - [artefact2@gmail.com](mailto:artefact2@gmail.com)
* **Dan Spencer** - *Initial work contributor* - [dan@atomicpotato.net](mailto:dan@atomicpotato.net)
* **Peter "Projectitis" Vullings** - *Teensy port* - [peter@projectitis.com](mailto:peter@projectitis.com)

## License

The Teensy port is covered by the same WTF licence as the original work this is based on.

This program is free software. It comes without any warranty, to the extent permitted by applicable law. You can redistribute it and/or modify it under the terms of the Do What The Fuck You Want To Public License, Version 2, as published by Sam Hocevar. See [http://sam.zoy.org/wtfpl/COPYING](http://sam.zoy.org/wtfpl/COPYING) for more details.

## Acknowledgements

* [Mod Archive](https://modarchive.org/) - Where all the cool mods live
* [libxm](https://github.com/Artefact2/libxm) - libxm repo on github
* [Teensy](https://www.pjrc.com/teensy/) - Teensy microprocessor boards
* [Dillinger](https://dillinger.io) - Online Markdown editor for this readme
