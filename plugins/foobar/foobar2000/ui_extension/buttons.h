#ifndef _UI_EXTENSION_BUTTONS_H_
#define _UI_EXTENSION_BUTTONS_H_

namespace ui_extension
{

enum t_mask {
	MASK_NONE,
	MASK_BITMAP,
	MASK_COLOR,
};

enum t_button_guid {
	BUTTON_GUID_BUTTON,
	BUTTON_GUID_MENU_ITEM_CONTEXT,
	BUTTON_GUID_MENU_ITEM_MAIN,
};

enum t_button_type {
	BUTTON_TYPE_NORMAL,
	BUTTON_TYPE_DROPDOWN,
	BUTTON_TYPE_DROPDOWN_ARROW
};

enum t_button_state
{
	BUTTON_STATE_ENABLED		= (1 << 0),
	BUTTON_STATE_PRESSED		= (1 << 1),
	BUTTON_STATE_SHOW_TOOLTIP	= (1 << 2),
	BUTTON_STATE_DEFAULT		= BUTTON_STATE_ENABLED|BUTTON_STATE_SHOW_TOOLTIP,
};

class NOVTABLE button_callback
{
public:
	virtual void on_state_change(unsigned p_new_state)=0; //see t_button_state
};

class NOVTABLE menu_item_bitmap : public service_base
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
	static inline const GUID & get_class_guid() {return class_guid;}

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}

	/**
	 * Get the guid of button. If get_type_guid() == BUTTON_GUID_MENU_ITEM, this specifies a menu_item guid.
	 *
	 * \return GUID
	 *
	 * \see menu_item
	 */
	virtual const GUID & get_item_guid() const =0; 

	/**
	 * Get whether get_item_guid() specifies a menu item guid or a button guid.
	 *
	 * \Note Only recommended use of button-only buttons are dropdown-only buttons
	 *
	 * \return type of button
	 *
	 * \see t_button_guid
	 */
	virtual t_button_guid get_guid_type() const
	{
		return BUTTON_GUID_MENU_ITEM_MAIN;
	}

	/**
	 * Execute action.
	 *
	 * \note Only override if get_guid_type() == BUTTON_GUID_BUTTON.
	 *
	 * \see get_guid_type
	 */
	virtual void button_execute(const list_base_const_t<metadb_handle_ptr> & p_items)
	{
	}

	/**
	 * Get name.
	 *
	 * \note Only override if get_guid_type() == BUTTON_GUID_BUTTON.
	 *
	 * \see get_guid_type
	 */
	virtual void button_get_name(string_base & p_out)
	{
	}

	/**
	 * Get description.
	 *
	 * \return whether you set p_out to your description. 
	 *
	 * \note Only override if get_guid_type() == BUTTON_GUID_BUTTON.
	 *
	 * \see get_guid_type
	 */
	virtual bool button_get_description(string_base & p_out)
	{
		return false;
	}

	/**
	 * Get a handle to a bitmap and its transparency mask of the menu item.
	 *
	 * Caller presumes ownership of bitmap.
	 *
	 * \param[in] cr_btntext color to use for text/foreground
	 * \param[out] bm_mask HBITMAP of transparency mask. This is a monochrome bitmap.
	 *
	 * \note In the toolbar control, transparency masks are supported on all versions of windows;
	 * where as 32 bpp bitmaps with 8bpp alpha channel are supported only under
	 * common controls version 6.
	 * 
	 * \note Ensure you do not create a mask bitmap if you fail to create main bitmap
	 *
	 * \return HBITMAP of menu item
	 */
	virtual HBITMAP get_item_bitmap(COLORREF cr_btntext, t_mask & p_mask_type, COLORREF & cr_mask, HBITMAP & bm_mask) const = 0;

	virtual t_button_type get_button_type()
	{
		return BUTTON_TYPE_NORMAL;
	}
	virtual void get_menu_items(menu_hook & p_out)
	{
	}
	virtual unsigned get_item_state() //see t_button_state
	{
		return BUTTON_STATE_DEFAULT;
	}
	virtual void register_callback(button_callback & p_callback)
	{
	};
	virtual void unregister_callback(button_callback & p_callback)
	{
	};

	static bool g_button_get_name(const GUID & p_guid, string_base & p_out)
	{
		service_enum_t<ui_extension::menu_item_bitmap> e;
		pfc::refcounted_ptr_t<ui_extension::menu_item_bitmap> ptr;
		while(e.next(ptr))
		{
			if (ptr->get_item_guid() == p_guid)
			{
				ptr->button_get_name(p_out);
				return true;
			}
		}
		return false;
	}
};

template<class T>
class menu_item_bitmap_factory : public service_factory_t<menu_item_bitmap,T> {};

};

#endif