#include "config_interface.h"

#include "resource.h"

#include "config.h"
#include "main.h"

// {A2A5EF33-2C0E-405d-B7E5-360BB4A413C7}
static const GUID guid_cfg_enable = 
{ 0xa2a5ef33, 0x2c0e, 0x405d, { 0xb7, 0xe5, 0x36, 0xb, 0xb4, 0xa4, 0x13, 0xc7 } };
// {EC7D206E-9CD5-4632-B0D2-04150AF7AB9B}
static const GUID guid_cfg_placement = 
{ 0xec7d206e, 0x9cd5, 0x4632, { 0xb0, 0xd2, 0x4, 0x15, 0xa, 0xf7, 0xab, 0x9b } };
// {A138AA8F-826F-48df-ACFF-4A0662E259B2}
static const GUID guid_cfg_customcolors = 
{ 0xa138aa8f, 0x826f, 0x48df, { 0xac, 0xff, 0x4a, 0x6, 0x62, 0xe2, 0x59, 0xb2 } };

cfg_int cfg_enable(guid_cfg_enable, 0);

cfg_window_placement cfg_placement(guid_cfg_placement);

inline static CUSTOMCOLORS get_def_colors()
{
	CUSTOMCOLORS foo;
	for (int i=16;i--;) foo.colors[i] = 0xFFFFFF;
	return foo;
}

cfg_struct_t<CUSTOMCOLORS> cfg_customcolors(guid_cfg_customcolors, get_def_colors());

static const char * tab_names[] = { "General", "Effects", "Format", "Volume" };

#define MAKEID(X,Y) ( ( (DWORD)(X)<<16) | (DWORD)(Y))
#define WM_CMDNOTIFY (WM_USER+6)
#define WM_HSCNOTIFY (WM_USER+7)

enum
{
	IDC_CONFIG1 = 1100,
	IDC_CONFIG2,
	IDC_CONFIG3,
	IDC_CONFIG4
};

class preferences_page_osd : public preferences_page
{
	struct config_info
	{
		unsigned   n_preset;
		osd_config preset;
		bool       initialized;
		bool       modified;

		config_info() : initialized(false), modified(false) {}
	};

	static void on_count(HWND wnd, unsigned count, bool modified)
	{
		BOOL status = (count > 0) ? TRUE : FALSE;
		HWND hTab;
		EnableWindow(GetDlgItem(wnd, IDC_OVERLAY_REMOVE), status);
		EnableWindow(GetDlgItem(wnd, IDC_OVERLAY_RENAME), status);
		EnableWindow(GetDlgItem(wnd, IDC_OVERLAY_APPLY), status && modified);
		EnableWindow(GetDlgItem(wnd, IDC_OVERLAY_TEST), status);
		EnableWindow(hTab = GetDlgItem(wnd, IDC_TAB), status);
		EnableWindow(GetDlgItem(hTab, IDC_CONFIG1), status);
		EnableWindow(GetDlgItem(hTab, IDC_CONFIG2), status);
		EnableWindow(GetDlgItem(hTab, IDC_CONFIG3), status);
		EnableWindow(GetDlgItem(hTab, IDC_CONFIG4), status);
	}

	static void on_modified(config_info * ctx, HWND wnd)
	{
		if (!ctx->modified)
		{
			ctx->modified = true;
			EnableWindow(GetDlgItem(wnd, IDC_OVERLAY_APPLY), TRUE);
		}
	}

