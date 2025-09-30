// bse_segmenttemplate.h
//

/* -------------------------------------------------  flags & enums  ------ */
// rvSegmentTemplate::mFlags
enum rvSegTemplateFlags
{
	STF_ENABLED = 1, //karin: 0x1 from old Quake4BSE
    STF_LOCKED = 1 << 1, //karin: 0x2 k??? TODO Q4BSE is 1 << 0,   // *locked*   * stays fixed to owner

	STF_HASPARTICLES = 1 << 2, // 0x4 // acitve
	STF_IGNORE_DURATION = 1 << 4, // 0x10

    STF_INFINITE_DURATION = 1 << 5, // 0x20 //k??? TODO Q4BSE is 1 << 1,   // *constant* * used for linked segs
    STF_ATTENUATE_EMITTER = 1 << 6,   // 0x40 *attenuateEmitter*
    STF_INVERSE_ATTENUATE = 1 << 7,   // *inverseAttenuateEmitter*
    STF_TEMPORARY = 1 << 8,   //karin: 0x100 from old Quake4BSE
//    STF_IS_TRAIL = 1 << 9,   // runtime * seg spawns trail seg
//    STF_IS_PARTICLE = 1 << 10,  // runtime * seg owns particles
//    STF_IS_LIGHT = 1 << 11,  // runtime * seg owns light
//    STF_DETAIL_CULL = 1 << 12,  // runtime * culled by detail thresh
    STF_COMPLEX = 1 << 13,   // MAX_DURATION *channel*-only seg * use snd len
    STF_CALCULATE_DURATION = 1 << 14, //karin: soundShader HAS_SOUND //k??? TODO Q4BSE is 1 << 8,   // runtime * SoundShader assigned
};
enum { // ETQW SDK
    STFLAG_ENABLED				= BITT< 0 >::VALUE,
    STFLAG_LOCKED				= BITT< 1 >::VALUE,
    STFLAG_HASPARTICLES			= BITT< 2 >::VALUE,
    STFLAG_HASPHYSICS			= BITT< 3 >::VALUE,
    STFLAG_IGNORE_DURATION		= BITT< 4 >::VALUE,
    STFLAG_INFINITE_DURATION	= BITT< 5 >::VALUE,
    STFLAG_ATTENUATE_EMITTER	= BITT< 6 >::VALUE,
    STFLAG_INVERSE_ATTENUATE	= BITT< 7 >::VALUE,
    STFLAG_TEMPORARY			= BITT< 8 >::VALUE,
    STFLAG_USEMATCOLOR			= BITT< 9 >::VALUE,
    STFLAG_DEPTH_SORT			= BITT< 10 >::VALUE,
    STFLAG_INVERSE_DRAWORDER	= BITT< 11 >::VALUE,
    STFLAG_ORIENTATE_IDENTITY	= BITT< 12 >::VALUE,
    STFLAG_COMPLEX				= BITT< 13 >::VALUE,
    STFLAG_CALCULATE_DURATION	= BITT< 14 >::VALUE,
};

// rvParticleTemplate::mType //karin: change prefix to PTYPE_ from SEG, because DECAL, LIGHT, SOUND are ambiguous with same name, and rename enum name to rvParticleTemplateType
enum rvParticleTemplateType
{
    PTYPE_NONE = 0,									// A non sprite - for sound and vision segments
    PTYPE_SPRITE = 1,									// Simple 2D alpha blended quad
    PTYPE_LINE = 2,										// 2D alpha blended line
    PTYPE_ORIENTED = 3,									// 2D particle oriented in 3D - alpha blended
    PTYPE_DECAL = 4,									// Hook into id's decal system
    PTYPE_MODEL = 5,									// Model - must only have 1 surface
    PTYPE_LIGHT = 6,									// Dynamic light - very expensive
    PTYPE_ELECTRIC = 7,								// A bolt of electricity
    PTYPE_LINKED = 8,									// A series of linked lines
    // PTYPE_ORIENTEDLINKED, // only in ETQW SDK
    PTYPE_DEBRIS = 9,									// A client side moveable entity spawned in the game
    PTYPE_SOUND = 10,   // *channel*-only segment // not in ETQW SDK
    PTYPE_COUNT = 11,
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
    ID_INLINE        eBSESegment  GetType()   const { return static_cast<eBSESegment>(mSegType); }

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
