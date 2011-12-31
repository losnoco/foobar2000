// foo_input_tfmx.cpp : Defines the exported functions for the DLL application.
//

#include <stdafx.h>

#define MYVERSION "0.7"

/*

	change log

2011-12-31 17:55 UTC - kode54
- Fixed loop detection for files which don't actually loop
- Skip any non-ending or frozen songs
- Version is now 0.7

2011-12-31 16:59 UTC - kode54
- Changed default length to loop count, with actual length detection
- Version is now 0.6

2011-03-23 02:45 UTC - kode54
- Implemented file stats collection properly for file size and modification time reporting
- Version is now 0.5

2011-03-15 19:40 UTC - kode54
- Worked around an issue with wave seekbar
- Version is now 0.4

2011-03-15 07:11 UTC - kode54
- Fixed stereo separation and changed default to 83%
- Version is now 0.3

2011-03-15 06:56 UTC - kode54
- Hard coded untagged files to 16 subsongs
- Version is now 0.2

2011-03-15 05:35 UTC - kode54
- Initial release ready
- Version is now 0.1

2011-03-15 00:16 UTC - kode54
- Created project and imported old player code

*/

//static const char *compat_txt[4]={"default (Turrican ][ and most of other stuff)","old (Turrican 1, R-Type, X-Out)","emulate other TFMX players","new (Turrican 3)"};

extern advconfig_integer_factory cfg_sample_rate;

static bool tfmx_test_filename(const char * full_path,const char * extension)
{
	int n = pfc::scan_filename( full_path );
	if (n>0)
	{
		if (!strnicmp(full_path+n,"mdat.",5)) return true;
		if (!strnicmp(full_path+n,"tfmx.",5)) return true;
	}
	return !stricmp(extension,"MDAT") || !stricmp(extension,"TFM") || !stricmp(extension,"TFMX") || !stricmp(extension,"TFX");
}

class input_tfmx
{
	CTFMXSource * p_src;

	pfc::string8 m_path;

	t_filestats m_stats;

	int sample_rate;

	bool first_frame;

public:
	input_tfmx()
	{
		p_src = 0;
	}

	~input_tfmx()
	{
		delete p_src;
	}

	void open( service_ptr_t<file> m_file, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		if ( p_reason == input_open_info_write ) throw exception_io_data();

		if ( m_file.is_empty() ) filesystem::g_open( m_file, p_path, filesystem::open_mode_read, p_abort );

		m_stats = m_file->get_stats( p_abort );

		p_src = new CTFMXSource();
		if ( p_src->Open( m_file, p_path, 0, p_abort ) ) throw exception_io_data();

		m_path = p_path;
	}

	unsigned get_subsong_count()
	{
		return p_src->GetSongCount();
	}

	t_uint32 get_subsong( unsigned p_index )
	{
		return p_src->GetSong( p_index );
	}

	void get_info( t_uint32 p_subsong, file_info & p_info, abort_callback & p_abort )
	{
		p_info.info_set( "codec", "TFMX" );
		p_info.info_set_int( "channels", 2 );
		p_info.info_set_int( "tfmx_channels", p_src->multimode ? 7 : 4 );

		bool has_length = false;
		bool has_title = false;
		double length;

		if ( p_src->tag )
		{
			if ( !p_src->tag->artist.is_empty() )
				p_info.meta_set( "artist", pfc::stringcvt::string_utf8_from_ansi( p_src->tag->artist ) );
			if ( !p_src->tag->album.is_empty() )
				p_info.meta_set( "album", pfc::stringcvt::string_utf8_from_ansi( p_src->tag->album ) );
			if ( !p_src->tag->comment.is_empty() )
				p_info.meta_set( "comment", pfc::stringcvt::string_utf8_from_ansi( p_src->tag->comment ) );
			if ( !p_src->tag->date.is_empty() )
				p_info.meta_set( "date", pfc::stringcvt::string_utf8_from_ansi( p_src->tag->date ) );

			SONG * song = p_src->tag->find_song( p_subsong );
			if (song)
			{
				length = (double) ( song->len + song->fade ) * 0.001;
				has_length = song->len || song->fade;

				if ( !song->title.is_empty() )
				{
					has_title = true;
					p_info.meta_set( "title", pfc::stringcvt::string_utf8_from_ansi( song->title ) );
				}
			}

			if ( !has_title && !p_src->tag->title.is_empty() )
			{
				has_title = true;
				p_info.meta_set( "title", pfc::stringcvt::string_utf8_from_ansi( p_src->tag->title ) );
			}
		}

		if ( !has_title )
		{
			t_size n = m_path.scan_filename();
			if ( n > 0 )
			{
				if (!strnicmp(m_path+n,"mdat.",5)) has_title = true;
				else if (!strnicmp(m_path+n,"tfmx.",5)) has_title = true;
				if ( has_title )
				{
					p_info.meta_set( "title", m_path + n + 5 );
				}
			}
		}

		if ( !has_length )
		{
			length = p_src->GetLength( p_subsong );
		}

		p_info.set_length( length );
	}

	t_filestats get_file_stats( abort_callback & p_abort )
	{
		return m_stats;
	}

	void decode_initialize( t_uint32 p_subsong, unsigned p_flags, abort_callback & p_abort )
	{
		p_src->cur_song = p_subsong;
		p_src->init_song();
		if ( p_flags & input_flag_no_looping ) p_src->has_len = true;
		first_frame = true;
		sample_rate = 0;
	}

	bool decode_run( audio_chunk & p_chunk,abort_callback & p_abort )
	{
		p_chunk.set_data_size( 1024 * 2 );
		int n = p_src->GetSamples( p_chunk.get_data(), 1024, &sample_rate );
		if ( !n ) return false;
		p_chunk.set_sample_count( n );
		p_chunk.set_srate( sample_rate );
		p_chunk.set_channels( 2 );
		return true;
	}

	void decode_seek( double p_seconds, abort_callback & p_abort )
	{
		first_frame = true;
		sample_rate = 0;
		p_src->SetPosition( p_seconds );
	}

	bool decode_can_seek()
	{
		return true;
	}

	bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta )
	{
		if ( first_frame )
		{
			first_frame = false;
			if ( !sample_rate )
			{
				sample_rate = cfg_sample_rate.get();
				console::formatter() << "[foo_input_tfmx] Somebody called decode_get_dynamic_info without calling decode_run first";
			}
			p_out.info_set_int( "samplerate", sample_rate );
			p_timestamp_delta = 0;
			return true;
		}
		return false;
	}

	bool decode_get_dynamic_info_track( file_info & p_out, double & p_timestamp_delta )
	{
		return false;
	}

	void decode_on_idle( abort_callback & p_abort )
	{
	}

	void retag_set_info( t_uint32 p_subsong, const file_info & p_info, abort_callback & p_abort )
	{
		throw exception_io_unsupported_format();
	}

	void retag_commit( abort_callback & p_abort )
	{
		throw exception_io_unsupported_format();
	}

	static bool g_is_our_content_type( const char * p_content_type )
	{
		return false;
	}

	static bool g_is_our_path( const char * p_path, const char * p_extension )
	{
		return tfmx_test_filename( p_path, p_extension );
	}
};

static input_factory_t<input_tfmx> g_input_tfmx_factory;

DECLARE_FILE_TYPE("TFMX files", "*.TFM");

DECLARE_COMPONENT_VERSION("TFMX decoder",MYVERSION,"MDAT.*;TFMX.*;*.MDAT;*.TFM;*.TFMX;*.TFX");

VALIDATE_COMPONENT_FILENAME("foo_input_tfmx.dll");
