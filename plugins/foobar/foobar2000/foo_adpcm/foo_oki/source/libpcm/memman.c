#include "libpcm.h"
#include "memman.h"

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#include <windows.h>

#ifdef _DEBUG
#pragma pack(push, 1)
typedef struct BLOCK_tag {
	struct BLOCK_tag *next;
	struct BLOCK_tag *prev;
	int l;
	char *f;
	int s;
	char data[4];
} BLOCK;
#pragma pack(pop)

static BLOCK gurdian = { &gurdian,&gurdian,0,0 };

volatile static int easylock = -1; /* ‹C‹x‚ßƒƒbƒN */
static void libpcm_add(BLOCK *cur)
{
	while (++easylock != 0) easylock--;
	gurdian.l++;
	cur->prev = &gurdian;
	cur->next = gurdian.next;
	cur->prev->next = cur;
	cur->next->prev = cur;
	easylock--;
}
static void libpcm_remove(BLOCK *cur)
{
	BLOCK *next, *prev;
	while (++easylock != 0) easylock--;
	gurdian.l--;
	next = cur->next;
	prev = cur->prev;
	prev->next = next;
	next->prev = prev;
	easylock--;
}
static void * libpcm_malloc_o(uint32_t size)
{
	return HeapAlloc(GetProcessHeap(), 0, size);
}
static void * libpcm_realloc_o(void *p, uint32_t size)
{
	return HeapReAlloc(GetProcessHeap(), 0, p, size);
}
static void libpcm_mfree_o(void *p)
{
	HeapFree(GetProcessHeap(), 0, p);
}
void * LIBPCM_API libpcm_malloc_dbg(uint32_t size, char *f, int l)
{
	BLOCK *cur = libpcm_malloc_o(sizeof(BLOCK) + size);
	if (!cur) return 0;
	cur->f = f;
	cur->l = l;
	cur->s = size;
	libpcm_add(cur);
	return &cur->data[4];	
}
void * LIBPCM_API libpcm_realloc_dbg(void *p, uint32_t size)
{
	char *f;
	int l;
	BLOCK *cur = (BLOCK *)(((char *)p) - sizeof(BLOCK));
	BLOCK *reb;
	f = cur->f;
	l = cur->l;
	libpcm_remove(cur);
	reb = libpcm_realloc_o(cur, sizeof(BLOCK) + size);
	if (!reb)
	{
		libpcm_add(cur);
		return 0;
	}
	reb->f = f;
	reb->l = l;
	reb->s = size;
	libpcm_add(reb);
	return &reb->data[4];
}
void LIBPCM_API libpcm_mfree_dbg(void *p)
{
	BLOCK *cur = (BLOCK *)(((char *)p) - sizeof(BLOCK));
	libpcm_remove(cur);
	libpcm_mfree_o(cur);
}
int LIBPCM_API libpcm_leakcheck_dbg(void)
{
	return gurdian.next != &gurdian;
}
#else
void * LIBPCM_API libpcm_malloc(uint32_t size)
{
	return HeapAlloc(GetProcessHeap(), 0, size);
}
void * LIBPCM_API libpcm_realloc(void *p, uint32_t size)
{
	return HeapReAlloc(GetProcessHeap(), 0, p, size);
}
void LIBPCM_API libpcm_mfree(void *p)
{
	HeapFree(GetProcessHeap(), 0, p);
}
#endif

#else

#include <stdlib.h>
void * LIBPCM_API libpcm_malloc(uint32_t size)
{
	return malloc(size);
}
void * LIBPCM_API libpcm_realloc(void *p, uint32_t size)
{
	return realloc(p, size);
}
void LIBPCM_API libpcm_mfree(void *p)
{
	free(p);
}

#endif
