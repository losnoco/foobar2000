// changelog

// 2003-07-31 21:40 - kode54
// - Fixed support for basic KSS format

// 2003-06-26 07:32 - kode54
// - Updated to 0.7 API, except for some annoying ANSI shit buried in the mess of C code which is NEZplug

// 2003-04-12 23:37 - kode54
// - added bitspersample info

#include "../SDK/foobar2000.h"
#include "../helpers/dropdown_helper.h"

#include "common/nsfsdk/nsfsdk.h"

#include "common/win32/rc/nezplug.rh"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/*
#define DBG(a) { \
	OutputDebugString( # a ); \
	a; \
	OutputDebugString("success"); \
}
*/

static const char * exts[]=
{ "NSF",
//  "NEZ", "NSZ", "NSD",
  "KSS",
  "GBR",
  "GBS",
  "HES", "PCE",
  "AY", "CPC",
};

static cfg_int cfg_srate("nez_srate", 48000);
static cfg_int cfg_nonsf("nez_nonsf", 0);

// HANDLE hLock;

// static int playing = 0, terminate = 0;

// f-u NEZplug, or smth

/*
class initquit_nez : public initquit
{
public:
	virtual void on_init()
	{
		hLock = CreateSemaphore(NULL, 1, 1, NULL);
	}
	virtual void on_quit()
	{
		CloseHandle(hLock);
	}

	static GUID get_class_guid();
};
*/

static int isKSS(char *buf)
{
	if (*(int*)buf == 'CCSK') return 0x10;
	if (*(int*)buf == 'XSSK') return 0x10 + ((unsigned char)buf[0xe]);
	return 0;
}

static int get_nez_info(reader * r, int *firstsong, int *numsongs, const char *filename, file_info *info)
{
	char fbuf[64];
	string8 title, fmt, artist, copyright;
	int fs, ns, ch;
	
	r->seek(0);
	r->read(&fbuf, 32);
	
	fs = 0;
	ns = 0;
	ch = 1;
	
	if (*(int*)&fbuf == 'MSEN')
	{
		ns = ((int)fbuf[6]) & 255;
		fs = 1;
		// inital fbuf[7];
		r->seek(0x000e);
		r->read(fbuf, 32);
		fbuf[32] = '\0';
		title = (char*)&fbuf;
		fmt = "NSF";
		r->read(&fbuf, 32);
		artist = (char*)&fbuf;
		r->read(&fbuf, 32);
		copyright = (char*)&fbuf;
	}
	else if (!memcmp(fbuf,"GBS",3))
	{
		ch = 2;
		fs = 1;
		ns = ((int)fbuf[4]) & 255;
		// inital fbuf[5];
		r->seek(0x0010);
		r->read(&fbuf, 32);
		fbuf[32] = '\0';
		title = (char*)&fbuf;
		fmt = "GBS";
		r->read(&fbuf, 32);
		artist = (char*)&fbuf;
		r->read(&fbuf, 32);
		copyright = (char*)&fbuf;
	}
	else if (*(int*)&fbuf == 'FRBG' || *(int*)&fbuf == 'MSEH')
	{
		ch = 2;
		ns = 256;
		fs = 1;
		string8 temp = filename;
		title = temp.get_ptr() + temp.scan_filename();
		int ext = title.find_last('.');
		if (ext >= 0) title.truncate(ext);
		if (fbuf[0] == 'G') fmt = "GBR";
		else fmt = "HES";
	}
	else if (isKSS(fbuf))
	{
		fs = 1;
		int top = isKSS(fbuf);
		if (top >= 0x1c)
		{
			ns = (((unsigned char)fbuf[0x1b]) << 8) + ((unsigned char)fbuf[0x1a]) + 1;
			if ((fbuf[0x0f] & (1 << 2)) ||
				((fbuf[0x0f] & (3 << 3)) == (3 << 3)))
				ch = 2;
		}
		else
		{
			ns = 256;
		}
		r->seek(top);
		if (r->read(&fbuf, 16) != 16) return 0;
		if (!memcmp(fbuf,"MGS",3) || !memcmp(fbuf,"MPK",3))
		{
			int c = 0, ret, p = 0;
			r->seek(top);
			do {
				ret = r->read(&c, 1);
			} while (ret == 1 && c != 0x1a && c != 0x0a);
			do
			{
				ret = r->read(&c, 1);
				if (c >= 0x20)
					title.add_char(c);
				p++;
			}
			while (ret == 1 && c != 0x1a && c != 0x0a && p < 64);
		}
		else if (!memcmp(fbuf,"MBM",3))
		{
			int c = 0, ret, p = 0;
			r->seek(top + 0x0010);
			do
			{
				ret = r->read(&c, 1);
				if (c >= 0x20) title.add_char(c);
				p++;
			}
			while (ret == 1 && p < 0x28);
		}
		else if (!memcmp(fbuf,"BTO KINROU 5th",14))
		{
			int p = 0, c = 0, ret, base, title_offset;
			r->seek(top + 0x1150);
			if (r->read(&fbuf, 4) != 4) return 0;
			/* MPK2KSS check */
			if (fbuf[0] != '\x80' || fbuf[1] != '\x11') return 0;
			base = (((unsigned char)fbuf[3]) << 8) + ((unsigned char)fbuf[2]);
			r->seek(top + 0x2049);
			if (r->read(&fbuf, 3) != 3) return 0;
			if (memcmp(fbuf, "BTO", 3)) return 0;
			r->seek(top + 0x2054);
			if (r->read(&fbuf, 2) != 2) return 0;
			title_offset = (((unsigned char)fbuf[1]) << 8) + ((unsigned char)fbuf[0]);
			title_offset = (title_offset - base) & 0xffff;
			r->seek(top + 0x2000 + title_offset);
			do
			{
				ret = r->read(&c, 1);
				if (c >= 0x20)
					title.add_char(c);
				p++;
			}
			while (ret == 1 && c != 0x00 && p < 64);
		}
	}
	
	if (info)
	{
		/*
		char num[8];
		int song = info->get_subsong_index();
		if (ns > fs)
		{
			if (ns < 10 ||
				(ns < 100 && song > 9) ||
				(ns < 1000 && song > 99) ||
				(ns < 10000 && song > 999) ||
				(ns < 100000 && song > 9999)) itoa(song, (char*)&num, 10);
			else if (ns < 100 ||
				(ns < 1000 && song > 9) ||
				(ns < 10000 && song > 99) ||
				(ns < 100000 && song > 999))
			{
				num[0] = '0';
				itoa(song, (char*)&num[1], 10);
			}
			else if (ns < 1000 ||
				(ns < 10000 && song > 9) ||
				(ns < 100000 && song > 99))
			{
				*(short*)&num = '00';
				itoa(song, (char*)&num[2], 10);
			}
			else if (ns < 10000 ||
				(ns < 1000000 && song > 9))
			{
				*(int*)num = '000';
				itoa(song, (char*)&num[3], 10);
			}
			else
			{
				*(int*)&num = '0000';
				itoa(song, (char*)&num[4], 10);
			}
			
			title.add_string(" (");
			title.add_string((const char *)&num);
			title.add_char('/');
			title.add_int(ns);
			title.add_char(')');
		}
		
		info->meta_add("title", (const char *)title.get_ptr());
		*/
		info->meta_add("album", title);
		if (artist.length()) info->meta_add("artist", artist);
		if (copyright.length()) info->meta_add("copyright", copyright);
		title = "";
		title.add_int(info->get_subsong_index());
		info->meta_add("tracknumber", title);
		info->meta_add("title", "?");

		info->info_set_int("samplerate", cfg_srate);
		info->info_set_int("bitspersample", 16);
		info->info_set_int("channels", ch);
		info->info_set("extrainfo", fmt);
	}

	if (firstsong) *firstsong = fs;
	if (numsongs) *numsongs = ns;
	
	return 1;
}

