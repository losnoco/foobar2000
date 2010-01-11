#ifndef _UI_EXTENSION_HOST_H_
#define _UI_EXTENSION_HOST_H_

/**
 * Interface for ui_extension_host service.
 * This interface is to be implemented by UI extension hosts.
 * It provides information about the host and its state as well
 * as methods to manage hosted extensions.
 *
Subclasses <em>need to implement</em> the following methods:
\code
virtual const GUID & get_host_guid() const =0;
virtual void get_name(string_base & out)=0;
virtual bool is_available()=0;
virtual unsigned get_supported_types()=0;
virtual void insert_extension (const GUID & guid, unsigned height, unsigned width)=0;
virtual void insert_extension (class ui_extension * p_ext, unsigned height, unsigned width)=0;
virtual void on_size_limit_change(HWND wnd, unsigned flags)=0;
virtual void override_status_text(const char * text)=0;
virtual void restore_status_text()=0;
virtual bool get_keyboard_shortcuts_enabled()=0;
virtual bool get_show_shortcuts()=0;
virtual void relinquish_ownership(HWND wnd)=0;
\endcode
\todo update
 */
class NOVTABLE ui_extension_host : public service_base
{
public:
	/**
	 * ui_extension_host class GUID
	 * 
	 * Used by service mechanism.
	 */
	static const GUID class_guid;

	/**
	 * Get ui_extension_host class GUID
	 * 
	 * Used by service mechanism.
	 *
	 * \return class GUID
	 */
	static inline const GUID & get_class_guid(){return class_guid;}

	/**
	 * Get the unique ID of the host.
	 *
	 * This GUID is used to identify a specific host.
	 *
	 * \return host GUID
	 */
	virtual const GUID & get_host_guid()const=0;

	/**
	 * Get the name of the host.
	 *
	 * Get a user-readable name of the host.
	 *
	 * \warning
	 * Do not use the name to identify hosts; use host GUIDs instead.
	 *
	 * \param[out] out receives the name of the host, e.g. "My UI/Sidebar"
	 *
	 * \see get_host_guid
	 */
	virtual void get_name(string_base & out) const =0;

	/**
	 * Get availability of the host.
	 *
	 * \return true if it is possible to insert a UI extension into the host.
	 *
	 * \see insert_extension(const GUID &, unsigned, unsigned), insert_extension(ui_extension *, unsigned, unsigned)
	 */
	virtual bool is_available() const=0;

	/**
	 * Get supported UI extension types.
	 *
	 * \return a combination of ui_extension_flag::TYPE_* flags to indicate recommended types for the host
	 * \see ui_extension_flag::ui_extension_type
	 */
	virtual unsigned get_supported_types() const=0; 

	/**
	 * Insert new instance of a UI extension.
	 *
	 * Creates an instance of the specified extension and inserts it into the host's
	 * client area. Single-instance extensions should removed themselves from the
	 * old host, if any.
	 *
	 * \pre May only be called, if is_available() returned true.
	 *
	 * \param[in] guid unique ID of the UI extension to be inserted
	 * \param[in] height desired height of the new panel
	 * \param[in] width desired width of the new panel
	 *
	 * \see is_available, ui_extension::init_or_take_ownership
	 */
	virtual void insert_extension (const GUID & guid, unsigned height, unsigned width)=0;

	/**
	 * Insert existing instance of a UI extension.
	 *
	 * Inserts the given UI extension instance into the host's client area.
	 *
	 * \pre May only be called, if is_available() returned true.
	 *
	 * \param[in] p_ext pointer to the UI extension instance to be inserted
	 * \param[in] height desired height of the new panel
	 * \param[in] width desired width of the new panel
	 *
	 * \see is_available, ui_extension::init_or_take_ownership
	 */
	virtual void insert_extension (class ui_extension * p_ext, unsigned height, unsigned width)=0; //insert existing instance (or new if it hasnt been initialised)

	/**
	 * Notify host about changed size limits of a hosted extension.
	 *
	 * \pre May only be called by a hosted UI extension.
	 *
	 * \param[in] wnd window handle of the extension's window
	 * \param[in] flags a combination of SLC_* flags indicating which size limits changed
	 *
	 * \see ui_extension_flag::size_limit_changed
	 */
	virtual void on_size_limit_change(HWND wnd, unsigned flags)=0;

	/**
	* Called by ui extension hosted by this host to find out whether the host supports resizing
	* 
	* \param[in] wnd   handle to the window to test
	* \return   combination of ui_extension_flag::RSZ_* to indicate wether the width or height can be nodified
	*
	* \pre May only be called by a hosted UI extension.
	*
	* \see ui_extension_flag::resize_flags
	*/
	virtual unsigned is_resize_supported(HWND wnd)=0;

