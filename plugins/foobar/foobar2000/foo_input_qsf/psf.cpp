#define MYVERSION "2.0.22"

/*
	changelog

2012-12-27 17:16 UTC - kode54
- Replaced bulky PSF loading code with new psflib
- Version is now 2.0.22

2012-12-22 03:04 UTC - kode54
- Added support for multi-value fields
- Version is now 2.0.21

2012-02-19 19:53 UTC - kode54
- Added abort check to decoder
- Version is now 2.0.20

2011-01-26 04:04 UTC - kode54
- Fixed playback initialization for when files do not specify a
  fade time
- Version is now 2.0.19

2010-11-20 21:13 UTC - kode54
- Changed zlib dependency to use standard zlib1.dll
- Version is now 2.0.18

2010-07-13 03:03 UTC - kode54
- Implemented better end silence detection
- Version is now 2.0.17

2010-04-13 14:56 UTC - kode54
- Amended preferences WM_INITDIALOG handler
- Version is now 2.0.16

2010-03-19 16:59 UTC - kode54
- Updated zlib to version 1.2.4
- Version is now 2.0.15

2010-03-15 12:13 UTC - kode54
- Fixed Unicode tag reading
- Version is now 2.0.14

2010-01-13 00:48 UTC - kode54
- Updated context menu handler
- Version is now 2.0.12

2010-01-11 12:25 UTC - kode54
- Updated preferences page to 1.0 API
- Version is now 2.0.11

2009-08-17 00:14 UTC - kode54
- Fixed tag writer for correct QSF version number
- Version is now 2.0.10

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

#include "../../../ESP/QSound/Core/qsound.h"

#include <psflib.h>

#include "circular_buffer.h"

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

// {DE8827F1-4F5A-4784-9227-E233D06FFD02}
static const GUID guid_cfg_infinite = 
{ 0xde8827f1, 0x4f5a, 0x4784, { 0x92, 0x27, 0xe2, 0x33, 0xd0, 0x6f, 0xfd, 0x2 } };
// {A760F5FA-B488-434c-8DFD-3FC6913163F9}
static const GUID guid_cfg_deflength = 
{ 0xa760f5fa, 0xb488, 0x434c, { 0x8d, 0xfd, 0x3f, 0xc6, 0x91, 0x31, 0x63, 0xf9 } };
// {D272CFA8-ACC3-4930-AC26-56DE47AD0933}
static const GUID guid_cfg_deffade = 
{ 0xd272cfa8, 0xacc3, 0x4930, { 0xac, 0x26, 0x56, 0xde, 0x47, 0xad, 0x9, 0x33 } };
// {D6347D5A-0EEF-4437-A4B8-F2E0967F3123}
static const GUID guid_cfg_suppressopeningsilence = 
{ 0xd6347d5a, 0xeef, 0x4437, { 0xa4, 0xb8, 0xf2, 0xe0, 0x96, 0x7f, 0x31, 0x23 } };
// {E170D413-AB74-4870-A7B2-7220BA199A22}
static const GUID guid_cfg_suppressendsilence = 
{ 0xe170d413, 0xab74, 0x4870, { 0xa7, 0xb2, 0x72, 0x20, 0xba, 0x19, 0x9a, 0x22 } };
// {561DCAEA-CAB0-48af-96A8-DE79B79F2CC4}
static const GUID guid_cfg_endsilenceseconds = 
{ 0x561dcaea, 0xcab0, 0x48af, { 0x96, 0xa8, 0xde, 0x79, 0xb7, 0x9f, 0x2c, 0xc4 } };
// {9F5C9286-ACD0-4d7e-8844-79F70CDE3EC9}
static const GUID guid_cfg_placement = 
{ 0x9f5c9286, 0xacd0, 0x4d7e, { 0x88, 0x44, 0x79, 0xf7, 0xc, 0xde, 0x3e, 0xc9 } };

enum
{
	default_cfg_infinite = 0,
	default_cfg_deflength = 170000,
	default_cfg_deffade = 10000,
	default_cfg_suppressopeningsilence = 1,
	default_cfg_suppressendsilence = 1,
	default_cfg_endsilenceseconds = 5
};

static cfg_int cfg_infinite(guid_cfg_infinite,default_cfg_infinite);
static cfg_int cfg_deflength(guid_cfg_deflength,default_cfg_deflength);
static cfg_int cfg_deffade(guid_cfg_deffade,default_cfg_deffade);
static cfg_int cfg_suppressopeningsilence(guid_cfg_suppressopeningsilence,default_cfg_suppressopeningsilence);
static cfg_int cfg_suppressendsilence(guid_cfg_suppressendsilence,default_cfg_suppressendsilence);
static cfg_int cfg_endsilenceseconds(guid_cfg_endsilenceseconds,default_cfg_endsilenceseconds);
static cfg_window_placement cfg_placement(guid_cfg_placement);

static const char field_length[]="qsf_length";
static const char field_fade[]="qsf_fade";

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

static void info_meta_add(file_info & info, const char * tag, pfc::ptr_list_t< const char > const& values)
{
	t_size count = info.meta_get_count_by_name( tag );
	if ( count )
	{
		// append as another line
		pfc::string8 final = info.meta_get(tag, count - 1);
		final += "\r\n";
		final += values[0];
		info.meta_modify_value( info.meta_find( tag ), count - 1, final );
	}
	else
	{
		info.meta_add(tag, values[0]);
	}
	for ( count = 1; count < values.get_count(); count++ )
	{
		info.meta_add( tag, values[count] );
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

static const char * fields_to_split[] = {"ARTIST", "ALBUM ARTIST", "PRODUCER", "COMPOSER", "PERFORMER", "GENRE"};

static bool meta_split_value( const char * tag )
{
	for ( unsigned i = 0; i < _countof( fields_to_split ); i++ )
	{
		if ( !stricmp_utf8( tag, fields_to_split[ i ] ) ) return true;
	}
	return false;
}

static void info_meta_write(pfc::string_base & tag, const file_info & info, const char * name, int idx, int & first)
{
	pfc::string8 v = info.meta_enum_value(idx, 0);
	if (meta_split_value(name))
	{
		t_size count = info.meta_enum_value_count(idx);
		for (t_size i = 1; i < count; i++)
		{
			v += "; ";
			v += info.meta_enum_value(idx, i);
		}
	}

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

struct psf_info_meta_state
{
	file_info * info;

	pfc::string8_fast name;

	bool utf8;

	int tag_song_ms;
	int tag_fade_ms;

	psf_info_meta_state()
		: info( 0 ), utf8( false ), tag_song_ms( 0 ), tag_fade_ms( 0 )
	{
	}
};

static int psf_info_meta(void * context, const char * name, const char * value)
{
	psf_info_meta_state * state = ( psf_info_meta_state * ) context;

	pfc::string8_fast & tag = state->name;

	tag = name;

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
		state->info->info_set_replaygain(tag, value);
	}
	else if (!stricmp_utf8(tag, "length"))
	{
		DBG("reading length");
		int temp = parse_time_crap(value);
		if (temp != BORK_TIME)
		{
			state->tag_song_ms = temp;
			state->info->info_set_int(field_length, state->tag_song_ms);
		}
	}
	else if (!stricmp_utf8(tag, "fade"))
	{
		DBG("reading fade");
		int temp = parse_time_crap(value);
		if (temp != BORK_TIME)
		{
			state->tag_fade_ms = temp;
			state->info->info_set_int(field_fade, state->tag_fade_ms);
		}
	}
	else if (!stricmp_utf8(tag, "utf8"))
	{
		state->utf8 = true;
	}
	else if (!stricmp_utf8_partial(tag, "_lib"))
	{
		DBG("found _lib");
		state->info->info_set(tag, value);
	}
	else if (!stricmp_utf8(tag, "_refresh"))
	{
		DBG("found _refresh");
		state->info->info_set(tag, value);
	}
	else if (tag[0] == '_')
	{
		DBG("found unknown required tag, failing");
		console::formatter() << "Unsupported tag found: " << tag << ", required to play file.";
		return -1;
	}
	else
	{
		state->info->meta_add( tag, value );
	}

	return 0;
}

class qsound_rom
{
public:
	struct valid_range
	{
		t_uint32 start;
		t_uint32 size;
	};

	pfc::array_t<t_uint8> m_aKey;
	pfc::array_t<valid_range> m_aKeyValid;
	pfc::array_t<t_uint8> m_aZ80ROM;
	pfc::array_t<valid_range> m_aZ80ROMValid;
	pfc::array_t<t_uint8> m_aSampleROM;
	pfc::array_t<valid_range> m_aSampleROMValid;

	qsound_rom() {}

	void superimpose_from( const qsound_rom & from )
	{
		superimpose_section_from("KEY", from.m_aKey      , from.m_aKeyValid      );
		superimpose_section_from("Z80", from.m_aZ80ROM   , from.m_aZ80ROMValid   );
		superimpose_section_from("SMP", from.m_aSampleROM, from.m_aSampleROMValid);
	}

	void upload_section( const char * section, t_uint32 start, const t_uint8 * data, t_uint32 size )
	{
		pfc::array_t<t_uint8> * pArray = NULL;
		pfc::array_t<valid_range> * pArrayValid = NULL;
		t_uint32 maxsize = 0x7FFFFFFF;

		if ( !strcmp( section, "KEY" ) ) { pArray = &m_aKey; pArrayValid = &m_aKeyValid; maxsize = 11; }
		else if ( !strcmp( section, "Z80" ) ) { pArray = &m_aZ80ROM; pArrayValid = &m_aZ80ROMValid; }
		else if ( !strcmp( section, "SMP" ) ) { pArray = &m_aSampleROM; pArrayValid = &m_aSampleROMValid; }
		else
		{
			throw foobar2000_io::exception_io_data( pfc::string_formatter() << "Unknown tag: '" << section << "'" );
		}

		if ( ( start + size ) < start )
		{
			throw foobar2000_io::exception_io_data( pfc::string_formatter() << "Section '" << section << "' is too large" );
		}

		t_uint32 newsize = start + size;
		t_uint32 oldsize = pArray->get_size();
		if ( newsize > maxsize )
		{
			throw foobar2000_io::exception_io_data( pfc::string_formatter() << "Section '" << section << "' is too large (max " << pfc::format_int( maxsize ) << " bytes)" );
		}

		if ( newsize > oldsize )
		{
			pArray->set_size( newsize );
			memset( pArray->get_ptr() + oldsize, 0, newsize - oldsize );
		}

		memcpy( pArray->get_ptr() + start, data, size );

		oldsize = pArrayValid->get_size();
		pArrayValid->set_size( oldsize + 1 );
		valid_range & range = ( pArrayValid->get_ptr() ) [ oldsize ];
		range.start = start;
		range.size = size;
	}

	void clear()
	{
		m_aKey.set_size(0);
		m_aKeyValid.set_size(0);
		m_aZ80ROM.set_size(0);
		m_aZ80ROMValid.set_size(0);
		m_aSampleROM.set_size(0);
		m_aSampleROMValid.set_size(0);
	}

private:
	void superimpose_section_from( const char * section, const pfc::array_t<t_uint8> & from, const pfc::array_t<valid_range> & fromvalid )
	{
		for ( unsigned i = 0; i < fromvalid.get_size(); i++ )
		{
			const valid_range & range = fromvalid[ i ];
			t_uint32 start = range.start;
			t_uint32 size = range.size;
			if ( ( start >= from.get_size() ) ||
				( size >= from.get_size() ) ||
				( ( start + size ) > from.get_size() ) )
			{
				throw foobar2000_io::exception_io_data( "Invalid start/size in QSoundROM internal list (program error)" );
			}

			upload_section( section, start, from.get_ptr() + start, size );
		}
	}
};

static int qsound_load(void * context, const uint8_t * exe, size_t exe_size,
                                  const uint8_t * reserved, size_t reserved_size)
{
	qsound_rom * rom = ( qsound_rom * ) context;

	for (;;)
	{
		char s[4];
		if ( exe_size < 11 ) break;
		memcpy( s, exe, 3 ); exe += 3; exe_size -= 3;
		s [3] = 0;
		uint32_t dataofs = pfc::byteswap_if_be_t( *(uint32_t*)exe ); exe += 4; exe_size -= 4;
		uint32_t datasize = pfc::byteswap_if_be_t( *(uint32_t*)exe ); exe += 4; exe_size -= 4;
		if ( datasize > exe_size )
			return -1;

		rom->upload_section( s, dataofs, exe, datasize );

		exe += datasize;
		exe_size -= datasize; 
	}

	return 0;
}

struct psf_file_state
{
	file::ptr f;
};

static void * psf_file_fopen( const char * uri )
{
	try
	{
		psf_file_state * state = new psf_file_state;
		filesystem::g_open( state->f, uri, filesystem::open_mode_read, abort_callback_dummy() );
		return state;
	}
	catch (...)
	{
		return NULL;
	}
}

static size_t psf_file_fread( void * buffer, size_t size, size_t count, void * handle )
{
	try
	{
		psf_file_state * state = ( psf_file_state * ) handle;
		size_t bytes_read = state->f->read( buffer, size * count, abort_callback_dummy() );
		return bytes_read / size;
	}
	catch (...)
	{
		return 0;
	}
}

static int psf_file_fseek( void * handle, int64_t offset, int whence )
{
	try
	{
		psf_file_state * state = ( psf_file_state * ) handle;
		state->f->seek_ex( offset, (foobar2000_io::file::t_seek_mode) whence, abort_callback_dummy() );
		return 0;
	}
	catch (...)
	{
		return -1;
	}
}

static int psf_file_fclose( void * handle )
{
	try
	{
		psf_file_state * state = ( psf_file_state * ) handle;
		delete state;
		return 0;
	}
	catch (...)
	{
		return -1;
	}
}

static long psf_file_ftell( void * handle )
{
	try
	{
		psf_file_state * state = ( psf_file_state * ) handle;
		return state->f->get_position( abort_callback_dummy() );
	}
	catch (...)
	{
		return -1;
	}
}

const psf_file_callbacks psf_file_system =
{
	"\\/|:",
	psf_file_fopen,
	psf_file_fread,
	psf_file_fseek,
	psf_file_fclose,
	psf_file_ftell
};

class input_qsf
{
	bool no_loop, eof;

	pfc::array_t<t_uint8> qsound_state;
	pfc::array_t<t_int16> sample_buffer;

	circular_buffer<t_int16> silence_test_buffer;

	qsound_rom m_rom;

	service_ptr_t<file> m_file;

	pfc::string8 m_path;

	int err;

	int data_written,remainder,pos_delta,startsilence,silence;

	double qsfemu_pos;

	int song_len,fade_len;
	int tag_song_ms,tag_fade_ms;

	file_info_impl m_info;

	bool do_filter, do_suppressendsilence;

public:
	input_qsf() : silence_test_buffer( 0 )
	{
	}

	~input_qsf()
	{
	}

	void open( service_ptr_t<file> p_file, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		input_open_file_helper( p_file, p_path, p_reason, p_abort );

		psf_info_meta_state info_state;
		info_state.info = &m_info;

		if ( psf_load( p_path, &psf_file_system, 0x41, 0, 0, psf_info_meta, &info_state ) <= 0 )
			throw exception_io_data( "Not a QSF file" );

		if ( !info_state.utf8 )
			info_meta_ansi( m_info );

		tag_song_ms = info_state.tag_song_ms;
		tag_fade_ms = info_state.tag_fade_ms;

		if (!tag_song_ms)
		{
			tag_song_ms = cfg_deflength;
			tag_fade_ms = cfg_deffade;
		}

		m_info.set_length( (double)( tag_song_ms + tag_fade_ms ) * .001 );

		m_file = p_file;
		m_path = p_path;
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
		{
			insync(g_sync);
			if (!initialized)
			{
				DBG("qsound_init()");
				if (qsound_init()) throw std::exception("QSound emulator static initialization failed");
				initialized = 1;
			}
		}

		qsound_state.set_size( qsound_get_state_size() );

		void * pEmu = qsound_state.get_ptr();

		qsound_clear_state( pEmu );

		m_rom.clear();

		if ( psf_load( m_path, &psf_file_system, 0x41, qsound_load, &m_rom, 0, 0 ) < 0 )
			throw exception_io_data( "Invalid QSF" );

		if(m_rom.m_aKey.get_size() == 11)
		{
			uint8_t * ptr = m_rom.m_aKey.get_ptr();
			uint32_t swap_key1 = pfc::byteswap_if_le_t( *( uint32_t * )( ptr +  0 ) );
			uint32_t swap_key2 = pfc::byteswap_if_le_t( *( uint32_t * )( ptr +  4 ) );
			uint32_t addr_key  = pfc::byteswap_if_le_t( *( uint16_t * )( ptr +  8 ) );
			uint8_t  xor_key   =                                      *( ptr + 10 );
			qsound_set_kabuki_key( pEmu, swap_key1, swap_key2, addr_key, xor_key );
		}
		else
		{
			qsound_set_kabuki_key( pEmu, 0, 0, 0, 0 );
		}
		qsound_set_z80_rom( pEmu, m_rom.m_aZ80ROM.get_ptr(), m_rom.m_aZ80ROM.get_size() );
		qsound_set_sample_rom( pEmu, m_rom.m_aSampleROM.get_ptr(), m_rom.m_aSampleROM.get_size() );

		qsfemu_pos = 0.;

		startsilence = silence = 0;

		eof = 0;
		err = 0;
		data_written = 0;
		remainder = 0;
		pos_delta = 0;
		qsfemu_pos = 0;
		no_loop = ( p_flags & input_flag_no_looping ) || !cfg_infinite;

		calcfade();

		do_suppressendsilence = !! cfg_suppressendsilence;

		unsigned skip_max = cfg_endsilenceseconds * 44100;

		if ( cfg_suppressopeningsilence ) // ohcrap
		{
			for (;;)
			{
				p_abort.check();

				unsigned skip_howmany = skip_max - silence;
				if ( skip_howmany > 1024 ) skip_howmany = 1024;
				sample_buffer.grow_size( skip_howmany * 2 );
				int rtn = qsound_execute( pEmu, 0x7FFFFFFF, sample_buffer.get_ptr(), & skip_howmany );
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
	}

	bool decode_run( audio_chunk & p_chunk, abort_callback & p_abort )
	{
		p_abort.check();

		if ( ( eof || err < 0 ) && !silence_test_buffer.data_available() ) return false;

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
						err = qsound_execute( qsound_state.get_ptr(), 0x7FFFFFFF, sample_buffer.get_ptr(), & samples_to_render );
						if ( err < 0 ) console::print( "Execution halted with an error." );
						if ( !samples_to_render ) throw exception_io_data();
					}
					silence_test_buffer.write( sample_buffer.get_ptr(), samples_to_render * 2 );
					free_space -= samples_to_render;
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
				err = qsound_execute( qsound_state.get_ptr(), 0x7FFFFFFF, sample_buffer.get_ptr(), & written );
				if ( err < 0 ) console::print( "Execution halted with an error." );
				if ( !written ) throw exception_io_data();
			}
		}

		qsfemu_pos += double( written ) / 44100.;

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

		silence_test_buffer.reset();

		void *pEmu = qsound_state.get_ptr();
		if ( p_seconds < qsfemu_pos )
		{
			decode_initialize( no_loop ? input_flag_no_looping : 0, p_abort );
		}
		unsigned int howmany = ( int )( audio_math::time_to_samples( p_seconds - qsfemu_pos, 44100 ) );

		// more abortable, and emu doesn't like doing huge numbers of samples per call anyway
		while ( howmany )
		{
			p_abort.check();

			unsigned todo = howmany;
			if ( todo > 2048 ) todo = 2048;
			int rtn = qsound_execute( pEmu, 0x7FFFFFFF, 0, & todo );
			if ( rtn < 0 || ! todo )
			{
				eof = true;
				return;
			}
			howmany -= todo;
		}

		data_written = 0;
		pos_delta = ( int )( p_seconds * 1000. );
		qsfemu_pos = p_seconds;

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
			ptr[3] != 0x41) throw exception_io_data();
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
		return (!stricmp(p_extension,"qsf") || !stricmp(p_extension,"miniqsf"));
	}

private:
	void calcfade()
	{
		song_len=MulDiv(tag_song_ms-pos_delta,44100,1000);
		fade_len=MulDiv(tag_fade_ms,44100,1000);
	}
};

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
	SendDlgItemMessage( IDC_INDEFINITE, BM_SETCHECK, cfg_infinite );
	SendDlgItemMessage( IDC_SOS, BM_SETCHECK, cfg_suppressopeningsilence );
	SendDlgItemMessage( IDC_SES, BM_SETCHECK, cfg_suppressendsilence );
	
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
	char temp[16];
	SendDlgItemMessage( IDC_INDEFINITE, BM_SETCHECK, default_cfg_infinite );
	SendDlgItemMessage( IDC_SOS, BM_SETCHECK, default_cfg_suppressopeningsilence );
	SendDlgItemMessage( IDC_SES, BM_SETCHECK, default_cfg_suppressendsilence );
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
	const char * get_name() {return "QSF Decoder";}
	GUID get_guid() {
		// {F8B94A52-4900-4507-BA7C-7C8EF86CE286}
		static const GUID guid = { 0xf8b94a52, 0x4900, 0x4507, { 0xba, 0x7c, 0x7c, 0x8e, 0xf8, 0x6c, 0xe2, 0x86 } };
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

class context_qsf : public contextmenu_item_simple
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
		out = "Edits the length of the selected QSF file, or sets the length of all selected QSF files.";
		return true;
	}

	virtual GUID get_item_guid(unsigned p_index)
	{
		if (p_index) uBugCheck();
		static const GUID guid = { 0x1dec5d89, 0xc62a, 0x41bb, { 0x94, 0x5a, 0xda, 0x4b, 0xdf, 0x2a, 0x13, 0xb0 } };
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
			if (stricmp_utf8(ext, "QSF") && stricmp_utf8(ext, "MINIQSF")) return false;
		}
		if (i == 1) out = "Edit length";
		else out = "Set length";
		return true;
	}

	virtual void context_command(unsigned n,const pfc::list_base_const_t<metadb_handle_ptr> & data,const GUID& caller)
	{
		if (n) uBugCheck();
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
		static_api_ptr_t<metadb_io_v2> p_imgr;

		service_ptr_t<length_info_filter> p_filter = new service_impl_t< length_info_filter >( data );
		if ( tag_song_ms != ~0 ) p_filter->length( tag_song_ms );
		if ( tag_fade_ms != ~0 ) p_filter->fade( tag_fade_ms );

		p_imgr->update_info_async( data, p_filter, core_api::get_main_window(), 0, 0 );
	}
};

class version_qsf : public componentversion
{
public:
	virtual void get_file_name(pfc::string_base & out) { out = core_api::get_my_file_name(); }
	virtual void get_component_name(pfc::string_base & out) { out = "Highly Quixotic"; }
	virtual void get_component_version(pfc::string_base & out) { out = MYVERSION; }
	virtual void get_about_message(pfc::string_base & out)
	{
		out = "Foobar2000 version by kode54\nOriginal library and concept by Neill Corlett" /*"\n\nCore: ";
		out += psx_getversion();
		out +=*/ "\n\nhttp://www.neillcorlett.com/\nhttp://kode54.foobar2000.org/";
	}
};

DECLARE_FILE_TYPE( "QSF files", "*.QSF;*.MINIQSF" );

static input_singletrack_factory_t<input_qsf>               g_input_qsf_factory;
static preferences_page_factory_t <preferences_page_myimpl> g_config_qsf_factory;
static contextmenu_item_factory_t <context_qsf>             g_contextmenu_item_qsf_factory;
static service_factory_single_t   <version_qsf>             g_componentversion_qsf_factory;

VALIDATE_COMPONENT_FILENAME("foo_input_qsf.dll");
