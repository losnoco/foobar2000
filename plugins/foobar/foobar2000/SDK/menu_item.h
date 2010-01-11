#ifndef _FOOBAR2000_MENU_ITEM_H_
#define _FOOBAR2000_MENU_ITEM_H_

typedef void * t_glyph;


class NOVTABLE menu_item_node
{
public:
	enum t_flags
	{
		FLAG_CHECKED = 1,
		FLAG_DISABLED = 2,
		FLAG_GRAYED = 4,
		FLAG_DISABLED_GRAYED = FLAG_DISABLED|FLAG_GRAYED,
	};

	enum t_type
	{
		TYPE_POPUP,TYPE_COMMAND,TYPE_SEPARATOR
	};

	virtual bool get_display_data(string_base & p_out,unsigned & p_displayflags,const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller) = 0;
	virtual t_type get_type() = 0;
	virtual void execute(const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller) = 0;
	virtual t_glyph get_glyph(const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller) {return 0;}//RESERVED
	virtual unsigned get_children_count() = 0;
	virtual menu_item_node * get_child(unsigned p_index) = 0;
	virtual bool get_description(string_base & p_out) = 0;
	virtual GUID get_guid() = 0;
	virtual bool is_mappable_shortcut() = 0;

protected:
	inline menu_item_node() {}
	inline ~menu_item_node() {}
};

class NOVTABLE menu_item_node_root : public menu_item_node
{
public:
	virtual ~menu_item_node_root() {}
};

class NOVTABLE menu_item_node_leaf : public menu_item_node
{
public:
	t_type get_type() {return TYPE_COMMAND;}
	unsigned get_children_count() {return 0;}
	menu_item_node * get_child(unsigned) {return 0;}
};

class NOVTABLE menu_item_node_root_leaf : public menu_item_node_root
{
public:
	t_type get_type() {return TYPE_COMMAND;}
	unsigned get_children_count() {return 0;}
	menu_item_node * get_child(unsigned) {return 0;}
};

class NOVTABLE menu_item_node_popup : public menu_item_node
{
public:
	t_type get_type() {return TYPE_POPUP;}
	void execute(const list_base_const_t<metadb_handle_ptr> & data,const GUID & caller) {}
	bool get_description(string_base & p_out) {return false;}
};

class NOVTABLE menu_item_node_root_popup : public menu_item_node_root
{
public:
	t_type get_type() {return TYPE_POPUP;}
	void execute(const list_base_const_t<metadb_handle_ptr> & data,const GUID & caller) {}
	bool get_description(string_base & p_out) {return false;}
};

class menu_item_node_separator : public menu_item_node
{
public:
	t_type get_type() {return TYPE_SEPARATOR;}
	void execute(const list_base_const_t<metadb_handle_ptr> & data,const GUID & caller) {}
	bool get_description(string_base & p_out) {return false;}
	unsigned get_children_count() {return 0;}
	bool get_display_data(string_base & p_out,unsigned & p_displayflags,const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller) 
	{
		p_displayflags = 0;
		p_out = "---";
		return true;
	}
	menu_item_node * get_child(unsigned) {return 0;}
};

class NOVTABLE menu_item : public service_base
{
public:

	enum t_enabled_state
	{
		DEFAULT_OFF,
		DEFAULT_ON,
		FORCE_ON,
		FORCE_OFF,
	};

	enum type
	{
		TYPE_MAIN,
		TYPE_CONTEXT,
	};

	virtual type get_type()=0;
	virtual unsigned get_num_items() = 0;
	virtual menu_item_node_root * instantiate_item(unsigned p_index,const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller) = 0;
	virtual GUID get_item_guid(unsigned p_index) = 0;
	virtual void get_item_name(unsigned p_index,string_base & p_out) = 0;
	virtual void get_item_default_path(unsigned p_index,string_base & p_out) = 0;
	virtual bool get_item_description(unsigned p_index,string_base & p_out) = 0;
	virtual t_enabled_state get_enabled_state(unsigned p_index) = 0;
	virtual void item_execute_simple(unsigned p_index,const GUID & p_node,const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller) = 0;

