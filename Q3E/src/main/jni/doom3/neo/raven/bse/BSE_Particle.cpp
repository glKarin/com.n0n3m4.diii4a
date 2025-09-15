/*
===========================================================================

QUAKE 4 BSE CODE RECREATION EFFORT - (c) 2025 by Justin Marshall(IceColdDuke).

QUAKE 4 BSE CODE RECREATION EFFORT is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

QUAKE 4 BSE CODE RECREATION EFFORT is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with QUAKE 4 BSE CODE RECREATION EFFORT.  If not, see <http://www.gnu.org/licenses/>.

In addition, the QUAKE 4 BSE CODE RECREATION EFFORT is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "BSE.h"

#include "BSE_Compat.h"

// ---------------------------------------------------------------------------
//  helper constants / macros
// ---------------------------------------------------------------------------
static const float LOG2E = 1.4426950408889634f;   // 1 / ln(2)

// ---------------------------------------------------------------------------
//  rvParticle :: Attenuate  (vec2 envelope)
// ---------------------------------------------------------------------------
void rvParticle::Attenuate(float               atten,
    const rvParticleParms& parms,
    rvEnvParms1& env)
{
    if (!(parms.mFlags & PF_ATTENUATE)) {
        return;
    }

    const float s = (parms.mFlags & PF_INVERT_ATTEN) ? (1.0f - atten) : atten;

    env.mStart *= s;
    env.mEnd *= s;
    // Note: original code did not touch env.mRate
}

// ---------------------------------------------------------------------------
//  rvParticle :: Attenuate  (vec2 envelope, no rate)
// ---------------------------------------------------------------------------
void rvParticle::Attenuate(float               atten,
    const rvParticleParms& parms,
    rvEnvParms2& env)
{
    if (!(parms.mFlags & PF_ATTENUATE)) {
        return;
    }

    const float s = (parms.mFlags & PF_INVERT_ATTEN) ? (1.0f - atten) : atten;

    env.mStart *= s;
    env.mEnd *= s;
}

// ---------------------------------------------------------------------------
//  rvParticle :: Attenuate  (vec3 envelope)
// ---------------------------------------------------------------------------
void rvParticle::Attenuate(float               atten,
    const rvParticleParms& parms,
    rvEnvParms3& env)
{
    if (!(parms.mFlags & PF_ATTENUATE)) {
        return;
    }

    const float s = (parms.mFlags & PF_INVERT_ATTEN) ? (1.0f - atten) : atten;

    env.mStart *= s;
    env.mEnd *= s;
}

// ---------------------------------------------------------------------------
//  rvLineParticle :: HandleTiling
// ---------------------------------------------------------------------------
void rvLineParticle::HandleTiling(const rvParticleTemplate* pt)
{
    if (!(mFlags & PF_TILING_LENGTH)) {
        return;
    }

    const idVec3 len(GetInitLength()[0], GetInitLength()[1], GetInitLength()[2]);
    mTextureScale = len.LengthFast() / pt->mTiling;
}

// ---------------------------------------------------------------------------
//  rvLinkedParticle :: HandleTiling
// ---------------------------------------------------------------------------
void rvLinkedParticle::HandleTiling(const rvParticleTemplate* pt)
{
    if (mFlags & PF_TILING_LENGTH) {
        mTextureScale = pt->mTiling;
    }
}

// ---------------------------------------------------------------------------
//  GetArrayEntry helpers – trivial contiguous-pool accessors
// ---------------------------------------------------------------------------
#define DEFINE_ARRAY_ENTRY( TYPE )                              \
    TYPE* TYPE::GetArrayEntry( int i ) const {                  \
        return ( i < 0 ) ? NULL : const_cast<TYPE*>( this ) + i; \
    }

#define DEFINE_ARRAY_INDEX( TYPE )                              \
    int TYPE::GetArrayIndex( rvParticle* p ) const {            \
        if ( !p )                                               \
            return -1;                                          \
        ptrdiff_t diff = reinterpret_cast< const uint8_t* >( p ) \
                       - reinterpret_cast< const uint8_t* >( this ); \
        return static_cast<int>( diff / sizeof( TYPE ) );       \
    }

DEFINE_ARRAY_ENTRY(rvLineParticle)
DEFINE_ARRAY_INDEX(rvLineParticle)

DEFINE_ARRAY_ENTRY(rvOrientedParticle)
DEFINE_ARRAY_INDEX(rvOrientedParticle)

DEFINE_ARRAY_ENTRY(rvElectricityParticle)
DEFINE_ARRAY_INDEX(rvElectricityParticle)

DEFINE_ARRAY_ENTRY(rvDecalParticle)
DEFINE_ARRAY_INDEX(rvDecalParticle)

DEFINE_ARRAY_ENTRY(rvLightParticle)
DEFINE_ARRAY_INDEX(rvLightParticle)

DEFINE_ARRAY_ENTRY(rvLinkedParticle)
DEFINE_ARRAY_INDEX(rvLinkedParticle)

DEFINE_ARRAY_ENTRY(rvDebrisParticle)
DEFINE_ARRAY_INDEX(rvDebrisParticle)

// ---------------------------------------------------------------------------
//  rvParticle :: EvaluateVelocity
// ---------------------------------------------------------------------------
void rvParticle::EvaluateVelocity(const rvBSE* /*effect*/, idVec3& out, float time) const
{
    // simple “constant direction” flag
    if (mFlags & PF_DIRECTIONAL) {
        out.Set(1.0f, 0.0f, 0.0f);
        return;
    }

    // base linear components
    out = mVelocity + mAcceleration * time;

    // lifespan-scaled friction term
    const float life = mEndTime - mStartTime;
    if (life <= 0.0f) {
        return;
    }

    const float expArg = ((life - time) / life) * LOG2E;
    const float expTerm = idMath::Pow(2.0f, expArg);       // 2^(expArg)
    const float frictionScalar = (expTerm * (1.0f - time * 0.33333334f) + 1.0f)
        * 0.5f * time * time;

    out += mFriction * frictionScalar;
}

// ===========================================================================
//  Spawn-position helpers
// ===========================================================================



