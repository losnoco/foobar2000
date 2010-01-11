#ifndef _PFC_BIT_ARRAY_H_
#define _PFC_BIT_ARRAY_H_

class NOVTABLE bit_array {
public:
	virtual bool get(t_size n) const = 0;
	virtual t_size find(bool val,t_size start,t_ssize count) const//can be overridden for improved speed; returns first occurance of val between start and start+count (excluding start+count), or start+count if not found; count may be negative if we're searching back
	{
		t_ssize d, todo, ptr = start;
		if (count==0) return start;
		else if (count<0) {d = -1; todo = -count;}
		else {d = 1; todo = count;}
		while(todo>0 && get(ptr)!=val) {ptr+=d;todo--;}
		return ptr;
	}
	inline bool operator[](t_size n) const {return get(n);}

	t_size calc_count(bool val,t_size start,t_size count,t_size count_max = ~0) const//counts number of vals for start<=n<start+count
	{
		t_size found = 0;
		t_size max = start+count;
		t_size ptr;
		for(ptr=find(val,start,count);found<count_max && ptr<max;ptr=find(val,ptr+1,max-ptr-1)) found++;
		return found;
	}

	inline t_size find_first(bool val,t_size start,t_size max) const {return find(val,start,max-start);}
	inline t_size find_next(bool val,t_size previous,t_size max) const {return find(val,previous+1,max-(previous+1));}
	//for(n = mask.find_first(true,0,m); n < m; n = mask.find_next(true,n,m) )
protected:
	bit_array() {}
	~bit_array() {}
};

class NOVTABLE bit_array_var : public bit_array {
public:
	virtual void set(t_size n,bool val)=0;
protected:
	bit_array_var() {}
	~bit_array_var() {}
};


template<class T>
class bit_array_table_t : public bit_array
{
	const T * data;
	t_size count;
	bool after;
public:
	inline bit_array_table_t(const T * p_data,t_size p_count,bool p_after = false)
		: data(p_data), count(p_count), after(p_after)
	{
	}

	bool get(t_size n) const
	{
		if (n<count) return !!data[n];
		else return after;
	}
};

template<class T>
class bit_array_var_table_t : public bit_array_var
{
	T * data;
	t_size count;
	bool after;
public:
	inline bit_array_var_table_t(T * p_data,t_size p_count,bool p_after = false)
		: data(p_data), count(p_count), after(p_after)
	{
	}

	bool get(t_size n) const {
		if (n<count) return !!data[n];
		else return after;
	}

	void set(t_size n,bool val) {
		if (n<count) data[n] = !!val;
	}
};


typedef bit_array_table_t<bool> bit_array_table;
typedef bit_array_var_table_t<bool> bit_array_var_table;

class bit_array_range : public bit_array
{
	t_size begin,end;
	bool state;
public:
	bit_array_range(t_size first,t_size count,bool p_state = true) : begin(first), end(first+count), state(p_state) {}
	
	bool get(t_size n) const
	{
		bool rv = n>=begin && n<end;
		if (!state) rv = !rv;
		return rv;
	}
};

class bit_array_and : public bit_array
{
	const bit_array & a1, & a2;
public:
	bit_array_and(const bit_array & p_a1, const bit_array & p_a2) : a1(p_a1), a2(p_a2) {}
	bool get(t_size n) const {return a1.get(n) && a2.get(n);}
};

class bit_array_or : public bit_array
{
	const bit_array & a1, & a2;
public:
	bit_array_or(const bit_array & p_a1, const bit_array & p_a2) : a1(p_a1), a2(p_a2) {}
	bool get(t_size n) const {return a1.get(n) || a2.get(n);}
};

class bit_array_xor : public bit_array
{
	const bit_array & a1, & a2;
public:
	bit_array_xor(const bit_array & p_a1, const bit_array & p_a2) : a1(p_a1), a2(p_a2) {}
	bool get(t_size n) const
	{
		bool v1 = a1.get(n), v2 = a2.get(n);
		return (v1 && !v2) || (!v1 && v2);
	}
};

