#ifdef _WIN32
#include "common/win32/win32l.h"
#undef NOUSER
#undef NOKEYSTATES
#undef NOSYSMETRICS
#undef NOWINMESSAGES
#undef NOGDI
#undef NOCOLOR
#undef NODRAWTEXT
#undef NORASTEROPS
#undef NOTEXTMETRIC
#undef NOWINOFFSETS
#include <windows.h>
#include "common/win32/rc/nezplug.rh"
#ifdef _WIN32
#define MAX_PLAYERTB 5
typedef struct {
	RECT rEdge;
	DWORD tx1;
	DWORD ty1;
	DWORD tx2;
	DWORD ty2;
} PLAYERTB;
typedef struct PlatformInstance_TAG
{
	HWND		hwnd;
	WNDPROC		fnOldWndProc;
	PLAYERTB	ptb[MAX_PLAYERTB];
	RECT		rtv;
	unsigned pt;
	unsigned ps;
	DWORD cx;
	DWORD cy;
	DWORD ex;
	DWORD ey;
} PlatformInstance;
#elif !defined(_MAC)
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
typedef struct PlatformInstance_TAG
{
	Window	window;
	Display	*display;
	uint32	x, y;
	uint32	width, height;
} PlatformInstance;
#define PLUGIN_NAME			"NEZplug"
#define PLUGIN_DESCRIPTION	"NSF player plug-in."
#define PLUGIN_MIME 		"application/x-nsf:nsf:"PLUGIN_NAME
#error "Platform-X11 not supported"
#else
typedef struct PlatformInstance_TAG
{
	NPWindow *window;
} PlatformInstance;
#error "Platform-MAC not supported"
#endif

#include <stdio.h>
#include <string.h>
#include <npapi.h>
#include <npupp.h>

#include "common/nsfsdk/nsfsdk.h"
#include "common/zlib/nez.h"
#include "snddrv/snddrv.h"

typedef struct
{
	void *buf;
	unsigned offset;
	unsigned max;
} EBUFFER;
typedef struct
{
	char *buf;
} EMIME;
typedef struct
{
	PlatformInstance fPlatform;
	uint16 fMode;
	NPP instance;
	EBUFFER ebuf;
	EMIME mime;
	unsigned songno;
	SOUNDDEVICEPDI sndpdi;
	HNSF hnsf;
	SOUNDDEVICE *psd;
} PluginInstance;

static void NPNEZWriteProc(void *lpargs, void *lpbuf, unsigned len)
{
	PluginInstance *pdata = lpargs;
	NSFSDK_Render(pdata->hnsf, lpbuf, len >> 2);
}
static void NPNEZTermProc(void *lpargs)
{
	PluginInstance *pdata = lpargs;
}
static void NPNEZInit(PluginInstance *pdata)
{
	pdata->songno = 0;
	pdata->psd = NULL;
}
static void NPNEZStop(PluginInstance *pdata)
{
	SOUNDDEVICE *psd = pdata->psd;
	if (psd)
	{
		pdata->psd = NULL;
		psd->Term(psd);
	}
}
static void NPNEZPause(PluginInstance *pdata)
{
	if (pdata->psd)
	{
		pdata->psd->Pause(pdata->psd, pdata->psd->IsPause(pdata->psd) ^ 1);
	}
}
static void NPNEZTerm(PluginInstance *pdata)
{
	NPNEZStop(pdata);
	if (pdata->hnsf)
	{
		NSFSDK_Terminate(pdata->hnsf);
		pdata->hnsf = 0;
	}
}

