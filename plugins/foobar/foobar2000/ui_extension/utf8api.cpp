#include "utf8api.h"

#include "../shared/shared.h"

class param_utf16_from_utf8 : public string_utf16_from_utf8
{
	bool is_null;
	WORD low_word;
public:
	param_utf16_from_utf8(const char * p) : 
		is_null(p==0), 
		low_word( HIWORD((DWORD)p)==0 ? LOWORD((DWORD)p) : 0),
		string_utf16_from_utf8( p && HIWORD((DWORD)p)!=0 ?p:"") 
		{}
	inline operator const WCHAR *()
	{
		return get_ptr();
	}
	const WCHAR * get_ptr()
	{
		return low_word ? (const WCHAR*)(DWORD)low_word : is_null ? 0 : string_utf16_from_utf8::get_ptr();
	}
	
};

class param_ansi_from_utf8 : public string_ansi_from_utf8
{
	bool is_null;
	WORD low_word;
public:
	param_ansi_from_utf8(const char * p) : 
		is_null(p==0), 
		low_word( HIWORD((DWORD)p)==0 ? LOWORD((DWORD)p) : 0),
		string_ansi_from_utf8( p && HIWORD((DWORD)p)!=0 ?p:"") 
		{}
	inline operator const char *()
	{
		return get_ptr();
	}
	const char * get_ptr()
	{
		return low_word ? (const char*)(DWORD)low_word : is_null ? 0 : string_ansi_from_utf8::get_ptr();
	}
	
};

int uHeader_InsertItem(HWND wnd, int n, uHDITEM * hdi, bool insert)
{
	if (IsUnicode())
	{
		param_utf16_from_utf8 text((hdi->mask & HDI_TEXT) ? hdi->pszText : 0);
		HDITEMW hdw;
		hdw.cchTextMax = hdi->cchTextMax;
		hdw.cxy = hdi->cxy;
		hdw.fmt = hdi->fmt;
		hdw.hbm = hdi->hbm;
		hdw.iImage = hdi->iImage;
		hdw.iOrder = hdi->iOrder;
		hdw.lParam = hdi->lParam;
		hdw.mask = hdi->mask;
		hdw.pszText = const_cast<WCHAR *>(text.get_ptr());

		return uSendMessage(wnd, insert ? HDM_INSERTITEMW : HDM_SETITEMW, n, (LPARAM)&hdw);
		
	}
	else
	{
		param_ansi_from_utf8 text((hdi->mask & HDI_TEXT) ? hdi->pszText : 0);

		HDITEMA hda;
		hda.cchTextMax = hdi->cchTextMax;
		hda.cxy = hdi->cxy;
		hda.fmt = hdi->fmt;
		hda.hbm = hdi->hbm;
		hda.iImage = hdi->iImage;
		hda.iOrder = hdi->iOrder;
		hda.lParam = hdi->lParam;
		hda.mask = hdi->mask;
		hda.pszText = const_cast<char *>(text.get_ptr());

		return uSendMessage(wnd, insert ? HDM_INSERTITEMA : HDM_SETITEMA, n, (LPARAM)&hda);
	}
}

BOOL uRebar_InsertItem(HWND wnd, int n, uREBARBANDINFO * rbbi, bool insert)
{
	BOOL rv = FALSE;
	if (IsUnicode())
	{
		param_utf16_from_utf8 text((rbbi->fMask & RBBIM_TEXT) ? rbbi->lpText : 0);

		REBARBANDINFOW  rbw;
		rbw.cbSize       = sizeof(REBARBANDINFOW);

		rbw.fMask = rbbi->fMask;
		rbw.fStyle = rbbi->fStyle;
		rbw.clrFore = rbbi->clrFore;
		rbw.clrBack = rbbi->clrBack;
		rbw.lpText = const_cast<WCHAR *>(text.get_ptr());;
		rbw.cch = rbbi->cch;
		rbw.iImage = rbbi->iImage;
		rbw.hwndChild = rbbi->hwndChild;
		rbw.cxMinChild = rbbi->cxMinChild;
		rbw.cyMinChild = rbbi->cyMinChild;
		rbw.cx = rbbi->cx;
		rbw.hbmBack = rbbi->hbmBack;
		rbw.wID = rbbi->wID;
	#if (_WIN32_IE >= 0x0400) //we should check size of structure instead so keeping compatibility is possible, but whatever
		rbw.cyChild = rbbi->cyChild;
		rbw.cyMaxChild = rbbi->cyMaxChild;
		rbw.cyIntegral = rbbi->cyIntegral;
		rbw.cxIdeal = rbbi->cxIdeal;
		rbw.lParam = rbbi->lParam;
		rbw.cxHeader = rbbi->cxHeader;
	#endif
		
		rv = uSendMessage(wnd, insert ? RB_INSERTBANDW : RB_SETBANDINFOW, n, (LPARAM)&rbw);

	}
	else
	{
		param_ansi_from_utf8 text((rbbi->fMask & RBBIM_TEXT) ? rbbi->lpText : 0);

		REBARBANDINFOA  rba;
		rba.cbSize       = sizeof(REBARBANDINFOA);

		rba.fMask = rbbi->fMask;
		rba.fStyle = rbbi->fStyle;
		rba.clrFore = rbbi->clrFore;
		rba.clrBack = rbbi->clrBack;
		rba.lpText = const_cast<char *>(text.get_ptr());;
		rba.cch = rbbi->cch;
		rba.iImage = rbbi->iImage;
		rba.hwndChild = rbbi->hwndChild;
		rba.cxMinChild = rbbi->cxMinChild;
		rba.cyMinChild = rbbi->cyMinChild;
		rba.cx = rbbi->cx;
		rba.hbmBack = rbbi->hbmBack;
		rba.wID = rbbi->wID;
	#if (_WIN32_IE >= 0x0400)
		rba.cyChild = rbbi->cyChild;
		rba.cyMaxChild = rbbi->cyMaxChild;
		rba.cyIntegral = rbbi->cyIntegral;
		rba.cxIdeal = rbbi->cxIdeal;
		rba.lParam = rbbi->lParam;
		rba.cxHeader = rbbi->cxHeader;
	#endif
		
		rv = uSendMessage(wnd, insert ? RB_INSERTBANDA : RB_SETBANDINFOA, n, (LPARAM)&rba);

	}
	return rv;
}

