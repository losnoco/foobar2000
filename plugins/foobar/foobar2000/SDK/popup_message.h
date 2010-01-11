#ifndef _POPUP_MESSAGE_H_
#define _POPUP_MESSAGE_H_


class NOVTABLE popup_message : public service_base
{
public:
	virtual void show_ex(const char * p_msg,unsigned p_msg_length,const char * p_title,unsigned p_title_length) = 0;

	inline void show(const char * p_msg,const char * p_title) {show_ex(p_msg,infinite,p_title,infinite);}

	static void g_show_ex(const char * p_msg,unsigned p_msg_length,const char * p_title,unsigned p_title_length);
	static inline void g_show(const char * p_msg,const char * p_title) {g_show_ex(p_msg,infinite,p_title,infinite);}

	static void g_show_file_error(const char * p_message,t_io_result p_status,const char * p_path,const char * p_title);
	static void g_show_file_error_multi(const char * p_message,const list_base_const_t<const char *> & p_list,const char * p_title);

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}

};

#endif //_POPUP_MESSAGE_H_