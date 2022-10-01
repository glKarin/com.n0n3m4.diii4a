// Copyright (C) 2007 Id Software, Inc.
//


#ifndef _BSE_PARTICLE_H_INC_
#define _BSE_PARTICLE_H_INC_

#include "../../renderer/ModelManager.h"
#include "../../renderer/RenderWorld.h"

#define BSE_FUTURE				( 0.016f )			// How far into the future to check for particle spawning
#define BSE_TIME_EPSILON		( 0.002f )			// Edge condition checks
#define BSE_PHYSICS_TIME_SAMPLE	( 0.1f )			// Number of seconds to check the position delta for physics
#define BSE_MINIMUM_TRACE_DIST	( 16.0f )			// The square of the distance below which physics will not check
#define BSE_SURFACE_OFFSET		( 2.0f )			// Amount a collision is pushed back along the normal
#define BSE_BOUNCE_LIMIT		( 2500.0f )			// Square of velocity below which a particle stops bouncing
#define BSE_TRACE_OFFSET		( 0.02f )			// How much the bounce should move back from the endpos
#define MAX_PARTICLES			( 2048 )			// Max number of particles attached to an effect (arbitrary sanity check)
#define BSE_DENSITY_FACTOR		( 50.0f )

#define BSE_MAX_FORKS			( 16 )

#define BSE_MAX_DURATION		( 60.0f * 5.0f )	// 5 Minutes

#define BSE_NUM_SPAWNABLE		( 4 )				// Number of random effects to choose from

enum
{
	PTYPE_NONE = 0,									// A non sprite - for sound and vision segments
	PTYPE_SPRITE,									// Simple 2D alpha blended quad
	PTYPE_LINE,										// 2D alpha blended line
	PTYPE_ORIENTED,									// 2D particle oriented in 3D - alpha blended
	PTYPE_DECAL,									// Hook into id's decal system
	PTYPE_MODEL,									// Model - must only have 1 surface
	PTYPE_LIGHT,									// Dynamic light - very expensive
	PTYPE_ELECTRICITY,								// A bolt of electricity
	PTYPE_LINKED,									// A series of linked lines
	PTYPE_ORIENTEDLINKED,
	PTYPE_DEBRIS,									// A client side moveable entity spawned in the game
	PTYPE_COUNT
};

// Defined classes 
class rvParticle;
class rvSpriteParticle;
class rvLineParticle;
class rvOrientedParticle;
class rvElectricityParticle;
class rvDecalParticle;
class rvModelParticle;
class rvLightParticle;
class rvLinkedParticle;
class sdOrientedLinkedParticle;
class rvDebrisParticle;

class rvParticleTemplate;
class rvParticleParms;

// Referenced classes
class rvBSE;
class rvDeclEffect;
class rvSegment;
class rvSegmentTemplate;

#define PTFLAG_STATIONARY			BIT( 0 )
#define PTFLAG_LOCKED				BIT( 1 )
#define PTFLAG_HAS_OFFSET			BIT( 2 )

#define PTFLAG_PARSED				BIT( 8 )
#define PTFLAG_HAS_PHYSICS			BIT( 9 )
#define PTFLAG_DELETE_ON_IMPACT		BIT( 10 )
#define PTFLAG_GENERATED_NORMAL		BIT( 11 )
#define PTFLAG_GENERATED_ORG_NORMAL	BIT( 12 )
#define PTFLAG_FLIPPED_NORMAL		BIT( 13 )
#define PTFLAG_CALCED_NORMAL		BIT( 14 )
#define PTFLAG_ADDITIVE				BIT( 15 )
#define PTFLAG_GENERATED_LINE		BIT( 16 )
#define PTFLAG_SHADOWS				BIT( 17 )
#define PTFLAG_SPECULAR				BIT( 18 )
#define PTFLAG_LINKED				BIT( 19 )
#define PTFLAG_TILED				BIT( 20 )
#define PTFLAG_PERSIST				BIT( 21 )
#define PTFLAG_USELIGHTNING_AXIS	BIT( 22 )
#define PTFLAG_FADE_IN				BIT( 23 )
#define PTFLAG_USE_MATERIAL_COLOR	BIT( 24 )
#define PTFLAG_PARENTVEL			BIT( 25 )
#define PTFLAG_HAS_LINEHIT			BIT( 26 )
#define PTFLAG_INITED				BIT( 27 )

// ==============================================
// Particle types (from multiple elements)
// ==============================================
class rvParticle
{
public:
	friend		class			rvParticleTemplate;

								rvParticle( void ) {}
	virtual						~rvParticle( void ) {}

				void			SetFlag( bool on, int flag ) { on ? mFlags |= flag : mFlags &= ~flag; }
				bool			GetFlag( int flag ) const { return ( mFlags & flag ) != 0 ; }
				
				int			GetStationary( void ) const { return( ( mFlags & PTFLAG_STATIONARY ) ); }
				int			GetLocked( void ) const { return( ( mFlags & PTFLAG_LOCKED ) ); }
				int			GetHasOffset( void ) const { return( ( mFlags & PTFLAG_HAS_OFFSET ) ); }

				int			GetGeneratedLine( void ) const { return( ( mFlags & PTFLAG_GENERATED_LINE ) ); }
				int			GetAdditive( void ) const { return( ( mFlags & PTFLAG_ADDITIVE ) ); }
				int			GetTiled( void ) const { return( ( mFlags & PTFLAG_TILED ) ); }
				int			GetPersist( void ) const { return( ( mFlags & PTFLAG_PERSIST ) ); }

				int			GetFlags(void) {
					return mFlags;
				}

				void			SetStationary( bool stopped ) { SetFlag( stopped, PTFLAG_STATIONARY ); }
				void			SetLocked( bool locked ) { SetFlag( locked, PTFLAG_LOCKED ); }
				void			SetHasOffset( bool hasOffset ) { SetFlag( hasOffset, PTFLAG_HAS_OFFSET ); }

				void			SetNext( rvParticle *next ) { mNext = next; }
				rvParticle		*GetNext( void ) const { return( mNext ); }	

