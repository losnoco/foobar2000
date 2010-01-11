#ifndef _UTF8API_DLL__UTF8API_H_
#define _UTF8API_DLL__UTF8API_H_

#include "../../pfc/pfc.h"

#ifndef WIN32
#error N/A
#endif

#ifndef STRICT
#define STRICT
#endif

#include <windows.h>
#include <ddeml.h>
#include <commctrl.h>

#ifndef SHARED_EXPORTS
#define SHARED_EXPORT _declspec(dllimport)
#else
#define SHARED_EXPORT _declspec(dllexport)
#endif

extern "C" {

//SHARED_EXPORT BOOL IsUnicode();
#ifdef UNICODE
#define IsUnicode() 1
#else
#define IsUnicode() 0
#endif

SHARED_EXPORT LRESULT uSendMessageText(HWND wnd,UINT msg,WPARAM wp,const char * text);
SHARED_EXPORT LRESULT uSendDlgItemMessageText(HWND wnd,UINT id,UINT msg,WPARAM wp,const char * text);
SHARED_EXPORT BOOL uGetWindowText(HWND wnd,string_base & out);
SHARED_EXPORT BOOL uSetWindowText(HWND wnd,const char * p_text);
SHARED_EXPORT BOOL uSetWindowTextEx(HWND wnd,const char * p_text,unsigned p_text_length);
SHARED_EXPORT BOOL uGetDlgItemText(HWND wnd,UINT id,string_base & out);
SHARED_EXPORT BOOL uSetDlgItemText(HWND wnd,UINT id,const char * p_text);
SHARED_EXPORT BOOL uSetDlgItemTextEx(HWND wnd,UINT id,const char * p_text,unsigned p_text_length);
SHARED_EXPORT BOOL uBrowseForFolder(HWND parent,const char * title,string_base & out);
SHARED_EXPORT BOOL uBrowseForFolderWithFile(HWND parent,const char * title,string_base & out,const char * p_file_to_find);
SHARED_EXPORT int uMessageBox(HWND wnd,const char * text,const char * caption,UINT type);
SHARED_EXPORT void uOutputDebugString(const char * msg);
SHARED_EXPORT BOOL uAppendMenu(HMENU menu,UINT flags,UINT id,const char * content);
SHARED_EXPORT BOOL uInsertMenu(HMENU menu,UINT position,UINT flags,UINT id,const char * content);
SHARED_EXPORT int uStringCompare(const char * elem1, const char * elem2);
SHARED_EXPORT HINSTANCE uLoadLibrary(const char * name);
SHARED_EXPORT HANDLE uCreateEvent(LPSECURITY_ATTRIBUTES lpEventAttributes,BOOL bManualReset,BOOL bInitialState, const char * lpName);
SHARED_EXPORT DWORD uGetModuleFileName(HMODULE hMod,string_base & out);
SHARED_EXPORT BOOL uSetClipboardString(const char * ptr);
SHARED_EXPORT BOOL uIsDialogMessage(HWND dlg,LPMSG msg);
SHARED_EXPORT BOOL uGetMessage(LPMSG msg,HWND wnd,UINT min,UINT max);
SHARED_EXPORT BOOL uGetClassName(HWND wnd,string_base & out);
SHARED_EXPORT UINT uCharLength(const char * src);
SHARED_EXPORT BOOL uDragQueryFile(HDROP hDrop,UINT idx,string_base & out);
SHARED_EXPORT UINT uDragQueryFileCount(HDROP hDrop);
SHARED_EXPORT BOOL uGetTextExtentPoint32(HDC dc,const char * text,UINT cb,LPSIZE size);//note, cb is number of bytes, not actual unicode characters in the string (read: plain strlen() will do)
SHARED_EXPORT BOOL uExtTextOut(HDC dc,int x,int y,UINT flags,const RECT * rect,const char * text,UINT cb,const int * lpdx);
SHARED_EXPORT BOOL uTextOutColors(HDC dc,const char * src,UINT len,int x,int y,const RECT * clip,BOOL is_selected,DWORD default_color);
SHARED_EXPORT BOOL uTextOutColorsTabbed(HDC dc,const char * src,UINT src_len,const RECT * item,int border,const RECT * clip,BOOL selected,DWORD default_color,BOOL use_columns);
SHARED_EXPORT UINT uGetTextHeight(HDC dc);
SHARED_EXPORT UINT uGetFontHeight(HFONT font);
SHARED_EXPORT BOOL uChooseColor(DWORD * p_color,HWND parent,DWORD * p_custom_colors);
SHARED_EXPORT HCURSOR uLoadCursor(HINSTANCE hIns,const char * name);
SHARED_EXPORT HICON uLoadIcon(HINSTANCE hIns,const char * name);
SHARED_EXPORT HMENU uLoadMenu(HINSTANCE hIns,const char * name);
SHARED_EXPORT BOOL uGetEnvironmentVariable(const char * name,string_base & out);
SHARED_EXPORT HMODULE uGetModuleHandle(const char * name);
SHARED_EXPORT UINT uRegisterWindowMessage(const char * name);
SHARED_EXPORT BOOL uMoveFile(const char * src,const char * dst);
SHARED_EXPORT BOOL uDeleteFile(const char * fn);
SHARED_EXPORT DWORD uGetFileAttributes(const char * fn);
SHARED_EXPORT BOOL uFileExists(const char * fn);
SHARED_EXPORT BOOL uRemoveDirectory(const char * fn);
SHARED_EXPORT HANDLE uCreateFile(const char * fn,DWORD access,DWORD share,LPSECURITY_ATTRIBUTES blah,DWORD creat,DWORD flags,HANDLE tmpl);
SHARED_EXPORT HANDLE uCreateFileMapping(HANDLE hFile,LPSECURITY_ATTRIBUTES lpFileMappingAttributes,DWORD flProtect,DWORD dwMaximumSizeHigh,DWORD dwMaximumSizeLow,const char * lpName);
SHARED_EXPORT BOOL uCreateDirectory(const char * fn,LPSECURITY_ATTRIBUTES blah);
SHARED_EXPORT HANDLE uCreateMutex(LPSECURITY_ATTRIBUTES blah,BOOL bInitialOwner,const char * name);
SHARED_EXPORT BOOL uGetLongPathName(const char * name,string_base & out);//may just fail to work on old windows versions, present on win98/win2k+
SHARED_EXPORT BOOL uGetFullPathName(const char * name,string_base & out);
SHARED_EXPORT BOOL uSearchPath(const char * path, const char * filename, const char * extension, string_base & p_out);
SHARED_EXPORT BOOL uFixPathCaps(const char * path,string_base & p_out);
SHARED_EXPORT void uGetCommandLine(string_base & out);
SHARED_EXPORT BOOL uGetTempPath(string_base & out);
SHARED_EXPORT BOOL uGetTempFileName(const char * path_name,const char * prefix,UINT unique,string_base & out);
SHARED_EXPORT BOOL uGetOpenFileName(HWND parent,const char * p_ext_mask,unsigned def_ext_mask,const char * p_def_ext,const char * p_title,const char * p_directory,string_base & p_filename,BOOL b_save);
//note: uGetOpenFileName extension mask uses | as separator, not null
SHARED_EXPORT HANDLE uLoadImage(HINSTANCE hIns,const char * name,UINT type,int x,int y,UINT flags);
SHARED_EXPORT UINT uRegisterClipboardFormat(const char * name);
SHARED_EXPORT BOOL uGetClipboardFormatName(UINT format,string_base & out);

SHARED_EXPORT HANDLE uSortStringCreate(const char * src);
SHARED_EXPORT int uSortStringCompare(HANDLE string1,HANDLE string2);
SHARED_EXPORT int uSortStringCompareEx(HANDLE string1,HANDLE string2,DWORD flags);//flags - see win32 CompareString
SHARED_EXPORT int uSortPathCompare(HANDLE string1,HANDLE string2);
SHARED_EXPORT void uSortStringFree(HANDLE string);


SHARED_EXPORT int uCompareString(DWORD flags,const char * str1,unsigned len1,const char * str2,unsigned len2);

class NOVTABLE uGetOpenFileNameMultiResult : public list_base_const_t<const char*>
{
public:
	inline UINT GetCount() {return get_count();}
	inline const char * GetFileName(UINT index) {return get_item(index);}
	virtual ~uGetOpenFileNameMultiResult() {}
};

SHARED_EXPORT uGetOpenFileNameMultiResult * uGetOpenFileNameMulti(HWND parent,const char * p_ext_mask,unsigned def_ext_mask,const char * p_def_ext,const char * p_title,const char * p_directory);

class NOVTABLE uFindFile
{
protected:
	uFindFile() {}
public:
	virtual BOOL FindNext()=0;
	virtual const char * GetFileName()=0;
	virtual t_uint64 GetFileSize()=0;
	virtual DWORD GetAttributes()=0;
	virtual FILETIME GetCreationTime()=0;
	virtual FILETIME GetLastAccessTime()=0;
	virtual FILETIME GetLastWriteTime()=0;
	virtual ~uFindFile() {};
	inline bool IsDirectory() {return (GetAttributes() & FILE_ATTRIBUTE_DIRECTORY) ? true : false;}
};

SHARED_EXPORT uFindFile * uFindFirstFile(const char * path);

SHARED_EXPORT HINSTANCE uShellExecute(HWND wnd,const char * oper,const char * file,const char * params,const char * dir,int cmd);
SHARED_EXPORT HWND uCreateStatusWindow(LONG style,const char * text,HWND parent,UINT id);

typedef WNDCLASSA uWNDCLASS;
SHARED_EXPORT ATOM uRegisterClass(const uWNDCLASS * ptr);
SHARED_EXPORT BOOL uUnregisterClass(const char * name,HINSTANCE ins);

SHARED_EXPORT BOOL uShellNotifyIcon(DWORD dwMessage,HWND wnd,UINT id,UINT callbackmsg,HICON icon,const char * tip);
SHARED_EXPORT BOOL uShellNotifyIconEx(DWORD dwMessage,HWND wnd,UINT id,UINT callbackmsg,HICON icon,const char * tip,const char * balloon_title,const char * balloon_msg);

SHARED_EXPORT HWND uCreateWindowEx(DWORD dwExStyle,const char * lpClassName,const char * lpWindowName,DWORD dwStyle,int x,int y,int nWidth,int nHeight,HWND hWndParent,HMENU hMenu,HINSTANCE hInstance,LPVOID lpParam);

SHARED_EXPORT BOOL uGetSystemDirectory(string_base & out); 
SHARED_EXPORT BOOL uGetWindowsDirectory(string_base & out); 
SHARED_EXPORT BOOL uSetCurrentDirectory(const char * path);
SHARED_EXPORT BOOL uGetCurrentDirectory(string_base & out);
SHARED_EXPORT BOOL uExpandEnvironmentStrings(const char * src,string_base & out);
SHARED_EXPORT BOOL uGetUserName(string_base & out);
SHARED_EXPORT BOOL uGetShortPathName(const char * src,string_base & out);

SHARED_EXPORT HSZ uDdeCreateStringHandle(DWORD ins,const char * src);
SHARED_EXPORT BOOL uDdeQueryString(DWORD ins,HSZ hsz,string_base & out);
SHARED_EXPORT UINT uDdeInitialize(LPDWORD pidInst,PFNCALLBACK pfnCallback,DWORD afCmd,DWORD ulRes);
SHARED_EXPORT BOOL uDdeAccessData_Text(HDDEDATA data,string_base & out);

SHARED_EXPORT HIMAGELIST uImageList_LoadImage(HINSTANCE hi, const char * lpbmp, int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags);

#define uDdeFreeStringHandle DdeFreeStringHandle
#define uDdeCmpStringHandles DdeCmpStringHandles
#define uDdeKeepStringHandle DdeKeepStringHandle
#define uDdeUninitialize DdeUninitialize
#define uDdeNameService DdeNameService
#define uDdeFreeDataHandle DdeFreeDataHandle


typedef TVINSERTSTRUCTA uTVINSERTSTRUCT;

SHARED_EXPORT HTREEITEM uTreeView_InsertItem(HWND wnd,const uTVINSERTSTRUCT * param);
SHARED_EXPORT LPARAM uTreeView_GetUserData(HWND wnd,HTREEITEM item);
SHARED_EXPORT bool uTreeView_GetText(HWND wnd,HTREEITEM item,string_base & out);

#define uSetWindowsHookEx SetWindowsHookEx
#define uUnhookWindowsHookEx UnhookWindowsHookEx
#define uCallNextHookEx CallNextHookEx


typedef SHFILEOPSTRUCTA uSHFILEOPSTRUCT;
SHARED_EXPORT int uSHFileOperation(uSHFILEOPSTRUCT * lpFileOp);

typedef STARTUPINFOA uSTARTUPINFO;

SHARED_EXPORT BOOL uCreateProcess(
  const char * lpApplicationName,
  const char * lpCommandLine,
  LPSECURITY_ATTRIBUTES lpProcessAttributes,
  LPSECURITY_ATTRIBUTES lpThreadAttributes,
  BOOL bInheritHandles,
  DWORD dwCreationFlags,
  const char * lpEnvironment,
  const char * lpCurrentDirectory,
  const uSTARTUPINFO * lpStartupInfo,
  LPPROCESS_INFORMATION lpProcessInformation
);

SHARED_EXPORT unsigned uOSStringEstimateSize(const char * src,unsigned len = -1);//return value in bytes
SHARED_EXPORT unsigned uOSStringConvert(const char * src,void * dst,unsigned len = -1);//length of src (will cut if found null earlier)
/* usage:

  const char * src = "something";

  void * temp = malloc(uOSStringEstimateSize(src));
  uOSStringConvert(src,temp);
  //now temp contains OS-friendly (TCHAR) version of src
*/

typedef TCITEMA uTCITEM;
SHARED_EXPORT int uTabCtrl_InsertItem(HWND wnd,int idx,const uTCITEM * item);
SHARED_EXPORT int uTabCtrl_SetItem(HWND wnd,int idx,const uTCITEM * item);

SHARED_EXPORT int uGetKeyNameText(LONG lparam,string_base & out);

SHARED_EXPORT void uFixAmpersandChars(const char * src,string_base & out);//for systray
SHARED_EXPORT void uFixAmpersandChars_v2(const char * src,string_base & out);//for other controls
//gotta hate MS for shoving ampersand=>underline conversion into random things AND making each of them use different rules for inserting ampersand char instead of underline


//deprecated
SHARED_EXPORT UINT uPrintCrashInfo(LPEXCEPTION_POINTERS param,const char * extrainfo,char * out);
#define uPrintCrashInfo_max_length 1024

SHARED_EXPORT void uPrintCrashInfo_Init(const char * name);//called only by exe on startup
SHARED_EXPORT void uPrintCrashInfo_AddInfo(const char * p_info);//called only by exe on startup
SHARED_EXPORT void uPrintCrashInfo_SetDumpPath(const char * name);//called only by exe on startup


SHARED_EXPORT void uDumpCrashInfo(LPEXCEPTION_POINTERS param);

SHARED_EXPORT BOOL uListBox_GetText(HWND listbox,UINT index,string_base & out);

SHARED_EXPORT void uPrintf(string_base & out,const char * fmt,...);
SHARED_EXPORT void uPrintfV(string_base & out,const char * fmt,va_list arglist);

class NOVTABLE uResource
{
public:
	virtual const void * GetPointer() = 0;
	virtual unsigned GetSize() = 0;
	virtual ~uResource() {}
};

SHARED_EXPORT uResource * uLoadResource(HMODULE hMod,const char * name,const char * type,WORD wLang = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL) );
SHARED_EXPORT uResource * LoadResourceEx(HMODULE hMod,const TCHAR * name,const TCHAR * type,WORD wLang = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL) );
SHARED_EXPORT HRSRC uFindResource(HMODULE hMod,const char * name,const char * type,WORD wLang = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL) );

