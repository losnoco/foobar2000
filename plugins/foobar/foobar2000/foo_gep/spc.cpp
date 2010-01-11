#include "base.h"
#include "reader.h"
#include "config.h"

#include <Spc_Emu.h>

#include "../helpers/window_placement_helper.h"

#include "resource.h"

#undef HEADER_STRING
#define HEADER_STRING(i,n,f) if ((f)[0]) (i).meta_set_ansi((n), string_simple((f), sizeof((f))))

static const char field_length[]="spc_length";
static const char field_fade[]="spc_fade";

// {65753F0B-DADE-4341-BDE7-89564C7B5596}
static const GUID guid_cfg_placement = 
{ 0x65753f0b, 0xdade, 0x4341, { 0xbd, 0xe7, 0x89, 0x56, 0x4c, 0x7b, 0x55, 0x96 } };

// This remembers the editor window's position if user enables "Remember window position" in player UI settings
static cfg_window_placement cfg_placement(guid_cfg_placement);

//based on Uematsu.h from SuperJukebox


//Sub-chunk ID's
#define	XID_SONG		0x01
#define	XID_GAME		0x02
#define	XID_ARTIST	0x03
#define	XID_DUMPER	0x04
#define	XID_DATE		0x05
#define	XID_EMU		0x06
#define	XID_CMNTS	0x07
#define	XID_INTRO	0x30
#define	XID_LOOP		0x31
#define	XID_END		0x32
#define	XID_FADE		0x33
#define	XID_MUTE		0x34
#define	XID_OST		0x10
#define	XID_DISC		0x11
#define	XID_TRACK	0x12
#define	XID_PUB		0x13
#define	XID_COPY		0x14

//Data types
#define	XTYPE_DATA	0x00
#define	XTYPE_STR	0x01
#define	XTYPE_INT	0x04
#define	XTYPE_BCD	0x10

typedef struct _SUBCHK
{
	unsigned char	id;                        //Subchunk ID
	unsigned char	type;                      //Type of data
	unsigned short	data;                      //Data
}SUBCHK,*PSUBCHK,FAR *LPSUBCHK;

typedef struct _ID666TAG
{
	char szTitle[256];			//Title of song
	char szGame[256];			//Name of game
	char szArtist[256];		//Name of artist
	char szPublisher[256];		//Game publisher
	char szDumper[256];		//Name of dumper
	char szDate[256];			//Date dumped
	char szComment[256];		//Optional comment
	char szOST[256];			//Original soundtrack title
	UINT uSong_ms;				//Length of song
	UINT uLoop_ms;				//Length of loop
	UINT uEnd_ms;				//Length of end
	UINT uFade_ms;				//Length of fadeout
	BYTE bDisc;					//OST disc number
	WORD wTrack;				//OST track number
	WORD wCopyright;			//Game copyright date
	BYTE bMute;					//Bitmask for channel states	
	BYTE bEmulator;				//Emulator used to dump
}ID666TAG,*PID666TAG,FAR *LPID666TAG;

static void SetDate(LPSTR lpszDate,int year,int month,int day)
{
	if(year<100)
	{
		year+=1900;
//		if(year<1997)year+=100;
	}
//	if(year<1997)year=0;
	if(month<1||month>12)year=0;
	if(day<1||day>31)year=0;

	if(year)
	{
/*	What the bloody fuck is this?
		if(month>2)
		{
			if(!(year&3)&&year%100)day++;
			else if(!(year%400))day++;
		}
*/
		string8 temp;
		temp.add_int(month);
		temp.add_char('/');
		temp.add_int(day);
		temp.add_char('/');
		temp.add_int(year);
		strcpy(lpszDate, temp);
	}
	else lpszDate[0]='\0';
}

