// foo_r128norm.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#define MY_VERSION "1.4"

/*
	change log
- And decreased latency to maintain at least 500ms worth of samples
- Version is now 1.4

2011-01-17 20:19 UTC - kode54
- Increased latency to maintain a buffer of 3 seconds worth of samples

2011-01-17 20:06 UTC - kode54
- Reverted short-term gain level changes to instantaneous again
- Version is now 1.3

2011-01-27 19:58 UTC - kode54
- Disabled momentary loudness polling and increased short-term loudness
  polling frequency
- Version is now 1.2

2011-01-27 19:22 UTC - kode54
- Adjusted gain level changes a bit
- Version is now 1.1

2011-01-27 19:00 UTC - kode54
- Initial release
- Version is now 1.0

2011-01-27 16:53 UTC - kode54
- Created project

*/

// #define ENABLE_MOMENTARY

class dsp_r128 : public dsp_impl_base
{
	unsigned m_rate, m_ch, m_ch_mask;
	ebur128_state * m_state;
	dsp_chunk_list_impl sample_buffer;
	bool startup_complete;
#ifdef ENABLE_MOMENTARY
	int frames_until_next_moment;
#endif
	int frames_until_next_shortterm;
#ifdef ENABLE_MOMENTARY
	double momentary_scale;
#endif
	double shortterm_scale;
	double current_scale;
	double target_scale;
public:
	dsp_r128() :
#ifdef ENABLE_MOMENTARY
	  momentary_scale(1.0),
#endif
	  shortterm_scale(1.0), current_scale(1.0), target_scale(1.0), m_rate( 0 ), m_ch( 0 ), m_ch_mask( 0 ), m_state( NULL )
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

#ifdef ENABLE_MOMENTARY
		if ( frames_until_next_moment <= 0 )
		{
			frames_until_next_moment += m_rate / 10;
			double new_momentary_scale = loudness_to_scale( ebur128_loudness_momentary( m_state ) );
			momentary_scale = sqrt( new_momentary_scale * momentary_scale );
			target_scale = sqrt( momentary_scale * shortterm_scale );
		}
#endif

		if ( frames_until_next_shortterm <= 0 )
		{
#ifdef ENABLE_MOMENTARY
			frames_until_next_shortterm += m_rate / 2;
#else
			frames_until_next_shortterm += m_rate / 10;
#endif
			shortterm_scale = loudness_to_scale( ebur128_loudness_shortterm( m_state ) );
#ifdef ENABLE_MOMENTARY
			target_scale = sqrt( momentary_scale * shortterm_scale );
#else
			target_scale = shortterm_scale;
#endif
		}

#ifdef ENABLE_MOMENTARY
		frames_until_next_moment -= chunk->get_sample_count();
#endif
		frames_until_next_shortterm -= chunk->get_sample_count();

#if 0
		flush_buffer();
#else
		flush_buffer( 0.5 );
		if ( sample_buffer.get_count() )
		{
			audio_chunk * copy_chunk = sample_buffer.insert_item( sample_buffer.get_count(), chunk->get_data_size() );
			copy_chunk->copy( *chunk );
			audio_chunk * output_chunk = sample_buffer.get_item( 0 );
			process_chunk( output_chunk );
			chunk->copy( *output_chunk );
			sample_buffer.remove_by_idx( 0 );
		}
		else
#endif
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
#ifdef ENABLE_MOMENTARY
		momentary_scale = 1.0;
#endif
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

#ifdef ENABLE_MOMENTARY
		frames_until_next_moment = 0;
#endif
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

	void flush_buffer( double latency = 0 )
	{
		while ( sample_buffer.get_duration() > latency )
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
