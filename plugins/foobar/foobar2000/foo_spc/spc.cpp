#define MYVERSION "1.7"

/*
	changelog

2004-10-22 01:21 UTC - kode54
- Using Shay Green's Spc_Emu library on trial
- Version is now 1.7

2004-04-26 07:51 UTC - kode54
- Changed finite rendering to match the wanted length instead of outputting a multiple of 576 samples
- Version is now 1.68

2004-02-10 20:31 UTC - DEATH
- Fixed ID666 reader handling tags without XID_END
- Eliminated b_use_spctag condition, copied some redundant code
- Version is now 1.67

2004-02-02 22:38 UTC - kode54
- Fixed context_get_display function declaration in context_fest
- Version is now 1.66

2004-01-14 16:46 UTC - kode54
- Added load-time global volume surround removal
- Version is now 1.65

2004-01-09 20:38 UTC - kode54
- Removed support for retarded binary number format in header, since nobody seems to use it anyway
- Added input_file_type service
- Version is now 1.64

2003-10-19 03:12 UTC - kode54
- Changed length editor to use query_info_locked to retrieve the existing length from a single track

2003-10-15 16:14 UTC - kode54
- Amended IsFucked for matching nulled buffers
- Version is now 1.63a

2003-10-11 07:13 UTC - kode54
- Updated length editor locking behavior

2003-09-28 15:18 UTC - kode54
- Fixed length editor and removed circular database update crap
- Version is now 1.63

2003-09-19 13:07 UTC - kode54
- Added default length/fade settings
- Version is now 1.62

2003-06-29 00:11 UTC - kode54
- Removed references to cfg_window_remember
- Version is now 1.61

2003-06-26 14:58 UTC - kode54
- Merged changes from DEATH
- Updated to 0.7 API
- Version is now 1.6

2003-06-23 04:23 UTC - kode54
- Added no_infinite check to fade out
- Moved no_infinite init to open()
- Version is now 1.53

2003-05-21 02:36 UTC - kode54
- Small endianness corrections by FatlXception, nothing we'd notice with x86 target
- Changed hint() slightly
- Version is now 1.52

2003-05-20 11:21 UTC - kode54
- Rearranged libopenspc++ a bit

2003-05-12 04:21 UTC - kode54
- Corrected minor error with garbage checker

2003-05-11 01:02 UTC - kode54
- Added detection of garbage left by crappy tag writers
- Commented extrainfo as requested by Case, pending clearance
- Version is now 1.51
- APEv2 tag handler now zeroes length/fade if there are no
  fields setting them, rather than possibly falling back on
  values from the file header. Eh, who knows.

2003-05-08 01:08 UTC - kode54
- Added samplerate, channels, and bitspersample info

2003-05-07 17:17 UTC - kode54
- Converted libopenspc into a fully C++ class system, multi instance
  now works perfectly. Holy shit, that took me 8 hours.

*/

#define dsp fb2k_dsp
#include "../SDK/foobar2000.h"
#undef dsp
#include "../helpers/window_placement_helper.h"

#include <stdio.h>

//#include "../../libopenspc/openspc.h"
#include "../../openspc2/spc_emu-0.1.0/spc_emu.h"
#include "../../openspc2/packed_spc-0.1.0/unpack_spc.h"

#include "resource.h"

static const char field_length[]="SPC_LENGTH";
static const char field_fade[]="SPC_FADE";

/*
char (*OSPC_ReadPort0)(void);
char (*OSPC_ReadPort1)(void);
char (*OSPC_ReadPort2)(void);
char (*OSPC_ReadPort3)(void);
void (*OSPC_WritePort0)(char);
void (*OSPC_WritePort1)(char);
void (*OSPC_WritePort2)(char);
void (*OSPC_WritePort3)(char);
*/

#if 0
static void openspc_filter(signed short *ptr)
{
	signed int li, ri;
	if (cfg_dspopt & DSP_REVERSE)
	{
		ri = ptr[0];
		li = ptr[1];
	}
	else
	{
		li = ptr[0];
		ri = ptr[1];
	}

	if (cfg_dspopt & DSP_SURND)
	{
		/* Cheap stereo expansion trick I created,
		     as seen in ZSNES, now adapted to C */
		signed int h = (li+ri) >> 1, l = li - h, r = ri - h;
		l = ri - l;
		r = li - r;
		if (l < -32768) ri = -32768;
		else if (l > 32767) ri = 32767;
		else ri = l;
		if (r < -32768) li = -32768;
		else if (r > 32767) li = 32767;
		else li = r;
	}
	ptr[0] = li;
	ptr[1] = ri;
}
#endif






#define SPC_SIZE 0x10200

//based on Uematsu.h from SuperJukebox