SHARED_EXPORT BOOL uLoadString(HINSTANCE ins,UINT id,string_base & out);

SHARED_EXPORT UINT uCharLower(UINT c);
SHARED_EXPORT UINT uCharUpper(UINT c);

SHARED_EXPORT BOOL uGetMenuString(HMENU menu,UINT id,string_base & out,UINT flag);
SHARED_EXPORT BOOL uModifyMenu(HMENU menu,UINT id,UINT flags,UINT newitem,const char * data);
SHARED_EXPORT UINT uGetMenuItemType(HMENU menu,UINT position);


}//extern "C"

inline char * uCharNext(char * src) {return src+uCharLength(src);}
inline const char * uCharNext(const char * src) {return src+uCharLength(src);}


class string_utf8_from_window
{
public:
	string_utf8_from_window(HWND wnd)
	{
		uGetWindowText(wnd,m_data);
	}
	string_utf8_from_window(HWND wnd,UINT id)
	{
		uGetDlgItemText(wnd,id,m_data);
	}
	inline operator const char * () const {return m_data.get_ptr();}
	inline unsigned length() const {return m_data.length();}
	inline bool is_empty() const {return length() == 0;}
	inline const char * get_ptr() const {return m_data.get_ptr();}
private:
	 string8 m_data;
};

