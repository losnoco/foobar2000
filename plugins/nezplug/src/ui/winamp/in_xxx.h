/*
	KUMAamp project(仮称)
	Copyright (C) Mamiya 2000.
	---------------------------------------------------------------------------
	generic WINAMP interface
	---------------------------------------------------------------------------
	$Id: in_xxx.h,v 0.7.9 2000/05/07 Mamiya Exp $
*/

#define MAX_SAMPLERATE 192000 /*<- Mwahaha. 48000 */
#define SIZE_OF_BUFFER 30	/* (msec) */

static In_Module mod;
static struct {
	DWORD dwThreadId;
	HANDLE hThread;
	HANDLE hEvent;
	int request;
	int paused;
	int need_seek;
	int length;
	int time_pos;
} player_data = {
	0,
	NULL,NULL,
	0,0,0,0,0,
};

enum PLAYER_REQUEST {
	PLAYER_REQUEST_QUIT,
	PLAYER_REQUEST_STOP,
	PLAYER_REQUEST_PAUSE,
	PLAYER_REQUEST_UNPAUSE,
	PLAYER_REQUEST_SEEK
};

static void PlayerSetEvent(enum PLAYER_REQUEST request)
{
	if (player_data.hEvent != NULL)
	{
		player_data.request |= 1 << request;
		SetEvent(player_data.hEvent);
	}
}

static void init(void)
{
	player_data.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	InitSequencer(mod.hDllInstance);
}

static void quit(void)
{
	QuitSequencer(mod.hDllInstance);
	if (player_data.hEvent != NULL) {
		CloseHandle(player_data.hEvent);
		player_data.hEvent = NULL;
	}
}

static void getfileinfo(char *fn, char *title, int *length_in_ms)
{
	getFileInfo(fn, title, length_in_ms);
}

static DWORD __stdcall PlayThread(SEQUENCER *sequencer)
{
#if !N_VERSION
#if !defined(MAX_SAMPLERATE)
	void *samp_buf = NULL;	/* DYNAMIC ALLOCATION */
#else
	static short samp_buf[((MAX_SAMPLERATE * SIZE_OF_BUFFER) / 1000) * 2 * 2 + 8];
#endif
#endif
	int request = 0;
	player_data.paused = 0;
	player_data.request = 0;
	ResetEvent(player_data.hEvent);
	/* try */
	do
	{
		static int tbl_prio[] = {
			THREAD_PRIORITY_IDLE,
			THREAD_PRIORITY_LOWEST,
			THREAD_PRIORITY_BELOW_NORMAL,
			THREAD_PRIORITY_NORMAL,
			THREAD_PRIORITY_ABOVE_NORMAL,
			THREAD_PRIORITY_HIGHEST,
			THREAD_PRIORITY_TIME_CRITICAL
		};
		int samplerate, loop_n, loop_c = -1;
		DWORD wt;
#if !N_VERSION
		int ch, bufsize, len;
		ch = sequencer->GetChannel(sequencer);
		samplerate = sequencer->GetRate(sequencer);
		bufsize = (samplerate * SIZE_OF_BUFFER) / 1000;

		if (bufsize >= (MAX_SAMPLERATE * SIZE_OF_BUFFER) / 1000)
			bufsize = (MAX_SAMPLERATE * SIZE_OF_BUFFER) / 1000;
		else if (bufsize < 576)
			bufsize = 576;

#if !defined(MAX_SAMPLERATE)
		samp_buf = malloc(bufsize * ch * 2 * 2  + 8);
		if (samp_buf == NULL) break;
#endif
#else
		samplerate = 0;
#endif
		SetThreadPriority(GetCurrentThread(), tbl_prio[sequencer->GetPriority(sequencer)]);
		sequencer->Play(sequencer);

		while (sequencer->IsPlaying(sequencer))
		{
			player_data.time_pos = sequencer->GetPosition(sequencer);

			loop_n = sequencer->GetLoopCount(sequencer);
			if (loop_c != loop_n)
			{
				loop_c = loop_n;
				mod.SetInfo(loop_c, samplerate / 1000, ch, 0);
			}

#if !N_VERSION
			sequencer->Mix(sequencer, samp_buf, bufsize);
			mod.SAAddPCMData(samp_buf, ch, 16, player_data.time_pos);
			mod.VSAAddPCMData(samp_buf, ch, 16, player_data.time_pos);
			if(mod.dsp_isactive())
			{
				len = mod.dsp_dosamples(samp_buf, bufsize, 16, ch, samplerate);
			}
			else
			{
				len = bufsize;
			}
			wt = 0;
#else
			wt = 10;
#endif
			do
			{
				if (WaitForSingleObject(player_data.hEvent, wt) != WAIT_TIMEOUT)
				{
					request = InterlockedExchange((LPLONG)&player_data.request, 0);
					if (request == 0)
					{
						/* ??? THREAD FAULT ??? */
						/* イヤなタイミングでボタンを押すね */
						request |= (1 << PLAYER_REQUEST_STOP);
					}
					break;
				}
				wt = 10;
			}
#if !N_VERSION
			while (mod.outMod->CanWrite() < (len << 2));
#else
			while (0);
#endif
			if (request & (1 << PLAYER_REQUEST_QUIT)) break;
			if (request & (1 << PLAYER_REQUEST_STOP)) break;
#if N_VERSION
			if (request & (1 << PLAYER_REQUEST_PAUSE))
			{
				player_data.paused = 1;
				sequencer->Pause(sequencer, player_data.paused);
				request = 0;
				continue;
			}
			if (request & (1 << PLAYER_REQUEST_UNPAUSE))
			{
				player_data.paused = 0;
				sequencer->Pause(sequencer, player_data.paused);
				request = 0;
				continue;
			}
#endif
			if (request & (1 << PLAYER_REQUEST_SEEK))
			{
#if !N_VERSION
				mod.outMod->Flush(player_data.need_seek);
#endif
				sequencer->SetPosition(sequencer, player_data.need_seek);
				player_data.need_seek = 0;
				request = 0;
				continue;
			}
#if !N_VERSION
			if (mod.outMod->CanWrite() >= (len << ch))
				mod.outMod->Write((char *)samp_buf, (len << ch));
#endif
		}
#if 1 && !N_VERSION	/* 必要らしい */
		if (request == 0)
		{
			/*  再生終了(出力終了待ち) */
			wt = 0;
			do
			{
				if (WaitForSingleObject(player_data.hEvent, wt) != WAIT_TIMEOUT)
				{
					request = InterlockedExchange((LPLONG)&player_data.request, 0);
					if (request & (1 << PLAYER_REQUEST_QUIT)) break;
					if (request & (1 << PLAYER_REQUEST_STOP)) break;
					/* ??? THREAD FAULT ??? */
					/* イヤなタイミングでボタンを押すね */
					request |= (1 << PLAYER_REQUEST_STOP);
					break;
				}
				wt = 10;
			} while (mod.outMod->IsPlaying());
		}
#endif
		sequencer->Stop(sequencer);
	} while (0);
	/* finaly */

	sequencer->Term(sequencer);
#if !N_VERSION
#if !defined(MAX_SAMPLERATE)
	if (samp_buf != NULL) free(samp_buf);
#endif
#endif

	if (!request)
	{
		/* WINAMPに終了を通知 */
#define WM_WA_MPEG_EOF (WM_USER + 2)
		PostMessage(mod.hMainWindow, WM_WA_MPEG_EOF, 0, 0);
	}
	else
	{
		UnsubclassWinamp();
	}
	return 0;
}

