#ifndef _PFC_LIST_H_
#define _PFC_LIST_H_


#if 0
//todo
class NOVTABLE permutation_base_const
{
public:
	virtual unsigned get_item(unsigned p_index) const = 0;
};

class NOVTABLE permutation_base : public permutation_base_const
{
public:
	virtual void swap(unsigned p_index1,unsigned p_index2) = 0;
	
};

template<typename T>
class permutation_buffer_const_t : public permutation_base_const
{
public:
	permutation_buffer_const_t(const T* p_buffer,unsigned p_count) : m_buffer(p_buffer), m_count(p_count) {}
	unsigned get_item(unsigned p_index) const {assert(p_index < m_count); return m_buffer[p_index];}
private:
	const T* m_buffer;
	unsigned m_count;
};

template<typename T>
class permutation_buffer_t : public permutation_base
{
public:
	permutation_buffer_t(T* p_buffer,unsigned p_count) : m_buffer(p_buffer), m_count(p_count) {}
	unsigned get_item(unsigned p_index) const {assert(p_index < m_count); return m_buffer[p_index];}
	void swap(unsigned p_index1,unsigned p_index2)
	{
		assert(p_index1 < m_count && p_index2 < m_count);
		T temp = m_buffer[p_index1];
		m_buffer[p_index1] = m_buffer[p_index2];
		m_buffer[p_index2] = temp;
	}
private:
	const T* m_buffer;
	unsigned m_count;
};

class permutation_inverse_order : public permutation_base_const
{
public:
	permutation_inverse_order(unsigned p_count) : m_count(p_count) {}
	unsigned get_item(unsigned p_index) const {return p_index >= m_count ? p_index : (m_count - 1 - p_index);}
private:
	unsigned m_count;
};

#endif

template<typename T>
class NOVTABLE list_base_const_t
{
public:
	virtual unsigned get_count() const = 0;
	virtual void get_item_ex(T& p_out, unsigned n) const = 0;

	inline T get_item(unsigned n) const {T temp; get_item_ex(temp,n); return temp;}
	inline T operator[](unsigned n) const {T temp; get_item_ex(temp,n); return temp;}

	template<typename t_compare>
	unsigned find_duplicates_sorted_t(t_compare p_compare,bit_array_var & p_out) const
	{
		return pfc::find_duplicates_sorted_t<list_base_const_t<T> const &,t_compare>(*this,get_count(),p_compare,p_out);
	}

	template<typename t_compare,typename t_permutation>
	unsigned find_duplicates_sorted_permutation_t(t_compare p_compare,t_permutation p_permutation,bit_array_var & p_out)
	{
		return pfc::find_duplicates_sorted_permutation_t<list_base_const_t<T> const &,t_compare,t_permutation>(*this,get_count(),p_compare,p_permutation,p_out);
	}

	unsigned find_item(const T & item) const//returns index of first occurance, -1 if not found
	{
		unsigned n,max = get_count();
		for(n=0;n<max;n++)
			if (get_item(n)==item) return n;
		return infinite;
	}

	inline bool have_item(const T & item) const {return find_item(item)!=infinite;}


	template<typename S, typename C>
	bool bsearch_t(C compare,S param,unsigned &index) const 
	{
		return pfc::bsearch_t(get_count(),*this,compare,param,index);
	}

	template<typename t_compare,typename t_permutation>
	void sort_get_permutation_t(t_compare p_compare,t_permutation p_permutation) const
	{
		pfc::sort_get_permutation_t<list_base_const_t<T>,t_compare,t_permutation>(*this,p_compare,get_count(),p_permutation);
	}

	template<typename t_compare,typename t_permutation>
	void sort_stable_get_permutation_t(t_compare p_compare,t_permutation p_permutation) const
	{
		pfc::sort_stable_get_permutation_t<list_base_const_t<T>,t_compare,t_permutation>(*this,p_compare,get_count(),p_permutation);
	}
	
};


template<typename T>
class list_single_ref_t : public list_base_const_t<T>
{
public:
	list_single_ref_t(const T & p_item,unsigned p_count = 1) : m_item(p_item), m_count(p_count) {}
	unsigned get_count() const {return m_count;}
	void get_item_ex(T& p_out,unsigned n) const {assert(n<m_count); p_out = m_item;}
private:
	const T & m_item;
	unsigned m_count;
};

