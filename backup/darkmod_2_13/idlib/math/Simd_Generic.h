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

#ifndef __MATH_SIMD_GENERIC_H__
#define __MATH_SIMD_GENERIC_H__

/*
===============================================================================

	Generic implementation of idSIMDProcessor

===============================================================================
*/

class idSIMD_Generic : public idSIMDProcessor {
protected:
	idStr name;
public:
	idSIMD_Generic();
	virtual const char * GetName( void ) const override;

	virtual void Add( float *dst,			const float constant,	const float *src,		const int count ) override;
	virtual void Add( float *dst,			const float *src0,		const float *src1,		const int count ) override;
	virtual void Sub( float *dst,			const float constant,	const float *src,		const int count ) override;
	virtual void Sub( float *dst,			const float *src0,		const float *src1,		const int count ) override;
	virtual void Mul( float *dst,			const float constant,	const float *src,		const int count ) override;
	virtual void Mul( float *dst,			const float *src0,		const float *src1,		const int count ) override;
	virtual void Div( float *dst,			const float constant,	const float *src,		const int count ) override;
	virtual void Div( float *dst,			const float *src0,		const float *src1,		const int count ) override;
	virtual void MulAdd( float *dst,			const float constant,	const float *src,		const int count ) override;
	virtual void MulAdd( float *dst,			const float *src0,		const float *src1,		const int count ) override;
	virtual void MulSub( float *dst,			const float constant,	const float *src,		const int count ) override;
	virtual void MulSub( float *dst,			const float *src0,		const float *src1,		const int count ) override;

	virtual void Dot( float *dst,			const idVec3 &constant,	const idVec3 *src,		const int count ) override;
	virtual void Dot( float *dst,			const idVec3 &constant,	const idPlane *src,		const int count ) override;
	virtual void Dot( float *dst,			const idVec3 &constant,	const idDrawVert *src,	const int count ) override;
	virtual void Dot( float *dst,			const idPlane &constant,const idVec3 *src,		const int count ) override;
	virtual void Dot( float *dst,			const idPlane &constant,const idPlane *src,		const int count ) override;
	virtual void Dot( float *dst,			const idPlane &constant,const idDrawVert *src,	const int count ) override;
	virtual void Dot( float *dst,			const idVec3 *src0,		const idVec3 *src1,		const int count ) override;
	virtual void Dot( float &dot,			const float *src1,		const float *src2,		const int count ) override;

	virtual void CmpGT( byte *dst,			const float *src0,		const float constant,	const int count ) override;
	virtual void CmpGT( byte *dst,			const byte bitNum,		const float *src0,		const float constant,	const int count ) override;
	virtual void CmpGE( byte *dst,			const float *src0,		const float constant,	const int count ) override;
	virtual void CmpGE( byte *dst,			const byte bitNum,		const float *src0,		const float constant,	const int count ) override;
	virtual void CmpLT( byte *dst,			const float *src0,		const float constant,	const int count ) override;
	virtual void CmpLT( byte *dst,			const byte bitNum,		const float *src0,		const float constant,	const int count ) override;
	virtual void CmpLE( byte *dst,			const float *src0,		const float constant,	const int count ) override;
	virtual void CmpLE( byte *dst,			const byte bitNum,		const float *src0,		const float constant,	const int count ) override;

	virtual void MinMax( float &min,			float &max,				const float *src,		const int count ) override;
	virtual	void MinMax( idVec2 &min,		idVec2 &max,			const idVec2 *src,		const int count ) override;
	virtual void MinMax( idVec3 &min,		idVec3 &max,			const idVec3 *src,		const int count ) override;
	virtual	void MinMax( idVec3 &min,		idVec3 &max,			const idDrawVert *src,	const int count ) override;
	virtual	void MinMax( idVec3 &min,		idVec3 &max,			const idDrawVert *src,	const int *indexes,		const int count ) override;

	virtual void Clamp( float *dst,			const float *src,		const float min,		const float max,		const int count ) override;
	virtual void ClampMin( float *dst,		const float *src,		const float min,		const int count ) override;
	virtual void ClampMax( float *dst,		const float *src,		const float max,		const int count ) override;

	virtual void Memcpy( void *dst,			const void *src,		const int count ) override;
	virtual void Memset( void *dst,			const int val,			const int count ) override;
	virtual void MemcpyNT( void *dst,		const void *src,		const int count ) override;

	virtual void Zero16( float *dst,			const int count ) override;
	virtual void Negate16( float *dst,		const int count ) override;
	virtual void Copy16( float *dst,			const float *src,		const int count ) override;
	virtual void Add16( float *dst,			const float *src1,		const float *src2,		const int count ) override;
	virtual void Sub16( float *dst,			const float *src1,		const float *src2,		const int count ) override;
	virtual void Mul16( float *dst,			const float *src1,		const float constant,	const int count ) override;
	virtual void AddAssign16( float *dst,	const float *src,		const int count ) override;
	virtual void SubAssign16( float *dst,	const float *src,		const int count ) override;
	virtual void MulAssign16( float *dst,	const float constant,	const int count ) override;

