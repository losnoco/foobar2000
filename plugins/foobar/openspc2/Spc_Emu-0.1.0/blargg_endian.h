
// CPU Byte Order Utilities

#ifndef BLARGG_ENDIAN
#define BLARGG_ENDIAN
#ifndef GET_LE16

#if 0
	// Read 16-bit little-endian unsigned integer from memory
	unsigned GET_LE16( const void* );

	// Write 16-bit little-endian integer to memory
	void SET_LE16( void*, unsigned );
#endif

// Use blargg_common.h preprocessor flags if they're available, otherwise
// use portable definitions.
#if !BLARGG_NONPORTABLE || BLARGG_BIG_ENDIAN
	#if !BLARGG_NONPORTABLE || !BLARGG_CPU_POWERPC
		inline unsigned GET_LE16( const void* p ) {
			return *((unsigned char*) p + 1) * 0x100 + *(unsigned char*) p;
		}
		
		inline void SET_LE16( void* p, unsigned n ) {
			*(unsigned char*) p = n;
			*((unsigned char*) p + 1) = n >> 8;
		}
	
	#else
		// PowerPC has special byte-reversed instructions
		#define GET_LE16( addr )        ((unsigned) __lhbrx( (addr), 0 ))
		#define SET_LE16( addr, data )  (__sthbrx( (data), (addr), 0 ))
	
	#endif

#elif BLARGG_LITTLE_ENDIAN
	#define GET_LE16( addr )        (*(unsigned short*) (addr))
	#define SET_LE16( addr, data )  (void (*(unsigned short*) (addr) = (data)))

#endif

#endif
#endif

