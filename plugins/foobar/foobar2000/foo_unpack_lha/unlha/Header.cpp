/*----------------------------------------------------------------------*/
/*	header.c (from lharc.c)	-- header manipulate functions		*/
/*	Original					by Y.Tagawa	*/
/*	modified  Dec 16 1991				by M.Oki	*/
/*----------------------------------------------------------------------*/

static int calc_sum(char *p, int len)
{
	int sum;
	for (sum = 0; len; len --) sum += *p++;
	return sum & 0xff;
}

int CLhaArchive::get_bytes( char *buf, int len, int size)
{
	int i;

	for ( i = 0; i < len && i < size; i++ )
		buf [i] = get_ptr [i];

	get_ptr += len;
	return i;
}

static time_t wintime_to_unix_stamp( uint32_t a, uint32_t b )
{
    uint64_t t;
    uint64_t epoch = ( ( uint64_t ) 0x019db1de << 32 ) + 0xd53e8000;
                     /* 0x019db1ded53e8000ULL: 1970-01-01 00:00:00 (UTC) */

    t = ( unsigned long ) a;
    t |= ( uint64_t ) ( unsigned long ) b << 32;
    t = ( t - epoch ) / 10000000;
    return t;
}

/*
 * Generic (MS-DOS style) time stamp format (localtime):
 *
 *  31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16
 * |<---- year-1980 --->|<- month ->|<--- day ---->|
 *
 *  15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
 * |<--- hour --->|<---- minute --->|<- second/2 ->|
 *
 */

#include <time.h>

static time_t generic_to_unix_stamp( long t )
{
    struct tm tm;

#define subbits( n, off, len ) ( ( ( n ) >> ( off ) ) & ( ( 1 << ( len ) ) - 1 ) )

    tm.tm_sec  = subbits( t,  0, 5 ) * 2;
    tm.tm_min  = subbits( t,  5, 6 );
    tm.tm_hour = subbits( t, 11, 5 );
    tm.tm_mday = subbits( t, 16, 5 );
    tm.tm_mon  = subbits( t, 21, 4 ) - 1;
    tm.tm_year = subbits( t, 25, 7 ) + 80;

#undef subbits

    tm.tm_isdst = -1;

    return mktime( &tm );
}

/*----------------------------------------------------------------------*/
/*			build header functions										*/
/*----------------------------------------------------------------------*/

