// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __MATH_SIMD_SSE3_H__
#define __MATH_SIMD_SSE3_H__

/*
===============================================================================

	SSE3 implementation of idSIMDProcessor

===============================================================================
*/

class idSIMD_SSE3 : public idSIMD_SSE2 {
#ifdef _WIN32
public:
	virtual const char * VPCALL GetName( void ) const;

	virtual void VPCALL TransformVerts( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idVec4 *weights, const int *index, const int numWeights );

#if NEW_MESH_TRANSFORM
	virtual void VPCALL TransformVertsNew( idDrawVert *verts, const int numVerts, idBounds &bounds, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights );
	virtual void VPCALL TransformVertsAndTangents( idDrawVert *verts, const int numVerts, idBounds &bounds, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights );
	virtual void VPCALL TransformVertsAndTangentsFast( idDrawVert *verts, const int numVerts, idBounds &bounds, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights );
#endif

#endif
};

#endif /* !__MATH_SIMD_SSE3_H__ */
