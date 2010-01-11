#include <foobar2000.h>
#include "main.h"
#include "config_interface.h"

class menu_item_osd_enable : public menu_item_node
{
public:
	bool get_display_data( pfc::string_base & p_out,unsigned & p_displayflags,const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller)
	{
		p_displayflags = cfg_enable ? FLAG_CHECKED : 0;
		p_out = "Enable";
		return true;
	}

	t_type get_type() {return TYPE_COMMAND;}

	void execute(const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller)
	{
		if (cfg_enable)
		{
			g_osd.quit();
			cfg_enable = 0;
		}
		else
		{
			cfg_enable = g_osd.init();
		}
	}

	unsigned get_children_count() {return 0;}

	menu_item_node * get_child(unsigned p_index) {return 0;}

	bool get_description(pfc::string_base & p_out)
	{
		if (cfg_enable) p_out = "Disable";
		else p_out = "Enable";
		p_out += "s the On-Screen Display system";
		return true;
	}

	GUID get_guid()
	{
		return g_guid;
	}

	bool is_mappable_shortcut() {return true;}

	static const GUID g_guid;
};

const GUID menu_item_osd_enable::g_guid =
{ 0xccf56f, 0xeb00, 0x4ffe, { 0xa6, 0xb4, 0xff, 0x8c, 0x30, 0x4f, 0x1f, 0x13 } };

static GUID guid_from_string(const char * p_string)
{
	return static_api_ptr_t<hasher_md5>()->process_single_guid(p_string,strlen(p_string));
}

// mmkay!
static GUID guid_from_guid(const GUID & p_guid)
{
	return static_api_ptr_t<hasher_md5>()->process_single_guid(&p_guid,sizeof(p_guid));
}

class menu_item_osd_show_track : public menu_item_node
{
public:
	void set_data(unsigned p_index, const GUID & p_guid)
	{
		m_index = p_index;
		m_guid = p_guid;
	}

	bool get_display_data(pfc::string_base & p_out,unsigned & p_displayflags,const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller)
	{
		p_displayflags = 0;
		p_out = "Show current track";
		return true;
	}

	t_type get_type() {return TYPE_COMMAND;}

	void execute(const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller)
	{
		g_osd.show_track(m_index);
	}

	unsigned get_children_count() {return 0;}

	menu_item_node * get_child(unsigned p_index) {return 0;}

	bool get_description(pfc::string_base & p_out)
	{
		p_out = "Displays the current track";
		return true;
	}

	GUID get_guid() {return m_guid;}

	bool is_mappable_shortcut() {return true;}
private:
	unsigned m_index;
	GUID m_guid;
};

class menu_item_osd_show_playlist : public menu_item_node
{
public:
	void set_data(unsigned p_index, const GUID & p_guid)
	{
		m_index = p_index;
		m_guid = p_guid;
	}

	bool get_display_data(pfc::string_base & p_out,unsigned & p_displayflags,const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller)
	{
		p_displayflags = 0;
		p_out = "Show active playlist";
		return true;
	}

	t_type get_type() {return TYPE_COMMAND;}

	void execute(const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller)
	{
		g_osd.show_playlist(m_index);
	}

	unsigned get_children_count() {return 0;}

	menu_item_node * get_child(unsigned p_index) {return 0;}

	bool get_description(pfc::string_base & p_out)
	{
		p_out = "Displays the active playlist";
		return true;
	}

	GUID get_guid() {return m_guid;}

	bool is_mappable_shortcut() {return true;}
private:
	unsigned m_index;
	GUID m_guid;
};

class menu_item_osd_show_volume : public menu_item_node
{
public:
	void set_data(unsigned p_index, const GUID & p_guid)
	{
		m_index = p_index;
		m_guid = p_guid;
	}

	bool get_display_data(pfc::string_base & p_out,unsigned & p_displayflags,const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller)
	{
		p_displayflags = 0;
		p_out = "Show volume level";
		return true;
	}

	t_type get_type() {return TYPE_COMMAND;}

	void execute(const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller)
	{
		g_osd.show_volume(m_index);
	}

	unsigned get_children_count() {return 0;}

	menu_item_node * get_child(unsigned p_index) {return 0;}

	bool get_description(pfc::string_base & p_out)
	{
		p_out = "Displays the volume level";
		return true;
	}

	GUID get_guid() {return m_guid;}

	bool is_mappable_shortcut() {return true;}
private:
	unsigned m_index;
	GUID m_guid;
};

class menu_item_osd_hide : public menu_item_node
{
public:
	void set_data(unsigned p_index, const GUID & p_guid)
	{
		m_index = p_index;
		m_guid = p_guid;
	}

	bool get_display_data(pfc::string_base & p_out,unsigned & p_displayflags,const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller)
	{
		p_displayflags = 0;
		p_out = "Hide";
		return true;
	}

	t_type get_type() {return TYPE_COMMAND;}

	void execute(const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller)
	{
		g_osd.hide(m_index);
	}

	unsigned get_children_count() {return 0;}

	menu_item_node * get_child(unsigned p_index) {return 0;}

	bool get_description(pfc::string_base & p_out)
	{
		p_out = "Hides the overlay";
		return true;
	}

	GUID get_guid() {return m_guid;}

	bool is_mappable_shortcut() {return true;}
private:
	unsigned m_index;
	GUID m_guid;
};

class menu_item_osd : public menu_item_node
{
public:
	bool is_mappable_shortcut() {return false;}

