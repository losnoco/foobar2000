#include "base.h"
#include "config.h"
#include "monitor.h"

#include <cmath>

static t_size sjis_decode_char( const char * p_sjis, t_size max )
{
	const t_uint8 * sjis = (const t_uint8 *) p_sjis;

	if ( max == 0 )
		return 0;
	else if ( max > 2 )
		max = 2;

	if ( sjis [0] < 0x80 )
	{
		return sjis [0] > 0 ? 1 : 0;
	}

	if ( max >= 1 )
	{
		// TODO: definitely weak
		if ( unsigned( sjis [0] - 0xA1 ) < 0x3F )
			return 1;
	}

	if ( max >= 2 )
	{
		// TODO: probably very weak
		if ( ( unsigned( sjis [0] - 0x81 ) < 0x1F || unsigned( sjis [0] - 0xE0 ) < 0x10 ) &&
			unsigned( sjis [1] - 0x40 ) < 0xBD )
			return 2;
	}

	return 0;
}

static bool is_valid_sjis( const char * param, t_size max = ~0 )
{
	t_size walk = 0;
	while ( walk < max && param [walk] != 0 )
	{
		t_size d;
		d = sjis_decode_char( param + walk, max - walk );
		if ( d == 0 ) return false;
		walk += d;
		if ( walk > max )
		{
			return false;
		}
	}
	return true;
}

void input_gep::meta_add( file_info & p_info, const char * name, const char * value, t_size max )
{
	if ( value[ 0 ] )
	{
		pfc::string8 t;
		if ( value[ max - 1 ] )
		{
			// TODO: moo
			t.set_string( value, max );
			value = t;
		}
		else max = strlen( value );
		if ( pfc::is_lower_ascii( value ) || pfc::is_valid_utf8( value, max ) )
			p_info.meta_add( name, value );
		else if ( is_valid_sjis( value, max ) )
			p_info.meta_add( name, pfc::stringcvt::string_utf8_from_codepage( 932, value ) ); // Shift-JIS
		else
			p_info.meta_add( name, pfc::stringcvt::string_utf8_from_ansi( value ) );
	}
}

input_gep::input_gep()
{
	emu = 0;
	//voice_mask = 0;

	buffer = 0;

	sample_rate = cfg_sample_rate;

	monitoring = false;

	effects_enable = !!cfg_effects_enable;
	effects_bass = cfg_effects_bass;
	effects_treble = cfg_effects_treble;
	effects_echo_depth = cfg_effects_echo_depth;
}

input_gep::~input_gep()
{
	if (emu)
	{
		handle_warning();
		delete emu;
	}

	delete buffer;

	if ( monitoring ) monitor_stop();
}

void input_gep::handle_warning()
{
	const char * s = emu->warning();
	if ( s )
	{
		pfc::string8 path;
		filesystem::g_get_display_path( m_path, path );
		console::formatter() << "Emulation warning: " << s << " (" << path << ")";
	}
}

void input_gep::monitor_start()
{
	monitoring = true;
	::monitor_start( emu, m_path );
}

void input_gep::monitor_update()
{
	::monitor_update( emu );
}

void input_gep::monitor_stop()
{
	monitoring = false;
	::monitor_stop( emu );
}

void input_gep::setup_effects( bool echo )
{
	if ( effects_enable )
	{
		Music_Emu::equalizer_t eq;
		// bass - logarithmic, 2 to 8194 Hz
		double bass = double( 255 - effects_bass ) / 255;
		eq.bass = std::pow( 2.0, bass * 13 ) + 2.0;

		// treble - level from -108 to 0 to 5 dB
		double treble = double( effects_treble - 128 ) / 128;
		eq.treble = treble * ( ( treble > 0 ) ? 16.0 : 80.0 ) - 8.0;

		emu->set_equalizer( eq );

		if ( echo && effects_echo_depth > 0 )
		{
			if ( ! buffer ) buffer = new Effects_Buffer;
			emu->set_buffer( buffer );
		}
	}
}

void input_gep::setup_effects_2()
{
	if ( effects_enable && effects_echo_depth > 0 )
	{
		double depth = double( effects_echo_depth ) / 255;
		Effects_Buffer::config_t & cfg = buffer->config();

		cfg.simple.stereo = 0.6 * depth;
		cfg.simple.echo = 0.30 * depth;
		cfg.simple.enabled = true;
		cfg.enabled = true;

		buffer->apply_config();
	}
}

