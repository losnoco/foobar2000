#include "stdafx.h"
#include "../helpers/helpers.h"
#include "resource.h"


class dialog_cue : public dialog_helper::dialog
{
public:

	static void g_run(const char * p_path,HWND p_parent);

private:

	dialog_cue(const char * p_path) : m_path(p_path), m_resizer(g_resizing_table,tabsize(g_resizing_table),120,50,0,0) {}

	BOOL on_message(UINT msg,WPARAM wp,LPARAM lp);
	void on_cue_loaded(const char * data);

	string8 m_path;
	dialog_resize_helper m_resizer;
	
	static const dialog_resize_helper::param g_resizing_table[5];
};

void dialog_cue::on_cue_loaded(const char * data)
{
	HWND wnd = get_wnd();
	EnableWindow(GetDlgItem(wnd,IDC_CUESHEET),TRUE);
	uSendDlgItemMessage(wnd,IDC_ENABLED,BM_SETCHECK,1,0);
	uSetDlgItemText(wnd,IDC_CUESHEET,data);
}

void dialog_cue::g_run(const char * p_path,HWND p_parent)
{
	(new dialog_cue(p_path))->run_modeless(IDD_CUE,p_parent);
}

const dialog_resize_helper::param dialog_cue::g_resizing_table[5] = {
	{IDC_CUESHEET,dialog_resize_helper::XY_SIZE},
	{IDC_ENABLED,dialog_resize_helper::Y_MOVE},
	{IDC_LOAD,dialog_resize_helper::XY_MOVE},
	{IDOK,dialog_resize_helper::XY_MOVE},
	{IDCANCEL,dialog_resize_helper::XY_MOVE},
};

BOOL dialog_cue::on_message(UINT msg,WPARAM wp,LPARAM lp)
{
	HWND wnd = get_wnd();
	if (m_resizer.process_message(wnd,msg,wp,lp)) return TRUE;
	else switch(msg)
	{
	case WM_DROPFILES:
		{
			HDROP hdrop = (HDROP) wp;
			if (uDragQueryFileCount(hdrop) == 1)
			{
				string8 path;
				if (uDragQueryFile(hdrop,0,path))
				{
					if (!stricmp_utf8(string_extension(path),"CUE"))
					{
						string8 path_c;
						filesystem::g_get_canonical_path(path,path_c);
						bool meh;
						string8 data;
						if (io_result_succeeded(text_file_loader::read(path_c,abort_callback_impl(),data,meh)))
							on_cue_loaded(data);
						else
							MessageBeep(0);
					}
					else MessageBeep(0);
				}
				else MessageBeep(0);
			}
			else MessageBeep(0);
			DragFinish(hdrop);
		}
		return TRUE;
	case WM_INITDIALOG:
		modeless_dialog_manager::add(wnd);
		{
			file_info_i info;
			if (io_result_failed(input_entry::g_get_info(make_playable_location(m_path,0),info,abort_callback_impl())))
			{
				popup_message::g_show("Error reading information from file.","Error");
				DestroyWindow(wnd);
				return TRUE;
			}
			const char * tag = info.meta_get("cuesheet",0);
			if (tag == 0)
			{
				EnableWindow(GetDlgItem(wnd,IDC_CUESHEET),FALSE);
				uSendDlgItemMessage(wnd,IDC_ENABLED,BM_SETCHECK,0,0);
			}
			else
			{
				on_cue_loaded(tag);
			}

			{
				string8 temp;
				uGetWindowText(wnd,temp);
				temp.add_string(" - ");
				temp.add_string(string_filename_ext(m_path));
				uSetWindowText(wnd,temp);
			}
		}
		return TRUE;
	case WM_COMMAND:
		switch(wp)
		{
		case IDC_LOAD:
			{
				string8 path;
				if (uGetOpenFileName(wnd,"Cuesheet files|*.cue",0,"cue","Load cuesheet",0,path,FALSE))
				{
					bool meh;
					string8 data;
					if (io_result_succeeded(text_file_loader::read(path,abort_callback_impl(),data,meh)))
						on_cue_loaded(data);
					else
						popup_message::g_show("Error reading file.","Error");
				}
			}

			return TRUE;
		case IDC_ENABLED:
			EnableWindow(GetDlgItem(wnd,IDC_CUESHEET),SendMessage((HWND)lp,BM_GETCHECK,0,0));
			return TRUE;
		case IDOK:
			{
				if (static_api_ptr_t<metadb_io>()->is_file_updating_blocked())
				{
					popup_message::g_show("Tag update blocking option is enabled, can't write to file.","Information");
					return TRUE;
				}

				static_api_ptr_t<metadb_io>()->pending_flush(false,wnd,false);

				file_info_i info;
				if (io_result_failed(input_entry::g_get_info(make_playable_location(m_path,0),info,abort_callback_impl())))
				{
					uMessageBox(wnd,"Error reading information from file.",0,0);
					return TRUE;
				}
				if (uSendDlgItemMessage(wnd,IDC_ENABLED,BM_GETCHECK,0,0))
				{
					string8 tag;
					uGetDlgItemText(wnd,IDC_CUESHEET,tag);
					info.meta_set("cuesheet",tag);
				}
				else
				{
					info.meta_remove_field("cuesheet");
				}
				if (io_result_failed(input_entry::g_set_info(make_playable_location(m_path,0),info,abort_callback_impl())))
				{
					popup_message::g_show("Error writing information to file.","Error");
					return TRUE;
				}
			}
			{
				metadb_handle_list data;
				abort_callback_impl l_abort;
				if (io_result_succeeded(track_indexer::g_get_tracks_simple(m_path,0,filestats_invalid,data,l_abort)))
				{
					static_api_ptr_t<metadb_io>()->load_info_multi(data,metadb_io::load_info_force,wnd,false,0);
				}
			}
			DestroyWindow(wnd);
			return TRUE;
		case IDCANCEL:
			DestroyWindow(wnd);
			return FALSE;
		default:
			return FALSE;
		}
	case WM_CLOSE:
		DestroyWindow(wnd);
		modeless_dialog_manager::remove(wnd);
		return TRUE;
	case WM_DESTROY:
		delete this;
		return TRUE;
	default:
		return FALSE;
	}
}

