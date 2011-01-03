#ifndef _UI_EXTENSION_EXTENSION_H_
#define _UI_EXTENSION_EXTENSION_H_

#if 0
namespace ui_extension{

class NOVTABLE extension_base : public service_base
{
public:

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
	 * \param[out] out receives the name of the extension, e.g. "Spectrum analyser"
	 *
	 * \see get_extension_guid
	 */
	virtual void get_name(string_base & out) const =0; 

	/**
	 * Set instance configuration data.
	 * Note: only called before enabled.
	 * Must not be used by single instance extensions.
	 *
	 * \param[in] config
	 */
	virtual void set_config(cfg_var::read_config_helper * config){};

	/**
	 * Get instance configuration data.
	 * Must not be used by single instance extensions.
	 *
	 * \param[in] out
	 */
	virtual void get_config(cfg_var::write_config_callback * out){};

	virtual bool have_config_popup(){return false;}

	virtual bool show_config_popup(HWND wnd_parent){return false;}

	virtual void get_menu_items (menu_hook & p_hook) {};

};

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
 * \todo update, add some notes
 */
class NOVTABLE window : public extension_base
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
	* Get whether extension is single instance or not.
	*
	* Do not explicitly override
	*/
	virtual const bool get_is_single_instance() const = 0;

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
	virtual bool is_available(const window_host_ptr & p_host) const =0;

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
	virtual HWND create_or_transfer_window(HWND wnd_parent, const window_host_ptr & p_host)=0; 

	/**
	 * Destroy extension window.
	 */
	virtual void destroy_window()=0; 

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
	static inline bool create_by_guid(const GUID & guid, window_ptr & p_out) 
	{
		service_enum_t<window> e;
		service_ptr_t<window> ptr;

		
		if (e.first(ptr)) do {
			if (ptr->get_extension_guid() == guid)
			{
				p_out.copy(ptr);
				return true;
				
			}
		} while(e.next(ptr));
		return false;
	}

	static HWND g_on_tab(HWND wnd_focus);
};

/**
 * Subclass of ui_extension, specifically for menubars.
 * \todo elaborate...
 */
class NOVTABLE menu_window : public window
{
public:
	/**
	 * menu_ui_extension class GUID
	 *
	 * Used by service mechanism.
	 */
	static const GUID class_guid;

	/**
	 * Get menu_ui_extension class GUID
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
		else return window::service_query(guid);
	}
};

/**
 * Subclass of window for playlist views
 */
class NOVTABLE playlist_window : public window
{
public:
	/**
	 * Class GUID
	 *
	 * Used by service mechanism.
	 */
	static const GUID class_guid;

	/**
	 * Get class GUID
	 * 
	 * Used by service mechanism.
	 *
	 * \return class GUID
	 */

	static inline const GUID & get_class_guid(){return class_guid;}

	/**
	 * Called by host to indicate you should focus your window.
	 *
	 * \pre May only be called on hosted extensions.
	 */
	virtual void set_focus()=0;

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return window::service_query(guid);
	}
};

	class NOVTABLE extension_data
	{
	public:
		virtual const GUID & get_guid()const=0;
		virtual void get_data (cfg_var::write_config_callback * p_out)const=0;
		virtual void set_guid(const GUID & p_guid) =0;
		virtual void set_data (const void * p_data, unsigned p_size)=0;
		virtual window_ptr & get_window_ptr()=0;
		virtual void set_window_ptr(const window_ptr & p_source)=0;
		inline void set(const extension_data & p_source)
		{
			set_guid(p_source.get_guid());
			cfg_var::write_config_callback_i temp;
			p_source.get_data(&temp);
			set_data(temp.data, temp.data.get_size());
		}
		inline extension_data() {;}
		inline const extension_data & operator= (const extension_data & p_source) 
		{
			set(p_source);
			return *this;
		}
	};

	class extension_data_impl : public extension_data
	{
	public:
		virtual const GUID & get_guid()const {return m_guid;}
		virtual void get_data (cfg_var::write_config_callback * p_out)const {p_out->write(m_data, m_data.get_size());}
		virtual void set_guid(const GUID & p_guid) {m_guid = p_guid;}
		virtual void set_data (const void * p_data, unsigned p_size) {m_data.force_reset();m_data.append(p_data, p_size);}
		virtual window_ptr & get_window_ptr(){return m_ptr;}
		virtual void set_window_ptr(const window_ptr & p_source){m_ptr = p_source;}
		inline extension_data_impl(const extension_data & p_source) 
		{
			set(p_source);
		}
		inline extension_data_impl() : m_guid(pfc::guid_null)
		{
		}
	private:
		GUID m_guid;
		mem_block m_data;
		window_ptr m_ptr;
	};

