#ifndef _OSD_CONFIG_INTERFACE_H_
#define _OSD_CONFIG_INTERFACE_H_

#include <foobar2000.h>
#include "../helpers/window_placement_helper.h"

extern cfg_int cfg_enable;
extern cfg_window_placement cfg_placement;

struct CUSTOMCOLORS
{
	DWORD colors[16];
};

extern cfg_struct_t<CUSTOMCOLORS> cfg_customcolors;

#endif
