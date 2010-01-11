namespace pfc {

	template<typename t_item> class alloc_dummy {
	private: typedef alloc_dummy<t_item> t_self;
	public:
		alloc_dummy() {}
		void set_size(t_size p_size) {throw pfc::exception_not_implemented();}
		t_size get_size() const {throw pfc::exception_not_implemented();}
		const t_item & operator[](t_size p_index) const {throw pfc::exception_not_implemented();}
		t_item & operator[](t_size p_index) {throw pfc::exception_not_implemented();}

		//not mandatory
		const t_item * get_ptr() const {throw pfc::exception_not_implemented();}
		t_item * get_ptr() {throw pfc::exception_not_implemented();}
		void prealloc(t_size) {throw pfc::exception_not_implemented();}
	private:
		const t_self & operator=(const t_self &) {throw pfc::exception_not_implemented();}
		alloc_dummy(const t_self&) {throw pfc::exception_not_implemented();}
	};



	template<typename t_item> class alloc_standard {
	private: typedef alloc_standard<t_item> t_self;
	public:
		alloc_standard() : m_data(NULL), m_size(0) {}

		void set_size(t_size p_size) {
			m_data = pfc::realloc_t(m_data,p_size);
			m_size = p_size;
		}
		
		t_size get_size() const {return m_size;}
		
		const t_item & operator[](t_size p_index) const {return m_data[p_index];}
		t_item & operator[](t_size p_index) {return m_data[p_index];}

		const t_item * get_ptr() const {return m_data;}
		t_item * get_ptr() {return m_data;}

		~alloc_standard() { if (m_data != NULL) free_t(m_data); }
	private:
		alloc_standard(const t_self &) {throw pfc::exception_not_implemented();}
		const t_self & operator=(const t_self&) {throw pfc::exception_not_implemented();}

		t_item * m_data;
		t_size m_size;
	};

	template<typename t_item> class __array_fast_helper_t {
	private:
		typedef __array_fast_helper_t<t_item> t_self;
	public:
		__array_fast_helper_t() : m_buffer(NULL), m_size_total(0), m_size(0) {}
		

		void set_size(t_size p_size,t_size p_size_total) {
			assert(p_size <= p_size_total);
			if (p_size_total == 0) {
				if (m_buffer != NULL) {
					__unsafe__in_place_destructor_array_t(m_buffer,m_size);
					__unsafe__free_t(m_buffer);
					m_buffer = NULL;
				}
				m_size_total = m_size = 0;
			} else if (m_buffer == NULL) {
				t_item * newptr = __unsafe__malloc_t<t_item>(p_size_total);
				try {
					__unsafe__in_place_constructor_array_t(newptr,p_size);
				} catch(...) {
					__unsafe__free_t(newptr);
					throw;
				}
				m_buffer = newptr;
				m_size = p_size;
				m_size_total = p_size_total;
			} else if (p_size_total < m_size_total) {//shrink
				__unsafe__in_place_resize_array_t(m_buffer,m_size,p_size);
				m_size = p_size;
				__unsafe__expand_t(m_buffer,p_size_total);//shrinking, should never fail
				m_size_total = p_size_total;
			} else if (p_size_total > m_size_total) {//expand
				//try in-place first
				if (__unsafe__expand_nothrow_t(m_buffer,p_size_total) != NULL) { //in-place expand successful
					try {//resize actual data
						__unsafe__in_place_resize_array_t(m_buffer,m_size,p_size);
					} catch(...) {//failure, revert to old size
						__unsafe__expand_nothrow_t(m_buffer,m_size_total);//shrinking, should never fail
						throw;
					}
					m_size_total = p_size_total; m_size = p_size;
				} else {//can't do in-place resize, create new block
					t_item * newptr = pfc::__unsafe__malloc_t<t_item>(p_size_total);
					if (pfc::traits_t<t_item>::realloc_safe) {//OK to memcpy it over
						if (m_size < p_size) {//expanding actual data
							try { //new size is bigger than old, call constructors
								__unsafe__in_place_constructor_array_t(newptr+m_size,p_size-m_size);
							} catch(...) {//failure
								__unsafe__free_t(newptr);
								throw;
							}
							__unsafe__memcpy_t(newptr,m_buffer,m_size);//memcpy old content over
						} else {//shrinking actual data
							__unsafe__in_place_destructor_array_t(m_buffer+p_size,m_size-p_size);//destroy items not memcpy'd over
							__unsafe__memcpy_t(newptr,m_buffer,p_size);//memcpy over
						}
					} else {//can't do memcpy, do it the slow way
						try {
							__unsafe__in_place_constructor_array_copy_partial_t(newptr,p_size,m_buffer,m_size);
						} catch(...) {
							__unsafe__free_t(newptr);
							throw;
						}
						__unsafe__in_place_destructor_array_t(m_buffer,m_size);
					}
					__unsafe__free_t(m_buffer);
					m_buffer = newptr;
					m_size = p_size;
					m_size_total = p_size_total;
				}
			} else { //total size unchanged
				__unsafe__in_place_resize_array_t(m_buffer,m_size,p_size);
				m_size = p_size;
			}
		}



		t_size get_size() const {return m_size;}
		t_size get_size_total() const {return m_size_total;}
		const t_item & operator[](t_size p_index) const {assert(p_index < m_size); return m_buffer[p_index];}
		t_item & operator[](t_size p_index) {assert(p_index < m_size); return m_buffer[p_index];}
		~__array_fast_helper_t() {
			set_size(0,0);
		}
		t_item * get_ptr() {return m_buffer;}
		const t_item * get_ptr() const {return m_buffer;}
	private:
		const t_self & operator=(const t_self &) {throw pfc::exception_not_implemented();}
		__array_fast_helper_t(const t_self &) {throw pfc::exception_not_implemented();}

		t_item * m_buffer;
		t_size m_size_total,m_size;
	};

	template<typename t_item> class alloc_fast {
	private: typedef alloc_fast<t_item> t_self;
	public:
		alloc_fast() {}

		void set_size(t_size p_size) {
			t_size size_base = m_data.get_size_total();
			if (size_base == 0) size_base = 1;
			while(size_base < p_size) {
				size_base = safe_shift_left_t<std::bad_alloc,t_size>(size_base,1);
			}
			while(size_base >> 2 > p_size) {
				size_base >>= 1;
			}
			m_data.set_size(p_size,size_base);
		}
		
		t_size get_size() const {return m_data.get_size();}
		const t_item & operator[](t_size p_index) const {return m_data[p_index];}
		t_item & operator[](t_size p_index) {return m_data[p_index];}

		const t_item * get_ptr() const {return m_data.get_ptr();}
		t_item * get_ptr() {return m_data.get_ptr();}
	private:
		alloc_fast(const t_self &) {throw pfc::exception_not_implemented();}
		const t_self & operator=(const t_self&) {throw pfc::exception_not_implemented();}
		__array_fast_helper_t<t_item> m_data;
	};

	template<typename t_item> class alloc_fast_aggressive {
	private: typedef alloc_fast_aggressive<t_item> t_self;
	public:
		alloc_fast_aggressive() {}

		void set_size(t_size p_size) {
			t_size size_base = m_data.get_size_total();
			if (size_base == 0) size_base = 1;
			while(size_base < p_size) {
				size_base = safe_shift_left_t<std::bad_alloc,t_size>(size_base,1);
			}
			m_data.set_size(p_size,size_base);
		}

		void prealloc(t_size p_size) {
			if (p_size > 0) {
				t_size size_base = m_data.get_size_total();
				if (size_base == 0) size_base = 1;
				while(size_base < p_size) {
					size_base = safe_shift_left_t<std::bad_alloc,t_size>(size_base,1);
				}
				m_data.set_size(m_data.get_size(),size_base);
			}
		}
		
		t_size get_size() const {return m_data.get_size();}
		const t_item & operator[](t_size p_index) const {;return m_data[p_index];}
		t_item & operator[](t_size p_index) {return m_data[p_index];}

		const t_item * get_ptr() const {return m_data.get_ptr();}
		t_item * get_ptr() {return m_data.get_ptr();}
	private:
		alloc_fast_aggressive(const t_self &) {throw pfc::exception_not_implemented();}
		const t_self & operator=(const t_self&) {throw pfc::exception_not_implemented();}
		__array_fast_helper_t<t_item> m_data;
	};


	template<t_size p_width> class alloc_fixed {
	public:
		template<typename t_item> class alloc {
		private: typedef alloc<t_item> t_self;
		public:
			alloc() : m_size(0) {}

			void set_size(t_size p_size) {
				static_assert_t<sizeof(m_array) == sizeof(t_item[p_width])>();

				if (p_size > p_width) throw pfc::exception_overflow();
				else if (p_size > m_size) {
					__unsafe__in_place_constructor_array_t(get_ptr()+m_size,p_size-m_size);
					m_size = p_size;
				} else if (p_size < m_size) {
					__unsafe__in_place_destructor_array_t(get_ptr()+p_size,m_size-p_size);
					m_size = p_size;
				}
			}

			~alloc() {
				set_size(0);
			}

			t_size get_size() const {return m_size;}

			t_item * get_ptr() {return reinterpret_cast<t_item*>(&m_array);}
			const t_item * get_ptr() const {return reinterpret_cast<const t_item*>(&m_array);}

			const t_item & operator[](t_size n) const {return get_ptr()[n];}
			t_item & operator[](t_size n) {return get_ptr()[n];}
		private:
			alloc(const t_self&) {throw pfc::exception_not_implemented();}
			const t_self& operator=(const t_self&) {throw pfc::exception_not_implemented();}

			t_uint8 m_array[sizeof(t_item[p_width])];
			t_size m_size;

		};
	};

	template<t_size p_width, template<typename> class t_alloc = alloc_standard > class alloc_hybrid {
	public:
		template<typename t_item> class alloc {
		private: typedef alloc<t_item> t_self;
		public:
			alloc() {}

			void set_size(t_size p_size) {
				if (p_size > p_width) {
					m_fixed.set_size(p_width);
					m_variable.set_size(p_size - p_width);
				} else {
					m_fixed.set_size(p_size);
					m_variable.set_size(0);
				}
			}

			t_item & operator[](t_size p_index) {
				assert(p_index < get_size());
				if (p_index < p_width) return m_fixed[p_index];
				else return m_variable[p_index - p_width];
			}

			const t_item & operator[](t_size p_index) const {
				assert(p_index < get_size());
				if (p_index < p_width) return m_fixed[p_index];
				else return m_variable[p_index - p_width];
			}

			t_size get_size() const {return m_fixed.get_size() + m_variable.get_size();}

		private:
			alloc(const t_self&) {throw pfc::exception_not_implemented();}
			const t_self& operator=(const t_self&) {throw pfc::exception_not_implemented();}

			typename alloc_fixed<p_width>::alloc<t_item> m_fixed;
			t_alloc<t_item> m_variable;
		};
	};
};