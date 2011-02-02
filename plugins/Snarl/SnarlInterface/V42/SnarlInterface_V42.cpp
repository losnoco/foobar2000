/// <summary>
///  Snarl C++ interface implementation
///  API version 42
///
///  http://sourceforge.net/apps/mediawiki/snarlwin/index.php?title=Windows_API
///  https://sourceforge.net/apps/mediawiki/snarlwin/index.php?title=Generic_API
///
///  Written and maintained by Toke Noer Nøttrup (toke@noer.it)
///
///  Please note the following changes compared to the VB6 (official API) dokumentation:
///  - Function names doesn't have the prefix "sn". Naming of constants and variables are
///    generally changed to follow Microsoft C# standard. This naming convention is kept for
///    the C++ version, to keep them alike.
///  - Grouped variables like SNARL_LAUNCHED, SNARL_QUIT is enums in SnarlEnums namespace.
///  - Message events like SNARL_NOTIFICATION_CLICKED, is found in SnarlEnums::MessageEvent.
///  - Please note that string functions return NULL when they fail and not an empty string.
///  - Some functions in the VB API takes an appToken as first parameter. This token is a
///    member variable in C++ version, so it is omitted from the functions.
///    (Always call RegisterApp as first function!)
///  - Functions manipulating messages (Update, Hide etc.) still takes a message token as
///    parameter, but you can get the last message token calling GetLastMsgToken();
///    Example: snarl.Hide(snarl.GetLastMsgToken());
///
///  The functions in SnarlInterface both have ANSI(UTF8) and UNICODE versions.
///  If the LPCWSTR (unicode) version of the functions are called, the strings
///  are converted to UTF8 by SnarlInterface before sent to Snarl. So using the
///  ANSI/UTF8/LPCSTR versions of the functions are faster!
///
///  --------------------------------------------------------------------------
///  Example:
///  <code>
/// 	const LPCTSTR APP_ID = _T("CppTest");
/// 
/// 	SnarlInterface snarl;
/// 	snarl.RegisterApp(APP_ID, _T("C++ test app"), NULL);
/// 
/// 	snarl.AddClass(_T("Class1"), _T("Some class description"));
/// 	snarl.EZNotify(_T("Class1"), _T("C++ example 1"), _T("Some text"), 10);
/// 
/// 	cout &lt;&lt; "Hit a key to unregister" &lt;&lt; std::endl;
/// 	_getch();
/// 	snarl.UnregisterApp(APP_ID);///
///  Please see the sample test code for me examples.
///  </code>
/// </summary>
///----------------------------------------------------------------------------
/// <VersionHistory>
///  2011-02-02 : First release of V42 Snarl API implementation
/// </VersionHistory>
///
/// <Todo>
///  - Implement Character Escaping (http://sourceforge.net/apps/mediawiki/snarlwin/index.php?title=Generic_API#Character_Escaping)
/// </Todo>


#define _CRT_SECURE_NO_WARNINGS

#include "SnarlInterface.h"


