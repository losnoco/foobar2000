
// Game_Music_Emu 0.2.7. http://www.slack.net/~ant/libs/

#include "Dual_Resampler.h"

#include <stdlib.h>
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

#include BLARGG_SOURCE_BEGIN

int const resampler_extra = 256;

Dual_Resampler::Dual_Resampler()
{
	pairs_per_frame = 0;
	sample_buf = NULL;
}

Dual_Resampler::~Dual_Resampler()
{
	free( sample_buf );
}

blargg_err_t Dual_Resampler::resize( int pairs )
{
	pairs_per_frame = pairs;
	
	void* p = realloc( sample_buf, pairs * 2 * sizeof *sample_buf );
	BLARGG_CHECK_ALLOC( p );
	sample_buf = (sample_t*) p;
	
	oversamples_per_frame = int (pairs_per_frame * resampler.ratio()) * 2 + 2;
	return resampler.buffer_size( oversamples_per_frame + resampler_extra );
}

void Dual_Resampler::play_frame_( Blip_Buffer& blip_buf, sample_t* out )
{
	blip_time_t blip_time = blip_buf.count_clocks( pairs_per_frame );
	int sample_count = oversamples_per_frame - resampler.written();
	
	int new_count = play_frame( blip_time, sample_count, resampler.buffer() );
	assert( unsigned (new_count - sample_count) < resampler_extra );
	
	blip_buf.end_frame( blip_time );
	assert( blip_buf.samples_avail() == pairs_per_frame );
	
	resampler.write( new_count );
	
	int count = resampler.read( sample_buf, pairs_per_frame * 2 );
	assert( count == pairs_per_frame * 2 );
	
	mix_samples( blip_buf, out );
	blip_buf.remove_samples( pairs_per_frame );
}

void Dual_Resampler::play( long count, sample_t* out, Blip_Buffer& blip_buf )
{
	const int samples_per_frame = pairs_per_frame * 2;
	
	// empty extra buffer
	if ( extra_pos )
	{
		int n = samples_per_frame - extra_pos;
		if ( n > count )
			n = count;
		memcpy( out, sample_buf + extra_pos, n * sizeof *out );
		out += n;
		count -= n;
		extra_pos = (extra_pos + n) % samples_per_frame;
	}
	
	// entire frames
	while ( count >= samples_per_frame )
	{
		play_frame_( blip_buf, out );
		out += samples_per_frame;
		count -= samples_per_frame;
	}
	
	// extra
	if ( count )
	{
		play_frame_( blip_buf, sample_buf );
		extra_pos = count;
		memcpy( out, sample_buf, count * sizeof *out );
		out += count;
	}
}

#include BLARGG_ENABLE_OPTIMIZER

void Dual_Resampler::mix_samples( Blip_Buffer& blip_buf, sample_t* out )
{
	Blip_Reader sn;
	int bass = sn.begin( blip_buf );
	const sample_t* in = sample_buf;
	
	for ( int n = pairs_per_frame; n--; )
	{
		int s = sn.read();
		long l = (long) in [0] * 2 + s;
		sn.next( bass );
		long r = in [1];
		if ( (BOOST::int16_t) l != l )
			l = 0x7FFF - (l >> 24);
		r = r * 2 + s;
		in += 2;
		out [0] = l;
		out [1] = r;
		out += 2;
		if ( (BOOST::int16_t) r != r )
			out [-1] = 0x7FFF - (r >> 24);
	}
	
	sn.end( blip_buf );
}

