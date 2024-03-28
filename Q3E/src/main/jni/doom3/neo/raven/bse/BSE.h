// BSE.h
//

#pragma once

#ifndef _RAVEN_BSE_H
#define _RAVEN_BSE_H

struct viewDef_s;

// BSE - Basic System for Effects

// Notes:
// All times are floats of seconds
// All tints are floats of 0.0 to 1.0
// All effects are presumed to be in the "base/effects" folder and have the extension of BSE_EFFECT_EXTENSION
// All angles are in fractions of a circle - 1 means 1 full circle
// All effect files are case insensitive
// Will not have different shaders to be randomly chosen - need to keep the tesses to a minimum (except possibly for decals)

/*
 * //karin: OpenBSE: BSE by DOOM3's FX
 * Convert Quake4's BSE to DOOM3's FX/Particle
 *
 * Quake4's fx other decl -> DOOM3's idDeclFX(.fx)
 *   spawner/emitter/trail segment -> idDeclFX::PARTICLE: DOOM3's idRenderModelPrt(.prt)
 *   other segments -> DOOM3's idDeclFX::SOUND/SHAKE/LIGHT/DECAL
 * 
 * class relations(Quake4 -> DOOM3):
 *   rvDeclEffect == idDeclFX: BSE_Decl.cpp, BSE_Parser.cpp
 *   rvDeclParticle == idDeclParticle: BSE_Particle.cpp
 *   rvBSE == idEntityFx: BSE_Fx.cpp
 *   rvRenderModelBse == idRenderModelPrt: Model_bse.cpp
 *
 * Quake4 game library::RunFrame
 * 1. query BSE::BSE_Manager the rvDeclEffect is filtered/has sound/can play rated.
 * 2. call Renderer::idRenderWorld::AddEffectDef/UpdateEffectDef to add/update effect render entity.
 * 3. finally call Renderer::idRenderWorld::StopEffectDef/FreeEffectDef to stop/destroy effect render entity.
 * */

// Defined classes
class rvDeclEffect;
class rvBSEManagerLocal;

// Referenced classes

const float WORLD_SIZE = (128.0f * 1024.0f);
const float BSE_LARGEST = (512.0f);
const float BSE_TESS_COST = (20.0f);				// The expense of a new tess
const float BSE_PHYSICS_COST = (80.0f);				// The expense of 1 particle having physics

const float BSE_PARTICLE_TEXCOORDSCALE = (0.01f);



const unsigned int MEMORY_BLOCK_SIZE = (0x100000);
const unsigned int BSE_ELEC_MAX_BOLTS = (200);

/*typedef*/ enum eBSEPerfCounter
{
	PERF_NUM_BSE,
	PERF_NUM_TRACES,
	PERF_NUM_PARTICLES,
	PERF_NUM_TEXELS,
	PERF_NUM_SEGMENTS,
	NUM_PERF_COUNTERS
};

/*typedef*/ enum eBSESegment
{
	SEG_NONE = 0,
	SEG_EFFECT,						// Spawns another effect inheriting data from owner
	SEG_EMITTER,					// Spawns particles at a rate
	SEG_SPAWNER,					// Spawns particles instantly
	SEG_TRAIL,						// Leaves a trail of particles
	SEG_SOUND,						// Plays a sound
	SEG_DECAL,						// Leaves an idDecal
	SEG_LIGHT,						// Displays a 3D light
	SEG_DELAY,						// A control segment for looping
	SEG_SHAKE,						// Triggers a screen shake
	SEG_TUNNEL,						// Triggers the id tunnel vision effect
	SEG_COUNT
};
enum eBSEParticleType 
{
	PTYPE_NONE = 0,									// A non sprite - for sound and vision segments
	PTYPE_SPRITE,									// Simple 2D alpha blended quad
	PTYPE_LINE,										// 2D alpha blended line
	PTYPE_ORIENTED,									// 2D particle oriented in 3D - alpha blended
	PTYPE_DECAL,									// Hook into id's decal system
	PTYPE_MODEL,									// Model - must only have 1 surface
	PTYPE_LIGHT,									// Dynamic light - very expensive
	PTYPE_ELECTRICITY,								// A bolt of electricity
	PTYPE_LINKED,									// A series of linked lines
	PTYPE_ORIENTEDLINKED,
	PTYPE_DEBRIS,									// A client side moveable entity spawned in the game
	PTYPE_COUNT
};

/*typedef*/ enum eBSETrail
{
	TRAIL_NONE = 0,
	TRAIL_BURN,
	TRAIL_MOTION,
	TRAIL_PARTICLE,
	TRAIL_COUNT
};

enum {
	SFLAG_EXPIRED = BITT< 0 >::VALUE,
	SFLAG_SOUNDPLAYING = BITT< 1 >::VALUE,
	SFLAG_HASMOTIONTRAIL = BITT< 2 >::VALUE,
};

// ==============================================
// Effect class
// ==============================================

