#define WIN32_LEAN_AND_MEAN
#include "leakchk.h"

#include <windows.h>
#include <windowsx.h>

/* Winamp SDK headers */
#include <winamp/wa_ipc.h>
#ifndef IPC_GETINIFILEW
#define IPC_GETINIFILEW 1334
#endif

#include "../pversion.h"
#include "in_xsfcfg.h"

#if _MSC_VER >= 1200
#pragma warning(disable:4290)
#endif

namespace
{

xsfc::TAutoPtr<IConfigIO> pcfg;
prefsDlgRec preftop;
prefsDlgRec prefwork;
int prefreg = 0;
HMODULE hDLL = 0;
const char guidpreftop[] = "{90AABF06-1A41-4a1f-8C5C-6E98D653D33A}";
}

class IniConfig : public IConfigIO
{
	xsfc::TString sIniPath;
protected:
	IniConfig() throw()
	{
	}

	bool Initialize(HWND hwndWinamp) throw(xsfc::EShortOfMemory)
	{
		LPWSTR iniW = (LPWSTR)(::SendMessageW(hwndWinamp, WM_WA_IPC, 0, IPC_GETINIFILEW));
		if (iniW && intptr_t(iniW) != 1 && !IsBadStringPtrW(iniW, MAX_PATH))
		{
			sIniPath = xsfc::TString(iniW);
			return true;
		}
		LPSTR iniA = (LPSTR)(::SendMessageA(hwndWinamp, WM_WA_IPC, 0, IPC_GETINIFILE));
		if (iniA && intptr_t(iniA) != 1 && !IsBadStringPtrA(iniA, MAX_PATH))
		{
			sIniPath = xsfc::TString(iniA);
			return true;
		}
		sIniPath = xsfc::TWin32::ExtractPath(xsfc::TWin32::ModulePath(NULL)) + "winamp.ini";
		return true;
	}

public:
	~IniConfig() throw()
	{
	}

	void SetULong(const wchar_t *name, const unsigned long value) throw()
	{
		try
		{
			xsfc::TWin32::SetPrivateProfile(sIniPath, xsfc::TString(XSFDRIVER_SIMPLENAME), name, xsfc::TString(value));
		}
		catch (xsfc::EShortOfMemory e)
		{
		}
	}
	unsigned long GetULong(const wchar_t *name, const unsigned long defaultvalue = 0) throw()
	{
		try
		{
			xsfc::TString value = xsfc::TWin32::GetPrivateProfile(sIniPath, xsfc::TString(XSFDRIVER_SIMPLENAME), name, xsfc::TString(defaultvalue));
			if (value[0]) return value.GetULong();
		}
		catch (xsfc::EShortOfMemory e)
		{
		}
		return defaultvalue;
	}
	void SetFloat(const wchar_t *name, const double value) throw()
	{
		try
		{
			xsfc::TWin32::SetPrivateProfile(sIniPath, xsfc::TString(XSFDRIVER_SIMPLENAME), name, xsfc::TString(value));
		}
		catch (xsfc::EShortOfMemory e)
		{
		}
	}
	double GetFloat(const wchar_t *name, const double defaultvalue = 0) throw()
	{
		try
		{
			xsfc::TString value = xsfc::TWin32::GetPrivateProfile(sIniPath, xsfc::TString(XSFDRIVER_SIMPLENAME), name, xsfc::TString(defaultvalue));
			if (value[0]) return value.GetFloat();
		}
		catch (xsfc::EShortOfMemory e)
		{
		}
		return defaultvalue;
	}
	void SetString(const wchar_t *name, const wchar_t *value) throw()
	{
		try
		{
			xsfc::TWin32::SetPrivateProfile(sIniPath, xsfc::TString(XSFDRIVER_SIMPLENAME), name, value);
		}
		catch (xsfc::EShortOfMemory e)
		{
		}
	}
	xsfc::TString GetString(const wchar_t *name, const wchar_t *defaultvalue = 0) throw(xsfc::EShortOfMemory)
	{
		return xsfc::TWin32::GetPrivateProfile(sIniPath, xsfc::TString(XSFDRIVER_SIMPLENAME), name, defaultvalue);
	}

	static LPIConfigIO Create(HWND hwndWinamp) throw(xsfc::EShortOfMemory)
	{
		xsfc::TAutoPtr<IniConfig> cfg(new(xsfc::nothrow) IniConfig);
		return (cfg && cfg->Initialize(hwndWinamp)) ? cfg.Detach() : 0;
	}
};

static BOOL CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam))
		{
		case IDOK:
			CFGUpdate(pcfg, hwndDlg);
			CFGSave(pcfg);
			::EndDialog(hwndDlg, IDOK);
			break;
		case IDCANCEL:
			::EndDialog(hwndDlg, IDCANCEL);
			break;
		case 0x206:
			if (GET_WM_COMMAND_CMD(wParam, lParam) == LBN_SELCHANGE)
			{
				CFGMuteChange(hwndDlg, 0x206);
			}
			break;
		default:
			return FALSE;
		}
		break;
	case WM_INITDIALOG:
		::SetWindowTextA(hwndDlg, WINAMPPLUGIN_NAME);
		::ShowWindow(GetDlgItem(hwndDlg, IDOK), SW_SHOWNA);
		::ShowWindow(GetDlgItem(hwndDlg, IDCANCEL), SW_SHOWNA);
		CFGLoad(pcfg);
		CFGReset(pcfg, hwndDlg);
		return TRUE;
	}
	return FALSE;
}

