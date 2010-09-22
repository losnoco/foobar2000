#define MY_VERSION "1.9"

/*
	change log

2010-09-22 03:44 UTC - kode54
- Added detection for redundant format inputs to disable support for formats
  better supported by vgmstream
- Version is now 1.9

2010-08-10 20:24 UTC - kode54
- Added more decryption keys to ADX input
- Fixed EA MUS failing on wrong file type so other MUS inputs can handle it
- Version is now 1.8

2010-04-26 01:36 UTC - kode54
- Moved loop configuration setting to advanced preferences
- DSP/ADP input now observes loop configuration setting
- Version is now 1.7

2010-04-16 10:14 UTC - kode54
- Removed component version spam
- Version is now 1.6

2010-04-13 14:52 UTC - kode54
- Amended preferences WM_INITDIALOG handler
- Version is now 1.5

2010-01-11 05:13 UTC - kode54
- Updated preferences page to 1.0 API
- Version is now 1.4

2009-08-01 05:29 UTC - kode54
- Minor fixes to the DSP decoder
- Version is now 1.3

2005-05-22 11:14 UTC - kode54
- Fixed stupid bug in ACM init function
- Version is now 1.2

2004-10-08 14:57 UTC - kode54
- Wow, my first combined plug-in
- Hopefully, this file's component_version service stays at the top of the service list, for foover
- Version is now 1.0

*/

#define _WIN32_WINNT 0x0501

#include <foobar2000.h>

#include "../ATLHelpers/ATLHelpers.h"

#include "resource.h"

// {CCB20BBE-8C08-4337-B4EE-77778AF499E3}
static const GUID guid_cfg_loop = 
{ 0xccb20bbe, 0x8c08, 0x4337, { 0xb4, 0xee, 0x77, 0x77, 0x8a, 0xf4, 0x99, 0xe3 } };

advconfig_checkbox_factory cfg_loop("ADPCM - Loop indefinitely", guid_cfg_loop, advconfig_branch::guid_branch_playback, 0, false);

bool cfg_enable_overlapping = true;

class adpcm_detect_redundant_decoders : public initquit
{
public:
	virtual void on_init()
	{
		unsigned service_count = 0;

		service_enum_t<input_entry> e;
		service_ptr_t<input_entry> f;

		while ( e.next( f ) )
		{
			if ( f->is_our_path( "foo.brstm", "brstm" ) ) service_count++;
		}

		if ( service_count > 1 ) cfg_enable_overlapping = false;
	}

	virtual void on_quit() { }
};

static initquit_factory_t< adpcm_detect_redundant_decoders > g_initquit_adpcm_detector;

DECLARE_COMPONENT_VERSION("kode's ADPCM decoders", MY_VERSION, "Contains ACM, ADX, BRR, EA MUS, Game Cube DSP/ADP, OKI ADPCM, RAC, and XA decoders.");

VALIDATE_COMPONENT_FILENAME("foo_adpcm.dll");