class input_nez : public input_pcm
{
private:
	virtual bool is_our_content_type(const char *url, const char *type)
	{
		return !strcmp(type, "audio/x-nsf");
	}

	virtual bool test_filename(const char * fn,const char * ext)
	{
		int n;
		for(n=cfg_nonsf;n<tabsize(exts);n++)
		{
			if (!stricmp(ext,exts[n])) return true;
		}
		return false;
	}

	virtual bool open( reader * r, file_info * info,unsigned openflags)	//FUCKO: multiinstance safety ?
	{
		__int64 sz64 = r->get_length();
		if (sz64 > 16 * 1024 * 1024) return false;
		unsigned size = (int)sz64;

		if (!get_nez_info(r, NULL, NULL, info->get_file_path(), info)) return false;

		if (!(openflags & OPEN_FLAG_DECODE)) return true;

/*
		if (playing) terminate = 1;
		if (terminate)
		{
			WaitForSingleObject(hLock, 500);
		}
*/

		char *ptr = nezbuffer.set_size(size);
		r->seek(0);
		r->read(ptr, size);
		
		hnsf = NSFSDK_Load(ptr, size);
		if (!hnsf) return false;
		nezbuffer.set_size(0);

		rate = cfg_srate;
		chan = NSFSDK_GetChannel(hnsf);

		NSFSDK_SetFrequency(hnsf, rate);
		NSFSDK_SetChannel(hnsf, chan);

		NSFSDK_SetNosefartFilter(hnsf, 0);

		int song = info->get_subsong_index();

		NSFSDK_SetSongNo(hnsf, song);

		NSFSDK_Reset(hnsf);
		NSFSDK_Volume(hnsf, 0);

//		playing = 1;

		return true;
	}
public:
	input_nez()
	{
		hnsf = NULL;
	}

	~input_nez()
	{
		if (hnsf) NSFSDK_Terminate(hnsf);
		/*
		playing = 0;
		terminate = 0;
		ReleaseSemaphore(hLock, 1, NULL);
		*/
	}

