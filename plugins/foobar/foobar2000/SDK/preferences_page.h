#ifndef _FOOBAR2000_SDK_PREFERENCES_PAGE_H_
#define _FOOBAR2000_SDK_PREFERENCES_PAGE_H_

//! Implementing this service will generate a page in preferences dialog. Use preferences_page_factory_t template to register.
class NOVTABLE preferences_page : public service_base {
public:
	//! Creates preferences page dialog window. It is safe to assume that two dialog instances will never coexist. Caller is responsible for embedding it into preferences dialog itself.
	virtual HWND create(HWND parent) = 0;
	//! Retrieves name of the prefernces page to be displayed in preferences tree (static string).
	virtual const char * get_name() = 0;
	//! Retrieves GUID of the page.
	virtual GUID get_guid() = 0;
	//! Retrieves GUID of parent page/branch of this page. See preferences_page::guid_* constants for list of standard parent GUIDs. Can also be a GUID of another page or a branch (see: preferences_branch).
	virtual GUID get_parent_guid() = 0;
	//! Queries whether this page supports "reset page" feature.
	virtual bool reset_query() = 0;
	//! Activates "reset page" feature. It is safe to assume that the preferences page dialog does not exist at the point this is called (caller destroys it before calling reset and creates it again afterwards).
	virtual void reset() = 0;
	//! Retrieves help URL. Without overriding it, it will redirect to foobar2000 wiki.
	virtual bool get_help_url(pfc::string_base & p_out);
	
	static const GUID guid_root, guid_hidden, guid_tools,guid_core,guid_display,guid_playback,guid_visualisations,guid_input,guid_tag_writing,guid_media_library;

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

//! Creates a preferences branch - an empty page that only serves as a parent for other pages and is hidden when no child pages exist. Use preferences_branch_factory_t template to register.
class NOVTABLE preferences_branch : public service_base {
public:
	//! Retrieves name of the preferences branch.
	virtual const char * get_name() = 0;
	//! Retrieves GUID of the preferences branch. Use this GUID as parent GUID for pages/branches nested in this branch.
	virtual GUID get_guid() = 0;
	//! Retrieves GUID of parent page/branch of this branch. See preferences_page::guid_* constants for list of standard parent GUIDs. Can also be a GUID of another branch or a page.
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

class preferences_branch_impl : public preferences_branch {
public:
	preferences_branch_impl(const GUID & p_guid,const GUID & p_parent,const char * p_name) : m_guid(p_guid), m_parent(p_parent), m_name(p_name) {}
	const char * get_name() {return m_name;}
	GUID get_guid() {return m_guid;}
	GUID get_parent_guid() {return m_parent;}
private:
	GUID m_guid,m_parent;
	pfc::string8 m_name;
};

typedef service_factory_single_t<preferences_branch,preferences_branch_impl> __preferences_branch_factory;

class preferences_branch_factory : public __preferences_branch_factory {
public:
	preferences_branch_factory(const GUID & p_guid,const GUID & p_parent,const char * p_name) : __preferences_branch_factory(p_guid,p_parent,p_name) {}
};

#endif //_FOOBAR2000_SDK_PREFERENCES_PAGE_H_