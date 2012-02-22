/* Setup for Microsoft Visual C++ Version 5 */
#ifndef _config_h_
#define _config_h_

#define PACKAGE "libsidplayfp"
#define VERSION "0.1.0"

/* Define if your C++ compiler implements exception-handling.  */
/* Note: exception specification is only available for MSVC > 6 */
#if _MSC_VER > 1200
#   define HAVE_EXCEPTIONS
#endif

/* Define if you support file names longer than 14 characters.  */
#define HAVE_LONG_FILE_NAMES

/* Define if you have the <sstream> header file.  */
#define HAVE_SSTREAM

#define EMBEDDED_ROMS 1

/* Define if ``ios::nocreate'' is supported. */
//#define HAVE_IOS_NOCREATE

#endif // _config_h_