#define MYVERSION "0.92"

/*
	changelog

2004-09-09 19:42 UTC - kode54
- Updated BASS to 2.0.0.22
- Changed S3M and IT info readers to translate using DOS codepage
- Added sample/instrument counts to IT info reader

2004-09-06 16:30 UTC - kode54
- Made 44100Hz the default sample rate

2004-09-06 01:51 UTC - kode54
- Optimized the audio to chunk setting to use set_data_32, finally...

2004-06-15 18:51 UTC - kode54
- Should now retrieve basic module info even when a tag is present
- Version is now 0.92

2004-05-24 15:12 UTC - kode54
- No longer allows seeking past the end of the file
- Version is now 0.91

2004-03-14 00:00 UTC - kode54
- Now it will no longer accept zero-length files
- Version is now 0.90

2004-02-18 01:18 UTC - kode54
- Added detection for MO3, so it won't hit the MOD info loader
- Completed IT info loader
- Corrected IT info loader version information (BCD, not decimal)
- Version is now 0.89

2003-11-11 04:19 UTC - kode54
- Changed BASS_ERROR_FORMAT to new BASS_ERROR_FILEFORM
- Version is now 0.88a

2003-11-09 12:29 UTC - kode54
- Updated to BASS 2.0 final
- Version is now 0.88

2003-10-10 21:54 UTC - kode54
- Added sample/comment info parsing for MOD/S3M/MTM
- Version is now 0.87

2003-10-06 18:29 UTC - kode54
- Moved BASS_Free() call to global destructor
- Version is now 0.86g

2003-10-05 04:42 UTC - kode54
- Blah, forgot to set r to the new unpacking reader after unpacker::g_open()
- Version is now 0.86ffs

2003-10-02 19:25 UTC - kode54
- Forgot to fucking initialize played to 0. when not looping
- Version is now 0.86e

2003-10-01 17:32 UTC - kode54
- Even more console info!
- Version is now 0.86d

2003-10-01 16:14 UTC - kode54
- Added more error info
- Changed error handling on BASS_MusicGetLength()
- Version is now 0.86c

2003-10-01 14:02 UTC - kode54
- Changed critical section crap around BASS init stuff
- Changed extension check in 15/31 sample converter
- Inserted reader_disposer because there were several returns that didn't release bloody unpacking reader
- Version is now 0.86b

2003-09-21 14:18 UTC - kode54
- Looping is now disabled by default
- Single-play behavior amended to loop internally and cut playback at the detected length
- Version is now 0.86

2003-07-20 02:18 - kode54
- Amended seeking function
- Version is now 0.85

2003-06-28 08:04 - kode54
- Aww, fuck. Windows 2000 ignoring (lack of) integral height attribute on
  combo boxes, so the MOD playback box which was sized too small failed to
  open. Meh.
- Version is now 0.81

2003-06-26 07:02 - kode54
- Updated to 0.7 API
- Changed BASS version error string slightly
- Version is now 0.8

2003-04-30 10:36 - kode54
- Fixed lame bug with tag reading and compressed archives
- Added compressed module check to tag writer

2003-04-30 10:09 - kode54
- Added tag reading and optional writing
- Version is now 0.7

2003-04-22 08:44 - kode54
- Init BASS like normal, but don't shut it down until player closes.
  Unfortunately, this locks sample rate changes until the next startup.
- Version is now 0.666

2003-04-09 16:25 - kode54
- Added bitspersample info
- File name is no longer hard coded
- Changed about dialog string slightly
- Version is now 0.65

*/

#include "../SDK/foobar2000.h"
#include "../helpers/dropdown_helper.h"

#include "resource.h"
#include <stdio.h>
#include <commctrl.h>

#include "bass.h"

// #define DBG(a) OutputDebugString(a)
#define DBG(a)

static long bass_init = 0;
static int srate;

static critical_section BASSCritical;

static const char * exts[]=
{
	// Original set copied from foo_mod, perhaps most frequently used types
	"MOD","MDZ",
	"S3M","S3Z",
	"IT","ITZ",
	"XM","XMZ",
	"MTM",
	"UMX",
	"MO3",
};

static cfg_int cfg_samplerate("srate",44100);
static cfg_int cfg_interp("interp",1);

static cfg_int cfg_loop("mod_loop",0);
static cfg_int cfg_surround("mod_surround",0);
static cfg_int cfg_volramp("mod_volramp",1);
static cfg_int cfg_modstyle("mod_style",0);
static cfg_int cfg_sseek("mod_sseek",0);

static cfg_int cfg_tag("mod_tag", 0);

// bah
class bass_shutdown
{
public:
	bass_shutdown() {}
	~bass_shutdown()
	{
		if (bass_init > 0)
		{
			BASS_Free();
		}
	}
};

bass_shutdown asdf;

unsigned convert_oem_to_utf16(const char * src, WCHAR * dst, unsigned len)
{
	len = strlen_max(src, len);
	unsigned rv;
	rv = MultiByteToWideChar(CP_OEMCP, 0, src, len, dst, estimate_ansi_to_utf16(src, len));
	if ((signed)rv < 0) rv = 0;
	dst[rv] = 0;
	return rv;
}

