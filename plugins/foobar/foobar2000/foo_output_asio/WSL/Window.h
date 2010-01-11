
struct
StructSWindowAttr
{
	DWORD	Style;
	DWORD	ExStyle;
	HMENU	Menu;
	HICON	Icon;
	HCURSOR	Cursor;
	HBRUSH	BackBrush;
	LPCTSTR	DlgId;
	int		Id;
	int		X;
	int		Y;
	int		W;
	int		H;
};

struct
Org_Mes
{
	bool	CloseOnDestroy;
	int		ControlMode;
	HWND	HWindow;
	WNDPROC	OriginalWndProc;

	UINT	uMsg;
	WPARAM	wParam;
	LPARAM	lParam;

	bool	ExecMessage;
	LRESULT	RetProcCode;
};

class
SWindow
{
public:
	enum
	{
		ControlMode_Normal, ControlMode_Control, ControlMode_Dialog,
	};
	StructSWindowAttr	Attr;
	SWindow*	Parent;
	HWND	HParentWindow;
	HWND	HWindow;

	SWindow(SWindow* parent, char* caption = NULL, char* classname = NULL,
							int id = 0, int Control = ControlMode_Normal);
	SWindow(HWND parent, char* caption = NULL, char* classname = NULL,
							int id = 0, int Control = ControlMode_Normal);
	virtual	~SWindow(void);

	LRESULT	WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	int		GetControlMode(void);

	void	HookProc(void);
	bool	Create(void);
	int		Execute(void);
	bool	Close(void);
	bool	CloseWindow(int nResult = IDOK);
	bool	DestroyWindow(void);
	bool	EndDialog(int nResult = IDOK);
	bool	PeekMessage(LPMSG lpmsg, UINT uMsgFilterMin, UINT uMsgFilterMax,
							UINT wRemoveMsg);
	bool	SetWindowPos(HWND hwndInsertAfter, int x, int y, int cx, int cy,
							UINT fuFlags);
	bool	MoveWindow(int x, int y, int cx, int cy, bool bRepaint = true);
	void	MoveWindowCenter(SWindow* parent = NULL);
	void	MoveWindowCenter(HWND Hwnd);
	bool	GetClientRect(LPRECT lprc);
	bool	GetWindowRect(LPRECT lprc);
	LONG_PTR	SetWindowLongPtr(int nIndex, LONG_PTR lNewVal);
	LONG_PTR	GetWindowLongPtr(int nIndex);
	ULONG_PTR	SetClassLongPtr(int nIndex, LONG_PTR lNewVal);
	ULONG_PTR	GetClassLongPtr(int nIndex);
	HWND	SetCapture(void);
	bool	ClientToScreen(LPPOINT lppt);
	bool	ScreenToClient(LPPOINT lppt);
	HWND	SetFocus(void);
	LRESULT	SendMessage(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0);
	LRESULT	PostMessage(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0);
	void	SetRedraw(bool redraw);
	int		MessageBox(LPCTSTR lpszText, LPCTSTR lpszTitle, UINT fuStyle);
	HWND	GetWindow(UINT uCmd);
	UINT	SetTimer(UINT idTimer, UINT uTimeout, TIMERPROC tmprc);
	bool	KillTimer(UINT idTimer);
	bool	UpdateWindow(void);
	bool	EnableWindow(bool fEnable);
	bool	Invalidate(bool fErase);
	void	DragAcceptFiles(bool fAccept);
	bool	IsWindow(void);
	bool	IsWindowEnabled(void);
	HWND	GetDlgItem(int idControl);
	void	SetWindowText(LPCTSTR lpsz);
	bool	ShowWindow(int nCmdShow);
	bool	GetWindowPlacement(LPWINDOWPLACEMENT lpwndpl);
	bool	OpenClipboard(void);
	int		GetWindowTextLength(void);
	int		GetWindowText(LPTSTR lpsz, int cch);
	HMENU	GetSystemMenu(bool fRevert = false);
	bool	SetForegroundWindow(void);
	HWND	SetActiveWindow(void);
	HDC		GetDC(void);
	HDC		GetWindowDC(void);
	int		ReleaseDC(HDC hdc);
	HDC		BeginPaint(LPPAINTSTRUCT lpps);
	bool	EndPaint(LPPAINTSTRUCT lpps);
	HRESULT	RegisterDragDrop(IDropTarget* pDropTarget);
	HRESULT	RevokeDragDrop(void);
//	HRESULT	EnableThemeDialogTexture(DWORD dwFlags);

private:
	int		ControlMode;
	bool	CreateModeless;
	WNDPROC	OriginalWndProc;

	void	Init(char* caption, char* classname, int id, int Control);
	void	SetSubClass(void);
	static LRESULT CALLBACK	SWindowProc(HWND hwnd, UINT uMsg,
							WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK	SDialogProc(HWND hwnd, UINT uMsg,
							WPARAM wParam, LPARAM lParam);
	LRESULT	DefaultProc(Org_Mes* OrgMes);

protected:
	char*	Caption;
	char*	ClassName;
	bool	CloseOnDestroy;

	virtual void	SetupWindow(void);
	virtual LRESULT	WmUser(Org_Mes* OrgMes,
							UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void	WmCommand(Org_Mes* OrgMes,
							int wNotifyCode, int wID, HWND hwndCtl);
	virtual void	WmSysCommand(Org_Mes* OrgMes,
							UINT uCmdType, int xPos, int yPos);
	virtual bool	WmAppCommand(Org_Mes* OrgMes, LPARAM lParam);
	virtual LRESULT	WmNotify(Org_Mes* OrgMes, UINT id, LPNMHDR pnmh);
	virtual int		WmCreate(Org_Mes* OrgMes, LPCREATESTRUCT lpcs);
	virtual bool	WmInitDialog(Org_Mes* OrgMes, HWND hwnd, LONG lInitParam);
	virtual void	WmDestroy(Org_Mes* OrgMes);
	virtual bool	WmQueryEndSession(Org_Mes* OrgMes, UINT nSource);
	virtual void	WmEndSession(Org_Mes* OrgMes, bool fEndSession);
	virtual void	WmSize(Org_Mes* OrgMes, UINT sizeType, int w, int h);
	virtual void	WmMove(Org_Mes* OrgMes, int xPos, int yPos);
	virtual void	WmPaint(Org_Mes* OrgMes);
	virtual void	Paint(HDC hdc);
	virtual void	WmMouseMove(Org_Mes* OrgMes, UINT fwKeys, SHORT x, SHORT y);
	virtual void	WmLButtonDown(Org_Mes* OrgMes, UINT fwKeys, SHORT x, SHORT y);
	virtual void	WmRButtonDown(Org_Mes* OrgMes, UINT fwKeys, SHORT x, SHORT y);
	virtual void	WmLButtonUp(Org_Mes* OrgMes, UINT fwKeys, SHORT x, SHORT y);
	virtual void	WmRButtonUp(Org_Mes* OrgMes, UINT fwKeys, SHORT x, SHORT y);
	virtual void	WmLButtonDblClk(Org_Mes* OrgMes,
							UINT fwKeys, SHORT x, SHORT y);
	virtual void	WmRButtonDblClk(Org_Mes* OrgMes,
							UINT fwKeys, SHORT x, SHORT y);
	virtual void	WmDropFiles(Org_Mes* OrgMes, HDROP hDrop);
	virtual void	WmKeyDown(Org_Mes* OrgMes, int nVirtKey, UINT lKeyData);
	virtual void	WmChar(Org_Mes* OrgMes, TCHAR chCharCode, UINT lKeyData);
	virtual void	WmContextMenu(Org_Mes* OrgMes, HWND hwnd, int x, int y);
	virtual void	WmMenuSelect(Org_Mes* OrgMes,
							UINT uItem, UINT fuFlag, HMENU hMenu);
	virtual void	WmClose(Org_Mes* OrgMes);
	virtual bool	WmDrawItem(Org_Mes* OrgMes,
							int idCtl, LPDRAWITEMSTRUCT lpdis);
	virtual void	WmTimer(Org_Mes* OrgMes, UINT wTimerID, TIMERPROC* tmprc);
	virtual void	WmActivate(Org_Mes* OrgMes,
							UINT fActive, bool fMinimized, HWND hwnd);
	virtual void	WmSetCursor(Org_Mes* OrgMes, HWND hwnd,
							UINT nHittest, UINT wMouseMsg);
	virtual void	WmSetFocus(Org_Mes* OrgMes, HWND hwndLoseFocus);
	virtual void	WmKillFocus(Org_Mes* OrgMes, HWND hwndGetFocus);
	virtual void	WmSysColorChange(Org_Mes* OrgMes);
/*
	virtual HBRUSH	WmCtlColorDlg(Org_Mes* OrgMes, HWND hwndDlg, HDC hdcDlg);
	virtual HBRUSH	WmCtlColorStatic(Org_Mes* OrgMes, HWND hwndStatic, HDC hdcStatic);
	virtual HBRUSH	WmCtlColorEdit(Org_Mes* OrgMes, HWND hwndEdit, HDC hdcEdit);
	virtual HBRUSH	WmCtlColorListBox(Org_Mes* OrgMes, HWND hwndLB, HDC hdcLB);
	virtual	bool	WmEraseBkGnd(Org_Mes* OrgMes, HDC hdc);
*/
};

