#define MYVERSION "1.40"

#define DISABLE_ADL // currently broken

/*
	change log

2012-02-19 19:49 UTC - kode54
- Added abort check to decoder
- Version is now 1.40

2011-02-14 17:48 UTC - kode54
- Fixed string compare which caused "a" to be added to all reported file name
  extensions except for .LDS
- Version is now 1.39

2011-02-14 11:43 UTC - kode54
- Shunted out LDS extension to make way for the new support in foo_midi
- Version is now 1.38

2010-08-18 00:19 UTC - kode54
- Implemented support for the Adlib Surround virtual emulator core
- Version is now 1.37

2010-08-10 20:34 UTC - kode54
- Added pipes to path separator splitting in AdPlug, for archives
- Version is now 1.36

2010-04-22 16:41 UTC - kode54
- Updated to latest AdPlug CVS
- Fixed a crash bug in the HSC player
- Version is now 1.35

2010-04-13 14:53 UTC - kode54
- Amended preferences WM_INITDIALOG handler
- Version is now 1.34

2010-01-11 11:25 UTC - kode54
- Updated preferences page to 1.0 API
- Version is now 1.33

2009-11-08 23:08 UTC - kode54
- Added extra debug logging so I can attempt to track down stupid crashes
- Version is now 1.32

2009-09-03 06:20 UTC - kode54
- Added file not found exception catch around database loader in input class
- Version is now 1.31

2009-08-28 07:56 UTC - kode54
- Updated Harekiet's OPL emulator to latest from Dosbox
- Doubled the volume output from Harekiet's OPL emulator
- Version is now 1.3

2009-07-22 04:32 UTC - kode54
- Disabled the ADL reader until it can be fixed
- Version is now 1.21

2009-05-03 04:38 UTC - kode54
- Implemented Harekiet's OPL emulator core and configuration
- Version is now 1.2

2009-03-30 01:00 UTC - kode54
- Implemented Ken Silverman's Adlib emulator core and configuration
- Version is now 1.1

2008-10-16 02:48 UTC - kode54
- Implemented database checking for live updates.
- Added artist and comment meta fields.

2008-10-07 - kode54
- Initial release.

*/

#define _WIN32_WINNT 0x0501

#include "fileprovider.h"

#include "../helpers/dropdown_helper.h"
#include "../ATLHelpers/ATLHelpers.h"

#include "resource.h"

#include <adplug.h>
#include <surroundopl.h>
#include <emuopl.h>
#include <kemuopl.h>
#include "opl/dbemuopl.h"

// {0BD2647E-90FE-4d99-BE78-D3DCC5B22E87}
static const GUID guid_cfg_samplerate = 
{ 0xbd2647e, 0x90fe, 0x4d99, { 0xbe, 0x78, 0xd3, 0xdc, 0xc5, 0xb2, 0x2e, 0x87 } };
// {FDA94358-952F-4133-8542-7A75E1C5EABF}
static const GUID guid_cfg_history_rate = 
{ 0xfda94358, 0x952f, 0x4133, { 0x85, 0x42, 0x7a, 0x75, 0xe1, 0xc5, 0xea, 0xbf } };
// {58EB29CE-0FE7-4647-BF96-5C5A70EA4AD3}
static const GUID guid_cfg_play_indefinitely = 
{ 0x58eb29ce, 0xfe7, 0x4647, { 0xbf, 0x96, 0x5c, 0x5a, 0x70, 0xea, 0x4a, 0xd3 } };
// {A526C7E1-3BC6-4ddb-BD6A-C5A0F5D725FE}
static const GUID guid_cfg_adlib_core = 
{ 0xa526c7e1, 0x3bc6, 0x4ddb, { 0xbd, 0x6a, 0xc5, 0xa0, 0xf5, 0xd7, 0x25, 0xfe } };
// {60CD2195-2C70-4BFF-BEE1-C4C09226A2B2}
static const GUID guid_cfg_adlib_surround = 
{ 0x60cd2195, 0x2c70, 0x4bff, { 0xbe, 0xe1, 0xc4, 0xc0, 0x92, 0x26, 0xa2, 0xb2 } };

enum
{
	default_cfg_samplerate = 44100,
	default_cfg_play_indefinitely = 0,
	default_cfg_adlib_core = 0,
	default_cfg_adlib_surround = 0
};

