#define MYVERSION "1.1"

/* libcurl feeds data through a callback, callback must always receive and never fail.
   Therefore, buffer will be this much larger than requested, and stream reception will
   be paused as buffer crosses this threshold. */
#define CURL_BUFFER_SAFETY 8192

#include "../SDK/foobar2000.h"

#include <ptypes.h>
#include <pasync.h>

USING_PTYPES

#include <curl/curl.h>

#include <commctrl.h>
#include "resource.h"

static const GUID guid_cfg_prebuffer_start = { 0xf5f1ffa1, 0xe300, 0x4a27, { 0xbe, 0x81, 0xed, 0xfe, 0x33, 0x65, 0x75, 0x87 } };
static const GUID guid_cfg_prebuffer_underrun = { 0x3cabfc1a, 0xfe65, 0x4ada, { 0xa7, 0x5a, 0x53, 0x79, 0x5e, 0x2a, 0x31, 0x22 } };
static const GUID guid_cfg_buffer_size = { 0x65c67e5b, 0xf1fc, 0x459b, { 0x8b, 0x6e, 0xeb, 0x40, 0x89, 0x3f, 0xe0, 0x4a } };
static const GUID guid_cfg_proxy_mode = { 0x5674d032, 0xced, 0x41cc, { 0x9a, 0x18, 0xf3, 0x26, 0xcc, 0x2, 0x70, 0xd2 } };
static const GUID guid_cfg_proxy_url = { 0x39b672b5, 0x5b5c, 0x46f7, { 0x9f, 0x66, 0x40, 0xde, 0x12, 0x95, 0x13, 0xb4 } };
static const GUID guid_cfg_icy_metadata = { 0xec2f3e1e, 0x4266, 0x400a, { 0x91, 0x5c, 0x8f, 0x3f, 0x77, 0xe5, 0xca, 0xf6 } };
static const GUID guid_cfg_curl_verbosity = { 0x44ad489d, 0xeec4, 0x489c, { 0xb3, 0x12, 0xfb, 0x94, 0xb6, 0xe0, 0x72, 0x3f } };

static cfg_int cfg_prebuffer_start(guid_cfg_prebuffer_start,25), cfg_prebuffer_underrun(guid_cfg_prebuffer_underrun,50), cfg_buffer_size(guid_cfg_buffer_size,200*1024);
static cfg_int cfg_proxy_mode(guid_cfg_proxy_mode,0);
static cfg_string cfg_proxy_url(guid_cfg_proxy_url,"");
static cfg_int cfg_icy_metadata(guid_cfg_icy_metadata,1);
static cfg_int cfg_curl_verbosity(guid_cfg_curl_verbosity,1);
static critical_section g_proxy_sync;

critical_section g_user_agent_sync;
static string8 g_user_agent;

const char * get_user_agent()
{
	insync( g_user_agent_sync );

	if ( ! g_user_agent.length() )
	{
		string8 temp = core_version_info::get_version_string();
		unsigned pos = temp.find_first(' ');
		unsigned pos2;

		g_user_agent.add_string(temp, pos);

		pos = temp.find_first('v', pos + 1) + 1;
		g_user_agent.add_byte('/');
		pos2 = temp.find_first(' ', pos + 1);

		if ( pos2 >= 0 )
		{
			g_user_agent.add_string(temp.get_ptr() + pos, pos2 - pos);
			const char * ptr = temp.get_ptr() + pos2;
			while ( * ptr && * ptr == ' ' ) ptr++;
			if ( * ptr )
			{
				if ( ! stricmp_utf8_partial( ptr, "alpha" ) ) g_user_agent.add_byte('a'), ptr += 5;
				else if ( ! stricmp_utf8_partial( ptr, "beta" ) ) g_user_agent.add_byte('b'), ptr += 4;
				while ( * ptr && * ptr == ' ' ) ptr++;
				while ( * ptr && * ptr != ' ' ) g_user_agent.add_byte( * ptr++ );
			}
		}
		else g_user_agent += temp.get_ptr() + pos;

		g_user_agent.add_byte(' ');

		g_user_agent += curl_version();
	}

	return g_user_agent;
}

class init_cleanup
{
	curl_slist * http200aliases;

public:
	init_cleanup() : http200aliases( NULL )
	{
		if ( curl_global_init( CURL_GLOBAL_ALL ) == CURLE_OK )
		{
			http200aliases = curl_slist_append( NULL, "ICY 200 OK" );
		}
	}

	~init_cleanup()
	{
		if ( http200aliases )
		{
			curl_slist_free_all( http200aliases );
			curl_global_cleanup();
		}
	}

	const curl_slist * get_aliases()
	{
		return http200aliases;
	}
};

static init_cleanup g_initclean;

struct conn_status
{
	bool closed;
	CURLcode code;

	conn_status() : closed(false), code(CURLE_OK) {}
};

class worker_thread : public thread
{
	trigger ready;
	critical_section sync;
	CURLM * multi_handle;

	int priority;

public:
	worker_thread() : thread( false ), multi_handle(0), ready(false, false)
	{
		priority = GetThreadPriority( GetCurrentThread() );
		start();
	}

	~worker_thread()
	{
		signal();
		waitfor();
	}

