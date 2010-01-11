#define MYVERSION "2.0.7"

/*
	changelog

2009-08-02 00:46 UTC - kode54
- Reimplemented tag reader to not use sscanf
- Version is now 2.0.7

2007-07-24 23:33 UTC - kode54
- Implemented UTF-8 tag support
- Version is now 2.0.6

2005-04-01 05:05 UTC - kode54
- Corrected ReplayGain tagging support for 0.9
- Version is now 2.0.5

2004-04-26 07:47 UTC - kode54
- Changed the no_loop behavior so that the configuration variable does not affect running instances
- Changed finite rendering to match the wanted length instead of outputting a multiple of 576 samples
- Version is now 2.0.4.1

2004-03-08 16:29 UTC - kode54
- Fixed NTSC/PAL header detection, now pulls region string from the first executable that it will
  also be importing code/stack pointer from
- Version is now 2.0.4

2004-02-23 17:05 UTC - kode54
- Now reads "year" as "date", which will be written as "year" unless it contains more than one numeral
- Version is now 2.0.3

2004-02-02 22:37 UTC - kode54
- Fixed context_get_display function declaration in context_fest
- Version is now 2.0.2

2004-01-27 03:50 UTC - kode54
- Changed _libN handling, previous method was broken anyway
- Version is now 2.0.1

2004-01-23 23:29 UTC - kode54
- Updated to PSXCore0008
- Added (dubious?) support for _refresh tags and executable header detection
- Version is now 2.0k

- Updated to PSXCore0007b, finally

2004-01-09 20:30 UTC - kode54
- Added input_file_type service for 0.7.7a

2004-01-03 15:39 UTC - kode54
- Fixed default length for untagged files
- Version is now 2.0j

2003-11-25 15:52 UTC - kode54
- Quick bugfix to length editor
- Version is now 2.0i

2003-10-19 03:12 UTC - kode54
- Changed length editor to use query_info_locked to retrieve the existing length from a single track

2003-10-06 09:57 UTC - kode54
- Fixed fade length code to use field_fade instead of "fade" as the info name
- Version is now 2.0h

2003-09-28 15:18 UTC - kode54
- Fixed length editor and removed circular database update crap
- Changed internal length/fade field names
- Version is now 2.0g

2003-08-07 00:54 - kode54
- Buoy... fixed default length config
- Version is now 2.0ffs

2003-08-01 22:04 - kode54
- Updated to 0.7 beta 29 SDK
- STUPID! Forgot to initialize tag_fade_ms to 0 in load_exe_recursive,
  causing random length display for files that have length but no fade
- Version is now 2.0f

2003-07-05 11:35 - kode54
- Updated to 0.7 beta 10 SDK

2003-06-28 15:14 - kode54
- Removed references to cfg_window_remember
- Version is now 2.0e

2003-06-27 04:58 - kode54
- Replaced LoadBitmap with uLoadImage

2003-06-26 06:59 - kode54
- Updated to 0.7 API, except for LoadBitmap call
- Version is now 2.0d

2003-06-06 05:33 - kode54
- Damnit, forgot checkbox message handler for IDC_WINDOW_REMEMBER
- Version is now 2.0c

2003-06-05 23:14 - kode54
- Oops, fixed header detection in set_info() for PSF2
- Version is now 2.0b

2003-06-01 01:45 - kode54
- ARGH! Forgot set_length for PSF2
- Version is now 2.0a

2003-05-31 23:47 - kode54
- Woohoo, PSF2!
- Added crazy static counters for config logo cycler
- Added debug info output option
- Version is now 2.0

2003-05-23 17:41 - kode54
- Updated to emu0006, which provides the following speedups:
  11% for playback, 26% for seeking
- Added frequency response filter class
- Version is now 1.10R

2003-05-22 02:10 - kode54
- ARGH! All this time and I forgot hint()
- Version is now 1.09r1

2003-05-21 17:42 - kode54
- Updated to emu0005 and the new API
- Changed extrainfo to codec
- Relabeled comment by invalid t_size work-around,
  looks like CaitSith2 was to blame all along
- Version is now 1.09

2003-04-09 16:27 - kode54
- Added bitspersample info
- File name is no longer hard coded
- Version is now 1.08r2

*/

#include "../SDK/foobar2000.h"
#include "../helpers/window_placement_helper.h"

#include "resource.h"

#include <stdio.h>
#include <zlib.h>

#include "../../../ESP/Sega/Core/sega.h"
#include "../../../ESP/Sega/Core/dcsound.h"
#include "../../../ESP/Sega/Core/satsound.h"
#include "../../../ESP/Sega/Core/yam.h"

#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>
#include <atlctrls.h>
#include <atlctrlx.h>

//#define DBG(a) OutputDebugString(a)
#define DBG(a)

typedef unsigned long u_long;

critical_section g_sync;
static int initialized = 0;
volatile long ssf_count = 0, dsf_count = 0;

// {091E116D-1353-4bad-A259-20FE14AB9943}
static const GUID guid_cfg_infinite = 
{ 0x91e116d, 0x1353, 0x4bad, { 0xa2, 0x59, 0x20, 0xfe, 0x14, 0xab, 0x99, 0x43 } };
// {888B00E1-1D86-4521-A29D-02781CB56FC4}
static const GUID guid_cfg_deflength = 
{ 0x888b00e1, 0x1d86, 0x4521, { 0xa2, 0x9d, 0x2, 0x78, 0x1c, 0xb5, 0x6f, 0xc4 } };
// {CE8A16F7-8136-43b7-821B-44DEC320B274}
static const GUID guid_cfg_deffade = 
{ 0xce8a16f7, 0x8136, 0x43b7, { 0x82, 0x1b, 0x44, 0xde, 0xc3, 0x20, 0xb2, 0x74 } };
// {0AC510F0-8F15-4a92-9B20-1E66A16AB9A2}
static const GUID guid_cfg_suppressopeningsilence = 
{ 0xac510f0, 0x8f15, 0x4a92, { 0x9b, 0x20, 0x1e, 0x66, 0xa1, 0x6a, 0xb9, 0xa2 } };
// {BC554EE8-1F25-427d-815C-FAE5F2629977}
static const GUID guid_cfg_suppressendsilence = 
{ 0xbc554ee8, 0x1f25, 0x427d, { 0x81, 0x5c, 0xfa, 0xe5, 0xf2, 0x62, 0x99, 0x77 } };
// {B783A8A0-6F95-4a68-84CB-5421C05B6562}
static const GUID guid_cfg_endsilenceseconds = 
{ 0xb783a8a0, 0x6f95, 0x4a68, { 0x84, 0xcb, 0x54, 0x21, 0xc0, 0x5b, 0x65, 0x62 } };
// {2EBC2AD6-93D3-4e96-90C9-98529C37C10E}
static const GUID guid_cfg_dry = 
{ 0x2ebc2ad6, 0x93d3, 0x4e96, { 0x90, 0xc9, 0x98, 0x52, 0x9c, 0x37, 0xc1, 0xe } };
// {20A292DD-97BA-48bf-8A7C-36B002CEA7C0}
static const GUID guid_cfg_dsp = 
{ 0x20a292dd, 0x97ba, 0x48bf, { 0x8a, 0x7c, 0x36, 0xb0, 0x2, 0xce, 0xa7, 0xc0 } };
// {76B919C4-287E-4dc7-8962-819DA40BF4CF}
static const GUID guid_cfg_dsp_dynarec = 
{ 0x76b919c4, 0x287e, 0x4dc7, { 0x89, 0x62, 0x81, 0x9d, 0xa4, 0xb, 0xf4, 0xcf } };
// {2B836D9B-D6C7-49f6-ADF6-208C4E5E213C}
static const GUID guid_cfg_placement = 
{ 0x2b836d9b, 0xd6c7, 0x49f6, { 0xad, 0xf6, 0x20, 0x8c, 0x4e, 0x5e, 0x21, 0x3c } };

