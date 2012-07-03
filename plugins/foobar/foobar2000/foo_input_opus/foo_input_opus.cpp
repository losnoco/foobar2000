// foo_input_opus.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#define MAX_FRAME_SIZE (960*6)

#define readint(buf, base) (((buf[base+3]<<24)&0xff000000)| \
                           ((buf[base+2]<<16)&0xff0000)| \
                           ((buf[base+1]<<8)&0xff00)| \
                           (buf[base]&0xff))

const int MAXCHANNELS = 8;
const int chmap[MAXCHANNELS][MAXCHANNELS] = {
	{ 0, },						// mono
	{ 0, 1, },					// l, r
	{ 0, 2, 1, },				// l, c, r -> l, r, c
	{ 0, 1, 2, 3, },			// l, r, bl, br
	{ 0, 2, 1, 3, 4, },			// l, c, r, bl, br -> l, r, c, bl, br
	{ 0, 2, 1, 5, 3, 4, },		// l, c, r, bl, br, lfe -> l, r, c, lfe, bl, br
	{ 0, 2, 1, 6, 3, 4, 5, },	// l, c, r, sl, sr, bc, lfe -> l, r, c, lfe, sl, sr, bc
	{ 0, 2, 1, 7, 5, 6, 3, 4 },	// l, c, r, sl, sr, bl, br, lfe -> l, r, c, lfe, bl, br, sl, sr
};

class packet_decoder_opus : public packet_decoder
{
	unsigned preskip;
	//float gain;
	unsigned skip_packet;

	file_info_impl info;

	OpusHeader header;
	unsigned char stream_map[256];
	OpusMSDecoder * st;

	pfc::array_t<float> sample_buffer;

	void init()
	{
		int err;
		st = opus_multistream_decoder_create( 48000, header.channels, header.nb_streams, header.nb_coupled, stream_map, &err );
		if ( err != OPUS_OK )
		{
			throw exception_io_data( opus_strerror( err ) );
		}
		if ( st == NULL )
		{
			throw exception_io_data( opus_strerror( err ) );
		}

		preskip = header.preskip;

		//gain = pow(10., header.gain/5120.);

		sample_buffer.set_count( MAX_FRAME_SIZE * header.channels );
	}

	void cleanup()
	{
		if ( st ) opus_multistream_decoder_destroy( st );
		st = NULL;
	}

#if 0
	void remap_channels( float * in, audio_sample * out, unsigned sample_count )
	{
		unsigned channel_count = header.channels;

		for ( unsigned channel = 0; channel < channel_count; ++channel )
		{
			audio_sample * out_ptr = out + channel;
			float * in_ptr = in + chmap[ channel_count - 1 ][ channel ];
			for ( unsigned sample = 0; sample < sample_count; ++sample )
			{
				*out_ptr = *in_ptr;
				out_ptr += channel_count;
				in_ptr += channel_count;
			}
		}
	}
#endif

	void remap_channels()
	{
		unsigned channel_count = header.channels;

		for ( unsigned channel = 0; channel < channel_count; ++channel )
		{
			stream_map[ channel ] = header.stream_map[ chmap[ channel_count - 1 ][ channel ] ];
		}
	}

