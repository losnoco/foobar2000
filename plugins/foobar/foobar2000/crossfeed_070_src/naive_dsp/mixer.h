#ifndef _mixer_h_
#define _mixer_h_

namespace naive_dsp
{
    class mixer
    {
    public:
        mixer();
        void set_levels(float ch1_level, float ch2_level);
        double process(double ch1_sample, double ch2_sample);

    private:
        double m_ch1_level;
        double m_ch2_level;
    };
}

#endif // _mixer_h_