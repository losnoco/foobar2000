#define MYVERSION "2.0.20"

/*
	changelog

2011-03-10 04:53 UTC - kode54
- Fixed handling PSF1 refresh tags from PSFLIB files
- Version is now 2.0.20

2011-01-26 04:04 UTC - kode54
- Fixed playback initialization for when files do not specify a
  fade time
- Version is now 2.0.19

2010-11-20 21:16 UTC - kode54
- Changed zlib dependency to use standard zlib1.dll
- Version is now 2.0.18

2010-07-13 03:03 UTC - kode54
- Implemented better end silence detection
- Version is now 2.0.17

2010-07-11 03:49 UTC - kode54
- Updated PSX response filter to use all float storage

2010-07-08 23:05 UTC - kode54
- Implemented live ADSR change support

2010-07-05 07:24 UTC - kode54
- Implemented SPU IRQ support

2010-07-04 21:00 UTC - kode54
- Implemented noise and frequency modulation support

2010-04-13 14:58 UTC - kode54
- Amended preferences WM_INITDIALOG handler
- Version is now 2.0.16

2010-03-19 16:59 UTC - kode54
- Updated zlib to version 1.2.4
- Version is now 2.0.15

2010-03-15 12:08 UTC - kode54
- Fixed Unicode tag reading
- Version is now 2.0.14

2010-03-07 05:22 UTC - kode54
- Fixed to abort loading properly if file is neither PSF nor PSF2
- Version is now 2.0.13

2010-01-13 00:43 UTC - kode54
- Updated context menu handler
- Version is now 2.0.12

2010-01-11 13:14 UTC - kode54
- Updated preferences page to 1.0 API
- Version is now 2.0.11

2009-08-08 23:50 UTC - kode54
- Implemented more ScrubFormat features into PSX EXE loader

2009-08-06 00:34 UTC - kode54
- Fixed start silence skipping when resetting state in seek function

2009-08-04 09:00 UTC - kode54
- Updated context menu tag writer to new metadb_io_v2 API
- Version is now 2.0.9

2009-08-02 23:58 UTC - kode54
- Fixed bug with start silence detection hitting the maximum length
- Version is now 2.0.8

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

#define _WIN32_WINNT 0x0501
#include "../SDK/foobar2000.h"
#include "../helpers/window_placement_helper.h"
#include "../ATLHelpers/ATLHelpers.h"

#include "resource.h"

#include <stdio.h>
#include <zlib.h>

#if 0
#include "../../psfemucore/psx.h"
#include "../../psfemucore/iop.h"
#include "../../psfemucore/r3000.h"
#include "../../psfemucore/spu.h"
#else
#include "../../../ESP/PSX/Core/psx.h"
#include "../../../ESP/PSX/Core/iop.h"
#include "../../../ESP/PSX/Core/r3000.h"
#include "../../../ESP/PSX/Core/spu.h"
#endif

#include "../../psfemucore/psf2fs.h"

#include "PSXFilter.h"

#include "circular_buffer.h"

#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>
#include <atlctrls.h>
#include <atlctrlx.h>

//#define DBG(a) OutputDebugString(a)
#define DBG(a)

typedef unsigned long u_long;

typedef struct {
	u_long pc0;
	u_long gp0;
	u_long t_addr;
	u_long t_size;
	u_long d_addr;
	u_long d_size;
	u_long b_addr;
	u_long b_size;
	u_long s_ptr;
	u_long s_size;
	u_long sp,fp,gp,ret,base;
} exec_header_t;

typedef struct {
	char key[8];
	u_long text;
	u_long data;
	exec_header_t exec;
	char title[60];
} psxexe_hdr_t;

critical_section g_sync;
static int initialized = 0;
volatile long psf_count = 0, psf2_count = 0;

// {A18393A9-A816-4565-B4E2-F407DEDD937F}
static const GUID guid_cfg_infinite = 
{ 0xa18393a9, 0xa816, 0x4565, { 0xb4, 0xe2, 0xf4, 0x7, 0xde, 0xdd, 0x93, 0x7f } };
// {8525FF23-965F-4271-B8B9-F23DC839AE3C}
static const GUID guid_cfg_info = 
{ 0x8525ff23, 0x965f, 0x4271, { 0xb8, 0xb9, 0xf2, 0x3d, 0xc8, 0x39, 0xae, 0x3c } };
// {03A97779-C731-447d-94EF-1C80C9A0606B}
static const GUID guid_cfg_deflength = 
{ 0x3a97779, 0xc731, 0x447d, { 0x94, 0xef, 0x1c, 0x80, 0xc9, 0xa0, 0x60, 0x6b } };
// {01448061-6F56-488b-BB3C-510A61FE72A8}
static const GUID guid_cfg_deffade = 
{ 0x1448061, 0x6f56, 0x488b, { 0xbb, 0x3c, 0x51, 0xa, 0x61, 0xfe, 0x72, 0xa8 } };
// {F25817AB-FF74-4770-87A5-5E9AB8FC4C4D}
static const GUID guid_cfg_suppressopeningsilence = 
{ 0xf25817ab, 0xff74, 0x4770, { 0x87, 0xa5, 0x5e, 0x9a, 0xb8, 0xfc, 0x4c, 0x4d } };
// {D40A957C-0C5C-4cde-A303-175E0FABEC70}
static const GUID guid_cfg_suppressendsilence = 
{ 0xd40a957c, 0xc5c, 0x4cde, { 0xa3, 0x3, 0x17, 0x5e, 0xf, 0xab, 0xec, 0x70 } };
// {1BBE3011-C59C-4632-8F88-175E815AD27C}
static const GUID guid_cfg_endsilenceseconds = 
{ 0x1bbe3011, 0xc59c, 0x4632, { 0x8f, 0x88, 0x17, 0x5e, 0x81, 0x5a, 0xd2, 0x7c } };
// {C1D9913B-2357-442b-A3D5-4A205764C925}
static const GUID guid_cfg_reverb = 
{ 0xc1d9913b, 0x2357, 0x442b, { 0xa3, 0xd5, 0x4a, 0x20, 0x57, 0x64, 0xc9, 0x25 } };
// {57549E60-64F6-4235-8031-77503A08A41D}
static const GUID guid_cfg_freq = 
{ 0x57549e60, 0x64f6, 0x4235, { 0x80, 0x31, 0x77, 0x50, 0x3a, 0x8, 0xa4, 0x1d } };
// {B72B332D-208E-47ab-B968-2A40481CA5E3}
static const GUID guid_cfg_compat = 
{ 0xb72b332d, 0x208e, 0x47ab, { 0xb9, 0x68, 0x2a, 0x40, 0x48, 0x1c, 0xa5, 0xe3 } };
// {BC3A4F79-54EF-4202-9B86-D81297DAC217}
static const GUID guid_cfg_placement = 
{ 0xbc3a4f79, 0x54ef, 0x4202, { 0x9b, 0x86, 0xd8, 0x12, 0x97, 0xda, 0xc2, 0x17 } };

enum
{
	default_cfg_infinite = 0,
	default_cfg_info = 0,
	default_cfg_deflength = 170000,
	default_cfg_deffade = 10000,
	default_cfg_suppressopeningsilence = 1,
	default_cfg_suppressendsilence = 1,
	default_cfg_endsilenceseconds = 5,
	default_cfg_reverb = 1,
	default_cfg_freq = 1,
	default_cfg_compat = IOP_COMPAT_FRIENDLY
};

static cfg_int cfg_infinite(guid_cfg_infinite,0);
static cfg_int cfg_info(guid_cfg_info, 0);
static cfg_int cfg_deflength(guid_cfg_deflength,170000);
static cfg_int cfg_deffade(guid_cfg_deffade,10000);
static cfg_int cfg_suppressopeningsilence(guid_cfg_suppressopeningsilence,1);
static cfg_int cfg_suppressendsilence(guid_cfg_suppressendsilence,1);
static cfg_int cfg_endsilenceseconds(guid_cfg_endsilenceseconds,5);
static cfg_int cfg_reverb(guid_cfg_reverb,1);
static cfg_int cfg_freq(guid_cfg_freq,1);
static cfg_int cfg_compat(guid_cfg_compat,IOP_COMPAT_FRIENDLY);
static cfg_window_placement cfg_placement(guid_cfg_placement);

static const char field_length[]="psf_length";
static const char field_fade[]="psf_fade";

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
	while ( *start > 0 && *start < 0x20 ) ++start;
	while ( end >= start && *end >= 0 && *end < 0x20 ) --end;
	memcpy( (void *) val.get_ptr(), start, end - start + 1 );
	val.truncate( end - start + 1 );
}

static int info_read(const BYTE * ptr, int len, file_info & info, int inherit, unsigned & tag_refresh, int & tag_song_ms, int & tag_fade_ms)
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
			if ( line_end == ~0 ) line_end = len;
			tag.set_string( whole_tag.get_ptr() + pos, line_end - pos );
			pos = line_end;
			line_end = tag.find_first( '=' );
			if ( line_end == ~0 ) continue;
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
					if ( !tag_refresh )
					{
						tag_refresh = atoi( value );
					}
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
				else if (!stricmp_utf8(tag, "_refresh"))
				{
					if ( !tag_refresh )
					{
						tag_refresh = atoi( value );
					}
				}
				else if ( tag[0] == '_' )
				{
					console::formatter() << "Unsupported tag found: " << tag << ", required to play file.";
					return -1;
				}
			}
		}
	}
	if (!utf8) info_meta_ansi( info );
	return precede;
}

static int EMU_CALL virtual_readfile(void *context, const char *path, int offset, char *buffer, int length)
{
	return psf2fs_virtual_readfile(context, path, offset, buffer, length);
}

static void EMU_CALL virtual_console_out(void * context, char c)
{
	pfc::string_base * out = reinterpret_cast<pfc::string_base *>(context);
	if (c == 13 || c == 10)
	{
		if (out->length())
		{
			console::info(out->get_ptr());
			out->reset();
		}
	}
	else
		out->add_char(c);
}

static int load_exe_unpack(void *state, const BYTE *data, uLong srclen, int inherit, unsigned & refresh)
{
	DBG("loading");
	int err;
	pfc::array_t<t_uint8> buf;
	buf.set_size( 0x1F0800 );
	BYTE *ptr = buf.get_ptr();

	uLong destlen = 0x1F0800;
	uncompress(ptr, &destlen, data, srclen);
	// ScrubFormat checks this, but I already did this :)
	if ( destlen < 0x800 )
	{
		memset( ptr + destlen, 0, 0x800 - destlen );
		destlen = 0x800;
	}
	DBG("header in");

	buf.set_size( destlen );
	ptr = buf.get_ptr();

	psxexe_hdr_t *psx = (psxexe_hdr_t *) ptr;
	uLong t_addr = pfc::byteswap_if_be_t( psx->exec.t_addr );
	uLong t_size = destlen - 0x800; // pfc::byteswap_if_be_t( psx->exec.t_size );

	// ScrubFormat checks total EXE size > 0x1f0800, but I try to carry it a step further
	t_addr &= 0x1FFFFF;
	if ((t_addr < 0x10000) ||
		(t_size > 0x1F0000) ||
		(t_addr + t_size > 0x200000)) return 0;

	if (inherit < 1 && !refresh)
	{
		if (!strnicmp((const char *) ptr + 113, "Japan", 5)) refresh = 60;
		else if (!strnicmp((const char *) ptr + 113, "Europe", 6)) refresh = 50;
		else if (!strnicmp((const char *) ptr + 113, "North America", 13)) refresh = 60;
	}

	/*destlen += t_size;

	buf.set_size( destlen );
	ptr = buf.get_ptr();

	err = uncompress(ptr, &destlen, data, srclen);

	if (destlen < 2048 + t_size)
	{
		if (err != 0) return 0;
		// Part of the ScrubFormat stupidity checking and correction system.
		memset(ptr + destlen, 0, 2048 + t_size - destlen);
	}*/
	DBG("executable loaded");

	void * pIOP = psx_get_iop_state(state);
	iop_upload_to_ram(pIOP, t_addr, ptr + 2048, t_size);

	if (inherit)
	{
		void * pR3000 = iop_get_r3000_state(pIOP);
		psx = (psxexe_hdr_t*) ptr;
		r3000_setreg(pR3000, R3000_REG_PC, pfc::byteswap_if_be_t( psx->exec.pc0 ) );
		r3000_setreg(pR3000, R3000_REG_GEN+29, pfc::byteswap_if_be_t( psx->exec.s_ptr ) );
		DBG("registers loaded");
	}

	return 1;
}

