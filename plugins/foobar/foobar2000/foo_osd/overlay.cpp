#define _WIN32_WINNT 0x500

#if defined(GDIPLUS) && !defined(UNICODE)
#define UNICODE
#define _UNICODE
#endif

#include <windows.h>

#ifdef GDIPLUS
#include <gdiplus.h>
using namespace Gdiplus;
#pragma comment(lib, "gdiplus.lib")
#endif

#include "overlay.h"
#include "dissolve.h"

#define IDT_DUE_TIME		1
#define IDT_FADE_TIME		2

#ifdef GDIPLUS

static PointF get_text_size(Graphics * graphics, Font * font, const TCHAR * text, UINT len)
{
	PointF rv(0.f,0.f);
	RectF rect;
	StringFormat format(StringFormat::GenericTypographic());
	format.SetFormatFlags(StringFormatFlagsMeasureTrailingSpaces);
	graphics->MeasureString(text, len, font, rv, &format, &rect);
	rv.X = rect.Width;
	rv.Y = rect.Height;
	return rv;
}

static PointF get_text_size(Graphics * graphics, Font * font, const char * text, UINT len)
{
	string8 meh(text,len);
	string8 out;
	int s = 0, e;
	while ((e = meh.find_first(3, s)) >= 0)
	{
		out.add_string(meh.get_ptr() + s, e - s);
		s = meh.find_first(3, e + 1);
		if (s >= 0) s++;
		else
		{
			s = strlen(meh);
			break;
		}
	}
	e = strlen(meh.get_ptr()+s);
	if (e)
	{
		out += meh.get_ptr()+s;
	}
	string_os_from_utf8 temp(out);
	return get_text_size(graphics, font, temp, _tcslen(temp));
}

static float get_text_width(Graphics * graphics, Font * font, const TCHAR * src,int len)
{
	PointF ptSize = get_text_size(graphics, font, src, len);
	return ptSize.X;
}

static float get_text_width(Graphics * graphics, Font * font, const char * src,int len)
{
	PointF ptSize = get_text_size(graphics, font, src, len);
	return ptSize.X;
}

static float get_text_height(Graphics * graphics, Font * font, const char * src, UINT len)
{
	PointF ptSize = get_text_size(graphics, font, src, len);
	return ptSize.Y;
}

#define swap_color(x) (((x) & 0xFF00) | (((x) & 0xFF) << 16) | (((x) & 0xFF0000) >> 16))
#define invert_color(x) ((0xFFFFFF - ((x) & 0xFFFFFF)) | ((x) & 0xFF000000))

static BOOL text_out_colors(Graphics * graphics, Font * font, const TCHAR * src, int len, PointF & origin, bool selected, DWORD default_color)
{
	default_color = 0xFF000000 | swap_color(default_color);
	SolidBrush brush(Color(selected ? invert_color(default_color) : default_color));

	int title_ptr = 0;
	int textout_start = 0;
	PointF myOrigin(origin);

	StringFormat format(StringFormat::GenericTypographic());

	for(;;)
	{
		if (title_ptr>=len || src[title_ptr]==3)
		{
			if (title_ptr>textout_start)
			{
				float width = get_text_width(graphics,font,src+textout_start,title_ptr-textout_start);
				graphics->DrawString(src+textout_start,title_ptr-textout_start,font,myOrigin,&format,&brush);
				myOrigin.X = (float)floor(myOrigin.X + width + .5);
				textout_start = title_ptr;
			}
			if (title_ptr>=len) break;
		}
		if (src[title_ptr]==3)
		{
			DWORD new_color;
			DWORD new_inverted;
			bool have_inverted = false;

			if (src[title_ptr+1]==3) {new_color=default_color;title_ptr+=2;}
			else
			{
				title_ptr++;
				new_color = _tcstoul(src+title_ptr,0,16);
				while(title_ptr<len && src[title_ptr]!=3)
				{
					if (!have_inverted && src[title_ptr-1]=='|')
					{
						new_inverted = _tcstoul(src+title_ptr,0,16);
						have_inverted = true;
					}
					title_ptr++;
				}
				if (title_ptr<len) title_ptr++;
			}
			if (selected) new_color = have_inverted ? new_inverted : invert_color(new_color);
			brush.SetColor(0xFF000000 | swap_color(new_color));
			textout_start = title_ptr;
		}
		else
		{
			title_ptr = CharNext(src+title_ptr)-src;
		}
	}
	return TRUE;
}

BOOL uTextOutColorz(Graphics * graphics, Font * font, const char * text,UINT len, PointF & origin, BOOL is_selected, DWORD default_color)
{
	unsigned temp_len = estimate_utf8_to_os(text,len);
	mem_block_t<TCHAR> temp_block;
	TCHAR * temp;
	if ( ( temp_len * sizeof(TCHAR) ) <= PFC_ALLOCA_LIMIT )
	{
		temp = ( TCHAR * ) alloca( temp_len * sizeof( TCHAR ) );
	}
	else
	{
		if ( ! temp_block.set_size( temp_len ) )
		{
			return FALSE;
		}

		temp = temp_block.get_ptr();
	}
	assert(temp);
	convert_utf8_to_os(text,temp,len);

	return text_out_colors(graphics,font,temp,_tcslen(temp),origin,!!is_selected,default_color);
}