static cfg_int cfg_infinite(guid_cfg_infinite,0);
static cfg_int cfg_deflength(guid_cfg_deflength,170000);
static cfg_int cfg_deffade(guid_cfg_deffade,10000);
static cfg_int cfg_suppressopeningsilence(guid_cfg_suppressopeningsilence,1);
static cfg_int cfg_suppressendsilence(guid_cfg_suppressendsilence,1);
static cfg_int cfg_endsilenceseconds(guid_cfg_endsilenceseconds,5);
static cfg_int cfg_dry(guid_cfg_dry,1);
static cfg_int cfg_dsp(guid_cfg_dsp,1);
static cfg_int cfg_dsp_dynarec(guid_cfg_dsp_dynarec,1);
static cfg_window_placement cfg_placement(guid_cfg_placement);

static const char field_length[]="xsf_length";
static const char field_fade[]="xsf_fade";

#define BORK_TIME 0xC0CAC01A

static unsigned long parse_time_crap(const char *input)
{
	if (!input) return BORK_TIME;
	int len = strlen(input);
	if (!len) return BORK_TIME;
	int value = 0;
	{
		int i;
		for (i = len - 1; i >= 0; i--)
		{
			if ((input[i] < '0' || input[i] > '9') && input[i] != ':' && input[i] != ',' && input[i] != '.')
			{
				return BORK_TIME;
			}
		}
	}
	pfc::string8 foo = input;
	char *bar = (char *) foo.get_ptr();
	char *strs = bar + foo.length() - 1;
	while (strs > bar && (*strs >= '0' && *strs <= '9'))
	{
		strs--;
	}
	if (*strs == '.' || *strs == ',')
	{
		// fraction of a second
		strs++;
		if (strlen(strs) > 3) strs[3] = 0;
		value = atoi(strs);
		switch (strlen(strs))
		{
		case 1:
			value *= 100;
			break;
		case 2:
			value *= 10;
			break;
		}
		strs--;
		*strs = 0;
		strs--;
	}
	while (strs > bar && (*strs >= '0' && *strs <= '9'))
	{
		strs--;
	}
	// seconds
	if (*strs < '0' || *strs > '9') strs++;
	value += atoi(strs) * 1000;
	if (strs > bar)
	{
		strs--;
		*strs = 0;
		strs--;
		while (strs > bar && (*strs >= '0' && *strs <= '9'))
		{
			strs--;
		}
		if (*strs < '0' || *strs > '9') strs++;
		value += atoi(strs) * 60000;
		if (strs > bar)
		{
			strs--;
			*strs = 0;
			strs--;
			while (strs > bar && (*strs >= '0' && *strs <= '9'))
			{
				strs--;
			}
			value += atoi(strs) * 3600000;
		}
	}
	return value;
}

static void print_time_crap(int ms, char *out)
{
	char frac[8];
	int i,h,m,s;
	if (ms % 1000)
	{
		sprintf(frac, ".%3.3d", ms % 1000);
		for (i = 3; i > 0; i--)
			if (frac[i] == '0') frac[i] = 0;
		if (!frac[1]) frac[0] = 0;
	}
	else
		frac[0] = 0;
	h = ms / (60*60*1000);
	m = (ms % (60*60*1000)) / (60*1000);
	s = (ms % (60*1000)) / 1000;
	if (h) sprintf(out, "%d:%2.2d:%2.2d%s",h,m,s,frac);
	else if (m) sprintf(out, "%d:%2.2d%s",m,s,frac);
	else sprintf(out, "%d%s",s,frac);
}

static void info_meta_add(file_info & info, const char * tag, const char * value)
{
	if (info.meta_get_count_by_name(tag))
	{
		// append as another line
		pfc::string8 final = info.meta_get(tag, 0);
		final += "\r\n";
		final += value;
		info.meta_set(tag, final);
	}
	else
	{
		info.meta_add(tag, value);
	}
}

static void info_meta_ansi( file_info & info )
{
	for ( unsigned i = 0, j = info.meta_get_count(); i < j; i++ )
	{
		for ( unsigned k = 0, l = info.meta_enum_value_count( i ); k < l; k++ )
		{
			const char * value = info.meta_enum_value( i, k );
			info.meta_modify_value( i, k, pfc::stringcvt::string_utf8_from_ansi( value ) );
		}
	}
	for ( unsigned i = 0, j = info.info_get_count(); i < j; i++ )
	{
		const char * name = info.info_enum_name( i );
		if ( name[ 0 ] == '_' )
			info.info_set( pfc::string8( name ), pfc::stringcvt::string_utf8_from_ansi( info.info_enum_value( i ) ) );
	}
}

static int find_crlf(pfc::string8 & blah)
{
	int pos = blah.find_first('\r');
	if (pos >= 0 && *(blah.get_ptr()+pos+1) == '\n') return pos;
	return -1;
}

static void info_meta_write(pfc::string_base & tag, const file_info & info, const char * name, int idx, int & first)
{
	pfc::string8 v = info.meta_enum_value(idx, 0);
	int pos = find_crlf(v);

	if (pos == -1)
	{
		if (first) first = 0;
		else tag.add_byte('\n');
		tag += name;
		tag.add_byte('=');
		// r->write(v.c_str(), v.length());
		tag += v;
		return;
	}
	while (pos != -1)
	{
		pfc::string8 foo;
		foo = v;
		foo.truncate(pos);
		if (first) first = 0;
		else tag.add_byte('\n');
		tag += name;
		tag.add_byte('=');
		tag += foo;
		v = v.get_ptr() + pos + 2;
		pos = find_crlf(v);
	}
	if (v.length())
	{
		tag.add_byte('\n');
		tag += name;
		tag.add_byte('=');
		tag += v;
	}
}