// ---------------------------------------------------------------------------
//  rvParticle :: SetOriginUsingEndOrigin   (internal helper)
// ---------------------------------------------------------------------------
void rvParticle::SetOriginUsingEndOrigin(rvBSE* effect,const rvParticleTemplate* pt, idVec3* normal, const idVec3* centre)
{
#if 1 //karin: position hasEndOrigin, I think is it //k??? TODO add
    // reset position max range
    rvParticleParms copy = pt->mSpawnPosition;
    if(mFlags & PTF_SEGMENT_LOCKED)
    {
        idVec3 dis = effect->mCurrentEndOrigin - effect->mCurrentOrigin; // world
        copy.mMaxs += effect->mCurrentAxisTransposed * dis; // local
    }
    else
    {
        idVec3 dis = effect->mOriginalEndOrigin - effect->mOriginalOrigin; // world
        copy.mMaxs += effect->mOriginalAxis.Transpose() * dis; // local
    }
    idVec3 mInitPos2 = mInitPos;
    rvParticleParms::spawnFunctions[pt->mSpawnPosition.mSpawnType](&mInitPos.x, copy, NULL, NULL);
	copy.mMins.y = pt->mSpawnPosition.mMins.y;
	copy.mMins.z = pt->mSpawnPosition.mMins.z;
	copy.mMaxs.y = pt->mSpawnPosition.mMaxs.y;
	copy.mMaxs.z = pt->mSpawnPosition.mMaxs.z;
    rvParticleParms::spawnFunctions[pt->mSpawnPosition.mSpawnType](&mInitPos2.x, copy, normal, centre);
    mInitPos.y = mInitPos2.y;
    mInitPos.z = mInitPos2.z;
#else
    // jmarshall: hex rays hates this so might be wrong
    rvParticleParms::spawnFunctions[pt->mSpawnPosition.mSpawnType](&mInitPos.x, pt->mSpawnPosition, NULL, NULL);
    idVec3 mInitPos2 = mInitPos;
    mInitPos2.x = mInitPos.x;
    rvParticleParms::spawnFunctions[pt->mSpawnPosition.mSpawnType](&mInitPos2.x, pt->mSpawnPosition, normal, centre);
#endif
}


// ---------------------------------------------------------------------------
//  local helper
// ---------------------------------------------------------------------------
#if 0 //k??? TODO unuse this
static ID_INLINE void CallSpawnFunc(int index, const void* blob)
{
    typedef void (*Fn)(const idCmdArgs*);
    ((Fn)(*rvParticleParms::spawnFunctions[index]))(reinterpret_cast<const idCmdArgs*>(blob));
}
#endif

static ID_INLINE void CallSpawnFunc(int index, float* out, const rvParticleParms& p, idVec3* n = NULL, const idVec3* c = NULL)
{
    (*rvParticleParms::spawnFunctions[index])(out, p, n, c);
}

static ID_INLINE void CallSpawnFunc(int index, idVec3* out, const rvParticleParms& p, idVec3* n = NULL, const idVec3* c = NULL)
{
    CallSpawnFunc(index, out->ToFloatPtr(), p, n, c);
}

// ---------------------------------------------------------------------------
//  rvParticle :: HandleEndOrigin
// ---------------------------------------------------------------------------
void rvParticle::HandleEndOrigin(rvBSE* effect,
    const rvParticleTemplate* pt,
    idVec3* normal,
    const idVec3* centre)
{
    // preserve fraction in x (matches original semantics)
    mInitPos.x = mFraction;

    const bool effectWants = (effect->mFlags & EF_USES_END_ORIGIN/* 2 */) != 0;
    const bool spawnWants = (pt->mSpawnPosition.mFlags & PPF_USE_END_ORIGIN/* 2 */) != 0;

    if (effectWants && spawnWants) {
        SetOriginUsingEndOrigin(effect, pt, normal, centre);
    }
    else {
        CallSpawnFunc(pt->mSpawnPosition.mSpawnType, &mInitPos, pt->mSpawnPosition, normal, centre);
    }
}

