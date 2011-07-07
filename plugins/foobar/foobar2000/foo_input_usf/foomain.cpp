#define _WIN32_WINNT 0x0501
#include <foobar2000.h>
#include <helpers.h>
#include <shared.h>
#include "usf.h"
#include "audio.h"
#include "psftag.h"


static const GUID guid_cfg_CpuMode = { 0x9d04d35c, 0xd2f, 0x42ea, { 0xbf, 0xe9, 0x99, 0x18, 0x9f, 0xbf, 0xee, 0xfa } };
static const GUID guid_cfg_RspMode = { 0xb583ee83, 0xf4e, 0x4c99, { 0x9d, 0x69, 0xc4, 0xb5, 0x6b, 0xb4, 0x14, 0xff } };
static const GUID guid_cfg_PlayTime = { 0xf57d734b, 0x36ab, 0x40ef, { 0x98, 0x4b, 0x9b, 0x56, 0x67, 0x90, 0xdb, 0x6c } };
static const GUID guid_cfg_FastSeek = { 0x4df8e490, 0x44e, 0x4502, { 0x99, 0xfd, 0x1d, 0x27, 0x14, 0xa5, 0xbf, 0xdb } };
static const GUID guid_cfg_DetectSilence = { 0xab46c592, 0xbd85, 0x4a87, { 0x97, 0xf5, 0x58, 0xb9, 0xf7, 0x95, 0xe8, 0x85 } };
static const GUID guid_cfg_DefaultLength = { 0x62a2cfd5, 0x3b54, 0x47eb, { 0x8b, 0xa1, 0x0, 0x93, 0xd4, 0xe9, 0x89, 0x23 } };
static const GUID guid_cfg_DefaultFade = { 0x519a7716, 0xe680, 0x468d, { 0x90, 0x81, 0x88, 0x4, 0xe1, 0x44, 0xc, 0x67 } };
static const GUID guid_cfg_DetectSilenceTime = { 0x5f54744f, 0x1a46, 0x419d, { 0xb3, 0x77, 0x98, 0xd, 0xaf, 0xe2, 0x6e, 0x94 } };
//static const GUID guid_cfg_Priority = { 0x154496c7, 0xdc36, 0x4f7a, { 0x91, 0x35, 0xae, 0x30, 0x98, 0x25, 0xfb, 0x6d } };
static const GUID guid_cfg_FadeType = { 0xbc8d52c2, 0x1dab, 0x47d5, { 0x8b, 0xeb, 0xdb, 0xfb, 0x11, 0x2, 0x21, 0x6d } };
static const GUID guid_cfg_placement = { 0x409da9cc, 0xc674, 0x4721, { 0xa1, 0xc, 0x5f, 0x9, 0x76, 0xd0, 0x86, 0x97 } };


static cfg_uint cfg_CpuMode(guid_cfg_CpuMode, CPU_RECOMPILER);
static cfg_uint cfg_RspMode(guid_cfg_RspMode, RSP_RECOMPILER);
static cfg_uint cfg_PlayTime(guid_cfg_PlayTime, PLAYTIME_NORMAL);
static cfg_uint cfg_FadeType(guid_cfg_FadeType, 1);
static cfg_bool cfg_FastSeek(guid_cfg_FastSeek, true);
static cfg_bool cfg_DetectSilence(guid_cfg_DetectSilence, true);
static cfg_string cfg_DefaultLength(guid_cfg_DefaultLength, "180");
static cfg_string cfg_DefaultFade(guid_cfg_DefaultFade, "5");
static cfg_string cfg_DetectSilenceTime(guid_cfg_DetectSilenceTime, "5");
//static cfg_uint cfg_Priority(guid_cfg_Priority, 3);
static cfg_window_placement cfg_placement(guid_cfg_placement);

#include "foomain.h"

