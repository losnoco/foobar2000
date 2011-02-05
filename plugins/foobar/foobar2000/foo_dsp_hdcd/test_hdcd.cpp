/*
   Copyright (C) 2010, Chris Moeller,
   All rights reserved.                          

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote 
        products derived from this software without specific prior written 
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define _WIN32_WINNT 0x0501

#include <foobar2000.h>
#include "../ATLHelpers/ATLHelpers.h"

#include <atlframe.h>

#include "../helpers/helpers.h"

#include "resource.h"

struct hdcd_results
{
	metadb_handle_list m_handles;
	
	struct hdcd_data
	{
		enum status
		{
			disabled = 0,
			intermittent,
			enabled
		};

		status peak_extension, transient_filter;

		float min_gain, max_gain;

		hdcd_data()
		{
			peak_extension = disabled;
			transient_filter = disabled;
			min_gain = 0.0f;
			max_gain = 0.0f;
		}

		hdcd_data( const hdcd_data & in )
		{
			peak_extension = in.peak_extension;
			transient_filter = in.transient_filter;
			min_gain = in.min_gain;
			max_gain = in.max_gain;
		}
	};

	pfc::array_t<hdcd_data> m_data;

	hdcd_results() { }

	hdcd_results( const hdcd_results & in )
	{
		m_handles = in.m_handles;
		m_data = in.m_data;
	}
};

static void RunHDCDResultsPopup( const hdcd_results & p_data, HWND p_parent );

bool check_hdcd( metadb_handle_ptr p_handle, abort_callback & p_abort, hdcd_results::hdcd_data & p_out )
{
	input_helper               m_decoder;
	service_ptr_t<file>        m_file;
	audio_chunk_impl_temporary m_chunk;
	file_info_impl             m_info;
	double                     m_duration = 0.;
	double                     m_temp;
	const char *               m_tag;

	bool                       m_hdcd = false;
	unsigned                   m_chunk_count = 0;
	unsigned                   m_peak_extension_count = 0;
	unsigned                   m_transient_filter_count = 0;
	float                      m_minimum_gain = 0.0f;
	float                      m_maximum_gain = -8.0f;
	float                      m_gain;

	m_decoder.open( m_file, p_handle, input_flag_simpledecode, p_abort, false, true );

	while ( m_decoder.run( m_chunk, p_abort ) )
	{
		p_abort.check();

		if ( m_decoder.get_dynamic_info( m_info, m_temp ) )
		{
			m_tag = m_info.info_get( "hdcd" );
			if ( m_tag && !pfc::stricmp_ascii( m_tag, "yes" ) )
			{
				m_chunk_count++;

				m_hdcd = true;
				m_tag = m_info.info_get( "hdcd_peak_extend" );
				if ( m_tag && !pfc::stricmp_ascii( m_tag, "yes" ) ) m_peak_extension_count++;
				m_tag = m_info.info_get( "hdcd_transient_filter" );
				if ( m_tag && !pfc::stricmp_ascii( m_tag, "yes" ) ) m_transient_filter_count++;
				m_tag = m_info.info_get( "hdcd_gain" );
				if ( m_tag )
				{
					m_gain = atof( m_tag );
					if ( m_minimum_gain > m_gain ) m_minimum_gain = m_gain;
					if ( m_maximum_gain < m_gain ) m_maximum_gain = m_gain;
				}
			}
		}

		m_duration += m_chunk.get_duration();

		if ( !m_hdcd && m_duration >= 5.0 ) break;
	}

	if ( m_hdcd )
	{
		p_out.peak_extension = ( !m_peak_extension_count ) ? hdcd_results::hdcd_data::disabled : ( m_peak_extension_count < m_chunk_count ) ? hdcd_results::hdcd_data::intermittent : hdcd_results::hdcd_data::enabled;
		p_out.transient_filter = ( !m_transient_filter_count ) ? hdcd_results::hdcd_data::disabled : ( m_transient_filter_count < m_chunk_count ) ? hdcd_results::hdcd_data::intermittent : hdcd_results::hdcd_data::enabled;
		p_out.min_gain = m_minimum_gain;
		p_out.max_gain = m_maximum_gain;
	}

	return m_hdcd;
}

class hdcd_scanner : public threaded_process_callback
{
	critical_section lock_status;
	threaded_process_status * status_callback;

	abort_callback * m_abort;

	pfc::array_t<HANDLE> m_extra_threads;

	LONG input_items_total;
	volatile LONG input_items_remaining;

	critical_section lock_input_list;
	metadb_handle_list input_list;

	critical_section lock_output_list;
	hdcd_results output_list;

	void scanner_process()
	{
		for (;;)
		{
			metadb_handle_ptr m_current_file;

			m_abort->check();

			{
				insync( lock_input_list );

				if ( ! input_list.get_count() ) break;

				m_current_file = input_list.get_item( 0 );
				input_list.remove_by_idx( 0 );
			}

			try
			{
				hdcd_results::hdcd_data m_data;
				if ( check_hdcd( m_current_file, *m_abort, m_data ) )
				{
					insync( lock_output_list );
					output_list.m_handles.add_item( m_current_file );
					output_list.m_data.append_single( m_data );
				}
			}
			catch (exception_io &) { }

			InterlockedDecrement( &input_items_remaining );

			update_status();
		}
	}

	void update_status()
	{
		pfc::string8 paths, temp;

		{
			insync( lock_input_list );

			for ( unsigned i = 0; i < input_list.get_count(); i++ )
			{
				temp = input_list.get_item( i )->get_path();
				if ( paths.length() ) paths += "; ";
				paths.add_string( temp.get_ptr() + temp.scan_filename() );
			}
		}

		{
			insync( lock_status );
			status_callback->set_item( paths );
			status_callback->set_progress( input_items_total - input_items_remaining, input_items_total );
		}
	}

	static DWORD CALLBACK g_entry(void* p_instance)
	{
		try
		{
			reinterpret_cast<hdcd_scanner*>(p_instance)->scanner_process();
		}
		catch (...) { }
		return 0;
	}

	void threads_start( unsigned count )
	{
		int priority = GetThreadPriority( GetCurrentThread() );

		for ( unsigned i = 0; i < count; i++ )
		{
			HANDLE thread = CreateThread( NULL, 0, g_entry, reinterpret_cast<void*>(this), CREATE_SUSPENDED, NULL );
			if ( thread != NULL )
			{
				SetThreadPriority( thread, priority );
				m_extra_threads.append_single( thread );
			}
		}

		for ( unsigned i = 0; i < m_extra_threads.get_count(); i++ )
		{
			ResumeThread( m_extra_threads[ i ] );
		}
	}

	void threads_stop()
	{
		for ( unsigned i = 0; i < m_extra_threads.get_count(); i++ )
		{
			HANDLE thread = m_extra_threads[ i ];
			WaitForSingleObject( thread, INFINITE );
			CloseHandle( thread );
		}

		m_extra_threads.set_count( 0 );
	}

public:
	hdcd_scanner( const metadb_handle_list & p_input )
	{
		input_items_remaining = input_items_total = p_input.get_count();
		input_list = p_input;
	}

	virtual void run(threaded_process_status & p_status,abort_callback & p_abort)
	{
		status_callback = &p_status;
		m_abort = &p_abort;

		update_status();

		unsigned thread_count = pfc::getOptimalWorkerThreadCountEx( 4 );

		if ( thread_count > 1 ) threads_start( thread_count - 1 );

		try
		{
			scanner_process();
		}
		catch (...) { }

		threads_stop();
	}

	virtual void on_done( HWND p_wnd, bool p_was_aborted )
	{
		threads_stop();

		if ( !p_was_aborted )
		{
			RunHDCDResultsPopup( output_list, core_api::get_main_window() );
		}
	}
};

class CMyResultsPopup : public CDialogImpl<CMyResultsPopup>, public CDialogResize<CMyResultsPopup>
{
public:
	CMyResultsPopup( const hdcd_results & initData ) : m_initData( initData ) { }

	enum { IDD = IDD_RESULTS };

	BEGIN_MSG_MAP( CMyDSPPopup )
		MSG_WM_INITDIALOG( OnInitDialog )
		COMMAND_HANDLER_EX( IDCANCEL, BN_CLICKED, OnButton )
		MSG_WM_NOTIFY( OnNotify )
		CHAIN_MSG_MAP(CDialogResize<CMyResultsPopup>)
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP( CMyResultsPopup )
		DLGRESIZE_CONTROL( IDC_LISTVIEW, DLSZ_SIZE_X | DLSZ_SIZE_Y )
	END_DLGRESIZE_MAP()

private:
	BOOL OnInitDialog(CWindow, LPARAM)
	{
		DlgResize_Init();

		m_listview = GetDlgItem( IDC_LISTVIEW );
		pfc::string8_fast temp;

		LVCOLUMN lvc = { 0 };
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvc.fmt = LVCFMT_LEFT;
		lvc.pszText = _T("Path");
		lvc.cx = m_listview.GetStringWidth( lvc.pszText ) + 200;
		lvc.iSubItem = 0;
		m_listview.InsertColumn( 0, &lvc );
		lvc.fmt = LVCFMT_RIGHT;
		lvc.pszText = _T("Subsong");
		lvc.cx = m_listview.GetStringWidth( lvc.pszText ) + 15;
		lvc.iSubItem = 1;
		m_listview.InsertColumn( 1, &lvc );
		lvc.fmt = LVCFMT_LEFT;
		lvc.pszText = _T("Name");
		lvc.cx = m_listview.GetStringWidth( lvc.pszText ) + 150;
		m_listview.InsertColumn( 2, &lvc );
		lvc.fmt = LVCFMT_RIGHT;
		lvc.pszText = _T("Minimum gain");
		lvc.cx = m_listview.GetStringWidth( lvc.pszText ) + 15;
		m_listview.InsertColumn( 3, &lvc );
		lvc.pszText = _T("Maximum gain");
		lvc.cx = m_listview.GetStringWidth( lvc.pszText ) + 15;
		m_listview.InsertColumn( 4, &lvc );
		lvc.fmt = LVCFMT_CENTER;
		lvc.pszText = _T("Peak extension");
		lvc.cx = m_listview.GetStringWidth( lvc.pszText ) + 15;
		m_listview.InsertColumn( 5, &lvc );
		lvc.pszText = _T("Transient filter");
		lvc.cx = m_listview.GetStringWidth( lvc.pszText ) + 15;
		m_listview.InsertColumn( 6, &lvc );

		for ( unsigned i = 0; i < m_initData.m_handles.get_count(); i++ )
		{
			m_listview.InsertItem( i, LPSTR_TEXTCALLBACK );
			m_listview.SetItemText( i, 1, LPSTR_TEXTCALLBACK );
			m_listview.SetItemText( i, 2, LPSTR_TEXTCALLBACK );
			m_listview.SetItemText( i, 3, LPSTR_TEXTCALLBACK );
			m_listview.SetItemText( i, 4, LPSTR_TEXTCALLBACK );
			m_listview.SetItemText( i, 5, LPSTR_TEXTCALLBACK );
			m_listview.SetItemText( i, 6, LPSTR_TEXTCALLBACK );
		}

		if ( !static_api_ptr_t<titleformat_compiler>()->compile( m_script, "%title%" ) )
			m_script.release();

		return TRUE;
	}

	LRESULT OnNotify( int, LPNMHDR message )
	{
		if ( message->hwndFrom == m_listview.m_hWnd )
		{
			switch ( message->code )
			{
			case LVN_GETDISPINFO:
				{
					LV_DISPINFO *pLvdi = (LV_DISPINFO *)message;

					const metadb_handle_ptr p_file = m_initData.m_handles.get_item( pLvdi->item.iItem );
					const hdcd_results::hdcd_data & p_data = m_initData.m_data[ pLvdi->item.iItem ];
					switch (pLvdi->item.iSubItem)
					{
					case 0:
						filesystem::g_get_display_path( p_file->get_path(), m_temp );
						m_convert.convert( m_temp );
						pLvdi->item.pszText = (TCHAR *) m_convert.get_ptr();
						break;

					case 1:
						m_convert.convert( pfc::format_int( p_file->get_subsong_index() ) );
						pLvdi->item.pszText = (TCHAR *) m_convert.get_ptr();
						break;

					case 2:
						if ( m_script.is_valid() ) p_file->format_title( NULL, m_temp, m_script, NULL );
						else m_temp.reset();
						m_convert.convert( m_temp );
						pLvdi->item.pszText = (TCHAR *) m_convert.get_ptr();
						break;

					case 3:
						m_temp = pfc::format_float( p_data.min_gain, 0, 1 );
						m_temp += " dB";
						m_convert.convert( m_temp );
						pLvdi->item.pszText = (TCHAR *) m_convert.get_ptr();
						break;

					case 4:
						m_temp = pfc::format_float( p_data.max_gain, 0, 1 );
						m_temp += " dB";
						m_convert.convert( m_temp );
						pLvdi->item.pszText = (TCHAR *) m_convert.get_ptr();
						break;

					case 5:
						m_temp.reset();
						switch ( p_data.peak_extension )
						{
						case hdcd_results::hdcd_data::disabled:     m_temp = "Disabled";     break;
						case hdcd_results::hdcd_data::intermittent: m_temp = "Intermittent"; break;
						case hdcd_results::hdcd_data::enabled:      m_temp = "Enabled";      break;
						}
						m_convert.convert( m_temp );
						pLvdi->item.pszText = (TCHAR *) m_convert.get_ptr();
						break;

					case 6:
						m_temp.reset();
						switch ( p_data.transient_filter )
						{
						case hdcd_results::hdcd_data::disabled:     m_temp = "Disabled";     break;
						case hdcd_results::hdcd_data::intermittent: m_temp = "Intermittent"; break;
						case hdcd_results::hdcd_data::enabled:      m_temp = "Enabled";      break;
						}
						m_convert.convert( m_temp );
						pLvdi->item.pszText = (TCHAR *) m_convert.get_ptr();
						break;
					}
				}
				break;
			}
		}

		return 0;
	}

	void OnButton( UINT, int id, CWindow )
	{
		DestroyWindow();
	}

	hdcd_results m_initData;

	CListViewCtrl m_listview;
	service_ptr_t<titleformat_object> m_script;
	pfc::string8_fast m_temp;
	pfc::stringcvt::string_os_from_utf8_fast m_convert;
};

static void RunHDCDResultsPopup( const hdcd_results & p_data, HWND p_parent )
{
	CMyResultsPopup * popup = new CWindowAutoLifetime<ImplementModelessTracking<CMyResultsPopup>>( p_parent, p_data );
}

class context_hdcd : public contextmenu_item_simple
{
public:
	virtual unsigned get_num_items() { return 1; }

	virtual void get_item_name(unsigned n, pfc::string_base & out)
	{
		if (n) uBugCheck();
		out = "Scan for HDCD tracks";
	}

	/*virtual void get_item_default_path(unsigned n, pfc::string_base & out)
	{
		out.reset();
	}*/
	GUID get_parent() {return contextmenu_groups::utilities;}

	virtual bool get_item_description(unsigned n, pfc::string_base & out)
	{
		if (n) uBugCheck();
		out = "Scans the selected tracks for HDCD encoding.";
		return true;
	}

	virtual GUID get_item_guid(unsigned p_index)
	{
		if (p_index) uBugCheck();
		static const GUID guid = { 0xfbec6ed7, 0x9d34, 0x4d1b, { 0xb9, 0xe0, 0x18, 0x5e, 0x2f, 0xfa, 0xf6, 0x1a } };
		return guid;
	}

	virtual bool context_get_display(unsigned n,const pfc::list_base_const_t<metadb_handle_ptr> & data,pfc::string_base & out,unsigned & displayflags,const GUID &)
	{
		if (n) uBugCheck();
		out = "Scan for HDCD tracks";
		return true;
	}

	virtual void context_command(unsigned n,const pfc::list_base_const_t<metadb_handle_ptr> & data,const GUID& caller)
	{
		metadb_handle_list input_files = data;
		input_files.remove_duplicates();

		unsigned i, j;
		i = input_files.get_count();
		for (j = 0; j < i; j++)
		{
			const char * encoding;
			file_info_impl info;
			if ( !input_files.get_item( j )->get_info_async( info ) ||
				info.info_get_decoded_bps() != 16 ||
				!( encoding = info.info_get( "encoding" ) ) ||
				pfc::stricmp_ascii( encoding, "lossless" ) )
			{
				input_files.remove_by_idx( j );
				j--;
				i--;
			}
		}

		if ( i )
		{
			service_ptr_t<threaded_process_callback> p_callback = new service_impl_t< hdcd_scanner >( input_files );

			threaded_process::g_run_modeless( p_callback, threaded_process::flag_show_abort | threaded_process::flag_show_progress | threaded_process::flag_show_item | threaded_process::flag_show_delayed, core_api::get_main_window(), "HDCD Scanner" );
		}
		else
		{
			uMessageBox( core_api::get_main_window(), "No valid files to scan.", "HDCD Scanner", MB_ICONEXCLAMATION );
		}
	}
};

static contextmenu_item_factory_t<context_hdcd> g_contextmenu_item_hdcd_factory;
