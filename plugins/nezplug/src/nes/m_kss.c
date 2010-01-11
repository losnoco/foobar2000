#include "neserr.h"
#include "handler.h"
#include "audiosys.h"
#include "songinfo.h"
#include "nsdout.h"

#include "m_kss.h"

#include "device/s_psg.h"
#include "device/s_scc.h"
#include "device/s_sng.h"
#include "device/s_opl.h"
#include "device/divfix.h"

#include "kmz80/kmz80.h"


#define SHIFT_CPS 15
#define BASECYCLES       (3579545)

#define EXTDEVICE_SNG		(1 << 1)

#define EXTDEVICE_FMUNIT		(1 << 0)
#define EXTDEVICE_GGSTEREO	(1 << 2)
#define EXTDEVICE_GGRAM		(1 << 3)

#define EXTDEVICE_MSXMUSIC	(1 << 0)
#define EXTDEVICE_MSXRAM	(1 << 2)
#define EXTDEVICE_MSXAUDIO	(1 << 3)
#define EXTDEVICE_MSXSTEREO	(1 << 4)

#define EXTDEVICE_PAL		(1 << 6)
#define EXTDEVICE_EXRAM		(1 << 7)

#define SND_PSG 0
#define SND_SCC 1
#define SND_MSXMUSIC 2
#define SND_MSXAUDIO 3
#define SND_MAX 4

#define SND_SNG 0
#define SND_FMUNIT 2


typedef struct KSSSEQ_TAG KSSSEQ;

struct  KSSSEQ_TAG {
	KMZ80_CONTEXT ctx;
	KMIF_SOUND_DEVICE *sndp[SND_MAX];
	Uint8 vola[SND_MAX];
	KMEVENT kme;
	KMEVENT_ITEM_ID vsync_id;

	Uint8 *readmap[8];
	Uint8 *writemap[8];

	Uint32 cps;		/* cycles per sample:fixed point */
	Uint32 cpsrem;	/* cycle remain */
	Uint32 cpsgap;	/* cycle gap */
	Uint32 cpf;		/* cycles per frame:fixed point */
	Uint32 cpfrem;	/* cycle remain */
	Uint32 total_cycles;	/* total played cycles */

	Uint32 startsong;
	Uint32 maxsong;

	Uint32 initaddr;
	Uint32 playaddr;

	Uint8 *data;
	Uint32 dataaddr;
	Uint32 datasize;
	Uint8 *bankbase;
	Uint32 bankofs;
	Uint32 banknum;
	Uint32 banksize;
	enum
	{
		KSS_BANK_OFF,
		KSS_16K_MAPPER,
		KSS_8K_MAPPER
	} bankmode;
	enum
	{
		SYNTHMODE_SMS,
		SYNTHMODE_MSX,
		SYNTHMODE_MSXSTEREO
	} synthmode;
	Uint8 sccenable;
	Uint8 rammode;
	Uint8 extdevice;
	Uint8 majutushimode;

	Uint8 ram[0x10000];
};

static Uint8 usertone_enable[2] = { 0,0 };
static Uint8 usertone[2][16 * 19];

static Uint32 GetWordLE(Uint8 *p)
{
	return p[0] | (p[1] << 8);
}

static Uint32 GetWordBE(Uint8 *p)
{
	return p[1] | (p[0] << 8);
}

static void SetWordLE(Uint8 *p, Uint32 v)
{
	p[0] = (v >> (8 * 0)) & 0xFF;
	p[1] = (v >> (8 * 1)) & 0xFF;
}

