#include <string.h>
#define _XOPEN_SOURCE 600
#include <stdlib.h>

#include "types.h"
#include "festalon.h"
#include "nes/nsf.h"
#include "pce/hes.h"

#include "driver.h"

uint32 uppow2(uint32 n)
{
 int x;

 for(x=31;x>=0;x--)
  if(n&(1<<x))
  {
   if((1<<x)!=n)
    return(1<<(x+1));
   break;
  }
 return n;
}

char *FESTA_FixString(char *str)
{
 char *orig = str;
 if(str)
  while(*str)
  {
   if(*str < 0x20) *str = 0x20;
   str++;
  }
 return(orig);
}


float *FESTAI_Emulate(FESTALON *fe, int *Count)
{
 if(fe->nsf)
  return(FESTANSF_Emulate(fe->nsf, Count));
 else if(fe->hes)
  return(FESTAHES_Emulate(fe->hes, Count));

 return(0);
}

void FESTAI_Disable(FESTALON *fe, int t)
{
 if(fe->nsf)
  FESTANSF_Disable(fe->nsf, t);
 else if(fe->hes)
  FESTAHES_Disable(fe->hes, t);
}

static void FFI(FESTALON *fe)
{
 int x;

 if(fe->GameName) free(fe->GameName);
 if(fe->Artist) free(fe->Artist);
 if(fe->Copyright) free(fe->Copyright);
 if(fe->Ripper) free(fe->Ripper);
 if(fe->SongNames)
 {
  for(x=0;x<fe->TotalSongs;x++)
   if(fe->SongNames[x])
    free(fe->SongNames[x]);
  free(fe->SongNames);
 }
 if(fe->SongLengths)
  free(fe->SongLengths);
 if(fe->SongFades)
  free(fe->SongFades);
 if(fe->Playlist)
  free(fe->Playlist);
}

void FESTAI_FreeFileInfo(FESTALON *fe)
{
 if(fe->nsf)
  FESTANSF_FreeFileInfo(fe->nsf);
 FFI(fe);
 free(fe);
}


FESTALON *FESTAI_Load(uint8 *buf, uint32 size)
{
 FESTALON *fe;

 fe = malloc(sizeof(FESTALON));
 memset(fe, 0, sizeof(FESTALON));

 if(!memcmp(buf,"HESM",4))
 {
  if(!(fe->hes = FESTAHES_Load(fe, buf, size)))
  {
   free(fe);
   return(0);
  }
 }
 else
 {
  if(!(fe->nsf = FESTANSF_Load(fe, buf, size)))
  {
   free(fe);
   return(0);
  }
 }
 return(fe);
}


void FESTAI_Close(FESTALON *fe)
{
 FFI(fe);
 if(fe->nsf)
  FESTANSF_Close(fe->nsf);
 else if(fe->hes)
  FESTAHES_Close(fe->hes);
 free(fe);
}

int32 FESTAI_SetVolume(FESTALON *fe, int32 volume)
{
 if(volume > 999) volume = 999;
 if(volume < -999) volume = -999;

 if(fe->nsf)
  FESTANSF_SetVolume(fe->nsf, volume);
 else if(fe->hes)
  FESTAHES_SetVolume(fe->hes, volume);

 return(volume);
}

int FESTAI_SetLowpass(FESTALON *fe, int on, uint32 corner, uint32 order)
{
 if(fe->nsf)
  return(FESTANSF_SetLowpass(fe->nsf, on, corner, order));
 else if(fe->hes)
  return(FESTAHES_SetLowpass(fe->hes, on, corner, order));

 return(0);
}


int FESTAI_SongControl(FESTALON *fe, int z, int o)
{
 if(o)
  fe->CurrentSong=z;
 else
  fe->CurrentSong+=z;

 if(fe->CurrentSong<0) fe->CurrentSong=0;
 else if(fe->CurrentSong>=fe->TotalSongs) fe->CurrentSong=fe->TotalSongs - 1;


 if(fe->nsf)
  FESTANSF_SongControl(fe->nsf, fe->CurrentSong);
 else if(fe->hes)
  FESTAHES_SongControl(fe->hes, fe->CurrentSong);
 return(fe->CurrentSong);
}

int FESTAI_SetSound(FESTALON *fe, uint32 rate, int quality)
{
 if(fe->nsf)
  return(FESTANSF_SetSound(fe->nsf, rate, quality));
 else if(fe->hes)
  return(FESTAHES_SetSound(fe->hes, rate, quality));

 return(0);
}


void *FESTA_malloc(uint32 align, uint32 total)
{
 void *ret;

 #if HAVE_POSIX_MEMALIGN
 if(align < sizeof(int *))
	 align = sizeof(int *);

 if(posix_memalign(&ret, align, total))
  return(0);
 #else	/* Eh, we'll try our best!  Assumes that memory has already been aligned to a sizeof(void *) boundary by malloc. */
 void *real;
 void *tmp;
 int t;

 if(align < sizeof(int *))
	 align = sizeof(int *);

 t = align / sizeof(int *);

 if(t * sizeof(int *) != align)        // Erhm...           
  return(0);

 real = malloc(total + align);

 tmp = real;

 do
 {
  memcpy(tmp, &real, sizeof(int *));
  ((char *)tmp) += sizeof(int *);
 } while((unsigned int)tmp & (align - 1));
 ret = tmp;
 //printf("%08x:%08x\n",real,ret);
 #endif
 return(ret);
}

void FESTA_free(void *m)
{
 #if HAVE_POSIX_MEMALIGN
 free(m);
 #else
 void *meep;
 ((char*)m)-=sizeof(int*);
 memcpy(&meep, m, sizeof(int *));
 //printf("Meep: %08x\n",meep);
 free(meep);
 #endif
}


FESTALON *FESTAI_GetFileInfo(uint8 *buf, uint32 size, int type)
{
  FESTALON *ret;

  ret = malloc(sizeof(FESTALON));
  memset(ret, 0, sizeof(FESTALON));

  if(!memcmp(buf,"HESM",4))
  {
   
  }
  else
  {
   ret->nsf = FESTANSF_GetFileInfo(ret, buf, size, type);
  }
  return(ret);
}
