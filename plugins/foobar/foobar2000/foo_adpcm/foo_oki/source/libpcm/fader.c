#include "libpcm.h"
#include "memman.h"
#include "fader.h"
#include "util.h"


struct LIBPCM_FADER_tag
{
	LIBPCM_DECODER *d;
	uint32_t fadetime;
	uint32_t length_ms;

	uint32_t startfade;
	unsigned char enable;
};
#define FADETABLE_BITS 6
#define FADETABLE_OUTBITS 8
unsigned char fadetbl[1 << FADETABLE_BITS] =
{
	0xfd, 0xfa, 0xf7, 0xf5, 0xf2, 0xef, 0xed, 0xea, 
	0xe8, 0xe5, 0xe3, 0xe0, 0xde, 0xdb, 0xd9, 0xd7, 
	0xd4, 0xd2, 0xd0, 0xce, 0xcb, 0xc9, 0xc7, 0xc5, 
	0xc3, 0xc1, 0xbf, 0xbd, 0xba, 0xb8, 0xb6, 0xb5, 
	0xb3, 0xb1, 0xaf, 0xad, 0xab, 0xa9, 0xa7, 0xa5, 
	0xa4, 0xa2, 0xa0, 0x9e, 0x9d, 0x9b, 0x99, 0x98, 
	0x96, 0x94, 0x93, 0x91, 0x90, 0x8e, 0x8d, 0x8b, 
	0x8a, 0x88, 0x87, 0x85, 0x84, 0x82, 0x81, 0x80, 
};


#define read8f(p) ((unsigned char *)p)[0]
#define write8f(p, v) ((unsigned char *)p)[0] = ((unsigned char)v)
#ifndef LIBPCM_WORDS_BIGENDIAN
static uint_t read16f(const unsigned char *p)
{
	uint_t ret;
	ret  = ((uint_t)(const unsigned char)p[0]) << (0 * 8);
	ret |= ((uint_t)(const unsigned char)p[1]) << (1 * 8);
	return ret;
}
static void write16f(unsigned char *p, uint_t v)
{
	p[0] = (unsigned char)(v >> (8 * 0));
	p[1] = (unsigned char)(v >> (8 * 1));
}
static uint32_t read24f(const unsigned char *p)
{
	uint32_t ret;
	ret  = ((uint32_t)(const unsigned char)p[0]) << (0 * 8);
	ret |= ((uint32_t)(const unsigned char)p[1]) << (1 * 8);
	ret |= ((uint32_t)(const unsigned char)p[2]) << (2 * 8);
	return ret;
}
static void write24f(unsigned char *p, uint32_t v)
{
	p[0] = (unsigned char)(v >> (8 * 0));
	p[1] = (unsigned char)(v >> (8 * 1));
	p[2] = (unsigned char)(v >> (8 * 2));
}
#else
static uint_t read16f(const unsigned char *p)
{
	uint_t ret;
	ret  = ((uint_t)(const unsigned char)p[0]) << (1 * 8);
	ret |= ((uint_t)(const unsigned char)p[1]) << (0 * 8);
	return ret;
}
static void write16f(unsigned char *p, uint_t v)
{
	p[0] = (unsigned char)(v >> (8 * 1));
	p[1] = (unsigned char)(v >> (8 * 0));
}
static uint32_t read24f(const unsigned char *p)
{
	uint32_t ret;
	ret  = ((uint32_t)(const unsigned char)p[0]) << (2 * 8);
	ret |= ((uint32_t)(const unsigned char)p[1]) << (1 * 8);
	ret |= ((uint32_t)(const unsigned char)p[2]) << (0 * 8);
	return ret;
}
static void write24f(unsigned char *p, uint32_t v)
{
	p[0] = (unsigned char)(v >> (8 * 2));
	p[1] = (unsigned char)(v >> (8 * 1));
	p[2] = (unsigned char)(v >> (8 * 0));
}
#endif

