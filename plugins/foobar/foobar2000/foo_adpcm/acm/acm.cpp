#define MY_VERSION "1.0"

/*
	change log

2005-02-06 23:18 UTC - kode54
- Initial release
- Version is now 1.0

*/

#include <foobar2000.h>

#include "acmstrm.h"

class input_acm
{
	service_ptr_t<file>         m_file;
	CACMUnpacker              * m_unpacker;

	mem_block_t<int>            data_buffer;

	int srate, nch, size;

	int pos, swallow;

	/* for reader callback */
	abort_callback            * m_abort;

public:
	input_acm() : m_unpacker(0), m_abort(0) {}

	~input_acm()
	{
		if (m_unpacker) delete m_unpacker;
	}

	static int _stdcall reader_callback(int hFile, unsigned char * buffer, int count)
	{
		return ((input_acm *)hFile)->reader_callback_i(buffer, count);
	}

	int reader_callback_i(unsigned char * buffer, int count)
	{
		return m_file->read_e(buffer, count, *m_abort);
	}

	t_io_result open( service_ptr_t<file> p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort )
	{
		if ( p_reason == input_open_info_write ) return io_result_error_data;

		t_io_result status;

		if ( p_filehint.is_empty() )
		{
			status = filesystem::g_open( m_file, p_path, filesystem::open_mode_read, p_abort );
			if ( io_result_failed( status ) ) return status;
		}
		else m_file = p_filehint;

		return io_result_success;
	}

private:
	t_io_result open_internal( abort_callback & p_abort )
	{
		m_unpacker = new CACMUnpacker;
		if (!m_unpacker) return io_result_error_out_of_memory;

		try
		{
			m_abort = &p_abort;
			if (!m_unpacker->init( reader_callback, (int)this, nch, srate, size) ) return io_result_error_data;
		}
		catch(t_io_result code)
		{
			return code;
		}

		return io_result_success;
	}

public:
	t_io_result get_info( file_info & p_info, abort_callback & p_abort )
	{
		if ( ! m_unpacker )
		{
			t_io_result status = open_internal( p_abort );
			if ( io_result_failed( status ) ) return status;
		}

		p_info.info_set_int( "samplerate", srate );
		p_info.info_set_int( "channels", nch );
		p_info.info_set( "codec", "Interplay ACM" );

		p_info.set_length( double( size / nch ) / double( srate ) );

		return io_result_success;
	}

	t_io_result get_file_stats( t_filestats & p_stats,abort_callback & p_abort )
	{
		return m_file->get_stats( p_stats, p_abort );
	}

	t_io_result decode_initialize( unsigned p_flags,abort_callback & p_abort )
	{
		if ( ! m_unpacker )
		{
			t_io_result status = open_internal( p_abort );
			if ( io_result_failed( status ) ) return status;
		}

		pos = 0;
		swallow = 0;

		return io_result_success;
	}

	t_io_result decode_run( audio_chunk & p_chunk,abort_callback & p_abort )
	{
		if (pos >= size) return io_result_eof;

		int todo = 588 * nch, done;

		if ( ! data_buffer.check_size( todo ) )
			return io_result_error_out_of_memory;

		int * out = data_buffer.get_ptr();

		try
		{
			m_abort = &p_abort;

			do {
				done = m_unpacker->readAndDecompress(out, todo);
				if (!done) break;
				pos += done;

				if (swallow)
				{
					if (swallow > done)
					{
						swallow -= done;
						done = 0;
					}
					else
					{
						done -= swallow;
						swallow = 0;
					}
				}
			} while(!done);
		}
		catch(t_io_result code)
		{
			return code;
		}

		if (done)
		{
			if ( ! p_chunk.check_data_size( done ) )
				return io_result_error_out_of_memory;

			audio_sample * final = p_chunk.get_data();
			for (int i = 0; i < done; i++)
			{
				*final++ = audio_sample(*out++) * (1. / 32768.);
			}
			p_chunk.set_srate(srate);
			p_chunk.set_channels(nch);
			p_chunk.set_sample_count(done / nch);
			return io_result_success;
		}

		return io_result_eof;
	}

	t_io_result decode_seek( double p_seconds,abort_callback & p_abort )
	{
		swallow = int( p_seconds * double( srate ) + .5 ) * nch;
		if ( swallow > pos )
		{
			swallow -= pos;
			return io_result_success;
		}
		pos = 0;
		delete m_unpacker;

		t_io_result status = m_file->seek( 0, p_abort );
		if ( io_result_failed( status ) ) return status;

		m_unpacker = new CACMUnpacker;
		if ( ! m_unpacker ) return io_result_error_out_of_memory;

		try
		{
			m_abort = &p_abort;
			if ( ! m_unpacker->init( reader_callback, (int) this, nch, srate, size ) ) return io_result_error_data;
		}
		catch(t_io_result code)
		{
			return code;
		}

		return io_result_success;
	}

	bool decode_can_seek()
	{
		return true;
	}

	bool decode_get_dynamic_info( file_info & p_out, double & p_timestamp_delta,bool & p_track_change )
	{
		return false;
	}

	void decode_on_idle( abort_callback & p_abort )
	{
	}

	t_io_result retag( const file_info & p_info,abort_callback & p_abort )
	{
		return io_result_error_data;
	}

	static bool g_is_our_content_type( const char * p_content_type )
	{
		return false;
	}

	static bool g_is_our_path( const char * p_full_path, const char * p_extension )
	{
		return ! stricmp( p_extension, "acm" );
	}
};

DECLARE_FILE_TYPE("Interplay ACM files", "*.ACM");

static input_singletrack_factory_t<input_acm> g_input_acm_factory;

DECLARE_COMPONENT_VERSION("Interplay ACM decoder", MY_VERSION, "Plays Interplay ACM files from such titles as Fallout, Baldur's Gate, and MDK 2.");
