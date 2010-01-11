#ifndef _DSP_MANAGER_H_
#define _DSP_MANAGER_H_

#include "dsp.h"

class dsp_manager
{
public:
	dsp_manager() : m_config_changed(false) {}

	void set_config( const dsp_chain_config & p_data );
	double run(dsp_chunk_list * list,const metadb_handle_ptr & cur_file,unsigned flags);
	void flush();

	bool is_active();

private:
	service_list_t<dsp> m_dsp_list;
	dsp_chain_config_impl m_config;
	bool m_config_changed;
	
	void dsp_run(unsigned idx,dsp_chunk_list * list,const metadb_handle_ptr & cur_file,unsigned flags,double & latency);
};


class dsp_config_manager : public service_base
{
public:
	virtual void get_core_settings(dsp_chain_config & p_out) = 0;
	virtual void set_core_settings(const dsp_chain_config & p_data) = 0;
	
	virtual bool configure_popup(dsp_chain_config & p_data,HWND p_parent,const char * p_title) = 0;
	
	virtual HWND configure_embedded(const dsp_chain_config & p_initdata,HWND p_parent,unsigned p_id) = 0;
	virtual void configure_embedded_retrieve(HWND wnd,dsp_chain_config & p_data) = 0;
	virtual void configure_embedded_change(HWND wnd,const dsp_chain_config & p_data) = 0;

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}
protected:
	inline dsp_config_manager() {}
	inline ~dsp_config_manager() {}
};

class NOVTABLE dsp_config_callback : public service_base
{
public:
	virtual void on_core_settings_change(const dsp_chain_config & p_newdata) = 0;

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}
	
protected:
	inline dsp_config_callback() {}
	inline ~dsp_config_callback() {}

};

#endif