	ID_INLINE   float			GetDuration( void ) const { return( mEndTime - mStartTime ); }
				void			ExtendLife( float time ) { mEndTime = time; }	
				bool			Expired( float time ) const { return( time >= mEndTime - BSE_TIME_EPSILON ); }
	
				void			CalcImpactPoint( idVec3 &endPos, const idVec3 &origin, const idVec3 &motion, const idBounds &bounds, const idVec3 &normal );
				void			SetOriginUsingEndOrigin( rvBSE *effect, rvParticleTemplate *pt, idVec3 *normal, idVec3 *centre );
				void			HandleEndOrigin( rvBSE *effect, rvParticleTemplate *pt, idVec3 *normal = NULL, idVec3 *centre = NULL );
				void			SetLengthUsingEndOrigin( rvBSE *effect, rvParticleParms &parms, float *length );
				void			HandleEndLength( rvBSE *effect, rvParticleTemplate *pt, rvParticleParms &parms, float *length );
				void			Bounce( rvBSE *effect, rvParticleTemplate *pt, idVec3 endPos, idVec3 normal, float time );
				bool			RunPhysics( rvBSE *effect, rvSegmentTemplate *pt, float time );
				void			CheckTimeoutEffect( rvBSE *effect, rvSegmentTemplate *st, float time );
				dword			HandleTint( const rvBSE *effect, idVec4 &colour, float alpha );
				void			RenderQuadTrail( const rvBSE *effect, srfTriangles_t *tri, idVec3 offset, float fraction, idVec4 &colour, idVec3 &pos, bool first );
				void			EmitSmokeParticles( rvBSE *effect, rvSegment *child, rvParticleTemplate *pt, float time );
				void			ScaleAngle( float constant ) { mAngleEnv.Scale( constant ); }
				float			GetEndTime( void ) const { return( mEndTime ); }

	virtual		rvParticle		*GetArrayEntry( int i ) const { assert( 0 ); return( NULL ); }
	virtual		int				GetArrayIndex( rvParticle *p ) const { assert( 0 ); return( 0 ); }

				bool			GetEvaluationTime( float time, float &evalTime, bool infinite = false );
				void			EvaluatePosition( const rvBSE *effect, rvParticleTemplate *pt, idVec3 &pos, float time );
				void			EvaluateVelocity( const rvBSE *effect, idVec3 &velocity, float time );

	ID_INLINE	void			EvaluateTint( rvEnvParms *tint, rvEnvParms *fade, const float time, float oneOverDuration, idVec4 &dest ) {
									tint->Evaluate( mTintEnv, time, oneOverDuration, dest.ToFloatPtr() );
									fade->Evaluate( mFadeEnv, time, oneOverDuration, &dest[3] );
								}

	ID_INLINE	void			EvaluateAngle( rvEnvParms *angle, const float time, float oneOverDuration, rvAngles &dest ) {
									angle->Evaluate( mAngleEnv, time, oneOverDuration, dest.ToFloatPtr() );
								}

	ID_INLINE	void			EvaluateOffset( rvEnvParms *offset, const float time, float oneOverDuration, idVec3 &dest ) {
									offset->Evaluate( mOffsetEnv, time, oneOverDuration, dest.ToFloatPtr() );
								}

	
	virtual		void			EvaluateSize( rvEnvParms *size, const float time, float oneOverDuration, float *dest ) { assert( 0 ); }
	virtual		void			EvaluateRotation( rvEnvParms *rotation, const float time, float oneOverDuration, float *dest ) { assert( 0 ); }
	virtual		void			EvaluateLength( rvEnvParms *length, const float time, float oneOverDuration, idVec3 &dest ) { assert( 0 ); }
	

				void			InitTintEnv( rvEnvParms &env, float duration ) { mTintEnv.Init( env, duration ); }
				void			InitFadeEnv( rvEnvParms &env, float duration ) { mFadeEnv.Init( env, duration ); }
				void			InitAngleEnv( rvEnvParms &env, float duration ) { mAngleEnv.Init( env, duration ); }
				void			InitOffsetEnv( rvEnvParms &env, float duration ) { mOffsetEnv.Init( env, duration ); }
	virtual		void			InitSizeEnv( rvEnvParms &env, float duration ) { assert( 0 ); }
	virtual		void			InitRotationEnv( rvEnvParms &env, float duration ) { assert( 0 ); }
	virtual		void			InitLengthEnv( rvEnvParms &env, float duration ) { assert( 0 ); }

	virtual		float			*GetInitSize( void ) { assert( 0 ); return( NULL ); }
	virtual		float			*GetDestSize( void ) { assert( 0 ); return( NULL ); }

	virtual		float			*GetInitRotation( void ) { assert( 0 ); return( NULL ); }
	virtual		float			*GetDestRotation( void ) { assert( 0 ); return( NULL ); }
	virtual		void			ScaleRotation( float constant ) {}

	virtual		float			*GetInitLength( void ) { assert( 0 );  return( NULL ); }
	virtual		float			*GetDestLength( void ) { assert( 0 );  return( NULL ); }

				void			Attenuate( float atten, rvParticleParms &parms, rvEnvParms1 &result );
				void			Attenuate( float atten, rvParticleParms &parms, rvEnvParms2 &result );
				void			Attenuate( float atten, rvParticleParms &parms, rvEnvParms3 &result );
				void			Attenuate( float atten, rvParticleParms &parms, rvEnvParms1Particle &result );
				void			Attenuate( float atten, rvParticleParms &parms, rvEnvParms2Particle &result );
				void			Attenuate( float atten, rvParticleParms &parms, rvEnvParms3Particle &result );

				void			AttenuateFade( float atten, rvParticleParms &parms ) { Attenuate( atten, parms, mFadeEnv ); }
	virtual		void			AttenuateSize( float atten, rvParticleParms &parms ) {}
	virtual		void			AttenuateLength( float atten, rvParticleParms &parms ) {}

