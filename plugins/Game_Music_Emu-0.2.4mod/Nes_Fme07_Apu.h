
// AY-3-8910 Sound Chip Emulator

// $package. Copyright (C) 2003-2005 Shay Green. GNU LGPL license.

#ifndef NES_FME07_APU_H
#define NES_FME07_APU_H

#include "Nes_Apu.h"

struct fme07_snapshot_t
{
	enum { reg_count = 14 };
	BOOST::uint8_t regs [reg_count];
	BOOST::uint16_t counters [5]; // a, b, c, noise, envelope
	BOOST::uint32_t noise_bits;
	BOOST::uint8_t phases [3]; // 0 or 1
	BOOST::uint8_t env_phase;
	BOOST::uint8_t env_vol;
	BOOST::uint8_t latch;
	BOOST::uint8_t unused1 [2];
	
	void swap();
};
BOOST_STATIC_ASSERT( sizeof (fme07_snapshot_t) == 36 );

class Nes_Fme07_Apu : private fme07_snapshot_t {
public:
	Nes_Fme07_Apu();
	~Nes_Fme07_Apu();
	
	// See Nes_Apu.h for reference
	void reset();
	void volume( double );
	void treble_eq( blip_eq_t const& );
	void output( Blip_Buffer* );
	enum { osc_count = 3 };
	void osc_output( int index, Blip_Buffer* );
	void end_frame( blip_time_t );
	void save_snapshot( fme07_snapshot_t* ) const;
	void load_snapshot( fme07_snapshot_t const& );
	
	void write_latch( int );
	void write_data( blip_time_t, int data );
	
private:
	// noncopyable
	Nes_Fme07_Apu( const Nes_Fme07_Apu& );
	Nes_Fme07_Apu& operator = ( const Nes_Fme07_Apu& );
	
	struct osc_t
	{
		Blip_Buffer* output;
		int last_amp;
	};
	
	osc_t oscs [osc_count];
	blip_time_t last_time;
	
	enum { amp_range = 192 }; // can be any value; this gives best error/quality tradeoff
	static int amp_table [16];
	//Blip_Synth<blip_med_quality,amp_range> noise_synth;
	Blip_Synth<blip_good_quality,amp_range> synth;
	
	void run_until( blip_time_t );
	void run_square( int index, int volume, blip_time_t start, blip_time_t end );
	void run_noise( blip_time_t start, blip_time_t end );
};

inline void Nes_Fme07_Apu::write_latch( int data ) { latch = data; }

inline void Nes_Fme07_Apu::osc_output( int i, Blip_Buffer* buf )
{
	assert( (unsigned) i < osc_count );
	oscs [i].output = buf;
}

#endif

