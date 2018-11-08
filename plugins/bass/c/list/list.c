/*
	BASSWASAPI device list example
	Copyright (c) 2010-2017 Un4seen Developments Ltd.
*/

#include <stdio.h>
#include "basswasapi.h"

void DisplayDeviceInfo(DWORD device, BASS_WASAPI_DEVICEINFO *di)
{
	printf("dev %d: %s\n\tid: %s\n\ttype: ",device,di->name,di->id);
	switch (di->type) {
		case BASS_WASAPI_TYPE_NETWORKDEVICE:
			printf("Remote Network Device");
			break;
		case BASS_WASAPI_TYPE_SPEAKERS:
			printf("Speakers");
			break;
		case BASS_WASAPI_TYPE_LINELEVEL:
			printf("Line");
			break;
		case BASS_WASAPI_TYPE_HEADPHONES:
			printf("Headphones");
			break;
		case BASS_WASAPI_TYPE_MICROPHONE: 
			printf("Microphone");
			break;
		case BASS_WASAPI_TYPE_HEADSET:
			printf("Headset");
			break;
		case BASS_WASAPI_TYPE_HANDSET:
			printf("Handset");
			break;
		case BASS_WASAPI_TYPE_DIGITAL:
			printf("Digital");
			break;
		case BASS_WASAPI_TYPE_SPDIF:
			printf("SPDIF");
			break;
		case BASS_WASAPI_TYPE_HDMI:
			printf("HDMI");
			break;
		default:
			printf("Undefined (%d)",di->type);
	}
	printf("\n\tflags:");
	if (di->flags&BASS_DEVICE_LOOPBACK) printf(" loopback");
	if (di->flags&BASS_DEVICE_INPUT) printf(" input");
	if (di->flags&BASS_DEVICE_ENABLED) printf(" enabled");
	if (di->flags&BASS_DEVICE_DEFAULT) printf(" default");
	if (di->flags&BASS_DEVICE_UNPLUGGED) printf(" unplugged");
	if (di->flags&BASS_DEVICE_DISABLED) printf(" disabled");
	printf(" (%d)\n",di->flags);
#if 0
	{ // check supported rates
		int b=0;
		while (1) {
			int r1,r2;
			r1=BASS_WASAPI_CheckFormat(device,44100<<b,2,BASS_WASAPI_EXCLUSIVE);
			r2=BASS_WASAPI_CheckFormat(device,48000<<b,2,BASS_WASAPI_EXCLUSIVE);
			if (r1<0 && r2<0) break;
			if (!b) printf("\trates:");
			if (r1>=0) printf(" %d",44100<<b);
			if (r2>=0) printf(" %d",48000<<b);
			b++;
		}
		if (b) printf("\n");
	}
#endif
}

void CALLBACK WasapiNotifyProc(DWORD notify, DWORD device, void *user)
{
	static const char *types[]={"ENABLED", "DISABLED", "DEFOUTPUT", "DEFINPUT"};
	BASS_WASAPI_DEVICEINFO di;
	BASS_WASAPI_GetDeviceInfo(device,&di);
	if (di.flags&BASS_DEVICE_LOOPBACK) return; // ignore loopback devices
	printf("notification type %d (%s)\n",notify,notify<4?types[notify]:"unknown");
	DisplayDeviceInfo(device,&di);
}

void main()
{
	BASS_WASAPI_DEVICEINFO di;
	int device;
	for (device=0;BASS_WASAPI_GetDeviceInfo(device,&di);device++)
		DisplayDeviceInfo(device,&di);

	printf("\nmonitoring changes (press enter to stop)\n\n");
	BASS_WASAPI_SetNotify(WasapiNotifyProc,NULL); // request device notifications
	getchar();
	BASS_WASAPI_SetNotify(NULL,NULL); // disable device notifications
}
