/* Author: Peter "Projectitis" Vullings <peter@projectitis.com> */
/* Website: https://github.com/Artefact2/libxm */

/* This program is free software. It comes without any warranty, to the
 * extent permitted by applicable law. You can redistribute it and/or
 * modify it under the terms of the Do What The Fuck You Want To Public
 * License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details. */
 
#include "xm_t3.h"
#include <SD.h>

extern "C" {
	#ifdef XM_HAS_OWN_STDOUT
		#include <stdarg.h>
		void xm_stdout( const char *str ){
			Serial.print(str);
		}
	#endif
	#include "xm_internal.h"
}

void xm_delay( uint32_t ms ){
	delay( ms );
}

/**
 * The context containing info about the mod
 **/
static xm_context_t* _context = NULL;
		
/**
 * Interval timer for actual playback
 **/
static IntervalTimer _timer;

/**
 * Flag indicated state of player
 **/
static bool _started = false;

/**
 * Ring buffer for samples
 **/
static float* _buffer;
static uint16_t _bufferlen;	// sample pairs
static uint16_t _buffersize;	// samples
static uint16_t _bufferhead;	// index of head (writing)
static volatile uint16_t _buffertail;	// index of tail (reading)
static volatile uint16_t _bufferavail;	// available samples

/**
 * Initialise the mod player
 * @param	moddata		The mod data as a libxmized format (non-delta coded)
 **/
void xm_player_xmize( const char* moddata ){
	if (_context) return;
	
	// Create context
	xm_create_shared_context_from_libxmize(
		&_context,
		moddata,
		XM_SAMPLE_RATE
	);
	// Create buffer
	_bufferlen = uint16_t(XM_SAMPLE_RATE / 480);	// Number of sample pairs (L+R)
	_buffersize = _bufferlen * 2;					// Size of buffer in samples
	_buffer = new float[ _buffersize ];
	_buffertail = 0;								// We read data from tail
	_bufferhead = 0;								// We put data in from head (until we reach tail)
	_bufferavail = 0;								// No samples data available yet
	
	// Set up pins
	analogWriteResolution(12);
	#ifdef XM_STEREO
		pinMode ( XM_PIN_L, OUTPUT );
		pinMode ( XM_PIN_R, OUTPUT );
	#else
		pinMode ( XM_PIN_L, OUTPUT );
	#endif
}
void xm_player_xm( const char* moddata, uint32_t moddata_size ){
	if (_context) return;
	
	// Create context
	xm_create_context_safe(
		&_context,
		moddata,
		moddata_size,
		XM_SAMPLE_RATE
	);
	
	// Create buffer
	_bufferlen = uint16_t(XM_SAMPLE_RATE / 480);	// Number of sample pairs (L+R)
	_buffersize = _bufferlen * 2;					// Size of buffer in samples
	_buffer = new float[ _buffersize ];
	_buffertail = 0;								// We read data from tail
	_bufferhead = 0;								// We put data in from head (until we reach tail)
	_bufferavail = 0;								// No samples data available yet
	
	// Set up pins
	analogWriteResolution(12);
	#ifdef XM_STEREO
		pinMode ( XM_PIN_L, OUTPUT );
		pinMode ( XM_PIN_R, OUTPUT );
	#else
		pinMode ( XM_PIN_L, OUTPUT );
	#endif
}

/**
 * Destructor
 **/
void xm_player_exit( void ){
	if (_context) xm_free_context( _context );
	_context = NULL;
	if (_buffer) delete [] _buffer;
	_buffer = NULL;
}

/**
 * Set global volume
 **/
void xm_player_volume( float vol ){
	if (!_context) return;
	_context->amplification = vol;
}

/**
 * Output sample on timer to produce the sound!
 **/
static void xm_timer_interrupt( void ){
	// XXX: Is this the correct way to handle empty buffer?
	//if (!_bufferavail) return;
	
	int16_t l_smp = 2048 + (_buffer[_buffertail++] * 2048.0f);
	int16_t r_smp = 2048 + (_buffer[_buffertail++] * 2048.0f);
	l_smp = (l_smp<0)?0:(l_smp>4095)?4095:l_smp; // Clamp to 12 bit range
	r_smp = (r_smp<0)?0:(r_smp>4095)?4095:r_smp; // Clamp to 12 bit range
	#ifdef XM_STEREO
		analogWrite( XM_PIN_L, l_smp);
		analogWrite( XM_PIN_R, r_smp);
	#else
		analogWrite( XM_PIN_L, (l_smp+r_smp)/2.0f );
	#endif

	if (_buffertail >= _buffersize) _buffertail = 0;
	_bufferavail -= 2;
}