template<typename T>
class list_partial_ref_t : public list_base_const_t<T>
{
public:
	list_partial_ref_t(const list_base_const_t<T> & p_list,unsigned p_base,unsigned p_count)
		: m_list(p_list), m_base(p_base), m_count(p_count)
	{
		assert(m_base + m_count <= m_list.get_count());
	}

private:
	const list_base_const_t<T> & m_list;
	unsigned m_base,m_count;

	unsigned get_count() const {return m_count;}
	void get_item_ex(T & p_out,unsigned n) const {m_list.get_item_ex(p_out,n+m_base);}
};

template<typename T,typename A>
class list_const_array_t : public list_base_const_t<T>
{
public:
	inline list_const_array_t(A p_data,unsigned p_count) : m_data(p_data), m_count(p_count) {}
	unsigned get_count() const {return m_count;}
	void get_item_ex(T & p_out,unsigned n) const {p_out = m_data[n];}
private:
	A m_data;
	unsigned m_count;
};

template<typename to,typename from>
class list_const_cast_t : public list_base_const_t<to>
{
public:
	list_const_cast_t(const list_base_const_t<from> & p_from) : m_from(p_from) {}
	unsigned get_count() const {return m_from.get_count();}
	void get_item_ex(to & p_out,unsigned n) const
	{
		from temp;
		m_from.get_item_ex(temp,n);
		p_out = temp;
	}
private:
	const list_base_const_t<from> & m_from;
};

template<typename T,typename A>
class ptr_list_const_array_t : public list_base_const_t<T*>
{
public:
	inline ptr_list_const_array_t(A p_data,unsigned p_count) : m_data(p_data), m_count(p_count) {}
	unsigned get_count() const {return m_count;}
	void get_item_ex(T* & p_out,unsigned n) const {p_out = &m_data[n];}
private:
	A m_data;
	unsigned m_count;
};
template<typename T>
class list_const_ptr_t : public list_base_const_t<T>
{
public:
	inline list_const_ptr_t(const T * p_data,unsigned p_count) : m_data(p_data), m_count(p_count) {}
	unsigned get_count() const {return m_count;}
	void get_item_ex(T & p_out,unsigned n) const {p_out = m_data[n];}
private:
	const T * m_data;
	unsigned m_count;
};

template<typename T>
class NOVTABLE list_base_t : public list_base_const_t<T>
{
public:
	class NOVTABLE sort_callback
	{
	public:
		virtual int compare(const T& p_item1,const T& p_item2) = 0;
	};

	virtual void filter_mask(const bit_array & mask) = 0;
	virtual unsigned insert_items(const list_base_const_t<T> & items,unsigned base) = 0;
	virtual void reorder_partial(unsigned p_base,const unsigned * p_data,unsigned p_count) = 0;
	virtual void sort(sort_callback & p_callback) = 0;
	virtual void sort_stable(sort_callback & p_callback) = 0;
	virtual void replace_item(unsigned p_index,const T& p_item) = 0;
	virtual void swap_item_with(unsigned p_index,T & p_item) = 0;
	virtual void swap_items(unsigned p_index1,unsigned p_index2) = 0;

	inline void reorder(const unsigned * p_data) {reorder_partial(0,p_data,get_count());}

	inline unsigned insert_item(const T & item,unsigned base) {return insert_items(list_single_ref_t<T>(item),base);}
	unsigned insert_items_repeat(const T & item,unsigned num,unsigned base) {return insert_items(list_single_ref_t<T>(item,num),base);}
	inline unsigned add_items_repeat(T item,unsigned num) {return insert_items_repeat(item,num,get_count());}
	unsigned insert_items_fromptr(const T* source,unsigned num,unsigned base) {return insert_items(list_const_ptr_t<T>(source,num),base);}
	inline unsigned add_items_fromptr(const T* source,unsigned num) {return insert_items_fromptr(source,num,get_count());}

