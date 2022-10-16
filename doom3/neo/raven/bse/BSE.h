// BSE.h
//

#pragma once

#ifndef _RAVEN_BSE_H
#define _RAVEN_BSE_H

// BSE - Basic System for Effects

// Notes:
// All times are floats of seconds
// All tints are floats of 0.0 to 1.0
// All effects are presumed to be in the "base/effects" folder and have the extension of BSE_EFFECT_EXTENSION
// All angles are in fractions of a circle - 1 means 1 full circle
// All effect files are case insensitive
// Will not have different shaders to be randomly chosen - need to keep the tesses to a minimum (except possibly for decals)

// Defined classes
class rvSegment;
class rvSegmentTemplate;
class rvDeclEffect;
class rvBSE;
class rvBSEManagerLocal;

// Referenced classes
class rvRenderModelBSE;
class idRenderModel;
class rvParticle;
class idPlayerView;

const float WORLD_SIZE = (128.0f * 1024.0f);
const float BSE_LARGEST = (512.0f);
const float BSE_TESS_COST = (20.0f);				// The expense of a new tess
const float BSE_PHYSICS_COST = (80.0f);				// The expense of 1 particle having physics

const float BSE_PARTICLE_TEXCOORDSCALE = (0.01f);



const unsigned int MEMORY_BLOCK_SIZE = (0x100000);
const unsigned int BSE_ELEC_MAX_BOLTS = (200);

typedef enum eBSEPerfCounter
{
	PERF_NUM_BSE,
	PERF_NUM_TRACES,
	PERF_NUM_PARTICLES,
	PERF_NUM_TEXELS,
	PERF_NUM_SEGMENTS,
	NUM_PERF_COUNTERS
};

typedef enum eBSESegment
{
	SEG_NONE = 0,
	SEG_EFFECT,						// Spawns another effect inheriting data from owner
	SEG_EMITTER,					// Spawns particles at a rate
	SEG_SPAWNER,					// Spawns particles instantly
	SEG_TRAIL,						// Leaves a trail of particles
	SEG_SOUND,						// Plays a sound
	SEG_DECAL,						// Leaves an idDecal
	SEG_LIGHT,						// Displays a 3D light
	SEG_DELAY,						// A control segment for looping
	SEG_SHAKE,						// Triggers a screen shake
	SEG_TUNNEL,						// Triggers the id tunnel vision effect
	SEG_COUNT
};

typedef enum eBSETrail
{
	TRAIL_NONE = 0,
	TRAIL_BURN,
	TRAIL_MOTION,
	TRAIL_PARTICLE,
	TRAIL_COUNT
};

enum {
	SFLAG_EXPIRED = BITT< 0 >::VALUE,
	SFLAG_SOUNDPLAYING = BITT< 1 >::VALUE,
	SFLAG_HASMOTIONTRAIL = BITT< 2 >::VALUE,
};

// ==============================================
// Effect class
// ==============================================

class rvSegment
{
public:
	friend	class			rvBSE;
	rvSegment(void) { mFlags = 0; mParticles = NULL; mUsedHead = NULL; mFreeHead = NULL; mParticleCount = 0; mLoopParticleCount = 0; }
	~rvSegment(void);

	void					SetFlag(bool on, int flag) { on ? mFlags |= flag : mFlags &= ~flag; }

	bool					GetExpired(void) const { return((bool)(mFlags & SFLAG_EXPIRED)); }
	bool					GetSoundPlaying(void) const { return(!!(mFlags & SFLAG_SOUNDPLAYING)); }
	bool					GetHasMotionTrail(void) const { return(!!(mFlags & SFLAG_HASMOTIONTRAIL)); }

	void					SetExpired(bool expired) { SetFlag(expired, SFLAG_EXPIRED); }
	void					SetSoundPlaying(bool soundPlaying) { SetFlag(soundPlaying, SFLAG_SOUNDPLAYING); }
	void					SetHasMotionTrail(bool hmt) { SetFlag(hmt, SFLAG_HASMOTIONTRAIL); }

	int						GetMotionTrailCount(void) { return(mActiveCount); }

