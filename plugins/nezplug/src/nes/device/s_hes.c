#include "kmsnddev.h"
#include "divfix.h"
#include "s_hes.h"
#include "s_logtbl.h"

#define CPS_SHIFT 16
#define LOG_KEYOFF (32 << LOG_BITS)
#define LFO_BASE (0x10 << 8)

#define NOISE_EDGE 1

typedef struct {
	Uint32 cps;				/* cycles per sample */
	Uint32 pt;				/* programmable timer */
	Uint32 wl;				/* wave length */
	Uint32 npt;				/* noise programmable timer */
	Uint32 nvol;			/* noise volume */
	Uint32 rng;				/* random number generator (seed) */
	Uint32 dda;				/* direct D/A output */
	Uint32 tone[0x20];		/* tone waveform */
	Uint32 lfooutput;		/* lfo output */
	Uint32 nwl;				/* noise wave length */
	Uint8 regs[8 - 2];		/* registers per channel */
	Uint8 st;				/* wave step */
	Uint8 tonep;			/* tone waveform write pointer*/
	Uint8 mute;
#if NOISE_EDGE
	Uint8 edge;
	Uint8 edgeout;
	Uint8 rngold;
#endif
} HES_WAVEMEMORY;

typedef struct {
	Uint32 cps;			/* cycles per sample */
	Uint32 pt;			/* lfo programmable timer */
	Uint32 wl;			/* lfo wave length */
	Uint8 tone[0x20];	/* lfo waveform */
	Uint8 tonep;		/* lfo waveform write pointer */
	Uint8 st;			/* lfo step */
	Uint8 update;
	Uint8 regs[2];
} HES_LFO;

typedef struct {
	KMIF_SOUND_DEVICE kmif;
	KMIF_LOGTABLE *logtbl;
	HES_WAVEMEMORY ch[6];
	HES_LFO lfo;
	HES_WAVEMEMORY *cur;
	struct {
		Int32 mastervolume;
		Uint8 sysregs[2];
	} common;
} HESSOUND;

#define V(a) ((((Uint32)(0x1F - (a)) * (Uint32)(1 << LOG_BITS) * (Uint32)1000) / (Uint32)5786) << 1)
const static Uint32 voltbl[0x20] = {
	LOG_KEYOFF,
	         V(0x01), V(0x02),V(0x03),V(0x04), V(0x05), V(0x06),V(0x07),
	V(0x08), V(0x09), V(0x0A),V(0x0B),V(0x0C), V(0x0D), V(0x0E),V(0x0F),
	V(0x10), V(0x11), V(0x12),V(0x13),V(0x14), V(0x15), V(0x16),V(0x17),
	V(0x18), V(0x19), V(0x1A),V(0x1B),V(0x1C), V(0x1D), V(0x1E),V(0x1F),
};
const static Uint32 voltbl2[0x20] = {
	V(0x10)+1, V(0x0F)+1, V(0x0E)+1,V(0x0D)+1,V(0x0C)+1, V(0x0B)+1, V(0x0A)+1,V(0x09)+1,
	V(0x08)+1, V(0x07)+1, V(0x06)+1,V(0x05)+1,V(0x04)+1, V(0x03)+1, V(0x02)+1,V(0x01)+1,
	LOG_KEYOFF,
	         V(0x01), V(0x02),V(0x03),V(0x04), V(0x05), V(0x06),V(0x07),
	V(0x08), V(0x09), V(0x0A),V(0x0B),V(0x0C), V(0x0D), V(0x0E),V(0x0F),
};
#undef V

#if HES_TONE_DEBUG_OPTION_ENABLE
Uint8 HES_tone_debug_option = 0;
#endif
Uint8 HES_noise_debug_option1 = 6;
Uint8 HES_noise_debug_option2 = 10;
Int32 HES_noise_debug_option3 = 3;
Int32 HES_noise_debug_option4 = 508;

