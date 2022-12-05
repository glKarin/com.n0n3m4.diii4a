#include "../precompiled.h"
#pragma hdrstop

#include "Simd_generic.h"
#include "Simd_MMX.h"


//===============================================================
//
//	MMX implementation of idSIMDProcessor
//
//===============================================================

/*
gcc inline assembly:
inline assembly for the MMX SIMD processor written there mostly as an experiment
does not increase performance on timedemos ( nor did I expect it to, libc-i686 does the job very well already )

although the newer gcc can read inline asm using the intel syntax ( with minor reformatting and escaping of register names ),
it's still a long way from providing an easy compatibility with MSVC inline assembly
mostly because of the input/output registers, the clobber lists
and generally all the things gcc tries to be clever about when you give it a piece of inline assembly
( typically, compiling this at -O1 or better will produce bad code, and some of it won't compile with -fPIC either )

at this point, writing everything in nasm from the ground up, or using intel's compiler to produce the Simd_*.o objects is
still the best alternative
*/

#if defined( _WINDOWS ) || defined( __linux__ )

#ifdef _WINDOWS
#define EMMS_INSTRUCTION		__asm emms
#else
#define EMMS_INSTRUCTION		__asm__ __volatile__ ( "emms\n\t" );
#endif

/*
============
idSIMD_MMX::GetName
============
*/
const char * idSIMD_MMX::GetName( void ) const {
	return "MMX";
}

/*
================
MMX_Memcpy8B
================
*/
void MMX_Memcpy8B( void *dest, const void *src, const int count ) {
#ifdef _MSC_VER
	_asm { 
        mov		esi, src 
        mov		edi, dest 
        mov		ecx, count 
        shr		ecx, 3			// 8 bytes per iteration 

loop1: 
        movq	mm1,  0[ESI]	// Read in source data 
        movntq	0[EDI], mm1		// Non-temporal stores 

        add		esi, 8
        add		edi, 8
        dec		ecx 
        jnz		loop1 

	} 
	EMMS_INSTRUCTION
#elif 0
	/*
not using constraints, so no double escape of registers
not necessary to push edi/esi
	 */
		__asm__ __volatile__ ( 
							  //        "mov		%edi, dest\n\t"
"mov	%edi, DWORD PTR [%ebp+8]\n\t"
							  //        "mov		%esi, src\n\t"
"mov	%esi, DWORD PTR [%ebp+12]\n\t"
							  //        "mov		%ecx, count\n\t"
"mov	%ecx, DWORD PTR [%ebp+16]\n\t"

        "shr		%ecx, 3\n\t"			// 8 bytes per iteration
"loop1_1:\n\t"
        "movq	%mm1,  0[%ESI]\n\t"			// Read in source data
        "movntq	0[%EDI], %mm1\n\t"			// Non-temporal stores
        "add		%esi, 8\n\t"
        "add		%edi, 8\n\t"
        "dec		%ecx \n\t"
        "jnz		loop1_1\n\t"
		"emms\n\t"
							   );
#elif 1
		__asm__ __volatile__ ( 
							  //        "mov		%esi, src\n\t"
							  //        "mov		%edi, dest\n\t"
							  //        "mov		%ecx, count\n\t"
        "shr		%%ecx, 3\n\t"			// 8 bytes per iteration
"0:\n\t"
        "movq	%%mm1,  0[%%esi]\n\t"			// Read in source data
        "movntq	0[%%edi], %%mm1\n\t"			// Non-temporal stores
        "add		%%esi, 8\n\t"
        "add		%%edi, 8\n\t"
        "dec		%%ecx \n\t"
        "jnz		0b\n\t"
		"emms\n\t"
		: /* no outputs */
		: "S" (src), "D" (dest), "c" (count)
							   );
#endif
}

