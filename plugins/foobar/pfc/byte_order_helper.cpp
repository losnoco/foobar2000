#include "pfc.h"

namespace byte_order {

#ifndef PFC_BYTE_ORDER_IS_BIG_ENDIAN

bool machine_is_big_endian()
{
	t_uint8 temp[4];
	*(t_uint32*)temp = 0xDEADBEEF;
	return temp[0]==0xDE;
}

#endif

void swap_order(void * p_ptr,unsigned bytes)
{
	t_uint8 * ptr = (t_uint8*)p_ptr;
	t_uint8 t;
	unsigned n;
	for(n=0;n<bytes>>1;n++)
	{
		t = ptr[n];
		ptr[n] = ptr[bytes-n-1];
		ptr[bytes-n-1] = t;
	}
}

#if 0
typedef struct _GUID {          // size is 16
    DWORD Data1;
    WORD   Data2;
    WORD   Data3;
    BYTE  Data4[8];
} GUID;
#endif

void swap_guid(GUID & g)
{
	swap_order(&g.Data1,sizeof(g.Data1));
	swap_order(&g.Data2,sizeof(g.Data2));
	swap_order(&g.Data3,sizeof(g.Data3));
}


void guid_native_to_le(GUID &param)
{
	if (machine_is_big_endian()) swap_guid(param);
}

void guid_native_to_be(GUID &param)
{
	if (!machine_is_big_endian()) swap_guid(param);
}

void guid_le_to_native(GUID &param)
{
	if (machine_is_big_endian()) swap_guid(param);
}

void guid_be_to_native(GUID &param)
{
	if (!machine_is_big_endian()) swap_guid(param);
}


t_uint16 swap_word(t_uint16 p)
{
	return ((p&0x00FF)<<8)|((p&0xFF00)>>8);
}

t_uint32 swap_dword(t_uint32 p)
{
	return ((p&0xFF000000)>>24) | ((p&0x00FF0000)>>8) | ((p&0x0000FF00)<<8) | ((p&0x000000FF)<<24);
}


#ifndef PFC_BYTE_ORDER_IS_BIG_ENDIAN
void swap_order_if_le(void * ptr,unsigned bytes)
{
	if (machine_is_little_endian()) swap_order(ptr,bytes);
}
void swap_order_if_be(void * ptr,unsigned bytes)
{
	if (machine_is_big_endian()) swap_order(ptr,bytes);
}
t_uint16 swap_word_if_le(t_uint16 p)
{
	return machine_is_little_endian() ? swap_word(p) : p;
}
t_uint16 swap_word_if_be(t_uint16 p)
{
	return machine_is_big_endian() ? swap_word(p) : p;
}
t_uint32 swap_dword_if_le(t_uint32 p)
{
	return machine_is_little_endian() ? swap_dword(p) : p;
}
t_uint32 swap_dword_if_be(t_uint32 p)
{
	return machine_is_big_endian() ? swap_dword(p) : p;
}
#endif

}