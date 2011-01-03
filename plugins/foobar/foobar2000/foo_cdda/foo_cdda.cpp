#include "../SDK/foobar2000.h"
#include "resource.h"

// {DE32CD47-E74A-4929-88D8-BF473E5B16FD}
static const GUID guid_cfg_limit_speed = 
{ 0xde32cd47, 0xe74a, 0x4929, { 0x88, 0xd8, 0xbf, 0x47, 0x3e, 0x5b, 0x16, 0xfd } };

static cfg_int cfg_limit_speed(guid_cfg_limit_speed,0);

/*
  cdda://XXXXXXXX,Y
  X - cdplayer.ini id of TOC, Y - track number
  */

extern "C" {

	#include "akrip/akrip32.h"

	DWORD setCDSpeed( HCDROM hCD, DWORD speed );//Speed is specified in KB/sec: 1x == 176, 2x == 353, 4x == 706
	DWORD genCDPlayerIniIndex( HCDROM hCD );
}

static DWORD rev32(DWORD p)
{
	DWORD ret = 0;
	ret |= (p & 0xFF) << 24;
	ret |= (p & 0xFF00) << 8;
	ret |= (p & 0xFF0000) >> 8;
	ret |= (p & 0xFF000000) >> 24;
	return ret;
}

static bool cdplayer_test_idx( const TOC * toc, DWORD code)
{
	DWORD retVal = 0;
	int i;
	int max_track = toc->lastTrack - toc->firstTrack;

    for( i = 0; i <= max_track; i++ )
    {
		if ( !(toc->tracks[i].ADR&4) )
		{
			DWORD temp = rev32(*(DWORD*)&toc->tracks[i].addr);
			retVal += temp%75;//frames
			temp /= 75;
			retVal += (temp%60)<<8;//seconds
			temp /= 60;
			retVal += (temp)<<16;//minutes
		}
    }

	return (code&0xFF) == (retVal&0xFF) && code >= retVal && code<(retVal+0x100000);
}


static CDLIST g_cdlist;
static critical_section g_access_sync;
static bool g_have_cdlist = false;

class akrip_reference
{
	static unsigned refcount;
	static bool g_succeeded;
	bool l_inited;
	static bool g_crashed;

	static bool g_akrip_init()
	{
		bool rv;
		if (g_crashed) rv = false;
		else __try {
			rv = !!AkripInit();
		} __except(1)
		{
			console::error("akrip init failure");
			g_crashed = true;
			rv = false;
		}
		return rv;
	}
	static bool g_akrip_deinit()
	{
		bool rv;
		if (g_crashed) rv = false;
		else __try
		{
			AkripDeinit();
			rv = true;
		} __except(1)
		{
			console::error("akrip deinit failure");
			g_crashed = true;
			rv = false;
		}
		return rv;
	}
public:
	akrip_reference() : l_inited(false) {}
	bool init()	
	{
		insync(g_access_sync);
		if (!l_inited)
		{
			l_inited = true;
			if (refcount++==0)
			{
				TRACK_CALL_TEXT("AkripInit");
				g_succeeded = g_akrip_init();
			}
		}
		return g_succeeded;
	}

	void deinit()
	{
		insync(g_access_sync);
		if (l_inited)
		{
			l_inited = false;
			if (--refcount == 0)
			{
				TRACK_CALL_TEXT("AkripDeinit");
				g_akrip_deinit();
			}
		}
	}

	~akrip_reference()
	{
		deinit();
	}

	inline bool succeeded() const {return l_inited && g_succeeded;}
};

unsigned akrip_reference::refcount = 0;
bool akrip_reference::g_succeeded = false;
bool akrip_reference::g_crashed = false;

static struct cdhandle
{
	HCDROM handle;
	int refcount;
	bool speed_limited;
} cdhandles[tabsize(g_cdlist.cd)];


static void init_cdlist()
{
	TRACK_CALL_TEXT("init_cdlist");
	insync(g_access_sync);
	if (!g_have_cdlist)
	{
		akrip_reference akref;
		if (akref.init())
		{
			g_have_cdlist = true;
			memset(&g_cdlist,0,sizeof(g_cdlist));
			g_cdlist.max = MAXCDLIST; 		
			GetCDList(&g_cdlist);
			memset(&cdhandles,0,sizeof(cdhandles));
		}
	}
}

