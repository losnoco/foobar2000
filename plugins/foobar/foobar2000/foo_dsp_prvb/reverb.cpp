#include "../SDK/foobar2000.h"

#include <commctrl.h>

#include "resource.h"

static critical_section cfg_sync;

typedef struct tagReverbConfig
{
	unsigned delay;

	int      mVolLeft;
	int      mVolRight;

	int      VolLeft;
	int      VolRight;

	int      FB_SRC_A;
	int      FB_SRC_B;

	int      IIR_ALPHA;
	int      ACC_COEF_A;
	int      ACC_COEF_B;
	int      ACC_COEF_C;
	int      ACC_COEF_D;
	int      IIR_COEF;
	int      FB_ALPHA;
	int      FB_X;

	int      IIR_DEST_A0;
	int      IIR_DEST_A1;
	int      ACC_SRC_A0;
	int      ACC_SRC_A1;
	int      ACC_SRC_B0;
	int      ACC_SRC_B1;
	int      IIR_SRC_A0;
	int      IIR_SRC_A1;
	int      IIR_DEST_B0;
	int      IIR_DEST_B1;
	int      ACC_SRC_C0;
	int      ACC_SRC_C1;
	int      ACC_SRC_D0;
	int      ACC_SRC_D1;
	int      IIR_SRC_B1;
	int      IIR_SRC_B0;
	int      MIX_DEST_A0;
	int      MIX_DEST_A1;
	int      MIX_DEST_B0;
	int      MIX_DEST_B1;

	int      IN_COEF_L;
	int      IN_COEF_R;
} ReverbConfig;

static const ReverbConfig defaults[] = {
	{ 14320, 32768, 32768, 12800, 12800, 0xE3, 0xA9, 28512, 20392, -17184, 17680, -16656, -22912, 22144, 21184, 0xDFB, 0xB58, 0xD09, 0xA3C, 0xBD9, 0x973, 0xB59, 0x8DA, 0x8D9, 0x5E9, 0x7EC, 0x4B0, 0x6EF, 0x3D2, 0x5EA, 0x31D, 0x31C, 0x238, 0x154, 0xAA, -32768, -32768},
};

static cfg_struct_t<ReverbConfig> cfg_preset("reverb_config", defaults[0]);

class dsp_reverb : public dsp_i_base
{
private:
	mem_block_t<double> reverb;
	mem_block_aligned_t<audio_sample> output_buffer;
	int srate, nch, oldsrate, oldnch;

	ReverbConfig r;
	
	int delay, CurrAddr;
	
	int FB_SRC_A, FB_SRC_B, IIR_DEST_A0, IIR_DEST_A1, ACC_SRC_A0, ACC_SRC_A1, ACC_SRC_B0,
		ACC_SRC_B1, IIR_SRC_A0, IIR_SRC_A1, IIR_DEST_B0, IIR_DEST_B1, ACC_SRC_C0,
		ACC_SRC_C1, ACC_SRC_D0, ACC_SRC_D1, IIR_SRC_B1, IIR_SRC_B0, MIX_DEST_A0,
		MIX_DEST_A1, MIX_DEST_B0, MIX_DEST_B1;
	
	double IIR_ALPHA, ACC_COEF_A, ACC_COEF_B, ACC_COEF_C, ACC_COEF_D, IIR_COEF, FB_ALPHA, FB_X,
		IN_COEF_L, IN_COEF_R;

	double iRVBLeft, iRVBRight, VolLeft, VolRight, mVolLeft, mVolRight;
	
	__inline double g_buffer(int iOff, double *ptr)                          // get_buffer content helper: takes care about wraps
	{
		iOff=(iOff*4)+CurrAddr;
		while(iOff>=delay)		iOff-=delay;
		while(iOff<0)			iOff+=delay;
		return (double)*(ptr+iOff);
	}
	
	////////////////////////////////////////////////////////////////////////
	
	__inline void s_buffer(int iOff,double iVal, double *ptr)                // set_buffer content helper: takes care about wraps
	{
		iOff=(iOff*4)+CurrAddr;
		while(iOff>=delay)		iOff-=delay;
		while(iOff<0)			iOff+=delay;
		*(ptr+iOff)=iVal;
	}
	
