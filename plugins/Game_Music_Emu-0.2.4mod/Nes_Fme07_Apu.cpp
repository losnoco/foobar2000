
// $package. http://www.slack.net/~ant/libs/

#include "Nes_Fme07_Apu.h"

#include "blargg_endian.h"
#include "blargg_source.h"

#include <string.h>

/* Copyright (C) 2003-2005 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
more details. You should have received a copy of the GNU Lesser General
Public License along with this module; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

int const period_factor = 16;

int Nes_Fme07_Apu::amp_table [16] = {
	0.0055*amp_range, 0.0078*amp_range, 0.0110*amp_range, 0.0156*amp_range, 
	0.0221*amp_range, 0.0312*amp_range, 0.0441*amp_range, 0.0624*amp_range, 
	0.0883*amp_range, 0.1249*amp_range, 0.1766*amp_range, 0.2498*amp_range, 
	0.3534*amp_range, 0.4998*amp_range, 0.7070*amp_range, 1.0000*amp_range,
};

void fme07_snapshot_t::swap()
{
	set_le32( &noise_bits, noise_bits );
	for ( int i = 0; i < 5; i++ )
		set_le16( &counters [i], counters [i] );
}

Nes_Fme07_Apu::Nes_Fme07_Apu()
{
	output( NULL );
	volume( 1.0 );
	reset();
}

Nes_Fme07_Apu::~Nes_Fme07_Apu()
{
}

void Nes_Fme07_Apu::reset()
{
	last_time = 0;
	
	for ( int i = 0; i < osc_count; i++ )
		oscs [i].last_amp = 0;
	
	fme07_snapshot_t* state = this;
	memset( state, 0, sizeof *state );
}

void Nes_Fme07_Apu::volume( double v )
{
	v *= 0.4; // to do: adjust
	synth.volume( v );
	//noise_synth.volume( v );
}

void Nes_Fme07_Apu::treble_eq( blip_eq_t const& eq )
{
	synth.treble_eq( eq );
	//noise_synth.treble_eq( eq );
}

void Nes_Fme07_Apu::output( Blip_Buffer* buf )
{
	for ( int i = 0; i < osc_count; i++ )
		osc_output( i, buf );
}

void Nes_Fme07_Apu::save_snapshot( fme07_snapshot_t* out ) const
{
	*out = *this;
}

void Nes_Fme07_Apu::load_snapshot( fme07_snapshot_t const& in )
{
	reset();
	fme07_snapshot_t* out = this;
	*out = in;
}

void Nes_Fme07_Apu::run_square( int index, int volume, blip_time_t time, blip_time_t end_time )
{
	unsigned period = (regs [index * 2 + 1] & 0x0f) * 0x100 + regs [index * 2];
	if ( !period )
		period = 1;
	period *= period_factor;
	
	osc_t& osc = oscs [index];
	if ( !volume || period < 41 )
	{
		if ( osc.last_amp )
		{
			synth.offset( time, -osc.last_amp, osc.output );
			osc.last_amp = 0;
		}
		
		time += counters [index];
		if ( time < end_time )
		{
			int count = (end_time - time + period - 1) / period;
			phases [index] ^= count & 1;
			time += (long) count * period;
		}
	}
	else
	{
		int amp = phases [index] * volume;
		int delta = amp - osc.last_amp;
		if ( delta )
		{
			osc.last_amp = amp;
			synth.offset( time, delta, osc.output );
		}
		
		time += counters [index];
		if ( time < end_time )
		{
			int delta = amp * 2 - volume;
			Blip_Buffer* osc_output = osc.output;
			
			do
			{
				delta = -delta;
				synth.offset_inline( time, delta, osc_output );
				time += period;
			}
			while ( time < end_time );
			
			osc.last_amp = (delta + volume) >> 1;
			phases [index] = (delta > 0);
		}
	}
	
	counters [index] = time - end_time;
}

void Nes_Fme07_Apu::run_until( blip_time_t time )
{
	require( time >= last_time );
	
	int disables = regs [7];
	for ( int i = 0; i < osc_count; i++, disables >>= 1 )
	{
		if ( ! oscs [i].output ) continue;
		int vol = regs [010 + i];
		if ( (disables & 011) == 010 && !(vol & 0x10) )
			run_square( i, amp_table [vol & 0x0f], last_time, time );
		else if ( ~disables & 011 && vol & 0x1f )
			dprintf( "FME07 %02X %02X\n", disables & 011, vol & 0x1f );
	}
	
	last_time = time;
}

void Nes_Fme07_Apu::write_data( blip_time_t time, int data )
{
	int const latch = this->latch;
	if ( (unsigned) latch <reg_count )
	{
		run_until( time );
		regs [latch] = data;
	}
}

void Nes_Fme07_Apu::end_frame( blip_time_t time )
{
	if ( time > last_time )
		run_until( time );
	last_time -= time;
	assert( last_time >= 0 );
}

