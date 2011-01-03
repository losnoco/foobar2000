#include "stdafx.h"
#include "input_mp3.h"

#include "CVbriHeader.h"

#include "mp3_seektable.h"

#include "mpglib/mpglib.h"

#include "preferences_mp3.h"

static seektable_manager g_seektable_manager;

enum
{
	MAX_FRAME_DEPENDENCY = 10,
};


static unsigned get_delay(unsigned layer)
{
	switch(layer)
	{
	case 1:
	default:
		assert(0);
		return 0;//unsupported
	case 2:
		return 480+1;
	case 3:
		return 528+1;
	}
}



#define _rv(X) ((((DWORD)(X)&0xFF)<<24)|(((DWORD)(X)&0xFF00)<<8)|(((DWORD)(X)&0xFF0000)>>8)|(((DWORD)(X)&0xFF000000)>>24))


static DWORD make_dword(t_int64 val)
{
	if (val<0) return 0;
	else if (val>0xFFFFFFFF) return 0xFFFFFFFF;
	else return *(DWORD*)&val;
}




static int test_header(DWORD h)
{
	frame fr;
	memset(&fr,0,sizeof(fr));
	return decode_header(&fr,h);
}

static bool test_header2(DWORD h,DWORD prev)
{
	frame fr1,fr2;
	memset(&fr2,0,sizeof(fr2));

	if (!decode_header(&fr1,prev)) return 0;
	if (!decode_header(&fr2,h)) return 0;
	
	return fr1.mpeg25==fr2.mpeg25 && fr1.sampling_frequency==fr2.sampling_frequency && fr1.stereo==fr2.stereo;
}

static unsigned get_samples_per_frame(unsigned long h)
{
	frame fr;
	memset(&fr,0,sizeof(fr));
	if (!decode_header(&fr,h)) return 0;
	return fr.get_sample_count();
}

typedef enum {
    MPEG_2  = 0,
    MPEG_1  = 1,
    MPEG_25 = 2
} MPEG_t;
 
static const int  bitrate_table    [3] [16] = {
    { 0,  8, 16, 24, 32, 40, 48, 56,  64,  80,  96, 112, 128, 144, 160, -1 },
    { 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, -1 },
    { 0,  8, 16, 24, 32, 40, 48, 56,  64,  80,  96, 112, 128, 144, 160, -1 },
};

static const int  samplerate_table [3]  [4] = { 
    { 22050, 24000, 16000, -1 },      /* MPEG 2 */
    { 44100, 48000, 32000, -1 },      /* MPEG 1 */  
    { 11025, 12000,  8000, -1 },      /* MPEG 2.5 */
};

const unsigned char SizeOfEmptyFrame[2][2]=
{
	{17,9},
	{32,17},
};

static const char* version_string  [3] = { "2", "1", "2.5" };

static const unsigned  header_word [3] = { 0xFFF00000, 0xFFF80000, 0xFFE00000 };


#define FRAMES_FLAG     0x0001
#define BYTES_FLAG      0x0002
#define TOC_FLAG        0x0004
#define VBR_SCALE_FLAG  0x0008

#define NUMTOCENTRIES 100


 
/*structure to receive extracted header */
/* toc may be NULL*/
typedef struct
{
  int		h_id;			/* from MPEG header, 0=MPEG2, 1=MPEG1 */
  int		samprate;		/* determined from MPEG header */
  int		flags;			/* from Vbr header data */
  int		frames;			/* total bit stream frames from Vbr header data */
  int		bytes;			/* total bit stream bytes from Vbr header data*/
  int		vbr_scale;		/* encoded vbr scale from Vbr header data*/
  unsigned char	toc[NUMTOCENTRIES];	/* may be NULL if toc not desired*/
  int           headersize;             /* size of VBR header, in bytes */
  int           enc_delay;              /* encoder delay */
  int           enc_padding;            /* encoder paddign added at end of stream */
  int is_vbr;
}   VBRTAGDATA;


const static char	VBRTag[]={"Xing"};
const static char	VBRTag2[]={"Info"};

/*-------------------------------------------------------------*/
static int ExtractI4(unsigned char *buf)
{
	int x;
	/* big endian extract */
	x = buf[0];
	x <<= 8;
	x |= buf[1];
	x <<= 8;
	x |= buf[2];
	x <<= 8;
	x |= buf[3];
	return x;
}

