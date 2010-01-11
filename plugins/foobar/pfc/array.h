#ifndef _PFC_ARRAY_H_
#define _PFC_ARRAY_H_

template<class T>
class array_t
{
public:
	inline array_t() : m_data(0), m_size(0) {}
	
	inline array_t(unsigned p_size) : m_data(0), m_size(0)
	{
		set_size(p_size);
	}

	inline const T& operator[](unsigned n) const
	{
		assert(n<m_size); assert(m_data);
		return m_data[n];
	}

	inline T& operator[](unsigned n)
	{
		assert(n<m_size); assert(m_data);
		return m_data[n];
	}
	
	bool set_size(unsigned new_size)
	{
		if (new_size == m_size) //do nothing
		{
		}
		else if (new_size == 0)
		{
			if (m_data)
			{
				delete[] m_data;
				m_data = 0;
			}
			m_size = 0;
		}
		else
		{
			T* new_data = new T[new_size];
			if (new_data == 0) return false;
			if (m_data)
			{
				unsigned n,tocopy = new_size > m_size ? m_size : new_size;
				for(n=0;n<tocopy;n++)
					pfc::swap_t(new_data[n],m_data[n]);
				delete[] m_data;
				
			}
			m_data = new_data;
			m_size = new_size;
		}
		return true;
	}


	inline unsigned get_size() const {return m_size;}

	inline void prealloc(unsigned) {}

	inline T* get_ptr() {return m_data;}
	inline const T* get_ptr() const {return m_data;}

	inline void swap(unsigned p_index1,unsigned p_index2)
	{
		pfc::swap_t(m_data[p_index1],m_data[p_index2]);
	}

	~array_t()
	{
		if (m_data) delete[] m_data;
	}

	inline static void g_swap(array_t<T> & item1, array_t<T> & item2)
	{
		pfc::swap_t(item1.m_data,item2.m_data);
		pfc::swap_t(item1.m_size,item2.m_size);
	}

	inline void fill(const T & p_value)
	{
		pfc::fill_ptr_t(m_data,m_size,p_value);
	}
	

	inline array_t(const array_t<T> & p_source) : m_data(0), m_size(0)
	{
		copy(p_source);
	}

	inline const array_t<T> & operator=(const array_t<T> & p_source) {copy(p_source); return *this;}

private:

	void copy(const array_t<T> & p_source)
	{
		unsigned n, m = p_source.get_size();
		set_size(m);
		for(n=0;n<m;n++) m_data[n] = p_source[n];
	}

	T* m_data;
	unsigned m_size;
};

template<class T>
class array_fast_t
{
public:
	inline array_fast_t() : m_size(0) {}

	inline array_fast_t(unsigned p_size) : m_size(0) {set_size(p_size);}

	inline array_fast_t(const array_fast_t<T> & p_source) : m_data(p_source.m_data), m_size(p_source.m_size) {}

	inline const array_fast_t<T> & operator=(const array_fast_t<T> & p_source) {m_data = p_source.m_data; m_size = p_source.m_size; return *this;}

	inline const T& operator[](unsigned n) const
	{
		assert(n<m_size);
		return m_data[n];
	}

	inline T& operator[](unsigned n)
	{
		assert(n<m_size);
		return m_data[n];
	}

	inline unsigned get_size() const {return m_size;}

	bool set_size(unsigned p_size)
	{
		unsigned new_size;
		if (p_size == 0) new_size = 0;
		else
		{
			
			if (p_size >= PFC_MEMORY_SPACE_LIMIT/2) new_size = p_size;
			else
			{
				new_size = m_data.get_size();
				if (new_size == 0) new_size = 1;
				while(new_size < p_size) new_size <<= 1;
				while(new_size>>1 >= p_size) new_size >>= 1;
			}
		}
		if (m_data.set_size(new_size))
		{
			m_size = p_size;
			return true;
		}
		else return false;
	}

	inline void prealloc(unsigned) {}

	inline T* get_ptr() {return m_data.get_ptr();}
	inline const T* get_ptr() const {return m_data.get_ptr();}

	inline void swap(unsigned p_index1,unsigned p_index2)
	{
		pfc::swap_t(m_data[p_index1],m_data[p_index2]);
	}

	inline static void g_swap(array_fast_t<T> & item1, array_fast_t<T> & item2)
	{
		pfc::swap_t(item1.m_data,item2.m_data);
		pfc::swap_t(item1.m_size,item2.m_size);
	}

	inline void fill(const T & p_value)
	{
		pfc::fill_t(m_data,m_size,p_value);
	}

private:
	array_t<T> m_data;
	unsigned m_size;
};


template<class T>
class array_fast_aggressive_t
{
public:
	inline array_fast_aggressive_t() : m_size(0) {}

	inline array_fast_aggressive_t(unsigned p_size) : m_size(0) {set_size(p_size);}

	inline array_fast_aggressive_t(const array_fast_aggressive_t<T> & p_source) : m_data(p_source.m_data), m_size(p_source.m_size) {}

