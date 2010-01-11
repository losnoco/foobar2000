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
	virtual bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta) = 0;
	virtual bool decode_get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta) = 0;

	static t_io_result g_open(service_ptr_t<ogg_stream_handler> & p_out,service_ptr_t<file> p_reader,t_input_open_reason p_reason,abort_callback & p_abort);

	static const GUID class_guid;
	
	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	ogg_stream_handler() {}
	~ogg_stream_handler() {}
};


template<class T>
class ogg_stream_handler_factory_t : public service_factory_t<ogg_stream_handler,T> {};