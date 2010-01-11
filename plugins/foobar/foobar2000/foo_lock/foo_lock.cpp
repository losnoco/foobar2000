/*
     � 2004 Kreisquadratur
     icq#56165405

	2004-02-09 15:59 UTC - kode54
	- Added new WTSAPI32-based window class for session monitoring in Windows XP and 2003 Server
	- Consolidated lock/unlock code into static functions
	- Safeguarded enable/disable functions with win_ver, set by initial call to GetVersion()
	- Changed cfg_waslocked/cfg_resume to static BOOL ... not sure these should persist across sessions
	- Version is now 0.2

	2004-02-09 18:15 UTC - kode54
	- Fixed multi-user session monitoring ... see large comment block below :)
	- Version is now 0.3

	2004-02-24 21:44 UTC - kode54
	- Disable function is now a little saner
	- Version is now 0.4
*/

#define _WIN32_WINNT 0x501

#include <foobar2000.h>

#include <wtsapi32.h>

#define IsWinVer2000Plus() (LOBYTE(LOWORD(GetVersion()))>=5)





DECLARE_COMPONENT_VERSION(
	"Pause on Lock",
	"0.4",
	"Pause/Resume when Workstation is Locked/Unlocked\n\n"
	"for foobar2000 v0.8\n\n"
	"(c) Kreisquadratur 2004");

// {79FC6DE5-0291-47ab-A806-B4764B1C2279}
static const GUID guid_cfg_enabled = 
{ 0x79fc6de5, 0x291, 0x47ab, { 0xa8, 0x6, 0xb4, 0x76, 0x4b, 0x1c, 0x22, 0x79 } };

cfg_int cfg_enabled(guid_cfg_enabled, 0);
// eh?
//cfg_int cfg_waslocked("waslocked", 0);
//cfg_int cfg_resume("resume", 0);
static BOOL cfg_waslocked = FALSE;
static BOOL cfg_resume = FALSE;
static UINT hTimer = 0;

static BOOL win_ver = FALSE;

class CSessionWnd;
typedef BOOL (WINAPI * pWTSRegisterSessionNotification)(HWND hWnd, DWORD dwFlags);
typedef BOOL (WINAPI * pWTSUnRegisterSessionNotification)(HWND hWnd);

CSessionWnd * g_sessionwnd = NULL;

static void lock()
{
	string8 info("Workstation locked");
	service_ptr_t<play_control> pc;
	if (!play_control::g_get(pc)) return;
	cfg_waslocked = TRUE;
	if (pc->is_playing() && !pc->is_paused())
	{
		standard_commands::main_pause();
		info += " - paused";
		cfg_resume = TRUE;
	}
	info.add_byte('.');
	console::info(info);
}

static void unlock()
{
	string8 info("Workstation unlocked");
	service_ptr_t<play_control> pc;
	if (!play_control::g_get(pc)) return;
	cfg_waslocked = FALSE;
	if (cfg_resume)
	{
		cfg_resume = FALSE;
		if (pc->is_paused())
		{
			standard_commands::main_play();
			info += " - resumed";
		}
	}
	info.add_byte('.');
	console::info(info);
}

static const TCHAR class_name[] = _T( "870AC6B2-D141-46f4-A196-ADCB72B8AE4E" );

class CSessionWnd
{
public:
	CSessionWnd()
	{
		m_bRegistered = FALSE;
		m_hWnd = NULL;
		hWTSAPI = NULL;
		m_bDisconnected = FALSE;
	}

	bool Initialize(HINSTANCE hInstance)
	{
		m_hInstance = hInstance;

		hWTSAPI = LoadLibrary(_T("wtsapi32.dll"));
		if (!hWTSAPI) return false;

		register_session = (pWTSRegisterSessionNotification) GetProcAddress(hWTSAPI, "WTSRegisterSessionNotification");
		if (!register_session) return false;
		unregister_session = (pWTSUnRegisterSessionNotification) GetProcAddress(hWTSAPI, "WTSUnRegisterSessionNotification");
		if (!unregister_session) return false;

		WNDCLASS wcl;
		memset(&wcl, 0, sizeof(wcl));
		wcl.hInstance = hInstance;
		wcl.lpfnWndProc = (WNDPROC)WndProc;
		wcl.lpszClassName = class_name;
		m_lpszClassName = ( const TCHAR * ) RegisterClass( & wcl );
		if ( ! m_lpszClassName ) return false;

		m_bRegistered = TRUE;

		m_hWnd = CreateWindowEx(0, m_lpszClassName, _T( "uninteresting" ), 0, 0, 0, 0, 0, 0, 0, hInstance, this);

		if (m_hWnd)
		{
			//modeless_dialog_manager::add(m_hWnd);
			return !!register_session(m_hWnd, NOTIFY_FOR_THIS_SESSION);
		}
		else
		{
			return false;
		}
	}

	~CSessionWnd()
	{
		if (IsWindow(m_hWnd)) DestroyWindow(m_hWnd);
		if (m_bRegistered)
			UnregisterClass( m_lpszClassName, m_hInstance );
		if (hWTSAPI) FreeLibrary(hWTSAPI);
	}

private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
	{
		static CSessionWnd *pThis = NULL;
		if(uMessage == WM_CREATE)
		{
			pThis = (CSessionWnd *)((CREATESTRUCT *)(lParam))->lpCreateParams;
		}

		return pThis->WindowProc(hWnd, uMessage, wParam, lParam);
	}

	LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
	{
		switch (uMessage)
		{
		case WM_DESTROY:
			unregister_session(hWnd);
			//modeless_dialog_manager::remove(hWnd);
			break;

			/* the story behind the madness....
				it seems that Microsoft handles things like this:

			For a normal workstation lock,
				WTS_SESSION_LOCK on lock,
				WTS_SESSION_UNLOCK on unlock.

			BUT, for multiple users at the same terminal...
				WTS_SESSION_LOCK on fast user switch,
			AND just as a new user logs in,
				WTS_SESSION_UNLOCK immediately before this account gets
				WTS_CONSOLE_DISCONNECT to signal local console has disconnected from the account
			another BUT... I don't get the WTS_SESSION_UNLOCK for that account
			but I do get
				WTS_CONSOLE_CONNECT after this account is unlocked again

			So, summarizing...

			Single-user:
				WTS_SESSION_LOCK (pause)
				WTS_SESSION_UNLOCK (unpause)

			Multi-user:
				WTS_SESSION_LOCK (pause)
				WTS_SESSION_UNLOCK (unpause)
				
						or

				WTS_SESSION_LOCK (pause)
				WTS_CONSOLE_DISCONNECT
				WTS_SESSION_UNLOCK (ignore)
				WTS_CONSOLE_CONNECT (unpause)

			...WHEW! */

		case WM_WTSSESSION_CHANGE:
			if (wParam == WTS_SESSION_LOCK)
			{
				lock();
			}
			else if (wParam == WTS_SESSION_UNLOCK)
			{
				if (!m_bDisconnected) unlock();
			}
			else if (wParam == WTS_CONSOLE_CONNECT)
			{
				unlock();
				m_bDisconnected = FALSE;
			}
			else if (wParam == WTS_CONSOLE_DISCONNECT)
			{
				m_bDisconnected = TRUE;
			}
			break;

		default:
			return uDefWindowProc(hWnd, uMessage, wParam, lParam);
		}

		return 0;
	}

	HINSTANCE     m_hInstance;
	HWND          m_hWnd;
	const TCHAR * m_lpszClassName;
	BOOL          m_bRegistered;

	HMODULE								hWTSAPI;
	pWTSRegisterSessionNotification		register_session;
	pWTSUnRegisterSessionNotification	unregister_session;

	BOOL		m_bDisconnected;
};




static VOID CALLBACK IsWorkstationLocked(HWND hwnd,UINT message,UINT idEvent,DWORD dwTime)
{
	HDESK hd;
	char buf[256];
	BOOL isLocked = FALSE;
	hd = OpenInputDesktop(0,FALSE,MAXIMUM_ALLOWED);
	if (hd==NULL) isLocked=TRUE;
	else
	{
		GetUserObjectInformation(hd,UOI_NAME,buf,sizeof(buf),NULL);
		CloseDesktop(hd);
		if (strcmp(buf,"Winlogon")==0) isLocked=TRUE;
	}

	if (isLocked)
	{
		if (!cfg_waslocked)
		{
			lock();
		}
	}
	else
	{
		if (cfg_waslocked)
		{
			unlock();
		}
	}
}


static void enable()
{
	if (win_ver)
	{
		g_sessionwnd = new CSessionWnd;
		if (!g_sessionwnd->Initialize(core_api::get_my_instance()))
		{
			delete g_sessionwnd;
			g_sessionwnd = NULL;
			hTimer = SetTimer(NULL, 0, 1000, IsWorkstationLocked);
		}
	}
}

static void disable()
{
	if (win_ver)
	{
		if (g_sessionwnd)
		{
			delete g_sessionwnd;
			g_sessionwnd = NULL;
		}
		else
		{
			if (hTimer)
			{
				KillTimer(NULL, hTimer);
				hTimer = 0;
			}
		}
	}
}





class initquit_foolock : public initquit {
	virtual void on_init() {
		if (IsWinVer2000Plus())
		{
			win_ver = TRUE;
			if (cfg_enabled) enable();
		}
	}

	virtual void on_quit() {
		disable();
	}
};
static initquit_factory <initquit_foolock> foo_initquit;





class menu_item_main_foolock : public menu_item_legacy_main {
	virtual unsigned get_num_items() {
		return 1;
	}

	virtual void get_item_name(unsigned n, pfc::string_base & out) {
		out = "Pause on lock";
	}

	virtual void get_item_default_path(unsigned n, pfc::string_base & out) {
		out = "Components";
	}

	virtual bool get_item_description(unsigned n, pfc::string_base & out) {
		return false;
	}

	virtual GUID get_item_guid(unsigned n)
	{
		// {AF0E45EF-3C49-4d54-A3D2-DE8134813FFA}
		static const GUID guid = 
		{ 0xaf0e45ef, 0x3c49, 0x4d54, { 0xa3, 0xd2, 0xde, 0x81, 0x34, 0x81, 0x3f, 0xfa } };
		return guid;
	}
	
	virtual void perform_command(unsigned n) {
		if (n==0 && core_api::assert_main_thread()) {
			if (cfg_enabled) {
				cfg_enabled=FALSE;
				disable();
			}
			else {
				cfg_enabled=TRUE;
				enable();
			}
		}
	}

	virtual bool is_checked(unsigned n) {
		if (n == 0) return !!cfg_enabled;
		return false;
	}

	virtual bool is_disabled(unsigned n) {
		return !win_ver;
	}

	virtual bool get_description(unsigned n, pfc::string_base & out) {
		if (n == 0) {
			out = "Toggles PauseOnLock";
			return true;
		}
		return false;
	}
};

static menu_item_factory_t <menu_item_main_foolock> g_menu_item_main_factory;
