#include "stdafx.h"
#include "mpegdec.h"
#include "mad.h"

#if (MAD_VERSION_MAJOR>=1) || \
    ((MAD_VERSION_MAJOR==0) && \
     (((MAD_VERSION_MINOR==14) && \
       (MAD_VERSION_PATCH>=2)) || \
      (MAD_VERSION_MINOR>14)))
#define MadErrorString(x) mad_stream_errorstr(x)
#else
static const char *MadErrorString(const struct mad_stream *Stream)
{
	switch(Stream->error)
	{
		/* Generic unrecoverable errors. */
		case MAD_ERROR_BUFLEN:
			return("input buffer too small (or EOF)");
		case MAD_ERROR_BUFPTR:
			return("invalid (null) buffer pointer");
		case MAD_ERROR_NOMEM:
			return("not enough memory");

		/* Frame header related unrecoverable errors. */
		case MAD_ERROR_LOSTSYNC:
			return("lost synchronization");
		case MAD_ERROR_BADLAYER:
			return("reserved header layer value");
		case MAD_ERROR_BADBITRATE:
			return("forbidden bitrate value");
		case MAD_ERROR_BADSAMPLERATE:
			return("reserved sample frequency value");
		case MAD_ERROR_BADEMPHASIS:
			return("reserved emphasis value");

		/* Recoverable errors */
		case MAD_ERROR_BADCRC:
			return("CRC check failed");
		case MAD_ERROR_BADBITALLOC:
			return("forbidden bit allocation value");
		case MAD_ERROR_BADSCALEFACTOR:
			return("bad scalefactor index");
		case MAD_ERROR_BADFRAMELEN:
			return("bad frame length");
		case MAD_ERROR_BADBIGVALUES:
			return("bad big_values count");
		case MAD_ERROR_BADBLOCKTYPE:
			return("reserved block_type");
		case MAD_ERROR_BADSCFSI:
			return("bad scalefactor selection info");
		case MAD_ERROR_BADDATAPTR:
			return("bad main_data_begin pointer");
		case MAD_ERROR_BADPART3LEN:
			return("bad audio data length");
		case MAD_ERROR_BADHUFFTABLE:
			return("bad Huffman table select");
		case MAD_ERROR_BADHUFFDATA:
			return("Huffman data overrun");
		case MAD_ERROR_BADSTEREO:
			return("incompatible block_type for JS");

		/* Unknown error. This swich may be out of sync with libmad's
		 * defined error codes.
		 */
		default:
			return("Unknown error code");
	}
}
#endif

class mpegdec_mad : public mpegdec
{
	mad_stream	Stream;
	mad_frame	Frame;
	mad_synth	Synth;
public:
	mpegdec_mad()
	{
		mad_stream_init(&Stream);
		mad_frame_init(&Frame);
		mad_synth_init(&Synth);
	}
	virtual ~mpegdec_mad()
	{
		mad_synth_finish(&Synth);
		mad_frame_finish(&Frame);
		mad_stream_finish(&Stream);
	}
	virtual decoder_status process_frame(const void * buf,unsigned bytes,audio_sample * out,unsigned & nch,unsigned & srate,unsigned & samples_done)
	{
		if (buf==0 || bytes==0) return DECODE_NEED_MORE;
		unsigned done;
		mad_stream_buffer(&Stream, (const unsigned char *) buf, bytes);
		Stream.error = MAD_ERROR_NONE;
		if (mad_frame_decode(&Frame, &Stream))
		{
			if (MAD_RECOVERABLE(Stream.error))
			{
				if (Stream.error != MAD_ERROR_BADDATAPTR)
				{
					console::warning(string_printf("recoverable frame-level error (%s)", MadErrorString(&Stream)));
					return DECODE_ERROR;
				}
				else
				{
					// Bit reservoir underrun, most likely from seeking, or a corrupt packet
					return DECODE_NEED_MORE;
				}
			}
			else
			{
				if (Stream.error == MAD_ERROR_BUFLEN)
				{
					return DECODE_NEED_MORE;
				}
				console::warning(string_printf("unrecoverable frame-level error (%s)", MadErrorString(&Stream)));
				return DECODE_ERROR;
			}
		}

		mad_synth_frame(&Synth, &Frame);

		nch = Synth.pcm.channels;
		srate = Synth.pcm.samplerate;
		done = Synth.pcm.length;
		samples_done = done * nch;
		if (nch == 2)
		{
			for (unsigned i = 0; done; done--, i++)
			{
				*(out++) = (audio_sample)(Synth.pcm.samples[0][i]) / (audio_sample)MAD_F_ONE;
				*(out++) = (audio_sample)(Synth.pcm.samples[1][i]) / (audio_sample)MAD_F_ONE;
			}
		}
		else
		{
			for (unsigned i = 0; done; done--, i++)
			{
				*(out++) = (audio_sample)(Synth.pcm.samples[0][i]) / (audio_sample)MAD_F_ONE;
			}
		}

		return DECODE_OK;
	}
	virtual void reset()
	{
		mad_synth_finish(&Synth);
		mad_frame_finish(&Frame);
		mad_stream_finish(&Stream);
		mad_stream_init(&Stream);
		mad_frame_init(&Frame);
		mad_synth_init(&Synth);
	}
};

mpegdec * mpegdec::create_mad() {return new mpegdec_mad;}