static HCDROM cdrom_open(int drive_num)
{
	TRACK_CALL_TEXT("cdrom_open");
	insync(g_access_sync);


	init_cdlist();


	if (drive_num >= g_cdlist.num || drive_num<0) return 0;

	if (cdhandles[drive_num].handle && cdhandles[drive_num].refcount>0)
	{
		cdhandles[drive_num].refcount++;
		return cdhandles[drive_num].handle;
	}

	GETCDHAND gcd;
	memset(&gcd,0,sizeof(gcd));
	gcd.size = sizeof(gcd);
	gcd.ver = 1;
	gcd.readType = CDR_ANY;
	gcd.ha = g_cdlist.cd[drive_num].ha;
	gcd.tgt = g_cdlist.cd[drive_num].tgt;
	gcd.lun = g_cdlist.cd[drive_num].lun;

	HCDROM ret = GetCDHandle(&gcd);
	if (ret!=0)
	{
		cdhandles[drive_num].handle = ret;
		cdhandles[drive_num].refcount = 1;
		bool use_limit = !!cfg_limit_speed;
		cdhandles[drive_num].speed_limited = use_limit;
		if (use_limit) setCDSpeed(ret,353);
	}
	return ret;
}

static void cdrom_close(HCDROM handle)
{
	TRACK_CALL_TEXT("cdrom_close");
	insync(g_access_sync);
	UINT n;
	for(n=0;n<g_cdlist.num;n++)
	{
		if (cdhandles[n].handle==handle)
		{
			if (--cdhandles[n].refcount == 0)
			{
				if (cdhandles[n].speed_limited) setCDSpeed(handle,0xFFFF);
				CloseCDHandle(handle);
				cdhandles[n].handle = 0;
			}
			break;
		}
	}
}

static HCDROM cdrom_from_id(DWORD id)
{
	insync(g_access_sync);

	UINT n;
	for(n=0;n<g_cdlist.num;n++)
	{
		HCDROM handle = cdrom_open(n);
		if (handle != 0)
		{
			if (genCDPlayerIniIndex(handle) == id) return handle;
			cdrom_close(handle);
		}
	}
	return 0;
}


class input_cdda : public input
{
	akrip_reference akref;
	HCDROM handle;
	PTRACKBUF trackbuf;
	int start_frame,end_frame;
	int cur_frame;
	enum {BUF_SIZE = 8, SECTOR = 2352};
	int bytes_to_skip;
	
public:
	t_io_result set_info(const service_ptr_t<file> & p_file,const playable_location & p_location,file_info & p_info,abort_callback & p_abort)
	{
		return io_result_error_data;
	}

	t_io_result get_info(const service_ptr_t<file> & p_file,const playable_location & p_location,file_info & p_info,abort_callback & p_abort)
	{
		TRACK_CALL_TEXT("input_cdda::get_info");
		return open_internal(p_file,p_location,p_info,false,true,p_abort);
	}
	
	t_io_result open(const service_ptr_t<file> & p_file,const playable_location & p_location,file_info & p_info,abort_callback & p_abort,unsigned p_flags)
	{
		TRACK_CALL_TEXT("input_cdda::open");
		return open_internal(p_file,p_location,p_info,true,!!(p_flags & OPEN_FLAG_GET_INFO),p_abort);
	}