	void execute()
	{
		SetThreadPriority( GetCurrentThread(), priority );

		{
			insync( sync );
			multi_handle = curl_multi_init();
		}

		ready.post();

		if ( multi_handle )
		{
			CURLMcode status;
			int running, maxfd;
			CURLMsg * msg;
			fd_set readfds, writefds, excfds;
			timeval t = { 0, 100000 };
			while ( ! get_signaled() )
			{
				{
					insync( sync );
					status = curl_multi_perform( multi_handle, & running );
				}

				if ( status == CURLM_CALL_MULTI_PERFORM ) continue;
				else if ( status != CURLM_OK ) break;

				if ( running > 0 )
				{
					msg = 0;
					do
					{
						if ( msg )
						{
							if ( msg->msg == CURLMSG_DONE )
							{
								conn_status * status;
								curl_easy_getinfo( msg->easy_handle, CURLINFO_PRIVATE, (char **) & status );
								if ( status )
								{
									status->code = msg->data.result;
									status->closed = true;
								}
							}
						}
						msg = curl_multi_info_read( multi_handle, & running );
					}
					while ( msg );


					FD_ZERO( & readfds );
					FD_ZERO( & writefds );
					FD_ZERO( & excfds );

					{
						insync( sync );
						if ( curl_multi_fdset( multi_handle, & readfds, & writefds, & excfds, & maxfd ) != CURLM_OK ) break;
					}

					if ( maxfd > 0 )
						select( FD_SETSIZE, & readfds, & writefds, & excfds, & t );
					else
						if ( relax( 100 ) ) break;
				}
				else
					if ( relax( 100 ) ) break;
			}

			{
				insync( sync );
				curl_multi_cleanup( multi_handle );
				multi_handle = 0;
			}
		}
	}

	CURLMcode add_handle( CURL * easy_handle )
	{
		ready.wait();
		insync( sync );
		return curl_multi_add_handle( multi_handle, easy_handle );
	}

	CURLMcode remove_handle( CURL * easy_handle )
	{
		ready.wait();
		insync( sync );
		return curl_multi_remove_handle( multi_handle, easy_handle );
	}

	CURLMcode pause_handle( CURL * easy_handle, bool pause )
	{
		ready.wait();
		insync( sync );
		return curl_multi_pause_handle( multi_handle, easy_handle, pause );
	}

	void lock()
	{
		sync.enter();
	}

	void unlock()
	{
		sync.leave();
	}
};

#define STATUS_WINDOW_CRAP

#ifdef STATUS_WINDOW_CRAP
#include "../helpers/window_placement_helper.h"

// {9CD9C2B8-B59D-4692-A749-1CD56E8DC238}
static const GUID guid_cfg_status_window = 
{ 0x9cd9c2b8, 0xb59d, 0x4692, { 0xa7, 0x49, 0x1c, 0xd5, 0x6e, 0x8d, 0xc2, 0x38 } };
// {4BCD8345-1B59-4016-A711-FD53DFD412C6}
static const GUID guid_cfg_window_placement = 
{ 0x4bcd8345, 0x1b59, 0x4016, { 0xa7, 0x11, 0xfd, 0x53, 0xdf, 0xd4, 0x12, 0xc6 } };


static cfg_int cfg_status_window(guid_cfg_status_window, 0);
static cfg_window_placement cfg_placement(guid_cfg_window_placement);

static HWND g_bufwnd = NULL;
static bool g_bufwnd_showing = false;

static critical_section g_panel_sync;
static mem_block_list<HWND> g_panels;

#define IDC_PROGRESS1 1001

typedef struct
{
	WNDPROC oldproc;
} tclassdata;

static LRESULT CALLBACK TransCtlProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	HGLOBAL hglob = (HGLOBAL) GetPropA(wnd, "trans_dat");
	if (hglob == NULL) return DefWindowProc(wnd, msg, wp, lp);
	tclassdata * ocd = (tclassdata *) GlobalLock(hglob); tclassdata cd = *ocd;
	GlobalUnlock(hglob);
	switch (msg)
	{
	case WM_DESTROY:
		RemovePropA(wnd, "trans_dat"); GlobalFree(hglob);
		break;
	case WM_NCHITTEST:
		return HTTRANSPARENT;
	}
	return CallWindowProcA(cd.oldproc, wnd, msg, wp, lp);
}

/*
#define WS_EX_LAYERED           0x00080000
#define LWA_ALPHA               0x00000002

static void MakeTranslucent(HWND hWnd)
{
	HMODULE hUser = uGetModuleHandle("user32");
	if (hUser)
	{
		typedef BOOL (WINAPI * t_slwa)(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);

		t_slwa slwa = (t_slwa) GetProcAddress(hUser, "SetLayeredWindowAttributes");

		if (slwa)
		{
			SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
			slwa(hWnd, 0, 192, LWA_ALPHA);
		}

	}
}
*/

