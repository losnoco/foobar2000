#ifndef __FOOFILE_H__
#define __FOOFILE_H__

#include <foobar2000.h>

class foofile
{
	static const int buffer_size = 8192;

protected:
	service_ptr_t<file>        r;
	pfc::array_t<t_uint8>      buffer;
	unsigned                   bufpos;
	unsigned                   bufend;

	bool                       eof;
	t_filesize                 abspos;

public:
	foofile(const service_ptr_t<file> & r);

	bool get_eof(abort_callback &);

	char preview(abort_callback &);
	char get(abort_callback &);

	void seek(t_filesize, abort_callback &);

protected:
	void requirebuf();
	void bufclear();
	void bufvalidate(abort_callback &);
};

#endif