//Sub-chunk ID's
#define	XID_SONG		0x01
#define	XID_GAME		0x02
#define	XID_ARTIST	0x03
#define	XID_DUMPER	0x04
#define	XID_DATE		0x05
#define	XID_EMU		0x06
#define	XID_CMNTS	0x07
#define	XID_INTRO	0x30
#define	XID_LOOP		0x31
#define	XID_END		0x32
#define	XID_FADE		0x33
#define	XID_MUTE		0x34
#define	XID_OST		0x10
#define	XID_DISC		0x11
#define	XID_TRACK	0x12
#define	XID_PUB		0x13
#define	XID_COPY		0x14

//Data types
#define	XTYPE_DATA	0x00
#define	XTYPE_STR	0x01
#define	XTYPE_INT	0x04
#define	XTYPE_BCD	0x10

typedef struct _SUBCHK
{
	unsigned char	id;                        //Subchunk ID
	unsigned char	type;                      //Type of data
	unsigned short	data;                      //Data
}SUBCHK,*PSUBCHK,FAR *LPSUBCHK;

typedef struct _SPCREGS
{
	unsigned char	PC[2];
	unsigned char	A;
	unsigned char	X;
	unsigned char	Y;
	unsigned char	PSW;
	unsigned char	SP;
}SPCREGS,*PSPCREGS,FAR *LPSPCREGS;

typedef struct _SPCHEADER
{
	char		FTag[33];                       //File tag
	char		Term[3];                        //Tag terminator
	char		Ver;                            //Version #/100
	SPCREGS		Regs;                           //SPC Registers
	short		__r1;
	char		Title[32];                       //Song title
	char		Game[32];                       //Game title
	char		Dumper[16];                     //Name of dumper
	char		Comment[32];                    //Comments
	char		Date[11];                       //Date dumped
	char		SongLen[3];                     //Song length (number of seconds to play before fading)
	char		FadeLen[5];                     //Fade length (milliseconds)
	char		Artist[32];                     //Artist of song
	char		ChnDis;                         //Channel Disabled
	char		Emulator;                       //Emulator dumped with
	char		__r2[45];
}SPCHEADER,*PSPCHEADER,FAR *LPSPCHEADER;

typedef struct _ID666TAG
{
	TCHAR szTitle[256];			//Title of song
	TCHAR szGame[256];			//Name of game
	TCHAR szArtist[256];		//Name of artist
	TCHAR szPublisher[256];		//Game publisher
	TCHAR szDumper[256];		//Name of dumper
	TCHAR szDate[256];			//Date dumped
	TCHAR szComment[256];		//Optional comment
	TCHAR szOST[256];			//Original soundtrack title
	UINT uSong_ms;				//Length of song
	UINT uLoop_ms;				//Length of loop
	UINT uEnd_ms;				//Length of end
	UINT uFade_ms;				//Length of fadeout
	BYTE bDisc;					//OST disc number
	WORD wTrack;				//OST track number
	WORD wCopyright;			//Game copyright date
	BYTE bMute;					//Bitmask for channel states	
	BYTE bEmulator;				//Emulator used to dump
}ID666TAG,*PID666TAG,FAR *LPID666TAG;

//BOOL load_id666(SPCHEADER * spc,reader* r,LPID666TAG lpTag);

static cfg_int cfg_infinite("spc_infinite",0);
static cfg_int cfg_deflength("spc_deflength",170000);
static cfg_int cfg_deffade("spc_deffade",10000);

static cfg_int cfg_antisurround("spc_antisurround", 0);

static cfg_struct_t<WINDOWPLACEMENT> cfg_window_placement("window_placement",0);

static BOOL IsString(LPCTSTR lpszStr,INT nLen)
{
	while(--nLen>=0&&lpszStr[nLen]==0);
	while(nLen>=0&&((lpszStr[nLen]>=0x30&&lpszStr[nLen]<=0x39)||lpszStr[nLen]=='/'||lpszStr[nLen]=='-'))nLen--;
	return ((nLen==-1)?TRUE:FALSE);
}

/* Special function to check for fucked up ASCII tags because
   some piece of crap tag writer recycles the same string buffer
   without nulling the whole thing between writes */
/*
static BOOL IsFucked(LPCSTR one, LPCSTR two, INT nLen)
{
	int start = strlen(two) + 1;
	if (start >= nLen) return FALSE;
	int i, fc, nc;
	for (one += start, two += start, i = start, fc = 0, nc = 0; i < nLen; one++, two++, i++)
	{
		if (!*one && !*two) nc++;
		else if (*one == *two) fc++;
		else break;
	}

	return fc && fc + nc == nLen - start;
}*/

static void SetDate(LPTSTR lpszDate,int year,int month,int day)
{
	if(year<100)
	{
		year+=1900;
//		if(year<1997)year+=100;
	}
//	if(year<1997)year=0;
	if(month<1||month>12)year=0;
	if(day<1||day>31)year=0;

	if(year)
	{
/*	What the bloody fuck is this?
		if(month>2)
		{
			if(!(year&3)&&year%100)day++;
			else if(!(year%400))day++;
		}
*/
		string8 temp;
		temp.add_int(month);
		temp.add_char('/');
		temp.add_int(day);
		temp.add_char('/');
		temp.add_int(year);
		strcpy(lpszDate, temp);
	}
	else lpszDate[0]='\0';
}

