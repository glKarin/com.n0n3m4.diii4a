/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __MATH_SIMD_SSE2_H__
#define __MATH_SIMD_SSE2_H__

/*
===============================================================================

	SSE2 implementation of idSIMDProcessor

===============================================================================
*/

class idSIMD_SSE2 : public idSIMD_SSE
{
	public:
#if defined(__GNUC__) && defined(__SSE2__) || ( ( defined(_M_X64) || defined(__x86_64__) ) && defined(_USE_SSE) ) || ( ( defined(__arm__) || defined(__aarch64__) ) && defined(_ARM_SIMD_SSE2NEON) )

		virtual const char *VPCALL GetName(void) const;
		virtual void VPCALL CmpLT(byte *dst,			const byte bitNum,		const float *src0,		const float constant,	const int count);

		// DOOM3-BFG
		virtual void VPCALL BlendJoints( idJointQuat* joints, const idJointQuat* blendJoints, const float lerp, const int* index, const int numJoints );
		virtual void VPCALL ConvertJointQuatsToJointMats( idJointMat* jointMats, const idJointQuat* jointQuats, const int numJoints );
		virtual void VPCALL ConvertJointMatsToJointQuats( idJointQuat* jointQuats, const idJointMat* jointMats, const int numJoints );
		virtual void VPCALL TransformJoints( idJointMat* jointMats, const int* parents, const int firstJoint, const int lastJoint );
		virtual void VPCALL UntransformJoints( idJointMat* jointMats, const int* parents, const int firstJoint, const int lastJoint );

		// The Dark Mod
		virtual	void MinMax( idVec3 &min, idVec3 &max, const idDrawVert *src, const int count );
#if 0 // slower
		virtual void NormalizeTangents( idDrawVert *verts, const int numVerts );
		virtual void TransformVerts( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idVec4 *weights, const int *index, const int numWeights );
#endif
		virtual void DeriveTangents( idPlane *planes, idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes );
		virtual int  CreateVertexProgramShadowCache( idVec4 *vertexCache, const idDrawVert *verts, const int numVerts );
		virtual void TracePointCull( byte *cullBits, byte &totalOr, const float radius, const idPlane *planes, const idDrawVert *verts, const int numVerts );

#elif defined(_MSC_VER) && defined(_M_IX86)
		virtual const char *VPCALL GetName(void) const;

		//virtual void VPCALL MatX_LowerTriangularSolve( const idMatX &L, float *x, const float *b, const int n, int skip = 0 );
		//virtual void VPCALL MatX_LowerTriangularSolveTranspose( const idMatX &L, float *x, const float *b, const int n );

		virtual void VPCALL MixedSoundToSamples(short *samples, const float *mixBuffer, const int numSamples);

#endif
};

#endif /* !__MATH_SIMD_SSE2_H__ */
