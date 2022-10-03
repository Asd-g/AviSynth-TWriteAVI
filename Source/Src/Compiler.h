
#ifndef __COMPILER_H__
	#define __COMPILER_H__

    #ifdef UNICODE
    	#undef UNICODE													// Avoid default wide character stuff
    #endif
    #ifdef _UNICODE
    	#undef _UNICODE													// Avoid default wide character stuff
    #endif
	#ifdef MBCS 
		#undef MBCS 
	#endif
	#ifdef _MBCS 
		#undef _MBCS 
	#endif
    
    #define _CRT_SECURE_NO_WARNINGS

    // Compile for minimum supported system. MUST be defined before includes.
    // NEED to use SDK for updated headers, TK3 will give error messages/Warnings about Beta versions.
    #define WINVER			0x0501			// XP
    #define _WIN32_WINNT	0x0501			// XP
    #define NTDDI_VERSION	0x05010300		// XP SP3
    #define _WIN32_IE		0x0603			// 0x0603=IE 6 SP2, 0x0700=1e7, 0x0800=1e8


//	#define BUG							// Uncomment to enable DPRINTF() output

	#ifdef BUG
		#if _MSC_VER >= 1500									// VS 2008
			// Call as eg DPRINTF("Forty Two = %d",42)          // No Semi colon
			// C99 Compiler (eg VS2008)
			#define DPRINTF(fmt, ...)   dprintf(fmt, ##__VA_ARGS__);       // No Semi colon needed
		#else
			// OLD C89 Compiler: (eg VS6)
			// Call as eg DPRINTF(("Forty Two = %d",42))        // Enclosed in double parentethis, No Semi colon
			#define   DPRINTF(x)      dprintf x;
		#endif
	#else
		#if _MSC_VER >= 1500									// VS 2008
			// C99 Compiler (eg VS2008)
			#define DPRINTF(fmt,...)							/* fmt */
		#else
			// OLD C89 Compiler: (eg VS6)
			#define DPRINTF(x)									/* x */
		#endif
	#endif

	extern int __cdecl dprintf(char* fmt, ...);

#endif // __COMPILER_H__

