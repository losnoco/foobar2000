#include "stdafx.h"


#define uTEXT(blah) TEXT(blah)
#define uLVM_SETITEM LVM_SETITEM
#define uLVM_INSERTITEM LVM_INSERTITEM
#define uLVM_INSERTCOLUMN LVM_INSERTCOLUMN
#define uLVM_GETITEM LVM_GETITEM

namespace listview_helper {

	unsigned insert_item(HWND p_listview,unsigned p_index,const char * p_name,LPARAM p_param)
	{
		LVITEM item;
		memset(&item,0,sizeof(item));

		pfc::stringcvt::string_os_from_utf8 os_string_temp;
		if (!os_string_temp.convert(p_name)) return infinite;

		item.mask = LVIF_TEXT | LVIF_PARAM;
		item.iItem = p_index;
		item.lParam = p_param;
		item.pszText = const_cast<TCHAR*>(os_string_temp.get_ptr());
		
		int ret = uSendMessage(p_listview,uLVM_INSERTITEM,0,(LPARAM)&item);
		if (ret < 0) return infinite;
		else return (unsigned) ret;
	}



	unsigned insert_column(HWND p_listview,unsigned p_index,const char * p_name,unsigned p_width_dlu)
	{
		pfc::stringcvt::string_os_from_utf8 os_string_temp;
		if (!os_string_temp.convert(p_name)) return infinite;

		RECT rect = {0,0,p_width_dlu,0};
		MapDialogRect(GetParent(p_listview),&rect);

		LVCOLUMN data;
		memset(&data,0,sizeof(data));
		data.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
		data.fmt = LVCFMT_LEFT;
		data.cx = rect.right;
		data.pszText = const_cast<TCHAR*>(os_string_temp.get_ptr());
		
		int ret = uSendMessage(p_listview,uLVM_INSERTCOLUMN,p_index,(LPARAM)&data);
		if (ret < 0) return infinite;
		else return (unsigned) ret;
	}

	bool set_item_text(HWND p_listview,unsigned p_index,unsigned p_column,const char * p_name)
	{
		LVITEM item;
		memset(&item,0,sizeof(item));

		pfc::stringcvt::string_os_from_utf8 os_string_temp;
		if (!os_string_temp.convert(p_name)) return false;

		item.mask = LVIF_TEXT;
		item.iItem = p_index;
		item.iSubItem = p_column;
		item.pszText = const_cast<TCHAR*>(os_string_temp.get_ptr());
		return uSendMessage(p_listview,uLVM_SETITEM,0,(LPARAM)&item) ? true : false;
	}

	bool is_item_selected(HWND p_listview,unsigned p_index)
	{
		LVITEM item;
		memset(&item,0,sizeof(item));
		item.mask = LVIF_STATE;
		item.iItem = p_index;
		item.stateMask = LVIS_SELECTED;
		if (!uSendMessage(p_listview,uLVM_GETITEM,0,(LPARAM)&item)) return false;
		return (item.state & LVIS_SELECTED) ? true : false;
	}

	bool set_item_selection(HWND p_listview,unsigned p_index,bool p_state)
	{
		LVITEM item;
		memset(&item,0,sizeof(item));
		item.mask = LVIF_STATE;
		item.iItem = p_index;
		item.stateMask = LVIS_SELECTED;
		item.state = p_state ? LVIS_SELECTED : 0;
		return uSendMessage(p_listview,uLVM_SETITEM,0,(LPARAM)&item) ? true : false;
	}

	bool select_single_item(HWND p_listview,unsigned p_index)
	{
		unsigned n; const unsigned m = uSendMessage(p_listview,LVM_GETITEMCOUNT,0,0);
		if (m == infinite) return false;
		for(n=0;n<m;n++)
		{
			if (!set_item_selection(p_listview,n,p_index == n)) return false;
		}
		return ensure_visible(p_listview,p_index);
	}

	bool ensure_visible(HWND p_listview,unsigned p_index)
	{
		return uSendMessage(p_listview,LVM_ENSUREVISIBLE,p_index,FALSE) ? true : false;
	}
}