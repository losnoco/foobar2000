#ifndef _UI_EXTENSION_EXTENSION_H_
#define _UI_EXTENSION_EXTENSION_H_

/**
 * Interface for ui_extension service.
 *
Subclasses <em>need to implement</em> the following methods:
\code
virtual const GUID & get_extension_guid()=0;
virtual void get_name(string_base & out)=0;
virtual void get_category(string_base & out)=0;
virtual bool is_available(ui_extension_host * p_host)=0;
virtual HWND init_or_take_ownership(HWND wnd_parent, ui_extension_host * p_host, const void * data, unsigned size)=0; 
virtual void destroy_window()=0;
virtual HWND get_wnd()=0;
\endcode

Subclasses <em>can overide</em> the following methods:
\code
// Override to provide a short name.
virtual bool get_short_name(string_base & out) {return false;}
// Override to provide a description.
virtual bool get_description(string_base & out) {return false;}
// Override if you use per instance configuration data.
virtual void get_config(cfg_var::write_config_callback * out) {}
// Override if you want to add items to the host's menu.
virtual void build_menu(HMENU menu, unsigned base, POINT pt, bool show_shortcuts, void * & user_data_out) {}
virtual void menu_action(HMENU menu, unsigned base, unsigned id, void * & user_data_in) {}
// Override if you want these items to have a description.
virtual bool get_menu_action_description(HMENU menu, unsigned base, unsigned id, string_base & out, void * & user_data_in) {return false;}
\endcode
 *
 * \par Window messages sent by host
 * <code>WM_GETMINMAXINFO</code>
 * \todo update, add notes on how (multiple) instance service gets used
 */
class NOVTABLE ui_extension : public service_base
{
public:

	/**
	 * ui_extension class GUID
	 *
	 * Used by service mechanism.
	 */
	static const GUID class_guid;

	/**
	 * Get ui_extension class GUID
	 * 
	 * Used by service mechanism.
	 *
	 * \return class GUID
	 */
	static inline const GUID & get_class_guid(){return class_guid;}

	/**
	 * Get unique ID of extension.
	 *
	 * This GUID is used to identify a specific extension.
	 *
	 * \return extension GUID
	 */
	virtual const GUID & get_extension_guid() const = 0;

	/**
	 * Get the name of the extension.
	 *
	 * Get a user-readable name of the extension.
	 *
	 * \warning
	 * Do not use the name to identify extensions; use extension GUIDs instead.
	 *
	 * \param[out] out receives the name of the extension, e.g. "Playlist buttons"
	 *
	 * \see get_extension_guid
	 */
	virtual void get_name(string_base & out) const =0; 

	/**
	 * Get the category of the extension.
	 *
	 * \param[out] out receives the category of the extension, e.g. "Toolbars"
	 * \todo document recommended categories
	 */
	virtual void get_category(string_base & out) const =0;

	/**
	 * Get the short name of the extension.
	 *
	 * Get a shorter, presumably friendlier, name of the extension.
	 *
	 * \param[out] out receives the short name of the extension, e.g. "Order" instead
	 * of "Playback order", or "Playlists" instead of "Playlist switcher"
	 * \return true if the extension has a short name
	 */
	virtual bool get_short_name(string_base & out) const {return false;}; //short/friendly name, e.g. order vs. playback order, playlists vs. playlist switcher

	/**
	 * Get the description of the extension.
	 *
	 * \param[out] out receives the descriptopm of the extension,
	 * e.g. "Drop-down list for displaying and changing the current playback order"
	 * \return true if the extension has a short name
	 */
	virtual bool get_description(string_base & out) const {return false;}; //e.g. "Drop-down list to display the current playback order and to allow you to select a new playback order"

	/**
	 * Get the type of the extension.
	 *
	 * \return a combination of ui_extension_flag::TYPE_* flags
	 *
	 * \see ui_extension_flag::ui_extension_type
	 */
	virtual unsigned get_type() const =0; //return UET_* flags above, can combine flags

	/**
	 * 
	 * \todo document
	 */
	virtual bool get_prefer_multiple_instances() const {return false;}


	/**
	 * Get availability of the extension.
	 *
	 * This method is called before init_or_take_ownership() to test, if this call will be legal.
	 * If this instance is already hosted, it should check whether the given host equals its
	 * current host, and should return <code>false</code>, if it does. This is mostly important 
	 * for single instance extensions.
	 *
	 * Extensions that support multiple instances can generally return <code>true</code>.
	 *
	 * \return whether this instance can be created in or moved to the given host
	 */
	virtual bool is_available(ui_extension_host * p_host) const =0;

