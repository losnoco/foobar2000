
// Yamaha YM2612 FM sound chip emulator interface

// Game_Music_Emu 0.2.7. Copyright (C) 2004-2005 Shay Green. GNU LGPL license.

#ifndef YM2612_EMU_H
#define YM2612_EMU_H

#include "blargg_common.h"

struct Ym2612_Impl;

class Ym2612_Emu  {
	Ym2612_Impl* impl;
public:
	Ym2612_Emu() { impl = NULL; }
	~Ym2612_Emu();
	
	blargg_err_t set_rate( double sample_rate, double clock_rate );
	
	void reset();
	
	enum { channel_count = 6 };
	void mute_voices( int mask );
	
	void write0( int addr, int data );
	void write1( int addr, int data );
	
	typedef short sample_t;
	
	enum { out_chan_count = 2 }; // stereo
	void run( int pair_count, sample_t* out );
};

#endif

