#include "foobar2000.h"


bool play_control::g_get(service_ptr_t<play_control> & p_out)
{
	return service_enum_create_t(p_out,0);
}

double play_control::playback_get_length()
{
	double rv = 0;
	metadb_handle_ptr ptr;
	if (get_now_playing(ptr))
	{
		rv = ptr->get_length();
	}
	return rv;
}
