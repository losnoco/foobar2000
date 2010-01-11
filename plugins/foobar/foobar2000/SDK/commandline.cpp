#include "foobar2000.h"

void commandline_handler_metadb_handle::on_file(const char * url)
{

	abort_callback_impl blah;

	{
		playlist_loader_callback_i callback(blah);

		playlist_loader::g_process_path_ex(url,callback);

		{
			unsigned n,m=callback.get_count();
			for(n=0;n<m;n++) on_file(callback.get_item(n));
		}
	}
}
