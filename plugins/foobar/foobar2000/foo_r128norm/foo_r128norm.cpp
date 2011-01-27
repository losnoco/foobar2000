// foo_r128norm.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#define MY_VERSION "1.1"

/*
	change log

2011-01-27 19:22 UTC - kode54
- Adjusted gain level changes a bit
- Version is now 1.1

2011-01-27 19:00 UTC - kode54
- Initial release
- Version is now 1.0

2011-01-27 16:53 UTC - kode54
- Created project

*/

class dsp_r128 : public dsp_impl_base
{
	int m_rate, m_ch, m_ch_mask;
	ebur128_state * m_state;
	dsp_chunk_list_impl sample_buffer;
	bool startup_complete;
	int frames_until_next_moment;
	int frames_until_next_shortterm;
	double momentary_scale;
	double shortterm_scale;
	double current_scale;
	double target_scale;
public:
	dsp_r128() : momentary_scale(1.0), shortterm_scale(1.0), current_scale(1.0), target_scale(1.0), m_rate( 0 ), m_ch( 0 ), m_ch_mask( 0 ), m_state( NULL )
	{
	}

	~dsp_r128()
	{
		if ( m_state ) ebur128_destroy( &m_state );
	}

	static GUID g_get_guid()
	{
		// {492763AA-867E-4941-9CF6-FAFC81D80737}
		static const GUID guid = 
		{ 0x492763aa, 0x867e, 0x4941, { 0x9c, 0xf6, 0xfa, 0xfc, 0x81, 0xd8, 0x7, 0x37 } };
		return guid;
	}

	static void g_get_name( pfc::string_base & p_out ) { p_out = "EBU R128 Compressor"; }

	bool on_chunk( audio_chunk * chunk, abort_callback & )
	{
		if ( chunk->get_srate() != m_rate || chunk->get_channels() != m_ch || chunk->get_channel_config() != m_ch_mask )
		{
			m_rate = chunk->get_srate();
			m_ch = chunk->get_channels();
			m_ch_mask = chunk->get_channel_config();
			reinitialize();
			flush_buffer();
		}

		if ( ebur128_add_frames_float( m_state, chunk->get_data(), chunk->get_sample_count() ) ) return true;

		if ( !startup_complete )
		{
			if ( sample_buffer.get_duration() + chunk->get_duration() < 3.0 )
			{
				audio_chunk * chunk_copy = sample_buffer.insert_item( sample_buffer.get_count(), chunk->get_data_size() );
				chunk_copy->copy( *chunk );
				return false;
			}
			startup_complete = true;
		}

		if ( frames_until_next_moment <= 0 )
		{
			frames_until_next_moment += m_rate / 10;
			double new_momentary_scale = loudness_to_scale( ebur128_loudness_momentary( m_state ) );
			momentary_scale = sqrt( sqrt( new_momentary_scale * momentary_scale * momentary_scale * momentary_scale ) );
			target_scale = sqrt( momentary_scale * shortterm_scale );
		}

		if ( frames_until_next_shortterm <= 0 )
		{
			frames_until_next_shortterm += m_rate / 2;
			double new_shortterm_scale = loudness_to_scale( ebur128_loudness_shortterm( m_state ) );
			shortterm_scale = sqrt( shortterm_scale * new_shortterm_scale );
			target_scale = sqrt( momentary_scale * shortterm_scale );
		}

		frames_until_next_moment -= chunk->get_sample_count();
		frames_until_next_shortterm -= chunk->get_sample_count();

		flush_buffer();

		process_chunk( chunk );

		return true;
	}

	void on_endofplayback( abort_callback & ) { }
	void on_endoftrack( abort_callback & ) { }

	void flush()
	{
		m_rate = 0;
		m_ch = 0;
		m_ch_mask = 0;
		momentary_scale = 1.0;
		shortterm_scale = 1.0;
		current_scale = 1.0;
		target_scale = 1.0;
		sample_buffer.remove_all();
	}

	double get_latency()
	{
		return sample_buffer.get_duration();
	}

