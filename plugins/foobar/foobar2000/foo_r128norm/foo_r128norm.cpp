#include "stdafx.h"

#define MY_VERSION "1.12"

/*
	change log

2017-02-04 05:00 UTC - kode54
- Add link to about string
- Version is now 1.12

2011-12-19 03:09 UTC - kode54
- Changed a number of constants to local variables in preparation for adding a
  configuration system
- Restructured to use a large duration gated loudness measurement
- Version is now 1.11

2011-02-07 14:12 UTC - kode54
- Fixed a stupid bug in libebur128 when handling weird sample rates
- Version is now 1.10

2011-02-05 06:11 UTC - kode54
- Re-enabled increasing the gain level
- Changed momentary and short-term loudness to gated loudness
- Version is now 1.9

2011-02-04 13:44 UTC - kode54
- Disabled increasing the gain level, for now
- Version is now 1.8

2011-01-28 06:19 UTC - kode54
- Fixed initial gain values on startup, again.
- Version is now 1.7

2011-01-27 23:43 UTC - kode54
- Now correctly flushes the buffer on playback termination
- Version is now 1.6

2011-01-27 23:07 UTC - kode54
- Reenabled momentary loudness polling
- Changed volume ramping to 1 dB every 50ms
- Current scale is now forced to the detected target scale after the initial
  buffering completes.
- Version is now 1.5

2011-01-27 21:13 UTC - kode54
- And decreased latency to maintain at least 500ms worth of samples
- Version is now 1.4

2011-01-27 20:19 UTC - kode54
- Increased latency to maintain a buffer of 3 seconds worth of samples

2011-01-27 20:06 UTC - kode54
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

class dsp_r128 : public dsp_impl_base
{
	unsigned m_rate, m_ch, m_ch_mask;
	ebur128_state * m_state;
	dsp_chunk_list_impl sample_buffer;
	bool startup_complete;
	int frames_until_next;
	double current_scale;
	double target_scale;
	double scale_up;
	double scale_down;
	double latency_startup;
	double latency_minimum;
	double latency_window;
	double latency_update;
	double reference_loudness;

	unsigned block_count;

public:
	dsp_r128() :
	  current_scale(1.0), target_scale(1.0), latency_startup(10.0), latency_window(20.0), latency_minimum(4.0), latency_update(0.05), reference_loudness(-18.0),
	  m_rate( 0 ), m_ch( 0 ), m_ch_mask( 0 ), m_state( NULL )
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

		if ( !startup_complete )
		{
			if ( ebur128_add_frames_float( m_state, chunk->get_data(), chunk->get_sample_count() ) ) return true;
			if ( sample_buffer.get_duration() + chunk->get_duration() < latency_startup )
			{
				audio_chunk * chunk_copy = sample_buffer.insert_item( sample_buffer.get_count(), chunk->get_data_size() );
				chunk_copy->copy( *chunk );
				return false;
			}
		}

		unsigned samples_added = 0;
		while ( samples_added < chunk->get_sample_count() )
		{
			unsigned samples_to_add = chunk->get_sample_count() - samples_added;
			if ( frames_until_next > 0 && frames_until_next < samples_to_add ) samples_to_add = frames_until_next;

			if ( frames_until_next <= 0 )
			{
				if (m_rate / 10 < samples_to_add) samples_to_add = m_rate / 10;
				frames_until_next += m_rate / 10;
				target_scale = loudness_to_scale( ebur128_loudness_global( m_state ) );
			}

			if ( ebur128_add_frames_float( m_state, chunk->get_data() + chunk->get_channels() * samples_added, samples_to_add ) ) return true;

			ebur128_gated_loudness_cleanup( m_state, block_count );

			samples_added += samples_to_add;
			frames_until_next -= samples_to_add;
		}

		if ( !startup_complete )
		{
			startup_complete = true;
			current_scale = target_scale;
		}

#if 0
		flush_buffer();
#else
		flush_buffer( latency_minimum );
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

	void on_endofplayback( abort_callback & )
	{
		flush_buffer();
		flush();
	}

	void on_endoftrack( abort_callback & ) { }

	void flush()
	{
		m_rate = 0;
		m_ch = 0;
		m_ch_mask = 0;
#ifdef ENABLE_MOMENTARY
		momentary_scale = 1.0;
#endif
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

		m_state = ebur128_init( m_ch, m_rate, EBUR128_MODE_I );
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

		block_count = (unsigned) (latency_window * 2.5 + 0.5);

		frames_until_next = 0;

		// Scale up or down by 1 dB every latency_update seconds
		scale_up   = pow( pow( 10.0,  1.0 / 20.0 ), 1.0 / ( latency_update * double( m_rate ) ) );
		scale_down = pow( pow( 10.0, -1.0 / 20.0 ), 1.0 / ( latency_update * double( m_rate ) ) );

		startup_complete = false;
	}

	double loudness_to_scale(double lu)
	{
		if ( lu == std::numeric_limits<double>::quiet_NaN() || lu == std::numeric_limits<double>::infinity() || lu < -70 ) return 1.0;
		else return pow( 10.0, ( reference_loudness - lu ) / 20.0 );
	}

	// Increase by one decibel every 32 samples, or decrease by one decibel every 256 samples
	void update_scale()
	{
		if ( current_scale < target_scale )
		{
			current_scale *= scale_up; //1.00360429286956809;
			if ( current_scale > target_scale ) current_scale = target_scale;
		}
		else if ( current_scale > target_scale )
		{
			current_scale *= scale_down; //0.99955034255981445;
			if ( current_scale < target_scale ) current_scale = target_scale;
		}
	}

	void process_chunk( audio_chunk * chunk )
	{
		unsigned count = chunk->get_sample_count();
		unsigned channels = chunk->get_channels();
		audio_sample * sample = chunk->get_data();
		unsigned i;

		for ( i = 0; i < count; i++ )
		{
			if ( current_scale == target_scale ) break;
			for ( unsigned j = 0; j < channels; j++ )
			{
				*sample++ *= current_scale;
			}
			update_scale();
		}

		if ( i < count ) audio_math::scale( sample, ( count - i ) * channels, sample, current_scale );
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
	"Copyright (C) 2011-2017 Christopher Snowhill\n"
	"\n"
	"https://www.patreon.com/kode54""\n"
	"\n"
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
