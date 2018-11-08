/*
	WASAPI version of BASS full-duplex test
	Copyright (c) 2002-2017 Un4seen Developments Ltd.
*/

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <math.h>
#include "basswasapi.h"
#include "bassmix.h"

HWND win=NULL;

#define MESS(id,m,w,l) SendDlgItemMessage(win,id,m,(WPARAM)(w),(LPARAM)(l))

int indev=-1;			// input device
int outdev=-1;			// output device

HSTREAM rchan;	// input push stream
HSTREAM chan;	// output mixer
HFX fx[4]={0};	// FX handles
float latency=0;	// current latency

void Error(const char *es)
{
	char mes[200];
	sprintf(mes,"%s\n(error code: %d)",es,BASS_ErrorGetCode());
	MessageBox(win,mes,0,0);
}

DWORD CALLBACK InWasapiProc(void *buffer, DWORD length, void *user)
{
	BASS_StreamPutData(rchan,buffer,length); // feed input data to output (via push stream)
	return 1; // continue input
}

DWORD CALLBACK OutWasapiProc(void *buffer, DWORD length, void *user)
{
	return BASS_ChannelGetData(chan,buffer,length); // get data from the mixer
}

static BOOL Initialize()
{
	// not playing anything via BASS, so don't need an update thread
	BASS_SetConfig(BASS_CONFIG_UPDATETHREADS,0);
	// initialze BASS "no sound" device
	BASS_Init(0,48000,0,0,NULL);
	// initialize default WASAPI input device in event-driven shared mode with minimal buffer
	if (!BASS_WASAPI_Init(-2,0,0,BASS_WASAPI_EVENT,0,0,InWasapiProc,NULL)) {
		Error("Can't initialize input");
		return FALSE;
	}
	indev=BASS_WASAPI_GetDevice();
	// initialize default WASAPI output device in event-driven shared mode with minimal buffer
	if (!BASS_WASAPI_Init(-1,0,0,BASS_WASAPI_EVENT,0,0,OutWasapiProc,NULL)) {
		Error("Can't initialize output");
		return FALSE;
	}
	outdev=BASS_WASAPI_GetDevice();
	{ // create streams to receive input data and output it
		BASS_WASAPI_INFO wi;
		BASS_WASAPI_SetDevice(indev);
		BASS_WASAPI_GetInfo(&wi); // get input format
		rchan=BASS_StreamCreate(wi.freq,wi.chans,BASS_STREAM_DECODE|BASS_SAMPLE_FLOAT,STREAMPROC_PUSH,0); // create push stream to hold input data
		BASS_WASAPI_SetDevice(outdev);
		BASS_WASAPI_GetInfo(&wi); // get output format
		chan=BASS_Mixer_StreamCreate(wi.freq,wi.chans,BASS_STREAM_DECODE|BASS_SAMPLE_FLOAT); // create mixer to feed output
		BASS_Mixer_StreamAddChannel(chan,rchan,0); // plug the push stream into it
		BASS_ChannelSetAttribute(rchan,BASS_ATTRIB_VOL,0); // start muted
	}
	// start the devices
	BASS_WASAPI_SetDevice(indev);
	BASS_WASAPI_Start();
	BASS_WASAPI_SetDevice(outdev);
	BASS_WASAPI_Start();
	return TRUE;
}

BOOL CALLBACK dialogproc(HWND h,UINT m,WPARAM w,LPARAM l)
{
	switch (m) {
		case WM_TIMER:
			{ // display current latency (input+output buffer level)
				char buf[20];
				BASS_WASAPI_SetDevice(outdev);
				latency=(latency*3
					+BASS_ChannelBytes2Seconds(rchan,BASS_StreamPutData(rchan,NULL,0)) // input buffer
					+BASS_ChannelBytes2Seconds(chan,BASS_WASAPI_GetData(NULL,BASS_DATA_AVAILABLE) // output buffer
					))/4;
				sprintf(buf,"%d",(int)(latency*1000));
				MESS(15,WM_SETTEXT,0,buf);
			}
			break;

		case WM_COMMAND:
			switch (LOWORD(w)) {
				case IDCANCEL:
					DestroyWindow(h);
					break;
				case 20: // toggle chorus
					if (fx[0]) {
						BASS_ChannelRemoveFX(chan,fx[0]);
						fx[0]=0;
					} else
						fx[0]=BASS_ChannelSetFX(chan,BASS_FX_DX8_CHORUS,0);
					break;
				case 21: // toggle gargle
					if (fx[1]) {
						BASS_ChannelRemoveFX(chan,fx[1]);
						fx[1]=0;
					} else
						fx[1]=BASS_ChannelSetFX(chan,BASS_FX_DX8_GARGLE,0);
					break;
				case 22: // toggle reverb
					if (fx[2]) {
						BASS_ChannelRemoveFX(chan,fx[2]);
						fx[2]=0;
					} else
						fx[2]=BASS_ChannelSetFX(chan,BASS_FX_DX8_REVERB,0);
					break;
				case 23: // toggle flanger
					if (fx[3]) {
						BASS_ChannelRemoveFX(chan,fx[3]);
						fx[3]=0;
					} else
						fx[3]=BASS_ChannelSetFX(chan,BASS_FX_DX8_FLANGER,0);
					break;
			}
			break;

		case WM_HSCROLL:
			if (l) { // set input level
				float level=SendMessage((HWND)l,TBM_GETPOS,0,0)/100.f;
				BASS_ChannelSetAttribute(rchan,BASS_ATTRIB_VOL,level);
			}
			break;

		case WM_INITDIALOG:
			win=h;
			MESS(11,TBM_SETRANGE,FALSE,MAKELONG(0,100)); // initialize input level slider
			if (!Initialize()) {
				DestroyWindow(win);
				break;
			}
			SetTimer(h,1,250,NULL);
			return 1;

		case WM_DESTROY:
			KillTimer(h,1);
			// release both WASAPI devices and the BASS stuff
			BASS_WASAPI_Free();
			BASS_WASAPI_Free();
			BASS_Free();
			break;
	}
	return 0;
}

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,LPSTR lpCmdLine, int nCmdShow)
{
	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion())!=BASSVERSION) {
		MessageBox(0,"An incorrect version of BASS.DLL was loaded",0,MB_ICONERROR);
		return 0;
	}

	{ // enable trackbar support (for the level control)
		INITCOMMONCONTROLSEX cc={sizeof(cc),ICC_BAR_CLASSES};
		InitCommonControlsEx(&cc);
	}

	DialogBox(hInstance,(char*)1000,0,&dialogproc);

	return 0;
}
