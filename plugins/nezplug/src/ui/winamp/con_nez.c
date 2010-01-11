/*
	KUMAamp project(âºèÃ)
	Copyright (C) Mamiya 2000.
	License:LGPL
	---------------------------------------------------------------------------
	NEZamp Controler
	con_nez.c
*/

#include "in_nez.h"

/* ANSI/Windows standard headers */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>

/* Libraries headers */
#ifdef __cplusplus
extern "C" {
#endif
#include "minisdk/in2.h"
#include "minisdk/wafe.h"
#define WM_WA_UNKNOWN (WM_USER + 1)
#define WM_WA_MPEG_EOF (WM_USER + 2)
#ifdef __cplusplus
}
#endif

/* Project headers */
#include "con_nez.h"
#include "common/win32/rc/nezplug.rh"
#include "common/cso.h"

#ifdef __cplusplus
extern "C" In_Module __declspec(dllexport) * winampGetInModule2(void);
#else
extern In_Module __declspec(dllexport) * winampGetInModule2(void);
#endif
#define hwndWinamp (winampGetInModule2()->hMainWindow)

static int num_songs = -1;
static int cur_song = 0;

static BOOL bControlerEnable = FALSE;
static BOOL bControlerPlayEnable = TRUE;
static WNDPROC wpOrigDialogProc = NULL;
static HWND hwndControler = NULL;
static CRITICAL_SECTION_OBJECT(controlerbuild_lock);
static int iControlerRequest = 0;
static int restart = 0;
static int isplaying = 0;
enum CONTROLER_REQUEST
{
	CR_SHOW,
	CR_HIDE,
	CR_UPDATE
};

static struct
{
	BOOL installed;
	BOOL enabled;
	HWND hwndWA;
	HWND hwndEQ;
	HWND hwndPE;
	HHOOK hhookCALLWNDPROC;
	HHOOK hhookGETMESSAGE;
	HHOOK hhookKEYBOARD;
} subclasswinamp = { FALSE, FALSE, };
static CRITICAL_SECTION_OBJECT(subclasswinamp_lock);
typedef enum {
	SUBCLASS_WA,
	SUBCLASS_EQ,
	SUBCLASS_PE
} SUBCLASS_TYPE;

static void ControlerRequest(enum CONTROLER_REQUEST req)
{
	if (hwndControler != NULL)
	{
		iControlerRequest |= 1 << req;
		PostMessage(hwndControler, WM_APP, 0, 0);
	}
}
static void ControlerPlay(void)
{
	if (bControlerPlayEnable && hwndWinamp != NULL)
	{
#if 0
		restart = 1;
#else
		/* Push (WINAMP) play button */
		PostMessage(hwndWinamp, WM_COMMAND, WINAMP_BUTTON2, 0);
#endif
	}
}

int GetControler(void)
{
	return cur_song;
}

int GetRestart(void)
{
	int ret;
	ret = restart ? cur_song : 0;
	restart = 0;
	return ret;
}

void SetStateControler(int state)
{
	if (state)
	{
		isplaying = 1;
	}
	else
	{
		isplaying = 0;
	}
}

/*
	-----------------------------------------------------------------
	Called by DlgProc & Subclassed WndProc of Controler Dialog
	(SendMessage API is used)
	-----------------------------------------------------------------
*/
static void AdjustPosition(HWND hwnd)
{
	static CRITICAL_SECTION_OBJECT(lock);
	RECT rect1, rect2;
	if (CRITICAL_SECTION_ENTER(lock))
	{
		if (hwndWinamp != NULL)
		{
			GetWindowRect(hwndWinamp, &rect1);
			GetWindowRect(hwnd, &rect2);
			SetWindowPos(hwnd, hwndWinamp, rect1.left, rect1.bottom,rect1.right - rect1.left, rect2.bottom - rect2.top,SWP_NOACTIVATE);
		}
		CRITICAL_SECTION_LEAVE(lock);
	}
}
static void SetControler(HWND hwnd, int num)
{
	SetDlgItemInt(hwnd, IDC_NEZSONG, num, FALSE);
}

static void ControlerPlayState(HWND hwnd)
{
	char buf[128];
	wsprintf(buf, "/%d%c", num_songs, bControlerPlayEnable ? '*' : ' ');
	SetDlgItemText(hwnd, IDC_NEZNUMSONGS, buf); 
}

static void CheckPlaying(void)
{
	if (!isplaying && SendMessage(hwndWinamp,WM_WA_IPC,0,IPC_ISPLAYING))
	{
		DisableControler();
	}
}

