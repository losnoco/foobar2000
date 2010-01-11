// Adjust these as necessary
#include "../SDK/foobar2000.h"
#include "../../../libmad-0.15.1b/mad.h"

#include "../foo_input_std/mpeg_decoder.h"

class mpegdec_mad : public mpeg_decoder
{
	mad_stream	* Stream;
	mad_frame	* Frame;
	mad_synth	* Synth;
public:
	mpegdec_mad()
	{
		Stream = NULL;
		Frame = NULL;
		Synth = NULL;
	}
	~mpegdec_mad()
	{
		reset_after_seek();
	}

	virtual bool decode(const void * buf,unsigned bytes,audio_chunk * chunk)
	{
		if (buf==0 || bytes==0) return false;
		unsigned done;

		if (!Stream)
		{
			Stream = new mad_stream;
			mad_stream_init(Stream);
		}
		if (!Frame)
		{
			Frame = new mad_frame;
			mad_frame_init(Frame);
		}
		if (!Synth)
		{
			Synth = new mad_synth;
			mad_synth_init(Synth);
		}

		mad_stream_buffer(Stream, (const unsigned char *) buf, bytes);
		Stream->error = MAD_ERROR_NONE;
		if (mad_frame_decode(Frame, Stream))
		{
			static const char error_string[] = "unrecoverable frame-level error (%s)";
			if (MAD_RECOVERABLE(Stream->error))
			{
				if (Stream->error != MAD_ERROR_BADDATAPTR)
				{
					console::warning(uStringPrintf(error_string + 2, mad_stream_errorstr(Stream)));
					return false;
				}
				else
				{
					// Bit reservoir underrun, most likely from seeking, or a corrupt packet
					chunk->reset();
					return true;
				}
			}
			else
			{
				if (Stream->error == MAD_ERROR_BUFLEN)
				{
					chunk->reset();
					return true;
				}
				console::warning(uStringPrintf(error_string, mad_stream_errorstr(Stream)));
				return false;
			}
		}

		mad_synth_frame(Synth, Frame);

		unsigned nch = Synth->pcm.channels;
		done = Synth->pcm.length;

		audio_sample * out = chunk->check_data_size(done * nch);
		chunk->set_srate(Synth->pcm.samplerate);
		chunk->set_channels(nch);
		chunk->set_sample_count(done);

		if (nch == 2)
		{
			for (unsigned i = 0; done; done--, i++)
			{
				*(out++) = (audio_sample)(Synth->pcm.samples[0][i]) / (audio_sample)MAD_F_ONE;
				*(out++) = (audio_sample)(Synth->pcm.samples[1][i]) / (audio_sample)MAD_F_ONE;
			}
		}
		else if (nch == 1)
		{
			for (unsigned i = 0; done; done--, i++)
			{
				*(out++) = (audio_sample)(Synth->pcm.samples[0][i]) / (audio_sample)MAD_F_ONE;
			}
		}
		else return false;

		return true;
	}

	virtual void reset_after_seek()
	{
		if (Synth)
		{
			mad_synth_finish(Synth);
			delete Synth;
			Synth = NULL;
		}
		if (Frame)
		{
			mad_frame_finish(Frame);
			delete Frame;
			Frame = NULL;
		}
		if (Stream)
		{
			mad_stream_finish(Stream);
			delete Stream;
			Stream = NULL;
		}
	}

	virtual const char * get_name()
	{
		return "MAD";
	}

	virtual GUID get_guid()
	{
		// {8D5098EF-C7C6-4e4d-9750-4AF98F64F222}
		static const GUID guid = 
		{ 0x8d5098ef, 0xc7c6, 0x4e4d, { 0x97, 0x50, 0x4a, 0xf9, 0x8f, 0x64, 0xf2, 0x22 } };
		return guid;
	}
};

class version_mad : public componentversion
{
public:
	virtual void get_file_name(string_base & out) { out.set_string(core_api::get_my_file_name()); }
	virtual void get_component_name(string_base & out) { out.set_string("MPEG Audio Decoder"); }
	virtual void get_component_version(string_base & out) { out.set_string("0.15.1b"); }
	virtual void get_about_message(string_base & out)
	{
		out = "MPEG Audio Decoder";
		out.add_byte(32);
		out += "0.15.1b";
		out += "\n" "Copyright (C) " MAD_PUBLISHYEAR " " MAD_AUTHOR
			   "\n\nThis plug-in is licensed under the GNU General\nPublic License. See COPYING.TXT.";
	}
};

static service_factory_t<mpeg_decoder,mpegdec_mad> asdfasdfsadf;
static service_factory_single_t<componentversion,version_mad> foo;
