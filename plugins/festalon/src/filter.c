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

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <samplerate.h>

#include "types.h"

#include "filter.h"

#include "fcoeffs.h"

#include "cputest.h"

static void SexyFilter(FESTAFILT *ff, float *in, float *out, int32 count)
{
 double mul1,mul2,vmul;

 mul1=(double)94/ff->rate;
 mul2=(double)24/ff->rate;
 vmul=(double)ff->SoundVolume*3/2/100;

 while(count)
 {    
  double ino=vmul * *in;
  ff->acc1+=((ino-ff->acc1)*mul1);
  ff->acc2+=((ino-ff->acc1-ff->acc2)*mul2);
  {   
   float t=(ff->acc1-ino+ff->acc2);
   
   t += 32767;
   t /= 65535;
   if(t < 0.0) t = 0.0;
   if(t > 1.0) t = 1.0;
   //if(t>32767 || t<-32768) printf("Flow: %d\n",t);
   //if(t>32767) t=32767;
   //if(t<-32768) t=-32768;   
   *out=t;
  }  
  in++;  
  out++;  
  count--; 
 }
}

/* Returns number of samples written to out. */
/* leftover is set to the number of samples that need to be copied 
   from the end of in to the beginning of in.
*/

/* This filtering code assumes that almost all input values stay below 32767.
   Do not adjust the volume in the wlookup tables and the expansion sound
   code to be higher, or you *might* overflow the FIR code.
*/
int32 FESTAFILT_Do(FESTAFILT *ff, float *in, float *out, uint32 maxoutlen, uint32 inlen, int32 *leftover, int sinput)
{
	uint32 x;
	int32 max;
	int32 count=0;
	float *flout = ff->boobuf;

	max = inlen & ~0x1F;
	max -= NCOEFFS;
	if(max < 0) max = 0;

	#ifdef ARCH_X86

	#ifdef __x86_64__
	if(ff->cpuext & MM_MMX)
	{
         uint32 bigcount = max / ff->mrratio;
         if(sinput)
         {
          #define FILTMMX_SKIP_ADD_STR "skippy"
          #define FILTMMX_SKIP_ADD
          #include "filter-amd64-mmx.h"
          #undef FILTMMX_SKIP_ADD
          #undef FILTMMX_SKIP_ADD_STR
         }
         else
         {
          #define FILTMMX_SKIP_ADD_STR ""
          #include "filter-amd64-mmx.h"
          #undef FILTMMX_SKIP_ADD_STR
         }
	}
	#else
	if(ff->cpuext & MM_SSE3)
	{
	 #define TMP_FSSE3
	 #include "filter-sse.h"
	 #undef TMP_FSSE3
	}
	else if(ff->cpuext & MM_3DNOW)
	{
		#include "filter-3dnow.h"
	}
	else if(ff->cpuext & MM_SSE)
	{
		#include "filter-sse.h"
	}
	else if(ff->cpuext & MM_MMX)
	{
 	 uint32 bigcount = max / ff->mrratio;
	 if(sinput)
	 {
	  #define FILTMMX_SKIP_ADD_STR "skippy"
	  #define FILTMMX_SKIP_ADD
	  #include "filter-mmx.h"
	  #undef FILTMMX_SKIP_ADD
	  #undef FILTMMX_SKIP_ADD_STR
	 }
	 else
	 {
	  #define FILTMMX_SKIP_ADD_STR ""
	  #include "filter-mmx.h"
	  #undef FILTMMX_SKIP_ADD_STR
	 }
	}
	#endif
	else
	#elif ARCH_POWERPC
	if(ff->cpuext & MM_ALTIVEC)
	{
	 static unsigned int ld_vecsr14[4] __attribute__ ((aligned (16))) = {14,14,14,14};
	 vector unsigned int vecsr14 = vec_ld(0, ld_vecsr14);

	 for(x=0;x<max;x+=ff->mrratio)
	 {
	  vector signed int acc;
	  signed int acc_store[4];
	  signed short *wave,*coeffs;
	  unsigned int c;
	  
	  acc = vec_xor(acc, acc);
	  
	  coeffs = ff->coeffs_i16;
	  wave = (int16 *)in + x;

	  for(c=0;c<NCOEFFS;c+=8)
	  {
	   vector signed short wd, fd;
	   wd = vec_ld(c, wave);
	   fd = vec_ld(c, coeffs);
	   
	   acc = vec_msums(wd, fd, acc);
	  }
	  vec_st(vec_sra(acc, vecsr14), 0, acc_store);	// Shifting right might not be necessary at this point...
	  *flout = (float)((acc_store[0] + acc_store[1] + acc_store[2] + acc_store[3]) >> 2);

	  if(!sinput)
	   *flout += 32767;
	  flout++;
	  count++;
	 }
	}
	else
	#endif
        for(x=0;x<max;x+=ff->mrratio)
        {
         float acc = 0;
         unsigned int c;
         float *wave;
         float *coeffs;

         for(c=0,wave=(float *)&in[x],coeffs=ff->coeffs;c<NCOEFFS;c+=2)
         {
          acc+=wave[c] * coeffs[c];
          acc+= (wave+1)[c] * (coeffs+1)[c];
         }
	 *flout = acc;
	 flout++;
	 count++;
	}

        *leftover=inlen - max;

	count = max / ff->mrratio;
	if(ff->fidbuf)
	 ff->fidfuncp(ff->fidbuf, ff->boobuf, max / ff->mrratio);
	{
	 SRC_DATA doot;
	 int error;

	 doot.data_in = ff->boobuf;
	 doot.data_out = out;
	 doot.input_frames = count;
	 doot.output_frames = maxoutlen;
	 doot.src_ratio = ff->lrhfactor;
	 doot.end_of_input = 0;

	 if((error=src_process(ff->lrh, &doot)))
	 {
	  //printf("Eeeek:  %s, %d, %d\n",src_strerror(error),ff->boobuf,out);
	  //exit(1);
	 }

	 //if(doot.input_frames_used - count) exit(1);
	 //printf("Oops: %d\n\n", doot.input_frames_used - count);

	 //printf("%d\n",doot.output_frames_gen);
	 SexyFilter(ff, out, out, doot.output_frames_gen);
 	 return(doot.output_frames_gen);
	}
}

