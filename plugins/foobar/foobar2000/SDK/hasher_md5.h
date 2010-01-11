struct hasher_md5_state
{
	char m_data[128];
};

struct hasher_md5_result
{
	char m_data[16];
};

class NOVTABLE hasher_md5 : public service_base
{
public:

	virtual void initialize(hasher_md5_state & p_state) = 0;
	virtual void process(hasher_md5_state & p_state,const void * p_buffer,unsigned p_bytes) = 0;
	virtual hasher_md5_result get_result(const hasher_md5_state & p_state) = 0;

	static GUID guid_from_result(const hasher_md5_result & param);

	hasher_md5_result process_single(const void * p_buffer,unsigned p_bytes);
	GUID process_single_guid(const void * p_buffer,unsigned p_bytes);


	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

};