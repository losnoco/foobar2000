// changelog

// 2003-06-26 07:07 - kode54
// - Updated to 0.7 API
// - Version is now 1.2

// 2003-05-16 10:38 - kode54
// - Increased expansion strength range to 800%
// - Version is now 1.11

// 2003-05-16 06:56 - kode54
// - LPF now properly zeroes history and coef arrays after allocating them
// - Version is now 1.1

#include "../SDK/foobar2000.h"

#include "resource.h"

#include <commctrl.h>

#include <math.h>

static cfg_int cfg_lowpass("surr_lowpass", 1), cfg_realtime("surr_realtime", 0),
			   cfg_cutoff("surr_cutoff", 1200), cfg_expstr("surr_expstr", 1647);

#define FILTER_SECTIONS   2   /* 2 filter sections for 24 db/oct filter */

class LPF
{
private:
	typedef struct {
		unsigned int length;       /* size of filter */
		double *history;    /* history in filter */
		double *coef;               /* pointer to coefficients of filter */
	} FILTER;

	typedef struct {
		double a0, a1, a2;       /* numerator coefficients */
		double b0, b1, b2;       /* denominator coefficients */
	} BIQUAD;

	FILTER iir;
	BIQUAD *ProtoCoef;      /* Filter prototype coefficients,
							   1 for each filter section */

public:
	LPF(double cutoff, int samplerate)
	{
		ProtoCoef = new BIQUAD[FILTER_SECTIONS];

		ProtoCoef[0].a0 = 1.0;
		ProtoCoef[0].a1 = 0.;
		ProtoCoef[0].a2 = 0.;
		ProtoCoef[0].b0 = 1.0;
		ProtoCoef[0].b1 = 0.765367;
		ProtoCoef[0].b2 = 1.0;

		ProtoCoef[1].a0 = 1.0;
		ProtoCoef[1].a1 = 0.;
		ProtoCoef[1].a2 = 0.;
		ProtoCoef[1].b0 = 1.0;
		ProtoCoef[1].b1 = 1.847759;
		ProtoCoef[1].b2 = 1.0;

		iir.length = FILTER_SECTIONS;

		iir.history = new double[2 * FILTER_SECTIONS];
		iir.coef = new double[4 * iir.length + 1];

		memset(iir.history, 0, 2 * FILTER_SECTIONS * sizeof(double));
		memset(iir.coef, 0, (4 * iir.length + 1) * sizeof(double));

		update(cutoff, samplerate);
	}

	~LPF()
	{
		delete [] iir.coef;
		delete [] iir.history;
		delete [] ProtoCoef;
	}

	double process(double input)
	{
		unsigned int i;
		double *hist1_ptr,*hist2_ptr,*coef_ptr;
		double output,new_hist,history1,history2;

		/* allocate history array if different size than last call */

		coef_ptr = iir.coef;                /* coefficient pointer */

		hist1_ptr = iir.history;            /* first history */
		hist2_ptr = hist1_ptr + 1;           /* next history */

											 /* 1st number of coefficients array is overall input scale factor,
		* or filter gain */
		output = input * (*coef_ptr++);

		for (i = 0 ; i < iir.length; i++)
		{
			history1 = *hist1_ptr;           /* history values */
			history2 = *hist2_ptr;
			
			output = output - history1 * (*coef_ptr++);
			new_hist = output - history2 * (*coef_ptr++);    /* poles */
			
			output = new_hist + history1 * (*coef_ptr++);
			output = output + history2 * (*coef_ptr++);      /* zeros */
			
			*hist2_ptr++ = *hist1_ptr;
			*hist1_ptr++ = new_hist;
			hist1_ptr++;
			hist2_ptr++;
		}
		
		return(output);
	}

	void update(double cutoff, int samplerate)
	{
		unsigned nInd;
		double   a0, a1, a2, b0, b1, b2;
		double   fs;    /* Sampling frequency, cutoff frequency */
		double   k;           /* overall gain factor */
		double    *coef;

		k = 1.0;				/* Set overall filter gain */
		coef = iir.coef + 1;		/* Skip k, or gain */
		fs = (double)samplerate;    /* Sampling frequency (Hz) */

									/*
									* Compute z-domain coefficients for each biquad section
									* for new Cutoff Frequency and Resonance
		*/
		for (nInd = 0; nInd < iir.length; nInd++)
		{
			a0 = ProtoCoef[nInd].a0;
			a1 = ProtoCoef[nInd].a1;
			a2 = ProtoCoef[nInd].a2;

			b0 = ProtoCoef[nInd].b0;
			b1 = ProtoCoef[nInd].b1;
			b2 = ProtoCoef[nInd].b2;
			szxform(&a0, &a1, &a2, &b0, &b1, &b2, cutoff, fs, &k, coef);
			coef += 4;                       /* Point to next filter section */
		}

		/* Update overall filter gain in coef array */
		iir.coef[0] = k;
	}

private:
	void szxform(
		double *a0, double *a1, double *a2, /* numerator coefficients */
		double *b0, double *b1, double *b2, /* denominator coefficients */
		double fc,         /* Filter cutoff frequency */
		double fs,         /* sampling rate */
		double *k,         /* overall gain factor */
		double *coef)         /* pointer to 4 iir coefficients */
	{
		/* Calculate a1 and a2 and overwrite the original values */
        prewarp(a0, a1, a2, fc, fs);
        prewarp(b0, b1, b2, fc, fs);
        bilinear(*a0, *a1, *a2, *b0, *b1, *b2, k, fs, coef);
	}

