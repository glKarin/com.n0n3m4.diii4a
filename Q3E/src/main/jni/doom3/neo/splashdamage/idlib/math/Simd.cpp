// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "Simd_Generic.h"
#include "Simd_MMX.h"
#include "Simd_3DNow.h"
#include "Simd_SSE.h"
#include "Simd_SSE2.h"
#include "Simd_SSE3.h"
#include "Simd_AltiVec.h"
#include "Simd_Xenon.h"

idSIMDProcessor	*	processor = NULL;			// pointer to SIMD processor
idSIMDProcessor *	generic = NULL;				// pointer to generic SIMD implementation
idSIMDProcessor *	SIMDProcessor = NULL;

#define XENON_FRAMES		0

#if XENON_FRAMES
extern IDirect3D9*             g_pD3D;			// Used to create the D3DDevice
extern IDirect3DDevice9*       g_pd3dDevice;	// the rendering device
#endif

/*
================
idSIMD::Init
================
*/
void idSIMD::Init( void ) {
	generic = new idSIMD_Generic;
	generic->cpuid = CPUID_GENERIC;
	processor = NULL;
	SIMDProcessor = generic;
}

/*
============
idSIMD::InitProcessor
============
*/

void idSIMD::InitProcessor( const char *module, bool forceGeneric ) {
	cpuid_t cpuid;
	idSIMDProcessor *newProcessor;

	cpuid = idLib::sys->GetProcessorId();

	if ( forceGeneric ) {

		newProcessor = generic;

	} else {

		if ( !processor ) {
			if ( ( cpuid & CPUID_XENON ) ) {
				processor = new idSIMD_Xenon;
			} else if ( ( cpuid & CPUID_ALTIVEC ) ) {
				processor = new idSIMD_AltiVec;
			} else if ( ( cpuid & CPUID_MMX ) && ( cpuid & CPUID_SSE ) && ( cpuid & CPUID_SSE2 ) && ( cpuid & CPUID_SSE3 ) ) {
				processor = new idSIMD_SSE3;
			} else if ( ( cpuid & CPUID_MMX ) && ( cpuid & CPUID_SSE ) && ( cpuid & CPUID_SSE2 ) ) {
				processor = new idSIMD_SSE2;
			} else if ( ( cpuid & CPUID_MMX ) && ( cpuid & CPUID_SSE ) ) {
				processor = new idSIMD_SSE;
			} else if ( ( cpuid & CPUID_MMX ) && ( cpuid & CPUID_3DNOW ) ) {
				processor = new idSIMD_3DNow;
			} else if ( ( cpuid & CPUID_MMX ) ) {
				processor = new idSIMD_MMX;
			} else {
				processor = generic;
			}
			processor->cpuid = cpuid;
		}

		newProcessor = processor;
	}

	if ( newProcessor != SIMDProcessor ) {
		SIMDProcessor = newProcessor;
		idLib::common->Printf( "%s using %s for SIMD processing\n", module, SIMDProcessor->GetName() );
	}

	if ( cpuid & CPUID_FTZ ) {
		idLib::sys->FPU_SetFTZ( true );
		idLib::common->Printf( "enabled Flush-To-Zero mode\n" );
	}

	if ( cpuid & CPUID_DAZ ) {
		idLib::sys->FPU_SetDAZ( true );
		idLib::common->Printf( "enabled Denormals-Are-Zero mode\n" );
	}
}

/*
================
idSIMD::Shutdown
================
*/
void idSIMD::Shutdown( void ) {
	if ( processor != generic ) {
		delete processor;
	}
	delete generic;
	generic = NULL;
	processor = NULL;
	SIMDProcessor = NULL;
}


//===============================================================
//
// Test code
//
//===============================================================

#define COUNT		1024		// data count
#define NUMTESTS	1024		// number of tests

#define RANDOM_SEED		1013904223L	//((int)idLib::sys->GetClockTicks())

idSIMDProcessor *p_simd;
idSIMDProcessor *p_generic;

#ifdef _XENON

#define TIME_TYPE double

void PrintResolution( void ) {
    LARGE_INTEGER TicksPerSecond;
    QueryPerformanceFrequency( &TicksPerSecond );
    common->Printf( "%f ticks per second\n", (DOUBLE)TicksPerSecond.QuadPart );
}

#define StartRecordTime( start )			\
	{										\
		LARGE_INTEGER li;					\
		QueryPerformanceCounter( &li );		\
		start = (double ) li.LowPart + (double) 0xFFFFFFFF * li.HighPart;	\
		start *= 64; /* 50 MHz */			\
	}

#define StopRecordTime( end )				\
	{										\
		LARGE_INTEGER li;					\
		QueryPerformanceCounter( &li );		\
		end = (double ) li.LowPart + (double) 0xFFFFFFFF * li.HighPart;	\
		end *= 64; /* 50 MHz */				\
	}

void FlushCache( void ) {
}

#elif defined( _WIN32 )

#ifdef ID_WIN_X86_ASM

#define TIME_TYPE int

void PrintResolution( void ) {
}

#pragma warning(disable : 4731)     // frame pointer register 'ebx' modified by inline assembly code

long saved_ebx = 0;

#define StartRecordTime( start )			\
	__asm mov saved_ebx, ebx				\
	__asm xor eax, eax						\
	__asm cpuid								\
	__asm rdtsc								\
	__asm mov start, eax					\
	__asm xor eax, eax						\
	__asm cpuid

#define StopRecordTime( end )				\
	__asm xor eax, eax						\
	__asm cpuid								\
	__asm rdtsc								\
	__asm mov end, eax						\
	__asm mov ebx, saved_ebx				\
	__asm xor eax, eax						\
	__asm cpuid

void FlushCache( void ) {
//	byte *ptr = (byte *)_alloca16( 1 << 20 );
//	memset( ptr, 0, 1 << 20 );
}

#else	// no ID_WIN_X86_ASM

#define TIME_TYPE double

void PrintResolution( void ) {
    LARGE_INTEGER TicksPerSecond;
    QueryPerformanceFrequency( &TicksPerSecond );
    common->Printf( "%f ticks per second\n", (LONGLONG)TicksPerSecond.QuadPart );
}

#define StartRecordTime( start )			\
	{										\
		LARGE_INTEGER li;					\
		QueryPerformanceCounter( &li );		\
		start = (double ) li.LowPart + (double) 0xFFFFFFFF * li.HighPart;	\
	}

#define StopRecordTime( end )				\
	{										\
		LARGE_INTEGER li;					\
		QueryPerformanceCounter( &li );		\
		end = (double ) li.LowPart + (double) 0xFFFFFFFF * li.HighPart;	\
	}

void FlushCache( void ) {
}

#endif

#elif MACOS_X

#include <stdlib.h>
#include <unistd.h>			// this is for sleep()
#include <sys/time.h>
#include <sys/resource.h>
#include <mach/mach_time.h>

#define TIME_TYPE uint64_t

void PrintResolution( void ) { }

void FlushCache( void ) { }

#define StartRecordTime( start )			\
	start = mach_absolute_time(); 

#define StopRecordTime( end )				\
	end = mach_absolute_time();

#elif __linux__

#define TIME_TYPE int

void PrintResolution( void ) { }

#define StartRecordTime( start )			\
	start = 0;

#define StopRecordTime( end )				\
	end = 1;

void FlushCache( void ) {
}

#else

#error missing OS define

#endif

#define GetBest( start, end, best )			\
	if ( !best || end - start < best ) {	\
		best = end - start;					\
	}

TIME_TYPE baseClocks = 0;


/*
============
PrintClocks
============
*/
void PrintClocks( char *string, int dataCount, int clocks, int otherClocks = 0 ) {
	int i;

	idLib::common->Printf( string );
	for ( i = idStr::LengthWithoutColors(string); i < 48; i++ ) {
		idLib::common->Printf(" ");
	}
	clocks -= baseClocks;
	if ( otherClocks && clocks ) {
		otherClocks -= baseClocks;
		int p = (int) ( (float) ( otherClocks - clocks ) * 100.0f / (float) otherClocks );
		idLib::common->Printf( "c = %4d, clcks = %5d, %d%%\n", dataCount, clocks, p );
	} else {
		idLib::common->Printf( "c = %4d, clcks = %5d\n", dataCount, clocks );
	}
}

/*
============
GetBaseClocks
============
*/
void GetBaseClocks( void ) {
	int i;
	TIME_TYPE start, end, bestClocks;

	bestClocks = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	baseClocks = bestClocks;
}