static void HESSoundWaveMemoryRender(HESSOUND *sndp, HES_WAVEMEMORY *ch, Int32 *p)
{
	Uint32 wl, output, lvol, rvol;
	if (ch->mute || !(ch->regs[4 - 2] & 0x80)) return;
	lvol = voltbl[(sndp->common.sysregs[1] >> 3) & 0x1E];
	lvol +=	voltbl[(ch->regs[5 - 2] >> 3) & 0x1E];
	lvol += voltbl[ch->regs[4 - 2] & 0x1F];
	rvol = voltbl[(sndp->common.sysregs[1] << 1) & 0x1E];
	rvol += voltbl[(ch->regs[5 - 2] << 1) & 0x1E];
	rvol += voltbl[ch->regs[4 - 2] & 0x1F];

	if (ch->regs[4 - 2] & 0x40)	/* DDA */
	{
		output = ch->dda;
	}
	else if (ch->regs[7 - 2] & 0x80)	/* NOISE */
	{
		/* if (wl == 0) return; */
		ch->npt += ch->cps;
		while (ch->npt >= ch->nwl)
		{
			ch->npt -= ch->nwl;
			ch->rng <<= 1;
			ch->rng ^= ((ch->rng >> 20) & 1) + ((ch->rng >> 18) & 8);
#if NOISE_EDGE
			ch->edge ^=/*|=*/ (ch->rng ^ ch->rngold) & 1;
			ch->rngold = ch->rng;
#endif
		}
#if NOISE_EDGE
		ch->edgeout ^= ch->edge;
		ch->edge = 0;
		output = ch->nvol + ch->edgeout;
#else
		output = ch->nvol + ((ch->rng >> 20) & 1);
#endif
	}
	else
	{
		wl = ch->wl + ch->lfooutput;
		/* if (wl <= (LFO_BASE + 16)) wl = (LFO_BASE + 16); */
		if (wl <= (LFO_BASE + 4)) return;
		wl = (wl - LFO_BASE) << CPS_SHIFT;
		ch->pt += ch->cps;
		while (ch->pt >= wl)
		{
			ch->pt -= wl;
			ch->st++;
		}
		output = ch->tone[ch->st & 0x1f];
	}
	p[0] += LogToLin(sndp->logtbl, lvol + output + sndp->common.mastervolume, LOG_LIN_BITS - LIN_BITS - 17 - 1);
	p[1] += LogToLin(sndp->logtbl, rvol + output + sndp->common.mastervolume, LOG_LIN_BITS - LIN_BITS - 17 - 1);
}

static void HESSoundLfoStep(HESSOUND *sndp)
{
	if (sndp->lfo.update & 1)
	{
		sndp->lfo.update &= ~1;
		sndp->lfo.wl = sndp->ch[1].regs[2 - 2] + ((sndp->ch[1].regs[3 - 2] & 0xf) << 8);
		sndp->lfo.wl *= sndp->lfo.regs[0];
		sndp->lfo.wl <<= CPS_SHIFT;
	}
	if (sndp->lfo.wl <= (16 << CPS_SHIFT))
	{
		sndp->ch[0].lfooutput = LFO_BASE;
		return;
	}
	sndp->lfo.pt += sndp->lfo.cps;
	while (sndp->lfo.pt >= sndp->lfo.wl)
	{
		sndp->lfo.pt -= sndp->lfo.wl;
		sndp->lfo.st++;
	}
	sndp->ch[0].lfooutput = sndp->lfo.tone[sndp->lfo.st & 0x1f];
	switch (sndp->lfo.regs[1] & 3)
	{
		case 0:
			sndp->ch[0].lfooutput = LFO_BASE;
			break;
		case 1:
			sndp->ch[0].lfooutput += LFO_BASE - (0x10 << 0);
			break;
		case 2:
			sndp->ch[0].lfooutput <<= 4;
			sndp->ch[0].lfooutput += LFO_BASE - (0x10 << 4);
			break;
		case 3:
			sndp->ch[0].lfooutput <<= 8;
			/* sndp->ch[0].lfooutput += LFO_BASE - (0x10 << 8); */
			break;
	}
}

static void sndsynth(HESSOUND *sndp, Int32 *p)
{
	HESSoundWaveMemoryRender(sndp, &sndp->ch[5], p);
	HESSoundWaveMemoryRender(sndp, &sndp->ch[4], p);
	HESSoundWaveMemoryRender(sndp, &sndp->ch[3], p);
	HESSoundWaveMemoryRender(sndp, &sndp->ch[2], p);
	if (sndp->lfo.regs[1] & 0x80)
	{
		HESSoundLfoStep(sndp);
	}
	else
	{
		HESSoundWaveMemoryRender(sndp, &sndp->ch[1], p);
		sndp->ch[0].lfooutput = LFO_BASE;
	}
	HESSoundWaveMemoryRender(sndp, &sndp->ch[0], p);
}

static void HESSoundChReset(HESSOUND *sndp, HES_WAVEMEMORY *ch, Uint32 clock, Uint32 freq)
{
	int i;
	XMEMSET(ch, 0, sizeof(HES_WAVEMEMORY));
	ch->cps = DivFix(clock, 6 * freq, CPS_SHIFT);
	ch->nvol = LinToLog(sndp->logtbl, 10);
	ch->dda = LOG_KEYOFF;
	ch->rng = 0x51f631e4;
	ch->lfooutput = LFO_BASE;
	for (i = 0; i < 0x20; i++) ch->tone[i] = LOG_KEYOFF;
}

