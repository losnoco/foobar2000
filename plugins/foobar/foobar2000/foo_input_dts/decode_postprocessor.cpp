#include "../SDK/foobar2000.h"
#include "../SDK/decode_postprocessor.h"

typedef unsigned int    uint32_t;
typedef signed int      int32_t;
typedef unsigned short  uint16_t;
typedef signed short    int16_t;
typedef unsigned char   uint8_t;
typedef signed char     int8_t;

extern "C" {
#include "dca.h"
}

enum {
	BUFFER_SIZE = 24576,
	HEADER_SIZE = 14,
	FRAME_SAMPLES = 256
};

bool is_chunk_silent( audio_chunk * chunk )
{
	audio_sample * data = chunk->get_data();
	for ( unsigned i = 0, j = chunk->get_data_length(); i < j; i++ )
	{
		if ( data[ i ] ) return false;
	}
	return true;
}

class dts_postprocessor_instance : public decode_postprocessor_instance
{
	dsp_chunk_list_impl original_chunks;
	dsp_chunk_list_impl output_chunks;

	dca_state_t *state;

	pfc::array_t<audio_sample> output;
	pfc::array_t<uint8_t> buffer;
	uint8_t buf[BUFFER_SIZE];
	uint8_t *bufptr, *bufpos;
	bool valid_stream_found;

	int dts_flags, nch, srate, bitrate, frame_length;
	unsigned int channel_mask;

	bool info_emitted, gave_up;

	bool init()
	{
		cleanup();

		state = dca_init(0);
		if (!state)
		{
			console::error("Failed to initialize DTS decoder");
			return false;
		}

		srate = 44100;

		return true;
	}

    void cleanup()
    {
		if (state)
		{
			dca_free(state);
			state = 0;
		}

		original_chunks.remove_all();
		output_chunks.remove_all();

		bufptr = buf;
		bufpos = buf + HEADER_SIZE;
		dts_flags = nch = srate = bitrate = 0;
		valid_stream_found = false;
		info_emitted = false;
		gave_up = false;
	}

	int get_channel_count( int flags )
	{
		int channels = 0, tmp = flags & DCA_CHANNEL_MASK;

		if ( tmp > DCA_CHANNEL_MAX ) return 0;

		if ( tmp == DCA_MONO ) channels = 1;
		else if ( tmp < DCA_3F ) channels = 2;
		else if ( tmp < DCA_3F1R ) channels = 3;
		else if ( tmp < DCA_3F2R ) channels = 4;
		else if ( tmp < DCA_4F2R ) channels = 5;
		//else if ( tmp == DCA_4F2R ) channels = 6; // not supported (version 0.0.5)

		if ( channels && ( flags & DCA_LFE ) ) channels++;

		return channels;
    }

    unsigned int get_channel_map_mask( int flags )
    {
		static const unsigned int output_nch[10] = {
			audio_chunk::channel_front_center,                                  // DCA_MONO
			audio_chunk::channel_front_left | audio_chunk::channel_front_right, // DCA_CHANNEL
			audio_chunk::channel_front_left | audio_chunk::channel_front_right, // DCA_STEREO
			audio_chunk::channel_front_left | audio_chunk::channel_front_right, // DCA_STEREO_SUMDIFF
			audio_chunk::channel_front_left | audio_chunk::channel_front_right, // DCA_STEREO_TOTAL
			audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_front_center, // DCA_3F
			audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_back_center,  // DCA_2F1R
			audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_front_center | audio_chunk::channel_back_center, // DCA_3F1R
			audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_back_left    | audio_chunk::channel_back_right,  // DCA_2F2R
			audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_front_center | audio_chunk::channel_back_left | audio_chunk::channel_back_right, // DCA_3F2R
		};

		unsigned int map = output_nch[ flags & DCA_CHANNEL_MASK ];
		if ( flags & DCA_LFE ) map |= audio_chunk::channel_lfe;
		return map;
	}

