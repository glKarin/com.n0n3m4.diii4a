/*
extdll.h - must be included into the all ui files
Copyright (C) 2010 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef EXTDLL_H
#define EXTDLL_H

#include "xash3d_types.h"

#ifndef EXPORT
#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#elif defined(__GNUC__)
#define EXPORT __attribute__((visibility("default")))
#else
#define EXPORT
#endif
#endif

// shut-up compiler warnings
#ifdef _MSC_VER
#pragma warning(disable : 4305)	// int or float data truncation
#pragma warning(disable : 4201)	// nameless struct/union
#pragma warning(disable : 4514)	// unreferenced inline function removed
#pragma warning(disable : 4100)	// unreferenced formal parameter
#pragma warning(disable : 4244)	// conversion from 'float' to 'int', possible loss of data
#pragma warning(disable : 4520) // multiple default constructors specified
#pragma warning(disable : 4996) // This function or variable may be unsafe
// disable c++11 on old msvc
#if _MSC_VER < 1800 && !defined MY_COMPILER_SUCKS
#define MY_COMPILER_SUCKS
#endif
#endif
// Misc C-runtime library headers
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#ifdef MY_COMPILER_SUCKS
// C++11 keywords
#define final
#define constexpr
#define override
#define nullptr NULL
#endif

#ifdef bound
#undef bound
#endif

#if !defined(_WIN32) && !defined(__MINGW32__)
#define stricmp		strcasecmp
#define strnicmp	strncasecmp
#else
#define strnicmp	_strnicmp
#define stricmp		_stricmp
#define snprintf	_snprintf
#endif

typedef unsigned char byte;
typedef unsigned int uint;

#include "menu_int.h"

#endif//EXTDLL_H
