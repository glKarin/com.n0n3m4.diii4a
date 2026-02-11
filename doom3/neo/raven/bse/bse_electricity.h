#pragma once

struct SElecWork
{
    float fraction;
    float step;
    idVec4 tint;
    float alpha;
    float size;
    idVec3 length;
    idVec3 forward;
    idVec3 viewPos;
    srfTriangles_s* tri;
    idVec3 coords[BSE_ELEC_MAX_BOLTS/* 200 */];
};

class rvElectricityParticle : public rvLineParticle
{
public:
    rvElectricityParticle()
        : mNumBolts(0),
          mNumForks(0),
          mSeed(0),
          mForkSizeMins(idVec3(0.0f, 0.0f, 0.0f)),
          mForkSizeMaxs(idVec3(0.0f, 0.0f, 0.0f)),
          mJitterSize(idVec3(0.0f, 0.0f, 0.0f)),
          mLastJitter(0.0f),
          mJitterRate(1.0f),
          mJitterTable(NULL),
          mEvalSizePtr(&rvParticle::EvaluateSize),
          mEvalLengthPtr(&rvParticle::EvaluateLength)
    {}
    ~rvElectricityParticle() {}

    /* -------------- primary interface -------------- */
    int   GetBoltCount(float length);
    virtual int 	Update(rvParticleTemplate* pt, float time);
    void  RenderLineSegment(const rvBSE* effect,
        SElecWork* work,
        const idVec3& start,
        float           startFrac);
    void  ApplyShape(const rvBSE* effect,
        SElecWork* work,
        const idVec3& start,
        const idVec3& end,
        int             recurse,
        float           startFrac,
        float           endFrac);
    void  RenderBranch(const rvBSE* effect,
        SElecWork* work,
        const idVec3& start,
        const idVec3& end);
    void  SetupElectricity(const rvParticleTemplate* pt);
    virtual bool  Render(const rvBSE* effect, const rvParticleTemplate* pt, const idMat3& view, srfTriangles_t* tri, float time, float override);

#if 0 //k??? TODO using parent
    /* -------------- evaluation hooks (normally supplied by the particle system) */
    virtual void EvaluateSize(float time, float* outSize) { *outSize = 1.0f; }
    virtual void EvaluateLength(float time, idVec3* outLen) { *outLen = idVec3(0, 0, 10); }
#endif

    rvElectricityParticle* GetArrayEntry(int) const;
    int         GetArrayIndex(rvParticle* p) const;
private:
    /* ------------ persistent state ------------ */
    int mNumBolts;
    int mNumForks;
    int mSeed;
    idVec3 mForkSizeMins;
    idVec3 mForkSizeMaxs;
    idVec3 mJitterSize;
    float mLastJitter;
    float mJitterRate;
    const idDeclTable* mJitterTable;

    /* raw member-function pointers kept for binary compatibility * the      */
    /* system sets these so de-compiled calls continue to work as designed.  */
    void (rvElectricityParticle::* mEvalSizePtr)(float, float*)/* = &rvElectricityParticle::EvaluateSize*/;
    void (rvElectricityParticle::* mEvalLengthPtr)(float, idVec3*)/* = &rvElectricityParticle::EvaluateLength*/;
};