#if 0
static bool info_read_line( const BYTE * ptr, int len, pfc::string_base & tag, pfc::string_base & value )
{
	int p = 0;
	for ( ;; ++p )
	{
		if ( p >= len ) break;
		unsigned u = ptr[ p ];
		if ( ! u ) return false;
		if ( u != 0x0A ) continue;
		break;
	}
	len = p;
	p = 0;
	for ( ;; ++p )
	{
		if ( p >= len ) return false;
		unsigned u = ptr[ p ];
		if ( u == '=' ) break;
		continue;
	}
	int equals_position = p;
	p = 0;
	for ( ;; ++p )
	{
		if ( p >= len ) return false;
		unsigned u = ptr[ p ];
		if ( u <= 0x20 ) continue;
		break;
	}
	if ( p == equals_position ) return false; // no name or pure whitespace in name field

	// okay, we have the tag name
	int tag_start = p;

	p = equals_position - 1;
	for ( ;; --p )
	{
		if ( p < tag_start ) return false;
		unsigned u = ptr[ p ];
		if ( u <= 0x20 ) continue;
		break;
	}
	tag.set_string( ( const char * ) ptr + tag_start, p - tag_start + 1 );

	p = equals_position + 1;

	for ( ;; ++p )
	{
		if ( p >= len ) return false;
		unsigned u = ptr[ p ];
		if ( u <= 0x20 ) continue;
		break;
	}

	tag_start = p;
	p = len;

	for ( ;; --p )
	{
		if ( p < tag_start ) return false;
		unsigned u = ptr[ p ];
		if ( u <= 0x20 ) continue;
		break;
	}
	value.set_string( ( const char * ) ptr + tag_start, p - tag_start + 1 );

	return true;
}
#endif

static void trim_whitespace( pfc::string_base & val )
{
	const char * start = val.get_ptr();
	const char * end = start + strlen( start ) - 1;
	while ( *start && *start < 0x20 ) ++start;
	while ( end >= start && *end < 0x20 ) --end;
	memcpy( (void *) val.get_ptr(), start, end - start + 2 );
	val.truncate( end - start + 1 );
}

static int info_read(const BYTE * ptr, int len, file_info & info, int inherit, int & tag_song_ms, int & tag_fade_ms)
{
	int pos, precede = 0, utf8 = 0;
	pfc::string8 whole_tag( (const char *)ptr, len );
	tag_song_ms = 0;
	tag_fade_ms = 0;
	if (!memcmp(whole_tag, "[TAG]", 5))
	{
		DBG("found tag block");
		pfc::string8_fastalloc tag, value;

		for (pos = 5; pos < len; pos++)
		{
			DBG("scanning for name/value");

			t_size line_end = whole_tag.find_first( '\n', pos );
			if ( line_end == infinite ) line_end = len;
			tag.set_string( whole_tag.get_ptr() + pos, line_end - pos );
			pos = line_end;
			line_end = tag.find_first( '=' );
			if ( line_end == infinite ) continue;
			value.set_string( tag.get_ptr() + line_end + 1 );
			tag.truncate( line_end );
			trim_whitespace( tag );
			trim_whitespace( value );
			
			if (inherit < 0)
			{
				// only parse metadata for top level executable
				if (!stricmp_utf8(tag, "game"))
				{
					DBG("reading game as album");
					tag = "album";
				}
				else if (!stricmp_utf8(tag, "year"))
				{
					DBG("reading year as date");
					tag = "date";
				}

				if (!stricmp_utf8_partial(tag, "replaygain_"))
				{
					DBG("reading RG info");
					//info.info_set(tag, value);
					info.info_set_replaygain(tag, value);
				}
				else if (!stricmp_utf8(tag, "length"))
				{
					DBG("reading length");
					int temp = parse_time_crap(value);
					if (temp != BORK_TIME)
					{
						tag_song_ms = temp;
						info.info_set_int(field_length, tag_song_ms);
					}
				}
				else if (!stricmp_utf8(tag, "fade"))
				{
					DBG("reading fade");
					int temp = parse_time_crap(value);
					if (temp != BORK_TIME)
					{
						tag_fade_ms = temp;
						info.info_set_int(field_fade, tag_fade_ms);
					}
				}
				else if (!stricmp_utf8(tag, "utf8"))
				{
					utf8 = 1;
				}
				else if (!stricmp_utf8_partial(tag, "_lib"))
				{
					DBG("found _lib");
					if (! *(tag.get_ptr() + 4)) precede = 1;
					info.info_set(tag, value);
				}
				else if (!stricmp_utf8(tag, "_refresh"))
				{
					DBG("found _refresh");
					info.info_set(tag, value);
				}
				else if (tag[0] == '_')
				{
					DBG("found unknown required tag, failing");
					console::formatter() << "Unsupported tag found: " << tag << ", required to play file.";
					return -1;
				}
				else
				{
//					char err[64];
//					wsprintf(err, "found %s", tag);
//					DBG(err);
					info_meta_add(info, tag, value);
				}
			}
			else
			{
				// handle nested libraries only, and info is now
				// the top level internal info class
				if (!stricmp_utf8_partial(tag, "_lib"))
				{
					pfc::string8 blah;
					if (!*(tag.get_ptr() + 4)) precede = 1;
					blah.set_string(tag);
					blah.add_byte('=');
					blah.add_string(value);
					info.meta_add(info.info_get("current"), value);
				}
			}
		}
	}
	if (!utf8) info_meta_ansi( info );
	return precede;
}

static int load_exe_unpack(pfc::array_t<t_uint8> & dst, const BYTE *src, uLong srclen)
{
	DBG("loading");

	pfc::array_t<t_uint8> buf;
	buf.set_size( 0x200004 );
	BYTE *ptr = buf.get_ptr();

	uLong destlen = 0x200004;
	uncompress(ptr, &destlen, src, srclen);
	// ScrubFormat checks this, but I already did this :)
	if (destlen < 4) return 0;
	DBG("header in");
	buf.set_size( destlen );

	if ( dst.get_size() < 4 )
	{
		dst.set_size( destlen );
		memcpy( dst.get_ptr(), buf.get_ptr(), destlen );
		return 1;
	}

	DWORD dst_start = pfc::byteswap_if_be_t( *(DWORD*)(dst.get_ptr()) );
	DWORD src_start = pfc::byteswap_if_be_t( *(DWORD*)(buf.get_ptr()) );
	dst_start &= 0x1FFFFF;
	src_start &= 0x1FFFFF;
	DWORD dst_len = dst.get_size() - 4;
	DWORD src_len = buf.get_size() - 4;
	if ( dst_len > 0x200000 ) dst_len = 0x200000;
	if ( src_len > 0x200000 ) src_len = 0x200000;

	if ( src_start < dst_start )
	{
		DWORD diff = dst_start - src_start;
		dst.set_size( dst_len + 4 + diff );
		memmove( dst.get_ptr() + 4 + diff, dst.get_ptr() + 4, dst_len );
		memset( dst.get_ptr() + 4, 0, diff );
		dst_len += diff;
		dst_start = src_start;
		*(DWORD*)(dst.get_ptr()) = pfc::byteswap_if_be_t( dst_start );
	}
	if ( ( src_start + src_len ) > ( dst_start + dst_len ) )
	{
		DWORD diff = ( dst_start + dst_len ) - ( src_start + src_len );
		dst.set_size( dst_len + 4 + diff );
		memset( dst.get_ptr() + 4 + dst_len, 0, diff );
		dst_len += diff;
	}

	memcpy( dst.get_ptr() + 4 + ( src_start - dst_start ), buf.get_ptr() + 4, src_len );

	return 1;
}

class load_exe_recursive
{
	file_info_impl   internal;

	void             * state;
	abort_callback   & m_abort;
	const char       * base_path;
	const char       * filename;
	int                m_version;

