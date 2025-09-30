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

/* clamp constants */
static const float kMinSpawnRate = 0.002f;
static const float kMaxSpawnRate = 300.0f;

/* ------------------------------------------------------------ helpers --- */
namespace {
    template <typename T>
    ID_INLINE T Clamp(T v, T lo, T hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    }
}

/* ------------------------------------------------------------ statics --- */
float rvSegment::s_segmentBaseCost[11] = {
    /* 0-10 seg types */ 0, 0, 50, 50, 0, 20, 10, 10, 0, 30, 40
};

/* ------------------------------------------------------------- dtor ---- */
rvSegment::~rvSegment() {
    if (!mParticles)
        return;

    const rvSegmentTemplate* st = mEffectDecl->GetSegmentTemplate(mSegmentTemplateHandle);
    if (!st)
        return;

    for (int i = 0; i < mParticleList.Num()/*//k??? TODO using idList mParticleCount*/; ++i) {
        rvParticle* p = mParticleList[i];
        delete p;
        // p->~rvParticle();
    }
    mParticleList.Clear();
    //Mem_Free(mParticles);
    mParticles = NULL;
}

void rvSegment::Rewind(rvBSE* effect)
{
    float v2; // st7

    if (effect->mDuration == mSegEndTime - mSegStartTime)
    {
        v2 = mSegStartTime - (effect->mDuration + effect->mDuration);
        mSegStartTime = v2;
        mLastTime = v2;
    }
}

/* ------------------------------------------------------- ValidateSpawn -- */
void rvSegment::ValidateSpawnRates() {
    float& lo = mSecondsPerParticle.x;
    float& hi = mSecondsPerParticle.y;

    hi = Clamp(hi, kMinSpawnRate, kMaxSpawnRate);
    lo = Clamp(lo, hi, kMaxSpawnRate);   // lo may not exceed hi
}

/* ------------------------------------------------ GetSecondsPerParticle -- */
void rvSegment::GetSecondsPerParticle(rvBSE* effect,
    rvSegmentTemplate* st,
    rvParticleTemplate* pt)
{
    if (st->mDensity.y == 0.0f) {            // fixed-count segment?
        mCount = st->mCount;
    }
    else {
        /* density * volume, clamped ------------------------------------ */
        float volume = idMath::ClampFloat(kMinSpawnRate, 1000.0f, pt->GetSpawnVolume(effect));

        mCount.x = volume * st->mDensity.x;
        mCount.y = volume * st->mDensity.y;

        /* overall cap -------------------------------------------------- */
        if (st->mParticleCap != 0.0f) {
            mCount.x = Clamp(mCount.x, 1.0f, st->mParticleCap);
            mCount.y = Clamp(mCount.y, 1.0f, st->mParticleCap);
        }
    }

    /* convert count → seconds-per-particle for LOOPING / CONTINUOUS ---- */
    if (st->mSegType == SEG_EMITTER/* 2 */ || st->mSegType == SEG_TRAIL/* 4 */) {
        if (mCount.x != 0.0f) mSecondsPerParticle.x = 1.0f / mCount.x;
        if (mCount.y != 0.0f) mSecondsPerParticle.y = 1.0f / mCount.y;
        ValidateSpawnRates();
    }
}

/* ------------------------------------------------------------- InitTime -- */
void rvSegment::InitTime(rvBSE* effect,
    const rvSegmentTemplate* st,
    float timeNow)
{
    mFlags &= ~SFLAG_EXPIRED/* 1 */;   // clear “done” bit
    mSegStartTime = rvRandom::flrand(st->mLocalStartTime.x,
        st->mLocalStartTime.y)
        + timeNow;

    mSegEndTime = rvRandom::flrand(st->mLocalDuration.x,
        st->mLocalDuration.y)
        + mSegStartTime;

    /* if this segment dictates BSE duration --------------------------- */
    const bool segDefinesDuration =
        (st->mFlags & STFLAG_IGNORE_DURATION/* 0x10 */) == 0 ||
        ((effect->mFlags & EFLAG_LOOPING/* 1 */) == 0 &&
            !st->GetSoundLooping());

    if (segDefinesDuration) {
        effect->SetDuration(mSegEndTime - timeNow);
    }
}

/* ------------------------------------------------------------ Init ---- */
void rvSegment::Init(rvBSE* effect,
    rvDeclEffect* decl,
    int templateHandle,
    float timeNow)
{
    mFlags = 0;
    mEffectDecl = decl;
    mSegmentTemplateHandle = templateHandle;

    const rvSegmentTemplate* st = decl->GetSegmentTemplate(templateHandle);
	BSE_LOGFI(rvSegment::Init, "%s: handle=%d, template=%s", decl->GetName(), templateHandle, st ? st->GetName().c_str() : "NULL")
    if (!st) return;

    mLastTime = timeNow;
    mActiveCount = 0;
    mSecondsPerParticle = idVec2(0.0f, 0.0f);
    mCount = idVec2(1.0f, 1.0f);
    mSoundVolume = 0.0f;
    mFreqShift = 1.0f;
    mParticles = NULL;
    mParticleList.Clear();
    mParticleList.SetNum(0);

    InitTime(effect, const_cast<rvSegmentTemplate*>(st), effect->mStartTime);
    GetSecondsPerParticle(effect, const_cast<rvSegmentTemplate*>(st),
        const_cast<rvParticleTemplate*>(&st->mParticleTemplate));
    const_cast<rvSegmentTemplate*>(st)->mBSEEffect = effect;
}

/* --------------------------------------------------------- InsertParticle */
void rvSegment::InsertParticle(rvParticle* p,
                               const rvSegmentTemplate* st)
{
    if (st->mFlags & STFLAG_TEMPORARY/* 0x100 */)                 // invisible?
        return;

    ++mActiveCount;

    /* SRTF_SORTBYENDTIME flag? ---------------------------------------- */
    if (st->mFlags & 0x200) {
        p->mNext = mUsedHead;
        mUsedHead = p;
        return;
    }

    /* otherwise keep list sorted by EndTime --------------------------- */
    rvParticle* prev = NULL;
    rvParticle* cur = mUsedHead;
    while (cur && p->mEndTime > cur->mEndTime) {
        prev = cur;
        cur = cur->mNext;
    }
    p->mNext = cur;
    if (prev) prev->mNext = p;
    else        mUsedHead = p;
}

