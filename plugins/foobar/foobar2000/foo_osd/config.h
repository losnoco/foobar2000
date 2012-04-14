#ifndef _OSD_CONFIG_H_
#define _OSD_CONFIG_H_

#include <foobar2000.h>

extern const char default_format[];
extern const char default_format_next[];

enum t_osd_flags
{
	osd_pop            = 1<<0,
	osd_play           = 1<<1,
	osd_pause          = 1<<2,
	osd_seek           = 1<<3,
	osd_switch         = 1<<4,
	osd_dynamic        = 1<<5,
	osd_dynamic_all    = 1<<6,
	osd_volume         = 1<<7,
	osd_interval       = 1<<8,
	osd_permanent      = 1<<9,
	osd_outline        = 1<<10,
	osd_alpha          = 1<<11,
	osd_antialias      = 1<<12,
	osd_fadeinout      = 1<<13,
	osd_dissolve       = 1<<14,
	osd_hide_on_stop   = 1<<15,
	osd_hide_until_idle= 1<<16,
	osd_show_until_idle= 1<<17
};

struct osd_config
{
	pfc::string_simple  name;
	unsigned            flags;
	t_font_description  font;
	unsigned            displaytime;
	unsigned            x, y, pos, align;
	unsigned            vwidth, vheight, vsteps;
	int                 vmin;
	pfc::string_simple  format;//, formatnext;
	unsigned            color, bgcolor;
	unsigned            alphalev, alphaback;
	unsigned            fadetime;
	unsigned            dissolve_decay;

	// v2
	unsigned            idletime;

	// v3
	unsigned			display_number;

	osd_config();

	osd_config(const osd_config * in) { *this = *in; }
	osd_config(const osd_config & in) { *this = in; }
	const osd_config * operator =(const osd_config * in) { *this = *in; return this; }
	const osd_config & operator =(const osd_config & in);
};

class cfg_osd_list;

class osd_state
{
	friend class cfg_osd_list;

	osd_config                      & conf;

protected:
	service_ptr_t<titleformat_object> format;//, formatnext;

public:
	osd_state(osd_config & _conf);

	void on_change();

	const osd_config & get() const
	{
		return conf;
	}
};

#endif
