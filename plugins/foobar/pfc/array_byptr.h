#if 0

template<typename T,typename t_storage = mem_block_t<T*> >
class array_byptr_t
{
public:
	inline unsigned get_size() const {return m_storage.get_size();}
	array_byptr_t() {}
	array_byptr_t(const array_byptr_t<T,t_storage> & p_source) {*this = p_source;}
	
	const array_byptr_t<T,t_storage> & operator=(const array_byptr_t<T,t_storage> & p_source)
	{
		for(unsigned n=0;n<m_storage.get_size();n++)
		{
			delete m_storage[n];
			m_storage[n] = 0;
		}
		if (m_storage.set_size(p_source.get_size()))
		{
			for(unsigned n=0;n<m_storage.get_size();n++)
			{
				m_storage[n] = new T(p_source[n]);
			}
		}
		return *this;
	}

	bool set_size(unsigned p_size)
	{
		unsigned old_size = m_storage.get_size();
		if (old_size == p_size) return true;

		for(unsigned n=p_size;n<old_size;n++)
			delete m_storage[n];

		if (!m_storage.set_size(p_size))
		{
			for(unsigned n=p_size;n<old_size;n++)
				m_storage[n] = new T;
			return false;
		}

		for(unsigned n=old_size;n<p_size;n++)
			m_storage[n] = new T;
		
		return true;
	}

	~array_byptr_t()
	{
		for(unsigned n=0;n<m_storage.get_size();n++)
		{
			delete m_storage[n];
		}
	}

	const T& operator[](unsigned n) const {return *m_storage[n];}
	T& operator[](unsigned n) {return *m_storage[n];}

	inline static void g_swap(array_byptr_t<T,t_storage> & item1,array_byptr_t<T,t_storage> & item2)
	{
		pfc::swap_t(item1.m_storage,item2.m_storage);
	}

private:
	t_storage m_storage;
};

namespace pfc 
{
	template<typename T,typename t_storage>
	inline void swap_t(array_byptr_t<T,t_storage> & item1,array_byptr_t<T,t_storage> & item2)
	{
		array_byptr_t<T,t_storage>::g_swap(item1,item2);
	}
};

#endif