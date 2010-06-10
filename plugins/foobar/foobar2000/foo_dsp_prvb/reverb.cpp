#define MYVERSION "1.0"

/*
	changelog

-------------------- - kode54
- Initial version for foobar2000 0.8 or maybe something even older, no configuration
- Version is now 0.5

-------------------- - kode54
- Updated for 0.9, I think

2010-06-09 01:14 UTC - kode54
- Updated for 1.0

2010-06-10 00:11 UTC - kode54
- Added full configuration

2010-06-10 06:20 UTC - kode54
- And presets
- Version is now 1.0

*/

#define _WIN32_WINNT 0x0501
#include <foobar2000.h>

#include "../ATLHelpers/ATLHelpers.h"

#include "resource.h"

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

static const ReverbPreset defaults[] = {

	{ _T("Room"),
		{  4960,
		  32767, 32767, 12800, 12800,
		   0x7D,  0x5B, 28032, 21688, -16688,     0,      0, -17792,
		  22528, 21248, 0x4D6, 0x333,  0x3f0, 0x227,  0x374,  0x1EF,
		  0x334, 0x1B5,     0,     0,      0,     0,      0,      0,
		      0,     0, 0x1B4, 0x136,   0xB8,  0x5C, -32768, -32768 }
	},

	{ _T("Studio Small"),
		{ 4000,
		 32767, 32767, 12800, 12800,
		  0x33,  0x25, 28912, 20392, -17184, 17424, -16144, -25600,
		 21120, 20160, 0x3E4, 0x31B,  0x3A4, 0x2AF,  0x372,  0x266,
		 0x31C, 0x25D, 0x25C, 0x18E,  0x22F, 0x135,  0x1D2,   0xB7,
		 0x18F,  0xB5,  0xB4,  0x80,   0x4C,  0x26, -32768, -32768 }
	},

	{ _T("Studio Medium"),
		{ 9248,
		 32767, 32767, 12800, 12800,
		  0xB1,  0x7F, 28912, 20392, -17184, 17680, -16656, -19264,
		 21120, 20160, 0x904, 0x76B,  0x824, 0x65F,  0x7A2,  0x616,
		 0x76C, 0x5ED, 0x5EC, 0x42E,  0x50F, 0x305,  0x462,  0x2B7,
		 0x42F, 0x265, 0x264, 0x1B2,  0x100,  0x80, -32768, -32768 }
	},

	{ _T("Studio Large"),
		{ 14320,
		  32767, 32767, 12800, 12800,
		   0xE3,  0xA9, 28512, 20392, -17184, 17680, -16656, -22912,
		  22144, 21184, 0xDFB, 0xB58,  0xD09, 0xA3C,  0xBD9,  0x973,
		  0xB59, 0x8DA, 0x8D9, 0x5E9,  0x7EC, 0x4B0,  0x6EF,  0x3D2,
		  0x5EA, 0x31D, 0x31C, 0x238,  0x154,  0xAA, -32768, -32768 }
	},

	{ _T("Hall"),
		{  22256,
		   32767, 32767,  12800,  12800,
		   0x1A5, 0x139,  24576,  20480,  19456, -18432, -17408, -16384,
		   24576, 23552, 0x15BA, 0x11BB, 0x14C2, 0x10BD, 0x11BC,  0xDC1,
		  0x11C0, 0xDC3,  0xDC0,  0x9C1,  0xBC4,  0x7C1,  0xA00,  0x6CD,
		   0x9C2, 0x5C1,  0x5C0,  0x41A,  0x274,  0x13A, -32768, -32768 }
	},

	{ _T("Space Echo"),
		{  31584,
		   32767,  32767,  12800,  12800,
		   0x33D,  0x231,  32256,  20480, -19456, -20480,  19456, -20480,
		   24576,  21504, 0x1ED6, 0x1A31, 0x1D14, 0x183B, 0x1BC2, 0x16B2,
		  0x1A32, 0x15EF, 0x15EE, 0x1055, 0x1334,  0xF2D, 0x11F6,  0xC5D,
		  0x1056,  0xAE1,  0xAE0,  0x7A2,  0x464,  0x232, -32768, -32768 }
	},

	{ _T("Echo"),
		{ 49184,
		  32767,  32767, 12800,   12800,
		      1,      1,  32767,  32767,      0, 0,      0, -32512,
			  0,      0, 0x1FFF,  0xFFF, 0x1005, 5,      0,      0,
		 0x1005,      5,      0,      0,      0, 0,      0,      0,
		      0,      0, 0x1004, 0x1002,      4, 2, -32768, -32768 }
	},

	{ _T("Delay"),
		{ 49184,
		  32767,  32767,  12800,  12800,
		      1,      1,  32767,  32767,      0, 0,      0,      0,
			  0,      0, 0x1FFF,  0xFFF, 0x1005, 5,      0,      0,
		 0x1005,      5,      0,      0,      0, 0,      0,      0,
		      0,      0, 0x1004, 0x1002,      4, 2, -32768, -32768 }
	},

	{ _T("Half Echo"),
		{  7680,
		  32767, 32767, 12800, 12800,
		   0x17,  0x13, 28912, 20392, -17184, 17680, -16656, -31488,
		  24448, 21696, 0x371, 0x2AF,  0x2E5, 0x1DF,  0x2B0,  0x1D7,
		  0x358, 0x26A, 0x1D6, 0x11E,  0x12D,  0xB1,  0x11F,   0x59,
		  0x1A0,  0xE3,  0x58,  0x40,   0x28,  0x14, -32768, -32768 }
	}
};