class load_exe_recursive
{
	file_info_impl   internal;

	void             * state;
	abort_callback   & m_abort;
	const char       * base_path;
	const char       * filename;
	pfc::string_base & errors;
	unsigned         & refresh;
	unsigned         & tag_refresh;

public:
	load_exe_recursive(void * p_state, service_ptr_t<file> & p_reader, const char * p_path, file_info & p_info, const char * p_base_path, pfc::string_base & p_errors, int inherit, unsigned & p_refresh, unsigned & p_tag_refresh, abort_callback & p_abort)
	: state(p_state), m_abort(p_abort), base_path(p_base_path), filename( p_path ), errors(p_errors), refresh(p_refresh), tag_refresh(p_tag_refresh)
	{
		if (!load(p_reader, p_info, inherit)) throw exception_io_data();
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
			info.info_set("codec", "PSF");
			info.info_set( "encoding", "synthesized" );

			if (inherit != -2)
			{
				//internal.reset();

				psx_clear_state(state, 1);
				if (cfg_info) psx_set_console_out(state, virtual_console_out, &errors);

				void *pIOP = psx_get_iop_state(state);
				iop_set_compat(pIOP, cfg_compat);
				spu_enable_reverb(iop_get_spu_state(pIOP), cfg_reverb);
			}
		}