static LRESULT CALLBACK BufWndProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_CREATE:
		{
			cfg_placement.on_window_creation( wnd );

			RECT rc_client;
			GetClientRect(wnd, &rc_client);

			HWND w = uCreateWindowEx(0, "msctls_progress32", 0, PBS_SMOOTH | WS_CHILD | WS_VISIBLE, 0, 0, rc_client.right - rc_client.left, rc_client.bottom - rc_client.top, wnd, (HMENU)IDC_PROGRESS1, core_api::get_my_instance(), 0);
			SendMessageA(w, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
			SendMessageA(w, PBM_SETPOS, 0, 0);

			HGLOBAL hglob = GlobalAlloc(GMEM_MOVEABLE,sizeof(tclassdata));
			tclassdata * cd = (tclassdata*) GlobalLock(hglob);
			cd->oldproc = (WNDPROC) GetWindowLongA(w, GWL_WNDPROC);
			GlobalUnlock(hglob);
			SetPropA(w, "trans_dat", hglob);
			SetWindowLongA(w, GWL_WNDPROC, (long) TransCtlProc);

			//MakeTranslucent(wnd);
		}
		break;
	case WM_NCHITTEST:
		return HTCAPTION;
		break;
	case WM_USER + 666:
		SendDlgItemMessageA(wnd, IDC_PROGRESS1, PBM_SETPOS, wp, 0);
		{
			insync(g_panel_sync);
			for (int i = 0, j = g_panels.get_count(); i < j; i++)
			{
				PostMessageA(g_panels[i], PBM_SETPOS, wp, 0);
			}
		}

		if (cfg_status_window)
		{
			if (!g_bufwnd_showing && IsWindowVisible(core_api::get_main_window()))
			{
				ShowWindow(wnd, SW_SHOWNA);
			}
			SetTimer(wnd, 666, 2000, 0);
		}
		break;

	case WM_ACTIVATE:
		if (LOWORD(wp) != WA_INACTIVE)
			SetTimer(wnd, 666, 2000, 0);
		break;

	case WM_SETFOCUS:
		SetFocus(core_api::get_main_window());
		break;

	case WM_SHOWWINDOW:
		if (wp == TRUE)
		{
			g_bufwnd_showing = true;
			SetTimer(wnd, 666, 2000, 0);
		}
		else
		{
			g_bufwnd_showing = false;
			KillTimer(wnd, 666);
		}
		break;

	case WM_TIMER:
		if (wp == 666)
		{
			KillTimer(wnd, 666);
			ShowWindow(wnd, SW_HIDE);
		}
		break;

	case WM_DESTROY:
		KillTimer(wnd, 666);
		cfg_placement.on_window_destruction( wnd );
		DestroyWindow(GetDlgItem(wnd, IDC_PROGRESS1));
		break;
	}
	return uDefWindowProc(wnd, msg, wp, lp);
}

static const TCHAR class_name[] = _T("4EA3CD05-04B8-4dc4-83BF-C6517F270C76");

class http_initquit : public initquit
{
	ATOM class_atom;

	virtual void on_init()
	{
		WNDCLASS wc;
		memset(&wc, 0, sizeof(wc));
		wc.lpfnWndProc   = BufWndProc;
		wc.hInstance     = core_api::get_my_instance();
		wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
		wc.lpszClassName = class_name;
		class_atom = RegisterClass(&wc);

		if (class_atom)
		{
			g_bufwnd = uCreateWindowEx(WS_EX_TOOLWINDOW, (const char *)class_atom, "Buffer status", WS_POPUP | WS_DLGFRAME, 10, 10, 150, 20, core_api::get_main_window(), NULL, core_api::get_my_instance(), NULL);
		}
	}
	virtual void on_quit()
	{
		if (IsWindow(g_bufwnd))
		{
			DestroyWindow(g_bufwnd);
			g_bufwnd = NULL;
		}

		if (class_atom)
		{
			UnregisterClass((const TCHAR *)class_atom, core_api::get_my_instance());
			class_atom = NULL;
		}
	}
};

static initquit_factory<http_initquit> feh;
#endif

static void convert_url_chars(string8 & out,const char * src)
{
	out.reset();
	while(*src)
	{
		unsigned val = *(BYTE*)src;
		src++;
		if (val<0x80 && val!=' ')
			out.add_byte((char)val);
		else
		{
			char temp[16];
			sprintf(temp,"%%%02X",val);
			out.add_string(temp);
		}
	}
}

static const char * do_proxy(const char * url)
{
	if (strlen(cfg_proxy_url)==0) return 0;
    switch(cfg_proxy_mode)
    {
    default:
        return 0;
    case 1:
        {
            const char * p=strstr(url,"://");
            if (!p) p=url;
			else p+=3;
            while(*p && *p!=':' && *p!='/') p++;
            if (*p==':')
            {
                if (atoi(p+1)!=80) return 0;
            }
        }
    case 2:
        return cfg_proxy_url;
    }
}

class file_http : public reader_seekback_t<file_dynamicinfo>
{
private:
    string8 url,mimetype,proxy;
	__int64 length,position,seekto;
    UINT bsize;
	bool b_starting;

	critical_section bsync;
	bool paused;
	conn_status status;
    CURL * get;
	mem_block_t<unsigned char> buffer;
	UINT bptr, bfill;

	worker_thread * thread;