static BOOL CALLBACK DialogProcPrefTop(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		return TRUE;
	}
	return FALSE;
}

static BOOL CALLBACK DialogProcPref(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static bool initialized = false;
	switch (uMsg)
	{
	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, LpARAM))
		{
		case 0x200: case 0x201: case 0x202:
		case 0x203: case 0x204: case 0x205:
			if (initialized)
			{
				CFGUpdate(pcfg, hwndDlg);
				CFGSave(pcfg);
			}
			break;
		case 0x206:
			if (GET_WM_COMMAND_CMD(wParam, lParam) == LBN_SELCHANGE)
			{
				CFGMuteChange(hwndDlg, 0x206);
			}
			break;
		default:
			return FALSE;
		}
		break;
	case WM_INITDIALOG:
		initialized = false;
		CFGLoad(pcfg);
		CFGReset(pcfg, hwndDlg);
		initialized = true;
		return TRUE;
	}
	return FALSE;
}

void winamp_config_init(HINSTANCE hinstDLL)
{
	hDLL = hinstDLL;
}
void winamp_config_load(HWND hwndWinamp)
{
	if (!pcfg)
	{
		pcfg = IniConfig::Create(hwndWinamp);
		if (!pcfg) pcfg = NullConfig::Create();
		CFGLoad(pcfg);
	}
}


void winamp_config_add_prefs(HWND hwndWinamp)
{
	if (prefreg) return;

	if (((intptr_t)xsfc::TWin32::WndMsgSend(hwndWinamp, WM_WA_IPC, 0, IPC_GETVERSION) & 0xffffU) >= 0x2090)
	{
		/* 2.9+ */
		intptr_t ppos = (intptr_t)GetProp(hwndWinamp, guidpreftop);
		if (!ppos)
		{
			preftop.hInst = hDLL;
			preftop.dlgID = 2;
			preftop.proc = (void *)DialogProcPrefTop;
			preftop.name = "Input / XSF";
			preftop.where = 1;
			preftop._id = 0;
			preftop.next = 0;
			xsfc::TWin32::WndMsgSend(hwndWinamp, WM_WA_IPC, &preftop, IPC_ADD_PREFS_DLG);
			ppos = (intptr_t)&preftop;
			SetProp(hwndWinamp, guidpreftop, (HANDLE)ppos);
		}

		prefwork.hInst = hDLL;
		prefwork.dlgID = 1;
		prefwork.proc = (void *)DialogProcPref;
		prefwork.name = XSFDRIVER_SIMPLENAME;
		prefwork.where = ppos;
		prefwork._id = 0;
		prefwork.next = 0;
		prefreg = !(intptr_t)xsfc::TWin32::WndMsgSend(hwndWinamp, WM_WA_IPC, &prefwork, IPC_ADD_PREFS_DLG);
	}
}

void winamp_config_remove_prefs(HWND hwndWinamp)
{

	if (pcfg) pcfg.Release();
	if (prefreg)
	{
		xsfc::TWin32::WndMsgSend(hwndWinamp, WM_WA_IPC, &prefwork, IPC_REMOVE_PREFS_DLG);
		prefreg = 0;
	}
	intptr_t ppos = (intptr_t)GetProp(hwndWinamp, guidpreftop);
	if (!ppos)
	{
		SetProp(hwndWinamp, guidpreftop, (HANDLE)0);
		xsfc::TWin32::WndMsgSend(hwndWinamp, WM_WA_IPC, &preftop, IPC_REMOVE_PREFS_DLG);
	}
}

void winamp_config_dialog(HWND hwndWinamp, HWND hwndParent)
{
	HRSRC hrsrc;
	DWORD dwSize;
	HGLOBAL hGlobal;
	xsfc::TSimpleArray<BYTE> resbuf;
	if (prefreg)
	{
		if (((intptr_t)xsfc::TWin32::WndMsgSend(hwndWinamp, WM_WA_IPC, 0, IPC_GETVERSION) & 0xffffU) >= 0x5000)
		{
			xsfc::TWin32::WndMsgSend(hwndWinamp, WM_WA_IPC, &prefwork, IPC_OPENPREFSTOPAGE);
			return; 
		}
	}

	hrsrc = ::FindResourceA(hDLL, MAKEINTRESOURCE(1), RT_DIALOG);
	if (!hrsrc) return;

	dwSize = ::SizeofResource(hDLL, hrsrc);
	hGlobal = ::LoadResource(hDLL, hrsrc);

	if (dwSize < 4 || !hGlobal) return;
	
	if (!resbuf.Resize(dwSize)) return;
	
	LPBYTE lpResource = resbuf.Ptr();

	::CopyMemory(lpResource, hGlobal, dwSize);

	lpResource[0] = '\xc0';
	lpResource[1] = '\x08';
	lpResource[2] = '\xc8';
	lpResource[3] = '\x80';

	xsfc::TWin32::DlgInvoke(::GetModuleHandleA(NULL), lpResource, NULL, DialogProc, NULL);
}