/*
============
TestAdd
============
*/
void TestAdd( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fdst0[COUNT] );
	ALIGN16( float fdst1[COUNT] );
	ALIGN16( float fsrc0[COUNT] );
	ALIGN16( float fsrc1[COUNT] );
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
		fsrc1[i] = srnd.CRandomFloat() * 10.0f;
	}

	idLib::common->Printf("====================================\n" );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Add( fdst0, 4.0f, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Add( float + float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Add( fdst1, 4.0f, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( idMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Add( float + float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Add( fdst0, fsrc0, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Add( float[] + float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Add( fdst1, fsrc0, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( idMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Add( float[] + float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestSub
============
*/
void TestSub( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fdst0[COUNT]; )
	ALIGN16( float fdst1[COUNT]; )
	ALIGN16( float fsrc0[COUNT]; )
	ALIGN16( float fsrc1[COUNT]; )
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
		fsrc1[i] = srnd.CRandomFloat() * 10.0f;
	}

	idLib::common->Printf("====================================\n" );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Sub( fdst0, 4.0f, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Sub( float + float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Sub( fdst1, 4.0f, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( idMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Sub( float + float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Sub( fdst0, fsrc0, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Sub( float[] + float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Sub( fdst1, fsrc0, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( idMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Sub( float[] + float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestMul
============
*/
void TestMul( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fdst0[COUNT]; )
	ALIGN16( float fdst1[COUNT]; )
	ALIGN16( float fsrc0[COUNT]; )
	ALIGN16( float fsrc1[COUNT]; )
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
		fsrc1[i] = srnd.CRandomFloat() * 10.0f;
	}

	idLib::common->Printf("====================================\n" );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Mul( fdst0, 4.0f, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Mul( float * float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Mul( fdst1, 4.0f, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( idMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Mul( float * float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Mul( fdst0, fsrc0, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Mul( float[] * float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Mul( fdst1, fsrc0, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( idMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Mul( float[] * float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestDiv
============
*/
void TestDiv( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fdst0[COUNT]; )
	ALIGN16( float fdst1[COUNT]; )
	ALIGN16( float fsrc0[COUNT]; )
	ALIGN16( float fsrc1[COUNT]; )
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
		do {
			fsrc1[i] = srnd.CRandomFloat() * 10.0f;
		} while( idMath::Fabs( fsrc1[i] ) < 0.1f );
	}

	idLib::common->Printf("====================================\n" );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Div( fdst0, 4.0f, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Div( float * float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Div( fdst1, 4.0f, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( idMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Div( float * float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Div( fdst0, fsrc0, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Div( float[] * float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Div( fdst1, fsrc0, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( idMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-3f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Div( float[] * float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestMulAdd
============
*/
void TestMulAdd( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fdst0[COUNT]; )
	ALIGN16( float fdst1[COUNT]; )
	ALIGN16( float fsrc0[COUNT]; )
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
	}

	idLib::common->Printf("====================================\n" );

	for ( j = 0; j < 50 && j < COUNT; j++ ) {

		bestClocksGeneric = 0;
		for ( i = 0; i < NUMTESTS; i++ ) {
			for ( int k = 0; k < COUNT; k++ ) {
				fdst0[k] = k;
			}
			StartRecordTime( start );
			p_generic->MulAdd( fdst0, 0.123f, fsrc0, j );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		PrintClocks( va( "generic->MulAdd( float * float[%2d] )", j ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( i = 0; i < NUMTESTS; i++ ) {
			for ( int k = 0; k < COUNT; k++ ) {
				fdst1[k] = k;
			}
			StartRecordTime( start );
			p_simd->MulAdd( fdst1, 0.123f, fsrc0, j );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		for ( i = 0; i < COUNT; i++ ) {
			if ( idMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
				break;
			}
		}
		result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MulAdd( float * float[%2d] ) %s", j, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

/*
============
TestMulSub
============
*/
void TestMulSub( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fdst0[COUNT]; )
	ALIGN16( float fdst1[COUNT]; )
	ALIGN16( float fsrc0[COUNT]; )
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
	}

	idLib::common->Printf("====================================\n" );

	for ( j = 0; j < 50 && j < COUNT; j++ ) {

		bestClocksGeneric = 0;
		for ( i = 0; i < NUMTESTS; i++ ) {
			for ( int k = 0; k < COUNT; k++ ) {
				fdst0[k] = k;
			}
			StartRecordTime( start );
			p_generic->MulSub( fdst0, 0.123f, fsrc0, j );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		PrintClocks( va( "generic->MulSub( float * float[%2d] )", j ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( i = 0; i < NUMTESTS; i++ ) {
			for ( int k = 0; k < COUNT; k++ ) {
				fdst1[k] = k;
			}
			StartRecordTime( start );
			p_simd->MulSub( fdst1, 0.123f, fsrc0, j );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		for ( i = 0; i < COUNT; i++ ) {
			if ( idMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
				break;
			}
		}
		result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MulSub( float * float[%2d] ) %s", j, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

/*
============
TestDot
============
*/
void TestDot( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fdst0[COUNT]; )
	ALIGN16( float fdst1[COUNT]; )
	ALIGN16( float fsrc0[COUNT]; )
	ALIGN16( float fsrc1[COUNT]; )
	ALIGN16( idVec3 v3src0[COUNT]; )
	ALIGN16( idVec3 v3src1[COUNT]; )
	ALIGN16( idVec3 v3constant( 1.0f, 2.0f, 3.0f ); )
	ALIGN16( idPlane v4src0[COUNT]; )
	ALIGN16( idPlane v4constant(1.0f, 2.0f, 3.0f, 4.0f); )
	ALIGN16( idDrawVert drawVerts[COUNT]; )
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
		fsrc1[i] = srnd.CRandomFloat() * 10.0f;
		v3src0[i][0] = srnd.CRandomFloat() * 10.0f;
		v3src0[i][1] = srnd.CRandomFloat() * 10.0f;
		v3src0[i][2] = srnd.CRandomFloat() * 10.0f;
		v3src1[i][0] = srnd.CRandomFloat() * 10.0f;
		v3src1[i][1] = srnd.CRandomFloat() * 10.0f;
		v3src1[i][2] = srnd.CRandomFloat() * 10.0f;
		v4src0[i] = v3src0[i];
		v4src0[i][3] = srnd.CRandomFloat() * 10.0f;
		drawVerts[i].xyz = v3src0[i];
	}

	idLib::common->Printf("====================================\n" );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Dot( fdst0, v3constant, v3src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Dot( idVec3 * idVec3[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Dot( fdst1, v3constant, v3src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( idMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Dot( idVec3 * idVec3[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Dot( fdst0, v3constant, v4src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Dot( idVec3 * idPlane[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Dot( fdst1, v3constant, v4src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( idMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Dot( idVec3 * idPlane[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Dot( fdst0, v3constant, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Dot( idVec3 * idDrawVert[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Dot( fdst1, v3constant, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( idMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Dot( idVec3 * idDrawVert[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Dot( fdst0, v4constant, v3src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Dot( idPlane * idVec3[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Dot( fdst1, v4constant, v3src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( idMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Dot( idPlane * idVec3[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Dot( fdst0, v4constant, v4src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Dot( idPlane * idPlane[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Dot( fdst1, v4constant, v4src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( idMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Dot( idPlane * idPlane[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Dot( fdst0, v4constant, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Dot( idPlane * idDrawVert[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Dot( fdst1, v4constant, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( idMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Dot( idPlane * idDrawVert[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Dot( fdst0, v3src0, v3src1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Dot( idVec3[] * idVec3[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Dot( fdst1, v3src0, v3src1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( idMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-4f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Dot( idVec3[] * idVec3[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );


	idLib::common->Printf("====================================\n" );

	float dot1 = 0.0f, dot2 = 0.0f;
	for ( j = 0; j < 50 && j < COUNT; j++ ) {

		bestClocksGeneric = 0;
		for ( i = 0; i < NUMTESTS; i++ ) {
			StartRecordTime( start );
			p_generic->Dot( dot1, fsrc0, fsrc1, j );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		PrintClocks( va( "generic->Dot( float[%2d] * float[%2d] )", j, j ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( i = 0; i < NUMTESTS; i++ ) {
			StartRecordTime( start );
			p_simd->Dot( dot2, fsrc0, fsrc1, j );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}
		result = idMath::Fabs( dot1 - dot2 ) < 1e-4f ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->Dot( float[%2d] * float[%2d] ) %s", j, j, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

/*
============
TestCompare
============
*/
void TestCompare( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fsrc0[COUNT]; )
	ALIGN16( byte bytedst[COUNT]; )
	ALIGN16( byte bytedst2[COUNT]; )
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
	}

	idLib::common->Printf("====================================\n" );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->CmpGT( bytedst, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CmpGT( float[] >= float )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->CmpGT( bytedst2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( bytedst[i] != bytedst2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->CmpGT( float[] >= float ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		memset( bytedst, 0, COUNT );
		StartRecordTime( start );
		p_generic->CmpGT( bytedst, 2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CmpGT( 2, float[] >= float )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		memset( bytedst2, 0, COUNT );
		StartRecordTime( start );
		p_simd->CmpGT( bytedst2, 2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( bytedst[i] != bytedst2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->CmpGT( 2, float[] >= float ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	// ======================

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->CmpGE( bytedst, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CmpGE( float[] >= float )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->CmpGE( bytedst2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( bytedst[i] != bytedst2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->CmpGE( float[] >= float ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		memset( bytedst, 0, COUNT );
		StartRecordTime( start );
		p_generic->CmpGE( bytedst, 2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CmpGE( 2, float[] >= float )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		memset( bytedst2, 0, COUNT );
		StartRecordTime( start );
		p_simd->CmpGE( bytedst2, 2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( bytedst[i] != bytedst2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->CmpGE( 2, float[] >= float ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	// ======================

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->CmpLT( bytedst, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CmpLT( float[] >= float )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->CmpLT( bytedst2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( bytedst[i] != bytedst2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->CmpLT( float[] >= float ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		memset( bytedst, 0, COUNT );
		StartRecordTime( start );
		p_generic->CmpLT( bytedst, 2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CmpLT( 2, float[] >= float )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		memset( bytedst2, 0, COUNT );
		StartRecordTime( start );
		p_simd->CmpLT( bytedst2, 2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( bytedst[i] != bytedst2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->CmpLT( 2, float[] >= float ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		memset( bytedst, 0, COUNT );
		StartRecordTime( start );
		p_generic->SetCmpLT( bytedst, 2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->SetCmpLT( 2, float[] >= float )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		memset( bytedst2, 0, COUNT );
		StartRecordTime( start );
		p_simd->SetCmpLT( bytedst2, 2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( bytedst[i] != bytedst2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->SetCmpLT( 2, float[] >= float ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	// ======================

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->CmpLE( bytedst, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CmpLE( float[] >= float )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->CmpLE( bytedst2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( bytedst[i] != bytedst2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->CmpLE( float[] >= float ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		memset( bytedst, 0, COUNT );
		StartRecordTime( start );
		p_generic->CmpLE( bytedst, 2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CmpLE( 2, float[] >= float )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		memset( bytedst2, 0, COUNT );
		StartRecordTime( start );
		p_simd->CmpLE( bytedst2, 2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( bytedst[i] != bytedst2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->CmpLE( 2, float[] >= float ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestMinMax
============
*/
void TestMinMax( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fsrc0[COUNT]; )
	ALIGN16( idVec2 v2src0[COUNT]; )
	ALIGN16( idVec3 v3src0[COUNT]; )
	ALIGN16( idDrawVert drawVerts[COUNT]; )
	ALIGN16( shadowCache_t shadowVerts[COUNT]; )
	ALIGN16( vertIndex_t indexes[COUNT]; )
	float min = 0.0f, max = 0.0f, min2 = 0.0f, max2 = 0.0f;
	idVec2 v2min, v2max, v2min2, v2max2;
	idVec3 vmin, vmax, vmin2, vmax2;
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
		v2src0[i][0] = srnd.CRandomFloat() * 10.0f;
		v2src0[i][1] = srnd.CRandomFloat() * 10.0f;
		v3src0[i][0] = srnd.CRandomFloat() * 10.0f;
		v3src0[i][1] = srnd.CRandomFloat() * 10.0f;
		v3src0[i][2] = srnd.CRandomFloat() * 10.0f;
		drawVerts[i].xyz = v3src0[i];
		shadowVerts[i].xyz.ToVec3() = v3src0[i];
		indexes[i] = i;
	}

	idLib::common->Printf("====================================\n" );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		min = idMath::INFINITY;
		max = -idMath::INFINITY;
		StartRecordTime( start );
		p_generic->MinMax( min, max, fsrc0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MinMax( float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->MinMax( min2, max2, fsrc0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	result = ( min == min2 && max == max2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MinMax( float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->MinMax( v2min, v2max, v2src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MinMax( idVec2[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->MinMax( v2min2, v2max2, v2src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	result = ( v2min == v2min2 && v2max == v2max2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MinMax( idVec2[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->MinMax( vmin, vmax, v3src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MinMax( idVec3[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->MinMax( vmin2, vmax2, v3src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	result = ( vmin == vmin2 && vmax == vmax2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MinMax( idVec3[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->MinMax( vmin, vmax, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MinMax( idDrawVert[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->MinMax( vmin2, vmax2, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	result = ( vmin == vmin2 && vmax == vmax2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MinMax( idDrawVert[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );



	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->MinMax( vmin, vmax, shadowVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MinMax( shadowCache_t[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->MinMax( vmin2, vmax2, shadowVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	result = ( vmin == vmin2 && vmax == vmax2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MinMax( shadowCache_t[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );



	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->MinMax( vmin, vmax, drawVerts, indexes, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MinMax( idDrawVert[], indexes[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->MinMax( vmin2, vmax2, drawVerts, indexes, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	result = ( vmin == vmin2 && vmax == vmax2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MinMax( idDrawVert[], indexes[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestClamp
============
*/
void TestClamp( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fdst0[COUNT]; )
	ALIGN16( float fdst1[COUNT]; )
	ALIGN16( float fsrc0[COUNT]; )
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
	}

	idLib::common->Printf("====================================\n" );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Clamp( fdst0, fsrc0, -1.0f, 1.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Clamp( float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Clamp( fdst1, fsrc0, -1.0f, 1.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( fdst0[i] != fdst1[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Clamp( float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->ClampMin( fdst0, fsrc0, -1.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->ClampMin( float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->ClampMin( fdst1, fsrc0, -1.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( fdst0[i] != fdst1[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->ClampMin( float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->ClampMax( fdst0, fsrc0, 1.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->ClampMax( float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->ClampMax( fdst1, fsrc0, 1.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( fdst0[i] != fdst1[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->ClampMax( float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestMemcpy
============
*/
void TestMemcpy( void ) {
	int i, j;
	byte test0[8192];
	byte test1[8192];

	idRandom random( RANDOM_SEED );

	idLib::common->Printf("====================================\n" );

	for ( i = 5; i < 8192; i += 31 ) {
		for ( j = 0; j < i; j++ ) {
			test0[j] = random.RandomInt( 255 );
		}
		p_simd->Memcpy( test1, test0, 8192 );
		for ( j = 0; j < i; j++ ) {
			if ( test1[j] != test0[j] ) {
				idLib::common->Printf( "   simd->Memcpy() "S_COLOR_RED"X\n" );
				return;
			}
		}
	}
	idLib::common->Printf( "   simd->Memcpy() ok\n" );
}

/*
============
TestMemset
============
*/
void TestMemset( void ) {
	int i, j, k;
	byte test[8192];

	for ( i = 0; i < 8192; i++ ) {
		test[i] = 0;
	}

	for ( i = 5; i < 8192; i += 31 ) {
		for ( j = -1; j <= 1; j++ ) {
			p_simd->Memset( test, j, i );
			for ( k = 0; k < i; k++ ) {
				if ( test[k] != (byte)j ) {
					idLib::common->Printf( "   simd->Memset() "S_COLOR_RED"X\n" );
					return;
				}
			}
		}
	}
	idLib::common->Printf( "   simd->Memset() ok\n" );
}

#define	MATX_SIMD_EPSILON			1e-5f

/*
============
TestMatXMultiplyVecX
============
*/
void TestMatXMultiplyVecX( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	const char *result;
	idMatX mat;
	idVecX src(6);
	idVecX dst(6);
	idVecX tst(6);

	src[0] = 1.0f;
	src[1] = 2.0f;
	src[2] = 3.0f;
	src[3] = 4.0f;
	src[4] = 5.0f;
	src[5] = 6.0f;

	idLib::common->Printf("================= NxN * Nx1 ===================\n" );

	for ( i = 1; i <= 6; i++ ) {
		mat.Random( i, i, RANDOM_SEED, -10.0f, 10.0f );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_generic->MatX_MultiplyVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_MultiplyVecX %dx%d*%dx1", i, i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_simd->MatX_MultiplyVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_MultiplyVecX %dx%d*%dx1 %s", i, i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}

	idLib::common->Printf("================= Nx6 * 6x1 ===================\n" );

	for ( i = 1; i <= 6; i++ ) {
		mat.Random( i, 6, RANDOM_SEED, -10.0f, 10.0f );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_generic->MatX_MultiplyVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_MultiplyVecX %dx6*6x1", i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_simd->MatX_MultiplyVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_MultiplyVecX %dx6*6x1 %s", i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}

	idLib::common->Printf("================= 6xN * Nx1 ===================\n" );

	for ( i = 1; i <= 6; i++ ) {
		mat.Random( 6, i, RANDOM_SEED, -10.0f, 10.0f );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_generic->MatX_MultiplyVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_MultiplyVecX 6x%d*%dx1", i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_simd->MatX_MultiplyVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_MultiplyVecX 6x%d*%dx1 %s", i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

/*
============
TestMatXMultiplyAddVecX
============
*/
void TestMatXMultiplyAddVecX( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	const char *result;
	idMatX mat;
	idVecX src(6);
	idVecX dst(6);
	idVecX tst(6);

	src[0] = 1.0f;
	src[1] = 2.0f;
	src[2] = 3.0f;
	src[3] = 4.0f;
	src[4] = 5.0f;
	src[5] = 6.0f;

	idLib::common->Printf("================= NxN * Nx1 ===================\n" );

	for ( i = 1; i <= 6; i++ ) {
		mat.Random( i, i, RANDOM_SEED, -10.0f, 10.0f );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_generic->MatX_MultiplyAddVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_MultiplyAddVecX %dx%d*%dx1", i, i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_simd->MatX_MultiplyAddVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_MultiplyAddVecX %dx%d*%dx1 %s", i, i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}

	idLib::common->Printf("================= Nx6 * 6x1 ===================\n" );

	for ( i = 1; i <= 6; i++ ) {
		mat.Random( i, 6, RANDOM_SEED, -10.0f, 10.0f );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_generic->MatX_MultiplyAddVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_MultiplyAddVecX %dx6*6x1", i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_simd->MatX_MultiplyAddVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_MultiplyAddVecX %dx6*6x1 %s", i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}

	idLib::common->Printf("================= 6xN * Nx1 ===================\n" );

	for ( i = 1; i <= 6; i++ ) {
		mat.Random( 6, i, RANDOM_SEED, -10.0f, 10.0f );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_generic->MatX_MultiplyAddVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_MultiplyAddVecX 6x%d*%dx1", i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_simd->MatX_MultiplyAddVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_MultiplyAddVecX 6x%d*%dx1 %s", i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

/*
============
TestMatXTransposeMultiplyVecX
============
*/
void TestMatXTransposeMultiplyVecX( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	const char *result;
	idMatX mat;
	idVecX src(6);
	idVecX dst(6);
	idVecX tst(6);

	src[0] = 1.0f;
	src[1] = 2.0f;
	src[2] = 3.0f;
	src[3] = 4.0f;
	src[4] = 5.0f;
	src[5] = 6.0f;

	idLib::common->Printf("================= Nx6 * Nx1 ===================\n" );

	for ( i = 1; i <= 6; i++ ) {
		mat.Random( i, 6, RANDOM_SEED, -10.0f, 10.0f );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_generic->MatX_TransposeMultiplyVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_TransposeMulVecX %dx6*%dx1", i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_simd->MatX_TransposeMultiplyVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_TransposeMulVecX %dx6*%dx1 %s", i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}

	idLib::common->Printf("================= 6xN * 6x1 ===================\n" );

	for ( i = 1; i <= 6; i++ ) {
		mat.Random( 6, i, RANDOM_SEED, -10.0f, 10.0f );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_generic->MatX_TransposeMultiplyVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_TransposeMulVecX 6x%d*6x1", i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_simd->MatX_TransposeMultiplyVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_TransposeMulVecX 6x%d*6x1 %s", i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

/*
============
TestMatXTransposeMultiplyAddVecX
============
*/
void TestMatXTransposeMultiplyAddVecX( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	const char *result;
	idMatX mat;
	idVecX src(6);
	idVecX dst(6);
	idVecX tst(6);

	src[0] = 1.0f;
	src[1] = 2.0f;
	src[2] = 3.0f;
	src[3] = 4.0f;
	src[4] = 5.0f;
	src[5] = 6.0f;

	idLib::common->Printf("================= Nx6 * Nx1 ===================\n" );

	for ( i = 1; i <= 6; i++ ) {
		mat.Random( i, 6, RANDOM_SEED, -10.0f, 10.0f );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_generic->MatX_TransposeMultiplyAddVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_TransposeMulAddVecX %dx6*%dx1", i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_simd->MatX_TransposeMultiplyAddVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_TransposeMulAddVecX %dx6*%dx1 %s", i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}

	idLib::common->Printf("================= 6xN * 6x1 ===================\n" );

	for ( i = 1; i <= 6; i++ ) {
		mat.Random( 6, i, RANDOM_SEED, -10.0f, 10.0f );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_generic->MatX_TransposeMultiplyAddVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_TransposeMulAddVecX 6x%d*6x1", i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_simd->MatX_TransposeMultiplyAddVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_TransposeMulAddVecX 6x%d*6x1 %s", i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

/*
============
TestMatXMultiplyMatX
============
*/
#define TEST_VALUE_RANGE			10.0f
#define	MATX_MATX_SIMD_EPSILON		1e-4f

void TestMatXMultiplyMatX( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	const char *result;
	idMatX m1, m2, dst, tst;

	idLib::common->Printf("================= NxN * Nx6 ===================\n" );

	// NxN * Nx6
	for ( i = 1; i <= 5; i++ ) {
		m1.Random( i, i, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		m2.Random( i, 6, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		dst.SetSize( i, 6 );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_generic->MatX_MultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_MultiplyMatX %dx%d*%dx6", i, i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_simd->MatX_MultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_MultiplyMatX %dx%d*%dx6 %s", i, i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}

	idLib::common->Printf("================= 6xN * Nx6 ===================\n" );

	// 6xN * Nx6
	for ( i = 1; i <= 5; i++ ) {
		m1.Random( 6, i, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		m2.Random( i, 6, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		dst.SetSize( 6, 6 );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_generic->MatX_MultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_MultiplyMatX 6x%d*%dx6", i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_simd->MatX_MultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_MultiplyMatX 6x%d*%dx6 %s", i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}

	idLib::common->Printf("================= Nx6 * 6xN ===================\n" );

	// Nx6 * 6xN
	for ( i = 1; i <= 5; i++ ) {
		m1.Random( i, 6, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		m2.Random( 6, i, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		dst.SetSize( i, i );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_generic->MatX_MultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_MultiplyMatX %dx6*6x%d", i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_simd->MatX_MultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_MultiplyMatX %dx6*6x%d %s", i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}

	idLib::common->Printf("================= 6x6 * 6xN ===================\n" );

	// 6x6 * 6xN
	for ( i = 1; i <= 6; i++ ) {
		m1.Random( 6, 6, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		m2.Random( 6, i, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		dst.SetSize( 6, i );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_generic->MatX_MultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_MultiplyMatX 6x6*6x%d", i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_simd->MatX_MultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_MultiplyMatX 6x6*6x%d %s", i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

/*
============
TestMatXTransposeMultiplyMatX
============
*/
void TestMatXTransposeMultiplyMatX( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	const char *result;
	idMatX m1, m2, dst, tst;

	idLib::common->Printf("================= Nx6 * NxN ===================\n" );

	// Nx6 * NxN
	for ( i = 1; i <= 5; i++ ) {
		m1.Random( i, 6, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		m2.Random( i, i, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		dst.SetSize( 6, i );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_generic->MatX_TransposeMultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_TransMultiplyMatX %dx6*%dx%d", i, i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_simd->MatX_TransposeMultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_TransMultiplyMatX %dx6*%dx%d %s", i, i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}

	idLib::common->Printf("================= 6xN * 6x6 ===================\n" );

	// 6xN * 6x6
	for ( i = 1; i <= 6; i++ ) {
		m1.Random( 6, i, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		m2.Random( 6, 6, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		dst.SetSize( i, 6 );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_generic->MatX_TransposeMultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_TransMultiplyMatX 6x%d*6x6", i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_simd->MatX_TransposeMultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_TransMultiplyMatX 6x%d*6x6 %s", i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

#define MATX_TRIANGULAR_SOLVE_SIMD_EPSILON		0.1f
#define MATX_TRIANGULAR_SOLVE_SIZE				100

/*
============
TestMatXLowerTriangularSolve
============
*/
void TestMatXLowerTriangularSolve( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	const char *result;
	idMatX L;
	idVecX x, b, tst;

	idLib::common->Printf("====================================\n" );

	L.Random( MATX_TRIANGULAR_SOLVE_SIZE, MATX_TRIANGULAR_SOLVE_SIZE, 0, -1.0f, 1.0f );
	x.SetSize( MATX_TRIANGULAR_SOLVE_SIZE );
	b.Random( MATX_TRIANGULAR_SOLVE_SIZE, 0, -1.0f, 1.0f );

	for ( i = 1; i < MATX_TRIANGULAR_SOLVE_SIZE; i++ ) {

		x.Zero( i );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_generic->MatX_LowerTriangularSolve( L, x.ToFloatPtr(), b.ToFloatPtr(), i );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = x;
		x.Zero();

		PrintClocks( va( "generic->MatX_LowerTriangularSolve %dx%d", i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_simd->MatX_LowerTriangularSolve( L, x.ToFloatPtr(), b.ToFloatPtr(), i );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = x.Compare( tst, MATX_TRIANGULAR_SOLVE_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_LowerTriangularSolve %dx%d %s", i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

/*
============
TestMatXLowerTriangularSolveTranspose
============
*/
void TestMatXLowerTriangularSolveTranspose( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	const char *result;
	idMatX L;
	idVecX x, b, tst;

	idLib::common->Printf("====================================\n" );

	L.Random( MATX_TRIANGULAR_SOLVE_SIZE, MATX_TRIANGULAR_SOLVE_SIZE, 0, -1.0f, 1.0f );
	x.SetSize( MATX_TRIANGULAR_SOLVE_SIZE );
	b.Random( MATX_TRIANGULAR_SOLVE_SIZE, 0, -1.0f, 1.0f );

	for ( i = 1; i < MATX_TRIANGULAR_SOLVE_SIZE; i++ ) {

		x.Zero( i );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_generic->MatX_LowerTriangularSolveTranspose( L, x.ToFloatPtr(), b.ToFloatPtr(), i );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = x;
		x.Zero();

		PrintClocks( va( "generic->MatX_LowerTriangularSolveT %dx%d", i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_simd->MatX_LowerTriangularSolveTranspose( L, x.ToFloatPtr(), b.ToFloatPtr(), i );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = x.Compare( tst, MATX_TRIANGULAR_SOLVE_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_LowerTriangularSolveT %dx%d %s", i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

/*
============
TestMatXUpperTriangularSolve
============
*/
void TestMatXUpperTriangularSolve( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	const char *result;
	idMatX L;
	idVecX x, b, tst;

	idLib::common->Printf("====================================\n" );

	L.Random( MATX_TRIANGULAR_SOLVE_SIZE, MATX_TRIANGULAR_SOLVE_SIZE, 0, -1.0f, 1.0f );
	x.SetSize( MATX_TRIANGULAR_SOLVE_SIZE );
	b.Random( MATX_TRIANGULAR_SOLVE_SIZE, 0, -1.0f, 1.0f );

	for ( i = 1; i < MATX_TRIANGULAR_SOLVE_SIZE; i++ ) {

		x.Zero( i );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_generic->MatX_UpperTriangularSolve( L, x.ToFloatPtr(), b.ToFloatPtr(), i );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = x;
		x.Zero();

		PrintClocks( va( "generic->MatX_UpperTriangularSolve %dx%d", i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_simd->MatX_UpperTriangularSolve( L, x.ToFloatPtr(), b.ToFloatPtr(), i );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = x.Compare( tst, MATX_TRIANGULAR_SOLVE_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_UpperTriangularSolve %dx%d %s", i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

/*
============
TestMatXUpperTriangularSolveTranspose
============
*/
void TestMatXUpperTriangularSolveTranspose( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	const char *result;
	idMatX L;
	idVecX x, b, tst;

	idLib::common->Printf("====================================\n" );

	L.Random( MATX_TRIANGULAR_SOLVE_SIZE, MATX_TRIANGULAR_SOLVE_SIZE, 0, -1.0f, 1.0f );
	x.SetSize( MATX_TRIANGULAR_SOLVE_SIZE );
	b.Random( MATX_TRIANGULAR_SOLVE_SIZE, 0, -1.0f, 1.0f );

	for ( i = 1; i < MATX_TRIANGULAR_SOLVE_SIZE; i++ ) {

		x.Zero( i );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_generic->MatX_UpperTriangularSolveTranspose( L, x.ToFloatPtr(), b.ToFloatPtr(), i );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = x;
		x.Zero();

		PrintClocks( va( "generic->MatX_UpperTriangularSolveT %dx%d", i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_simd->MatX_UpperTriangularSolveTranspose( L, x.ToFloatPtr(), b.ToFloatPtr(), i );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = x.Compare( tst, MATX_TRIANGULAR_SOLVE_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_UpperTriangularSolveT %dx%d %s", i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

#define MATX_FACTOR_SIMD_EPSILON			0.1f
#define MATX_FACTOR_SOLVE_SIZE				64

/*
============
TestMatXLUFactor
============
*/
void TestMatXLUFactor( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	const char *result;
	idMatX src, original, mat1, mat2;
	idVecX invDiag1, invDiag2;

	idLib::common->Printf("====================================\n" );

	original.SetSize( MATX_FACTOR_SOLVE_SIZE, MATX_FACTOR_SOLVE_SIZE );
	src.Random( MATX_FACTOR_SOLVE_SIZE, MATX_FACTOR_SOLVE_SIZE, 0, -1.0f, 1.0f );
	src.TransposeMultiply( original, src );

	for ( i = 1; i < MATX_FACTOR_SOLVE_SIZE; i++ ) {

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			mat1 = original;
			invDiag1.Zero( MATX_FACTOR_SOLVE_SIZE );
			StartRecordTime( start );
			p_generic->MatX_LU_Factor( mat1, invDiag1, i );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}

		PrintClocks( va( "generic->MatX_LU_Factor %dx%d", i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			mat2 = original;
			invDiag2.Zero( MATX_FACTOR_SOLVE_SIZE );
			StartRecordTime( start );
			p_simd->MatX_LU_Factor( mat2, invDiag2, i );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = mat1.Compare( mat2, MATX_FACTOR_SIMD_EPSILON ) && invDiag1.Compare( invDiag2, MATX_FACTOR_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_LU_Factor %dx%d %s", i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

/*
============
TestMatXLDLTFactor
============
*/
void TestMatXLDLTFactor( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	const char *result;
	idMatX src, original, mat1, mat2;
	idVecX invDiag1, invDiag2;

	idLib::common->Printf("====================================\n" );

	original.SetSize( MATX_FACTOR_SOLVE_SIZE, MATX_FACTOR_SOLVE_SIZE );
	src.Random( MATX_FACTOR_SOLVE_SIZE, MATX_FACTOR_SOLVE_SIZE, 0, -1.0f, 1.0f );
	src.TransposeMultiply( original, src );

	for ( i = 1; i < MATX_FACTOR_SOLVE_SIZE; i++ ) {

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			mat1 = original;
			invDiag1.Zero( MATX_FACTOR_SOLVE_SIZE );
			StartRecordTime( start );
			p_generic->MatX_LDLT_Factor( mat1, invDiag1, i );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}

		PrintClocks( va( "generic->MatX_LDLT_Factor %dx%d", i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			mat2 = original;
			invDiag2.Zero( MATX_FACTOR_SOLVE_SIZE );
			StartRecordTime( start );
			p_simd->MatX_LDLT_Factor( mat2, invDiag2, i );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = mat1.Compare( mat2, MATX_FACTOR_SIMD_EPSILON ) && invDiag1.Compare( invDiag2, MATX_FACTOR_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_LDLT_Factor %dx%d %s", i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

/*
============
TestBlendJoints
============
*/
void TestBlendJoints( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( idCompressedJointQuat compJoints[COUNT]; )
	ALIGN16( idJointQuat baseJoints[COUNT]; )
	ALIGN16( idJointQuat joints1[COUNT]; )
	ALIGN16( idJointQuat joints2[COUNT]; )
	ALIGN16( idJointQuat blendJoints[COUNT]; )
	ALIGN16( int index[COUNT]; )
	float lerp = 0.3f;
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		idAngles angles;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		baseJoints[i].q = angles.ToQuat();
		baseJoints[i].t[0] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].t[1] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].t[2] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].w = 0.0f;

		idCQuat q = baseJoints[i].q.ToCQuat();
		idVec3 t = baseJoints[i].t;
		t.FixDenormals();
		q.FixDenormals();
		compJoints[i].t[0] = idCompressedJointQuat::OffsetToShort( t.x );
		compJoints[i].t[1] = idCompressedJointQuat::OffsetToShort( t.y );
		compJoints[i].t[2] = idCompressedJointQuat::OffsetToShort( t.z );
		compJoints[i].q[0] = idCompressedJointQuat::QuatToShort( q.x );
		compJoints[i].q[1] = idCompressedJointQuat::QuatToShort( q.y );
		compJoints[i].q[2] = idCompressedJointQuat::QuatToShort( q.z );

		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		blendJoints[i].q = angles.ToQuat();
		blendJoints[i].t[0] = srnd.CRandomFloat() * 10.0f;
		blendJoints[i].t[1] = srnd.CRandomFloat() * 10.0f;
		blendJoints[i].t[2] = srnd.CRandomFloat() * 10.0f;
		blendJoints[i].w = 0.0f;
		index[i] = i;
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < COUNT; j++ ) {
			joints1[j] = baseJoints[j];
		}
		StartRecordTime( start );
		p_generic->BlendJoints( joints1, blendJoints, lerp, index, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->BlendJoints()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < COUNT; j++ ) {
			joints2[j] = baseJoints[j];
		}
		StartRecordTime( start );
		p_simd->BlendJoints( joints2, blendJoints, lerp, index, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( !joints1[i].q.Compare( joints2[i].q, 1e-2f ) ) {
			break;
		}
		if ( !joints1[i].t.Compare( joints2[i].t, 1e-3f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->BlendJoints() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < COUNT; j++ ) {
			joints1[j] = baseJoints[j];
		}
		StartRecordTime( start );
		p_generic->BlendJointsFast( joints1, blendJoints, lerp, index, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->BlendJointsFast()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < COUNT; j++ ) {
			joints2[j] = baseJoints[j];
		}
		StartRecordTime( start );
		p_simd->BlendJointsFast( joints2, blendJoints, lerp, index, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( !joints1[i].q.Compare( joints2[i].q, 1e-2f ) ) {
			break;
		}
		if ( !joints1[i].t.Compare( joints2[i].t, 1e-3f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->BlendJointsFast() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );



	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->DecompressJoints(joints1, compJoints, index, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->DecompressJoints()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->DecompressJoints( joints2, compJoints, index, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( !joints1[i].q.Compare( joints2[i].q, 1e-2f ) ) {
			break;
		}
		if ( !joints1[i].t.Compare( joints2[i].t, 1e-3f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->DecompressJoints() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

}

/*
============
TestConvertJointQuatsToJointMats
============
*/
void TestConvertJointQuatsToJointMats( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( idJointQuat baseJoints[COUNT]; )
	ALIGN16( idJointMat joints1[COUNT]; )
	ALIGN16( idJointMat joints2[COUNT]; )
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		idAngles angles;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		baseJoints[i].q = angles.ToQuat();
		baseJoints[i].t[0] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].t[1] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].t[2] = srnd.CRandomFloat() * 10.0f;
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->ConvertJointQuatsToJointMats( joints1, baseJoints, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->ConvertJointQuatsToJointMats()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->ConvertJointQuatsToJointMats( joints2, baseJoints, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( !joints1[i].Compare( joints2[i], 1e-4f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->ConvertJointQuatsToJointMats() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestConvertJointMatsToJointQuats
============
*/
void TestConvertJointMatsToJointQuats( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( idJointMat baseJoints[COUNT]; )
	ALIGN16( idJointQuat joints1[COUNT]; )
	ALIGN16( idJointQuat joints2[COUNT]; )
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		idAngles angles;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		baseJoints[i].SetRotation( angles.ToMat3() );
		idVec3 v;
		v[0] = srnd.CRandomFloat() * 10.0f;
		v[1] = srnd.CRandomFloat() * 10.0f;
		v[2] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].SetTranslation( v );
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->ConvertJointMatsToJointQuats( joints1, baseJoints, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->ConvertJointMatsToJointQuats()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->ConvertJointMatsToJointQuats( joints2, baseJoints, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( !joints1[i].q.Compare( joints2[i].q, 1e-3f ) ) {
			idLib::common->Printf("ConvertJointMatsToJointQuats: broken q %i\n", i );
			break;
		}
		if ( !joints1[i].t.Compare( joints2[i].t, 1e-3f ) ) {
			idLib::common->Printf("ConvertJointMatsToJointQuats: broken t %i\n", i );
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->ConvertJointMatsToJointQuats() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestTransformJoints
============
*/
void TestTransformJoints( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( idJointMat joints[COUNT+1]; )
	ALIGN16( idJointMat joints1[COUNT+1]; )
	ALIGN16( idJointMat joints2[COUNT+1]; )
	ALIGN16( int parents[COUNT+1]; )
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i <= COUNT; i++ ) {
		idAngles angles;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		joints[i].SetRotation( angles.ToMat3() );
		idVec3 v;
		v[0] = srnd.CRandomFloat() * 2.0f;
		v[1] = srnd.CRandomFloat() * 2.0f;
		v[2] = srnd.CRandomFloat() * 2.0f;
		joints[i].SetTranslation( v );
		parents[i] = i - 1;
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j <= COUNT; j++ ) {
			joints1[j] = joints[j];
		}
		StartRecordTime( start );
		p_generic->TransformJoints( joints1, parents, 1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->TransformJoints()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j <= COUNT; j++ ) {
			joints2[j] = joints[j];
		}
		StartRecordTime( start );
		p_simd->TransformJoints( joints2, parents, 1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( !joints1[i+1].Compare( joints2[i+1], 1e-4f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->TransformJoints() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestUntransformJoints
============
*/
void TestUntransformJoints( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( idJointMat joints[COUNT+1]; )
	ALIGN16( idJointMat joints1[COUNT+1]; )
	ALIGN16( idJointMat joints2[COUNT+1]; )
	ALIGN16( int parents[COUNT+1]; )
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i <= COUNT; i++ ) {
		idAngles angles;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		joints[i].SetRotation( angles.ToMat3() );
		idVec3 v;
		v[0] = srnd.CRandomFloat() * 2.0f;
		v[1] = srnd.CRandomFloat() * 2.0f;
		v[2] = srnd.CRandomFloat() * 2.0f;
		joints[i].SetTranslation( v );
		parents[i] = i - 1;
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j <= COUNT; j++ ) {
			joints1[j] = joints[j];
		}
		StartRecordTime( start );
		p_generic->UntransformJoints( joints1, parents, 1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->UntransformJoints()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j <= COUNT; j++ ) {
			joints2[j] = joints[j];
		}
		StartRecordTime( start );
		p_simd->UntransformJoints( joints2, parents, 1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( !joints1[i+1].Compare( joints2[i+1], 1e-4f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->UntransformJoints() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestMultiplyJoints
============
*/
void TestMultiplyJoints( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( idJointMat results1[COUNT]; )
	ALIGN16( idJointMat results2[COUNT]; )
	ALIGN16( idJointMat joints1[COUNT]; )
	ALIGN16( idJointMat joints2[COUNT]; )
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		idAngles angles;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		joints1[i].SetRotation( angles.ToMat3() );
		idVec3 v;
		v[0] = srnd.CRandomFloat() * 2.0f;
		v[1] = srnd.CRandomFloat() * 2.0f;
		v[2] = srnd.CRandomFloat() * 2.0f;
		joints1[i].SetTranslation( v );
		joints2[i] = joints1[i];
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->MultiplyJoints( results1, joints1, joints2, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MultiplyJoints()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->MultiplyJoints( results2, joints1, joints2, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( !results1[i].Compare( results2[i], 1e-4f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MultiplyJoints() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestTransformVerts
============
*/
#define NUMJOINTS			64
#define NUMVERTS			COUNT
#define NUMWEIGHTSPERVERT	2
#define NUMWEIGHTS			(NUMVERTS*NUMWEIGHTSPERVERT)

void TestTransformVerts( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( idDrawVert baseVerts[NUMVERTS]; )
	ALIGN16( idDrawVert drawVerts1[NUMVERTS]; )
	ALIGN16( idDrawVert drawVerts2[NUMVERTS]; )
	ALIGN16( shadowCache_t shadowVerts1[NUMVERTS]; )
	ALIGN16( shadowCache_t shadowVerts2[NUMVERTS]; )
	ALIGN16( idJointMat joints[NUMJOINTS]; )
	ALIGN16( idVec4 base[NUMWEIGHTS*3]; )
	ALIGN16( jointWeight_t weights[NUMWEIGHTS]; )
	ALIGN16( short		maxWeights[NUMVERTS]; )
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < NUMJOINTS; i++ ) {
		idAngles angles;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		joints[i].SetRotation( angles.ToMat3() );
		idVec3 v;
		v[0] = srnd.CRandomFloat() * 2.0f;
		v[1] = srnd.CRandomFloat() * 2.0f;
		v[2] = srnd.CRandomFloat() * 2.0f;
		joints[i].SetTranslation( v );
	}

	for ( i = 0; i < NUMWEIGHTS*3; i++ ) {
		base[i][0] = srnd.CRandomFloat() * 2.0f;
		base[i][1] = srnd.CRandomFloat() * 2.0f;
		base[i][2] = srnd.CRandomFloat() * 2.0f;
		base[i][3] = srnd.CRandomFloat();
	}

	for ( i = 0; i < NUMVERTS; i++ ) {
		baseVerts[i].xyz[0] = srnd.CRandomFloat() * 2.0f;
		baseVerts[i].xyz[1] = srnd.CRandomFloat() * 2.0f;
		baseVerts[i].xyz[2] = srnd.CRandomFloat() * 2.0f;
	}

	for ( i = 0; i < NUMWEIGHTS; i++ ) {
		weights[i].weight = srnd.CRandomFloat();
		weights[i].jointMatOffset = idMath::ClampInt( 0, NUMJOINTS-1, i * NUMJOINTS / NUMWEIGHTS + 0 ) * JOINTMAT_SIZE;
		weights[i].nextVertexOffset = ( NUMWEIGHTSPERVERT - ( i % NUMWEIGHTSPERVERT ) ) * JOINTWEIGHT_SIZE;
	}

	for ( i = 0; i < NUMVERTS; i++ ) {
		maxWeights[i] = idMath::ClampInt( 0, NUMJOINTS-1, i * NUMJOINTS / NUMWEIGHTS + 0 ) * JOINTMAT_SIZE;
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->TransformVerts( drawVerts1, NUMVERTS, joints, base, weights, NUMWEIGHTS );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->TransformVerts()", NUMVERTS, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->TransformVerts( drawVerts2, NUMVERTS, joints, base, weights, NUMWEIGHTS );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < NUMVERTS; i++ ) {
		if ( !drawVerts1[i].xyz.Compare( drawVerts2[i].xyz, 0.5f ) ) {
			break;
		}
	}
	result = ( i >= NUMVERTS ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->TransformVerts() %s", result ), NUMVERTS, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->TransformShadowVerts( drawVerts1, NUMVERTS, joints, baseVerts, weights, NUMWEIGHTS );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->TransformShadowVerts()", NUMVERTS, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->TransformShadowVerts( drawVerts2, NUMVERTS, joints, baseVerts, weights, NUMWEIGHTS );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < NUMVERTS; i++ ) {
		if ( !drawVerts1[i].xyz.Compare( drawVerts2[i].xyz, 0.5f ) ) {
			break;
		}
	}
	result = ( i >= NUMVERTS ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->TransformShadowVerts() %s", result ), NUMVERTS, bestClocksSIMD, bestClocksGeneric );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->TransformShadowVerts( drawVerts1, NUMVERTS, joints, baseVerts, maxWeights, NUMWEIGHTS );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->TransformShadowVerts()", NUMVERTS, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->TransformShadowVerts( drawVerts2, NUMVERTS, joints, baseVerts, maxWeights, NUMWEIGHTS );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < NUMVERTS; i++ ) {
		if ( !drawVerts1[i].xyz.Compare( drawVerts2[i].xyz, 0.5f ) ) {
			break;
		}
	}
	result = ( i >= NUMVERTS ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->TransformShadowVerts() %s", result ), NUMVERTS, bestClocksSIMD, bestClocksGeneric );



	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->TransformShadowVerts( shadowVerts1, NUMVERTS, joints, baseVerts, maxWeights, NUMWEIGHTS );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->TransformShadowVerts()", NUMVERTS, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->TransformShadowVerts( shadowVerts2, NUMVERTS, joints, baseVerts, maxWeights, NUMWEIGHTS );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < NUMVERTS; i++ ) {
		if ( !shadowVerts1[i].xyz.ToVec3().Compare( shadowVerts2[i].xyz.ToVec3(), 0.5f ) ) {
			break;
		}
	}
	result = ( i >= NUMVERTS ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->TransformShadowVerts() %s", result ), NUMVERTS, bestClocksSIMD, bestClocksGeneric );

/*
	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->TransformVertsAndTangents( drawVerts1, NUMVERTS, joints, base, weights, NUMWEIGHTS );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->TransformVertsAndTangents()", NUMVERTS, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->TransformVertsAndTangents( drawVerts2, NUMVERTS, joints, base, weights, NUMWEIGHTS );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < NUMVERTS; i++ ) {
		if ( !drawVerts1[i].xyz.Compare( drawVerts2[i].xyz, 0.5f ) ) {
			break;
		}
		if ( !drawVerts1[i].normal.Compare( drawVerts2[i].normal, 0.1f ) ) {
			break;
		}
		if ( !drawVerts1[i].tangent.ToVec3().Compare( drawVerts2[i].tangent.ToVec3(), 0.1f ) ) {
			break;
		}
	}
	result = ( i >= NUMVERTS ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->TransformVertsAndTangents() %s", result ), NUMVERTS, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->TransformVertsAndTangentsFast( drawVerts1, NUMVERTS, joints, base, weights, NUMWEIGHTS );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->TransformVertsAndTangentsFast()", NUMVERTS, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->TransformVertsAndTangentsFast( drawVerts2, NUMVERTS, joints, base, weights, NUMWEIGHTS );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < NUMVERTS; i++ ) {
		if ( !drawVerts1[i].xyz.Compare( drawVerts2[i].xyz, 0.5f ) ) {
			break;
		}
		if ( !drawVerts1[i].normal.Compare( drawVerts2[i].normal, 0.1f ) ) {
			break;
		}
		if ( !drawVerts1[i].tangent.ToVec3().Compare( drawVerts2[i].tangent.ToVec3(), 0.1f ) ) {
			break;
		}
	}
	result = ( i >= NUMVERTS ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->TransformVertsAndTangentsFast() %s", result ), NUMVERTS, bestClocksSIMD, bestClocksGeneric );
*/
}

#if SD_SUPPORT_UNSMOOTHEDTANGENTS
/*
============
TestDeriveUnsmoothedTangents
============
*/
void TestDeriveUnsmoothedTangents( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( idDrawVert drawVerts1[COUNT]; )
	ALIGN16( idDrawVert drawVerts2[COUNT]; )
	ALIGN16( dominantTri_s dominantTris[COUNT] );
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			drawVerts1[i].xyz[j] = srnd.CRandomFloat() * 10.0f;
		}
		drawVerts1[i].SetST( srnd.CRandomFloat(), srnd.CRandomFloat() );
		drawVerts2[i] = drawVerts1[i];

		dominantTris[i].v2 = ( i + 1 + srnd.RandomInt( 8 ) ) % COUNT;
		dominantTris[i].v3 = ( i + 9 + srnd.RandomInt( 8 ) ) % COUNT;
		dominantTris[i].normalizationScale[0] = srnd.CRandomFloat();
		dominantTris[i].normalizationScale[1] = srnd.CRandomFloat();
		dominantTris[i].normalizationScale[2] = srnd.CRandomFloat();
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->DeriveUnsmoothedTangents( drawVerts1, dominantTris, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->DeriveUnsmoothedTangents()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->DeriveUnsmoothedTangents( drawVerts2, dominantTris, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		idVec3 v1, v2;

		v1 = drawVerts1[i].GetNormal();
		v1.Normalize();
		v2 = drawVerts2[i].GetNormal();
		v2.Normalize();
		if ( !v1.Compare( v2, 1e-1f ) ) {
			break;
		}
		v1 = drawVerts1[i].GetTangent();
		v1.Normalize();
		v2 = drawVerts2[i].GetTangent();
		v2.Normalize();
		if ( !v1.Compare( v2, 1e-1f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->DeriveUnsmoothedTangents() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}
#endif // SD_SUPPORT_UNSMOOTHEDTANGENTS

/*
============
TestTracePointCull
============
*/
void TestTracePointCull( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( idPlane planes[4]; )
	ALIGN16( idDrawVert drawVerts[COUNT]; )
	ALIGN16( shadowCache_t shadowVerts[COUNT]; )
	ALIGN16( byte cullBits1[COUNT]; )
	ALIGN16( byte cullBits2[COUNT]; )
	byte totalOr1 = 0, totalOr2 = 0;
	const char *result;

	idRandom srnd( RANDOM_SEED );

	planes[0].SetNormal( idVec3(  1,  0, 0 ) );
	planes[1].SetNormal( idVec3( -1,  0, 0 ) );
	planes[2].SetNormal( idVec3(  0,  1, 0 ) );
	planes[3].SetNormal( idVec3(  0, -1, 0 ) );
	planes[0][3] = -5.3f;
	planes[1][3] = 5.3f;
	planes[2][3] = -3.4f;
	planes[3][3] = 3.4f;

	for ( i = 0; i < COUNT; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			drawVerts[i].xyz[j] = srnd.CRandomFloat() * 10.0f;
			shadowVerts[i].xyz[j] = drawVerts[i].xyz[j];
		}
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->TracePointCull( cullBits1, totalOr1, 0.0f, planes, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->TracePointCull()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->TracePointCull( cullBits2, totalOr2, 0.0f, planes, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( cullBits1[i] != cullBits2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT && totalOr1 == totalOr2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->TracePointCull() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );



	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->TracePointCullShadowVerts( cullBits1, totalOr1, 0.0f, planes, shadowVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->TracePointCull()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->TracePointCullShadowVerts( cullBits2, totalOr2, 0.0f, planes, shadowVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( cullBits1[i] != cullBits2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT && totalOr1 == totalOr2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->TracePointCull() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestDecalPointCull
============
*/
void TestDecalPointCull( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( idPlane planes[6]; )
	ALIGN16( idDrawVert drawVerts[COUNT]; )
	ALIGN16( byte cullBits1[COUNT]; )
	ALIGN16( byte cullBits2[COUNT]; )
	const char *result;

	idRandom srnd( RANDOM_SEED );

	planes[0].SetNormal( idVec3(  1,  0,  0 ) );
	planes[1].SetNormal( idVec3( -1,  0,  0 ) );
	planes[2].SetNormal( idVec3(  0,  1,  0 ) );
	planes[3].SetNormal( idVec3(  0, -1,  0 ) );
	planes[4].SetNormal( idVec3(  0,  0,  1 ) );
	planes[5].SetNormal( idVec3(  0,  0, -1 ) );
	planes[0][3] = -5.3f;
	planes[1][3] = 5.3f;
	planes[2][3] = -4.4f;
	planes[3][3] = 4.4f;
	planes[4][3] = -3.5f;
	planes[5][3] = 3.5f;

	for ( i = 0; i < COUNT; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			drawVerts[i].xyz[j] = srnd.CRandomFloat() * 10.0f;
		}
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->DecalPointCull( cullBits1, planes, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->DecalPointCull()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->DecalPointCull( cullBits2, planes, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( cullBits1[i] != cullBits2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->DecalPointCull() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestOverlayPointCull
============
*/
void TestOverlayPointCull( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( idPlane planes[2]; )
	ALIGN16( idDrawVert drawVerts[COUNT]; )
	ALIGN16( shadowCache_t shadowVerts[COUNT]; )
	ALIGN16( byte cullBits1[COUNT]; )
	ALIGN16( byte cullBits2[COUNT]; )
	ALIGN16( idVec2 texCoords1[COUNT]; )
	ALIGN16( idVec2 texCoords2[COUNT]; )
	const char *result;

	idRandom srnd( RANDOM_SEED );

	planes[0].SetNormal( idVec3( 0.3f, 0.2f, 0.9f ) );
	planes[1].SetNormal( idVec3( 0.9f, 0.2f, 0.3f ) );
	planes[0][3] = -5.3f;
	planes[1][3] = -4.3f;

	for ( i = 0; i < COUNT; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			drawVerts[i].xyz[j] = srnd.CRandomFloat() * 10.0f;
			shadowVerts[i].xyz[j] = drawVerts[i].xyz[j];
		}
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->OverlayPointCull( cullBits1, texCoords1, planes, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->OverlayPointCull()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->OverlayPointCull( cullBits2, texCoords2, planes, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( cullBits1[i] != cullBits2[i] ) {
			break;
		}
		if ( !texCoords1[i].Compare( texCoords2[i], 1e-4f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->OverlayPointCull() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );



	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->OverlayPointCull( cullBits1, texCoords1, planes, shadowVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->OverlayPointCull()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->OverlayPointCull( cullBits2, texCoords2, planes, shadowVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( cullBits1[i] != cullBits2[i] ) {
			break;
		}
		if ( !texCoords1[i].Compare( texCoords2[i], 1e-4f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->OverlayPointCull() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestDeriveTriPlanes
============
*/
void TestDeriveTriPlanes( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( idDrawVert drawVerts[COUNT]; )
	ALIGN16( shadowCache_t shadowVerts[COUNT]; )
	ALIGN16( idPlane planes1[COUNT]; )
	ALIGN16( idPlane planes2[COUNT]; )
	ALIGN16( vertIndex_t indexes[COUNT*3]; )
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			drawVerts[i].xyz[j] = srnd.CRandomFloat() * 10.0f;
			shadowVerts[i].xyz[j] = drawVerts[i].xyz[j];
		}
		drawVerts[i].SetST( srnd.CRandomFloat(), srnd.CRandomFloat() );
	}

	for ( i = 0; i < COUNT; i++ ) {
		indexes[i*3+0] = ( i + 0 ) % COUNT;
		indexes[i*3+1] = ( i + 1 ) % COUNT;
		indexes[i*3+2] = ( i + 2 ) % COUNT;
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->DeriveTriPlanes( planes1, drawVerts, COUNT, indexes, COUNT*3 );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->DeriveTriPlanes()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->DeriveTriPlanes( planes2, drawVerts, COUNT, indexes, COUNT*3 );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( !planes1[i].Compare( planes2[i], 1e-1f, 5e-1f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->DeriveTriPlanes() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );



	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->DeriveTriPlanes( planes1, shadowVerts, COUNT, indexes, COUNT*3 );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->DeriveTriPlanes()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->DeriveTriPlanes( planes2, shadowVerts, COUNT, indexes, COUNT*3 );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( !planes1[i].Compare( planes2[i], 1e-1f, 5e-1f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->DeriveTriPlanes() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

}

/*
============
TestCalculateFacing
============
*/
void TestCalculateFacing( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( byte facing1[COUNT+1]; )
	ALIGN16( byte facing2[COUNT+1]; )
	ALIGN16( idPlane planes[COUNT]; )
	const char *result;

	idVec4 lightOrigin( 1, 2, 3, 1 );

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			planes[i][j] = srnd.CRandomFloat() * 10.0f;
		}
		planes[i].Normalize();
		planes[i][3] = srnd.CRandomFloat() * 10.0f;
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->CalculateFacing( facing1, planes, COUNT, lightOrigin );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CalculateFacing()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->CalculateFacing( facing2, planes, COUNT, lightOrigin );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT+1; i++ ) {
		if ( facing1[i] != facing2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->CalculateFacing() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestCalculateCullBits
============
*/
void TestCalculateCullBits( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( byte cullBits1[COUNT+1]; )
	ALIGN16( byte cullBits2[COUNT+1]; )
	ALIGN16( idDrawVert drawVerts[COUNT]; )
	ALIGN16( idPlane planes[COUNT]; )
	const char *result;

	idPlane lightPlanes[NUM_LIGHT_PLANES];
	lightPlanes[0][0] =  0.0f; lightPlanes[0][1] =  0.0f; lightPlanes[0][2] =  1.0f; lightPlanes[0][3] = -5.0f;
	lightPlanes[1][0] =  0.0f; lightPlanes[1][1] =  0.0f; lightPlanes[1][2] = -1.0f; lightPlanes[1][3] =  5.0f;
	lightPlanes[2][0] =  0.0f; lightPlanes[2][1] =  1.0f; lightPlanes[2][2] =  0.0f; lightPlanes[2][3] = -5.0f;
	lightPlanes[3][0] =  0.0f; lightPlanes[3][1] = -1.0f; lightPlanes[3][2] =  0.0f; lightPlanes[3][3] =  5.0f;
	lightPlanes[4][0] =  1.0f; lightPlanes[4][1] =  0.0f; lightPlanes[4][2] =  0.0f; lightPlanes[4][3] = -5.0f;
	lightPlanes[5][0] = -1.0f; lightPlanes[5][1] =  0.0f; lightPlanes[5][2] =  0.0f; lightPlanes[5][3] =  5.0f;

	idRandom srnd( RANDOM_SEED );
	idBounds bounds;

	bounds.Clear();
	for ( i = 0; i < COUNT; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			drawVerts[i].xyz[j] = srnd.CRandomFloat() * 10.0f;
		}
		drawVerts[i].SetST( srnd.CRandomFloat(), srnd.CRandomFloat() );
		bounds.AddPoint( drawVerts[i].xyz );
	}

	idVec3 center = bounds.GetCenter();
	idVec3 extents = bounds[1] - center;

	int frontBits = 0;

	// cull the triangle surface bounding box
	for ( i = 0; i < NUM_LIGHT_PLANES; i++ ) {
		const idPlane &plane = lightPlanes[i];
		float d1 = plane[0] * center.x +
						plane[1] * center.y +
							plane[2] * center.z +
								plane[3];
		float d2 = fabs( plane[0] * extents.x ) +
						fabs( plane[1] * extents.y ) +
							fabs( plane[2] * extents.z );

		if ( d1 - d2 >= 0.0f ) {
			frontBits |= 1 << i;    // front bits for the whole surface
		}
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->CalculateCullBits( cullBits1, drawVerts, COUNT, frontBits, lightPlanes );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CalculateCullBits()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->CalculateCullBits( cullBits2, drawVerts, COUNT, frontBits, lightPlanes );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( cullBits1[i] != cullBits2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->CalculateCullBits() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestCreateShadowCache
============
*/
void TestCreateShadowCache( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( idDrawVert drawVerts[COUNT]; )
	ALIGN16( shadowCache_t shadowVerts[COUNT]; )
	ALIGN16( idVec4 vertexCache1[COUNT*2]; )
	ALIGN16( idVec4 vertexCache2[COUNT*2]; )
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		drawVerts[i].xyz[0] = srnd.CRandomFloat() * 100.0f;
		drawVerts[i].xyz[1] = srnd.CRandomFloat() * 100.0f;
		drawVerts[i].xyz[2] = srnd.CRandomFloat() * 100.0f;
		shadowVerts[i].xyz[0] = drawVerts[i].xyz[0];
		shadowVerts[i].xyz[1] = drawVerts[i].xyz[1];
		shadowVerts[i].xyz[2] = drawVerts[i].xyz[2];
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->CreateShadowCache( vertexCache1, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CreateShadowCache()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->CreateShadowCache( vertexCache2, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( !vertexCache1[i*2+0].Compare( vertexCache2[i*2+0], 1e-2f ) ) {
			break;
		}
		if ( !vertexCache1[i*2+1].Compare( vertexCache2[i*2+1], 1e-2f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->CreateShadowCache() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );






	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->CreateShadowCache( vertexCache1, shadowVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CreateShadowCache()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->CreateShadowCache( vertexCache2, shadowVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( !vertexCache1[i*2+0].Compare( vertexCache2[i*2+0], 1e-2f ) ) {
			break;
		}
		if ( !vertexCache1[i*2+1].Compare( vertexCache2[i*2+1], 1e-2f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->CreateShadowCache() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );


}

#include "Simd_TestData.h"

/*
============
CreateShadowVolume_Generic
============
*/
int	CreateShadowVolume_Generic( vertIndex_t *shadowIndexes, const vertIndex_t *indexes, int numIndexes, const silEdge_t *silEdges, int numSilEdges, byte *facing, const byte *cullBits ) {
	int	numShadowingTriangles, numSilhouetteIndexes, numCapIndexes;
	int	numTriangles = numIndexes /	3;

	if ( cullBits == NULL )	{
		// count the number	of shadowing triangles
		numShadowingTriangles =	numTriangles - p_generic->ShadowVolume_CountFacing( facing, numTriangles );
	} else {
		// count the number	of shadowing triangles and make	all	triangles that are outside the light frustum "facing" so they won't	cast shadows
		numShadowingTriangles =	numTriangles - p_generic->ShadowVolume_CountFacingCull( facing, numTriangles, indexes, cullBits );
	}

	if ( !numShadowingTriangles	) {
		// no triangles	are	inside the light frustum and still facing the right	way
		return 0;
	}

	// create triangles	along silhouette planes
	numSilhouetteIndexes = p_generic->ShadowVolume_CreateSilTriangles( shadowIndexes, facing, silEdges, numSilEdges );

	// put some	triangles on the model and some	on the distant projection
	numCapIndexes =	p_generic->ShadowVolume_CreateCapTriangles( shadowIndexes + numSilhouetteIndexes, facing, indexes, numIndexes );

	return numSilhouetteIndexes	+ numCapIndexes;
}

/*
============
CreateShadowVolumeParallel_Generic
============
*/
int	CreateShadowVolumeParallel_Generic( vertIndex_t *shadowIndexes, const vertIndex_t *indexes, int numIndexes, const silEdge_t *silEdges, int numSilEdges, byte *facing, const byte *cullBits ) {
	int	numShadowingTriangles, numSilhouetteIndexes, numCapIndexes;
	int	numTriangles = numIndexes /	3;

	if ( cullBits == NULL )	{
		// count the number	of shadowing triangles
		numShadowingTriangles =	numTriangles - p_generic->ShadowVolume_CountFacing( facing, numTriangles );
	} else {
		// count the number	of shadowing triangles and make	all	triangles that are outside the light frustum "facing" so they won't	cast shadows
		numShadowingTriangles =	numTriangles - p_generic->ShadowVolume_CountFacingCull( facing, numTriangles, indexes, cullBits );
	}

	if ( !numShadowingTriangles	) {
		// no triangles	are	inside the light frustum and still facing the right	way
		return 0;
	}

	// create triangles	along silhouette planes
	numSilhouetteIndexes = p_generic->ShadowVolume_CreateSilTrianglesParallel( shadowIndexes, facing, silEdges, numSilEdges );

	// put some	triangles on the model and some	on the distant projection
	numCapIndexes =	p_generic->ShadowVolume_CreateCapTrianglesParallel( shadowIndexes + numSilhouetteIndexes, facing, indexes, numIndexes );

	return numSilhouetteIndexes	+ numCapIndexes;
}

/*
============
CreateShadowVolume_SIMD
============
*/
int	CreateShadowVolume_SIMD( vertIndex_t *shadowIndexes, const vertIndex_t *indexes, int numIndexes, const silEdge_t *silEdges, int numSilEdges, byte *facing, const byte *cullBits ) {
	int	numShadowingTriangles, numSilhouetteIndexes, numCapIndexes;
	int	numTriangles = numIndexes /	3;

	if ( cullBits == NULL )	{
		// count the number	of shadowing triangles
		numShadowingTriangles =	numTriangles - p_simd->ShadowVolume_CountFacing( facing, numTriangles );
	} else {
		// count the number	of shadowing triangles and make	all	triangles that are outside the light frustum "facing" so they won't	cast shadows
		numShadowingTriangles =	numTriangles - p_simd->ShadowVolume_CountFacingCull( facing, numTriangles, indexes, cullBits );
	}

	if ( !numShadowingTriangles	) {
		// no triangles	are	inside the light frustum and still facing the right	way
		return 0;
	}

	// create triangles	along silhouette planes
	numSilhouetteIndexes = p_simd->ShadowVolume_CreateSilTriangles( shadowIndexes, facing, silEdges, numSilEdges );

	// put some	triangles on the model and some	on the distant projection
	numCapIndexes =	p_simd->ShadowVolume_CreateCapTriangles( shadowIndexes + numSilhouetteIndexes, facing, indexes, numIndexes );

	return numSilhouetteIndexes	+ numCapIndexes;
}

/*
============
CreateShadowVolumeParallel_SIMD
============
*/
int	CreateShadowVolumeParallel_SIMD( vertIndex_t *shadowIndexes, const vertIndex_t *indexes, int numIndexes, const silEdge_t *silEdges, int numSilEdges, byte *facing, const byte *cullBits ) {
	int	numShadowingTriangles, numSilhouetteIndexes, numCapIndexes;
	int	numTriangles = numIndexes /	3;

	if ( cullBits == NULL )	{
		// count the number	of shadowing triangles
		numShadowingTriangles =	numTriangles - p_simd->ShadowVolume_CountFacing( facing, numTriangles );
	} else {
		// count the number	of shadowing triangles and make	all	triangles that are outside the light frustum "facing" so they won't	cast shadows
		numShadowingTriangles =	numTriangles - p_simd->ShadowVolume_CountFacingCull( facing, numTriangles, indexes, cullBits );
	}

	if ( !numShadowingTriangles	) {
		// no triangles	are	inside the light frustum and still facing the right	way
		return 0;
	}

	// create triangles	along silhouette planes
	numSilhouetteIndexes = p_simd->ShadowVolume_CreateSilTrianglesParallel( shadowIndexes, facing, silEdges, numSilEdges );

	// put some	triangles on the model and some	on the distant projection
	numCapIndexes =	p_simd->ShadowVolume_CreateCapTrianglesParallel( shadowIndexes + numSilhouetteIndexes, facing, indexes, numIndexes );

	return numSilhouetteIndexes	+ numCapIndexes;
}

/*
============
TestShadowVolumes
============
*/
void TestShadowVolumes( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( vertIndex_t shadowIndexes1[16384]; )
	ALIGN16( vertIndex_t shadowIndexes2[16384]; )
	ALIGN16( byte facing1[8129]; )
	ALIGN16( byte facing2[8129]; )
	int numFacing1 = 0;
	int numFacing2 = 0;
	const char *result;
	idRandom random;

	FlushCache();

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		numFacing1 = p_generic->ShadowVolume_CountFacing( SIMD_testFacing, SIMD_numTestFacing );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->ShadowVolume_CountFacing()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		numFacing2 = p_simd->ShadowVolume_CountFacing( SIMD_testFacing, SIMD_numTestFacing );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	result = ( numFacing1 == numFacing2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->ShadowVolume_CountFacing() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < SIMD_numTestFacing; j++ ) {
			facing1[j] = SIMD_testFacing[j];
		}
		StartRecordTime( start );
		numFacing1 = p_generic->ShadowVolume_CountFacingCull( facing1, SIMD_numTestFacing, SIMD_testIndexes, SIMD_testCullBits );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->ShadowVolume_CountFacingCull()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < SIMD_numTestFacing; j++ ) {
			facing2[j] = SIMD_testFacing[j];
		}
		StartRecordTime( start );
		numFacing2 = p_simd->ShadowVolume_CountFacingCull( facing2, SIMD_numTestFacing, SIMD_testIndexes, SIMD_testCullBits );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < numFacing1; i++ ) {
		if ( facing1[i] != facing2[i] ) {
			break;
		}
	}

	result = ( i >= numFacing1 && numFacing1 == numFacing2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->ShadowVolume_CountFacingCull() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	for ( j = 0; j < SIMD_numTestFacing; j++ ) {
		facing1[j] = SIMD_testFacing[j];
		facing2[j] = SIMD_testFacing[j];
	}
	// make 20% of the potential silhouette edges part of the shadow silhouette
	for ( i = 0; i < SIMD_numTestSilEdges; i++ ) {
		numFacing1 = p_generic->ShadowVolume_CreateSilTriangles( shadowIndexes1, facing1, SIMD_testSilEdges, SIMD_numTestSilEdges );
		if ( numFacing1 / 6 * 100 / SIMD_numTestSilEdges >= 20 ) {
			break;
		}
		int index = random.RandomInt( SIMD_numTestFacing );
		facing1[index] ^= 1;
	}
	// make 20% of the potential silhouette edges part of the shadow silhouette
	for ( i = 0; i < SIMD_numTestSilEdges; i++ ) {
		numFacing1 = p_generic->ShadowVolume_CreateSilTriangles( shadowIndexes1, facing2, SIMD_testSilEdges, SIMD_numTestSilEdges );
		if ( numFacing1 / 6 * 100 / SIMD_numTestSilEdges >= 20 ) {
			break;
		}
		int index = random.RandomInt( SIMD_numTestFacing );
		facing2[index] ^= 1;
	}

	bestClocksGeneric = 0;
	random.SetSeed( 0 );
	for ( i = 0; i < NUMTESTS; i++ ) {
		byte *facingPtr = ( i & 1 ) ? facing1 : facing2;
		StartRecordTime( start );
		numFacing1 = p_generic->ShadowVolume_CreateSilTriangles( shadowIndexes1, facingPtr, SIMD_testSilEdges, SIMD_numTestSilEdges );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->ShadowVolume_CreateSilTriangles()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	random.SetSeed( 0 );
	for ( i = 0; i < NUMTESTS; i++ ) {
		byte *facingPtr = ( i & 1 ) ? facing1 : facing2;
		StartRecordTime( start );
		numFacing2 = p_simd->ShadowVolume_CreateSilTriangles( shadowIndexes2, facingPtr, SIMD_testSilEdges, SIMD_numTestSilEdges );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < numFacing1; i++ ) {
		if ( shadowIndexes1[i] != shadowIndexes2[i] ) {
			break;
		}
	}

	result = ( i >= numFacing1 && numFacing1 == numFacing2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->ShadowVolume_CreateSilTriangles() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	random.SetSeed( 0 );
	for ( i = 0; i < NUMTESTS; i++ ) {
		byte *facingPtr = ( i & 1 ) ? facing1 : facing2;
		StartRecordTime( start );
		numFacing1 = p_generic->ShadowVolume_CreateSilTrianglesParallel( shadowIndexes1, facingPtr, SIMD_testSilEdges, SIMD_numTestSilEdges );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->ShadowVolume_CreateSilTrianglesP()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	random.SetSeed( 0 );
	for ( i = 0; i < NUMTESTS; i++ ) {
		byte *facingPtr = ( i & 1 ) ? facing1 : facing2;
		StartRecordTime( start );
		numFacing2 = p_simd->ShadowVolume_CreateSilTrianglesParallel( shadowIndexes2, facingPtr, SIMD_testSilEdges, SIMD_numTestSilEdges );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < numFacing1; i++ ) {
		if ( shadowIndexes1[i] != shadowIndexes2[i] ) {
			break;
		}
	}

	result = ( i >= numFacing1 && numFacing1 == numFacing2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->ShadowVolume_CreateSilTrianglesP() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	random.SetSeed( 0 );
	for ( i = 0; i < NUMTESTS; i++ ) {
		byte *facingPtr = ( i & 1 ) ? facing1 : facing2;
		StartRecordTime( start );
		numFacing1 = p_generic->ShadowVolume_CreateCapTriangles( shadowIndexes1, facingPtr, SIMD_testIndexes, SIMD_numTestIndexes );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->ShadowVolume_CreateCapTriangles()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	random.SetSeed( 0 );
	for ( i = 0; i < NUMTESTS; i++ ) {
		byte *facingPtr = ( i & 1 ) ? facing1 : facing2;
		StartRecordTime( start );
		numFacing2 = p_simd->ShadowVolume_CreateCapTriangles( shadowIndexes2, facingPtr, SIMD_testIndexes, SIMD_numTestIndexes );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < numFacing1; i++ ) {
		if ( shadowIndexes1[i] != shadowIndexes2[i] ) {
			break;
		}
	}

	result = ( i >= numFacing1 && numFacing1 == numFacing2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->ShadowVolume_CreateCapTriangles() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	random.SetSeed( 0 );
	for ( i = 0; i < NUMTESTS; i++ ) {
		byte *facingPtr = ( i & 1 ) ? facing1 : facing2;
		StartRecordTime( start );
		numFacing1 = p_generic->ShadowVolume_CreateCapTrianglesParallel( shadowIndexes1, facingPtr, SIMD_testIndexes, SIMD_numTestIndexes );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->ShadowVolume_CreateCapTrianglesP()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	random.SetSeed( 0 );
	for ( i = 0; i < NUMTESTS; i++ ) {
		byte *facingPtr = ( i & 1 ) ? facing1 : facing2;
		StartRecordTime( start );
		numFacing2 = p_simd->ShadowVolume_CreateCapTrianglesParallel( shadowIndexes2, facingPtr, SIMD_testIndexes, SIMD_numTestIndexes );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < numFacing1; i++ ) {
		if ( shadowIndexes1[i] != shadowIndexes2[i] ) {
			break;
		}
	}

	result = ( i >= numFacing1 && numFacing1 == numFacing2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->ShadowVolume_CreateCapTrianglesP() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	random.SetSeed( 0 );
	for ( i = 0; i < NUMTESTS; i++ ) {
		byte *facingPtr = ( i & 1 ) ? facing1 : facing2;
		StartRecordTime( start );
		numFacing1 = CreateShadowVolume_Generic( shadowIndexes1, SIMD_testIndexes, SIMD_numTestIndexes, SIMD_testSilEdges, SIMD_numTestSilEdges, facingPtr, NULL );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CreateShadowVolume()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	random.SetSeed( 0 );
	for ( i = 0; i < NUMTESTS; i++ ) {
		byte *facingPtr = ( i & 1 ) ? facing1 : facing2;
		StartRecordTime( start );
		numFacing2 = CreateShadowVolume_SIMD( shadowIndexes2, SIMD_testIndexes, SIMD_numTestIndexes, SIMD_testSilEdges, SIMD_numTestSilEdges, facingPtr, NULL );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < numFacing1; i++ ) {
		if ( shadowIndexes1[i] != shadowIndexes2[i] ) {
			break;
		}
	}

	result = ( i >= numFacing1 && numFacing1 == numFacing2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->CreateShadowVolume() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	random.SetSeed( 0 );
	for ( i = 0; i < NUMTESTS; i++ ) {
		byte *facingPtr = ( i & 1 ) ? facing1 : facing2;
		StartRecordTime( start );
		numFacing1 = CreateShadowVolumeParallel_Generic( shadowIndexes1, SIMD_testIndexes, SIMD_numTestIndexes, SIMD_testSilEdges, SIMD_numTestSilEdges, facingPtr, NULL );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CreateShadowVolumeP()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	random.SetSeed( 0 );
	for ( i = 0; i < NUMTESTS; i++ ) {
		byte *facingPtr = ( i & 1 ) ? facing1 : facing2;
		StartRecordTime( start );
		numFacing2 = CreateShadowVolumeParallel_SIMD( shadowIndexes2, SIMD_testIndexes, SIMD_numTestIndexes, SIMD_testSilEdges, SIMD_numTestSilEdges, facingPtr, NULL );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < numFacing1; i++ ) {
		if ( shadowIndexes1[i] != shadowIndexes2[i] ) {
			break;
		}
	}

	result = ( i >= numFacing1 && numFacing1 == numFacing2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->CreateShadowVolumeP() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	random.SetSeed( 0 );
	for ( i = 0; i < NUMTESTS; i++ ) {
		byte *facingPtr = ( i & 1 ) ? facing1 : facing2;
		StartRecordTime( start );
		numFacing1 = CreateShadowVolume_Generic( shadowIndexes1, SIMD_testIndexes, SIMD_numTestIndexes, SIMD_testSilEdges, SIMD_numTestSilEdges, facingPtr, SIMD_testCullBits );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CreateShadowVolume() + cull", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	random.SetSeed( 0 );
	for ( i = 0; i < NUMTESTS; i++ ) {
		byte *facingPtr = ( i & 1 ) ? facing1 : facing2;
		StartRecordTime( start );
		numFacing2 = CreateShadowVolume_SIMD( shadowIndexes2, SIMD_testIndexes, SIMD_numTestIndexes, SIMD_testSilEdges, SIMD_numTestSilEdges, facingPtr, SIMD_testCullBits );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < numFacing1; i++ ) {
		if ( shadowIndexes1[i] != shadowIndexes2[i] ) {
			break;
		}
	}

	result = ( i >= numFacing1 && numFacing1 == numFacing2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->CreateShadowVolume() + cull %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	random.SetSeed( 0 );
	for ( i = 0; i < NUMTESTS; i++ ) {
		byte *facingPtr = ( i & 1 ) ? facing1 : facing2;
		StartRecordTime( start );
		numFacing1 = CreateShadowVolumeParallel_Generic( shadowIndexes1, SIMD_testIndexes, SIMD_numTestIndexes, SIMD_testSilEdges, SIMD_numTestSilEdges, facingPtr, SIMD_testCullBits );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CreateShadowVolumeP() + cull", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	random.SetSeed( 0 );
	for ( i = 0; i < NUMTESTS; i++ ) {
		byte *facingPtr = ( i & 1 ) ? facing1 : facing2;
		StartRecordTime( start );
		numFacing2 = CreateShadowVolumeParallel_SIMD( shadowIndexes2, SIMD_testIndexes, SIMD_numTestIndexes, SIMD_testSilEdges, SIMD_numTestSilEdges, facingPtr, SIMD_testCullBits );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < numFacing1; i++ ) {
		if ( shadowIndexes1[i] != shadowIndexes2[i] ) {
			break;
		}
	}

	result = ( i >= numFacing1 && numFacing1 == numFacing2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->CreateShadowVolumeP() + cull %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestSoundUpSampling
============
*/
#define SOUND_UPSAMPLE_EPSILON		1.0f

void TestSoundUpSampling( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( short pcm[MIXBUFFER_SAMPLES*2]; )
	ALIGN16( float ogg0[MIXBUFFER_SAMPLES*2]; )
	ALIGN16( float ogg1[MIXBUFFER_SAMPLES*2]; )
	ALIGN16( float samples1[MIXBUFFER_SAMPLES*2]; )
	ALIGN16( float samples2[MIXBUFFER_SAMPLES*2]; )
	float *ogg[2];
	int kHz, numSpeakers;
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < MIXBUFFER_SAMPLES*2; i++ ) {
		pcm[i] = srnd.RandomInt( (1<<16) ) - (1<<15);
		ogg0[i] = srnd.RandomFloat();
		ogg1[i] = srnd.RandomFloat();
	}

	ogg[0] = ogg0;
	ogg[1] = ogg1;

	for ( numSpeakers = 1; numSpeakers <= 2; numSpeakers++ ) {

		for ( kHz = 11025; kHz <= 44100; kHz *= 2 ) {
			bestClocksGeneric = 0;
			for ( i = 0; i < NUMTESTS; i++ ) {
				StartRecordTime( start );
				p_generic->UpSamplePCMTo44kHz( samples1, pcm, MIXBUFFER_SAMPLES*numSpeakers*kHz/44100, kHz, numSpeakers );
				StopRecordTime( end );
				GetBest( start, end, bestClocksGeneric );
			}
			PrintClocks( va( "generic->UpSamplePCMTo44kHz( %d, %d )", kHz, numSpeakers ), MIXBUFFER_SAMPLES*numSpeakers*kHz/44100, bestClocksGeneric );

			bestClocksSIMD = 0;
			for ( i = 0; i < NUMTESTS; i++ ) {
				StartRecordTime( start );
				p_simd->UpSamplePCMTo44kHz( samples2, pcm, MIXBUFFER_SAMPLES*numSpeakers*kHz/44100, kHz, numSpeakers );
				StopRecordTime( end );
				GetBest( start, end, bestClocksSIMD );
			}

			for ( i = 0; i < MIXBUFFER_SAMPLES*numSpeakers; i++ ) {
				if ( idMath::Fabs( samples1[i] - samples2[i] ) > SOUND_UPSAMPLE_EPSILON ) {
					break;
				}
			}
			result = ( i >= MIXBUFFER_SAMPLES*numSpeakers ) ? "ok" : S_COLOR_RED"X";
			PrintClocks( va( "   simd->UpSamplePCMTo44kHz( %d, %d ) %s", kHz, numSpeakers, result ), MIXBUFFER_SAMPLES*numSpeakers*kHz/44100, bestClocksSIMD, bestClocksGeneric );
		}
	}

	for ( numSpeakers = 1; numSpeakers <= 2; numSpeakers++ ) {

		for ( kHz = 11025; kHz <= 44100; kHz *= 2 ) {
			bestClocksGeneric = 0;
			for ( i = 0; i < NUMTESTS; i++ ) {
				StartRecordTime( start );
				p_generic->UpSampleOGGTo44kHz( samples1, ogg, MIXBUFFER_SAMPLES*numSpeakers*kHz/44100, kHz, numSpeakers );
				StopRecordTime( end );
				GetBest( start, end, bestClocksGeneric );
			}
			PrintClocks( va( "generic->UpSampleOGGTo44kHz( %d, %d )", kHz, numSpeakers ), MIXBUFFER_SAMPLES*numSpeakers*kHz/44100, bestClocksGeneric );

			bestClocksSIMD = 0;
			for ( i = 0; i < NUMTESTS; i++ ) {
				StartRecordTime( start );
				p_simd->UpSampleOGGTo44kHz( samples2, ogg, MIXBUFFER_SAMPLES*numSpeakers*kHz/44100, kHz, numSpeakers );
				StopRecordTime( end );
				GetBest( start, end, bestClocksSIMD );
			}

			for ( i = 0; i < MIXBUFFER_SAMPLES*numSpeakers; i++ ) {
				if ( idMath::Fabs( samples1[i] - samples2[i] ) > SOUND_UPSAMPLE_EPSILON ) {
					break;
				}
			}
			result = ( i >= MIXBUFFER_SAMPLES ) ? "ok" : S_COLOR_RED"X";
			PrintClocks( va( "   simd->UpSampleOGGTo44kHz( %d, %d ) %s", kHz, numSpeakers, result ), MIXBUFFER_SAMPLES*numSpeakers*kHz/44100, bestClocksSIMD, bestClocksGeneric );
		}
	}
}

/*
============
TestSoundMixing
============
*/
#define SOUND_MIX_EPSILON		2.0f

void TestSoundMixing( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float origMixBuffer[MIXBUFFER_SAMPLES*8]; )
	ALIGN16( float mixBuffer1[MIXBUFFER_SAMPLES*8]; )
	ALIGN16( float mixBuffer2[MIXBUFFER_SAMPLES*8]; )
	ALIGN16( float samples[MIXBUFFER_SAMPLES*8]; )
	ALIGN16( short outSamples1[MIXBUFFER_SAMPLES*8]; )
	ALIGN16( short outSamples2[MIXBUFFER_SAMPLES*8]; )
	float lastV[8];
	float currentV[8];
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < 8; i++ ) {
		lastV[i] = srnd.CRandomFloat();
		currentV[i] = srnd.CRandomFloat();
	}

	for ( i = 0; i < MIXBUFFER_SAMPLES*8; i++ ) {
		origMixBuffer[i] = srnd.CRandomFloat();
		samples[i] = srnd.RandomInt( (1<<16) ) - (1<<15);
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*8; j++ ) {
			mixBuffer1[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_generic->MixSoundTwoSpeakerMono( mixBuffer1, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MixSoundTwoSpeakerMono()", MIXBUFFER_SAMPLES, bestClocksGeneric );


	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*8; j++ ) {
			mixBuffer2[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_simd->MixSoundTwoSpeakerMono( mixBuffer2, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < MIXBUFFER_SAMPLES*8; i++ ) {
		if ( idMath::Fabs( mixBuffer1[i] - mixBuffer2[i] ) > SOUND_MIX_EPSILON ) {
			break;
		}
	}
	result = ( i >= MIXBUFFER_SAMPLES*8 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MixSoundTwoSpeakerMono() %s", result ), MIXBUFFER_SAMPLES, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*8; j++ ) {
			mixBuffer1[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_generic->MixSoundTwoSpeakerStereo( mixBuffer1, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MixSoundTwoSpeakerStereo()", MIXBUFFER_SAMPLES, bestClocksGeneric );


	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*8; j++ ) {
			mixBuffer2[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_simd->MixSoundTwoSpeakerStereo( mixBuffer2, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < MIXBUFFER_SAMPLES*8; i++ ) {
		if ( idMath::Fabs( mixBuffer1[i] - mixBuffer2[i] ) > SOUND_MIX_EPSILON ) {
			break;
		}
	}
	result = ( i >= MIXBUFFER_SAMPLES*8 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MixSoundTwoSpeakerStereo() %s", result ), MIXBUFFER_SAMPLES, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*8; j++ ) {
			mixBuffer1[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_generic->MixSoundFourSpeakerMono( mixBuffer1, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MixSoundFourSpeakerMono()", MIXBUFFER_SAMPLES, bestClocksGeneric );


	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*8; j++ ) {
			mixBuffer2[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_simd->MixSoundFourSpeakerMono( mixBuffer2, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < MIXBUFFER_SAMPLES*8; i++ ) {
		if ( idMath::Fabs( mixBuffer1[i] - mixBuffer2[i] ) > SOUND_MIX_EPSILON ) {
			break;
		}
	}
	result = ( i >= MIXBUFFER_SAMPLES*8 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MixSoundFourSpeakerMono() %s", result ), MIXBUFFER_SAMPLES, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*8; j++ ) {
			mixBuffer1[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_generic->MixSoundFourSpeakerStereo( mixBuffer1, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MixSoundFourSpeakerStereo()", MIXBUFFER_SAMPLES, bestClocksGeneric );


	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*8; j++ ) {
			mixBuffer2[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_simd->MixSoundFourSpeakerStereo( mixBuffer2, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < MIXBUFFER_SAMPLES*8; i++ ) {
		if ( idMath::Fabs( mixBuffer1[i] - mixBuffer2[i] ) > SOUND_MIX_EPSILON ) {
			break;
		}
	}
	result = ( i >= MIXBUFFER_SAMPLES*8 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MixSoundFourSpeakerStereo() %s", result ), MIXBUFFER_SAMPLES, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*8; j++ ) {
			mixBuffer1[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_generic->MixSoundSixSpeakerMono( mixBuffer1, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MixSoundSixSpeakerMono()", MIXBUFFER_SAMPLES, bestClocksGeneric );


	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*8; j++ ) {
			mixBuffer2[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_simd->MixSoundSixSpeakerMono( mixBuffer2, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < MIXBUFFER_SAMPLES*8; i++ ) {
		if ( idMath::Fabs( mixBuffer1[i] - mixBuffer2[i] ) > SOUND_MIX_EPSILON ) {
			break;
		}
	}
	result = ( i >= MIXBUFFER_SAMPLES*8 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MixSoundSixSpeakerMono() %s", result ), MIXBUFFER_SAMPLES, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*8; j++ ) {
			mixBuffer1[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_generic->MixSoundSixSpeakerStereo( mixBuffer1, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MixSoundSixSpeakerStereo()", MIXBUFFER_SAMPLES, bestClocksGeneric );


	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*8; j++ ) {
			mixBuffer2[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_simd->MixSoundSixSpeakerStereo( mixBuffer2, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < MIXBUFFER_SAMPLES*8; i++ ) {
		if ( idMath::Fabs( mixBuffer1[i] - mixBuffer2[i] ) > SOUND_MIX_EPSILON ) {
			break;
		}
	}
	result = ( i >= MIXBUFFER_SAMPLES*8 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MixSoundSixSpeakerStereo() %s", result ), MIXBUFFER_SAMPLES, bestClocksSIMD, bestClocksGeneric );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*8; j++ ) {
			mixBuffer1[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_generic->MixSoundEightSpeakerMono( mixBuffer1, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MixSoundEightSpeakerMono()", MIXBUFFER_SAMPLES, bestClocksGeneric );


	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*8; j++ ) {
			mixBuffer2[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_simd->MixSoundEightSpeakerMono( mixBuffer2, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < MIXBUFFER_SAMPLES*8; i++ ) {
		if ( idMath::Fabs( mixBuffer1[i] - mixBuffer2[i] ) > SOUND_MIX_EPSILON ) {
			break;
		}
	}
	result = ( i >= MIXBUFFER_SAMPLES*8 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MixSoundEightSpeakerMono() %s", result ), MIXBUFFER_SAMPLES, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*8; j++ ) {
			mixBuffer1[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_generic->MixSoundEightSpeakerStereo( mixBuffer1, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MixSoundEightSpeakerStereo()", MIXBUFFER_SAMPLES, bestClocksGeneric );


	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*8; j++ ) {
			mixBuffer2[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_simd->MixSoundEightSpeakerStereo( mixBuffer2, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < MIXBUFFER_SAMPLES*8; i++ ) {
		if ( idMath::Fabs( mixBuffer1[i] - mixBuffer2[i] ) > SOUND_MIX_EPSILON ) {
			break;
		}
	}
	result = ( i >= MIXBUFFER_SAMPLES*8 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MixSoundEightSpeakerStereo() %s", result ), MIXBUFFER_SAMPLES, bestClocksSIMD, bestClocksGeneric );


	for ( i = 0; i < MIXBUFFER_SAMPLES*8; i++ ) {
		origMixBuffer[i] = srnd.RandomInt( (1<<17) ) - (1<<16);
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*8; j++ ) {
			mixBuffer1[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_generic->MixedSoundToSamples( outSamples1, mixBuffer1, MIXBUFFER_SAMPLES*8 );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MixedSoundToSamples()", MIXBUFFER_SAMPLES, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*8; j++ ) {
			mixBuffer2[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_simd->MixedSoundToSamples( outSamples2, mixBuffer2, MIXBUFFER_SAMPLES*8 );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < MIXBUFFER_SAMPLES*8; i++ ) {
		if ( outSamples1[i] != outSamples2[i] ) {
			break;
		}
	}
	result = ( i >= MIXBUFFER_SAMPLES*8 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MixedSoundToSamples() %s", result ), MIXBUFFER_SAMPLES, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestMath
============
*/
void TestMath( void ) {
	int i;
	TIME_TYPE start, end, bestClocks;

	idLib::common->Printf("====================================\n" );

	float tst = -1.0f;
	float tst2 = 1.0f;
	float testvar = 1.0f;
	idRandom rnd;

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = fabs( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "            fabs( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		int tmp = * ( int * ) &tst;
		tmp &= 0x7FFFFFFF;
		tst = * ( float * ) &tmp;
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    idMath::Fabs( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = 10.0f + 100.0f * rnd.RandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = sqrt( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * 0.01f;
		tst = 10.0f + 100.0f * rnd.RandomFloat();
	}
	PrintClocks( "            sqrt( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.RandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Sqrt( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.RandomFloat();
	}
	PrintClocks( "    idMath::Sqrt( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.RandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Sqrt16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.RandomFloat();
	}
	PrintClocks( "  idMath::Sqrt16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.RandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Sqrt64( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.RandomFloat();
	}
	PrintClocks( "  idMath::Sqrt64( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.RandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = tst * idMath::RSqrt( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.RandomFloat();
	}
	PrintClocks( "   idMath::RSqrt( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Sin( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "     idMath::Sin( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Sin16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "   idMath::Sin16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Cos( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "     idMath::Cos( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Cos16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "   idMath::Cos16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		idMath::SinCos( tst, tst, tst2 );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  idMath::SinCos( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		idMath::SinCos16( tst, tst, tst2 );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "idMath::SinCos16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Tan( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "     idMath::Tan( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Tan16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "   idMath::Tan16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::ASin( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * ( 1.0f / idMath::PI );
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    idMath::ASin( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::ASin16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * ( 1.0f / idMath::PI );
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  idMath::ASin16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::ACos( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * ( 1.0f / idMath::PI );
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    idMath::ACos( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::ACos16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * ( 1.0f / idMath::PI );
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  idMath::ACos16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::ATan( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    idMath::ATan( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::ATan16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  idMath::ATan16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Pow( 2.7f, tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * 0.1f;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    idMath::Pow( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Pow16( 2.7f, tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * 0.1f;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  idMath::Pow16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Exp( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * 0.1f;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    idMath::Exp( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Exp16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * 0.1f;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  idMath::Exp16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		tst = fabs( tst ) + 1.0f;
		StartRecordTime( start );
		tst = idMath::Log( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    idMath::Log( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		tst = fabs( tst ) + 1.0f;
		StartRecordTime( start );
		tst = idMath::Log16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  idMath::Log16( tst )", 1, bestClocks );

	idLib::common->Printf( "testvar = %f\n", testvar );

	idMat3 resultMat3;
	idQuat fromQuat, toQuat, resultQuat;
	idCQuat cq;
	idAngles ang;

	fromQuat = idAngles( 30, 45, 0 ).ToQuat();
	toQuat = idAngles( 45, 0, 0 ).ToQuat();
	cq = idAngles( 30, 45, 0 ).ToQuat().ToCQuat();
	ang = idAngles( 30, 40, 50 );

	bestClocks = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		resultMat3 = fromQuat.ToMat3();
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	PrintClocks( "       idQuat::ToMat3()", 1, bestClocks );

	bestClocks = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		resultQuat.Slerp( fromQuat, toQuat, 0.3f );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	PrintClocks( "        idQuat::Slerp()", 1, bestClocks );

	bestClocks = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		resultQuat = cq.ToQuat();
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	PrintClocks( "      idCQuat::ToQuat()", 1, bestClocks );

	bestClocks = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		resultQuat = ang.ToQuat();
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	PrintClocks( "     idAngles::ToQuat()", 1, bestClocks );

	bestClocks = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		resultMat3 = ang.ToMat3();
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	PrintClocks( "     idAngles::ToMat3()", 1, bestClocks );
}

/*
============
TestNegate
============
*/

// this wasn't previously in the test
void TestNegate( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fsrc0[COUNT]; )
	ALIGN16( float fsrc1[COUNT]; )
	ALIGN16( float fsrc2[COUNT]; )
	
	const char *result;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = fsrc1[i] = fsrc2[i] = srnd.CRandomFloat() * 10.0f;
		//fsrc1[i] = srnd.CRandomFloat() * 10.0f;
	}

	idLib::common->Printf("====================================\n" );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
	
		memcpy( &fsrc1[0], &fsrc0[0], COUNT * sizeof(float) );
	
		StartRecordTime( start );
		p_generic->Negate16( fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Negate16( float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
	
		memcpy( &fsrc2[0], &fsrc0[0], COUNT * sizeof(float) );
	
		StartRecordTime( start );
		p_simd->Negate16( fsrc2, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( fsrc1[i] != fsrc2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Negate16( float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}


/*
============
idSIMD::Test_f
============
*/
void idSIMD::Test_f( const idCmdArgs &args ) {

	p_simd = processor;
	p_generic = generic;

	if ( idStr::Length( args.Argv( 1 ) ) != 0 ) {
		cpuid_t cpuid = idLib::sys->GetProcessorId();
		idStr argString = args.Args();

		argString.Replace( " ", "" );

		if ( idStr::Icmp( argString, "MMX" ) == 0 ) {
			if ( !( cpuid & CPUID_MMX ) ) {
				idLib::common->Printf( "CPU does not support MMX\n" );
				return;
			}
			p_simd = new idSIMD_MMX;
		} else if ( idStr::Icmp( argString, "3DNow" ) == 0 ) {
			if ( !( cpuid & CPUID_MMX ) || !( cpuid & CPUID_3DNOW ) ) {
				idLib::common->Printf( "CPU does not support MMX & 3DNow\n" );
				return;
			}
			p_simd = new idSIMD_3DNow;
		} else if ( idStr::Icmp( argString, "SSE" ) == 0 ) {
			if ( !( cpuid & CPUID_MMX ) || !( cpuid & CPUID_SSE ) ) {
				idLib::common->Printf( "CPU does not support MMX & SSE\n" );
				return;
			}
			p_simd = new idSIMD_SSE;
		} else if ( idStr::Icmp( argString, "SSE2" ) == 0 ) {
			if ( !( cpuid & CPUID_MMX ) || !( cpuid & CPUID_SSE ) || !( cpuid & CPUID_SSE2 ) ) {
				idLib::common->Printf( "CPU does not support MMX & SSE & SSE2\n" );
				return;
			}
			p_simd = new idSIMD_SSE2;
		} else if ( idStr::Icmp( argString, "SSE3" ) == 0 ) {
			if ( !( cpuid & CPUID_MMX ) || !( cpuid & CPUID_SSE ) || !( cpuid & CPUID_SSE2 ) || !( cpuid & CPUID_SSE3 ) ) {
				idLib::common->Printf( "CPU does not support MMX & SSE & SSE2 & SSE3\n" );
				return;
			}
			p_simd = new idSIMD_SSE3();
		} else if ( idStr::Icmp( argString, "AltiVec" ) == 0 ) {
			if ( !( cpuid & CPUID_ALTIVEC ) ) {
				idLib::common->Printf( "CPU does not support AltiVec\n" );
				return;
			}
			p_simd = new idSIMD_AltiVec();
		} else if ( idStr::Icmp( argString, "VMX128" ) == 0 ) {
			if ( !( cpuid & CPUID_XENON ) ) {
				idLib::common->Printf( "CPU does not support VMX128\n" );
				return;
			}
			p_simd = new idSIMD_Xenon();
		} else {
			idLib::common->Printf( "invalid argument, use: MMX, 3DNow, SSE, SSE2, SSE3, AltiVec\n" );
			return;
		}
	}

#ifdef ID_WIN_X86_ASM
	::SetThreadPriority( ::GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL );
#endif

	idLib::common->SetRefreshOnPrint( true );

	idLib::common->Printf( "using %s for SIMD processing\n", p_simd->GetName() );

	PrintResolution();

	GetBaseClocks();

#if XENON_FRAMES
	for ( int i = 0; i < 1000; i++ ) {
		// Clear the backbuffer
		g_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, 
									0xff000000, 1.0f, 0L );
#endif

/*
	TestMath();
	TestAdd();
	TestSub();
	TestMul();
	TestDiv();
	TestMulAdd();
	TestMulSub();
	TestDot();*/
	TestCompare();
/*	TestMinMax();
	TestClamp();
	TestMemcpy();
	TestMemset();
	TestNegate();

	TestMatXMultiplyVecX();
	TestMatXMultiplyAddVecX();
	TestMatXTransposeMultiplyVecX();
	TestMatXTransposeMultiplyAddVecX();
	TestMatXMultiplyMatX();
	TestMatXTransposeMultiplyMatX();
	TestMatXLowerTriangularSolve();
	TestMatXLowerTriangularSolveTranspose();
	TestMatXUpperTriangularSolve();
	TestMatXUpperTriangularSolveTranspose();
	TestMatXLUFactor();
	TestMatXLDLTFactor();

	idLib::common->Printf("====================================\n" );
*/
//	TestBlendJoints();
//	TestConvertJointQuatsToJointMats();
//	TestConvertJointMatsToJointQuats();
//	TestTransformJoints();
//	TestUntransformJoints();
//	TestMultiplyJoints();
//	TestTransformVerts();
//	TestTracePointCull();
//	TestDecalPointCull();
//	TestOverlayPointCull();
//	TestDeriveTriPlanes();
//	TestCalculateFacing();
//	TestCalculateCullBits();
//	TestCreateShadowCache();
//	TestShadowVolumes();

//	idLib::common->Printf("====================================\n" );

//	TestSoundUpSampling();
	TestSoundMixing();

	idLib::common->Printf("====================================\n" );

#if XENON_FRAMES
		// Present the backbuffer contents to the display
		g_pd3dDevice->Present( NULL, NULL, NULL, NULL );
	}
#endif

	idLib::common->SetRefreshOnPrint( false );

#ifdef ID_WIN_X86_ASM
	SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_NORMAL );
#endif

	if ( p_simd != processor ) {
		delete p_simd;
	}
	p_simd = NULL;
	p_generic = NULL;
}
