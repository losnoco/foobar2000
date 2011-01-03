
// Simple SPC music file unpacker using Packed_Spc library

#include "unpack_spc.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "zlib/zlib.h"

static void exit_if_error( const char* str ) {
	if ( str ) {
		fprintf( stderr, "%s\n", str );
		exit( 1 );
	}
}

int main( int argc, char* argv [] )
{
	if ( argc < 5 ) {
		printf( "%s out_dir in_dir track1.spc track2.spc [trackn.spc...] "
				"# Unpack SPC files from in_dir into out_dir\n", argv [0] );
		return EXIT_FAILURE;
	}
	
	int arg = 1;
	
	const char* out_dir = argv [arg++];
	const char* in_dir = argv [arg++];
	
	while ( arg < argc )
	{
		const char* name = argv [arg++];
		char path [256];
		
		// read file
		static char in_buf [0x11000];
		strcpy( path, in_dir );
		strcat( path, name );
		gzFile in = gzopen( path, "rb" );
		if ( !in )
			exit_if_error( "Couldn't open input file" );
		long size = gzread( in, in_buf, sizeof in_buf );
		if ( size <= 0 )
			exit_if_error( "Couldn't read from file" );
		
		long out_size = 0x11000;
		static char out_buf [0x11000];
		if ( !is_packed_spc( in_buf, size ) ) {
			// not packed
			printf( "Copying %s\n", name );
			fflush( stdout );
			
			out_size = size;
			memcpy( out_buf, in_buf, size );
		}
		else {
			// packed
			printf( "Unpacking %s\n", name );
			fflush( stdout );
			
			strcpy( path, in_dir );
			strcat( path, spc_shared_filename( in_buf, size ) );
			exit_if_error( read_packed_spc( in_buf, size, path, out_buf, &out_size ) );
		}
		
		// write output
		strcpy( path, out_dir );
		strcat( path, name );
		FILE* out = fopen( path, "wb" );
		if ( !out )
			exit_if_error( "Couldn't create output file" );
		if ( !fwrite( out_buf, out_size, 1, out ) )
			exit_if_error( "Couldn't write output" );
		fclose( out );
	}
}