	pfc::array_t<t_uint8> m_executable;

public:
	load_exe_recursive( void * p_state, service_ptr_t<file> & p_reader, const char * p_path, file_info & p_info, const char * p_base_path, int inherit, int version, abort_callback & p_abort )
	: state( p_state ), m_abort( p_abort ), base_path( p_base_path ), filename( p_path ), m_version( version )
	{
		if ( m_version != 2 ) m_version = 1;
		if ( !load( p_reader, p_info, inherit ) ) throw exception_io_data();
		if ( state ) sega_upload_program( state, m_executable.get_ptr(), m_executable.get_size() );
	}

private:
	int load( service_ptr_t<file> & r, file_info & info, int inherit )
	{
		pfc::array_t<t_uint8> buf;
		DBG("r->get_length()");
		t_filesize size64 = r->get_size_ex(m_abort);
		if (size64 < 16 || size64 > 0x10000000) return 0;
		int size = (int)size64;

		buf.set_size( 16 );
		BYTE *ptr = buf.get_ptr();
		DBG("seek(0)");
		r->seek(0, m_abort);
		DBG("read(16)");
		r->read_object(ptr, 16, m_abort);

		if (ptr[3] != m_version + 0x10) return 0;

		int reserved_size = pfc::byteswap_if_be_t( ((unsigned long*)ptr)[1] );
		int exe_size = pfc::byteswap_if_be_t( ((unsigned long*)ptr)[2] );
		if (size < 16 + reserved_size + exe_size) return 0;
		DBG("size okay");
		uLong exe_crc = pfc::byteswap_if_be_t( ((unsigned long*)ptr)[3] );
		r->seek(reserved_size + 16, m_abort);
		buf.set_size( exe_size );
		ptr = buf.get_ptr();
		r->read_object(ptr, exe_size, m_abort);
		if (exe_crc != crc32(crc32(0L, Z_NULL, 0), ptr, exe_size)) return 0;
		DBG("CRC okay");

		//file_info_i_full internal;

		if (inherit < 0)
		{
			DBG("setting first-pass info");
			// first pass
			info.info_set_int("samplerate", 44100);
			info.info_set_int("bitspersample", 16);
			info.info_set_int("channels", 2);
			info.info_set("codec", (m_version == 2) ? "DSF" : "SSF");
			info.info_set( "encoding", "synthesized" );

			if (inherit != -2)
			{
				//internal.reset();

				sega_clear_state( state, m_version );

				sega_enable_dry( state, cfg_dry );
				sega_enable_dsp( state, cfg_dsp );

				int dynarec = cfg_dsp_dynarec;
				sega_enable_dsp_dynarec( state, dynarec );

				if ( dynarec )
				{
					void * yam = 0;
					if ( m_version == 2 )
					{
						void * dcsound = sega_get_dcsound_state( state );
						yam = dcsound_get_yam_state( dcsound );
					}
					else
					{
						void * satsound = sega_get_satsound_state( state );
						yam = satsound_get_yam_state( satsound );
					}
					if ( yam ) yam_prepare_dynacode( yam );
				}
			}
		}

		int tag_song_ms = 0, tag_fade_ms = 0;

		if (size < 16 + reserved_size + exe_size + 5)
		{
			DBG("loading executable only");
			// load executable only
			info.set_length((double)(tag_song_ms+tag_fade_ms)*.001);
			if (inherit == -2) return 1;
			return load_exe_unpack(m_executable, ptr, exe_size);
		}

		int precede = 0;

		// check for tags
		{
			pfc::array_t<t_uint8> buf2;
			int len = size - (16 + reserved_size + exe_size);
			buf2.set_size( len + 1 );
			BYTE * ptr = buf2.get_ptr();
			r->read_object(ptr, 5, m_abort);
			ptr[len] = 0;
			if (!memcmp(ptr, "[TAG]", 5))
			{
				r->read_object(ptr + 5, len - 5, m_abort);
				precede = info_read(ptr, len, info, inherit, tag_song_ms, tag_fade_ms);
				if (precede < 0) return 0;
			}
		}

		if (inherit < 0)
		{
			DBG("setting length");
			if (!tag_song_ms)
			{
				tag_song_ms = cfg_deflength;
				tag_fade_ms = cfg_deffade;
			}
			info.set_length((double)(tag_song_ms+tag_fade_ms)*.001);
			if (inherit == -2) return 1;
		}

		pfc::string8 current;
		if (inherit >= 0)
		{
			current = info.info_get("current");
		}

		int rtn;

		if (precede)
		{
			// alrighty, we've at least got _lib
			const char * n;
			service_ptr_t<file> rdr;
			if (inherit < 0)
			{
				n = info.info_get("_lib");
				pfc::string8 f = filename;
				const char *fn = f.get_ptr() + f.scan_filename();
				if (!stricmp(fn, n))
				{
					// Umm, no.
					return 0;
				}
				internal.meta_add(fn, "dummy");
				internal.info_set("current", n);
			}
			else
			{
				n = info.meta_get(current, 0);
				if (info.meta_get_count_by_name(n))
				{
					// Not happening.
					return 0;
				}
				info.info_set("current",n);
			}

			pfc::string8 fn = base_path;
			fn += n;
			DBG(fn);
			filesystem::g_open( rdr, fn, filesystem::open_mode_read, m_abort );
			DBG("g_open success");
			if (inherit < 0)
			{
				if (!load(rdr, internal, -inherit)) return 0;
			}
			else 
			{
				if (!load(rdr, info, inherit)) return 0;
			}
			rdr.release();
			DBG("success");
			if (inherit >= 0)
			{
				info.meta_remove_field(current);
			}
		}

		rtn = load_exe_unpack(m_executable, buf.get_ptr(), exe_size);
		buf.set_size(0);
		if (!rtn)
		{
			return 0;
		}

		char temp[16];
		unsigned count, cur, lib;
		service_ptr_t<file> rdr;

		if (inherit < 0)
		{
			for (lib = 2;; lib++)
			{
				sprintf(temp, "_lib%u", lib);
				const char * v = info.info_get(temp);
				if (!v) break;

				internal.info_set("current", v);

				pfc::string8 fn(base_path);
				fn += v;
				filesystem::g_open( rdr, fn, filesystem::open_mode_read, m_abort );
				if ( !load( rdr, internal, 0 ) ) return 0;
				rdr.release();
			}
		}
		else
		{
			const char * n, * v;
			bool found;
			pfc::ptr_list_t<const char> libs;
			count = info.meta_get_count_by_name(current);
			for (cur = 0; cur < count; cur++)
			{
				libs.add_item(info.meta_get(current, cur));
			}
			for (lib = 2;; lib++)
			{
				sprintf(temp, "_lib%u", lib);
				found = false;
				for (cur=0; cur<count; cur++)
				{
					n = libs[cur];
					if (stricmp_utf8_partial(n, temp)) continue;
					v = strchr(n, '=') + 1;
					if (info.meta_get_count_by_name(v))
					{
						// wtf are you trying to pull? :P
						return 0;
					}
					info.info_set("current", v);
					pfc::string8 fn(base_path);
					fn.add_string(n);
					filesystem::g_open( rdr, fn, filesystem::open_mode_read, m_abort );
					if ( !load( rdr, info, 0 ) ) return 0;
					rdr.release();
					found = true;
				}
				if (!found) break;
			}
		}
		return 1;
	}
};

class input_xsf
{
	bool no_loop, eof;

