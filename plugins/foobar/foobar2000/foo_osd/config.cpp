#include "config.h"

const char default_format[] = "$rgb(0,255,0,0,0,0)[%artist% - ]$if(%title%,['['%album%[ #[%disc%/]$num(%tracknumber%,2)]'] ']%title%,%_filename%)[ '['%_time_elapsed%']']$if(%_ispaused%,' [paused]')\r\n[$char(10)$rgb(35,169,207,16,43,75)Next: %_next%]";
const char default_format_next[] = "[%artist% - ]$if2(%title%,%_filename%)";

inline static t_font_description get_def_font()
{
	t_font_description foo;
	foo.m_height = 120; // 18 points
	foo.m_weight = FW_BOLD;
	foo.m_italic = FALSE;
	foo.m_charset = DEFAULT_CHARSET;
	strcpy(foo.m_facename, "Tahoma");
	return foo;
}

osd_config::osd_config()
{
	{
		name = "Default";
		flags = osd_play | osd_outline | osd_antialias;
		font = get_def_font();
		displaytime = 6000;
		x = 50;
		y = 50;
		pos = DT_CENTER;
		align = DT_CENTER;
		vwidth = 75;
		vheight = 8;
		vsteps = 54;
		vmin = -4000;
		format = default_format;
		formatnext = default_format_next;
		color = RGB(0, 255, 0);
		bgcolor = 0;
		alphalev = 192;
		alphaback = 0;
		fadetime = 200;
		dissolve_decay = 50;
	}
}

const osd_config & osd_config::operator =(const osd_config & in)
{
	name = in.name;
	flags = in.flags;
	font = in.font;
	displaytime = in.displaytime;
	x = in.x;
	y = in.y;
	pos = in.pos;
	align = in.align;
	vwidth = in.vwidth;
	vheight = in.vheight;
	vsteps = in.vsteps;
	vmin = in.vmin;
	format = in.format;
	formatnext = in.formatnext;
	color = in.color;
	bgcolor = in.bgcolor;
	alphalev = in.alphalev;
	alphaback = in.alphaback;
	fadetime = in.fadetime;
	dissolve_decay = in.dissolve_decay;

	return *this;
}

osd_state::osd_state(osd_config & _conf) : conf(_conf)
{
	on_change();
}

void osd_state::on_change()
{
	static_api_ptr_t<titleformat_compiler> tf;

	tf->compile_safe(format, conf.format);
	tf->compile_safe(formatnext, conf.formatnext);
}
