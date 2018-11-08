/*
	WASAPI version of BASS simple synth
	Copyright (c) 2001-2017 Un4seen Developments Ltd.
*/

#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <math.h>
#include "basswasapi.h"
#include "bass.h"

// display error messages
void Error(const char *text) 
{
	printf("Error(%d): %s\n",BASS_ErrorGetCode(),text);
	BASS_WASAPI_Free();
	BASS_Free();
	exit(0);
}


#define PI 3.14159265358979323846
#define TABLESIZE 2048
float sinetable[TABLESIZE];	// sine table
#define KEYS 20
const WORD keys[KEYS]={
	'Q','2','W','3','E','R','5','T','6','Y','7','U',
	'I','9','O','0','P',219,187,221
};
#define MAXVOL	4000	// higher value = longer fadeout
int vol[KEYS]={0},pos[KEYS];	// keys' volume & pos
float samrate;
HSTREAM chan;

// WASAPI function
DWORD CALLBACK WasapiProc(void *buffer, DWORD length, void *user)
{
	return BASS_ChannelGetData(chan,buffer,length);
}

// STREAMPROC function
DWORD CALLBACK SynthStreamProc(HSTREAM handle, void *buffer, DWORD length, void *user)
{
	float *b=(float*)buffer;
	int n,c;
	float f,s;
	memset(b,0,length);
	for (n=0;n<KEYS;n++) {
		if (!vol[n]) continue;
		f=pow(2.0,(n+3)/12.0)*TABLESIZE*440.0/samrate;
		for (c=0;c<length/4/2 && vol[n];c++) {
			s=sinetable[(int)((pos[n]++)*f)&(TABLESIZE-1)]*vol[n]/MAXVOL;
			s+=b[c*2];
			b[c*2+1]=b[c*2]=s; // left and right channels are the same
			if (vol[n]<MAXVOL) vol[n]--;
		}
	}
	return length;
}