	pfc::array_t<t_uint8> sega_state;
	pfc::array_t<t_int16> sample_buffer;

	service_ptr_t<file> m_file;

	pfc::string8 base_path;
	pfc::string8 filename;

	int err;

	int xsf_version;

	int data_written,remainder,pos_delta,startsilence,silence;

	double xsfemu_pos;

	int song_len,fade_len;
	int tag_song_ms,tag_fade_ms;

	file_info_impl m_info;

	bool do_filter, do_suppressendsilence;

	int load_xsf(service_ptr_t<file> & r, const char * p_path, file_info & info, bool full_open, abort_callback & p_abort);

public:
	input_xsf()
	{
	}

	~input_xsf()
	{
		void * yam = 0;
		if ( xsf_version == 2 )
		{
			void * dcsound = sega_get_dcsound_state( sega_state.get_ptr() );
			yam = dcsound_get_yam_state( dcsound );
		}
		else
		{
			void * satsound = sega_get_satsound_state( sega_state.get_ptr() );
			yam = satsound_get_yam_state( satsound );
		}
		if ( yam ) yam_unprepare_dynacode( yam );

	}

	void open( service_ptr_t<file> p_file, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		input_open_file_helper( p_file, p_path, p_reason, p_abort );

		{
			xsf_version = load_xsf( p_file, p_path, m_info, false, p_abort );

			if (xsf_version == 1) InterlockedIncrement(&ssf_count);
			else if (xsf_version == 2) InterlockedIncrement(&dsf_count);

			tag_song_ms = 0;

			m_file = p_file;

			{
				pfc::string8 f = p_path;
				int meh = f.scan_filename();
				filename = f.get_ptr() + meh;
				f.truncate(meh);
				base_path = f;
			}
		}
	}

	void get_info( file_info & p_info, abort_callback & p_abort )
	{
		p_info.copy( m_info );
	}

	t_filestats get_file_stats( abort_callback & p_abort )
	{
		return m_file->get_stats( p_abort );
	}

	void decode_initialize( unsigned p_flags, abort_callback & p_abort )
	{
		const char *t = m_info.info_get(field_length);
		if (t)
		{
			tag_song_ms = atoi(t);
			t = m_info.info_get(field_fade);
			if (t) tag_fade_ms = atoi(t);
		}
		if (!tag_song_ms)
		{
			tag_song_ms = cfg_deflength;
			tag_fade_ms = cfg_deffade;
		}

		{
			insync(g_sync);
			if (!initialized)
			{
				DBG("sega_init()");
				if (sega_init()) throw std::exception("Sega emulator static initialization failed");
				initialized = 1;
			}
		}

		sega_state.set_size( sega_get_state_size( xsf_version ) );

		void * pEmu = sega_state.get_ptr();

		m_info.reset();

		{
			pfc::string8 path = base_path;
			path += filename;
			err = load_xsf( m_file, path, m_info, true, p_abort );
		}

		xsfemu_pos = 0.;

		startsilence = silence = 0;

		eof = 0;
		err = 0;
		data_written = 0;
		remainder = 0;
		pos_delta = 0;
		xsfemu_pos = 0;
		no_loop = ( p_flags & input_flag_no_looping ) || !cfg_infinite;

		calcfade();

		do_suppressendsilence = !! cfg_suppressendsilence;

		if ( cfg_suppressopeningsilence ) // ohcrap
		{
			unsigned skip_max = cfg_endsilenceseconds * 44100;

			for (;;)
			{
				p_abort.check();

				unsigned skip_howmany = skip_max - silence;
				if ( skip_howmany > 1024 ) skip_howmany = 1024;
				sample_buffer.grow_size( skip_howmany * 2 );
				int rtn = sega_execute( pEmu, 0x7FFFFFFF, sample_buffer.get_ptr(), & skip_howmany );
				if ( rtn < 0 ) throw exception_io_data();
				short * foo = sample_buffer.get_ptr();
				unsigned i;
				for ( i = 0; i < skip_howmany; ++i )
				{
					if ( foo[ 0 ] || foo[ 1 ] ) break;
					foo += 2;
				}
				silence += i;
				if ( i < skip_howmany )
				{
					remainder = skip_howmany - i;
					memmove( sample_buffer.get_ptr(), foo, remainder * sizeof( short ) * 2 );
					break;
				}
				if ( silence >= skip_max ) eof = true;
			}

			startsilence += silence;
			silence = 0;
		}
	}

	bool decode_run( audio_chunk & p_chunk, abort_callback & p_abort )
	{
		if ( eof ) return false;

		if ( err < 0 ) throw exception_io_data();
		if ( no_loop && tag_song_ms && ( pos_delta + MulDiv( data_written, 1000, 44100 ) ) >= tag_song_ms + tag_fade_ms )
			return false;

		UINT written = 0;

		int samples;

		if ( no_loop )
		{
			samples = ( song_len + fade_len ) - data_written;
			if ( samples > 1024 ) samples = 1024;
		}
		else
		{
			samples = 1024;
		}

		sample_buffer.grow_size( samples * 2 );

		if ( remainder )
		{
			written = remainder;
			remainder = 0;
		}
		else
		{
			written = samples;
			//DBG("hw_execute()");
			err = sega_execute( sega_state.get_ptr(), 0x7FFFFFFF, sample_buffer.get_ptr(), & written );
			if ( err < 0 )
			{
				console::formatter() << "Execution halted with an error in " << filename;
			}
			if ( ! written ) throw exception_io_data();
		}

		xsfemu_pos += double( written ) / 44100.;

		if ( cfg_suppressendsilence )
		{
			uLong n;
			short * foo = sample_buffer.get_ptr();
			for ( n = 0; n < written; ++n )
			{
				if ( foo[ 0 ] == 0 && foo[ 1 ] == 0 ) ++silence;
				else silence = 0;
				foo += 2;
			}
			if ( silence >= 44100 * cfg_endsilenceseconds )
				return false;
		}

		int d_start, d_end;
		d_start = data_written;
		data_written += written;
		d_end = data_written;

		if ( tag_song_ms && d_end > song_len && no_loop )
		{
			short * foo = sample_buffer.get_ptr();
			int n;
			for( n = d_start; n < d_end; ++n )
			{
				if ( n > song_len )
				{
					if ( n > song_len + fade_len )
					{
						* ( DWORD * ) foo = 0;
					}
					else
					{
						int bleh = song_len + fade_len - n;
						foo[ 0 ] = MulDiv( foo[ 0 ], bleh, fade_len );
						foo[ 1 ] = MulDiv( foo[ 1 ], bleh, fade_len );
					}
				}
				foo += 2;
			}
		}

		p_chunk.set_data_fixedpoint( sample_buffer.get_ptr(), written * 4, 44100, 2, 16, audio_chunk::channel_config_stereo );

		return true;
	}

