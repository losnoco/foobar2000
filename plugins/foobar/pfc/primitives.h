class bit_array;
class bit_array_var;

namespace pfc {

	template<typename T>
	inline void swap_t(T & item1, T & item2)
	{
		T temp(item2);
		item2 = item1;
		item1 = temp;
	}

	template<typename t_array,typename t_filler>
	inline void fill_t(t_array & p_buffer,const unsigned p_count, const t_filler & p_filler)
	{
		unsigned n;
		for(n=0;n<p_count;n++)
			p_buffer[n] = p_filler;
	}

	template<typename t_array,typename t_filler>
	inline void fill_ptr_t(t_array * p_buffer,const unsigned p_count, const t_filler & p_filler)
	{
		unsigned n;
		for(n=0;n<p_count;n++)
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
	inline unsigned append_t(t_array & p_array,const T & p_item)
	{
		unsigned old_count = p_array.get_size();
		p_array.set_size(old_count + 1);
		p_array[old_count] = p_item;
		return old_count;
	}

	template<typename t_array,typename T>
	inline unsigned append_swap_t(t_array & p_array,T & p_item)
	{
		unsigned old_count = p_array.get_size();
		p_array.set_size(old_count + 1);
		swap_t(p_array[old_count],p_item);
		return old_count;
	}

	template<typename t_array,typename T>
	inline unsigned insert_t(t_array & p_array,const T & p_item,unsigned p_index)
	{
		unsigned old_count = p_array.get_size();
		if (p_index > old_count) p_index = old_count;
		p_array.set_size(old_count + 1);
		unsigned n;
		for(n=old_count;n>p_index;n--)
			p_array[n] = p_array[n-1];
		p_array[p_index] = p_item;
		return p_index;
	}

	template<typename t_array,typename T>
	inline unsigned insert_swap_t(t_array & p_array,T & p_item,unsigned p_index)
	{
		unsigned old_count = p_array.get_size();
		if (p_index > old_count) p_index = old_count;
		p_array.set_size(old_count + 1);
		unsigned n;
		for(n=old_count;n>p_index;n--)
			swap_t(p_array[n],p_array[n-1]);
		swap_t(p_array[p_index],p_item);
		return p_index;
	}

	
	template<typename t_array>
	inline unsigned remove_mask_t(t_array & p_array,const bit_array & p_mask)//returns amount of items left
	{
		unsigned n,count = p_array.get_size(), total = 0;

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
	inline T max_t(const T & item1, const T & item2)
	{
		return item1 > item2 ? item1 : item2;
	};

	template<typename T>
	inline T min_t(const T & item1, const T & item2)
	{
		return item1 < item2 ? item1 : item2;
	};

	template<typename T>
	inline T abs_t(T item)
	{
		return item<0 ? -item : item;
	}

	template<typename T>
	inline T clip_t(T p_item, T p_min, T p_max) {
		if (p_item < p_min) return p_min;
		else if (p_item <= p_max) return p_item;
		else return p_max;
	}



	template<typename t_array,typename t_compare>
	unsigned find_duplicates_sorted_t(t_array p_array,unsigned p_count,t_compare p_compare,bit_array_var & p_out)
	{
		unsigned ret = 0;
		unsigned n;
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
	unsigned find_duplicates_sorted_permutation_t(t_array p_array,unsigned p_count,t_compare p_compare,t_permutation p_permutation,bit_array_var & p_out)
	{
		unsigned ret = 0;
		unsigned n;
		if (p_count > 0)
		{
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


	template<typename T>
	T* new_ptr_check_t(T* p_ptr) {
		if (p_ptr == NULL) throw std::bad_alloc();
		return p_ptr;
	}
};
