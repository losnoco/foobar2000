namespace pfc {

	class traits_default {
	public:
		enum { realloc_safe = false, needs_destructor = true, needs_constructor = true, constructor_may_fail = true};
	};

	class traits_rawobject : public traits_default {
	public:
		enum { realloc_safe = true, needs_destructor = false, needs_constructor = false, constructor_may_fail = false};
	};

	template<typename T> class traits_t : public traits_default {};
	
	template<typename T> class traits_t<T*> : public traits_rawobject {};

	template<> class traits_t<t_uint8> : public traits_rawobject {};
	template<> class traits_t<t_int8> : public traits_rawobject {};
	template<> class traits_t<t_uint16> : public traits_rawobject {};
	template<> class traits_t<t_int16> : public traits_rawobject {};
	template<> class traits_t<t_uint32> : public traits_rawobject {};
	template<> class traits_t<t_int32> : public traits_rawobject {};
	template<> class traits_t<t_uint64> : public traits_rawobject {};
	template<> class traits_t<t_int64> : public traits_rawobject {};
	template<> class traits_t<bool> : public traits_rawobject {};

	template<> class traits_t<t_float32> : public traits_rawobject {};
	template<> class traits_t<t_float64> : public traits_rawobject {};
	
	template<> class traits_t<GUID> : public traits_rawobject {};

	template<typename T,t_size p_count>
	class traits_t<T[p_count]> : public traits_t<T> {};
}