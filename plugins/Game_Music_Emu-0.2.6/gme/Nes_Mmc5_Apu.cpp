#include "Nes_Mmc5.h"

#include BLARGG_SOURCE_BEGIN

void Nes_Mmc5::write_register( nes_time_t time, nes_addr_t addr, int data )
{
	if ( unsigned( addr - 0x5010 ) < 4 )
	{
		dprintf( "Nes_Mmc5::write_register( %02X, %02X )\n", addr, data );
		return;
	}
	if ( addr == 0x5001 || addr == 0x5005 )
	{
		if ( data & 0x80 ) dprintf( "Nes_Mmc5::write_register( %02X, %02X )\n", addr, data );
		data &= ~0x80;
	}
	Nes_Apu::write_register( time, addr - 0x1000, data );
}