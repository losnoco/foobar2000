#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "aosdk/ao.h"
#include "aosdk/eng_protos.h"

#include "xsfc/xsfdrv.h"

#if _MSC_VER >= 1200
#pragma comment(linker, "/EXPORT:XSFSetup=_XSFSetup@8")
#endif

#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/MERGE:.rdata=.text")
#endif



static void * PASCAL XSFLibAlloc(DWORD dwSize)
{
	return malloc(dwSize);
}
static void PASCAL XSFLibFree(void *lpPtr)
{
	free(lpPtr);
}

static int PASCAL XSFStart(void *lpPtr, DWORD dwSize)
{
	return AO_SUCCESS != qsf_start(lpPtr, dwSize);
}
static void PASCAL XSFGen(void *lpPtr, DWORD dwSamples)
{
	qsf_gen(lpPtr, dwSamples);
}
static void PASCAL XSFTerm(void)
{
	qsf_stop();
}

UINT32 dwChannelMute = 0;

static void PASCAL XSFSetChannelMute(DWORD dwPage, DWORD dwMute)
{
	if (dwPage == 0)
		dwChannelMute = dwMute;
}

static void *lpUserWrok = 0;
static LPFNGETLIB_XSFDRV lpfnAOGetLib = 0;
static IXSFDRV ifaoxsf =
{
	XSFLibAlloc,
	XSFLibFree,
	XSFStart,
	XSFGen,
	XSFTerm,
	2,
	XSFSetChannelMute
};
IXSFDRV * PASCAL XSFSetup(LPFNGETLIB_XSFDRV lpfn, void *lpWork)
{
	lpfnAOGetLib = lpfn;
	lpUserWrok = lpWork;
	return &ifaoxsf;
}

int ao_get_lib(char *filename, uint8 **pbuffer, uint64 *plength)
{
	DWORD length32;
	if (!lpfnAOGetLib || lpfnAOGetLib(lpUserWrok, filename, pbuffer, &length32)) return AO_FAIL;
	if (plength) *plength = length32;
	return AO_SUCCESS;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	return TRUE;
}