	/**
	 * Create or transfer extension window.
	 *
	 * create your window here
	 * or if it already created (i.e. single instance windows), set parent to wnd_parent
	 * and modify win styles as neccesary (WS_CHILD, WS_EX_CONTROLPARENT etc., no WS_POPUP, WS_CAPTION)
	 * and hide your window (ShowWindow(wnd, SW_HIDE))
	 * and if it is owned by some other host, call its relinquish_ownership()
	 *
	 * use WS_EX_CONTROLPARENT if you have child windows that receive keyboard input, and you want them to be included in tab operations in the host window
	 *
	 * remember to make a parent window as well to catch any notifications etc. e.g. if using common controls, or just use a dialog
	 *
	 * \pre May only be called if is_available() returned true.
	 *
	 * \post The created window must not be visible. It must have the WS_CHILD window style.
	 * It must have a dialog item ID of 0.
	 *
	 * \param[in] wnd_parent window handle of the container provided by the host
	 * \param[in] p_host pointer to the host that creates the extension.
	 * This parameter may not be NULL. It is not necessary to call <code>service_add_ref()</code> and
	 * <code>service_release()</code> on this object.
	 * \param[in] config   configuration data for this extension instance.
	 * \return window handle of the extension window
	 * \see cfg_var::read_config_helper
	 */
	virtual HWND init_or_take_ownership(HWND wnd_parent, ui_extension_host * p_host, cfg_var::read_config_helper * config)=0; 

	/**
	 * Get instance configuration data.
	 *
	 * \param[in] out
	 */
	virtual void get_config(cfg_var::write_config_callback * out){};

	/**
	 * Destroy extension window.
	 */
	virtual void destroy_window()=0; 

	/**
	 * Called when the host menu is created.
	 *
	 * Extensions can add their own items to the host menu.
	 * Note that extensions should handle WM_CONTEXTMENU, if they wish to have
	 * a context menu of their own.
	 *
	 * \pre May only be called on hosted extensions.
	 *
	 * \param[in] menu handle of the host menu
	 * \param[in] base base index of extension menu IDs
	 * \param[in] pt location of the menu
	 * \param[in] show_shortcuts whether keyboard shortcuts should be shown in the menu (only if applicable)
	 * \param[out] user_data used by the extension to store data needed to manage its entries in the host menu
	 */
	virtual void build_menu(HMENU menu, unsigned base, POINT pt, bool show_shortcuts, void * & user_data){}; 

	/**
	 * Called when the host menu is destroyed.
	 * 
	 * The host uses this method to pass information about the selected command to the extension.
	 * It will even be called, when none of the extension's commands was selected. The extension
	 * can use this method to perform clean-up.
	 *
	 * \pre May only be called on hosted extensions. May only be called after build_menu.
	 *
	 * \param[in] menu handle to the host menu
	 * \param[in] base base index of the extensions menu IDs
	 * \param[in] id ID of the selected command
	 * \param[in, out] user_data the value the extension stored in user_data_out in the previous call to build_menu
	 * \todo explicitly state you should perform your action here
	 */
	virtual void menu_action(HMENU menu, unsigned base, unsigned id, void * user_data){}; 

	/**
	 * Called when the host menu is destroyed.
	 * 
	 * The host uses this method to pass information about the selected command to the extension.
	 * It will even be called, when none of the extension's commands was selected. The extension
	 * can use this method to perform clean-up.
	 *
	 * \pre May only be called on hosted extensions. May only be called between build_menu and menu_action.
	 *
	 * \param[in] menu handle to the host menu
	 * \param[in] base base index of the extensions menu IDs
	 * \param[in] id ID of a command added by the extension
	 * \param[in] out store the description here, if available
	 * \param[in] user_data the value the extension stored in user_data_out in the previous call to build_menu
	 * \return whether a description for the item is available (defaults to <code>false</code>)
	 * \todo fix this its nothing to do with destroying menu
	 */
	virtual bool get_menu_action_description(HMENU menu, unsigned base, unsigned id, string_base & out, void * & user_data){return false;}; 