static CRITICAL_SECTION_OBJECT(playlock);

static int play(char *fn)
{
	SEQUENCER *sequencer;
#if !N_VERSION
	int samplerate, maxlatency, ch;
#endif
	if (player_data.hEvent == NULL) return 1;
	sequencer = loadFile(fn);
	getfileinfo(NULL, NULL, (int *)&player_data.length);
#if !N_VERSION
	if (sequencer == NULL) return 1;
	samplerate = sequencer->GetRate(sequencer);
	ch = sequencer->GetChannel(sequencer);
	maxlatency = mod.outMod->Open(samplerate, ch, 16, 0, 0);
	if (maxlatency < 0)
	{
		if (sequencer != NULL) sequencer->Term(sequencer);
		return 1;
	}
	mod.SetInfo(0, samplerate / 1000, ch, 0);
	mod.SAVSAInit(maxlatency, samplerate);
	mod.VSASetInfo(samplerate, ch);
	mod.outMod->SetVolume(-666);
#endif

	player_data.hThread = CreateThread(NULL, 0, PlayThread, sequencer, 0, &player_data.dwThreadId);
	return 0;
}

static void stop(void)
{
	if (player_data.hThread != NULL)
	{
		do
		{
			PlayerSetEvent(PLAYER_REQUEST_STOP);
		} while (WaitForSingleObject(player_data.hThread, 20) == WAIT_TIMEOUT);
		CloseHandle(player_data.hThread);
		player_data.hThread = NULL;
		mod.outMod->Close();
		mod.SAVSADeInit();
	}
}

#if !N_VERSION
static void pause(void) { player_data.paused = 1; mod.outMod->Pause(1); }
static void unpause(void) { player_data.paused = 0; mod.outMod->Pause(0); }
static int getoutputtime(void) { return mod.outMod->GetOutputTime(); }
static void setvolume(int volume) { mod.outMod->SetVolume(volume); }
static void setpan(int pan) { mod.outMod->SetPan(pan); }
#else
static void pause(void) { PlayerSetEvent(PLAYER_REQUEST_PAUSE); }
static void unpause(void) { PlayerSetEvent(PLAYER_REQUEST_UNPAUSE); }
static int getoutputtime(void) { return player_data.time_pos; }
static void setvolume(int volume) { return; }
static void setpan(int pan) { return; }
#endif

static void config(HWND hwnd) { ConfigSequencer(hwnd); }
static void about(HWND hwnd) { AboutSequencer(hwnd); }
static int infobox(char *fn, HWND hwnd) { return infoBox(fn, hwnd); }
static int isourfile(char *fn) { return isOurFile(fn); }
static void eq_set(int on, char data[10], int preamp) {}
static int ispaused(void) { return player_data.paused; }
static int getlength(void) { return player_data.length; }
static void setoutputtime(int time_in_ms)
{
	player_data.need_seek = time_in_ms;
	PlayerSetEvent(PLAYER_REQUEST_SEEK);
}

static In_Module mod = {
	IN_VER,
	PLUGIN_NAME,
	0,	/* hMainWindow */
	0,	/* hDllInstance */
	0,
	1,	/* is_seekable */
#if !N_VERSION
	1,	/* UsesOutputPlug */
#else
	0,
#endif
	config,
	about,
	init,
	quit,
	getfileinfo,
	infobox,
	isourfile,
	play,
	pause,
	unpause,
	ispaused,
	stop,

	getlength,
	getoutputtime,
	setoutputtime,
	setvolume,
	setpan,

	/* pointers filled in by winamp */
	0,0,0,0,0,0,0,0,0,	/* vis stuff */
	0,0,	/* dsp */

	eq_set,	/* EQ, not used */
	NULL,	/* setinfo */
	0		/* out_mod */
};

In_Module __declspec(dllexport) *winampGetInModule2(void)
{
	mod.FileExtensions = StartWinamp();
	return &mod;
}

