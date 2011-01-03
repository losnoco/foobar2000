#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>

#define NUMCHANNELS 16
#define MAXWAVES 256
#define MAXTRACKS 256
#define MAXNOTES 8192
#define MAXEFFECTS 16

static _inline long scale (long a, long d, long c)
{
	_asm
	{
		mov eax, a
		mov edx, d
		mov ecx, c
		imul edx
		idiv ecx
		mov a, eax
	}
	return(a);
}

	//Actual sound parameters after initsb was called
long kdmsamplerate, kdmnumspeakers, kdmbytespersample;

	//KWV wave variables
static long numwaves;
static char instname[MAXWAVES][16];
static long wavleng[MAXWAVES];
static long repstart[MAXWAVES], repleng[MAXWAVES];
static long finetune[MAXWAVES];

	//Other useful wave variables
static long totsndbytes, wavoffs[MAXWAVES];

	//Effects array
static long eff[MAXEFFECTS][256];

	//KDM song variables:
static long kdmversionum, numnotes, numtracks;
static char trinst[MAXTRACKS], trquant[MAXTRACKS];
static char trvol1[MAXTRACKS], trvol2[MAXTRACKS];
static long nttime[MAXNOTES];
static char nttrack[MAXNOTES], ntfreq[MAXNOTES];
static char ntvol1[MAXNOTES], ntvol2[MAXNOTES];
static char ntfrqeff[MAXNOTES], ntvoleff[MAXNOTES], ntpaneff[MAXNOTES];

	//Other useful song variables:
static long timecount, notecnt, musicstatus, musicrepeat, loopcnt;

	//These shared with K.ASM:
long kdmasm1, kdmasm2, kdmasm3, kdmasm4;
char qualookup[512*16];

static char *snd = NULL;
static long kdminited = 0;

#define MAXSAMPLESTOPROCESS 32768
static long stemp[MAXSAMPLESTOPROCESS];

	//Sound reading information
static long splc[NUMCHANNELS], sinc[NUMCHANNELS], soff[NUMCHANNELS];
static long svol1[NUMCHANNELS], svol2[NUMCHANNELS];
static long volookup[NUMCHANNELS<<9];
static long swavenum[NUMCHANNELS];
static long frqeff[NUMCHANNELS], frqoff[NUMCHANNELS];
static long voleff[NUMCHANNELS], voloff[NUMCHANNELS];
static long paneff[NUMCHANNELS], panoff[NUMCHANNELS];

static long frqtable[256];
static long ramplookup[64];

#ifdef __WATCOMC__

extern long monohicomb(long,long,long,long,long,long);
#pragma aux monohicomb parm [eax][ebx][ecx][edx][esi][edi];
extern long stereohicomb(long,long,long,long,long,long);
#pragma aux stereohicomb parm [eax][ebx][ecx][edx][esi][edi];
extern long bound2char(long,long,long);
#pragma aux bound2char parm [ecx][esi][edi];
extern long bound2short(long,long,long);
#pragma aux bound2short parm [ecx][esi][edi];

static long oneshr10 = 0x3a800000, oneshl14 = 0x46800000;
#pragma aux fsin =\
	"fldpi"\
	"fimul dword ptr [eax]"\
	"fmul dword ptr [oneshr10]"\
	"fsin"\
	"fmul dword ptr [oneshl14]"\
	"fistp dword ptr [eax]"\
	parm [eax]\

#pragma aux calcvolookupmono =\
	"mov ecx, 64"\
	"lea edx, [eax+ebx]"\
	"add ebx, ebx"\
	"begit: mov dword ptr [edi], eax"\
	"mov dword ptr [edi+4], edx"\
	"add eax, ebx"\
	"add edx, ebx"\
	"mov dword ptr [edi+8], eax"\
	"mov dword ptr [edi+12], edx"\
	"add eax, ebx"\
	"add edx, ebx"\
	"add edi, 16"\
	"dec ecx"\
	"jnz begit"\
	parm [edi][eax][ebx]\
	modify [ecx edx]\