	bool decode( const void *data, t_size bytes, audio_chunk & p_chunk, abort_callback & p_abort )
    {
		static const int chan_map[10][6] = {
			{ 0, 0, 0, 0, 0, 0 },       // DCA_MONO
			{ 0, 1, 0, 0, 0, 0 },       // DCA_CHANNEL
			{ 0, 1, 0, 0, 0, 0 },       // DCA_STEREO
			{ 0, 1, 0, 0, 0, 0 },       // DCA_STEREO_SUMDIFF
			{ 0, 1, 0, 0, 0, 0 },       // DCA_STEREO_TOTAL
			{ 2, 0, 1, 0, 0, 0 },       // DCA_3F
			{ 0, 1, 2, 0, 0, 0 },       // DCA_2F1R
			{ 2, 0, 1, 3, 0, 0 },       // DCA_3F1R
			{ 0, 1, 2, 3, 0, 0 },       // DCA_2F2R
			{ 2, 0, 1, 3, 4, 0 },       // DCA_3F2R
		};
		static const int chan_map_lfe[10][6] = {
			{ 0, 1, 0, 0, 0, 0 },       // DCA_MONO
			{ 0, 1, 2, 0, 0, 0 },       // DCA_CHANNEL
			{ 0, 1, 2, 0, 0, 0 },       // DCA_STEREO
			{ 0, 1, 2, 0, 0, 0 },       // DCA_STEREO_SUMDIFF
			{ 0, 1, 2, 0, 0, 0 },       // DCA_STEREO_TOTAL
			{ 2, 0, 1, 3, 0, 0 },       // DCA_3F
			{ 0, 1, 3, 2, 0, 0 },       // DCA_2F1R
			{ 2, 0, 1, 4, 3, 0 },       // DCA_3F1R
			{ 0, 1, 3, 4, 2, 0 },       // DCA_2F2R
			{ 2, 0, 1, 4, 5, 3 },       // DCA_3F2R
		};

		uint8_t *start = (uint8_t *)data;
		uint8_t *end = (uint8_t *)data + bytes;
		unsigned int samples = 0;

		while (1)
		{
			unsigned len = end - start;
			if ( !len ) break;
			if ( len > bufpos - bufptr ) len = bufpos - bufptr;

			memcpy( bufptr, start, len );
			bufptr += len;
			start += len;

			if ( bufptr == bufpos )
			{
				if ( bufpos == buf + HEADER_SIZE )
				{
					int length = dca_syncinfo( state, buf, &dts_flags, &srate, &bitrate, &frame_length );
					if ( !length )
					{
						//console::warning("DTS: skip");
						for ( bufptr = buf; bufptr < buf + HEADER_SIZE - 1; bufptr++ ) bufptr[0] = bufptr[1];
						continue;
					}

					nch = get_channel_count( dts_flags );
					channel_mask = get_channel_map_mask( dts_flags );

					bufpos = buf + length;
				}
				else
				{
					int n;
					audio_sample *out = 0;
					level_t level = 1.0;
					sample_t bias = 0;

					dts_flags &= DCA_CHANNEL_MASK | DCA_LFE;
					if ( dca_frame( state, buf, &dts_flags, &level, bias ) ) goto error;
					//if (use_drc==0)
					dca_dynrng(state, NULL, NULL);

					for ( n = 0; n < dca_blocks_num( state ); n++ )
					{
						if ( dca_block( state ) ) goto error;
						output.grow_size( ( samples + FRAME_SAMPLES ) * nch );
						out = ( audio_sample * ) output.get_ptr() + samples * nch;
						float *src = dca_samples( state );
						if ( src )
						{
							int tmp = dts_flags & ( ~DCA_LFE );

							if ( dts_flags & DCA_LFE )
							{
								for ( int j = 0; j < nch; j++ )
								{
									for ( int i = 0; i < FRAME_SAMPLES; i++ )
									{
										out[ i * nch + chan_map_lfe[ tmp ][ j ] ] = src[ j * FRAME_SAMPLES + i ];
									}
								}
							}
							else
							{
								for (int j = 0; j < nch; j++) {
									for (int i = 0; i < FRAME_SAMPLES; i++) {
										out[i * nch + chan_map[tmp][j]] = src[j * FRAME_SAMPLES + i];
									}
								}
							}

							samples += FRAME_SAMPLES;
						}
					}

					bufptr = buf;
					bufpos = buf + HEADER_SIZE;
					continue;

error:
					return false;
					//console::warning("DTS: error");
					bufptr = buf;
					bufpos = buf + HEADER_SIZE;
				}
			}
		}

		if (samples >= FRAME_SAMPLES)
		{
			p_chunk.set_data( output.get_ptr(), samples, nch, srate, channel_mask );
			return true;
		}
		return false;
	}

	unsigned flush_chunks( dsp_chunk_list & p_chunk_list, unsigned insert_point, bool output = false )
	{
		dsp_chunk_list * list = output ? &output_chunks : &original_chunks;
		unsigned ret = list->get_count();
		if ( ret )
		{
			for ( unsigned i = 0; i < list->get_count(); i++ )
			{
				audio_chunk * in = list->get_item( i );
				audio_chunk * out = p_chunk_list.insert_item( insert_point++, in->get_data_length() );
				out->copy( *in );
			}
		}
		original_chunks.remove_all();
		output_chunks.remove_all();
		return ret;
	}

public:
	dts_postprocessor_instance()
	{
		state = 0;
		cleanup();
	}