	rvSegmentTemplate* GetSegmentTemplate(void);
	const		rvDeclEffect* GetEffectDecl(void) { return(mEffectDecl); }

	bool					GetLocked(void);
	bool					Active(void);

	void					PlayEffect(rvBSE* effect, rvSegmentTemplate* st, float depthOffset);
	void					InitTime(rvBSE* effect, rvSegmentTemplate* st, float time);
	void					ResetTime(rvBSE* effect, float time);
	void					Init(rvBSE* effect, const rvDeclEffect* decl, int segmentTemplateHandle, float time);
	void					InitParticles(rvBSE* effect);
	void					RefreshParticles(rvBSE* effect, rvSegmentTemplate* st);
	bool					Check(rvBSE* effect, float time, float offset);
	void					Handle(rvBSE* effect, float time);
	void					UpdateGenericParticles(rvBSE* effect, rvSegmentTemplate* st, float time);
	void					UpdateSimpleParticles(float time);
	bool					UpdateParticles(rvBSE* effect, float time);
	float					AttenuateDuration(rvBSE* effect, rvSegmentTemplate* st);
	float					AttenuateInterval(rvBSE* effect, rvSegmentTemplate* st);
	float					AttenuateCount(rvBSE* effect, rvSegmentTemplate* st, float min, float max);
	void					ValidateSpawnRates(void);
	void					AddToParticleCount(rvBSE* effect, int count, int loopCount, float duration);
	void					GetSecondsPerParticle(rvBSE* effect, rvSegmentTemplate* st, rvParticleTemplate* pt);
	void					CalcTrailCounts(rvBSE* effect, rvSegmentTemplate* st, rvParticleTemplate* pt, float duration);
	void					CalcCounts(rvBSE* effect, float time);
	void					EmitSmokeParticles(rvBSE* effect, rvSegmentTemplate* st, rvParticle* particle, float time);
	//		soundChannel_t			GetSoundChannel( void ) { return static_cast< soundChannel_t >( mSegmentTemplateHandle ) + SCHANNEL_ONE; }

	void					CreateDecal(rvBSE* effect, float time);
	void					InitLight(rvBSE* effect, rvSegmentTemplate* st, float time);
	bool					HandleLight(rvBSE* effect, rvSegmentTemplate* st, float time);

	rvParticle* InitParticleArray(rvBSE* effect);
	rvParticle* GetFreeParticle(rvBSE* effect);
	rvParticle* SpawnParticle(rvBSE* effect, rvSegmentTemplate* st, float birthTime, const idVec3& initPos = vec3_origin, const idMat3& initAxis = mat3_identity);
	void					SpawnParticles(rvBSE* effect, rvSegmentTemplate* st, float birthTime, int count);

	void					AllocateSurface(rvBSE* effect, idRenderModel* model);
	void					ClearSurface(rvBSE* effect, idRenderModel* model);
	void					RenderMotion(rvBSE* effect, const struct renderEffect_s* owner, idRenderModel* model, rvParticleTemplate* pt, float time);
	void					RenderTrail(rvBSE* effect, const struct renderEffect_s* owner, idRenderModel* model, float time);
	void					Render(rvBSE* effect, const struct renderEffect_s* owner, idRenderModel* model, float time);
protected:
	void					Sort(const idVec3& eyePos);

	// Fixed at spawn time
	int						mSegmentTemplateHandle;
	int						mParticleType;
	int						mSurfaceIndex;					// Index of the model surface that this segment uses
	const		rvDeclEffect* mEffectDecl;

	float					mSegStartTime;					// Start time of segment
	float					mSegEndTime;

	idVec2					mSecondsPerParticle;			// How quickly the particles are spawned in an emitter
	idVec2					mCount;							// The count of particles from a spawner

	int						mParticleCount;					// For getting the amount of memory to alloc for tris data
	int						mLoopParticleCount;

	float					mSoundVolume;
	float					mFreqShift;

	// Dynamically altered during effect
	int						mFlags;
	float					mLastTime;						// Last time the segment was serviced
	int						mActiveCount;					// Number of active particles 