	void decode_seek( double p_seconds, abort_callback & p_abort )
	{
		eof = false;

		void *pEmu = sega_state.get_ptr();
		if ( p_seconds < xsfemu_pos )
		{
			//file_info *temp = new file_info_i_full;
			file_info_impl temp;
			pfc::string8 path = base_path;
			path += filename;

			load_xsf( m_file, path, temp, true, p_abort );
			xsfemu_pos = 0.;
			if ( startsilence )
			{
				unsigned int meh = startsilence;
				sega_execute( pEmu, 0x7FFFFFFF, 0, & meh );
			}
		}
		unsigned int howmany = ( int )( audio_math::time_to_samples( p_seconds - xsfemu_pos, 44100 ) );

		// more abortable, and emu doesn't like doing huge numbers of samples per call anyway
		while ( howmany )
		{
			p_abort.check();

			unsigned todo = howmany;
			if ( todo > 2048 ) todo = 2048;
			int rtn = sega_execute( pEmu, 0x7FFFFFFF, 0, & todo );
			if ( rtn < 0 || ! todo )
			{
				eof = true;
				return;
			}
			howmany -= todo;
		}

		data_written = 0;
		pos_delta = ( int )( p_seconds * 1000. );
		xsfemu_pos = p_seconds;

		calcfade();
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
		m_info.copy( p_info );

		pfc::array_t<t_uint8> buffer;
		buffer.set_size( 16 );

		m_file->seek( 0, p_abort );

		BYTE *ptr = buffer.get_ptr();
		m_file->read_object( ptr, 16, p_abort );
		if (ptr[0] != 'P' || ptr[1] != 'S' || ptr[2] != 'F' ||
			(ptr[3] != 1 && ptr[3] != 2)) throw exception_io_data();
		int reserved_size = pfc::byteswap_if_be_t( ((unsigned long*)ptr)[1] );
		int exe_size = pfc::byteswap_if_be_t( ((unsigned long*)ptr)[2] );
		m_file->seek(16 + reserved_size + exe_size, p_abort);
		m_file->set_eof(p_abort);

		pfc::string8 tag = "[TAG]utf8=1\n";

		int first = 1;
		// _lib and _refresh tags first
		int n, p = p_info.info_get_count();
		for (n = 0; n < p; n++)
		{
			const char *t = p_info.info_enum_name(n);
			if (*t == '_')
			{
				if (first) first = 0;
				else tag.add_byte('\n');
				tag += t;
				tag.add_byte('=');
				tag += p_info.info_enum_value(n);
			}
		}
		// Then info
		p = p_info.meta_get_count();
		for (n = 0; n < p; n++)
		{
			const char * t = p_info.meta_enum_name(n);
			if (*t == '_' ||
				!stricmp(t, "length") ||
				!stricmp(t, "fade")) continue; // dummy protection
			if (!stricmp(t, "album")) info_meta_write(tag, p_info, "game", n, first);
			else if (!stricmp(t, "date"))
			{
				const char * val = p_info.meta_enum_value(n, 0);
				char * end;
				strtoul(p_info.meta_enum_value(n, 0), &end, 10);
				if (size_t(end - val) < strlen(val))
					info_meta_write(tag, p_info, t, n, first);
				else
					info_meta_write(tag, p_info, "year", n, first);
			}
			else info_meta_write(tag, p_info, t, n, first);
		}
		// Then time and fade
		{
			int tag_song_ms = 0, tag_fade_ms = 0;
			const char *t = p_info.info_get(field_length);
			if (t)
			{
				char temp[16];
				tag_song_ms = atoi(t);
				if (first) first = 0;
				else tag.add_byte('\n');
				tag += "length=";
				print_time_crap(tag_song_ms, temp);
				tag += temp;
				t = p_info.info_get(field_fade);
				if (t)
				{
					tag_fade_ms = atoi(t);
					tag.add_byte('\n');
					tag += "fade=";
					print_time_crap(tag_fade_ms, (char *)&temp);
					tag += temp;
				}
			}
		}

		// Then ReplayGain
		/*
		p = p_info.info_get_count();
		for (n = 0; n < p; n++)
		{
			const char *t = p_info.info_enum_name(n);
			if (!strnicmp(t, "replaygain_",11))
			{
				if (first) first = 0;
				else tag.add_byte('\n');
				tag += t;
				else tag.add_byte('=');
				tag += p_info.info_enum_value(n);
			}
		}
		*/
		replaygain_info rg = p_info.get_replaygain();
		char rgbuf[replaygain_info::text_buffer_size];
		if (rg.is_track_gain_present())
		{
			rg.format_track_gain(rgbuf);
			if (first) first = 0;
			else tag.add_byte('\n');
			tag += "replaygain_track_gain";
			tag.add_byte('=');
			tag += rgbuf;
		}
		if (rg.is_track_peak_present())
		{
			rg.format_track_peak(rgbuf);
			if (first) first = 0;
			else tag.add_byte('\n');
			tag += "replaygain_track_peak";
			tag.add_byte('=');
			tag += rgbuf;
		}
		if (rg.is_album_gain_present())
		{
			rg.format_album_gain(rgbuf);
			if (first) first = 0;
			else tag.add_byte('\n');
			tag += "replaygain_album_gain";
			tag.add_byte('=');
			tag += rgbuf;
		}
		if (rg.is_album_peak_present())
		{
			rg.format_album_peak(rgbuf);
			if (first) first = 0;
			else tag.add_byte('\n');
			tag += "replaygain_album_peak";
			tag.add_byte('=');
			tag += rgbuf;
		}

		m_file->write_object( tag.get_ptr(), tag.length(), p_abort );
	}

	static bool g_is_our_content_type( const char * p_content_type )
	{
		return false;
	}

	static bool g_is_our_path( const char * p_full_path, const char * p_extension )
	{
		return (!stricmp(p_extension,"ssf") || !stricmp(p_extension,"minissf") ||
				!stricmp(p_extension,"dsf") || !stricmp(p_extension, "minidsf"));
	}

private:
	void calcfade()
	{
		song_len=MulDiv(tag_song_ms-pos_delta,44100,1000);
		fade_len=MulDiv(tag_fade_ms,44100,1000);
	}
};

int input_xsf::load_xsf(service_ptr_t<file> & r, const char * p_path, file_info & info, bool full_open, abort_callback & p_abort)
{
	unsigned char header[16];
	int fl;
	__int64 fl64;
	fl64 = r->get_size_ex(p_abort);
	if (fl64 > 1<<30) throw exception_io_data();
	fl = (int)fl64;
	if (fl < 16) throw exception_io_data();
	r->seek(0, p_abort);
	r->read_object(header, 16, p_abort);
	if (memcmp(header, "PSF", 3)) throw exception_io_data();

	if (header[3] == 0x11 || header[3] == 0x12)
	{
		int version = header[3] == 0x12 ? 2 : 1;

		if (!full_open)
		{
			load_exe_recursive( 0, r, p_path, info, 0, -2, version, p_abort );
			return version;
		}
		
		load_exe_recursive( sega_state.get_ptr(), r, p_path, info, base_path, -1, version, p_abort);

		return version;
	}

	return 0;
}