void input_gep::open( service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
{
	if ( p_filehint.is_empty() )
	{
		filesystem::g_open( m_file, p_path, ( p_reason == input_open_info_write ) ? filesystem::open_mode_write_existing : filesystem::open_mode_read, p_abort );
	}
	else m_file = p_filehint;

	m_stats = m_file->get_stats( p_abort );

	m_path = p_path;

	tag_song_ms = cfg_default_length;
	tag_fade_ms = cfg_default_fade;
}

unsigned input_gep::get_subsong_count()
{
	return 1;
}

t_uint32 input_gep::get_subsong( unsigned p_index )
{
	return p_index;
}

t_filestats input_gep::get_file_stats( abort_callback & p_abort )
{
	return m_stats;
}

void input_gep::decode_initialize( t_uint32 p_subsong, unsigned p_flags, abort_callback & p_abort )
{
	played = 0;
	no_infinite = !cfg_indefinite || ( p_flags & input_flag_no_looping );

	/*if ( p_flags & input_flag_playback )*/ monitor_start();

	subsong = p_subsong;
	emu->start_track( subsong );
	handle_warning();

	if ( no_infinite )
	{
		/*song_len=MulDiv(tag_song_ms, sample_rate, 1000);
		fade_len=MulDiv(tag_fade_ms, sample_rate, 1000);*/
		//int fade_min = ( 512 * 8 * 1000 / 2 + sample_rate - 1 ) / sample_rate;
		emu->set_fade( tag_song_ms, tag_fade_ms /*max( tag_fade_ms, fade_min )*/ );
	}

	stop_on_errors = !! ( p_flags & input_flag_testing_integrity );

	first_block = true;
}

bool input_gep::decode_run( audio_chunk & p_chunk,abort_callback & p_abort )
{
	if ( ! emu->track_ended() )
	{
		//if ( no_infinite && played >= song_len + fade_len ) return false;

		if ( monitoring ) monitor_update();

		sample_buffer.grow_size( 1024 );
		blip_sample_t * buf = sample_buffer.get_ptr();
		ERRCHK( emu->play( 1024, buf ) );

		if ( stop_on_errors )
		{
			const char * s = emu->warning();
			if ( s ) throw exception_io_data( s );
		}

		/*int d_start,d_end;
		d_start = played;*/
		played += 512;
		/*d_end   = played;

		if ( no_infinite && song_len && d_end > song_len )
		{
			blip_sample_t * foo = buf;
			for(int n = d_start; n < d_end; n++)
			{
				if ( n > song_len )
				{
					if ( n <= song_len + fade_len )
					{
						int bleh = song_len + fade_len - n;
						*foo++ = MulDiv( *foo, bleh, fade_len );
						*foo++ = MulDiv( *foo, bleh, fade_len );
					}
					else
					{
						*foo++ = 0;
						*foo++ = 0;
					}
				}
			}
		}*/

		p_chunk.set_data_fixedpoint( buf, 1024 * sizeof( blip_sample_t ), sample_rate, 2, sizeof( blip_sample_t ) * 8, audio_chunk::channel_config_stereo );

		return true;
	}

	return false;
}

void input_gep::decode_seek( double p_seconds, abort_callback & p_abort )
{
	first_block = true;

	int now = int( audio_math::time_to_samples( p_seconds, sample_rate ) );
	if ( now == played ) return;
	else if ( now < played )
	{
		emu->start_track( subsong );
		handle_warning();
		played = 0;
	}

	/*sample_buffer.grow_size( 1024 );
	blip_sample_t * buf = sample_buffer.get_ptr();*/
	int played_ = played;

	int ten_seconds = int( sample_rate ) * 10;
	while ( now - played_ > ten_seconds )
	{
		p_abort.check();
		ERRCHK( emu->skip( ten_seconds * 2 ) );
		played_ += ten_seconds;
	}

	ERRCHK( emu->skip( ( now - played_ ) * 2 ) );

	played = played_;
}

bool input_gep::decode_can_seek()
{
	return true;
}

bool input_gep::decode_get_dynamic_info( file_info & p_out, double & p_timestamp_delta )
{
	if ( first_block )
	{
		p_out.info_set_int( "samplerate", sample_rate );
		p_timestamp_delta = 0.;
		first_block = false;
		return true;
	}
	return false;
}

bool input_gep::decode_get_dynamic_info_track( file_info & p_out, double & p_timestamp_delta )
{
	return false;
}

void input_gep::decode_on_idle( abort_callback & p_abort )
{
}

void input_gep::retag_set_info( t_uint32 p_subsong, const file_info & p_info, abort_callback & p_abort )
{
	throw exception_io_data();
}

void input_gep::retag_commit( abort_callback & p_abort )
{
	throw exception_io_data();
}

bool input_gep::g_is_our_content_type( const char * p_content_type )
{
	return false;
}