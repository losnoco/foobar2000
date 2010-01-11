#ifndef _COMPONENTVERSION_H_
#define _COMPONENTVERSION_H_

class NOVTABLE componentversion : public service_base
{
public:
	virtual void get_file_name(string_base & out)=0;
	virtual void get_component_name(string_base & out)=0;
	virtual void get_component_version(string_base & out)=0;
	virtual void get_about_message(string_base & out)=0;//about message uses "\n" for line separators

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	componentversion() {}
	~componentversion() {}
};

class componentversion_impl_simple : public componentversion
{
	const char * name,*version,*about;
public:
	//do not derive/override
	virtual void get_file_name(string_base & out) {out.set_string(core_api::get_my_file_name());}
	virtual void get_component_name(string_base & out) {out.set_string(name?name:"");}
	virtual void get_component_version(string_base & out) {out.set_string(version?version:"");}
	virtual void get_about_message(string_base & out) {out.set_string(about?about:"");}
	explicit componentversion_impl_simple(const char * p_name,const char * p_version,const char * p_about) : name(p_name), version(p_version), about(p_about ? p_about : "") {}
};

class componentversion_impl_copy : public componentversion
{
	string_simple name,version,about;
public:
	//do not derive/override
	virtual void get_file_name(string_base & out) {out.set_string(core_api::get_my_file_name());}
	virtual void get_component_name(string_base & out) {out.set_string(name);}
	virtual void get_component_version(string_base & out) {out.set_string(version);}
	virtual void get_about_message(string_base & out) {out.set_string(about);}
	explicit componentversion_impl_copy(const char * p_name,const char * p_version,const char * p_about) : name(p_name), version(p_version), about(p_about ? p_about : "") {}
};


#define DECLARE_COMPONENT_VERSION(NAME,VERSION,ABOUT) \
	static service_factory_single_transparent_p3_t<componentversion,componentversion_impl_simple,const char*,const char*,const char*> g_componentversion_service(NAME,VERSION,ABOUT);

#define DECLARE_COMPONENT_VERSION_COPY(NAME,VERSION,ABOUT) \
	static service_factory_single_transparent_p3_t<componentversion,componentversion_impl_copy,const char*,const char*,const char*> g_componentversion_service(NAME,VERSION,ABOUT);

//usage: DECLARE_COMPONENT_VERSION("blah","v1.337",0)
//about message is optional can be null
//_copy version copies strings around instead of keeping pointers (bigger but sometimes needed, eg. if strings are created as string_printf() or something inside constructor

#endif