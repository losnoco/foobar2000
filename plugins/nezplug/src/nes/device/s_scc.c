#include "kmsnddev.h"
#include "divfix.h"
#include "s_logtbl.h"
#include "s_scc.h"


#define CPS_SHIFT 18
#define LOG_KEYOFF (31 << (LOG_BITS + 1))

typedef struct {
	Uint32 cycles;
	Uint32 spd;
	Uint32 tone[32];
	Uint32 volume;
	Uint8 regs[3];
	Uint8 adr;
	Uint8 mute;
	Uint8 key;
	Uint8 pad4[2];
} SCC_CH;

typedef struct {
	KMIF_SOUND_DEVICE kmif;
	KMIF_LOGTABLE *logtbl;
	SCC_CH ch[5];
	Uint32 majutushida;
	struct {
		Uint32 cps;
		Int32 mastervolume;
		Uint8 mode;
		Uint8 enable;
	} common;
} SCCSOUND;

__inline static Int32 SCCSoundChSynth(SCCSOUND *sndp, SCC_CH *ch)
{

	if (ch->spd <= (9 << CPS_SHIFT)) return 0;

	ch->cycles += sndp->common.cps;
	while (ch->cycles >= ch->spd)
	{
		ch->cycles -= ch->spd;
		ch->adr++;
	}

	if (ch->mute || !ch->key) return 0;
	return LogToLin(sndp->logtbl, ch->volume + sndp->common.mastervolume + ch->tone[ch->adr & 0x1F], LOG_LIN_BITS - LIN_BITS - LIN_BITS - 9);
}

__inline static void SCCSoundChReset(SCC_CH *ch, Uint32 clock, Uint32 freq)
{
	XMEMSET(ch, 0, sizeof(SCC_CH));
}

static void sndsynth(SCCSOUND *sndp, Int32 *p)
{
	if (sndp->common.enable)
	{
		Uint32 ch;
		Int32 accum = 0;
		for (ch = 0; ch < 5; ch++) accum += SCCSoundChSynth(sndp, &sndp->ch[ch]);
		accum += LogToLin(sndp->logtbl, sndp->common.mastervolume + sndp->majutushida, LOG_LIN_BITS - LIN_BITS - 14);
		p[0] += accum;
		p[1] += accum;

	}
}

static void sndvolume(SCCSOUND *sndp, Int32 volume)
{
	volume = (volume << (LOG_BITS - 8)) << 1;
	sndp->common.mastervolume = volume;
}

static Uint32 sndread(SCCSOUND *sndp, Uint32 a)
{
	return 0;
}

static void sndwrite(SCCSOUND *sndp, Uint32 a, Uint32 v)
{
	if (a == 0xbffe || a == 0xbfff)
	{
		sndp->common.mode = v;
	}
	else if ((0x9800 <= a && a <= 0x985F) || (0xB800 <= a && a <= 0xB89F))
	{
		Uint32 tone = LinToLog(sndp->logtbl, ((Int32)(v ^ 0x80)) - 0x80);
		sndp->ch[(a & 0xE0) >> 5].tone[a & 0x1F] = tone;
	}
	else if (0x9860 <= a && a <= 0x987F)
	{
		Uint32 tone = LinToLog(sndp->logtbl, ((Int32)(v ^ 0x80)) - 0x80);
		sndp->ch[3].tone[a & 0x1F] = sndp->ch[4].tone[a & 0x1F] = tone;
	}
	else if ((0x9880 <= a && a <= 0x988F) || (0xB8A0 <= a && a <= 0xB8AF))
	{
		Uint32 port = a & 0x0F;
		if (0x0 <= port && port <= 0x9)
		{
			SCC_CH *ch = &sndp->ch[port >> 1];
			ch->regs[port & 0x1] = v;
			ch->spd = (((ch->regs[1] & 0x0F) << 8) + ch->regs[0] + 1) << CPS_SHIFT;
		}
		else if (0xA <= port && port <= 0xE)
		{
			SCC_CH *ch = &sndp->ch[port - 0xA];
			ch->regs[2] = v;
			ch->volume = LinToLog(sndp->logtbl, ch->regs[2] & 0xF);
		}
		else
		{
			Uint32 i;
			if (v & 0x1f) sndp->common.enable = 1;
			for (i = 0; i < 5; i++) sndp->ch[i].key = (v & (1 << i));
		}
	}
	else if (0x5000 <= a && a <= 0x5FFF)
	{
		sndp->common.enable = 1;
		sndp->majutushida = LinToLog(sndp->logtbl, ((Int32)(v ^ 0x00)) - 0x80);
	}
}

static void sndreset(SCCSOUND *sndp, Uint32 clock, Uint32 freq)
{
	Uint32 ch;
	XMEMSET(&sndp->common,  0, sizeof(sndp->common));
	sndp->common.cps = DivFix(clock, freq, CPS_SHIFT);
	for (ch = 0; ch < 5; ch++) SCCSoundChReset(&sndp->ch[ch], clock, freq);
	for (ch = 0x9800; ch < 0x988F; ch++) sndwrite(sndp, ch, 0);
	sndp->majutushida = LOG_KEYOFF;
}

static void sndrelease(SCCSOUND *sndp)
{
	if (sndp->logtbl) sndp->logtbl->release(sndp->logtbl->ctx);
	XFREE(sndp);
}

static void setinst(void *ctx, Uint32 n, void *p, Uint32 l){}

KMIF_SOUND_DEVICE *SCCSoundAlloc(void)
{
	SCCSOUND *sndp;
	sndp = XMALLOC(sizeof(SCCSOUND));
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