static void FreeFid(FESTAFILT *ff)
{
 if(ff->fidbuf)
 {
  fid_run_freebuf(ff->fidbuf);
  ff->fidbuf = 0;
 }

 if(ff->fidrun)
 {
  fid_run_free(ff->fidrun);
  ff->fidrun = 0;
 }

 if(ff->fid)
 {
  free(ff->fid);
  ff->fid = 0;
 }
}

void FESTAFILT_Kill(FESTAFILT *ff)
{
 if(ff->lrh)
  src_delete(ff->lrh);

 FreeFid(ff);
 free(ff->realmem);
}

FESTAFILT * FESTAFILT_Init(int32 rate, double cpuclock, int PAL, int soundq)
{
 double *tabs[2]={COEF_NTSC,COEF_PAL};
 double *tabs2[2]={COEF_NTSC_HI,COEF_PAL_HI};
 double *tmp;
 int32 x;
 uint32 nco;
 uint32 div;
 FESTAFILT *ff;
 void *realmem;
 int srctype;
 int error;

 if(!(realmem=ff=malloc(16 + sizeof(FESTAFILT)))) return(0);

 ff = (FESTAFILT *)(unsigned long)(((unsigned long)ff+0xFLL)&~0xFLL);
 memset(ff,0,sizeof(FESTAFILT));
 ff->realmem = realmem;
 ff->soundq=soundq;

 nco=NCOEFFS;

 if(soundq)
 {
  tmp=tabs2[(PAL?1:0)];
  div = 16;
  srctype = SRC_SINC_BEST_QUALITY;
 }
 else
 {
  tmp=tabs[(PAL?1:0)];
  div = 32;
  srctype = SRC_SINC_FASTEST;
 }

 ff->mrratio=div;
 //int max=0;
 for(x=0;x<NCOEFFS>>1;x++)
 {
  ff->coeffs_i16[x]=ff->coeffs_i16[NCOEFFS-1-x]=tmp[x] * 65536;
  ff->coeffs[x]=ff->coeffs[NCOEFFS-1-x]=(float)tmp[x];
  //if(abs(ff->coeffs_i16[x]) > abs(max)) max = abs(ff->coeffs_i16[x]);
 }
 ff->rate=rate;
 
 ff->imrate = cpuclock / div;
 ff->lrhfactor = rate / ff->imrate;

 ff->lrh = src_new(srctype, 1, &error);


 ff->cpuext = ac_mmflag();

 ff->input_format = FFI_FLOAT;

 //ff->cpuext &= MM_MMX;

 if(ff->cpuext & (MM_SSE3 | MM_SSE | MM_3DNOW))
 {

 }
 else if(ff->cpuext & MM_MMX)
  ff->input_format = FFI_INT16;
 else if(ff->cpuext & MM_ALTIVEC)
  ff->input_format = FFI_INT16;

 return(ff);
}

int FESTAFILT_SetLowpass(FESTAFILT *ff, int on, uint32 corner, uint32 order)
{
 //FESTAFILT *ff =  fe->apu->ff;
 char spec[256];
 char *speca;
 char *err;

 FreeFid(ff);

 //on = 1;
 if(on)
 {
  //snprintf(spec,256,"HpBuZ%d/%d",2,1000);
#ifdef _MSC_VER
  _snprintf
#else
  snprintf
#endif
	  (spec,256,"LpBuZ%d/%d",order,corner);
  speca = spec;
  //printf("%s\n",spec);
  // p = "LpBuZ2/5000";
  //printf("%f, %s\n",ff->imrate,spec);
  err = fid_parse(ff->imrate, &speca, &ff->fid);

  if(!err)
  {
   if(!(ff->fidrun = fid_run_new(ff->fid, &ff->fidfuncp)))
    {FreeFid(ff); return(0); }
   if(!(ff->fidbuf = fid_run_newbuf(ff->fidrun)))
    {FreeFid(ff); return(0); }
   return(1);
  }
  else
  {
   ff->fid = 0;
   return(0);
  }
 }
 return(1);
}
	

