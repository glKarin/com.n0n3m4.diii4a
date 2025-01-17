#ifndef CL_PARTICLES_H
#define CL_PARTICLES_H

#include "qtypes.h"
struct entity_s;

extern struct cvar_s cl_particles;
extern struct cvar_s cl_particles_quality;
extern struct cvar_s cl_particles_size;
extern struct cvar_s cl_particles_quake;
extern struct cvar_s cl_particles_blood;
extern struct cvar_s cl_particles_blood_alpha;
extern struct cvar_s cl_particles_blood_decal_alpha;
extern struct cvar_s cl_particles_blood_decal_scalemin;
extern struct cvar_s cl_particles_blood_decal_scalemax;
extern struct cvar_s cl_particles_blood_bloodhack;
extern struct cvar_s cl_particles_bulletimpacts;
extern struct cvar_s cl_particles_explosions_sparks;
extern struct cvar_s cl_particles_explosions_shell;
extern struct cvar_s cl_particles_rain;
extern struct cvar_s cl_particles_snow;
extern struct cvar_s cl_particles_smoke;
extern struct cvar_s cl_particles_smoke_alpha;
extern struct cvar_s cl_particles_smoke_alphafade;
extern struct cvar_s cl_particles_sparks;
extern struct cvar_s cl_particles_bubbles;
extern struct cvar_s cl_decals;
extern struct cvar_s cl_decals_time;
extern struct cvar_s cl_decals_fadetime;

typedef enum
{
	PARTICLE_BILLBOARD = 0,
	PARTICLE_SPARK = 1,
	PARTICLE_ORIENTED_DOUBLESIDED = 2,
	PARTICLE_VBEAM = 3,
	PARTICLE_HBEAM = 4,
	PARTICLE_INVALID = -1
}
porientation_t;

typedef enum
{
	PBLEND_ALPHA = 0,
	PBLEND_ADD = 1,
	PBLEND_INVMOD = 2,
	PBLEND_INVALID = -1
}
pblend_t;

typedef struct particletype_s
{
	pblend_t blendmode;
	porientation_t orientation;
	qbool lighting;
}
particletype_t;

typedef enum ptype_e
{
	pt_dead,
	pt_alphastatic,
	pt_static,
	pt_spark,
	pt_beam,
	pt_rain,
	pt_raindecal,
	pt_snow,
	pt_bubble,
	pt_blood,
	pt_smoke,
	pt_decal,
	pt_entityparticle,
	pt_explode,   // used for Quake-style explosion particle colour ramping
	pt_explode2,  // used for Quake-style explosion particle colour ramping
	pt_total
}
ptype_t;

typedef struct particle_s
{
	// for faster batch rendering, particles are rendered in groups by effect (resulting in less perfect sorting but far less state changes)

	// fields used by rendering: (48 bytes)
	vec3_t          sortorigin; ///< sort by this group origin, not particle org
	vec3_t          org;
	vec3_t          vel;        ///< velocity of particle, or orientation of decal, or end point of beam
	float           size;
	float           alpha;      ///< 0-255
	float           stretch;    ///< only for sparks

	// fields not used by rendering:  (44 bytes)
	float           stainsize;
	float           stainalpha;
	float           sizeincrease;   ///< rate of size change per second
	float           alphafade;      ///< how much alpha reduces per second
	float           time2;          ///< used for snow fluttering, decal fade, explosion colour ramp
	float           bounce;         ///< how much bounce-back from a surface the particle hits (0 = no physics, 1 = stop and slide, 2 = keep bouncing forever, 1.5 is typical)
	float           gravity;        ///< how much gravity affects this particle (1.0 = normal gravity, 0.0 = none)
	float           airfriction;    ///< how much air friction affects this object (objects with a low mass/size ratio tend to get more air friction)
	float           liquidfriction; ///< how much liquid friction affects this object (objects with a low mass/size ratio tend to get more liquid friction)
//	float           delayedcollisions; ///< time that p->bounce becomes active
	float           delayedspawn;   ///< time that particle appears and begins moving
	float           die;            ///< time when this particle should be removed, regardless of alpha

	// short variables grouped to save memory (4 bytes)
	short           angle; ///< base rotation of particle
	short           spin;  ///< geometry rotation speed around the particle center normal

	// byte variables grouped to save memory (12 bytes)
	unsigned char   color[3];
	unsigned char   qualityreduction; ///< enables skipping of this particle according to r_refdef.view.qualityreduction
	unsigned char   typeindex;
	unsigned char   blendmode;
	unsigned char   orientation;
	unsigned char   texnum;
	unsigned char   staincolor[3];
	signed char     staintexnum;
}
particle_t;

