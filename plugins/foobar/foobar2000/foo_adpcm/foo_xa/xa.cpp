#define MY_VERSION "1.5"

/*
	changelog

2009-10-17 13:45 UTC - kode54
- Fixed song scanner for files with RIFF headers
- Version is now 1.5

2009-06-13 04:16 UTC - kode54
- Fixed loop detection.

2009-06-13 01:41 UTC - kode54
- Implemented support for 2336 byte sector files on hard disk only.
- Version is now 1.4

2009-01-16 01:50 UTC - kode54
- Updated file add/open function.
- Version is now 1.3

2006-10-23 19:31 UTC - kode54
- Re-added main menu items
- Fixed file scanner to catch end of file bits
- Version is now 1.2

2005-08-30 07:31 UTC - kode54
- Fixed playback when caller hits get_info() after decode_initialize() on single file song
- Fixed raw CD reading ( oops )
- Version is now 1.1

2005-08-29 20:56 UTC - kode54
- Rewrote scanner and implemented caching system for new input API introduced in 0.9 beta 7
- Context menu items temporarily disabled
- Version is now 1.0

2004-10-08 14:23 UTC - kode54
- Implemented seeking
- Version is now 0.9

2004-10-08 14:09 UTC - kode54
- Removed cfg_loop from in-place checks

2004-06-21 22:14 UTC - kode54
- Changed get_file_info class to use strdup/free for elements .. eh, remainder already used strdup
- Version is now 0.85

2004-04-02 22:49 UTC - kode54
- Fixed raw to 2048 conversion for reads of more than one sector, good thing nothing needed it...
- Version is now 0.84

2004-02-02 22:38 UTC - kode54
- Fixed context_get_display function declaration in context_fest
- Version is now 0.83

2003-11-28 00:46 UTC - kode54
- Fixed stupid bug with tag remover
- No longer returns most meta info when OPEN_FLAG_GET_INFO is not set
- Version is now 0.82

2003-10-31 10:24 UTC - kode54
- Added more error checking to check_cdxa, just in case

2003-10-30 23:03 UTC - kode54
- Changed about notice to reflect new raw reading
- Added items to components menu for opening/adding XA files
- Version is now 0.81

2003-10-29 15:38 UTC - kode54
- Real raw reading based on libisofs and akrip
- I'll deal with GPL violation when I feel like it
- Fixed scanner for illegal sectors
- Version is now 0.8

2003-08-15 20:03 - kode54
- OOPS! Fixed rip function for 0.7
- Version is now 0.75

2003-06-26 06:46 - kode54
- Updated to 0.7 API
- Version is now 0.7

2003-05-19 00:49 - kode54
- Sample rate, format, and channels are detected in open()
- Changed one constant to get rid of a stupid warning

2003-05-17 20:13 - kode54
- Added tag reading, and writing for single-channel XAs
- check_cdxa no longer cares if file is larger than CDXA header specifies
- Indexer only scans multi-channel XAs
- Version is now 0.6

2003-04-28 16:19 - kode54
- Added hint() function to input
- Made bitspersample accurate to source data

2003-04-09 16:28 - kode54
- Added bitspersample info
- File name is no longer hard coded
- Version is now 0.4

*/

#include <foobar2000.h>
#include "../../helpers/window_placement_helper.h"

extern "C" {
	#include "../../foo_cdda/AKRip/akrip32.h"
	#include "../../../../libisofs/isofs.h"

	DWORD setCDSpeed( HCDROM hCD, DWORD speed );
}

#ifdef FOO_ADPCM_EXPORTS

#include "../resource.h"
extern cfg_int cfg_loop /*, cfg_scanloops*/;

#else

#include "resource.h"
static cfg_int cfg_loop("xa_loop", 1) /*, cfg_scanloops("xa_scanloops", 1)*/;

#endif

#include <commctrl.h>

/*#define DBG(a) \
	OutputDebugString( # a ); \
	a; \
	OutputDebugString("success");*/

// {60E2458C-D71B-4d8d-BE5D-CBC6A6E486D8}
static const GUID guid_cfg_placement = 
{ 0x60e2458c, 0xd71b, 0x4d8d, { 0xbe, 0x5d, 0xcb, 0xc6, 0xa6, 0xe4, 0x86, 0xd8 } };

static cfg_window_placement cfg_placement(guid_cfg_placement);

static const BYTE sync[12] = {0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0};

static CDLIST g_cdlist;
static critical_section g_access_sync;
static bool g_have_cdlist = false;

class akrip_reference
{
	static unsigned refcount;
	static bool g_succeeded;
	bool l_inited;
	static bool g_crashed;

	static bool g_akrip_init()
	{
		bool rv;
		if (g_crashed) rv = false;
		else __try {
			rv = !!AkripInit();
		} __except(1)
		{
			console::error("akrip init failure");
			g_crashed = true;
			rv = false;
		}
		return rv;
	}
	static bool g_akrip_deinit()
	{
		bool rv;
		if (g_crashed) rv = false;
		else __try
		{
			AkripDeinit();
			rv = true;
		} __except(1)
		{
			console::error("akrip deinit failure");
			g_crashed = true;
			rv = false;
		}
		return rv;
	}
public:
	akrip_reference() : l_inited(false) {}
	bool init()	
	{
		insync(g_access_sync);
		if (!l_inited)
		{
			l_inited = true;
			if (refcount++==0)
			{
				TRACK_CALL_TEXT("AkripInit");
				g_succeeded = g_akrip_init();
			}
		}
		return g_succeeded;
	}

	void deinit()
	{
		insync(g_access_sync);
		if (l_inited)
		{
			l_inited = false;
			if (--refcount == 0)
			{
				TRACK_CALL_TEXT("AkripDeinit");
				g_akrip_deinit();
			}
		}
	}

	~akrip_reference()
	{
		deinit();
	}

	inline bool succeeded() const {return l_inited && g_succeeded;}
};

unsigned akrip_reference::refcount = 0;
bool akrip_reference::g_succeeded = false;
bool akrip_reference::g_crashed = false;

static struct cdhandle
{
	HCDROM handle;
	int refcount;
	bool speed_limited;
} cdhandles[tabsize(g_cdlist.cd)];


static void init_cdlist()
{
	TRACK_CALL_TEXT("init_cdlist");
	insync(g_access_sync);
	if (!g_have_cdlist)
	{
		akrip_reference akref;
		if (akref.init())
		{
			g_have_cdlist = true;
			memset(&g_cdlist,0,sizeof(g_cdlist));
			g_cdlist.max = MAXCDLIST; 		
			GetCDList(&g_cdlist);
			memset(&cdhandles,0,sizeof(cdhandles));
		}
	}
}

static HCDROM cdrom_open( int drive_num )
{
	TRACK_CALL_TEXT("cdrom_open");
	insync(g_access_sync);


	init_cdlist();


	if (drive_num >= g_cdlist.num || drive_num<0) return 0;

	if (cdhandles[drive_num].handle && cdhandles[drive_num].refcount>0)
	{
		cdhandles[drive_num].refcount++;
		return cdhandles[drive_num].handle;
	}

	GETCDHAND gcd;
	memset(&gcd,0,sizeof(gcd));
	gcd.size = sizeof(gcd);
	gcd.ver = 1;
	gcd.readType = CDR_ANY;
	gcd.ha = g_cdlist.cd[drive_num].ha;
	gcd.tgt = g_cdlist.cd[drive_num].tgt;
	gcd.lun = g_cdlist.cd[drive_num].lun;

	HCDROM ret = GetCDHandle(&gcd);
	if (ret!=0)
	{
		cdhandles[drive_num].handle = ret;
		cdhandles[drive_num].refcount = 1;
		cdhandles[drive_num].speed_limited = false;
	}
	return ret;
}

static void cdrom_close(HCDROM handle)
{
	TRACK_CALL_TEXT("cdrom_close");
	insync(g_access_sync);
	UINT n;
	for(n=0;n<g_cdlist.num;n++)
	{
		if (cdhandles[n].handle==handle)
		{
			if (--cdhandles[n].refcount == 0)
			{
				if ( cdhandles[n].speed_limited ) setCDSpeed( handle, 0xFFFF );
				CloseCDHandle(handle);
				cdhandles[n].handle = 0;
			}
			break;
		}
	}
}