static Uint32 GetDwordLE(Uint8 *p)
{
	return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

#define GetDwordLEM(p) (Uint32)((((Uint8 *)p)[0] | (((Uint8 *)p)[1] << 8) | (((Uint8 *)p)[2] << 16) | (((Uint8 *)p)[3] << 24)))

void OPLLSetTone(Uint8 *p, Uint32 type)
{
	if ((GetDwordLE(p) & 0xf0ffffff) == GetDwordLEM("ILL0"))
		XMEMCPY(usertone[type], p, 16 * 19);
	else
		XMEMCPY(usertone[type], p, 8 * 15);
	usertone_enable[type] = 1;
}

static Int32 execute(KSSSEQ *THIS_)
{
	Uint32 cycles;
	THIS_->cpsrem += THIS_->cps;
	cycles = THIS_->cpsrem >> SHIFT_CPS;
	if (THIS_->cpsgap >= cycles)
		THIS_->cpsgap -= cycles;
	else
	{
		Uint32 excycles = cycles - THIS_->cpsgap;
		THIS_->cpsgap = kmz80_exec(&THIS_->ctx, excycles) - excycles;
	}
	THIS_->cpsrem &= (1 << SHIFT_CPS) - 1;
	THIS_->total_cycles += cycles;
	return 0;
}

__inline static void synth(KSSSEQ *THIS_, Int32 *d)
{
	switch (THIS_->synthmode)
	{
		default:
			NEVER_REACH
		case SYNTHMODE_SMS:
			THIS_->sndp[SND_SNG]->synth(THIS_->sndp[SND_SNG]->ctx, d);
			if (THIS_->sndp[SND_FMUNIT]) THIS_->sndp[SND_FMUNIT]->synth(THIS_->sndp[SND_FMUNIT]->ctx, d);
			break;
		case SYNTHMODE_MSX:
			THIS_->sndp[SND_SCC]->synth(THIS_->sndp[SND_SCC]->ctx, d);
			THIS_->sndp[SND_PSG]->synth(THIS_->sndp[SND_PSG]->ctx, d);
			if (THIS_->sndp[SND_MSXMUSIC]) THIS_->sndp[SND_MSXMUSIC]->synth(THIS_->sndp[SND_MSXMUSIC]->ctx, d);
			if (THIS_->sndp[SND_MSXAUDIO]) THIS_->sndp[SND_MSXAUDIO]->synth(THIS_->sndp[SND_MSXAUDIO]->ctx, d);
			break;
		case SYNTHMODE_MSXSTEREO:
			{
				Int32 b[3];
				b[0] = b[1] = b[2] = 0;
				THIS_->sndp[SND_PSG]->synth(THIS_->sndp[SND_PSG]->ctx, &b[0]);
				THIS_->sndp[SND_SCC]->synth(THIS_->sndp[SND_SCC]->ctx, &b[0]);
				if (THIS_->sndp[SND_MSXMUSIC]) THIS_->sndp[SND_MSXMUSIC]->synth(THIS_->sndp[SND_MSXMUSIC]->ctx, &b[0]);
				if (THIS_->sndp[SND_MSXAUDIO]) THIS_->sndp[SND_MSXAUDIO]->synth(THIS_->sndp[SND_MSXAUDIO]->ctx, &b[1]);
				d[0] += b[1];
				d[1] += b[2] + b[2];
			}
			break;
	}
}

__inline static void volume(KSSSEQ *THIS_, Uint32 v)
{
	v += 256;
	switch (THIS_->synthmode)
	{
		default:
			NEVER_REACH
		case SYNTHMODE_SMS:
			THIS_->sndp[SND_SNG]->volume(THIS_->sndp[SND_SNG]->ctx, v + (THIS_->vola[SND_SNG] << 4) - 0x800);
			if (THIS_->sndp[SND_FMUNIT]) THIS_->sndp[SND_FMUNIT]->volume(THIS_->sndp[SND_FMUNIT]->ctx, v + (THIS_->vola[SND_FMUNIT] << 4) - 0x800);
			break;
		case SYNTHMODE_MSX:
		case SYNTHMODE_MSXSTEREO:
			THIS_->sndp[SND_PSG]->volume(THIS_->sndp[SND_PSG]->ctx, v + (THIS_->vola[SND_PSG] << 4) - 0x800);
			THIS_->sndp[SND_SCC]->volume(THIS_->sndp[SND_SCC]->ctx, v + (THIS_->vola[SND_SCC] << 4) - 0x800);
			if (THIS_->sndp[SND_MSXMUSIC]) THIS_->sndp[SND_MSXMUSIC]->volume(THIS_->sndp[SND_MSXMUSIC]->ctx, v + (THIS_->vola[SND_MSXMUSIC] << 4) - 0x800);
			if (THIS_->sndp[SND_MSXAUDIO]) THIS_->sndp[SND_MSXAUDIO]->volume(THIS_->sndp[SND_MSXAUDIO]->ctx, v + (THIS_->vola[SND_MSXAUDIO] << 4) - 0x800);
			break;
	}
}

static void vsync_setup(KSSSEQ *THIS_)
{
	Int32 cycles;
	THIS_->cpfrem += THIS_->cpf;
	cycles = THIS_->cpfrem >> SHIFT_CPS;
	THIS_->cpfrem &= (1 << SHIFT_CPS) - 1;
	kmevent_settimer(&THIS_->kme, THIS_->vsync_id, cycles);
}

static void play_setup(KSSSEQ *THIS_, Uint32 pc)
{
	Uint32 sp = 0xF380, rp;
	THIS_->ram[--sp] = 0;
	THIS_->ram[--sp] = 0xfe;
	THIS_->ram[--sp] = 0x18;	/* JR +0 */
	THIS_->ram[--sp] = 0x76;	/* HALT */
	rp = sp;
	THIS_->ram[--sp] = rp >> 8;
	THIS_->ram[--sp] = rp;
	THIS_->ctx.sp = sp;
	THIS_->ctx.pc = pc;
	THIS_->ctx.regs8[REGID_HALTED] = 0;
}

static Uint32 busread_event(KSSSEQ *THIS_, Uint32 a)
{
	return 0x38;
}

static Uint32 read_event(KSSSEQ *THIS_, Uint32 a)
{
	return THIS_->readmap[a >> 13][a];
}



static void KSSMaper8KWrite(KSSSEQ *THIS_, Uint32 a, Uint32 v)
{
	Uint32 page = a >> 13;
	v -= THIS_->bankofs;
	if (v < THIS_->banknum)
	{
		THIS_->readmap[page] = THIS_->bankbase + THIS_->banksize * v - a;
	}
	else
	{
		THIS_->readmap[page] = THIS_->ram;
	}
}

static void KSSMaper16KWrite(KSSSEQ *THIS_, Uint32 a, Uint32 v)
{
	Uint32 page = a >> 13;
	v -= THIS_->bankofs;
	if (v < THIS_->banknum)
	{
		THIS_->readmap[page] = THIS_->readmap[page + 1] = THIS_->bankbase + THIS_->banksize * v - a;
		if (!(THIS_->extdevice & EXTDEVICE_SNG)) THIS_->sccenable = 1;
	}
	else
	{
		THIS_->readmap[page] = THIS_->readmap[page + 1] = THIS_->ram;
		if (!(THIS_->extdevice & EXTDEVICE_SNG)) THIS_->sccenable = !THIS_->rammode;
	}
}

static void write_event(KSSSEQ *THIS_, Uint32 a, Uint32 v)
{
	Uint32 page = a >> 13;
	if (THIS_->writemap[page])
	{
		THIS_->writemap[page][a] = v;
	}
	else if (THIS_->bankmode == KSS_8K_MAPPER)
	{
		if (a == 0x9000)
			KSSMaper8KWrite(THIS_, 0x8000, v);
		else if (a == 0xb000)
			KSSMaper8KWrite(THIS_, 0xa000, v);
		else if (THIS_->sccenable)
			THIS_->sndp[SND_SCC]->write(THIS_->sndp[SND_SCC]->ctx, a, v);
	}
	else
	{
		if (THIS_->sccenable)
			THIS_->sndp[SND_SCC]->write(THIS_->sndp[SND_SCC]->ctx, a, v);
		else if (THIS_->rammode)
			THIS_->ram[a] = v;
	}
}

static Uint32 ioread_eventSMS(KSSSEQ *THIS_, Uint32 a)
{
	return 0xff;
}
static Uint32 ioread_eventMSX(KSSSEQ *THIS_, Uint32 a)
{
	a &= 0xff;
	if (a == 0xa2)
		return THIS_->sndp[SND_PSG]->read(THIS_->sndp[SND_PSG]->ctx, 0);
	if (THIS_->sndp[SND_MSXAUDIO] && (a & 0xfe) == 0xc0)
		return THIS_->sndp[SND_MSXAUDIO]->read(THIS_->sndp[SND_MSXAUDIO]->ctx, a & 1);
	return 0xff;
}

static void iowrite_eventSMS(KSSSEQ *THIS_, Uint32 a, Uint32 v)
{
	a &= 0xff;
	if ((a & 0xfe) == 0x7e)
		THIS_->sndp[SND_SNG]->write(THIS_->sndp[SND_SNG]->ctx, 0, v);
	else if (a == 0x06)
		THIS_->sndp[SND_SNG]->write(THIS_->sndp[SND_SNG]->ctx, 1, v);
	else if (THIS_->sndp[SND_FMUNIT] && (a & 0xfe) == 0xf0)
		THIS_->sndp[SND_FMUNIT]->write(THIS_->sndp[SND_FMUNIT]->ctx, a & 1, v);
	else if (a == 0xfe && THIS_->bankmode == KSS_16K_MAPPER)
		KSSMaper16KWrite(THIS_, 0x8000, v);
}
static void iowrite_eventMSX(KSSSEQ *THIS_, Uint32 a, Uint32 v)
{
	a &= 0xff;
	if ((a & 0xfe) == 0xa0)
		THIS_->sndp[SND_PSG]->write(THIS_->sndp[SND_PSG]->ctx, a & 1, v);
	else if (a == 0xab)
		THIS_->sndp[SND_PSG]->write(THIS_->sndp[SND_PSG]->ctx, 2, v & 1);
	else if (THIS_->sndp[SND_MSXMUSIC] && (a & 0xfe) == 0x7c)
		THIS_->sndp[SND_MSXMUSIC]->write(THIS_->sndp[SND_MSXMUSIC]->ctx, a & 1, v);
	else if (THIS_->sndp[SND_MSXAUDIO] && (a & 0xfe) == 0xc0)
		THIS_->sndp[SND_MSXAUDIO]->write(THIS_->sndp[SND_MSXAUDIO]->ctx, a & 1, v);
	else if (a == 0xfe && THIS_->bankmode == KSS_16K_MAPPER)
		KSSMaper16KWrite(THIS_, 0x8000, v);
}

static void vsync_event(KMEVENT *event, KMEVENT_ITEM_ID curid, KSSSEQ *THIS_)
{
	vsync_setup(THIS_);
	if (THIS_->ctx.regs8[REGID_HALTED]) play_setup(THIS_, THIS_->playaddr);
}

static void reset(KSSSEQ *THIS_)
{
	Uint32 i, freq, song;

	freq = NESAudioFrequencyGet();
	song = SONGINFO_GetSongNo() - 1;
	if (song >= THIS_->maxsong) song = THIS_->startsong - 1;

	/* sound reset */
	if (THIS_->extdevice & EXTDEVICE_SNG)
	{
		if (THIS_->sndp[SND_FMUNIT] && usertone_enable[1]) THIS_->sndp[SND_FMUNIT]->setinst(THIS_->sndp[SND_FMUNIT]->ctx, 0, usertone[1], 16 * 19);
	}
	else
	{
		if (THIS_->sndp[SND_MSXMUSIC] && usertone_enable[0]) THIS_->sndp[SND_MSXMUSIC]->setinst(THIS_->sndp[SND_MSXMUSIC]->ctx, 0, usertone[0], 16 * 19);
	}
	for (i = 0; i < SND_MAX; i++)
	{
		if (THIS_->sndp[i]) THIS_->sndp[i]->reset(THIS_->sndp[i]->ctx, BASECYCLES, freq);
	}

	/* memory reset */
	XMEMSET(THIS_->ram, 0xC9, 0x4000);
	XMEMCPY(THIS_->ram + 0x93, "\xC3\x01\x00\xC3\x09\x00", 6);
	XMEMCPY(THIS_->ram + 0x01, "\xD3\xA0\xF5\x7B\xD3\xA1\xF1\xC9", 8);
	XMEMCPY(THIS_->ram + 0x09, "\xD3\xA0\xDB\xA2\xC9", 5);
	XMEMSET(THIS_->ram + 0x4000, 0, 0xC000);
	if (THIS_->datasize)
	{
		if (THIS_->dataaddr + THIS_->datasize > 0x10000)
			XMEMCPY(THIS_->ram + THIS_->dataaddr, THIS_->data, 0x10000 - THIS_->dataaddr);
		else
			XMEMCPY(THIS_->ram + THIS_->dataaddr, THIS_->data, THIS_->datasize);
	}
	for (i = 0; i < 8; i++)
	{
		THIS_->readmap[i] = THIS_->ram;
		THIS_->writemap[i] = THIS_->ram;
	}
	THIS_->writemap[4] = THIS_->writemap[5] = 0;
	if (THIS_->majutushimode)
	{
		THIS_->writemap[2] = 0;
		THIS_->writemap[3] = 0;
	}
	if (!(THIS_->extdevice & EXTDEVICE_SNG)) THIS_->sccenable = !THIS_->rammode;

	/* cpu reset */
	kmz80_reset(&THIS_->ctx);
	THIS_->ctx.user = THIS_;
	THIS_->ctx.kmevent = &THIS_->kme;
	THIS_->ctx.memread = read_event;
	THIS_->ctx.memwrite = write_event;
	if (THIS_->extdevice & EXTDEVICE_SNG)
	{
		THIS_->ctx.ioread = ioread_eventSMS;
		THIS_->ctx.iowrite = iowrite_eventSMS;
		THIS_->ctx.regs8[REGID_M1CYCLE] = 1;
	}
	else
	{
		THIS_->ctx.ioread = ioread_eventMSX;
		THIS_->ctx.iowrite = iowrite_eventMSX;
		THIS_->ctx.regs8[REGID_M1CYCLE] = 2;
	}
	THIS_->ctx.busread = busread_event;

	THIS_->ctx.regs8[REGID_A] = (song >> 0) & 0xff;
	THIS_->ctx.regs8[REGID_H] = (song >> 8) & 0xff;
	play_setup(THIS_, THIS_->initaddr);

	THIS_->ctx.exflag = 3;	/* ICE/ACI */
	THIS_->ctx.regs8[REGID_IFF1] = THIS_->ctx.regs8[REGID_IFF2] = 0;
	THIS_->ctx.regs8[REGID_INTREQ] = 0;
	THIS_->ctx.regs8[REGID_IMODE] = 1;

	/* vsync reset */
	kmevent_init(&THIS_->kme);
	THIS_->vsync_id = kmevent_alloc(&THIS_->kme);
	kmevent_setevent(&THIS_->kme, THIS_->vsync_id, vsync_event, THIS_);
	THIS_->cpsgap = THIS_->cpsrem  = 0;
	THIS_->cpfrem  = 0;
	THIS_->cps = DivFix(BASECYCLES, freq, SHIFT_CPS);
	if (THIS_->extdevice & EXTDEVICE_PAL)
		THIS_->cpf = (4 * 342 * 313 / 6) << SHIFT_CPS;
	else
		THIS_->cpf = (4 * 342 * 262 / 6) << SHIFT_CPS;

	{
		/* request execute */
		Uint32 initbreak = 5 << 8; /* 5sec */
		while (!THIS_->ctx.regs8[REGID_HALTED] && --initbreak) kmz80_exec(&THIS_->ctx, BASECYCLES >> 8);
	}
	vsync_setup(THIS_);
	if (THIS_->ctx.regs8[REGID_HALTED])
	{
		play_setup(THIS_, THIS_->playaddr);
	}
	THIS_->total_cycles = 0;
}

static void terminate(KSSSEQ *THIS_)
{
	Uint32 i;
	for (i = 0; i < SND_MAX; i++)
	{
		if (THIS_->sndp[i]) THIS_->sndp[i]->release(THIS_->sndp[i]->ctx);
	}
	if (THIS_->data) XFREE(THIS_->data);
	XFREE(THIS_);
}

static Uint32 load(KSSSEQ *THIS_, Uint8 *pData, Uint32 uSize)
{
	Uint32 i, headersize;
	XMEMSET(THIS_, 0, sizeof(KSSSEQ));
	for (i = 0; i < SND_MAX; i++) THIS_->sndp[i] = 0;
	for (i = 0; i < SND_MAX; i++) THIS_->vola[i] = 0x80;
	THIS_->data = 0;

	if (GetDwordLE(pData) == GetDwordLEM("KSCC"))
	{
		headersize = 0x10;
		THIS_->startsong = 1;
		THIS_->maxsong = 256;
	}
	else if (GetDwordLE(pData) == GetDwordLEM("KSSX"))
	{
		headersize = 0x10 + pData[0x0e];
		if (pData[0x0e] >= 0x0c)
		{
			THIS_->startsong = GetWordLE(pData + 0x18) + 1;
			THIS_->maxsong = GetWordLE(pData + 0x1a) + 1;
		}
		else
		{
			THIS_->startsong = 1;
			THIS_->maxsong = 256;
		}
		if (pData[0x0e] >= 0x10)
		{
			THIS_->vola[SND_PSG] = 0x100 - (pData[0x1c] ^ 0x80);
			THIS_->vola[SND_SCC] = 0x100 - (pData[0x1d] ^ 0x80);
			THIS_->vola[SND_MSXMUSIC] = 0x100 - (pData[0x1e] ^ 0x80);
			THIS_->vola[SND_MSXAUDIO] = 0x100 - (pData[0x1f] ^ 0x80);
		}
	}
	else
	{
		return NESERR_FORMAT;
	}
	THIS_->dataaddr = GetWordLE(pData + 0x04);
	THIS_->datasize = GetWordLE(pData + 0x06);
	THIS_->initaddr = GetWordLE(pData + 0x08);
	THIS_->playaddr = GetWordLE(pData + 0x0A);
	THIS_->bankofs = pData[0x0C];
	THIS_->banknum = pData[0x0D];
	THIS_->extdevice = pData[0x0F];
	SONGINFO_SetStartSongNo(THIS_->startsong);
	SONGINFO_SetMaxSongNo(THIS_->maxsong);
	SONGINFO_SetInitAddress(THIS_->initaddr);
	SONGINFO_SetPlayAddress(THIS_->playaddr);
	SONGINFO_SetExtendDevice(THIS_->extdevice << 8);


	if (THIS_->banknum & 0x80)
	{
		THIS_->banknum &= 0x7f;
		THIS_->bankmode = KSS_8K_MAPPER;
		THIS_->banksize = 0x2000;
	}
	else if (THIS_->banknum)
	{
		THIS_->bankmode = KSS_16K_MAPPER;
		THIS_->banksize = 0x4000;
	}
	else
	{
		THIS_->bankmode = KSS_BANK_OFF;
	}
	THIS_->data = XMALLOC(THIS_->datasize + THIS_->banksize * THIS_->banknum + 8);
	if (!THIS_->data) return NESERR_SHORTOFMEMORY;
	THIS_->bankbase = THIS_->data + THIS_->datasize;
	if (uSize > 0x10 && THIS_->datasize)
	{
		Uint32 size;
		XMEMSET(THIS_->data, 0, THIS_->datasize);
		if (THIS_->datasize < uSize - headersize)
			size = THIS_->datasize;
		else
			size = uSize - headersize;
		XMEMCPY(THIS_->data, pData + headersize, size);
	}
	else
	{
		THIS_->datasize = 0;
	}
	if (uSize > (headersize + THIS_->datasize) && THIS_->bankmode != KSS_BANK_OFF)
	{
		Uint32 size;
		XMEMSET(THIS_->bankbase, 0, THIS_->banksize * THIS_->banknum);
		if (THIS_->banksize * THIS_->banknum < uSize - (headersize + THIS_->datasize))
			size = THIS_->banksize * THIS_->banknum;
		else
			size = uSize - (headersize + THIS_->datasize);
		XMEMCPY(THIS_->bankbase, pData + (headersize + THIS_->datasize), size);
	}
	else
	{
		THIS_->banknum = 0;
		THIS_->bankmode = KSS_BANK_OFF;
	}

	THIS_->majutushimode = 0;
	if (THIS_->extdevice & EXTDEVICE_SNG)
	{
		THIS_->synthmode = SYNTHMODE_SMS;
		if (THIS_->extdevice & EXTDEVICE_GGSTEREO)
		{
			THIS_->sndp[SND_SNG] = SNGSoundAlloc(SNG_TYPE_GAMEGEAR);
			SONGINFO_SetChannel(2);
		}
		else
		{
			THIS_->sndp[SND_SNG] = SNGSoundAlloc(SNG_TYPE_SN76496);
			SONGINFO_SetChannel(1);
		}
		if (!THIS_->sndp[SND_SNG]) return NESERR_SHORTOFMEMORY;
		if (THIS_->extdevice & EXTDEVICE_FMUNIT)
		{
			THIS_->sndp[SND_FMUNIT] = OPLSoundAlloc(OPL_TYPE_SMSFMUNIT);
			if (!THIS_->sndp[SND_FMUNIT]) return NESERR_SHORTOFMEMORY;
		}
		if (THIS_->extdevice & (EXTDEVICE_EXRAM | EXTDEVICE_GGRAM))
		{
			THIS_->rammode = 1;
		}
		else
		{
			THIS_->rammode = 0;
		}
		THIS_->sccenable = 0;
	}
	else
	{
		if (THIS_->extdevice & EXTDEVICE_MSXSTEREO)
		{
			if (THIS_->extdevice & EXTDEVICE_MSXAUDIO)
			{
				SONGINFO_SetChannel(2);
				THIS_->synthmode = SYNTHMODE_MSXSTEREO;
			}
			else
			{
				SONGINFO_SetChannel(1);
				THIS_->synthmode = SYNTHMODE_MSX;
				THIS_->majutushimode = 1;
			}
		}
		else
		{
			SONGINFO_SetChannel(1);
			THIS_->synthmode = SYNTHMODE_MSX;
		}
		THIS_->sndp[SND_PSG] = PSGSoundAlloc(PSG_TYPE_AY_3_8910);
		if (!THIS_->sndp[SND_PSG]) return NESERR_SHORTOFMEMORY;
		if (THIS_->extdevice & EXTDEVICE_MSXMUSIC)
		{
			THIS_->sndp[SND_MSXMUSIC] = OPLSoundAlloc(OPL_TYPE_MSXMUSIC);
			if (!THIS_->sndp[SND_MSXMUSIC]) return NESERR_SHORTOFMEMORY;
		}
		if (THIS_->extdevice & EXTDEVICE_MSXAUDIO)
		{
			THIS_->sndp[SND_MSXAUDIO] = OPLSoundAlloc(OPL_TYPE_MSXAUDIO);
			if (!THIS_->sndp[SND_MSXAUDIO]) return NESERR_SHORTOFMEMORY;
		}
		if (THIS_->extdevice & EXTDEVICE_EXRAM)
		{
			THIS_->rammode = 1;
			THIS_->sccenable = 0;
		}
		else
		{
			THIS_->sndp[SND_SCC] = SCCSoundAlloc();
			if (!THIS_->sndp[SND_SCC]) return NESERR_SHORTOFMEMORY;
			THIS_->rammode = (THIS_->extdevice & EXTDEVICE_MSXRAM) != 0;
			THIS_->sccenable = !THIS_->rammode;
		}
	}
	return NESERR_NOERROR;
}

static KSSSEQ *kssseq = 0;
static Int32 __fastcall KSSSEQExecuteZ80CPU(void)
{
	execute(kssseq);
	return 0;
}

static void __fastcall KSSSEQSoundRenderStereo(Int32 *d)
{
	synth(kssseq, d);
}

static Int32 __fastcall KSSSEQSoundRenderMono(void)
{
	Int32 d[2] = { 0, 0 };
	synth(kssseq, d);
#if (((-1) >> 1) == -1)
	return (d[0] + d[1]) >> 1;
#else
	return (d[0] + d[1]) / 2;
#endif
}

static NES_AUDIO_HANDLER kssseq_audio_handler[] = {
	{ 0, KSSSEQExecuteZ80CPU, 0, },
	{ 3, KSSSEQSoundRenderMono, KSSSEQSoundRenderStereo },
	{ 0, 0, 0, },
};

static void __fastcall KSSSEQVolume(Uint32 v)
{
	if (kssseq)
	{
		volume(kssseq, v);
	}
}

static NES_VOLUME_HANDLER kssseq_volume_handler[] = {
	{ KSSSEQVolume, }, 
	{ 0, }, 
};

static void __fastcall KSSSEQReset(void)
{
	if (kssseq) reset(kssseq);
}

static NES_RESET_HANDLER kssseq_reset_handler[] = {
	{ NES_RESET_SYS_LAST, KSSSEQReset, },
	{ 0,                  0, },
};

static void __fastcall KSSSEQTerminate(void)
{
	if (kssseq)
	{
		terminate(kssseq);
		kssseq = 0;
	}
}

static NES_TERMINATE_HANDLER kssseq_terminate_handler[] = {
	{ KSSSEQTerminate, },
	{ 0, },
};

Uint32 KSSLoad(Uint8 *pData, Uint32 uSize)
{
	Uint32 ret;
	KSSSEQ *THIS_;
	if (kssseq) *((char *)(0)) = 0;	/* ASSERT */
	THIS_ = XMALLOC(sizeof(KSSSEQ));
	if (!THIS_) return NESERR_SHORTOFMEMORY;
	ret = load(THIS_, pData, uSize);
	if (ret)
	{
		terminate(THIS_);
		return ret;
	}
	kssseq = THIS_;
	NESAudioHandlerInstall(kssseq_audio_handler);
	NESVolumeHandlerInstall(kssseq_volume_handler);
	NESResetHandlerInstall(kssseq_reset_handler);
	NESTerminateHandlerInstall(kssseq_terminate_handler);
	return ret;
}
