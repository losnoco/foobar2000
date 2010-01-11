#define MYVERSION "1.19"

/*
	change log

2004-09-09 18:06 UTC - kode54
- Fixed minor bug with ISO-8859-1 translation to UTF-8, meh
- Version is now 1.19

2004-07-23 01:36 UTC - kode54
- Disabled ISO-8859-1 fallback because iRiver sucks donkey balls
- Version is now 1.18

2004-07-22 11:37 UTC - kode54
- Disabled numeric / coded genre writing
- Version is now 1.17

2004-07-17 10:28 UTC - kode54
- Disabled comment/usertext/wtf auto description writer bullshit
- Version is now 1.16

2004-05-24 12:34 UTC - kode54
- Added automatic fallback to ISO-8859-1 where possible
- Version is now 1.15

2004-04-11 11:44 UTC - kode54
- Fixed tag_writer to return whatever id3v2_write returns instead of 0, blah.
- Version is now 1.14.2

2004-03-23 15:50 UTC - kode54
- Changed year processing to use strtoul...
- Version is now 1.14.1

2004-02-29 09:24 UTC - kode54
- ID3_FieldID reverse lookup now uses a gperf generated hash function
- id3v2_isreplaygain does too
- Version is now 1.14

2004-02-03 21:15 UTC - kode54
- Added an option to disable padding ... might as well, it doesn't improve updating speed anyway, due to remove/rewrite process
- Version is now 1.13

2004-02-01 11:34 UTC - kode54
- Corrected Unicode conversion in io_helpers.cpp and field_string_ascii.cpp yet again...
  This time, big endian is only assumed for UTF16BE encoding
- Version is now 1.12

2004-01-16 02:37 UTC - kode54
- Corrected Byte Order Marker and Unicode conversion in ID3lib io_helpers.cpp
- Version is now 1.11

2004-01-02 22:39 UTC - kode54
- Added support for ID3FID_UNIQUEFILEID as "unique_file_id" info field
- Version is now 1.10

2003-12-22 17:19 UTC - kode54
- Custom tags are now written as ID3FID_USERTEXT frames
- Custom reading supports both ID3FID_COMMENT and ID3FID_USERTEXT frames
- Changed how USERTEXT fields' descriptions are written and used
- Version is now 1.09

2003-11-24 19:12 UTC - kode54
- Fixed URL shit ... no encodings allowed, all uses ANSI
- Version is now 1.08

2003-10-31 15:50 UTC - kode54
- Fucking BOM madness... now UTF-16 is written little-endian, like Winamp and WMP expect
- Version is now 1.07

2003-10-30 20:00 UTC - kode54
- Restructured reading and writing a bit, now local CP option can't be changed in the middle of reading or writing
- Added BOM writing option to id3lib field interface and a configuration option for controlling it
- Version is now 1.06

2003-10-29 15:38 UTC - kode54
- Changed to write UTF-16 instead of UTF-8 (MusicBrainz claims UTF-8 requires 2.4 tag)
- Description field is properly encoded
- Fixed UTF-16 reading (confusion led me to add byte swapping code)
- Version is now 1.05

2003-10-17 11:09 UTC - kode54
- Fixed URL writing
- Version is now 1.04d

2003-10-03 16:20 UTC - kode54
- Made standard ISO-8859-1 decoding optional, so my software can behave like every other piece of fucking
  lame software out there and read according to the system codepage.
- Version is now 1.04c

2003-09-27 08:09 UTC - kode54
- Changed ISO-8859-1 converters to use system code page conversion instead of crappy internal converters
- Version is now 1.04b

2003-09-26 11:13 UTC - kode54
- Changed ISO-8859-1 converters to emit question marks instead of breaking on illegal characters
- Version is now 1.04a

2003-09-26 10:00 UTC - kode54
- Added ISO-8859-1 converter, changed reader to use it instead of string_utf8_from_ansi
- Added ISO-8859-1 writing
- Version is now 1.04

2003-09-25 00:04 UTC - kode54
- Changed id3v2_hacks slightly, so removal uses tempmem on files smaller than 16MB, not LARGER than 10MB
- Version is now 1.03

2003-07-24 12:19 - kode54
- Rewriting process uses tempmem reader if file is smaller than 16MB
- Version is now 1.02

2003-07-22 22:04 - kode54
- Fixed URL handling
- Version is now 1.01

2003-06-26 12:15 - kode54
- Added tag_remover service

2003-06-26 05:52 - kode54
- Updated to 0.7 API
- Version is now 1.0

2003-05-14 23:45 - kode54
- Single number DATE will be written as year, multi-number DATE will
  be searched for the largest number which will then be written to
  the year field
- Both date and year fields will be read as "DATE" and any which exist
  as a sub-string of another will be ignored
- Oops, almost forgot to account for the auto two to four digit extension
  I do to year values on write when searching for duplicates on read.
- Version is now 0.99
- Fixed slight mishap with year reading

2003-05-07 16:16 - kode54
- Changed genre handling again, no fucking apps follow the "standard" for
  custom types enclosed in (()
- Version is now 0.988
- FIXED IT AGAIN, NOT CHANGING VERSION NUMBER
- AND AGAIN !@#!#$@$#%!#@$%

2003-05-07 15:54 - kode54
- Made reading optional for one nagging user
- Version is now 0.987

2003-05-05 04:11 - kode54
- Reordered a couple of if/then checks, just a guess on how frequently
  comments appear vs usertext

2003-05-05 04:05 - kode54
- Removed unnecessary mem_block_manager crap, seems id3lib doesn't need it
- Version is now 0.986

2003-05-05 01:08 - kode54
- ID3_TagImpl::operator=(const ID3_Tag &) now properly transfers spec id
- Version is now 0.985

2003-05-03 14:14 - kode54
- Changed genre writer
- Version is now 0.981

2003-05-03 02:48 - kode54
- Renamed to ID3v2 tag support

2003-05-03 02:25 - kode54
- Only INVOLVEDPEOPLE and GENRE tags are accumulated into a single frame

2003-05-03 01:06 - kode54
- Added decent ID3v2 writer, damn this shit is hard to code for...
- No more remover, except as a time saver when handed empty file info
- Tag updating is optional, defaults to disabled
- Version is now 0.98

2003-04-27 23:15 - kode54
- Extra safety checks to tag field reading, proper length handling for UTF16
  <Case> consistency ist overrated

2003-04-27 22:42 - kode54
- Added AAC extension to read/write accept list
- Removal is now configurable, be prepared for metadata desync

2003-04-25 13:33 - kode54
- Fixed fucking UTF16 code, POS id3lib converts all to big endian in memory,
  ignores UTF16/UTF16BE field encoding ID

2003-04-19 02:52 - kode54
- Changed ID3V2_LATEST declaration in id3/globals.h to support v2.4.0 tags

2003-04-06 04:51 - kode54
- Fixed )!@#( missing UTF8 function in library, now at I can at least pull
  out the data, rather than it being thrown away by "oldconvert" function

2003-04-05 15:32 - kode54
- Amended genre reading, as per ID3v2.3.0 documentation

2003-04-04 05:41 - kode54
- Replaced evil input hack with tag_read/tag_write services, but it still
  depends on us being loaded before foo_input_std

2003-04-04 05:14 - kode54
- Oops, forgot to add name recursion check to is_our_content_type, might
  have broken MP3 streaming until now

2003-04-04 04:52 - kode54
- Now I use the tag detect/skip func from id3v2_hacks.cpp since id3lib seems
  to hang if fed a file that starts with a few MBs of null data
*/

