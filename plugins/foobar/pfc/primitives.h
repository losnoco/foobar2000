class bit_array;
class bit_array_var;

#define tabsize(x) ((size_t)(sizeof(x)/sizeof(*x)))

#define TEMPLATE_CONSTRUCTOR_FORWARD_FLOOD_WITH_INITIALIZER(THISCLASS,MEMBER,INITIALIZER)	\
	THISCLASS() INITIALIZER	\
	template<typename t_param1>																								THISCLASS(const t_param1 & p_param1) :																																		MEMBER(p_param1) INITIALIZER	\
	template<typename t_param1,typename t_param2>																			THISCLASS(const t_param1 & p_param1,const t_param2 & p_param2) :																											MEMBER(p_param1,p_param2) INITIALIZER	\
	template<typename t_param1,typename t_param2,typename t_param3>															THISCLASS(const t_param1 & p_param1,const t_param2 & p_param2,const t_param3 & p_param3) :																					MEMBER(p_param1,p_param2,p_param3) INITIALIZER	\
	template<typename t_param1,typename t_param2,typename t_param3,typename t_param4>										THISCLASS(const t_param1 & p_param1,const t_param2 & p_param2,const t_param3 & p_param3,const t_param4 & p_param4) :														MEMBER(p_param1,p_param2,p_param3,p_param4) INITIALIZER	\
	template<typename t_param1,typename t_param2,typename t_param3,typename t_param4,typename t_param5>						THISCLASS(const t_param1 & p_param1,const t_param2 & p_param2,const t_param3 & p_param3,const t_param4 & p_param4,const t_param5 & p_param5) :								MEMBER(p_param1,p_param2,p_param3,p_param4,p_param5) INITIALIZER	\
	template<typename t_param1,typename t_param2,typename t_param3,typename t_param4,typename t_param5,typename t_param6>	THISCLASS(const t_param1 & p_param1,const t_param2 & p_param2,const t_param3 & p_param3,const t_param4 & p_param4,const t_param5 & p_param5,const t_param6 & p_param6) :	MEMBER(p_param1,p_param2,p_param3,p_param4,p_param5,p_param6) INITIALIZER

#define TEMPLATE_CONSTRUCTOR_FORWARD_FLOOD(THISCLASS,MEMBER) TEMPLATE_CONSTRUCTOR_FORWARD_FLOOD_WITH_INITIALIZER(THISCLASS,MEMBER,{})

#define PFC_DECLARE_EXCEPTION(NAME,BASECLASS,DEFAULTMSG)	\
class NAME : public BASECLASS {	\
public:	\
	NAME() : BASECLASS(DEFAULTMSG,0) {}	\
	NAME(const char * p_msg) : BASECLASS(p_msg) {}	\
	NAME(const char * p_msg,int) : BASECLASS(p_msg,0) {}	\
	NAME(const NAME & p_source) : BASECLASS(p_source) {}	\
};

namespace pfc {

	template<typename t_type> void __unsafe__memcpy_t(t_type * p_dst,const t_type * p_src,t_size p_count) {
		::memcpy(reinterpret_cast<void*>(p_dst), reinterpret_cast<const void*>(p_src), p_count * sizeof(t_type));
	}

	template<typename t_type> void __unsafe__in_place_destructor_t(t_type & p_item) throw() {
		if (traits_t<t_type>::needs_destructor) try{ p_item.~t_type(); } catch(...) {}
	}
	
	template<typename t_type> void __unsafe__in_place_constructor_t(t_type & p_item) {
		if (traits_t<t_type>::needs_constructor) {
			t_type * ret = new(&p_item) t_type;
			PFC_ASSERT(ret == &p_item);
		}
	}

	template<typename t_type> void __unsafe__in_place_destructor_array_t(t_type * p_items, t_size p_count) throw() {
		if (traits_t<t_type>::needs_destructor) {
			t_type * walk = p_items;
			for(t_size n=p_count;n;--n) __unsafe__in_place_destructor_t(*(walk++));
		}
	}
	
	template<typename t_type> t_type * __unsafe__in_place_constructor_array_t(t_type * p_items,t_size p_count) {
		if (traits_t<t_type>::needs_constructor) {
			t_size walkptr = 0;
			try {
				for(walkptr=0;walkptr<p_count;++walkptr) __unsafe__in_place_constructor_t(p_items[walkptr]);
			} catch(...) {
				__unsafe__in_place_destructor_array_t(p_items,walkptr);
				throw;
			}
		}
		return p_items;
	}