const int channel_config_4point0 = audio_chunk::channel_front_left | audio_chunk::channel_front_right | audio_chunk::channel_back_left | audio_chunk::channel_back_right;

static void RunDSPConfigPopup( const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback );

class dsp_reverb : public dsp_impl_base
{
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

	////////////////////////////////////////////////////////////////////////

	inline void s_buffer( int iOff, float iVal, float *ptr )                // set_buffer content helper: takes care about wraps
	{
		iOff = ( iOff * 4 ) + CurrAddr;
		while ( iOff >= delay )		iOff -= delay;
		while ( iOff < 0 )			iOff += delay;
		*( ptr + iOff ) = iVal;
	}

	////////////////////////////////////////////////////////////////////////

	inline void s_buffer1( int iOff, float iVal, float *ptr )                // set_buffer (+1 sample) content helper: takes care about wraps
	{
		iOff = ( iOff * 4 ) + CurrAddr + 1;
		while ( iOff >= delay )		iOff -= delay;
		while ( iOff < 0 )			iOff += delay;
		*( ptr + iOff ) = iVal;
	}

	////////////////////////////////////////////////////////////////////////

	audio_sample MixREVERBLeft( audio_sample INPUT_SAMPLE_L, audio_sample INPUT_SAMPLE_R, float *ptr )
	{
		float ACC0, ACC1, FB_A0, FB_A1, FB_B0, FB_B1;

		const float IIR_INPUT_A0 = ( g_buffer( IIR_SRC_A0, ptr ) * IIR_COEF ) + ( INPUT_SAMPLE_L * IN_COEF_L );
		const float IIR_INPUT_A1 = ( g_buffer( IIR_SRC_A1, ptr ) * IIR_COEF ) + ( INPUT_SAMPLE_R * IN_COEF_R );
		const float IIR_INPUT_B0 = ( g_buffer( IIR_SRC_B0, ptr ) * IIR_COEF ) + ( INPUT_SAMPLE_L * IN_COEF_L );
		const float IIR_INPUT_B1 = ( g_buffer( IIR_SRC_B1, ptr ) * IIR_COEF ) + ( INPUT_SAMPLE_R * IN_COEF_R );

		const float IIR_A0 = ( IIR_INPUT_A0 * IIR_ALPHA ) + ( g_buffer( IIR_DEST_A0, ptr ) * ( 1.f - IIR_ALPHA ) );
		const float IIR_A1 = ( IIR_INPUT_A1 * IIR_ALPHA ) + ( g_buffer( IIR_DEST_A1, ptr ) * ( 1.f - IIR_ALPHA ) );
		const float IIR_B0 = ( IIR_INPUT_B0 * IIR_ALPHA ) + ( g_buffer( IIR_DEST_B0, ptr ) * ( 1.f - IIR_ALPHA ) );
		const float IIR_B1 = ( IIR_INPUT_B1 * IIR_ALPHA ) + ( g_buffer( IIR_DEST_B1, ptr ) * ( 1.f - IIR_ALPHA ) );

		s_buffer1( IIR_DEST_A0, IIR_A0, ptr );
		s_buffer1( IIR_DEST_A1, IIR_A1, ptr );
		s_buffer1( IIR_DEST_B0, IIR_B0, ptr );
		s_buffer1( IIR_DEST_B1, IIR_B1, ptr );

		ACC0 = ( g_buffer( ACC_SRC_A0, ptr ) * ACC_COEF_A ) +
			( g_buffer( ACC_SRC_B0, ptr ) * ACC_COEF_B ) +
			( g_buffer( ACC_SRC_C0, ptr ) * ACC_COEF_C ) +
			( g_buffer( ACC_SRC_D0, ptr ) * ACC_COEF_D );
		ACC1 = ( g_buffer( ACC_SRC_A1, ptr ) * ACC_COEF_A ) +
			( g_buffer( ACC_SRC_B1, ptr ) * ACC_COEF_B ) +
			( g_buffer( ACC_SRC_C1, ptr ) * ACC_COEF_C ) +
			( g_buffer( ACC_SRC_D1, ptr ) * ACC_COEF_D );

		FB_A0 = g_buffer( MIX_DEST_A0 - FB_SRC_A, ptr );
		FB_A1 = g_buffer( MIX_DEST_A1 - FB_SRC_A, ptr );
		FB_B0 = g_buffer( MIX_DEST_B0 - FB_SRC_B, ptr );
		FB_B1 = g_buffer( MIX_DEST_B1 - FB_SRC_B, ptr );

		s_buffer( MIX_DEST_A0, ACC0 - ( FB_A0 * FB_ALPHA ), ptr );
		s_buffer( MIX_DEST_A1, ACC1 - ( FB_A1 * FB_ALPHA ), ptr );

		float fb_alpha_sign = ( FB_ALPHA > 0 ) ? 1.f : -1.f;
		float fb_alpha_reversed = ( fabs( FB_ALPHA ) - 1.f ) * fb_alpha_sign;

		s_buffer( MIX_DEST_B0, ( FB_ALPHA * ACC0 ) - ( FB_A0 * fb_alpha_reversed ) - ( FB_B0 * FB_X ), ptr );
		s_buffer( MIX_DEST_B1, ( FB_ALPHA * ACC1 ) - ( FB_A1 * fb_alpha_reversed ) - ( FB_B1 * FB_X ), ptr );

		iRVBLeft  = ( g_buffer( MIX_DEST_A0, ptr ) + g_buffer( MIX_DEST_B0, ptr ) ) / 3.f;
		iRVBRight = ( g_buffer( MIX_DEST_A1, ptr ) + g_buffer( MIX_DEST_B1, ptr ) ) / 3.f;

		iRVBLeft  = ( iRVBLeft  * VolLeft );
		iRVBRight = ( iRVBRight * VolRight );

		CurrAddr++;
		if ( CurrAddr >= delay ) CurrAddr = 0;

		return ( audio_sample ) iRVBLeft;
	}