BOOL uToolTip_AddTool(HWND wnd, uTOOLINFO * ti, bool update)
{
	BOOL rv = FALSE;
	if (IsUnicode())
	{
		param_utf16_from_utf8 text(ti->lpszText);
		TOOLINFOW tiw;
		
		tiw.cbSize = sizeof(tiw);
		tiw.uFlags = ti->uFlags;
		tiw.hwnd = ti->hwnd;
		tiw.uId = ti->uId;
		tiw.rect = ti->rect;
		tiw.hinst = ti->hinst;
		tiw.lParam = ti->lParam;
		tiw.lpszText = const_cast<WCHAR*>(text.get_ptr());

		rv = uSendMessage(wnd, update ? TTM_UPDATETIPTEXTW : TTM_ADDTOOLW, 0, (LPARAM)  &tiw);
	}
	else
	{
		param_ansi_from_utf8 text(ti->lpszText);
		TOOLINFOA tia;
		
		tia.cbSize = sizeof(tia);
		tia.uFlags = ti->uFlags;
		tia.hwnd = ti->hwnd;
		tia.uId = ti->uId;
		tia.rect = ti->rect;
		tia.hinst = ti->hinst;
		tia.lParam = ti->lParam;
		tia.lpszText = const_cast<char*>(text.get_ptr());

		rv = uSendMessage(wnd, update ? TTM_UPDATETIPTEXTA : TTM_ADDTOOLA, 0, (LPARAM)  &tia);
	}
	return rv;
}

BOOL uTabCtrl_InsertItemText(HWND wnd, int idx, const char * text, bool insert)
{
	string8 temp2;
	uTCITEM tabs;
	memset(&tabs, 0, sizeof(uTCITEM));
	tabs.mask = TCIF_TEXT;
	uFixAmpersandChars_v2(text, temp2);
	tabs.pszText = const_cast<char *>(temp2.get_ptr());
	return insert ? uTabCtrl_InsertItem(wnd, idx, &tabs) : uTabCtrl_SetItem(wnd, idx, &tabs);
}

BOOL uComboBox_GetText(HWND combo,UINT index,string_base & out)
{
	unsigned len = uSendMessage(combo,CB_GETLBTEXTLEN,index,0);
	if (len==CB_ERR || len>16*1024*1024) return FALSE;
	if (len==0) {out.reset();return TRUE;}

	if (IsUnicode())
	{
		mem_block_t<WCHAR> temp(len+1);
		if (temp.get_ptr()==0) return FALSE;
		temp.zeromemory();
		len = uSendMessage(combo,CB_GETLBTEXT,index,(LPARAM)temp.get_ptr());
		if (len==CB_ERR) return false;
		out.set_string(string_utf8_from_utf16(temp));
	}
	else
	{
		mem_block_t<char> temp(len+1);
		if (temp.get_ptr()==0) return FALSE;
		temp.zeromemory();
		len = uSendMessage(combo,CB_GETLBTEXT,index,(LPARAM)temp.get_ptr());
		if (len==CB_ERR) return false;
		out.set_string(string_utf8_from_ansi(temp));
	}
	return TRUE;
}

