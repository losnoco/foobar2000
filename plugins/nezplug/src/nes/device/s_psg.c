#include "kmsnddev.h"
#include "divfix.h"
#include "s_logtbl.h"
#include "s_psg.h"

#define DCFIX 0/*8*/
#define ANAEX 0
#define CPS_SHIFT 18
#define CPS_ENVSHIFT 12
#define LOG_KEYOFF (31 << (LOG_BITS + 1))

typedef struct {
#if DCFIX
	Uint32 dcbuf[1 << DCFIX];
	Uint32 dcave;
	Uint32 dcptr;
#endif
	Uint32 cps;
	Uint32 cycles;
#if ANAEX
	Int32 anaex[2];
#endif
	Uint8 regs[3];
	Uint8 adr;
	Uint8 mute;
	Uint8 key;
} PSG_SQUARE;

typedef struct {
	Uint32 cps;
	Uint32 cycles;
	Uint32 noiserng;
	Uint8 regs[1];
} PSG_NOISE;

typedef struct {
	Uint32 cps;
	Uint32 cycles;
	Int8 *adr;
	Uint8 regs[3];
	Uint8 adrmask;
} PSG_ENVELOPE;

typedef struct {
	KMIF_SOUND_DEVICE kmif;
	KMIF_LOGTABLE *logtbl;
	PSG_SQUARE square[3];
	PSG_ENVELOPE envelope;
	PSG_NOISE noise;
	struct {
		Int32 mastervolume;
		Uint32 davolume;
		Uint32 envout;
		Uint8 daenable;
		Uint8 regs[1];
		Uint8 rngout;
		Uint8 adr;
	} common;
	Uint8 type;
} PSGSOUND;

static Int8 env_pulse[] = 
{
	0x1F,+02,0x1E,+02,0x1D,+02,0x1C,+02,0x1B,+02,0x1A,+02,0x19,+02,0x18,+02,
	0x17,+02,0x16,+02,0x15,+02,0x14,+02,0x13,+02,0x12,+02,0x11,+02,0x10,+02,
	0x0F,+02,0x0E,+02,0x0D,+02,0x0C,+02,0x0B,+02,0x0A,+02,0x09,+02,0x08,+02,
	0x07,+02,0x06,+02,0x05,+02,0x04,+02,0x03,+02,0x02,+02,0x01,+02,0x00,+00,
};
static Int8 env_pulse_hold[] = 
{
	0x1F,+02,0x1E,+02,0x1D,+02,0x1C,+02,0x1B,+02,0x1A,+02,0x19,+02,0x18,+02,
	0x17,+02,0x16,+02,0x15,+02,0x14,+02,0x13,+02,0x12,+02,0x11,+02,0x10,+02,
	0x0F,+02,0x0E,+02,0x0D,+02,0x0C,+02,0x0B,+02,0x0A,+02,0x09,+02,0x08,+02,
	0x07,+02,0x06,+02,0x05,+02,0x04,+02,0x03,+02,0x02,+02,0x01,+02,0x00,+02,
	0x1F,+00,
};
static Int8 env_saw[] = 
{
	0x1F,+02,0x1E,+02,0x1D,+02,0x1C,+02,0x1B,+02,0x1A,+02,0x19,+02,0x18,+02,
	0x17,+02,0x16,+02,0x15,+02,0x14,+02,0x13,+02,0x12,+02,0x11,+02,0x10,+02,
	0x0F,+02,0x0E,+02,0x0D,+02,0x0C,+02,0x0B,+02,0x0A,+02,0x09,+02,0x08,+02,
	0x07,+02,0x06,+02,0x05,+02,0x04,+02,0x03,+02,0x02,+02,0x01,+02,0x00,-62,
};
static Int8 env_tri[] = 
{
	0x1F,+02,0x1E,+02,0x1D,+02,0x1C,+02,0x1B,+02,0x1A,+02,0x19,+02,0x18,+02,
	0x17,+02,0x16,+02,0x15,+02,0x14,+02,0x13,+02,0x12,+02,0x11,+02,0x10,+02,
	0x0F,+02,0x0E,+02,0x0D,+02,0x0C,+02,0x0B,+02,0x0A,+02,0x09,+02,0x08,+02,
	0x07,+02,0x06,+02,0x05,+02,0x04,+02,0x03,+02,0x02,+02,0x01,+02,0x00,+02,
	0x00,+02,0x01,+02,0x02,+02,0x03,+02,0x04,+02,0x05,+02,0x06,+02,0x07,+02,
	0x08,+02,0x09,+02,0x0A,+02,0x0B,+02,0x0C,+02,0x0D,+02,0x0E,+02,0x0F,+02,
	0x10,+02,0x11,+02,0x12,+02,0x13,+02,0x14,+02,0x15,+02,0x16,+02,0x17,+02,
	0x18,+02,0x19,+02,0x1A,+02,0x1B,+02,0x1C,+02,0x1D,+02,0x1E,+02,0x1F,-126,
};
static Int8 env_xpulse[] = 
{
	0x00,+02,0x01,+02,0x02,+02,0x03,+02,0x04,+02,0x05,+02,0x06,+02,0x07,+02,
	0x08,+02,0x09,+02,0x0A,+02,0x0B,+02,0x0C,+02,0x0D,+02,0x0E,+02,0x0F,+02,
	0x10,+02,0x11,+02,0x12,+02,0x13,+02,0x14,+02,0x15,+02,0x16,+02,0x17,+02,
	0x18,+02,0x19,+02,0x1A,+02,0x1B,+02,0x1C,+02,0x1D,+02,0x1E,+02,0x1F,+00,
};
static Int8 env_xpulse_hold[] = 
{
	0x00,+02,0x01,+02,0x02,+02,0x03,+02,0x04,+02,0x05,+02,0x06,+02,0x07,+02,
	0x08,+02,0x09,+02,0x0A,+02,0x0B,+02,0x0C,+02,0x0D,+02,0x0E,+02,0x0F,+02,
	0x10,+02,0x11,+02,0x12,+02,0x13,+02,0x14,+02,0x15,+02,0x16,+02,0x17,+02,
	0x18,+02,0x19,+02,0x1A,+02,0x1B,+02,0x1C,+02,0x1D,+02,0x1E,+02,0x1F,+02,
	0x00,+00,
};
static Int8 env_xsaw[] = 
{
	0x00,+02,0x01,+02,0x02,+02,0x03,+02,0x04,+02,0x05,+02,0x06,+02,0x07,+02,
	0x08,+02,0x09,+02,0x0A,+02,0x0B,+02,0x0C,+02,0x0D,+02,0x0E,+02,0x0F,+02,
	0x10,+02,0x11,+02,0x12,+02,0x13,+02,0x14,+02,0x15,+02,0x16,+02,0x17,+02,
	0x18,+02,0x19,+02,0x1A,+02,0x1B,+02,0x1C,+02,0x1D,+02,0x1E,+02,0x1F,-62,
};