	////////////////////////////////////////////////////////////////////////

	inline audio_sample MixREVERBRight()
	{
		return ( audio_sample ) iRVBRight;
	}

	inline int cnv_offset( int in )
	{
		return MulDiv( in, srate, 22050 );
	}

	inline float cnv_float(int in)
	{
		return ( float ) in * ( 1.f / 32768.f );
	}

public:
	dsp_reverb( dsp_preset const & in ) : r( defaults[ 3 ].config )
	{
		parse_preset( r, in );
		flush();
	}
	
	static GUID g_get_guid()
	{
		static const GUID guid = { 0xf6ba3354, 0x306f, 0x4251, { 0xac, 0x74, 0x9d, 0xe6, 0x16, 0xf2, 0xa4, 0xc6 } };
		return guid;
	}
	
	static void g_get_name( pfc::string_base & p_out ) { p_out = "Programmable reverb"; }

	bool on_chunk( audio_chunk * chunk, abort_callback & )
	{
		UINT samples = chunk->get_sample_count();
		srate = chunk->get_srate();
		nch = chunk->get_channels();
		ch_mask = chunk->get_channel_config();
		if (!samples || !srate || !nch || !ch_mask) return true;

#define cfgf(a) a = cnv_float(r.##a)
#define cfgo(a) a = cnv_offset(r.##a)

		if ( srate != oldsrate )
		{
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

		if ( nch == 1 && ch_mask == audio_chunk::channel_config_mono )
		{
			audio_sample *in = chunk->get_data();
			reverb.grow_size( delay );
			float * ptr = reverb.get_ptr();
			if ( 1 != oldnch || audio_chunk::channel_config_mono != oldch_mask || srate != oldsrate )
			{
				oldnch = 1;
				oldch_mask = audio_chunk::channel_config_mono;
				oldsrate = srate;
				memset( ptr, 0, delay * sizeof( float ) );
				CurrAddr = 0;
			}
			output_buffer.grow_size( samples * 2 );
			audio_sample * out = output_buffer.get_ptr();
			for ( UINT n = samples; n--; )
			{
				*out++ = *in * mVolLeft + MixREVERBLeft( *in, *in, ptr );
				*out++ = *in * mVolRight + MixREVERBRight();
				in++;
			}
			chunk->set_data( output_buffer.get_ptr(), samples, 2, srate, audio_chunk::channel_config_stereo );
		}
		else if ( nch == 2 && ch_mask == audio_chunk::channel_config_stereo )
		{
			audio_sample *in = chunk->get_data();
			reverb.grow_size( delay );
			float * ptr = reverb.get_ptr();
			if ( 2 != oldnch || audio_chunk::channel_config_stereo != oldch_mask || srate != oldsrate )
			{
				oldnch = 2;
				oldch_mask = audio_chunk::channel_config_stereo;
				oldsrate = srate;
				memset( ptr, 0, delay * sizeof( float ) );
				CurrAddr = 0;
			}
			output_buffer.grow_size( samples * 2 );
			audio_sample * out = output_buffer.get_ptr();
			for ( UINT n = samples; n--; )
			{
				*out++ = *in * mVolLeft + MixREVERBLeft( *in, in[1], ptr );
				*out++ = in[1] * mVolRight + MixREVERBRight();
				in += 2;
			}
			chunk->set_data( output_buffer.get_ptr(), samples, 2, srate, audio_chunk::channel_config_stereo );
		}
		else if ( nch == 4 && ch_mask == channel_config_4point0 )
		{
			audio_sample * in = chunk->get_data();
			reverb.grow_size( delay );
			float * ptr = reverb.get_ptr();
			if ( 4 != oldnch || channel_config_4point0 != oldch_mask || srate != oldsrate )
			{
				oldnch = 4;
				oldch_mask = channel_config_4point0;
				oldsrate = srate;
				memset( ptr, 0, delay * sizeof( float ) );
				CurrAddr = 0;
			}
			output_buffer.grow_size( samples * 4 );
			audio_sample * out = output_buffer.get_ptr();
			for ( UINT n = samples; n--; )
			{
				audio_sample ml = *in + in[2];
				audio_sample dl = *in - in[2];
				audio_sample mr = in[1] + in[3];
				audio_sample dr = in[1] - in[3];
				ml = ml * mVolLeft + MixREVERBLeft( ml, mr, ptr );
				mr = mr * mVolRight + MixREVERBRight();
				*out++ = .5f * (ml + dl);
				*out++ = .5f * (mr + dr);
				*out++ = .5f * (ml - dl);
				*out++ = .5f * (mr - dr);
				in += 4;
			}
			chunk->set_data( output_buffer.get_ptr(), samples, 4, srate, channel_config_4point0 );
		}
		else
		{
			oldsrate = srate;
			oldnch = nch;
		}
		return true;
	}

	void on_endofplayback(abort_callback &) { }
	void on_endoftrack(abort_callback &) { }
	void flush()
	{
		oldch_mask = oldnch = oldsrate = 0;
	}

	double get_latency()
	{
		return 0;
	}

	bool need_track_change_mark()
	{
		return false;
	}
	static bool g_get_default_preset( dsp_preset & p_out )
	{
		make_preset( defaults[ 3 ].config, p_out );
		return true;
	}
	static void g_show_config_popup( const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback )
	{
		::RunDSPConfigPopup( p_data, p_parent, p_callback );
	}
	static bool g_have_config_popup() { return true; }
	static void make_preset( const ReverbConfig & r, dsp_preset & out )
	{
		dsp_preset_builder builder; builder << r.delay << r.mVolLeft << r.mVolRight << r.VolLeft << r.VolRight << r.FB_SRC_A << r.FB_SRC_B
			<< r.IIR_ALPHA << r.ACC_COEF_A << r.ACC_COEF_B << r.ACC_COEF_C << r.ACC_COEF_D << r.IIR_COEF << r.FB_ALPHA << r.FB_X
			<< r.IIR_DEST_A0 << r.IIR_DEST_A1 << r.ACC_SRC_A0 << r.ACC_SRC_A1 << r.ACC_SRC_B0 << r.ACC_SRC_B1
			<< r.IIR_SRC_A0 << r.IIR_SRC_A1 << r.IIR_DEST_B0 << r.IIR_DEST_B1 << r.ACC_SRC_C0 << r.ACC_SRC_C1
			<< r.ACC_SRC_D0 << r.ACC_SRC_D1 << r.IIR_SRC_B1 << r.IIR_SRC_B0 << r.MIX_DEST_A0 << r.MIX_DEST_A1
			<< r.MIX_DEST_B0 << r.MIX_DEST_B1 << r.IN_COEF_L << r.IN_COEF_R; builder.finish(g_get_guid(), out);
	}
	static void parse_preset( ReverbConfig & r, const dsp_preset & in )
	{
		try
		{
			dsp_preset_parser parser( in ); parser >> r.delay >> r.mVolLeft >> r.mVolRight >> r.VolLeft >> r.VolRight >> r.FB_SRC_A >> r.FB_SRC_B
				>> r.IIR_ALPHA >> r.ACC_COEF_A >> r.ACC_COEF_B >> r.ACC_COEF_C >> r.ACC_COEF_D >> r.IIR_COEF >> r.FB_ALPHA >> r.FB_X
				>> r.IIR_DEST_A0 >> r.IIR_DEST_A1 >> r.ACC_SRC_A0 >> r.ACC_SRC_A1 >> r.ACC_SRC_B0 >> r.ACC_SRC_B1
				>> r.IIR_SRC_A0 >> r.IIR_SRC_A1 >> r.IIR_DEST_B0 >> r.IIR_DEST_B1 >> r.ACC_SRC_C0 >> r.ACC_SRC_C1
				>> r.ACC_SRC_D0 >> r.ACC_SRC_D1 >> r.IIR_SRC_B1 >> r.IIR_SRC_B0 >> r.MIX_DEST_A0 >> r.MIX_DEST_A1
				>> r.MIX_DEST_B0 >> r.MIX_DEST_B1 >> r.IN_COEF_L >> r.IN_COEF_R;
		}
		catch(exception_io_data) { r = defaults[ 3 ].config; }
	}
};

class dsp_stereo2x_2 : public dsp_impl_base
{
	pfc::array_t<audio_sample> temp;

public:
	static GUID g_get_guid()
	{
		static const GUID guid = { 0x793eb87d, 0x810, 0x4cc6, { 0xae, 0x4f, 0x57, 0xac, 0xbf, 0xe0, 0xe4, 0x51 } };
		return guid;
	}