static void cdrom_limit(HCDROM handle)
{
	TRACK_CALL_TEXT("cdrom_limit");
	insync(g_access_sync);
	UINT n;
	for(n=0;n<g_cdlist.num;++n)
	{
		if (cdhandles[n].handle==handle)
		{
			if ( ! cdhandles[n].speed_limited )
			{
				cdhandles[n].speed_limited = true;
				setCDSpeed( handle, 353 );
			}
		}
	}
}

LPTRACKBUF newTrackBuf( DWORD numFrames )
{
  LPTRACKBUF t;
  int numAlloc;
  numAlloc = (((int)numFrames)*2352) + TRACKBUFEXTRA;
  t = (LPTRACKBUF)malloc( numAlloc );
  if ( !t )
    return NULL;
  t->startFrame = 0;
  t->numFrames = 0;
  t->maxLen = numFrames * 2352;
  t->len = 0;
  t->status = 0;
  t->startOffset = 0;
  return t;
}

static void get_volume_label(const char * path, pfc::string_base & out)
{
	if (IsUnicode())
	{
		WCHAR volume_name[MAX_PATH];
		GetVolumeInformationW(pfc::stringcvt::string_wide_from_utf8(path), volume_name, tabsize(volume_name), 0, 0, 0, 0, 0);
		out.set_string(pfc::stringcvt::string_utf8_from_wide(volume_name));
	}
	else
	{
		char volume_name[MAX_PATH];
		GetVolumeInformationA(pfc::stringcvt::string_ansi_from_utf8(path), volume_name, tabsize(volume_name), 0, 0, 0, 0, 0);
		out.set_string(pfc::stringcvt::string_utf8_from_ansi(volume_name));
	}
}

static int readf(char *buf, int start, int len,void *udata)
{
	int size = len;
	if (size > 8) size = 8;
	HCDROM hCD = (HCDROM)udata;
	LPTRACKBUF t = newTrackBuf(size);

	int read_total = 0;

	while (len > 0)
	{
		size = len;
		if (size > 8) size = 8;
		int retries = 3;
		int dwStatus = SS_ERR;
		while ( retries-- && (dwStatus != SS_COMP) )
		{
			t->numFrames = size;
			t->startOffset = 0;
			t->len = 0;
			t->startFrame = start;
			g_access_sync.enter();
			dwStatus = ReadCDAudioLBA(hCD, t);
			g_access_sync.leave();
		}
		if (dwStatus != SS_COMP)
		{
			free(t);
			return 0;
		}
		
		int seek_header, seek_ecc, sector_size, read = 0;
		
		BYTE * inbuf = t->buf;
		
		while (read < size)
		{
			if (memcmp(sync, inbuf, 12))
			{
				seek_header = 8;
				seek_ecc = 280;
				sector_size = 2336;
			}
			else
			{
				switch(inbuf[15])
				{
				case 2:
					seek_header = 24;
					seek_ecc = 280;
					sector_size = 2352;
					break;
				case 1:
					seek_header = 16;
					seek_ecc = 288;
					sector_size = 2352;
					break;
				default:
					free(t);
					return 0;
				}
			}
			
			inbuf += seek_header;
			memcpy(buf, inbuf, 2048);
			inbuf += 2048 + seek_ecc;
			buf += 2048;
			read++;
		}

		read_total += size;
		len -= size;
	}

	free(t);
	return read_total;
}

static int readf_gfi(char *buf, int start, int len,void *udata);
static int mycallb(struct iso_directory_record *idr,void *udata);

class get_file_info
{
private:
	HCDROM hCD;
	pfc::ptr_list_t<char> elements;
	unsigned start, len, depth, joliet;
	abort_callback & m_abort;

public:
	get_file_info(HCDROM p_hCD, const char * p_path, unsigned & p_start, unsigned & p_len, abort_callback & p_abort) : m_abort(p_abort)
	{
		pfc::string8 path(strchr(p_path + 7, '\\') + 1);

		int s = 0, e;
		while ((e = path.find_first('\\', s)) >= 0)
		{
			elements.add_item(pfc::strdup_n(path.get_ptr() + s, e - s));
			s = e + 1;
		}
		if (strlen(path.get_ptr() + s))
		{
			elements.add_item(strdup(path.get_ptr() + s));
		}

		hCD = p_hCD;

		iso_vol_desc * desc = ReadISO9660(readf_gfi, 0, this);
		iso_vol_desc * next = desc;
		struct iso_primary_descriptor * ivd;
		while (next)
		{
			p_abort.check();

			switch(isonum_711(desc->data.type))
			{
			case ISO_VD_PRIMARY:
			case ISO_VD_SUPPLEMENTARY:
				depth = 0xFFFFFFFF;
				joliet = JolietLevel(&desc->data);
				ivd = (struct iso_primary_descriptor*) &desc->data;
				if (callback((struct iso_directory_record *)&ivd->root_directory_record))
				{
					FreeISO9660(desc);
					p_start = start;
					p_len = len;
					return;
				}
				break;
			}
			next = next->next;
		}
		FreeISO9660(desc);
	}

	~get_file_info()
	{
		elements.free_all();
	}

	int callback(struct iso_directory_record * idr)
	{
		if (m_abort.is_aborting()) return 1;

		if ( !(idr->flags[0] & 2) && depth == elements.get_count() - 1)
		{
			pfc::string8 name;
			if (joliet)
			{
				name = pfc::stringcvt::string_utf8_from_wide((WCHAR*) &idr->name, isonum_711(idr->name_len));
			}
			else
			{
				name = pfc::stringcvt::string_utf8_from_ansi(idr->name, isonum_711(idr->name_len));
			}
			int e = name.find_first(';');
			if (e >= 0) name.truncate(e);
			if (!stricmp_utf8(elements[depth], name))
			{
				start = isonum_733(idr->extent);
				len = isonum_733(idr->size);
				if (len & 2047) return 0;
				len >>= 11;
				return 1;
			}
		}
		else if ( (idr->flags[0] & 2) )
		{
			if (depth == 0xFFFFFFFF)
			{
				depth++;
				if (ProcessDir(readf_gfi,isonum_733(idr->extent),isonum_733(idr->size),mycallb,this)) return 1;
				depth--;
			}
			else if (depth < elements.get_count() - 1)
			{
				pfc::string8 name;

				if (joliet)
				{
					name = pfc::stringcvt::string_utf8_from_wide((WCHAR*) &idr->name, isonum_711(idr->name_len));
				}
				else
				{
					name = pfc::stringcvt::string_utf8_from_ansi(idr->name, isonum_711(idr->name_len));
				}
				int e = name.find_first(';');
				if (e >= 0) name.truncate(e);
				if (!stricmp_utf8(elements[depth], name))
				{
					depth++;
					if (ProcessDir(readf_gfi,isonum_733(idr->extent),isonum_733(idr->size),mycallb,this)) return 1;
					depth--;
				}
			}
		}
		return 0;
	}

	int readfunc(char * buf, int start, int len)
	{
		return readf(buf, start, len, hCD);
	}
};

static int mycallb(struct iso_directory_record *idr,void *udata)
{
	get_file_info * hGFI = (get_file_info *) udata;
	return hGFI->callback(idr);
}

static int readf_gfi(char *buf, int start, int len,void *udata)
{
	get_file_info * hGFI = (get_file_info *) udata;
	return hGFI->readfunc(buf, start, len);
}

