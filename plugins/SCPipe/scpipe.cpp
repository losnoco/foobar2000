// scpipe.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

// #define LOG_EXCHANGE

enum
{
	BUFFER_SIZE = 4096
};

static HANDLE null_file = NULL;
static HANDLE pipe_in = NULL;
static HANDLE pipe_out = NULL;

#ifdef LOG_EXCHANGE
unsigned exchange_count = 0;
#endif

void put_bytes( const void * out, uint32_t size )
{
	DWORD dwWritten;
	WriteFile( pipe_out, out, size, &dwWritten, NULL );
#ifdef LOG_EXCHANGE
	TCHAR logfile[MAX_PATH];
	_stprintf_s( logfile, _T("C:\\temp\\log\\bytes_%08u.out"), exchange_count++ );
	FILE * f = _tfopen( logfile, _T("wb") );
	fwrite( out, 1, size, f );
	fclose( f );
#endif
}

void put_code( uint32_t code )
{
	put_bytes( &code, sizeof(code) );
}

void get_bytes( void * in, uint32_t size )
{
	DWORD dwRead;
	if ( !ReadFile( pipe_in, in, size, &dwRead, NULL ) || dwRead < size )
	{
		memset( in, 0, size );
#ifdef LOG_EXCHANGE
		TCHAR logfile[MAX_PATH];
		_stprintf_s( logfile, _T("C:\\temp\\log\\bytes_%08u.err"), exchange_count++ );
		FILE * f = _tfopen( logfile, _T("wb") );
		_ftprintf( f, _T("Wanted %u bytes, got %u"), size, dwRead );
		fclose( f );
#endif
	}
	else
	{
#ifdef LOG_EXCHANGE
		TCHAR logfile[MAX_PATH];
		_stprintf_s( logfile, _T("C:\\temp\\log\\bytes_%08u.in"), exchange_count++ );
		FILE * f = _tfopen( logfile, _T("wb") );
		fwrite( in, 1, size, f );
		fclose( f );
#endif
	}
}

uint32_t get_code()
{
	uint32_t code;
	get_bytes( &code, sizeof(code) );
	return code;
}

LONG __stdcall myExceptFilterProc( LPEXCEPTION_POINTERS param )
{
	if (IsDebuggerPresent())
	{
		return UnhandledExceptionFilter(param);
	}
	else
	{
		//DumpCrashInfo( param );
		TerminateProcess( GetCurrentProcess(), 0 );
		return 0;// never reached
	}
}

int CALLBACK _tWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow )
{
	int argc = 0;
	LPWSTR * argv = CommandLineToArgvW( GetCommandLineW(), &argc );

	if ( argv == NULL || argc != 2 ) return 1;

#ifdef _DEBUG
	MessageBox(GetDesktopWindow(), _T("Boop"), _T("meh"), 0);
#endif

	unsigned code = 0;

	SCCore * sampler = NULL;

	uint32_t sample_rate = 44100;

	std::vector<float> sample_buffer;
	unsigned int samples_buffered = 0;

	null_file = CreateFile( _T("NUL"), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL );

	pipe_in = GetStdHandle( STD_INPUT_HANDLE );
	pipe_out = GetStdHandle( STD_OUTPUT_HANDLE );

	SetStdHandle( STD_INPUT_HANDLE, null_file );
	SetStdHandle( STD_OUTPUT_HANDLE, null_file );

	if ( FAILED( CoInitialize( NULL ) ) ) return 5;

#ifndef _DEBUG
	SetUnhandledExceptionFilter( myExceptFilterProc );
#endif

	sampler = new SCCore;

	if ( !sampler->Load( argv[ 1 ], false ) )
	{
		code = 1;
		goto exit;
	}

	if (sampler->TG_initialize(0) < 0)
	{
		code = 2;
		goto exit;
	}

	sample_buffer.resize( BUFFER_SIZE * 4 );

	put_code( 0 );

	for (;;)
	{
		uint32_t command = get_code();
		if ( !command ) break;

		switch ( command )
		{
		case 1: // Set Sample Rate
			{
				uint32_t size = get_code();
				if ( size != sizeof(sample_rate) )
				{
					code = 10;
					goto exit;
				}

				sample_rate = get_code();

				sampler->TG_activate(44100.0, 1024);
				sampler->TG_setMaxBlockSize(256);
				sampler->TG_setSampleRate((float)sample_rate);
				sampler->TG_setSampleRate((float)sample_rate);
				sampler->TG_setMaxBlockSize(BUFFER_SIZE);

				put_code( 0 );
			}
			break;

		case 2: // Send MIDI Event
			{
				uint32_t b = get_code();

				sampler->TG_ShortMidiIn(b, 0);

				put_code( 0 );
			}
			break;

		case 3: // Send System Exclusive Event
			{
				uint32_t size = get_code();

				unsigned char * msg = (unsigned char*) malloc( size + 1 );

				if (!msg)
				{
					code = 3;
					goto exit;
				}

				get_bytes( msg, size );

				if ( msg[size-1] != 0xF7 )
					msg[size] = 0xF7;

				sampler->TG_LongMidiIn( msg, 0 );

				free( msg );

				put_code( 0 );
			}
			break;

		case 4: // Render Samples
			{
				uint32_t count = get_code();

				put_code( 0 );

				while( count )
				{
					unsigned count_to_do = min(count, BUFFER_SIZE);

					memset(&sample_buffer[0], 0, count_to_do * sizeof(float));
					memset(&sample_buffer[BUFFER_SIZE], 0, count_to_do * sizeof(float));

					sampler->TG_setInterruptThreadIdAtThisTime();
					sampler->TG_Process(&sample_buffer[0], &sample_buffer[BUFFER_SIZE], count_to_do);

					float * out = &sample_buffer[BUFFER_SIZE * 2];

					for ( unsigned i = 0; i < count_to_do; ++i )
					{
						float sample = sample_buffer[i];
						out[ 0 ] = sample;
						sample = sample_buffer[BUFFER_SIZE + i];
						out[ 1 ] = sample;
						out += 2;
					}

					put_bytes( &sample_buffer[BUFFER_SIZE * 2], count_to_do * sizeof(float) * 2 );

					count -= count_to_do;
				}
			}
			break;

		case 5: // Junk Samples
			{
				uint32_t count = get_code();

				while ( count )
				{
					unsigned count_to_do = min(count, BUFFER_SIZE);

					sampler->TG_setInterruptThreadIdAtThisTime();
					sampler->TG_Process(&sample_buffer[0], &sample_buffer[BUFFER_SIZE], count_to_do);

					count -= count_to_do;
				}

				put_code( 0 );
			}
			break;

		default:
			code = 4;
			goto exit;
			break;
		}
	}

exit:
	delete sampler;
	CoUninitialize();
	if ( argv ) LocalFree( argv );

	put_code( code );

	if ( null_file )
	{
		CloseHandle( null_file );

		SetStdHandle( STD_INPUT_HANDLE, pipe_in );
		SetStdHandle( STD_OUTPUT_HANDLE, pipe_out );
	}

	return code;
}