	void prewarp(
		double *a0, double *a1, double *a2,
		double fc, double fs)
	{
		double wp, pi;

		pi = 4.0 * atan(1.0);
		wp = 2.0 * fs * tan(pi * fc / fs);

		*a2 = (*a2) / (wp * wp);
		*a1 = (*a1) / wp;
	}

	void bilinear(
		double a0, double a1, double a2,	/* numerator coefficients */
		double b0, double b1, double b2,	/* denominator coefficients */
		double *k,							/* overall gain factor */
		double fs,							/* sampling rate */
		double *coef							/* pointer to 4 iir coefficients */
		)
	{
		double ad, bd;

		/* alpha (Numerator in s-domain) */
		ad = 4. * a2 * fs * fs + 2. * a1 * fs + a0;
		/* beta (Denominator in s-domain) */
		bd = 4. * b2 * fs * fs + 2. * b1* fs + b0;

		/* update gain constant for this section */
		*k *= ad/bd;

		/* Denominator */
		*coef++ = (float)((2. * b0 - 8. * b2 * fs * fs)           / bd); /* beta1 */
		*coef++ = (float)((4. * b2 * fs * fs - 2. * b1 * fs + b0) / bd); /* beta2 */

		/* Nominator */
		*coef++ = (float)((2. * a0 - 8. * a2 * fs * fs) / ad);			 /* alpha1 */
		*coef   = (float)((4. * a2 * fs * fs - 2. * a1 * fs + a0) / ad); /* alpha2 */
	}
};

class dsp_phase : public dsp_i_base
{
private:
	UINT srate;
	int cutoff;
	LPF * lpf;

public:
	dsp_phase()
	{
		srate = 0;
		cutoff = 0;
		lpf = 0;
	}

	~dsp_phase()
	{
		if (lpf) delete lpf;
	}

	virtual GUID get_guid()
	{
		// {DD4B6EE0-6003-46f3-92C7-32851D228AC3}
		static const GUID guid = 
		{ 0xdd4b6ee0, 0x6003, 0x46f3, { 0x92, 0xc7, 0x32, 0x85, 0x1d, 0x22, 0x8a, 0xc3 } };
		return guid;
	}

	virtual const char * get_name() {return "Phase inverter";}

	virtual bool on_chunk(audio_chunk * chunk)
	{
		if (chunk->get_channels() == 2)
		{
			if (cfg_lowpass)
			{
				if (chunk->get_srate() != srate || (int)cfg_cutoff != cutoff)
				{
					srate = chunk->get_srate();
					cutoff = cfg_cutoff;
					if (!lpf) lpf = new LPF((double)(cutoff) * .1, srate);
					else lpf->update((double)(cutoff) * .1, srate);
				}
			}
			else
			{
				if (lpf)
				{
					delete lpf;
					lpf = 0;
				}
				cutoff = 0;
			}
			int i;
			audio_sample * foo = chunk->get_data() + 1;
			if (cutoff)
			{
				for (i = chunk->get_sample_count(); i--;)
				{
					audio_sample lp;
					lp = lpf->process(*foo);
					*foo -= lp;
					*foo = (-(*foo)) + lp;
					foo += 2;
				}
			}
			else
			{
				for (i = chunk->get_sample_count(); i--;)
				{
					*foo = -(*foo);
					foo += 2;
				}
			}
		}
		return 1;
	}

	virtual void flush() {}
	virtual double get_latency() {return 0;}
	virtual bool need_track_change_mark() {return false;}
};

class dsp_expand : public dsp_i_base
{
private:
	UINT srate;
	int cutoff;
	LPF * llp, * rlp;

public:
	dsp_expand()
	{
		srate = 0;
		cutoff = 0;
		llp = 0;
		rlp = 0;
	}

	~dsp_expand()
	{
		if (rlp) delete rlp;
		if (llp) delete llp;
	}

	virtual GUID get_guid()
	{
		// {0637FB96-AD70-4e0b-B033-B352A5C83B05}
		static const GUID guid = 
		{ 0x637fb96, 0xad70, 0x4e0b, { 0xb0, 0x33, 0xb3, 0x52, 0xa5, 0xc8, 0x3b, 0x5 } };
		return guid;
	}

	virtual const char * get_name() {return "Stereo expander";}