	inline unsigned add_items(const list_base_const_t<T> & items) {return insert_items(items,get_count());}
	inline unsigned add_item(const T& item) {return insert_item(item,get_count());}
	
	inline void remove_mask(const bit_array & mask) {filter_mask(bit_array_not(mask));}
	inline void remove_all() {filter_mask(bit_array_false());}
	inline void truncate(unsigned val) {if (val < get_count()) remove_mask(bit_array_range(val,get_count()-val,true));}
	
	inline T replace_item_ex(unsigned p_index,const T & p_item) {T ret = p_item;swap_item_with(p_index,ret);return ret;}

	inline T operator[](unsigned n) const {return get_item(n);}

	template<typename t_compare>
	class sort_callback_impl_t : public sort_callback
	{
	public:
		sort_callback_impl_t(t_compare p_compare) : m_compare(p_compare) {}
		int compare(const T& p_item1,const T& p_item2) {return m_compare(p_item1,p_item2);}
	private:
		t_compare m_compare;
	};

	class sort_callback_auto : public sort_callback
	{
	public:
		int compare(const T& p_item1,const T& p_item2) {return pfc::compare_t(p_item1,p_item2);}
	};

	void sort() {sort(sort_callback_auto());}
	template<typename t_compare> void sort_t(t_compare p_compare) {sort(sort_callback_impl_t<t_compare>(p_compare));}
	template<typename t_compare> void sort_stable_t(t_compare p_compare) {sort_stable(sort_callback_impl_t<t_compare>(p_compare));}

	template<typename t_compare> void sort_remove_duplicates_t(t_compare p_compare)
	{
		sort_t<t_compare>(p_compare);
		bit_array_bittable array(get_count());		
		if (this->template find_duplicates_sorted_t<t_compare>(p_compare,array) > 0)
			remove_mask(array);
	}

	template<typename t_compare> void sort_stable_remove_duplicates_t(t_compare p_compare)
	{
		sort_stable_t<t_compare>(p_compare);
		bit_array_bittable array(get_count());		
		if (this->template find_duplicates_sorted_t<t_compare>(p_compare,array) > 0)
			remove_mask(array);
	}


	template<typename t_compare> void remove_duplicates_t(t_compare p_compare)
	{
		order_helper order(get_count());
		sort_get_permutation_t<t_compare,order_helper&>(p_compare,order);
		bit_array_bittable array(get_count());
		if (this->template find_duplicates_sorted_permutation_t<t_compare,order_helper const&>(p_compare,order,array) > 0)
			remove_mask(array);
	}
};


template<typename T,typename t_storage>
class list_impl_t : public list_base_t<T>
{
public:
	list_impl_t() {}
	list_impl_t(const list_impl_t<T,t_storage> & p_source) { *this = p_source; }

	void prealloc(unsigned count) {m_buffer.prealloc(count);}

	void set_count(unsigned p_count) {m_buffer.set_size(p_count);}

	void insert_item(const T& item,unsigned idx)
	{
		unsigned max = m_buffer.get_size() + 1;
		m_buffer.set_size(max);
		unsigned n;
		for(n=max-1;n>idx;n--)
			m_buffer[n]=m_buffer[n-1];
		m_buffer[idx]=item;
	}

	T remove_by_idx(unsigned idx)
	{
		T ret = m_buffer[idx];
		int n;
		int max = m_buffer.get_size();
		for(n=idx+1;n<max;n++)
		{
			pfc::swap_t(m_buffer[n-1],m_buffer[n]);
		}
		m_buffer.set_size(max-1);
		return ret;
	}


	inline void get_item_ex(T& p_out,unsigned n) const
	{
		assert(n>=0);
		assert(n<get_count());
		p_out = m_buffer[n];
	}

	inline const T& get_item_ref(unsigned n) const
	{
		assert(n>=0);
		assert(n<get_count());
		return m_buffer[n];
	}

	inline T get_item(unsigned n) const
	{
		assert(n >= 0);
		assert(n < get_count() );
		return m_buffer[n];
	};

	inline unsigned get_count() const {return m_buffer.get_size();}

	inline const T & operator[](unsigned n) const
	{
		assert(n>=0);
		assert(n<get_count());
		return m_buffer[n];
	}