	critical_section hsync;
	curl_slist * outheaders;
	curl_slist * headers;

#ifdef STATUS_WINDOW_CRAP
	bool b_showing;
#endif

#ifdef STATUS_WINDOW_CRAP
	void close_dlg()
	{
		PostMessageA(g_bufwnd, WM_USER + 666, 0, 0);
	}
#endif

	virtual void on_idle(abort_callback & p_abort)
	{
		insync( bsync );

		if ( thread && !status.closed && paused && ( ( bsize - bfill ) >= CURL_BUFFER_SAFETY ) )
		{
			paused = false;
			thread->pause_handle( get, false );
		}
	}

	static size_t g_write_callback(void * ptr, size_t size, size_t nmemb, void * stream)
	{
		file_http * pthis = (file_http *) stream;
		if ( pthis ) return pthis->write_callback(ptr, size * nmemb) / size;
		else return 0;
	}

	static size_t g_header_callback(void * ptr, size_t size, size_t nmemb, void * stream)
	{
		file_http * pthis = (file_http *) stream;
		if ( pthis ) return pthis->header_callback( ptr, size * nmemb ) / size;
		else return 0;
	}

#if 1
	static int g_debug_callback( CURL * easy_handle, curl_infotype info, char * text, size_t size, void * data )
	{
		switch ( info )
		{
		case CURLINFO_TEXT:
			if (cfg_curl_verbosity > 0) console::info( uStringPrintf( "[%u] %s", (int)data, string8(text, size).get_ptr() ) );
			break;
			
		case CURLINFO_HEADER_IN:
			if (cfg_curl_verbosity > 1) console::info( uStringPrintf( "[%u] header in: %s", (int)data, string8(text, size).get_ptr() ) );
			break;

		case CURLINFO_HEADER_OUT:
			if (cfg_curl_verbosity > 1) console::info( uStringPrintf( "[%u] header out: %s", (int)data, string8(text, size).get_ptr() ) );
			break;

		case CURLINFO_DATA_IN:
			if (cfg_curl_verbosity > 2) console::info( uStringPrintf( "[%u] data in (%u bytes)", (int)data, size ) );
			break;

		case CURLINFO_DATA_OUT:
			if (cfg_curl_verbosity > 2) console::info( uStringPrintf( "[%u] data out (%u bytes)", (int)data, size ) );
			break;
		}

		return 0;
	}
#endif

public:
	size_t write_callback(void * ptr, size_t size)
	{
		if ( status.closed ) return 0;

		unsigned char * p = (unsigned char *) ptr;

		while ( size && !status.closed )
		{
			bsync.enter();
			size_t remain = bsize - bfill;
			size_t todo = (remain > size) ? size : remain;
			bptr = buffer.write_circular( bptr, p, todo );
			p += todo;
			remain -= todo;
			size -= todo;
			bfill += todo;
			if ( g_bufwnd ) PostMessage( g_bufwnd, WM_USER + 666, bfill * 100 / bsize, 0 );
			bsync.leave();
			if ( remain < CURL_BUFFER_SAFETY )
			{
				if ( ! paused )
				{
					paused = true;
					thread->pause_handle( get, true );
				}
			}
			if ( size )
			{
				Sleep( 10 );
			}
		}

		return status.closed ? 0 : ( p - ((unsigned char *) ptr) );
	}

	size_t header_callback( void * ptr, size_t size )
	{
		insync( hsync );
		string8_fastalloc temp;
		temp.set_string( (const char *) ptr, size );
		int p1 = temp.find_first('\r');
		int p2 = temp.find_first('\n');
		if ( p1 < 0 || ( p2 >= 0 && p2 < p1 ) ) p1 = p2;
		if ( p1 >= 0 ) temp.truncate( p1 );
		headers = curl_slist_append( headers, temp );
		return size;
	}

private:
	virtual t_io_result seekback_read(void * p_buffer,unsigned p_bytes,unsigned & p_bytes_read,abort_callback & p_abort);
	virtual t_io_result get_size(t_filesize & p_out,abort_callback & p_abort) 
	{
		if (p_abort.is_aborting()) return io_result_aborted;
		p_out = length>0 ? length : -1;
		return io_result_success;
	}
	
	virtual bool get_content_type(string_base & out)
	{
		if (mimetype.is_empty()) return false;
		out.set_string(mimetype);
		return true;
	}

	bool icy_enabled;
	unsigned icy_interval,icy_pointer;
	mem_block_t<char> icy_metablock;
	bool icy_metablock_updated;

