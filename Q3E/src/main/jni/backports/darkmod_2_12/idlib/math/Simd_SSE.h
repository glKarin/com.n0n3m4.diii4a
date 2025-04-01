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

#ifndef __MATH_SIMD_SSE_H__
#define __MATH_SIMD_SSE_H__

#include "Simd_Generic.h"

/*
===============================================================================

	SSE implementation of idSIMDProcessor

===============================================================================
*/

class idSIMD_SSE : public idSIMD_Generic {
public:
	idSIMD_SSE();

#ifdef ENABLE_SSE_PROCESSORS
	virtual void CullByFrustum( idDrawVert *verts, const int numVerts, const idPlane frustum[6], byte *pointCull, float epsilon ) override;
	virtual void CullByFrustum2( idDrawVert *verts, const int numVerts, const idPlane frustum[6], unsigned short *pointCull, float epsilon ) override;
	virtual void CullTrisByFrustum( idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes, const idPlane frustum[6], byte *triCull, float epsilon ) override;
#endif
};

#endif /* !__MATH_SIMD_SSE_H__ */
