#include <exception>
#include <windows.h>

#include "upse.h"

#include "dll_manager.h"

static dll_manager g_manager;

typedef	upse_psf_t * ( FAR __cdecl * get_psf_metadata_proc ) ( const char *, const upse_iofuncs_t * );
typedef void ( FAR __cdecl * free_psf_metadata_proc ) ( upse_psf_t * );

typedef const upse_module_t * ( FAR __cdecl * open_proc ) (const char *, const upse_iofuncs_t * , int, int, int);
typedef void ( FAR __cdecl * close_proc ) ();
typedef int ( FAR __cdecl * seek_proc ) ( u32 );
typedef int ( FAR __cdecl * render_proc ) ( s16 ** );

upse::upse( const char * path )
{
	upse_dll = g_manager.load( path, true );
	if ( upse_dll )
	{
		p_get_psf_metadata = GetProcAddress( upse_dll, "upse_dll_get_psf_metadata" );
		p_free_psf_metadata = GetProcAddress( upse_dll, "upse_dll_free_psf_metadata" );
		p_open = GetProcAddress( upse_dll, "upse_dll_open" );
		p_close = GetProcAddress( upse_dll, "upse_dll_close" );
		p_seek = GetProcAddress( upse_dll, "upse_dll_seek" );
		p_render = GetProcAddress( upse_dll, "upse_dll_render" );
	}
	if ( !upse_dll || !p_get_psf_metadata || !p_free_psf_metadata || !p_open || !p_close || !p_seek || !p_render )
	{
		throw std::exception( "Failed to load UPSE library." );
	}
}

upse::~upse()
{
	if ( upse_dll && p_close ) reinterpret_cast< close_proc > ( p_close ) ();
	if ( upse_dll ) g_manager.free( upse_dll );
}

upse_psf_t * upse::get_psf_metadata( const char * path, const upse_iofuncs_t * iofuncs )
{
	return reinterpret_cast< get_psf_metadata_proc > ( p_get_psf_metadata ) ( path, iofuncs );
}

void upse::free_psf_metadata( upse_psf_t * info )
{
	reinterpret_cast< free_psf_metadata_proc > ( p_free_psf_metadata ) ( info );
}

const upse_module_t * upse::open( const char * path, const upse_iofuncs_t * iofuncs, int play_forever, int default_length, int default_fade )
{
	reinterpret_cast< close_proc > ( p_close ) ();
	return reinterpret_cast< open_proc > ( p_open ) ( path, iofuncs, play_forever, default_length, default_fade );
}

int upse::seek( u32 t )
{
	return reinterpret_cast< seek_proc > ( p_seek ) ( t );
}

int upse::render( s16 ** s )
{
	return reinterpret_cast< render_proc > ( p_render ) ( s );
}