/*
	-----------------------------------------------------------------
	DlgProc of Controler Dialog
	-----------------------------------------------------------------
*/
static BOOL CALLBACK ControlerDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
			return TRUE;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_NEZSONGM10:
					if (BN_CLICKED == HIWORD(wParam))
					{
						int songno = GetControler() - 10;
						if (songno < 1) songno = 1;
						SetControler(hwnd, songno);
					}
					break;
				case IDC_NEZSONGP10:
					if (BN_CLICKED == HIWORD(wParam))
					{
						int songno = GetControler() + 10;
						if (songno > num_songs) songno = num_songs;
						SetControler(hwnd, songno);
					}
					break;
				case IDC_NEZNUMSONGS:
					if (BN_CLICKED == HIWORD(wParam))
					{
						bControlerPlayEnable = !bControlerPlayEnable;
						ControlerPlayState(hwnd);
					}
					break;
				case IDC_NEZSONG:
					if (EN_CHANGE == HIWORD(wParam))
					{
						BOOL bTemp;
						int song;
						song = GetDlgItemInt(hwnd, IDC_NEZSONG, &bTemp, FALSE);
						if (bTemp)
						{
							if (0 < song && song <= num_songs && cur_song != song)
							{
								cur_song = song;
								ControlerPlay();
							}
						}
					}
					break;
			}
			break;
	}
	return FALSE;
}

/*
	-----------------------------------------------------------------
	Subclassed WndProc of Controler Dialog
	-----------------------------------------------------------------
*/
static LRESULT CALLBACK SubclassDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_TIMER:
			if (bControlerEnable)
			{
				AdjustPosition(hwnd);
				CheckPlaying();
			}
			SetTimer(hwnd, wParam, 100, NULL);
			break;
		case WM_APP:
			if (iControlerRequest & (1 << CR_SHOW))
			{
				AdjustPosition(hwnd);
				ControlerPlayState(hwnd);
				SendDlgItemMessage(hwnd, IDC_NEZSONGSPIN, UDM_SETRANGE, 0, MAKELONG(num_songs,1));
				SetControler(hwnd, cur_song);
				iControlerRequest &= ~((1 << CR_SHOW) | (1 << CR_UPDATE));
				ShowWindow(hwnd, SW_SHOWNA);
			}
			if (iControlerRequest & (1 << CR_UPDATE))
			{
				ControlerPlayState(hwnd);
				SendDlgItemMessage(hwnd, IDC_NEZSONGSPIN, UDM_SETRANGE, 0, MAKELONG(num_songs,1));
				SetControler(hwnd, cur_song);
				iControlerRequest &= ~(1 << CR_UPDATE);
			}
			if (iControlerRequest & (1 << CR_HIDE))
			{
				ShowWindow(hwnd, SW_HIDE);
				iControlerRequest &= ~(1 << CR_HIDE);
			}
			break;
		case WM_DESTROY:
			{
				return CallWindowProc(wpOrigDialogProc, hwnd, uMsg, wParam, lParam); 
			}
			break;
	}
	return CallWindowProc(wpOrigDialogProc, hwnd, uMsg, wParam, lParam); 
}


/*
	-----------------------------------------------------------------
	Subclassing WndProc of Controler
	-----------------------------------------------------------------
*/
static void SubclassDialog(HWND hwnd)
{
	if (wpOrigDialogProc == NULL && hwnd != NULL)
	{
		wpOrigDialogProc = (WNDPROC)SetWindowLong(hwnd, GWL_WNDPROC, (LONG)SubclassDialogProc);
	}
}

/*
	-----------------------------------------------------------------
	Create Controler Dialog
	-----------------------------------------------------------------
*/
static void CreateControler(void)
{
	if (CRITICAL_SECTION_ENTER(controlerbuild_lock))
	{
		if (hwndControler == NULL)
		{
			HINSTANCE hInst = (HINSTANCE)GetModuleHandle(NULL);
			hwndControler = CreateDialog(
				winampGetInModule2()->hDllInstance,
				MAKEINTRESOURCE(IDD_NEZSONGNO),
				GetDesktopWindow(),
				ControlerDlgProc
			);
			SubclassDialog(hwndControler);
			PostMessage(hwndControler, WM_TIMER, 1, 0);
		}
		CRITICAL_SECTION_LEAVE(controlerbuild_lock);
	}
}

/*
	-----------------------------------------------------------------
	Destroy Controler Dialog
	-----------------------------------------------------------------
*/
static void DestroyControler(void)
{
	if (CRITICAL_SECTION_ENTER(controlerbuild_lock))
	{
		if (hwndControler != NULL) DestroyWindow(hwndControler);
		CRITICAL_SECTION_LEAVE(controlerbuild_lock);
	}
}