#include "../SDK/foobar2000.h"

#include "../foo_input_std/id3v2_hacks.h"

#include "resource.h"

#define ID3LIB_LINKOPTION 1
#define NEED_ID3_v1_CRAP
#include <id3.h>
#include <id3/tag.h>
#include <id3/reader.h>
#include <id3/writers.h>

/*
static const char * mimes[]=
{
	"audio/mp3",
	"audio/mpeg",
	"audio/mpg",
	"audio/x-mp3",
	"audio/x-mpeg",
	"audio/x-mpg",
};
*/

/*
cfg_int cfg_read("id3v2_read", 1);
cfg_int cfg_write("id3v2_sync", 0);
*/

static cfg_int cfg_ansi("id3v2_ansi", 0);

static cfg_int cfg_fuckinglame("id3v2_ansi_fucking_lame", 0);

static cfg_int cfg_usebom("id3v2_unicode_usebom_feh", 1);

static cfg_int cfg_disablepadding("id3v2_disable_padding", 0);

bool is_iso88591(const char * src, unsigned len = ~0)
{
	len = strlen_max(src, len);
	while (*src && len)
	{
		unsigned c, d;
		d = utf8_decode_char(src, &c, len);
		if (d == 0 || d > len) break;
		src += d;
		len -= d;
		if ((c >= 128 && c <= 159) || (c >= 256)) break;
	}
	return !len;
}

unsigned convert_iso88591_to_utf16(const char * src,WCHAR * dst,unsigned len)
{
	len = strlen_max(src,len);
	unsigned rv;
	rv = MultiByteToWideChar(28591,0,src,len,dst,estimate_ansi_to_utf16(src));
	if ((signed)rv<0) rv = 0;
	dst[rv]=0;
	return rv;
}

unsigned convert_utf16_to_iso88591(const WCHAR * src,char * dst,unsigned len)
{
	len = wcslen_max(src,len);
	unsigned rv;
	rv = WideCharToMultiByte(28591,0,src,len,dst,estimate_utf16_to_ansi(src),0,0);
	if ((signed)rv<0) rv = 0;
	dst[rv]=0;
	return rv;
}

unsigned convert_utf8_to_iso88591(const char * src,char * dst,unsigned len)
{
	len = strlen_max(src,len);

	unsigned temp_len = estimate_utf8_to_utf16(src,len);
	mem_block_t<WCHAR> temp_block;
	WCHAR * temp = (temp_len * sizeof(WCHAR) <= PFC_ALLOCA_LIMIT) ? (WCHAR*)alloca(temp_len * sizeof(WCHAR)) : temp_block.set_size(temp_len);
	assert(temp);

	len = convert_utf8_to_utf16(src,temp,len);
	return convert_utf16_to_iso88591(temp,dst,len);
}

unsigned convert_iso88591_to_utf8(const char * src,char * dst,unsigned len)
{
	len = strlen_max(src,len);

	unsigned temp_len = estimate_ansi_to_utf16(src,len);
	mem_block_t<WCHAR> temp_block;
	WCHAR * temp = (temp_len * sizeof(WCHAR) <= PFC_ALLOCA_LIMIT) ? (WCHAR*)alloca(temp_len * sizeof(WCHAR)) : temp_block.set_size(temp_len);
	assert(temp);

	len = convert_iso88591_to_utf16(src,temp,len);
	return convert_utf16_to_utf8(temp,dst,len);
}

class string_iso88591_from_utf8 : public string_convert_base<char>
{
public:
	explicit string_iso88591_from_utf8(const char * src, unsigned len = -1)
	{
		len = strlen_max(src, len);
		alloc(len + 1);
		convert_utf8_to_iso88591(src, ptr, len);
	}
};

class string_utf8_from_iso88591 : public string_convert_base<char>
{
public:
	explicit string_utf8_from_iso88591(const char * src, unsigned len = -1)
	{
		len = strlen_max(src, len);
		alloc(len * 3 + 1);
		convert_iso88591_to_utf8(src, ptr, len);
	}
};

