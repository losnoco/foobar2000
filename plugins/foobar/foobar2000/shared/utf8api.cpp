#include "shared.h"
#include "shared_internal.h"

#include <shlobj.h>
#include <lmcons.h>

#ifndef BIF_NEWDIALOGSTYLE
#define BIF_NEWDIALOGSTYLE 0x0040
#endif

using namespace pfc;

class param_os_from_utf8
{
	bool m_is_null;
	WORD m_low_word;
	stringcvt::string_os_from_utf8 m_cvt;
public:
	param_os_from_utf8(const char * p) : 
		m_is_null(p==NULL), 
		m_low_word( ((t_size)p & ~0xFFFF) == 0 ? (WORD)((t_size)p & 0xFFFF) : 0),
		m_cvt( p != NULL && ((t_size)p & ~0xFFFF) != 0 ? p : "") 
		{}
	operator const TCHAR *()
	{
		return get_ptr();
	}
	const TCHAR * get_ptr()
	{
		return m_low_word ? (const TCHAR*)(t_size)m_low_word : m_is_null ? 0 : m_cvt.get_ptr();
	}
	
};


struct browse_for_dir_struct
{
	const TCHAR * m_initval;
	const TCHAR * m_tofind;

	modal_dialog_scope m_scope;
};

static bool file_exists(const TCHAR * p_path)
{
	DWORD val = GetFileAttributes(p_path);
	if (val == (-1) || (val & FILE_ATTRIBUTE_DIRECTORY)) return false;
	return true;
}

static void browse_proc_check_okbutton(HWND wnd,const browse_for_dir_struct * p_struct,const TCHAR * p_path)
{
	TCHAR temp[MAX_PATH+1];
	_tcsncpy(temp,p_path,MAX_PATH);
	temp[MAX_PATH] = 0;

	t_size len = _tcslen(temp);
	if (len < MAX_PATH && len > 0)
	{
		if (temp[len-1] != '\\')
			temp[len++] = '\\';
	}
	t_size idx = 0;
	while(p_struct->m_tofind[idx] && idx+len < MAX_PATH)
	{
		temp[len+idx] = p_struct->m_tofind[idx];
		idx++;
	}
	temp[len+idx] = 0;

	SendMessage(wnd,BFFM_ENABLEOK,0,!!file_exists(temp));

}

static int _stdcall browse_proc(HWND wnd,UINT msg,LPARAM lp,LPARAM dat)
{
	browse_for_dir_struct * p_struct = reinterpret_cast<browse_for_dir_struct*>(dat);
	switch(msg)
	{
	case BFFM_INITIALIZED:
		p_struct->m_scope.initialize(wnd);
		SendMessage(wnd,BFFM_SETSELECTION,1,(LPARAM)p_struct->m_initval);
		if (p_struct->m_tofind) browse_proc_check_okbutton(wnd,p_struct,p_struct->m_initval);
		break;
	case BFFM_SELCHANGED:
		if (p_struct->m_tofind)
		{
			if (lp != 0)
			{
				TCHAR temp[MAX_PATH+1];
				if (SHGetPathFromIDList(reinterpret_cast<const ITEMIDLIST*>(lp),temp))
				{
					temp[MAX_PATH] = 0;
					browse_proc_check_okbutton(wnd,p_struct,temp);
				}
				else
					SendMessage(wnd,BFFM_ENABLEOK,0,FALSE);
			}
			else SendMessage(wnd,BFFM_ENABLEOK,0,FALSE);
		}
		break;
	}
	return 0;
}

static int browse_for_dir(HWND p_parent,const TCHAR * p_title,TCHAR * p_out,const TCHAR * p_file_to_find)
{
	com_ptr_t<IMalloc> mallocptr;
	
	SHGetMalloc((IMalloc**)&mallocptr);
	if (mallocptr.is_empty()) return 0;

	browse_for_dir_struct data;
	data.m_initval = p_out;
	data.m_tofind = p_file_to_find;


	BROWSEINFO bi=
	{
		p_parent,
		0,
		0,
		p_title,
		BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE,
		browse_proc,
		reinterpret_cast<LPARAM>(&data),
		0
	};

	int rv = 0;
	__unaligned ITEMIDLIST* li=SHBrowseForFolder(&bi);
	if (li)
	{
		SHGetPathFromIDList(li,p_out);

		mallocptr->Free(li);
		rv = 1;
	}

	return rv;
}

