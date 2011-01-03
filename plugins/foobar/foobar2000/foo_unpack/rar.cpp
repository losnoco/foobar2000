#include <reader.h>
#include <unpack.h>
#include <input.h>//for test_filename
#include "../../../unrar/dll.hpp"
#include <metadb.h>
#include <playlist_loader.h>
#include <console.h>

#include "../SDK/componentversion.h"

#include <reader_helper_mem.h>

class rar_reader_i : public rar_reader
{
private:
	reader * r;
public:
	rar_reader_i(reader * p) {r=p;}
	virtual __int64 getlen() {return r->get_length();}
	virtual int read( void *buffer, int size) {return r->read(buffer,size);}
	virtual int seek( __int64 offset, int origin )
	{
		switch(origin)
		{
		case SEEK_CUR:
			offset += r->get_position();
			break;
		case SEEK_END:
			offset += r->get_length();
			break;
		}
		return !r->seek(offset);
	}
	virtual __int64 tell() {return r->get_position();}
	virtual int getc()
	{
		int t=0;
		return r->read(&t,1) ? t : -1;
	}
	virtual int eof()
	{
		return r->get_position()==r->get_length();
	}
};

struct RARPARAM
{
	mem_block_t<char> out;
	int ptr,max;
	bool extract;

	int setup(RARHeaderData * data,bool p_extract)
	{
		extract = p_extract;
		if (extract)
		{
			ptr=0;
			max = data->UnpSize;			
			if (!out.set_size(max)) extract=0;
		}
		return extract ? 1 : 0;
	}
	RARPARAM() {extract=0;ptr=max=0;}
};

static int PASCAL DataProc(RARPARAM * p,unsigned char *addr,int size)
{
	if (p->extract)
	{
		if (size>p->max-p->ptr) size=p->max-p->ptr;
		memcpy(p->out.get_ptr()+p->ptr,addr,size);
		p->ptr+=size;
	}
	return size;
}

class reader_rar : public reader_mem_base
{
private:
	virtual int open(const char * path,int mode)
	{
		if (mode!=MODE_READ) return 0;
		
		string8 archive,file;
		if (!archive_i::parse_unpack_path(path,archive,file)) return 0;
		
		reader * r = file::g_open(archive,reader::MODE_READ);
		if (r==0)
			return 0;

		string16 file_to_extract = string_wide_from_utf8(file);

		RARPARAM rp;
		rar_reader_i rr(r);
		
		RAROpenArchiveData open_data;
		memset(&open_data,0,sizeof(open_data));
		open_data.OpenMode=RAR_OM_EXTRACT;
		open_data.Reader=&rr;
		HANDLE hRar=RAROpenArchive(&open_data);
		if (hRar)
		{
			RARSetProcessDataProc(hRar,(PROCESSDATAPROC)DataProc,&rp);
			RARHeaderData header;
			memset(&header,0,sizeof(header));
			while(RARReadHeader(hRar,&header)==0)
			{
				rp.setup(&header,!wcsicmp(file_to_extract,header.FileName));
				RARProcessFile(hRar,rp.extract ? RAR_EXTRACT : RAR_SKIP,0,0);//RAR_SKIP RAR_TEST
				if (rp.ptr>0) break;
			};
			RARCloseArchive(hRar);
		}

		r->reader_release();

		mem_init(rp.out.release(),rp.ptr);
		return 1;
	}

};


class archive_rar : public archive_i
{
	virtual const char * get_archive_type() {return "rar";}

	virtual reader * get_reader(const char * path)
	{
		return new reader_rar;
	}

	virtual int precache(const char * path,playlist_loader_callback * callback)
	{
		metadb * p_metadb = metadb::get();

		reader * r = file::g_open(path,reader::MODE_READ);
		if (!r) return 0;

		string8 unpack_path;
		unpack_path.set_mem_logic(mem_block::ALLOC_FAST_DONTGODOWN);

		RARPARAM rp;
		rar_reader_i rr(r);
		rp.extract=1;
		
		RAROpenArchiveData open_data;
		memset(&open_data,0,sizeof(open_data));
		open_data.OpenMode=RAR_OM_EXTRACT;
		open_data.Reader=&rr;
		HANDLE hRar=RAROpenArchive(&open_data);
		int rv=0;
		if (hRar)
		{
			rv=1;
			if (open_data.SolidFlag)
			{
				RARSetProcessDataProc(hRar,(PROCESSDATAPROC)DataProc,&rp);
				RARHeaderData header;
				memset(&header,0,sizeof(header));
				while(RARReadHeader(hRar,&header)==0)
				{
					if (!(header.FileAttr & 0x10))
					{
						make_unpack_path(unpack_path,path,string_utf8_from_wide(header.FileName));
						if (!callback->on_progress(unpack_path)) break;
						rp.setup(&header,!!input::g_test_filename(unpack_path));
						if (rp.extract && p_metadb->query_dbonly(make_playlist_entry(unpack_path,0),0)) break;
						RARProcessFile(hRar,rp.extract ? RAR_EXTRACT : RAR_SKIP,0,0);
						
						if (rp.ptr>0)
							p_metadb->precache(unpack_path,&reader_mem_temp(rp.out.get_ptr(),rp.ptr));
					}
					else RARProcessFile(hRar,RAR_SKIP,0,0);
				};
				RARCloseArchive(hRar);
			}
		}

		r->reader_release();

		return rv;
	}

