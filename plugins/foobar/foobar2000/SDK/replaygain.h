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
	
	t_source_mode m_source_mode;
	t_processing_mode m_processing_mode;
	float m_preamp_without_rg, m_preamp_with_rg;//preamp values in dB

	void reset();
	audio_sample query_scale(const file_info & info) const;
	audio_sample query_scale(const metadb_handle_ptr & info) const;
};

class NOVTABLE replaygain_manager : public service_base
{
public:
	virtual void get_core_settings(t_replaygain_config & p_out) = 0;

	virtual HWND configure_embedded(const t_replaygain_config & p_initdata,HWND p_parent,unsigned p_id) = 0;
	virtual void configure_embedded_retrieve(HWND wnd,t_replaygain_config & p_data) = 0;
	//sends WM_COMMAND with id/BN_CLICKED to parent when changes

	virtual bool configure_popup(t_replaygain_config & p_data,HWND p_parent,const char * p_title) = 0;//title can be null

	virtual void set_core_settings(const t_replaygain_config & p_config) = 0;

	audio_sample core_settings_query_scale(const file_info & p_info);
	audio_sample core_settings_query_scale(const metadb_handle_ptr & info);

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}
};

#endif //_FOOBAR2000_SDK_REPLAYGAIN_H_