#include "neserr.h"
#include "handler.h"
#include "audiosys.h"
#include "songinfo.h"
#include "nsdout.h"

#include "m_hes.h"

#include "device/s_hes.h"
#include "device/divfix.h"

#include "kmz80/kmevent.h"

/* -------------------- */
/*  km6502 HuC6280 I/F  */
/* -------------------- */
#define USE_DIRECT_ZEROPAGE 0
#define USE_CALLBACK 1
#define USE_INLINEMMC 0
#define USE_USERPOINTER 1
#define External __inline static
#include "km6502/km6280m.h"

#define SHIFT_CPS 15
#define HES_BASECYCLES (21477270)
#define HES_TIMERCYCLES (1024 * 3)

#if 0

HES
system clock 21477270Hz
CPUH clock 21477270Hz system clock 
CPUL clock 3579545 system clock / 6

FF		I/O
F9-FB	SGX-RAM
F8		RAM
80-		CD-ROM^2 RAM
00-		ROM

#endif

#define DEBUGX2 0
#if DEBUGX2
#define DUMP_MAX 65536
#include <stdio.h>
void dox(int i, int s, int pc, int flag, int a, int x, int y)
{
	static int p = 0;
	static FILE *fp = NULL;
	static int stop = 0;
	if (i == -1000)
	{
		stop = 1;
		return;
	}
	if (i == -10000)
	{
		stop = 0;
		return;
	}
	if (stop) return;
	if (p >= DUMP_MAX)
	{
		if (fp) { fclose(fp); fp = 0; }
		return;
	}
	if (!fp) fp = fopen("km6280d.out", "wb");
	if (fp) { 
		fputc(i, fp);
		fputc(s, fp);
		fputc(pc >> 8, fp);
		fputc(pc, fp);
		fputc(flag, fp);
		fputc(a, fp);
		fputc(x, fp);
		fputc(y, fp);
		p++;
	}
}
#define DEBUG_OUTPUT(i,s,pc,p,a,x,y) dox(i,s,pc,p,a,x,y)
#else
#define DEBUG_OUTPUT(i,s,pc,p,a,x,y) {}
#endif



typedef struct HESHES_TAG HESHES;
typedef Uint32 (*READPROC)(HESHES *THIS_, Uint32 a);
typedef void (*WRITEPROC)(HESHES *THIS_, Uint32 a, Uint32 v);

struct  HESHES_TAG {
	struct K6280_Context ctx;
	KMIF_SOUND_DEVICE *hessnd;
	KMEVENT kme;
	KMEVENT_ITEM_ID vsync;
	KMEVENT_ITEM_ID timer;

	Uint32 bp;			/* break point */
	Uint32 breaked;		/* break point flag */

	Uint32 cps;				/* cycles per sample:fixed point */
	Uint32 cpsrem;			/* cycle remain */
	Uint32 cpsgap;			/* cycle gap */
	Uint32 total_cycles;	/* total played cycles */

	Uint8 mpr[0x8];
	Uint8 firstmpr[0x8];
	Uint8 *memmap[0x100];
	Uint32 initaddr;

	Uint32 playerromaddr;
	Uint8 playerrom[0x10];

	Uint8 hestim_RELOAD;
	Uint8 hestim_COUNTER;
	Uint8 hestim_START;
	Uint8 hesvdc_STATUS;
	Uint8 hesvdc_CR;
	Uint8 hesvdc_ADR;
};

static Uint32 km6280_exec(struct K6280_Context *ctx, Uint32 cycles)
{
	HESHES *THIS_ = ctx->user;
	Uint32 kmecycle;
	kmecycle = ctx->clock = 0;
	while (ctx->clock < cycles)
	{
		if (!THIS_->breaked)
		{
			K6280_Exec(ctx);	/* Execute 1op */
			DEBUG_OUTPUT(ctx->lastcode, ctx->S, ctx->PC, ctx->P, ctx->A, ctx->X, ctx->Y);
			if (ctx->PC == THIS_->bp)
			{
				THIS_->breaked = 1;
			}
		}
		else
		{
			Uint32 nextcount;
			/* break時は次のイベントまで一度に進める */
			nextcount = THIS_->kme.item[THIS_->kme.item[0].next].count;
			if (kmevent_gettimer(&THIS_->kme, 0, &nextcount))
			{
				/* イベント有り */
				if (ctx->clock + nextcount < cycles)
					ctx->clock += nextcount;	/* 期間中にイベント有り */
				else
					ctx->clock = cycles;		/* 期間中にイベント無し */
			}
			else
			{
				/* イベント無し */
				ctx->clock = cycles;
			}
		}
		/* イベント進行 */
		kmevent_process(&THIS_->kme, ctx->clock - kmecycle);
		kmecycle = ctx->clock;
	}
	ctx->clock = 0;
	return kmecycle;
}

