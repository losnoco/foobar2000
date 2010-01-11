
// Game_Music_Emu 0.2.7. http://www.slack.net/~ant/libs/

#include "Classic_Emu.h"

#include "Multi_Buffer.h"

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

#include BLARGG_SOURCE_BEGIN

Classic_Emu::Classic_Emu()
{
	buf = NULL;
	std_buf = NULL;
}

Classic_Emu::~Classic_Emu()
{
	delete std_buf;
}

void Classic_Emu::update_eq_()
{
	equalizer_t const& eq = equalizer();
	update_eq( blip_eq_t( eq.treble, eq.cutoff, buf->sample_rate() ) );
	buf->bass_freq( equalizer().bass );
}

void Classic_Emu::set_equalizer( equalizer_t const& eq )
{
	Music_Emu::set_equalizer( eq );
	if ( buf ) // called in constructors where buf hasn't been set up yet
		update_eq_();
}
	
blargg_err_t Classic_Emu::set_sample_rate( long sample_rate )
{
	if ( !buf )
	{
		if ( !std_buf )
			BLARGG_CHECK_ALLOC( std_buf = BLARGG_NEW Stereo_Buffer );
		buf = std_buf;
	}
	
	return buf->set_sample_rate( sample_rate, 1000 / 20 );
}

void Classic_Emu::mute_voices( int mask )
{
	require( buf ); // init() must have been called
	
	Music_Emu::mute_voices( mask );
	for ( int i = voice_count(); i--; )
	{
		if ( mask & (1 << i) )
		{
			set_voice( i, NULL, NULL, NULL );
		}
		else
		{
			Multi_Buffer::channel_t ch = buf->channel( i );
			set_voice( i, ch.center, ch.left, ch.right );
		}
	}
}

blargg_err_t Classic_Emu::setup_buffer( long clock_rate )
{
	require( buf ); // init() must have been called
	
	buf->clock_rate( clock_rate );
	update_eq_();
	return buf->set_channel_count( voice_count() );
}

void Classic_Emu::start_track( int track )
{
	require( buf ); // init() must have been called
	
	Music_Emu::start_track( track );
	mute_voices( 0 );
	buf->clear();
}

void Classic_Emu::play( long count, sample_t* out )
{
	require( buf ); // init() must have been called
	
	long remain = count;
	while ( remain )
	{
		remain -= buf->read_samples( &out [count - remain], remain );
		if ( remain )
		{
			bool added_stereo = false;
			blip_time_t clocks_emulated = run( buf->length(), &added_stereo );
			buf->end_frame( clocks_emulated, added_stereo );
		}
	}
}

