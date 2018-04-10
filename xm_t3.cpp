/* Author: Peter "Projectitis" Vullings <peter@projectitis.com> */
/* Website: https://github.com/Artefact2/libxm */

/* This program is free software. It comes without any warranty, to the
 * extent permitted by applicable law. You can redistribute it and/or
 * modify it under the terms of the Do What The Fuck You Want To Public
 * License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details. */
 
#include "xm_t3.h"
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
void xm_player_init( const char* moddata ){
	if (_context) return;
	
	// Create context
	xm_create_shared_context_from_libxmize(
		&_context,
		moddata,
		XM_SAMPLE_RATE
	);
	// Create buffer
	_bufferlen = uint16_t(XM_SAMPLE_RATE / 240);	// Number of sample pairs (L+R)
	_buffersize = _bufferlen * 2;					// Size of buffer in samples
	_buffer = new float[ _buffersize ];
	_buffertail = 0;								// We read data from tail
	_bufferhead = 0;								// We put data in from head (until we reach tail)
	_bufferavail = 0;								// No samples data available yet
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
 * Output sample on timer to produce the sound!
 **/
static void xm_timer_interrupt( void ){
	// XXX: Is this the correct way to handle empty buffer?
	if (!_bufferavail) return;
	
	uint16_t l_smp = 2048 + (uint16_t)((float)_buffer[_buffertail++] * 2048);
	uint16_t r_smp = 2048 + (uint16_t)((float)_buffer[_buffertail++] * 2048);
	l_smp = l_smp<0?0:l_smp>4095?4095:l_smp; // Clamp to 12 bit range
	r_smp = r_smp<0?0:r_smp>4095?4095:r_smp; // Clamp to 12 bit range
	#ifdef XM_STEREO
		analogWrite( XM_PIN_L, l_smp);
		analogWrite( XM_PIN_R, r_smp);
	#else
		analogWrite( XM_PIN_L, (l_smp+r_smp)/2.0f );
	#endif

	if (_buffertail > _buffersize) _buffertail = 0;
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
void xm_player_update( void ){
	// Check ringbuffer full
	if (_bufferavail==_buffersize) return;
	
	uint16_t numpairs;
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
		numpairs = _buffertail >> 1;
		xm_generate_samples( _context, _buffer, numpairs);
	}
	_bufferhead = _buffertail;
	_bufferavail = _buffersize;
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