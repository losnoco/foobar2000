// foo_pd_aac.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"


class packet_decoder_aac : public packet_decoder
{
	HANDLE_AACDECODER decoder;

	pfc::array_t<UCHAR> setup_data;

	pfc::array_t<INT_PCM> sample_buffer;

	unsigned srate, nch, channel_map;

	AUDIO_OBJECT_TYPE aot;

	CStreamInfo info;

	unsigned get_channel_map( CStreamInfo * info )
	{
		static const unsigned channel_maps[8] = {
			audio_chunk::channel_config_mono,
			audio_chunk::channel_config_stereo,
			audio_chunk::channel_config_stereo | audio_chunk::channel_front_center,
			audio_chunk::channel_config_stereo | audio_chunk::channel_back_left | audio_chunk::channel_back_right,
			audio_chunk::channel_config_5point1 - audio_chunk::channel_lfe,
			audio_chunk::channel_config_5point1,
			audio_chunk::channel_config_7point1 - audio_chunk::channel_lfe,
			audio_chunk::channel_config_7point1
		};

		if ( info->channelConfig > 1 && info->channelConfig < 9 ) return channel_maps[ info->channelConfig - 1 ];
		else return audio_chunk::g_guess_channel_config( info->numChannels );
	}

public:
	packet_decoder_aac()
	{
		decoder = NULL;
	}

	~packet_decoder_aac()
	{
		if ( decoder ) aacDecoder_Close( decoder );
	}

	virtual t_size set_stream_property ( const GUID &p_type, t_size p_param1, const void *p_param2, t_size p_param2size )
	{
		return 0;
	}

	virtual void get_info ( file_info &p_info )
	{
		p_info.info_set_int( "samplerate", srate );
		p_info.info_set_int( "channels", nch );
		p_info.info_set( "codec", "AAC" );
		const char * aot_desc = NULL;
		switch (aot)
		{
		case AOT_AAC_MAIN: aot_desc = "Main"; break;
		case AOT_AAC_LC:   aot_desc = "LC"; break;
		case AOT_AAC_SSR:  aot_desc = "SSR"; break;
		case AOT_AAC_LTP:  aot_desc = "LTP"; break;
		case AOT_SBR:      aot_desc = "SBR"; break;
		case AOT_PS:       aot_desc = "SBR+PS"; break;
		}
		if ( info.aacSampleRate * 2 == info.sampleRate ) aot_desc = "SBR";
		if ( info.numChannels == 2 && info.channelConfig == 1 ) aot_desc = "SBR+PS";
		if ( aot_desc ) p_info.info_set( "codec_profile", aot_desc );
		p_info.info_set( "encoding", "lossy" );
	}

	void open( const GUID &p_owner, bool p_decode, t_size p_param1, const void *p_param2, t_size p_param2size, abort_callback &p_abort )
    {
		const UCHAR * codec_setup_data = NULL;
		UINT codec_setup_length = 0;

		if ( p_owner == owner_MP4 )
		{
			codec_setup_data = (const UCHAR *) p_param2;
			codec_setup_length = p_param2size;
		}
		else if ( p_owner == owner_matroska )
		{
			matroska_setup * setup = ( matroska_setup * ) p_param2;
			if ( p_param2size == sizeof(*setup) )
			{
				codec_setup_data = (const UCHAR *) setup->codec_private;
				codec_setup_length = setup->codec_private_size;
			}
		}

		setup_data.set_count( codec_setup_length );
		if ( codec_setup_data && codec_setup_length ) memcpy( setup_data.get_ptr(), codec_setup_data, codec_setup_length );

		decoder = aacDecoder_Open( TT_MP4_RAW, 1 );
		if ( !decoder ) throw std::bad_alloc();

		if ( aacDecoder_ConfigRaw( decoder, (UCHAR**) &codec_setup_data, &codec_setup_length ) ) throw exception_io_data();

		sample_buffer.set_count( MAX_FRAME_SIZE );

		CStreamInfo * info = aacDecoder_GetStreamInfo( decoder );

		srate = info->sampleRate;
		nch = info->numChannels;
		channel_map = get_channel_map( info );
		aot = info->aot;

		this->info = *info;
	}

	static bool g_is_our_setup( const GUID &p_owner, t_size p_param1, const void *p_param2, t_size p_param2size )
	{
		if ( p_owner == owner_ADTS ) { return true; }
		else if ( p_owner == owner_ADIF ) { return true; }
		else if ( p_owner == owner_MP4 )
		{
			if ( p_param1 == 0x40 || p_param1 == 0x66 || p_param1 == 0x67 || p_param1 == 0x68 ) return true;
		}
		else if ( p_owner == owner_matroska )
		{
			const matroska_setup * setup = ( const matroska_setup * ) p_param2;
			if ( p_param2size == sizeof(*setup) )
			{
				if ( !strcmp(setup->codec_id, "A_AAC") ) return true;
				if ( !strncmp(setup->codec_id, "A_AAC/", 6) )
				{
					if ( !strcmp(setup->codec_id + 5, "/MAIN") ||
						!strcmp(setup->codec_id + 5, "/LTP") ||
						!strcmp(setup->codec_id + 5, "/SBR") ||
						!strcmp(setup->codec_id + 5, "/SSR") )
					{
						return true;
					}
				}
			}
		}
		return false;
	}