	rvParticle* mFreeHead;						// Linked list of particles
	rvParticle* mUsedHead;
	rvParticle* mParticles;
};

enum {
	STFLAG_ENABLED = BITT< 0 >::VALUE,
	STFLAG_LOCKED = BITT< 1 >::VALUE,
	STFLAG_HASPARTICLES = BITT< 2 >::VALUE,
	STFLAG_HASPHYSICS = BITT< 3 >::VALUE,
	STFLAG_IGNORE_DURATION = BITT< 4 >::VALUE,
	STFLAG_INFINITE_DURATION = BITT< 5 >::VALUE,
	STFLAG_ATTENUATE_EMITTER = BITT< 6 >::VALUE,
	STFLAG_INVERSE_ATTENUATE = BITT< 7 >::VALUE,
	STFLAG_TEMPORARY = BITT< 8 >::VALUE,
	STFLAG_USEMATCOLOR = BITT< 9 >::VALUE,
	STFLAG_DEPTH_SORT = BITT< 10 >::VALUE,
	STFLAG_INVERSE_DRAWORDER = BITT< 11 >::VALUE,
	STFLAG_ORIENTATE_IDENTITY = BITT< 12 >::VALUE,
	STFLAG_COMPLEX = BITT< 13 >::VALUE,
	STFLAG_CALCULATE_DURATION = BITT< 14 >::VALUE,
};

class rvSegmentTemplate
{
public:
	friend	class			rvSegment;
	friend	class			rvBSE;
	friend	class			rvSegmentTemplateWrapper;

	rvSegmentTemplate(void) { Init(NULL); SetEnabled(true); }
	~rvSegmentTemplate(void) {}

	// Support copying of a template
	rvSegmentTemplate(const rvSegmentTemplate& copy) { *this = copy; }

	void					operator = (const rvSegmentTemplate& copy);
	bool					operator == (const rvSegmentTemplate& a) const { return Compare(a); }
	bool					operator != (const rvSegmentTemplate& a) const { return !Compare(a); }

	void					SetFlag(bool on, int flag) { on ? mFlags |= flag : mFlags &= ~flag; }

	bool					GetEnabled(void) const { return(!!(mFlags & STFLAG_ENABLED)); }
	bool					GetLocked(void) const { return(!!(mFlags & STFLAG_LOCKED)); }
	bool					GetHasParticles(void) const { return(!!(mFlags & STFLAG_HASPARTICLES)); }
	bool					GetHasPhysics(void) const { return(!!(mFlags & STFLAG_HASPHYSICS)); }
	bool					GetIgnoreDuration(void) const { return(!!(mFlags & STFLAG_IGNORE_DURATION)); }
	bool					GetInfiniteDuration(void) const { return(!!(mFlags & STFLAG_INFINITE_DURATION)); }
	bool					GetAttenuateEmitter(void) const { return(!!(mFlags & STFLAG_ATTENUATE_EMITTER)); }
	bool					GetInverseAttenuate(void) const { return(!!(mFlags & STFLAG_INVERSE_ATTENUATE)); }
	bool					GetUseMaterialColor(void) const { return(!!(mFlags & STFLAG_USEMATCOLOR)); }
	bool					GetTemporary(void) const { return(!!(mFlags & STFLAG_TEMPORARY)); }
	bool					GetDepthSort(void) const { return(!!(mFlags & STFLAG_DEPTH_SORT)); }
	bool					GetInverseDrawOrder(void) const { return(!!(mFlags & STFLAG_INVERSE_DRAWORDER)); }
	bool					GetOrientateIdentity(void) const { return(!!(mFlags & STFLAG_ORIENTATE_IDENTITY)); }
	bool					GetComplexParticle(void) const { return(!!(mFlags & STFLAG_COMPLEX)); }
	bool					GetCalculateDuration(void) const { return(!!(mFlags & STFLAG_CALCULATE_DURATION)); }

