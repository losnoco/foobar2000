#include "main.h"

void g_callback_register();
void g_callback_modify();
void g_callback_unregister();

static const GUID guid_cfg_g_osd = 
{ 0xb575899f, 0xdb1c, 0x42c3, { 0x9b, 0xba, 0x7, 0x71, 0x59, 0x72, 0xac, 0x4a } };

cfg_osd_list g_osd(guid_cfg_g_osd);

#if 0
class next_extras : public titleformat_hook
{
	service_ptr_t<titleformat_object> & m_format;

public:
	next_extras(service_ptr_t<titleformat_object> & p_format) : m_format(p_format) {}

	virtual bool process_function(titleformat_text_out * p_out,const char * p_name,unsigned p_name_length,titleformat_hook_function_params * p_params,bool & p_found_flag)
	{
		return false;
	}

	virtual bool process_field(titleformat_text_out * p_out,const char * p_name,unsigned p_name_length,bool & p_found_flag)
	{
		if (stricmp_utf8_max(p_name, "_next", p_name_length))
		{
			p_found_flag = false;
			return true;
		}

		do
		{
			static_api_ptr_t<play_control> pc;
			if (pc->get_stop_after_current()) break;
			/*{
			metadb_handle * file = pc->get_now_playing();
			double len = file->handle_get_length();
			file->handle_release();
			if (!len) break;
			}*/
			int repeat = 0;

			static_api_ptr_t<playlist_manager> pm;
			const char * order = pm->playback_order_get_name(pm->playback_order_get_active());
			if (!stricmp_utf8(order, "Repeat")) repeat = 1;
			if (!repeat && stricmp_utf8(order, "Default")) break;

			unsigned playing, next, count;

			bool follow;

			if (io_result_failed(config_object::g_get_data_bool(standard_config_objects::bool_playback_follows_cursor, follow))) break;

			if (!pm->get_playing_item_location(&playing, &next) ||
				playing == infinite)
			{
				playing = pm->get_playing_playlist();
				next = pm->playlist_get_playback_cursor(playing);
			}
			else if (next == infinite)
			{
				next = pm->playlist_get_playback_cursor(playing);
			}

			do {
				if (playing != infinite && next != infinite)
				{
					if (follow)
					{
						unsigned active = pm->get_active_playlist();
						unsigned focus = pm->activeplaylist_get_focus_item();
						if (active != playing || focus != next)
						{
							playing = active;
							next = focus;
							count = infinite;
							break;
						}
					}

					count = pm->playlist_get_item_count(playing);

					if (++next >= count)
					{
						if (repeat) next = 0;
					}
				}
			} while(0);

			if (playing == infinite || next >= count) break;

			string8 result;
			pm->playlist_item_format_title(playing, next, NULL, result, m_format, NULL, play_control::display_level_all);
			if (result.length())
			{
				string_utf8_nocolor nc(result);
				if (nc.length())
				{
					p_out->write(nc, nc.length());
					p_found_flag = true;
				}
			}
		} while (0);

		if (!p_found_flag) p_out->write("?", 1);

		return true;
	}
};
#define next_extra_i &next_extras(s->formatnext)
#else
#define next_extra_i 0
#endif

void cfg_osd_list::get_data_raw(stream_writer * p_stream,abort_callback & p_abort)
{
	unsigned i, count = val.get_count();
	p_stream->write_lendian_t( count, p_abort );

	for (i = 0; i < count && !p_abort.is_aborting(); i++)
	{
		const osd_config * c = val[ i ];
		p_stream->write_string( c->name, p_abort );
		p_stream->write_lendian_t( c->flags, p_abort );

		p_stream->write_lendian_t( c->font.m_height, p_abort );
		p_stream->write_lendian_t( c->font.m_weight, p_abort );
		p_stream->write_object_t( c->font.m_italic, p_abort );
		p_stream->write_object_t( c->font.m_charset, p_abort );
		p_stream->write_object( &c->font.m_facename, sizeof( c->font.m_facename ), p_abort );

		p_stream->write_lendian_t( c->displaytime, p_abort );
		p_stream->write_lendian_t( c->x, p_abort );
		p_stream->write_lendian_t( c->y, p_abort );
		p_stream->write_lendian_t( c->pos, p_abort );
		p_stream->write_lendian_t( c->align, p_abort );
		p_stream->write_lendian_t( c->vwidth, p_abort );
		p_stream->write_lendian_t( c->vheight, p_abort );
		p_stream->write_lendian_t( c->vsteps, p_abort );
		p_stream->write_lendian_t( c->vmin, p_abort );
		p_stream->write_string( c->format, p_abort );
		//p_stream->write_string( c->formatnext, p_abort );
		p_stream->write_lendian_t( c->color, p_abort );
		p_stream->write_lendian_t( c->bgcolor, p_abort );
		p_stream->write_lendian_t( c->alphalev, p_abort );
		p_stream->write_lendian_t( c->alphaback, p_abort );
		p_stream->write_lendian_t( c->fadetime, p_abort );
		p_stream->write_lendian_t( c->dissolve_decay, p_abort );
	}
}

