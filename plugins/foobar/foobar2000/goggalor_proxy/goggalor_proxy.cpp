// goggalor_proxy.cpp : Implementation of DLL Exports.


#include "stdafx.h"
#include "resource.h"
#include "goggalor_proxy.h"


class Cgoggalor_proxyModule : public CAtlDllModuleT< Cgoggalor_proxyModule >
{
public :
	DECLARE_LIBID(LIBID_goggalor_proxyLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_GOGGALOR_PROXY, "{7AFF8E1B-9C4E-482A-B8AB-51241E74BE30}")
};

Cgoggalor_proxyModule _AtlModule;


#ifdef _MANAGED
#pragma managed(push, off)
#endif

// DLL Entry Point
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	hInstance;
    return _AtlModule.DllMain(dwReason, lpReserved); 
}

#ifdef _MANAGED
#pragma managed(pop)
#endif




// Used to determine whether the DLL can be unloaded by OLE
STDAPI DllCanUnloadNow(void)
{
    return _AtlModule.DllCanUnloadNow();
}


// Returns a class factory to create an object of the requested type
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
}


// DllRegisterServer - Adds entries to the system registry
STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    HRESULT hr = _AtlModule.DllRegisterServer();
	return hr;
}


// DllUnregisterServer - Removes entries from the system registry
STDAPI DllUnregisterServer(void)
{
	HRESULT hr = _AtlModule.DllUnregisterServer();
	return hr;
}

