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

#ifndef __MATH_SIMD_H__
#define __MATH_SIMD_H__

/*
===============================================================================

	Single Instruction Multiple Data (SIMD)

	For optimal use data should be aligned on a 16 byte boundary.
	All idSIMDProcessor routines are thread safe.

===============================================================================
*/

class idSIMDProcessor;

class idSIMD {
public:
	static void			Init( void );
	static void			InitProcessor( const char *module, const char *forceImpl = nullptr );
	static idSIMDProcessor *CreateProcessor( const char *forceImpl );	// stgatilov: global state not changed
	static void			Shutdown( void );
	static void			Test_f( const class idCmdArgs &args );
};

//stgatilov: when we should compile SSE/AVX processors at all?
//  a) when we compile on x86 (32-bit or 64-bit)
//  b) when compile-time SSE is enabled (e.g. Elbrus compiler cross-compiles SSE intrinsics)
#if defined(_MSC_VER) || (defined(__i386__) || defined(__x86_64__)) || (defined(__SSE__) || defined (__SSE2__))
#define ENABLE_SSE_PROCESSORS
#endif

/*
===============================================================================

	virtual base class for different SIMD processors

===============================================================================
*/

class idVec2;
class idVec3;
class idVec4;
class idVec5;
class idVec6;
class idVecX;
class idMat2;
class idMat3;
class idMat4;
class idMat5;
class idMat6;
class idMatX;
class idPlane;
class idDrawVert;
class idJointQuat;
class idJointMat;
class idBounds;
struct dominantTri_s;

const int MIXBUFFER_SAMPLES = 4096;

typedef enum {
	SPEAKER_LEFT = 0,
	SPEAKER_RIGHT,
	SPEAKER_CENTER,
	SPEAKER_LFE,
	SPEAKER_BACKLEFT,
	SPEAKER_BACKRIGHT
} speakerLabel;


class idSIMDProcessor {
public:
	virtual							~idSIMDProcessor();

	cpuid_t							cpuid = CPUID_NONE;

	virtual const char * GetName( void ) const = 0;

	virtual void Add( float *dst,			const float constant,	const float *src,		const int count ) = 0;
	virtual void Add( float *dst,			const float *src0,		const float *src1,		const int count ) = 0;
	virtual void Sub( float *dst,			const float constant,	const float *src,		const int count ) = 0;
	virtual void Sub( float *dst,			const float *src0,		const float *src1,		const int count ) = 0;
	virtual void Mul( float *dst,			const float constant,	const float *src,		const int count ) = 0;
	virtual void Mul( float *dst,			const float *src0,		const float *src1,		const int count ) = 0;
	virtual void Div( float *dst,			const float constant,	const float *src,		const int count ) = 0;
	virtual void Div( float *dst,			const float *src0,		const float *src1,		const int count ) = 0;
	virtual void MulAdd( float *dst,			const float constant,	const float *src,		const int count ) = 0;
	virtual void MulAdd( float *dst,			const float *src0,		const float *src1,		const int count ) = 0;
	virtual void MulSub( float *dst,			const float constant,	const float *src,		const int count ) = 0;
	virtual void MulSub( float *dst,			const float *src0,		const float *src1,		const int count ) = 0;

	virtual	void Dot( float *dst,			const idVec3 &constant,	const idVec3 *src,		const int count ) = 0;
	virtual	void Dot( float *dst,			const idVec3 &constant,	const idPlane *src,		const int count ) = 0;
	virtual void Dot( float *dst,			const idVec3 &constant,	const idDrawVert *src,	const int count ) = 0;
	virtual	void Dot( float *dst,			const idPlane &constant,const idVec3 *src,		const int count ) = 0;
	virtual	void Dot( float *dst,			const idPlane &constant,const idPlane *src,		const int count ) = 0;
	virtual void Dot( float *dst,			const idPlane &constant,const idDrawVert *src,	const int count ) = 0;
	virtual	void Dot( float *dst,			const idVec3 *src0,		const idVec3 *src1,		const int count ) = 0;
	virtual void Dot( float &dot,			const float *src1,		const float *src2,		const int count ) = 0;

