#include "../SDK/input.h"
#include "../SDK/unpack.h"
#include "../SDK/config.h"
#include "resource.h"

extern "C"
{
#include "mikmod.h"
#include "mplayer.h"
}

extern "C"
{
	extern MD_DEVICE drv_amp;
}


#define CPLAYFLG_LOOPALL      (1ul<<0)  // disables selective looping - loop everything!
#define CPLAYFLG_PLAYALL      (1ul<<1)  // plays hidden patterns (tack onto end of the song)
#define CPLAYFLG_FADEOUT      (1ul<<2)  // Fadeout the song before the end cometh?
#define CPLAYFLG_STRIPSILENCE (1ul<<3)  // Strip silence at the end of the song?
#define CPLAYFLG_SEEKBYORDERS (1ul<<4)  // Seek by orders instead of seconds
#define CPLAYFLG_CONT_LOOP    (1ul<<5)  // continue after loop

static const char * exts[]=
{
	"MOD","MDZ",
	"STM","STZ",
	"S3M","S3Z",
	"IT","ITZ",
	"XM","XMZ",
	"MTM",
	"ULT",
	"669",
	"UMX",
	"FAR",
	"OKT",
	"PTM",
};


// Winamp Stuff
//static UBYTE config_samplesize=1;

// Output Settings
static UBYTE    config_nch=2;
static UBYTE    config_cpu=0; // config_cpu_autodetect = 1; // MMX / 3DNow / whatever else might come out.

// Player Settings...
// ------------------

static int      config_loopcount=0;    // Auto-looping count, if the song has a natural loop.
static int     config_playflag=0;    // See CPLAYFLG_* defines above for flags
static int      config_pansep=128;  // master panning separation (0 == mono, 128 == stereo, 512 = way-separate)
static UBYTE    config_resonance = 0;
static int      config_fadeout = 1000;; // fadeout when the song is ending last loop (in ms)

// Mixer Settings...
// -----------------

static int config_voices=128;   // maximum voices player can use.

static cfg_int cfg_interp("interp",2),cfg_srate("srate",48000);

#define SILENCE_THRESHOLD  10800

typedef struct 
{
	reader * reader;
	bool is_unpacker;
} foobar_file;

static size_t _cdecl rfread( void *buffer, size_t size, size_t count, foobar_file *stream )
{
	return stream->reader->read((char*)buffer,size*count);
}

static size_t _cdecl rfwrite( const void *buffer, size_t size, size_t count, foobar_file *stream ) {return -1;}

static int _cdecl rfgetc( foobar_file *stream )
{
	int rv=0;
	if (rfread(&rv,1,1,stream)!=1) rv=EOF;
	return rv;
}

static int _cdecl rfputc( int c, foobar_file *stream )
{
    // not implemented
    return -1;
}

static int _cdecl rfseek( foobar_file *stream, long offset, int origin )
{
	return !stream->reader->seek2(offset,origin);
}

static long _cdecl rftell(foobar_file * stream)
{
	return stream->reader->get_position();
}

static int _cdecl rfeof(foobar_file * stream)
{
	return stream->reader->get_length() == stream->reader->get_position();
}

static int _cdecl rfclose(foobar_file * stream)
{
	if (stream->is_unpacker) stream->reader->reader_release();
	delete stream;
	return 0;
}

static const MMSTREAM_CALLBACK mm_callback_foobar = 
{
	(int (__cdecl *)(void *buffer, size_t size, size_t count, FILE *stream))rfread,
	(int (__cdecl *)(const void *buffer, size_t size, size_t count, FILE *stream))rfwrite,
	(int (__cdecl *)(FILE *stream))rfgetc,
	(int (__cdecl *)(int c, FILE *stream))rfputc,
	(int (__cdecl *)(FILE *stream, long offset, int origin))rfseek,
	(int (__cdecl *)(FILE *stream))rftell,
	(int (__cdecl *)(FILE *stream))rfeof,
	(int (__cdecl *)(FILE * stream))rfclose
};


static MMSTREAM *_mm_fopen_foobar(reader * r)
{
	foobar_file * stream;
//	if (!reader->canSeek()) return 0;
	
	reader * unpack = unpacker::unpack_open(r);
	stream = new foobar_file;
	
	if (unpack)
	{
		stream->reader = unpack;
		stream->is_unpacker = true;
	}
	else
	{
		stream->reader = r;
		stream->is_unpacker = false;
	}
	MMSTREAM * fp = _mmstream_createfp_callback((FILE*)stream,0,&mm_callback_foobar);
	if (!fp)
	{
		rfclose(stream);		
		return 0;
	}

	DWORD moo = _mm_read_I_ULONG(fp);
	_mm_rewind(fp);
	if (moo==0x9e2a83c1)
	{//Unreal UMX hack
		UBYTE boo[256];
		if (_mm_read_UBYTES(boo,256,fp))
		{
			int n,fooset=0;
			for(n=0;n<256-4;n++)
			{
				if (*(DWORD*)(boo+n)=='MPMI')
				{
					fp->iobase  = n;
					break;
				}
				else if (n>0x2c && *(DWORD*)(boo+n)=='MRCS')
				{
					fp->iobase = n-0x2c;
					break;
				}
				else if (*(DWORD*)(boo+n)=='etxE')
				{
					fp->iobase = n;
					break;
				}
			}
		}
		_mm_rewind(fp);
	}
	return fp;
}


