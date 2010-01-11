#ifndef _FOOBAR2000_TITLEFORMAT_H_
#define _FOOBAR2000_TITLEFORMAT_H_

#include "service.h"

#include "file_info.h"

class NOVTABLE titleformat_text_out
{
public:
	virtual void write(const char * p_data,unsigned p_data_length) = 0;
	void write_int(t_int64 val);
	void write_int_padded(t_int64 val,t_int64 maxval);
};

class NOVTABLE titleformat_text_filter
{
public:
	virtual void on_new_field() = 0;
	virtual void write(titleformat_text_out * p_out,const char * p_data,unsigned p_data_length) = 0;
};

class NOVTABLE titleformat_hook_function_params
{
public:
	virtual unsigned get_param_count() = 0;
	virtual void get_param(unsigned index,const char * & p_string,unsigned & p_string_len) = 0;//warning: not a null-terminated string
	
	//helper
	unsigned get_param_uint(unsigned index);
};

class NOVTABLE titleformat_hook
{
public:
	virtual bool process_field(titleformat_text_out * p_out,const char * p_name,unsigned p_name_length,bool & p_found_flag) = 0;
	virtual bool process_function(titleformat_text_out * p_out,const char * p_name,unsigned p_name_length,titleformat_hook_function_params * p_params,bool & p_found_flag) = 0;
};

class NOVTABLE titleformat_object : public service_base
{
public:
	virtual void run(titleformat_hook * p_source,string_base & p_out)=0;

	//helpers
	void run_filtered(titleformat_hook * p_source,string_base & p_out,titleformat_text_filter * p_filter);
	void run_hook_filtered(const playable_location & p_location,const file_info * p_source,titleformat_hook * p_hook,string_base & p_out,titleformat_text_filter * p_filter);
	void run_simple(const playable_location & p_location,const file_info * p_source,string_base & p_out);

	static const GUID class_guid;
	
	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	titleformat_object() {}
	~titleformat_object() {}
};

class NOVTABLE titleformat_compiler : public service_base
{
public:
	virtual bool compile(service_ptr_t<titleformat_object> & p_out,const char * p_spec) = 0;
	
	void run(titleformat_hook * p_source,string_base & p_out,const char * p_spec);
	void compile_safe(service_ptr_t<titleformat_object> & p_out,const char * p_spec);//should never fail


	static void remove_color_marks(const char * src,string_base & out);//helper
	static void remove_forbidden_chars(titleformat_text_out * p_out,const char * p_source,unsigned p_source_len,const char * p_forbidden_chars);
	static void remove_forbidden_chars_string_append(string_base & p_out,const char * p_source,unsigned p_source_len,const char * p_forbidden_chars);
	static void remove_forbidden_chars_string(string_base & p_out,const char * p_source,unsigned p_source_len,const char * p_forbidden_chars);

	static const GUID class_guid;
	
	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	titleformat_compiler() {}
	~titleformat_compiler() {}
};



//helpers


class titleformat_text_out_impl_filter_chars : public titleformat_text_out
{
public:
	inline titleformat_text_out_impl_filter_chars(titleformat_text_out * p_chain,const char * p_restricted_chars)
		: m_chain(p_chain), m_restricted_chars(p_restricted_chars) {}
	void write(const char * p_data,unsigned p_data_length);
private:
	titleformat_text_out * m_chain;
	const char * m_restricted_chars;
};

class titleformat_text_out_impl_string : public titleformat_text_out
{
public:
	titleformat_text_out_impl_string(string_base & p_string) : m_string(p_string) {}
	void write(const char * p_data,unsigned p_data_length) {m_string.add_string(p_data,p_data_length);}
private:
	string_base & m_string;
};

class titleformat_hook_impl_file_info : public titleformat_hook
{
public:
	titleformat_hook_impl_file_info(const playable_location & p_location,const file_info * p_info) : m_location(p_location), m_info(p_info) {}//caller must ensure that referenced file_info object is alive as long as the titleformat_hook_impl_file_info instance
	bool process_field(titleformat_text_out * p_out,const char * p_name,unsigned p_name_length,bool & p_found_flag);
	bool process_function(titleformat_text_out * p_out,const char * p_name,unsigned p_name_length,titleformat_hook_function_params * p_params,bool & p_found_flag);
protected:
	void output_text(titleformat_text_out * p_out,const char * p_text,unsigned p_text_length);
	void output_int(titleformat_text_out * p_out,t_uint64 p_val);
	bool process_meta(titleformat_text_out * p_out,const char * p_name,unsigned p_name_length,const char * p_sep1,unsigned p_sep1_length,const char * p_sep2,unsigned p_sep2_length);
	bool process_meta(titleformat_text_out * p_out,unsigned p_index,const char * p_sep1,unsigned p_sep1_length,const char * p_sep2,unsigned p_sep2_length);
	bool process_extra(titleformat_text_out * p_out,const char * p_name,unsigned p_name_length);
	bool remap_meta(unsigned & p_index, const char * p_name, unsigned p_name_length);
	const file_info * m_info;
private:
	void process_codec(titleformat_text_out * p_out);
	const playable_location & m_location;
};

class titleformat_hook_impl_splitter : public titleformat_hook
{
public:
	inline titleformat_hook_impl_splitter(titleformat_hook * p_hook1,titleformat_hook * p_hook2) : m_hook1(p_hook1), m_hook2(p_hook2) {}
	bool process_field(titleformat_text_out * p_out,const char * p_name,unsigned p_name_length,bool & p_found_flag);
	bool process_function(titleformat_text_out * p_out,const char * p_name,unsigned p_name_length,titleformat_hook_function_params * p_params,bool & p_found_flag);
private:
	titleformat_hook * m_hook1, * m_hook2;
};

class titleformat_text_out_impl_filter : public titleformat_text_out
{
public:
	inline titleformat_text_out_impl_filter(titleformat_text_filter * p_filter,titleformat_text_out * p_out) : m_filter(p_filter), m_out(p_out) {}
	void write(const char * p_data,unsigned p_data_length);
private:
	titleformat_text_filter * m_filter;
	titleformat_text_out * m_out;
};

class titleformat_text_filter_impl_reserved_chars : public titleformat_text_filter
{
public:
	titleformat_text_filter_impl_reserved_chars(const char * p_reserved_chars) : m_reserved_chars(p_reserved_chars) {}
	void on_new_field();
	void write(titleformat_text_out * p_out,const char * p_data,unsigned p_data_length);
private:
	const char * m_reserved_chars;
};

class titleformat_hook_impl_text_filter : public titleformat_hook
{
public:
	inline titleformat_hook_impl_text_filter(titleformat_hook * p_chain,titleformat_text_filter * p_filter) : m_chain(p_chain), m_filter(p_filter) {}
	bool process_field(titleformat_text_out * p_out,const char * p_name,unsigned p_name_length,bool & p_found_flag);
	bool process_function(titleformat_text_out * p_out,const char * p_name,unsigned p_name_length,titleformat_hook_function_params * p_params,bool & p_found_flag);
private:
	titleformat_hook * m_chain;
	titleformat_text_filter * m_filter;
};

#endif //_FOOBAR2000_TITLEFORMAT_H_