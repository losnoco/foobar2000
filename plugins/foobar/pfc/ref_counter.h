namespace pfc {

	class NOVTABLE refcounted_object_root
	{
	public:
		inline void refcount_add_ref() {InterlockedIncrement(&m_counter);}
		inline void refcount_release() {if (InterlockedDecrement(&m_counter) == 0) delete this;}
	protected:
		inline refcounted_object_root() : m_counter(0) {}
		virtual ~refcounted_object_root() {}
	private:
		long m_counter;
	};

	template<typename T>
	class refcounted_container_t
	{
	public:
		inline void refcount_add_ref() {InterlockedIncrement(&m_counter);}
		inline void refcount_release() {if (InterlockedDecrement(&m_counter) == 0) delete this;}

		inline T * get_ptr() {return &m_object;}
		inline const T * get_ptr() const {return &m_object;}


		refcounted_container_t() : m_counter(0) {}
		
		refcounted_container_t(const T& p_object) : m_counter(0), m_object(p_object) {}
	private:
		refcounted_container_t(const refcounted_container_t & ) {assert(0);}
		~refcounted_container_t() {}
		T m_object;
		long m_counter;
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
	class autoreference_t
	{
	private:
		typedef refcounted_container_t<T> t_ref;
	public:
		inline autoreference_t() : m_ptr(0) {}
		inline autoreference_t(const autoreference_t<T> & p_source) : m_ptr(p_source.m_ptr) {if (m_ptr) m_ptr->refcount_add_ref();}

		inline ~autoreference_t() {if (m_ptr) m_ptr->refcount_release();}


		inline void copy(const autoreference_t<T> & p_source)
		{
			if (m_ptr) m_ptr->refcount_release();
			m_ptr = p_source.m_ptr;
			if (m_ptr) m_ptr->refcount_add_ref();
		}

		inline const autoreference_t<T> & operator=(const autoreference_t<T> & p_source) {copy(p_source); return *this;}

	

		inline void release()
		{
			if (m_ptr) m_ptr->refcount_release();
			m_ptr = 0;
		}


		inline T * operator->() const {assert(m_ptr != 0);return m_ptr->get_ptr();}

		inline T * get_ptr() const {assert(m_ptr != 0); return m_ptr->get_ptr();}
		

		inline bool is_valid() const {return m_ptr != 0;}
		inline bool is_empty() const {return m_ptr == 0;}

		inline bool operator==(const autoreference_t<T> & p_item) const {return m_ptr == p_item.m_ptr;}
		inline bool operator!=(const autoreference_t<T> & p_item) const {return m_ptr != p_item.m_ptr;}
		inline bool operator>(const autoreference_t<T> & p_item) const {return m_ptr > p_item.m_ptr;}
		inline bool operator<(const autoreference_t<T> & p_item) const {return m_ptr < p_item.m_ptr;}

		bool create()
		{
			release();
			m_ptr = new t_ref();
			if (m_ptr == 0) return false;
			m_ptr->refcount_add_ref();
			return true;
		}

		bool create_auto()
		{
			if (is_valid()) return true;
			return create();
		}

		bool create(const T & p_item)
		{
			release();
			m_ptr = new t_ref(p_item);
			if (m_ptr == 0) return false;
			m_ptr->refcount_add_ref();
			return true;
		}

		inline static void g_swap(autoreference_t<T> & item1, autoreference_t<T> & item2)
		{
			pfc::swap_t(item1.m_ptr,item2.m_ptr);
		}

	private:
		t_ref * m_ptr;
	};

	template<typename T>
	inline void swap_t(refcounted_ptr_t<T> & item1, refcounted_ptr_t<T> & item2)
	{
		refcounted_ptr_t<T>::g_swap(item1,item2);
	}

	template<typename T>
	inline void swap_t(autoreference_t<T> & item1, autoreference_t<T> & item2)
	{
		autoreference_t<T>::g_swap(item1,item2);
	}

};