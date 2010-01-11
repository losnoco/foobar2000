#ifndef _UI_EXTENSION_H_
#define _UI_EXTENSION_H_

#define UI_EXTENSION_VERSION "5.0.0"

/**
 * \file ui_extension.h
 * User interface extension API
 * \author musicmusic (interfaces)
 * \author Holger Stenger (doxygen comments)
 * \version 5.0.0
 * \par Changes
 * - added methods for showing/hiding your window
 * - added methods to resize your window
 * - added function to retrieve whether your extension prefers multiple instances
 * - other minor changes
 */

 /**
 * Namespace for flags used by ui_extension and ui_extension_host services.
 * \see ui_extension, ui_extension_host
 */
namespace ui_extension_flag
{

 /**
 * Flags indicating the type of the UI extension.
 *
 * Combine multiple flags using bitwise or,
 * if an extension supports more than one type.
 *
 * \see ui_extension::get_type, ui_extension_host::get_supported_types
 */
enum ui_extension_type
{
	/** The extension is a sidebar panel. */
	TYPE_PANEL	= (1 << 0),
	/** The extension is a toolbar panel. */
	TYPE_TOOLBAR	= (1 << 1),
	/** The extension is a playlist panel. */
	TYPE_PLAYLIST	= (1 << 2),
	/** The extension is a layout panel. */
	TYPE_LAYOUT	= (1 << 3),
};
	
	
/**
 * Flags indicating a change of a size limit.
 *
 * Combine multiple flags using bitwise or,
 * if more than one size limit changed.
 *
 * \see ui_extension_host::on_size_limit_change
 */
enum size_limit_changed
{
	/** The minimum width changed. */
	SLC_MIN_WIDTH	= (1 << 0),
	/** The maximum width changed. */
	SLC_MAX_WIDTH	= (1 << 1),
	/** The minimum height changed. */
	SLC_MIN_HEIGHT	= (1 << 2),
	/** The maximum height changed. */
	SLC_MAX_HEIGHT	= (1 << 3),
};
	
/**
 * Flags indicating .
 *
 * Combine multiple flags using bitwise or,
 * to indicate .
 *
 * \todo finish description
 * \see ui_extension_host::on_size_limit_change
 */
enum resize_flags
{
	/** The width parameter is valid. */
	RSZ_WIDTH		= (1 <<0),
	/** The height parameter is valid. */
	RSZ_HEIGHT	= (1 << 1),
};

};

#include "../SDK/foobar2000.h"
#include "host.h"
#include "extension.h"

/**
 * Helper class to hold information about ui_extension services
 */
class ui_extension_info_simple
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
class ui_extension_info_list_simple: public ptr_list_autodel_t<ui_extension_info_simple>
{
public:
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
		ptr_list_t<ui_extension_info_simple>::sort(sortproc());
	}

	class sortproc : public list_base_t<ui_extension_info_simple>::sort_callback
	{
	public:
		virtual int compare(const ui_extension_info_simple * &n1,const ui_extension_info_simple * &n2);
	};
};


#endif