extern "C" {

LRESULT SHARED_EXPORT uSendMessageText(HWND wnd,UINT msg,WPARAM wp,const char * p_text)
{//this is called somewhat often (playlist search listbox...), so lets avoid mallocing
	if (p_text == NULL)
		return SendMessage(wnd,msg,wp,0);
	else {
		stringcvt::string_os_from_utf8 temp;
		temp.convert(p_text);
		return SendMessage(wnd,msg,wp,(LPARAM)temp.get_ptr());
	}
}

LRESULT SHARED_EXPORT uSendDlgItemMessageText(HWND wnd,UINT id,UINT msg,WPARAM wp,const char * text)
{
	return uSendMessageText(uGetDlgItem(wnd,id),msg,wp,text);//SendDlgItemMessage(wnd,id,msg,wp,(long)(const TCHAR*)string_os_from_utf8(text));
}

BOOL SHARED_EXPORT uGetWindowText(HWND wnd,string_base & out)
{
	int len = GetWindowTextLength(wnd);
	if (len>0)
	{
		len++;
		pfc::array_t<TCHAR> temp;
		temp.set_size(len);
		temp[0]=0;		
		if (GetWindowText(wnd,temp.get_ptr(),len)>0)
		{
			out = stringcvt::string_utf8_from_os(temp.get_ptr(),len);
			return TRUE;
		}
		else return FALSE;
	}
	else
	{
		out.reset();
		return TRUE;
	}
}

BOOL SHARED_EXPORT uSetWindowTextEx(HWND wnd,const char * p_text,unsigned p_text_length)
{
	stringcvt::string_os_from_utf8 temp(p_text,p_text_length);
	return SetWindowText(wnd,temp);
}


BOOL SHARED_EXPORT uGetDlgItemText(HWND wnd,UINT id,string_base & out)
{
	return uGetWindowText(GetDlgItem(wnd,id),out);
}

BOOL SHARED_EXPORT uSetDlgItemTextEx(HWND wnd,UINT id,const char * p_text,unsigned p_text_length)
{
	stringcvt::string_os_from_utf8 temp(p_text,p_text_length);
	return SetDlgItemText(wnd,id,temp);
}

BOOL SHARED_EXPORT uBrowseForFolder(HWND parent,const char * p_title,string_base & p_out)
{
	TCHAR temp[MAX_PATH];
	tcsncpy_addnull(temp,stringcvt::string_os_from_utf8(p_out),tabsize(temp));
	BOOL rv = browse_for_dir(parent,stringcvt::string_os_from_utf8(p_title),temp,0);
	if (rv)
	{
		p_out.set_string(stringcvt::string_utf8_from_os(temp,tabsize(temp)));
	}
	return rv;
}

BOOL SHARED_EXPORT uBrowseForFolderWithFile(HWND parent,const char * title,string_base & out,const char * p_file_to_find)
{
	TCHAR temp[MAX_PATH];
	tcsncpy_addnull(temp,stringcvt::string_os_from_utf8(out),tabsize(temp));
	BOOL rv = browse_for_dir(parent,stringcvt::string_os_from_utf8(title),temp,stringcvt::string_os_from_utf8(p_file_to_find));
	if (rv)
	{
		out.set_string(stringcvt::string_utf8_from_os(temp,tabsize(temp)));
	}
	return rv;
}

int SHARED_EXPORT uMessageBox(HWND wnd,const char * text,const char * caption,UINT type)
{
	modal_dialog_scope scope(wnd);
	return MessageBox(wnd,param_os_from_utf8(text),param_os_from_utf8(caption),type);
}

void SHARED_EXPORT uOutputDebugString(const char * msg) {OutputDebugString(stringcvt::string_os_from_utf8(msg));}

BOOL SHARED_EXPORT uAppendMenu(HMENU menu,UINT flags,UINT_PTR id,const char * content)
{
	return AppendMenu(menu,flags,id,param_os_from_utf8(content));
}

BOOL SHARED_EXPORT uInsertMenu(HMENU menu,UINT position,UINT flags,UINT_PTR id,const char * content)
{
	return InsertMenu(menu,position,flags,id,param_os_from_utf8(content));
}

int SHARED_EXPORT uStringCompare(const char * elem1, const char * elem2)
{
#ifdef UNICODE
	wchar_t temp1[4],temp2[4];
	for(;;)
	{
		unsigned c1,c2; t_size l1,l2;
		l1 = utf8_decode_char(elem1,&c1);
		l2 = utf8_decode_char(elem2,&c2);
		if (l1==0 && l2==0) return 0;
		if (c1!=c2)
		{
			temp1[utf16_encode_char(c1,temp1)]=0;
			temp2[utf16_encode_char(c2,temp2)]=0;
			int test = lstrcmpiW(temp1,temp2);
			if (test) return test;
		}
		elem1 += l1;
		elem2 += l2;
	}
#else
	wchar_t temp1[4],temp2[4];
	char ctemp1[20],ctemp2[20];
	for(;;)
	{
		unsigned c1,c2; t_size l1,l2;
		l1 = utf8_decode_char(elem1,&c1);
		l2 = utf8_decode_char(elem2,&c2);
		if (l1==0 && l2==0) return 0;
		if (c1!=c2)
		{
			temp1[utf16_encode_char(c1,temp1)]=0;
			temp2[utf16_encode_char(c2,temp2)]=0;
			WideCharToMultiByte(CP_ACP,0,temp1,-1,ctemp1,tabsize(ctemp1),0,0);
			WideCharToMultiByte(CP_ACP,0,temp2,-1,ctemp2,tabsize(ctemp2),0,0);
			int test = lstrcmpiA(ctemp1,ctemp2);
			if (test) return test;
		}
		elem1 += l1;
		elem2 += l2;
	}
#endif
}

HINSTANCE SHARED_EXPORT uLoadLibrary(const char * name)
{
	return LoadLibrary(param_os_from_utf8(name));
}

HANDLE SHARED_EXPORT uCreateEvent(LPSECURITY_ATTRIBUTES lpEventAttributes,BOOL bManualReset,BOOL bInitialState, const char * lpName)
{
	return CreateEvent(lpEventAttributes,bManualReset,bInitialState, param_os_from_utf8(lpName));
}


DWORD SHARED_EXPORT uGetModuleFileName(HMODULE hMod,string_base & out)
{
	TCHAR temp[MAX_PATH];
	temp[0] = 0;
	DWORD ret = GetModuleFileName(hMod,temp,tabsize(temp));
	if (ret)
	{
		out = stringcvt::string_utf8_from_os(temp,tabsize(temp));
	}
	return ret;
}

BOOL SHARED_EXPORT uSetClipboardString(const char * ptr)
{
	if (OpenClipboard(0))
	{
		EmptyClipboard();

		stringcvt::string_os_from_utf8 temp(ptr);

		HANDLE h_global = GlobalAlloc(GMEM_DDESHARE, (_tcslen(temp) + 1) * sizeof(TCHAR)); 
		if (h_global!=0)
		{
			TCHAR * ptr = (TCHAR*)GlobalLock(h_global);
			_tcscpy(ptr,temp);
			GlobalUnlock(h_global);
			SetClipboardData(
#ifdef UNICODE
				CF_UNICODETEXT
#else
				CF_TEXT
#endif
				,h_global);
		}
		CloseClipboard();
		return TRUE;
	}
	else return FALSE;
}

BOOL SHARED_EXPORT uGetClassName(HWND wnd,string_base & out)
{
	TCHAR temp[512];
	temp[0]=0;
	if (GetClassName(wnd,temp,tabsize(temp))>0)
	{
		out = stringcvt::string_utf8_from_os(temp,tabsize(temp));
		return TRUE;
	}
	else return FALSE;
}

t_size SHARED_EXPORT uCharLength(const char * src) {return utf8_char_len(src);}

BOOL SHARED_EXPORT uDragQueryFile(HDROP hDrop,UINT idx,string_base & out)
{
	UINT len = DragQueryFile(hDrop,idx,0,0);
	if (len>0 && len!=(UINT)(~0))
	{
		len++;
		array_t<TCHAR> temp;
		temp.set_size(len);
		temp[0] =0 ;
		if (DragQueryFile(hDrop,idx,temp.get_ptr(),len)>0)
		{
			out = stringcvt::string_utf8_from_os(temp.get_ptr(),len);
			return TRUE;
		}
	}
	return FALSE;
}

UINT SHARED_EXPORT uDragQueryFileCount(HDROP hDrop)
{
	return DragQueryFile(hDrop,-1,0,0);
}



BOOL SHARED_EXPORT uGetTextExtentPoint32(HDC dc,const char * text,UINT cb,LPSIZE size)
{
	stringcvt::string_os_from_utf8 temp(text,cb);
	return GetTextExtentPoint32(dc,temp,pfc::downcast_guarded<int>(_tcslen(temp)),size);
}

BOOL SHARED_EXPORT uExtTextOut(HDC dc,int x,int y,UINT flags,const RECT * rect,const char * text,UINT cb,const int * lpdx)
{
	stringcvt::string_os_from_utf8 temp(text,cb);
	return ExtTextOut(dc,x,y,flags,rect,temp,pfc::downcast_guarded<int>(_tcslen(temp)),lpdx);
}

static UINT_PTR CALLBACK choose_color_hook(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			CHOOSECOLOR * cc = reinterpret_cast<CHOOSECOLOR*>(lp);
			reinterpret_cast<modal_dialog_scope*>(cc->lCustData)->initialize(FindOwningPopup(wnd));
		}
		return 0;
	default:
		return 0;
	}
}

