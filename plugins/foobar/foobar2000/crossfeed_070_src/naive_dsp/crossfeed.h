#ifndef _crossfeed_h_
#define _crossfeed_h_

namespace naive_dsp
{
    class crossfeed
    {
    public:
        crossfeed(int sample_rate);
        void process(double & left_sample, double & right_sample);

    private:
        delay_line m_delay[2];
        mixer m_mixer;
        highshelf_filter m_direct_filter[2];
        highshelf_filter m_crossfeed_filter[2];
    };
}

#endif // _crossfeed_h_