	void read_comments( const char * c, unsigned size )
	{
		if ( size < ( 8 + 4 + 4 ) ) throw exception_io_data( "Invalid Opus comments packet" );

		if ( strncmp( c, "OpusTags", 8 ) != 0 ) throw exception_io_data( "Invalid Opus comments packet" );
		c += 8;

		int len = readint( c, 0 );
		c += 4;

		if ( len < 0 || len > ( size - 16 ) ) throw exception_io_data( "Invalid Opus comments packet" );
		if ( len ) info.info_set( "tool", pfc::string8( c, len ) );
		c += len;

		int nb_fields = readint( c, 0 );
		c += 4;
		size -= 16 + len;

		if ( nb_fields < 0 || nb_fields > ( size >> 2 ) ) throw exception_io_data( "Invalid Opus comments packet" );

		double track_gain = 0.;
		bool track_gain_present = false;

		for ( int i = 0; i < nb_fields; ++i )
		{
			if ( size < 4 ) throw exception_io_data( "Invalid Opus comments packet" );
			len = readint( c, 0 );
			c += 4;
			size -= 4;

			if ( len < 0 || len > size ) throw exception_io_data( "Invalid Opus comments packet" );

			pfc::string8_fast name, value;

			name.set_string( c, len );
			c += len;
			size -= len;

			t_size equals = name.find_first( '=' );

			if ( equals != 0 && equals != ~0 )
			{
				value = name.get_ptr() + equals + 1;
				name.truncate( equals );
			}
			else
			{
				value = name.get_ptr() + ( ( equals == 0 ) ? 1 : 0 );
				name = "COMMENT";
			}

#if 0
			if ( !info.info_set_replaygain( name, value ) )
				info.meta_add( name, value );
#endif
			if ( stricmp_utf8( name, "R128_TRACK_GAIN" ) == 0 )
			{
				if ( track_gain_present ) throw exception_io_data( "Multiple Opus R128 track gain values found" );
				char * temp;
				int track_gain_int = strtol( value, &temp, 10 );
				if ( *temp || track_gain_int < -32768 || track_gain_int > 32767 ) throw exception_io_data( "Invalid Opus R128 track gain value" );
				track_gain = track_gain_int / 256.;
				track_gain_present = true;
			}
			else if ( stricmp_utf8( name, "METADATA_PICTURE_BLOCK" ) == 0 )
			{
			}
			else info.meta_add( name, value );
		}

		double album_gain = header.gain / 256.;

		/*
			This is going by the standard, which says that album gain will be written to the header,
			and track gain will be written to a tag as an offset from the album gain.

			It also suggests that track-only gain should probably be written to the header field and a zero
			value written to the track gain tag, as various supporting software is more likely to support the
			header value than the tag.
		*/

		replaygain_info rg = info.get_replaygain();
		if ( track_gain_present && !track_gain )
		{
			if ( album_gain ) rg.m_track_gain = album_gain;
		}
		else
		{
			if ( album_gain ) rg.m_album_gain = album_gain;
			if ( track_gain ) rg.m_track_gain = album_gain + track_gain;
		}
		info.set_replaygain( rg );
	}

public:
	packet_decoder_opus()
	{
		st = NULL;
	}

	~packet_decoder_opus()
	{
		cleanup();
	}

	virtual t_size set_stream_property ( const GUID &p_type, t_size p_param1, const void *p_param2, t_size p_param2size )
	{
		if ( p_type == property_ogg_header )
		{
			// First header packet will be the comments packet
			if ( p_param2size == sizeof( ogg_packet ) )
			{
				ogg_packet * comments = ( ogg_packet * ) p_param2;
				read_comments( ( const char * ) comments->packet, comments->bytes );
			}
			return 1;
		}
		else if ( p_type == property_ogg_query_sample_rate )
		{
			return 48000;
		}
		else if ( p_type == property_ogg_packet )
		{
			if ( p_param2size == sizeof( ogg_packet ) )
			{
				ogg_packet * packet = ( ogg_packet * ) p_param2;
				if ( packet->granulepos == 0 ) skip_packet = 1;
			}
			return 0;
		}
		return 0;
	}

	virtual void get_info ( file_info &p_info )
	{
		t_size n, m = info.meta_get_count();
		for ( n = 0; n < m; n++ )
			p_info.copy_meta_single( info, n );
		m = info.info_get_count();
		for ( n = 0; n < m; n++ )
			p_info.copy_info_single( info, n );
		p_info.set_replaygain( info.get_replaygain() );
		p_info.info_set_int( "samplerate", 48000 );
		p_info.info_set_int( "channels", header.channels );
		p_info.info_set( "codec", "Opus" );
		p_info.info_set( "encoding", "lossy" );
	}