	virtual	void CmpGT( byte *dst,			const float *src0,		const float constant,	const int count ) = 0;
	virtual	void CmpGT( byte *dst,			const byte bitNum,		const float *src0,		const float constant,	const int count ) = 0;
	virtual	void CmpGE( byte *dst,			const float *src0,		const float constant,	const int count ) = 0;
	virtual	void CmpGE( byte *dst,			const byte bitNum,		const float *src0,		const float constant,	const int count ) = 0;
	virtual	void CmpLT( byte *dst,			const float *src0,		const float constant,	const int count ) = 0;
	virtual	void CmpLT( byte *dst,			const byte bitNum,		const float *src0,		const float constant,	const int count ) = 0;
	virtual	void CmpLE( byte *dst,			const float *src0,		const float constant,	const int count ) = 0;
	virtual	void CmpLE( byte *dst,			const byte bitNum,		const float *src0,		const float constant,	const int count ) = 0;

	virtual	void MinMax( float &min,			float &max,				const float *src,		const int count ) = 0;
	virtual	void MinMax( idVec2 &min,		idVec2 &max,			const idVec2 *src,		const int count ) = 0;
	virtual	void MinMax( idVec3 &min,		idVec3 &max,			const idVec3 *src,		const int count ) = 0;
	virtual	void MinMax( idVec3 &min,		idVec3 &max,			const idDrawVert *src,	const int count ) = 0;
	virtual	void MinMax( idVec3 &min,		idVec3 &max,			const idDrawVert *src,	const int *indexes,		const int count ) = 0;

	virtual	void Clamp( float *dst,			const float *src,		const float min,		const float max,		const int count ) = 0;
	virtual	void ClampMin( float *dst,		const float *src,		const float min,		const int count ) = 0;
	virtual	void ClampMax( float *dst,		const float *src,		const float max,		const int count ) = 0;

	// note: better call ordinary memcpy, since it can inline calls for small and fixed count (e.g. 64 bytes)
	virtual void Memcpy( void *dst,			const void *src,		const int count ) = 0;
	// note: better call ordinary memset, since it can inline calls for small and fixed count (e.g. 64 bytes)
	virtual void Memset( void *dst,			const int val,			const int count ) = 0;
	// uses nontemporal writes: only use this for writing to GPU-mapped memory!
	virtual void MemcpyNT( void *dst,		const void *src,		const int count ) = 0;

	// these assume 16 byte aligned and 16 byte padded memory
	virtual void Zero16( float *dst,			const int count ) = 0;
	virtual void Negate16( float *dst,		const int count ) = 0;
	virtual void Copy16( float *dst,			const float *src,		const int count ) = 0;
	virtual void Add16( float *dst,			const float *src1,		const float *src2,		const int count ) = 0;
	virtual void Sub16( float *dst,			const float *src1,		const float *src2,		const int count ) = 0;
	virtual void Mul16( float *dst,			const float *src1,		const float constant,	const int count ) = 0;
	virtual void AddAssign16( float *dst,	const float *src,		const int count ) = 0;
	virtual void SubAssign16( float *dst,	const float *src,		const int count ) = 0;
	virtual void MulAssign16( float *dst,	const float constant,	const int count ) = 0;

