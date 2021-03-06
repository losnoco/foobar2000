
// Super Nintendo (SNES) SPC music file emulator

// Game_Music_Emu 0.2.7. Copyright (C) 2004 Shay Green. GNU LGPL license.

#ifndef SPC_EMU_H
#define SPC_EMU_H

#include "Fir_Resampler.h"
#include "Music_Emu.h"
#include "Snes_Spc.h"

class Spc_Emu : public Music_Emu {
	enum { trailer_offset = 0x10200 };
public:
	// A gain of 1.0 results in almost no clamping. Default gain roughly
	// matches volume of other emulators.
	Spc_Emu( double gain = 1.4 );
	
	// The Super Nintendo hardware samples at 32kHz. Other sample rates are
	// handled by adding a resampler after emulation; accuracy is not affected.
	enum { native_sample_rate = 32000 };
	
	// SPC file header
	struct header_t {
		char tag [35];
		byte format;
		byte version;
		byte pc [2];
		byte a, x, y, psw, sp;
		byte unused [2];
		char song [32];
		char game [32];
		char dumper [16];
		char comment [32];
		byte date [11];
		char len_secs [3];
		byte fade_msec [5];
		char author [32];
		byte mute_mask;
		byte emulator;
		byte unused2 [45];
		
		enum { copyright = 0 }; // no copyright field
	};
	
	// Load SPC data
	blargg_err_t load( Data_Reader& );
	
	// Load SPC using already-loaded header and remaining data
	blargg_err_t load( header_t const&, Data_Reader& );
	
	// Header for currently loaded SPC
	header_t const& header() const { return *(header_t*) spc_data; }
	
	// Pointer and size for trailer data
	byte const* trailer() const { return spc_data + trailer_offset; }
	long trailer_size() const { return spc_data_size - trailer_offset; }
	
	// If true, prevent channels and global volumes from being phase-negated
	void disable_surround( bool disable = true );
	
public:
	~Spc_Emu();
	blargg_err_t set_sample_rate( long );
	void mute_voices( int );
	void start_track( int );
	void play( long, sample_t* );
	void skip( long );
	const char** voice_names() const;
private:
	double resample_factor;
	bool use_resampler;
	
	byte* spc_data;
	long spc_data_size;
	
	Fir_Resampler<24> resampler;
	Snes_Spc apu;
};

inline void Spc_Emu::disable_surround( bool b ) { apu.disable_surround( b ); }

#endif

