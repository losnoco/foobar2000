// +-------------------------------------------------------------------------------------+
// | Monkee Limiter - Copyrights:  Cmon & Dmon droidinc (c) 2002 |
// +-------------------------------------------------------------------------------------+

#include "../SDK/foobar2000.h"
#include <math.h>
#include <float.h>

static inline float f_max(float v1,float v2)
{	
	return v1>v2 ? v1 : v2;
}

static const float limiter_max = (float)0.9999;

enum
{
	bufPow=8,
	bufLength=1<<bufPow,
	bufMax =	bufLength - 1,
};

class limiter
{
	int bufPos;

	audio_sample backbuffer[bufLength];

	float buffer[bufLength*2];
	
	audio_sample gain,minGainLP;

	bool active;

public:
	limiter()
	{
		bufPos = 0;
		gain		= 1.0f,
		minGainLP	= 1.0f;
		active = false;
	}


	audio_sample process_sample(audio_sample in)
	{
		audio_sample max,out;

		float val = (float)fabs(in);
		
		if (val > limiter_max)
		{
			if (!active) memset(buffer,0,sizeof(buffer));
			active = true;
		}

		{
			
			if (active)
			{
				int offset = bufPos, level = bufPow;
				do
				{
					float * ptr = buffer + bufLength*2 - (2<<level);
					ptr[offset]=val;
					val = f_max(val,ptr[offset^1]);
					offset>>=1;
					level--;
				} while(level>0);
				max = val;
				if (max<=limiter_max) active = false;
			}
			

			backbuffer[bufPos] = in;
			bufPos = (bufPos+1)&bufMax;
			out = backbuffer[bufPos];
		}

		// attenute when getting close to the point of clipping
		audio_sample minGain;

		if(active)
			minGain = limiter_max/max;
		else
			minGain = limiter_max;
			
		
		// Low-pass filter of the control signal
		minGainLP = 0.9f*minGainLP + 0.1f * minGain;

		// choose the lesser of these two control signals
		gain = 0.001f + 0.999f*gain;
		if (minGainLP<gain) gain = minGainLP;
		
		if (fabs(out*gain)>limiter_max) gain = (audio_sample)(limiter_max/fabs(out));
		
		return out * gain;
	}
};


class dsp_monkee : public dsp_i_base
{
	array_t<limiter> m_limiters;
	unsigned m_channels,m_channel_config,m_sample_rate;
	unsigned m_samples_buffered;

	bool flush_data()
	{
		if (m_samples_buffered==0) return false;
		unsigned samples_todo = m_samples_buffered;
		audio_chunk * out = insert_chunk(samples_todo * m_channels);
		
		audio_sample * data = out->check_data_size( samples_todo * m_channels );
		out->set_channels(m_channels,m_channel_config);
		out->set_srate(m_sample_rate);
		out->set_sample_count(samples_todo);

		

		if (m_samples_buffered<bufMax)
		{
			int d = bufMax - m_samples_buffered;
			for(;d>0;d--)
			{
				UINT c;
				for(c=0;c<m_channels;c++)
					m_limiters[c].process_sample(0);
			}
		}
		unsigned n;
		for(n=0;n<samples_todo*m_channels;n++)
		{
			data[n] = m_limiters[n%m_channels].process_sample(0);
		}
		m_samples_buffered = 0;
		return true;
	}

public:
	dsp_monkee()
	{
		m_channels = 0;
		m_channel_config = 0;
		m_sample_rate = 0;
		m_samples_buffered = 0;
	}

	~dsp_monkee()
	{
	}

	static GUID g_get_guid()
	{
		// {11B13A25-C282-41f6-B1F0-CEB7ECBCEAA8}
		static const GUID guid = 
		{ 0x11b13a25, 0xc282, 0x41f6, { 0xb1, 0xf0, 0xce, 0xb7, 0xec, 0xbc, 0xea, 0xa8 } };
		return guid;
	}

	static void g_get_name(string_base & p_out) { p_out = "Advanced Limiter";}

	virtual bool on_chunk(audio_chunk * chunk)
	{
		if (m_limiters.get_size() > 0 && (chunk->get_channels()!=m_channels || chunk->get_srate()!=m_sample_rate || chunk->get_channel_config() != m_channel_config))
		{
			flush_data();
			m_limiters.set_size(0);
		}

		if (m_limiters.get_size() == 0)
		{
			m_sample_rate = chunk->get_srate();
			m_channels = chunk->get_channels();
			m_channel_config = chunk->get_channel_config();
			m_limiters.set_size(m_channels);
			m_samples_buffered=0;
		}

		{
			UINT read_ptr = 0, write_ptr = 0;
			UINT n;
			UINT count = chunk->get_sample_count() * m_channels;
			audio_sample * data = chunk->get_data();
			for(n=0;n<count;n++)
			{
				audio_sample val = m_limiters[n%m_channels].process_sample(data[read_ptr++]);

				if (m_samples_buffered==bufMax)
					data[write_ptr++]=val;
				else if (n%m_channels==m_channels-1)
					m_samples_buffered++;
			}
			chunk->set_sample_count(write_ptr / m_channels);
		}
		return !chunk->is_empty();
	}

	virtual void on_endofplayback()
	{
		flush_data();
	}

	virtual double get_latency()
	{
		return (m_samples_buffered>0) && (m_sample_rate>0) ? (double)m_samples_buffered / (double)m_sample_rate : 0;
	}

	virtual void flush()
	{
		m_limiters.set_size(0);
		m_samples_buffered = 0;
	}
	virtual bool need_track_change_mark() {return false;}

};

static dsp_factory_nopreset_t<dsp_monkee> g_dsp_monkee_factory;



class dsp_hardlimit : public dsp_i_base
{
public:
	static GUID g_get_guid()
	{
		// {CB592C20-2E33-4972-925F-3FBB6BDBA9BF}
		static const GUID guid = 
		{ 0xcb592c20, 0x2e33, 0x4972, { 0x92, 0x5f, 0x3f, 0xbb, 0x6b, 0xdb, 0xa9, 0xbf } };

		return guid;
	}

	static void g_get_name(string_base & p_out) {p_out = "Hard -6dB limiter";}

	virtual bool on_chunk(audio_chunk * chunk)
	{
		static const float limiter_max_05 = (float)(limiter_max - 0.5);
		audio_sample * ptr = chunk->get_data();
		UINT n;
		for(n = chunk->get_data_length();n;n--)
		{
			double val = *ptr;
			if (val < -0.5)
				val = (tanh((val + 0.5) / limiter_max_05) * limiter_max_05 - 0.5);
			else if (val > 0.5)
				val = (tanh((val - 0.5) / limiter_max_05) * limiter_max_05 + 0.5);
			*ptr = (audio_sample) val;

			ptr++;
		}
		return true;
	}

	virtual void flush() {}
	virtual double get_latency() {return 0;}
	virtual bool need_track_change_mark() {return false;}
};
static dsp_factory_nopreset_t<dsp_hardlimit> foo2;