static void drawtext(Graphics * graphics, Font * font, const char * text, UINT len, PointF & origin, PointF & size, DWORD default_color, BOOL outline, int align)
{
	PointF myOrigin(origin);
	if (align == DT_CENTER)
	{
		myOrigin.X += ((size.X - get_text_width(graphics, font, text, len)) * .5f);
	}
	else if (align == DT_RIGHT)
	{
		myOrigin.X += size.X - get_text_width(graphics, font, text, len);
	}

	if (outline)
	{
		PointF temp;
		for (temp.Y = myOrigin.Y - 1.f; temp.Y <= myOrigin.Y + 1.f; temp.Y++)
		{
			for (temp.X = myOrigin.X - 1.f; temp.X <= myOrigin.X + 1.f; temp.X++)
			{
				if (temp.Equals(myOrigin)) continue;
				uTextOutColorz(graphics, font, text, len, temp, true, default_color);
			}
		}
	}

	uTextOutColorz(graphics, font, text, len, myOrigin, false, default_color);
}

static PointF get_text_size_lines(Graphics * graphics, Font * font, const char * text, UINT len)
{
	PointF rv(0.0f, 0.0f);
	string8 meh(text,len);
	int s = 0, e;
	while ((e = meh.find_first('\n', s)) >= 0)
	{
		PointF temp = get_text_size(graphics, font, meh.get_ptr() + s, e - s);
		if (temp.X > rv.X) rv.X = temp.X;
		rv.Y += temp.Y + 4.f;
		if (*(meh.get_ptr()+e) == 10) e++;
		s = e;
	}
	e = strlen(meh.get_ptr()+s);
	if (e)
	{
		PointF temp = get_text_size(graphics, font, meh.get_ptr() + s , e);
		if (temp.X > rv.X) rv.X = temp.X;
		rv.Y += temp.Y;
	}
	else
		rv.Y -= 4.f;
	return rv;
}

static void drawtextlines(Graphics * graphics, Font * font, const char * text, UINT len, PointF & origin, DWORD default_color, BOOL outline, int align)
{
	string8 meh(text,len);
	int s = 0, e;
	PointF size = get_text_size_lines(graphics, font, text, len);
	PointF myOrigin(origin);
	while ((e = meh.find_first('\n', s)) >= 0)
	{
		drawtext(graphics, font, meh.get_ptr() + s, e - s, myOrigin, size, default_color, outline, align);
		myOrigin.Y += get_text_height(graphics, font, meh.get_ptr() + s, e - s) + 4.f;
		if (*(meh.get_ptr()+e) == 10) e++;
		s = e;
	}
	e = strlen(meh.get_ptr()+s);
	if (e) drawtext(graphics, font, meh.get_ptr() + s , e, myOrigin, size, default_color, outline, align);
}

#else

static int get_text_width(HDC dc,const char * src,int len)
{
	if (len<=0) return 0;
	else
	{
		SIZE s;
		uGetTextExtentPoint32(dc,src,len,&s);
		return s.cx;
	}
}

static int get_text_width_color(HDC dc,const char * src,int len)
{
	int ptr = 0;
	int start = 0;
	int rv = 0;
	if (len<0) len = strlen(src);
	while(ptr<len)
	{
		if (src[ptr]==3)
		{
			rv += get_text_width(dc,src+start,ptr-start);
			ptr++;
			while(ptr<len && src[ptr]!=3) ptr++;
			if (ptr<len) ptr++;
			start = ptr;
		}
		else ptr++;
	}
	rv += get_text_width(dc,src+start,ptr-start);
	return rv;
}

static void drawtext(HDC dc, const char * text, UINT len, int x, int y, const RECT *clip, DWORD default_color, BOOL outline, BOOL pure_white, int align)
{
	if (clip)
	{
		if (align == DT_CENTER)
		{
			x = (((clip->right - clip->left) - get_text_width_color(dc, text, len)) / 2) + clip->left;
		}
		else if (align == DT_RIGHT)
		{
			x = clip->right - get_text_width_color(dc, text, len) - 10;
		}
	}

	if (pure_white)
	{
		string_utf8_nocolor pw(text, len);
		len = pw.length();
		text = new char[len+16];
		memcpy((void*)text, "\003ffffff|ffffff\003", 15);
		memcpy((void*)(text + 15), pw.get_ptr(), len + 1);
		len += 15;
	}

	if (outline)
	{
		uTextOutColors(dc, text, len, x-1, y-1, clip, true, default_color);
		uTextOutColors(dc, text, len, x, y-1, clip, true, default_color);
		uTextOutColors(dc, text, len, x+1, y-1, clip, true, default_color);
		uTextOutColors(dc, text, len, x-1, y, clip, true, default_color);
		uTextOutColors(dc, text, len, x+1, y, clip, true, default_color);
		uTextOutColors(dc, text, len, x-1, y+1, clip, true, default_color);
		uTextOutColors(dc, text, len, x, y+1, clip, true, default_color);
		uTextOutColors(dc, text, len, x+1, y+1, clip, true, default_color);
	}

	uTextOutColors(dc, text, len, x, y, clip, false, default_color);

	if (pure_white) delete (void*) text;
}