unsigned convert_oem_to_utf8(const char * src,char * dst,unsigned len)
{
	len = strlen_max(src,len);

	unsigned temp_len = estimate_ansi_to_utf16(src,len);
	mem_block_t<WCHAR> temp_block;
	WCHAR * temp = (temp_len * sizeof(WCHAR) <= PFC_ALLOCA_LIMIT) ? (WCHAR*)alloca(temp_len * sizeof(WCHAR)) : temp_block.set_size(temp_len);
	assert(temp);

	len = convert_oem_to_utf16(src,temp,len);
	return convert_utf16_to_utf8(temp,dst,len);
}

class string_utf8_from_oem : public string_convert_base<char>
{
public:
	explicit string_utf8_from_oem(const char * src, unsigned len = -1)
	{
		len = strlen_max(src, len);
		alloc(len * 3 + 1);
		convert_oem_to_utf8(src, ptr, len);
	}
};

class reader_disposer
{
private:
	reader * m_r;
public:
	reader_disposer(reader * r) { m_r = r; }
	~reader_disposer() { if (m_r) m_r->reader_release(); }
	void attach(reader * r) { m_r = r; }
};

#pragma pack(push)
#pragma pack(1)

static const char field_channels[] = "mod_channels";
static const char field_samples[] = "mod_samples";
static const char field_instruments[] = "mod_instruments";
static const char field_trackerver[] = "mod_ver_tracker";
static const char field_sample[] = "smpl";
static const char field_instrument[] = "inst";
static const char field_pattern[] = "patt";
static const char field_channel[] = "chan";
static const char field_comment[] = "comment";

typedef struct _MODSAMPLE
{
	CHAR name[22];
	WORD length;
	BYTE finetune;
	BYTE volume;
	WORD loopstart;
	WORD looplen;
} MODSAMPLE, *PMODSAMPLE;

typedef struct _MODMAGIC
{
	BYTE nOrders;
	BYTE nRestartPos;
	BYTE Orders[128];
        char Magic[4];          // changed from CHAR
} MODMAGIC, *PMODMAGIC;

#pragma pack(pop)

static bool IsMagic(LPCSTR s1, LPCSTR s2)
{
	return ((*(DWORD *)s1) == (*(DWORD *)s2)) ? TRUE : FALSE;
}

static bool ReadMod(const BYTE * ptr, unsigned size, file_info * info, bool meta)
{
	char s[4];          // changed from CHAR
	DWORD dwMemPos;
	DWORD nSamples, nChannels;
	PMODMAGIC pMagic;
	
	if ((!ptr) || (size < 0x600)) return false;
	dwMemPos = 20;
	nSamples = 31;
	nChannels = 4;
	pMagic = (PMODMAGIC)(ptr+dwMemPos+sizeof(MODSAMPLE)*31);
	// Check Mod Magic
	memcpy(s, pMagic->Magic, 4);
	if ((IsMagic(s, "M.K.")) || (IsMagic(s, "M!K!"))
	 || (IsMagic(s, "M&K!")) || (IsMagic(s, "N.T."))) nChannels = 4; else
	if ((IsMagic(s, "CD81")) || (IsMagic(s, "OKTA"))) nChannels = 8; else
	if ((s[0]=='F') && (s[1]=='L') && (s[2]=='T') && (s[3]>='4') && (s[3]<='9')) nChannels = s[3] - '0'; else
	if ((s[0]>='4') && (s[0]<='9') && (s[1]=='C') && (s[2]=='H') && (s[3]=='N')) nChannels = s[0] - '0'; else
	if ((s[0]=='1') && (s[1]>='0') && (s[1]<='9') && (s[2]=='C') && (s[3]=='H')) nChannels = s[1] - '0' + 10; else
	if ((s[0]=='2') && (s[1]>='0') && (s[1]<='9') && (s[2]=='C') && (s[3]=='H')) nChannels = s[1] - '0' + 20; else
	if ((s[0]=='3') && (s[1]>='0') && (s[1]<='2') && (s[2]=='C') && (s[3]=='H')) nChannels = s[1] - '0' + 30; else
	if ((s[0]=='T') && (s[1]=='D') && (s[2]=='Z') && (s[3]>='4') && (s[3]<='9')) nChannels = s[3] - '0'; else
	if (IsMagic(s,"16CN")) nChannels = 16; else
	if (IsMagic(s,"32CN")) nChannels = 32; else nSamples = 15;

	info->info_set("codec", "MOD");
	info->info_set_int(field_channels, nChannels);
	info->info_set_int(field_samples, nSamples);

	if (meta)
	{
		PMODSAMPLE pms = (PMODSAMPLE)(ptr+dwMemPos);

		for (unsigned i = 0; i < nSamples; i++)
		{
			if (*pms->name)
			{
				string8 name(field_sample), val(pms->name, 22);
				if (i < 10) name.add_byte('0');
				name.add_int(i);
				info->meta_add_ansi(name, val);
			}
			pms++;
		}
	}

	return true;
}

#pragma pack(push)
#pragma pack(1)

typedef struct tagS3MSAMPLESTRUCT
{
	BYTE type;
	CHAR dosname[12];
	BYTE hmem;
	WORD memseg;
	DWORD length;
	DWORD loopbegin;
	DWORD loopend;
	BYTE vol;
	BYTE bReserved;
	BYTE pack;
	BYTE flags;
	DWORD finetune;
	DWORD dwReserved;
	WORD intgp;
	WORD int512;
	DWORD lastused;
	CHAR name[28];
	CHAR scrs[4];
} S3MSAMPLESTRUCT, *PS3MSAMPLESTRUCT;


