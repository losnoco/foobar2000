#include "stdafx.h"


namespace dialog_helper {


	BOOL CALLBACK dialog::DlgProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		dialog * p_this;
		BOOL rv;
		if (msg==WM_INITDIALOG)
		{
			p_this = reinterpret_cast<dialog*>(lp);
			p_this->wnd = wnd;
			uSetWindowLong(wnd,DWL_USER,lp);

			if (p_this->m_is_modal) p_this->m_modal_scope.initialize(wnd);
		}
		else p_this = reinterpret_cast<dialog*>(uGetWindowLong(wnd,DWL_USER));

		rv = p_this ? p_this->on_message(msg,wp,lp) : FALSE;

		if (msg==WM_DESTROY && p_this)
		{
			uSetWindowLong(wnd,DWL_USER,0);
//			p_this->wnd = 0;
		}

		return rv;
	}

	void set_item_text_multi(HWND wnd,const set_item_text_multi_param * param,unsigned count)
	{
		unsigned n;
		for(n=0;n<count;n++)
		{
			if (param[n].id)
				uSetDlgItemText(wnd,param[n].id,param[n].text);
			else
				uSetWindowText(wnd,param[n].text);
		}
	}

	int dialog::run_modal(unsigned id,HWND parent)
	{
		assert(wnd == 0);
		if (wnd != 0) return -1;
		m_is_modal = true; 
		return uDialogBox(id,parent,DlgProc,reinterpret_cast<long>(this));
	}
	HWND dialog::run_modeless(unsigned id,HWND parent)
	{
		assert(wnd == 0);
		if (wnd != 0) return 0;
		m_is_modal = false; 
		return uCreateDialog(id,parent,DlgProc,reinterpret_cast<long>(this));
	}

	void dialog::end_dialog(int code)
	{
		assert(m_is_modal); 
		if (m_is_modal) uEndDialog(wnd,code);
	}










	int dialog_modal::run(unsigned p_id,HWND p_parent,HINSTANCE p_instance)
	{
		int status;

		// note: uDialogBox() has its own modal scope, we don't want that to trigger
		// if this is ever changed, move deinit to WM_DESTROY handler in DlgProc

		status = DialogBoxParam(p_instance,MAKEINTRESOURCE(p_id),p_parent,DlgProc,reinterpret_cast<LPARAM>(this));

		m_modal_scope.deinitialize();

		return status;
	}
		
	void dialog_modal::end_dialog(int p_code)
	{
		EndDialog(m_wnd,p_code);
	}

		
	BOOL CALLBACK dialog_modal::DlgProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		dialog_modal * _this;
		if (msg==WM_INITDIALOG)
		{
			_this = reinterpret_cast<dialog_modal*>(lp);
			_this->m_wnd = wnd;
			uSetWindowLong(wnd,DWL_USER,lp);

			_this->m_modal_scope.initialize(wnd);
		}
		else _this = reinterpret_cast<dialog_modal*>(uGetWindowLong(wnd,DWL_USER));

		assert(_this == 0 || _this->m_wnd == wnd);

		return _this ? _this->on_message(msg,wp,lp) : FALSE;
	}


	bool dialog_modeless::create(unsigned p_id,HWND p_parent,HINSTANCE p_instance)
	{
		assert(!m_is_in_create);
		if (m_is_in_create) return false;
		vartoggle_t<bool> scope(m_is_in_create,true);
		if (CreateDialogParam(p_instance,MAKEINTRESOURCE(p_id),p_parent,DlgProc,reinterpret_cast<long>(this)) == 0) return false;
		return m_wnd != 0;
	}

	dialog_modeless::~dialog_modeless()
	{
		assert(!m_is_in_create);
		switch(m_destructor_status)
		{
		case destructor_none:
			m_destructor_status = destructor_normal;
			if (m_wnd != 0)
			{
				DestroyWindow(m_wnd);
				m_wnd = 0;
			}
			break;
		case destructor_fromwindow:
			//do nothing
			break;
		default:
			//should never trigger
			pfc::crash();
			break;
		}
	}

	void dialog_modeless::on_window_destruction()
	{
		if (m_is_in_create)
		{
			m_wnd = 0;
		}
		else
		switch(m_destructor_status)
		{
		case destructor_none:
			m_destructor_status = destructor_fromwindow;
			delete this;
			break;
		case destructor_fromwindow:
			pfc::crash();
			break;
		default:
			break;
		}
	}

	BOOL dialog_modeless::on_message_wrap(UINT msg,WPARAM wp,LPARAM lp)
	{
		if (m_destructor_status == destructor_none)
			return on_message(msg,wp,lp);
		else
			return FALSE;
	}

	BOOL CALLBACK dialog_modeless::DlgProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		dialog_modeless * thisptr;
		BOOL rv;
		if (msg == WM_INITDIALOG)
		{
			thisptr = reinterpret_cast<dialog_modeless*>(lp);
			thisptr->m_wnd = wnd;
			uSetWindowLong(wnd,DWL_USER,lp);
			modeless_dialog_manager::add(wnd);
		}
		else thisptr = reinterpret_cast<dialog_modeless*>(uGetWindowLong(wnd,DWL_USER));

		rv = thisptr ? thisptr->on_message_wrap(msg,wp,lp) : FALSE;

		if (msg == WM_DESTROY)
			modeless_dialog_manager::remove(wnd);

		if (msg == WM_DESTROY && thisptr != 0)
			thisptr->on_window_destruction();

		return rv;
	}

}