static bool load_id666(const service_ptr_t<file> & p_file,LPID666TAG lpTag, abort_callback & p_abort)//must be seeked to correct spot before calling
{
	bool ret = false;
	DWORD dwBytesRead;
	SUBCHK sub;
	char szBuf[5]={0};
	p_file->read_object_e((char*)szBuf, 4, p_abort);

	if(!stricmp(szBuf, "xid6"))
	{
		{
			DWORD zzz;
			p_file->read_object_e(&zzz, 4, p_abort);
		}
		dwBytesRead = p_file->read_e(&sub, sizeof(sub), p_abort);

#define ReadFile(a,b,c,d,e) (*d)=p_file->read_e((b), c, p_abort)

		while(dwBytesRead && sub.data <= (p_file->get_size_e(p_abort) - p_file->get_position_e(p_abort)))
		{
			switch(sub.id)
			{
			case XID_SONG:
				ReadFile(hFile,lpTag->szTitle,sub.data,&dwBytesRead,NULL);
				break;
			case XID_GAME:
				ReadFile(hFile,lpTag->szGame,sub.data,&dwBytesRead,NULL);
				break;
			case XID_ARTIST:
				ReadFile(hFile,lpTag->szArtist,sub.data,&dwBytesRead,NULL);
				break;
			case XID_DUMPER:
				ReadFile(hFile,lpTag->szDumper,sub.data,&dwBytesRead,NULL);
				break;
			case XID_DATE:
				UINT uDate;
				ReadFile(hFile,&uDate,sub.data,&dwBytesRead,NULL);
				SetDate(lpTag->szDate,uDate>>16,(uDate>>8)&0xFF,uDate&0xFF);
				break;
			case XID_EMU:
				lpTag->bEmulator=(BYTE)sub.data;
				break;
			case XID_CMNTS:
				ReadFile(hFile,lpTag->szComment,sub.data,&dwBytesRead,NULL);
				break;
			case XID_OST:
				ReadFile(hFile,lpTag->szOST,sub.data,&dwBytesRead,NULL);
				break;
			case XID_DISC:
				lpTag->bDisc=(BYTE)sub.data;
				break;
			case XID_TRACK:
				lpTag->wTrack=(WORD)sub.data;
				break;
			case XID_PUB:
				ReadFile(hFile,lpTag->szPublisher,sub.data,&dwBytesRead,NULL);
				break;
			case XID_COPY:
				lpTag->wCopyright=(WORD)sub.data;
				break;
			case XID_INTRO:
				ReadFile(hFile,&lpTag->uSong_ms,sub.data,&dwBytesRead,NULL);
				if(lpTag->uSong_ms>61376000)lpTag->uSong_ms=61376000;
				lpTag->uSong_ms/=64;
				break;
			case XID_LOOP:
				ReadFile(hFile,&lpTag->uLoop_ms,sub.data,&dwBytesRead,NULL);
				if(lpTag->uLoop_ms>383936000)lpTag->uLoop_ms=383936000;
				lpTag->uLoop_ms/=64;
				break;
			case XID_END:
				if (sub.data == sizeof(lpTag->uEnd_ms))
				{
					ReadFile(hFile,&lpTag->uEnd_ms,sub.data,&dwBytesRead,NULL);
					if(lpTag->uEnd_ms>61376000)lpTag->uEnd_ms=61376000;
					lpTag->uEnd_ms/=64;
				}
				else
				{
					p_file->seek2_e(sub.data, SEEK_CUR, p_abort);
				}
				break;
			case XID_FADE:
				ReadFile(hFile,&lpTag->uFade_ms,sub.data,&dwBytesRead,NULL);
				if(lpTag->uFade_ms>3839360)lpTag->uFade_ms=3839360;
				lpTag->uFade_ms/=64;
				break;
			case XID_MUTE:
				lpTag->bMute=(BYTE)sub.data;
				break;
			default:
				if(sub.type)
				{
					char foo[0x100];
					UINT size=sub.data;
					while(size)
					{
						UINT delta=size;
						if (delta>0x100) delta=0x100;
						p_file->read_e(foo, delta, p_abort);
						size-=delta;
					}
				}
				break;
			}
			ReadFile(hFile,&sub,sizeof(sub),&dwBytesRead,NULL);
		}
#undef ReadFile
		ret = true;
	}
	if ((!lpTag->uSong_ms && !lpTag->uFade_ms)/* || cfg_ignore_time*/)
	{
		lpTag->uSong_ms=0;//cfg_def_song;
		lpTag->uFade_ms=0;//cfg_def_fade;
	}
	return ret;
}

class input_spc : public input_gep
{
	Spc_Emu::header_t m_header;
	file_info_impl    m_info;
	bool              retagging;

public:
	input_spc()
	{
		sample_rate = Spc_Emu::native_sample_rate;
	}

	static bool g_is_our_path( const char * p_path, const char * p_extension )
	{
		return ! stricmp( p_extension, "spc" );
	}