static int GetVbrTag(VBRTAGDATA *pTagData,  unsigned char *buf,unsigned buf_size)
{
	int			i, head_flags;
	int			h_bitrate,h_id, h_mode, h_sr_index;
        int enc_delay = -1,enc_padding = -1; 
	unsigned char * buf_max = buf + buf_size;

	/* get Vbr header data */
	pTagData->flags = 0;

	/* get selected MPEG header data */
	h_id       = (buf[1] >> 3) & 1;
	h_sr_index = (buf[2] >> 2) & 3;
	h_mode     = (buf[3] >> 6) & 3;
        h_bitrate  = ((buf[2]>>4)&0xf);
	h_bitrate = bitrate_table[h_id][h_bitrate];

        /* check for FFE syncword */
        if ((buf[1]>>4)==0xE) 
            pTagData->samprate = samplerate_table[2][h_sr_index];
        else
            pTagData->samprate = samplerate_table[h_id][h_sr_index];
        //	if( h_id == 0 )
        //		pTagData->samprate >>= 1;



	/*  determine offset of header */
	if( h_id )
	{
                /* mpeg1 */
		if( h_mode != 3 )	buf+=(32+4);
		else				buf+=(17+4);
	}
	else
	{
                /* mpeg2 */
		if( h_mode != 3 ) buf+=(17+4);
		else              buf+=(9+4);
	}

	if (strncmp((const char*)buf,VBRTag,4) && strncmp((const char*)buf,VBRTag2,4)) return 0;
	pTagData->is_vbr = !strncmp((const char*)buf,VBRTag,4);

	buf+=4;

	pTagData->h_id = h_id;

	head_flags = pTagData->flags = ExtractI4(buf); buf+=4;      /* get flags */

	if (!(head_flags & FRAMES_FLAG)) return 0;

	if( head_flags & FRAMES_FLAG )
	{
		pTagData->frames   = ExtractI4(buf); buf+=4;
		if (pTagData->frames <= 1) return 0;
	}

	if( head_flags & BYTES_FLAG )
	{
		pTagData->bytes = ExtractI4(buf); buf+=4;
	}

	if( head_flags & TOC_FLAG )
	{
		if( pTagData->toc != NULL )
		{
			for(i=0;i<NUMTOCENTRIES;i++)
				pTagData->toc[i] = buf[i];
		}
		buf+=NUMTOCENTRIES;
	}

	pTagData->vbr_scale = -1;

	if( head_flags & VBR_SCALE_FLAG )
	{
		pTagData->vbr_scale = ExtractI4(buf); buf+=4;
	}

	pTagData->headersize = 
	((h_id+1)*72000*h_bitrate) / pTagData->samprate;

    buf+=21;
	if (buf+2<buf_max)
	{
		enc_delay = (unsigned)buf[0] << 4;
		enc_delay += (unsigned)buf[1] >> 4;
		enc_padding = (unsigned)(buf[1] & 0x0F)<<8;
		enc_padding += (unsigned)buf[2];
	}

	if (enc_delay>=0 && enc_delay<=1152 && enc_padding>=0 && enc_padding <= 2*1152)
	{
		pTagData->enc_delay=enc_delay;
		pTagData->enc_padding=enc_padding;
	}
	else
	{
		pTagData->enc_delay = -1;
		pTagData->enc_padding = -1;
	}

	return 1;       /* success */
}



bool input_mp3::get_dynamic_info(file_info & out,double * timestamp_delta,bool * b_track_change)
{
	if (m_reader_v2.is_valid() && m_reader_v2->is_dynamic_info_enabled())
	{
		*timestamp_delta = 0;
		if (m_reader_v2->get_dynamic_info(out,b_track_change)) return true;
	}

	return m_vbr_handler.on_update(out,timestamp_delta);
}

t_io_result input_mp3::open_internal(const service_ptr_t<file> & r,const playable_location & p_location,file_info & info,abort_callback & p_abort,unsigned p_flags,bool p_decode)
{
	m_decoder.release();
	
	if (p_seektable)
	{
		p_seektable->release();
		p_seektable = 0;
	}

	if (p_decode) m_file = p_location;

	{
		t_io_result code;
		code = r->get_timestamp(m_timestamp,p_abort);
		if (io_result_failed(code)) return code;
	}
	is_seekable = !!r->can_seek();
	noseeking = !is_seekable || !!(p_flags & OPEN_FLAG_NO_SEEKING);

	m_reader = r;

	if (!service_query_t(m_reader_v2,m_reader)) m_reader_v2.release();

	try {
		getinfo_e(info,p_decode,!!(p_flags & OPEN_FLAG_GET_INFO),p_abort);
	} catch(t_io_result code)
	{
		return io_result_failed(code) ? code : io_result_error_generic;
	}


	if (p_decode)
	{
		m_reader->seek(first_frame,p_abort);
		if (!packet_decoder::g_open(m_decoder,is_mp2 ? packet_decoder::owner_MP2 : packet_decoder::owner_MP3,0,0,0,info)) return io_result_error_generic;

		skip_frames = 0;
		skip_samples = skip_start;
		m_vbr_handler.reset();
		eof = false;

	}
	return io_result_success;
}

