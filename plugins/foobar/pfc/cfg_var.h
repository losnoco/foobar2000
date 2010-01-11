#ifndef _PFC_CFG_VAR_H_
#define _PFC_CFG_VAR_H_

//IMPORTANT:
//cfg_var objects are intended ONLY to be created statically !!!!


class NOVTABLE cfg_var
{
private:
	GUID m_guid;
	static cfg_var * list;
	cfg_var * next;

	cfg_var(const cfg_var& ) {assert(0);}
	void operator=(const cfg_var& ) {assert(0);}

public:
	class NOVTABLE write_config_callback
	{
	public:
		inline void write_byte(BYTE param) {write(&param,1);}
		void write_dword_le(t_uint32 param);
		void write_dword_be(t_uint32 param);
		void write_guid_le(const GUID& param);
		void write_string(const char * param);
		virtual void write(const void * ptr,unsigned bytes)=0;
	};

	class write_config_callback_i : public write_config_callback
	{
	public:
		mem_block data;
		write_config_callback_i() {data.set_mem_logic(mem_block::ALLOC_FAST_DONTGODOWN);}
		virtual void write(const void * ptr,unsigned bytes) {data.append(ptr,bytes);}
	};

	class write_config_callback_i_ref : public write_config_callback
	{
	public:
		mem_block & data;
		write_config_callback_i_ref(mem_block & out) : data(out) {}
		virtual void write(const void * ptr,unsigned bytes) {data.append(ptr,bytes);}
	};
	

	class read_config_helper
	{
		const unsigned char * ptr;
		unsigned remaining;
	public:
		read_config_helper(const void * p_ptr,unsigned bytes) : ptr((const unsigned char*)p_ptr), remaining(bytes) {}
		bool read(void * out,unsigned bytes);
		bool read_dword_le(t_uint32 & val);
		bool read_dword_be(t_uint32 & val);
		inline bool read_byte(BYTE & val) {return read(&val,1)==1;}
		bool read_guid_le(GUID & val);
		bool read_string(string_base & out);
		inline unsigned get_remaining() const {return remaining;}
		inline const void * get_ptr() const {return ptr;}
		inline void advance(unsigned delta) {assert(delta<=remaining);ptr+=delta;remaining-=delta;}
	};

protected:
	explicit inline cfg_var(const GUID & p_guid) : m_guid(p_guid) {next=list;list=this;};
	
	//override me
	virtual bool get_raw_data(write_config_callback * out)=0;//return false if values are default and dont need to be stored
	virtual void set_raw_data(const void * data,int size)=0;

public:

	inline const GUID & get_guid() const {return m_guid;}

	static void config_read_file(const void * src,unsigned size);
	static void config_write_file(write_config_callback * out);
};

class cfg_int : public cfg_var
{
private:
	int m_val;
	virtual bool get_raw_data(write_config_callback * out);
	virtual void set_raw_data(const void * data,int size);
public:
	explicit inline cfg_int(const GUID & p_guid,int p_val) : cfg_var(p_guid) {m_val=p_val;}
	
	inline const cfg_int & operator=(const cfg_int & p_val) {m_val=p_val.m_val;return *this;}
	inline int operator=(int v) {m_val=v;return m_val;}

	inline operator int() const {return m_val;}
	
};

class cfg_string : public cfg_var
{
private:
	string_simple m_val;
	
	virtual bool get_raw_data(write_config_callback * out)
	{
		out->write((const char*)m_val,strlen(m_val) * sizeof(char));
		return true;
	}

	virtual void set_raw_data(const void * data,int size)
	{
		m_val.set_string((const char*)data,size/sizeof(char));
	}

public:
	explicit inline cfg_string(const GUID & p_guid,const char * p_val) : cfg_var(p_guid), m_val(p_val) {}

	inline const cfg_string& operator=(const cfg_string & p_val) {m_val=p_val.get_ptr();return *this;}
	inline const cfg_string& operator=(const char* p_val) {m_val=p_val;return *this;}

	inline operator const char * () const {return m_val;}
	
	inline const char * get_ptr() const {return m_val;}
	inline bool is_empty() {return !m_val[0];}
};

class cfg_guid : public cfg_var
{
	GUID m_val;

	virtual bool get_raw_data(write_config_callback * out)
	{
		out->write_guid_le(m_val);
		return true;
	}

	virtual void set_raw_data(const void * data,int size)
	{
		read_config_helper r(data,size);
		r.read_guid_le(m_val);
	}
public:
	explicit inline cfg_guid(const GUID & p_guid,const GUID & p_val = pfc::guid_null) : cfg_var(p_guid) {m_val=p_val;}
	inline const cfg_guid & operator=(const GUID & p_val) {m_val = p_val;return *this;}
	inline const cfg_guid & operator=(const cfg_guid & p_val) {m_val = p_val.m_val;return *this;}
	inline operator GUID() const {return m_val;}
	inline const GUID & get_value() const {return m_val;}
};

template<class T>
class cfg_struct_t : public cfg_var
{
private:
	T m_val;


	virtual bool get_raw_data(write_config_callback * out)
	{
		out->write(&m_val,sizeof(m_val));
		return true;
	}

	virtual void set_raw_data(const void * data,int size)
	{
		if (size==sizeof(T))
		{
			memcpy(&m_val,data,sizeof(T));
		}
	}
public:
	explicit inline cfg_struct_t(const GUID & p_guid,T p_val) : cfg_var(p_guid), m_val(p_val) {}
	explicit inline cfg_struct_t(const GUID & p_guid,int filler) : cfg_var(p_guid) {memset(&m_val,filler,sizeof(T));}
	
	inline const cfg_struct_t<T> & operator=(const cfg_struct_t<T> & p_val) {m_val = p_val.get_value();return *this;}
	inline const cfg_struct_t<T> & operator=(const T & p_val) {m_val = p_val;return *this;}

	inline const T& get_value() const {return m_val;}
	inline operator T() const {return m_val;}
};


#endif