static void NPNEZLoad(PluginInstance *pdata)
{
	unsigned nezs;
	void *nezp;
	NPNEZTerm(pdata);
	nezs = NEZ_extractMem(pdata->ebuf.buf, pdata->ebuf.offset, &nezp);
	if (nezs)
	{
		pdata->hnsf = NSFSDK_Load(nezp, nezs);
		free(nezp);
	}
	else
	{
		pdata->hnsf = NSFSDK_Load(pdata->ebuf.buf, pdata->ebuf.offset);
	}
}
static void NPNEZPlay(PluginInstance *pdata)
{
	SOUNDDEVICEINITDATA sdid;
	unsigned songno;
	NPNEZStop(pdata);
	if (!pdata->hnsf) return;
	sdid.freq = 44100;
	sdid.ch = 2;
	sdid.bit = 16;
	sdid.lpargs = pdata;
	sdid.Write = NPNEZWriteProc;
	sdid.Term = NPNEZTermProc;
	sdid.ppdi = &pdata->sndpdi;
	if (pdata->songno) NSFSDK_SetSongNo(pdata->hnsf, pdata->songno);
	NSFSDK_SetFrequency(pdata->hnsf, sdid.freq);
	NSFSDK_SetChannel(pdata->hnsf, sdid.ch);
	NSFSDK_Reset(pdata->hnsf);
	pdata->psd = CreateSoundDevice(&sdid);
	songno = NSFSDK_GetSongNo(pdata->hnsf);
	if (songno) pdata->songno = songno;
	return;
}
static void NPNEZNext(PluginInstance *pdata, unsigned fTen)
{
	pdata->songno += fTen;
	if (pdata->songno > NSFSDK_GetSongMax(pdata->hnsf))
		pdata->songno = NSFSDK_GetSongMax(pdata->hnsf);
	if (pdata->psd) NPNEZPlay(pdata);
}
static void NPNEZPrev(PluginInstance *pdata, unsigned fTen)
{
	if (pdata->songno > fTen)
		pdata->songno -= fTen;
	else
		pdata->songno = 1;
	if (pdata->psd) NPNEZPlay(pdata);
}


