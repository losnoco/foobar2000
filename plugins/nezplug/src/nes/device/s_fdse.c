#include "common/win32/win32l.h"
#include <windows.h>

#include "../nestypes.h"
#include "../audiosys.h"
#include "../handler.h"
#include "../nsf6502.h"
#include "../nsdout.h"
#include "logtable.h"
#include "s_fds.h"
#include "fdsplugin/FDSplugin.h"

#define NES_BASECYCLES (21477270)
#define VOL_SHIFT 15
#define FDS_SYNC 0

typedef struct FDSSOUND {
	HMODULE hDll;
	HFDS hfds;
	FDSCREATE create;
	FDSCLOSE close;
	FDSRESET reset;
	FDSPROCESS process;
#if FDS_SYNC
	FDSSYNC sync;
	FDSWRITESYNC writesync;
	FDSREADSYNC readsync;
	Uint32 cycles;
#else
	FDSWRITE write;
	FDSREAD read;
#endif
	Uint32 mastervolume;
	Int32 mastervolumel;
} FDSSOUND;


static FDSSOUND fdssound;

static Int32 __fastcall FDSSoundRender(void)
{
	Int32 ret;
	ret = fdssound.process(fdssound.hfds);
#if FDS_SYNC
	{
		Uint32 curcycles, addcycles;
		curcycles = NES6502GetCycles();
		addcycles = (curcycles > fdssound.cycles) ? (curcycles - fdssound.cycles) : 0;
		fdssound.sync(fdssound.hfds, addcycles * 256);
		fdssound.cycles = curcycles;
	}
#endif
	return (fdssound.mastervolumel * ret) >> (VOL_SHIFT - 8);
}

static NES_AUDIO_HANDLER s_fds_audio_handler[] =
{
	{ 1, FDSSoundRender, }, 
	{ 0, 0, }, 
};

static void __fastcall FDSSoundWrite(Uint address, Uint value)
{
	if (0x4040 <= address && address <= 0x40FF)
	{
#if FDS_SYNC
		Uint32 curcycles, addcycles;
		curcycles = NES6502GetCycles();
		addcycles = (curcycles > fdssound.cycles) ? (curcycles - fdssound.cycles) : 0;
		fdssound.sync(fdssound.hfds, addcycles * 256);
		fdssound.cycles = curcycles;
		fdssound.writesync(fdssound.hfds, (WORD)address, (BYTE)value);
#else
		fdssound.write(fdssound.hfds, (WORD)address, (BYTE)value);
#endif
	}
}

static NES_WRITE_HANDLER s_fds_write_handler[] =
{
	{ 0x4040, 0x40FF, FDSSoundWrite, },
	{ 0,      0,      0, },
};

static Uint __fastcall FDSSoundRead(Uint address)
{
	if (0x4040 <= address && address <= 0x40FF)
	{
#if FDS_SYNC
		Uint32 curcycles, addcycles;
		curcycles = NES6502GetCycles();
		addcycles = (curcycles > fdssound.cycles) ? (curcycles - fdssound.cycles) : 0;
		fdssound.sync(fdssound.hfds, addcycles * 256);
		fdssound.cycles = curcycles;
		return fdssound.readsync(fdssound.hfds, (WORD)address);
#else
		return fdssound.read(fdssound.hfds, (WORD)address);
#endif
	}
	return 0;
}

static NES_READ_HANDLER s_fds_read_handler[] =
{
	{ 0x4040, 0x40FF, FDSSoundRead, },
	{ 0,      0,      0, },
};

static void __fastcall FDSSoundReset(void)
{
	Uint32 address;
#if FDS_SYNC
	fdssound.cycles = ~0;
#endif
	fdssound.reset(fdssound.hfds, NESAudioFrequencyGet());
	for (address = 0x4040; address <= 0x407f; address++)
		fdssound.write(fdssound.hfds, (WORD)address, (BYTE)((address < 0x4060) ? 0x3f : 0x00));
}

static NES_RESET_HANDLER s_fds_reset_handler[] =
{
	{ NES_RESET_SYS_NOMAL, FDSSoundReset, }, 
	{ 0,                   0, }, 
};

static void __fastcall FDSSoundTerm(void)
{
	if (fdssound.hfds)
	{
		fdssound.close(fdssound.hfds);
		fdssound.hfds = 0;
	}
	if (fdssound.hDll)
	{
		FreeLibrary(fdssound.hDll);
		fdssound.hDll = 0;
	}
}

static NES_TERMINATE_HANDLER s_fds_terminate_handler[] = {
	{ FDSSoundTerm, }, 
	{ 0, }, 
};

static void __fastcall FDSSoundVolume(Uint volume)
{
	fdssound.mastervolume = (volume << (LOG_BITS - 8)) << 1;
	fdssound.mastervolumel = LogToLinear(fdssound.mastervolume, (LOG_LIN_BITS - VOL_SHIFT));
}

static NES_VOLUME_HANDLER s_fds_volume_handler[] = {
	{ FDSSoundVolume, }, 
	{ 0, }, 
};

extern char *GetDLLArgv0(void);
int FDSSoundInstallExt(void)
{
	char dllpath[MAX_PATH];
	LPTSTR fnoff;

	LogTableInitialize();
	fdssound.hDll = 0;
	fdssound.hfds = 0;

	GetFullPathName(GetDLLArgv0(), MAX_PATH, dllpath, &fnoff);
	lstrcpy(fnoff, "FDSplugin.dll");
	fdssound.hDll = LoadLibrary(dllpath);
	if (!fdssound.hDll)
	{
		FDSSoundTerm();
		return 0;
	}

#define RESOLVE(symbol, type, method) \
{ \
	fdssound.method = (type)GetProcAddress(fdssound.hDll, symbol); \
	if (!fdssound.method) \
	{ \
		FDSSoundTerm(); \
		return 0; \
	} \
}

	RESOLVE("FDS_Create",FDSCREATE,create);
	RESOLVE("FDS_Close",FDSCLOSE,close);
	RESOLVE("FDS_Reset",FDSRESET,reset);
	RESOLVE("FDS_Process",FDSPROCESS,process);
#if FDS_SYNC
	RESOLVE("FDS_Sync",FDSSYNC,sync);
	RESOLVE("FDS_WriteSync",FDSWRITESYNC,writesync);
	RESOLVE("FDS_ReadSync",FDSREADSYNC,readsync);
#else
	RESOLVE("FDS_Write",FDSWRITE,write);
	RESOLVE("FDS_Read",FDSREAD,read);
#endif

	fdssound.hfds = fdssound.create();
	if (!fdssound.hfds)
	{
		FDSSoundTerm();
		return 0;
	}

	NESAudioHandlerInstall(s_fds_audio_handler);
	NESReadHandlerInstall(s_fds_read_handler);
	NESWriteHandlerInstall(s_fds_write_handler);
	NESResetHandlerInstall(s_fds_reset_handler);
	NESTerminateHandlerInstall(s_fds_terminate_handler);
	NESVolumeHandlerInstall(s_fds_volume_handler);
	return 1;
}