void UnsubclassWinamp(void)
{
	subclasswinamp.enabled = FALSE;
}

static LRESULT CALLBACK CallWndProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		CWPSTRUCT *pmsg = (CWPSTRUCT *)lParam;
		switch (pmsg->message)
		{
			case WM_COMMAND:
			case WM_SYSCOMMAND:
				if (!subclasswinamp.enabled) break;
				switch (LOWORD(pmsg->wParam))
				{
					case WINAMP_JUMP10FWD:
					case WINAMP_BUTTON5:	/* fwd button */
						if (cur_song < num_songs)
						{
							cur_song += (LOWORD(pmsg->wParam) == WINAMP_JUMP10FWD) ? 10: 1;
							if (cur_song > num_songs) cur_song = num_songs;
							PostMessage(subclasswinamp.hwndWA, WM_COMMAND, WINAMP_BUTTON2, 0);
						}
						break;
					case WINAMP_JUMP10BACK:
					case WINAMP_BUTTON1:	/* back button */
						if (cur_song > 1)
						{
							cur_song -= (LOWORD(pmsg->wParam) == WINAMP_JUMP10BACK) ? 10 : 1;
							if (cur_song < 1) cur_song = 1;
							PostMessage(subclasswinamp.hwndWA, WM_COMMAND, WINAMP_BUTTON2, 0);
						}
						break;
				}
				break;
		}
	}
	return CallNextHookEx(subclasswinamp.hhookCALLWNDPROC, nCode, wParam, lParam);
}
static LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		MSG *pmsg = (MSG *)lParam;
		switch (pmsg->message)
		{
			case WM_WA_MPEG_EOF:
				if (!subclasswinamp.enabled) break;
				if (cur_song < num_songs)
				{
					cur_song++;
					pmsg->message = WM_COMMAND;
					pmsg->wParam = WINAMP_BUTTON4;
					PostMessage(subclasswinamp.hwndWA, WM_COMMAND, WINAMP_BUTTON2, 0);
				}
				UnsubclassWinamp();
				break;
			case WM_COMMAND:
			case WM_SYSCOMMAND:
				if (!subclasswinamp.enabled) break;
				switch (LOWORD(pmsg->wParam))
				{
					case WINAMP_JUMP10FWD:
					case WINAMP_BUTTON5:	/* fwd button */
						if (cur_song < num_songs)
						{
							cur_song += (LOWORD(pmsg->wParam) == WINAMP_JUMP10FWD) ? 10: 1;
							if (cur_song > num_songs) cur_song = num_songs;
							pmsg->message = WM_COMMAND;
							pmsg->wParam = WINAMP_BUTTON2;
						}
						else
						{
							pmsg->message = WM_COMMAND;
							pmsg->wParam = WINAMP_BUTTON5;
						}
						break;
					case WINAMP_JUMP10BACK:
					case WINAMP_BUTTON1:	/* back button */
						if (cur_song > 1)
						{
							cur_song -= (LOWORD(pmsg->wParam) == WINAMP_JUMP10BACK) ? 10 : 1;
							if (cur_song < 1) cur_song = 1;
							pmsg->message = WM_COMMAND;
							pmsg->wParam = WINAMP_BUTTON2;
						}
						else
						{
							pmsg->message = WM_COMMAND;
							pmsg->wParam = WINAMP_BUTTON1;
						}
						break;
				}
				break;
		}
	}
	return CallNextHookEx(subclasswinamp.hhookGETMESSAGE, nCode, wParam, lParam);}

static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	while (nCode == HC_ACTION && (lParam & 0xc0000000) == 0 && subclasswinamp.enabled)
	{
		HWND hFocus = GetFocus();
		if ((hFocus == NULL) || (
			(hFocus != subclasswinamp.hwndWA) &&
			(hFocus != subclasswinamp.hwndEQ) &&
			(hFocus != subclasswinamp.hwndPE)
		)) break;
		switch (wParam)
		{
		case VK_NUMPAD4:	case 'Z':	
			PostMessage(subclasswinamp.hwndWA, WM_COMMAND, WINAMP_BUTTON1, 0);
			return 1;
		case VK_NUMPAD1:
			PostMessage(subclasswinamp.hwndWA, WM_COMMAND, WINAMP_JUMP10BACK, 0);
			return 1;
		case VK_NUMPAD6:	case 'B':
			PostMessage(subclasswinamp.hwndWA, WM_COMMAND, WINAMP_BUTTON5, 0);
			return 1;
		case VK_NUMPAD3:
			PostMessage(subclasswinamp.hwndWA, WM_COMMAND, WINAMP_JUMP10FWD, 0);
			return 1;
		}
		break;
	}
	return CallNextHookEx(subclasswinamp.hhookKEYBOARD, nCode, wParam, lParam);
}