	inline const T* get_ptr() const {return m_buffer.get_ptr();}
	inline T* get_ptr() {return m_buffer.get_ptr();}

	inline T& operator[](unsigned n) {return m_buffer[n];}

	inline void remove_from_idx(unsigned idx,unsigned num)
	{
		remove_mask(bit_array_range(idx,num));
	}

	unsigned insert_items(const list_base_const_t<T> & source,unsigned base)
	{
		unsigned count = get_count();
		if (base>count) base = count;
		unsigned num = source.get_count();
		m_buffer.set_size(count+num);
		if (count > base)
		{
			unsigned n;
			for(n=count-1;(int)n>=(int)base;n--)
			{
				pfc::swap_t(m_buffer[n+num],m_buffer[n]);
			}
		}

		{
			unsigned n;
			for(n=0;n<num;n++)
			{
				source.get_item_ex(m_buffer[n+base],n);
			}
		}
		return base;

	}

	void get_items_mask(list_impl_t<T,t_storage> & out,const bit_array & mask)
	{
		unsigned n,count = get_count();
		for_each_bit_array(n,mask,true,0,count)
			out.add_item(m_buffer[n]);
	}

	void filter_mask(const bit_array & mask)
	{
		unsigned n,count = get_count(), total = 0;

		n = total = mask.find(false,0,count);

		if (n<count)
		{
			for(n=mask.find(true,n+1,count-n-1);n<count;n=mask.find(true,n+1,count-n-1))
				pfc::swap_t(m_buffer[total++],m_buffer[n]);

			m_buffer.set_size(total);
		}
	}

	void replace_item(unsigned idx,const T& item)
	{
		assert(idx>=0);
		assert(idx<get_count());
		m_buffer[idx] = item;
	}

	void sort()
	{
		pfc::sort_callback_impl_auto_wrap_t<t_storage> wrapper(m_buffer);
		pfc::sort(wrapper,get_count());
	}

	template<typename t_compare>
	void sort_t(t_compare p_compare)
	{
		pfc::sort_callback_impl_simple_wrap_t<t_storage,t_compare> wrapper(m_buffer,p_compare);
		pfc::sort(wrapper,get_count());
	}

	template<typename t_compare>
	void sort_stable_t(t_compare p_compare)
	{
		pfc::sort_callback_impl_simple_wrap_t<t_storage,t_compare> wrapper(m_buffer,p_compare);
		pfc::sort_stable(wrapper,get_count());
	}
	inline void reorder_partial(unsigned p_base,const unsigned * p_order,unsigned p_count)
	{
		assert(p_base+p_count<=get_count());
		pfc::reorder_partial_t(m_buffer,p_base,p_order,p_count);
	}

	template<typename t_compare>
	unsigned find_duplicates_sorted_t(t_compare p_compare,bit_array_var & p_out) const
	{
		return pfc::find_duplicates_sorted_t<list_impl_t<T,t_storage> const &,t_compare>(*this,get_count(),p_compare,p_out);
	}

	template<typename t_compare,typename t_permutation>
	unsigned find_duplicates_sorted_permutation_t(t_compare p_compare,t_permutation p_permutation,bit_array_var & p_out)
	{
		return pfc::find_duplicates_sorted_permutation_t<list_impl_t<T,t_storage> const &,t_compare,t_permutation>(*this,get_count(),p_compare,p_permutation,p_out);
	}


private:
	class sort_callback_wrapper
	{
	public:
		explicit inline sort_callback_wrapper(sort_callback & p_callback) : m_callback(p_callback) {}
		inline int operator()(const T& item1,const T& item2) const {return m_callback.compare(item1,item2);}
	private:
		sort_callback & m_callback;
	};
public:
	void sort(sort_callback & p_callback)
	{
		sort_t(sort_callback_wrapper(p_callback));
	}
	
	void sort_stable(sort_callback & p_callback)
	{
		sort_stable_t(sort_callback_wrapper(p_callback));
	}

	void remove_mask(const bit_array & mask) {filter_mask(bit_array_not(mask));}

