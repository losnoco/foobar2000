#include "common/win32/win32l.h"
#include <windows.h>
#include "kmp_pi.h"
#include "kmp_com.h"

int isReentrant = 0;
static LONG mutex = 1;

static HKMP WINAPI kmp_OpenSub(const void *p, unsigned size, SOUNDINFO *pInfo)
{
	KMP_CTX *ctx = 0;
	LONG signal = 1;
	if (!isReentrant) signal = InterlockedExchange(&mutex,0);
	while (signal)
	{
		if (!pInfo) break;
		ctx = Open(p, size, pInfo->dwSamplesPerSec, pInfo->dwChannels);
		if (!ctx) break;
		pInfo->dwSamplesPerSec = GetFrequency(ctx);
		pInfo->dwChannels = GetChannels(ctx);
		pInfo->dwBitsPerSample = GetSamplebits(ctx);
		pInfo->dwSeekable = GetSeekableFlag(ctx);
		pInfo->dwUnitRender = GetUnitSamples(ctx) * pInfo->dwChannels * (pInfo->dwBitsPerSample >> 3);
		pInfo->dwReserved1 = GetLoopFlag(ctx);
		pInfo->dwReserved2 = GetMultisongFlag(ctx);
		pInfo->dwLength = GetLength(ctx);
		return ctx;
	}
	if (ctx) Close(ctx);
	if (!isReentrant && signal) InterlockedExchange(&mutex, signal);
	return 0;
}
static HKMP WINAPI kmp_Open(const char *cszFileName, SOUNDINFO *pInfo)
{
	return kmp_OpenSub(cszFileName, 0, pInfo);
}

static HKMP WINAPI kmp_OpenFromBuffer(const BYTE *pBuffer, DWORD dwSize, SOUNDINFO *pInfo)
{
	if (!dwSize || !pBuffer) return 0;
	return kmp_OpenSub(pBuffer, dwSize, pInfo);
}

static void WINAPI kmp_Close(HKMP hKMP)
{
	KMP_CTX *ctx = hKMP;
	if(!ctx) return;
	if (ctx)
	{
		Close(ctx);
		if (!isReentrant) InterlockedExchange(&mutex, 1);
	}
}

static DWORD WINAPI kmp_Render(HKMP hKMP, BYTE* Buffer, DWORD dwSize)
{
	KMP_CTX *ctx = hKMP;
	if(!ctx) return 0;
	return Buffer ? Write(ctx, Buffer, dwSize) : WriteSkip(ctx, dwSize);;
}

static DWORD WINAPI kmp_SetPosition(HKMP hKMP, DWORD dwPos)
{
	KMP_CTX *ctx = hKMP;
	if(!ctx) return 0;
	return SetPosition(ctx, dwPos);
}

void  WINAPI kmp_Init(void)
{
	Init();
}

void  WINAPI kmp_Deinit(void)
{
	Deinit();
}

KMPMODULE* WINAPI KMP_GETMODULE(void)
{
	static KMPMODULE Mod = {
		KMPMODULE_VERSION,
		0,
		0,
		0,
		0,
		0,
		kmp_Init,
		kmp_Deinit,
		kmp_Open,
		kmp_OpenFromBuffer,
		kmp_Close,
		kmp_Render,
		kmp_SetPosition
	};
	const char **ppinfo;
	ppinfo = GetPluginInfo();
	if (!ppinfo) return 0;
	Mod.dwPluginVersion = ppinfo[0][0];
	Mod.dwReentrant = isReentrant = ppinfo[0][1];
	Mod.pszDiscription = ppinfo[1];
	Mod.pszCopyright = ppinfo[2];
	Mod.ppszSupportExts = ppinfo + 3;
	if (!(ppinfo[0][2] & 1)) Mod.OpenFromBuffer = 0;
	if (!(ppinfo[0][2] & 2)) Mod.Init = 0;
	if (!(ppinfo[0][2] & 4)) Mod.Deinit = 0;
	return &Mod;
}
