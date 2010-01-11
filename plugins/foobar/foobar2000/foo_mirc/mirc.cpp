#include <windows.h>
#define __INET__
#ifdef __INET__
#include <wininet.h>
#endif

#include "../SDK/foobar2000.h"

// #define DBG(a) OutputDebugString(a)
#define DBG(a)

#ifdef __INET__
static char hexchar(unsigned char c)
{
	if (c < 10)
		return c + '0';
	else
		return c + 'A' - 10;
}

class url_encode_extra : public mem_block_t<char>
{
public:
	url_encode_extra(const char * src)
	{
		append("ttl=", 4);
		while (*src)
		{
			if (*src == ' ') append('+');
			else if ((*src < '0' && *src != '-' && *src != '.') ||
					 (*src < 'A' && *src > '9') ||
					 (*src > 'Z' && *src < 'a' && *src != '_') ||
					 (*src > 'z'))
			{
				append('%');
				append(hexchar((unsigned char) *src >> 4));
				append(hexchar((unsigned char) *src & 15));
			}
			else
			{
				append(*src);
			}
			src++;
		}
		append(0);
		append(0);
	}
};
#endif

BOOL CALLBACK mircSendAll(HWND hWnd, LPARAM lParam)
{
	char wndclass[256];
	int do_status = 0;

	if (!RealGetWindowClass(hWnd, (char *)&wndclass, sizeof(wndclass))) return TRUE;

	if (stricmp(wndclass, "mirc32") && stricmp(wndclass, "mirc")) return TRUE; // Change constant to "mirc32" for 5.9x, or "mirc16" ... doubt 16 bit version supports this method, though.

	{
		HWND mdiclient, active;

		DBG("found mirc window");

		mdiclient = FindWindowEx(hWnd, 0, "mdiclient", NULL);
		// channel = FindWindowEx(mdiclient, 0, "channel", NULL);
		active = GetWindow(mdiclient, GW_CHILD);
		uSendMessage(active, WM_USER + 200, 5, 0L);
	}
	return 0;
}

class mirc_callback : public play_callback
{
public:
	// Yay! I have to define the crap I'm not using!

	virtual void on_playback_starting() {}
	virtual void on_playback_stop(enum play_control::stop_reason reason) {}
	virtual void on_playback_seek(double time) {}
	virtual void on_playback_pause(int state) {}
	virtual void on_playback_edited(metadb_handle * track) {}
	virtual void on_playback_dynamic_info(const file_info *info, bool b_track_change) { }

	virtual unsigned get_callback_mask()
	{
		return MASK_on_playback_new_track;
	}

	virtual void on_playback_new_track(metadb_handle * track)
	{
		string8 cmd;
		track->handle_format_title(cmd, "'//'mp3title [%artist% - ]$if(%title%,['['%album%[ #[%disc%/]$num(%tracknumber%,2)]'] ']%title%,%_filename%) | /mp3tme %_length% | /mp3file %_path_raw% | /mp3rate %__samplerate% | /mp3bitrate $ifgreater(%__channels%,2,%__channels%0000,%__bitrate%) | /mp3channels %__channels% | /mp3codec [%__codec% ]| /mp3seconds %_length_seconds% | /mp3play", NULL);
		DBG(cmd);
//		metadb->format_title(track, cmd, "'//'set %mp3ad '\002[\002'$upper($ext(%_path%))'\002][\002'%_filename%'\002][\002'%_length%'\002][\002'$if($strcmp(%__extrainfo%,VBR),VBR,%__bitrate%kbps)'\002][\002 $+ $calc('%__samplerate%'/1000) $+ kHz\002][\002 $+ $bytes($file($mid('%_path%',8,512)).size,m) $+ MB\002][\002'%_foobar2000_version%'\002]'", NULL, NULL);
		HANDLE hFileMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4096, "mIRC");
		if (hFileMap)
		{
			char *mData;

			DBG("File Map created");

			mData = (char *)MapViewOfFile(hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
			if (mData)
			{
				DBG("sending command");
				strcpy(mData, cmd.get_ptr());
				EnumWindows(mircSendAll, 0);
				UnmapViewOfFile((LPCVOID)mData);
			}
			CloseHandle(hFileMap);
		}

#ifdef __INET__
		track->handle_format_title(cmd, "[%artist% - ]$if(%title%,['['%album%[ #[%disc%/]$num(%tracknumber%,2)]'] ']%title%,%_filename%)", NULL);
		track->handle_format_title(cmd, "http://locke/submitdb.php?title=%_ttl%&len=%_length_seconds%", url_encode_extra(cmd));

		//DBG("InternetOpen");
		HINTERNET hSession;
		hSession = InternetOpen(_TX("Foobar2000/0.6 beta"), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);

		if (hSession)
		{
			//DBG("InternetOpenUrl");
			//DBG(cmd.get_ptr());
			HINTERNET hService;
			hService = InternetOpenUrl(hSession, cmd.get_ptr(), 0, 0, INTERNET_FLAG_NO_CACHE_WRITE, 0);

			if (hService)
			{
				//DBG("InternetQueryDataAvailable");
				DWORD dwBytesAvailable;

				InternetQueryDataAvailable(hService, &dwBytesAvailable, 0, 0);
				if (dwBytesAvailable)
				{
					mem_block_t<char> buffer;
					DWORD dwBytesRead;
					char *lpBuffer = buffer.set_size(dwBytesAvailable);
					InternetReadFile(hService, lpBuffer, dwBytesAvailable, &dwBytesRead);
				}
				InternetCloseHandle(hService);
			}
			InternetCloseHandle(hSession);
		}
#endif
	}
};

class menu_date : public menu_item
{
	virtual type get_type()
	{
		return TYPE_MAIN;
	}

	virtual unsigned get_num_items()
	{
		return 1;
	}

	virtual void enum_item(unsigned n, string_base & out)
	{
		out = "Components/Copy date";
	}

	virtual enabled_state get_enabled_state(unsigned idx)
	{
		return DEFAULT_OFF;
	}

	virtual bool get_display_data(unsigned n, const ptr_list_base<metadb_handle> &data, string_base &out, unsigned & displayflags)
	{
		out = "yuo spoony bard";
		return true;
	}

	virtual void perform_command(unsigned n, const ptr_list_base<metadb_handle> &data)
	{
		string8 foo;
		SYSTEMTIME t;
		GetSystemTime(&t);
		foo.add_int(t.wYear);
		foo.add_char('-');
		if (t.wMonth < 10) foo.add_char('0');
		foo.add_int(t.wMonth);
		foo.add_char('-');
		if (t.wDay < 10) foo.add_char('0');
		foo.add_int(t.wDay);
		foo.add_char(' ');
		if (t.wHour < 10) foo.add_char('0');
		foo.add_int(t.wHour);
		foo.add_char(':');
		if (t.wMinute < 10) foo.add_char('0');
		foo.add_int(t.wMinute);
		foo.add_string(" UTC");
		uSetClipboardString(foo);
	}
};

static play_callback_factory<mirc_callback> foo;
static menu_item_factory<menu_date> foo2;
