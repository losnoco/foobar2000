#include "stdafx.h"
#include <math.h>

#ifdef _MSC_VER
#pragma warning(disable:4723)
#endif

#pragma pack(push)
#pragma pack(1)

typedef struct
{
	WORD nch;
	t_uint32 smp;
	WORD bps;
	BYTE srate[10];
} AIFF_COMM;

typedef struct
{
	t_uint32 size;
	t_uint32 u1,u2;
	WORD srate;
	BYTE u3,comp;
	//WORD u4,u5
} _8SVX_HDR;

#pragma pack(pop)



#define INFINITY	(sqrt(1.)/0.0)
#define NAN		(sqrt(-1.))

#define EXP_BIAS	16383
#define EXPC_ZERO	0
#define EXPC_NAN	32767
#define EXPC_INF	32767

static double UTdIEEE80 (const unsigned char b[10])

{
  double dv;
  short expc;
  int exp;
  unsigned long mantH, mantL;

  expc = ((short) (b[0] & 0x7F) << 8) | (short) b[1];
  exp = expc - EXP_BIAS;

  mantH = ((unsigned long) b[2] << 24) | ((unsigned long) b[3] << 16) |
          ((unsigned long) b[4] << 8)  | ((unsigned long) b[5]);
  mantL = ((unsigned long) b[6] << 24) | ((unsigned long) b[7] << 16) |
          ((unsigned long) b[8] << 8)  | ((unsigned long) b[9]);

  if (expc == EXPC_ZERO && mantH == 0 && mantL == 0)

    dv = 0.0;

   else if (expc == EXPC_INF)

     if (mantH == 0 && mantL == 0)
       dv = INFINITY;
     else
       dv = NAN;

  else

    dv = ldexp ((double) mantH, exp - 31) +
         ldexp ((double) mantL, exp - 63);

  if (b[0] & 0x80)
    dv = -dv;

  return dv;
}



#define	SIGN_BIT	(0x80)		/* Sign bit for a A-law byte. */
#define	QUANT_MASK	(0xf)		/* Quantization field mask. */
#define	NSEGS		(8)		/* Number of A-law segments. */
#define	SEG_SHIFT	(4)		/* Left shift for segment number. */
#define	SEG_MASK	(0x70)		/* Segment field mask. */


static BYTE inv_bits(BYTE b)
{
	BYTE ret=0;
	UINT n;
	for(n=0;n<8;n++)
	{
		ret<<=1;
		ret|=b&1;
		b>>=1;
	}
	return ret;
}

static int
alaw2linear(
	unsigned char	a_val)
{
	int		t;
	int		seg;

	a_val ^= 0x55;

	t = (a_val & QUANT_MASK) << 4;
	seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
	switch (seg) {
	case 0:
		t += 8;
		break;
	case 1:
		t += 0x108;
		break;
	default:
		t += 0x108;
		t <<= seg - 1;
	}
	return ((a_val & SIGN_BIT) ? t : -t);
}

static int
ulaw2linear(
	unsigned char	u_val)
{
	int		t;

	enum {BIAS = 0x84}; /* Bias for linear code. */

	/* Complement to obtain normal u-law value. */
	u_val = ~u_val;

	/*
	 * Extract and bias the quantization bits. Then
	 * shift up by the segment number and subtract out the bias.
	 */
	t = ((u_val & QUANT_MASK) << 3) + BIAS;
	t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;

	return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}


#define _rev32(X) ((((t_uint32)(X)&0xFF)<<24)|(((t_uint32)(X)&0xFF00)<<8)|(((t_uint32)(X)&0xFF0000)>>8)|(((t_uint32)(X)&0xFF000000)>>24))

static t_uint32 rev32(t_uint32 d) {return _rev32(d);}

static t_uint32 iff_seek_e(const service_ptr_t<file> & r,t_uint32 code,t_filesize max,abort_callback & p_abort)
{
	t_uint32 dw,s;
	if (max<=12+8) throw io_result_error_data;
	r->seek_e(12,p_abort);
	t_filesize pos = r->get_position_e(p_abort);
	for(;;)
	{
		pos+=8;
		if (pos>=max) return 0;
		r->read_object_e(&dw,4,p_abort);
		r->read_object_e(&s,4,p_abort);
		s=byte_order::dword_be_to_native(s);
		if ((signed)s<0) throw io_result_error_data;
		if (s&1) s++;
		if (dw==code)
		{
			if (pos+s>max) s=(t_uint32)(max-pos);
			return s;
		}
		if (pos+s>max) throw io_result_error_data;
		r->seek_e(pos+=s,p_abort);
	}
}

