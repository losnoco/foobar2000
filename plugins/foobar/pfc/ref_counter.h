namespace pfc {

	class NOVTABLE refcounted_object_root
	{
	public:
		void refcount_add_ref() {++m_counter;}
		void refcount_release() {if (--m_counter == 0) delete this;}
	protected:
		refcounted_object_root() {}
		virtual ~refcounted_object_root() {}
	private:
		refcounter m_counter;
	};

	template<class T>
	class refcounted_ptr_t
	{
	public:
		inline refcounted_ptr_t() : m_ptr(0) {}
		inline refcounted_ptr_t(T* p_ptr) : m_ptr(p_ptr) {if (m_ptr) m_ptr->refcount_add_ref();}
		inline refcounted_ptr_t(const refcounted_ptr_t<T> & p_source) : m_ptr(p_source.m_ptr) {if (m_ptr) m_ptr->refcount_add_ref();}

		inline ~refcounted_ptr_t() {if (m_ptr) m_ptr->refcount_release();}
		
		inline void copy(T * p_ptr)
		{
			if (m_ptr) m_ptr->refcount_release();
			m_ptr = p_ptr;
			if (m_ptr) m_ptr->refcount_add_ref();
		}

		inline void copy(const refcounted_ptr_t<T> & p_source) {copy(p_source.m_ptr);}

		inline void set(T * p_ptr)//should not be used ! temporary !
		{
			if (m_ptr) m_ptr->refcount_release();
			m_ptr = p_ptr;
		}

		inline const refcounted_ptr_t<T> & operator=(const refcounted_ptr_t<T> & p_source) {copy(p_source); return *this;}
		inline const refcounted_ptr_t<T> & operator=(T * p_ptr) {copy(p_ptr); return *this;}

		inline void release()
		{
			if (m_ptr) m_ptr->refcount_release();
			m_ptr = 0;
		}


		inline T* operator->() const {assert(m_ptr);return m_ptr;}

		inline T* get_ptr() const {return m_ptr;}
		
		inline T* duplicate_ptr() const//should not be used ! temporary !
		{
			if (m_ptr) m_ptr->refcount_add_ref();
			return m_ptr;
		}

		inline T* duplicate_ptr_release()
		{
			T* ret = m_ptr;
			m_ptr = 0;
			return ret;
		}

		inline bool is_valid() const {return m_ptr != 0;}
		inline bool is_empty() const {return m_ptr == 0;}

		inline bool operator==(const refcounted_ptr_t<T> & p_item) const {return m_ptr == p_item.m_ptr;}
		inline bool operator!=(const refcounted_ptr_t<T> & p_item) const {return m_ptr != p_item.m_ptr;}
		inline bool operator>(const refcounted_ptr_t<T> & p_item) const {return m_ptr > p_item.m_ptr;}
		inline bool operator<(const refcounted_ptr_t<T> & p_item) const {return m_ptr < p_item.m_ptr;}

		inline static void g_swap(refcounted_ptr_t<T> & item1, refcounted_ptr_t<T> & item2)
		{
			pfc::swap_t(item1.m_ptr,item2.m_ptr);
		}

	private:
		T* m_ptr;
	};

	template<typename T>
	inline void swap_t(refcounted_ptr_t<T> & item1, refcounted_ptr_t<T> & item2)
	{
		refcounted_ptr_t<T>::g_swap(item1,item2);
	}


};