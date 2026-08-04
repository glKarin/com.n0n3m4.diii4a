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

#ifndef __MATH_SIMD_SSSE3_H__
#define __MATH_SIMD_SSSE3_H__

#include "Simd_SSE2.h"

/*
===============================================================================

	SSSE3 implementation of idSIMDProcessor

===============================================================================
*/

#ifdef __linux__
#define ALLOW_SSSE3 __attribute__ ((__target__ ("ssse3")))
#else
#define ALLOW_SSSE3
#endif


class idSIMD_SSSE3 : public idSIMD_SSE2 {
public:
	idSIMD_SSSE3();

#ifdef ENABLE_SSE_PROCESSORS
	virtual bool ConvertRowToRGBA8( const byte *srcPtr, int width, int bitsPerPixel, bool bgr, byte *dstPtr ) override ALLOW_SSSE3;
#endif
};

#endif /* !__MATH_SIMD_SSSE3_H__ */
