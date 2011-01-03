
// Simple SPC music file recorder using Spc_Emu. Records uncompressed
// SPC file "test.spc" into a sound file named "out.aif".

#include "Spc_Emu.h"

#include <stdlib.h>
#include <stdio.h>

#include "Sound_Writer.hpp"

static void exit_if_error( const char* str ) {
	if ( str ) {
		fprintf( stderr, "Error: %s\n", str );
		exit( EXIT_FAILURE );
	}
}

int main()
{
	// Create emulator
	Spc_Emu* emu = new Spc_Emu;
	if ( !emu )
		exit_if_error( "Out of memory" );
	
	// Read SPC file
	FILE* file = fopen( "test.spc", "rb" );
	if ( !file )
		exit_if_error( "Couldn't open file" );
	fseek( file, 0, SEEK_END );
	long spc_size = ftell( file );
	fseek( file, 0, SEEK_SET );
	char* spc = new char [spc_size];
	if ( !spc )
		exit_if_error( "Out of memory" );
	if ( !fread( spc, spc_size, 1, file ) )
		exit_if_error( "Couldn't read data" );
	fclose( file );
	
	// Load SPC into player
	exit_if_error( emu->load_spc( spc, spc_size ) );
	delete [] spc; // player makes copy of data
	
	// Skip one minute into track
	//emu->skip( 60 * 1000L );
	
	// Play into AIFF file
	Sound_Writer aiff( 32000, "out.aif" );
	aiff.stereo( 1 );
	int const buf_size = 3200 * 2; // 0.1 second buffer
	Spc_Emu::sample_t* buf = new Spc_Emu::sample_t [buf_size];
	for ( int n = 30 * 10; n--; ) {
		int sample_count = emu->play( buf_size, buf );
		if ( !sample_count )
			exit_if_error( "SPC emulation error" );
		aiff.write( buf, sample_count );
	}
	delete [] buf;
	
	delete emu;
	
	return 0;
}

