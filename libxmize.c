/**
 * Author: Romain "Artefact2" Dal Maso <artefact2@gmail.com>
 * Contributor: Peter "Projectitis" Vullings <peter@projectitis.com>
 **/

/* This program is free software. It comes without any warranty, to the
 * extent permitted by applicable law. You can redistribute it and/or
 * modify it under the terms of the Do What The Fuck You Want To Public
 * License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details. */

/**
 * Modified for Teensy microprocessor boards by Peter "Projectitis" Vullings
 **/

#include "xm_internal.h"
#include <stdio.h>

#define OFFSET(ptr) do { (ptr) = (void*)((intptr_t)(ptr) - (intptr_t)ctx); } while(0)

/**
 * XXX: implement per-waveform zapping 
 * XXX: maybe also wipe loop info, finetune, length etc 
 * XXX: properly replace sample by a 0-length waveform instead of zeroing buffer? harder than it sounds
static size_t zero_waveforms(xm_context_t* ctx) {
	size_t i, j, total_saved_bytes = 0;

	for(i = 0; i < ctx->module.num_instruments; ++i) {
		xm_instrument_t* inst = &(ctx->module.instruments[i]);

		for(j = 0; j < inst->num_samples; ++j) {
			xm_sample_t* sample = &(inst->samples[j]);

			size_t saved_bytes = (sample->bits == 8) ? sample->length : sample->length * 2;
			memset(sample->data8, 0, saved_bytes);
			total_saved_bytes += saved_bytes;
		}
	}

	return total_saved_bytes;
}
*/

/**
 * Convert xm module to libxmlized version and output as a byte array to serial. This is an
 * 'unwrapped' version of the module that can be stored in readonly memory.
 **/
void xm_libxmize( const char* moddata, uint32_t moddata_size ){
	xm_context_t* ctx;
	size_t i, j, sz;
	char outstr[255];
	#ifdef XM_LIBXMIZE_DELTA_SAMPLES
		size_t k;
	#endif
	uint8_t* ctx_p;
	uint8_t ctx_val;
	
	// Comment start of serial output
	#ifdef XM_LIBXMIZE_DELTA_SAMPLES
		xm_stdout("// module data in libxmized format with delta-encoded samples\n");
	#else
		xm_stdout("// module data in libxmized format without delta-encoded samples\n");
	#endif
	
	// Create the context. Will use a reasonable amount of memory, so larger XM files will
	// probably not work. If this becomes an issue then
	// XXX: write xm_context_to_serial that outputs direct to serial instead of creating context
	xm_create_context_safe(&ctx, moddata, moddata_size, 48000);
	if(ctx == NULL){
		xm_stdout("Error creating the XM context. Aborting.\n");
		return;
	}
	
	// Zero waveforms
	//sprintf( outstr, "// Saved %u bytes zeroing waveforms\n", zero_waveforms(ctx) );
	//xm_stdout(outstr);
	
	// Ugly pointer offsetting ahead
	for(i = 0; i < ctx->module.num_patterns; ++i) {
		OFFSET(ctx->module.patterns[i].slots);
	}
	for(i = 0; i < ctx->module.num_instruments; ++i) {
		for(j = 0; j < ctx->module.instruments[i].num_samples; ++j) {
			
			#ifdef XM_LIBXMIZE_DELTA_SAMPLES
			if(ctx->module.instruments[i].samples[j].length > 1) {
				// Half-ass delta encoding of samples, this compresses much better
				if(ctx->module.instruments[i].samples[j].bits == 8) {
					for(size_t k = ctx->module.instruments[i].samples[j].length - 1; k > 0; --k) {
						ctx->module.instruments[i].samples[j].data8[k] -= ctx->module.instruments[i].samples[j].data8[k-1];
					}
				} else {
					for(size_t k = ctx->module.instruments[i].samples[j].length - 1; k > 0; --k) {
						ctx->module.instruments[i].samples[j].data16[k] -= ctx->module.instruments[i].samples[j].data16[k-1];
					}
				}
			}
			#endif
			
			OFFSET(ctx->module.instruments[i].samples[j].data8);
		}
		OFFSET(ctx->module.instruments[i].samples);
	}
	OFFSET(ctx->module.patterns);
	OFFSET(ctx->module.instruments);
	OFFSET(ctx->row_loop_count);
	OFFSET(ctx->channels);
	
	// Write libxmized data to Serial
	sz = xm_get_memory_needed_for_context( moddata, moddata_size );
	sprintf( outstr, "const uint32_t moddata_len = %u;\n", sz );
	xm_stdout( outstr );
	xm_stdout("const char moddata[] = {\n");
	j = 0;
	ctx_p = (uint8_t*)ctx;
	for (i=0; i<sz; i++){
		if (i==0) xm_stdout(" ");
		else xm_stdout(",");
		ctx_val = *ctx_p;
		sprintf( outstr, "%3u", ctx_val );
		xm_stdout( outstr );
		if (++j==64){
			xm_stdout("\n");
			j=0;
		}
		ctx_p++;
	}
	xm_stdout("};\n");
	
	// Free memory
	xm_free_context(ctx);
}