void input_usf::open(service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort)
{
	if(mUsf == NULL)
		mUsf = new Usf(p_path, p_abort);

	input_open_file_helper( p_filehint, p_path, p_reason, p_abort );
	m_stats = p_filehint->get_stats( p_abort );
	p_filehint.release();

	switch(p_reason)
	{
		case input_open_info_read:
		case input_open_info_write:
			break;

		case input_open_decode:
			try
			{
				mUsf->LoadUSF(NULL);
				if(mUsf->savestate == NULL) _asm int 3
			}
			catch (exception_aborted)
			{
//				if(mUsf->savestate == NULL) _asm int 3
				throw exception_aborted();
				return;
			}
			break;

		default:
			break;
	}

}



void input_usf::get_info(file_info & p_info, abort_callback & p_abort)
{
	mUsf->ReadFileInfo(p_info);
}



void input_usf::decode_initialize(unsigned p_flags, abort_callback & p_abort)
{
	no_looping = !!(p_flags & input_flag_no_looping);

	mUsf->PlayUSF();
}



bool input_usf::decode_run(audio_chunk & p_chunk, abort_callback & p_abort)
{
	uint32_t BufferSize = 5000;


	if(((cfg_PlayTime != PLAYTIME_FOREVER) || no_looping) && (mUsf->cpu->audio->GetTime() > (mUsf->TrackLength + mUsf->FadeLength)))
	{
		return false;
	}

	if(cfg_DetectSilence && (mUsf->SilenceTime <= mUsf->cpu->audio->SilenceCount) && (!mUsf->PlayForever || no_looping))
		return false;

	if(mUsf->PlayDefault && (mUsf->cpu->audio->GetTime() > mUsf->DefLength) && (!mUsf->PlayForever || no_looping))
	{
		return false;
	}

	/*int32_t CurPriority = GetThreadPriority(GetCurrentThread());
	SetThreadPriority(GetCurrentThread(), PriArray[6 - cfg_Priority]);*/

	void * Buffer = mUsf->cpu->RunCpu(&BufferSize);

	p_chunk.set_data_floatingpoint_ex(Buffer, BufferSize, mUsf->cpu->audio->SampleRate, 2, 32, audio_chunk::FLAG_LITTLE_ENDIAN, audio_chunk::g_guess_channel_config(2));

	//SetThreadPriority(GetCurrentThread(), CurPriority);

	return BufferSize?true:false;
}



void input_usf::decode_seek(double p_seconds, abort_callback & p_abort)
{
	uint32_t BufferSize = 2000;

	mUsf->Seek(p_seconds);

	while(mUsf->Seeking)
	{
		p_abort.check();
		mUsf->cpu->RunCpu(&BufferSize);
	}
}



t_filestats input_usf::get_file_stats(abort_callback & p_abort)
{
	return m_stats;
}



bool input_usf::decode_get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta)
{
	return false;
}



bool input_usf::decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta)
{
	if(mUsf)
	{
		if(mUsf->cpu)
		{
			if(mUsf->cpu->audio)
			{
				p_out.info_set_int("samplerate", mUsf->cpu->audio->SampleRate);
				return true;
			}
		}
	}
	return false;
}



static void GetTagz(const file_info & p_info, char * TagBuf, const char * TagzName, const char * PsfTagName)
{
	const char * Tag;
	Tag = p_info.meta_get(TagzName, 0);
	if(Tag)
		psftag_raw_setvar(TagBuf, 49999, PsfTagName, Tag);
}



int find_crlf(pfc::string8 & blah)
{
	int pos = blah.find_first('\r');
	if (pos >= 0 && *(blah.get_ptr()+pos+1) == '\n') return pos;
	return -1;
}