	void					SetEnabled(bool enabled) { SetFlag(enabled, STFLAG_ENABLED); }
	void					SetLocked(bool locked) { SetFlag(locked, STFLAG_LOCKED); }
	void					SetHasParticles(bool hasParticles) { SetFlag(hasParticles, STFLAG_HASPARTICLES); }
	void					SetHasPhysics(bool hasPhysics) { SetFlag(hasPhysics, STFLAG_HASPHYSICS); }
	void					SetIgnoreDuration(bool ignoreDuration) { SetFlag(ignoreDuration, STFLAG_IGNORE_DURATION); }
	void					SetInfiniteDuration(bool infiniteDuration) { SetFlag(infiniteDuration, STFLAG_INFINITE_DURATION); }
	void					SetAttenuateEmitter(bool attenuate) { SetFlag(attenuate, STFLAG_ATTENUATE_EMITTER); }
	void					SetInverseAttenuate(bool attenuate) { SetFlag(attenuate, STFLAG_INVERSE_ATTENUATE); }
	void					SetUseMaterialColor(bool attenuate) { SetFlag(attenuate, STFLAG_USEMATCOLOR); }
	void					SetTemporary(bool persistent) { SetFlag(persistent, STFLAG_TEMPORARY); }
	void					SetDepthSort(bool locked) { SetFlag(locked, STFLAG_DEPTH_SORT); }
	void					SetInverseDrawOrder(bool inverseDrawOrder) { SetFlag(inverseDrawOrder, STFLAG_INVERSE_DRAWORDER); }
	void					SetOrientateIdentity(bool orientateIdentity) { SetFlag(orientateIdentity, STFLAG_ORIENTATE_IDENTITY); }
	void					SetComplexParticle(bool enabled) { SetFlag(enabled, STFLAG_COMPLEX); }
	void					SetCalculateDuration(bool calculateDuration) { SetFlag(calculateDuration, STFLAG_CALCULATE_DURATION); }

	const idStr& GetSegmentName(void) const { return(mSegmentName); }

	const rvDeclEffect* GetEffectDecl(void) { return(mDeclEffect); }

	void					EvaluateTrailSegment(rvDeclEffect* et);
	int						GetTrailSegmentIndex(void) const { return(mTrailSegmentIndex); }

	int						GetType(void) const { return(mSegType); }
	bool					GetSmoker(void);
	bool					Parse(rvDeclEffect* et, int segmentType, idParser* lexer);
	void					Init(rvDeclEffect* decl);
	float					GetStartTime(void) const { return(rvRandom::flrand(mLocalStartTime[0], mLocalStartTime[1])); }
	float					GetDuration(void) const { return(rvRandom::flrand(mLocalDuration[0], mLocalDuration[1])); }
	void					SetMinDuration(rvDeclEffect* effect);
	void					SetMaxDuration(rvDeclEffect* effect);
	bool					GetSoundLooping(void);
	float					GetSoundVolume(void) const { return(rvRandom::flrand(mSoundVolume[0], mSoundVolume[1])); }
	float					GetFreqShift(void) const { return(rvRandom::flrand(mFreqShift[0], mFreqShift[1])); }
	float					GetDensity(void) const { return(rvRandom::flrand(mDensity[0], mDensity[1])); }
	float					GetMaxDensity(void) const { return(mDensity[1]); }
	float					GetParticleCap(void) const { return(mParticleCap); }
	float					GetMaxCount(void) const { return(mCount[1]); }
	int						GetTexelCount(void);
	bool					Finish(rvDeclEffect* et);
	void					CreateParticleTemplate(rvDeclEffect* et, idParser* lexer, int particleType);
	rvParticleTemplate* GetParticleTemplate(void) { return(&mParticleTemplate); }
	const rvParticleTemplate* GetParticleTemplate(void) const { return(&mParticleTemplate); }
	float					CalculateBounds(void);
	bool					DetailCull(void) const;

	float					EvaluateCost(int activeParticles) const;

	void					Duplicate(const rvSegmentTemplate& copy);
private:
	bool					Compare(const rvSegmentTemplate& a) const;

	rvDeclEffect* mDeclEffect;