	t_io_result open( service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		assert(sample_rate == Spc_Emu::native_sample_rate);

		t_io_result status = input_gep::open( p_filehint, p_path, p_reason, p_abort );
		if ( io_result_failed( status ) ) return status;

		foobar_File_Reader rdr(m_file, p_abort);

		try
		{
			ERRCHK( rdr.read( &m_header, sizeof(m_header) ) );

			if ( strncmp( m_header.tag, "SNES-SPC700 Sound File Data", 27 ) != 0 )
			{
				console::info("Not an SPC file");
				return io_result_error_data;
			}

			t_io_result status = tag_processor::read_trailing( m_file, m_info, p_abort );
			if ( status != io_result_error_data && status != io_result_error_not_found && io_result_failed( status ) ) return status;

			if ( status != io_result_error_not_found )
			{
				const char * p;
				p = m_info.meta_get( field_length, 0 );
				if (p)
				{
					m_info.info_set( field_length, p );
					tag_song_ms = atoi( p );
					m_info.meta_remove_field( field_length );
				}
				else
				{
					tag_song_ms = 0;
				}
				p = m_info.meta_get( field_fade, 0 );
				if ( p )
				{
					m_info.info_set( field_fade,p );
					tag_fade_ms = atoi( p );
					m_info.meta_remove_field( field_fade );
				}
				else
				{
					tag_fade_ms = 0;
				}
			}
			else
			{
				ID666TAG tag;

				memset(&tag, 0, sizeof(tag));

				HEADER_STRING(m_info, "title", m_header.song);
				HEADER_STRING(m_info, "album", m_header.game);
				HEADER_STRING(m_info, "dumper", m_header.dumper);
				HEADER_STRING(m_info, "comment", m_header.comment);
				HEADER_STRING(m_info, "date", (const char *)&m_header.date);
				HEADER_STRING(m_info, "artist", m_header.author);

				tag_song_ms = atoi(string_simple(m_header.len_secs, sizeof(m_header.len_secs))) * 1000;
				tag_fade_ms = atoi(string_simple((const char *)&m_header.fade_msec, sizeof(m_header.fade_msec)));
				voice_mask = m_header.mute_mask;

				try
				{
					m_file->seek_e( 66048, p_abort );
					if ( load_id666( m_file, & tag, p_abort ) )
					{
						char temp[16];

						HEADER_STRING(m_info, "title", tag.szTitle);
						HEADER_STRING(m_info, "album", tag.szGame);
						HEADER_STRING(m_info, "artist", tag.szArtist);
						HEADER_STRING(m_info, "dumper", tag.szDumper);
						HEADER_STRING(m_info, "date", tag.szDate);
						HEADER_STRING(m_info, "comment", tag.szComment);

						HEADER_STRING(m_info, "OST", tag.szOST);
						HEADER_STRING(m_info, "publisher", tag.szPublisher);

						if (tag.wTrack > 0)
						{
							itoa( ( ( tag.wTrack & 0xFF00 ) >> 8 ) | ( ( tag.wTrack & 0x00FF ) << 8 ), temp, 10 );
							m_info.meta_set("tracknumber", temp);
						}
						if (tag.bDisc > 0)
						{
							itoa( tag.bDisc, temp, 10 );
							m_info.meta_set("disc", temp);
						}

						if (tag.uSong_ms) tag_song_ms = tag.uSong_ms;
						if (tag.uFade_ms) tag_fade_ms = tag.uFade_ms;
						voice_mask = tag.bMute;
					}
				}
				catch(t_io_result /*code*/)
				{
					//return code;
					// failed to read id666 tag? do nothing...
				}

				if (tag_song_ms > 0) m_info.info_set_int(field_length, tag_song_ms);
				if (tag_fade_ms > 0) m_info.info_set_int(field_fade, tag_fade_ms);
			}

			if (!tag_song_ms)
			{
				tag_song_ms = cfg_default_length;
				tag_fade_ms = cfg_default_fade;
			}
		}
		catch ( t_io_result code )
		{
			return code;
		}

		bool retagging = p_reason == input_open_info_write;

		return io_result_success;
	}

	t_io_result get_info( t_uint32 p_subsong, file_info & p_info, abort_callback & p_abort )
	{
		p_info.copy( m_info );

		p_info.info_set("codec", "SPC");

		p_info.info_set_int("samplerate", Spc_Emu::native_sample_rate );
		p_info.info_set_int("channels", 2 );
		p_info.info_set_int("bitspersample", 16 );

		p_info.set_length(double(tag_song_ms + tag_fade_ms) * .001);

		return io_result_success;
	}

