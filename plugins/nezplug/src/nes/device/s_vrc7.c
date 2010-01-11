#include "../nestypes.h"
#include "../audiosys.h"
#include "../handler.h"
#include "../nsf6502.h"
#include "../nsdout.h"
#include "logtable.h"
#include "s_vrc7.h"

#define MASTER_CLOCK        (3579545)

#include "s_opl.h"

static Uint8 usertone_enable[1] = { 0 };
static Uint8 usertone[1][16 * 19];

static Uint32 GetDwordLE(Uint8 *p)
{
	return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}
#define GetDwordLEM(p) (Uint32)((((Uint8 *)p)[0] | (((Uint8 *)p)[1] << 8) | (((Uint8 *)p)[2] << 16) | (((Uint8 *)p)[3] << 24)))

void VRC7SetTone(Uint8 *p, Uint type)
{
	extern void OPLLSetTone(Uint8 *p, Uint32 type);
	switch (type)
	{
		case 1:
			if ((GetDwordLE(p) & 0xf0ffffff) == GetDwordLEM("ILL0"))
				XMEMCPY(usertone[0], p, 16 * 19);
			else
				XMEMCPY(usertone[0], p, 8 * 15);
			usertone_enable[0] = 1;
			break;
		case 2:
			OPLLSetTone(p, 0);
			break;
		case 3:
			OPLLSetTone(p, 1);
			break;
	}
}

typedef struct {
	KMIF_SOUND_DEVICE *kmif;
} OPLLSOUND_INTF;

static OPLLSOUND_INTF sndp = { 0 } ;

static Int32 __fastcall OPLLSoundRender(void)
{
	Int32 b[2] = {0, 0};
	sndp.kmif->synth(sndp.kmif->ctx, b);
	return b[0];
}

static void __fastcall OPLLSoundRender2(Int32 *d)
{
	sndp.kmif->synth(sndp.kmif->ctx, d);
}


static NES_AUDIO_HANDLER s_opll_audio_handler[] = {
	{ 3, OPLLSoundRender, OPLLSoundRender2, }, 
	{ 0, 0, 0, }, 
};

static void __fastcall OPLLSoundVolume(Uint32 volume)
{
	sndp.kmif->volume(sndp.kmif->ctx, volume);
}

static NES_VOLUME_HANDLER s_opll_volume_handler[] = {
	{ OPLLSoundVolume, },
	{ 0, }, 
};

static void __fastcall VRC7SoundReset(void)
{
	if (usertone_enable[0]) sndp.kmif->setinst(sndp.kmif->ctx, 0, usertone[0], 16 * 19);
	sndp.kmif->reset(sndp.kmif->ctx, MASTER_CLOCK, NESAudioFrequencyGet());
}

static NES_RESET_HANDLER s_vrc7_reset_handler[] = {
	{ NES_RESET_SYS_NOMAL, VRC7SoundReset, }, 
	{ 0,                   0, }, 
};

static void __fastcall OPLLSoundTerm(void)
{
	if (sndp.kmif)
	{
		sndp.kmif->release(sndp.kmif->ctx);
		sndp.kmif = 0;
	}
}

static NES_TERMINATE_HANDLER s_opll_terminate_handler[] = {
	{ OPLLSoundTerm, }, 
	{ 0, }, 
};

static void __fastcall OPLLSoundWriteAddr(Uint32 address, Uint32 value)
{
	sndp.kmif->write(sndp.kmif->ctx, 0, value);
}

static void __fastcall OPLLSoundWriteData(Uint32 address, Uint32 value)
{
	sndp.kmif->write(sndp.kmif->ctx, 1, value);
}

static Uint32 __fastcall MSXAUDIOSoundRead(Uint32 address)
{
	return sndp.kmif->read(sndp.kmif->ctx, address);
}


static NES_WRITE_HANDLER s_vrc7_write_handler[] =
{
	{ 0x9010, 0x9010, OPLLSoundWriteAddr, },
	{ 0x9030, 0x9030, OPLLSoundWriteData, },
	{ 0,      0,      0, },
};


void VRC7SoundInstall(void)
{
	LogTableInitialize();
	sndp.kmif = OPLSoundAlloc(OPL_TYPE_VRC7);
	if (sndp.kmif)
	{
		NESAudioHandlerInstall(s_opll_audio_handler);
		NESVolumeHandlerInstall(s_opll_volume_handler);
		NESTerminateHandlerInstall(s_opll_terminate_handler);

		NESResetHandlerInstall(s_vrc7_reset_handler);
		NESWriteHandlerInstall(s_vrc7_write_handler);
	}
}

