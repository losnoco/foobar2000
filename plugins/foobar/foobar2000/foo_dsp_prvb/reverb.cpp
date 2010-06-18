#define MYVERSION "1.1"

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

2010-06-18 04:52 UTC - kode54
- Implemented resampling version that is more accurate to the actual hardware
- Version is now 1.1

*/

#define _WIN32_WINNT 0x0501
#include <foobar2000.h>

#include "../ATLHelpers/ATLHelpers.h"

#include "config.h"
#include "reverb.h"

const ReverbPreset defaults[9] = {

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

audio_sample dsp_reverb::MixREVERBLeft( audio_sample INPUT_SAMPLE_L, audio_sample INPUT_SAMPLE_R, float *ptr )
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

dsp_reverb::dsp_reverb( dsp_preset const & in ) : r( defaults[ 3 ].config )
{
	parse_preset( r, in );
	flush();
}
	
GUID dsp_reverb::g_get_guid()
{
	static const GUID guid = { 0xf6ba3354, 0x306f, 0x4251, { 0xac, 0x74, 0x9d, 0xe6, 0x16, 0xf2, 0xa4, 0xc6 } };
	return guid;
}
	
void dsp_reverb::g_get_name( pfc::string_base & p_out ) { p_out = "Programmable reverb"; }

bool dsp_reverb::on_chunk( audio_chunk * chunk, abort_callback & )
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
#undef cfgf
#undef cfgo

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

bool dsp_reverb::g_get_default_preset( dsp_preset & p_out )
{
	make_preset( defaults[ 3 ].config, p_out, g_get_guid() );
	return true;
}

void dsp_reverb::g_show_config_popup( const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback )
{
	::RunDSPConfigPopup( p_data, p_parent, p_callback );
}

void dsp_reverb::make_preset( const ReverbConfig & r, dsp_preset & out, const GUID & guid )
{
	dsp_preset_builder builder; builder << r.delay << r.mVolLeft << r.mVolRight << r.VolLeft << r.VolRight << r.FB_SRC_A << r.FB_SRC_B
		<< r.IIR_ALPHA << r.ACC_COEF_A << r.ACC_COEF_B << r.ACC_COEF_C << r.ACC_COEF_D << r.IIR_COEF << r.FB_ALPHA << r.FB_X
		<< r.IIR_DEST_A0 << r.IIR_DEST_A1 << r.ACC_SRC_A0 << r.ACC_SRC_A1 << r.ACC_SRC_B0 << r.ACC_SRC_B1
		<< r.IIR_SRC_A0 << r.IIR_SRC_A1 << r.IIR_DEST_B0 << r.IIR_DEST_B1 << r.ACC_SRC_C0 << r.ACC_SRC_C1
		<< r.ACC_SRC_D0 << r.ACC_SRC_D1 << r.IIR_SRC_B1 << r.IIR_SRC_B0 << r.MIX_DEST_A0 << r.MIX_DEST_A1
		<< r.MIX_DEST_B0 << r.MIX_DEST_B1 << r.IN_COEF_L << r.IN_COEF_R; builder.finish(guid, out);
}

void dsp_reverb::parse_preset( ReverbConfig & r, const dsp_preset & in )
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

void dsp_reverb_accurate::recreate_resamplers()
{
	if ( !resampler_entry::g_create( resampler [0], srate, 22050, 0 ) ||
		! resampler_entry::g_create( resampler [1], 22050, srate, 0 ) ) throw exception_io_data( "Invalid sample rate conversion or no resamplers installed." );
}

dsp_reverb_accurate::dsp_reverb_accurate( dsp_preset const & in ) : dsp_reverb( in )
{
	flush();
}

GUID dsp_reverb_accurate::g_get_guid()
{
	static const GUID guid = { 0x12a8cd05, 0xc39a, 0x4868, { 0x8d, 0x45, 0xf9, 0x26, 0x1f, 0xe8, 0x87, 0xed } };
	return guid;
}
	
void dsp_reverb_accurate::g_get_name( pfc::string_base & p_out ) { p_out = "Programmable reverb (resampling/accurate)"; }

bool dsp_reverb_accurate::on_chunk( audio_chunk * chunk, abort_callback & p_abort )
{
	UINT samples = chunk->get_sample_count();
	srate = chunk->get_srate();
	nch = chunk->get_channels();
	ch_mask = chunk->get_channel_config();
	if (!samples || !srate || !nch || !ch_mask) return true;

	if ( ( nch != 1 || ch_mask != audio_chunk::channel_config_mono ) &&
		( nch != 2 || ch_mask != audio_chunk::channel_config_stereo ) &&
		( nch != 4 || ch_mask != channel_config_4point0 ) ) return true;

	if ( srate != oldsrate )
	{
		oldsrate = srate;
		recreate_resamplers();
	}

#define cfgf(a) a = cnv_float(r.##a)
#define cfgo(a) a = cnv_offset(r.##a)

	if ( !initialized )
	{
		initialized = true;

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

		reverb.grow_size( delay );
		memset( reverb.get_ptr(), 0, delay * sizeof( float ) );

		CurrAddr = 0;
	}

	if ( srate != oldsrate || nch != oldnch || ch_mask != oldch_mask ) { samples_remain = 0; reverb_samples_remain = 0; }

	dsp_chunk_list_impl resample_list;

	audio_chunk * temp = resample_list.insert_item( 0, ( samples + samples_remain ) * 2 );
	temp->grow_data_size( ( samples + samples_remain ) * 2 );
	temp->set_sample_count( samples + samples_remain );
	temp->set_srate( srate );
	temp->set_channels( 2, audio_chunk::channel_config_stereo );

	input_samples.grow_size( ( samples + samples_remain ) * nch );
	audio_sample * in = input_samples.get_ptr();
	memcpy( in + samples_remain * nch, chunk->get_data(), samples * nch * sizeof( float ) );

	audio_sample * out = temp->get_data();

	samples += samples_remain;
	samples_remain = 0;

	if ( nch == 1 )
	{
		for ( unsigned i = 0; i < samples; i++ )
		{
			out [i * 2 + 0] = in [i];
			out [i * 2 + 1] = in [i];
		}
	}
	else if ( nch == 2 )
	{
		memcpy( out, in, samples * 2 * sizeof( float ) );
	}
	else if ( nch == 4 )
	{
		for ( unsigned i = 0; i < samples; i++ )
		{
			audio_sample fl, fr, rl, rr;
			fl = in [i * 4 + 0];
			fr = in [i * 4 + 1];
			rl = in [i * 4 + 2];
			rr = in [i * 4 + 3];
			out [i * 2 + 0] = fl + rl;
			out [i * 2 + 1] = fr + rr;
		}
	}

	resampler [0]->run_abortable( &resample_list, NULL, 0, p_abort );

	float * ptr = reverb.get_ptr();

	for ( unsigned i = 0, j = resample_list.get_count(); i < j; i++ )
	{
		audio_chunk * reverb_chunk = resample_list.get_item( i );
		audio_sample * samples = reverb_chunk->get_data();

		for ( unsigned k = 0; k < reverb_chunk->get_sample_count(); k++ )
		{
			samples [k * 2 + 0] = MixREVERBLeft( samples [k * 2 + 0], samples [k * 2 + 1], ptr );
			samples [k * 2 + 1] = MixREVERBRight();
		}
	}

	resampler [1]->run_abortable( &resample_list, NULL, 0, p_abort );

	unsigned processed_out = 0;

	for ( unsigned i = 0; i < resample_list.get_count(); i++ )
	{
		processed_out += resample_list.get_item( i )->get_sample_count();
	}

	reverb_samples.grow_size( ( processed_out + reverb_samples_remain ) * 2 );
	audio_sample * reverb_ptr = reverb_samples.get_ptr() + reverb_samples_remain * 2;
	
	for ( unsigned i = 0; i < resample_list.get_count(); i++ )
	{
		audio_chunk * reverb_chunk = resample_list.get_item( i );
		audio_sample * reverb_data = reverb_chunk->get_data();
		unsigned copy_count = reverb_chunk->get_sample_count() * 2;
		memcpy( reverb_ptr, reverb_data, copy_count * sizeof( audio_sample ) );
		reverb_ptr += copy_count;
	}

	reverb_ptr = reverb_samples.get_ptr();

	unsigned processed_todo = min( processed_out, samples );

	if ( nch == 1 )
	{
		output_buffer.grow_size( processed_todo * 2 );
		audio_sample * out = output_buffer.get_ptr();

		for ( unsigned i = 0; i < processed_todo; i++ )
		{
			out [i * 2 + 0] = in [i] * mVolLeft + reverb_ptr [i * 2 + 0];
			out [i * 2 + 1] = in [i] * mVolRight + reverb_ptr [i * 2 + 1];
		}
		audio_chunk * out_chunk = insert_chunk( processed_todo * 2 );
		out_chunk->set_data( out, processed_todo, 2, srate, audio_chunk::channel_config_stereo );
	}
	else if ( nch == 2 )
	{
		output_buffer.grow_size( processed_todo * 2 );
		audio_sample * out = output_buffer.get_ptr();

		for ( unsigned i = 0; i < processed_todo; i++ )
		{
			out [i * 2 + 0] = in [i * 2 + 0] * mVolLeft + reverb_ptr [i * 2 + 0];
			out [i * 2 + 1] = in [i * 2 + 1] * mVolRight + reverb_ptr [i * 2 + 1];
		}
		audio_chunk * out_chunk = insert_chunk( processed_todo * 2 );
		out_chunk->set_data( out, processed_todo, 2, srate, audio_chunk::channel_config_stereo );
	}
	else if ( nch == 4 )
	{
		output_buffer.grow_size( processed_todo * 4 );
		audio_sample * out = output_buffer.get_ptr();

		for ( unsigned i = 0; i < processed_todo; i++ )
		{
			float iml, imr, oml, omr, dl, dr;
			iml = in [i * 4 + 0] + in [i * 4 + 2];
			imr = in [i * 4 + 1] + in [i * 4 + 3];
			dl = in [i * 4 + 0] - in [i * 4 + 2];
			dr = in [i * 4 + 1] - in [i * 4 + 3];
			oml = reverb_ptr [i * 2 + 0];
			omr = reverb_ptr [i * 2 + 1];

			oml = iml * mVolLeft + oml;
			omr = imr * mVolRight + omr;

			out [i * 4 + 0] = .5f * ( oml + dl );
			out [i * 4 + 1] = .5f * ( omr + dr );
			out [i * 4 + 2] = .5f * ( oml - dl );
			out [i * 4 + 3] = .5f * ( omr - dr );
		}
		audio_chunk * out_chunk = insert_chunk( processed_todo * 4 );
		out_chunk->set_data( out, processed_todo, 4, channel_config_4point0 );
	}

	if ( samples > processed_todo )
	{
		samples_remain = samples - processed_todo;
		reverb_samples_remain = 0;
		memcpy( in, in + processed_out * nch, samples_remain * nch * sizeof( audio_sample ) );
	}
	else
	{
		samples_remain = 0;
		reverb_samples_remain = processed_out - samples;
		memcpy( reverb_ptr, reverb_ptr + samples * 2, reverb_samples_remain * 2 * sizeof( audio_sample ) );
	}

	oldsrate = srate; oldnch = nch; oldch_mask = ch_mask;

	return false;
}

void dsp_reverb_accurate::flush()
{
	dsp_reverb::flush();
	initialized = false;
	samples_remain = 0;
	reverb_samples_remain = 0;
}

double dsp_reverb_accurate::get_latency()
{
	double latency = 0.;
	if ( srate && samples_remain ) latency += ( double ) samples_remain / ( double ) srate;
	return latency;
}

bool dsp_reverb_accurate::g_get_default_preset( dsp_preset & p_out )
{
	make_preset( defaults[ 3 ].config, p_out, g_get_guid() );
	return true;
}

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

static dsp_factory_t          <dsp_reverb>          g_dsp_reverb_factory;
static dsp_factory_t          <dsp_reverb_accurate> g_dsp_reverb_accurate_factory;
static dsp_factory_nopreset_t <dsp_stereo2x_2>      g_dsp_stereo2x_2_factory;

DECLARE_COMPONENT_VERSION("Programmable reverb DSP", MYVERSION, "Programmable reverb, based on PSX reverb\ncode from the P.E.Op.S. project.\n\nhttp://www.sourceforge.net/projects/peops");

VALIDATE_COMPONENT_FILENAME("foo_dsp_prvb.dll");