class ID3_FooReader : public ID3_Reader
{
	reader * _reader;
	int closed;
public:
	ID3_FooReader(reader * r) : _reader(r) { closed = 0; }
	virtual ~ID3_FooReader() { ; }
	virtual void close() { closed = 1; }

	virtual int_type peekChar()
	{
		if (closed) return END_OF_READER;
		int_type temp = 0;
		_reader->read(&temp, 1);
		_reader->seek2(-1, SEEK_CUR);
		return temp;
	}

	virtual size_type readChars(char_type buf[], size_type len)
	{
		if (closed) return END_OF_READER;
		return _reader->read(buf, len);
	}

	virtual pos_type getBeg()
	{
		if (closed) return END_OF_READER;
		return 0;
	}
	virtual pos_type getCur()
	{
		if (closed) return END_OF_READER;
		return _reader->get_position();
	}
	virtual pos_type getEnd()
	{
		if (closed) return END_OF_READER;
		return _reader->get_length();
	}

	virtual pos_type setCur(pos_type pos)
	{
		if (closed) return END_OF_READER;
		return _reader->seek(pos);
	}
};

static const ID3_FrameID field_ids[] =
{
	ID3FID_ALBUM,
	ID3FID_BPM,
	ID3FID_COMPOSER,
	ID3FID_CONTENTTYPE,
	ID3FID_COPYRIGHT,
	ID3FID_DATE,
	ID3FID_PLAYLISTDELAY,
	ID3FID_ENCODEDBY,
	ID3FID_LYRICIST,
	ID3FID_FILETYPE,
	ID3FID_TIME,
	ID3FID_CONTENTGROUP,
	ID3FID_TITLE,
	ID3FID_SUBTITLE,
	ID3FID_INITIALKEY,
	ID3FID_LANGUAGE,
	ID3FID_SONGLEN,
	ID3FID_MEDIATYPE,
	ID3FID_ORIGALBUM,
	ID3FID_ORIGFILENAME,
	ID3FID_ORIGLYRICIST,
	ID3FID_ORIGARTIST,
	ID3FID_ORIGYEAR,
	ID3FID_FILEOWNER,
	ID3FID_LEADARTIST,
	ID3FID_BAND,
	ID3FID_CONDUCTOR,
	ID3FID_MIXARTIST,
	ID3FID_PARTINSET,
	ID3FID_PUBLISHER,
	ID3FID_TRACKNUM,
	ID3FID_RECORDINGDATES,
	ID3FID_NETRADIOSTATION,
	ID3FID_NETRADIOOWNER,
	ID3FID_SIZE,
	ID3FID_ISRC,
	ID3FID_ENCODERSETTINGS,
	ID3FID_YEAR,
	ID3FID_USERTEXT,
	ID3FID_COMMENT,
	ID3FID_UNSYNCEDLYRICS,
	ID3FID_UNIQUEFILEID,
	ID3FID_WWWAUDIOFILE,
	ID3FID_WWWARTIST,
	ID3FID_WWWAUDIOSOURCE,
	ID3FID_WWWCOMMERCIALINFO,
	ID3FID_WWWCOPYRIGHT,
	ID3FID_WWWPUBLISHER,
	ID3FID_WWWPAYMENT,
	ID3FID_WWWRADIOPAGE,
	ID3FID_WWWUSER,
	ID3FID_INVOLVEDPEOPLE,
};

static const char * field_names[] =
{
	"ALBUM",
	"BPM",
	"COMPOSER",
	"GENRE",
	"COPYRIGHT",
	"DATE",
	"PLAYLISTDELAY",
	"ENCODEDBY",
	"LYRICIST",
	"FILETYPE",
	"TIME",
	"CONTENTGROUP",
	"TITLE",
	"SUBTITLE",
	"INITIALKEY",
	"LANGUAGE",
	"SONGLEN",
	"MEDIATYPE",
	"ORIGALBUM",
	"ORIGFILENAME",
	"ORIGLYRICIST",
	"ORIGARTIST",
	"ORIGYEAR",
	"FILEOWNER",
	"ARTIST",
	"BAND",
	"CONDUCTOR",
	"MIXARTIST",
	"PARTINSET",
	"PUBLISHER",
	"TRACKNUMBER",
	"RECORDINGDATES",
	"NETRADIOSTATION",
	"NETRADIOOWNER",
	"SIZE",
	"ISRC",
	"ENCODERSETTINGS",
	"YEAR",
	"USERTEXT",
	"COMMENT",
	"UNSYNCEDLYRICS",
	"unique_file_id",
	"WWWAUDIOFILE",
	"WWWARTIST",
	"WWWAUDIOSOURCE",
	"WWWCOMMERCIALINFO",
	"WWWCOPYRIGHT",
	"WWWPUBLISHER",
	"WWWPAYMENT",
	"WWWRADIOPAGE",
	"WWWUSER",
	"INVOLVEDPEOPLE",
};

static const char * rg_fields[] =
{
	"replaygain_track_gain",
	"replaygain_album_gain",
	"replaygain_track_peak",
	"replaygain_album_peak",
};

#define sc(a) \
	case ID3FID_##a: \
		name = field_##a; \
		break;

#define sc2(a,b) \
	case ID3FID_##a: \
		name = field_##a; \
		usertext = b; \
		break;

char *ID3_GetStringA(const ID3_Frame *frame, ID3_FieldID fldName)
{
  char *text = NULL;
//  if (NULL != frame)
  ID3_Field* fld;
  if (NULL != frame && NULL != (fld = frame->GetField(fldName)))
  {
//    ID3_Field* fld = frame->GetField(fldName);
    size_t nText = fld->Size();
    text = new char[nText + 1];
    fld->Get(text, nText);
	text[nText] = 0;
  }
  return text;
}