BOOL SHARED_EXPORT uChooseColor(DWORD * p_color,HWND parent,DWORD * p_custom_colors)
{
	modal_dialog_scope scope;

	CHOOSECOLOR cc;
	memset(&cc,0,sizeof(cc));
	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = parent;
	cc.rgbResult = *p_color;
	cc.lpCustColors = p_custom_colors;
	cc.Flags = CC_ANYCOLOR|CC_FULLOPEN|CC_RGBINIT|CC_ENABLEHOOK;
	cc.lpfnHook = choose_color_hook;
	cc.lCustData = reinterpret_cast<LPARAM>(&scope);
	BOOL rv = ChooseColor(&cc);
	if (rv)
	{
		*p_color = cc.rgbResult;
		return TRUE;
	}
	else return FALSE;
}

HCURSOR SHARED_EXPORT uLoadCursor(HINSTANCE hIns,const char * name)
{
	return LoadCursor(hIns,param_os_from_utf8(name));
}

HICON SHARED_EXPORT uLoadIcon(HINSTANCE hIns,const char * name)
{
	return LoadIcon(hIns,param_os_from_utf8(name));
}

HMENU SHARED_EXPORT uLoadMenu(HINSTANCE hIns,const char * name)
{
	return LoadMenu(hIns,param_os_from_utf8(name));
}



BOOL SHARED_EXPORT uGetEnvironmentVariable(const char * name,string_base & out)
{
	stringcvt::string_os_from_utf8 name_t(name);
	DWORD size = GetEnvironmentVariable(name_t,0,0);
	if (size>0)
	{
		size++;
		array_t<TCHAR> temp;
		temp.set_size(size);
		temp[0]=0;
		if (GetEnvironmentVariable(name_t,temp.get_ptr(),size)>0)
		{
			out = stringcvt::string_utf8_from_os(temp.get_ptr(),size);
			return TRUE;
		}
	}
	return FALSE;
}

HMODULE SHARED_EXPORT uGetModuleHandle(const char * name)
{
	return GetModuleHandle(param_os_from_utf8(name));
}

UINT SHARED_EXPORT uRegisterWindowMessage(const char * name)
{
	return RegisterWindowMessage(stringcvt::string_os_from_utf8(name));
}

BOOL SHARED_EXPORT uMoveFile(const char * src,const char * dst)
{
	return MoveFile(stringcvt::string_os_from_utf8(src),stringcvt::string_os_from_utf8(dst));
}

BOOL SHARED_EXPORT uDeleteFile(const char * fn)
{
	return DeleteFile(stringcvt::string_os_from_utf8(fn));
}

DWORD SHARED_EXPORT uGetFileAttributes(const char * fn)
{
	return GetFileAttributes(stringcvt::string_os_from_utf8(fn));
}

BOOL SHARED_EXPORT uRemoveDirectory(const char * fn)
{
	return RemoveDirectory(stringcvt::string_os_from_utf8(fn));
}

HANDLE SHARED_EXPORT uCreateFile(const char * fn,DWORD access,DWORD share,LPSECURITY_ATTRIBUTES blah,DWORD creat,DWORD flags,HANDLE tmpl)
{
	return CreateFile(stringcvt::string_os_from_utf8(fn),access,share,blah,creat,flags,tmpl);
}

BOOL SHARED_EXPORT uCreateDirectory(const char * fn,LPSECURITY_ATTRIBUTES blah)
{
	return CreateDirectory(stringcvt::string_os_from_utf8(fn),blah);
}

HANDLE SHARED_EXPORT uCreateMutex(LPSECURITY_ATTRIBUTES blah,BOOL bInitialOwner,const char * name)
{
	return name ? CreateMutex(blah,bInitialOwner,stringcvt::string_os_from_utf8(name)) : CreateMutex(blah,bInitialOwner,0);
}

BOOL SHARED_EXPORT uGetFullPathName(const char * name,string_base & out)
{
	stringcvt::string_os_from_utf8 name_os(name);
	unsigned len = GetFullPathName(name_os,0,0,0);
	if (len==0) return FALSE;
	array_t<TCHAR> temp;
	temp.set_size(len+1);
	TCHAR * blah;
	if (GetFullPathName(name_os,len+1,temp.get_ptr(),&blah)==0) return FALSE;
	out = stringcvt::string_utf8_from_os(temp.get_ptr(),len);
	return TRUE;
}

BOOL SHARED_EXPORT uGetLongPathName(const char * name,string_base & out)
{
	static bool inited;
	typedef DWORD (_stdcall * PGETLONGPATHNAME)(LPCTSTR lpszShortPath, LPTSTR lpszLongPath, DWORD cchBuffer);
	static PGETLONGPATHNAME pGetLongPathName;
	if (!inited)
	{
		pGetLongPathName = (PGETLONGPATHNAME)GetProcAddress(uGetModuleHandle("kernel32.dll"),
#ifdef UNICODE
			"GetLongPathNameW"
#else
			"GetLongPathNameA"
#endif
			);
		inited = true;
	}
	if (pGetLongPathName)
	{
		TCHAR temp[4096];
		temp[0]=0;
		if (pGetLongPathName(stringcvt::string_os_from_utf8(name),temp,tabsize(temp)))
		{
			out = stringcvt::string_utf8_from_os(temp,tabsize(temp));
			return TRUE;
		}
	}
	return FALSE;
}

void SHARED_EXPORT uGetCommandLine(string_base & out)
{
	out = stringcvt::string_utf8_from_os(GetCommandLine());
}