#pragma aux calcvolookupstereo =\
	"mov esi, 128"\
	"begit: mov dword ptr [edi], eax"\
	"mov dword ptr [edi+4], ecx"\
	"add eax, ebx"\
	"add ecx, edx"\
	"mov dword ptr [edi+8], eax"\
	"mov dword ptr [edi+12], ecx"\
	"add eax, ebx"\
	"add ecx, edx"\
	"add edi, 16"\
	"dec esi"\
	"jnz begit"\
	parm [edi][eax][ebx][ecx][edx]\
	modify [esi]\

#pragma aux mulscale16 =\
	"imul ebx"\
	"shrd eax, edx, 16"\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]\

#pragma aux mulscale24 =\
	"imul ebx"\
	"shrd eax, edx, 24"\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]\

#pragma aux mulscale30 =\
	"imul ebx"\
	"shrd eax, edx, 30"\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]\

#pragma aux clearbuf =\
	"snot: mov dword ptr [edi], eax"\
	"add edi, 4"\
	"loop snot"\
	parm [edi][ecx][eax]\

#pragma aux copybuf =\
	"snot: mov eax, dword ptr [esi]"\
	"mov dword ptr [edi], eax"\
	"add esi, 4"\
	"add edi, 4"\
	"loop snot"\
	parm [esi][edi][ecx]\
	modify [eax]\

#else

#ifdef __cplusplus
extern "C" {
#endif

void k_asm_dep_unlock ();
long monohicomb (long, long, long, long, long, long);
long stereohicomb (long, long, long, long, long, long);
long bound2char (long, long, long);
long bound2short (long, long, long);

#ifdef __cplusplus
}
#endif

static long oneshr10 = 0x3a800000, oneshl14 = 0x46800000;
static _inline void fsin (long *a)
{
	_asm
	{
		mov eax, a
		fldpi
		fimul dword ptr [eax]
		fmul dword ptr [oneshr10]
		fsin
		fmul dword ptr [oneshl14]
		fistp dword ptr [eax]
	}
}

static _inline void calcvolookupmono (long t, long a, long b)
{
	_asm
	{
		push ebx
		push edi
		mov edi, t
		mov eax, a
		mov ebx, b
		mov ecx, 64
		lea edx, [eax+ebx]
		add ebx, ebx
		begit: mov dword ptr [edi], eax
		mov dword ptr [edi+4], edx
		add eax, ebx
		add edx, ebx
		mov dword ptr [edi+8], eax
		mov dword ptr [edi+12], edx
		add eax, ebx
		add edx, ebx
		add edi, 16
		dec ecx
		jnz begit
		pop edi
		pop ebx
	}
}

static _inline void calcvolookupstereo (long t, long a, long b, long c, long d)
{
	_asm
	{
		push ebx
		push esi
		push edi
		mov edi, t
		mov eax, a
		mov ebx, b
		mov ecx, c
		mov edx, d
		mov esi, 128
		begit2: mov dword ptr [edi], eax
		mov dword ptr [edi+4], ecx
		add eax, ebx
		add ecx, edx
		mov dword ptr [edi+8], eax
		mov dword ptr [edi+12], ecx
		add eax, ebx
		add ecx, edx
		add edi, 16
		dec esi
		jnz begit2
		pop edi
		pop esi
		pop ebx
	}
}

static _inline long mulscale16 (long a, long d)
{
	_asm
	{
		mov eax, a
		imul d
		shrd eax, edx, 16
	}
}

static _inline long mulscale24 (long a, long d)
{
	_asm
	{
		mov eax, a
		imul d
		shrd eax, edx, 24
	}
}

static _inline long mulscale30 (long a, long d)
{
	_asm
	{
		mov eax, a
		imul d
		shrd eax, edx, 30
	}
}

static _inline void clearbuf (void *d, long c, long a)
{
	_asm
	{
		push edi
		mov edi, d
		mov ecx, c
		mov eax, a
		rep stosd
		pop edi
	}
}

static _inline void copybuf (void *s, void *d, long c)
{
	_asm
	{
		push esi
		push edi
		mov esi, s
		mov edi, d
		mov ecx, c
		rep movsd
		pop edi
		pop esi
	}
}

#endif

void kdmmusicon ()
{
	notecnt = 0;
	timecount = nttime[notecnt];
	musicrepeat = 1;
	musicstatus = 1;
}

