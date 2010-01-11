#if !defined(UI_EXTENSION_HELPER_H)
#define UI_EXTENSION_HELPER_H

#include "ui_extension.h"

enum ui_extension_multiplicity
{
	UEM_MULTIPLE,
	UEM_SINGLE,
};

template< ui_extension_flag::ui_extension_type TYPE, ui_extension_multiplicity MULT >
class NOVTABLE ui_extension_base_t : public ui_extension
{
protected:
	HWND wnd;
	ui_extension_host * host;

public:
	ui_extension_base_t()
	{
		wnd = NULL;
		host = NULL;
	}

	virtual unsigned get_type() const {return TYPE;}

	virtual bool is_available(ui_extension_host * p_host) const
	{
		if (MULT == UEM_SINGLE) {
			return p_host != host;
		} else {
			return true;
		}
	}

	virtual HWND init_or_take_ownership(HWND wnd_parent, ui_extension_host * p_host, cfg_var::read_config_helper * config)
	{
		if (wnd == NULL) {
			// Create new window.

			// Store host.
			host = p_host;

			// only apply configuration data to a new instance
			set_config(config);

			// create window
			wnd = create_window(wnd_parent);
		} else {
			// Transfer existing window to new host.
			// Possibly extend this to handle free-floating <-> hosted transitions.

			// Set new parent window.
			SetParent(wnd, wnd_parent);

			// Tell old host to let us go. We need to do this after using SetParent()!
			host->relinquish_ownership(wnd);

			// Store new host.
			host = p_host;
		}

		// ensure the window is not visible
		ShowWindow(wnd, SW_HIDE);

		return wnd;
	}

	virtual HWND get_wnd() const {return wnd;}

	virtual void destroy_window()
	{
		if (wnd != NULL) {
			DestroyWindow(wnd);
			wnd = NULL;
		}
	}

protected:
	// Override these in subclasses.

	virtual HWND create_window(HWND wnd_parent) = 0;

	virtual void set_config(cfg_var::read_config_helper * config) {}
	virtual void get_config(cfg_var::write_config_callback * out) {}

};

#endif /* !defined(UI_EXTENSION_HELPER_H) */
