#ifndef UPSE_H
#define UPSE_H

#include <Windows.h>

#include <upse.h>

class upse
{
	HMODULE upse_dll;
	FARPROC p_get_psf_metadata;
	FARPROC p_free_psf_metadata;
	FARPROC p_open;
	FARPROC p_close;
	FARPROC p_seek;
	FARPROC p_render;

public:
	upse( const char * path );
	~upse();

	upse_psf_t *          get_psf_metadata( const char * path, const upse_iofuncs_t * iofuncs );
	void                  free_psf_metadata(upse_psf_t * info);

	const upse_module_t * open( const char *path, const upse_iofuncs_t * iofuncs, int play_forever, int default_length, int default_fade);
	int                   seek( u32 t );
	int                   render( s16 ** s );
};

#endif