static BOOL CALLBACK ConfigProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	struct config_data
	{
		CHyperLink m_link_neill;
		CHyperLink m_link_kode54;
	};

	switch(msg)
	{
	case WM_INITDIALOG:
		{
			int n;
			uSendDlgItemMessage(wnd, IDC_INDEFINITE, BM_SETCHECK, cfg_infinite, 0);
			uSendDlgItemMessage(wnd, IDC_SOS, BM_SETCHECK, cfg_suppressopeningsilence, 0);
			uSendDlgItemMessage(wnd, IDC_SES, BM_SETCHECK, cfg_suppressendsilence, 0);
			uSendDlgItemMessage(wnd, IDC_DRY, BM_SETCHECK, cfg_dry, 0);
			uSendDlgItemMessage(wnd, IDC_DSP, BM_SETCHECK, cfg_dsp, 0);
			uSendDlgItemMessage(wnd, IDC_DSP_DYNAREC, BM_SETCHECK, cfg_dsp_dynarec, 0);

			SetDlgItemInt(wnd,IDC_SILENCE,cfg_endsilenceseconds,0);

			{
				char temp[16];
				// wsprintf((char *)&temp, "= %d Hz", 33868800 / cfg_divider);
				// SetDlgItemText(wnd, IDC_HZ, (char *)&temp);

				print_time_crap(cfg_deflength, (char *)&temp);
				uSetDlgItemText(wnd, IDC_DLENGTH, (char *)&temp);

				print_time_crap(cfg_deffade, (char *)&temp);
				uSetDlgItemText(wnd, IDC_DFADE, (char *)&temp);
			}

			config_data * data = new config_data;

			data->m_link_neill.SetLabel( _T( "Neill Corlett's Home Page" ) );
			data->m_link_neill.SetHyperLink( _T( "http://www.neillcorlett.com/" ) );
			data->m_link_neill.SubclassWindow( GetDlgItem( wnd, IDC_URL ) );

			data->m_link_kode54.SetLabel( _T( "kode's foobar2000 plug-ins" ) );
			data->m_link_kode54.SetHyperLink( _T( "http://kode54.foobar2000.org/" ) );
			data->m_link_kode54.SubclassWindow( GetDlgItem( wnd, IDC_K54 ) );

			{
				/*OSVERSIONINFO ovi = { 0 };
				ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
				BOOL bRet = ::GetVersionEx(&ovi);
				if ( bRet && ( ovi.dwMajorVersion >= 5 ) )*/
				{
					DWORD color = GetSysColor( 26 /* COLOR_HOTLIGHT */ );
					data->m_link_neill.m_clrLink = color;
					data->m_link_neill.m_clrVisited = color;
					data->m_link_kode54.m_clrLink = color;
					data->m_link_kode54.m_clrVisited = color;
				}
			}

			SetWindowLong( wnd, DWL_USER, (LONG) data );

			unsigned long ssfc = ssf_count;
			unsigned long dsfc = dsf_count;

			if (dsfc)
			{
				__int64 total = ((__int64)ssfc + (__int64)dsfc) - 1;
				__int64 meh = ((__int64)rand() * total) / RAND_MAX;
				if (meh >= (__int64)ssfc)
				{
					uSendMessage(GetDlgItem(wnd, IDC_LOGO), STM_SETIMAGE, IMAGE_BITMAP, (long) uLoadImage(core_api::get_my_instance(), MAKEINTRESOURCEA(IDB_LOGOBMP2), IMAGE_BITMAP, 0, 0, 0));
				}
			}
		}
		return 1;

	case WM_DESTROY:
		{
			config_data * data = reinterpret_cast< config_data * > ( GetWindowLong( wnd, DWL_USER ) );
			if ( data )
			{
				data->m_link_neill.DestroyWindow();
				data->m_link_kode54.DestroyWindow();
				delete data;
			}
		}
		break;

	case WM_COMMAND:
		switch(wp)
		{
		case IDC_INDEFINITE:
			cfg_infinite = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		case IDC_SOS:
			cfg_suppressopeningsilence = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
			if (cfg_suppressopeningsilence && !cfg_endsilenceseconds)
			{
				cfg_endsilenceseconds = 1;
				SetDlgItemInt(wnd, IDC_SILENCE, cfg_endsilenceseconds, 0);
			}
			break;
		case IDC_SES:
			cfg_suppressendsilence = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
			if (cfg_suppressendsilence && !cfg_endsilenceseconds)
			{
				cfg_endsilenceseconds = 1;
				SetDlgItemInt(wnd, IDC_SILENCE, cfg_endsilenceseconds, 0);
			}
			break;
		case IDC_DRY:
			cfg_dry = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		case IDC_DSP:
			cfg_dsp = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		case IDC_DSP_DYNAREC:
			cfg_dsp_dynarec = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		case (EN_CHANGE<<16)|IDC_SILENCE:
			{
				int res;
				int temp = GetDlgItemInt(wnd, IDC_SILENCE, &res, 0);
				if (res && temp)
					cfg_endsilenceseconds = temp;
			}
			break;
		case (EN_KILLFOCUS<<16)|IDC_SILENCE:
			{
				int res;
				int temp = GetDlgItemInt(wnd, IDC_SILENCE, &res, 0);
				if (res && !temp)
					SetDlgItemInt(wnd, IDC_SILENCE, cfg_endsilenceseconds, 0);
			}
			break;
		case (EN_CHANGE<<16)|IDC_DLENGTH:
			{
				int meh = parse_time_crap(string_utf8_from_window((HWND)lp));
				if (meh != BORK_TIME) cfg_deflength = meh;
			}
			break;
		case (EN_KILLFOCUS<<16)|IDC_DLENGTH:
			{
				char temp[16];
				print_time_crap(cfg_deflength, (char *)&temp);
				uSetWindowText((HWND)lp, temp);
			}
			break;
		case (EN_CHANGE<<16)|IDC_DFADE:
			{
				int meh = parse_time_crap(string_utf8_from_window((HWND)lp));
				if (meh != BORK_TIME) cfg_deffade = meh;
			}
			break;
		case (EN_KILLFOCUS<<16)|IDC_DFADE:
			{
				char temp[16];
				print_time_crap(cfg_deffade, (char *)&temp);
				uSetWindowText((HWND)lp, temp);
			}
			break;
		}
		break;
	}
	return 0;
}

class preferences_page_xsf : public preferences_page
{
public:
	virtual HWND create(HWND parent)
	{
		return uCreateDialog(IDD_PSF_CONFIG,parent,ConfigProc);
	}
	GUID get_guid()
	{
		// {8CAEADE6-1AAA-4763-B8CF-DE0BAE3EBEE9}
		static const GUID guid = 
		{ 0x8caeade6, 0x1aaa, 0x4763, { 0xb8, 0xcf, 0xde, 0xb, 0xae, 0x3e, 0xbe, 0xe9 } };
		return guid;
	}
	virtual const char * get_name() {return "SSF/DSF Decoder";}
	GUID get_parent_guid() {return guid_input;}

	bool reset_query() {return true;}
	void reset()
	{
		cfg_infinite = 0;
		cfg_deflength = 170000;
		cfg_deffade = 10000;
		cfg_suppressopeningsilence = 1;
		cfg_suppressendsilence = 1;
		cfg_endsilenceseconds = 5;
		cfg_dry = 1;
		cfg_dsp = 1;
		cfg_dsp_dynarec = 1;
	}
};

typedef struct
{
	unsigned song, fade;
} INFOSTRUCT;