static void drawtextlines(HDC dc, const char * text, UINT len, int x, int y, const RECT *clip, DWORD default_color, BOOL outline, BOOL pure_white, int align)
{
	string8 meh(text, len);
	int s = 0, e;
	while ((e = meh.find_first('\n', s)) >= 0)
	{
		drawtext(dc, meh.get_ptr() + s, e - s, x, y, clip, default_color, outline, pure_white, align);
		y += uGetTextHeight(dc) + 4;
		if (*(meh.get_ptr()+e) == 10) e++;
		s = e;
	}
	e = strlen(meh.get_ptr()+s);
	if (e) drawtext(dc, meh.get_ptr() + s , e, x, y, clip, default_color, outline, pure_white, align);
}

static SIZE get_text_size_lines(HDC dc, const char * text, UINT len)
{
	SIZE rv = {0,0};
	string8 meh(text,len);
	if (meh.length())
	{
		int s = 0, e, x, y = uGetTextHeight(dc);
		while ((e = meh.find_first('\n', s)) >= 0)
		{
			x = get_text_width_color(dc, meh.get_ptr() + s, e - s);
			if (x > rv.cx) rv.cx = x;
			rv.cy += y + 4;
			if (*(meh.get_ptr()+e) == 10) e++;
			s = e;
		}
		e = strlen(meh.get_ptr()+s);
		if (e)
		{
			x = get_text_width_color(dc, meh.get_ptr() + s, e);
			if (x > rv.cx) rv.cx = x;
			rv.cy += y;
		}
		else
			rv.cy -= 4;
	}
	return rv;
}
#endif

static const TCHAR class_name[] = _T( "1FCD54F1-335E-4bed-9128-0E83807381CE" );

class register_window_class
{
	volatile LONG ref_count;
	ATOM class_atom;

public:
	register_window_class() : ref_count(0), class_atom(0) {}
	~register_window_class()
	{
		if (ref_count) _unregister();
	}

	ATOM Register()
	{
		if (InterlockedIncrement(&ref_count) == 1)
		{
			_register();
			if (!class_atom) ref_count = 0;
		}
		return class_atom;
	}

	void Unregister()
	{
		if (InterlockedDecrement(&ref_count) == 0) _unregister();
	}

private:
	void _register()
	{
		WNDCLASS wc;
		memset(&wc, 0, sizeof(wc));
		wc.lpfnWndProc   = (WNDPROC)COsdWnd::WndProc;
		wc.hInstance     = core_api::get_my_instance();
		wc.lpszClassName = class_name;
		class_atom = RegisterClass(&wc);
	}

	void _unregister()
	{
		if (class_atom)
		{
			UnregisterClass((const TCHAR *)class_atom, core_api::get_my_instance());
			class_atom = 0;
		}
	}
};

register_window_class rwc;

COsdWnd::COsdWnd(osd_state & _state) : m_state(_state)
{
	m_hWnd = NULL;
	m_hBitmap = NULL;
	m_dissolve = NULL;
}

typedef const char * pconst;

BOOL uFormatMessage(DWORD dw_error, string_base & out)
{
	PVOID buffer;
	size_t length = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 0, dw_error, 0, (LPTSTR) &buffer, 256, 0 );
	if ( length )
	{
#ifdef UNICODE
		out.set_string_utf16( ( const WCHAR * ) buffer );
#else
		out.set_string_ansi( ( const char * ) buffer );
#endif
		LocalFree( buffer );
		return TRUE;
	}
	return FALSE;
}

bool COsdWnd::Initialize()
{
	ATOM wnd_class = rwc.Register();

	if (!wnd_class)
	{
		string8 error;
		uFormatMessage(GetLastError(), error);
		console::info(uStringPrintf("Failed to register window class - %s", pconst(error)));
		return 0;
	}

	HWND hWndDesktop = GetDesktopWindow();

	m_hWnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_TRANSPARENT,
		(const TCHAR *)wnd_class, NULL, WS_POPUP, 0, 0, 0, 0, hWndDesktop, NULL, core_api::get_my_instance(), this);

	if (!m_hWnd)
	{
		string8 error;
		uFormatMessage(GetLastError(), error);
		console::info(uStringPrintf("Failed to create window - %s", pconst(error)));
		rwc.Unregister();
		return false;
	}
	else
	{
		m_sState = HIDDEN;
		m_mMode = TEXT;
		return true;
	}
}