    void fill_buffer(unsigned max,abort_callback & p_abort);
    unsigned http_read(void* ptr,size_t total,abort_callback & p_abort);
    int reconnect(__int64 ofs);
	

public:
    file_http(const char * _proxy) : get(0), paused(false), headers(0), outheaders(0), thread(0),
		reader_seekback_t<file_dynamicinfo>(0x10000)
    {
        length = position = seekto = 0;
        bsize=cfg_buffer_size + CURL_BUFFER_SAFETY;
		bptr=bfill=0;
		buffer.set_size(bsize);
		icy_enabled = false;
		icy_interval = 0;
		icy_pointer = 0;
		icy_metablock_updated = 0;

#ifdef STATUS_WINDOW_CRAP
		b_showing = false;
#endif

		get = curl_easy_init();
		if ( get )
		{
			curl_easy_setopt( get, CURLOPT_NOSIGNAL, 1 );
			curl_easy_setopt( get, CURLOPT_WRITEFUNCTION, & file_http::g_write_callback );
			curl_easy_setopt( get, CURLOPT_WRITEDATA, this );
			curl_easy_setopt( get, CURLOPT_HEADERFUNCTION, & file_http::g_header_callback );
			curl_easy_setopt( get, CURLOPT_WRITEHEADER, this );
			curl_easy_setopt( get, CURLOPT_BUFFERSIZE, 4096 );
			curl_easy_setopt( get, CURLOPT_HTTPAUTH, CURLAUTH_ANY );
			curl_easy_setopt( get, CURLOPT_ENCODING, "" );
			curl_easy_setopt( get, CURLOPT_FOLLOWLOCATION, 1 );
			curl_easy_setopt( get, CURLOPT_MAXREDIRS, 15 );
			curl_easy_setopt( get, CURLOPT_USERAGENT, get_user_agent() );
			curl_easy_setopt( get, CURLOPT_HTTP200ALIASES, g_initclean.get_aliases() );
			curl_easy_setopt( get, CURLOPT_FILETIME, 1 );
			curl_easy_setopt( get, CURLOPT_PRIVATE, (char*) & status );

#if 1
			if (cfg_curl_verbosity > 0)
			{
				static unsigned count = 0;
				curl_easy_setopt( get, CURLOPT_VERBOSE, 1 );
				curl_easy_setopt( get, CURLOPT_DEBUGFUNCTION, & file_http::g_debug_callback );
				curl_easy_setopt( get, CURLOPT_DEBUGDATA, count++ );
			}
#endif

			if ( _proxy )
			{
				proxy = _proxy;

				curl_easy_setopt( get, CURLOPT_PROXY, proxy.get_ptr() );
				curl_easy_setopt( get, CURLOPT_PROXYAUTH, CURLAUTH_ANY );
			}
		}
    }

	bool open(const char *path,filesystem::t_open_mode mode,abort_callback & p_abort);
    ~file_http()
    {
		if ( get && thread ) thread->remove_handle( get );
		if ( headers ) curl_slist_free_all( headers );
		if ( outheaders ) curl_slist_free_all( outheaders );
		if ( get ) curl_easy_cleanup( get );
		if ( thread ) delete thread;
#ifdef STATUS_WINDOW_CRAP
		close_dlg();
#endif
    }

	virtual bool is_dynamic_info_enabled()
	{
		return icy_enabled;
	}

	bool get_dynamic_info(file_info & info, bool * b_track_change)
	{
		if (icy_enabled && icy_metablock_updated)
		{
			icy_metablock_updated = false;
			assert(icy_metablock.get_size()>0);
			string8 meta(icy_metablock);

			string_simple old_artist, old_title;
			{
				const char * p_old = info.meta_get("ARTIST",0);
				if (p_old) old_artist = p_old;
				p_old = info.meta_get("TITLE",0);
				if (p_old) old_title = p_old;
			}

			unsigned name_pos, name_len;
			unsigned val_pos, val_end;
			unsigned s = 0, p, e = meta.length();
			bool found = false;
			while (s < e)
			{
				p = meta.find_first('=', s);
				if (p < 0) break;
				name_pos = s;
				name_len = p - s;
				p = meta.find_first('\'', p);
				if (p < 0) break;
				s = val_pos = p + 1;
				while ((p = meta.find_first('\'', s)) >= 0)
				{
					s = p + 1;
					char meh = *(meta.get_ptr() + s);
					if (!meh || meh == ';') break;
				}
				if (p < 0) break;
				val_end = p;
				s++;
				if (!strnicmp(meta.get_ptr() + name_pos, "streamtitle", name_len))
				{
					string8 artist(string_utf8_from_ansi(meta.get_ptr() + val_pos, val_end - val_pos)), title;
					if (!found)
					{
						info.meta_remove_all();
						found = true;
					}
					int pos = artist.find_first(" - ");
					if (pos < 0)
					{
						title.set_string(artist);
						artist.reset();
					}
					else
					{
						title.set_string(artist.get_ptr() + pos + 3);
						artist.truncate(pos);
					}
					if (stricmp_utf8(old_artist, artist) || stricmp_utf8(old_title, title))
					{
						*b_track_change = true;
					}
					if (artist.length()) info.meta_set("ARTIST", artist);
					if (title.length()) info.meta_set("TITLE", title);
				}
				else if (!strnicmp(meta.get_ptr() + name_pos, "streamurl", name_len))
				{
					string_utf8_from_ansi url(meta.get_ptr() + val_pos, val_end - val_pos);
					if (!found)
					{
						info.meta_remove_all();
						found = true;
					}
					if (url.length()) info.meta_set("URL", url);
				}
			}
			return found;
		}
		return false;
	}

	virtual t_io_result get_timestamp(t_filetimestamp & p_out,abort_callback & p_abort)
	{
		if (p_abort.is_aborting() || !get) return io_result_aborted;
		long filetime;
		if ( curl_easy_getinfo( get, CURLINFO_FILETIME, & filetime ) == CURLE_OK && filetime >= 0 )
		{
			p_out = t_filetimestamp(filetime) * 10000000i64 + 116444736000000000i64;
		}
		else
		{
			p_out = filetimestamp_invalid;
		}
		return io_result_success;
	}