// ===========================================================================
//  rvParticle :: FinishSpawn
// ===========================================================================
void rvParticle::FinishSpawn(rvBSE* effect,
    rvSegment* segment,
    float              birthTime,
    float              fraction,
    const idVec3& initOffset,
    const idMat3& initAxis)
{
    // ----------------------------------------------------------------------
    //  1)  fetch the template that drives this segment / particle
    // ----------------------------------------------------------------------
    const rvSegmentTemplate* st = segment->GetSegmentTemplate();
    if (!st) {
        return;
    }

    const rvParticleTemplate& pt = st->mParticleTemplate;

    // ----------------------------------------------------------------------
    //  2)  copy template-level particle flags, add the “segment locked” bit
    // ----------------------------------------------------------------------
    mFlags = pt.mFlags;
    if (segment->GetLocked()) {
        mFlags |= PF_SEGMENT_LOCKED;
    }
    else {
        mFlags &= ~PF_SEGMENT_LOCKED;
    }

    // ----------------------------------------------------------------------
    //  3)  spawn initial kinematic parameters
    // ----------------------------------------------------------------------
    CallSpawnFunc(pt.mSpawnVelocity.mSpawnType, &mVelocity, pt.mSpawnVelocity); //k??? TODO should add particle template to parameter?
    CallSpawnFunc(pt.mSpawnAcceleration.mSpawnType, &mAcceleration, pt.mSpawnAcceleration); //k??? TODO should add particle template to parameter?
    CallSpawnFunc(pt.mSpawnFriction.mSpawnType, &mFriction, pt.mSpawnFriction); //k??? TODO should add particle template to parameter?

    // misc bookkeeping
    mFraction = fraction;
    mTextureScale = 1.0f;

    // ----------------------------------------------------------------------
    //  4)  decide which “end-origin helpers” we need
    // ----------------------------------------------------------------------
    const bool wantsAlignNormal = (pt.mFlags & PTF_RELATIVE_NORMAL) != 0;
    const bool wantsCentre = (pt.mFlags & PTF_ALIGN_TO_NORMAL) != 0;

    const idVec3* centrePtr = NULL;
    idVec3* normalPtr = NULL;

    if (wantsAlignNormal) {
        // normal is generated later – centre unused
        normalPtr = &mNormal;
    }
    else if (wantsCentre) {
        // centre provided by template, normal generated later
        centrePtr = &pt.mCentre;
        normalPtr = &mNormal;
    }
    else {
        // optional explicit direction spawn
        if (pt.mFlags & PTF_SPAWN_DIRECTION) {
            CallSpawnFunc(pt.mSpawnDirection.mSpawnType, &mNormal, pt.mSpawnDirection);
        }
        else {
            mNormal.Set(1.0f, 0.0f, 0.0f);
        }
    }

    // ----------------------------------------------------------------------
    //  5)  initial position handling (may depend on end-origin flags)
    // ----------------------------------------------------------------------
    HandleEndOrigin(effect, &pt, normalPtr, centrePtr);

    // ----------------------------------------------------------------------
    //  6)  optionally rotate velocity into the emitter’s local axis
    // ----------------------------------------------------------------------
    if (pt.mFlags & PTF_TRANSFORM_PARENT) {
        mVelocity = initAxis * mVelocity;
    }

    rvAngles faceAngles = mNormal.ToRadians(); //k??? TODO calc here in Q4D // fix orient effect
    // ----------------------------------------------------------------------
    //  7)  if requested, align velocity / acceleration / friction so that
    //      the template “forward” points along mNormal
    // ----------------------------------------------------------------------
    if (wantsAlignNormal || wantsCentre) {

        //
        // – ensure mNormal is unit-length
        //
        if (!mNormal.Compare(vec3_zero)) {
            mNormal.Normalize(); //k??? TODO NormalizeFast
        }

        //
        // – build an orientation matrix whose X-axis == mNormal and transform
        //   the various vectors into that local space
        //
        idMat3 toLocal = mNormal.ToMat3();
        mVelocity = toLocal * mVelocity;
#if 1
        TransformLength(mNormal);
#else
        TransformLength(*reinterpret_cast<int*>(&mNormal.x),
            *reinterpret_cast<int*>(&mNormal.y),
            *reinterpret_cast<int*>(&mNormal.z));
#endif
    }

    // ----------------------------------------------------------------------
    //  8)  “invert” flag  –  flip velocity & length
    // ----------------------------------------------------------------------
    if (pt.mFlags & PTF_INVERT_VELOCITY) {
        mVelocity = -mVelocity;
        ScaleLength(-1.0f);
    }

    // ----------------------------------------------------------------------
    //  9)  choose a reasonable default facing direction
    // ----------------------------------------------------------------------
    if (!wantsAlignNormal && !wantsCentre) {
        mNormal = mVelocity;
        if (!mNormal.Compare(vec3_zero)) {
            mNormal.Normalize(); //k??? TODO NormalizeFast
        }
    }

    // ----------------------------------------------------------------------
    // 10) transform acceleration / friction into the same local space
    // ----------------------------------------------------------------------
    {
        idMat3 toLocal = mNormal.ToMat3();
        mAcceleration = toLocal * mAcceleration;
    }

    if (!wantsAlignNormal && !wantsCentre) {
        mNormal = mAcceleration;
        if (!mNormal.Compare(vec3_zero)) {
            mNormal.Normalize(); //k??? TODO NormalizeFast
        }
    }

    {
        idMat3 toLocal = mNormal.ToMat3();
        mFriction = toLocal * mFriction;
    }

    // ----------------------------------------------------------------------
    // 11)  copy current effect transform into the particle (unless locked)
    // ----------------------------------------------------------------------
    if (!(mFlags & PF_SEGMENT_LOCKED)) {
        mInitAxis = effect->mCurrentAxis;
        mInitEffectPos = effect->mCurrentOrigin;

        // subtract the emitter’s current offset so particles keep cooking
        idVec3 result;
        const idVec3 offset = effect->GetInterpolatedOffset(&result/*//k??? TODO don't write mInitPos */, birthTime);
        mInitPos -= mInitAxis * offset;
    }

    // ----------------------------------------------------------------------
    // 12)  optionally add caller-supplied offset
    // ----------------------------------------------------------------------
    if (pt.mFlags & PTF_TRANSFORM_PARENT) {
        mInitPos += initOffset;
    }

    // ----------------------------------------------------------------------
    // 13)  spawn start / end envelopes
    // ----------------------------------------------------------------------
    CallSpawnFunc(pt.mSpawnTint.mSpawnType, &mTintEnv.mStart, pt.mSpawnTint); //k??? TODO should add particle template to parameter?
    CallSpawnFunc(pt.mSpawnFade.mSpawnType, &mFadeEnv.mStart, pt.mSpawnFade); //k??? TODO should add particle template to parameter?
    CallSpawnFunc(pt.mSpawnSize.mSpawnType, GetInitSize(), pt.mSpawnSize); //k??? TODO should add particle template to parameter?
    CallSpawnFunc(pt.mSpawnRotate.mSpawnType, GetInitRotation(), pt.mSpawnRotate); //k??? TODO should add particle template to parameter?
    CallSpawnFunc(pt.mSpawnAngle.mSpawnType, &mAngleEnv.mStart, pt.mSpawnAngle); //k??? TODO should add particle template to parameter?
    CallSpawnFunc(pt.mSpawnOffset.mSpawnType, &mOffsetEnv.mStart, pt.mSpawnOffset); //k??? TODO should add particle template to parameter?

    CallSpawnFunc(pt.mDeathTint.mSpawnType, &mTintEnv.mEnd, pt.mDeathTint); //k??? TODO should add particle template to parameter?
    CallSpawnFunc(pt.mDeathFade.mSpawnType, &mFadeEnv.mEnd, pt.mDeathFade); //k??? TODO should add particle template to parameter?
    CallSpawnFunc(pt.mDeathSize.mSpawnType, GetDestSize(), pt.mDeathSize); //k??? TODO should add particle template to parameter?
    CallSpawnFunc(pt.mDeathRotate.mSpawnType, GetDestRotation(), pt.mDeathRotate); //k??? TODO should add particle template to parameter?
    CallSpawnFunc(pt.mDeathAngle.mSpawnType, &mAngleEnv.mEnd, pt.mDeathAngle); //k??? TODO should add particle template to parameter?
    CallSpawnFunc(pt.mDeathOffset.mSpawnType, &mOffsetEnv.mEnd, pt.mDeathOffset); //k??? TODO should add particle template to parameter?

    //  Offsets that don’t stay in local space -> flag for update during run
    if (pt.mSpawnOffset.mSpawnType != 3 || pt.mDeathOffset.mSpawnType != 3) {
        mFlags |= 0x000004;      // “needs offset update” (original bit-3)
    }

    //  Handle “relative” death-vs-spawn parameter conversions
    pt.mDeathTint.HandleRelativeParms(&mTintEnv.mEnd.x,
        &mTintEnv.mStart.x, 3);
    pt.mDeathFade.HandleRelativeParms(&mFadeEnv.mEnd,
        &mFadeEnv.mStart, 1);

    pt.mDeathSize.HandleRelativeParms(
        GetDestSize(),
        GetInitSize(),
        pt.mNumSizeParms);

    pt.mDeathRotate.HandleRelativeParms(
        GetDestRotation(),
        GetInitRotation(),
        pt.mNumRotateParms);

    pt.mDeathAngle.HandleRelativeParms(
        &mAngleEnv.mEnd.x,
        &mAngleEnv.mStart.x,
        3);

    pt.mDeathOffset.HandleRelativeParms(
        &mOffsetEnv.mEnd.x,
        &mOffsetEnv.mStart.x,
        3);

    // ----------------------------------------------------------------------
    // 14)  scale rotation / angle from [0-1] to [0-2π]
    // ----------------------------------------------------------------------
    ScaleRotation(idMath::TWO_PI);
    ScaleAngle(idMath::TWO_PI);

    // ----------------------------------------------------------------------
    // 15)  final orientation handling (templates can override)
    // ----------------------------------------------------------------------
    //k??? TODO Q4D not is rvAngles faceAngles = RvAngles(mNormal.ToAngles()); // fix orient effect
    HandleOrientation(&faceAngles);

    // ----------------------------------------------------------------------
    // 16)  trail & model / electricity extras
    // ----------------------------------------------------------------------
    mTrailTime = rvRandom::flrand(pt.mTrailTime.x, pt.mTrailTime.y);
    mTrailCount = pt.GetTrailCount();

    SetModel(pt.mModelName.c_str());
    SetupElectricity(&pt);

    // ----------------------------------------------------------------------
    // 17)  global attenuation & size scaling
    // ----------------------------------------------------------------------
    const float atten = effect->GetAttenuation(st);
    AttenuateFade(atten, &pt.mSpawnFade);
    AttenuateSize(atten, &pt.mSpawnSize);

    // ----------------------------------------------------------------------
    // 18)  lifespan  –  pick random duration inside range
    // ----------------------------------------------------------------------
    const float life = rvRandom::flrand(pt.mDuration.x, pt.mDuration.y);
    mStartTime = birthTime;
    mEndTime = birthTime + life;
    mMotionStartTime = birthTime;
    mLastTrailTime = birthTime;

    // ----------------------------------------------------------------------
    // 19)  initialise all per-particle envelopes
    // ----------------------------------------------------------------------
    mTintEnv.Init(pt.mTintEnvelope, life);
    mFadeEnv.Init(pt.mFadeEnvelope, life);
    InitSizeEnv(pt.mSizeEnvelope, life);
    InitRotationEnv(pt.mRotateEnvelope, life);
    mAngleEnv.Init(pt.mAngleEnvelope, life);
    mOffsetEnv.Init(pt.mOffsetEnvelope, life);

    // ----------------------------------------------------------------------
    // 20)  gravity – template value scaled by emitter gravity vector
    // ----------------------------------------------------------------------
    const float gScale = rvRandom::flrand(pt.mGravity.x, pt.mGravity.y);
    const idVec3 gWorld = effect->mGravity * gScale;          // local-space
    const idVec3 gAccel = effect->mCurrentAxisTransposed * gWorld;
    mAcceleration += gAccel;
}

