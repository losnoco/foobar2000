#include "stdafx.h"
#include "mpeg_decoder.h"


static const GUID g_mpglib_guid = 
{ 0x97b368f5, 0xf449, 0x4f74, { 0x9d, 0x34, 0xdd, 0xff, 0x14, 0x0, 0xa0, 0x6b } };

#if 0
// {E3929D66-F581-4f8c-994C-077F21C30B15}
static const GUID guid_cfg_mp3dec = 
{ 0xe3929d66, 0xf581, 0x4f8c, { 0x99, 0x4c, 0x7, 0x7f, 0x21, 0xc3, 0xb, 0x15 } };

cfg_guid cfg_mp3dec(guid_cfg_mp3dec,g_mpglib_guid);
#else
#define cfg_mp3dec g_mpglib_guid
#endif

unsigned get_frame_size_dword(unsigned long newhead);
static unsigned get_frame_size(const void * src) {return get_frame_size_dword(byte_order::dword_be_to_native(*(DWORD*)src));}


class packet_decoder_mp3 : public packet_decoder
{
	service_ptr_t<mpeg_decoder> m_decoder;

	bool init(file_info & info,bool is_mp2)
	{
		m_decoder.release();
		if (!mpeg_decoder::g_create(m_decoder,cfg_mp3dec))
		{
			if (!mpeg_decoder::g_create_any(m_decoder))
				return false;
		}
		info.info_set("codec",is_mp2 ? "MP2" : "MP3");
		return true;
	}

public:
	packet_decoder_mp3()
	{
	}

	~packet_decoder_mp3()
	{
	}
	
	virtual unsigned get_max_frame_dependency() {return 10;}
	virtual double get_max_frame_dependency_time() {return 10.0 * 1152.0 / 32000.0;}

	virtual bool open_stream(const GUID & owner,unsigned param1,const void * param2,unsigned param2size,file_info & info)
	{
		if (owner == owner_MP3)
		{
			return init(info,false);
		}
		else if (owner == owner_MP2)
		{
			return init(info,true);
		}
		else if (owner == owner_MP4)
		{
			switch(param1)
			{
			case 0x6B:
				return init(info,false);
			case 0x69:
				return init(info,false);
			default:
				return false;
			}
		}
		else if (owner == owner_matroska)
		{
			if (param2size==sizeof(matroska_setup))
			{
				const matroska_setup * setup = (const matroska_setup*) param2;
				if (!strcmp(setup->codec_id,"A_MPEG/L3"))
				{
					return init(info,false);
				}
				else if (!strcmp(setup->codec_id,"A_MPEG/L2"))
				{
					return init(info,true);
				}
				else return false;
			}
			else return false;
		}
		else return false;
	}

	virtual void reset_after_seek()
	{
		if (m_decoder.is_valid())
		{
			m_decoder->reset_after_seek();
		}
	}

	virtual bool decode(const void * buffer,unsigned bytes,audio_chunk * out)
	{
		if (m_decoder.is_empty()) return false;
		return m_decoder->decode(buffer,bytes,out);
	}

	virtual const char * get_name() {return "MPEG layer 3";}
};

static packet_decoder_factory<packet_decoder_mp3> g_packet_decoder_mp3_factory;