	const array_fast_aggressive_t<T> & operator=(const array_fast_aggressive_t<T> & p_source) {m_data = p_source.m_data; m_size = p_source.m_size; return *this;}


	inline const T& operator[](unsigned n) const
	{
		assert(n<m_size);
		return m_data[n];
	}

	inline T& operator[](unsigned n)
	{
		assert(n<m_size);
		return m_data[n];
	}

	inline unsigned get_size() const {return m_size;}

	bool set_size(unsigned p_size)
	{
		unsigned new_size;
		if (p_size == 0) new_size = 0;
		else
		{
			new_size = m_data.get_size();
			if (new_size == 0) new_size = 1;
			while(new_size < p_size) new_size <<= 1;
		}
		if (m_data.set_size(new_size))
		{
			m_size = p_size;
			return true;
		}
		else return false;
	}

	inline void prealloc(unsigned) {}

	inline T* get_ptr() {return m_data.get_ptr();}
	inline const T* get_ptr() const {return m_data.get_ptr();}


	inline void swap(unsigned p_index1,unsigned p_index2)
	{
		pfc::swap_t(m_data[p_index1],m_data[p_index2]);
	}

	inline static void g_swap(array_fast_aggressive_t<T> & item1, array_fast_aggressive_t<T> & item2)
	{
		pfc::swap_t(item1.m_data,item2.m_data);
		pfc::swap_t(item1.m_size,item2.m_size);
	}

	inline void fill(const T & p_value)
	{
		pfc::fill_t(m_data,m_size,p_value);
	}

private:

	array_t<T> m_data;
	unsigned m_size;
};


template<typename T,unsigned t_fixed_count,typename t_variable_storage = array_t<T> >
class array_hybrid_t
{
public:
	inline array_hybrid_t() : m_size(0) {}
	inline array_hybrid_t(unsigned p_size) : m_size(0) {set_size(p_size);}
	inline array_hybrid_t(const array_hybrid_t<T,t_fixed_count,t_variable_storage> & p_source)
		: m_fixed(p_source.m_fixed), m_variable(p_source.m_variable), m_size(p_source.m_size)
	{
	}

	inline const array_hybrid_t<T,t_fixed_count,t_variable_storage> & operator=(const array_hybrid_t<T,t_fixed_count,t_variable_storage> & p_source)
	{
		unsigned n;
		for(n=0;n<t_fixed_count;n++) m_fixed[n] = p_source.m_fixed[n];
		m_variable = p_source.m_variable;
		m_size = p_source.m_size;
		return *this;
	}

	inline bool set_size(unsigned p_size)
	{
		m_size = p_size;
		return m_variable.set_size(p_size > t_fixed_count ? p_size - t_fixed_count : 0);
	}

	inline T& operator[](unsigned p_index)
	{
		assert(p_index < m_size);
		if (p_index < t_fixed_count) return m_fixed[p_index];
		else return m_variable[p_index - t_fixed_count];
	}

	inline const T& operator[](unsigned p_index) const
	{
		assert(p_index < m_size);
		if (p_index < t_fixed_count) return m_fixed[p_index];
		else return m_variable[p_index - t_fixed_count];
	}

	inline unsigned get_size() const {return m_size;}

	static void g_swap(array_hybrid_t<T,t_fixed_count,t_variable_storage> & p_item1,array_hybrid_t<T,t_fixed_count,t_variable_storage> & p_item2)
	{
		unsigned n;
		for(n=0;n<t_fixed_count;n++) pfc::swap_t(p_item1.m_fixed[n],p_item2.m_fixed[n]);

		pfc::swap_t(p_item1.m_variable,p_item2.m_variable);
		pfc::swap_t(p_item1.m_size,p_item2.m_size);
	}

private:
	T m_fixed[t_fixed_count];
	t_variable_storage m_variable;
	unsigned m_size;
};

namespace pfc {

	template<typename T>
	inline void swap_t(array_t<T> & item1, array_t<T> & item2)
	{
		array_t<T>::g_swap(item1,item2);
	}

	template<typename T>
	inline void swap_t(array_fast_t<T> & item1, array_fast_t<T> & item2)
	{
		array_fast_t<T>::g_swap(item1,item2);
	}

	template<typename T>
	inline void swap_t(array_fast_aggressive_t<T> & item1, array_fast_aggressive_t<T> & item2)
	{
		array_fast_aggressive_t<T>::g_swap(item1,item2);
	}

	template<typename T,unsigned t_fixed_count,typename t_variable_storage>
	inline void swap_t(array_hybrid_t<T,t_fixed_count,t_variable_storage> & p_item1,array_hybrid_t<T,t_fixed_count,t_variable_storage> & p_item2)
	{
		array_hybrid_t<T,t_fixed_count,t_variable_storage>::g_swap(p_item1,p_item2);
	}
};


#endif //_PFC_ARRAY_H_