void info_meta_write(pfc::string_base & tag, const file_info & info, const char * name, int idx)
{
	pfc::string8 v = info.meta_enum_value(idx, 0);
	int pos = find_crlf(v);

	if (pos == -1)
	{
		tag.add_byte( '\n' );
		tag += name;
		tag.add_byte( '=' );
		tag += v;
		return;
	}
	while (pos != -1)
	{
		pfc::string8 foo;
		foo = v;
		foo.truncate( pos );
		tag.add_byte( '\n' );
		tag += name;
		tag.add_byte( '=' );
		tag += foo;
		v = v.get_ptr() + pos + 2;
		pos = find_crlf( v );
	}
	if ( v.length() )
	{
		tag.add_byte( '\n' );
		tag += name;
		tag.add_byte( '=' );
		tag += v;
	}
}

void input_usf::retag(const file_info & p_info, abort_callback & p_abort)
{
	pfc::string8 output_tag = "utf8=1";

	for ( unsigned i = 0, j = p_info.info_get_count(); i < j; i++ )
	{
		const char * name = p_info.info_enum_name( i );
		if ( name[0] == '_' )
		{
			output_tag.add_byte( '\n' );
			output_tag += name;
			output_tag.add_byte( '=' );
			output_tag += p_info.info_enum_value( i );
		}
	}

	for ( unsigned i = 0, j = p_info.meta_get_count(); i < j; i++ )
	{
		const char * name = p_info.meta_enum_name( i );
		if ( name[0] == '_' || !pfc::stricmp_ascii( name, "length" ) || !pfc::stricmp_ascii( name, "fade" ) || !pfc::stricmp_ascii( name, "game" ) ) continue;
		if ( !pfc::stricmp_ascii( name, "album" ) ) name = "game";
		else if ( !pfc::stricmp_ascii( name, "USF By" ) ) name = "usfby";
		else if ( !pfc::stricmp_ascii( name, "Tagged By" ) ) name = "tagger";
		if ( !pfc::stricmp_ascii( name, "date" ) )
		{
			const char * val = p_info.meta_enum_value( i, 0 );
			char * end;
			strtoul( p_info.meta_enum_value( i, 0 ), &end, 10 );
			if ( size_t( end - val ) < strlen( val ) )
				info_meta_write( output_tag, p_info, name, i );
			else
				info_meta_write( output_tag, p_info, "year", i );
		}
		else info_meta_write( output_tag, p_info, name, i );
	}

	const char * value = p_info.info_get( "usf_length" );
	if ( value )
	{
		output_tag.add_byte( '\n' );
		output_tag += "length";
		output_tag.add_byte( '=' );
		output_tag += value;
		value = p_info.info_get( "usf_fade" );
		if ( value )
		{
			output_tag.add_byte( '\n' );
			output_tag += "fade";
			output_tag.add_byte( '=' );
			output_tag += value;
		}
	}

	replaygain_info rg = p_info.get_replaygain();
	char rgbuf[replaygain_info::text_buffer_size];
	if ( rg.is_track_gain_present() )
	{
		rg.format_track_gain( rgbuf );
		output_tag.add_byte( '\n' );
		output_tag += "replaygain_track_gain";
		output_tag.add_byte( '=' );
		output_tag += rgbuf;
	}
	if ( rg.is_track_peak_present() )
	{
		rg.format_track_peak( rgbuf );
		output_tag.add_byte( '\n' );
		output_tag += "replaygain_track_peak";
		output_tag.add_byte( '=' );
		output_tag += rgbuf;
	}
	if ( rg.is_album_gain_present() )
	{
		rg.format_album_gain( rgbuf );
		output_tag.add_byte( '\n' );
		output_tag += "replaygain_album_gain";
		output_tag.add_byte( '=' );
		output_tag += rgbuf;
	}
	if ( rg.is_album_peak_present() )
	{
		rg.format_album_peak( rgbuf );
		output_tag.add_byte( '\n' );
		output_tag += "replaygain_album_peak";
		output_tag.add_byte( '=' );
		output_tag += rgbuf;
	}

	mUsf->SetTags( output_tag, &m_stats );
}



input_usf::input_usf()
{
	mUsf = NULL;
}