enum {
	STFLAG_ENABLED = BITT< 0 >::VALUE,
	STFLAG_LOCKED = BITT< 1 >::VALUE,
	STFLAG_HASPARTICLES = BITT< 2 >::VALUE,
	STFLAG_HASPHYSICS = BITT< 3 >::VALUE,
	STFLAG_IGNORE_DURATION = BITT< 4 >::VALUE,
	STFLAG_INFINITE_DURATION = BITT< 5 >::VALUE,
	STFLAG_ATTENUATE_EMITTER = BITT< 6 >::VALUE,
	STFLAG_INVERSE_ATTENUATE = BITT< 7 >::VALUE,
	STFLAG_TEMPORARY = BITT< 8 >::VALUE,
	STFLAG_USEMATCOLOR = BITT< 9 >::VALUE,
	STFLAG_DEPTH_SORT = BITT< 10 >::VALUE,
	STFLAG_INVERSE_DRAWORDER = BITT< 11 >::VALUE,
	STFLAG_ORIENTATE_IDENTITY = BITT< 12 >::VALUE,
	STFLAG_COMPLEX = BITT< 13 >::VALUE,
	STFLAG_CALCULATE_DURATION = BITT< 14 >::VALUE,
};

// ==============================================
// ==============================================

enum {
	ETFLAG_HAS_SOUND = BITT< 0 >::VALUE,
	ETFLAG_USES_ENDORIGIN = BITT< 1 >::VALUE,
	ETFLAG_ATTENUATES = BITT< 2 >::VALUE,
	ETFLAG_EDITOR_MODIFIED = BITT< 3 >::VALUE,
	ETFLAG_USES_MATERIAL_COLOR = BITT< 4 >::VALUE,
	ETFLAG_ORIENTATE_IDENTITY = BITT< 5 >::VALUE,
	ETFLAG_USES_AMBIENT_CUBEMAP = BITT< 6 >::VALUE,
	ETFLAG_HAS_PHYSICS = BITT< 7 >::VALUE,
};

#ifdef _RAVEN_FX
extern idCVar bse_render;
extern idCVar bse_singleEffect;
extern idCVar bse_debug;
#ifdef _K_DEV // my debug macros
#if !defined(_MSC_VER)
#define BSE_VERBOSE(fmt, args...) { if(bse_debug.GetInteger() >= 1) { common->Printf(fmt, ##args); } }
#define BSE_DEBUG(fmt, args...) { if(bse_debug.GetInteger() >= 2) { common->Printf(fmt, ##args); } }
#define BSE_INFO(fmt, args...) { if(bse_debug.GetInteger() >= 3) { common->Printf(fmt, ##args); } }
#define BSE_WARNING(fmt, args...) { if(bse_debug.GetInteger() >= 4) { common->Printf(fmt, ##args); } }
#define BSE_ERROR(fmt, args...) { if(bse_debug.GetInteger() >= 5) { common->Printf(fmt, ##args); } }
#else
#define BSE_VERBOSE(fmt, ...) { if(bse_debug.GetInteger() >= 1) { common->Printf(fmt, __VA_ARGS__); } }
#define BSE_DEBUG(fmt, ...) { if(bse_debug.GetInteger() >= 2) { common->Printf(fmt, __VA_ARGS__); } }
#define BSE_INFO(fmt, ...) { if(bse_debug.GetInteger() >= 3) { common->Printf(fmt, __VA_ARGS__); } }
#define BSE_WARNING(fmt, ...) { if(bse_debug.GetInteger() >= 4) { common->Printf(fmt, __VA_ARGS__); } }
#define BSE_ERROR(fmt, ...) { if(bse_debug.GetInteger() >= 5) { common->Printf(fmt, __VA_ARGS__); } }
#endif

#else

#if !defined(_MSC_VER)
#define BSE_VERBOSE(fmt, args...)
#define BSE_DEBUG(fmt, args...)
#define BSE_INFO(fmt, args...)
#define BSE_WARNING(fmt, args...)
#define BSE_ERROR(fmt, args...)
#else
#define BSE_VERBOSE(fmt, ...)
#define BSE_DEBUG(fmt, ...)
#define BSE_INFO(fmt, ...)
#define BSE_WARNING(fmt, ...)
#define BSE_ERROR(fmt, ...)
#endif
#endif

typedef struct {
	int						type;
	int						sibling;

	idStr					data;
	idStr					name;
	idStr					fire;

	float					delay;
	float					duration;
	float					restart;
	float					size;
	float					fadeInTime;
	float					fadeOutTime;
	float					shakeTime;
	float					shakeAmplitude;
	float					shakeDistance;
	float					shakeImpulse;
	float					lightRadius;
	float					rotate;
	float					random1;
	float					random2;

	idVec3					lightColor;
	idVec3					offset;
	idMat3					axis;

	bool					soundStarted;
	bool					shakeStarted;
	bool					shakeFalloff;
	bool					shakeIgnoreMaster;
	bool					bindParticles;
	bool					explicitAxis;
	bool					noshadows;
	bool					particleTrackVelocity;
	bool					trackOrigin;



	int seg;
	int ptype;
	idVec3					position;
	int count;
	bool useEndOrigin;
	float length;
} rvFXSingleAction;

