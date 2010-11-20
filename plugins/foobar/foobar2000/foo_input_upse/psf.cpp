#define MYVERSION "1.2"

/*
	changelog

2010-11-20 22:06 UTC - kode54
- Updated fully reentrant libupse
- Version is now 1.2

2010-11-05 00:05 UTC - kode54
- Updated to latest libupse2
- Removed PSXFilter since it is now integrated into UPSE
- Version is now 1.1

2010-11-04 06:13 UTC - kode54
- Initial release
- Version is now 1.0

*/

#define _WIN32_WINNT 0x0501
#include "../SDK/foobar2000.h"
#include "../helpers/window_placement_helper.h"
#include "../ATLHelpers/ATLHelpers.h"

#include "resource.h"

#include <stdio.h>

#include "upse.h"

//#define DBG(a) OutputDebugString(a)
#define DBG(a)

// {A18393A9-A816-4565-B4E2-F407DEDD937F}
static const GUID guid_cfg_infinite = 
{ 0xa18393a9, 0xa816, 0x4565, { 0xb4, 0xe2, 0xf4, 0x7, 0xde, 0xdd, 0x93, 0x7f } };
// {8525FF23-965F-4271-B8B9-F23DC839AE3C}
static const GUID guid_cfg_deflength = 
{ 0x3a97779, 0xc731, 0x447d, { 0x94, 0xef, 0x1c, 0x80, 0xc9, 0xa0, 0x60, 0x6b } };
// {01448061-6F56-488b-BB3C-510A61FE72A8}
static const GUID guid_cfg_deffade = 
{ 0x1448061, 0x6f56, 0x488b, { 0xbb, 0x3c, 0x51, 0xa, 0x61, 0xfe, 0x72, 0xa8 } };
// {BC3A4F79-54EF-4202-9B86-D81297DAC217}
static const GUID guid_cfg_placement = 
{ 0xbc3a4f79, 0x54ef, 0x4202, { 0x9b, 0x86, 0xd8, 0x12, 0x97, 0xda, 0xc2, 0x17 } };

enum
{
	default_cfg_infinite = 0,
	default_cfg_deflength = 170000,
	default_cfg_deffade = 10000,
};

static cfg_int cfg_infinite(guid_cfg_infinite,0);
static cfg_int cfg_deflength(guid_cfg_deflength,170000);
static cfg_int cfg_deffade(guid_cfg_deffade,10000);
static cfg_window_placement cfg_placement(guid_cfg_placement);

static const char field_length[]="psf_length";
static const char field_fade[]="psf_fade";

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
	pfc::string8 foo = input;
	char *bar = (char *) foo.get_ptr();
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

static void info_meta_add(file_info & info, const char * tag, const char * value)
{
	if (info.meta_get_count_by_name(tag))
	{
		// append as another line
		pfc::string8 final = info.meta_get(tag, 0);
		final += "\r\n";
		final += value;
		info.meta_set(tag, final);
	}
	else
	{
		info.meta_add(tag, value);
	}
}

static void info_meta_ansi( file_info & info )
{
	for ( unsigned i = 0, j = info.meta_get_count(); i < j; i++ )
	{
		for ( unsigned k = 0, l = info.meta_enum_value_count( i ); k < l; k++ )
		{
			const char * value = info.meta_enum_value( i, k );
			info.meta_modify_value( i, k, pfc::stringcvt::string_utf8_from_ansi( value ) );
		}
	}
	for ( unsigned i = 0, j = info.info_get_count(); i < j; i++ )
	{
		const char * name = info.info_enum_name( i );
		if ( name[ 0 ] == '_' )
			info.info_set( pfc::string8( name ), pfc::stringcvt::string_utf8_from_ansi( info.info_enum_value( i ) ) );
	}
}

static int find_crlf(pfc::string8 & blah)
{
	int pos = blah.find_first('\r');
	if (pos >= 0 && *(blah.get_ptr()+pos+1) == '\n') return pos;
	return -1;
}