	/**
	 * Get extension window.
	 *
	 * \pre May only be called on hosted extensions.
	 *
	 * \return window handle of the extension window
	 */
	virtual HWND get_wnd() const =0;

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}

	/**
	 * Create extension by GUID.
	 *
	 * \param[in] guid GUID of a ui_extension
	 * \return A pointer to an instance of a ui_extension with the given
	 * GUID, or NULL if no such ui_extension was found. In the first case,
	 * the returned instance must be freed using <code>service_release()</code>.
	 */
	static inline bool create_by_guid(service_ptr_t<ui_extension> & p_out, const GUID & guid) 
	{
		service_enum_t<ui_extension> e;
		service_ptr_t<ui_extension> l;

		unsigned n=0;

		for(; e.first(l); e.next(l))
		{
			if (l->get_extension_guid() == guid) { p_out = l; return true; }
		}

		return false;
	}

	static HWND g_on_tab(HWND wnd_focus);
};

/**
 * Subclass of ui_extension, specifically for menubars.
 * \todo elaborate...
 */
class NOVTABLE menu_ui_extension : public ui_extension
{
public:
	/**
	 * menu_ui_extension class GUID
	 *
	 * Used by service mechanism.
	 */
	static const GUID class_guid;

	/**
	 * Get ui_extension class GUID
	 * 
	 * Used by service mechanism.
	 *
	 * \return class GUID
	 */

	static inline const GUID & get_class_guid(){return class_guid;}
	/**
	 * Called by host in its WM_MENUCHAR handler to notify extension that a menu was requested to be opened. 
	 * You should check whether the accelerator key pressed is one of yours.
	 *
	 * \pre May only be called on hosted extensions.
	 *
	 * \param[in] chr character that was pressed
	 * \return whether you claimed the accelerator key, and showed/will show your menu
	 */
	virtual bool on_menuchar(unsigned short chr)=0; 

	/**
	 * Called by host to indicate you should focus your menu.
	 *
	 * \pre May only be called on hosted extensions.
	 */
	virtual void set_focus()=0;	// set the focus to your menu

	/**
	 * Retrieve whether the menu has the keyboard focus..
	 *
	 * \pre May only be called on hosted extensions.
	 * \return whether your menu has keyboard focus
	 */
	virtual bool is_menu_focused()=0;

	/**
	 * \todo document
	 */
	virtual void show_accelerators()=0;

	/**
	 * \todo document
	 */
	virtual void hide_accelerators()=0;
	// for NT5 & newer
	// before doing anything check OS and for hide_accelerators() whether user has enabled "Hide underlined letters for keyboard navigation until I press the Alt key" in display properties
	// then you would probably have to post message to your window to perform req action
	// in hide_accelerators() check that your menu does not have the focus first !

	//e.g. rough code for hide_accelerators()
	/*
	if (is_win2k_or_newer())
	{
		BOOL a = true; 
		SystemParametersInfo(SPI_GETKEYBOARDCUES, 0, &a, 0);
		uPostMessage(wnd_menu, WM_UPDATEUISTATE, MAKEWPARAM(a ? UIS_CLEAR : UIS_SET , UISF_HIDEACCEL), 0); //standard method of hiding/showing accelerator chars
	}
	*/

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return ui_extension::service_query(guid);
	}
};

/**
 * Service factory for multiple instance UI extensions.
 * \par Usage example
 * \code
 * static ui_extension_factory< my_ui_extension > foo_extension;
 * \endcode
 */
template<class T>
class ui_extension_factory : public service_factory_t<ui_extension,T> {};

/**
 * Service factory for single instance UI extensions.
 * \par Usage example
 * \code
 * static ui_extension_factory_single< my_ui_extension > foo_extension2;
 * \endcode
 * The static instance of <code>my_ui_extension</code> can be accessed
 * as <code>foo_extension2.get_static_instance()</code>.
 */
template<class T>
class ui_extension_factory_single : public service_factory_single_t<ui_extension,T> {};

/**
 * Service factory for single instance UI extensions.
 * \par Usage example
 * \code
 * static ui_extension_factory_single_transparent< my_ui_extension > foo_extension3;
 * \endcode
 * The static instance of <code>my_ui_extension</code> can be accessed
 * as <code>foo_extension3</code>.
 */
template<class T>
class ui_extension_factory_single_transparent : public service_factory_single_transparent_t<ui_extension,T> {};

#endif
