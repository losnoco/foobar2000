#ifndef _OUTPUT_MANAGER_H_
#define _OUTPUT_MANAGER_H_

#include "output.h"

struct t_output_config
{
	GUID m_output;//null guid for default user-configured device
	GUID m_device;

	double m_buffer_length;//null for default user-configured buffer length

	int m_override_format;

	struct t_format
	{
		int bps,bps_pad;
		int b_float;
		int b_dither;
	};

	t_format m_format;//format valid only if m_override_format is set to non-zero, otherwise user settings from core preferences are used
	
	t_output_config() 
	{
		reset();
	}

	t_output_config(t_output_config const & p_config) {*this = p_config;}

	void reset()
	{
		m_output = pfc::guid_null;
		m_device = pfc::guid_null;
		m_buffer_length = 0;
		m_override_format = false;

		m_format.bps = 16;
		m_format.bps_pad = 16;
		m_format.b_float = 0;
		m_format.b_dither = 1;
	}
};

class NOVTABLE output_manager : public service_base
{
public:
	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

	static bool g_create(service_ptr_t<output_manager> & p_out) {return service_enum_create_t(p_out,0);}

	virtual void get_default_config(t_output_config & out)=0;//retrieves current user settings from core preferences; do not pass returned data to set_config (same effect as not calling set_config at all or passing uninitialized struct)

	virtual void set_config(const t_output_config & param)=0;//optionally call before first use
	
	virtual double get_latency()=0;//in seconds
	virtual t_io_result process_samples(const audio_chunk& chunk)=0;//1 on success, 0 on error, no sleeping, copies data to its own buffer immediately; format_code - see enums below
	virtual t_io_result update(bool & p_ready)=0;
	virtual t_io_result pause(bool state)=0;
	virtual t_io_result flush()=0;
	virtual t_io_result force_play()=0;
	virtual t_io_result set_volume(double p_val) = 0;
};

#endif //_OUTPUT_MANAGER_H_