	void remove_mask(const bool * mask) {remove_mask(bit_array_table(mask,get_count()));}
	void filter_mask(const bool * mask) {filter_mask(bit_array_table(mask,get_count()));}

	unsigned add_item(const T& item)
	{
		unsigned idx = get_count();
		insert_item(item,idx);
		return idx;
	}

	void remove_all() {remove_mask(bit_array_true());}

	void remove_item(const T& item)
	{
		unsigned n,max = get_count();
		bit_array_bittable mask(max);
		for(n=0;n<max;n++)
			mask.set(n,get_item(n)==item);
		remove_mask(mask);		
	}

	void swap_item_with(unsigned p_index,T & p_item)
	{
		assert(p_index < get_count());
		pfc::swap_t(m_buffer[p_index],p_item);
	}

	void swap_items(unsigned p_index1,unsigned p_index2) 
	{
		assert(p_index1 < get_count());
		assert(p_index1 < get_count());
		pfc::swap_t(m_buffer[p_index1],m_buffer[p_index2]);
	}

	inline static void g_swap(list_impl_t<T,t_storage> & p_item1,list_impl_t<T,t_storage> & p_item2)
	{
		pfc::swap_t(p_item1.m_buffer,p_item2.m_buffer);
	}

protected:
	t_storage m_buffer;
};

template<typename T>
class mem_block_list_t : public list_impl_t<T,mem_block_t<T> >
{
public:
	mem_block_list_t() {m_buffer.set_mem_logic(mem_block::ALLOC_FAST);}
	mem_block_list_t(const mem_block_list_t<T> & p_source) {m_buffer.set_mem_logic(mem_block::ALLOC_FAST); *this = p_source;}

	inline void set_mem_logic(mem_block::mem_logic_t v) {this->m_buffer.set_mem_logic(v);}

	void prealloc(unsigned count) {m_buffer.prealloc(count);}
};

template<typename T, typename S = array_fast_t<T> >
class list_t : public list_impl_t<T,S >
{
public:
	list_t() {}
	list_t(const list_t<T,S> & p_src) {*this = p_src;}
	
};

namespace pfc {
	template<typename T,typename A>
	inline void swap_t(list_t<T,A> & item1, list_t<T,A> & item2)
	{
		list_t<T,A>::g_swap(item1,item2);
	}

	template<typename T,typename A>
	inline void swap_t(list_impl_t<T,A> & item1, list_impl_t<T,A> & item2)
	{
		list_impl_t<T,A>::g_swap(item1,item2);
	}
};

template<typename T>
class ptr_list_const_cast_t : public list_base_const_t<const T *>
{
public:
	inline ptr_list_const_cast_t(const list_base_const_t<T*> & p_param) : m_param(p_param) {}
	unsigned get_count() const {return m_param.get_count();}
	void get_item_ex(const T * & p_out,unsigned n) const {T* temp; m_param.get_item_ex(temp,n); p_out = temp;}
private:
	const list_base_const_t<T*> & m_param;

};


template<typename T,typename P>
class list_const_permutation_t : public list_base_const_t<T>
{
public:
	inline list_const_permutation_t(const list_base_const_t<T> & p_list,P p_permutation) : m_list(p_list), m_permutation(p_permutation) {}
	unsigned get_count() const {return m_list.get_count();}
	void get_item_ex(T & p_out,unsigned n) const {m_list.get_item_ex(p_out,m_permutation[n]);}
private:
	P m_permutation;
	const list_base_const_t<T> & m_list;
};


template<class T>
class list_permutation_t : public list_base_const_t<T>
{
public:
	unsigned get_count() const {return m_count;}
	void get_item_ex(T & p_out,unsigned n) const {m_base.get_item_ex(p_out,m_order[n]);}
	list_permutation_t(const list_base_const_t<T> & p_base,const unsigned * p_order,unsigned p_count)
		: m_base(p_base), m_order(p_order), m_count(p_count)
	{
		assert(m_base.get_count() >= m_count);
	}
private:
	const list_base_const_t<T> & m_base;
	const unsigned * m_order;
	unsigned m_count;
};

#define mem_block_list mem_block_list_t //for compatibility

#endif //_PFC_LIST_H_