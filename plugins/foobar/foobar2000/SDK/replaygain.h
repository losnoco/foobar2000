#ifndef _FOOBAR2000_SDK_REPLAYGAIN_H_
#define _FOOBAR2000_SDK_REPLAYGAIN_H_

#include "service.h"

#include "file_info.h"
#include "metadb_handle.h"
#include "metadb.h"

struct t_replaygain_config
{
	enum t_source_mode {source_mode_none,source_mode_track,source_mode_album};
	enum t_processing_mode {processing_mode_none,processing_mode_gain,processing_mode_gain_and_peak,processing_mode_peak};

	t_replaygain_config() {reset();}
	t_replaygain_config(const t_replaygain_config & p_source) {*this = p_source;}
	t_replaygain_config(t_source_mode p_source_mode,t_processing_mode p_processing_mode,float p_preamp_without_rg, float p_preamp_with_rg)
		: m_source_mode(p_source_mode), m_processing_mode(p_processing_mode), m_preamp_without_rg(p_preamp_without_rg), m_preamp_with_rg(p_preamp_with_rg) {}

	
	t_source_mode m_source_mode;
	t_processing_mode m_processing_mode;
	float m_preamp_without_rg, m_preamp_with_rg;//preamp values in dB

	void reset();
	audio_sample query_scale(const file_info & info) const;
	audio_sample query_scale(const metadb_handle_ptr & info) const;

	void format_name(pfc::string_base & p_out) const;
	bool is_active() const;
};

class NOVTABLE replaygain_manager : public service_base
{
public:
	virtual void get_core_settings(t_replaygain_config & p_out) = 0;

	virtual HWND configure_embedded(const t_replaygain_config & p_initdata,HWND p_parent,unsigned p_id,bool p_from_modal) = 0;
	virtual void configure_embedded_retrieve(HWND wnd,t_replaygain_config & p_data) = 0;
	//sends WM_COMMAND with id/BN_CLICKED to parent when changes

	virtual bool configure_popup(t_replaygain_config & p_data,HWND p_parent,const char * p_title) = 0;//title can be null

	virtual void set_core_settings(const t_replaygain_config & p_config) = 0;

	audio_sample core_settings_query_scale(const file_info & p_info);
	audio_sample core_settings_query_scale(const metadb_handle_ptr & info);

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	replaygain_manager() {}
	~replaygain_manager() {}

};

#endif //_FOOBAR2000_SDK_REPLAYGAIN_H_