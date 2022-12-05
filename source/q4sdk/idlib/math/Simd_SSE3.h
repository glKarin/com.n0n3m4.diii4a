
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

	virtual void VPCALL TransformVertsNew( idDrawVert *verts, const int numVerts, idBounds &bounds, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights );
	virtual void VPCALL TransformVertsAndTangents( idDrawVert *verts, const int numVerts, idBounds &bounds, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights );
	virtual void VPCALL TransformVertsAndTangentsFast( idDrawVert *verts, const int numVerts, idBounds &bounds, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights );

#endif
};

#endif /* !__MATH_SIMD_SSE3_H__ */
