#pragma once

#include "../../idlib/precompiled.h"

#include "../../renderer/tr_local.h"
#include "../../renderer/Model_local.h"

#include "BSEInterface.h"

extern const float kEpsilon;
extern const float kHalf;

//karin: for BSE devel
//#define BSE_DEV 1

#if BSE_DEV
extern idCVar bse_log;
#define BSE_LOGFI(x, ...) do { if(bse_log.GetBool()) LOGFI(x, __VA_ARGS__); } while(0);
#else
#define BSE_LOGFI(...)
#endif

/*typedef */enum eBSEPerfCounter
{
    PERF_NUM_BSE,
    PERF_NUM_TRACES,
    PERF_NUM_PARTICLES,
    PERF_NUM_TEXELS,
    PERF_NUM_SEGMENTS,
    NUM_PERF_COUNTERS
};

// rvSegmentTemplate::mSegType //karin: move enum from class rvSegmentTemplate to globals
/*typedef */enum eBSESegment
{
    SEG_NONE = 0x0,
    SEG_EFFECT = 0x1,						// Spawns another effect inheriting data from owner
    SEG_EMITTER = 0x2,					// Spawns particles at a rate
    SEG_SPAWNER = 0x3,					// Spawns particles instantly
    SEG_TRAIL = 0x4,						// Leaves a trail of particles
    SEG_SOUND = 0x5,						// Plays a sound
    SEG_DECAL = 0x6,						// Leaves an idDecal
    SEG_LIGHT = 0x7,						// Displays a 3D light
    SEG_DELAY = 0x8,						// A control segment for looping
    SEG_DOUBLEVISION = 0x9, // SEG_DV // not in ETQW SDK
    SEG_SHAKE = 0xA, // 10						// Triggers a screen shake
    SEG_TUNNEL = 0xB, // 11						// Triggers the id tunnel vision effect
    SEG_COUNT = 0xC, // =12
};
#define SEG_DV SEG_DOUBLEVISION

// rvParticleTemplate::mTrailType
/*typedef */enum eBSETrail {
    TRAIL_NONE = 0,
    TRAIL_BURN = 1,
    TRAIL_MOTION = 2,
    TRAIL_PARTICLE = 3, // custom
    TRAIL_COUNT
};

// rvSegment::mFlag
enum {
    SFLAG_EXPIRED			= BITT< 0 >::VALUE,
    SFLAG_SOUNDPLAYING		= BITT< 1 >::VALUE,
    SFLAG_HASMOTIONTRAIL	= BITT< 2 >::VALUE,
};

class rvParticleParms
{
public:
    //— SpawnType enumeration mirrors the game ticker values ————————
    enum SpawnType
    {
        SPAWN_NONE = 0x00,
        SPAWN_CONSTANT_ONE = 0x05,
        SPAWN_AT_POINT = 0x09,
        SPAWN_LINEAR = 0x0D,
        SPAWN_BOX = 0x11,
        SPAWN_SURFACE_BOX = 0x15,
        SPAWN_SPHERE = 0x19,
        SPAWN_SURFACE_SPHERE = 0x1D,
        SPAWN_CYLINDER = 0x21,
        SPAWN_SURFACE_CYLINDER = 0x25,
        SPAWN_SPIRAL = 0x29,
        SPAWN_MODEL = 0x2D,

        SPAWN_CONSTANT_ONE_2D = 0x06, // SPAWN_CONSTANT_ONE+1,
        SPAWN_AT_POINT_2D = 0x0A, // SPAWN_AT_POINT+1,
        SPAWN_LINEAR_2D = 0x0E, // SPAWN_LINEAR+1,
        SPAWN_BOX_2D = 0x12, // SPAWN_BOX+1,
        SPAWN_SURFACE_BOX_2D = 0x16, // SPAWN_SURFACE_BOX+1,
        SPAWN_SPHERE_2D = 0x1A, // SPAWN_SPHERE+1,
        SPAWN_SURFACE_SPHERE_2D = 0x1E, // SPAWN_SURFACE_SPHERE+1,
        SPAWN_CYLINDER_2D = 0x22, // SPAWN_CYLINDER+1,
        SPAWN_SURFACE_CYLINDER_2D = 0x26, // SPAWN_SURFACE_CYLINDER+1,
        SPAWN_SPIRAL_2D = 0x2A, // SPAWN_SPIRAL+1,
        SPAWN_MODEL_2D = 0x2E, // SPAWN_MODEL+1,

        SPAWN_CONSTANT_ONE_3D = 0x07, // SPAWN_CONSTANT_ONE+2,
        SPAWN_AT_POINT_3D = 0x0B, // SPAWN_AT_POINT+2,
        SPAWN_LINEAR_3D = 0x0F, // SPAWN_LINEAR+2,
        SPAWN_BOX_3D = 0x13, // SPAWN_BOX+2,
        SPAWN_SURFACE_BOX_3D = 0x17, // SPAWN_SURFACE_BOX+2,
        SPAWN_SPHERE_3D = 0x1B, // SPAWN_SPHERE+2,
        SPAWN_SURFACE_SPHERE_3D = 0x1F, // SPAWN_SURFACE_SPHERE+2,
        SPAWN_CYLINDER_3D = 0x23, // SPAWN_CYLINDER+2,
        SPAWN_SURFACE_CYLINDER_3D = 0x27, // SPAWN_SURFACE_CYLINDER+2,
        SPAWN_SPIRAL_3D = 0x2B, // SPAWN_SPIRAL+2,
        SPAWN_MODEL_3D = 0x2F, // SPAWN_MODEL+2,
    };