static void info_meta_write(pfc::string_base & tag, const file_info & info, const char * name, int idx, int & first)
{
	pfc::string8 v = info.meta_enum_value(idx, 0);
	int pos = find_crlf(v);

	if (pos == -1)
	{
		if (first) first = 0;
		else tag.add_byte('\n');
		tag += name;
		tag.add_byte('=');
		// r->write(v.c_str(), v.length());
		tag += v;
		return;
	}
	while (pos != -1)
	{
		pfc::string8 foo;
		foo = v;
		foo.truncate(pos);
		if (first) first = 0;
		else tag.add_byte('\n');
		tag += name;
		tag.add_byte('=');
		tag += foo;
		v = v.get_ptr() + pos + 2;
		pos = find_crlf(v);
	}
	if (v.length())
	{
		tag.add_byte('\n');
		tag += name;
		tag.add_byte('=');
		tag += v;
	}
}

#if 0
static bool info_read_line( const BYTE * ptr, int len, pfc::string_base & tag, pfc::string_base & value )
{
	int p = 0;
	for ( ;; ++p )
	{
		if ( p >= len ) break;
		unsigned u = ptr[ p ];
		if ( ! u ) return false;
		if ( u != 0x0A ) continue;
		break;
	}
	len = p;
	p = 0;
	for ( ;; ++p )
	{
		if ( p >= len ) return false;
		unsigned u = ptr[ p ];
		if ( u == '=' ) break;
		continue;
	}
	int equals_position = p;
	p = 0;
	for ( ;; ++p )
	{
		if ( p >= len ) return false;
		unsigned u = ptr[ p ];
		if ( u <= 0x20 ) continue;
		break;
	}
	if ( p == equals_position ) return false; // no name or pure whitespace in name field

	// okay, we have the tag name
	int tag_start = p;

	p = equals_position - 1;
	for ( ;; --p )
	{
		if ( p < tag_start ) return false;
		unsigned u = ptr[ p ];
		if ( u <= 0x20 ) continue;
		break;
	}
	tag.set_string( ( const char * ) ptr + tag_start, p - tag_start + 1 );

	p = equals_position + 1;

	for ( ;; ++p )
	{
		if ( p >= len ) return false;
		unsigned u = ptr[ p ];
		if ( u <= 0x20 ) continue;
		break;
	}

	tag_start = p;
	p = len;

	for ( ;; --p )
	{
		if ( p < tag_start ) return false;
		unsigned u = ptr[ p ];
		if ( u <= 0x20 ) continue;
		break;
	}
	value.set_string( ( const char * ) ptr + tag_start, p - tag_start + 1 );

	return true;
}
#endif

static void info_read(upse_psf_t * psf_info, file_info & info)
{
	int utf8 = 0;
	upse_xsf_t * the_xsf = psf_info->xsf;
	unsigned tag_song_ms = ~0, tag_fade_ms = 0;
	if ( the_xsf )
	{
		for ( unsigned i = 0; i < MAX_UNKNOWN_TAGS; i++ )
		{
			const char * key = the_xsf->tag_name[ i ];
			const char * value = the_xsf->tag_data[ i ];

			if ( !*key || !*value ) continue;

			// only parse metadata for top level executable
			if (!stricmp_utf8(key, "game"))
			{
				key = "album";
			}
			else if (!stricmp_utf8(key, "year"))
			{
				key = "date";
			}

			if (!stricmp_utf8_partial(key, "replaygain_"))
			{
				info.info_set_replaygain(key, value);
			}
			else if (!stricmp_utf8(key, "length"))
			{
				int temp = parse_time_crap(value);
				if (temp != BORK_TIME)
				{
					tag_song_ms = temp;
					info.info_set_int(field_length, temp);
				}
			}
			else if (!stricmp_utf8(key, "fade"))
			{
				int temp = parse_time_crap(value);
				if (temp != BORK_TIME)
				{
					tag_fade_ms = temp;
					info.info_set_int(field_fade, temp);
				}
			}
			else if (!stricmp_utf8(key, "utf8"))
			{
				utf8 = 1;
			}
			else if (key[0] == '_')
			{
				info.info_set(key, value);
			}
			else
			{
				info_meta_add(info, key, value);
			}
		}
	}
	if (!utf8) info_meta_ansi( info );
	unsigned tag_length = 0;
	if ( tag_song_ms == ~0 )
	{
		tag_song_ms = cfg_deflength;
		tag_fade_ms = cfg_deffade;
	}
	info.set_length( ( tag_song_ms + tag_fade_ms ) * .001 );
	info.info_set( "codec", "PSF" );
	info.info_set_int( "samplerate", 44100 );
	info.info_set_int( "channels", 2 );
}

