
// Rar_Extractor 0.3.0. See unrar_license.txt.

#ifndef RAR_COMMON_HPP
#define RAR_COMMON_HPP

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

const int NM = 1024; // max filename length

typedef unsigned char  byte;
typedef unsigned short ushort;
typedef unsigned int   uint;

#define Min( x, y ) ((x) < (y) ? (x) : (y))

// 64-bit int support disabled, since this library isn't meant for huge 2GB+ archives
typedef long Int64;
#define int64to32(x) ((uint)(x))

// used as base class to override new and delete to not throw exceptions
struct Rar_Allocator
{
	static void* operator new ( size_t s ) { return malloc( s ); }
	static void operator delete ( void* p ) { free( p ); }
};

class Rar_Extractor_Impl;
typedef Rar_Extractor_Impl Rar_Error_Handler;
void rar_out_of_memory( Rar_Error_Handler* );

// lots of order dependencies here
class Unpack;
#include "rarmisc.hpp"
#include "../abstract_file.h"
#include "array.hpp"
#include "headers.hpp"
#include "archive.hpp"
#include "rarvm.hpp"
#include "suballoc.hpp"
#include "model.hpp"
#include "unpack.hpp"
#include "Rar_Extractor_Impl.h"

#endif