LIBPCM_FADER * LIBPCM_API libpcm_fader_initialize(LIBPCM_DECODER *d)
{
	LIBPCM_FADER *f = libpcm_malloc(sizeof(LIBPCM_FADER));
	if (f)
	{
		f->d = d;
		libpcm_fader_configure(f, 2, 5000);
	}
	return f;
}
void LIBPCM_API libpcm_fader_terminate(LIBPCM_FADER *f)
{
	libpcm_mfree(f);
}
void LIBPCM_API libpcm_fader_configure(LIBPCM_FADER *f, uint_t loopcount, uint32_t fadetime_ms)
{
	uint32_t looplength = libpcm_get_looplength(f->d);
	if (looplength && loopcount)
	{
		f->startfade = libpcm_get_length(f->d) + (loopcount - 1) * looplength;
		f->fadetime = libpcm_muldiv(fadetime_ms, libpcm_get_samplerate(f->d), 1000);
		libpcm_switch_loop(f->d, 1);
		f->length_ms = libpcm_get_length_ms(f->d) + (loopcount - 1) * libpcm_get_looplength_ms(f->d) + fadetime_ms;
		f->enable = 1;
	}
	else
	{
		libpcm_switch_loop(f->d, (loopcount == 0));
		f->length_ms = libpcm_get_length_ms(f->d);
		f->enable = 0;
	}
}
uint_t LIBPCM_API libpcm_fader_read(LIBPCM_FADER *f, void *buf, uint_t nsamples)
{
#if 0
	return libpcm_read(f->d, buf, nsamples);
#else
	uint_t ret;
	if (f->enable)
	{
		uint32_t cur = libpcm_get_currentposition(f->d);
		if (cur + nsamples > f->startfade + f->fadetime)
			nsamples = (f->startfade + f->fadetime > cur) ? f->startfade + f->fadetime - cur : 0;
		ret = nsamples = nsamples ? libpcm_read(f->d, buf, nsamples) : 0;
		if (ret && cur + ret > f->startfade)
		{
			sint32_t va, vf;
			uint_t ch, ba, nch, bps;
			bps = libpcm_get_bitspersample(f->d);
			nch = libpcm_get_numberofchannels(f->d);
			ba = libpcm_get_blockalign(f->d);
			if (cur < f->startfade)
			{
				buf = ba * (f->startfade - cur) + (unsigned char *)buf;
				nsamples -= (f->startfade - cur);
				cur = f->startfade;
			}
			cur -= f->startfade;
			switch (bps)
			{
			case 8:
				while (nsamples--)
				{
					vf = libpcm_muldiv(cur, 1 << (3 + FADETABLE_BITS), f->fadetime);
					for (ch = 0; ch < nch; ch++)
					{
						va = read8f(buf) - 0x80;
						va = libpcm_mulfix(va, fadetbl[vf & ((1 << FADETABLE_BITS) - 1)], (vf >> FADETABLE_BITS) + FADETABLE_OUTBITS) ^ 0x80;
						write8f(buf, va);
						buf = 1 + (unsigned char *)buf;
					}
				}
				break;
			case 16:
				while (nsamples--)
				{
					vf = libpcm_muldiv(cur, 1 << (3 + FADETABLE_BITS), f->fadetime);
					for (ch = 0; ch < nch; ch++)
					{
						va = (read16f(buf) ^ 0x8000)- 0x8000;
						va = libpcm_mulfix(va, fadetbl[vf & ((1 << FADETABLE_BITS) - 1)], (vf >> FADETABLE_BITS) + FADETABLE_OUTBITS);
						write16f(buf, va);
						buf = 2 + (unsigned char *)buf;
					}
				}
				break;
			case 24:
				while (nsamples--)
				{
					vf = libpcm_muldiv(cur, 1 << (3 + FADETABLE_BITS), f->fadetime);
					for (ch = 0; ch < nch; ch++)
					{
						va = (read24f(buf) ^ 0x800000)- 0x800000;
						va = libpcm_mulfix(va, fadetbl[vf & ((1 << FADETABLE_BITS) - 1)], (vf >> FADETABLE_BITS) + FADETABLE_OUTBITS);
						write24f(buf, va);
						buf = 3 + (unsigned char *)buf;
					}
				}
				break;
			}
		}
	}
	else
		ret = libpcm_read(f->d, buf, nsamples);
	return ret;
#endif
}
uint32_t LIBPCM_API libpcm_fader_get_length_ms(LIBPCM_FADER *f)
{
	return f->length_ms;
}
LIBPCM_DECODER * LIBPCM_API libpcm_fader_get_decoder(LIBPCM_FADER *f)
{
	return f->d;
}