COsdWnd::~COsdWnd()
{
	if (IsWindow(m_hWnd))
	{
		DestroyWindow(m_hWnd);
		m_hWnd = NULL;
		rwc.Unregister();
	}

	if (m_dissolve)
	{
		delete m_dissolve;
		m_dissolve = NULL;
	}

	if (m_hBitmap)
	{
		DeleteObject(m_hBitmap);
		m_hBitmap = NULL;
	}
}

void COsdWnd::Post(const char * msg, bool interval)
{
	m_bInterval = interval;
	Setup();
	m_strText = msg;
	Repaint();
	Setup2();
	Update();
}

void COsdWnd::PostVolume(int volume)
{
	m_bInterval = false;
	Setup();
	bool repainted = RepaintVolume(volume);
	Setup2();
	if (repainted) Update();
}

void COsdWnd::Repost(const char * msg)
{
	m_strText = msg;
	Repaint();
	Update();
}

void COsdWnd::Hide()
{
	if (m_sState == FADING_IN || m_sState == VISIBLE)
	{
		KillTimer(m_hWnd, IDT_DUE_TIME);
		KillTimer(m_hWnd, IDT_FADE_TIME);
		uSendMessage(m_hWnd, WM_TIMER, IDT_DUE_TIME, 0);
	}
}

COsdWnd::STATE COsdWnd::GetState()
{
	return m_sState;
}

bool COsdWnd::DoInterval()
{
	return m_bInterval;
}

void COsdWnd::Setup()
{
	KillTimer(m_hWnd, IDT_DUE_TIME);
	KillTimer(m_hWnd, IDT_FADE_TIME);
	if (m_sState == HIDDEN || m_sState == FADING_OUT)
	{
		const osd_config & p_config = m_state.get();
		if (p_config.flags & osd_fadeinout)
		{
			m_iFadeTo = (p_config.flags & osd_alpha) ? (p_config.alphalev << 8) : 65280;
			if (m_sState != FADING_OUT)
			{
				m_iFadeNow = 0;
				m_iFadeStep = m_iFadeTo * 10 / p_config.fadetime;
				if (!m_iFadeStep) m_iFadeStep = 1;
			}
			m_bFade = TRUE;
			m_sState = FADING_IN;
		}
		else
		{
			m_bFade = FALSE;
			m_iFadeNow = (p_config.flags & osd_alpha) ? (p_config.alphalev << 8) : 65280;
		}
	}
	ShowWindow(m_hWnd, SW_SHOWNA);
}

void COsdWnd::Setup2()
{
	if (!m_bFade && m_sState == HIDDEN) m_sState = VISIBLE;
	const osd_config & p_config = m_state.get();
	if (m_sState == VISIBLE && !(p_config.flags & osd_permanent)) SetTimer(m_hWnd, IDT_DUE_TIME, p_config.displaytime, NULL);
	else if (m_sState == FADING_IN) SetTimer(m_hWnd, IDT_FADE_TIME, 10, NULL);
}

void COsdWnd::Update()
{
	insync(m_bitmapsync);
	if (m_hBitmap)
	{
		POINT ptSrc = {0, 0};
		BLENDFUNCTION blend = {AC_SRC_OVER, 0, m_iFadeNow >> 8, AC_SRC_ALPHA};
		HDC dDC = GetDC(NULL);
		HDC hDC = CreateCompatibleDC(dDC);
		HBITMAP pOldBmp = (HBITMAP) SelectObject(hDC, m_hBitmap);
		UpdateLayeredWindow(m_hWnd, dDC, &m_pPos, &m_sSize, hDC, &ptSrc, 0, &blend, ULW_ALPHA);
		SelectObject(hDC, pOldBmp);
		DeleteDC(hDC);
		ReleaseDC(NULL, dDC);
	}
}

void COsdWnd::HideInternal()
{
	SIZE sz = {0, 0};
	UpdateLayeredWindow(m_hWnd, 0, 0, &sz, 0, 0, 0, 0, 0);
	ShowWindow(m_hWnd, SW_HIDE);
	m_sState = HIDDEN;
}

LRESULT CALLBACK COsdWnd::WndProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	COsdWnd *pThis;

	if(msg == WM_NCCREATE)
	{
		pThis = (COsdWnd *)((CREATESTRUCT *)(lp))->lpCreateParams;
		uSetWindowLong(wnd, GWL_USERDATA, (long)pThis);
	}
	else
	{
		pThis = reinterpret_cast<COsdWnd *>(uGetWindowLong(wnd, GWL_USERDATA));
	}

	return pThis ? pThis->WindowProc(wnd, msg, wp, lp) : uDefWindowProc(wnd, msg, wp, lp);
}