void * foo_open( const char * path, const char * mode )
{
	bool is_read = false, is_write = false, is_binary = false;
	for ( unsigned i = 0, j = strlen( mode ); i < j; i++ )
	{
		if ( mode [ i ] == 'r' ) is_read = true;
		else if ( mode [ i ] == 'w' ) is_write = true;
		else if ( mode [ i ] == 'b' ) is_binary = true;
	}
	if ( ! is_binary ) return NULL;
	service_ptr_t< file > * the_file = NULL;
	try
	{
		abort_callback_impl m_abort;
		the_file = new service_ptr_t<file>;
		if ( is_read && ! is_write ) filesystem::g_open( *the_file, path, filesystem::open_mode_read, m_abort );
		else if ( is_write ) filesystem::g_open( *the_file, path, filesystem::open_mode_write_existing, m_abort );
		else
		{
			delete the_file;
			the_file = NULL;
		}
	}
	catch (...)
	{
		delete the_file;
		the_file = NULL;
	}
	return the_file;
}

size_t foo_read( void * ptr, size_t size, size_t nmemb, void * p_file )
{
	service_ptr_t<file> * the_file = reinterpret_cast< service_ptr_t<file> * > ( p_file );
	try
	{
		return (*the_file)->read( ptr, size * nmemb, abort_callback_impl() ) / size;
	}
	catch (...)
	{
		return 0;
	}
}

int foo_seek( void * p_file, long offset, int whence )
{
	service_ptr_t<file> * the_file = reinterpret_cast< service_ptr_t<file> * > ( p_file );
	try
	{
		(*the_file)->seek_ex( offset, ( file::t_seek_mode ) whence, abort_callback_impl() );
		return 0;
	}
	catch (...)
	{
		return 1;
	}
}

int foo_close( void * p_file )
{
	service_ptr_t<file> * the_file = reinterpret_cast< service_ptr_t<file> * > ( p_file );
	try
	{
		delete the_file;
		return 0;
	}
	catch (...)
	{
		return EOF;
	}
}

long foo_tell( void * p_file )
{
	service_ptr_t<file> * the_file = reinterpret_cast< service_ptr_t<file> * > ( p_file );
	try
	{
		return ( long ) ( ( *the_file )->get_position( abort_callback_impl() ) );
	}
	catch (...)
	{
		return -1;
	}
}

const upse_iofuncs_t foo_io = { foo_open, foo_read, foo_seek, foo_close, foo_tell };

class input_psf
{
	upse emu;

	s16 * sample_buffer;

	pfc::string8 filename;

	t_filestats m_stats;

	file_info_impl m_info;

public:
	input_psf() { }

	~input_psf() { }

	void open( service_ptr_t<file> p_file, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		input_open_file_helper( p_file, p_path, p_reason, p_abort );

		m_stats = p_file->get_stats( p_abort );

		p_file.release();

		upse_psf_t * the_psf = upse_get_psf_metadata( p_path, &foo_io );
		if ( !the_psf ) throw exception_io_data();

		filename = p_path;

		try
		{
			info_read( the_psf, m_info );
			upse_free_psf_metadata( the_psf );
		}
		catch (...)
		{
			upse_free_psf_metadata( the_psf );
			throw;
		}
	}

	void get_info( file_info & p_info, abort_callback & p_abort )
	{
		p_info.copy( m_info );
	}

	t_filestats get_file_stats( abort_callback & p_abort )
	{
		return m_stats;
	}

	void decode_initialize( unsigned p_flags, abort_callback & p_abort )
	{
		int play_forever = !( p_flags & input_flag_no_looping ) && cfg_infinite;
		if ( !emu.open( filename, &foo_io, play_forever, cfg_deflength, cfg_deffade ) )
			throw exception_io_data();
	}

	bool decode_run( audio_chunk & p_chunk, abort_callback & p_abort )
	{
		int samples_rendered = emu.render( &sample_buffer );

		if ( samples_rendered <= 0 )
			return false;

		p_chunk.set_data_fixedpoint( sample_buffer, samples_rendered * 4, 44100, 2, 16, audio_chunk::channel_config_stereo );

		return true;
	}

