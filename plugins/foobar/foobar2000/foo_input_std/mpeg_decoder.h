#ifndef _FOOBAR2000_MPEG_DEC_H_
#define _FOOBAR2000_MPEG_DEC_H_

class mpeg_decoder : public service_base
{
public:
	static const GUID class_guid;

	virtual void reset_after_seek()=0;
	virtual bool decode(const void * buffer,unsigned bytes,audio_chunk * out)=0;//may return empty chunk (decoder delay etc), caller must check for it (and not crash on samplerate==0 etc)
	virtual const char * get_name()=0;//for diagnostic purposes, codec list, etc
	virtual GUID get_guid()=0;//for storing user-selected decoder in config

	static bool g_create_any(service_ptr_t<mpeg_decoder> & p_out)
	{
		return service_enum_t<mpeg_decoder>().first(p_out);
	}

	static bool g_create(service_ptr_t<mpeg_decoder> & p_out,const GUID & g)
	{
		service_enum_t<mpeg_decoder> e;
		service_ptr_t<mpeg_decoder> ptr;
		if (e.first(ptr)) do {
			if (g == ptr->get_guid())
			{
				p_out = ptr;
				return true;
			}
		} while(e.next(ptr));
		return false;
	}

	static const char * name_from_guid(const GUID & g)
	{
		const char * name = 0;
		service_ptr_t<mpeg_decoder> ptr;
		if (g_create(ptr,g))
		{
			name = ptr->get_name();
		}
		return name;
	}
};


DECLARE_CLASS_GUID(mpeg_decoder, 0x836e8410,0x42d0,0x46b0,0xa4,0xb3,0xbf,0x7f,0xfb,0xb9,0xa7,0x90 );

#endif