const char *gPropNameInstance = "nezplug-npnez.dll-0.92-instance->pdata";
static LRESULT CALLBACK PluginWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PluginInstance *pdata = (PluginInstance *)GetProp(hwnd, gPropNameInstance);
	switch (uMsg)
	{
		case WM_LBUTTONDOWN:
		{
			if (pdata->fPlatform.ps != MAX_PLAYERTB) break;
			if (pdata->fPlatform.pt == MAX_PLAYERTB) break;
			pdata->fPlatform.ps = pdata->fPlatform.pt;
			InvalidateRect(hwnd, &pdata->fPlatform.ptb[pdata->fPlatform.ps].rEdge, FALSE);
			break;
		}
		case WM_LBUTTONUP:
		{
			if (pdata->fPlatform.ps == MAX_PLAYERTB) break;
			InvalidateRect(hwnd, &pdata->fPlatform.ptb[pdata->fPlatform.ps].rEdge, FALSE);
			pdata->fPlatform.ps = MAX_PLAYERTB;
			if (pdata->fPlatform.pt == MAX_PLAYERTB)
			{
				ReleaseCapture();
			}
			else
			{
				switch (pdata->fPlatform.pt)
				{
					case 0:
						NPNEZPlay(pdata);
						break;
					case 1:
						NPNEZPause(pdata);
						break;
					case 2:
						NPNEZStop(pdata);
						break;
					case 3:
						NPNEZPrev(pdata, (wParam & MK_SHIFT) ? 10 : 1);
						InvalidateRect(hwnd, &pdata->fPlatform.rtv, FALSE);
						break;
					case 4:
						NPNEZNext(pdata, (wParam & MK_SHIFT) ? 10 : 1);
						InvalidateRect(hwnd, &pdata->fPlatform.rtv, FALSE);
						break;
				}
			}
			break;
		}
		case WM_MOUSEMOVE:
		{
			int np, op;
			POINT pt;
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);
			op = pdata->fPlatform.pt;
			for (np = 0; np < MAX_PLAYERTB; np++)
			{
				if (PtInRect(&pdata->fPlatform.ptb[np].rEdge, pt))
					break;
			}
			if (op != np)
			{
				pdata->fPlatform.pt = np;
				if (op != MAX_PLAYERTB)
				{
					InvalidateRect(hwnd, &pdata->fPlatform.ptb[op].rEdge, FALSE);
				}
				else
				{
					SetCapture(hwnd);
				}
				if (np != MAX_PLAYERTB)
				{
					InvalidateRect(hwnd, &pdata->fPlatform.ptb[np].rEdge, FALSE);
				}
				else
				{
					if (pdata->fPlatform.ps == MAX_PLAYERTB)
						ReleaseCapture();
				}
			}
			break;
		}
		case WM_PAINT:
		{
			unsigned i;
			CHAR buf[16];
			PAINTSTRUCT paintStruct;
			HBRUSH hBrushBlack = CreateSolidBrush(RGB(0,0,0));
			HBITMAP hBitmap = LoadBitmap(GetDLLInstance(), MAKEINTRESOURCE(IDB_PLAYERTB));
			HDC hdc = BeginPaint(hwnd, &paintStruct);
			HDC hdcMem = CreateCompatibleDC(hdc);
			HFONT hfont = CreateFont(pdata->fPlatform.cy - 4, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH, NULL);
			HFONT hfontOld = SelectObject(hdc, hfont);
			SelectObject(hdcMem, hBitmap);
			for (i = 0; i < MAX_PLAYERTB; i++)
			{
				FillRect(hdc, &pdata->fPlatform.ptb[i].rEdge, GetSysColorBrush(COLOR_MENU));
				if (pdata->fPlatform.ps != MAX_PLAYERTB)
				{
					if (pdata->fPlatform.ps != i)
					{
						DrawEdge(hdc, &pdata->fPlatform.ptb[i].rEdge, 0, BF_FLAT);
						BitBlt(hdc, pdata->fPlatform.ptb[i].tx1, pdata->fPlatform.ptb[i].ty1, 11, 11, hdcMem, 11 * i, 0, SRCCOPY); 
					}
					else if (pdata->fPlatform.ps == pdata->fPlatform.pt)
					{
					DrawEdge(hdc, &pdata->fPlatform.ptb[i].rEdge, BDR_SUNKENOUTER, BF_RECT);
					BitBlt(hdc, pdata->fPlatform.ptb[i].tx2, pdata->fPlatform.ptb[i].ty2, 11, 11, hdcMem, 11 * i, 0, SRCCOPY); 
					}
					else
					{
						DrawEdge(hdc, &pdata->fPlatform.ptb[i].rEdge, BDR_RAISEDOUTER, BF_RECT);
						BitBlt(hdc, pdata->fPlatform.ptb[i].tx1, pdata->fPlatform.ptb[i].ty1, 11, 11, hdcMem, 11 * i, 0, SRCCOPY); 
					}
				}
				else
				{
					if (pdata->fPlatform.pt != i)
					{
						DrawEdge(hdc, &pdata->fPlatform.ptb[i].rEdge, 0, BF_FLAT);
						BitBlt(hdc, pdata->fPlatform.ptb[i].tx1, pdata->fPlatform.ptb[i].ty1, 11, 11, hdcMem, 11 * i, 0, SRCCOPY); 
					}
					else
					{
						DrawEdge(hdc, &pdata->fPlatform.ptb[i].rEdge, BDR_RAISEDOUTER, BF_RECT);
						BitBlt(hdc, pdata->fPlatform.ptb[i].tx1, pdata->fPlatform.ptb[i].ty1, 11, 11, hdcMem, 11 * i, 0, SRCCOPY); 
					}
				}
			}
			FillRect(hdc, &pdata->fPlatform.rtv, hBrushBlack);
			SetTextColor(hdc, RGB(160,160,32));
			SetBkColor(hdc, RGB(0,0,0));
			wsprintf(buf, "[%03d/%03d]", pdata->songno, NSFSDK_GetSongMax(pdata->hnsf));
			DrawText(hdc, buf, -1, &pdata->fPlatform.rtv, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
			SelectObject(hdc, hfontOld);
			DeleteDC(hdcMem);
			EndPaint(hwnd, &paintStruct);
			DeleteObject(hBitmap);
			DeleteObject(hBrushBlack);
			return 0;
		}
	}
	return CallWindowProc(pdata->fPlatform.fnOldWndProc, hwnd, uMsg, wParam, lParam);
}