/*
================
MMX_Memcpy64B

  165MB/sec
================
*/
void MMX_Memcpy64B( void *dest, const void *src, const int count ) {
#ifdef _MSC_VER
	_asm { 
        mov		esi, src 
        mov		edi, dest 
        mov		ecx, count 
        shr		ecx, 6		// 64 bytes per iteration

loop1: 
        prefetchnta 64[ESI]	// Prefetch next loop, non-temporal 
        prefetchnta 96[ESI] 

        movq mm1,  0[ESI]	// Read in source data 
        movq mm2,  8[ESI] 
        movq mm3, 16[ESI] 
        movq mm4, 24[ESI] 
        movq mm5, 32[ESI] 
        movq mm6, 40[ESI] 
        movq mm7, 48[ESI] 
        movq mm0, 56[ESI] 

        movntq  0[EDI], mm1	// Non-temporal stores 
        movntq  8[EDI], mm2 
        movntq 16[EDI], mm3 
        movntq 24[EDI], mm4 
        movntq 32[EDI], mm5 
        movntq 40[EDI], mm6 
        movntq 48[EDI], mm7 
        movntq 56[EDI], mm0 

        add		esi, 64 
        add		edi, 64 
        dec		ecx 
        jnz		loop1 
	} 
	EMMS_INSTRUCTION
#else
	__asm__ __volatile__ ( 
						  //"mov %%esi, src \n\t"
						  //"mov %%edi, dest \n\t"
						  //"mov %%ecx, count \n\t"
"shr %%ecx, 6 \n\t"// 64 bytes per iteration
"\n\t"
"1: \n\t"
"prefetchnta 64[%%ESI] \n\t"// Prefetch next loop, non-temporal
"prefetchnta 96[%%ESI] \n\t"
"\n\t"
"movq %%mm1, 0[%%ESI] \n\t"// Read in source data
"movq %%mm2, 8[%%ESI] \n\t"
"movq %%mm3, 16[%%ESI] \n\t"
"movq %%mm4, 24[%%ESI] \n\t"
"movq %%mm5, 32[%%ESI] \n\t"
"movq %%mm6, 40[%%ESI] \n\t"
"movq %%mm7, 48[%%ESI] \n\t"
"movq %%mm0, 56[%%ESI] \n\t"
"\n\t"
"movntq 0[%%EDI], %%mm1 \n\t"// Non-temporal stores
"movntq 8[%%EDI], %%mm2 \n\t"
"movntq 16[%%EDI], %%mm3 \n\t"
"movntq 24[%%EDI], %%mm4 \n\t"
"movntq 32[%%EDI], %%mm5 \n\t"
"movntq 40[%%EDI], %%mm6 \n\t"
"movntq 48[%%EDI], %%mm7 \n\t"
"movntq 56[%%EDI], %%mm0 \n\t"
"\n\t"
"add %%esi, 64 \n\t"
"add %%edi, 64 \n\t"
"dec %%ecx \n\t"
"jnz 1b \n\t"
"emms \n\t"
:
: "S" (src), "D" (dest), "c" (count)
    );
#endif
}

