
#ifndef __MATH_SIMD_GENERIC_H__
#define __MATH_SIMD_GENERIC_H__

/*
===============================================================================

	Generic implementation of idSIMDProcessor

===============================================================================
*/

// RAVEN BEGIN
// jsinger: should not be virtual on xenon otherwise the mispredicted branch far outways the
//          savings we get from doing SIMD in the first place.  Also inheritence is unnecessary
#ifdef _XENON
#define VIRTUAL
class idSIMD_Generic {
#else
#define VIRTUAL virtual
class idSIMD_Generic : public idSIMDProcessor {
#endif
public:
	VIRTUAL const char * VPCALL GetName( void ) const;

	VIRTUAL void VPCALL Add( float * RESTRICT dst,			const float constant,	const float * RESTRICT src,		const int count );
	VIRTUAL void VPCALL Add( float * RESTRICT dst,			const float * RESTRICT src0,		const float * RESTRICT src1,		const int count );
	VIRTUAL void VPCALL Sub( float * RESTRICT dst,			const float constant,	const float * RESTRICT src,		const int count );
	VIRTUAL void VPCALL Sub( float * RESTRICT dst,			const float * RESTRICT src0,		const float * RESTRICT src1,		const int count );
	VIRTUAL void VPCALL Mul( float * RESTRICT dst,			const float constant,	const float * RESTRICT src,		const int count );
	VIRTUAL void VPCALL Mul( float * RESTRICT dst,			const float * RESTRICT src0,		const float * RESTRICT src1,		const int count );
	VIRTUAL void VPCALL Div( float * RESTRICT dst,			const float constant,	const float * RESTRICT src,		const int count );
	VIRTUAL void VPCALL Div( float * RESTRICT dst,			const float * RESTRICT src0,		const float * RESTRICT src1,		const int count );
	VIRTUAL void VPCALL MulAdd( float * RESTRICT dst,			const float constant,	const float * RESTRICT src,		const int count );
	VIRTUAL void VPCALL MulAdd( float * RESTRICT dst,			const float * RESTRICT src0,		const float * RESTRICT src1,		const int count );
	VIRTUAL void VPCALL MulSub( float * RESTRICT dst,			const float constant,	const float * RESTRICT src,		const int count );
	VIRTUAL void VPCALL MulSub( float * RESTRICT dst,			const float * RESTRICT src0,		const float * RESTRICT src1,		const int count );

	VIRTUAL void VPCALL Dot( float * RESTRICT dst,			const idVec3 &constant,	const idVec3 * RESTRICT src,		const int count );
	VIRTUAL void VPCALL Dot( float * RESTRICT dst,			const idVec3 &constant,	const idPlane * RESTRICT src,		const int count );
	VIRTUAL void VPCALL Dot( float * RESTRICT dst,			const idVec3 &constant,	const idDrawVert * RESTRICT src,	const int count );
	VIRTUAL void VPCALL Dot( float * RESTRICT dst,			const idPlane &constant,const idVec3 * RESTRICT src,		const int count );
	VIRTUAL void VPCALL Dot( float * RESTRICT dst,			const idPlane &constant,const idPlane * RESTRICT src,		const int count );
	VIRTUAL void VPCALL Dot( float * RESTRICT dst,			const idPlane &constant,const idDrawVert * RESTRICT src,	const int count );
	VIRTUAL void VPCALL Dot( float * RESTRICT dst,			const idVec3 * RESTRICT src0,		const idVec3 * RESTRICT src1,		const int count );
	VIRTUAL void VPCALL Dot( float &dot,			const float * RESTRICT src1,		const float * RESTRICT src2,		const int count );

