#include "Hes_Emu.h"

#include "hes/emu.h"
#include "hes/hes.h"

#include <string.h>
#include <stdio.h>

#include "blargg_endian.h"

#include BLARGG_SOURCE_BEGIN

Hes_Emu::Hes_Emu( double gain_ )
{
	hes = NULL;
	gain = gain_;
}

Hes_Emu::~Hes_Emu()
{
	unload();
}

void Hes_Emu::unload()
{
	if ( hes )
	{
		FESTAHES_Close( ( FESTALON_HES * ) hes );
		hes = NULL;
	}
}

blargg_err_t Hes_Emu::init_sound()
{
	voice_count_ = 6;
	
	synth.volume( 0.6 / voice_count_ );
	
	return blargg_success;
}

void Hes_Emu::update_eq( blip_eq_t const& eq )
{
	synth.treble_eq( eq );
}

blargg_err_t Hes_Emu::load( const header_t& h, Emu_Reader& in )
{
	unload();
	
	// check compatibility
	if ( 0 != memcmp( h.tag, "HESM", 4 ) )
		return "Not a HES file";
	if ( h.vers != 0 )
		return "Unsupported HES version";
	
	blargg_err_t err = init_sound();
	if ( err )
		return err;

	unsigned rom_size = sizeof( h ) + in.remain();
	BOOST::uint8_t * rom = new BOOST::uint8_t [ rom_size ];
	if ( ! rom )
		return "Out of memory";
	memcpy( rom, & h, sizeof( h ) );
	err = in.read( rom + sizeof( h ), in.remain() );
	if ( err )
		return err;

	hes = FESTAHES_Load( rom, rom_size );
	delete [] rom;
	
	( ( FESTALON_HES * ) hes )->psg.blip_synth = & synth;
	
	return setup_buffer( 1789772.7272 * 4 + 0.5 );
}

void Hes_Emu::set_voice( int i, Blip_Buffer* center, Blip_Buffer* left, Blip_Buffer* right )
{
	require( hes );
	
	if ( hes )
	{
		if ( ! left && ! right )
		{
			left = center;
			right = center;
		}
		( ( FESTALON_HES * ) hes )->psg.channel[ i ].blip_buffer[ 0 ] = left;
		( ( FESTALON_HES * ) hes )->psg.channel[ i ].blip_buffer[ 1 ] = right;
	}
}

blargg_err_t Hes_Emu::start_track( int track )
{
	require( hes ); // file must be loaded
	
	starting_track();
	
	FESTAHES_SongControl( ( FESTALON_HES * ) hes, track );
	
	return blargg_success;
}

blip_time_t Hes_Emu::run( int msec, bool* stereo_used )
{
	if ( stereo_used ) * stereo_used = true;
	return FESTAHES_Emulate( ( FESTALON_HES * ) hes );
}