	static void load(HWND wnd, const osd_config & c)
	{
		HWND hTab, w, w2;

		hTab = GetDlgItem(wnd, IDC_TAB);

		w = GetDlgItem(hTab, IDC_CONFIG1);
		uSendDlgItemMessage(w, IDC_POP, BM_SETCHECK, !!(c.flags & osd_pop), 0);
		uSendDlgItemMessage(w, IDC_POP_PLAY, BM_SETCHECK, !!(c.flags & osd_play), 0);
		uSendDlgItemMessage(w, IDC_POP_PAUSE, BM_SETCHECK, !!(c.flags & osd_pause), 0);
		uSendDlgItemMessage(w, IDC_POP_SEEK, BM_SETCHECK, !!(c.flags & osd_seek), 0);
		uSendDlgItemMessage(w, IDC_POP_SWITCH, BM_SETCHECK, !!(c.flags & osd_switch), 0);
		uSendDlgItemMessage(w, IDC_POP_DYNAMIC, BM_SETCHECK, !!(c.flags & osd_dynamic), 0);
		uSendDlgItemMessage(w, IDC_POP_DYNAMIC_ALL, BM_SETCHECK, !!(c.flags & osd_dynamic_all), 0);
		uSendDlgItemMessage(w, IDC_POP_VOLUME, BM_SETCHECK, !!(c.flags & osd_volume), 0);
		uSendDlgItemMessage(w, IDC_POP_STOP, BM_SETCHECK, !!(c.flags & osd_hide_on_stop), 0);

		uSendDlgItemMessage(w, IDC_PERMANENT, BM_SETCHECK, !!(c.flags & osd_permanent), 0);
		uSendDlgItemMessage(w, IDC_INTERVAL, BM_SETCHECK, !!(c.flags & osd_interval), 0);
		uSendDlgItemMessage(w, IDC_OUTLINE, BM_SETCHECK, !!(c.flags & osd_outline), 0);
// FUCKO
#ifdef GDIPLUS
		uSendDlgItemMessage(w, IDC_ANTIALIAS, BM_SETCHECK, !!(c.flags & osd_antialias), 0);
#else
		EnableWindow(GetDlgItem(w, IDC_ANTIALIAS), FALSE);
#endif

		w2 = GetDlgItem(w, IDC_TIMESPIN);
		uSendMessage(w2, UDM_SETRANGE, 0L, MAKELONG(99, 1));
		uSendMessage(w2, UDM_SETPOS, 0L, MAKELONG((c.displaytime / 1000), 0));
		w2 = GetDlgItem(w, IDC_XSPIN);
		uSendMessage(w2, UDM_SETRANGE, 0L, MAKELONG(100, 0));
		uSendMessage(w2, UDM_SETPOS, 0L, MAKELONG(c.x, 0));
		w2 = GetDlgItem(w, IDC_YSPIN);
		uSendMessage(w2, UDM_SETRANGE, 0L, MAKELONG(0, 100));
		uSendMessage(w2, UDM_SETPOS, 0L, MAKELONG(c.y, 0));

		CheckRadioButton(w, IDC_PLEFT, IDC_PRIGHT, IDC_PLEFT + c.pos);
		CheckRadioButton(w, IDC_ALEFT, IDC_ARIGHT, IDC_ALEFT + c.align);


		w = GetDlgItem(hTab, IDC_CONFIG2);
		uSendDlgItemMessage(w, IDC_ALPHA, BM_SETCHECK, !!(c.flags & osd_alpha), 0);
		uSendDlgItemMessage(w, IDC_FADEINOUT, BM_SETCHECK, !!(c.flags & osd_fadeinout), 0);
		uSendDlgItemMessage(w, IDC_DISSOLVE, BM_SETCHECK, !!(c.flags & osd_dissolve), 0);

		w2 = GetDlgItem(w, IDC_ALPHATEXT);
		uSendMessage(w2, TBM_SETRANGE, 0, MAKELONG(0, 255));
		uSendMessage(w2, TBM_SETPOS, 1, 255 - c.alphalev);
		w2 = GetDlgItem(w, IDC_ALPHABACK);
		uSendMessage(w2, TBM_SETRANGE, 0, MAKELONG(0, 255));
		uSendMessage(w2, TBM_SETPOS, 1, 255 - c.alphaback);

		w2 = GetDlgItem(w, IDC_FADESPIN);
		uSendMessage(w2, UDM_SETRANGE, 0L, MAKELONG(5000, 1));
		uSendMessage(w2, UDM_SETPOS, 0L, MAKELONG(c.fadetime, 0));
		w2 = GetDlgItem(w, IDC_DECAYSPIN);
		uSendMessage(w2, UDM_SETRANGE, 0L, MAKELONG(255, 1));
		uSendMessage(w2, UDM_SETPOS, 0L, MAKELONG(c.dissolve_decay, 0));


		w = GetDlgItem(hTab, IDC_CONFIG3);
		uSetDlgItemText(w, IDC_FORMAT, c.format);
		//uSetDlgItemText(w, IDC_FORMATNEXT, c.formatnext);
		SetDlgItemTextA(w, IDC_CCF, "$rgb(0,0,0)");
		SetDlgItemTextA(w, IDC_CC, "$rgb()");


		w = GetDlgItem(hTab, IDC_CONFIG4);
		w2 = GetDlgItem(w, IDC_VWIDTHSPIN);
		uSendMessage(w2, UDM_SETRANGE, 0L, MAKELONG(100, 0));
		uSendMessage(w2, UDM_SETPOS, 0L, MAKELONG(c.vwidth,0));
		w2 = GetDlgItem(w, IDC_VHEIGHTSPIN);
		uSendMessage(w2, UDM_SETRANGE, 0L, MAKELONG(100, 0));
		uSendMessage(w2, UDM_SETPOS, 0L, MAKELONG(c.vheight,0));
		w2 = GetDlgItem(w, IDC_VSTEPSPIN);
		uSendMessage(w2, UDM_SETRANGE, 0L, MAKELONG(100,1));
		uSendMessage(w2, UDM_SETPOS, 0L, MAKELONG(c.vsteps,0));
		w2 = GetDlgItem(w, IDC_VMINSPIN);
		uSendMessage(w2, UDM_SETRANGE, 0L, MAKELONG(1,100));
		uSendMessage(w2, UDM_SETPOS, 0L, MAKELONG(abs(c.vmin) / 100,0));
	}