namespace Snarl {
namespace V42 {


// workaround for mingw-w64 bug
#ifdef __MINGW64_VERSION_MAJOR
	extern "C" {
		__declspec(dllimport) errno_t __cdecl strncat_s(char *_Dst, size_t _DstSizeInChars, const char *_Src, size_t _MaxCount);
	}
#endif //__MINGW64_VERSION_MAJOR


//-----------------------------------------------------------------------------

SnarlInterface::SnarlInterface()
	: appToken(0), lastMsgToken(0)
{
}

SnarlInterface::~SnarlInterface()
{
}

// ----------------------------------------------------------------------------
// Static Snarl interface functions
// ----------------------------------------------------------------------------

UINT SnarlInterface::AppMsg()
{
	return RegisterWindowMessage(SnarlAppMsg);
}

UINT SnarlInterface::Broadcast()
{
	return RegisterWindowMessage(SnarlGlobalMsg);
}

LONG32 SnarlInterface::DoRequest(LPCSTR request, UINT replyTimeout)
{
	DWORD_PTR nResult = 0;

	HWND hWnd = GetSnarlWindow();
	if (!IsWindow(hWnd))
		return -SnarlEnums::ErrorNotRunning;

	// Create COPYDATASTRUCT
	COPYDATASTRUCT cds;
	cds.dwData = 0x534E4C03;           // "SNL",3
	cds.cbData = strlen(request);      // No knowledge of max string lenght
	cds.lpData = const_cast<char*>(request);

	// Send message
	if (SendMessageTimeout(hWnd, WM_COPYDATA, (WPARAM)GetCurrentProcessId(), (LPARAM)&cds, SMTO_ABORTIFHUNG | SMTO_NOTIMEOUTIFNOTHUNG, replyTimeout, &nResult) == 0)
	{
		DWORD nError = GetLastError();
		if (nError == ERROR_TIMEOUT)
			nResult = -SnarlEnums::ErrorTimedOut;
		else
			nResult = -SnarlEnums::ErrorFailed;
	}

	return nResult;
}

LONG32 SnarlInterface::DoRequest(LPCWSTR request, UINT replyTimeout)
{
	DWORD_PTR nResult = 0;

	// Convert to UTF8
	LPSTR utf8Request = WideToUTF8(request);
	if (utf8Request == NULL)
		return -SnarlEnums::ErrorCppInterface;

	nResult = DoRequest(utf8Request, replyTimeout);

	// Cleanup and return result
	FreeString(utf8Request);
	return nResult;
}

LONG32 SnarlInterface::DoRequest(LPCWSTR request, SnarlParameterList<wchar_t>& spl, UINT replyTimeout)
{
	// <action>[?<data>=<value>[&<data>=<value>]]
	const std::vector<SnarlParameterList<wchar_t>::Type>&list = spl.GetList();

	if (list.size() > 0)
	{
		std::basic_string<wchar_t> requestStr = request;
		requestStr.append(L"?");

		std::vector<SnarlParameterList<wchar_t>::Type>::const_iterator listEnd = list.cend();
		for (std::vector<SnarlParameterList<wchar_t>::Type>::const_iterator iter = list.cbegin();
			iter != listEnd; ++iter)
		{
			SnarlParameterList<wchar_t>::Type pair = *iter;

			if (iter->second.length() > 0)
			{
				requestStr.append(iter->first).append(L"=").append(iter->second);
				requestStr.append(L"&");
			}
		}
		// Delete last &
		requestStr.erase(requestStr.size() - 1);

		return DoRequest(requestStr.c_str(), replyTimeout);
	}
	else
		return DoRequest(request, replyTimeout);
}

LONG32 SnarlInterface::DoRequest(LPCSTR request, SnarlParameterList<char>& spl, UINT replyTimeout)
{
	// <action>[?<data>=<value>[&<data>=<value>]]
	const std::vector<SnarlParameterList<char>::Type>&list = spl.GetList();

	if (list.size() > 0)
	{
		std::string requestStr = request;
		requestStr.append("?");

		std::vector<SnarlParameterList<char>::Type>::const_iterator listEnd = list.cend();
		for (std::vector<SnarlParameterList<char>::Type>::const_iterator iter = list.cbegin();
			iter != listEnd; ++iter)
		{
			SnarlParameterList<char>::Type pair = *iter;

			if (iter->second.length() > 0)
			{
				requestStr.append(iter->first).append("=").append(iter->second);
				requestStr.append("&");
			}
		}
		// Delete last &
		requestStr.erase(requestStr.size() - 1);

		return DoRequest(requestStr.c_str(), replyTimeout);
	}
	else
		return DoRequest(request, replyTimeout);
}

LPCTSTR SnarlInterface::GetAppPath()
{
	HWND hWnd = GetSnarlWindow();
	if (hWnd)
	{
		HWND hWndPath = FindWindowEx(hWnd, NULL, _T("static"), NULL);
		if (hWndPath)
		{
			TCHAR strTmp[MAX_PATH] = {0};
			int nReturn = GetWindowText(hWndPath, strTmp, MAX_PATH-1);
			if (nReturn > 0) {
				TCHAR* strReturn = AllocateString(nReturn + 1);
				_tcsncpy(strReturn, strTmp, nReturn + 1);
				strReturn[nReturn] = 0;
				return strReturn;
			}
		}
	}

	return NULL;
}

LPCTSTR SnarlInterface::GetIconsPath()
{
	TCHAR* szIconPath = NULL;
	LPCTSTR szPath = GetAppPath();
	if (!szPath)
		return NULL;

	size_t nLen = 0;
	if (nLen = _tcsnlen(szPath, MAX_PATH))
	{
		nLen += 10 + 1; // etc\\icons\\ + NULL
		szIconPath = AllocateString(nLen);

		_tcsncpy(szIconPath, szPath, nLen);
		_tcsncat(szIconPath, _T("etc\\icons\\"), nLen);
	}
	
	FreeString(szPath);

	return szIconPath;
}

HWND SnarlInterface::GetSnarlWindow()
{
	return FindWindow(SnarlWindowClass, SnarlWindowTitle);;
}

LONG32 SnarlInterface::GetVersion()
{
	return DoRequest(Requests::VersionA());
}

BOOL SnarlInterface::IsSnarlRunning()
{
	return IsWindow(GetSnarlWindow());
}


// --------------------------------------------------------------------------------------------
// SnarlInterface member functions
// --------------------------------------------------------------------------------------------

/// <param name="password">Optional</param>
/// <param name="hWndReplyTo">Optional</param>
/// <param name="msgReply">Optional</param>
/// <param name="appFlags">Optional (SnarlEnums::AppDefault)</param>
LONG32 SnarlInterface::RegisterApp(LPCSTR signature, LPCSTR name, LPCSTR icon, LPCSTR password, HWND hWndReplyTo, LONG32 msgReply, SnarlEnums::AppFlags appFlags)
{
	// reg?id=<signature>&title=<title>[&icon=<icon_path>][&password=<password>]

	SnarlParameterList<char> spl(7);
	spl.Add("app-sig", signature);
	spl.Add("title", name);
	spl.Add("icon", icon);
	spl.Add("password", password);
	spl.Add("reply-to", hWndReplyTo);
	spl.Add("reply", msgReply);
	spl.Add("flags", appFlags);

	LONG32 request = DoRequest(Requests::RegisterA(), spl);
	if (request > 0)
		appToken = request;

	return request;
}

LONG32 SnarlInterface::RegisterApp(LPCWSTR signature, LPCWSTR name, LPCWSTR icon, LPCWSTR password, HWND hWndReplyTo, LONG32 msgReply, SnarlEnums::AppFlags appFlags)
{
	SnarlParameterList<wchar_t> spl(7);
	spl.Add(L"app-sig", signature);
	spl.Add(L"title", name);
	spl.Add(L"icon", icon);
	spl.Add(L"password", password);
	spl.Add(L"reply-to", hWndReplyTo);
	spl.Add(L"reply", msgReply);
	spl.Add(L"flags", appFlags);

	LONG32 request = DoRequest(Requests::RegisterW(), spl);
	if (request > 0)
		appToken = request;

	return request;
}

/// <param name="password">Optional</param>
LONG32 SnarlInterface::UnregisterApp(LPCSTR signature, LPCSTR password)
{
	// unreg?token=<app_token>[&password=<password>]

	SnarlParameterList<char> spl(2);
	spl.Add("app-sig", signature);
	spl.Add("password", password);

	appToken = 0;
	lastMsgToken = 0;

	return DoRequest(Requests::UnregisterA(), spl);
}

LONG32 SnarlInterface::UnregisterApp(LPCWSTR signature, LPCWSTR password)
{
	SnarlParameterList<wchar_t> spl(2);
	spl.Add(L"app-sig", signature);
	spl.Add(L"password", password);

	appToken = 0;
	lastMsgToken = 0;

	return DoRequest(Requests::UnregisterW(), spl);
}

LONG32 SnarlInterface::UpdateApp(LPCSTR title, LPCSTR icon)
{
	SnarlParameterList<char> spl(3);
	spl.Add("token", appToken);
	spl.Add("title", title);
	spl.Add("icon", icon);

	return DoRequest(Requests::UpdateAppA(), spl);
}

LONG32 SnarlInterface::UpdateApp(LPCWSTR title, LPCWSTR icon)
{
	SnarlParameterList<wchar_t> spl(2);
	spl.Add(L"token", appToken);
	spl.Add(L"title", title);
	spl.Add(L"icon", icon);

	return DoRequest(Requests::UpdateAppW(), spl);
}

LONG32 SnarlInterface::AddAction(LONG32 msgToken, LPCSTR label, LPCSTR cmd, LPCSTR password)
{
	// addaction?[token=<token>|app-sig=<app-sig>&uid=<uid>]&label=<label>&cmd=<command>[&password=<password>]

	SnarlParameterList<char> spl(4);
	spl.Add("token", msgToken);
	spl.Add("label", label);
	spl.Add("cmd", cmd);
	spl.Add("password", password);

	return DoRequest(Requests::AddActionA(), spl);
}

LONG32 SnarlInterface::AddAction(LONG32 msgToken, LPCWSTR label, LPCWSTR cmd, LPCWSTR password)
{
	SnarlParameterList<wchar_t> spl(4);
	spl.Add(L"token", msgToken);
	spl.Add(L"label", label);
	spl.Add(L"cmd", cmd);
	spl.Add(L"password", password);

	return DoRequest(Requests::AddActionW(), spl);
}

LONG32 SnarlInterface::AddClass(LPCSTR classId, LPCSTR className, LPCSTR password)
{
	// addclass?token=<token>&id=<class_id>&name=<class_name>[&password=<password>]

	SnarlParameterList<char> spl(4);
	spl.Add("token", appToken);
	spl.Add("id", classId);
	spl.Add("name", className);
	spl.Add("password", password);

	return DoRequest(Requests::AddClassA(), spl);
}

LONG32 SnarlInterface::AddClass(LPCWSTR classId, LPCWSTR className, LPCWSTR password)
{
	SnarlParameterList<wchar_t> spl(4);
	spl.Add(L"token", appToken);
	spl.Add(L"id", classId);
	spl.Add(L"name", className);
	spl.Add(L"password", password);

	return DoRequest(Requests::AddClassW(), spl);
}

LONG32 SnarlInterface::RemoveClass(LPCSTR classId, LPCSTR password)
{
	// remclass?token=<app_token>&id=<class_id>[&password=<password>]

	SnarlParameterList<char> spl(3);
	spl.Add("token", appToken);
	spl.Add("id", classId);
	spl.Add("password", password);

	return DoRequest(Requests::RemoveClassA(), spl);
}

LONG32 SnarlInterface::RemoveClass(LPCWSTR classId, LPCWSTR password)
{
	SnarlParameterList<wchar_t> spl(3);
	spl.Add(L"token", appToken);
	spl.Add(L"id", classId);
	spl.Add(L"password", password);

	return DoRequest(Requests::RemoveClassW(), spl);
}

LONG32 SnarlInterface::KillClasses(LPCSTR password)
{
	// killclasses?token=<app_token>[&password=<password>]

	SnarlParameterList<char> spl(2);
	spl.Add("token", appToken);
	spl.Add("password", password);

	return DoRequest(Requests::KillClassesA(), spl);
}

LONG32 SnarlInterface::KillClasses(LPCWSTR password)
{
	SnarlParameterList<wchar_t> spl(2);
	spl.Add(L"token", appToken);
	spl.Add(L"password", password);

	return DoRequest(Requests::KillClassesW(), spl);
}

LONG32 SnarlInterface::GetLastMsgToken() const
{
	return lastMsgToken;
}

LONG32 SnarlInterface::EZNotify(LPCSTR classId, LPCSTR title, LPCSTR text, LONG32 timeout, LPCSTR iconPath, LPCSTR iconData, LONG32 priority, LPCSTR ack, LPCSTR password)
{
	// notify?token=<app_token>[&class=<class_id>][&title=<title>][&text=<text>][&timeout=<timeout>]
	//                         [&icondata=<icon_data>|icon=<icon_path>][&priority=<priority>]
	//                         [&ack=<acknowledge>][&password=<password>]

	SnarlParameterList<char> spl(10);
	spl.Add("token", appToken);
	spl.Add("class", classId);
	spl.Add("title", title);
	spl.Add("text", text);
	spl.Add("timeout", timeout);
	spl.Add("icon", iconPath);
	spl.Add("icondata", iconData);
	spl.Add("priority", priority);
	spl.Add("ack", ack);
	spl.Add("password", password);

	LONG32 request = DoRequest(Requests::NotifyA(), spl);
	lastMsgToken = (request > 0) ? request : 0;

	return request;
}

LONG32 SnarlInterface::EZNotify(LPCWSTR classId, LPCWSTR title, LPCWSTR text, LONG32 timeout, LPCWSTR iconPath, LPCWSTR iconData, LONG32 priority, LPCWSTR ack, LPCWSTR password)
{
	SnarlParameterList<wchar_t> spl(10);
	spl.Add(L"token", appToken);
	spl.Add(L"class", classId);
	spl.Add(L"title", title);
	spl.Add(L"text", text);
	spl.Add(L"timeout", timeout);
	spl.Add(L"icon", iconPath);
	spl.Add(L"icondata", iconData);
	spl.Add(L"priority", priority);
	spl.Add(L"ack", ack);
	spl.Add(L"password", password);

	LONG32 request = DoRequest(Requests::NotifyW(), spl);
	lastMsgToken = (request > 0) ? request : 0;

	return request;
}

LONG32 SnarlInterface::EZUpdate(LONG32 msgToken, LPCSTR classId, LPCSTR title, LPCSTR text, LONG32 timeout, LPCSTR iconPath, LPCSTR iconData, LONG32 priority, LPCSTR ack, LPCSTR password)
{
	// Made from best guess - no documentation available yet
	SnarlParameterList<char> spl(10);
	//spl.Add("token", appToken);
	spl.Add("token", msgToken);
	spl.Add("class", classId);
	spl.Add("title", title);
	spl.Add("text", text);
	spl.Add("timeout", timeout);
	spl.Add("icon", iconPath);
	spl.Add("icondata", iconData);
	spl.Add("priority", priority);
	spl.Add("ack", ack);
	spl.Add("password", password);

	return DoRequest(Requests::UpdateA(), spl);
}

LONG32 SnarlInterface::EZUpdate(LONG32 msgToken, LPCWSTR classId, LPCWSTR title, LPCWSTR text, LONG32 timeout, LPCWSTR iconPath, LPCWSTR iconData, LONG32 priority, LPCWSTR ack, LPCWSTR password)
{
	// Made from best guess - no documentation available yet
	SnarlParameterList<wchar_t> spl(10);
	//spl.Add("token", appToken);
	spl.Add(L"token", msgToken);
	spl.Add(L"class", classId);
	spl.Add(L"title", title);
	spl.Add(L"text", text);
	spl.Add(L"timeout", timeout);
	spl.Add(L"icon", iconPath);
	spl.Add(L"icondata", iconData);
	spl.Add(L"priority", priority);
	spl.Add(L"ack", ack);
	spl.Add(L"password", password);

	return DoRequest(Requests::UpdateW(), spl);
}

LONG32 SnarlInterface::Hide(LONG32 msgToken, LPCSTR password)
{
	// hide?token=<token>[&password=<password>]

	SnarlParameterList<char> spl(2);
	spl.Add("token", msgToken);
	spl.Add("password", password);

	return DoRequest(Requests::HideA(), spl);
}

LONG32 SnarlInterface::Hide(LONG32 msgToken, LPCWSTR password)
{
	SnarlParameterList<wchar_t> spl(2);
	spl.Add(L"token", msgToken);
	spl.Add(L"password", password);

	return DoRequest(Requests::HideW(), spl);
}

LONG32 SnarlInterface::IsVisible(LONG32 msgToken)
{
	// isvisible?token=<token>

	SnarlParameterList<char> spl(1);
	spl.Add("token", msgToken);

	return DoRequest(Requests::IsVisibleA(), spl);
}


//-----------------------------------------------------------------------------
// Private functions 
//-----------------------------------------------------------------------------

// Remember to delete [] returned string
inline
LPSTR SnarlInterface::WideToUTF8(LPCWSTR szWideStr)
{
	if (szWideStr == NULL)
		return NULL;

	int nSize = WideCharToMultiByte(CP_UTF8, 0, szWideStr, -1, NULL, 0, NULL, NULL);
	LPSTR szUTF8 = new char[nSize];
	WideCharToMultiByte(CP_UTF8, 0, szWideStr, -1, szUTF8, nSize, NULL, NULL);
	
	return szUTF8;
}

}} // namespace Snarl::V42