/**
 * Subclass of ui_extension, specifically for splitters.
 * \todo elaborate...
 */
class NOVTABLE splitter_window : public window
{
public:
	/**
	 * splitter_ui_extension class GUID
	 *
	 * Used by service mechanism.
	 */
	static const GUID class_guid;

	/**
	 * Get splitter_ui_extension class GUID
	 * 
	 * Used by service mechanism.
	 *
	 * \return class GUID
	 */
	static inline const GUID & get_class_guid(){return class_guid;}

	static const GUID bool_show_caption;
	static const GUID bool_hidden;
	static const GUID bool_autohide;
	static const GUID bool_locked;
	static const GUID uint32_orientation;

	/**
	 * Get config item supported
	 *
	 * \return count
	 */
	virtual bool get_config_item_supported(unsigned p_index, const GUID & p_type) const
	{
		return false;
	}

	virtual bool get_config_item(unsigned p_index, const GUID & p_type, void * p_out, unsigned p_size)
	{
		return false;
	};

	template <class T> bool get_config_item(unsigned p_index, const GUID & p_type, T & p_out)
	{
		return get_config_item(p_index, p_type, &p_out, sizeof(T));
	}

	virtual bool have_config_popup(unsigned index)
	{
		return false;
	}

	virtual bool show_config_popup(HWND wnd_parent, unsigned index)
	{
		return false;
	}

	/**
	 * Recursively search your windows
	 * e.g.
	 * service_ptr_t<uie::playlist_window> p_playlist_wnd;
	 * service_ptr_t<uie::splitter_window> p_splitter_wnd;
	 * for(n;n<count;n++)
	 *  if (m_children[n]->m_wnd->service_query_t(p_playlist_wnd))
	 *  { p_playlist_wnd->set_focus(); return true; }
	 *  else if (m_children[n]->m_wnd->service_query_t(p_splitter_wnd) && p_splitter_wnd->focus_playlist_window())
	 *   return true;
	 *
	 * \return if window focused
	 */
	//virtual bool focus_playlist_window()const=0;

	/**
	 * Get children count
	 *
	 * \return count
	 */
	//virtual unsigned int get_children_count() const =0; 

	/**
	 * Get max children count
	 *
	 * \return count
	 */
	//virtual unsigned int get_max_children_count() const
	//{
	//	return infinite;
	//}

	/**
	 * Get children data
	 * Implementers: They are refcounted, but you should not keep any references to them after the function returns.
	 *
	 * \param[in] p_out extension_data for you to fill
	 */
	//virtual void get_children(list_base_t<extension_data_ptr> & p_out)const=0; 

	/**
	 * Get children data
	 *
	 * \param[in] p_in extension_data to replace your current children
	 */
	//virtual void set_children(const list_base_const_t<extension_data_ptr> & p_in)=0; 

	virtual void insert_panel(unsigned index, const extension_data & p_item)=0;
	virtual void remove_panel(unsigned index)=0;
	virtual void replace_panel(unsigned index, const extension_data & p_item)=0;
	virtual unsigned get_panel_count()const=0;
	virtual unsigned get_maximum_panel_count()const{return infinite;};
	virtual void get_panel(unsigned index, extension_data & p_out)const=0;

	unsigned add_panel(const extension_data & p_item){unsigned count = get_panel_count(); insert_panel(count, p_item); return count;}

	//helpers
	inline void swap_items(unsigned p_item1, unsigned p_item2)
	{
		extension_data_impl p1, p2;
		get_panel(p_item1, p1);
		get_panel(p_item2, p2);
		replace_panel(p_item1, p2);
		replace_panel(p_item2, p1);
	}

	inline bool move_up(unsigned p_index)
	{
		unsigned count = get_panel_count();
		if (p_index > 0 && p_index< count)
		{
			swap_items(p_index, p_index-1);
			return true;
		}
		return false;
	}
	inline bool move_down(unsigned p_index)
	{
		unsigned count = get_panel_count();
		if (p_index >= 0 && p_index < (count-1))
		{
			swap_items(p_index, p_index+1);
			return true;
		}
		return false;
	}

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return window::service_query(guid);
	}
};