	t_io_result open_internal(const service_ptr_t<file> & r,const playable_location & p_location,file_info & info,bool p_decode,bool p_get_info,abort_callback & p_abort)
	{
		const char * path = p_location.get_path();
		if (stricmp_utf8_partial(path,"cdda://")) return io_result_error_data;
		path += 7;

		DWORD toc_id = strtoul(path,0,16);
		while((*path>='0' && *path<='9') || (*path>='A' && *path<='F') || (*path>='a' && *path<='f')) path++;
		while(*path && (*path<'0' || *path>'9')) path++;
		int track_num = atoi(path);

		if (track_num <= 0 || track_num>=100) return io_result_error_data;

		TRACK_CALL_TEXT("input_cdda::open_internal");
		insync(g_access_sync);
		if (!akref.init())
		{
			console::error("Error initializing akrip library.");
			return io_result_error_generic;
		}
		

		if (p_get_info)
		{
			info.meta_set("title",uStringPrintf("cd track %02u",track_num));
			info.meta_set("tracknumber",uStringPrintf("%02u",track_num));
			info.info_set("samplerate","44100");
			info.info_set("bitrate","1411");
			info.info_set("bitspersample","16");
			info.info_set("channels","2");
			info.info_set("codec","CDDA");
		}

		init_cdlist();
		

		handle = cdrom_from_id(toc_id);
		if (handle == 0) return io_result_error_generic;

		TOC toc;
		memset(&toc,0,sizeof(toc));
		if (ReadTOC(handle,&toc,FALSE)!=SS_COMP) return io_result_error_generic;

		if (track_num < (int)toc.firstTrack || track_num > (int)toc.lastTrack)
			return io_result_error_data;

		track_num -= toc.firstTrack;
		if (toc.tracks[track_num].ADR&4) return io_result_error_data;//not audio track

		start_frame = rev32(*(DWORD*)&toc.tracks[track_num].addr);
		if (track_num<99) end_frame = rev32(*(DWORD*)&toc.tracks[track_num+1].addr);
		else end_frame = 66666666;

		if (p_get_info) info.set_length((double)(end_frame - start_frame) * (double)SECTOR / (44100.0 * 4.0));

		if (p_decode)
		{
			cur_frame = start_frame;

			trackbuf = (PTRACKBUF)malloc(TRACKBUFEXTRA+BUF_SIZE * SECTOR);
			
			bytes_to_skip = 0;
		}
		else
		{
			cdrom_close(handle);
			handle = 0;
		}

		return io_result_success;
	}

	inline static bool g_test_filename(const char * full_path,const char * extension)
	{
		return !stricmp_utf8(extension,"cda") || !stricmp_utf8_partial(full_path,"cdda://");
	}

	input_cdda()
	{
		handle = 0;
		trackbuf = 0;
	}
	
	~input_cdda()
	{
		if (handle) cdrom_close(handle);
		if (trackbuf) free(trackbuf);
	}

	inline static bool g_needs_reader() {return false;}

	virtual t_io_result seek(double seconds,abort_callback & p_abort)
	{
		__int64 offset_bytes = (__int64)(44100.0 * 4.0 * seconds);
		bytes_to_skip = (int)(offset_bytes%SECTOR);
		bytes_to_skip -= bytes_to_skip % 4;
		cur_frame = start_frame + (int)(offset_bytes/SECTOR);
		return io_result_success;
	}
	
	virtual t_io_result run(audio_chunk * chunk,abort_callback & p_abort)
	{
		TRACK_CALL_TEXT("input_cdda::get_samples_pcm");
		int delta = end_frame - cur_frame;
		if (delta<=0) return io_result_eof;
		if (delta > BUF_SIZE) delta = BUF_SIZE;

		memset(trackbuf->buf,0,delta * SECTOR);

		bool success = false;
		int rereads;
		bool whined = false;
		for(rereads=10;rereads && !p_abort.is_aborting();rereads--)
		{
			trackbuf->startFrame = cur_frame;
			trackbuf->numFrames = delta;
			trackbuf->maxLen = BUF_SIZE * SECTOR;
			trackbuf->len = 0;
			trackbuf->status = 0;
			trackbuf->startOffset = 0;
			g_access_sync.enter();
			DWORD rv = ReadCDAudioLBA(handle,trackbuf);
			g_access_sync.leave();
			if (rv==SS_COMP) {success = true; break;}
			delta = 1;
			if (!whined)
			{
				console::warning("cd access error, attempting to reread");
				whined = true;
			}
		}

		if (!success)
		{
			console::error("cd access failed");
			return io_result_error_generic;
		}
		else if (whined)
		{
			console::info("reread successful");
		}

		cur_frame += delta;

		chunk->set_data_fixedpoint(trackbuf->buf + bytes_to_skip,delta * SECTOR - bytes_to_skip,44100,2,16,audio_chunk::channel_config_stereo);

		bytes_to_skip = 0;
		return io_result_success;
	}

	virtual bool can_seek() {return 1;}

	inline static bool g_is_our_content_type(const char*,const char*) {return false;}

	static GUID g_get_guid()
	{
		// {FF9A6E39-18AF-4e11-AAC3-0091B0581814}
		static const GUID guid = 
		{ 0xff9a6e39, 0x18af, 0x4e11, { 0xaa, 0xc3, 0x0, 0x91, 0xb0, 0x58, 0x18, 0x14 } };
		return guid;
	}

