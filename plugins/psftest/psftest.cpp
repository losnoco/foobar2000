// psftest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define BUFFNUM 8
#define RATE    48000
#define BUFFLEN441 ((44100*2*2)/50)
#define BUFFLEN48 ((48000*2*2)/50)

HWAVEOUT     hWaveOut = 0; /* Device handle */
WAVEFORMATEX wfx;
LPSTR        audblock;
char audiobuffer[BUFFNUM][BUFFLEN48];

void *psx_state = NULL;
void *psf2fs = NULL;

int version = 0;
int rate = 44100;

HANDLE eventh;

/* Standard error macro for reporting API errors */
#define PERR(bSuccess, api){if(!(bSuccess)) printf("%s:Error %d from %s \
on line %d\n", __FILE__, GetLastError(), api, __LINE__);}

void cls(HANDLE hConsole)
{
	COORD coordScreen = { 0, 0 };    /* here's where we'll home the
									 cursor */
	BOOL bSuccess;
	DWORD cCharsWritten;
	CONSOLE_SCREEN_BUFFER_INFO csbi; /* to get buffer info */
	DWORD dwConSize;                 /* number of character cells in
									 the current buffer */

									 /* get the number of character cells in the current buffer */

	bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);
	PERR(bSuccess, "GetConsoleScreenBufferInfo");
	dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

	/* fill the entire screen with blanks */

	bSuccess = FillConsoleOutputCharacter(hConsole, (TCHAR) ' ',
		dwConSize, coordScreen, &cCharsWritten);
	PERR(bSuccess, "FillConsoleOutputCharacter");

	/* get the current text attribute */

	bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);
	PERR(bSuccess, "ConsoleScreenBufferInfo");

	/* now set the buffer's attributes accordingly */

	bSuccess = FillConsoleOutputAttribute(hConsole, csbi.wAttributes,
		dwConSize, coordScreen, &cCharsWritten);
	PERR(bSuccess, "FillConsoleOutputAttribute");

	/* put the cursor at (0, 0) */

	bSuccess = SetConsoleCursorPosition(hConsole, coordScreen);
	PERR(bSuccess, "SetConsoleCursorPosition");
	return;
}


void pressAny(void) {
	printf("Clicky key, get continue!\n");
	getchar();
}

struct psf1_load_state
{
	void * emu;
	bool first;
	unsigned refresh;
};

static int psf1_info(void * context, const char * name, const char * value)
{
	psf1_load_state * state = (psf1_load_state *)context;

	if (!state->refresh && !stricmp(name, "_refresh"))
	{
		state->refresh = atoi(value);
	}

	return 0;
}

int psf1_load(void * context, const uint8_t * exe, size_t exe_size,
	const uint8_t * reserved, size_t reserved_size)
{
	psf1_load_state * state = (psf1_load_state *)context;

	if (reserved && reserved_size)
		return -1;

	if (psf_load_section((PSX_STATE *)state->emu, exe, exe_size, state->first))
		return -1;

	state->first = false;

	return 0;
}

static void * psf_file_fopen(const char * uri)
{
	FILE * f = fopen(uri, "rb");
	return (void *)f;
}

static size_t psf_file_fread(void * buffer, size_t size, size_t count, void * handle)
{
	return fread(buffer, size, count, (FILE *)handle);
}

static int psf_file_fseek(void * handle, int64_t offset, int whence)
{
	return fseek((FILE *)handle, offset, whence);
}

static int psf_file_fclose(void * handle)
{
	fclose((FILE *)handle);
	return 0;
}

static long psf_file_ftell(void * handle)
{
	return ftell((FILE *)handle);
}

const psf_file_callbacks psf_file_system =
{
	"\\/|:",
	psf_file_fopen,
	psf_file_fread,
	psf_file_fseek,
	psf_file_fclose,
	psf_file_ftell
};

void psf_log(void * context, const char * message)
{
	fprintf(stdout, "%s", message);
	fflush(stdout);
}

