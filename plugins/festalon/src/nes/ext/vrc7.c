/*  Festalon - NSF Player
 *  Copyright (C) 2004 Xodnizel
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "common.h"
#include "emu2413.h"

typedef struct {
	OPLL *ym;
	int32 BC;
	uint8 indox;
	int32 divc;
	int32 out;
	NESAPU *gapu;
} VRC7APU;

#include "emu2413.h"

static void DoVRC7Sound(void *private)
{
 VRC7APU *VRC7Sound=private;
 int32 V;

 for(V=VRC7Sound->BC;V<VRC7Sound->gapu->X->timestamp;V++)
 {
  if(!VRC7Sound->divc)
  {
   VRC7Sound->out=(OPLL_calc(VRC7Sound->ym)+(2048*6))<<1;
  }
  VRC7Sound->divc=(VRC7Sound->divc+1)%36;
  VRC7Sound->gapu->WaveHi[V]+=VRC7Sound->out;
 }

 VRC7Sound->BC=VRC7Sound->gapu->X->timestamp;
}

static void VRC7Sync(void *private, int32 ts)
{
 VRC7APU *VRC7Sound=private;
 VRC7Sound->BC=ts;
}

static DECLFW(Mapper85_write)
{
	VRC7APU *VRC7Sound=private;

	A|=(A&8)<<1;
	A&=0xF030;

	if(A==0x9030)
	{
	 DoVRC7Sound(VRC7Sound);
	 OPLL_writeReg(VRC7Sound->ym, VRC7Sound->indox, V);
	}
	else if(A==0x9010)
	 VRC7Sound->indox=V;
}

static void Kill(void *private)
{
 VRC7APU *v7=private;

 if(v7->ym)
  OPLL_delete(v7->ym);
 free(v7);
}

static void Disable(void *private, int mask)
{
	VRC7APU *v7=private;
	OPLL_setMask(v7->ym, mask);
}

int NSFVRC7_Init(EXPSOUND *ep, NESAPU *apu)
{
	VRC7APU *v7;

	v7=malloc(sizeof(VRC7APU));
	memset(v7, 0, sizeof(VRC7APU));

	v7->divc = 0;
	SetWriteHandler(apu->X, 0x9000, 0x9FFF, Mapper85_write, v7);

	//SetWriteHandler(apu->X,0x9010,0x901F,Mapper85_write,v7);
	//SetWriteHandler(apu->X,0x9030,0x903F,Mapper85_write,v7);

	v7->ym=OPLL_new(3579545);
	v7->gapu=apu;
	OPLL_reset(v7->ym);

	ep->private=v7;
        ep->HiFill=DoVRC7Sound;
        ep->HiSync=VRC7Sync;
	ep->Kill=Kill;
	ep->Disable=Disable;
	ep->Channels = 6;

	return(1);
}