/* ----------------------------------------------------------- SpawnParticle */
rvParticle* rvSegment::SpawnParticle(rvBSE* effect,
    const rvSegmentTemplate* st,
    float birthTime,
    const idVec3* offset,
    const idMat3* axis)
{
    rvParticle* p = NULL;

    if (st->mFlags & STFLAG_TEMPORARY/* 0x100 */) {           // re-use internal array slot
        p = mParticles;
    }
    else {
        p = mFreeHead;
        if (p) mFreeHead = p->mNext;
    }
    if (!p) return NULL;

    p->FinishSpawn(effect, this, birthTime, birthTime, *offset, *axis);
    InsertParticle(p, st);
    return p;
}

/* ------------------------------------------------------ AttenuateDuration */
float rvSegment::AttenuateDuration(rvBSE* effect,
    const rvSegmentTemplate* st)
{
    return effect->GetAttenuation(st) * (mSegEndTime - mSegStartTime);
}


/* ---------------------------------------------------------------------------
   ❶  Attenuation helpers
   --------------------------------------------------------------------------- */
float rvSegment::AttenuateInterval(rvBSE* effect,
                                   const rvSegmentTemplate* st)
{
    const float minRate = mSecondsPerParticle.x;
    const float maxRate = mSecondsPerParticle.y;

    float rate = idMath::Lerp(maxRate, minRate, bse_scale.GetFloat());
    rate = Clamp(rate, maxRate, minRate);

    if (!(st->mFlags & STFLAG_ATTENUATE_EMITTER/* 0x40 */))
        return rate;                                 // no attenuation flag

    float att = effect->GetAttenuation(st);
    if (st->mFlags & STFLAG_INVERSE_ATTENUATE/* 0x80 */)
        att = 1.0f - att;                            // invert?

    return (att >= kMinSpawnRate) ? rate / att     // avoid div-0
        : 1.0f;
}

const rvSegmentTemplate* rvSegment::GetSegmentTemplate() const
{
    return mEffectDecl->GetSegmentTemplate(mSegmentTemplateHandle);
}

float rvSegment::AttenuateCount(rvBSE* effect,
    const rvSegmentTemplate* st,
    float min, float max)
{
    const float scaledMax = idMath::Lerp(min, max, bse_scale.GetFloat());
    float count = rvRandom::flrand(min, scaledMax);
    count = Clamp(count, min, max);

    if (!(st->mFlags & STFLAG_ATTENUATE_EMITTER/* 0x40 */))
        return count;

    float att = effect->GetAttenuation(st);
    if (st->mFlags & STFLAG_INVERSE_ATTENUATE/* 0x80 */)
        att = 1.0f - att;

    return att * count;
}

/* ---------------------------------------------------------------------------
   ❷  Simple per-frame particle list maintenance
   --------------------------------------------------------------------------- */
void rvSegment::UpdateSimpleParticles(float timeNow)
{
    while (mUsedHead &&
        mUsedHead->mEndTime - kMinSpawnRate <= timeNow)
    {
        rvParticle* dead = mUsedHead;
        mUsedHead = dead->mNext;

        dead->mNext = mFreeHead;
        mFreeHead = dead;
        --mActiveCount;
    }
}

void rvSegment::UpdateElectricityParticles(float timeNow)
{
    mActiveCount = 0;
    rvSegmentTemplate* st =
            (rvSegmentTemplate*)mEffectDecl->GetSegmentTemplate(mSegmentTemplateHandle);
    for (rvParticle* p = mUsedHead; p; p = p->mNext) {
        mActiveCount += p->Update(&st->mParticleTemplate, timeNow);
    }
}

void rvSegment::RefreshParticles(rvBSE* effect,
    const rvSegmentTemplate* st)
{
    if (!st->mParticleTemplate.UsesEndOrigin())
        return;

    for (rvParticle* p = mUsedHead; p; p = p->mNext) {
        p->Refresh(effect, st, &st->mParticleTemplate);
    }
}

/* ---------------------------------------------------------------------------
   ❸  Generic physics + lifetime handling
   --------------------------------------------------------------------------- */
void rvSegment::UpdateGenericParticles(rvBSE* effect,
                                       const rvSegmentTemplate* st,
    float timeNow)
{
    const bool smoker = st->GetSmoker();
    const bool looping = (st->mFlags & STFLAG_INFINITE_DURATION/* 0x20 */) != 0;

    rvParticle* prev = NULL;
    rvParticle* cur = mUsedHead;

    while (cur) {
        rvParticle* next = cur->mNext;
        bool kill = false;

        if (looping) {
            cur->RunPhysics(effect, st, timeNow);
            if (effect->mFlags & EFLAG_STOPPED/* 8 */)
                kill = true;
        }
        else {
            if (cur->mEndTime - kMinSpawnRate <= timeNow) {
                cur->CheckTimeoutEffect(effect, st, timeNow);
                kill = true;
            }
            else {
                kill = cur->RunPhysics(effect, st, timeNow);
            }
        }

        if ((effect->mFlags & EFLAG_STOPPED/* 8 */) && !(cur->mFlags & PTF_PERSIST/* 0x200000 */))
            kill = true;

        if (smoker && st->mTrailSegmentIndex >= 0)
            cur->EmitSmokeParticles(
                effect,
                effect->mSegments[st->mTrailSegmentIndex],
                timeNow);

        if (kill) {
            if (prev) prev->mNext = next;
            else        mUsedHead = next;

            cur->Destroy();
            cur->mNext = mFreeHead;
            mFreeHead = cur;
            --mActiveCount;
        }
        else {
            prev = cur;
        }
        cur = next;
    }
}

/* ---------------------------------------------------------------------------
   ❹  Segment-type-specific handling & updates
   --------------------------------------------------------------------------- */