BOOL init(char *name)
{
	//MMRESULT result;

	version = psf_load(name, &psf_file_system, 0, 0, 0, 0, 0, 0);

	if (version <= 0) return FALSE;

	if (version == 1 || version == 2)
	{
		psx_state = calloc(1, psx_get_state_size(version));
		if (!psx_state) return FALSE;

		psx_register_console_callback((PSX_STATE *)psx_state, psf_log, 0);

		if (version == 1)
		{
			psf1_load_state state;

			state.emu = psx_state;
			state.first = true;
			state.refresh = 0;

			if (psf_load(name, &psf_file_system, 1, psf1_load, &state, psf1_info, &state, 1) < 0)
				return FALSE;

			if (state.refresh)
				psx_set_refresh((PSX_STATE *)psx_state, state.refresh);

			psf_start((PSX_STATE *)psx_state);
		}
		else if (version == 2)
		{
			psf2fs = psf2fs_create();
			if (!psf2fs) return FALSE;

			psf1_load_state state;

			state.refresh = 0;

			if (psf_load(name, &psf_file_system, 2, psf2fs_load_callback, psf2fs, psf1_info, &state, 1) < 0)
				return FALSE;

			if (state.refresh)
				psx_set_refresh((PSX_STATE *)psx_state, state.refresh);

			psf2_register_readfile((PSX_STATE *)psx_state, psf2fs_virtual_readfile, psf2fs);

			psf2_start((PSX_STATE *)psx_state);
		}
	}

	wfx.nSamplesPerSec = version == 2 ? 48000 : 44100;
	wfx.wBitsPerSample = 16;
	wfx.nChannels = 2;

	wfx.cbSize = 0;
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nBlockAlign = (wfx.wBitsPerSample >> 3) * wfx.nChannels;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;

	eventh = CreateEvent(
		NULL,               // default security attributes
		TRUE,               // manual-reset event
		FALSE,              // initial state is nonsignaled
		TEXT("WriteEvent")  // object name
	);

	if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, (unsigned int)eventh, 0, CALLBACK_EVENT) != MMSYSERR_NOERROR)
	{
		printf("Unable to open waveout\n");
		return FALSE;
	}

	return TRUE;
}

void shut(void)
{
	//if( synSong ) free( synSong );
	if (hWaveOut != INVALID_HANDLE_VALUE) waveOutClose(hWaveOut);
}

void updateScreen(void)
{
}

int main(int argc, char *argv[])
{
	WAVEHDR header[BUFFNUM];
	int nextbuf = 0;
	//sigset_t base_mask, waiting_mask;

	if (argc < 2)
	{
		//My F2 gets stuck, lol
		printf("Usage: psftest <tune.psf>\n");
		printf("ESC closes.\n");
		printf("\n");
		system("pause");
		return 0;
	}

	if (init(argv[1]))
	{
		int i;
		updateScreen();

		int32(*mixChunk)(PSX_STATE *state, int16 *buffer, uint32 count);

		mixChunk = (version == 2) ? psf2_gen : psf_gen;
		int BUFFLEN = (version == 2) ? BUFFLEN48 : BUFFLEN441;

		for (i = 0; i<BUFFNUM; i++) {
			memset(&header[i], 0, sizeof(WAVEHDR));
			header[i].dwBufferLength = BUFFLEN;
			header[i].lpData = (LPSTR)audiobuffer[i];
		}
		for (i = 0; i<BUFFNUM - 1; i++) {
			mixChunk((PSX_STATE *)psx_state, (int16_t *) audiobuffer[nextbuf], BUFFLEN / 4);
			waveOutPrepareHeader(hWaveOut, &header[nextbuf], sizeof(WAVEHDR));
			waveOutWrite(hWaveOut, &header[nextbuf], sizeof(WAVEHDR));
			nextbuf = (nextbuf + 1) % BUFFNUM;
		}
		for (;;)
		{
			mixChunk((PSX_STATE *)psx_state, (int16_t *) audiobuffer[nextbuf], BUFFLEN / 4);
			waveOutPrepareHeader(hWaveOut, &header[nextbuf], sizeof(WAVEHDR));
			waveOutWrite(hWaveOut, &header[nextbuf], sizeof(WAVEHDR));
			nextbuf = (nextbuf + 1) % BUFFNUM;


			while (waveOutUnprepareHeader(hWaveOut, &header[nextbuf], sizeof(WAVEHDR)) == WAVERR_STILLPLAYING) {
				if (_kbhit()) {
					int subnum;
					switch (_getch()) {
					case 0:  /* introduces an extended key */
					case 227:  /* this also happens on Win32 (I think) */
						switch (_getch()) {  /* read the extended key code */
						case 72:    //up arrow press
							break;
						case 75:    //left arrow press
							break;
						case 77:    //right arrow press
							break;
						case 80:    //down arrow press
							break;
							/* etc */
						}
						break;
					case 27:    //escape, exit
						goto GTFO;
						break;
					}
				}
				updateScreen();
				WaitForSingleObject(eventh, INFINITE);
			}
			ResetEvent(eventh);
		}

	}
GTFO:
	//TODO: properly close WaveOut
	//too lazy
	if (psf2fs) psf2fs_delete(psf2fs);
	if (psx_state) {
		if (version == 1) psf_stop((PSX_STATE *)psx_state);
		else if (version == 2) psf2_stop((PSX_STATE *)psx_state);
		free(psx_state);
	}

	return 0;
}