static void PlatformNew(PluginInstance *pdata)
{
	pdata->fPlatform.hwnd = pdata->sndpdi.hwnd = 0;
	pdata->fPlatform.fnOldWndProc = 0;
	pdata->fPlatform.pt = MAX_PLAYERTB;
	pdata->fPlatform.ps = MAX_PLAYERTB;
}
static void PlatformCatchWindow(PluginInstance *pdata, NPWindow *window)
{
	int i;
	pdata->fPlatform.hwnd = pdata->sndpdi.hwnd = (HWND)window->window;
	SetProp(pdata->fPlatform.hwnd, gPropNameInstance, (HANDLE)pdata);
	pdata->fPlatform.fnOldWndProc = (WNDPROC)SetWindowLong(pdata->fPlatform.hwnd, GWL_WNDPROC, (LONG)PluginWndProc);

	pdata->fPlatform.ex = GetSystemMetrics(SM_CXEDGE);
	pdata->fPlatform.ey = GetSystemMetrics(SM_CYEDGE);
	pdata->fPlatform.cx = 11 + 2 * 2 + pdata->fPlatform.ex * 3;
	pdata->fPlatform.cy = 11 + 2 * 2 + pdata->fPlatform.ey * 3;
	for (i = 0; i < MAX_PLAYERTB; i++)
	{
		pdata->fPlatform.ptb[i].tx1 = pdata->fPlatform.cx * i + 2 + 0 + pdata->fPlatform.ex * 1;
		pdata->fPlatform.ptb[i].ty1 = 0 + 2 + 0 + pdata->fPlatform.ey * 1 ;
		pdata->fPlatform.ptb[i].tx2 = pdata->fPlatform.cx * i + 2 + 1 + pdata->fPlatform.ex * 1;
		pdata->fPlatform.ptb[i].ty2 = 0 + 2 + 1 + pdata->fPlatform.ey * 1 ;
		pdata->fPlatform.ptb[i].rEdge.left = pdata->fPlatform.cx * i;
		pdata->fPlatform.ptb[i].rEdge.right = pdata->fPlatform.cx * (i + 1);
		pdata->fPlatform.ptb[i].rEdge.top = 0;
		pdata->fPlatform.ptb[i].rEdge.bottom = pdata->fPlatform.cy;
	}
	pdata->fPlatform.rtv.left = pdata->fPlatform.cx * i;
	pdata->fPlatform.rtv.right = pdata->fPlatform.cx * (i + 4);
	pdata->fPlatform.rtv.top = 0;
	pdata->fPlatform.rtv.bottom = pdata->fPlatform.cy;
/*
	SetWindowPos(pdata->fPlatform.hwnd, NULL, 0, 0, pdata->fPlatform.rtv.right, pdata->fPlatform.rtv.bottom, SWP_NOZORDER);
*/
}
static void PlatformUncatchWindow(PluginInstance *pdata)
{
	SetWindowLong(pdata->fPlatform.hwnd, GWL_WNDPROC, (LONG)pdata->fPlatform.fnOldWndProc);
	RemoveProp(pdata->fPlatform.hwnd, gPropNameInstance);
	PlatformNew(pdata);
}
static NPError PlatformSetWindow(PluginInstance *pdata, NPWindow *window)
{
	if (pdata->fPlatform.hwnd != NULL)
	{
		if ((window == NULL) || (window->window == NULL))
		{
			PlatformUncatchWindow(pdata);
			return NPERR_NO_ERROR;
		}
		else if (pdata->fPlatform.hwnd == (HWND)window->window)
		{
			InvalidateRect(pdata->fPlatform.hwnd, NULL, TRUE);
			UpdateWindow(pdata->fPlatform.hwnd);
			return NPERR_NO_ERROR;
		}
		else
		{
			PlatformUncatchWindow(pdata);
		}
	}
	if ((window == NULL) || (window->window == NULL))
	{
		return NPERR_NO_ERROR;
	}
	PlatformCatchWindow(pdata, window);
	InvalidateRect(pdata->fPlatform.hwnd, NULL, TRUE);
	UpdateWindow(pdata->fPlatform.hwnd);
	return NPERR_NO_ERROR;
}
static NPError PlatformDestroy(PluginInstance *pdata)
{
	if (pdata->fPlatform.hwnd != NULL)
	{
		PlatformUncatchWindow(pdata);
	}
	return NPERR_NO_ERROR;
}
#elif !defined(_MAC)
/* X11 { */
void Redraw(Widget w, XtPointer closure, XEvent *event)
{
	PluginInstance *pdata = (PluginInstance *)closure;
	GC gc;
	XGCValues gcv;
	XtVaGetValues(w, XtNbackground, &gcv.background,
				  XtNforeground, &gcv.foreground, 0);
	gc = XCreateGC(pdata->fPlatform.display, pdata->fPlatform.window, 
				   GCForeground|GCBackground, &gcv);
	XDrawRectangle(pdata->fPlatform.display, pdata->fPlatform.window, gc, 
				   0, 0, pdata->fPlatform.width-1, pdata->fPlatform.height-1);
}
static void PlatformNew(PluginInstance *pdata)
{
	pdata->fPlatform.window = 0;
}
static NPError PlatformSetWindow(PluginInstance *pdata, NPWindow *window)
{
	Widget netscape_widget;
	pdata->fPlatform.window = (Window)window->window;
	pdata->fPlatform.x = window->x;
	pdata->fPlatform.y = window->y;
	pdata->fPlatform.width = window->width;
	pdata->fPlatform.height = window->height;
	pdata->fPlatform.display = ((NPSetWindowCallbackStruct *)window->ws_info)->display;
	netscape_widget = XtWindowToWidget(pdata->fPlatform.display, pdata->fPlatform.window);
	XtAddEventHandler(netscape_widget, ExposureMask, FALSE, (XtEventHandler)Redraw, pdata);
	Redraw(netscape_widget, (XtPointer)pdata, NULL);
	return NPERR_NO_ERROR;
}
static NPError PlatformDestroy(PluginInstance *pdata)
{
	return NPERR_NO_ERROR;
}
char* NPP_GetMIMEDescription(void)
{
	return PLUGIN_MIME;
}
NPError NPP_GetValue(void *future, NPPVariable variable, void *value)
{
	if (variable == NPPVpluginNameString)
		*((char **)value) = PLUGIN_NAME;
	else if (variable == NPPVpluginDescriptionString)
		*((char **)value) = PLUGIN_DESCRIPTION;
	else
		return NPERR_GENERIC_ERROR;
	return NPERR_NO_ERROR;
}
/* } X11 */
#else
/* Machintosh { */
static CGrafPort	gSavePort;
static CGrafPtr		gOldPort;
static NPBool StartDraw(NPWindow *window)
{
	if (window == NULL) return FALSE;
	if (window->clipRect.left < window->clipRect.right)
	{
		NP_Port *port = (NP_Port *)window->window;
		Rect clipRect;
		RGBColor col;
		/* Preserve the old port */
		GetPort((GrafPtr*)&gOldPort);
		SetPort((GrafPtr)port->port);
		/* Preserve the old drawing environment */
		gSavePort.portRect = port->port->portRect;
		gSavePort.txFont = port->port->txFont;
		gSavePort.txFace = port->port->txFace;
		gSavePort.txMode = port->port->txMode;
		gSavePort.rgbFgColor = port->port->rgbFgColor;
		gSavePort.rgbBkColor = port->port->rgbBkColor;
		GetClip(gSavePort.clipRgn);
		/* Setup our drawing environment */
		clipRect.top = window->clipRect.top + port->porty;
		clipRect.left = window->clipRect.left + port->portx;
		clipRect.bottom = window->clipRect.bottom + port->porty;
		clipRect.right = window->clipRect.right + port->portx;
		SetOrigin(port->portx,port->porty);
		ClipRect(&clipRect);
		clipRect.top = clipRect.left = 0;
		TextSize(12);
		TextFont(geneva);
		TextMode(srcCopy);
		col.red = col.green = col.blue = 0;
		RGBForeColor(&col);
		col.red = col.green = col.blue = 65000;
		RGBBackColor(&col);
		return TRUE;
	}
	else
		return FALSE;
}
static void EndDraw(NPWindow* window)
{
	CGrafPtr myPort;
	NP_Port *port = (NP_Port *)window->window;
	SetOrigin(gSavePort.portRect.left, gSavePort.portRect.top);
	SetClip(gSavePort.clipRgn);
	GetPort((GrafPtr*)&myPort);
	myPort->txFont = gSavePort.txFont;
	myPort->txFace = gSavePort.txFace;
	myPort->txMode = gSavePort.txMode;
	RGBForeColor(&gSavePort.rgbFgColor);
	RGBBackColor(&gSavePort.rgbBkColor);
	SetPort((GrafPtr)gOldPort);
}
static void DoDraw(PluginInstance *pdata)
{
	Rect drawRect;
	drawRect.top = 0;
	drawRect.left = 0;
	drawRect.bottom = drawRect.top + pdata->fWindow->height;
	drawRect.right = drawRect.left + pdata->fWindow->width;
	EraseRect(&drawRect);
	MoveTo(2, 12);
}
static void PlatformNew(PluginInstance *pdata)
{
	pdata->fPlatform.window = 0;
}
static NPError PlatformSetWindow(PluginInstance *pdata, NPWindow *window)
{
	pdata->fPlatform.window = window;
	if(StartDraw(pdata->fPlatform.window))
	{
		DoDraw(pdata);
		EndDraw(pdata->fPlatform.window);
	}
	return NPERR_NO_ERROR;
}
static NPError PlatformDestroy(PluginInstance *pdata)
{
	return NPERR_NO_ERROR;
}
static int16 PlatformHandleEvent(PluginInstance *pdata, void *event)
{
	int16 eventHandled = FALSE;
	if (pdata != NULL && event != NULL)
	{
		EventRecord *ev = (EventRecord *)event;
		switch (ev->what)
		{
			case updateEvt:
				if (StartDraw(pdata->fPlatform.window))
				{
					DoDraw(pdata);
					EndDraw(pdata->fPlatform.window);
				}
				eventHandled = true;
				break;
			default:
				break;
		}
	}
	return eventHandled;
}
int16 NPP_HandleEvent(NPP instance, void *event)
{
	if (instance != NULL)
	{
		PluginInstance *pdata = (PluginInstance *)instance->pdata;
		return PlatformHandleEvent(pdata, event);
	}
	return FALSE;
}
/* } Machintosh */
#endif

