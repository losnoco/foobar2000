#ifndef _POPUP_MESSAGE_H_
#define _POPUP_MESSAGE_H_


class NOVTABLE popup_message : public service_base
{
public:
	enum t_icon {icon_information, icon_error, icon_query};
	virtual void show_ex(const char * p_msg,unsigned p_msg_length,const char * p_title,unsigned p_title_length,t_icon p_icon = icon_information) = 0;

	inline void show(const char * p_msg,const char * p_title,t_icon p_icon = icon_information) {show_ex(p_msg,infinite,p_title,infinite,p_icon);}

	static void g_show_ex(const char * p_msg,unsigned p_msg_length,const char * p_title,unsigned p_title_length,t_icon p_icon = icon_information);
	static inline void g_show(const char * p_msg,const char * p_title,t_icon p_icon = icon_information) {g_show_ex(p_msg,infinite,p_title,infinite,p_icon);}

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	popup_message() {}
	~popup_message() {}
};

#endif //_POPUP_MESSAGE_H_