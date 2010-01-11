#include "../SDK/foobar2000.h"

unsigned convert_utf16_to_korean(const WCHAR * src,char * dst,unsigned len)
{
	len = wcslen_max(src,len);
	unsigned rv;
#ifdef WIN32
	rv = WideCharToMultiByte(949,0,src,len,dst,len * 2 + 1,0,0);
#else
	setlocale(LC_CTYPE,"");
	rv = wcstombs(dst,src,len);
#endif
	if ((signed)rv<0) rv = 0;
	dst[rv]=0;
	return rv;
}

class string_korean_from_utf16 : public string_convert_base<char>
{
public:
	explicit string_korean_from_utf16(const WCHAR * src,unsigned len = -1)
	{
		len = wcslen_max(src,len);
		alloc(len * 2 + 1);
		convert_utf16_to_korean(src,ptr,len);
	}
};

class menu_clip : public menu_item
{
	virtual type get_type()
	{
		return TYPE_MAIN;
	}

	virtual unsigned get_num_items()
	{
		return 1;
	}

	virtual void enum_item(unsigned n, string_base & out)
	{
		out = "Components/Convert clipboard text to Korean codepage";
	}

	virtual enabled_state get_enabled_state(unsigned idx)
	{
		return DEFAULT_OFF;
	}

	virtual bool get_display_data(unsigned n, const ptr_list_base<metadb_handle> &data, string_base &out, unsigned & displayflags)
	{
		out = "yuo spoony bard";
		return true;
	}

	virtual void perform_command(unsigned n, const ptr_list_base<metadb_handle> &data)
	{
		if (OpenClipboard(0))
		{
			HGLOBAL hglb = GetClipboardData(CF_UNICODETEXT);
			if (hglb != NULL)
			{
				WCHAR * instr = (WCHAR *) GlobalLock(hglb);
				if (instr)
				{
					string_korean_from_utf16 temp(instr);
					GlobalUnlock(instr);
					EmptyClipboard();
					HANDLE h_global = GlobalAlloc(GMEM_DDESHARE, strlen(temp) + 1);
					if (h_global != 0)
					{
						char * ptr = (char *) GlobalLock(h_global);
						strcpy(ptr, temp);
						GlobalUnlock(h_global);
						SetClipboardData(CF_TEXT, h_global);
					}
				}
			}
			CloseClipboard();
		}

	}
};

static menu_item_factory<menu_clip> foo;
