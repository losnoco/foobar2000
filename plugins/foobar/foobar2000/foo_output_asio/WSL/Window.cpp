
#define	STRICT

#define _WIN32_WINNT 0x500

#include <windows.h>
//#include <uxtheme.h>

#include "WSL.h"
#include "Window.h"

HINSTANCE	WSLhInstance;

SWindow::SWindow(SWindow* parent, char* caption, char* classname, int id, int Control)
{
	Parent = parent;
	HParentWindow = NULL;

	Init(caption, classname, id, Control);
}

SWindow::SWindow(HWND parent, char* caption, char* classname, int id, int Control)
{
	Parent = NULL;
	HParentWindow = parent;

	Init(caption, classname, id, Control);
}

SWindow::~SWindow(void)
{
	if(IsWindow()) {
		SetWindowLongPtr(GWLP_USERDATA, NULL);
		if(OriginalWndProc) {
			SetWindowLongPtr(GWLP_WNDPROC,
									reinterpret_cast<LONG_PTR>(OriginalWndProc));
		}
	} else {
		if(ControlMode == ControlMode_Normal) {
			::UnregisterClass(ClassName, WSLhInstance);
		}
	}
}

void
SWindow::Init(char* caption, char* classname, int id, int Control)
{
	Caption = caption;

	if(classname) {
		ClassName = classname;
	} else {
		ClassName = "SWindow Class";
	}

	ControlMode = Control;

	Attr.Id = id;
	Attr.Style = WS_VISIBLE;
	Attr.ExStyle = 0;
	Attr.Menu = NULL;
	Attr.Icon = ::LoadIcon(NULL, IDI_APPLICATION);
	Attr.Cursor = ::LoadCursor(NULL, IDC_ARROW);
	Attr.BackBrush = static_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH));

	Attr.X = CW_USEDEFAULT;
	Attr.Y = CW_USEDEFAULT;
	Attr.W = CW_USEDEFAULT;
	Attr.H = CW_USEDEFAULT;

	HWindow = NULL;
	OriginalWndProc = NULL;

	if(Parent && (ControlMode == ControlMode_Control)) {
		if(Parent->GetControlMode() == ControlMode_Dialog) {
			HWindow = Parent->GetDlgItem(Attr.Id);
		}
	}

	CloseOnDestroy = false;
}

int
SWindow::GetControlMode(void)
{
	return ControlMode;
}

void
SWindow::HookProc(void)
{
	SetSubClass();
}

bool
SWindow::Create(void)
{
	CreateModeless = true;

	if(ControlMode == ControlMode_Normal) {
		WNDCLASSEX	wndclass;

		wndclass.cbSize        = sizeof wndclass;
		wndclass.style         = CS_DBLCLKS;
		wndclass.lpfnWndProc   = SWindowProc;
		wndclass.cbClsExtra    = 0;
		wndclass.cbWndExtra    = 0;
		wndclass.hInstance     = WSLhInstance;
		wndclass.hIcon         = Attr.Icon;
		wndclass.hCursor       = Attr.Cursor;
		wndclass.hbrBackground = Attr.BackBrush;
		wndclass.lpszMenuName  = NULL;
		wndclass.lpszClassName = ClassName;
		wndclass.hIconSm       = NULL;

		::RegisterClassEx(&wndclass);
	}

	switch(ControlMode) {
	case ControlMode_Normal:
	case ControlMode_Control:
		HWindow = ::CreateWindowEx(
					Attr.ExStyle,
					ClassName,
					Caption,
					Attr.Style,
					Attr.X,
					Attr.Y,
					Attr.W,
					Attr.H,
					Parent ? Parent->HWindow : HParentWindow,
					Attr.Menu ? Attr.Menu : reinterpret_cast<HMENU>(Attr.Id),
					WSLhInstance,
					(ControlMode == ControlMode_Normal) ? this : NULL);

		if(ControlMode == ControlMode_Control) SetSubClass();
		break;
	case ControlMode_Dialog:
		HWindow = ::CreateDialogParam(
					WSLhInstance,
					Attr.DlgId,
					Parent ? Parent->HWindow : HParentWindow,
					SDialogProc,
					reinterpret_cast<LPARAM>(this));
		break;
	}

	UpdateWindow();

	return true;
}

