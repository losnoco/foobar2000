#include "kmsnddev.h"
#include "divfix.h"
#include "s_logtbl.h"
#include "s_dmg.h"

#define CPS_BITS 11
#define CPFWM_BITS 4

#define NOISE_EDGE 1

#define DMGSOUND_IS_SIGNED 0
/* #define DMGSOUND_IS_SIGNED 0 */ /* real DMG-SYSTEM output */
/* #define DMGSOUND_IS_SIGNED 1 */ /* modified output */

#define RELEASE_SPEED 3
/* #define RELEASE_SPEED 0 */ /* disable */
/* #define RELEASE_SPEED 3 */ /* default */

/* -------------------- */
/*  DMG INTERNAL SOUND  */
/* -------------------- */

typedef struct {
	Uint32 initial_counter;	/* length initial counter */
	Uint32 counter;			/* length counter */
	Uint8 enable;			/* length counter clock disable */
} LENGTHCOUNTER;
typedef struct {
	Uint8 rate;				/* envelope rate */
	Uint8 direction;		/* sweep direction */
	Uint8 timer;			/* envelope timer */
	Uint8 volume;			/* current volume */
	Uint8 initial_volume;	/* initial volume */
} ENVELOPEDECAY;
typedef struct {
	Uint8 rate;			/* sweep rate */
	Uint8 direction;	/* sweep direction */
	Uint8 timer;		/* sweep timer */
	Uint8 shifter;		/* sweep shifter */
	Uint32 wl;			/* wave length */
} SWEEP;
typedef struct {
	LENGTHCOUNTER lc;
	ENVELOPEDECAY ed;
	SWEEP sw;
	Int32 mastervolume;
#if RELEASE_SPEED
	Uint32 release;
#endif
	Uint32 cps;		/* cycles per sample */
	Uint32 cpf;	/* cycles per frame (240/192Hz) ($4017.bit7) */
	Uint32 fc;		/* frame counter; */
	Uint32 pt;		/* programmable timer */
	Uint8 st;		/* wave step */
	Uint8 fp;		/* frame position; */
	Uint8 duty;		/* frame position; */
	Uint8 key;
	Uint8 mute;
} DMG_SQUARE;

typedef struct {
	LENGTHCOUNTER lc;
	Int32 mastervolume;
#if RELEASE_SPEED
	Uint32 release;
#endif
	Uint32 cps;		/* cycles per sample */
	Uint32 cpf;		/* cycles per frame (240/192Hz) ($4017.bit7) */
	Uint32 fc;		/* frame counter */
	Uint32 pt;		/* programmable timer */
	Uint32 wl;		/* wave length */
	Uint8 st;		/* wave step */
	Uint8 fp;		/* frame position */
	Uint8 volume;
	Uint8 on;
	Uint8 key;
	Uint8 mute;
	Uint8 tone[32];
} DMG_WAVEMEMORY;

typedef struct {
	LENGTHCOUNTER lc;
	ENVELOPEDECAY ed;
	Int32 mastervolume;
	Uint32 cps;		/* cycles per sample */
	Uint32 cpf;		/* cycles per frame (240/192Hz) ($4017.bit7) */
	Uint32 fc;		/* frame counter */
	Uint32 pt;		/* programmable timer */
	Uint32 shift;
	Uint32 divratio;
	Uint32 rng;
	Uint8 fp;		/* frame position */
	Uint8 step1;
	Uint8 step2;
	Uint8 key;
	Uint8 mute;
#if NOISE_EDGE
	Uint8 edge;
	Uint8 edgeout;
	Uint8 rngold;
#endif
} DMG_NOISE;

typedef struct {
	KMIF_SOUND_DEVICE kmif;
	KMIF_LOGTABLE *logtbl;
	DMG_SQUARE square[2];
	DMG_WAVEMEMORY wavememory;
	DMG_NOISE noise;
	struct {
		Uint8 regs[0x30];
	} common;
} DMGSOUND;

const static Uint8 square_duty_table[4] = { 1, 2, 4, 6};
const static Uint8 wavememory_volume_table[4] = { 6, 0, 1, 2};
const static Uint8 noise_divratio_table[8] = { 1,2,4,6,8,10,12,14 };

static void LengthCounterStep(LENGTHCOUNTER *lc, Uint8 *key)
{
	if (lc->enable && lc->counter && !--lc->counter) *key = 0;
}