	t_io_result decode_initialize( t_uint32 p_subsong, unsigned p_flags, abort_callback & p_abort )
	{
		Spc_Emu * emu = ( Spc_Emu * ) this->emu;
		if ( ! emu )
		{
			emu = new Spc_Emu;
			if ( ! emu )
			{
				console::info("Out of memory");
				return io_result_error_out_of_memory;
			}
			this->emu = emu;

			try
			{
				m_file->seek_e( 0, p_abort );
				foobar_File_Reader rdr( m_file, p_abort );
				rdr.skip( sizeof( m_header ) );

				ERRCHK( emu->init( Spc_Emu::native_sample_rate ) );
				ERRCHK( emu->load( m_header, rdr ) );
			}
			catch ( t_io_result code )
			{
				return code;
			}

			if ( ! retagging ) m_file.release();

			emu->mute_voices( voice_mask );

			emu->disable_surround(cfg_spc_anti_surround);
		}

		return input_gep::decode_initialize( 0, p_flags, p_abort );
	}

	virtual t_io_result set_info(const service_ptr_t<file> & p_reader,const playable_location & p_location,file_info & p_info,abort_callback & p_abort)
	{
		try
		{
			m_file->seek_e( 66048, p_abort );
			m_file->set_eof_e( p_abort );
		}
		catch(t_io_result code)
		{
			return code;
		}

		m_info.copy( p_info );

		file_info_impl l_info;
		l_info.copy( p_info );

		{
			const char * p;
			p = l_info.info_get( field_length );
			if (p)
			{
				l_info.meta_set( field_length, p );
			}
			p = l_info.info_get( field_fade );
			if (p)
			{
				l_info.meta_set( field_fade, p );
			}
		}

		t_io_result status = tag_processor::write_apev2( m_file, l_info, p_abort );
		if ( io_result_failed( status ) ) return status;

		return m_file->get_stats( m_stats, p_abort );
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
			INFOSTRUCT * i = (INFOSTRUCT*) lp;
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

static bool context_time_dialog(unsigned *song_ms, unsigned *fade_ms)
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

class context_spc : public menu_item_legacy_context
{
public:
	virtual unsigned get_num_items() { return 1; }

	virtual void get_item_name(unsigned n, string_base & out)
	{
		out = "Edit length";
	}

	virtual void get_item_default_path(unsigned n, string_base & out)
	{
		out.reset();
	}

	virtual bool get_item_description(unsigned n, string_base & out)
	{
		out = "Edits the length of the selected SPC file, or sets the length of all selected SPC files.";
		return true;
	}

	virtual GUID get_item_guid(unsigned n)
	{
		// {1B41F297-E794-42ac-AC56-0111D99A238E}
		static const GUID guid = 
		{ 0x1b41f297, 0xe794, 0x42ac, { 0xac, 0x56, 0x1, 0x11, 0xd9, 0x9a, 0x23, 0x8e } };
		return guid;
	}

	virtual bool context_get_display(unsigned n,const list_base_const_t<metadb_handle_ptr> & data,string_base & out,unsigned & displayflags,const GUID &)
	{
		unsigned i, j;
		i = data.get_count();
		for (j = 0; j < i; j++)
		{
			const playable_location & foo = data.get_item(j)->get_location();
			if (stricmp(string_extension_8(foo.get_path()), "spc")) return false;
		}
		if (i == 1) out = "Edit length";
		else out = "Set length";
		return true;
	}

	virtual void context_command(unsigned n,const list_base_const_t<metadb_handle_ptr> & data,const GUID& caller)
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
			//foo->metadb_lock();
			if (foo->get_info(info))
			{
				if (tag_song_ms > 0) info.info_set_int(field_length, tag_song_ms);
				else info.info_remove(field_length);
				if (tag_fade_ms > 0) info.info_set_int(field_fade, tag_fade_ms);
				else info.info_remove(field_fade);
				{
					if (!tag_song_ms)
					{
						tag_song_ms = cfg_default_length;
						tag_fade_ms = cfg_default_fade;
					}
					double length = (double)(tag_song_ms + tag_fade_ms) * .001;
					info.set_length(length);
				}
				if (io_result_failed(p_imgr->update_info(foo, info, core_api::get_main_window(), true))) j = i;
			}
			else j = i;
			//foo->metadb_unlock();
		}
	}
};

DECLARE_FILE_TYPE("SPC files", "*.SPC");

static input_factory_t    <input_spc>   g_input_spc_factory;
static menu_item_factory_t<context_spc> g_menu_item_context_spc_factory;