ssize_t CLhaArchive::get_extended_header( LzHeader *hdr, size_t header_size, unsigned int *hcrc )
{
	char data [LZHEADER_STORAGE];
	int name_length;
	char dirname [FILENAME_LENGTH];
	int dir_length = 0;
	int i;
	ssize_t whole_size = header_size;
	int ext_type;
	int n = 1 + hdr->size_field_length; /* `ext-type' + `next-header size' */

	if ( hdr->header_level == 0 )
		return 0;

	name_length = strnlen( hdr->name, sizeof( hdr->name ) / sizeof( hdr->name[0] ) );

	while ( header_size )
	{
		setup_get( data );
		if ( sizeof( data ) < header_size )
			return -1;

		if ( lharead( data, header_size, 1 ) == 0 )
			return -1;

		ext_type = get_byte();
		switch ( ext_type )
		{
		case 0:
			/* header crc (CRC-16) */
			hdr->header_crc = get_word();
			/* clear buffer for CRC calculation. */
			data[ 1 ] = data[ 2 ] = 0;
			skip_bytes( header_size - n - 2 );
			break;

		case 1:
			/* filename */
			name_length =
				get_bytes( hdr->name, header_size - n, sizeof( hdr->name ) - 1 );
			hdr->name [name_length] = 0;
			break;

		case 2:
			/* directory */
			dir_length = get_bytes( dirname, header_size - n, sizeof( dirname ) - 1 );
			dirname [dir_length] = 0;
			break;

		case 0x40:
			/* MS-DOS attribute */
			hdr->attribute = ( unsigned char ) get_word();
			break;

		case 0x41:
			/* Windows time stamp (FILETIME structure) */
			/* it is time in 100 nano seconds since 1601-01-01 00:00:00 */

			skip_bytes( 8 ); /* create time is ignored */

			/* set last modified time */
			if (hdr->header_level >= 2)
				skip_bytes( 8 );  /* time_t has been already set */
			else
			{
				uint32_t a, b;
				a = get_longword();
				b = get_longword();
				hdr->unix_last_modified_stamp = wintime_to_unix_stamp( a, b );
			}

			skip_bytes( 8 ); /* last access time is ignored */

			break;

		case 0x50:
			/* UNIX permission */
			hdr->unix_mode = get_word();
			break;

		case 0x51:
			/* UNIX gid and uid */
			hdr->unix_gid = get_word();
			hdr->unix_uid = get_word();
			break;

		case 0x52:
			/* UNIX group name */
			i = get_bytes( hdr->group, header_size - n, sizeof( hdr->group ) - 1 );
			hdr->group [i] = '\0';
			break;

		case 0x53:
			/* UNIX user name */
			i = get_bytes( hdr->user, header_size - n, sizeof( hdr->user ) - 1 );
			hdr->user[i] = '\0';
			break;

		case 0x54:
			/* UNIX last modified time */
			hdr->unix_last_modified_stamp = ( time_t ) get_longword();
			break;

		default:
			/* other headers */
			/* 0x39: multi-disk header
			0x3f: uncompressed comment
			0x42: 64bit large file size
			0x48-0x4f(?): reserved for authenticity verification
			0x7d: encapsulation
			0x7e: extended attribute - platform information
			0x7f: extended attribute - permission, owner-id and timestamp
			(level 3 on OS/2)
			0xc4: compressed comment (dict size: 4096)
			0xc5: compressed comment (dict size: 8192)
			0xc6: compressed comment (dict size: 16384)
			0xc7: compressed comment (dict size: 32768)
			0xc8: compressed comment (dict size: 65536)
			0xd0-0xdf(?): operating systemm specific information
			0xfc: encapsulation (another opinion)
			0xfe: extended attribute - platform information(another opinion)
			0xff: extended attribute - permission, owner-id and timestamp
			(level 3 on UNLHA32) */
			skip_bytes( header_size - n );
			break;
		}

		if ( hcrc )
			*hcrc = calccrc( *hcrc, data, header_size );

		if ( hdr->size_field_length == 2 )
			whole_size += header_size = get_word();
		else
			whole_size += header_size = get_longword();
	}

	/* concatenate dirname and filename */
	if ( dir_length )
	{
		if ( name_length + dir_length >= sizeof( hdr->name ) )
		{
			name_length = sizeof( hdr->name ) - dir_length - 1;
			hdr->name[ name_length ] = 0;
		}
		strcat_s( dirname, hdr->name ); /* ok */
		strcpy_s( hdr->name, dirname ); /* ok */
		name_length += dir_length;
	}

	return whole_size;
}

/*
 * level 0 header
 *
 *
 * offset  size  field name
 * ----------------------------------
 *     0      1  header size    [*1]
 *     1      1  header sum
 *            ---------------------------------------
 *     2      5  method ID                         ^
 *     7      4  packed size    [*2]               |
 *    11      4  original size                     |
 *    15      2  time                              |
 *    17      2  date                              |
 *    19      1  attribute                         | [*1] header size (X+Y+22)
 *    20      1  level (0x00 fixed)                |
 *    21      1  name length                       |
 *    22      X  pathname                          |
 * X +22      2  file crc (CRC-16)                 |
 * X +24      Y  ext-header(old style)             v
 * -------------------------------------------------
 * X+Y+24        data                              ^
 *                 :                               | [*2] packed size
 *                 :                               v
 * -------------------------------------------------
 *
 * ext-header(old style)
 *     0      1  ext-type ('U')
 *     1      1  minor version
 *     2      4  UNIX time
 *     6      2  mode
 *     8      2  uid
 *    10      2  gid
 *
 * attribute (MS-DOS)
 *    bit1  read only
 *    bit2  hidden
 *    bit3  system
 *    bit4  volume label
 *    bit5  directory
 *    bit6  archive bit (need to backup)
 *
 */