	////////////////////////////////////////////////////////////////////////
	
	__inline void s_buffer1(int iOff,double iVal, double *ptr)                // set_buffer (+1 sample) content helper: takes care about wraps
	{
		iOff=(iOff*4)+CurrAddr+1;
		while(iOff>=delay)		iOff-=delay;
		while(iOff<0)			iOff+=delay;
		*(ptr+iOff)=iVal;
	}
	
	////////////////////////////////////////////////////////////////////////
	
	__inline audio_sample MixREVERBLeft(audio_sample INPUT_SAMPLE_L, audio_sample INPUT_SAMPLE_R, double *ptr)
	{
		double ACC0,ACC1,FB_A0,FB_A1,FB_B0,FB_B1;
		
		const double IIR_INPUT_A0 = (g_buffer(IIR_SRC_A0, ptr) * IIR_COEF) + (INPUT_SAMPLE_L * IN_COEF_L);
		const double IIR_INPUT_A1 = (g_buffer(IIR_SRC_A1, ptr) * IIR_COEF) + (INPUT_SAMPLE_R * IN_COEF_R);
		const double IIR_INPUT_B0 = (g_buffer(IIR_SRC_B0, ptr) * IIR_COEF) + (INPUT_SAMPLE_L * IN_COEF_L);
		const double IIR_INPUT_B1 = (g_buffer(IIR_SRC_B1, ptr) * IIR_COEF) + (INPUT_SAMPLE_R * IN_COEF_R);
		
		const double IIR_A0 = (IIR_INPUT_A0 * IIR_ALPHA) + (g_buffer(IIR_DEST_A0, ptr) * (1.f - IIR_ALPHA));
		const double IIR_A1 = (IIR_INPUT_A1 * IIR_ALPHA) + (g_buffer(IIR_DEST_A1, ptr) * (1.f - IIR_ALPHA));
		const double IIR_B0 = (IIR_INPUT_B0 * IIR_ALPHA) + (g_buffer(IIR_DEST_B0, ptr) * (1.f - IIR_ALPHA));
		const double IIR_B1 = (IIR_INPUT_B1 * IIR_ALPHA) + (g_buffer(IIR_DEST_B1, ptr) * (1.f - IIR_ALPHA));
		
		s_buffer1(IIR_DEST_A0, IIR_A0, ptr);
		s_buffer1(IIR_DEST_A1, IIR_A1, ptr);
		s_buffer1(IIR_DEST_B0, IIR_B0, ptr);
		s_buffer1(IIR_DEST_B1, IIR_B1, ptr);
		
		ACC0 = (g_buffer(ACC_SRC_A0, ptr) * ACC_COEF_A) +
			(g_buffer(ACC_SRC_B0, ptr) * ACC_COEF_B) +
			(g_buffer(ACC_SRC_C0, ptr) * ACC_COEF_C) +
			(g_buffer(ACC_SRC_D0, ptr) * ACC_COEF_D);
		ACC1 = (g_buffer(ACC_SRC_A1, ptr) * ACC_COEF_A) +
			(g_buffer(ACC_SRC_B1, ptr) * ACC_COEF_B) +
			(g_buffer(ACC_SRC_C1, ptr) * ACC_COEF_C) +
			(g_buffer(ACC_SRC_D1, ptr) * ACC_COEF_D);
		
		FB_A0 = g_buffer(MIX_DEST_A0 - FB_SRC_A, ptr);
		FB_A1 = g_buffer(MIX_DEST_A1 - FB_SRC_A, ptr);
		FB_B0 = g_buffer(MIX_DEST_B0 - FB_SRC_B, ptr);
		FB_B1 = g_buffer(MIX_DEST_B1 - FB_SRC_B, ptr);
		
		s_buffer(MIX_DEST_A0, ACC0 - (FB_A0 * FB_ALPHA), ptr);
		s_buffer(MIX_DEST_A1, ACC1 - (FB_A1 * FB_ALPHA), ptr);
		
		s_buffer(MIX_DEST_B0, (FB_ALPHA * ACC0) - (FB_A0 * (FB_ALPHA - 1.f)) - (FB_B0 * FB_X), ptr);
		s_buffer(MIX_DEST_B1, (FB_ALPHA * ACC1) - (FB_A1 * (FB_ALPHA - 1.f)) - (FB_B1 * FB_X), ptr);
		
		iRVBLeft  = (g_buffer(MIX_DEST_A0, ptr)+g_buffer(MIX_DEST_B0, ptr))/3.f;
		iRVBRight = (g_buffer(MIX_DEST_A1, ptr)+g_buffer(MIX_DEST_B1, ptr))/3.f;
		
		iRVBLeft  = (iRVBLeft  * VolLeft);
		iRVBRight = (iRVBRight * VolRight);
		
		CurrAddr++;
		if(CurrAddr>=delay) CurrAddr=0;
		
		return (audio_sample)iRVBLeft;
	}
	