char *ID3_GetStringA(const ID3_Frame *frame, ID3_FieldID fldName, size_t nIndex)
{
  char *text = NULL;
  ID3_Field* fld;
  if (NULL != frame && NULL != (fld = frame->GetField(fldName)))
  {
    size_t nText = fld->Size();
    text = new char[nText + 1];
    fld->Get(text, nText, nIndex);
	text[nText] = 0;
  }
  return text;
}

unicode_t *ID3_GetStringW(const ID3_Frame *frame, ID3_FieldID fldName)
{
  unicode_t *text = NULL;
//  if (NULL != frame)
  ID3_Field* fld;
  if (NULL != frame && NULL != (fld = frame->GetField(fldName)))
  {
//    ID3_Field* fld = frame->GetField(fldName);
    size_t nText = fld->Size() / 2;
    text = new unicode_t[nText + 1];
    fld->Get(text, nText);
	text[nText] = 0;
  }
  return text;
}

unicode_t *ID3_GetStringW(const ID3_Frame *frame, ID3_FieldID fldName, size_t nIndex)
{
  unicode_t *text = NULL;
  ID3_Field* fld;
  if (NULL != frame && NULL != (fld = frame->GetField(fldName)))
  {
    size_t nText = fld->Size() / 2;
    text = new unicode_t[nText + 1];
    fld->Get(text, nText, nIndex);
	text[nText] = 0;
  }
  return text;
}

void add_string(const class ID3_Frame *frame, string8 & string, ID3_TextEnc enc, ID3_FieldID id, bool fuckinglame)
{
	if (enc == ID3TE_ISO8859_1)
	{
		char * sText = ID3_GetStringA(frame, id);
		if (sText)
		{
			if (fuckinglame) string.add_string(string_utf8_from_ansi(sText));
			else string.add_string(string_utf8_from_iso88591(sText));
			delete [] sText;
		}
	}
/*
	else if (enc == ID3TE_UTF16BE)
	{
		unicode_t * sText = ID3_GetStringW(frame, id);
		string.add_string(string_utf8_from_utf16(sText));
		delete [] sText;
	}
*/
	else if (enc == ID3TE_UTF16 || enc == ID3TE_UTF16BE)
	{
		unicode_t * sText = ID3_GetStringW(frame, id);
		if (sText)
		{
			/*unicode_t * sptr = sText;
			while (*sptr)
			{
				unicode_t temp = ((*sptr & 255) << 8) | ((*sptr >> 8) & 255);
				*sptr++ = temp;
			}*/
			string.add_string(string_utf8_from_utf16(sText));
			delete [] sText;
		}
	}
	else if (enc == ID3TE_UTF8)
	{
		char * sText = ID3_GetStringA(frame, id);
		if (sText)
		{
			string.add_string(sText);
			delete [] sText;
		}
	}
}

void add_string(const class ID3_Frame *frame, string8 & string, ID3_TextEnc enc, ID3_FieldID id, size_t nIndex, bool fuckinglame)
{
	if (enc == ID3TE_ISO8859_1)
	{
		char * sText = ID3_GetStringA(frame, id, nIndex);
		if (sText)
		{
			if (fuckinglame) string.add_string(string_utf8_from_ansi(sText));
			else string.add_string(string_utf8_from_iso88591(sText));
			delete [] sText;
		}
	}
/*
	else if (enc == ID3TE_UTF16BE)
	{
		unicode_t * sText = ID3_GetStringW(frame, id, nIndex);
		string.add_string(string_utf8_from_utf16(sText));
		delete [] sText;
	}
*/
	else if (enc == ID3TE_UTF16 || enc == ID3TE_UTF16BE)
	{
		unicode_t * sText = ID3_GetStringW(frame, id, nIndex);
		if (sText)
		{
			unicode_t * sptr = sText;
			while (*sptr)
			{
				unicode_t temp = ((*sptr & 255) << 8) | ((*sptr >> 8) & 255);
				*sptr++ = temp;
			}
			string.add_string(string_utf8_from_utf16(sText));
			delete [] sText;
		}
	}
	else if (enc == ID3TE_UTF8)
	{
		char * sText = ID3_GetStringA(frame, id, nIndex);
		if (sText)
		{
			string.add_string(sText);
			delete [] sText;
		}
	}
}

void add_unique(file_info * info, const char * name, const char * value)
{
	int t;
	string8 temp = "";
	{
		string8 blah = value;
		t = blah.find_first('\r');
		while (t >= 0)
		{
			temp.add_string_n(blah, t);
			temp.add_string("\r\n");
			t++;
			if (*(blah.get_ptr() + t) == 10) t++;
			blah.remove_chars(0, t);
			t = blah.find_first('\r');
		}
		if (blah.length()) temp.add_string(blah);
	}

	if (!(t = info->meta_get_count_by_name(name)))
	{
		info->meta_add(name, temp);
		return;
	}
	int i;
	for (i = 0; i < t; i++)
	{
		int idx = info->meta_get_idx(name, i);
		char *ex = (char*) info->meta_enum_value(idx);
		int l = strlen(ex), m = temp.length();
		if (l > m)
		{
			l = m;
			m = -1;
		}
		if (!strncmp(ex, temp, l))
		{
			if (m > 0) info->meta_modify_value(idx, temp);
			return;
		}
	}
	info->meta_add(name, temp);
}

extern "C" int is_replaygain(const char * name);

int id3v2_isreplaygain(const char * name)
{
	/*
	int i;
	for (i = 0; i < tabsize(rg_fields); i++)
	{
		if (!stricmp(name, rg_fields[i])) return 1;
	}
	return 0;
	*/
	string8_fastalloc check;
	uAddStringLower(check, name);
	return is_replaygain(check);
}