	virtual unsigned get_max_frame_dependency()
	{
		return 2;
	}
	virtual double get_max_frame_dependency_time()
	{
		return 1024.0 / 8000.0;
	}

	virtual void reset_after_seek()
	{
		aacDecoder_DecodeFrame( decoder, sample_buffer.get_ptr(), MAX_FRAME_SIZE, AACDEC_FLUSH | AACDEC_INTR | AACDEC_CLRHIST );
	}

	virtual bool analyze_first_frame_supported() { return true; }

	virtual void analyze_first_frame(const void *p_buffer, t_size p_bytes, abort_callback &p_abort)
	{
		HANDLE_AACDECODER local_decoder = aacDecoder_Open( TT_MP4_RAW, 1 );
		if ( !local_decoder ) throw exception_io_data();
		UCHAR * setup_ptr = setup_data.get_ptr();
		UINT setup_size = setup_data.get_count();
		UINT bytes_valid;
		if ( aacDecoder_ConfigRaw( local_decoder, &setup_ptr, &setup_size ) )
		{
			aacDecoder_Close( local_decoder );
			throw exception_io_data();
		}
		setup_ptr = (UCHAR*) p_buffer;
		setup_size = p_bytes;
		bytes_valid = p_bytes;
		if ( aacDecoder_Fill( local_decoder, &setup_ptr, &setup_size, &bytes_valid ) || bytes_valid )
		{
			aacDecoder_Close( local_decoder );
			throw exception_io_data();
		}
		if ( aacDecoder_DecodeFrame( local_decoder, sample_buffer.get_ptr(), MAX_FRAME_SIZE, 0 ) )
		{
			aacDecoder_Close( local_decoder );
			throw exception_io_data();
		}
		
		CStreamInfo * info = aacDecoder_GetStreamInfo( local_decoder );

		srate = info->sampleRate;
		nch = info->numChannels;
		channel_map = get_channel_map( info );
		aot = info->aot;

		this->info = *info;

		aacDecoder_Close( local_decoder );
	}

	virtual void decode ( const void *data, t_size bytes, audio_chunk &p_chunk, abort_callback &p_abort )
	{
		UCHAR * data_ptr = ( UCHAR * ) data;
		UINT data_size = bytes;
		UINT bytes_valid = bytes;

		if ( aacDecoder_Fill( decoder, &data_ptr, &data_size, &bytes_valid ) || bytes_valid ) throw exception_io_data();
		if ( aacDecoder_DecodeFrame( decoder, sample_buffer.get_ptr(), MAX_FRAME_SIZE, 0 ) ) throw exception_io_data();

		CStreamInfo * info = aacDecoder_GetStreamInfo( decoder );

		srate = info->sampleRate;
		nch = info->numChannels;
		channel_map = get_channel_map( info );
		aot = info->aot;

		p_chunk.set_data_size( info->frameSize * info->numChannels );
		p_chunk.set_channels( nch, channel_map );
		p_chunk.set_sample_count( info->frameSize );
		p_chunk.set_srate( srate );

		audio_math::convert_from_int32( ( const t_int32 * ) sample_buffer.get_ptr(), info->frameSize * info->numChannels, p_chunk.get_data(), 1 << (32 - SAMPLE_BITS) );
	}
};

class version_pd_aac : public componentversion
{
public:
	virtual void get_file_name(pfc::string_base & out) { out = core_api::get_my_file_name(); }
	virtual void get_component_name(pfc::string_base & out) { out = "fdk-aac packet decoder"; }
	virtual void get_component_version(pfc::string_base & out) { out = "1.0"; }
	virtual void get_about_message(pfc::string_base & out)
	{
		LIB_INFO info[FDK_MODULE_LAST];
		memset(info, 0, sizeof(info));
		aacDecoder_GetLibInfo(info);
		out = "Foobar2000 version by Chris Moeller\n\n";
		for (unsigned i = 0; i < FDK_MODULE_LAST; i++) {
			if (info[i].module_id == FDK_NONE) break;
			out += info[i].title;
			out += " v";
			out += info[i].versionStr;
			out += " built ";
			out += info[i].build_date;
			out += " ";
			out += info[i].build_time;
			out += "\n";
		}
		out += "\nhttp://kode54.foobar2000.org/";
	}
};

static packet_decoder_factory_t< packet_decoder_aac > g_packet_decoder_aac_factory;
static service_factory_single_t<version_pd_aac>       g_componentversion_pd_aac_factory;

VALIDATE_COMPONENT_FILENAME("foo_pd_aac.dll");