	virtual		void			TransformLength( idVec3 normal ) {}
	virtual		void			ScaleLength( float constant ) {}
	virtual		void			GetSpawnInfo( idVec4 &tint, idVec3 &size, idVec3 &rotate ) { assert( 0 ); }
	virtual		void			HandleTiling( rvParticleTemplate *pt ) {}

	virtual		void			HandleOrientation( rvAngles &angles ) {}
	virtual		void			FinishSpawn( rvBSE *effect, rvSegment *segment, float birthTime, float fraction = 0.0f, const idVec3 &initOffset = vec3_origin, const idMat3 &initAxis = mat3_identity );
	virtual		int				Update( rvParticleTemplate *pt, float time ) { return( 1 ); }
	virtual		bool			Render( const rvBSE *effect, rvParticleTemplate *pt, const idMat3 &view, srfTriangles_t *tri, float time, float override = 1.0f ) { return( false ); }
				void			DoRenderBurnTrail( rvBSE *effect, rvParticleTemplate *st, const idMat3 &view, srfTriangles_t *tri, float time );
	virtual		void			RenderBurnTrail( rvBSE *effect, rvParticleTemplate *pt, const idMat3 &view, srfTriangles_t *tri, float time ) {}
	virtual		void			RenderMotion( rvBSE *effect, rvParticleTemplate *pt, srfTriangles_t *tri, const struct renderEffect_s *owner, float time, float trailScale );
	virtual		bool			InitLight( rvBSE *effect, rvSegmentTemplate *st, float time ) { return( false ); }
	virtual		bool			PresentLight( rvBSE *effect, rvParticleTemplate *pt, float time, bool infinite ) { return( false ); }
	virtual		bool			Destroy( void ) { return( false ); }
	virtual		void			SetModel( const idRenderModel *model ) {}
	virtual		void			SetupElectricity( rvParticleTemplate *pt ) {}
	virtual		void			Refresh( rvBSE *effect, rvSegmentTemplate *st, rvParticleTemplate *pt ) {}

				float			UpdateViewDist( const idVec3 &eyePos ) { return (mPosition - eyePos).LengthSqr(); }
//REMOVE		unsigned int				GetSortKey( void ) { return *((int *)(&mSqrViewDist)); }
//REMOVE	const idVec3&				GetPosition() const { return mPosition; }
protected:
				// Can be altered in UpdateParticles
	class		rvParticle		*mNext;
				float			mMotionStartTime;						// World start time for motion calcs - reset on a bounce

				// could be stored as a delta from start
				float			mLastTrailTime;							// Last time a smoke particle was emitted

				// need to restructure flags
				int				mFlags;

				// Fixed at spawn time
				float			mStartTime;								// World start time of particle in seconds
				float			mEndTime;								// End of particle's life

				float			mTrailTime;								// Length of trail
				int				mTrailCount;							// Number of particles in trail

				// always between 0 and 1 inc
				float			mFraction;								// Fraction along the line

				float			mTextureScale;							// Multiplier for the texture coord
				float			mTextureOffset;

				idVec3			mInitEffectPos;							// Initial effect position when the particle is spawned
				idMat3			mInitAxis;								// Initial effect axis when the particle is spawned

				idVec3			mInitPos;								// Position at time = 0
				idVec3			mVelocity;								// Initial velocity
				idVec3			mAcceleration;							// Initial acceleration
				float			mFriction;								// Friction works by slowing down over time, this float is basically ( 1 / [Time in seconds from spawning at which velocity should be zero] )

				rvEnvParms3Particle		mTintEnv;
				rvEnvParms1Particle		mFadeEnv;
				rvEnvParms3Particle		mAngleEnv;
				rvEnvParms3Particle		mOffsetEnv;
				int			mTrailRepeat;

				idVec3					mPosition;								// Updated every time EvaluatePosition is called
//REMOVE			float					mSqrViewDist;	// Fixme: Allocate on stack during sorting?						// For sorting
};

class rvSpriteParticle : public rvParticle
{
public:
	friend		class			rvParticleTemplate;

	virtual		rvParticle		*GetArrayEntry( int i ) const;
	virtual		int				GetArrayIndex( rvParticle *p ) const;

	virtual		void			EvaluateSize( rvEnvParms *size, const float time, float oneOverDuration, float *dest ) {
		size->Evaluate( mSizeEnv, time, oneOverDuration, dest );
	}
	virtual		void			EvaluateRotation( rvEnvParms *rotation, const float time, float oneOverDuration, float *dest ) {
		rotation->Evaluate( mRotationEnv, time, oneOverDuration, dest );
	}
	//virtual		void			EvaluateLength( rvEnvParms *length, const float time, float oneOverDuration, idVec3 &dest ) {}

	virtual		void			InitSizeEnv( rvEnvParms &env, float duration ) { mSizeEnv.Init( env, duration ); }
	virtual		float			*GetInitSize( void ) { return( mSizeEnv.GetStart() ); }
	virtual		float			*GetDestSize( void ) { return( mSizeEnv.GetEnd() ); }

	virtual		void			InitRotationEnv( rvEnvParms &env, float duration ) { mRotationEnv.Init( env, duration ); }
	virtual		float			*GetInitRotation( void ) { return( mRotationEnv.GetStart() ); }
	virtual		float			*GetDestRotation( void ) { return( mRotationEnv.GetEnd() ); }

	virtual		void			ScaleRotation( float constant ) { mRotationEnv.Scale( constant ); }

	virtual		void			AttenuateSize( float atten, rvParticleParms &parms ) { Attenuate( atten, parms, mSizeEnv ); }

	virtual		void			GetSpawnInfo( idVec4 &tint, idVec3 &size, idVec3 &rotate );
	virtual		bool			Render( const rvBSE *effect, rvParticleTemplate *pt, const idMat3 &view, srfTriangles_t* tri, float time, float override = 1.0f );
	virtual		void			RenderBurnTrail( rvBSE *effect, rvParticleTemplate *pt, const idMat3 &view, srfTriangles_t* tri, float time ) { DoRenderBurnTrail( effect, pt, view, tri, time ); }

protected:
				// Fixed at spawn time
				rvEnvParms2Particle		mSizeEnv;
				rvEnvParms1Particle		mRotationEnv;
};

