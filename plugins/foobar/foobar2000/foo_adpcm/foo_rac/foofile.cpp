#include "foofile.h"

const char eofchar = 0;

foofile::foofile(const service_ptr_t<file> & in)
	: r(in), bufpos(0), bufend(0), eof(false), abspos(0)  {}

void foofile::requirebuf()
{
	buffer.grow_size( buffer_size );
}

void foofile::bufclear()
{
	bufpos = 0;
	bufend = 0;
}

void foofile::bufvalidate( abort_callback & p_abort )
{
	requirebuf();
	bufclear();
	bufend = r->read( buffer.get_ptr(), buffer.get_size(), p_abort );
	abspos += t_int64( bufend );
	if ( bufend == 0 ) eof = true;
}

void foofile::seek( t_filesize newpos, abort_callback & p_abort )
{
    if ( buffer.get_size() != 0 )
    {
        t_filesize newbufpos = newpos - ( abspos - bufend );
        if ( newbufpos >= 0 && newbufpos <= t_filesize( bufend ) ) 
        {
            bufpos = unsigned( newbufpos );
            eof = false;
            return;
        }
    }

	bufclear();
	eof = false;
	abspos = newpos;
    r->seek( newpos, p_abort );
}

bool foofile::get_eof( abort_callback & p_abort )
{
    if ( !eof && buffer.get_size() != 0 && bufpos >= bufend )
        bufvalidate( p_abort );
    return eof;
}

char foofile::preview( abort_callback & p_abort )
{
    if ( !eof && bufpos >= bufend )
        bufvalidate( p_abort );
    if ( eof )
        return eofchar;
    return buffer[ bufpos ];
}

char foofile::get( abort_callback & p_abort )
{
    char ret = preview(p_abort);
    if (!eof)
        bufpos++;
    return ret;
}
