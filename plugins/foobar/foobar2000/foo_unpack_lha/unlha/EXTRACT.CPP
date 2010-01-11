/***********************************************************
	extract.c -- extract file from archive
***********************************************************/

void CLhaArchive::decode_lzhuf( long original_size, long packed_size, int method )
{
	LzInterface.method = method;
	LzInterface.dicbit = LZHUFF5_DICBIT;
	LzInterface.original = original_size;
	LzInterface.packed = packed_size;
#ifdef LHADEBUG
	Log("Decode_lzhuf: method=%d\n", method);
#endif
	switch (method)
	{
	case LZHUFF0_METHOD_NUM:
	case LARC4_METHOD_NUM:
		{
			char data [LZHEADER_STORAGE];
			int len = (original_size < packed_size) ? original_size : packed_size;
			while ( len )
			{
				int len2 = ( len > sizeof( data ) ) ? sizeof( data ) : len;
				if ( lharead( data, len2, 1 ) == 0 )
					break;
				fwrite_crc( data, len2 );
				len -= len2;
			}
		}
		break;
	case LARC_METHOD_NUM:	 // -lzs-
		LzInterface.dicbit = 11;
		decode( &LzInterface );
		break;
	case LZHUFF1_METHOD_NUM: // -lh1-
	case LZHUFF4_METHOD_NUM: // -lh4-
	case LARC5_METHOD_NUM:   // -lz5-
		LzInterface.dicbit = 12;
		decode( &LzInterface );
		break;
	case LZHUFF6_METHOD_NUM: // -lh6-
		LzInterface.dicbit = 15;
		decode( &LzInterface );
		break;
	case LZHUFF7_METHOD_NUM: // -lh7-
		LzInterface.dicbit = 16;
	default:
		decode( &LzInterface );
	}
}