class rvLineParticle : public rvParticle
{
public:
	friend		class			rvParticleTemplate;

	virtual		rvParticle		*GetArrayEntry( int i ) const;
	virtual		int				GetArrayIndex( rvParticle *p ) const;

	virtual		void			EvaluateSize( rvEnvParms *size, const float time, float oneOverDuration, float *dest ) {// { mSizeEnv.Evaluate( time, dest ); }
		size->Evaluate( mSizeEnv, time, oneOverDuration, dest );
	}
	virtual		void			EvaluateLength( rvEnvParms *length, const float time, float oneOverDuration, idVec3 &dest ) {// { mLengthEnv.Evaluate( time, dest.ToFloatPtr() ); }
		length->Evaluate( mLengthEnv, time, oneOverDuration, dest.ToFloatPtr() );
	}

	virtual		void			InitSizeEnv( rvEnvParms &env, float duration ) { mSizeEnv.Init( env, duration ); }
	virtual		float			*GetInitSize( void ) { return( mSizeEnv.GetStart() ); }
	virtual		float			*GetDestSize( void ) { return( mSizeEnv.GetEnd() ); }

	virtual		void			InitRotationEnv( rvEnvParms &env, float duration ) {}
	virtual		float			*GetInitRotation( void ) { return( NULL ); }
	virtual		float			*GetDestRotation( void ) { return( NULL ); }
	virtual		void			SetRotation( void ) {}

	virtual		void			InitLengthEnv( rvEnvParms &env, float duration ) { mLengthEnv.Init( env, duration ); }
	virtual		float			*GetInitLength( void ) { return( mLengthEnv.GetStart() ); }
	virtual		float			*GetDestLength( void ) { return( mLengthEnv.GetEnd() ); }
	virtual		void			TransformLength( idVec3 normal ) { mLengthEnv.Transform( normal ); }
	virtual		void			ScaleLength( float constant ) { mLengthEnv.Scale( constant ); }

	virtual		void			AttenuateSize( float atten, rvParticleParms &parms ) { Attenuate( atten, parms, mSizeEnv ); }
	virtual		void			AttenuateLength( float atten, rvParticleParms &parms ) { Attenuate( atten, parms, mLengthEnv ); }

	virtual		void			GetSpawnInfo( idVec4 &tint, idVec3 &size, idVec3 &rotate );
	virtual		void			HandleTiling( rvParticleTemplate *pt );
	virtual		void			FinishSpawn( rvBSE *effect, rvSegment *segment, float birthTime, float fraction = 0.0f, const idVec3 &initOffset = vec3_origin, const idMat3 &initAxis = mat3_identity );
	virtual		bool			Render( const rvBSE *effect, rvParticleTemplate *pt, const idMat3 &view, srfTriangles_t* tri, float time, float override = 1.0f );
	virtual		void			RenderBurnTrail( rvBSE *effect, rvParticleTemplate *pt, const idMat3 &view, srfTriangles_t* tri, float time ) { DoRenderBurnTrail( effect, pt, view, tri, time ); }
	virtual		void			Refresh( rvBSE *effect, rvSegmentTemplate *st, rvParticleTemplate *pt );

protected:
				// Fixed at spawn time
				rvEnvParms1Particle		mSizeEnv;
				rvEnvParms3Particle		mLengthEnv;
};

class rvOrientedParticle : public rvSpriteParticle
{
public:
	friend		class			rvParticleTemplate;

	virtual		rvParticle		*GetArrayEntry( int i ) const;
	virtual		int				GetArrayIndex( rvParticle *p ) const;

	virtual		void			EvaluateSize( rvEnvParms *size, const float time, float oneOverDuration, float *dest ) {// { mSizeEnv.Evaluate( time, dest ); }
		size->Evaluate( mSizeEnv, time, oneOverDuration, dest );
	}
	virtual		void			EvaluateRotation( rvEnvParms *rotation, const float time, float oneOverDuration, float *dest ) {
		rotation->Evaluate( mRotationEnv, time, oneOverDuration, dest );
	}// { mRotationEnv.Evaluate( time, dest ); }
	virtual		void			EvaluateLength( rvEnvParms *length, const float time, float oneOverDuration, idVec3 &dest ) {}

	virtual		void			InitSizeEnv( rvEnvParms &env, float duration ) { mSizeEnv.Init( env, duration ); }
	virtual		float			*GetInitSize( void ) { return( mSizeEnv.GetStart() ); }
	virtual		float			*GetDestSize( void ) { return( mSizeEnv.GetEnd() ); }

	virtual		void			InitRotationEnv( rvEnvParms &env, float duration ) { mRotationEnv.Init( env, duration ); }
	virtual		float			*GetInitRotation( void ) { return( mRotationEnv.GetStart() ); }
	virtual		float			*GetDestRotation( void ) { return( mRotationEnv.GetEnd() ); }

	virtual		void			ScaleRotation( float constant ) { mRotationEnv.Scale( constant ); }

	virtual		void			GetSpawnInfo( idVec4 &tint, idVec3 &size, idVec3 &rotate );
	virtual		void			HandleOrientation( rvAngles &angles ) { mRotationEnv.Rotate( angles ); }
	virtual		bool			Render( const rvBSE *effect, rvParticleTemplate *pt, const idMat3 &view, srfTriangles_t* tri, float time, float override = 1.0f );

private:
				// Fixed at spawn time
				rvEnvParms2Particle		mSizeEnv;
				rvEnvParms3Particle		mRotationEnv;
};

class rvElectricityParticle : public rvLineParticle
{
public:
	friend		class			rvParticleTemplate;

				int				GetBoltCount( float length );
				void			RenderBranch( const rvBSE *effect, struct SElecWork *work, idVec3 start, idVec3 end );
				void			RenderLineSegment( const rvBSE *effect, struct SElecWork *work, idVec3 start, float startFraction );
				void			ApplyShape( const rvBSE *effect, struct SElecWork *work, idVec3 start, idVec3 end, int count, float startFraction, float endFraction );

