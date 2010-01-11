#include "stdafx.h"
#include "../foo_goggalor.h"

#include "../../goggalor_proxy/goggalor_proxy.h"

static const GUID guid_cfg_registered = { 0xeb14fe6c, 0xc59b, 0x49a4, { 0x9b, 0xf6, 0x64, 0xd7, 0xec, 0x95, 0x0, 0xd1 } };


//static cfg_int cfg_registered( guid_cfg_registered, 0 );

static HRESULT gds_register()
{
	string8_fastalloc temp, temp2;
	ptr_list_t<wchar_t> extensions;

	service_enum_t<input_file_type> e;
	service_ptr_t<input_file_type> l;

	if ( e.first( l ) ) do
	{
		for ( unsigned i = 0, n = l->get_count(); i < n; ++i )
		{
			if ( l->get_mask( i, temp ) )
			{
				if ( ! strcmp_partial( temp, "*." ) &&
					 ! strchr( temp.get_ptr() + 2, '*' ) &&
					 ! strchr( temp.get_ptr() + 2, '?' ) )
				{
					uStringLower( temp2, temp.get_ptr() + 2 );
					string_utf16_from_utf8 ext16( temp2 );
					bool found = false;
					for ( unsigned i = 0, n = extensions.get_count(); i < n; ++i )
					{
						if ( ! wcscmp( extensions[ i ], ext16 ) )
						{
							found = true;
							break;
						}
					}
					if ( ! found )
					{
						wchar_t * ext = new wchar_t[ ext16.length() + 1 ];
						wcscpy( ext, ext16 );
						extensions.add_item( ext );
					}
				}
			}
		}
	}
	while ( e.next( l ) );

	int ext_count = extensions.get_count();
	const wchar_t ** ext_list = 0;

	if ( ext_count )
	{
		ext_list = new const wchar_t*[ ext_count ];
		for ( int i = 0; i < ext_count; ++i )
		{
			ext_list[ i ] = extensions[ i ];
		}
	}

	uGetModuleFileName( NULL, temp );
	temp += ",198";

	HRESULT hr = RegisterComponentHelper(
		CLSID_foobarCrawlProxy,
		L"foobar2000 GDS interface",
		L"Exposes new files recorded by foobar2000 to Google Desktop Search,"
		L" and registers foobar2000 to catch GDS events for supported file types.",
		string_utf16_from_utf8( temp ),
		ext_count,
		ext_list);

	if ( ext_list ) delete [] ext_list;
	extensions.delete_all();

	return hr;
}

static HRESULT gds_unregister()
{
	return UnregisterComponentHelper( CLSID_foobarCrawlProxy );
}

BOOL uFormatMessage(DWORD dw_error, string_base & out, HANDLE hModule)
{
	BOOL rv = FALSE;
	PVOID buffer;
	if (IsUnicode())
	{
		if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | (hModule ? FORMAT_MESSAGE_FROM_HMODULE : FORMAT_MESSAGE_FROM_SYSTEM), hModule, dw_error, 0, (LPWSTR)&buffer, 256, 0))
		{
			out.set_string_utf16((const WCHAR *)buffer);
			LocalFree(buffer);
		}
	}
	else
	{
		if (FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | (hModule ? FORMAT_MESSAGE_FROM_HMODULE : FORMAT_MESSAGE_FROM_SYSTEM), hModule, dw_error, 0, (LPSTR)&buffer, 256, 0))
		{
			out.set_string_ansi((const char *)buffer);
			LocalFree(buffer);
		}
	}
	return rv;
}

class menu_item_gds : public menu_item_impl_simple
{
public:
	virtual void item_execute(unsigned p_index,const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller)
	{
		if ( ! p_index )
		{
			HRESULT hr = gds_register();
			if ( FAILED( hr ) )
			{
				string8 error;
				uFormatMessage( hr, error, 0 );
				console::info( uStringPrintf( "Registration failure: %s", error.get_ptr() ) );
			}
		}
		else
		{
			HRESULT hr = gds_unregister();
			if ( FAILED( hr ) )
			{
				string8 error;
				uFormatMessage( hr, error, 0 );
				console::info( uStringPrintf( "Unregistration failure: %s", error.get_ptr() ) );
			}
		}
	}

	virtual bool item_get_display_data(string_base & p_out,unsigned & p_displayflags,unsigned p_index,const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller)
	{
		if ( p_index == 0 ) p_out = "Register";
		else p_out = "Unregister";
		return true;
	}

	virtual type get_type()
	{
		return TYPE_MAIN;
	}

	virtual unsigned get_num_items()
	{
		return 2;
	}

	virtual t_enabled_state get_enabled_state(unsigned p_index)
	{
		return DEFAULT_ON;
	}

	virtual bool get_item_description(unsigned p_index,string_base & out)
	{
		return false;
	}

	virtual void get_item_name(unsigned p_index,string_base & p_out)
	{
		p_out = p_index ? "Unregister" : "Register";
	}

	virtual void get_item_default_path(unsigned p_index,string_base & p_out)
	{
		p_out = "Components/Google Desktop Search";
	}

	virtual GUID get_item_guid(unsigned p_index)
	{
		// {49B766D0-6A6B-4cb5-B8C4-22B606ACCD9E}
		static const GUID guid1 = 
		{ 0x49b766d0, 0x6a6b, 0x4cb5, { 0xb8, 0xc4, 0x22, 0xb6, 0x6, 0xac, 0xcd, 0x9e } };
		// {9B31E3D8-180A-428e-AD1D-686FDFC95423}
		static const GUID guid2 = 
		{ 0x9b31e3d8, 0x180a, 0x428e, { 0xad, 0x1d, 0x68, 0x6f, 0xdf, 0xc9, 0x54, 0x23 } };
		if ( p_index == 0 ) return guid1;
		else return guid2;
	}
};

static menu_item_factory_t<menu_item_gds> g_menu_item_factory_gds;