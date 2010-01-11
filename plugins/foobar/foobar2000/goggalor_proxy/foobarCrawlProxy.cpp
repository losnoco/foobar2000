// foobarCrawlProxy.cpp : Implementation of CfoobarCrawlProxy

#include "stdafx.h"
#include "foobarCrawlProxy.h"

#include "../foo_goggalor/foo_goggalor.h"


// CfoobarCrawlProxy

STDMETHODIMP CfoobarCrawlProxy::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IfoobarCrawlProxy
	};

	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CfoobarCrawlProxy::HandleFile(BSTR path, IDispatch* event_factory)
{
	CComPtr<IfoobarCrawlPlugin> m_Plugin;

	HRESULT hr = m_Plugin.CoCreateInstance( CLSID_foobarCrawlPlugin );
	if ( FAILED( hr ) )
		return Error( "Failed to connect to foobar2000 GDS COM server", GUID_NULL, hr );

	return m_Plugin->HandleFile( path, event_factory );
}
