#ifndef _USF_H_
#define _USF_H_
#define _CRT_SECURE_NO_WARNINGS

#include <foobar2000.h>
#include <helpers.h>
#include <shared.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include "cpu.h"


enum
{
	CPU_INVALID,
	CPU_INTERPRETER,
	CPU_RECOMPILER,
	RSP_INTERPRETER,
	RSP_RECOMPILER,
	RSP_AUDIOHLE,
	PLAYTIME_FOREVER,
	PLAYTIME_USEDEFAULTLENGTH,
	PLAYTIME_NORMAL
};

static char * PriStr[] = {"Idle","Lowest","Below Normal","Normal","Above Normal","Highest)","Time Critical"};
static int32_t PriArray[] = {THREAD_PRIORITY_IDLE,THREAD_PRIORITY_LOWEST,THREAD_PRIORITY_BELOW_NORMAL,THREAD_PRIORITY_NORMAL,THREAD_PRIORITY_ABOVE_NORMAL,THREAD_PRIORITY_HIGHEST,THREAD_PRIORITY_TIME_CRITICAL};


#define MAX_FILENAME_LENGTH		511

#define USF_STATUS_LOADED		0x00000001
#define USF_STATUS_PLAYING		0x00000002
#define USF_STATUS_PAUSED		0x00000004
#define USF_STATUS_SEEKING 		0x00000008
#define USF_STATUS_PANIC		0x80000000

#ifdef __cplusplus

class Usf {
	private:
		char file_name[MAX_FILENAME_LENGTH];
	  	uint32_t status;

	public:
		Usf(const char * FileName, abort_callback & p_abort);
		~Usf();
		void LoadSettings(void);
		void ReadFileInfo(file_info & p_info);
		class Cpu * cpu;

		uint32_t rdram_size;
		uint8_t * savestate;
		uint8_t * rom_pages[0x400];
		char * GetTags();
		void SetTags(const char * Tags, t_filestats * end_stats);

		uint32_t CpuMode, RspMode, FadeMode;
		bool EnableCompare, AudioHle, EnableFIFOFull, Seeking;
		bool Fading, Forever, FastSeek, DetectSilence;
		bool PlayForever, PlayDefault;
		double TrackLength, FadeLength, SilenceTime, DefLength;
		double RelVolume;

		uint32_t LoadUSF(char *fn);
		uint32_t PlayUSF(void);
		void Seek(double Time);

		abort_callback & m_abort;
		pfc::string8 libpath8;  //utf8 string containing the path inside an archive where the usf is located for foobar
		t_filestats fileStats;
};

double GetLengthFromString(char * timestring);

#endif

#endif