	////////////////////////////////////////////////////////////////////////
	
	__inline audio_sample MixREVERBRight(void)
	{
		return (audio_sample)iRVBRight;
	}

	int cnv_offset(int in)
	{
		__int64 temp = ((__int64)in * (__int64)srate) / 22050;
		return (int)temp;
	}

	double cnv_double(int in)
	{
		return (double)in * (1. / 32768.);
	}
	
public:
	dsp_reverb()
	{
		oldsrate = oldnch = nch = CurrAddr = 0;
	}
	
	virtual GUID get_guid()
	{
		// {F6BA3354-306F-4251-AC74-9DE616F2A4C6}
		static const GUID guid = 
		{ 0xf6ba3354, 0x306f, 0x4251, { 0xac, 0x74, 0x9d, 0xe6, 0x16, 0xf2, 0xa4, 0xc6 } };
		return guid;
	}
	
	virtual const char * get_name() { return "Programmable reverb";}

	virtual bool on_chunk(audio_chunk * chunk)
	{
		UINT samples = chunk->get_sample_count();
		nch = chunk->get_channels();
		srate = chunk->get_srate();
		if (!nch || !samples || !srate) return 1;

#define cfgf(a) a = cnv_double(r.##a)
#define cfgo(a) a = cnv_offset(r.##a)

		cfg_sync.enter();
		if (srate != oldsrate || memcmp(&r, &(cfg_preset.get_val()), sizeof(r)))
		{
			r = cfg_preset;

			cfgo(delay);
			cfgf(mVolLeft);
			cfgf(mVolRight);
			cfgf(VolLeft);
			cfgf(VolRight);

			cfgo(FB_SRC_A);
			cfgo(FB_SRC_B);
			cfgf(IIR_ALPHA);
			cfgf(ACC_COEF_A);
			cfgf(ACC_COEF_B);
			cfgf(ACC_COEF_C);
			cfgf(ACC_COEF_D);
			cfgf(IIR_COEF);
			cfgf(FB_ALPHA);
			cfgf(FB_X);
			cfgo(IIR_DEST_A0);
			cfgo(IIR_DEST_A1);
			cfgo(ACC_SRC_A0);
			cfgo(ACC_SRC_A1);
			cfgo(ACC_SRC_B0);
			cfgo(ACC_SRC_B1);
			cfgo(IIR_SRC_A0);
			cfgo(IIR_SRC_A1);
			cfgo(IIR_DEST_B0);
			cfgo(IIR_DEST_B1);
			cfgo(ACC_SRC_C0);
			cfgo(ACC_SRC_C1);
			cfgo(ACC_SRC_D0);
			cfgo(ACC_SRC_D1);
			cfgo(IIR_SRC_B1);
			cfgo(IIR_SRC_B0);
			cfgo(MIX_DEST_A0);
			cfgo(MIX_DEST_A1);
			cfgo(MIX_DEST_B0);
			cfgo(MIX_DEST_B1);
			cfgf(IN_COEF_L);
			cfgf(IN_COEF_R);
		}
		cfg_sync.leave();

		if (nch == 1)
		{
			audio_sample *in = chunk->get_data();
			double *ptr = reverb.check_size(delay);
			if (1 != oldnch || srate != oldsrate)
			{
				oldnch = 1;
				oldsrate = srate;
				reverb.zeromemory();
			}
			audio_sample *out = output_buffer.check_size(samples * 2);
			for (UINT n = samples; n--;)
			{
				*out++ = *in * mVolLeft + MixREVERBLeft(*in, *in, ptr);
				*out++ = *in * mVolRight + MixREVERBRight();
				in++;
			}
			chunk->set_data(output_buffer.get_ptr(), samples, 2, srate);
		}
		else if (nch == 2)
		{
			audio_sample *in = chunk->get_data();
			double *ptr = reverb.check_size(delay);
			if (2 != oldnch || srate != oldsrate)
			{
				oldnch = 2;
				oldsrate = srate;
				reverb.zeromemory();
			}
			audio_sample *out = output_buffer.check_size(samples * 2);
			for (UINT n = samples; n--;)
			{
				*out++ = *in * mVolLeft + MixREVERBLeft(*in, in[1], ptr);
				*out++ = in[1] * mVolRight + MixREVERBRight();
				in += 2;
			}
			chunk->set_data(output_buffer.get_ptr(), samples, 2, srate);
		}
		else if (nch == 4)
		{
			audio_sample *in = chunk->get_data();
			double *ptr = reverb.check_size(delay);
			if (4 != oldnch || srate != oldsrate)
			{
				oldnch = 4;
				oldsrate = srate;
				reverb.zeromemory();
			}
			audio_sample *out = output_buffer.check_size(samples * 4);
			for (UINT n = samples; n--;)
			{
				audio_sample ml = *in + in[2];
				audio_sample dl = *in - in[2];
				audio_sample mr = in[1] + in[3];
				audio_sample dr = in[1] - in[3];
				ml = ml * mVolLeft + MixREVERBLeft(ml, mr, ptr);
				mr = mr * mVolRight + MixREVERBRight();
				*out++ = .5 * (ml + dl);
				*out++ = .5 * (mr + dr);
				*out++ = .5 * (ml - dl);
				*out++ = .5 * (mr - dr);
				in += 4;
			}
			chunk->set_data(output_buffer.get_ptr(), samples, 4, srate);
		}
		return 1;
	}

