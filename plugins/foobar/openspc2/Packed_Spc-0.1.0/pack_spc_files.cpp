
// Simple SPC music file packer using Packed_Spc library

#include "Spc_Packer.h"

#include <stdlib.h>
#include <stdio.h>

static void exit_if_error( const char* str ) {
	if ( str ) {
		fprintf( stderr, "%s\n", str );
		exit( 1 );
	}
}

int main( int argc, char* argv [] )
{
	if ( argc < 5 ) {
		printf( "%s [-e] out_dir in_dir track1.spc track2.spc [trackn.spc...] "
				"# Pack SPC files from in_dir into out_dir\n", argv [0] );
		printf( "\t-e # Preserve echo buffer (might result in larger files)\n" );
		return EXIT_FAILURE;
	}
	
	int arg = 1;
	
	static Spc_Packer packer; // static to avoid stack overflow
	
	if ( argv [arg] [0] == '-' ) {
		char option = argv [arg++] [1];
		if ( option == 'e' ) {
			packer.preserve_echo_buffer( true );
		}
		else {
			exit_if_error( "Invalid option" );
		}
	}
	
	// it would be better to name shared file after game
	exit_if_error( packer.init( "shared.dat", argv [arg++] ) );
	
	const char* in_dir = argv [arg++];
	size_t in_dir_len = strlen(in_dir);
	
	// read
	while ( arg < argc )
	{
		const char* name = argv [arg++];
		const char* short_name = name;

		if (!strnicmp(name, in_dir, in_dir_len)) short_name += in_dir_len;

		printf( "Reading %s\n", short_name );
		fflush( stdout );
		
		static char buf [0x10200]; // static to avoid stack overflow
		char path [256];
		strcpy( path, name );
		FILE* in = fopen( path, "rb" );
		if ( !in )
			exit_if_error( "Couldn't open file" );
		long size = fread( buf, 1, sizeof buf, in );
		if ( !size )
			exit_if_error( "Couldn't read file" );
		if ( size < 0x10180 )
			exit_if_error( "Unexpected end-of-file" );
		fclose( in );
		
		exit_if_error( packer.add_spc( buf, size, short_name ) );
	}
	printf( "\n" );
	
	// pack
	const char* name;
	while ( (name = packer.next_file()) != 0 ) {
		printf( "Packing %s\n", name );
		fflush( stdout );
		exit_if_error( packer.pack_file() );
	}
	printf( "\n" );
	
	// shared
	printf( "Writing shared data\n" );
	exit_if_error( packer.write_shared() );
}

