/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
//  cl_dll.h
//

// 4-23-98  JOHN

//
//  This DLL is linked by the client when they first initialize.
// This DLL is responsible for the following tasks:
//		- Loading the HUD graphics upon initialization
//		- Drawing the HUD graphics every frame
//		- Handling the custum HUD-update packets
//
#pragma once
#if !defined(CL_DLL_H)
#define CL_DLL_H
#include "build.h"
typedef unsigned char byte;
typedef unsigned short word;
typedef float vec_t;
// redefine
//typedef int ( *pfnUserMsgHook )( const char *pszName, int iSize, void *pbuf );

#include "util_vector.h"

#include "../engine/cdll_int.h"
#include "../dlls/cdll_dll.h"

#if !XASH_WIN32
#define _cdecl
#endif
#include "exportdef.h"
#include <string.h>
#include "safe_snprintf.h"
#ifndef __restrict
#define __restrict
#endif
#if !HAVE_STRLCPY
extern "C" size_t strlcpy(char * __restrict dst, const char * __restrict src, size_t dsize);
#endif
#if !HAVE_STRLCAT
extern "C" size_t strlcat(char * __restrict dst, const char * __restrict src, size_t dsize);
#endif
#if HAVE_CMATH
#include <cmath>
#else
#include <math.h>
#endif
extern cl_enginefunc_t gEngfuncs;
#include "../engine/mobility_int.h"
extern mobile_engfuncs_t *gMobileEngfuncs;
#endif