LRESULT CALLBACK COsdWnd::WindowProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch(msg)
	{
	case WM_DESTROY:
		KillTimer(wnd, IDT_DUE_TIME);
		KillTimer(wnd, IDT_FADE_TIME);
		break;

	case WM_TIMER:
		switch (wp)
		{
		case IDT_DUE_TIME:
			{
				KillTimer(wnd, IDT_DUE_TIME);
				if (m_bFade)
				{
					m_iFadeTo = 0;
					m_sState = FADING_OUT;
					if (m_dissolve) m_bInterval = false;
					SetTimer(wnd, IDT_FADE_TIME, 10, 0);
				}
				else
				{
					m_iFadeNow = 0;
					Update();
					HideInternal();
				}
			}
			break;
		case IDT_FADE_TIME:
			{
				bool update = true;
				if (m_sState == FADING_IN)
				{
					if ((m_iFadeNow & 0xFF00) == ((m_iFadeNow + m_iFadeStep) & 0xFF00)) update = false;
					m_iFadeNow += m_iFadeStep;
					if (m_iFadeNow > m_iFadeTo) m_iFadeNow = m_iFadeTo;
				}
				else
				{
					if (m_dissolve)
					{
						if (m_dissolve->draw())
						{
							m_iFadeNow = m_iFadeTo;
							delete m_dissolve;
							m_dissolve = NULL;
						}
					}
					else
					{
						if ((m_iFadeNow & 0xFF00) == ((m_iFadeNow - m_iFadeStep) & 0xFF00)) update = false;
						m_iFadeNow -= m_iFadeStep;
						if (m_iFadeNow < m_iFadeTo) m_iFadeNow = m_iFadeTo;
					}
				}
				if (m_iFadeNow == m_iFadeTo)
				{
					KillTimer(wnd, IDT_FADE_TIME);
					if (m_sState == FADING_OUT)
					{
						HideInternal();
						break;
					}
					else
					{
						m_sState = VISIBLE;
						const osd_config & p_config = m_state.get();
						if (!(p_config.flags & osd_permanent)) SetTimer(wnd, IDT_DUE_TIME, p_config.displaytime, NULL);
					}
				}
				if (update) Update();
			}
			break;
		}
		break;

	default:
		return uDefWindowProc(wnd, msg, wp, lp);
	}

	return 0;
}

