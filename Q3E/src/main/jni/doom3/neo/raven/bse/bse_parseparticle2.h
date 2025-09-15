// rvParticleTemplate::mFlags
enum {
    PTF_DIRECTIONAL = 1, // 0x1              // bit 0
    PTF_SEGMENT_LOCKED = 1 << 1, // 0x2,
    PTF_ATTENUATE = 1 << 5, // 0x20              // bit 5
    PTF_INVERT_ATTEN = 1 << 6, // 0x40              // bit 6

	PTF_PARSED = 1 << 8, // 0x100
	PTF_GENERATED_ORIGIN_NORMAL = 1 << 9, // 0x200
	PTF_FLIP_NORMAL = 1 << 10, // 0x400
	PTF_ALIGN_TO_NORMAL = 1 << 11, // 0x800
	PTF_RELATIVE_NORMAL = 1 << 12, // 0x1000
	PTF_INVERT_VELOCITY = 1 << 13, // 0x2000
	PTF_SPAWN_DIRECTION = 1 << 14, // 0x4000
	PTF_ADD = 1 << 15, // 0x8000
	PTF_GENERATED_LINE = 1 << 16, // 0x10000
	PTF_SHADOWS = 1 << 17, // 0x20000
	PTF_SPECULAR = 1 << 18, // 0x40000
	PTF_TRANSFORM_PARENT = 1 << 19, // 0x80000
	PTF_TILING = 1 << 20, // 0x100000
	PTF_PERSIST = 1 << 21, // 0x200000
};
#define PTF_LINKED_TRAIL 8 // 1 << 3
#define SEGMENT_NO_SHADOWS PTF_SHADOWS
#define SEGMENT_NO_SPECULAR PTF_SPECULAR
#define PF_SEGMENT_LOCKED PTF_SEGMENT_LOCKED
#define PF_DIRECTIONAL PTF_DIRECTIONAL
#define PF_ATTENUATE PTF_ATTENUATE
#define PF_INVERT_ATTEN PTF_INVERT_ATTEN
#define PF_TILING_LENGTH PTF_TILING

// rvParticleParms::mFlags
enum {
	PPF_SURFACE = 1, // 0x1
	PPF_USE_END_ORIGIN = 1 << 1, // 0x2
	PPF_CONE = 1 << 2, // 0x4
	PPF_RELATIVE = 1 << 3, // 0x8
	PPF_LINEAR_SPACING = 1 << 4, // 0x10
	PPF_ATTENUATE = 1 << 5, // 0x20
	PPF_INVERSE_ATTENUATE = 1 << 6, // 0x40
};

class rvParticleTemplate {
public:
    // ---------------------------------------------------------------------
    //  Lifetime helpers
    // ---------------------------------------------------------------------
    rvParticleTemplate() { Init(); }
    void        Init();                               // zero + sane defaults
    void        Finish();                             // post-parse fix-ups

    // ---------------------------------------------------------------------
    //  Query helpers
    // ---------------------------------------------------------------------
    bool        UsesEndOrigin()                 const;
    void        SetParameterCounts();                 // derive parm counts
    float       GetSpawnVolume(rvBSE* fx)      const;
    float       CostTrail(float baseCost)      const;
    idTraceModel* GetTraceModel()               const;
    int         GetTrailCount()                 const;

    float       GetFurthestDistance() const;

    void        EvaluateSimplePosition(
        idVec3* pos,
        float time,
        float lifeTime,
        const idVec3* initPos,
        const idVec3* velocity,
        const idVec3* acceleration,
        const idVec3* friction
    ) const;

    // ---------------------------------------------------------------------
    //  Parsing helpers (return true on success)
    // ---------------------------------------------------------------------
    bool        Parse(rvDeclEffect* effect, idLexer* src);
    bool        ParseSpawnDomains(rvDeclEffect* effect, idLexer* src);
    bool        ParseMotionDomains(rvDeclEffect* effect, idLexer* src);
    bool        ParseDeathDomains(rvDeclEffect* effect, idLexer* src);
    bool        ParseImpact(rvDeclEffect* effect, idLexer* src);
    bool        ParseTimeout(rvDeclEffect* effect, idLexer* src);
    bool        ParseBlendParms(rvDeclEffect* effect, idLexer* src);

    // ---------------------------------------------------------------------
    //  Comparisons / utilities
    // ---------------------------------------------------------------------
    bool        Compare(const rvParticleTemplate& rhs)   const;
    void        FixupParms(rvParticleParms& p);
    static bool GetVector(idLexer* src, int components, idVec3& out);
    static bool CheckCommonParms(idLexer* src, rvParticleParms& p);

