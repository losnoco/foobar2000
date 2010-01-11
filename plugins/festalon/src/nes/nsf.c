/*  Festalon - NSF Player
 *  Copyright (C) 2004 Xodnizel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../types.h"
#include "../festalon.h"
#include "x6502.h"
#include "sound.h"
#include "cart.h"
#include "nsf.h"
#include "nsfe.h"
#include "../driver.h"

static int NSF_init(FESTALON *fe, FESTALON_NSF *nfe);

static DECLFW(BRAML)
{
        ((uint8 *)private)[A&0x7FF]=V;
}
             
static DECLFR(ARAML)
{
        return(((uint8 *)private)[A&0x7FF]);
}

static DECLFW(NSF_write);

void FESTANSF_Close(FESTALON_NSF *festa)
{
  if(festa->apu) FCEUSND_Kill(festa->apu);

  if(festa->cart) FESTAC_Kill(festa->cart);

  if(festa->NSFDATA) {free(festa->NSFDATA); festa->NSFDATA = 0; }

  if(festa->ExWRAM) free(festa->ExWRAM);

  if(festa->X) X6502_Free(festa->X);

  //free(festa);
  FESTANSF_FreeFileInfo(festa);
}

static INLINE void BANKSET(FESTALON_NSF *fe, uint32 A, uint32 bank)
{
 bank&=fe->NSFMaxBank;
 if(fe->SoundChip&4)
  memcpy(fe->ExWRAM+(A-0x6000),fe->NSFDATA+(bank<<12),4096);
 else 
  setprg4(fe->cart,A,bank);
}

static int LoadNSF(FESTALON *fe, FESTALON_NSF *nfe, uint8 *buf, uint32 size, int info_only);

FESTALON_NSF *FESTANSF_Load(FESTALON *fe, uint8 *buf, uint32 size)
{
  FESTALON_NSF *nfe;

  fe->OutChannels = 1;

  if(!(nfe=malloc(sizeof(FESTALON_NSF))))
   return(0);

  memset(nfe,0,sizeof(FESTALON_NSF));

  if(size >= 5 && !memcmp(buf,"NESM\x1a",5))
  {
   if(!LoadNSF(fe, nfe, buf, size, 0)) { FESTAI_Close(fe); return(0); }
  }
  else if(!memcmp(buf,"NSFE",4))
  {
   if(!LoadNSFE(fe, nfe, buf, size, 0)) { FESTAI_Close(fe); return(0); }
  }
  else
  {
   FESTANSF_Close(nfe);
   return 0;
  }

  if(!NSF_init(fe, nfe))
  {
   return(0);
  }

  return(nfe);
}

/* Note: This function will leave the timing values in "1/1000000th sec ticks" zeroed
   out, even though the NSF specs call for it;  they're redundant, and inaccurate.
*/
uint8 *FESTAI_CreateNSF(FESTALON *fe, uint32 *totalsize)
{
 FESTALON_NSF *nfe = fe->nsf;
 NSF_HEADER NSFHeader;
 uint8 *buffer;

 memset(&NSFHeader, 0, sizeof(NSFHeader));

 memcpy(NSFHeader.ID, "NESM\x1A",5);

 NSFHeader.Version = 0x01;
 NSFHeader.TotalSongs = fe->TotalSongs;
 NSFHeader.StartingSong = fe->StartingSong;

 NSFHeader.LoadAddressLow = nfe->LoadAddr;
 NSFHeader.LoadAddressHigh = nfe->LoadAddr >> 8;

 NSFHeader.InitAddressLow = nfe->InitAddr;
 NSFHeader.InitAddressHigh = nfe->InitAddr >> 8;

 NSFHeader.PlayAddressLow = nfe->PlayAddr;
 NSFHeader.PlayAddressHigh = nfe->PlayAddr >> 8;

 strncpy(NSFHeader.GameName, fe->GameName, 32);
 NSFHeader.GameName[31] = 0;

 strncpy(NSFHeader.Artist, fe->Artist, 32);
 NSFHeader.Artist[31] = 0;

 strncpy(NSFHeader.Copyright, fe->Copyright, 32);
 NSFHeader.Copyright[31] = 0;

 memcpy(NSFHeader.BankSwitch, nfe->BankSwitch, 8);
 
 NSFHeader.VideoSystem = fe->VideoSystem;

 NSFHeader.SoundChip = nfe->SoundChip;

 *totalsize = sizeof(NSFHeader) + nfe->NSFRawDataSize;

 if(!(buffer = malloc(*totalsize)))
  return(0);

 memcpy(buffer, &NSFHeader, sizeof(NSFHeader));
 memcpy(buffer + sizeof(NSFHeader), nfe->NSFRawData, nfe->NSFRawDataSize);

 return(buffer);
}

