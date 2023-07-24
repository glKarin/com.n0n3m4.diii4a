// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __MATH_SIMD_SSE_H__
#define __MATH_SIMD_SSE_H__

/*
===============================================================================

	SSE implementation of idSIMDProcessor

===============================================================================
*/

class idSIMD_SSE : public idSIMD_MMX {
#ifdef ID_WIN_X86_ASM
public:
	virtual const char * VPCALL GetName( void ) const;

	virtual void VPCALL Add( float *dst,			const float constant,	const float *src,		const int count );
	virtual void VPCALL Add( float *dst,			const float *src0,		const float *src1,		const int count );
	virtual void VPCALL Sub( float *dst,			const float constant,	const float *src,		const int count );
	virtual void VPCALL Sub( float *dst,			const float *src0,		const float *src1,		const int count );
	virtual void VPCALL Mul( float *dst,			const float constant,	const float *src,		const int count );
	virtual void VPCALL Mul( float *dst,			const float *src0,		const float *src1,		const int count );
	virtual void VPCALL Div( float *dst,			const float constant,	const float *src,		const int count );
	virtual void VPCALL Div( float *dst,			const float *src0,		const float *src1,		const int count );
	virtual void VPCALL MulAdd( float *dst,			const float constant,	const float *src,		const int count );
	virtual void VPCALL MulAdd( float *dst,			const float *src0,		const float *src1,		const int count );
	virtual void VPCALL MulSub( float *dst,			const float constant,	const float *src,		const int count );
	virtual void VPCALL MulSub( float *dst,			const float *src0,		const float *src1,		const int count );

	virtual void VPCALL Dot( float *dst,			const idVec3 &constant,	const idVec3 *src,		const int count );
	virtual void VPCALL Dot( float *dst,			const idVec3 &constant,	const idPlane *src,		const int count );
	virtual void VPCALL Dot( float *dst,			const idVec3 &constant,	const idDrawVert *src,	const int count );
	virtual void VPCALL Dot( float *dst,			const idPlane &constant,const idVec3 *src,		const int count );
	virtual void VPCALL Dot( float *dst,			const idPlane &constant,const idPlane *src,		const int count );
	virtual void VPCALL Dot( float *dst,			const idPlane &constant,const idDrawVert *src,	const int count );
	virtual void VPCALL Dot( float *dst,			const idVec3 *src0,		const idVec3 *src1,		const int count );
	virtual void VPCALL Dot( float &dot,			const float *src1,		const float *src2,		const int count );

	virtual void VPCALL CmpGT( byte *dst,			const float *src0,		const float constant,	const int count );
	virtual void VPCALL CmpGT( byte *dst,			const byte bitNum,		const float *src0,		const float constant,	const int count );
	virtual void VPCALL CmpGE( byte *dst,			const float *src0,		const float constant,	const int count );
	virtual void VPCALL CmpGE( byte *dst,			const byte bitNum,		const float *src0,		const float constant,	const int count );
	virtual void VPCALL CmpLT( byte *dst,			const float *src0,		const float constant,	const int count );
	virtual void VPCALL SetCmpLT( byte *dst,			const byte bitNum,		const float *src0,		const float constant,	const int count );
	virtual void VPCALL CmpLT( byte *dst,			const byte bitNum,		const float *src0,		const float constant,	const int count );
	virtual void VPCALL CmpLE( byte *dst,			const float *src0,		const float constant,	const int count );
	virtual void VPCALL CmpLE( byte *dst,			const byte bitNum,		const float *src0,		const float constant,	const int count );

	virtual void VPCALL MinMax( float &min,			float &max,				const float *src,		const int count );
	virtual	void VPCALL MinMax( idVec2 &min,		idVec2 &max,			const idVec2 *src,		const int count );
	virtual void VPCALL MinMax( idVec3 &min,		idVec3 &max,			const idVec3 *src,		const int count );
	virtual	void VPCALL MinMax( idVec3 &min,		idVec3 &max,			const idDrawVert *src,	const int count );
	virtual	void VPCALL MinMax( idVec3 &min,		idVec3 &max,			const idDrawVert *src,	const vertIndex_t *indexes,		const int count );
	virtual	void VPCALL MinMax( idVec3 &min,		idVec3 &max,			const struct shadowCache_s *src,	const int count );