	template<typename t_type> t_type * __unsafe__in_place_resize_array_t(t_type * p_items,t_size p_from,t_size p_to) {
		if (p_from < p_to) __unsafe__in_place_constructor_array_t(p_items + p_from, p_to - p_from);
		else if (p_from > p_to) __unsafe__in_place_destructor_array_t(p_items + p_to, p_from - p_to);
		return p_items;
	}

	template<typename t_type,typename t_copy> void __unsafe__in_place_constructor_copy_t(t_type & p_item,const t_copy & p_copyfrom) {
		if (traits_t<t_type>::needs_constructor) {
			t_type * ret = new(&p_item) t_type(p_copyfrom);
			PFC_ASSERT(ret == &p_item);
		} else {
			p_item = p_copyfrom;
		}
	}

	template<typename t_type,typename t_copy> t_type * __unsafe__in_place_constructor_array_copy_t(t_type * p_items,t_size p_count, const t_copy * p_copyfrom) {
		t_size walkptr = 0;
		try {
			for(walkptr=0;walkptr<p_count;++walkptr) __unsafe__in_place_constructor_copy_t(p_items[walkptr],p_copyfrom[walkptr]);
		} catch(...) {
			__unsafe__in_place_destructor_array_t(p_items,walkptr);
			throw;
		}
		return p_items;
	}

	template<typename t_type,typename t_copy> t_type * __unsafe__in_place_constructor_array_copy_partial_t(t_type * p_items,t_size p_count, const t_copy * p_copyfrom,t_size p_copyfrom_count) {
		if (p_copyfrom_count > p_count) p_copyfrom_count = p_count;
		__unsafe__in_place_constructor_array_copy_t(p_items,p_copyfrom_count,p_copyfrom);
		try {
			__unsafe__in_place_constructor_array_t(p_items + p_copyfrom_count,p_count - p_copyfrom_count);
		} catch(...) {
			__unsafe__in_place_destructor_array_t(p_items,p_copyfrom_count);
			throw;
		}
		return p_items;
	}

	template<typename t_ret,typename t_param> 
	t_ret safe_cast(t_param const & p_param) {
		t_ret temp ( p_param );
		return temp;
	}

	template<typename t_ret,typename t_param>
	t_ret downcast_guarded(const t_param & p_param) {
		t_ret temp = (t_ret) p_param;
		if ((t_param) temp != p_param) throw exception_overflow();
		return temp;
	}
	
	template<typename t_exception,typename t_ret,typename t_param>
	t_ret downcast_guarded_ex(const t_param & p_param) {
		t_ret temp = (t_ret) p_param;
		if ((t_param) temp != p_param) throw t_exception;
		return temp;
	}

	template<typename t_acc,typename t_add>
	void accumulate_guarded(t_acc & p_acc, const t_add & p_add) {
		t_acc delta = downcast_guarded<t_acc>(p_add);
		delta += p_acc;
		if (delta < p_acc) throw exception_overflow();
		p_acc = delta;
	}

	typedef std::exception exception;

	PFC_DECLARE_EXCEPTION(exception_overflow,exception,"Overflow");
	PFC_DECLARE_EXCEPTION(exception_bug_check,exception,"Bug check");
	PFC_DECLARE_EXCEPTION(exception_not_implemented,exception_bug_check,"Feature not implemented");
	PFC_DECLARE_EXCEPTION(exception_dynamic_assert,exception_bug_check,"dynamic_assert failure");

	//deprecated
	inline void bug_check_assert(bool p_condition, const char * p_msg) {
		if (!p_condition) {
			PFC_ASSERT(0);
			throw exception_bug_check(p_msg);
		}
	}
	//deprecated
	inline void bug_check_assert(bool p_condition) {
		if (!p_condition) {
			PFC_ASSERT(0);
			throw exception_bug_check();
		}
	}

	inline void dynamic_assert(bool p_condition, const char * p_msg) {
		if (!p_condition) {
			PFC_ASSERT(0);
			throw exception_dynamic_assert(p_msg);
		}
	}
	inline void dynamic_assert(bool p_condition) {
		if (!p_condition) {
			PFC_ASSERT(0);
			throw exception_dynamic_assert();
		}
	}

