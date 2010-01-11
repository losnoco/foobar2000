/*
	SOUND DRIVER - Win32 - DirectSOUND
	Author:   Mamiya (mamiya@proc.org.tohoku.ac.jp)
	Language: ANSI-C + WIN32SDK + DXSDK
	License:  PDS
	Required DirectX3 or later vresion
*/

/* WIN32 standard headers */
#include "common/win32/win32l.h"
#undef NOGDI
#undef NOUSER
#undef NOMSG
#include <windows.h>
#undef MMNOWAVE
#include <mmsystem.h>
#define CINTERFACE
#include <dsound.h>
#ifdef _MSC_VER
#pragma comment(linker, "/DEFAULTLIB:dxguid.lib")
#endif
/* ANSI standard headers */
#include <stdlib.h>
/* Libraries headers */
/* Project headers */
#include "snddrv/snddrv.h"

typedef struct {
	SOUNDDEVICE sd;
	HMODULE hdll;
	LPDIRECTSOUND lpds;
	LPDIRECTSOUNDBUFFER lpdsb;
	LPDIRECTSOUNDNOTIFY lpdsn;
	HANDLE hevent;
	HANDLE hthread;
	DWORD tid;
	BOOL killthread;
	DWORD nextpos;
	DWORD maxpos;
} SOUNDDEVICE_DS;

#define BLOCKBIT	12
#define BLOCKLEN	(1<<BLOCKBIT)
#define BLOCKNUM	8

#undef SD
#define SD ((SOUNDDEVICE_DS *)(psd->lpSystemData))
static void Term(struct SOUNDDEVICE_TAG *psd)
{
	if (SD->hthread)
	{
		SD->killthread = TRUE;
		if (SetEvent(SD->hevent)) WaitForSingleObject(SD->hthread, INFINITE);
		CloseHandle(SD->hthread);
	}
	if (SD->hevent) CloseHandle(SD->hevent);
	if (SD->lpdsn) IDirectSoundNotify_Release(SD->lpdsn);
	if (SD->lpdsb) IDirectSoundBuffer_Release(SD->lpdsb);
	if (SD->lpds) IDirectSound_Release(SD->lpds);
	if (SD->hdll) FreeLibrary(SD->hdll);
	SD->sd.sdid.Term(SD->sd.sdid.lpargs);
	free(SD);
}
static unsigned IsPause(struct SOUNDDEVICE_TAG *psd)
{
	return 0;
}
static void Pause(struct SOUNDDEVICE_TAG *psd, unsigned isPause)
{
}

#undef SD
#define SD psdds
static char mixbuf[BLOCKLEN + 8];
static void Write(SOUNDDEVICE_DS *psdds)
{
	HRESULT hr;
	DWORD st;
	DWORD cp, pp, wp;
	LPBYTE buf1, buf2;
	DWORD len1, len2, out1, out2;

	hr = IDirectSoundBuffer_GetStatus(SD->lpdsb, &st);
	if (SUCCEEDED(hr) && (st & DSBSTATUS_BUFFERLOST))
	{
		hr = IDirectSoundBuffer_Restore(SD->lpdsb);
		if (FAILED(hr)) return;
	}

	hr = IDirectSoundBuffer_GetCurrentPosition(SD->lpdsb, &pp, &wp);
	if (FAILED(hr)) return;

	cp = pp & ~(BLOCKLEN-1);

	while (SD->nextpos != cp)
	{
		SD->sd.sdid.Write(SD->sd.sdid.lpargs, mixbuf, BLOCKLEN);
		hr = IDirectSoundBuffer_Lock(SD->lpdsb, SD->nextpos, BLOCKLEN, &buf1, &len1, &buf2, &len2, 0);
		if (FAILED(hr)) return;
		if (buf1)
		{
			out1 = (len1 > BLOCKLEN) ? BLOCKLEN : len1;
			memcpy(buf1, mixbuf, out1);
		}
		else
		{
			out1 = 0;
		}
		if (buf2 && BLOCKLEN > out1)
		{
			out2 = BLOCKLEN - out1;
			if (out2 > len2) out2 = len2;
			memcpy(buf2, mixbuf + out1, out2);
		}
		else
		{
			out2 = 0;
		}
		IDirectSoundBuffer_Unlock(SD->lpdsb, buf1, out1, buf2, out2);
		SD->nextpos += BLOCKLEN;
		if (SD->nextpos >= SD->maxpos)
			SD->nextpos -= SD->maxpos;
	}
}