int
SWindow::Execute(void)
{
	CreateModeless = false;

	switch(ControlMode) {
	case ControlMode_Dialog:
		return ::DialogBoxParam(
					WSLhInstance,
					Attr.DlgId,
					Parent ? Parent->HWindow : HParentWindow,
					SDialogProc,
					reinterpret_cast<LPARAM>(this));
	}

	return 0;
}

void
SWindow::SetSubClass(void)
{
	OriginalWndProc = reinterpret_cast<WNDPROC>(GetWindowLongPtr(GWLP_WNDPROC));

	SetWindowLongPtr(GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
	SetWindowLongPtr(GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(SWindowProc));

	SetupWindow();
}

LRESULT CALLBACK
SWindow::SWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SWindow*	SWindowClass;

	if(uMsg == WM_CREATE) {
		SWindowClass = reinterpret_cast<SWindow*>
							(reinterpret_cast<LPCREATESTRUCT>
									(lParam)->lpCreateParams);
		::SetWindowLongPtr(hwnd, GWLP_USERDATA,
										reinterpret_cast<LONG_PTR>(SWindowClass));
	} else {
		SWindowClass = reinterpret_cast<SWindow*>
							(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
	}

	LRESULT	RetCode;

	if(SWindowClass) {
		RetCode = SWindowClass->WndProc(hwnd, uMsg, wParam, lParam);
	} else {
		RetCode = ::DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	return RetCode;
}

INT_PTR CALLBACK
SWindow::SDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SWindow*	SWindowClass;

	if(uMsg == WM_INITDIALOG) {
		SWindowClass = reinterpret_cast<SWindow*>(lParam);
		::SetWindowLongPtr(hwnd, GWLP_USERDATA,
										reinterpret_cast<LONG_PTR>(SWindowClass));
	} else {
		SWindowClass = reinterpret_cast<SWindow*>
							(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
	}

	INT_PTR	RetCode;

	if(SWindowClass) {
		RetCode = SWindowClass->WndProc(hwnd, uMsg, wParam, lParam);
	} else {
		RetCode = false;
	}

	return RetCode;
}

LRESULT
SWindow::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(HWindow == NULL) HWindow = hwnd;

	Org_Mes	OrgMes;

	OrgMes.CloseOnDestroy  = CloseOnDestroy;
	OrgMes.ControlMode     = ControlMode;
	OrgMes.HWindow         = hwnd;
	OrgMes.OriginalWndProc = OriginalWndProc;

	OrgMes.uMsg   = uMsg;
	OrgMes.wParam = wParam;
	OrgMes.lParam = lParam;

	OrgMes.ExecMessage = false;
	OrgMes.RetProcCode = 0;

	bool	RetMessage = false;
	LRESULT	RetCode;

	switch(uMsg) {
	case WM_COMMAND:
		WmCommand(&OrgMes, HIWORD(wParam), LOWORD(wParam),
							reinterpret_cast<HWND>(lParam));
		break;
	case WM_SYSCOMMAND:
		WmSysCommand(&OrgMes, wParam, LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_APPCOMMAND:
		RetMessage = true;
		RetCode = WmAppCommand(&OrgMes, lParam);
		break;
	case WM_NOTIFY:
		RetMessage = true;
		RetCode = WmNotify(&OrgMes, wParam, reinterpret_cast<LPNMHDR>(lParam));
		break;
	case WM_SIZE:
		WmSize(&OrgMes, wParam, LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_MOVE:
		WmMove(&OrgMes, LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_PAINT:
		WmPaint(&OrgMes);
		break;
	case WM_MOUSEMOVE:
		WmMouseMove(&OrgMes, wParam, LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_LBUTTONDOWN:
		WmLButtonDown(&OrgMes, wParam, LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_RBUTTONDOWN:
		WmRButtonDown(&OrgMes, wParam, LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_LBUTTONUP:
		WmLButtonUp(&OrgMes, wParam, LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_RBUTTONUP:
		WmRButtonUp(&OrgMes, wParam, LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_LBUTTONDBLCLK:
		WmLButtonDblClk(&OrgMes, wParam, LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_RBUTTONDBLCLK:
		WmRButtonDblClk(&OrgMes, wParam, LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_DROPFILES:
		WmDropFiles(&OrgMes, reinterpret_cast<HDROP>(wParam));
		break;
	case WM_KEYDOWN:
		WmKeyDown(&OrgMes, wParam, lParam);
		break;
	case WM_CHAR:
		WmChar(&OrgMes, wParam, lParam);
		break;
	case WM_CONTEXTMENU:
		WmContextMenu(&OrgMes, reinterpret_cast<HWND>(wParam),
							LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_MENUSELECT:
		WmMenuSelect(&OrgMes, LOWORD(wParam), HIWORD(wParam),
							reinterpret_cast<HMENU>(lParam));
		break;
	case WM_DRAWITEM:
		RetMessage = true;
		RetCode = WmDrawItem(&OrgMes, wParam,
							reinterpret_cast<LPDRAWITEMSTRUCT>(lParam));
		break;
	case WM_TIMER:
		WmTimer(&OrgMes, wParam, reinterpret_cast<TIMERPROC*>(lParam));
		break;
	case WM_ACTIVATE:
		WmActivate(&OrgMes, LOWORD(wParam), HIWORD(wParam) != 0,
							reinterpret_cast<HWND>(lParam));
		break;
	case WM_SETCURSOR:
		WmSetCursor(&OrgMes, reinterpret_cast<HWND>(wParam),
							LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_SETFOCUS:
		WmSetFocus(&OrgMes, reinterpret_cast<HWND>(wParam));
		break;
	case WM_KILLFOCUS:
		WmKillFocus(&OrgMes, reinterpret_cast<HWND>(wParam));
		break;
/*
	case WM_CTLCOLORDLG:
		RetMessage = true;
		RetCode = reinterpret_cast<LRESULT>
							(WmCtlColorDlg(&OrgMes, reinterpret_cast<HWND>(lParam),
							reinterpret_cast<HDC>(wParam)));
		break;
	case WM_CTLCOLORSTATIC:
		RetMessage = true;
		RetCode = reinterpret_cast<LRESULT>
							(WmCtlColorStatic(&OrgMes, reinterpret_cast<HWND>(lParam),
							reinterpret_cast<HDC>(wParam)));
		break;
	case WM_CTLCOLOREDIT:
		RetMessage = true;
		RetCode = reinterpret_cast<LRESULT>
							(WmCtlColorEdit(&OrgMes, reinterpret_cast<HWND>(lParam),
							reinterpret_cast<HDC>(wParam)));
		break;
	case WM_CTLCOLORLISTBOX:
		RetMessage = true;
		RetCode = reinterpret_cast<LRESULT>
							(WmCtlColorListBox(&OrgMes, reinterpret_cast<HWND>(lParam),
							reinterpret_cast<HDC>(wParam)));
		break;
	case WM_ERASEBKGND:
		RetMessage = true;
		RetCode = WmEraseBkGnd(&OrgMes, reinterpret_cast<HDC>(wParam));
		break;
*/
	case WM_CREATE:
		RetMessage = true;
		RetCode = WmCreate(&OrgMes, reinterpret_cast<LPCREATESTRUCT>(lParam));
		SetupWindow();
		break;
	case WM_INITDIALOG:
		RetMessage = true;
		RetCode = WmInitDialog(&OrgMes, reinterpret_cast<HWND>(wParam), lParam);
		break;
	case WM_CLOSE:
		WmClose(&OrgMes);
		break;
	case WM_DESTROY:
		WmDestroy(&OrgMes);
		if(OrgMes.CloseOnDestroy) {
			SetWindowLongPtr(GWLP_USERDATA, 0);
			delete this;
		}
		break;
	case WM_SYSCOLORCHANGE:
		WmSysColorChange(&OrgMes);
		break;
	case WM_QUERYENDSESSION:
		RetMessage = true;
		RetCode = WmQueryEndSession(&OrgMes, wParam);
		break;
	case WM_ENDSESSION:
		WmEndSession(&OrgMes, wParam != 0);
		if(OrgMes.CloseOnDestroy && wParam) {
			SetWindowLongPtr(GWLP_USERDATA, 0);
			delete this;
		}
		break;
	default:
		if((uMsg >= WM_USER) && (uMsg <= 0x7fff)) {
			RetMessage = true;
			RetCode = WmUser(&OrgMes, uMsg, wParam, lParam);
		} else {
			DefaultProc(&OrgMes);
		}
		break;
	}

	if(RetMessage == false) {
		if(OrgMes.OriginalWndProc || (OrgMes.ControlMode == ControlMode_Normal)) {
			RetCode = OrgMes.ExecMessage ? 0 : OrgMes.RetProcCode;
		} else {
			RetCode = OrgMes.ExecMessage;
		}
	}

	return RetCode;
}

LRESULT
SWindow::DefaultProc(Org_Mes* OrgMes)
{
	if((OrgMes->ControlMode != ControlMode_Dialog) && (OrgMes->ExecMessage == false)) {
		if(OrgMes->OriginalWndProc) {
			OrgMes->RetProcCode = ::CallWindowProc(
								OrgMes->OriginalWndProc,
								OrgMes->HWindow,
								OrgMes->uMsg, OrgMes->wParam, OrgMes->lParam);
		} else {
			OrgMes->RetProcCode = ::DefWindowProc(
								OrgMes->HWindow,
								OrgMes->uMsg, OrgMes->wParam, OrgMes->lParam);
		}
	}

	return OrgMes->RetProcCode;
}

bool
SWindow::Close(void)
{
	return PostMessage(WM_CLOSE) != 0;
}

bool
SWindow::CloseWindow(int nResult)
{
	bool	RetCode = true;

	if(CreateModeless) {
		DestroyWindow();
	} else {
		if(ControlMode == ControlMode_Dialog) {
			RetCode = EndDialog(nResult);
		} else {
			DestroyWindow();
		}
	}

	return RetCode;
}

bool
SWindow::DestroyWindow(void)
{
	return ::DestroyWindow(HWindow) != 0;
}

bool
SWindow::EndDialog(int nResult)
{
	return ::EndDialog(HWindow, nResult) != 0;
}

bool
SWindow::PeekMessage(LPMSG lpmsg, UINT uMsgFilterMin, UINT uMsgFilterMax,
																UINT wRemoveMsg)
{
	return ::PeekMessage(lpmsg, HWindow, uMsgFilterMin, uMsgFilterMax,
															wRemoveMsg) != 0;
}

bool
SWindow::SetWindowPos(HWND hwndInsertAfter, int x, int y, int cx, int cy,
																UINT fuFlags)
{
	return ::SetWindowPos(HWindow, hwndInsertAfter, x, y, cx, cy, fuFlags) != 0;
}

bool
SWindow::MoveWindow(int x, int y, int cx, int cy, bool bRepaint)
{
	return ::MoveWindow(HWindow, x, y, cx, cy, bRepaint) != 0;
}

void
SWindow::MoveWindowCenter(SWindow* parent)
{
	MoveWindowCenter(
			parent ? parent->HWindow : (Parent ? Parent->HWindow : HParentWindow));
}

void
SWindow::MoveWindowCenter(HWND Hwnd)
{
	RECT	Rect;
	RECT	ParentRect;

	GetWindowRect(&Rect);
	if(Hwnd) {
		::GetWindowRect(Hwnd, &ParentRect);
	} else {
		::GetClientRect(::GetDesktopWindow(), &ParentRect);
	}

	int		ParentRectW = ParentRect.right - ParentRect.left;
	int		ParentRectH = ParentRect.bottom - ParentRect.top;
	int		RectW = Rect.right - Rect.left;
	int		RectH = Rect.bottom - Rect.top;

	MoveWindow(ParentRect.left + (ParentRectW - RectW) / 2,
						ParentRect.top + (ParentRectH - RectH) / 2, RectW, RectH);
}

bool
SWindow::GetClientRect(LPRECT lprc)
{
	return ::GetClientRect(HWindow, lprc) != 0;
}

bool
SWindow::GetWindowRect(LPRECT lprc)
{
	return ::GetWindowRect(HWindow, lprc) != 0;
}

LONG_PTR
SWindow::SetWindowLongPtr(int nIndex, LONG_PTR lNewVal)
{
	return ::SetWindowLongPtr(HWindow, nIndex, lNewVal);
}

LONG_PTR
SWindow::GetWindowLongPtr(int nIndex)
{
	return ::GetWindowLongPtr(HWindow, nIndex);
}

ULONG_PTR
SWindow::SetClassLongPtr(int nIndex, LONG_PTR lNewVal)
{
	return ::SetClassLongPtr(HWindow, nIndex, lNewVal);
}

ULONG_PTR
SWindow::GetClassLongPtr(int nIndex)
{
	return ::GetClassLongPtr(HWindow, nIndex);
}

HWND
SWindow::SetCapture(void)
{
	return ::SetCapture(HWindow);
}

bool
SWindow::ClientToScreen(LPPOINT lppt)
{
	return ::ClientToScreen(HWindow, lppt) != 0;
}

bool
SWindow::ScreenToClient(LPPOINT lppt)
{
	return ::ScreenToClient(HWindow, lppt) != 0;
}

HWND
SWindow::SetFocus(void)
{
	return ::SetFocus(HWindow);
}

HMENU
SWindow::GetSystemMenu(bool fRevert)
{
	return ::GetSystemMenu(HWindow, fRevert);
}

bool
SWindow::SetForegroundWindow(void)
{
	return ::SetForegroundWindow(HWindow) != 0;
}

HWND
SWindow::SetActiveWindow(void)
{
	return ::SetActiveWindow(HWindow);
}

LRESULT
SWindow::SendMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return ::SendMessage(HWindow, uMsg, wParam, lParam);
}

LRESULT
SWindow::PostMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return ::PostMessage(HWindow, uMsg, wParam, lParam);
}

void
SWindow::SetRedraw(bool redraw)
{
	SendMessage(WM_SETREDRAW, redraw);
}

int
SWindow::MessageBox(LPCTSTR lpszText, LPCTSTR lpszTitle, UINT fuStyle)
{
	return ::MessageBox(HWindow, lpszText, lpszTitle, fuStyle);
}

HWND
SWindow::GetWindow(UINT uCmd)
{
	return ::GetWindow(HWindow, uCmd);
}

UINT
SWindow::SetTimer(UINT idTimer, UINT uTimeout, TIMERPROC tmprc)
{
	return ::SetTimer(HWindow, idTimer, uTimeout, tmprc);
}

bool
SWindow::KillTimer(UINT idTimer)
{
	return ::KillTimer(HWindow, idTimer) != 0;
}

bool
SWindow::UpdateWindow(void)
{
	return ::UpdateWindow(HWindow) != 0;
}

bool
SWindow::EnableWindow(bool fEnable)
{
	return ::EnableWindow(HWindow, fEnable) != 0;
}

bool
SWindow::Invalidate(bool fErase)
{
	return ::InvalidateRect(HWindow, NULL, fErase) != 0;
}

void
SWindow::DragAcceptFiles(bool fAccept)
{
	::DragAcceptFiles(HWindow, fAccept);
}

bool
SWindow::IsWindow(void)
{
	return ::IsWindow(HWindow) != 0;
}

bool
SWindow::IsWindowEnabled(void)
{
	return ::IsWindowEnabled(HWindow) != 0;
}

HWND
SWindow::GetDlgItem(int idControl)
{
	return ::GetDlgItem(HWindow, idControl);
}

void
SWindow::SetWindowText(LPCTSTR lpsz)
{
	::SetWindowText(HWindow, lpsz);
}

bool
SWindow::ShowWindow(int nCmdShow)
{
	return ::ShowWindow(HWindow, nCmdShow) != 0;
}

bool
SWindow::GetWindowPlacement(LPWINDOWPLACEMENT lpwndpl)
{
	return ::GetWindowPlacement(HWindow, lpwndpl) != 0;
}

bool
SWindow::OpenClipboard(void)
{
	return ::OpenClipboard(HWindow) != 0;
}

int
SWindow::GetWindowTextLength(void)
{
	return ::GetWindowTextLength(HWindow);
}

int
SWindow::GetWindowText(LPTSTR lpsz, int cch)
{
	return ::GetWindowText(HWindow, lpsz, cch);
}

HDC
SWindow::GetDC(void)
{
	return ::GetDC(HWindow);
}

HDC
SWindow::GetWindowDC(void)
{
	return ::GetWindowDC(HWindow);
}

int
SWindow::ReleaseDC(HDC hdc)
{
	return ::ReleaseDC(HWindow, hdc);
}

HDC
SWindow::BeginPaint(LPPAINTSTRUCT lpps)
{
	return ::BeginPaint(HWindow, lpps);
}

bool
SWindow::EndPaint(LPPAINTSTRUCT lpps)
{
	return ::EndPaint(HWindow, lpps) != 0;
}

HRESULT
SWindow::RegisterDragDrop(IDropTarget* pDropTarget)
{
	return ::RegisterDragDrop(HWindow, pDropTarget);
}

HRESULT
SWindow::RevokeDragDrop(void)
{
	return ::RevokeDragDrop(HWindow);
}

/*
HRESULT
SWindow::EnableThemeDialogTexture(DWORD dwFlags)
{
	return ::EnableThemeDialogTexture(HWindow, dwFlags);
}
*/

void
SWindow::SetupWindow(void)
{
}

LRESULT
SWindow::WmUser(Org_Mes* OrgMes,
							UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	return DefaultProc(OrgMes);
}

void
SWindow::WmCommand(Org_Mes* OrgMes, int /*wNotifyCode*/, int /*wID*/,
															HWND /*hwndCtl*/)
{
	DefaultProc(OrgMes);
}

void
SWindow::WmSysCommand(Org_Mes* OrgMes, UINT /*uCmdType*/,
													int /*xPos*/, int /*yPos*/)
{
	DefaultProc(OrgMes);
}

bool
SWindow::WmAppCommand(Org_Mes* OrgMes, LPARAM /*lParam*/)
{
	return DefaultProc(OrgMes) != 0;
}

LRESULT
SWindow::WmNotify(Org_Mes* OrgMes, UINT /*id*/, LPNMHDR /*pnmh*/)
{
	return DefaultProc(OrgMes);
}

int
SWindow::WmCreate(Org_Mes* OrgMes, LPCREATESTRUCT /*lpcs*/)
{
	return DefaultProc(OrgMes);
}

bool
SWindow::WmInitDialog(Org_Mes* OrgMes, HWND /*hwnd*/, LONG /*lInitParam*/)
{
	return DefaultProc(OrgMes) != 0;
}

void
SWindow::WmDestroy(Org_Mes* OrgMes)
{
	DefaultProc(OrgMes);
}

bool
SWindow::WmQueryEndSession(Org_Mes* OrgMes, UINT /*nSource*/)
{
	return DefaultProc(OrgMes) != 0;
}

void
SWindow::WmEndSession(Org_Mes* OrgMes, bool /*fEndSession*/)
{
	DefaultProc(OrgMes);
}

void
SWindow::WmSize(Org_Mes* OrgMes, UINT /*sizeType*/, int w, int h)
{
	DefaultProc(OrgMes);

	Attr.W = w;
	Attr.H = h;
}

void
SWindow::WmMove(Org_Mes* OrgMes, int xPos, int yPos)
{
	DefaultProc(OrgMes);

	Attr.X = xPos;
	Attr.Y = yPos;
}

void
SWindow::WmPaint(Org_Mes* OrgMes)
{
	if(OrgMes->ExecMessage/* && (ControlMode == ControlMode_Normal)*/) {
		PAINTSTRUCT	ps;
		HDC		hdc = BeginPaint(&ps);

		Paint(hdc);

		EndPaint(&ps);
	}

	DefaultProc(OrgMes);
}

void
SWindow::Paint(HDC /*hdc*/)
{
}

void
SWindow::WmMouseMove(Org_Mes* OrgMes, UINT /*fwKeys*/, SHORT /*x*/, SHORT /*y*/)
{
	DefaultProc(OrgMes);
}

void
SWindow::WmLButtonDown(Org_Mes* OrgMes, UINT /*fwKeys*/, SHORT /*x*/, SHORT /*y*/)
{
	DefaultProc(OrgMes);
}

void
SWindow::WmRButtonDown(Org_Mes* OrgMes, UINT /*fwKeys*/, SHORT /*x*/, SHORT /*y*/)
{
	DefaultProc(OrgMes);
}

void
SWindow::WmLButtonUp(Org_Mes* OrgMes, UINT /*fwKeys*/, SHORT /*x*/, SHORT /*y*/)
{
	DefaultProc(OrgMes);
}

void
SWindow::WmRButtonUp(Org_Mes* OrgMes, UINT /*fwKeys*/, SHORT /*x*/, SHORT /*y*/)
{
	DefaultProc(OrgMes);
}

void
SWindow::WmLButtonDblClk(Org_Mes* OrgMes, UINT /*fwKeys*/,
														SHORT /*x*/, SHORT /*y*/)
{
	DefaultProc(OrgMes);
}

void
SWindow::WmRButtonDblClk(Org_Mes* OrgMes, UINT /*fwKeys*/,
														SHORT /*x*/, SHORT /*y*/)
{
	DefaultProc(OrgMes);
}

void
SWindow::WmDropFiles(Org_Mes* OrgMes, HDROP /*hDrop*/)
{
	DefaultProc(OrgMes);
}

void
SWindow::WmKeyDown(Org_Mes* OrgMes, int /*nVirtKey*/, UINT /*lKeyData*/)
{
	DefaultProc(OrgMes);
}

void
SWindow::WmChar(Org_Mes* OrgMes, TCHAR /*chCharCode*/, UINT /*lKeyData*/)
{
	DefaultProc(OrgMes);
}

void
SWindow::WmContextMenu(Org_Mes* OrgMes, HWND /*hwnd*/, int /*x*/, int /*y*/)
{
	DefaultProc(OrgMes);
}

void
SWindow::WmMenuSelect(Org_Mes* OrgMes, UINT /*uItem*/, UINT /*fuFlag*/,
															HMENU /*hMenu*/)
{
	DefaultProc(OrgMes);
}

void
SWindow::WmClose(Org_Mes* OrgMes)
{
	DefaultProc(OrgMes);
}

bool
SWindow::WmDrawItem(Org_Mes* OrgMes, int /*idCtl*/, LPDRAWITEMSTRUCT /*lpdis*/)
{
	return DefaultProc(OrgMes) != 0;
}

void
SWindow::WmTimer(Org_Mes* OrgMes, UINT /*wTimerID*/, TIMERPROC* /*tmprc*/)
{
	DefaultProc(OrgMes);
}

void
SWindow::WmActivate(Org_Mes* OrgMes, UINT /*fActive*/, bool /*fMinimized*/,
																HWND /*hwnd*/)
{
	DefaultProc(OrgMes);
}

void
SWindow::WmSetCursor(Org_Mes* OrgMes, HWND /*hwnd*/,
											UINT /*nHittest*/, UINT /*wMouseMsg*/)
{
	DefaultProc(OrgMes);
}


void
SWindow::WmSetFocus(Org_Mes* OrgMes, HWND /*hwndLoseFocus*/)
{
	DefaultProc(OrgMes);
}

void
SWindow::WmKillFocus(Org_Mes* OrgMes, HWND /*hwndGetFocus*/)
{
	DefaultProc(OrgMes);
}

void
SWindow::WmSysColorChange(Org_Mes* OrgMes)
{
	DefaultProc(OrgMes);
}

/*

HBRUSH
SWindow::WmCtlColorDlg(Org_Mes* OrgMes, HWND hwndDlg HDC hdcDlg)
{
	return reinterpret_cast<HBRUSH>(DefaultProc(OrgMes));
}

HBRUSH
SWindow::WmCtlColorStatic(Org_Mes* OrgMes, HWND hwndStatic, HDC hdcStatic)
{
	return reinterpret_cast<HBRUSH>(DefaultProc(OrgMes));
}

HBRUSH
SWindow::WmCtlColorEdit(Org_Mes* OrgMes, HWND hwndEdit, HDC hdcEdit)
{
	return reinterpret_cast<HBRUSH>(DefaultProc(OrgMes));
}

HBRUSH
SWindow::WmCtlColorListBox(Org_Mes* OrgMes, HWND hwndLB, HDC hdcLB)
{
	return reinterpret_cast<HBRUSH>(DefaultProc(OrgMes));
}

bool
SWindow::WmEraseBkGnd(Org_Mes* OrgMes, HDC hdc)
{
	return DefaultProc(OrgMes) != 0;
}
*/