	virtual void VPCALL Clamp( float *dst,			const float *src,		const float min,		const float max,		const int count );
	virtual void VPCALL ClampMin( float *dst,		const float *src,		const float min,		const int count );
	virtual void VPCALL ClampMax( float *dst,		const float *src,		const float max,		const int count );

	virtual void VPCALL Zero16( float *dst,			const int count );
	virtual void VPCALL Negate16( float *dst,		const int count );
	virtual void VPCALL Copy16( float *dst,			const float *src,		const int count );
	virtual void VPCALL Add16( float *dst,			const float *src1,		const float *src2,		const int count );
	virtual void VPCALL Sub16( float *dst,			const float *src1,		const float *src2,		const int count );
	virtual void VPCALL Mul16( float *dst,			const float *src1,		const float constant,	const int count );
	virtual void VPCALL AddAssign16( float *dst,	const float *src,		const int count );
	virtual void VPCALL SubAssign16( float *dst,	const float *src,		const int count );
	virtual void VPCALL MulAssign16( float *dst,	const float constant,	const int count );

	virtual void VPCALL MatX_MultiplyVecX( idVecX &dst, const idMatX &mat, const idVecX &vec );
	virtual void VPCALL MatX_MultiplyAddVecX( idVecX &dst, const idMatX &mat, const idVecX &vec );
	virtual void VPCALL MatX_MultiplySubVecX( idVecX &dst, const idMatX &mat, const idVecX &vec );
	virtual void VPCALL MatX_TransposeMultiplyVecX( idVecX &dst, const idMatX &mat, const idVecX &vec );
	virtual void VPCALL MatX_TransposeMultiplyAddVecX( idVecX &dst, const idMatX &mat, const idVecX &vec );
	virtual void VPCALL MatX_TransposeMultiplySubVecX( idVecX &dst, const idMatX &mat, const idVecX &vec );
	virtual void VPCALL MatX_MultiplyMatX( idMatX &dst, const idMatX &m1, const idMatX &m2 );
	virtual void VPCALL MatX_TransposeMultiplyMatX( idMatX &dst, const idMatX &m1, const idMatX &m2 );
	virtual void VPCALL MatX_LowerTriangularSolve( const idMatX &L, float *x, const float *b, const int n, int skip = 0 );
	virtual void VPCALL MatX_LowerTriangularSolveTranspose( const idMatX &L, float *x, const float *b, const int n );
	virtual void VPCALL MatX_UpperTriangularSolve( const idMatX &U, float *x, const float *b, const int n );
	virtual void VPCALL MatX_UpperTriangularSolveTranspose( const idMatX &U, float *x, const float *b, const int n );
	virtual bool VPCALL MatX_LU_Factor( idMatX &mat, idVecX &invDiag, const int n );
	virtual bool VPCALL MatX_LDLT_Factor( idMatX &mat, idVecX &invDiag, const int n );