	VIRTUAL void VPCALL CmpGT( byte * RESTRICT dst,			const float * RESTRICT src0,		const float constant,	const int count );
	VIRTUAL void VPCALL CmpGT( byte * RESTRICT dst,			const byte bitNum,		const float * RESTRICT src0,		const float constant,	const int count );
	VIRTUAL void VPCALL CmpGE( byte * RESTRICT dst,			const float * RESTRICT src0,		const float constant,	const int count );
	VIRTUAL void VPCALL CmpGE( byte * RESTRICT dst,			const byte bitNum,		const float * RESTRICT src0,		const float constant,	const int count );
	VIRTUAL void VPCALL CmpLT( byte * RESTRICT dst,			const float * RESTRICT src0,		const float constant,	const int count );
	VIRTUAL void VPCALL CmpLT( byte * RESTRICT dst,			const byte bitNum,		const float * RESTRICT src0,		const float constant,	const int count );
	VIRTUAL void VPCALL CmpLE( byte * RESTRICT dst,			const float * RESTRICT src0,		const float constant,	const int count );
	VIRTUAL void VPCALL CmpLE( byte * RESTRICT dst,			const byte bitNum,		const float * RESTRICT src0,		const float constant,	const int count );

	VIRTUAL void VPCALL MinMax( float &min,			float &max,				const float * RESTRICT src,		const int count );
	VIRTUAL	void VPCALL MinMax( idVec2 &min,		idVec2 &max,			const idVec2 * RESTRICT src,		const int count );
	VIRTUAL void VPCALL MinMax( idVec3 &min,		idVec3 &max,			const idVec3 * RESTRICT src,		const int count );
	VIRTUAL	void VPCALL MinMax( idVec3 &min,		idVec3 &max,			const idDrawVert * RESTRICT src,	const int count );
	VIRTUAL	void VPCALL MinMax( idVec3 &min,		idVec3 &max,			const idDrawVert * RESTRICT src,	const int * RESTRICT indexes,		const int count );

	VIRTUAL void VPCALL Clamp( float * RESTRICT dst,			const float * RESTRICT src,		const float min,		const float max,		const int count );
	VIRTUAL void VPCALL ClampMin( float * RESTRICT dst,		const float * RESTRICT src,		const float min,		const int count );
	VIRTUAL void VPCALL ClampMax( float * RESTRICT dst,		const float * RESTRICT src,		const float max,		const int count );

	VIRTUAL void VPCALL Memcpy( void * RESTRICT dst,			const void * RESTRICT src,		const int count );
	VIRTUAL void VPCALL Memset( void * RESTRICT dst,			const int val,			const int count );

	VIRTUAL void VPCALL Zero16( float * RESTRICT dst,			const int count );
	VIRTUAL void VPCALL Negate16( float * RESTRICT dst,		const int count );
	VIRTUAL void VPCALL Copy16( float * RESTRICT dst,			const float * RESTRICT src,		const int count );
	VIRTUAL void VPCALL Add16( float * RESTRICT dst,			const float * RESTRICT src1,		const float * RESTRICT src2,		const int count );
	VIRTUAL void VPCALL Sub16( float * RESTRICT dst,			const float * RESTRICT src1,		const float * RESTRICT src2,		const int count );
	VIRTUAL void VPCALL Mul16( float * RESTRICT dst,			const float * RESTRICT src1,		const float constant,	const int count );
	VIRTUAL void VPCALL AddAssign16( float * RESTRICT dst,	const float * RESTRICT src,		const int count );
	VIRTUAL void VPCALL SubAssign16( float * RESTRICT dst,	const float * RESTRICT src,		const int count );
	VIRTUAL void VPCALL MulAssign16( float * RESTRICT dst,	const float constant,	const int count );

