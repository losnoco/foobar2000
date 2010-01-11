#ifndef _PFC_MEM_BLOCK_H_
#define _PFC_MEM_BLOCK_H_


template<class T>
class mem_ops
{
public:

	static void copy(T* dst,const T* src,unsigned count) {memcpy(dst,src,count*sizeof(T));}
	static void move(T* dst,const T* src,unsigned count) {memmove(dst,src,count*sizeof(T));}
	static void set(T* dst,int val,unsigned count) {memset(dst,val,count*sizeof(T));}
	static void setval(T* dst,T val,unsigned count) {for(;count;count--) *(dst++)=val;}
	static T* alloc(unsigned count) {return reinterpret_cast<T*>(malloc(count * sizeof(T)));}
	static T* alloc_zeromem(unsigned count)
	{
		T* ptr = alloc(count);
		if (ptr) set(ptr,0,count);
		return ptr;
	}
	static T* realloc(T* ptr,unsigned count)
	{return ptr ? reinterpret_cast<T*>(::realloc(reinterpret_cast<void*>(ptr),count * sizeof(T))) : alloc(count);}

	static void free(T* ptr) {::free(reinterpret_cast<void*>(ptr)); }

	inline static T make_null_item()
	{
		char item[sizeof(T)];
		memset(&item,0,sizeof(T));
		return * reinterpret_cast<T*>(&item);
	}
};

class mem_block
{
public:
	enum t_mem_logic {ALLOC_DEFAULT,ALLOC_FAST,ALLOC_FAST_DONTGODOWN};
private:
	void * data;
	unsigned size,used;
	t_mem_logic mem_logic;
public:
	inline void set_mem_logic(t_mem_logic v) {mem_logic=v;}
	inline t_mem_logic get_mem_logic() const {return mem_logic;}

	bool prealloc(unsigned size);

	mem_block() : data(0), size(0), used(0), mem_logic(ALLOC_DEFAULT) {}
	mem_block(const mem_block & p_block) : data(0), size(0), used(0), mem_logic(ALLOC_DEFAULT) {*this = p_block;}

	~mem_block();

	unsigned get_size() const {return used;}

	const void * get_ptr() const {return data;}
	void * get_ptr() {return data;}

	bool set_size(unsigned new_used);

	void set_size_e(unsigned p_size) {
		if (!set_size(p_size)) throw std::bad_alloc();
	}

	bool check_size(unsigned new_size)
	{
		if (used<new_size) return set_size(new_size);
		else return true;
	}

	void check_size_e(unsigned p_size) {
		if (!check_size(p_size)) throw std::bad_alloc();
	}

	operator const void * () const {return get_ptr();}
	operator void * () {return get_ptr();}

	bool copy(const void *ptr, unsigned bytes,unsigned start=0);

	bool append(const void *ptr, unsigned bytes) {return copy(ptr,bytes,used);}

	void zeromemory() {memset(get_ptr(),0,used);}

	void force_reset();

	bool set_data(const void * src,unsigned size)
	{
		if (!set_size(size)) return false;
		memcpy(get_ptr(),src,size);
		return true;
	}

	inline const mem_block & operator=(const mem_block & src) {
		if (!set_data(src.get_ptr(),src.get_size()))
			throw std::bad_alloc();
		return *this;
	}

	inline static void g_swap(mem_block & p_item1,mem_block & p_item2)
	{
		pfc::swap_t(p_item1.data,p_item2.data);
		pfc::swap_t(p_item1.size,p_item2.size);
		pfc::swap_t(p_item1.used,p_item2.used);
		pfc::swap_t(p_item1.mem_logic,p_item2.mem_logic);
	}
};

class mem_block_fast : public mem_block
{
public:
	mem_block_fast() {set_mem_logic(ALLOC_FAST);}
	mem_block_fast(const mem_block_fast & p_source) {set_mem_logic(ALLOC_FAST_DONTGODOWN); *this = p_source;}
};

