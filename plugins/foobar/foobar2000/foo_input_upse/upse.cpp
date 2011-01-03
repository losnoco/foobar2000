#include <exception>
#include <windows.h>

#include "upse.h"

extern "C" {
#include <upse-internal.h>
}

static struct upse_init
{
	upse_init() { upse_module_init(); }
} initializer;

upse::upse()
{
	m_module = NULL;
	m_stopped = false;
}

upse::~upse()
{
	close();
}

void upse::close()
{
	if ( m_module && !m_stopped )
	{
		s16 * temp;
		upse_eventloop_stop( m_module );
		upse_eventloop_render( m_module, &temp );
	}
	if ( m_module ) upse_module_close( m_module );
	m_module = NULL;
	m_stopped = false;
}

int upse::open( const char * path, const upse_iofuncs_t * iofuncs, int play_forever, int default_length, int default_fade )
{
	close();
	strcpy_s( m_path, path );
	m_iofuncs = iofuncs;
	m_play_forever = play_forever;
	m_default_length = default_length;
	m_default_fade = default_fade;
	return reopen();
}

int upse::reopen()
{
	m_module = upse_module_open( m_path, m_iofuncs );
	if ( m_module )
	{
		upse_spu_state_t * p_spu = reinterpret_cast<upse_spu_state_t *> ( m_module->instance.spu );
		upse_ps1_spu_setvolume( p_spu, 32 );
		if ( m_play_forever ) upse_ps1_spu_setlength( p_spu, 0, 0 );
		else if ( m_module->metadata->length == 0 ) upse_ps1_spu_setlength( p_spu, m_default_length, m_default_fade );
		m_stopped = false;
	}
	return !!m_module;
}

int upse::seek( u32 t )
{
	if ( m_module )
	{
		if ( ! upse_eventloop_seek( m_module, t ) )
		{
			close();
			if ( ! reopen() ) return 0;
			upse_eventloop_seek( m_module, t );
		}
		return 1;
	}
	return 0;
}

int upse::render( s16 ** s )
{
	if ( m_module )
	{
		int ret = upse_eventloop_render( m_module, s );
		if ( !ret ) m_stopped = true;
		return ret;
	}
	return 0;
}