// ============================================================================
// rvDebrisParticle :: FinishSpawn
// ----------------------------------------------------------------------------
//  This override is simpler than the “general” particle path because debris
//  particles are converted to *client-side moveable entities* as soon as
//  they are spawned.  They never run an envelope update on the CPU side after
//  the first frame, so a lot of usual book-keeping (size/fade/tint/offset)
//  is skipped.
//
//  Steps
//  -----
//   1)  Early-out if the **bse_debris** switch is OFF or we are replaying a
//       demo (because the server recorded the moveables already).
//   2)  Spawn velocity, origin, normal – respecting the same “end-origin” and
//       “align” flags used by regular particles.
//   3)  Convert velocity / length into *local debris space* when required.
//   4)  Add emitter-space gravity → local space acceleration.
//   5)  Convert everything to absolute world coordinates.
//   6)  Call **game->SpawnClientMoveable** with the template’s entityDef.
// ----------------------------------------------------------------------------

// ---------------------------------------------------------------------------
void rvDebrisParticle::FinishSpawn(rvBSE* effect,
    rvSegment* segment,
    float              birthTime,
    float              fraction,
    const idVec3& initOffset,
    const idMat3& initAxis)
{
    //--------------------------------------------------------------------------
    // 0.  Debris system disabled? ( cvar + demo playback guard )
    //--------------------------------------------------------------------------
    if (!bse_debris.GetBool() || session->readDemo) {
        return;
    }

    //--------------------------------------------------------------------------
    // 1.  Fetch segment template and basic per-particle flags / velocity
    //--------------------------------------------------------------------------
    rvSegmentTemplate* st = (rvSegmentTemplate * )segment->GetSegmentTemplate();
    if (!st) {
        return;
    }
    rvParticleTemplate& pt = st->mParticleTemplate;

    mFlags = pt.mFlags;
    CallSpawnFunc(pt.mSpawnVelocity.mSpawnType, &mVelocity, pt.mSpawnVelocity); //k??? TODO should add particle template to parameter?

    mFraction = fraction;
    mTextureScale = 1.0f;

    //--------------------------------------------------------------------------
    // 2.  Spawn origin / normal  (honours ALIGN / RELATIVE flags)
    //--------------------------------------------------------------------------
    if (pt.mFlags & PTF_RELATIVE_NORMAL) {
        HandleEndOrigin(effect, &pt, &mNormal, NULL);
    }
    else if (pt.mFlags & PTF_ALIGN_TO_NORMAL) {
        HandleEndOrigin(effect, &pt, &mNormal, &pt.mCentre);
    }
    else {
        HandleEndOrigin(effect, &pt, NULL, NULL);
        mNormal.Set(1.f, 0.f, 0.f);          // default facing
    }

    //--------------------------------------------------------------------------
    // 3.  When aligning to a normal, rotate velocity into that local frame
    //--------------------------------------------------------------------------
    const bool needLocal = (pt.mFlags & (PTF_ALIGN_TO_NORMAL | PTF_RELATIVE_NORMAL)) != 0;
    if (needLocal) {
        if (!mNormal.Compare(vec3_zero)) {
            mNormal.Normalize(); //k??? TODO NormalizeFast
        }
        idMat3 toLocal = mNormal.ToMat3();
        mVelocity = toLocal * mVelocity;

        // length helpers are still used internally by the renderer
#if 1
        TransformLength(mNormal);
#else
        TransformLength(*reinterpret_cast<int*>(&mNormal.x),
            *reinterpret_cast<int*>(&mNormal.y),
            *reinterpret_cast<int*>(&mNormal.z));
#endif
    }

    //--------------------------------------------------------------------------
    // 4.  Optional “invert” – flip velocity + length
    //--------------------------------------------------------------------------
    if (pt.mFlags & PTF_INVERT_VELOCITY) {
        mVelocity = -mVelocity;
        ScaleLength(-1.0f);
    }

    //--------------------------------------------------------------------------
    // 5.  Make sure mNormal is non-zero (fall back to velocity dir)
    //--------------------------------------------------------------------------
    if (mNormal.Compare(vec3_zero)) {
        mNormal = mVelocity;
        if (!mNormal.Compare(vec3_zero)) {
            mNormal.Normalize(); //k??? NormalizeFast
        }
    }

    //--------------------------------------------------------------------------
    // 6.  Debris never uses velocity-dependent friction
    //--------------------------------------------------------------------------
    mFriction.Zero();

    //--------------------------------------------------------------------------
    // 7.  Copy emitter transform into “init” members and subtract current
    //     emitter offset so the world-space calculation below is correct.
    //--------------------------------------------------------------------------
    mInitAxis = effect->mCurrentAxis;
    mInitEffectPos = effect->mCurrentOrigin;

    idVec3 result;
    idVec3 emitterOffset = effect->GetInterpolatedOffset(&result/* mInitPos */, birthTime);
    mInitPos -= mInitAxis * emitterOffset;          // localise

    //--------------------------------------------------------------------------
    // 8.  Spawn initial / dest rotation, scale them to ±π
    //--------------------------------------------------------------------------
    CallSpawnFunc(pt.mSpawnRotate.mSpawnType, GetInitRotation(), pt.mSpawnRotate); //k??? TODO should add particle template to parameter?
    CallSpawnFunc(pt.mSpawnAngle.mSpawnType, &mAngleEnv.mStart, pt.mSpawnAngle); //k??? TODO should add particle template to parameter?
    CallSpawnFunc(pt.mDeathRotate.mSpawnType, GetDestRotation(), pt.mDeathRotate); //k??? TODO should add particle template to parameter?

    ScaleRotation(idMath::TWO_PI);
    ScaleAngle(idMath::TWO_PI);

    // debris uses a 1-second rotation/angle envelope (arbitrary but cheap)
    InitRotationEnv(pt.mRotateEnvelope, 1.0f);
    mAngleEnv.Init(pt.mAngleEnvelope, 1.0f);

    //--------------------------------------------------------------------------
    // 9.  Time stamps – lifetime is zero because the moveable takes over
    //--------------------------------------------------------------------------
    mStartTime = birthTime;
    mMotionStartTime = birthTime;
    mEndTime = birthTime;       // no CPU-side lifespan
    mLastTrailTime = birthTime;
    mTrailTime = 0.f;
    mTrailCount = 0;

    //--------------------------------------------------------------------------
    // 10.  Add gravity from the template  (scaled random in range)
    //--------------------------------------------------------------------------
    const float gRand = rvRandom::flrand(pt.mGravity.x, pt.mGravity.y);
    const idVec3 gWorld = effect->mGravity * gRand;
    const idVec3 gLocal = effect->mCurrentAxisTransposed * gWorld;
    mAcceleration += gLocal;

    //--------------------------------------------------------------------------
    // 11.  Convert origin / velocity / angular velocity into *world* space
    //--------------------------------------------------------------------------
    idVec3 worldPos = effect->mOriginalOrigin + effect->mOriginalAxis * mInitPos;
    idVec3 worldVel = effect->mCurrentAxis * mVelocity;
    idVec3 angularVel(mRotationEnv.mEnd.x,
        mRotationEnv.mEnd.y,
        mRotationEnv.mEnd.z);

    //--------------------------------------------------------------------------
    // 12.  Finally spawn the client-side moveable (life == 0 → remove by FX)
    //--------------------------------------------------------------------------
    const int lifeMS = int((mEndTime - mStartTime) * 1000.0f); //k??? TODO 0;   // handled entirely by the moveable itself
    game->SpawnClientMoveable(pt.mEntityDefName.c_str(),
        lifeMS,
        worldPos,
        effect->mCurrentAxis,
        worldVel,
        angularVel);
}