/**
 * Start or unpause the player
 * @param	frombeginning		If true, will start again from the
 *								beginning. If false, will continue
 *								from last stop()
 */
void xm_player_start( bool frombeginning ){
	if (!_context) return;
	
	if (frombeginning){
		//xm_reset();
	}
	if (!_started){
		_started = true;
		_timer.begin( xm_timer_interrupt, 1000000.0f/XM_SAMPLE_RATE);
	}
}

/**
 * Stop or pause the player and discontinue audio output
 **/
void xm_player_stop( void ){
	_started = false;
	_timer.end();
	analogWrite( XM_PIN_L, 0);
	#ifdef XM_STEREO
		analogWrite( XM_PIN_R, 0);
	#endif
}

/**
 * Called during the update loop to step the player. This is typically done
 * even while the player is stopped, though it is safe not to call update
 * if the player is stopped.
 */
uint16_t xm_player_update( void ){
	// Check ringbuffer full
	if (_bufferavail==_buffersize) return 0;
	
	uint16_t numpairs;
	uint16_t numpairs2 = 0;
	// Available space doesn't loop end of ring
	if (_buffertail>_bufferhead){
		numpairs = (_buffertail - _bufferhead) >> 1;
		xm_generate_samples( _context, (float*)(_buffer+_bufferhead), numpairs);
	}
	else{
		// Samples between head and end
		numpairs = (_buffersize - _bufferhead) >> 1;
		xm_generate_samples( _context, (float*)(_buffer+_bufferhead), numpairs);
		// Samples between start and tail
		numpairs2 = _buffertail >> 1;
		xm_generate_samples( _context, _buffer, numpairs2);
	}
	_bufferhead = _buffertail;
	_bufferavail = _buffersize;
	return numpairs + numpairs2;
}

/**
 * Jump to a specific location in the file.
 *
 * This is intended to change music mid-game depending on what is happening.
 * Recommended to have un-reachable parts of the song that the player can
 * jump to for different areas of the game.
 * 
 * @param	location	Location to jump to
 * @param	wait		If true will wait for the current pattern to end
 *						first before jumping, otherwise will jump immediately
 */
void xm_player_jump( uint8_t location, bool wait ){
	
}

typedef union {
	float f;
	uint32_t i;
} f_to_uint32_t;
static void write_uint32_be(uint32_t in, File file) {
	char* i = (char*)(&in);
	
	#ifdef XM_BIG_ENDIAN
		file.write( i[0] );
		file.write( i[1] );
		file.write( i[2] );
		file.write( i[3] );
	#else
		file.write( i[3] );
		file.write( i[2] );
		file.write( i[1] );
		file.write( i[0] );
	#endif
}
static void write_uint32_le(uint32_t in, File file) {
	char* i = (char*)(&in);
	
	#ifdef XM_BIG_ENDIAN
		file.write( i[3] );
		file.write( i[2] );
		file.write( i[1] );
		file.write( i[0] );
	#else
		file.write( i[0] );
		file.write( i[1] );
		file.write( i[2] );
		file.write( i[3] );
	#endif
}
void write_uint16_be(uint16_t in, File file) {
	char* i = (char*)(&in);

	#ifdef XM_BIG_ENDIAN
		file.write( i[0] );
		file.write( i[1] );
	#else
		file.write( i[1] );
		file.write( i[0] );
	#endif
}
void write_uint16_le(uint16_t in, File file) {
	char* i = (char*)(&in);

	#ifdef XM_BIG_ENDIAN
		file.write( i[1] );
		file.write( i[0] );
	#else
		file.write( i[0] );
		file.write( i[1] );
	#endif
}

/**
 * Save to SD card. This is designed to work after loading a module
 * but before playing. If you have played or are playing the module
 * when this is called, the results are undefined!
 **/
