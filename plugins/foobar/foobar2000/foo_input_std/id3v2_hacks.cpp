#include "stdafx.h"
#include "id3v2_hacks.h"

__int64 id3v2_calc_size(reader * r)
{
    unsigned char  tmp[10];

    if ( r->read (tmp, sizeof (tmp)) != sizeof (tmp) ) return -1;

    // check id3-tag
    if ( 0 != memcmp ( tmp, "ID3", 3) ) {
        r->seek ( 0 );
        return 0;
    }

    // read flags
    int Unsynchronisation = tmp[5] & 0x80;
    int ExtHeaderPresent  = tmp[5] & 0x40;
    int ExperimentalFlag  = tmp[5] & 0x20;
    int FooterPresent     = tmp[5] & 0x10;

    if ( tmp[5] & 0x0F ) {                              // not (yet???) allowed
        r->seek ( 0 );
        return 0;
    }

    if ( (tmp[6] | tmp[7] | tmp[8] | tmp[9]) & 0x80 ) { // not allowed
        r->seek ( 0 );
        return 0;
    }

    // read HeaderSize (syncsave: 4 * $0xxxxxxx = 28 significant bits)
    unsigned int ret;
    ret  = tmp[6] << 21;
    ret += tmp[7] << 14;
    ret += tmp[8] <<  7;
    ret += tmp[9]      ;
    ret += 10;
    if ( FooterPresent ) ret += 10;

    r->seek ( ret );

    return ret;
}

__int64 id3v2_remove(reader * r)
{
	if (!r->can_seek()) return 0;
	__int64 len = r->get_length();
	if (len<=0) return 0;
	r->seek(0);
	__int64 offset = id3v2_calc_size(r);
	if (offset>0 && offset<len)
	{
		len-=offset;
		reader * temp = len > 16*1024*1024 ? file::g_open_temp() : file::g_open_tempmem();
		if (temp==0) return 0;
		if (reader::transfer(r,temp,len)!=len)
		{
			temp->reader_release();
			return 0;
		}
		r->seek(0);
		r->set_eof();
		temp->seek(0);
		reader::transfer(temp,r,len);
		temp->reader_release();
	}
	return offset;
}