typedef struct tagS3MFILEHEADER
{
	CHAR name[28];
	BYTE b1A;
	BYTE type;
	WORD reserved1;
	WORD ordnum;
	WORD insnum;
	WORD patnum;
	WORD flags;
	WORD cwtv;
	WORD version;
	DWORD scrm;	// "SCRM" = 0x4D524353
	BYTE globalvol;
	BYTE speed;
	BYTE tempo;
	BYTE mastervol;
	BYTE ultraclicks;
	BYTE panning_present;
	BYTE reserved2[8];
	WORD special;
	BYTE channels[32];
} S3MFILEHEADER, *PS3MFILEHEADER;

#pragma pack(pop)

static bool ReadS3M(const BYTE * ptr, unsigned size, file_info * info, bool meta)
{
	UINT insnum;
	DWORD nChannels, dwMemPos;
	PS3MFILEHEADER psfh = (PS3MFILEHEADER)ptr;

	if ((!ptr) || (size <= sizeof(S3MFILEHEADER)+sizeof(S3MSAMPLESTRUCT)+64)) return false;
	if (psfh->scrm != 0x4D524353) return false;
	dwMemPos = 0x60;
	nChannels = 4;
	for (UINT ich=0; ich<32; ich++)
	{
		if (psfh->channels[ich] != 0xFF)
		{
			nChannels = ich+1;
		}
	}

	UINT iord = psfh->ordnum;
	if (iord<1) iord = 1;
	if (iord)
	{
		dwMemPos += iord;
	}
	if ((iord & 1) && (ptr[dwMemPos] == 0xFF)) dwMemPos++;

	insnum = psfh->insnum;

	info->info_set("codec", "S3M");
	info->info_set_int(field_channels, nChannels);
	info->info_set_int(field_samples, insnum);

	if (meta)
	{
		string8_fastalloc name;
		WORD ptrs[256];
		memset(ptrs, 0, sizeof(ptrs));
		memcpy(ptrs, ptr + dwMemPos, 2 * insnum);

		for (UINT i = 0; i < insnum; i++)
		{
			UINT nInd = ((DWORD)ptrs[i]) * 16;
			if ((!nInd) || (nInd + 0x50 > size)) continue;
			PS3MSAMPLESTRUCT pss = (PS3MSAMPLESTRUCT) (ptr + nInd);
			if (*pss->name)
			{
				name = field_sample;
				if (i < 10) name.add_byte('0');
				name.add_int(i);
				info->meta_add(name, string_utf8_from_oem(pss->name, 28));
			}
		}
	}

	return true;
}

static bool ReadXM(const BYTE * ptr, unsigned size, file_info * info, bool meta)
{
	if ((!ptr) || (size < 0x200)) return false;
	if (strnicmp((LPCSTR)ptr, "Extended Module", 15)) return false;

	info->info_set("codec", "XM");

	return true;
}

#pragma pack(push)
#pragma pack(1)

typedef struct tagITFILEHEADER
{
	DWORD id;			// 0x4D504D49
	CHAR songname[26];
	WORD reserved1;		// 0x1004
	WORD ordnum;
	WORD insnum;
	WORD smpnum;
	WORD patnum;
	WORD cwtv;
	WORD cmwt;
	WORD flags;
	WORD special;
	BYTE globalvol;
	BYTE mv;
	BYTE speed;
	BYTE tempo;
	BYTE sep;
	BYTE zero;
	WORD msglength;
	DWORD msgoffset;
	DWORD reserved2;
	BYTE chnpan[64];
	BYTE chnvol[64];
} ITFILEHEADER, *PITFILEHEADER;

typedef struct MODMIDICFG
{
        char szMidiGlb[9*32];      // changed from CHAR
        char szMidiSFXExt[16*32];  // changed from CHAR
        char szMidiZXXExt[128*32]; // changed from CHAR
} MODMIDICFG, *LPMODMIDICFG;

#pragma pack(pop)