static void EnvelopeDecayStep(ENVELOPEDECAY *ed)
{
	if (ed->rate && ++ed->timer >= ed->rate)
	{
		ed->timer = 0;
		if (ed->direction)
		{
			ed->volume += (ed->volume < 0xf);
		}
		else if (ed->volume)
		{
			ed->volume--;
		}
	}
}

static void SweepStep(SWEEP *sw, Uint8 *key)
{
	if (sw->rate && sw->shifter && ++sw->timer >= sw->rate)
	{
		sw->timer = 0;
		if (sw->direction)
		{
			sw->wl -= sw->wl >> sw->shifter;
		}
		else
		{
			sw->wl += sw->wl >> sw->shifter;
			if (sw->wl > 0x7ff) *key = 0;
		}
	}
}

static Int32 DMGSoundSquareRender(DMGSOUND *sndp, DMG_SQUARE *ch)
{
	Uint32 wl;
	ch->fc += ch->cps;
	while (ch->fc >= ch->cpf)
	{
		ch->fc -= ch->cpf;
		if (!(ch->fp & 3)) EnvelopeDecayStep(&ch->ed);		/* 64Hz */
		if (!(ch->fp & 1)) SweepStep(&ch->sw, &ch->key);	/* 128Hz */
		LengthCounterStep(&ch->lc, &ch->key);				/* 256Hz */
		ch->fp++;
	}
	if (!ch->sw.wl || ch->sw.wl >= 0x7ff) return 0;
	wl = (0x800 - ch->sw.wl) << CPS_BITS;
	ch->pt += ch->cps;
	while (ch->pt >= wl)
	{
		ch->pt -= wl;
		ch->st++;
	}
	ch->st &= 7;
	if (ch->mute) return 0;
#if DMGSOUND_IS_SIGNED
	wl = LinToLog(sndp->logtbl, ch->ed.volume) + ch->mastervolume + (ch->st >= ch->duty) + (1 << (LOG_BITS + 1));
#else
	wl = (ch->st >= ch->duty) ? ch->ed.volume : 0;
	wl = LinToLog(sndp->logtbl, wl) + ch->mastervolume;
#endif
#if RELEASE_SPEED
	if (!ch->key)
	{
		if (ch->release > (32 << (LOG_BITS + 1))) return 0;
		ch->release += RELEASE_SPEED << (LOG_BITS - 8 + 1);
		wl += ch->release;
	}
	else
	{
		ch->release = 0;
	}
#else
	if (!ch->key) return 0;
#endif
	return LogToLin(sndp->logtbl, wl, LOG_LIN_BITS - LIN_BITS - 14);
}

static Int32 DMGSoundWaveMemoryRender(DMGSOUND *sndp, DMG_WAVEMEMORY *ch)
{
	Uint32 wl;
	ch->fc += ch->cps >> CPS_BITS;
	while (ch->fc >= ch->cpf)
	{
		ch->fc -= ch->cpf;
		LengthCounterStep(&ch->lc, &ch->key);				/* 256Hz */
		ch->fp++;
	}
	if (!ch->wl || ch->wl >= 0x7ff) return 0;
	wl = (0x800 - ch->wl) << CPS_BITS;
	ch->pt += ch->cps;
	while (ch->pt >= wl)
	{
		ch->pt -= wl;
		ch->st++;
	}
	ch->st &= 0x1f;
	if (ch->mute) return 0;
#if DMGSOUND_IS_SIGNED && 0
	if (ch->tone[ch->st] & 0x20)
		wl = LinToLog(sndp->logtbl, (ch->tone[ch->st] & 0x1c) >> ch->volume) + ch->mastervolume;
	else
		wl = 1 + LinToLog(sndp->logtbl, ((ch->tone[ch->st] & 0x1c) ^ 0x1c) >> ch->volume) + ch->mastervolume;
#else
	wl = ch->tone[ch->st] >> ch->volume;
	wl = LinToLog(sndp->logtbl, wl) + ch->mastervolume;
#endif
#if RELEASE_SPEED
	if (!ch->key || !ch->on)
	{
		if (ch->release > (32 << (LOG_BITS + 1))) return 0;
		ch->release += RELEASE_SPEED << (LOG_BITS - 8 + 1);
		wl += ch->release;
	}
	else
	{
		ch->release = 0;
	}
#else
	if (!ch->key || !ch->on) return 0;
#endif
	return LogToLin(sndp->logtbl, wl, LOG_LIN_BITS - LIN_BITS - 11);
}

