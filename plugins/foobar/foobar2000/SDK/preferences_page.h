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

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

	static const GUID guid_root, guid_hidden, guid_components,guid_core,guid_display,guid_playback,guid_visualisations,guid_input,guid_tag_writing,guid_media_library;
};

template<class T>
class preferences_page_factory_t : public service_factory_single_t<preferences_page,T> {};

#endif //_FOOBAR2000_SDK_PREFERENCES_PAGE_H_