	virtual bool can_seek2()
	{
		return length > 0;
	}

	t_io_result do_seek( t_filesize position, abort_callback & p_abort )
	{
		seekto = position;
		return io_result_success;
	}
};

int file_http::reconnect(__int64 ofs)
{
	if ( ! get || ! thread ) return 1;

	thread->remove_handle( get );

	curl_easy_setopt( get, CURLOPT_URL, url.get_ptr() );

    if ( outheaders ) curl_slist_free_all( outheaders );

	outheaders = curl_slist_append( NULL, "Accept:*/*");
	if (cfg_icy_metadata) outheaders = curl_slist_append( outheaders, "Icy-MetaData:1" );

	curl_easy_setopt( get, CURLOPT_RESUME_FROM_LARGE, ofs );

	curl_easy_setopt( get, CURLOPT_HTTPHEADER, outheaders );

	paused = false;

	status.closed = false;
	status.code = CURLE_OK;

	bptr = 0;
	bfill = 0;

	position = ofs;

	thread->add_handle( get );

    return 0;
}

void file_http::fill_buffer(unsigned max,abort_callback & p_abort)
{
	on_idle( p_abort );
	if (max==0) max=1;
    if (length>0 && position+max>length) max=(int)(length-position);
    while(!p_abort.is_aborting())    //stop prebuffering if we want to seek
    {
		{
			insync( bsync );
			if ( bfill >= max ) break;
			if ( status.closed ) break;
		}
		SleepEx(1,TRUE);
    }
}

unsigned file_http::http_read(void* vptr,size_t total,abort_callback & p_abort)
{
	char * ptr = (char*)vptr;

	if ( seekto >= 0 )
	{
		if ( seekto > position )
		{
			t_filesize skip = seekto - position;
			insync( bsync );
			if ( t_filesize( bfill ) >= skip )
			{
				bfill -= UINT( skip );
				seekto = -1;
			}
		}

		if ( seekto >= 0 )
		{
			if ( seekto != position )
				if ( reconnect( seekto ) ) return 0;

			seekto = -1;

			long status = -1;

			while(!p_abort.is_aborting())
			{
				bsync.enter();
				status = ( bfill > 0 || this->status.closed ) ? -1 : 0;
				bsync.leave();

				if ( status < 0 )
				{
					thread->lock();
					curl_easy_getinfo( get, CURLINFO_RESPONSE_CODE, & status );
					thread->unlock();
				}

				if (status<0 || ( status>1 && ( status < 300 || status >= 400 ) ) ) break;

	            if (this->status.closed && this->status.code != CURLE_OK)
		        {
			        return 0;
				}
				SleepEx(1,TRUE);
			}

			if ( status < 0 || status >= 300 )
			{
				if ( status == 416 )
				{
					return 0;
				}

				if ( status < 0 ) console::info( "connection error" );
				else if ( status >= 500 ) console::info( "server error" );
				else if ( status >= 400 ) console::info( "file not found" );
				else if ( status >= 300 ) console::info( "redirection limit exceeded" );
				return 0;
			}

			b_starting = true;
		}
	}

	if (b_starting)
	{
		b_starting = false;
		fill_buffer(bsize * cfg_prebuffer_start / 100,p_abort);
	}
    int d;
	unsigned wr=0;
    while(!p_abort.is_aborting() && wr<total)
    {
		{
			insync( bsync );
			d = total - wr;
			if ( d > bfill ) d = bfill;
			buffer.read_circular( bptr - bfill, (unsigned char *) ptr, d );
			bfill -= d;
			if ( g_bufwnd ) SendMessage( g_bufwnd, WM_USER + 666, bfill * 100 / bsize, 0 );
		}

		on_idle( p_abort );

		if (status.closed && !d) break;
        wr+=d;
        ptr+=d;
        position+=d;
        if ((length>0 && position>=length) || wr>=total || p_abort.is_aborting()) break;
		SleepEx(1,TRUE);
        fill_buffer(bsize * cfg_prebuffer_underrun / 100,p_abort);
    }


    return wr;
}


t_io_result file_http::seekback_read(void * p_buffer,unsigned p_bytes,unsigned & p_bytes_read,abort_callback & p_abort)
{
	if (p_abort.is_aborting()) return io_result_aborted;
	if (p_bytes<=0) return io_result_success;

	char * out = (char*)p_buffer;
	unsigned written = 0;
	unsigned size = p_bytes;

	if (size>0)
	{
		if (icy_enabled)
		{
			assert(icy_interval>0);
			while(size>0)
			{
				unsigned delta = icy_interval - icy_pointer;
				if (delta>size) delta=size;
				if (delta>0)
				{
					unsigned done = http_read(out,delta,p_abort);
					written += done;
					out += done;
					size -= done;
					if (done!=delta) break;
					icy_pointer += delta;
				}
				if (icy_pointer==icy_interval)
				{
					icy_pointer = 0;
					unsigned char metablock_size_byte=0;
					if (http_read(&metablock_size_byte,1,p_abort)!=1) break;
					if (!metablock_size_byte) continue;
					unsigned metablock_size = (unsigned)metablock_size_byte * 16;
					if (!icy_metablock.check_size(metablock_size+1)) break;
					if (http_read(icy_metablock.get_ptr(),metablock_size,p_abort)!=metablock_size) break;
					icy_metablock[metablock_size]=0;
					icy_metablock_updated = true;
				}
			}
		}
		else
		{
			unsigned delta = http_read(out,size,p_abort);
			written += delta;
			out += delta;
			size -= delta;
		}
	}

	p_bytes_read = written;
	return p_abort.is_aborting() ? io_result_aborted : io_result_success;
}

