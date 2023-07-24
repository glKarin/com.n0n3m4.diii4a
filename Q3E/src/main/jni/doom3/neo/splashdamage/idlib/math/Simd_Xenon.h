// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __MATH_SIMD_XENON_H__
#define __MATH_SIMD_XENON_H__

/*
===============================================================================

	AltiVec implementation of idSIMDProcessor

===============================================================================
*/

#ifdef _XENON
#include <ppcintrinsics.h>
#endif

class idSIMD_Xenon : public idSIMD_Generic {
#ifdef _XENON
public:
	virtual const char * VPCALL GetName( void ) const;

	virtual void VPCALL MinMax( idVec3 &min,		idVec3 &max,			const idDrawVert *src,	const int count );
	virtual void VPCALL MinMax( idVec3 &min,		idVec3 &max,			const idDrawVert *src,	const int *indexes,		const int count );

	virtual void VPCALL BlendJoints( idJointQuat *joints, const idJointQuat *blendJoints, const float lerp, const int *index, const int numJoints );
	virtual void VPCALL BlendJointsFast( idJointQuat *joints, const idJointQuat *blendJoints, const float lerp, const int *index, const int numJoints );
	virtual void VPCALL ConvertJointQuatsToJointMats( idJointMat *jointMats, const idJointQuat *jointQuats, const int numJoints );
	virtual void VPCALL ConvertJointMatsToJointQuats( idJointQuat *jointQuats, const idJointMat *jointMats, const int numJoints );
	virtual void VPCALL TransformJoints( idJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint );
	virtual void VPCALL UntransformJoints( idJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint );
	virtual void VPCALL MultiplyJoints( idJointMat *result, const idJointMat *joints1, const idJointMat *joints2, const int numJoints );
	virtual void VPCALL TransformVerts( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights );
	virtual void VPCALL TransformShadowVerts( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idDrawVert *base, const jointWeight_t *weights, const int numWeights );
	//virtual void VPCALL TransformVertsAndTangents( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights );
	//virtual void VPCALL TransformVertsAndTangentsFast( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights );
	virtual void VPCALL TracePointCull( byte *cullBits, byte &totalOr, const float radius, const idPlane *planes, const idDrawVert *verts, const int numVerts );
	virtual void VPCALL DecalPointCull( byte *cullBits, const idPlane *planes, const idDrawVert *verts, const int numVerts );
	virtual void VPCALL OverlayPointCull( byte *cullBits, idVec2 *texCoords, const idPlane *planes, const idDrawVert *verts, const int numVerts );
	virtual void VPCALL DeriveTriPlanes( idPlane *planes, const idDrawVert *verts, const int numVerts, const vertIndex_t *indexes, const int numIndexes );
	virtual void VPCALL CalculateFacing( byte *facing, const idPlane *planes, const int numTriangles, const idVec4 &light );
	virtual void VPCALL CalculateCullBits( byte *cullBits, const idDrawVert *verts, const int numVerts, const int frontBits, const idPlane lightPlanes[NUM_LIGHT_PLANES] );
	virtual int  VPCALL CreateShadowCache( idVec4 *vertexCache, const idDrawVert *verts, const int numVerts );
	virtual int  VPCALL ShadowVolume_CountFacing( const byte *facing, const int numFaces );
	virtual int  VPCALL ShadowVolume_CountFacingCull( byte *facing, const int numFaces, const vertIndex_t *indexes, const byte *cull );
	virtual int  VPCALL ShadowVolume_CreateSilTriangles( vertIndex_t *shadowIndexes, const byte *facing, const silEdge_t *silEdges, const int numSilEdges );
	virtual int  VPCALL ShadowVolume_CreateSilTrianglesParallel( vertIndex_t *shadowIndexes, const byte *facing, const silEdge_t *silEdges, const int numSilEdges );
	virtual int  VPCALL ShadowVolume_CreateCapTriangles( vertIndex_t *shadowIndexes, const byte *facing, const vertIndex_t *indexes, const int numIndexes );
	virtual int  VPCALL ShadowVolume_CreateCapTrianglesParallel( vertIndex_t *shadowIndexes, const byte *facing, const vertIndex_t *indexes, const int numIndexes );

#endif
};

#endif /* !__MATH_SIMD_XENON_H__ */