//static const uint ss_flags[3]={0,DMODE_16BITS,DMODE_24BITS};

extern "C" void VC_RegisterMixerHack(VIRTCH *vc, VMIXER *mixer);//add mixers to override existing mixers


class input_mod : public input_pcm
{
private:

	void * __data;
	int __size;
	int __bps,__nch,__srate;

	UNIMOD * mf;
	MPLAYER * mp;
	MDRIVER * md;

public:

	input_mod()
	{
		__data=0;
		__size = __bps = __nch = __srate = 0;
		mf = 0;
		mp = 0;
		md = 0;
	}

	~input_mod()
	{
	    if (mp) {Player_Free(mp);mp=0;}
		if (mf) {Unimod_Free(mf);mf=0;}
	    if (md) {Mikmod_Exit(md);md=0;}
	}

	int open(reader * r,file_info * info,int full_open);

	virtual int test_filename(const char * fn,const char * ext)
	{
		int n;
		int found=0;
		for(n=0;n<tabsize(exts);n++)
		{
			if (!stricmp(ext,exts[n])) return 1;
		}
		return 0;
	}

	virtual int get_samples_pcm(void ** out_buffer,int * out_size,int * srate,int * bps,int * nch)
	{
		if (!Player_Active(mp)) return 0;
		__data = 0;


		Mikmod_Update(md);

		if (!__data) return 0;

		*out_buffer = __data;
		*out_size = __size;
		*srate = __srate;
		*bps = __bps;
		*nch = __nch;

		return 1;
	}

	virtual int seek(double seconds)
	{
		Player_SetPosTime(mp,(int)(seconds*1000.0));
		return 1;
	}

	void __foobar_hack_write(void * data,int len,int bps,int nch,int srate)
	{
		__data = data;
		__size = len;
		__bps = bps;
		__nch = nch;
		__srate = srate;
	}

};




static void mikmod_init()
{
	static bool inited;
	if (inited) return;
	inited=1;
//    _mmerr_sethandler(&mmerr);

#ifdef MM_LOG_VERBOSE
    log_init("c:\\temp\\in_mod", LOG_SILENT);
#endif
 
    ML_RegisterLoader(&load_it);
    ML_RegisterLoader(&load_xm);
    ML_RegisterLoader(&load_s3m);
    ML_RegisterLoader(&load_mod);
    ML_RegisterLoader(&load_mtm);
    ML_RegisterLoader(&load_stm);
    //ML_RegisterLoader(&load_dsm);
    //ML_RegisterLoader(&load_med);
    ML_RegisterLoader(&load_far);
	ML_RegisterLoader(&load_ptm);
	ML_RegisterLoader(&load_okt);
    ML_RegisterLoader(&load_ult);
    ML_RegisterLoader(&load_669);
    ML_RegisterLoader(&load_m15);

    MD_RegisterDriver(&drv_amp);
}


extern "C"
{
	void mikmod_foobar_hack_write(void * foo,void * data,int len,int bps,int nch,int srate)
	{
		input_mod * c=(input_mod*)foo;
		c->__foobar_hack_write(data,len,bps,nch,srate);
	}
}



