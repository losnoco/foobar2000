
// Nintendo Entertainment System (NES) NSF-format game music file emulator

// Game_Music_Emu 0.2.4. Copyright (C) 2003-2005 Shay Green. GNU LGPL license.

#ifndef NSF_EMU_H
#define NSF_EMU_H

#include "Classic_Emu.h"
#include "Nes_Apu.h"

class Hes_Emu : public Classic_Emu {
public:
	// Set internal gain, where 1.0 results in almost no clamping.
	Hes_Emu( double gain = 1.0 );
	~Hes_Emu();
	
	struct header_t {
		char tag [4];
		char vers;
		char first_track;
		
		enum { game = 0, author = 0, copyright = 0, song = 0 }; // no metadata
		enum { track_count = 256 };
	};
	BOOST_STATIC_ASSERT( sizeof (header_t) == 0x06 );
	
	// Load HES, given reader
	blargg_err_t load( Data_Reader& );

	// Load HES, given its header and reader for remaining data
	blargg_err_t load( const header_t&, Data_Reader& );
	
	void start_track( int );

	typedef Blip_Synth<blip_med_quality,192> synth_t;

// End of public interface
protected:
	void set_voice( int, Blip_Buffer*, Blip_Buffer*, Blip_Buffer* );
	void update_eq( blip_eq_t const& );
	blip_time_t run( int, bool* );
private:
	double gain;

	void * hes;
	void unload();

	blargg_err_t init_sound();
	
	synth_t synth;
};

#endif

