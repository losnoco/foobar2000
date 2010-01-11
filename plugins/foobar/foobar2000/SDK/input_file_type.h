class input_file_type : public service_base
{
public:
	virtual unsigned get_count()=0;
	virtual bool get_name(unsigned idx,string_base & out)=0;//e.g. "MPEG file"
	virtual bool get_mask(unsigned idx,string_base & out)=0;//e.g. "*.MP3;*.MP2"; separate with semicolons
	virtual bool is_associatable(unsigned idx) = 0;
	
	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}

	static void build_openfile_mask(string_base & out,bool b_include_playlists=true);
};

class input_file_type_impl : public service_impl_single_t<input_file_type>
{
	const char * name, * mask;
	bool m_associatable;
public:
	input_file_type_impl(const char * p_name, const char * p_mask,bool p_associatable) : name(p_name), mask(p_mask), m_associatable(p_associatable) {}
	unsigned get_count() {return 1;}
	bool get_name(unsigned idx,string_base & out) {if (idx==0) {out = name; return true;} else return false;}
	bool get_mask(unsigned idx,string_base & out) {if (idx==0) {out = mask; return true;} else return false;}
	bool is_associatable(unsigned idx) {return m_associatable;}
};


#define DECLARE_FILE_TYPE(NAME,MASK) \
	namespace { static input_file_type_impl g_filetype_instance(NAME,MASK,true); \
	static service_factory_single_ref_t<input_file_type,input_file_type_impl> g_filetype_service(g_filetype_instance); }


//USAGE: DECLARE_FILE_TYPE("Blah file","*.blah;*.bleh");

class input_file_type_factory : private service_factory_single_transparent_p3_t<input_file_type,input_file_type_impl,const char*,const char*,bool>
{
public:
	input_file_type_factory(const char * p_name,const char * p_mask,bool p_associatable)
		: service_factory_single_transparent_p3_t<input_file_type,input_file_type_impl,const char*,const char*,bool>(p_name,p_mask,p_associatable) {}
};


//usage: static input_file_type_factory mytype("blah type","*.bla;*.meh",true)