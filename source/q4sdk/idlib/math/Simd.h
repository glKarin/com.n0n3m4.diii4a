#ifndef __MATH_SIMD_H__
#define __MATH_SIMD_H__

/*
===============================================================================

	Single Instruction Multiple Data (SIMD)

	For optimal use data should be aligned on a 16 byte boundary.
	All idSIMDProcessor routines are thread safe.

===============================================================================
*/

class idSIMD {
public:
	static void			Init( void );
	static void			InitProcessor( const char *module, bool forceGeneric );
	static void			Shutdown( void );
	static void			Test_f( const class idCmdArgs &args );
};


/*
===============================================================================

	virtual base class for different SIMD processors

===============================================================================
*/

#ifdef _WIN32
#define VPCALL __fastcall
#else
#define VPCALL
#endif

class idVec2;
class idVec3;
#ifdef _XENON
class __declspec(align(16)) idVec4;
#else
class idVec4;
#endif
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
class idBounds;
class idDrawVert;
class idJointQuat;
class idJointMat;
struct dominantTri_s;
struct jointWeight_t;
struct silEdge_s;

// RAVEN BEGIN
// dluetscher: declared new vertex format
#ifdef _MD5R_SUPPORT
class rvSilTraceVertT;
#endif
// RAVEN END

const int MIXBUFFER_SAMPLES = 4096;

typedef enum {
	SPEAKER_LEFT = 0,
	SPEAKER_RIGHT,
	SPEAKER_CENTER,
	SPEAKER_LFE,
	SPEAKER_BACKLEFT,
	SPEAKER_BACKRIGHT
} speakerLabel;


// RAVEN BEGIN
// jsinger: forward declare and use a typedef so that xenon doesn't have to use inheritence for the SIMD stuff
#ifdef _XENON
class idSIMD_Xenon;
typedef idSIMD_Xenon idSIMDProcessor;
#else
// RAVEN END
class idSIMDProcessor {
public:
									idSIMDProcessor( void ) { cpuid = CPUID_NONE; }
									virtual ~idSIMDProcessor( void ) { }

	cpuid_t							cpuid;

	virtual const char * VPCALL		GetName( void ) const = 0;

	virtual void VPCALL Add( float *dst,			const float constant,	const float *src,		const int count ) = 0;
	virtual void VPCALL Add( float *dst,			const float *src0,		const float *src1,		const int count ) = 0;
	virtual void VPCALL Sub( float *dst,			const float constant,	const float *src,		const int count ) = 0;
	virtual void VPCALL Sub( float *dst,			const float *src0,		const float *src1,		const int count ) = 0;
	virtual void VPCALL Mul( float *dst,			const float constant,	const float *src,		const int count ) = 0;
	virtual void VPCALL Mul( float *dst,			const float *src0,		const float *src1,		const int count ) = 0;
	virtual void VPCALL Div( float *dst,			const float constant,	const float *src,		const int count ) = 0;
	virtual void VPCALL Div( float *dst,			const float *src0,		const float *src1,		const int count ) = 0;
	virtual void VPCALL MulAdd( float *dst,			const float constant,	const float *src,		const int count ) = 0;
	virtual void VPCALL MulAdd( float *dst,			const float *src0,		const float *src1,		const int count ) = 0;
	virtual void VPCALL MulSub( float *dst,			const float constant,	const float *src,		const int count ) = 0;
	virtual void VPCALL MulSub( float *dst,			const float *src0,		const float *src1,		const int count ) = 0;

	virtual	void VPCALL Dot( float *dst,			const idVec3 &constant,	const idVec3 *src,		const int count ) = 0;
	virtual	void VPCALL Dot( float *dst,			const idVec3 &constant,	const idPlane *src,		const int count ) = 0;
	virtual void VPCALL Dot( float *dst,			const idVec3 &constant,	const idDrawVert *src,	const int count ) = 0;
	virtual	void VPCALL Dot( float *dst,			const idPlane &constant,const idVec3 *src,		const int count ) = 0;
	virtual	void VPCALL Dot( float *dst,			const idPlane &constant,const idPlane *src,		const int count ) = 0;
	virtual void VPCALL Dot( float *dst,			const idPlane &constant,const idDrawVert *src,	const int count ) = 0;
	virtual	void VPCALL Dot( float *dst,			const idVec3 *src0,		const idVec3 *src1,		const int count ) = 0;
	virtual void VPCALL Dot( float &dot,			const float *src1,		const float *src2,		const int count ) = 0;

