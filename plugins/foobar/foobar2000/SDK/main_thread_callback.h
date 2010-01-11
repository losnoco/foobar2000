class NOVTABLE main_thread_callback {
public:
	virtual void callback_run() = 0;
protected:
	main_thread_callback() {}
	~main_thread_callback() {}
};

class NOVTABLE main_thread_callback_manager : public service_base {
public:
	virtual void add_callback(main_thread_callback * p_callback) = 0;
	virtual void flush() = 0;

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	main_thread_callback_manager() {}
	~main_thread_callback_manager() {}
};