	template<typename T>
	inline void swap_multi_t(T * p_buffer1,T * p_buffer2,t_size p_size) {
		T * walk1 = p_buffer1, * walk2 = p_buffer2;
		for(t_size n=p_size;n;--n) {
			T temp (* walk1);
			*walk1 = *walk2;
			*walk2 = temp;
			walk1++; walk2++;
		}
	}

	template<typename T,t_size p_size>
	inline void swap_multi_t(T * p_buffer1,T * p_buffer2) {
		T * walk1 = p_buffer1, * walk2 = p_buffer2;
		for(t_size n=p_size;n;--n) {
			T temp (* walk1);
			*walk1 = *walk2;
			*walk2 = temp;
			walk1++; walk2++;
		}
	}


	template<t_size p_size>
	inline void __unsafe__swap_raw_t(void * p_object1, void * p_object2) {
		if (p_size % sizeof(t_size) == 0) {
			swap_multi_t<t_size,p_size/sizeof(t_size)>(reinterpret_cast<t_size*>(p_object1),reinterpret_cast<t_size*>(p_object2));
		} else {
			swap_multi_t<t_uint8,p_size>(reinterpret_cast<t_uint8*>(p_object1),reinterpret_cast<t_uint8*>(p_object2));
		}
	}

	template<typename T>
	inline void swap_t(T & p_item1, T & p_item2) {
		if (traits_t<T>::realloc_safe) {
			__unsafe__swap_raw_t<sizeof(T)>( reinterpret_cast<void*>( &p_item1 ), reinterpret_cast<void*>( &p_item2 ) );
		} else {
			T temp(p_item2);
			p_item2 = p_item1;
			p_item1 = temp;
		}
	}

	template<typename t_array>
	t_size array_size_t(const t_array & p_array) {return p_array.get_size();}

	template<typename t_item, t_size p_width>
	t_size array_size_t(const t_item (&p_array)[p_width]) {return p_width;}


	template<typename t_array,typename t_filler>
	inline void fill_t(t_array & p_buffer,const t_size p_count, const t_filler & p_filler) {
		for(t_size n=0;n<p_count;n++)
			p_buffer[n] = p_filler;
	}

	template<typename t_array,typename t_filler>
	inline void fill_ptr_t(t_array * p_buffer,const t_size p_count, const t_filler & p_filler) {
		for(t_size n=0;n<p_count;n++)
			p_buffer[n] = p_filler;
	}

	template<typename T>
	inline int compare_t(const T & item1,const T & item2)
	{
		if (item1<item2) return -1;
		else if (item1>item2) return 1;
		else return 0;
	}

	

	template<typename t_array,typename T>
	inline t_size append_t(t_array & p_array,const T & p_item)
	{
		t_size old_count = p_array.get_size();
		p_array.set_size(old_count + 1);
		p_array[old_count] = p_item;
		return old_count;
	}

	template<typename t_array,typename T>
	inline t_size append_swap_t(t_array & p_array,T & p_item)
	{
		t_size old_count = p_array.get_size();
		p_array.set_size(old_count + 1);
		swap_t(p_array[old_count],p_item);
		return old_count;
	}

	template<typename t_array,typename T>
	inline t_size insert_t(t_array & p_array,const T & p_item,t_size p_index)
	{
		t_size old_count = p_array.get_size();
		if (p_index > old_count) p_index = old_count;
		p_array.set_size(old_count + 1);
		for(t_size n=old_count;n>p_index;n--)
			p_array[n] = p_array[n-1];
		p_array[p_index] = p_item;
		return p_index;
	}

	template<typename t_array,typename T>
	inline t_size insert_swap_t(t_array & p_array,T & p_item,t_size p_index)
	{
		t_size old_count = p_array.get_size();
		if (p_index > old_count) p_index = old_count;
		p_array.set_size(old_count + 1);
		for(t_size n=old_count;n>p_index;n--)
			swap_t(p_array[n],p_array[n-1]);
		swap_t(p_array[p_index],p_item);
		return p_index;
	}

	
	template<typename t_array>
	inline t_size remove_mask_t(t_array & p_array,const bit_array & p_mask)//returns amount of items left
	{
		t_size n,count = p_array.get_size(), total = 0;

		n = total = p_mask.find(true,0,count);

		if (n<count)
		{
			for(n=p_mask.find(false,n+1,count-n-1);n<count;n=p_mask.find(false,n+1,count-n-1))
				swap_t(p_array[total++],p_array[n]);

			p_array.set_size(total);
			
			return total;
		}
		else return count;
	}