	static void g_get_name( pfc::string_base & p_out ) { p_out = "Convert stereo to 4 channels (front only)"; }

	virtual bool on_chunk( audio_chunk * chunk, abort_callback & )
	{
		if ( chunk->get_channels() == 2 && chunk->get_channel_config() == audio_chunk::channel_config_stereo )
		{
			UINT samples = chunk->get_sample_count();
			temp.grow_size( samples * 4 );
			audio_sample * p_temp = temp.get_ptr(),
			               * data = chunk->get_data();
			for( UINT n = samples;n--; )
			{
				*p_temp++ = *data++;
				*p_temp++ = *data++;
				*p_temp++ = 0;
				*p_temp++ = 0;
			}

			chunk->set_data( temp.get_ptr(), samples, 4, chunk->get_srate(), channel_config_4point0 );
		}
		return true;
	}

	virtual void on_endoftrack( abort_callback & ) { }
	virtual void on_endofplayback( abort_callback & ) { }
	virtual void flush() { }
	virtual double get_latency() { return 0; }
	virtual bool need_track_change_mark() { return false; }
};

enum
{
	delay = 0,

	mVolLeft,
	mVolRight,

	VolLeft,
	VolRight,

	FB_SRC_A,
	FB_SRC_B,

	IIR_ALPHA,
	ACC_COEF_A,
	ACC_COEF_B,
	ACC_COEF_C,
	ACC_COEF_D,
	IIR_COEF,
	FB_ALPHA,
	FB_X,

