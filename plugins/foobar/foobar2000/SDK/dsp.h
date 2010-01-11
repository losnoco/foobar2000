#ifndef _DSP_H_
#define _DSP_H_

#include "service.h"
#include "audio_chunk.h"

class NOVTABLE dsp_chunk_list//interface (cross-dll safe)
{
public:
	virtual t_size get_count() const = 0;
	virtual audio_chunk * get_item(t_size n) const = 0;
	virtual void remove_by_idx(t_size idx) = 0;
	virtual void remove_mask(const bit_array & mask) = 0;
	virtual audio_chunk * insert_item(t_size idx,t_size hint_size=0) = 0;

	audio_chunk * add_item(t_size hint_size=0) {return insert_item(get_count(),hint_size);}

	void remove_all() {remove_mask(bit_array_true());}

	double get_duration()
	{
		double rv = 0;
		t_size n,m = get_count();
		for(n=0;n<m;n++) rv += get_item(n)->get_duration();
		return rv;
	}

	void add_chunk(const audio_chunk * chunk)
	{
		audio_chunk * dst = insert_item(get_count(),chunk->get_data_length());
		if (dst) dst->copy_from(chunk);
	}

	void remove_bad_chunks();
protected:
	dsp_chunk_list() {}
	~dsp_chunk_list() {}
};

class dsp_chunk_list_i : public dsp_chunk_list//implementation
{
	ptr_list_t<audio_chunk_i> data,recycled;
public:
	virtual t_size get_count() const;
	virtual audio_chunk * get_item(t_size n) const;
	virtual void remove_by_idx(t_size idx);
	virtual void remove_mask(const bit_array & mask);
	virtual audio_chunk * insert_item(t_size idx,t_size hint_size=0);
	~dsp_chunk_list_i();
};

class NOVTABLE dsp : public service_base
{
public:
	enum
	{
		END_OF_TRACK = 1,	//flush whatever you need to when tracks change
		FLUSH = 2	//flush everything
	};

	virtual void run(dsp_chunk_list * list,const metadb_handle_ptr & cur_file,int flags)=0;//int flags <= see flags above
	//cur_file is OPTIONAL and may be null
	virtual void flush() = 0;//after seek etc
	virtual double get_latency() = 0;//amount of data buffered (in seconds)
	virtual bool need_track_change_mark() = 0;//return true if you need to know exact track change point (eg. for crossfading, removing silence), will force-flush any DSPs placed before you so when you get END_OF_TRACK, chunks you get contain last samples of the track; will often break regular gapless playback so don't use it unless you have reasons to


	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	dsp() {}
	~dsp() {}
};

template<class T>
class dsp_impl_base_t : public T
{
private:
	dsp_chunk_list * list;
	t_size chunk_ptr;
	metadb_handle_ptr cur_file;
	virtual void run(dsp_chunk_list * p_list,const metadb_handle_ptr & p_cur_file,int flags);
protected:
	inline bool get_cur_file(metadb_handle_ptr & p_out) {p_out = cur_file; return p_out.is_valid();}// call only from on_chunk / on_endoftrack (on_endoftrack will give info on track being finished); may return null !!
	dsp_impl_base_t() {list = 0;cur_file=0;chunk_ptr=0;}
	audio_chunk * insert_chunk(t_size hint_size = 0)	//call only from on_endoftrack / on_endofplayback / on_chunk
	{//hint_size - optional, amout of buffer space you want to use
		return list ? list->insert_item(chunk_ptr++,hint_size) : 0;
	}
	//override these
	virtual void on_endoftrack()//use insert_chunk() if you have data you want to dump
	{
	}
	virtual void on_endofplayback()
	{//use insert_chunk() if you have data you want to dump

	}
	virtual bool on_chunk(audio_chunk * chunk)
	{//return true if your chunk, possibly modified needs to be put back into chain, false if you want it removed
		//use insert_chunk() if you want to insert pending data before current chunk
		return true;
	}

public:
	
	virtual void flush() = 0;//after seek etc
	virtual double get_latency() = 0;//amount of data buffered (in seconds)
	virtual bool need_track_change_mark() = 0;//return true if you need to know exact track change point (eg. for crossfading, removing silence), will force-flush any DSPs placed before you so when you get END_OF_TRACK, chunks you get contain last samples of the track; will often break regular gapless playback so don't use it unless you have reasons to

};