		int tag_song_ms = 0, tag_fade_ms = 0;

		if (size < 16 + reserved_size + exe_size + 5)
		{
			DBG("loading executable only");
			// load executable only
			info.set_length((double)(tag_song_ms+tag_fade_ms)*.001);
			if (inherit == -2) return 1;
			return load_exe_unpack(state, ptr, exe_size, inherit, refresh);
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
				precede = info_read(ptr, len, info, inherit, tag_refresh, tag_song_ms, tag_fade_ms);
				if (precede < 0) return 0;
			}
		}

		if (!tag_refresh)
		{
			const char * ref = info.info_get("_refresh");
			if (ref) tag_refresh = atoi(ref);
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

		rtn = load_exe_unpack(state, buf.get_ptr(), exe_size, inherit * (1-precede), refresh);
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

class input_psf
{
	bool no_loop, eof;

	pfc::array_t<t_uint8> psx_state;
	pfc::array_t<t_int16> sample_buffer;

	circular_buffer<t_int16> silence_test_buffer;

	void * psf2fs;

	service_ptr_t<file> m_file;

	CPSXFilter filter;

	pfc::string8 base_path;
	pfc::string8 filename;

	pfc::string8 errors;

	int err;

	int psf_version;

	int data_written,remainder,pos_delta,startsilence,silence;

	double psfemu_pos;

	int song_len,fade_len;
	int tag_song_ms,tag_fade_ms;

	file_info_impl m_info;

	bool do_filter, do_suppressendsilence;

	int load_psf(service_ptr_t<file> & r, const char * p_path, file_info & info, bool full_open, abort_callback & p_abort);

public:
	input_psf() : silence_test_buffer( 0 )
	{
		psf2fs = NULL;
	}

	~input_psf()
	{
		if (psf2fs) psf2fs_delete(psf2fs);
		if (errors.length()) console::info(errors);
	}

	void open( service_ptr_t<file> p_file, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		if ( p_file.is_empty() )
		{
			filesystem::g_open( p_file, p_path, p_reason == input_open_info_write ? filesystem::open_mode_write_existing : filesystem::open_mode_read, p_abort );
		}

		{
			psf_version = load_psf( p_file, p_path, m_info, false, p_abort );

			if (psf_version == 1) InterlockedIncrement(&psf_count);
			else if (psf_version == 2) InterlockedIncrement(&psf2_count);

			tag_song_ms = 0;
			tag_fade_ms = 0;

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
				DBG("psx_init()");
				if (psx_init()) throw std::exception("PSX emulator static initialization failed");
				initialized = 1;
			}
		}

		psx_state.set_size( psx_get_state_size( psf_version ) );

		void * pEmu = psx_state.get_ptr();

		m_info.reset();

		{
			pfc::string8 path = base_path;
			path += filename;
			err = load_psf( m_file, path, m_info, true, p_abort );
		}

		psfemu_pos = 0.;

		startsilence = silence = 0;

		eof = 0;
		err = 0;
		data_written = 0;
		remainder = 0;
		pos_delta = 0;
		psfemu_pos = 0;
		no_loop = ( p_flags & input_flag_no_looping ) || !cfg_infinite;

		calcfade();

		do_filter = !! cfg_freq;
		do_suppressendsilence = !! cfg_suppressendsilence;

		unsigned skip_max = cfg_endsilenceseconds * ( ( psf_version == 2 ) ? 48000 : 44100 );

		if ( cfg_suppressopeningsilence ) // ohcrap
		{
			if ( psf2fs ) psf2fs_setabortcallback( psf2fs, p_abort );

			for (;;)
			{
				p_abort.check();

				unsigned skip_howmany = skip_max - silence;
				if ( skip_howmany > 1024 ) skip_howmany = 1024;
				sample_buffer.grow_size( skip_howmany * 2 );
				int rtn = psx_execute( pEmu, 0x7FFFFFFF, sample_buffer.get_ptr(), & skip_howmany, 0 );
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
				if ( silence >= skip_max )
				{
					eof = true;
					break;
				}
			}

			startsilence += silence;
			silence = 0;
		}

		if ( do_suppressendsilence ) silence_test_buffer.resize( skip_max * 2 );

		if ( do_filter ) filter.Redesign( ( psf_version == 2 ) ? 48000 : 44100 );
	}

	bool decode_run( audio_chunk & p_chunk, abort_callback & p_abort )
	{
		int srate;

		if ( ( eof || err < 0 ) && !silence_test_buffer.data_available() ) return false;

		srate = ( psf_version == 2 ) ? 48000 : 44100;

		if ( no_loop && tag_song_ms && ( pos_delta + MulDiv( data_written, 1000, srate ) ) >= tag_song_ms + tag_fade_ms )
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

		if ( psf2fs ) psf2fs_setabortcallback( psf2fs, p_abort );

		if ( do_suppressendsilence )
		{
			sample_buffer.grow_size( 1024 * 2 );

			if ( !eof )
			{
				unsigned free_space = silence_test_buffer.free_space() / 2;
				while ( free_space )
				{
					p_abort.check();

					unsigned samples_to_render;
					if ( remainder )
					{
						samples_to_render = remainder;
						remainder = 0;
					}
					else
					{
						samples_to_render = free_space;
						if ( samples_to_render > 1024 ) samples_to_render = 1024;
						err = psx_execute( psx_state.get_ptr(), 0x7FFFFFFF, sample_buffer.get_ptr(), & samples_to_render, 0 );
						if ( err == -2 ) console::print( "Execution halted with an error." );
						if ( !samples_to_render ) throw exception_io_data();
					}
					silence_test_buffer.write( sample_buffer.get_ptr(), samples_to_render * 2 );
					free_space -= samples_to_render;
					if ( err < 0 )
					{
						eof = true;
						break;
					}
				}
			}

			if ( silence_test_buffer.test_silence() )
			{
				eof = true;
				return false;
			}

			written = silence_test_buffer.data_available() / 2;
			if ( written > samples ) written = samples;
			silence_test_buffer.read( sample_buffer.get_ptr(), written * 2 );
		}
		else
		{
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
				err = psx_execute( psx_state.get_ptr(), 0x7FFFFFFF, sample_buffer.get_ptr(), & written, 0 );
				if ( err == -2 ) console::print( "Execution halted with an error." );
				if ( !written ) throw exception_io_data();
				if ( err < 0 ) eof = true;
			}
		}

		psfemu_pos += double( written ) / double( srate );

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

		p_chunk.set_data_fixedpoint( sample_buffer.get_ptr(), written * 4, srate, 2, 16, audio_chunk::channel_config_stereo );

		if ( do_filter ) filter.Process( p_chunk.get_data(), p_chunk.get_sample_count() );

		return true;
	}

	void decode_seek( double p_seconds, abort_callback & p_abort )
	{
		eof = false;

		silence_test_buffer.reset();

		if ( do_filter ) filter.Reset();

		if ( psf2fs ) psf2fs_setabortcallback( psf2fs, p_abort );

		void *pEmu = psx_state.get_ptr();
		if ( p_seconds < psfemu_pos )
		{
			//file_info *temp = new file_info_i_full;
			file_info_impl temp;
			pfc::string8 path = base_path;
			path += filename;
			if (psf_version == 2)
			{
				psf2fs_delete( psf2fs );
			}

			//try
			{
				load_psf( m_file, path, temp, true, p_abort );
			}
			//catch(exception_io const & e) {return e.get_code();}
			psfemu_pos = 0.;
			if ( startsilence )
			{
				unsigned int silence = startsilence;
				while ( silence )
				{
					p_abort.check();

					unsigned int todo = silence;
					int err = psx_execute( pEmu, 0x7FFFFFFF, 0, & todo, 0 );
					if ( err < 0 )
					{
						eof = true;
						return;
					}
					silence -= todo;
				}
			}
		}
		unsigned int howmany = ( int )( audio_math::time_to_samples( p_seconds - psfemu_pos, psf_version == 2 ? 48000 : 44100 ) );


		// more abortable, and emu doesn't like doing huge numbers of samples per call anyway
		while ( howmany )
		{
			p_abort.check();

			unsigned todo = howmany;
			if ( todo > 2048 ) todo = 2048;
			int rtn = psx_execute( pEmu, 0x7FFFFFFF, 0, & todo, 0 );
			if ( rtn < 0 || ! todo )
			{
				eof = true;
				return;
			}
			howmany -= todo;
		}

		data_written = 0;
		pos_delta = ( int )( p_seconds * 1000. );
		psfemu_pos = p_seconds;

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
		return (!stricmp(p_extension,"psf") || !stricmp(p_extension,"minipsf") ||
				!stricmp(p_extension,"psf2") || !stricmp(p_extension, "minipsf2"));
	}