	virtual void flush() {}
	virtual double get_latency() {return 0;}
	virtual bool need_track_change_mark() {return false;}
};

class dsp_stereo2x_2 : public dsp_i_base
{
	mem_block_t<audio_sample> temp;
public:
	virtual GUID get_guid()
	{
		// {793EB87D-0810-4cc6-AE4F-57ACBFE0E451}
		static const GUID guid = 
		{ 0x793eb87d, 0x810, 0x4cc6, { 0xae, 0x4f, 0x57, 0xac, 0xbf, 0xe0, 0xe4, 0x51 } };

		return guid;
	}

	virtual const char * get_name() {return "Convert stereo to 4 channels (front only)";}

	virtual bool on_chunk(audio_chunk * chunk)
	{
		if (chunk->get_channels() == 2)
		{
			UINT samples = chunk->get_sample_count();
			audio_sample * p_temp = temp.check_size(samples*4),
						   * data = chunk->get_data();
			for(UINT n = samples;n--;)
			{
				*p_temp++ = *data++;
				*p_temp++ = *data++;
				*p_temp++ = 0;
				*p_temp++ = 0;
			}

			chunk->set_data(temp.get_ptr(),samples,4,chunk->get_srate());
		}
		return true;
	}

	virtual void flush() {}
	virtual double get_latency() {return 0;}
	virtual bool need_track_change_mark() {return false;}
};

static service_factory_t<dsp,dsp_reverb> foo;
static service_factory_t<dsp,dsp_stereo2x_2> foo2;

DECLARE_COMPONENT_VERSION("Programmable reverb", "0.5", "Programmable reverb, based on PSX reverb\ncode from the P.E.Op.S. project.\n\nhttp://www.sourceforge.net/projects/peops");
