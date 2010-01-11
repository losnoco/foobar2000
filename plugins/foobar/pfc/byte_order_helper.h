#ifndef _PFC_BYTE_ORDER_HELPER_
#define _PFC_BYTE_ORDER_HELPER_

namespace byte_order {

#ifdef _M_IX86
#define PFC_BYTE_ORDER_IS_BIG_ENDIAN 0
#endif

#ifdef PFC_BYTE_ORDER_IS_BIG_ENDIAN
#define PFC_BYTE_ORDER_IS_LITTLE_ENDIAN (!(PFC_BYTE_ORDER_IS_BIG_ENDIAN))
#endif
	//if not defined, use machine_is_big_endian() to detect in runtime

#ifndef PFC_BYTE_ORDER_IS_BIG_ENDIAN
	bool machine_is_big_endian();
#else
	inline bool machine_is_big_endian() {return PFC_BYTE_ORDER_IS_BIG_ENDIAN;}
#endif

	inline bool machine_is_little_endian() {return !machine_is_big_endian();}


	void swap_order(void * ptr,unsigned bytes);
	void swap_guid(GUID & g);
	t_uint16 swap_word(t_uint16);
	t_uint32 swap_dword(t_uint32);

#ifdef PFC_BYTE_ORDER_IS_BIG_ENDIAN
#if PFC_BYTE_ORDER_IS_BIG_ENDIAN
	inline void swap_order_if_le(void * ptr,unsigned bytes) {}
	inline void swap_order_if_be(void * ptr,unsigned bytes) {swap_order(ptr,bytes);}
	inline t_uint16 swap_word_if_le(t_uint16 p) {return p;}
	inline t_uint16 swap_word_if_be(t_uint16 p) {return swap_word(p);}
	inline t_uint32 swap_dword_if_le(t_uint32 p) {return p;}
	inline t_uint32 swap_dword_if_be(t_uint32 p) {return swap_dword(p);}
#else
	inline void swap_order_if_le(void * ptr,unsigned bytes) {swap_order(ptr,bytes);}
	inline void swap_order_if_be(void * ptr,unsigned bytes) {}
	inline t_uint16 swap_word_if_le(t_uint16 p) {return swap_word(p);}
	inline t_uint16 swap_word_if_be(t_uint16 p) {return p;}
	inline t_uint32 swap_dword_if_le(t_uint32 p) {return swap_dword(p);}
	inline t_uint32 swap_dword_if_be(t_uint32 p) {return p;}
#endif
#else
	void swap_order_if_le(void * ptr,unsigned bytes);
	void swap_order_if_be(void * ptr,unsigned bytes);
	t_uint16 swap_word_if_le(t_uint16 p);
	t_uint16 swap_word_if_be(t_uint16 p);
	t_uint32 swap_dword_if_le(t_uint32 p);
	t_uint32 swap_dword_if_be(t_uint32 p);
#endif

	inline void order_be_to_native(void * ptr,unsigned bytes) {swap_order_if_le(ptr,bytes);}
	inline void order_le_to_native(void * ptr,unsigned bytes) {swap_order_if_be(ptr,bytes);}
	inline void order_native_to_be(void * ptr,unsigned bytes) {swap_order_if_le(ptr,bytes);}
	inline void order_native_to_le(void * ptr,unsigned bytes) {swap_order_if_be(ptr,bytes);}

	inline t_uint64 swap_qword(t_uint64 param) {swap_order(&param,sizeof(param));return param;}

	inline t_uint64 qword_be_to_native(t_uint64 param) {swap_order_if_le(&param,sizeof(param));return param;}
	inline t_uint64 qword_le_to_native(t_uint64 param) {swap_order_if_be(&param,sizeof(param));return param;}
	inline t_uint64 qword_native_to_be(t_uint64 param) {swap_order_if_le(&param,sizeof(param));return param;}
	inline t_uint64 qword_native_to_le(t_uint64 param) {swap_order_if_be(&param,sizeof(param));return param;}

	inline t_uint32 dword_be_to_native(t_uint32 param) {return swap_dword_if_le(param);}
	inline t_uint32 dword_le_to_native(t_uint32 param) {return swap_dword_if_be(param);}
	inline t_uint32 dword_native_to_be(t_uint32 param) {return swap_dword_if_le(param);}
	inline t_uint32 dword_native_to_le(t_uint32 param) {return swap_dword_if_be(param);}

	inline t_uint16 word_be_to_native(t_uint16 param) {return swap_word_if_le(param);}
	inline t_uint16 word_le_to_native(t_uint16 param) {return swap_word_if_be(param);}
	inline t_uint16 word_native_to_be(t_uint16 param) {return swap_word_if_le(param);}
	inline t_uint16 word_native_to_le(t_uint16 param) {return swap_word_if_be(param);}

	void guid_native_to_le(GUID &param);//blah: GUIDs need byte order fixing too
	void guid_native_to_be(GUID &param);
	void guid_le_to_native(GUID &param);
	void guid_be_to_native(GUID &param);

	template<typename T> inline void order_native_to_le_t(T& param) {order_native_to_le(&param,sizeof(param));}
	template<typename T> inline void order_native_to_be_t(T& param) {order_native_to_be(&param,sizeof(param));}
	template<typename T> inline void order_le_to_native_t(T& param) {order_le_to_native(&param,sizeof(param));}
	template<typename T> inline void order_be_to_native_t(T& param) {order_be_to_native(&param,sizeof(param));}

	template<> inline void order_native_to_le_t<GUID>(GUID& param) {guid_native_to_le(param);}
	template<> inline void order_native_to_be_t<GUID>(GUID& param) {guid_native_to_be(param);}
	template<> inline void order_le_to_native_t<GUID>(GUID& param) {guid_le_to_native(param);}
	template<> inline void order_be_to_native_t<GUID>(GUID& param) {guid_be_to_native(param);}


	template<> inline void order_native_to_le_t<t_uint32>(t_uint32& param) {param = dword_native_to_le(param);}
	template<> inline void order_native_to_be_t<t_uint32>(t_uint32& param) {param = dword_native_to_be(param);}
	template<> inline void order_le_to_native_t<t_uint32>(t_uint32& param) {param = dword_le_to_native(param);}
	template<> inline void order_be_to_native_t<t_uint32>(t_uint32& param) {param = dword_be_to_native(param);}

	template<typename T> inline void swap_t(T & item)
	{
		if (sizeof(T) < 2) {}
		else if (sizeof(T) == 2)
		{
			t_uint8 * ptr = reinterpret_cast<t_uint8*>(&item);
			t_uint8 temp;
			temp = ptr[0];
			ptr[0] = ptr[1];
			ptr[1] = temp;
		}
		else if (sizeof(T) == 4)
		{
			t_uint8 * ptr = reinterpret_cast<t_uint8*>(&item);
			t_uint8 temp;
			temp = ptr[0];
			ptr[0] = ptr[3];
			ptr[3] = temp;
			temp = ptr[1];
			ptr[1] = ptr[2];
			ptr[2] = temp;
		}
		else if (sizeof(T) == 8)
		{
			t_uint8 * ptr = reinterpret_cast<t_uint8*>(&item);
			t_uint8 temp;
			temp = ptr[0];
			ptr[0] = ptr[7];
			ptr[7] = temp;
			temp = ptr[1];
			ptr[1] = ptr[6];
			ptr[6] = temp;
			temp = ptr[2];
			ptr[2] = ptr[5];
			ptr[5] = temp;
			temp = ptr[3];
			ptr[3] = ptr[4];
			ptr[4] = temp;
		}
		else swap_order(&item,sizeof(T));
	}

};

#endif
