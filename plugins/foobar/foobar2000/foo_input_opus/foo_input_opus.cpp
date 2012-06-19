// foo_input_opus.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#define MAX_FRAME_SIZE (960*6)

#define readint(buf, base) (((buf[base+3]<<24)&0xff000000)| \
                           ((buf[base+2]<<16)&0xff0000)| \
                           ((buf[base+1]<<8)&0xff00)| \
                           (buf[base]&0xff))

class packet_decoder_opus : public packet_decoder
{
	unsigned preskip;
	float gain;

	file_info_impl info;

	OpusHeader header;
	OpusMSDecoder * st;

	pfc::array_t<float> sample_buffer;

	void init()
	{
		int err;
		st = opus_multistream_decoder_create( 48000, header.channels, header.nb_streams, header.nb_coupled, header.stream_map, &err );
		if ( err != OPUS_OK )
		{
			throw exception_io_data( opus_strerror( err ) );
		}
		if ( st == NULL )
		{
			throw exception_io_data( opus_strerror( err ) );
		}

		preskip = header.preskip;

		gain = pow(10., header.gain/5120.);

		sample_buffer.set_count( MAX_FRAME_SIZE * header.channels );
	}

	void cleanup()
	{
		if ( st ) opus_multistream_decoder_destroy( st );
		st = NULL;
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
			if ( equals == ~0 || equals == 0 ) throw exception_io_data( "Invalid Opus comments packet" );

			value = name.get_ptr() + equals + 1;
			name.truncate( equals );

			if ( !info.info_set_replaygain_ex( name, name.length(), value, value.length() ) )
				info.meta_add( name, value );
		}
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
				read_comments( ( const char * ) comments->header, comments->header_size );
			}
			return 1;
		}
		else if ( p_type == property_ogg_query_sample_rate )
		{
			return 48000;
		}
		else if ( p_type == property_ogg_packet )
		{
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
	}

	void open( const GUID &p_owner, bool p_decode, t_size p_param1, const void *p_param2, t_size p_param2size, abort_callback &p_abort )
    {
		ogg_packet * setup = ( ogg_packet * ) p_param2;
		opus_header_parse( setup->header, setup->header_size, &header );
	}

	static bool g_is_our_setup( const GUID &p_owner, t_size p_param1, const void *p_param2, t_size p_param2size )
	{
		if ( p_owner == owner_Ogg )
		{
			if ( p_param2size == sizeof( ogg_packet ) )
			{
				ogg_packet * setup = ( ogg_packet * ) p_param2;
				OpusHeader h;
				if ( opus_header_parse( setup->header, setup->header_size, &h ) > 0 ) return true;
			}
		}
		return false;
	}

	virtual unsigned get_max_frame_dependency() { return 5; }
	virtual double get_max_frame_dependency_time() { return .5; }

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
			audio_math::scale( p_chunk.get_data(), ret * header.channels, p_chunk.get_data(), gain );
		}
	}
};

static packet_decoder_factory_t< packet_decoder_opus > g_packet_decoder_opus_factory;
