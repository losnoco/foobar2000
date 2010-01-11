#include "../nestypes.h"
#include "../audiosys.h"
#include "../handler.h"
#include "../nsf6502.h"
#include "../nsdout.h"
#include "s_n106.h"
#include "logtable.h"

#define NES_BASECYCLES (21477270)
#define CPS_SHIFT (20)
#define PHASE_SHIFT (16+2)
#define  N106SOUND_DATA 0x4800
#define  N106SOUND_ADDR 0xF800

typedef struct {
	Uint32 logvol;
	Uint32 cycles;
	Uint32 spd;
	Uint32 phase;
	Uint32 tlen;

	Uint8 update;
	Uint8 freql;
	Uint8 freqm;
	Uint8 freqh;
	Uint8 vreg;
	Uint8 tadr;
	Uint8 nazo;
	Uint8 mute;
} N106_WM;

typedef struct {
	Uint32 cps;
	Uint32 mastervolume;

	N106_WM ch[8];

	Uint8 addressauto;
	Uint8 address;
	Uint8 chinuse;

	Uint32 tone[0x100];	/* TONE DATA */
	Uint8 data[0x80];
} N106SOUND;

static N106SOUND n106s;

__inline static void UPDATE(N106_WM *chp)
{
	if (chp->update & 3)
	{
		Uint32 freq;
		freq  = ((int)chp->freql);
		freq += ((int)chp->freqm) << 8;
		freq += ((int)chp->freqh) << 16;
		chp->spd = freq & 0x3ffff;
	}
	if (chp->update & 2)
	{
		Uint32 tlen;
		tlen = (0x20 - (chp->freqh & 0x1c)) << PHASE_SHIFT;
		if (chp->tlen != tlen)
		{
			chp->tlen = tlen;
			chp->phase = 0;
		}
	}
	if (chp->update & 4)
	{
		chp->logvol = LinearToLog((chp->vreg & 0x0f) << 2);
	}
	chp->update = 0;
}

static Int32 __fastcall N106SoundRender(void)
{
	N106_WM *chp;
	Int32 accum = 0;
	for (chp = &n106s.ch[8 - n106s.chinuse]; chp < &n106s.ch[8]; chp++)
	{
		Uint32 cyclesspd = n106s.chinuse << CPS_SHIFT;
		if (chp->update) UPDATE(chp);
		chp->cycles += n106s.cps;
		while (chp->cycles >= cyclesspd)
		{
			chp->cycles -= cyclesspd;
			chp->phase += chp->spd;
		}
		while (chp->phase >= chp->tlen) chp->phase -= chp->tlen;
		if (chp->mute) continue;
		accum += LogToLinear(n106s.tone[((chp->phase >> PHASE_SHIFT) + chp->tadr) & 0xff] + chp->logvol + n106s.mastervolume, LOG_LIN_BITS - LIN_BITS - LIN_BITS - 10);
	}
	return accum;
}

static NES_AUDIO_HANDLER s_n106_audio_handler[] = {
	{ 1, N106SoundRender, }, 
	{ 0, 0, }, 
};

static void __fastcall N106SoundVolume(Uint volume)
{
	n106s.mastervolume = (volume << (LOG_BITS - 8)) << 1;
}

static NES_VOLUME_HANDLER s_n106_volume_handler[] = {
	{ N106SoundVolume, }, 
	{ 0, }, 
};

static void __fastcall N106SoundWriteAddr(Uint address, Uint value)
{
	n106s.address     = value & 0x7f;
	n106s.addressauto = (value & 0x80) ? 1 : 0;
}

static void __fastcall N106SoundWriteData(Uint address, Uint value)
{
	if (NSD_out_mode) NSDWrite(NSD_N106, n106s.address, value);
	n106s.data[n106s.address] = value;
	n106s.tone[n106s.address * 2]     = LinearToLog(((Int32)(value & 0xf) << 2) - 0x20);
	n106s.tone[n106s.address * 2 + 1] = LinearToLog(((Int32)(value >>  4) << 2) - 0x20);
	if (n106s.address >= 0x40)
	{
		N106_WM *chp = &n106s.ch[(n106s.address - 0x40) >> 3];
		switch (n106s.address & 7)
		{
			case 0:
				chp->update |= 1;
				chp->freql = value;
				break;
			case 2:
				chp->update |= 1;
				chp->freqm = value;
				break;
			case 4:
				chp->update |= 2;
				chp->freqh = value;
				break;
			case 6:
				chp->tadr = value & 0xff;
				break;
			case 7:
				chp->update |= 4;
				chp->vreg = value;
				chp->nazo = (value >> 4) & 0x07;
				if (chp == &n106s.ch[7])
					n106s.chinuse = 1 + chp->nazo;
				break;
		}
	}
	if (n106s.addressauto)
	{
		n106s.address = (n106s.address + 1) & 0x7f;
	}
}

static NES_WRITE_HANDLER s_n106_write_handler[] =
{
	{ N106SOUND_DATA, N106SOUND_DATA, N106SoundWriteData, },
	{ N106SOUND_ADDR, N106SOUND_ADDR, N106SoundWriteAddr, },
	{ 0,              0,              0, },
};

static Uint __fastcall N106SoundReadData(Uint address)
{
	Uint ret = n106s.data[n106s.address];
	if (n106s.addressauto)
	{
		n106s.address = (n106s.address + 1) & 0x7f;
	}
	return ret;
}

static NES_READ_HANDLER s_n106_read_handler[] =
{
	{ N106SOUND_DATA, N106SOUND_DATA, N106SoundReadData, },
	{ 0,              0,              0, },
};

static Uint32 DivFix(Uint32 p1, Uint32 p2, Uint32 fix)
{
	Uint32 ret;
	ret = p1 / p2;
	p1  = p1 % p2;/* p1 = p1 - p2 * ret; */
	while (fix--)
	{
		p1 += p1;
		ret += ret;
		if (p1 >= p2)
		{
			p1 -= p2;
			ret++;
		}
	}
	return ret;
}

static void __fastcall N106SoundReset(void)
{
	int i;
	XMEMSET(&n106s, 0, sizeof(N106SOUND));
	for (i = 0; i < 8; i++)
	{
		n106s.ch[i].tlen = 0x10 << PHASE_SHIFT;
		n106s.ch[i].logvol = LinearToLog(0);
	}
	n106s.addressauto = 1;
	n106s.chinuse = 8;
	n106s.cps = DivFix(NES_BASECYCLES, 45 * NESAudioFrequencyGet(), CPS_SHIFT);
}

static NES_RESET_HANDLER s_n106_reset_handler[] = {
	{ NES_RESET_SYS_NOMAL, N106SoundReset, }, 
	{ 0,                   0, }, 
};

void N106SoundInstall(void)
{
	LogTableInitialize();
	NESAudioHandlerInstall(s_n106_audio_handler);
	NESVolumeHandlerInstall(s_n106_volume_handler);
	NESReadHandlerInstall(s_n106_read_handler);
	NESWriteHandlerInstall(s_n106_write_handler);
	NESResetHandlerInstall(s_n106_reset_handler);
}
