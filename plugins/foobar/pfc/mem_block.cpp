#include "pfc.h"


bool mem_block::set_size(unsigned new_used)
{
	if (new_used==0)
	{
		if (mem_logic != ALLOC_FAST_DONTGODOWN)
		{
			if (data!=0) {free(data);data=0;}
			size = 0;
		}
		used = 0;
		return true;
	}
	else
	{
		unsigned new_size;
		if (mem_logic == ALLOC_FAST || mem_logic == ALLOC_FAST_DONTGODOWN)
		{
			if (new_used >= (unsigned)(PFC_MEMORY_SPACE_LIMIT/2))
			{
				new_size = new_used;
			}
			else
			{
				new_size = size;
				if (new_size < 1) new_size = 1;
				while(new_size < new_used) new_size <<= 1; 
				if (mem_logic!=ALLOC_FAST_DONTGODOWN) while(new_size>>1 > new_used) new_size >>= 1;
			}
		}
		else
		{
			new_size = new_used;
		}

		if (new_size!=size)
		{
			if (data==0)
			{
				data = malloc(new_size);
				if (data == 0)
				{
					size = used = 0;
					return false;
				}
			}
			else
			{
				void * new_data;
				new_data = realloc(data,new_size);
				if (new_data==0) return false;
				data = new_data;
			}

			used = new_used;
			size = new_size;
			return true;
		}
		else
		{
			used = new_used;
			return true;
		}
	}
}

void mem_block::prealloc(unsigned num)
{
	if (size<num && mem_logic==ALLOC_FAST_DONTGODOWN)
	{
		int old_used = used;
		set_size(num);
		used = old_used;
	}
}

void* mem_block::copy(const void *ptr, unsigned bytes,unsigned start)
{
	check_size(bytes+start);

	if (ptr) 
		memcpy((char*)get_ptr()+start,ptr,bytes);
	else 
		memset((char*)get_ptr()+start,0,bytes);
	
	return (char*)get_ptr()+start;
}