t_io_result input_mp3::run(audio_chunk * chunk,abort_callback & p_abort)
{
	if (m_decoder.is_empty()) return io_result_error_generic;
	enum {DECODE_OK,DECODE_NEED_MORE,DECODE_ERROR} status = DECODE_ERROR;
	
	unsigned frame = 0;
	do {
		
		try {
			frame = advance_frame_e(skip_frames <= MAX_FRAME_DEPENDENCY,false,p_abort);
		}
		catch(t_io_result code)
		{
			eof = true;
			return io_result_failed(code) ? code : io_result_error_generic;
		}

		if (frame<=0)
		{
#if 0
			if (total_bytes>0 && m_reader->get_position()<total_bytes+first_frame)
			{
				console::warning("File appears to be incomplete:");
				console::info_location(&file);
			}
#endif
			eof = true; return io_result_eof;
		}
		
		

		if (skip_frames <= MAX_FRAME_DEPENDENCY)
		{
			m_tempchunk.reset();
			if (m_decoder->decode(buf,frame,&m_tempchunk))
			{
				status = m_tempchunk.is_empty() ? DECODE_NEED_MORE : DECODE_OK;
			}
			else
			{
				status = DECODE_ERROR;
			}
		}
		else
		{
			status = DECODE_NEED_MORE;

		}


		if (status == DECODE_NEED_MORE && !eof && skip_frames==0)
		{
			struct frame fr;
			memset(&fr,0,sizeof(fr));
			decode_header(&fr,byte_order::dword_be_to_native(*(DWORD*)buf));
			m_tempchunk.reset();
			m_tempchunk.set_srate(fr.get_sample_rate());
			m_tempchunk.set_channels(fr.get_channels());
			m_tempchunk.pad_with_silence(fr.get_sample_count());
			status = DECODE_OK;
		}

		if (status == DECODE_ERROR)
		{
			console::info("error while decoding mpeg frame");
		}

		if (skip_frames>0)
		{
			if (status == DECODE_OK)
				status = DECODE_NEED_MORE;
			skip_frames--;
		}

		if (status == DECODE_OK && skip_samples>0)
		{
			if (skip_samples >= (t_int64)m_tempchunk.get_sample_count())
			{
				skip_samples -= m_tempchunk.get_sample_count();
				m_tempchunk.reset();
				status = DECODE_NEED_MORE;
			}
			else
			{
				skip_samples -= m_tempchunk.skip_first_samples((unsigned)skip_samples);
			}
		}

		if (status == DECODE_OK)
		{
			if (skip_end>0)
			{
				t_int64 max = total_samples - samples_done;
				if (max<0) max=0;
				if (max < (t_int64)m_tempchunk.get_sample_count()) m_tempchunk.set_sample_count((unsigned)max);
				if (m_tempchunk.is_empty()) {eof = true; return io_result_eof; }
			}
		}

	} while(status != DECODE_OK);

	if (status == DECODE_OK)
	{
		m_vbr_handler.on_frame(m_tempchunk.get_duration(),frame * 8);

		samples_done += m_tempchunk.get_sample_count();

		return chunk->copy_from(&m_tempchunk) ? io_result_success : io_result_error_generic;
	}
	else if (status == DECODE_ERROR) return io_result_error_data;
	else return io_result_eof;
}

t_io_result input_mp3::seek(double seconds,abort_callback & p_abort)
{
	if (!is_seekable || noseeking) return io_result_error_generic;
	if (seconds<0) seconds = 0;
	eof = false;

	t_int64 dest_sample = (t_int64)floor(seconds * sample_rate) + skip_start;

	t_int64 dest_frame = dest_sample / samples_per_frame;
#if 0

	if (dest_frame < frames_done) seek_to_start();

	skip_frames = dest_frame - frames_done;
#else
	eof = false;

	{
		t_int64 dest_frame_tab = 0,dest_offset_tab = 0;
		t_int64 dest_frame_safe = dest_frame > MAX_FRAME_DEPENDENCY ? dest_frame - MAX_FRAME_DEPENDENCY : 0;
		if (dest_frame_safe>0) g_seektable_manager.find_entry(m_file.get_path(),m_timestamp,dest_frame,dest_frame_tab,dest_offset_tab);

		frames_done = dest_frame_tab;
		skip_frames = dest_frame - dest_frame_tab;
		
		m_reader->seek(first_frame + dest_offset_tab,p_abort);
		if (m_decoder.is_valid()) m_decoder->reset_after_seek();
	}
#endif
	skip_samples = dest_sample % samples_per_frame;
	samples_done = dest_sample - skip_start;

	m_vbr_handler.reset();

	return io_result_success;
}


