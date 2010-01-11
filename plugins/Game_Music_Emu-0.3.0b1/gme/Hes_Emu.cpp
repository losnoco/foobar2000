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

void Hes_Emu::update_eq( blip_eq_t const& eq )
{
	synth.treble_eq( eq );
}

blargg_err_t Hes_Emu::load( const header_t& h, Data_Reader& in )
{
	unload();
	
	// check compatibility
	if ( 0 != memcmp( h.signature, "HESM", 4 ) )
		return "Not a HES file";
	if ( h.version != 0 )
		return "Unsupported HES version";
	
	set_voice_count( 6 );
	set_track_count( 256 );
	
	synth.volume( 0.6 / voice_count() );

	unsigned rom_size = sizeof( h ) + in.remain();
	BOOST::uint8_t * rom = new BOOST::uint8_t [ rom_size ];
	if ( ! rom )
		return "Out of memory";
	memcpy( rom, & h, sizeof( h ) );
	blargg_err_t err = in.read( rom + sizeof( h ), in.remain() );
	if ( err )
		return err;

	hes = FESTAHES_Load( rom, rom_size );
	delete [] rom;
	
	( ( FESTALON_HES * ) hes )->psg.blip_synth = & synth;
	
	return setup_buffer( 1789772.7272 * 4 + 0.5 );
}

blargg_err_t Hes_Emu::load( Data_Reader& in )
{
	header_t h;
	BLARGG_RETURN_ERR( in.read( &h, sizeof h ) );
	return load( h, in );
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

void Hes_Emu::start_track( int track )
{
	require( hes ); // file must be loaded
	
	Classic_Emu::start_track( track );
	
	FESTAHES_SongControl( ( FESTALON_HES * ) hes, track );
}

blip_time_t Hes_Emu::run( int msec, bool* stereo_used )
{
	if ( stereo_used ) * stereo_used = true;
	return FESTAHES_Emulate( ( FESTALON_HES * ) hes );
}

