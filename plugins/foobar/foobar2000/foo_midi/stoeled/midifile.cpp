#include "main.h"

//#define HUNT_LEAKS
#ifdef HUNT_LEAKS
static UINT n_files;
#endif

MIDI_file::MIDI_file()
{
	flags=0;
	format=0;
	len=0;tix=0;
	size=0;
	data=0;
	info.fmt=info.ntrax=info.tix=0;
	info.channels=0;
	info.e_type=0;
	info.traxnames=0;
	info.traxtext=0;
	bmp_data=0;
	bmp_size=0;
	kar_track=0;
	tmap=0;
	smap=0;
	mmap=0;
#ifdef HUNT_LEAKS
	n_files++;
#endif
	refcount=1;

}

static bool is_gmd(const BYTE* b,int s)
{
	return s>12 && *(DWORD*)b==_rv('MIDI') && *(DWORD*)(b+8)==_rv('MDpg');
}

static bool is_hmi(const BYTE* b,int s)
{
	return s>12 && *(DWORD*)b==_rv('HMI-') && *(DWORD*)(b+4)==_rv('MIDI') && *(DWORD*)(b+8)==_rv('SONG');
}

static bool is_hmp(const BYTE* b,int s)
{
	if (s>8 && ((DWORD*)b)[0]==_rv('HMIM') && (((DWORD*)b)[1]==_rv('IDIP') || ((DWORD*)b)[1]==_rv('IDIR')) )
	{
		//DWORD d=*(DWORD*)(b+0x30);
		//return (d<0x40 && d);
		return 1;
	}
	else return 0;
}

static bool is_xmidi(const BYTE* b,int s)
{
	return s>0x20 && *(DWORD*)b==_rv('FORM') && *(DWORD*)(b+8)==_rv('XDIR') && *(DWORD*)(b+0x1e)==_rv('XMID');
}

static bool is_rmi(const BYTE* b,int s)
{
	return s>20+8+6+8 && *(DWORD*)b==_rv('RIFF') && *(DWORD*)(b+8)==_rv('RMID') && *(DWORD*)(b+12)==_rv('data');
}

static bool is_midi(const BYTE* b,int s)
{
	return s>8+6+8 && *(DWORD*)b==_rv('MThd') && *(DWORD*)(b+4)==0x06000000 && *(DWORD*)(b+14)==_rv('MTrk');
}

static bool is_midi_scan(const BYTE* b,int s)
{
	int x,m=s;
	if (m>256) m=256;
	m-=8+6+8;
	for(x=0;x<m;x++)
		if (is_midi(b+x,s-x)) return 1;
	return 0;
}

#define REM (int)(sz-ptr)

static bool load_midi_fix(MIDI_file* mf,const BYTE* buf,int sz,int n_track,int p_ofs)
{
	if (!cfg_recover_tracks) return 0;
	BYTE* outbuf=(BYTE*)malloc(sz+0x10);
	if (!outbuf) return 0;
	int ptr=p_ofs;
	int bp=ptr;
	BYTE lc=0;
	while(1)
	{
		bp=ptr;
		if (REM<4) break;
		while(buf[ptr]&0x80)
		{
			if (ptr==bp+4) break;
			ptr++;
		}
		ptr++;
		if (REM<3) break;
		BYTE b=buf[ptr];
		if (b==0xFF)
		{
			ptr+=2;
			if (REM<4) break;
			int d;
			int l=DecodeDelta(buf+ptr,&d);
			if (l+d>REM) break;
			ptr+=l+d;
		}
		else if (b==0xF0)
		{
			ptr++;
			if (REM<4) break;
			int d;
			int l=DecodeDelta(buf+ptr,&d);
			if (l+d>REM) break;
			ptr+=l+d;
		}
		else
		{
			if (b&0x80)
			{
				lc=b&0xF0;
				if (lc==0xF0) break;
				ptr++;
			}
			else if (!lc) break;
			if (lc==0xC0 || lc==0xD0) ptr++;
			else ptr+=2;
		}
	}
	memcpy(outbuf,buf,ptr);
	ptr=bp;
	outbuf[ptr++]=0;
	outbuf[ptr++]=0xFF;
	outbuf[ptr++]=0x2F;
	outbuf[ptr++]=0;
	*(DWORD*)(outbuf+p_ofs-4)=rev32(ptr-p_ofs);
	mf->data=outbuf;
	mf->size=ptr;

	return 1;
}