BOOL SHARED_EXPORT uGetTempPath(string_base & out)
{
	TCHAR temp[MAX_PATH+1];
	temp[0]=0;
	if (GetTempPath(tabsize(temp),temp))
	{
		out = stringcvt::string_utf8_from_os(temp,tabsize(temp));
		return TRUE;
	}
	return FALSE;

}
BOOL SHARED_EXPORT uGetTempFileName(const char * path_name,const char * prefix,UINT unique,string_base & out)
{
	if (path_name==0 || prefix==0) return FALSE;
	TCHAR temp[MAX_PATH+1];
	temp[0]=0;
	if (GetTempFileName(stringcvt::string_os_from_utf8(path_name),stringcvt::string_os_from_utf8(prefix),unique,temp))
	{
		out = stringcvt::string_utf8_from_os(temp,tabsize(temp));
		return TRUE;
	}
	return FALSE;
}

class uFindFile_i : public uFindFile
{
	string8 fn;
	WIN32_FIND_DATA fd;
	HANDLE hFF;
public:
	uFindFile_i() {hFF = INVALID_HANDLE_VALUE;}
	bool FindFirst(const char * path)
	{
		hFF = FindFirstFile(stringcvt::string_os_from_utf8(path),&fd);
		if (hFF==INVALID_HANDLE_VALUE) return false;
		fn = stringcvt::string_utf8_from_os(fd.cFileName,tabsize(fd.cFileName));
		return true;
	}
	virtual BOOL FindNext()
	{
		if (hFF==INVALID_HANDLE_VALUE) return FALSE;
		BOOL rv = FindNextFile(hFF,&fd);
		if (rv) fn = stringcvt::string_utf8_from_os(fd.cFileName,tabsize(fd.cFileName));
		return rv;
	}

	virtual const char * GetFileName()
	{
		return fn;
	}

	virtual t_uint64 GetFileSize()
	{
		union
		{
			t_uint64 val64;
			struct
			{
				DWORD lo,hi;
			};
		} ret;
		
		ret.hi = fd.nFileSizeHigh;
		ret.lo = fd.nFileSizeLow;
		return ret.val64;

	}
	virtual DWORD GetAttributes()
	{
		return fd.dwFileAttributes;
	}

	virtual FILETIME GetCreationTime()
	{
		return fd.ftCreationTime;
	}
	virtual FILETIME GetLastAccessTime()
	{
		return fd.ftLastAccessTime;
	}
	virtual FILETIME GetLastWriteTime()
	{
		return fd.ftLastWriteTime;
	}
	virtual ~uFindFile_i()
	{
		if (hFF!=INVALID_HANDLE_VALUE) FindClose(hFF);
	}
};

puFindFile SHARED_EXPORT uFindFirstFile(const char * path)
{
	uFindFile_i * ptr = new uFindFile_i;
	if (!ptr->FindFirst(path))
	{
		delete ptr;
		ptr = 0;
	}
	return ptr;
}

static UINT_PTR CALLBACK uGetOpenFileName_Hook(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			OPENFILENAME * ofn = reinterpret_cast<OPENFILENAME*>(lp);
			reinterpret_cast<modal_dialog_scope*>(ofn->lCustData)->initialize(FindOwningPopup(wnd));
		}
		return 0;
	default:
		return 0;
	}
}

BOOL SHARED_EXPORT uGetOpenFileName(HWND parent,const char * p_ext_mask,unsigned def_ext_mask,const char * p_def_ext,const char * p_title,const char * p_directory,string_base & p_filename,BOOL b_save)
{
	modal_dialog_scope scope;

	stringcvt::string_os_from_utf8 ext_mask_t(p_ext_mask);
	array_t<TCHAR> ext_mask; ext_mask.set_size(_tcslen(ext_mask_t)+2);
	pfc::memset_t(ext_mask,(TCHAR)0);
	_tcscpy(ext_mask.get_ptr(),ext_mask_t);

	{
		UINT n;
		for(n=0;n<ext_mask.get_size();n++)
			if (ext_mask[n]=='|')
				ext_mask[n]=0;
	}
	
	TCHAR buffer[4096];

	tcsncpy_addnull(buffer,stringcvt::string_os_from_utf8(p_filename),tabsize(buffer));

	stringcvt::string_os_from_utf8 def_ext(p_def_ext ? p_def_ext : ""),title(p_title ? p_title : ""),
		directory(p_directory ? p_directory : "");

	OPENFILENAME ofn;

	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hwndOwner = parent;
	ofn.lpstrFilter = ext_mask.get_ptr();
	ofn.nFilterIndex = def_ext_mask + 1;
	ofn.lpstrFile = buffer;
	ofn.lpstrInitialDir = directory;
	ofn.nMaxFile = tabsize(buffer);
	ofn.Flags = b_save ? OFN_EXPLORER|OFN_HIDEREADONLY|OFN_PATHMUSTEXIST|OFN_OVERWRITEPROMPT|OFN_ENABLEHOOK|OFN_ENABLESIZING : OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_PATHMUSTEXIST|OFN_ENABLEHOOK|OFN_ENABLESIZING;
	ofn.lpstrDefExt = *(const TCHAR*)def_ext ? (const TCHAR*)def_ext : 0;
	ofn.lpstrTitle = *(const TCHAR*)title ? (const TCHAR*)title : 0;
	ofn.lCustData = reinterpret_cast<LPARAM>(&scope);
	ofn.lpfnHook = uGetOpenFileName_Hook;
	if (b_save ? GetSaveFileName(&ofn) : GetOpenFileName(&ofn))
	{
		buffer[tabsize(buffer)-1]=0;

		{
			t_size ptr = _tcslen(buffer);
			while(ptr>0 && buffer[ptr-1]==' ') buffer[--ptr] = 0;
		}

		p_filename = stringcvt::string_utf8_from_os(buffer,tabsize(buffer));
		return TRUE;
	}
	else return FALSE;
}

class uGetOpenFileNameMultiResult_i : public uGetOpenFileNameMultiResult
{
	ptr_list_t<char> m_data;
public:
	void AddItem(const char * param) {m_data.add_item(strdup(param));}

	t_size get_count() const {return m_data.get_count();}
	void get_item_ex(const char * & p_out,t_size n) const {char * temp;m_data.get_item_ex(temp,n);p_out = temp;}
	virtual ~uGetOpenFileNameMultiResult_i() {m_data.free_all();}
};