	void open( const GUID &p_owner, bool p_decode, t_size p_param1, const void *p_param2, t_size p_param2size, abort_callback &p_abort )
    {
		if ( p_owner == owner_Ogg )
		{
			ogg_packet * setup = ( ogg_packet * ) p_param2;
			opus_header_parse( setup->packet, setup->bytes, &header );
		}
		else if ( p_owner == owner_matroska )
		{
			matroska_setup * setup = ( matroska_setup * ) p_param2;
			opus_header_parse( ( const unsigned char * ) setup->codec_private, setup->codec_private_size, &header );
		}
		remap_channels();
		skip_packet = 0;
	}

	static bool g_is_our_setup( const GUID &p_owner, t_size p_param1, const void *p_param2, t_size p_param2size )
	{
		if ( p_owner == owner_Ogg )
		{
			if ( p_param2size == sizeof( ogg_packet ) )
			{
				ogg_packet * setup = ( ogg_packet * ) p_param2;
				OpusHeader h;
				if ( opus_header_parse( setup->packet, setup->bytes, &h ) > 0 && h.channel_mapping <= 1 && h.channels <= MAXCHANNELS ) return true;
			}
		}
		else if ( p_owner == owner_matroska )
		{
			if ( p_param2size == sizeof( matroska_setup ) )
			{
				matroska_setup * setup = ( matroska_setup * ) p_param2;
				if ( strcmp( setup->codec_id, "A_OPUS" ) == 0 && setup->codec_private != NULL && setup->codec_private_size > 0 )
				{
					OpusHeader h;
					if ( opus_header_parse( ( const unsigned char * ) setup->codec_private, setup->codec_private_size, &h ) > 0 &&
						h.channel_mapping <= 1 && h.channels <= MAXCHANNELS ) return true;
				}
			}
		}
		return false;
	}

	virtual unsigned get_max_frame_dependency()
	{
		return 1;
	}
	virtual double get_max_frame_dependency_time()
	{
		return .1;
	}

	virtual void reset_after_seek()
	{
		cleanup();
	}

	virtual bool analyze_first_frame_supported() { return false; }

	virtual void analyze_first_frame(const void *p_buffer, t_size p_bytes, abort_callback &p_abort)
	{
		return;
	}

	virtual void decode ( const void *data, t_size bytes, audio_chunk &p_chunk, abort_callback &p_abort )
	{
		if ( skip_packet == 1 )
		{
			skip_packet = 0;
			return;
		}

		if ( !st ) init();

		float * ptr = sample_buffer.get_ptr();

		int ret = opus_multistream_decode_float( st, ( const unsigned char * ) data, bytes, ptr, MAX_FRAME_SIZE, 0 );

		if ( ret < 0 ) throw exception_io_data( opus_strerror( ret ) );

		if ( preskip )
		{
			if ( preskip > ret )
			{
				preskip -= ret;
				ret = 0;
			}
			else
			{
				ret -= preskip;
				ptr += preskip * header.channels;
				preskip = 0;
			}
		}

		if ( ret )
		{
			p_chunk.set_data_32( ptr, ret, header.channels, 48000 );
			p_chunk.set_channels( header.channels, audio_chunk::g_guess_channel_config_xiph( header.channels ) );
#if 0
			p_chunk.set_data_size( ret * header.channels );
			p_chunk.set_sample_count( ret );
			p_chunk.set_srate( 48000 );
			p_chunk.set_channels( header.channels, audio_chunk::g_guess_channel_config_xiph( header.channels ) );
			remap_channels( ptr, p_chunk.get_data(), ret );
#endif
			// audio_math::scale( p_chunk.get_data(), ret * header.channels, p_chunk.get_data(), gain );
		}
	}
};

static packet_decoder_factory_t< packet_decoder_opus > g_packet_decoder_opus_factory;