private:
	void calcfade()
	{
		song_len=MulDiv(tag_song_ms-pos_delta,(psf_version == 2 ? 48000 : 44100),1000);
		fade_len=MulDiv(tag_fade_ms,(psf_version == 2 ? 48000 : 44100),1000);
	}
};

int input_psf::load_psf(service_ptr_t<file> & r, const char * p_path, file_info & info, bool full_open, abort_callback & p_abort)
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

	if (header[3] == 1)
	{
		unsigned refresh = 0, tag_refresh = 0;

		if (!full_open)
		{
			load_exe_recursive(0, r, p_path, info, 0, pfc::string8(), -2, refresh, tag_refresh, p_abort);
			return 1;
		}
		
		load_exe_recursive(psx_state.get_ptr(), r, p_path, info, base_path, errors, -1, refresh, tag_refresh, p_abort);

		if (refresh || tag_refresh)
		{
			if (tag_refresh) refresh = tag_refresh;
			psx_set_refresh(psx_state.get_ptr(), refresh);
		}

		return 1;
	}
	else if (header[3] == 2)
	{
		int reserved_size;
		int program_size;
		int tag_ofs;
		unsigned tag_bytes;
		unsigned tag_refresh;

		reserved_size =
			((((int)(header[ 4])) & 0xFF) <<  0) |
			((((int)(header[ 5])) & 0xFF) <<  8) |
			((((int)(header[ 6])) & 0xFF) << 16) |
			((((int)(header[ 7])) & 0xFF) << 24);
		program_size =
			((((int)(header[ 8])) & 0xFF) <<  0) |
			((((int)(header[ 9])) & 0xFF) <<  8) |
			((((int)(header[10])) & 0xFF) << 16) |
			((((int)(header[11])) & 0xFF) << 24);

		if ((reserved_size < 0) ||
			(program_size < 0) ||
			(reserved_size > fl) ||
			(program_size > fl)) throw exception_io_data();

		tag_ofs = 16 + reserved_size + program_size;

		if ((tag_ofs < 16) ||
			(tag_ofs > fl)) throw exception_io_data();

		info.info_set_int("samplerate", 48000);
		info.info_set_int("bitspersample", 16);
		info.info_set_int("channels", 2);
		info.info_set("codec", "PSF2");
		info.info_set( "encoding", "synthesized" );

		tag_bytes = fl - (tag_ofs + 5);
		if (tag_bytes > 50000) tag_bytes = 50000;
		if (tag_bytes > 0)
		{
			pfc::array_t<t_uint8> tag;
			tag.set_size( tag_bytes + 6 );
			unsigned char * ptr = tag.get_ptr();
			r->seek(tag_ofs, p_abort);
			if (r->read(ptr, 5, p_abort) == 5)
			{
				if (!memcmp(ptr, "[TAG]", 5))
				{
					if (r->read(ptr + 5, tag_bytes, p_abort) == tag_bytes)
					{
						ptr[tag_bytes + 5] = 0;
						if (info_read(ptr, tag_bytes + 5, info, -1, tag_refresh, tag_song_ms, tag_fade_ms) < 0)
						{
							throw exception_io_data();
						}
					}
				}
			}
		}

		if (tag_refresh) psx_set_refresh(psx_state.get_ptr(), tag_refresh);

		DBG("setting length");
		if (!tag_song_ms)
		{
			tag_song_ms = cfg_deflength;
			tag_fade_ms = cfg_deffade;
		}
		info.set_length((double)(tag_song_ms+tag_fade_ms)*.001);

		if (!full_open) return 2;

		psx_clear_state(psx_state.get_ptr(), 2);

		if (cfg_info) psx_set_console_out(psx_state.get_ptr(), virtual_console_out, (void*) &errors);

		psf2fs = psf2fs_create();

		psf2fs_setabortcallback(psf2fs, p_abort);
		int ret = psf2fs_addarchive(psf2fs, p_path, 2);

		if (ret < 0)
		{
			console::info(psf2fs_getlasterror(psf2fs));
			return 0;
		}

		psx_set_readfile(psx_state.get_ptr(), virtual_readfile, psf2fs);

		return 2;
	}

	throw exception_io_data();
}