int input_mod::open(reader * r,file_info * info,int full_open)
{
	mikmod_init();
    // Initialize MDRVER
    // -----------------

	int md_mode=0;
	//if (!md)
	{

		//if (config_interp & 1) md_mode |= DMODE_INTERP;
		/*if (config_interp & 2)*/ md_mode |= DMODE_NOCLICK;
		//if (config_interp & 4) md_mode |= DMODE_FIR;
		switch(cfg_interp)
		{
		case 1:
			md_mode|=DMODE_INTERP;
			break;
		case 2:
			md_mode|=DMODE_INTERP|DMODE_FIR;
			break;
		}
		md_mode |= DMODE_24BITS;//ss_flags[config_samplesize];
		if (config_nch==3)     md_mode |= DMODE_SURROUND;
//		if (config_panrev)     md_mode |= DMODE_REVERSE;
		if (config_resonance)  md_mode |= DMODE_RESONANCE;

		md = Mikmod_Init(cfg_srate, 1000, this, (config_nch == 1) ? MD_MONO : MD_STEREO, config_cpu, md_mode);

		if(!md)
		{   return 0;
		}

		md->pansep = config_pansep;

		// Register non-interpolation mixers
		// ---------------------------------
		// if the user has disabled interpolation...

		if(!cfg_interp)
		{
			VC_RegisterMixer(md->device.vc, &RF_M8_MONO);
			VC_RegisterMixer(md->device.vc, &RF_M16_MONO);
			VC_RegisterMixer(md->device.vc, &RF_M8_STEREO);
			VC_RegisterMixer(md->device.vc, &RF_M16_STEREO);

			VC_RegisterMixer(md->device.vc, &M8_MONO);
			VC_RegisterMixer(md->device.vc, &M16_MONO);
			VC_RegisterMixer(md->device.vc, &M8_STEREO);
			VC_RegisterMixer(md->device.vc, &M16_STEREO);
		}
		else// if (config_interp&4)
		{
	/*
			VC_RegisterMixerHack(md->device.vc, &M16_MONO_CUBIC);
			VC_RegisterMixerHack(md->device.vc, &M16_STEREO_CUBIC);
			VC_RegisterMixerHack(md->device.vc, &M8_MONO_CUBIC);
			VC_RegisterMixerHack(md->device.vc, &M8_STEREO_CUBIC);
	*/
			VC_RegisterMixerHack(md->device.vc, &M16_MONO_FIR);
			VC_RegisterMixerHack(md->device.vc, &M16_STEREO_FIR);
			VC_RegisterMixerHack(md->device.vc, &M8_MONO_FIR);
			VC_RegisterMixerHack(md->device.vc, &M8_STEREO_FIR);
		}
	}

    if(!mf)
    {
		MMSTREAM * fp=_mm_fopen_foobar(r);
		if (!fp) 
		{
            Mikmod_Exit(md);
			md=0;
            return 0;
		}
		string8 fn = info->get_file_path();

		mf = Unimod_Load_FP(md,fn,fp);//slow...

		_mm_fclose(fp);
		if (mf == NULL)
        {

            Mikmod_Exit(md);
			md=0;
            return 0;
        }
    }

    if(config_playflag & CPLAYFLG_STRIPSILENCE) Unimod_StripSilence(mf, SILENCE_THRESHOLD);

	md_mode = (config_playflag & CPLAYFLG_SEEKBYORDERS) ? PF_LOOP : 0;
    mp = Player_InitSong(mf, NULL, md_mode, config_voices);
	if(config_loopcount > 0)
	    Player_SetLoopStatus(mp, config_playflag & CPLAYFLG_LOOPALL, config_loopcount);
	Player_BuildQuickLookups(mp);
	if(config_playflag & CPLAYFLG_FADEOUT) Player_VolumeFadeEx(mp, MP_VOLUME_CUR, 0, config_fadeout, MP_SEEK_END, config_fadeout);
	mf->songlen = mp->songlen;

	//decode_pos_ms = 0;

    Player_Start(mp);

    _mmalloc_report(NULL);

	Player_PredictSongLength(mp);
	info->set_length((double)mp->songlen/1000.0);
	info->info_set_int("channels",mf->numchn);
	info->info_set("extrainfo",mf->modtype);

	
	if (mf->songname && *mf->songname && !info->meta_get("title")) info->meta_set("title",mf->songname);

    return 1;
}

static const int srate_tab[]={8000,11025,16000,22050,24000,32000,44100,48000,64000,88200,96000};

static BOOL CALLBACK ConfigProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			HWND list = GetDlgItem(wnd,IDC_INTERPOLATION);
			SendMessage(list,CB_ADDSTRING,0,(long)"none");
			SendMessage(list,CB_ADDSTRING,0,(long)"linear");
			SendMessage(list,CB_ADDSTRING,0,(long)"FIR");
			SendMessage(list,CB_SETCURSEL,cfg_interp,0);
			list = GetDlgItem(wnd,IDC_SAMPLERATE);
			int n;
			for(n=0;n<tabsize(srate_tab);n++)
			{
				char temp[16];
				wsprintf(temp,"%u",srate_tab[n]);
				SendMessage(list,CB_ADDSTRING,0,(long)temp);
			}			
			SetDlgItemInt(wnd,IDC_SAMPLERATE,cfg_srate,0);

		}
		return 1;
	case WM_COMMAND:
		switch(wp)
		{
		case (CBN_SELCHANGE<<16)|IDC_SAMPLERATE:
			cfg_srate = srate_tab[SendMessage((HWND)lp,CB_GETCURSEL,0,0)];
			break;
		case (CBN_EDITCHANGE<<16)|IDC_SAMPLERATE:
			{
				int t = GetDlgItemInt(wnd,IDC_SAMPLERATE,0,0);
				if (t<6000) t=6000;
				else if (t>192000) t=192000;
				cfg_srate = t;
			}
			break;
		case (CBN_SELCHANGE<<16)|IDC_INTERPOLATION:
			cfg_interp = SendMessage((HWND)lp,CB_GETCURSEL,0,0);
			break;
		}

		break;
	}
	return 0;
}

class config_mod : public config
{
public:
	virtual HWND create(HWND parent)
	{
		return CreateDialog(service_factory_base::get_my_instance(),(const char*)IDD_MOD_CONFIG,parent,ConfigProc);
	}

	virtual const char * get_name() {return "Module Decoder";}
	virtual const char * get_parent_name() {return "Playback";}
};


static service_factory_t<input,input_mod> foo;
static service_factory_single_t<config,config_mod> foo2;