	// Common parms
	idStr					mSegmentName;
	int						mFlags;
	int						mSegType;						// SEG_ enum
	idVec2					mLocalStartTime;				// Start time of segment wrt effect
	idVec2					mLocalDuration;					// Min and max duration
	idVec2					mAttenuation;					// How effect fades off to the distance
	float					mParticleCap;
	float					mScale;
	float					mDetail;						// bse_scale value that culls out this segment

	// Emitter parms
	rvParticleTemplate		mParticleTemplate;
	idVec2					mCount;							// The count of particles from a spawner
	idVec2					mDensity;						// Sets count or rate based on volume, area or length
	int						mTrailSegmentIndex;				// Segment containing the trail info

	int						mNumEffects;
	const rvDeclEffect* mEffects[BSE_NUM_SPAWNABLE];	// Effect to play on certain conditions

	const		idSoundShader* mSoundShader;
	idVec2					mSoundVolume;					// Starting volume of sound in decibels
	idVec2					mFreqShift;						// Frequency shift of sound

	int						mDecalAxis;						// Axis to project decals along

	static		float					mSegmentBaseCosts[SEG_COUNT];
};

// ==============================================
// ==============================================

enum {
	ETFLAG_HAS_SOUND = BITT< 0 >::VALUE,
	ETFLAG_USES_ENDORIGIN = BITT< 1 >::VALUE,
	ETFLAG_ATTENUATES = BITT< 2 >::VALUE,
	ETFLAG_EDITOR_MODIFIED = BITT< 3 >::VALUE,
	ETFLAG_USES_MATERIAL_COLOR = BITT< 4 >::VALUE,
	ETFLAG_ORIENTATE_IDENTITY = BITT< 5 >::VALUE,
	ETFLAG_USES_AMBIENT_CUBEMAP = BITT< 6 >::VALUE,
	ETFLAG_HAS_PHYSICS = BITT< 7 >::VALUE,
};

class rvDeclEffect : public idDecl
{
public:
	friend	class			rvBSE;
	friend	class			rvEffectTemplateWrapper;
	friend  class			rvBSEManagerLocal;

	rvDeclEffect(void) { Init(); }
	rvDeclEffect(const rvDeclEffect& copy) { Init(); *this = copy; }
	virtual 						~rvDeclEffect(void) { }

	bool					operator== (const rvDeclEffect& comp) const { return Compare(comp); }
	bool					operator!= (const rvDeclEffect& comp) const { return !Compare(comp); }

	rvDeclEffect& operator= (const rvDeclEffect& copy);

	virtual bool					SetDefaultText(void);
	virtual const char* DefaultDefinition(void) const;
	virtual bool					Parse(const char* text, const int textLength);
	virtual void					FreeData(void);
	virtual size_t					Size(void) const;

	static	void					CacheFromDict(const idDict& dict);

	void					SetFlag(bool on, int flag) { on ? mFlags |= flag : mFlags &= ~flag; }

	bool					GetEditorModified(void) const { return(!!(mFlags & ETFLAG_EDITOR_MODIFIED)); }
	bool					GetHasSound(void) const { return(!!(mFlags & ETFLAG_HAS_SOUND)); }
	bool					GetHasPhysics(void) const { return(!!(mFlags & ETFLAG_HAS_PHYSICS)); }
	bool					GetUsesEndOrigin(void) const { return(!!(mFlags & ETFLAG_USES_ENDORIGIN)); }
	bool					GetAttenuates(void) const { return(!!(mFlags & ETFLAG_ATTENUATES)); }
	bool					GetOrientateIdentity(void) const { return(!!(mFlags & ETFLAG_ORIENTATE_IDENTITY)); }
	bool					GetUsesAmbientCubeMap(void) const { return(!!(mFlags & ETFLAG_USES_AMBIENT_CUBEMAP)); }