	virtual		rvParticle		*GetArrayEntry( int i ) const;
	virtual		int				GetArrayIndex( rvParticle *p ) const;

	virtual		int				Update( rvParticleTemplate *pt, float time );
	virtual		bool			Render( const rvBSE *effect, rvParticleTemplate *pt, const idMat3 &view, srfTriangles_t *tri, float time, float override = 1.0f );

	virtual		void			SetupElectricity( rvParticleTemplate *pt );

private:
				// Alterable
				int				mNumBolts;

				// Fixed at spawn time
				int				mNumForks;
				int				mSeed;
				idVec3			mForkSizeMins;
				idVec3			mForkSizeMaxs;
				idVec3			mJitterSize;
				float			mLastJitter;
				float			mJitterRate;
	const		idDeclTable		*mJitterTable;
};

class rvDecalParticle : public rvParticle //rvSpriteParticle
{
public:
	friend		class			rvParticleTemplate;

	virtual		rvParticle		*GetArrayEntry( int i ) const;
	virtual		int				GetArrayIndex( rvParticle *p ) const;

	virtual		void			EvaluateSize( rvEnvParms *size, const float time, float oneOverDuration, float *dest ) {}
	virtual		void			EvaluateRotation( rvEnvParms *rotation, const float time, float oneOverDuration, float *dest ) {
		rotation->Evaluate( mRotationEnv, time, oneOverDuration, dest );
	}
	virtual		void			EvaluateLength( rvEnvParms *length, const float time, float oneOverDuration, idVec3 &dest ) {}

	virtual		void			InitSizeEnv( rvEnvParms &env, float duration ) {}
	virtual		float			*GetInitSize( void ) { return( mSizeEnv.GetStart() ); }
	virtual		float			*GetDestSize( void ) { return( mSizeEnv.GetEnd() ); }
	virtual		void			InitRotationEnv( rvEnvParms &env, float duration ) {}
	virtual		float			*GetInitRotation( void ) { return( mRotationEnv.GetStart() ); }
	virtual		float			*GetDestRotation( void ) { return( mRotationEnv.GetEnd() ); }

	virtual		void			ScaleRotation( float constant ) { mRotationEnv.Scale( constant ); }

	virtual		void			GetSpawnInfo( idVec4 &tint, idVec3 &size, idVec3 &rotate );
private:
		rvEnvParms3Particle		mSizeEnv;
		rvEnvParms1Particle		mRotationEnv;

};

class rvModelParticle : public rvParticle
{
public:
	friend		class			rvParticleTemplate;

	virtual		rvParticle		*GetArrayEntry( int i ) const;
	virtual		int				GetArrayIndex( rvParticle *p ) const;

	virtual		void			EvaluateSize( rvEnvParms *size, const float time, float oneOverDuration, float *dest ) {// { mSizeEnv.Evaluate( time, dest ); }
		size->Evaluate( mSizeEnv, time, oneOverDuration, dest );
	}
	virtual		void			EvaluateRotation( rvEnvParms *rotation, const float time, float oneOverDuration, float *dest ) {// { mRotationEnv.Evaluate( time, dest ); }
		rotation->Evaluate( mRotationEnv, time, oneOverDuration, dest );
	}
	virtual		void			EvaluateLength( rvEnvParms *length, const float time, float oneOverDuration, idVec3 &dest ) {}

	virtual		void			InitSizeEnv( rvEnvParms &env, float duration ) { mSizeEnv.Init( env, duration ); }
	virtual		float			*GetInitSize( void ) { return( mSizeEnv.GetStart() ); }
	virtual		float			*GetDestSize( void ) { return( mSizeEnv.GetEnd() ); }

	virtual		void			InitRotationEnv( rvEnvParms &env, float duration ) { mRotationEnv.Init( env, duration ); }
	virtual		float			*GetInitRotation( void ) { return( mRotationEnv.GetStart() ); }
	virtual		float			*GetDestRotation( void ) { return( mRotationEnv.GetEnd() ); }

	virtual		void			GetSpawnInfo( idVec4 &tint, idVec3 &size, idVec3 &rotate );
	virtual		bool			Render( const rvBSE *effect, rvParticleTemplate *pt, const idMat3 &view, srfTriangles_t* tri, float time, float override = 1.0f );
	virtual		void			SetModel( const idRenderModel *model ) { mModel = model; }

	virtual		void			AttenuateSize( float atten, rvParticleParms &parms ) { Attenuate( atten, parms, mSizeEnv ); }

private:
				// Fixed at spawn time
				rvEnvParms3Particle		mSizeEnv;
				rvEnvParms3Particle		mRotationEnv;

				const idRenderModel	*mModel;
};

class rvLightParticle : public rvParticle
{
public:
	friend		class			rvParticleTemplate;

								rvLightParticle( void ) { mLightDefHandle = -1; }
								~rvLightParticle( void ) { Destroy(); }

	virtual		rvParticle		*GetArrayEntry( int i ) const;
	virtual		int				GetArrayIndex( rvParticle *p ) const;

	virtual		void			EvaluateSize( rvEnvParms *size, const float time, float oneOverDuration, float *dest ) {// { mSizeEnv.Evaluate( time, dest ); }
		size->Evaluate( mSizeEnv, time, oneOverDuration, dest );
	}
	virtual		void			EvaluateLength( rvEnvParms *length, const float time, float oneOverDuration, idVec3 &dest ) {}

	virtual		void			InitSizeEnv( rvEnvParms &env, float duration ) { mSizeEnv.Init( env, duration ); }
	virtual		float			*GetInitSize( void ) { return( mSizeEnv.GetStart() ); }
	virtual		float			*GetDestSize( void ) { return( mSizeEnv.GetEnd() ); }