	bool need_track_change_mark()
	{
		return false;
	}

private:
	void reinitialize(void)
	{
		if ( m_state ) ebur128_destroy( &m_state );

		m_state = ebur128_init( m_ch, m_rate, EBUR128_MODE_S | EBUR128_MODE_M );
		if ( !m_state ) throw std::bad_alloc();

		pfc::array_t<int> channel_map;
		channel_map.set_count( m_ch );

		for ( unsigned i = 0; i < m_ch; i++ )
		{
			int channel = EBUR128_UNUSED;
			switch ( audio_chunk::g_extract_channel_flag( m_ch_mask, i ) )
			{
			case audio_chunk::channel_front_left:   channel = EBUR128_LEFT;           break;
			case audio_chunk::channel_front_right:  channel = EBUR128_RIGHT;          break;
			case audio_chunk::channel_front_center: channel = EBUR128_CENTER;         break;
			case audio_chunk::channel_back_left:    channel = EBUR128_LEFT_SURROUND;  break;
			case audio_chunk::channel_back_right:   channel = EBUR128_RIGHT_SURROUND; break;
			}
			channel_map[ i ] = channel;
		}

		ebur128_set_channel_map( m_state, channel_map.get_ptr() );

		frames_until_next_moment = 0;
		frames_until_next_shortterm = 0;

		startup_complete = false;
	}

	double loudness_to_scale(double lu)
	{
		if ( lu == std::numeric_limits<double>::quiet_NaN() || lu == std::numeric_limits<double>::infinity() || lu < -70 ) return 1.0;
		else return pow( 10.0, ( -18.0 - lu ) / 20.0 );
	}

	// Increase by one decibel every 32 samples, or decrease by one decibel every 256 samples
	void update_scale()
	{
		if ( current_scale < target_scale )
		{
			current_scale *= 1.00360429286956809;
			if ( current_scale > target_scale ) current_scale = target_scale;
		}
		else if ( current_scale > target_scale )
		{
			current_scale *= 0.99955034255981445;
			if ( current_scale < target_scale ) current_scale = target_scale;
		}
	}

	void process_chunk( audio_chunk * chunk )
	{
		unsigned count = chunk->get_sample_count();
		unsigned channels = chunk->get_channels();
		for ( unsigned i = 0; i < count; i++ )
		{
			audio_sample * sample = chunk->get_data() + i * channels;
			for ( unsigned j = 0; j < chunk->get_channels(); j++ )
			{
				sample[ j ] *= current_scale;
			}
			update_scale();
		}
	}

	void flush_buffer()
	{
		while ( sample_buffer.get_count() )
		{
			audio_chunk * buffered_chunk = sample_buffer.get_item( 0 );
			process_chunk( buffered_chunk );
			audio_chunk * output_chunk = insert_chunk( buffered_chunk->get_data_size() );
			output_chunk->copy( *buffered_chunk );
			sample_buffer.remove_by_idx( 0 );
		}
	}
};

static dsp_factory_nopreset_t<dsp_r128> g_dsp_r128_factory;

DECLARE_COMPONENT_VERSION("EBU R128 Normalizer", MY_VERSION,
	"EBU R128 Normalizer.\n"
	"\n"
	"Copyright (C) 2011 Chris Moeller\n"
	"\n"
	"Portions copyright (c) 2011 Jan Kokemüller\n"
	"\n"
	"Permission is hereby granted, free of charge, to any person obtaining a copy\n"
	"of this software and associated documentation files (the \"Software\"), to deal\n"
	"in the Software without restriction, including without limitation the rights\n"
	"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n"
	"copies of the Software, and to permit persons to whom the Software is\n"
	"furnished to do so, subject to the following conditions:\n"
	"\n"
	"The above copyright notice and this permission notice shall be included in\n"
	"all copies or substantial portions of the Software.\n"
	"\n"
	"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
	"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
	"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
	"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
	"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
	"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN\n"
	"THE SOFTWARE."
);

VALIDATE_COMPONENT_FILENAME("foo_r128norm.dll");