	void					SetEditorModified(bool modified) { base->EnsureNotPurged(); SetFlag(modified, ETFLAG_EDITOR_MODIFIED); }
	void					SetHasSound(bool hasSound) { SetFlag(hasSound, ETFLAG_HAS_SOUND); }
	void					SetHasPhysics(bool hasPhysics) { SetFlag(hasPhysics, ETFLAG_HAS_PHYSICS); }
	void					SetUsesEndOrigin(bool usesEndOrigin) { SetFlag(usesEndOrigin, ETFLAG_USES_ENDORIGIN); }
	void					SetAttenuates(bool attenuates) { SetFlag(attenuates, ETFLAG_ATTENUATES); }
	void					SetOrientateIdentity(bool orientateIdentity) { SetFlag(orientateIdentity, ETFLAG_ORIENTATE_IDENTITY); }
	void					SetUsesAmbientCubeMap(bool usesAmbientCubeMap) { SetFlag(usesAmbientCubeMap, ETFLAG_USES_AMBIENT_CUBEMAP); }

	rvSegmentTemplate* GetSegmentTemplate(int i) { return(&mSegmentTemplates[i]); }
	const rvSegmentTemplate* GetSegmentTemplate(int i) const { return(&mSegmentTemplates[i]); }
	rvSegmentTemplate* GetSegmentTemplate(const char* name);

	int						GetTrailSegmentIndex(const idStr& name);

	void					SetMinDuration(float duration);
	float					GetMinDuration(void) const { return(mMinDuration); }
	void					SetMaxDuration(float duration);
	float					GetMaxDuration(void) const { return(mMaxDuration); }

	float					GetSize(void) const { return(mSize); }
	void					SetSize(float size) { mSize = size; }

	int						GetNumSegmentTemplates(void) const { return(mSegmentTemplates.Num()); }
	rvSegmentTemplate* GetSegmentTemplate(const char* name) const;

	void					Init(void);
	void					Finish(void);
	bool					ParseSegment(int segmentType, idLexer* lexer);
	float					CalculateBounds(void);

	void					IncPlayCount(void) const { mPlayCount++; }
	int						GetPlayCount(void) const { return(mPlayCount); }
	void					IncLoopCount(void) const { mLoopCount++; }
	int						GetLoopCount(void) const { return(mLoopCount); }

	float					EvaluateCost(int activeParticles, int segment = -1) const;

	float					GetCutOffDistance(void) const { return mCutOffDistance; }
	void					SetCutOffDistance(float dist) { mCutOffDistance = dist; }
private:
	bool					Compare(const rvDeclEffect& comp) const;

	int						mFlags;
	float					mMinDuration;				// Minimum possible duration of the effect
	float					mMaxDuration;				// Maximum possible duration of the effect
	float					mCutOffDistance;
	float					mSize;
	idList	<rvSegmentTemplate>		mSegmentTemplates;

	mutable int				mPlayCount;					// For profiling
	mutable int				mLoopCount;					// For profiling
};

enum {
	EFLAG_LOOPING = BITT< 0 >::VALUE,
	EFLAG_HASENDORIGIN = BITT< 1 >::VALUE,
	EFLAG_ENDORIGINCHANGED = BITT< 2 >::VALUE,
	EFLAG_STOPPED = BITT< 3 >::VALUE,
	EFLAG_ORIENTATE_IDENTITY = BITT< 4 >::VALUE,
	EFLAG_AMBIENT = BITT< 5 >::VALUE,
};

const int LIGHTID_EFFECT_LIGHT = 300;

class rvBSE
{
public:
	rvBSE(void) { mFlags = 0; }
	~rvBSE(void) {}

	void					Init(const rvDeclEffect* declEffect, struct renderEffect_s* parms, float time);
	void					Destroy(void);

	void					SetFlag(bool on, int flag) { on ? mFlags |= flag : mFlags &= ~flag; }

	bool					GetLooping(void) const { return(!!(mFlags & EFLAG_LOOPING)); }
	bool					GetHasEndOrigin(void) const { return(!!(mFlags & EFLAG_HASENDORIGIN)); }
	bool					GetEndOriginChanged(void) const { return(!!(mFlags & EFLAG_ENDORIGINCHANGED)); }
	bool					GetStopped(void) const { return(!!(mFlags & EFLAG_STOPPED)); }
	bool					GetOrientateIdentity(void) const { return(!!(mFlags & EFLAG_ORIENTATE_IDENTITY)); }

