class NOVTABLE message_filter
{
public:
    virtual bool pretranslate_message(MSG * p_msg) = 0;
};
 
class NOVTABLE idle_handler
{
public:
    virtual bool on_idle() = 0;
};
 
class NOVTABLE message_loop : public service_base
{
public:
    virtual void add_message_filter(message_filter * ptr) = 0;
    virtual void remove_message_filter(message_filter * ptr) = 0;
 
    virtual void add_idle_handler(idle_handler * ptr) = 0;
    virtual void remove_idle_handle(idle_handler * ptr) = 0;


	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}
};