	virtual bool on_chunk(audio_chunk * chunk)
	{
		if (chunk->get_channels() == 2)
		{
			if (cfg_lowpass)
			{
				if (chunk->get_srate() != srate || cfg_cutoff != cutoff)
				{
					srate = chunk->get_srate();
					cutoff = cfg_cutoff;
					if (!llp) llp = new LPF((double)(cutoff) * .1, srate);
					else llp->update((double)(cutoff) * .1, srate);
					if (!rlp) rlp = new LPF((double)(cutoff) * .1, srate);
					else rlp->update((double)(cutoff) * .1, srate);
				}
			}
			else
			{
				if (rlp)
				{
					delete rlp;
					rlp = 0;
				}
				if (llp)
				{
					delete llp;
					llp = 0;
				}
				cutoff = 0;
			}
			int i;
			audio_sample * foo = chunk->get_data();
			for (i = chunk->get_sample_count(); i--;)
			{
				audio_sample l, r, c;
				l = *foo;
				r = foo[1];
				if (cutoff)
				{
					l -= llp->process(l);
					r -= rlp->process(r);
				}
				c = (l + r) * .5;
				l -= c;
				r -= c;
				*foo -= r * (double)(cfg_expstr) * .001;
				foo[1] -= l * (double)(cfg_expstr) * .001;
				foo += 2;
			}
		}
		return 1;
	}

	virtual void flush() {}
	virtual double get_latency() {return 0;}
	virtual bool need_track_change_mark() {return false;}
};

static void set_lowpass(HWND wnd, int status)
{
	EnableWindow(GetDlgItem(wnd, IDC_CUTOFFSLIDER), status);
	EnableWindow(GetDlgItem(wnd, IDC_CUTOFF), status);
	EnableWindow(GetDlgItem(wnd, IDC_CUTOFFST), status);
	uSendDlgItemMessage(wnd, IDC_LOWPASS, BM_SETCHECK, status, 0);
	cfg_lowpass = status;
}

static void upd_cutoff(HWND wnd, int apply)
{
	string8 temp;
	int val = uSendDlgItemMessage(wnd, IDC_CUTOFFSLIDER, TBM_GETPOS, 0, 0);
	temp.add_int(val / 10);
	temp.add_char('.');
	temp.add_int(val % 10);
	uSetDlgItemText(wnd, IDC_CUTOFF, temp);
	if (cfg_realtime || apply)
		cfg_cutoff = val;
	else
		EnableWindow(GetDlgItem(wnd, IDC_APPLY), cfg_cutoff == val ? 0 : 1);
}

static void upd_expstr(HWND wnd, int apply)
{
	string8 temp;
	int val = uSendDlgItemMessage(wnd, IDC_EXPSTRSLIDER, TBM_GETPOS, 0, 0);
	temp.add_int(val / 10);
	temp.add_char('.');
	temp.add_int(val % 10);
	uSetDlgItemText(wnd, IDC_EXPSTR, temp);
	if (cfg_realtime || apply)
		cfg_expstr = val;
	else
		EnableWindow(GetDlgItem(wnd, IDC_APPLY), cfg_expstr == val ? 0 : 1);
}

static BOOL CALLBACK ConfigProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		uSendDlgItemMessage(wnd, IDC_REALTIME, BM_SETCHECK, cfg_realtime, 0);
		set_lowpass(wnd, cfg_lowpass);
		uSendDlgItemMessage(wnd,IDC_CUTOFFSLIDER,TBM_SETRANGE,0,MAKELONG(0,3000));
		uSendDlgItemMessage(wnd,IDC_CUTOFFSLIDER,TBM_SETPOS,1,cfg_cutoff);
		upd_cutoff(wnd, 0);
		uSendDlgItemMessage(wnd,IDC_EXPSTRSLIDER,TBM_SETRANGE,0,MAKELONG(0,8000));
		uSendDlgItemMessage(wnd,IDC_EXPSTRSLIDER,TBM_SETPOS,1,cfg_expstr);
		upd_expstr(wnd, 0);
		return 1;
	case WM_COMMAND:
		switch(wp)
		{
		case IDC_LOWPASS:
			set_lowpass(wnd, uSendMessage((HWND)lp,BM_GETCHECK,0,0));
			break;
		case IDC_REALTIME:
			cfg_realtime = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		case IDC_APPLY:
			upd_cutoff(wnd, 1);
			upd_expstr(wnd, 1);
			EnableWindow((HWND)lp, 0);
			break;
		}
	case WM_HSCROLL:
		switch(uGetWindowLong((HWND)lp,GWL_ID))
		{
		case IDC_CUTOFFSLIDER:
			upd_cutoff(wnd, 0);
			break;
		case IDC_EXPSTRSLIDER:
			upd_expstr(wnd, 0);
			break;
		}
		break;
	}
	return 0;
}

class config_surr : public config
{
public:
	virtual HWND create(HWND parent)
	{
		return uCreateDialog(IDD_CONFIG,parent,ConfigProc);
	}
	virtual const char * get_name() {return "k0de's Surround";}
	virtual const char * get_parent_name() {return "DSP Manager";}
};

static service_factory_t<dsp,dsp_phase> foo;
static service_factory_t<dsp,dsp_expand> foo2;
static service_factory_single_t<config,config_surr> foo3;

DECLARE_COMPONENT_VERSION("k0de's Surround DSP", "1.2", "Phase inverter and Stereo expander, with optional bass exclusion.");