static Int32 DMGSoundNoiseRender(DMGSOUND *sndp, DMG_NOISE *ch)
{
	Uint32 wl;
	ch->fc += ch->cps;
	while (ch->fc >= ch->cpf)
	{
		ch->fc -= ch->cpf;
		if (!(ch->fp & 3)) EnvelopeDecayStep(&ch->ed);		/* 64Hz */
		LengthCounterStep(&ch->lc, &ch->key);				/* 256Hz */
		ch->fp++;
	}
	if (ch->mute || !ch->key) return 0;
	ch->pt += ch->cps >> CPS_BITS;
	while (ch->pt >= ch->divratio)
	{
		ch->pt -= ch->divratio;
		ch->rng += ch->rng + (((ch->rng >> ch->step1) ^ (ch->rng >> ch->step2)) & 1);
#if NOISE_EDGE
		ch->edge ^=/*|=*/ (ch->rng ^ ch->rngold) & 1;
		ch->rngold = ch->rng;
#endif
	}
#if NOISE_EDGE
	ch->edgeout ^= ch->edge;
	ch->edge = 0;
#if DMGSOUND_IS_SIGNED
	wl = LinToLog(sndp->logtbl, ch->ed.volume) + ch->mastervolume + ch->edgeout + (1 << (LOG_BITS + 1));
#else
	wl = ch->edgeout ? ch->ed.volume : 0;
	wl = LinToLog(sndp->logtbl, wl) + ch->mastervolume;
#endif
#else
#if DMGSOUND_IS_SIGNED
	wl = LinToLog(sndp->logtbl, ch->ed.volume) + ch->mastervolume + ((ch->rng >> ch->step1) & 1) + (1 << (LOG_BITS + 1));
#else
	wl = ((ch->rng >> ch->step1) & 1) ? ch->ed.volume : 0;
	wl = LinToLog(sndp->logtbl, wl) + ch->mastervolume;
#endif
#endif
	return LogToLin(sndp->logtbl, wl, LOG_LIN_BITS - LIN_BITS - 14);
}

static void sndvolume(DMGSOUND *sndp, Int32 volume)
{
	volume = (volume << (LOG_BITS - 8)) << 1;
	sndp->square[0].mastervolume = volume;
	sndp->square[1].mastervolume = volume;
	sndp->wavememory.mastervolume = volume;
	sndp->noise.mastervolume = volume;
}

static void sndsynth(DMGSOUND *sndp, Int32 *p)
{
	Int32 b[2] = { 0, 0 };
	Int32 outputch;
	if (!(sndp->common.regs[0x16] & 0x80)) return;
	outputch = DMGSoundSquareRender(sndp, &sndp->square[0]);
	if ((sndp->common.regs[0x15] & 0x10)) b[0] += outputch;
	if ((sndp->common.regs[0x15] & 0x01)) b[1] += outputch;
	outputch = DMGSoundSquareRender(sndp, &sndp->square[1]);
	if ((sndp->common.regs[0x15] & 0x20)) b[0] += outputch;
	if ((sndp->common.regs[0x15] & 0x02)) b[1] += outputch;
	outputch = DMGSoundWaveMemoryRender(sndp, &sndp->wavememory);
	if ((sndp->common.regs[0x15] & 0x40)) b[0] += outputch;
	if ((sndp->common.regs[0x15] & 0x04)) b[1] += outputch;
	outputch = DMGSoundNoiseRender(sndp, &sndp->noise);
	if ((sndp->common.regs[0x15] & 0x80)) b[0] += outputch;
	if ((sndp->common.regs[0x15] & 0x08)) b[1] += outputch;
	p[0] += b[0] * ((sndp->common.regs[0x14] & 0x70) >> 4) + 1;
	p[1] += b[1] * (sndp->common.regs[0x14] & 0x07) + 1;
}