	virtual	void VPCALL CmpGT( byte *dst,			const float *src0,		const float constant,	const int count ) = 0;
	virtual	void VPCALL CmpGT( byte *dst,			const byte bitNum,		const float *src0,		const float constant,	const int count ) = 0;
	virtual	void VPCALL CmpGE( byte *dst,			const float *src0,		const float constant,	const int count ) = 0;
	virtual	void VPCALL CmpGE( byte *dst,			const byte bitNum,		const float *src0,		const float constant,	const int count ) = 0;
	virtual	void VPCALL CmpLT( byte *dst,			const float *src0,		const float constant,	const int count ) = 0;
	virtual	void VPCALL CmpLT( byte *dst,			const byte bitNum,		const float *src0,		const float constant,	const int count ) = 0;
	virtual	void VPCALL CmpLE( byte *dst,			const float *src0,		const float constant,	const int count ) = 0;
	virtual	void VPCALL CmpLE( byte *dst,			const byte bitNum,		const float *src0,		const float constant,	const int count ) = 0;

	virtual	void VPCALL MinMax( float &min,			float &max,				const float *src,		const int count ) = 0;
	virtual	void VPCALL MinMax( idVec2 &min,		idVec2 &max,			const idVec2 *src,		const int count ) = 0;
	virtual	void VPCALL MinMax( idVec3 &min,		idVec3 &max,			const idVec3 *src,		const int count ) = 0;
	virtual	void VPCALL MinMax( idVec3 &min,		idVec3 &max,			const idDrawVert *src,	const int count ) = 0;
	virtual	void VPCALL MinMax( idVec3 &min,		idVec3 &max,			const idDrawVert *src,	const int *indexes,		const int count ) = 0;

	virtual	void VPCALL Clamp( float *dst,			const float *src,		const float min,		const float max,		const int count ) = 0;
	virtual	void VPCALL ClampMin( float *dst,		const float *src,		const float min,		const int count ) = 0;
	virtual	void VPCALL ClampMax( float *dst,		const float *src,		const float max,		const int count ) = 0;

	virtual void VPCALL Memcpy( void *dst,			const void *src,		const int count ) = 0;
	virtual void VPCALL Memset( void *dst,			const int val,			const int count ) = 0;

	// these assume 16 byte aligned and 16 byte padded memory
	virtual void VPCALL Zero16( float *dst,			const int count ) = 0;
	virtual void VPCALL Negate16( float *dst,		const int count ) = 0;
	virtual void VPCALL Copy16( float *dst,			const float *src,		const int count ) = 0;
	virtual void VPCALL Add16( float *dst,			const float *src1,		const float *src2,		const int count ) = 0;
	virtual void VPCALL Sub16( float *dst,			const float *src1,		const float *src2,		const int count ) = 0;
	virtual void VPCALL Mul16( float *dst,			const float *src1,		const float constant,	const int count ) = 0;
	virtual void VPCALL AddAssign16( float *dst,	const float *src,		const int count ) = 0;
	virtual void VPCALL SubAssign16( float *dst,	const float *src,		const int count ) = 0;
	virtual void VPCALL MulAssign16( float *dst,	const float constant,	const int count ) = 0;

