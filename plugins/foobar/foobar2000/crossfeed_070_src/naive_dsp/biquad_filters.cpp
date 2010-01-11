#include "math.h"
#include "biquad_filters.h"

using namespace naive_dsp;

biquad_filter::biquad_filter()
{
    m_a0 = m_a1 = m_a2 = m_b0 = m_b1 = m_b2 = m_x1 = m_x2 = m_y1 = m_y2 = 0;
}

double biquad_filter::process(double sample)
{
    // biquad transform function
    double temp = (m_b0 * sample + m_b1 * m_x1 + m_b2 * m_x2 - m_a1 * m_y1 - m_a2 * m_y2) / m_a0;

    // update history
    m_y2 = m_y1;
    m_y1 = temp;
    m_x2 = m_x1;
    m_x1 = sample;

    // return filtered sample
    return temp;
}

void highshelf_filter::calculate_coefficients(float f1, float gain, float slope, int sample_rate)
{
    double omega = 2.0 * 3.141592 * f1 / double(sample_rate);
    double sine = sin(omega);
    double cosine = cos(omega);
    double a = pow(10., (double(gain) / 40));
    double beta = sqrt((pow(a, 2) + 1) / slope - pow(a - 1, 2));

    m_a0 = (a + 1.0) - (a - 1.0) * cosine + beta * sine;
    m_a1 = 2.0 * ((a - 1.0) - (a + 1.0) * cosine);
    m_a2 = (a + 1.0) - (a - 1.0) * cosine - beta * sine;
    m_b0 = a * ((a + 1.0) + (a - 1.0) * cosine + beta * sine);
    m_b1 = -2.0 * a * ((a - 1.0) + (a + 1.0) * cosine);
    m_b2 = a * ((a + 1.0) + (a - 1.0) * cosine - beta * sine);
}