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

	class formatter : public string_formatter
	{
	public:
		~formatter() {if (!is_empty())console::print(get_ptr());}
	};
};

//! Interface receiving console output. Do not reimplement or call directly; use console namespace functions instead.
class NOVTABLE console_receiver : public service_base
{
public:
	virtual void print(const char * p_message,unsigned p_message_length) = 0;

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}
};

#endif