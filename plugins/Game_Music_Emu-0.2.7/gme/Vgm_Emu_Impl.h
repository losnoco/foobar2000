
// Game_Music_Emu 0.2.7. Copyright (C) 2003-2005 Shay Green. GNU LGPL license.

#ifndef VGM_EMU_IMPL_H
#define VGM_EMU_IMPL_H

#include "Dual_Resampler.h"
#include "Classic_Emu.h"
#include "Ym2413_Emu.h"
#include "Ym2612_Emu.h"
#include "Sms_Apu.h"

template<class Emu>
class Ym_Emu : public Emu {
protected:
	int last_time;
	short* out;
	enum { disabled_time = -1 };
public:
	Ym_Emu()                        : last_time( disabled_time ), out( NULL ) { }
	void enable( bool b )           { last_time = b ? 0 : disabled_time; }
	bool enabled() const            { return last_time != disabled_time; }
	void begin_frame( short* p );
	int run_until( int time );
};

class Vgm_Emu_Impl : public Classic_Emu, protected Dual_Resampler {
public:
	typedef Classic_Emu::sample_t sample_t;
	typedef BOOST::uint8_t byte;
	
protected:
	enum { stereo = 2 };
	
	typedef int vgm_time_t;
	
	enum { fm_time_bits = 12 };
	typedef int fm_time_t;
	long fm_time_offset;
	int fm_time_factor;
	fm_time_t to_fm_time( vgm_time_t ) const;
	
	enum { blip_time_bits = 12 };
	int blip_time_factor;
	blip_time_t to_blip_time( vgm_time_t ) const;
	
	byte const* data;
	byte const* loop_begin;
	byte const* data_end;
	void update_fm_rates( long* ym2413_rate, long* ym2612_rate ) const;
	
	vgm_time_t vgm_time;
	byte const* pos;
	blip_time_t run_commands( vgm_time_t );
	int play_frame( blip_time_t blip_time, int sample_count, sample_t* buf );
	
	byte const* pcm_data;
	byte const* pcm_pos;
	int dac_amp;
	int dac_disabled; // -1 if disabled
	void write_pcm( vgm_time_t, int amp );
	
	Ym_Emu<Ym2612_Emu> ym2612;
	Ym_Emu<Ym2413_Emu> ym2413;
	
	Sms_Apu psg;
	Blip_Synth<blip_med_quality,1> dac_synth;
};

#endif