puGetOpenFileNameMultiResult SHARED_EXPORT uGetOpenFileNameMulti(HWND parent,const char * p_ext_mask,unsigned def_ext_mask,const char * p_def_ext,const char * p_title,const char * p_directory)
{
	modal_dialog_scope scope;

	stringcvt::string_os_from_utf8 ext_mask_t(p_ext_mask);
	array_t<TCHAR> ext_mask; ext_mask.set_size(_tcslen(ext_mask_t)+2);
	pfc::memset_t(ext_mask,(TCHAR)0);
	_tcscpy(ext_mask.get_ptr(),ext_mask_t);

	{
		UINT n;
		for(n=0;n<ext_mask.get_size();n++)
			if (ext_mask[n]=='|')
				ext_mask[n]=0;
	}
	
	TCHAR buffer[0x4000];
	buffer[0]=0;

	stringcvt::string_os_from_utf8 def_ext(p_def_ext ? p_def_ext : ""),title(p_title ? p_title : ""),
		directory(p_directory ? p_directory : "");

	OPENFILENAME ofn;

	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hwndOwner = parent;
	ofn.lpstrFilter = ext_mask.get_ptr();
	ofn.nFilterIndex = def_ext_mask + 1;
	ofn.lpstrFile = buffer;
	ofn.lpstrInitialDir = directory;
	ofn.nMaxFile = tabsize(buffer);
	ofn.Flags = OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_PATHMUSTEXIST|OFN_ALLOWMULTISELECT|OFN_ENABLEHOOK|OFN_ENABLESIZING;
	ofn.lpstrDefExt = *(const TCHAR*)def_ext ? (const TCHAR*)def_ext : 0;
	ofn.lpstrTitle = *(const TCHAR*)title ? (const TCHAR*)title : 0;
	ofn.lCustData = reinterpret_cast<LPARAM>(&scope);
	ofn.lpfnHook = uGetOpenFileName_Hook;
	if (GetOpenFileName(&ofn))
	{
		buffer[tabsize(buffer)-1]=0;
		buffer[tabsize(buffer)-2]=0;

		uGetOpenFileNameMultiResult_i * result = new uGetOpenFileNameMultiResult_i;

		TCHAR * p=buffer;
		while(*p) p++;
		p++;
		if (!*p)
		{
			{
				t_size ptr = _tcslen(buffer);
				while(ptr>0 && buffer[ptr-1]==' ') buffer[--ptr] = 0;
			}

			result->AddItem(stringcvt::string_utf8_from_os(buffer));
		}
		else
		{
			string8 s = stringcvt::string_utf8_from_os(buffer,tabsize(buffer));
			t_size ofs = s.length();
			if (ofs>0 && s[ofs-1]!='\\') {s.add_char('\\');ofs++;}
			while(*p)
			{
				s.truncate(ofs);
				s += stringcvt::string_utf8_from_os(p);
				s.skip_trailing_char(' ');
				result->AddItem(s);
				while(*p) p++;
				p++;
			}
		}
		return result;
	}
	else return 0;
}

HINSTANCE SHARED_EXPORT uShellExecute(HWND wnd,const char * oper,const char * file,const char * params,const char * dir,int cmd)
{
	return ShellExecute(wnd,param_os_from_utf8(oper),param_os_from_utf8(file),param_os_from_utf8(params),param_os_from_utf8(dir),cmd);
}

HWND SHARED_EXPORT uCreateStatusWindow(LONG style,const char * text,HWND parent,UINT id)
{
	return CreateStatusWindow(style,param_os_from_utf8(text),parent,id);
}

HWND SHARED_EXPORT uCreateWindowEx(DWORD dwExStyle,const char * lpClassName,const char * lpWindowName,DWORD dwStyle,int x,int y,int nWidth,int nHeight,HWND hWndParent,HMENU hMenu,HINSTANCE hInstance,LPVOID lpParam)
{
	return CreateWindowEx(dwExStyle,param_os_from_utf8(lpClassName),param_os_from_utf8(lpWindowName),dwStyle,x,y,nWidth,nHeight,hWndParent,hMenu,hInstance,lpParam);
}

HANDLE SHARED_EXPORT uLoadImage(HINSTANCE hIns,const char * name,UINT type,int x,int y,UINT flags)
{
	return LoadImage(hIns,param_os_from_utf8(name),type,x,y,flags);
}

BOOL SHARED_EXPORT uGetSystemDirectory(string_base & out)
{
	UINT len = GetSystemDirectory(0,0);
	if (len==0) len = MAX_PATH;
	len++;
	array_t<TCHAR> temp;
	temp.set_size(len);
	if (GetSystemDirectory(temp.get_ptr(),len)==0) return FALSE;
	out = stringcvt::string_utf8_from_os(temp.get_ptr(),len);
	return TRUE;	
}

BOOL SHARED_EXPORT uGetWindowsDirectory(string_base & out)
{
	UINT len = GetWindowsDirectory(0,0);
	if (len==0) len = MAX_PATH;
	len++;
	array_t<TCHAR> temp;
	temp.set_size(len);
	if (GetWindowsDirectory(temp.get_ptr(),len)==0) return FALSE;
	out = stringcvt::string_utf8_from_os(temp.get_ptr(),len);
	return TRUE;	
}

BOOL SHARED_EXPORT uSetCurrentDirectory(const char * path)
{
	return SetCurrentDirectory(stringcvt::string_os_from_utf8(path));
}

BOOL SHARED_EXPORT uGetCurrentDirectory(string_base & out)
{
	UINT len = GetCurrentDirectory(0,0);
	if (len==0) len = MAX_PATH;
	len++;
	array_t<TCHAR> temp;
	temp.set_size(len);
	if (GetCurrentDirectory(len,temp.get_ptr())==0) return FALSE;
	out = stringcvt::string_utf8_from_os(temp.get_ptr(),len);
	return TRUE;		
}

BOOL SHARED_EXPORT uExpandEnvironmentStrings(const char * src,string_base & out)
{
	stringcvt::string_os_from_utf8 src_os(src);
	UINT len = ExpandEnvironmentStrings(src_os,0,0);
	if (len==0) len = 256;
	len++;
	array_t<TCHAR> temp;
	temp.set_size(len);
	if (ExpandEnvironmentStrings(src_os,temp.get_ptr(),len)==0) return FALSE;
	out = stringcvt::string_utf8_from_os(temp.get_ptr(),len);
	return TRUE;
}

BOOL SHARED_EXPORT uGetUserName(string_base & out)
{
	TCHAR temp[UNLEN+1];
	DWORD len = tabsize(temp);
	if (GetUserName(temp,&len))
	{
		out = stringcvt::string_utf8_from_os(temp,tabsize(temp));
		return TRUE;
	}
	else return FALSE;
}

