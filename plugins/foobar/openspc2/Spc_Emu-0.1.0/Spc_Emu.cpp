
// Spc_Emu 0.1.0.

#include "Spc_Emu.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

// SPC_EMU_DEBUG mostly enables reporting of hardware behavior which doesn't
// get used by most/all SPCs. This usually catches CPU faults. This also
// highlights sections of code which might not work since they haven't ever
// been run.

#ifndef SPC_EMU_DEBUG
	#define SPC_EMU_DEBUG 0
#endif

/* Library Copyright (C) 2004 Shay Green. This library is free software;
you can redistribute it and/or modify it under the terms of the GNU
General Public License as published by the Free Software Foundation;
either version 2 of the License, or (at your option) any later version.
This library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
for more details. You should have received a copy of the GNU General
Public License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

Spc_Emu::Spc_Emu() : cpu( ram, this ), dsp( ram )
{
	timer [0].shift = 7; // 8 kHz
	timer [1].shift = 7; // 8 kHz
	timer [2].shift = 4; // 64 kHz
	
	// Put STOP instruction past end of memory to catch PC overflow.
	memset( ram + ram_size, 0xff, (sizeof ram) - ram_size );
}

// Load

const char* Spc_Emu::load_spc( const void* data, long size )
{
	struct spc_file_t {
		char    signature [27];
		char    unused [10];
		uint8_t pc [2];
		uint8_t a;
		uint8_t x;
		uint8_t y;
		uint8_t status;
		uint8_t sp;
		char    unused2 [212];
		uint8_t ram [0x10000];
		uint8_t dsp [128];
	};
	assert(( "Spc_Emu::load_spc(): structure had unwanted padding added",
			 sizeof (spc_file_t) == spc_file_size ));
	
	const spc_file_t* spc = (spc_file_t*) data;
	
	if ( size < spc_file_size )
		return "Not an SPC file";
	
	if ( strncmp( spc->signature, "SNES-SPC700 Sound File Data", 27 ) != 0 )
		return "Not an SPC file";
	
	registers_t regs;
	regs.pc = spc->pc [1] * 0x100 + spc->pc [0];
	regs.a = spc->a;
	regs.x = spc->x;
	regs.y = spc->y;
	regs.status = spc->status;
	regs.sp = spc->sp;
	
	return load_state( regs, spc->ram, spc->dsp );
}

// Handle other file formats (emulator save states) in user code, not here.

const char* Spc_Emu::load_state( const registers_t& cpu_state, const void* new_ram,
		const void* dsp_state )
{
	// cpu
	cpu.r = cpu_state;
	
	// ram
	memcpy( ram, new_ram, ram_size );
	memcpy( extra_ram, ram + rom_addr, sizeof extra_ram );
	
	// boot rom (have to force enable_rom() to update it)
	rom_enabled = !(ram [0xf1] & 0x80);
	enable_rom( !rom_enabled );
	
	// dsp
	dsp.reset();
	int i;
	for ( i = 0; i < dsp.register_count; i++ )
		dsp.write( i, ((uint8_t*) dsp_state) [i] );
	
	// timers
	for ( i = 0; i < timer_count; i++ )
	{
		Timer& t = timer [i];
		
		t.enabled = (ram [0xf1] >> i) & 1;
		t.count = 0;
		t.next_tick = 0;
		t.counter = ram [0xfd + i] & 15;
		
		int p = ram [0xfa + i];
		t.period = p ? p : 0x100;
	}
	
	// these registers always return 0 when read
	ram [0xf0] = 0;
	ram [0xf1] = 0;
	ram [0xfa] = 0;
	ram [0xfb] = 0;
	ram [0xfc] = 0;
	
	return NULL; // success
}

void Spc_Emu::clear_echo() {
	if ( !(dsp.read( 0x6C ) & 0x20) ) {
		unsigned addr = dsp.read( 0x6D ) * 0x100;
		unsigned size = dsp.read( 0x7D ) * 0x800;
		unsigned limit = ram_size - addr;
		memset( ram + addr, 0, (size < limit) ? size : limit );
	}
}

// Hardware

// Current time starts negative and ends at 0
inline spc_time_t Spc_Emu::time() const {
	return - cpu.remain();
}

// Keep track of next time to run and avoid a function call if it hasn't been reached.

// Timers

void Spc_Emu::Timer::run_until_( spc_time_t time )
{
	assert( enabled ); // when disabled, next_tick should always be in the future
	
	int elapsed = ((time - next_tick) >> shift) + 1;
	next_tick += elapsed << shift;
	elapsed += count;
	if ( elapsed >= period ) { // avoid costly divide
		int n = elapsed / period;
		elapsed -= n * period;
		counter = (counter + n) & 15;
	}
	count = elapsed;
}

// DSP

const int clocks_per_sample = 32; // 1.024 MHz CPU clock / 32000 samples per second

void Spc_Emu::run_dsp_( spc_time_t time ) {
	int count = (time - next_dsp) / (unsigned)clocks_per_sample + 1;
	sample_t* buf = sample_buf;
	if ( buf )
		sample_buf = buf + count * 2; // stereo
	next_dsp += count * clocks_per_sample;
	dsp.run( count, buf );
}

inline void Spc_Emu::run_dsp( spc_time_t time ) {
	if ( time >= next_dsp )
		run_dsp_( time );
}

// Read

int Spc_Emu::read( spc_addr_t addr )
{
	// zero page ram is used most often
	if ( addr < 0xf0 )
		return ram [addr];
	
	// dsp
	if ( addr == 0xf3 ) {
		run_dsp( time() );
		if ( SPC_EMU_DEBUG && ram [0xf2] >= 0x80 )
			printf( "DSP read from $%02X\n", (int) ram [0xf2] );
		return dsp.read( ram [0xf2] & 0x7f );
	}
	
	// counters
	unsigned i = addr - 0xfd;
	if ( i < timer_count ) {
		Timer& t = timer [i];
		t.run_until( time() );
		int result = t.counter;
		t.counter = 0;
		return result;
	}
	
	//if ( unsigned (addr - 0xf4) < 4 )
	//  Spc_Debug_Cpu::trace = true;
	
	if ( SPC_EMU_DEBUG ) {
		if ( addr == 0xf0 || addr == 0xf1 || addr == 0xf8 ||
				addr == 0xf9 || addr == 0xfa )
			printf( "Read from register $%02X\n", (int) addr );
	}
	
	// Registers which always read as 0 are handled by setting ram [reg] to 0
	// at startup then never changing that value.
	
	// ram
	return ram [addr];
}


// Write

const unsigned char Spc_Emu::boot_rom [rom_size] = { // verified
	0xCD, 0xEF, 0xBD, 0xE8, 0x00, 0xC6, 0x1D, 0xD0,
	0xFC, 0x8F, 0xAA, 0xF4, 0x8F, 0xBB, 0xF5, 0x78,
	0xCC, 0xF4, 0xD0, 0xFB, 0x2F, 0x19, 0xEB, 0xF4,
	0xD0, 0xFC, 0x7E, 0xF4, 0xD0, 0x0B, 0xE4, 0xF5,
	0xCB, 0xF4, 0xD7, 0x00, 0xFC, 0xD0, 0xF3, 0xAB,
	0x01, 0x10, 0xEF, 0x7E, 0xF4, 0x10, 0xEB, 0xBA,
	0xF6, 0xDA, 0x00, 0xBA, 0xF4, 0xC4, 0xF4, 0xDD,
	0x5D, 0xD0, 0xDB, 0x1F, 0x00, 0x00, 0xC0, 0xFF
};

void Spc_Emu::enable_rom( int enable ) {
	if ( rom_enabled != enable ) {
		rom_enabled = enable;
		memcpy( ram + rom_addr, enable ? boot_rom : extra_ram, rom_size );
	}
}

void Spc_Emu::write( spc_addr_t addr, int data )
{
	// first page is very common
	if ( addr < 0xf0 ) {
		ram [addr] = data;
	}
	else switch ( addr )
	{
		// RAM
		default:
			if ( addr < rom_addr ) {
				ram [addr] = data;
			}
			else {
				if ( SPC_EMU_DEBUG )
					printf( "Wrote to $%02X to high RAM at $%04X\n", (int) data, (int) addr );
				extra_ram [addr - rom_addr] = data;
				if ( !rom_enabled )
					ram [addr] = data;
			}
			break;
		
		// DSP
		//case 0xf2: // mapped to RAM
		case 0xf3: {
			run_dsp( time() );
			int reg = ram [0xf2];
			if ( next_dsp > 0 ) {
				// skip mode
				
				// key press
				if ( reg == 0x4C )
					keys_pressed |= data & ~dsp.read( 0x5C );
				
				if ( reg == 0x5C ) {
					keys_released |= data;
					keys_pressed &= ~data;
				}
			}
			if ( reg < 0x80 ) {
				dsp.write( reg, data );
			}
			else if ( SPC_EMU_DEBUG ) {
				printf( "DSP write to $%02X\n", (int) reg );
			}
			break;
		}
		
		case 0xf0: // Test register
			if ( SPC_EMU_DEBUG )
				printf( "Wrote $%02X to $F0\n", (int) data );
			// to do: disable timers
			break;
		
		// Config
		case 0xf1: {
			
			// timers
			for ( int i = 0; i < timer_count; i++ )
			{
				Timer& t = timer [i];
				if ( !(data & (1 << i)) ) {
					t.enabled = 0;
					t.next_tick = 0;
				}
				else if ( !t.enabled ) {
					// just enabled
					t.enabled = 1;
					t.counter = 0;
					t.count = 0;
					t.next_tick = time();
				}
			}
			
			// port clears
			if ( data & 0x10 ) {
				ram [0xf4] = 0;
				ram [0xf5] = 0;
			}
			if ( data & 0x20 ) {
				ram [0xf6] = 0;
				ram [0xf7] = 0;
			}
			
			enable_rom( data & 0x80 );
			
			break;
		}
		
		// Ports
		case 0xf4:
		case 0xf5:
		case 0xf6:
		case 0xf7:
			// to do: handle output ports
			break;
		
		//case 0xf8: // mapped to RAM
		//case 0xf9: // mapped to RAM
		
		// Timers
		case 0xfa:
		case 0xfb:
		case 0xfc: {
			Timer& t = timer [addr - 0xfa];
			if ( (t.period & 0xff) != data ) {
				t.run_until( time() );
				t.period = data ? data : 0x100;
			}
			break;
		}
		
		// Counters (cleared on write)
		case 0xfd:
		case 0xfe:
		case 0xff:
			if ( SPC_EMU_DEBUG )
				printf( "Wrote to counter $%02X\n", (int) addr );
			timer [addr - 0xfd].counter = 0;
			break;
	}
}

// Play

void Spc_Emu::skip( int msec )
{
	long count = (32000L / 1000) * 2 * msec;
	if ( count > 10 * 32000L )
	{
		// don't run DSP for long durations (2-3 times faster)
		
		const long sync_count = 32000L * 4;
		
		// keep track of any keys pressed/released (and not subsequently released)
		keys_pressed = 0;
		keys_released = 0;
		play( count - sync_count, skip_sentinel ); // sentinel tells play to ignore DSP
		
		// press/release keys now
		dsp.write( 0x5C, keys_released & ~keys_pressed );
		dsp.write( 0x4C, keys_pressed );
		
		// play the last few seconds normally to help synchronize DSP
		count = sync_count;
	}
	
	play( count, NULL );
}

long Spc_Emu::play( long count, sample_t* out )
{
	assert(( "Spc_Emu::play(): count must be even", count % 2 == 0 ));
	
	// CPU time() runs from -duration to 0
	spc_time_t duration = (count / 2) * clocks_per_sample;
	
	// DSP output is made on-the-fly when the CPU reads/writes DSP registers
	sample_buf = out;
	next_dsp = (out == skip_sentinel) ? clocks_per_sample : -duration + clocks_per_sample;
	
	// Localize timer next_tick times and run them to the present to prevent a running
	// but ignored timer's next_tick from getting too far behind and overflowing.
	for ( int i = 0; i < timer_count; i++ ) {
		Timer& t = timer [i];
		if ( t.enabled ) {
			t.next_tick -= duration;
			t.run_until( -duration );
		}
	}
	
	// Run CPU for full duration
	if ( cpu.run( duration ) > 0 ) {
		if ( SPC_EMU_DEBUG )
			printf( "Unhandled instruction $%02X, pc = $%04X\n",
					(int) cpu.read( cpu.r.pc ), (unsigned) cpu.r.pc );
		return 0; // error
	}
	
	// Catch DSP up to present.
	run_dsp( 0 );
	assert( !out || next_dsp == clocks_per_sample );
	
	// Should have filled entire output buffer
	assert( !out || out == skip_sentinel || sample_buf - out == count );
	
	return count;
}

