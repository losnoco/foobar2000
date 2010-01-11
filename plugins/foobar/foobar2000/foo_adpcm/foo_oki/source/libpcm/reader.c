#include "libpcm.h"
#include "memman.h"
#include "mbstr.h"

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#include <windows.h>

typedef struct
{
	libpcm_IReader vif;
	volatile uint32_t refcount;
	HANDLE hFile;
	uint32_t size;
	char path[1];
} STREAM_READER;

LIBPCM_INLINE static int reader_open(STREAM_READER *s, const char *path)
{
	DWORD hi;
	s->hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (s->hFile == INVALID_HANDLE_VALUE)
	{
		return 0;
	}
	s->size = GetFileSize(s->hFile, &hi);
	if (hi) s->size = 0xffffffffUL;
	return 1;
}
static void LIBPCM_CALLBACK reader_seek(STREAM_READER *s, uint32_t bytes)
{
	if (s->hFile != INVALID_HANDLE_VALUE)
	{
		LONG hi = 0;
		SetFilePointer(s->hFile, bytes, &hi, FILE_BEGIN);
	}
}
static uint_t LIBPCM_CALLBACK reader_read(STREAM_READER *s, void *buf, uint_t nbytes)
{
	DWORD nbr = 0;
	if (s->hFile != INVALID_HANDLE_VALUE)
	{
		if (!ReadFile(s->hFile, buf, nbytes, &nbr, NULL)) return 0;
	}
	return nbr;
}
LIBPCM_INLINE static void reader_close(STREAM_READER *s)
{
	if (s->hFile != INVALID_HANDLE_VALUE) CloseHandle(s->hFile);
	libpcm_mfree(s);
}

#else

#include <stdio.h>
#include <stdlib.h>

typedef struct
{
	libpcm_IReader vif;
	volatile uint32_t refcount;
	FILE *fp;
	uint32_t size;
	char path[1];
} STREAM_READER;

LIBPCM_INLINE static int reader_open(STREAM_READER *s, const char *path)
{
	s->fp = fopen(path, "rb");
	if (!s->fp)
	{
		return 0;
	}
	fseek(s->fp, 0, SEEK_END);
	s->size = ftell(s->fp);
	fseek(s->fp, 0, SEEK_SET);
	return 1;
}
static void LIBPCM_CALLBACK reader_seek(STREAM_READER *s, uint32_t bytes)
{
	if (s->fp)
		fseek(s->fp, bytes, SEEK_SET);
}
static uint_t LIBPCM_CALLBACK reader_read(STREAM_READER *s, void *buf, uint_t nbytes)
{
	return (s->fp) ? fread(buf, 1, nbytes, s->fp) : 0;
}
LIBPCM_INLINE static void reader_close(STREAM_READER *s)
{
	if (s->fp) fclose(s->fp);
	libpcm_mfree(s);
}
#endif

static uint32_t LIBPCM_CALLBACK reader_size(STREAM_READER *s)
{
	return s->size;
}

static void * LIBPCM_CALLBACK reader_addref(STREAM_READER * s)
{
	s->refcount++;
	return s;
}
static void LIBPCM_CALLBACK reader_release(STREAM_READER * s)
{
	if (--s->refcount == 0)
		reader_close(s);
}
static const char * LIBPCM_CALLBACK reader_path(STREAM_READER * s)
{
	return s->path;
}

static const libpcm_IReaderVtbl FileReaderVtbl =
{
	reader_addref,
	reader_release,
	reader_size,
	reader_read,
	reader_seek,
	reader_path,
};


static libpcm_IReader * libpcm_CreateFileReader(const char *path)
{
	uint_t pathlen = libpcm_strlen(path);
	STREAM_READER *s = libpcm_malloc(sizeof(STREAM_READER) + pathlen);
	if (!s) return 0;
	libpcm_strncpy(s->path, path, pathlen + 1);
	s->vif.lpVtbl = &FileReaderVtbl;
	s->refcount = 1;
	if (!reader_open(s, path))
	{
		libpcm_mfree(s);
		return 0;
	}
	return &s->vif;
}

LIBPCM_DECODER * LIBPCM_API libpcm_open_from_file(const char *path)
{
	LIBPCM_DECODER *d = 0;
	libpcm_IReader *preader = libpcm_CreateFileReader(path);
	if (preader)
	{
		d = libpcm_open_from_stream(preader);
		preader->lpVtbl->Release(preader);
	}
	return d;
}
