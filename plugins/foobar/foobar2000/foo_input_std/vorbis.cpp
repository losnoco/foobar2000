#include "stdafx.h"
#include <vorbis/vorbisfile.h>
#include <math.h>
#include "ogg_helper.h"

struct vcedit_callback
{
	service_ptr_t<file> m_reader;
	abort_callback & m_abort;

	static size_t callback_fread(void *ptr, size_t size, size_t nmemb, void* instance)
	{
		vcedit_callback * callback = reinterpret_cast<vcedit_callback *>(instance);
		assert(callback);
		assert(callback->m_reader.is_valid());
		unsigned io_bytes_done;
		return io_result_succeeded( callback->m_reader->read(ptr,size*nmemb,io_bytes_done,callback->m_abort))
			? io_bytes_done / size : 0;
	}

	static size_t callback_write(const void *ptr, size_t size, size_t nmemb, void * instance)
	{
		vcedit_callback * callback = reinterpret_cast<vcedit_callback *>(instance);
		assert(callback);
		assert(callback->m_reader.is_valid());
		unsigned io_bytes_done;
		return io_result_succeeded( callback->m_reader->write(ptr,size*nmemb,io_bytes_done,callback->m_abort))
			? io_bytes_done / size : 0;
	}

	vcedit_callback(const service_ptr_t<file> & p_reader,abort_callback & p_abort) : m_reader(p_reader), m_abort(p_abort) {}
};

static const struct vorbis_release {
    char *vendor_string;
    char *desc;
} vorbis_releases[] = {
    {"Xiphophorus libVorbis I 20000508", "1.0 beta 1 or beta 2"},
    {"Xiphophorus libVorbis I 20001031", "1.0 beta 3"},
    {"Xiphophorus libVorbis I 20010225", "1.0 beta 4"},
    {"Xiphophorus libVorbis I 20010615", "1.0 RC1"},
    {"Xiphophorus libVorbis I 20010813", "1.0 RC2"},
    {"Xiphophorus libVorbis I 20011217", "1.0 RC3"},
    {"Xiphophorus libVorbis I 20011231", "1.0 RC3"},
    {"Xiph.Org libVorbis I 20020717", "1.0"},
    {"Xiph.Org libVorbis I 20030909", "1.0.1"},
    {"Xiphophorus libVorbis I 20010816 (gtune 1)", "1.0 RC2 GT1"},
    {"Xiphophorus libVorbis I 20011014 (GTune 2)", "1.0 RC2 GT2"},
    {"Xiph.Org/Sjeng.Org libVorbis I 20020717 (GTune 3, beta 1)", "1.0 GT3b1"},
	 {"Xiph.Org libVorbis I 20040629", "1.1.0"},
};

class input_vorbis : public input
{
private:



	service_impl_single_t<ogg_helper::stream_reader> oggreader;

	mem_block_aligned_t<audio_sample> buffer;

	OggVorbis_File vf;
	bool is_seekable;
	long serial;
	double time_total;
	bool eof,b_new_track;
	dynamic_bitrate_helper m_vbr_handler;
	
	abort_callback * m_abort;
	service_ptr_t<file> m_reader;

public:
	inline static bool g_is_our_content_type(const char * url,const char * type)
	{
		return !strcmp(type,"application/ogg") || !strcmp(type,"application/x-ogg");
	}

	inline static bool g_test_filename(const char * fn,const char * ext) {return !stricmp_utf8(ext,"OGG");}

	input_vorbis()
	{
		memset(&vf,0,sizeof(vf));
		serial = 0;
		m_reader = 0;
		m_abort = 0;
	}

	~input_vorbis()
	{
		ov_clear(&vf);
	}

