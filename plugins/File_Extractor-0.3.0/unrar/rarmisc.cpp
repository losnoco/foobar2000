
// Rar_Extractor 0.3.0. See unrar_license.txt.

#include "rar.hpp"

// CRC

static uint CRCTab [256];

void InitCRC()
{
	if ( !CRCTab [1] )
	{
		for ( int I = 0; I < 256; I++ )
		{
			uint C = I;
			for ( int J = 0; J < 8 ; J++ )
				C = (C & 1) ? (C >> 1) ^ 0xEDB88320L : (C >> 1);
			CRCTab [I] = C;
		}
	}
}

uint CRC( uint crc, void const* in, uint count )
{
	for ( int I = 0; I < count; I++ )
		crc = CRCTab [(((byte*) in) [I] ^ crc) & 0xFF] ^ (crc >> 8);
	return crc;
}

ushort OldCRC( ushort crc, void const* in, uint count )
{
	for ( int I = 0; I < count; I++ )
	{
		crc = (((byte*) in) [I] + crc) & 0xFFFF;
		crc = ((crc << 1) | (crc >> 15)) & 0xFFFF;
	}
	return crc;
}

// BitInput

BitInput::BitInput( Rar_Error_Handler* eh )
{
	error_handler = eh;
	InBuf = (byte*) malloc( MAX_SIZE );
	if ( !InBuf )
		rar_out_of_memory( error_handler );
}

BitInput::~BitInput()
{
	free( InBuf );
}

void BitInput::faddbits(int Bits)
{
	addbits( Bits );
}


unsigned int BitInput::fgetbits()
{
	return getbits();
}