	template<typename T>
	inline T max_t(const T & item1, const T & item2) {return item1 > item2 ? item1 : item2;};

	template<typename T>
	inline T min_t(const T & item1, const T & item2) {return item1 < item2 ? item1 : item2;};

	template<typename T>
	inline T abs_t(T item) {return item<0 ? -item : item;}

	template<typename T>
	inline T clip_t(const T & p_item, const T & p_min, const T & p_max) {
		if (p_item < p_min) return p_min;
		else if (p_item <= p_max) return p_item;
		else return p_max;
	}



	template<typename t_array,typename t_compare>
	t_size find_duplicates_sorted_t(t_array p_array,t_size p_count,t_compare p_compare,bit_array_var & p_out) {
		t_size ret = 0;
		t_size n;
		if (p_count > 0)
		{
			p_out.set(0,false);
			for(n=1;n<p_count;n++)
			{
				bool found = p_compare(p_array[n-1],p_array[n]) == 0;
				if (found) ret++;
				p_out.set(n,found);
			}
		}
		return ret;
	}

	template<typename t_array,typename t_compare,typename t_permutation>
	t_size find_duplicates_sorted_permutation_t(t_array p_array,t_size p_count,t_compare p_compare,t_permutation const & p_permutation,bit_array_var & p_out) {
		t_size ret = 0;
		t_size n;
		if (p_count > 0) {
			p_out.set(p_permutation[0],false);
			for(n=1;n<p_count;n++)
			{
				bool found = p_compare(p_array[p_permutation[n-1]],p_array[p_permutation[n]]) == 0;
				if (found) ret++;
				p_out.set(p_permutation[n],found);
			}
		}
		return ret;
	}




	template<typename T>
	inline void delete_t(T* ptr) {delete ptr;}

	template<typename T>
	inline void delete_array_t(T* ptr) {delete[] ptr;}

	template<typename T>
	inline T* clone_t(T* ptr) {return new T(*ptr);}


	template<typename t_exception,typename t_int>
	inline t_int mul_safe_t(t_int p_val1,t_int p_val2) {
		if (p_val1 == 0 || p_val2 == 0) return 0;
		t_int temp = (t_int) (p_val1 * p_val2);
		if (temp < p_val1 || temp < p_val2) throw t_exception();
		return temp;
	}

	template<typename T>
	void memcpy_t(T* p_dst,const T* p_src,t_size p_count) {
		for(t_size n=0;n<p_count;n++) p_dst[n] = p_src[n];
	}

	template<typename t_dst,typename t_src>
	void memcpy_t(t_dst & p_dst,const t_src & p_src,t_size p_count) {
		for(t_size n=0;n<p_count;n++) p_dst[n] = p_src[n];
	}

	template<typename T>
	void memcpy_backwards_t(T* p_dst,const T* p_src,t_size p_count) {
		p_dst += p_count; p_src += p_count;
		for(t_size n=0;n<p_count;n++) *(--p_dst) = *(--p_src);
	}

	template<typename T,typename t_val>
	void memset_t(T * p_buffer,const t_val & p_val,t_size p_count) {
		for(t_size n=0;n<p_count;n++) p_buffer[n] = p_val;
	}

	template<typename T,typename t_val>
	void memset_t(T &p_buffer,const t_val & p_val) {
		const t_size width = pfc::array_size_t(p_buffer);
		for(t_size n=0;n<width;n++) p_buffer[n] = p_val;
	}

	template<typename T>
	void memset_null_t(T * p_buffer,t_size p_count) {
		for(t_size n=0;n<p_count;n++) p_buffer[n] = 0;
	}

	template<typename T>
	void memset_null_t(T &p_buffer) {
		const t_size width = pfc::array_size_t(p_buffer);
		for(t_size n=0;n<width;n++) p_buffer[n] = 0;
	}