void cfg_osd_list::set_data_raw(stream_reader * p_stream,unsigned p_sizehint,abort_callback & p_abort)
{
	unsigned i, count;

	val.delete_all();

	osd_config * c = 0;

	try
	{
		p_stream->read_lendian_t( count, p_abort );

		string8_fastalloc name, format; //, formatnext;
		for (i = 0; i < count; i++)
		{
			c = 0;
			c = new osd_config;
			p_stream->read_string( name, p_abort );
			p_stream->read_lendian_t( c->flags, p_abort );

			p_stream->read_lendian_t( c->font.m_height, p_abort );
			p_stream->read_lendian_t( c->font.m_weight, p_abort );
			p_stream->read_object_t( c->font.m_italic, p_abort );
			p_stream->read_object_t( c->font.m_charset, p_abort );
			p_stream->read_object( &c->font.m_facename, sizeof( c->font.m_facename ), p_abort );

			p_stream->read_lendian_t( c->displaytime, p_abort );
			p_stream->read_lendian_t( c->x, p_abort );
			p_stream->read_lendian_t( c->y, p_abort );
			p_stream->read_lendian_t( c->pos, p_abort );
			p_stream->read_lendian_t( c->align, p_abort );
			p_stream->read_lendian_t( c->vwidth, p_abort );
			p_stream->read_lendian_t( c->vheight, p_abort );
			p_stream->read_lendian_t( c->vsteps, p_abort );
			p_stream->read_lendian_t( c->vmin, p_abort );
			p_stream->read_string( format, p_abort );
			//p_stream->read_string( formatnext, p_abort );
			p_stream->read_lendian_t( c->color, p_abort );
			p_stream->read_lendian_t( c->bgcolor, p_abort );
			p_stream->read_lendian_t( c->alphalev, p_abort );
			p_stream->read_lendian_t( c->alphaback, p_abort );
			p_stream->read_lendian_t( c->fadetime, p_abort );
			p_stream->read_lendian_t( c->dissolve_decay, p_abort );

			c->name = name;
			c->format = format;
			//c->formatnext = formatnext;
			val.add_item( c );
			c = 0;
		}
	}
	catch ( ... )
	{
		if ( c ) delete c;
		reset();
		throw;
	}
}

void cfg_osd_list::reset()
{
	insync(sync);
	osd.delete_all();
	state.delete_all();
	val.delete_all();

	osd_config * c = new osd_config;
	c->name = "Title auto-pop";
	c->flags = osd_pop | osd_play | osd_dynamic | osd_outline | osd_antialias | osd_fadeinout;
	c->y = 40;

	val.add_item(c);

	c = new osd_config;
	c->name = "Volume display";
	c->flags = osd_pop | osd_volume | osd_outline | osd_antialias | osd_fadeinout;
	c->y = 70;

	val.add_item(c);

	if (initialized) init();
}

cfg_osd_list::cfg_osd_list(const GUID & p_guid) : cfg_var(p_guid), initialized(false)
{
	reset();
}

cfg_osd_list::~cfg_osd_list()
{
	val.delete_all();
}

bool cfg_osd_list::init()
{
	unsigned i, count;

	insync(sync);

	osd.delete_all();
	state.delete_all();

	for (i = 0, count = val.get_count(); i < count; i++)
	{
		osd_config * c = val[i];
		osd_state * s = new osd_state(*c);
		COsdWnd * o = new COsdWnd(*s);
		if (o->Initialize())
		{
			state.add_item(s);
			osd.add_item(o);
		}
		else
		{
			delete o;
			delete s;
			osd.delete_all();
			state.delete_all();
			return initialized = false;
		}
	}

	g_callback_register();

	return initialized = true;
}

void cfg_osd_list::quit()
{
	insync(sync);
	g_callback_unregister();
	osd.delete_all();
	state.delete_all();
	initialized = false;
}