static void UninstallSubclassWinamp(void)
{
	if (subclasswinamp.installed)
	{
		if (subclasswinamp.hhookCALLWNDPROC)
		{
			UnhookWindowsHookEx(subclasswinamp.hhookCALLWNDPROC);
		}
		if (subclasswinamp.hhookGETMESSAGE)
		{
			UnhookWindowsHookEx(subclasswinamp.hhookGETMESSAGE);
		}
		if (subclasswinamp.hhookKEYBOARD)
		{
			UnhookWindowsHookEx(subclasswinamp.hhookKEYBOARD);
		}
		subclasswinamp.hwndWA = NULL;
		subclasswinamp.hwndEQ = NULL;
		subclasswinamp.hwndPE = NULL;
		subclasswinamp.installed = FALSE;
	}
}

typedef struct
{
	DWORD dwThreadId;
	LPCSTR lpszClassName;
	HWND hwndRet;
} FINDTHREADWINDOW_WORK;

static BOOL CALLBACK FindThreadWindowsProc(HWND hwnd, LPARAM lParam)
{
	FINDTHREADWINDOW_WORK *pWork = (FINDTHREADWINDOW_WORK *)lParam;
	if (GetWindowThreadProcessId(hwnd, NULL) == pWork->dwThreadId)
	{
#define MAX_CLASS_NAME MAX_PATH
		CHAR szClassName[MAX_CLASS_NAME];
		if (GetClassName(hwnd, szClassName, MAX_CLASS_NAME))
		{
			if (lstrcmp(szClassName, pWork->lpszClassName) == 0)
			{
				pWork->hwndRet = hwnd;
				return FALSE;
			}
		}
	}
	return TRUE;
}

static HWND FindThreadWindow(LPCSTR lpszClassName, DWORD dwThreadId)
{
	FINDTHREADWINDOW_WORK fwww;
	fwww.dwThreadId = dwThreadId;
	fwww.lpszClassName = lpszClassName;
	fwww.hwndRet = NULL;
	EnumWindows(FindThreadWindowsProc, (LONG)&fwww);
	return fwww.hwndRet;
}

void SubclassWinamp(void)
{
	if (!subclasswinamp.installed && hwndWinamp != NULL)
	{
		DWORD dwThreadId;
		subclasswinamp.hwndWA = hwndWinamp;
		dwThreadId = GetWindowThreadProcessId(subclasswinamp.hwndWA, NULL);
		subclasswinamp.hwndEQ = FindThreadWindow("Winamp EQ", dwThreadId);
		subclasswinamp.hwndPE = FindThreadWindow("Winamp PE", dwThreadId);
		subclasswinamp.hhookCALLWNDPROC = SetWindowsHookEx(WH_CALLWNDPROC, CallWndProc, NULL, dwThreadId);
		subclasswinamp.hhookGETMESSAGE = SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc, NULL, dwThreadId);
		subclasswinamp.hhookKEYBOARD = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, NULL, dwThreadId);
		subclasswinamp.installed = TRUE;
	}
	if (subclasswinamp.installed) subclasswinamp.enabled = TRUE;
}

/*
	-----------------------------------------------------------------
	Install Controler Dialog
	-----------------------------------------------------------------
*/
void InstallControler(int bControlerEnable)
{
	if (bControlerEnable) CreateControler();
}

/*
	-----------------------------------------------------------------
	Uninstall Controler Dialog
	-----------------------------------------------------------------
*/
void UninstallControler(void)
{
	DisableControler();
	DestroyControler();
	UninstallSubclassWinamp();
}

/*
	-----------------------------------------------------------------
	Enable Controler Dialog
	-----------------------------------------------------------------
*/
void EnableControler(int starts, int nums)
{
	if (nums == -1)
	{
		cur_song = starts;
		if (bControlerEnable) ControlerRequest(CR_UPDATE);
	}
	else
	{
		if (num_songs != nums)
		{
			num_songs = nums;
			cur_song = starts;
			if (bControlerEnable) ControlerRequest(CR_UPDATE);
		}
		if (!bControlerEnable)
		{
			ControlerRequest(CR_SHOW);
			bControlerEnable = TRUE;
		}
	}
}

/*
	-----------------------------------------------------------------
	Disable Controler Dialog
	-----------------------------------------------------------------
*/
void DisableControler(void)
{
	if (bControlerEnable)
	{
		ControlerRequest(CR_HIDE);
		bControlerEnable = FALSE;
	}
}