#undef REM

static bool load_midi(MIDI_file* mf,const BYTE* buf,int sz)
{
	int trax=rev16(*(WORD*)(buf+4+4+2));
	int ofs=6+8;
	int n;
	for(n=0;n<trax;n++)
	{
		if (*(DWORD*)(buf+ofs)!=_rv('MTrk') || ofs+12>sz)
		{
			mf->flags|=FLAG_INCOMPLETE;
			*(WORD*)(buf+4+4+2)=rev16(n);
			sz=ofs;
			break;
		}
		ofs+=8;
		int p_ofs=ofs;
		ofs+=rev32(*(DWORD*)(buf+ofs-4));
		if (ofs>sz)
		{
			mf->flags|=FLAG_INCOMPLETE;
			*(WORD*)(buf+4+4+2)=rev16(n+1);
			if (!load_midi_fix(mf,buf,sz,n,p_ofs))
			{
				*(WORD*)(buf+4+4+2)=rev16(n);
				sz=p_ofs-8;
				break;
			}
			else return 1;
		}
	}

	BYTE * out = (BYTE*)malloc(sz);
	memcpy(out,buf,sz);
	mf->data=out;
	mf->size=sz;
	return 1;
}

static bool load_gmd(MIDI_file* mf,const BYTE* buf,int sz)
{
	if (sz<=0x10) return 0;
	int s=rev32(*(DWORD*)(buf+4));
	if (sz<s+8) return 0;
	int ofs=rev32(*(DWORD*)(buf+12))+0x10;
	s-=ofs;
	BYTE * out=(BYTE*)malloc(s);
	if (!out) return 0;
	mf->size=s;
	memcpy(out,buf+ofs,s);
	mf->data=out;
	return 1;
}

static bool load_rmi(MIDI_file* mf,const BYTE* source,int source_size)
{
	int sx=*(DWORD*)(source+4);
	int _p=0;
	BYTE * out;
	if (sx+8>source_size) goto _er;
	mf->size=*(DWORD*)(source+16);
	if (mf->size+20>source_size) goto _er;
	out=(BYTE*)malloc(mf->size);
	if (!out) goto _er;
	memcpy(out,source+20,mf->size);
	mf->data = out;

	_p=20+mf->size;
	if (_p&1) _p++;
	while(_p<source_size)
	{
		if (((_p + *(DWORD*)(source+_p+4) + 1) & ~1) >= source_size)
			break;
		if (! mf->bmp_data && *(DWORD*)(source+_p)==_rv('DISP') && *(DWORD*)(source+_p+8)==8)//bitmap
		{
			DWORD s=*(DWORD*)(source+_p+4)-4;
			void * r=malloc(s);
			if (r)
			{
				memcpy(r,source+_p+12,s);
				mf->bmp_size=s;
				mf->bmp_data=r;
			}
		}
		else if (*(DWORD*)(source+_p)==_rv('DISP') && *(DWORD*)(source+_p+8)==1)
		{
			DWORD s=*(DWORD*)(source+_p+4)-4;
			char * src=(char*)(source+_p+12);	//remove eol's
			char * dst=mf->title.buffer_get(s+1);
			char * src_b=src;
			while(*src && (UINT)(src-src_b)<s)
			{
				if (*src!=10 && *src!=13) *(dst++)=*src;
				src++;
			}
			*dst=0;
			mf->title.buffer_done();
		}
		else if (*(DWORD*)(source+_p)==_rv('LIST') && *(DWORD*)(source+_p+8)==_rv('INFO'))
		{
			DWORD s=*(DWORD*)(source+_p+4);
			if (s >= 4)
			{
				s -= 4;
				const BYTE * data = source + _p + 12;
				while (s >= 8)
				{
					unsigned len = s - 8;
					if (len > *(DWORD *)(data + 4)) len = *(DWORD *)(data + 4);
					mf->rmi_data.add_entry(*(int*)data, (char*)data + 8, len);
					if (len & 1) len++;
					s -= len + 8;
					data += len + 8;
				}
			}
		}
		_p+=*(DWORD*)(source+_p+4)+8;
		if (_p&1) _p++;
	}
	return 1;
_er:
	return 0;
}