class bit_array_not : public bit_array
{
	const bit_array & a1;
public:
	bit_array_not(const bit_array & p_a1) : a1(p_a1) {}
	bool get(t_size n) const {return !a1.get(n);}
	t_size find(bool val,t_size start,t_ssize count) const
	{return a1.find(!val,start,count);}

};

class bit_array_true : public bit_array
{
public:
	bool get(t_size n) const {return true;}
	t_size find(bool val,t_size start,t_ssize count) const
	{return val ? start : start+count;}
};

class bit_array_false : public bit_array
{
public:
	bool get(t_size n) const {return false;}
	t_size find(bool val,t_size start,t_ssize count) const
	{return val ? start+count : start;}
};

class bit_array_val : public bit_array
{
	bool val;
public:
	bit_array_val(bool v) : val(v) {}
	bool get(t_size n) const {return val;}
	t_size find(bool p_val,t_size start,t_ssize count) const
	{return val==p_val ? start : start+count;}
};

class bit_array_one : public bit_array
{
	t_size val;
public:
	bit_array_one(t_size p_val) : val(p_val) {}
	virtual bool get(t_size n) const {return n==val;}

	virtual t_size find(bool p_val,t_size start,t_ssize count) const
	{
		if (count==0) return start;
		else if (p_val)
		{
			if (count>0)
				return (val>=start && val<start+count) ? val : start+count;
			else
				return (val<=start && val>start+count) ? val : start+count;
		}
		else
		{
			if (start == val) return count>0 ? start+1 : start-1;
			else return start;
		}
	}
};

class bit_array_bittable : public bit_array_var
{
	unsigned char * m_data;
	t_size m_count;
public:
	//helpers
	template<typename t_array>
	inline static bool g_get(const t_array & p_array,t_size idx)
	{
		return !! (p_array[idx>>3] & (1<<(idx&7)));
	}

	template<typename t_array>
	inline static void g_set(t_array & p_array,t_size idx,bool val)
	{
		unsigned char & dst = p_array[idx>>3];
		unsigned char mask = 1<<(idx&7);
		dst = val ? dst|mask : dst&~mask;
	}

	inline static t_size g_estimate_size(t_size p_count) {return (p_count+7)>>3;}

	void resize(t_size p_count)
	{
		t_size old_bytes = g_estimate_size(m_count);
		m_count = p_count;
		t_size bytes = g_estimate_size(m_count);
		if (bytes==0)
		{
			if (m_data != NULL) {pfc::free_t(m_data);m_data=NULL;}
		}
		else
		{
			if (m_data == NULL) {
				m_data = pfc::malloc_t<unsigned char>(bytes);
			} else {
				m_data = pfc::realloc_t(m_data,bytes);
			}
			if (bytes > old_bytes) pfc::memset_t(m_data+old_bytes,(unsigned char)0,bytes-old_bytes);
		}
	}

	bit_array_bittable(t_size p_count) : m_count(0), m_data(NULL) {resize(p_count);}
	~bit_array_bittable() {if (m_data != NULL) pfc::free_t(m_data);}
	
	void set(t_size n,bool val)
	{
		if (n<m_count)
		{
			g_set(m_data,n,val);
		}
	}

	bool get(t_size n) const
	{
		bool rv = false;
		if (n<m_count) {
			rv = g_get(m_data,n);
		}
		return rv;
	}
};


class bit_array_order_changed : public bit_array
{
public:
	bit_array_order_changed(const t_size * p_order) : m_order(p_order) {}
	bool get(t_size n) const
	{
		return m_order[n] != n;
	}

private:
	const t_size * m_order;
};

#define for_each_bit_array(var,mask,val,start,count) for(var = mask.find(val,start,count);var<start+count;var=mask.find(val,var+1,count-(var+1-start)))

#endif