BOOL CLhaArchive::get_header_level0( LzHeader *hdr, char *data )
{
	size_t header_size;
	ssize_t extend_size;
	int checksum;
	int name_length;
	int i;

	hdr->size_field_length = 2; /* in bytes */
	hdr->header_size = header_size = get_byte();
	if ( header_size < COMMON_HEADER_SIZE )
		return FALSE;

	checksum = get_byte();

	if ( lharead( data + COMMON_HEADER_SIZE, header_size + 2 - COMMON_HEADER_SIZE, 1 ) == 0 )
		return FALSE;   /* finish */

	if ( calc_sum( data + I_METHOD, header_size ) != checksum )
		return FALSE;

	get_bytes( hdr->method, 5, sizeof( hdr->method ) );
	hdr->packed_size = get_longword();
	hdr->original_size = get_longword();
	hdr->unix_last_modified_stamp = generic_to_unix_stamp( get_longword() );
	hdr->attribute = get_byte(); /* MS-DOS attribute */
	hdr->header_level = get_byte();
	name_length = get_byte();
	i = get_bytes( hdr->name, name_length, sizeof( hdr->name ) - 1 );
	hdr->name [i] = '\0';

	/* defaults for other type */
	hdr->unix_mode = UNIX_FILE_REGULAR | UNIX_RW_RW_RW;
	hdr->unix_gid = 0;
	hdr->unix_uid = 0;

	extend_size = header_size + 2 - name_length - 24;

	if ( extend_size < 0 )
	{
		if ( extend_size == -2 )
		{
			/* CRC field is not given */
			hdr->extend_type = EXTEND_GENERIC;
			hdr->has_crc = false;

			return TRUE;
		} 

		return FALSE;
	}

	hdr->has_crc = true;
	hdr->crc = get_word();

	if ( extend_size == 0 )
		return TRUE;

	hdr->extend_type = get_byte();
	extend_size--;

	if ( hdr->extend_type == EXTEND_UNIX )
	{
		if ( extend_size >= 11 )
		{
			hdr->minor_version = get_byte();
			hdr->unix_last_modified_stamp = ( time_t ) get_longword();
			hdr->unix_mode = get_word();
			hdr->unix_uid = get_word();
			hdr->unix_gid = get_word();
			extend_size -= 11;
		}
		else hdr->extend_type = EXTEND_GENERIC;
	}
	if ( extend_size > 0 )
		skip_bytes( extend_size );

	hdr->header_size += 2;
	return TRUE;
}

/*
 * level 1 header
 *
 *
 * offset   size  field name
 * -----------------------------------
 *     0       1  header size   [*1]
 *     1       1  header sum
 *             -------------------------------------
 *     2       5  method ID                        ^
 *     7       4  skip size     [*2]               |
 *    11       4  original size                    |
 *    15       2  time                             |
 *    17       2  date                             |
 *    19       1  attribute (0x20 fixed)           | [*1] header size (X+Y+25)
 *    20       1  level (0x01 fixed)               |
 *    21       1  name length                      |
 *    22       X  filename                         |
 * X+ 22       2  file crc (CRC-16)                |
 * X+ 24       1  OS ID                            |
 * X +25       Y  ???                              |
 * X+Y+25      2  next-header size                 v
 * -------------------------------------------------
 * X+Y+27      Z  ext-header                       ^
 *                 :                               |
 * -----------------------------------             | [*2] skip size
 * X+Y+Z+27       data                             |
 *                 :                               v
 * -------------------------------------------------
 *
 */
