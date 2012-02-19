#define MY_VERSION "0.4"

/*
	changelog

2012-02-19 19:49 UTC - kode54
- Added abort check to decoder
- Version is now 0.4

2010-01-11 20:10 UTC - kode54
- Added filename validator
- Version is now 0.3

2009-07-31 00:43 UTC - kode54
- Changed AviSynth handling to read the scripts from ANSI path using Import function to fix relative paths
- Implemented tag writing to support editing scripts
- Version is now 0.2

*/

#include <foobar2000.h>

//#pragma pack
//#include "internal.h"
#include "avisynth.h"
//#include <windows.h>

const char tag_script[] = "AviSynth script";

/*
enum {
	raw_bits_per_sample = 16,
	raw_channels = 2,
	raw_sample_rate = 44100,

	raw_bytes_per_sample = raw_bits_per_sample / 8,
	raw_total_sample_width = raw_bytes_per_sample * raw_channels,
};
*/

// No inheritance. Our methods get called over input framework templates. See input_singletrack_impl for descriptions of what each method does.
class input_avs
{
	service_ptr_t<file> m_file;
	t_filestats m_stats;
	t_input_open_reason m_reason;
	pfc::string8 m_path, m_local_path, m_script;
	pfc::array_t<t_uint8> m_buffer;

	PClip m_clip;
	AVSValue* m_res;
	IScriptEnvironment* m_env;
	HMODULE m_dll;
	VideoInfo m_inf;
	__int64 m_pos;

	void cleanUp()
	{
		m_clip = NULL;
		m_pos = 0;

		memset( &m_inf, 0, sizeof( m_inf ) );

		delete m_res;
		m_res = NULL;

		delete m_env;
		m_env = NULL;

		if(m_dll)
		{
			FreeLibrary( m_dll );
			m_dll = NULL;
		}
	}

	void loadClip( abort_callback & p_abort )
	{
		if ( m_reason == input_open_info_write )
			m_file.release();

		m_dll = LoadLibrary( L"avisynth.dll" );
		if( !m_dll )  throw exception_io_data( "Cannot load avisynth.dll" );

		IScriptEnvironment* (__stdcall * CreateScriptEnvironment)(int version) = (IScriptEnvironment*(__stdcall *)(int)) GetProcAddress( m_dll, "CreateScriptEnvironment" );
		if( !CreateScriptEnvironment ) throw exception_io_data( "Cannot load CreateScriptEnvironment" );

		m_env = CreateScriptEnvironment( AVISYNTH_INTERFACE_VERSION );
		if (m_env == NULL) throw exception_io_data( "Requires AviSynth 2.5" );

		try
		{
			char * scr = (char *) m_local_path.get_ptr();
			AVSValue arg( scr );
			AVSValue res = m_env->Invoke( "Import", AVSValue( &arg, 1 ) );
			m_clip = res.AsClip();
			m_inf = m_clip->GetVideoInfo();
			if( !m_inf.HasAudio() )
				throw exception_io_unsupported_format( "AviSynth script does not return any audio" );

			if( m_inf.sample_type == SAMPLE_FLOAT )
			{
				res = m_env->Invoke( "ConvertAudioTo24Bit", AVSValue( &res, 1 ) );
				m_clip = res.AsClip();
				m_inf = m_clip->GetVideoInfo();
			}
		}
		catch( AvisynthError err )
		{
			throw exception_io_data( err.msg );
		}

		m_pos = 0;

		if ( m_reason == input_open_info_write )
			input_open_file_helper( m_file, m_path, m_reason, p_abort );
	}

public:
	input_avs()
	{
		m_res = NULL;
		m_clip = NULL;
		m_env = NULL;
		m_dll = NULL;
	}

	~input_avs()
	{
		cleanUp();
	}

	void open( service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		cleanUp();

		if ( stricmp_utf8_partial( p_path, "file://" ) )
			throw exception_io_unsupported_format( "Only supports local files" );

		m_path = p_path;
		m_local_path = pfc::stringcvt::string_ansi_from_utf8( p_path + 7 );
		if ( m_local_path.find_first( '?' ) != ~0 )
			throw exception_io_unsupported_format( "Only supports ANSI paths" );

		//if (p_reason == input_open_info_write) throw exception_io_unsupported_format();//our input does not support retagging.

		m_file = p_filehint;//p_filehint may be null, hence next line
		input_open_file_helper( m_file, p_path, p_reason, p_abort );//if m_file is null, opens file with appropriate privelages for our operation (read/write for writing tags, read-only otherwise).

		m_stats = m_file->get_stats( p_abort );

		t_filesize scriptSize = m_file->get_size_ex( p_abort );
		if ( scriptSize > UINT_MAX )
			throw exception_io_unsupported_format( "Script is too large" );

		m_file->read_string_ex( m_script, scriptSize, p_abort );

		m_reason = p_reason;
	}