	template<typename T>
	void memmove_t(T* p_dst,const T* p_src,t_size p_count) {
		if (p_dst == p_src) {/*do nothing*/}
		else if (p_dst > p_src && p_dst < p_src + p_count) memcpy_backwards_t<T>(p_dst,p_src,p_count);
		else memcpy_t<T>(p_dst,p_src,p_count);
	}

	template<typename T>
	T* new_ptr_check_t(T* p_ptr) {
		if (p_ptr == NULL) throw std::bad_alloc();
		return p_ptr;
	}

	template<typename T>
	t_size msize_t(const T * p_ptr) throw() {
		static_assert_t<sizeof(T) != 0>();
		t_size size = ::_msize(const_cast<void*>(reinterpret_cast<const void*>(p_ptr)));
		PFC_ASSERT( size % sizeof(T) == 0);
		return size / sizeof(T);
	}

	
	template<typename T>
	T* __unsafe__malloc_t(t_size p_size) {
		return pfc::new_ptr_check_t( reinterpret_cast<T*>(::malloc(mul_safe_t<std::bad_alloc,t_size>(p_size, sizeof(T)))) );
	}

	template<typename T>
	void __unsafe__free_t(T * p_ptr) throw() {
		::free(reinterpret_cast<void*>(p_ptr));
	}

	template<typename t_type>
	t_type* malloc_t(t_size p_size) {
		static_assert_t< sizeof(t_type)!=0 >();
#if 1
		t_type* ptr = __unsafe__malloc_t<t_type>(p_size);
		try {
			__unsafe__in_place_constructor_array_t(ptr,p_size);
		} catch(...) {
			__unsafe__free_t(ptr);
			throw;
		}
		return ptr;
#else
		return new t_type[p_size];
#endif
	}
	template<typename t_type,typename t_copyfrom>
	t_type* malloc_copy_t(t_size p_size,const t_copyfrom * p_copyfrom,t_size p_copyfrom_size) {
		static_assert_t< sizeof(t_type)!=0 >();
		t_type * ptr = __unsafe__malloc_t<t_type>(p_size);
		try {
			__unsafe__in_place_constructor_array_copy_t(ptr,min_t(p_size,p_copyfrom_size),p_copyfrom);
			if (p_copyfrom_size < p_size) {
				try {
					__unsafe__in_place_constructor_array_t(ptr + p_copyfrom_size, p_size - p_copyfrom_size);
				} catch(...) {
					__unsafe__in_place_destructor_array_t(ptr,min_t(p_size,p_copyfrom_size));
					throw;
				}
			}
		} catch(...) {
			__unsafe__free_t(ptr);
			throw;
		}
		return ptr;
	}

	template<typename t_type,typename t_copyfrom>
	t_type* malloc_copy_t(t_size p_size,const t_copyfrom * p_copyfrom) {
		static_assert_t< sizeof(t_type)!=0 >();
		t_type * ptr = __unsafe__malloc_t<t_type>(p_size);
		try {
			__unsafe__in_place_constructor_array_copy_t(ptr,p_size,p_copyfrom);
		} catch(...) {
			__unsafe__free_t(ptr);
			throw;
		}
		return ptr;
	}

	template<typename T>
	T* __unsafe__realloc_t(T* p_ptr,t_size p_size) {
		return pfc::new_ptr_check_t( reinterpret_cast<T*>(::realloc(reinterpret_cast<void*>(p_ptr),mul_safe_t<std::bad_alloc,t_size>(p_size, sizeof(T)))) );
	}

	template<typename T>
	T* __unsafe__expand_nothrow_t(T* p_ptr,t_size p_size) throw() {
		return reinterpret_cast<T*>(::_expand(reinterpret_cast<void*>(p_ptr),mul_safe_t<std::bad_alloc,t_size>(p_size, sizeof(T))));
	}

	template<typename T>
	T* __unsafe__expand_t(T* p_ptr,t_size p_size) {
		return new_ptr_check_t( __unsafe__expand_nothrow_t(p_ptr,p_size) );
	}

	template<typename T>
	void free_t(T* p_ptr) throw() {
		if (p_ptr != NULL) {
			__unsafe__in_place_destructor_array_t(p_ptr,msize_t(p_ptr));
			__unsafe__free_t(p_ptr);
		}
	}

