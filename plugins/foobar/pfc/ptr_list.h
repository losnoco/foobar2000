#ifndef __PFC_PTR_LIST_H_
#define __PFC_PTR_LIST_H_

template<class T>
class ptr_list_t : public mem_block_list<T*>
{
public:
	ptr_list_t() {}
	ptr_list_t(const ptr_list_t<T> & p_source) {*this = p_source;}

	void free_by_idx(unsigned n)
	{
		void * ptr = remove_by_idx(n);
		assert(ptr);
		free(ptr);
	}

	void free_all()
	{
		unsigned n,max=get_count();
		for(n=0;n<max;n++) free(get_item(n));
		remove_all();
	}

	void free_mask(const bit_array & mask)
	{
		unsigned n,max=get_count();
		for(n=mask.find(true,0,max);n<max;n=mask.find(true,n+1,max-n-1))
		{
			void * ptr = get_item(n);
			assert(ptr);
			free(ptr);
		}
		remove_mask(mask);
	}

	void add_items_copy(const list_base_const_t<T*> & source)
	{
		unsigned n;
		for(n=0;n<source.get_count();n++) add_item(new T(*source.get_item(n)));
	}

	inline void delete_item(T* ptr) {remove_item(ptr);delete ptr;}

	inline void delete_by_idx(unsigned idx) 
	{
		T* ptr = remove_by_idx(idx);
		assert(ptr);
		delete ptr;
	}

	void delete_all()
	{
		unsigned n,max=get_count();
		for(n=0;n<max;n++)
		{
			T* ptr = get_item(n);
			assert(ptr);
			delete ptr;
		}
		remove_all();
	}

	void delete_mask(const bit_array & mask)
	{
		unsigned n,max=get_count();
		for(n=mask.find(true,0,max);n<max;n=mask.find(true,n+1,max-n-1))
		{
			T* ptr = get_item(n);
			assert(ptr);
			delete ptr;
		}
		remove_mask(mask);
	}

	inline void delete_mask(const bool * mask) {delete_mask(bit_array_table(mask,get_count()));}


	T * operator[](unsigned n) const {return get_item(n);}
};

typedef ptr_list_t<void> ptr_list;


template<class T>
class ptr_list_autodel_t : public ptr_list_t<T>
{
public:
	ptr_list_autodel_t() {}
	ptr_list_autodel_t(const ptr_list_autodel_t<T> & p_source) {*this = p_source;}

	~ptr_list_autodel_t() {delete_all();}
};


template<class T>
class ptr_list_autofree_t : public ptr_list_t<T>
{
public:
	ptr_list_autofree_t() {}
	ptr_list_autofree_t(const ptr_list_autofree_t<T> & p_source) {*this = p_source;}

	~ptr_list_autofree_t() {free_all();}
};


namespace pfc {
	template<typename T>
	inline void swap_t(ptr_list_t<T> & item1, ptr_list_t<T> & item2)
	{
		ptr_list_t<T>::g_swap(item1,item2);
	}

	template<typename T>
	inline void swap_t(ptr_list_autofree_t<T> & item1, ptr_list_autofree_t<T> & item2)
	{
		ptr_list_t<T>::g_swap(item1,item2);
	}

	template<typename T>
	inline void swap_t(ptr_list_autodel_t<T> & item1, ptr_list_autodel_t<T> & item2)
	{
		ptr_list_t<T>::g_swap(item1,item2);
	}
};


#endif //__PFC_PTR_LIST_H_