static cfg_int cfg_samplerate( guid_cfg_samplerate, 44100 );
static cfg_int cfg_play_indefinitely( guid_cfg_play_indefinitely, 0 );
static cfg_int cfg_adlib_core( guid_cfg_adlib_core, 0 );
static cfg_int cfg_adlib_surround( guid_cfg_adlib_surround, 0 );

static critical_section  g_database_lock;
static t_filestats       g_database_stats = {0};
static CAdPlugDatabase * g_database = NULL;

static void refresh_database( abort_callback & p_abort )
{
	service_ptr_t<file> m_file;
	t_filestats m_stats;
	pfc::string8 path = core_api::get_my_full_path();
	path.truncate( path.scan_filename() );
	path += "adplug.db";
	filesystem::g_open( m_file, path, filesystem::open_mode_read, p_abort );
	m_stats = m_file->get_stats( p_abort );
	if ( m_stats != g_database_stats )
	{
		delete g_database;
		g_database = 0;

		CProvider_foobar2000 fp( path.get_ptr() , m_file, p_abort );
		binistream * f = fp.open( path.get_ptr() );
		if ( f )
		{
			g_database = new CAdPlugDatabase;
			g_database->load( *f );
			fp.close( f );
		}

		CAdPlug::set_database( g_database );

		g_database_stats = m_stats;
	}
}

class initquit_adplug : public initquit
{
public:
	void on_init()
	{
		try
		{
			abort_callback_impl  m_abort;
			insync( g_database_lock );
			refresh_database( m_abort );
		}
		catch (...) {}
	}
};

static Copl * create_adlib( unsigned core, unsigned srate, bool stereo = true )
{
	Copl * p_emu = NULL;

	switch ( core )
	{
	case 2:
		p_emu = new CEmuopl( srate, true, stereo );
		break;
	case 1:
		p_emu = new CKemuopl( srate, true, stereo );
		break;
	case 0:
		p_emu = new DBemuopl( srate, stereo );
	}

	return p_emu;
}

static Copl * create_adlib_surround( unsigned core, unsigned srate )
{
	Copl * a = create_adlib( core, srate, false );
	Copl * b = create_adlib( core, srate, false );

	return new CSurroundopl( a, b, true );
}

class input_adplug
{
	CPlayer * m_player;
	Copl    * m_emu;

	t_filestats m_stats;

	unsigned srate;

	bool first_block, dont_loop;//, is_adl;

	unsigned subsong, samples_todo;

	double seconds;

	pfc::array_t< t_int16 > sample_buffer;

	pfc::array_t< unsigned long > lengths;

public:
	input_adplug()
	{
		m_player = 0;
		m_emu = 0;
	}

	~input_adplug()
	{
		delete m_player;
		delete m_emu;
	}

	void open( service_ptr_t<file> m_file, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		if ( p_reason == input_open_info_write )
			throw exception_io_data();

		if ( m_file.is_empty() )
		{
			filesystem::g_open( m_file, p_path, filesystem::open_mode_read, p_abort );
		}

		m_stats = m_file->get_stats( p_abort );

		srate = cfg_samplerate;

		if ( cfg_adlib_surround ) m_emu = create_adlib_surround( cfg_adlib_core, srate );
		else m_emu = create_adlib( cfg_adlib_core, srate );

		pfc::string8 path = p_path;
		pfc::string_extension p_extension( p_path );

		if ( !pfc::stricmp_ascii( p_extension, "mida" ) || !pfc::stricmp_ascii( p_extension, "s3ma" ) || !pfc::stricmp_ascii( p_extension, "msca" ) || !pfc::stricmp_ascii( p_extension, "ldsa" ) )
		{
			path.truncate( path.length() - 1 );
		}

		{
			insync( g_database_lock );

			try
			{
				refresh_database( p_abort );
			}
			catch ( const exception_io_not_found & ) {}

			m_player = CAdPlug::factory( std::string( path ), m_emu, CAdPlug::players, CProvider_foobar2000( std::string( p_path ), m_file, p_abort ) );
			if ( ! m_player )
				throw exception_io_data();
		}

		/*if ( !strcmp( m_player->gettype().c_str(), "Westwood ADL" ) )
		{
			is_adl = true;
			lengths.set_size( 1 );
			lengths[0] = m_player->songlength( -1 );
		}
		else*/
		{
			unsigned i, j = m_player->getsubsongs();
			lengths.set_size( j );
			for ( i = 0; i < j; ++i )
			{
				lengths[ i ] = m_player->songlength( i );
			}
		}
	}

	unsigned get_subsong_count()
	{
		/*if ( is_adl ) return 1;
		else*/ return m_player->getsubsongs();
	}