static void mime_init(EMIME *pmime)
{
	pmime->buf = NULL;
}
static void mime_term(EMIME *pmime)
{
	if (pmime->buf != NULL)
	{
		NPN_MemFree(pmime->buf);
	}
	mime_init(pmime);
}
static int mime_set(EMIME *pmime, char *type)
{
	if (pmime->buf != NULL) mime_term(pmime);
	pmime->buf = NPN_MemAlloc(strlen(type) + 8);
	if (pmime->buf == NULL)
	{
		mime_term(pmime);
		return 1;
	}
	strcpy(pmime->buf, type);
	return 0;
}

static void buf_init(EBUFFER *pbuf)
{
	pbuf->buf = NULL;
	pbuf->max = 0;
}

static void buf_reset(EBUFFER *pbuf)
{
	if (pbuf->buf != NULL)
	{
		NPN_MemFree(pbuf->buf);
		buf_init(pbuf);
	}
}

static int buf_add(EBUFFER *pbuf, void *p, unsigned l)
{
	unsigned size;
	if (pbuf->buf == NULL)
	{
		pbuf->max = 0x3FFF + 1;
		pbuf->buf = NPN_MemAlloc(pbuf->max);
		if (pbuf->buf == NULL) return 1;
		pbuf->offset = 0;
	}
	size = pbuf->offset + l;
	size = (size + 0x3FFF) & ~0x3FFF;	/* 4kB */
	if (pbuf->max < size)
	{
		void *nbuf;
		nbuf = NPN_MemAlloc(size);
		if (nbuf == NULL)
		{
			buf_reset(pbuf);
			return 1;
		}
		memcpy(nbuf, pbuf->buf, pbuf->offset);
		NPN_MemFree(pbuf->buf);
		pbuf->buf = nbuf;
		pbuf->max = size;
	}
	memcpy(pbuf->offset + (char *)pbuf->buf, p, l);
	pbuf->offset += l;
	return 0;
}

