typedef signed int sint_t;
typedef unsigned int uint_t;
typedef signed short sint16_t;
typedef unsigned short uint16_t;
typedef signed long sint32_t;
typedef unsigned long uint32_t;

#if defined(_MSC_VER)
#define LIBPCM_CDECL __cdecl
#define LIBPCM_API __fastcall
#define LIBPCM_CALLBACK __fastcall
#define LIBPCM_INLINE  __forceinline
typedef signed __int64 sint64_t;
typedef unsigned __int64 uint64_t;
#elif defined(__INTEL_COMPILER)
#define LIBPCM_CDECL __cdecl
#define LIBPCM_API __fastcall
#define LIBPCM_CALLBACK __fastcall
#define LIBPCM_INLINE  __inline
typedef signed __int64 sint64_t;
typedef unsigned __int64 uint64_t;
#elif defined(__BORLANDC__)
#define LIBPCM_CDECL __cdecl
#define LIBPCM_API __fastcall
#define LIBPCM_CALLBACK __fastcall
#define LIBPCM_INLINE  __inline
typedef signed __int64 sint64_t;
typedef unsigned __int64 uint64_t;
#elif defined(__WATCOMC__)
#define LIBPCM_CDECL __cdecl
#define LIBPCM_API __fastcall
#define LIBPCM_CALLBACK __fastcall
#define LIBPCM_INLINE  __inline
typedef signed __int64 sint64_t;
typedef unsigned __int64 uint64_t;
#elif defined(__GNUC__)
#define LIBPCM_CDECL
#define LIBPCM_API
#define LIBPCM_CALLBACK
#define LIBPCM_INLINE __inline__
typedef signed long long sint64_t;
typedef unsigned long long uint64_t;
#elif defined(__DMC__)
#define LIBPCM_CDECL __cdecl
#define LIBPCM_API __fastcall
#define LIBPCM_CALLBACK __fastcall
#define LIBPCM_INLINE __inline
typedef signed long long sint64_t;
typedef unsigned long long uint64_t;
#elif defined(__LCC__)
#define LIBPCM_CDECL
#define LIBPCM_API
#define LIBPCM_CALLBACK
#define LIBPCM_INLINE inline
typedef signed long long sint64_t;
typedef unsigned long long uint64_t;
#else
#define LIBPCM_CDECL
#define LIBPCM_API
#define LIBPCM_CALLBACK
#define LIBPCM_INLINE
/* 誤差に注意 */
typedef double sint64_t;
typedef double uint64_t;
#endif
/*
  libpcm_writeでのバッファ書き込み時のバイトオーダー指定です。
  各ファイルの読み込みは設定に関わらず適切に処理されます。
#define LIBPCM_WORDS_BIGENDIAN
#undef  LIBPCM_WORDS_BIGENDIAN
*/