void input_mp3::process_lamefix_e(const service_ptr_t<file> & p_temp,unsigned p_delay,t_int64 wanted_total_samples,bool b_rebuild,abort_callback & p_abort)
{
	if (!is_seekable) throw io_result_error_generic;
	t_int64 total_frames_calc = 0;
	bool is_vbr = false;
	int cbr_frame_size = 0;
	seek(0,p_abort);
	mem_block_fast_aggressive_t<t_int64> offsets(1024);
	t_int64 offset = 0;
	DWORD first_header = 0;
	bool b_is_first_frame = true;
	for(;;)
	{
		unsigned new_frame_size;
		new_frame_size = advance_frame_e(false,true,p_abort);
		if (new_frame_size<=0) break;
		total_frames_calc++;
		if (!b_rebuild)
		{
			offset = m_reader->get_position_e(p_abort) - new_frame_size;
		}
		offsets.append(&offset,1);
		if (b_rebuild) offset += new_frame_size;
		if (b_is_first_frame)
		{
			first_header = *(DWORD*)buf;
			cbr_frame_size = new_frame_size;
			b_is_first_frame = false;
		}
		else if (!is_vbr && abs((int)(cbr_frame_size-new_frame_size))>1) is_vbr = true;
	}
	if (total_frames_calc==0) throw io_result_error_data;

	memset(buf,0,sizeof(buf));
	unsigned pos=0;
	*(DWORD*)(buf+pos)=first_header;

	
	frame first_frame_info;
	memset(&first_frame_info,0,sizeof(first_frame_info));
	decode_header(&first_frame_info,byte_order::dword_be_to_native(first_header));
	
	{
		unsigned first_header_size = get_frame_size(&first_header);

		*(DWORD*)buf = first_header;

		BYTE tb = buf[1]&0xF0;
		UINT c_mod=(first_header&0xC0000000)>>30;
		UINT ver = (first_header&0x800)>>11;
	
		if (ver==1) {
		  buf[1]=tb| 0x0b;
		  tb = buf[2] & 0x0c;   
		  buf[2]= 0x50 | tb;     /* 64kbs MPEG1 frame */
		}else{
		  buf[1]=tb | 0x03;    
		  tb = buf[2] & 0x0c;   
		  buf[2]= 0x80 | tb;     /* 64kbs MPEG2 frame */
		}

		unsigned header_size = get_frame_size(buf);
		if (!is_vbr && header_size < first_header_size)
		{
			*(DWORD*)buf = first_header;
		}

		pos=SizeOfEmptyFrame[ver][c_mod==3 ? 1 : 0]+4;
	}

	unsigned header_size = get_frame_size(buf);

	*(DWORD*)(buf+pos)=is_vbr ? _rv('Xing') : _rv('Info');
	pos+=4;

	*(DWORD*)(buf+pos)=_rv(7);	//flags
	pos+=4;

	*(DWORD*)(buf+pos)=_rv(make_dword(total_frames_calc));
	pos+=4;

	*(DWORD*)(buf+pos)=_rv(make_dword(offset + header_size));
	pos+=4;

	{
		unsigned n;	//TOC
		for(n=0;n<NUMTOCENTRIES;n++)
		{
			unsigned frameptr = (unsigned)( (n * total_frames_calc) / NUMTOCENTRIES);
			unsigned res = (unsigned) ( ( 255 * offsets[frameptr] ) / offset);
			if (res>255) res=255;
			buf[pos++]=res;
		}
	}

	pos += 21;

	{
		unsigned delay = p_delay,padding = 0;
		unsigned decoder_delay = get_delay(first_frame_info.lay);


		if (wanted_total_samples>0)
		{
			t_int64 total_samples_in_file = (t_int64)get_samples_per_frame(byte_order::dword_be_to_native(first_header)) * total_frames_calc;
			if (delay>1152) {console::warning("mp3 header fix: delay value too big");delay = 1152;}
			t_int64 padding64 = total_samples_in_file - wanted_total_samples - delay;
			if (padding64<(t_int64)decoder_delay) {console::warning("mp3 header fix: padding value too small");padding64 = decoder_delay;}
			else if (padding64 > 2*1152) {console::warning("mp3 header fix: padding value too big");padding64 = 2*1152;}
			padding = (unsigned)padding64;
		}
		else padding = decoder_delay;
		
		unsigned char * out = (unsigned char*)buf+pos;
		out[0] = (unsigned char)((delay & 0xFF0) >> 4);
		out[1] = (unsigned char)((delay & 0xF) << 4) | (unsigned char)((padding & 0xF00) >> 8);
		out[2] = (unsigned char)(padding & 0xFF);
	}
	if (id3v2_size>0)
	{
		m_reader->seek_e(0,p_abort);
		file::g_transfer_object_e(m_reader.get_ptr(),p_temp.get_ptr(),id3v2_size,p_abort);
	}
	p_temp->write_object_e(buf,header_size,p_abort);
	m_reader->seek_e(first_frame,p_abort);
	frames_done = 0;
	if (b_rebuild)
	{
		t_int64 cur_offset = first_frame;
		for(;;)
		{
			cur_offset = m_reader->get_position_e(p_abort);
			unsigned framesize;
			framesize = advance_frame_e(true,true,p_abort);
			if (framesize<=0) break;
			p_temp->write_object_e(buf,framesize,p_abort);
		}
		m_reader->seek_e(cur_offset,p_abort);
	}
	{
		t_filesize pos = m_reader->get_position_e(p_abort);
		t_filesize len = m_reader->get_size_e(p_abort);
		if (pos<len)
			file::g_transfer_object_e(m_reader.get_ptr(),p_temp.get_ptr(),len-pos,p_abort);
	}

}