#define uMAKEINTRESOURCE(x) ((const char*)LOWORD(x))

class critical_section
{
private:
	CRITICAL_SECTION sec;
	int count;
public:
	int enter() {EnterCriticalSection(&sec);return ++count;}
	int leave() {int rv = --count;LeaveCriticalSection(&sec);return rv;}
	int get_lock_count() {return count;}
	int get_lock_count_check() {enter();return leave();}
	inline void assert_locked() {assert(get_lock_count_check()>0);}
	inline void assert_not_locked() {assert(get_lock_count_check()==0);}
	critical_section() {InitializeCriticalSection(&sec);count=0;}
	~critical_section() {DeleteCriticalSection(&sec);}
#ifdef CRITICAL_SECTION_HAVE_TRYENTER
	bool try_enter() {return !!TryEnterCriticalSection(&sec);}
#endif
};

class c_insync
{
private:
	critical_section * ptr;
public:
	c_insync(critical_section * p) {ptr=p;ptr->enter();}
	c_insync(critical_section & p) {ptr=&p;ptr->enter();}
	~c_insync() {ptr->leave();}
};

#define insync(X) c_insync blah____sync(X)


class critical_section2	//smarter version, has try_enter()
{
private:
	HANDLE hMutex;
	int count;
public:
	int enter() {return enter_timeout(INFINITE);}
	int leave() {int rv = --count;ReleaseMutex(hMutex);return rv;}
	int get_lock_count() {return count;}
	int get_lock_count_check()
	{
		int val = try_enter();
		if (val>0) val = leave();
		return val;
	}
	int enter_timeout(DWORD t) {return WaitForSingleObject(hMutex,t)==WAIT_OBJECT_0 ? ++count : 0;}
	int try_enter() {return enter_timeout(0);}
	int check_count() {enter();return leave();}
	critical_section2()
	{
		hMutex = uCreateMutex(0,0,0);
		count=0;
	}
	~critical_section2() {CloseHandle(hMutex);}