// ============================================================================
// rvLineParticle :: Refresh
// ============================================================================
void rvLineParticle::Refresh(const rvBSE* effect,
    const rvSegmentTemplate* st,
                             const rvParticleTemplate* pt)
{
    //----------------------------------------------------------------------
    // 1.  Re-evaluate INIT / DEST length vectors (exactly like FinishSpawn)
    //----------------------------------------------------------------------
    float* initLen = GetInitLength();
    if ((effect->mFlags & 2) && (pt->mSpawnLength.mFlags & 2)) {
        SetLengthUsingEndOrigin(effect, &pt->mSpawnLength, initLen);
    }
    else {
        CallSpawnFunc(pt->mSpawnLength.mSpawnType, initLen, pt->mSpawnLength); //k??? TODO should add particle template to parameter?
    }

    float* destLen = GetDestLength();
    if ((effect->mFlags & 2) && (pt->mSpawnLength.mFlags & 2)) {
        SetLengthUsingEndOrigin(effect, &pt->mDeathLength, destLen);
    }
    else {
        CallSpawnFunc(pt->mDeathLength.mSpawnType, destLen, pt->mDeathLength); //k??? TODO should add particle template to parameter?
    }

    //----------------------------------------------------------------------
    // 2.  Propagate “relative” death-vs-spawn conversions
    //----------------------------------------------------------------------
    pt->mDeathLength.HandleRelativeParms(destLen,initLen,/*parmCount =*/ 3);

    //----------------------------------------------------------------------
    // 3.  Refresh tiling and attenuation in case template was edited
    //----------------------------------------------------------------------
    HandleTiling(pt);

#if 1 //k??? TODO not in Q4D
    const float atten = effect->GetAttenuation(st);
    AttenuateLength(atten, &pt->mSpawnLength);
#endif

    //----------------------------------------------------------------------
    // 4.  Re-initialise the length envelope with the current lifetime
    //----------------------------------------------------------------------
    const float duration = mEndTime - mStartTime;
    mLengthEnv.Init(pt->mLengthEnvelope, duration);
}

// ============================================================================
//  Spawn-info helpers
// ----------------------------------------------------------------------------
//  These are used by the renderer when the particle first becomes visible.
// ============================================================================

// ----------------------------  rvSpriteParticle  ----------------------------
void rvSpriteParticle::GetSpawnInfo(idVec4& tint, idVec3& size, idVec3& rotate)
{
    tint.Set(mTintEnv.mStart.x,
        mTintEnv.mStart.y,
        mTintEnv.mStart.z,
        mTintEnv.mRate.x);

    size.Set(mSizeEnv.mStart.x,
        mSizeEnv.mStart.y,
        0.0f);

    rotate.Set(mRotationEnv.mStart.x, 0.0f, 0.0f);
}

// ----------------------------  rvLineParticle  ------------------------------
void rvLineParticle::GetSpawnInfo(idVec4& tint, idVec3& size, idVec3& rotate)
{
    tint.Set(mTintEnv.mStart.x,
        mTintEnv.mStart.y,
        mTintEnv.mStart.z,
        mTintEnv.mRate.x);

    size.Set(mSizeEnv.mStart.x, 0.0f, 0.0f); //k??? TODO rvLineParticle's size is 1D
    rotate.Zero();
}

// ---------------------------  rvOrientedParticle  ---------------------------
void rvOrientedParticle::GetSpawnInfo(idVec4& tint, idVec3& size, idVec3& rotate)
{
    tint.Set(mTintEnv.mStart.x,
        mTintEnv.mStart.y,
        mTintEnv.mStart.z,
        mTintEnv.mRate.x);

    size.Set(mSizeEnv.mStart.x,
        mSizeEnv.mStart.y,
        0.0f);

    rotate = mRotationEnv.mStart;    // full XYZ
}