class CMyPreferences : public CDialogImpl<CMyPreferences>, public preferences_page_instance {
public:
	//Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
	CMyPreferences(preferences_page_callback::ptr callback) : m_callback(callback) {}

	//Note that we don't bother doing anything regarding destruction of our class.
	//The host ensures that our dialog is destroyed first, then the last reference to our preferences_page_instance object is released, causing our object to be deleted.


	//dialog resource ID
	enum {IDD = IDD_PSF_CONFIG};
	// preferences_page_instance methods (not all of them - get_wnd() is supplied by preferences_page_impl helpers)
	t_uint32 get_state();
	void apply();
	void reset();

	//WTL message map
	BEGIN_MSG_MAP(CMyPreferences)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_INDEFINITE, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_SOS, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_SES, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_REVERB, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_FREQ, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_INFO, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_HARSH, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_FRIENDLY, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_SILENCE, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_DLENGTH, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_DFADE, EN_CHANGE, OnEditChange)
	END_MSG_MAP()
private:
	BOOL OnInitDialog(CWindow, LPARAM);
	void OnEditChange(UINT, int, CWindow);
	void OnButtonClick(UINT, int, CWindow);
	bool HasChanged();
	void OnChanged();

	const preferences_page_callback::ptr m_callback;

	CHyperLink m_link_neill;
	CHyperLink m_link_kode54;
};

