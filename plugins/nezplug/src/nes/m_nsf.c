/* NSF mapper(4KBx8) */

#include "neserr.h"
#include "handler.h"
#include "audiosys.h"
#include "songinfo.h"
#include "m_nsf.h"
#include "m_hes.h"
#include "m_kss.h"
#include "m_gbr.h"
#include "m_zxay.h"
#include "nsf6502.h"
#include "nsdout.h"
#include "nsdplay.h"
#include "device/s_apu.h"
#include "device/s_mmc5.h"
#include "device/s_vrc6.h"
#include "device/s_vrc7.h"
#include "device/s_fds.h"
#include "device/s_n106.h"
#include "device/s_fme7.h"

#define NSF_MAPPER_STATIC 0

#define EXTSOUND_VRC6	(1 << 0)
#define EXTSOUND_VRC7	(1 << 1)
#define EXTSOUND_FDS	(1 << 2)
#define EXTSOUND_MMC5	(1 << 3)
#define EXTSOUND_N106	(1 << 4)
#define EXTSOUND_FME7	(1 << 5)
#define EXTSOUND_J86	(1 << 6)	/* JALECO-86 */

static struct {
	Uint8 *bankbase;
	Uint32 banksw;
	Uint32 banknum;
	Uint8 head[0x80];		/* nsf header */
	Uint8 ram[0x800];
#if !NSF_MAPPER_STATIC
	Uint8 *bank[8];
	Uint8 static_area[0x8000 - 0x6000];
	Uint8 zero_area[0x1000];
#else
	Uint8 static_area[0x10000 - 0x6000];
#endif
} nsf = {
	0, 0,
};

/* RAM area */
static Uint32 __fastcall ReadRam(Uint32 A)
{
	return nsf.ram[A & 0x07FF];
}
static void __fastcall WriteRam(Uint32 A, Uint32 V)
{
	nsf.ram[A & 0x07FF] = V;
}

/* SRAM area */
static Uint32 __fastcall ReadStaticArea(Uint32 A)
{
	return nsf.static_area[A - 0x6000];
}
static void __fastcall WriteStaticArea(Uint32 A, Uint32 V)
{
	nsf.static_area[A - 0x6000] = V;
}

#if !NSF_MAPPER_STATIC
static Uint32 __fastcall ReadRom8000(Uint32 A)
{
	return nsf.bank[0][A];
}
static Uint32 __fastcall ReadRom9000(Uint32 A)
{
	return nsf.bank[1][A];
}
static Uint32 __fastcall ReadRomA000(Uint32 A)
{
	return nsf.bank[2][A];
}
static Uint32 __fastcall ReadRomB000(Uint32 A)
{
	return nsf.bank[3][A];
}
static Uint32 __fastcall ReadRomC000(Uint32 A)
{
	return nsf.bank[4][A];
}
static Uint32 __fastcall ReadRomD000(Uint32 A)
{
	return nsf.bank[5][A];
}
static Uint32 __fastcall ReadRomE000(Uint32 A)
{
	return nsf.bank[6][A];
}
static Uint32 __fastcall ReadRomF000(Uint32 A)
{
	return nsf.bank[7][A];
}
#endif

/* Mapper I/O */
static void __fastcall WriteMapper(Uint32 A, Uint32 V)
{
	Uint32 bank;
	bank = A - 0x5FF0;
	if (bank < 6 || bank > 15) return;
#if !NSF_MAPPER_STATIC
	if (bank >= 8)
	{
		if (V < nsf.banknum)
		{
			if (NSD_out_mode) NSDWrite(NSD_NSF_MAPPER, A, V);
			nsf.bank[bank - 8] = &nsf.bankbase[V << 12] - (bank << 12);
		}
		else
			nsf.bank[bank - 8] = nsf.zero_area - (bank << 12);
		return;
	}
#endif
	if (V < nsf.banknum)
	{
		if (NSD_out_mode) NSDWrite(NSD_NSF_MAPPER, A, V);
		XMEMCPY(&nsf.static_area[(bank - 6) << 12], &nsf.bankbase[V << 12], 0x1000);
	}
	else
		XMEMSET(&nsf.static_area[(bank - 6) << 12], 0x00, 0x1000);
}

