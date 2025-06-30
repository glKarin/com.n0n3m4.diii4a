/* Copyright (c) 2002-2012 Croteam Ltd.
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */


#define ENGINE_INTERNAL 1
#define ENGINE_EXPORTS 1

#define __STDC_LIMIT_MACROS 1

#ifdef _MSC_VER
#define __extern extern
#else
#define __extern
#endif

#ifdef _MSC_VER
#ifndef INDEX_T
#define INDEX_T INDEX
#endif
#include <stdint.h>
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <time.h>
#include <math.h>
#include <search.h>   // for qsort
#include <float.h>    // for FPU control

#ifdef _MSC_VER
#include <io.h>
#include <malloc.h>
#include <conio.h>
#include <crtdbg.h>
#include <winsock2.h>
#include <windows.h>
#include <mmsystem.h> // for timers
#elif defined(_DIII4A) //karin: no SDL
#else
#include "SDL.h"
#endif

#if PLATFORM_MACOSX
#ifdef MACOS
#undef MACOS
#endif
#ifdef TARGET_OS_MAC
#undef TARGET_OS_MAC
#endif
#endif

#include <Engine/Base/Types.h>
#include <Engine/Base/Assert.h>
#include <Engine/Base/iconvlite.h>
