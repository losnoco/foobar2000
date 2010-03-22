#ifndef _FOOMAIN_H_
#define _FOOMAIN_H_

#include <foobar2000.h>
#include <helpers.h>
#include <shared.h>
#include <ATLHelpers.h>
#include "usf.h"
#include "resource.h"

const char * VERSION = "2.0 (Build " __DATE__ ")";

#define ABOUTMSG	"LazyUSF v2.0 beta for Foobar2000\nBased on Project 64 1.6 by Zilmar and Jabo\n" \
					"Audio HLE by Azimer\nPSF concept and tagging by Neill Corlett\n" \
					"USF Format by hcs (http://hcs64.com/usf)\nPlayer by Josh W (http://joshw.info/)"


class input_usf
{
	public:
		input_usf();
		~input_usf();
		void open(service_ptr_t<file> p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort);
		void get_info(file_info & p_info,abort_callback & p_abort);
		void decode_initialize(unsigned p_flags,abort_callback & p_abort);
		bool decode_run(audio_chunk & p_chunk,abort_callback & p_abort);
		void decode_seek(double p_seconds,abort_callback & p_abort);
		bool decode_can_seek() { return true; }
		bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta);
		bool decode_get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta);
		void decode_on_idle(abort_callback & p_abort) { }
		t_filestats get_file_stats(abort_callback & p_abort);

		void retag(const file_info & p_info,abort_callback & p_abort);

		static bool g_is_our_content_type(const char * p_content_type) {return false;}
		static bool g_is_our_path(const char * p_path,const char * p_extension)
		{
			if(stricmp_utf8(p_extension, "miniusf") == 0) return 1;
			return stricmp_utf8(p_extension, "usf") == 0;
		}

	private:
		Usf * mUsf;
		bool no_looping;
		t_filestats m_stats;
};


class UsfPreferences : public CDialogImpl<UsfPreferences>, public preferences_page_instance {
public:
	//Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
	UsfPreferences(preferences_page_callback::ptr callback) : m_callback(callback) {}

	//Note that we don't bother doing anything regarding destruction of our class.
	//The host ensures that our dialog is destroyed first, then the last reference to our preferences_page_instance object is released, causing our object to be deleted.


	//dialog resource ID
	enum {IDD = IDD_PREFERENCES};
	// preferences_page_instance methods (not all of them - get_wnd() is supplied by preferences_page_impl helpers)
	t_uint32 get_state();
	void apply();
	void reset();

	//WTL message map
	BEGIN_MSG_MAP(UsfPreferences)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_VSCROLL(OnVScroll)
		COMMAND_HANDLER_EX(IDC_DEFLENGTH, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_FADELENGTH, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_SILLENGTH, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_RECOMPILERCPU, BN_CLICKED, OnEditChange)
		COMMAND_HANDLER_EX(IDC_INTERPRETERCPU, BN_CLICKED, OnEditChange)
		COMMAND_HANDLER_EX(IDC_RSPRECOMPILER, BN_CLICKED, OnEditChange)
		COMMAND_HANDLER_EX(IDC_RSPINTERPRETER, BN_CLICKED, OnEditChange)
		COMMAND_HANDLER_EX(IDC_AUDIOHLE, BN_CLICKED, OnEditChange)
		COMMAND_HANDLER_EX(IDC_PLAYFOREVER, BN_CLICKED, OnEditChange)
		COMMAND_HANDLER_EX(IDC_PLAYDEFAULT, BN_CLICKED, OnEditChange)
		COMMAND_HANDLER_EX(IDC_PLAYNORMAL, BN_CLICKED, OnEditChange)
		COMMAND_HANDLER_EX(IDC_FASTSEEK, BN_CLICKED, OnEditChange)
		COMMAND_HANDLER_EX(IDC_DETECTSILENCE, BN_CLICKED, OnEditChange)
		COMMAND_HANDLER_EX(IDC_FADETYPE, CBN_SELCHANGE, OnSelectionChange)
	END_MSG_MAP()
private:
	BOOL OnInitDialog(CWindow, LPARAM);
	void OnEditChange(UINT, int, CWindow);
	void OnVScroll(UINT nSBCode, UINT nPos, CTrackBarCtrl pScrollBar);
	bool HasChanged();
	void OnChanged();
	void OnSelectionChange(UINT, int, CWindow);

	const preferences_page_callback::ptr m_callback;
};

class input_usf_pref : public preferences_page_impl<UsfPreferences>
{

public:
	const char * get_name();
	GUID get_guid();
	GUID get_parent_guid();

};

#endif

