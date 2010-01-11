/* gzio.c -- IO on .gz files
 * Copyright (C) 1995-1998 Jean-loup Gailly.
 * For conditions of distribution and use, see copyright notice in zlib.h
 *
 * Compile this file with -DNO_DEFLATE to avoid the compression code.
 */

/* @(#) $Id$ */

#include "reader_hacks.h"


extern "C"
{
#include "../../../zlib/zutil.h"
}

#define EOF (-1)


#ifndef Z_BUFSIZE
#  ifdef MAXSEG_64K
#    define Z_BUFSIZE 4096 /* minimize memory usage for 16-bit DOS */
#  else
#    define Z_BUFSIZE 16384
#  endif
#endif
#ifndef Z_PRINTF_BUFSIZE
#  define Z_PRINTF_BUFSIZE 4096
#endif

#define ALLOC(size) malloc(size)
#define TRYFREE(p) {if (p) free(p);}

static int gz_magic[2] = {0x1f, 0x8b}; /* gzip magic header */

/* gzip flag byte */
#define ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define HEAD_CRC     0x02 /* bit 1 set: header CRC present */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */
#define RESERVED     0xE0 /* bits 5..7: reserved */

typedef struct gz_stream {
    z_stream stream;
    int      z_err;   /* error code for last stream operation */
    int      z_eof;   /* set if end of input file */
    WFILE     *file;   /* .gz file */
    Byte     *inbuf;  /* input buffer */
    Byte     *outbuf; /* output buffer */
    uLong    crc;     /* crc32 of uncompressed data */
    int      transparent; /* 1 if input file is not a .gz file */
//    char     mode;    /* 'w' or 'r' */
    long     startpos; /* start of compressed data in file (header skipped) */
} gz_stream;


local gzFile gz_open      OF((WFILE * f));
local int do_flush        OF((gzFile file, int flush));
local int    get_byte     OF((gz_stream *s));
local void   check_header OF((gz_stream *s));
local int    destroy      OF((gz_stream *s));
local uLong  getLong      OF((gz_stream *s));

/* ===========================================================================
     Opens a gzip (.gz) file for reading or writing. The mode parameter
   is as in fopen ("rb" or "wb"). The file is given either by file descriptor
   or path name (if fd == -1).
     gz_open return NULL if the file could not be opened or if there was
   insufficient memory to allocate the (de)compression state; errno
   can be checked to distinguish the two cases (if errno is zero, the
   zlib error is Z_MEM_ERROR).
*/
local gzFile gz_open (WFILE * file)
{
    int err;
    int level = Z_DEFAULT_COMPRESSION; /* compression level */
    int strategy = Z_DEFAULT_STRATEGY; /* compression strategy */
    gz_stream *s;
    char fmode[80]; /* copy of mode, without the compression level */
    char *m = fmode;


    s = (gz_stream *)ALLOC(sizeof(gz_stream));
    if (!s) return Z_NULL;

    s->stream.zalloc = (alloc_func)0;
    s->stream.zfree = (free_func)0;
    s->stream.opaque = (voidpf)0;
    s->stream.next_in = s->inbuf = Z_NULL;
    s->stream.next_out = s->outbuf = Z_NULL;
    s->stream.avail_in = s->stream.avail_out = 0;
    s->file = NULL;
    s->z_err = Z_OK;
    s->z_eof = 0;
    s->crc = crc32(0L, Z_NULL, 0);
    s->transparent = 0;

   
    s->stream.next_in  = s->inbuf = (Byte*)ALLOC(Z_BUFSIZE);

    err = inflateInit2(&(s->stream), -MAX_WBITS);
    /* windowBits is passed < 0 to tell that there is no zlib header.
     * Note that in this case inflate *requires* an extra "dummy" byte
     * after the compressed stream in order to complete decompression and
     * return Z_STREAM_END. Here the gzip CRC32 ensures that 4 bytes are
     * present after the compressed stream.
     */
    if (err != Z_OK || s->inbuf == Z_NULL) {
        return destroy(s), (gzFile)Z_NULL;
    }

    s->stream.avail_out = Z_BUFSIZE;

    errno = 0;
    s->file = file;

	check_header(s); /* skip the .gz header */
	s->startpos = (wftell(s->file) - s->stream.avail_in);
    
    return (gzFile)s;
}