	IIR_DEST_A0,
	IIR_DEST_A1,
	ACC_SRC_A0,
	ACC_SRC_A1,
	ACC_SRC_B0,
	ACC_SRC_B1,
	IIR_SRC_A0,
	IIR_SRC_A1,
	IIR_DEST_B0,
	IIR_DEST_B1,
	ACC_SRC_C0,
	ACC_SRC_C1,
	ACC_SRC_D0,
	ACC_SRC_D1,
	IIR_SRC_B1,
	IIR_SRC_B0,
	MIX_DEST_A0,
	MIX_DEST_A1,
	MIX_DEST_B0,
	MIX_DEST_B1,

	IN_COEF_L,
	IN_COEF_R
};

#define SLIDER_NAMED(a,b) { a, IDC_SLIDER_##b, IDC_SLIDER_LABEL_##b }
#define SLIDER(a) { a, IDC_SLIDER_##a, IDC_SLIDER_LABEL_##a }

struct slider
{
	unsigned index, id, label_id;
};

static const slider sliders_percent[] =
{
	SLIDER_NAMED( mVolLeft, MVOLLEFT ),
	SLIDER_NAMED( mVolRight, MVOLRIGHT ),
	SLIDER_NAMED( VolLeft, VOLLEFT ),
	SLIDER_NAMED( VolRight, VOLRIGHT ),
	SLIDER( IIR_ALPHA ),
	SLIDER( ACC_COEF_A ),
	SLIDER( ACC_COEF_B ),
	SLIDER( ACC_COEF_C ),
	SLIDER( ACC_COEF_D ),
	SLIDER( IIR_COEF ),
	SLIDER( FB_ALPHA ),
	SLIDER( FB_X ),
	SLIDER( IN_COEF_L ),
	SLIDER( IN_COEF_R )
};

static const slider sliders_delay[] =
{
	SLIDER( FB_SRC_A ),
	SLIDER( FB_SRC_B ),
	SLIDER( IIR_DEST_A0 ),
	SLIDER( IIR_DEST_A1 ),
	SLIDER( ACC_SRC_A0 ),
	SLIDER( ACC_SRC_A1 ),
	SLIDER( ACC_SRC_B0 ),
	SLIDER( ACC_SRC_B1 ),
	SLIDER( IIR_SRC_A0 ),
	SLIDER( IIR_SRC_A1 ),
	SLIDER( IIR_DEST_B0 ),
	SLIDER( IIR_DEST_B1 ),
	SLIDER( ACC_SRC_C0 ),
	SLIDER( ACC_SRC_C1 ),
	SLIDER( ACC_SRC_D0 ),
	SLIDER( ACC_SRC_D1 ),
	SLIDER( IIR_SRC_B1 ),
	SLIDER( IIR_SRC_B0 ),
	SLIDER( MIX_DEST_A0 ),
	SLIDER( MIX_DEST_A1 ),
	SLIDER( MIX_DEST_B0 ),
	SLIDER( MIX_DEST_B1 )
};

class CMyDSPPopup : public CDialogImpl<CMyDSPPopup>
{
public:
	CMyDSPPopup(const dsp_preset & initData, dsp_preset_edit_callback & callback) : m_initData(initData), m_callback(callback) { }

