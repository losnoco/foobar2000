#ifndef __PFC_PTR_LIST_H_
#define __PFC_PTR_LIST_H_

template<class T, class B = list_t<T*> >
class ptr_list_t : public B
{
public:
	ptr_list_t() {}
	ptr_list_t(const ptr_list_t<T> & p_source) {*this = p_source;}

	void free_by_idx(t_size n) {free_mask(bit_array_one(n));}
	void free_all() {remove_all_ex(free);}
	void free_mask(const bit_array & p_mask) {remove_mask_ex(p_mask,free);}

	void delete_item(T* ptr) {delete_by_idx(find_item(ptr));}

	void delete_by_idx(t_size p_index) {
		delete_mask(bit_array_one(p_index));
	}

	void delete_all() {
		remove_all_ex(pfc::delete_t<T>);
	}

	void delete_mask(const bit_array & p_mask) {
		remove_mask_ex(p_mask,pfc::delete_t<T>);
	}

	T * operator[](t_size n) const {return get_item(n);}
};

template<typename T,t_size N>
class ptr_list_hybrid_t : public ptr_list_t<T,list_hybrid_t<T*,N> > {
public:
	ptr_list_hybrid_t() {}
	ptr_list_hybrid_t(const ptr_list_hybrid_t<T,N> & p_source) {*this = p_source;}
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



namespace pfc {
	template<typename T>
	inline void swap_t(ptr_list_t<T> & item1, ptr_list_t<T> & item2)
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