#ifndef UNLHA32_LIB_H
#define UNLHA32_LIB_H

#define SUPPORT_LH7

#include "lha_macro.h"

typedef signed int ssize_t; /* XXX */

#if !defined(_MSC_VER) && !defined(_EVC_VER)
#include <stdint.h>
#else
typedef unsigned __int64 uint64_t;
typedef unsigned __int32 uint32_t;
#endif

typedef struct LzHeader 
{
  size_t			header_size;
  int				size_field_length;
  char				method[5];
  size_t			packed_size;
  size_t			original_size;
  unsigned char		attribute;
  unsigned char		header_level;
  char				name[FILENAME_LENGTH];
  char				realname[FILENAME_LENGTH]; /* real name for a symbolic link */
  unsigned int		crc; /* file CRC */
  bool				has_crc; /* file CRC */
  unsigned int      header_crc; /* header CRC */
  unsigned char		extend_type;
  unsigned char		minor_version;

  /* extend_type == EXTEND_UNIX  and convert from other type. */
  time_t			unix_last_modified_stamp;
  unsigned short	unix_mode;
  unsigned short	unix_uid;
  unsigned short	unix_gid;
  char              user[256];
  char              group[256];
} LzHeader;

typedef struct _LzInterfacing
{
	unsigned long original;
	unsigned long packed;
	int dicbit;
	int method;
} LzInterfacing;



//===============
class CLhaArchive
//===============
{
public:
	enum { MAXMATCH = 256 };	// formerly F (not more than UCHAR_MAX + 1)
	enum { THRESHOLD = 3 };		// choose optimal value
	enum { NC = 255 + MAXMATCH + 2 - THRESHOLD };
	
public:
	CLhaArchive();
	~CLhaArchive();

public:
	/*LPBYTE GetOutputFile() const { return m_lpOutputFile; }
	DWORD GetOutputFileLength() const { return m_dwOutputLen; }*/
	BOOL IsArchive();
	BOOL ExtractFile();

protected:
	/*LPBYTE m_lpStream;		// LHA file data
	DWORD m_dwStreamLen;	// LHA file size
	DWORD m_dwStreamPos;	// LHA file position
	LPBYTE m_lpOutputFile;
	DWORD m_dwOutputLen;*/
	BYTE *m_pDecoderData[2];

protected:
	char *get_ptr;
	unsigned int get_size;
	unsigned short crc, bitbuf;
	unsigned char subbitbuf, bitcount;
	unsigned short crctable[256];
	LzInterfacing LzInterface;
	unsigned int n_max;
	unsigned short total_p;
	int avail, n1, most_p, nn;
	unsigned long count, nextcount, compsize;
	short *child, *parent, *block, *edge, *stock, *node;
	unsigned short *freq;
	unsigned char *buf;
	unsigned short bufsiz, blocksize;
	unsigned int np, pbit;
	void * m_pHufData;
	int flag, flagcnt, matchpos, prev_char;
	unsigned int dicsiz, dicbit, maxmatch, loc;
	unsigned short left[2 * NC - 1], right[2 * NC - 1];

protected:
	void setup_get(char *p, unsigned int size) { get_ptr = p; get_size = size; }
	int get_byte(bool & error) { if (get_size) { get_size--; return (*get_ptr++ & 0xff); } else { error = true; return 0; } }
	int get_bytes( char * buf, int len, int size );
	void skip_bytes( size_t len, bool & error ) { get_ptr += len; if (len > get_size) { get_size = 0; error = true; } else get_size -= len; }
	unsigned short get_word(bool & error) {	int b0, b1;	b0 = get_byte(error); b1 = get_byte(error); return (b1 << 8) + b0; }
	long get_longword(bool & error) { long b0, b1, b2, b3; b0 = get_byte(error); b1 = get_byte(error); b2 = get_byte(error); b3 = get_byte(error); return (b3 << 24) + (b2 << 16) + (b1 << 8) + b0; }

	enum { I_HEADER_SIZE         = 0 };            /* levels 0, 1, 2 */
	enum { I_HEADER_CHECKSUM     = 1 };            /* level 0,1     */
	enum { I_METHOD              = 2 };            /* level 0,1,2,3 */
	enum { I_PACKED_SIZE         = 7 };            /* level 0,1,2,3 */
	enum { I_ATTRIBUTE           = 19 };           /* level 0,1,2,3 */
	enum { I_HEADER_LEVEL        = 20 };           /* level 0,1,2,3 */

	enum { COMMON_HEADER_SIZE    = 21 };           /* size of common part */

	enum { I_GENERIC_HEADER_SIZE = 24 }; /* + name_length */
	enum { I_LEVEL0_HEADER_SIZE  = 36 }; /* + name_length (unix extended) */
	enum { I_LEVEL1_HEADER_SIZE  = 27 }; /* + name_length */
	enum { I_LEVEL2_HEADER_SIZE  = 26 }; /* + padding */
	enum { I_LEVEL3_HEADER_SIZE  = 32 };

	BOOL get_header( LzHeader *hdr );
	BOOL get_header_level0( LzHeader *hdr, char *data );
	BOOL get_header_level1( LzHeader *hdr, char *data );
	BOOL get_header_level2( LzHeader *hdr, char *data );
	BOOL get_header_level3( LzHeader *hdr, char *data );

	ssize_t get_extended_header( LzHeader *hdr, size_t header_size, unsigned int *hcrc );

	VOID make_crctable();
	inline unsigned int calccrc( unsigned int crc, const void *p, int n ) { return calccrc( crc, ( const unsigned char * ) p, n ); }
	unsigned int calccrc( unsigned int crc, const unsigned char *p , int n );
	void init_getbits();
	unsigned short getbits(unsigned int n);
	void fillbuf(unsigned char n);
	virtual int lharead( void *, int, int )=0; // fread equivalent
	int lhagetc() { unsigned char c; if ( lharead( &c, 1, 1 ) == 0 ) return -1; return c; }
	virtual void fwrite_crc( const void *p, int n )=0;
	void decode_lzhuf( long original_size, long packed_size, int method );
	void decode( LzInterfacing *pinterface );
	void make_table( unsigned int nchar, unsigned char *bitlen, unsigned int tablebits, unsigned short *table );

protected:
	// dhuf.cpp
	VOID InitDecodeTables();
	void start_c_dyn();
	void start_p_dyn();
	void decode_start_dyn();
	void reconst(int start, int end);
	int swap_inc(int p);
	void update_c(int p);
	void update_p(int p);
	void make_new_node(int p);
	unsigned short decode_c_dyn();
	unsigned short decode_p_dyn();

protected:
	// huf.cpp
	VOID InitHufTables();
	void decode_start_st1(int dicbit);
	void read_pt_len(short nn, short nbit, short i_special);
	void read_c_len();
	unsigned short decode_c_st1();
	unsigned short decode_p_st1();

protected:
	// shuf.cpp
	void decode_start_st0();
	void decode_start_fix();
	unsigned short decode_c_st0();
	unsigned short decode_p_st0();
	void read_tree_p();
	void read_tree_c();
	void ready_made(int method);

protected:
	// larc.cpp
	unsigned short decode_c_lzs();
	unsigned short decode_p_lzs();
	unsigned short decode_c_lz5();
	unsigned short decode_p_lz5();
	void decode_start_lzs();
	void decode_start_lz5(unsigned char *);
};


#endif // UNLHA32_LIB_H