NPError NPP_Initialize(void)
{
	return NPERR_NO_ERROR;
}
jref NPP_GetJavaClass(void)
{
	return NULL;
}
void NPP_Shutdown(void)
{
}

NPError NPP_New(NPMIMEType pluginType, NPP instance, uint16 mode, int16 argc, char **argn, char **argv, NPSavedData *saved)
{
	if (NULL != instance)
	{
		int i;
		PluginInstance *pdata = NPN_MemAlloc(sizeof(PluginInstance));
		instance->pdata = pdata;
		if (NULL == pdata) return NPERR_OUT_OF_MEMORY_ERROR;
		pdata->instance = instance;
		pdata->fMode = mode;
		buf_init(&pdata->ebuf);
		mime_init(&pdata->mime);
		NPNEZInit(pdata);
		PlatformNew(pdata);
		for (i = 0; i < argc; i++)
		{
			if (0 == stricmp("SONGNO", argn[i]))
			{
				pdata->songno = atoi(argv[i]);
			}
		}
		return NPERR_NO_ERROR;
	}
	return NPERR_INVALID_INSTANCE_ERROR;
}

NPError NPP_Destroy(NPP instance, NPSavedData **save)
{
	if ((NULL != instance) && (NULL != instance->pdata))
	{
		PluginInstance *pdata = (PluginInstance *)instance->pdata;
		PlatformDestroy(pdata);
		NPNEZTerm(pdata);
		buf_reset(&pdata->ebuf);
		mime_term(&pdata->mime);
		NPN_MemFree(pdata);
		return NPERR_NO_ERROR;
	}
	return NPERR_INVALID_INSTANCE_ERROR;
}