static NES_READ_HANDLER nsf_mapper_read_handler[] = {
	{ 0x0000,0x0FFF,ReadRam, },
	{ 0x1000,0x1FFF,ReadRam, },
	{ 0x6000,0x6FFF,ReadStaticArea, },
	{ 0x7000,0x7FFF,ReadStaticArea, },
#if !NSF_MAPPER_STATIC
	{ 0x8000,0x8FFF,ReadRom8000, },
	{ 0x9000,0x9FFF,ReadRom9000, },
	{ 0xA000,0xAFFF,ReadRomA000, },
	{ 0xB000,0xBFFF,ReadRomB000, },
	{ 0xC000,0xCFFF,ReadRomC000, },
	{ 0xD000,0xDFFF,ReadRomD000, },
	{ 0xE000,0xEFFF,ReadRomE000, },
	{ 0xF000,0xFFFF,ReadRomF000, },
#else
	{ 0x8000,0x8FFF,ReadStaticArea, },
	{ 0x9000,0x9FFF,ReadStaticArea, },
	{ 0xA000,0xAFFF,ReadStaticArea, },
	{ 0xB000,0xBFFF,ReadStaticArea, },
	{ 0xC000,0xCFFF,ReadStaticArea, },
	{ 0xD000,0xDFFF,ReadStaticArea, },
	{ 0xE000,0xEFFF,ReadStaticArea, },
	{ 0xF000,0xFFFF,ReadStaticArea, },
#endif
	{ 0     ,0     ,0, },
};
static NES_WRITE_HANDLER nsf_mapper_write_handler[] = {
	{ 0x0000,0x0FFF,WriteRam, },
	{ 0x1000,0x1FFF,WriteRam, },
	{ 0x6000,0x6FFF,WriteStaticArea, },
	{ 0x7000,0x7FFF,WriteStaticArea, },
	{ 0     ,0     ,0, },
};
static NES_WRITE_HANDLER nsf_mapper_write_handler2[] = {
	{ 0x5FF6,0x5FFF,WriteMapper, },
	{ 0     ,0     ,0, },
};

Uint8 *NSFGetHeader(void)
{
	return nsf.head;
}

static Uint GetWordLE(Uint8 *p)
{
	return p[0] | (p[1] << 8);
}

static Uint32 GetDwordLE(Uint8 *p)
{
	return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}
#define GetDwordLEM(p) (Uint32)((((Uint8 *)p)[0] | (((Uint8 *)p)[1] << 8) | (((Uint8 *)p)[2] << 16) | (((Uint8 *)p)[3] << 24)))

static void __fastcall ResetBank(void)
{
	Uint i, startbank;
	XMEMSET(nsf.ram, 0, 0x0800);
	startbank = GetWordLE(nsf.head + 8) >> 12;
	for (i = 0; i < 16; i++)
		WriteMapper(0x5FF0 + i, 0x10000/* off */);
	if (nsf.banksw)
	{
		for (i = 0; (startbank + i) < 8; i++)
			WriteMapper(0x5FF0 + (startbank + i) , i);
		if (nsf.banksw & 2)
		{
			WriteMapper(0x5FF6, nsf.head[0x70 + 6]);
			WriteMapper(0x5FF7, nsf.head[0x70 + 7]);
		}
		for (i = 8; i < 16; i++)
			WriteMapper(0x5FF0 + i, nsf.head[0x70 + i - 8]);
	}
	else
	{
		for (i = startbank; i < 16; i++)
			WriteMapper(0x5FF0 + i , i - startbank);
	}
}

static NES_RESET_HANDLER nsf_mapper_reset_handler[] = {
	{ NES_RESET_SYS_FIRST, ResetBank, },
	{ 0,                   0, },
};

static void __fastcall Terminate(void)
{
	if (nsf.bankbase)
	{
		XFREE(nsf.bankbase);
		nsf.bankbase = 0;
	}
}

static NES_TERMINATE_HANDLER nsf_mapper_terminate_handler[] = {
	{ Terminate, },
	{ 0, },
};