	inline void assert_locked() {assert(get_lock_count_check()>0);}
	inline void assert_not_locked() {assert(get_lock_count_check()==0);}

};

class c_insync2
{
private:
	critical_section2 * ptr;
public:
	c_insync2(critical_section2 * p) {ptr=p;ptr->enter();}
	c_insync2(critical_section2 & p) {ptr=&p;ptr->enter();}
	~c_insync2() {ptr->leave();}
};

#define insync2(X) c_insync2 blah____sync2(X)


//other
static inline HWND uCreateDialog(HINSTANCE hIns,UINT id,HWND parent,DLGPROC proc,long param=0) {return CreateDialogParam(hIns,MAKEINTRESOURCE(id),parent,proc,param);}
static inline int uDialogBox(HINSTANCE hIns,UINT id,HWND parent,DLGPROC proc,long param=0) {return DialogBoxParam(hIns,MAKEINTRESOURCE(id),parent,proc,param);}

#define uIsDialogMessage IsDialogMessage
#define uGetMessage GetMessage
#define uPeekMessage PeekMessage
#define uDispatchMessage DispatchMessage

#define uCallWindowProc CallWindowProc
#define uDefWindowProc DefWindowProc
#define uGetWindowLong GetWindowLong
#define uSetWindowLong SetWindowLong

