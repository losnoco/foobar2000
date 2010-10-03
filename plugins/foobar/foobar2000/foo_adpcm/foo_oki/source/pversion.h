#define LIBPCM_VERSION 14
#define LIBPCM_VERSIONS "0.14"
#define LIBPCM_COPYRIGHT "<?>"

#define WINAMPPLUGIN_COPYRIGHT LIBPCM_COPYRIGHT
#define WINAMPPLUGIN_NAME "OKI-ADPCM Decoder v" LIBPCM_VERSIONS " (x86)"
#define WINAMPPLUGIN_EXTS	\
	"PCM\0OKI-ADPCM/Circus-XPCM sample files(*.PCM)\0"	\
	"8\0Konami-Gungage-XA sample files(*.8)\0"	\
	"WAF\0KID-WAF sample files(*.WAF)\0"	\
	"WPD\0BasiL-WPD sample files(*.WPD)\0"	\
	"MPF\0TamaSoft-MPF sample files(*.MPF)\0"	\
	"WDT\0StudioMiris-WDT sample files(*.WDT)\0"	\
	"KWF\0Hayashigumi-KWF sample files(*.KWF)\0"	\
	"NWA\0VisualArts-NWA sample files(*.NWA)\0"	\
	"PX\0jANIS-PX sample files(*.PX)\0"	\
	"BW\0CLOVER-BW sample files(*.BW)\0"	\
	"ADP\0AbogadoPowers-ADP sample files(*.ADP)\0"

#define WAC_SERVICE_NAME "OKI-ADPCM to PCM converter v" LIBPCM_VERSIONS
#define WAC_CLIENT_NAME "OKI-ADPCM support"
#define WAC_CAN_CONVERT_FROM(r,n,c)	\
	(	\
		n &&	\
		(	\
			!STRICMP(Std::extension((n)),"pcm")	\
			|| !STRICMP(Std::extension((n)),"8")	\
			|| !STRICMP(Std::extension((n)),"waf")	\
			|| !STRICMP(Std::extension((n)),"wpd")	\
			|| !STRICMP(Std::extension((n)),"mpf")	\
			|| !STRICMP(Std::extension((n)),"wdt")	\
			|| !STRICMP(Std::extension((n)),"kwf")	\
			|| !STRICMP(Std::extension((n)),"nwa")	\
			|| !STRICMP(Std::extension((n)),"px")	\
			|| !STRICMP(Std::extension((n)),"bw")	\
			|| !STRICMP(Std::extension((n)),"adp")	\
		)	\
	)
#define WAC_REGISTER_EXTS	\
	{	\
		api->core_registerExtension("*.pcm","OKI-ADPCM/Circus-XPCM Files");	\
		api->core_registerExtension("*.8","Konami-Gungage-XA Files");	\
		api->core_registerExtension("*.waf","KID-WAF Files");	\
		api->core_registerExtension("*.wpd","BasiL-WPD Files");	\
		api->core_registerExtension("*.mpf","TamaSoft-MPF Files");	\
		api->core_registerExtension("*.wdt","StudioMiris-WDT Files");	\
		api->core_registerExtension("*.kwf","Hayashigumi-KWF Files");	\
		api->core_registerExtension("*.nwa","VisualArts-NWA Files");	\
		api->core_registerExtension("*.px","jANIS-PX Files");	\
		api->core_registerExtension("*.bw","CLOVER-BW Files");	\
		api->core_registerExtension("*.adp","AbogadoPowers-ADP Files");	\
	}

#define KBMEDIAPLUGIN_VERSION LIBPCM_VERSION
#define KBMEDIAPLUGIN_COPYRIGHT LIBPCM_COPYRIGHT
#define KBMEDIAPLUGIN_NAME "OKI-ADPCM plugin v" LIBPCM_VERSIONS
#define KBMEDIAPLUGIN_EXTS(n)	\
	static const char n##_pcmext[] = ".pcm";	\
	static const char n##_8ext[] = ".8";	\
	static const char n##_wafext[] = ".waf";	\
	static const char n##_wpdext[] = ".wpd";	\
	static const char n##_mpfext[] = ".mpf";	\
	static const char n##_wdtext[] = ".wdt";	\
	static const char n##_kwfext[] = ".kwf";	\
	static const char n##_nwaext[] = ".nwa";	\
	static const char n##_pxext[] = ".px";	\
	static const char n##_bwext[] = ".bw";	\
	static const char n##_adpext[] = ".adp";	\
	static const char * const (n) [] = {	\
		n##_pcmext,	\
		n##_8ext,	\
		n##_wafext,	\
		n##_wpdext,	\
		n##_mpfext,	\
		n##_wdtext,	\
		n##_kwfext,	\
		n##_nwaext,	\
		n##_pxext,	\
		n##_bwext,	\
		n##_adpext,	\
		0,	\
	};

#define FOOBAR2000COMPONENT_NAME "OKI-ADPCM decoder"
#define FOOBAR2000COMPONENT_VERSION LIBPCM_VERSIONS
#define FOOBAR2000COMPONENT_EXTS(n)	\
	static const char n##_pcmext[] = "PCM";	\
	static const char n##_8ext[] = "8";	\
	static const char n##_wafext[] = "WAF";	\
	static const char n##_wpdext[] = "WPD";	\
	static const char n##_mpfext[] = "MPF";	\
	static const char n##_wdtext[] = "WDT";	\
	static const char n##_kwfext[] = "KWF";	\
	static const char n##_nwaext[] = "NWA";	\
	static const char n##_pxext[] = "PX";	\
	static const char n##_bwext[] = "BW";	\
	/*static const char n##_adpext[] = "ADP";*/	\
	static const char * const (n) [] = {	\
		n##_pcmext,	\
		n##_8ext,	\
		n##_wafext,	\
		n##_wpdext,	\
		n##_mpfext,	\
		n##_wdtext,	\
		n##_kwfext,	\
		n##_nwaext,	\
		n##_pxext,	\
		n##_bwext,	\
		/*n##_adpext,*/	\
		0,	\
	};
