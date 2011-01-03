#include "stdafx.h"

class packet_decoder_pcm : public packet_decoder
{
	unsigned bps,nch,srate;
	unsigned flags;
public:
	packet_decoder_pcm()
	{
		bps=0; nch=0; srate=0;
		flags = 0;
	}

	~packet_decoder_pcm()
	{

	}

	virtual unsigned get_max_frame_dependency() {return 0;}
	virtual double get_max_frame_dependency_time() {return 0;}

	virtual bool open_stream(const GUID & owner,unsigned param1,const void * param2,unsigned param2size,file_info & info)
	{
		if (owner==owner_matroska)
		{
			if (param2size==sizeof(matroska_setup))
			{
				const matroska_setup * setup = (const matroska_setup*) param2;
				if (!strcmp(setup->codec_id,"A_PCM/INT/LIT"))
				{
					nch = setup->channels;
					srate = setup->sample_rate;
					flags = audio_chunk::FLAG_LITTLE_ENDIAN;
					return true;
				}
				else if (!strcmp(setup->codec_id,"A_PCM/INT/BIG"))
				{
					nch = setup->channels;
					srate = setup->sample_rate;
					flags = audio_chunk::FLAG_BIG_ENDIAN;
					return true;
				}
				else return false;
			}
			else return false;
		}
		else return false;
	}

	virtual unsigned set_stream_property(const GUID & type,unsigned param1,const void * param2,unsigned param2size,file_info & info)
	{
		if (type==property_samplerate)
		{
			srate = param1;
		}
		else if (type==property_bitspersample)
		{
			bps = param1;
		}
		else if (type==property_channels)
		{
			nch = param1;
		}
		else if (type==property_signed)
		{
			flags = (flags & ~(audio_chunk::FLAG_SIGNED|audio_chunk::FLAG_UNSIGNED)) | (param1 ? audio_chunk::FLAG_SIGNED : audio_chunk::FLAG_UNSIGNED);
		}
		else if (type==property_byteorder)
		{
			flags = (flags & ~(audio_chunk::FLAG_LITTLE_ENDIAN|audio_chunk::FLAG_BIG_ENDIAN)) | (param1 ? audio_chunk::FLAG_LITTLE_ENDIAN : audio_chunk::FLAG_BIG_ENDIAN);
		}
		return 0;
	}

	virtual void reset_after_seek()
	{
	}

	virtual bool decode(const void * buffer,unsigned bytes,audio_chunk * out)
	{
		if (!bps || !nch || !srate) return false;

		if (!(flags & (audio_chunk::FLAG_LITTLE_ENDIAN|audio_chunk::FLAG_BIG_ENDIAN))) flags |= audio_chunk::flags_autoendian();
		if (!(flags & (audio_chunk::FLAG_SIGNED|audio_chunk::FLAG_UNSIGNED))) flags |= bps>8 ? audio_chunk::FLAG_SIGNED : audio_chunk::FLAG_UNSIGNED;

		return out->set_data_fixedpoint_ex(buffer, bytes, srate, nch, bps, flags, audio_chunk::g_guess_channel_config(nch));
	}

	virtual const char * get_name() {return "PCM";}

};

static packet_decoder_factory<packet_decoder_pcm> meheheheheh;