void CL_Particles_Clear(void);
void CL_Particles_Init(void);
void CL_Particles_Shutdown(void);
particle_t *CL_NewParticle(const vec3_t sortorigin, unsigned short ptypeindex, int pcolor1, int pcolor2, int ptex, float psize, float psizeincrease, float palpha, float palphafade, float pgravity, float pbounce, float px, float py, float pz, float pvx, float pvy, float pvz, float pairfriction, float pliquidfriction, float originjitter, float velocityjitter, qbool pqualityreduction, float lifetime, float stretch, pblend_t blendmode, porientation_t orientation, int staincolor1, int staincolor2, int staintex, float stainalpha, float stainsize, float angle, float spin, float tint[4]);
particle_t *CL_NewQuakeParticle(const vec3_t origin, const unsigned short ptypeindex, const int color_1, const int color_2, const float gravity, const float offset_x, const float offset_y, const float offset_z, const float velocity_offset_x, const float velocity_offset_y, const float velocity_offset_z, const float air_friction, const float liquid_friction, const float origin_jitter, const float velocity_jitter, const float lifetime);

typedef enum effectnameindex_s
{
	EFFECT_NONE,
	EFFECT_TE_GUNSHOT,
	EFFECT_TE_GUNSHOTQUAD,
	EFFECT_TE_SPIKE,
	EFFECT_TE_SPIKEQUAD,
	EFFECT_TE_SUPERSPIKE,
	EFFECT_TE_SUPERSPIKEQUAD,
	EFFECT_TE_WIZSPIKE,
	EFFECT_TE_KNIGHTSPIKE,
	EFFECT_TE_EXPLOSION,
	EFFECT_TE_EXPLOSIONQUAD,
	EFFECT_TE_TAREXPLOSION,
	EFFECT_TE_TELEPORT,
	EFFECT_TE_LAVASPLASH,
	EFFECT_TE_SMALLFLASH,
	EFFECT_TE_FLAMEJET,
	EFFECT_EF_FLAME,
	EFFECT_TE_BLOOD,
	EFFECT_TE_SPARK,
	EFFECT_TE_PLASMABURN,
	EFFECT_TE_TEI_G3,
	EFFECT_TE_TEI_SMOKE,
	EFFECT_TE_TEI_BIGEXPLOSION,
	EFFECT_TE_TEI_PLASMAHIT,
	EFFECT_EF_STARDUST,
	EFFECT_TR_ROCKET,
	EFFECT_TR_GRENADE,
	EFFECT_TR_BLOOD,
	EFFECT_TR_WIZSPIKE,
	EFFECT_TR_SLIGHTBLOOD,
	EFFECT_TR_KNIGHTSPIKE,
	EFFECT_TR_VORESPIKE,
	EFFECT_TR_NEHAHRASMOKE,
	EFFECT_TR_NEXUIZPLASMA,
	EFFECT_TR_GLOWTRAIL,
	EFFECT_SVC_PARTICLE,
	EFFECT_TOTAL
}
effectnameindex_t;

int CL_ParticleEffectIndexForName(const char *name);
const char *CL_ParticleEffectNameForIndex(int i);
void CL_ParticleEffect(int effectindex, float pcount, const vec3_t originmins, const vec3_t originmaxs, const vec3_t velocitymins, const vec3_t velocitymaxs, struct entity_s *ent, int palettecolor);
void CL_ParticleTrail(int effectindex, float pcount, const vec3_t originmins, const vec3_t originmaxs, const vec3_t velocitymins, const vec3_t velocitymaxs, struct entity_s *ent, int palettecolor, qbool spawndlight, qbool spawnparticles, float tintmins[4], float tintmaxs[4], float fade);
void CL_ParticleBox(int effectindex, float pcount, const vec3_t originmins, const vec3_t originmaxs, const vec3_t velocitymins, const vec3_t velocitymaxs, struct entity_s *ent, int palettecolor, qbool spawndlight, qbool spawnparticles, float tintmins[4], float tintmaxs[4], float fade);
void CL_ParseParticleEffect (void);
void CL_ParticleCube (const vec3_t mins, const vec3_t maxs, const vec3_t dir, int count, int colorbase, vec_t gravity, vec_t randomvel);
void CL_ParticleRain (const vec3_t mins, const vec3_t maxs, const vec3_t dir, int count, int colorbase, int type);
void CL_EntityParticles (const struct entity_s *ent);
void CL_ParticleExplosion (const vec3_t org);
void CL_ParticleExplosion2 (const vec3_t org, int colorStart, int colorLength);

#endif