	bool item_get_display_data_root(string_base & p_out,unsigned & displayflags,unsigned p_index,const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller);
	bool item_get_display_data(string_base & p_out,unsigned & displayflags,unsigned p_index,const GUID & p_node,const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller);
	
	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}

	static const GUID caller_now_playing;
	static const GUID caller_playlist;
	static const GUID caller_undefined;
	static const GUID caller_keyboard_shortcut_list;

};

class NOVTABLE menu_item_impl_simple : public menu_item
{
private:
	class menu_item_node_impl : public menu_item_node_root_leaf
	{
	public:
		menu_item_node_impl(menu_item_impl_simple * p_owner,unsigned p_index) : m_owner(p_owner), m_index(p_index) {}
		bool get_display_data(string_base & p_out,unsigned & p_displayflags,const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller) {return m_owner->item_get_display_data(p_out,p_displayflags,m_index,p_data,p_caller);}
		void execute(const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller) {m_owner->item_execute(m_index,p_data,p_caller);}
		bool get_description(string_base & p_out) {return m_owner->get_item_description(m_index,p_out);}
		GUID get_guid() {return pfc::guid_null;}
		bool is_mappable_shortcut() {return m_owner->item_is_mappable_shortcut(m_index);}
	private:
		service_ptr_t<menu_item_impl_simple> m_owner;
		unsigned m_index;
	};
protected:
	menu_item_node_root * instantiate_item(unsigned p_index,const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller)
	{
		return new menu_item_node_impl(this,p_index);
	}


	void item_execute_simple(unsigned p_index,const GUID & p_node,const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller)
	{
		if (p_node == pfc::guid_null)
			item_execute(p_index,p_data,p_caller);
	}

	virtual bool item_is_mappable_shortcut(unsigned p_index)
	{
		return true;
	}
public:
	enum t_flags
	{
		FLAG_CHECKED = 1,
		FLAG_DISABLED = 2,
		FLAG_GRAYED = 4,
		FLAG_DISABLED_GRAYED = FLAG_DISABLED|FLAG_GRAYED,
	};


	//override these

	virtual void item_execute(unsigned p_index,const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller) = 0;
	virtual bool item_get_display_data(string_base & p_out,unsigned & p_displayflags,unsigned p_index,const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller) = 0;

	//override these
	virtual type get_type()=0;
	virtual unsigned get_num_items() = 0;
	virtual t_enabled_state get_enabled_state(unsigned p_index)=0;
	virtual bool get_item_description(unsigned p_index,string_base & p_out) = 0;
	virtual void get_item_name(unsigned p_index,string_base & p_out) = 0;
	virtual void get_item_default_path(unsigned p_index,string_base & p_out) = 0;
	virtual GUID get_item_guid(unsigned p_index) = 0;
};

class NOVTABLE menu_item_simple : public menu_item_impl_simple
{
public:
	virtual t_enabled_state get_enabled_state(unsigned idx) {return DEFAULT_ON;}//return if menu item should be enabled by default or not

	virtual type get_type()=0;
	virtual unsigned get_num_items()=0;
	
	virtual bool get_display_data(unsigned n,const list_base_const_t<metadb_handle_ptr> & data,string_base & p_out,unsigned & displayflags,const GUID & caller) = 0;
	virtual void perform_command(unsigned n,const list_base_const_t<metadb_handle_ptr> & data,const GUID & caller) = 0;

	virtual bool get_item_description(unsigned n,string_base & p_out) = 0;
	
	virtual GUID get_item_guid(unsigned p_index) = 0;

	virtual void get_item_name(unsigned p_index,string_base & p_out) = 0;
	virtual void get_item_default_path(unsigned p_index,string_base & p_out) = 0;

protected:
	void item_execute(unsigned p_index,const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller) {perform_command(p_index,p_data,p_caller);}
	bool item_get_display_data(string_base & p_out,unsigned & p_displayflags,unsigned p_index,const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller) {return get_display_data(p_index,p_data,p_out,p_displayflags,p_caller);}

};


class NOVTABLE menu_item_simple_main : public menu_item_simple
{
	virtual type get_type() {return TYPE_MAIN;}

