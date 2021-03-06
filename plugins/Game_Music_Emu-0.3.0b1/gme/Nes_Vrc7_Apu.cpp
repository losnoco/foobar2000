#include "Nes_Vrc7_Apu.h"

#include "emu2413.h"

#include "blargg_source.h"

#include <string.h>

Nes_Vrc7_Apu::Nes_Vrc7_Apu()
{
	opll = OPLL_new( 3579545 );
	output( NULL );
	volume( 1.0 );
	reset();
}

Nes_Vrc7_Apu::~Nes_Vrc7_Apu()
{
	OPLL_delete( ( OPLL * ) opll );
}

void Nes_Vrc7_Apu::reset()
{
	last_time = 0;
	count = 0;

	for ( int i = 0; i < osc_count; ++i )
	{
		Vrc7_Osc& osc = oscs [i];
		for ( int j = 0; j < 3; ++j )
			osc.regs [j] = 0;
		osc.last_amp = 0;
	}

	OPLL_reset( ( OPLL * ) opll );
}

void Nes_Vrc7_Apu::volume( double v )
{
	synth.volume( v * 1. / 3. );
}

void Nes_Vrc7_Apu::treble_eq( blip_eq_t const& eq )
{
	synth.treble_eq( eq );
}

void Nes_Vrc7_Apu::output( Blip_Buffer* buf )
{
	for ( int i = 0; i < osc_count; i++ )
		osc_output( i, buf );
}

void Nes_Vrc7_Apu::run_until( nes_time_t end_time )
{
	require( end_time >= last_time );

	nes_time_t time = last_time;

	while ( time < end_time )
	{
		if ( ++count == 36 )
		{
			count = 0;
			bool run = false;
			for ( unsigned i = 0; i < osc_count; ++i )
			{
				Vrc7_Osc & osc = oscs [i];
				if ( osc.output )
				{
					if ( ! run )
					{
						run = true;
						OPLL_run( ( OPLL * ) opll );
					}
					int amp = OPLL_calcCh( ( OPLL * ) opll, i );
					int delta = amp - osc.last_amp;
					if ( delta )
					{
						osc.last_amp = amp;
						synth.offset( time, delta, osc.output );
					}
				}
			}
		}
		++time;
	}

	last_time = end_time;
}

void Nes_Vrc7_Apu::write_reg( int data )
{
	OPLL_writeIO( ( OPLL * ) opll, 0, data );
}

void Nes_Vrc7_Apu::write_data( nes_time_t time, int data )
{
	if ( ( unsigned ) ( ( ( OPLL * ) opll )->adr - 0x10 ) < 0x36 )
	{
		int type = ( ( OPLL * ) opll )->adr >> 4;
		int chan = ( ( OPLL * ) opll )->adr & 15;
		
		if ( chan < 6 ) oscs [chan].regs [type-1] = data;
	}

	run_until( time );
	OPLL_writeIO( ( OPLL * ) opll, 1, data );
}

void Nes_Vrc7_Apu::end_frame( nes_time_t time )
{
	if ( time > last_time )
		run_until( time );
	last_time -= time;
	assert( last_time >= 0 );
}

void Nes_Vrc7_Apu::save_snapshot( vrc7_snapshot_t* out ) const
{
	out->latch = ( ( OPLL * ) opll )->adr;
	memcpy( out->inst, ( ( OPLL * ) opll )->CustInst, 8 );
	for ( int i = 0; i < osc_count; ++i )
	{
		for ( int j = 0; j < 3; ++j )
		{
			out->regs [i] [j] = oscs [i].regs [j];
		}
	}
	out->count = count;
}

void Nes_Vrc7_Apu::load_snapshot( vrc7_snapshot_t const& in )
{
	reset();
	write_reg( in.latch );
	int i;
	for ( i = 0; i < osc_count; ++i )
	{
		for ( int j = 0; j < 3; ++j )
		{
			oscs [i].regs [j] = in.regs [i] [j];
		}
	}
	count = in.count;

	for ( i = 0; i < 8; ++i )
	{
		OPLL_writeReg( ( OPLL * ) opll, i, in.inst [i] );
	}

	for ( i = 0; i < 3; ++i )
	{
		for ( int j = 0; j < 6; ++j )
		{
			OPLL_writeReg( ( OPLL * ) opll, 0x10 + i * 0x10 + j, oscs [j].regs [i] );
		}
	}
}
