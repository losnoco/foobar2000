#include <foobar2000.h>
#include <ShObjIdl.h>

#include "resource.h"

// {0EA3A6C2-0DF3-43af-B24B-0D2E18190C73}
static const GUID guid_cfg_hide_from_taskbar = 
{ 0xea3a6c2, 0xdf3, 0x43af, { 0xb2, 0x4b, 0xd, 0x2e, 0x18, 0x19, 0xc, 0x73 } };

static cfg_int cfg_hide_from_taskbar(guid_cfg_hide_from_taskbar, 0);

class CHackWnd;

critical_section sync;
CHackWnd * wnd = NULL;

class CHackWnd
{
public:
	CHackWnd() : m_lpszClassName("2B208F73-04F7-4e74-AC94-7757168D9004")
	{
		m_bInitialized = FALSE;
		m_bRegistered = FALSE;
		m_bShowing = FALSE;
		m_hWnd = NULL;
	}

	bool Initialize(HINSTANCE hInstance)
	{
		m_hInstance = hInstance;

		HRESULT rv = CoInitialize(NULL);
		if (rv != S_OK && rv != S_FALSE)
		{
			if (rv == RPC_E_CHANGED_MODE)
			{
				// feh
			}
			else
			{
				return false;
			}
		}
		else
		{
			m_bInitialized = TRUE;
		}

		uWNDCLASS wcl;
		memset(&wcl, 0, sizeof(WNDCLASS));
		wcl.hInstance = hInstance;
		wcl.lpfnWndProc = (WNDPROC)WndProc;
		wcl.lpszClassName = m_lpszClassName;
		if (!uRegisterClass(&wcl)) return false;

		m_bRegistered = TRUE;

		m_hWnd = uCreateWindowEx(WS_EX_TOOLWINDOW, m_lpszClassName, "uninteresting", WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP, 0, 0, 0, 0, 0, 0, hInstance, this);

		return !!m_hWnd;
	}

	~CHackWnd()
	{
		if (m_bShowing) Show();
		if (IsWindow(m_hWnd)) DestroyWindow(m_hWnd);
		if (m_bRegistered)
			uUnregisterClass(m_lpszClassName, m_hInstance);
		if (m_bInitialized)
			CoUninitialize();
	}

	inline HWND GetHandle() const
	{
		return m_hWnd;
	}

	void Show(bool status = false)
	{
		HWND w = core_api::get_main_window();
		ITaskbarList * itbl = NULL;
		
		if (CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_ALL, IID_ITaskbarList, (LPVOID *) &itbl) == S_OK && itbl)
		{
			itbl->HrInit();
			if (status)
			{
				itbl->AddTab(w);
				m_bShowing = TRUE;
			}
			else
			{
				itbl->DeleteTab(w);
				m_bShowing = FALSE;
			}
			itbl->Release();
		}
	}

private:
	static LRESULT CALLBACK WndProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
	{
		if(msg == WM_NCCREATE)
		{
			CHackWnd * pThis = (CHackWnd *)((CREATESTRUCT *)(lp))->lpCreateParams;
			uSetWindowLong(wnd, GWL_USERDATA, (long)pThis);
		}
		else if (msg == WM_CREATE)
		{
			CHackWnd * pThis = reinterpret_cast<CHackWnd *>(uGetWindowLong(wnd, GWL_USERDATA));
			HWND w = core_api::get_main_window();
			if (pThis && cfg_hide_from_taskbar) pThis->Show();
			SetClassLong(wnd, GCL_HICON, (LONG)GetClassLong(w, GCL_HICON));
			uSetWindowLong(w, GWL_EXSTYLE, uGetWindowLong(w, GWL_EXSTYLE) | WS_EX_APPWINDOW);
			uSetWindowLong(w, GWL_HWNDPARENT, (LONG)wnd);
		}
		else if (msg == WM_DESTROY)
		{
			uSetWindowLong(core_api::get_main_window(), GWL_HWNDPARENT, (LONG)HWND_DESKTOP);
		}
		else if (msg == WM_GETICON)
		{
			return uSendMessage(core_api::get_main_window(), msg, wp, lp);
		}
		
		return uDefWindowProc(wnd, msg, wp, lp);
	}
	
	HINSTANCE	m_hInstance;
	HWND		m_hWnd;
	const char *m_lpszClassName;
	BOOL        m_bInitialized;
	BOOL		m_bRegistered;
	BOOL        m_bShowing;
};