	~dts_postprocessor_instance()
	{
		cleanup();
	}

	virtual bool run( dsp_chunk_list & p_chunk_list, t_uint32 p_flags, abort_callback & p_abort )
	{
		if ( gave_up || p_flags & flag_altered ) return false;

		bool modified = false;

		for ( unsigned i = 0; i < p_chunk_list.get_count(); )
		{
			audio_chunk * chunk = p_chunk_list.get_item( i );

			if ( chunk->get_channels() != 2 || chunk->get_srate() != 44100 ) {
				i += flush_chunks( p_chunk_list, i, valid_stream_found ) + 1;
				continue;
			}

			if (!state)
			{
				if (!init()) break;
			}

			int data = chunk->get_sample_count() * 4;
			buffer.grow_size( data );
			audio_math::convert_to_int16( chunk->get_data(), chunk->get_sample_count() * 2, (t_int16 *)buffer.get_ptr(), 1.0 );

			if ( !valid_stream_found )
			{
				audio_chunk * out = original_chunks.insert_item( original_chunks.get_count(), chunk->get_data_length() );
				out->copy( *chunk );
				if ( decode( buffer.get_ptr(), data, *chunk, p_abort ) )
				{
					if ( output_chunks.get_count() )
					{
						valid_stream_found = true;
						i += flush_chunks( p_chunk_list, i, true ) + 1;
						modified = true;
					}
					else
					{
						out = output_chunks.insert_item( output_chunks.get_count(), chunk->get_data_length() );
						out->copy( *chunk );
						p_chunk_list.remove_by_idx( i );
					}
				}
				else
				{
					p_chunk_list.remove_by_idx( i );
					output_chunks.remove_all();
				}
			}
			else
			{
				if ( decode( buffer.get_ptr(), data, *chunk, p_abort ) )
				{
					i++;
					modified = true;
				}
				else
				{
					p_chunk_list.remove_by_idx( i );
				}
			}
		}

		for ( unsigned i = 0; i < original_chunks.get_count(); )
		{
			audio_chunk * in = original_chunks.get_item( i );
			if ( !is_chunk_silent( in ) ) break;
			audio_chunk * out = p_chunk_list.insert_item( p_chunk_list.get_count(), in->get_data_length() );
			out->copy( *in );
			original_chunks.remove_by_idx( i );
		}

		if ( original_chunks.get_duration() >= 1.0 )
		{
			flush_chunks( p_chunk_list, p_chunk_list.get_count() );
			gave_up = true;
		}

		if ( p_flags & flag_eof )
		{
			flush_chunks( p_chunk_list, p_chunk_list.get_count(), valid_stream_found );
			cleanup();
		}

		return modified;
	}

	virtual bool get_dynamic_info( file_info & p_out )
	{
        static const char *dts_mode_list[10] = {
            { "Mono" },
            { "Dual Mono" },
            { "Stereo" },
            { "Stereo (SUMDIFF)" },
            { "Stereo (TOTAL)" },
            { "3 front channels" },
            { "2 front, 1 rear surround channel" },
            { "3 front, 1 rear surround channel" },
            { "2 front, 2 rear surround channels" },
            { "3 front, 2 rear surround channels" }
        };

		if ( !info_emitted )
		{
			if ( valid_stream_found )
			{
				info_emitted = true;
				p_out.info_set_int( "samplerate", srate );
				p_out.info_set_int( "channels", nch );
				//p_out.info_set_int( "bitrate", bitrate / 1000 );
				p_out.info_set( "codec", "DTS" );
				p_out.info_set( "encoding", "lossy" );
				p_out.info_set( "channel_mode", pfc::string_formatter() << dts_mode_list[ dts_flags & DCA_CHANNEL_MASK ] << ( dts_flags & DCA_LFE ? " + LFE" : "" ) );
				return true;
			}
		}
		return false;
	}

	virtual void flush()
	{
		cleanup();
	}

	virtual double get_buffer_ahead()
	{
		return 16384. / 4. / 44100.;
	}
};

class dts_postprocessor_entry : public decode_postprocessor_entry
{
public:
	virtual bool instantiate( const file_info & info, decode_postprocessor_instance::ptr & out )
	{
        if ( info.info_get_decoded_bps() != 16 )
		{
            return false;
        }

		const char * encoding = info.info_get( "encoding" );
		if ( !encoding || pfc::stricmp_ascii( encoding, "lossless" ) )
		{
			return false;
		}

		out = new service_impl_t< dts_postprocessor_instance >;

		return true;
	}
};

static service_factory_single_t< dts_postprocessor_entry > g_dts_postprocessor_entry_factory;