	void decode_seek( double p_seconds, abort_callback & p_abort )
	{
		unsigned int howmany = ( int )( audio_math::time_to_samples( p_seconds, 1000 ) );

		if ( !emu.seek( howmany ) )
			throw exception_io_data();
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
	}

	void retag( const file_info & p_info, abort_callback & p_abort )
	{
		m_info.copy( p_info );

		service_ptr_t<file> m_file;

		filesystem::g_open( m_file, filename, filesystem::open_mode_write_existing, p_abort );

		pfc::array_t<t_uint8> buffer;
		buffer.set_size( 16 );

		m_file->seek( 0, p_abort );

		BYTE *ptr = buffer.get_ptr();
		m_file->read_object( ptr, 16, p_abort );
		if (ptr[0] != 'P' || ptr[1] != 'S' || ptr[2] != 'F' ||
			(ptr[3] != 1)) throw exception_io_data();
		int reserved_size = pfc::byteswap_if_be_t( ((unsigned long*)ptr)[1] );
		int exe_size = pfc::byteswap_if_be_t( ((unsigned long*)ptr)[2] );
		m_file->seek(16 + reserved_size + exe_size, p_abort);
		m_file->set_eof(p_abort);

		pfc::string8 tag = "[TAG]utf8=1\n";

		int first = 1;
		// _lib and _refresh tags first
		int n, p = p_info.info_get_count();
		for (n = 0; n < p; n++)
		{
			const char *t = p_info.info_enum_name(n);
			if (*t == '_')
			{
				if (first) first = 0;
				else tag.add_byte('\n');
				tag += t;
				tag.add_byte('=');
				tag += p_info.info_enum_value(n);
			}
		}
		// Then info
		p = p_info.meta_get_count();
		for (n = 0; n < p; n++)
		{
			const char * t = p_info.meta_enum_name(n);
			if (*t == '_' ||
				!stricmp(t, "length") ||
				!stricmp(t, "fade")) continue; // dummy protection
			if (!stricmp(t, "album")) info_meta_write(tag, p_info, "game", n, first);
			else if (!stricmp(t, "date"))
			{
				const char * val = p_info.meta_enum_value(n, 0);
				char * end;
				strtoul(p_info.meta_enum_value(n, 0), &end, 10);
				if (size_t(end - val) < strlen(val))
					info_meta_write(tag, p_info, t, n, first);
				else
					info_meta_write(tag, p_info, "year", n, first);
			}
			else info_meta_write(tag, p_info, t, n, first);
		}
		// Then time and fade
		{
			int tag_song_ms = 0, tag_fade_ms = 0;
			const char *t = p_info.info_get(field_length);
			if (t)
			{
				char temp[16];
				tag_song_ms = atoi(t);
				if (first) first = 0;
				else tag.add_byte('\n');
				tag += "length=";
				print_time_crap(tag_song_ms, temp);
				tag += temp;
				t = p_info.info_get(field_fade);
				if (t)
				{
					tag_fade_ms = atoi(t);
					tag.add_byte('\n');
					tag += "fade=";
					print_time_crap(tag_fade_ms, (char *)&temp);
					tag += temp;
				}
			}
		}

		// Then ReplayGain
		/*
		p = p_info.info_get_count();
		for (n = 0; n < p; n++)
		{
			const char *t = p_info.info_enum_name(n);
			if (!strnicmp(t, "replaygain_",11))
			{
				if (first) first = 0;
				else tag.add_byte('\n');
				tag += t;
				else tag.add_byte('=');
				tag += p_info.info_enum_value(n);
			}
		}
		*/
		replaygain_info rg = p_info.get_replaygain();
		char rgbuf[replaygain_info::text_buffer_size];
		if (rg.is_track_gain_present())
		{
			rg.format_track_gain(rgbuf);
			if (first) first = 0;
			else tag.add_byte('\n');
			tag += "replaygain_track_gain";
			tag.add_byte('=');
			tag += rgbuf;
		}
		if (rg.is_track_peak_present())
		{
			rg.format_track_peak(rgbuf);
			if (first) first = 0;
			else tag.add_byte('\n');
			tag += "replaygain_track_peak";
			tag.add_byte('=');
			tag += rgbuf;
		}
		if (rg.is_album_gain_present())
		{
			rg.format_album_gain(rgbuf);
			if (first) first = 0;
			else tag.add_byte('\n');
			tag += "replaygain_album_gain";
			tag.add_byte('=');
			tag += rgbuf;
		}
		if (rg.is_album_peak_present())
		{
			rg.format_album_peak(rgbuf);
			if (first) first = 0;
			else tag.add_byte('\n');
			tag += "replaygain_album_peak";
			tag.add_byte('=');
			tag += rgbuf;
		}

		m_file->write_object( tag.get_ptr(), tag.length(), p_abort );
	}