/* ===========================================================================
     Opens a gzip (.gz) file for reading or writing.
*/
static gzFile gzopen (WFILE * f)
{
    return gz_open (f);
}


/* ===========================================================================
     Read a byte from a gz_stream; update next_in and avail_in. Return EOF
   for end of file.
   IN assertion: the stream s has been sucessfully opened for reading.
*/
local int get_byte(gz_stream *s)
{
    if (s->z_eof) return EOF;
    if (s->stream.avail_in == 0) {
	errno = 0;
	s->stream.avail_in = wfread(s->inbuf, 1, Z_BUFSIZE, s->file);
	if (s->stream.avail_in == 0) {
	    s->z_eof = 1;
	    return EOF;
	}
	s->stream.next_in = s->inbuf;
    }
    s->stream.avail_in--;
    return *(s->stream.next_in)++;
}

/* ===========================================================================
      Check the gzip header of a gz_stream opened for reading. Set the stream
    mode to transparent if the gzip magic header is not present; set s->err
    to Z_DATA_ERROR if the magic header is present but the rest of the header
    is incorrect.
    IN assertion: the stream s has already been created sucessfully;
       s->stream.avail_in is zero for the first time, but may be non-zero
       for concatenated .gz files.
*/
local void check_header(gz_stream *s)
{
    int method; /* method byte */
    int flags;  /* flags byte */
    uInt len;
    int c;

    /* Check the gzip magic header */
    for (len = 0; len < 2; len++) {
	c = get_byte(s);
	if (c != gz_magic[len]) {
	    if (len != 0) s->stream.avail_in++, s->stream.next_in--;
	    if (c != EOF) {
		s->stream.avail_in++, s->stream.next_in--;
		s->transparent = 1;
	    }
	    s->z_err = s->stream.avail_in != 0 ? Z_OK : Z_STREAM_END;
	    return;
	}
    }
    method = get_byte(s);
    flags = get_byte(s);
    if (method != Z_DEFLATED || (flags & RESERVED) != 0) {
	s->z_err = Z_DATA_ERROR;
	return;
    }

    /* Discard time, xflags and OS code: */
    for (len = 0; len < 6; len++) (void)get_byte(s);

    if ((flags & EXTRA_FIELD) != 0) { /* skip the extra field */
	len  =  (uInt)get_byte(s);
	len += ((uInt)get_byte(s))<<8;
	/* len is garbage if EOF but the loop below will quit anyway */
	while (len-- != 0 && get_byte(s) != EOF) ;
    }
    if ((flags & ORIG_NAME) != 0) { /* skip the original file name */
	while ((c = get_byte(s)) != 0 && c != EOF) ;
    }
    if ((flags & COMMENT) != 0) {   /* skip the .gz file comment */
	while ((c = get_byte(s)) != 0 && c != EOF) ;
    }
    if ((flags & HEAD_CRC) != 0) {  /* skip the header crc */
	for (len = 0; len < 2; len++) (void)get_byte(s);
    }
    s->z_err = s->z_eof ? Z_DATA_ERROR : Z_OK;
}

 /* ===========================================================================
 * Cleanup then free the given gz_stream. Return a zlib error code.
   Try freeing in the reverse order of allocations.
 */
local int destroy (gz_stream *s)
{
    int err = Z_OK;

    if (!s) return Z_STREAM_ERROR;

    if (s->stream.state != NULL) {
		err = inflateEnd(&(s->stream));
    }
    if (s->z_err < 0) err = s->z_err;

    TRYFREE(s->inbuf);
    TRYFREE(s->outbuf);
    TRYFREE(s);
    return err;
}