class menu_node_configure : public ui_extension::menu_node_leaf
{
	window_ptr p_this;
public:
	virtual bool get_display_data(string_base & p_out,unsigned & p_displayflags)
	{
		p_out = "Configure...";
		p_displayflags= 0;
		return true;
	}
	virtual bool get_description(string_base & p_out)
	{
		return false;
	}
	virtual void execute()
	{
		p_this->show_config_popup(p_this->get_wnd());
	}
	menu_node_configure(window * wnd) : p_this(wnd) {};
};

template<class T>
class window_factory;

template<class T>
class window_factory_single;

template<class T>
class window_factory_transparent_single;

}

template<class T, bool B>
class window_implementation : public T
{
	virtual const bool get_is_single_instance() const{return B;}
protected:
	window_implementation() {};

	friend ui_extension::window_factory<T>;
	friend ui_extension::window_factory_single<T>;
	friend ui_extension::window_factory_transparent_single<T>;
};

namespace ui_extension{

/**
 * Service factory for multiple instance UI extensions.
 * \par Usage example
 * \code
 * static ui_extension_factory< my_ui_extension > foo_extension;
 * \endcode
 */
template<class T>
class window_factory : public service_factory_base
{
public:
	window_factory() : service_factory_base(window::class_guid)
	{
	}

	~window_factory()
	{
	}

	virtual bool instance_create(service_ptr_t<service_base> & p_out)
	{
		service_impl_t<::window_implementation<T, false> > * item = new service_impl_t<::window_implementation<T, false> >;
		if (item == 0) return false;
		p_out = (service_base*)(window*)(::window_implementation<T, false>*)item;
		return true;
	}
};

/**
 * Service factory for single instance UI extensions.
 * \par Usage example
 * \code
 * static window_factory_single< my_window > foo_extension2;
 * \endcode
 * The static instance of <code>my_window</code> can be accessed
 * as <code>foo_extension2.get_static_instance()</code>.
 */
template<class T>
class window_factory_single : public service_factory_base
{
	service_impl_single_t<window_implementation<T, true> > g_instance;
public:
	window_factory_single() : service_factory_base(window::class_guid) {}

	~window_factory_single() {}

	virtual bool instance_create(service_ptr_t<service_base> & p_out)
	{
		p_out = (service_base*)(window*)(window_implementation<T, true>*)&g_instance;
		return true;
	}

	inline T& get_static_instance() const {return (T&)g_instance;}
};

/**
 * Service factory for single instance UI extensions.
 * \par Usage example
 * \code
 * static window_factory_single_transparent< my_window > foo_extension3;
 * \endcode
 * The static instance of <code>my_window</code> can be accessed
 * as <code>foo_extension3</code>.
 */
template<class T>
class window_factory_transparent_single : public service_factory_base, public service_impl_single_t<window_implementation<T, true> >
{	
public:
	window_factory_transparent_single() : service_factory_base(window::class_guid) {}

	virtual bool instance_create(service_ptr_t<service_base> & p_out)
	{
		p_out = (service_base*)(window*)(window_implementation<T, true>*)this;
		return this;
	}

	inline T& get_static_instance() const {return *(T*)this;}
};


/**
* Helper class to hold information about ui_extension services
*/
class window_info_simple
{
public:
	GUID guid;
	string8 name;
	string8 category;
	bool prefer_multiple_instances;
};

/**
* Helper class to hold information about many ui_extension services
*/
class window_info_list_simple: public ptr_list_autodel_t<window_info_simple>
{
public:
/**
* Sort callback class for sorting
*/
	template <class T>
	class window_list_sort_callback : public sort_callback
	{
		/**
		* ui_extension_info_simple comparison function
		*/
		int compare(const T &n1,const T &n2)
		{
			int rv = 0;
			rv =  uStringCompare(n1->category,n2->category);
			if (!rv) rv = uStringCompare(n1->name,n2->name);
			return rv;
		}
	};
	/**
	* Helper function to get the name of a ui_extension by its GUID.
	* \pre You must populate the list first
	*/
	void get_name_by_guid (const GUID & in, string_base & out);
	/**
	* Helper function to sort the extensions, first by category, then by name.
	*/
	inline void sort()
	{
		window_list_sort_callback<window_info_simple *> t_sort_callback;
		ptr_list_autodel_t<window_info_simple>::sort(t_sort_callback);
	}
};

}
#endif
#endif