	virtual int list(const char * path,ptr_list_interface<string_interface> * out,playlist_loader_callback * callback)
	{
		if (stricmp(string_extension_8(path),"RAR")) return 0;

		reader * r = file::g_open(path,reader::MODE_READ);
		if (!r) return 0;

		string8 unpack_path;
		unpack_path.set_mem_logic(mem_block::ALLOC_FAST_DONTGODOWN);

		RARPARAM rp;
		rar_reader_i rr(r);
		
		RAROpenArchiveData open_data;
		memset(&open_data,0,sizeof(open_data));
		open_data.OpenMode=RAR_OM_LIST;
		open_data.Reader=&rr;
		HANDLE hRar=RAROpenArchive(&open_data);
		int rv=0;
		if (hRar)
		{
			rv=1;
			RARSetProcessDataProc(hRar,(PROCESSDATAPROC)DataProc,&rp);
			RARHeaderData header;
			memset(&header,0,sizeof(header));
			while(RARReadHeader(hRar,&header)==0)
			{
				if (!(header.FileAttr & 0x10))
				{
					make_unpack_path(unpack_path,path,string_utf8_from_wide(header.FileName));
					out->add_item(new string_interface_i(unpack_path));
					if (!callback->on_progress(unpack_path)) break;
				}
				RARProcessFile(hRar,RAR_SKIP,0,0);
			};
			RARCloseArchive(hRar);
		}

		r->reader_release();

		return rv;

	}

	virtual int exists_in_archive(const char * archive,const char * file)
	{
		reader * r = file::g_open(archive,reader::MODE_READ);
		if (r==0) return 0;

		string16 file_to_extract = string_wide_from_utf8(file);

		RARPARAM rp;
		rar_reader_i rr(r);
		
		RAROpenArchiveData open_data;
		memset(&open_data,0,sizeof(open_data));
		open_data.OpenMode=RAR_OM_LIST;
		open_data.Reader=&rr;
		HANDLE hRar=RAROpenArchive(&open_data);
		int found = 0;
		if (hRar)
		{
			RARSetProcessDataProc(hRar,(PROCESSDATAPROC)DataProc,&rp);
			RARHeaderData header;
			memset(&header,0,sizeof(header));
			while(RARReadHeader(hRar,&header)==0)
			{
				if (!wcsicmp(file_to_extract,header.FileName)) {found=1;break;}

				RARProcessFile(hRar,RAR_SKIP,0,0);//RAR_SKIP RAR_TEST
			};
			RARCloseArchive(hRar);
		}

		r->reader_release();

		return found ? FILE_EXISTS : 0;
	}

};

class unpacker_rar : public unpacker
{
private:
	virtual reader * open(reader * r)
	{
		DWORD hdr=0;
		r->read(&hdr,4);
		r->seek(0);
		if (hdr!='!raR') return 0;

		RARPARAM rp;
		rar_reader_i rr(r);
		
		RAROpenArchiveData open_data;
		memset(&open_data,0,sizeof(open_data));
		open_data.OpenMode=RAR_OM_EXTRACT;
		open_data.Reader=&rr;
		HANDLE hRar=RAROpenArchive(&open_data);
		if (hRar)
		{
			RARSetProcessDataProc(hRar,(PROCESSDATAPROC)DataProc,&rp);
			RARHeaderData header;
			memset(&header,0,sizeof(header));
			while(RARReadHeader(hRar,&header)==0)
			{
				{
					string8 ext = string_extension_8(string_utf8_from_wide(header.FileName));
					rp.setup(&header,!(!stricmp(ext,"nfo") || !stricmp(ext,"txt")));
				}

				RARProcessFile(hRar,rp.extract ? RAR_EXTRACT : RAR_SKIP,0,0);//RAR_SKIP RAR_TEST
				if (rp.ptr>0) break;
			};
			RARCloseArchive(hRar);
		}

		return new reader_mem(rp.out.release(),rp.ptr);
	}
};


static service_factory_single_t<file,archive_rar> foo1;
static service_factory_single_t<archive,archive_rar> foo2;
static service_factory_t<unpacker,unpacker_rar> foo3;

DECLARE_COMPONENT_VERSION("RAR reader","1.1",0);