BOOL SHARED_EXPORT uGetShortPathName(const char * src,string_base & out)
{
	stringcvt::string_os_from_utf8 src_os(src);
	UINT len = GetShortPathName(src_os,0,0);
	if (len==0) len = MAX_PATH;
	len++;
	array_t<TCHAR> temp; temp.set_size(len);
	if (GetShortPathName(src_os,temp.get_ptr(),len))
	{
		out = stringcvt::string_utf8_from_os(temp.get_ptr(),len);
		return TRUE;
	}
	else return FALSE;
}


#ifdef UNICODE
#define DDE_CODEPAGE CP_WINUNICODE
#else
#define DDE_CODEPAGE CP_WINANSI
#endif


HSZ SHARED_EXPORT uDdeCreateStringHandle(DWORD ins,const char * src)
{
	return DdeCreateStringHandle(ins,stringcvt::string_os_from_utf8(src),DDE_CODEPAGE);
}

BOOL SHARED_EXPORT uDdeQueryString(DWORD ins,HSZ hsz,string_base & out)
{
	array_t<TCHAR> temp;
	UINT len = DdeQueryString(ins,hsz,0,0,DDE_CODEPAGE);
	if (len==0) len = MAX_PATH;
	len++;
	temp.set_size(len);
	if (DdeQueryString(ins,hsz,temp.get_ptr(),len,DDE_CODEPAGE))
	{
		out = stringcvt::string_utf8_from_os(temp.get_ptr(),len);
		return TRUE;
	}
	else return FALSE;
}

UINT SHARED_EXPORT uDdeInitialize(LPDWORD pidInst,PFNCALLBACK pfnCallback,DWORD afCmd,DWORD ulRes)
{
	return DdeInitialize(pidInst,pfnCallback,afCmd,ulRes);
}

BOOL SHARED_EXPORT uDdeAccessData_Text(HDDEDATA data,string_base & out)
{
	const TCHAR * ptr = (const TCHAR*) DdeAccessData(data,0);
	if (ptr)
	{
		out = stringcvt::string_utf8_from_os(ptr);
		return TRUE;
	}
	else return FALSE;
}

HANDLE SHARED_EXPORT uSortStringCreate(const char * src)
{
	stringcvt::string_os_from_utf8 temp;
	temp.convert(src);
	TCHAR * ret = pfc::malloc_t<TCHAR>(temp.length() + 1);
	_tcscpy(ret,temp);
	return (HANDLE) ret;
}

int SHARED_EXPORT uSortStringCompareEx(HANDLE string1,HANDLE string2,DWORD flags)
{
	return CompareString(LOCALE_USER_DEFAULT,flags,reinterpret_cast<const TCHAR*>(string1),-1,reinterpret_cast<const TCHAR*>(string2),-1);
}

int SHARED_EXPORT uSortStringCompare(HANDLE string1,HANDLE string2)
{
	return lstrcmpi(reinterpret_cast<const TCHAR*>(string1),reinterpret_cast<const TCHAR*>(string2));
}

void SHARED_EXPORT uSortStringFree(HANDLE string)
{
	pfc::free_t(reinterpret_cast<TCHAR*>(string));
}

HTREEITEM SHARED_EXPORT uTreeView_InsertItem(HWND wnd,const uTVINSERTSTRUCT * param)
{
	stringcvt::string_os_from_utf8 temp;
	temp.convert(param->item.pszText);


	TVINSERTSTRUCT l_param;
	memset(&l_param,0,sizeof(l_param));
	l_param.hParent = param->hParent;
	l_param.hInsertAfter = param->hInsertAfter;
	l_param.item.mask = param->item.mask;
	l_param.item.hItem = param->item.hItem;
	l_param.item.state = param->item.state;
	l_param.item.stateMask = param->item.stateMask;
	l_param.item.pszText = const_cast<TCHAR*>(temp.get_ptr());
	l_param.item.cchTextMax = 0;
	l_param.item.iImage = param->item.iImage;
	l_param.item.iSelectedImage = param->item.iImage;
	l_param.item.cChildren = param->item.cChildren;
	l_param.item.lParam = param->item.lParam;
	if (param->item.mask & TVIF_INTEGRAL)
	{
		l_param.itemex.iIntegral = param->itemex.iIntegral;
	}

	return (HTREEITEM) uSendMessage(wnd,TVM_INSERTITEM,0,(LPARAM)&l_param);
}

UINT SHARED_EXPORT uGetFontHeight(HFONT font)
{
	UINT ret;
	HDC dc = CreateCompatibleDC(0);
	SelectObject(dc,font);
	ret = uGetTextHeight(dc);
	DeleteDC(dc);
	return ret;
}


HIMAGELIST SHARED_EXPORT uImageList_LoadImage(HINSTANCE hi, const char * lpbmp, int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags)
{
	return ImageList_LoadImage(hi,param_os_from_utf8(lpbmp),cx,cGrow,crMask,uType,uFlags);
}

int SHARED_EXPORT uTabCtrl_InsertItem(HWND wnd,t_size idx,const uTCITEM * item)
{
	param_os_from_utf8 text((item->mask & TCIF_TEXT) ? item->pszText : 0);
	TCITEM l_item;
	assert(sizeof(l_item)==sizeof(*item));//meh lazy
	memcpy(&l_item,item,sizeof(l_item));
	l_item.pszText = const_cast<TCHAR*>(text.get_ptr());
	l_item.cchTextMax = 0;
	return TabCtrl_InsertItem(wnd,idx,&l_item);
}

int SHARED_EXPORT uTabCtrl_SetItem(HWND wnd,t_size idx,const uTCITEM * item)
{
	param_os_from_utf8 text((item->mask & TCIF_TEXT) ? item->pszText : 0);
	TCITEM l_item;
	assert(sizeof(l_item)==sizeof(*item));//meh lazy
	memcpy(&l_item,item,sizeof(l_item));
	l_item.pszText = const_cast<TCHAR*>(text.get_ptr());
	l_item.cchTextMax = 0;
	return TabCtrl_SetItem(wnd,idx,&l_item);
}

int SHARED_EXPORT uGetKeyNameText(LONG lparam,string_base & out)
{
	TCHAR temp[256];
	temp[0]=0;
	if (!GetKeyNameText(lparam,temp,tabsize(temp))) return 0;
	out = stringcvt::string_utf8_from_os(temp,tabsize(temp));
	return 1;
}

HANDLE SHARED_EXPORT uCreateFileMapping(HANDLE hFile,LPSECURITY_ATTRIBUTES lpFileMappingAttributes,DWORD flProtect,DWORD dwMaximumSizeHigh,DWORD dwMaximumSizeLow,const char * lpName)
{
	return CreateFileMapping(hFile,lpFileMappingAttributes,flProtect,dwMaximumSizeHigh,dwMaximumSizeLow,param_os_from_utf8(lpName));
}