static bool ReadIT(const BYTE * ptr, unsigned size, file_info * info, bool meta)
{
	PITFILEHEADER pifh = (PITFILEHEADER) ptr;
	if ((!ptr) || (size < 0x100)) return false;
	if ((pifh->id != 0x4D504D49) || (pifh->insnum >= 256)
	 || (!pifh->smpnum) || (pifh->smpnum >= 256) || (!pifh->ordnum)) return false;
	if (sizeof(ITFILEHEADER) + pifh->ordnum + pifh->insnum*4
	 + pifh->smpnum*4 + pifh->patnum*4 > size) return false;

	string8_fastalloc ver;

	ver = "IT v";
	ver.add_int(pifh->cmwt >> 8);
	ver.add_byte('.');
	ver.add_int((pifh->cmwt >> 4) & 15, 16);
	ver.add_int(pifh->cmwt & 15, 16);
	info->info_set("codec", ver);

	ver.reset();
	ver.add_int(pifh->cwtv >> 8);
	ver.add_byte('.');
	ver.add_int((pifh->cwtv >> 4) & 15, 16);
	ver.add_int(pifh->cwtv & 15, 16);
	info->info_set(field_trackerver, ver);

	if (pifh->smpnum) info->info_set_int(field_samples, pifh->smpnum);
	if (pifh->insnum) info->info_set_int(field_instruments, pifh->insnum);

	unsigned n, l, m_nChannels = 0;

	if (meta && (pifh->special & 1) && (pifh->msglength) && (pifh->msgoffset + pifh->msglength < size))
	{
		string8 msg;
		const char * str = (const char *) ptr + pifh->msgoffset;
		for (n = 0, l = pifh->msglength; n < l; n++, str++)
		{
			if (*str == 13)
			{
				if ((n + 1 >= l) || (str[1] != 10))
				{
					msg.add_byte(13);
					msg.add_byte(10);
					continue;
				}
			}
			else if (*str == 10)
			{
				if ((!n) || (str[-1] != 13))
				{
					msg.add_byte(13);
					msg.add_byte(10);
					continue;
				}
			}
			msg.add_byte(*str);
		}
		info->meta_add(field_comment, string_utf8_from_oem(msg));
	}

	unsigned * offset;
	string8_fastalloc name;
	
	if (meta)
	{
		offset = (unsigned *)(ptr + 0xC0 + pifh->ordnum + pifh->insnum * 4);

		for (n = 0, l = pifh->smpnum; n < l; n++, offset++)
		{
			if ((!*offset) || (*offset >= size)) continue;
			if (*(ptr + *offset + 0x14))
			{
				name = field_sample;
				if (n < 10) name.add_byte('0');
				name.add_int(n);
				info->meta_add(name, string_utf8_from_oem((const char *) ptr + *offset + 0x14, 26));
			}
		}

		offset = (unsigned *)(ptr + 0xC0 + pifh->ordnum);

		for (n = 0, l = pifh->insnum; n < l; n++, offset++)
		{
			if ((!*offset) || (*offset >= size)) continue;
			if (*(ptr + *offset + 0x20))
			{
				name = field_instrument;
				if (n < 10) name.add_byte('0');
				name.add_int(n);
				info->meta_add(name, string_utf8_from_oem((const char *) ptr + *offset + 0x20, 26));
			}
		}
	}

	unsigned pos = 0xC0 + pifh->ordnum + pifh->insnum * 4 + pifh->smpnum * 4 + pifh->patnum * 4;

	if (pos < size)
	{
		unsigned short val16 = *(unsigned short *)(ptr + pos);
		pos += 2;
		if (pos + val16 * 8 < size) pos += val16 * 8;
	}

	if (pifh->flags & 0x80)
	{
		if (pos + sizeof(MODMIDICFG) < size)
		{
			pos += sizeof(MODMIDICFG);
		}
	}

	if ((pos + 8 < size) && (*(DWORD *)(ptr + pos) == 0x4d414e50)) // "PNAM"
	{
		unsigned len = *(DWORD *)(ptr + pos + 4);
		pos += 8;
		if ((pos + len <= size) && (len <= 240 * 32) && (len >= 32))
		{
			if (meta)
			{
				l = len / 32;
				for (n = 0; n < l; n++)
				{
					if (*(ptr + pos + n * 32))
					{
						name = field_pattern;
						if (n < 10) name.add_byte('0');
						name.add_int(n);
						info->meta_add(name, string_utf8_from_oem((const char *) ptr + pos + n * 32, 32));
					}
				}
			}
			pos += len;
		}
	}

	if ((pos + 8 < size) && (*(DWORD *)(ptr + pos) == 0x4d414e43)) // "CNAM"
	{
		unsigned len = *(DWORD *)(ptr + pos + 4);
		pos += 8;
		if ((pos + len <= size) && (len <= 64 * 20) && (len >= 20))
		{
			l = len / 20;
			m_nChannels = l;
			if (meta)
			{
				for (n = 0; n < l; n++)
				{
					if (*(ptr + pos + n * 20))
					{
						name = field_channel;
						if (n < 10) name.add_byte('0');
						name.add_int(n);
						info->meta_add(name, string_utf8_from_oem((const char *) ptr + pos + n * 20, 20));
					}
				}
			}
			pos += len;
		}
	}

	offset = (unsigned *)(ptr + 0xC0 + pifh->ordnum + pifh->insnum * 4 + pifh->smpnum * 4);

	BYTE chnmask[64];

	for (n = 0, l = pifh->patnum; n < l; n++)
	{
		memset(chnmask, 0, sizeof(chnmask));
		if ((!offset[n]) || (offset[n] + 4 >= size)) continue;
		unsigned len = *(WORD *)(ptr + offset[n]);
		unsigned rows = *(WORD *)(ptr + offset[n] + 2);
		if ((rows < 4) || (rows > 256)) continue;
		if (offset[n] + 8 + len > size) continue;
		unsigned i = 0;
		const BYTE * p = ptr + offset[n] + 8;
		unsigned nrow = 0;
		while (nrow < rows)
		{
			if (i >= len) break;
			BYTE b = p[i++];
			if (!b)
			{
				nrow++;
				continue;
			}
			UINT ch = b & 0x7F;
			if (ch) ch = (ch - 1) & 0x3F;
			if (b & 0x80)
			{
				if (i >= len) break;
				chnmask[ch] = p[i++];
			}
			// Channel used
			if (chnmask[ch] & 0x0F)
			{
				if ((ch >= m_nChannels) && (ch < 64)) m_nChannels = ch+1;
			}
			// Note
			if (chnmask[ch] & 1) i++;
			// Instrument
			if (chnmask[ch] & 2) i++;
			// Volume
			if (chnmask[ch] & 4) i++;
			// Effect
			if (chnmask[ch] & 8) i += 2;
			if (i >= len) break;
		}
	}

	if (m_nChannels) info->info_set_int(field_channels, m_nChannels);

	return true;
}

#pragma pack(push)
#pragma pack(1)