    float       GetMaxParmValue(const rvParticleParms* spawn,const rvParticleParms* death,const rvEnvParms* envelope) const;

    const char *PTypeName() const
    {
        return BSE::ParticleTypeName(mType);
    }
public:
    // ── sub-parsers (internal) ────────────────────────────────────────────
    bool        ParseSpawnParms(rvDeclEffect*, idLexer*, rvParticleParms&, int vecCount);
    bool        ParseMotionParms(idLexer*, int vecCount, rvEnvParms&);
    int         GetMaxTrailCount() const;                      // helper

    // ── generic flags bitfield (Init zeros it) ────────────────────────────
    uint32_t      mFlags;                 // 0x00000001 == parsed, others per bits

    // ── basic type / material / model info ────────────────────────────────
    uint8_t       mType;                         // 0-9 primitive type enum
    idStr       mMaterialName;                 // parsed “material”
    const idMaterial* mMaterial;             // resolved pointer
    idStr       mModelName;                    // parsed “model”
    int         mTraceModelIndex;              // into trace-model pool

    // ── physics / timing & render counts ──────────────────────────────────
    idVec2      mGravity;                      // min, max
    idVec2      mSoundVolume;                  // optional
    idVec2      mFreqShift;                    // optional
    idVec2      mDuration;                     // life (min,max)
    float       mBounce;                       // elasticity 0-1
    float       mTiling;                       // UV tiling (>=.002)
    int         mVertexCount;                  // set in Finish()
    int         mIndexCount;                   // set in Finish()

    // ── trail rendering parameters ────────────────────────────────────────
    int         mTrailType;                    // 0-none,1-burn,2-motion,3-custom
    idStr       mTrailTypeName;                // custom name (type==3)
    idStr       mTrailMaterialName;            // override material
    const idMaterial* mTrailMaterial;
    idVec2      mTrailTime;                    // life of a trail segment
    idVec2      mTrailCount;                   // segments emitted / particle

    // ── fork & jitter extras ──────────────────────────────────────────────
    int         mNumForks;                     // 0-16
    idVec3      mForkSizeMins;
    idVec3      mForkSizeMaxs;
    idVec3      mJitterSize;                   // per-frame deviation
    float       mJitterRate;                   // Hz
    const idDeclTable* mJitterTable;          // lookup

    // ── centre of spawn bounds (pre-calc) ─────────────────────────────────
    idVec3      mCentre;

    // ── component counts derived from mType ───────────────────────────────
    int         mNumSizeParms;                 // 0-3
    int         mNumRotateParms;               // 0-3

    // ── SPAWN domains ─────────────────────────────────────────────────────
    rvParticleParms  mSpawnPosition;
    rvParticleParms  mSpawnDirection;
    rvParticleParms  mSpawnVelocity;
    rvParticleParms  mSpawnAcceleration;
    rvParticleParms  mSpawnFriction;
    rvParticleParms  mSpawnTint;
    rvParticleParms  mSpawnFade;
    rvParticleParms  mSpawnSize;
    rvParticleParms  mSpawnRotate;
    rvParticleParms  mSpawnAngle;
    rvParticleParms  mSpawnOffset;
    rvParticleParms  mSpawnLength;

    // ── PER-FRAME ENVELOPES ───────────────────────────────────────────────
    rvEnvParms       mTintEnvelope;
    rvEnvParms       mFadeEnvelope;
    rvEnvParms       mSizeEnvelope;
    rvEnvParms       mRotateEnvelope;
    rvEnvParms       mAngleEnvelope;
    rvEnvParms       mOffsetEnvelope;
    rvEnvParms       mLengthEnvelope;

    // ── DEATH (timeout) domains ───────────────────────────────────────────
    rvParticleParms  mDeathTint;
    rvParticleParms  mDeathFade;
    rvParticleParms  mDeathSize;
    rvParticleParms  mDeathRotate;
    rvParticleParms  mDeathAngle;
    rvParticleParms  mDeathOffset;
    rvParticleParms  mDeathLength;

    // ── impact / timeout effect hooks ─────────────────────────────────────
    int                 mNumImpactEffects;
    int                 mNumTimeoutEffects;
    const rvDeclEffect* mImpactEffects[4];
    const rvDeclEffect* mTimeoutEffects[4];

    // ── optional entityDef attachment (for spawned rigid-bodys etc.) ──────
    idStr       mEntityDefName;
};
