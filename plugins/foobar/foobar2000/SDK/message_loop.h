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

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	message_loop() {}
	~message_loop() {}
};