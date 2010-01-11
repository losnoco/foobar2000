#include "base.h"
#include "reader.h"
#include "config.h"

#include <gme/Spc_Emu.h>

#include "../helpers/window_placement_helper.h"

#include "resource.h"

#undef HEADER_STRING
#define HEADER_STRING(i,n,f) if ((f)[0]) (i).meta_set((n), pfc::stringcvt::string_utf8_from_ansi((f), sizeof((f))))

static const char field_length[]="spc_length";
static const char field_fade[]="spc_fade";

// {65753F0B-DADE-4341-BDE7-89564C7B5596}
static const GUID guid_cfg_placement = 
{ 0x65753f0b, 0xdade, 0x4341, { 0xbd, 0xe7, 0x89, 0x56, 0x4c, 0x7b, 0x55, 0x96 } };

// This remembers the editor window's position if user enables "Remember window position" in player UI settings
static cfg_window_placement cfg_placement(guid_cfg_placement);

//based on Uematsu.h from SuperJukebox


//Sub-chunk ID's
#define	XID_SONG    0x01
#define	XID_GAME    0x02
#define	XID_ARTIST  0x03
#define	XID_DUMPER  0x04
#define	XID_DATE    0x05
#define	XID_EMU     0x06
#define	XID_CMNTS   0x07
#define	XID_INTRO   0x30
#define	XID_LOOP    0x31
#define	XID_END     0x32
#define	XID_FADE    0x33
#define	XID_MUTE    0x34
#define XID_LOOPX   0x35
#define XID_AMP     0x36
#define	XID_OST     0x10
#define	XID_DISC    0x11
#define	XID_TRACK   0x12
#define	XID_PUB     0x13
#define	XID_COPY    0x14

//Data types
#define	XTYPE_DATA	0x00
#define	XTYPE_STR	0x01
#define	XTYPE_INT	0x04

typedef struct _ID666TAG
{
	char     szTitle[256];     //Title of song
	char     szGame[256];      //Name of game
	char     szArtist[256];    //Name of artist
	char     szPublisher[256]; //Game publisher
	char     szDumper[256];    //Name of dumper
	char     szDate[256];      //Date dumped
	char     szComment[256];   //Optional comment
	char     szOST[256];       //Original soundtrack title
	t_uint32 uSong_ms;         //Length of song
	t_uint32 uLoop_ms;         //Length of loop
	t_uint32 uLoopCount;       //Loop count
	t_uint32 uEnd_ms;          //Length of end
	t_uint32 uFade_ms;         //Length of fadeout
	t_uint8  bDisc;            //OST disc number
	t_uint16 wTrack;           //OST track number
	t_uint16 wCopyright;       //Game copyright date
	t_uint8  bMute;            //Bitmask for channel states	
	t_uint8  bEmulator;        //Emulator used to dump
	t_uint32 uAmplification;   //Amplification level
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
		pfc::string8 temp;
		temp << month;
		temp.add_char('/');
		temp << day;
		temp.add_char('/');
		temp << year;
		strcpy(lpszDate, temp);
	}
	else lpszDate[0]='\0';
}