static void sndwrite(DMGSOUND *sndp, Uint32 a, Uint32 v)
{
	Uint32 ch;
	sndp->common.regs[a - 0xff10] = v;
	if (0xff30 <= a/* && a <= 0xff3f*/)
	{
		sndp->wavememory.tone[((a - 0xff30) << 1) + 0] = (v >> 2) & 0x3c;
		sndp->wavememory.tone[((a - 0xff30) << 1) + 1] = (v << 2) & 0x3c;
		return;
	}
	switch (a)
	{
		case 0xff10:
			sndp->square[0].sw.rate = (v >> 4) & 7;
			sndp->square[0].sw.direction = v & 8;
			sndp->square[0].sw.shifter = v & 7;
			break;
		case 0xff11:
		case 0xff16:
			ch = a >= 0xff16;
			sndp->square[ch].lc.initial_counter = 0x40 - (v & 0x3f);
			sndp->square[ch].duty = square_duty_table[v >> 6];
			/* sndp->square[ch].lc.counter = sndp->square[ch].lc.initial_counter; */
			break;
		case 0xff12:
		case 0xff17:
			ch = a >= 0xff16;
			sndp->square[ch].ed.initial_volume = v >> 4;
			sndp->square[ch].ed.direction = v & 8;
			sndp->square[ch].ed.rate = v & 7;
			sndp->square[ch].ed.volume = sndp->square[ch].ed.initial_volume;
			break;
		case 0xff13:
		case 0xff18:
			ch = a >= 0xff16;
			sndp->square[ch].sw.wl &= 0x700;
			sndp->square[ch].sw.wl |= v;
			break;
		case 0xff14:
		case 0xff19:
			ch = a >= 0xff16;
			sndp->square[ch].sw.wl &= 0xff;
			sndp->square[ch].sw.wl |= (v << 8) & 0x700;
			sndp->square[ch].lc.enable = v & 0x40;
			if (v & 0x80)
			{
				sndp->square[ch].key = 1 << ch;
				sndp->square[ch].lc.counter = sndp->square[ch].lc.initial_counter;
				sndp->square[ch].ed.volume = sndp->square[ch].ed.initial_volume;
				sndp->square[ch].ed.timer = 0;
				sndp->square[ch].sw.timer = 0;
				sndp->square[ch].fc = 0;
			}
			break;
		case 0xff1a:
			sndp->wavememory.on = v & 0x80;
			break;
		case 0xff1b:
			sndp->wavememory.lc.initial_counter = 0x100 - (v & 0xff);
			break;
		case 0xff1c:
			sndp->wavememory.volume = wavememory_volume_table[(v >> 5) & 3];
			break;
		case 0xff1d:
			sndp->wavememory.wl &= 0x700;
			sndp->wavememory.wl |= v;
			break;
		case 0xff1e:
			sndp->wavememory.wl &= 0xff;
			sndp->wavememory.wl |= (v << 8) & 0x700;
			sndp->wavememory.lc.enable = v & 0x40;
			if (v & 0x80)
			{
				sndp->wavememory.key = 1 << 2;
				sndp->wavememory.lc.counter = sndp->wavememory.lc.initial_counter;
				sndp->wavememory.fc = 0;
			}
			break;

		case 0xff20:
			sndp->noise.lc.initial_counter = 0x40 - (v & 0x3f);
			break;
		case 0xff21:
			sndp->noise.ed.initial_volume = v >> 4;
			sndp->noise.ed.direction = v & 8;
			sndp->noise.ed.rate = v & 7;
			sndp->noise.ed.volume = sndp->noise.ed.initial_volume;
			break;
		case 0xff22:
			sndp->noise.shift = v >> 4;
			if (sndp->noise.shift > 14) sndp->noise.shift = 14;
			sndp->noise.rng = 1;
#if 1
			/* GBSOUND.TXT */
			sndp->noise.step1 = (v & 8) ? (6) : (14);
			sndp->noise.step2 = (v & 8) ? (5) : (13);
#else
			sndp->noise.step1 = (v & 8) ? (7 + 1) : (15 + 1);
			sndp->noise.step2 = (v & 8) ? (2 + 1) : (10 + 1);
#endif
			sndp->noise.divratio = noise_divratio_table[v & 7] << sndp->noise.shift;
			break;
		case 0xff23:
			sndp->noise.lc.enable = v & 0x40;
			if (v & 0x80)
			{
				sndp->noise.key = 1 << 3;
				sndp->noise.lc.counter = sndp->noise.lc.initial_counter;
				sndp->noise.ed.volume = sndp->noise.ed.initial_volume;
				sndp->noise.ed.timer = 0;
				sndp->noise.fc = 0;
			}
			break;
	}
}