/* ===========================================================================
     Reads the given number of uncompressed bytes from the compressed file.
   gzread returns the number of bytes actually read (0 for end of file).
*/
static int ZEXPORT gzread (gzFile file, voidp buf, unsigned len)
{
    gz_stream *s = (gz_stream*)file;
    Bytef *start = (Bytef*)buf; /* starting point for crc computation */
    Byte  *next_out; /* == stream.next_out but not forced far (for MSDOS) */


    if (s->z_err == Z_DATA_ERROR || s->z_err == Z_ERRNO) return -1;
    if (s->z_err == Z_STREAM_END) return 0;  /* EOF */

    next_out = (Byte*)buf;
    s->stream.next_out = (Bytef*)buf;
    s->stream.avail_out = len;

    while (s->stream.avail_out != 0) {

	if (s->transparent) {
	    /* Copy first the lookahead bytes: */
	    uInt n = s->stream.avail_in;
	    if (n > s->stream.avail_out) n = s->stream.avail_out;
	    if (n > 0) {
		zmemcpy(s->stream.next_out, s->stream.next_in, n);
		next_out += n;
		s->stream.next_out = next_out;
		s->stream.next_in   += n;
		s->stream.avail_out -= n;
		s->stream.avail_in  -= n;
	    }
	    if (s->stream.avail_out > 0) {
		s->stream.avail_out -= wfread(next_out, 1, s->stream.avail_out,
					     s->file);
	    }
	    len -= s->stream.avail_out;
	    s->stream.total_in  += (uLong)len;
	    s->stream.total_out += (uLong)len;
            if (len == 0) s->z_eof = 1;
	    return (int)len;
	}
        if (s->stream.avail_in == 0 && !s->z_eof) {

            errno = 0;
            s->stream.avail_in = wfread(s->inbuf, 1, Z_BUFSIZE, s->file);
            if (s->stream.avail_in == 0) {
                s->z_eof = 1;
            }
            s->stream.next_in = s->inbuf;
        }
        s->z_err = inflate(&(s->stream), Z_NO_FLUSH);

	if (s->z_err == Z_STREAM_END) {
	    /* Check CRC and original size */
	    s->crc = crc32(s->crc, start, (uInt)(s->stream.next_out - start));
	    start = s->stream.next_out;

	    if (getLong(s) != s->crc) {
		s->z_err = Z_DATA_ERROR;
	    } else {
	        (void)getLong(s);
                /* The uncompressed length returned by above getlong() may
                 * be different from s->stream.total_out) in case of
		 * concatenated .gz files. Check for such files:
		 */
		check_header(s);
		if (s->z_err == Z_OK) {
		    uLong total_in = s->stream.total_in;
		    uLong total_out = s->stream.total_out;

		    inflateReset(&(s->stream));
		    s->stream.total_in = total_in;
		    s->stream.total_out = total_out;
		    s->crc = crc32(0L, Z_NULL, 0);
		}
	    }
	}
	if (s->z_err != Z_OK || s->z_eof) break;
    }
    s->crc = crc32(s->crc, start, (uInt)(s->stream.next_out - start));

    return (int)(len - s->stream.avail_out);
}


/* ===========================================================================
      Reads one byte from the compressed file. gzgetc returns this byte
   or -1 in case of end of file or error.
*/
static int ZEXPORT gzgetc(gzFile file)
{
    unsigned char c;

    return gzread(file, &c, 1) == 1 ? c : -1;
}