class mem_block_fast_aggressive : public mem_block
{
public:
	mem_block_fast_aggressive() {set_mem_logic(ALLOC_FAST_DONTGODOWN);}
	mem_block_fast_aggressive(const mem_block_fast_aggressive & p_source) {set_mem_logic(ALLOC_FAST_DONTGODOWN); *this = p_source;}
};

namespace pfc {

	template<>
	inline void swap_t(mem_block & p_item1, mem_block & p_item2)
	{
		mem_block::g_swap(p_item1,p_item2);
	}
}

template<class T>
class mem_block_t
{
	mem_block theBlock;
public:
	mem_block_t() {}
	mem_block_t(unsigned p_size) {set_size_e(p_size);}
	mem_block_t(const mem_block_t<T> & p_source) {*this = p_source;}

	void set_mem_logic(mem_block::t_mem_logic v) {theBlock.set_mem_logic(v);}
	mem_block::t_mem_logic get_mem_logic() const {return theBlock.get_mem_logic();}

	unsigned get_size() const {return theBlock.get_size()/sizeof(T);}

	const T* get_ptr() const {return reinterpret_cast<const T*>(theBlock.get_ptr());}
	T* get_ptr() {return reinterpret_cast<T*>(theBlock.get_ptr());}

	bool set_size(unsigned p_size) {return theBlock.set_size(p_size*sizeof(T));}

	void set_size_e(unsigned p_size) {
		if (!set_size(p_size)) throw std::bad_alloc();
	}

	bool check_size(unsigned p_size) {return theBlock.check_size(p_size*sizeof(T));}
	
	void check_size_e(unsigned p_size) {
		if (!check_size(p_size)) throw std::bad_alloc();
	}

	inline operator const T * () const {return get_ptr();}
	inline operator T * () {return get_ptr();}

	inline bool copy(const T* ptr,unsigned size,unsigned start=0) {return theBlock.copy(reinterpret_cast<const void*>(ptr),size*sizeof(T),start*sizeof(T));}
	inline bool append(const T* ptr,unsigned size) {return theBlock.append(reinterpret_cast<const void*>(ptr),size*sizeof(T));}
	inline bool append(T item) {return theBlock.append(reinterpret_cast<const void*>(&item),sizeof(item));}

	inline void swap(unsigned idx1,unsigned idx2)
	{
		T * ptr = get_ptr();
		pfc::swap_t(ptr[idx1],ptr[idx2]);
	}

	bool set_data(const T* src,unsigned count)
	{
		return theBlock.set_data(reinterpret_cast<const void*>(src),count*sizeof(T));
	}

	const mem_block_t<T> & operator=(const mem_block_t<T> & src) {
		if (!set_data(src.get_ptr(),src.get_size()))
			throw std::bad_alloc();
		return *this;
	}

	unsigned write_circular(unsigned offset,const T* src,unsigned count)
	{//returns new offset
		unsigned total = get_size();
		if ((int)offset<0)
		{
			offset = total - ( (-(int)offset)%total );
		}
		else offset%=total;

		if (count>total)
		{
			src += count - total;
			count = total;
		}

		while(count>0)
		{
			unsigned delta = count;
			if (delta > total - offset) delta = total - offset;
			unsigned n;
			for(n=0;n<delta;n++)
				get_ptr()[n+offset] = *(src++);
			count -= delta;
			offset = (offset + delta) % total;
		}
		return offset;
	}

	unsigned read_circular(unsigned offset,T* dst,unsigned count)
	{
		unsigned total = get_size();
		if ((int)offset<0)
		{
			offset = total - ( (-(int)offset)%total );
		}
		else offset%=total;

		while(count>0)
		{
			unsigned delta = count;
			if (delta > total - offset) delta = total - offset;
			unsigned n;
			for(n=0;n<delta;n++)
				*(dst++) = get_ptr()[n+offset];
			count -= delta;
			offset = (offset + delta) % total;
		}
		return offset;
	}

	inline void zeromemory() {theBlock.zeromemory();}

	void fill(T val)
	{
		unsigned n = get_size();
		T * dst = get_ptr();
		for(;n;n--) *(dst++) = val;
	}

	inline void force_reset() {theBlock.force_reset();}

