#include <foobar2000.h>

class reader_adf : public service_impl_t<reader>
{
	reader * src;

public:
	reader_adf() : src(0) {}
	~reader_adf()
	{
		if (src) src->service_release();
	}

	virtual bool open(const char * path,enum reader::MODE mode)
	{
		if (mode != MODE_READ) return false;
		src = file::g_open(path, mode);
		return !!src;
	}

	virtual unsigned read(void * buffer, unsigned length)
	{
		if (src)
		{
			unsigned rval = src->read(buffer, length);
			char * ptr = (char *) buffer;
			for (unsigned i = 0; i < rval; i++) *ptr++ ^= 0x22;
			return rval;
		}
		return 0;
	}

	virtual unsigned write(const void * buffer, unsigned length)
	{
		return 0;
	}

	virtual __int64 get_length()
	{
		if (src) return src->get_length();
		return 0;
	}

	virtual __int64 get_position()
	{
		if (src) return src->get_position();
		return 0;
	}

	virtual bool set_eof()
	{
		if (src) return src->set_eof();
		return false;
	}

	virtual bool seek(__int64 position)
	{
		if (src) return src->seek(position);
		return false;
	}

	virtual bool can_seek()
	{
		if (src) return src->can_seek();
		return false;
	}

	virtual void abort()
	{
		if (src) src->abort();
	}

	virtual bool get_content_type(string_base & out)
	{
		if (src) return src->get_content_type(out);
		return false;
	}

	virtual bool is_in_memory()
	{
		if (src) return src->is_in_memory();
		return false;
	}

	virtual void on_idle()
	{
		if (src) src->on_idle();
	}

	virtual bool error()
	{
		if (src) src->error();
		return false;
	}
};

class input_adf : public input
{
	reader * rdr;
	input_helper in;

public:
	input_adf() : rdr(0) {}
	~input_adf()
	{
		if (rdr) rdr->reader_release();
	}

	virtual bool needs_reader()
	{
		return false;
	}

	virtual set_info_t set_info(reader *r, const file_info *info)
	{
		return SET_INFO_FAILURE;
	}

	virtual bool open(reader * r,file_info * info,unsigned flags)
	{
		string8 path = info->get_file_path();

		if (!file::g_exists(path)) return 0;

		rdr = new reader_adf;
		if (!rdr->open(path, reader::MODE_READ)) return 0;

		playable_location_i meh;
		path += ".mp3";
		meh.set_path(path);
		meh.set_subsong_index(info->get_subsong_index());
		info->set_location(&meh);

		return in.open(info, rdr);
	}

	virtual bool is_our_content_type(const char *url, const char *type)
	{
		return 0;
	}

	virtual bool test_filename(const char * fn,const char * ext)
	{
		return !stricmp(ext, "ADF");
	}

	virtual int run(audio_chunk * chunk)
	{
		return in.run(chunk);
	}

	virtual bool can_seek()
	{
		return in.can_seek();
	}	
	virtual bool seek(double ms)
	{
		return in.seek(ms);
	}
	virtual void abort()
	{
		in.abort();
	}
};

static service_factory_t<input,input_adf> foo;

DECLARE_COMPONENT_VERSION("ADF reader","1.0",0);