	VIRTUAL void VPCALL MatX_MultiplyVecX( idVecX &dst, const idMatX &mat, const idVecX &vec );
	VIRTUAL void VPCALL MatX_MultiplyAddVecX( idVecX &dst, const idMatX &mat, const idVecX &vec );
	VIRTUAL void VPCALL MatX_MultiplySubVecX( idVecX &dst, const idMatX &mat, const idVecX &vec );
	VIRTUAL void VPCALL MatX_TransposeMultiplyVecX( idVecX &dst, const idMatX &mat, const idVecX &vec );
	VIRTUAL void VPCALL MatX_TransposeMultiplyAddVecX( idVecX &dst, const idMatX &mat, const idVecX &vec );
	VIRTUAL void VPCALL MatX_TransposeMultiplySubVecX( idVecX &dst, const idMatX &mat, const idVecX &vec );
	VIRTUAL void VPCALL MatX_MultiplyMatX( idMatX &dst, const idMatX &m1, const idMatX &m2 );
	VIRTUAL void VPCALL MatX_TransposeMultiplyMatX( idMatX &dst, const idMatX &m1, const idMatX &m2 );
	VIRTUAL void VPCALL MatX_LowerTriangularSolve( const idMatX &L, float * RESTRICT x, const float * RESTRICT b, const int n, int skip = 0 );
	VIRTUAL void VPCALL MatX_LowerTriangularSolveTranspose( const idMatX &L, float * RESTRICT x, const float *b, const int n );
	VIRTUAL bool VPCALL MatX_LDLTFactor( idMatX &mat, idVecX &invDiag, const int n );

	ID_INLINE void VPCALL BlendJoints( idJointQuat * RESTRICT joints, const idJointQuat * RESTRICT blendJoints, const float lerp, const int * RESTRICT index, const int numJoints );
	VIRTUAL void VPCALL ConvertJointQuatsToJointMats( idJointMat * RESTRICT jointMats, const idJointQuat * RESTRICT jointQuats, const int numJoints );
	VIRTUAL void VPCALL ConvertJointMatsToJointQuats( idJointQuat * RESTRICT jointQuats, const idJointMat * RESTRICT jointMats, const int numJoints );
	VIRTUAL void VPCALL TransformJoints( idJointMat * RESTRICT jointMats, const int * RESTRICT parents, const int firstJoint, const int lastJoint );
	VIRTUAL void VPCALL UntransformJoints( idJointMat * RESTRICT jointMats, const int * RESTRICT parents, const int firstJoint, const int lastJoint );
	VIRTUAL void VPCALL MultiplyJoints( idJointMat * RESTRICT result, const idJointMat * RESTRICT joints1, const idJointMat * RESTRICT joints2, const int numJoints );
	VIRTUAL void VPCALL TransformVertsNew( idDrawVert * RESTRICT verts, const int numVerts, idBounds &bounds, const idJointMat * RESTRICT joints, const idVec4 * RESTRICT base, const jointWeight_t * RESTRICT weights, const int numWeights );
	VIRTUAL void VPCALL TransformVertsAndTangents( idDrawVert * RESTRICT verts, const int numVerts, idBounds &bounds, const idJointMat * RESTRICT joints, const idVec4 * RESTRICT base, const jointWeight_t * RESTRICT weights, const int numWeights );
	VIRTUAL void VPCALL TransformVertsAndTangentsFast( idDrawVert * RESTRICT verts, const int numVerts, idBounds &bounds, const idJointMat * RESTRICT joints, const idVec4 * RESTRICT base, const jointWeight_t * RESTRICT weights, const int numWeights );
	VIRTUAL void VPCALL TracePointCull( byte * RESTRICT cullBits, byte &totalOr, const float radius, const idPlane * RESTRICT planes, const idDrawVert * RESTRICT verts, const int numVerts );
	VIRTUAL void VPCALL DecalPointCull( byte * RESTRICT cullBits, const idPlane * RESTRICT planes, const idDrawVert * RESTRICT verts, const int numVerts );
	VIRTUAL void VPCALL OverlayPointCull( byte * RESTRICT cullBits, idVec2 * RESTRICT texCoords, const idPlane * RESTRICT planes, const idDrawVert * RESTRICT verts, const int numVerts );
	VIRTUAL void VPCALL DeriveTriPlanes( idPlane * RESTRICT planes, const idDrawVert * RESTRICT verts, const int numVerts, const int * RESTRICT indexes, const int numIndexes );
	VIRTUAL void VPCALL DeriveTangents( idPlane * RESTRICT planes, idDrawVert * RESTRICT verts, const int numVerts, const int * RESTRICT indexes, const int numIndexes );
	VIRTUAL void VPCALL DeriveUnsmoothedTangents( idDrawVert * RESTRICT verts, const dominantTri_s * RESTRICT dominantTris, const int numVerts );
	VIRTUAL void VPCALL NormalizeTangents( idDrawVert * RESTRICT verts, const int numVerts );
	VIRTUAL void VPCALL CreateTextureSpaceLightVectors( idVec3 * RESTRICT lightVectors, const idVec3 &lightOrigin, const idDrawVert * RESTRICT verts, const int numVerts, const int * RESTRICT indexes, const int numIndexes );
	VIRTUAL void VPCALL CreateSpecularTextureCoords( idVec4 * RESTRICT texCoords, const idVec3 &lightOrigin, const idVec3 &viewOrigin, const idDrawVert * RESTRICT verts, const int numVerts, const int * RESTRICT indexes, const int numIndexes );
	VIRTUAL int  VPCALL CreateShadowCache( idVec4 * RESTRICT vertexCache, int * RESTRICT vertRemap, const idVec3 &lightOrigin, const idDrawVert * RESTRICT verts, const int numVerts );
	VIRTUAL int  VPCALL CreateVertexProgramShadowCache( idVec4 * RESTRICT vertexCache, const idDrawVert * RESTRICT verts, const int numVerts );
	VIRTUAL int  VPCALL ShadowVolume_CountFacing( const byte * RESTRICT facing, const int numFaces );
	VIRTUAL int  VPCALL ShadowVolume_CountFacingCull( byte * RESTRICT facing, const int numFaces, const int * RESTRICT indexes, const byte * RESTRICT cull );
	VIRTUAL int  VPCALL ShadowVolume_CreateSilTriangles( int * RESTRICT shadowIndexes, const byte * RESTRICT facing, const silEdge_s * RESTRICT silEdges, const int numSilEdges );
	VIRTUAL int  VPCALL ShadowVolume_CreateCapTriangles( int * RESTRICT shadowIndexes, const byte * RESTRICT facing, const int * RESTRICT indexes, const int numIndexes );

