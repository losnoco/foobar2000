#include <foobar2000.h>

#include "source/libpcm/libpcm.h"
//#include "source/libpcm/fader.h"
#include "source/pversion.h"

#include "reader_foo.h"

extern cfg_int cfg_loop;

FOOBAR2000COMPONENT_EXTS(foobar2000_extlist);

#define UNIT_RENDER 4096

class input_okiadpcm
{
	pfc::array_t<t_int8> buffer;
	libpcm_IReader *stream;
	LIBPCM_DECODER *decoder;
	//LIBPCM_FADER *fader;

	service_ptr_t<file> m_file;

public:
	input_okiadpcm()
	{
		stream = 0;
		decoder = 0;
		//fader = 0;
	}

	~input_okiadpcm()
	{
		//if (fader) libpcm_fader_terminate(fader);
		if (decoder) libpcm_close(decoder);
		if ( stream ) stream->lpVtbl->Release( stream );
	}

	void open( service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		if ( p_reason == input_open_info_write ) throw exception_io_unsupported_format();

		if ( p_filehint.is_empty() )
		{
			filesystem::g_open( m_file, p_path, filesystem::open_mode_read, p_abort );
		}
		else m_file = p_filehint;

		stream = libpcm_CreateFooReader( m_file, p_abort );
		if ( ! stream ) throw exception_io_data();

		decoder = libpcm_open_from_stream( stream );
		if ( ! decoder ) throw exception_io_data();

		/*fader = libpcm_fader_initialize(decoder);
		if (!fader) return io_result_error_out_of_memory;*/
	}

	void get_info( file_info & p_info, abort_callback & p_abort )
	{
		p_info.set_length( double( libpcm_get_length( decoder ) ) / double( libpcm_get_samplerate( decoder ) ) );
		p_info.info_set_int("bitrate",  libpcm_get_bitrate( decoder ) / 1000 );
		p_info.info_set_int("samplerate", libpcm_get_samplerate( decoder ) );
		p_info.info_set_int("channels", libpcm_get_numberofchannels( decoder ) );
		p_info.info_set_int("bitspersample", libpcm_get_bitspersample( decoder ) );
		p_info.info_set("codec", libpcm_get_codecname( libpcm_get_codec( decoder ) ) );
		p_info.info_set( "encoding", "lossy" );

		uint32_t loop_length = libpcm_get_looplength( decoder );
		if ( loop_length )
		{
			p_info.info_set_int( "oki_loop_length", loop_length );
		}
	}

	t_filestats get_file_stats( abort_callback & p_abort )
	{
		return m_file->get_stats( p_abort );
	}

	void decode_initialize( unsigned p_flags, abort_callback & p_abort )
	{
		libpcm_SetFooReaderAbort( stream, p_abort );

		if ( cfg_loop && ! ( p_flags & input_flag_no_looping ) && libpcm_get_looplength( decoder ) )
		{
			libpcm_switch_loop( decoder, 1 );
		}
		else
		{
			libpcm_switch_loop( decoder, 0 );
		}

		libpcm_seek( decoder, 0 );
	}

	bool decode_run( audio_chunk & p_chunk, abort_callback & p_abort )
	{
		libpcm_SetFooReaderAbort( stream, p_abort );

		uint_t ba = libpcm_get_blockalign( decoder );
		buffer.grow_size( ba * UNIT_RENDER );

		char * ptr = buffer.get_ptr();
		uint_t numread = libpcm_read( decoder, ptr, UNIT_RENDER );
		if ( ! numread ) return false;

		uint_t nch = libpcm_get_numberofchannels( decoder );
		p_chunk.set_data_fixedpoint( ptr, ba * numread, libpcm_get_samplerate( decoder ), nch, libpcm_get_bitspersample( decoder ), audio_chunk::g_guess_channel_config( nch ) );

		return true;
	}

	void decode_seek( double p_seconds, abort_callback & p_abort )
	{
		libpcm_SetFooReaderAbort( stream, p_abort );

		libpcm_seek( decoder, unsigned( audio_math::time_to_samples( p_seconds, libpcm_get_samplerate( decoder ) ) ) );
	}

	bool decode_can_seek()
	{
		return true;
	}

	bool decode_get_dynamic_info( file_info & p_out, double & p_timestamp_delta )
	{
		return false;
	}

	bool decode_get_dynamic_info_track( file_info & p_out, double & p_timestamp_delta )
	{
		return false;
	}

	void decode_on_idle( abort_callback & p_abort )
	{
		m_file->on_idle( p_abort );
	}

	void retag( const file_info & p_info,abort_callback & p_abort )
	{
		throw exception_io_unsupported_format();
	}

	static bool g_is_our_content_type( const char * p_content_type )
	{
		return false;
	}

	static bool g_is_our_path( const char * p_full_path, const char * p_extension )
	{
		for (int i = 0; foobar2000_extlist[i]; i++)
			if (!stricmp_utf8(p_extension, foobar2000_extlist[i])) return true;
		return false;
	}
};

class okiadpcm_file_types : public input_file_type
{
	virtual unsigned get_count()
	{
		return 10;
	}

	virtual bool get_name(unsigned idx, pfc::string_base & out)
	{
		static const char * names[] = {
			"OKI-ADPCM / Circus-XPCM sample files",
			"Konami-Gungage-XA sample files",
			"KID-WAF sample files",
			"BasiL-WPD sample files",
			"TamaSoft-MPF sample files",
			"StudioMiris-WDT sample files",
			"Hayashigumi-KWF sample files",
			"VisualArts-NWA sample files",
			"jANIS-PX sample files",
			"CLOVER-BW sample files"/*,
			"AbogadoPowers-ADP sample files"*/
		};
		if (idx >= tabsize( names ) ) return false;
		out = names[ idx ];
		return true;
	}

	virtual bool get_mask(unsigned idx, pfc::string_base & out)
	{
		if (idx >= 10) return false;
		out = "*.";
		out += foobar2000_extlist[ idx ];
		return true;
	}

	virtual bool is_associatable(unsigned idx)
	{
		return true;
	}
};

static input_singletrack_factory_t<input_okiadpcm>                      g_okiadpcm_factory;
static service_factory_single_t   <okiadpcm_file_types> g_input_file_type_okiadpcm_factory;

//DECLARE_COMPONENT_VERSION(FOOBAR2000COMPONENT_NAME,FOOBAR2000COMPONENT_VERSION,"");
