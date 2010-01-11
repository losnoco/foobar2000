#include "foobar2000.h"

bool playback_core::get_cur_track_handle(metadb_handle_ptr & p_out)
{
	track t;
	if (get_cur_track(&t))
	{
		p_out = t.handle;
		return true;
	}
	else return false;
}

double playback_core::get_total_time()
{
	double rv = 0;
	metadb_handle_ptr item;
	if (get_cur_track_handle(item))
	{
		rv = item->get_length();
	}
	return rv;
}

bool playback_core::g_create(service_ptr_t<playback_core> & p_out,callback * cb)
{
	if (service_enum_create_t(p_out,0))
	{
		p_out->set_callback(cb);
		return true;
	}
	else return false;
}
