//! Entrypoint interface for declaring component's version information. Instead of implementing this directly, use DECLARE_COMPONENT_VERSION().
class NOVTABLE componentversion : public service_base {
public:
	virtual void get_file_name(pfc::string_base & out)=0;
	virtual void get_component_name(pfc::string_base & out)=0;
	virtual void get_component_version(pfc::string_base & out)=0;
	virtual void get_about_message(pfc::string_base & out)=0;//about message uses "\n" for line separators

	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(componentversion);
};

//! Implementation helper.
class componentversion_impl_simple : public componentversion {
	const char * name,*version,*about;
public:
	//do not derive/override
	virtual void get_file_name(pfc::string_base & out) {out.set_string(core_api::get_my_file_name());}
	virtual void get_component_name(pfc::string_base & out) {out.set_string(name?name:"");}
	virtual void get_component_version(pfc::string_base & out) {out.set_string(version?version:"");}
	virtual void get_about_message(pfc::string_base & out) {out.set_string(about?about:"");}
	explicit componentversion_impl_simple(const char * p_name,const char * p_version,const char * p_about) : name(p_name), version(p_version), about(p_about ? p_about : "") {}
};

//! Implementation helper.
class componentversion_impl_copy : public componentversion {
	pfc::string8 name,version,about;
public:
	//do not derive/override
	virtual void get_file_name(pfc::string_base & out) {out.set_string(core_api::get_my_file_name());}
	virtual void get_component_name(pfc::string_base & out) {out.set_string(name);}
	virtual void get_component_version(pfc::string_base & out) {out.set_string(version);}
	virtual void get_about_message(pfc::string_base & out) {out.set_string(about);}
	explicit componentversion_impl_copy(const char * p_name,const char * p_version,const char * p_about) : name(p_name), version(p_version), about(p_about ? p_about : "") {}
};

typedef service_factory_single_transparent_t<componentversion_impl_simple> __componentversion_impl_simple_factory;
typedef service_factory_single_transparent_t<componentversion_impl_copy> __componentversion_impl_copy_factory;

class componentversion_impl_simple_factory : public __componentversion_impl_simple_factory {
public:
	componentversion_impl_simple_factory(const char * p_name,const char * p_version,const char * p_about) : __componentversion_impl_simple_factory(p_name,p_version,p_about) {}
};

class componentversion_impl_copy_factory : public __componentversion_impl_copy_factory {
public:
	componentversion_impl_copy_factory(const char * p_name,const char * p_version,const char * p_about) : __componentversion_impl_copy_factory(p_name,p_version,p_about) {}
};

//! Use this to declare your component's version information. Parameters must ba plain const char * string constants. The ABOUT string can be NULL if you don't provide any information to show in the "About" dialog. \n
//! Example: DECLARE_COMPONENT_VERSION("blah","v1.337","")
#define DECLARE_COMPONENT_VERSION(NAME,VERSION,ABOUT) \
	namespace {class componentversion_myimpl : public componentversion { public: componentversion_myimpl() {PFC_ASSERT( ABOUT );} \
		void get_file_name(pfc::string_base & out) {out = core_api::get_my_file_name();}	\
		void get_component_name(pfc::string_base & out) {out = NAME;}	\
		void get_component_version(pfc::string_base & out) {out = VERSION;}	\
		void get_about_message(pfc::string_base & out) {out = ABOUT;}	\
		}; static service_factory_single_t<componentversion_myimpl> g_componentversion_myimpl_factory; }
	// static componentversion_impl_simple_factory g_componentversion_service(NAME,VERSION,ABOUT);

//! Same as DECLARE_COMPONENT_VERSION(), but parameters can be dynamically generated strings rather than compile-time constants.
#define DECLARE_COMPONENT_VERSION_COPY(NAME,VERSION,ABOUT) \
	static componentversion_impl_copy_factory g_componentversion_service(NAME,VERSION,ABOUT);


//! \since 1.0
//! Allows components to cleanly abort app startup in case the installation appears to have become corrupted.
class component_installation_validator : public service_base {
public:
	virtual bool is_installed_correctly() = 0;

	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(component_installation_validator)
};

//! Simple implementation of component_installation_validator that makes sure that our component DLL has not been renamed around by idiot users.
class component_installation_validator_filename : public component_installation_validator {
public:
	component_installation_validator_filename(const char * dllName) : m_dllName(dllName) {}
	bool is_installed_correctly() {
		const char * path = core_api::get_my_full_path();
		path += pfc::scan_filename(path);
		bool retVal = ( strcmp(path, m_dllName) == 0 );
		PFC_ASSERT( retVal );
		return retVal;
	}
private:
	const char * const m_dllName;
};

#define VALIDATE_COMPONENT_FILENAME(FN) \
	static service_factory_single_t<component_installation_validator_filename> g_component_installation_validator_filename(FN);