	void set_data(unsigned p_index, const char * p_info)
	{
		m_config = p_info;

		GUID p_guid = guid_from_string(m_config);
		m_c_track.set_data(p_index, p_guid);

		// whee!
		GUID x_guid = guid_from_guid(p_guid);
		m_c_playlist.set_data(p_index, x_guid);
		p_guid = guid_from_guid(x_guid);
		m_c_volume.set_data(p_index, p_guid);
		x_guid = guid_from_guid(p_guid);
		m_c_hide.set_data(p_index, x_guid);
	}

	bool get_display_data(pfc::string_base & p_out,unsigned & p_displayflags,const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller)
	{
		p_displayflags = 0;
		p_out = m_config;
		return true;
	}

	t_type get_type() {return TYPE_POPUP;}

	void execute(const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller) {}
	unsigned get_children_count() {return 4;}

	menu_item_node * get_child(unsigned p_index)
	{
		switch (p_index)
		{
		case 0: return &m_c_track;
		case 1: return &m_c_playlist;
		case 2: return &m_c_volume;
		case 3: return &m_c_hide;
		}

		return 0;
	}

	bool get_description(pfc::string_base & p_out)
	{
		p_out = "Controls an OSD preset : \"";
		p_out += m_config;
		p_out += "\"";
		return true;
	}
	GUID get_guid() {return pfc::guid_null;}
private:
	const char * m_config;

	menu_item_osd_show_track    m_c_track;
	menu_item_osd_show_playlist m_c_playlist;
	menu_item_osd_show_volume   m_c_volume;
	menu_item_osd_hide          m_c_hide;
};

class menu_item_osd_root : public menu_item_node_root
{
public:
	bool is_mappable_shortcut() {return false;}

	menu_item_osd_root()
	{
		g_osd.get_names(m_config);

		unsigned n, m = m_config.get_size();
		m_children.set_size(m);
		for(n=0;n<m;n++)
		{
			m_children[n].set_data(n, m_config[n]);
		}
	}

	bool get_display_data(pfc::string_base & p_out,unsigned & p_displayflags,const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller)
	{
		if (m_children.get_size() == 0) return false;
		p_displayflags = 0;
		p_out = "On-Screen Display";
		return true;
	}

	t_type get_type()
	{
		return TYPE_POPUP;
	}
	void execute(const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller) {}
	unsigned get_children_count()
	{
		return m_children.get_size() + 1;
	}
	menu_item_node * get_child(unsigned p_index)
	{
		if (p_index) return &m_children[p_index-1];
		else return &m_c_enable;
	}
	bool get_description(pfc::string_base & p_out)
	{
		p_out = "Controls the On-Screen Display.";
		return true;
	}
	GUID get_guid() {return pfc::guid_null;}
private:
	pfc::array_t<string_simple> m_config;
	pfc::array_t<menu_item_osd> m_children;
	menu_item_osd_enable   m_c_enable;
};

class menu_osd_control : public menu_item
{
public:
	type get_type() {return TYPE_MAIN;}
	unsigned get_num_items() {return 1;}
	menu_item_node_root * instantiate_item(unsigned p_index,const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller)
	{
		assert(p_index == 0);
		if (core_api::assert_main_thread())
		{
			return new menu_item_osd_root;
		}
		else return 0;
	}

	GUID get_item_guid(unsigned p_index)
	{
		assert(p_index == 0);
		// {4BF7726A-AD44-404b-9391-84A41AC1F4C2}
		static const GUID guid = 
		{ 0x4bf7726a, 0xad44, 0x404b, { 0x93, 0x91, 0x84, 0xa4, 0x1a, 0xc1, 0xf4, 0xc2 } };
		return guid;
	}

	void get_item_name(unsigned p_index,pfc::string_base & p_out)
	{
		assert(p_index == 0);
		p_out = "On-Screen Display";
	}

	void get_item_default_path(unsigned p_index,pfc::string_base & p_out)
	{
		assert(p_index == 0);
		p_out = "Components";
	}

	bool get_item_description(unsigned p_index,pfc::string_base & p_out)
	{
		return false;
	}

	t_enabled_state get_enabled_state(unsigned p_index)
	{
		return DEFAULT_ON;
	}

	void item_execute_simple(unsigned p_index,const GUID & p_node,const list_base_const_t<metadb_handle_ptr> & p_data,const GUID & p_caller)
	{
		if (core_api::assert_main_thread())
		{
			if (p_node == menu_item_osd_enable::g_guid)
			{
				if (cfg_enable)
				{
					g_osd.quit();
					cfg_enable = 0;
				}
				else
				{
					cfg_enable = g_osd.init();
				}

				return;
			}

			pfc::array_t<string_simple> m_names;
			g_osd.get_names(m_names);
			unsigned n, m = m_names.get_size();
			for(n = 0; n < m; n++)
			{
				GUID p_guid = guid_from_string(m_names[n]);
				if (p_node == p_guid) { g_osd.show_track(n); break; }

				GUID x_guid = guid_from_guid(p_guid);
				if (p_node == x_guid) { g_osd.show_playlist(n); break; }

				p_guid = guid_from_guid(x_guid);
				if (p_node == p_guid) { g_osd.show_volume(n); break; }

				x_guid = guid_from_guid(p_guid);
				if (p_node == x_guid) { g_osd.hide(n); break; }
			}
		}
	}

};

static menu_item_factory_t<menu_osd_control> g_menu_item_osd_control_factory;
