#ifndef _PARTICLES_H_
#define _PARTICLES_H_

extern int 
	pt_gunshot,
	ptdp_gunshotquad,
	pt_spike,
	ptdp_spikequad,
	pt_superspike,
	ptdp_superspikequad,
	pt_wizspike,
	pt_knightspike,
	pt_explosion,
	ptdp_explosionquad,
	pt_tarexplosion,
	pt_teleportsplash,
	pt_lavasplash,
	ptdp_smallflash,
	ptdp_flamejet,
	ptdp_flame,
	ptdp_blood,
	ptdp_spark,
	ptdp_plasmaburn,
	ptdp_tei_g3,
	ptdp_tei_smoke,
	ptdp_tei_bigexplosion,
	ptdp_tei_plasmahit,
	ptdp_stardust,
	rt_rocket,
	rt_grenade,
	rt_blood,
	rt_wizspike,
	rt_slightblood,
	rt_knightspike,
	rt_vorespike,
	rtdp_nexuizplasma,
	rtdp_glowtrail,

	ptqw_blood,
	ptqw_lightningblood,

	rtqw_railtrail,	//common to zquake/fuhquake/fte
	ptfte_bullet,
	ptfte_superbullet;


#ifdef Q2CLIENT
/*WARNING: must match cl_tent.c*/
typedef enum
{
	/*MUST start with standard q2 te effects*/
	Q2TE_GUNSHOT = 0,	//0
	Q2TE_BLOOD,
	Q2TE_BLASTER,
	Q2TE_RAILTRAIL,
	Q2TE_SHOTGUN,
	Q2TE_EXPLOSION1,
	Q2TE_EXPLOSION2,
	Q2TE_ROCKET_EXPLOSION,
	Q2TE_GRENADE_EXPLOSION,
	Q2TE_SPARKS,
	Q2TE_SPLASH,	//10
	Q2TE_BUBBLETRAIL,
	Q2TE_SCREEN_SPARKS,
	Q2TE_SHIELD_SPARKS,
	Q2TE_BULLET_SPARKS,
	Q2TE_LASER_SPARKS,
	Q2TE_PARASITE_ATTACK,
	Q2TE_ROCKET_EXPLOSION_WATER,
	Q2TE_GRENADE_EXPLOSION_WATER,
	Q2TE_MEDIC_CABLE_ATTACK,
	Q2TE_BFG_EXPLOSION,	//20
	Q2TE_BFG_BIGEXPLOSION,
	Q2TE_BOSSTPORT,			// used as '22' in a map, so DON'T RENUMBER!!!
	Q2TE_BFG_LASER,
	Q2TE_GRAPPLE_CABLE,
	Q2TE_WELDING_SPARKS,
	Q2TE_GREENBLOOD,
	Q2TE_BLUEHYPERBLASTER,
	Q2TE_PLASMA_EXPLOSION,
	Q2TE_TUNNEL_SPARKS,
//ROGUE
	Q2TE_BLASTER2,	//30
	Q2TEEX_RAILTRAIL2,
	Q2TE_FLAME,
	Q2TE_LIGHTNING,
	Q2TE_DEBUGTRAIL,
	Q2TE_PLAIN_EXPLOSION,
	Q2TE_FLASHLIGHT,
	Q2TE_FORCEWALL,
	Q2TE_HEATBEAM,
	Q2TE_MONSTER_HEATBEAM,
	Q2TE_STEAM,		//40
	Q2TE_BUBBLETRAIL2,
	Q2TE_MOREBLOOD,
	Q2TE_HEATBEAM_SPARKS,
	Q2TE_HEATBEAM_STEAM,
	Q2TE_CHAINFIST_SMOKE,
	Q2TE_ELECTRIC_SPARKS,
	Q2TE_TRACKER_EXPLOSION,
	Q2TE_TELEPORT_EFFECT,
	Q2TE_DBALL_GOAL,
	Q2TE_WIDOWBEAMOUT,	//50
	Q2TE_NUKEBLAST,
	Q2TE_WIDOWSPLASH,
	Q2TE_EXPLOSION1_BIG,
	Q2TE_EXPLOSION1_NP,
	Q2TE_FLECHETTE,
//ROGUE

//RERELEASE
	Q2TEEX_BLUEHYPERBLASTER=56,
	Q2TEEX_BFGZAP,
	Q2TEEX_BERSERK_SLAM,
	Q2TEEX_GRAPPLE_CABLE_2,
	Q2TEEX_POWER_SPLASH,
	Q2TEEX_LIGHTNING_BEAM,
	Q2TEEX_EXPLOSION1_NL,
	Q2TEEX_EXPLOSION2_NL,
//RERELEASE

//CODERED
	CRTE_LEADERBLASTER=56,	//56
	CRTE_BLASTER_MUZZLEFLASH,
	CRTE_BLUE_MUZZLEFLASH,
	CRTE_SMART_MUZZLEFLASH,
	CRTE_LEADERFIELD,	//60
	CRTE_DEATHFIELD,
	CRTE_BLASTERBEAM,
	CRTE_STAIN,
	CRTE_FIRE,
	CRTE_CABLEGUT,
	CRTE_SMOKE,
//CODERED

#define Q2TE_MAX CRTE_SMOKE

	/*splashes are somewhat special, but are dynamically indexed*/
	Q2SPLASH_UNKNOWN,		//0
	Q2SPLASH_SPARKS,		//1
	Q2SPLASH_BLUE_WATER,	//2
	Q2SPLASH_BROWN_WATER,	//3
	Q2SPLASH_SLIME,			//4
	Q2SPLASH_LAVA,			//5
	Q2SPLASH_BLOOD,			//6
	Q2EXSPLASH_ELECTRIC,	//7
#define Q2SPLASH_MAX Q2EXSPLASH_ELECTRIC

	/*free form*/
	/*WARNING: must match cl_tent.c*/
	Q2RT_BLASTERTRAIL,
	Q2RT_BLASTERTRAIL2,
	Q2RT_GIB,
	Q2RT_GREENGIB,
	Q2RT_ROCKET,
	Q2RT_GRENADE,

	Q2RT_TRAP,
	Q2RT_FLAG1,
	Q2RT_FLAG2,
	Q2RT_TAGTRAIL,
	Q2RT_TRACKER,
	Q2RT_IONRIPPER,
	Q2RT_PLASMA,

	Q2PT_BFGPARTICLES,
	Q2PT_FLIES,
	Q2PT_TRAP,
	Q2PT_TRACKERSHELL,

	Q2PT_RESPAWN,
	Q2PT_PLAYER_TELEPORT,
	Q2PT_FOOTSTEP,
	Q2PT_OTHER_FOOTSTEP,
	Q2PT_LADDER_STEP,

	Q2PT_MAX
} q2particleeffects_t;
extern int pt_q2[];
#endif

