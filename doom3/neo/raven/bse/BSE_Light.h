#pragma once

class rvLightParticle : public rvParticle {
public:
    rvLightParticle()
            : rvParticle(),
              mLightDefHandle(-1)
    {
        memset(&mLight, 0, sizeof(mLight));
    }
    ~rvLightParticle() {}

    // ───── rvParticle interface ────────────────────────────────────────────
    virtual bool Destroy();   // kills render-light and frees handle
    virtual bool InitLight(rvBSE* effect,const rvSegmentTemplate* st,float time);
    virtual bool PresentLight(rvBSE* effect, const rvParticleTemplate* pt, float time, bool infinite);

    virtual void AttenuateSize(float atten, rvParticleParms *parms) {
        rvParticle::Attenuate(atten, *parms, mSizeEnv);
    }
    virtual		void			EvaluateLength(float time, idVec3* dest) {
        // mLengthEnv.Evaluate(time, dest->ToFloatPtr());
    }

    rvLightParticle* GetArrayEntry(int) const;
    int         GetArrayIndex(rvParticle* p) const;

    virtual void GetSpawnInfo(idVec4& tint, idVec3& size, idVec3& rotate);

private:
    // helpers
    void        ClampRadius();                          // ensures xyz ≥ 1
    void        SetOriginFromLocal(const idVec3& p);  // world-space origin
    void        SetAxis(const idMat3& m);    // orient light

    // ───── members ---------------------------------------------------------
//    rvEnvParms3 mSizeEnv;
    int mLightDefHandle;
    renderLight_s mLight;
};