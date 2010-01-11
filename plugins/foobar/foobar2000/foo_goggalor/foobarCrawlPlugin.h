// foobarCrawlPlugin.h : Declaration of the CfoobarCrawlPlugin

#pragma once
#include "resource.h"       // main symbols

#include "foo_goggalor.h"


#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif



// CfoobarCrawlPlugin

class ATL_NO_VTABLE CfoobarCrawlPlugin :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CfoobarCrawlPlugin, &CLSID_foobarCrawlPlugin>,
	public ISupportErrorInfo,
	public IDispatchImpl<IfoobarCrawlPlugin, &IID_IfoobarCrawlPlugin, &LIBID_foo_goggalorLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
public:
	CfoobarCrawlPlugin()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_FOOBARCRAWLPLUGIN)


BEGIN_COM_MAP(CfoobarCrawlPlugin)
	COM_INTERFACE_ENTRY(IfoobarCrawlPlugin)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);


	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}

public:

	STDMETHOD(HandleFile)(BSTR path, IDispatch* event_factory);
};

OBJECT_ENTRY_AUTO(__uuidof(foobarCrawlPlugin), CfoobarCrawlPlugin)