	static void check_flag(unsigned & flags, unsigned flag, int state)
	{
		if (state) flags |= flag;
		else flags &= ~flag;
	}

	static void save(HWND wnd, osd_config & c)
	{
		HWND hTab, w;
		unsigned n;

		hTab = GetDlgItem(wnd, IDC_TAB);

		w = GetDlgItem(hTab, IDC_CONFIG1);

		check_flag(c.flags, osd_pop, uSendDlgItemMessage(w, IDC_POP, BM_GETCHECK, 0, 0));
		check_flag(c.flags, osd_play, uSendDlgItemMessage(w, IDC_POP_PLAY, BM_GETCHECK, 0, 0));
		check_flag(c.flags, osd_pause, uSendDlgItemMessage(w, IDC_POP_PAUSE, BM_GETCHECK, 0, 0));
		check_flag(c.flags, osd_seek, uSendDlgItemMessage(w, IDC_POP_SEEK, BM_GETCHECK, 0, 0));
		check_flag(c.flags, osd_switch, uSendDlgItemMessage(w, IDC_POP_SWITCH, BM_GETCHECK, 0, 0));
		check_flag(c.flags, osd_dynamic, uSendDlgItemMessage(w, IDC_POP_DYNAMIC, BM_GETCHECK, 0, 0));
		check_flag(c.flags, osd_dynamic_all, uSendDlgItemMessage(w, IDC_POP_DYNAMIC_ALL, BM_GETCHECK, 0, 0));
		check_flag(c.flags, osd_volume, uSendDlgItemMessage(w, IDC_POP_VOLUME, BM_GETCHECK, 0, 0));

		check_flag(c.flags, osd_permanent, uSendDlgItemMessage(w, IDC_PERMANENT, BM_GETCHECK, 0, 0));
		check_flag(c.flags, osd_interval, uSendDlgItemMessage(w, IDC_INTERVAL, BM_GETCHECK, 0, 0));
		check_flag(c.flags, osd_outline, uSendDlgItemMessage(w, IDC_OUTLINE, BM_GETCHECK, 0, 0));
// FUCKO
#ifdef GDIPLUS
		check_flag(c.flags, osd_antialias, uSendDlgItemMessage(w, IDC_ANTIALIAS, BM_GETCHECK, 0, 0));
#endif

		n = atoi(string_utf8_from_window(w, IDC_TIME));
		if (n < 1) n = 1;
		else if (n > 99) n = 99;
		c.displaytime = n * 1000;

		n = atoi(string_utf8_from_window(w, IDC_POSX));
		if (n > 100) n = 100;
		c.x = n;

		n = atoi(string_utf8_from_window(w, IDC_POSY));
		if (n > 100) n = 100;
		c.y = n;

		if (uSendDlgItemMessage(w, IDC_PLEFT, BM_GETCHECK, 0, 0)) n = DT_LEFT;
		else if (uSendDlgItemMessage(w, IDC_PCENTER, BM_GETCHECK, 0, 0)) n = DT_CENTER;
		else n = DT_RIGHT;
		c.pos = n;

		if (uSendDlgItemMessage(w, IDC_ALEFT, BM_GETCHECK, 0, 0)) n = DT_LEFT;
		else if (uSendDlgItemMessage(w, IDC_ACENTER, BM_GETCHECK, 0, 0)) n = DT_CENTER;
		else n = DT_RIGHT;
		c.align = n;


		w = GetDlgItem(hTab, IDC_CONFIG2);
		check_flag(c.flags, osd_alpha, uSendDlgItemMessage(w, IDC_ALPHA, BM_GETCHECK, 0, 0));
		check_flag(c.flags, osd_fadeinout, uSendDlgItemMessage(w, IDC_FADEINOUT, BM_GETCHECK, 0, 0));
		check_flag(c.flags, osd_dissolve, uSendDlgItemMessage(w, IDC_DISSOLVE, BM_GETCHECK, 0, 0));

		c.alphalev = 255 - uSendDlgItemMessage(w, IDC_ALPHATEXT, TBM_GETPOS, 0, 0);
		c.alphaback = 255 - uSendDlgItemMessage(w, IDC_ALPHABACK, TBM_GETPOS, 0, 0);

		n = atoi(string_utf8_from_window(w, IDC_FADETIME));
		if (n < 1) n = 1;
		else if (n > 5000) n = 5000;
		c.fadetime = n;

		n = atoi(string_utf8_from_window(w, IDC_DECAY));
		if (n < 1) n = 1;
		else if (n > 255) n = 255;
		c.dissolve_decay = n;


		w = GetDlgItem(hTab, IDC_CONFIG3);
		c.format = string_utf8_from_window(w, IDC_FORMAT);
		//c.formatnext = string_utf8_from_window(w, IDC_FORMATNEXT);


		w = GetDlgItem(hTab, IDC_CONFIG4);

		n = atoi(string_utf8_from_window(w, IDC_VWIDTH));
		if (n > 100) n = 100;
		c.vwidth = n;

		n = atoi(string_utf8_from_window(w, IDC_VHEIGHT));
		if (n > 100) n = 100;
		c.vheight = n;

		n = atoi(string_utf8_from_window(w, IDC_VSTEPS));
		if (n < 1) n = 1;
		else if (n > 100) n = 100;
		c.vsteps = n;

		n = atoi(string_utf8_from_window(w, IDC_VMIN));
		if (n < 1) n = 1;
		else if (n > 100) n = 100;
		c.vmin = - (int(n) * 100);
	}

