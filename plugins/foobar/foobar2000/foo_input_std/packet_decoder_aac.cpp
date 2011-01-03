#include "stdafx.h"
#include <neaacdec.h>

struct t_channel_map { 
	unsigned m_source;
	unsigned m_mask;
};

static int channel_map_compare(const t_channel_map & p_item1,const t_channel_map & p_item2)
{
	return pfc::compare_t(p_item1.m_mask,p_item2.m_mask);
}

class packet_decoder_aac : public packet_decoder
{
    faacDecHandle hDecoder;
    faacDecFrameInfo frameInfo;
	mem_block_t<audio_sample> m_reorder_buffer;
	void cleanup()
	{
		if (hDecoder) {faacDecClose(hDecoder);hDecoder=0;}
	}

	bool init(const void * data,unsigned bytes,file_info & info)
	{
		if (data==0) return false;
		hDecoder = faacDecOpen();
		if (hDecoder == 0)
		{
			cleanup();
			console::error("Failed to open FAAD2 library.");
			return false;
		}

		{
			faacDecConfigurationPtr config;
            config = faacDecGetCurrentConfiguration(hDecoder);
            config->outputFormat = 
#if audio_sample_size == 64
				FAAD_FMT_DOUBLE
#else
				FAAD_FMT_FLOAT
#endif
				;
//			config->dontUpSampleImplicitSBR = 1;
            faacDecSetConfiguration(hDecoder, config);
		}

		unsigned long t_samplerate;
		unsigned char t_channels;

		if(faacDecInit2(hDecoder, (unsigned char*)data, bytes,
						&t_samplerate,&t_channels) < 0)
		{
			faacDecClose(hDecoder);
			hDecoder = 0;
			return false;
		}

		info.info_set_int("samplerate",t_samplerate);
		info.info_set_int("channels",t_channels);

		{
			mp4AudioSpecificConfig mp4ASC;
			if (AudioSpecificConfig((unsigned char*)data, bytes, &mp4ASC) >= 0)
			{
				static const char *ot[6] = { "NULL", "MAIN AAC", "LC AAC", "SSR AAC", "LTP AAC", "HE AAC" };
				info.info_set("aac_profile",ot[(mp4ASC.objectTypeIndex > 5)?0:mp4ASC.objectTypeIndex]);
			}
		}
		
		info.info_set("codec","AAC");

		return true;
	}

	bool init_mka(const matroska_setup * param,file_info & info)
	{
		char codecprivate[5];
		unsigned codecprivate_size = 2;

		char profile, srate_idx;
		const char * name = param->codec_id + 6;
		unsigned samplesPerSec = param->sample_rate;
		unsigned samplesOutputPerSec = param->sample_rate_output;

		if(strstr(name,"/MAIN")) 
			profile = 0;
		else if(strstr(name,"/SBR"))
			profile = -1;
		else if(strstr(name,"/LC"))
			profile = 1;
		else if(strstr(name,"/SSR"))
			profile = 2;
		else if(strstr(name,"/LTP"))
			profile = 3;
		else 
			// Unknown AAC format
			return false;

		if(92017 <= samplesPerSec) srate_idx = 0;
		else if(75132 <= samplesPerSec) srate_idx = 1;
		else if(55426 <= samplesPerSec) srate_idx = 2;
		else if(46009 <= samplesPerSec) srate_idx = 3;
		else if(37566 <= samplesPerSec) srate_idx = 4;
		else if(27713 <= samplesPerSec) srate_idx = 5;
		else if(23004 <= samplesPerSec) srate_idx = 6;
		else if(18783 <= samplesPerSec) srate_idx = 7;
		else if(13856 <= samplesPerSec) srate_idx = 8;
		else if(11502 <= samplesPerSec) srate_idx = 9;
		else if(9391 <= samplesPerSec) srate_idx = 10;
		else srate_idx = 11;

		codecprivate[0] = ((abs(profile) + 1) << 3) | ((srate_idx & 0xe) >> 1);
		codecprivate[1] = ((srate_idx & 0x1) << 7) | ((BYTE)param->channels << 3);

		if(profile < 0)
		{
			codecprivate[0] = ((abs(profile) + 1) << 3) | ((srate_idx & 0xe) >> 1);

			codecprivate_size = 5;

			//pwfe->nSamplesPerSec *= 2;
			//pwfe->nAvgBytesPerSec *= 2;

			if(92017 <= samplesOutputPerSec) srate_idx = 0;
			else if(75132 <= samplesOutputPerSec) srate_idx = 1;
			else if(55426 <= samplesOutputPerSec) srate_idx = 2;
			else if(46009 <= samplesOutputPerSec) srate_idx = 3;
			else if(37566 <= samplesOutputPerSec) srate_idx = 4;
			else if(27713 <= samplesOutputPerSec) srate_idx = 5;
			else if(23004 <= samplesOutputPerSec) srate_idx = 6;
			else if(18783 <= samplesOutputPerSec) srate_idx = 7;
			else if(13856 <= samplesOutputPerSec) srate_idx = 8;
			else if(11502 <= samplesOutputPerSec) srate_idx = 9;
			else if(9391 <= samplesOutputPerSec) srate_idx = 10;
			else srate_idx = 11;

			codecprivate[2] = (unsigned char) ( 0x2B7>>3 );
			codecprivate[3] = (unsigned char) ( (0x2B7<<5) | 5 );
			codecprivate[4] = (unsigned char) ( (1<<7) | (srate_idx<<3) );
		}
		return init(codecprivate,codecprivate_size,info);
	}
public:
	packet_decoder_aac()
	{
		hDecoder = 0;
	}