input_usf::~input_usf()
{
	if(mUsf->cpu)
		if(mUsf->cpu->audio)
			delete mUsf->cpu->audio;
	delete mUsf;
}


/******************************************************************************/
/****************************** Prefrences ************************************/

void Usf::LoadSettings(void)
{
	Fading = true;
	PlayForever = Forever = (cfg_PlayTime == PLAYTIME_FOREVER);
	PlayDefault = (cfg_PlayTime == PLAYTIME_USEDEFAULTLENGTH);

	CpuMode = cfg_CpuMode;
	RspMode = cfg_RspMode;

	AudioHle = (cfg_RspMode == RSP_AUDIOHLE);

	FadeMode = cfg_FadeType;

	DefLength = TrackLength = GetLengthFromString((char *)cfg_DefaultLength.get_ptr());
	FadeLength = GetLengthFromString((char *)cfg_DefaultFade.get_ptr());
	SilenceTime = GetLengthFromString((char *)cfg_DetectSilenceTime.get_ptr());

	DetectSilence = cfg_DetectSilence;
}


const char * input_usf_pref::get_name()
{
	return "LazyUSF";
}

GUID input_usf_pref::get_guid()
{
	static const GUID guid = { 0x6827d66e, 0xe0bd, 0x4694, { 0x92, 0x49, 0x5c, 0x8e, 0xdf, 0x53, 0xe2, 0xf2 } };
	return guid;
}

GUID input_usf_pref::get_parent_guid()
{
	return guid_input;
}



BOOL UsfPreferences::OnInitDialog(CWindow, LPARAM)
{

	SendDlgItemMessage(IDC_RECOMPILERCPU, BM_SETCHECK, cfg_CpuMode==CPU_RECOMPILER);
	SendDlgItemMessage(IDC_INTERPRETERCPU, BM_SETCHECK, cfg_CpuMode==CPU_INTERPRETER);

	CheckDlgButton(IDC_RSPRECOMPILER, cfg_RspMode==RSP_RECOMPILER?BST_CHECKED:BST_UNCHECKED);
	CheckDlgButton(IDC_RSPINTERPRETER, cfg_RspMode==RSP_INTERPRETER?BST_CHECKED:BST_UNCHECKED);
	CheckDlgButton(IDC_AUDIOHLE, cfg_RspMode==RSP_AUDIOHLE?BST_CHECKED:BST_UNCHECKED);

	CheckDlgButton(IDC_FASTSEEK, cfg_FastSeek?BST_CHECKED:BST_UNCHECKED);

	switch(cfg_PlayTime)
	{
		case PLAYTIME_FOREVER: CheckDlgButton(IDC_PLAYFOREVER, BST_CHECKED); break;
		case PLAYTIME_USEDEFAULTLENGTH: CheckDlgButton(IDC_PLAYDEFAULT, BST_CHECKED); break;
		case PLAYTIME_NORMAL: CheckDlgButton(IDC_PLAYNORMAL, BST_CHECKED); break;
	}

	CheckDlgButton(IDC_DETECTSILENCE, cfg_DetectSilence?BST_CHECKED:BST_UNCHECKED);

	uSetDlgItemText(m_hWnd, IDC_DEFLENGTH, cfg_DefaultLength);
	uSetDlgItemText(m_hWnd, IDC_FADELENGTH, cfg_DefaultFade);
	uSetDlgItemText(m_hWnd, IDC_SILLENGTH, cfg_DetectSilenceTime);

	/*SendDlgItemMessage(IDC_PRIORITY, TBM_SETRANGE, 1, MAKELONG(0,6));
	SendDlgItemMessage(IDC_PRIORITY, TBM_SETPOS, 1, 6 - cfg_Priority);*/

	//uSetDlgItemText(m_hWnd, IDC_PRIORITYTXT, PriStr[cfg_Priority]);

	CWindow wnd = GetDlgItem(IDC_FADETYPE);
	uSendMessageText(wnd, CB_ADDSTRING, 0, "None");
	uSendMessageText(wnd, CB_ADDSTRING, 0, "Linear");
	uSendMessageText(wnd, CB_ADDSTRING, 0, "Logarithmic");
	SendMessage(wnd, CB_SETCURSEL, cfg_FadeType, 0);

	return TRUE;
}