void kdmmusicoff ()
{
	long i;

	musicstatus = 0;
	for(i=0;i<NUMCHANNELS;i++) splc[i] = 0;
	musicrepeat = 0;
	timecount = 0;
	notecnt = 0;
}

	//Unlike KDM, this function doesn't suck ALL memory
static long loadwaves (char *refdir)
{
	long fil, i, j, dawaversionum;
	char kwvnam[MAX_PATH+1];

	if (snd) return(1);

		//Look for WAVES.KWV in same directory as KDM file
	for(i=0;refdir[i];i++) kwvnam[i] = refdir[i];
	while ((i > 0) && (kwvnam[i-1] != '\\') && (kwvnam[i-1] != '/')) i--;
	strcpy(&kwvnam[i],"waves.kwv");

	if ((fil = open(kwvnam,O_BINARY|O_RDONLY)) == -1) return(0);
	read(fil,&dawaversionum,4);
	if (dawaversionum != 0) { close(fil); return(0); }

	totsndbytes = 0;
	read(fil,&numwaves,4);
	for(i=0;i<numwaves;i++)
	{
		read(fil,&instname[i][0],16);
		read(fil,&wavleng[i],4);
		read(fil,&repstart[i],4);
		read(fil,&repleng[i],4);
		read(fil,&finetune[i],4);
		wavoffs[i] = totsndbytes;
		totsndbytes += wavleng[i];
	}
	for(i=numwaves;i<MAXWAVES;i++)
	{
		for(j=0;j<16;j++) instname[i][j] = 0;
		wavoffs[i] = totsndbytes;
		wavleng[i] = 0L;
		repstart[i] = 0L;
		repleng[i] = 0L;
		finetune[i] = 0L;
	}

	if (!(snd = (char *)malloc(totsndbytes+2))) return(0);

	for(i=0;i<MAXWAVES;i++) wavoffs[i] += ((long)snd);
	read(fil,&snd[0],totsndbytes);
	snd[totsndbytes] = snd[totsndbytes+1] = 128;
	close(fil);
	return(1);
}

void initkdmeng ()
{
	long i, j, k;

	k_asm_dep_unlock();

	j = (((11025L*2093)/kdmsamplerate)<<13);
	for(i=1;i<76;i++)
	{
		frqtable[i] = j;
		j = mulscale30(j,1137589835);  //(pow(2,1/12)<<30) = 1137589835
	}
	for(i=0;i>=-14;i--) frqtable[i&255] = (frqtable[(i+12)&255]>>1);

	timecount = notecnt = musicstatus = musicrepeat = 0;

	clearbuf((void *)stemp,sizeof(stemp)>>2,32768L);
	for(i=0;i<256;i++)
		for(j=0;j<16;j++)
		{
			qualookup[(j<<9)+i] = (((-i)*j+8)>>4);
			qualookup[(j<<9)+i+256] = (((256-i)*j+8)>>4);
		}
	for(i=0;i<(kdmsamplerate>>11);i++)
	{
		j = 1536 - (i<<10)/(kdmsamplerate>>11);
		fsin(&j);
		ramplookup[i] = ((16384-j)<<1);
	}

	for(i=0;i<256;i++)
	{
		j = i*90; fsin(&j);
		eff[0][i] = 65536+j/9;
		eff[1][i] = min(58386+((i*(65536-58386))/30),65536);
		eff[2][i] = max(69433+((i*(65536-69433))/30),65536);
		j = (i*2048)/120; fsin(&j);
		eff[3][i] = 65536+(j<<2);
		j = (i*2048)/30; fsin(&j);
		eff[4][i] = 65536+j;
		switch((i>>1)%3)
		{
			case 0: eff[5][i] = 65536; break;
			case 1: eff[5][i] = 65536*330/262; break;
			case 2: eff[5][i] = 65536*392/262; break;
		}
		eff[6][i] = min((i<<16)/120,65536);
		eff[7][i] = max(65536-(i<<16)/120,0);
	}

	kdmmusicoff();
}

