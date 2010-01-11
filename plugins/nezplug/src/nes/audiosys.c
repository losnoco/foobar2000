#include "audiosys.h"

/* ---------------------- */
/*  Audio Render Handler  */
/* ---------------------- */

#define SHIFT_BITS 8

static Uint frequency = 44100;
static Uint channel = 1;

static NES_AUDIO_HANDLER *nah = 0;
static NES_VOLUME_HANDLER *nvh = 0;

static Uint naf_type = NES_AUDIO_FILTER_LOWPASS;
static Uint32 naf_prev[2] = { 0x8000, 0x8000 };
void NESAudioFilterSet(Uint filter)
{
	naf_type = filter;
	naf_prev[0] = 0x8000;
	naf_prev[1] = 0x8000;
}

void NESAudioRender(Int16 *bufp, Uint buflen)
{
	Uint maxch = NESAudioChannelGet();
	while (buflen--)
	{
		NES_AUDIO_HANDLER *ph;
		Int32 accum[2] = { 0, 0 };
		Uint ch;

		for (ph = nah; ph; ph = ph->next)
		{
			if (!(ph->fMode & 1) || bufp)
			{
				if (channel == 2 && ph->fMode & 2)
				{
					ph->Proc2(accum);
				}
				else
				{
					Int32 devout = ph->Proc();
					accum[0] += devout;
					accum[1] += devout;
				}
			}
		}

		if (bufp)
		{
			for (ch = 0; ch < maxch; ch++)
			{
				Uint32 output[2];
				accum[ch] += (0x8000 << SHIFT_BITS);
				if (accum[ch] < 0)
					output[ch] = 0;
				else if (accum[ch] > (0x10000 << SHIFT_BITS) - 1)
					output[ch] = (0x10000 << SHIFT_BITS) - 1;
				else
					output[ch] = accum[ch];
				output[ch] >>= SHIFT_BITS;
				switch (naf_type)
				{
					case NES_AUDIO_FILTER_LOWPASS:
						{
							Uint32 prev = naf_prev[ch];
							naf_prev[ch] = output[ch];
							output[ch] = (output[ch] + prev) >> 1;
						}
						break;
					case NES_AUDIO_FILTER_WEIGHTED:
						{
							Uint32 prev = naf_prev[ch];
							naf_prev[ch] = output[ch];
							output[ch] = (output[ch] + output[ch] + output[ch] + prev) >> 2;
						}
						break;
				}
				*bufp++ = ((Int32)output[ch]) - 0x8000;
			}
		}
	}
}

void NESVolume(Uint volume)
{
	NES_VOLUME_HANDLER *ph;
	for (ph = nvh; ph; ph = ph->next) ph->Proc(volume);
}

static void NESAudioHandlerInstallOne(NES_AUDIO_HANDLER *ph)
{
	/* Add to tail of list*/
	ph->next = 0;
	if (nah)
	{
		NES_AUDIO_HANDLER *p = nah;
		while (p->next) p = p->next;
		p->next = ph;
	}
	else
	{
		nah = ph;
	}
}
void NESAudioHandlerInstall(NES_AUDIO_HANDLER *ph)
{
	for (;(ph->fMode&2)?(!!ph->Proc2):(!!ph->Proc);ph++) NESAudioHandlerInstallOne(ph);
}
void NESVolumeHandlerInstall(NES_VOLUME_HANDLER *ph)
{
	for (;ph->Proc;ph++)
	{
		/* Add to top of list*/
		ph->next = nvh;
		nvh = ph;
	}
}

void NESAudioHandlerInitialize(void)
{
	nah = 0;
	nvh = 0;
}

void NESAudioFrequencySet(Uint freq)
{
	frequency = freq;
}
Uint NESAudioFrequencyGet(void)
{
	return frequency;
}

void NESAudioChannelSet(Uint ch)
{
	channel = ch;
}
Uint NESAudioChannelGet(void)
{
	return channel;
}
