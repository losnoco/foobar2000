/*
	WASAPI + BASSmix multi-speaker example
	Copyright (c) 2009-2012 Un4seen Developments Ltd.
*/

#include <windows.h>
#include <stdio.h>
#include "basswasapi.h"
#include "bass.h"
#include "bassmix.h"

HWND win=NULL;

HSTREAM mixer,chan[4]; // mixer and source channels
DWORD flags[4]={BASS_SPEAKER_FRONT,BASS_SPEAKER_REAR,BASS_SPEAKER_CENLFE,BASS_SPEAKER_REAR2};

// display error messages
void Error(const char *es)
{
	char mes[200];
	sprintf(mes,"%s\n(error code: %d)",es,BASS_ErrorGetCode());
	MessageBox(win,mes,0,0);
}

#define MESS(id,m,w,l) SendDlgItemMessage(win,id,m,(WPARAM)(w),(LPARAM)(l))

// WASAPI function
DWORD CALLBACK WasapiProc(void *buffer, DWORD length, void *user)
{
	DWORD c=BASS_ChannelGetData(mixer,buffer,length);
	if (c==-1) c=0; // an error, no data
	return c;
}

INT_PTR CALLBACK dialogproc(HWND h,UINT m,WPARAM w,LPARAM l)
{
	static OPENFILENAME ofn;

	switch (m) {
		case WM_COMMAND:
			switch (LOWORD(w)) {
				case IDCANCEL:
					DestroyWindow(h);
					return 1;
				case 10: // open a file to play on #1
				case 11: // open a file to play on #2
				case 12: // open a file to play on #3
				case 13: // open a file to play on #4
					{
						int speaker=LOWORD(w)-10;
						char file[MAX_PATH]="";
						ofn.lpstrFile=file;
						if (GetOpenFileName(&ofn)) {
							BASS_StreamFree(chan[speaker]); // free old stream before opening new
							if (!(chan[speaker]=BASS_StreamCreateFile(FALSE,file,0,0,BASS_SAMPLE_LOOP|BASS_SAMPLE_FLOAT|BASS_STREAM_DECODE))) {
								MESS(10+speaker,WM_SETTEXT,0,"click here to open a file...");
								Error("Can't play the file");
								return 1;
							}
							MESS(10+speaker,WM_SETTEXT,0,file);
							BASS_Mixer_StreamAddChannel(mixer,chan[speaker],flags[speaker]); // add it to the mixer
						}
					}
					return 1;
				case 20: // swap #1 & #2
				case 21: // swap #2 & #3
				case 22: // swap #3 & #4
					{
						int speaker=LOWORD(w)-20;
						{ // swap handles
							HSTREAM temp=chan[speaker];
							chan[speaker]=chan[speaker+1];
							chan[speaker+1]=temp;
						}
						{ // swap text
							char temp1[MAX_PATH],temp2[MAX_PATH];
							MESS(10+speaker,WM_GETTEXT,MAX_PATH,temp1);
							MESS(10+speaker+1,WM_GETTEXT,MAX_PATH,temp2);
							MESS(10+speaker,WM_SETTEXT,0,temp2);
							MESS(10+speaker+1,WM_SETTEXT,0,temp1);
						}
						// update speaker flags
						BASS_Mixer_ChannelFlags(chan[speaker],flags[speaker],BASS_SPEAKER_FRONT);
						BASS_Mixer_ChannelFlags(chan[speaker+1],flags[speaker+1],BASS_SPEAKER_FRONT);
					}
					return 1;
			}
			break;

		case WM_INITDIALOG:
			win=h;
			memset(&ofn,0,sizeof(ofn));
			ofn.lStructSize=sizeof(ofn);
			ofn.hwndOwner=h;
			ofn.nMaxFile=MAX_PATH;
			ofn.Flags=OFN_HIDEREADONLY|OFN_EXPLORER;
			ofn.lpstrFilter="Streamable files\0*.mp3;*.mp2;*.mp1;*.ogg;*.wav;*.aif\0All files\0*.*\0\0";
			// not playing anything via BASS, so don't need an update thread
			BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD,0);
			// setup BASS - "no sound" device
			BASS_Init(0,48000,0,0,NULL);
			{ // initialize the default WASAPI device (400ms buffer, 50ms update period, auto-select format)
				if (!BASS_WASAPI_Init(-1,0,0,BASS_WASAPI_AUTOFORMAT|BASS_WASAPI_EXCLUSIVE,0.4,0.05,WasapiProc,NULL)) {
					// exclusive mode failed, try shared mode
					if (!BASS_WASAPI_Init(-1,0,0,BASS_WASAPI_AUTOFORMAT,0.4,0.05,WasapiProc,NULL)) {
						Error("Can't initialize device");
						DestroyWindow(win);
						break;
					}
				}
				{
					BASS_WASAPI_INFO wi;
					BASS_WASAPI_GetInfo(&wi);
					// create a mixer with same format as the output
					mixer=BASS_Mixer_StreamCreate(wi.freq,wi.chans,BASS_SAMPLE_FLOAT|BASS_STREAM_DECODE);
					// start the output
					BASS_WASAPI_Start();
					if (wi.chans<8) {
						EnableWindow(GetDlgItem(h,13),FALSE);
						EnableWindow(GetDlgItem(h,22),FALSE);
					}
					if (wi.chans<6) {
						EnableWindow(GetDlgItem(h,12),FALSE);
						EnableWindow(GetDlgItem(h,21),FALSE);
					}
					if (wi.chans<4) {
						EnableWindow(GetDlgItem(h,11),FALSE);
						EnableWindow(GetDlgItem(h,20),FALSE);
					}
				}
			}
			MESS(11,BM_SETCHECK,BST_CHECKED,0);
			MESS(12,BM_SETCHECK,BST_CHECKED,0);
			MESS(13,BM_SETCHECK,BST_CHECKED,0);
			MESS(14,BM_SETCHECK,BST_CHECKED,0);
			return 1;

		case WM_DESTROY:
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

	DialogBox(hInstance,(char*)1000,0,&dialogproc);

	return 0;
}