static void startwave (long wavnum, long dafreq, long davolume1, long davolume2, long dafrqeff, long davoleff, long dapaneff)
{
	long i, j, chanum;

	if ((davolume1|davolume2) == 0) return;

	chanum = 0;
	for(i=NUMCHANNELS-1;i>0;i--)
		if (splc[i] > splc[chanum])
			chanum = i;

	splc[chanum] = 0;     //Disable channel temporarily for clean switch

	if (kdmnumspeakers == 1)
		calcvolookupmono(((long)volookup)+(chanum<<(9+2)),-(davolume1+davolume2)<<6,(davolume1+davolume2)>>1);
	else
		calcvolookupstereo(((long)volookup)+(chanum<<(9+2)),-(davolume1<<7),davolume1,-(davolume2<<7),davolume2);

	sinc[chanum] = dafreq;
	svol1[chanum] = davolume1;
	svol2[chanum] = davolume2;
	soff[chanum] = wavoffs[wavnum]+wavleng[wavnum];
	splc[chanum] = -(wavleng[wavnum]<<12);              //splc's modified last
	swavenum[chanum] = wavnum;
	frqeff[chanum] = dafrqeff; frqoff[chanum] = 0;
	voleff[chanum] = davoleff; voloff[chanum] = 0;
	paneff[chanum] = dapaneff; panoff[chanum] = 0;
}

long kdmload (char *filename)
{
	long i, fil;

	if (!kdminited)
	{
		initkdmeng();
		if (!loadwaves(filename)) return(0);
		kdminited = 1;
	}
	if (!snd) return(0);

	kdmmusicoff();
	if ((fil = open(filename,O_BINARY|O_RDONLY)) == -1) return(0);
	read(fil,&kdmversionum,4); if (kdmversionum != 0) { close(fil); return(0); }
	read(fil,&numnotes,4);
	read(fil,&numtracks,4);
	read(fil,trinst,numtracks);
	read(fil,trquant,numtracks);
	read(fil,trvol1,numtracks);
	read(fil,trvol2,numtracks);
	read(fil,nttime,numnotes<<2);
	read(fil,nttrack,numnotes);
	read(fil,ntfreq,numnotes);
	read(fil,ntvol1,numnotes);
	read(fil,ntvol2,numnotes);
	read(fil,ntfrqeff,numnotes);
	read(fil,ntvoleff,numnotes);
	read(fil,ntpaneff,numnotes);
	close(fil);
	loopcnt = 0;
	return(scale(nttime[numnotes-1]-nttime[0],1000,120));
}