BOOL CMyPreferences::OnInitDialog(CWindow, LPARAM) {
	int n;
	SendDlgItemMessage( IDC_INDEFINITE, BM_SETCHECK, cfg_infinite );
	SendDlgItemMessage( IDC_SOS, BM_SETCHECK, cfg_suppressopeningsilence );
	SendDlgItemMessage( IDC_SES, BM_SETCHECK, cfg_suppressendsilence );
	SendDlgItemMessage( IDC_REVERB, BM_SETCHECK, cfg_reverb );
	SendDlgItemMessage( IDC_FREQ, BM_SETCHECK, cfg_freq );
	SendDlgItemMessage( IDC_INFO, BM_SETCHECK, cfg_info );
	n = IDC_FRIENDLY;
	if (cfg_compat == IOP_COMPAT_HARSH) n = IDC_HARSH;
	CheckRadioButton( IDC_HARSH, IDC_FRIENDLY, n );
	
	SetDlgItemInt( IDC_SILENCE, cfg_endsilenceseconds, FALSE );
	
	{
		char temp[16];
		// wsprintf((char *)&temp, "= %d Hz", 33868800 / cfg_divider);
		// SetDlgItemText(wnd, IDC_HZ, (char *)&temp);
		
		print_time_crap( cfg_deflength, (char *)&temp );
		uSetDlgItemText( m_hWnd, IDC_DLENGTH, (char *)&temp );
		
		print_time_crap( cfg_deffade, (char *)&temp );
		uSetDlgItemText( m_hWnd, IDC_DFADE, (char *)&temp );
	}
	
	m_link_neill.SetLabel( _T( "Neill Corlett's Home Page" ) );
	m_link_neill.SetHyperLink( _T( "http://www.neillcorlett.com/" ) );
	m_link_neill.SubclassWindow( GetDlgItem( IDC_URL ) );
	
	m_link_kode54.SetLabel( _T( "kode's foobar2000 plug-ins" ) );
	m_link_kode54.SetHyperLink( _T( "http://kode54.foobar2000.org/" ) );
	m_link_kode54.SubclassWindow( GetDlgItem( IDC_K54 ) );
	
	{
		/*OSVERSIONINFO ovi = { 0 };
		ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		BOOL bRet = ::GetVersionEx(&ovi);
		if ( bRet && ( ovi.dwMajorVersion >= 5 ) )*/
		{
			DWORD color = GetSysColor( 26 /* COLOR_HOTLIGHT */ );
			m_link_neill.m_clrLink = color;
			m_link_neill.m_clrVisited = color;
			m_link_kode54.m_clrLink = color;
			m_link_kode54.m_clrVisited = color;
		}
	}
	
	unsigned long psfc = psf_count;
	unsigned long psf2c = psf2_count;

	if (psf2c)
	{
		__int64 total = ((__int64)psfc + (__int64)psf2c) - 1;
		__int64 meh = ((__int64)rand() * total) / RAND_MAX;
		if (meh >= (__int64)psfc)
		{
			SendDlgItemMessage( IDC_LOGO, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) LoadImage( core_api::get_my_instance(), MAKEINTRESOURCE(IDB_LOGOBMP2), IMAGE_BITMAP, 0, 0, 0 ) );
		}
	}
	
	return FALSE;
}

void CMyPreferences::OnEditChange(UINT, int, CWindow) {
	OnChanged();
}

void CMyPreferences::OnButtonClick(UINT, int, CWindow) {
	OnChanged();
}

t_uint32 CMyPreferences::get_state() {
	t_uint32 state = preferences_state::resettable;
	if (HasChanged()) state |= preferences_state::changed;
	return state;
}

void CMyPreferences::reset() {
	int n;
	char temp[16];
	SendDlgItemMessage( IDC_INDEFINITE, BM_SETCHECK, default_cfg_infinite );
	SendDlgItemMessage( IDC_SOS, BM_SETCHECK, default_cfg_suppressopeningsilence );
	SendDlgItemMessage( IDC_SES, BM_SETCHECK, default_cfg_suppressendsilence );
	SendDlgItemMessage( IDC_REVERB, BM_SETCHECK, default_cfg_reverb );
	SendDlgItemMessage( IDC_FREQ, BM_SETCHECK, default_cfg_freq );
	SendDlgItemMessage( IDC_INFO, BM_SETCHECK, default_cfg_info );
	n = IDC_FRIENDLY;
	if (default_cfg_compat == IOP_COMPAT_HARSH) n = IDC_HARSH;
	CheckRadioButton( IDC_HARSH, IDC_FRIENDLY, n );
	SetDlgItemInt( IDC_SILENCE, default_cfg_endsilenceseconds, FALSE );
	print_time_crap( default_cfg_deflength, (char *)&temp );
	uSetDlgItemText( m_hWnd, IDC_DLENGTH, (char *)&temp );
	print_time_crap( default_cfg_deffade, (char *)&temp );
	uSetDlgItemText( m_hWnd, IDC_DFADE, (char *)&temp );
	
	OnChanged();
}