int id3v2_read(reader * r, file_info * info)
{
	int skip;
	if (!(skip = id3v2_calc_size(r))) return 0;
	r->seek(0);
	ID3_Tag *tag = new ID3_Tag;
	ID3_FooReader rdr(r);
	if (!(skip = tag->Link(rdr, ID3TT_ID3V2)))
	{
		delete tag;
		return 0;
	}

	bool fuckinglame = cfg_fuckinglame > 0;

	{
		ID3_Tag::Iterator* iter = tag->CreateIterator();
		const ID3_Frame* frame = NULL;
		while (NULL != (frame = iter->GetNext()))
		{
			ID3_FrameID eFrameID = frame->GetID();
			const char *name = NULL;
			int usertext = 0;
			for (int i = 0; i < tabsize(field_ids); i++)
			{
				if (field_ids[i] == eFrameID)
				{
					name = field_names[i];
					if (eFrameID == ID3FID_CONTENTTYPE) usertext = 16;
					else if (eFrameID == ID3FID_COMMENT) usertext = 8;
					else if (eFrameID == ID3FID_YEAR) usertext = 64;
					else if (eFrameID == ID3FID_USERTEXT) usertext = 1;
					else if (eFrameID == ID3FID_INVOLVEDPEOPLE) usertext = 4;
					else if (eFrameID == ID3FID_UNIQUEFILEID) usertext = 32;
					else if (i > 41) usertext = eFrameID == ID3FID_WWWUSER ? 3 : 2;
					break;
				}
			}

			if (name)
			{
				string8 value = "", nm = name;
				ID3_TextEnc enc;
				ID3_Field * field;
				field = frame->GetField(ID3FN_TEXTENC);
				if (field) enc = (ID3_TextEnc) field->Get();
				else enc = ID3TE_ISO8859_1; // fucking software forgot to specify the encoding type
				if (usertext & 8)
				{
					add_string(frame, value, enc, ID3FN_DESCRIPTION, fuckinglame);
					if (value.length())
					{
						if (!stricmp(value, "no description")) value.reset();
						else
						{
							if (!stricmp(value, "Comments") && usertext & 8) value.reset();
							else if (!strnicmp(value, "Foobar2000: ", 12))
							{
								nm.set_string(value.get_ptr() + 12);
								value.reset();
								if (id3v2_isreplaygain(nm)) usertext |= 32;
							}
							else value.add_string(": ");
						}
					}
				}
				if (usertext & 1)
				{
					add_string(frame, value, enc, ID3FN_DESCRIPTION, fuckinglame);
					if (value.length())
					{
						nm.set_string(value);
						value.reset();
						if (id3v2_isreplaygain(nm)) usertext |= 32;
					}
				}
				if (usertext & 2)
				{
					add_string(frame, value, ID3TE_ISO8859_1, ID3FN_URL, fuckinglame);
				}
				if (usertext & 4)
				{
					size_t nItems = frame->GetField(ID3FN_TEXT)->GetNumTextItems();
					for (size_t nIndex = 0; nIndex < nItems; nIndex++)
					{
						value = "";
						add_string(frame, value, enc, ID3FN_TEXT, nIndex, fuckinglame);
						if (value.length()) add_unique(info, name, value);
					}
					continue;
				}
				if (!(usertext & 2)) add_string(frame, value, enc, ID3FN_TEXT, fuckinglame);
				if (usertext & 16)
				{
					// fecking genre field madness
					char * genre = new char[value.length()];
					int skip;
					while (sscanf(value, "(%[^)]) %n", genre, &skip) > 0)
					{
						string8 gentext;
						if (!strcmp(genre, "RX")) gentext.set_string("Remix");
						else if (!strcmp(genre, "CR")) gentext.set_string("Cover");
						else if (genre[0] == '(') gentext.set_string(genre + 1);
						else
						{
							int gennr = atoi(genre);
							if (gennr < ID3_NR_OF_V1_GENRES && gennr >= 0)
								gentext.set_string(ID3_v1_genre_description[gennr]);
							else
								gentext.reset();
						}
						if (gentext.length()) add_unique(info, name, gentext);
						if (value.length() > skip)
						{
							gentext.set_string(value.get_ptr() + skip);
							value = gentext;
						}
						else
						{
							value.reset();
							break;
						}
					}
					delete [] genre;
				}
				
				if (value.length())
				{
					if (usertext & 32) info->info_set(nm, value);
					else if (usertext & 64)
					{
						// fecking DATE/YEAR madness
						int count = info->meta_get_count_by_name("DATE");
						if (count)
						{
							int i, num;
							const char * blah = value;
							num = atol(blah);
							if (num > 1939 && num < 2040) blah += 2;
							for (i = 0; i < count; i++)
							{
								int idx = info->meta_get_idx("DATE", i);
								const char * val = info->meta_enum_value(idx);
								if (strstr(val, blah)) break;
							}
							if (i < count) continue;
						}
						add_unique(info, "DATE", value);
					}
					else add_unique(info, nm, value);
				}
			}
		}
		delete iter;
	}
	delete tag;

	r->seek(skip);
	return skip;
}

enum field_flags_t
{
	FIELD_FLAG_ANSI = 1,
	FIELD_FLAG_LOCALCP = 2,
	FIELD_FLAG_USEBOM = 4,
};

extern "C" ID3_FrameID get_frame_id(const char * name);

