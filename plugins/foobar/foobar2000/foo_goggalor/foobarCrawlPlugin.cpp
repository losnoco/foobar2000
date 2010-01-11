// foobarCrawlPlugin.cpp : Implementation of CfoobarCrawlPlugin

#include "stdafx.h"
#include "foobarCrawlPlugin.h"
#include "../goggalor_proxy/goggalor_proxy.h"

#include "foobar2000/send_info.h"

// CfoobarCrawlPlugin

STDMETHODIMP CfoobarCrawlPlugin::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IfoobarCrawlPlugin
	};

	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CfoobarCrawlPlugin::HandleFile(BSTR path, IDispatch* event_factory)
{
	// Event factory object to handle the passed in pointer from GDS.
	CComPtr<IGoogleDesktopSearchEventFactory> mcEventFactory;

	HRESULT hr;

	if ( ! core_api::are_services_available() )
		return Error(L"foobar2000 is not running");

	hr  = event_factory->QueryInterface(&mcEventFactory);  // Get the event factory
	if (FAILED(hr))
		return Error(L"No event factory", GUID_NULL, hr);

	string8 path8;

	path8 = "file://";
	path8.add_string_utf16( path );

	metadb_handle_list m_list;

	if ( ! static_api_ptr_t<playlist_incoming_item_filter>()->process_location( path8, m_list, false, NULL, NULL, GetDesktopWindow() ) )
		return Error(L"Error processing file", GUID_NULL, E_UNEXPECTED);

	try
	{
		hr = send_info( m_list, mcEventFactory );
	}
	catch ( error_info_t & e )
	{
		return Error( e.msg, GUID_NULL, e.hr );
	}

	// Get indexable content from the file.
	// Set the event object's properties.
	// Send the event object to GDS.

	return hr;
}
