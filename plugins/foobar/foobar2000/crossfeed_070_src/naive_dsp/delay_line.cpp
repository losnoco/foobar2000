#include "string.h"
#include "delay_line.h"

using namespace naive_dsp;

delay_line::delay_line()
{
    m_delay_buf = 0;
    m_delay_samples = 0;
}

delay_line::~delay_line()
{
    delete [] m_delay_buf;
}

void delay_line::set_delay(int delay_usecs, int sample_rate)
{
    // calculate sample delay and allocate buffer space
    m_delay_samples = int(double(sample_rate / 1000000.0) * double(delay_usecs));
    m_delay_buf = new double[m_delay_samples];

    // clear buffer space
    memset(m_delay_buf, 0, sizeof(double) * m_delay_samples);
}

double delay_line::process(double sample)
{
    // delayed sample to be returned
    double result = m_delay_buf[m_delay_samples - 1];

    // shift memory and input new sample
    memmove(m_delay_buf + 1, m_delay_buf, sizeof(double) * (m_delay_samples - 1));
    m_delay_buf[0] = sample;

    return result;
}