/*
================
MMX_Memcpy2kB

  240MB/sec
================
*/
void MMX_Memcpy2kB( void *dest, const void *src, const int count ) {
	byte *tbuf = (byte *)_alloca16(2048);
#ifdef _MSC_VER
	__asm { 
		push	ebx
        mov		esi, src
        mov		ebx, count
        shr		ebx, 11		// 2048 bytes at a time 
        mov		edi, dest

loop2k:
        push	edi			// copy 2k into temporary buffer
        mov		edi, tbuf
        mov		ecx, 32

loopMemToL1: 
        prefetchnta 64[ESI] // Prefetch next loop, non-temporal
        prefetchnta 96[ESI]

        movq mm1,  0[ESI]	// Read in source data
        movq mm2,  8[ESI]
        movq mm3, 16[ESI]
        movq mm4, 24[ESI]
        movq mm5, 32[ESI]
        movq mm6, 40[ESI]
        movq mm7, 48[ESI]
        movq mm0, 56[ESI]

        movq  0[EDI], mm1	// Store into L1
        movq  8[EDI], mm2
        movq 16[EDI], mm3
        movq 24[EDI], mm4
        movq 32[EDI], mm5
        movq 40[EDI], mm6
        movq 48[EDI], mm7
        movq 56[EDI], mm0
        add		esi, 64
        add		edi, 64
        dec		ecx
        jnz		loopMemToL1

        pop		edi			// Now copy from L1 to system memory
        push	esi
        mov		esi, tbuf
        mov		ecx, 32

loopL1ToMem:
        movq mm1, 0[ESI]	// Read in source data from L1
        movq mm2, 8[ESI]
        movq mm3, 16[ESI]
        movq mm4, 24[ESI]
        movq mm5, 32[ESI]
        movq mm6, 40[ESI]
        movq mm7, 48[ESI]
        movq mm0, 56[ESI]

        movntq 0[EDI], mm1	// Non-temporal stores
        movntq 8[EDI], mm2
        movntq 16[EDI], mm3
        movntq 24[EDI], mm4
        movntq 32[EDI], mm5
        movntq 40[EDI], mm6
        movntq 48[EDI], mm7
        movntq 56[EDI], mm0

        add		esi, 64
        add		edi, 64
        dec		ecx
        jnz		loopL1ToMem

        pop		esi			// Do next 2k block
        dec		ebx
        jnz		loop2k
		pop		ebx
	}
	EMMS_INSTRUCTION
#else

#ifdef __PIC__
		memcpy( dest, src, count );
#else
		/*
ebx problem:
when not compiling with -fPIC, compiles fine. No need to push/pop ebx, the constraints setup will save and restore ( or so it seems with no optimizations )

when compiling with -fPIC:
if not putting ebx in clobber list, "can't find a register in class 'BREG' while reloading 'asm'"
if putting ebx in clobber list, "PIC register 'ebx' clobbered in 'asm'"
but really, you don't want to put it in clobber list, you want to push/pop it

BREG error due to -masm=intel? ( doesn't sound likely - could test with the cpuid thing? )

tbuf constrained in memory since the loop loads it up in edi
		 */
	__asm__ __volatile__ ( 
"push %%ebx \n\t"
//"mov %%esi, src \n\t"
//"mov %%ebx, count \n\t"
"shr %%ebx, 11 \n\t"// 2048 bytes at a time
//"mov %%edi, dest \n\t"
"\n\t"
"loop2k: \n\t"
"push %%edi \n\t"// copy 2k into temporary buffer
//"mov %%edi, tbuf \n\t"
"mov %%edi, %0 \n\t"
"mov %%ecx, 32 \n\t"
"\n\t"
"loopMemToL1: \n\t"
"prefetchnta 64[%%ESI] \n\t"// Prefetch next loop, non-temporal
"prefetchnta 96[%%ESI] \n\t"
"\n\t"
"movq %%mm1, 0[%%ESI] \n\t"// Read in source data
"movq %%mm2, 8[%%ESI] \n\t"
"movq %%mm3, 16[%%ESI] \n\t"
"movq %%mm4, 24[%%ESI] \n\t"
"movq %%mm5, 32[%%ESI] \n\t"
"movq %%mm6, 40[%%ESI] \n\t"
"movq %%mm7, 48[%%ESI] \n\t"
"movq %%mm0, 56[%%ESI] \n\t"
"\n\t"
"movq 0[%%EDI], %%mm1 \n\t"// Store into L1
"movq 8[%%EDI], %%mm2 \n\t"
"movq 16[%%EDI], %%mm3 \n\t"
"movq 24[%%EDI], %%mm4 \n\t"
"movq 32[%%EDI], %%mm5 \n\t"
"movq 40[%%EDI], %%mm6 \n\t"
"movq 48[%%EDI], %%mm7 \n\t"
"movq 56[%%EDI], %%mm0 \n\t"
"add %%esi, 64 \n\t"
"add %%edi, 64 \n\t"
"dec %%ecx \n\t"
"jnz loopMemToL1 \n\t"
"\n\t"
"pop %%edi \n\t"// Now copy from L1 to system memory
"push %%esi \n\t"
//"mov %%esi, tbuf \n\t"
"mov %%esi, %0 \n\t"
"mov %%ecx, 32 \n\t"
"\n\t"
"loopL1ToMem: \n\t"
"movq %%mm1, 0[%%ESI] \n\t"// Read in source data from L1
"movq %%mm2, 8[%%ESI] \n\t"
"movq %%mm3, 16[%%ESI] \n\t"
"movq %%mm4, 24[%%ESI] \n\t"
"movq %%mm5, 32[%%ESI] \n\t"
"movq %%mm6, 40[%%ESI] \n\t"
"movq %%mm7, 48[%%ESI] \n\t"
"movq %%mm0, 56[%%ESI] \n\t"
"\n\t"
"movntq 0[%%EDI], %%mm1 \n\t"// Non-temporal stores
"movntq 8[%%EDI], %%mm2 \n\t"
"movntq 16[%%EDI], %%mm3 \n\t"
"movntq 24[%%EDI], %%mm4 \n\t"
"movntq 32[%%EDI], %%mm5 \n\t"
"movntq 40[%%EDI], %%mm6 \n\t"
"movntq 48[%%EDI], %%mm7 \n\t"
"movntq 56[%%EDI], %%mm0 \n\t"
"\n\t"
"add %%esi, 64 \n\t"
"add %%edi, 64 \n\t"
"dec %%ecx \n\t"
"jnz loopL1ToMem \n\t"
"\n\t"
"pop %%esi \n\t"// Do next 2k block
"dec %%ebx \n\t"
"jnz loop2k \n\t"
"pop %%ebx \n\t"
"emms \n\t"
:
: "m" (tbuf), "S" (src), "D" (dest), "b" (count)
//: "ebx"
    );

#endif // !ID_PIC

#endif
}


