#define MYVERSION "1.3"

#define DISABLE_ADL // currently broken

/*
	change log

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

#include "fileprovider.h"

#include "../helpers/dropdown_helper.h"

#include "resource.h"

#include <adplug.h>
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

static cfg_int cfg_samplerate( guid_cfg_samplerate, 44100 );
static cfg_int cfg_play_indefinitely( guid_cfg_play_indefinitely, 0 );
static cfg_int cfg_adlib_core( guid_cfg_adlib_core, 0 );

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

		switch (cfg_adlib_core)
		{
		case 2:
			m_emu = new CEmuopl( srate, true, true );
			break;
		case 1:
			m_emu = new CKemuopl( srate, true, true );
			break;
		case 0:
			m_emu = new DBemuopl( srate, true );
		}

		{
			insync( g_database_lock );
			refresh_database( p_abort );
			m_player = CAdPlug::factory( std::string( p_path ), m_emu, CAdPlug::players, CProvider_foobar2000( std::string( p_path ), m_file, p_abort ) );
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
		if ( !stricmp_utf8( p_extension, "mid" ) || !stricmp_utf8( p_extension, "s3m" ) || !stricmp_utf8( p_extension, "msc" ) )
			return false;

#ifdef DISABLE_ADL
		if ( !stricmp_utf8( p_extension, "adl" ) )
			return false;
#endif

		if ( !stricmp_utf8( p_extension, "mida" ) || !stricmp_utf8( p_extension, "s3ma" ) || !stricmp_utf8( p_extension, "msca" ) )
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
			if ( !stricmp( ext + 1, "s3m" ) || !stricmp( ext + 1, "mid" ) || !stricmp( ext + 1, "msc" ) )
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

class preferences_page_adplug : public preferences_page
{
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
					if (srate_tab[n] != cfg_samplerate)
					{
						itoa(srate_tab[n], temp, 10);
						cfg_history_rate.add_item(temp);
					}
				}
				itoa(cfg_samplerate, temp, 10);
				cfg_history_rate.add_item(temp);
				cfg_history_rate.setup_dropdown(w = GetDlgItem(wnd,IDC_SAMPLERATE));
				uSendMessage(w, CB_SETCURSEL, 0, 0);

				w = GetDlgItem(wnd, IDC_ADLIBCORE);
				uSendMessageText(w, CB_ADDSTRING, 0, "Harekiet's");
				uSendMessageText(w, CB_ADDSTRING, 0, "Ken Silverman's");
				uSendMessageText(w, CB_ADDSTRING, 0, "Jarek Burczynski's");
				uSendMessage(w, CB_SETCURSEL, cfg_adlib_core, 0);

				uSendDlgItemMessage(wnd, IDC_PLAY_INDEFINITELY, BM_SETCHECK, cfg_play_indefinitely, 0);
			}
			return 1;
		case WM_COMMAND:
			switch(wp)
			{
			case IDC_PLAY_INDEFINITELY:
				cfg_play_indefinitely = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
				break;
			case (CBN_KILLFOCUS<<16)|IDC_SAMPLERATE:
				{
					int t = GetDlgItemInt(wnd,IDC_SAMPLERATE,0,0);
					if (t<6000) t=6000;
					else if (t>192000) t=192000;
					cfg_samplerate = t;
				}
				break;
			case (CBN_SELCHANGE<<16)|IDC_ADLIBCORE:
				cfg_adlib_core = uSendMessage((HWND)lp, CB_GETCURSEL, 0, 0);
				break;
			}
			break;
		case WM_DESTROY:
			char temp[16];
			itoa(cfg_samplerate, temp, 10);
			cfg_history_rate.add_item(temp);
			break;
		}
		return 0;
	}

public:
	virtual HWND create(HWND parent)
	{
		return uCreateDialog(IDD_CONFIG,parent,ConfigProc);
	}
	GUID get_guid()
	{
		// {61DC98D5-A3A3-42a8-B555-97C355FE21D3}
		static const GUID guid = 
		{ 0x61dc98d5, 0xa3a3, 0x42a8, { 0xb5, 0x55, 0x97, 0xc3, 0x55, 0xfe, 0x21, 0xd3 } };
		return guid;
	}
	virtual const char * get_name() {return "AdPlug";}
	GUID get_parent_guid() {return guid_input;}

	bool reset_query() {return true;}
	void reset()
	{
		cfg_samplerate = 44100;
		cfg_play_indefinitely = 0;
	}
};

static input_factory_t            <input_adplug>            g_input_adplug_factory;
static initquit_factory_t         <initquit_adplug>         g_initquit_adplug_factory;
static service_factory_single_t   <adplug_file_types>       g_input_file_type_adplug_factory;
static preferences_page_factory_t <preferences_page_adplug> g_config_adplug_factory;

DECLARE_COMPONENT_VERSION( "AdPlug", MYVERSION, "Input based on the AdPlug library.\n\nhttp://adplug.sourceforge.net/" );