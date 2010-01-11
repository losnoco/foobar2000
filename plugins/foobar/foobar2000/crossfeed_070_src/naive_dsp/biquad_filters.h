#ifndef _biquad_filters_h_
#define _biquad_filters_h_

namespace naive_dsp
{
    class biquad_filter
    {
    public:
        biquad_filter();
        double process(double sample);

    protected:
        // coefficients
        double m_a0;
        double m_a1;
        double m_a2;
        double m_b0;
        double m_b1;
        double m_b2;
        // history
        double m_x1;
        double m_x2;
        double m_y1;
        double m_y2;
    };

    class highshelf_filter : public biquad_filter
    {
    public:
        void calculate_coefficients(float f1, float gain, float slope, int sample_rate);
    };
}

#endif // _biquad_filters_h_