// ----------------------------  rvModelParticle  -----------------------------
void rvModelParticle::GetSpawnInfo(idVec4& tint, idVec3& size, idVec3& rotate)
{
    tint.Set(mTintEnv.mStart.x,
        mTintEnv.mStart.y,
        mTintEnv.mStart.z,
        mTintEnv.mRate.x);

    size = mSizeEnv.mStart;
    rotate = mRotationEnv.mStart;
}

// ----------------------------  rvLightParticle  -----------------------------
void rvLightParticle::GetSpawnInfo(idVec4& tint, idVec3& size, idVec3& rotate)
{
    tint.Set(mTintEnv.mStart.x,
        mTintEnv.mStart.y,
        mTintEnv.mStart.z,
        mTintEnv.mRate.x);

    size = mSizeEnv.mStart;
    rotate.Zero();
}


// ---------------------------------------------------------------------------
//  small helpers
// ---------------------------------------------------------------------------

// ===========================================================================
//  rvParticle :: Bounce
// ===========================================================================
void rvParticle::Bounce(rvBSE* effect, const rvParticleTemplate* pt, idVec3 endPos, idVec3 normal, float time)
{
    // ----------------------------------------------------------------------
    // 1)  old velocity at the impact moment
    // ----------------------------------------------------------------------
    const float dt = time - mMotionStartTime;

    idVec3 oldVel;
    EvaluateVelocity(effect, oldVel, dt);

    // convert to **world space**
    idVec3 worldOldVel = effect->mCurrentAxis * (mInitAxis.Transpose() * oldVel);

    // ----------------------------------------------------------------------
    // 2)  reflect and scale by bounce coefficient
    // ----------------------------------------------------------------------
    const float proj = BSE::Dot(worldOldVel, normal);                 // component toward plane
    idVec3 worldNewVel = worldOldVel - (2.0f * proj) * normal;  // reflection
    worldNewVel *= pt->mBounce;                                   // restitution

    // optional “stick” if we hit from below at a steep angle
    if (proj < -0.70710678f) {                                  // cos(45°)
        mFlags |= 0x000001;                                   // PF_DIRECTIONAL (“stuck”)
        worldNewVel = vec3_zero;
    }

    // ----------------------------------------------------------------------
    // 3)  convert everything back to *particle local space*
    // ----------------------------------------------------------------------
    idMat3 mCurrentAxisTransposed = BSE::Transposed(effect->mCurrentAxis);
    mVelocity = mInitAxis * (mCurrentAxisTransposed * worldNewVel);
    mMotionStartTime = time;

    // translate new start position from world → local
    idVec3 worldPos = endPos;
    idVec3 localPos = mCurrentAxisTransposed
        * (worldPos - effect->mCurrentOrigin);
    mInitPos = mInitAxis * localPos;            // into original local frame
}

// ===========================================================================
//  rvParticle :: EvaluatePosition
// ===========================================================================
void rvParticle::EvaluatePosition(const rvBSE* effect, idVec3& outPos, float time)
{
    // ----------------------------------------------------------------------
    // 1)  if flagged “stuck”, particle never moves again
    // ----------------------------------------------------------------------
    if (mFlags & 0x000001) {                         // PF_DIRECTIONAL used as “stuck”
        outPos = mInitPos;
        return;
    }

    // ----------------------------------------------------------------------
    // 2)  local-space integration  (V + A + optional angle/offset envelope)
    // ----------------------------------------------------------------------
    outPos = mInitPos + mVelocity * time
        + 0.5f * mAcceleration * (time * time);

    if (mFlags & 0x000004) {                         // needs offset update
        rvAngles angle;
        idVec3   offset;
        mAngleEnv.Evaluate(time, &angle.pitch);
        mOffsetEnv.Evaluate(time, &offset.x);

        idMat3 rot;
        angle.ToMat3(rot);
        outPos += rot * offset;
    }

    // ----------------------------------------------------------------------
    // 3)  quadratic drag / friction  (matches original exponential form)
    // ----------------------------------------------------------------------
    if (!mFriction.Compare(vec3_zero)) {
        const float life = mEndTime - mStartTime;
        const float t_2 = 0.5f * time * time;
        const float expoExp = ((life - t_2) / life) * LOG2E;     // ln→log2 conversion
        const float expoTerm = idMath::Pow(2.0f, expoExp);          // 2^(x)
        const float dispK = t_2 * ((expoTerm - 1.0f) * t_2) * (1.0f / 3.0f);

        outPos += mFriction * dispK;
    }

    // ----------------------------------------------------------------------
    // 4)  transform from *particle local* → *current world* coordinates
    //     (unless the particle was “locked” at spawn)
    // ----------------------------------------------------------------------
    if (!(mFlags & 0x000002)) {                    // PF_SEGMENT_LOCKED
        // rotate into the emitter’s current orientation
        idVec3 worldPos = effect->mCurrentAxis
            * (mInitAxis.Transpose() * outPos);

        // offset for emitter translation since spawn
        idVec3 delta = mInitEffectPos - effect->mCurrentOrigin;
        worldPos += effect->mCurrentAxisTransposed * delta;

        outPos = worldPos;
    }
}


// ----------------------------------------------------------------------------
// 1.  CheckTimeoutEffect – plays a one-shot effect when the particle times out
// ----------------------------------------------------------------------------
void rvParticle::CheckTimeoutEffect(rvBSE* effect,
                                    const rvSegmentTemplate* st,
    float              time)
{
    if (!(st->mFlags & 0x0001)) {                     // segment wants timeout FX?
        return;
    }

    const int numFx = st->mParticleTemplate.mNumTimeoutEffects;
    if (numFx == 0) {
        return;
    }

    // ----------------------------------------------------------------------
    // evaluate current world-space position / velocity
    // ----------------------------------------------------------------------
    const float dt = time - mMotionStartTime;

    idVec3 posLocal;
    EvaluatePosition(effect, posLocal, dt);

    idVec3 velLocal;
    EvaluateVelocity(effect, velLocal, dt);

    // normalise direction for orientation matrix
    if (!velLocal.Compare(vec3_zero)) {
        velLocal.NormalizeFast();
    }

    idVec3 velWorld = effect->mCurrentAxis * (mInitAxis.Transpose() * velLocal);
    idMat3 axis = velWorld.ToMat3();

    idVec3 posWorld = effect->mCurrentOrigin
        + effect->mCurrentAxis * (mInitAxis.Transpose() * posLocal);

    // ----------------------------------------------------------------------
    // trigger the effect
    // ----------------------------------------------------------------------
    const idDecl* fxName = st->mParticleTemplate.mTimeoutEffects[BSE::RandIndex(numFx)];
    game->PlayEffect(fxName,
        posWorld,
        axis,
        /*joint=*/ NULL,
        vec3_origin,   // dir
        /*surfId=*/ NULL,
        /* predictBit = */false,
        EC_IGNORE,
        vec4_one);
}