	~packet_decoder_aac()
	{
		cleanup();
	}
	
	virtual bool is_our_type(const char * name) {return !stricmp_utf8(name,"AAC");}
	
	virtual unsigned get_max_frame_dependency() {return 2;}//1 for LC, 2 for HE
	virtual double get_max_frame_dependency_time() {return 1024.0 / 8000.0;}
	
	virtual bool open_stream(const GUID & owner,unsigned param1,const void * param2,unsigned param2size,file_info & info)
	{
		if (hDecoder)
		{
			faacDecClose(hDecoder);
			hDecoder = 0;
		}
		
		if (owner==owner_MP4)
		{
			switch(param1)
			{
			case 0x40:
			case 0x66:
			case 0x67:
			case 0x68:
				return init(param2,param2size,info);
			default:
				return false;
			}
		}
		else if (owner==owner_matroska)
		{
			if (param2size==sizeof(matroska_setup))
			{
				const matroska_setup * setup = (const matroska_setup*) param2;
				if (!strncmp(setup->codec_id,"A_AAC/",6))
				{
					return init_mka(setup,info);
				}
				else return false;
			}
			else return false;
		}
		else return false;
	}
	
	virtual void reset_after_seek()
	{
		if (hDecoder)
		{
			faacDecPostSeekReset(hDecoder, -1);
		}
	}

