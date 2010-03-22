#include <stdint.h>
#include "usf.h"
#include "cpu.h"

#include "audio.h"
#include "psftag.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


uint32_t Usf::LoadUSF(char * fn)
{
	pfc::string8 name, arch;
	service_ptr_t<file> m_file;

	uint32_t reservedsize = 0, codesize = 0, crc = 0, tagstart = 0, reservestart = 0;
	uint32_t filesize = 0, tagsize = 0, temp = 0;
	bool isArchive = false;
	char buffer[16], * buffer2 = NULL, * tagbuffer = NULL, lib[MAX_FILENAME_LENGTH];
	char usfFileName[MAX_FILENAME_LENGTH], drive[32], dir[MAX_FILENAME_LENGTH], ext[MAX_FILENAME_LENGTH];

	if(fn == NULL)
	{
		fn = file_name;
		if(file_name[0] == 0)
		{
			throw exception_io_no_handler_for_path();
			return 0;
		}
	}

	//try
	//{
		if(archive_impl::g_is_unpack_path(fn) == true)
		{
			archive_impl::g_parse_unpack_path(fn, arch, name);
			_splitpath(strrchr(fn, '|') + 1, drive, dir, usfFileName, ext );
			filesystem::g_open_read(m_file, fn, m_abort);
			isArchive = true;
		}
		else
		{
			isArchive = false;

			if(strncmp(fn, "file:", 5))
			{
				throw exception_io_unsupported_format();
				return 0;
			}

			_splitpath( &fn[7], drive, dir, usfFileName, ext );
			filesystem::g_open_read(m_file, fn, m_abort);
		}
	//}
	/*catch (exception_aborted)
	{
		_asm int 3
		return 0;
	}*/

	fileStats = m_file->get_stats(m_abort);

	if(savestate == NULL)
	{
		savestate = (uint8_t*) malloc(0x80275c);
		if(savestate == NULL)
		{
			throw exception_out_of_resources();
		}

		memset(savestate, 0, 0x80275c);
	}

	m_file->read(buffer, 4, m_abort);

	if(buffer[0] != 'P' && buffer[1] != 'S' && buffer[2] != 'F' && buffer[3] != 0x21)
	{
		throw exception_io_data_truncation();
		return 0;
	}

	m_file->read(&reservedsize, 4, m_abort);
	m_file->read(&codesize, 4, m_abort);
	m_file->read(&crc, 4, m_abort);

	filesize = m_file->get_size(m_abort);

	reservestart = 0x10;
	tagstart = reservestart + reservedsize;
	tagsize = filesize - tagstart;

	if(tagsize)
	{
		char Length[64], Fade[64];

		m_file->seek(tagstart, m_abort);
		m_file->read(buffer, 5, m_abort);

		if(buffer[0] != '[' && buffer[1] != 'T' && buffer[2] != 'A' && buffer[3] != 'G' && buffer[4] != ']')
		{
			throw exception_io_data_truncation();
		}

		buffer2 = (char*) malloc(50001);
		tagbuffer = (char*) malloc(tagsize - 4);

		m_file->read(tagbuffer, tagsize - 5, m_abort);
		tagbuffer[ tagsize - 5 ] = 0;

		psftag_raw_getvar(tagbuffer, "_lib", lib, 50000);

		if(strlen(lib))
		{
			char path[MAX_FILENAME_LENGTH], buf[MAX_FILENAME_LENGTH];
			char drive2[32], usfFileName2[MAX_FILENAME_LENGTH], dir2[MAX_FILENAME_LENGTH], ext2[MAX_FILENAME_LENGTH];
			int pathlength = 0;

			if(isArchive)
			{
				if(strlen(dir))
					sprintf(buf,"%s%s", dir, lib);
				else
					sprintf(buf,"%s",lib);

				_splitpath(arch.get_ptr()+7, drive2, dir2, usfFileName2, ext2 );

				archive_impl::g_make_unpack_path(libpath8, arch.get_ptr(), buf, ext2+1);

				LoadUSF((char *)libpath8.get_ptr());
			}
			else
			{
				char libpath[MAX_FILENAME_LENGTH];
				sprintf(libpath,"file://%s%s%s", drive, dir, lib);
				LoadUSF(libpath);
			}
		}

		psftag_raw_getvar(tagbuffer, "_enablecompare", buffer2, 50000);
		if(strlen(buffer2))
			EnableCompare = true;

		psftag_raw_getvar(tagbuffer, "_enableFIFOfull", buffer2, 50000);
		if(strlen(buffer2))
			EnableFIFOFull = true;

		psftag_raw_getvar(tagbuffer, "length", Length, sizeof(Length) - 1);
		psftag_raw_getvar(tagbuffer, "fade", Fade, sizeof(Fade) - 1);

		psftag_raw_getvar(tagbuffer, "volume", buffer2, 50000);
		if(strlen(buffer2))
		{
			sscanf(buffer2, "%f", &RelVolume);
			if(_isnan(RelVolume))
				RelVolume = 1.0;
		}



		if(strlen(Length) && !PlayDefault)
		{
			if(strlen(Fade))
			{
				TrackLength = GetLengthFromString(Length);
				FadeLength = GetLengthFromString(Fade);
			}
			else
			{
				TrackLength = GetLengthFromString(Length);
			}
		}


		free(buffer2);
		buffer2 = NULL;

		free(tagbuffer);
		tagbuffer = NULL;

	}

	m_file->seek(reservestart, m_abort);
	m_file->read(&temp, 4, m_abort);

	if(temp == 0x34365253) //there is a rom section
	{
		int32_t len = 0, start = 0;
		m_file->read(&len, 4, m_abort);

		while(len)
		{
			m_file->read(&start, 4, m_abort);

			while(len)
			{
				int32_t page = start >> 16;
				int32_t readLen = ( ((start + len) >> 16) > page) ? (((page + 1) << 16) - start) : len;

				if(rom_pages[page] == 0)
				{
					rom_pages[page] = (uint8_t*) malloc(0x10000);
					memset(rom_pages[page], 0, 0x10000);
				}

				m_file->read(rom_pages[page] + (start & 0xffff), readLen, m_abort);

				start += readLen;
				len -= readLen;
			}

			m_file->read(&len, 4, m_abort);
		}

	}

	m_file->read(&temp, 4, m_abort);

	if(temp == 0x34365253) /* RAM section*/
	{
		int32_t len = 0, start = 0;
		m_file->read(&len, 4, m_abort);

		while(len)
		{
			m_file->read(&start, 4, m_abort);

			m_file->read(savestate + start, len, m_abort);

			m_file->read(&len, 4, m_abort);
		}
	}

	return 1;
}