	static void do_info(OggVorbis_File & l_vf,file_info & info)
	{
		vorbis_comment * vc = ov_comment(&l_vf,-1);
		vorbis_info * vi = ov_info(&l_vf,-1);

		info.meta_remove_all();
		info.info_remove_all();

		info.set_length(ov_time_total(&l_vf,-1));
		info.info_set_int("bitrate",(t_int64)(ov_bitrate(&l_vf,-1)/1000.0+0.5));
		if (vi)
		{
			info.info_set_int("channels",vi->channels);
			info.info_set_int("samplerate",vi->rate);
			if (vi->bitrate_nominal>0)
				info.info_set_int("bitrate_nominal",vi->bitrate_nominal/1000);
		}
		info.info_set("codec","Vorbis");
		info.info_set("tool",vc->vendor);

        for(int n=0;n<tabsize(vorbis_releases);n++)
        {
            if (!strcmp(vc->vendor,vorbis_releases[n].vendor_string))
            {
		        info.info_set("vorbis_version",vorbis_releases[n].desc);
                break;
            }
        }


		if (vc)
		{
			string8_fastalloc name,value;
			
			int n;
			for(n=0;n<vc->comments;n++)
			{
				const char * p_name = vc->user_comments[n];
				if (is_valid_utf8(p_name))
				{
					name = p_name;
					int separator = name.find_first('=');
					if (separator>0)
					{
						value = name.get_ptr() + separator + 1;
						name.truncate(separator);
						const char *item = (const char *)name;
						if (!info.info_set_replaygain(item,value))
							info.meta_add(item,value);
					}
					else
					{
						info.meta_add("COMMENT",separator==0 ? (const char*)name+1 : (const char*)name);
					}
				}
			}
		}
	}
	
	t_io_result get_info(const service_ptr_t<file> & p_reader, const playable_location & p_location, file_info & p_info, abort_callback & p_abort)
	{
		return open_internal(p_reader,p_location,p_info,p_abort,false,true);
	}

	t_io_result open(const service_ptr_t<file> & p_reader,const playable_location & p_location, file_info & p_info,abort_callback & p_abort,unsigned p_flags)
	{
		return open_internal(p_reader,p_location,p_info,p_abort,true,!!(p_flags&OPEN_FLAG_GET_INFO));
	}

	t_io_result open_internal(const service_ptr_t<file> & p_file,const playable_location & p_location,file_info & p_info,abort_callback & p_abort,bool p_decode,bool p_want_info)
	{
		vartoggle_t<abort_callback*> blah(m_abort,&p_abort);
		is_seekable = !!p_file->can_seek();
		if (is_seekable)
		{	
			//seekable
			t_io_result status = oggreader.init(p_file,p_location,p_abort);
			if (io_result_failed(status)) return io_result_error_data;
			m_reader = &oggreader;
		
			if (ov_open_callbacks(this,&vf,0,0,*(ov_callbacks*)&callbacks))
			{
				return io_result_error_data;
			}
		}
		else
		{
			m_reader = p_file;
			if (ov_open_callbacks(this,&vf,0,0,*(ov_callbacks*)&callbacks))
			{
				return io_result_error_data;
			}

		}


		if (p_want_info)
		{
			if (!is_seekable)
			{
				b_new_track = true;
				p_info.set_length(-1.0);
				time_total = -1;
			}
			else
			{
				b_new_track = false;
				do_info(vf,p_info);
				time_total = p_info.get_length();
			}
		}
		else
		{
			if (!is_seekable)
			{
				b_new_track = true;
				time_total = -1;
			}
			else
			{
				b_new_track = false;
				time_total = ov_time_total(&vf,-1);
			}
		}

		eof = false;

		m_vbr_handler.reset();

		return io_result_success;

	}