void UsfPreferences::OnVScroll(UINT nSBCode, UINT nPos, CTrackBarCtrl pScrollBar)
{
	uint32_t index = 6 - pScrollBar.GetPos();

	uSetDlgItemText(m_hWnd, IDC_PRIORITYTXT, PriStr[index]);

	OnChanged();
}

void UsfPreferences::OnSelectionChange(UINT, int, CWindow)
{
	//SendDlgItemMessage( IDC_INTERPOLATION, CB_GETCURSEL ) > 0 );
	OnChanged();
}

void UsfPreferences::OnEditChange(UINT, int, CWindow)
{
	// not much to do here
	OnChanged();
}

t_uint32 UsfPreferences::get_state()
{
	t_uint32 state = preferences_state::resettable;
	if (HasChanged()) state |= preferences_state::changed;
	return state;
}

void UsfPreferences::reset()
{
	CheckDlgButton(IDC_INTERPRETERCPU, BST_UNCHECKED);
	CheckDlgButton(IDC_RSPINTERPRETER, BST_UNCHECKED);
	CheckDlgButton(IDC_AUDIOHLE, BST_UNCHECKED);
	CheckDlgButton(IDC_PLAYFOREVER, BST_UNCHECKED);
	CheckDlgButton(IDC_PLAYDEFAULT, BST_UNCHECKED);


	CheckDlgButton(IDC_RECOMPILERCPU, BST_CHECKED);
	CheckDlgButton(IDC_RSPRECOMPILER, BST_CHECKED);
	CheckDlgButton(IDC_FASTSEEK, BST_CHECKED);
	CheckDlgButton(IDC_PLAYNORMAL, BST_CHECKED);
	CheckDlgButton(IDC_DETECTSILENCE, BST_CHECKED);

	uSetDlgItemText(m_hWnd, IDC_DEFLENGTH, "180");
	uSetDlgItemText(m_hWnd, IDC_FADELENGTH, "5");
	uSetDlgItemText(m_hWnd, IDC_SILLENGTH, "5");

	SendDlgItemMessage(IDC_PRIORITY, TBM_SETRANGE, 1, MAKELONG(0,6));
	SendDlgItemMessage(IDC_PRIORITY, TBM_SETPOS, 1, 3);

	uSetDlgItemText(m_hWnd, IDC_PRIORITYTXT, PriStr[3]);

	SendDlgItemMessage(IDC_FADETYPE, CB_SETCURSEL, 1);

	OnChanged();
}

void UsfPreferences::apply()
{
	if(IsDlgButtonChecked(IDC_INTERPRETERCPU))
		cfg_CpuMode = CPU_INTERPRETER;
	else
		cfg_CpuMode = CPU_RECOMPILER;

	if(IsDlgButtonChecked(IDC_RSPRECOMPILER))
		cfg_RspMode = RSP_RECOMPILER;
	else if(IsDlgButtonChecked(IDC_RSPINTERPRETER))
		cfg_RspMode = RSP_INTERPRETER;
	else if(IsDlgButtonChecked(IDC_AUDIOHLE))
		cfg_RspMode = RSP_AUDIOHLE;

	cfg_FastSeek = IsDlgButtonChecked(IDC_FASTSEEK)?true:false;

	if(IsDlgButtonChecked(IDC_PLAYFOREVER))
		cfg_PlayTime = PLAYTIME_FOREVER;
	else if(IsDlgButtonChecked(IDC_PLAYDEFAULT))
		cfg_PlayTime = PLAYTIME_USEDEFAULTLENGTH;
	else if(IsDlgButtonChecked(IDC_PLAYNORMAL))
		cfg_PlayTime = PLAYTIME_NORMAL;

	cfg_DefaultLength = uGetDlgItemText(m_hWnd, IDC_DEFLENGTH).get_ptr();
	cfg_DefaultFade = uGetDlgItemText(m_hWnd, IDC_FADELENGTH).get_ptr();
	cfg_DetectSilenceTime = uGetDlgItemText(m_hWnd, IDC_SILLENGTH).get_ptr();

	cfg_DetectSilence = IsDlgButtonChecked(IDC_DETECTSILENCE);

	//cfg_Priority = 6 - SendDlgItemMessage(IDC_PRIORITY, TBM_GETPOS, 0, 0);

	cfg_FadeType = SendDlgItemMessage(IDC_FADETYPE, CB_GETCURSEL);

	OnChanged(); //our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
}