void id3v2_setupframe(ID3_Frame & frame, const char * name, unsigned flags)
{
	ID3_TextEnc enc = (flags & FIELD_FLAG_ANSI) ? ID3TE_ISO8859_1 : ID3TE_UTF16;
	frame.Clear();

	string8_fastalloc check;
	uAddStringUpper(check, name);
	ID3_FrameID fid = get_frame_id(check);

	if (fid != ID3FID_NOFRAME)
	{
		frame.SetID(fid);
		ID3_Field * fld = frame.GetField(ID3FN_TEXTENC);
		if (fld) fld->Set(enc);
	}
	else
	{
		string8 desc;
		frame.SetID(ID3FID_USERTEXT);
		frame.Field(ID3FN_TEXTENC) = enc;
		desc.add_string(name);
		ID3_Field * field = frame.GetField(ID3FN_DESCRIPTION);
		field->SetEncoding(enc);
		if (flags & FIELD_FLAG_ANSI)
		{
			if (flags & FIELD_FLAG_LOCALCP) field->Set(string_ansi_from_utf8(desc));
			else field->Set(string_iso88591_from_utf8(desc));
		}
		else
		{
			field->SetBOM((flags & FIELD_FLAG_USEBOM) > 0);
			field->Set(string_utf16_from_utf8(desc));
		}
	}
}

int id3v2_addgenre(string8 & text, const char * val)
{
	int i;
	for (i = 0; i < ID3_NR_OF_V1_GENRES; i++)
	{
		if (!stricmp(val, ID3_v1_genre_description[i]))
		{
			text.add_char('(');
			text.add_int(i);
			text.add_char(')');
			break;
		}
	}
	if (i == ID3_NR_OF_V1_GENRES)
	{
		if (!stricmp(val, "Remix"))
		{
			text.add_char('(');
			text.add_string("RX");
			text.add_char(')');
		}
		else if (!stricmp(val, "Cover"))
		{
			text.add_char('(');
			text.add_string("CR");
			text.add_char(')');
		}
		else
			return 0; // add to its own separate tag later, fucking "standard" and fucking programs that don't follow it
	}
	return 1;
}

void id3v2_addtoframe(ID3_Frame & frame, const char * val, unsigned flags)
{
	string8 foo(val);
	ID3_FrameID id = frame.GetID();
	ID3_TextEnc enc = (flags & FIELD_FLAG_ANSI) ? ID3TE_ISO8859_1 : ID3TE_UTF16;

	ID3_FieldID fid;
	ID3_Field * field;

	if (id == ID3FID_WWWAUDIOFILE || id == ID3FID_WWWARTIST || id == ID3FID_WWWAUDIOSOURCE ||
		id == ID3FID_WWWCOMMERCIALINFO || id == ID3FID_WWWCOPYRIGHT || id == ID3FID_WWWPUBLISHER ||
		id == ID3FID_WWWPAYMENT || id == ID3FID_WWWRADIOPAGE || id == ID3FID_WWWUSER)
	{
		fid = ID3FN_URL;
	}
	else
	{
		fid = ID3FN_TEXT;
	}

/*
	if (id == ID3FID_COMMENT || id == ID3FID_USERTEXT || id == ID3FID_WWWUSER)
	{
		char * blah = strstr(foo, ": ");
		if (blah)
		{
			field = frame.GetField(ID3FN_DESCRIPTION);
			*blah = 0;
			blah += 2;
			if (flags & FIELD_FLAG_ANSI)
			{
				field->SetEncoding(enc);
				if (flags & FIELD_FLAG_LOCALCP) field->Set(string_ansi_from_utf8(foo));
				else field->Set(string_iso88591_from_utf8(foo));
				field = frame.GetField(fid);
				if (field->IsEncodable()) field->SetEncoding(enc);
fuckety:
				if (flags & FIELD_FLAG_LOCALCP) field->Set(string_ansi_from_utf8(blah));
				else field->Set(string_iso88591_from_utf8(blah));
			}
			else
			{
				field->SetEncoding(enc);
				field->SetBOM((flags & FIELD_FLAG_USEBOM) > 0);
				field->Set(string_utf16_from_utf8(foo));
				field = frame.GetField(fid);
				if (!field->IsEncodable()) goto fuckety;
				field->SetEncoding(enc);
				field->SetBOM((flags & FIELD_FLAG_USEBOM) > 0);
				field->Set(string_utf16_from_utf8(blah));
			}
			return;
		}
	}
*/

	field = frame.GetField(fid);
	field->SetEncoding(enc);

	if (fid == ID3FN_URL)
	{
		if (flags & FIELD_FLAG_LOCALCP) field->Set(string_ansi_from_utf8(foo));
		else field->Set(string_iso88591_from_utf8(foo));
	}
	else
	{
		if (flags & FIELD_FLAG_ANSI)
		{
			if (flags & FIELD_FLAG_LOCALCP) field->Add(string_ansi_from_utf8(foo));
			else field->Add(string_iso88591_from_utf8(foo));
		}
		else
		{
			field->SetBOM((flags & FIELD_FLAG_USEBOM) > 0);
			field->Add(string_utf16_from_utf8(foo));
		}
	}
}

