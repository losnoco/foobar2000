#include "pfc.h"


/*
#ifdef _DEBUG
#define USE_GUARD
#endif
*/

#ifdef USE_GUARD

static void * malloc_guarded(unsigned p_size) {
	void * ptr = VirtualAlloc(0,p_size,MEM_COMMIT,PAGE_READWRITE);
	if (ptr != NULL) memset(ptr,0xCC,p_size);
	return ptr;
}
static void free_guarded(void * ptr) {
	VirtualFree(ptr,0,MEM_RELEASE);
}

static void* realloc_guarded(void * p_ptr,unsigned p_size_old,unsigned p_size) {
	void * newptr = malloc_guarded(p_size);
	if (newptr != NULL) {
		memcpy(newptr,p_ptr,pfc::min_t(p_size_old,p_size));
		free_guarded(p_ptr);
	}
	return newptr;
}

// {EFCA6E64-0499-48e2-828B-38E89B232C03}
static const GUID filler_header = 
{ 0xefca6e64, 0x499, 0x48e2, { 0x82, 0x8b, 0x38, 0xe8, 0x9b, 0x23, 0x2c, 0x3 } };
// {1C6CAF60-16EB-4435-BDB5-E2C19DEA3211}
static const GUID filler_footer = 
{ 0x1c6caf60, 0x16eb, 0x4435, { 0xbd, 0xb5, 0xe2, 0xc1, 0x9d, 0xea, 0x32, 0x11 } };


static void validate_block(void * p_buffer,unsigned p_size) {
	assert(memcmp((char*)p_buffer - sizeof(filler_header),&filler_header,sizeof(filler_header)) == 0);
	assert(memcmp((char*)p_buffer + p_size,&filler_footer,sizeof(filler_footer)) == 0);
}

static void free_internal(void * p_buffer,unsigned p_size) {
	validate_block(p_buffer,p_size);
	free_guarded((char*)p_buffer - sizeof(filler_header));
}

static void * malloc_internal(unsigned p_size) {
	void * ptr = malloc_guarded(sizeof(filler_header) + p_size + sizeof(filler_footer));
	if (ptr == NULL) return NULL;
	memcpy(ptr, &filler_header, sizeof(filler_header));
	memcpy((char*) ptr + sizeof(filler_header) + p_size, &filler_footer, sizeof(filler_footer));
	return (char*) ptr + sizeof(filler_header);
}

static void * realloc_internal(void * p_buffer,unsigned p_old_size,unsigned p_size) {
	validate_block(p_buffer,p_old_size);
	void * new_buffer = realloc_guarded((char*)p_buffer - sizeof(filler_header), sizeof(filler_header) + p_old_size+ sizeof(filler_footer), sizeof(filler_header) + p_size + sizeof(filler_footer));
	if (new_buffer == NULL) return NULL;
	memcpy(new_buffer, &filler_header, sizeof(filler_header));
	memcpy((char*) new_buffer + sizeof(filler_header) + p_size, &filler_footer, sizeof(filler_footer));
	return (char*) new_buffer + sizeof(filler_header);
}

#else

static void free_internal(void * p_buffer,unsigned p_size) {
	free(p_buffer);
}

static void * malloc_internal(unsigned p_size) {
	return malloc(p_size);
}

static void * realloc_internal(void * p_buffer,unsigned p_old_size,unsigned p_size) {
	return realloc(p_buffer,p_size);
}

#endif

mem_block::~mem_block() {
	if (data != NULL) free_internal(data,size);
}

void mem_block::force_reset() {if (data != NULL) free_internal(data,size);data=0;size=0;used=0;}

bool mem_block::set_size(unsigned new_used)
{
	if (new_used==0)
	{
		if (mem_logic != ALLOC_FAST_DONTGODOWN)
		{
			if (data!=0) {free_internal(data,size);data=0;}
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
			if (data == NULL)
			{
				data = malloc_internal(new_size);
				if (data == 0)
				{
					size = used = 0;
					return false;
				}
			}
			else
			{
				void * new_data;
				new_data = realloc_internal(data,size,new_size);
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

bool mem_block::prealloc(unsigned num)
{
	if (size<num && mem_logic==ALLOC_FAST_DONTGODOWN)
	{
		unsigned old_used = used;
		if (!set_size(num)) return false;
		used = old_used;
		return true;
	}
	else return true;
}

bool mem_block::copy(const void *ptr, unsigned bytes,unsigned start)
{
	if (!check_size(bytes+start)) return false;

	if (ptr) 
		memcpy((char*)get_ptr()+start,ptr,bytes);
	else 
		memset((char*)get_ptr()+start,0,bytes);
	
	return true;
}



#ifdef USE_GUARD

void *__cdecl operator new(
   size_t count
   ) {
	void * ret = malloc_guarded(count);
	if (ret == NULL) throw std::bad_alloc();
	return ret;
}
void *__cdecl operator new(
   size_t count, 
   const std::nothrow_t&
   ) throw() {
	return malloc_guarded(count);
}

void *__cdecl operator new[](
   size_t count
   ) {
	void * ret = malloc_guarded(count);
	if (ret == NULL) throw std::bad_alloc();
	return ret;
}

void *__cdecl operator new[](
   size_t count, 
   const std::nothrow_t&
   ) throw() {
	return malloc_guarded(count);
}

void operator delete(
   void* ptr
   ) throw( ) {
   free_guarded(ptr);
}

void operator delete(
   void* ptr,
   const std::nothrow_t&
   ) throw( ) {
	free_guarded(ptr);
}

void operator delete[](
   void* ptr
   ) throw( ) {
   free_guarded(ptr);
}

void operator delete[](
   void* ptr,
   const std::nothrow_t&
   ) throw( ) {
	free_guarded(ptr);
}


#endif