void CMyPreferences::apply() {
	int t;
	char temp[16];
	cfg_infinite = SendDlgItemMessage( IDC_INDEFINITE, BM_GETCHECK );
	cfg_suppressopeningsilence = SendDlgItemMessage( IDC_SOS, BM_GETCHECK );
	cfg_suppressendsilence = SendDlgItemMessage( IDC_SES, BM_GETCHECK );
	cfg_reverb = SendDlgItemMessage( IDC_REVERB, BM_GETCHECK );
	cfg_freq = SendDlgItemMessage( IDC_FREQ, BM_GETCHECK );
	cfg_info = SendDlgItemMessage( IDC_INFO, BM_GETCHECK );
	cfg_compat = SendDlgItemMessage( IDC_FRIENDLY, BM_GETCHECK ) ? IOP_COMPAT_FRIENDLY : IOP_COMPAT_HARSH;
	t = GetDlgItemInt( IDC_SILENCE, NULL, FALSE );
	if ( t > 0 ) cfg_endsilenceseconds = t;
	SetDlgItemInt( IDC_SILENCE, cfg_endsilenceseconds, FALSE );
	t = parse_time_crap( string_utf8_from_window( GetDlgItem( IDC_DLENGTH ) ) );
	if ( t != BORK_TIME ) cfg_deflength = t;
	else
	{
		print_time_crap( cfg_deflength, (char *)&temp );
		uSetDlgItemText( m_hWnd, IDC_DLENGTH, (char *)&temp );
	}
	t = parse_time_crap( string_utf8_from_window( GetDlgItem( IDC_DFADE ) ) );
	if ( t != BORK_TIME ) cfg_deffade = t;
	else
	{
		print_time_crap( cfg_deffade, (char *)&temp );
		uSetDlgItemText( m_hWnd, IDC_DFADE, (char *)&temp );
	}
	
	OnChanged(); //our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
}

bool CMyPreferences::HasChanged() {
	//returns whether our dialog content is different from the current configuration (whether the apply button should be enabled or not)
	bool changed = false;
	if ( !changed && SendDlgItemMessage( IDC_INDEFINITE, BM_GETCHECK ) != cfg_infinite ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_SOS, BM_GETCHECK ) != cfg_suppressopeningsilence ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_SES, BM_GETCHECK ) != cfg_suppressendsilence ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_REVERB, BM_GETCHECK ) != cfg_reverb ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_FREQ, BM_GETCHECK ) != cfg_freq ) changed = true;
	if ( !changed && SendDlgItemMessage( IDC_INFO, BM_GETCHECK ) != cfg_info ) changed = true;
	if ( !changed )
	{
		int n = IDC_FRIENDLY;
		if (cfg_compat == IOP_COMPAT_HARSH) n = IDC_HARSH;
		if ( !SendDlgItemMessage( n, BM_GETCHECK ) ) changed = true;
	}
	if ( !changed && GetDlgItemInt( IDC_SILENCE, NULL, FALSE ) != cfg_endsilenceseconds ) changed = true;
	if ( !changed )
	{
		int t = parse_time_crap( string_utf8_from_window( GetDlgItem( IDC_DLENGTH ) ) );
		if ( t != BORK_TIME && t != cfg_deflength ) changed = true;
	}
	if ( !changed )
	{
		int t = parse_time_crap( string_utf8_from_window( GetDlgItem( IDC_DFADE ) ) );
		if ( t != BORK_TIME && t != cfg_deffade ) changed = true;
	}
	return changed;
}
void CMyPreferences::OnChanged() {
	//tell the host that our state has changed to enable/disable the apply button appropriately.
	m_callback->on_state_changed();
}

class preferences_page_myimpl : public preferences_page_impl<CMyPreferences> {
	// preferences_page_impl<> helper deals with instantiation of our dialog; inherits from preferences_page_v3.
public:
	const char * get_name() {return "PSF Decoder";}
	GUID get_guid() {
		// {E4A73418-437F-4fad-849F-7F25DCFF1AE3}
		static const GUID guid = { 0xe4a73418, 0x437f, 0x4fad, { 0x84, 0x9f, 0x7f, 0x25, 0xdc, 0xff, 0x1a, 0xe3 } };
		return guid;
	}
	GUID get_parent_guid() {return guid_input;}
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
			if ( i->song != ~0 )
			{
				print_time_crap(i->song, (char*)&temp);
				uSetDlgItemText(wnd, IDC_LENGTH, (char*)&temp);
			}
			if ( i->fade != ~0 )
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
				else i->song = ~0;
				foo = parse_time_crap(string_utf8_from_window(wnd, IDC_FADE));
				if (foo != BORK_TIME) i->fade = foo;
				else i->fade = ~0;
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

class length_info_filter : public file_info_filter
{
	bool set_length, set_fade;
	unsigned m_length, m_fade;

	metadb_handle_list m_handles;

public:
	length_info_filter( const pfc::list_base_const_t<metadb_handle_ptr> & p_list )
	{
		set_length = false;
		set_fade = false;

		pfc::array_t<t_size> order;
		order.set_size(p_list.get_count());
		order_helper::g_fill(order.get_ptr(),order.get_size());
		p_list.sort_get_permutation_t(pfc::compare_t<metadb_handle_ptr,metadb_handle_ptr>,order.get_ptr());
		m_handles.set_count(order.get_size());
		for(t_size n = 0; n < order.get_size(); n++) {
			m_handles[n] = p_list[order[n]];
		}

	}