int id3v2_write(reader * r, file_info * info)
{
	if (!r->can_seek()) return 0;
	__int64 len = r->get_length();
	if (len<=0) return 0;
	if (!info->meta_get_count() && !info->info_get_count())
	{
		id3v2_remove(r);
		return 1;
	}

	unsigned flags;
	if (cfg_ansi)
	{
		flags = FIELD_FLAG_ANSI;
		if (cfg_fuckinglame) flags |= FIELD_FLAG_LOCALCP;
	}
	else
	{
		if (cfg_usebom) flags = FIELD_FLAG_USEBOM;
		else flags = 0;
	}

	ID3_Tag tag;
	ID3_Frame frame;

	bool usepadding = !cfg_disablepadding;

	tag.SetSpec(ID3V2_3_0);
	tag.SetPadding(usepadding);
	frame.SetSpec(ID3V2_3_0);

	while (info->meta_get_count())
	{
		//bool fallback;
		const char * name = info->meta_enum_name(0);
		if (id3v2_isreplaygain(name))
		{
			// no way
			info->meta_remove_field(name);
		}
		else
		{
			//fallback = !(flags & FIELD_FLAG_ANSI);
			id3v2_setupframe(frame, name, flags);
			switch (frame.GetID())
			{
			/*
			case ID3FID_CONTENTTYPE:
				{
					ptr_list_t<char> nonstandard;
					string8 text;
					int n = info->meta_get_count_by_name(name), i;
					for (i = 0; i < n; i++)
					{
						int idx = info->meta_get_idx(name, i);
						const char * val = info->meta_enum_value(idx);
						if (!id3v2_addgenre(text, val))
						{
							if (strlen(val))
							{
								char * blah = new char[strlen(val) + 1];
								strcpy(blah, val);
								nonstandard.add_item(blah);
							}
						}
					}
					info->meta_remove_field(name);
					if (text.length())
					{
						if (fallback) id3v2_setupframe(frame, "GENRE", FIELD_FLAG_ANSI);
						id3v2_addtoframe(frame, text, FIELD_FLAG_ANSI);
						tag.AddFrame(frame);
					}
					if (n = nonstandard.get_count())
					{
						for (i = 0; i < n; i++)
						{
							const char * item = nonstandard.get_item(i);
							if (fallback)
							{
								unsigned tflags = is_iso88591(item) ? FIELD_FLAG_ANSI : flags;
								id3v2_setupframe(frame, "GENRE", tflags); // meta_remove_field effectively rendered name var useless
								id3v2_addtoframe(frame, item, tflags);
							}
							else
							{
								id3v2_setupframe(frame, "GENRE", flags);
								id3v2_addtoframe(frame, item, flags);
							}
							tag.AddFrame(frame);
						}
						nonstandard.delete_all();
					}
				}
				break;
			*/
			case ID3FID_INVOLVEDPEOPLE:
				{
					int n = info->meta_get_count_by_name(name), i;
					/*if (fallback)
					{
						for (i = 0; i < n, fallback; i++)
						{
							int idx = info->meta_get_idx(name, i);
							const char * val = info->meta_enum_value(idx);
							fallback = is_iso88591(val);
						}
					}*/
					unsigned tflags = flags;
					/*if (fallback)
					{
						tflags = FIELD_FLAG_ANSI;
						id3v2_setupframe(frame, name, tflags);
					}*/
					for (i = 0; i < n; i++)
					{
						int idx = info->meta_get_idx(name, i);
						const char * val = info->meta_enum_value(idx);
						id3v2_addtoframe(frame, val, tflags);
					}
					info->meta_remove_field(name);
					tag.AddFrame(frame);
				}
				break;
			case ID3FID_DATE:
				{
					// ah, shit. another user-requested hack.
					const char * val = info->meta_enum_value(0);
					int i, j = strlen(val);
					for (i = 0; i < j; i++)
					{
						if (val[i] < '0' || val[i] > '9') break;
					}
					if (i == j)
					{
						id3v2_setupframe(frame, "YEAR", FIELD_FLAG_ANSI);
						id3v2_addtoframe(frame, val, FIELD_FLAG_ANSI);
						tag.AddFrame(frame);
					}
					else
					{
						unsigned tflags = flags;
						/*if (fallback)
						{
							fallback = is_iso88591(val);
							if (fallback)
							{
								tflags = FIELD_FLAG_ANSI;
								id3v2_setupframe(frame, name, tflags);
							}
						}*/
						id3v2_addtoframe(frame, val, tflags);
						tag.AddFrame(frame);
						char * blah;
						int num, ln = 0;
						while (*val && (num = strtoul(val, &blah, 10)) && blah > val)
						{
							if (num > ln) ln = num;
							val = blah;
							while (*val && (*val < '0' || *val > '9')) val++;
						}
						if (ln > 31)
						{
							string8 foo;
							if (ln < 40) ln += 2000;
							else if (ln < 100) ln += 1900;
							foo.add_int(ln);
							id3v2_setupframe(frame, "YEAR", FIELD_FLAG_ANSI);
							id3v2_addtoframe(frame, foo, FIELD_FLAG_ANSI);
							tag.AddFrame(frame);
						}
					}
					info->meta_remove(0);
				}
				break;
			default:
				{
					const char * val = info->meta_enum_value(0);
					unsigned tflags = flags;
					/*if (fallback)
					{
						fallback = is_iso88591(val);
						if (fallback)
						{
							tflags = FIELD_FLAG_ANSI;
							id3v2_setupframe(frame, name, tflags);
						}
					}*/
					id3v2_addtoframe(frame, val, tflags);
					info->meta_remove(0);
					tag.AddFrame(frame);
				}
				break;
			}
		}
	}

	int n = info->info_get_count(), i;
	for (i = 0; i < n; i++)
	{
		//bool fallback = !(flags & FIELD_FLAG_ANSI);
		const char * name = info->info_enum_name(i);
		unsigned tflags = flags;
		if (id3v2_isreplaygain(name) || !stricmp(name, "unique_file_id"))
		{
			const char * val = info->info_enum_value(i);
			/*if (fallback)
			{
				fallback = is_iso88591(val);
				if (fallback)
				{
					tflags = FIELD_FLAG_ANSI;
				}
			}*/
			id3v2_setupframe(frame, name, tflags);
			id3v2_addtoframe(frame, val, tflags);
			tag.AddFrame(frame);
		}
	}

	reader * temp;
	__int64 offset = id3v2_calc_size(r);

	if (usepadding) tag.SetFileSize(len - offset);
	luint tagsize = tag.Size();
	if (tagsize > 0)
	{
		r->seek(0);

		// fecking kludge
		//if (usepadding) tagsize = (tagsize + 2047) & ~0x7FF;
		uchar * buffer;
		if (buffer = new uchar[tagsize])
		{
			tagsize = tag.Render(ID3_MemoryWriter(buffer,tagsize));
			if (offset == tagsize)
			{
				r->seek(0);
				r->write(buffer, tagsize);
				delete [] buffer;
				return 1;
			}
			temp = len > 16 * 1024 * 1024 ? file::g_open_temp() : file::g_open_tempmem();
			if (temp == 0) return 0;
			temp->write(buffer, tagsize);
			delete [] buffer;
		}
		if (offset > len)
		{
			temp->reader_release();
			return 0;
		}
		len -= offset;
		if (reader::transfer(r, temp, len) != len)
		{
			temp->reader_release();
			return 0;
		}
		r->seek(0);
		r->set_eof();
		temp->seek(0);
		reader::transfer(temp, r, temp->get_length());
		temp->reader_release();
	}
	return 1;
}