static Uint NSFMapperInitialize(Uint8 *pData, Uint uSize)
{
	Uint32 size = uSize;
	Uint8 *data = pData;

	size += GetWordLE(nsf.head + 8) & 0x0FFF;
	size  = (size + 0x0FFF) & ~0x0FFF;

	nsf.bankbase = XMALLOC(size + 8);
	if (!nsf.bankbase) return NESERR_SHORTOFMEMORY;

	nsf.banknum = size >> 12;
	XMEMSET(nsf.bankbase, 0, size);
	XMEMCPY(nsf.bankbase + (GetWordLE(nsf.head + 8) & 0x0FFF), data, uSize);

	nsf.banksw = nsf.head[0x70] || nsf.head[0x71] || nsf.head[0x72] || nsf.head[0x73] || nsf.head[0x74] || nsf.head[0x75] || nsf.head[0x76] || nsf.head[0x77];
	if (SONGINFO_GetExtendDevice() & EXTSOUND_FDS) nsf.banksw <<= 1;
	return NESERR_NOERROR;
}

static Uint NSFDeviceInitialize(void)
{
	NESResetHandlerInstall(nsf_mapper_reset_handler);
	NESTerminateHandlerInstall(nsf_mapper_terminate_handler);
	NESReadHandlerInstall(nsf_mapper_read_handler);
	NESWriteHandlerInstall(nsf_mapper_write_handler);
	if (nsf.banksw) NESWriteHandlerInstall(nsf_mapper_write_handler2);

	XMEMSET(nsf.ram, 0, 0x0800);
	XMEMSET(nsf.static_area, 0, sizeof(nsf.static_area));
#if !NSF_MAPPER_STATIC
	XMEMSET(nsf.zero_area, 0, sizeof(nsf.zero_area));
#endif

	APUSoundInstall();
	if (SONGINFO_GetExtendDevice() & EXTSOUND_VRC6) VRC6SoundInstall();
	if (SONGINFO_GetExtendDevice() & EXTSOUND_VRC7) VRC7SoundInstall();
	if (SONGINFO_GetExtendDevice() & EXTSOUND_FDS)  FDSSoundInstall();
	if (SONGINFO_GetExtendDevice() & EXTSOUND_MMC5)
	{
		MMC5SoundInstall();
		MMC5MutiplierInstall();
		MMC5ExtendRamInstall();
	}
	if (SONGINFO_GetExtendDevice() & EXTSOUND_N106) N106SoundInstall();
	if (SONGINFO_GetExtendDevice() & EXTSOUND_FME7) FME7SoundInstall();
	return NESERR_NOERROR;
}

static void NSFMapperSetInfo(Uint8 *pData)
{
	XMEMCPY(nsf.head, pData, 0x80);
	SONGINFO_SetStartSongNo(pData[0x07]);
	SONGINFO_SetMaxSongNo(pData[0x06]);
	SONGINFO_SetExtendDevice(pData[0x7B]);
	SONGINFO_SetInitAddress(GetWordLE(pData + 0x0A));
	SONGINFO_SetPlayAddress(GetWordLE(pData + 0x0C));
	SONGINFO_SetChannel(1);
}

static Uint NSDPlayerSetInfo(Uint8 *pData, Uint uSize)
{
	Uint ret;
	NESMemoryHandlerInitialize();
	XMEMSET(nsf.head, 0, 0x80);
	SONGINFO_SetStartSongNo(1);
	SONGINFO_SetMaxSongNo(1);
	SONGINFO_SetExtendDevice(pData[0x0C]);
	SONGINFO_SetInitAddress(0);
	SONGINFO_SetPlayAddress(0);
	SONGINFO_SetChannel(1);
	if (GetDwordLE(pData + 0x28))
	{
		Uint8 *src = pData + GetDwordLE(pData + 0x28);
		Uint8 *des;
		nsf.banksw = 1;				/* bank sw on */
		nsf.banknum = *src++;
		des = nsf.bankbase = XMALLOC((nsf.banknum << 12) + 8);
		if (!nsf.bankbase) return NESERR_SHORTOFMEMORY;
		XMEMSET(nsf.bankbase, 0, (nsf.banknum << 12));
		while (*src)
		{
			Uint n = 0;
			do
			{
				n <<= 7;
				n |= (*src & 0x7f);
			} while (*src++ & 0x80);
			if (n & 1)
			{
				n >>= 1;
				XMEMCPY(des, src, n);
				src += n;
				des += n;
			}
			else
			{
				n >>= 1;
				des += n;
			}
		}
	}
	else
	{
		nsf.banksw = 0;
		nsf.banknum = 0;
		nsf.bankbase = 0;
	}
	ret = NSDPlayerInstall(pData, uSize);
	if (ret) return ret;
	ret = NSFDeviceInitialize();
	if (ret) return ret;
	SONGINFO_SetSongNo(SONGINFO_GetStartSongNo());
	return NESERR_NOERROR;
}