	virtual		void			InitRotationEnv( rvEnvParms &env, float duration ) {}
	virtual		float			*GetInitRotation( void ) { return( NULL ); }
	virtual		float			*GetDestRotation( void ) { return( NULL ); }

	virtual		void			AttenuateSize( float atten, rvParticleParms &parms ) { Attenuate( atten, parms, mSizeEnv ); }

	virtual		void			GetSpawnInfo( idVec4 &tint, idVec3 &size, idVec3 &rotate );
	virtual		bool			InitLight( rvBSE *effect, rvSegmentTemplate *st, float time );
	virtual		bool			PresentLight( rvBSE *effect, rvParticleTemplate *pt, float time, bool infinite );
	virtual		bool			Destroy( void );

private:
				// Fixed at spawn time
				rvEnvParms3Particle		mSizeEnv;

				// Alterable
				qhandle_t		mLightDefHandle;
				renderLight_t	mLight;
};

class rvLinkedParticle : public rvParticle
{
public:
	friend		class			rvParticleTemplate;

	virtual		rvParticle		*GetArrayEntry( int i ) const;
	virtual		int				GetArrayIndex( rvParticle *p ) const;

	virtual		void			EvaluateSize( rvEnvParms *size, const float time, float oneOverDuration, float *dest ) {// { mSizeEnv.Evaluate( time, dest ); }
		size->Evaluate( mSizeEnv, time, oneOverDuration, dest );
	}
	virtual		void			EvaluateLength( rvEnvParms *length, const float time, float oneOverDuration, idVec3 &dest ) {}

	virtual		void			InitSizeEnv( rvEnvParms &env, float duration ) { mSizeEnv.Init( env, duration ); }
	virtual		float			*GetInitSize( void ) { return( mSizeEnv.GetStart() ); }
	virtual		float			*GetDestSize( void ) { return( mSizeEnv.GetEnd() ); }

	virtual		void			InitRotationEnv( rvEnvParms &env, float duration ) {}
	virtual		float			*GetInitRotation( void ) { return( NULL ); }
	virtual		float			*GetDestRotation( void ) { return( NULL ); }

	virtual		void			AttenuateSize( float atten, rvParticleParms &parms ) { Attenuate( atten, parms, mSizeEnv ); }

	virtual		void			FinishSpawn( rvBSE *effect, rvSegment *segment, float birthTime, float fraction = 0.0f, const idVec3 &initOffset = vec3_origin, const idMat3 &initAxis = mat3_identity );
	virtual		void			GetSpawnInfo( idVec4 &tint, idVec3 &size, idVec3 &rotate ) {}
	virtual		void			HandleTiling( rvParticleTemplate *pt );
	virtual		bool			Render( const rvBSE *effect, rvParticleTemplate *pt, const idMat3 &view, srfTriangles_t* tri, float time, float override = 1.0f );

private:
				// Fixed at spawn time
				rvEnvParms1Particle		mSizeEnv;
};

class sdOrientedLinkedParticle : public rvLinkedParticle
{
public:
	virtual		bool			Render( const rvBSE *effect, rvParticleTemplate *pt, const idMat3 &view, srfTriangles_t* tri, float time, float override = 1.0f );
};

class rvDebrisParticle : public rvParticle
{
public:
	friend		class			rvParticleTemplate;

	virtual		rvParticle		*GetArrayEntry( int i ) const;
	virtual		int				GetArrayIndex( rvParticle *p ) const;

	virtual		void			EvaluateRotation( rvEnvParms *rotation, const float time, float oneOverDuration, float *dest ) {// { mRotationEnv.Evaluate( time, dest ); }
		rotation->Evaluate( mRotationEnv, time, oneOverDuration, dest );
	}

	virtual		void			InitRotationEnv( rvEnvParms &env, float duration ) { mRotationEnv.Init( env, duration ); }
	virtual		float			*GetInitRotation( void ) { return( mRotationEnv.GetStart() ); }
	virtual		float			*GetDestRotation( void ) { return( mRotationEnv.GetEnd() ); }

	virtual		void			ScaleRotation( float constant ) { mRotationEnv.Scale( constant ); }
	virtual		void			FinishSpawn( rvBSE *effect, rvSegment *segment, float birthTime, float fraction = 0.0f, const idVec3 &initOffset = vec3_origin, const idMat3 &initAxis = mat3_identity );

private:
				// Fixed at spawn time
				rvEnvParms3Particle		mRotationEnv;
};

// ================================================================================================

struct rvTrailInfo {
	short					mTrailType;
	byte					mStatic;
	byte					mPad;
	idStr					mTrailTypeName;
	const idMaterial		*mTrailMaterial;
	idVec2					mTrailTime;								// Length of trial in seconds
	idVec2					mTrailCount;							// Number of particles in trail
	float					mTrailScale;							// Width of the motion trails will be particleSize scaled by this

	rvTrailInfo() : mStatic(0) {
	}
};

struct rvElectricityInfo {
	int						mNumForks;								// Number of forks for 
	byte					mStatic;
	byte					mPad;
	idVec3					mForkSizeMins;
	idVec3					mForkSizeMaxs;
	idVec3					mJitterSize;							// Amount of jitter for the electricity
	float					mJitterRate;
	const idDeclTable		*mJitterTable;							// The envelope for the jitter in the lightning bolt

	rvElectricityInfo() : mStatic(0) {
	}
};

class rvParticleTemplate
{
public:
	friend		class				rvParticle;
	friend		class				rvSpriteParticle;
	friend		class				rvLineParticle;
	friend		class				rvOrientedParticle;
	friend		class				rvElectricityParticle;
	friend		class				rvModelParticle;
	friend		class				rvLightParticle;
	friend		class				rvDebrisParticle;
	friend		class				rvParticleTemplateWrapper;
	friend		class				rvLinkedParticle;
	friend		class				sdOrientedLinkedParticle;
	friend      class rvSegmentTemplate;

									rvParticleTemplate( void ) : mFlags(0) { }
									~rvParticleTemplate( void ) {}

