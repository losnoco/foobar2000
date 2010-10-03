#ifndef STRICT
#define STRICT
#endif

#include <foobar2000.h>

#include <windows.h>
#include <malloc.h>

#ifndef NOVTABLE
#ifdef _DEBUG
#define NOVTABLE
#else
#define NOVTABLE _declspec(novtable)
#endif
#endif

#include <mmsystem.h>

#include "utils.h"

//#define USE_LOG

#ifdef USE_LOG
void log_write(char*);
void log_start();
void log_quit();
#else
#define log_write(X)
#define log_start()
#define log_quit()
#endif

class CStream;
struct CTempoMap;
struct CSysexMap;

#include "midifile.h"

#define _ERROR _NAME" error"

//#pragma warning(disable:4800)

extern cfg_int cfg_recover_tracks, cfg_nosysex;
