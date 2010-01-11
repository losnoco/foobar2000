#ifndef FILE_INTERFACE_H
#define FILE_INTERFACE_H

#include <foobar2000.h>
#include <fex/Data_Reader.h>
#include <fex/abstract_file.h>

class foobar_File_Reader : public File_Reader
{
	const service_ptr_t<file> & m_file;
	abort_callback            & m_abort;

public:
	foobar_File_Reader( const service_ptr_t<file> & p_file, abort_callback & p_abort ) : m_file( p_file ), m_abort( p_abort ) { }
	virtual ~foobar_File_Reader() { }
	
	// Read at most 'n' bytes. Return number of bytes read, zero or negative
	// if error.
	virtual long read_avail( void* p, long n );
	virtual uint64_t remain() const;
	virtual uint64_t size() const;
	virtual uint64_t tell() const;
	virtual error_t seek( uint64_t offset );
};

class foobar_Data_Writer : public Data_Writer
{
	service_ptr_t<file> & m_file;
	abort_callback      & m_abort;
public:
	foobar_Data_Writer( service_ptr_t<file> & p_file, abort_callback & p_abort ) : m_file( p_file ), m_abort( p_abort ) { }
	virtual ~foobar_Data_Writer() { }
	
	virtual error_t write( const void* p, long n );
};

#endif