Usf::Usf(const char * FileName, abort_callback & p_abort) : m_abort(p_abort)
{
	int i = 0;

	status = 0;
	savestate = NULL;
	TrackLength = FadeLength = SilenceTime = 0.0;
	RelVolume = 1.0;

	LoadSettings();

	EnableCompare = EnableFIFOFull = Seeking = false;

	cpu = NULL;

	for(i = 0; i < 0x400; i++)
	{
		rom_pages[i] = NULL;
	}

	if(FileName)
		strcpy(file_name, FileName);
}


uint32_t Usf::PlayUSF(void)
{
	if(savestate == NULL) _asm int 3

	if(CpuMode == CPU_RECOMPILER)
		cpu = new Recompiler(*this);
	else
		cpu = new Interpreter(*this);

	cpu->LoadCpu(savestate, rom_pages);
	cpu->StartCPU();
	return 1;
}


Usf::~Usf()
{

	delete cpu;
	free(savestate);
}

// from 64th note
// Will work on any properly formatted string, will fail miserably on anything else
double GetLengthFromString(char * timestring)
{
	int c=0,decimalused=0,multiplier=1;
	unsigned long total=0;
	if (strlen(timestring) == 0) return 0;
	for (c=strlen(timestring)-1; c >= 0; c--) {
		if (timestring[c]=='.' || timestring[c]==',') {
			decimalused=1;
			total*=1000/multiplier;
			multiplier=1000;
		} else if (timestring[c]==':') multiplier=multiplier*6/10;
		else {
			total+=(timestring[c]-'0')*multiplier;
			multiplier*=10;
		}
	}
	if (!decimalused) total*=1000;
	return ((double)total) / 1000.0;
}

static void SetTagzAnsi(file_info & p_info ,char * tagz, char * value)
{
	pfc::stringcvt::string_utf8_from_ansi String;
	String.convert(value, strlen(value));
	p_info.meta_set(tagz, String);
}

void SetTag(file_info & p_info, char * psftag, char * tagz, char * tag)
{
	char Buf[50001];
	pfc::stringcvt::string_utf8_from_ansi String;
	psftag_raw_getvar(psftag, tag, Buf, 50001);
	String.convert(Buf, strlen(Buf));
	p_info.meta_set(tagz, String);

}

void trim_whitespace( pfc::string_base & val )
{
	const char * start = val.get_ptr();
	const char * end = start + strlen( start ) - 1;
	while ( *start > 0 && *start < 0x20 ) ++start;
	while ( end >= start && *end >= 0 && *end < 0x20 ) --end;
	memcpy( (void *) val.get_ptr(), start, end - start + 1 );
	val.truncate( end - start + 1 );
}

void info_set( file_info & p_info, const char * name, const char * value, bool utf8 )
{
	if ( utf8 ) p_info.info_set( name, value );
	else
	{
		pfc::stringcvt::string_utf8_from_ansi _name(name), _value(value);
		p_info.info_set( _name, _value );
	}
}

