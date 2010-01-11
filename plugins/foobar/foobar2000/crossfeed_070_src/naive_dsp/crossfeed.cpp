#include "delay_line.h"
#include "mixer.h"
#include "biquad_filters.h"
#include "crossfeed.h"

using namespace naive_dsp;

crossfeed::crossfeed(int sample_rate)
{
    m_crossfeed_filter[0].calculate_coefficients(700, -12, 1, sample_rate);
    m_crossfeed_filter[1].calculate_coefficients(700, -12, 1, sample_rate);

    m_direct_filter[0].calculate_coefficients(700, 2.9f, 1, sample_rate);
    m_direct_filter[1].calculate_coefficients(700, 2.9f, 1, sample_rate);

    m_delay[0].set_delay(270, sample_rate);
    m_delay[1].set_delay(270, sample_rate);

    m_mixer.set_levels(0.999969f - 0.27f, 0.27f);
}

void crossfeed::process(float & left_sample, float & right_sample)
{
    // crossfeed signal processing
    double left_crossfeed = m_crossfeed_filter[0].process(m_delay[0].process(left_sample));
    double right_crossfeed = m_crossfeed_filter[1].process(m_delay[1].process(right_sample));

    // direct signal processing
    double left_direct = m_direct_filter[0].process(left_sample);
    double right_direct = m_direct_filter[1].process(right_sample);

    // mix the signals
    left_sample = m_mixer.process(left_direct, right_crossfeed);
    right_sample = m_mixer.process(right_direct, left_crossfeed);
}