static Int32 execute(HESHES *THIS_)
{
	Uint32 cycles;
	THIS_->cpsrem += THIS_->cps;
	cycles = THIS_->cpsrem >> SHIFT_CPS;
	if (THIS_->cpsgap >= cycles)
		THIS_->cpsgap -= cycles;
	else
	{
		Uint32 excycles = cycles - THIS_->cpsgap;
		THIS_->cpsgap = km6280_exec(&THIS_->ctx, excycles) - excycles;
	}
	THIS_->cpsrem &= (1 << SHIFT_CPS) - 1;
	THIS_->total_cycles += cycles;
	return 0;
}

__inline static void synth(HESHES *THIS_, Int32 *d)
{
	THIS_->hessnd->synth(THIS_->hessnd->ctx, d);
}

__inline static void volume(HESHES *THIS_, Uint32 v)
{
	THIS_->hessnd->volume(THIS_->hessnd->ctx, v);
}


static void vsync_setup(HESHES *THIS_)
{
	kmevent_settimer(&THIS_->kme, THIS_->vsync, 4 * 342 * 262);
}

static void timer_setup(HESHES *THIS_)
{
	kmevent_settimer(&THIS_->kme, THIS_->timer, HES_TIMERCYCLES);
}

static void write_6270(HESHES *THIS_, Uint32 a, Uint32 v)
{
	switch (a)
	{
		case 0:
			THIS_->hesvdc_ADR = v;
			break;
		case 2:
			switch (THIS_->hesvdc_ADR)
			{
				case 5:	/* CR */
					THIS_->hesvdc_CR = v;
					break;
			}
			break;
		case 3:
			break;
	}
}

static Uint32 read_6270(HESHES *THIS_, Uint32 a)
{
	Uint32 v = 0;
	if (a == 0)
	{
		if (THIS_->hesvdc_STATUS)
		{
			THIS_->hesvdc_STATUS = 0;
			v = 0x20;
		}
		THIS_->ctx.iRequest &= ~K6280_INT1;
#if 1
		v = 0x20;	/* 常にVSYNC期間 */
#endif
	}
	return v;
}

static Uint32 read_io(HESHES *THIS_, Uint32 a)
{
	switch (a >> 10)
	{
		case 0:	/* VDC */	
			return read_6270(THIS_, a & 3);
		case 2:	/* PSG */
			return THIS_->hessnd->read(THIS_->hessnd->ctx, a & 0xf);
		case 3:	/* TIMER */
			if (a & 1)
				return THIS_->hestim_START;
			else
				return THIS_->hestim_COUNTER;
		case 5:	/* IRQ */
			switch (a & 15)
			{
				case 2:
					{
						Uint32 v = 0xf8;
						if (!(THIS_->ctx.iMask & K6280_TIMER)) v |= 4;
						if (!(THIS_->ctx.iMask & K6280_INT1)) v |= 2;
						if (!(THIS_->ctx.iMask & K6280_INT2)) v |= 1;
						return v;
					}
				case 3:
					{
						Uint8 v = 0;
						if (THIS_->ctx.iRequest & K6280_TIMER) v |= 4;
						if (THIS_->ctx.iRequest & K6280_INT1) v |= 2;
						if (THIS_->ctx.iRequest & K6280_INT2) v |= 1;
#if 0
						THIS_->ctx.iRequest &= ~(K6280_TIMER | K6280_INT1 | K6280_INT2);
#endif
						return v;
					}
			}
			return 0x00;
		case 7:
			a -= THIS_->playerromaddr;
			if (a < 0x10) return THIS_->playerrom[a];
			return 0xff;
		default:
		case 1:	/* VCE */
		case 4:	/* PAD */
		case 6:	/* CDROM */
			return 0xff;
	}
}