BOOL SHARED_EXPORT uListBox_GetText(HWND listbox,UINT index,string_base & out)
{
	t_size len = uSendMessage(listbox,LB_GETTEXTLEN,index,0);
	if (len==LB_ERR || len>16*1024*1024) return FALSE;
	if (len==0) {out.reset();return TRUE;}

	array_t<TCHAR> temp; temp.set_size(len+1);
	pfc::memset_t(temp,(TCHAR)0);
	len = uSendMessage(listbox,LB_GETTEXT,index,(LPARAM)temp.get_ptr());
	if (len==LB_ERR) return false;
	out = stringcvt::string_utf8_from_os(temp.get_ptr());
	return TRUE;
}
/*
void SHARED_EXPORT uPrintf(string_base & out,const char * fmt,...)
{
	va_list list;
	va_start(list,fmt);
	uPrintfV(out,fmt,list);
	va_end(list);
}
*/
void SHARED_EXPORT uPrintfV(string_base & out,const char * fmt,va_list arglist)
{
	string_printf::g_run(out,fmt,arglist);
}

int SHARED_EXPORT uCompareString(DWORD flags,const char * str1,unsigned len1,const char * str2,unsigned len2)
{
	return CompareString(LOCALE_USER_DEFAULT,flags,stringcvt::string_os_from_utf8(str1,len1),-1,stringcvt::string_os_from_utf8(str2,len2),-1);
}

class uResource_i : public uResource
{
	unsigned size;
	const void * ptr;
public:
	inline uResource_i(const void * p_ptr,unsigned p_size) : ptr(p_ptr), size(p_size)
	{
	}
	virtual const void * GetPointer()
	{
		return ptr;
	}
	virtual unsigned GetSize()
	{
		return size;
	}
	virtual ~uResource_i()
	{
	}
};

puResource SHARED_EXPORT uLoadResource(HMODULE hMod,const char * name,const char * type,WORD wLang)
{
	HRSRC res = uFindResource(hMod,name,type,wLang);
	if (res==0) return 0;
	HGLOBAL hglob = LoadResource(hMod,res);
	if (hglob)
	{
		void * ptr = LockResource(hglob);
		if (ptr)
		{
			return new uResource_i(ptr,SizeofResource(hMod,res));
		}
		else return 0;
	}
	else return 0;
}

puResource SHARED_EXPORT LoadResourceEx(HMODULE hMod,const TCHAR * name,const TCHAR * type,WORD wLang)
{
	HRSRC res = wLang ? FindResourceEx(hMod,name,type,wLang) : FindResource(hMod,name,type);
	if (res==0) return 0;
	HGLOBAL hglob = LoadResource(hMod,res);
	if (hglob)
	{
		void * ptr = LockResource(hglob);
		if (ptr)
		{
			return new uResource_i(ptr,SizeofResource(hMod,res));
		}
		else return 0;
	}
	else return 0;
}

HRSRC SHARED_EXPORT uFindResource(HMODULE hMod,const char * name,const char * type,WORD wLang)
{
	return wLang ? FindResourceEx(hMod,param_os_from_utf8(name),param_os_from_utf8(type),wLang) : FindResource(hMod,param_os_from_utf8(name),param_os_from_utf8(type));
}

BOOL SHARED_EXPORT uLoadString(HINSTANCE ins,UINT id,string_base & out)
{
	BOOL rv = FALSE;
	uResource * res = uLoadResource(ins,uMAKEINTRESOURCE(id),(const char*)(RT_STRING));
	if (res)
	{
		unsigned size = res->GetSize();
		const WCHAR * ptr = (const WCHAR*)res->GetPointer();
		if (size>=4)
		{
			unsigned len = *(const WORD*)(ptr+1);
			if (len * 2 + 4 <= size)
			{
				out = stringcvt::string_utf8_from_wide(ptr+2,len);
			}
		}
		
		delete res;
		rv = TRUE;
	}
	return rv;
}

BOOL SHARED_EXPORT uGetMenuString(HMENU menu,UINT id,string_base & out,UINT flag)
{
	unsigned len = GetMenuString(menu,id,0,0,flag);
	if (len==0)
	{
		out.reset();
		return FALSE;
	}
	array_t<TCHAR> temp;
	temp.set_size(len+1);
	if (GetMenuString(menu,id,temp.get_ptr(),len+1,flag)==0) {
		out.reset();
		return FALSE;
	}
	out = stringcvt::string_utf8_from_os(temp.get_ptr());
	return TRUE;
}

BOOL SHARED_EXPORT uModifyMenu(HMENU menu,UINT id,UINT flags,UINT newitem,const char * data)
{
	return ModifyMenu(menu,id,flags,newitem,param_os_from_utf8(data));
}

UINT SHARED_EXPORT uGetMenuItemType(HMENU menu,UINT position)
{
	MENUITEMINFO info;
	memset(&info,0,sizeof(info));
	info.cbSize = sizeof(info);
	info.fMask = MIIM_TYPE;
	if (!GetMenuItemInfo(menu,position,TRUE,&info))
		return 0;
	return info.fType;
}

static inline bool i_is_path_separator(unsigned c)
{
	return c=='\\' || c=='/' || c=='|' || c==':';
}

int SHARED_EXPORT uSortPathCompare(HANDLE string1,HANDLE string2)
{
	const TCHAR * s1 = reinterpret_cast<const TCHAR*>(string1);
	const TCHAR * s2 = reinterpret_cast<const TCHAR*>(string2);
	const TCHAR * p1, * p2;

	while (*s1 || *s2)
	{
		if (*s1 == *s2)
		{
			s1++;
			s2++;
			continue;
		}

		p1 = s1; while (*p1 && !i_is_path_separator(*p1)) p1++;
		p2 = s2; while (*p2 && !i_is_path_separator(*p2)) p2++;

		if ((!*p1 && !*p2) || (*p1 && *p2))
		{
			int test = CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH, s1, pfc::downcast_guarded<int>(p1 - s1), s2, pfc::downcast_guarded<int>(p2 - s2));
			if (test && test != 2) return test - 2;
			if (!*p1) return 0;
		}
		else
		{
			if (*p1) return -1;
			else return 1;
		}

		s1 = p1 + 1;
		s2 = p2 + 1;
	}
	
	return 0;
}

UINT SHARED_EXPORT uRegisterClipboardFormat(const char * name)
{
	return RegisterClipboardFormat(stringcvt::string_os_from_utf8(name));
}

