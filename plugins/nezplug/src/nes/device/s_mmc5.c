#include "../nestypes.h"
#include "../audiosys.h"
#include "../handler.h"
#include "../nsf6502.h"
#include "../nsdout.h"
#include "logtable.h"
#include "s_mmc5.h"

#define NES_BASECYCLES (21477270)

/* 31 - log2(NES_BASECYCLES/(12*MIN_FREQ)) > CPS_BITS  */
/* MIN_FREQ:11025 23.6 > CPS_BITS */
/* 32-12(max spd) > CPS_BITS */
#define CPS_BITS 19

/* ----------------- */
/*  MMC5 EXTEND RAM  */
/* ----------------- */

static Uint8 mmc5exram[0x400];

static Uint __fastcall mmc5exram_read(Uint address)
{
	return mmc5exram[address & 0x03FF];
}

static void __fastcall mmc5exram_write(Uint address, Uint value)
{
	mmc5exram[address & 0x03FF] = value;
}

static NES_READ_HANDLER mmc5exram_read_handler[] =
{
	{ 0x5C00, 0x5FEF, mmc5exram_read, },
	{ 0,      0,      0, },
};
static NES_WRITE_HANDLER mmc5exram_write_handler[] =
{
	{ 0x5C00, 0x5FEF, mmc5exram_write, },
	{ 0,      0,      0, },
};

void MMC5ExtendRamInstall(void)
{
	NESReadHandlerInstall(mmc5exram_read_handler);
	NESWriteHandlerInstall(mmc5exram_write_handler);
}



/* ----------------- */
/*  MMC5 MULTIPLIER  */
/* ----------------- */

static Uint8 mmc5multiplier[2];

static Uint __fastcall mmc5mul_read(Uint address)
{
	Uint mul;
	address = address - 0x5205;
	/* if (address > 1) return; */
	mul = mmc5multiplier[0] * mmc5multiplier[1];
	return address ? (Uint8)(mul >> 8) : (Uint8)mul;
}

static void __fastcall mmc5mul_write(Uint address, Uint value)
{
	address = address - 0x5205;
	/* if (address > 1) return; */
	mmc5multiplier[address] = value;
}

static NES_READ_HANDLER mmc5mul_read_handler[] =
{
	{ 0x5205, 0x5206, mmc5mul_read, },
	{ 0,      0,      0, },
};
static NES_WRITE_HANDLER mmc5mul_write_handler[] =
{
	{ 0x5205, 0x5206, mmc5mul_write, },
	{ 0,      0,      0, },
};

void MMC5MutiplierInstall(void)
{
	NESReadHandlerInstall(mmc5mul_read_handler);
	NESWriteHandlerInstall(mmc5mul_write_handler);
}



/* ------------ */
/*  MMC5 SOUND  */
/* ------------ */

/* 31 - 7 - log2(MAX_FREQ/60) > SPF_BITS */
/* MAX_FREQ:96000 13.3 > SPF_BITS */
#define SPF_BITS 12
#define ENV_BITS	(31 - CPS_BITS - 5/* bitsof env(1-16) */)
#define KEY_ON		1
#define KEY_RELEASE 2

typedef struct {
	Uint32 cps;
	Int32 cycles;
	Int32 envphase;

	Uint32 spd;
	Uint32 envspd;

	Uint32 length;
	Uint32 freq;
	Uint32 mastervolume;

	Uint8 regs[4];
	Uint8 update;
	Uint8 key;
	Uint8 adr;
	Uint8 envadr;
	Uint8 duty;
	Uint8 mute;
} MMC5_SQUARE;
typedef struct {
	Int32 output;
	Int32 linearvolume;
	Uint8 key;
	Uint8 mute;
} MMC5_DA;

typedef struct {
	MMC5_SQUARE square[2];
	MMC5_DA da;
} MMC5SOUND;

static MMC5SOUND mmc5;

#define V(x) (x*64/60)
static Uint vbl_length[32] =
{
	V(0x05), V(0x7F), V(0x0A), V(0x01), V(0x14), V(0x02), V(0x28), V(0x03),
	V(0x50), V(0x04), V(0x1E), V(0x05), V(0x07), V(0x06), V(0x0E), V(0x07),
	V(0x06), V(0x08), V(0x0C), V(0x09), V(0x18), V(0x0A), V(0x30), V(0x0B),
	V(0x60), V(0x0C), V(0x24), V(0x0D), V(0x08), V(0x0E), V(0x10), V(0x0F),
};
#undef V

#define V(x) ((x) << CPS_BITS)
static const Uint32 spd_limit[8] =
{
	V(0x3FF), V(0x555), V(0x666), V(0x71C), 
	V(0x787), V(0x7C1), V(0x7E0), V(0x7F0),
};
#undef V

