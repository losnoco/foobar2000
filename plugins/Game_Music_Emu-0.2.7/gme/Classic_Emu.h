
// Classic game music emulator interface base class for emulators which use Blip_Buffer
// for sound output.

// Game_Music_Emu 0.2.7. Copyright (C) 2003-2005 Shay Green. GNU LGPL license.

#ifndef CLASSIC_EMU_H
#define CLASSIC_EMU_H

#include "Music_Emu.h"
class Blip_Buffer;
class blip_eq_t;

class Classic_Emu : public Music_Emu {
public:
	Classic_Emu();
	~Classic_Emu();
	blargg_err_t set_sample_rate( long sample_rate );
	void set_buffer( Multi_Buffer* );
	void mute_voices( int );
	void play( long, sample_t* );
	void start_track( int track );  
	void set_equalizer( equalizer_t const& );
protected:
	virtual blargg_err_t setup_buffer( long clock_rate );
	virtual void set_voice( int index, Blip_Buffer* center,
			Blip_Buffer* left, Blip_Buffer* right ) = 0;
	virtual long run( int msec, bool* added_stereo ) = 0;
	virtual void update_eq( blip_eq_t const& ) = 0;
private:
	Multi_Buffer* buf;
	Multi_Buffer* std_buf; // owned
	void update_eq_();
};

inline void Classic_Emu::set_buffer( Multi_Buffer* new_buf )
{
	assert( !buf && new_buf );
	buf = new_buf;
}

#endif

