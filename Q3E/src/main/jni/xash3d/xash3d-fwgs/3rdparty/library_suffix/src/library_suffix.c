/*
library_suffix.c -

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "library_suffix.h"

/*
============
Q_PlatformStringByID

Returns name of operating system by ID. Without any spaces.
============
*/
const char *Q_PlatformStringByID( const int platform )
{
	switch( platform )
	{
	case PLATFORM_WIN32:
		return "win32";
	case PLATFORM_ANDROID:
		return "android";
	case PLATFORM_LINUX:
		return "linux";
	case PLATFORM_APPLE:
		return "apple";
	case PLATFORM_FREEBSD:
		return "freebsd";
	case PLATFORM_NETBSD:
		return "netbsd";
	case PLATFORM_OPENBSD:
		return "openbsd";
	case PLATFORM_EMSCRIPTEN:
		return "emscripten";
	case PLATFORM_DOS4GW:
		return "DOS4GW";
	case PLATFORM_HAIKU:
		return "haiku";
	case PLATFORM_SERENITY:
		return "serenity";
	case PLATFORM_IRIX:
		return "irix";
	case PLATFORM_NSWITCH:
		return "nswitch";
	case PLATFORM_PSVITA:
		return "psvita";
	case PLATFORM_WASI:
		return "wasi";
	case PLATFORM_SUNOS:
		return "sunos";
	case PLATFORM_HURD:
		return "hurd";
	case PLATFORM_PSP:
		return "psp";
	}

	return "unknown";
}

/*
============
Q_buildos

Shortcut for Q_PlatformStringByID( XASH_PLATFORM )
============
*/
const char *Q_buildos( void )
{
	return Q_PlatformStringByID( XASH_PLATFORM );
}


/*
============
Q_ArchitectureStringByID

Returns name of the architecture by it's ID. Without any spaces.
============
*/
const char *Q_ArchitectureStringByID( int arch, unsigned int abi, int endianness, int is64 )
{
	// I don't want to change this function prototype
	// and don't want to use static buffer either
	// so encode all possible variants... :)
	switch( arch )
	{
	case ARCHITECTURE_AMD64:
		return "amd64";
	case ARCHITECTURE_X86:
		return "i386";
	case ARCHITECTURE_E2K:
		return "e2k";
	case ARCHITECTURE_JS:
		return "javascript";
	case ARCHITECTURE_PPC:
		return endianness == ENDIANNESS_LITTLE ?
			( is64 ? "ppc64el" : "ppcel" ):
			( is64 ? "ppc64" : "ppc" );
	case ARCHITECTURE_MIPS:
		return endianness == ENDIANNESS_LITTLE ?
			( is64 ? "mips64el" : "mipsel" ):
			( is64 ? "mips64" : "mips" );
	case ARCHITECTURE_ARM:
		// no support for big endian ARM here
		if( endianness == ENDIANNESS_LITTLE )
		{
			const unsigned int ver = ( abi >> ARCH_ARM_VER_SHIFT ) & ARCH_ARM_VER_MASK;
			const int hardfp = FBitSet( abi, ARCH_ARM_HARDFP );

			if( is64 )
				return "arm64"; // keep as arm64, it's not aarch64!

			switch( ver )
			{
			case 8:
				return hardfp ? "armv8_32hf" : "armv8_32l";
			case 7:
				return hardfp ? "armv7hf" : "armv7l";
			case 6:
				return "armv6l";
			case 5:
				return "armv5l";
			case 4:
				return "armv4l";
			}
		}
		break;
	case ARCHITECTURE_RISCV:
		switch( abi )
		{
		case ARCH_RISCV_FP_SOFT:
			return is64 ? "riscv64" : "riscv32";
		case ARCH_RISCV_FP_SINGLE:
			return is64 ? "riscv64f" : "riscv32f";
		case ARCH_RISCV_FP_DOUBLE:
			return is64 ? "riscv64d" : "riscv32d";
		}
		break;
	case ARCHITECTURE_WASM:
		return is64 ? "wasm64" : "wasm32";
	}

	return is64 ?
		( endianness == ENDIANNESS_LITTLE ? "unknown64el" : "unknownel" ) :
		( endianness == ENDIANNESS_LITTLE ? "unknown64be" : "unknownbe" );
}

/*
============
Q_buildarch

Shortcut for Q_ArchitectureStringByID( XASH_PLATFORM )
============
*/
const char *Q_buildarch( void )
{
	return Q_ArchitectureStringByID(
		XASH_ARCHITECTURE,
		XASH_ARCHITECTURE_ABI,
		XASH_ENDIANNESS,
#if XASH_64BIT
		1
#else
		0
#endif
	);
}

/*
====================
COM_StripIntelSuffix

Strips legacy Intel architecture suffix at the end of string
====================
*/
void COM_StripIntelSuffix( char *out )
{
	char *suffix = strrchr( out, '_' );

	if( !suffix )
		return;

	// not enough to fit the pattern
	if( strlen( suffix + 1 ) != 4 )
		return;

	if( suffix[1] == 'i'
		&& suffix[2] >= '3' && suffix[2] <= '6'
		&& suffix[3] == '8'
		&& suffix[4] == '6' )
	{
		// strip the suffix as it matches the pattern
		suffix[0] = 0;
	}
}

/*
=============================
COM_GenerateCommonLibraryName

Fills `out` with library name of OS/CPU combo we're running on
=============================
*/
int COM_GenerateCommonLibraryName( const char *name, char *out, size_t size )
{
	return COM_GenerateLibraryName( out, size,
#if XASH_ANDROID
		"lib",
#else
		"",
#endif
		name,
		XASH_PLATFORM,
		XASH_ARCHITECTURE, XASH_ARCHITECTURE_ABI, XASH_ENDIANNESS,
#if XASH_64BIT
		1,
#else
		0,
#endif
		OS_LIB_EXT
	);
}

/*
=======================
COM_GenerateLibraryName

Lets generate custom DLL name for any specified target
=======================
*/
int COM_GenerateLibraryName( char *out, size_t size,
	const char *prefix, const char *name,
	int platform,
	int arch, unsigned int abi, int endianness, int is64,
	const char *ext )
{
	int len;

#ifdef _DIII4A //karin: library file name is plain format: libxxx.so
    len = snprintf( out, size, "%s%s.%s", prefix, name, ext );
#else
	if(( platform == PLATFORM_WIN32 || platform == PLATFORM_LINUX || platform == PLATFORM_APPLE ))
	{
		if( arch == ARCHITECTURE_X86 )
		{
			len = snprintf( out, size, "%s%s.%s", prefix, name, ext );
		}
		else
		{
			len = snprintf( out, size, "%s%s_%s.%s", prefix, name, Q_ArchitectureStringByID( arch, abi, endianness, is64 ), ext );
		}
	}
	else
	{
		len = snprintf( out, size, "%s%s_%s_%s.%s", prefix, name, Q_PlatformStringByID( platform ), Q_ArchitectureStringByID( arch, abi, endianness, is64 ), ext );
	}
#endif

	if( len >= size )
	{
		out[size - 1] = 0;
		return -1;
	}

	return len;
}