void input_mp3::getinfo_e(file_info & info,bool b_full_open,bool b_want_tags,abort_callback & p_abort)
{
	t_int64 first=-1;

	is_vbr = false;
	int cbr_framesize = -1;

	{
		t_io_result status = tag_processor_id3v2::g_skip(m_reader,id3v2_size,p_abort);
		if (io_result_failed(status)) {id3v2_size=0;throw status;}
	}
	
	mp3header header;

	first = scan_for_frame_e(m_reader,&header,p_abort);

	if (first<0)
	{
		m_reader->seek_e(0,p_abort);
		throw io_result_error_data;
	}

	frame fr;
	memset(&fr,0,sizeof(fr));
	if (!decode_header(&fr,header)) throw io_result_error_data;

	is_mp2 = fr.lay == 2;
	unsigned decoder_delay = get_delay(fr.lay);
	
	if (is_seekable)
	{
		m_reader->seek_e(first,p_abort);
		memset(buf,0,sizeof(buf));
		m_reader->read_object_e(buf,4,p_abort);
		
		int hdr_size = header.get_frame_size();

		m_reader->read_object_e(buf+4,hdr_size-4,p_abort);
		m_reader->seek_e(first,p_abort);

		CVbriHeader vbri_header;

		VBRTAGDATA vbr_tag;
		memset(&vbr_tag,0,sizeof(vbr_tag));

		skip_start = decoder_delay;

		if (GetVbrTag(&vbr_tag,(unsigned char*)buf,hdr_size) && vbr_tag.frames>0)
		{
			total_frames = vbr_tag.frames;
			if (vbr_tag.bytes>0)
			{
				total_bytes = vbr_tag.bytes;
			}
			else
			{
				t_int64 last = find_end_of_stream_e(first,header,p_abort);
				if (last<=first) throw io_result_error_data;
				total_bytes = last - first;
			}

			if (vbr_tag.is_vbr) is_vbr=true;
			cbr_framesize = (int)(total_bytes / total_frames);
			if (vbr_tag.enc_delay>=0) info.info_set_int("enc_delay",vbr_tag.enc_delay);
			if (vbr_tag.enc_padding>=0) info.info_set_int("enc_padding",vbr_tag.enc_padding);
			if (vbr_tag.enc_delay>=0 && vbr_tag.enc_padding>=0)
			{
				skip_start = vbr_tag.enc_delay + decoder_delay;
				skip_end = vbr_tag.enc_padding - decoder_delay;
			}
			if (skip_end<0) skip_end = 0;
			first += hdr_size;
			total_bytes -= hdr_size;
			info.info_set("mp3_accurate_length","yes");
			have_accurate_length = true;
		}
		else if (!vbri_header.readVbriHeader((unsigned char*)buf))
		{
			{
				t_int64 last = find_end_of_stream_e(first,header,p_abort);
				if (last<=first) throw io_result_error_data;
				total_bytes = last - first;
			}
			total_frames = vbri_header.totalFrames();
			is_vbr = true;
			cbr_framesize = (int)(total_bytes / total_frames);
			first += hdr_size;
			total_bytes -= hdr_size;
		}
		else
		{//assume CBR
			{
				t_int64 last = find_end_of_stream_e(first,header,p_abort);
				if (last<=first) throw io_result_error_data;
				total_bytes = last - first;
			}
			is_vbr=false;
			cbr_framesize = hdr_size;
			if (cbr_framesize==0) throw io_result_error_data;
			total_frames = ( total_bytes /  (t_int64)cbr_framesize  );

		}
		if (total_frames <= 0 || total_bytes<=0) throw io_result_error_data;
		if (is_vbr)
		{
			m_reader->seek_e(first,p_abort);
			m_reader->read_object_e(&header,4,p_abort);
			memset(&fr,0,sizeof(fr));
			if (!decode_header(&fr,header)) throw io_result_error_data;
		}
	}
	else
	{
		m_reader->seek_e(first,p_abort);
		m_reader->read_object_e(&header,4,p_abort);
		memset(&fr,0,sizeof(fr));
		if (!decode_header(&fr,header)) throw io_result_error_data;
		cbr_framesize = header.get_frame_size();
		total_frames = -1;
		total_bytes = -1;
	}

	{

		sample_rate = fr.get_sample_rate();
		samples_per_frame = get_samples_per_frame(header);

		double frame_time = (double)samples_per_frame / (double)sample_rate;

		if (is_seekable)
		{
			total_samples = total_frames * samples_per_frame - (skip_start + skip_end);
			if (total_samples<=0) throw io_result_error_data;
			total_length = (double)total_samples / (double)sample_rate;
			info.info_set_int("bitrate",(t_int64)(((double)total_bytes * 8.0 / (frame_time * total_frames) / 1000.0)+0.5));
		}
		else
		{
			total_length = -1.0;
			info.info_set_int("bitrate",(t_int64) (  ((double)(cbr_framesize * 8)) / (1000.0 * frame_time) + 0.5) );
		}

		info.set_length(total_length);
	
		info.info_set("codec",is_mp2 ? "MP2" : "MP3");
		info.info_set_int("channels",fr.stereo);
		info.info_set_int("samplerate",sample_rate);
		if (is_vbr) info.info_set("extrainfo","VBR");

		if (b_want_tags)
		{
			const char * msg = 0;
			switch(fr.mode)
			{
			case MPG_MD_STEREO:
				msg = "stereo";
				break;
			case MPG_MD_JOINT_STEREO:
				msg = "joint stereo";
				break;
			case MPG_MD_DUAL_CHANNEL:
				msg = "dual channel";
				break;
			case MPG_MD_MONO:
				msg = "mono";
				break;
			}
			if (msg) info.info_set("mp3_stereo_mode",msg);
		}

	}

	if (is_seekable && b_want_tags)
	{
		t_io_result status;
		status = tag_processor::read_id3v2_trailing(m_reader,info,p_abort);
		if (status != io_result_error_not_found && status != io_result_error_data && io_result_failed(status)) throw io_result_error_data;
	}

	first_frame = first;
	frames_done = 0;
	samples_done = 0;
}