static Uint32 sndread(DMGSOUND *sndp, Uint32 a)
{
	switch (a)
	{
#if 1
		case 0xff11: return sndp->common.regs[a - 0xff10] & 0xc0;
		case 0xff14: return sndp->common.regs[a - 0xff10] & 0x40;
		case 0xff16: return sndp->common.regs[a - 0xff10] & 0xc0;
		case 0xff19: return sndp->common.regs[a - 0xff10] & 0x40;
		case 0xff1a: return sndp->common.regs[a - 0xff10] & 0x80;
		case 0xff1c: return sndp->common.regs[a - 0xff10] & 0x60;
		case 0xff1d: return 0;
		case 0xff1e: return sndp->common.regs[a - 0xff10] & 0x40;
		case 0xff23: return sndp->common.regs[a - 0xff10] & 0x40;
#endif
		case 0xff26: return (sndp->common.regs[a - 0xff10] & 0x80) + sndp->square[0].key + sndp->square[1].key + sndp->noise.key + (sndp->wavememory.on ? sndp->wavememory.key : 0);
	}
	return sndp->common.regs[a - 0xff10];
}


static void DMGSoundSquareReset(DMG_SQUARE *ch, Uint32 clock, Uint32 freq)
{
	XMEMSET(ch, 0, sizeof(DMG_SQUARE));
	ch->cps = DivFix(clock, 4 * freq, CPS_BITS);
	ch->cpf = DivFix(clock, 4 * 256, CPS_BITS);
	ch->duty = 4;
}

static void DMGSoundWaveMemoryReset(DMG_WAVEMEMORY *ch, Uint32 clock, Uint32 freq)
{
	XMEMSET(ch, 0, sizeof(DMG_WAVEMEMORY));
	ch->cps = DivFix(clock, 2 * freq, CPS_BITS);
	ch->cpf = DivFix(clock, 2 * 2, CPFWM_BITS);
	ch->on = 0x80;
}

static void DMGSoundNoiseReset(DMG_NOISE *ch, Uint32 clock, Uint32 freq)
{
	XMEMSET(ch, 0, sizeof(DMG_NOISE));
	ch->cps = DivFix(clock, 4 * freq, CPS_BITS);
	ch->cpf = DivFix(clock, 4 * 256, CPS_BITS);
	ch->rng = 1;
	ch->divratio = 1;
}


static void sndreset(DMGSOUND *sndp, Uint32 clock, Uint32 freq)
{
	const static struct {
		Uint32 a;
		Uint8 v;
	} *p, dmginitdata[] = {
		{ 0xff10, 0x80 }, /* NR10 */
		{ 0xff11, 0xbf }, /* NR11 */
		{ 0xff12, 0xf0 }, /* NR12 */

		{ 0xff16, 0x3f }, /* NR21 */
		{ 0xff17, 0x00 }, /* NR22 */

		{ 0xff1a, 0x7f }, /* NR30 */
		{ 0xff1b, 0xff }, /* NR31 */
		{ 0xff1c, 0x9f }, /* NR32 */

		{ 0xff20, 0xff }, /* NR41 */
		{ 0xff21, 0x00 }, /* NR42 */

		{ 0xff24, 0x77 }, /* NR50 */
		{ 0xff25, 0xf3 }, /* NR51 */
		{ 0xff26, 0xf1 }, /* NR52 */
#if 0
		{ 0xff13, 0xff }, /* NR13 */
		{ 0xff14, 0xbf }, /* NR14 */
		{ 0xff18, 0xff }, /* NR23 */
		{ 0xff19, 0xbf }, /* NR24 */
		{ 0xff1d, 0xff }, /* NR33 */
		{ 0xff1e, 0xbf }, /* NR34 */
		{ 0xff22, 0x00 }, /* NR43 */
		{ 0xff23, 0xbf }, /* NR44 */
#endif
		{ 0, 0 }, /* ENDMARK */
	};
	Uint32 a;
	XMEMSET(&sndp->common, 0, sizeof(sndp->common));
	DMGSoundSquareReset(&sndp->square[0], clock, freq);
	DMGSoundSquareReset(&sndp->square[1], clock, freq);
	DMGSoundWaveMemoryReset(&sndp->wavememory, clock, freq);
	DMGSoundNoiseReset(&sndp->noise, clock, freq);
	for (p = dmginitdata; p->a; p++) sndwrite(sndp, p->a, p->v);
	for (a = 0xff30; a <= 0xff3f; a++) sndwrite(sndp, a, (a & 8) ? 0xff : 0);
}

static void sndrelease(DMGSOUND *sndp)
{
	if (sndp->logtbl) sndp->logtbl->release(sndp->logtbl->ctx);
	XFREE(sndp);
}

static void setinst(void *ctx, Uint32 n, void *p, Uint32 l){}

KMIF_SOUND_DEVICE *DMGSoundAlloc(void)
{
	DMGSOUND *sndp;
	sndp = XMALLOC(sizeof(DMGSOUND));
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