class input_aiff : public input
{
	service_ptr_t<file>  p_reader;
	t_int64 start,length,position;
	unsigned srate,bps,nch,decoded_bps;
	mem_block buffer,decodebuf;

	bool (input_aiff::* p_decodefunc)(unsigned bytes,audio_chunk * out);
	
	bool do_g711(unsigned bytes,audio_chunk * chunk,int (*p_func)(unsigned char val))
	{
		assert(bps==8);
		assert(bytes<=buffer.get_size());

		short * out = (short*)decodebuf.check_size(bytes*2);
		if (out==0) return false;

		const unsigned char * src = (const unsigned char *) buffer.get_ptr();

		unsigned n;
		for(n=0;n<bytes;n++)
		{
			out[n] = p_func(src[n]);
		}
		
		return chunk->set_data_fixedpoint_signed(out,bytes*2,srate,nch,16,audio_chunk::g_guess_channel_config(nch));
	}

	bool decode_pcm_fixedpoint(unsigned bytes,audio_chunk * chunk)
	{
		assert(bytes<=buffer.get_size());
		return chunk->set_data_fixedpoint_ex(buffer,bytes,srate,nch,bps,audio_chunk::FLAG_BIG_ENDIAN | audio_chunk::FLAG_SIGNED, audio_chunk::g_guess_channel_config(nch));
	}

	bool decode_pcm_floatingpoint(unsigned bytes,audio_chunk * chunk)
	{
		assert(bytes<=buffer.get_size());
		assert(bps==32 || bps==64);
		return chunk->set_data_floatingpoint_ex(buffer,bytes,srate,nch,bps,audio_chunk::FLAG_BIG_ENDIAN,audio_chunk::g_guess_channel_config(nch));
	}

	bool decode_alaw(unsigned bytes,audio_chunk * chunk)
	{
		return do_g711(bytes,chunk,alaw2linear);
	}

	bool decode_ulaw(unsigned bytes,audio_chunk * chunk)
	{
		return do_g711(bytes,chunk,ulaw2linear);
	}

public:

	input_aiff()
	{
		p_reader = 0;
		p_decodefunc = 0;
		decoded_bps = 0;
		bps = 0;
		nch = 0;
		srate = 0;
	}
	~input_aiff()
	{
	}

	void _8svx_init_e(bool is_16,t_int64 max,file_info & info,abort_callback & p_abort)
	{
		t_uint32 hdr;
		_8SVX_HDR comm;
		hdr = iff_seek_e(p_reader,_rev32('VHDR'),max,p_abort);
		
		if (hdr==0 || hdr<sizeof(comm) || hdr>=0x100) throw io_result_error_data;
		p_reader->read_object_e(&comm,sizeof(comm),p_abort);

		byte_order::order_be_to_native(&comm.srate,sizeof(comm.srate));

		if (comm.comp!=0) throw io_result_error_data;

		srate = comm.srate;
		bps = is_16 ? 16 : 8;
		nch = 1;

		hdr = iff_seek_e(p_reader,_rev32('NAME'),max,p_abort);
		if (hdr!=0)
		{
			mem_block_t<char> buffer;
			if (buffer.set_size(hdr+1))
			{
				buffer.zeromemory();
				if (io_result_succeeded(p_reader->read_object(buffer.get_ptr(),hdr,p_abort)))
				{
					info.meta_set("title",string_utf8_from_ansi(buffer));
				}
			}			
		}

		hdr = iff_seek_e(p_reader,_rev32('BODY'),max,p_abort);
		
		length = hdr;
		start = p_reader->get_position_e(p_abort);
		p_decodefunc = decode_pcm_fixedpoint;
	}

