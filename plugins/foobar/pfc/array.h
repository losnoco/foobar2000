#ifndef _PFC_ARRAY_H_
#define _PFC_ARRAY_H_

namespace pfc {
	template<typename t_exception,typename t_int>
	inline t_int safe_shift_left_t(t_int p_val,t_size p_shift = 1) {
		t_int newval = p_val << p_shift;
		if (newval >> p_shift != p_val) throw t_exception();
		return newval;
	}

	template<typename t_item, template<typename> class t_alloc = alloc_standard> class array_t;

	template<typename t_to,typename t_from>
	inline void copy_array_t(t_to & p_to,const t_from & p_from) {
		const t_size size = array_size_t(p_from);
		p_to.set_size(size);
		for(t_size n=0;n<size;n++) p_to[n] = p_from[n];
	}

	template<typename t_array,typename t_value>
	inline void fill_array_t(t_array & p_array,const t_value & p_value) {
		const t_size size = array_size_t(p_array);
		for(t_size n=0;n<size;n++) p_array[n] = p_value;
	}

	template<typename t_item, template<typename> class t_alloc> class array_t {
	private: typedef array_t<t_item,t_alloc> t_self;
	public:
		array_t() {}
		array_t(const t_self & p_source) {copy_array_t(*this,p_source);}
		template<typename t_source> array_t(const t_source & p_source) {copy_array_t(*this,p_source);}
		const t_self & operator=(const t_self & p_source) {copy_array_t(*this,p_source); return *this;}
		template<typename t_source> const t_self & operator=(const t_source & p_source) {copy_array_t(*this,p_source); return *this;}
		
		void set_size(t_size p_size) {m_alloc.set_size(p_size);}
		t_size get_size() const {return m_alloc.get_size();}
		
		const t_item & operator[](t_size p_index) const {PFC_ASSERT(p_index < get_size());return m_alloc[p_index];}
		t_item & operator[](t_size p_index) {PFC_ASSERT(p_index < get_size());return m_alloc[p_index];}

		template<typename t_source>
		void set_data_fromptr(const t_source * p_buffer,t_size p_count) {
			set_size(p_count);
			pfc::memcpy_t(*this,p_buffer,p_count);
//			for(t_size n=0;n<p_count;n++) m_alloc[n] = p_buffer[n];
		}

		template<typename t_array>
		void append(const t_array & p_source) {
			t_size base = get_size();
			const t_size source_size = array_size_t(p_source);
			increase_size(source_size);
			for(t_size n=0;n<source_size;n++) m_alloc[base+n] = p_source[n];
		}

		template<typename t_append>
		void append_fromptr(const t_append * p_buffer,t_size p_count) {
			t_size base = get_size();
			increase_size(p_count);
			for(t_size n=0;n<p_count;n++) m_alloc[base+n] = p_buffer[n];
		}

		void increase_size(t_size p_delta) {
			t_size new_size = get_size() + p_delta;
			if (new_size < p_delta) throw std::bad_alloc();
			set_size(new_size);
		}

		template<typename t_append>
		void append_single(const t_append & p_item) {
			t_size base = get_size();
			increase_size(1);
			m_alloc[base] = p_item;
		}

		template<typename t_filler>
		void fill(const t_filler & p_filler) {
			const t_size max = get_size();
			for(t_size n=0;n<max;n++) m_alloc[n] = p_filler;
		}

		void grow_size(t_size p_size) {
			if (p_size > get_size()) set_size(p_size);
		}

		//not supported by some allocs
		const t_item * get_ptr() const {return m_alloc.get_ptr();}
		t_item * get_ptr() {return m_alloc.get_ptr();}

		void prealloc(t_size p_size) {m_alloc.prealloc(p_size);}

	private:
		t_alloc<t_item> m_alloc;
	};

	template<typename t_item,t_size p_width,template<typename> class t_alloc = alloc_standard >
	class array_hybrid_t : public array_t<t_item, pfc::alloc_hybrid<p_width,t_alloc>::alloc >
	{};
}


#endif //_PFC_ARRAY_H_