#include <stdint.h>
#include "cpu.h"
#include "audio.h"
#include "usf.h"
//#include "audio_hle.h"

#include <stdlib.h>
#include <stdio.h>

#include <stdio.h>
#include <conio.h>
#include <math.h>

void Audio::AddBuffer(void * buf, uint32_t length)
{

	double SilenceDetect = 0.0;

	if(!hCpu.usf.FadeMode || !hCpu.usf.Fading || hCpu.usf.PlayForever || (Time < (hCpu.usf.TrackLength)))
	{

		for(uint32_t InPos = 0; InPos < (length >> 1); InPos += 2)
		{
			AudioBuffer[BufferPos++] = hCpu.usf.RelVolume * (((float) ((int16_t*)buf)[InPos + 1]) / 32768.0);
			AudioBuffer[BufferPos++] = hCpu.usf.RelVolume * (((float) ((int16_t*)buf)[InPos]) / 32768.0);

			SilenceDetect += AudioBuffer[BufferPos - 2];
			SilenceDetect += AudioBuffer[BufferPos - 1];
		}
	}
	else if(Time >= hCpu.usf.TrackLength)
	{
		switch(hCpu.usf.FadeMode)
		{
			case 1:	// linear
				for(uint32_t InPos = 0; InPos < (length >> 1); InPos += 2)
				{
					double LTime = Time + ((double)InPos / ((double)SampleRate * 2.0));
					double FadePos = (LTime - hCpu.usf.TrackLength);
					double Vol = 1.0 - (FadePos / hCpu.usf.FadeLength);

					AudioBuffer[BufferPos++] = hCpu.usf.RelVolume * (Vol * ((float) ((int16_t*)buf)[InPos + 1]) / 32768.0);
					AudioBuffer[BufferPos++] = hCpu.usf.RelVolume * (Vol * ((float) ((int16_t*)buf)[InPos]) / 32768.0);

					SilenceDetect += AudioBuffer[BufferPos - 2];
					SilenceDetect += AudioBuffer[BufferPos - 1];
				}
				break;
			case 2: // log
				for(uint32_t InPos = 0; InPos < (length >> 1); InPos += 2)
				{
					double LTime = Time + ((double)InPos / ((double)SampleRate * 2.0));
					double FadePos = (LTime - hCpu.usf.TrackLength);
					double Vol = 1.0 / pow(10, FadePos / hCpu.usf.FadeLength);

					AudioBuffer[BufferPos++] = hCpu.usf.RelVolume * (Vol * ((float) ((int16_t*)buf)[InPos + 1]) / 32768.0);
					AudioBuffer[BufferPos++] = hCpu.usf.RelVolume * (Vol * ((float) ((int16_t*)buf)[InPos]) / 32768.0);

					SilenceDetect += AudioBuffer[BufferPos - 2];
					SilenceDetect += AudioBuffer[BufferPos - 1];
				}
				break;

		}
	}

	if(SilenceDetect == 0.0)
		SilenceCount += ((double)length / ((double)SampleRate * 4.0));
	else
		SilenceCount = 0.0;

}

double Audio::GetTime()
{
	return Time;
}

void Audio::AiLenChanged(void)
{
	int i = 0, length;
	uint32_t address = (hCpu.AI_DRAM_ADDR_REG & 0x00FFFFF8);

	uint64_t cyc = hCpu.CP0[9] - hCpu.LastCount;
	double ltime = ((double)(hCpu.AI_LEN_REG & 0x3FFF8) / (double)SampleRate) / 4.0;

	/* Calculates CPU use ;) */
	hCpu.CpuPerc = (int32_t)((((double)cyc / ltime) / 187500000.0) * 100.0);
	hCpu.LastCount = 0;//hCpu.CP0[9];
	length = hCpu.AI_LEN_REG & 0x3FFF8;

	Time += ((double)(length >> 2) / (double)SampleRate);

	if(length && !(hCpu.AI_STATUS_REG & 0x80000000))
	{
		const float VSyncTiming = 789000.0f;
		double BytesPerSecond = ((double)hCpu.TimerV / (hCpu.AI_DACRATE_REG + 1)) * 4;
		double CountsPerSecond = (double)((((double)VSyncTiming) * (double)hCpu.TimerHz)) * 2.0;
		double CountsPerByte = (double)CountsPerSecond / (double)BytesPerSecond;
		unsigned int IntScheduled = (unsigned int)((double)hCpu.AI_LEN_REG * CountsPerByte);

		hCpu.ChangeTimer(AiTimer, IntScheduled);
	}

	if(hCpu.usf.EnableFIFOFull)
	{
		if(hCpu.AI_STATUS_REG & 0x40000000)
			hCpu.AI_STATUS_REG |= 0x80000000;
	}

	hCpu.AI_STATUS_REG |= 0x40000000;

	if(hCpu.usf.Seeking)
	{
		if(Time >= SeekTime)
			hCpu.usf.Seeking = false;
	}
	else
	{
		AddBuffer(hCpu.RDRAM + address, length);
	}

}

uint32_t Audio::AiReadLength(void)
{
	hCpu.AI_LEN_REG = 0;
	//return (int)((double)hCpu.AI_LEN_REG * 0.269);
	return 0;
}

void Audio::AiDacrateChanged(uint32_t Value)
{
	hCpu.AI_DACRATE_REG = Value;
	SampleRate = hCpu.TimerV / (hCpu.AI_DACRATE_REG + 1);
}



Audio::Audio(Cpu & cpu) : BufferPos(0), SeekTime(0.0), hCpu(cpu), AudioFirst(0), audioHle(NULL), Time(0.0)
{
	AiDacrateChanged(hCpu.AI_DACRATE_REG);
	SilenceCount = 0.0;
}

Audio::~Audio()
{
	if(audioHle)
		free(audioHle);
}

ABIBase::ABIBase(Cpu & mCpu) : hCpu(mCpu)
{
}
