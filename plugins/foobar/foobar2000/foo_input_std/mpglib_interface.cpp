#include "stdafx.h"
#include "mpglib/mpglib.h"
#include "mpeg_decoder.h"

static critical_section g_mpglib_init_sync;

class mpeg_decoder_mpglib : public mpeg_decoder
{
	sample tempbuf[2*1152];
	mpglib * p_mp;
	bool init()
	{
		if (p_mp == 0)
		{
			g_mpglib_init_sync.enter();
			p_mp = new mpglib;
			g_mpglib_init_sync.leave();
			return p_mp != 0;
		}
		else return true;
	}
public:
	mpeg_decoder_mpglib()
	{
		p_mp = 0;
	}
	~mpeg_decoder_mpglib()
	{
		if (p_mp) delete p_mp;
	}
	virtual void reset_after_seek()
	{
		if (p_mp) p_mp->reset();
	}
	
	virtual bool decode(const void * buffer,unsigned bytes,audio_chunk * out)
	{
		if (buffer==0 || bytes==0) return false;
		if (!init()) return false;
		int done;
		int val;
		val = p_mp->decode(reinterpret_cast<const unsigned char*>(buffer),bytes,tempbuf,tabsize(tempbuf),&done);
		
		if (val==MP3_OK)
		{
			unsigned nch = p_mp->get_channels();
			if (nch != 1 && nch != 2) return false;
			out->set_data_32(tempbuf,done/nch,nch,p_mp->get_sample_rate());
			return true;
		}
		else if (val==MP3_NEED_MORE)
		{
			out->reset();
			return true;
		}
		else if (val==MP3_ERR)
			return false;
		else 
		{
			assert(0);
			return false;
		}
	}
	
	virtual const char * get_name() {return "mpglib";}
	
	virtual GUID get_guid()
	{
		// {97B368F5-F449-4f74-9D34-DDFF1400A06B}
		static const GUID guid = 
		{ 0x97b368f5, 0xf449, 0x4f74, { 0x9d, 0x34, 0xdd, 0xff, 0x14, 0x0, 0xa0, 0x6b } };
		return guid;
	}
};

static service_factory_t<mpeg_decoder,mpeg_decoder_mpglib> blaaaa;