typedef struct tagMTMSAMPLE
{
        char samplename[22];      // changed from CHAR
	DWORD length;
	DWORD reppos;
	DWORD repend;
	CHAR finetune;
	BYTE volume;
	BYTE attribute;
} MTMSAMPLE;


typedef struct tagMTMHEADER
{
	char id[4];	        // MTM file marker + version // changed from CHAR
	char songname[20];	// ASCIIZ songname  // changed from CHAR
	WORD numtracks;		// number of tracks saved
	BYTE lastpattern;	// last pattern number saved
	BYTE lastorder;		// last order number to play (songlength-1)
	WORD commentsize;	// length of comment field
	BYTE numsamples;	// number of samples saved
	BYTE attribute;		// attribute byte (unused)
	BYTE beatspertrack;
	BYTE numchannels;	// number of channels used
	BYTE panpos[32];	// voice pan positions
} MTMHEADER;

#pragma pack(pop)

static bool ReadMTM(const BYTE * ptr, unsigned size, file_info * info, bool meta)
{
	MTMHEADER *pmh = (MTMHEADER *)ptr;
	DWORD dwMemPos = 66;
	DWORD nSamples, nChannels;

	if ((!ptr) || (size < 0x100)) return false;
	if ((strncmp(pmh->id, "MTM", 3)) || (pmh->numchannels > 32)
	 || (pmh->numsamples >= 256) || (!pmh->numsamples)
	 || (!pmh->numtracks) || (!pmh->numchannels)
	 || (!pmh->lastpattern) || (pmh->lastpattern > 256)) return false;
	if (dwMemPos + 37*pmh->numsamples + 128 + 192*pmh->numtracks
	 + 64 * (pmh->lastpattern+1) + pmh->commentsize >= size) return false;
	nSamples = pmh->numsamples;
	nChannels = pmh->numchannels;
	// Reading instruments

	info->info_set("codec", "MTM");
	info->info_set_int(field_channels, nChannels);
	info->info_set_int(field_samples, nSamples);

	if (meta)
	{
		string8_fastalloc name;
		for	(UINT i=0; i<nSamples; i++)
		{
			MTMSAMPLE *pms = (MTMSAMPLE *)(ptr + dwMemPos);
			if (*pms->samplename)
			{
				name = field_sample;
				if (i < 10) name.add_byte('0');
				name.add_int(i);
				info->meta_add(name, string_utf8_from_oem(pms->samplename, 22));
			}
			dwMemPos += 37;
		}
		dwMemPos += 128;
		dwMemPos += 192 * pmh->numtracks;
		dwMemPos += 64*(pmh->lastpattern+1);
		if ((pmh->commentsize) && (dwMemPos + pmh->commentsize < size))
		{
			name.reset();
			for (UINT i = pmh->commentsize / 40; i--;)
			{
				if (*(ptr + dwMemPos))
				{
					if (name.length())
					{
						name.add_byte(13);
						name.add_byte(10);
					}
					name.add_string((const char *) ptr + dwMemPos, 40);
				}
				dwMemPos += 40;
			}
			info->meta_add(field_comment, string_utf8_from_oem(name));
		}
	}

	return true;
}

#define MODMAGIC_OFFSET	(20+31*30+130)

static bool ReadUMX(const BYTE * ptr, unsigned size, file_info * info, bool meta)
{
	if (*(DWORD *)ptr != 0x9E2A83C1) return false;
	if ((*((DWORD *)(ptr + 0x20)) < size) &&
		(*((DWORD *)(ptr + 0x18)) <= size - 0x10) &&
		(*((DWORD *)(ptr + 0x18)) >= size - 0x200))
	{
		for (UINT uscan=0x40; uscan<0x500; uscan++)
		{
			DWORD dwScan = *((DWORD *)(ptr + uscan));
			// IT
			if (dwScan == 0x4D504D49)
			{
				DBG("IT");
				return ReadIT(ptr + uscan, size - uscan, info, meta);
			}
			// S3M
			if (dwScan == 0x4D524353)
			{
				DBG("S3M");
				DWORD dwRipOfs = uscan - 44;
				return ReadS3M(ptr + dwRipOfs, size - dwRipOfs, info, meta);
			}
			// XM
			if (!strnicmp((LPCSTR)(ptr+uscan), "Extended Module", 15))
			{
				DBG("XM");
				return ReadXM(ptr + uscan, size - uscan, info, meta);
			}
			// MOD
			if ((uscan > MODMAGIC_OFFSET) && (dwScan == 0x2E4B2E4D))
			{
				DBG("MOD");
				DWORD dwRipOfs = uscan - MODMAGIC_OFFSET;
				return ReadMod(ptr + dwRipOfs, size - dwRipOfs, info, meta);
			}
		}
	}
	return false;
}

static bool ReadMO3(const BYTE * ptr, unsigned size, file_info * info, bool meta)
{
	if ((!ptr) || (size < 3)) return false;
	if (memcmp(ptr, "MO3", 3)) return false;

	info->info_set("codec", "MO3");

	return true;
}

static void get_sample_info(const BYTE * ptr, unsigned size, file_info * info, bool meta)
{
	if (ReadS3M(ptr, size, info, meta)) return;
	if (ReadXM(ptr, size, info, meta)) return;
	if (ReadIT(ptr, size, info, meta)) return;
	if (ReadMO3(ptr, size, info, meta)) return;
	if (ReadMTM(ptr, size, info, meta)) return;
	if (ReadUMX(ptr, size, info, meta)) return;
	if (ReadMod(ptr, size, info, meta)) return;
}