	virtual bool decode(const void * buffer,unsigned bytes,audio_chunk * out)
	{
		const audio_sample * faad_sample_buffer;

		faad_sample_buffer = (audio_sample*)faacDecDecode(hDecoder, &frameInfo, (unsigned char*)buffer, bytes);
		
		enum {max_channels = 16};

		if (frameInfo.error > 0)
		{
			cleanup();
			console::info(faacDecGetErrorMessage(frameInfo.error));
			return false;
		}

		if (frameInfo.channels<=0 || frameInfo.channels>max_channels)
		{
			cleanup();
			console::info("Internal AAC decoder error.");
			return false;
		}

		if (frameInfo.samples > 0)
		{
#if 0
#define FRONT_CHANNEL_CENTER (1)
#define FRONT_CHANNEL_LEFT   (2)
#define FRONT_CHANNEL_RIGHT  (3)
#define SIDE_CHANNEL_LEFT    (4)
#define SIDE_CHANNEL_RIGHT   (5)
#define BACK_CHANNEL_LEFT    (6)
#define BACK_CHANNEL_RIGHT   (7)
#define BACK_CHANNEL_CENTER  (8)
#define LFE_CHANNEL          (9)
#define UNKNOWN_CHANNEL      (0)

			channel_position[];

		channel_front_left			= 1<<0,
		channel_front_right			= 1<<1,
		channel_front_center		= 1<<2,
		channel_lfe					= 1<<3,
		channel_back_left			= 1<<4,
		channel_back_right			= 1<<5,
		channel_front_center_right	= 1<<6,
		channel_front_center_left	= 1<<7,
		channel_back_center			= 1<<8,
		channel_side_left			= 1<<9,
		channel_side_right			= 1<<10,
		channel_top_center			= 1<<11,
		channel_top_front_left		= 1<<12,
		channel_top_front_center	= 1<<13,
		channel_top_front_right		= 1<<14,
		channel_top_back_left		= 1<<15,
		channel_top_back_center		= 1<<16,
		channel_top_back_right		= 1<<17,

#endif
			t_channel_map channel_map[max_channels];

			const unsigned channels_decoded = frameInfo.channels, samples = frameInfo.samples / frameInfo.channels;
			unsigned channels = 0;
			unsigned channel_mask = 0;
			
			{
				unsigned n;
				for(n=0;n<channels_decoded;n++)
				{
					unsigned newmask = 0;
					switch(frameInfo.channel_position[n])
					{
					case FRONT_CHANNEL_CENTER: newmask = audio_chunk::channel_front_center; break;
					case FRONT_CHANNEL_LEFT: newmask = audio_chunk::channel_front_left; break;
					case FRONT_CHANNEL_RIGHT: newmask = audio_chunk::channel_front_right; break;
					case SIDE_CHANNEL_LEFT: newmask = audio_chunk::channel_side_left; break;
					case SIDE_CHANNEL_RIGHT: newmask = audio_chunk::channel_side_right; break;
					case BACK_CHANNEL_LEFT: newmask = audio_chunk::channel_back_left; break;
					case BACK_CHANNEL_RIGHT: newmask = audio_chunk::channel_back_right; break;
					case BACK_CHANNEL_CENTER: newmask = audio_chunk::channel_back_center; break;
					case LFE_CHANNEL: newmask = audio_chunk::channel_lfe; break;
					default: newmask = 0; break;
					}
					if (newmask != 0 && (channel_mask & newmask) == 0)
					{
						channel_mask |= newmask;
						channel_map[channels].m_source = n;
						channel_map[channels].m_mask = newmask;
						channels++;
					}
				}

				if (channels == 0)
				{
					cleanup();
					console::info("Invalid AAC channel map.");
					return false;
				}

				pfc::sort_t(channel_map,channel_map_compare,channels);
			}

			bool need_reorder = false;
			if (channels_decoded != channels) need_reorder = true;
			else
			{
				unsigned n;
				for(n=0;n<channels_decoded;n++)
				{
					if (channel_map[n].m_source != n) {need_reorder = true; break; }
				}
			}
			
			if (need_reorder)
			{
				audio_sample * reorder_buffer = m_reorder_buffer.check_size( samples * channels ), * reorder_ptr = reorder_buffer;
				const audio_sample * source_ptr = faad_sample_buffer;
				if (out == 0) return false;
				unsigned n, c;
				for(n=0;n<samples;n++)
				{
					for(c=0;c<channels;c++)
					{
						*(reorder_ptr++) = source_ptr[channel_map[c].m_source];
					}
					source_ptr += frameInfo.channels;
				}
				return out->set_data(reorder_buffer,samples,channels,frameInfo.samplerate,channel_mask);
			}
			else
			{
				return out->set_data(faad_sample_buffer,samples,channels,frameInfo.samplerate,channel_mask);
			}
		}
		else
		{
			out->reset();
			return true;
		}
	}
	virtual const char * get_name() {return "MPEG-4 AAC";}
};

static packet_decoder_factory<packet_decoder_aac> AHAHAHAHA;


bool is_valid_aac_decoder_config(const void * data,unsigned bytes)
{
	mp4AudioSpecificConfig mp4ASC;
	return AudioSpecificConfig((unsigned char*)data, bytes, &mp4ASC)>=0;
}