static Uint NSFPlayerSetInfo(Uint8 *pData, Uint uSize)
{
	Uint ret;
	NESMemoryHandlerInitialize();
	NSFMapperSetInfo(pData);
	ret = NSF6502Install();
	if (ret) return ret;
	ret = NSFMapperInitialize(pData + 0x80, uSize - 0x80);
	if (ret) return ret;
	ret = NSFDeviceInitialize();
	if (ret) return ret;
	SONGINFO_SetSongNo(SONGINFO_GetStartSongNo());
	return NESERR_NOERROR;
}


Uint NSFLoad(Uint8 *pData, Uint uSize)
{
	Uint ret = NESERR_NOERROR;
	while (1)
	{
		NESTerminate();
		NESHandlerInitialize();
		NESAudioHandlerInitialize();
		if (uSize < 8)
		{
			ret = NESERR_FORMAT;
			break;
		}
		else if (GetDwordLE(pData + 0) == GetDwordLEM("KSCC"))
		{
			/* KSS */
			ret =  KSSLoad(pData, uSize);
			if (ret) break;
		}
		else if (GetDwordLE(pData + 0) == GetDwordLEM("KSSX"))
		{
			/* KSS */
			ret =  KSSLoad(pData, uSize);
			if (ret) break;
		}
		else if (GetDwordLE(pData + 0) == GetDwordLEM("HESM"))
		{
			/* HES */
			ret =  HESLoad(pData, uSize);
			if (ret) break;
		}
		else if (uSize > 0x220 && GetDwordLE(pData + 0x200) == GetDwordLEM("HESM"))
		{
			/* HES(+512byte header) */
			ret =  HESLoad(pData + 0x200, uSize - 0x200);
			if (ret) break;
		}
		else if (GetDwordLE(pData + 0) == GetDwordLEM("NESL") && pData[4] == 0x1A)
		{
			/* NSD */
			ret = NSDPlayerSetInfo(pData, uSize);
			if (ret) break;
		}
		else if (GetDwordLE(pData + 0) == GetDwordLEM("NESM") && pData[4] == 0x1A)
		{
			/* NSF */
			ret = NSFPlayerSetInfo(pData, uSize);
			if (ret) break;
		}
		else if (GetDwordLE(pData + 0) == GetDwordLEM("ZXAY") && GetDwordLE(pData + 4) == GetDwordLEM("EMUL"))
		{
			/* ZXAY */
			ret =  ZXAYLoad(pData, uSize);
			if (ret) break;
		}
		else if (GetDwordLE(pData + 0) == GetDwordLEM("GBRF"))
		{
			/* GBR */
			ret =  GBRLoad(pData, uSize);
			if (ret) break;
		}
		else if ((GetDwordLE(pData + 0) & 0x00ffffff) == GetDwordLEM("GBS\x0"))
		{
			/* GBS */
			ret =  GBRLoad(pData, uSize);
			if (ret) break;
		}
		else if (pData[0] == 0xc3 && uSize > GetWordLE(pData + 1) + 4 && GetWordLE(pData + 1) > 0x70 && (GetDwordLE(pData + GetWordLE(pData + 1) - 0x70) & 0x00ffffff) == GetDwordLEM("GBS\x0"))
		{
			/* GB(GBS player) */
			ret =  GBRLoad(pData + GetWordLE(pData + 1) - 0x70, uSize - (GetWordLE(pData + 1) - 0x70));
			if (ret) break;
		}
#if 0
		else if (GetDwordLE(pData + 0) == GetDwordLEM("NES\x1a"))
		{
			/* NES */
			ret = NESERR_FORMAT;
			break;
		}
#endif
		else
		{
			ret = NESERR_FORMAT;
			break;
		}
		return NESERR_NOERROR;
	}
	NESTerminate();
	return ret;
}