/*
static ptr_list_t<string8> names;

static int name_check(const char *fn)
{
	if (names.get_count())
	{
		for (int i = 0; i < names.get_count(); i++)
		{
			string8 *temp;
			temp = names.get_item(i);
			if (!stricmp(fn, temp->get_ptr())) return 0;
		}
	}
	return 1;
}
*/

class tag_reader_id3v2 : public tag_reader
{
	int run(reader * r, file_info * info)
	{
		r->seek(0);
		return id3v2_read(r, info) ? 1 : 0;
	}
	const char * get_name() { return "id3v2"; }
};

class tag_writer_id3v2 : public tag_writer
{
	int run(reader * r, const file_info * info)
	{
		file_info_i_full foo;
		foo.copy(info);
		return id3v2_write(r, &foo);
	}
	const char * get_name() { return "id3v2"; }
};

class tag_remover_id3v2 : public tag_remover
{
	void run(reader * r)
	{
		id3v2_remove(r);
	}
};

/*
class input_id3v2 : public input
{
private:

	input_helper in;
	string8 name;

	virtual int set_info(reader *r, const file_info *info)
	{
		file_info_i_full info2;
		info2.copy(info);
		if (!input::g_get_info(&info2,r)) return 0;
		id3v2_remove(r);

		name = info->get_file_path();
		names.add_item(&name);

		int ret = input::g_set_info(info, r);

		names.remove_item(&name);

		return ret;
	}

	virtual int open(reader * r,file_info * info,int full_open)
	{
		name = info->get_file_path();
		names.add_item(&name);
		
		int ret;

		if (full_open)
			ret = in.open(info);
		else
			ret = input::g_get_info(info, r);

		r->seek(0);
		id3v2_read(r, info);

		names.remove_item(&name);

		return ret;
	}

	virtual int is_our_content_type(const char *url, const char *type)
	{
		int n;
		for (n = 0; n < tabsize(mimes); n++)
		{
			if (!strcmp(type, mimes[n]))
			{
				// okay, I don't want to break mp3 streaming here
				return name_check(url);
			}
		}
		return 0;
	}

	virtual int test_filename(const char * fn,const char * ext)
	{
		if (stricmp(ext, "MP3") && stricmp(ext, "MP2")) return 0;
		return name_check(fn);
	}

public:
	input_id3v2()
	{
	}
	~input_id3v2()
	{
	}

	virtual int run(audio_chunk * chunk)
	{
		return in.run(chunk);
	}

	virtual int can_seek()
	{
		return in.can_seek();
	}	
	virtual int seek(double ms)
	{
		return in.seek(ms);
	}
	virtual void abort()
	{
		in.abort();
	}
};
*/

static BOOL CALLBACK ConfigProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			uSendDlgItemMessage(wnd,IDC_ANSI,BM_SETCHECK,cfg_ansi,0);
			HWND w = GetDlgItem(wnd, IDC_USEBOM);
			EnableWindow(w, !cfg_ansi);
			uSendMessage(w, BM_SETCHECK, cfg_usebom, 0);
			uSendDlgItemMessage(wnd,IDC_FUCKINGLAME,BM_SETCHECK,cfg_fuckinglame,0);
			uSendDlgItemMessage(wnd,IDC_PADDING,BM_SETCHECK,cfg_disablepadding,0);
		}
		break;
	case WM_COMMAND:
		switch(wp)
		{
		case IDC_ANSI:
			cfg_ansi = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
			EnableWindow(GetDlgItem(wnd,IDC_USEBOM),!cfg_ansi);
			break;
		case IDC_USEBOM:
			if (IDYES == uMessageBox(wnd, "YOU KNOW WHAT YOU DOING?", "TAKE OFF EVERY 'BOM'!!", MB_YESNO | MB_ICONQUESTION))
			{
				cfg_usebom = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
			}
			else
			{
				uSendMessage((HWND)lp,BM_SETCHECK,cfg_usebom,0);
			}
			break;
		case IDC_FUCKINGLAME:
			cfg_fuckinglame = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		case IDC_PADDING:
			cfg_disablepadding = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		}
		break;
	}
	return 0;
}

class config_id3v2 : public config
{
public:
	HWND create(HWND parent)
	{
		return uCreateDialog(IDD_CONFIG,parent,ConfigProc);
	}
	const char * get_name() {return "ID3v2 tag support";}
	const char * get_parent_name() {return "Components";}
};

static service_factory_single_t<config,config_id3v2> foo;
//static service_factory_t<input,input_id3v2> foo3;
static service_factory_t<tag_reader,tag_reader_id3v2> foo2;
static service_factory_t<tag_writer,tag_writer_id3v2> foo3;
static service_factory_t<tag_remover,tag_remover_id3v2> foo4;

DECLARE_COMPONENT_VERSION("ID3v2 tag support",MYVERSION,"You've got your ID3v2, now STFU.");
