#include "pfc.h"

void order_helper::g_swap(unsigned * data,unsigned ptr1,unsigned ptr2)
{
	unsigned temp = data[ptr1];
	data[ptr1] = data[ptr2];
	data[ptr2] = temp;
}


unsigned order_helper::g_find_reverse(const unsigned * order,unsigned val)
{
	unsigned prev = val, next = order[val];
	while(next != val)
	{
		prev = next;
		next = order[next];
	}
	return prev;
}


void order_helper::g_reverse(unsigned * order,unsigned base,unsigned count)
{
	unsigned max = count>>1;
	unsigned n;
	unsigned base2 = base+count-1;
	for(n=0;n<max;n++)
		g_swap(order,base+n,base2-n);
}

void order_helper::g_fill(unsigned * p_order,const unsigned p_count)
{
	unsigned n; for(n=0;n<p_count;n++) p_order[n] = n;
}

void pfc::crash()
{
	__asm int 3
}