	void aiff_init_e(bool is_aifc,t_int64 max,abort_callback & p_abort)
	{
		AIFF_COMM comm;

		t_uint32 hdr;
		hdr = iff_seek_e(p_reader,_rev32('COMM'),max,p_abort);
		if (hdr==0 || hdr<(is_aifc ? sizeof(comm) + 4 : sizeof(comm)) || hdr>=0x200) throw io_result_error_data;
		p_reader->read_object_e(&comm,sizeof(comm),p_abort);
		if (is_aifc)
		{
			t_uint32 code;
			p_reader->read_object_e(&code,4,p_abort);
			if (code!=*(t_uint32*)"NONE") throw io_result_error_data;
		}
		byte_order::order_be_to_native(&comm.nch,sizeof(comm.nch));
		byte_order::order_be_to_native(&comm.smp,sizeof(comm.smp));
		byte_order::order_be_to_native(&comm.bps,sizeof(comm.bps));
		//byte_order_helper::order_be_to_native(&comm.srate,sizeof(comm.srate));

		bps = comm.bps;
		nch = comm.nch;

		srate = (unsigned)UTdIEEE80(comm.srate);

//		assert(sizeof(long double)==10);
//		srate = (unsigned) *(long double*)&comm.srate;


		hdr = iff_seek_e(p_reader,_rev32('SSND'),max,p_abort);

		if (hdr<=8) throw io_result_error_data;

		t_uint32 offset,block;
		p_reader->read_bendian_e_t(offset,p_abort);
		p_reader->read_bendian_e_t(block,p_abort);
		if (offset)
		{
			if (offset+8>hdr) throw io_result_error_data;
			p_reader->seek_e(p_reader->get_position_e(p_abort)+offset,p_abort);
		}
		
		length = hdr - 8 - offset;
		start = p_reader->get_position_e(p_abort);
		p_decodefunc = decode_pcm_fixedpoint;
	}