static void write_io(HESHES *THIS_, Uint32 a, Uint32 v)
{
	switch (a >> 10)
	{
		case 0:	/* VDC */
			write_6270(THIS_, a & 3, v);
			break;
		case 2:	/* PSG */
			THIS_->hessnd->write(THIS_->hessnd->ctx, a & 0xf, v);
			break;
		case 3:	/* TIMER */
			switch (a & 1)
			{
				case 0:
					THIS_->hestim_RELOAD = v & 127;
					break;
				case 1:
					v &= 1;
					if (v && !THIS_->hestim_START)
						THIS_->hestim_COUNTER = THIS_->hestim_RELOAD;
					THIS_->hestim_START = v;
					break;
			}
			break;
		case 5:	/* IRQ */
			switch (a & 15)
			{
				case 2:
					THIS_->ctx.iMask &= ~(K6280_TIMER | K6280_INT1 | K6280_INT2);
					if (!(v & 4)) THIS_->ctx.iMask |= K6280_TIMER;
					if (!(v & 2)) THIS_->ctx.iMask |= K6280_INT1;
					if (!(v & 1)) THIS_->ctx.iMask |= K6280_INT2;
					break;
				case 3:
					THIS_->ctx.iRequest &= ~K6280_TIMER;
					break;
			}
			break;
		default:
		case 1:	/* VCE */
		case 4:	/* PAD */
		case 6:	/* CDROM */
		case 7:
			break;
			
	}
}

static Uint32 Callback read_event(HESHES *THIS_, Uword a)
{
	Uint8 page = THIS_->mpr[a >> 13];
	if (THIS_->memmap[page])
		return THIS_->memmap[page][a & 0x1fff];
	else if (page == 0xff)
		return read_io(THIS_, a & 0x1fff);
	else
		return 0xff;
}

static void Callback write_event(HESHES *THIS_, Uword a, Uword v)
{
	Uint8 page = THIS_->mpr[a >> 13];
	if (THIS_->memmap[page])
		THIS_->memmap[page][a & 0x1fff] = v;
	else if (page == 0xff)
		write_io(THIS_, a & 0x1fff, v);
}

static Uint32 Callback readmpr_event(HESHES *THIS_, Uword a)
{
	Uint32 i;
	for (i = 0; i < 8; i++) if (a & (1 << i)) return THIS_->mpr[i];
	return 0xff;
}

static void Callback writempr_event(HESHES *THIS_, Uword a, Uword v)
{
	Uint32 i;
	if (v < 0x80 && !THIS_->memmap[v]) return;
	for (i = 0; i < 8; i++) if (a & (1 << i)) THIS_->mpr[i] = v;
}

static void Callback write6270_event(HESHES *THIS_, Uword a, Uword v)
{
	write_6270(THIS_, a & 0x1fff, v);
}


static void vsync_event(KMEVENT *event, KMEVENT_ITEM_ID curid, HESHES *THIS_)
{
	vsync_setup(THIS_);
	if (THIS_->hesvdc_CR & 8)
	{
		THIS_->ctx.iRequest |= K6280_INT1;
		THIS_->breaked = 0;
	}
	THIS_->hesvdc_STATUS = 1;
	if (NSD_out_mode) NSDWrite(NSD_VSYNC, 0, 0);
}

static void timer_event(KMEVENT *event, KMEVENT_ITEM_ID curid, HESHES *THIS_)
{
	if (THIS_->hestim_START && THIS_->hestim_COUNTER-- == 0)
	{
		THIS_->hestim_COUNTER = THIS_->hestim_RELOAD;
		THIS_->ctx.iRequest |= K6280_TIMER;
		THIS_->breaked = 0;
	}
	timer_setup(THIS_);
}

