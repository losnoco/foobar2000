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

#include "../helpers/helpers.h"

#include "resource.h"

static void RunHDCDResultsPopup( const metadb_handle_list & p_data, HWND p_parent );

bool check_hdcd( metadb_handle_ptr p_handle, abort_callback & p_abort )
{
	input_helper               m_decoder;
	service_ptr_t<file>        m_file;
	audio_chunk_impl_temporary m_chunk;
	file_info_impl             m_info;
	double                     m_duration = 0.;
	double                     m_temp;
	const char *               m_tag;

	m_decoder.open( m_file, p_handle, input_flag_simpledecode, p_abort, false, true );

	while ( m_duration < 5. )
	{
		p_abort.check();

		m_decoder.run( m_chunk, p_abort );

		if ( m_decoder.get_dynamic_info( m_info, m_temp ) )
		{
			m_tag = m_info.info_get( "hdcd" );
			if ( m_tag && !pfc::stricmp_ascii( m_tag, "yes" ) ) return true;
		}

		m_duration += m_chunk.get_duration();
	}

	return false;
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
	metadb_handle_list output_list;

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

			if ( check_hdcd( m_current_file, *m_abort ) )
			{
				insync( lock_output_list );
				output_list.add_item( m_current_file );
			}

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

		scanner_process();

		threads_stop();
	}

	virtual void on_done( HWND p_wnd, bool p_was_aborted )
	{
		threads_stop();

		if ( !p_was_aborted )
		{
			ShowWindow( p_wnd, SW_HIDE );

			RunHDCDResultsPopup( output_list, p_wnd );
		}
	}
};

class CMyResultsPopup : public CDialogImpl<CMyResultsPopup>
{
public:
	CMyResultsPopup( const metadb_handle_list & initData ) : m_initData( initData ) { }

	enum { IDD = IDD_RESULTS };

	BEGIN_MSG_MAP( CMyDSPPopup )
		MSG_WM_INITDIALOG( OnInitDialog )
		COMMAND_HANDLER_EX( IDCANCEL, BN_CLICKED, OnButton )
		MSG_WM_NOTIFY( OnNotify )
	END_MSG_MAP()

private:
	BOOL OnInitDialog(CWindow, LPARAM)
	{
		m_listview = GetDlgItem( IDC_LISTVIEW );
		pfc::string8_fast temp;

		LVCOLUMN lvc = { 0 };
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvc.fmt = LVCFMT_LEFT;
		lvc.pszText = _T("Path");
		lvc.cx = m_listview.GetStringWidth( lvc.pszText ) + 200;
		lvc.iSubItem = 0;
		m_listview.InsertColumn( 0, &lvc );
		lvc.pszText = _T("Subsong");
		lvc.cx = m_listview.GetStringWidth( lvc.pszText ) + 15;
		lvc.iSubItem = 1;
		m_listview.InsertColumn( 1, &lvc );
		lvc.pszText = _T("Name");
		lvc.cx = m_listview.GetStringWidth( lvc.pszText ) + 150;
		m_listview.InsertColumn( 2, &lvc );

		for ( unsigned i = 0; i < m_initData.get_count(); i++ )
		{
			m_listview.InsertItem( i, LPSTR_TEXTCALLBACK );
			m_listview.SetItemText( i, 1, LPSTR_TEXTCALLBACK );
			m_listview.SetItemText( i, 2, LPSTR_TEXTCALLBACK );
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

					const metadb_handle_ptr p_file = m_initData.get_item( pLvdi->item.iItem );
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
					}
				}
				break;
			}
		}

		return 0;
	}

	void OnButton( UINT, int id, CWindow )
	{
		EndDialog( id );
	}

	const metadb_handle_list & m_initData;

	CListViewCtrl m_listview;
	service_ptr_t<titleformat_object> m_script;
	pfc::string8_fast m_temp;
	pfc::stringcvt::string_os_from_utf8_fast m_convert;
};

static void RunHDCDResultsPopup( const metadb_handle_list & p_data, HWND p_parent )
{
	CMyResultsPopup popup( p_data );
	popup.DoModal( p_parent );
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
		unsigned i, j;
		i = data.get_count();
		for (j = 0; j < i; j++)
		{
			file_info_impl info;
			if ( !data.get_item( j )->get_info_async( info ) )
			{
				return false;
			}

			if ( info.info_get_decoded_bps() != 16 )
			{
				return false;
			}

			const char * encoding = info.info_get( "encoding" );
			if ( !encoding || pfc::stricmp_ascii( encoding, "lossless" ) )
			{
				return false;
			}
		}
		out = "Scan for HDCD tracks";
		return true;
	}

	virtual void context_command(unsigned n,const pfc::list_base_const_t<metadb_handle_ptr> & data,const GUID& caller)
	{
		metadb_handle_list input_files = data;
		input_files.remove_duplicates();

		service_ptr_t<threaded_process_callback> p_callback = new service_impl_t< hdcd_scanner >( input_files );

		threaded_process::g_run_modeless( p_callback, threaded_process::flag_show_abort | threaded_process::flag_show_progress | threaded_process::flag_show_item | threaded_process::flag_show_delayed, core_api::get_main_window(), "HDCD Scanner" );
	}
};

static contextmenu_item_factory_t<context_hdcd> g_contextmenu_item_hdcd_factory;
