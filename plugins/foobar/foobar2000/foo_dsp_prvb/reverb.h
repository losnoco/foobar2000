#ifndef _REVERB_H_
#define _REVERB_H_

#include <foobar2000.h>

struct ReverbConfig
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
};

struct ReverbPreset
{
	const TCHAR * name;
	ReverbConfig config;
};

extern const ReverbPreset defaults[9];

class dsp_reverb : public dsp_impl_base
{
protected:
	pfc::array_t<float> reverb;
	pfc::array_t<audio_sample> output_buffer;
	int srate, nch, ch_mask, oldsrate, oldnch, oldch_mask;

	ReverbConfig r;

	int delay, CurrAddr;

	int FB_SRC_A, FB_SRC_B, IIR_DEST_A0, IIR_DEST_A1, ACC_SRC_A0, ACC_SRC_A1, ACC_SRC_B0,
		ACC_SRC_B1, IIR_SRC_A0, IIR_SRC_A1, IIR_DEST_B0, IIR_DEST_B1, ACC_SRC_C0,
		ACC_SRC_C1, ACC_SRC_D0, ACC_SRC_D1, IIR_SRC_B1, IIR_SRC_B0, MIX_DEST_A0,
		MIX_DEST_A1, MIX_DEST_B0, MIX_DEST_B1;

	float IIR_ALPHA, ACC_COEF_A, ACC_COEF_B, ACC_COEF_C, ACC_COEF_D, IIR_COEF, FB_ALPHA, FB_X,
		IN_COEF_L, IN_COEF_R;

	float iRVBLeft, iRVBRight, VolLeft, VolRight, mVolLeft, mVolRight;

	inline float g_buffer( int iOff, float *ptr )                          // get_buffer content helper: takes care about wraps
	{
		iOff = ( iOff * 4 ) + CurrAddr;
		while ( iOff >= delay )		iOff -= delay;
		while ( iOff < 0 )			iOff += delay;
		return ( float ) *( ptr + iOff );
	}

	inline void s_buffer( int iOff, float iVal, float *ptr )                // set_buffer content helper: takes care about wraps
	{
		iOff = ( iOff * 4 ) + CurrAddr;
		while ( iOff >= delay )		iOff -= delay;
		while ( iOff < 0 )			iOff += delay;
		*( ptr + iOff ) = iVal;
	}

	inline void s_buffer1( int iOff, float iVal, float *ptr )                // set_buffer (+1 sample) content helper: takes care about wraps
	{
		iOff = ( iOff * 4 ) + CurrAddr + 1;
		while ( iOff >= delay )		iOff -= delay;
		while ( iOff < 0 )			iOff += delay;
		*( ptr + iOff ) = iVal;
	}

	audio_sample MixREVERBLeft( audio_sample INPUT_SAMPLE_L, audio_sample INPUT_SAMPLE_R, float *ptr );
	inline audio_sample MixREVERBRight() { return ( audio_sample ) iRVBRight; }

	inline int cnv_offset( int in ) { return MulDiv( in, srate, 22050 ); }
	inline float cnv_float( int in ) { return ( float ) in * ( 1.f / 32768.f ); }

public:
	dsp_reverb( dsp_preset const & in );

	static GUID g_get_guid();
	static void g_get_name( pfc::string_base & p_out );

	bool on_chunk( audio_chunk * chunk, abort_callback & );

	void on_endofplayback(abort_callback &) { }
	void on_endoftrack(abort_callback &) { }
	void flush() { oldsrate = 0; oldnch = 0; oldch_mask = 0; }

	double get_latency() { return 0; }

	bool need_track_change_mark() { return false; }

	static bool g_get_default_preset( dsp_preset & p_out );

	static void g_show_config_popup( const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback );
	static bool g_have_config_popup() { return true; }

	static void make_preset( const ReverbConfig & r, dsp_preset & out, const GUID & guid );
	static void parse_preset( ReverbConfig & r, const dsp_preset & in );
};

class dsp_reverb_accurate : public dsp_reverb
{
protected:
	bool initialized;

	unsigned samples_remain, reverb_samples_remain;

	pfc::array_t< audio_sample > input_samples;
	pfc::array_t< audio_sample > reverb_samples;

	service_ptr_t< dsp > resampler [2];

	inline int cnv_offset( int in ) { return in; }

	void recreate_resamplers();

public:
	dsp_reverb_accurate( dsp_preset const & in );

	static GUID g_get_guid();
	static void g_get_name( pfc::string_base & p_out );

	bool on_chunk( audio_chunk * chunk, abort_callback & );

	void flush();

	double get_latency();

	static bool g_get_default_preset( dsp_preset & p_out );
};

#endif
