/* WIN32 standard headers */
#include "common/win32/win32l.h"
#undef NOGDI
#undef NOFONTSIG	/* WIN32SDK bug */
#include <windows.h>
/* ANSI standard headers */
#include <stdlib.h>
/* Libraries headers */
#include "hspsdk/hspdll.h"
/* Project headers */
#include "common/nsfsdk/nsfsdk.h"
#include "common/zlib/nez.h"
#include "snddrv/snddrv.h"
#include "ui/version.h"

static struct HSPXXX_T
{
	int songno;
	HNSF hnsf;
	SOUNDDEVICE *psd;
	SOUNDDEVICEPDI pdi;
} hspxxx = { -1, NULL, NULL };

BOOL WINAPI HSPNEZVersion(int p1, int p2, int p3, int p4)
{
	return -HSP_VERSION;
}

static void HSPNSFWriteProc(void *lpargs, void *lpbuf, unsigned len)
{
	NSFSDK_Render(hspxxx.hnsf, lpbuf, len >> 2);
}

static void HSPNSFTermProc(void *lpargs)
{
}

BOOL WINAPI HSPNSFStop(int p1, int p2, int p3, int p4)
{
	SOUNDDEVICE *psd = hspxxx.psd;
	if (psd)
	{
		hspxxx.psd = NULL;
		psd->Term(psd);
	}
	return 0;
}
BOOL WINAPI HSPNSFPlay(BMSCR *p1, int p2, int p3, int p4)
{
	SOUNDDEVICEINITDATA sdid;
	HSPNSFStop(0,0,0,0);
	if (!hspxxx.hnsf) return -1;
	sdid.freq = 44100;
	sdid.ch = 2;
	sdid.bit = 16;
	sdid.lpargs = 0;
	sdid.Write = HSPNSFWriteProc;
	sdid.Term = HSPNSFTermProc;
	if (p1) hspxxx.pdi.hwnd = p1->hwnd;
	sdid.ppdi = &hspxxx.pdi;
	if (hspxxx.songno > 0) NSFSDK_SetSongNo(hspxxx.hnsf, hspxxx.songno);
	NSFSDK_SetFrequency(hspxxx.hnsf, sdid.freq);
	NSFSDK_SetChannel(hspxxx.hnsf, sdid.ch);
	NSFSDK_Reset(hspxxx.hnsf);
	hspxxx.psd = CreateSoundDevice(&sdid);
	return 0;
}

BOOL WINAPI HSPNSFClose(BMSCR *p1, int p2, int p3, int p4)
{
	HSPNSFStop(0,0,0,0);
	if (hspxxx.hnsf)
	{
		NSFSDK_Terminate(hspxxx.hnsf);
		hspxxx.hnsf = NULL;
	}
	return 0;
}

BOOL WINAPI HSPNSFOpen(BMSCR *p1, char *p2, int p3, void *p4)
{
	DWORD size;
	void *pbuf;
	hspxxx.pdi.hwnd = p1->hwnd;
	HSPNSFClose(0,0,0,0);
	size = NEZ_extract(p2, &pbuf);
	if (size)
	{
		hspxxx.hnsf = NSFSDK_Load(pbuf, size);
		free(pbuf);
		if (hspxxx.hnsf) return 0;
	}
	return -1;
}

BOOL WINAPI HSPNSFOpenMemory(void *p1, int p2, int p3, void *p4)
{
	DWORD size;
	void *pbuf;
	HSPNSFClose(0,0,0,0);
	if (p2)
	{
		size = NEZ_extractMem(p1, p2, &pbuf);
		if (size)
		{
			hspxxx.hnsf = NSFSDK_Load(pbuf, size);
			free(pbuf);
			if (hspxxx.hnsf) return 0;
		}
		else
		{
			hspxxx.hnsf = NSFSDK_Load(p1, p2);
			if (hspxxx.hnsf) return 0;
		}
	}
	return -1;
}

BOOL WINAPI HSPNSFSongNo(int p1, int p2, int p3, int p4)
{
	if (p1) hspxxx.songno = p1;
	return 0-NSFSDK_GetSongNo(hspxxx.hnsf);
}

BOOL WINAPI HSPNSFVolume(int p1, int p2, int p3, int p4)
{
	NSFSDK_Volume(hspxxx.hnsf, p1);
	return 0;
}
BOOL WINAPI HSPNSFClean(BMSCR *p1, int p2, int p3, int p4)
{
	HSPNSFClose(0,0,0,0);
	return 0;
}