	t_uint32 get_subsong(unsigned p_index)
	{
		return p_index;
	}

	void get_info( t_uint32 p_subsong, file_info & p_info, abort_callback & p_abort )
	{
		p_info.info_set( "encoding", "synthesized" );
		p_info.info_set_int( "channels", 2 );
		if ( !m_player->gettitle().empty() )
			p_info.meta_set( "title", pfc::stringcvt::string_utf8_from_ansi( m_player->gettitle().c_str() ) );
		if ( !m_player->getauthor().empty() )
			p_info.meta_set( "artist", pfc::stringcvt::string_utf8_from_ansi( m_player->getauthor().c_str() ) );
		if ( !m_player->getdesc().empty() )
			p_info.meta_set( "comment", pfc::stringcvt::string_utf8_from_ansi( m_player->getdesc().c_str() ) );
		if ( !m_player->gettype().empty() )
			p_info.info_set( "codec_profile", pfc::stringcvt::string_utf8_from_ansi( m_player->gettype().c_str() ) );

		unsigned long length;
		/*if ( is_adl ) length = lengths[ 0 ];
		else*/ length = lengths[ p_subsong ];
		p_info.set_length( length / 1000. );
	}

	t_filestats get_file_stats( abort_callback & p_abort )
	{
		return m_stats;
	}

	void decode_initialize( t_uint32 p_subsong, unsigned p_flags, abort_callback & p_abort )
	{
		first_block = true;

		samples_todo = 0;

		seconds = 0;

		subsong = p_subsong;
		/*if ( is_adl ) subsong = -1;*/

		m_emu->init();

		m_player->rewind( subsong );

		sample_buffer.set_size( 2048 * 2 );

		dont_loop = !! ( p_flags & input_flag_no_looping ) || !cfg_play_indefinitely;
	}

	bool decode_run(audio_chunk & p_chunk,abort_callback & p_abort)
	{
		p_abort.check();

		if ( !samples_todo )
		{
			bool ret = m_player->update();

			if ( dont_loop && !ret ) return false;

			samples_todo = srate / m_player->getrefresh();

			seconds += 1. / m_player->getrefresh();
		}

		unsigned sample_count = samples_todo;
		if ( sample_count > 2048 ) sample_count = 2048;

		m_emu->update( sample_buffer.get_ptr(), sample_count );

		samples_todo -= sample_count;

		p_chunk.set_data_fixedpoint( sample_buffer.get_ptr(), sample_count * 4, srate, 2, 16, audio_chunk::channel_config_stereo );
		
		return true;
	}

	void decode_seek( double p_seconds, abort_callback & p_abort )
	{
		first_block = true;

		samples_todo = 0;

		if ( p_seconds < seconds )
		{
			seconds = 0;
			m_player->rewind( subsong );
		}

		while ( seconds < p_seconds )
		{
			p_abort.check();
			m_player->update();
			seconds += 1. / m_player->getrefresh();
		}

		samples_todo = ( seconds - p_seconds ) * srate;
	}

	bool decode_can_seek()
	{
		return true;
	}

	bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta)
	{
		if ( first_block )
		{
			p_out.info_set_int( "samplerate", srate );
			first_block = false;
			return true;
		}
		return false;
	}

	bool decode_get_dynamic_info_track( file_info & p_out, double & p_timestamp_delta )
	{
		return false;
	}

	void decode_on_idle( abort_callback & p_abort )
	{
	}

	void retag_set_info( t_uint32 p_subsong, const file_info & p_info, abort_callback & p_abort )
	{
		throw pfc::exception_not_implemented();
	}

	void retag_commit( abort_callback & p_abort )
	{
		throw pfc::exception_not_implemented();
	}

	static bool g_is_our_content_type( const char * p_content_type )
	{
		return false;
	}

	static bool g_is_our_path( const char * p_path, const char * p_extension )
	{
		if ( !pfc::stricmp_ascii( p_extension, "mid" ) || !pfc::stricmp_ascii( p_extension, "s3m" ) || !pfc::stricmp_ascii( p_extension, "msc" ) || !pfc::stricmp_ascii( p_extension, "lds" ) )
			return false;

#ifdef DISABLE_ADL
		if ( !stricmp_utf8( p_extension, "adl" ) )
			return false;
#endif

		if ( !pfc::stricmp_ascii( p_extension, "mida" ) || !pfc::stricmp_ascii( p_extension, "s3ma" ) || !pfc::stricmp_ascii( p_extension, "msca" ) || !pfc::stricmp_ascii( p_extension, "ldsa" ) )
			return true;

		const CPlayers & pl = CAdPlug::players;

		CPlayers::const_iterator i;
		unsigned                 j;
		
		for ( i = pl.begin(); i != pl.end(); ++i )
		{
			for ( j = 0; ( *i )->get_extension( j ); ++j )
			{
				if ( !stricmp_utf8( p_extension, ( *i )->get_extension( j ) + 1 ) )
					return true;
			}
		}

		return false;
	}
};