class input_mod : public input
{
public:

	virtual bool is_our_content_type(const char *url, const char *type)
	{
		if (bass_init == -1) return 0;
		return !strcmp(type, "audio/x-mod");
	}

	virtual bool test_filename(const char * fn,const char * ext)
	{
		if (bass_init == -1) return 0;
		int n;
		for(n=0;n<tabsize(exts);n++)
		{
			if (!stricmp(ext,exts[n])) return 1;
		}
		return 0;
	}

	virtual bool open( reader * r, file_info * info,unsigned openflags)
	{
		reader_disposer disposer(0);
		mem_block_t<BYTE> temp;
		reader * unpack = unpacker::g_open(r);
		if (unpack)
		{
			r = unpack;
			disposer.attach(r);
		}
		__int64 size64 = r->get_length();
		BYTE *ptr;
		if (size64 < 1 || size64 > (1<<30))
		{
			console::warning("Illegal file size, <1 or >(1<<30)");
			return 0;
		}

		unsigned size = (int)size64;

//		OutputDebugString("allocating buffer");
		ptr = temp.set_size(size);

//		OutputDebugString("reading file");
		if (r->read(ptr,size)!=size)
		{
			console::warning("Error reading file, or read back more or less than expected file size.");
			return 0;
		}

		srate = cfg_samplerate;

		{
			insync(BASSCritical);

			if (!bass_init)
			{
				int bver = BASS_GetVersion();
				if ((bver & 0xFFFF) < 2)
				{
					string8 err = "BASS.dll too old (Wanted v2.0 or newer, found v";
					err.add_int(bver & 0xFFFF);
					err.add_byte('.');
					err.add_int(bver >> 16);
					err.add_byte(')');
					console::error(err);
					bass_init = -1;
					return 0;
				}
				if (!BASS_Init(0, srate, 0, core_api::get_main_window(), 0))
				{
					console::error(uStringPrintf("BASS_Init() returned error: %d", BASS_ErrorGetCode()));
					return 0;
				}
			}

			bass_init = 1;
		}

//		OutputDebugString("loading module");

		DWORD flags = BASS_SAMPLE_FLOAT |
					  BASS_MUSIC_DECODE |
					  BASS_MUSIC_CALCLEN |
					  BASS_MUSIC_LOOP;

		if (!(openflags & OPEN_FLAG_DECODE)) flags |= BASS_MUSIC_NOSAMPLE;

		if (openflags & OPEN_FLAG_DECODE)
		{
			if (!cfg_interp) flags |= BASS_MUSIC_NONINTER;
			switch (cfg_surround)
			{
			case 1:
				flags |= BASS_MUSIC_SURROUND;
				break;
			case 2:
				flags |= BASS_MUSIC_SURROUND2;
				break;
			}
			switch (cfg_volramp)
			{
			case 1:
				flags |= BASS_MUSIC_RAMP;
				break;
			case 2:
				flags |= BASS_MUSIC_RAMPS;
				break;
			}
			switch (cfg_modstyle)
			{
			case 1:
				flags |= BASS_MUSIC_FT2MOD;
				break;
			case 2:
				flags |= BASS_MUSIC_PT1MOD;
				break;
			}
			if (cfg_sseek) flags |= BASS_MUSIC_POSRESET;
		}

/*
		DBG("Loading");
#define MODMAGIC_OFFSET	(20+31*30+130)

		if (size >= 0x800 && *((DWORD*)ptr) == 0x9E2A83C1)
		{
			DBG("UMX");
			/* woot, let's scan the UMX, borrowed from libmodplug
			  ... since BASS.dll v1.8 seems to be crashing on mem
			  load of UMX. Bleh.
			if ((*((DWORD *)(ptr + 0x20)) < size) &&
				(*((DWORD *)(ptr + 0x18)) <= size - 0x10) &&
				(*((DWORD *)(ptr + 0x18)) >= size - 0x200))
			{
				for (UINT uscan=0x40; uscan<0x500; uscan++)
				{
					DWORD dwScan = *((DWORD *)(ptr + uscan));
					// IT
					if (dwScan == 0x4D504D49)
					{
						DBG("IT");
						ptr += uscan;
						size -= uscan;
						break;
					}
					// S3M
					if (dwScan == 0x4D524353)
					{
						DBG("S3M");
						DWORD dwRipOfs = uscan - 44;
						ptr += dwRipOfs;
						size -= dwRipOfs;
						break;
					}
					// XM
					if (!strnicmp((LPCSTR)(ptr+uscan), "Extended Module", 15))
					{
						DBG("XM");
						ptr += uscan;
						size -= uscan;
						break;
					}
					// MOD
					if ((uscan > MODMAGIC_OFFSET) && (dwScan == 0x2E4B2E4D))
					{
						DBG("MOD");
						DWORD dwRipOfs = uscan - MODMAGIC_OFFSET;
						ptr += dwRipOfs;
						size -= dwRipOfs;
						break;
					}
				}
			}
		}

		char err[16];
		wsprintf(err, "%d", ptr - temp.get_ptr());
		DBG(err);
		*/

		chan = BASS_MusicLoad(TRUE, ptr, 0, size, flags, srate);

		if (!chan)
		{
			// hack time
			int err = BASS_ErrorGetCode();
			if (err != BASS_ERROR_FILEFORM)
			{
				console::warning(uStringPrintf("BASS_MusicLoad() returned error: %d", err));
				return 0;
			}
			string_extension_8 ext(info->get_file_path());
			if (stricmp(ext, exts[0]) && stricmp(ext, exts[1])) return 0;
			if (size < 20+30*15+130 + 1) return 0;
			mem_block_t<char> foo;
			char *bar = foo.set_size(size + 30*16+4);
			memcpy(bar, ptr = temp.get_ptr(), 20+30*15);
			memset(bar + 20+30*15, 0, 30*16);
			memcpy(bar + 20+30*31, ptr + 20+30*15, 130);
			*(int*)(bar + 20+30*31+130) = '.K.M';
			memcpy(bar + 20+30*31+134, ptr + 20+30*15+130, size - (20+30*15+130));
			size += 30*16+4;
			chan = BASS_MusicLoad(TRUE, bar, 0, size, flags, srate);
		}

		DBG("loaded");

		if (!chan)
		{
			console::warning(uStringPrintf("BASS_MusicLoad() returned error, possibly after conversion: %d", BASS_ErrorGetCode()));
			return 0;
		}

		if (info->get_subsong_index()) BASS_ChannelSetPosition(chan, (QWORD)MAKELONG(info->get_subsong_index(),0));

//		OutputDebugString("module loaded");

		DBG("setting info");

		info->info_set_int("samplerate", srate);
		//info->info_set_int("bitspersample", 32);
		//info->info_set("extrainfo","module");

		DBG("name");

		if (openflags & OPEN_FLAG_GET_INFO)
		{
			if (!tag_reader::g_run_multi(r,info,"ape|id3v1"))
			{
				char *name = BASS_MusicGetName(chan);
				if (name && *name) info->meta_add_ansi("title", name);
				get_sample_info(temp.get_ptr(), temp.get_size(), info, true);
			}
			else
				get_sample_info(temp.get_ptr(), temp.get_size(), info, false);
		}

		temp.set_size(0);
		DBG("buffers away");

		DBG("length");
		int pos = BASS_MusicGetLength(chan,TRUE);
		if (pos >= 0)
		{
			length = BASS_ChannelBytes2Seconds(chan,pos);
			if (length >= 0.) info->set_length(length);
		}
		else
		{
			// wtfh?
			console::warning(uStringPrintf("BASS_MusicGetLength() returned error: %d", BASS_ErrorGetCode()));
			length = 0.;
		}

		no_loop = !!(openflags & OPEN_FLAG_NO_LOOPING) || !cfg_loop;

		if (no_loop) played = 0.;

		return 1;
	}
	input_mod()
	{
		chan = 0;
	}