	virtual t_io_result run(audio_chunk * chunk,abort_callback & p_abort)
	{
		vartoggle_t<abort_callback*> blah(m_abort,&p_abort);

		if (eof) return io_result_eof;

		float ** channels;
		int nsam;
		while(1)
		{
			nsam=ov_read_float(&vf,&channels,4096,0);
			if (nsam<0)
			{
				if (nsam==OV_HOLE) continue;
				return io_result_eof;
			}
			break;
		}
		if (nsam<=0) return io_result_eof;
		if (!is_seekable && serial != vf.current_serialno)
		{
			b_new_track = true;
			serial = vf.current_serialno;
		}

		vorbis_info* vi=ov_info(&vf,-1);

		buffer.check_size(nsam * vi->channels);
		
		{
			int n,p=0,c;
			if (vi->channels==6)
			{
				for(n=0;n<nsam;n++)
				{
					buffer[p++]=channels[0][n];
					buffer[p++]=channels[2][n];
					buffer[p++]=channels[1][n];
					buffer[p++]=channels[5][n];
					buffer[p++]=channels[3][n];
					buffer[p++]=channels[4][n];
				}
			}
			else for(n=0;n<nsam;n++)
			{
				for(c=0;c<vi->channels;c++)
				{	
					buffer[p++]=channels[c][n];
				}
			}

		}

		

		{
			long val = ov_bitrate_instant(&vf);
			if (val>0)
			{
				double length = (double)nsam/(double)vi->rate;
				m_vbr_handler.on_frame(length, (unsigned)( val * length ) );
			}
		}

		chunk->set_data(buffer,nsam,vi->channels,vi->rate);
		return io_result_success;
	}

	virtual t_io_result seek(double s,abort_callback & p_abort)
	{
		vartoggle_t<abort_callback*> blah(m_abort,&p_abort);
		if (!is_seekable) return io_result_error_generic;
		else if (s<0) return io_result_error_generic;
		else if (s>=time_total)
		{
			eof = true;
			return io_result_success;
		}
		else if (ov_time_seek(&vf,s)==0)
		{
			eof = false;
			return io_result_success;
		}
		else {eof = true;return io_result_error_generic;}
	}
	virtual bool can_seek() {return !!vf.seekable;}

	virtual t_io_result set_info(const service_ptr_t<file> &r,const playable_location & p_location,file_info & info,abort_callback & p_abort);

	virtual bool get_dynamic_info(file_info & out,double * timestamp_delta,bool * b_track_change)
	{
		if (b_new_track)
		{
			b_new_track = false;
			do_info(vf,out);
			*b_track_change = true;
			return true;
		}
		return m_vbr_handler.on_update(out,timestamp_delta);
	}

	inline static bool g_needs_reader() {return true;}

	static GUID g_get_guid()
	{
		// {CB8FDC3C-3B69-49fe-A2D2-CE08084D1D72}
		static const GUID guid = 
		{ 0xcb8fdc3c, 0x3b69, 0x49fe, { 0xa2, 0xd2, 0xce, 0x8, 0x8, 0x4d, 0x1d, 0x72 } };
		return guid;
	}

	static const char * g_get_name() {return "Ogg Vorbis decoder";}

	inline static t_io_result g_get_extended_data(const service_ptr_t<file> & p_reader,const playable_location & p_location,const GUID & p_guid,stream_writer * p_out,abort_callback & p_abort) {return io_result_error_data;}

private:

	static size_t callback_fread(void *ptr, size_t size, size_t nmemb, input_vorbis * r)
	{
		assert(r->m_abort);
		unsigned io_bytes_done;
		return io_result_succeeded( r->m_reader->read(ptr,size*nmemb,io_bytes_done,*r->m_abort) )
			? io_bytes_done / size : 0;
	}

	static size_t callback_write(const void *ptr, size_t size, size_t nmemb, input_vorbis * r)
	{
		assert(r->m_abort);
		unsigned io_bytes_done;
		return io_result_succeeded( r->m_reader->write(ptr,size*nmemb,io_bytes_done,*r->m_abort) )
			? io_bytes_done / size : 0;
	}

	static int callback_fseek(input_vorbis *r, t_int64 offset, int whence)
	{
		assert(r->m_abort);
		if (!r->m_reader->can_seek()) return -1;
		return io_result_succeeded( r->m_reader->seek2(offset,whence,*r->m_abort) ) ? 0 : 1;
	}

	static int callback_fclose(input_vorbis *r) {return 0;}

	static t_int64 callback_ftell(input_vorbis *r)
	{
		t_filesize ret;
		if (io_result_failed(r->m_reader->get_position(ret,*r->m_abort))) return -1;
		else return (t_int64) ret;
	}


