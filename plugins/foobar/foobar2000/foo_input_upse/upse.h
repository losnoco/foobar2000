#ifndef UPSE_H
#define UPSE_H

#include <Windows.h>

extern "C" {
#include <upse.h>
}

class upse
{
	upse_module_t * m_module;
	bool m_stopped;

	char m_path[32768];
	const upse_iofuncs_t * m_iofuncs;
	int m_play_forever, m_default_length, m_default_fade;

	void close();
	int reopen();

public:
	upse();
	~upse();

	int         open( const char *path, const upse_iofuncs_t * iofuncs, int play_forever, int default_length, int default_fade);
	int         seek( u32 t );
	int         render( s16 ** s );
};

#endif
