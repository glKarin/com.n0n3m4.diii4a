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

#pragma once

#include "Simd_AVX.h"

/*
===============================================================================

	AVX2 implementation of idSIMDProcessor

===============================================================================
*/

#ifdef __linux__
#define ALLOW_AVX2 __attribute__ ((__target__ ("avx2")))  __attribute__ ((__target__ ("fma")))
#else
#define ALLOW_AVX2
#endif


class idSIMD_AVX2 : public idSIMD_AVX {
public:
	idSIMD_AVX2();

#ifdef ENABLE_SSE_PROCESSORS
	virtual void CullByFrustum( idDrawVert *verts, const int numVerts, const idPlane frustum[6], byte *pointCull, float epsilon ) override ALLOW_AVX2;
	virtual void CullByFrustum2( idDrawVert *verts, const int numVerts, const idPlane frustum[6], unsigned short *pointCull, float epsilon ) override ALLOW_AVX2;
	virtual void DeriveTangents( idPlane *planes, idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes ) override ALLOW_AVX2;
	virtual void NormalizeTangents( idDrawVert *verts, const int numVerts ) override ALLOW_AVX2;
#endif
};