static void parse_header(SPCHEADER * spc,ID666TAG * lpTag)
{
	char SongStr[4]={0},FadeStr[6]={0};
	memset(lpTag,0,sizeof(ID666TAG));
	memcpy(lpTag->szTitle,spc->Title,32);
	memcpy(lpTag->szGame,spc->Game,32);
	memcpy(lpTag->szDumper,spc->Dumper,16);
	memcpy(lpTag->szDate,spc->Date,11);
	memcpy(lpTag->szComment,spc->Comment,32);
	memcpy(SongStr,spc->SongLen,3);
	memcpy(FadeStr,spc->FadeLen,5);


	
	lpTag->uSong_ms=0;//cfg_def_song;
	lpTag->uFade_ms=0;//cfg_def_fade;

/*	BOOL Bin=FALSE;

	if(IsString(SongStr,3)&&IsString(FadeStr,5)&&IsString(lpTag->szDate,11))
	{
		if(SongStr[0]!=0||FadeStr[0]!=0||lpTag->szDate[0]!=0)
			Bin=FALSE;
	}
	else
	{
		/ Let's check for fucked up tag writing, shall we? *
		if (!IsFucked(lpTag->szComment,lpTag->szDate,11) &&
			!IsFucked(lpTag->szDate,SongStr,3) &&
			!IsFucked(SongStr,FadeStr,3))
		{
			Bin=TRUE;
		}
	}*/

	/*long i,j;
	char str[12],*t;
	DWORD d,m,y;
	if(Bin)
	{
		j=*(DWORD*)(FadeStr);
		if(j>59999)j=59999;

		i=*(WORD*)(SongStr);
		if(i>959)i=959;

		lpTag->uSong_ms=i;
		lpTag->uFade_ms=j;

		memcpy(lpTag->szArtist,spc->Artist-1,32);
		lpTag->bEmulator=spc->ChnDis;

		y=*(WORD*)(&lpTag->szDate[2]);
		m=*(BYTE*)(&lpTag->szDate[1]);
		d=*(BYTE*)(&lpTag->szDate[0]);
	}
	else*/
	{
		lpTag->uSong_ms=atoi(SongStr);
		lpTag->uFade_ms=atoi(FadeStr);

		if(lpTag->uSong_ms>959)lpTag->uSong_ms=959;
		if(lpTag->uFade_ms>59999)lpTag->uFade_ms=59999;

		memcpy(lpTag->szArtist,spc->Artist,32);
		lpTag->bEmulator=spc->Emulator;
		if(lpTag->bEmulator>=0x30&&lpTag->bEmulator<=0x39)lpTag->bEmulator-=0x30;
		else lpTag->bEmulator=0;

		/*y=0;
		strcpy(str,lpTag->szDate);
		if(str[0]!=0)
		{
			t=strchr(str,'-');
			if (t&&*(++t)!=0)
			{
				m=atoi(t);
				t=strchr(t,'-');
				if(t&&*(++t)!=0)
				{
					d=atoi(t);
					y=atoi(str);
					if (d > 1000)
					{
						int blah = d;
						d = m;
						m = y;
						y = blah;
					}
				}
			}
			else
			{
				t=strchr(str,'/');
				if(t&&*(++t)!=0)
				{
					d=atoi(t);
					t=strchr(t,'/');
					if(t&&*(++t)!=0)
					{
						y=atoi(t);
						m=atoi(str);
					}
				}
			}
		}*/
	}
	
	//SetDate(lpTag->szDate,y,m,d);
	lpTag->uSong_ms*=1000;


}