bool load_xmi(MIDI_file* mf,const BYTE*,int);
bool load_hmp(MIDI_file* mf,const BYTE*,int);
bool load_hmi(MIDI_file* mf,const BYTE*,int);
bool load_mus(MIDI_file* mf,const BYTE*,int);
bool load_cmf(MIDI_file* mf,const BYTE*,int);
bool load_mids(MIDI_file* mf,const BYTE*,int);
bool load_gmf(MIDI_file* mf,const BYTE*,int);
bool is_mus(const BYTE*,int);
bool is_cmf(const BYTE*,int);
bool is_mids(const BYTE*,int);
bool is_gmf(const BYTE*,int);

static bool load_midi_scan(MIDI_file* mf,const BYTE* ptr,int size)
{
	int max = size-3;
	if (max>256) max=256;
	int x;
	for(x=0;x<256;x++)
	{
		if (*(DWORD*)(ptr+x)==_rv('MThd') && *(DWORD*)(ptr+x+4)==_rv(6))
		{
			size-=x;
			ptr+=x;
			void * buf=malloc(size);
			if (!buf) return 0;
			memcpy(buf,ptr,size);
			bool r=load_midi(mf,(BYTE*)buf,size);
			if (!r) free(buf);
			return r;
		}
	}
	return 0;
}


struct
{
	bool ( * test ) (const BYTE* b,int s);
	bool ( * load ) (MIDI_file* mf,const BYTE* ptr,int size);
} format_list[] = 
{
	{is_midi,load_midi},
	{is_rmi,load_rmi},
	{is_hmp,load_hmp},
	{is_hmi,load_hmi},
	{is_xmidi,load_xmi},
	{is_mus,load_mus},
	{is_cmf,load_cmf},
	{is_gmd,load_gmd},
	{is_mids,load_mids},
	{is_gmf,load_gmf},
	{is_midi_scan,load_midi_scan}
};

//static fmtfunc fmts[]={is_midi,is_rmi,is_hmp,is_hmi,is_xmidi,is_mus,is_cmf,is_gmd,is_mids,is_gmf,is_midi_scan};
//loadfunc loaders[]={load_midi,load_rmi,load_hmp,load_hmi,load_xmi,load_mus,load_cmf,load_gmd,load_mids,load_gmf,load_midi_scan};

MIDI_file::~MIDI_file()
{
	if (data) free((BYTE*)data);
	if (tmap) delete tmap;
	if (smap) delete smap;
	if (mmap) delete mmap;
	if (info.traxnames) delete[] info.traxnames;
	if (info.traxtext) delete[] info.traxtext;
	if (bmp_data) free(bmp_data);
#ifdef HUNT_LEAKS
	n_files--;
#endif
}

bool GetMidiInfo(MIDI_file*);

static void * z_malloc(int s)
{
	void * ptr = malloc(s);
	if (ptr) memset(ptr,0,s);
	return ptr;
}

static bool try_format(const void * data,int size,int idx)
{
	bool rv;
	try	{
		rv = format_list[idx].test((const BYTE*)data,size);
	} catch(...) 
	{
		rv = 0;
	}
	return rv;
}

int MIDI_file::HeaderTest(const void * data,int size)
{
	int n;
	for(n=0;n<tabsize(format_list);n++)
	{
		if (try_format(data,size,n)) return 1;
	}
	return 0;
}

int MIDI_file::Load(const void * data,int size)
{
	log_write("Load()");
	
	{
		int n;
		int fmt=-1;
		for(n=0;n<tabsize(format_list);n++)
		{
			if (try_format(data,size,n)) {fmt=n;break;}
		}
		if (fmt==-1) return 0;
		format = fmt;
	}



	{
		bool r;
		try {
			r=format_list[format].load(this,(const BYTE*)data,size);
		} catch(...) {
			log_write("midi loader crashed");
			r=0;
		}
		if (!r) return 0;
	}

	return GetMidiInfo(this);
}

MIDI_file* MIDI_file::Create(const void * data,int size)
{
	MIDI_file* mf=new MIDI_file;
	if (!mf->Load(data,size))
	{
		delete mf;
		mf=0;
	}
	return mf;
}