bool UsfPreferences::HasChanged()
{
	//returns whether our dialog content is different from the current configuration (whether the apply button should be enabled or not)
	bool Changed = false;

	if(IsDlgButtonChecked(IDC_INTERPRETERCPU))
	{
		if(cfg_CpuMode != CPU_INTERPRETER) return true;
	}
	else
	{
		if(cfg_CpuMode != CPU_RECOMPILER) return true;
	}

	if(IsDlgButtonChecked(IDC_RSPRECOMPILER))
	{
		if(cfg_RspMode != RSP_RECOMPILER) return true;
	}
	else if(IsDlgButtonChecked(IDC_RSPINTERPRETER))
	{
		if(cfg_RspMode != RSP_INTERPRETER) return true;
	}
	else if(IsDlgButtonChecked(IDC_AUDIOHLE))
	{
		if(cfg_RspMode != RSP_AUDIOHLE) return true;
	}

	if(cfg_FastSeek && !IsDlgButtonChecked(IDC_FASTSEEK)) return true;
	if(cfg_DetectSilence && !IsDlgButtonChecked(IDC_DETECTSILENCE)) return true;

	if(IsDlgButtonChecked(IDC_PLAYFOREVER))
	{
		if(cfg_PlayTime != PLAYTIME_FOREVER) return true;
	}
	else if(IsDlgButtonChecked(IDC_PLAYDEFAULT))
	{
		if(cfg_PlayTime != PLAYTIME_USEDEFAULTLENGTH) return true;
	}
	else if(IsDlgButtonChecked(IDC_PLAYNORMAL))
	{
		if(cfg_PlayTime != PLAYTIME_NORMAL) return true;
	}

	if(SendDlgItemMessage(IDC_FADETYPE, CB_GETCURSEL) != cfg_FadeType) return true;

	pfc::string sLength(cfg_DefaultLength);
	pfc::string sFade(cfg_DefaultFade);
	pfc::string sSil(cfg_DetectSilenceTime);

	if(sLength != uGetDlgItemText(m_hWnd, IDC_DEFLENGTH)) return true;
	if(sFade != uGetDlgItemText(m_hWnd, IDC_FADELENGTH)) return true;
	if(sSil != uGetDlgItemText(m_hWnd, IDC_SILLENGTH)) return true;

	/*uint32_t Priority = 6 - SendDlgItemMessage(IDC_PRIORITY, TBM_GETPOS, 0, 0);
	if(Priority != cfg_Priority) return true;*/

	return false;
}
void UsfPreferences::OnChanged()
{
	//tell the host that our state has changed to enable/disable the apply button appropriately.
	m_callback->on_state_changed();
}

typedef struct
{
	pfc::string8 length, fade;
} INFOSTRUCT;