BOOL CLhaArchive::get_header_level1( LzHeader *hdr, char *data )
{
	size_t header_size;
	ssize_t extend_size;
	int checksum;
	int name_length;
	int i, dummy;

	hdr->size_field_length = 2; /* in bytes */
	hdr->header_size = header_size = get_byte();
	if ( header_size < COMMON_HEADER_SIZE )
		return FALSE;

	checksum = get_byte();

	if ( lharead( data + COMMON_HEADER_SIZE, header_size + 2 - COMMON_HEADER_SIZE, 1 ) == 0 )
		return FALSE;   /* finish */

	if ( calc_sum( data + I_METHOD, header_size ) != checksum )
		return FALSE;

	get_bytes( hdr->method, 5, sizeof( hdr->method ) );
	hdr->packed_size = get_longword(); /* skip size */
	hdr->original_size = get_longword();
	hdr->unix_last_modified_stamp = generic_to_unix_stamp( get_longword() );
	hdr->attribute = get_byte(); /* 0x20 fixed */
	hdr->header_level = get_byte();

	name_length = get_byte();
	i = get_bytes( hdr->name, name_length, sizeof( hdr->name ) - 1 );
	hdr->name [i] = '\0';

	/* defaults for other type */
	hdr->unix_mode = UNIX_FILE_REGULAR | UNIX_RW_RW_RW;
	hdr->unix_gid = 0;
	hdr->unix_uid = 0;

	hdr->has_crc = true;
	hdr->crc = get_word();
	hdr->extend_type = get_byte();

	dummy = header_size + 2 - name_length - I_LEVEL1_HEADER_SIZE;
	if ( dummy > 0 )
		skip_bytes( dummy ); /* skip old style extend header */

	extend_size = get_word();
	extend_size = get_extended_header( hdr, extend_size, 0 );
	if ( extend_size == -1 )
		return FALSE;

	/* On level 1 header, size fields should be adjusted. */
	/* the `packed_size' field contains the extended header size. */
	/* the `header_size' field does not. */
	hdr->packed_size -= extend_size;
	hdr->header_size += extend_size + 2;

	return TRUE;
}

/*
 * level 2 header
 *
 *
 * offset   size  field name
 * --------------------------------------------------
 *     0       2  total header size [*1]           ^
 *             -----------------------             |
 *     2       5  method ID                        |
 *     7       4  packed size       [*2]           |
 *    11       4  original size                    |
 *    15       4  time                             |
 *    19       1  RESERVED (0x20 fixed)            | [*1] total header size
 *    20       1  level (0x02 fixed)               |      (X+26+(1))
 *    21       2  file crc (CRC-16)                |
 *    23       1  OS ID                            |
 *    24       2  next-header size                 |
 * -----------------------------------             |
 *    26       X  ext-header                       |
 *                 :                               |
 * -----------------------------------             |
 * X +26      (1) padding                          v
 * -------------------------------------------------
 * X +26+(1)      data                             ^
 *                 :                               | [*2] packed size
 *                 :                               v
 * -------------------------------------------------
 *
 */
BOOL CLhaArchive::get_header_level2( LzHeader *hdr, char *data )
{
	size_t header_size;
	ssize_t extend_size;
	int padding;
	unsigned int hcrc;

	hdr->size_field_length = 2; /* in bytes */
	hdr->header_size = header_size = get_word();
	if ( header_size < I_LEVEL2_HEADER_SIZE )
		return FALSE;

	if ( lharead( data + COMMON_HEADER_SIZE, I_LEVEL2_HEADER_SIZE - COMMON_HEADER_SIZE, 1 ) == 0 )
		return FALSE;   /* finish */

	get_bytes( hdr->method, 5, sizeof( hdr->method ) );
	hdr->packed_size = get_longword();
	hdr->original_size = get_longword();
	hdr->unix_last_modified_stamp = ( time_t ) get_longword();
	hdr->attribute = get_byte(); /* reserved */
	hdr->header_level = get_byte();

	/* defaults for other type */
	hdr->unix_mode = UNIX_FILE_REGULAR | UNIX_RW_RW_RW;
	hdr->unix_gid = 0;
	hdr->unix_uid = 0;

	hdr->has_crc = true;
	hdr->crc = get_word();
	hdr->extend_type = get_byte();
	extend_size = get_word();
	if ( extend_size < 0 || header_size < static_cast<unsigned>( I_LEVEL2_HEADER_SIZE + extend_size ) )
		return FALSE;

	INITIALIZE_CRC( hcrc );
	hcrc = calccrc( hcrc, data, get_ptr - data );

	extend_size = get_extended_header( hdr, extend_size, &hcrc );
	if ( extend_size == -1 )
		return FALSE;

	padding = header_size - I_LEVEL2_HEADER_SIZE - extend_size;
	while ( padding-- )           /* padding should be 0 or 1 */
		hcrc = UPDATE_CRC( hcrc, lhagetc() );

	if ( hdr->header_crc != hcrc )
		return FALSE;

	return TRUE;
}