/* ===========================================================================
      Sets the starting position for the next gzread or gzwrite on the given
   compressed file. The offset represents a number of bytes in the
      gzseek returns the resulting offset location as measured in bytes from
   the beginning of the uncompressed stream, or -1 in case of error.
      SEEK_END is not implemented, returns error.
      In this version of the library, gzseek can be extremely slow.
*/
static z_off_t ZEXPORT gzseek (gzFile file, z_off_t offset, int whence)
{
    gz_stream *s = (gz_stream*)file;

    if (s == NULL || whence == SEEK_END ||
	s->z_err == Z_ERRNO || s->z_err == Z_DATA_ERROR) {
	return -1L;
    }
    
    /* compute absolute position */
    if (whence == SEEK_CUR) {
	offset += s->stream.total_out;
    }
    if (offset < 0) return -1L;

    if (s->transparent) {
	/* map to fseek */
	s->stream.avail_in = 0;
	s->stream.next_in = s->inbuf;
        if (wfseek(s->file, offset, SEEK_SET) < 0) return -1L;

	s->stream.total_in = s->stream.total_out = (uLong)offset;
	return offset;
    }

    /* For a negative seek, rewind and use positive seek */
    if ((uLong)offset >= s->stream.total_out) {
	offset -= s->stream.total_out;
    } else if (gzrewind(file) < 0) {
	return -1L;
    }
    /* offset is now the number of bytes to skip. */

    if (offset != 0 && s->outbuf == Z_NULL) {
	s->outbuf = (Byte*)ALLOC(Z_BUFSIZE);
    }
    while (offset > 0)  {
	int size = Z_BUFSIZE;
	if (offset < Z_BUFSIZE) size = (int)offset;

	size = gzread(file, s->outbuf, (uInt)size);
	if (size <= 0) return -1L;
	offset -= size;
    }
    return (z_off_t)s->stream.total_out;
}

/* ===========================================================================
     Rewinds input file. 
*/
static int ZEXPORT gzrewind (gzFile file)
{
    gz_stream *s = (gz_stream*)file;
    

    s->z_err = Z_OK;
    s->z_eof = 0;
    s->stream.avail_in = 0;
    s->stream.next_in = s->inbuf;
    s->crc = crc32(0L, Z_NULL, 0);
	
    if (s->startpos == 0) { /* not a compressed file */
		wfseek(s->file,0,SEEK_SET);
	return 0;
    }

    (void) inflateReset(&s->stream);
    return wfseek(s->file, s->startpos, SEEK_SET);
}

/* ===========================================================================
     Returns the starting position for the next gzread or gzwrite on the
   given compressed file. This position represents a number of bytes in the
   uncompressed data stream.
*/
static z_off_t ZEXPORT gztell (gzFile file)
{
    return gzseek(file, 0L, SEEK_CUR);
}

/* ===========================================================================
     Returns 1 when EOF has previously been detected reading the given
   input stream, otherwise zero.
*/
static int ZEXPORT gzeof (gzFile file)
{
    gz_stream *s = (gz_stream*)file;
    
    return s->z_eof;
}

/* ===========================================================================
   Reads a long in LSB order from the given gz_stream. Sets z_err in case
   of error.
*/
local uLong getLong (gz_stream *s)
{
    uLong x = (uLong)get_byte(s);
    int c;

    x += ((uLong)get_byte(s))<<8;
    x += ((uLong)get_byte(s))<<16;
    c = get_byte(s);
    if (c == EOF) s->z_err = Z_DATA_ERROR;
    x += ((uLong)c)<<24;
    return x;
}

/* ===========================================================================
     Flushes all pending output if necessary, closes the compressed file
   and deallocates all the (de)compression state.
*/
static int ZEXPORT gzclose (gzFile file)
{
    gz_stream *s = (gz_stream*)file;

    if (s == NULL) return Z_STREAM_ERROR;

    return destroy((gz_stream*)file);
}

static long gzip_size(WFILE* f)//hack...
{
	long rv=0;
	wfseek((WFILE*)f,-4,SEEK_END);
	wfread(&rv,4,1,(WFILE*)f);
	return rv;
}

int gzip_loadfile(WFILE* file,void * &out_data,int &out_size)
{
    gzFile in;

	out_size=gzip_size(file);
	if (out_size<=0) return 0;
	wfseek(file,0,SEEK_SET);

    in = gzopen(file);

	if (!in) return 0;

	
	out_data=malloc(out_size);

	if (!out_data)
	{
		gzclose(in);
		return 0;
	}

	int rv=1;
	
    if (gzread(in, out_data, out_size)!=out_size)
	{
		free(out_data);
		out_data=0;
		rv=0;
	}

    gzclose(in);

	return rv;
}