static void HESSoundLfoReset(HES_LFO *ch, Uint32 clock, Uint32 freq)
{
	XMEMSET(ch, 0, sizeof(HES_LFO));
	ch->cps = DivFix(clock, 6 * freq, CPS_SHIFT);
	/* ch->regs[1] = 0x80; */
}


static void sndreset(HESSOUND *sndp, Uint32 clock, Uint32 freq)
{
	Uint32 ch;
	XMEMSET(&sndp->common, 0, sizeof(sndp->common));
	sndp->cur = sndp->ch;
	sndp->common.sysregs[1] = 0xFF;
	for (ch = 0; ch < 6; ch++) HESSoundChReset(sndp, &sndp->ch[ch], clock, freq);
	HESSoundLfoReset(&sndp->lfo, clock, freq);
}

static void sndwrite(HESSOUND *sndp, Uint32 a, Uint32 v)
{
	switch (a & 0xF)
	{
		case 0:
			sndp->common.sysregs[0] = v & 7;
			if (sndp->common.sysregs[0] <= 5)
				sndp->cur = &sndp->ch[sndp->common.sysregs[0]];
			else
				sndp->cur = 0;
			break;
		case 1:
			sndp->common.sysregs[1] = v;
			break;
		case 2:	case 3:
			if (sndp->cur)
			{
				sndp->cur->regs[a - 2] = v;
				sndp->cur->wl = sndp->cur->regs[2 - 2] + ((sndp->cur->regs[3 - 2] & 0xf) << 8);
				if (sndp->cur == &sndp->ch[1]) sndp->lfo.update |= 1;
			}
			break;
		case 4:	case 5:
			if (sndp->cur) sndp->cur->regs[a - 2] = v;
			break;
		case 6:
			if (sndp->cur)
			{
				Uint32 tone;
				Int32 data = v & 0x1f;
				sndp->cur->regs[6 - 2] = v;
#if HES_TONE_DEBUG_OPTION_ENABLE
				switch (HES_tone_debug_option)
				{
					default:
					case 0:
						tone = LinToLog(sndp->logtbl, data - 0x10) + (1 << (LOG_BITS + 1));
						break;
					case 1:
						tone = voltbl2[data];
						break;
					case 2:
						tone = LinToLog(sndp->logtbl, data) + (1 << (LOG_BITS + 1));
						break;
				}
#else
				tone = LinToLog(sndp->logtbl, data - 0x10) + (1 << (LOG_BITS + 1));
#endif
				if (sndp->cur->regs[4 - 2] & 0x40)
				{
					sndp->cur->dda = tone;
#if 1
					if (sndp->cur == &sndp->ch[1]) sndp->lfo.tonep = 0x0;
					sndp->cur->tonep = 0;
#endif
				}
				else
				{
					sndp->cur->dda = LOG_KEYOFF;
					if (sndp->cur == &sndp->ch[1])
					{
						sndp->lfo.tone[sndp->lfo.tonep] = data ^ 0x10;
						if (++sndp->lfo.tonep == 0x20) sndp->lfo.tonep = 0;
					}
					sndp->cur->tone[sndp->cur->tonep] = tone;
					if (++sndp->cur->tonep == 0x20) sndp->cur->tonep = 0;
				}
			}
			break;
		case 7:
			if (sndp->cur && sndp->common.sysregs[0] >= 4)
			{
				Uint32 nwl;
				sndp->cur->regs[7 - 2] = v;
				switch (HES_noise_debug_option1)
				{
				case 1:
					/* v0.9.3beta7 old linear frequency */
					/* HES_noise_debug_option1=1 */
					/* HES_noise_debug_option2=HES_noise_debug_option(default:5) */
					/* HES_noise_debug_option3=512 */
					/* HES_noise_debug_option5=0 */
					nwl = HES_noise_debug_option4 << (CPS_SHIFT - 10);
					nwl += (((v & 0x1f) << 9) + HES_noise_debug_option3) << (CPS_SHIFT - 9 + HES_noise_debug_option2);
					break;
				case 2:
					/* linear frequency */
					nwl = HES_noise_debug_option4 << (CPS_SHIFT - 10);
					nwl += (v & 0x1f) << (CPS_SHIFT - 10 + HES_noise_debug_option2 - HES_noise_debug_option3);
					break;
				default:
				case 3:
					/* positive logarithmic frequency */
					nwl = HES_noise_debug_option4 << (CPS_SHIFT - 10);
					nwl -= LogToLin(sndp->logtbl, ((0 & 0x1f) ^ 0x1f) << (LOG_BITS - HES_noise_debug_option3 + 1), LOG_LIN_BITS - CPS_SHIFT - HES_noise_debug_option2);
					nwl += LogToLin(sndp->logtbl, ((v & 0x1f) ^ 0x1f) << (LOG_BITS - HES_noise_debug_option3 + 1), LOG_LIN_BITS - CPS_SHIFT - HES_noise_debug_option2);
					break;
				case 4:
					/* negative logarithmic frequency */
					nwl = HES_noise_debug_option4 << (CPS_SHIFT - 10);
					nwl += LogToLin(sndp->logtbl, ((0 & 0x1f) ^ 0x00) << (LOG_BITS - HES_noise_debug_option3 + 1), LOG_LIN_BITS - CPS_SHIFT - HES_noise_debug_option2);
					nwl -= LogToLin(sndp->logtbl, ((v & 0x1f) ^ 0x00) << (LOG_BITS - HES_noise_debug_option3 + 1), LOG_LIN_BITS - CPS_SHIFT - HES_noise_debug_option2);
					break;
				case 5:
					/* positive logarithmic frequency (reverse) */
					nwl = HES_noise_debug_option4 << (CPS_SHIFT - 10);
					nwl -= LogToLin(sndp->logtbl, ((0 & 0x1f) ^ 0x1f) << (LOG_BITS - HES_noise_debug_option3 + 1), LOG_LIN_BITS - CPS_SHIFT - HES_noise_debug_option2);
					nwl += LogToLin(sndp->logtbl, ((v & 0x1f) ^ 0x00) << (LOG_BITS - HES_noise_debug_option3 + 1), LOG_LIN_BITS - CPS_SHIFT - HES_noise_debug_option2);
					break;
				case 6:
					/* negative logarithmic frequency (reverse) */
					nwl = HES_noise_debug_option4 << (CPS_SHIFT - 10);
					nwl += LogToLin(sndp->logtbl, ((0 & 0x1f) ^ 0x00) << (LOG_BITS - HES_noise_debug_option3 + 1), LOG_LIN_BITS - CPS_SHIFT - HES_noise_debug_option2);
					nwl -= LogToLin(sndp->logtbl, ((v & 0x1f) ^ 0x1f) << (LOG_BITS - HES_noise_debug_option3 + 1), LOG_LIN_BITS - CPS_SHIFT - HES_noise_debug_option2);
					break;
				case 7:
					/* v0.9.3beta8 old logarithmic frequency type B */
					nwl = HES_noise_debug_option4 << (CPS_SHIFT - 10);
					nwl += LogToLin(sndp->logtbl, ((v & 0x1f) ^ 0x1f) << (LOG_BITS - HES_noise_debug_option3 + 1), LOG_LIN_BITS - CPS_SHIFT - HES_noise_debug_option2);
					break;
				case 8:
					/* v0.9.3beta8 old logarithmic frequency type C */
					/* HES_noise_debug_option1=3 */
					/* HES_noise_debug_option2=13 */
					/* HES_noise_debug_option3=2 */
					/* HES_noise_debug_option4=0 */
					nwl = HES_noise_debug_option4;
					nwl += LogToLin(sndp->logtbl, (v & 0x1f) << (LOG_BITS - HES_noise_debug_option3 + 1), LOG_LIN_BITS - CPS_SHIFT - HES_noise_debug_option2);
					break;
				}
				sndp->cur->npt = 0;
				sndp->cur->nwl = nwl;
			}
			break;
		case 8:
			sndp->lfo.regs[a - 8] = v;
			sndp->lfo.update |= 1;
			break;
		case 9:
			sndp->lfo.regs[a - 8] = v;
			break;
	}
}

static Uint32 sndread(HESSOUND *sndp, Uint32 a)
{
	a &= 0xF;
	switch (a & 0xF)
	{
		case 0:	case 1:
			return sndp->common.sysregs[a];
		case 2:	case 3:	case 4:	case 5:	case 6:	case 7:
			if (sndp->cur) return sndp->cur->regs[a - 2];
			return 0;
		case 8:	case 9:
			return sndp->lfo.regs[a - 8];
	}
	return 0;
}

static void sndvolume(HESSOUND *sndp, Int32 volume)
{
	volume = (volume << (LOG_BITS - 8)) << 1;
	sndp->common.mastervolume = volume;
}

static void sndrelease(HESSOUND *sndp)
{
	if (sndp->logtbl) sndp->logtbl->release(sndp->logtbl->ctx);
	XFREE(sndp);
}

static void setinst(void *ctx, Uint32 n, void *p, Uint32 l){}

KMIF_SOUND_DEVICE *HESSoundAlloc(void)
{
	HESSOUND *sndp;
	sndp = XMALLOC(sizeof(HESSOUND));
	if (!sndp) return 0;
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
