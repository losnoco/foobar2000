// foobarCrawlProxy.h : Declaration of the CfoobarCrawlProxy

#pragma once
#include "resource.h"       // main symbols

#include "goggalor_proxy.h"


#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif



// CfoobarCrawlProxy

class ATL_NO_VTABLE CfoobarCrawlProxy :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CfoobarCrawlProxy, &CLSID_foobarCrawlProxy>,
	public ISupportErrorInfo,
	public IDispatchImpl<IfoobarCrawlProxy, &IID_IfoobarCrawlProxy, &LIBID_goggalor_proxyLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
public:
	CfoobarCrawlProxy()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_FOOBARCRAWLPROXY)


BEGIN_COM_MAP(CfoobarCrawlProxy)
	COM_INTERFACE_ENTRY(IfoobarCrawlProxy)
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

OBJECT_ENTRY_AUTO(__uuidof(foobarCrawlProxy), CfoobarCrawlProxy)