class adplug_file_types : public input_file_type
{
	virtual unsigned get_count()
	{
		const CPlayers & pl = CAdPlug::players;
		return pl.size();
	}

	virtual bool get_name(unsigned idx, pfc::string_base & out)
	{
		const CPlayers & pl = CAdPlug::players;
		CPlayers::const_iterator i;
		unsigned                 j;
		for ( i = pl.begin(), j = 0; i != pl.end() && j != idx; ++i, ++j );
		out = ( *i )->filetype.c_str();
		return true;
	}

	virtual bool get_mask(unsigned idx, pfc::string_base & out)
	{
		const CPlayers & pl = CAdPlug::players;
		CPlayers::const_iterator i;
		unsigned                 j;
		for ( i = pl.begin(), j = 0; i != pl.end() && j != idx; ++i, ++j );
		const CPlayerDesc * p = *i;
		out.reset();
		for ( unsigned i = 0; p->get_extension( i ); ++i )
		{
			const char * ext = p->get_extension( i );
#ifdef DISABLE_ADL
			if ( !stricmp( ext + 1, "adl" ) ) continue;
#endif
			if ( i ) out.add_byte( ';' );
			out.add_byte( '*' );
			out += ext;
			if ( !pfc::stricmp_ascii( ext + 1, "s3m" ) || !pfc::stricmp_ascii( ext + 1, "mid" ) || !pfc::stricmp_ascii( ext + 1, "msc" ) || !pfc::stricmp_ascii( ext + 1, "lds" ) )
				out.add_byte( 'a' );
		}
		return true;
	}

	virtual bool is_associatable(unsigned idx)
	{
		return true;
	}
};

static cfg_dropdown_history cfg_history_rate(guid_cfg_history_rate,16);

static const int srate_tab[]={8000,11025,16000,22050,24000,32000,44100,48000,64000,88200,96000};

class CMyPreferences : public CDialogImpl<CMyPreferences>, public preferences_page_instance {
public:
	//Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
	CMyPreferences(preferences_page_callback::ptr callback) : m_callback(callback) {}

	//Note that we don't bother doing anything regarding destruction of our class.
	//The host ensures that our dialog is destroyed first, then the last reference to our preferences_page_instance object is released, causing our object to be deleted.


	//dialog resource ID
	enum {IDD = IDD_CONFIG};
	// preferences_page_instance methods (not all of them - get_wnd() is supplied by preferences_page_impl helpers)
	t_uint32 get_state();
	void apply();
	void reset();

	//WTL message map
	BEGIN_MSG_MAP(CMyPreferences)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_PLAY_INDEFINITELY, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_SURROUND, BN_CLICKED, OnButtonClick)
		COMMAND_HANDLER_EX(IDC_SAMPLERATE, CBN_EDITCHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_SAMPLERATE, CBN_SELCHANGE, OnSelectionChange)
		DROPDOWN_HISTORY_HANDLER(IDC_SAMPLERATE, cfg_history_rate)
		COMMAND_HANDLER_EX(IDC_ADLIBCORE, CBN_SELCHANGE, OnSelectionChange)
	END_MSG_MAP()
private:
	BOOL OnInitDialog(CWindow, LPARAM);
	void OnEditChange(UINT, int, CWindow);
	void OnSelectionChange(UINT, int, CWindow);
	void OnButtonClick(UINT, int, CWindow);
	bool HasChanged();
	void OnChanged();

	const preferences_page_callback::ptr m_callback;
};