static HCDROM cdrom_from_path(const char * path)
{
	pfc::string8 root, label;

	if (pfc::strcmp_partial(path, "file://")) return 0;

	root.set_string(path + 7);
	root.truncate(root.find_first(':') + 2);
	if (root.is_empty()) return 0;

	get_volume_label(root, label);

	HCDROM cd;

	init_cdlist();

	int n, count;

	for (n = 0, count = g_cdlist.num; n < count; n++)
	{
		cd = cdrom_open( n );
		if (cd)
		{
			int joliet;
			iso_vol_desc * desc = ReadISO9660(readf, 0, cd);
			iso_vol_desc * next = desc;
			struct iso_primary_descriptor * ivd;
			pfc::string8 check_label;
			while (next)
			{
				switch(isonum_711(desc->data.type))
				{
				case ISO_VD_PRIMARY:
				case ISO_VD_SUPPLEMENTARY:
					joliet = JolietLevel(&desc->data);
					ivd = (struct iso_primary_descriptor*) &desc->data;
					if (joliet)
					{
						check_label = pfc::stringcvt::string_utf8_from_wide((WCHAR*) &ivd->volume_id, ISODCL(41, 72));
					}
					else
					{
						check_label = pfc::stringcvt::string_utf8_from_ansi(ivd->volume_id, ISODCL(41, 72));
					}
					{
						const char * ptr = check_label.get_ptr() + check_label.length() - 1;
						while (*ptr == 32) ptr--;
						check_label.truncate(ptr - check_label.get_ptr() + 1);
					}
					if (!stricmp_utf8(label, check_label))
					{
						FreeISO9660(desc);
						return cd;
					}
					break;
				}
				next = next->next;
			}
			FreeISO9660(desc);
			cdrom_close(cd);
		}
	}

	return 0;
}

class reader_raw : public file
{
	akrip_reference akref;
	t_filetimestamp m_timestamp;
	bool eof;
public:
	void limit_speed()
	{
		if ( hCD ) cdrom_limit( hCD );
	}

	bool open(const char *path, abort_callback & p_abort)
	{
		if (!filesystem::g_exists(path, p_abort)) return false;

		insync(g_access_sync);
		if (akref.init())
		{
			hCD = cdrom_from_path(path);
			if (!hCD) return false;

			t = newTrackBuf(8);
			if (!t) return false;

			get_file_info(hCD, path, start, len, p_abort);

			frame = start;
			ptr = 0;

			eof = false;

			return true;
		}

		return false;
	}

	virtual t_size read( void * p_buffer, unsigned p_bytes, abort_callback & p_abort )
	{
		unsigned dwStatus, retries;
		unsigned p_bytes_read = 0;
		if (p_bytes > t->len)
		{
			if (t->len)
			{
				memcpy(p_buffer, t->buf + ptr, t->len);
				p_bytes_read += t->len;
				p_buffer = (void*)((char*)p_buffer + t->len);
				p_bytes -= t->len;
				t->len = 0;
			}
			if (frame >= start + len)
			{
				eof = true;
				return p_bytes_read;
			}
			retries = 3;
			dwStatus = SS_ERR;
			while ( retries-- && (dwStatus != SS_COMP) )
			{
				p_abort.check();
				t->numFrames = (start + len) - frame;
				if (t->numFrames > 8) t->numFrames = 8;
				t->startOffset = 0;
				t->len = 0;
				t->startFrame = frame;
				g_access_sync.enter();
				dwStatus = ReadCDAudioLBA(hCD, t);
				g_access_sync.leave();
			}
			if (dwStatus != SS_COMP) throw exception_io();
			frame += t->numFrames;
			ptr = t->startOffset;
		}
		if (p_bytes > t->len) p_bytes = t->len;
		memcpy(p_buffer, t->buf + ptr, p_bytes);
		t->len -= p_bytes;
		ptr += p_bytes;
		p_bytes_read += p_bytes;
		return p_bytes_read;
	}

	virtual void write( const void * p_buffer, unsigned p_bytes, abort_callback & p_abort )
	{
		throw exception_io();
	}

	virtual void set_eof( abort_callback & p_abort )
	{
		throw exception_io();
	}

	virtual t_filetimestamp get_timestamp( abort_callback & p_abort )
	{
		return m_timestamp;
	}

	virtual t_filesize get_size( abort_callback & p_abort )
	{
		return 2352 * t_filesize(len);
	}

	virtual t_filesize get_position( abort_callback & p_abort )
	{
		return t_filesize(frame - start) * 2352 + t_filesize(ptr - t->startOffset);
	}

	virtual void seek( t_filesize position, abort_callback & p_abort )
	{
		eof = false;
		unsigned newframe = (unsigned)(position / 2352);
		unsigned newptr = (unsigned)(position % 2352);
		if (newframe >= len)
		{
			eof = true;
			return;
		}
		newframe += start;
		int dwStatus, retries;
		retries = 3;
		dwStatus = SS_ERR;
		while ( retries-- && (dwStatus != SS_COMP) )
		{
			p_abort.check();

			t->numFrames = (start + len) - newframe;
			if (t->numFrames > 8) t->numFrames = 8;
			t->startOffset = 0;
			t->len = 0;
			t->startFrame = newframe;
			g_access_sync.enter();
			dwStatus = ReadCDAudioLBA(hCD, t);
			g_access_sync.leave();
		}
		if (dwStatus != SS_COMP) throw exception_io();
		frame = newframe + t->numFrames;
		ptr = t->startOffset + newptr;
		t->len -= newptr;
	}

	virtual void resize(t_filesize p_size,abort_callback & p_abort)
	{
		throw exception_io();
	}

	virtual bool get_content_type(pfc::string_base & p_out)
	{
		return false;
	}

	virtual void reopen(abort_callback & p_abort)
	{
		seek( 0, p_abort );
	}

	bool can_seek()
	{
		return true;
	}

	bool is_remote()
	{
		return false;
	}

	void set_timestamp( const t_filetimestamp & p_timestamp )
	{
		m_timestamp = p_timestamp;
	}

	reader_raw()
	{
		hCD = NULL;
		t = NULL;
		m_timestamp = 0;
	}

	~reader_raw()
	{
		if (t) free(t);
		if (hCD) cdrom_close(hCD);
	}

private:
	HCDROM hCD;
	LPTRACKBUF t;
	unsigned start, frame, ptr, len;
};

static const int xa_adpcm_table[5][2] = {
	{   0,   0 },
	{  60,   0 },
	{ 115, -52 },
	{  98, -55 },
	{ 122, -60 }
};
		
#define kNumOfSGs       18

typedef char SoundGroup[128];

typedef struct SoundSector {
	char            sectorFiller[0x18];
	SoundGroup      SoundGroups[kNumOfSGs];
} SoundSector;

static t_filesize check_cdxa(service_ptr_t<file> & r, abort_callback & p_abort)
{
	pfc::array_t<t_uint8> foo;
	foo.set_size( 8 );

	t_uint8 * header = foo.get_ptr();
	r->seek( 0, p_abort );
	r->read_object( header, 4, p_abort );
	t_filesize length = r->get_size( p_abort );

	if ( pfc::byteswap_if_be_t( * ( ( t_uint32 * ) header ) ) != 'FFIR' )
	{
		if (!(length & 2047)) return ~0;
		return 0; // no header
	}
	r->read_object( header, 4, p_abort );
	if ( pfc::byteswap_if_be_t( * ( ( t_uint32 * ) header ) ) + 8 > length ) throw exception_io_data();
	r->read_object( header, 8, p_abort );
	if ( pfc::byteswap_if_be_t( * ( ( t_uint32 * ) header ) ) != 'AXDC' ||
		pfc::byteswap_if_be_t( ( ( t_uint32 * ) header )[ 1 ] ) != ' tmf' ) throw exception_io_data();
	unsigned size;
	r->read_object( header, 4, p_abort );
	size = pfc::byteswap_if_be_t( * ( ( t_uint32 * ) header ) );
	foo.grow_size( size );
	header = foo.get_ptr();
	r->read_object( header, size, p_abort );
	if ( header[5] != 'U' || header[6] != 'X' || header[7] != 'A' ) throw exception_io_data();
blah:
	r->read_object( header, 8, p_abort );
	size = pfc::byteswap_if_be_t( ( ( t_uint32 * ) header )[ 1 ] );
	if ( pfc::byteswap_if_be_t( * ( ( t_uint32 * ) header ) ) != 'atad' )
	{
		// yay! let's skip ahead
		r->seek_ex( size, file::seek_from_current, p_abort );
		goto blah;
	}

	return r->get_position( p_abort );
}