bool file_http::open(const char *path,filesystem::t_open_mode mode,abort_callback & p_abort)
{		
	if (mode!=filesystem::open_mode_read) return false;
	convert_url_chars(url,path);

	thread = new worker_thread;

    if (reconnect(0))
    {
		console::info("reconnect failed");
        return false;
    }


    {
        //need to get http headers first
		long status = -1;

        while(!p_abort.is_aborting())
        {
			bsync.enter();
			status = ( bfill > 0 || this->status.closed ) ? -1 : 0;
			bsync.leave();

			if ( status < 0 )
			{
				thread->lock();
				curl_easy_getinfo( get, CURLINFO_RESPONSE_CODE, & status );
				thread->unlock();
			}

			if (status<0 || ( status>1 && ( status < 300 || status >= 400 ) ) ) break;

            if (this->status.closed && this->status.code != CURLE_OK)
            {
                return false;
            }
            SleepEx(1,TRUE);
        }

		if ( status < 0 || status >= 300 )
		{
			if ( status < 0 ) console::info( "connection error" );
			else if ( status >= 500 ) console::info( "server error" );
			else if ( status >= 400 ) console::info( "file not found" );
			else if ( status >= 300 ) console::info( "redirection limit exceeded" );
			return false;
		}

		{
			double dlength;
			curl_easy_getinfo( get, CURLINFO_CONTENT_LENGTH_DOWNLOAD, & dlength );
			length = dlength ? __int64(dlength) : -1;
		}
/*
		{
			const char * headers = get.getallheaders();
			do
			{
				console::info(headers);
				headers += strlen(headers) + 1;
			}
			while(*headers);
		}
*/
		{
			const char * bleh;
			
			curl_easy_getinfo( get, CURLINFO_CONTENT_TYPE, &bleh );
			mimetype = bleh ? bleh : "";

			curl_slist * next = headers;

			icy_enabled = false;

			while ( next )
			{
				if ( ! stricmp_utf8_partial( next->data, "icy-metaint:" ) )
				{
					bleh = next->data + 12;
					while ( * bleh && ! isdigit( * bleh ) ) bleh++;
					if ( * bleh )
					{
						icy_interval = atoi( bleh );
						icy_enabled = icy_interval > 0;
					}
					break;
				}
				next = next->next;
			}
		}
    }

	icy_pointer = 0;
	icy_metablock_updated = false;


    seekto=-1;
	b_starting = true;

    return true;
}


class filesystem_http : public filesystem
{
public:
	virtual bool get_display_path(const char * path,string_base & out)
	{
		out = path;
		return true;
	}

	virtual bool get_canonical_path(const char * path,string_base & out)
	{
		if (!is_our_path(path)) return 0;
		DWORD len=0;
		string8 temp;

		if (!strnicmp(path,"http://",7))
		{
			if (!strchr(path+7,'/'))
			{
				temp = path;
				temp.add_char('/');
				path = temp;
			}
		}

		out.set_string(path);
		
		return 1;
	}

	virtual bool is_our_path(const char * path)
	{
		return !strcmp_partial(path,"http://");
	}

	virtual t_io_result open(service_ptr_t<file> & p_out,const char * path, t_open_mode mode,abort_callback & p_abort)
	{
		insync(g_proxy_sync);

		string8 temp;
		convert_url_chars(temp,path);

		service_ptr_t<file_http> ptr = new service_impl_p1_t<file_http,const char*>(do_proxy(temp));
		if (ptr.is_empty()) return io_result_error_generic;
		if (!ptr->open(path,mode,p_abort)) return io_result_error_generic;

		p_out = ptr.get_ptr();
		return io_result_success;
	}

	virtual int exists(const char * path,abort_callback & p_abort)
	{
		return 1;//rather pointless to run checks
	}
	virtual t_io_result remove(const char * path,abort_callback & p_abort)
	{
		return io_result_error_generic;
	}
	virtual t_io_result move(const char * src,const char * dst,abort_callback & p_abort)
	{
		return io_result_error_generic;
	}
	virtual bool dont_read_infos(const char *) {return false;}
	virtual t_io_result create_directory(const char * path,abort_callback &) {return io_result_error_generic;}

	virtual t_io_result list_directory(const char * p_path,directory_callback & p_out,abort_callback & p_abort)
	{
		return io_result_error_not_found;
	}

	virtual t_io_result get_stats(const char * p_path,t_filestats & p_stats,bool & p_is_writeable,abort_callback & p_abort)
	{
		return io_result_error_generic;
	}

};