template<class T>
void dsp_impl_base_t<T>::run(dsp_chunk_list * p_list,const metadb_handle_ptr & p_cur_file,int flags)
{
	list = p_list;
	cur_file = p_cur_file;
	for(chunk_ptr = 0;chunk_ptr<list->get_count();chunk_ptr++)
	{
		audio_chunk * c = list->get_item(chunk_ptr);
		if (c->is_empty() || !on_chunk(c))
			list->remove_by_idx(chunk_ptr--);
	}
	if (flags & FLUSH)
		on_endofplayback();
	else if (flags & END_OF_TRACK)
		on_endoftrack();

	list = 0;
	cur_file = 0;
}


typedef dsp_impl_base_t<dsp> dsp_impl_base;
typedef dsp_impl_base dsp_i_base;//for old code

class NOVTABLE dsp_preset {
public:
	virtual GUID get_owner() const = 0;
	virtual void set_owner(const GUID & p_owner) = 0;
	virtual const void * get_data() const = 0;
	virtual t_size get_data_size() const = 0;
	virtual void set_data(const void * p_data,t_size p_data_size) = 0;
	virtual void set_data_from_stream(stream_reader * p_stream,t_size p_bytes,abort_callback & p_abort) = 0;

	const dsp_preset & operator=(const dsp_preset & p_source) {copy(p_source); return *this;}

	void copy(const dsp_preset & p_source) {set_owner(p_source.get_owner());set_data(p_source.get_data(),p_source.get_data_size());}

	void contents_to_stream(stream_writer * p_stream,abort_callback & p_abort) const;
	void contents_from_stream(stream_reader * p_stream,abort_callback & p_abort);
	static void g_contents_from_stream_skip(stream_reader * p_stream,abort_callback & p_abort);

protected:
	dsp_preset() {}
	~dsp_preset() {}
};

class dsp_preset_impl : public dsp_preset
{
public:
	dsp_preset_impl() {}
	dsp_preset_impl(const dsp_preset_impl & p_source) {copy(p_source);}
	dsp_preset_impl(const dsp_preset & p_source) {copy(p_source);}

	const dsp_preset_impl& operator=(const dsp_preset_impl & p_source) {copy(p_source); return *this;}
	const dsp_preset_impl& operator=(const dsp_preset & p_source) {copy(p_source); return *this;}

	GUID get_owner() const {return m_owner;}
	void set_owner(const GUID & p_owner) {m_owner = p_owner;}
	const void * get_data() const {return m_data.get_ptr();}
	t_size get_data_size() const {return m_data.get_size();}
	void set_data(const void * p_data,t_size p_data_size) {m_data.set_data_fromptr((const t_uint8*)p_data,p_data_size);}
	void set_data_from_stream(stream_reader * p_stream,t_size p_bytes,abort_callback & p_abort);
private:
	GUID m_owner;
	pfc::array_t<t_uint8> m_data;
};

class NOVTABLE dsp_entry : public service_base
{
public:
	virtual void get_name(pfc::string_base & p_out) = 0;
	virtual bool get_default_preset(dsp_preset & p_out) = 0;
	virtual bool instantiate(service_ptr_t<dsp> & p_out,const dsp_preset & p_preset) = 0;	
	virtual GUID get_guid() = 0;
	virtual bool have_config_popup() = 0;
	virtual bool show_config_popup(dsp_preset & p_data,HWND p_parent) = 0;


	static bool g_get_interface(service_ptr_t<dsp_entry> & p_out,const GUID & p_guid);
	static bool g_instantiate(service_ptr_t<dsp> & p_out,const dsp_preset & p_preset);
	static bool g_instantiate_default(service_ptr_t<dsp> & p_out,const GUID & p_guid);
	static bool g_name_from_guid(pfc::string_base & p_out,const GUID & p_guid);
	static bool g_dsp_exists(const GUID & p_guid);
	static bool g_get_default_preset(dsp_preset & p_out,const GUID & p_guid);
	static bool g_have_config_popup(const GUID & p_guid);
	static bool g_have_config_popup(const dsp_preset & p_preset);
	static bool g_show_config_popup(dsp_preset & p_preset,HWND p_parent);

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	dsp_entry() {}
	~dsp_entry() {}
};