t_int64 input_mp3::scan_for_frame_e(const service_ptr_t<file> & r,mp3header * p_header,abort_callback & p_abort)
{
	t_int64 max = r->can_seek() ? 0x100000 : 0x8000;
	t_int64 scan_start = r->get_position_e(p_abort);
	t_int64 ret = -1;
	for(;;)
	{
		unsigned len = r->read_e(buf,sizeof(buf),p_abort);
		if (len<4) break;
		unsigned n;
		for(n=0;n<=len-4;n++)
		{
			mp3header header(buf+n);
			if (test_header(header))
			{
				t_int64 curpos = r->get_position_e(p_abort);
				t_int64 ptr = n + curpos -(t_int64)len;
				mp3header header2;
				r->seek_e(ptr+header.get_frame_size(),p_abort);
				if (r->read_e(&header2,4,p_abort) != 4) break;
				if (test_header2(header2,header))
				{
					ret = ptr;
					if (p_header) *p_header = header;
					break;
				}
				else
				{
					r->seek_e(curpos,p_abort);
				}
			}
		}
		if (ret>=0 || len!=sizeof(buf)) break;
		if ((t_int64)r->get_position_e(p_abort)-scan_start>max)
		{
			break;
		}
		r->seek2_e(-3,SEEK_CUR,p_abort);
	}

	return ret;
}



t_int64 input_mp3::find_end_of_stream_e(t_int64 first,DWORD header,abort_callback & p_abort)
{
	t_int64 last = -1;
	t_int64 total = m_reader->get_size_e(p_abort);
	if (is_seekable)
	{
		t_int64 offset = total;
		for(;;)
		{
			t_int64 delta = offset - first;
			if (delta<=0) break;
			if (delta>sizeof(buf)) delta = sizeof(buf);
			unsigned to_read = (unsigned)delta;
			if (offset == total)
				offset -= delta;
			else
			{
				if (delta>sizeof(buf)-3) delta = sizeof(buf)-3;
				offset -= delta;
			}

			m_reader->seek_e(offset,p_abort);
			m_reader->read_object_e(buf,to_read,p_abort);
			int n;
			for(n=to_read-4;n>=0;n--)
			{
				mp3header cur_hdr(buf+n);
				DWORD cur_hdr_size = cur_hdr.get_frame_size();
				if (cur_hdr_size>0 && test_header2(cur_hdr,header) && cur_hdr_size + offset + n <= total)
				{
					last = n + offset + cur_hdr_size;
					break;
				}
			}
			if (last>=0 || offset<=first) break;
		}
	}
	return last;
}

static void parse_tagtype_internal(const char * p_tagtype,bool & p_have_id3v1,bool & p_have_id3v2,bool & p_have_apev2)
{
	const char * tagtype = p_tagtype;
	while(*tagtype)
	{
		unsigned delta = 0;
		while(tagtype[delta] != 0 && tagtype[delta] != '|') delta++;
		if (delta > 0)
		{
			if (!stricmp_utf8_ex(tagtype,delta,"apev1",infinite) || !stricmp_utf8_ex(tagtype,delta,"apev2",infinite))
				p_have_apev2 = true;
			else if (!stricmp_utf8_ex(tagtype,delta,"id3v1",infinite))
				p_have_id3v1 = true;
			else if (!stricmp_utf8_ex(tagtype,delta,"id3v2",infinite))
				p_have_id3v2 = true;
		}
		tagtype += delta;
		while(*tagtype == '|') tagtype++;
	}
}