bool check_single( service_ptr_t< file > & r, abort_callback & p_abort )
{
	unsigned read, meh = 0;
	pfc::array_t< t_uint8 > xabuffer;
	xabuffer.set_size( 2352 * 4 );
	BYTE * xa = xabuffer.get_ptr();
	read = r->read( xa, 2352 * 4, p_abort ) / 2352;
	
	switch (read)
	{
	case 4:
		meh += xa[16] == xa[7056 + 16];
		meh += xa[17] == xa[7056 + 17];
	case 3:
		meh += xa[16] == xa[4704 + 16];
		meh += xa[17] == xa[4704 + 17];
	case 2:
		meh += xa[16] == xa[2352 + 16];
		meh += xa[17] == xa[2352 + 17];
	case 1:
		meh += 2;
		break;
	default:
		return false;
	}

	if (meh == (read * 2)) return true;
	else return false;
}

#define CLAMP_TO_SHORT(x) if ((x) != ((short)(x))) (x) = 0x7fff - ((x) >> ( sizeof(x) * 8 - 1 ) )

static void xa_decode_4m(t_int16 *out, const unsigned char *in, short * temp)
{
	int h, i, j;
	int shift,filter,f0,f1;
	int s_1,s_2;
	int s,t;

	s_1 = temp[0];
	s_2 = temp[1];

	in += 8;

	for (h = 0; h < 18; h++, in += 128)
	{
		for(i = 0; i < 4; i++)
		{
			shift  = 12 - (in[4 + i * 2] & 15);
			filter = in[4 + i * 2] >> 4;
			f0 = xa_adpcm_table[filter][0];
			f1 = xa_adpcm_table[filter][1];

			for(j = 0; j < 28; j++)
			{
				t = (signed char)(in[16 + i + j * 4] << 4);

				t = t >> 4;
				s = (t << shift) + ((s_1 * f0 + s_2 * f1 + 32) >> 6);
				CLAMP_TO_SHORT(s);
				*out++ = s;
				s_2 = s_1;
				s_1 = s;
			}

			shift  = 12 - (in[5 + i * 2] & 15);
			filter = in[5 + i * 2] >> 4;

			f0 = xa_adpcm_table[filter][0];
			f1 = xa_adpcm_table[filter][1];

			for(j = 0; j < 28; j++)
			{
				t = (signed char)(in[16 + i + j * 4]);

				t = t >> 4;
				s = (t << shift) + ((s_1 * f0 + s_2 * f1 + 32) >> 6);
				CLAMP_TO_SHORT(s);
				*out++ = s;
				s_2 = s_1;
				s_1 = s;
			}
		}
	}

	temp[0] = s_1;
	temp[1] = s_2;
}

static void xa_decode_4s(t_int16 *out, const unsigned char *in, short * temp)
{
	int h, i, j;
	int shift,filter,f0,f1;
	int s_1,s_2,s_3,s_4;
	int s,t;

	s_1 = temp[0];
	s_2 = temp[1];
	s_3 = temp[2];
	s_4 = temp[3];

	in += 8;

	for (h = 0; h < 18; h++, in += 128)
	{
		for(i = 0; i < 4; i++)
		{
			shift  = 12 - (in[4 + i * 2] & 15);
			filter = in[4 + i * 2] >> 4;

			f0 = xa_adpcm_table[filter][0];
			f1 = xa_adpcm_table[filter][1];

			for(j = 0; j < 28; j++)
			{
				t = (signed char)(in[16 + i + j * 4] << 4);

				t = t >> 4;
				s = (t << shift) + ((s_1 * f0 + s_2 * f1 + 32) >> 6);
				CLAMP_TO_SHORT(s);
				*out = s;
				out += 2;
				s_2 = s_1;
				s_1 = s;
			}

			out += 1 - 28 * 2;

			shift  = 12 - (in[5 + i * 2] & 15);
			filter = in[5 + i * 2] >> 4;
			f0 = xa_adpcm_table[filter][0];
			f1 = xa_adpcm_table[filter][1];

			for(j = 0; j < 28; j++)
			{
				t = (signed char)(in[16 + i + j * 4]);

				t = t >> 4;
				//if (t & 8) t |= -8;
				s = (t << shift) + ((s_3 * f0 + s_4 * f1 + 32) >> 6);
				CLAMP_TO_SHORT(s);
				*out = s;
				out += 2;
				s_4 = s_3;
				s_3 = s;
			}

			out--;
		}
	}

	temp[0] = s_1;
	temp[1] = s_2;
	temp[2] = s_3;
	temp[3] = s_4;
}

static void xa_decode_8m(t_int16 *out, const unsigned char *in, short * temp)
{
	int h, i, j;
	int shift,filter,f0,f1;
	int s_1,s_2;
	int d,s,t;

	s_1 = temp[0];
	s_2 = temp[1];

	in += 8;

	for (h = 0; h < 18; h++, in += 128)
	{
		for(i = 0; i < 4; i++)
		{
			shift  = 8 - (in[4 + i] & 15);
			filter = in[4 + i] >> 4;
			f0 = xa_adpcm_table[filter][0];
			f1 = xa_adpcm_table[filter][1];

			for(j = 0; j < 28; j++)
			{
				d = in[16 + i + j * 4];

				t = (signed char)d;
				s = (t << shift) + ((s_1 * f0 + s_2 * f1 + 32) >> 6);
				CLAMP_TO_SHORT(s);
				*out++ = s;
				s_2 = s_1;
				s_1 = s;
			}
		}
	}

	temp[0] = s_1;
	temp[1] = s_2;
}

static void xa_decode_8s(t_int16 *out, const unsigned char *in, short * temp)
{
	int h, i, j;
	int shift,filter,f0,f1;
	int s_1,s_2,s_3,s_4;
	int d,s,t;

	s_1 = temp[0];
	s_2 = temp[1];
	s_3 = temp[2];
	s_4 = temp[3];

	in += 8;

	for (h = 0; h < 18; h++, in += 128)
	{
		for(i = 0; i < 4; i += 2)
		{
			shift  = 8 - (in[4 + i] & 15);
			filter = in[4 + i] >> 4;
			f0 = xa_adpcm_table[filter][0];
			f1 = xa_adpcm_table[filter][1];

			for(j = 0; j < 28; j++)
			{
				d = in[16 + i + j * 4];

				t = (signed char)d;
				s = (t << shift) + ((s_1 * f0 + s_2 * f1 + 32) >> 6);
				CLAMP_TO_SHORT(s);
				*out = s;
				out += 2;
				s_2 = s_1;
				s_1 = s;
			}

			out += 1 - 28 * 2;

			shift  = 8 - (in[5 + i] & 15);
			filter = in[5 + i] >> 4;
			f0 = xa_adpcm_table[filter][0];
			f1 = xa_adpcm_table[filter][1];

			for(j = 0; j < 28; j++)
			{
				d = in[17 + i + j * 4];

				t = (signed char)d;
				s = (t << shift) + ((s_3 * f0 + s_4 * f1 + 32) >> 6);
				CLAMP_TO_SHORT(s);
				*out = s;
				out += 2;
				s_4 = s_3;
				s_3 = s;
			}

			out--;
		}
	}

	temp[0] = s_1;
	temp[1] = s_2;
	temp[2] = s_3;
	temp[3] = s_4;
}

struct xa_subsong_info
{
	unsigned sector_offset;
	unsigned sector_count;
	unsigned loop_start;

	inline bool operator < ( const xa_subsong_info & p_item ) { return sector_offset < p_item.sector_offset; }
	inline bool operator > ( const xa_subsong_info & p_item ) { return sector_offset > p_item.sector_offset; }
};

struct xa_subsong_scanner_info : public xa_subsong_info
{
	unsigned channel;
	unsigned file_number;
	unsigned sector_offset_end;

	unsigned srate;
	unsigned nch;
	unsigned bits_per_sample;
};

struct xa_subsong_scanner_loop_info
{
	unsigned char file_number;
	unsigned char channel;
	unsigned char scalefactor;
	unsigned char data[56];
};

class xa_subsong_scanner /*: public threaded_process_callback*/
{
	pfc::ptr_list_t< xa_subsong_scanner_info >   m_info;
	pfc::array_t< xa_subsong_scanner_loop_info > m_loop_data;

	pfc::array_t< xa_subsong_scanner_info * >    info;

