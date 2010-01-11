#include "reader_hacks.h"
#include "../SDK/componentversion.h"
#include <unpack.h>
#include <reader_helper_mem.h>
#include <playlist_loader.h>


int wfread( void *buffer, int size, int count, WFILE *stream )
{
	int r=stream->read((char*)buffer,size*count);
	return r/size;
}

int wfseek( WFILE *f, long offset, int origin )
{
	switch(origin)
	{
	case SEEK_CUR:
		offset+=(int)f->get_position();
		break;
	case SEEK_END:
		offset+=(int)f->get_length();
		break;
	}
	return !f->seek(offset);
}

long wftell(WFILE * f) {return (long)f->get_position();}

int wfgetlen(WFILE *f)
{
	return (int)f->get_length();
}

int wfgetc(WFILE *f)
{
	int rv=0;
	if (f->read((char*)&rv,1)<1) rv=-1;
	return rv;
}

int wfeof(WFILE * f)
{
	return f->get_position()==f->get_position();

}

int zip_loadfile(WFILE* src,const char * fn1,void * &out_data,int &out_size);
int gzip_loadfile(WFILE* file,void * &out_data,int &out_size);
int zip_list(WFILE* src,const char * fn,directory_callback * out);
int zip_exists(WFILE* src,const char * fn);
int zip_precache(WFILE* src,const char * path,playlist_loader_callback * callback);

class unpacker_zip : public unpacker
{
public:
	virtual reader *open(reader * file)
	{
//		if (!file->canSeek()) return 0;
		void * data=0;
		int size=0;
		char header[10];
		{
			int read = file->read(header,10);
			file->seek(0);
			if (read!=10) return 0;
		}

		if (*(DWORD*)header == 0x04034B50)
		{
			//zip
			void * _data=0;
			int _size = 0;
			if (zip_loadfile(file,0,_data,_size))
			{
				data = (char*)_data;
				size = _size;
			}
		}
		if ((*(DWORD*)header & 0xFFFFFF) == 0x088b1f)
		{
			void * _data;
			int _size;
			if (gzip_loadfile(file,_data,_size))
			{
				data = (char*)_data;
				size = _size;
			}

		}
		if (!data)
		{
			file->seek(0);
			return 0;
		}
		return new reader_mem(data,size);
	}
};

class reader_zip : public reader_mem_base
{
private:
	virtual bool open(const char * path,enum reader::MODE mode)
	{
		if (mode!=MODE_READ) return 0;
		string8 archive,file;
		if (!archive_i::parse_unpack_path(path,archive,file)) return 0;

		reader * r = file::g_open(archive,reader::MODE_READ);
		if (r==0) return 0;

		void * data = 0;
		int size = 0;

		int rv = zip_loadfile(r,file,data,size);

		r->reader_release();

		if (rv) mem_init(data,size);
		return rv;
	}
};

class archive_zip : public archive_i
{
private:
	virtual int precache(const char * path,playlist_loader_callback * callback)
	{
		return 0;
#if 0
		if (stricmp(string_extension_8(path),"ZIP") && !stricmp(string_extension_8(path),"PK3"))
			return 0;

		reader * r = file::g_open(path,reader::MODE_READ);
		if (!r) return 0;

		int rv = zip_precache(r,path,callback);

		r->reader_release();

		return rv;
#endif
	}

	virtual int list(const char * path,directory_callback *out,playlist_loader_callback * callback)
	{
		if (stricmp(string_extension_8(path),"ZIP") && stricmp(string_extension_8(path),"PK3"))
			return 0;

		reader * r = file::g_open(path,reader::MODE_READ);
		if (!r) return 0;

		int rv = zip_list(r,path,out);

		r->reader_release();

		return rv;
	}

	virtual const char * get_archive_type() {return "zip";}

	virtual int exists_in_archive(const char * archive,const char * file)
	{
		reader * r = file::g_open(archive,reader::MODE_READ);
		if (r==0) return 0;

		int rv = zip_exists(r,file);

		r->reader_release();

		return rv ? FILE_EXISTS : 0;

	}

	virtual reader * get_reader(const char * path)
	{
		return new reader_zip;
	}

};



static service_factory_t<unpacker,unpacker_zip> foo;
static archive_factory<archive_zip> foo2;

DECLARE_COMPONENT_VERSION("ZIP/GZIP reader","1.0",0);