				bool				operator== ( const rvParticleTemplate& a ) const { return( Compare( a ) ); }
				bool				operator!= ( const rvParticleTemplate& a ) const { return( !Compare( a ) ); }
				rvParticleTemplate& operator=(const rvParticleTemplate& __that);

				int					GetFlags(void) { return mFlags; }
				void				SetFlag( bool on, int flag ) { on ? mFlags |= flag : mFlags &= ~flag; }
				bool				GetFlag( int flag ) const { return ( mFlags & flag ) != 0; }
				bool				GetParsed( void ) const { return( !!( mFlags & PTFLAG_PARSED ) ); }
				bool				GetHasPhysics( void ) const { return( !!( mFlags & PTFLAG_HAS_PHYSICS ) ); }
				bool				GetHasLineHit( void ) const { return( !!( mFlags & PTFLAG_HAS_LINEHIT ) ); }
				bool				GetDeleteOnImpact( void ) const { return( !!( mFlags & PTFLAG_DELETE_ON_IMPACT ) ); }
				bool				GetGeneratedNormal( void ) const { return( !!( mFlags & PTFLAG_GENERATED_NORMAL ) ); }
				bool				GetGeneratedOriginNormal( void ) const { return( !!( mFlags & PTFLAG_GENERATED_ORG_NORMAL ) ); }
				bool				GetFlippedNormal( void ) const { return( !!( mFlags & PTFLAG_FLIPPED_NORMAL ) ); }
				bool				GetCalculatedNormal( void ) const { return( !!( mFlags & PTFLAG_CALCED_NORMAL ) ); }
				bool				GetAdditive( void ) const { return( !!( mFlags & PTFLAG_ADDITIVE ) ); }
				bool				GetGeneratedLine( void ) const { return( !!( mFlags & PTFLAG_GENERATED_LINE ) ); }
				bool				GetShadows( void ) const { return( !!( mFlags & PTFLAG_SHADOWS ) ); }
				bool				GetSpecular( void ) const { return( !!( mFlags & PTFLAG_SPECULAR ) ); }
				bool				GetLinked( void ) const { return( !!( mFlags & PTFLAG_LINKED ) ); }
				bool				GetTiled( void ) const { return( !!( mFlags & PTFLAG_TILED ) ); }
				bool				GetPersist( void ) const { return( !!( mFlags & PTFLAG_PERSIST ) ); }
				bool				GetParentVelocity( void ) const { return( !!( mFlags & PTFLAG_PARENTVEL ) ); }

				void				SetParsed( bool parsed ) { SetFlag( parsed, PTFLAG_PARSED ); }
				void				SetHasPhysics( bool hasPhysics ) { SetFlag( hasPhysics, PTFLAG_HAS_PHYSICS ); }
				void				SetHasLineHit( bool hasLH ) { SetFlag( hasLH, PTFLAG_HAS_LINEHIT ); }
				void				SetDeleteOnImpact( bool deleteOnImpact ) { SetFlag( deleteOnImpact, PTFLAG_DELETE_ON_IMPACT ); }
				void				SetGeneratedNormal( bool generatedNormal ) { SetFlag( generatedNormal, PTFLAG_GENERATED_NORMAL ); }
				void				SetGeneratedOriginNormal( bool generatedNormal ) { SetFlag( generatedNormal, PTFLAG_GENERATED_ORG_NORMAL ); }
				void				SetFlippedNormal( bool flippedNormal ) { SetFlag( flippedNormal, PTFLAG_FLIPPED_NORMAL ); }
				void				SetCalculatedNormal( bool calcedNormal ) { SetFlag( calcedNormal, PTFLAG_CALCED_NORMAL ); }
				void				SetAdditive( bool additive ) { SetFlag( additive, PTFLAG_ADDITIVE ); }
				void				SetGeneratedLine( bool generatedLine ) { SetFlag( generatedLine, PTFLAG_GENERATED_LINE ); }
				void				SetShadows( bool shadows ) { SetFlag( shadows, PTFLAG_SHADOWS ); }
				void				SetSpecular( bool specular ) { SetFlag( specular, PTFLAG_SPECULAR ); }
				void				SetLinked( bool linked ) { SetFlag( linked, PTFLAG_LINKED ); }
				void				SetTiled( bool tiled ) { SetFlag( tiled, PTFLAG_TILED ); }
				void				SetPersist( bool tiled ) { SetFlag( tiled, PTFLAG_PERSIST ); }
				void				SetParentVelocity( bool pv ) { SetFlag( pv, PTFLAG_PARENTVEL ); }

				void				SetType( int type ) { mType = type; }
				int					GetType( void ) const { return( mType ); }

				const idMaterial	*GetMaterial( void ) const { return( mMaterial ); }
				void				SetMaterial( idMaterial *material ) { mMaterial = material; }

				idRenderModel		*GetModel( void ) const { return mModel; }
				idTraceModel		*GetTraceModel( void ) const;

				float				GetPhysicsDistance() const { return mPhysicsDistance; }

				const char			*GetEntityDefName( void ) const { return( mEntityDefName ); }

				float				GetGravity( void ) const { return( rvRandom::flrand( mGravity[0], mGravity[1] ) ); }
				float				GetTiling( void ) const { return( mTiling ); }
				int					GetTrailType( void ) const { return( mTrailInfo->mTrailType ); }
				const idMaterial	*GetTrailMaterial( void ) const { return( mTrailInfo->mTrailMaterial ); }
				float				GetTrailTime( void ) const { return( rvRandom::flrand( mTrailInfo->mTrailTime[0], mTrailInfo->mTrailTime[1] ) ); }
				float				GetMaxTrailTime( void ) const { return( mTrailInfo->mTrailTime[1] ); }
				int					GetTrailCount( void ) const;
				int					GetMaxTrailCount( void ) const { return( ( int )ceilf( mTrailInfo->mTrailCount[1] ) + 1 ); }
				float				GetDuration( void ) const { return( rvRandom::flrand( mDuration[0], mDuration[1] ) ); }
				float				GetMaxDuration( void ) const { return( mDuration[1] ); }
				int					GetNumTimeoutEffects( void ) const { return( mNumTimeoutEffects ); }
				bool				HasTrail( void ) const { return( mTrailInfo->mTrailType && !mTrailInfo->mTrailTypeName.IsEmpty() ); }
				int					GetTrailRepeat( void ) { return mTrailRepeat; }
				float				GetWindDeviationAngle( void ) { return mWindDeviationAngle; }
				int					GetNumFrames( void ) { return mNumFrames; }