	VIRTUAL void VPCALL UpSamplePCMTo44kHz( float * RESTRICT dest, const short * RESTRICT pcm, const int numSamples, const int kHz, const int numChannels );
	VIRTUAL void VPCALL UpSampleOGGTo44kHz( float * RESTRICT dest, const float * const * RESTRICT ogg, const int numSamples, const int kHz, const int numChannels );
	VIRTUAL void VPCALL MixSoundTwoSpeakerMono( float * RESTRICT mixBuffer, const float * RESTRICT samples, const int numSamples, const float lastV[2], const float currentV[2] );
	VIRTUAL void VPCALL MixSoundTwoSpeakerMonoSimple( float * RESTRICT mixBuffer, const float * RESTRICT samples, const int numSamples );
	VIRTUAL void VPCALL MixSoundTwoSpeakerStereo( float * RESTRICT mixBuffer, const float * RESTRICT samples, const int numSamples, const float lastV[2], const float currentV[2] );
	VIRTUAL void VPCALL MixSoundSixSpeakerMono( float * RESTRICT mixBuffer, const float * RESTRICT samples, const int numSamples, const float lastV[6], const float currentV[6] );
	VIRTUAL void VPCALL MixSoundSixSpeakerMonoSimple( float * RESTRICT mixBuffer, const float * RESTRICT samples, const int numSamples );
	VIRTUAL void VPCALL MixSoundSixSpeakerStereo( float * RESTRICT mixBuffer, const float * RESTRICT samples, const int numSamples, const float lastV[6], const float currentV[6] );
	VIRTUAL void VPCALL MixedSoundToSamples( short * RESTRICT samples, const float * RESTRICT mixBuffer, const int numSamples );