	void					SetLooping(bool looping) { SetFlag(looping, EFLAG_LOOPING); }
	void					SetHasEndOrigin(bool hasEndOrigin) { SetFlag(hasEndOrigin, EFLAG_HASENDORIGIN); }
	void					SetEndOriginChanged(bool endOriginChanged) { SetFlag(endOriginChanged, EFLAG_ENDORIGINCHANGED); }
	virtual void			SetStopped(bool stopped) { SetFlag(stopped, EFLAG_STOPPED); }
	virtual void			SetOrientateIdentity(bool orientateIdentity) { SetFlag(orientateIdentity, EFLAG_ORIENTATE_IDENTITY); }

	void					SetDuration(float time);
	virtual float			GetDuration(void) const { return(mDuration); }
	float					GetStartTime(void) const { return mStartTime; }
	bool					Expired(float time) const { return(time > mStartTime + mDuration); }

	void					SetAttenuation(float atten) { mAttenuation = atten; }
	float					GetAttenuation(rvSegmentTemplate* st) const;
	float					GetOriginAttenuation(rvSegmentTemplate* st) const;
	void					UpdateAttenuation(void);

	idSoundEmitter* GetReferenceSound(void) const { return(mReferenceSound); }

	float					GetRed(void) const { return(mTint[0]); }
	float					GetGreen(void) const { return(mTint[1]); }
	float					GetBlue(void) const { return(mTint[2]); }
	float					GetAlpha(void) const { return(mTint[3]); }

	int						GetSuppressLightsInViewID(void) const { return mSuppressLightsInViewID; }

	float					GetBrightness(void) const { return(mBrightness); }
	void					SetBrightness(float bright) { mBrightness = bright; }

	bool					CanInterpolate(void) { return(mCurrentTime - mLastTime > BSE_TIME_EPSILON); }
	void					UpdateFromOwner(renderEffect_t* parms, float time, bool init = false);
	void					LoopInstant(float time);
	void					LoopLooping(float time);
	virtual bool			Service(renderEffect_t* parms, float time, bool spawn, bool& forcePush);
	void					UpdateSoundEmitter(rvSegmentTemplate* st, rvSegment* seg);

	virtual void			DisplayDebugInfo(const struct renderEffect_s* parms, const struct viewDef_s* view, idBounds& bounds);
	void					InitModel(idRenderModel* model);
	//idRenderModel* Render(idRenderModel* model, const struct renderEffect_s* owner, const viewDef_t* view); // <-- jmarshall
	const char* GetDeclName(void);
	rvSegment* GetTrailSegment(int child) { return(&mSegments[child]); }
	rvSegment* GetTrailSegment(const idStr& name);

	const idVec3& GetViewOrg(void) const { return(mViewOrg); }
	const idMat3& GetViewAxis(void) const { return(mViewAxis); }

	const idVec3& GetGravity(void) const { return(mGravity); }
	const idVec3& GetGravityDir(void) const { return(mGravityDir); }

	const idVec3& GetOriginalOrigin(void) const { return(mOriginalOrigin); }
	const idVec3& GetOriginalEndOrigin(void) const { return(mOriginalEndOrigin); }
	const idMat3& GetOriginalAxis(void) const { return(mOriginalAxis); }
	const idMat3& GetLightningAxis(void) const { return(mLightningAxis); }

	const idVec3& GetCurrentOrigin(void) const { return(mCurrentOrigin); }
	const idVec3& GetCurrentVelocity(void) const { return(mCurrentVelocity); }
	const idVec3& GetCurrentEndOrigin(void) const { return(mCurrentEndOrigin); }
	const idVec3			GetInterpolatedOffset(float time) const;
	const idMat3& GetCurrentAxis(void) const { return(mCurrentAxis); }
	const idMat3& GetCurrentAxisTransposed(void) const { return(mCurrentAxisTransposed); }
	virtual const idBounds& GetCurrentLocalBounds(void) const { return(mCurrentLocalBounds); }
	const idBounds& GetLastRenderBounds(void) const { return(mLastRenderBounds); }
	const idVec3& GetCurrentWindVector(void) const { return(mCurrentWindVector); }