	static BOOL CALLBACK RenameProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
	{
		switch(msg)
		{
		case WM_INITDIALOG:
			uSetWindowLong(wnd, DWL_USER, lp);
			{
				string_base * ptr = reinterpret_cast<string_base*>(lp);
				uSetWindowText(wnd, uStringPrintf("Rename preset: \"%s\"", ptr->get_ptr()));
				uSetDlgItemText(wnd, IDC_EDIT, ptr->get_ptr());
				cfg_placement.on_window_creation(wnd);
			}
			return 1;

		case WM_COMMAND:
			switch(wp)
			{
			case IDOK:
				uGetDlgItemText(wnd, IDC_EDIT, *reinterpret_cast<string_base*>(uGetWindowLong(wnd, DWL_USER)));
				EndDialog(wnd, 1);
				break;

			case IDCANCEL:
				cfg_placement.on_window_destruction(wnd);
				EndDialog(wnd, 0);
				break;
			}
			break;

		case WM_CLOSE:
			cfg_placement.on_window_destruction(wnd);
			EndDialog(wnd, 0);
			break;
		}

		return 0;
	}

	static bool rename(string_base & param,HWND parent)
	{
		return !!uDialogBox(IDD_RENAME, parent, RenameProc, reinterpret_cast<long>(&param));
	}

