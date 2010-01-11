/*
	NSF SDK API - Win32
	Author:   Mamiya (mamiya@proc.org.tohoku.ac.jp)
	Language: ANSI-C
	License:  PDS
	Required DirectX3 or later vresion
*/

/* #define NSFSDKAPI __declspec(dllexport) */

#include "nsfsdk.h"
#include "common/cso.h"
#include "nes/handler.h"
#include "nes/audiosys.h"
#include "nes/songinfo.h"
#include "nes/m_nsf.h"
#include "nes/nsdout.h"

static struct NSFSDK_TAG
{
	unsigned volume;
} d = { 0 };

static CRITICAL_SECTION_OBJECT(nsflock);

HNSF NSFSDKAPI NSFSDK_Load(void *pData, unsigned uSize)
{
	if (CRITICAL_SECTION_ENTER(nsflock))
	{
		extern void LoadVRC7Tone(void);
		LoadVRC7Tone();
		d.volume = 0;
		if (!NSFLoad(pData, uSize)) return &d;
		CRITICAL_SECTION_LEAVE(nsflock);
	}
	return 0;
}

HNSF NSFSDKAPI NSFSDK_StartNSD(void *pData, unsigned uSize, unsigned syncmode)
{
	if (CRITICAL_SECTION_ENTER(nsflock))
	{
		extern void LoadVRC7Tone(void);
		NSDStart(syncmode);
		LoadVRC7Tone();
		d.volume = 0;
		if (!NSFLoad(pData, uSize)) return &d;
		NSDTerm(0);
		CRITICAL_SECTION_LEAVE(nsflock);
	}
	return 0;
}

void NSFSDKAPI NSFSDK_SetSongNo(HNSF hnsf, unsigned uSongNo)
{
	if (hnsf == 0) return;
	SONGINFO_SetSongNo(uSongNo);
}

void NSFSDKAPI NSFSDK_SetFrequency(HNSF hnsf, unsigned freq)
{
	if (hnsf == 0) return;
	NESAudioFrequencySet(freq);
}

void NSFSDKAPI NSFSDK_SetNosefartFilter(HNSF hnsf, unsigned filter)
{
	if (hnsf == 0) return;
	NESAudioFilterSet(filter);
}


void NSFSDKAPI NSFSDK_SetChannel(HNSF hnsf, unsigned ch)
{
	if (hnsf == 0) return;
	NESAudioChannelSet(ch);
}

void NSFSDKAPI NSFSDK_Reset(HNSF hnsf)
{
	if (hnsf == 0) return;
	NESReset();
	NESVolume(hnsf->volume);
}

void NSFSDKAPI NSFSDK_Volume(HNSF hnsf, unsigned uVolume)
{
	if (hnsf == 0) return;
	hnsf->volume = uVolume;
	NESVolume(hnsf->volume);
}

void NSFSDKAPI NSFSDK_Render(HNSF hnsf, void *bufp, unsigned buflen)
{
	if (hnsf == 0) return;
	NESAudioRender(bufp, buflen);
}

void NSFSDKAPI NSFSDK_Terminate(HNSF hnsf)
{
	if (hnsf == 0) return;
	NESTerminate();
	CRITICAL_SECTION_LEAVE(nsflock);
}

unsigned NSFSDKAPI NSFSDK_GetSongNo(HNSF hnsf)
{
	if (hnsf == 0) return 0;
	return SONGINFO_GetSongNo();
}

unsigned NSFSDKAPI NSFSDK_GetSongStart(HNSF hnsf)
{
	if (hnsf == 0) return 0;
	return SONGINFO_GetStartSongNo();
}

unsigned NSFSDKAPI NSFSDK_GetSongMax(HNSF hnsf)
{
	unsigned ret;
	if (hnsf == 0) return 0;
	ret = SONGINFO_GetMaxSongNo();
	if (!ret) return 256;
	return ret;
}

unsigned NSFSDKAPI NSFSDK_GetChannel(HNSF hnsf)
{
	if (hnsf == 0) return 1;
	return SONGINFO_GetChannel();
}

void NSFSDKAPI NSFSDK_OutputNSD(HNSF hnsf, void (*fnCallBack)(void *p, unsigned l))
{
	if (hnsf == 0) return;
	NSDTerm(fnCallBack);
}
