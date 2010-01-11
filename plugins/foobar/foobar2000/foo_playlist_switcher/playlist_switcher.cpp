#include "../SDK/foobar2000.h"
#include "../helpers/helpers.h"

#include "resource.h"

//code originally ripped from main exe, hence the namespace and other stuff

static void g_build_playlist_filter(string_base & filter)	//reused in a bunch of places
{
	service_enum_t<playlist_loader> e;
	playlist_loader * l;
	filter = "Playlist files|";
	int first=1;
	for(l=e.first();l;l = e.next())
	{
		if (first) first=0;
		else filter += ";";
		filter += "*.";
		filter += l->get_extension();
		l->service_release();
	}
}


namespace {

	static BOOL CALLBACK RenameProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		switch(msg)
		{
		case WM_INITDIALOG:
			uSetWindowLong(wnd,DWL_USER,lp);
			{
				string_base * ptr = reinterpret_cast<string_base*>(lp);
				uSetWindowText(wnd,uStringPrintf("Rename playlist: \"%s\"",ptr->get_ptr()));
				uSetDlgItemText(wnd,IDC_EDIT,ptr->get_ptr());
			}
			return 1;
		case WM_COMMAND:
			switch(wp)
			{
			case IDOK:
				uGetDlgItemText(wnd,IDC_EDIT,*reinterpret_cast<string_base*>(uGetWindowLong(wnd,DWL_USER)));
				EndDialog(wnd,1);
				break;
			case IDCANCEL:
				EndDialog(wnd,0);
				break;
			}
			break;
		case WM_CLOSE:
			EndDialog(wnd,0);
			break;
		}
		return 0;
	}

	static bool g_rename(string_base & param,HWND parent)
	{
		return !!uDialogBox(IDD_PLAYLIST_SWITCHER_RENAME,parent,RenameProc,reinterpret_cast<long>(&param));
	}

	static playlist_switcher * g_switcher;

	static HWND g_switcher_wnd,g_list;
	static const char g_activestring[] = " (active)";

	static void g_reinit_list()
	{
		uSendMessage(g_list,LB_RESETCONTENT,0,0);

	
		unsigned max = g_switcher->get_num_playlists();
		unsigned n;
		string8_fastalloc name;
		unsigned active = g_switcher->get_active_playlist();
		for(n=0;n<max;n++)
		{
			name.reset();
			if (!g_switcher->get_playlist_name(n,name)) name = "[error]";
			if (active==n) name += g_activestring;
			uSendMessageText(g_list,LB_ADDSTRING,0,name);
		}
	}

	static cfg_struct_t<WINDOWPLACEMENT> cfg_window_placement("playlist_switcher_window_placement",0);

	static BOOL CALLBACK SwitcherProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		static const dialog_resize_helper::param g_resize_helper_table[] = 
		{
			{IDC_LIST,dialog_resize_helper::X_SIZE|dialog_resize_helper::Y_SIZE},
		};
		static dialog_resize_helper g_resize_helper(g_resize_helper_table,tabsize(g_resize_helper_table),20,20,0,0);

		assert(g_switcher);
		if (!g_resize_helper.process_message(wnd,msg,wp,lp))
		switch(msg)
		{
		case WM_INITDIALOG:
			modeless_dialog_manager::add(wnd);
			g_switcher_wnd = wnd;
			g_list = GetDlgItem(wnd,IDC_LIST);


			g_reinit_list();

			apply_window_placement(cfg_window_placement,wnd);

			return 1;
		case WM_CLOSE:
			DestroyWindow(wnd);
			break;
		case WM_CONTEXTMENU:
			{
				POINT pt = {(short)LOWORD(lp),(short)HIWORD(lp)};
				RECT rc_list;
				GetWindowRect(g_list,&rc_list);
				if (PtInRect(&rc_list,pt))
				{
					{
						POINT temp = pt;
						if (ScreenToClient(g_list,&temp))
						{
							int idx = uSendMessage(g_list,LB_ITEMFROMPOINT,0,MAKELONG(temp.x,temp.y));
							if (idx>=0 && idx<uSendMessage(g_list,LB_GETCOUNT,0,0)) uSendMessage(g_list,LB_SETCURSEL,idx,0);
						}

					}
					unsigned idx =  uSendMessage(g_list,LB_GETCURSEL,0,0);
					unsigned num = g_switcher->get_num_playlists(), active = g_switcher->get_active_playlist();
					bool idx_valid = (signed)idx>=0 && idx<num;
				
					enum {ID_SWITCH = 1, ID_REMOVE, ID_RENAME, ID_NEW, ID_SAVE, ID_LOAD, ID_UP, ID_DOWN};

					HMENU menu = CreatePopupMenu();

					enum {FLAGS_ENABLED = MF_STRING, FLAGS_DISABLED = MF_STRING|MF_DISABLED|MF_GRAYED};

					uAppendMenu(menu,((idx_valid && active!=idx) ? FLAGS_ENABLED : FLAGS_DISABLED),ID_SWITCH,"&Switch to this playlist");
					uAppendMenu(menu,((idx_valid && num>1 && active!=idx) ? FLAGS_ENABLED : FLAGS_DISABLED),ID_REMOVE,"&Remove this playlist");
					uAppendMenu(menu,idx_valid ? FLAGS_ENABLED : FLAGS_DISABLED,ID_RENAME,"Re&name this playlist...");
					uAppendMenu(menu,FLAGS_ENABLED,ID_NEW,"&Create new playlist");
					uAppendMenu(menu,MF_SEPARATOR,0,0);
					uAppendMenu(menu,idx_valid ? FLAGS_ENABLED : FLAGS_DISABLED,ID_SAVE,"&Save playlist...");
					uAppendMenu(menu,FLAGS_ENABLED,ID_LOAD,"&Load playlist...");
					uAppendMenu(menu,MF_SEPARATOR,0,0);
					uAppendMenu(menu,idx>0 ? FLAGS_ENABLED : FLAGS_DISABLED,ID_UP,"Move up");
					uAppendMenu(menu,idx+1<num ? FLAGS_ENABLED : FLAGS_DISABLED,ID_DOWN,"Move down");


					
					int cmd = TrackPopupMenu(menu,TPM_RIGHTBUTTON|TPM_NONOTIFY|TPM_RETURNCMD,pt.x,pt.y,0,wnd,0);
					DestroyMenu(menu);
					if (cmd)
					{
						idx =  uSendMessage(g_list,LB_GETCURSEL,0,0);
						num = g_switcher->get_num_playlists(); active = g_switcher->get_active_playlist();
						idx_valid = (signed)idx>=0 && idx<num;

						switch(cmd)
						{
						case ID_SWITCH:
							if (idx_valid) g_switcher->set_active_playlist(idx);							
							break;
						case ID_REMOVE:
							if (idx_valid) g_switcher->delete_playlist(idx);								
							break;
						case ID_RENAME:
							if (idx_valid)
							{
								string8 temp;
								if (g_switcher->get_playlist_name(idx,temp))
								{
									if (g_rename(temp,wnd))
									{//fucko: dialogobx has a messgeloop, someone might have called switcher api funcs in the meanwhile
										idx = uSendMessage(g_list,LB_GETCURSEL,0,0);
										num = g_switcher->get_num_playlists();
										if ((signed)idx>=0 && idx<num)
										{
											g_switcher->rename_playlist_fixname(idx,temp);
										}
									}
								}
							}							
							break;
						case ID_NEW:
							{
								metadb_handle_list data;
								g_switcher->create_playlist_fixname(string8("New playlist"),data);
							}
							break;
						case ID_SAVE:
							if (idx_valid)
							{
								metadb_handle_list temp;
								if (g_switcher->get_playlist_data(idx,temp))
								{
									menu_manager::run_command_context("Save as playlist...",temp);
								}
								temp.delete_all();
							}
							break;
						case ID_LOAD:
							{
								string8 filter;
								g_build_playlist_filter(filter);

								string8 dir;
								config_var_string::g_get_value("Last directory",dir);
								dir.fix_dir_separator('\\');
							
								uGetOpenFileNameMultiResult * result;
								result = uGetOpenFileNameMulti(wnd,filter,0,"fpl","Load playlist...",dir);
								if (result)
								{
									unsigned n,m=result->GetCount();
									for(n=0;n<m;n++)
									{
										const char * fn = result->GetFileName(n);
										if (n==0)
										{
											string8 temp(fn);
											temp.truncate(temp.scan_filename());
											config_var_string::g_set_value("Last directory",temp);
										}
										playlist_loader_callback_i playlist_data;
										if (playlist_loader::load_playlist(fn,&playlist_data))
										{
											string8 tempname = string_filename(fn);
											g_switcher->create_playlist_fixname(tempname,playlist_data.get_data());
										}	
									}
									delete result;						
								}
							}
							break;
						case ID_UP:
							if (idx>0)
							{
								order_helper order(num);
								order.swap(idx,idx-1);
								g_switcher->reorder(order,num);
							}
							break;
						case ID_DOWN:
							if (idx+1<num)
							{
								order_helper order(num);
								order.swap(idx,idx+1);
								g_switcher->reorder(order,num);
							}
							break;
						}
					}
				}
			}
			break;
		case WM_COMMAND:
			switch(wp)
			{
			case IDCANCEL:
				DestroyWindow(wnd);
				break;
			case IDOK:
			case IDC_LIST | (LBN_DBLCLK<<16):
				{
					DWORD pts = GetMessagePos();
					POINT pt = {(short)LOWORD(pts),(short)HIWORD(pts)};
					if (ScreenToClient(g_list,&pt))
					{
						int idx = uSendMessage(g_list,LB_ITEMFROMPOINT,0,MAKELONG(pt.x,pt.y));
						if (idx>=0 && idx<uSendMessage(g_list,LB_GETCOUNT,0,0))
						{
							g_switcher->set_active_playlist((unsigned)idx);
						}
					}
				}
				break;
			}
			break;
		case WM_DESTROY:
			read_window_placement(cfg_window_placement,wnd);
			modeless_dialog_manager::remove(wnd);
			g_switcher_wnd = 0;
			g_list = 0;
			break;
		}
		return 0;
	}

	static void g_activate()
	{
		if (g_switcher_wnd)
		{
			ShowWindow(g_switcher_wnd,SW_SHOW);
			SetForegroundWindow(g_switcher_wnd);
		}
		else
		{
			uCreateDialog(IDD_PLAYLIST_SWITCHER,core_api::get_main_window(),SwitcherProc);
		}
	}

	class playlist_switcher_callback_i : public playlist_switcher_callback
	{
		virtual void on_item_replaced(unsigned pls,unsigned item,metadb_handle * from,metadb_handle * to) {};//workaround for inactive-playlist-needs-modification-when-using-masstagger-autorename; does not work for active playlist !
		virtual void on_playlist_switch_before(unsigned from,unsigned to)
		{
		}

		virtual void on_playlist_switch_after(unsigned from,unsigned to)
		{
			if (g_switcher_wnd)
			{
				string8 temp;
				g_switcher->get_playlist_name(from,temp);
				on_rename_playlist(from,temp);
				g_switcher->get_playlist_name(to,temp);
				on_rename_playlist(to,temp);
			}
		}

		virtual void on_reorder(const int * order,unsigned count)
		{
			if (g_switcher_wnd)
			{
				int old_sel = uSendMessage(g_list,LB_GETCURSEL,0,0);
				g_reinit_list();
				if (old_sel>=0)
				{
					unsigned n;
					for(n=0;n<count;n++)
					{
						if (old_sel==order[n]) {old_sel = (int)n;break;}
					}
					uSendMessage(g_list,LB_SETCURSEL,old_sel,0);
				}
			}
		}

		virtual void on_new_playlist(const char * name,unsigned idx,const ptr_list_interface<metadb_handle> & data)
		{
			if (g_switcher_wnd)
			{
				string8 temp = name;
				if (g_switcher->get_active_playlist()==idx) temp += g_activestring;
				uSendMessageText(g_list,LB_ADDSTRING,0,temp);
			}
		}

		virtual void on_delete_playlist(unsigned idx)
		{
			if (g_switcher_wnd)
			{
				uSendMessage(g_list,LB_DELETESTRING,idx,0);
			}
		}

		virtual void on_rename_playlist(unsigned idx,const char * new_name)
		{
			if (g_switcher_wnd)
			{
				int old_sel = uSendMessage(g_list,LB_GETCURSEL,0,0);
				uSendMessage(g_list,LB_DELETESTRING,idx,0);
				string8 temp = new_name;
				if (g_switcher->get_active_playlist()==idx) temp += g_activestring;
				uSendMessageText(g_list,LB_INSERTSTRING,idx,temp);
				if (old_sel==(int)idx) uSendMessage(g_list,LB_SETCURSEL,old_sel,0);
			}
		}
	};

	static playlist_switcher_callback_factory<playlist_switcher_callback_i> asdfasdfasdf;


	class menu_item_playlist_switch : public menu_item_main
	{
		enum
		{
			SWITCHER,
			TOTAL,
		};


		virtual int get_num_items() {return TOTAL;}

		virtual bool is_checked(int n)
		{
			return false;
		}

		virtual const char * enum_item(int n)
		{
			switch(n)
			{
			case SWITCHER:	return "Components/Playlist switcher";
			default: return 0;
			}
		}

		virtual void perform_command(int action)
		{
			switch(action)
			{
			case SWITCHER:
				if (core_api::assert_main_thread() && (g_switcher || (g_switcher = playlist_switcher::get()))) g_activate();
				break;
			}
		}
	};


	static menu_item_factory<menu_item_playlist_switch> foo;

}