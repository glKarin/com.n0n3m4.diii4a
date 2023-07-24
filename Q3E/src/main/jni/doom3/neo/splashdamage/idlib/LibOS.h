// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __LIBOS_H__
#define __LIBOS_H__

#if defined( _WIN32 )
	// _WIN32 always defined
	// _WIN64 also defined for x64 target
	#if !defined( _WIN64 ) && !defined( _XENON )
		#define ID_WIN_X86_ASM
		#define ID_WIN_X86_MMX
		#define ID_WIN_X86_SSE
		//#define ID_WIN_X86_SSE2
	#endif

	// we should never rely on that define in our code. this is here so dodgy external libraries don't get confused
	#ifndef WIN32
		#define WIN32
	#endif

#if !defined( _XENON )
	#undef _XENON
	#undef _CONSOLE								// Used to comment out code that can't be used on a console
	#define _OPENGL
	#define _LITTLE_ENDIAN
	#undef _CASE_SENSITIVE_FILESYSTEM
#endif

	#define NEWLINE "\r\n"

	#pragma warning( disable : 4100 )			// unreferenced formal parameter
	#pragma warning( disable : 4127 )			// conditional expression is constant
	#pragma warning( disable : 4244 )			// conversion to smaller type, possible loss of data
	#pragma warning( disable : 4714 )			// function marked as __forceinline not inlined
	#pragma warning( disable : 4750 )			// function using _alloca inlined in a loop

	#ifndef WINVER
#if defined( ID_DEDICATED ) || defined( GAME_DLL )
		#define WINVER 0x0500
#else
		#define WINVER 0x0501
#endif
	#endif

	#ifndef _WIN32_WINNT
		#define _WIN32_WINNT WINVER
	#endif

	#ifndef _WIN32_WINDOWS
		#define _WIN32_WINDOWS WINVER
	#endif

	#ifndef _WIN32_IE
		#define _WIN32_IE WINVER
	#endif

	#define WIN32_LEAN_AND_MEAN				// Exclude rarely-used stuff from Windows headers

	#ifndef VC_EXTRALEAN
	#define VC_EXTRALEAN					// Exclude rarely-used stuff from Windows headers
	#endif

	#include <malloc.h>						// no malloc.h on mac or unix
#if !defined( _XENON )
	#include <windows.h>
#endif

	// stupid namespace polluting Microsoft monkeys
	#undef FindText
	#undef IsMinimized

#else /* _WIN32 */

	/*
	PATH_MAX vs MAX_PATH mess:

	* MAX_PATH is a Windows invention - god knows how it's actually defined, if you ever find it in MSDN give me a poke

	* POSIX (IEEE Std 1003.1, 2004 Edition):

	PATH_MAX
	Maximum number of bytes in a pathname, including the terminating
	null character. Minimum Acceptable Value: _POSIX_PATH_MAX
	[...]

	_POSIX_PATH_MAX
	Maximum number of bytes in a pathname. Value: 256
	*/

	#include <limits.h>
	#define MAX_PATH PATH_MAX
#endif

#if defined( __linux__ )
	#undef WIN32
	#undef _XENON
	#undef _CONSOLE
	#define _OPENGL
	#define _LITTLE_ENDIAN
	#define _CASE_SENSITIVE_FILESYSTEM

	#define NEWLINE				"\n"

	// access to 64 bit int data types
	#include <stdint.h>
#endif /* __linux__ */

#if defined( MACOS_X )
	#define __STDC_ISO_10646__

	#include <pthread.h>
	#include <stddef.h>
	
	#undef WIN32
	#undef _XENON
	#undef _CONSOLE
	#define _OPENGL
	#if defined( __ppc__ )
	#undef _LITTLE_ENDIAN
	#else
	#define _LITTLE_ENDIAN
	#endif
	#define _CASE_SENSITIVE_FILESYSTEM

	#define NEWLINE				"\n"
#endif /* MACOS_X */

#if defined( _XENON )
	// inferred _WIN32 define
	#ifndef WIN32
		#define WIN32
	#endif

	#define _CONSOLE
	#undef _OPENGL
	#undef _LITTLE_ENDIAN
	#undef _CASE_SENSITIVE_FILESYSTEM

	// _XENON
	#undef _XENON
	#define _XENON

	#undef XENON
	#define XENON

	#undef DBG
	#define DBG 1

	#include <xtl.h>
	#include <xgraphics.h>
	#include <xaudio.h>
	#include <x3daudio.h>
	#include <xmp.h>
	// _XENON

	#define NEWLINE				"\r\n"
#endif /* _XENON */

#endif /* !__LIBOS_H__ */