	static const char * g_get_name() {return "CD Audio decoder";}

	inline static t_io_result g_get_extended_data(const service_ptr_t<file> & p_reader,const playable_location & p_location,const GUID & p_guid,stream_writer * p_out,abort_callback & p_abort) {return io_result_error_data;}
};

class menu_item_cdda : public menu_item_legacy_main
{
	virtual GUID get_item_guid(unsigned n)
	{
		return standard_commands::guid_main_play_cd;
	}

	static bool build_tracklist(metadb_handle_list & out,int drive)
	{
		TRACK_CALL_TEXT("build_tracklist");
		akrip_reference akref;
		if (!akref.init())
		{
			console::error("Error initializing akrip library.");
			return false;
		}
		static_api_ptr_t<metadb> p_metadb;
		insync(g_access_sync);
	
		HCDROM handle = cdrom_open(drive);
		if (handle == 0)
		{
			//MessageBox(core_api::get_main_window(),"build_tracklist failed: cant open drive",0,0);
			return false;
		}

		TOC toc;
		memset(&toc,0,sizeof(toc));
		DWORD result = ReadTOC(handle,&toc,FALSE);
		DWORD id = genCDPlayerIniIndex(handle);
		cdrom_close(handle);
		if (result!=SS_COMP)
		{
			//MessageBox(core_api::get_main_window(),"build_tracklist failed: error getting TOC",0,0);
			return false;
		}
		
		UINT n;
		string_fixed_t<128> temp;
		int added = 0;
		for(n=toc.firstTrack;n<=toc.lastTrack;n++)
		{
			if (!(toc.tracks[n-toc.firstTrack].ADR&4))
			{
				added++;
				uPrintf(temp,"cdda://%08X,%02u",id,n);
				metadb_handle_ptr item;
				if (p_metadb->handle_create(item,make_playable_location(temp,0)))
					out.add_item(item);
			}
/*			else
			{
				TCHAR meh[256];
				wsprintf(meh,"build_tracklist: unknown track type - %u",toc.tracks[n-toc.firstTrack].ADR);
				MessageBox(core_api::get_main_window(),meh,0,0);
			}*/
		}
//		if (added==0)
//			MessageBox(core_api::get_main_window(),"build_tracklist failed: no audio tracks found",0,0);
		return added>0;
	}

	static BOOL CALLBACK PlayCdProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		TRACK_CALL_TEXT("PlayCdProc");
		static const TCHAR error_msg[] = TEXT("Error reading CD info.");

		switch(msg)
		{
		case WM_INITDIALOG:
			{
				TRACK_CALL_TEXT("WM_INITDIALOG");
				init_cdlist();
				if (g_cdlist.num == 0)
				{
					popup_message::g_show("No CD drives found. If you're using limited account, please install Nero BurnRights (freeware) to use CD playback functionality.","Information");
					EndDialog(wnd,0);
				}
				else
				{
					HWND list = GetDlgItem(wnd,IDC_DRIVE_LIST);
					unsigned n;
					for(n=0;n<g_cdlist.num;n++)
						SendMessage(list,LB_ADDSTRING,0,(int)(const TCHAR*)string_os_from_utf8(string_utf8_from_ansi(g_cdlist.cd[n].info.prodId)));
				}
				SendDlgItemMessage(wnd,IDC_LIMIT_SPEED,BM_SETCHECK,cfg_limit_speed,0);
			}
			return TRUE;
		case WM_COMMAND:
			switch(wp)
			{
			case IDC_LIMIT_SPEED:
				cfg_limit_speed = SendMessage((HWND)lp,BM_GETCHECK,0,0);
				return TRUE;
			case (LBN_DBLCLK<<16) | IDC_DRIVE_LIST:
			case IDOK:
				{
					int idx = SendDlgItemMessage(wnd,IDC_DRIVE_LIST,LB_GETCURSEL,0,0);
					if (idx>=0)
					{
						akrip_reference akref;
						if (akref.init())
						{
							metadb_handle_list out;

							if (build_tracklist(out,idx))
							{
								if (static_api_ptr_t<metadb_io>()->load_info_multi(out,metadb_io::load_info_default,wnd,true,0))
								{
									static_api_ptr_t<playlist_manager> api;
									if (api.is_valid())
									{
										unsigned idx = api->create_playlist("Audio CD",infinite,infinite);
										if (idx!=infinite)
										{
											api->set_active_playlist(idx);
											api->reset_playing_playlist();
										}
									}
									api->activeplaylist_undo_backup();
									api->activeplaylist_add_items(out,bit_array_true());
									static_api_ptr_t<play_control>()->play_start();
									EndDialog(wnd,0);
								}
							}
							else
								MessageBox(wnd,error_msg,0,0);
						}						
					}
				}
				return TRUE;
			case IDC_ADD:
				{
					int idx = SendDlgItemMessage(wnd,IDC_DRIVE_LIST,LB_GETCURSEL,0,0);
					if (idx>=0)
					{
						akrip_reference akref;
						if (akref.init())
						{
							metadb_handle_list out;

							if (build_tracklist(out,idx))
							{
								static_api_ptr_t<playlist_manager> api;
								if (api.is_valid())
								{
									api->activeplaylist_clear_selection();
									api->activeplaylist_undo_backup();
									api->activeplaylist_add_items(out,bit_array_true());
								}
							}
							else 
								MessageBox(wnd,error_msg,0,0);
						}
					}
				}
				return TRUE;
			case IDCANCEL:
				EndDialog(wnd,0);
				return TRUE;
			default:
				return FALSE;
			}
		case WM_CLOSE:
			EndDialog(wnd,0);
			return TRUE;
		default:
			return FALSE;
		}
	}
