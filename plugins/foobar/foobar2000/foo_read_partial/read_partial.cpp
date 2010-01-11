// change log

// 2003-04-30 10:49 UTC - kode54
// - Correctly set is_mp3 when there is no content-type

#include "../SDK/foobar2000.h"

static __int64 xtoi(const char * in)
{
	__int64 out = 0;
	while (in && *in && ((*in >= '0' && *in <= '9') || (*in >= 'A' && *in <= 'F') || (*in >= 'a' && *in <= 'f')))
	{
		out *= 16;
		if (*in >= 'a') out += *in - 'a' + 10;
		else if (*in >= 'A') out += *in - 'A' + 10;
		else out += *in - '0';
		in++;
	}
	return out;
}

class reader_partial : public reader
{
private:
	reader * r;
	__int64 start, end;

public:
	~reader_partial()
	{
	}

	reader_partial()
	{
	}


	virtual bool open(const char *p_path,MODE mode)
	{
		if (mode!=MODE_READ) return false;

		const char * meh = p_path + 10;
		
		start = xtoi(meh);

		meh = strchr(meh, '-');
		if (!meh) return false;
		meh++;

		end = xtoi(meh);

		meh = strchr(meh, ':');
		if (!meh) return false;
		meh++;

		string8 path("file://");
		path.add_string(meh);
		path.truncate(path.find_last('\\'));

		r = file::g_open(path, mode);

		if (!r) return false;

		r->seek(start);

		return true;
	}

	virtual unsigned read(void *buffer, unsigned int length)
	{
		if (length<=0) return 0;

		__int64 remain = end - r->get_position() + 1;

		if ((__int64)length > remain) length = (unsigned int)remain;

		return r->read(buffer, length);
	}

	virtual unsigned write(const void *buffer, unsigned length)
	{
		return 0;
	}
	
	virtual __int64 get_length()
	{
		return end - start + 1;
	}

	virtual __int64 get_position()
	{
		return r->get_position() - start;
	}

	virtual bool seek(__int64 position)
	{
		if (start + position > end) return false;
		return r->seek(start + position);
	}
};

class file_partial : public file
{
public:
	virtual int get_display_path(const char * path,string_base & out)
	{
		const char * ptr = strstr(path,"://");
		if (ptr)
		{
			out.set_string(ptr+3);
			return 1;
		}
		else return 0;
	}

	virtual int get_canonical_path(const char * path,string_base & out)
	{
		if (!is_our_path(path)) return 0;
		out.set_string(path);
		return 1;
	}

	virtual int is_our_path(const char * path)
	{
		return !strcmp_partial(path,"partial://");
	}

	virtual reader * get_reader(const char * path)
	{
		return new reader_partial;
	}

	virtual int exists(const char * path)
	{
		const char * meh = path + 10;
		meh = strchr(meh, '-');
		if (!meh) return 0;
		meh = strchr(meh, ':');
		if (!meh) return 0;
		string8 temp("file://");
		temp.add_string(++meh);
		temp.truncate(temp.find_last('\\'));
		return file::g_exists(temp);
	}
	virtual int remove(const char * path)
	{
		return 0;
	}
	virtual int move(const char * src,const char * dst)
	{
		return 0;
	}
};

static service_factory_single_t<file,file_partial> foo;


DECLARE_COMPONENT_VERSION("Partial reader","1.0",0);