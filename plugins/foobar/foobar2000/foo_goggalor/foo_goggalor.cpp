// foo_goggalor.cpp : Implementation of DLL Exports.


#include "stdafx.h"
#include "resource.h"
#include "foo_goggalor.h"
#include "foobarCrawlPlugin.h"

class Cfoo_goggalorModule : public CComModule
{
public:
	virtual HRESULT AddCommonRGSReplacements(IRegistrarBase* pRegistrar) throw()
	{
		HRESULT hr = CComModule::AddCommonRGSReplacements(pRegistrar);
		if (SUCCEEDED(hr))
		{
			string8 appmodule;
			uGetModuleFileName(NULL, appmodule);
			string_utf16_from_utf8 ole_appmodule(appmodule);
			hr = pRegistrar->AddReplacement(L"APPMODULE", ole_appmodule.get_ptr());
		}
		return hr;
	}
};

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_foobarCrawlPlugin, CfoobarCrawlPlugin)
END_OBJECT_MAP()

class commandline_handler_regsvr;
class initquit_regsvr;

const char * AtlFoobar2000GetRegSvrId()
{
	return core_api::get_my_file_name() + 4;
}

class uFormatMessageString : public string8
{
public:
	uFormatMessageString( HRESULT hr, HMODULE hModule = NULL )
	{
		PVOID buffer;
		if (IsUnicode())
		{
			if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | (hModule ? FORMAT_MESSAGE_FROM_HMODULE : FORMAT_MESSAGE_FROM_SYSTEM), hModule, hr, 0, (LPWSTR)&buffer, 256, 0))
			{
				set_string_utf16((const WCHAR *)buffer);
				LocalFree(buffer);
			}
		}
		else
		{
			if (FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | (hModule ? FORMAT_MESSAGE_FROM_HMODULE : FORMAT_MESSAGE_FROM_SYSTEM), hModule, hr, 0, (LPSTR)&buffer, 256, 0))
			{
				set_string_ansi((const char *)buffer);
				LocalFree(buffer);
			}
		}
	}
};

void AtlFoobar2000DisplayResult(const char * action, HRESULT hr)
{
	if (SUCCEEDED(hr))
	{
		if (hr == S_OK)
			console::info( uStringPrintf( "%s succeeded.", action ) );
		else
			console::info( uStringPrintf( "%s failed. (%s)", action, uFormatMessageString( hr ) ) );
	}
	else
	{
		console::info( uStringPrintf( "%s failed. (%s)", action, uFormatMessageString( hr ) ) );
	}
}

Cfoo_goggalorModule _Module;

namespace ATL
{

static bool g_bInitialized = false;

class commandline_handler_regsvr : public commandline_handler
{
public:
	virtual result on_token(const char * token)
	{
		if (!stricmp_utf8_partial(token, "/regsvr:", infinite))
		{
			if (!stricmp_utf8(token + strlen("/regsvr:"), AtlFoobar2000GetRegSvrId()))
			{
				if (!g_bInitialized)
				{
					_Module.Init(ObjectMap, NULL, &LIBID_foo_goggalorLib);
					_Module.SetResourceInstance(core_api::get_my_instance());
					g_bInitialized = true;
				}

				HRESULT hr = _Module.RegisterServer(TRUE);

				AtlFoobar2000DisplayResult("Registration", hr);

				return RESULT_PROCESSED;
			}
		}
		else if (!stricmp_utf8_partial(token, "/unregsvr:", infinite))
		{
			if (!stricmp_utf8(token + strlen("/unregsvr:"), AtlFoobar2000GetRegSvrId()))
			{
				if (!g_bInitialized)
				{
					_Module.Init(ObjectMap, NULL, &LIBID_foo_goggalorLib);
					_Module.SetResourceInstance(core_api::get_my_instance());
					g_bInitialized = true;
				}

				HRESULT hr = _Module.UnregisterServer();

				AtlFoobar2000DisplayResult("Unregistration", hr);

				return RESULT_PROCESSED;
			}
		}
		else if (!stricmp_utf8(token, "-Embedding"))
		{
			// do nothing
			return RESULT_PROCESSED;
		}
		return RESULT_NOT_OURS;
	}
};

#define DECLARE_COMMANDLINE_HANDLER_REGSVR() \
	static commandline_handler_factory<commandline_handler_regsvr> g_cmdline_regsvr;

DECLARE_COMMANDLINE_HANDLER_REGSVR()

#define DECLARE_INITQUIT_REGSVR() \
	static initquit_factory<initquit_regsvr> g_initquit;

class initquit_regsvr : public initquit
{
public:
	virtual void on_init()
	{
		if (!g_bInitialized)
		{
			_Module.Init(ObjectMap, NULL, &LIBID_foo_goggalorLib);
			_Module.SetResourceInstance(core_api::get_my_instance());
			g_bInitialized = true;
		}

		HRESULT hr = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, REGCLS_MULTI_SEPARATE);
		AtlFoobar2000DisplayResult("Registering class objects", (hr == S_FALSE) ? S_OK : hr);
	}

	virtual void on_quit()
	{
		HRESULT hr = _Module.RevokeClassObjects();
		AtlFoobar2000DisplayResult("Registering class objects", hr);

		_Module.Term();
	}
};

DECLARE_INITQUIT_REGSVR()
}

DECLARE_COMPONENT_VERSION("Google Desktop Search interface", "0.1", 0)
