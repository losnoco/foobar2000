#define MYVERSION "1.0"

#include <foobar2000.h>
#include "../helpers/dropdown_helper.h"

#include "hvl_replay.h"

#include "resource.h"

// {4BCFA055-4B91-4e3d-AD67-45A63EC15D87}
static const GUID guid_cfg_samplerate = 
{ 0x4bcfa055, 0x4b91, 0x4e3d, { 0xad, 0x67, 0x45, 0xa6, 0x3e, 0xc1, 0x5d, 0x87 } };
// {84F70B27-7FE6-4a4a-A256-8C65429D354C}
static const GUID guid_cfg_history_rate = 
{ 0x84f70b27, 0x7fe6, 0x4a4a, { 0xa2, 0x56, 0x8c, 0x65, 0x42, 0x9d, 0x35, 0x4c } };

static cfg_int cfg_samplerate(guid_cfg_samplerate,44100);

class init_stuff
{
public:
	init_stuff()
	{
		hvl_InitReplayer();
	}

	~init_stuff() {}
};

static init_stuff asdf;

class input_hvl
{
	struct hvl_tune * m_tune;

	t_filestats m_stats;

	unsigned srate;

	bool first_block, dont_loop;

	pfc::array_t< t_int16 > sample_buffer;

public:
	input_hvl()
	{
		m_tune = 0;
	}

	~input_hvl()
	{
		hvl_FreeTune( m_tune );
	}

	void open( service_ptr_t<file> m_file, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		if ( m_file.is_empty() )
		{
			filesystem::g_open( m_file, p_path, ( p_reason == input_open_info_write ) ? filesystem::open_mode_write_existing : filesystem::open_mode_read, p_abort );
		}

		m_stats = m_file->get_stats( p_abort );

		t_uint8            * ptr;
		unsigned             size;

		srate = cfg_samplerate;

		t_filesize size64 = m_file->get_size_ex( p_abort );
		if ( size64 > ( 1 << 24 ) )
			throw exception_io_data();

		size = (unsigned) size64;

		pfc::array_t< t_uint8 > buffer;
		buffer.set_size( size );
		ptr = buffer.get_ptr();

		m_file->read_object( ptr, size, p_abort );

		m_tune = hvl_LoadTune( ptr, size, srate, 0 );
		if ( ! m_tune )
			throw exception_io_data();
	}

	unsigned get_subsong_count()
	{
		return m_tune->ht_SubsongNr + 1;
	}

	t_uint32 get_subsong(unsigned p_index)
	{
		return p_index;
	}

	void get_info( t_uint32 p_subsong, file_info & p_info, abort_callback & p_abort )
	{
		p_info.info_set_int( "channels", 2 );
		p_info.meta_set( "title", pfc::stringcvt::string_utf8_from_ansi( m_tune->ht_Name, 128 ) );
		pfc::string8 temp;
		for ( unsigned i = 0, j = m_tune->ht_InstrumentNr; i < j; ++i )
		{
			temp = "inst";
			temp += pfc::format_int( i, 2 );
			p_info.meta_set( temp, pfc::stringcvt::string_utf8_from_ansi( m_tune->ht_Instruments[ i ].ins_Name, 128 ) );
		}
	}

	t_filestats get_file_stats( abort_callback & p_abort )
	{
		return m_stats;
	}

	void decode_initialize( t_uint32 p_subsong, unsigned p_flags, abort_callback & p_abort )
	{
		first_block = true;

		hvl_InitSubsong( m_tune, p_subsong );

		sample_buffer.set_size( srate / 50 * 2 );

		dont_loop = !! ( p_flags & input_flag_no_looping );
	}

	bool decode_run(audio_chunk & p_chunk,abort_callback & p_abort)
	{
		if ( dont_loop && m_tune->ht_SongEndReached ) return false;

		t_int8 * ptr = ( t_int8 * ) sample_buffer.get_ptr();

		hvl_DecodeFrame( m_tune, ptr, ptr + 2, 4 );

		p_chunk.set_data_fixedpoint( ptr, srate / 50 * 4, srate, 2, 16, audio_chunk::channel_config_stereo );
		
		return true;
	}

	void decode_seek( double p_seconds, abort_callback & p_abort )
	{
	}

	bool decode_can_seek()
	{
		return false;
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
	}

	void retag_commit( abort_callback & p_abort )
	{
	}

	static bool g_is_our_content_type( const char * p_content_type )
	{
		return false;
	}

	static bool g_is_our_path( const char * p_path, const char * p_extension )
	{
		return !stricmp(p_extension, "HVL") || !stricmp(p_extension, "AHX");
	}
};

static cfg_dropdown_history cfg_history_rate(guid_cfg_history_rate,16);

static const int srate_tab[]={8000,11025,16000,22050,24000,32000,44100,48000,64000,88200,96000};

class preferences_page_hvl : public preferences_page
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
			}
			return 1;
		case WM_COMMAND:
			switch(wp)
			{
			case (CBN_KILLFOCUS<<16)|IDC_SAMPLERATE:
				{
					int t = GetDlgItemInt(wnd,IDC_SAMPLERATE,0,0);
					if (t<6000) t=6000;
					else if (t>192000) t=192000;
					cfg_samplerate = t;
				}
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
		// {94F07062-C0FC-455e-A6BE-0F008F980231}
		static const GUID guid = 
		{ 0x94f07062, 0xc0fc, 0x455e, { 0xa6, 0xbe, 0xf, 0x0, 0x8f, 0x98, 0x2, 0x31 } };
		return guid;
	}
	virtual const char * get_name() {return "Hively Tracker decoder";}
	GUID get_parent_guid() {return guid_input;}

	bool reset_query() {return true;}
	void reset()
	{
		cfg_samplerate = 44100;
	}
};

class hvl_file_types : public input_file_type
{
	virtual unsigned get_count()
	{
		return 2;
	}

	virtual bool get_name(unsigned idx, pfc::string_base & out)
	{
		static const char * names[] = { "Abyss AHX files", "Hively Tracker files" };
		if (idx > 1) return false;
		out = names[ idx ];
		return true;
	}

	virtual bool get_mask(unsigned idx, pfc::string_base & out)
	{
		static const char * extensions[] = { "AHX", "HVL" };
		out = "*.";
		if (idx > 1) return false;
		out += extensions[ idx ];
		return true;
	}

	virtual bool is_associatable(unsigned idx)
	{
		return true;
	}
};

static input_factory_t           <input_hvl>            g_input_hvl_factory;
static preferences_page_factory_t<preferences_page_hvl> g_config_mod_factory;
static service_factory_single_t  <hvl_file_types>       g_input_file_type_hvl_factory;

DECLARE_COMPONENT_VERSION( "Hively Tracker decoder", MYVERSION, "Using Hively Tracker player v1.4.");
