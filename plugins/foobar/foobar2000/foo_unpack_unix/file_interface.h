#ifndef FILE_INTERFACE_H
#define FILE_INTERFACE_H

#include <foobar2000.h>
#include <fex/Data_Reader.h>

class foobar_File_Reader : public File_Reader
{
	const service_ptr_t<file> & m_file;
	abort_callback            & m_abort;

public:
	foobar_File_Reader( const service_ptr_t<file> & p_file, abort_callback & p_abort );
	virtual ~foobar_File_Reader() { }
	
	virtual blargg_err_t read_v( void*, int n );
	virtual blargg_err_t skip_v( int n );
	virtual blargg_err_t seek_v( BOOST::uint64_t n );
};

#endif