	enum { IDD = IDD_DSP };

	enum
	{
		DelayRangeMin = 1,
		DelayRangeMax = 22050 * 6,

		DelayRangeTotal = DelayRangeMax - DelayRangeMin,

		PercentRangeMin = -32768,
		PercentRangeMax = 32767,

		PercentRangeTotal = PercentRangeMax - PercentRangeMin
	};

	BEGIN_MSG_MAP(CMyDSPPopup)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnButton)
		COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnButton)
		COMMAND_HANDLER_EX(IDC_COMBO, CBN_SELCHANGE, OnSelect)
		MSG_WM_HSCROLL(OnHScroll)
	END_MSG_MAP()

private:

	BOOL OnInitDialog(CWindow, LPARAM)
	{
		m_combo = GetDlgItem( IDC_COMBO );
		for ( unsigned i = 0, j = _countof( defaults ); i < j; i++ )
		{
			m_combo.AddString( defaults[ i ].name );
		}
		m_combo.AddString( _T("Custom") );

		m_slider[ delay ] = GetDlgItem( IDC_SLIDER_DELAY );
		m_slider[ delay ].SetRangeMin( 0 );
		m_slider[ delay ].SetRangeMax( DelayRangeTotal );

		for ( unsigned i = 0, j = _countof( sliders_percent ); i < j; i++ )
		{
			const slider & s = sliders_percent[ i ];
			CTrackBarCtrl & sl = m_slider[ s.index ];
			sl = GetDlgItem( s.id );
			sl.SetRangeMin( 0 );
			sl.SetRangeMax( PercentRangeTotal, TRUE );
		}

		for ( unsigned i = 0, j = _countof( sliders_delay ); i < j; i++ )
		{
			const slider & s = sliders_delay[ i ];
			m_slider[ s.index ] = GetDlgItem( s.id );
		}

		{
			ReverbConfig r;
			dsp_reverb::parse_preset( r, m_initData );

			m_custom = r;

			RefreshDelayRanges( r.delay );

			FindPreset( r );

			RefreshSliders( r );

			RefreshLabels( r );
		}
		return TRUE;
	}

	void OnButton(UINT, int id, CWindow)
	{
		EndDialog(id);
	}

	void OnSelect(UINT, int id, CWindow)
	{
		unsigned idx = m_combo.GetCurSel();
		ReverbConfig r;
		if ( idx < _countof( defaults ) ) r = defaults[ idx ].config;
		else r = m_custom;
		RefreshDelayRanges( r.delay );
		RefreshSliders( r );
		{
			dsp_preset_impl preset;
			dsp_reverb::make_preset( r, preset );
			m_callback.on_preset_changed( preset );
		}
		RefreshLabels( r );
	}

	void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar)
	{
		ReverbConfig r;

		CollectPreset( r );

		FindPreset( r );
		
		if ( pScrollBar.m_hWnd == m_slider[ delay ].m_hWnd ) RefreshDelayRanges( r.delay );

		{
			dsp_preset_impl preset;
			dsp_reverb::make_preset( r, preset );
			m_callback.on_preset_changed( preset );
		}
		RefreshLabels( r );
	}

	void RefreshLabels(const ReverbConfig & r)
	{
		pfc::string_formatter msg; msg << pfc::format_int( MulDiv( r.delay, 1000, 22050 ) ) << " ms";
		::uSetDlgItemText( *this, IDC_SLIDER_LABEL_DELAY, msg );
		
		for ( unsigned i = 0, j = _countof( sliders_percent ); i < j; i++ )
		{
			const slider & s = sliders_percent[ i ];
			const int * val = reinterpret_cast<const int*>( &r ) + s.index;
			msg.reset(); msg << pfc::format_float( ( double ) ( *val ) * 100. / 32768., 0, 2 ) << "%";
			::uSetDlgItemText( *this, s.label_id, msg );
		}

		for ( unsigned i = 0, j = _countof( sliders_delay ); i < j; i++ )
		{
			const slider & s = sliders_delay[ i ];
			const int * val = reinterpret_cast<const int*>( &r ) + s.index;
			msg.reset(); msg << pfc::format_int( MulDiv( *val, 1000 * 4, 22050 ) ) << " ms";
			::uSetDlgItemText( *this, s.label_id, msg );
		}
	}

	void RefreshDelayRanges(unsigned delay)
	{
		unsigned delay_max = delay / 4;
		for ( unsigned i = 0, j = _countof( sliders_delay ); i < j; i++ )
		{
			const slider & s = sliders_delay[ i ];
			CTrackBarCtrl & sl = m_slider[ s.index ];
			sl.SetRangeMin( 0 );
			sl.SetRangeMax( delay_max, TRUE );
		}
	}

	void RefreshSliders( const ReverbConfig & r )
	{
		m_slider[ delay ].SetPos( pfc::clip_t<t_int32>( r.delay, DelayRangeMin, DelayRangeMax ) - DelayRangeMin );

		for ( unsigned i = 0, j = _countof( sliders_percent ); i < j; i++ )
		{
			const slider & s = sliders_percent[ i ];
			CTrackBarCtrl & sl = m_slider[ s.index ];
			const int * val = reinterpret_cast<const int*>( &r ) + s.index;
			sl.SetPos( pfc::clip_t<t_int32>( *val, PercentRangeMin, PercentRangeMax ) - PercentRangeMin );
		}

		unsigned delay_max = r.delay / 4;

		for ( unsigned i = 0, j = _countof( sliders_delay ); i < j; i++ )
		{
			const slider & s = sliders_delay[ i ];
			CTrackBarCtrl & sl = m_slider[ s.index ];
			const int * val = reinterpret_cast<const int*>( &r ) + s.index;
			sl.SetPos( pfc::clip_t<t_int32>( *val, 0, delay_max ) );
		}
	}

	void CollectPreset( ReverbConfig & r )
	{
		r.delay = m_slider[ delay ].GetPos() + DelayRangeMin;

		for ( unsigned i = 0, j = _countof( sliders_percent ); i < j; i++ )
		{
			const slider & s = sliders_percent[ i ];
			CTrackBarCtrl & sl = m_slider[ s.index ];
			int * val = reinterpret_cast<int*>( &r ) + s.index;
			*val = sl.GetPos() + PercentRangeMin;
		}

		for ( unsigned i = 0, j = _countof( sliders_delay ); i < j; i++ )
		{
			const slider & s = sliders_delay[ i ];
			CTrackBarCtrl & sl = m_slider[ s.index ];
			int * val = reinterpret_cast<int*>( &r ) + s.index;
			*val = sl.GetPos();
		}
	}

	void FindPreset( const ReverbConfig & r )
	{
		unsigned i, j;
		for ( i = 0, j = _countof( defaults ); i < j; i++ )
		{
			const ReverbConfig & rc = defaults[ i ].config;
			if ( !memcmp( &rc, &r, sizeof( r ) ) ) break;
		}
		m_combo.SetCurSel( i );
		if ( i == j ) m_custom = r;
	}

	const dsp_preset & m_initData; // modal dialog so we can reference this caller-owned object.
	dsp_preset_edit_callback & m_callback;

	CTrackBarCtrl m_slider[37];
	CComboBox m_combo;

	ReverbConfig m_custom;
};

static void RunDSPConfigPopup(const dsp_preset & p_data,HWND p_parent,dsp_preset_edit_callback & p_callback) {
	CMyDSPPopup popup(p_data, p_callback);
	if (popup.DoModal(p_parent) != IDOK) p_callback.on_preset_changed(p_data);
}

static dsp_factory_t          <dsp_reverb>     g_dsp_reverb_factory;
static dsp_factory_nopreset_t <dsp_stereo2x_2> g_dsp_stereo2x_2_factory;

DECLARE_COMPONENT_VERSION("Programmable reverb DSP", MYVERSION, "Programmable reverb, based on PSX reverb\ncode from the P.E.Op.S. project.\n\nhttp://www.sourceforge.net/projects/peops");

VALIDATE_COMPONENT_FILENAME("foo_dsp_prvb.dll");