boolean xm_player_save( const char* filename, xm_savetype_t savetype ){
	#if !defined(__MK64FX512__) && !defined(__MK66FX1M0__)
		Serial.println(F("Save is currently supported on Teensy 3.5 and 3.6 only"));
		return false;
	#endif
	
	if (!_context){
		Serial.println(F("Module is not loaded"));
		return false;
	}
	
	if (!SD.begin( BUILTIN_SDCARD )) {
		Serial.println(F("SD initialization failed"));
		return false;
	}
	
	File outfile;
	float samplepair[2];
	f_to_uint32_t sample;
	uint32_t count;
	uint32_t frames;
	uint32_t framesc;
	
	// Open file for writing (delete first if exists)
	Serial.print(F("Opening file\n"));
	SD.remove( filename );
	outfile = SD.open(filename, FILE_WRITE);
	if (!outfile) {
		Serial.printf(F("Opening %s failed\n"), filename);
		return false;
	}
	
	switch (savetype){
		case SAVETYPE_WAV:
			/*
				WAVE format info taken from http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html

				Unlike AU, WAVE files cannot have an unknown length. This
				is why we can't write directly to stdout (we need to rewind
				because module length is hard to know.
			*/
			
			Serial.print(F("Writing WAVE file\n"));
			
			outfile.print(F("RIFF"));
		Serial.printf(F("after RIFF position is %d\n"),outfile.position());
			write_uint32_le(0, outfile); 				// Chunk size. Will be filled later.
			outfile.print(F("WAVE"));
		Serial.printf(F("after WAVE position is %d\n"),outfile.position());

			outfile.print(F("fmt "));					// Start format chunk
			write_uint32_le(16, outfile);				// Format chunk size
			write_uint16_le(3, outfile);				// IEEE float sample data
			#ifdef XM_STEREO
				write_uint16_le(2, outfile);			// Two channels
			#else
				write_uint16_le(1, outfile);			// One channel
			#endif
			write_uint32_le(XM_SAMPLE_RATE, outfile);	// Frames/sec (sampling rate)
			#ifdef XM_STEREO
				write_uint32_le(48000 * 2 * sizeof(float), outfile);	// nAvgBytesPerSec
				write_uint16_le(2 * sizeof(float), outfile); 			// nBlockAlign
			#else
				write_uint32_le(48000 * 1 * sizeof(float), outfile);	// nAvgBytesPerSec
				write_uint16_le(1 * sizeof(float), outfile);			// nBlockAlign
			#endif
			write_uint16_le(8 * sizeof(float), outfile);// wBitsPerSample
			
		Serial.print(F("About to write data:\n"));
			
			outfile.print(F("data"));					// Start data chunk
			write_uint32_le(0, outfile);				// Data chunk size. Will be filled later.
			// Make sure we only do the song once
			count = 0;
			frames = 0;
			framesc = 0;
			while(!xm_get_loop_count(_context)) {
				// generate a sample pair and save the bytes
				xm_generate_samples( _context, samplepair, 1);
				sample.f = samplepair[0];
				write_uint32_le(sample.i, outfile);
				sample.f = samplepair[1];
				write_uint32_le(sample.i, outfile);
				count += 2;
				
				frames++;
				if (frames==48000){
					framesc++;
					frames=0;
					Serial.printf(F("Frame %d count %d\n"),framesc,count);
				}
			}
			
		Serial.print(F("Seeking back to write size: "));
		Serial.print(count);
		Serial.print(F(" float values\n"));
			
			// Riff chunk size
			outfile.seek(4);
			write_uint32_le(36 + count * sizeof(float), outfile);
			
			// Data chunk size
			outfile.seek(40);
			write_uint32_le(count * sizeof(float), outfile);
			
			break;
		case SAVETYPE_AU:
			/*
				From: https://whatis.techtarget.com/fileformat/AU-Sun-NeXT-DEC-UNIX-sound-file
				
				AU is a file extension for a sound file format belonging to Sun,
				NeXT and DEC and used in UNIX. The AU file format is also known
				as the Sparc-audio or u-law fomat.

				AU files contain three parts: the audio data and text for a header
				(containing 24 bytes) and an annotation block.

				MIME type: audio/basic, audio/x-basic, audio/au, audio/x-au,
				audio/x-pn-au, audio/rmf, audio/x-rmf, audio/x-ulaw, audio/vnd.qcelp
				audio/x-gsm, audio/snd
			*/
			write_uint32_be(0x2E736E64, outfile);		//.snd magic number
			write_uint32_be(28, outfile); 				// Header size
			write_uint32_be((uint32_t)(-1), outfile);	// Data size, unknown
			write_uint32_be(6, outfile); 				// Encoding: 32-bit IEEE floating point
			write_uint32_be(XM_SAMPLE_RATE, outfile);	// Sample rate
			#ifdef XM_STEREO
				write_uint32_be(2, outfile);			// Two channels
			#else
				write_uint32_be(1, outfile);			// One channel
			#endif
			write_uint32_be(0, outfile);				// Optional text info
			
			// Make sure we only do the song once
			count = 0;
			//while(!xm_get_loop_count(_context)) {
			while(count < 1000000) {
				// generate a sampel pair and save the bytes
				xm_generate_samples( _context, samplepair, 1);
				sample.f = samplepair[0];
				write_uint32_be(sample.i, outfile);
				sample.f = samplepair[1];
				write_uint32_be(sample.i, outfile);
				count += 2;
			}
			
			break;
		case SAVETYPE_BIN:
			// Simply save all sample data and to end as 2x4-byte words
			
			// Make sure we only do the song once
			while(!xm_get_loop_count(_context)) {
				// generate a sampel pair and save the bytes
				xm_generate_samples( _context, samplepair, 1);
				sample.f = samplepair[0];
				write_uint32_le(sample.i, outfile);
				sample.f = samplepair[1];
				write_uint32_le(sample.i, outfile);
			}
			break;
		case SAVETYPE_XMIZE:
			
			break;
		case SAVETYPE_XMIZE_H:
			
			break;
	}
	
	Serial.print(F("Closing file\n"));
	
	// close the file
    outfile.close();
	
	// Success
	return true;
}

