/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
#ifndef SYS_INCLUDES_H
#define SYS_INCLUDES_H

// Include the various platform specific header files (windows.h, etc)

/*
================================================================================================

	Windows

================================================================================================
*/

// RB: windows specific stuff should only be set on Windows
#if defined(_WIN32)

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// prevent auto literal to string conversion

#ifndef _D3SDK
#ifndef GAME_DLL

//#define WINVER				0x501
//stgatilov: use only WinAPI functionality present on Vista and above
#define _WIN32_WINNT _WIN32_WINNT_VISTA

#ifdef NO_MFC
#include <winsock2.h>						// greebo: Include this before windows.h
#include <windows.h>						// for qgl.h
#else
#include "../tools/comafx/StdAfx.h"
#endif

#include <winsock2.h>
#include <mmsystem.h>
#include <mmreg.h>

#include <mmreg.h>

#include <InitGuid.h>						//https://social.msdn.microsoft.com/Forums/vstudio/en-US/16fe61c7-0ff0-400a-9f9e-39768edbb6b4/what-is-dxguidlib?forum=vcgeneral
#define DIRECTINPUT_VERSION  0x0800			// was 0x0700 with the old mssdk
#include <dinput.h>

#ifndef _MSC_VER

// RB: was missing in MinGW/include/winuser.h
#ifndef MAPVK_VSC_TO_VK_EX
#define MAPVK_VSC_TO_VK_EX 3
#endif

// RB begin
#if defined(__MINGW32__)
//#include <sal.h> 	// RB: missing __analysis_assume
// including <sal.h> breaks some STL crap ...

#ifndef __analysis_assume
#define __analysis_assume( x )
#endif

#endif
// RB end

#endif


#endif /* !GAME_DLL */
#endif /* !_D3SDK */

// DG: intrinsics for GCC
#if defined(__GNUC__) && defined(__SSE2__)
#include <emmintrin.h>

// TODO: else: alternative implementations?
#endif
// DG end

#ifdef _MSC_VER
#include <intrin.h>			// needed for intrinsics like _mm_setzero_si28

#pragma warning(disable : 4100)				// unreferenced formal parameter
#pragma warning(disable : 4127)				// conditional expression is constant
#pragma warning(disable : 4244)				// conversion to smaller type, possible loss of data
#pragma warning(disable : 4714)				// function marked as __forceinline not inlined
#pragma warning(disable : 4996)				// unsafe string operations
#endif // _MSC_VER

#include <windows.h>						// for gl.h

#undef FindText								// stupid namespace poluting Microsoft monkeys

#undef FORCEINLINE							//stgatilov: use portable ID_FORCE_INLINE instead of FORCEINLINE defined in windows.h

#elif defined(__linux__) || defined(__FreeBSD__)

#include <signal.h>
#include <pthread.h>

#endif // #if defined(_WIN32)
// RB end

#include <stdlib.h>							// no malloc.h on mac or unix
#undef FindText								// fix namespace pollution


/*
================================================================================================

	Common Include Files

================================================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <typeinfo>
#include <errno.h>
#include <math.h>
#include <limits.h>
#include <memory>
// RB: added <stdint.h> for missing uintptr_t with MinGW
#include <stdint.h>
// RB end
// Yamagi: <stddef.h> for ptrdiff_t on FreeBSD
#include <stddef.h>
// Yamagi end

//-----------------------------------------------------

// Hacked stuff we may want to consider implementing later
class idScopedGlobalHeap
{
};

#endif // SYS_INCLUDES_H