void input_mp3::g_parse_tagtype(const file_info & p_info,bool & p_have_id3v1,bool & p_have_id3v2,bool & p_have_apev2)
{
	const char * tagtype;
	tagtype = p_info.info_get("tagtype");
	if (tagtype == 0) tagtype = "";

	parse_tagtype_internal(tagtype,p_have_id3v1,p_have_id3v2,p_have_apev2);
}


t_io_result input_mp3::set_info(const service_ptr_t<file> & p_file,const playable_location & p_location,file_info & p_info,abort_callback & p_abort)
{
    if (!p_file->can_seek()) return io_result_error_generic;
	g_purge_seektable(p_location.get_path());

	bool have_id3v1 = false, have_id3v2 = false, have_apev2 = false;

	const char * tagtype_override;
	tagtype_override = p_info.info_get("tagtype_override");
	if (tagtype_override != 0)
	{
		parse_tagtype_internal(tagtype_override,have_id3v1,have_id3v2,have_apev2);
	}
	else
	{
		if (cfg_mp3_forcetag)
		{
			switch(cfg_mp3_forcetagtype)
			{
			case mp3_tagtype_id3v1:
				have_id3v1 = true;
				break;
			case mp3_tagtype_apev2:
				have_apev2 = true;
				break;
			case mp3_tagtype_apev2_id3v1:
				have_id3v1 = true;
				have_apev2 = true;
				break;
			case mp3_tagtype_id3v2:
				have_id3v2 = true;
				break;
			case mp3_tagtype_id3v2_id3v1:
				have_id3v1 = true;
				have_id3v2 = true;
				break;
			}
		}
		else
		{
			file_info_impl info_temp;
			t_io_result status;
			status = tag_processor::read_id3v2_trailing(p_file,info_temp,p_abort);
			if (io_result_failed(status) && status != io_result_error_data) return status;
			if (io_result_succeeded(status)) g_parse_tagtype(info_temp,have_id3v1,have_id3v2,have_apev2);

			

			if (!have_id3v1 && !have_id3v2 && !have_apev2)
			{
				switch(cfg_mp3_notagaction)
				{
				case mp3_tagtype_id3v1:
					have_id3v1 = true;
					break;
				case mp3_tagtype_apev2:
					have_apev2 = true;
					break;
				case mp3_tagtype_apev2_id3v1:
					have_id3v1 = true;
					have_apev2 = true;
					break;
				case mp3_tagtype_id3v2:
					have_id3v2 = true;
					break;
				case mp3_tagtype_id3v2_id3v1:
					have_id3v1 = true;
					have_id3v2 = true;
					break;
				}

				/*
				have_id3v1 = true;
				if (tag_processor::is_id3v2_processor_available()) have_id3v2 = true;
				else have_apev2 = true;
				*/
			}

			if (have_id3v1 && !have_id3v2 && !have_apev2 && !tag_processor::is_id3v1_sufficient(p_info))
			{
				switch(cfg_mp3_id3v1action)
				{
				case mp3_tagtype_id3v1:
					
					break;
				case mp3_tagtype_id3v2:
					have_id3v1 = false; have_id3v2 = true;
					break;
				case mp3_tagtype_id3v2_id3v1:
					have_id3v2 = true;
					break;
				case mp3_tagtype_apev2:
					have_id3v1 = false; have_apev2 = true;
					break;
				case mp3_tagtype_apev2_id3v1:
					have_apev2 = true;
					break;
				}
			}
		}
	}

	p_info.info_remove("tagtype_override");

	if (!have_id3v1 && !have_id3v2 && !have_apev2)
	{
		p_info.meta_remove_all();
		p_info.reset_replaygain();
	}
	else if (have_id3v1 && !have_id3v2 && !have_apev2)
	{
		tag_processor::truncate_to_id3v1(p_info);
	}

	return tag_processor::write_multi(p_file,p_info,p_abort,have_id3v1,have_id3v2,have_apev2);
	
}


