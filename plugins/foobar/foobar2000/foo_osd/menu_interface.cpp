#include <foobar2000.h>
#include "main.h"
#include "config_interface.h"

static const GUID guid_mainmenu_group_osd = { 0x623337f4, 0xe5ce, 0x4a1f, { 0xa4, 0xf8, 0x72, 0xbc, 0xff, 0x52, 0xa4, 0x29 } };

static mainmenu_group_popup_factory g_mainmenu_group_osd( guid_mainmenu_group_osd, mainmenu_groups::view, mainmenu_commands::sort_priority_dontcare, "On-Screen Display" );

class mainmenu_node_command_osd : public mainmenu_node_command
{
	t_uint32 overlay_index, overlay_command;

public:
	mainmenu_node_command_osd( t_uint32 p_index, t_uint32 p_command ) : overlay_index( p_index ), overlay_command( p_command ) { }

	virtual void get_display( pfc::string_base & text, t_uint32 & flags )
	{
		static const char * names[] = { "Show current track", "Show active playlist", "Show volume level", "Hide" };
		flags = 0;
		text = names[ overlay_command ];
	}

	virtual bool get_description( pfc::string_base & out )
	{
		static const char * descriptions[] = { "Displays the current track", "Displays the active playlist", "Displays the volume level", "Hides the overlay" };
		out = descriptions[ overlay_command ];
		return true;
	}

	virtual GUID get_guid()
	{
		static const GUID guids[4] = {
			{ 0x56b4fb47, 0xdfa, 0x4c98, { 0xa4, 0x2a, 0x6, 0x58, 0x3d, 0x51, 0x30, 0x38 } },
			{ 0xadee0de, 0xefaf, 0x40a6, { 0x83, 0xdd, 0x3b, 0xb9, 0x92, 0x25, 0x56, 0x9 } },
			{ 0xe88b5dc5, 0xa4be, 0x4778, { 0x9a, 0x3d, 0x49, 0x15, 0xc0, 0x93, 0x6c, 0x4f } },
			{ 0x1b52b71f, 0x24c5, 0x46e0, { 0x8e, 0x45, 0xb1, 0x31, 0x87, 0xa1, 0xca, 0xa8 } }
		};

		return guids[ overlay_command ];
	}

	void execute(service_ptr_t<service_base>)
	{
		switch ( overlay_command )
		{
		case 0:
			g_osd.show_track( overlay_index );
			break;

		case 1:
			g_osd.show_playlist( overlay_index );
			break;

		case 2:
			g_osd.show_volume( overlay_index );
			break;

		case 3:
			g_osd.hide( overlay_index );
			break;
		}
	}
};

class mainmenu_node_group_osd : public mainmenu_node_group
{
	t_uint32 overlay_index;

public:
	mainmenu_node_group_osd( t_uint32 p_index ) : overlay_index( p_index ) { }

	virtual void get_display( pfc::string_base & text, t_uint32 & flags )
	{
		pfc::array_t< pfc::string_simple > names;
		flags = 0;
		g_osd.get_names( names );
		text = names[ overlay_index ];
	}

	virtual t_size get_children_count()
	{
		return 4;
	}

	virtual ptr get_child( t_size index )
	{
		return new service_impl_t< mainmenu_node_command_osd > ( overlay_index, index );
	}
};

class mainmenu_commands_osd : public mainmenu_commands_v2
{
public:
	virtual t_uint32 get_command_count()
	{
		pfc::array_t<pfc::string_simple> names;
		g_osd.get_names( names );
		if ( names.get_count() ) return 2 + names.get_count();
		return 1;
	}

	virtual GUID get_command( t_uint32 p_index )
	{
		// {CEBCFC86-7AAF-472D-9047-4EFE795BAE2B}
		static const GUID guid = 
		{ 0xcebcfc86, 0x7aaf, 0x472d, { 0x90, 0x47, 0x4e, 0xfe, 0x79, 0x5b, 0xae, 0x2b } };
		if ( !p_index ) return guid;

		hasher_md5_state m_state;
		static_api_ptr_t< hasher_md5 > p_hasher;
		p_hasher->initialize( m_state );
		p_hasher->process( m_state, &guid, sizeof( guid ) );
		p_index = pfc::byteswap_if_be_t( p_index );
		p_hasher->process( m_state, &p_index, sizeof( p_index ) );

		return p_hasher->get_result_guid( m_state );
	}

	virtual void get_name( t_uint32 p_index, pfc::string_base & p_out )
	{
		if ( !p_index ) p_out = "Enable";
		else if ( p_index > 1 )
		{
			pfc::array_t< pfc::string_simple > names;
			g_osd.get_names( names );
			p_out = names[ p_index - 2 ];
		}
	}

	virtual bool get_description( t_uint32 p_index, pfc::string_base & p_out )
	{
		if ( !p_index )
		{
			p_out = "Controls the On-Screen Display.";
			return true;
		}
		return false;
	}

	virtual GUID get_parent()
	{
		return guid_mainmenu_group_osd;
	}

	virtual bool get_display( t_uint32 p_index, pfc::string_base & p_text, t_uint32 & p_flags )
	{
		p_flags = ( !p_index && cfg_enable ) ? flag_checked : 0;
		get_name( p_index, p_text );
		return true;
	}

	virtual void execute( t_uint32 p_index, service_ptr_t<service_base> p_callback )
	{
		if ( !p_index )
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
	}

	virtual bool is_command_dynamic( t_uint32 index )
	{
		return !!index;
	}

	virtual mainmenu_node::ptr dynamic_instantiate( t_uint32 index )
	{
		if ( index == 1 ) return new service_impl_t< mainmenu_node_separator >;
		else if ( index > 1 ) return new service_impl_t< mainmenu_node_group_osd > ( index - 2 );
		throw pfc::exception_invalid_params();
	}
};

static service_factory_single_t< mainmenu_commands_osd > g_mainmenu_commands_osd_factory;
