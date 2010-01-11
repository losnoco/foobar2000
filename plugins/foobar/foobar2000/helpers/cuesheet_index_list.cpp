#include "stdafx.h"

static bool is_numeric(char c) {return c>='0' && c<='9';}

void t_cuesheet_index_list::to_infos(file_info & p_out) const
{
	double base = m_positions[1];
	
	if (m_positions[0] < base)
		p_out.info_set("pregap",cuesheet_format_index_time(base - m_positions[0]));
	else
		p_out.info_remove("pregap");

	p_out.info_remove("index 00");	
	p_out.info_remove("index 01");
	
	for(unsigned n=2;n<count;n++)
	{
		char namebuffer[16];
		sprintf(namebuffer,"index %02u",n);
		double position = m_positions[n] - base;
		if (position > 0)
			p_out.info_set(namebuffer,cuesheet_format_index_time(position));
		else
			p_out.info_remove(namebuffer);
	}
}

static bool parse_value(const char * p_name,double & p_out)
{
	if (p_name == NULL) return false;
	try {
		p_out = cuesheet_parse_index_time_e(p_name,strlen(p_name));
	} catch(pfc::exception_text const &) {return false;}
	return true;
}

void t_cuesheet_index_list::from_infos(file_info const & p_in,double p_base)
{
	double pregap;
	if (!parse_value(p_in.info_get("pregap"),pregap)) pregap = 0;
	m_positions[0] = p_base - pregap;
	m_positions[1] = p_base;
	for(unsigned n=2;n<count;n++)
	{
		char namebuffer[16];
		sprintf(namebuffer,"index %02u",n);
		double temp;
		if (parse_value(p_in.info_get(namebuffer),temp))
			m_positions[n] = temp + p_base;
		else
			m_positions[n] = 0;
	}
	
}

cuesheet_format_index_time::cuesheet_format_index_time(double p_time)
{
	t_uint64 ticks = dsp_util::duration_samples_from_time(p_time,75);
	t_uint64 seconds = ticks / 75; ticks %= 75;
	t_uint64 minutes = seconds / 60; seconds %= 60;
	m_buffer << format_uint(minutes,2) << ":" << format_uint(seconds,2) << ":" << format_uint(ticks,2);
}

double cuesheet_parse_index_time_e(const char * p_string,unsigned p_length)
{
	return (double) cuesheet_parse_index_time_ticks_e(p_string,p_length) / 75.0;
}

unsigned cuesheet_parse_index_time_ticks_e(const char * p_string,unsigned p_length)
{
	unsigned ptr = 0;
	unsigned splitmarks[2];
	unsigned splitptr = 0;
	for(ptr=0;ptr<p_length;ptr++)
	{
		if (p_string[ptr] == ':')
		{
			if (splitptr >= 2) throw pfc::exception_text("invalid INDEX time syntax");
			splitmarks[splitptr++] = ptr;
		}
		else if (!is_numeric(p_string[ptr])) throw pfc::exception_text("invalid INDEX time syntax");
	}
	
	unsigned minutes_base = 0, minutes_length = 0, seconds_base = 0, seconds_length = 0, frames_base = 0, frames_length = 0;

	switch(splitptr)
	{
	case 0:
		frames_base = 0;
		frames_length = p_length;
		break;
	case 1:
		seconds_base = 0;
		seconds_length = splitmarks[0];
		frames_base = splitmarks[0] + 1;
		frames_length = p_length - frames_base;
		break;
	case 2:
		minutes_base = 0;
		minutes_length = splitmarks[0];
		seconds_base = splitmarks[0] + 1;
		seconds_length = splitmarks[1] - seconds_base;
		frames_base = splitmarks[1] + 1;
		frames_length = p_length - frames_base;
		break;
	}

	unsigned ret = 0;

	if (frames_length > 0) ret += atoui_ex(p_string + frames_base,frames_length);
	if (seconds_length > 0) ret += 75 * atoui_ex(p_string + seconds_base,seconds_length);
	if (minutes_length > 0) ret += 60 * 75 * atoui_ex(p_string + minutes_base,minutes_length);

	return ret;	
}
