#include <foobar2000.h>

#include "resource.h"

#include "loadpic.h"

#include <atlbase.h>
#include <atlwin.h>

class GEPCtrl : public CWindowImpl<GEPCtrl>
{
	unsigned frame;

	IPicture * logos[2];

	POINT ptSize;
	HBITMAP backbuffer;
	HBITMAP frames[2];

	BEGIN_MSG_MAP( GEPCtrl )
		MESSAGE_HANDLER( WM_CREATE, OnCreate )
		MESSAGE_HANDLER( WM_TIMER, OnTimer )
		MESSAGE_HANDLER( WM_PAINT, OnPaint )
		MESSAGE_HANDLER( WM_DESTROY, OnDestroy )
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		HMODULE hInst = core_api::get_my_instance();
		
		frame = 0;
		logos[0] = LoadPic(MAKEINTRESOURCE(IDB_LOGO1), _T("JPG"), hInst);
		logos[1] = LoadPic(MAKEINTRESOURCE(IDB_LOGO2), _T("JPG"), hInst);
		
		IPicture_getHandle(logos[0], (OLE_HANDLE *)&frames[0]);
		IPicture_getHandle(logos[1], (OLE_HANDLE *)&frames[1]);

		BITMAP bm;
		GetObject( frames[0], sizeof(bm), &bm );
		ptSize.x = bm.bmWidth;
		ptSize.y = bm.bmHeight;

		BITMAPINFOHEADER bmih;
		bmih.biSize = sizeof(bmih);
		bmih.biWidth = bm.bmWidth;
		bmih.biHeight = bm.bmHeight;
		bmih.biPlanes = bm.bmPlanes;
		bmih.biBitCount = bm.bmBitsPixel;
		bmih.biCompression = BI_RGB;
		bmih.biSizeImage = 0;
		bmih.biXPelsPerMeter = 0;
		bmih.biYPelsPerMeter = 0;
		bmih.biClrUsed = 0;
		bmih.biClrImportant = 0;

		BYTE * gfx;
		backbuffer = CreateDIBSection(0, (const BITMAPINFO *) &bmih, DIB_RGB_COLORS, (VOID **) &gfx, 0, 0);

		SetWindowPos( 0, 0, 0, ptSize.x, ptSize.y, SWP_NOMOVE | SWP_NOREDRAW | SWP_NOZORDER );

		SetTimer( 0, 10 );

		return 0;
	}

	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		frame++;
		if (frame >= 1000) frame = 0;
		if (!frame ||
			(frame - 400 <= 100) ||
			(frame >= 900))
		{
			RECT rc;
			GetClientRect( &rc );
			InvalidateRect( &rc, FALSE );
		}

		return 0;
	}

	LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint( &ps );

		HDC hdcMem, hdcBB;
		HBITMAP hbmOld, hbmOldBB;
		BLENDFUNCTION func = { AC_SRC_OVER, 0, 0, 0 };

		ps.rcPaint.right -= ps.rcPaint.left;
		ps.rcPaint.bottom -= ps.rcPaint.top;

		if (frame < 400)
		{
			hdcMem = CreateCompatibleDC( hdc );
			hbmOld = (HBITMAP) SelectObject( hdcMem, frames[0] );

			BitBlt( hdc, ps.rcPaint.left, ps.rcPaint.top,
				ps.rcPaint.right, ps.rcPaint.bottom, hdcMem,
				ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);

			SelectObject( hdcMem, hbmOld );
			DeleteDC( hdcMem );
		}
		else if (frame < 500)
		{
			hdcBB = CreateCompatibleDC( hdc );
			hbmOldBB = (HBITMAP) SelectObject( hdcBB, backbuffer );

			hdcMem = CreateCompatibleDC( hdc );
			hbmOld = (HBITMAP) SelectObject( hdcMem, frames[0] );

			BitBlt( hdcBB, ps.rcPaint.left, ps.rcPaint.top,
				ps.rcPaint.right, ps.rcPaint.bottom, hdcMem,
				ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);

			SelectObject( hdcMem, frames[1] );

			func.SourceConstantAlpha = (frame - 400) * 255 / 100;

			AlphaBlend( hdcBB, ps.rcPaint.left, ps.rcPaint.top,
				ps.rcPaint.right, ps.rcPaint.bottom, hdcMem,
				ps.rcPaint.left, ps.rcPaint.top,
				ps.rcPaint.right, ps.rcPaint.bottom, func);

			SelectObject( hdcMem, hbmOld );
			DeleteDC( hdcMem );

			BitBlt( hdc, ps.rcPaint.left, ps.rcPaint.top,
				ps.rcPaint.right, ps.rcPaint.bottom, hdcBB,
				ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);

			SelectObject( hdcBB, hbmOldBB );
			DeleteDC( hdcBB );
		}
		else if (frame < 900)
		{
			hdcMem = CreateCompatibleDC( hdc );
			hbmOld = (HBITMAP) SelectObject( hdcMem, frames[1] );

			BitBlt( hdc, ps.rcPaint.left, ps.rcPaint.top,
				ps.rcPaint.right, ps.rcPaint.bottom, hdcMem,
				ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);

			SelectObject( hdcMem, hbmOld );
			DeleteDC( hdcMem );
		}
		else if (frame < 1000)
		{
			hdcBB = CreateCompatibleDC( hdc );
			hbmOldBB = (HBITMAP) SelectObject( hdcBB, backbuffer );

			hdcMem = CreateCompatibleDC( hdc );
			hbmOld = (HBITMAP) SelectObject( hdcMem, frames[1] );

			BitBlt( hdcBB, ps.rcPaint.left, ps.rcPaint.top,
				ps.rcPaint.right, ps.rcPaint.bottom, hdcMem,
				ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);

			SelectObject( hdcMem, frames[0] );

			func.SourceConstantAlpha = (frame - 900) * 255 / 100;

			AlphaBlend( hdcBB, ps.rcPaint.left, ps.rcPaint.top,
				ps.rcPaint.right, ps.rcPaint.bottom, hdcMem,
				ps.rcPaint.left, ps.rcPaint.top,
				ps.rcPaint.right, ps.rcPaint.bottom, func);

			SelectObject( hdcMem, hbmOld );
			DeleteDC( hdcMem );

			BitBlt( hdc, ps.rcPaint.left, ps.rcPaint.top,
				ps.rcPaint.right, ps.rcPaint.bottom, hdcBB,
				ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);

			SelectObject( hdcBB, hbmOldBB );
			DeleteDC( hdcBB );
		}

		EndPaint( &ps );

		return 0;
	}

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		KillTimer( 0 );
		IPicture_Release(logos[0]);
		IPicture_Release(logos[1]);
		DeleteObject(backbuffer);

		return 0;
	}

	void OnFinalMessage( HWND )
	{
		delete this;
	}

public:
	DECLARE_WND_CLASS(NULL);

	GEPCtrl( HWND p_parent, RECT & p_rect )
	{
		Create( p_parent, p_rect );
	}
};

bool CreateLogo( HWND parent, int x, int y )
{
	RECT r;
	r.left = x; r.top = y;
	r.right = 0; r.bottom = 0;
	return !! new GEPCtrl( parent, r );
}