/*
void dismiss_all()
{
insync(sync);
if (!initialized) return;

unsigned i, count;

for (i = 0, count = osd.get_count(); i < count; i++)
{
osd_config * c = val[i];
COsdWnd    * o = osd[i];

c->flags &= ~osd_pop;
o->Hide();
}
}
*/

void cfg_osd_list::get(unsigned n, osd_config & p_out)
{
	insync(sync);

	if (n < val.get_count())
	{
		p_out = val[n];
	}
}

/*void cfg_osd_list::get_all(array_t<osd_config> & p_out)
{
	insync(sync);

	unsigned i, count = val.get_count();

	p_out.set_size(count);

	for (i = 0; i < count; i++) p_out[i] = val[i];
}*/

void cfg_osd_list::set(unsigned n, const osd_config & p_in)
{
	insync(sync);

	if (n < val.get_count())
	{
		*val[n] = p_in;

		if (initialized)
		{
			COsdWnd * o = osd[n];

			if (o->GetState() != COsdWnd::HIDDEN) o->Hide();

			state[n]->on_change();

			g_callback_modify();
		}
	}
}

unsigned cfg_osd_list::add(osd_config & p_in)
{
	insync(sync);

	val.add_item(new osd_config(p_in));

	if (initialized)
	{
		unsigned n = val.get_count() - 1;
		osd_config * c = val[n];
		osd_state  * s = new osd_state(*c);
		COsdWnd    * o = new COsdWnd(*s);
		if (o->Initialize())
		{
			state.add_item(s);
			osd.add_item(o);

			g_callback_modify();
		}
		else
		{
			// ohnoes!
			delete o;
			delete s;
			val.delete_by_idx(n);
		}
	}

	return val.get_count();
}

unsigned cfg_osd_list::del(unsigned n)
{
	insync(sync);

	if (n < val.get_count())
	{
		if (initialized)
		{
			osd.delete_by_idx(n);
			state.delete_by_idx(n);

			g_callback_modify();
		}

		val.delete_by_idx(n);
	}

	return val.get_count();
}

void cfg_osd_list::rename(unsigned n, const char * name)
{
	insync(sync);

	if (n < val.get_count())
	{
		val[n]->name = name;
	}
}

void cfg_osd_list::get_names(pfc::array_t<string_simple> & p_out)
{
	insync(sync);

	unsigned i, count = val.get_count();

	p_out.set_size(count);

	for (i = 0; i < count; i++) p_out[i] = val[i]->name;
}

void cfg_osd_list::get_callback_flags( unsigned & p_play_callback_flags, unsigned & p_playlist_callback_flags )
{
	p_play_callback_flags = 0;
	p_playlist_callback_flags = 0;

	insync(sync);

	unsigned i, count = val.get_count();

	for ( i = 0; i < count; ++i )
	{
		osd_config * c = val[ i ];
		if ( c->flags & osd_interval ) p_play_callback_flags |= play_callback::flag_on_playback_time;
		if ( c->flags & osd_dynamic_all ) p_play_callback_flags |= play_callback::flag_on_playback_dynamic_info;
		if ( c->flags & osd_pop )
		{
			if ( c->flags & osd_switch ) p_playlist_callback_flags |= playlist_callback::flag_on_playlist_activate;
			if ( c->flags & osd_play ) p_play_callback_flags |= play_callback::flag_on_playback_new_track;
			if ( c->flags & osd_pause ) p_play_callback_flags |= play_callback::flag_on_playback_pause;
			if ( c->flags & osd_seek ) p_play_callback_flags |= play_callback::flag_on_playback_seek;
			if ( c->flags & osd_dynamic ) p_play_callback_flags |= play_callback::flag_on_playback_dynamic_info_track;
			if ( c->flags & osd_volume ) p_play_callback_flags |= play_callback::flag_on_volume_change;
			if ( c->flags & osd_hide_on_stop ) p_play_callback_flags |= play_callback::flag_on_playback_stop;
		}
	}
}

