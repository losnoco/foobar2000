#define MYVERSION "1.0"

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

2010-03-14 13:40 UTC - kode54
- Initial release

*/

#include <foobar2000.h>

#include "hdcd_decode.h"

template <typename T>
class circular_buffer
{
	pfc::array_t<T> buffer;
	unsigned readptr,writeptr,used,size;
public:
	circular_buffer() : readptr(0), writeptr(0), size(0), used(0) { }
	circular_buffer(unsigned p_size) : readptr(0), writeptr(0), size(p_size), used(0) { buffer.set_size(p_size); }
	void set_size(unsigned p_size) { readptr = 0; writeptr = 0; size = p_size; used = 0; buffer.set_size(p_size); }
	unsigned data_available() {return used;}
	unsigned free_space() {return size-used;}
	bool write(const T * src,unsigned count)
	{
		if (count>free_space()) return false;
		while(count)
		{
			unsigned delta = size - writeptr;
			if (delta>count) delta = count;
			memcpy(buffer.get_ptr() + writeptr, src, delta * sizeof(T));
			used += delta;
			writeptr = (writeptr + delta) % size;
			src += delta;
			count -= delta;
		}
		return true;
	}
	unsigned read(T * dst,unsigned count)
	{
		unsigned done = 0;
		for(;;)
		{
			unsigned delta = size - readptr;
			if (delta>used) delta=used;
			if (delta>count) delta=count;
			if (delta==0) break;

			memcpy(dst,buffer.get_ptr() + readptr, delta * sizeof(T));
			dst += delta;
			done += delta;
			readptr = (readptr + delta) % size;
			count -= delta;
			used -= delta;
		}
		return done;
	}
	unsigned remove(unsigned count)
	{
		unsigned done = 0;
		for(;;)
		{
			unsigned delta = size - readptr;
			if (delta>used) delta=used;
			if (delta>count) delta=count;
			if (delta==0) break;

			done += delta;
			readptr = (readptr + delta) % size;
			count -= delta;
			used -= delta;
		}
		return done;
	}
	void reset()
	{
		readptr=writeptr=used=0;
	}
};

class hdcd_dsp : public dsp_impl_base {
	pfc::array_t<hdcd_decode> decoders;

	circular_buffer<audio_sample> unmodified_sample_buffer;
	circular_buffer<audio_sample> sample_buffer;
	pfc::array_t<t_int32> buffer;

	unsigned srate, nch, channel_config;

	bool init()
	{
		decoders.set_size( nch );

		for ( unsigned i = 0; i < nch; i++ )
		{
			decoders[ i ].reset();
			decoders[ i ].set_sample_rate( srate );
		}

		unmodified_sample_buffer.set_size( srate * nch );
		sample_buffer.set_size( srate * nch );

		return true;
	}

	void cleanup()
	{
		decoders.set_size( 0 );
		unmodified_sample_buffer.set_size( 0 );
		sample_buffer.set_size( 0 );
		buffer.set_size( 0 );
		srate = 0;
		nch = 0;
		channel_config = 0;
	}

	void flush_chunk()
	{
		if ( sample_buffer.data_available() )
		{
			unsigned enabled = 0;
			for ( unsigned i = 0; i < nch; i++ )
			{
				enabled |= decoders[ i ].get_sample_counter();
			}

			audio_chunk * chunk = insert_chunk( sample_buffer.data_available() );
			chunk->set_data_size( sample_buffer.data_available() );
			chunk->set_sample_count( sample_buffer.data_available() / nch );
			chunk->set_srate( srate );
			chunk->set_channels( nch, channel_config );

			if ( enabled )
			{
				sample_buffer.read( chunk->get_data(), sample_buffer.data_available() );
				unmodified_sample_buffer.reset();
			}
			else
			{
				unmodified_sample_buffer.read( chunk->get_data(), unmodified_sample_buffer.data_available() );
				sample_buffer.reset();
			}
		}
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
			bool buffered = false;

			if ( sample_buffer.data_available() )
			{
				audio_chunk * temp = insert_chunk( sample_buffer.data_available() );
				temp->set_data_size( sample_buffer.data_available() );
				temp->set_sample_count( sample_buffer.data_available() / nch );
				temp->set_srate( srate );
				temp->set_channels( nch, channel_config );
				sample_buffer.read( temp->get_data(), sample_buffer.data_available() );
				unmodified_sample_buffer.reset();
				buffered = true;
			}

			process_chunk( chunk );

			return true;
		}

		int data = chunk->get_sample_count() * nch;

		if ( unmodified_sample_buffer.free_space() < data )
		{
			int data_to_emit = data - unmodified_sample_buffer.free_space();
			audio_chunk * temp = insert_chunk( data_to_emit );
			temp->set_data_size( data_to_emit );
			temp->set_sample_count( data_to_emit / nch );
			temp->set_srate( srate );
			temp->set_channels( nch, channel_config );
			unmodified_sample_buffer.read( temp->get_data(), data_to_emit );
			sample_buffer.remove( data_to_emit );
		}

		unmodified_sample_buffer.write( chunk->get_data(), data );

		process_chunk( chunk );

		sample_buffer.write( chunk->get_data(), data );

		return false;
	}

	virtual void flush()
	{
		cleanup();
	}

	virtual double get_latency()
	{
		double latency = 0;
		if ( srate && nch && sample_buffer.data_available() )
		{
			latency += (double)(sample_buffer.data_available() / nch) / (double)srate;
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