	/*service_ptr_t<file> m_file;*/

public:
	~xa_subsong_scanner()
	{
		m_info.delete_all();
		for ( unsigned i = 0, j = info.get_size(); i < j; ++i ) delete info[ i ];
	}

	void run( service_ptr_t<file> & m_file, int header, int sector_size, abort_callback & p_abort )
	{
		{
			t_filesize length = m_file->get_size( p_abort ) - header;
			
			t_uint64 sector_count = length / sector_size;

			m_loop_data.set_size( unsigned( sector_count ) );

			info.set_size( 256 );
			for ( unsigned i = 0; i < 256; ++i ) info[ i ] = 0;

			pfc::array_t< t_uint8 > xa_buffer;
			xa_buffer.set_size( sector_size );
			unsigned char * ptr = xa_buffer.get_ptr();

			m_file->seek( header, p_abort );

			t_uint64 sector = 0;

			int offset = 0;

			while ( sector < sector_count )
			{
				p_abort.check();

				m_file->read_object( ptr, sector_size, p_abort );

				offset = ( sector_size == 2352 ) ? 16 : 0;

				sector++;
				/*p_status.set_progress_float( double( sector ) / double( sector_count ) );*/

				if ( ( sector_size != 2352 || ( ! memcmp( sync, ptr, 12 ) &&
					ptr[ 15 ] == 2 ) ) &&
					( ptr[ offset + 2 ] & 0x2E ) == 0x24 )
				{
					const int block_offset = 128 * 17 + 8;
					xa_subsong_scanner_loop_info & p_loop_info = m_loop_data[ sector - 1 ];
					p_loop_info.file_number = ptr[ offset ];
					p_loop_info.channel = ptr[ offset + 1 ];
					p_loop_info.scalefactor = ptr[ offset + block_offset + 11 ];
					for ( unsigned i = 0; i < 28; ++i )
					{
						p_loop_info.data[ i * 2 ]     = ptr[ offset + block_offset + 18 + i * 4 ];
						p_loop_info.data[ i * 2 + 1 ] = ptr[ offset + block_offset + 18 + i * 4 + 1 ];
					}

					xa_subsong_scanner_info * p_info = info[ ptr[ offset + 1 ] ];

					if ( p_info )
					{
						if ( p_info->file_number == ptr[ offset ] )
						{
							++ p_info->sector_count;
							p_info->sector_offset_end = unsigned( sector - 1 );
							if ( ptr[ offset + 2 ] & 0x81 )
							{
								m_info.add_item( p_info );
								info[ ptr[ offset + 1 ] ] = 0;
							}
						}
						else
						{
							m_info.add_item( p_info );
							info[ ptr[ offset + 1 ] ] = p_info = 0;
						}
					}

					if ( ! p_info )
					{
						info[ ptr[ offset + 1 ] ] = p_info = new xa_subsong_scanner_info;
						p_info->file_number = ptr[ offset ];
						p_info->sector_offset = unsigned( sector - 1 );
						p_info->sector_count = 1;
						p_info->channel = ptr[ offset + 1 ];
						p_info->loop_start = ~0;

						p_info->bits_per_sample = ( ptr[ offset + 3 ] & 48 ) >> 4;
						if (p_info->bits_per_sample > 1) throw exception_io_data();
						switch ( ptr[ offset + 3 ] & 12 )
						{
						case 0:
							p_info->srate = 37800;
							break;
						case 4:
							p_info->srate = 18900;
							break;
						default:
							throw exception_io_data();
						}
						switch ( ptr[ offset + 3 ] & 3 )
						{
						case 0:
							p_info->nch = 1;
							break;
						case 1:
							p_info->nch = 2;
							break;
						default:
							throw exception_io_data();
						}
					}
				}
				else
				{
					xa_subsong_scanner_loop_info & p_loop_info = m_loop_data[ sector - 1 ];
					p_loop_info.file_number = 0xDE;
					p_loop_info.channel     = 0xAD;
				}
			}

			for ( unsigned i = 0; i < 256; ++i )
			{
				if ( info[ i ] )
				{
					m_info.add_item( info[ i ] );
					info[ i ] = 0;
				}
			}

			// loop processing!
			/*p_status.set_title( "Scanning for loop offsets..." );
			p_status.set_progress( threaded_process_status::progress_min );*/

			for ( unsigned i = 0, j = m_info.get_count(); i < j; ++i )
			{
				p_abort.check();

				xa_subsong_scanner_info & p_info = * m_info[ i ];

				xa_subsong_scanner_loop_info & p_end = m_loop_data[ p_info.sector_offset_end ];

				for ( int k = p_info.sector_offset_end - 1, l = p_info.sector_offset; k >= l; --k )
				{
					xa_subsong_scanner_loop_info & p_loop_info = m_loop_data[ k ];
					if ( p_loop_info.file_number != 0xDE &&
					     p_loop_info.channel     != 0xAD &&
						 p_loop_info.file_number == p_end.file_number &&
						 p_loop_info.channel     == p_end.channel )
					{
						if ( p_loop_info.scalefactor == p_end.scalefactor &&
						     ! memcmp( p_loop_info.data, p_end.data, 56 ) )
						{
							int next_sector;

							for ( next_sector = k + 1; next_sector < l; ++ next_sector )
							{
								p_loop_info = m_loop_data[ next_sector ];
								if ( p_loop_info.channel     == p_end.channel &&
								     p_loop_info.file_number == p_end.file_number ) break;
							}

							p_info.loop_start = next_sector;

							break;
						}
					}
				}

				/*p_status.set_progress( i, j );*/
			}

			m_loop_data.set_size( 0 );
		}
		//catch(exception_io const & e) {return e.get_code();}

		m_info.sort();
	}

	void get_info( pfc::ptr_list_t< xa_subsong_scanner_info > & out )
	{
		for ( unsigned i = 0, j = m_info.get_count(); i < j; ++i )
		{
			xa_subsong_scanner_info * in = m_info[ i ];
			xa_subsong_scanner_info * out_item = new xa_subsong_scanner_info;
			/*out_item->sector_offset     = in.sector_offset;
			out_item->sector_count      = in.sector_count;
			out_item->loop_start        = in.loop_start;
			out_item->channel           = in.channel;
			out_item->file_number       = in.file_number;
			out_item->sector_offset_end = in.sector_offset_end;
			out_item->srate             = in.srate;
			out_item->nch               = in.nch;
			out_item->bits_per_sample   = in.bits_per_sample;*/
			memcpy( out_item, in, sizeof( *in ) );
			out.add_item( out_item );
		}
	}

	/*
	bool scan_file( service_ptr_t<file> & p_file )
	{
		return threaded_process::g_run_modal(* this,
			threaded_process::flag_show_abort |
			threaded_process::flag_show_progress,
			core_api::get_main_window(),
			"Scanning for subsongs...");
	}
	*/
};

class xa_subsong_info_cache
{
	struct t_info
	{
		pfc::string_simple                         path;
		t_filetimestamp                            timestamp;
		pfc::ptr_list_t< xa_subsong_scanner_info > info;

		~t_info()
		{
			info.delete_all();
		}
	};

	pfc::ptr_list_t< t_info > m_cache;

	critical_section sync;

public:
	~xa_subsong_info_cache()
	{
		m_cache.delete_all();
	}

	void run( service_ptr_t<file> & p_file, const char * p_path, t_filetimestamp p_timestamp, int header, int sector_size, pfc::ptr_list_t< xa_subsong_scanner_info > & p_out, abort_callback & p_abort )
	{
		insync( sync );

		for ( unsigned i = 0, j = m_cache.get_count(); i < j; ++i )
		{
			t_info * item = m_cache[ i ];
			if ( ! stricmp_utf8( p_path, item->path ) && p_timestamp == item->timestamp )
			{
				for ( unsigned k = 0, l = item->info.get_count(); k < l; ++k )
				{
					xa_subsong_scanner_info * in = item->info[ k ];
					xa_subsong_scanner_info * out_item = new xa_subsong_scanner_info;
					memcpy( out_item, in, sizeof( *in ) );
					p_out.add_item( out_item );
				}

				return;
			}
		}

		t_info * item = new t_info;

		xa_subsong_scanner scanner;

		try
		{
			scanner.run( p_file, header, sector_size, p_abort );
		}
		catch (...)
		{
			delete item;
			throw;
		}

		{
			scanner.get_info( item->info );
			for ( unsigned i = 0, j = item->info.get_count(); i < j; ++i )
			{
				xa_subsong_scanner_info * in = item->info[ i ];
				xa_subsong_scanner_info * out_item = new xa_subsong_scanner_info;
				memcpy( out_item, in, sizeof( *in ) );
				p_out.add_item( out_item );
			}

			while ( m_cache.get_count() >= 10 )
			{
				m_cache.delete_by_idx( 0 );
			}

			item->path = p_path;
			item->timestamp = p_timestamp;

			m_cache.add_item( item );
		}
	}
};