/*
================
idSIMD_MMX::Memcpy

  optimized memory copy routine that handles all alignment cases and block sizes efficiently
================
*/
void VPCALL idSIMD_MMX::Memcpy( void *dest0, const void *src0, const int count0 ) {
#ifndef _WIN32
	memcpy( dest0, src0, count0 );
#else
	// if copying more than 16 bytes and we can copy 8 byte aligned
	if ( count0 > 16 && !( ( (int)dest0 ^ (int)src0 ) & 7 ) ) {
		byte *dest = (byte *)dest0;
		byte *src = (byte *)src0;

		// copy up to the first 8 byte aligned boundary
		int count = ((int)dest) & 7;
		memcpy( dest, src, count );
		dest += count;
		src += count;
		count = count0 - count;

		// if there are multiple blocks of 2kB
		if ( count & ~4095 ) {
			MMX_Memcpy2kB( dest, src, count );
			src += (count & ~2047);
			dest += (count & ~2047);
			count &= 2047;
		}

		// if there are blocks of 64 bytes
		if ( count & ~63 ) {
			MMX_Memcpy64B( dest, src, count );
			src += (count & ~63);
			dest += (count & ~63);
			count &= 63;
		}

		// if there are blocks of 8 bytes
		if ( count & ~7 ) {
			MMX_Memcpy8B( dest, src, count );
			src += (count & ~7);
			dest += (count & ~7);
			count &= 7;
		}

		// copy any remaining bytes
		memcpy( dest, src, count );
	} else {
		// use the regular one if we cannot copy 8 byte aligned
		memcpy( dest0, src0, count0 );
	}
#endif // _WIN32
}

