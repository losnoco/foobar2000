class NOVTABLE ogg_stream_handler : public service_base
{
public:
	virtual t_io_result open(service_ptr_t<file> p_reader,t_input_open_reason p_reason,abort_callback & p_abort) = 0;
	virtual t_io_result get_info(file_info & p_info,abort_callback & p_abort) = 0;
	virtual t_io_result decode_initialize(abort_callback & p_abort) = 0;
	virtual t_io_result decode_run(audio_chunk & p_chunk,abort_callback & p_abort) = 0;
	virtual t_io_result decode_seek(double p_seconds,abort_callback & p_abort) = 0;
	virtual t_io_result retag(const file_info & p_info,stream_writer * p_out,abort_callback & p_abort) = 0;
	virtual t_io_result retag_inplace(const file_info & p_info,bool & p_retagged,abort_callback & p_abort) = 0;
	virtual bool decode_can_seek() = 0;
	virtual bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta,bool & p_track_change) = 0;

	static t_io_result g_open(service_ptr_t<ogg_stream_handler> & p_out,service_ptr_t<file> p_reader,t_input_open_reason p_reason,abort_callback & p_abort);

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

	
	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}
};


template<class T>
class ogg_stream_handler_factory_t : public service_factory_t<ogg_stream_handler,T> {};