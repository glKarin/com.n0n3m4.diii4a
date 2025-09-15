#pragma once

class rvSegment;

//‑‑‑ Base particle --------------------------------------------------------------------
class rvParticle {
public:
    /* life‑cycle -------------------------------------------------------------*/
    rvParticle()
    : mNext(NULL),
      mMotionStartTime(0.0f),
      mLastTrailTime(0.0f),
      mFlags(0),
      mStartTime(0.0f),
      mEndTime(0.0f),
      mTrailTime(0.0f),
      mTrailCount(0),
      mFraction(0.0f),
      mTextureScale(0.0f),
      mInitEffectPos(idVec3(0.0f, 0.0f, 0.0f)),
      mInitAxis(mat3_identity),
      mInitPos(idVec3(0.0f, 0.0f, 0.0f)),
      mNormal(idVec3(0.0f, 0.0f, 0.0f)),
      mVelocity(idVec3(0.0f, 0.0f, 0.0f)),
      mAcceleration(idVec3(0.0f, 0.0f, 0.0f)),
      mFriction(idVec3(0.0f, 0.0f, 0.0f))
    {}
    virtual ~rvParticle() {}
    virtual void FinishSpawn(rvBSE* effect, rvSegment* segment,
        float birthTime, float fraction,
        const idVec3& initOffset,
        const idMat3& initAxis);

    virtual		void			GetSpawnInfo(idVec4& tint, idVec3& size, idVec3& rotate) { assert(0); }

    /* evaluation & simulation -----------------------------------------------*/
    void            EvaluateVelocity(const rvBSE* effect, idVec3& out, float time) const;
    void            EvaluatePosition(const rvBSE* effect, idVec3& out, float time);
    bool            RunPhysics(rvBSE* effect, const rvSegmentTemplate* st, float time);
    void            Bounce(rvBSE* effect, const rvParticleTemplate* pt,
        idVec3 endPos, idVec3 normal, float time);

    /* attenuation helpers ----------------------------------------------------*/
    void Attenuate(float a, const rvParticleParms& p, rvEnvParms1& env);
    void Attenuate(float a, const rvParticleParms& p, rvEnvParms2& env);
    void Attenuate(float a, const rvParticleParms& p, rvEnvParms3& env);

    /* spawn‑time utilities ---------------------------------------------------*/
    void HandleEndOrigin(rvBSE* effect, const rvParticleTemplate* pt,
        idVec3* normal, const idVec3* centre);
    virtual void HandleOrientation(const rvAngles* angles);    // stub (not shown)

    /* trail / timeout --------------------------------------------------------*/
    void EmitSmokeParticles(rvBSE* effect, rvSegment* child, float time);
    void CheckTimeoutEffect(rvBSE* effect, const rvSegmentTemplate* st, float time);

    /* math helpers -----------------------------------------------------------*/
    void CalcImpactPoint(idVec3& impact,
        const idVec3& origin,
        const idVec3& motion,
        const idBounds& bounds,
        const idVec3& planeNormal);

    virtual void SetOriginUsingEndOrigin(rvBSE* effect, const rvParticleTemplate* pt, idVec3* normal, const idVec3* centre);

    virtual void RenderMotion(rvBSE* effect, srfTriangles_s* tri, const renderEffect_s* owner, float time);

    /* virtual array helpers (overridden by derived fixed‑size pools) ---------*/
    virtual rvParticle* GetArrayEntry(int index) const { return index < 0 ? NULL : const_cast<rvParticle*>(this) + index; }
    virtual int         GetArrayIndex(rvParticle* p) const { return p ? intptr_t (reinterpret_cast<uint8_t*>(p) - reinterpret_cast<uint8_t const*>(this)) / sizeof(rvParticle) : -1; }
    virtual void RenderQuadTrail(const rvBSE* effect, srfTriangles_s* tri, const idVec3& offset, float fraction, const idVec4& colour, const idVec3& pos, bool firstSegment);
    virtual int  HandleTint(const rvBSE* effect, const idVec4& colour, float alpha) const;
    virtual bool GetEvaluationTime(float time, float& evalTime, bool infinite) const;

    /* accessor shims used in original code -----------------------------------*/
    virtual float* GetInitSize() {
        return mSizeEnv.mStart.ToFloatPtr();
    }
    virtual float* GetDestSize() {
        return mSizeEnv.mEnd.ToFloatPtr();
    }
    virtual float* GetInitRotation() {
        return mRotationEnv.mStart.ToFloatPtr();
    }
    virtual float* GetDestRotation() {
        return mRotationEnv.mEnd.ToFloatPtr();
    }

