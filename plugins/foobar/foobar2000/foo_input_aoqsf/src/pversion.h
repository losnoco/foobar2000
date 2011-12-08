#define XSFDRIVER_VERSIONS "0.07"
#define XSFDRIVER_MODULENAME "aoqsf.bin"
#define XSFDRIVER_ENTRYNAME "XSFSetup"
#define XSFDRIVER_ISSYSTEMTAG(taglen, tag) (((taglen) > 0) && ((tag)[0] == '_'))
#define XSFDRIVER_GUID1 { 0x8a1e5bef, 0x77ed, 0x4235, { 0xbe, 0x05, 0x26, 0x0a, 0x3a, 0x5e, 0x6a, 0x86 } } /* {8A1E5BEF-77ED-4235-BE05-260A3A5E6A86} */
#define XSFDRIVER_GUID2 { 0x4d83c16e, 0xeaf2, 0x4d77, { 0xb7, 0x2f, 0x2d, 0x0b, 0x15, 0x96, 0x55, 0xba } } /* {4D83C16E-EAF2-4d77-B72F-2D0B159655BA} */
#define XSFDRIVER_CHANNELMAP { { 16, "QSound %2d" } , { 0, 0 } }
#define XSFDRIVER_SIMPLENAME "QSF Decoder"

#define AOSDK_VERSIONS "1.4.8"
#define AOSDK_COPYRIGHT "Copyright (c) 2007-2009 R. Belmont and Richard Bannister."

#define WINAMPPLUGIN_COPYRIGHT AOSDK_COPYRIGHT
#define WINAMPPLUGIN_NAME "QSF Decoder " XSFDRIVER_VERSIONS "/ Audio Overload SDK " AOSDK_VERSIONS " (x86)"
#define WINAMPPLUGIN_EXTS "QSF;MINIQSF\0QSound Format files(*.QSF;*.MINIQSF)\0\0\0"
#define WINAMPPLUGIN_TAG_XSFBY "qsfby"

#define KBMEDIAPLUGIN_VERSION 7
#define KBMEDIAPLUGIN_COPYRIGHT AOSDK_COPYRIGHT
#define KBMEDIAPLUGIN_NAME "QSF plugin " XSFDRIVER_VERSIONS " / Audio Overload SDK " AOSDK_VERSIONS
#define KBMEDIAPLUGIN_EXTS(n)	\
	static const char n##_qsfext[] = ".qsf";	\
	static const char n##_miniqsfext[] = ".miniqsf";	\
	static const char * const (n) [] = {	\
		n##_qsfext,	\
		n##_miniqsfext,	\
		0,	\
	};

#define FOOBAR2000COMPONENT_NAME "QSF decoder / Audio Overload SDK " AOSDK_VERSIONS
#define FOOBAR2000COMPONENT_VERSION XSFDRIVER_VERSIONS
#define FOOBAR2000COMPONENT_ABOUT "Audio Overload SDK " AOSDK_VERSIONS "\n" AOSDK_COPYRIGHT "\n"
#define FOOBAR2000COMPONENT_TYPE "QSound Format files"
#define FOOBAR2000COMPONENT_EXTS "*.QSF;*.MINIQSF"
#define FOOBAR2000COMPONENT_EXT_CHECK (!stricmp_utf8(p_extension,"QSF") || !stricmp_utf8(p_extension,"MINIQSF"))
#define FOOBAR2000COMPONENT_ENCODING "qsf"