/*
 * level 3 header
 *
 *
 * offset   size  field name
 * --------------------------------------------------
 *     0       2  size field length (4 fixed)      ^
 *     2       5  method ID                        |
 *     7       4  packed size       [*2]           |
 *    11       4  original size                    |
 *    15       4  time                             |
 *    19       1  RESERVED (0x20 fixed)            | [*1] total header size
 *    20       1  level (0x03 fixed)               |      (X+32)
 *    21       2  file crc (CRC-16)                |
 *    23       1  OS ID                            |
 *    24       4  total header size [*1]           |
 *    28       4  next-header size                 |
 * -----------------------------------             |
 *    32       X  ext-header                       |
 *                 :                               v
 * -------------------------------------------------
 * X +32          data                             ^
 *                 :                               | [*2] packed size
 *                 :                               v
 * -------------------------------------------------
 *
 */
BOOL CLhaArchive::get_header_level3( LzHeader *hdr, char *data )
{
	size_t header_size;
	ssize_t extend_size;
	int padding;
	unsigned int hcrc;

	hdr->size_field_length = get_word();

	if ( lharead( data + COMMON_HEADER_SIZE, I_LEVEL3_HEADER_SIZE - COMMON_HEADER_SIZE, 1 ) == 0 )
		return FALSE;   /* finish */

	get_bytes( hdr->method, 5, sizeof( hdr->method ) );
	hdr->packed_size = get_longword();
	hdr->original_size = get_longword();
	hdr->unix_last_modified_stamp = get_longword();
	hdr->attribute = get_byte(); /* reserved */
	hdr->header_level = get_byte();

	/* defaults for other type */
	hdr->unix_mode = UNIX_FILE_REGULAR | UNIX_RW_RW_RW;
	hdr->unix_gid = 0;
	hdr->unix_uid = 0;

	hdr->has_crc = true;
	hdr->crc = get_word();
	hdr->extend_type = get_byte();
	hdr->header_size = header_size = get_longword();
	extend_size = get_longword();
	if ( extend_size < 0 || header_size < static_cast<unsigned>( I_LEVEL3_HEADER_SIZE + extend_size ) )
		return FALSE;

	INITIALIZE_CRC( hcrc );
	hcrc = calccrc( hcrc, data, get_ptr - data );

	extend_size = get_extended_header( hdr, extend_size, &hcrc );
	if ( extend_size == -1 )
		return FALSE;

	padding = header_size - I_LEVEL3_HEADER_SIZE - extend_size;
	while ( padding-- )           /* padding should be 0 */
		hcrc = UPDATE_CRC( hcrc, lhagetc() );

	if ( hdr->header_crc != hcrc )
		return FALSE;

	return TRUE;
}

BOOL CLhaArchive::get_header( LzHeader *hdr )
{
	char data[LZHEADER_STORAGE];

	int end_mark;

	memset( hdr, 0, sizeof( LzHeader ) );

	setup_get( data );

	if ( ! lharead( &end_mark, 1, 1 ) || end_mark == 0 )
		return FALSE;
	data [0] = end_mark;

	if ( lharead( data + 1, COMMON_HEADER_SIZE - 1, 1 ) == 0 )
		return FALSE;           /* finish */

	switch ( data [I_HEADER_LEVEL] )
	{
	case 0:
		if ( get_header_level0( hdr, data ) == FALSE )
			return FALSE;
		break;

	case 1:
		if ( get_header_level1( hdr, data ) == FALSE )
			return FALSE;
		break;

	case 2:
		if ( get_header_level2( hdr, data ) == FALSE )
			return FALSE;
		break;

	case 3:
		if ( get_header_level3( hdr, data ) == FALSE )
			return FALSE;
		break;

	default:
		return FALSE;
	}

	if ( ( hdr->unix_mode & UNIX_FILE_SYMLINK ) == UNIX_FILE_SYMLINK )
	{
		char *p;
		/* split symbolic link */
		p = strchr( hdr->name, '|' );
		if ( p )
		{
			/* hdr->name is symbolic link name */
			/* hdr->realname is real name */
			*p = 0;
			strcpy_s( hdr->realname, p + 1 ); /* ok */
		}
	}

	return TRUE;
}