    /* length / direction helpers (rvLineParticle etc.) -----------------------*/
    virtual float* GetInitLength() {
        return mLengthEnv.mStart.ToFloatPtr();
    }
    virtual float* GetDestLength() {
        return mLengthEnv.mEnd.ToFloatPtr();
    }
    virtual void   AttenuateLength(float atten, const rvParticleParms*parms) {
        Attenuate(atten, *parms, mLengthEnv);
    }

    /* transform helpers injected by compiler‑generated thunks ----------------*/
    virtual void ScaleAngle(float s);
    virtual void ScaleRotation(float s);
    virtual void ScaleLength(float s) {
        mLengthEnv.Scale(s);
    }
    virtual void TransformLength(const idVec3 &l) {
        mLengthEnv.Transform(l);
    }

    virtual		bool			InitLight(rvBSE* effect, const rvSegmentTemplate* st, float time) { return(false); }
    virtual		bool			PresentLight(rvBSE* effect, const rvParticleTemplate* pt, float time, bool infinite) { return(false); }
    virtual		bool			Destroy() { return(false); }
    virtual		void			SetModel(const char* model) {}
    virtual		void			SetupElectricity(const rvParticleTemplate* pt) {}
    virtual		void			Refresh(const rvBSE* effect, const rvSegmentTemplate* st, const rvParticleTemplate* pt) {}

    virtual		void			InitSizeEnv(const rvEnvParms& env, float duration) {
        mSizeEnv.Init(env, duration);
    } //k??? TODO { assert(0); } implement move to rvParticle
    virtual		void			InitRotationEnv(const rvEnvParms& env, float duration) {
        mRotationEnv.Init(env, duration);
    } //k??? TODO { assert(0); } implement move to rvParticle
    virtual		void			InitLengthEnv(const rvEnvParms& env, float duration) {
        mLengthEnv.Init(env, duration);
    } //k??? TODO { assert(0); } implement move to rvParticle

    virtual		bool			Render(const rvBSE* effect, const rvParticleTemplate* pt, const idMat3& view, srfTriangles_t* tri, float time, float override = 1.0f) { return(false); }
    virtual		int				Update(rvParticleTemplate* pt, float time) { return(1); }

    virtual		void			EvaluateSize(float time, float* dest) {
        //k??? TODO assert(0); implement move to rvParticle
        mSizeEnv.Evaluate(time, dest);
    }
    virtual		void			EvaluateRotation(float time, float* dest) {
        //k??? TODO assert(0); implement move to rvParticle
        mRotationEnv.Evaluate(time, dest);
    }
    virtual		void			EvaluateLength(float time, idVec3* dest) {
        //k??? TODO assert(0); implement move to rvParticle
        mLengthEnv.Evaluate(time, dest->ToFloatPtr());
    }

    void SetLengthUsingEndOrigin(
        const rvBSE* effect,
        const rvParticleParms* parms,
        float* length);

    void AttenuateFade(float atten, const rvParticleParms *parms);
    virtual void AttenuateSize(float atten, const rvParticleParms *parms);
    virtual void RenderBurnTrail(rvBSE *effect, const idMat3 *view, srfTriangles_s *tri, float time) {
        DoRenderBurnTrail(effect, view, tri, time);
    }
    void DoRenderBurnTrail(rvBSE *effect, const idMat3 *view, srfTriangles_s *tri, float time);

public:
    // --- data copied verbatim from dump (trim/rename as needed) -------------
    rvParticle* mNext;
    float mMotionStartTime;
    float mLastTrailTime;
    unsigned int mFlags;
    float mStartTime;
    float mEndTime;
    float mTrailTime;
    int mTrailCount;
    float mFraction;
    float mTextureScale;
    idVec3 mInitEffectPos;
    idMat3 mInitAxis;
    idVec3 mInitPos;
    idVec3 mNormal;
    idVec3 mVelocity;
    idVec3 mAcceleration;
    idVec3 mFriction;
    rvEnvParms3 mTintEnv;
    rvEnvParms1 mFadeEnv;
    rvEnvParms3 mAngleEnv;
    rvEnvParms3 mOffsetEnv;

    rvEnvParms3 mRotationEnv;
    rvEnvParms3 mSizeEnv;
    rvEnvParms3 mLengthEnv;
};

//‑‑‑ Specialized particles ------------------------------------------------------------
class rvLineParticle : public rvParticle {
public:
    ~rvLineParticle() {}
    /* overrides */
    void        HandleTiling(const rvParticleTemplate* pt);
    rvLineParticle* GetArrayEntry(int) const;
    int         GetArrayIndex(rvParticle* p) const;
    void        FinishSpawn(rvBSE*, rvSegment*, float, float, const idVec3&, const idMat3&);
    void        Refresh(const rvBSE*, const rvSegmentTemplate*, const rvParticleTemplate*);
    void        GetSpawnInfo(idVec4& tint, idVec3& size, idVec3& rotate);

