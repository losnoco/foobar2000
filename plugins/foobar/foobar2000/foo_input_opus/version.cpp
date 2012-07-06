#include "stdafx.h"

#define MY_VERSION "1.0"

/*

2012-07-06 19:13 UTC - kode54
- Fixed analysis reporting for lower mode and bandwidth

2012-07-06 02:09 UTC - kode54
- Added initial packet analysis for technical information

2012-07-03 05:37 UTC - kode54
- Implemented channel mapping by transmuting the stream map table

2012-06-20 06:17 UTC - kode54
- Completed preliminary version with basic playback, seeking, and tag reading support
- Version is now 1.0

2012-06-19 02:55 UTC - kode54
- Project created

*/

DECLARE_COMPONENT_VERSION( "Opus Decoder", MY_VERSION, "" );

VALIDATE_COMPONENT_FILENAME("foo_input_opus.dll");