void meta_add( file_info & p_info, const char * name, const char * value, bool utf8 )
{
	pfc::stringcvt::string_utf8_from_ansi _name, _value;
	if ( !utf8 )
	{
		_name.convert( name );
		_value.convert( value );
		name = _name;
		value = _value;
	}
	if (p_info.meta_get_count_by_name(name))
	{
		// append as another line
		pfc::string8 final = p_info.meta_get(name, 0);
		final += "\r\n";
		final += value;
		p_info.meta_set(name, final);
	}
	else
	{
		p_info.meta_add(name, value);
	}
}

void Usf::ReadFileInfo(file_info & p_info)
{
	pfc::string8 name, arch;
	service_ptr_t<file> m_file;

	char buffer[64];
	char usfFileName[MAX_FILENAME_LENGTH], drive[32], dir[MAX_FILENAME_LENGTH], ext[MAX_FILENAME_LENGTH];
	uint32_t reservedsize = 0, codesize = 0, crc = 0, tagstart = 0, reservestart = 0;
	uint32_t filesize = 0, tagsize = 0;

	if(file_name[0] == 0)
	{
		throw exception_io_no_handler_for_path();
		return;
	}

	try
	{
		if(archive_impl::g_is_unpack_path(file_name) == true)
		{
			archive_impl::g_parse_unpack_path(file_name, arch, name);
			_splitpath(strrchr(file_name, '|') + 1, drive, dir, usfFileName, ext );
			filesystem::g_open_read(m_file, file_name, m_abort);
		}
		else
		{
			if(strncmp(file_name, "file:", 5))
			{
				throw exception_io_unsupported_format();
				return;
			}

			_splitpath( &file_name[7], drive, dir, usfFileName, ext );
			filesystem::g_open_read(m_file, file_name, m_abort);
		}
	}
	catch (exception_aborted)
	{
		return;
	}


	m_file->read(buffer, 4, m_abort);

	if(buffer[0] != 'P' && buffer[1] != 'S' && buffer[2] != 'F' && buffer[3] != 0x21)
	{
		throw exception_io_data_truncation();
		return;
	}

	m_file->read(&reservedsize, 4, m_abort);
	m_file->read(&codesize, 4, m_abort);
	m_file->read(&crc, 4, m_abort);

	filesize = m_file->get_size(m_abort);

	reservestart = 0x10;
	tagstart = reservestart + reservedsize;
	tagsize = filesize - tagstart;

	if(tagsize)
	{
		char * TagBuffer = new char[tagsize + 1];
		char * Tag = new char [50001];
		char Length[64], Fade[64], Utf8[2];
		bool ReadUtf8 = false;

		m_file->seek(tagstart, m_abort);
		m_file->read(Tag, 5, m_abort);

		if(Tag[0] != '[' && Tag[1] != 'T' && Tag[2] != 'A' && Tag[3] != 'G' && Tag[4] != ']')
		{
			throw exception_io_data_truncation();
			delete [] TagBuffer;
			delete [] Tag;
			return;
		}

		m_file->read(TagBuffer, tagsize - 5, m_abort);
		TagBuffer[tagsize - 5] = 0;


		psftag_raw_getvar(TagBuffer, "length", Length, sizeof(Length) - 1);
		psftag_raw_getvar(TagBuffer, "fade", Fade, sizeof(Fade) - 1);

		if(strlen(Length))
		{
			p_info.info_set( "usf_length", Length );
			if(strlen(Fade))
			{
				p_info.info_set( "usf_fade", Fade );
				TrackLength = GetLengthFromString(Length);
				FadeLength = GetLengthFromString(Fade);
				p_info.set_length(TrackLength + FadeLength);
			}
			else
			{
				TrackLength = GetLengthFromString(Length);
				p_info.set_length(TrackLength);
			}
		}
		else
		{
			p_info.set_length(TrackLength);
			//p_info.set_length(180.0f);
		}

		psftag_raw_getvar(TagBuffer, "utf8", Utf8, sizeof(Utf8) - 1);
		if (strlen(Utf8)) ReadUtf8 = Utf8[0] != '0';

		pfc::string8_fastalloc name, value;

		for ( unsigned pos = 0; pos < tagsize; pos++ )
		{
			const char * line_end = strchr( TagBuffer + pos, '\n' );
			if ( !line_end ) line_end = TagBuffer + tagsize;
			name.set_string( TagBuffer + pos, line_end - TagBuffer - pos );
			pos = line_end - TagBuffer;
			unsigned equals = name.find_first( '=' );
			if ( equals == ~0 ) continue;
			value.set_string( name.get_ptr() + equals + 1 );
			name.truncate( equals );
			trim_whitespace( name );
			trim_whitespace( value );

			if ( !pfc::stricmp_ascii( name, "game" ) ) name = "album";
			else if ( !pfc::stricmp_ascii( name, "year" ) ) name = "date";
			else if ( !pfc::stricmp_ascii( name, "usfby" ) ) name = "USF By";
			else if ( !pfc::stricmp_ascii( name, "tagger" ) ) name = "Tagged By";

			if ( !stricmp_utf8_partial( name, "replaygain_" ) ) p_info.info_set_replaygain( name, value );
			else if ( !pfc::stricmp_ascii( name, "length" ) || !pfc::stricmp_ascii( name, "fade" ) ) {} // already assigned to info fields above
			else if ( !pfc::stricmp_ascii( name, "utf8" ) ) {} // also retrieved above
			else if ( name[0] == '_' ) info_set( p_info, name, value, ReadUtf8 );
			else meta_add( p_info, name, value, ReadUtf8 );
		}

		p_info.info_set_int("channels", 2);
		p_info.info_set_int("bitspersample", 16);
		p_info.info_set("encoding", "synthesized");

		delete [] TagBuffer;
		delete [] Tag;

	}

}