static void reset(HESHES *THIS_)
{
	Uint32 i, initbreak;
	Uint32 freq = NESAudioFrequencyGet();

	THIS_->hessnd->reset(THIS_->hessnd->ctx, HES_BASECYCLES, freq);
	kmevent_init(&THIS_->kme);

	/* RAM CLEAR */
	for (i = 0xf8; i <= 0xfb; i++)
		if (THIS_->memmap[i]) XMEMSET(THIS_->memmap[i], 0, 0x2000);

	THIS_->cps = DivFix(HES_BASECYCLES, freq, SHIFT_CPS);
	THIS_->ctx.user = THIS_;
	THIS_->ctx.ReadByte = read_event;
	THIS_->ctx.WriteByte = write_event;
	THIS_->ctx.ReadMPR = readmpr_event;
	THIS_->ctx.WriteMPR = writempr_event;
	THIS_->ctx.Write6270 = write6270_event;

	THIS_->vsync = kmevent_alloc(&THIS_->kme);
	THIS_->timer = kmevent_alloc(&THIS_->kme);
	kmevent_setevent(&THIS_->kme, THIS_->vsync, vsync_event, THIS_);
	kmevent_setevent(&THIS_->kme, THIS_->timer, timer_event, THIS_);

	THIS_->bp = THIS_->playerromaddr + 3;
	for (i = 0; i < 8; i++) THIS_->mpr[i] = THIS_->firstmpr[i];

	THIS_->breaked = 0;
	THIS_->cpsrem = THIS_->cpsgap = THIS_->total_cycles = 0;

	THIS_->ctx.A = (SONGINFO_GetSongNo() - 1) & 0xff;
	THIS_->ctx.P = K6280_Z_FLAG + K6280_I_FLAG;
	THIS_->ctx.X = THIS_->ctx.Y = 0;
	THIS_->ctx.S = 0xFF;
	THIS_->ctx.PC = THIS_->playerromaddr;
	THIS_->ctx.iRequest = 0;
	THIS_->ctx.iMask = ~0;
	THIS_->ctx.lowClockMode = 0;

	THIS_->playerrom[0x00] = 0x20;	/* JSR */
	THIS_->playerrom[0x01] = (THIS_->initaddr >> 0) & 0xFF;
	THIS_->playerrom[0x02] = (THIS_->initaddr >> 8) & 0xFF;
	THIS_->playerrom[0x03] = 0x4c;	/* JMP */
	THIS_->playerrom[0x04] = ((THIS_->playerromaddr + 3) >> 0) & 0xFF;
	THIS_->playerrom[0x05] = ((THIS_->playerromaddr + 3) >> 8) & 0xFF;

	THIS_->hesvdc_STATUS = 0;
	THIS_->hesvdc_CR = 0;
	THIS_->hesvdc_ADR = 0;
	vsync_setup(THIS_);
	THIS_->hestim_RELOAD = THIS_->hestim_COUNTER = THIS_->hestim_START = 0;
	timer_setup(THIS_);

	/* request execute(5sec) */
	initbreak = 5 << 8;
	while (!THIS_->breaked && --initbreak)
		km6280_exec(&THIS_->ctx, HES_BASECYCLES >> 8);

	if (THIS_->breaked)
	{
		THIS_->breaked = 0;
		THIS_->ctx.P &= ~K6280_I_FLAG;
	}

	THIS_->cpsrem = THIS_->cpsgap = THIS_->total_cycles = 0;
}

static void terminate(HESHES *THIS_)
{
	Uint32 i;
	if (THIS_->hessnd) THIS_->hessnd->release(THIS_->hessnd->ctx);
	for (i = 0; i < 0x100; i++) if (THIS_->memmap[i]) XFREE(THIS_->memmap[i]);
	XFREE(THIS_);
}

static Uint32 GetWordLE(Uint8 *p)
{
	return p[0] | (p[1] << 8);
}

