/* ===========================================================================
   rvSegment.h ― cleaned declaration
   ========================================================================== */
#pragma once

class rvBSE;
class rvDeclEffect;
class rvParticle;
class rvParticleTemplate;
class rvSegmentTemplate;
class rvRenderModelBSE;

/* -------------------------------------------------------------------------
   rvSegment
   ------------------------------------------------------------------------- */
class rvSegment {
public:
    /* --- construction --------------------------------------------------- */
    rvSegment()
    : mSegmentTemplateHandle(-1),
      mEffectDecl(NULL),
      mSegStartTime(0.0f),
      mSegEndTime(0.0f),
      mSecondsPerParticle(idVec2(0.0f, 0.0f)),
      mCount(idVec2(0.0f, 0.0f)),
      mSoundVolume(0.0f),
      mFreqShift(0.0f),
      mLastTime(0.0f),
      mActiveCount(0)
    {
        mFlags = 0;
        mParticles = NULL;
        mUsedHead = NULL;
        mFreeHead = NULL;
        mParticleCount = 0;
        mLoopParticleCount = 0;
    }
    ~rvSegment();                                             /* dtor below */

    /* --- initialisation ------------------------------------------------- */
    void            Init(rvBSE* effect,
        rvDeclEffect* decl,
        int segmentTemplateHandle,
        float timeNow);

    void            Rewind(rvBSE* effect);

    void            InitParticles(rvBSE* effect);          /* alloc array */
    void            InitTime(rvBSE* effect,
        const rvSegmentTemplate* st,
        float timeNow);
    void            ResetTime(rvBSE* effect, float timeNow);
    void            Advance(rvBSE* effect);          /* one loop   */

    /* --- per-frame update ----------------------------------------------- */
    bool            Check(rvBSE* effect, float timeNow);
    bool            UpdateParticles(rvBSE* effect, float timeNow);
    void            CalcCounts(rvBSE* effect, float timeNow);

    /* --- rendering ------------------------------------------------------ */
    void            Render(rvBSE* effect,
        const renderEffect_s* owner,
        rvRenderModelBSE* model,
        float timeNow);
    void            RenderTrail(rvBSE* effect,
        const renderEffect_s* owner,
        rvRenderModelBSE* model,
        float timeNow);

    /* --- misc helpers --------------------------------------------------- */
    float           EvaluateCost() const;
    bool            Active() const;
    bool            GetLocked() const;

    /* --- particle helpers ---------------------------------------------- */
    rvParticle* SpawnParticle(rvBSE* effect,
        const rvSegmentTemplate* st,
        float birthTime,
        const idVec3* localOffset = &vec3_origin,
        const idMat3* localAxis = &mat3_identity);
    void            SpawnParticles(rvBSE* effect,
        const rvSegmentTemplate* st,
        float birthTime,
        int count);
    void            InsertParticle(rvParticle* p,
        const rvSegmentTemplate* st);

    /* --- attenuation utilities ----------------------------------------- */
    float           AttenuateDuration(rvBSE* effect, const rvSegmentTemplate* st);
    float           AttenuateInterval(rvBSE* effect, const rvSegmentTemplate* st);
    float           AttenuateCount(rvBSE* effect,
        const rvSegmentTemplate* st,
        float min, float max);

    const rvSegmentTemplate* GetSegmentTemplate() const;

    /* --- static data ---------------------------------------------------- */
    static float    s_segmentBaseCost[11];   /* indexed by SegType enum */

private:
    /* rule-of-five disabled */
    rvSegment(const rvSegment&);
    rvSegment& operator= (const rvSegment&);

    /* --- internal helpers ---------------------------------------------- */
    void            ValidateSpawnRates();
    void            GetSecondsPerParticle(rvBSE* effect,
        rvSegmentTemplate* st,
        rvParticleTemplate* pt);
    void            AddToParticleCount(rvBSE* effect,
        int  count,
        int  loopCount,
        float duration);
    void            CalcTrailCounts(rvBSE* effect,
                                    const rvSegmentTemplate* st,
                                    const rvParticleTemplate* pt,
        float duration);
    void            Handle(rvBSE* effect,
                           float timeNow);
    void            Handle(rvBSE* effect,
                           const rvSegmentTemplate* st, float timeNow);
    void            PlayEffect(rvBSE* effect,
        const rvSegmentTemplate* st);
    void            RefreshParticles(rvBSE* effect,
        const rvSegmentTemplate* st);
    void            UpdateSimpleParticles(float timeNow);
    void            UpdateElectricityParticles(float timeNow);
    void            UpdateGenericParticles(rvBSE* effect,
        const rvSegmentTemplate* st,
        float timeNow);
    rvParticle* InitParticleArray(rvBSE* effect);
    void            RenderMotion(rvBSE* effect,
        const renderEffect_s* owner,
        rvRenderModelBSE* model,
        const rvParticleTemplate* pt,
        float timeNow);
    /* ultra-long decal routine kept separate for clarity */
    void            CreateDecal(rvBSE* effect, float timeNow);
    void InitLight(rvBSE *effect, const rvSegmentTemplate *st, float time);

    bool HandleLight(rvBSE *effect, const rvSegmentTemplate *st, float time);

public:
    /* --- data members --------------------------------------------------- */
    int mSegmentTemplateHandle;
    const rvDeclEffect* mEffectDecl;
    float mSegStartTime;
    float mSegEndTime;
    idVec2 mSecondsPerParticle;
    idVec2 mCount;
    int mParticleCount;
    int mLoopParticleCount;
    float mSoundVolume;
    float mFreqShift;
    int mFlags;
    float mLastTime;
    int mActiveCount;
    rvParticle* mFreeHead;
    rvParticle* mUsedHead;
    rvParticle* mParticles;

    idList<rvParticle *> mParticleList; //k??? using idList instead of rvParticle[]
};