	// idMatX operations
	virtual void VPCALL MatX_MultiplyVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) = 0;
	virtual void VPCALL MatX_MultiplyAddVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) = 0;
	virtual void VPCALL MatX_MultiplySubVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) = 0;
	virtual void VPCALL MatX_TransposeMultiplyVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) = 0;
	virtual void VPCALL MatX_TransposeMultiplyAddVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) = 0;
	virtual void VPCALL MatX_TransposeMultiplySubVecX( idVecX &dst, const idMatX &mat, const idVecX &vec ) = 0;
	virtual void VPCALL MatX_MultiplyMatX( idMatX &dst, const idMatX &m1, const idMatX &m2 ) = 0;
	virtual void VPCALL MatX_TransposeMultiplyMatX( idMatX &dst, const idMatX &m1, const idMatX &m2 ) = 0;
	virtual void VPCALL MatX_LowerTriangularSolve( const idMatX &L, float *x, const float *b, const int n, int skip = 0 ) = 0;
	virtual void VPCALL MatX_LowerTriangularSolveTranspose( const idMatX &L, float *x, const float *b, const int n ) = 0;
	virtual bool VPCALL MatX_LDLTFactor( idMatX &mat, idVecX &invDiag, const int n ) = 0;

	// rendering
	virtual void VPCALL BlendJoints( idJointQuat *joints, const idJointQuat *blendJoints, const float lerp, const int *index, const int numJoints ) = 0;
	virtual void VPCALL ConvertJointQuatsToJointMats( idJointMat *jointMats, const idJointQuat *jointQuats, const int numJoints ) = 0;
	virtual void VPCALL ConvertJointMatsToJointQuats( idJointQuat *jointQuats, const idJointMat *jointMats, const int numJoints ) = 0;
	virtual void VPCALL TransformJoints( idJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint ) = 0;
	virtual void VPCALL UntransformJoints( idJointMat *jointMats, const int *parents, const int firstJoint, const int lastJoint ) = 0;
	virtual void VPCALL MultiplyJoints( idJointMat *result, const idJointMat *joints1, const idJointMat *joints2, const int numJoints ) = 0;
	virtual void VPCALL TransformVertsNew( idDrawVert *verts, const int numVerts, idBounds &bounds, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights ) = 0;
	virtual void VPCALL TransformVertsAndTangents( idDrawVert *verts, const int numVerts, idBounds &bounds, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights ) = 0;
	virtual void VPCALL TransformVertsAndTangentsFast( idDrawVert *verts, const int numVerts, idBounds &bounds, const idJointMat *joints, const idVec4 *base, const jointWeight_t *weights, const int numWeights ) = 0;
	virtual void VPCALL TracePointCull( byte *cullBits, byte &totalOr, const float radius, const idPlane *planes, const idDrawVert *verts, const int numVerts ) = 0;
	virtual void VPCALL DecalPointCull( byte *cullBits, const idPlane *planes, const idDrawVert *verts, const int numVerts ) = 0;
	virtual void VPCALL OverlayPointCull( byte *cullBits, idVec2 *texCoords, const idPlane *planes, const idDrawVert *verts, const int numVerts ) = 0;
	virtual void VPCALL DeriveTriPlanes( idPlane *planes, const idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes ) = 0;
	virtual void VPCALL DeriveTangents( idPlane *planes, idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes ) = 0;
	virtual void VPCALL DeriveUnsmoothedTangents( idDrawVert *verts, const dominantTri_s *dominantTris, const int numVerts ) = 0;
	virtual void VPCALL NormalizeTangents( idDrawVert *verts, const int numVerts ) = 0;
	virtual void VPCALL CreateTextureSpaceLightVectors( idVec3 *lightVectors, const idVec3 &lightOrigin, const idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes ) = 0;
	virtual void VPCALL CreateSpecularTextureCoords( idVec4 *texCoords, const idVec3 &lightOrigin, const idVec3 &viewOrigin, const idDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes ) = 0;
	virtual int  VPCALL CreateShadowCache( idVec4 *vertexCache, int *vertRemap, const idVec3 &lightOrigin, const idDrawVert *verts, const int numVerts ) = 0;
	virtual int  VPCALL CreateVertexProgramShadowCache( idVec4 *vertexCache, const idDrawVert *verts, const int numVerts ) = 0;
	virtual int  VPCALL ShadowVolume_CountFacing( const byte *facing, const int numFaces ) = 0;
	virtual int  VPCALL ShadowVolume_CountFacingCull( byte *facing, const int numFaces, const int *indexes, const byte *cull ) = 0;
	virtual int  VPCALL ShadowVolume_CreateSilTriangles( int *shadowIndexes, const byte *facing, const silEdge_s *silEdges, const int numSilEdges ) = 0;
	virtual int  VPCALL ShadowVolume_CreateCapTriangles( int *shadowIndexes, const byte *facing, const int *indexes, const int numIndexes ) = 0;

	// sound mixing
	virtual void VPCALL UpSamplePCMTo44kHz( float *dest, const short *pcm, const int numSamples, const int kHz, const int numChannels ) = 0;
	virtual void VPCALL UpSampleOGGTo44kHz( float *dest, const float * const *ogg, const int numSamples, const int kHz, const int numChannels ) = 0;
	virtual void VPCALL MixSoundTwoSpeakerMono( float *mixBuffer, const float *samples, const int numSamples, const float lastV[2], const float currentV[2] ) = 0;
	virtual void VPCALL MixSoundTwoSpeakerMonoSimple( float * RESTRICT mixBuffer, const float * RESTRICT samples, const int numSamples ) = 0;
	virtual void VPCALL MixSoundTwoSpeakerStereo( float *mixBuffer, const float *samples, const int numSamples, const float lastV[2], const float currentV[2] ) = 0;
	virtual void VPCALL MixSoundSixSpeakerMono( float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6] ) = 0;
	virtual void VPCALL MixSoundSixSpeakerMonoSimple( float * RESTRICT mixBuffer, const float * RESTRICT samples, const int numSamples ) = 0;
	virtual void VPCALL MixSoundSixSpeakerStereo( float *mixBuffer, const float *samples, const int numSamples, const float lastV[6], const float currentV[6] ) = 0;
	virtual void VPCALL MixedSoundToSamples( short *samples, const float *mixBuffer, const int numSamples ) = 0;

	// rvSilTraceVertT operations