	void					UpdateSegments(float time);
	virtual int				GetValidFrames(void) const { return mValidFrames; };

	void					SetMaterialColor(const idVec3& color) { mMaterialColor = color; }
	const idVec3& GetMaterialColor() const { return mMaterialColor; }

	float					EvaluateCost(int segment = -1);

private:
	// Fixed at spawn time
	const rvDeclEffect* mDeclEffect;
	idSoundEmitter* mReferenceSound;

	idVec3					mOriginalOrigin;			// Origin in world space
	idVec3					mOriginalEndOrigin;
	idMat3					mOriginalAxis;				// Original axis of orientation
	idMat3					mLightningAxis;

	float					mStartTime;					// World start time of effect
	float					mLastTime;					// Last time the effect was serviced
	float					mDuration;					// Duration of the effect - including any randomness

	idVec3					mMaterialColor;

	// Dynamically altered
	int						mFlags;
	int						mSuppressLightsInViewID;
	float					mAttenuation;				// Forced attenuation settable in code
	float					mOriginDistanceToCamera;	// Distance from effect origin to view origin
	float					mShortestDistanceToCamera;	// Closest point on effects bounds to view origin
	idVec4					mTint;						// Overridable tint
	float					mBrightness;				// Overall brightness of effect
	float					mCost;						// Best guess at how expensive the effect is

	idVec3					mViewOrg;
	idMat3					mViewAxis;
	idVec3					mGravity;
	idVec3					mGravityDir;

	idVec3					mLastOrigin;

	float					mCurrentTime;
	idVec3					mCurrentOrigin;				// Current origin in world space
	idVec3					mCurrentVelocity;			// Current velocity in world space
	idVec3					mCurrentEndOrigin;			// Current end origin in world space
	idMat3					mCurrentAxis;				// Current axis of orientation
	idMat3					mCurrentAxisTransposed;		// Current axis of orientation transposed
	idBounds				mCurrentLocalBounds;		// Current local bounds
	idBounds				mCurrentWorldBounds;		// Current world bounds
	idBounds				mLastRenderBounds;			// Last render bounds 
	idBounds				mGrownRenderBounds;			// Last render bounds 
	bool					mForcePush;
	idVec3					mCurrentWindVector;

	idList		<rvSegment>				mSegments;
	int						mValidFrames;				// The model doesn't need to be updated the next x frames (zero means reupdate every frame obviously)
};

class rvRenderEffectLocal;

class rvBSEManagerLocal : public rvBSEManager
{
public:
	virtual	bool				Init(void);
	virtual	bool				Shutdown(void);

	virtual	bool				PlayEffect(class rvRenderEffectLocal* def, float time);
	virtual	bool				ServiceEffect(class rvRenderEffectLocal* def, float time);
	virtual	void				StopEffect(rvRenderEffectLocal* def);
	virtual	void				FreeEffect(rvRenderEffectLocal* def);
	virtual	float				EffectDuration(const rvRenderEffectLocal* def);

	virtual	bool				CheckDefForSound(const renderEffect_t* def);

	virtual	void				BeginLevelLoad(void);
	virtual	void				EndLevelLoad(void);

	virtual	void				StartFrame(void);
	virtual	void				EndFrame(void);
	virtual bool				Filtered(const char* name, effectCategory_t category);

	virtual void				UpdateRateTimes(void) ;
	virtual bool				CanPlayRateLimited(effectCategory_t category);

	virtual int							AddTraceModel(idTraceModel* model);
	virtual idTraceModel*				GetTraceModel(int index);
	virtual void						FreeTraceModel(int index);
private:
	static		idBlockAlloc<rvBSE, 256/*, 0 //k*/>	effects;
	static		idVec3						mCubeNormals[6];
	static		idMat3						mModelToBSE;
	static		idList<idTraceModel*>		mTraceModels;
	static		const char* mSegmentNames[SEG_COUNT];
	static		int							mPerfCounters[NUM_PERF_COUNTERS];
	static		float						mEffectRates[EC_MAX];
	float								pauseTime;	// -1 means pause at the next time update
};
#endif
