#include "fileprovider.h"

class binistream_foobar2000 : public binistream
{
	service_ptr_t<file> m_file;
	abort_callback    & m_abort;

	Byte                m_buffer[ 4096 ];
	int                 m_buffer_filled, m_buffer_position;

public:
	binistream_foobar2000( service_ptr_t<file> & p_file, abort_callback & p_abort )
		: m_file( p_file ), m_abort( p_abort ), m_buffer_filled( 0 ), m_buffer_position( 0 ) {}

	void seek(long pos, Offset offs)
	{
		try
		{
			switch(offs)
			{
			case Set: m_file->seek( pos, m_abort ); break;
			case Add:
				if ( ( pos < 0 && m_buffer_position + pos >= 0 ) ||
					( pos >= 0 && m_buffer_position + pos < m_buffer_filled + m_buffer_position ) )
				{
					m_buffer_filled -= pos;
					m_buffer_position += pos;
					err &= ~Eof;
					return;
				}
				else
					m_file->seek_ex( pos - m_buffer_filled, file::seek_from_current, m_abort );
				break;
			case End: m_file->seek_ex( pos, file::seek_from_eof, m_abort ); break;
			}
			err &= ~Eof;
		}
		catch( exception_io_seek_out_of_range & )
		{
			err |= Eof;
			m_file->seek_ex( 0, file::seek_from_eof, m_abort );
		}

		m_buffer_filled = 0;
		m_buffer_position = 0;
	}

	long pos()
	{
		t_filesize pos = m_file->get_position( m_abort ) - m_buffer_filled;
		if (pos > INT_MAX) pos = INT_MAX;
		return pos;
	}

	Byte getByte()
	{
		if ( err & Eof ) return 0;

		if ( ! m_buffer_filled )
		{
			m_buffer_filled = m_file->read( m_buffer, 4096, m_abort );
			if ( ! m_buffer_filled )
			{
				err |= Eof;
				return -1;
			}
			m_buffer_position = 0;
		}

		Byte value = m_buffer[ m_buffer_position ];
		++m_buffer_position;
		--m_buffer_filled;
		return value;
	}
};

binistream * CProvider_foobar2000::open(std::string filename) const
{
	service_ptr_t<file> p_file;

	if ( m_file_hint.is_valid() )
	{
		if ( !strcmp( filename.c_str(), m_file_path.c_str() ) )
		{
			p_file = m_file_hint;
			p_file->seek( 0, m_abort );
		}
	}

	if ( p_file.is_empty() )
		filesystem::g_open( p_file, filename.c_str(), filesystem::open_mode_read, m_abort );

	binistream_foobar2000 * f = new binistream_foobar2000( p_file, m_abort );
	if ( f->error() ) { delete f; return 0; }

	// Open all files as little endian with IEEE floats by default
	f->setFlag(binio::BigEndian, false); f->setFlag(binio::FloatIEEE);

	return f;
}

void CProvider_foobar2000::close(binistream *f) const
{
	binistream_foobar2000 * ff = ( binistream_foobar2000 * ) f;
	if ( f )
		delete ff;
}