static Int8 env_xtri[] = 
{
	0x00,+02,0x01,+02,0x02,+02,0x03,+02,0x04,+02,0x05,+02,0x06,+02,0x07,+02,
	0x08,+02,0x09,+02,0x0A,+02,0x0B,+02,0x0C,+02,0x0D,+02,0x0E,+02,0x0F,+02,
	0x10,+02,0x11,+02,0x12,+02,0x13,+02,0x14,+02,0x15,+02,0x16,+02,0x17,+02,
	0x18,+02,0x19,+02,0x1A,+02,0x1B,+02,0x1C,+02,0x1D,+02,0x1E,+02,0x1F,+02,
	0x1F,+02,0x1E,+02,0x1D,+02,0x1C,+02,0x1B,+02,0x1A,+02,0x19,+02,0x18,+02,
	0x17,+02,0x16,+02,0x15,+02,0x14,+02,0x13,+02,0x12,+02,0x11,+02,0x10,+02,
	0x0F,+02,0x0E,+02,0x0D,+02,0x0C,+02,0x0B,+02,0x0A,+02,0x09,+02,0x08,+02,
	0x07,+02,0x06,+02,0x05,+02,0x04,+02,0x03,+02,0x02,+02,0x01,+02,0x00,-126,
};

#if 1
static Int8 *(env_table[16]) =
{
	env_pulse,       env_pulse,       env_pulse,       env_pulse,
	env_xpulse_hold, env_xpulse_hold, env_xpulse_hold, env_xpulse_hold,
	env_saw,         env_pulse,       env_tri,         env_pulse_hold,
	env_xsaw,        env_xpulse,      env_xtri,        env_xpulse_hold,
};
#else
static Int8 *(env_table[16]) =
{
	env_pulse,	env_pulse,	env_pulse,	env_pulse,
	env_xpulse,	env_xpulse,	env_xpulse,	env_xpulse,
	env_saw,	env_pulse,	env_tri,	env_pulse_hold,
	env_xsaw,	env_xpulse,	env_xtri,	env_xpulse_hold,
};
#endif

__inline static Uint32 PSGSoundNoiseStep(PSGSOUND *sndp)
{
	Uint32 spd;
	spd = sndp->noise.regs[0] & 0x1F;
	spd = spd ? (spd << CPS_SHIFT) : (1 << CPS_SHIFT);

	sndp->noise.cycles += sndp->noise.cps;
	while (sndp->noise.cycles >= spd)
	{
		sndp->noise.cycles -= spd;
		sndp->noise.noiserng += sndp->noise.noiserng + (1 & ((sndp->noise.noiserng >> 14) ^ (sndp->noise.noiserng >> 16)));
	}
	return (sndp->noise.noiserng >> 16) & 1;
}