	template<typename T>
	T* realloc_t(T * p_ptr,t_size p_size) {
		if (p_ptr == NULL) {
			if (p_size != 0) p_ptr = malloc_t<T>(p_size);
		} else if (p_size == 0) {
			free_t(p_ptr); 
			p_ptr = NULL;
		} else {//p_ptr != NULL, p_size != 0
			t_size old_size = msize_t(p_ptr);
			if (old_size != p_size) {
				if (traits_t<T>::needs_constructor || traits_t<T>::needs_destructor) {
					if (p_size < old_size) {//shrinking, _expand should not fail
						__unsafe__in_place_destructor_array_t(p_ptr+p_size,old_size-p_size);
						__unsafe__expand_t(p_ptr,p_size);
					} else {//expanding, try _expand first
						if (__unsafe__expand_nothrow_t(p_ptr,p_size) != NULL) {
							//success
							try {
								__unsafe__in_place_constructor_array_t(p_ptr + old_size, p_size - old_size);
							} catch(...) {
								__unsafe__expand_nothrow_t(p_ptr,old_size);
								throw;
							}
						} else if (traits_t<T>::realloc_safe) {//fresh malloc + raw memcpy of existing items
							T * newptr = __unsafe__malloc_t<T>(p_size);
							__unsafe__memcpy_t(newptr,p_ptr,old_size);
							try {
								__unsafe__in_place_constructor_array_t(newptr+old_size,p_size-old_size);
							} catch(...) {__unsafe__free_t(newptr); throw;}
							__unsafe__free_t(p_ptr);
							p_ptr = newptr;
						} else {//attempt fresh malloc
							T * newptr = malloc_copy_t<T>(p_size,p_ptr,old_size);
							free_t(p_ptr);
							p_ptr = newptr;
						}
					}
				} else {
					p_ptr = __unsafe__realloc_t(p_ptr,p_size);
				}
			}
		}
		return p_ptr;
	}

	template<typename T, typename t_val>
	T* malloc_fill_t(t_size p_size,const t_val & p_val) {
		T* ptr = malloc_t<T>(p_size);
		memset_t<T,t_val>(ptr,p_val,p_size);
		return ptr;
	}

	template<typename T>
	T* malloc_fill_t(t_size p_size) {
		T* ptr = malloc_t<T>(p_size);
		memset_null_t(ptr,p_size);
		return ptr;
	}

	template<typename T>
	int sgn_t(const T & p_val) {
		if (p_val < 0) return -1;
		else if (p_val > 0) return 1;
		else return 0;
	}

	template<typename T> const T* empty_string_t();

	template<> inline const char * empty_string_t<char>() {return "";}
	template<> inline const wchar_t * empty_string_t<wchar_t>() {return L"";}


	template<typename t_type,typename t_newval>
	t_type replace_t(t_type & p_var,const t_newval & p_newval) {
		t_type oldval = p_var;
		p_var = p_newval;
		return oldval;
	}

	template<bool p_val> class __static_assert_switcher_t;
	template<> class __static_assert_switcher_t<true> {
	public:
		typedef void t_assert_failed;
	};

	//depreacted
	template<bool p_val> typename __static_assert_switcher_t<p_val>::t_assert_failed static_assert_t() {}

	template<bool p_val> typename __static_assert_switcher_t<p_val>::t_assert_failed static_assert() {}

	template<typename t_type>
	void assert_raw_type() {static_assert_t< !traits_t<t_type>::needs_constructor && !traits_t<t_type>::needs_destructor >();}

	template<t_size p_size_pow2>
	inline bool is_ptr_aligned_t(const void * p_ptr) {
		static_assert_t< (p_size_pow2 & (p_size_pow2 - 1)) == 0 >();
		return ( ((t_size)p_ptr) & (p_size_pow2-1) ) == 0;
	}


	template<typename t_array>
	void array_rangecheck_t(const t_array & p_array,t_size p_index) {
		if (p_index >= pfc::array_size_t(p_array)) throw pfc::exception_overflow();
	}

	template<typename t_array>
	void array_rangecheck_t(const t_array & p_array,t_size p_from,t_size p_to) {
		if (p_from > p_to) throw pfc::exception_overflow();
		array_rangecheck_t(p_array,p_from); array_rangecheck_t(p_array,p_to);
	}

};