// ----------------------------------------------------------------------------
// 2.  CalcImpactPoint – nudges hit-position so the smoke appears just outside
//                       the collision surface (works for oriented bboxes)
// ----------------------------------------------------------------------------
void rvParticle::CalcImpactPoint(idVec3& out,
    const idVec3& origin,
    const idVec3& motion,
    const idBounds& bounds,
    const idVec3& normal)
{
    out = origin;                                         // default

    // guard against degenerate motion or bounds
    if (motion.Compare(vec3_zero) || bounds.IsCleared()) {
        return;
    }

    // convert motion into *unit cube* space so we can pick the
    // dominant axis (largest normalised component)
    idVec3 size = BSE::GetSize(bounds);
    idVec3 work(motion.x / size.x,
        motion.y / size.y,
        motion.z / size.z);

    work.Normalize(); //k??? NormalizeFast

    const idVec3 absWork(fabsf(work.x), fabsf(work.y), fabsf(work.z));
    int axis = 0;
    if (absWork.y >= absWork.x && absWork.y >= absWork.z) axis = 1;
    if (absWork.z >= absWork.x && absWork.z >= absWork.y) axis = 2;

    // distance to move:   half-extent along *chosen* axis
    const float halfX = size.x * 0.5f;
    const float halfY = size.y * 0.5f;
    const float halfZ = size.z * 0.5f;

    const float invLen = 0.5f / fabsf(work[axis]);
    const idVec3 push = BSE::Mult(idVec3(halfX, halfY, halfZ) * invLen, work); //k??? TODO dot not dot

    // offset the hit position by “normal + push”
    out += 2.0f * normal + push;
}

// ----------------------------------------------------------------------------
// 3.  EmitSmokeParticles – leaves a smoke trail behind moving particles
// ----------------------------------------------------------------------------
void rvParticle::EmitSmokeParticles(rvBSE* effect,rvSegment* child, float      time)
{
    static const float kUpdate = 0.016f;              // ~60 Hz

	const rvSegmentTemplate* st = child->GetSegmentTemplate();
	if (!st) {
		return;
	}
    const float timeEnd = time + kUpdate;
    while (mLastTrailTime < timeEnd) {

        // stay within particle lifetime
        if (mLastTrailTime >= mStartTime && mLastTrailTime < mEndTime) {

            // position / dir at this timestamp
            const float t = mLastTrailTime - mStartTime;

            idVec3 posLocal;
            EvaluatePosition(effect, posLocal, t);

            idVec3 velLocal;
            EvaluateVelocity(effect, velLocal, t);
            velLocal.Normalize(); //k??? TODO NormalizeFast

            idMat3 axis = velLocal.ToMat3();

            // hand off to child segment
            child->SpawnParticle(effect,st,mLastTrailTime,&posLocal,&axis);
        }

        mLastTrailTime += child->AttenuateInterval(effect, st);
    }
}

// ----------------------------------------------------------------------------
// 4.  RunPhysics – optional collision + bounce for individual particles
// ----------------------------------------------------------------------------
bool rvParticle::RunPhysics(rvBSE* effect,
    const rvSegmentTemplate* st,
    float              time)
{
    // -------------------------------------------------- early outs
    if (!bse_physics.GetBool() ||
        session->readDemo ||
        (mFlags & 0x000001) ||          // stuck
        !(st->mFlags & 0x0001))           // segment wants physics
    {
        return false;
    }

    const rvParticleTemplate& pt = st->mParticleTemplate;

    // require “physics” flag and at least 100 ms of motion
    if (!(pt.mFlags & 0x0200) || time - mMotionStartTime < 0.1f) {
        return false;
    }

	if ( bseLocal.DebugHudActive() )
		++bseLocal.mPerfCounters[1]; // dword_1137DDAC;
    //--------------------------------------------------- build world ray
    const idVec3 worldOrigin = effect->mCurrentOrigin;
    const idMat3& worldAxis = effect->mCurrentAxis;

    const float dtStart = time - mMotionStartTime - 0.1f;
    const float dtEnd = time - mMotionStartTime;

    idVec3 fromLocal; EvaluatePosition(effect, fromLocal, dtStart);
    idVec3 toLocal;   EvaluatePosition(effect, toLocal, dtEnd);

    idVec3 fromWorld = worldOrigin + worldAxis * (mInitAxis.Transpose() * fromLocal);
    idVec3 toWorld = worldOrigin + worldAxis * (mInitAxis.Transpose() * toLocal);

    //---------------------------------------------------- trace
    idTraceModel* trm = pt.GetTraceModel();

    trace_t tr;
    game->Translation(tr, fromWorld, toWorld, trm, CONTENTS_SOLID | CONTENTS_OPAQUE);

    if (tr.fraction >= 1.0f) {                         // no hit
        return false;
    }

    //---------------------------------------------------- play impact FX
    if (pt.mNumImpactEffects && bse->CanPlayRateLimited(EC_IMPACT_PARTICLES)) {

        idVec3 impactPos = tr.endpos;

        if (trm) {                                     // nudge inside oriented bbox
            idVec3 motion = (toWorld - fromWorld) * tr.fraction;
            CalcImpactPoint(impactPos, tr.endpos, motion, trm->bounds, tr.c.normal);
        }

        const rvDeclEffect *fxName = pt.mImpactEffects[BSE::RandIndex(pt.mNumImpactEffects)];
        idMat3 axis = tr.c.normal.ToMat3();

        game->PlayEffect(fxName,impactPos,axis,NULL,vec3_origin,NULL,EC_IGNORE);
    }

    //---------------------------------------------------- bounce?
    if (pt.mBounce > 0.0f) {
        Bounce(effect, &pt, tr.endpos, tr.c.normal, time);
    }

    // if template has “killOnImpact” flag (bit-10 in original),
    // return true to remove the particle
    return (pt.mFlags & 0x0400) != 0;
}

void rvParticle::AttenuateFade(float atten, const rvParticleParms *parms)
{
    int flags; // eax
    float v4; // st7

    flags = parms->mFlags;
    if ((flags & PTF_ATTENUATE/* 0x20 */) != 0) {
        if ((flags & PTF_INVERT_ATTEN/* 0x40 */) != 0) {
            v4 = 1.0f - atten;
            mFadeEnv.mStart = v4 * mFadeEnv.mStart;
            mFadeEnv.mEnd = v4 * mFadeEnv.mEnd;
        } else {
            mFadeEnv.mStart = atten * mFadeEnv.mStart;
            mFadeEnv.mEnd = atten * mFadeEnv.mEnd;
        }
    }
}

int rvParticleTemplate::GetMaxTrailCount() const
{
    return (int)ceil(mTrailCount.y) + 1;
}