template<class T,class T_entry = dsp_entry>
class dsp_entry_impl_nopreset_t : public T_entry
{
public:
	void get_name(pfc::string_base & p_out) {T::g_get_name(p_out);}
	bool get_default_preset(dsp_preset & p_out)
	{
		p_out.set_owner(T::g_get_guid());
		p_out.set_data(0,0);
		return true;
	}
	bool instantiate(service_ptr_t<dsp> & p_out,const dsp_preset & p_preset)
	{
		if (p_preset.get_owner() == T::g_get_guid() && p_preset.get_data_size() == 0)
		{
			p_out = new service_impl_t<T>();
			return p_out.is_valid();
		}
		else return false;
	}
	GUID get_guid() {return T::g_get_guid();}

	bool have_config_popup() {return false;}
	bool show_config_popup(dsp_preset & p_data,HWND p_parent) {return false;}
};

template<class T, class T_entry = dsp_entry>
class dsp_entry_impl_t : public T_entry
{
public:
	void get_name(pfc::string_base & p_out) {T::g_get_name(p_out);}
	bool get_default_preset(dsp_preset & p_out) {return T::g_get_default_preset(p_out);}
	bool instantiate(service_ptr_t<dsp> & p_out,const dsp_preset & p_preset)
	{
		if (p_preset.get_owner() == T::g_get_guid())
		{
			service_ptr_t<T> temp;
			temp = new service_impl_t<T>();
			if (!temp->set_data(p_preset)) return false;
			p_out = temp.get_ptr();
			return true;
		}
		else return false;
	}
	GUID get_guid() {return T::g_get_guid();}

	bool have_config_popup() {return T::g_have_config_popup();}
	bool show_config_popup(dsp_preset & p_data,HWND p_parent) {return T::g_show_config_popup(p_data,p_parent);}
};

template<class T>
class dsp_factory_nopreset_t : public service_factory_single_t<dsp_entry,dsp_entry_impl_nopreset_t<T> > {};

template<class T>
class dsp_factory_t : public service_factory_single_t<dsp_entry,dsp_entry_impl_t<T> > {};

class NOVTABLE dsp_chain_config
{
public:
	virtual t_size get_count() const = 0;
	virtual const dsp_preset & get_item(t_size p_index) const = 0;
	virtual void replace_item(const dsp_preset & p_data,t_size p_index) = 0;
	virtual void insert_item(const dsp_preset & p_data,t_size p_index) = 0;
	virtual void remove_mask(const bit_array & p_mask) = 0;
	
	void remove_item(t_size p_index);
	void remove_all();
	void add_item(const dsp_preset & p_data);
	void copy(const dsp_chain_config & p_source);

	const dsp_chain_config & operator=(const dsp_chain_config & p_source) {copy(p_source); return *this;}

	void contents_to_stream(stream_writer * p_stream,abort_callback & p_abort) const;
	void contents_from_stream(stream_reader * p_stream,abort_callback & p_abort);

	void instantiate(service_list_t<dsp> & p_out);

	void get_name_list(pfc::string_base & p_out) const;
};

class dsp_chain_config_impl : public dsp_chain_config
{
public:
	dsp_chain_config_impl() {}
	dsp_chain_config_impl(const dsp_chain_config & p_source) {copy(p_source);}
	dsp_chain_config_impl(const dsp_chain_config_impl & p_source) {copy(p_source);}
	t_size get_count() const;
	const dsp_preset & get_item(t_size p_index) const;
	void replace_item(const dsp_preset & p_data,t_size p_index);
	void insert_item(const dsp_preset & p_data,t_size p_index);
	void remove_mask(const bit_array & p_mask);

	const dsp_chain_config_impl & operator=(const dsp_chain_config & p_source) {copy(p_source); return *this;}
	const dsp_chain_config_impl & operator=(const dsp_chain_config_impl & p_source) {copy(p_source); return *this;}

	~dsp_chain_config_impl();
private:
	ptr_list_t<dsp_preset_impl> m_data;
};

class cfg_dsp_chain_config : public cfg_var
{
protected:
	void get_data_raw(stream_writer * p_stream,abort_callback & p_abort);
	void set_data_raw(stream_reader * p_stream,t_size p_sizehint,abort_callback & p_abort);
public:
	void reset();
	inline cfg_dsp_chain_config(const GUID & p_guid) : cfg_var(p_guid) {}
	t_size get_count() const {return m_data.get_count();}
	const dsp_preset & get_item(t_size p_index) const {return m_data.get_item(p_index);}
	bool get_data(dsp_chain_config & p_data) const;
	void set_data(const dsp_chain_config & p_data);
private:
	dsp_chain_config_impl m_data;
	
};


#include "dsp_manager.h"

#endif