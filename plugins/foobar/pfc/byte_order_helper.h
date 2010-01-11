#ifndef _PFC_BYTE_ORDER_HELPER_
#define _PFC_BYTE_ORDER_HELPER_

namespace pfc {
	template<typename T> T byteswap_t(T p_source);

	template<> inline char byteswap_t<char>(char p_source) {return p_source;}
	template<> inline unsigned char byteswap_t<unsigned char>(unsigned char p_source) {return p_source;}

	template<> inline wchar_t byteswap_t<wchar_t>(wchar_t p_source) {return _byteswap_ushort(p_source);}

	template<> inline short byteswap_t<short>(short p_source) {return _byteswap_ushort(p_source);}
	template<> inline unsigned short byteswap_t<unsigned short>(unsigned short p_source) {return _byteswap_ushort(p_source);}

	template<> inline int byteswap_t<int>(int p_source) {return _byteswap_ulong(p_source);}
	template<> inline unsigned int byteswap_t<unsigned int>(unsigned int p_source) {return _byteswap_ulong(p_source);}

	template<> inline long byteswap_t<long>(long p_source) {return _byteswap_ulong(p_source);}
	template<> inline unsigned long byteswap_t<unsigned long>(unsigned long p_source) {return _byteswap_ulong(p_source);}

	template<> inline long long byteswap_t<long long>(long long p_source) {return _byteswap_uint64(p_source);}
	template<> inline unsigned long long byteswap_t<unsigned long long>(unsigned long long p_source) {return _byteswap_uint64(p_source);}

	template<> inline float byteswap_t<float>(float p_source) {
		float ret;
		*(t_uint32*) &ret = byteswap_t(*(const t_uint32*)&p_source );
		return ret;
	}

	template<> inline double byteswap_t<double>(double p_source) {
		double ret;
		*(t_uint64*) &ret = byteswap_t(*(const t_uint64*)&p_source );
		return ret;
	}

	template<> inline GUID byteswap_t<GUID>(GUID p_guid) {
		GUID ret;
		ret.Data1 = pfc::byteswap_t(p_guid.Data1);
		ret.Data2 = pfc::byteswap_t(p_guid.Data2);
		ret.Data3 = pfc::byteswap_t(p_guid.Data3);
		ret.Data4[0] = p_guid.Data4[0];
		ret.Data4[1] = p_guid.Data4[1];
		ret.Data4[2] = p_guid.Data4[2];
		ret.Data4[3] = p_guid.Data4[3];
		ret.Data4[4] = p_guid.Data4[4];
		ret.Data4[5] = p_guid.Data4[5];
		ret.Data4[6] = p_guid.Data4[6];
		ret.Data4[7] = p_guid.Data4[7];
		return ret;
	}


	void byteswap_raw(void * p_buffer,t_size p_bytes);
};

#if defined(_M_IX86) || defined(_M_X64)
#define PFC_BYTE_ORDER_IS_BIG_ENDIAN 0
#endif

#ifdef PFC_BYTE_ORDER_IS_BIG_ENDIAN
#define PFC_BYTE_ORDER_IS_LITTLE_ENDIAN (!(PFC_BYTE_ORDER_IS_BIG_ENDIAN))
#else
#error please update byte order #defines
#endif

namespace pfc {
	enum {
		byte_order_is_big_endian = PFC_BYTE_ORDER_IS_BIG_ENDIAN,
		byte_order_is_little_endian = PFC_BYTE_ORDER_IS_LITTLE_ENDIAN,
	};

	template<typename T> T byteswap_if_be_t(T p_param) {return byte_order_is_big_endian ? byteswap_t(p_param) : p_param;}
	template<typename T> T byteswap_if_le_t(T p_param) {return byte_order_is_little_endian ? byteswap_t(p_param) : p_param;}
}

namespace byte_order {

#if PFC_BYTE_ORDER_IS_BIG_ENDIAN//big endian
	template<typename T> inline void order_native_to_le_t(T& param) {param = pfc::byteswap_t(param);}
	template<typename T> inline void order_native_to_be_t(T& param) {}
	template<typename T> inline void order_le_to_native_t(T& param) {param = pfc::byteswap_t(param);}
	template<typename T> inline void order_be_to_native_t(T& param) {}
#else//little endian
	template<typename T> inline void order_native_to_le_t(T& param) {}
	template<typename T> inline void order_native_to_be_t(T& param) {param = pfc::byteswap_t(param);}
	template<typename T> inline void order_le_to_native_t(T& param) {}
	template<typename T> inline void order_be_to_native_t(T& param) {param = pfc::byteswap_t(param);}
#endif
};

#endif