	void au_init_e(t_int64 max,abort_callback & p_abort)
	{
		t_uint32 snd_data_offset, snd_data_size, snd_encoding, snd_srate, snd_channels;
		p_reader->read_bendian_e_t(snd_data_offset,p_abort);
		p_reader->read_bendian_e_t(snd_data_size,p_abort);
		p_reader->read_bendian_e_t(snd_encoding,p_abort);
		p_reader->read_bendian_e_t(snd_srate,p_abort);
		p_reader->read_bendian_e_t(snd_channels,p_abort);


		if (snd_data_offset + snd_data_size > max) throw io_result_error_data;
		start = snd_data_offset;
		length = snd_data_size;
		srate = snd_srate;
		nch = snd_channels;
		switch(snd_encoding)
		{
		case 1:
			bps = 8;
			decoded_bps = 16;
			p_decodefunc = decode_ulaw;			
			break;
		case 2:
			bps = 8;
			p_decodefunc = decode_pcm_fixedpoint;
			break;
		case 3:
			bps = 16;
			p_decodefunc = decode_pcm_fixedpoint;
			break;
		case 4:
			bps = 24;
			p_decodefunc = decode_pcm_fixedpoint;
			break;
		case 5:
			bps=32;
			p_decodefunc = decode_pcm_fixedpoint;
			break;
		case 6:
			bps = 32;
			p_decodefunc = decode_pcm_floatingpoint;
			break;
		case 7:
			bps = 64;
			p_decodefunc = decode_pcm_floatingpoint;
			break;
		case 27:
			bps = 8;
			decoded_bps = 16;
			p_decodefunc = decode_alaw;
			break;
		default:
			throw io_result_error_data;
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

	t_io_result open_internal(const service_ptr_t<file> & r,const playable_location & p_location,file_info & info,abort_callback & p_abort,bool p_decode,bool p_want_info)
	{
		try {
			open_internal_e(r,p_location,info,p_abort,p_decode,p_want_info);
			return io_result_success;
		} catch(t_io_result status) {
			return status;
		}
	}

	void open_internal_e(const service_ptr_t<file> & r,const playable_location & p_location,file_info & info,abort_callback & p_abort,bool p_decode,bool p_want_info)
	{
		if (r.is_empty() || !r->can_seek()) throw io_result_error_data;
		p_reader = r;

		t_filesize max = p_reader->get_size_e(p_abort);
		if (max == filesize_invalid) throw io_result_error_data;

		decoded_bps = 0;
		bps = 0;
		nch = 0;
		srate = 0;
		p_decodefunc = 0;

		t_uint32 hdr = 0;
		p_reader->read_object_e(&hdr,4,p_abort);
		if (hdr==_rev32('FORM'))
		{
			p_reader->read_object_e(&hdr,4,p_abort);
			hdr = byte_order::dword_be_to_native(hdr);

			if (hdr && (max-8)>hdr) max = (t_int64)hdr+8;

			p_reader->read_object_e(&hdr,4,p_abort);

			if (hdr==*(t_uint32*)"AIFF")
			{
				aiff_init_e(false,max,p_abort);
			}
			else if (hdr==*(t_uint32*)"AIFC")
			{
				aiff_init_e(true,max,p_abort);
			}
			else if (hdr==*(t_uint32*)"8SVX")
			{
				_8svx_init_e(false,max,info,p_abort);
			}
			else if (hdr==*(t_uint32*)"16SV")
			{
				_8svx_init_e(true,max,info,p_abort);
			}
			else throw io_result_error_data;
		}
		else if (hdr==*(t_uint32*)".snd")
		{
			au_init_e(max,p_abort);
		}
		else throw io_result_error_data;

		if (srate<1000 || srate>10000000 || bps<1 || bps>64 || nch<1 || nch>256 || (bps%8) ) throw io_result_error_data;

		length -= length % ((bps>>3) * nch);

		if (length<=0) throw io_result_error_data;

		if (p_decodefunc==0) throw io_result_error_data;

		info.set_length( (double)(length / ((bps>>3) * nch)) / (double)srate );
		info.info_set_int("samplerate",srate);
		info.info_set_int("bitspersample",bps);
		info.info_set_int("channels",nch);
		info.info_set_int("bitrate",(srate * bps * nch + 500) / 1000);
		if (decoded_bps) info.info_set_int("decoded_bitspersample",decoded_bps);

		p_reader->seek_e(start,p_abort);

		position = 0;

	}
	
	inline static bool g_test_filename(const char * full_path,const char * extension)
	{
		return !stricmp_utf8(extension,"AIF") || !stricmp_utf8(extension,"AIFF") || !stricmp_utf8(extension,"AIFC") || !stricmp_utf8(extension,"IFF") || !stricmp_utf8(extension,"SVX") || !stricmp_utf8(extension,"SND") || !stricmp_utf8(extension,"AU");
	}
	
	virtual t_io_result set_info(const service_ptr_t<file> & r,const playable_location & p_location,file_info & info,abort_callback & p_abort)
	{
		return io_result_error_data;
	}

	inline static bool g_needs_reader() {return true;}
	
	virtual t_io_result run(audio_chunk * chunk,abort_callback & p_abort)
	{
		if (!p_reader.is_valid() || !p_decodefunc) return io_result_error_generic;
		unsigned delta = (bps>>3) * nch * 1024;
		t_int64 remaining = length - position;
		if (remaining<=0) return io_result_eof;
		if ((t_int64)delta > remaining) delta = (unsigned)remaining;
		if (!buffer.check_size(delta)) return io_result_error_generic;
		t_io_result status = p_reader->read_object(buffer,delta,p_abort);
		if (io_result_failed(status)) return status;
		if (!(this->*p_decodefunc)(delta,chunk)) return io_result_error_generic;
		position += delta;
		return io_result_success;
	}

	virtual t_io_result seek(double seconds,abort_callback & p_abort)
	{
		if (p_reader==0) return io_result_error_generic;
		t_int64 destsample = dsp_util::duration_samples_from_time(seconds,srate);
		t_int64 destbyte = destsample * nch * (bps>>3);
		if (destbyte>length) destbyte = length;
		else if (destbyte<0) destbyte = 0;
		position = destbyte;
		
		return p_reader->seek(position + start,p_abort);
	}

	virtual bool can_seek() {return p_reader.is_valid() ? p_reader->can_seek() : false;}
	
	inline static bool g_is_our_content_type(const char*,const char*) {return false;}

	static GUID g_get_guid()
	{
		// {24B3C9CA-D60B-45fc-A1A1-B1076FE89661}
		static const GUID guid = 
		{ 0x24b3c9ca, 0xd60b, 0x45fc, { 0xa1, 0xa1, 0xb1, 0x7, 0x6f, 0xe8, 0x96, 0x61 } };
		return guid;
	}

	static const char * g_get_name() {return "AIFF decoder";}

	inline static t_io_result g_get_extended_data(const service_ptr_t<file> & p_reader,const playable_location & p_location,const GUID & p_guid,stream_writer * p_out,abort_callback & p_abort) {return io_result_error_data;}

};

static input_factory_t<input_aiff> g_input_aiff_factory;