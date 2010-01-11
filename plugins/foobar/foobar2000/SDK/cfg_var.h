#ifndef _FOOBAR2000_SDK_CFG_VAR_H_
#define _FOOBAR2000_SDK_CFG_VAR_H_
//IMPORTANT:
//cfg_var objects are intended ONLY to be created statically !!!!


class NOVTABLE cfg_var
{
private:
	GUID m_guid;
	static cfg_var * list;
	cfg_var * next;

	cfg_var(const cfg_var& ) {throw pfc::exception_not_implemented();}
	const cfg_var & operator=(const cfg_var& ) {throw pfc::exception_not_implemented();}

protected:
	explicit inline cfg_var(const GUID & p_guid) : m_guid(p_guid) {next=list;list=this;};
	~cfg_var() {}
public:
	
	virtual void get_data_raw(stream_writer * p_stream,abort_callback & p_abort) = 0;
	virtual void set_data_raw(stream_reader * p_stream,t_size p_sizehint,abort_callback & p_abort) = 0;

	inline const GUID & get_guid() const {return m_guid;}

	static void config_read_file(stream_reader * p_stream,abort_callback & p_abort);
	static void config_write_file(stream_writer * p_stream,abort_callback & p_abort);

};

template<typename t_inttype>
class cfg_int_t : public cfg_var
{
private:
	t_inttype m_val;
protected:
	void get_data_raw(stream_writer * p_stream,abort_callback & p_abort) {p_stream->write_lendian_t(m_val,p_abort);}
	void set_data_raw(stream_reader * p_stream,t_size p_sizehint,abort_callback & p_abort) {
		t_inttype temp;
		p_stream->read_lendian_t(temp,p_abort);
		m_val = temp;
	}

public:
	explicit inline cfg_int_t(const GUID & p_guid,t_inttype p_val) : cfg_var(p_guid), m_val(p_val) {}
	
	inline const cfg_int_t<t_inttype> & operator=(const cfg_int_t<t_inttype> & p_val) {m_val=p_val.m_val;return *this;}
	inline t_inttype operator=(t_inttype p_val) {m_val=p_val;return m_val;}

	inline operator t_inttype() const {return m_val;}

	inline t_inttype get_value() const {return m_val;}
};

typedef cfg_int_t<t_int32> cfg_int;
typedef cfg_int_t<t_uint32> cfg_uint;
typedef cfg_int_t<GUID> cfg_guid;
typedef cfg_int_t<bool> cfg_bool;

class cfg_string : public cfg_var, public pfc::string8
{
protected:
	void get_data_raw(stream_writer * p_stream,abort_callback & p_abort);
	void set_data_raw(stream_reader * p_stream,t_size p_sizehint,abort_callback & p_abort);

public:
	explicit inline cfg_string(const GUID & p_guid,const char * p_val) : cfg_var(p_guid), pfc::string8(p_val) {}

	inline const cfg_string& operator=(const cfg_string & p_val) {set_string(p_val);return *this;}
	inline const cfg_string& operator=(const char* p_val) {set_string(p_val);return *this;}

	inline operator const char * () const {return get_ptr();}

};

template<typename t_struct>
class cfg_struct_t : public cfg_var
{
private:
	t_struct m_val;

protected:

	void get_data_raw(stream_writer * p_stream,abort_callback & p_abort) {p_stream->write_object(&m_val,sizeof(m_val),p_abort);}
	void set_data_raw(stream_reader * p_stream,t_size p_sizehint,abort_callback & p_abort) {
		t_struct temp;
		p_stream->read_object(&temp,sizeof(temp),p_abort);
		m_val = temp;
	}
public:
	inline cfg_struct_t(const GUID & p_guid,const t_struct & p_val) : cfg_var(p_guid), m_val(p_val) {}
	inline cfg_struct_t(const GUID & p_guid,int filler) : cfg_var(p_guid) {memset(&m_val,filler,sizeof(t_struct));}
	
	inline const cfg_struct_t<t_struct> & operator=(const cfg_struct_t<t_struct> & p_val) {m_val = p_val.get_value();return *this;}
	inline const cfg_struct_t<t_struct> & operator=(const t_struct & p_val) {m_val = p_val;return *this;}

	inline const t_struct& get_value() const {return m_val;}
	inline t_struct& get_value() {return m_val;}
	inline operator t_struct() const {return m_val;}
};


#endif
