/// <summary>
///  Snarl C++ interface implementation
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
///  Funtions special to C++ V41 API compared to VB version:
///    GetLastMsgToken()
///    GetAppPath()
///    GetIconsPath()
/// </summary>
///----------------------------------------------------------------------------
/// <example>
/// SnarlInterface snarl;
/// snarl.RegisterApp(_T("CppTest"), _T("C++ test app"), NULL);
/// snarl.AddClass(_T("Class1"), _T("Class 1"));
/// snarl.EZNotify(_T("Class1"), _T("C++ example 1"), _T("Some text"), 10);
/// snarl.UnregisterApp();
///
/// Please see the SimpleTest.cpp and SnarlV41Test.cpp for more example code.
/// </example>
///----------------------------------------------------------------------------
/// <VersionHistory>
///  2011-01-02 : mingw-w64 patch by Patrick von Reth
///  2010-08-13 : First release of V41 Snarl API implementation
/// </VersionHistory>

#define _CRT_SECURE_NO_WARNINGS

#include "SnarlInterface.h"


namespace Snarl {
namespace V41 {


// workaround for mingw-w64 bug
#ifdef __MINGW64_VERSION_MAJOR
	extern "C" {
		__declspec(dllimport) errno_t __cdecl strncat_s(char *_Dst, size_t _DstSizeInChars, const char *_Src, size_t _MaxCount);
	}
#endif //__MINGW64_VERSION_MAJOR


//-----------------------------------------------------------------------------
// Constructor/Destructor
//-----------------------------------------------------------------------------
SnarlInterface::SnarlInterface()
	: appToken(0), lastMsgToken(0), localError(SnarlEnums::Success), v42Version(0)
{
}

SnarlInterface::~SnarlInterface()
{
}

// ----------------------------------------------------------------------------

LONG32 SnarlInterface::RegisterApp(LPCSTR signature, LPCSTR title, LPCSTR icon, HWND hWndReply /* = NULL */, LONG32 msgReply /* = 0 */, SnarlEnums::AppFlags flags /* = SnarlEnums::AppDefault */)
{
	v42Version = SendRequest("version");

	if (v42Version >= 42)
	{
		LPSTR msg = PackData("reg?app-sig=%s&title=%s&icon=%s&reply-to=%d&reply=%d&flags=%d", signature, title, icon, hWndReply, msgReply, flags);
		if (!msg)
		{
			localError = SnarlEnums::ErrorFailed;
			return 0;
		}
		appToken = SendRequest(msg);
		delete [] msg;

		if (!appToken && localError == SnarlEnums::Success) localError = SnarlEnums::ErrorNotRunning;
	}
	else
	{
		SnarlMessage msg;
		msg.Command = SnarlEnums::RegisterApp;
		msg.Token = 0;
		PackData(msg.PacketData, 
			"id::%s#?title::%s#?icon::%s#?hwnd::%d#?umsg::%d#?flags::%d", 
			signature, title, icon, hWndReply, msgReply, flags);

		appToken = Send(msg);
	}

	lastMsgToken = 0;

	return appToken;
}

LONG32 SnarlInterface::RegisterApp(LPCWSTR signature, LPCWSTR title, LPCWSTR icon, HWND hWndReply /* = NULL */, LONG32 msgReply /* = 0 */, SnarlEnums::AppFlags flags /* = SnarlEnums::AppDefault */)
{
	LPCSTR szParam1 = WideToUTF8(signature);
	LPCSTR szParam2 = WideToUTF8(title);
	LPCSTR szParam3 = WideToUTF8(icon);
	
	LONG32 result = RegisterApp(szParam1, szParam2, szParam3, hWndReply, msgReply, flags);
	
	delete [] szParam1;
	delete [] szParam2;
	delete [] szParam3;

	return result;
}

LONG32 SnarlInterface::UnregisterApp()
{
	LONG32 rval;

	if (v42Version >= 42)
	{
		LPSTR msg = PackData("unreg?token=%d", appToken);
		if (!msg)
		{
			localError = SnarlEnums::ErrorFailed;
			return 0;
		}
		rval = SendRequest(msg);
		delete [] msg;

		rval = rval ? 0 : -1;
	}
	else
	{
		SnarlMessage msg;
		msg.Command = SnarlEnums::UnregisterApp;
		msg.Token = appToken;
		PackData(msg.PacketData, NULL);

		rval = Send(msg);
	}

	appToken = 0;
	lastMsgToken = 0;

	return rval;
}

LONG32 SnarlInterface::UpdateApp(LPCSTR title /* = NULL */, LPCSTR icon /* = NULL */)
{
	if (title == NULL && icon == NULL)
		return 0;

	bool has_title = title != NULL && title[0] != 0;
	bool has_icon = icon != NULL && icon[0] != 0;

	LONG32 rval;

	if (v42Version >= 42)
	{
		LPSTR msg = PackData("updateapp?token=%d%s%s%s%s", has_title ? "&title=" : "", has_title ? title : "", has_icon ? "&icon=" : "", has_icon ? icon : "");
		if (!msg)
		{
			localError = SnarlEnums::ErrorFailed;
			return 0;
		}

		rval = SendRequest(msg);
		delete [] msg;

		rval = rval ? 0 : -1;
	}
	else
	{
		SnarlMessage msg;
		msg.Command = SnarlEnums::UpdateApp;
		msg.Token = appToken;

		// TODO: Uckly code ahead
		if (has_title && has_icon)
			PackData(msg.PacketData, "title::%s#?icon::%s", title, icon);
		else if (has_title)
			PackData(msg.PacketData, "title::%s", title);
		else if (has_icon)
			PackData(msg.PacketData, "icon::%s", icon);
	
		rval = Send(msg);
	}

	return rval;
}

LONG32 SnarlInterface::UpdateApp(LPCWSTR title /* = NULL */, LPCWSTR icon /* = NULL */)
{
	LPCSTR szParam1 = WideToUTF8(title);
	LPCSTR szParam2 = WideToUTF8(icon);
	
	LONG32 result = UpdateApp(szParam1, szParam2);
	
	delete [] szParam1;
	delete [] szParam2;

	return result;
}

LONG32 SnarlInterface::AddClass(LPCSTR className, LPCSTR description, bool enabled /* = true */)
{
	LONG32 rval;

	if (v42Version >= 42)
	{
		LPSTR msg = PackData("addclass?token=%d&id=%s&name=%s&enabled=%d", appToken, className, description, (enabled ? 1 : 0));
		if (!msg)
		{
			localError = SnarlEnums::ErrorFailed;
			return 0;
		}
		rval = SendRequest(msg);
		delete [] msg;

		rval = rval ? 0 : -1;
	}
	else
	{
		SnarlMessage msg;
		msg.Command = SnarlEnums::AddClass;
		msg.Token = appToken;
		PackData(msg.PacketData, "id::%s#?name::%s#?enabled::%d", className, description, (enabled ? 1 : 0));
		rval = Send(msg);
	}

	return rval;
}

LONG32 SnarlInterface::AddClass(LPCWSTR className, LPCWSTR description, bool enabled /* = true */)
{
	LPCSTR szParam1 = WideToUTF8(className);
	LPCSTR szParam2 = WideToUTF8(description);
	
	LONG32 result = AddClass(szParam1, szParam2, enabled);
	
	delete [] szParam1;
	delete [] szParam2;

	return result;
}

LONG32 SnarlInterface::RemoveClass(LPCSTR className, bool forgetSettings /* = false */)
{
	LONG32 rval;

	if (v42Version >= 42)
	{
		LPSTR msg = PackData("remclass?token=%d&id=%s&forget=%d", appToken, className, (forgetSettings ? 1 : 0));
		if (!msg)
		{
			localError = SnarlEnums::ErrorFailed;
			return 0;
		}
		rval = SendRequest(msg);
		delete [] msg;

		rval = rval ? 0 : -1;
	}
	else
	{
		SnarlMessage msg;
		msg.Command = SnarlEnums::RemoveClass;
		msg.Token = appToken;
		PackData(msg.PacketData, "id::%s#?forget::%d", className, (forgetSettings ? 1 : 0));

		rval = Send(msg);
	}

	return rval;
}

LONG32 SnarlInterface::RemoveClass(LPCWSTR className, bool forgetSettings /* = false */)
{
	LPCSTR szParam1 = WideToUTF8(className);
	
	LONG32 result = RemoveClass(szParam1, forgetSettings);
	
	delete [] szParam1;

	return result;
}

LONG32 SnarlInterface::RemoveAllClasses(bool forgetSettings /* = false */)
{
	LONG32 rval;

	if (v42Version >= 42)
	{
		LPSTR msg = PackData("clearclasses?token=%d&forget=%d", appToken, (forgetSettings ? 1 : 0));
		if (!msg)
		{
			localError = SnarlEnums::ErrorFailed;
			return 0;
		}
		rval = SendRequest(msg);
		delete [] msg;

		rval = rval ? 0 : -1;
	}
	else
	{
		SnarlMessage msg;
		msg.Command = SnarlEnums::RemoveClass;
		msg.Token = appToken;
		PackData(msg.PacketData, "all::1#?forget::%d", (forgetSettings ? 1 : 0));

		rval = Send(msg);
	}

	return rval;
}

LONG32 SnarlInterface::AddAction(LONG32 token, LPCSTR label /* = NULL */, LPCSTR command /* = NULL */)
{
	if ( label == NULL || command == NULL )
		return 0;

	LONG32 rval;

	if (v42Version >= 42)
	{
		LPSTR msg = PackData("addaction?token=%d&label=%s&cmd=%s", token, label, command);
		if (!msg)
		{
			localError = SnarlEnums::ErrorFailed;
			return 0;
		}
		rval = SendRequest(msg);
		delete [] msg;

		rval = rval ? 0 : -1;
	}
	else
	{
		SnarlMessage msg;
		msg.Command = SnarlEnums::AddAction;
		msg.Token = token;

		// Create packed data
		errno_t err = 0;
		ZeroMemory(msg.PacketData, sizeof(msg.PacketData));
		char* pData = reinterpret_cast<char*>(msg.PacketData);

		err |= strncat_s(pData, SnarlPacketDataSize, (pData[0] != NULL) ? "#?label::" : "label::", _TRUNCATE); //StringCbCat(tmp, SnarlPacketDataSize, "title::%s");
		err |= strncat_s(pData, SnarlPacketDataSize, label, _TRUNCATE);
		err |= strncat_s(pData, SnarlPacketDataSize, (pData[0] != NULL) ? "#?command::" : "command::", _TRUNCATE); //StringCbCat(tmp, SnarlPacketDataSize, "title::%s");
		err |= strncat_s(pData, SnarlPacketDataSize, command, _TRUNCATE);

		// Check for strcat errors and exit on error
		if (err != 0) {
			localError = SnarlEnums::ErrorFailed;
			return 0;
		}

		rval = Send(msg);
	}

	return rval;
}

LONG32 SnarlInterface::RemoveAllActions(LONG32 token)
{
	LONG32 rval;

	if (v42Version >= 42)
	{
		LPSTR msg = PackData("clearactions?token=%d", token);
		if (!msg)
		{
			localError = SnarlEnums::ErrorFailed;
			return 0;
		}
		rval = SendRequest(msg);
		delete [] msg;

		rval = rval ? 0 : -1;
	}
	else
	{
		SnarlMessage msg;
		msg.Command = SnarlEnums::ClearActions;
		msg.Token = token;

		ZeroMemory(msg.PacketData, sizeof(msg.PacketData));

		rval = Send(msg);
	}

	return rval;
}

LONG32 SnarlInterface::EZNotify(LPCSTR className, LPCSTR title, LPCSTR text, LONG32 timeout /* = -1 */, LPCSTR icon /* = NULL */, LONG32 priority /* = 0 */, LPCSTR acknowledge /* = NULL */, LPCSTR value /* = NULL */)
{
	if (v42Version >= 42)
	{
		LPSTR msg = PackData("notify?token=%d&id=%s&title=%s&text=%s&timeout=%d%s%s&priority=%d%s%s%s%s", appToken, className, title, text, timeout, icon ? "&icon=" : "", icon ? icon : "", priority, acknowledge ? "&ack=" : "", acknowledge ? acknowledge : "", value ? "&value=" : "", value ? value : "");
		if (!msg)
		{
			localError = SnarlEnums::ErrorFailed;
			return 0;
		}
		lastMsgToken = SendRequest(msg);
		delete [] msg;
	}
	else
	{
		SnarlMessage msg;
		msg.Command = SnarlEnums::Notify;
		msg.Token = appToken;
		PackData(msg.PacketData,
			"id::%s#?title::%s#?text::%s#?timeout::%d#?icon::%s#?priority::%d#?ack::%s#?value::%s",
			className, title, text, timeout, (icon ? icon : ""), priority, (acknowledge ? acknowledge : ""), (value ? value : ""));

		lastMsgToken = Send(msg);
	}

	return lastMsgToken;
}

LONG32 SnarlInterface::EZNotify(LPCWSTR className, LPCWSTR title, LPCWSTR text, LONG32 timeout /* = -1 */, LPCWSTR icon /* = NULL */, LONG32 priority /* = 0 */, LPCWSTR acknowledge /* = NULL */, LPCWSTR value /* = NULL */)
{
	LPCSTR szParam1 = WideToUTF8(className);
	LPCSTR szParam2 = WideToUTF8(title);
	LPCSTR szParam3 = WideToUTF8(text);
	LPCSTR szParam4 = WideToUTF8(icon);
	LPCSTR szParam5 = WideToUTF8(acknowledge);
	LPCSTR szParam6 = WideToUTF8(value);
	
	LONG32 result = EZNotify(szParam1, szParam2, szParam3, timeout, szParam4, priority, szParam5, szParam6);
	
	delete [] szParam1; delete [] szParam2; delete [] szParam3;
	delete [] szParam4; delete [] szParam5; delete [] szParam6;

	return result;
}

LONG32 SnarlInterface::Notify(LPCSTR className, LPCSTR packetData)
{
	if (v42Version >= 42)
	{
		LPSTR msg = PackData("notify?token=%d&id=%s&%s", appToken, className, packetData);
		if (!msg)
		{
			localError = SnarlEnums::ErrorFailed;
			return 0;
		}
		lastMsgToken = SendRequest(msg);
		delete [] msg;
	}
	else
	{
		SnarlMessage msg;
		msg.Command = SnarlEnums::Notify;
		msg.Token = appToken;
		PackData(msg.PacketData, "id::%s#?%s", className, packetData);

		lastMsgToken = Send(msg);
	}

	return lastMsgToken;
}

LONG32 SnarlInterface::Notify(LPCWSTR className, LPCWSTR packetData)
{
	LPCSTR szParam1 = WideToUTF8(className);
	LPCSTR szParam2 = WideToUTF8(packetData);

	LONG32 result = Notify(szParam1, szParam2);
	
	delete [] szParam1; delete [] szParam2;
	
	return result;
}

LONG32 SnarlInterface::EZUpdate(LONG32 msgToken, LPCSTR title /* = NULL */, LPCSTR text /* = NULL */, LONG32 timeout /* = -1 */, LPCSTR icon /* = NULL */)
{
	bool has_title = title != NULL && title[0] != 0;
	bool has_text = text != NULL && text[0] != 0;
	bool has_icon = icon != NULL && icon[0] != 0;
	CHAR ttimeout[16];
	if (timeout != -1) _itoa_s(timeout, ttimeout, 10);

	LONG32 rval;

	if (v42Version >= 42)
	{
		LPSTR msg = PackData("update?token=%d%s%s%s%s%s%s%s%s", msgToken, has_title ? "&title=" : "", has_title ? title : "", has_text ? "&text=" : "", has_text ? text : "", has_icon ? "&icon=" : "", has_icon ? icon : "", timeout != -1 ? "&timeout=" : "", timeout != -1 ? ttimeout : "");
		if (!msg)
		{
			localError = SnarlEnums::ErrorFailed;
			return 0;
		}
		rval = SendRequest(msg);
		delete [] msg;

		rval = rval ? 0 : -1;
	}
	else
	{
		SnarlMessage msg;
		msg.Command = SnarlEnums::UpdateNotification;
		msg.Token = msgToken;
	
		// Create packed data
		errno_t err = 0;
		ZeroMemory(msg.PacketData, sizeof(msg.PacketData));
		char* pData = reinterpret_cast<char*>(msg.PacketData);

		if (has_title) {
			err |= strncat_s(pData, SnarlPacketDataSize, (pData[0] != NULL) ? "#?title::" : "title::", _TRUNCATE); //StringCbCat(tmp, SnarlPacketDataSize, "title::%s");
			err |= strncat_s(pData, SnarlPacketDataSize, title, _TRUNCATE);
		}
		if (has_text) {
			err |= strncat_s(pData, SnarlPacketDataSize, (pData[0] != NULL) ? "#?text::" : "text::", _TRUNCATE);
			err |= strncat_s(pData, SnarlPacketDataSize, text, _TRUNCATE);
		}
		if (has_icon) {
			err |= strncat_s(pData, SnarlPacketDataSize, (pData[0] != NULL) ? "#?icon::" : "icon::", _TRUNCATE);
			err |= strncat_s(pData, SnarlPacketDataSize, icon, _TRUNCATE);
		}
		if (timeout != -1) {
			err |= strncat_s(pData, SnarlPacketDataSize, (pData[0] != NULL) ? "#?timeout::" : "timeout::", _TRUNCATE);
			err |= strncat_s(pData, SnarlPacketDataSize, ttimeout, _TRUNCATE);
		}
	
		// Check for strcat errors and exit on error
		if (err != 0) {
			localError = SnarlEnums::ErrorFailed;
			return 0;
		}

		rval = Send(msg);
	}

	return rval;
}

LONG32 SnarlInterface::EZUpdate(LONG32 msgToken, LPCWSTR title /* = NULL */, LPCWSTR text /* = NULL */, LONG32 timeout /* = -1 */, LPCWSTR icon /* = NULL */)
{
	LPCSTR szParam1 = WideToUTF8(title);
	LPCSTR szParam2 = WideToUTF8(text);
	LPCSTR szParam3 = WideToUTF8(icon);

	LONG32 result = EZUpdate(msgToken, szParam1, szParam2, timeout, szParam3);
	
	delete [] szParam1; delete [] szParam2; delete [] szParam3;
	
	return result;
}

LONG32 SnarlInterface::Update(LONG32 msgToken, LPCSTR packetData)
{
	LONG32 rval;

	if (v42Version >= 42)
	{
		LPSTR msg = PackData("update?token=%d&%s", msgToken, packetData);
		if (!msg)
		{
			localError = SnarlEnums::ErrorFailed;
			return 0;
		}
		rval = SendRequest(msg);
		delete [] msg;

		rval = rval ? 0 : -1;
	}
	else
	{
		SnarlMessage msg;
		msg.Command = SnarlEnums::UpdateNotification;
		msg.Token = msgToken;
		PackData(msg.PacketData, packetData);

		rval = Send(msg);
	}

	return rval;
}

LONG32 SnarlInterface::Update(LONG32 msgToken, LPCWSTR packetData)
{
	LPCSTR szParam1 = WideToUTF8(packetData);

	LONG32 result = Update(msgToken, szParam1);
	
	delete [] szParam1;
	
	return result;
}

LONG32 SnarlInterface::Hide(LONG32 msgToken)
{
	LONG32 rval;

	if (v42Version >= 42)
	{
		LPSTR msg = PackData("hide?token=%d", msgToken);
		if (!msg)
		{
			localError = SnarlEnums::ErrorFailed;
			return 0;
		}

		LONG32 rval = SendRequest(msg);
		delete [] msg;

		rval = rval ? 0 : -1;
	}
	else
	{
		SnarlMessage msg;
		msg.Command = SnarlEnums::HideNotification;
		msg.Token = msgToken;
		PackData(msg.PacketData, NULL);

		rval = Send(msg);
	}

	return rval;
}

LONG32 SnarlInterface::IsVisible(LONG32 msgToken)
{
	LONG32 rval;

	if (v42Version >= 42)
	{
		LPSTR msg = PackData("isvisible?token=%d", msgToken);
		if (!msg)
		{
			localError = SnarlEnums::ErrorFailed;
			return 0;
		}
		rval = SendRequest(msg);
		delete [] msg;

		rval = rval <= 0 ? 0 : -1;
	}
	else
	{
		SnarlMessage msg;
		msg.Command = SnarlEnums::IsNotificationVisible;
		msg.Token = msgToken;
		PackData(msg.PacketData, NULL);

		rval = Send(msg);
	}

	return rval;
}

SnarlEnums::SnarlStatus SnarlInterface::GetLastError()
{
	return localError;
}

// static
BOOL SnarlInterface::IsSnarlRunning()
{
	return IsWindow(GetSnarlWindow());
}

LONG32 SnarlInterface::GetVersion()
{
	localError = SnarlEnums::Success;

	HWND hWnd = GetSnarlWindow();
	if (!IsWindow(hWnd))
	{
		localError = SnarlEnums::ErrorNotRunning;
		return 0;
	}

	HANDLE hProp = GetProp(hWnd, _T("_version"));
	return reinterpret_cast<int>(hProp);
}

// static
UINT SnarlInterface::Broadcast()
{
	return RegisterWindowMessage(SnarlGlobalMsg);
}

// static
UINT SnarlInterface::AppMsg()
{
	return RegisterWindowMessage(SnarlAppMsg);
}

// static
HWND SnarlInterface::GetSnarlWindow()
{
	return FindWindow(SnarlWindowClass, SnarlWindowTitle);;
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
	// TODO: _tcsnlen MAX_PATH
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

LONG32 SnarlInterface::GetLastMsgToken() const
{
	return lastMsgToken;
}


//-----------------------------------------------------------------------------
// Private functions 
//-----------------------------------------------------------------------------

LONG32 SnarlInterface::Send(SnarlMessage msg)
{
	DWORD_PTR nReturn = 0; // Failure

	HWND hWnd = GetSnarlWindow();
	if (!IsWindow(hWnd))
	{
		localError = SnarlEnums::ErrorNotRunning;
		return 0;
	}

	COPYDATASTRUCT cds;
	cds.dwData = 0x534E4C02; // "SNL",2;
	cds.cbData = sizeof(SnarlMessage);
	cds.lpData = &msg;

	if (SendMessageTimeout(hWnd, WM_COPYDATA, (WPARAM)GetCurrentProcessId(), (LPARAM)&cds, SMTO_ABORTIFHUNG | SMTO_NOTIMEOUTIFNOTHUNG, 500, &nReturn) == 0)
	{
		// return zero on failure
		if (GetLastError() == ERROR_TIMEOUT)
			localError = SnarlEnums::ErrorTimedOut;
		else
			localError = SnarlEnums::ErrorFailed;
		
		return 0;
	}

	// return result and cache LastError
	HANDLE hProp = GetProp(hWnd, _T("last_error"));
	localError = static_cast<SnarlEnums::SnarlStatus>(reinterpret_cast<int>(hProp));

	return nReturn;
}

//-----------------------------------------------------------------------------

LONG32 SnarlInterface::SendRequest(LPCSTR szMsg)
{
	DWORD_PTR nReturn = 0; // Failure

	HWND hWnd = GetSnarlWindow();
	if (!IsWindow(hWnd))
	{
		localError = SnarlEnums::ErrorNotRunning;
		return 0;
	}

	COPYDATASTRUCT cds;
	cds.dwData = 0x534E4C03; // "SNL",3;
	cds.cbData = lstrlenA(szMsg) + 1;
	cds.lpData = (PVOID) szMsg;

	if (SendMessageTimeout(hWnd, WM_COPYDATA, (WPARAM)GetCurrentProcessId(), (LPARAM)&cds, SMTO_ABORTIFHUNG | SMTO_NOTIMEOUTIFNOTHUNG, 500, &nReturn) == 0)
	{
		// return zero on failure
		if (GetLastError() == ERROR_TIMEOUT)
			localError = SnarlEnums::ErrorTimedOut;
		else
			localError = SnarlEnums::ErrorFailed;
		
		return 0;
	}

	if (nReturn < 0) localError = (SnarlEnums::SnarlStatus)-nReturn;
	else localError = SnarlEnums::Success;

	return nReturn;
}

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

void SnarlInterface::PackData(BYTE* data, LPCSTR format, ...)
{
	// Always zero array - Used to clear the array in member functions
	ZeroMemory(data, SnarlPacketDataSize);

	// Return if format string is empty
	if (format == NULL || format[0] == 0)
		return;

	int cchStrTextLen = 0;
	va_list args;
	va_start(args, format);
	
	// Get size of buffer
	cchStrTextLen = _vscprintf(format, args) + 1; // + NULL
	if (cchStrTextLen <= 1)
		return;

	// Create formated string - _TRUNCATE will ensure zero terminated
	_vsnprintf_s((char*)data, SnarlPacketDataSize, _TRUNCATE, format, args);

	va_end(args);
}

LPSTR SnarlInterface::PackData(LPCSTR format, ...)
{
	// Return if format string is empty
	if (format == NULL || format[0] == 0)
		return NULL;

	int cchStrTextLen = 0;
	va_list args;
	va_start(args, format);
	
	// Get size of buffer
	cchStrTextLen = _vscprintf(format, args) + 1; // + NULL
	if (cchStrTextLen <= 1)
		return NULL;

	// Allocate the string
	LPSTR rval = new char[cchStrTextLen];

	// Create formated string - _TRUNCATE will ensure zero terminated
	_vsnprintf_s((char*)rval, cchStrTextLen, _TRUNCATE, format, args);

	va_end(args);

	return rval;
}

}} // namespace Snarl::V41