#define uEndDialog EndDialog
#define uDestroyWindow DestroyWindow
#define uGetDlgItem GetDlgItem
#define uEnableWindow EnableWindow
#define uGetDlgItemInt GetDlgItemInt
#define uSetDlgItemInt SetDlgItemInt
#define uHookWindowProc(WND,PROC) ((WNDPROC)uSetWindowLong(WND,GWL_WNDPROC,(long)(PROC)))
#define uCreateToolbarEx CreateToolbarEx
#define uIsBadStringPtr IsBadStringPtrA
#define uSendMessage SendMessage
#define uSendDlgItemMessage SendDlgItemMessage
#define uSendMessageTimeout SendMessageTimeout
#define uSendNotifyMessage SendNotifyMessage
#define uSendMessageCallback SendMessageCallback
#define uPostMessage PostMessage
#define uPostThreadMessage PostThreadMessage


class string_print_crash
{
	char block[uPrintCrashInfo_max_length];
public:
	inline operator const char * () const {return block;}
	inline const char * get_ptr() const {return block;}
	inline unsigned length() {return strlen(block);}
	inline string_print_crash(LPEXCEPTION_POINTERS param,const char * extrainfo = 0) {uPrintCrashInfo(param,extrainfo,block);}
};

class uStringPrintf
{
public:
	inline explicit uStringPrintf(const char * fmt,...)
	{
		va_list list;
		va_start(list,fmt);
		uPrintfV(m_data,fmt,list);
		va_end(list);
	}
	inline operator const char * () const {return m_data.get_ptr();}
	inline unsigned length() const {return m_data.length();}
	inline bool is_empty() const {return length() == 0;}
	inline const char * get_ptr() const {return m_data.get_ptr();}
private:
	string8_fastalloc m_data;
};