	static BOOL CALLBACK ChildProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		if (msg == WM_COMMAND)
		{
			uSendMessage(GetParent(GetParent(wnd)), WM_CMDNOTIFY, wp, MAKEID(GetMenu(wnd), LOWORD(wp)));
		}
		else if (msg == WM_HSCROLL)
		{
			uSendMessage(GetParent(GetParent(wnd)), WM_HSCNOTIFY, (WPARAM)GetMenu(wnd), lp);
		}

		return 0;
	}

	static BOOL CALLBACK ConfigProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		config_info * ctx;

		switch(msg)
		{
		case WM_INITDIALOG:
			{
				uSendDlgItemMessage(wnd, IDC_ENABLE, BM_SETCHECK, cfg_enable, 0);

				HWND hTab = GetDlgItem(wnd, IDC_TAB);
				HWND w;
				UINT n, count;
				uTCITEM it =
				{
					TCIF_TEXT,
						0, 0,
						0,
						0,
						-1, 0
				};

				for(n = 0; n < tabsize(tab_names); n++)
				{
					it.pszText = (LPSTR)tab_names[n];
					uTabCtrl_InsertItem(hTab, n, &it);
				}
				uSendMessage(hTab,TCM_SETCURFOCUS,0,0);

				RECT r;
				GetClientRect(hTab,&r);
				TabCtrl_AdjustRect(hTab,0,&r);
				if (r.left == 4) r.left = 2; // bleh

				ctx = new config_info;

				uSetWindowLong(wnd, DWL_USER, (long)ctx);

				for (n = 0; n < tabsize(tab_names); n++)
				{
					w = uCreateDialog(IDD_CONFIG1 + n, hTab, ChildProc);
					SetWindowPos(w, 0, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_NOZORDER);
					uSetWindowLong(w, GWL_ID, (long)(IDC_CONFIG1 + n));
					uSetWindowLong(w, DWL_USER, (long)ctx);
				}

				ShowWindow(uGetDlgItem(hTab, IDC_CONFIG1), SW_SHOW);

				array_t<string_simple> names;
				g_osd.get_names(names);

				w = GetDlgItem(wnd, IDC_OVERLAY);

				for (n = 0, count = names.get_size(); n < count; n++)
				{
					uSendMessageText(w, CB_ADDSTRING, 0, names[n]);
				}
				uSendMessage(w, CB_SETCURSEL, 0, 0);

				ctx->n_preset = 0;
				g_osd.get(0, ctx->preset);

				on_count(wnd, names.get_size(), false);

				load(wnd, ctx->preset);

				ctx->initialized = true;
			}
			return 1;

		case WM_NOTIFY:
			switch(wp)
			{
			case IDC_TAB:
				if (((NMHDR*)lp)->code==TCN_SELCHANGE)
				{
					UINT n, cur_tab;
					HWND hTab = ((NMHDR*)lp)->hwndFrom;
					cur_tab = uSendMessage(hTab, TCM_GETCURSEL, 0, 0);
					for(n = 0; n < tabsize(tab_names); n++)
					{
						HWND w = GetDlgItem(hTab, IDC_CONFIG1 + n);
						ShowWindow(w, (n == cur_tab) ? SW_SHOW : SW_HIDE);
					}
				}
				break;
			}
			break;

		case WM_COMMAND:
			ctx = reinterpret_cast<config_info *>(uGetWindowLong(wnd, DWL_USER));

			switch (wp)
			{
			case IDC_ENABLE:
				{
					if (uSendMessage((HWND)lp, BM_GETCHECK, 0, 0))
					{
						if (!g_osd.init()) uSendMessage((HWND)lp, BM_SETCHECK, 0, 0);
						else cfg_enable = 1;
					}
					else
					{
						g_osd.quit();
						cfg_enable = 0;
					}
				}
				break;
			case IDC_OVERLAY_ADD:
				{
					HWND w;
					unsigned count;

					if (ctx->modified)
					{
						switch (uMessageBox(wnd, "The current preset has been modified. Do you want to save the changes (yes),\napply them to the new preset (no) or cancel?", "On-Screen Display configuration", MB_YESNOCANCEL | MB_ICONEXCLAMATION))
						{
						case IDYES:
							save(wnd, ctx->preset);
							g_osd.set(ctx->n_preset, ctx->preset);
							ctx->preset = osd_config();
							break;

						case IDCANCEL:
							return 0;
						}
						EnableWindow(GetDlgItem(wnd, IDC_OVERLAY_APPLY), FALSE);
						ctx->modified = false;
					}
					else ctx->preset = osd_config();

					count = g_osd.add(ctx->preset);

					w = GetDlgItem(wnd, IDC_OVERLAY);
					uSendMessageText(w, CB_ADDSTRING, 0, ctx->preset.name);
					uSendMessage(w, CB_SETCURSEL, ctx->n_preset = count - 1, 0);
					on_count(wnd, count, ctx->modified);
				}
				break;

			case IDC_OVERLAY_REMOVE:
				{
					unsigned count;

					if (ctx->modified)
					{
						EnableWindow(GetDlgItem(wnd, IDC_OVERLAY_APPLY), FALSE);
						ctx->modified = false;
					}

					HWND w = GetDlgItem(wnd, IDC_OVERLAY);
					uSendMessage(w, CB_DELETESTRING, ctx->n_preset, 0);

					count = g_osd.del(ctx->n_preset);
					if (count)
					{
						if (ctx->n_preset == count) ctx->n_preset--;
						g_osd.get(ctx->n_preset, ctx->preset);

						uSendMessage(w, CB_SETCURSEL, ctx->n_preset, 0);

						ctx->initialized = false;
						load(wnd, ctx->preset);
						ctx->initialized = true;
					}

					on_count(wnd, count, ctx->modified);
				}
				break;

			case IDC_OVERLAY_RENAME:
				{
					string8_fastalloc temp;
					temp = ctx->preset.name;

					if (rename(temp, wnd))
					{
						ctx->preset.name = temp;
						g_osd.rename(ctx->n_preset, temp);

						HWND w = GetDlgItem(wnd, IDC_OVERLAY);
						uSendMessage(w, CB_DELETESTRING, ctx->n_preset, 0);
						uSendMessageText(w, CB_INSERTSTRING, ctx->n_preset, ctx->preset.name);
						uSendMessage(w, CB_SETCURSEL, ctx->n_preset, 0);
					}
				}
				break;

			case IDC_OVERLAY_APPLY:
				EnableWindow((HWND)lp, FALSE);
				if (ctx->modified)
				{
					save(wnd, ctx->preset);
					g_osd.set(ctx->n_preset, ctx->preset);
					ctx->modified = false;
				}
				else
				{
					//console::error("NEIN");
					uMessageBox(wnd, "You spoony bard.", "NEIN", MB_ICONEXCLAMATION);
				}
				break;

			case IDC_OVERLAY_TEST:
				g_osd.test(ctx->n_preset);
				break;

			case (CBN_SELCHANGE << 16) | IDC_OVERLAY:
				{
					unsigned n = uSendMessage((HWND)lp, CB_GETCURSEL, 0, 0);

					if (n != ctx->n_preset)
					{
						if (ctx->modified)
						{
							switch (uMessageBox(wnd, "The current preset has been modified. Do you want to save the changes?", "On-Screen Display configuration", MB_YESNOCANCEL | MB_ICONEXCLAMATION))
							{
							case IDCANCEL:
								uSendMessage((HWND)lp, CB_SETCURSEL, ctx->n_preset, 0);
								return 0;

							case IDYES:
								save(wnd, ctx->preset);
								g_osd.set(ctx->n_preset, ctx->preset);
								break;
							}
							EnableWindow(GetDlgItem(wnd, IDC_OVERLAY_APPLY), FALSE);
							ctx->modified = false;
						}

						ctx->n_preset = n;
						g_osd.get(ctx->n_preset, ctx->preset);

						ctx->initialized = false;
						load(wnd, ctx->preset);
						ctx->initialized = true;
					}
				}
				break;
			}
			break;

		case WM_CMDNOTIFY:
			ctx = reinterpret_cast<config_info *>(uGetWindowLong(wnd, DWL_USER));

			if (ctx->initialized)
			{
				switch (lp)
				{
				case MAKEID(IDC_CONFIG1, IDC_FONT):
					{
						if (ctx->preset.font.popup_dialog(wnd))
						{
							on_modified(ctx, wnd);
						}
					}
					break;

				case MAKEID(IDC_CONFIG1, IDC_COLOR):
					{
						DWORD temp = ctx->preset.color;
						CUSTOMCOLORS meh = cfg_customcolors;
						if (uChooseColor(&temp, wnd, (DWORD*)&meh.colors))
						{
							ctx->preset.color = temp;
							on_modified(ctx, wnd);
						}
						cfg_customcolors = meh;
					}
					break;

				case MAKEID(IDC_CONFIG1, IDC_BGCOLOR):
					{
						DWORD temp = ctx->preset.bgcolor;
						CUSTOMCOLORS meh = cfg_customcolors;
						if (uChooseColor(&temp, wnd, (DWORD*)&meh.colors))
						{
							ctx->preset.bgcolor = temp;
							on_modified(ctx, wnd);
						}
						cfg_customcolors = meh;
					}
					break;

				case MAKEID(IDC_CONFIG3, IDC_TAGZHELP):
					standard_commands::main_titleformat_help();
					break;

				case MAKEID(IDC_CONFIG3, IDC_RESET):
					{
						HWND w = GetDlgItem(GetDlgItem(GetDlgItem(wnd, IDC_TAB), IDC_CONFIG3), IDC_FORMAT);
						string_utf8_from_window fmt(w);
						if (strcmp(fmt, default_format))
						{
							uSetWindowText(w, default_format);
							ctx->preset.format = default_format;
							on_modified(ctx, wnd);
						}
					}
					break;

				/*case MAKEID(IDC_CONFIG3, IDC_RESET_NEXT):
					{
						HWND w = GetDlgItem(GetDlgItem(GetDlgItem(wnd, IDC_TAB), IDC_CONFIG3), IDC_FORMATNEXT);
						string_utf8_from_window fmt(w);
						if (strcmp(fmt, default_format_next))
						{
							uSetWindowText(w, default_format_next);
							ctx->preset.formatnext = default_format_next;
							on_modified(ctx, wnd);
						}
					}
					break;*/

				case MAKEID(IDC_CONFIG3, IDC_CCP):
					{
						HWND w = GetDlgItem(GetDlgItem(GetDlgItem(wnd, IDC_TAB), IDC_CONFIG3), IDC_CCF);
						string8_fastalloc temp;
						temp = string_utf8_from_window(w);
						const char * ptr = temp.get_ptr() + 5;
						DWORD color = strtoul(ptr, (char **) &ptr, 10);
						color |= strtoul(ptr + 1, (char **) &ptr, 10) << 8;
						color |= strtoul(ptr + 1, 0, 10) << 16;
						CUSTOMCOLORS meh = cfg_customcolors;
						if (uChooseColor(&color, wnd, (DWORD*)&meh.colors))
						{
							temp = "$rgb(";
							temp.add_int(color & 255);
							temp.add_byte(',');
							temp.add_int((color >> 8) & 255);
							temp.add_byte(',');
							temp.add_int((color >> 16) & 255);
							temp.add_byte(')');
							uSetWindowText(w, temp);
						}
						cfg_customcolors = meh;
					}
					break;

				case MAKEID(IDC_CONFIG1, IDC_POP):
				case MAKEID(IDC_CONFIG1, IDC_POP_PLAY):
				case MAKEID(IDC_CONFIG1, IDC_POP_PAUSE):
				case MAKEID(IDC_CONFIG1, IDC_POP_SEEK):
				case MAKEID(IDC_CONFIG1, IDC_POP_SWITCH):
				case MAKEID(IDC_CONFIG1, IDC_POP_DYNAMIC):
				case MAKEID(IDC_CONFIG1, IDC_POP_DYNAMIC_ALL):
				case MAKEID(IDC_CONFIG1, IDC_POP_VOLUME):
				case MAKEID(IDC_CONFIG1, IDC_POP_STOP):
				case MAKEID(IDC_CONFIG1, IDC_PERMANENT):
				case MAKEID(IDC_CONFIG1, IDC_INTERVAL):
				case MAKEID(IDC_CONFIG1, IDC_OUTLINE):
				case MAKEID(IDC_CONFIG1, IDC_ANTIALIAS):
				case MAKEID(IDC_CONFIG1, IDC_PLEFT):
				case MAKEID(IDC_CONFIG1, IDC_PCENTER):
				case MAKEID(IDC_CONFIG1, IDC_PRIGHT):
				case MAKEID(IDC_CONFIG1, IDC_ALEFT):
				case MAKEID(IDC_CONFIG1, IDC_ACENTER):
				case MAKEID(IDC_CONFIG1, IDC_ARIGHT):
				case MAKEID(IDC_CONFIG2, IDC_ALPHA):
				case MAKEID(IDC_CONFIG2, IDC_FADEINOUT):
				case MAKEID(IDC_CONFIG2, IDC_DISSOLVE):
					on_modified(ctx, wnd);
					break;

				case MAKEID(IDC_CONFIG1, IDC_TIME):
				case MAKEID(IDC_CONFIG1, IDC_POSX):
				case MAKEID(IDC_CONFIG1, IDC_POSY):
				case MAKEID(IDC_CONFIG2, IDC_FADETIME):
				case MAKEID(IDC_CONFIG2, IDC_DECAY):
				case MAKEID(IDC_CONFIG3, IDC_FORMAT):
				//case MAKEID(IDC_CONFIG3, IDC_FORMATNEXT):
				case MAKEID(IDC_CONFIG4, IDC_VWIDTH):
				case MAKEID(IDC_CONFIG4, IDC_VHEIGHT):
				case MAKEID(IDC_CONFIG4, IDC_VSTEPS):
				case MAKEID(IDC_CONFIG4, IDC_VMIN):
					if (HIWORD(wp) == EN_CHANGE) on_modified(ctx, wnd);
					break;
				}
			}
			break;

		case WM_HSCNOTIFY:
			ctx = reinterpret_cast<config_info *>(uGetWindowLong(wnd, DWL_USER));

			if (ctx->initialized)
			{
				on_modified(ctx, wnd);
			}
			break;

		case WM_DESTROY:
			{
				HWND hTab = GetDlgItem(wnd, IDC_TAB);
				for (UINT n = 0; n < tabsize(tab_names); n++)
				{
					HWND w = GetDlgItem(hTab, IDC_CONFIG1 + n);
					DestroyWindow(w);
				}

				ctx = reinterpret_cast<config_info *>(GetWindowLong(wnd, DWL_USER));

				if (ctx->modified)
				{
					if (uMessageBox(wnd, "The current preset has been modified. Do you want to save the changes?", "On-Screen Display configuration", MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
					{
						g_osd.set(ctx->n_preset, ctx->preset);
					}
				}

				delete ctx;
			}
			break;
		}
		return 0;
	}

public:
	virtual HWND create(HWND parent)
	{
		return uCreateDialog(IDD_CONFIG,parent,ConfigProc);
	}
	GUID get_guid()
	{
		// {4D3CC458-020A-409c-8DA2-A760D5E01E73}
		static const GUID guid = 
		{ 0x4d3cc458, 0x20a, 0x409c, { 0x8d, 0xa2, 0xa7, 0x60, 0xd5, 0xe0, 0x1e, 0x73 } };
		return guid;
	}
	virtual const char * get_name() {return "On-Screen Display";}
	GUID get_parent_guid() {return guid_components;}

	bool reset_query() {return true;}
	void reset()
	{
		g_osd.quit();
		cfg_enable = 0;
		g_osd.reset();
	}
};

static preferences_page_factory_t<preferences_page_osd> g_config_osd_factory;