static BOOL load_id666(reader * r,LPID666TAG lpTag)//must be seeked to correct spot before calling
{
	DWORD dwBytesRead;
	SUBCHK sub;
	TCHAR szBuf[5]={0};
	dwBytesRead=r->read((char*)szBuf,4);

	if(!stricmp(szBuf,"xid6"))
	{
		{
			DWORD zzz;
			r->read((char*)&zzz,4);
		}
		dwBytesRead=r->read((char*)&sub,sizeof(sub));

#define ReadFile(a,b,c,d,e) (*d)=r->read((char*)(b),c)

		while(dwBytesRead && sub.data <= (r->get_length() - r->get_position()))
		{
			switch(sub.id)
			{
			case XID_SONG:
				ReadFile(hFile,lpTag->szTitle,sub.data,&dwBytesRead,NULL);
				break;
			case XID_GAME:
				ReadFile(hFile,lpTag->szGame,sub.data,&dwBytesRead,NULL);
				break;
			case XID_ARTIST:
				ReadFile(hFile,lpTag->szArtist,sub.data,&dwBytesRead,NULL);
				break;
			case XID_DUMPER:
				ReadFile(hFile,lpTag->szDumper,sub.data,&dwBytesRead,NULL);
				break;
			case XID_DATE:
				UINT uDate;
				ReadFile(hFile,&uDate,sub.data,&dwBytesRead,NULL);
				SetDate(lpTag->szDate,uDate>>16,(uDate>>8)&0xFF,uDate&0xFF);
				break;
			case XID_EMU:
				lpTag->bEmulator=(BYTE)sub.data;
				break;
			case XID_CMNTS:
				ReadFile(hFile,lpTag->szComment,sub.data,&dwBytesRead,NULL);
				break;
			case XID_OST:
				ReadFile(hFile,lpTag->szOST,sub.data,&dwBytesRead,NULL);
				break;
			case XID_DISC:
				lpTag->bDisc=(BYTE)sub.data;
				break;
			case XID_TRACK:
				lpTag->wTrack=(WORD)sub.data;
				break;
			case XID_PUB:
				ReadFile(hFile,lpTag->szPublisher,sub.data,&dwBytesRead,NULL);
				break;
			case XID_COPY:
				lpTag->wCopyright=(WORD)sub.data;
				break;
			case XID_INTRO:
				ReadFile(hFile,&lpTag->uSong_ms,sub.data,&dwBytesRead,NULL);
				if(lpTag->uSong_ms>61376000)lpTag->uSong_ms=61376000;
				lpTag->uSong_ms/=64;
				break;
			case XID_LOOP:
				ReadFile(hFile,&lpTag->uLoop_ms,sub.data,&dwBytesRead,NULL);
				if(lpTag->uLoop_ms>383936000)lpTag->uLoop_ms=383936000;
				lpTag->uLoop_ms/=64;
				break;
			case XID_END:
				if (sub.data == sizeof(lpTag->uEnd_ms))
				{
					ReadFile(hFile,&lpTag->uEnd_ms,sub.data,&dwBytesRead,NULL);
					if(lpTag->uEnd_ms>61376000)lpTag->uEnd_ms=61376000;
					lpTag->uEnd_ms/=64;
				}
				else
				{
					r->seek2(sub.data,SEEK_CUR);
				}
				break;
			case XID_FADE:
				ReadFile(hFile,&lpTag->uFade_ms,sub.data,&dwBytesRead,NULL);
				if(lpTag->uFade_ms>3839360)lpTag->uFade_ms=3839360;
				lpTag->uFade_ms/=64;
				break;
			case XID_MUTE:
				lpTag->bMute=(BYTE)sub.data;
				break;
			default:
				if(sub.type)
				{
					char foo[0x100];
					UINT size=sub.data;
					while(size)
					{
						UINT delta=size;
						if (delta>0x100) delta=0x100;
						r->read(foo,delta);
						size-=delta;
					}
				}
				break;
			}
			ReadFile(hFile,&sub,sizeof(sub),&dwBytesRead,NULL);
		}
#undef ReadFile
	}
	if ((!lpTag->uSong_ms && !lpTag->uFade_ms)/* || cfg_ignore_time*/)
	{
		lpTag->uSong_ms=0;//cfg_def_song;
		lpTag->uFade_ms=0;//cfg_def_fade;
	}

	return 1;
}


class input_spc : public input_pcm
{
private:

	virtual bool test_filename(const char * fn,const char * ext) 
	{
		return !stricmp(ext,"SPC");
	}

