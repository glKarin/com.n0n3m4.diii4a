// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __MATH_SIMD_SSE2_H__
#define __MATH_SIMD_SSE2_H__

/*
===============================================================================

	SSE2 implementation of idSIMDProcessor

===============================================================================
*/

class idSIMD_SSE2 : public idSIMD_SSE {
#ifdef _WIN32
public:
	virtual const char * VPCALL GetName( void ) const;

	//virtual void VPCALL MatX_LowerTriangularSolve( const idMatX &L, float *x, const float *b, const int n, int skip = 0 );
	//virtual void VPCALL MatX_LowerTriangularSolveTranspose( const idMatX &L, float *x, const float *b, const int n );

	virtual void VPCALL MixedSoundToSamples( short *samples, const float *mixBuffer, const int numSamples );

	//HUMANHEAD rww
	virtual void VPCALL Memcpy( void *dst, const void *src, const int count );
	virtual void VPCALL TransformJoints( idJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint );
	//HUMANHEAD END

#if NEW_MESH_TRANSFORM
	virtual void VPCALL MultiplyJoints( idJointMat *result, const idJointMat *joints1, const idJointMat *joints2, const int numJoints );
	virtual void VPCALL TransformVertsNew( idDrawVert *verts, const int numVerts, idBounds &bounds, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights );
	virtual void VPCALL TransformVertsAndTangents( idDrawVert *verts, const int numVerts, idBounds &bounds, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights );
	virtual void VPCALL TransformVertsAndTangentsFast( idDrawVert *verts, const int numVerts, idBounds &bounds, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights );
#endif

#if SIMD_SHADOW
	virtual int  VPCALL ShadowVolume_CountFacing( const byte *facing, const int numFaces );
	virtual int  VPCALL ShadowVolume_CountFacingCull( byte *facing, const int numFaces, const int *indexes, const byte *cull );
	virtual int  VPCALL ShadowVolume_CreateSilTriangles( int *shadowIndexes, const byte *facing, const silEdge_s *silEdges, const int numSilEdges );
	virtual int  VPCALL ShadowVolume_CreateCapTriangles( int *shadowIndexes, const byte *facing, const int *indexes, const int numIndexes );
#endif

#endif
};

#endif /* !__MATH_SIMD_SSE2_H__ */