	// rvSilTraceVertT operations
// dluetscher: added support for operations on idSilTraceVerts and idJointMats
#ifdef _MD5R_SUPPORT
	VIRTUAL void VPCALL JointMat_MultiplyMats( float * RESTRICT destMats, const idJointMat * RESTRICT src1Mats, const idJointMat * RESTRICT src2Mats, int * RESTRICT transformPalette, int transformCount );
	VIRTUAL void VPCALL TransformVertsMinMax4Bone( rvSilTraceVertT * RESTRICT silTraceVertOutputData, idVec3 &min, idVec3 &max, byte * RESTRICT vertexInputData, int vertStride, int numVerts, float * RESTRICT skinToModelTransforms ); // transforms an array of index-weighted vertices into an array of idSilTraceVerts, while simulatenously calculating the bounds
	VIRTUAL void VPCALL TransformVertsMinMax1Bone( rvSilTraceVertT * RESTRICT silTraceVertOutputData, idVec3 &min, idVec3 &max, byte * RESTRICT vertexInputData, int vertStride, int numVerts, float * RESTRICT skinToModelTransforms ); // transforms an array of index-weighted vertices into an array of idSilTraceVerts, while simulatenously calculating the bounds
	VIRTUAL void VPCALL Dot( float * RESTRICT dst, const idVec3 &constant, const rvSilTraceVertT * RESTRICT src,	const int count );
	VIRTUAL void VPCALL Dot( float * RESTRICT dst, const idPlane &constant, const rvSilTraceVertT * RESTRICT src, const int count );
	VIRTUAL void VPCALL TracePointCull( byte * RESTRICT cullBits, byte &totalOr, const float radius, const idPlane * RESTRICT planes, const rvSilTraceVertT * RESTRICT verts, const int numVerts );
	VIRTUAL void VPCALL DecalPointCull( byte * RESTRICT cullBits, const idPlane * RESTRICT planes, const rvSilTraceVertT * RESTRICT verts, const int numVerts );
	VIRTUAL void VPCALL OverlayPointCull( byte * RESTRICT cullBits, idVec2 * RESTRICT texCoords, const idPlane * RESTRICT planes, const rvSilTraceVertT * RESTRICT verts, const int numVerts );
	VIRTUAL void VPCALL DeriveTriPlanes( idPlane * RESTRICT planes, const rvSilTraceVertT * RESTRICT verts, const int numVerts, const int * RESTRICT indexes, const int numIndexes );
	VIRTUAL void VPCALL DeriveTriPlanes( idPlane * RESTRICT planes, const rvSilTraceVertT * RESTRICT verts, const int numVerts, const unsigned short * RESTRICT indexes, const int numIndexes );
	VIRTUAL	void VPCALL MinMax( idVec3 &min, idVec3 &max, const rvSilTraceVertT * RESTRICT src,	const int count );
	VIRTUAL	void VPCALL MinMax( idVec3 &min, idVec3 &max, const rvSilTraceVertT * RESTRICT src, const int * RESTRICT indexes, const int count );
#endif
};

// jsinger: inlined during profiling with Microsoft.  This shows up pretty high on our profiles
//          inlining reduced the many call overhead and every little bit helps on xenon
void VPCALL idSIMD_Generic::BlendJoints( idJointQuat * RESTRICT joints, const idJointQuat * RESTRICT blendJoints, const float lerp, const int * RESTRICT index, const int count ) {
#define UNROLL4(Y) { int _IX, _NM = count&0xfffffffc; for (_IX=0;_IX<_NM;_IX+=4){Y(_IX+0);Y(_IX+1);Y(_IX+2);Y(_IX+3);}for(;_IX<count;_IX++){Y(_IX);}}
#define OPER(X) { int j = index[(X)]; joints[j].q.Slerp( joints[j].q, blendJoints[j].q, lerp ); joints[j].t.Lerp( joints[j].t, blendJoints[j].t, lerp ); }
	UNROLL4(OPER)
#undef OPER
#undef UNROLL4

}

// RAVEN END

#endif /* !__MATH_SIMD_GENERIC_H__ */