				int					GetVertexCount( void ) const { return( mVertexCount ); }
				int					GetIndexCount( void ) const { return( mIndexCount ); }

				float				GetMaxParmValue( rvParticleParms &spawn, rvParticleParms &death, rvEnvParms &envelope );
				float				GetMaxSize( void );
				float				GetMaxOffset( void );
				float				GetMaxLength( void );
				void				EvaluateSimplePosition( idVec3 &pos, float time, float lifeTime, idVec3 &initPos, idVec3 &velocity, idVec3 &acceleration, idVec3 &friction );
				float				GetFurthestDistance( void );
				float				CostTrail( float cost ) const;
				const idStr			&GetTrailTypeName( void ) const { return( mTrailInfo->mTrailTypeName ); }
				float				GetTrailScale( void ) const { return mTrailInfo->mTrailScale; }
				float				GetSpawnVolume( rvBSE *effect );
				bool				UsesEndOrigin( void );
				bool				GetVector( idParser *src, int count, idVec3 &result );
				void				SetParameterCounts( void );
				rvEnvParms*			ParseMotionParms( idParser *src, int count, rvEnvParms *def );
				rvParticleParms*	ParseSpawnParms( rvDeclEffect *effect, idParser *src, int count, rvParticleParms *def );
				bool				ParseMotionDomains( rvDeclEffect *effect, idParser *src );
				bool				CheckCommonParms( idParser *src, rvParticleParms &parms );
				bool				ParseSpawnDomains( rvDeclEffect *effect, idParser *src );
				bool				ParseDeathDomains( rvDeclEffect *effect, idParser *src );
				bool				ParseImpact( rvDeclEffect *effect, idParser *src );
				bool				ParseTimeout( rvDeclEffect *effect, idParser *src );
				bool				ParseBlendParms( rvDeclEffect *effect, idParser *src );
				bool				Parse( rvDeclEffect *effect, idParser *src );
				void				Init( void );
				void				Finish( void );
				void				Purge( void );
				void				PurgeTraceModel( void );

				void				MakeEditable( void );
				static void			ShutdownStatic( void );
				void				Duplicate( rvParticleTemplate const &copy );
private:
			bool					Compare( const rvParticleTemplate& a ) const;
			void					FixupParms( rvParticleParms *parms );
			void					AllocTrail( void );
			void					AllocElectricityInfo( void );

			int						mFlags;
			int						mTraceModelIndex;
			int						mType;									// Type of particle

			const idMaterial		*mMaterial;
			idRenderModel			*mModel;

			//idStr					mModelName;
			idStr					mEntityDefName;

			idVec2					mGravity;
			idVec2					mDuration;
			idVec3					mCentre;								// Centre of bounds for normal generation

			float					mTiling;								// Multiplier for texcoords
			float					mBounce;
			float					mPhysicsDistance;
			float					mWindDeviationAngle;

			short					mVertexCount;
			short					mIndexCount;
			
			byte					mTrailRepeat;
			byte					mNumSizeParms;
			byte					mNumRotateParms;
			byte					mNumFrames;

			rvTrailInfo				*mTrailInfo;

			rvElectricityInfo		*mElecInfo;
			// Spawn info
			rvParticleParms			*mpSpawnPosition;
			rvParticleParms			*mpSpawnDirection;
			rvParticleParms			*mpSpawnVelocity;
			rvParticleParms			*mpSpawnAcceleration;
			rvParticleParms			*mpSpawnFriction;
			rvParticleParms			*mpSpawnTint;
			rvParticleParms			*mpSpawnFade;
			rvParticleParms			*mpSpawnSize;
			rvParticleParms			*mpSpawnRotate;
			rvParticleParms			*mpSpawnAngle;
			rvParticleParms			*mpSpawnOffset;
			rvParticleParms			*mpSpawnLength;
			rvParticleParms			*mpSpawnWindStrength;

			// Motion info
			rvEnvParms				*mpTintEnvelope;
			rvEnvParms				*mpFadeEnvelope;
			rvEnvParms				*mpSizeEnvelope;
			rvEnvParms				*mpRotateEnvelope;
			rvEnvParms				*mpAngleEnvelope;
			rvEnvParms				*mpOffsetEnvelope;
			rvEnvParms				*mpLengthEnvelope;

			// Death (end condition) info
			rvParticleParms			*mpDeathTint;
			rvParticleParms			*mpDeathFade;
			rvParticleParms			*mpDeathSize;
			rvParticleParms			*mpDeathRotate;
			rvParticleParms			*mpDeathAngle;
			rvParticleParms			*mpDeathOffset;
			rvParticleParms			*mpDeathLength;

			// Misc info
			int						mNumImpactEffects;
			const rvDeclEffect		*mImpactEffects[BSE_NUM_SPAWNABLE];
			int						mNumTimeoutEffects;
			const rvDeclEffect		*mTimeoutEffects[BSE_NUM_SPAWNABLE];

			static rvTrailInfo		sTrailInfo;
			static rvElectricityInfo sElectricityInfo;

			// Motion info
			static rvEnvParms		sDefaultEnvelope;
			static rvEnvParms		sEmptyEnvelope;

			static rvParticleParms	sSPF_ONE_1;
			static rvParticleParms	sSPF_ONE_2;
			static rvParticleParms	sSPF_ONE_3;
			static rvParticleParms	sSPF_NONE_0;
			static rvParticleParms	sSPF_NONE_1;
			static rvParticleParms	sSPF_NONE_3;

			static bool sInited;
			static void InitStatic( void );
};

#endif //_BSE_PARTICLE_H_INC_

