/*
	WASAPI recording example
	Copyright (c) 2002-2010 Un4seen Developments Ltd.
*/

#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <stdio.h>
#include "basswasapi.h"
#include "bass.h"
#include "bassmix.h"

HWND win=NULL;

#define BUFSTEP 200000	// memory allocation unit

int indev=-1;			// current input device
HSTREAM instream=0;		// input stream
HSTREAM inmixer=0;		// mixer for resampling input

int outdev=-1;			// output device
HSTREAM outstream=0;	// playback stream
HSTREAM outmixer=0;		// mixer for resampling output

char *recbuf=NULL;		// recording buffer
DWORD reclen;			// recording length

float inlevel=0;		// input level

// display error messages
void Error(const char *es)
{
	char mes[200];
	sprintf(mes,"%s\n(error code: %d)",es,BASS_ErrorGetCode());
	MessageBox(win,mes,0,0);
}

// messaging macros
#define MESS(id,m,w,l) SendDlgItemMessage(win,id,m,(WPARAM)(w),(LPARAM)(l))
#define DLGITEM(id) GetDlgItem(win,id)

// WASAPI input processing function
DWORD CALLBACK InWasapiProc(void *buffer, DWORD length, void *user)
{
	BYTE temp[50000];
	int c;
	// give the data to the mixer feeder stream
	BASS_StreamPutData(instream,buffer,length);
	// get back resampled data from the mixer
	while ((c=BASS_ChannelGetData(inmixer,temp,sizeof(temp)))>0) {
		// increase buffer size if needed
		if ((reclen%BUFSTEP)+c>=BUFSTEP) {
			recbuf=realloc(recbuf,((reclen+c)/BUFSTEP+1)*BUFSTEP);
			if (!recbuf) {
				Error("Out of memory!");
				MESS(10,WM_SETTEXT,0,"Record");
				return 0; // stop recording
			}
		}
		// buffer the data
		memcpy(recbuf+reclen,temp,c);
		reclen+=c;
	}
	return 1; // continue recording
}

// WASAPI output processing function
DWORD CALLBACK OutWasapiProc(void *buffer, DWORD length, void *user)
{
	int c=BASS_ChannelGetData(outmixer,buffer,length);
	if (c<0) { // at the end
		if (!BASS_WASAPI_GetData(NULL,BASS_DATA_AVAILABLE)) // no buffered data remaining, so...
			BASS_WASAPI_Stop(FALSE); // stop the output
		return 0;
	}
	return c;
}

void InitInputDevice()
{
	// inialize the input device (shared mode, 1s buffer & 100ms update period)
	if (BASS_WASAPI_Init(indev,0,0,0,1,0.1,InWasapiProc,NULL)) {
		// create a BASS push stream of same format to feed the mixer/resampler
		BASS_WASAPI_INFO wi;
		BASS_WASAPI_GetInfo(&wi);
		instream=BASS_StreamCreate(wi.freq,wi.chans,BASS_SAMPLE_FLOAT|BASS_STREAM_DECODE,STREAMPROC_PUSH,NULL);
		if (inmixer) { // already recording, start the new device...
			BASS_Mixer_StreamAddChannel(inmixer,instream,0);
			BASS_WASAPI_Start();
		}
		{ // update level slider
			float level=BASS_WASAPI_GetVolume(BASS_WASAPI_CURVE_WINDOWS);
			if (level<0) { // failed to get level
				level=1; // just display 100%
				EnableWindow(DLGITEM(14),FALSE);
			} else
				EnableWindow(DLGITEM(14),TRUE);
			MESS(14,TBM_SETPOS,TRUE,level*100);
		}
	} else { // failed, just set level slider to 0
		EnableWindow(DLGITEM(14),FALSE);
		MESS(14,TBM_SETPOS,TRUE,0);
	}
	{ // update device type display
		char type[50];
		BASS_WASAPI_DEVICEINFO di;
		BASS_WASAPI_GetDeviceInfo(indev,&di);
		switch (di.type) {
			case BASS_WASAPI_TYPE_NETWORKDEVICE:
				strcpy(type,"Remote Network Device");
				break;
			case BASS_WASAPI_TYPE_SPEAKERS:
				strcpy(type,"Speakers");
				break;
			case BASS_WASAPI_TYPE_LINELEVEL:
				strcpy(type,"Line In");
				break;
			case BASS_WASAPI_TYPE_HEADPHONES:
				strcpy(type,"Headphones");
				break;
			case BASS_WASAPI_TYPE_MICROPHONE: 
				strcpy(type,"Microphone");
				break;
			case BASS_WASAPI_TYPE_HEADSET:
				strcpy(type,"Headset");
				break;
			case BASS_WASAPI_TYPE_HANDSET:
				strcpy(type,"Handset");
				break;
			case BASS_WASAPI_TYPE_DIGITAL:
				strcpy(type,"Digital");
				break;
			case BASS_WASAPI_TYPE_SPDIF:
				strcpy(type,"SPDIF");
				break;
			case BASS_WASAPI_TYPE_HDMI:
				strcpy(type,"HDMI");
				break;
			default:
				strcpy(type,"undefined");
		}
		if (di.flags&BASS_DEVICE_LOOPBACK) strcat(type," (loopback)");
		MESS(15,WM_SETTEXT,0,type);
	}
}

