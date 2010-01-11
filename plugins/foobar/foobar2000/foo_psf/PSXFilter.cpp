//////////////////////////////////////////////////////////////////////

#include "PSXFilter.h"

#include <math.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//
// this algorithm is scale-sensitive... hurr
//
#define COEF_16BIT_TO_FLOAT (0.000030517578125f)
#define COEF_FLOAT_TO_16BIT (32768.0f)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPSXFilter::CPSXFilter()
{
}

CPSXFilter::~CPSXFilter()
{
}

void CPSXFilter::Reset()
{
  lx1=0;
  lx2=0;
  ly1=0;
  ly2=0;
  hx1[0]=0;
  hx2[0]=0;
  hy1[0]=0;
  hy2[0]=0;
  hx1[1]=0;
  hx2[1]=0;
  hy1[1]=0;
  hy2[1]=0;
}

void CPSXFilter::Redesign(int nSampleRate)
{
// 150 and 4 is about spot-on for low

if(nSampleRate==48000) {
// LOW SHELF sample=48000.000000 freq=150.000000 db=4.000000
la0=1.00320890889339290000;
la1=-1.97516434134506300000;
la2=0.97243484967313087000;
lb1=-1.97525280404731810000;
lb2=0.97555529586426892000;
// HIGH SHELF sample=48000.000000 freq=6000.000000 db=-5.000000
ha0=1.52690772687271160000;
ha1=-1.62653918974914990000;
ha2=0.57997976029249387000;
hb1=-0.80955590379048203000;
hb2=0.28990420120653748000;
} else {
// LOW SHELF sample=44100.000000 freq=150.000000 db=4.000000
la0=1.00349314378906680000;
la1=-1.97295980267143170000;
la2=0.97003400595243994000;
lb1=-1.97306449030610280000;
lb2=0.97342246210683581000;
// HIGH SHELF sample=44100.000000 freq=6000.000000 db=-5.000000
ha0=1.50796284998687450000;
ha1=-1.48628361940858910000;
ha2=0.52606706092412581000;
hb1=-0.71593574211151134000;
hb2=0.26368203361392234000;
}

  Reset();
}

#define EPSILON (1e-10f)

void CPSXFilter::Process(float *stereobuffer, int nSamples)
{
#define OVERALL_SCALE (0.87f)

  int i;

  //
  // underflow protection
  //
  // still possible to underflow if you request a lot of samples
  // and the input is digitally silent.  but assuming reasonable
  // numbers like 576 at a time, this should be fine
  //
  if(fabsf(lx1) < EPSILON) lx1 = 0;
  if(fabsf(lx2) < EPSILON) lx2 = 0;
  if(fabsf(ly1) < EPSILON) ly1 = 0;
  if(fabsf(ly2) < EPSILON) ly2 = 0;
  if(fabsf(hx1[0]) < EPSILON) hx1[0] = 0;
  if(fabsf(hx2[0]) < EPSILON) hx2[0] = 0;
  if(fabsf(hy1[0]) < EPSILON) hy1[0] = 0;
  if(fabsf(hy2[0]) < EPSILON) hy2[0] = 0;
  if(fabsf(hx1[1]) < EPSILON) hx1[1] = 0;
  if(fabsf(hx2[1]) < EPSILON) hx2[1] = 0;
  if(fabsf(hy1[1]) < EPSILON) hy1[1] = 0;
  if(fabsf(hy2[1]) < EPSILON) hy2[1] = 0;

  for (i = 0; i < nSamples; i++) {
    float in, out;
    float l, r;
    l = stereobuffer[0] * COEF_FLOAT_TO_16BIT;
    r = stereobuffer[1] * COEF_FLOAT_TO_16BIT;

    float mid  = l+r;
    float side = l-r;
    //
    // Low shelf
    //
    in = mid;
    out = la0 * in + la1 * lx1 + la2 * lx2 - lb1 * ly1 - lb2 * ly2;
    lx2 = lx1; lx1 = in;
    ly2 = ly1; ly1 = out;
    mid = out;

    l = ((0.5f)*(OVERALL_SCALE))*(mid + side);
    r = ((0.5f)*(OVERALL_SCALE))*(mid - side);

    //
    // High shelf
    //
    in = l;
    out = ha0 * in + ha1 * hx1[0] + ha2 * hx2[0] - hb1 * hy1[0] - hb2 * hy2[0];
    hx2[0] = hx1[0]; hx1[0] = in;
    hy2[0] = hy1[0]; hy1[0] = out;
    l = out;

    in = r;
    out = ha0 * in + ha1 * hx1[1] + ha2 * hx2[1] - hb1 * hy1[1] - hb2 * hy2[1];
    hx2[1] = hx1[1]; hx1[1] = in;
    hy2[1] = hy1[1]; hy1[1] = out;
    r = out;

    stereobuffer[0] = l * COEF_16BIT_TO_FLOAT;
    stereobuffer[1] = r * COEF_16BIT_TO_FLOAT;
    stereobuffer += 2;
  }

}