    virtual bool Render(const rvBSE* effect, const rvParticleTemplate* pt, const idMat3& view, srfTriangles_t* tri, float time, float override);

#if 0
    virtual		void			InitLengthEnv(const rvEnvParms& env, float duration) {
        mLengthEnv.Init(env, duration);
    }
    /* length helpers */
    virtual void        AttenuateLength(float atten, const rvParticleParms* parms) {
        Attenuate(atten, *parms, mLengthEnv);
    }
    virtual void EvaluateLength(float time, idVec3* dest) {
        mLengthEnv.Evaluate(time, dest->ToFloatPtr());
    }
    virtual float* GetInitLength() {
        return mLengthEnv.mStart.ToFloatPtr();
    }
    virtual float* GetDestLength() {
        return mLengthEnv.mEnd.ToFloatPtr();
    }
    virtual void TransformLength(const idVec3 &l) {
        mLengthEnv.Transform(l);
    }
    virtual void ScaleLength(float s) {
        mLengthEnv.Scale(s);
    }
    virtual void RenderBurnTrail(rvBSE *effect, const idMat3 *view, srfTriangles_s *tri, float time) {
        rvParticle::DoRenderBurnTrail(effect, view, tri, time);
    }

#if 0
    virtual void EvaluateSize(float time, float* dest) {
        mSizeEnv.Evaluate(time, dest);
    }
    virtual float* GetInitSize() {
        return &mSizeEnv.mStart;
    }
    virtual float* GetDestSize() {
        return &mSizeEnv.mEnd;
    }

    rvEnvParms1 mSizeEnv;
#endif
    rvEnvParms3 mLengthEnv;
#endif
};

class rvLinkedParticle : public rvParticle {
public:
    void        HandleTiling(const rvParticleTemplate* pt);
    rvLinkedParticle* GetArrayEntry(int) const;
    int         GetArrayIndex(rvParticle* p) const;
    void        GetSpawnInfo(idVec4& tint, idVec3& size, idVec3& rotate) {}
    void        FinishSpawn(rvBSE*, rvSegment*, float, float, const idVec3&, const idMat3&);
    virtual bool Render(const rvBSE* effect, const rvParticleTemplate* pt, const idMat3& view, srfTriangles_t* tri, float time, float override);

#if 0
    virtual		void			InitSizeEnv(const rvEnvParms& env, float duration) {
        mSizeEnv.Init(env, duration);
    }
    virtual void AttenuateSize(float atten, const rvParticleParms *parms) {
        Attenuate(atten, *parms, mSizeEnv);
    }

    rvEnvParms1 mSizeEnv;
#endif
};

class rvDecalParticle : public rvParticle {
public:
    rvDecalParticle* GetArrayEntry(int) const;
    int         GetArrayIndex(rvParticle* p) const;
    virtual bool Render(const rvBSE* effect, const rvParticleTemplate* pt, const idMat3& view, srfTriangles_t* tri, float time, float override) { return false; }
};

class rvModelParticle : public rvParticle {
public:
	rvModelParticle()
		: mModel(NULL)
	{}

    virtual bool Render(const rvBSE* effect, const rvParticleTemplate* pt, const idMat3& view, srfTriangles_t* tri, float time, float override);

    void GetSpawnInfo(idVec4 &tint, idVec3 &size, idVec3 &rotate);

    virtual		void			SetModel(const char* model) {
        mModel = renderModelManager->FindModel(model);
    }

#if 0
    virtual void EvaluateSize(float time, float* dest) {
        //k??? TODO Q4D is rvEnvParms3::Evaluate(&this->mRotationEnv, time, dest);
        mSizeEnv.Evaluate(time, dest);
    }
    virtual float* GetInitSize() {
        // k??? TODO Q4D is return &this->mRotationEnv.mStart;
        return mSizeEnv.mStart.ToFloatPtr();
    }

    virtual void EvaluateRotation(float time, float *dest) {
        mRotationEnv.Evaluate(time, dest);
    }
    virtual		void			InitRotationEnv(const rvEnvParms& env, float duration) {
        mRotationEnv.Init(env, duration);
    }
    virtual float* GetInitRotation() {
        return mRotationEnv.mStart.ToFloatPtr();
    }
    virtual float* GetDestRotation() {
        return mRotationEnv.mEnd.ToFloatPtr();
    }