	int open_file(reader * r,file_info * info)
	{
		__int64 size64 = r->get_length();
		if (size64 < 0 || size64 > 1024*1024) return 0;

		bool own_reader = false;

		reader * rdr = unpacker::g_open(r);
		if (rdr)
		{
			own_reader = true;
			r = rdr;
			size64 = r->get_length();
			if (size64 < 0 || size64 > 0x1000000L)
			{
				r->reader_release();
				return 0;
			}
		}

		unsigned size = (int)size64;

		if (size < SPC_SIZE)
		{
			if (
			return 0;
		}
		size=SPC_SIZE;

		spc_backup.set_size(SPC_SIZE);

		if (r->read(spc_backup.get_ptr(),size)!=size)
		{
			r->seek(0);
			return 0;
		}

		{
			ID666TAG tag;
			memset(&tag,0,sizeof(tag));
			parse_header((SPCHEADER*)spc_backup.get_ptr(),&tag);

			tag_song_ms = tag.uSong_ms;
			tag_fade_ms = tag.uFade_ms;
			mute_flags = tag.bMute;
			
			if (tag_song_ms>0) info->info_set_int(field_length,tag_song_ms);
			if (tag_fade_ms>0) info->info_set_int(field_fade,tag_fade_ms);

			
			bool have_id666;
			{
				char blah[4];
				r->seek(SPC_SIZE);
				r->read(&blah,4);
				r->seek(SPC_SIZE);
				have_id666 = !memcmp(blah,"xid6",4);				
			}

			if (have_id666)
			{
				if (tag.szTitle[0]) info->meta_set_ansi("TITLE",tag.szTitle);
				if (tag.szGame[0]) info->meta_set_ansi("ALBUM",tag.szGame);
				if (tag.szArtist[0]) info->meta_set_ansi("ARTIST",tag.szArtist);
				if (tag.szDumper[0]) info->meta_set_ansi("DUMPER",tag.szDumper);
				if (tag.szDate[0]) info->meta_set_ansi("DATE",tag.szDate);
				if (tag.szComment[0]) info->meta_set_ansi("COMMENT",tag.szComment);
				tag_song_ms = tag.uSong_ms;
				tag_fade_ms = tag.uFade_ms;
				mute_flags = tag.bMute;
			
				if (tag_song_ms>0) info->info_set_int(field_length,tag_song_ms);
				if (tag_fade_ms>0) info->info_set_int(field_fade,tag_fade_ms);

				load_id666(r,&tag);

				if (tag.szOST[0]) info->meta_add_ansi("OST",tag.szOST);
				if (tag.szPublisher[0]) info->meta_add_ansi("PUBLISHER",tag.szPublisher);
				if (tag.wTrack>0)
				{
					char temp[16];
					sprintf(temp,"%u",((tag.wTrack&0xFF00)>>8)|((tag.wTrack&0xFF)<<8));
					info->meta_set("TRACKNUMBER",temp);
				}
				if (tag.wTrack>0)
				{
					char temp[16];
					sprintf(temp,"%u",tag.bDisc);
					info->meta_set("DISC",temp);
				}

				if (tag.szTitle[0]) info->meta_set_ansi("TITLE",tag.szTitle);
				if (tag.szGame[0]) info->meta_set_ansi("ALBUM",tag.szGame);
				if (tag.szArtist[0]) info->meta_set_ansi("ARTIST",tag.szArtist);
				if (tag.szDumper[0]) info->meta_set_ansi("DUMPER",tag.szDumper);
				if (tag.szDate[0]) info->meta_set_ansi("DATE",tag.szDate);
				if (tag.szComment[0]) info->meta_set_ansi("COMMENT",tag.szComment);
				tag_song_ms = tag.uSong_ms;
				tag_fade_ms = tag.uFade_ms;
				mute_flags = tag.bMute;

			}
			else if (tag_reader::g_run_multi(r,info,"ape|id3v1"))
			{
				const char * p;
				p = info->meta_get(field_length);
				if (p)
				{
					info->info_set(field_length,p);
					tag_song_ms = atoi(p);
					info->meta_remove_field(field_length);
				}
				else
				{
					tag_song_ms = 0;
				}
				p = info->meta_get(field_fade);
				if (p)
				{
					info->info_set(field_fade,p);
					tag_fade_ms = atoi(p);
					info->meta_remove_field(field_fade);
				}
				else
				{
					tag_fade_ms = 0;
				}
			}
			else
			{
				if (tag.szTitle[0]) info->meta_set_ansi("TITLE",tag.szTitle);
				if (tag.szGame[0]) info->meta_set_ansi("ALBUM",tag.szGame);
				if (tag.szArtist[0]) info->meta_set_ansi("ARTIST",tag.szArtist);
				if (tag.szDumper[0]) info->meta_set_ansi("DUMPER",tag.szDumper);
				if (tag.szDate[0]) info->meta_set_ansi("DATE",tag.szDate);
				if (tag.szComment[0]) info->meta_set_ansi("COMMENT",tag.szComment);
				tag_song_ms = tag.uSong_ms;
				tag_fade_ms = tag.uFade_ms;
				mute_flags = tag.bMute;
			
				if (tag_song_ms>0) info->info_set_int(field_length,tag_song_ms);
				if (tag_fade_ms>0) info->info_set_int(field_fade,tag_fade_ms);

			}
		}
	

		if (!tag_song_ms)
		{
			tag_song_ms = cfg_deflength;
			tag_fade_ms = cfg_deffade;
		}
		info->set_length((double)(tag_song_ms + tag_fade_ms)*.001);
		info->info_set_int("samplerate", 32000);
		info->info_set_int("channels", 2);
		info->info_set_int("bitspersample", 16);
		// info->info_set("extrainfo","SPC");

		return 1;

	}

	virtual bool open( reader * r, file_info * info,unsigned openflags)
	{
		if (!open_file(r,info)) return false;
		if (!(openflags & OPEN_FLAG_DECODE)) return true;

		if (cfg_antisurround)
		{
			signed char * ptr = reinterpret_cast<signed char *>(spc_backup.get_ptr());
			ptr[0x10100 + 0x0C] = abs(ptr[0x10100 + 0x0C]);
			ptr[0x10100 + 0x1C] = abs(ptr[0x10100 + 0x1C]);
		}

		//spc = new openspc;
		spc = new Spc_Emu;

		spc->load_spc(spc_backup.get_ptr(), 66048);
		spc->mute_voices(0);
		spc->clear_echo();

/*		if (cfg_dspopt & (DSP_REVERSE|DSP_SURND))
			OSPC_SetFilter(&openspc_filter);
*/
		openspc_pos = 0;
		data_written=0;
		pos_delta=0;

		newtag=1;

		no_infinite = !!(openflags & OPEN_FLAG_NO_LOOPING) || !cfg_infinite;

		return true;
	}
public:
	input_spc()
	{
		spc = 0;
		openspc_pos=0;
		song_len=fade_len=0;
		tag_song_ms=tag_fade_ms=0;
		mute_flags=0;
	}

	~input_spc()
	{
		if (spc) delete spc;
	}

	virtual input::set_info_t set_info(reader *r,const file_info * info)
	{
		r->seek(SPC_SIZE);
		r->set_eof();
		file_info_i_full l_info;
		l_info.copy(info);

		{
			const char * p;
			p = l_info.info_get(field_length);
			if (p)
			{
				l_info.meta_set(field_length,p);
			}
			p = l_info.info_get(field_fade);
			if (p)
			{
				l_info.meta_set(field_fade,p);
			}
		}

		tag_remover::g_run(r);
		return tag_writer::g_run(r, &l_info, "ape") ? SET_INFO_SUCCESS : SET_INFO_FAILURE;
	}


	virtual int get_samples_pcm(void ** buffer,int * size,int * srate,int * bps,int * nch)
	{
		*srate = 32000;
		*bps = 16;
		*nch = 2;

		if (newtag)
		{
			spc->mute_voices(mute_flags);
			newtag=0;
			calcfade();
		}
		
		if (no_infinite && tag_song_ms && (pos_delta + MulDiv(data_written,1000,2*2*32000))>tag_song_ms+tag_fade_ms + 100) return 0;

		UINT written=0;

		int samples;

		if (no_infinite)
		{
			samples = (song_len + fade_len) - (data_written >> 2);
			if (samples > 576) samples = 576;
			else if (samples <= 0) return 0;
			samples <<= 1;
		}
		else
		{
			samples = 576 * 2;
		}
		
		//written=spc->run(-1, (short*)sample_buffer.check_size(samples), samples * sizeof(short));
		written = spc->play(samples, (Spc_Emu::sample_t*)sample_buffer.check_size(samples)) * sizeof(short);

		if (!written)
		{
			console::warning(uStringPrintf("SPC execution halted at PC 0x%04X", spc->get_cpu_pc()));
			return 0;
		}

		openspc_pos += (written>>7);

		int d_start,d_end;
		d_start=data_written>>2;//16bit stereo always
		data_written+=written;
		d_end=data_written>>2;

		if (tag_song_ms && d_end>song_len && no_infinite)
		{
			short * foo=(short*)sample_buffer;
			int n;
			for(n=d_start;n<d_end;n++)
			{
				if (n>song_len)
				{
					if (n>song_len+fade_len)
					{
						*(DWORD*)foo=0;
					}
					else
					{
						int bleh=song_len+fade_len-n;
						foo[0]=MulDiv(foo[0],bleh,fade_len);
						foo[1]=MulDiv(foo[1],bleh,fade_len);
					}
				}
				foo+=2;
			}
		}

		*buffer = sample_buffer.get_ptr();
		*size = written;


		return 1;

	}

	virtual bool seek(double pos64)
	{
		int pos = (int)(pos64*1000.0);
		if (pos<openspc_pos)
		{
			//spc->init(spc_backup.get_ptr(), 66048);
			delete spc;
			spc = new Spc_Emu;
			spc->load_spc(spc_backup.get_ptr(), 66048);
			spc->mute_voices(mute_flags);
			spc->clear_echo();
			openspc_pos = 0;
		}
		int delta = pos - openspc_pos;
		/*while (delta)
		{
			int run = delta;
			if (run > 0x1FFFFFF) run = 0x1FFFFFF;
			spc->run(-1, NULL, run<<7);
			delta -= run;
		}*/
		spc->skip(delta);

		data_written=0;
		pos_delta=pos;

		calcfade();

		return true;
	}

private:

	bool newtag;
	bool no_infinite;

	mem_block_t<BYTE> spc_backup;

	mem_block_t<short> sample_buffer;

	//openspc * spc;
	Spc_Emu * spc;
	
	int data_written,pos_delta;

	int openspc_pos;

	int song_len,fade_len;
	int tag_song_ms,tag_fade_ms,mute_flags;

	void calcfade()
	{
		song_len=MulDiv(tag_song_ms-pos_delta,32000,1000);
		fade_len=MulDiv(tag_fade_ms,32000,1000);

	}
	
};

#define BORK_TIME 0xC0CAC01A

static unsigned long parse_time_crap(const char *input)
{
	if (!input) return BORK_TIME;
	int len = strlen(input);
	if (!len) return BORK_TIME;
	int value = 0;
	{
		int i;
		for (i = len - 1; i >= 0; i--)
		{
			if ((input[i] < '0' || input[i] > '9') && input[i] != ':' && input[i] != ',' && input[i] != '.')
			{
				return BORK_TIME;
			}
		}
	}
	string8 foo = input;
	char *bar = (char *)foo.get_ptr();
	char *strs = bar + foo.length() - 1;
	while (strs > bar && (*strs >= '0' && *strs <= '9'))
	{
		strs--;
	}
	if (*strs == '.' || *strs == ',')
	{
		// fraction of a second
		strs++;
		if (strlen(strs) > 3) strs[3] = 0;
		value = atoi(strs);
		switch (strlen(strs))
		{
		case 1:
			value *= 100;
			break;
		case 2:
			value *= 10;
			break;
		}
		strs--;
		*strs = 0;
		strs--;
	}
	while (strs > bar && (*strs >= '0' && *strs <= '9'))
	{
		strs--;
	}
	// seconds
	if (*strs < '0' || *strs > '9') strs++;
	value += atoi(strs) * 1000;
	if (strs > bar)
	{
		strs--;
		*strs = 0;
		strs--;
		while (strs > bar && (*strs >= '0' && *strs <= '9'))
		{
			strs--;
		}
		if (*strs < '0' || *strs > '9') strs++;
		value += atoi(strs) * 60000;
		if (strs > bar)
		{
			strs--;
			*strs = 0;
			strs--;
			while (strs > bar && (*strs >= '0' && *strs <= '9'))
			{
				strs--;
			}
			value += atoi(strs) * 3600000;
		}
	}
	return value;
}

static void print_time_crap(int ms, char *out)
{
	char frac[8];
	int i,h,m,s;
	if (ms % 1000)
	{
		sprintf(frac, ".%3.3d", ms % 1000);
		for (i = 3; i > 0; i--)
			if (frac[i] == '0') frac[i] = 0;
		if (!frac[1]) frac[0] = 0;
	}
	else
		frac[0] = 0;
	h = ms / (60*60*1000);
	m = (ms % (60*60*1000)) / (60*1000);
	s = (ms % (60*1000)) / 1000;
	if (h) sprintf(out, "%d:%2.2d:%2.2d%s",h,m,s,frac);
	else if (m) sprintf(out, "%d:%2.2d%s",m,s,frac);
	else sprintf(out, "%d%s",s,frac);
}

static BOOL CALLBACK ConfigProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			char temp[16];
			uSendDlgItemMessage(wnd, IDC_INFINITE, BM_SETCHECK, cfg_infinite, 0);
			uSendDlgItemMessage(wnd, IDC_ANTISURROUND, BM_SETCHECK, cfg_antisurround, 0);
			print_time_crap(cfg_deflength, (char *)&temp);
			uSetDlgItemText(wnd, IDC_DLENGTH, (char *)&temp);
			print_time_crap(cfg_deffade, (char *)&temp);
			uSetDlgItemText(wnd, IDC_DFADE, (char *)&temp);
		}
		return 1;
	case WM_COMMAND:
		switch(wp)
		{
		case IDC_INFINITE:
			cfg_infinite = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		case IDC_ANTISURROUND:
			cfg_antisurround = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		case (EN_CHANGE<<16)|IDC_DLENGTH:
			{
				int meh = parse_time_crap(string_utf8_from_window((HWND)lp));
				if (meh != BORK_TIME) cfg_deflength = meh;
			}
			break;
		case (EN_KILLFOCUS<<16)|IDC_DLENGTH:
			{
				char temp[16];
				print_time_crap(cfg_deflength, (char *)&temp);
				uSetWindowText((HWND)lp, temp);
			}
			break;
		case (EN_CHANGE<<16)|IDC_DFADE:
			{
				int meh = parse_time_crap(string_utf8_from_window((HWND)lp));
				if (meh != BORK_TIME) cfg_deffade = meh;
			}
			break;
		case (EN_KILLFOCUS<<16)|IDC_DFADE:
			{
				char temp[16];
				print_time_crap(cfg_deffade, (char *)&temp);
				uSetWindowText((HWND)lp, temp);
			}
			break;
		}
		break;
	}
	return 0;
}