    //— Public data kept identical to the Hex-Rays layout ————————
    uint32_t   mSpawnType;
    uint32_t   mFlags;           // bit-field (see code)
    idVec3     mMins;             // local bounding box
    idVec3     mMaxs;
    float      mRange;        // spiral radius, etc.
    void* mMisc;     // model / extra payload

    bool operator!=(const rvParticleParms& rhs) const
    {
        if (mSpawnType != rhs.mSpawnType) return true;
        if (mFlags != rhs.mFlags)     return true;
        if (mMisc != rhs.mMisc)      return true;      // pointer identity

        // idVec3 already has an epsilon-aware Compare()
        if (!mMins.Compare(rhs.mMins, kEpsilon))  return true;
        if (!mMaxs.Compare(rhs.mMaxs, kEpsilon))  return true;
        if (fabs(mRange - rhs.mRange) > kEpsilon) return true;

        return false; // everything matched
    }

    typedef void (*SpawnFunc)(float*               /*scratch*/,
        const rvParticleParms& /*template parms*/,
        idVec3*              /*in-out position*/,
        const idVec3*        /*reference*/);

    static SpawnFunc spawnFunctions[48];

    //— Small helpers ————————————————————————————————————————————————
    bool  Compare(const rvParticleParms& rhs) const;
    void  HandleRelativeParms(float* death, float* init, int count) const;
    void  GetMinsMaxs(idVec3& mins, idVec3& maxs) const;

    rvParticleParms()
    : mSpawnType(SPAWN_NONE),
      mFlags(0),
      mMins(idVec3(0.0f, 0.0f, 0.0f)),
      mMaxs(idVec3(0.0f, 0.0f, 0.0f)),
      mRange(1.0f),
      mMisc(NULL)
    {}

    //static const float kEpsilon;
};

namespace BSE
{
    const char * SegmentTypeName(int segType);
    const char * ParticleTypeName(int pType);
};

#include "bse_effect.h"
#include "BSE_Envelope.h"
#include "bse_parseparticle2.h"
#include "bse_effecttemplate.h"
#include "bse_segment.h"
#include "BSE_Particle.h"
#include "bse_light.h"
#include "bse_segmenttemplate.h"
#include "BSE_SpawnDomains.h"
#include "bse_electricity.h"

//──────────────────────────────────────────────────────────────────────────────
//  Constants & helpers
//──────────────────────────────────────────────────────────────────────────────
#if 0
namespace {
    const float   kDecayPerFrame = 0.1f;      // how fast rate-limit credits decay
    const size_t  kMaxCategories = 3;         // EC_IGNORE, EC_SMALL, EC_LARGE (example)
}
#endif

class rvBSEManagerLocal final : public rvBSEManager {
public:
    //──────────────────────────────
    // Lifetime
    //──────────────────────────────
    rvBSEManagerLocal() {}
    ~rvBSEManagerLocal() {}

    bool            Init();
    bool            Shutdown();

    //──────────────────────────────
    // Per-frame & level control
    //──────────────────────────────
    void            BeginLevelLoad()                      { /* nop */ }
    void            EndLevelLoad();              // clear rates
    void            StartFrame();              // reset perf HUD
    void            EndFrame();              // push perf HUD
    void            UpdateRateTimes();              // decay credits

    //──────────────────────────────
    // Effect API
    //──────────────────────────────
    bool            PlayEffect(rvRenderEffectLocal* def, float now);
    bool            ServiceEffect(rvRenderEffectLocal* def, float now);
    void            StopEffect(rvRenderEffectLocal* def);
    void            FreeEffect(rvRenderEffectLocal* def);
    float           EffectDuration(const rvRenderEffectLocal* def);

    bool            CheckDefForSound(const renderEffect_t* def);

    // view-kick helpers exposed to script
    void            SetDoubleVisionParms(float time, float scale);
    void            SetShakeParms(float time, float scale);
    void            SetTunnelParms(float time, float scale);

    // rate-limit helpers
    bool            Filtered(const char* name, effectCategory_t c = EC_MAX);
    bool            CanPlayRateLimited(effectCategory_t c);

    // trace models
    int             AddTraceModel(idTraceModel* m);
    idTraceModel* GetTraceModel(int idx);
    void            FreeTraceModel(int idx);

    // debug console commands
    static  void    BSE_Stats_f(const idCmdArgs& args);
    static  void    BSE_Log_f(const idCmdArgs& args);

public:
    //──────────────────────────────
    // Internal helpers / data
    //──────────────────────────────
    bool DebugHudActive() const { return com_debugHudActive; }

    idBlockAlloc<rvBSE, 256, 26>          effects_;             // effect pool
    static idMat3                          mModelToBSE;
    idList<idTraceModel*>          traceModels;         // loose heap-allocated models
    static		const char* mSegmentNames[SEG_COUNT];
	static float mEffectRates[EC_MAX];
	static float effectCosts[EC_MAX];
	static unsigned int mPerfCounters[NUM_PERF_COUNTERS];
    static idCVar *g_decals;
};

extern rvBSEManagerLocal bseLocal;