/**
 * Dump information about the loaded module to serial.
 **/
void xm_player_info( boolean verbose ){
	Serial.println("### Dumping module information");
	#ifdef XM_STRINGS
		Serial.printf("  Module name:%s\n", _context->module->name );
		Serial.printf("  Tracker name:%s\n", _context->module->trackername );
	#else
		Serial.println("  Strings not supported (module, instrument and sample names)");
	#endif
	
	Serial.printf("  Channels:%u\n", _context->module.num_channels );
	Serial.printf("  Patterns:%u\n", _context->module.num_patterns );
	Serial.printf("  Instruments:%u\n", _context->module.num_instruments );
	Serial.printf("  Tempo:%u\n", _context->tempo );
	Serial.printf("  BPM:%u\n", _context->bpm );
	Serial.println("");
	Serial.printf("  Global volume:%1.5f\n", _context->global_volume );
	Serial.printf("  Amplification:%1.5f\n", _context->amplification );
	
	Serial.println("");
	for(uint16_t i = 0; i < _context->module.num_channels; ++i) {
		Serial.printf("  ## Channel %u\n", i);
		Serial.printf("    Volume:%u\n", _context->channels[i].volume );
		Serial.printf("    Panning:%u\n", _context->channels[i].panning );
	}
	
	Serial.println("");
	for(uint16_t i = 0; i < _context->module.num_instruments; ++i) {
		Serial.printf("  ## Instrument %u\n", i);
		#ifdef XM_STRINGS
			Serial.printf("    Instrument name:%s\n", _context->module.instruments[i].name );
		#endif
		Serial.printf("    Samples:%u\n", _context->module.instruments[i].num_samples );
		for(uint16_t j = 0; j < _context->module.instruments[i].num_samples; ++j) {
			Serial.printf("    # Sample %u\n", j);
			#ifdef XM_STRINGS
				Serial.printf("      Sample name:%s\n", _context->module.instruments[i].sample[j]->name );
			#endif
			Serial.printf("      Length:%u\n", _context->module.instruments[i].samples[j].length );
			Serial.printf("      Volume:%1.5f\n", _context->module.instruments[i].samples[j].volume );
		}
	}
}
