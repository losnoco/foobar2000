#ifndef _FOOBAR2000_SDK_PREFERENCES_PAGE_H_
#define _FOOBAR2000_SDK_PREFERENCES_PAGE_H_

class NOVTABLE preferences_page : public service_base
{
public:
	virtual HWND create(HWND parent)=0;
	virtual const char * get_name()=0;
	virtual GUID get_guid() = 0;
	virtual GUID get_parent_guid() = 0;
	virtual bool reset_query() = 0;
	virtual void reset() = 0;
	virtual bool get_help_url(string_base & p_out);
	
	static const GUID guid_root, guid_hidden, guid_components,guid_core,guid_display,guid_playback,guid_visualisations,guid_input,guid_tag_writing,guid_media_library;

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	preferences_page() {}
	~preferences_page() {}
};

template<class T>
class preferences_page_factory_t : public service_factory_single_t<preferences_page,T> {};

class NOVTABLE preferences_branch : public service_base
{
public:
	virtual const char * get_name() = 0;
	virtual GUID get_guid() = 0;
	virtual GUID get_parent_guid() = 0;
	
	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	preferences_branch() {}
	~preferences_branch() {}
};

template<class T>
class preferences_branch_factory_t : public service_factory_single_t<preferences_branch,T> {};

#endif //_FOOBAR2000_SDK_PREFERENCES_PAGE_H_