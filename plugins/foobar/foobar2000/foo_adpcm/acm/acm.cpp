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

	pfc::array_t<int>           data_buffer;

	int srate, nch, size;

	int pos, swallow;

	/* for reader callback */
	abort_callback            * m_abort;

	static int _stdcall reader_callback(void * hFile, unsigned char * buffer, int count)
	{
		return ((input_acm *)hFile)->reader_callback_i(buffer, count);
	}

	int reader_callback_i(unsigned char * buffer, int count)
	{
		return m_file->read(buffer, count, *m_abort);
	}

public:
	input_acm() : m_unpacker(0), m_abort(0) {}

	~input_acm()
	{
		if (m_unpacker) delete m_unpacker;
	}

	void open( service_ptr_t<file> p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort )
	{
		if ( p_reason == input_open_info_write ) throw exception_io_data();

		if ( p_filehint.is_empty() )
		{
			filesystem::g_open( m_file, p_path, filesystem::open_mode_read, p_abort );
		}
		else m_file = p_filehint;
	}

private:
	void open_internal( abort_callback & p_abort )
	{
		m_unpacker = new CACMUnpacker;

		m_abort = &p_abort;
		if ( ! m_unpacker->init( reader_callback, this, nch, srate, size) ) throw exception_io_data();
	}

public:
	void get_info( file_info & p_info, abort_callback & p_abort )
	{
		if ( ! m_unpacker ) open_internal( p_abort );

		p_info.info_set_int( "samplerate", srate );
		p_info.info_set_int( "channels", nch );
		p_info.info_set( "codec", "Interplay ACM" );

		p_info.set_length( double( size / nch ) / double( srate ) );
	}

	t_filestats get_file_stats( abort_callback & p_abort )
	{
		return m_file->get_stats( p_abort );
	}

	void decode_initialize( unsigned p_flags, abort_callback & p_abort )
	{
		if ( ! m_unpacker ) open_internal( p_abort );

		pos = 0;
		swallow = 0;
	}

	bool decode_run( audio_chunk & p_chunk, abort_callback & p_abort )
	{
		if (pos >= size) return false;

		int todo = 588 * nch, done;

		data_buffer.grow_size( todo );

		int * out = data_buffer.get_ptr();

		//try
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

		if (done)
		{
			p_chunk.check_data_size( done );
			audio_math::convert_from_int32( out, done, p_chunk.get_data(), 1 << 16 );
			p_chunk.set_srate(srate);
			p_chunk.set_channels(nch);
			p_chunk.set_sample_count(done / nch);
			return true;
		}

		return false;
	}

	void decode_seek( double p_seconds, abort_callback & p_abort )
	{
		swallow = int( p_seconds * double( srate ) + .5 ) * nch;
		if ( swallow > pos )
		{
			swallow -= pos;
			return;
		}

		pos = 0;
		delete m_unpacker;

		m_file->seek( 0, p_abort );

		m_unpacker = new CACMUnpacker;

		m_abort = &p_abort;
		if ( ! m_unpacker->init( reader_callback, this, nch, srate, size ) ) throw exception_io_data();
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
	}

	void retag( const file_info & p_info, abort_callback & p_abort )
	{
		throw exception_io_data();
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