	~input_mod()
	{
		if (chan) BASS_MusicFree(chan);
	}

	virtual input::set_info_t set_info(reader *r,const file_info * info)
	{
		if (cfg_tag)
		{
			reader * unpack = unpacker::g_open(r);
			if (unpack)
			{
				unpack->reader_release();
				return SET_INFO_FAILURE;
			}
			else
			{
				tag_remover::g_run(r);
				return tag_writer::g_run(r, info,"ape") ? SET_INFO_SUCCESS : SET_INFO_FAILURE;
			}
		}
		else
		{
			return SET_INFO_FAILURE;
		}
	}

	virtual int run(audio_chunk * chunk)
	{
		if (no_loop)
		{
			if (length && played >= length)
			{
				return 0;
			}
		}


		DWORD written=0;

		int samples = 576 * 2; //(stereo)
		
		written=BASS_ChannelGetData(chan, buf.check_size(samples), samples * sizeof(float));

		if (written == 0 || written == -1)
		{
			if (written < 0) console::warning(uStringPrintf("BASS_ChannelGetData() returned error: %d", BASS_ErrorGetCode()));
			return 0;
		}

		chunk->set_data_32(buf.get_ptr(), written / (2 * sizeof(float)), 2, srate);
/*
#if audio_sample_size == 64
		float *in = (float*)(((char*)buf.get_ptr()) + written);
		audio_sample *out = (audio_sample*)(((char*)buf.get_ptr()) + written * 2);
		for (int i = written / sizeof(float); i; i--)
		{
			*--out = (audio_sample) *--in;
		}
		chunk->set_data(out,written / (sizeof(float) * 2),2,srate);
#else if audio_sample_size == 32
		chunk->set_data(buf,written / (sizeof(float) * 2),2,srate);
#endif
*/

		if (no_loop)
		{
			played += chunk->get_duration();

			if (played > length)
			{
				int sc = chunk->get_sample_count() - (int)((played - length) * srate);
				if (sc<0) sc=0;
				chunk->set_sample_count(sc);
			}
		}

		return 1;
	}

	virtual bool seek(double pos64)
	{
		if (pos64 >= length)
		{
			no_loop = 1;
			length = 1.;
			played = 1.;
			return 1;
		}
		int ms = (int)(pos64 * 1000.);
		int sec = ms / 1000;
		ms %= 1000;
		if (BASS_ChannelSetPosition(chan, (QWORD)MAKELONG(sec, 0xFFFF)) == FALSE)
		{
			//return 0;
			console::warning(uStringPrintf("BASS_ChannelSetPosition() returned error: %d", BASS_ErrorGetCode()));
			no_loop = 1;
			length = 1.;
			played = 1.;
			return 1;
		}
		if (ms)
		{
			mem_block_t<float> temp;
			int count = (ms * srate / 500) & ~1;
			BASS_ChannelGetData(chan, temp.set_size(count), count * sizeof(float));
		}
		played = pos64;
		return 1;
	}

private:

