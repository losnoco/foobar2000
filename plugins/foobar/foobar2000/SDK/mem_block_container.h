class NOVTABLE mem_block_container
{
public:
	virtual const void * get_ptr() const = 0;
	virtual void * get_ptr() = 0;
	virtual unsigned get_size() const = 0;
	virtual bool set_size(unsigned p_size) = 0;
	
	t_io_result from_stream(stream_reader * p_stream,unsigned p_bytes,abort_callback & p_abort);

	bool set(const void * p_buffer,unsigned p_size);

	inline bool copy(const mem_block_container & p_source) {return set(p_source.get_ptr(),p_source.get_size());}
	inline void reset() {set_size(0);}

	const mem_block_container & operator=(const mem_block_container & p_source) {copy(p_source);return *this;}
};

template<class t_container>
class mem_block_container_impl_t : public mem_block_container
{
public:
	const void * get_ptr() const {return m_data.get_ptr();}
	void * get_ptr() {return m_data.get_ptr();}
	unsigned get_size() const {return m_data.get_size();}
	bool set_size(unsigned p_size)
	{
		if (p_size == 0) {m_data.set_size(0); return true;}
		else {return m_data.set_size(p_size) != 0;}
	}
private:
	t_container m_data;
};

typedef mem_block_container_impl_t<mem_block> mem_block_container_impl;