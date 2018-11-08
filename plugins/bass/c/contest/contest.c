/*
	WASAPI version of the BASS simple console player
	Copyright (c) 1999-2017 Un4seen Developments Ltd.
*/

#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include "basswasapi.h"
#include "bassmix.h"
#include "bass.h"

HSTREAM mixer;

// display error messages
void Error(const char *text) 
{
	printf("Error(%d): %s\n",BASS_ErrorGetCode(),text);
	BASS_WASAPI_Free();
	BASS_Free();
	exit(0);
}

// WASAPI function
DWORD CALLBACK WasapiProc(void *buffer, DWORD length, void *user)
{
	return BASS_ChannelGetData(mixer,buffer,length);
}

void ListDevices()
{
	BASS_WASAPI_DEVICEINFO di;
	int a;
	for (a=0;BASS_WASAPI_GetDeviceInfo(a,&di);a++) {
		if ((di.flags&BASS_DEVICE_ENABLED) && !(di.flags&BASS_DEVICE_INPUT)) // enabled output device
			printf("dev %d: %s\n",a,di.name);
	}
}

void main(int argc, char **argv)
{
	const char *formats[5]={"32-bit float","8-bit","16-bit","24-bit","32-bit"};
	DWORD source,time,opos;
	BOOL ismod;
	QWORD pos;
	int a,filep,device=-1;
	DWORD flags=BASS_WASAPI_AUTOFORMAT|BASS_WASAPI_BUFFER; // default initialization flags
	BASS_WASAPI_DEVICEINFO di;

	printf("Simple console mode BASS+WASAPI example player\n"
			"----------------------------------------------\n");

	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion())!=BASSVERSION) {
		printf("An incorrect version of BASS was loaded");
		return;
	}

	for (filep=1;filep<argc;filep++) {
		if (!strcmp(argv[filep],"-l")) {
			ListDevices();
			return;
		} else if (!strcmp(argv[filep],"-d") && filep+1<argc) device=atoi(argv[++filep]);
		else if (!strcmp(argv[filep],"-x")) flags|=BASS_WASAPI_EXCLUSIVE;
		else if (!strcmp(argv[filep],"-e")) flags|=BASS_WASAPI_EVENT;
		else break;
	}
	if (filep==argc) {
		printf("\tusage: contest [-l] [-d #] [-s] [-e] <file>\n"
			"\t-l = list devices\n"
			"\t-d = device number\n"
			"\t-x = exclusive mode, else shared mode\n"
			"\t-e = event-driven buffering\n");
		return;
	}

	if (device==-1) { // find the default output device and get its "mix" format
		for (device==-1,a=0;BASS_WASAPI_GetDeviceInfo(a,&di);a++) {
			if ((di.flags&(BASS_DEVICE_DEFAULT|BASS_DEVICE_LOOPBACK|BASS_DEVICE_INPUT))==BASS_DEVICE_DEFAULT) {
				device=a;
				break;
			}
		}
		if (device==-1) Error("Can't find an output device");
	} else {
		if (!BASS_WASAPI_GetDeviceInfo(device,&di) || (di.flags&BASS_DEVICE_INPUT))
			Error("Invalid device number");
	}

	// not playing anything via BASS, so don't need an update thread
	BASS_SetConfig(BASS_CONFIG_UPDATETHREADS,0);
	// setup BASS - "no sound" device with the "mix" sample rate (default for MOD music)
	BASS_Init(0,di.mixfreq,0,0,NULL);

	// try streaming the file/url
	if ((source=BASS_StreamCreateFile(FALSE,argv[filep],0,0,BASS_SAMPLE_LOOP|BASS_STREAM_DECODE|BASS_SAMPLE_FLOAT))
		|| (source=BASS_StreamCreateURL(argv[filep],0,BASS_SAMPLE_LOOP|BASS_STREAM_DECODE|BASS_SAMPLE_FLOAT,0,0))) {
		pos=BASS_ChannelGetLength(source,BASS_POS_BYTE);
		if (BASS_StreamGetFilePosition(source,BASS_FILEPOS_DOWNLOAD)!=-1) {
			// streaming from the internet
			if (pos!=-1)
				printf("streaming internet file [%I64u bytes]",pos);
			else
				printf("streaming internet file");
		} else
			printf("streaming file [%I64u bytes]",pos);
		ismod=FALSE;
	} else {
		// try loading the MOD (with looping, sensitive ramping, and calculate the duration)
		if (!(source=BASS_MusicLoad(FALSE,argv[filep],0,0,BASS_SAMPLE_LOOP|BASS_STREAM_DECODE|BASS_SAMPLE_FLOAT|BASS_MUSIC_RAMPS|BASS_MUSIC_PRESCAN,0)))
			// not a MOD either
			Error("Can't play the file");
		{ // count channels
			float dummy;
			for (a=0;BASS_ChannelGetAttribute(source,BASS_ATTRIB_MUSIC_VOL_CHAN+a,&dummy);a++);
		}
		printf("playing MOD music \"%s\" [%u chans, %u orders]",
			BASS_ChannelGetTags(source,BASS_TAG_MUSIC_NAME),a,BASS_ChannelGetLength(source,BASS_POS_MUSIC_ORDER));
		pos=BASS_ChannelGetLength(source,BASS_POS_BYTE);
		ismod=TRUE;
	}

	// display the time length
	if (pos!=-1) {
		time=(DWORD)BASS_ChannelBytes2Seconds(source,pos);
		printf(" %u:%02u\n",time/60,time%60);
	} else // no time length available
		printf("\n");

	{ // setup output
		float buflen=((flags&(BASS_WASAPI_EXCLUSIVE|BASS_WASAPI_EVENT))==(BASS_WASAPI_EXCLUSIVE|BASS_WASAPI_EVENT)?0.1:0.4); // smaller buffer with event-driven exclusive mode
		BASS_WASAPI_INFO wi;
		BASS_CHANNELINFO ci;
		BASS_ChannelGetInfo(source,&ci);
		printf("sample format: %d Hz, %d channels\n",ci.freq,ci.chans);
		// initialize the WASAPI device
		if (!BASS_WASAPI_Init(device,ci.freq,ci.chans,flags,buflen,0.05,WasapiProc,NULL)) {
			// failed, try falling back to shared mode
			if (!(flags&BASS_WASAPI_EXCLUSIVE) || !BASS_WASAPI_Init(device,ci.freq,ci.chans,flags&~BASS_WASAPI_EXCLUSIVE,buflen,0.05,WasapiProc,NULL))
				Error("Can't initialize device");
		}
		// get the output details
		BASS_WASAPI_GetInfo(&wi);
		printf("output: %s%s mode, %d Hz, %d channels, %s\n",
			wi.initflags&BASS_WASAPI_EVENT?"event-driven ":"",wi.initflags&BASS_WASAPI_EXCLUSIVE?"exclusive":"shared",wi.freq,wi.chans,formats[wi.format]);
		// create a mixer with the same sample format (and enable GetPositionEx)
		mixer=BASS_Mixer_StreamCreate(wi.freq,wi.chans,BASS_SAMPLE_FLOAT|BASS_STREAM_DECODE|BASS_MIXER_POSEX);
		// add the source to the mixer (downmix if necessary)
		BASS_Mixer_StreamAddChannel(mixer,source,BASS_MIXER_DOWNMIX);
		// start it
		if (!BASS_WASAPI_Start())
			Error("Can't start output");
	}

	while (!_kbhit() && BASS_ChannelIsActive(mixer)) {
		// display some stuff and wait a bit
		BASS_WASAPI_Lock(TRUE); // prevent processing mid-calculation
		a=BASS_WASAPI_GetData(NULL,BASS_DATA_AVAILABLE); // get amount of buffered data
		pos=BASS_Mixer_ChannelGetPositionEx(source,BASS_POS_BYTE,a); // get source position at that point
		if (ismod) opos=BASS_Mixer_ChannelGetPositionEx(source,BASS_POS_MUSIC_ORDER,a); // get the "order" position too
		BASS_WASAPI_Lock(FALSE);
		time=BASS_ChannelBytes2Seconds(source,pos);
		printf("pos %09llu",pos);
		if (ismod) printf(" (%03u:%03u)",LOWORD(opos),HIWORD(opos));
		printf(" - %u:%02u - L ",time/60,time%60);
		{
			DWORD level=BASS_WASAPI_GetLevel(); // left channel level
			for (a=27204;a>200;a=a*2/3) putchar(LOWORD(level)>=a?'*':'-');
			putchar(' ');
			for (a=210;a<32768;a=a*3/2) putchar(HIWORD(level)>=a?'*':'-');
		}
		printf(" R - cpu %.2f%%  \r",BASS_WASAPI_GetCPU());
		fflush(stdout);
		Sleep(50);
	}
	printf("                                                                             \r");

	BASS_WASAPI_Free();
	BASS_Free();
}