	// idMatX operations
	virtual void MatX_MultiplyVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) = 0;
	virtual void MatX_MultiplyAddVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) = 0;
	virtual void MatX_MultiplySubVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) = 0;
	virtual void MatX_TransposeMultiplyVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) = 0;
	virtual void MatX_TransposeMultiplyAddVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) = 0;
	virtual void MatX_TransposeMultiplySubVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) = 0;
	virtual void MatX_MultiplyMatX( idMatX &dst, const idMatX &m1, const idMatX &m2 ) = 0;
	virtual void MatX_TransposeMultiplyMatX( idMatX &dst, const idMatX &m1, const idMatX &m2 ) = 0;
	virtual void MatX_LowerTriangularSolve( const idMatX &L, float *x, const float *b, const int n, int skip = 0 ) = 0;
	virtual void MatX_LowerTriangularSolveTranspose( const idMatX &L, float *x, const float *b, const int n ) = 0;
	virtual bool MatX_LDLTFactor( idMatX &mat, idVecX &invDiag, const int n ) = 0;

	// skeletal animation
	virtual void BlendJoints( idJointQuat *joints, const idJointQuat *blendJoints, const float lerp, const int *index, const int numJoints ) = 0;
	virtual void ConvertJointQuatsToJointMats( idJointMat *jointMats, const idJointQuat *jointQuats, const int numJoints ) = 0;
	virtual void ConvertJointMatsToJointQuats( idJointQuat *jointQuats, const idJointMat *jointMats, const int numJoints ) = 0;
	virtual void TransformJoints( idJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint ) = 0;
	virtual void UntransformJoints( idJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint ) = 0;
	virtual void TransformVerts( idDrawVert *verts, const int numVerts, const idJointMat *joints, const idVec4 *weights, const int *index, const int numWeights ) = 0;
	virtual void ComputeBoundsFromJointBounds( idBounds &totalBounds, int numJoints, const idJointMat *joints, const idBounds *jointBounds ) = 0;

	// rendering
	virtual void TracePointCull( byte *cullBits, byte &totalOr, const float radius, const idPlane *planes, const idDrawVert *verts, const int numVerts ) = 0;
	virtual void DecalPointCull( byte *cullBits, const idPlane *planes, const idDrawVert *verts, const int numVerts ) = 0;
	virtual void OverlayPointCull( byte *cullBits, idVec2 *texCoords, const idPlane *planes, const idDrawVert *verts, const int numVerts ) = 0;
	virtual void CalcTriFacing( const idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes, const idVec3 &lightOrigin, byte *facing ) = 0;
	virtual void DeriveTriPlanes( idPlane *planes, const idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes ) = 0;
	virtual void DeriveTangents( idPlane *planes, idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes ) = 0;
	virtual void DeriveUnsmoothedTangents( idDrawVert *verts, const dominantTri_s *dominantTris, const int numVerts ) = 0;
	virtual void NormalizeTangents( idDrawVert *verts, const int numVerts ) = 0;
	virtual int  CreateShadowCache( idVec4 *vertexCache, int *vertRemap, const idVec3 &lightOrigin, const idDrawVert *verts, const int numVerts ) = 0;
	virtual int  CreateVertexProgramShadowCache( idVec4 *vertexCache, const idDrawVert *verts, const int numVerts ) = 0;
	virtual void CullByFrustum( idDrawVert *verts, const int numVerts, const idPlane frustum[6], byte *pointCull, float epsilon ) = 0;
	virtual void CullByFrustum2( idDrawVert *verts, const int numVerts, const idPlane frustum[6], unsigned short *pointCull, float epsilon ) = 0;
	// stgatilov #5886: makes sense only if you process part of mesh; for full mesh better call CullByFrustum, then combine cull masks from vertices
	virtual void CullTrisByFrustum( idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes, const idPlane frustum[6], byte *triCull, float epsilon ) = 0;

	// images
	virtual void GenerateMipMap2x2( const byte *srcPtr, int srcStride, int halfWidth, int halfHeight, byte *dstPtr, int dstStride ) = 0;
	virtual bool ConvertRowToRGBA8( const byte *srcPtr, int width, int bitsPerPixel, bool bgr, byte *dstPtr ) = 0;
	virtual void CompressRGTCFromRGBA8( const byte *srcPtr, int width, int height, int stride, byte *dstPtr ) = 0;
	virtual void CompressDXT1FromRGBA8( const byte *srcPtr, int width, int height, int stride, byte *dstPtr ) = 0;
	virtual void CompressDXT3FromRGBA8( const byte *srcPtr, int width, int height, int stride, byte *dstPtr ) = 0;
	virtual void CompressDXT5FromRGBA8( const byte *srcPtr, int width, int height, int stride, byte *dstPtr ) = 0;
	virtual void DecompressRGBA8FromDXT1( const byte *srcPtr, int width, int height, byte *dstPtr, int stride, bool allowTransparency ) = 0;
	virtual void DecompressRGBA8FromDXT3( const byte *srcPtr, int width, int height, byte *dstPtr, int stride ) = 0;
	virtual void DecompressRGBA8FromDXT5( const byte *srcPtr, int width, int height, byte *dstPtr, int stride ) = 0;
	virtual void DecompressRGBA8FromRGTC( const byte *srcPtr, int width, int height, byte *dstPtr, int stride ) = 0;

	// sound mixing
	virtual void UpSamplePCMTo44kHz( float *dest, const short *pcm, const int numSamples, const int kHz, const int numChannels ) = 0;
	virtual void UpSampleOGGTo44kHz( float *dest, const float * const *ogg, const int numSamples, const int kHz, const int numChannels ) = 0;
	virtual void MixSoundTwoSpeakerMono( float *mixBuffer, const float *samples, const int numSamples, const float lastV[2], const float currentV[2] ) = 0;
	virtual void MixSoundTwoSpeakerStereo( float *mixBuffer, const float *samples, const int numSamples, const float lastV[2], const float currentV[2] ) = 0;
	virtual void MixSoundSixSpeakerMono( float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6] ) = 0;
	virtual void MixSoundSixSpeakerStereo( float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6] ) = 0;
	virtual void MixedSoundToSamples( short *samples, const float *mixBuffer, const int numSamples ) = 0;
};

// pointer to SIMD processor
extern idSIMDProcessor *SIMDProcessor;

#endif /* !__MATH_SIMD_H__ */