	void get_info( file_info & p_info, abort_callback & p_abort ) 
	{
		if ( !m_dll ) loadClip( p_abort );

		p_info.set_length( audio_math::samples_to_time( m_inf.num_audio_samples, m_inf.audio_samples_per_second ) );
		//note that the values below should be based on contents of the file itself, NOT on user-configurable variables for example. To report info that changes independently from file contents, use get_dynamic_info/get_dynamic_info_track instead.
		p_info.info_set_int( "samplerate", m_inf.audio_samples_per_second );
		p_info.info_set_int( "channels", m_inf.nchannels );
		p_info.info_set_int( "bitspersample", m_inf.BytesPerChannelSample() * 8 );
		p_info.info_set( "encoding", "lossless" );
		p_info.meta_add( tag_script, pfc::stringcvt::string_utf8_from_ansi( m_script ) );
		int bitrate = ( 8 * m_inf.BytesPerAudioSample() * m_inf.audio_samples_per_second + 500 );
		bitrate /= 1000;
		p_info.info_set_bitrate( bitrate );
	}

	t_filestats get_file_stats( abort_callback & p_abort ) { return m_stats; }

	void decode_initialize( unsigned p_flags, abort_callback & p_abort )
	{
		if ( !m_dll ) loadClip( p_abort );

		m_pos = 0;
	}

	bool decode_run( audio_chunk & p_chunk, abort_callback & p_abort )
	{
		p_abort.check();

		int	deltaread = (int) max( min( m_inf.num_audio_samples - m_pos , 1024 ), 0 );

		if( 0 == deltaread ) return false;

		int bytes = deltaread * m_inf.BytesPerAudioSample();

		try 
		{
			m_buffer.set_size( bytes );
			m_clip->GetAudio( m_buffer.get_ptr(), m_pos, deltaread, m_env );
			m_pos += deltaread;
		}
		catch( AvisynthError err )
		{
			throw exception_io_data( err.msg );
		}

		p_chunk.set_data_fixedpoint( m_buffer.get_ptr(), bytes, m_inf.audio_samples_per_second, m_inf.nchannels, m_inf.BytesPerChannelSample() * 8, audio_chunk::g_guess_channel_config( m_inf.nchannels ) );

		return true;
	}

	void decode_seek( double p_seconds, abort_callback & p_abort )
	{
		// IMPORTANT: convert time to sample offset with proper rounding! audio_math::time_to_samples does this properly for you.
		m_pos = min( audio_math::time_to_samples( p_seconds, m_inf.audio_samples_per_second ), (t_uint64) m_inf.num_audio_samples );
	}
	bool decode_can_seek() {return true;}
	bool decode_get_dynamic_info( file_info & p_out, double & p_timestamp_delta ) {return false;}
	bool decode_get_dynamic_info_track( file_info & p_out, double & p_timestamp_delta ) {return false;}
	void decode_on_idle( abort_callback & p_abort ) {m_file->on_idle( p_abort );}

	void retag( const file_info & p_info, abort_callback & p_abort )
	{
		const char * p_script = p_info.meta_get( tag_script, 0 );
		if ( p_script )
		{
			m_script = pfc::stringcvt::string_ansi_from_utf8( p_script );
			m_file->seek( 0, p_abort );
			m_file->set_eof( p_abort );
			m_file->write_string_raw( m_script, p_abort );
			m_stats = m_file->get_stats( p_abort );
		}
	}
	
	static bool g_is_our_content_type( const char * p_content_type ) { return stricmp_utf8( p_content_type, "text/avisynth" ) == 0; }
	static bool g_is_our_path( const char * p_path, const char * p_extension ) { return stricmp_utf8_partial( p_path, "file://" ) == 0 && stricmp_utf8( p_extension, "avs" ) == 0; }
};

static input_singletrack_factory_t<input_avs> g_input_avs_factory;


DECLARE_COMPONENT_VERSION("AVS input",MY_VERSION,"written by Dmitry Alexandrov aka dimzon\ndimzon541@gmail.com");
DECLARE_FILE_TYPE("AviSynth files","*.AVS");
VALIDATE_COMPONENT_FILENAME("foo_input_avs.dll");