	static bool g_is_our_content_type( const char * p_content_type )
	{
		return false;
	}

	static bool g_is_our_path( const char * p_full_path, const char * p_extension )
	{
		return !stricmp(p_extension,"psf") || !stricmp(p_extension,"minipsf");
	}
};

class CMyPreferences : public CDialogImpl<CMyPreferences>, public preferences_page_instance {
public:
	//Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
	CMyPreferences(preferences_page_callback::ptr callback) : m_callback(callback) {}

	//Note that we don't bother doing anything regarding destruction of our class.
	//The host ensures that our dialog is destroyed first, then the last reference to our preferences_page_instance object is released, causing our object to be deleted.


	//dialog resource ID
	enum {IDD = IDD_PSF_CONFIG};
	// preferences_page_instance methods (not all of them - get_wnd() is supplied by preferences_page_impl helpers)
	t_uint32 get_state();
	void apply();
	void reset();

	//WTL message map
	BEGIN_MSG_MAP(CMyPreferences)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_INDEFINITE, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_DLENGTH, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_DFADE, EN_CHANGE, OnEditChange)
	END_MSG_MAP()
private:
	BOOL OnInitDialog(CWindow, LPARAM);
	void OnEditChange(UINT, int, CWindow);
	void OnButtonClick(UINT, int, CWindow);
	bool HasChanged();
	void OnChanged();

	const preferences_page_callback::ptr m_callback;
};

BOOL CMyPreferences::OnInitDialog(CWindow, LPARAM) {
	SendDlgItemMessage( IDC_INDEFINITE, BM_SETCHECK, cfg_infinite );

	{
		char temp[16];
		// wsprintf((char *)&temp, "= %d Hz", 33868800 / cfg_divider);
		// SetDlgItemText(wnd, IDC_HZ, (char *)&temp);
		
		print_time_crap( cfg_deflength, (char *)&temp );
		uSetDlgItemText( m_hWnd, IDC_DLENGTH, (char *)&temp );
		
		print_time_crap( cfg_deffade, (char *)&temp );
		uSetDlgItemText( m_hWnd, IDC_DFADE, (char *)&temp );
	}
	
	return FALSE;
}

void CMyPreferences::OnEditChange(UINT, int, CWindow) {
	OnChanged();
}

void CMyPreferences::OnButtonClick(UINT, int, CWindow) {
	OnChanged();
}

t_uint32 CMyPreferences::get_state() {
	t_uint32 state = preferences_state::resettable;
	if (HasChanged()) state |= preferences_state::changed;
	return state;
}

void CMyPreferences::reset() {
	char temp[16];
	SendDlgItemMessage( IDC_INDEFINITE, BM_SETCHECK, default_cfg_infinite );
	print_time_crap( default_cfg_deflength, (char *)&temp );
	uSetDlgItemText( m_hWnd, IDC_DLENGTH, (char *)&temp );
	print_time_crap( default_cfg_deffade, (char *)&temp );
	uSetDlgItemText( m_hWnd, IDC_DFADE, (char *)&temp );
	
	OnChanged();
}