	static void* callbacks[4];

};

void* input_vorbis::callbacks[4]=
{
	callback_fread,callback_fseek,callback_fclose,callback_ftell
};


static input_factory_t<input_vorbis> g_input_vorbis_factory;

#include "vcedit.h"


t_io_result input_vorbis::set_info(const service_ptr_t<file> &r,const playable_location & p_location,file_info & info,abort_callback & p_abort)
{
	t_io_result status;
	if (!r->can_seek()) return io_result_error_data;
	t_filesize total_size;
	status = r->get_size(total_size,p_abort);
	if (io_result_failed(status)) return status;	
	if (total_size == filesize_invalid) return io_result_error_generic;
	t_filesize stream_beginning,stream_end;
	vcedit_state *vs;
	status = ogg_helper::query_chained_stream_offset(r,p_location,stream_beginning,stream_end,p_abort);
	if (io_result_failed(status)) return status;
	service_ptr_t<file> temp;
	if (total_size > 16*1024*1024) status = filesystem::g_open_temp(temp,p_abort);
	else status = filesystem::g_open_tempmem(temp,p_abort);
	if (io_result_failed(status))
	{
		console::error("error creating temp file for vorbis comment editor");
		return status;
	}
	r->seek(stream_beginning,p_abort);

	{
		service_impl_single_t<reader_limited> r2;

		r2.init(r,stream_beginning,stream_end,p_abort);

		vcedit_callback vcedit_callback_in(&r2,p_abort),
			vcedit_callback_out(temp,p_abort);

		vs = vcedit_new_state();
		if (vcedit_open_callbacks(vs,&vcedit_callback_in,vcedit_callback::callback_fread,vcedit_callback::callback_write)<0)
		{
			vcedit_clear(vs);
			return io_result_error_data;
		}
		
		{
			vorbis_comment * vc = vcedit_comments(vs);
			vorbis_comment_clear(vc);
			vorbis_comment_init(vc);
			{
				unsigned n, m = info.meta_get_count();
				for(n=0;n<m;n++)
				{
					const char * name = info.meta_enum_name(n);
					unsigned n1,m1 = info.meta_enum_value_count(n);
					for(n1=0;n1<m1;n1++)
						vorbis_comment_add_tag(vc,const_cast<char*>(name), const_cast<char*>(info.meta_enum_value(n,n1)));
				}
			}
			{
				replaygain_info rg = info.get_replaygain();
				char rgtemp[replaygain_info::text_buffer_size];
				if (rg.is_album_gain_present())
				{
					rg.format_album_gain(rgtemp);
					vorbis_comment_add_tag(vc,"replaygain_album_gain",rgtemp);
				}
				if (rg.is_album_peak_present())
				{
					rg.format_album_peak(rgtemp);
					vorbis_comment_add_tag(vc,"replaygain_album_peak",rgtemp);
				}
				if (rg.is_track_gain_present())
				{
					rg.format_track_gain(rgtemp);
					vorbis_comment_add_tag(vc,"replaygain_track_gain",rgtemp);
				}
				if (rg.is_track_peak_present())
				{
					rg.format_track_peak(rgtemp);
					vorbis_comment_add_tag(vc,"replaygain_track_peak",rgtemp);
				}
			}
		}
		
		vcedit_write(vs,&vcedit_callback_out);
		vcedit_clear(vs);
	}
	try {
		r->seek_e(stream_end,p_abort);
		if (stream_end<r->get_size_e(p_abort)) file::g_transfer_object_e(r.get_ptr(),temp.get_ptr(),r->get_size_e(p_abort)-stream_end,p_abort);
		r->seek_e(p_location.get_subsong()>0 ? stream_beginning : 0,p_abort);
		r->set_eof_e(p_abort);
		temp->seek_e(0,p_abort);
		file::g_transfer_object_e(temp.get_ptr(),r.get_ptr(),temp->get_size_e(p_abort),p_abort);
	} catch(t_io_result status) {
		return status;
	}
	return io_result_success;
}