void rvSegment::PlayEffect(rvBSE* effect,
                           const rvSegmentTemplate* st)
{
    const int idx =
        rvRandom::irand(0, st->mNumEffects - 1);

    game->PlayEffect(
        st->mEffects[idx],
        effect->mCurrentOrigin,
        effect->mCurrentAxis,
        /*joint*/ NULL,
        vec3_origin,
        /*surfId*/ NULL,
        /* predictBit = */false,
        EC_IGNORE,
        vec4_one);
}

void rvSegment::Handle(rvBSE* effect,
    const rvSegmentTemplate* st,
    float timeNow)
{
    switch (st->mSegType) {
	case SEG_EMITTER: // 2:      // continous
	case SEG_SPAWNER: // 3:      // burst
        if (effect->mFlags & EFLAG_ENDORIGINCHANGED/* 4 */)
            RefreshParticles(effect, st);
        break;

	case SEG_SOUND: // 5:      // sound
        effect->UpdateSoundEmitter(st, this);
        break;

	case SEG_LIGHT: // 7:      // light
        if (st->mFlags & STFLAG_ENABLED/* 1 */)
            HandleLight(effect, st, timeNow);   // existing engine fn
        break;

    default:    /* nothing */ break;
    }
}

void rvSegment::Handle(rvBSE* effect, float timeNow)
{
    const rvSegmentTemplate* st =
            mEffectDecl->GetSegmentTemplate(mSegmentTemplateHandle);
    if (!st || timeNow < mSegStartTime)
        return;

    Handle(effect, st, timeNow);
}

/* ---------------------------------------------------------------------------
   ❺  Per-frame UpdateParticles entry
   --------------------------------------------------------------------------- */
bool rvSegment::UpdateParticles(rvBSE* effect, float timeNow)
{
    const rvSegmentTemplate* st =
            mEffectDecl->GetSegmentTemplate(mSegmentTemplateHandle);
    if (!st) return false;

    Handle(effect, timeNow);

    const bool forceGeneric =
        (effect->mFlags & EFLAG_STOPPED/* 8 */) || (st->mFlags & 0x200);

    if (forceGeneric)
        UpdateGenericParticles(effect, st, timeNow);
    else
        UpdateSimpleParticles(timeNow);

    if (st->mParticleTemplate.mType == PTYPE_ELECTRIC/* 7 */)
        UpdateElectricityParticles(timeNow);

    /* debug HUD stats -------------------------------------------------- */
    if (bseLocal.DebugHudActive()) {
        bseLocal.mPerfCounters[PERF_NUM_PARTICLES]/* dword_1137DDB0 */ += mActiveCount;
        if (mUsedHead)
            bseLocal.mPerfCounters[PERF_NUM_TEXELS]/* dword_1137DDB4 */ +=
            st->GetTexelCount();
    }
    return mUsedHead != NULL;
}

/* ---------------------------------------------------------------------------
   ❻  Rendering helpers
   --------------------------------------------------------------------------- */
void rvSegment::RenderMotion(rvBSE* effect,
    const renderEffect_s* owner,
    rvRenderModelBSE* model,
    const rvParticleTemplate* pt,
    float timeNow)
{
    const int segments = mActiveCount *
        (static_cast<int>(ceilf(pt->mTrailCount.y)) + 1);

    srfTriangles_s* tri = R_AllocStaticTriSurf();
    R_AllocStaticTriSurfVerts(tri, 2 * segments + 2);
    R_AllocStaticTriSurfIndexes(tri, 12 * segments);

    const idMaterial* mat = pt->mTrailMaterial;

    for (rvParticle* p = mUsedHead; p; p = p->mNext)
        p->RenderMotion(effect, tri, owner, timeNow);

    R_BoundTriSurf(tri);
	BSE::AddSurface( model, /*id*/0, mat, tri, 0);
}

void rvSegment::RenderTrail(rvBSE* effect,
    const renderEffect_s* owner,
    rvRenderModelBSE* model,
    float timeNow)
{
    const rvSegmentTemplate* st =
            mEffectDecl->GetSegmentTemplate(mSegmentTemplateHandle);
    if (!st) return;

    const rvParticleTemplate* pt = &st->mParticleTemplate;
    if (ceilf(pt->mTrailCount.y) < 0 ||
        pt->mTrailTime.y < kMinSpawnRate ||
        pt->mTrailType != TRAIL_MOTION/* 2 */)
        return;

    RenderMotion(effect, owner, model, pt, timeNow);
}

void rvSegment::Render(rvBSE* effect,
    const renderEffect_s* owner,
    rvRenderModelBSE* model,
    float timeNow)
{
    const rvSegmentTemplate* st =
            mEffectDecl->GetSegmentTemplate(mSegmentTemplateHandle);
    if (!st) return;

    const size_t bytesNeeded =
        (mActiveCount * st->mParticleTemplate.mVertexCount) * sizeof(idDrawVert);

    if (bytesNeeded > 1 * 1024 * 1024) {     // >1 MiB safety
        common->Warning("^4BSE:^1 '%s'\nMore than a MiB of vertex data",
            effect->GetDeclName());
        return;
    }

    BSE_LOGFI(rvSegment::Render, "%s: active=%d, vertex=%d, index=%d, material=%s", st->GetName().c_str(), mActiveCount, st->mParticleTemplate.mVertexCount, st->mParticleTemplate.mIndexCount, st->mParticleTemplate.mMaterial ? st->mParticleTemplate.mMaterial->GetName() : "NULL");
	//k??? TODO add check 0 memory
	if(!st->mParticleTemplate.mVertexCount || !st->mParticleTemplate.mIndexCount || !mActiveCount)
		return;

    srfTriangles_s* tri = R_AllocStaticTriSurf();
    R_AllocStaticTriSurfVerts(tri, mActiveCount * st->mParticleTemplate.mVertexCount);
    R_AllocStaticTriSurfIndexes(tri, mActiveCount * st->mParticleTemplate.mIndexCount);
    const idMaterial *shader = st->mParticleTemplate.mMaterial; //k??? TODO add

    /* build view-aligned axes once per call --------------------------- */
    idMat3 viewAxis;
    float modelMat[16]; //karin: sizeof(idMat3) < sizeof(float[16])
    R_AxisToModelMatrix(owner->axis, owner->origin, modelMat);
    R_GlobalVectorToLocal(modelMat, effect->mViewAxis[1], viewAxis[1]);
    R_GlobalVectorToLocal(modelMat, effect->mViewAxis[2], viewAxis[2]);
    idVec3 toEye = effect->mViewOrg - owner->origin;
    viewAxis[0].x = toEye * owner->axis[0];
    viewAxis[0].y = toEye * owner->axis[1];
    viewAxis[0].z = toEye * owner->axis[2];

    /* draw each particle --------------------------------------------- */
    for (rvParticle* p = mUsedHead; p; p = p->mNext) {
        if (st->mFlags & STFLAG_INFINITE_DURATION/* 0x20 */)
            p->mEndTime = timeNow + 1.0f;

        BSE_LOGFI(rvSegment::Render Particle, "%s: decl=%s, template=%s, type=%s", typeid(*p).name(), mEffectDecl->GetName(), st->GetName().c_str(), st->mParticleTemplate.PTypeName());
        if (p->Render(effect, &st->mParticleTemplate, viewAxis, tri, timeNow) &&
            st->mParticleTemplate.mTrailType == TRAIL_BURN/* 1 */)
        {
            p->RenderBurnTrail(effect, &viewAxis, tri, timeNow);
        }
    }

    R_BoundTriSurf(tri);
	BSE::AddSurface(model, /*id*/0, /*tri->*/shader, tri, 0);
}