void COsdWnd::Repaint()
{
	m_mMode = TEXT;

	const osd_config & p_config = m_state.get();

	HFONT hFont;
// FUCKO
#if 1 //#ifdef GDIPLUS
	{
		t_font_description FUCKO = p_config.font;
		hFont = FUCKO.create();
	}
#else
	{
		uLOGFONT font = p_config.font;
		font.lfQuality = (p_config.flags & osd_antialias) ? ANTIALIASED_QUALITY : NONANTIALIASED_QUALITY;
		hFont = uCreateFontIndirect(&font);
	}
#endif
	if (!hFont) return;

	RECT rc;
	HDC dDC = GetDC(NULL);
	if (!GetClientRect(GetDesktopWindow(), &rc))
	{
		ReleaseDC(NULL, dDC);
		return;
	}

	{
#ifdef GDIPLUS
		HFONT pOldFont = (HFONT) SelectObject(dDC, hFont);
		Graphics graphics(dDC);
		Font font(dDC);
		graphics.SetTextRenderingHint((p_config.flags & osd_antialias) ? TextRenderingHintAntiAliasGridFit : TextRenderingHintSingleBitPerPixelGridFit);
		PointF sz = get_text_size_lines(&graphics, &font, m_strText.get_ptr(), m_strText.length());
		m_sSize.cx = (int)(sz.X + 20.5f);
		m_sSize.cy = (int)(sz.Y + 20.5f);
		m_pPos.y = (p_config.y * rc.bottom / 100) - m_sSize.cy / 2;
#else
		HFONT pOldFont = (HFONT) SelectObject(dDC, hFont);
		SIZE sz = get_text_size_lines(dDC, m_strText.get_ptr(), m_strText.length());
		m_sSize.cx = sz.cx + 20;
		m_sSize.cy = sz.cy + 20;
		m_pPos.y = (p_config.y * rc.bottom / 100) - m_sSize.cy / 2;
#endif
		switch (p_config.pos)
		{
		case DT_RIGHT:
			m_pPos.x = (p_config.x * rc.right / 100);
			break;
		case DT_CENTER:
			m_pPos.x = (p_config.x * rc.right / 100) - m_sSize.cx / 2;
			break;
		case DT_LEFT:
			m_pPos.x = (p_config.x * rc.right / 100) - m_sSize.cx;
			break;
		}
		SelectObject(dDC, pOldFont);
	}

	insync(m_bitmapsync);
	if (m_hBitmap) DeleteObject(m_hBitmap);

	BYTE * gfx;

	{
		BITMAPINFOHEADER bmih;
		bmih.biSize = sizeof(bmih);
		bmih.biWidth = m_sSize.cx;
		bmih.biHeight = m_sSize.cy;
		bmih.biPlanes = 1;
		bmih.biBitCount = 32;
		bmih.biCompression = BI_RGB;
		bmih.biSizeImage = 0;
		bmih.biXPelsPerMeter = 0;
		bmih.biYPelsPerMeter = 0;
		bmih.biClrUsed = 0;
		bmih.biClrImportant = 0;

		m_hBitmap = CreateDIBSection(0, (const BITMAPINFO *) &bmih, DIB_RGB_COLORS, (VOID **) &gfx, 0, 0);
		if (!m_hBitmap)
		{
			ReleaseDC(NULL, dDC);
			DeleteObject(hFont);
			return;
		}
	}

	if (m_dissolve)
	{
		delete m_dissolve;
		m_dissolve = NULL;
	}
	if ((p_config.flags & osd_dissolve) && m_bFade) m_dissolve = new dissolve(gfx, m_sSize.cx, m_sSize.cy, p_config.dissolve_decay);

#ifdef GDIPLUS
	BOOL bShitFixed = FALSE;
	Color bgcolor(swap_color(p_config.bgcolor) | (p_config.alphaback << 24));

	if (p_config.alphaback == 0)
	{
		bgcolor.SetValue(swap_color(p_config.bgcolor) | (1 << 24));
		bShitFixed = TRUE;
	}
#endif	

	HDC hDC = CreateCompatibleDC(dDC);
	HBITMAP pOldBmp = (HBITMAP) SelectObject(hDC, m_hBitmap);
	HFONT pOldFont = (HFONT) SelectObject(hDC, hFont);
#ifdef GDIPLUS
	{
		Graphics graphics(hDC);
		graphics.Clear(bgcolor);

		PointF origin(10.0f, 10.0f);
		Font font(hDC);

		graphics.SetTextRenderingHint((p_config.flags & osd_antialias) ? TextRenderingHintAntiAliasGridFit : TextRenderingHintSingleBitPerPixelGridFit);
		drawtextlines(&graphics, &font, m_strText.get_ptr(), m_strText.length(), origin, p_config.color, p_config.flags & osd_outline, p_config.align);
	}
#else
	{
		int alpha = p_config.alphaback;
		COLORREF color = RGB(GetRValue(p_config.bgcolor)*alpha/255,GetGValue(p_config.bgcolor)*alpha/255,GetBValue(p_config.bgcolor)*alpha/255);
		RECT clip = {0, 0, m_sSize.cx, m_sSize.cy};
		HBRUSH brush = CreateSolidBrush(RGB(alpha,alpha,alpha));
		FillRect(hDC, &clip, brush);
		DeleteObject(brush);
		drawtextlines(hDC, m_strText.get_ptr(), m_strText.length(), 10, 10, &clip, p_config.color, p_config.flags & osd_outline, TRUE, p_config.align);

		BYTE * ptr = new BYTE[m_sSize.cy*m_sSize.cx];
		BYTE * bptr = gfx;
		BYTE * aptr = ptr;

		for (int y = 0; y < m_sSize.cy; y++)
		{
			for (int x = 0; x < m_sSize.cx; x++)
			{
				*aptr++ = bptr[2];
				bptr += 4;
			}
		}

		brush = CreateSolidBrush(color);
		FillRect(hDC, &clip, brush);
		DeleteObject(brush);
		drawtextlines(hDC, m_strText.get_ptr(), m_strText.length(), 10, 10, &clip, p_config.color, p_config.flags & osd_outline, FALSE, p_config.align);

		bptr = gfx;
		aptr = ptr;

		for (int y = 0; y < m_sSize.cy; y++)
		{
			for (int x = 0; x < m_sSize.cx; x++)
			{
				bptr[3] = *aptr++;
				bptr += 4;
			}
		}

		delete ptr;
	}
#endif

	SelectObject(hDC, pOldFont);
	SelectObject(hDC, pOldBmp);
	DeleteDC(hDC);
	ReleaseDC(NULL, dDC);
	DeleteObject(hFont);

#ifdef GDIPLUS
	// Oh boy, shit to clean up due to bug in renderer
	for (int y = 0; y < m_sSize.cy; y++)
	{
		for (int x = 0; x < m_sSize.cx; x++)
		{
			if (gfx[3] == 0) gfx[3] = 255;
			else if (bShitFixed && gfx[3] == 1) gfx[3] = 0;
			gfx += 4;
		}
	}
#endif
}

