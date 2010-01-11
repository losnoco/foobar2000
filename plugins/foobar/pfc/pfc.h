#ifndef ___PFC_H___
#define ___PFC_H___

#if !defined(_WINDOWS) && (defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64) || defined(_WIN32_WCE))
#define _WINDOWS
#endif


#if 0
#ifdef PFC_DLL_EXPORTS
#define PFC_DLL_EXPORT __declspec(dllexport)
#else
#define PFC_DLL_EXPORT __declspec(dllimport)
#endif
#else
#define PFC_DLL_EXPORT
#endif

#if defined(WIN32) || defined(_WIN32_WCE)
#pragma warning(disable:4996)
#ifndef STRICT
#define STRICT
#endif
#include <windows.h>
#else
#error Only win32 target supported.
#endif


#if !defined(_MSC_VER) && !defined(_EVC_VER)
typedef long long t_int64;
typedef unsigned long long t_uint64;
typedef long t_int32;
typedef unsigned long t_uint32;
typedef short t_int16;
typedef unsigned short t_uint16;
typedef char t_int8;
typedef unsigned char t_uint8;
#else
typedef __int64 t_int64;
typedef unsigned __int64 t_uint64;
typedef __int32 t_int32;
typedef unsigned __int32 t_uint32;
typedef __int16 t_int16;
typedef unsigned __int16 t_uint16;
typedef __int8 t_int8;
typedef unsigned __int8 t_uint8;
#endif

typedef int t_int;
typedef unsigned int t_uint;

typedef float t_float32;
typedef double t_float64;


/*
#ifndef QWORD
typedef t_uint64 QWORD;
#endif

#ifndef DWORD
typedef t_uint32 DWORD;
#endif

#ifndef WORD
typedef t_uint16 WORD;
#endif

#ifndef BYTE
typedef t_uint8 BYTE;
#endif
*/

#define PFC_MEMORY_SPACE_LIMIT ((t_uint64)1<<(sizeof(void*)*8))

#define PFC_ALLOCA_LIMIT (4096)

#define INDEX_INVALID ((unsigned)(-1))

#include <exception>
#include <new>

#include <malloc.h>

#include <tchar.h>
#include <stdio.h>

#include <assert.h>

#include <math.h>
#include <float.h>

#ifdef _MSC_VER

#ifdef _DEBUG
#define NOVTABLE
#else
#define NOVTABLE _declspec(novtable)
#endif

#ifdef _DEBUG
#define ASSUME(X) assert(X)
#else
#define ASSUME(X) __assume(X)
#endif

#else

#define NOVTABLE

#define ASSUME(X) assert(X)

#endif

const unsigned infinite = (unsigned)(-1);
const t_uint16 infinite16 = (t_uint16)(-1);
const t_uint32 infinite32 = (t_uint32)(-1);
const t_uint64 infinite64 = (t_uint64)(-1);

#define tabsize(x) (sizeof(x)/sizeof(*x))

template<typename t_ret,typename t_param> 
inline t_ret __safe_cast(t_param p_param) {
	t_ret temp ( p_param );
	return temp;
}

#include "primitives.h"
#include "array.h"
#include "bit_array.h"
#include "bsearch_inline.h"
#include "bsearch.h"
#include "sort.h"
#include "mem_block.h"
#include "list.h"
#include "ptr_list.h"
#include "string.h"
#include "string_fixed.h"
#include "string_list.h"
#include "profiler.h"
#include "guid.h"
#include "byte_order_helper.h"
#include "other.h"
#include "chainlist.h"
#include "ref_counter.h"
#include "rcptr.h"
#include "com_ptr_t.h"
#include "string_conv.h"
#endif //___PFC_H___