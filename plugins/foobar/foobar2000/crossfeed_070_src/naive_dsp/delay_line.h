#ifndef _delay_line_h_
#define _delay_line_h_

namespace naive_dsp
{
    class delay_line
    {
    public:
        delay_line();
        ~delay_line();
        void set_delay(int delay_usecs, int sample_rate);
        double process(double sample);

    private:
        double * m_delay_buf;
        int m_delay_samples;
    };
}

#endif // _delay_line_h_