    rvEnvParms3 mRotationEnv;
    rvEnvParms3 mSizeEnv;
#endif
    const idRenderModel *mModel;
};

class rvOrientedParticle : public rvModelParticle {
public:
    rvOrientedParticle* GetArrayEntry(int) const;
    int         GetArrayIndex(rvParticle* p) const;
    void        GetSpawnInfo(idVec4& tint, idVec3& size, idVec3& rotate);
    virtual bool Render(const rvBSE* effect, const rvParticleTemplate* pt, const idMat3& view, srfTriangles_t* tri, float time, float override);

#if 0
    virtual void EvaluateRotation(float time, float *dest) {
        mRotationEnv.Evaluate(time, dest);
    }
    virtual		void			InitRotationEnv(const rvEnvParms& env, float duration) {
        mRotationEnv.Init(env, duration);
    }
    virtual void HandleOrientation(const rvAngles* angles);
    virtual float* GetInitRotation() {
        return mRotationEnv.mStart.ToFloatPtr();
    }
    virtual float* GetDestRotation() {
        return mRotationEnv.mEnd.ToFloatPtr();
    }
    virtual void ScaleRotation(float constant) {
        mRotationEnv.mStart.x = constant * mRotationEnv.mStart.x;
        mRotationEnv.mStart.y = constant * mRotationEnv.mStart.y;
        mRotationEnv.mStart.z = constant * mRotationEnv.mStart.z;
        mRotationEnv.mEnd.x = constant * mRotationEnv.mEnd.x;
        mRotationEnv.mEnd.y = constant * mRotationEnv.mEnd.y;
        mRotationEnv.mEnd.z = constant * mRotationEnv.mEnd.z;
    }

    rvEnvParms3 mRotationEnv;
#endif
};

class rvSpriteParticle : public rvParticle {
public:
    void        GetSpawnInfo(idVec4& tint, idVec3& size, idVec3& rotate);

    virtual bool Render(const rvBSE* effect, const rvParticleTemplate* pt, const idMat3& view, srfTriangles_t* tri, float time, float override);

#if 0
    virtual void EvaluateSize(float time, float* dest) {
        mSizeEnv.Evaluate(time, dest);
    }
    void EvaluateRotation(float time, float *dest) {
        mRotationEnv.Evaluate(time, dest);
    }
    virtual		void			InitSizeEnv(const rvEnvParms& env, float duration) {
        mSizeEnv.Init(env, duration);
    }
    virtual		void			InitRotationEnv(const rvEnvParms& env, float duration) {
        mRotationEnv.Init(env, duration);
    }
    virtual float* GetInitRotation() {
        return &mRotationEnv.mStart;
    }
    virtual float* GetDestRotation() {
        return &mRotationEnv.mEnd;
    }
    virtual void ScaleRotation(float constant) {
        mRotationEnv.mStart = constant * mRotationEnv.mStart;
        mRotationEnv.mEnd = constant * mRotationEnv.mEnd;
    }
    virtual void AttenuateSize(float atten, const rvParticleParms *parms) {
        Attenuate(atten, *parms, mSizeEnv);
    }
    virtual float* GetDestSize() {
        return this->mSizeEnv.mEnd.ToFloatPtr();
    }

    rvEnvParms1 mRotationEnv;
    rvEnvParms2 mSizeEnv;
#endif
};

class rvDebrisParticle : public rvParticle {
public:
    rvDebrisParticle* GetArrayEntry(int) const;
    int         GetArrayIndex(rvParticle* p) const;
    void        FinishSpawn(rvBSE*, rvSegment*, float, float, const idVec3&, const idMat3&);
    virtual bool Render(const rvBSE* effect, const rvParticleTemplate* pt, const idMat3& view, srfTriangles_t* tri, float time, float override) { return false; }

#if 0
    virtual		void			InitRotationEnv(const rvEnvParms& env, float duration) {
        mRotationEnv.Init(env, duration);
    }
    virtual float* GetDestRotation() {
        return mRotationEnv.mEnd.ToFloatPtr();
    }
    virtual void ScaleRotation(float constant) {
        mRotationEnv.mStart.x = constant * mRotationEnv.mStart.x;
        mRotationEnv.mStart.y = constant * mRotationEnv.mStart.y;
        mRotationEnv.mStart.z = constant * mRotationEnv.mStart.z;
        mRotationEnv.mEnd.x = constant * mRotationEnv.mEnd.x;
        mRotationEnv.mEnd.y = constant * mRotationEnv.mEnd.y;
        mRotationEnv.mEnd.z = constant * mRotationEnv.mEnd.z;
    }

    rvEnvParms3 mRotationEnv;
#endif
};