typedef quint32_t trailkey_t;
#define trailkey_null 0

#define PARTICLE_Z_CLIP	8.0

typedef enum {
	BM_BLEND/*SRC_ALPHA ONE_MINUS_SRC_ALPHA*/,
	BM_BLENDCOLOUR/*SRC_COLOR ONE_MINUS_SRC_COLOR*/,
	BM_ADDA/*SRC_ALPHA ONE*/,
	BM_ADDC/*GL_SRC_COLOR GL_ONE*/,
	BM_SUBTRACT/*SRC_ALPHA ONE_MINUS_SRC_COLOR*/,
	BM_INVMODA/*ZERO ONE_MINUS_SRC_ALPHA*/,
	BM_INVMODC/*ZERO ONE_MINUS_SRC_COLOR*/,
	BM_PREMUL/*ONE ONE_MINUS_SRC_ALPHA*/,
	BM_RTSMOKE	/*special shader generation that causes these particles to be lit up by nearby rtlights, instead of just being fullbright junk*/
} blendmode_t;
#define frandom() (rand()*(1.0f/(float)RAND_MAX))
#define crandom() (rand()*(2.0f/(float)RAND_MAX)-1.0f)
#define hrandom() (rand()*(1.0f/(float)RAND_MAX)-0.5f)

