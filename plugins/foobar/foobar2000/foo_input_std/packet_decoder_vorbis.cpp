#include "stdafx.h"
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

typedef struct tagVORBISFORMAT2
{
	DWORD Channels;
	DWORD SamplesPerSec;
	DWORD BitsPerSample;
	DWORD HeaderSize[3]; // 0: Identification, 1: Comment, 2: Setup
} VORBISFORMAT2, *PVORBISFORMAT2, FAR *LPVORBISFORMAT2;

const int MAXCHANNELS = 6;
const int chmap[MAXCHANNELS][MAXCHANNELS] = {
	{ 0, },					// mono
	{ 0, 1, },				// l, r
	{ 0, 2, 1, },			// l, c, r -> l, r, c
	{ 0, 1, 2, 3, },		// l, r, bl, br
	{ 0, 2, 1, 3, 4, },		// l, c, r, bl, br -> l, r, c, bl, br
	{ 0, 2, 1, 5, 3, 4 }	// l, c, r, bl, br, lfe -> l, r, c, lfe, bl, br
};

class packet_decoder_vorbis : public packet_decoder
{
	vorbis_info m_vi;
	vorbis_comment m_vc;
	vorbis_dsp_state m_vd;
	vorbis_block m_vb;
	
	unsigned m_packetno;

	bool m_bVorbisInitialized;

	mem_block m_init_data[3];
	mem_block_t<audio_sample> m_tempbuffer;

	bool parse_init_data_matroska(const void * source,unsigned source_length)
	{
		BYTE *codecPrivate = (BYTE*)source;
		DWORD lastHeaderSize = source_length - 1;
		
		
		if (*(codecPrivate++)!=2) return false;
		
		DWORD header_sizes[3];
		unsigned n;

		// 3 headers for vorbis
		for(n=0;n<2;n++)
		{
			DWORD currentHeaderSize = 0;
			do{
				currentHeaderSize += *(BYTE *)codecPrivate;
				lastHeaderSize--;
			} while((*codecPrivate++) == 0xFF);
			lastHeaderSize -= currentHeaderSize;
			header_sizes[n] = currentHeaderSize;
		}			
		header_sizes[2] = lastHeaderSize;
		
		{
			
			for(n=0;n<3;n++)
			{
				m_init_data[n].set_data(codecPrivate,header_sizes[n]);
				codecPrivate += header_sizes[n];
			}
		}

		return true;
	}

	bool init(file_info & info)
	{
		cleanup();

		info.info_set("codec","Vorbis");

		vorbis_info_init(&m_vi);
		vorbis_comment_init(&m_vc);

		m_packetno = 0;
		// Build the "Identification Header" packet
		ogg_packet opId;
		memset(&opId, 0, sizeof(ogg_packet));
		opId.packet = (BYTE*)m_init_data[0].get_ptr();
		opId.bytes = m_init_data[0].get_size();
		opId.b_o_s = 1;
		opId.packetno = m_packetno++;
		
		// Build the "Comment Header" packet
		ogg_packet opCmt;
		memset(&opCmt, 0, sizeof(ogg_packet));
		opCmt.packet = (BYTE*)m_init_data[1].get_ptr();
		opCmt.bytes = m_init_data[1].get_size();
		opCmt.b_o_s = 0;
		opCmt.packetno = m_packetno++;
		
		// Build the "Setup Header" packet
		ogg_packet opSetup;
		memset(&opSetup, 0, sizeof(ogg_packet));
		opSetup.packet = (BYTE*)m_init_data[2].get_ptr();
		opSetup.bytes = m_init_data[2].get_size();
		opSetup.b_o_s = 0;
		opSetup.packetno = m_packetno++;
		
		if( (vorbis_synthesis_headerin(&m_vi,&m_vc, &opId) < 0) ||
			(vorbis_synthesis_headerin(&m_vi,&m_vc, &opCmt) < 0) ||
			(vorbis_synthesis_headerin(&m_vi,&m_vc, &opSetup) < 0) )
		{
			vorbis_info_clear(&m_vi);
			vorbis_comment_clear(&m_vc);
			return false;
		}
		
		vorbis_synthesis_init(&m_vd,&m_vi);

		vorbis_block_init(&m_vd,&m_vb);

		m_bVorbisInitialized = true;
		return true;
	}

