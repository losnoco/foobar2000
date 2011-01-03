#include "stdafx.h"

#if 0//broken, cant do gapless playback because of bugs in FhG ACM
#include "mpeg_decoder.h"

#include <mmreg.h>
#include <msacm.h>
#include "mpglib/mpglib.h"

#include "acm.h"

unsigned get_frame_size_dword(unsigned long newhead);
static unsigned get_frame_size(const void * src) {return get_frame_size_dword(byte_order_helper::dword_be_to_native(*(DWORD*)src));}

static void alter_bit(void * ptr,unsigned index,bool state)
{
	DWORD temp = byte_order_helper::dword_be_to_native(*(DWORD*)ptr);
	DWORD mask = 1 << index;
	if (state) temp |= mask; else temp &= ~mask;
	*(DWORD*)ptr = byte_order_helper::dword_native_to_be(temp);
}

static bool test_bit(const void * ptr,unsigned index)
{
	DWORD temp = byte_order_helper::dword_be_to_native(*(DWORD*)ptr);
	return !!(temp & (1 << index) );
}

static unsigned extract_int(const void * ptr,unsigned index,unsigned len)
{
	unsigned val = 0;
	unsigned n;
	for(n=0;n<len;n++)
	{
		if (test_bit(ptr,index+n)) val |= 1 << n;
	}
	return val;
}

static void insert_int(void * ptr,unsigned index,unsigned len,unsigned val)
{
	unsigned n;
	for(n=0;n<len;n++)
	{
		alter_bit(ptr,index+n,!!(val & (1<<n)));
	}
}

static void apply_max_padding(void * header)
{
	alter_bit(header,9,true);//padding
	
	unsigned size = 14;
	if (extract_int(header,17,2)==2 && extract_int(header,6,2)==3)//mono + layer2 == cant be >192kbps
		size = 10;

	insert_int(header,12,4,size);
}

class mpegdec_acm : public mpeg_decoder, private acm_converter
{
	bool inited;
	mem_block_t<unsigned char> incoming;

	const unsigned char * framebuffer;	
	unsigned framebuffer_size;
	short tempbuf[1152*2];
	unsigned dec_nch,dec_srate;
	bool b_headerhack;
	virtual UINT acm_read_callback(void * out,UINT size)
	{
		unsigned delta = size;
		if (delta>framebuffer_size) delta = framebuffer_size;
		if (delta>0)
		{
			memcpy(out,framebuffer,delta);
			framebuffer+=delta;
			framebuffer_size-=delta;
		}
		return delta;
	}
public:
	mpegdec_acm()
	{
		inited = false;
		framebuffer = 0;
		framebuffer_size = 0;
	}
	~mpegdec_acm()
	{
	}

	virtual const char * get_name()
	{
		return "ACM";
	}

	virtual GUID get_guid()
	{
		// {6F5F488B-D8EC-4bcf-B594-E6F9CEAD6753}
		static const GUID guid = 
		{ 0x6f5f488b, 0xd8ec, 0x4bcf, { 0xb5, 0x94, 0xe6, 0xf9, 0xce, 0xad, 0x67, 0x53 } };
		return guid;
	}

	virtual bool decode(const void * source,unsigned source_bytes,audio_chunk * out)
	{
		if (!inited)
		{
			if (source==0 || source_bytes<4) return false;
			frame fr;
			memset(&fr,0,sizeof(fr));

			if (!decode_header(&fr,byte_order_helper::dword_be_to_native(*(DWORD*)source))) return false;

			struct
			{
				WAVEFORMATEX wfx;
				WORD blah[6];
			} fmt_src;
			WAVEFORMATEX fmt_dst;
			memset(&fmt_src,0,sizeof(fmt_src));
			memset(&fmt_dst,0,sizeof(fmt_dst));
			dec_nch = fr.get_channels();
			dec_srate = fr.get_sample_rate();
			fmt_src.wfx.wFormatTag = WAVE_FORMAT_MPEGLAYER3;
			fmt_src.wfx.nChannels = dec_nch;
			fmt_src.wfx.nSamplesPerSec = dec_srate;
			fmt_src.wfx.nAvgBytesPerSec = fr.get_bitrate() / 8;
			fmt_src.wfx.nBlockAlign = fr.framesize + 4;
			fmt_src.wfx.wBitsPerSample = 0;
			fmt_src.wfx.cbSize = sizeof(fmt_src) - sizeof(fmt_src.wfx);
			fmt_src.blah[0] = 1;
			fmt_src.blah[1] = 2;
			fmt_src.blah[2] = 0;
			fmt_src.blah[3] = fr.framesize + 4;
			fmt_src.blah[4] = 1;
			fmt_src.blah[5] = 0x571;
/*			switch(fr.lay)
			{
			case 1: fmt_src.fwHeadLayer = ACM_MPEG_LAYER1; break;
			case 2: fmt_src.fwHeadLayer = ACM_MPEG_LAYER2; break;
			case 3: fmt_src.fwHeadLayer = ACM_MPEG_LAYER3; break;
			default:
				return false;
			}
			fmt_src.dwHeadBitrate = 0;
			fmt_src.fwHeadMode = ACM_MPEG_JOINTSTEREO;
			fmt_src.fwHeadModeExt = 0;
			fmt_src.wHeadEmphasis = 0;*/

			fmt_dst.wFormatTag = WAVE_FORMAT_PCM;
			fmt_dst.nChannels = dec_nch;
			fmt_dst.nSamplesPerSec = dec_srate;
			fmt_dst.nAvgBytesPerSec = dec_srate * dec_nch * 2;//16bit
			fmt_dst.nBlockAlign = dec_nch * 2;
			fmt_dst.wBitsPerSample = 16;
			fmt_dst.cbSize = 0;
			if (!acm_init(&fmt_src.wfx,&fmt_dst))
				return false;
			inited = true;
			b_headerhack = false;
		}


		unsigned char * incoming_ptr = incoming.check_size(source_bytes);
		if (!incoming_ptr) return false;
		memcpy(incoming_ptr,source, source_bytes);

		framebuffer = incoming_ptr;
		framebuffer_size = source_bytes;
		

		unsigned done = acm_read(tempbuf,sizeof(tempbuf));
		if (done == 0)
		{
			console::error("blah");
			out->reset();
			return true;
		}

		return out->set_data_fixedpoint(tempbuf,done,dec_srate,dec_nch,16);
	}

	virtual void reset_after_seek()
	{
		if (inited)
		{
			acm_cleanup();
			inited = false;
		}
	}
};

static service_factory_t<mpeg_decoder,mpegdec_acm> asdfasdfasdf;

#endif