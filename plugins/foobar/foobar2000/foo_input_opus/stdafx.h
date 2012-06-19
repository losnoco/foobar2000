// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

// Windows Header Files:
#include <windows.h>



// TODO: reference additional headers your program requires here

#include <foobar2000.h>

struct ogg_packet
{
	const t_uint8 * header;
	unsigned header_size;
	unsigned reserved[6];
};

#include <opus.h>
#include <opus_multistream.h>

#include "opus_header.h"
#include <string.h>
#include <stdio.h>