BOOL uComboBox_SelectString(HWND combo,const char * src)
{
	unsigned idx = CB_ERR;
	idx = uSendMessageText(combo,CB_FINDSTRINGEXACT,-1,src);
	if (idx==CB_ERR) return false;
	uSendMessage(combo,CB_SETCURSEL,idx,0);
	return TRUE;
}


BOOL uStatus_SetText(HWND wnd,int part,const char * text)
{
	BOOL rv = 0;

	if (IsUnicode())
	{
		rv = uSendMessage(wnd,SB_SETTEXTW,part,(LPARAM)(param_utf16_from_utf8(text).get_ptr()));
	}
	else
	{
		rv = uSendMessage(wnd,SB_SETTEXTA,part,(LPARAM)(param_ansi_from_utf8(text).get_ptr()));
	}

//	rv = uSendMessageText(wnd, IsUnicode() ? SB_SETTEXTW : SB_SETTEXTA, part, text);
	return rv;
}

HFONT uCreateIconFont()
{
		HFONT fnt_menu = 0;
		
		if (IsUnicode())
		{
			LOGFONTW lf;
			memset(&lf, 0, sizeof(LOGFONTW));
			SystemParametersInfoW(SPI_GETICONTITLELOGFONT, 0, &lf, 0);
			
			fnt_menu = CreateFontIndirectW(&lf);
		}
		else
		{
			LOGFONTA lf;
			memset(&lf, 0, sizeof(LOGFONTA));
			SystemParametersInfoA(SPI_GETICONTITLELOGFONT, 0, &lf, 0);
			
			fnt_menu = CreateFontIndirectA(&lf);
		}

		return fnt_menu;
		
}

HFONT uCreateMenuFont()
{
		HFONT fnt_menu = 0;
		
		if (IsUnicode())
		{
			NONCLIENTMETRICSW ncm;
			memset(&ncm, 0, sizeof(NONCLIENTMETRICSW));
			ncm.cbSize = sizeof(NONCLIENTMETRICSW);
			SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
			
			fnt_menu = CreateFontIndirectW(&ncm.lfMenuFont);
		}
		else
		{
			NONCLIENTMETRICSA ncm;
			memset(&ncm, 0, sizeof(NONCLIENTMETRICSA));
			ncm.cbSize = sizeof(NONCLIENTMETRICSA);
			SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
			
			fnt_menu = CreateFontIndirectA(&ncm.lfMenuFont);
		}

		return fnt_menu;
}

BOOL uFormatMessage(DWORD dw_error, string_base & out, HANDLE hModule)
{
	BOOL rv = FALSE;
	PVOID buffer;
	if (IsUnicode())
	{
		if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | (hModule ? FORMAT_MESSAGE_FROM_HMODULE : FORMAT_MESSAGE_FROM_SYSTEM), hModule, dw_error, 0, (LPWSTR)&buffer, 256, 0))
		{
			out.set_string_utf16((const WCHAR *)buffer);
			LocalFree(buffer);
		}
	}
	else
	{
		if (FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | (hModule ? FORMAT_MESSAGE_FROM_HMODULE : FORMAT_MESSAGE_FROM_SYSTEM), hModule, dw_error, 0, (LPSTR)&buffer, 256, 0))
		{
			out.set_string_ansi((const char *)buffer);
			LocalFree(buffer);
		}
	}
	return rv;
}


DWORD uSetClassLong(HWND wnd, int index, long new_long)
{
	if (IsUnicode())
	{
		return SetClassLongW(wnd,index, new_long);
	} 
	else
	{
		return SetClassLongA(wnd,index, new_long);
	} 
}

DWORD uGetClassLong(HWND wnd, int index)
{
	if (IsUnicode())
	{
		return GetClassLongW(wnd,index);
	} 
	else
	{
		return GetClassLongA(wnd,index);
	} 
}

#if(WINVER >= 0x0500)
HWND uRecursiveChildWindowFromPoint(HWND parent, POINT pt_parent)
{
	HWND wnd = RealChildWindowFromPoint(parent, pt_parent);
	if (wnd && wnd != parent)
	{
		HWND wnd_last = wnd;
		POINT pt = pt_parent;
		MapWindowPoints(parent, wnd_last, &pt, 1);
		for (;;)
		{
			wnd = RealChildWindowFromPoint(wnd_last, pt);
			if (!wnd) return 0;
			if (wnd == wnd_last) return wnd;
			MapWindowPoints(wnd_last, wnd, &pt, 1);
			wnd_last = wnd;
		}
	}
	return 0;
}

BOOL uGetMonitorInfo(HMONITOR monitor, LPMONITORINFO lpmi)
{
	if (IsUnicode())
		return GetMonitorInfoW(monitor, lpmi);
	else
		return GetMonitorInfoA(monitor, lpmi);
}
#endif