long kdmrendersound (void *dasnd, long numbytes)
{
	long i, j, k, voloffs1, voloffs2, *stempptr;
	long daswave, dasinc, dacnt, bytespertic, numsamplestoprocess;
	long ox, oy, x, y;
	char *sndptr, v1, v2;

	numsamplestoprocess = (numbytes>>(kdmnumspeakers+kdmbytespersample-2));
	bytespertic = (kdmsamplerate/120);
	for(dacnt=0;dacnt<numsamplestoprocess;dacnt+=bytespertic)
	{
		if (musicstatus > 0)    //Gets here 120 times/second
		{
			while ((notecnt < numnotes) && (timecount >= nttime[notecnt]))
			{
				j = trinst[nttrack[notecnt]];
				k = mulscale24(frqtable[ntfreq[notecnt]],finetune[j]+748);

				if (ntvol1[notecnt] == 0)   //Note off
				{
					for(i=NUMCHANNELS-1;i>=0;i--)
						if (splc[i] < 0)
							if (swavenum[i] == j)
								if (sinc[i] == k)
									splc[i] = 0;
				}
				else                        //Note on
					startwave(j,k,ntvol1[notecnt],ntvol2[notecnt],ntfrqeff[notecnt],ntvoleff[notecnt],ntpaneff[notecnt]);

				notecnt++;
				if (notecnt >= numnotes)
				{
					loopcnt++;
					if (musicrepeat > 0)
						notecnt = 0, timecount = nttime[0];
				}
			}
			timecount++;
		}

		for(i=NUMCHANNELS-1;i>=0;i--)
		{
			if (splc[i] >= 0) continue;

			dasinc = sinc[i];

			if (frqeff[i] != 0)
			{
				dasinc = mulscale16(dasinc,eff[frqeff[i]-1][frqoff[i]]);
				frqoff[i]++; if (frqoff[i] >= 256) frqeff[i] = 0;
			}
			if ((voleff[i]) || (paneff[i]))
			{
				voloffs1 = svol1[i];
				voloffs2 = svol2[i];
				if (voleff[i])
				{
					voloffs1 = mulscale16(voloffs1,eff[voleff[i]-1][voloff[i]]);
					voloffs2 = mulscale16(voloffs2,eff[voleff[i]-1][voloff[i]]);
					voloff[i]++; if (voloff[i] >= 256) voleff[i] = 0;
				}

				if (kdmnumspeakers == 1)
					calcvolookupmono(((long)volookup)+(i<<(9+2)),-(voloffs1+voloffs2)<<6,(voloffs1+voloffs2)>>1);
				else
				{
					if (paneff[i])
					{
						voloffs1 = mulscale16(voloffs1,131072-eff[paneff[i]-1][panoff[i]]);
						voloffs2 = mulscale16(voloffs2,eff[paneff[i]-1][panoff[i]]);
						panoff[i]++; if (panoff[i] >= 256) paneff[i] = 0;
					}
					calcvolookupstereo(((long)volookup)+(i<<(9+2)),-(voloffs1<<7),voloffs1,-(voloffs2<<7),voloffs2);
				}
			}

			daswave = swavenum[i];
			voloffs1 = ((long)volookup)+(i<<(9+2));

			kdmasm1 = repleng[daswave];
			kdmasm2 = wavoffs[daswave]+repstart[daswave]+repleng[daswave];
			kdmasm3 = (repleng[daswave]<<12); //repsplcoff
			kdmasm4 = soff[i];
			if (kdmnumspeakers == 1)
				{ splc[i] = monohicomb(0L,voloffs1,bytespertic,dasinc,splc[i],(long)stemp); }
			else
				{ splc[i] = stereohicomb(0L,voloffs1,bytespertic,dasinc,splc[i],(long)stemp); }
			soff[i] = kdmasm4;

			if (splc[i] >= 0) continue;
			if (kdmnumspeakers == 1)
				{ monohicomb(0L,voloffs1,kdmsamplerate>>11,dasinc,splc[i],((long)stemp)+(bytespertic<<2)); }
			else
				{ stereohicomb(0L,voloffs1,kdmsamplerate>>11,dasinc,splc[i],((long)stemp)+(bytespertic<<3)); }
		}

		if (kdmnumspeakers == 1)
		{
			for(i=(kdmsamplerate>>11)-1;i>=0;i--)
				stemp[i] += mulscale16(stemp[i+1024]-stemp[i],ramplookup[i]);
			j = bytespertic; k = (kdmsamplerate>>11);
			copybuf((void *)&stemp[j],(void *)&stemp[1024],k);
			clearbuf((void *)&stemp[j],k,32768);
		}
		else
		{
			for(i=(kdmsamplerate>>11)-1;i>=0;i--)
			{
				j = (i<<1);
				stemp[j+0] += mulscale16(stemp[j+1024]-stemp[j+0],ramplookup[i]);
				stemp[j+1] += mulscale16(stemp[j+1025]-stemp[j+1],ramplookup[i]);
			}
			j = (bytespertic<<1); k = ((kdmsamplerate>>11)<<1);
			copybuf((void *)&stemp[j],(void *)&stemp[1024],k);
			clearbuf((void *)&stemp[j],k,32768);
		}

		if (kdmnumspeakers == 1)
		{
			if (kdmbytespersample == 1) bound2char(bytespertic>>1,(long)stemp,((long)dasnd)+dacnt);
										  else bound2short(bytespertic>>1,(long)stemp,((long)dasnd)+(dacnt<<1));
		}
		else
		{
			if (kdmbytespersample == 1) bound2char(bytespertic,(long)stemp,((long)dasnd)+(dacnt<<1));
										  else bound2short(bytespertic,(long)stemp,((long)dasnd)+(dacnt<<2));
		}
	}
	return(loopcnt);
}

void kdmseek (long seektoms)
{
	long i;

	for(i=0;i<NUMCHANNELS;i++) splc[i] = 0;

	i = scale(seektoms,120,1000)+nttime[0];

	notecnt = 0;
	while ((nttime[notecnt] < i) && (notecnt < numnotes)) notecnt++;
	if (notecnt >= numnotes) notecnt = 0;

	timecount = nttime[notecnt]; loopcnt = 0;
}