/* ---------------------------------------------------------------------------
   *  Book-keeping / queries
   --------------------------------------------------------------------------- */
float rvSegment::EvaluateCost() const
{
    const rvSegmentTemplate* st =
            mEffectDecl->GetSegmentTemplate(mSegmentTemplateHandle);
    if (!st || !(st->mFlags & STFLAG_ENABLED/* 1 */))
        return 0.0f;

    const int  segType = st->mSegType;
    float cost = s_segmentBaseCost[segType];

    if (st->mParticleTemplate.mType) {
        cost += st->mParticleTemplate.CostTrail(static_cast<float>(mActiveCount));
        if (st->mParticleTemplate.mFlags & 0x200)
            cost += (float)mActiveCount * 80.0f;
    }
    return cost;
}

bool rvSegment::Active() const
{
    const rvSegmentTemplate* st =
            mEffectDecl->GetSegmentTemplate(mSegmentTemplateHandle);

    return st && (st->mFlags & STFLAG_HASPARTICLES/* 4 */) && mActiveCount;
}

bool rvSegment::GetLocked() const
{
    const rvSegmentTemplate* st =
            mEffectDecl->GetSegmentTemplate(mSegmentTemplateHandle);
    return st ? (st->mFlags & STFLAG_LOCKED /* 2 */) != 0 : false;
}

/* ---------------------------------------------------------------------------
   *  Particle array allocation
   --------------------------------------------------------------------------- */