static Int32 MMC5SoundSquareRender(MMC5_SQUARE *ch)
{
	Uint32 output;
	if (ch->update)
	{
		if (ch->update & 1)
		{
			ch->duty = (ch->regs[0] >> 4) & 0x0C;
			if (ch->duty == 0) ch->duty = 2;
			ch->envspd = ((ch->regs[0] & 0x0F) + 1) << (CPS_BITS + ENV_BITS);
		}
		if (ch->update & (4 | 8))
		{
			ch->spd = (((ch->regs[3] & 7) << 8) + ch->regs[2] + 0) << CPS_BITS;
		}
		if ((ch->update & 8) && ch->key)
		{
			ch->length = (vbl_length[ch->regs[3] >> 3] * ch->freq) >> 6;
			ch->envadr = 0;
		}
		ch->update = 0;
	}

	if (ch->key == 0) return 0;

	ch->envphase -= ch->cps >> (13 - ENV_BITS);
	if (ch->regs[0] & 0x20)
	{
		while (ch->envphase < 0)
		{
			ch->envphase += ch->envspd;
			ch->envadr++;
		}
		ch->envadr &= 0x0F;
	}
	else
	{
		while (ch->envphase < 0)
		{
			ch->envphase += ch->envspd;
			ch->envadr += (ch->envadr < 15);
		}
	}

	if (ch->length)
	{
		if (!(ch->regs[0] & 0x20)) ch->length--;
	}
	else
	{
		ch->key = 0;
	}

	if (ch->spd < (4 << CPS_BITS)) return 0;
	if (!(ch->regs[1] & 8))
	{
		if (ch->spd > spd_limit[ch->regs[1] & 7]) return 0;
	}

	ch->cycles -= ch->cps;
	while (ch->cycles < 0)
	{
		ch->cycles += ch->spd;
		ch->adr++;
	}
	ch->adr &= 0x0F;

	if (ch->mute) return 0;

	if (ch->regs[0] & 0x10) /* fixed volume */
		output = ch->regs[0] & 0x0F;
	else
		output = 15 - ch->envadr;

	output = LinearToLog(output) + ch->mastervolume;
	output += (ch->adr < ch->duty);
	return LogToLinear(output, LOG_LIN_BITS - LIN_BITS - 16);
}

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

static void MMC5SoundSquareReset(MMC5_SQUARE *ch)
{
	XMEMSET(ch, 0, sizeof(MMC5_SQUARE));
	ch->freq = NESAudioFrequencyGet();
	ch->cps = DivFix(NES_BASECYCLES, 12 * ch->freq, CPS_BITS);
}



static Int32 __fastcall MMC5SoundRender(void)
{
	Int32 accum = 0;
	accum += MMC5SoundSquareRender(&mmc5.square[0]);
	accum += MMC5SoundSquareRender(&mmc5.square[1]);
	if (mmc5.da.key && !mmc5.da.mute) accum += mmc5.da.output * mmc5.da.linearvolume;
	return accum;
}

static NES_AUDIO_HANDLER s_mmc5_audio_handler[] = {
	{ 1, MMC5SoundRender, }, 
	{ 0, 0, }, 
};

static void __fastcall MMC5SoundVolume(Uint volume)
{
	volume = (volume << (LOG_BITS - 8)) << 1;
	mmc5.square[0].mastervolume = volume;
	mmc5.square[1].mastervolume = volume;
	mmc5.da.linearvolume = LogToLinear(volume, LOG_LIN_BITS - 16);
}

static NES_VOLUME_HANDLER s_mmc5_volume_handler[] = {
	{ MMC5SoundVolume, },
	{ 0, }, 
};

static void __fastcall MMC5SoundWrite(Uint address, Uint value)
{
	if (0x5000 <= address && address <= 0x5015)
	{
		if (NSD_out_mode) NSDWrite(NSD_MMC5, address, value);
		switch (address)
		{
			case 0x5000: case 0x5002: case 0x5003:
			case 0x5004: case 0x5006: case 0x5007:
				{
					int ch = address >= 0x5004;
					int port = address & 3;
					mmc5.square[ch].regs[port] = value;
					mmc5.square[ch].update |= 1 << port; 
				}
				break;
			case 0x5011:
				mmc5.da.output = ((Int)(value & 0xff)) - 0x80;
				break;
			case 0x5010:
				mmc5.da.key = (value & 0x01);
				break;
			case 0x5015:
				if (value & 1)
					mmc5.square[0].key = 1;
				else
				{
					mmc5.square[0].key = 0;
					mmc5.square[0].length = 0;
				}
				if (value & 2)
					mmc5.square[1].key = 1;
				else
				{
					mmc5.square[1].key = 0;
					mmc5.square[1].length = 0;
				}
				break;
		}
	}
}

static NES_WRITE_HANDLER s_mmc5_write_handler[] =
{
	{ 0x5000, 0x5015, MMC5SoundWrite, },
	{ 0,      0,      0, },
};

static void __fastcall MMC5SoundDaReset(MMC5_DA *ch)
{
	XMEMSET(ch, 0, sizeof(MMC5_DA));
}

static void __fastcall MMC5SoundReset(void)
{
	MMC5SoundSquareReset(&mmc5.square[0]);
	MMC5SoundSquareReset(&mmc5.square[1]);
	MMC5SoundDaReset(&mmc5.da);

	MMC5SoundWrite(0x5000, 0x00);
	MMC5SoundWrite(0x5002, 0x00);
	MMC5SoundWrite(0x5003, 0x00);
	MMC5SoundWrite(0x5004, 0x00);
	MMC5SoundWrite(0x5006, 0x00);
	MMC5SoundWrite(0x5007, 0x00);
	MMC5SoundWrite(0x5010, 0x00);
	MMC5SoundWrite(0x5011, 0x00);
}

static NES_RESET_HANDLER s_mmc5_reset_handler[] = {
	{ NES_RESET_SYS_NOMAL, MMC5SoundReset, }, 
	{ 0,                   0, }, 
};

void MMC5SoundInstall(void)
{
	LogTableInitialize();
	NESAudioHandlerInstall(s_mmc5_audio_handler);
	NESVolumeHandlerInstall(s_mmc5_volume_handler);
	NESWriteHandlerInstall(s_mmc5_write_handler);
	NESResetHandlerInstall(s_mmc5_reset_handler);
}