	virtual input::set_info_t set_info(reader *r,const file_info * info)
	{
		return SET_INFO_FAILURE;
	}

	virtual int get_samples_pcm(void ** buffer,int * size,int * srate,int * bps,int * nch)
	{
		/*if (terminate)
		{
			return 0;
		}*/
		signed short * output = sample_buffer.check_size(576 * chan);
		*buffer = output;
		*srate = rate;
		*bps = 16;
		*nch = chan;
		*size = 576 * 2 * chan;

		NSFSDK_Render(hnsf, output, 576);

		return 1;
	}

	virtual bool can_seek() { return false; }

	virtual bool seek(double pos64)
	{
		return false;
	}

private:

	mem_block_t<char> nezbuffer;
	mem_block_t<signed short> sample_buffer;

	HNSF hnsf;

	int rate, chan;
};

class track_indexer_nez : public track_indexer
{
private:
	virtual int get_tracks(const char* filename,callback * out,reader * r)
	{
		string8 ext = string_extension_8(filename);
		int n, found = 0, ret = 0;
		for(n=cfg_nonsf;n<tabsize(exts);n++)
		{
			if (!stricmp(ext,exts[n])) found = 1;
		}
		if (!found) return 0;
		bool own_reader = 0;
		if (!r)
		{
			r = file::g_open_precache(filename);
			if (!r) return 0;
			own_reader = 1;
		}

		int numsongs, firstsong;

		if (!get_nez_info(r, &firstsong, &numsongs, filename, NULL)) goto retn;
		
		for (n = firstsong; n <= numsongs; n++)
			out->on_entry(make_playlist_entry(filename,n));
		
		ret = 1;
retn:
		if (own_reader) r->reader_release();
		return ret;
	}
};

static cfg_dropdown_history history_rate("rate_history",16);

static const int srate_tab[]={8000,11025,16000,22050,24000,32000,44100,48000,64000,88200,96000};

static BOOL CALLBACK ConfigProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			HWND w;
			char temp[16];
			int n;
			for(n=tabsize(srate_tab);n--;)
			{
				if (srate_tab[n] != cfg_srate)
				{
					itoa(srate_tab[n], temp, 10);
					history_rate.add_item(temp);
				}
			}
			itoa(cfg_srate, temp, 10);
			history_rate.add_item(temp);
			history_rate.setup_dropdown(w = GetDlgItem(wnd,IDC_SAMPLERATE));
			uSendMessage(w, CB_SETCURSEL, 0, 0);
			uSendDlgItemMessage(wnd, IDC_NONSF, BM_SETCHECK, cfg_nonsf, 0);
		}
		return 1;
	case WM_COMMAND:
		switch(wp)
		{
		/*
		case (CBN_SELCHANGE<<16)|IDC_SAMPLERATE:
			{
				string8 meh;
				history_rate.get_item(meh, SendMessage((HWND)lp,CB_GETCURSEL,0,0));
				cfg_srate = atoi(meh);
			}
			break;
		case (CBN_EDITCHANGE<<16)|IDC_SAMPLERATE:
		*/
		case (CBN_KILLFOCUS<<16)|IDC_SAMPLERATE:
			{
				int t = GetDlgItemInt(wnd,IDC_SAMPLERATE,0,0);
				if (t<6000) t=6000;
				else if (t>192000) t=192000;
				cfg_srate = t;
			}
			break;
		case IDC_NONSF:
			cfg_nonsf = uSendMessage((HWND)lp, BM_GETCHECK, 0, 0);
			break;
		}
		break;
	case WM_DESTROY:
		char temp[16];
		itoa(cfg_srate, temp, 10);
		history_rate.add_item(temp);
		break;
	}
	return 0;
}

class config_nez : public config
{
public:
	virtual HWND create(HWND parent)
	{
		return uCreateDialog(IDD_FOO_CONFIG,parent,ConfigProc);
	}

	virtual const char * get_name() {return "NEZplug";}
	virtual const char * get_parent_name() {return "Input";}
};

class nez_file_types : public input_file_type
{
	virtual unsigned get_count()
	{
		return 1;
	}

	virtual bool get_name(unsigned idx, string_base & out)
	{
		if (idx > 0) return false;
		out = "Nezplug files";
		return true;
	}

	virtual bool get_mask(unsigned idx, string_base & out)
	{
		if (idx > 0) return false;
		out = "*.KSS;*.GBR;*.GBS;*.HES;*.PCE;*.AY;*.CPC";
		if (!cfg_nonsf) out += ";*.NSF";
		return true;
	}
};

static service_factory_t<input,input_nez> foo;
static service_factory_single_t<track_indexer,track_indexer_nez> foo2;
static service_factory_single_t<input_file_type,nez_file_types> foo3;
static service_factory_single_t<config,config_nez> foo4;

DECLARE_COMPONENT_VERSION("NEZplug", "0.9.4.8", "Plays NSF, KSS, GBR, GBS, HES, PCE, AY, CPC files.");
