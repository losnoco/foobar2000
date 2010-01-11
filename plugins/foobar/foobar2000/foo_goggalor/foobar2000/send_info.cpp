#include "stdafx.h"
#include "send_info.h"

#include "../../goggalor_proxy/goggalor_proxy.h"

static bool compound_meta_field( const file_info * p_info, const char * field, string_base & out )
{
	out.reset();
	for ( unsigned i = 0, count = p_info->meta_get_count_by_name( field ); i < count; ++i )
	{
		if ( out.length() ) out += ", ";
		out += p_info->meta_get( field, i );
	}
	return !! out.length();
}

HRESULT send_info ( list_base_t< metadb_handle_ptr > & m_list,
				    CComPtr < IGoogleDesktopSearchEventFactory > & mcEventFactory )
{
	HRESULT hr = S_OK;

	// Used to create an event object for this file to send back to GDS.
	CComPtr<IDispatch> mcEventDisp;

	for ( unsigned i = 0, count = m_list.get_count(); i < count; ++i )
	{
		// Replace "Google.Desktop.TextFile" with the appropriate schema for your file.
		hr = mcEventFactory->CreateEvent(CComBSTR(CLSID_foobarCrawlProxy),
			CComBSTR(L"Google.Desktop.MediaFile"), &mcEventDisp);
		if (FAILED(hr))
			//return Error(L"Unable to create event", GUID_NULL, hr);
			throw error_info_t( L"Unable to create event", hr );

		CComQIPtr<IGoogleDesktopSearchEvent> mcEvent(mcEventDisp);

		ATLASSERT(mcEventDisp && mcEvent);
		if (mcEvent == NULL)
			//return Error(L"Event does not implement IGoogleDesktopSearchEvent", GUID_NULL, E_UNEXPECTED);
			throw error_info_t( L"Event does not implement IGoogleDesktopSearchEvent", E_UNEXPECTED );

		metadb_handle_ptr & handle = m_list[ i ];

		ULONGLONG size;
		double var_time;

		string8_fastalloc temp;

		{
			abort_callback_impl m_abort;
			service_ptr_t<file> m_file;
			t_filestats m_stats;
			t_io_result status = filesystem::g_open( m_file, handle->get_path(), filesystem::open_mode_read, m_abort );
			if ( io_result_succeeded( status ) )
				status = m_file->get_stats( m_stats, m_abort );
			if ( io_result_failed( status ) )
				//return Error(L"Error querying stats", GUID_NULL, E_UNEXPECTED);
				throw error_info_t( L"Error querying stats", E_UNEXPECTED );

			size = m_stats.m_size;

			SYSTEMTIME st;

			FileTimeToSystemTime( (const FILETIME *) & m_stats.m_timestamp, & st );
			SystemTimeToVariantTime( & st, & var_time );
		}

		hr = mcEvent->AddProperty( CComBSTR(L"uri"), CComVariant( string_utf16_from_utf8( handle->get_path() ).get_ptr() ) );
		if ( SUCCEEDED( hr ) )
			hr = mcEvent->AddProperty( CComBSTR(L"last_modified_time"), CComVariant( var_time, VT_DATE ) );
		if ( SUCCEEDED( hr ) )
			hr = mcEvent->AddProperty( CComBSTR(L"format"), CComVariant( L"text/plain" ) );
		if ( SUCCEEDED( hr ) )
			hr = mcEvent->AddProperty( CComBSTR(L"native_size"), CComVariant( size ) );
		if ( SUCCEEDED( hr ) )
		{
			handle->metadb_lock();

			const file_info * m_info;

			if ( handle->get_info_locked( m_info ) )
			{
				if ( compound_meta_field( m_info, "title", temp ) )
					hr = mcEvent->AddProperty( CComBSTR( L"title" ), CComVariant( string_utf16_from_utf8( temp ).get_ptr() ) );

				if ( SUCCEEDED( hr ) )
				{
					if ( compound_meta_field( m_info, "artist", temp ) )
						hr = mcEvent->AddProperty( CComBSTR( L"artist" ), CComVariant( string_utf16_from_utf8( temp ).get_ptr() ) );
				}

				if ( SUCCEEDED( hr ) )
				{
					if ( compound_meta_field( m_info, "genre", temp ) )
						hr = mcEvent->AddProperty( CComBSTR( L"genre" ), CComVariant( string_utf16_from_utf8( temp ).get_ptr() ) );
				}

				if ( SUCCEEDED( hr ) )
				{
					if ( compound_meta_field( m_info, "comment", temp ) )
						hr = mcEvent->AddProperty( CComBSTR( L"comment" ), CComVariant( string_utf16_from_utf8( temp ).get_ptr() ) );
				}

				if ( SUCCEEDED( hr ) )
				{
					const char * tn = m_info->meta_get( "tracknumber", 0 );
					if ( tn )
					{
						while ( * tn && ! isdigit( * tn ) ) ++tn;
						if ( * tn )
						{
							char * eptr;
							unsigned tracknumber = strtoul( tn, &eptr, 10 );
							if ( eptr != tn )
							{
								hr = mcEvent->AddProperty( CComBSTR(L"track_number"), CComVariant( tracknumber ) );
							}
						}
					}
				}

				if ( SUCCEEDED( hr ) )
				{
					t_int64 bitrate64 = m_info->info_get_bitrate();
					if ( bitrate64 > 0 && bitrate64 <= UINT_MAX )
					{
						unsigned bitrate = unsigned(bitrate64);
						hr = mcEvent->AddProperty( CComBSTR(L"bit_rate"), CComVariant( bitrate ) );
					}
				}

				if ( SUCCEEDED( hr ) )
				{
					t_int64 channels64 = m_info->info_get_int( "channels" );
					if ( channels64 > 0 && channels64 <= UINT_MAX ) // har har
					{
						unsigned channels = unsigned(channels64);
						hr = mcEvent->AddProperty( CComBSTR(L"channels"), CComVariant( channels ) );
					}
				}

				if ( SUCCEEDED( hr ) )
				{
					double dlength = m_info->get_length();
					if ( dlength )
					{
						ULONGLONG length = ULONGLONG(dlength * 1000000000.);
						hr = mcEvent->AddProperty( CComBSTR(L"length"), CComVariant( length ) );
					}
				}

				if ( SUCCEEDED( hr ) )
				{
					string8_fastalloc temp2;

					for ( unsigned n = 0, m = m_info->meta_get_count(); n < m; ++n )
					{
						const char * name = m_info->meta_enum_name( n );
						if ( stricmp_utf8( name, "title" ) &&
							stricmp_utf8( name, "artist" ) &&
							stricmp_utf8( name, "genre" ) &&
							stricmp_utf8( name, "comment" ) )
						{
							if ( compound_meta_field( m_info, name, temp ) )
							{
								if ( temp2.length() ) temp2 += ", ";
								temp2 += temp;
							}
						}
					}

					for ( unsigned n = 0, m = m_info->info_get_count(); n < m; ++n )
					{
						if ( temp2.length() ) temp2 += ", ";
						temp2 += m_info->info_enum_value( n );
					}

					if ( temp2.length() )
					{
						hr = mcEvent->AddProperty( CComBSTR(L"content"), CComVariant( string_utf16_from_utf8( temp2 ).get_ptr() ) );
					}
				}
			}

			handle->metadb_unlock();
		}

		if ( SUCCEEDED( hr ) )
			hr = mcEvent->Send( EventFlagIndexable );

		if ( FAILED( hr ) ) break;
	}

	return hr;
}