__inline static Int32 PSGSoundEnvelopeStep(PSGSOUND *sndp)
{
	Uint32 spd;
	spd = (sndp->envelope.regs[1] << 8) + sndp->envelope.regs[0];

	if (spd)
	{
		spd <<= CPS_ENVSHIFT;
		sndp->envelope.cycles += sndp->envelope.cps;
		while (sndp->envelope.cycles >= spd)
		{
			sndp->envelope.cycles -= spd;
			sndp->envelope.adr += sndp->envelope.adr[1];
		}
	}

	if (sndp->envelope.adr[0] & sndp->envelope.adrmask)
	{
		return LogToLin(sndp->logtbl, ((sndp->envelope.adrmask - (sndp->envelope.adr[0] & sndp->envelope.adrmask)) << (LOG_BITS - 2 + 1)) + sndp->common.mastervolume, LOG_LIN_BITS - 21);
	}
	else
		return 0;
}

__inline static Uint32 PSGSoundSquareSub(PSGSOUND *sndp, PSG_SQUARE *chp)
{
	Int32 volume, bit = 1;
	Uint32 spd;
	spd = ((chp->regs[1] & 0x0F) << 8) + chp->regs[0];

	/* if (!spd) return 0; */

	if (spd/* > 7*/)
	{
		spd <<= CPS_SHIFT;
		chp->cycles += chp->cps;
		while (chp->cycles >= spd)
		{
			chp->cycles -= spd;
			chp->adr++;
		}
	}

	if (chp->mute || !chp->key) return 0;

	if (chp->regs[2] & 0x10)
		volume = sndp->common.envout;
	else if (chp->regs[2] & 0xF)
	{
		volume = LogToLin(sndp->logtbl, ((0xF - (chp->regs[2] & 0xF)) << (LOG_BITS - 1 + 1)) + sndp->common.mastervolume, LOG_LIN_BITS - 21);
	}
	else
		volume = 0;

	if (spd && (chp->key & 1))
	{
		bit ^= (chp->adr & 1) ^ 1;
	}
	if (chp->key & 2)
	{
		bit ^= sndp->common.rngout;
	}
	return bit ? volume: 0;
}

#if (((-1) >> 1) == -1)
#define SSR(x, y) (((Int32)x) >> (y))
#else
#define SSR(x, y) (((x) >= 0) ? ((x) >> (y)) : (-((-(x) - 1) >> (y)) - 1))
#endif

static Int32 PSGSoundSquare(PSGSOUND *sndp, PSG_SQUARE *chp)
{
	Int32 out;
	out = PSGSoundSquareSub(sndp, chp);
#if DCFIX
	chp->dcptr = (chp->dcptr + 1) & ((1 << DCFIX) - 1);
	chp->dcave -= chp->dcbuf[chp->dcptr];
	chp->dcave += out;
	chp->dcbuf[chp->dcptr] = out;
	out = out - (chp->dcave >> DCFIX);
#endif
#if ANAEX
	out = ((chp->anaex[0] << ANAEX) + ((out - chp->anaex[0]) + ((out - chp->anaex[1]) >> 3))) >> ANAEX;
	chp->anaex[1] = chp->anaex[0];
	chp->anaex[0] = out;
	out += out;
#endif
	return out + out;
}

static void sndsynth(PSGSOUND *sndp, Int32 *p)
{
	Int32 accum = 0;
	sndp->common.rngout = PSGSoundNoiseStep(sndp);
	sndp->common.envout = PSGSoundEnvelopeStep(sndp);
	accum += PSGSoundSquare(sndp, &sndp->square[0]);
	accum += PSGSoundSquare(sndp, &sndp->square[1]);
	accum += PSGSoundSquare(sndp, &sndp->square[2]);
	if (sndp->common.daenable)
		accum += LogToLin(sndp->logtbl, sndp->common.mastervolume + sndp->common.davolume, LOG_LIN_BITS - 21);
	p[0] += accum;
	p[1] += accum;
}

static void sndvolume(PSGSOUND *sndp, Int32 volume)
{
	volume = (volume << (LOG_BITS - 8)) << 1;
	sndp->common.mastervolume = volume;
}

__inline static Uint32 sndreadreg(PSGSOUND *sndp, Uint32 a)
{
	switch (a)
	{
		case 0x0: case 0x1:
		case 0x2: case 0x3:
		case 0x4: case 0x5:
			return sndp->square[a >> 1].regs[a & 1];
		case 0x6:
			return sndp->noise.regs[0];
		case 0x7:
			return sndp->common.regs[0];
		case 0x8: case 0x9: case 0xA:
			return sndp->square[a - 0x8].regs[2];
		case 0xB: case 0xC: case 0xD:
			return sndp->envelope.regs[a - 0xB];
	}
	return 0;
}