	/**
	* Called by ui extension hosted by this host to find out whether the host supports resizing
	* 
	* \param[in] wnd   handle to the window to test
	* \return   combination of ui_extension_flag::RSZ_* to indicate wether the width or height can be nodified
	*
	* \todo finish
	*/
	virtual bool request_resize(HWND wnd, unsigned flags, unsigned width, unsigned height)=0;
	//flags - RSZ_*
	//if extension wants both width and height changed, but you cant do both, do not do any resizing, and return false.
	//it is then upto the extension to call you with either just width or height, if it wants to

	/**
	 * Override status bar text.
	 *
	 * Can be used to display additional information about menu commands in
	 * a menu bar or context menu.
	 *
	 * \pre May only be called by a hosted UI extension.
	 *
	 * \param[in] text text to be displayed in the status bar
	 */
	virtual void override_status_text(const char * text)=0;

	/**
	 * Restore status bar text.
	 *
	 * Returns control over status bar contents to host.
	 *
	 * \pre May only be called by a hosted UI extension.
	 */
	virtual void restore_status_text()=0; //restore status bar text

	/**
	 * Query if keyboard shortcuts should be processed.
	 *
	 * Use this to determine, if keyboard shortcuts should be processed.
	 * Do not process them, if this method returns false.
	 * Shortcuts can be processed using the keyboard_shortcut_manager service
	 * from the foobar2000 SDK.
	 * 
	 * The user should be able to navigate using the tab key. If VK_TAB is not
	 * processed by the keyboard_shortcut_manager and you are not using the
	 * modeless_dialog_manager either, you should use g_on_tab() to change to the
	 * next control.
	 *
	 * \par Usage example
	 * \code
	 * case WM_KEYDOWN:
	 *     if (p_host->get_keyboardshortcuts_enabled() && keyboard_shortcut_manager::get()->something()) break;
	 *     else if (wp == VK_TAB) ui_extension::g_on_tab(wnd);
	 * 	   break;
	 * \endcode
	 *
	 * \pre May only be called by a hosted UI extension.
	 *
	 * \return whether keyboard shortcuts should be processed
	 */
	virtual bool get_keyboard_shortcuts_enabled()const=0;

	/**
	 * Query if keyboard shortcuts should be sgown in menus.
	 *
	 * \pre May only be called by a hosted UI extension.
	 *
	 * \return whether keyboard shortcuts should be sgown in menus.
	 */
	virtual bool get_show_shortcuts()const=0;

	/**
	 * Query if extension window is visible.
	 * An extension that is not visible does not imply that its window has been hidden using ShowWindow
	 *
	 * \param[in] wnd   handle to the window to test
	 * \pre May only be called by a hosted UI extension.
	 *
	 * \return whether window is visible.
	 */
	virtual bool is_visible(HWND wnd)=0;

	/**
	 * Query if extension window can be hidden or shown.
	 *
	 * \param[in] wnd   handle to the window to test
	 * \param[in] desired_visibility   whether you want the window to be visible
	 * \pre May only be called by a hosted UI extension.
	 *
	 * \return whether the required visiblility can be set.
	 */
	virtual bool is_visibility_modifiable(HWND wnd, bool desired_visibility)=0;
	virtual bool set_window_visibility(HWND wnd, bool visibility)=0;

	/**
	 * Hides or shows extension window.
	 *
	 * \param[in] wnd   handle to the window to test
	 * \param[in] visibility   whether you want the window to be visible
	 * \pre May only be called by a hosted UI extension.
	 *
	 * \return whether the required visiblility was be set.
	 */

	/**
	 * Relinquish ownership of a UI extension instance.
	 *
	 * Call this to remove control of an extension window from the host.
	 * The host will not destroy the window as a result of this call.
	 * However, the window may be destroyed, if the host destroys the
	 * containing winow, so be sure to call <code>SetParent</code> first.
	 * The host will call <code>service_release</code>, when it removes
	 * the extension belonging to the window.
	 * 
	 * Reasons for calling this method include: another host tries to take
	 * ownership of an existing extension instance, the window should be
	 * destroyed/closed, or the window is to be turned into a popup dialog.
	 *
	 * \pre May only be called by a hosted UI extension.
	 *
	 * \param[in] wnd window handle of the extension's window
	 *
	 * \see ui_extension::init_or_take_ownership
	 */
	virtual void relinquish_ownership(HWND wnd)=0;

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}

};

/**
 * Service factory for UI extensions hosts.
 * \par Usage example
 * \code
 * static ui_extension_host_factory< my_ui_extension_host > foo_host;
 * \endcode
 * The static instance of <code>my_ui_extension_host</code> can be accessed
 * as <code>foo_host.get_static_instance()</code>.
 */
template<class T>
class ui_extension_host_factory : public service_factory_single_t<ui_extension_host,T> {};

/**
 * Service factory for UI extensions hosts.
 * \par Usage example
 * \code
 * static ui_extension_host_factory_transparent< my_ui_extension_host > foo_host2;
 * \endcode
 * The static instance of <code>my_ui_extension_host</code> can be accessed
 * as <code>foo_host2</code>.
 */
template<class T>
class ui_extension_host_factory_transparent : public service_factory_single_transparent_t<ui_extension_host,T> {};

#endif