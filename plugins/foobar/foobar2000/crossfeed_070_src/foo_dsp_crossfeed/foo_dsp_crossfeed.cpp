#include "stdlib.h"
#include "../../SDK/foobar2000.h"
#include "../naive_dsp/delay_line.h"
#include "../naive_dsp/mixer.h"
#include "../naive_dsp/biquad_filters.h"
#include "../naive_dsp/crossfeed.h"

using namespace naive_dsp;

inline audio_sample white_noise()
{
    return (audio_sample(rand()) / audio_sample(RAND_MAX)) * 0.000005;
}

class foo_dsp_crossfeed : public dsp_i_base
{
public:
    static GUID g_get_guid()
    {
        static const GUID guid = { 0xa45ebb9f, 0x62b6, 0x4a6f, { 0xa7, 0x30, 0x95, 0x6e, 0xcc, 0x17, 0x8f, 0x2 } };
        return guid;
    }

    static void g_get_name(string_base & p_out)
    {
        p_out = "Crossfeed";
    }

    virtual bool on_chunk(audio_chunk * chunk)
    {
        if (chunk->get_channels() == 2 &&
			chunk->get_channel_config() == (audio_chunk::channel_front_left | audio_chunk::channel_front_right))
        {
            audio_sample *data = chunk->get_data();
            // check for proper sample rate
            if (m_crossfeed && m_sample_rate != chunk->get_srate())
            {
                delete m_crossfeed;
                m_crossfeed = 0;
            }

            // initialize crossfeeder if it isn't already
            if (!m_crossfeed)
            {
                m_crossfeed = new crossfeed(m_sample_rate = chunk->get_srate());
            }
            for (unsigned int i = 0; i < chunk->get_sample_count() * chunk->get_channels(); i += 2)
            {
                m_crossfeed->process(data[i] += white_noise(), data[i + 1] += white_noise());
            }
        }

        return true;
    }

    virtual void flush()
    {
        delete m_crossfeed;
        m_crossfeed = 0;
    }

	virtual double get_latency()
	{
		return 0;
	}

	virtual bool need_track_change_mark()
	{
		return false;
	}

    foo_dsp_crossfeed()
    {
        m_crossfeed = 0;
    }

    ~foo_dsp_crossfeed()
    {
        delete m_crossfeed;
    }

private:
    unsigned int m_sample_rate;
    crossfeed * m_crossfeed;
};

static dsp_factory_nopreset_t<foo_dsp_crossfeed> g_dsp_crossfeed_factory;

DECLARE_COMPONENT_VERSION("Crossfeed", "0.70", "Crossfeed 0.70\nCopyright (c) 2002-2003 Michael Rhoades.\n\nhttp://www.naivesoftware.com");