static void color_cut( pfc::string_base & fmt )
{
	if (*(fmt.get_ptr()) == 3)
		fmt.truncate(fmt.find_first(3, 1) + 1);
	else if (!stricmp_utf8_partial(fmt.get_ptr(), "$rgb("))
	{
		const char * ptr = fmt.get_ptr() + 5;
		const char * ptr2;
		DWORD fgcolor, olcolor;

		fgcolor = strtoul(ptr, (char **) &ptr2, 10);
		if (ptr2 != ptr && *ptr2 == ',' && ptr2[1])
		{
			fgcolor |= strtoul(ptr2 + 1, (char **) &ptr, 10) << 8;
			if (ptr != ptr2 && *ptr == ',' && ptr[1])
			{
				fgcolor |= strtoul(ptr + 1, (char **) &ptr2, 10) << 16;
				if (ptr2 != ptr && *ptr2 == ',' && ptr2[1])
				{
					olcolor = strtoul(ptr2 + 1, (char **) &ptr, 10);
					if (ptr != ptr2 && *ptr == ',' && ptr[1])
					{
						olcolor |= strtoul(ptr + 1, (char **) &ptr2, 10) << 8;
						if (ptr2 != ptr && *ptr2 == ',' && ptr2[1])
						{
							olcolor |= strtoul(ptr2 + 1, (char **) &ptr, 10) << 16;
							if (ptr != ptr2 && *ptr == ')')
							{
								fmt.reset();
								fmt.add_byte( 3 );
								fmt << format_int( fgcolor, 6, 16 );
								fmt.add_byte( '|' );
								fmt << format_int( olcolor, 6, 16 );
								fmt.add_byte(3);
							}
							else fmt.reset();
						}
						else fmt.reset();
					}
					else fmt.reset();
				}
				else if (ptr2 != ptr && *ptr2 == ')')
				{
					fmt.reset();
					fmt.add_byte( 3 );
					fmt << format_int( fgcolor, 6, 16 );
					fmt.add_byte(3);
				}
				else fmt.reset();
			}
			else fmt.reset();
		}
		else fmt.reset();
	}
	else
		fmt.reset();
}

void cfg_osd_list::test(unsigned n)
{
	insync(sync);
	if (!initialized || n >= osd.get_count()) return;

	osd_config * c = val[n];
	osd_state  * s = state[n];
	COsdWnd    * o = osd[n];

	string8 text;
	static_api_ptr_t<play_control>()->playback_format_title(next_extra_i, text, s->format, NULL, play_control::display_level_all);

	if (text.length()) o->Post(text, !!(c->flags & osd_interval));
	else
	{
		string8 fmt(c->format);
		color_cut(fmt);
		fmt += "*silence*";
		o->Post(fmt, false);
	}
}

void cfg_osd_list::on_playback_time()
{
	insync(sync);
	if (!initialized) return;

	unsigned i, count;
	string8_fastalloc cmd;
	static_api_ptr_t<play_control> pc;

	for (i = 0, count = osd.get_count(); i < count; i++)
	{
		osd_config * c = val[i];
		osd_state  * s = state[i];
		COsdWnd    * o = osd[i];
		if ((c->flags & osd_interval) && o->DoInterval() && o->GetState() != COsdWnd::HIDDEN)
		{
			pc->playback_format_title(next_extra_i, cmd, s->format, NULL, play_control::display_level_all);
			if (cmd.length()) o->Repost(cmd);
		}
	}
}

void cfg_osd_list::on_volume_change(int new_val)
{
	insync(sync);
	if (!initialized) return;

	unsigned i, count;

	for (i = 0, count = osd.get_count(); i < count; i++)
	{
		osd_config * c = val[i];
		COsdWnd    * o = osd[i];
		if ((c->flags & (osd_pop | osd_volume)) == (osd_pop | osd_volume)) o->PostVolume(new_val);
	}
}

void cfg_osd_list::on_playback_dynamic_info(bool b_track_change)
{
	insync(sync);
	if (!initialized) return;

	unsigned i, count;
	string8_fastalloc cmd;
	static_api_ptr_t<play_control> pc;

	for (i = 0, count = osd.get_count(); i < count; i++)
	{
		osd_config * c = val[i];
		osd_state  * s = state[i];
		COsdWnd    * o = osd[i];

		if (b_track_change)
		{
			if ((c->flags & (osd_pop | osd_dynamic)) == (osd_pop | osd_dynamic))
			{
				pc->playback_format_title(next_extra_i, cmd, s->format, NULL, play_control::display_level_all);
				if (cmd.length()) o->Post(cmd, !!(c->flags & osd_interval));
			}
		}
		else
		{
			if ((c->flags & osd_dynamic_all) && o->DoInterval() && o->GetState() != COsdWnd::HIDDEN)
			{
				pc->playback_format_title(next_extra_i, cmd, s->format, NULL, play_control::display_level_all);
				if (cmd.length()) o->Repost(cmd);
			}
		}
	}
}