void CMyPreferences::apply() {
	int t;
	char temp[16];
	cfg_infinite = SendDlgItemMessage( IDC_INDEFINITE, BM_GETCHECK );
	t = parse_time_crap( string_utf8_from_window( GetDlgItem( IDC_DLENGTH ) ) );
	if ( t != BORK_TIME ) cfg_deflength = t;
	else
	{
		print_time_crap( cfg_deflength, (char *)&temp );
		uSetDlgItemText( m_hWnd, IDC_DLENGTH, (char *)&temp );
	}
	t = parse_time_crap( string_utf8_from_window( GetDlgItem( IDC_DFADE ) ) );
	if ( t != BORK_TIME ) cfg_deffade = t;
	else
	{
		print_time_crap( cfg_deffade, (char *)&temp );
		uSetDlgItemText( m_hWnd, IDC_DFADE, (char *)&temp );
	}
	
	OnChanged(); //our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
}

bool CMyPreferences::HasChanged() {
	//returns whether our dialog content is different from the current configuration (whether the apply button should be enabled or not)
	bool changed = false;
	if ( !changed && SendDlgItemMessage( IDC_INDEFINITE, BM_GETCHECK ) != cfg_infinite ) changed = true;
	if ( !changed )
	{
		int t = parse_time_crap( string_utf8_from_window( GetDlgItem( IDC_DLENGTH ) ) );
		if ( t != BORK_TIME && t != cfg_deflength ) changed = true;
	}
	if ( !changed )
	{
		int t = parse_time_crap( string_utf8_from_window( GetDlgItem( IDC_DFADE ) ) );
		if ( t != BORK_TIME && t != cfg_deffade ) changed = true;
	}
	return changed;
}
void CMyPreferences::OnChanged() {
	//tell the host that our state has changed to enable/disable the apply button appropriately.
	m_callback->on_state_changed();
}

class preferences_page_myimpl : public preferences_page_impl<CMyPreferences> {
	// preferences_page_impl<> helper deals with instantiation of our dialog; inherits from preferences_page_v3.
public:
	const char * get_name() {return "UPSE PSF Decoder";}
	GUID get_guid() {
		// {05C803D9-B615-48F9-9B22-73C30052C0D8}
		static const GUID guid = { 0x5c803d9, 0xb615, 0x48f9, { 0x9b, 0x22, 0x73, 0xc3, 0x0, 0x52, 0xc0, 0xd8 } };
		return guid;
	}
	GUID get_parent_guid() {return guid_input;}
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
			INFOSTRUCT * i=(INFOSTRUCT*)lp;
			char temp[16];
			if (!i->song && !i->fade) uSetWindowText(wnd, "Set length");
			else uSetWindowText(wnd, "Edit length");
			if ( i->song != ~0 )
			{
				print_time_crap(i->song, (char*)&temp);
				uSetDlgItemText(wnd, IDC_LENGTH, (char*)&temp);
			}
			if ( i->fade != ~0 )
			{
				print_time_crap(i->fade, (char*)&temp);
				uSetDlgItemText(wnd, IDC_FADE, (char*)&temp);
			}
		}
		cfg_placement.on_window_creation(wnd);
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
				else i->song = ~0;
				foo = parse_time_crap(string_utf8_from_window(wnd, IDC_FADE));
				if (foo != BORK_TIME) i->fade = foo;
				else i->fade = ~0;
			}
			EndDialog(wnd,1);
			break;
		case IDCANCEL:
			EndDialog(wnd,0);
			break;
		}
		break;
	case WM_DESTROY:
		cfg_placement.on_window_destruction(wnd);
		break;
	}
	return 0;
}

static bool context_time_dialog(unsigned * song_ms, unsigned * fade_ms)
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

class length_info_filter : public file_info_filter
{
	bool set_length, set_fade;
	unsigned m_length, m_fade;

	metadb_handle_list m_handles;

public:
	length_info_filter( const pfc::list_base_const_t<metadb_handle_ptr> & p_list )
	{
		set_length = false;
		set_fade = false;

		pfc::array_t<t_size> order;
		order.set_size(p_list.get_count());
		order_helper::g_fill(order.get_ptr(),order.get_size());
		p_list.sort_get_permutation_t(pfc::compare_t<metadb_handle_ptr,metadb_handle_ptr>,order.get_ptr());
		m_handles.set_count(order.get_size());
		for(t_size n = 0; n < order.get_size(); n++) {
			m_handles[n] = p_list[order[n]];
		}

	}

	void length( unsigned p_length )
	{
		set_length = true;
		m_length = p_length;
	}

	void fade( unsigned p_fade )
	{
		set_fade = true;
		m_fade = p_fade;
	}