class config_spc : public config
{
public:
	virtual HWND create(HWND parent)
	{
		return uCreateDialog(IDD_CONFIG, parent, ConfigProc);
	}

	virtual const char * get_name() {return "SPC Decoder";}
	virtual const char * get_parent_name() {return "Input";}
};

typedef struct
{
	unsigned song, fade;
} INFOSTRUCT;

static BOOL CALLBACK TimeProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		uSetWindowLong(wnd,DWL_USER,lp);
		{
			INFOSTRUCT * i = (INFOSTRUCT*) lp;
			char temp[16];
			if (!i->song && !i->fade) uSetWindowText(wnd, "Set length");
			else uSetWindowText(wnd, "Edit length");
			if (i->song)
			{
				print_time_crap(i->song, (char*)&temp);
				uSetDlgItemText(wnd, IDC_LENGTH, (char*)&temp);
			}
			if (i->fade)
			{
				print_time_crap(i->fade, (char*)&temp);
				uSetDlgItemText(wnd, IDC_FADE, (char*)&temp);
			}
		}
		apply_window_placement(cfg_window_placement,wnd);
		return 1;
	case WM_COMMAND:
		switch(wp)
		{
		case IDOK:
			{
				INFOSTRUCT * i=(INFOSTRUCT*)uGetWindowLong(wnd,DWL_USER);
				int foo;
				foo = parse_time_crap(string_utf8_from_window(wnd, IDC_LENGTH));
				if (foo != BORK_TIME) i->song = foo;
				else i->song = 0;
				foo = parse_time_crap(string_utf8_from_window(wnd, IDC_FADE));
				if (foo != BORK_TIME) i->fade = foo;
				else i->fade = 0;
			}
			EndDialog(wnd,1);
			break;
		case IDCANCEL:
			EndDialog(wnd,0);
			break;
		}
		break;
	case WM_DESTROY:
		read_window_placement(cfg_window_placement,wnd);
		break;
	}
	return 0;
}