__inline static void sndwritereg(PSGSOUND *sndp, Uint32 a, Uint32 v)
{
	switch (a)
	{
		case 0x0: case 0x1:
		case 0x2: case 0x3:
		case 0x4: case 0x5:
			sndp->square[a >> 1].regs[a & 1] = v;
			break;
		case 0x6:
			sndp->noise.regs[0] = v;
			break;
		case 0x7:
			sndp->common.regs[0] = v;
			{
				Uint32 ch;
				for (ch = 0; ch < 3; ch++)
				{
					sndp->square[ch].key = 0;
					if (!(v & (1 << ch))) sndp->square[ch].key |= 1;
					if (!(v & (8 << ch))) sndp->square[ch].key |= 2;
				}
			}
			break;
		case 0x8: case 0x9: case 0xA:
			sndp->square[a - 0x8].regs[2] = v;
			break;
		case 0xD:
			sndp->envelope.adr = env_table[v & 0xF];
		case 0xB: case 0xC:
			sndp->envelope.regs[a - 0xB] = v;
			break;
	}
}

static Uint32 sndread(PSGSOUND *sndp, Uint32 a)
{
	return sndreadreg(sndp, sndp->common.adr);
}

static void sndwrite(PSGSOUND *sndp, Uint32 a, Uint32 v)
{
	switch (a & 3)
	{
		case 0:
			sndp->common.adr = v;
			break;
		case 1:
			sndwritereg(sndp, sndp->common.adr, v);
			break;
		case 2:
			sndp->common.daenable = v;
			break;
	}
}

static void PSGSoundSquareReset(PSG_SQUARE *ch, Uint32 clock, Uint32 freq)
{
	XMEMSET(ch, 0, sizeof(PSG_SQUARE));
	ch->cps = DivFix(clock, 16 * freq, CPS_SHIFT);
}

static void PSGSoundNoiseReset(PSG_NOISE *ch, Uint32 clock, Uint32 freq)
{
	XMEMSET(ch, 0, sizeof(PSG_NOISE));
	ch->cps = DivFix(clock, 2 * 16 * freq, CPS_SHIFT);
	ch->noiserng = 0x8fec;
}

static void PSGSoundEnvelopeReset(PSG_ENVELOPE *ch, Uint32 clock, Uint32 freq)
{
	XMEMSET(ch, 0, sizeof(PSG_ENVELOPE));
	ch->cps = DivFix(clock * 2, 16 * freq, CPS_ENVSHIFT);
	ch->adr = env_table[0];
}

static void sndreset(PSGSOUND *sndp, Uint32 clock, Uint32 freq)
{
	const static Uint8 initdata[] = { 0,0,0,0,0,0, 0, 0x3f, 0x0,0x0,0x0, 0,0,8 };
	Uint32 i;
	XMEMSET(&sndp->common, 0, sizeof(sndp->common));
	PSGSoundNoiseReset(&sndp->noise, clock, freq);
	PSGSoundEnvelopeReset(&sndp->envelope, clock, freq);
	PSGSoundSquareReset(&sndp->square[0], clock, freq);
	PSGSoundSquareReset(&sndp->square[1], clock, freq);
	PSGSoundSquareReset(&sndp->square[2], clock, freq);
	if (sndp->type == PSG_TYPE_AY_3_8910)
	{
		sndp->envelope.adrmask = 0x1e;
	}
	else
	{
		sndp->envelope.adrmask = 0x1f;
	}
	for (i = 0; i < sizeof(initdata); i++)
	{
		sndwrite(sndp, 0, i);			/* address */
		sndwrite(sndp, 1, initdata[i]);	/* data */
	}
}

static void sndrelease(PSGSOUND *sndp)
{
	if (sndp->logtbl) sndp->logtbl->release(sndp->logtbl->ctx);
	XFREE(sndp);
}

static void setinst(void *ctx, Uint32 n, void *p, Uint32 l){}

KMIF_SOUND_DEVICE *PSGSoundAlloc(Uint32 psg_type)
{
	PSGSOUND *sndp;
	sndp = XMALLOC(sizeof(PSGSOUND));
	if (!sndp) return 0;
	sndp->type = psg_type;
	sndp->kmif.ctx = sndp;
	sndp->kmif.release = sndrelease;
	sndp->kmif.reset = sndreset;
	sndp->kmif.synth = sndsynth;
	sndp->kmif.volume = sndvolume;
	sndp->kmif.write = sndwrite;
	sndp->kmif.read = sndread;
	sndp->kmif.setinst = setinst;
	sndp->logtbl = LogTableAddRef();
	if (!sndp->logtbl)
	{
		sndrelease(sndp);
		return 0;
	}
	return &sndp->kmif;
}