	virtual bool apply_filter(metadb_handle_ptr p_location,t_filestats p_stats,file_info & p_info)
	{
		t_size index;
		if (m_handles.bsearch_t(pfc::compare_t<metadb_handle_ptr,metadb_handle_ptr>,p_location,index))
		{
			if ( set_length )
			{
				if ( m_length ) p_info.info_set_int( field_length, m_length );
				else p_info.info_remove( field_length );
			}
			if ( set_fade )
			{
				if ( m_fade ) p_info.info_set_int( field_fade, m_fade );
				else p_info.info_remove( field_fade );
			}
			return set_length | set_fade;
		}
		else
		{
			return false;
		}
	}
};

class context_psf : public contextmenu_item_simple
{
public:
	virtual unsigned get_num_items() { return 1; }

	virtual void get_item_name(unsigned n, pfc::string_base & out)
	{
		if (n) uBugCheck();
		out = "Edit length";
	}

	GUID get_parent() {return contextmenu_groups::tagging;}

	virtual bool get_item_description(unsigned n, pfc::string_base & out)
	{
		if (n) uBugCheck();
		out = "Edits the length of the selected PSF file, or sets the length of all selected PSF files.";
		return true;
	}

	virtual GUID get_item_guid(unsigned p_index)
	{
		if (p_index) uBugCheck();
		static const GUID guid = { 0x38c3f939, 0xe936, 0x49e9, { 0xb4, 0xc1, 0x3, 0x95, 0xcb, 0x19, 0xf2, 0x25 } };
		return guid;
	}

	virtual bool context_get_display(unsigned n,const pfc::list_base_const_t<metadb_handle_ptr> & data,pfc::string_base & out,unsigned & displayflags,const GUID &)
	{
		if (n) uBugCheck();
		unsigned i, j;
		i = data.get_count();
		for (j = 0; j < i; j++)
		{
			pfc::string_extension ext(data.get_item(j)->get_path());
			if ( stricmp_utf8( ext, "psf" ) && stricmp_utf8( ext, "minipsf" ) ) return false;
		}
		if (i == 1) out = "Edit length";
		else out = "Set length";
		return true;
	}

	virtual void context_command(unsigned n,const pfc::list_base_const_t<metadb_handle_ptr> & data,const GUID& caller)
	{
		if (n) uBugCheck();
		unsigned tag_song_ms = ~0, tag_fade_ms = ~0;
		unsigned i = data.get_count();
		file_info_impl info;
		abort_callback_impl m_abort;
		if (i == 1)
		{
			// fetch info from single file
			metadb_handle_ptr handle = data.get_item(0);
			handle->metadb_lock();
			const file_info * p_info;
			if (handle->get_info_locked(p_info) && p_info)
			{
				const char *t = p_info->info_get(field_length);
				if (t) tag_song_ms = atoi(t);
				t = p_info->info_get(field_fade);
				if (t) tag_fade_ms = atoi(t);
			}
			handle->metadb_unlock();
		}
		if (!context_time_dialog(&tag_song_ms, &tag_fade_ms)) return;
		static_api_ptr_t<metadb_io_v2> p_imgr;

		service_ptr_t<length_info_filter> p_filter = new service_impl_t< length_info_filter >( data );
		if ( tag_song_ms != ~0 ) p_filter->length( tag_song_ms );
		if ( tag_fade_ms != ~0 ) p_filter->fade( tag_fade_ms );

		p_imgr->update_info_async( data, p_filter, core_api::get_main_window(), 0, 0 );
	}
};

static input_singletrack_factory_t<input_psf>               g_input_psf_factory;
static preferences_page_factory_t <preferences_page_myimpl> g_config_psf_factory;
static contextmenu_item_factory_t <context_psf>             g_contextmenu_item_psf_factory;

DECLARE_FILE_TYPE( "PSF Files", "*.PSF;*.MINIPSF" );

DECLARE_COMPONENT_VERSION( "UPSE PSF Decoder", MYVERSION, "Uses the UPSE library, which is released under the\nGNU General Public License.\n\nCopyright (C) 2010 William Pitcock <nenolod@sacredspiral.co.uk>");

VALIDATE_COMPONENT_FILENAME("foo_input_upse.dll");