static int LoadNSF(FESTALON *fe, FESTALON_NSF *nfe, uint8 *buf, uint32 size, int info_only)
{
  NSF_HEADER NSFHeader;

  memcpy(&NSFHeader,buf,0x80);
  buf+=0x80;

  NSFHeader.GameName[31]=NSFHeader.Artist[31]=NSFHeader.Copyright[31]=0;

  fe->GameName = FESTA_FixString(strdup(NSFHeader.GameName));
  fe->Artist = FESTA_FixString(strdup(NSFHeader.Artist));
  fe->Copyright = FESTA_FixString(strdup(NSFHeader.Copyright));

  nfe->LoadAddr=NSFHeader.LoadAddressLow;
  nfe->LoadAddr|=NSFHeader.LoadAddressHigh<<8;

  if(nfe->LoadAddr<0x6000)	/* A buggy NSF... */
  {
   nfe->LoadAddr += 0x8000;
   //return(0);
  }

  nfe->InitAddr=NSFHeader.InitAddressLow;
  nfe->InitAddr|=NSFHeader.InitAddressHigh<<8;

  nfe->PlayAddr=NSFHeader.PlayAddressLow;
  nfe->PlayAddr|=NSFHeader.PlayAddressHigh<<8;

  nfe->NSFSize=size-0x80;

  nfe->NSFMaxBank=((nfe->NSFSize+(nfe->LoadAddr&0xfff)+4095)/4096);
  nfe->NSFMaxBank=uppow2(nfe->NSFMaxBank);

  if(!info_only)
  {
   if(!(nfe->NSFDATA = (uint8 *)malloc(nfe->NSFMaxBank*4096)))
   {
    return 0;
   }
   nfe->NSFRawData = nfe->NSFDATA + (nfe->LoadAddr & 0xFFF);
   nfe->NSFRawDataSize = nfe->NSFSize;

   memset(nfe->NSFDATA,0x00,nfe->NSFMaxBank*4096);
   memcpy(nfe->NSFDATA+(nfe->LoadAddr&0xfff),buf,nfe->NSFSize);

   nfe->NSFMaxBank--;
  }
  else if(info_only == FESTAGFI_TAGS_DATA)
  {
   if(!(nfe->NSFDATA = (uint8 *)malloc(nfe->NSFSize))) 
    return(0);
   nfe->NSFRawData = nfe->NSFDATA;
   nfe->NSFRawDataSize = nfe->NSFSize;
   memcpy(nfe->NSFDATA, buf, nfe->NSFSize);
  }

 fe->VideoSystem = NSFHeader.VideoSystem;

 nfe->SoundChip = NSFHeader.SoundChip;
 fe->TotalSongs = NSFHeader.TotalSongs;
 fe->StartingSong = NSFHeader.StartingSong - 1;
 
 memcpy(nfe->BankSwitch, NSFHeader.BankSwitch, sizeof(NSFHeader.BankSwitch));
 return(1);
}

void FESTANSF_FreeFileInfo(FESTALON_NSF *fe)
{
 if(fe->NSFDATA) free(fe->NSFDATA);
 free(fe);
}

FESTALON_NSF *FESTANSF_GetFileInfo(FESTALON *fe, uint8 *buf, uint32 size, int type)
{
  FESTALON_NSF *nfe;

  if(!type) return(0);	/* Invalid info type. */

  if(!(nfe=malloc(sizeof(FESTALON_NSF))))
   return(0);

  memset(nfe,0,sizeof(FESTALON_NSF));
  if(size >= 5 && !memcmp(buf,"NESM\x1a",5))
  {
   if(!LoadNSF(fe, nfe, buf, size, type)) { FESTANSF_Close(nfe); return(0); }
  }
  else if(!memcmp(buf,"NSFE",4))
  {
   if(!LoadNSFE(fe, nfe, buf, size, type)) { FESTANSF_Close(nfe); return(0); }
  }
  else
  {
   FESTANSF_Close(nfe);
   return 0;
  }
  return(nfe);
}