static Uint32 GetDwordLE(Uint8 *p)
{
	return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

static Uint32 alloc_physical_address(HESHES *THIS_, Uint32 a, Uint32 l)
{
	Uint8 page = a >> 13;
	Uint8 lastpage = (a + l - 1) >> 13;
	for (; page <= lastpage; page++)
	{
		if (!THIS_->memmap[page])
		{
			THIS_->memmap[page] = XMALLOC(0x2000);
			if (!THIS_->memmap[page]) return 0;
			XMEMSET(THIS_->memmap[page], 0, 0x2000);
		}
	}
	return 1;
}

static void copy_physical_address(HESHES *THIS_, Uint32 a, Uint32 l, Uint8 *p)
{
	Uint8 page = a >> 13;
	Uint32 w;
	if (a & 0x1fff)
	{
		w = 0x2000 - a & 0x1fff;
		if (w > l) w = l;
		XMEMCPY(THIS_->memmap[page++] + (a & 0x1fff), p, w);
		p += w;
		l -= w;
	}
	while (l)
	{
		w = (l > 0x2000) ? 0x2000 : l;
		XMEMCPY(THIS_->memmap[page++], p, w);
		p += w;
		l -= w;
	}
}

static Uint32 load(HESHES *THIS_, Uint8 *pData, Uint32 uSize)
{
	Uint32 i, p;
	XMEMSET(THIS_, 0, sizeof(HESHES));
	THIS_->hessnd = 0;
	for (i = 0; i < 0x100; i++) THIS_->memmap[i] = 0;

	if (uSize < 0x20) return NESERR_FORMAT;
	SONGINFO_SetStartSongNo(pData[5] + 1);
	SONGINFO_SetMaxSongNo(256);
	SONGINFO_SetChannel(2);
	SONGINFO_SetExtendDevice(0);
	for (i = 0; i < 8; i++) THIS_->firstmpr[i] = pData[8 + i];
	THIS_->playerromaddr = 0x1ff0;
	THIS_->initaddr = GetWordLE(pData + 0x06);
	SONGINFO_SetInitAddress(THIS_->initaddr);
	SONGINFO_SetPlayAddress(0);
	if (!alloc_physical_address(THIS_, 0xf8 << 13, 0x2000))	/* RAM */
		return NESERR_SHORTOFMEMORY;
	if (!alloc_physical_address(THIS_, 0xf9 << 13, 0x2000))	/* SGX-RAM */
		return NESERR_SHORTOFMEMORY;
	if (!alloc_physical_address(THIS_, 0xfa << 13, 0x2000))	/* SGX-RAM */
		return NESERR_SHORTOFMEMORY;
	if (!alloc_physical_address(THIS_, 0xfb << 13, 0x2000))	/* SGX-RAM */
		return NESERR_SHORTOFMEMORY;
	if (!alloc_physical_address(THIS_, 0x00 << 13, 0x2000))	/* IPL-ROM */
		return NESERR_SHORTOFMEMORY;
	for (p = 0x10; p + 0x10 < uSize; p += 0x10 + GetDwordLE(pData + p + 4))
	{
		if (GetDwordLE(pData + p) == 0x41544144)	/* 'DATA' */
		{
			Uint32 a, l;
			l = GetDwordLE(pData + p + 4);
			a = GetDwordLE(pData + p + 8);
			if (!alloc_physical_address(THIS_, a, l)) return NESERR_SHORTOFMEMORY;
			if (l > uSize - p - 0x10) l = uSize - p - 0x10;
			copy_physical_address(THIS_, a, l, pData + p + 0x10);
		}
	}
	THIS_->hessnd = HESSoundAlloc();
	if (!THIS_->hessnd) return NESERR_SHORTOFMEMORY;
	return NESERR_NOERROR;
}






static HESHES *heshes = 0;
static Int32 __fastcall ExecuteHES(void)
{
	return heshes ? execute(heshes) : 0;
}

static void __fastcall HESSoundRenderStereo(Int32 *d)
{
	synth(heshes, d);
}

static Int32 __fastcall HESSoundRenderMono(void)
{
	Int32 d[2] = { 0,0 } ;
	synth(heshes, d);
#if (((-1) >> 1) == -1)
	return (d[0] + d[1]) >> 1;
#else
	return (d[0] + d[1]) / 2;
#endif
}

static NES_AUDIO_HANDLER heshes_audio_handler[] = {
	{ 0, ExecuteHES, 0, },
	{ 3, HESSoundRenderMono, HESSoundRenderStereo },
	{ 0, 0, 0, },
};

static void __fastcall HESHESVolume(Uint32 v)
{
	if (heshes)
	{
		volume(heshes, v);
	}
}

static NES_VOLUME_HANDLER heshes_volume_handler[] = {
	{ HESHESVolume, }, 
	{ 0, }, 
};

static void __fastcall HESHESReset(void)
{
	if (heshes) reset(heshes);
}

static NES_RESET_HANDLER heshes_reset_handler[] = {
	{ NES_RESET_SYS_LAST, HESHESReset, },
	{ 0,                  0, },
};

static void __fastcall HESHESTerminate(void)
{
	if (heshes)
	{
		terminate(heshes);
		heshes = 0;
	}
}

static NES_TERMINATE_HANDLER heshes_terminate_handler[] = {
	{ HESHESTerminate, },
	{ 0, },
};

Uint32 HESLoad(Uint8 *pData, Uint32 uSize)
{
	Uint32 ret;
	HESHES *THIS_;
	if (heshes) *((char *)(0)) = 0;	/* ASSERT */
	THIS_ = XMALLOC(sizeof(HESHES));
	if (!THIS_) return NESERR_SHORTOFMEMORY;
	ret = load(THIS_, pData, uSize);
	if (ret)
	{
		terminate(THIS_);
		return ret;
	}
	heshes = THIS_;
	NESAudioHandlerInstall(heshes_audio_handler);
	NESVolumeHandlerInstall(heshes_volume_handler);
	NESResetHandlerInstall(heshes_reset_handler);
	NESTerminateHandlerInstall(heshes_terminate_handler);
	return ret;
}