static xa_subsong_info_cache g_cache;

class input_xa
{
	pfc::array_t< t_uint8 > xabuffer;
	pfc::array_t< t_int16 > samplebuffer;

	service_ptr_t<reader_raw> m_raw;
	service_ptr_t<file>       m_file;
	// reader * wrt;

	t_filesize header;
	int file_number, channel, sector_size;
	
	bool no_loop;

	long loopstart, eof;

	int srate, nch, bits_per_sample;

	short temp[4];

	double pos, swallow;
	unsigned start;

	pfc::ptr_list_t< xa_subsong_scanner_info > m_info;

	void setSector( long sector, abort_callback & p_abort )
	{
		m_file->seek( header + t_filesize(sector) * t_filesize(sector_size), p_abort );
	}

	/*long getSector()
	{
		t_int64 p64 = m_file->get_position();
		p64 -= header;
		p64 /= 2352;
		return (long)p64;
	}*/

public:
	input_xa() : m_raw(0), srate(0), file_number( -1 )
	{
		// rdr = NULL;
		// wrt = NULL;
	}

	~input_xa()
	{
		// if (rdr) rdr->reader_release();
		// if (wrt) wrt->reader_release();
		m_info.delete_all();
	}

	void open( service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		pfc::string_simple fn( p_path );

		if ( p_filehint.is_empty() )
		{
			if ( ! strnicmp(fn, "xa://", 5) )
			{
				// lovely, let's do this
				pfc::string8 temp("file://");
				temp += fn.get_ptr() + 5;
				fn = temp;
			}

			filesystem::g_open( m_file, fn, ( p_reason == input_open_info_write ) ? filesystem::open_mode_write_existing : filesystem::open_mode_read, p_abort );
		}
		else m_file = p_filehint;

		sector_size = 2352;

		try
		{
			header = check_cdxa(m_file, p_abort);

			if ( header != ~0 )
			{
				unsigned char temp [16];
				m_file->seek( header, p_abort );
				m_file->read_object( temp, 16, p_abort );
				m_file->seek( header, p_abort );

				if ( memcmp( temp, sync, 12 ) )
					sector_size = 2336;
			}
		}
		catch(exception_io const &)
		{
			header = ~0;
		}

		if (header == ~0) // read failed, try alternate reader
		{
			if ( p_reason == input_open_info_write ) throw exception_io_unsupported_format();

			m_raw = new service_impl_t<reader_raw>;
			m_raw->set_timestamp( m_file->get_timestamp( p_abort ) );
			m_file = m_raw.get_ptr();
			if ( ! m_raw->open( fn, p_abort ) )
			{
				throw exception_io();
			}
			header = 0;
		}

		g_cache.run( m_file, p_path, m_file->get_timestamp( p_abort ), header, sector_size, m_info, p_abort );
	}

	unsigned get_subsong_count()
	{
		return m_info.get_count();
	}

	t_uint32 get_subsong( unsigned p_index )
	{
		return p_index;
	}

	void get_info( t_uint32 p_subsong, file_info & p_info, abort_callback & p_abort )
	{
		assert( p_subsong < m_info.get_count() );

		if ( m_info.get_count() == 1 )
		{
			t_filesize ptr = m_file->get_position( p_abort );
			try
			{
				tag_processor::read_trailing( m_file, p_info, p_abort);
			}
			catch ( const exception_io_data & ) {}
			m_file->seek( ptr, p_abort );
		}

		xa_subsong_scanner_info * info = m_info[ p_subsong ];
		p_info.info_set_int( "samplerate", info->srate );
		p_info.info_set_int( "channels", info->nch );
		p_info.info_set_int( "bitspersample", info->bits_per_sample ? 8 : 4 );
		p_info.info_set_int( "decoded_bitspersample", 16 );
		p_info.info_set( "codec", "XA ADPCM" );
		p_info.info_set( "encoding", "lossy" );

		p_info.info_set_int( "xa_sector_start", info->sector_offset );
		p_info.info_set_int( "xa_sector_end", info->sector_offset_end );
		if ( info->loop_start != ~0 ) p_info.info_set_int( "xa_sector_loop_start", info->loop_start );
		p_info.info_set_int( "xa_channel", info->channel );
		p_info.info_set_int( "xa_file_number", info->file_number );

		double seconds_per_sector = 4032. / double( info->srate * info->nch * ( info->bits_per_sample + 1 ) );
		p_info.set_length( seconds_per_sector * double( info->sector_count ) );

		//if ( m_info.get_count() == 1 )
		{
			p_info.info_set_bitrate( t_int64( sector_size / seconds_per_sector * 8. / 1000. + .5 ) );
		}
		//else p_info.info_set_bitrate( 2458 ); // 2457.6
	}

	t_filestats get_file_stats( abort_callback & p_abort )
	{
		return m_file->get_stats( p_abort );
	}

	void decode_initialize( t_uint32 p_subsong, unsigned p_flags, abort_callback & p_abort )
	{
		assert( p_subsong < m_info.get_count() );

		xa_subsong_scanner_info * info = m_info[ p_subsong ];

		srate = info->srate;
		nch = info->nch;

		start = info->sector_offset;
		loopstart = info->loop_start;
		if ( loopstart == ~0 ) loopstart = start;

		setSector( start, p_abort );

		file_number = info->file_number;
		channel = info->channel;

		eof = 0;

		no_loop = ( p_flags & input_flag_no_looping ) || !cfg_loop;

		if ( m_raw.is_valid() && ( p_flags & input_flag_playback ) )
		{
			m_raw->limit_speed();
		}

		temp[0] = temp[1] = temp[2] = temp[3] = 0;

		pos = 0.;
		swallow = 0.;
	}

