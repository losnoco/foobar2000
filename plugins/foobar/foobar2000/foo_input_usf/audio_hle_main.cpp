
#include "usf.h"
#include "audio_hle.h"

#include "audio.h"
// "Mupen64 HLE RSP plugin v0.2 with Azimers code by Hacktarux"


int32_t Audio::audio_ucode_detect ( OSTask_t *task )
{

	if ( (*(uint8_t*) ( hCpu.N64MEM + task->ucode_data )) != 0x1 )
	{
		if ( (*(uint8_t*) ( hCpu.N64MEM + task->ucode_data )) == 0xF )
			return 4;
		else
			return 3;
	}
	else
	{
		if ( (*(uint32_t*) (hCpu.N64MEM + task->ucode_data + 0x30 )) == 0xF0000F00 ) {
			if ((*(uint32_t*) (hCpu.N64MEM + task->ucode_data + 0x28 )) == 0x1dc8138c)
				return 5; //goldeneye
			else
				return 1;

		}
		else
			return 2;
	}
}

int32_t Audio::audio_ucode ( OSTask_t *task )
{
	uint32_t * p_alist = ( uint32_t* )( task->data_ptr + hCpu.N64MEM );
	uint32_t i;

	if(audioHle == NULL)
	{
		switch ( audio_ucode_detect ( task ) )
		{
			case 1: // mario ucode
				audioHle = new Abi1(hCpu);
				break;
			case 2: // banjo kazooie ucode
				audioHle = new Abi2(hCpu);
				break;
			case 3: // zelda ucode
				audioHle = new Abi3(hCpu);
				break;
			case 5: // goldeneye
				audioHle = new Abi1(hCpu, 1);
				break;
			default:
			{
				return -1;
			}
		}
	}

	for ( i = 0; i < ( task->data_size >> 2 ); i += 2 )
	{
		audioHle->Execute(p_alist[i] >> 24, p_alist[i], p_alist[i + 1]);
	}

	return 0;
}

void ABIBase::Execute(uint32_t Code, uint32_t Inst1, uint32_t Inst2)
{
	inst1 = Inst1;
	inst2 = Inst2;

	(this->*ABI[Code])();
}
