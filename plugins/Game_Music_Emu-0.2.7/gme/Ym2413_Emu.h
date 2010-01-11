
// Yamaha YM2413 FM sound chip emulator interface

// Game_Music_Emu 0.2.7. Copyright (C) 2005 Shay Green. MIT license.

#ifndef YM2413_EMU_H
#define YM2413_EMU_H

class Ym2413_Emu  {
	struct OPLL* opll;
public:
	Ym2413_Emu();
	~Ym2413_Emu();
	
	int set_rate( double sample_rate, double clock_rate );
	
	void reset();
	
	enum { channel_count = 14 };
	void mute_voices( int mask );
	
	void write( int addr, int data );
	
	typedef short sample_t;
	
	enum { out_chan_count = 2 }; // stereo
	void run( int pair_count, sample_t* out );
};

#endif