void main(int argc, char **argv)
{
	BASS_WASAPI_INFO info;
	const char *fxname[9]={"CHORUS","COMPRESSOR","DISTORTION","ECHO",
		"FLANGER","GARGLE","I3DL2REVERB","PARAMEQ","REVERB"};
	HFX fx[9]={0}; // effect handles
	INPUT_RECORD keyin;
	DWORD r;
	DWORD flags=BASS_WASAPI_EVENT; // default initialization flags
	float buflen;

	printf("BASS+WASAPI Simple Sinewave Synth\n"
			"---------------------------------\n");

	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion())!=BASSVERSION) {
		printf("An incorrect version of BASS was loaded");
		return;
	}

	// initialize default WASAPI device in shared mode with minimal buffer
	if (!BASS_WASAPI_Init(-1,0,0,flags,0,0,WasapiProc,NULL))
		Error("Can't initialize device");

	// get the output details
	BASS_WASAPI_GetInfo(&info);
	if (info.chans!=2) Error("Need stereo ouput (see Sound control panel)");
	buflen=1000.0*info.buflen/(info.freq*info.chans*sizeof(float));
	samrate=info.freq;

	// not playing anything via BASS, so don't need an update thread
	BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD,0);
	// setup BASS - "no sound" device
	BASS_Init(0,48000,0,0,NULL);

	// build sine table
	for (r=0;r<TABLESIZE;r++)
		sinetable[r]=sin(2.0*PI*(double)r/TABLESIZE)*0.2;

	// create a stream, stereo so that effects sound nice
	chan=BASS_StreamCreate(samrate,2,BASS_STREAM_DECODE|BASS_SAMPLE_FLOAT,SynthStreamProc,0);

	// start the WASAPI device
	if (!BASS_WASAPI_Start())
		Error("Can't start output");

	printf("press these keys to play:\n\n"
		"  2 3  5 6 7  9 0  =\n"
		" Q W ER T Y UI O P[ ]\n\n"
		"press -/+ to de/increase the buffer\n"
		"press X to toggle exclusive mode\n"
		"press F1-F9 to toggle effects\n"
		"press spacebar to quit\n\n");
	printf("using a %.1fms buffer (%s mode)\r",
		buflen,flags&BASS_WASAPI_EXCLUSIVE?"exclusive":"shared");

	while (ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE),&keyin,1,&r)) {
		int key;
		if (keyin.EventType!=KEY_EVENT) continue;
		if (keyin.Event.KeyEvent.wVirtualKeyCode==VK_SPACE) break;
		if (keyin.Event.KeyEvent.bKeyDown) {
			if (keyin.Event.KeyEvent.wVirtualKeyCode>=VK_F1 && keyin.Event.KeyEvent.wVirtualKeyCode<=VK_F9) {
				r=keyin.Event.KeyEvent.wVirtualKeyCode-VK_F1;
				if (fx[r]) {
					BASS_ChannelRemoveFX(chan,fx[r]);
					fx[r]=0;
					printf("effect %s = OFF\t\t\t\r",fxname[r]);
				} else {
					// set the effect, not bothering with parameters (use defaults)
					if (fx[r]=BASS_ChannelSetFX(chan,BASS_FX_DX8_CHORUS+r,0))
						printf("effect %s = ON\t\t\t\r",fxname[r]);
				}
			}
			if (keyin.Event.KeyEvent.wVirtualKeyCode==VK_SUBTRACT || keyin.Event.KeyEvent.wVirtualKeyCode==VK_ADD || keyin.Event.KeyEvent.wVirtualKeyCode=='X') {
				// reinitialize with smaller/larger buffer or exclusive/shared mode
				float oldbuf=buflen;
				int tries=0;
				BASS_WASAPI_Free();
				buflen=(int)buflen;
				if (keyin.Event.KeyEvent.wVirtualKeyCode==VK_SUBTRACT)
					buflen--; // smaller buffer
				else if (keyin.Event.KeyEvent.wVirtualKeyCode==VK_ADD)
					buflen++; // larger buffer
				else
					flags^=BASS_WASAPI_EXCLUSIVE; // switch exclusive/shared mode
				for (;;) {
					// reinitialize default WASAPI device (event-driven exclusive mode = 2 buffers)
					if (!BASS_WASAPI_Init(-1,samrate,2,flags,buflen/1000/(flags&BASS_WASAPI_EXCLUSIVE?2:1),0,WasapiProc,NULL))
						Error("Can't initialize device");
					BASS_WASAPI_GetInfo(&info);
					buflen=1000.0*info.buflen/(info.freq*info.chans*sizeof(float))*(flags&BASS_WASAPI_EXCLUSIVE?2:1);
					if (keyin.Event.KeyEvent.wVirtualKeyCode==VK_SUBTRACT && buflen==oldbuf && buflen-tries>2 && (flags&BASS_WASAPI_EXCLUSIVE)) {
						// got the same buffer, try reducing it more in case the device's buffer granularity is higher than 1ms
						tries++;
						buflen=(int)oldbuf-1-tries;
						BASS_WASAPI_Free();
						continue;
					}
					break;
				}
				if (!BASS_WASAPI_Start())
					Error("Can't start output");
				printf("using a %.1fms buffer (%s mode)\t\r",buflen,flags&BASS_WASAPI_EXCLUSIVE?"exclusive":"shared");
			}
		}
		for (key=0;key<KEYS;key++)
			if (keyin.Event.KeyEvent.wVirtualKeyCode==keys[key]) break;
		if (key==KEYS) continue;
		if (keyin.Event.KeyEvent.bKeyDown && vol[key]!=MAXVOL) {
			pos[key]=0;
			vol[key]=MAXVOL; // start key
		} else if (!keyin.Event.KeyEvent.bKeyDown && vol[key])
			vol[key]--; // trigger key fadeout
	}

	BASS_WASAPI_Free();
	BASS_Free();
}