// RAVEN BEGIN
// dluetscher: added support for operations on idSilTraceVerts
#ifdef _MD5R_SUPPORT
	virtual void VPCALL JointMat_MultiplyMats( float *destMats, const idJointMat *src1Mats, const idJointMat *src2Mats, int *transformPalette, int transformCount ) = 0;
	virtual void VPCALL TransformVertsMinMax4Bone( rvSilTraceVertT *silTraceVertOutputData, idVec3 &min, idVec3 &max, byte *vertexInputData, int vertStride, int numVerts, float *skinToModelTransforms ) = 0; // transforms an array of index-weighted vertices into an array of idSilTraceVerts, while simulatenously calculating the bounds
	virtual void VPCALL TransformVertsMinMax1Bone( rvSilTraceVertT *silTraceVertOutputData, idVec3 &min, idVec3 &max, byte *vertexInputData, int vertStride, int numVerts, float *skinToModelTransforms ) = 0; // transforms an array of index-weighted vertices into an array of idSilTraceVerts, while simulatenously calculating the bounds
	virtual void VPCALL Dot( float *dst, const idVec3 &constant, const rvSilTraceVertT *src,	const int count ) = 0;
	virtual void VPCALL Dot( float *dst, const idPlane &constant, const rvSilTraceVertT *src, const int count ) = 0;
	virtual void VPCALL TracePointCull( byte *cullBits, byte &totalOr, const float radius, const idPlane *planes, const rvSilTraceVertT *verts, const int numVerts ) = 0;
	virtual void VPCALL DecalPointCull( byte *cullBits, const idPlane *planes, const rvSilTraceVertT *verts, const int numVerts ) = 0;
	virtual void VPCALL OverlayPointCull( byte *cullBits, idVec2 *texCoords, const idPlane *planes, const rvSilTraceVertT *verts, const int numVerts ) = 0;
	virtual void VPCALL DeriveTriPlanes( idPlane *planes, const rvSilTraceVertT *verts, const int numVerts, const int *indexes, const int numIndexes ) = 0;
	virtual void VPCALL DeriveTriPlanes( idPlane *planes, const rvSilTraceVertT *verts, const int numVerts, const unsigned short *indexes, const int numIndexes ) = 0;
	virtual	void VPCALL MinMax( idVec3 &min, idVec3 &max, const rvSilTraceVertT *src, const int count ) = 0;
	virtual	void VPCALL MinMax( idVec3 &min, idVec3 &max, const rvSilTraceVertT *src, const int *indexes, const int count ) = 0;
#endif
// RAVEN END
};
// RAVEN BEGIN
#endif
// RAVEN END

// pointer to SIMD processor
extern idSIMDProcessor *SIMDProcessor;

#endif /* !__MATH_SIMD_H__ */