	inline bool prealloc(unsigned size) {return theBlock.prealloc(size*sizeof(T));}

	inline static void g_swap(mem_block_t<T> & p_item1,mem_block_t<T> & p_item2) {pfc::swap_t(p_item1.theBlock,p_item2.theBlock);}
};

namespace pfc {
	template<typename T>
	inline void swap_t(mem_block_t<T> & p_item1, mem_block_t<T> & p_item2)
	{
		mem_block_t<T>::g_swap(p_item1,p_item2);
	}
}

template<class T>
class mem_block_fast_aggressive_t : public mem_block_t<T>
{
public:
	mem_block_fast_aggressive_t() {init();}
	mem_block_fast_aggressive_t(unsigned p_size) {init();this->set_size_e(p_size);}
	mem_block_fast_aggressive_t(const mem_block_fast_aggressive_t<T> & p_source) {init();*this=p_source;}
private:
	void init() {set_mem_logic(mem_block::ALLOC_FAST_DONTGODOWN);}
};

template<class T>
class mem_block_fast_t : public mem_block_t<T>
{
public:
	mem_block_fast_t() {init();}
	mem_block_fast_t(unsigned p_size) {init();this->set_size_e(p_size);}
	mem_block_fast_t(const mem_block_fast_t<T> & p_source) {init();*this=p_source;}
private:
	inline void init() {set_mem_logic(mem_block::ALLOC_FAST);}
};


#if 0

#define mem_block_aligned_t mem_block_t

#else

template<class T>
class mem_block_aligned_t
{
	mem_block data;
	T * ptr_aligned;
public:
	mem_block_aligned_t() {ptr_aligned = 0;}
	mem_block_aligned_t(unsigned size) {ptr_aligned = 0;set_size_e(size);}
	mem_block_aligned_t(const mem_block_aligned_t<T> & p_source) {ptr_aligned = 0; *this = p_source;}

	const mem_block_aligned_t<T> & operator=(const mem_block_aligned_t<T> & p_source)
	{
		if (!set_data(p_source.get_ptr(),p_source.get_size()))
			throw std::bad_alloc();
		return *this;
	}

	unsigned get_size() const {return data.get_size()/sizeof(T);}

	inline void set_mem_logic(mem_block::t_mem_logic v) {data.set_mem_logic(v);}
	inline mem_block::t_mem_logic get_mem_logic() const {return data.get_mem_logic();}

	bool set_size(unsigned size)
	{
		unsigned size_old = get_size();
		int delta_old = (unsigned)ptr_aligned - (unsigned)data.get_ptr();
		
		if (!data.set_size( (size+1) * sizeof(T) - 1)) return false;

		unsigned ptr = (unsigned)data.get_ptr(), old_ptr = ptr;
		ptr += sizeof(T) - 1;
		ptr -= ptr % sizeof(T);
		int delta_new = ptr - old_ptr;
		if (delta_new != delta_old)
		{
			unsigned to_move = size_old > size ? size : size_old;
			memmove((char*)ptr,(char*)ptr - (delta_new-delta_old),to_move * sizeof(T));
		}
		ptr_aligned = (T*)ptr;
		return true;
	}

	void set_size_e(unsigned p_size) {
		if (!set_size(p_size)) throw std::bad_alloc();
	}

	bool check_size(unsigned p_size) {
		return p_size > get_size() ? set_size(p_size) : true;
	}

	void check_size_e(unsigned p_size) {
		if (!check_size(p_size)) throw std::bad_alloc();
	}

	void fill(T val)
	{
		unsigned n = get_size();
		T * dst = get_ptr();
		for(;n;n--) *(dst++) = val;
	}

	bool set_data(const T* p_data,unsigned p_size)
	{
		if (!set_size(p_size)) return false;
		mem_ops<T>::copy(get_ptr(),p_data,p_size);
		return true;
	}

	inline void zeromemory() {data.zeromemory();}	
	inline const T * get_ptr() const {return ptr_aligned;}
	inline T * get_ptr() {return ptr_aligned;}
	inline operator const T * () const {return get_ptr();}
	inline operator T * () {return get_ptr();}
};
#endif

#endif