void rvParticle::ScaleAngle(float constant)
{
    mAngleEnv.mStart.x = constant * mAngleEnv.mStart.x;
    mAngleEnv.mStart.y = constant * mAngleEnv.mStart.y;
    mAngleEnv.mStart.z = constant * mAngleEnv.mStart.z;
    mAngleEnv.mEnd.x = constant * mAngleEnv.mEnd.x;
    mAngleEnv.mEnd.y = constant * mAngleEnv.mEnd.y;
    mAngleEnv.mEnd.z = constant * mAngleEnv.mEnd.z;
}

void rvParticle::ScaleRotation(float constant)
{
    mRotationEnv.mStart.x = constant * mRotationEnv.mStart.x;
    mRotationEnv.mStart.y = constant * mRotationEnv.mStart.y;
    mRotationEnv.mStart.z = constant * mRotationEnv.mStart.z;
    mRotationEnv.mEnd.x = constant * mRotationEnv.mEnd.x;
    mRotationEnv.mEnd.y = constant * mRotationEnv.mEnd.y;
    mRotationEnv.mEnd.z = constant * mRotationEnv.mEnd.z;
}

void rvParticle::AttenuateSize(float atten, const rvParticleParms *parms)
{
    Attenuate(atten, *parms, mSizeEnv);
}

void rvLineParticle::FinishSpawn(rvBSE* effect, rvSegment* segment, float birthTime, float a11, const idVec3& initOffset, const idMat3& initAxis)
{
    const rvSegmentTemplate *SegmentTemplate; // eax
    const rvParticleTemplate *p_mParticleTemplate; // edi
    float *v16; // eax
    float *v17; // eax
    float *v18; // eax
    float duration; // [esp+Ch] [ebp-18h]
    float *v21; // [esp+14h] [ebp-10h]
    float fractiona; // [esp+34h] [ebp+10h]

    SegmentTemplate = segment->GetSegmentTemplate();
    if ( SegmentTemplate )
    {
        p_mParticleTemplate = &SegmentTemplate->mParticleTemplate;
        v16 = GetInitLength();
        if ( (effect->mFlags & 2) != 0 && (p_mParticleTemplate->mSpawnLength.mFlags & 2) != 0 )
            SetLengthUsingEndOrigin(effect, &p_mParticleTemplate->mSpawnLength, v16);
        else
            rvParticleParms::spawnFunctions[p_mParticleTemplate->mSpawnLength.mSpawnType](v16, p_mParticleTemplate->mSpawnLength, NULL, NULL);
        v17 = GetDestLength();
        if ( (effect->mFlags & 2) != 0 && (p_mParticleTemplate->mSpawnLength.mFlags & 2) != 0 )
            SetLengthUsingEndOrigin(effect, &p_mParticleTemplate->mDeathLength, v17);
        else
            rvParticleParms::spawnFunctions[p_mParticleTemplate->mDeathLength.mSpawnType](v17, p_mParticleTemplate->mDeathLength, NULL, NULL);
        rvParticle::FinishSpawn(effect, segment, birthTime, a11, initOffset, initAxis);
        v21 = GetInitLength();
        v18 = GetDestLength();
        p_mParticleTemplate->mDeathLength.HandleRelativeParms(v18, v21, 3);
        HandleTiling(p_mParticleTemplate);
        fractiona = effect->GetAttenuation(SegmentTemplate);
        AttenuateLength(fractiona,
                &p_mParticleTemplate->mSpawnLength);
        duration = mEndTime - mStartTime;
        mLengthEnv.Init(p_mParticleTemplate->mLengthEnvelope, duration);
    }
}

void rvLinkedParticle::FinishSpawn(rvBSE* effect, rvSegment* segment, float birthTime, float fraction, const idVec3& initOffset, const idMat3& initAxis)
{
    const rvSegmentTemplate *SegmentTemplate; // edi

    SegmentTemplate = segment->GetSegmentTemplate();
    if ( SegmentTemplate )
    {
        rvParticle::FinishSpawn(effect, segment, birthTime, fraction, initOffset, initAxis);
        HandleTiling(&SegmentTemplate->mParticleTemplate);
    }
}

void rvParticle::DoRenderBurnTrail(rvBSE *effect, const idMat3 *view, srfTriangles_s *tri, float time) {
    int trailCount; // ecx
    int v7; // edi
    float v9; // st7
    float v10; // [esp+0h] [ebp-1Ch]
    int i; // [esp+14h] [ebp-8h]
    float delta; // [esp+18h] [ebp-4h]
    float tria; // [esp+28h] [ebp+Ch]

    trailCount = mTrailCount;
    if (trailCount) {
        if (mTrailTime != 0.0) {
            v7 = 1;
            i = 1;
            delta = mTrailTime / (float) trailCount;
            if (trailCount + 1 > 1) {
                do {
                    v9 = time - (float) i * delta;
                    tria = v9;
                    if (v9 >= mStartTime && tria < mEndTime) {
                        v10 = (float) (mTrailCount - v7) / (float) mTrailCount;
                        Render(
                                effect,
                                NULL, //k??? TODO should setup particle template?
                                *view,
                                tri,
                                tria,
                                v10);
                    }
                    i = ++v7;
                } while (v7 < mTrailCount + 1);
            }
        }
    }
}

void rvParticle::HandleOrientation(const rvAngles *angles)
{
	mRotationEnv.Rotate(BSE::IdAngles(*angles));
}

// ---------------------------------------------------------------------------
//  rvParticle :: SetLengthUsingEndOrigin
// ---------------------------------------------------------------------------
void rvParticle::SetLengthUsingEndOrigin(
        const rvBSE* effect,
        const rvParticleParms* parms,
        float* length)
{
#if 1 //karin: length useEndOrigin //k??? TODO add
    //if(parms->mFlags & PPF_USE_END_ORIGIN)
    {
        if(mFlags & PTF_SEGMENT_LOCKED) // if follow
        {
            idVec3 dis = effect->mCurrentEndOrigin - effect->mCurrentOrigin; // world
            dis = effect->mCurrentAxisTransposed * dis; // local
            //dis = mInitAxis * dis;
            length[0] = dis[0];
            length[1] = dis[1];
            length[2] = dis[2];
        }
        else
        {
            idVec3 dis = effect->mOriginalEndOrigin - effect->mOriginalOrigin; // world
            dis = effect->mOriginalAxis.Transpose() * dis; // local
            //dis = mInitAxis * dis;
            length[0] = dis[0];
            length[1] = dis[1];
            length[2] = dis[2];
        }
    }
#else
    rvParticleParms::spawnFunctions[parms->mSpawnType](length, *parms, NULL, NULL); // jmarshall <-- no idea hex rays HATTES this call
#endif
}

#if 0
void rvOrientedParticle::HandleOrientation(const rvAngles *angles)
{
    mRotationEnv.Rotate(IdAngles(*angles));
}
#endif
