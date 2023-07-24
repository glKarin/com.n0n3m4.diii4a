// Copyright (C) 2007 Id Software, Inc.
//

#ifndef _COMMON_COMMON_H_
#define _COMMON_COMMON_H_

typedef enum {
	CPUID_NONE							= 0x00000,
	CPUID_UNSUPPORTED					= 0x00001,	// unsupported (386/486)
	CPUID_GENERIC						= 0x00002,	// unrecognized processor
	CPUID_INTEL							= 0x00004,	// Intel
	CPUID_AMD							= 0x00008,	// AMD
	CPUID_MMX							= 0x00010,	// Multi Media Extensions
	CPUID_3DNOW							= 0x00020,	// 3DNow!
	CPUID_SSE							= 0x00040,	// Streaming SIMD Extensions
	CPUID_SSE2							= 0x00080,	// Streaming SIMD Extensions 2
	CPUID_SSE3							= 0x00100,	// Streaming SIMD Extentions 3 aka Prescott's New Instructions
	CPUID_ALTIVEC						= 0x00200,	// AltiVec
	CPUID_XENON							= 0x00400,	// Xenon
	CPUID_HTT							= 0x01000,	// Hyper-Threading Technology
	CPUID_CMOV							= 0x02000,	// Conditional Move (CMOV) and fast floating point comparison (FCOMI) instructions
	CPUID_FTZ							= 0x04000,	// Flush-To-Zero mode (denormal results are flushed to zero)
	CPUID_DAZ							= 0x08000,	// Denormals-Are-Zero mode (denormal source operands are set to zero)
#ifdef MACOS_X
	CPUID_PPC							= 0x40000	// PowerPC G4/G5
#endif
} cpuid_t;

#define STRTABLE_ID				"#str_"
#define LSTRTABLE_ID			L"#str_"
#define STRTABLE_ID_LENGTH		5

#if defined(_WIN32)

#ifdef __INTEL_COMPILER
	#define ID_STATIC_TEMPLATE
#else
	#define ID_STATIC_TEMPLATE			static
#endif

#define ID_INLINE						__forceinline
#define ID_TLS							__declspec( thread )
#define SD_DEPRECATED					__declspec( deprecated )

#include <basetsd.h>					// needed for UINT_PTR
#include <stddef.h>						// needed for offsetof
#include <memory.h>						// needed for memcmp

#if !defined(_WIN64)
	#define	BUILD_STRING				"win-x86"
	#define BUILD_OS_ID					0
	#define	CPUSTRING					"x86"
	#define CPU_EASYARGS				1
#else
	#define	BUILD_STRING				"win-x64"
	#define BUILD_OS_ID					0
	#define	CPUSTRING					"x64"
	#define CPU_EASYARGS				0
#endif

#ifdef _XENON
	#undef CPU_EASYARGS
	#define CPU_EASYARGS				0
#endif

#define ALIGN16( x )					__declspec(align(16)) x
#define PACKED

#include <malloc.h>
#define _alloca16( x )					((void *)((((UINT_PTR)_alloca( (x)+15 )) + 15) & ~15))

#define PATHSEPARATOR_STR				"\\"
#define PATHSEPARATOR_CHAR				'\\'

#define assertmem( x, y )				assert( _CrtIsValidPointer( x, y, true ) )

#define ID_AL_DYNAMIC

#endif


#ifndef BIT
#define BIT( num )				BITT< num >::VALUE
#endif

template< unsigned int B > 
class BITT {
public:
	typedef enum bitValue_e {
		VALUE = 1 << B,
	} bitValue_t;
};

// Mac OSX
#if defined(MACOS_X) || defined(__APPLE__)

#ifdef __ppc__
	#define BUILD_STRING				"MacOSX-ppc"
	#define BUILD_OS_ID					1
	#define	CPUSTRING					"ppc"
	#define CPU_EASYARGS				0
#elif defined(__i386__)
	#define BUILD_STRING				"MacOSX-x86"
	#define BUILD_OS_ID					1
	#define	CPUSTRING					"x86"
	#define CPU_EASYARGS				1
#endif

#if defined(__i386__)
#define ALIGN16( x )					x __attribute__ ((aligned (16))) 
#else
#define ALIGN16( x )					x
#endif

#ifdef __MWERKS__
#define PACKED
#include <alloca.h>
#else
#define PACKED							__attribute__((packed))
#endif

#define UINT_PTR						unsigned long

#define _alloca							alloca
#define _alloca16( x )					((void *)((((int)_alloca( (x)+15 )) + 15) & ~15))

#define PATHSEPARATOR_STR				"/"
#define PATHSEPARATOR_CHAR				'/'

#define __cdecl
#define ASSERT							assert

#define ID_STATIC_TEMPLATE
#define ID_INLINE						inline
#define SD_DEPRECATED
// from gcc 4.0 manual:
// The __thread specifier may be used alone, with the extern or static specifiers, but with no other storage class specifier. When used with extern or static, __thread must appear immediately after the other storage class specifier.
// The __thread specifier may be applied to any global, file-scoped static, function-scoped static, or static data member of a class. It may not be applied to block-scoped automatic or non-static data member. 
#define ID_TLS							__thread

#define assertmem( x, y )

#endif


// Linux
#ifdef __linux__

#ifdef __i386__
	#define	BUILD_STRING				"linux-x86"
	#define BUILD_OS_ID					2
	#define CPUSTRING					"x86"
	#define CPU_EASYARGS				1
#elif defined(__ppc__)
	#define	BUILD_STRING				"linux-ppc"
	#define BUILD_OS_ID					2
	#define CPUSTRING					"ppc"
	#define CPU_EASYARGS				0
#endif

#include <stddef.h>						// needed for offsetof

#define UINT_PTR						unsigned long

#include <alloca.h>
#define _alloca							alloca
#define _alloca16( x )					((void *)((((int)_alloca( (x)+15 )) + 15) & ~15))

#define ALIGN16( x )					x __attribute__ ((aligned (16)))
#define PACKED							__attribute__((packed))

#define PATHSEPARATOR_STR				"/"
#define PATHSEPARATOR_CHAR				'/'

#define __cdecl
#define ASSERT							assert

#define ID_STATIC_TEMPLATE
#define ID_INLINE						inline
#define ID_TLS							__thread
#define SD_DEPRECATED

#define assertmem( x, y )

#define ID_AL_DYNAMIC

#endif

template< typename T > ID_INLINE void Swap( T& l, T& r ) {
	T temp = l;
	l = r;
	r = temp;
}

#endif // _COMMON_COMMON_H_