static service_factory_single_t<filesystem,filesystem_http> g_filesystem_http_factory;

class preferences_page_http : public preferences_page
{
	static BOOL CALLBACK DialogProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		switch(msg)
		{
		case WM_INITDIALOG:
			uSetDlgItemInt(wnd,IDC_BUFSIZE,cfg_buffer_size/1024,0);
			uSendDlgItemMessage(wnd,IDC_PREBUF_START,TBM_SETRANGE,0,MAKELONG(1,100));
			uSendDlgItemMessage(wnd,IDC_PREBUF_UNDERRUN,TBM_SETRANGE,0,MAKELONG(1,100));
			uSendDlgItemMessage(wnd,IDC_PREBUF_START,TBM_SETPOS,1,cfg_prebuffer_start);
			uSendDlgItemMessage(wnd,IDC_PREBUF_UNDERRUN,TBM_SETPOS,1,cfg_prebuffer_underrun);
			uSetDlgItemText(wnd,IDC_STATUS_PREBUF1,uStringPrintf("%u%%",(int)cfg_prebuffer_start));
			uSetDlgItemText(wnd,IDC_STATUS_PREBUF2,uStringPrintf("%u%%",(int)cfg_prebuffer_underrun));
			uSendDlgItemMessageText(wnd,IDC_PROXY_MODE,CB_ADDSTRING,0,"Don't use proxy");
			uSendDlgItemMessageText(wnd,IDC_PROXY_MODE,CB_ADDSTRING,0,"Use proxy only for port 80 connections");
			uSendDlgItemMessageText(wnd,IDC_PROXY_MODE,CB_ADDSTRING,0,"Use proxy for all connections");
			uSendDlgItemMessage(wnd,IDC_PROXY_MODE,CB_SETCURSEL,cfg_proxy_mode,0);
			g_proxy_sync.enter();
			uSetDlgItemText(wnd,IDC_PROXY_URL,cfg_proxy_url);
			g_proxy_sync.leave();
			uSendDlgItemMessage(wnd,IDC_ICY_METADATA,BM_SETCHECK,cfg_icy_metadata,0);
			return TRUE;
		case WM_COMMAND:
			switch(wp)
			{
			case (CBN_SELCHANGE<<16) | IDC_PROXY_MODE:
				cfg_proxy_mode = uSendMessage((HWND)lp,CB_GETCURSEL,0,0);
				return TRUE;
			case (EN_CHANGE<<16)|IDC_BUFSIZE:
				cfg_buffer_size = uGetDlgItemInt(wnd,IDC_BUFSIZE,0,0) * 1024;
				return TRUE;
			case (EN_CHANGE<<16)|IDC_PROXY_URL:
				g_proxy_sync.enter();
				cfg_proxy_url = string_utf8_from_window((HWND)lp);
				g_proxy_sync.leave();
				return TRUE;
			case IDC_ICY_METADATA:
				cfg_icy_metadata = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
				return TRUE;
			default:
				return FALSE;
			}
		case WM_HSCROLL:
			switch(uGetWindowLong((HWND)lp,GWL_ID))
			{
			case IDC_PREBUF_START:
				cfg_prebuffer_start = uSendMessage((HWND)lp,TBM_GETPOS,0,0);
				uSetDlgItemText(wnd,IDC_STATUS_PREBUF1,string_printf("%u%%",(int)cfg_prebuffer_start));
				return TRUE;
			case IDC_PREBUF_UNDERRUN:
				cfg_prebuffer_underrun = uSendMessage((HWND)lp,TBM_GETPOS,0,0);
				uSetDlgItemText(wnd,IDC_STATUS_PREBUF2,string_printf("%u%%",(int)cfg_prebuffer_underrun));
				return TRUE;
			default:
				return FALSE;
			}
		default:
			return FALSE;
		}
	}
	virtual HWND create(HWND parent)
	{
		return uCreateDialog(IDD_CONFIG,parent,DialogProc);
	}
	GUID get_guid()
	{
		// {8FE09CEB-8EF3-4e30-A5D8-12EC55B37A8D}
		static const GUID guid = 
		{ 0x8fe09ceb, 0x8ef3, 0x4e30, { 0xa5, 0xd8, 0x12, 0xec, 0x55, 0xb3, 0x7a, 0x8d } };
		return guid;
	}
	const char * get_name() {return "HTTP reader";}
	GUID get_parent_guid() {return guid_components;}

	bool reset_query() {return false;}
	void reset() {}
};

static preferences_page_factory_t<preferences_page_http> g_preferences_page_factory;

class version_http : public componentversion
{
public:
	virtual void get_file_name(string_base & out) { out.set_string(core_api::get_my_file_name()); }
	virtual void get_component_name(string_base & out) { out.set_string("HTTP Reader"); }
	virtual void get_component_version(string_base & out) { out.set_string(MYVERSION); }
	virtual void get_about_message(string_base & out)
	{
		out = "PTypes v2.0.2\nCopyright (C) 2001-2004 Hovik Melikyan\n\n";
		out += curl_version();
	}
};

static service_factory_single_t<componentversion,version_http> g_componentversion_factory;