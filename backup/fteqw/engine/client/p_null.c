#include "quakedef.h"
#include "glquake.h"

#include "particles.h"
#include "renderque.h"

//returns a valid effect if its existance is known.
static int PNULL_FindParticleType(const char *name)
{
//	Con_DPrintf("P_FindParticleType %s\n", name);
	return P_INVALID;
}

static int PNULL_RunParticleEffectTypeString (vec3_t org, vec3_t dir, float count, char *name){return 1;}
static int PNULL_ParticleTrail (vec3_t startpos, vec3_t end, int type, float timestep, int dlkey, vec3_t dlaxis[3], trailkey_t *tk){return 1;}
static int PNULL_RunParticleEffectState (vec3_t org, vec3_t dir, float count, int typenum, trailkey_t *tk){return 1;}
static void PNULL_RunParticleWeather(vec3_t minb, vec3_t maxb, vec3_t dir, float count, int colour, char *efname){}
static void PNULL_RunParticleCube(int typenum, vec3_t minb, vec3_t maxb, vec3_t dir_min, vec3_t dir_max, float count, int colour, qboolean gravity, float jitter){}
static void PNULL_RunParticleEffect (vec3_t org, vec3_t dir, int color, int count){}
static void PNULL_RunParticleEffect2 (vec3_t org, vec3_t dmin, vec3_t dmax, int color, int effect, int count){}
static void PNULL_RunParticleEffect3 (vec3_t org, vec3_t box, int color, int effect, int count){}
static void PNULL_RunParticleEffect4 (vec3_t org, float radius, int color, int effect, int count){}
static void PNULL_RunParticleEffectPalette (const char *nameprefix, vec3_t org, vec3_t dir, int color, int count){}

static void PNULL_ParticleTrailIndex (vec3_t start, vec3_t end, int type, float timestep, int color, int crnd, trailkey_t *tk){}

static qboolean PNULL_InitParticles (void)
{
	return true;
}

static void PNULL_ShutdownParticles(void)
{
}

static void PNULL_DelinkTrailstate(trailkey_t *tk)
{
	*tk = 0;
}
static void PNULL_ClearParticles (void){}
static void PNULL_DrawParticles(void)
{
	RSpeedLocals();

	RSpeedRemark();
	RQ_RenderBatchClear();
	RSpeedEnd(RSPEED_PARTICLESDRAW);
}


particleengine_t pe_null =
{
	"null",
	"none",

	PNULL_FindParticleType,
	NULL,

	PNULL_RunParticleEffectTypeString,
	PNULL_ParticleTrail,
	PNULL_RunParticleEffectState,
	PNULL_RunParticleWeather,
	PNULL_RunParticleCube,
	PNULL_RunParticleEffect,
	PNULL_RunParticleEffect2,
	PNULL_RunParticleEffect3,
	PNULL_RunParticleEffect4,
	PNULL_RunParticleEffectPalette,

	PNULL_ParticleTrailIndex,
	PNULL_InitParticles,
	PNULL_ShutdownParticles,
	PNULL_DelinkTrailstate,
	PNULL_ClearParticles,
	PNULL_DrawParticles
};