	virtual void VPCALL DecompressJoints( idJointQuat *joints, const idCompressedJointQuat *compressedJoints, const int *index, const int numJoints );
	virtual void VPCALL BlendJoints( idJointQuat *joints, const idJointQuat *blendJoints, const float lerp, const int *index, const int numJoints );
	virtual void VPCALL BlendJointsFast( idJointQuat *joints, const idJointQuat *blendJoints, const float lerp, const int *index, const int numJoints );
	virtual void VPCALL ConvertJointQuatsToJointMats( idJointMat *jointMats, const idJointQuat *jointQuats, const int numJoints );
	virtual void VPCALL ConvertJointMatsToJointQuats( idJointQuat *jointQuats, const idJointMat *jointMats, const int numJoints );
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
#if SD_SUPPORT_UNSMOOTHEDTANGENTS
	virtual void VPCALL DeriveUnsmoothedTangents( idDrawVert *verts, const dominantTri_s *dominantTris, const int numVerts );
#endif // SD_SUPPORT_UNSMOOTHEDTANGENTS
	virtual void VPCALL TracePointCull( byte *cullBits, byte &totalOr, const float radius, const idPlane *planes, const idDrawVert *verts, const int numVerts );
	virtual void VPCALL TracePointCullShadowVerts( byte *cullBits, byte &totalOr, const float radius, const idPlane *planes, const struct shadowCache_s *verts, const int numVerts );
	virtual void VPCALL DecalPointCull( byte *cullBits, const idPlane *planes, const idDrawVert *verts, const int numVerts );
	virtual void VPCALL DecalPointCull( byte *cullBits, const idPlane *planes, const idDrawVert *verts, const int numVerts, int *indexes, int numIndexes );
	virtual void VPCALL DecalPointCull( byte *cullBits, const idPlane *planes, const idDrawVert *verts, const int numVerts, unsigned short *indexes, int numIndexes );
	virtual void VPCALL OverlayPointCull( byte *cullBits, idVec2 *texCoords, const idPlane *planes, const idDrawVert *verts, const int numVerts );
	virtual void VPCALL OverlayPointCull( byte *cullBits, idVec2 *texCoords, const idPlane *planes, const struct shadowCache_s *verts, const int numVerts );
	virtual void VPCALL DeriveTriPlanes( idPlane *planes, const idDrawVert *verts, const int numVerts, const vertIndex_t *indexes, const int numIndexes );
	virtual void VPCALL DeriveTriPlanes( idPlane *planes, const struct shadowCache_s *verts, const int numVerts, const vertIndex_t *indexes, const int numIndexes );
	virtual void VPCALL CalculateFacing( byte *facing, const idPlane *planes, const int numTriangles, const idVec4 &light );
	virtual void VPCALL CalculateCullBits( byte *cullBits, const idDrawVert *verts, const int numVerts, const int frontBits, const idPlane lightPlanes[NUM_LIGHT_PLANES] );
	virtual int  VPCALL CreateShadowCache( idVec4 *vertexCache, const idDrawVert *verts, const int numVerts );
	virtual int  VPCALL CreateShadowCache( idVec4 *vertexCache, const struct shadowCache_s *verts, const int numVerts );

#if 0
#if defined ( GL_INDEX_SHORT )
	virtual int  VPCALL ShadowVolume_CreateSilTriangles( vertIndex_t *shadowIndexes, const byte *facing, const silEdge_t *silEdges, const int numSilEdges );
	//virtual int  VPCALL ShadowVolume_CreateCapTriangles( vertIndex_t *shadowIndexes, const byte *facing, const vertIndex_t *indexes, const int numIndexes );
#endif
#endif

	virtual void VPCALL UpSamplePCMTo44kHz( float *dest, const short *pcm, const int numSamples, const int kHz, const int numChannels );
	virtual void VPCALL UpSampleOGGTo44kHz( float *dest, const float * const *ogg, const int numSamples, const int kHz, const int numChannels );
	virtual void VPCALL MixSoundTwoSpeakerMono( float *mixBuffer, const float *samples, const int numSamples, const float lastV[2], const float currentV[2] );
	virtual void VPCALL MixSoundTwoSpeakerStereo( float *mixBuffer, const float *samples, const int numSamples, const float lastV[2], const float currentV[2] );
	virtual void VPCALL MixSoundFourSpeakerMono( float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6] );
	virtual void VPCALL MixSoundFourSpeakerStereo( float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6] );
	virtual void VPCALL MixSoundSixSpeakerMono( float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6] );
	virtual void VPCALL MixSoundSixSpeakerStereo( float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6] );
	virtual void VPCALL MixSoundEightSpeakerMono( float *mixBuffer, const float *samples, const int numSamples, const float lastV[8], const float currentV[8] );
	virtual void VPCALL MixSoundEightSpeakerStereo( float *mixBuffer, const float *samples, const int numSamples, const float lastV[8], const float currentV[8] );
	virtual void VPCALL MixedSoundToSamples( short *samples, const float *mixBuffer, const int numSamples );

#endif
};

#endif /* !__MATH_SIMD_SSE_H__ */