static void parse_id666( service_ptr_t< file > & p_file, LPID666TAG lpTag, bool aligned, abort_callback & p_abort )
{
	while ( ! p_file->is_eof( p_abort ) )
	{
		t_uint8 id, type;
		t_uint16    data;

		p_file->read_object_t( id, p_abort );
		p_file->read_object_t( type, p_abort );
		p_file->read_lendian_t( data, p_abort );

		switch ( type )
		{
		case XTYPE_STR:
			{
				if ( data < 1 || data > 256 ) throw exception_io_data();

				switch ( id )
				{
				case XID_SONG:
					p_file->read_object( lpTag->szTitle, data, p_abort );
					break;

				case XID_GAME:
					p_file->read_object( lpTag->szGame, data, p_abort );
					break;

				case XID_ARTIST:
					p_file->read_object( lpTag->szArtist, data, p_abort );
					break;

				case XID_PUB:
					p_file->read_object( lpTag->szPublisher, data, p_abort );
					break;

				case XID_OST:
					p_file->read_object( lpTag->szOST, data, p_abort );
					break;

				case XID_DUMPER:
					p_file->read_object( lpTag->szDumper, data, p_abort );
					break;

				case XID_CMNTS:
					p_file->read_object( lpTag->szComment, data, p_abort );
					break;

				default:
					p_file->skip( data, p_abort );
					break;
				}

				if ( aligned && ( data & 3 ) ) p_file->skip( 4 - ( data & 3 ), p_abort );
			}
			break;

		case XTYPE_INT:
			{
				if ( data != 4 ) throw exception_io_data();

				t_uint32 value;
				p_file->read_lendian_t( value, p_abort );

				switch ( id )
				{
				case XID_DATE:
					SetDate( lpTag->szDate, ( value >> 16 ) & 255, ( value >> 8 ) & 255, value & 255 );
					break;

				case XID_INTRO:
					if ( value > 383999999 ) value = 383999999;
					lpTag->uSong_ms = value / 64;
					break;

				case XID_LOOP:
					if ( value > 383999999 ) value = 383999999;
					lpTag->uLoop_ms = value / 64;
					break;

				case XID_END:
					if ( value > 383999999 ) value = 383999999;
					lpTag->uEnd_ms = value / 64;
					break;

				case XID_FADE:
					if ( value > 3839999 ) value = 3839999;
					lpTag->uFade_ms = value / 64;
					break;

				case XID_AMP:
					if ( value < 32768 ) value = 32768;
					else if ( value > 524288 ) value = 524288;
					lpTag->uAmplification = value;
					break;
				}
			}
			break;

		case XTYPE_DATA:
			{
				switch ( id )
				{
				case XID_EMU:
					lpTag->bEmulator = t_uint8( data );
					break;

				case XID_DISC:
					lpTag->bDisc = t_uint8( data );
					if ( lpTag->bDisc > 9 ) lpTag->bDisc = 9;
					break;

				case XID_TRACK:
					if ( data > ( ( 100 << 8 ) - 1 ) ) data = 0;
					lpTag->wTrack = data;
					break;

				case XID_COPY:
					lpTag->wCopyright = data;
					break;

				case XID_MUTE:
					lpTag->bMute = t_uint8( data );
					break;

				case XID_LOOPX:
					if ( data < 1 ) data = 1;
					else if ( data > 9 ) data = 9;
					lpTag->uLoopCount = data;
					break;

				case XID_AMP:
					lpTag->uAmplification = data;
					lpTag->uAmplification <<= 12;
					if ( lpTag->uAmplification < 32768 ) lpTag->uAmplification = 32768;
					else if ( lpTag->uAmplification > 524288 ) lpTag->uAmplification = 524288;
					break;
				}
			}
			break;
		}
	}
}

