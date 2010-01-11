/*
	SOUND DRIVER - Win32 - MMSYSTEM
	Author:   Mamiya (mamiya@proc.org.tohoku.ac.jp)
	Language: ANSI-C + WIN32SDK
	License:  PDS
*/

/* WIN32 standard headers */
#include "common/win32/win32l.h"
#include <windows.h>
#undef MMNOWAVE
#include <mmsystem.h>
#ifdef _MSC_VER
#pragma comment(linker, "/DEFAULTLIB:winmm.lib")
#endif
/* ANSI standard headers */
#include <stdlib.h>
/* Libraries headers */
/* Project headers */
#include "snddrv/snddrv.h"

typedef struct {
	SOUNDDEVICE sd;
	HWAVEOUT hwo;
	unsigned blocknum;
	unsigned blocklen;
	unsigned isplaying;
	unsigned ispause;
	WAVEHDR *pawh;
} SOUNDDEVICE_WO;

#define BLOCKBIT	12
#define BLOCKLEN	(1<<BLOCKBIT)
#define BLOCKNUM	32

#undef SD
#define SD psdwo
static void Write(SOUNDDEVICE_WO *psdwo)
{
	unsigned i;
	for (i = 0; i < SD->blocknum; i++)
	{
		MMRESULT mmr;
		WAVEHDR *pwh = &SD->pawh[i];
		if (pwh->dwFlags) continue;
		mmr = waveOutPrepareHeader(SD->hwo, pwh, sizeof(WAVEHDR));
		if (mmr != MMSYSERR_NOERROR) continue;
		SD->sd.sdid.Write(SD->sd.sdid.lpargs, pwh->lpData, pwh->dwBufferLength);
		mmr = waveOutWrite(SD->hwo, pwh, sizeof(WAVEHDR));
		if (mmr != MMSYSERR_NOERROR)
		{
			waveOutUnprepareHeader(SD->hwo, pwh, sizeof(WAVEHDR));
			pwh->dwFlags = 0;
		}
	}
}

#undef SD
#define SD ((SOUNDDEVICE_WO *)(psd->lpSystemData))
static void Term(struct SOUNDDEVICE_TAG *psd)
{
	SD->isplaying = 0;
	if (SD->hwo)
	{
		waveOutReset(SD->hwo);
		waveOutClose(SD->hwo);
	}
	if (SD->pawh) free(SD->pawh);
	SD->sd.sdid.Term(SD->sd.sdid.lpargs);
	free(SD);
}
static unsigned IsPause(struct SOUNDDEVICE_TAG *psd)
{
	return SD->ispause & 1;
}
static void Pause(struct SOUNDDEVICE_TAG *psd, unsigned isPause)
{
	switch (isPause)
	{
		case 2 + 4:
			SD->ispause = 2 + 4;		/* lock init */
			waveOutPause(SD->hwo);
			Write(SD);
			waveOutRestart(SD->hwo);
			SD->ispause &= ~(2 + 4);	/* unlock uninit */
			break;
		case 1:
			if (SD->ispause == 0)
			{
				SD->ispause = 1 + 2;	/* lock pause */
				waveOutPause(SD->hwo);
				SD->ispause &= ~2;		/* unlock */
			}
			break;
		case 0:
			if (SD->ispause == 1)
			{
				SD->ispause = 0 + 2;	/* lock unpause */
				Write(SD);
				waveOutRestart(SD->hwo);
				SD->ispause &= ~2;		/* unlock */
			}
			break;
	}
}

#undef SD
#define SD ((SOUNDDEVICE_WO *)(dwInstance))
static void CALLBACK WaveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	if (uMsg == MM_WOM_DONE)
	{
		WAVEHDR *pwh = (WAVEHDR *)dwParam1;
		waveOutUnprepareHeader(SD->hwo, pwh, sizeof(WAVEHDR));
		pwh->dwFlags = 0;
		if (SD->isplaying) Write(SD);
	}
}

#undef SD
#define SD psdwo
SOUNDDEVICE *CreateSoundDeviceMMS(SOUNDDEVICEINITDATA *psdid)
{
	SOUNDDEVICE_WO *psdwo = NULL;
	do
	{
		unsigned i;
		WAVEFORMATEX wfex; 
		SD = (SOUNDDEVICE_WO *)malloc(sizeof(SOUNDDEVICE_WO));
		if (SD == NULL) break;
		SD->sd.lpSystemData = SD;
		SD->sd.sdid = *psdid;
		SD->sd.Term = Term;
		SD->sd.IsPause = IsPause;
		SD->sd.Pause = Pause;
		SD->hwo = NULL;
		SD->isplaying = 0;
		SD->blocknum = BLOCKNUM;
		SD->blocklen = BLOCKLEN;
		SD->pawh = malloc((sizeof(WAVEHDR)+SD->blocklen) * SD->blocknum);
		if (SD->pawh == NULL) break;
		for (i = 0; i < SD->blocknum; i++)
		{
			unsigned char *p;
			unsigned len;
			p = i * SD->blocklen + (unsigned char *)&SD->pawh[SD->blocknum];
			len = SD->blocklen;
			memset(&SD->pawh[i], 0, sizeof(WAVEHDR));
			memset(p, 0, len);
			SD->pawh[i].lpData = p;
			SD->pawh[i].dwBufferLength = len;
		}

		memset(&wfex, 0, sizeof(WAVEFORMATEX));
		wfex.wFormatTag = WAVE_FORMAT_PCM;
		wfex.nSamplesPerSec = SD->sd.sdid.freq;
		wfex.nChannels = SD->sd.sdid.ch;
		wfex.wBitsPerSample = SD->sd.sdid.bit;
		wfex.nBlockAlign = wfex.nChannels * ((wfex.wBitsPerSample + 7) >> 3);
		wfex.nAvgBytesPerSec = wfex.nSamplesPerSec * wfex.nBlockAlign;

		if (waveOutOpen(&SD->hwo, WAVE_MAPPER, &wfex, (DWORD)(LPVOID)WaveOutProc, (DWORD)(LPVOID)SD, CALLBACK_FUNCTION) != MMSYSERR_NOERROR) break;

		SD->isplaying = 1;

		Pause(&SD->sd, 2 + 4);

		return &SD->sd;
	} while (0);
	if (SD)
	{
		Term(&SD->sd);
	}
	else
	{
		psdid->Term(psdid->lpargs);
	}
	return NULL;
}
