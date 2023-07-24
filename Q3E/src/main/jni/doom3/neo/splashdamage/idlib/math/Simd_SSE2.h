// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __MATH_SIMD_SSE2_H__
#define __MATH_SIMD_SSE2_H__

/*
===============================================================================

	SSE2 implementation of idSIMDProcessor

===============================================================================
*/

class idSIMD_SSE2 : public idSIMD_SSE {
#ifdef ID_WIN_X86_ASM
public:
	virtual const char * VPCALL GetName( void ) const;

	//virtual void VPCALL MatX_LowerTriangularSolve( const idMatX &L, float *x, const float *b, const int n, int skip = 0 );
	//virtual void VPCALL MatX_LowerTriangularSolveTranspose( const idMatX &L, float *x, const float *b, const int n );

	virtual void VPCALL ConvertJointQuatsToJointMats( idJointMat *jointMats, const idJointQuat *jointQuats, const int numJoints );
	virtual void VPCALL TransformJoints( idJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint );
	virtual void VPCALL UntransformJoints( idJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint );
	virtual void VPCALL MultiplyJoints( idJointMat *result, const idJointMat *joints1, const idJointMat *joints2, const int numJoints );
	virtual void VPCALL TransformVerts( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights );
	virtual void VPCALL TransformShadowVerts( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idDrawVert *base, const jointWeight_t *weights, const int numWeights );
	virtual void VPCALL TransformShadowVerts( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idDrawVert *base, const short *weights, const int numWeights );
	virtual void VPCALL TransformShadowVerts( struct shadowCache_s *verts, const int numVerts, const idJointMat *joints, const idDrawVert *base, const short *weights, const int numWeights );
#if !defined(SD_USE_DRAWVERT_SIZE_32)
	virtual void VPCALL TransformVertsAndTangents( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights );
	virtual void VPCALL TransformVertsAndTangentsFast( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights );
#endif
	virtual int  VPCALL ShadowVolume_CountFacing( const byte *facing, const int numFaces );
	virtual int  VPCALL ShadowVolume_CountFacingCull( byte *facing, const int numFaces, const vertIndex_t *indexes, const byte *cull );
#if 0
#if defined ( GL_INDEX_INT )
	virtual int  VPCALL ShadowVolume_CreateSilTriangles( vertIndex_t *shadowIndexes, const byte *facing, const silEdge_t *silEdges, const int numSilEdges );
#endif
	virtual int  VPCALL ShadowVolume_CreateCapTriangles( vertIndex_t *shadowIndexes, const byte *facing, const vertIndex_t *indexes, const int numIndexes );
	virtual int  VPCALL ShadowVolume_CreateSilTrianglesParallel( vertIndex_t *shadowIndexes, const byte *facing, const silEdge_t *silEdges, const int numSilEdges );
	virtual int  VPCALL ShadowVolume_CreateCapTrianglesParallel( vertIndex_t *shadowIndexes, const byte *facing, const vertIndex_t *indexes, const int numIndexes );
#endif
	virtual void VPCALL MixedSoundToSamples( short *samples, const float *mixBuffer, const int numSamples );

#endif
};

#endif /* !__MATH_SIMD_SSE2_H__ */