#include "BSE_Fx.h"
#include "BSE_Particle.h"

class rvDeclEffectParser;
#endif

class rvDeclEffect : public idDecl
{
public:
	friend	class			rvBSE;
	friend  class			rvBSEManagerLocal;

	rvDeclEffect(void): mFlags(0) {  }
	virtual 						~rvDeclEffect(void) { }

	virtual bool					SetDefaultText(void);
	virtual const char* DefaultDefinition(void) const;
	virtual bool					Parse(const char* text, const int textLength, bool noCaching = false);
	virtual void					FreeData(void);
	virtual size_t					Size(void) const;
#ifdef _RAVEN_FX
	virtual void			Print(void) const;
	virtual void			List(void) const;

	idList<rvFXSingleAction>events;
	idStr					joint;
	idList<idStr> particles;

	friend class rvDeclEffectParser;
#endif

	int						mFlags;
	bool					GetHasSound(void) const { return(!!(mFlags & ETFLAG_HAS_SOUND)); }
};

enum {
	EFLAG_LOOPING = BITT< 0 >::VALUE,
	EFLAG_HASENDORIGIN = BITT< 1 >::VALUE,
	EFLAG_ENDORIGINCHANGED = BITT< 2 >::VALUE,
	EFLAG_STOPPED = BITT< 3 >::VALUE,
	EFLAG_ORIENTATE_IDENTITY = BITT< 4 >::VALUE,
	EFLAG_AMBIENT = BITT< 5 >::VALUE,
};

const int LIGHTID_EFFECT_LIGHT = 300;

class rvRenderEffectLocal;

class rvBSEManagerLocal : public rvBSEManager
{
public:
	virtual	bool				Init(void);
	virtual	bool				Shutdown(void);

	virtual	bool				PlayEffect(class rvRenderEffectLocal* def, float time);
	virtual	bool				ServiceEffect( class rvRenderEffectLocal *def, float time ) {
		bool forcePush = false;
		return ServiceEffect(def, time, forcePush);
	}
	virtual	bool				ServiceEffect(class rvRenderEffectLocal* def, float time, bool &forcePush);
	virtual	void				StopEffect(rvRenderEffectLocal* def);
	virtual	void					RestartEffect( rvRenderEffectLocal *def );
	virtual	void				FreeEffect(rvRenderEffectLocal* def);
	virtual	float				EffectDuration(const rvRenderEffectLocal* def);

	virtual	bool				CheckDefForSound(const renderEffect_t* def);

	virtual	void				BeginLevelLoad(void);
	virtual	void				EndLevelLoad(void);

	virtual	void				StartFrame(void);
	virtual	void				EndFrame(void);
	virtual bool				Filtered(const char* name, effectCategory_t category);

	virtual bool				CanPlayRateLimited(effectCategory_t category);
	virtual void				UpdateRateTimes(void) {}

	virtual void					SetShakeParms( float time, float scale ) {}
	virtual void					SetTunnelParms( float time, float scale ) {}
	
#if 0
	void							LoadAll(const idCmdArgs& args) { } ; // jmarshall: No code included in EXE for this function, not called
	void							Stats( const idCmdArgs &args );
	void							Pause( const idCmdArgs &args );
#endif
	rvBSE*							GetPlayingEffect(int handle, struct renderEntity_s* ent) { return NULL; }  // jmarshall: No code included in EXE for this function, not called

	virtual int							AddTraceModel(idTraceModel* model) { return 0; }
	virtual idTraceModel*				GetTraceModel(int index) { return NULL; }
	virtual void						FreeTraceModel(int index) {}

	virtual const idVec3&			GetCubeNormals(int index) { return vec3_origin; } // jmarshall: this is wrong
	virtual const idMat3&			GetModelToBSE() { return mat3_identity; }

	virtual bool					IsTimeLocked() const { return false; }
	virtual float					GetLockedTime() const { return 0.0; }

	virtual void					MakeEditable( class rvParticleTemplate *particle ) {}

	virtual void					CopySegment( class rvSegmentTemplate *dest, class rvSegmentTemplate *src ) {}

#ifdef _RAVEN_FX
	static idRandom random;
#endif
private:
#ifdef _RAVEN_FX
	static		idBlockAlloc<rvBSE, 256/*, 0 //k*/>	effects;
#endif
#if 0
	static		idVec3						mCubeNormals[6];
	static		idMat3						mModelToBSE;
	static		idList<idTraceModel*>		mTraceModels;
	static		const char* mSegmentNames[SEG_COUNT];
	static		int							mPerfCounters[NUM_PERF_COUNTERS];
	static		float						mEffectRates[EC_MAX];
	float								pauseTime;	// -1 means pause at the next time update
#endif
};
#endif