/*
================
idSIMD_MMX::Memset
================
*/
void VPCALL idSIMD_MMX::Memset( void* dest0, const int val, const int count0 ) {
#ifndef _WIN32
#else
	union {
		byte	bytes[8];
		word	words[4];
		dword	dwords[2];
	} dat;

	byte *dest = (byte *)dest0;
	int count = count0;

	while( count > 0 && (((int)dest) & 7) ) {
		*dest = val;
		dest++;
		count--;
	}
	if ( !count ) {
		return;
	}

	dat.bytes[0] = val;
	dat.bytes[1] = val;
	dat.words[1] = dat.words[0];
	dat.dwords[1] = dat.dwords[0];

	if ( count >= 64 ) {
#ifdef _MSC_VER
		__asm {
			mov edi, dest 
			mov ecx, count 
			shr ecx, 6				// 64 bytes per iteration 
			movq mm1, dat			// Read in source data 
			movq mm2, mm1
			movq mm3, mm1
			movq mm4, mm1
			movq mm5, mm1
			movq mm6, mm1
			movq mm7, mm1
			movq mm0, mm1
loop1: 
			movntq  0[EDI], mm1		// Non-temporal stores 
			movntq  8[EDI], mm2 
			movntq 16[EDI], mm3 
			movntq 24[EDI], mm4 
			movntq 32[EDI], mm5 
			movntq 40[EDI], mm6 
			movntq 48[EDI], mm7 
			movntq 56[EDI], mm0 

			add edi, 64 
			dec ecx 
			jnz loop1 
		}
#else
/*
dat constrained in memory
*/
		__asm__ __volatile__ (
							  //"mov %%edi, dest \n\t"
							  //"mov %%ecx, count \n\t"
"shr %%ecx, 6 \n\t"// 64 bytes per iteration
//"movq %%mm1, dat \n\t"// Read in source data
"movq %%mm1, %0 \n\t"
"movq %%mm2, %%mm1 \n\t"
"movq %%mm3, %%mm1 \n\t"
"movq %%mm4, %%mm1 \n\t"
"movq %%mm5, %%mm1 \n\t"
"movq %%mm6, %%mm1 \n\t"
"movq %%mm7, %%mm1 \n\t"
"movq %%mm0, %%mm1 \n\t"
"loop1_3: \n\t"
"movntq 0[%%EDI], %%mm1 \n\t"// Non-temporal stores
"movntq 8[%%EDI], %%mm2 \n\t"
"movntq 16[%%EDI], %%mm3 \n\t"
"movntq 24[%%EDI], %%mm4 \n\t"
"movntq 32[%%EDI], %%mm5 \n\t"
"movntq 40[%%EDI], %%mm6 \n\t"
"movntq 48[%%EDI], %%mm7 \n\t"
"movntq 56[%%EDI], %%mm0 \n\t"
"\n\t"
"add %%edi, 64 \n\t"
"dec %%ecx \n\t"
"jnz loop1_3 \n\t"
:
: "m" (dat), "D" (dest), "c" (count)
							  );
#endif
		dest += ( count & ~63 );
		count &= 63;
	}

	if ( count >= 8 ) {
#ifdef _MSC_VER
		__asm {
			mov edi, dest 
			mov ecx, count 
			shr ecx, 3				// 8 bytes per iteration 
			movq mm1, dat			// Read in source data 
loop2: 
			movntq  0[EDI], mm1		// Non-temporal stores 

			add edi, 8
			dec ecx 
			jnz loop2
		}
#else
/*
dat constrained in memory
*/
		__asm__ __volatile__ (
							  //"mov %%edi, dest \n\t"
							  //"mov %%ecx, count \n\t"
"shr %%ecx, 3 \n\t"// 8 bytes per iteration
//"movq %%mm1, dat \n\t"// Read in source data
"movq %%mm1, %0 \n\t"// Read in source data
"loop2: \n\t"
"movntq 0[%%EDI], %%mm1 \n\t"// Non-temporal stores
"\n\t"
"add %%edi, 8 \n\t"
"dec %%ecx \n\t"
"jnz loop2 \n\t"
:
: "m" (dat), "D" (dest), "c" (count)
							  );
#endif
		dest += (count & ~7);
		count &= 7;
	}

	while( count > 0 ) {
		*dest = val;
		dest++;
		count--;
	}

	EMMS_INSTRUCTION 
#endif	// _WIN32
}

#endif