NPError NPP_SetWindow(NPP instance, NPWindow *window)
{
	if ((NULL != instance) && (NULL != instance->pdata))
	{
		PluginInstance *pdata = (PluginInstance *)instance->pdata;
		return PlatformSetWindow(pdata, window);
	}
	return NPERR_INVALID_INSTANCE_ERROR;
}

NPError NPP_NewStream(NPP instance, NPMIMEType type, NPStream *stream, NPBool seekable, uint16 *stype)
{
	if ((NULL != instance) && (NULL != instance->pdata))
	{
		PluginInstance *pdata = (PluginInstance *)instance->pdata;
		*stype = NP_NORMAL;
		buf_reset(&pdata->ebuf);
		if (mime_set(&pdata->mime, type)) return NPERR_OUT_OF_MEMORY_ERROR;
		return NPERR_NO_ERROR;
	}
	return NPERR_INVALID_INSTANCE_ERROR;
}

NPError NPP_DestroyStream(NPP instance, NPStream *stream, NPReason reason)
{
	if ((NULL != instance) && (NULL != instance->pdata))
	{
		PluginInstance *pdata = (PluginInstance *)instance->pdata;
		if (NPRES_DONE == reason)
		{
			NPNEZLoad(pdata);
			NPNEZPlay(pdata);
		}
		buf_reset(&pdata->ebuf);
		return NPERR_NO_ERROR;
	}
	return NPERR_INVALID_INSTANCE_ERROR;
}

int32 NPP_WriteReady(NPP instance, NPStream *stream)
{
	const int32 STREAMBUFSIZE = 0X0FFFFFFF;
	if ((NULL != instance) && (NULL != instance->pdata))
	{
		return STREAMBUFSIZE;
	}
	return 0;
}

int32 NPP_Write(NPP instance, NPStream *stream, int32 offset, int32 len, void *buffer)
{
	if ((NULL != instance) && (NULL != instance->pdata))
	{
		PluginInstance *pdata = (PluginInstance *)instance->pdata;
		if (len && buf_add(&pdata->ebuf, buffer, len))
			return 0;
		return len;
	}
	return 0;
}

void NPP_StreamAsFile(NPP instance, NPStream *stream, const char* fname)
{
	if ((NULL != instance) && (NULL != instance->pdata))
	{
		PluginInstance *pdata = (PluginInstance *)instance->pdata;
		FILE *fp = NULL;
		pdata->instance = instance;
		buf_reset(&pdata->ebuf);
		fp = fopen(fname, "rb");
		if (fp == NULL) return;
		do {
			char buffer[4096];
			unsigned len;
			while (!feof(fp) && !ferror(fp))
			{
				len = fread(buffer, 1, sizeof(buffer), fp);
				if (len && buf_add(&pdata->ebuf, buffer, len)) break;
			}
			if (!ferror(fp))
			{
				NPNEZLoad(pdata);
				NPNEZPlay(pdata);
			}
		} while(0);
		buf_reset(&pdata->ebuf);
		if (fp) fclose(fp);
	}
}

void NPP_Print(NPP instance, NPPrint* printInfo)
{
}
void NPP_URLNotify(NPP instance, const char* url, NPReason reason, void* notifyData)
{
}
