#ifndef FILEPROVIDER_H
#define FILEPROVIDER_H

#include <foobar2000.h>
#include <string>
#include <binio.h>
#include <fprovide.h>

class CProvider_foobar2000: public CFileProvider
{
	abort_callback    & m_abort;

	// file hint
	service_ptr_t<file> m_file_hint;
	std::string         m_file_path;

public:
	virtual binistream *open(std::string filename) const;
	virtual void close(binistream *f) const;

	CProvider_foobar2000(abort_callback & p_abort)
		: m_abort( p_abort ) {}

	CProvider_foobar2000(std::string filename, service_ptr_t<file> & file, abort_callback & p_abort)
		: m_abort( p_abort ), m_file_path( filename ), m_file_hint( file ) {}
};

#endif