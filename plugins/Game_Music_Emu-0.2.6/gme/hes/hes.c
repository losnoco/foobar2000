#include "shared.h"
#include <string.h>
#include <stdlib.h>

static uint16 De16(uint8 *buf)
{
 return(buf[0] | (buf[1] << 8)); 
}

static uint32 De32(uint8 *buf)
{
 return(buf[0] | (buf[1]<<8) | (buf[2]<<16) | (buf[3] << 24));
}

FESTALON_HES *FESTAHES_Load(uint8 *buf, uint32 size)
{
 FESTALON_HES *hes;
 uint32 LoadAddr,LoadSize;
 uint16 InitAddr;
 uint8 *tmp;
 int x;

 hes = malloc(sizeof(FESTALON_HES));
 hes->h6280 = malloc(sizeof(h6280_Regs));

 InitAddr = De16(&buf[0x6]);

 tmp = &buf[0x10];

 while(tmp < (buf + size - 0x10))
 {
  LoadSize = De32(&tmp[0x4]);
  LoadAddr = De32(&tmp[0x8]);
  //printf("%08x:%08x\n",LoadSize,LoadAddr);
  tmp += 0x10;
  if(tmp >= (buf + size + LoadSize)) break;

  if((LoadAddr + LoadSize) > 0x100000) break;
  memcpy(hes->rom + LoadAddr,tmp,LoadSize);
  tmp += LoadSize;
 }

 system_init(hes);
 system_reset(hes);

 //fe->CurrentSong = fe->StartingSong = buf[5];
 //fe->TotalSongs = 256;

 hes->IBP[0x1C00] = 0x20;
 hes->IBP[0x1C01] = InitAddr;
 hes->IBP[0x1C02] = InitAddr >> 8;
 hes->IBP[0x1C03] = 0x4C;
 hes->IBP[0x1C04] = 0x03;
 hes->IBP[0x1C05] = 0x1C;

 //printf("Init: %04x, %04x\n",InitAddr,h6280_get_reg(hes->h6280, H6280_PC));
 h6280_set_reg(hes->h6280, H6280_A, 0/*fe->CurrentSong*/);
 h6280_set_reg(hes->h6280, H6280_PC, 0x1C00);

 for(x=0;x<8;x++)
 {
  hes->mpr_start[x] = buf[0x8 + x];
  h6280_set_reg (hes->h6280, H6280_MPR0 + x, hes->mpr_start[x]);
  bank_set(hes, x, hes->mpr_start[x]);
 } 
 return(hes);
}

void FESTAHES_SongControl(FESTALON_HES *hes, int which)
{
 int x;

 system_reset(hes);
 h6280_set_reg(hes->h6280, H6280_A, which);
 h6280_set_reg(hes->h6280, H6280_PC, 0x1C00);

 for(x=0;x<8;x++)
 {
  h6280_set_reg (hes->h6280, H6280_MPR0 + x, hes->mpr_start[x]);
  bank_set(hes, x, hes->mpr_start[x]);
 }
}



void FESTAHES_Close(FESTALON_HES *hes)
{
  //system_shutdown(hes);

  if(hes->h6280) free(hes->h6280);
  free(hes);
}

