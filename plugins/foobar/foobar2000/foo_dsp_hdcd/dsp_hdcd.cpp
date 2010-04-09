#define MYVERSION "1.3"

/*
   Copyright (C) 2010, Chris Moeller,
   All rights reserved.                          

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote 
        products derived from this software without specific prior written 
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*

	change log

2010-04-09 09:44 UTC - kode54
- Changed DSP to buffer whole audio_chunks, at least one second or one chunk worth
- Version is now 1.3

2010-03-14 14:50 UTC - kode54
- Changed the HDCD decoder peak extension condition to a single if statement
- Version is now 1.2

2010-03-14 14:16 UTC - kode54
- Cleaned up a few things.
- Added HDCD detection indicator to console
- Version is now 1.1

2010-03-14 13:40 UTC - kode54
- Initial release

*/

#include <foobar2000.h>

#include "hdcd_decode.h"

class hdcd_dsp : public dsp_impl_base {
	pfc::array_t<hdcd_decode> decoders;

	dsp_chunk_list_impl original_chunks;
	dsp_chunk_list_impl output_chunks;
	pfc::array_t<t_int32> buffer;

	unsigned srate, nch, channel_config;

	bool info_emitted;

	bool init()
	{
		decoders.set_size( nch );

		for ( unsigned i = 0; i < nch; i++ )
		{
			decoders[ i ].reset();
			decoders[ i ].set_sample_rate( srate );
		}

		return true;
	}

	void cleanup()
	{
		decoders.set_size( 0 );
		original_chunks.remove_all();
		output_chunks.remove_all();
		buffer.set_size( 0 );
		srate = 0;
		nch = 0;
		channel_config = 0;
		info_emitted = false;
	}

	void flush_chunk()
	{
		if ( output_chunks.get_count() )
		{
			unsigned enabled = 0;
			for ( unsigned i = 0; i < nch; i++ )
			{
				enabled |= decoders[ i ].get_sample_counter();
			}

			dsp_chunk_list * list = enabled ? &output_chunks : &original_chunks;

			for ( unsigned i = 0; i < list->get_count(); i++ )
			{
				audio_chunk * in_chunk = list->get_item( i );
				audio_chunk * out_chunk = insert_chunk( in_chunk->get_data_length() );
				out_chunk->copy( *in_chunk );
			}
		}
		cleanup();
	}

	void process_chunk( audio_chunk * chunk )
	{
		int data = chunk->get_sample_count() * nch;
		buffer.grow_size( data );
		audio_math::convert_to_int32( chunk->get_data(), data, buffer.get_ptr(), 1. / 65536. );

		for ( unsigned i = 0; i < nch; i++ )
		{
			for ( unsigned j = 0, k = chunk->get_sample_count(); j < k; j++ )
			{
				t_int32 * ptr = buffer.get_ptr() + j * nch + i;
				*ptr = decoders[ i ].decode_sample( *ptr );
			}
		}

		audio_math::convert_from_int32( buffer.get_ptr(), data, chunk->get_data(), 1 << ( 32 - 20 ) );
	}

public:
	hdcd_dsp()
	{
		cleanup();
	}

	~hdcd_dsp()
	{
		cleanup();
	}

	static GUID g_get_guid()
	{
		static const GUID guid = { 0xaef54f, 0x54f6, 0x4f97, { 0x83, 0x33, 0x67, 0x40, 0x94, 0x9d, 0x38, 0xbf } };
		return guid;
	}

	static void g_get_name(pfc::string_base &p_out)
	{
		p_out = "HDCD decoder";
	}

	virtual void on_endoftrack(abort_callback &p_abort)
	{
		flush_chunk();
	}

	virtual void on_endofplayback(abort_callback &p_abort)
	{
		flush_chunk();
	}

	virtual bool on_chunk(audio_chunk *chunk, abort_callback &p_abort)
	{
        metadb_handle_ptr fh;
        if ( !get_cur_file( fh ) )
		{
			flush_chunk();
            return true;
        }

        file_info_impl i;
        if ( !fh->get_info_async( i ) )
		{
			flush_chunk();
            return true;
        }

        if ( i.info_get_decoded_bps() != 16 )
		{
			flush_chunk();
            return true;
        }

		const char * encoding = i.info_get( "encoding" );
		if ( !encoding || pfc::stricmp_ascii( encoding, "lossless" ) )
		{
			flush_chunk();
			return true;
		}

		if ( srate != chunk->get_sample_rate() || nch != chunk->get_channels() || channel_config != chunk->get_channel_config() )
		{
			flush_chunk();
			srate = chunk->get_sample_rate();
			nch = chunk->get_channels();
			channel_config = chunk->get_channel_config();
			if (!init())
			{
				flush();
				return true;
			}
		}

		unsigned enabled = 0;
		for ( unsigned i = 0; i < nch; i++ )
		{
			enabled |= decoders[ i ].get_sample_counter();
		}

		if ( enabled )
		{
			if ( !info_emitted )
			{
				console::print( "HDCD detected." );
				info_emitted = true;
			}

			if ( output_chunks.get_count() )
			{
				for ( unsigned i = 0; i < output_chunks.get_count(); i++ )
				{
					audio_chunk * in_chunk = output_chunks.get_item( i );
					audio_chunk * out_chunk = insert_chunk( in_chunk->get_data_length() );
					out_chunk->copy( *in_chunk );
				}
				original_chunks.remove_all();
				output_chunks.remove_all();
			}

			process_chunk( chunk );

			return true;
		}

		original_chunks.add_chunk( chunk );

		process_chunk( chunk );

		output_chunks.add_chunk( chunk );

		while ( original_chunks.get_duration() >= 1.0 && original_chunks.get_count() > 1 )
		{
			audio_chunk * in_chunk = original_chunks.get_item( 0 );
			audio_chunk * out_chunk = insert_chunk( in_chunk->get_data_length() );
			out_chunk->copy( *in_chunk );
			original_chunks.remove_by_idx( 0 );
			output_chunks.remove_by_idx( 0 );
		}

		return false;
	}

	virtual void flush()
	{
		cleanup();
	}

	virtual double get_latency()
	{
		double latency = 0;
		if ( original_chunks.get_count() )
		{
			latency += original_chunks.get_duration();
		}
		return latency;
	}

	virtual bool need_track_change_mark()
	{
		return false;
	}
};

static dsp_factory_nopreset_t   <hdcd_dsp>     g_hdcd_dsp_factory;

static const char about_string[] = "HDCD is a registered trademark of Microsoft Corporation.";

DECLARE_COMPONENT_VERSION("HDCD decoder", MYVERSION, about_string);

VALIDATE_COMPONENT_FILENAME("foo_dsp_hdcd.dll");
