/*
	WASAPI version of BASS simple synth
	Copyright (c) 2001-2010 Un4seen Developments Ltd.
*/

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
	DWORD c=BASS_ChannelGetData(chan,buffer,length);
	if (c==-1) c=0; // an error, no data
	return c;
}

// stream writer
DWORD CALLBACK WriteStream(HSTREAM handle, float *buffer, DWORD length, void *user)
{
	int n,c;
	float f,s;
	memset(buffer,0,length);
	for (n=0;n<KEYS;n++) {
		if (!vol[n]) continue;
		f=pow(2.0,(n+3)/12.0)*TABLESIZE*440.0/samrate;
		for (c=0;c<length/4/2 && vol[n];c++) {
			s=sinetable[(int)((pos[n]++)*f)&(TABLESIZE-1)]*vol[n]/MAXVOL;
			s+=buffer[c*2];
			buffer[c*2+1]=buffer[c*2]=s; // left and right channels are the same
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
	const char *formats[5]={"32-bit float","8-bit","16-bit","24-bit","32-bit"};
	HFX fx[9]={0}; // effect handles
	INPUT_RECORD keyin;
	DWORD r;

	printf("BASS+WASAPI Simple Sinewave Synth\n"
			"---------------------------------\n");

	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion())!=BASSVERSION) {
		printf("An incorrect version of BASS was loaded");
		return;
	}

	// initialize default WASAPI device (15ms buffer, 5ms update period, auto-select a format if necessary)
	if (!BASS_WASAPI_Init(-1,48000,2,BASS_WASAPI_AUTOFORMAT|BASS_WASAPI_EXCLUSIVE,0.015,0.005,WasapiProc,NULL)) {
		// failed, try shared mode
		if (!BASS_WASAPI_Init(-1,48000,2,BASS_WASAPI_AUTOFORMAT,0.015,0.005,WasapiProc,NULL))
			Error("Can't initialize device");
	}
	// get the output details
	BASS_WASAPI_GetInfo(&info);
	if (info.chans!=2) Error("Can't initialize stereo ouput");
	printf("output: %s mode, %d Hz, stereo, %s\n",info.initflags&BASS_WASAPI_EXCLUSIVE?"exclusive":"shared",info.freq,formats[info.format]);
	samrate=info.freq;
	
	// not playing anything via BASS, so don't need an update thread
	BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD,0);
	// setup BASS - "no sound" device
	BASS_Init(0,48000,0,0,NULL);

	// build sine table
	for (r=0;r<TABLESIZE;r++)
		sinetable[r]=sin(2.0*PI*(double)r/TABLESIZE)*0.2;

	// create a stream, stereo so that effects sound nice
	chan=BASS_StreamCreate(samrate,2,BASS_STREAM_DECODE|BASS_SAMPLE_FLOAT,(STREAMPROC*)WriteStream,0);

	// start the WASAPI device
	if (!BASS_WASAPI_Start())
		Error("Can't start output");

	printf("press these keys to play:\n\n"
			"  2 3  5 6 7  9 0  =\n"
			" Q W ER T Y UI O P[ ]\n\n"
			"press spacebar to quit\n\n");
	printf("press F1-F9 to toggle effects\n\n");

	printf("buffer = %.1fms\r",info.buflen/4/2/samrate*1000);

	while (ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE),&keyin,1,&r)) {
		int key;
		if (keyin.EventType!=KEY_EVENT) continue;
		if (keyin.Event.KeyEvent.wVirtualKeyCode==VK_SPACE) break;
		if (keyin.Event.KeyEvent.bKeyDown) {
			if (keyin.Event.KeyEvent.wVirtualKeyCode>=VK_F1
				&& keyin.Event.KeyEvent.wVirtualKeyCode<=VK_F9) {
				r=keyin.Event.KeyEvent.wVirtualKeyCode-VK_F1;
				if (fx[r]) {
					BASS_ChannelRemoveFX(chan,fx[r]);
					fx[r]=0;
					printf("effect %s = OFF\t\t\r",fxname[r]);
				} else {
					// set the effect, not bothering with parameters (use defaults)
					if (fx[r]=BASS_ChannelSetFX(chan,BASS_FX_DX8_CHORUS+r,0))
						printf("effect %s = ON\t\t\r",fxname[r]);
				}
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
