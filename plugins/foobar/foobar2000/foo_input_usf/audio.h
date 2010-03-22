#ifndef _AUDIO_H_
#define _AUDIO_H_

#include <stdint.h>

#define BUFSIZE (16384)

//extern int SampleRate;
#include "audio_hle.h"

#ifdef __cplusplus

class Audio
{
	public:
		Audio(Cpu &);
		~Audio();

		uint32_t COMPILER_CALL AiReadLength(void);
		void COMPILER_CALL AiLenChanged(void);
		void COMPILER_CALL AiDacrateChanged(uint32_t value);
		void OpenSound(void);
		void AddBuffer(void * buf, uint32_t length);

		int32_t audio_ucode_detect ( OSTask_t *task );
		int32_t audio_ucode ( OSTask_t *task );
		double GetTime();

//	protected:
		uint32_t SampleRate;
		float AudioBuffer[BUFSIZE];
		uint32_t BufferPos;
		uint32_t BufferSize;

		uint32_t AudioFirst;

		double IntPlayTime;
		double SeekTime;
		double Time;
		double SilenceCount;

		Cpu & hCpu;
		ABIBase * audioHle;

};

#endif


#endif
