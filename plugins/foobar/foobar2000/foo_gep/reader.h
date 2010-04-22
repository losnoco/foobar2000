#include <foobar2000.h>
#include <gme/Music_Emu.h>

class foobar_Data_Reader : public Data_Reader
{
	const service_ptr_t<file> & m_file;
	abort_callback            & m_abort;

public:
	foobar_Data_Reader( const service_ptr_t<file> & p_file, abort_callback & p_abort );

	blargg_err_t read_v( void *, int );
	blargg_err_t skip_v( int n );
};

class foobar_File_Reader : public File_Reader
{
	const service_ptr_t<file> & m_file;
	abort_callback            & m_abort;

public:
	foobar_File_Reader( const service_ptr_t<file> & p_file, abort_callback & p_abort );

	blargg_err_t read_v( void *, int );
	blargg_err_t skip_v( int n );
	blargg_err_t seek_v( BOOST::uint64_t n );
};