/* EXPSOUND structure is set by NSF*_Init(), NESAPU structure is already set when these
   functions are called. 
*/

int NSFVRC6_Init(EXPSOUND *, NESAPU *);
int NSFVRC7_Init(EXPSOUND *, NESAPU *);
int NSFAY_Init(EXPSOUND *, NESAPU *);
int NSFMMC5_Init(EXPSOUND *, NESAPU *);
int NSFN106_Init(EXPSOUND *, NESAPU *);
int NSFFDS_Init(EXPSOUND *, NESAPU *);

static int NSF_init(FESTALON *fe, FESTALON_NSF *nfe)
{
  NESCART *ca;
  int x;

  if((fe->VideoSystem & 0x3) == 0)
   nfe->PAL=0;
  else if((fe->VideoSystem & 0x3) == 1)
   nfe->PAL=1;

  if(nfe->SoundChip&4)
   nfe->ExWRAM=malloc(32768+8192);
  else
   nfe->ExWRAM=malloc(8192);

  if(!(nfe->X=X6502_Init(nfe->RAM,nfe->PAL,nfe)))
  {
   FESTAI_Close(fe);
   return(0);
  }

  if(!(nfe->apu=FCEUSND_Init(nfe->X)))
  {
   FESTAI_Close(fe);
   return(0);
  }

  X6502_Power(nfe->X);
  FCEUSND_Power(nfe->apu);

  SetReadHandler(nfe->X,0x0000,0x1FFF,ARAML,nfe->RAM);
  SetWriteHandler(nfe->X,0x0000,0x1FFF,BRAML,nfe->RAM);

  nfe->doreset=1;

  nfe->cart=ca=FESTAC_Init();

  if(nfe->SoundChip&4)
  {
   FESTAC_SetupPRG(ca,0,nfe->ExWRAM,32768+8192,1);
   setprg32(ca,0x6000,0);
   setprg8(ca,0xE000,4);
   memset(nfe->ExWRAM,0x00,32768+8192);
   SetWriteHandler(nfe->X,0x6000,0xDFFF,CartBW,ca);
   SetReadHandler(nfe->X,0x6000,0xFFFF,CartBR,ca);
  }
  else
  {
   memset(nfe->ExWRAM,0x00,8192);
   SetReadHandler(nfe->X,0x6000,0x7FFF,CartBR,ca);
   SetWriteHandler(nfe->X,0x6000,0x7FFF,CartBW,ca);

   FESTAC_SetupPRG(ca,0,nfe->NSFDATA,((nfe->NSFMaxBank+1)*4096),0);
   FESTAC_SetupPRG(ca,1,nfe->ExWRAM,8192,1);

   setprg8r(ca,1,0x6000,0);
   SetReadHandler(nfe->X,0x8000,0xFFFF,CartBR,ca);
  }

  nfe->BSon=0;
  for(x=0;x<8;x++)
   nfe->BSon|=nfe->BankSwitch[x];

  if(!nfe->BSon)
  {
   int32 x;
   for(x=(nfe->LoadAddr&0xF000);x<0x10000;x+=0x1000)
   {
    BANKSET(nfe, x,((x-(nfe->LoadAddr&0xF000))>>12));
   }
  }

  SetWriteHandler(nfe->X,0x2000,0x3ffF,0,0);
  SetReadHandler(nfe->X,0x2000,0x3FFF,0,0);

  SetWriteHandler(nfe->X,0x5ff6,0x5fff,NSF_write,nfe);

  SetWriteHandler(nfe->X,0x3ff0,0x3fff,NSF_write,nfe);

  
  /* We don't support expansion sound chips in PAL mode.
     It would be EL BUTTO PAINO to do so, and probably slow, since
     it would require having two resamplers going at once(on some chips; Festalon
     takes advantage of the fact that chips like the VRC7 are run at a clock speed
     "compatible" with the NES' CPU clock speed, as far as resampling is concerned).
  */
 
  fe->TotalChannels = 5;

  if(!nfe->PAL)
  {
   int (*EXPInit[6])(EXPSOUND *, NESAPU *) =
	{NSFVRC6_Init,NSFVRC7_Init,NSFFDS_Init,NSFMMC5_Init,NSFN106_Init,NSFAY_Init};
   int x;

   for(x=0;x<6;x++)
    if(nfe->SoundChip&(1<<x)) 
    {
     EXPSOUND *ep;

     if(!(ep=malloc(sizeof(EXPSOUND)))) continue;	/* FIXME?  Error out? */
     memset(ep,0,sizeof(EXPSOUND));

     if( EXPInit[x](ep,nfe->apu) )			/* FIXME?  Error out? */
      FCEUSND_AddExp(nfe->apu,ep);    
     fe->TotalChannels += ep->Channels;
    }
  }

  nfe->CurrentSong = fe->CurrentSong=fe->StartingSong;
  nfe->SongReload=0xFF;

  return(1);
}

