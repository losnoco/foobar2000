namespace pfc {

	PFC_DLL_EXPORT void swap_void(void * item1,void * item2,unsigned width);

	PFC_DLL_EXPORT void reorder_void(void * data,unsigned width,const unsigned * order,unsigned num,void (*swapfunc)(void * item1,void * item2,unsigned width) = swap_void);

	class NOVTABLE reorder_callback
	{
	public:
		virtual void swap(unsigned p_index1,unsigned p_index2) = 0;
	};

	PFC_DLL_EXPORT void reorder(reorder_callback & p_callback,const unsigned * p_order,unsigned p_count);

	template<typename t_container>
	class reorder_callback_impl_t : public reorder_callback
	{
	public:
		reorder_callback_impl_t(t_container & p_data) : m_data(p_data) {}
		void swap(unsigned p_index1,unsigned p_index2)
		{
			pfc::swap_t(m_data[p_index1],m_data[p_index2]);
		}
	private:
		t_container & m_data;
	};

	class reorder_callback_impl_delta : public reorder_callback
	{
	public:
		reorder_callback_impl_delta(reorder_callback & p_data,unsigned p_delta) : m_data(p_data), m_delta(p_delta) {}
		void swap(unsigned p_index1,unsigned p_index2)
		{
			m_data.swap(p_index1+m_delta,p_index2+m_delta);
		}
	private:
		reorder_callback & m_data;
		unsigned m_delta;
	};

	template<typename t_container>
	void reorder_t(t_container & p_data,const unsigned * p_order,unsigned p_count)
	{
		reorder(reorder_callback_impl_t<t_container>(p_data),p_order,p_count);
	}

	template<typename t_container>
	void reorder_partial_t(t_container & p_data,unsigned p_base,const unsigned * p_order,unsigned p_count)
	{
		reorder(reorder_callback_impl_delta(reorder_callback_impl_t<t_container>(p_data),p_base),p_order,p_count);
	}
	
	template<typename T>
	class reorder_callback_impl_ptr_t : public reorder_callback
	{
	public:
		reorder_callback_impl_ptr_t(T * p_data) : m_data(p_data) {}
		void swap(unsigned p_index1,unsigned p_index2)
		{
			pfc::swap_t(m_data[p_index1],m_data[p_index2]);
		}
	private:
		T* m_data;
	};


	template<typename T>
	void reorder_ptr_t(T* p_data,const unsigned * p_order,unsigned p_count)
	{
		reorder(reorder_callback_impl_ptr_t<T>(p_data),p_order,p_count);
	}



	class NOVTABLE sort_callback
	{
	public:
		virtual int compare(unsigned p_index1, unsigned p_index2) const = 0;
		virtual void swap(unsigned p_index1, unsigned p_index2) = 0;
	};

	class sort_callback_stabilizer : public sort_callback
	{
	public:
		sort_callback_stabilizer(sort_callback & p_chain,unsigned p_count);
		virtual int compare(unsigned p_index1, unsigned p_index2) const;
		virtual void swap(unsigned p_index1, unsigned p_index2);
	private:
		sort_callback & m_chain;
		array_t<unsigned> m_order;
	};

	PFC_DLL_EXPORT void sort(sort_callback & p_callback,unsigned p_count);
	PFC_DLL_EXPORT void sort_stable(sort_callback & p_callback,unsigned p_count);

	PFC_DLL_EXPORT void sort_void_ex(void *base,unsigned num,unsigned width, int (*comp)(const void *, const void *),void (*swap)(void *, void *, unsigned) );
	PFC_DLL_EXPORT void sort_void(void * base,unsigned num,unsigned width,int (*comp)(const void *, const void *) );

	template<typename t_container,typename t_compare>
	class sort_callback_impl_simple_wrap_t : public sort_callback
	{
	public:
		sort_callback_impl_simple_wrap_t(t_container & p_data, t_compare p_compare) : m_data(p_data), m_compare(p_compare) {}
		int compare(unsigned p_index1, unsigned p_index2) const
		{
			return m_compare(m_data[p_index1],m_data[p_index2]);
		}

		void swap(unsigned p_index1, unsigned p_index2)
		{
			swap_t(m_data[p_index1],m_data[p_index2]);
		}
	private:
		t_container & m_data;
		t_compare m_compare;
	};

	template<typename t_container>
	class sort_callback_impl_auto_wrap_t : public sort_callback
	{
	public:
		sort_callback_impl_auto_wrap_t(t_container & p_data) : m_data(p_data) {}
		int compare(unsigned p_index1, unsigned p_index2) const
		{
			return compare_t(m_data[p_index1],m_data[p_index2]);
		}

		void swap(unsigned p_index1, unsigned p_index2)
		{
			swap_t(m_data[p_index1],m_data[p_index2]);
		}
	private:
		t_container & m_data;
	};

	template<typename t_container,typename t_compare,typename t_permutation>
	class sort_callback_impl_permutation_wrap_t : public sort_callback
	{
	public:
		sort_callback_impl_permutation_wrap_t(const t_container & p_data, t_compare p_compare,t_permutation p_permutation) : m_data(p_data), m_compare(p_compare), m_permutation(p_permutation) {}
		int compare(unsigned p_index1, unsigned p_index2) const
		{
			return m_compare(m_data[m_permutation[p_index1]],m_data[m_permutation[p_index2]]);
		}

		void swap(unsigned p_index1, unsigned p_index2)
		{
			swap_t(m_permutation[p_index1],m_permutation[p_index2]);
		}
	private:
		const t_container & m_data;
		t_compare m_compare;
		t_permutation m_permutation;
	};

	template<typename t_container,typename t_compare>
	static void sort_t(t_container & p_data,t_compare p_compare,unsigned p_count)
	{
		sort(sort_callback_impl_simple_wrap_t<t_container,t_compare>(p_data,p_compare),p_count);
	}

	template<typename t_container,typename t_compare>
	static void sort_stable_t(t_container & p_data,t_compare p_compare,unsigned p_count)
	{
		sort_stable(sort_callback_impl_simple_wrap_t<t_container,t_compare>(p_data,p_compare),p_count);
	}

	template<typename t_container,typename t_compare,typename t_permutation>
	static void sort_get_permutation_t(const t_container & p_data,t_compare p_compare,unsigned p_count,t_permutation p_permutation)
	{
		sort(sort_callback_impl_permutation_wrap_t<t_container,t_compare,t_permutation>(p_data,p_compare,p_permutation),p_count);
	}

	template<typename t_container,typename t_compare,typename t_permutation>
	static void sort_stable_get_permutation_t(const t_container & p_data,t_compare p_compare,unsigned p_count,t_permutation p_permutation)
	{
		sort_stable(sort_callback_impl_permutation_wrap_t<t_container,t_compare,t_permutation>(p_data,p_compare,p_permutation),p_count);
	}

}