static void load_id666(service_ptr_t<file> & p_file, LPID666TAG lpTag, abort_callback & p_abort)//must be seeked to correct spot before calling
{
	t_uint8 szBuf[4];
	p_file->read_object( &szBuf, 4, p_abort );

	static t_uint8 signature[] = {'x', 'i', 'd', '6'};

	if( ! memcmp( szBuf, signature, 4 ) )
	{
		t_uint32 tag_size;
		p_file->read_lendian_t( tag_size, p_abort );

		t_filesize offset = p_file->get_position( p_abort );

		service_ptr_t< reader_limited > m_file = new service_impl_t< reader_limited >;
		m_file->init( p_file, offset, offset + tag_size, p_abort );

		service_ptr_t<file> p_file = m_file.get_ptr();

		try
		{
			parse_id666( p_file, lpTag, false, p_abort );
		}
		catch ( const exception_io_data & )
		{
			p_file->seek( 0, p_abort );

			memset( lpTag, 0, sizeof( *lpTag ) );

			parse_id666( p_file, lpTag, true, p_abort );
		}
	}
	else throw exception_tag_not_found();
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

	void open( service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		assert(sample_rate == Spc_Emu::native_sample_rate);

		input_gep::open( p_filehint, p_path, p_reason, p_abort );

		foobar_File_Reader rdr(m_file, p_abort);

		{
			ERRCHK( rdr.read( &m_header, sizeof(m_header) ) );

			if ( strncmp( m_header.tag, "SNES-SPC700 Sound File Data", 27 ) != 0 )
			{
				console::info("Not an SPC file");
				throw exception_io_data();
			}

			bool valid_tag = false;

			try
			{
				tag_processor::read_trailing( m_file, m_info, p_abort );

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

				valid_tag = true;
			}
			catch ( const exception_tag_not_found & ) {}
			catch ( const exception_io_data & ) {}

			if ( ! valid_tag )
			{
				HEADER_STRING(m_info, "title", m_header.song);
				HEADER_STRING(m_info, "album", m_header.game);
				HEADER_STRING(m_info, "dumper", m_header.dumper);
				HEADER_STRING(m_info, "comment", m_header.comment);
				//HEADER_STRING(m_info, "date", m_header.date);
				if ((m_header.date)[0]) m_info.meta_set("date", pfc::stringcvt::string_utf8_from_ansi(((const char *)&m_header.date), sizeof((m_header.date))));
				HEADER_STRING(m_info, "artist", m_header.author);

				tag_song_ms = atoi(pfc::string_simple(m_header.len_secs, sizeof(m_header.len_secs))) * 1000;
				tag_fade_ms = atoi(pfc::string_simple((const char *)&m_header.fade_msec, sizeof(m_header.fade_msec)));
				voice_mask = m_header.mute_mask;

				try
				{
					ID666TAG tag;

					memset(&tag, 0, sizeof(tag));

					m_file->seek( 66048, p_abort );
					load_id666( m_file, & tag, p_abort );

					HEADER_STRING(m_info, "title", tag.szTitle);
					HEADER_STRING(m_info, "album", tag.szGame);
					HEADER_STRING(m_info, "artist", tag.szArtist);
					HEADER_STRING(m_info, "dumper", tag.szDumper);
					HEADER_STRING(m_info, "date", tag.szDate);
					HEADER_STRING(m_info, "comment", tag.szComment);

					HEADER_STRING(m_info, "OST", tag.szOST);
					HEADER_STRING(m_info, "publisher", tag.szPublisher);

					if ( tag.wTrack > 255 )
					{
						pfc::string8 temp;
						temp = pfc::format_int( tag.wTrack >> 8 );
						if ( tag.wTrack & 255 )
						{
							char foo[ 2 ] = { tag.wTrack & 255, 0 };
							temp << pfc::stringcvt::string_utf8_from_ansi( foo );
						}
						m_info.meta_set("tracknumber", temp);
					}

					if ( tag.bDisc > 0 )
						m_info.meta_set("disc", pfc::format_int( tag.bDisc ) );

					if (tag.uSong_ms) tag_song_ms = tag.uSong_ms;
					if (tag.uFade_ms) tag_fade_ms = tag.uFade_ms;
					voice_mask = tag.bMute;
				}
				catch ( const exception_tag_not_found & ) {}
				catch ( const exception_io_data & ) {}

				if (tag_song_ms > 0) m_info.info_set_int(field_length, tag_song_ms);
				if (tag_fade_ms > 0) m_info.info_set_int(field_fade, tag_fade_ms);
			}

			if (!tag_song_ms)
			{
				tag_song_ms = cfg_default_length;
				tag_fade_ms = cfg_default_fade;
			}
		}

		retagging = p_reason == input_open_info_write;
	}

	void get_info( t_uint32 p_subsong, file_info & p_info, abort_callback & p_abort )
	{
		p_info.copy( m_info );

		p_info.info_set( "codec", "SPC" );

		p_info.info_set_int( "samplerate", Spc_Emu::native_sample_rate );
		p_info.info_set_int( "channels", 2 );
		p_info.info_set_int( "bitspersample", 16 );

		p_info.set_length( double( tag_song_ms + tag_fade_ms ) * .001 );
	}

	void decode_initialize( t_uint32 p_subsong, unsigned p_flags, abort_callback & p_abort )
	{
		Spc_Emu * emu = ( Spc_Emu * ) this->emu;
		if ( ! emu )
		{
			this->emu = emu = new Spc_Emu;

			try
			{
				m_file->seek( 0, p_abort );
				foobar_File_Reader rdr( m_file, p_abort );
				rdr.skip( sizeof( m_header ) );

				ERRCHK( emu->set_sample_rate( Spc_Emu::native_sample_rate ) );
				ERRCHK( emu->load( m_header, rdr ) );
			}
			catch(...)
			{
				if ( emu )
				{
					delete emu;
					this->emu = emu = NULL;
				}
				throw;
			}

			if ( ! retagging ) m_file.release();

			emu->mute_voices( voice_mask );

			emu->disable_surround( !! ( cfg_spc_anti_surround ) );
			static bool woot = false;
			woot = !woot;
			if ( woot ) emu->set_cubic_interpolation();
		}

		input_gep::decode_initialize( 0, p_flags, p_abort );

		first_block = false;
	}

	void retag_set_info( t_uint32 p_subsong, const file_info & p_info, abort_callback & p_abort )
	{
		m_file->seek( 66048, p_abort );
		m_file->set_eof( p_abort );

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

		tag_processor::write_apev2( m_file, l_info, p_abort );

		m_stats = m_file->get_stats( p_abort );
	}

	void retag_commit( abort_callback & p_abort )
	{
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

class context_spc : public contextmenu_item_simple
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

	virtual bool context_get_display( unsigned n, const pfc::list_base_const_t< metadb_handle_ptr > & data, pfc::string_base & out, unsigned & displayflags, const GUID & )
	{
		unsigned i, j;
		i = data.get_count();
		for (j = 0; j < i; j++)
		{
			const playable_location & foo = data.get_item(j)->get_location();
			if ( stricmp( pfc::string_extension( foo.get_path() ), "spc" ) ) return false;
		}
		if (i == 1) out = "Edit length";
		else out = "Set length";
		return true;
	}

	virtual void context_command( unsigned n, const pfc::list_base_const_t< metadb_handle_ptr > & data, const GUID& )
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
				if ( metadb_io::update_info_success != p_imgr->update_info( foo, info, core_api::get_main_window(), true ) ) j = i;
			}
			else j = i;
			//foo->metadb_unlock();
		}
	}
};

DECLARE_FILE_TYPE("SPC files", "*.SPC");

static input_factory_t           <input_spc>   g_input_spc_factory;
static contextmenu_item_factory_t<context_spc> g_contextmenu_item_spc_factory;
