#ifndef _CONSOLE_H_
#define _CONSOLE_H_

//! Namespace with functions for sending text to console. All functions are fully multi-thread safe, thought they must not be called during dll initialization or deinitialization (e.g. static object constructors or destructors) when service system is not available.
namespace console
{
	void info(const char * p_message);
	void error(const char * p_message);
	void warning(const char * p_message);
	void info_location(const playable_location & src);
	void info_location(const metadb_handle_ptr & src);
	void print_location(const playable_location & src);
	void print_location(const metadb_handle_ptr & src);

	void print(const char*);
    void printf(const char*,...);
	void printfv(const char*,va_list p_arglist);

	class formatter : public pfc::string8_fastalloc {
	public:
		~formatter() {if (!is_empty()) console::print(get_ptr());}
	};
};

//! Interface receiving console output. Do not reimplement or call directly; use console namespace functions instead.
class NOVTABLE console_receiver : public service_base
{
public:
	virtual void print(const char * p_message,unsigned p_message_length) = 0;

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	console_receiver() {}
	~console_receiver() {}
};

#endif