static DECLFW(NSF_write)
{
 FESTALON_NSF *fe=private;

 if(A >= 0x5ff6 && A<= 0x5FFF)	/* Bank-switching */
 {
  if(A <= 0x5ff7 && !(fe->SoundChip&4)) return; /* Only valid in FDS mode */
  if(!fe->BSon) return;
  A&=0xF; 
  BANKSET(fe, (A*4096),V);
 }
}

static void CLRI(FESTALON_NSF *fe)
{
	int x;

             memset(fe->RAM,0x00,0x800);

	     X6502_DMW(fe->X,0x4015,0x00);

             for(x=0;x<0x14;x++)
              X6502_DMW(fe->X,0x4000+x,0);
             X6502_DMW(fe->X,0x4015,0xF);

	     if(fe->SoundChip&4) 
	     {
	      X6502_DMW(fe->X,0x4017,0xC0);	/* FDS BIOS writes $C0 */
	      X6502_DMW(fe->X,0x4089,0x80);
	      X6502_DMW(fe->X,0x408A,0xE8);
	     }
	     else 
	     {
	      memset(fe->ExWRAM,0x00,8192);
	      X6502_DMW(fe->X,0x4017,0xC0);
              X6502_DMW(fe->X,0x4017,0xC0);
              X6502_DMW(fe->X,0x4017,0x40);
	     }
             if(fe->BSon)
             {
              for(x=0;x<8;x++)
	      {
	       if((fe->SoundChip&4) && x>=6)
	        BANKSET(fe,0x6000+(x-6)*4096,fe->BankSwitch[x]);
	       BANKSET(fe, 0x8000+x*4096,fe->BankSwitch[x]);
	      }
             }
}

void FESTANSF_SongControl(FESTALON_NSF *nfe, int which)
{
 nfe->CurrentSong = which;
 nfe->SongReload=0xFF;
}

float *FESTANSF_Emulate(FESTALON_NSF *nfe, int *Count)
{
 // Reset the stack if we're going to call the play routine or the init routine.
 if(nfe->X->PC == 0x3800 || nfe->SongReload)
 {
  if(nfe->SongReload) CLRI(nfe);

  nfe->X->S = 0xFD;
  nfe->RAM[0x1FF] = 0x37;
  nfe->RAM[0x1FE] = 0xff;
  nfe->X->X = nfe->X->Y = nfe->X->A = 0;
  if(nfe->SongReload)
  {
   nfe->X->P = 4;
   nfe->X->X = nfe->PAL;
   nfe->X->A = nfe->CurrentSong;
   nfe->X->PC = nfe->InitAddr;
   nfe->SongReload = 0;
  }
  else
  {
   nfe->X->PC = nfe->PlayAddr;
  }
 }

 if(nfe->PAL)
  X6502_Run(nfe->X, 312*(256+85)-nfe->doodoo);
 else
  X6502_Run(nfe->X, 262*(256+85)-nfe->doodoo);
 nfe->doodoo^=1;

 X6502_SpeedHack(nfe->X,nfe->apu);

 *Count=FCEUSND_FlushEmulate(nfe->apu);
 return(nfe->apu->WaveFinal);
}

void FESTANSF_Disable(FESTALON_NSF *fe, int t)
{
 FCEUSND_Disable(fe->apu, t);
} 