inline LRESULT uButton_SetCheck(HWND wnd,UINT id,bool state) {return uSendDlgItemMessage(wnd,id,BM_SETCHECK,state ? BST_CHECKED : BST_UNCHECKED,0); }
inline bool uButton_GetCheck(HWND wnd,UINT id) {return uSendDlgItemMessage(wnd,id,BM_GETCHECK,0,0) == BST_CHECKED;}

class SHARED_EXPORT uCallStackTracker
{
	unsigned param;
public:
	explicit uCallStackTracker(const char * name);
	~uCallStackTracker();
};

extern "C"
{
	SHARED_EXPORT const char * uGetCallStackPath();
}

#define TRACK_CALL(X) uCallStackTracker TRACKER__##X(#X)
#define TRACK_CALL_TEXT(X) uCallStackTracker TRACKER__BLAH(X)
#define TRACK_CODE(X,Y) {uCallStackTracker __call_tracker(X); Y;}


extern "C" {
SHARED_EXPORT int stricmp_utf8(const char * p1,const char * p2);
SHARED_EXPORT int stricmp_utf8_ex(const char * p1,unsigned len1,const char * p2,unsigned len2);
SHARED_EXPORT int stricmp_utf8_stringtoblock(const char * p1,const char * p2,unsigned p2_bytes);
SHARED_EXPORT int stricmp_utf8_partial(const char * p1,const char * p2,unsigned num = (unsigned)(-1));
SHARED_EXPORT int stricmp_utf8_max(const char * p1,const char * p2,unsigned p1_bytes);
SHARED_EXPORT unsigned uReplaceStringAdd(string_base & out,const char * src,unsigned src_len,const char * s1,unsigned len1,const char * s2,unsigned len2,bool casesens);
SHARED_EXPORT unsigned uReplaceCharAdd(string_base & out,const char * src,unsigned src_len,unsigned c1,unsigned c2,bool casesens);
//all lengths in uReplaceString functions are optional, set to -1 if parameters is a simple null-terminated string
SHARED_EXPORT void uAddStringLower(string_base & out,const char * src,unsigned len = (unsigned)(-1));
SHARED_EXPORT void uAddStringUpper(string_base & out,const char * src,unsigned len = (unsigned)(-1));
}