public:
	virtual bool get_display_data(unsigned n,string_base & out,unsigned & displayflags)
	{
		out = "Play Audio CD...";
		return true;
	}

	virtual unsigned get_num_items() {return 1;}
	virtual void get_item_name(unsigned n,string_base & out) {out = "Play audio CD...";}
	virtual void get_item_default_path(unsigned,string_base & out) {out = "Components";}

	virtual bool get_item_description(unsigned n,string_base & out)
	{
		out = "Displays Audio CD playback dialog.";
		return true;
	}

	virtual void perform_command(unsigned n)
	{
		TRACK_CALL_TEXT("components_menu_item_cdda::perform_command");
		DialogBox(core_api::get_my_instance(),MAKEINTRESOURCE(IDD_PLAY_CD),core_api::get_main_window(),PlayCdProc);
	}
};

class track_indexer_cdda : public track_indexer	//parses .cda files
{
public:
	t_io_result get_tracks(const char * p_path,const service_ptr_t<file> & p_reader,track_indexer_callback & p_callback)
	{
		TRACK_CALL_TEXT("track_indexer_cdda::get_tracks");
		service_ptr_t<file> r = p_reader;
		if (stricmp_utf8(string_extension_8(p_path),"cda")) return io_result_error_data;
		t_io_result status;
		if (r.is_empty())
		{
			status = filesystem::g_open_precache(r,p_path,p_callback);
			if (io_result_failed(status)) return status;
		}

		status = r->seek(0,p_callback);
		if (io_result_failed(status)) return status;
		char buffer[44];
		bool rv = false;
		status = r->read_object(buffer,44,p_callback);
		if (io_result_failed(status)) return status;
		if (!(*(DWORD*)(buffer+0) == 'FFIR' && *(DWORD*)(buffer+4) == 0x24 && *(DWORD*)(buffer+8)=='ADDC' && *(WORD*)(buffer+0x14)==1)) return io_result_error_data;

		{
			DWORD code = *(DWORD*)(buffer+0x18);
			DWORD track = *(WORD*)(buffer+0x16);
			if (!(code!=0 && track<100)) return io_result_error_data;
			char temp[128];
			sprintf(temp,"cdda://%08X,%02u",code,track);

			metadb_handle_ptr item;
			if (!p_callback.handle_create(item,make_playable_location(temp,0))) return io_result_error_generic;
			p_callback.on_entry(item,filestats_invalid);
		}

		return io_result_success;
	}
};


static input_factory_t<input_cdda> g_input_cdda_factory;
static menu_item_factory_t<menu_item_cdda> g_menu_item_cdda_factory;
static service_factory_single_t<track_indexer,track_indexer_cdda> foo3;

DECLARE_COMPONENT_VERSION("CD Audio decoder","1.0","Powered by akrip.dll sourcecode\nhttp://akrip.sourceforge.net/");