unsigned input_mp3::advance_frame_e(bool read,bool ignorelength,abort_callback & p_abort)
{
	if (have_accurate_length && !ignorelength)
	{
		if (frames_done>=total_frames) return 0;//eof
	}
	else if (total_bytes>0)
	{
		t_int64 pos = m_reader->get_position_e(p_abort);
		if (pos>=0)
		{
			if (pos>=total_bytes+first_frame) return 0;//eof
		}
	}

	bool found_id3_garbage = false;
	unsigned id3_garbage_offset = 0;
	unsigned frame;
	for(;;)
	{
		m_reader->read_object_e(buf,4,p_abort);
		if (!strncmp(buf,"TAG",3))
		{
			t_int64 pos = m_reader->get_position_e(p_abort) - 4;
			if (!found_id3_garbage)
			{
				id3_garbage_offset = (unsigned)pos;
				found_id3_garbage = true;
			}
			if (pos+128>=total_bytes+first_frame) return 0;//eof
			m_reader->seek_e(pos+128,p_abort);
		}
		else
		{
			frame = get_frame_size(buf);
			break;
		}
	}
	if (frame<=4 || frame>sizeof(buf))
	{
		if (!strncmp(buf,"APET",4) || !strncmp(buf,"TAG",3))
		{
			return 0;
		}
		console::warning(uStringPrintf("mpeg stream error at %u bytes, attempting resync",(unsigned)(m_reader->get_position_e(p_abort)-4)));
		m_reader->seek_e(m_reader->get_position_e(p_abort)-3,p_abort);
		t_int64 new_offset;
		try
		{
			new_offset = scan_for_frame_e(m_reader,0,p_abort);
		}
		catch(t_io_result code)
		{
			if (code != io_result_aborted) console::warning("I/O error during mpeg resync");
			throw;
		}
		if (new_offset<=0)
		{
			console::warning("mpeg resync failed");
			return 0;
		}
		m_reader->seek_e(new_offset,p_abort);
		m_reader->read_object_e(buf,4,p_abort);
		frame = get_frame_size(buf);
		if (frame<=4 || frame>sizeof(buf))
		{
			console::warning("internal error");
			return 0;
		}
		if (m_decoder.is_valid()) m_decoder->reset_after_seek();
		console::info("mpeg stream resync successful");
	}
	else if (found_id3_garbage)
	{
		console::info(uStringPrintf("ID3 tag garbage inside stream at %u bytes",id3_garbage_offset));
	}

	if (!noseeking)
	{
		if (!p_seektable)
		{
			p_seektable = g_seektable_manager.make_table(m_file.get_path(),m_timestamp);
		}
		p_seektable->add_entry(frames_done,m_reader->get_position_e(p_abort)-4-first_frame);
	}

	if (read)
	{
		m_reader->read_object_e(buf+4,frame-4,p_abort);
	}
	else
	{
		m_reader->skip_object_e(frame-4,p_abort);
	}
	if (have_accurate_length && frames_done==total_frames) console::info("too many frames");
	frames_done++;
	return frame;
}

input_mp3::~input_mp3()
{
	if (p_seektable) p_seektable->release();
}

input_mp3::input_mp3() : is_vbr(false)
{
	p_seektable = 0;
	have_accurate_length = false;
	id3v2_size = 0;
	skip_start = 0;
	skip_end = 0;
}

void input_mp3::seek_to_start(abort_callback & p_abort)
{
	frames_done = 0;
	samples_done = 0;
	skip_frames = 0;
	skip_samples = skip_start;
	eof = false;
	m_reader->seek(first_frame,p_abort);
	if (m_decoder.is_valid()) m_decoder->reset_after_seek();
}


void input_mp3::g_purge_seektable(const char * p_path)
{
	g_seektable_manager.remove_table(p_path);
}

t_io_result input_mp3::open(const service_ptr_t<file> & p_reader,const playable_location & p_location,file_info & p_info,abort_callback & p_abort,unsigned p_flags)
{
	return open_internal(p_reader,p_location,p_info,p_abort,p_flags,true);
}

t_io_result input_mp3::get_info(const service_ptr_t<file> & p_reader,const playable_location & p_location,file_info & p_info,abort_callback & p_abort)
{
	return open_internal(p_reader,p_location,p_info,p_abort,OPEN_FLAG_GET_INFO,false);
}

bool input_mp3::g_is_our_content_type(const char * url,const char * type)
{
	return !strcmp(type,"audio/mp3") || !strcmp(type,"audio/mpeg") || !strcmp(type,"audio/mpg") || !strcmp(type,"audio/x-mp3") || !strcmp(type,"audio/x-mpeg") || !strcmp(type,"audio/x-mpg");
}

bool input_mp3::g_test_filename(const char * fn,const char * ext) 
{
	if (!stricmp_utf8(ext,"MP3") || !stricmp_utf8(ext,"MP2")) return true;
#if 1
	if (!strncmp(fn,"http://",7))//for extensionless http:// urls
	{
		const char * ptr = strrchr(fn+7,'/'),
				   * dot = ptr ? strchr(ptr,'.') : 0,
				 * query = strchr(fn+7,'?');
		if (!ptr || !strchr(ptr,'.') || (query && dot > query)) return true;
	}
#endif
	return false;
}

static input_factory_t<input_mp3> g_input_mp3_factory;