void StartRecording()
{
	WAVEFORMATEX *wf;
	int rate;
	if (recbuf) { // free old recording...
		BASS_StreamFree(outstream);
		outstream=0;
		free(recbuf);
		recbuf=NULL;
		EnableWindow(DLGITEM(11),FALSE);
		EnableWindow(DLGITEM(12),FALSE);
	}
	{ // get the sample rate choice
		char buf[20];
		GetWindowText(DLGITEM(16),buf,sizeof(buf));
		rate=atoi(buf);
	}
	// allocate initial buffer and make space for WAVE header
	recbuf=malloc(BUFSTEP);
	reclen=44;
	// fill the WAVE header
	memcpy(recbuf,"RIFF\0\0\0\0WAVEfmt \20\0\0\0",20);
	memcpy(recbuf+36,"data\0\0\0\0",8);
	wf=(WAVEFORMATEX*)(recbuf+20);
	wf->wFormatTag=1;
	wf->nChannels=2;
	wf->wBitsPerSample=16;
	wf->nSamplesPerSec=rate;
	wf->nBlockAlign=wf->nChannels*wf->wBitsPerSample/8;
	wf->nAvgBytesPerSec=wf->nSamplesPerSec*wf->nBlockAlign;

	// create a mixer and add the device's feeder stream to it
	inmixer=BASS_Mixer_StreamCreate(rate,2,BASS_STREAM_DECODE);
	BASS_Mixer_StreamAddChannel(inmixer,instream,0);
	// start the input device
	if (!BASS_WASAPI_SetDevice(indev) || !BASS_WASAPI_Start()) {
		Error("Can't start recording");
		BASS_StreamFree(inmixer);
		inmixer=0;
		free(recbuf);
		recbuf=0;
		return;
	}
	MESS(10,WM_SETTEXT,0,"Stop");
	EnableWindow(DLGITEM(16),FALSE);
}

void StopRecording()
{
	// stop the device and free the mixer
	BASS_WASAPI_SetDevice(indev);
	BASS_WASAPI_Stop(TRUE);
	BASS_StreamFree(inmixer);
	inmixer=0;
	MESS(10,WM_SETTEXT,0,"Record");
	// complete the WAVE header
	*(DWORD*)(recbuf+4)=reclen-8;
	*(DWORD*)(recbuf+40)=reclen-44;
	// enable "save" button
	EnableWindow(DLGITEM(12),TRUE);
	// re-enable rate selection
	EnableWindow(DLGITEM(16),TRUE);
	if (outdev>=0) {
		// create a stream from the recording
		if (outstream=BASS_StreamCreateFile(TRUE,recbuf,0,reclen,BASS_SAMPLE_FLOAT|BASS_STREAM_DECODE)) {
			BASS_Mixer_StreamAddChannel(outmixer,outstream,0);
			EnableWindow(DLGITEM(11),TRUE); // enable "play" button
		}
	}
}

void StartPlaying()
{
	BASS_WASAPI_SetDevice(outdev);
	BASS_WASAPI_Stop(TRUE); // flush the output device buffer (in case there is anything there)
	BASS_Mixer_ChannelSetPosition(outstream,0,BASS_POS_BYTE); // rewind output stream
	BASS_ChannelSetPosition(outmixer,0,BASS_POS_BYTE); // reset mixer
	BASS_WASAPI_Start(); // start the device
}

void WriteToDisk()
{
	FILE *fp;
	char file[MAX_PATH]="";
	OPENFILENAME ofn={0};
	ofn.lStructSize=sizeof(ofn);
	ofn.hwndOwner=win;
	ofn.nMaxFile=MAX_PATH;
	ofn.lpstrFile=file;
	ofn.Flags=OFN_HIDEREADONLY|OFN_EXPLORER;
	ofn.lpstrFilter="WAV files\0*.wav\0All files\0*.*\0\0";
	ofn.lpstrDefExt="wav";
	if (!GetSaveFileName(&ofn)) return;
	if (!(fp=fopen(file,"wb"))) {
		Error("Can't create the file");
		return;
	}
	fwrite(recbuf,reclen,1,fp);
	fclose(fp);
}