#undef SD
#define SD ((SOUNDDEVICE_DS *)(pv))
static DWORD WINAPI ThreadProc(LPVOID pv)
{
	while (!SD->killthread)
	{
		Write(SD);
		WaitForSingleObject(SD->hevent, INFINITE);
	}
	return 0;
}

#undef SD
#define SD psdds
typedef HRESULT (WINAPI *LPFNDSC)(LPGUID, LPDIRECTSOUND *, LPUNKNOWN);
SOUNDDEVICE *CreateSoundDeviceDX3(SOUNDDEVICEINITDATA *psdid)
{
	DSBPOSITIONNOTIFY dsbpn[BLOCKNUM];
	SOUNDDEVICE_DS *psdds = NULL;
	do
	{
		int i;
		PCMWAVEFORMAT pwf; 
		DSBUFFERDESC dsbd; 
		HRESULT hr;
		LPFNDSC lpfnDirectSoundCreate;

		memset(mixbuf, 0, sizeof(mixbuf));

		SD = (SOUNDDEVICE_DS *)malloc(sizeof(SOUNDDEVICE_DS));
		if (SD == NULL) break;
		SD->sd.lpSystemData = SD;
		SD->sd.sdid = *psdid;
		SD->sd.Term = Term;
		SD->sd.IsPause = IsPause;
		SD->sd.Pause = Pause;
		SD->hdll = NULL;
		SD->lpds = NULL;
		SD->lpdsb = NULL;
		SD->lpdsn = NULL;
		SD->hevent = NULL;
		SD->hthread = NULL;
		SD->killthread = FALSE;
		SD->nextpos = BLOCKLEN;
		SD->maxpos = BLOCKLEN * BLOCKNUM;

		SD->hdll = LoadLibrary("dsound.dll");
		if (SD->hdll == NULL) break;
		lpfnDirectSoundCreate = (LPFNDSC)GetProcAddress(SD->hdll, "DirectSoundCreate");
		if (lpfnDirectSoundCreate == NULL) break;
		hr = lpfnDirectSoundCreate(NULL, &SD->lpds, NULL);
		if (FAILED(hr)) break;
		hr = IDirectSound_SetCooperativeLevel(SD->lpds, SD->sd.sdid.ppdi->hwnd, DSSCL_NORMAL);

		memset(&pwf, 0, sizeof(PCMWAVEFORMAT));
		pwf.wf.wFormatTag = WAVE_FORMAT_PCM;
		pwf.wf.nSamplesPerSec = SD->sd.sdid.freq;
		pwf.wf.nChannels = SD->sd.sdid.ch;
		pwf.wBitsPerSample = SD->sd.sdid.bit;
		pwf.wf.nBlockAlign = pwf.wf.nChannels * ((pwf.wBitsPerSample + 7) >> 3);
		pwf.wf.nAvgBytesPerSec = pwf.wf.nSamplesPerSec * pwf.wf.nBlockAlign;

		memset(&dsbd, 0, sizeof(DSBUFFERDESC));
		dsbd.dwSize = sizeof(DSBUFFERDESC);
		dsbd.dwFlags = DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME |
				DSBCAPS_STICKYFOCUS | DSBCAPS_GETCURRENTPOSITION2 |
				DSBCAPS_CTRLPOSITIONNOTIFY;
		dsbd.dwBufferBytes = SD->maxpos;
		dsbd.lpwfxFormat = (LPWAVEFORMATEX)&pwf; 

		hr = IDirectSound_CreateSoundBuffer(SD->lpds, &dsbd, &SD->lpdsb, NULL);
		if (FAILED(hr)) break;

		hr = IDirectSoundBuffer_QueryInterface(SD->lpdsb, &IID_IDirectSoundNotify, &SD->lpdsn);
		if (FAILED(hr)) break;

		SD->hevent = CreateEvent(0, FALSE, FALSE, 0);
		if (SD->hevent == NULL) break;

		for (i = 0; i < BLOCKNUM; i++)
		{
			dsbpn[i].dwOffset = BLOCKLEN * i;
			dsbpn[i].hEventNotify = SD->hevent;
		}
		hr = IDirectSoundNotify_SetNotificationPositions(SD->lpdsn, BLOCKNUM, dsbpn);
		if (FAILED(hr)) break;

		SD->hthread = CreateThread(NULL, 0, ThreadProc, SD, 0, &SD->tid);
		if (SD->hthread == NULL) break;

		hr = IDirectSoundBuffer_Play(SD->lpdsb, 0, 0, DSBPLAY_LOOPING);
		if (FAILED(hr)) break;
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