BOOL SHARED_EXPORT uGetClipboardFormatName(UINT format,string_base & out)
{
	TCHAR temp[1024];
	if (!GetClipboardFormatName(format,temp,tabsize(temp))) return FALSE;
	out = stringcvt::string_utf8_from_os(temp,tabsize(temp));
	return TRUE;
}

}//extern "C"
#ifdef _MSC_VER
#if _MSC_VER==1200
#pragma comment(linker,"/opt:nowin98")
#endif
#endif

BOOL SHARED_EXPORT uSearchPath(const char * path, const char * filename, const char * extension, string_base & p_out)
{
	enum {temp_size = 1024};
	param_os_from_utf8 path_os(path), filename_os(filename), extension_os(extension);
	array_t<TCHAR> temp; temp.set_size(temp_size);
	LPTSTR dummy;
	unsigned len;

	len = SearchPath(path_os,filename_os,extension_os,temp_size,temp.get_ptr(),&dummy);
	if (len == 0) return FALSE;
	if (len >= temp_size)
	{
		unsigned len2;
		temp.set_size(len + 1);
		len2 = SearchPath(path_os,filename_os,extension_os,len+1,temp.get_ptr(),&dummy);
		if (len2 == 0 || len2 > len) return FALSE;
		len = len2;
	}

	p_out = stringcvt::string_utf8_from_os(temp.get_ptr(),len);

	return TRUE;

}

static bool is_ascii_alpha(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static char ascii_upper(char c)
{
	if (c >= 'a' && c <= 'z') c += 'A' - 'a';
	return c;
}

BOOL SHARED_EXPORT uFixPathCaps(const char * path,string_base & p_out)
{
	string8_fastalloc temp;
	if (path[0] == '\\' && path[1] == '\\')
	{
		unsigned index = 2;
		while(path[index] != '\\')
		{
			if (path[index] == 0) return FALSE;
			index++;
		}
		index++;
		if (path[index] == '\\' || path[index] == 0) return FALSE;
		while(path[index] != '\\')
		{
			if (path[index] == 0) {
				// \\host\share
				uStringLower(p_out,path);
				return TRUE;
			}
			index++;
		}
		index++;
		if (path[index] == '\\') return FALSE;
		uAddStringLower(temp,path,index);
		path += index;
	}
	else if (is_ascii_alpha(path[0]) && path[1] == ':' && path[2] == '\\')
	{
		temp.add_char(ascii_upper(path[0]));
		temp.add_string(":\\");
		path += 3;
	}
	else return FALSE;

	for(;;)
	{
		t_size truncat = temp.length();
		t_size delta = 0;
		while(path[delta]!=0 && path[delta]!='\\') delta++;
		if (delta == 0) break;
		temp.add_string(path,delta);
		uFindFile * ff = uFindFirstFile(temp);
		bool found = false;
		if (ff)
		{
			do {
				const char * fn = ff->GetFileName();
				if (!stricmp_utf8_ex(path,delta,fn,strlen(fn)))
				{
					found = true;
					temp.truncate(truncat);
					temp.add_string(fn);
					break;
				}
			} while(ff->FindNext());
			delete ff;
		}
		if (!found)
		{
			temp.add_string(path + delta);
			break;
		}
		path += delta;
		if (*path == 0) break;
		path ++;
		temp.add_char('\\');
	}


	p_out = temp;

	return TRUE;
}

LPARAM SHARED_EXPORT uTreeView_GetUserData(HWND p_tree,HTREEITEM p_item)
{
	TVITEM item;
	memset(&item,0,sizeof(item));
	item.mask = TVIF_PARAM;
	item.hItem = p_item;
	if (uSendMessage(p_tree,TVM_GETITEM,0,(LPARAM)&item))
		return item.lParam;
	return 0;
}

bool SHARED_EXPORT uTreeView_GetText(HWND p_tree,HTREEITEM p_item,string_base & p_out)
{
	TCHAR temp[1024];//changeme ?
	TVITEM item;
	memset(&item,0,sizeof(item));
	item.mask = TVIF_TEXT;
	item.hItem = p_item;
	item.pszText = temp;
	item.cchTextMax = tabsize(temp);
	if (uSendMessage(p_tree,TVM_GETITEM,0,(LPARAM)&item))
	{
		p_out = stringcvt::string_utf8_from_os(temp,tabsize(temp));
		return true;
	}
	else return false;
}

BOOL SHARED_EXPORT uSetWindowText(HWND wnd,const char * p_text)
{
	return uSetWindowTextEx(wnd,p_text,infinite);
}

BOOL SHARED_EXPORT uSetDlgItemText(HWND wnd,UINT id,const char * p_text)
{
	return uSetDlgItemTextEx(wnd,id,p_text,infinite);
}

BOOL SHARED_EXPORT uFileExists(const char * fn)
{
	DWORD attrib = uGetFileAttributes(fn);
	if (attrib == 0xFFFFFFFF || (attrib & FILE_ATTRIBUTE_DIRECTORY)) return FALSE;
	return TRUE;
}

BOOL SHARED_EXPORT uFormatSystemErrorMessage(string_base & p_out,DWORD p_code) {
	switch(p_code) {
	case ERROR_CHILD_NOT_COMPLETE:
		p_out = "Application cannot be run in Win32 mode.";
		return TRUE;
	case ERROR_INVALID_ORDINAL:
		p_out = "Invalid ordinal.";
		return TRUE;
	case ERROR_INVALID_STARTING_CODESEG:
		p_out = "Invalid code segment.";
		return TRUE;
	case ERROR_INVALID_STACKSEG:
		p_out = "Invalid stack segment.";
		return TRUE;
	case ERROR_INVALID_MODULETYPE:
		p_out = "Invalid module type.";
		return TRUE;
	case ERROR_INVALID_EXE_SIGNATURE:
		p_out = "Invalid executable signature.";
		return TRUE;
	case ERROR_BAD_EXE_FORMAT:
		p_out = "Not a valid Win32 application.";
		return TRUE;
	case ERROR_EXE_MACHINE_TYPE_MISMATCH:
		p_out = "Machine type mismatch.";
		return TRUE;
	case ERROR_EXE_CANNOT_MODIFY_SIGNED_BINARY:
	case ERROR_EXE_CANNOT_MODIFY_STRONG_SIGNED_BINARY:
		p_out = "Unable to modify signed binary.";
		return TRUE;
	default:
		{
			TCHAR temp[512];
			if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,0,p_code,0,temp,tabsize(temp),0) == 0) return FALSE;
			p_out = stringcvt::string_utf8_from_os(temp,tabsize(temp));
			return TRUE;
		}
		break;
	}
}