// Copyright (C) 2007 Id Software, Inc.
//

#ifdef _WIN32
#include <windows.h>
#endif
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#ifndef MACOS_X
#include <malloc.h>
#endif
#include <string.h>
#include <wchar.h>
#include <stdarg.h>

#pragma warning( disable : 4533 )

#undef min
#undef abs
#undef max

typedef unsigned char			byte;		// 8 bits
typedef unsigned short			word;		// 16 bits
typedef unsigned int			dword;		// 32 bits
typedef unsigned int			uint;
typedef unsigned long			ulong;

#define UINT_PTR						unsigned long
