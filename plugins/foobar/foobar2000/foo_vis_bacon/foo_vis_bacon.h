//Includes.  Be sure to set your project settings to include
//the SDK directory in the foobar SDK or you'll get errors
//about foobar2000.h not existing.

#ifndef _FOO_VIS_BACON_H_
#define _FOO_VIS_BACON_H_


#include <windows.h>
#include <process.h>
#include "foobar2000.h"
#include "deque.h"
#include "foo_osd.h"
#include "dll_manager.h"
#include "WVIS.H"
#include "WA_IPC.H"
#include "resource.h"
#include "hartwork_timer.h"

bool get_winamp_path(string8 & out);

class get_default_path : public string8_fastalloc
{
public:
	get_default_path()
	{
		if (get_winamp_path(*this))
		{
			add_string("\\Plugins\\");
		}
		else
		{
			uGetModuleFileName(NULL, *this);
			truncate(find_last('\\'));
			add_string("\\bacon\\");
		}
	}
};


#endif // _FOO_VIS_BACON_H_