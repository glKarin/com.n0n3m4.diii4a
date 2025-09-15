// bse_segmenttemplate.h
//

/* -------------------------------------------------  flags & enums  ------ */
enum rvSegTemplateFlags
{
	STF_ENABLED = 1, //karin: 0x1 from old Quake4BSE
    STF_LOCKED = 1 << 1, //karin: 0x2 k??? TODO Q4BSE is 1 << 0,   // *locked*   * stays fixed to owner
						 
	STF_HAS_PARTICLE = 1 << 2, // 0x4 // acitve
	STF_INGORE_DURATION = 1 << 4, // 0x10

    STF_CONSTANT = 1 << 5, // 0x20 //k??? TODO Q4BSE is 1 << 1,   // *constant* * used for linked segs
    STF_EMITTER_ATTEN = 1 << 6,   // 0x40 *attenuateEmitter*
    STF_EMITTER_INV_ATTEN = 1 << 7,   // *inverseAttenuateEmitter*
    STF_TEMPORARY = 1 << 8,   //karin: 0x100 from old Quake4BSE
    STF_HAS_SOUND = 1 << 14, //karin: //k??? TODO Q4BSE is 1 << 8,   // runtime * SoundShader assigned
    STF_IS_TRAIL = 1 << 9,   // runtime * seg spawns trail seg
    STF_IS_PARTICLE = 1 << 10,  // runtime * seg owns particles
    STF_IS_LIGHT = 1 << 11,  // runtime * seg owns light
    STF_DETAIL_CULL = 1 << 12,  // runtime * culled by detail thresh
    STF_MAX_DURATION = 1 << 13   // *channel*-only seg * use snd len
};

// rvSegmentTemplate::mSegType //karin: move enum from class rvSegmentTemplate to globals
enum rvSegTemplateType
{
    SEG_NONE = 0x0,
    SEG_EFFECT = 0x1,
    SEG_EMITTER = 0x2,
    SEG_SPAWNER = 0x3,
    SEG_TRAIL = 0x4,
    SEG_SOUND = 0x5,
    SEG_DECAL = 0x6,
    SEG_LIGHT = 0x7,
    SEG_DELAY = 0x8,
    SEG_DOUBLEVISION = 0x9, // SEG_DV
    SEG_SHAKE = 0xA, // 10
    SEG_TUNNEL = 0xB, // 11
    SEG_COUNT = 0xC, // =12
};
#define SEG_DV SEG_DOUBLEVISION

// rvParticleTemplate::mType //karin: change prefix to PTYPE_ from SEG, because DECAL, LIGHT, SOUND are ambiguous with same name, and rename enum name to rvParticleTemplateType
enum rvParticleTemplateType
{
    PTYPE_PARTICLE = 0,
    PTYPE_SPRITE = 1,
    PTYPE_LINE = 2,
    PTYPE_ORIENTED = 3,
    PTYPE_DECAL = 4,
    PTYPE_MODEL = 5,
    PTYPE_LIGHT = 6,
    PTYPE_ELECTRIC = 7,
    PTYPE_LINKED = 8,
    PTYPE_DEBRIS = 9,
    PTYPE_SOUND = 10,   // *channel*-only segment
    PTYPE_COUNT = 11,
    PTYPE_INVALID = 255
};

// rvParticleTemplate::mTrailType
enum {
	TRAIL_NONE = 0,
	TRAIL_BURN = 1,
	TRAIL_MOTION = 2,
	TRAIL_PARTICLE = 3, // custom
};

class rvSegmentTemplate
{
public:
    rvSegmentTemplate()
    : mDeclEffect(NULL),
      mFlags(0),
      mSegType(SEG_NONE), //k??? TODO Q4BSE is (SEG_INVALID),
      mLocalStartTime(idVec2(0.0f, 0.0f)),
      mLocalDuration(idVec2(0.0f, 0.0f)),
      mAttenuation(idVec2(0.0f, 0.0f)),
      mParticleCap(0.0f),
      mDetail(0.0f),
      mScale(1.0f),
      mCount(idVec2(1.0f, 1.0f)),
      mDensity(idVec2(0.0f, 0.0f)),
      mTrailSegmentIndex(-1),
      mNumEffects(0),
      mSoundShader(NULL),
      mSoundVolume(idVec2(0.0f, 0.0f)),
      mFreqShift(idVec2(1.0f, 1.0f)),
      mBSEEffect(NULL)
    {
        for(int i = 0; i < sizeof(mEffects) / sizeof(mEffects[0]); i++)
            mEffects[i] = NULL;
        Init(NULL);
    }

    /* construction helpers ------------------------------------------------ */
    void             Init(rvDeclEffect* decl);
    bool             Parse(rvDeclEffect* effect,
        int            segmentType,
        idLexer* lexer);
    bool             Finish(rvDeclEffect* effect);
    void             EvaluateTrailSegment(rvDeclEffect* effect);

    /* particle helpers ---------------------------------------------------- */
    void             CreateParticleTemplate(rvDeclEffect* effect,
        idLexer* lexer,
        int           particleType);

    /* run-time queries ----------------------------------------------------- */
    int              GetTexelCount() const;
    bool             GetSmoker() const;
    bool             Compare(const rvSegmentTemplate& rhs) const;
    bool             GetSoundLooping() const;
    bool             DetailCull() const;

    /* lifetime helpers ---------------------------------------------------- */
    void             SetMinDuration(rvDeclEffect* effect);
    void             SetMaxDuration(rvDeclEffect* effect);

    /* shorthand access ---------------------------------------------------- */
    ID_INLINE const  idStr& GetName()      const { return mSegmentName; }
    ID_INLINE        rvSegTemplateType  GetType()   const { return static_cast<rvSegTemplateType>(mSegType); }

    float               CalculateBounds() const;
    float GetSoundVolume() const;
    float GetFreqShift() const;

    const char *    SegTypeName() const
    {
        return BSE::SegmentTypeName(mSegType);
    }
public:
    /* data * identical order to the dump so pointer arithmetic stays valid */
    rvDeclEffect* mDeclEffect;

    uint32_t               mFlags;  // see enum above
    uint8_t                mSegType;   // rvSegTemplateType

    idVec2                 mLocalStartTime; // min,max
    idVec2                 mLocalDuration;
    idVec2                 mAttenuation;

    float                  mParticleCap;
    float                  mDetail;
    float                  mScale;

    idVec2                 mCount;
    idVec2                 mDensity;

    int32_t                mTrailSegmentIndex;

    int32_t                mNumEffects;
    const rvDeclEffect* mEffects[4];

    const idSoundShader* mSoundShader;
    idVec2                 mSoundVolume;
    idVec2                 mFreqShift;

    rvParticleTemplate     mParticleTemplate;

    rvBSE*                  mBSEEffect;

    idStr                  mSegmentName;
};