static void g_hide_taskbar_button(int status)
{
	insync(sync);

	switch (status)
	{
	case 1:
		if (!wnd)
		{
			wnd = new CHackWnd;
			if (!wnd->Initialize(core_api::get_my_instance()))
			{
				delete wnd;
				wnd = NULL;
			}
		}
		else
			wnd->Show(false);

		break;

	default:
		if (wnd) wnd->Show(true);
		break;
	}
}

class taskbar_setup : public initquit
{
	void on_init()
	{
		g_hide_taskbar_button(cfg_hide_from_taskbar);
	}

	void on_quit()
	{
		insync(sync);
		delete wnd;
		wnd = NULL;
	}
};

class taskbar_toggle : public menu_item_legacy_main
{
	virtual unsigned get_num_items()
	{
		return 1;
	}

	virtual void get_item_name(unsigned n, string_base & out) {
		out = "Toggle taskbar button";
	}

	virtual void get_item_default_path(unsigned n, string_base & out) {
		out = "Components";
	}

	virtual bool get_item_description(unsigned n, string_base & out) {
		return false;
	}

	virtual GUID get_item_guid(unsigned n)
	{
		// {A6803244-3CE8-44c4-92ED-5FE198F18300}
		static const GUID guid = 
		{ 0xa6803244, 0x3ce8, 0x44c4, { 0x92, 0xed, 0x5f, 0xe1, 0x98, 0xf1, 0x83, 0x0 } };
		return guid;
	}

	virtual void perform_command(unsigned n)
	{
		cfg_hide_from_taskbar = !cfg_hide_from_taskbar;
		g_hide_taskbar_button(cfg_hide_from_taskbar);
	}
};

class taskbar_preferences : public preferences_page
{
	static BOOL CALLBACK DialogProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		switch(msg)
		{
		case WM_INITDIALOG:
			uSendDlgItemMessage(wnd, IDC_HIDE, BM_SETCHECK, cfg_hide_from_taskbar, 0);
			break;

		case WM_COMMAND:
			switch (wp)
			{
			case IDC_HIDE:
				cfg_hide_from_taskbar = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
				g_hide_taskbar_button(cfg_hide_from_taskbar);
				break;
			}
			break;
		}
		return 0;
	}

	virtual HWND create(HWND parent)
	{
		return uCreateDialog(IDD_CONFIG, parent, DialogProc);
	}
	GUID get_guid()
	{
		// {C3F587AC-524D-4d23-8DF3-D5C907D4625F}
		static const GUID guid = 
		{ 0xc3f587ac, 0x524d, 0x4d23, { 0x8d, 0xf3, 0xd5, 0xc9, 0x7, 0xd4, 0x62, 0x5f } };
		return guid;
	}
	virtual const char * get_name() {return "Taskbar button control";}
	GUID get_parent_guid() {return guid_components;}

	bool reset_query() {return true;}
	void reset()
	{
		if (cfg_hide_from_taskbar)
		{
			g_hide_taskbar_button(0);
			cfg_hide_from_taskbar = 0;
		}
	}
};

static menu_item_factory_t<taskbar_toggle> foo1;
static initquit_factory<taskbar_setup> foo2;
static preferences_page_factory_t<taskbar_preferences> foo3;

DECLARE_COMPONENT_VERSION("Hide from taskbar", "1.0", "Hackish component for removing the\nmain window from the taskbar.");
