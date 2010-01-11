#ifndef ___PFC_H___
#define ___PFC_H___

#if !defined(_WINDOWS) && (defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64) || defined(_WIN32_WCE))
#define _WINDOWS
#endif


#define PFC_DLL_EXPORT


#if defined(_WIN32) || defined(_WIN32_WCE)
#pragma warning(disable:4996)
#ifndef STRICT
#define STRICT
#endif

#ifndef _SYS_GUID_OPERATOR_EQ_
#define _NO_SYS_GUID_OPERATOR_EQ_	//fix retarded warning with operator== on GUID returning int
#endif
#include <windows.h>

#ifndef _SYS_GUID_OPERATOR_EQ_
__inline bool __InlineIsEqualGUID(REFGUID rguid1, REFGUID rguid2)
{
   return (
      ((unsigned long *) &rguid1)[0] == ((unsigned long *) &rguid2)[0] &&
      ((unsigned long *) &rguid1)[1] == ((unsigned long *) &rguid2)[1] &&
      ((unsigned long *) &rguid1)[2] == ((unsigned long *) &rguid2)[2] &&
      ((unsigned long *) &rguid1)[3] == ((unsigned long *) &rguid2)[3]);
}

inline bool operator==(REFGUID guidOne, REFGUID guidOther) {return __InlineIsEqualGUID(guidOne,guidOther);}
inline bool operator!=(REFGUID guidOne, REFGUID guidOther) {return !__InlineIsEqualGUID(guidOne,guidOther);}
#endif

#else
#error Only win32 target supported.
#endif



#define PFC_MEMORY_SPACE_LIMIT ((t_uint64)1<<(sizeof(void*)*8-1))

#define PFC_ALLOCA_LIMIT (4096)

#define INDEX_INVALID ((unsigned)(-1))

#include <exception>
#include <stdexcept>
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

#include "int_types.h"
#include "traits.h"
#include "primitives.h"
#include "alloc.h"
#include "array.h"
#include "bit_array.h"
#include "bsearch_inline.h"
#include "bsearch.h"
#include "sort.h"
#include "list.h"
#include "ptr_list.h"
#include "string_simple.h"
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
#include "instance_tracker.h"
#endif //___PFC_H___