	void length( unsigned p_length )
	{
		set_length = true;
		m_length = p_length;
	}

	void fade( unsigned p_fade )
	{
		set_fade = true;
		m_fade = p_fade;
	}

	virtual bool apply_filter(metadb_handle_ptr p_location,t_filestats p_stats,file_info & p_info)
	{
		t_size index;
		if (m_handles.bsearch_t(pfc::compare_t<metadb_handle_ptr,metadb_handle_ptr>,p_location,index))
		{
			if ( set_length )
			{
				if ( m_length ) p_info.info_set_int( field_length, m_length );
				else p_info.info_remove( field_length );
			}
			if ( set_fade )
			{
				if ( m_fade ) p_info.info_set_int( field_fade, m_fade );
				else p_info.info_remove( field_fade );
			}
			return set_length | set_fade;
		}
		else
		{
			return false;
		}
	}
};

class context_psf : public contextmenu_item_simple
{
public:
	virtual unsigned get_num_items() { return 1; }

	virtual void get_item_name(unsigned n, pfc::string_base & out)
	{
		if (n) uBugCheck();
		out = "Edit length";
	}

	/*virtual void get_item_default_path(unsigned n, pfc::string_base & out)
	{
		out.reset();
	}*/
	GUID get_parent() {return contextmenu_groups::tagging;}

	virtual bool get_item_description(unsigned n, pfc::string_base & out)
	{
		if (n) uBugCheck();
		out = "Edits the length of the selected PSF file, or sets the length of all selected PSF files.";
		return true;
	}

	virtual GUID get_item_guid(unsigned p_index)
	{
		if (p_index) uBugCheck();
		static const GUID guid = { 0x37c4555, 0xabd6, 0x4524, { 0x97, 0xa9, 0xe4, 0x46, 0x50, 0x71, 0x20, 0xab } };
		return guid;
	}

	virtual bool context_get_display(unsigned n,const pfc::list_base_const_t<metadb_handle_ptr> & data,pfc::string_base & out,unsigned & displayflags,const GUID &)
	{
		if (n) uBugCheck();
		unsigned i, j;
		i = data.get_count();
		for (j = 0; j < i; j++)
		{
			pfc::string_extension ext(data.get_item(j)->get_path());
			int c1 = strnicmp(ext, "PSF", 3);
			int c2 = strnicmp(ext, "MINIPSF", 7);
			if (c1 && c2) return false;
			if ((!c1 && ext[3] != 0 && (ext[3] != '2' || ext[4] != 0)) ||
				(!c2 && ext[7] != 0 && (ext[7] != '2' || ext[8] != 0))) return false;
		}
		if (i == 1) out = "Edit length";
		else out = "Set length";
		return true;
	}

	virtual void context_command(unsigned n,const pfc::list_base_const_t<metadb_handle_ptr> & data,const GUID& caller)
	{
		if (n) uBugCheck();
		unsigned tag_song_ms = ~0, tag_fade_ms = ~0;
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
		static_api_ptr_t<metadb_io_v2> p_imgr;

		service_ptr_t<length_info_filter> p_filter = new service_impl_t< length_info_filter >( data );
		if ( tag_song_ms != ~0 ) p_filter->length( tag_song_ms );
		if ( tag_fade_ms != ~0 ) p_filter->fade( tag_fade_ms );

		p_imgr->update_info_async( data, p_filter, core_api::get_main_window(), 0, 0 );
	}
};

class psf_file_types : public input_file_type
{
	virtual unsigned get_count()
	{
		return 2;
	}

	virtual bool get_name(unsigned idx, pfc::string_base & out)
	{
		if (idx > 1) return false;
		else if (idx == 0) out = "PSF files";
		else if (idx == 1) out = "PSF2 files";
		return true;
	}

	virtual bool get_mask(unsigned idx, pfc::string_base & out)
	{
		if (idx > 1) return false;
		else if (idx == 0) out = "*.PSF;*.MINIPSF";
		else if (idx == 1) out = "*.PSF2;*.MINIPSF2";
		return true;
	}

	virtual bool is_associatable(unsigned idx)
	{
		return true;
	}
};

class version_psf : public componentversion
{
public:
	virtual void get_file_name(pfc::string_base & out) { out = core_api::get_my_file_name(); }
	virtual void get_component_name(pfc::string_base & out) { out = "Highly Experimental"; }
	virtual void get_component_version(pfc::string_base & out) { out = MYVERSION; }
	virtual void get_about_message(pfc::string_base & out)
	{
		out = "Foobar2000 version by kode54\nOriginal library and concept by Neill Corlett" /*"\n\nCore: ";
		out += psx_getversion();
		out +=*/ "\n\nhttp://www.neillcorlett.com/\nhttp://kode54.foobar2000.org/";
	}
};

static input_singletrack_factory_t<input_psf>               g_input_psf_factory;
static preferences_page_factory_t <preferences_page_myimpl> g_config_psf_factory;
static contextmenu_item_factory_t <context_psf>             g_contextmenu_item_psf_factory;
static service_factory_single_t   <psf_file_types>          g_input_file_type_psf_factory;
static service_factory_single_t   <version_psf>             g_componentversion_psf_factory;

VALIDATE_COMPONENT_FILENAME("foo_psf.dll");
