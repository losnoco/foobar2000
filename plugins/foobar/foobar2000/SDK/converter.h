#ifndef _CONVERTER_H_
#define _CONVERTER_H_


//! Class for modular encoding of various audio file types.
class NOVTABLE converter : public service_base
{
public:
	//! Retrieves human-readable name of this service. Non-preset-dependant.
	//! @param p_out Receives name string.
	virtual void get_name(string_base & p_out)=0;

	//! Retrieves output file name extension. Preset-dependant; called only after preset_set_data() etc.
	//! @param p_out Receives extension string.
	virtual void get_extension(string_base & p_out)=0;
	
	//! Initializes encoding process.
	virtual t_io_result open(const char * p_out_path,unsigned p_expected_bps,bool p_should_dither,abort_callback & p_abort) = 0;

	//! Encodes an audio chunk.
	virtual t_io_result process_samples(const audio_chunk & p_src,abort_callback & p_abort)=0;

	//! Sets info for track being currently encoded. Called after open() / multitrack_split().
	virtual t_io_result set_info(const file_info & p_info,abort_callback & p_abort) = 0;

	//! Finalizes encoding of file.
	virtual t_io_result flush(abort_callback & p_abort)=0;
	
	
	virtual GUID get_guid()=0;//GUID of your converter subclass, for storing config

	virtual bool preset_set_data(const void * ptr,unsigned bytes)=0;
	virtual void preset_get_data(cfg_var::write_config_callback * out)=0;
	virtual bool preset_configurable()=0;//return if you support config dialog or not
	virtual bool preset_configure(HWND parent)=0;//return false if user aborted the operation

	virtual unsigned preset_default_get_count()=0;
	virtual void preset_default_enumerate(unsigned index,cfg_var::write_config_callback * out)=0;
	virtual bool preset_get_description(string_base & out)=0;

	virtual bool on_user_init(HWND p_parent) = 0;

	virtual bool multitrack_query_support() = 0;
	virtual t_io_result multitrack_split(abort_callback & p_abort) = 0;

	virtual bool is_permanent() = 0;
	

	static bool g_instantiate(service_ptr_t<converter> & p_out,const GUID & guid);
	static bool g_instantiate_test(const GUID & guid);




	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

	
	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}
};

class NOVTABLE converter_presetless : public converter //helper, preset-less
{
public:
	virtual bool preset_set_data(const void * ptr,unsigned bytes) {return true;}
	virtual void preset_get_data(cfg_var::write_config_callback * out) {}
	virtual bool preset_configurable() {return false;}
	virtual bool preset_configure(HWND parent) {return false;}
	

	virtual unsigned preset_default_get_count() {return 1;}
	virtual void preset_default_enumerate(unsigned index,cfg_var::write_config_callback * out) {}
	virtual bool preset_get_description(string_base & out) {return false;}
	
};


template<class T>
class converter_factory : public service_factory_t<converter,T> {};

#endif