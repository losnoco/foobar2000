
// Super Nintendo (SNES) SPC music file emulator/synthesizer

// Spc_Emu 0.1.0. Copyright (C) 2004 Shay Green. GNU GPL license.

#ifndef SPC_EMU_H
#define SPC_EMU_H

#include "Spc_Cpu.h"
#include "Spc_Dsp.h"

class Spc_Emu {
public:
	Spc_Emu();
	
	// Load SPC data of given size into emulator. Size must be at least spc_file_size.
	// NULL on success, otherwise error string. No pointers are kept to the spc data,
	// so it can be freed immediately.
	enum { spc_file_size = 0x10180 };
	const char* load_spc( const void* spc, long size );
	
	// Load state given CPU registers, 64KB RAM contents, and 128 bytes of DSP registers.
	// NULL on success, otherwise error string. No pointers are kept to the state data,
	// so it can be freed immediately.
	typedef Spc_Cpu::registers_t registers_t;
	const char* load_state( const registers_t& cpu_state, const void* ram, const void* dsp );
	
	// Clear echo buffer (some SPC files have static in the echo buffer)
	void clear_echo();
	
	// Mute voice n if bit n (1 << n) of mask is set.
	enum { voice_count = Spc_Dsp::voice_count };
	void mute_voices( int mask );
	
	// Play for 'count' samples. Write resulting samples to 'buf' if it is not NULL.
	// Return number of samples played, or 0 is there was an emulation problem. Count
	// must be even.
	//
	// Sample output is 16-bit signed stereo pairs with the left channel first, at 32kHz.
	typedef short sample_t;
	long play( long count, sample_t* buf );
	
	// Skip forwards by the specified number of milliseconds (1/1000 second).
	void skip( int msec );

	// For debug notices in player
	inline unsigned short get_cpu_pc() const { return cpu.r.pc; }
	
private:
	typedef unsigned char uint8_t;
	
	// timers
	struct Timer {
		spc_time_t next_tick;
		int period;
		int count;
		int shift;
		int counter;
		int enabled;
		
		void run_until_( spc_time_t );
		void run_until( spc_time_t time ) {
			if ( time >= next_tick )
				run_until_( time );
		}
	};
	enum { timer_count = 3 };
	Timer timer [timer_count];

	// hardware
	Spc_Cpu cpu;
	spc_time_t time() const;
	int  read( spc_addr_t );
	void write( spc_addr_t, int );
	friend class Spc_Cpu;
	
	// boot rom
	enum { rom_size = 64 };
	enum { rom_addr = 0xffc0 };
	int rom_enabled;
	uint8_t extra_ram [rom_size];
	static const uint8_t boot_rom [rom_size];
	void enable_rom( int );
	
	// dsp
	sample_t* sample_buf;
	spc_time_t next_dsp;
	Spc_Dsp dsp;
	int keys_pressed;
	int keys_released;
	sample_t skip_sentinel [1]; // special value for play() passed by skip()
	void run_dsp( spc_time_t );
	void run_dsp_( spc_time_t );
	
	// 64KB RAM + padding filled with STOP instruction to catch PC overflow.
	enum { ram_size = 0x10000 };
	uint8_t ram [ram_size + 0x200];
};

	inline void Spc_Emu::mute_voices( int mask ) {
		dsp.mute_voices( mask );
	}

#endif