static bool context_time_dialog(unsigned *song_ms, unsigned *fade_ms)
{
	bool ret;
	INFOSTRUCT * i = new INFOSTRUCT;
	if (!i) return 0;
	i->song = *song_ms;
	i->fade = *fade_ms;
	HWND hwnd = core_api::get_main_window();
	ret = uDialogBox(IDD_TIME, hwnd, TimeProc, (long)i) > 0;
	if (ret)
	{
		*song_ms = i->song;
		*fade_ms = i->fade;
	}
	delete i;
	return ret;
}

class context_spc : public menu_item_context
{
public:
	virtual unsigned get_num_items() { return 1; }

	virtual void enum_item(unsigned n, string_base & out) {out.set_string("SPC: Edit Length");}

	virtual bool context_get_display(unsigned n, const ptr_list_base<metadb_handle> & data, string_base & out, unsigned & displayflags, const GUID & caller)
	{
		unsigned i, j;
		i = data.get_count();
		for (j = 0; j < i; j++)
		{
			const playlist_entry * foo = data.get_item(j)->handle_get_location();
			if (stricmp(string_extension_8(foo->get_path()), "SPC")) return false;
		}
		if (i == 1) out.set_string("Edit length");
		else out.set_string("Set length");
		return true;
	}

	virtual void context_command(unsigned n, const ptr_list_base<metadb_handle> & data, const GUID & caller)
	{
		unsigned tag_song_ms = 0, tag_fade_ms = 0;
		unsigned i = data.get_count();
		file_info_i_full info;
		if (i == 1)
		{
			// fetch info from single file
			metadb_handle * handle = data.get_item(0);
			handle->handle_lock();
			const file_info * p_info = handle->handle_query_locked();
			if (p_info)
			{
				const char *t = p_info->info_get(field_length);
				if (t) tag_song_ms = atoi(t);
				t = p_info->info_get(field_fade);
				if (t) tag_fade_ms = atoi(t);
			}
			handle->handle_unlock();
		}
		if (!context_time_dialog(&tag_song_ms, &tag_fade_ms)) return;
		for (unsigned j = 0; j < i; j++)
		{
			metadb_handle *foo = data.get_item(j);
			foo->handle_lock();
			foo->handle_query(&info);
			if (tag_song_ms > 0) info.info_set_int(field_length, tag_song_ms);
			else info.info_remove_field(field_length);
			if (tag_fade_ms > 0) info.info_set_int(field_fade, tag_fade_ms);
			else info.info_remove_field(field_fade);
			{
				if (!tag_song_ms)
				{
					tag_song_ms = cfg_deflength;
					tag_fade_ms = cfg_deffade;
				}
				double length = (double)(tag_song_ms + tag_fade_ms) * .001;
				info.set_length(length);
			}
			foo->handle_update_info(&info);
			foo->handle_unlock();
		}
	}
};

class spc_file_types : public input_file_type
{
	virtual unsigned get_count()
	{
		return 1;
	}

	virtual bool get_name(unsigned idx, string_base & out)
	{
		if (idx > 0) return false;
		out = "SPC files";
		return true;
	}

	virtual bool get_mask(unsigned idx, string_base & out)
	{
		if (idx > 0) return false;
		out = "*.SPC";
		return true;
	}
};

static service_factory_t<input,input_spc> foo;
static service_factory_single_t<config,config_spc> foo2;
static service_factory_single_t<input_file_type,spc_file_types> foo3;
static service_factory_single_t<menu_item,context_spc> foo4;

DECLARE_COMPONENT_VERSION("SPC decoder",MYVERSION,0);