bool COsdWnd::RepaintVolume(int volume)
{
	RECT rc;
	HDC dDC = GetDC(NULL);
	if (!GetClientRect(GetDesktopWindow(), &rc))
	{
		ReleaseDC(NULL, dDC);
		return false;
	}

	const osd_config & p_config = m_state.get();

	int vsteps = p_config.vsteps;
	int vmin = p_config.vmin;

	if (volume < vmin) volume = vmin;

	int volq = (volume * vsteps) / (-vmin) + vsteps;
	if (volq > 0) volq--;

	SIZE sz;
	POINT pos;

	sz.cx = rc.right * p_config.vwidth / 100;
	sz.cy = rc.bottom * p_config.vheight / 100;
	pos.y = (p_config.y * rc.bottom / 100) - sz.cy / 2;
	switch (p_config.pos)
	{
	case DT_RIGHT:
		pos.x = (p_config.x * rc.right / 100);
		break;
	case DT_CENTER:
		pos.x = (p_config.x * rc.right / 100) - sz.cx / 2;
		break;
	case DT_LEFT:
		pos.x = (p_config.x * rc.right / 100) - sz.cx;
		break;
	}

	if (m_mMode == VOLUME &&
		volq == m_iLastVolQ &&
		((volume <= vmin && m_iLastVol <= vmin) || (!volume && !m_iLastVol) ||
		(volume < 0 && volume > vmin && m_iLastVol < 0 && m_iLastVol > vmin)) &&
		pos.x == m_pPos.x && pos.y == m_pPos.y &&
		sz.cx == m_sSize.cx && sz.cy == m_sSize.cy) return false;

	m_mMode = VOLUME;
	m_iLastVol = volume;
	m_iLastVolQ = volq;
	m_pPos.x = pos.x;
	m_pPos.y = pos.y;
	m_sSize.cx = sz.cx;
	m_sSize.cy = sz.cy;

	insync(m_bitmapsync);
	if (m_hBitmap) DeleteObject(m_hBitmap);

	BYTE * gfx;

	{
		BITMAPINFOHEADER bmih;
		bmih.biSize = sizeof(bmih);
		bmih.biWidth = m_sSize.cx;
		bmih.biHeight = m_sSize.cy;
		bmih.biPlanes = 1;
		bmih.biBitCount = 32;
		bmih.biCompression = BI_RGB;
		bmih.biSizeImage = 0;
		bmih.biXPelsPerMeter = 0;
		bmih.biYPelsPerMeter = 0;
		bmih.biClrUsed = 0;
		bmih.biClrImportant = 0;

		m_hBitmap = CreateDIBSection(0, (const BITMAPINFO *) &bmih, DIB_RGB_COLORS, (VOID **) &gfx, 0, 0);
		if (!m_hBitmap)
		{
			ReleaseDC(NULL, dDC);
			return false;
		}
	}

	if (m_dissolve)
	{
		delete m_dissolve;
		m_dissolve = NULL;
	}
	if ((p_config.flags & osd_dissolve) && m_bFade) m_dissolve = new dissolve(gfx, m_sSize.cx, m_sSize.cy, p_config.dissolve_decay);

	DWORD color = p_config.color, ocolor = 0;

	{
		DWORD fgcolor, olcolor;

		if (*(p_config.format.get_ptr()) == 3)
		{
			const char * src = p_config.format.get_ptr();
			int ptr = 1, len = strlen(src);
			bool have_outline = false;

			if (src[ptr]!=3)
			{
				color = strtoul(src+ptr,0,16);
				while(ptr<len && src[ptr]!=3)
				{
					if (!have_outline && src[ptr-1]=='|')
					{
						ocolor = strtoul(src+ptr,0,16);
						have_outline = true;
					}
					ptr++;
				}
			}
		}
		else if (!stricmp_utf8_partial(p_config.format.get_ptr(), "$rgb("))
		{
			const char * ptr = p_config.format.get_ptr() + 5;
			const char * ptr2;

			fgcolor = strtoul(ptr, (char **) &ptr2, 10);
			if (ptr2 != ptr && *ptr2 == ',' && ptr2[1])
			{
				fgcolor |= strtoul(ptr2 + 1, (char **) &ptr, 10) << 8;
				if (ptr != ptr2 && *ptr == ',' && ptr[1])
				{
					fgcolor |= strtoul(ptr + 1, (char **) &ptr2, 10) << 16;
					if (ptr2 != ptr && *ptr2 == ',' && ptr2[1])
					{
						olcolor = strtoul(ptr2 + 1, (char **) &ptr, 10);
						if (ptr != ptr2 && *ptr == ',' && ptr[1])
						{
							olcolor |= strtoul(ptr + 1, (char **) &ptr2, 10) << 8;
							if (ptr2 != ptr && *ptr2 == ',' && ptr2[1])
							{
								olcolor |= strtoul(ptr2 + 1, (char **) &ptr, 10) << 16;
								if (ptr != ptr2 && *ptr == ')')
								{
									color = fgcolor;
									ocolor = olcolor;
								}
							}
						}
					}
					else if (ptr2 != ptr && *ptr2 == ')')
					{
						color = fgcolor;
					}
				}
			}
		}
	}

#ifdef GDIPLUS
	BOOL bShitFixed = FALSE;
	Color bgcolor(swap_color(p_config.bgcolor) | (p_config.alphaback << 24));

	if (p_config.alphaback == 0)
	{
		bgcolor.SetValue(swap_color(p_config.bgcolor) | (1 << 24));
		bShitFixed = TRUE;
	}
#endif	

	HDC hDC = CreateCompatibleDC(dDC);
	HBITMAP pOldBmp = (HBITMAP) SelectObject(hDC, m_hBitmap);
#ifdef GDIPLUS
	{
		float width = (float)m_sSize.cx - 2.f;
		float height = (float)m_sSize.cy - 2.f;
		float bracket_width = (height + 1.f) * .5f;
		float bar_width = (width - bracket_width * 2.f) * .99f;
		float line_width = bar_width / (float)vsteps;
		float line_space = line_width * .49f;
		line_width -= line_space;
		line_space *= .5f;

		Graphics graphics(hDC);
		GraphicsPath path;
		graphics.Clear(bgcolor);
		graphics.SetSmoothingMode(SmoothingModeAntiAlias);

		PointF bracket[] = { PointF(bracket_width + 1.f,1.f),
			PointF(1.f, height * .5f),
			PointF(bracket_width + 1.f, height - 1.f),
			PointF(bracket_width + 1.f, height - line_width - 1.f),
			PointF(line_width + 1.f, height * .5f),
			PointF(bracket_width + 1.f, line_width + 1.f)
		};

		if (volume > vmin) path.AddPolygon(bracket, 6);

		RectF line((width - bar_width) * .5f + line_space, 0.f, line_width, 0.f);

		for (int i = 0; i < vsteps; i++)
		{
			if (i == volq)
			{
				line.Y = 1.f;
				line.Height = height - 1.f;
			}
			else
			{
				line.Y = height - line_width;
				line.Height = line_width;
			}

			path.AddRectangle(line);

			line.X += line_space * 2.f + line_width;
		}

		if (volume < 0)
		{
			for (int i = 0; i < 6; i++)
			{
				bracket[i].X = width - bracket[i].X;
			}
			path.AddPolygon(bracket, 6);
		}

		SolidBrush brush(0);

		if (p_config.flags & osd_outline)
		{
			Pen temp(Color(0), 2.5f);
			GraphicsPath * outline = path.Clone();
			outline->Widen(&temp);
			brush.SetColor(Color(swap_color(ocolor) | (255 << 24)));
			graphics.FillPath(&brush, outline);
			delete outline;
		}

		brush.SetColor(Color(swap_color(color) | (255 << 24)));
		graphics.FillPath(&brush, &path);
	}
#else
	{
		// simple at first...
		bool outline = !!(p_config.flags & osd_outline);
		volq = (volume * (m_sSize.cx - 2)) / vmin;
		if (outline)
		{
			unsigned octemp = (ocolor | (255 << 24));
			unsigned * ptr = (unsigned*) gfx;
			unsigned * ptr2 = ptr + m_sSize.cx * (m_sSize.cy - 1);
			for (int x = m_sSize.cx; x--;)
			{
				*ptr++ = octemp;
				*ptr2++ = octemp;
			}
			ptr = ((unsigned*) gfx) + m_sSize.cx;
			for (int y = m_sSize.cy - 2; y--;)
			{
				*ptr = octemp;
				ptr += m_sSize.cx;
				ptr[-1] = octemp;
			}
		}
		unsigned col1 = (((color & 255) * 64 / 255) & 255) | (((color & 0xFF00) * 64 / 255) & 0xFF00) | (((color & 0xFF0000) * 64 / 255) & 0xFF0000) | (64 << 24);
		unsigned col2 = (((color & 255) * 192 / 255) & 255) | (((color & 0xFF00) * 192 / 255) & 0xFF00) | (((color & 0xFF0000) * 192 / 255) & 0xFF0000) | (192 << 24);
		unsigned * ptr = ((unsigned*) gfx) + m_sSize.cx + 1;
		for (int y = m_sSize.cy - 2; y--;)
		{
			for (int x = m_sSize.cx - 2 - volq; x--;)
			{
				*ptr++ = col2;
			}
			for (int x = volq; x--;)
			{
				*ptr++ = col1;
			}
			ptr += 2;
		}
	}
#endif

	SelectObject(hDC, pOldBmp);
	DeleteDC(hDC);
	ReleaseDC(NULL, dDC);

#ifdef GDIPLUS
	// Oh boy, shit to clean up due to bug in renderer
	for (int y = 0; y < m_sSize.cy; y++)
	{
		for (int x = 0; x < m_sSize.cx; x++)
		{
			if (gfx[3] == 0) gfx[3] = 255;
			else if (bShitFixed && gfx[3] == 1) gfx[3] = 0;
			gfx += 4;
		}
	}
#endif
	return true;
}

string_utf8_nocolor::string_utf8_nocolor(const char * src, int len /* = -1 */)
{
	string8 meh(src, len);
	if (meh.is_empty()) return;
	if (strchr(meh, 3))
		titleformat_compiler::remove_color_marks(meh, *this);
	else
		set_string(meh);
}