	DWORD chan, no_loop;
	mem_block_aligned_t<float> buf;
	double length, played;
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
			{
				char temp[16];
				int n;
				for(n=tabsize(srate_tab);n--;)
				{
					if (srate_tab[n] != cfg_samplerate)
					{
						itoa(srate_tab[n], temp, 10);
						history_rate.add_item(temp);
					}
				}
				itoa(cfg_samplerate, temp, 10);
				history_rate.add_item(temp);
				history_rate.setup_dropdown(w = GetDlgItem(wnd,IDC_SAMPLERATE));
				uSendMessage(w, CB_SETCURSEL, 0, 0);
			}
			w = GetDlgItem(wnd,IDC_SURROUND);
			uSendMessageText(w, CB_ADDSTRING, 0, "disabled");
			uSendMessageText(w, CB_ADDSTRING, 0, "mode 1");
			uSendMessageText(w, CB_ADDSTRING, 0, "mode 2");
			uSendMessage(w, CB_SETCURSEL, cfg_surround, 0);
			w = GetDlgItem(wnd,IDC_RAMP);
			uSendMessageText(w, CB_ADDSTRING, 0, "disabled");
			uSendMessageText(w, CB_ADDSTRING, 0, "normal");
			uSendMessageText(w, CB_ADDSTRING, 0, "sensitive");
			uSendMessage(w, CB_SETCURSEL, cfg_volramp, 0);
			w = GetDlgItem(wnd,IDC_MOD);
			uSendMessageText(w, CB_ADDSTRING, 0, "normal");
			uSendMessageText(w, CB_ADDSTRING, 0, "FT2 style");
			uSendMessageText(w, CB_ADDSTRING, 0, "PT1 style");
			uSendMessage(w, CB_SETCURSEL, cfg_modstyle, 0);
			uSendDlgItemMessage(wnd, IDC_INTERPOLATION, BM_SETCHECK, cfg_interp, 0);
			uSendDlgItemMessage(wnd, IDC_LOOP, BM_SETCHECK, cfg_loop, 0);
			uSendDlgItemMessage(wnd, IDC_SOS, BM_SETCHECK, cfg_sseek, 0);
			uSendDlgItemMessage(wnd, IDC_TAG, BM_SETCHECK, cfg_tag, 0);
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
				cfg_samplerate = atoi(meh);
			}			
			break;
		case (CBN_EDITCHANGE<<16)|IDC_SAMPLERATE:
		*/
		case (CBN_KILLFOCUS<<16)|IDC_SAMPLERATE:
			{
				int t = GetDlgItemInt(wnd,IDC_SAMPLERATE,0,0);
				if (t<6000) t=6000;
				else if (t>192000) t=192000;
				cfg_samplerate = t;
			}
			break;
		case IDC_INTERPOLATION:
			cfg_interp = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		case IDC_LOOP:
			cfg_loop = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		case IDC_SOS:
			cfg_sseek = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		case IDC_TAG:
			cfg_tag = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		case (CBN_SELCHANGE<<16)|IDC_SURROUND:
			cfg_surround = uSendMessage((HWND)lp,CB_GETCURSEL,0,0);
			break;
		case (CBN_SELCHANGE<<16)|IDC_RAMP:
			cfg_volramp = uSendMessage((HWND)lp,CB_GETCURSEL,0,0);
			break;
		case (CBN_SELCHANGE<<16)|IDC_MOD:
			cfg_modstyle = uSendMessage((HWND)lp,CB_GETCURSEL,0,0);
			break;
		}
		break;
	case WM_DESTROY:
		char temp[16];
		itoa(cfg_samplerate, temp, 10);
		history_rate.add_item(temp);
		break;
	}
	return 0;
}

class config_mod : public config
{
public:
	virtual HWND create(HWND parent)
	{
		return uCreateDialog(IDD_MOD_CONFIG,parent,ConfigProc);
	}

	virtual const char * get_name() {return "Module decoder";}
	virtual const char * get_parent_name() {return "Input";}
};

class version_mod : public componentversion
{
public:
	virtual void get_file_name(string_base & out) { out.set_string(service_factory_base::get_my_file_name()); }
	virtual void get_component_name(string_base & out) { out.set_string("Module decoder"); }
	virtual void get_component_version(string_base & out) { out.set_string(MYVERSION); }
	virtual void get_about_message(string_base & out)
	{
		int bver = BASS_GetVersion();
		out.set_string("Using BASS.dll v");
		out.add_int(bver & 0xFFFF);
		out.add_byte('.');
		out.add_int(bver >> 16);
		out.add_string("\n\nhttp://www.un4seen.com/");
	}
};

class mod_file_types : public input_file_type
{
	virtual unsigned get_count()
	{
		return 1;
	}

	virtual bool get_name(unsigned idx, string_base & out)
	{
		if (idx > 0) return false;
		out = "Module files";
		return true;
	}

	virtual bool get_mask(unsigned idx, string_base & out)
	{
		if (idx > 0) return false;
		out = "*.MOD;*.MDZ;*.S3M;*.S3Z;*.IT;*.ITZ;*.XM;*.XMZ;*.MTM;*.UMX;*.MO3";
		return true;
	}
};

static service_factory_t<input,input_mod> foo;
static service_factory_single_t<config,config_mod> foo2;
static service_factory_single_t<input_file_type,mod_file_types> foo3;
static service_factory_single_t<componentversion,version_mod> foo4;