BOOL CMyPreferences::OnInitDialog(CWindow, LPARAM) {
	CWindow w;
	char temp[16];
	int n;
	for(n=tabsize(srate_tab);n--;)
	{
		if (srate_tab[n] != cfg_samplerate)
		{
			itoa(srate_tab[n], temp, 10);
			cfg_history_rate.add_item(temp);
		}
	}
	itoa(cfg_samplerate, temp, 10);
	cfg_history_rate.add_item(temp);
	w = GetDlgItem( IDC_SAMPLERATE );
	cfg_history_rate.setup_dropdown( w );
	::SendMessage( w, CB_SETCURSEL, 0, 0 );

	w = GetDlgItem( IDC_ADLIBCORE );
	uSendMessageText( w, CB_ADDSTRING, 0, "Harekiet's" );
	uSendMessageText( w, CB_ADDSTRING, 0, "Ken Silverman's" );
	uSendMessageText( w, CB_ADDSTRING, 0, "Jarek Burczynski's" );
	::SendMessage( w, CB_SETCURSEL, cfg_adlib_core, 0 );

	SendDlgItemMessage( IDC_PLAY_INDEFINITELY, BM_SETCHECK, cfg_play_indefinitely );
	SendDlgItemMessage( IDC_SURROUND, BM_SETCHECK, cfg_adlib_surround );

	return FALSE;
}

void CMyPreferences::OnEditChange(UINT, int, CWindow) {
	OnChanged();
}

void CMyPreferences::OnSelectionChange(UINT, int, CWindow) {
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
	SetDlgItemInt( IDC_SAMPLERATE, default_cfg_samplerate, FALSE );
	SendDlgItemMessage( IDC_ADLIBCORE, CB_SETCURSEL, default_cfg_adlib_core );
	SendDlgItemMessage( IDC_PLAY_INDEFINITELY, BM_SETCHECK, default_cfg_play_indefinitely );
	SendDlgItemMessage( IDC_SURROUND, BM_SETCHECK, default_cfg_adlib_surround );
	
	OnChanged();
}

void CMyPreferences::apply() {
	char temp[16];
	int t = GetDlgItemInt( IDC_SAMPLERATE, NULL, FALSE );
	if ( t < 6000 ) t = 6000;
	else if ( t > 192000 ) t = 192000;
	SetDlgItemInt( IDC_SAMPLERATE, t, FALSE );
	itoa( t, temp, 10 );
	cfg_history_rate.add_item(temp);
	cfg_samplerate = t;
	cfg_adlib_core = SendDlgItemMessage( IDC_ADLIBCORE, CB_GETCURSEL );
	cfg_play_indefinitely = SendDlgItemMessage( IDC_PLAY_INDEFINITELY, BM_GETCHECK );
	cfg_adlib_surround = SendDlgItemMessage( IDC_SURROUND, BM_GETCHECK );
	
	OnChanged(); //our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
}

bool CMyPreferences::HasChanged() {
	//returns whether our dialog content is different from the current configuration (whether the apply button should be enabled or not)
	return GetDlgItemInt( IDC_SAMPLERATE, NULL, FALSE ) != cfg_samplerate ||
		SendDlgItemMessage( IDC_ADLIBCORE, CB_GETCURSEL ) != cfg_adlib_core ||
		SendDlgItemMessage( IDC_PLAY_INDEFINITELY, BM_GETCHECK ) != cfg_play_indefinitely ||
		SendDlgItemMessage( IDC_SURROUND, BM_GETCHECK ) != cfg_adlib_surround;
}
void CMyPreferences::OnChanged() {
	//tell the host that our state has changed to enable/disable the apply button appropriately.
	m_callback->on_state_changed();
}

class preferences_page_myimpl : public preferences_page_impl<CMyPreferences> {
	// preferences_page_impl<> helper deals with instantiation of our dialog; inherits from preferences_page_v3.
public:
	const char * get_name() {return "AdPlug";}
	GUID get_guid() {
		// {61DC98D5-A3A3-42a8-B555-97C355FE21D3}
		static const GUID guid = { 0x61dc98d5, 0xa3a3, 0x42a8, { 0xb5, 0x55, 0x97, 0xc3, 0x55, 0xfe, 0x21, 0xd3 } };
		return guid;
	}
	GUID get_parent_guid() {return guid_input;}
};

static input_factory_t            <input_adplug>            g_input_adplug_factory;
static initquit_factory_t         <initquit_adplug>         g_initquit_adplug_factory;
static service_factory_single_t   <adplug_file_types>       g_input_file_type_adplug_factory;
static preferences_page_factory_t <preferences_page_myimpl> g_config_adplug_factory;

DECLARE_COMPONENT_VERSION( "AdPlug", MYVERSION, "Input based on the AdPlug library.\n\nhttp://adplug.sourceforge.net/" );
VALIDATE_COMPONENT_FILENAME("foo_input_adplug.dll");