static BOOL CALLBACK TimeProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		uSetWindowLong(wnd,DWL_USER,lp);
		{
			INFOSTRUCT * i=(INFOSTRUCT*)lp;
			char temp[16];
			if (!i->song && !i->fade) uSetWindowText(wnd, "Set length");
			else uSetWindowText(wnd, "Edit length");
			if (i->song)
			{
				print_time_crap(i->song, (char*)&temp);
				uSetDlgItemText(wnd, IDC_LENGTH, (char*)&temp);
			}
			if (i->fade)
			{
				print_time_crap(i->fade, (char*)&temp);
				uSetDlgItemText(wnd, IDC_FADE, (char*)&temp);
			}
		}
		cfg_placement.on_window_creation(wnd);
		return 1;
	case WM_COMMAND:
		switch(wp)
		{
		case IDOK:
			{
				INFOSTRUCT * i=(INFOSTRUCT*)uGetWindowLong(wnd,DWL_USER);
				int foo;
				foo = parse_time_crap(string_utf8_from_window(wnd, IDC_LENGTH));
				if (foo != BORK_TIME) i->song = foo;
				else i->song = 0;
				foo = parse_time_crap(string_utf8_from_window(wnd, IDC_FADE));
				if (foo != BORK_TIME) i->fade = foo;
				else i->fade = 0;
			}
			EndDialog(wnd,1);
			break;
		case IDCANCEL:
			EndDialog(wnd,0);
			break;
		}
		break;
	case WM_DESTROY:
		cfg_placement.on_window_destruction(wnd);
		break;
	}
	return 0;
}

static bool context_time_dialog(unsigned * song_ms, unsigned * fade_ms)
{
	bool ret;
	INFOSTRUCT * i = new INFOSTRUCT;
	if (!i) return 0;
	i->song = *song_ms;
	i->fade = *fade_ms;
	HWND hwnd = core_api::get_main_window();
	ret = uDialogBox(IDD_TIME, hwnd, TimeProc, (long)i) > 0;
	if (ret)
	{
		*song_ms = i->song;
		*fade_ms = i->fade;
	}
	delete i;
	return ret;
}

class context_xsf : public contextmenu_item_simple
{
public:
	virtual unsigned get_num_items() { return 1; }

	virtual void get_item_name(unsigned n, pfc::string_base & out)
	{
		out = "Edit length";
	}

	virtual void get_item_default_path(unsigned n, pfc::string_base & out)
	{
		out.reset();
	}

	virtual bool get_item_description(unsigned n, pfc::string_base & out)
	{
		out = "Edits the length of the selected SSF/DSF file, or sets the length of all selected SSF/DSF files.";
		return true;
	}

	virtual GUID get_item_guid(unsigned p_index)
	{
		static const GUID guids[] =
		{
			{ 0xc359d3b6, 0xba3, 0x4087, { 0xa5, 0xac, 0xa8, 0x30, 0xf1, 0xf0, 0xcd, 0xd } },
			{ 0xad8c42c0, 0x51b3, 0x4616, { 0x94, 0x25, 0x26, 0x96, 0xc5, 0x26, 0x6c, 0x4f } }
		};
		assert(p_index < tabsize(guids));
		return guids[p_index];
	}

	virtual bool context_get_display(unsigned n,const pfc::list_base_const_t<metadb_handle_ptr> & data,pfc::string_base & out,unsigned & displayflags,const GUID &)
	{
		unsigned i, j;
		i = data.get_count();
		for (j = 0; j < i; j++)
		{
			pfc::string_extension ext(data.get_item(j)->get_path());
			if (stricmp_utf8(ext, "SSF") && stricmp_utf8(ext, "MINISSF") &&
				stricmp_utf8(ext, "DSF") && stricmp_utf8(ext, "MINIDSF")) return false;
		}
		if (i == 1) out = "Edit length";
		else out = "Set length";
		return true;
	}

	virtual void context_command(unsigned n,const pfc::list_base_const_t<metadb_handle_ptr> & data,const GUID& caller)
	{
		unsigned tag_song_ms = 0, tag_fade_ms = 0;
		unsigned i = data.get_count();
		file_info_impl info;
		abort_callback_impl m_abort;
		if (i == 1)
		{
			// fetch info from single file
			metadb_handle_ptr handle = data.get_item(0);
			handle->metadb_lock();
			const file_info * p_info;
			if (handle->get_info_locked(p_info) && p_info)
			{
				const char *t = p_info->info_get(field_length);
				if (t) tag_song_ms = atoi(t);
				t = p_info->info_get(field_fade);
				if (t) tag_fade_ms = atoi(t);
			}
			handle->metadb_unlock();
		}
		if (!context_time_dialog(&tag_song_ms, &tag_fade_ms)) return;
		static_api_ptr_t<metadb_io> p_imgr;
		for (unsigned j = 0; j < i; j++)
		{
			metadb_handle_ptr foo = data.get_item(j);
//			foo->metadb_lock();
			if (foo->get_info(info))
			{
				if (tag_song_ms > 0) info.info_set_int(field_length, tag_song_ms);
				else info.info_remove(field_length);
				if (tag_fade_ms > 0) info.info_set_int(field_fade, tag_fade_ms);
				else info.info_remove(field_fade);
				{
					if (!tag_song_ms)
					{
						tag_song_ms = cfg_deflength;
						tag_fade_ms = cfg_deffade;
					}
					double length = (double)(tag_song_ms + tag_fade_ms) * .001;
					info.set_length(length);
				}
				if ( metadb_io::update_info_success != p_imgr->update_info(foo, info, core_api::get_main_window(), true) ) j = i;
			}
			else j = i;
//			foo->metadb_unlock();
		}
	}
};

class xsf_file_types : public input_file_type
{
	virtual unsigned get_count()
	{
		return 2;
	}

	virtual bool get_name(unsigned idx, pfc::string_base & out)
	{
		if (idx > 1) return false;
		else if (idx == 0) out = "SSF files";
		else if (idx == 1) out = "DSF files";
		return true;
	}

	virtual bool get_mask(unsigned idx, pfc::string_base & out)
	{
		if (idx > 1) return false;
		else if (idx == 0) out = "*.SSF;*.MINISSF";
		else if (idx == 1) out = "*.DSF;*.MINIDSF";
		return true;
	}

	virtual bool is_associatable(unsigned idx)
	{
		return true;
	}
};

class version_xsf : public componentversion
{
public:
	virtual void get_file_name(pfc::string_base & out) { out = core_api::get_my_file_name(); }
	virtual void get_component_name(pfc::string_base & out) { out = "Highly Theoretical"; }
	virtual void get_component_version(pfc::string_base & out) { out = MYVERSION; }
	virtual void get_about_message(pfc::string_base & out)
	{
		out = "Foobar2000 version by kode54\nOriginal library and concept by Neill Corlett" /*"\n\nCore: ";
		out += psx_getversion();
		out +=*/ "\n\nhttp://www.neillcorlett.com/\nhttp://kode54.foobar2000.org/";
	}
};

static input_singletrack_factory_t<input_xsf>                      g_input_xsf_factory;
static preferences_page_factory_t <preferences_page_xsf>           g_config_xsf_factory;
static contextmenu_item_factory_t <context_xsf>                    g_contextmenu_item_xsf_factory;
static service_factory_single_t   <xsf_file_types> g_input_file_type_xsf_factory;
static service_factory_single_t   <version_xsf>   g_componentversion_xsf_factory;