inline void uStringLower(string_base & out,const char * src,unsigned len = (unsigned)(-1)) {out.reset();uAddStringLower(out,src,len);}
inline void uStringUpper(string_base & out,const char * src,unsigned len = (unsigned)(-1)) {out.reset();uAddStringUpper(out,src,len);}

inline unsigned uReplaceString(string_base & out,const char * src,unsigned src_len,const char * s1,unsigned len1,const char * s2,unsigned len2,bool casesens)
{
	out.reset();
	return uReplaceStringAdd(out,src,src_len,s1,len1,s2,len2,casesens);
}

inline unsigned uReplaceChar(string_base & out,const char * src,unsigned src_len,unsigned c1,unsigned c2,bool casesens)
{
	out.reset();
	return uReplaceCharAdd(out,src,src_len,c1,c2,casesens);
}

class string_lower
{
public:
	explicit string_lower(const char * ptr,unsigned num=-1) {uAddStringLower(m_data,ptr,num);}
	inline operator const char * () const {return m_data.get_ptr();}
	inline unsigned length() const {return m_data.length();}
	inline bool is_empty() const {return length() == 0;}
	inline const char * get_ptr() const {return m_data.get_ptr();}
private:
	string8 m_data;
};

class string_upper
{
public:
	explicit string_upper(const char * ptr,unsigned num=-1) {uAddStringUpper(m_data,ptr,num);}
	inline operator const char * () const {return m_data.get_ptr();}
	inline unsigned length() const {return m_data.length();}
	inline bool is_empty() const {return length() == 0;}
	inline const char * get_ptr() const {return m_data.get_ptr();}
private:
	string8 m_data;
};

inline UINT char_lower(UINT c) {return uCharLower(c);}
inline UINT char_upper(UINT c) {return uCharUpper(c);}
#define char_lower uCharLower
#define char_upper uCharUpper

inline BOOL uGetLongPathNameEx(const char * name,string_base & out)
{
	if (uGetLongPathName(name,out)) return TRUE;
	return uGetFullPathName(name,out);
}

struct t_font_description
{
	enum
	{
		m_facename_length = LF_FACESIZE*2,
		m_height_dpi = 480,
	};

	t_uint32 m_height;
	t_uint32 m_weight;
	t_uint8 m_italic;
	t_uint8 m_charset;
	char m_facename[m_facename_length];

	SHARED_EXPORT HFONT create();
	SHARED_EXPORT bool popup_dialog(HWND p_parent);
	SHARED_EXPORT void from_font(HFONT p_font);
	SHARED_EXPORT static t_font_description g_from_font(HFONT p_font);
};


struct t_modal_dialog_entry
{
	HWND m_wnd_to_poke;
	bool m_in_use;
};

extern "C" {
	SHARED_EXPORT void ModalDialog_Switch(t_modal_dialog_entry & p_entry);
	SHARED_EXPORT void ModalDialog_PokeExisting();
	SHARED_EXPORT bool ModalDialog_CanCreateNew();

	SHARED_EXPORT HWND FindOwningPopup(HWND p_wnd);
	SHARED_EXPORT void PokeWindow(HWND p_wnd);
};

class modal_dialog_scope
{
public:
	inline modal_dialog_scope(HWND p_wnd) : m_initialized(false) {initialize(p_wnd);}
	inline modal_dialog_scope() : m_initialized(false) {}
	inline ~modal_dialog_scope() {deinitialize();}

	inline static bool can_create() {return ModalDialog_CanCreateNew();}
	inline static void poke_existing() {ModalDialog_PokeExisting();}

	void initialize(HWND p_wnd)
	{
		if (!m_initialized)
		{
			m_initialized = true;
			m_entry.m_in_use = true;
			m_entry.m_wnd_to_poke = p_wnd;
			ModalDialog_Switch(m_entry);
		}
	}

	void deinitialize()
	{
		if (m_initialized)
		{
			ModalDialog_Switch(m_entry);
			m_initialized = false;
		}
	}

	

private:
	modal_dialog_scope(const modal_dialog_scope & p_scope) {assert(0);}
	const modal_dialog_scope & operator=(const modal_dialog_scope &) {assert(0); return *this;}

	t_modal_dialog_entry m_entry;

	bool m_initialized;
};

#endif //_UTF8API_DLL__UTF8API_H_