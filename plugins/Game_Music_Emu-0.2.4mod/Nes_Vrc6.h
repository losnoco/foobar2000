
// Konami VRC6 sound chip emulator

// Nes_Snd_Emu 0.1.6. Copyright (C) 2003-2005 Shay Green. GNU LGPL license.

#ifndef NES_VRC6_H
#define NES_VRC6_H

#include "Nes_Apu.h"

class Tagged_Data;

class Nes_Vrc6 {
public:
	Nes_Vrc6();
	~Nes_Vrc6();
	
	// See Nes_Apu.h for reference.
	
	void volume( double );
	void treble_eq( const blip_eq_t& );
	void output( Blip_Buffer* );
	enum { osc_count = 3 };
	void osc_output( int index, Blip_Buffer* );
	void reset();
	
	// Oscillator 0 write-only registers are at $9000-$9002
	// Oscillator 1 write-only registers are at $A000-$A002
	// Oscillator 2 write-only registers are at $B000-$B002
	enum { reg_count = 3 };
	enum { base_addr = 0x9000 };
	enum { addr_step = 0x1000 };
	void write_osc( nes_time_t, int osc, int reg, int data );
	
	void end_frame( nes_time_t );
	void reflect_state( Tagged_Data& );
	
// End of public interface

private:
	// noncopyable
	Nes_Vrc6( const Nes_Vrc6& );
	Nes_Vrc6& operator = ( const Nes_Vrc6& );
	
	struct Vrc6_Osc {
		BOOST::uint8_t regs [3];
		Blip_Buffer* output;
		int delay;
		int last_amp;
		int phase;
		int amp; // only used by saw
		
		int period() const {
			return (regs [2] & 0x0f) * 0x100L + regs [1] + 1;
		}
	};
	
	Vrc6_Osc oscs [osc_count];
	nes_time_t last_time;
	
	Blip_Synth<blip_med_quality,31> saw_synth;
	Blip_Synth<blip_good_quality,15> square_synth;
	
	void run_until( nes_time_t );
	void run_square( Vrc6_Osc& osc, nes_time_t );
	void run_saw( nes_time_t );
};

	inline void Nes_Vrc6::osc_output( int i, Blip_Buffer* buf ) {
		assert( (unsigned) i < osc_count );
		oscs [i].output = buf;
	}

#endif