	virtual bool get_display_data(unsigned n,const list_base_const_t<metadb_handle_ptr> & data,string_base & p_out,unsigned & displayflags,const GUID & caller)
	{
		assert(n>=0 && n<get_num_items());
		if (!is_available(n)) return false;
		get_item_name(n,p_out);
		displayflags = (is_checked(n) ? FLAG_CHECKED : 0) | (is_disabled(n) ? (FLAG_DISABLED|FLAG_GRAYED) : 0);
		return true;
	}

	virtual void perform_command(unsigned n,const list_base_const_t<metadb_handle_ptr> & data,const GUID & caller) {perform_command(n);}
protected:
	//override these
	virtual unsigned get_num_items()=0;
	virtual void get_item_name(unsigned n,string_base & p_out)=0;
	virtual void get_item_default_path(unsigned p_index,string_base & p_out) = 0;
	virtual void perform_command(unsigned n)=0;
	virtual bool is_checked(unsigned n) {return false;}
	virtual bool is_disabled(unsigned n) {return false;}
	virtual bool is_available(unsigned n) {return true;}
	virtual bool get_item_description(unsigned n,string_base & p_out) = 0;
	virtual GUID get_item_guid(unsigned p_index) = 0;
};

class NOVTABLE menu_item_simple_main_single : public menu_item_simple_main //helper
{
	virtual unsigned get_num_items() {return 1;}
	virtual void get_item_name(unsigned p_index,string_base & p_out) {assert(p_index == 0); get_name(p_out); }
	virtual void get_item_default_path(unsigned p_index,string_base & p_out) {assert(p_index == 0); get_default_path(p_out); }
	virtual void perform_command(unsigned p_index) {assert(p_index==0); run(); }
	virtual bool is_checked(unsigned n) {assert(n==0); return is_checked();};
	virtual bool is_disabled(unsigned n) {assert(n==0); return is_disabled();}
	virtual bool get_item_description(unsigned n,string_base & p_out) {assert(n==0); return get_description(p_out);}
	virtual GUID get_item_guid(unsigned p_index) {assert(p_index==0); return get_guid();}
protected://override these
	virtual void get_name(string_base & p_out) = 0;
	virtual void get_default_path(string_base & p_out) = 0;
	virtual void run()=0;
	virtual bool is_checked() {return false;}
	virtual bool is_disabled() {return false;}
	virtual bool get_description(string_base & p_out) = 0;
	virtual GUID get_guid() = 0;
};

class NOVTABLE menu_item_simple_context : public menu_item_simple
{
	virtual type get_type() {return TYPE_CONTEXT;}
	virtual bool get_display_data(unsigned n,const list_base_const_t<metadb_handle_ptr> & data,string_base & p_out,unsigned & displayflags,const GUID & caller)
	{
		bool rv = false;
		assert(n>=0 && n<get_num_items());
		if (data.get_count()>0)
		{
			rv = context_get_display(n,data,p_out,displayflags,caller);
		}
		return rv;
	}
	virtual void perform_command(unsigned n,const list_base_const_t<metadb_handle_ptr> & data,const GUID & caller)
	{
		if (data.get_count()>0) context_command(n,data,caller);
	}
	virtual bool is_checked(unsigned n,const list_base_const_t<metadb_handle_ptr> & data) {return false;}
protected:
	//override these
	virtual unsigned get_num_items()=0;
	virtual void get_item_name(unsigned n,string_base & p_out)=0;
	virtual void get_item_default_path(unsigned p_index,string_base & p_out) = 0;
	virtual void context_command(unsigned n,const list_base_const_t<metadb_handle_ptr> & data,const GUID& caller)=0;
	virtual bool context_get_display(unsigned n,const list_base_const_t<metadb_handle_ptr> & data,string_base & p_out,unsigned & displayflags,const GUID &)
	{
		assert(n>=0 && n<get_num_items());
		get_item_name(n,p_out);
		return true;
	}
	virtual GUID get_item_guid(unsigned p_index) = 0;
	virtual bool get_item_description(unsigned p_index,string_base & p_out) = 0;
};


template<class T>
class menu_item_factory_t : public service_factory_single_t<menu_item,T> {};


typedef menu_item_simple menu_item_legacy;
typedef menu_item_simple_main menu_item_legacy_main;
typedef menu_item_simple_main_single menu_item_legacy_main_single;
typedef menu_item_simple_context menu_item_legacy_context;

#endif //_FOOBAR2000_MENU_ITEM_H_