class menu_item_cue : public menu_item_legacy_context
{
	static bool g_test_item(const metadb_handle_ptr & p_item)
	{
		return !filesystem::g_dont_read_infos(p_item->get_path()) && p_item->query_info_field_test("cue_embedded");
	}
public:

	GUID get_item_guid(unsigned)
	{
		// {B7651E1A-5454-479a-9FB1-6E075F086783}
		static const GUID guid = 
		{ 0xb7651e1a, 0x5454, 0x479a, { 0x9f, 0xb1, 0x6e, 0x7, 0x5f, 0x8, 0x67, 0x83 } };
		return guid;
	}

	unsigned get_num_items() {return 1;}
	void get_item_name(unsigned n,string_base & out)
	{
		out = "Edit cuesheet";
	}

	void get_item_default_path(unsigned, string_base & out) {out = "Utils";}

	bool get_item_description(unsigned,string_base & out) {out = "Edits embedded cuesheet on specified file (advanced)."; return true;}


	void context_command(unsigned n,const list_base_const_t<metadb_handle_ptr> & data,const GUID&)
	{
		if (n == 0 && data.get_count() == 1 && g_test_item(data.get_item(0)))
		{
			dialog_cue::g_run(data.get_item(0)->get_path(),core_api::get_main_window());
		}
	}

	virtual bool context_get_display(unsigned n,const list_base_const_t<metadb_handle_ptr> & data,string_base & out,unsigned&,const GUID&)
	{
		if (n == 0 && data.get_count() == 1 && g_test_item(data.get_item(0)))
		{
			out = "Edit cuesheet";
			return true;
		}
		else return false;
	}
};

static menu_item_factory_t<menu_item_cue> g_menu_item_cue_factory;
