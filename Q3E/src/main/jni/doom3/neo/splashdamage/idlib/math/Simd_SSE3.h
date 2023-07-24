// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __MATH_SIMD_SSE3_H__
#define __MATH_SIMD_SSE3_H__

/*
===============================================================================

	SSE3 implementation of idSIMDProcessor

===============================================================================
*/

class idSIMD_SSE3 : public idSIMD_SSE2 {
#ifdef ID_WIN_X86_ASM
public:
	virtual const char * VPCALL GetName( void ) const;

	virtual void VPCALL TransformVerts( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights );
	virtual void VPCALL TransformShadowVerts( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idDrawVert *base, const jointWeight_t *weights, const int numWeights );
	virtual void VPCALL TransformShadowVerts( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idDrawVert *base, const short *weights, const int numWeights );
	virtual void VPCALL TransformShadowVerts( struct shadowCache_s *verts, const int numVerts, const idJointMat *joints, const idDrawVert *base, const short *weights, const int numWeights );
#if !defined(SD_USE_DRAWVERT_SIZE_32)
	virtual void VPCALL TransformVertsAndTangents( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights );
	virtual void VPCALL TransformVertsAndTangentsFast( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights );
#endif

#endif
};

#endif /* !__MATH_SIMD_SSE3_H__ */
