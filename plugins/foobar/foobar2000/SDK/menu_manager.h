#ifndef _FOOBAR2000_MENU_MANAGER_H_
#define _FOOBAR2000_MENU_MANAGER_H_

#include "menu_item.h"
#include "metadb_handle.h"



class NOVTABLE keyboard_shortcut_manager : public service_base
{
public:
	static bool g_get(service_ptr_t<keyboard_shortcut_manager> & p_out) {return service_enum_create_t(p_out,0);}

	enum shortcut_type
	{
		TYPE_MAIN,
		TYPE_CONTEXT,
		TYPE_CONTEXT_PLAYLIST,
		TYPE_CONTEXT_NOW_PLAYING,
	};


	virtual bool process_keydown(shortcut_type type,const list_base_const_t<metadb_handle_ptr> & data,unsigned keycode)=0;
	virtual bool process_keydown_ex(shortcut_type type,const list_base_const_t<metadb_handle_ptr> & data,unsigned keycode,const GUID & caller)=0;
//caller guid - see menu_item_v2, menu_item::caller_*
	bool on_keydown(shortcut_type type,WPARAM wp);
	bool on_keydown_context(const list_base_const_t<metadb_handle_ptr> & data,WPARAM wp,const GUID & caller);
	bool on_keydown_auto(WPARAM wp);
	bool on_keydown_auto_playlist(WPARAM wp);
	bool on_keydown_auto_context(const list_base_const_t<metadb_handle_ptr> & data,WPARAM wp,const GUID & caller);

	virtual bool get_key_description_for_action(const GUID & p_command,const GUID & p_subcommand, string_base & out, shortcut_type type, bool is_global)=0;

	
	
	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}
protected:
	keyboard_shortcut_manager() {}
	~keyboard_shortcut_manager() {}
/*
usage:

  in a windowproc:

	case WM_KEYDOWN:
		keyboard_shortcut_manager::get()->on_keydown(wparam);
		break;
	case WM_SYSKEYDOWN:
		keyboard_shortcut_manager::get()->on_keydown(wparam);
		break;

  return value is true if key got translated to one of user-configured actions, false if not
  */
};




class NOVTABLE menu_node
{
public:
	virtual menu_item_node::t_type get_type()=0;
	virtual const char * get_name()=0;
	virtual unsigned get_num_children()=0;//TYPE_POPUP only
	virtual menu_node * get_child(unsigned n)=0;//TYPE_POPUP only
	virtual unsigned get_display_flags()=0;//TYPE_COMMAND/TYPE_POPUP only, see menu_item::FLAG_*
	virtual unsigned get_id()=0;//TYPE_COMMAND only, returns zero-based index (helpful for win32 menu command ids)
	virtual void execute()=0;//TYPE_COMMAND only
	virtual bool get_description(string_base & out)=0;//TYPE_COMMAND only
	virtual bool get_full_name(string_base & out)=0;//TYPE_COMMAND only
	virtual void * get_glyph()=0;//RESERVED, do not use
};



class NOVTABLE menu_manager : public service_base
{
public:
	enum
	{
		FLAG_SHOW_SHORTCUTS = 1,
		FLAG_SHOW_SHORTCUTS_GLOBAL = 2,
	};
	virtual void init_context(const list_base_const_t<metadb_handle_ptr> & data,unsigned flags)=0;//flags - see FLAG_* above
	virtual void init_context_playlist(unsigned flags)=0;
	virtual void init_main(const char * path,unsigned flags)=0;
	virtual menu_node * get_root()=0;//releasing menu_manager service releaases nodes; root may be null in case of error or something
	virtual menu_node * find_by_id(unsigned id)=0;
	virtual void set_shortcut_preference(const keyboard_shortcut_manager::shortcut_type * data,unsigned count)=0;
	


	static bool g_create(service_ptr_t<menu_manager> & p_out) {return service_enum_create_t(p_out,0);}

#ifdef WIN32
	static void win32_build_menu(HMENU menu,menu_node * parent,int base_id,int max_id);//menu item identifiers are base_id<=N<base_id+max_id (if theres too many items, they will be clipped)
	static void win32_run_menu_context(HWND parent,const list_base_const_t<metadb_handle_ptr> & data, const POINT * pt = 0,unsigned flags = 0);
	static void win32_run_menu_context_playlist(HWND parent,const POINT * pt = 0,unsigned flags = 0);
	static void win32_run_menu_main(HWND parent,const char * path,const POINT * pt = 0,unsigned flags = 0);
	void win32_run_menu_popup(HWND parent,const POINT * pt = 0);
	void win32_build_menu(HMENU menu,int base_id,int max_id) {win32_build_menu(menu,get_root(),base_id,max_id);}
	static void win32_auto_mnemonics(HMENU menu);
	
#endif

	//new (0.8 beta3)
	virtual void init_context_ex(const list_base_const_t<metadb_handle_ptr> & data,unsigned flags,const GUID & caller)=0;
	virtual bool init_context_now_playing(unsigned flags)=0;//returns false if not playing
	virtual void init_main_ex(const char * path,unsigned flags,const GUID & caller)=0;

	bool execute_by_id(unsigned id);


	static const GUID class_guid;
	
	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	menu_manager() {}
	~menu_manager() {}
};

class NOVTABLE menu_manager_defaults_callback
{
public:
	virtual void set_path(const char * path)=0;
	virtual void add_command(const GUID & p_guid)=0;//full name/path of a command, according to menu_item::enum_item, case-sensitive!
	virtual void add_separator()=0;
	virtual bool is_command_present(const GUID & p_guid)=0;
};

class NOVTABLE menu_manager_defaults : public service_base
{
public:
	virtual GUID get_guid()=0;
	virtual menu_item::type get_type()=0;
	virtual void run(menu_manager_defaults_callback * callback)=0;

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}
};




#endif //_FOOBAR2000_MENU_MANAGER_H_