void cfg_osd_list::on_playback_seek()
{
	insync(sync);
	if (!initialized) return;

	unsigned i, count;
	string8_fastalloc cmd;
	static_api_ptr_t<play_control> pc;

	for (i = 0, count = osd.get_count(); i < count; i++)
	{
		osd_config * c = val[i];
		osd_state  * s = state[i];
		COsdWnd    * o = osd[i];

		if ((c->flags & (osd_pop | osd_seek)) == (osd_pop | osd_seek))
		{
			pc->playback_format_title(next_extra_i, cmd, s->format, NULL, play_control::display_level_all);
			if (cmd.length()) o->Post(cmd, !!(c->flags & osd_interval));
		}
	}
}

void cfg_osd_list::on_playback_pause(int _state)
{
	insync(sync);
	if (!initialized) return;

	unsigned i, count;
	string8_fastalloc cmd;
	static_api_ptr_t<play_control> pc;

	for (i = 0, count = osd.get_count(); i < count; i++)
	{
		osd_config * c = val[i];
		osd_state  * s = state[i];
		COsdWnd    * o = osd[i];

		if ((c->flags & (osd_pop | osd_pause)) == (osd_pop | osd_pause))
		{
			pc->playback_format_title(next_extra_i, cmd, s->format, NULL, play_control::display_level_all);
			if (cmd.length()) o->Post(cmd, !_state && !!(c->flags & osd_interval));
		}
	}
}

void cfg_osd_list::on_playback_stop()
{
	insync(sync);
	if (!initialized) return;

	for (unsigned i = 0, count = osd.get_count(); i < count; i++)
	{
		osd_config * c = val[i];
		COsdWnd    * o = osd[i];

		if ( ( c->flags & ( osd_pop | osd_hide_on_stop ) ) == ( osd_pop | osd_hide_on_stop ) )
		{
			o->Hide();
		}
	}
}

void cfg_osd_list::on_playback_new_track(const metadb_handle_ptr & track)
{
	insync(sync);
	if (!initialized) return;

	unsigned i, count;
	string8_fastalloc cmd;
	static_api_ptr_t<play_control> pc;

	for (i = 0, count = osd.get_count(); i < count; i++)
	{
		osd_config * c = val[i];
		osd_state  * s = state[i];
		COsdWnd    * o = osd[i];

		if ((c->flags & (osd_pop | osd_play)) == (osd_pop | osd_play))
		{
			track->format_title(next_extra_i, cmd, s->format, NULL);
			if (cmd.length()) o->Post(cmd, !!(c->flags & osd_interval));
		}
	}
}

void cfg_osd_list::on_playlist_switch()
{
	insync(sync);
	if (!initialized) return;

	unsigned i, count;
	string8_fastalloc name, fmt;

	if (static_api_ptr_t<playlist_manager>()->activeplaylist_get_name(name))
	{
		if (!name.length()) name = "(unnamed)";

		for (i = 0, count = osd.get_count(); i < count; i++)
		{
			osd_config * c = val[i];
			osd_state  * s = state[i];
			COsdWnd    * o = osd[i];

			if ((c->flags & (osd_pop | osd_switch)) == (osd_pop | osd_switch))
			{
				fmt = c->format;
				color_cut(fmt);
				fmt += name;
				o->Post(fmt, false);
			}
		}
	}
}

void cfg_osd_list::show_track(unsigned n)
{
	insync(sync);
	if (!initialized) return;

	if (n < osd.get_count())
	{
		osd_config * c = val[n];
		osd_state  * s = state[n];
		COsdWnd    * o = osd[n];
		string8_fastalloc cmd;

		static_api_ptr_t<play_control>()->playback_format_title(next_extra_i, cmd, s->format, NULL, play_control::display_level_all);
		if (cmd.length()) o->Post(cmd, !!(c->flags & osd_interval));
	}
}

void cfg_osd_list::show_playlist(unsigned n)
{
	insync(sync);
	if (!initialized) return;

	string8_fastalloc name, fmt;

	if (n < osd.get_count() && static_api_ptr_t<playlist_manager>()->activeplaylist_get_name(name))
	{
		osd_config * c = val[n];
		osd_state  * s = state[n];
		COsdWnd    * o = osd[n];

		if (!name.length()) name = "(unnamed)";

		fmt = c->format;
		color_cut(fmt);
		fmt += name;
		o->Post(fmt, false);
	}
}

void cfg_osd_list::show_volume(unsigned n)
{
	insync(sync);
	if (!initialized) return;

	if (n < osd.get_count())
	{
		osd[n]->PostVolume(static_api_ptr_t<play_control>()->get_volume());
	}
}

void cfg_osd_list::hide(unsigned n)
{
	insync(sync);
	if (!initialized) return;

	if (n < osd.get_count())
	{
		osd[n]->Hide();
	}
}