void Usf::Seek(double Time)
{
	if(Time > cpu->audio->Time)
	{
		Seeking = true;

		cpu->audio->SeekTime = Time;
	}
	else
	{

		cpu->LoadCpu(savestate, rom_pages);
		cpu->StartCPU();

		Seeking = true;
		cpu->audio->SeekTime = 0.0;
		cpu->audio->SeekTime = Time;

	}
}


char * Usf::GetTags(void)
{
	service_ptr_t<file> m_file;

	char buffer[64];
	uint32_t reservedsize = 0, codesize = 0, crc = 0, tagstart = 0, reservestart = 0;
	uint32_t filesize = 0, tagsize = 0;

	if(file_name[0] == 0)
	{
		throw exception_io_no_handler_for_path();
		return NULL;
	}

	try
	{
		filesystem::g_open_read(m_file, file_name, m_abort);
	}
	catch (exception_aborted)
	{
		return NULL;
	}


	m_file->read(buffer, 4, m_abort);

	if(buffer[0] != 'P' && buffer[1] != 'S' && buffer[2] != 'F' && buffer[3] != 0x21)
	{
		throw exception_io_data_truncation();
		return NULL;
	}

	m_file->read(&reservedsize, 4, m_abort);
	m_file->read(&codesize, 4, m_abort);
	m_file->read(&crc, 4, m_abort);

	filesize = m_file->get_size(m_abort);

	reservestart = 0x10;
	tagstart = reservestart + reservedsize;
	tagsize = filesize - tagstart;

	if(tagsize)
	{
		/* Nice big buffer, big enough for editing ;) */
		char * TagBuffer = new char[0x100000];

		memset(TagBuffer, 0, 0x100000);

		m_file->seek(tagstart, m_abort);
		m_file->read(buffer, 5, m_abort);

		if(buffer[0] != '[' && buffer[1] != 'T' && buffer[2] != 'A' && buffer[3] != 'G' && buffer[4] != ']')
		{
			throw exception_io_data_truncation();
			delete [] TagBuffer;
			return NULL;
		}

		m_file->read(TagBuffer, tagsize - 5, m_abort);
		TagBuffer[tagsize - 5] = 0;

		return TagBuffer;
	}

	return NULL;
}


void Usf::SetTags(const char * Tags, t_filestats * end_stats)
{
	service_ptr_t<file> m_file;

	char buffer[64];
	uint32_t reservedsize = 0, codesize = 0, crc = 0, tagstart = 0, reservestart = 0;
	uint32_t filesize = 0, tagsize = 0;

	if(file_name[0] == 0)
	{
		throw exception_io_no_handler_for_path();
		return;
	}

	try
	{
		filesystem::g_open(m_file, file_name, filesystem::open_mode_write_existing, m_abort);
	}
	catch (exception_aborted)
	{
		return;
	}


	m_file->read(buffer, 4, m_abort);

	if(buffer[0] != 'P' && buffer[1] != 'S' && buffer[2] != 'F' && buffer[3] != 0x21)
	{
		throw exception_io_data_truncation();
		return;
	}

	m_file->read(&reservedsize, 4, m_abort);
	m_file->read(&codesize, 4, m_abort);
	m_file->read(&crc, 4, m_abort);

	filesize = m_file->get_size(m_abort);

	reservestart = 0x10;
	tagstart = reservestart + reservedsize;
	tagsize = filesize - tagstart;

	const char * TagHeader = "[TAG]";

	m_file->seek(tagstart, m_abort);

	m_file->write(TagHeader, strlen(TagHeader), m_abort);
	m_file->write(Tags, strlen(Tags), m_abort);

	m_file->set_eof(m_abort);

	*end_stats = m_file->get_stats(m_abort);
}