	virtual void MatX_MultiplyVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) override;
	virtual void MatX_MultiplyAddVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) override;
	virtual void MatX_MultiplySubVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) override;
	virtual void MatX_TransposeMultiplyVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) override;
	virtual void MatX_TransposeMultiplyAddVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) override;
	virtual void MatX_TransposeMultiplySubVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) override;
	virtual void MatX_MultiplyMatX( idMatX &dst, const idMatX &m1, const idMatX &m2 ) override;
	virtual void MatX_TransposeMultiplyMatX( idMatX &dst, const idMatX &m1, const idMatX &m2 ) override;
	virtual void MatX_LowerTriangularSolve( const idMatX &L, float *x, const float *b, const int n, int skip = 0 ) override;
	virtual void MatX_LowerTriangularSolveTranspose( const idMatX &L, float *x, const float *b, const int n ) override;
	virtual bool MatX_LDLTFactor( idMatX &mat, idVecX &invDiag, const int n ) override;

	virtual void BlendJoints( idJointQuat *joints, const idJointQuat *blendJoints, const float lerp, const int *index, const int numJoints ) override;
	virtual void ConvertJointQuatsToJointMats( idJointMat *jointMats, const idJointQuat *jointQuats, const int numJoints ) override;
	virtual void ConvertJointMatsToJointQuats( idJointQuat *jointQuats, const idJointMat *jointMats, const int numJoints ) override;
	virtual void TransformJoints( idJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint ) override;
	virtual void UntransformJoints( idJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint ) override;
	virtual void TransformVerts( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idVec4 *weights, const int *index, const int numWeights ) override;
	virtual void ComputeBoundsFromJointBounds( idBounds &totalBounds, int numJoints, const idJointMat *joints, const idBounds *jointBounds ) override;

	virtual void TracePointCull( byte *cullBits, byte &totalOr, const float radius, const idPlane *planes, const idDrawVert *verts, const int numVerts ) override;
	virtual void DecalPointCull( byte *cullBits, const idPlane *planes, const idDrawVert *verts, const int numVerts ) override;
	virtual void OverlayPointCull( byte *cullBits, idVec2 *texCoords, const idPlane *planes, const idDrawVert *verts, const int numVerts ) override;
	virtual void CalcTriFacing( const idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes, const idVec3 &lightOrigin, byte *facing ) override;
	virtual void DeriveTriPlanes( idPlane *planes, const idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes ) override;
	virtual void DeriveTangents( idPlane *planes, idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes ) override;
	virtual void DeriveUnsmoothedTangents( idDrawVert *verts, const dominantTri_s *dominantTris, const int numVerts ) override;
	virtual void NormalizeTangents( idDrawVert *verts, const int numVerts ) override;
	virtual int  CreateShadowCache( idVec4 *vertexCache, int *vertRemap, const idVec3 &lightOrigin, const idDrawVert *verts, const int numVerts ) override;
	virtual int  CreateVertexProgramShadowCache( idVec4 *vertexCache, const idDrawVert *verts, const int numVerts ) override;
	virtual void CullByFrustum( idDrawVert *verts, const int numVerts, const idPlane frustum[6], byte *pointCull, float epsilon ) override;
	virtual void CullByFrustum2( idDrawVert *verts, const int numVerts, const idPlane frustum[6], unsigned short *pointCull, float epsilon ) override;
	virtual void CullTrisByFrustum( idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes, const idPlane frustum[6], byte *triCull, float epsilon ) override;

	virtual void GenerateMipMap2x2( const byte *srcPtr, int srcStride, int halfWidth, int halfHeight, byte *dstPtr, int dstStride ) override;
	virtual bool ConvertRowToRGBA8( const byte *srcPtr, int width, int bitsPerPixel, bool bgr, byte *dstPtr ) override;
	virtual void CompressRGTCFromRGBA8( const byte *srcPtr, int width, int height, int stride, byte *dstPtr ) override;
	virtual void CompressDXT1FromRGBA8( const byte *srcPtr, int width, int height, int stride, byte *dstPtr ) override;
	virtual void CompressDXT3FromRGBA8( const byte *srcPtr, int width, int height, int stride, byte *dstPtr ) override;
	virtual void CompressDXT5FromRGBA8( const byte *srcPtr, int width, int height, int stride, byte *dstPtr ) override;
	virtual void DecompressRGBA8FromDXT1( const byte *srcPtr, int width, int height, byte *dstPtr, int stride, bool allowTransparency ) override;
	virtual void DecompressRGBA8FromDXT3( const byte *srcPtr, int width, int height, byte *dstPtr, int stride ) override;
	virtual void DecompressRGBA8FromDXT5( const byte *srcPtr, int width, int height, byte *dstPtr, int stride ) override;
	virtual void DecompressRGBA8FromRGTC( const byte *srcPtr, int width, int height, byte *dstPtr, int stride ) override;

	virtual void UpSamplePCMTo44kHz( float *dest, const short *pcm, const int numSamples, const int kHz, const int numChannels ) override;
	virtual void UpSampleOGGTo44kHz( float *dest, const float * const *ogg, const int numSamples, const int kHz, const int numChannels ) override;
	virtual void MixSoundTwoSpeakerMono( float *mixBuffer, const float *samples, const int numSamples, const float lastV[2], const float currentV[2] ) override;
	virtual void MixSoundTwoSpeakerStereo( float *mixBuffer, const float *samples, const int numSamples, const float lastV[2], const float currentV[2] ) override;
	virtual void MixSoundSixSpeakerMono( float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6] ) override;
	virtual void MixSoundSixSpeakerStereo( float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6] ) override;
	virtual void MixedSoundToSamples( short *samples, const float *mixBuffer, const int numSamples ) override;
};

#endif /* !__MATH_SIMD_GENERIC_H__ */
