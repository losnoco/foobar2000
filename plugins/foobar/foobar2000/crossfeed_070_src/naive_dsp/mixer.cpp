#include "mixer.h"

using namespace naive_dsp;

mixer::mixer()
{
    set_levels(0, 0);
}

void mixer::set_levels(float ch1_level, float ch2_level)
{
    m_ch1_level = ch1_level;
    m_ch2_level = ch2_level;
}

double mixer::process(double ch1_sample, double ch2_sample)
{
    return (ch1_sample * m_ch1_level) + (ch2_sample * m_ch2_level);
}