	bool decode_run( audio_chunk & p_chunk,abort_callback & p_abort )
	{
		if (eof) return false;

		xabuffer.grow_size( sector_size );
		BYTE *xa = xabuffer.get_ptr();
		int stereo;

		UINT srate;
		int bits_per_sample;
		long samples;

		int offset = (sector_size == 2352) ? 16 : 0;

		t_int16 * out;
		
		//try
		{
retry1:
			if ( m_file->read( xa, sector_size, p_abort ) < sector_size )
			{
				if (!no_loop)
				{
					setSector(loopstart, p_abort);
					goto retry1;
				}
				else return false;
			}

			while ((offset == 16 && (memcmp(sync, xa, 12) || xa[15] != 2)) || (xa[offset + 2] & 0x2E) != 0x24 || xa[offset + 1] != channel)
			{
retry2:
				p_abort.check();

				if ( m_file->read(xa, sector_size, p_abort) < sector_size )
				{
					if (!no_loop)
					{
						setSector(loopstart, p_abort);
						goto retry2;
					}
					else return false;
				}
			}

			if (xa[offset] != file_number)
			{
				if (!no_loop)
				{
					setSector(loopstart, p_abort);
					goto retry1;
				}
				else return false;
			}

			// if (wrt) wrt->write(xa, sector_size);

			bits_per_sample = xa[offset + 3] & 48;
			if ( bits_per_sample > 16 ) throw exception_io_data(); // reserved values
			switch (xa[offset + 3] & 12)
			{
			case 0:
				srate = 37800;
				break;
			case 4:
				srate = 18900;
				break;
			default:
				throw exception_io_data(); // wtf? reserved rate
			}
			switch (xa[offset + 3] & 3)
			{
			case 0:
				stereo = 0;
				break;
			case 1:
				stereo = 1;
				break;
			default:
				throw exception_io_data(); // another reserved value
			}

			if (xa[offset + 2] & 0x81)
			{
				if (!no_loop) setSector(loopstart, p_abort);
				else
					eof = 1;
			}

			if (bits_per_sample)
			{
				samplebuffer.grow_size( 2016 );
				out = samplebuffer.get_ptr();
				if ( stereo ) { xa_decode_8s(out, xa + offset, temp); samples = 1008; }
				else { xa_decode_8m(out, xa + offset, temp); samples = 2016; }
			}
			else
			{
				samplebuffer.grow_size( 4032 );
				out = samplebuffer.get_ptr();
				if ( stereo ) { xa_decode_4s(out, xa + offset, temp); samples = 2016; }
				else { xa_decode_4m(out, xa + offset, temp); samples = 4032; }
			}

			pos += double(samples) / double(srate);

			if (swallow)
			{
				double swallowed = double(samples) / double(srate);
				if (swallowed > swallow) swallowed = swallow;
				swallow -= swallowed;
				if (swallow < 0.) swallow = 0.;
				int iswallowed = int(swallowed * double(srate) + .5);
				samples -= iswallowed;
				if (samples <= 0) goto retry1;
				//out += iswallowed << stereo;
				memmove( out, out + ( iswallowed << stereo ), sizeof(*out) * iswallowed << stereo);
			}
		}
		//catch(exception_io const & e) {return e.get_code();}

		if (samples)
		{
			p_chunk.set_data_fixedpoint( out, samples * 2 << stereo, srate, stereo + 1, 16, audio_chunk::g_guess_channel_config( stereo + 1 ) );
			return true;
		}

		return false;
	}

	void decode_seek( double p_seconds, abort_callback & p_abort )
	{
		swallow = p_seconds;
		if ( swallow > pos )
		{
			swallow -= pos;
			return;
		}
		pos = 0.;

		setSector( start, p_abort );
	}

	bool decode_can_seek()
	{
		return true;
	}

	bool decode_get_dynamic_info( file_info & p_out, double & p_timestamp_delta )
	{
		return false;
	}

	bool decode_get_dynamic_info_track( file_info & p_out, double & p_timestamp_delta )
	{
		return false;
	}

	void decode_on_idle( abort_callback & p_abort )
	{
		m_file->on_idle( p_abort );
	}

	void retag_set_info( t_uint32 p_subsong, const file_info & p_info, abort_callback & p_abort )
	{
		if ( p_subsong > 0 || m_info.get_count() > 1 )
			throw exception_io_unsupported_format();

		tag_processor::write_apev2( m_file, p_info, p_abort );
	}

	void retag_commit( abort_callback & p_abort )
	{
	}

	static bool g_is_our_content_type( const char * p_content_type )
	{
		return false;
	}

	static bool g_is_our_path(const char * p_path,const char * p_extension)
	{
		if ( strnicmp( p_path, "xa://", 5 ) &&
			( strnicmp( p_path, "file://", 7 ) || ( stricmp( p_extension, "xa") && stricmp( p_extension, "str" ) ) ) ) return false;
		return true;
	}
};

#if 0
class contextmenu_xa : public menu_item_legacy_context
{
public:
	virtual unsigned get_num_items()
	{
		return 1;
	}

	virtual void get_item_name(unsigned n, string_base & out)
	{
		out = "Rip";
	}

	virtual void get_item_default_path(unsigned n, string_base & out)
	{
		out = "XA";
	}

	virtual bool get_item_description(unsigned n, string_base & out)
	{
		out = "Rip a single track from a multi-channel XA file";
		return true;
	}

	virtual GUID get_item_guid(unsigned n)
	{
		static const GUID guid = { 0x8bd366b6, 0x4a9b, 0x48a3, { 0x9d, 0xe8, 0x64, 0xde, 0xa5, 0x63, 0x7a, 0x25 } };
		return guid;
	}

	virtual bool context_get_display(unsigned n,const list_base_const_t<metadb_handle_ptr> & data,string_base & out,unsigned & displayflags,const GUID &)
	{
		unsigned i = data.get_count();
		if (n == 1 && i != 1) return false;
		for (unsigned j = 0; j < i; j++)
		{
			const playable_location & foo = data.get_item(j)->get_location();
			const char * bar = foo.get_path();
			string_extension_8 ext(bar);
			if (strnicmp(bar, "xa://", 5) &&
				(strnicmp(bar, "file://", 7) || (stricmp(ext, "xa") && stricmp(ext, "str")))) return false;
		}
		out = "Rip";
		return true;
	}

	virtual void context_command(unsigned n,const list_base_const_t<metadb_handle_ptr> & data,const GUID& caller)
	{
		unsigned i = data.get_count();

		string8 outpath;
		service_ptr_t<file> wrt;
		abort_callback_impl m_abort;

		if (!uGetOpenFileName(core_api::get_main_window(), "XA files|*.xa", 0, "xa", 0, 0, outpath, true))
		{
			return;
		}
		string_simple meh("file://");
		meh += outpath;
		if (io_result_failed(filesystem::g_open(wrt, meh, filesystem::open_mode_write_new, m_abort))) return;

		unsigned die = i;
		HWND dialog;
		dialog = uCreateDialog(IDD_SCAN, core_api::get_main_window(), ScanProc, (LONG)&die);
		if (!dialog) return;

		modeless_dialog_manager::add(dialog);

		string8_fastalloc title;

		double length;
		int loopstart;

		for (unsigned j = 0; j < i; j++)
		{
			string8_fastalloc fn;
			metadb_handle_ptr dbh = data.get_item(j);
			const playable_location & foo = dbh->get_location();
			fn = foo.get_path();
			{
				if (n == 0) title = "Scanning ";
				else
				{
					title = "Ripping ";
					title += outpath.get_ptr() + outpath.scan_filename();
					title += " from ";
				}
				title += fn.get_ptr() + fn.scan_filename();
				if (i > 1)
				{
					title += " (";
					title.add_int(j+1);
					title.add_byte('/');
					title.add_int(i);
					title.add_byte(')');
				}
				uSetWindowText(dialog, title);
			}
			if (!strnicmp(fn, "xa://", 5))
			{
				string_simple temp("file://");
				temp += fn.get_ptr() + 5;
				fn = temp;
			}
			service_ptr_t<file> r;
			if (io_result_failed(filesystem::g_open(r, fn, filesystem::open_mode_read, m_abort))) continue;
			try
			{
				t_int64 header;

				try
				{
					header = check_cdxa(r, m_abort);
				}
				catch(exception_io const & e)
				{
					( void ) e;
					header = -1;
				}

				if (header == ~0) // read failed, try alternate reader
				{
					reader_raw * p_raw = new service_impl_t<reader_raw>;
					t_filetimestamp p_timestamp;
					if (io_result_succeeded(r->get_timestamp(p_timestamp, m_abort)))
						p_raw->set_timestamp(p_timestamp);
					r = p_raw;
					if (!p_raw->open(fn, m_abort))
					{
						return;
					}
					header = 0;
				}
				unsigned sector = foo.get_subsong(), end, eof = 0;
				r->seek_e(header + ((t_int64)sector) * 2352, m_abort);
				mem_block_t<BYTE> xabuffer, endxa;
				BYTE *xa = xabuffer.set_size(2352);
				r->read_object_e(xa, 2352, m_abort);
				uSendDlgItemMessage(dialog, IDC_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(1, 32));
				die = 1;
				unsigned file_number = xa[16];
				unsigned channel = xa[17];
				unsigned bits_per_sample;
				unsigned srate, nch;
				length = 0.;
				while (!eof)
				{
					handle_messages();
					while (memcmp(sync, xa, 12) || xa[15] != 2 || (xa[18] & 0x2E) != 0x24 || xa[17] != channel)
					{
						r->read_object_e(xa, 2352, m_abort);
						sector++;
					}
					if (!die) break;
					die++;
					if (die > 32*8) die = 1;
					if (!(die & 7)) uSendDlgItemMessage(dialog, IDC_PROGRESS, PBM_SETPOS, die/8, 0);
					if (xa[16] == file_number)
					{
						if (n == 1)
						{
							wrt->write_object_e(xa, 2352, m_abort);
						}
						else
						{
							bits_per_sample = (xa[19] & 48) >> 4;
							if (bits_per_sample > 1) throw io_result_error_data;
							bits_per_sample++;
							switch (xa[19] & 12)
							{
							case 0:
								srate = 37800;
								break;
							case 4:
								srate = 18900;
								break;
							default:
								throw io_result_error_data;
							}
							switch (xa[19] & 3)
							{
							case 0:
								nch = 1;
								break;
							case 1:
								nch = 2;
								break;
							default:
								throw io_result_error_data;
							}
							length += 4032. / (double)(srate * nch * bits_per_sample);
							end = sector;
						}
						if (xa[18] & 0x80) break;
					}
					else
						break;
					r->read_object_e(xa, 2352, m_abort);
					sector++;
				}
				if (!die) throw io_result_aborted;
				loopstart = -1;
				if (n == 0 && cfg_scanloops)
				{
					r->seek_e(header + (((t_int64)end) * 2352) + 2200, m_abort);
					BYTE *endx = endxa.set_size(128);
					r->read_object_e(endx, 128, m_abort);
					sector = foo.get_subsong();
					r->seek_e(header + ((t_int64)sector) * 2352, m_abort);
					r->read_object_e(xa, 2352, m_abort);
					uSendDlgItemMessage(dialog, IDC_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(sector, eof));
					while (sector < end && die)
					{
						handle_messages();
						while (xa[17] != channel || (xa[18] & 14) != 4)
						{
							r->read_object_e(xa, 2352, m_abort);
							sector++;
						}
						if (!die) throw io_result_aborted;
						die++;
						if (die > 8)
						{
							die = 1;
							uSendDlgItemMessage(dialog, IDC_PROGRESS, PBM_SETPOS, sector, 0);
						}
						if (xa[16] == file_number)
						{
							//if (!memcmp(xa + 2200, endx, 128))
							if (!nearmatch(xa, endx))
							{
								loopstart = -2;
							}
							else if (loopstart == -2)
							{
								loopstart = sector;
								break;
							}
						}
						r->read_object_e(xa, 2352, m_abort);
						sector++;
					}
				}
			}
			catch(exception_io const & e)
			{
				if (!n && e.get_code() != io_result_aborted) continue;
			}
		}
		if (n == 0)
		{
			metadb_handle_ptr dbh = data.get_item(0);
			dbh->metadb_lock();
			{
				file_info_i info;
				dbh->get_info(info);
				info.set_length(length);
				if (loopstart >= 0) info.info_set_int(field_loopstart, loopstart);
				static_api_ptr_t<metadb_io>()->update_info(dbh, info, core_api::get_main_window(), true);
			}
			dbh->metadb_unlock();
		}
		if (dialog)
		{
			modeless_dialog_manager::remove(dialog);
			DestroyWindow(dialog);
		}
	}
};
#endif

