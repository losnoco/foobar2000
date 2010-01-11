#include "pfc.h"

#include <intrin.h>

void order_helper::g_swap(t_size * data,t_size ptr1,t_size ptr2)
{
	t_size temp = data[ptr1];
	data[ptr1] = data[ptr2];
	data[ptr2] = temp;
}


t_size order_helper::g_find_reverse(const t_size * order,t_size val)
{
	t_size prev = val, next = order[val];
	while(next != val)
	{
		prev = next;
		next = order[next];
	}
	return prev;
}


void order_helper::g_reverse(t_size * order,t_size base,t_size count)
{
	t_size max = count>>1;
	t_size n;
	t_size base2 = base+count-1;
	for(n=0;n<max;n++)
		g_swap(order,base+n,base2-n);
}


void pfc::crash()
{
	__debugbreak();
}