INT_PTR CALLBACK dialogproc(HWND h,UINT m,WPARAM w,LPARAM l)
{
	switch (m) {
		case WM_TIMER:
			{ // update the recording/playback counter
				char text[30]="";
				if (outstream) {
					BASS_WASAPI_SetDevice(outdev);
					if (BASS_WASAPI_IsStarted()) { // playing
						QWORD pos;
						DWORD delay;
						BASS_WASAPI_Lock(TRUE); // prevent processing mid-calculation
						delay=BASS_WASAPI_GetData(NULL,BASS_DATA_AVAILABLE); // get amount of buffered data
						pos=BASS_Mixer_ChannelGetPositionEx(outstream,BASS_POS_BYTE,delay); // get source position at that point
						BASS_WASAPI_Lock(FALSE);
						sprintf(text,"%I64d / %I64d",pos,BASS_ChannelGetLength(outstream,BASS_POS_BYTE));
					} else
						sprintf(text,"%I64d",BASS_ChannelGetLength(outstream,BASS_POS_BYTE));
				} else if (inmixer) // recording
					sprintf(text,"%I64d",BASS_ChannelGetPosition(inmixer,BASS_POS_BYTE));
				MESS(20,WM_SETTEXT,0,text);
			}
			{ // update the input level meter
				float level=BASS_WASAPI_GetDeviceLevel(indev,-1);
				inlevel=inlevel>0.1?inlevel-0.1:0;
				if (level>inlevel) inlevel=level;
				{ // draw the level bar
					HWND w=DLGITEM(30);
					HDC dc=GetWindowDC(w);
					RECT r;
					GetClientRect(w,&r);
					InflateRect(&r,-1,-1);
					r.top=r.bottom*(1-inlevel);
					FillRect(dc,&r,GetStockObject(WHITE_BRUSH));
					r.bottom=r.top;
					r.top=1;
					FillRect(dc,&r,GetStockObject(LTGRAY_BRUSH));
					ReleaseDC(w,dc);
				}
			}
			break;

		case WM_COMMAND:
			switch (LOWORD(w)) {
				case IDCANCEL:
					DestroyWindow(h);
					break;
				case 10:
					if (!inmixer)
						StartRecording();
					else
						StopRecording();
					break;
				case 11:
					StartPlaying();
					break;
				case 12:
					WriteToDisk();
					break;
				case 13:
					if (HIWORD(w)==CBN_SELCHANGE) { // input selection changed
						int i;
						// free current device and its mixer feeder
						BASS_WASAPI_SetDevice(indev);
						BASS_WASAPI_Free();
						BASS_StreamFree(instream);
						instream=0;
						i=MESS(13,CB_GETCURSEL,0,0); // get the selected device
						indev=MESS(13,CB_GETITEMDATA,i,0); // get the device #
						InitInputDevice(); // initialize device
					}
					break;
			}
			break;

		case WM_HSCROLL:
			if (l) { // set device volume level
				float level=SendMessage((HWND)l,TBM_GETPOS,0,0)/100.f;
				if (BASS_WASAPI_SetDevice(indev))
					BASS_WASAPI_SetVolume(BASS_WASAPI_CURVE_WINDOWS,level);
			}
			break;

		case WM_INITDIALOG:
			win=h;
			// not playing anything via BASS, so don't need an update thread
			BASS_SetConfig(BASS_CONFIG_UPDATETHREADS,0);
			// setup BASS - "no sound" device
			BASS_Init(0,48000,0,0,NULL);
			// initialize default WASAPI output device for playback
			if (BASS_WASAPI_Init(-1,0,0,0,0.4,0.05,OutWasapiProc,NULL)) {
				BASS_WASAPI_INFO wi;
				outdev=BASS_WASAPI_GetDevice();
				// create a mixer to feed the output device
				BASS_WASAPI_GetInfo(&wi);
				outmixer=BASS_Mixer_StreamCreate(wi.freq,wi.chans,BASS_SAMPLE_FLOAT|BASS_STREAM_DECODE|BASS_MIXER_END|BASS_MIXER_POSEX);
			}
			MESS(14,TBM_SETRANGE,FALSE,MAKELONG(0,100)); // initialize input level slider
			{ // get list of WASAPI input devices
				int c;
				BASS_WASAPI_DEVICEINFO di;
				for (c=0;BASS_WASAPI_GetDeviceInfo(c,&di);c++) {
					if ((di.flags&BASS_DEVICE_INPUT) && (di.flags&BASS_DEVICE_ENABLED)) { // it's an enabled input device
						int i=MESS(13,CB_ADDSTRING,0,di.name);
						MESS(13,CB_SETITEMDATA,i,c); // retain device # for later
						if (di.flags&BASS_DEVICE_DEFAULT) { // it's the default
							MESS(13,CB_SETCURSEL,i,0);
							indev=c;
							InitInputDevice(); // initialize device
						}
					}
				}
				if (indev==-1) {
					Error("Can't find any WASAPI input devices");
					DestroyWindow(win);
					break;
				}
			}
			// initialize sample rate list
			MESS(16,CB_ADDSTRING,0,"96000");
			MESS(16,CB_ADDSTRING,0,"48000");
			MESS(16,CB_ADDSTRING,0,"44100");
			MESS(16,CB_ADDSTRING,0,"32000");
			MESS(16,CB_ADDSTRING,0,"22050");
			MESS(16,CB_SETCURSEL,1,0);
			SetTimer(h,0,100,0); // timer to update the position & level display
			return 1;

		case WM_DESTROY:
			// release all BASS/WASAPI stuff
			while (BASS_WASAPI_Free()) ;
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