#define P_INVALID -1

#define P_RunParticleEffectType(a,b,c,d) P_RunParticleEffectState(a,b,c,d,NULL)

// used for callback
extern cvar_t r_particlesdesc;
extern cvar_t r_particlesystem;

struct model_s;
struct msurface_s;

void P_InitParticleSystem(void);
void P_ShutdownParticleSystem(void);
void P_Shutdown(void);
void P_LoadedModel(struct model_s *mod);	/*checks a model's various effects*/
void P_DefaultTrail (unsigned int entityeffects, unsigned int modelflags, int *trailid, int *trailpalidx);
void P_EmitEffect (vec3_t pos, vec3_t orientation[3], unsigned int modeleflags, int type, trailkey_t *tsk);//this is just a wrapper
int P_FindParticleType(const char *efname);
#ifdef PSET_SCRIPT
void PScript_ClearSurfaceParticles(struct model_s *mod);
#endif

#define P_RunParticleEffectTypeString pe->RunParticleEffectTypeString
#define P_ParticleTrail pe->ParticleTrail
#define P_RunParticleEffectState pe->RunParticleEffectState
#define P_RunParticleWeather pe->RunParticleWeather
#define P_RunParticleCube pe->RunParticleCube
#define P_RunParticleEffect pe->RunParticleEffect
#define P_RunParticleEffect2 pe->RunParticleEffect2
#define P_RunParticleEffect3 pe->RunParticleEffect3
#define P_RunParticleEffect4 pe->RunParticleEffect4
#define P_RunParticleEffectPalette pe->RunParticleEffectPalette

#define P_ParticleTrailIndex pe->ParticleTrailIndex
#define P_InitParticles pe->InitParticles
#define P_DelinkTrailstate pe->DelinkTrailstate
#define P_ClearParticles pe->ClearParticles
#define P_DrawParticles pe->DrawParticles

typedef struct {
	char *name1;
	char *name2;

	int (*FindParticleType) (const char *name);
	qboolean (*ParticleQuery) (int type, int body, char *outstr, int outstrlen);

	int (*RunParticleEffectTypeString) (vec3_t org, vec3_t dir, float count, char *name);
	int (*ParticleTrail) (vec3_t startpos, vec3_t end, int type, float timeinterval, int dlkey, vec3_t dlaxis[3], trailkey_t *tk);
	int (*RunParticleEffectState) (vec3_t org, vec3_t dir, float count, int typenum, trailkey_t *tk);
	void (*RunParticleWeather) (vec3_t minb, vec3_t maxb, vec3_t dir, float count, int colour, char *efname);
	void (*RunParticleCube) (int typenum, vec3_t minb, vec3_t maxb, vec3_t dir_min, vec3_t dir_max, float count, int colour, qboolean gravity, float jitter); //typenum may be P_INVALID
	void (*RunParticleEffect) (vec3_t org, vec3_t dir, int color, int count);
	void (*RunParticleEffect2) (vec3_t org, vec3_t dmin, vec3_t dmax, int color, int effect, int count);
	void (*RunParticleEffect3) (vec3_t org, vec3_t box, int color, int effect, int count);
	void (*RunParticleEffect4) (vec3_t org, float radius, int color, int effect, int count);
	void (*RunParticleEffectPalette) (const char *nameprefix, vec3_t org, vec3_t dir, int color, int count);

	void (*ParticleTrailIndex) (vec3_t start, vec3_t end, int type, float timeinterval, int color, int crnd, trailkey_t *tk);	//P_INVALID is fine for the type here, you'll get a default trail.
	qboolean (*InitParticles) (void);
	void (*ShutdownParticles) (void);
	void (*DelinkTrailstate) (trailkey_t *tk);
	void (*ClearParticles) (void);
	void (*DrawParticles) (void);
} particleengine_t;
extern particleengine_t *pe;

#endif