template <bool ADD>
class mainmenu_command_xa : public mainmenu_commands {
	virtual t_uint32 get_command_count()
	{
		return 1;
	}

	virtual GUID get_command(t_uint32 p_index)
	{
		static const GUID guids[] = {
			{ 0x365b6599, 0x23e2, 0x413f, { 0xa3, 0x6e, 0x8b, 0xf4, 0xdc, 0x80, 0x5e, 0xde } },
			{ 0x472da356, 0xc5d7, 0x4b6a, { 0x85, 0x40, 0x34, 0xab, 0xa0, 0xcf, 0x3f, 0x18 } }
		};
		return guids[ADD ? 1 : 0];
	}
	
	virtual void get_name(t_uint32 p_index,pfc::string_base & p_out)
	{
		if (!ADD) p_out = "Open";
		else p_out = "Add";
		p_out += " XA files...";
	}

	virtual bool get_description(t_uint32 p_index,pfc::string_base & p_out)
	{
		if (!ADD) p_out = "Open";
		else p_out = "Add";
		p_out += "s one or more raw files as XA ADPCM files, regardless of name or extension.";
		return true;
	}

	virtual GUID get_parent()
	{
		return ADD ? mainmenu_groups::file_add : mainmenu_groups::file_open;
	}

	virtual bool get_display(t_uint32 p_index,pfc::string_base & p_text,t_uint32 & p_flags)
	{
		p_flags = 0;
		get_name(p_index,p_text);
		return true;
	}

	virtual void execute(t_uint32 p_index,service_ptr_t<service_base> p_callback)
	{
		uGetOpenFileNameMultiResult * list;

		pfc::string8 desc;
		if ( ADD ) desc = "Add";
		else desc = "Open";
		desc += " XA files...";
		list = uGetOpenFileNameMulti(core_api::get_main_window(), "All files|*.*|XA files|*.xa;*.str", 0, 0, desc, 0);

		if (list)
		{
			pfc::array_t<char> names;
			pfc::ptr_list_t<const char> urls;
			unsigned i, count = list->GetCount();
			for (i = 0; i < count; i++)
			{
				const char * fn = list->GetFileName(i);
				pfc::string_extension ext(fn);
				if (stricmp(ext, "xa") && stricmp(ext, "str"))
				{
					names.append_fromptr("xa://", 5);
				}
				else
				{
					names.append_fromptr("file://", 7);
				}
				names.append_fromptr(fn, strlen(fn) + 1);
			}
			delete list;

			const char * ptr = names.get_ptr();
			for (i = 0;;)
			{
				urls.add_item(ptr);
				if (++i < count) ptr += strlen(ptr) + 1;
				else break;
			}

			static_api_ptr_t<playlist_manager> pm;
			t_size playlist = pm->get_active_playlist(); // Use a different playlist if desired.

			pm->playlist_undo_backup(playlist); // Highly recommended unless you modify a locked, "smart" playlist (that you own)
			if (!ADD) pm->playlist_clear(playlist); // If you want to replace the existing playlist content.
			pm->playlist_add_locations(
				playlist,
				urls,
				true,
				core_api::get_main_window());

			if (!ADD)
			{
				pm->playlist_set_focus_item(playlist, 0);
				standard_commands::main_play();
			}
		}
	}
};

#ifndef FOO_ADPCM_EXPORTS

static BOOL CALLBACK ConfigProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		{
			uSendDlgItemMessage(wnd, IDC_LOOP, BM_SETCHECK, cfg_loop, 0);
			/*uSendDlgItemMessage(wnd, IDC_SCANLOOPS, BM_SETCHECK, cfg_scanloops, 0);*/
		}
		return 1;
	case WM_COMMAND:
		switch (wp)
		{
		case IDC_LOOP:
			cfg_loop = uSendMessage((HWND)lp, BM_GETCHECK, 0, 0);
			break;
		/*case IDC_SCANLOOPS:
			cfg_scanloops = uSendMessage((HWND)lp, BM_GETCHECK, 0, 0);
			break;*/
		}
		break;
	}
	return 0;
}

class config_xa : public config
{
private:
	virtual HWND create(HWND parent)
	{
		return uCreateDialog(IDD_CONFIG, parent, ConfigProc);
	}
	virtual const char * get_name() { return "XA decoder"; }
	virtual const char * get_parent_name() { return "Input"; }
};

#endif

DECLARE_FILE_TYPE("XA files", "*.XA");

static input_factory_t< input_xa > g_input_xa_factory;
//static menu_item_factory_t<contextmenu_xa> g_menu_item_context_xa_factory;
//static menu_item_factory_t<mainmenu_xa> g_menu_item_main_xa_factory;

static mainmenu_commands_factory_t< mainmenu_command_xa<false> > g_menu_item_main_xa_open_factory;
static mainmenu_commands_factory_t< mainmenu_command_xa<true> >  g_menu_item_main_xa_add_factory;

#ifndef FOO_ADPCM_EXPORTS
static config_factory<config_xa> g_config_xa_factory;
#endif

//DECLARE_COMPONENT_VERSION("XA ADPCM decoder", MY_VERSION, "Reads XA ADPCM files from Playstation and\nSaturn CDs.");