rvParticle* rvSegment::InitParticleArray(rvBSE* effect)
{
    /* decide how many -------------------------------------------------- */
    const rvSegmentTemplate* st =
            mEffectDecl->GetSegmentTemplate(mSegmentTemplateHandle);
    if (!st) return NULL;

    const int requested =
        (effect->mFlags & EFLAG_LOOPING/* 1 */) ? mLoopParticleCount : mParticleCount;

    const int maxAllowed = bse_maxParticles.GetInteger();
    int count = idMath::ClampInt(0, maxAllowed, requested);

    if (requested > maxAllowed) {
        common->Warning("^4BSE:^1 '%s'\nMore than %d particles required (%d)",
            effect->GetDeclName(),
            maxAllowed, requested);
    }

    BSE_LOGFI(rvSegment::InitParticleArray, "%s: count -> %d ? %d : %d = %d", st->GetName().c_str(), effect->mFlags & EFLAG_LOOPING, mLoopParticleCount, mParticleCount, count);
    if (count == 0) return NULL;

#if 1
    byte* mem = NULL;

    const int pType = st->mParticleTemplate.mType;
#define BSE_PARTICLE_CASE(type, clazz) \
    case type: \
        BSE_LOGFI(rvParticle::class, "%s", #clazz) \
        mParticleList.SetNum(count); \
        for(int i = 0; i < count; i++) { \
            mParticleList[i] = new clazz; \
        } \
        for(int i = 0; i < count; i++) { \
            clazz *p = static_cast<clazz *>(mParticleList[i]); \
            if (i < count - 1) \
                p->mNext = mParticleList[i + 1]; \
            else \
                p->mNext = NULL; \
        } \
        break;

    switch (pType) {
        BSE_PARTICLE_CASE(PTYPE_LINE, rvLineParticle)
        BSE_PARTICLE_CASE(PTYPE_ORIENTED, rvOrientedParticle)
        BSE_PARTICLE_CASE(PTYPE_DECAL, rvDecalParticle)
        BSE_PARTICLE_CASE(PTYPE_MODEL, rvModelParticle)
        BSE_PARTICLE_CASE(PTYPE_LIGHT, rvLightParticle)
        BSE_PARTICLE_CASE(PTYPE_ELECTRIC, rvElectricityParticle)
        BSE_PARTICLE_CASE(PTYPE_LINKED, rvLinkedParticle)
        BSE_PARTICLE_CASE(PTYPE_DEBRIS, rvDebrisParticle)
        BSE_PARTICLE_CASE(PTYPE_SPRITE, rvSpriteParticle)
        default:
		common->Warning("^4BSE:^1 '%s'::'%s' type %d Segment not match", mEffectDecl->GetName(), st->GetName().c_str(), pType);
            return NULL;
    }
#undef BSE_PARTICLE_CASE

    return mParticleList[0];
#else
    byte* mem = NULL;

    const int pType = st->mParticleTemplate.mType;
#define BSE_PARTICLE_CASE(type, clazz) \
    case type: \
        mem = (byte*)Mem_Alloc(sizeof(clazz) * count + 4); \
        if (!mem) return NULL; \
        for(int i = 0; i < count; i++) { \
            clazz *p = &((clazz *)(mem + 4))[i]; \
            *p = clazz(); \
            if (i < count - 1) \
                p->mNext = &((clazz *)(mem + 4))[i + 1]; \
            else \
                p->mNext = NULL; \
        } \
        break;

    switch (pType) {
        BSE_PARTICLE_CASE(2, rvLineParticle)
        BSE_PARTICLE_CASE(3, rvOrientedParticle)
        BSE_PARTICLE_CASE(4, rvDecalParticle)
        BSE_PARTICLE_CASE(5, rvModelParticle)
        BSE_PARTICLE_CASE(6, rvLightParticle)
        BSE_PARTICLE_CASE(7, rvElectricityParticle)
        BSE_PARTICLE_CASE(8, rvLinkedParticle)
        BSE_PARTICLE_CASE(9, rvDebrisParticle)
        BSE_PARTICLE_CASE(1, rvSpriteParticle)
        default:
            return NULL;
    }
#undef BSE_PARTICLE_CASE

    *reinterpret_cast<int*>(mem) = count;          // store length
    byte* base = mem + 4;

    return reinterpret_cast<rvParticle*>(base);
#endif
}

void rvSegment::InitParticles(rvBSE* effect)
{
    if (mEffectDecl->GetSegmentTemplate(mSegmentTemplateHandle)) {
        mParticles = InitParticleArray(effect);
        mUsedHead = NULL;
        mFreeHead = mParticles;
        mActiveCount = 0;
    }
}

ID_INLINE static bool BSE_DecalDebug() {
    return bse_debug.GetBool();
}

void DispatchDecalParms(rvParticle *p, idVec4 &tint, idVec3 &size, idVec3& rotation, float time)
{
	size.Set(0.0f, 0.0f, 0.0f);
	rotation.Set(0.0f, 0.0f, 0.0f);
	tint.Set(1.0f, 1.0f, 1.0f, 1.0f);

    if (!p)
		return;
    float eval = time - p->mStartTime;
    if (time >= p->mEndTime - kEpsilon) eval = (p->mEndTime - p->mStartTime) - kEpsilon;
    if (time < p->mStartTime - kEpsilon || time >= p->mEndTime) return;

    p->mTintEnv.Evaluate(eval, tint.ToFloatPtr());
    p->mFadeEnv.Evaluate(eval, &tint.w);
    p->EvaluateSize(eval, size.ToFloatPtr());
    p->EvaluateRotation(eval, rotation.ToFloatPtr());

	size.z = 8.0f; //DOOM3 Fx
    //const int packed = HandleTint(effect, tint, alphaOverride);
}

void rvSegment::CreateDecal(rvBSE* effect, float timeNow)
{
    if (!bse_render.GetBool())
        return;

    const rvSegmentTemplate* st =
            mEffectDecl->GetSegmentTemplate(mSegmentTemplateHandle);
    if (!st) return;

    /* only once ------------------------------------------------------- */
    if (mFlags & SFLAG_EXPIRED/* 1 */)
        return;

    /* -------------------------------------------------------------- dbg */
    if (BSE_DecalDebug()) {
        common->Printf("BSE: Decal from segment %d (%s)\n",
            mSegmentTemplateHandle,
            st->mParticleTemplate.mMaterial ?
            st->mParticleTemplate.mMaterial->GetName() :
            "<no material>");
    }

    /* ------------------------------------------------------ parameters */
    idVec3  origin = effect->mCurrentOrigin;
    idMat3  _axis = effect->mCurrentAxis;

    idVec3  size;      // filled by template callback
    idVec3  rotation;  // Euler (deg)
    idVec4  tint;      // RGBA 0-1
					   
    rvParticle *v22 = SpawnParticle(effect, st, timeNow, &vec3_origin, &mat3_identity);
    DispatchDecalParms(v22, tint, size, rotation, timeNow); /* ← wrapper around
                                                       script callbacks in
                                                       original code */

                                                       /* --------------------------------------------------- build quad    */

    /* apply Z-rotation (only rotation.z mattered in original) */
    const float ang = DEG2RAD(rotation.z);
    const float c = idMath::Cos(ang);
    const float s = idMath::Sin(ang);

    idFixedWinding w;

#if 1 // DOOM3 Fx

	// winding orientation
	idMat3 axis, axistemp;
	idVec3 dir = -_axis[0];
	axis[2] = dir;
	axis[2].Normalize();
	axis[2].NormalVectors(axistemp[0], axistemp[1]);
	axis[0] = axistemp[ 0 ] * c + axistemp[ 1 ] * -s;
	axis[1] = axistemp[ 0 ] * -s + axistemp[ 1 ] * -c;

	float projectionDepth = size.z;
	idVec3 windingOrigin = origin + projectionDepth * axis[2];
	idVec3 projectionOrg = origin - projectionDepth * axis[2];

	w.Clear();

#if 0 // error
    const float hw = size.x * 0.5f;
    const float hh = size.y * 0.5f;

	static idVec3 decalWinding[4] = {
        idVec3(hw,  hh, 0),
        idVec3(-hw,  hh, 0),
        idVec3(-hw, -hh, 0),
        idVec3(hw, -hh, 0)
	};

	w += idVec5(windingOrigin + (axis * decalWinding[0]), idVec2(1, 1));
	w += idVec5(windingOrigin + (axis * decalWinding[1]), idVec2(0, 1));
	w += idVec5(windingOrigin + (axis * decalWinding[2]), idVec2(0, 0));
	w += idVec5(windingOrigin + (axis * decalWinding[3]), idVec2(1, 0));

#else

	static idVec3 decalWinding[4] = {
		idVec3(1.0f,  1.0f, 0.0f),
		idVec3(-1.0f,  1.0f, 0.0f),
		idVec3(-1.0f, -1.0f, 0.0f),
		idVec3(1.0f, -1.0f, 0.0f)
	};
	float scale = Max(size.x, size.y);
	w += idVec5(windingOrigin + (axis * decalWinding[0]) * scale, idVec2(1, 1));
	w += idVec5(windingOrigin + (axis * decalWinding[1]) * scale, idVec2(0, 1));
	w += idVec5(windingOrigin + (axis * decalWinding[2]) * scale, idVec2(0, 0));
	w += idVec5(windingOrigin + (axis * decalWinding[3]) * scale, idVec2(1, 0));
#endif

#else // jmarshall

    const float hw = size.x * 0.5f;
    const float hh = size.y * 0.5f;

	const idMat3 axis = _axis;

    /* local-space corners (XY plane) */
    idVec3 local[4] = {
        idVec3(hw,  hh, 0),
        idVec3(-hw,  hh, 0),
        idVec3(-hw, -hh, 0),
        idVec3(hw, -hh, 0)
    };
	static const idVec2 texcoords[] = {
		idVec2(1, 1),
		idVec2(0, 1),
		idVec2(0, 0),
		idVec2(1, 0),
	};

    for(int i = 0; i < sizeof(local) / sizeof(local[0]); i++) {
        idVec3 &v = local[i];
        const float x = v.x * c - v.y * s;
        const float y = v.x * s + v.y * c;
        v.x = x; v.y = y;
        v = origin + v * axis;             // to world
    }

    /* ------------------------------------------------ winding & decal */
    Resize(w, 4);
    for (int i = 0; i < 4; ++i)
	{
        w[i] = local[i];
		w[i][3] = texcoords[i].x;
		w[i][4] = texcoords[i].y;
	}

    /* thickness == size.z, direction == axis[2] (model “forward”) */
    const idVec3 projectionDir = axis[2];
    const idVec3 projectionOrg = origin - projectionDir * size.z * 0.5f;
    const float  projectionDepth = size.z;
#endif

    session->rw->ProjectDecalOntoWorld(
        w,
        projectionOrg,
        true,/*projectionDir,*/
        projectionDepth * 0.5f, //karin: * 0.5f on DOOM3
        st->mParticleTemplate.mMaterial,
        int(timeNow * 1000.0f)/*tint*/ //karin: convert to game time
        );

    /* ---------------------------------------- mark done so it won’t
       repeat; also tick effect duration if this segment dictates it. */
    mFlags |= SFLAG_EXPIRED/* 1 */;
    if (!(st->mFlags & STFLAG_INFINITE_DURATION/* 0x20 */))
        effect->SetDuration((mSegEndTime - timeNow) + st->mParticleTemplate.mDuration.y);
}


void rvSegment::ResetTime(rvBSE *effect, float time) {
    const rvSegmentTemplate *SegmentTemplate; // eax

    SegmentTemplate = mEffectDecl->GetSegmentTemplate(mSegmentTemplateHandle);
    if (SegmentTemplate) {
        if ((SegmentTemplate->mFlags & STFLAG_INFINITE_DURATION/* 0x20 */) == 0)
            InitTime(effect, SegmentTemplate, time);
    }
}

void rvSegment::Advance(rvBSE *effect) {
    float v2; // st7

    v2 = effect->mDuration + mSegStartTime;
    mSegStartTime = v2;
    mLastTime = v2;
}

bool rvSegment::Check(rvBSE *effect, float time)
{
    bool v4; // zf
    int flags; // eax
    const rvSegmentTemplate *SegmentTemplate; // eax
    const rvSegmentTemplate *v8; // edi
    float v9; // st7
    unsigned char v11; // c0
    unsigned char v12; // c2
    idSoundEmitter *ReferenceSound; // ebx
    int v14; // eax
    idSoundEmitter *v15; // ebp
    idSoundEmitter *v16; // ebp
    float v18; // [esp+0h] [ebp-1Ch]
    float v19; // [esp+0h] [ebp-1Ch]
    float timea; // [esp+4h] [ebp-18h]
    float timeb; // [esp+4h] [ebp-18h]
    float timec; // [esp+4h] [ebp-18h]
    float scale; // [esp+8h] [ebp-14h]
    float scalea; // [esp+8h] [ebp-14h]
    float scaleb; // [esp+8h] [ebp-14h]
    float spawnTime; // [esp+18h] [ebp-4h]
    int effecta; // [esp+20h] [ebp+4h]
    float count; // [esp+24h] [ebp+8h]
    int counta; // [esp+24h] [ebp+8h]
    int countb; // [esp+24h] [ebp+8h]

    v11 = v12 = false; //k??? TODO not initialized in Q4D

    v4 = (mFlags & SFLAG_EXPIRED/* 1 */) == 0;
    spawnTime = mLastTime;
    mLastTime = time;
    if ( !v4 || (effect->mFlags & EFLAG_STOPPED/* 8 */) != 0 )
    {
        flags = 1;
    }
    else if ( time >= mSegStartTime )
    {
        SegmentTemplate = mEffectDecl->GetSegmentTemplate(mSegmentTemplateHandle);
        v8 = SegmentTemplate;
        if ( !SegmentTemplate || SegmentTemplate->DetailCull() )
        {
            flags = 1;
        }
        else
        {
            switch ( v8->mSegType )
            {
				case SEG_EFFECT: // 1:
                    if ( (v8->mFlags & STFLAG_ENABLED/* 1 */) == 0 )
                        break;
                    PlayEffect(effect, v8);
                    mFlags |= SFLAG_EXPIRED/* 1u */;
                    return mFlags & SFLAG_EXPIRED/* 1 */;
				case SEG_EMITTER: // 2:
                    if ( !effect->CanInterpolate() )
                        return mFlags & SFLAG_EXPIRED/* 1 */;
                    count = time + 0.016000001f;
                    if ( mSegEndTime - 0.0020000001f <= count )
                        count = mSegEndTime;
                    v9 = spawnTime;
                    if ( spawnTime < count )
                    {
                        do
                        {
                            if ( v9 >= mSegStartTime )
                                SpawnParticle(effect, v8, spawnTime, &vec3_origin, &mat3_identity);
                            v9 = AttenuateInterval(effect, v8) + spawnTime;
                            spawnTime = v9;
                        }
                        while ( v11 | v12 );
                    }
                    if ( mSegEndTime - 0.0020000001f <= count )
                        mFlags |= SFLAG_EXPIRED/* 1u */;
                    flags = mFlags;
                    mLastTime = v9;
                    flags = flags & SFLAG_EXPIRED/* 1 */;
                    return flags;
				case SEG_SPAWNER: // 3:
                    effecta = (int)AttenuateCount(effect, v8, mCount.x, mCount.y);
                    SpawnParticles(effect, v8, mSegStartTime, effecta);
                    mFlags |= SFLAG_EXPIRED/* 1u */;
                    return mFlags & SFLAG_EXPIRED/* 1 */;
				case SEG_TRAIL: // 4:
				case SEG_DELAY: // 8:
                    break;
				case SEG_SOUND: // 5:
                    if ( (v8->mFlags & STFLAG_ENABLED/* 1 */) == 0 )
                        break;
                    ReferenceSound = effect->GetReferenceSound(SOUNDWORLD_GAME/* 1 */);
                    if ( ReferenceSound )
                    {
                        mSoundVolume = v8->GetSoundVolume();
                        mFreqShift = v8->GetFreqShift();
                        effect->UpdateSoundEmitter(v8, this);
                        if ( v8->GetSoundLooping() )
                        {
                            v14 = mFlags;
                            if ( (v14 & SFLAG_SOUNDPLAYING/* 2 */) == 0 )
                            {
                                v14 = v14 | SFLAG_SOUNDPLAYING/* 2 */;
                                mFlags = v14;
                                v15 = ReferenceSound;
                                counta = mSegmentTemplateHandle + 1;
                                v18 = rvRandom::flrand(0.0, 1.0);
                                v15->StartSound(
                                        v8->mSoundShader,
                                        counta,
                                        v18,
                                        SSF_LOOPING/* 32 */);
                                mFlags |= SFLAG_EXPIRED/* 1u */;
                                return mFlags & SFLAG_EXPIRED/* 1 */;
                            }
                        }
                        else
                        {
                            v16 = ReferenceSound;
                            countb = mSegmentTemplateHandle + 1;
                            v19 = rvRandom::flrand(0.0, 1.0);
                            v16->StartSound(
                                    v8->mSoundShader,
                                    countb,
                                    v19,
                                    0);
                        }
                    }
                    mFlags |= SFLAG_EXPIRED/* 1u */;
                    flags = mFlags & SFLAG_EXPIRED/* 1 */;
                    break;
				case SEG_DECAL: // 6:
                    if ( (v8->mFlags & STFLAG_ENABLED/* 1 */) == 0 || !rvBSEManagerLocal::g_decals->GetBool() )
                        break;
                    CreateDecal(effect, mSegStartTime);
                    mFlags |= SFLAG_EXPIRED/* 1u */;
                    return mFlags & SFLAG_EXPIRED/* 1 */;
				case SEG_LIGHT: // 7:
                    if ( (v8->mFlags & STFLAG_ENABLED/* 1 */) == 0 )
                        break;
                    InitLight(effect, v8, mSegStartTime);
                    mFlags |= SFLAG_EXPIRED/* 1u */;
                    return mFlags & SFLAG_EXPIRED/* 1 */;
				case SEG_DV: // 9:
                    if ( (v8->mFlags & STFLAG_ENABLED/* 1 */) == 0 )
                        break;
                    scale = effect->GetOriginAttenuation(v8);
                    timea = AttenuateDuration(effect, v8) + mSegStartTime;
                    bseLocal.SetDoubleVisionParms(timea, scale);
                    mFlags |= SFLAG_EXPIRED/* 1u */;
                    return mFlags & SFLAG_EXPIRED/* 1 */;
				case SEG_SHAKE: // 0xA:
                    if ( (v8->mFlags & STFLAG_ENABLED/* 1 */) == 0 )
                        break;
                    scalea = effect->GetOriginAttenuation(v8);
                    timeb = AttenuateDuration(effect, v8) + mSegStartTime;
                    bseLocal.SetShakeParms(timeb, scalea);
                    mFlags |= SFLAG_EXPIRED/* 1u */;
                    return mFlags & SFLAG_EXPIRED/* 1 */;
				case SEG_TUNNEL: // 0xB:
                    if ( (v8->mFlags & STFLAG_ENABLED/* 1 */) != 0 )
                    {
                        scaleb = effect->GetOriginAttenuation(v8);
                        timec = AttenuateDuration(effect, v8) + mSegStartTime;
                        bseLocal.SetTunnelParms(timec, scaleb);
                    }
                    mFlags |= SFLAG_EXPIRED/* 1u */;
                    return mFlags & SFLAG_EXPIRED/* 1 */;
                default:
                    return mFlags & SFLAG_EXPIRED/* 1 */;
            }

            // $L119003:
            mFlags |= SFLAG_EXPIRED/* 1u */;
            return mFlags & SFLAG_EXPIRED/* 1 */;
        }
    }
    else
    {
        flags = 0;
    }
    return flags;
}

void rvSegment::CalcCounts(rvBSE *effect, float time) {
    rvSegment *v3; // ebp
    const rvSegmentTemplate *SegmentTemplate; // eax
    const rvSegmentTemplate *v5; // ebx
    int mSegType; // eax
    int mType; // ecx
    int v8; // esi
    int v9; // edi
    float y; // st7
    float v11; // st7
    //int v12; // ebp
    const char *v13; // eax
    int v14; // eax
    float _X; // [esp+0h] [ebp-30h]
    float _Xa; // [esp+0h] [ebp-30h]
    float _Xb; // [esp+0h] [ebp-30h]
    float _Xc; // [esp+0h] [ebp-30h]
    float particleMaxDuration; // [esp+18h] [ebp-18h]
    float effectMinDuration; // [esp+1Ch] [ebp-14h]
    float duration; // [esp+24h] [ebp-Ch]
    rvDeclEffect *effectDecl; // [esp+28h] [ebp-8h]

    v3 = this;
    effectDecl = (rvDeclEffect *) this->mEffectDecl;
    SegmentTemplate = effectDecl->GetSegmentTemplate(mSegmentTemplateHandle);
    v5 = SegmentTemplate;
    if (SegmentTemplate) {
        mSegType = SegmentTemplate->mSegType;
        if (mSegType != SEG_TRAIL/* 4 */) {
            mType = v5->mParticleTemplate.mType;
            if (mType) {
                v8 = 0;
                particleMaxDuration = v5->mParticleTemplate.mDuration.y + 0.016000001f;
                v9 = 0;
                duration = 0.0f;
                effectMinDuration = effectDecl->mMinDuration;
                switch (mSegType) {
					case SEG_EMITTER: // 2:
                        if (mType == PTYPE_DEBRIS/* 9 */) {
                            v8 = 1;
                            v9 = 1;
                        } else {
                            y = particleMaxDuration;
                            if (particleMaxDuration > v5->mLocalDuration.y)
                                y = v5->mLocalDuration.y;
                            v11 = y + 0.016000001f;
                            duration = v11;
                            _X = v11 / v3->mSecondsPerParticle.y;
                            v9 = (int) ceilf(_X) + 1;
                            v8 = v9;
                            if (effectMinDuration < particleMaxDuration) {
                                _Xa = (float)v9 / effectMinDuration * particleMaxDuration;
                                v8 = (int) ceilf(_Xa) + 1;
                            }
                        }
                        break;
					case SEG_SPAWNER: // 3:
                        if (mType == PTYPE_DEBRIS/* 9 */) {
                            v8 = 1;
                            v9 = 1;
                        } else {
                            v9 = (int) ceilf(v3->mCount.y);
                            v8 = v9;
                            if (effectMinDuration != 0.0
                                && (v5->mFlags & STFLAG_INFINITE_DURATION/* 0x20 */) == 0
                                && effectMinDuration < particleMaxDuration) {
                                _Xb = particleMaxDuration / effectMinDuration;
                                v8 = v9 * ((int) ceilf(_Xb) + 1) + 1;
                            }
                        }
                        break;
					case SEG_TRAIL: // 4:
                        break;
					case SEG_DECAL: // 6:
                    case SEG_LIGHT: // 7:
                        v9 = 1;
                        v8 = 1;
                        break;
                    default:
                        v9 = 0;
                        v8 = 0;
                        break;
                }
                if (v5->mSegType != SEG_TRAIL/* 4 */) {
                    v3->mParticleCount = v9;
                    v3->mLoopParticleCount = v8;
                    if ((v5->mFlags & STFLAG_HASPARTICLES/* 4 */) != 0) {
                        if (!v9 || !v8) {
                            v13 = effectDecl->GetName();
                            common->Warning(
                                    "^4BSE:^1 '%s'\nSegment with no particles for effect",
                                    v13);
                            v3 = this;
                        }
                        v14 = v5->mSegType;
                        if (v14 == 2 || v14 == 3)
                            v3->CalcTrailCounts(effect, v5, &v5->mParticleTemplate, duration);
                    }
                }
                if ((effect->mFlags & EFLAG_LOOPING/* 1 */) == 0) {
                    _Xc = v3->mSegEndTime - time + v5->mParticleTemplate.mDuration.y;
                    effect->SetDuration(_Xc);
                }
                if (bse_debug.GetInteger() == 2)
                    common->Printf(
                            "BSE: Segment %s: Count: %d Looping: %d\n",
                            rvBSEManagerLocal::mSegmentNames[v5->mSegType],
                            v9,
                            v8);
            }
        }
    }
}

void rvSegment::AddToParticleCount(rvBSE *effect, int count, int loopCount, float duration) {
    const rvSegmentTemplate *SegmentTemplate; // eax
    int v7; // [esp+Ch] [ebp-4h]

    SegmentTemplate = mEffectDecl->GetSegmentTemplate(mSegmentTemplateHandle);
    if (SegmentTemplate) {
        if (SegmentTemplate->mParticleTemplate.mDuration.y > duration)
            duration = SegmentTemplate->mParticleTemplate.mDuration.y;
        v7 = (int) ceil((duration + 0.016000001f) / mSecondsPerParticle.y);
        mParticleCount += count * (v7 + 1);
        mLoopParticleCount += loopCount * (v7 + 1);
    }
}

void rvSegment::CalcTrailCounts(rvBSE *effect, const rvSegmentTemplate *st, const rvParticleTemplate *pt, float duration) {
    int mTrailSegmentIndex; // eax

    mTrailSegmentIndex = st->mTrailSegmentIndex;
    if (mTrailSegmentIndex >= 0)
        effect->mSegments[mTrailSegmentIndex]->AddToParticleCount(
                effect,
                mParticleCount,
                mLoopParticleCount,
                duration);
}

void rvSegment::InitLight(rvBSE *effect, const rvSegmentTemplate *st, float time)
{
    if (!mUsedHead) {
        SpawnParticle(effect, st, time, &vec3_origin, &mat3_identity);
        mUsedHead->InitLight(
                effect,
                st,
                time);
    }
}

void rvSegment::SpawnParticles(rvBSE *effect, const rvSegmentTemplate *st, float birthTime, int count) {
    const rvSegmentTemplate *SegmentTemplate; // eax
    rvParticle *particles; // esi
    float v8; // [esp+0h] [ebp-20h]
    int i; // [esp+1Ch] [ebp-4h]

    if (count > bse_maxParticles.GetInteger())
        count = bse_maxParticles.GetInteger();
    for (i = 0; i < count; ++i) {
        SegmentTemplate = mEffectDecl->GetSegmentTemplate(mSegmentTemplateHandle);
        if (SegmentTemplate) {
            if ((SegmentTemplate->mFlags & STFLAG_TEMPORARY/* 0x100 */) != 0) {
                particles = mParticles;
            }
            else
            {
                particles = mFreeHead;
                if (particles)
                    mFreeHead = particles->mNext;
            }
            if (particles) {
                v8 = (float) i / (float) count;
                particles->FinishSpawn(
                        effect,
                        this,
                        birthTime,
                        v8,
                        vec3_origin,
                        mat3_identity);
                InsertParticle(particles, st);
            }
        }
    }
}
