#include <stdafx.h>

typedef struct
{
	UINT mdat,smpl;
} MOD;

static const MOD mod_old[]=
{
	//Turrican 1
	{8836,78244},//bonus
	{16044,45828},//intro
	{1352,512},//loader
	{9764,24822},//ongame1
	{7516,37356},//ongame2
	{10772,35560},//ongame3
	{4128,38130},//ongame4
	{7764,42574},//ongame5

	{29184,116160},//R-Type
	//X-Out
	{4768,33024},//XOut1
	{4828,35072},//XOut2
	{4736,35584},//XOut3
	{3844,40960},//XOut4
	{5936,39680},//XOut5
	{6180,40500},//XOutEnd
	{4484,28672},//XOutHigh
	{1260,7168},//XOutLoad
	{4320,42440},//XOutShop
	{5948,90112},//XOutTitle
};

static const MOD turrican3[]=
{
	{22912,74332},//title
	{16732,41220},//world1
	{9080,47964},//world2
	{14880,43298},//world3
	{6684,43524},//world4
	{11876,38916},//world5
};

#define _sizeof(x) (sizeof(x)/sizeof(x[0]))

BYTE get_compat(UINT mdat,UINT smpl)
{
	UINT n;
	for(n=0;n<_sizeof(mod_old);n++)
	{
		if (mdat==mod_old[n].mdat && smpl==mod_old[n].smpl)
			return G_COMPAT_OLD;
	}
	for(n=0;n<_sizeof(turrican3);n++)
	{
		if (mdat==turrican3[n].mdat && smpl==turrican3[n].smpl)
			return G_COMPAT_NEW;
	}

	return 0;
}
