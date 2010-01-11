#include "libpcm.h"

const char * LIBPCM_API libpcm_get_codecname(int codec)
{
	static const char codecname00[] = "Raw-PCM";
	static const char codecname01[] = "Ulaw-PCM";
	static const char codecname02[] = "OKI-ADPCM";
	static const char codecname03[] = "VC-XPCM";
	static const char codecname04[] = "AD-XPCM";
	static const char codecname05[] = "BW-ADPCM";
	static const char codecname06[] = "PX-ADPCM";
	static const char codecname07[] = "RL-DPCM";
	static const char codecname08[] = "MS-ADPCM";
	static const char codecname09[] = "DVI-ADPCM";
	static const char codecname0a[] = "XA-ADPCM";
	static const char codecname0b[] = "IMA-ADPCM";
	static const char codecname0c[] = "NWA-DPCM";
	static const char * const libpcm_codecs[] =
	{
		codecname00,
		codecname01,
		codecname02,
		codecname03,
		codecname04,
		codecname05,
		codecname06,
		codecname07,
		codecname08,
		codecname09,
		codecname0a,
		codecname0b,
		codecname0c,
	};
	return libpcm_codecs[codec];
}