	void cleanup()
	{
		if (m_bVorbisInitialized)
		{
			vorbis_block_clear(&m_vb);
			vorbis_dsp_clear(&m_vd);
			vorbis_comment_clear(&m_vc);
			vorbis_info_clear(&m_vi);
			m_bVorbisInitialized = false;
			m_packetno = 0;
		}
	}

public:
	packet_decoder_vorbis()
	{
		m_bVorbisInitialized = false;
		m_packetno = 0;
		memset(&m_vi,0,sizeof(m_vi));
		memset(&m_vc,0,sizeof(m_vc));
		memset(&m_vd,0,sizeof(m_vd));
		memset(&m_vb,0,sizeof(m_vb));
	}

	~packet_decoder_vorbis()
	{
		cleanup();
	}
	
	virtual bool is_our_type(const char * name) {return !stricmp_utf8(name,"Vorbis");}
	
	virtual unsigned get_max_frame_dependency() {return 1;}
	virtual double get_max_frame_dependency_time() {return 0.1;}//someone figure correct values ?

	virtual bool open_stream(const GUID & owner,unsigned param1,const void * param2,unsigned param2size,file_info & info)
	{
		if (owner==owner_matroska)
		{
			if (param2size==sizeof(matroska_setup))
			{
				const matroska_setup * setup = (const matroska_setup*) param2;
				if (!strcmp(setup->codec_id,"A_VORBIS"))
				{
					if (!parse_init_data_matroska(setup->codec_private,setup->codec_private_size)) return false;
					return init(info);
				}
				else return false;
			}
			else return false;
		}
		else return false;
	}	
	
	virtual void reset_after_seek()
	{
		if (m_bVorbisInitialized)
		{
			init(file_info_impl());
		}
	}

	virtual bool decode(const void * pSrc,unsigned SrcLength,audio_chunk * out)
	{
		if (!m_bVorbisInitialized) return false;

		ogg_packet op;
		memset(&op,0, sizeof(op));
		op.packet = (unsigned char*)pSrc;
		op.bytes = SrcLength;
		op.b_o_s = 0;
		op.packetno = m_packetno++;
		// XXX : granulpos is not used but it works

		if(vorbis_synthesis(&m_vb,&op) == 0)
		{
//			m_PacketsDecoded++;
			vorbis_synthesis_blockin(&m_vd,&m_vb);
		} else {
			cleanup();
			return false;
		}
		
		float **pcm = NULL;
		int samples = 0;
		unsigned ActualDstLength = 0;

		// **pcm is a multichannel float vector.  In stereo, for
		// example, pcm[0] is left, and pcm[1] is right.  samples is
		// the size of each channel.  Convert the float values
		// (-1.<=range<=1.) to whatever PCM format and write it out

		while( (samples = vorbis_synthesis_pcmout(&m_vd,&pcm)) > 0)
		{
			int i, j;

			if (!m_tempbuffer.check_size((ActualDstLength + samples) * m_vi.channels)) return false;
			
			// Convert sample and remap channels
			for(i = 0; i < m_vi.channels; i++)
			{
				audio_sample *ptr = m_tempbuffer.get_ptr() + (ActualDstLength * m_vi.channels) + i;
				float *mono = pcm[chmap[m_vi.channels-1][i]];
				for(j = 0; j < samples; j++)
				{
					*ptr = mono[j];
					ptr += m_vi.channels;
				}
			}

			ActualDstLength += samples;

			// tell libvorbis how many samples we actually consumed
			vorbis_synthesis_read(&m_vd,samples);
		}

		if (ActualDstLength==0)
		{
			out->reset();
			return true;
		}
		else return out->set_data(m_tempbuffer,ActualDstLength,m_vi.channels,m_vi.rate);
	}

	virtual const char * get_name() {return "Vorbis";}
};

static service_factory_t<packet_decoder,packet_decoder_vorbis> brouhahahaha;