static BOOL CALLBACK TimeProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		uSetWindowLong(wnd,DWL_USER,lp);
		{
			INFOSTRUCT * i=(INFOSTRUCT*)lp;
			if (!i->length.length() && !i->fade.length()) uSetWindowText(wnd, "Set length");
			else uSetWindowText(wnd, "Edit length");
			uSetDlgItemText(wnd, IDC_LENGTH, i->length);
			uSetDlgItemText(wnd, IDC_FADE, i->fade);
		}
		cfg_placement.on_window_creation(wnd);
		return 1;
	case WM_COMMAND:
		switch(wp)
		{
		case IDOK:
			{
				INFOSTRUCT * i=(INFOSTRUCT*)uGetWindowLong(wnd,DWL_USER);
				i->length = string_utf8_from_window(wnd, IDC_LENGTH);
				i->fade = string_utf8_from_window(wnd, IDC_FADE);
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

static bool context_time_dialog(pfc::string_base & length, pfc::string_base & fade)
{
	bool ret;
	INFOSTRUCT * i = new INFOSTRUCT;
	i->length = length;
	i->fade = fade;
	HWND hwnd = core_api::get_main_window();
	ret = uDialogBox(IDD_TIME, hwnd, TimeProc, (LONG)i) > 0;
	if (ret)
	{
		length = i->length;
		fade = i->fade;
	}
	delete i;
	return ret;
}

class length_info_filter : public file_info_filter
{
	bool set_length, set_fade;
	pfc::string8 m_length, m_fade;

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

	void length( const char * p_length )
	{
		set_length = true;
		m_length = p_length;
	}

	void fade( const char * p_fade )
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
				if ( m_length.length() ) p_info.info_set( "usf_length", m_length );
				else p_info.info_remove( "usf_length" );
			}
			if ( set_fade )
			{
				if ( m_fade ) p_info.info_set( "usf_fade", m_fade );
				else p_info.info_remove( "usf_fade" );
			}
			return set_length | set_fade;
		}
		else
		{
			return false;
		}
	}
};

class context_usf : public contextmenu_item_simple
{
public:
	virtual unsigned get_num_items() { return 1; }

	virtual void get_item_name(unsigned n, pfc::string_base & out)
	{
		if (n) uBugCheck();
		out = "Edit length";
	}

	/*virtual void get_item_default_path(unsigned n, pfc::string_base & out)
	{
		out.reset();
	}*/
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
		static const GUID guid = { 0x34e2a64e, 0x8964, 0x4351, { 0x90, 0xba, 0x54, 0x8, 0x2d, 0xc7, 0x6c, 0x1a } };
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
			if ( pfc::stricmp_ascii( ext, "USF" ) && pfc::stricmp_ascii( ext, "MINIUSF" ) ) return false;
		}
		if (i == 1) out = "Edit length";
		else out = "Set length";
		return true;
	}

	virtual void context_command(unsigned n,const pfc::list_base_const_t<metadb_handle_ptr> & data,const GUID& caller)
	{
		if (n) uBugCheck();
		pfc::string8 tag_length, tag_fade;
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
				const char * value = p_info->info_get( "usf_length" );
				if ( value ) tag_length = value;
				value = p_info->info_get( "usf_fade" );
				if ( value ) tag_fade = value;
			}
			handle->metadb_unlock();
		}
		if (!context_time_dialog(tag_length, tag_fade)) return;
		static_api_ptr_t<metadb_io_v2> p_imgr;

		service_ptr_t<length_info_filter> p_filter = new service_impl_t< length_info_filter >( data );
		if ( tag_length.length() ) p_filter->length( tag_length );
		if ( tag_fade.length() ) p_filter->fade( tag_fade );

		p_imgr->update_info_async( data, p_filter, core_api::get_main_window(), 0, 0 );
	}
};



static preferences_page_factory_t <input_usf_pref> g_input_usf_preferences_page_factory;
static input_singletrack_factory_t<input_usf>      g_input_usf_factory;
static contextmenu_item_factory_t <context_usf>    g_contextmenu_item_usf_factory;


DECLARE_COMPONENT_VERSION("LazyUSF", VERSION, ABOUTMSG);
DECLARE_FILE_TYPE("USF File","*.usf;*.miniusf");

VALIDATE_COMPONENT_FILENAME("foo_input_usf.dll");
