#ifndef _OGG_HELPER_H_
#define _OGG_HELPER_H_

#include "../SDK/playlist_loader.h"
#include "../SDK/reader_helper.h"

namespace ogg_helper
{
	t_io_result query_chained_stream_offset(const service_ptr_t<file> & r,const playable_location & src,t_filesize & out_beginning,t_filesize & out_end,abort_callback & p_abort);
	t_io_result query_link_count(const char * filename,const service_ptr_t<file> & r,unsigned & p_count,abort_callback & p_abort);

	class stream_reader : public reader_limited
	{
	public:
		t_io_result init(const service_ptr_t<file> & r,const playable_location & src,abort_callback & p_abort)
		{
			t_filesize begin,end;
			t_io_result status;
			status = ogg_helper::query_chained_stream_offset(r,src,begin,end,p_abort);
			if (io_result_failed(status)) return status;
			return reader_limited::init(r,begin,end,p_abort);
		}
		
		stream_reader() {}

		stream_reader(file * p_r,const playable_location & src,abort_callback & p_abort)
		{
			init(p_r,src,p_abort);
		}
	};
}



#endif