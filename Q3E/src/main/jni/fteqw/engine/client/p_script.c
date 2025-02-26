/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

/*
The aim of this particle system is to have as much as possible configurable.
Some parts still fail here, and are marked FIXME
Effects are flushed on new maps.
The engine has a few builtins.
*/

#include "quakedef.h"

#ifdef PSET_SCRIPT


#ifdef FTE_TARGET_WEB
#define rand myrand	//emscripten's libc is doing a terrible job of this.
static int rand(void)
{	//ripped from glibc
	static int state = 0xdeadbeef;
	int val = ((state * 1103515245) + 12345) & 0x7fffffff;
	state = val;
	return val;
}
#endif

#ifdef GLQUAKE
#include "glquake.h"//hack
#endif
#include "shader.h"

#include "renderque.h"

#ifdef HAVE_LEGACY
#include "r_partset.h"
#else
#define R_PARTSET_BUILTINS
#endif

#include "pr_common.h"
extern world_t csqc_world;

struct
{
	char *name;
	char **data;
} partset_list[] =
{
	{"none", NULL},
	R_PARTSET_BUILTINS
	{NULL}
};

extern qbyte *host_basepal;

static float throttle;

extern int PClassic_PointFile(int c, vec3_t point);
extern particleengine_t pe_classic;
particleengine_t *fallback = NULL; //does this really need to be 'extern'?
#define FALLBACKBIAS 0x1000000

#define PART_VALID(part) ((part) >= 0 && (part) < FALLBACKBIAS)	//copes with fallback indexes

static int pe_default = P_INVALID;
static int pe_size2 = P_INVALID;
static int pe_size3 = P_INVALID;
static int pe_defaulttrail = P_INVALID;
static qboolean pe_script_enabled;

static float psintable[256];

static qboolean P_LoadParticleSet(char *name, qboolean implicit, qboolean showwarning);
static void R_Particles_KillAllEffects(void);

static void buildsintable(void)
{
	int i;
	for (i = 0; i < 256; i++)
		psintable[i] = sin((i*M_PI)/128);
}
#define sin(x) (psintable[(int)((x)*(128/M_PI)) & 255])
#define cos(x) (psintable[((int)((x)*(128/M_PI)) + 64) & 255])

typedef struct particle_s
{
	struct particle_s	*next;
	float		die;

// driver-usable fields
	vec3_t		org;
	vec4_t		rgba;
	float		scale;
	float		s1, t1, s2, t2;

	vec3_t		oldorg;	//to throttle traces
	vec3_t		vel;	//renderer uses for sparks
	float		angle;
	union {
		float nextemit;
		trailkey_t trailstate;
	} state;
// drivers never touch the following fields
	float		rotationspeed;
} particle_t;

typedef struct clippeddecal_s
{
	struct clippeddecal_s	*next;
	float		die;

	int entity;		//>0 is a lerpentity, <0 is a csqc ent. 0 is world. woot.
//	model_t *model;	//just for paranoia

	vec3_t		vertex[3];
	vec2_t		texcoords[3];
	float		valpha[3];

	vec4_t		rgba;
} clippeddecal_t;

#define BS_LASTSEG 0x1 // no draw to next, no delete
#define BS_DEAD    0x2 // segment is dead
#define BS_NODRAW  0x4 // only used for lerp switching

typedef struct beamseg_s
{
	struct beamseg_s *next;  // next in beamseg list

	particle_t *p;
	int    flags;            // flags for beamseg
	vec3_t dir;

	float texture_s;
} beamseg_t;

typedef struct trailstate_s {
	trailkey_t key;  // key to check if ts has been overwriten
	trailkey_t assoc; // assoc linked trail
	struct beamseg_s* lastbeam; // last beam pointer (flagged with BS_LASTSEG)
	union {
		struct {
			float lastdist;			// last distance used with particle effect
			float laststop;			// last stopping point for particle effect
		} trail;
		struct {
			float statetime;		// time to emit effect again (used by spawntime field)
			float emittime;			// used by r_effect emitters
		} effect;
		trailkey_t fallbackkey; // passed to fallback system
	};
} trailstate_t;


typedef struct skytris_s {
	struct skytris_s	*next;
	vec3_t	org;
	vec3_t	x;
	vec3_t	y;
	float	area;
	float	nexttime;
	int		ptype;
	struct msurface_s	*face;
} skytris_t;

typedef struct skytriblock_s
{
	struct skytriblock_s *next;
	int count;
	skytris_t tris[1024];
} skytriblock_t;

//this is the required render state for each particle
//dynamic per-particle stuff isn't important. only static state.
typedef struct {
	enum {PT_NORMAL, PT_SPARK, PT_SPARKFAN, PT_TEXTUREDSPARK, PT_BEAM, PT_VBEAM, PT_CDECAL, PT_UDECAL, PT_INVISIBLE} type;

	blendmode_t blendmode;
	shader_t *shader;
	qboolean	nearest;

	float scalefactor;
	float invscalefactor;
	float stretch;
	float minstretch;	//limits the particle's length to a multiple of its width.
	int premul;	//0: direct rgba. 1: rgb*a,a (blend). 2: rgb*a,0 (add).
} plooks_t;

//these could be deltas or absolutes depending on ramping mode.
typedef struct {
	vec3_t rgb;
	float alpha;
	float scale;
	float rotation;
} ramp_t;
typedef struct {
	char name[MAX_QPATH];
	model_t *model;
	float framestart;
	float framecount;
	float framerate;
	float alpha;
	vec3_t rgb;
	float scalemin, scalemax;
	int skin;
	int traileffect;
	unsigned int rflags;
#define RF_USEORIENTATION Q2RF_CUSTOMSKIN	//private flag
} partmodels_t;
typedef struct {
	char name[MAX_QPATH];
	float vol;
	float atten;
	float delay;
	float pitch;
	float weight;
} partsounds_t;
// TODO: merge in alpha with rgb to gain benefit of vector opts
typedef struct part_type_s {
	char name[MAX_QPATH];
	char config[MAX_QPATH];
	char texname[MAX_QPATH];

	int nummodels;
	partmodels_t *models;

	int numsounds;
	partsounds_t *sounds;

	vec3_t rgb;	//initial colour
	float alpha;
	vec3_t rgbchange;	//colour delta (per second)
	float alphachange;
	vec3_t rgbrand;		//random rgb colour to start with
	float alpharand;
	int colorindex;		//get colour from a palette
	int colorrand;		//and add up to this amount
	float rgbchangetime;//colour stops changing at this time
	vec3_t rgbrandsync;	//like rgbrand, but a single random value instead of separate (can mix)
	float scale;		//initial scale
	float scalerand;	//with up to this much extra
	float die, randdie;	//how long it lasts (plus some rand)
	float veladd, randomveladd;		//scale the incoming velocity by this much
	float orgadd, randomorgadd;		//spawn the particle this far along its velocity direction
	float spawnvel, spawnvelvert; //spawn the particle with a velocity based upon its spawn type (generally so it flies outwards)
	vec3_t orgbias;		//static 3d world-coord bias
	vec3_t velbias;
	vec3_t orgwrand;	//3d world-coord randomisation without relation to spawn mode
	vec3_t velwrand;	//3d world-coord randomisation without relation to spawn mode
	float viewspacefrac;
	float flurry;
	int surfflagmatch;	//this decal only spawns on these surfaces
	int surfflagmask;	//this decal only spawns on these surfaces

	float s1, t1, s2, t2;	//texture coords
	float texsstride;	//addition for s for each random slot.
	int randsmax;	//max times the stride can be added

	plooks_t *slooks;	//shared looks, so state switches don't apply between particles so much.
	plooks_t looks;		//

	float spawntime;	//time limit for trails
	float spawnchance;	//if < 0, particles might not spawn so many

	float rotationstartmin, rotationstartrand;
	float rotationmin, rotationrand;

	float scaledelta;
	float countextra;
	float count;
	float countrand;
	float countspacing; //for trails.
	float countoverflow; //for badly-designed effects, instead of depending on trail state.
	float rainfrequency;

	int assoc;
	int cliptype;
	int inwater;
	float clipcount;
	int emit;
	float emittime;
	float emitrand;
	float emitstart;

	float areaspread;
	float areaspreadvert;

	float spawnparam1;
	float spawnparam2;
/*	float spawnparam3; */

	enum {
		SM_BOX, //box = even spread within the area
		SM_CIRCLE, //circle = around edge of a circle
		SM_BALL, //ball = filled sphere
		SM_SPIRAL, //spiral = spiral trail
		SM_TRACER, //tracer = tracer trail
		SM_TELEBOX, //telebox = q1-style telebox
		SM_LAVASPLASH, //lavasplash = q1-style lavasplash
		SM_UNICIRCLE, //unicircle = uniform circle
		SM_FIELD, //field = synced field (brightfield, etc)
		SM_DISTBALL, // uneven distributed ball
		SM_MESHSURFACE, //distributed roughly evenly over the surface of the mesh
		SM_FIXMEWARNING,	//for people to use to mark placeholder effects.
	} spawnmode;

	float gravity;
	vec3_t friction;
	float clipbounce;
	float stainonimpact;

	vec3_t dl_rgb;
	float dl_radius[2];
	float dl_time;
	vec4_t dl_decay;
	float dl_corona_intensity;
	float dl_corona_scale;
	vec3_t dl_scales;
	//PT_NODLSHADOW
	int dl_cubemapnum;
	int dl_lightstyle;
	vec3_t stain_rgb;
	float stain_radius;

	enum {RAMP_NONE, RAMP_DELTA, RAMP_NEAREST, RAMP_LERP} rampmode;
	int rampindexes;
	ramp_t *ramp;

	int loaded;	//0 if not loaded, 1 if automatically loaded, 2 if user loaded
	particle_t	*particles;
	clippeddecal_t *clippeddecals;
	beamseg_t *beams;
	struct part_type_s *nexttorun;
	struct part_type_s **runlink;

	unsigned int flags;
#define PT_VELOCITY			0x0001	// has velocity modifiers
#define PT_FRICTION			0x0002	// has friction modifiers
#define PT_CHANGESCOLOUR	0x0004
#define PT_CITRACER			0x0008	// Q1-style tracer behavior for colorindex
#define PT_INVFRAMETIME		0x0010	// apply inverse frametime to count (causes emits to be per frame)
#define PT_AVERAGETRAIL		0x0020	// average trail points from start to end, useful with t_lightning, etc
#define PT_NOSTATE			0x0040	// don't use trailstate for this emitter (careful with assoc...)
#define PT_NOSPREADFIRST	0x0080	// don't randomize org/vel for first generated particle
#define PT_NOSPREADLAST		0x0100	// don't randomize org/vel for last generated particle
#define PT_TROVERWATER		0x0200	// don't spawn if underwater
#define PT_TRUNDERWATER		0x0400	// don't spawn if overwater
#define PT_NODLSHADOW		0x0800	// dlights from this effect don't cast shadows.
#define PT_WORLDSPACERAND	0x1000	// effect has orgwrand or velwrand properties
	unsigned int fluidmask;

	unsigned int state;
#define PS_INRUNLIST 0x1 // particle type is currently in execution list
} part_type_t;

typedef struct pcfg_s
{
	struct pcfg_s *next;
	char name[1];
} pcfg_t;
static pcfg_t *loadedconfigs;

#ifndef TYPESONLY

//triangle fan sparks use these. // defined but not used
//static double sint[7] = {0.000000, 0.781832,  0.974928,  0.433884, -0.433884, -0.974928, -0.781832};
//static double cost[7] = {1.000000, 0.623490, -0.222521, -0.900969, -0.900969, -0.222521,  0.623490};

#define crand() (rand()%32767/16383.5f-1)

static void P_ReadPointFile_f (void);
#ifdef HAVE_LEGACY
static void P_ExportBuiltinSet_f(void);
#endif

#define MAX_BEAMSEGS			(1<<11)	// default max # of beam segments
#define MAX_PARTICLES			(1<<18)	// max # of particles at one time
#define MAX_DECALS				(1<<18)	// max # of decal fragments at one time
#define MAX_TRAILSTATES			(1<<10)	// default max # of trailstates

//int		ramp1[8] = {0x6f, 0x6d, 0x6b, 0x69, 0x67, 0x65, 0x63, 0x61};
//int		ramp2[8] = {0x6f, 0x6e, 0x6d, 0x6c, 0x6b, 0x6a, 0x68, 0x66};
//int		ramp3[8] = {0x6d, 0x6b, 6,	  5,    4,    3,    2,    1};

particle_t	*free_particles;
particle_t	*particles;	//contains the initial list of alloced particles.
int			r_numparticles;
int			r_particlerecycle;

beamseg_t   *free_beams;
beamseg_t   *beams;
int			r_numbeams;

clippeddecal_t	*free_decals;
clippeddecal_t	*decals;
int			r_numdecals;
int			r_decalrecycle;

trailstate_t *trailstates;
int			ts_cycle; // current cyclic index of trailstates
int			r_numtrailstates;

static		qboolean r_plooksdirty;	//a particle effect was changed, reevaluate shared looks.

extern cvar_t r_bouncysparks;
extern cvar_t r_part_rain;
extern cvar_t r_bloodstains;
extern cvar_t gl_part_flame;
extern cvar_t r_decal_noperpendicular;

static void PScript_EmitSkyEffectTris(model_t *mod, msurface_t 	*fa, int ptype);
static void FinishParticleType(part_type_t *ptype);
// callbacks
static void QDECL R_ParticleDesc_Callback(struct cvar_s *var, char *oldvalue);

extern cvar_t r_particledesc;
extern cvar_t r_part_rain_quantity;
extern cvar_t r_particle_tracelimit;
extern cvar_t r_part_sparks;
extern cvar_t r_part_sparks_trifan;
extern cvar_t r_part_sparks_textured;
extern cvar_t r_part_beams;
extern cvar_t r_part_contentswitch;
extern cvar_t r_part_density;
extern cvar_t r_part_maxparticles;
extern cvar_t r_part_maxdecals;

static float particletime;

#define BUFFERVERTS 2048*4
static vecV_t pscriptverts[BUFFERVERTS];
static avec4_t pscriptcolours[BUFFERVERTS];
static vec2_t pscripttexcoords[BUFFERVERTS];
static index_t pscriptquadindexes[(BUFFERVERTS/4)*6];
static index_t pscripttriindexes[BUFFERVERTS];
static mesh_t pscriptmesh;
static mesh_t pscripttmesh;

static int numparticletypes;
static part_type_t *part_type;
static part_type_t *part_run_list;

extern char part_parsenamespace[MAX_QPATH];
static qboolean part_parseweak;

static struct {
	char *oldn;
	char *newn;
} legacynames[] =
{
	{"t_rocket",	"TR_ROCKET"},

	//{"t_blastertrail", "TR_BLASTERTRAIL"},
	{"t_grenade",	"TR_GRENADE"},
	{"t_gib",		"TR_BLOOD"},

	{"te_plasma",	"TE_TEI_PLASMAHIT"},
	{"te_smoke",	"TE_TEI_SMOKE"},

	{NULL}
};

static part_type_t *P_GetParticleType(const char *config, const char *name)
{
	int i;
	part_type_t *ptype;
	part_type_t *oldlist = part_type;
	char cfgbuf[MAX_QPATH];
	char *dot = strchr(name, '.');
	if (dot && (dot - name) < MAX_QPATH-1)
	{
		config = cfgbuf;
		memcpy(cfgbuf, name, dot - name);
		cfgbuf[dot - name] = 0;
		name = dot+1;
	}

	for (i = 0; legacynames[i].oldn; i++)
	{
		if (!strcmp(name, legacynames[i].oldn))
		{
			name = legacynames[i].newn;
			break;
		}
	}
	for (i = 0; i < numparticletypes; i++)
	{
		ptype = &part_type[i];
		if (!stricmp(ptype->name, name))
			if (!stricmp(ptype->config, config))	//must be an exact match.
				return ptype;
	}
	part_type = BZ_Realloc(part_type, sizeof(part_type_t)*(numparticletypes+1));
	ptype = &part_type[numparticletypes++];
	memset(ptype, 0, sizeof(*ptype));
	Q_strncpyz(ptype->name, name, sizeof(ptype->name));
	Q_strncpyz(ptype->config, config, sizeof(ptype->config));
	ptype->assoc = P_INVALID;
	ptype->inwater = P_INVALID;
	ptype->cliptype = P_INVALID;
	ptype->emit = P_INVALID;

	if (oldlist)
	{
		part_type_t **link;
		if (part_run_list)
			part_run_list = (part_type_t*)((char*)part_run_list - (char*)oldlist + (char*)part_type);

		for (i = 0; i < numparticletypes; i++)
		{
			if (part_type[i].nexttorun)
				part_type[i].nexttorun = (part_type_t*)((char*)part_type[i].nexttorun - (char*)oldlist + (char*)part_type);
			part_type[i].runlink = NULL;
		}
		for (link = &part_run_list; *link;)
		{
			(*link)->runlink = link;
			link = &(*link)->nexttorun;
		}
	}

	ptype->loaded = 0;
	ptype->ramp = NULL;
	ptype->particles = NULL;
	ptype->beams = NULL;

	r_plooksdirty = true;
	return ptype;
}

//unconditionally allocates a particle object. this allows out-of-order allocations.
static int P_AllocateParticleType(const char *config, const char *name)	//guarentees that the particle type exists, returning it's index.
{
	part_type_t *pt = P_GetParticleType(config, name);
	return pt - part_type;
}

static void PScript_RetintEffect(part_type_t *to, part_type_t *from, const char *colourcodes)
{
	char name[sizeof(to->name)];
	char config[sizeof(to->config)];

	Q_strncpyz(name, to->name, sizeof(to->name));
	Q_strncpyz(config, to->config, sizeof(to->config));

	//'to' was already purged, so we don't need to care about that.
	memcpy(to, from, sizeof(*to));

	Q_strncpyz(to->name, name, sizeof(to->name));
	Q_strncpyz(to->config, config, sizeof(to->config));

	//make sure 'to' has its own copy of any lists, so that we don't have issues when freeing this memory again.
	if (to->models)
	{
		to->models = BZ_Malloc(to->nummodels * sizeof(*to->models));
		memcpy(to->models, from->models, to->nummodels * sizeof(*to->models));
	}
	if (to->sounds)
	{
		to->sounds = BZ_Malloc(to->numsounds * sizeof(*to->sounds));
		memcpy(to->sounds, from->sounds, to->numsounds * sizeof(*to->sounds));
	}
	if (to->ramp)
	{
		to->ramp = BZ_Malloc(to->rampindexes * sizeof(*to->ramp));
		memcpy(to->ramp, from->ramp, to->rampindexes * sizeof(*to->ramp));
	}

	//'from' might still have some links so we need to clear those out.
	to->nexttorun = NULL;
	to->runlink = NULL;
	to->particles = NULL;
	to->clippeddecals = NULL;
	to->beams = NULL;
	to->slooks = &to->looks;
	r_plooksdirty = true;

	to->colorindex = strtoul(colourcodes, (char**)&colourcodes, 10);
	if (*colourcodes == '_')
		colourcodes++;
	to->colorrand = strtoul(colourcodes, (char**)&colourcodes, 10);
}

//public interface. get without creating.
static int PScript_FindParticleType(const char *fullname)
{
	int i;
	part_type_t *ptype = NULL;
	char cfg[MAX_QPATH];
	char *dot;
	const char *name = fullname;
	dot = strchr(name, '.');
	if (dot && (dot - name) < MAX_QPATH-1)
	{
		memcpy(cfg, name, dot - name);
		cfg[dot-name] = 0;
		name = dot+1;
	}
	else
		*cfg = 0;

	for (i = 0; legacynames[i].oldn; i++)
	{
		if (!strcmp(name, legacynames[i].oldn))
		{
			name = legacynames[i].newn;
			break;
		}
	}

	if (*cfg)
	{	//favour the namespace if one is specified
		for (i = 0; i < numparticletypes; i++)
		{
			if (!stricmp(part_type[i].name, name))
			{
				if (!stricmp(part_type[i].config, cfg))
				{
					ptype = &part_type[i];
					break;
				}
			}
		}
	}
	else
	{
		//but be prepared to load it from any namespace if its not got a namespace specified.
		for (i = 0; i < numparticletypes; i++)
		{
			if (!stricmp(part_type[i].name, name))
			{
				ptype = &part_type[i];
				if (ptype->loaded)	//(mostly) ignore ones that are not currently loaded
					break;
			}
		}
	}
	if (!ptype || !ptype->loaded)
	{
		if (!strnicmp(name, "te_explosion2_", 14))
		{
			int from = PScript_FindParticleType(va("%s.te_explosion2", cfg));
			if (from != P_INVALID)
			{
				int to = P_AllocateParticleType(part_type[from].config, name);
				PScript_RetintEffect(&part_type[to], &part_type[from], name+14);
				return to;
			}
		}
		if (*cfg)
			P_LoadParticleSet(cfg, true, true);

		if (fallback)
		{
			if (!strncmp(name, "classic_", 8))
				i = fallback->FindParticleType(name+8);
			else
				i = fallback->FindParticleType(name);
			if (i != P_INVALID)
				return i+FALLBACKBIAS;
		}
		return P_INVALID;
	}
	return i;
}

static void P_SetModified(void)	//called when the particle system changes (from console).
{
	if (Cmd_IsInsecure())
		return;	//server stuffed particle descriptions don't count.

	f_modified_particles = true;

	if (care_f_modified)
	{
		care_f_modified = false;
		Cbuf_AddText("say particles description has changed\n", RESTRICT_LOCAL);
	}
}
static int CheckAssosiation(char *config, char *name, int from)
{
	int to, orig;

	orig = to = P_AllocateParticleType(config, name);

	while(to != P_INVALID)
	{
		if (to == from)
		{
			Con_Printf("Assosiation of %s would cause infinate loop\n", name);
			return P_INVALID;
		}
		to = part_type[to].assoc;
	}
	return orig;
}

static void P_LoadTexture(part_type_t *ptype, qboolean warn)
{
	texnums_t tn;
	char *defaultshader;
	char *namepostfix;
	int i;
	if (qrenderer == QR_NONE)
		return;

	for (i = 0; i < ptype->nummodels; i++)
		ptype->models[i].model = NULL;

	if (*ptype->texname)
	{
		char *bmpostfix;
		switch(ptype->looks.blendmode)
		{	//we typically need the blendmode as part of the shader name, so that we don't end up with collisions with default shaders and different particle blend modes.
			//shader blend modes still override, although I guess this way the shader itself can contain conditionals to use different blend modes... if needed.
		default:			bmpostfix = "#BLEND"; break;
		case BM_BLEND:		bmpostfix = ""; break;
		case BM_BLENDCOLOUR:bmpostfix = "#BLENDCOLOUR"; break;
		case BM_ADDA:		bmpostfix = "#ADDA"; break;
		case BM_ADDC:		bmpostfix = "#ADDC"; break;
		case BM_SUBTRACT:	bmpostfix = "#SUBTRACT"; break;
		case BM_INVMODA:	bmpostfix = "#INVMODA"; break;
		case BM_INVMODC:	bmpostfix = "#INVMODC"; break;
		case BM_PREMUL:		bmpostfix = "#PREMUL"; break;
		case BM_RTSMOKE:	bmpostfix = "#RTSMOKE"; break;
		}
		/*try and load the shader, fail if we would need to generate one*/
		ptype->looks.shader = R_RegisterCustom(NULL, va("%s%s", ptype->texname, bmpostfix), SUF_NONE, NULL, NULL);
	}
	else
		ptype->looks.shader = NULL;

	if (!ptype->looks.shader)
	{
		/*okay, so no shader, generate a shader that matches the legacy/shaderless mode*/
		switch(ptype->looks.blendmode)
		{
		case BM_BLEND:
		default:
			namepostfix = "_blend";
			defaultshader =
				"{\n"
					"program defaultsprite\n"
					"nomipmaps\n"
					"sort unlitdecal\n"
					"{\n"
						"map $diffuse\n"
						"blendfunc blend\n"
						"rgbgen vertex\n"
						"alphagen vertex\n"
						"nodepth\n"
					"}\n"
					"polygonoffset\n"
					"surfaceparm noshadows\n"
					"surfaceparm nodlight\n"
				"}\n"
				;
			break;
		case BM_BLENDCOLOUR:
			namepostfix = "_bc";
			defaultshader =
				"{\n"
					"program defaultsprite\n"
					"nomipmaps\n"
					"sort unlitdecal\n"
					"{\n"
						"map $diffuse\n"
						"blendfunc GL_SRC_COLOR GL_ONE_MINUS_SRC_COLOR\n"
						"rgbgen vertex\n"
						"alphagen vertex\n"
						"nodepth\n"
					"}\n"
					"polygonoffset\n"
					"surfaceparm noshadows\n"
					"surfaceparm nodlight\n"
				"}\n"
				;
			break;
		case BM_ADDA:
			namepostfix = "_add";
			defaultshader =
				"{\n"
					"program defaultsprite\n"
					"nomipmaps\n"
					"sort additive\n"
					"{\n"
						"map $diffuse\n"
						"blendfunc GL_SRC_ALPHA GL_ONE\n"
						"rgbgen vertex\n"
						"alphagen vertex\n"
						"nodepth\n"
					"}\n"
					"polygonoffset\n"
					"surfaceparm noshadows\n"
					"surfaceparm nodlight\n"
				"}\n"
				;
			break;
		case BM_ADDC:
			namepostfix = "_add";
			defaultshader =
				"{\n"
					"program defaultsprite\n"
					"nomipmaps\n"
					"sort additive\n"
					"{\n"
						"map $diffuse\n"
						"blendfunc GL_SRC_COLOR GL_ONE\n"
						"rgbgen vertex\n"
						"alphagen vertex\n"
						"nodepth\n"
					"}\n"
					"polygonoffset\n"
					"surfaceparm noshadows\n"
					"surfaceparm nodlight\n"
				"}\n"
				;
			break;
		case BM_PREMUL:
			namepostfix = "_premul";
			defaultshader =
				"{\n"
					"program defaultsprite\n"
					"nomipmaps\n"
					"sort additive\n"
					"{\n"
						"map $diffuse\n"
						"blendfunc GL_ONE GL_ONE_MINUS_SRC_ALPHA\n"
						"rgbgen vertex\n"
						"alphagen vertex\n"
						"nodepth\n"
					"}\n"
					"polygonoffset\n"
					"surfaceparm noshadows\n"
					"surfaceparm nodlight\n"
				"}\n"
				;
			break;
		case BM_INVMODA:
			namepostfix = "_invmoda";
			defaultshader =
				"{\n"
					"program defaultsprite\n"
					"nomipmaps\n"
					"sort unlitdecal\n"
					"{\n"
						"map $diffuse\n"
						"blendfunc GL_ZERO GL_ONE_MINUS_SRC_ALPHA\n"
						"rgbgen vertex\n"
						"alphagen vertex\n"
						"nodepth\n"
					"}\n"
					"polygonoffset\n"
					"surfaceparm noshadows\n"
					"surfaceparm nodlight\n"
				"}\n"
				;
			break;
		case BM_INVMODC:
			namepostfix = "_invmodc";
			defaultshader =
				"{\n"
					"program defaultsprite\n"
					"nomipmaps\n"
					"sort unlitdecal\n"
					"{\n"
						"map $diffuse\n"
						"blendfunc GL_ZERO GL_ONE_MINUS_SRC_COLOR\n"
						"rgbgen vertex\n"
						"alphagen vertex\n"
						"nodepth\n"
					"}\n"
					"polygonoffset\n"
					"surfaceparm noshadows\n"
					"surfaceparm nodlight\n"
				"}\n"
				;
			break;
		case BM_SUBTRACT:
			namepostfix = "_sub";
			defaultshader =
				"{\n"
					"program defaultsprite\n"
					"nomipmaps\n"
					"sort unlitdecal\n"
					"{\n"
						"map $diffuse\n"
						"blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_COLOR\n"
						"rgbgen vertex\n"
						"alphagen vertex\n"
						"nodepth\n"
					"}\n"
					"polygonoffset\n"
					"surfaceparm noshadows\n"
					"surfaceparm nodlight\n"
				"}\n"
				;
			break;
		case BM_RTSMOKE:
			namepostfix = "_rts";
			defaultshader =
				"{\n"
					"program defaultsprite#LIGHT\n"
					"{\n"
						"map $diffuse\n"
						"blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA\n"
						"rgbgen const vertex\n"
						"alphagen vertex\n"
					"}\n"
					"surfaceparm noshadows\n"
					"sort seethrough\n"	//needs to be low enough that its subject to rtlights
					"bemode rtlight\n"
					"{\n"
						"program rtlight#NOBUMP#VERTEXCOLOURS\n"
						"{\n"
							"map $diffuse\n"
							"blendfunc add\n"
						"}\n"
					"}\n"
				"}\n";
			break;
		}

		memset(&tn, 0, sizeof(tn));
		if (*ptype->texname)
		{
			tn.base = R_LoadHiResTexture(ptype->texname, "particles", IF_LOADNOW | IF_NOMIPMAP|(ptype->looks.nearest?IF_NEAREST:IF_LINEAR)|(ptype->looks.premul?IF_PREMULTIPLYALPHA:0));	//mipmapping breaks particlefont stuff
			if (tn.base && tn.base->status == TEX_LOADING)
				COM_WorkerPartialSync(tn.base, &tn.base->status, TEX_LOADING);
			if (!TEXLOADED(tn.base))
				tn.base = R_LoadHiResTexture(ptype->texname, "particles", IF_CLAMP|IF_NOREPLACE|IF_LOADNOW | IF_NOMIPMAP|(ptype->looks.nearest?IF_NEAREST:IF_LINEAR)|(ptype->looks.premul?IF_PREMULTIPLYALPHA:0));	//mipmapping breaks particlefont stuff
		}
		else
			tn.base = NULL;
		if (!TEXLOADED(tn.base))
		{
			/*okay, so the texture they specified wasn't valid either. use a fully default one*/

			//note that this could get messy if you depend upon vid_restart to reload your effect without re-execing it after.
			ptype->s1 = 0;
			ptype->t1 = 0;
			ptype->s2 = 1;
			ptype->t2 = 1;
			ptype->randsmax = 1;
			if (ptype->looks.type == PT_SPARK)
			{
				extern texid_t r_whiteimage;
				ptype->looks.shader = R_RegisterShader(va("line%s", namepostfix), SUF_NONE, defaultshader);
				TEXASSIGNF(tn.base, r_whiteimage);
			}
			else if (ptype->looks.type == PT_BEAM)
			{
				/*untextured beams get a single continuous blob*/
				ptype->looks.shader = R_RegisterShader(va("beam%s", namepostfix), SUF_NONE, defaultshader);
				TEXASSIGNF(tn.base, beamtexture);
			}
			else if (ptype->looks.type == PT_VBEAM)
			{
				/*untextured beams get a single continuous blob*/
				ptype->looks.shader = R_RegisterShader(va("vbeam%s", namepostfix), SUF_NONE, defaultshader);
				TEXASSIGNF(tn.base, beamtexture);
			}
			else if (ptype->looks.type == PT_SPARKFAN)
			{
				/*untextured beams get a single continuous blob*/
				ptype->looks.shader = R_RegisterShader(va("fan%s", namepostfix), SUF_NONE, defaultshader);
				TEXASSIGNF(tn.base, ptritexture);
			}
			else if (strstr(ptype->texname, "glow") || strstr(ptype->texname, "ball") || ptype->looks.type == PT_TEXTUREDSPARK)
			{
				/*sparks and special names get a nice circular texture.
				as these are fully default, we can basically discard the texture name in the shader, and get better batching*/
				ptype->looks.shader = R_RegisterShader(va("ball%s", namepostfix), SUF_NONE, defaultshader);
				TEXASSIGNF(tn.base, balltexture);
			}
			else
			{
				/*anything else gets a fuzzy texture*/
				ptype->looks.shader = R_RegisterShader(va("default%s", namepostfix), SUF_NONE, defaultshader);
				TEXASSIGNF(tn.base, explosiontexture);
			}
		}
		else
		{
			/*texture looks good, make a shader, and give it the texture as a diffuse stage*/
			ptype->looks.shader = R_RegisterShader(va("%s%s", ptype->texname, namepostfix), SUF_NONE, defaultshader);
		}
		R_BuildDefaultTexnums(&tn, ptype->looks.shader, 0);
	}
	else
		R_BuildDefaultTexnums(NULL, ptype->looks.shader, 0);
}

static void P_ResetToDefaults(part_type_t *ptype)
{
	particle_t *parts;
	char tnamebuf[sizeof(ptype->name)];
	char tconfbuf[sizeof(ptype->config)];

	// go with a lazy clear of list.. mark everything as DEAD and let
	// the beam rendering handle removing nodes
	beamseg_t *beamsegs = ptype->beams;
	while (beamsegs)
	{
		beamsegs->flags |= BS_DEAD;
		beamsegs = beamsegs->next;
	}

	// forget any particles before its wiped
	while (ptype->particles)
	{
		parts = ptype->particles->next;
		ptype->particles->next = free_particles;
		free_particles = ptype->particles;
		ptype->particles = parts;
	}

	// if we're in the runstate loop through and remove from linked list
	if (ptype->state & PS_INRUNLIST)
	{
		*ptype->runlink = ptype->nexttorun;
		if (ptype->nexttorun)
			ptype->nexttorun->runlink = ptype->runlink;
	}

	//some things need to be preserved before we clear everything.
	beamsegs = ptype->beams;
	strcpy(tnamebuf, ptype->name);
	strcpy(tconfbuf, ptype->config);

	//free uneeded info
	if (ptype->ramp)
		BZ_Free(ptype->ramp);
	if (ptype->models)
		BZ_Free(ptype->models);
	if (ptype->sounds)
		BZ_Free(ptype->sounds);

	//reset everything we're too lazy to specifically set
	memset(ptype, 0, sizeof(*ptype));

	//now set any non-0 defaults.

	ptype->beams = beamsegs;
	ptype->rainfrequency = 1;
	strcpy(ptype->name, tnamebuf);
	strcpy(ptype->config, tconfbuf);
	ptype->assoc=P_INVALID;
	ptype->inwater = P_INVALID;
	ptype->cliptype = P_INVALID;
	ptype->emit = P_INVALID;
	ptype->fluidmask = FTECONTENTS_FLUID;
	ptype->alpha = 1;
	ptype->alphachange = 1;
	ptype->clipbounce = 0.8;
	ptype->clipcount = 1;
	ptype->colorindex = -1;
	ptype->rotationstartmin = -M_PI;	//start with a random angle
	ptype->rotationstartrand = M_PI-ptype->rotationstartmin;
	ptype->spawnchance = 1;
	ptype->dl_time = 0;
	ptype->dl_lightstyle = -1;
	VectorSet(ptype->dl_rgb, 1, 1, 1);
	ptype->dl_corona_intensity = 0.25;
	ptype->dl_corona_scale = 0.5;
	VectorSet(ptype->dl_scales, 0, 1, 1);
	ptype->looks.stretch = 0.05;

	ptype->randsmax = 1;
	ptype->s2 = 1;
	ptype->t2 = 1;
}

void Cmd_if_f(void);

//Uses FTE's multiline console stuff.
//This is the function that loads the effect descriptions (via console).
void P_ParticleEffect_f(void)
{
	char *var, *value;
	char *buf;
	qboolean settype = false;
	qboolean setalphadelta = false;
	qboolean setbeamlen = false;

	part_type_t *ptype;
	int pnum, assoc;
	char *config = part_parsenamespace;

	if (Cmd_Argc()!=2)
	{
		if (!strcmp(Cmd_Argv(1), "namespace"))
		{
			Q_strncpyz(part_parsenamespace, Cmd_Argv(2), sizeof(part_parsenamespace));
			if (Cmd_Argc() >= 4)
				part_parseweak = atoi(Cmd_Argv(3));
			return;
		}
		Con_Printf("No name for particle effect\n");
		return;
	}

	buf = Cbuf_GetNext(Cmd_ExecLevel, false);
	while (*buf && *buf <= ' ')
		buf++;	//no whitespace please.
	if (*buf != '{')
	{
		Cbuf_InsertText(buf, Cmd_ExecLevel, true);
		Con_Printf("This is a multiline command and should be used within config files\n");
		return;
	}

	var = Cmd_Argv(1);
	if (!pe_script_enabled)
		ptype = NULL;
	else if (*var == '+')
		ptype = P_GetParticleType(config, var+1);
	else
		ptype = P_GetParticleType(config, var);

	//'weak' configs do not replace 'strong' configs
	//we allow weak to replace weak as a solution to the +assoc chain thing (to add, we effectively need to 'replace').
	if (!pe_script_enabled || (part_parseweak && ptype->loaded==2))
	{
		int depth = 1;
		while(1)
		{
			buf = Cbuf_GetNext(Cmd_ExecLevel, false);
			if (!*buf)
				return;

			while (*buf && *buf <= ' ')
				buf++;	//no whitespace please.
			if (*buf == '{')
				depth++;
			else if (*buf == '}')
			{
				if (--depth == 0)
					break;
			}
		}
		return;
	}

	if (*var == '+')
	{
		if (ptype->loaded)
		{
			int i, parenttype;
			char newname[256];
			for (i = 0; i < 64; i++)
			{
				parenttype = ptype - part_type;
				snprintf(newname, sizeof(newname), "+%i%s", i, ptype->name);
				ptype = P_GetParticleType(config, newname);
				if (!ptype->loaded)
				{
					if (part_type[parenttype].assoc != P_INVALID)
						Con_Printf("warning: assoc on particle chain \"%s.%s\" overridden\n", part_type[parenttype].config, part_type[parenttype].name);
					part_type[parenttype].assoc = ptype - part_type;
					break;
				}
			}
			if (i == 64)
			{
				Con_Printf("Too many duplicate names, gave up\n");
				return;
			}
		}
	}
	else
	{
		if (ptype->loaded)
		{
			assoc = ptype->assoc;
			while (assoc != P_INVALID && assoc < FALLBACKBIAS)
			{
				if (*part_type[assoc].name == '+')
				{
					part_type[assoc].loaded = false;
					assoc = part_type[assoc].assoc;
				}
				else
					break;
			}
		}
	}
	if (!ptype)
	{
		Con_Printf("Bad name\n");
		return;
	}

	P_SetModified();

	pnum = ptype-part_type;

	P_ResetToDefaults(ptype);

	while(1)
	{
		buf = Cbuf_GetNext(Cmd_ExecLevel, false);
		if (!*buf)
		{
			Con_Printf("Unexpected end of buffer with effect %s\n", ptype->name);
			return;
		}

		while (*buf && *buf <= ' ')
			buf++;	//no whitespace please.
		if (*buf == '}')
			break;

		Cmd_TokenizeString(buf, true, true);
		var = Cmd_Argv(0);
		value = Cmd_Argv(1);

		// TODO: switch this mess to some sort of binary tree to increase parse speed
		if (!strcmp(var, "if"))
		{
			//cheesy way to handle if statements inside particle configs.
			Cmd_if_f();
		}
		else if (!strcmp(var, "shader"))
		{
			if (*value)
				Q_strncpyz(ptype->texname, value, sizeof(ptype->texname));
			else
				Q_strncpyz(ptype->texname, ptype->name, sizeof(ptype->texname));
			buf = Cbuf_GetNext(Cmd_ExecLevel, true);
			while (*buf && *buf <= ' ')
				buf++;	//no leading whitespace please.
			if (*buf == '{')
			{
				int nest = 1;
				char *str = BZ_Malloc(3);
				int slen = 2;
				str[0] = '{';
				str[1] = '\n';
				str[2] = 0;
				while(nest)
				{
					buf = Cbuf_GetNext(Cmd_ExecLevel, true);
					if (!*buf)
					{
						Con_Printf("Unexpected end of buffer with effect %s\n", ptype->name);
						break;
					}
					while (*buf && *buf <= ' ')
						buf++;	//no leading whitespace please.
					if (*buf == '}')
						--nest;
					if (*buf == '{')
						nest++;
					str = BZ_Realloc(str, slen + strlen(buf) + 2);
					strcpy(str + slen, buf);
					slen += strlen(str + slen);
					str[slen++] = '\n';
				}
				str[slen] = 0;
				R_RegisterShader(ptype->texname, SUF_NONE, str);
				BZ_Free(str);
			}
			else
				Cbuf_InsertText(buf, Cmd_ExecLevel, true);
		}
		else if (!strcmp(var, "texture") || !strcmp(var, "linear_texture") || !strcmp(var, "nearest_texture") || !strcmp(var, "nearesttexture"))
		{
			Q_strncpyz(ptype->texname, value, sizeof(ptype->texname));
			ptype->looks.nearest = !strncmp(var, "nearest", 7);
		}
		else if (!strcmp(var, "tcoords"))
		{
			float tscale;

			tscale = atof(Cmd_Argv(5));
			if (tscale <= 0)
				tscale = 1;

			ptype->s1 = atof(value)/tscale;
			ptype->t1 = atof(Cmd_Argv(2))/tscale;
			ptype->s2 = atof(Cmd_Argv(3))/tscale;
			ptype->t2 = atof(Cmd_Argv(4))/tscale;

			ptype->randsmax = atoi(Cmd_Argv(6));
			if (Cmd_Argc()>7)
				ptype->texsstride = atof(Cmd_Argv(7));/*FIXME: divide-by-tscale missing */
			else
				ptype->texsstride = 1/tscale;

			if (ptype->randsmax < 1 || ptype->texsstride == 0)
				ptype->randsmax = 1;
		}
		else if (!strcmp(var, "atlas"))
		{	//atlas countineachaxis first [last]
			int dims;
			int i;
			int m;

			dims = atof(Cmd_Argv(1));
			i = atoi(Cmd_Argv(2));
			m = atoi(Cmd_Argv(3));
			if (dims < 1)
				dims = 1;

			if (m > (m/dims)*dims+dims-1)
			{
				m = (m/dims)*dims+dims-1;
				Con_Printf("effect %s wraps across an atlased line\n", ptype->name);
			}
			if (m < i)
				m = i;

			ptype->s1 = 1.0/dims * (i%dims);
			ptype->s2 = 1.0/dims * (1+(i%dims));
			ptype->t1 = 1.0/dims * (i/dims);
			ptype->t2 = 1.0/dims * (1+(i/dims));

			ptype->randsmax = m-i;
			ptype->texsstride = ptype->s2-ptype->s1;

			//its modulo
			ptype->randsmax++;
		}
		else if (!strcmp(var, "rotation"))
		{
			ptype->rotationstartmin = atof(value)*M_PI/180;
			if (Cmd_Argc()>2)
				ptype->rotationstartrand = atof(Cmd_Argv(2))*M_PI/180-ptype->rotationstartmin;
			else
				ptype->rotationstartrand = 0;

			ptype->rotationmin = atof(Cmd_Argv(3))*M_PI/180;
			if (Cmd_Argc()>4)
				ptype->rotationrand = atof(Cmd_Argv(4))*M_PI/180-ptype->rotationmin;
			else
				ptype->rotationrand = 0;
		}
		else if (!strcmp(var, "rotationstart"))
		{
			ptype->rotationstartmin = atof(value)*M_PI/180;
			if (Cmd_Argc()>2)
				ptype->rotationstartrand = atof(Cmd_Argv(2))*M_PI/180-ptype->rotationstartmin;
			else
				ptype->rotationstartrand = 0;
		}
		else if (!strcmp(var, "rotationspeed"))
		{
			ptype->rotationmin = atof(value)*M_PI/180;
			if (Cmd_Argc()>2)
				ptype->rotationrand = atof(Cmd_Argv(2))*M_PI/180-ptype->rotationmin;
			else
				ptype->rotationrand = 0;
		}
		else if (!strcmp(var, "beamtexstep"))
		{
			ptype->rotationstartmin = 1/atof(value);
			ptype->rotationstartrand = 0;
			setbeamlen = true;
		}
		else if (!strcmp(var, "beamtexspeed"))
		{
			ptype->rotationmin = atof(value);
		}
		else if (!strcmp(var, "scale"))
		{
			ptype->scale = atof(value);
			if (Cmd_Argc()>2)
				ptype->scalerand = atof(Cmd_Argv(2)) - ptype->scale;
		}
		else if (!strcmp(var, "scalerand"))
			ptype->scalerand = atof(value);

		else if (!strcmp(var, "scalefactor"))
			ptype->looks.scalefactor = atof(value);
		else if (!strcmp(var, "scaledelta"))
			ptype->scaledelta = atof(value);
		else if (!strcmp(var, "stretchfactor"))	//affects sparks
		{
			ptype->looks.stretch = atof(value);
			ptype->looks.minstretch = (Cmd_Argc()>2)?atof(Cmd_Argv(2)):0;
		}

		else if (!strcmp(var, "step"))
		{
			ptype->countspacing = atof(value);
			ptype->count = 1/atof(value);
			if (Cmd_Argc()>2)
				ptype->countrand = 1/atof(Cmd_Argv(2));
			if (Cmd_Argc()>3)
				ptype->countextra = atof(Cmd_Argv(3));
		}
		else if (!strcmp(var, "count"))
		{
			ptype->countspacing = 0;
			ptype->count = atof(value);
			if (Cmd_Argc()>2)
				ptype->countrand = atof(Cmd_Argv(2));
			if (Cmd_Argc()>3)
				ptype->countextra = atof(Cmd_Argv(3));
		}
		else if (!strcmp(var, "rainfrequency"))
		{	//multiplier to ramp up the effect or whatever (without affecting spawn patterns).
			ptype->rainfrequency = atof(value);
		}

		else if (!strcmp(var, "alpha"))
		{
			ptype->alpha = atof(value);
			if (Cmd_Argc()>2)
				ptype->alpharand = atof(Cmd_Argv(2)) - ptype->alpha;
			if (Cmd_Argc()>3)
			{
				ptype->alphachange = atof(Cmd_Argv(3));
				setalphadelta = true;
			}
		}
		else if (!strcmp(var, "alpharand"))
			ptype->alpharand = atof(value);
#ifdef HAVE_LEGACY
		else if (!strcmp(var, "alphachange"))
		{
			Con_DPrintf("%s.%s: alphachange is deprecated, use alphadelta\n", ptype->config, ptype->name);
			ptype->alphachange = atof(value);
		}
#endif
		else if (!strcmp(var, "alphadelta"))
		{
			ptype->alphachange = atof(value);
			setalphadelta = true;
		}
		else if (!strcmp(var, "die"))
		{
			ptype->die = atof(value);
			if (Cmd_Argc()>2)
			{
				float mn=ptype->die,mx=atof(Cmd_Argv(2));
				if (mn > mx)
				{
					mn = mx;
					mx = ptype->die;
				}
				ptype->die = mx;
				ptype->randdie = mx-mn;
			}
		}
#ifdef HAVE_LEGACY
		else if (!strcmp(var, "diesubrand"))
		{
			Con_DPrintf("%s.%s: diesubrand is deprecated, use die with two arguments\n", ptype->config, ptype->name);
			ptype->randdie = atof(value);
		}
#endif

		else if (!strcmp(var, "randomvel"))
		{	//shortcut for velwrand (and velbias for z bias)
			ptype->velbias[0] = ptype->velbias[1] = 0;
			ptype->velwrand[0] = ptype->velwrand[1] = atof(value);
			if (Cmd_Argc()>3)
			{
				ptype->velbias[2] = atof(Cmd_Argv(2));
				ptype->velwrand[2] = atof(Cmd_Argv(3));
				ptype->velwrand[2] -= ptype->velbias[2]; /*make vert be the total range*/
				ptype->velwrand[2] /= 2; /*vert is actually +/- 1, not 0 to 1, so rescale it*/
				ptype->velbias[2] += ptype->velwrand[2]; /*and bias must be centered to the range*/
			}
			else if (Cmd_Argc()>2)
			{
				ptype->velwrand[2] = atof(Cmd_Argv(2));
				ptype->velbias[2] = 0;
			}
			else
			{
				ptype->velwrand[2] = ptype->velwrand[0];
				ptype->velbias[2] = 0;
			}
		}
		else if (!strcmp(var, "veladd"))
		{
			ptype->veladd = atof(value);
			ptype->randomveladd = 0;
			if (Cmd_Argc()>2)
				ptype->randomveladd = atof(Cmd_Argv(2)) - ptype->veladd;
		}
		else if (!strcmp(var, "orgadd"))
		{
			ptype->orgadd = atof(value);
			ptype->randomorgadd = 0;
			if (Cmd_Argc()>2)
				ptype->randomorgadd = atof(Cmd_Argv(2)) - ptype->orgadd;
		}

		else if (!strcmp(var, "orgbias"))
		{
			ptype->orgbias[0] = atof(value);
			ptype->orgbias[1] = atof(Cmd_Argv(2));
			ptype->orgbias[2] = atof(Cmd_Argv(3));
		}
		else if (!strcmp(var, "velbias"))
		{
			ptype->velbias[0] = atof(value);
			ptype->velbias[1] = atof(Cmd_Argv(2));
			ptype->velbias[2] = atof(Cmd_Argv(3));
		}
		else if (!strcmp(var, "orgwrand"))
		{
			ptype->orgwrand[0] = atof(value);
			ptype->orgwrand[1] = atof(Cmd_Argv(2));
			ptype->orgwrand[2] = atof(Cmd_Argv(3));
		}
		else if (!strcmp(var, "velwrand"))
		{
			ptype->velwrand[0] = atof(value);
			ptype->velwrand[1] = atof(Cmd_Argv(2));
			ptype->velwrand[2] = atof(Cmd_Argv(3));
		}

		else if (!strcmp(var, "friction"))
		{
			ptype->friction[2] = ptype->friction[1] = ptype->friction[0] = atof(value);

			if (Cmd_Argc()>3)
			{
				ptype->friction[2] = atof(Cmd_Argv(3));
				ptype->friction[1] = atof(Cmd_Argv(2));
			}
			else if (Cmd_Argc()>2)
			{
				ptype->friction[2] = atof(Cmd_Argv(2));
			}
		}
		else if (!strcmp(var, "gravity"))
			ptype->gravity = atof(value);
		else if (!strcmp(var, "flurry"))
			ptype->flurry = atof(value);

		else if (!strcmp(var, "assoc"))
		{
			assoc = CheckAssosiation(config, value, pnum);	//careful - this can realloc all the particle types
			ptype = &part_type[pnum];
			ptype->assoc = assoc;
		}
		else if (!strcmp(var, "inwater"))
		{
			// the underwater effect switch should only occur for
			// 1 level so the standard assoc check works
			assoc = CheckAssosiation(config, value, pnum);
			ptype = &part_type[pnum];
			ptype->inwater = assoc;
		}
		else if (!strcmp(var, "underwater"))
		{
			ptype->flags |= PT_TRUNDERWATER;

parsefluid:
			if ((ptype->flags & (PT_TRUNDERWATER|PT_TROVERWATER)) == (PT_TRUNDERWATER|PT_TROVERWATER))
			{
				ptype->flags &= ~PT_TRUNDERWATER;
				Con_Printf("%s.%s: both over and under water\n", ptype->config, ptype->name);
			}
			if (Cmd_Argc() == 1)
				ptype->fluidmask = FTECONTENTS_FLUID;
			else
			{
				int i = Cmd_Argc();
				ptype->fluidmask = 0;
				while (i --> 1)
				{
					char *value = Cmd_Argv(i);
					if (!strcmp(value, "water"))
						ptype->fluidmask |= FTECONTENTS_WATER;
					else if (!strcmp(value, "slime"))
						ptype->fluidmask |= FTECONTENTS_SLIME;
					else if (!strcmp(value, "lava"))
						ptype->fluidmask |= FTECONTENTS_LAVA;
					else if (!strcmp(value, "sky"))
						ptype->fluidmask |= FTECONTENTS_SKY;
					else if (!strcmp(value, "fluid"))
						ptype->fluidmask |= FTECONTENTS_FLUID;
					else if (!strcmp(value, "solid"))
						ptype->fluidmask |= FTECONTENTS_SOLID;
					else if (!strcmp(value, "playerclip"))
						ptype->fluidmask |= FTECONTENTS_PLAYERCLIP;
					else if (!strcmp(value, "none"))
						ptype->fluidmask |= 0;
					else
						Con_Printf("%s.%s: unknown contents: %s\n", ptype->config, ptype->name, value);
				}
			}
		}
		else if (!strcmp(var, "notunderwater"))
		{
			ptype->flags |= PT_TROVERWATER;
			goto parsefluid;
		}
		else if (!strcmp(var, "model"))
		{
			partmodels_t *mod;
			char *e;

			ptype->models = BZ_Realloc(ptype->models, sizeof(partmodels_t)*(ptype->nummodels+1));
			Q_strncpyz(ptype->models[ptype->nummodels].name, Cmd_Argv(1), sizeof(ptype->models[ptype->nummodels].name));
			mod = &ptype->models[ptype->nummodels++];

			mod->framestart = 0;
			mod->framecount = 1;
			mod->framerate = 10;
			mod->alpha = 1;
			mod->skin = 0;
			mod->traileffect = P_INVALID;
			mod->rflags = RF_NOSHADOW;
			mod->scalemin = mod->scalemax = 1;
			mod->rgb[0] = 1.0f;
			mod->rgb[1] = 1.0f;
			mod->rgb[2] = 1.0f;

			strtoul(Cmd_Argv(2), &e, 0);
			while(*e == ' ' || *e == '\t')
				e++;
			if (*e)
			{
				int p;
				for(p = 2; p < Cmd_Argc(); p++)
				{
					e = Cmd_Argv(p);

					if (!Q_strncasecmp(e, "frame=", 6))
					{
						mod->framestart = atof(e+6);
						mod->framecount = 1;
					}
					else if (!Q_strncasecmp(e, "framestart=", 11))
						mod->framestart = atof(e+11);
					else if (!Q_strncasecmp(e, "framecount=", 11))
						mod->framecount = atof(e+11);
					else if (!Q_strncasecmp(e, "frameend=", 9))	//misnomer.
						mod->framecount = atof(e+9);
					else if (!Q_strncasecmp(e, "frames=", 7))
						mod->framecount = atof(e+7);
					else if (!Q_strncasecmp(e, "framerate=", 10))
						mod->framerate = atof(e+10);
					else if (!Q_strncasecmp(e, "skin=", 5))
						mod->skin = atoi(e+5);
					else if (!Q_strncasecmp(e, "alpha=", 6))
						mod->alpha = atof(e+6);
					else if (!Q_strncasecmp(e, "scalemin=", 9))
						mod->scalemin = atof(e+9);
					else if (!Q_strncasecmp(e, "scalemax=", 9))
						mod->scalemax = atof(e+9);
					else if (!Q_strncasecmp(e, "r=", 2))
						mod->rgb[0] = atof(e+2);
					else if (!Q_strncasecmp(e, "g=", 2))
						mod->rgb[1] = atof(e+2);
					else if (!Q_strncasecmp(e, "b=", 2))
						mod->rgb[2] = atof(e+2);
					else if (!Q_strncasecmp(e, "trail=", 6))
					{
						mod->traileffect = P_AllocateParticleType(config, e+6);//careful - this can realloc all the particle types
						ptype = &part_type[pnum];
					}
					else if (!Q_strncasecmp(e, "orient", 6))
						mod->rflags |= RF_USEORIENTATION;	//use the dir to orient the model, instead of always facing up.
					else if (!Q_strncasecmp(e, "additive", 8))
						mod->rflags |= RF_ADDITIVE;			//additive blend
					else if (!Q_strncasecmp(e, "transparent", 11))
						mod->rflags |= RF_TRANSLUCENT;		//force blend
					else if (!Q_strncasecmp(e, "fullbright", 10))
						mod->rflags |= RF_FULLBRIGHT;		//fullbright, woo
					else if (!Q_strncasecmp(e, "shadow", 6))
						mod->rflags &= ~RF_NOSHADOW;		//clear noshadow
					else if (!Q_strncasecmp(e, "noshadow", 8))
						mod->rflags |= RF_NOSHADOW;			//set noshadow (cos... you know...)
					else
						Con_Printf("Bad named argument: %s\n", e);
				}
			}
			else
			{
				mod->framestart = atof(Cmd_Argv(2));
				mod->framecount = atof(Cmd_Argv(3));
				mod->framerate = atof(Cmd_Argv(4));
				mod->alpha = atof(Cmd_Argv(5));
				if (*Cmd_Argv(6))
				{
					mod->traileffect = P_AllocateParticleType(config, Cmd_Argv(6));//careful - this can realloc all the particle types
					ptype = &part_type[pnum];
				}
				else
					mod->traileffect = P_INVALID;
			}
		}
		else if (!strcmp(var, "sound"))
		{
			char *e;
			ptype->sounds = BZ_Realloc(ptype->sounds, sizeof(partsounds_t)*(ptype->numsounds+1));
			Q_strncpyz(ptype->sounds[ptype->numsounds].name, Cmd_Argv(1), sizeof(ptype->sounds[ptype->numsounds].name));
			if (*ptype->sounds[ptype->numsounds].name)
				S_PrecacheSound(ptype->sounds[ptype->numsounds].name);

			ptype->sounds[ptype->numsounds].vol = 1;
			ptype->sounds[ptype->numsounds].atten = 1;
			ptype->sounds[ptype->numsounds].pitch = 1;
			ptype->sounds[ptype->numsounds].delay = 0;
			ptype->sounds[ptype->numsounds].weight = 0;

			strtoul(Cmd_Argv(2), &e, 0);
			while(*e == ' ' || *e == '\t')
				e++;
			if (*e)
			{
				int p;
				for(p = 2; p < Cmd_Argc(); p++)
				{
					e = Cmd_Argv(p);

					if (!Q_strncasecmp(e, "vol=", 4) || !Q_strncasecmp(e, "volume=", 7))
						ptype->sounds[ptype->numsounds].vol = atof(strchr(e, '=')+1);
					else if (!Q_strncasecmp(e, "attn=", 5) || !Q_strncasecmp(e, "atten=", 6) || !Q_strncasecmp(e, "attenuation=", 12))
					{
						e = strchr(e, '=')+1;
						if (!strcmp(e, "none"))
							ptype->sounds[ptype->numsounds].atten = 0;
						else if (!strcmp(e, "normal"))
							ptype->sounds[ptype->numsounds].atten = 1;
						else
							ptype->sounds[ptype->numsounds].atten = atof(e);
					}
					else if (!Q_strncasecmp(e, "pitch=", 6))
						ptype->sounds[ptype->numsounds].pitch = atof(strchr(e, '=')+1)*0.01;
					else if (!Q_strncasecmp(e, "delay=", 6))
						ptype->sounds[ptype->numsounds].delay = atof(strchr(e, '=')+1);
					else if (!Q_strncasecmp(e, "weight=", 7))
						ptype->sounds[ptype->numsounds].weight = atof(strchr(e, '=')+1);
					else
						Con_Printf("Bad named argument: %s\n", e);
				}
			}
			else
			{
				ptype->sounds[ptype->numsounds].vol = atof(Cmd_Argv(2));
				if (!ptype->sounds[ptype->numsounds].vol)
					ptype->sounds[ptype->numsounds].vol = 1;
				ptype->sounds[ptype->numsounds].atten = atof(Cmd_Argv(3));
				if (!ptype->sounds[ptype->numsounds].atten)
					ptype->sounds[ptype->numsounds].atten = 1;
				ptype->sounds[ptype->numsounds].pitch = atof(Cmd_Argv(4))*0.01;
				if (!ptype->sounds[ptype->numsounds].pitch)
					ptype->sounds[ptype->numsounds].pitch = 1;
				ptype->sounds[ptype->numsounds].delay = atof(Cmd_Argv(5));
				if (!ptype->sounds[ptype->numsounds].delay)
					ptype->sounds[ptype->numsounds].delay = 0;
				ptype->sounds[ptype->numsounds].weight = atof(Cmd_Argv(6));
			}
			if (!ptype->sounds[ptype->numsounds].weight)
				ptype->sounds[ptype->numsounds].weight = 1;
			ptype->numsounds++;
		}
		else if (!strcmp(var, "colorindex"))
		{
			if (Cmd_Argc()>2)
				ptype->colorrand = strtoul(Cmd_Argv(2), NULL, 0);
			ptype->colorindex = strtoul(value, NULL, 0);
		}
		else if (!strcmp(var, "colorrand"))
			ptype->colorrand = atoi(value); // now obsolete
		else if (!strcmp(var, "citracer"))
			ptype->flags |= PT_CITRACER;

		else if (!strcmp(var, "red"))
			ptype->rgb[0] = atof(value)/255;
		else if (!strcmp(var, "green"))
			ptype->rgb[1] = atof(value)/255;
		else if (!strcmp(var, "blue"))
			ptype->rgb[2] = atof(value)/255;
		else if (!strcmp(var, "rgb"))
		{	//byte version
			ptype->rgb[0] = ptype->rgb[1] = ptype->rgb[2] = atof(value)/255;
			if (Cmd_Argc()>3)
			{
				ptype->rgb[1] = atof(Cmd_Argv(2))/255;
				ptype->rgb[2] = atof(Cmd_Argv(3))/255;
			}
		}
		else if (!strcmp(var, "rgbf"))
		{	//float version
			ptype->rgb[0] = ptype->rgb[1] = ptype->rgb[2] = atof(value);
			if (Cmd_Argc()>3)
			{
				ptype->rgb[1] = atof(Cmd_Argv(2));
				ptype->rgb[2] = atof(Cmd_Argv(3));
			}
		}

		else if (!strcmp(var, "reddelta"))
		{
			ptype->rgbchange[0] = atof(value)/255;
			if (!ptype->rgbchangetime)
				ptype->rgbchangetime = ptype->die;
		}
		else if (!strcmp(var, "greendelta"))
		{
			ptype->rgbchange[1] = atof(value)/255;
			if (!ptype->rgbchangetime)
				ptype->rgbchangetime = ptype->die;
		}
		else if (!strcmp(var, "bluedelta"))
		{
			ptype->rgbchange[2] = atof(value)/255;
			if (!ptype->rgbchangetime)
				ptype->rgbchangetime = ptype->die;
		}
		else if (!strcmp(var, "rgbdelta"))
		{	//byte version
			ptype->rgbchange[0] = ptype->rgbchange[1] = ptype->rgbchange[2] = atof(value)/255;
			if (Cmd_Argc()>3)
			{
				ptype->rgbchange[1] = atof(Cmd_Argv(2))/255;
				ptype->rgbchange[2] = atof(Cmd_Argv(3))/255;
			}
			if (!ptype->rgbchangetime)
				ptype->rgbchangetime = ptype->die;
		}
		else if (!strcmp(var, "rgbdeltaf"))
		{	//float version
			ptype->rgbchange[0] = ptype->rgbchange[1] = ptype->rgbchange[2] = atof(value);
			if (Cmd_Argc()>3)
			{
				ptype->rgbchange[1] = atof(Cmd_Argv(2));
				ptype->rgbchange[2] = atof(Cmd_Argv(3));
			}
			if (!ptype->rgbchangetime)
				ptype->rgbchangetime = ptype->die;
		}
		else if (!strcmp(var, "rgbdeltatime"))
			ptype->rgbchangetime = atof(value);

		else if (!strcmp(var, "redrand"))
			ptype->rgbrand[0] = atof(value)/255;
		else if (!strcmp(var, "greenrand"))
			ptype->rgbrand[1] = atof(value)/255;
		else if (!strcmp(var, "bluerand"))
			ptype->rgbrand[2] = atof(value)/255;
		else if (!strcmp(var, "rgbrand"))
		{	//byte version
			ptype->rgbrand[0] = ptype->rgbrand[1] = ptype->rgbrand[2] = atof(value)/255;
			if (Cmd_Argc()>3)
			{
				ptype->rgbrand[1] = atof(Cmd_Argv(2))/255;
				ptype->rgbrand[2] = atof(Cmd_Argv(3))/255;
			}
		}
		else if (!strcmp(var, "rgbrandf"))
		{	//float version
			ptype->rgbrand[0] = ptype->rgbrand[1] = ptype->rgbrand[2] = atof(value);
			if (Cmd_Argc()>3)
			{
				ptype->rgbrand[1] = atof(Cmd_Argv(2));
				ptype->rgbrand[2] = atof(Cmd_Argv(3));
			}
		}

		else if (!strcmp(var, "rgbrandsync"))
		{
			ptype->rgbrandsync[0] = ptype->rgbrandsync[1] = ptype->rgbrandsync[2] = atof(value);
			if (Cmd_Argc()>3)
			{
				ptype->rgbrandsync[1] = atof(Cmd_Argv(2));
				ptype->rgbrandsync[2] = atof(Cmd_Argv(3));
			}
		}
		else if (!strcmp(var, "redrandsync"))
			ptype->rgbrandsync[0] = atof(value);
		else if (!strcmp(var, "greenrandsync"))
			ptype->rgbrandsync[1] = atof(value);
		else if (!strcmp(var, "bluerandsync"))
			ptype->rgbrandsync[2] = atof(value);

		else if (!strcmp(var, "stains"))
			ptype->stainonimpact = atof(value);
		else if (!strcmp(var, "blend"))
		{
			ptype->looks.premul = false;
			if (!strcmp(value, "adda") || !strcmp(value, "add"))
				ptype->looks.blendmode = BM_ADDA;
			else if (!strcmp(value, "addc"))
				ptype->looks.blendmode = BM_ADDC;
			else if (!strcmp(value, "subtract"))
				ptype->looks.blendmode = BM_SUBTRACT;
			else if (!strcmp(value, "invmoda") || !strcmp(value, "invmod"))
				ptype->looks.blendmode = BM_INVMODA;
			else if (!strcmp(value, "invmodc"))
				ptype->looks.blendmode = BM_INVMODC;
			else if (!strcmp(value, "blendcolour") || !strcmp(value, "blendcolor"))
				ptype->looks.blendmode = BM_BLENDCOLOUR;
			else if (!strcmp(value, "blendalpha"))
				ptype->looks.blendmode = BM_BLEND;
			else if (!strcmp(value, "premul_subtract"))
			{
				ptype->looks.premul = 1;
				ptype->looks.blendmode = BM_INVMODC;
			}
			else if (!strcmp(value, "premul_add"))
			{
				ptype->looks.premul = 2;
				ptype->looks.blendmode = BM_PREMUL;
			}
			else if (!strcmp(value, "premul_blend"))
			{
				ptype->looks.premul = 1;
				ptype->looks.blendmode = BM_PREMUL;
			}
			else if (!strcmp(value, "rtsmoke"))
			{
				ptype->looks.premul = 1;
				ptype->looks.blendmode = BM_RTSMOKE;
			}
			else
			{
				Con_DPrintf("%s.%s: uses unknown blend type '%s', assuming legacy 'blendalpha'\n", ptype->config, ptype->name, value);
				ptype->looks.blendmode = BM_BLEND;	//fallback
			}
		}
		else if (!strcmp(var, "placeholder"))
			ptype->spawnmode = SM_FIXMEWARNING;
		else if (!strcmp(var, "spawnmode"))
		{
			if (!strcmp(value, "circle"))
				ptype->spawnmode = SM_CIRCLE;
			else if (!strcmp(value, "ball"))
				ptype->spawnmode = SM_BALL;
			else if (!strcmp(value, "spiral"))
				ptype->spawnmode = SM_SPIRAL;
			else if (!strcmp(value, "tracer"))
				ptype->spawnmode = SM_TRACER;
			else if (!strcmp(value, "telebox"))
				ptype->spawnmode = SM_TELEBOX;
			else if (!strcmp(value, "lavasplash"))
				ptype->spawnmode = SM_LAVASPLASH;
			else if (!strcmp(value, "uniformcircle"))
				ptype->spawnmode = SM_UNICIRCLE;
			else if (!strcmp(value, "syncfield"))
			{
				ptype->spawnmode = SM_FIELD;
#ifdef HAVE_LEGACY
				ptype->spawnparam1 = 16;
				ptype->spawnparam2 = 0;
#endif
			}
			else if (!strcmp(value, "distball"))
				ptype->spawnmode = SM_DISTBALL;
			else if (!strcmp(value, "box"))
				ptype->spawnmode = SM_BOX;
			else
			{
				Con_DPrintf("%s.%s: uses unknown spawn type '%s', assuming 'box'\n", ptype->config, ptype->name, value);
				ptype->spawnmode = SM_BOX;	//fallback
			}

			if (Cmd_Argc()>2)
			{
				if (Cmd_Argc()>3)
					ptype->spawnparam2 = atof(Cmd_Argv(3));
				ptype->spawnparam1 = atof(Cmd_Argv(2));
			}
		}
		else if (!strcmp(var, "type"))
		{
			if (!strcmp(value, "beam"))
				ptype->looks.type = PT_BEAM;
			else if (!strcmp(value, "vbeam"))
				ptype->looks.type = PT_BEAM;
			else if (!strcmp(value, "spark") || !strcmp(value, "linespark"))
				ptype->looks.type = PT_SPARK;
			else if (!strcmp(value, "sparkfan") || !strcmp(value, "trianglefan"))
				ptype->looks.type = PT_SPARKFAN;
			else if (!strcmp(value, "texturedspark"))
				ptype->looks.type = PT_TEXTUREDSPARK;
			else if (!strcmp(value, "decal") || !strcmp(value, "cdecal"))
				ptype->looks.type = PT_CDECAL;
			else if (!strcmp(value, "udecal"))
				ptype->looks.type = PT_UDECAL;
			else if (!strcmp(value, "normal"))
				ptype->looks.type = PT_NORMAL;
			else
			{
				Con_DPrintf("%s.%s: uses unknown (render) type '%s', assuming 'normal'\n", ptype->config, ptype->name, value);
				ptype->looks.type = PT_NORMAL;	//fallback
			}
			settype = true;
		}
		else if (!strcmp(var, "clippeddecal"))	//mask, match
		{
			if (Cmd_Argc()>=2)
			{//decal only appears where: (surfflags&mask)==match
				ptype->surfflagmatch = ptype->surfflagmask = strtoul(Cmd_Argv(1), NULL, 0);
				if (Cmd_Argc()>=3)
					ptype->surfflagmatch = strtoul(Cmd_Argv(2), NULL, 0);
			}
			ptype->looks.type = PT_CDECAL;
			settype = true;
		}
#ifdef HAVE_LEGACY
		else if (!strcmp(var, "isbeam"))
		{
			Con_DPrintf("%s.%s: isbeam is deprecated, use type beam\n", ptype->config, ptype->name);
			ptype->looks.type = PT_BEAM;
			settype = true;
		}
#endif
		else if (!strcmp(var, "spawntime"))
			ptype->spawntime = atof(value);
		else if (!strcmp(var, "spawnchance"))
			ptype->spawnchance = atof(value);
		else if (!strcmp(var, "cliptype"))
		{
			assoc = P_AllocateParticleType(config, value);//careful - this can realloc all the particle types
			ptype = &part_type[pnum];
			ptype->cliptype = assoc;
		}
		else if (!strcmp(var, "clipcount"))
			ptype->clipcount = atof(value);
		else if (!strcmp(var, "clipbounce"))
		{
			ptype->clipbounce = atof(value);
			if (ptype->clipbounce < 0 && ptype->cliptype == P_INVALID)
				ptype->cliptype = pnum;
		}
		else if (!strcmp(var, "bounce"))
		{
			ptype->cliptype = pnum;
			ptype->clipbounce = atof(value);
		}

		else if (!strcmp(var, "emit"))
		{
			assoc = P_AllocateParticleType(config, value);//careful - this can realloc all the particle types
			ptype = &part_type[pnum];
			ptype->emit = assoc;
		}
		else if (!strcmp(var, "emitinterval"))
			ptype->emittime = atof(value);
		else if (!strcmp(var, "emitintervalrand"))
			ptype->emitrand = atof(value);
		else if (!strcmp(var, "emitstart"))
			ptype->emitstart = atof(value);

#ifdef HAVE_LEGACY
		// old names
		else if (!strcmp(var, "areaspread"))
		{
			Con_DPrintf("%s.%s: areaspread is deprecated, use spawnorg\n", ptype->config, ptype->name);
			ptype->areaspread = atof(value);
		}
		else if (!strcmp(var, "areaspreadvert"))
		{
			Con_DPrintf("%s.%s: areaspreadvert is deprecated, use spawnorg\n", ptype->config, ptype->name);
			ptype->areaspreadvert = atof(value);
		}
		else if (!strcmp(var, "offsetspread"))
		{
			Con_DPrintf("%s.%s: offsetspread is deprecated, use spawnvel\n", ptype->config, ptype->name);
			ptype->spawnvel = atof(value);
		}
		else if (!strcmp(var, "offsetspreadvert"))
		{
			Con_DPrintf("%s.%s: offsetspreadvert is deprecated, use spawnvel\n", ptype->config, ptype->name);
			ptype->spawnvelvert  = atof(value);
		}
#endif

		// current names
		else if (!strcmp(var, "spawnorg"))
		{
			ptype->areaspreadvert = ptype->areaspread = atof(value);

			if (Cmd_Argc()>2)
				ptype->areaspreadvert = atof(Cmd_Argv(2));
		}
		else if (!strcmp(var, "spawnvel"))
		{
			ptype->spawnvelvert = ptype->spawnvel = atof(value);

			if (Cmd_Argc()>2)
				ptype->spawnvelvert = atof(Cmd_Argv(2));
		}

#ifdef HAVE_LEGACY
		// spawn mode param fields
		else if (!strcmp(var, "spawnparam1"))
		{
			ptype->spawnparam1 = atof(value);
			Con_DPrintf("%s.%s: 'spawnparam1' is deprecated, use 'spawnmode foo X'\n", ptype->config, ptype->name);
		}
		else if (!strcmp(var, "spawnparam2"))
		{
			ptype->spawnparam2 = atof(value);
			Con_DPrintf("%s.%s: 'spawnparam2' is deprecated, use 'spawnmode foo X Y'\n", ptype->config, ptype->name);
		}
/*		else if (!strcmp(var, "spawnparam3"))
			ptype->spawnparam3 = atof(value); */
		else if (!strcmp(var, "up"))
			ptype->orgbias[2] = atof(value);
#endif

		else if (!strcmp(var, "rampmode"))
		{
			if (!strcmp(value, "none"))
				ptype->rampmode = RAMP_NONE;
#ifdef HAVE_LEGACY
			else if (!strcmp(value, "absolute"))
			{
				Con_DPrintf("%s.%s: 'rampmode absolute' is deprecated, use 'rampmode nearest'\n", ptype->config, ptype->name);
				ptype->rampmode = RAMP_NEAREST;
			}
#endif
			else if (!strcmp(value, "nearest"))
				ptype->rampmode = RAMP_NEAREST;
			else if (!strcmp(value, "lerp"))	//don't use the name 'linear'. ramps are there to avoid linear...
				ptype->rampmode = RAMP_LERP;
			else if (!strcmp(value, "delta"))
				ptype->rampmode = RAMP_DELTA;
			else
			{
				Con_DPrintf("%s.%s: uses unknown ramp mode '%s', assuming 'delta'\n", ptype->config, ptype->name, value);
				ptype->rampmode = RAMP_DELTA;
			}
		}
		else if (!strcmp(var, "rampindexlist"))
		{ // better not use this with delta ramps...
			int cidx, i;

			i = 1;
			while (i < Cmd_Argc())
			{
				ptype->ramp = BZ_Realloc(ptype->ramp, sizeof(ramp_t)*(ptype->rampindexes+1));

				cidx = atoi(Cmd_Argv(i));
				ptype->ramp[ptype->rampindexes].alpha = cidx > 255 ? 0.5 : 1;

				cidx = (cidx & 0xff) * 3;
				ptype->ramp[ptype->rampindexes].rgb[0] = host_basepal[cidx] * (1/255.0);
				ptype->ramp[ptype->rampindexes].rgb[1] = host_basepal[cidx+1] * (1/255.0);
				ptype->ramp[ptype->rampindexes].rgb[2] = host_basepal[cidx+2] * (1/255.0);

				ptype->ramp[ptype->rampindexes].scale = ptype->scale;

				ptype->rampindexes++;
				i++;
			}
		}
		else if (!strcmp(var, "rampindex"))
		{
			int cidx;
			ptype->ramp = BZ_Realloc(ptype->ramp, sizeof(ramp_t)*(ptype->rampindexes+1));

			cidx = atoi(value);
			ptype->ramp[ptype->rampindexes].alpha = cidx > 255 ? 0.5 : 1;

			if (Cmd_Argc() > 2) // they gave alpha
				ptype->ramp[ptype->rampindexes].alpha *= atof(Cmd_Argv(2));

			cidx = (cidx & 0xff) * 3;
			ptype->ramp[ptype->rampindexes].rgb[0] = host_basepal[cidx] * (1/255.0);
			ptype->ramp[ptype->rampindexes].rgb[1] = host_basepal[cidx+1] * (1/255.0);
			ptype->ramp[ptype->rampindexes].rgb[2] = host_basepal[cidx+2] * (1/255.0);

			if (Cmd_Argc() > 3) // they gave scale
				ptype->ramp[ptype->rampindexes].scale = atof(Cmd_Argv(3));
			else
				ptype->ramp[ptype->rampindexes].scale = ptype->scale;


			ptype->rampindexes++;
		}
		else if (!strcmp(var, "ramp"))
		{
			ptype->ramp = BZ_Realloc(ptype->ramp, sizeof(ramp_t)*(ptype->rampindexes+1));

			ptype->ramp[ptype->rampindexes].rgb[0] = atof(value)/255;
			if (Cmd_Argc()>3)	//seperate rgb
			{
				ptype->ramp[ptype->rampindexes].rgb[1] = atof(Cmd_Argv(2))/255;
				ptype->ramp[ptype->rampindexes].rgb[2] = atof(Cmd_Argv(3))/255;

				if (Cmd_Argc()>4)	//have we alpha and scale changes?
				{
					ptype->ramp[ptype->rampindexes].alpha = atof(Cmd_Argv(4));
					if (Cmd_Argc()>5)	//have we scale changes?
						ptype->ramp[ptype->rampindexes].scale = atof(Cmd_Argv(5));
					else
						ptype->ramp[ptype->rampindexes].scale = ptype->scaledelta;
				}
				else
				{
					ptype->ramp[ptype->rampindexes].alpha = ptype->alpha;
					ptype->ramp[ptype->rampindexes].scale = ptype->scaledelta;
				}
			}
			else	//they only gave one value
			{
				ptype->ramp[ptype->rampindexes].rgb[1] = ptype->ramp[ptype->rampindexes].rgb[0];
				ptype->ramp[ptype->rampindexes].rgb[2] = ptype->ramp[ptype->rampindexes].rgb[0];

				ptype->ramp[ptype->rampindexes].alpha = ptype->alpha;
				ptype->ramp[ptype->rampindexes].scale = ptype->scaledelta;
			}

			ptype->rampindexes++;
		}
		else if (!strcmp(var, "viewspace"))
			ptype->viewspacefrac = (Cmd_Argc()>1)?atof(value):1;
		else if (!strcmp(var, "perframe"))
			ptype->flags |= PT_INVFRAMETIME;
		else if (!strcmp(var, "averageout"))
			ptype->flags |= PT_AVERAGETRAIL;
		else if (!strcmp(var, "nostate"))
			ptype->flags |= PT_NOSTATE;
		else if (!strcmp(var, "nospreadfirst"))
			ptype->flags |= PT_NOSPREADFIRST;
		else if (!strcmp(var, "nospreadlast"))
			ptype->flags |= PT_NOSPREADLAST;

		else if (!strcmp(var, "lightradius"))
		{	//float version
			ptype->dl_radius[0] = ptype->dl_radius[1] = atof(value);
			if (Cmd_Argc()>2)
				ptype->dl_radius[1] = atof(Cmd_Argv(2));
			ptype->dl_radius[1] -= ptype->dl_radius[0];
		}
		else if (!strcmp(var, "lightradiusfade"))
			ptype->dl_decay[3] = atof(value);
		else if (!strcmp(var, "lightrgb"))
		{
			ptype->dl_rgb[0] = atof(value);
			ptype->dl_rgb[1] = atof(Cmd_Argv(2));
			ptype->dl_rgb[2] = atof(Cmd_Argv(3));
		}
		else if (!strcmp(var, "lightrgbfade"))
		{
			ptype->dl_decay[0] = atof(value);
			ptype->dl_decay[1] = atof(Cmd_Argv(2));
			ptype->dl_decay[2] = atof(Cmd_Argv(3));
		}
		else if (!strcmp(var, "lightcorona"))
		{
			ptype->dl_corona_intensity = atof(value);
			ptype->dl_corona_scale = atof(Cmd_Argv(2));
		}
		else if (!strcmp(var, "lighttime"))
			ptype->dl_time = atof(value);
		else if (!strcmp(var, "lightshadows"))
			ptype->flags = (ptype->flags & ~PT_NODLSHADOW) | (atof(value)?0:PT_NODLSHADOW);
		else if (!strcmp(var, "lightcubemap"))
			ptype->dl_cubemapnum = atoi(value);
		else if (!strcmp(var, "lightstyle"))
			ptype->dl_lightstyle = atoi(value);
		else if (!strcmp(var, "lightscales"))
		{	//ambient diffuse specular
			ptype->dl_scales[0] = atof(value);
			ptype->dl_scales[1] = atof(Cmd_Argv(2));
			ptype->dl_scales[2] = atof(Cmd_Argv(3));
		}
		else if (!strcmp(var, "spawnstain"))
		{
			ptype->stain_radius = atof(value);
			ptype->stain_rgb[0] = atof(Cmd_Argv(2));
			ptype->stain_rgb[1] = atof(Cmd_Argv(3));
			ptype->stain_rgb[2] = atof(Cmd_Argv(4));
		}
		else if (Cmd_Argc())
			Con_DPrintf("%s.%s: %s is not a recognised particle type field\n", ptype->config, ptype->name, var);
	}
	ptype->loaded = part_parseweak?1:2;
	if (ptype->clipcount < 1)
		ptype->clipcount = 1;

	if (!settype)
	{
		if (ptype->looks.type == PT_NORMAL && !*ptype->texname)
		{
			if (ptype->scale)
			{
				ptype->looks.type = PT_SPARKFAN;
				if (ptype->countextra || ptype->count || ptype->countrand)
					Con_DPrintf("%s.%s: effect lacks a texture. assuming type sparkfan.\n", ptype->config, ptype->name);
			}
			else
			{
				ptype->looks.type = PT_SPARK;
				if (ptype->countextra || ptype->count || ptype->countrand)
					Con_DPrintf("%s.%s: effect lacks a texture. assuming type spark.\n", ptype->config, ptype->name);
			}
		}
		else if (ptype->looks.type == PT_SPARK)
		{
			if (*ptype->texname)
				ptype->looks.type = PT_TEXTUREDSPARK;
			else if (ptype->scale)
				ptype->looks.type = PT_SPARKFAN;
		}
	}

	// use old behavior if not using alphadelta
	if (!setalphadelta)
		ptype->alphachange = (-ptype->alphachange / ptype->die) * ptype->alpha;

	FinishParticleType(ptype);

	if ((ptype->looks.type == PT_BEAM||ptype->looks.type == PT_VBEAM) && !setbeamlen)
		ptype->rotationstartmin = 1/128.0;
}

qboolean PScript_Query(int typenum, int body, char *outstr, int outstrlen)
{
#ifndef QUAKETC
	int i;
	part_type_t *ptype = &part_type[typenum];
	if (typenum < 0 || typenum >= numparticletypes)
		return false;

	if (body == 0)
	{
		Q_snprintfz(outstr, outstrlen, "%s.%s", ptype->config, ptype->name);
		return true;
	}

	if (body == 1 || body == 2)
	{
		qboolean all = body==2;
		*outstr = 0;
		if (!ptype->loaded)
			return true;

//		Q_strncatz(outstr, va("//this functionality is incomplete\n"), outstrlen);

		switch (ptype->looks.type)
		{
		default:
		case PT_NORMAL:
			Q_strncatz(outstr, "type normal\n", outstrlen);
			break;
		case PT_SPARK:
			Q_strncatz(outstr, "type linespark\n", outstrlen);
			break;
		case PT_SPARKFAN:
			Q_strncatz(outstr, "type sparkfan\n", outstrlen);
			break;
		case PT_TEXTUREDSPARK:
			Q_strncatz(outstr, "type texturedspark\n", outstrlen);
			break;
		case PT_BEAM:
			Q_strncatz(outstr, "type beam\n", outstrlen);
			break;
		case PT_VBEAM:
			Q_strncatz(outstr, "type vbeam\n", outstrlen);
			break;
		case PT_CDECAL:
			if (ptype->surfflagmatch || ptype->surfflagmask)
				Q_strncatz(outstr, va("clippeddecal %#x %#x\n", ptype->surfflagmask, ptype->surfflagmatch), outstrlen);
			else
				Q_strncatz(outstr, "type cdecal\n", outstrlen);
			break;
		case PT_UDECAL:
			Q_strncatz(outstr, "type udecal\n", outstrlen);
			break;
		}
		switch (ptype->looks.blendmode)
		{
		case BM_BLEND:
			Q_strncatz(outstr, "blend blendalpha\n", outstrlen);
			break;
		case BM_BLENDCOLOUR:
			Q_strncatz(outstr, "blend blendcolour\n", outstrlen);
			break;
		case BM_ADDA:
			Q_strncatz(outstr, "blend adda\n", outstrlen);
			break;
		case BM_ADDC:
			Q_strncatz(outstr, "blend addc\n", outstrlen);
			break;
		case BM_SUBTRACT:
			Q_strncatz(outstr, "blend subtract\n", outstrlen);
			break;
		case BM_INVMODA:
			Q_strncatz(outstr, "blend invmoda\n", outstrlen);
			break;
		case BM_INVMODC:
			if (ptype->looks.premul)
				Q_strncatz(outstr, "blend premul_subtract\n", outstrlen);
			else
				Q_strncatz(outstr, "blend invmodc\n", outstrlen);
			break;
		case BM_PREMUL:
			if (ptype->looks.premul == 2)
				Q_strncatz(outstr, "blend premul_add\n", outstrlen);
			else
				Q_strncatz(outstr, "blend premul_blend\n", outstrlen);
			break;
		case BM_RTSMOKE:
			Q_strncatz(outstr, "blend rtsmoke\n", outstrlen);
			break;
		}

		for (i = 0; i < ptype->nummodels; i++)
		{
			Q_strncatz(outstr, va("model \"%s\" framestart=%g frames=%g framerate=%g alpha=%g skin=%i",
				ptype->models[i].name, ptype->models[i].framestart, ptype->models[i].framecount, ptype->models[i].framerate, ptype->models[i].alpha, ptype->models[i].skin
				), outstrlen);
			if (ptype->models[i].traileffect!=P_INVALID)
				Q_strncatz(outstr, va(" trail=%s", part_type[ptype->models[i].traileffect].name), outstrlen);
			if (ptype->models[i].scalemin != 1 || ptype->models[i].scalemax != 1)
				Q_strncatz(outstr, va(" scalemin=%g scalemax=%g", ptype->models[i].scalemin, ptype->models[i].scalemax), outstrlen);
			if (ptype->models[i].rflags&RF_USEORIENTATION)
				Q_strncatz(outstr, " orient", outstrlen);
			if (ptype->models[i].rflags&RF_ADDITIVE)
				Q_strncatz(outstr, " additive", outstrlen);
			if (ptype->models[i].rflags&RF_TRANSLUCENT)
				Q_strncatz(outstr, " transparent", outstrlen);
			if (ptype->models[i].rflags&RF_FULLBRIGHT)
				Q_strncatz(outstr, " fullbright", outstrlen);
			if (ptype->models[i].rflags&RF_NOSHADOW)
				Q_strncatz(outstr, " noshadow", outstrlen);
			else
				Q_strncatz(outstr, " shadow", outstrlen);
			Q_strncatz(outstr, "\n", outstrlen);
		}
		for (i = 0; i < ptype->numsounds; i++)
		{
			Q_strncatz(outstr, va("sound \"%s\" %g %g %g %g %g\n", ptype->sounds[i].name, ptype->sounds[i].vol, ptype->sounds[i].atten, ptype->sounds[i].pitch, ptype->sounds[i].delay, ptype->sounds[i].weight), outstrlen);
		}

		if (*ptype->texname || all)
		{	//note: particles don't really know if the shader was embedded or not. the shader system handles all that.
			//this means that you'll really need to use external shaders for this to work.
			Q_strncatz(outstr, va("%stexture \"%s\"\n", ptype->looks.nearest?"nearest_":"", ptype->texname), outstrlen);
			Q_strncatz(outstr, va("tcoords %g %g %g %g %g %i %g\n", ptype->s1, ptype->t1, ptype->s2, ptype->t2, 1.0f, ptype->randsmax, ptype->texsstride), outstrlen);
		}

		if (ptype->count || ptype->countrand || ptype->countextra || all)
			Q_strncatz(outstr, va("count %g %g %g\n", ptype->count, ptype->countrand, ptype->countextra), outstrlen);
		if (ptype->rainfrequency != 1 || all)
			Q_strncatz(outstr, va("rainfrequency %g\n", ptype->rainfrequency), outstrlen);

		if (ptype->rgb[0] || ptype->rgb[1] || ptype->rgb[2] || all)
			Q_strncatz(outstr, va("rgbf %g %g %g\n", ptype->rgb[0], ptype->rgb[1], ptype->rgb[2]), outstrlen);
		if (ptype->rgbrand[0] || ptype->rgbrand[1] || ptype->rgbrand[2] || all)
			Q_strncatz(outstr, va("rgbrandf %g %g %g\n", ptype->rgbrand[0], ptype->rgbrand[1], ptype->rgbrand[2]), outstrlen);
		if (ptype->rgbrandsync[0] || ptype->rgbrandsync[1] || ptype->rgbrandsync[2] || all)
			Q_strncatz(outstr, va("rgbrandsync %g %g %g\n", ptype->rgbrandsync[0], ptype->rgbrandsync[1], ptype->rgbrandsync[2]), outstrlen);
		if (ptype->rgbchange[0] || ptype->rgbchange[1] || ptype->rgbchange[2] || all)
			Q_strncatz(outstr, va("rgbdeltaf %g %g %g\n", ptype->rgbchange[0], ptype->rgbchange[1], ptype->rgbchange[2]), outstrlen);
		if (ptype->rgbchangetime || all)
			Q_strncatz(outstr, va("rgbchangetime %g\n", ptype->rgbchangetime), outstrlen);

		if (ptype->colorindex != -1 || all)
			Q_strncatz(outstr, va("colorindex %i\n", ptype->colorindex), outstrlen);
		if (ptype->colorrand || all)
			Q_strncatz(outstr, va("colorrand %i\n", ptype->colorrand), outstrlen);

//		if (ptype->alpha || all)
			Q_strncatz(outstr, va("alpha %g\n", ptype->alpha), outstrlen);
		if (ptype->alpharand || all)
			Q_strncatz(outstr, va("alpharand %g\n", ptype->alpharand), outstrlen);
//		if (ptype->alphachange || all)
			Q_strncatz(outstr, va("alphadelta %g\n", ptype->alphachange), outstrlen);

		if (ptype->scale || ptype->scalerand || all)
			Q_strncatz(outstr, va("scale %g %g\n", ptype->scale, ptype->scale+ptype->scalerand), outstrlen);
//		if (ptype->looks.scalefactor || all)	//always write scalefactor, to avoid issues with defaults.
			Q_strncatz(outstr, va("scalefactor %g\n", ptype->looks.scalefactor), outstrlen);
		if (ptype->scaledelta || all)
			Q_strncatz(outstr, va("scaledelta %g\n", ptype->scaledelta), outstrlen);
		if (ptype->looks.stretch!=0.05 || ptype->looks.minstretch || all)
		{
			if (ptype->looks.type != PT_TEXTUREDSPARK)
				Q_strncatz(outstr, "//", outstrlen);
			Q_strncatz(outstr, va("stretchfactor %g %g\n", ptype->looks.stretch, ptype->looks.minstretch), outstrlen);
		}

		if (ptype->die || ptype->randdie || all)
			Q_strncatz(outstr, va("die %g %g\n", ptype->die, ptype->die+ptype->randdie), outstrlen);

		if (ptype->cliptype != P_INVALID && ptype->cliptype != typenum)
			Q_strncatz(outstr, va("cliptype \"%s.%s\"\n", part_type[ptype->cliptype].config, part_type[ptype->cliptype].name), outstrlen);
		if (ptype->clipbounce != 0.8)
			Q_strncatz(outstr, va("clipbounce %g\n", ptype->clipbounce), outstrlen);
		if (ptype->clipcount > 1)
			Q_strncatz(outstr, va("clipcount %g\n", ptype->clipcount), outstrlen);

		if (ptype->spawnmode != SM_BOX || ptype->spawnparam1 || ptype->spawnparam2 || all)
		switch(ptype->spawnmode)
		{
		case SM_CIRCLE:
			Q_strncatz(outstr, va("spawnmode circle %g %g\n", ptype->spawnparam1, ptype->spawnparam2), outstrlen);
			break;
		case SM_BALL:
			Q_strncatz(outstr, va("spawnmode ball %g %g\n", ptype->spawnparam1, ptype->spawnparam2), outstrlen);
			break;
		case SM_SPIRAL:
			Q_strncatz(outstr, va("spawnmode spiral %g %g\n", ptype->spawnparam1, ptype->spawnparam2), outstrlen);
			break;
		case SM_TRACER:
			Q_strncatz(outstr, va("spawnmode tracer %g %g\n", ptype->spawnparam1, ptype->spawnparam2), outstrlen);
			break;
		case SM_TELEBOX:
			Q_strncatz(outstr, va("spawnmode telebox %g %g\n", ptype->spawnparam1, ptype->spawnparam2), outstrlen);
			break;
		case SM_LAVASPLASH:
			Q_strncatz(outstr, va("spawnmode lavasplash %g %g\n", ptype->spawnparam1, ptype->spawnparam2), outstrlen);
			break;
		case SM_UNICIRCLE:
			Q_strncatz(outstr, va("spawnmode uniformcircle %g %g\n", ptype->spawnparam1, ptype->spawnparam2), outstrlen);
			break;
		case SM_FIELD:
			Q_strncatz(outstr, va("spawnmode syncfield %g %g\n", ptype->spawnparam1, ptype->spawnparam2), outstrlen);
			break;
		case SM_DISTBALL:
			Q_strncatz(outstr, va("spawnmode distball %g %g\n", ptype->spawnparam1, ptype->spawnparam2), outstrlen);
			break;
		case SM_BOX:
			Q_strncatz(outstr, va("spawnmode box %g %g\n", ptype->spawnparam1, ptype->spawnparam2), outstrlen);
			break;
		case SM_MESHSURFACE:
			Q_strncatz(outstr, va("spawnmode meshsurface\n"), outstrlen);
			break;
		case SM_FIXMEWARNING:
			Q_strncatz(outstr, va("placeholder\n"), outstrlen);
			break;
		}
		if (ptype->spawnvel || ptype->spawnvelvert || all)
			Q_strncatz(outstr, va("spawnvel %g %g\n", ptype->spawnvel, ptype->spawnvelvert), outstrlen);
		if (ptype->areaspread || ptype->areaspreadvert || all)
			Q_strncatz(outstr, va("spawnorg %g %g\n", ptype->areaspread, ptype->areaspreadvert), outstrlen);

		if (ptype->viewspacefrac || all)
			Q_strncatz(outstr, ((ptype->viewspacefrac==1)?"viewspace\n":va("viewspace %g\n", ptype->viewspacefrac)), outstrlen);

		if (ptype->veladd || ptype->randomveladd || all)
		{
			if (ptype->randomveladd)
				Q_strncatz(outstr, va("veladd %g %g\n", ptype->veladd, ptype->veladd+ptype->randomveladd), outstrlen);
			else
				Q_strncatz(outstr, va("veladd %g\n", ptype->veladd), outstrlen);
		}
		if (ptype->orgadd || ptype->randomorgadd || all)
		{
			if (ptype->randomorgadd)
				Q_strncatz(outstr, va("orgadd %g %g\n", ptype->orgadd, ptype->orgadd+ptype->randomorgadd), outstrlen);
			else
				Q_strncatz(outstr, va("orgadd %g\n", ptype->orgadd), outstrlen);
		}

		if (ptype->gravity || all)
			Q_strncatz(outstr, va("gravity %g\n", ptype->gravity), outstrlen);
		if (ptype->flurry)
			Q_strncatz(outstr, va("flurry %g\n", ptype->flurry), outstrlen);

		//these are more for dp-compat than anything else.
		if (DotProduct(ptype->orgbias,ptype->orgbias) || all)
			Q_strncatz(outstr, va("orgbias %g %g %g\n", ptype->orgbias[0], ptype->orgbias[1], ptype->orgbias[2]), outstrlen);
		if (DotProduct(ptype->orgwrand,ptype->orgwrand) || all)
			Q_strncatz(outstr, va("orgwrand %g %g %g\n", ptype->orgwrand[0], ptype->orgwrand[1], ptype->orgwrand[2]), outstrlen);
		if (ptype->velbias[0] == 0 && ptype->velbias[1] == 0 && ptype->velwrand[0] == ptype->velwrand[1] && !all)
			Q_strncatz(outstr, va("randomvel %g %g %g\n", ptype->velwrand[0], ptype->velbias[2] - ptype->velwrand[2], ptype->velbias[2]*2-(ptype->velbias[2]- ptype->velwrand[2])), outstrlen);
		else
		{
			if (DotProduct(ptype->velbias,ptype->velbias) || all)
				Q_strncatz(outstr, va("velbias %g %g %g\n", ptype->velbias[0], ptype->velbias[1], ptype->velbias[2]), outstrlen);
			if (DotProduct(ptype->velwrand,ptype->velwrand) || all)
				Q_strncatz(outstr, va("velwrand %g %g %g\n", ptype->velwrand[0], ptype->velwrand[1], ptype->velwrand[2]), outstrlen);
		}

		if (ptype->assoc != P_INVALID)
			Q_strncatz(outstr, va("assoc \"%s\"\n", part_type[ptype->assoc].name), outstrlen);

		if (ptype->rotationstartmin != -M_PI || ptype->rotationstartrand != 2*M_PI || all)
			Q_strncatz(outstr, va("rotationstart %g %g\n", ptype->rotationstartmin*180/M_PI, (ptype->rotationstartmin+ptype->rotationstartrand)*180/M_PI), outstrlen);
		if (ptype->rotationmin || ptype->rotationrand || all)
			Q_strncatz(outstr, va("rotationspeed %g %g\n", ptype->rotationmin*180/M_PI, (ptype->rotationmin+ptype->rotationrand)*180/M_PI), outstrlen);

		if (ptype->flags & (PT_TROVERWATER | PT_TRUNDERWATER))
		{
			if (ptype->flags & PT_TRUNDERWATER)
				Q_strncatz(outstr, "underwater", outstrlen);
			else
				Q_strncatz(outstr, "notunderwater", outstrlen);
			if (ptype->fluidmask == 0)
				Q_strncatz(outstr, " none", outstrlen);
			else if (ptype->fluidmask != FTECONTENTS_FLUID)
			{
				if ((ptype->fluidmask & FTECONTENTS_FLUID) == FTECONTENTS_FLUID)
					Q_strncatz(outstr, " fluid", outstrlen);
				else
				{
					if (ptype->fluidmask & FTECONTENTS_WATER)
						Q_strncatz(outstr, " water", outstrlen);
					if (ptype->fluidmask & FTECONTENTS_SLIME)
						Q_strncatz(outstr, " slime", outstrlen);
					if (ptype->fluidmask & FTECONTENTS_LAVA)
						Q_strncatz(outstr, " lava", outstrlen);
					if (ptype->fluidmask & FTECONTENTS_SKY)
						Q_strncatz(outstr, " sky", outstrlen);
				}
				if (ptype->fluidmask & FTECONTENTS_SOLID)
					Q_strncatz(outstr, " solid", outstrlen);
				if (ptype->fluidmask & FTECONTENTS_PLAYERCLIP)
					Q_strncatz(outstr, " playerclip", outstrlen);
			}
			Q_strncatz(outstr, "\n", outstrlen);
		}

		if (ptype->dl_radius[0] || ptype->dl_radius[1] || all)
		{
			Q_strncatz(outstr, va("lightradius %g %g\n", ptype->dl_radius[0], ptype->dl_radius[0]+ptype->dl_radius[1]), outstrlen);
			Q_strncatz(outstr, va("lightradiusfade %g\n", ptype->dl_decay[3]), outstrlen);
			Q_strncatz(outstr, va("lightrgb %g %g %g\n", ptype->dl_rgb[0], ptype->dl_rgb[1], ptype->dl_rgb[2]), outstrlen);
			Q_strncatz(outstr, va("lightrgbfade %g %g %g\n", ptype->dl_decay[0], ptype->dl_decay[1], ptype->dl_decay[2]), outstrlen);
			Q_strncatz(outstr, va("lighttime %g\n", ptype->dl_time), outstrlen);
			Q_strncatz(outstr, va("lightshadows %g\n", (ptype->flags & PT_NODLSHADOW)?0.0f:1.0f), outstrlen);
			Q_strncatz(outstr, va("lightcubemap %i\n", ptype->dl_cubemapnum), outstrlen);
			Q_strncatz(outstr, va("lightstyle %i\n", ptype->dl_lightstyle), outstrlen);
			Q_strncatz(outstr, va("lightcorona %g %g\n", ptype->dl_corona_intensity, ptype->dl_corona_scale), outstrlen);
			Q_strncatz(outstr, va("lightscales %g %g %g\n", ptype->dl_scales[0], ptype->dl_scales[1], ptype->dl_scales[2]), outstrlen);
		}
		if (ptype->stain_radius || all)
			Q_strncatz(outstr, va("spawnstain %g %g %g %g\n", ptype->stain_radius, ptype->stain_rgb[0], ptype->stain_rgb[1], ptype->stain_rgb[2]), outstrlen);
		if (ptype->stainonimpact || all)
			Q_strncatz(outstr, va("stains %g\n", ptype->stainonimpact), outstrlen);

		return true;
	}
#endif
	return false;
}

#ifdef HAVE_LEGACY
static void P_ExportAllEffects_f(void)
{
	char effect[8192];
	int i, assoc, n;
	vfsfile_t *outf;
	char fname[64] = "particles/export.cfg";
	FS_CreatePath("particles/", FS_GAMEONLY);
	outf = FS_OpenVFS(fname, "wb", FS_GAMEONLY);
	if (!outf)
	{
		FS_DisplayPath(fname, FS_GAMEONLY, effect, sizeof(effect));
		Con_TPrintf("Unable to open file %s\n", effect);
		return;
	}
	for (i = 0; i < numparticletypes; i++)
	{
		PScript_Query(i, 0, effect, sizeof(effect));
		if (strchr(effect, '+'))
			continue;
		assoc = i;
		while(assoc != P_INVALID)
		{
			n = part_type[assoc].assoc;
			part_type[assoc].assoc = P_INVALID;
			VFS_PUTS(outf, "r_part ");
			VFS_PUTS(outf, effect);
			VFS_PUTS(outf, "\n{\n");
			PScript_Query(assoc, 1, effect, sizeof(effect));
			VFS_PUTS(outf, effect);
			VFS_PUTS(outf, "}\n");
			part_type[assoc].assoc = n;
			assoc = n;

			Q_snprintfz(effect, sizeof(effect), "%s.+%s", part_type[i].config, part_type[i].name);
		}
	}
	VFS_CLOSE(outf);

	FS_DisplayPath(fname, FS_GAMEONLY, effect, sizeof(effect));
	Con_Printf("Written %s\n", effect);
}
#endif

#if 1//_DEBUG
// R_BeamInfo_f - debug junk
static void P_BeamInfo_f (void)
{
	beamseg_t *bs;
	int i, j, k, l, m;

	i = 0;

	for (bs = free_beams; bs; bs = bs->next)
		i++;

	Con_Printf("%i free beams\n", i);

	for (i = 0; i < numparticletypes; i++)
	{
		m = l = k = j = 0;
		for (bs = part_type[i].beams; bs; bs = bs->next)
		{
			if (!bs->p)
				k++;

			if (bs->flags & BS_DEAD)
				l++;

			if (bs->flags & BS_LASTSEG)
				m++;

			j++;
		}

		if (j)
			Con_Printf("Type %i = %i NULL p, %i DEAD, %i LASTSEG, %i total\n", i, k, l, m, j);
	}
}

static void P_PartInfo_f (void)
{
	particle_t *p;
	clippeddecal_t *d;
	part_type_t *ptype;
	int totalp = 0, totald = 0, freep, freed, runningp=0, runningd=0, runninge=0, runningt=0;

	int i, j, k;

	Con_DPrintf("Full list of  effects:\n");
	for (i = 0; i < numparticletypes; i++)
	{
		j = 0;
		for (p = part_type[i].particles; p; p = p->next)
			j++;
		totalp += j;

		k = 0;
		for (d = part_type[i].clippeddecals; d; d = d->next)
			k++;
		totald += k;

		if (j||k)
		{
			Con_DPrintf("Type %s.%s = %i+%i total\n", part_type[i].config, part_type[i].name, j,k);
			if (!(part_type[i].state & PS_INRUNLIST))
				Con_Printf(CON_WARNING "%s.%s NOT RUNNING\n", part_type[i].config, part_type[i].name);
		}
	}

	Con_Printf("Running effects:\n");
	// maintain run list
	for (ptype = part_run_list; ptype; ptype = ptype->nexttorun)
	{
		Con_Printf("Type %s.%s", ptype->config, ptype->name);

		j = 0;
		for (p = ptype->particles; p; p = p->next)
			j++;
		if (j)
		{
			Con_Printf("\t%i particles", j);
			if (ptype->cliptype >= 0 || ptype->stainonimpact)
			{
				Con_Printf("(+traceline)");
				runningt += j;
			}
		}
		runningp += j;

		k = 0;
		for (d = ptype->clippeddecals; d; d = d->next)
			k++;
		if (k)
			Con_Printf("%s%i decals", ptype->particles?", ":"\t", k);
		runningd += k;

		Con_Printf("\n");
		runninge++;
	}
	Con_Printf("End of list\n");

	for (p = free_particles, freep = 0; p; p = p->next)
		freep++;
	for (d = free_decals, freed = 0; d; d = d->next)
		freed++;

	Con_DPrintf("%i running effects.\n", runninge);
	Con_Printf("%i particles, %i free, %i traces.\n", runningp, freep, runningt);
	Con_Printf("%i decals, %i free.\n", runningd, freed);

	if (totalp != runningp)
		Con_Printf("%i particles unaccounted for\n", totalp - runningp);
	if (totald != runningd)
		Con_Printf("%i decals unaccounted for\n", totald - runningd);
}
#endif

static void FinishParticleType(part_type_t *ptype)
{
	//if there is a chance that it moves
	if (ptype->gravity || ptype->veladd || ptype->spawnvel || ptype->spawnvelvert || DotProduct(ptype->velwrand,ptype->velwrand) || DotProduct(ptype->velbias,ptype->velbias) || ptype->flurry)
		ptype->flags |= PT_VELOCITY;
	if (DotProduct(ptype->velbias,ptype->velbias) || DotProduct(ptype->velwrand,ptype->velwrand) || DotProduct(ptype->orgwrand,ptype->orgwrand))
		ptype->flags |= PT_WORLDSPACERAND;
	//if it has friction
	if (ptype->friction[0] || ptype->friction[1] || ptype->friction[2])
		ptype->flags |= PT_FRICTION;

	P_LoadTexture(ptype, true);
	if (ptype->dl_decay[3] && !ptype->dl_time)
		ptype->dl_time = ptype->dl_radius[0] / ptype->dl_decay[3];
	if (ptype->looks.scalefactor > 1 && !ptype->looks.invscalefactor)
	{
		ptype->scale *= ptype->looks.scalefactor;
		ptype->scalerand *= ptype->looks.scalefactor;
		/*too lazy to go through ramps*/
		ptype->looks.scalefactor = 1;
	}
	ptype->looks.invscalefactor = 1-ptype->looks.scalefactor;

	if (ptype->looks.type == PT_TEXTUREDSPARK && !ptype->looks.stretch)
		ptype->looks.stretch = 0.05;	//the old default.

	if (ptype->looks.type == PT_SPARK && r_part_sparks.ival<0)
		ptype->looks.type = PT_INVISIBLE;
	if (ptype->looks.type == PT_TEXTUREDSPARK && !r_part_sparks_textured.ival)
		ptype->looks.type = PT_SPARK;
	if (ptype->looks.type == PT_SPARKFAN && !r_part_sparks_trifan.ival)
		ptype->looks.type = PT_SPARK;
	if (ptype->looks.type == PT_SPARK && !r_part_sparks.ival)
		ptype->looks.type = PT_INVISIBLE;
	if ((ptype->looks.type == PT_BEAM||ptype->looks.type == PT_VBEAM) && r_part_beams.ival <= 0)
		ptype->looks.type = PT_INVISIBLE;
	
	if (ptype->rampmode && !ptype->ramp)
	{
		ptype->rampmode = RAMP_NONE;
		Con_Printf("%s.%s: Particle has a ramp mode but no ramp\n", ptype->config, ptype->name);
	}
	else if (ptype->ramp && !ptype->rampmode)
	{
		Con_Printf("%s.%s: Particle has a ramp but no ramp mode\n", ptype->config, ptype->name);
	}
	r_plooksdirty = true;
}

#ifdef HAVE_LEGACY
static void FinishEffectinfoParticleType(part_type_t *ptype, qboolean blooddecalonimpact)
{
	if (ptype->looks.type == PT_CDECAL)
	{
		if (ptype->die == 9999)
			ptype->die = 20;
		ptype->alphachange = -(ptype->alpha / ptype->die);
	}
	else if (ptype->looks.type == PT_UDECAL)
	{
		//dp's decals have a size as a radius. fte's udecals are 'just' quads.
		//also, dp uses 'stretch'.
		ptype->looks.stretch *= 1/1.414213562373095;
		ptype->scale *= ptype->looks.stretch;
		ptype->scalerand *= ptype->looks.stretch;
		ptype->scaledelta *= ptype->looks.stretch;
		ptype->looks.stretch = 1;
	}
	else if (ptype->looks.type == PT_NORMAL)
	{
		//fte's textured particles are *0.25 for some reason.
		//but fte also uses radiuses, while dp uses total size so we only need to double it here..
		ptype->looks.stretch = 1;//affects billboarding in dp
		ptype->scale *= 2*ptype->looks.stretch;
		ptype->scalerand *= 2*ptype->looks.stretch;
		ptype->scaledelta *= 2*2*ptype->looks.stretch;	//fixme: this feels wrong, the results look correct though. hrmph.
		ptype->looks.stretch = 1;
	}
	else if (ptype->looks.type == PT_BEAM || ptype->looks.type == PT_VBEAM)
	{	//ignore what the particle says and just spawn it from a to b. don't accept mid-segment randomness.
		if (ptype->t1 == 0 && ptype->t2 == 1)
			ptype->looks.type = PT_VBEAM;
		else
			ptype->looks.type = PT_BEAM;
		ptype->count = 0;
		ptype->countextra = 2;
		ptype->countrand = 0;
		ptype->countspacing = 10000;
		ptype->scale *= 0.5;
	}

	if (blooddecalonimpact)	//DP blood particles generate decals unconditionally (and prevent blood from bouncing)
		ptype->clipbounce = -2;
	if (ptype->looks.type == PT_TEXTUREDSPARK)
	{
		ptype->scale *= 2;
		ptype->scalerand *= 2;
		ptype->scaledelta *= 2;
		ptype->looks.stretch *= 0.04;
//		if (ptype->looks.stretch < 0)
//			ptype->looks.stretch = 0.000001;
	}
	
	if (ptype->die == 9999)	//internal: means unspecified.
	{
		if (ptype->alphachange)
			ptype->die = (ptype->alpha+ptype->alpharand)/-ptype->alphachange;
		else
			ptype->die = 15;
	}
	ptype->looks.minstretch = 0.5;
	FinishParticleType(ptype);
}
static void P_ImportEffectInfo(char *config, char *line)
{
	float printtimer = 0;
	part_type_t *ptype = NULL;
	int parenttype;
	char arg[8][1024];
	int args;
	qboolean blooddecalonimpact = false;	//tracked separately because it needs to override another field


	struct
	{
		float tc[4];
		char imgname[128];
	} teximages[256];

	{
		int i;
		vfsfile_t *file;
		char *line, linebuf[1024];
		//default assumes 8*8 grid, but we allow more
		for (i = 0; i < 256; i++)
		{
			teximages[i].tc[0] = 1/8.0 * (i & 7);
			teximages[i].tc[1] = 1/8.0 * (1+(i & 7));
			teximages[i].tc[2] = 1/8.0 * (1+(i>>3));
			teximages[i].tc[3] = 1/8.0 * (i>>3);
			strcpy(teximages[i].imgname, "particles/particlefont");
		}

		//and this one needs to be subject to a hack. ho hum.
		teximages[60].tc[0] = 0;
		teximages[60].tc[1] = 1;
		teximages[60].tc[2] = 0;
		teximages[60].tc[3] = 1;
		strcpy(teximages[60].imgname, "particles/nexbeam");

		file = FS_OpenVFS("particles/particlefont.txt", "rb", FS_GAME);
		if (file)
		{
			while (VFS_GETS(file, linebuf, sizeof(linebuf)))
			{
				args = 0;
				line = COM_StringParse(linebuf, arg[args], sizeof(arg[args]), false, false);
				if (line)
				{
					for (args++; args < countof(arg); args++)
					{
						line = COM_StringParse(line, arg[args], sizeof(arg[args]), false, false);
						if (!line)
							break;
					}
				}

				i = atoi(arg[0]);
				if (i >= countof(teximages))
					Con_Printf("particles/particlefont.txt: index too high - %i>=%u\n", i, (unsigned)countof(teximages));
				else if (args == 2)
				{
					teximages[i].tc[0] = 0;
					teximages[i].tc[1] = 1;
					teximages[i].tc[2] = 0;
					teximages[i].tc[3] = 1;
					Q_strncpyz(teximages[i].imgname, arg[1], sizeof(teximages[i].imgname));
				}
				else if (args >= 5 && args <= 6)
				{
					teximages[i].tc[0] = atof(arg[1]);	//s1
					teximages[i].tc[1] = atof(arg[3]);	//s2
					teximages[i].tc[2] = atof(arg[4]);	//t1
					teximages[i].tc[3] = atof(arg[2]);	//t2
					Q_strncpyz(teximages[i].imgname, (args>=6)?arg[5]:"particles/particlefont", sizeof(teximages[i].imgname));
				}
				else
					Con_Printf("particles/particlefont.txt: unsupported argument count - %i\n", args);
			}
			VFS_CLOSE(file);
		}
	}

	for (args = 0;;)
	{
		if (!*line)
			break;
		if (args == 8)
		{
			Con_Printf("Too many args!\n");
			args--;
		}
		line = COM_StringParse(line, com_token, sizeof(com_token), false, false);
		if (!line)
			break;
		Q_strncpyz(arg[args], com_token, sizeof(arg[args]));
		args++;
		if (*com_token == '\n')
			args--;
		else if (*line)
			continue;

		if (args <= 0)
			continue;

		if (!strcmp(arg[0], "effect"))
		{
			char newname[64];
			int i;

			if (ptype)
				FinishEffectinfoParticleType(ptype, blooddecalonimpact);
			blooddecalonimpact = false;

			ptype = P_GetParticleType(config, arg[1]);
			if (ptype->loaded)
			{
				for (i = 0; i < 64; i++)
				{
					parenttype = ptype - part_type;
					Q_snprintfz(newname, sizeof(newname), "%i+%s", i, arg[1]);
					ptype = P_GetParticleType(config, newname);
					if (!ptype->loaded)
					{
						part_type[parenttype].assoc = ptype - part_type;
						break;
					}
				}
				if (i == 64)
				{
					Con_Printf("Too many duplicate names, gave up\n");
					break;
				}
			}
			P_ResetToDefaults(ptype);
			ptype->loaded = part_parseweak?1:2;
			ptype->scale = 1;
			ptype->alpha = 0;
			ptype->alpharand = 1;
			ptype->alphachange = -1;
			ptype->die = 9999;
			ptype->rgb[0] = 1;
			ptype->rgb[1] = 1;
			ptype->rgb[2] = 1;

//			ptype->spawnmode = SM_BALL;

			ptype->colorindex = -1;
			ptype->spawnchance = 1;
			ptype->looks.scalefactor = 2;
			ptype->looks.invscalefactor = 0;
			ptype->looks.type = PT_NORMAL;
			ptype->looks.blendmode = BM_PREMUL;
			ptype->looks.premul = 1;
			ptype->looks.stretch = 1;

			ptype->dl_time = 0;

			i = 63; //default texture is 63.
			Q_strncpyz(ptype->texname, teximages[i].imgname, sizeof(ptype->texname));
			ptype->s1 = teximages[i].tc[0];
			ptype->s2 = teximages[i].tc[1];
			ptype->t1 = teximages[i].tc[2];
			ptype->t2 = teximages[i].tc[3];
			ptype->texsstride = 0;
			ptype->randsmax = 1;
		}
		else if (!ptype)
		{
			Con_Printf("Bad effectinfo file\n");
			break;
		}
		else if (!strcmp(arg[0], "countabsolute") && args == 2)
			ptype->countextra = atof(arg[1]);
		else if (!strcmp(arg[0], "count") && args == 2)
			ptype->count = atof(arg[1]);
		else if (!strcmp(arg[0], "type") && args == 2)
		{
			if (!strcmp(arg[1], "decal") || !strcmp(arg[1], "cdecal"))
			{
				ptype->looks.type = PT_CDECAL;
				ptype->looks.blendmode = BM_INVMODC;
				ptype->looks.premul = 2;
			}
			else if (!strcmp(arg[1], "udecal"))
			{
				ptype->looks.type = PT_UDECAL;
				ptype->looks.blendmode = BM_INVMODC;
				ptype->looks.premul = 2;
			}
			else if (!strcmp(arg[1], "alphastatic"))
			{
				ptype->looks.type = PT_NORMAL;
				ptype->looks.blendmode = BM_PREMUL;//BM_BLEND;
				ptype->looks.premul = 1;
			}
			else if (!strcmp(arg[1], "static"))
			{
				ptype->looks.type = PT_NORMAL;
				ptype->looks.blendmode = BM_PREMUL;//BM_ADDA;
				ptype->looks.premul = 2;
			}
			else if (!strcmp(arg[1], "smoke"))
			{
				ptype->looks.type = PT_NORMAL;
				ptype->looks.blendmode = BM_PREMUL;//BM_ADDA;
				ptype->looks.premul = 2;
			}
			else if (!strcmp(arg[1], "spark"))
			{
				ptype->looks.type = PT_TEXTUREDSPARK;
				ptype->looks.blendmode = BM_PREMUL;//BM_ADDA;
				ptype->looks.premul = 2;
			}
			else if (!strcmp(arg[1], "bubble"))
			{
				ptype->looks.type = PT_NORMAL;
				ptype->looks.blendmode = BM_PREMUL;//BM_ADDA;
				ptype->looks.premul = 2;
			}
			else if (!strcmp(arg[1], "blood"))
			{
				ptype->looks.type = PT_NORMAL;
				ptype->looks.blendmode = BM_INVMODC;
				ptype->looks.premul = 2;
				ptype->gravity = 800*1;
				blooddecalonimpact = true;
			}
			else if (!strcmp(arg[1], "beam"))
			{
				ptype->looks.type = PT_BEAM;
				ptype->looks.blendmode = BM_PREMUL;//BM_ADDA;
				ptype->looks.premul = 2;
			}
			else if (!strcmp(arg[1], "snow"))
			{
				ptype->looks.type = PT_NORMAL;
				ptype->looks.blendmode = BM_PREMUL;//BM_ADDA;
				ptype->looks.premul = 2;
				ptype->flurry = 32;	//may not still be valid later, but at least it would be an obvious issue with the original.
			}
			else if (!strcmp(arg[1], "entityparticle"))
			{
				ptype->die = 0;
				ptype->looks.type = PT_NORMAL;
				ptype->looks.blendmode = BM_PREMUL;//BM_BLEND;
				ptype->looks.premul = 1;
			}
			else
			{
				Con_Printf("effectinfo type %s not supported\n", arg[1]);
			}
		}
		else if (!strcmp(arg[0], "tex") && args == 3)
		{	//assume that all textures will be packed into the same texture and on the same line...
			int mini = atoi(arg[1]);
			int maxi = atoi(arg[2]);
			Q_strncpyz(ptype->texname, teximages[mini].imgname, sizeof(ptype->texname));
			ptype->s1 = teximages[mini].tc[0];
			ptype->s2 = teximages[mini].tc[1];
			ptype->t1 = teximages[mini].tc[2];
			ptype->t2 = teximages[mini].tc[3];
			ptype->texsstride = teximages[(mini+1)&(sizeof(teximages)/sizeof(teximages[0])-1)].tc[0] - teximages[mini].tc[0];
			ptype->randsmax = (maxi - mini);
			if (ptype->randsmax < 1)
				ptype->randsmax = 1;
		}
		else if (!strcmp(arg[0], "size") && args == 3)
		{
			float s1 = atof(arg[1]), s2 = atof(arg[2]);
			ptype->scale = s1;
			ptype->scalerand = (s2-s1);
		}
		else if (!strcmp(arg[0], "sizeincrease") && args == 2)
			ptype->scaledelta = atof(arg[1]);
		else if (!strcmp(arg[0], "color") && args == 3)
		{
			unsigned int rgb1 = strtoul(arg[1], NULL, 0), rgb2 = strtoul(arg[2], NULL, 0);
			int i;
			for (i = 0; i < 3; i++)
			{
				ptype->rgb[i] = ((rgb1>>(16-i*8)) & 0xff)/255.0;
				ptype->rgbrand[i] = (int)(((rgb2>>(16-i*8)) & 0xff) - ((rgb1>>(16-i*8)) & 0xff))/255.0;
				ptype->rgbrandsync[i] = 1;
			}
		}
		else if (!strcmp(arg[0], "alpha") && args == 4)
		{
			float a1 = atof(arg[1]), a2 = atof(arg[2]), f = atof(arg[3]);
			if (a1 > a2)
			{	//backwards
				ptype->alpha = a2/256;
				ptype->alpharand = (a1-a2)/256;
			}
			else
			{
				ptype->alpha = a1/256;
				ptype->alpharand = (a2-a1)/256;
			}
			ptype->alphachange = -f/256;
		}
		else if (!strcmp(arg[0], "velocityoffset") && args == 4)
		{	/*a 3d world-coord addition*/
			ptype->velbias[0] = atof(arg[1]);
			ptype->velbias[1] = atof(arg[2]);
			ptype->velbias[2] = atof(arg[3]);
		}
		else if (!strcmp(arg[0], "velocityjitter") && args == 4)
		{
			ptype->velwrand[0] = atof(arg[1]);
			ptype->velwrand[1] = atof(arg[2]);
			ptype->velwrand[2] = atof(arg[3]);
		}
		else if (!strcmp(arg[0], "originoffset") && args == 4)
		{	/*a 3d world-coord addition*/
			ptype->orgbias[0] = atof(arg[1]);
			ptype->orgbias[1] = atof(arg[2]);
			ptype->orgbias[2] = atof(arg[3]);
		}
		else if (!strcmp(arg[0], "originjitter") && args == 4)
		{
			ptype->orgwrand[0] = atof(arg[1]);
			ptype->orgwrand[1] = atof(arg[2]);
			ptype->orgwrand[2] = atof(arg[3]);
		}
		else if (!strcmp(arg[0], "gravity") && args == 2)
		{
			ptype->gravity = 800*atof(arg[1]);
		}
		else if (!strcmp(arg[0], "bounce") && args == 2)
		{
			ptype->clipbounce = atof(arg[1]);
			if (ptype->clipbounce < 0)
				ptype->cliptype = ptype - part_type;
		}
		else if (!strcmp(arg[0], "airfriction") && args == 2)
			ptype->friction[2] = ptype->friction[1] = ptype->friction[0] = atof(arg[1]);
		else if (!strcmp(arg[0], "liquidfriction") && args == 2)
			;
		else if (!strcmp(arg[0], "underwater") && args == 1)
			ptype->flags |= PT_TRUNDERWATER;
		else if (!strcmp(arg[0], "notunderwater") && args == 1)
			ptype->flags |= PT_TROVERWATER;
		else if (!strcmp(arg[0], "velocitymultiplier") && args == 2)
			ptype->veladd = atof(arg[1]);
		else if (!strcmp(arg[0], "trailspacing") && args == 2)
		{
			ptype->countspacing = atof(arg[1]);
			ptype->count = 1 / ptype->countspacing;
		}
		else if (!strcmp(arg[0], "time") && args == 3)
		{
			ptype->die = atof(arg[1]);
			ptype->randdie = atof(arg[2]) - ptype->die;
			if (ptype->randdie < 0)
			{
				ptype->die = atof(arg[2]);
				ptype->randdie = atof(arg[1]) - ptype->die;
			}
		}
		else if (!strcmp(arg[0], "stretchfactor") && args == 2)
			ptype->looks.stretch = atof(arg[1]);
		else if (!strcmp(arg[0], "blend") && args == 2)
		{
			if (!strcmp(arg[1], "invmod"))
			{
				ptype->looks.blendmode = BM_INVMODC;
				ptype->looks.premul = 2;
			}
			else if (!strcmp(arg[1], "alpha"))
			{
				ptype->looks.blendmode = BM_PREMUL;
				ptype->looks.premul = 1;
			}
			else if (!strcmp(arg[1], "add"))
			{
				ptype->looks.blendmode = BM_PREMUL;
				ptype->looks.premul = 2;
			}
			else
				Con_Printf("effectinfo 'blend %s' not supported\n", arg[1]);
		}
		else if (!strcmp(arg[0], "orientation") && args == 2)
		{
			if (!strcmp(arg[1], "billboard"))
				ptype->looks.type = PT_NORMAL;
			else if (!strcmp(arg[1], "spark"))
				ptype->looks.type = PT_TEXTUREDSPARK;
			else if (!strcmp(arg[1], "oriented"))	//FIXME: not sure this points the right way. also, its double-sided in dp.
			{
				if (ptype->looks.type != PT_CDECAL)
					ptype->looks.type = PT_UDECAL;
			}
			else if (!strcmp(arg[1], "beam"))
				ptype->looks.type = PT_BEAM;
			else
				Con_Printf("effectinfo 'orientation %s' not supported\n", arg[1]);
		}
		else if (!strcmp(arg[0], "lightradius") && args == 2)
		{
			ptype->dl_radius[0] = atof(arg[1]);
			ptype->dl_radius[1] = 0;
		}
		else if (!strcmp(arg[0], "lightradiusfade") && args == 2)
			ptype->dl_decay[3] = atof(arg[1]);
		else if (!strcmp(arg[0], "lightcolor") && args == 4)
		{
			ptype->dl_rgb[0] = atof(arg[1]);
			ptype->dl_rgb[1] = atof(arg[2]);
			ptype->dl_rgb[2] = atof(arg[3]);
		}
		else if (!strcmp(arg[0], "lighttime") && args == 2)
			ptype->dl_time = atof(arg[1]);
		else if (!strcmp(arg[0], "lightshadow") && args == 2)
			ptype->flags = (ptype->flags & ~PT_NODLSHADOW) | (!atoi(arg[1])?PT_NODLSHADOW:0);
		else if (!strcmp(arg[0], "lightcubemapnum") && args == 2)
			ptype->dl_cubemapnum = atoi(arg[1]);
		else if (!strcmp(arg[0], "lightcorona") && args == 3)
		{
			ptype->dl_corona_intensity = atof(arg[1])*0.25;	//dp scales them by 0.25
			ptype->dl_corona_scale = atof(arg[2]);
		}
#if 1
		else if (!strcmp(arg[0], "staincolor") && args == 3)	//stainmaps multiplier
			Con_ThrottlePrintf(&printtimer, 1, "%s.%s: Particle effect token %s not supported\n", *ptype->config?ptype->config:"<NONE>", ptype->name,arg[0]);
		else if (!strcmp(arg[0], "stainalpha") && args == 3)	//affects stainmaps AND stain-decals.
			Con_ThrottlePrintf(&printtimer, 1, "%s.%s: Particle effect token %s not supported\n", *ptype->config?ptype->config:"<NONE>", ptype->name,arg[0]);
		else if (!strcmp(arg[0], "stainsize") && args == 3)		//affects stainmaps AND stain-decals.
			Con_ThrottlePrintf(&printtimer, 1, "%s.%s: Particle effect token %s not supported\n", *ptype->config?ptype->config:"<NONE>", ptype->name,arg[0]);
		else if (!strcmp(arg[0], "staintex") && args == 3)		//actually spawns a decal
			Con_ThrottlePrintf(&printtimer, 1, "%s.%s: Particle effect token %s not supported\n", *ptype->config?ptype->config:"<NONE>", ptype->name,arg[0]);
		else if (!strcmp(arg[0], "stainless") && args == 2)
			Con_ThrottlePrintf(&printtimer, 1, "%s.%s: Particle effect token %s not supported\n", *ptype->config?ptype->config:"<NONE>", ptype->name,arg[0]);
		else if (!strcmp(arg[0], "relativeoriginoffset") && args == 4)
			Con_ThrottlePrintf(&printtimer, 1, "%s.%s: Particle effect token %s not supported\n", *ptype->config?ptype->config:"<NONE>", ptype->name,arg[0]);
		else if (!strcmp(arg[0], "relativevelocityoffset") && args == 4)
			Con_ThrottlePrintf(&printtimer, 1, "%s.%s: Particle effect token %s not supported\n", *ptype->config?ptype->config:"<NONE>", ptype->name, arg[0]);
#endif
		else if (!strcmp(arg[0], "rotate") && args == 5)
		{
			ptype->rotationstartmin		= atof(arg[1]);
			ptype->rotationstartrand	= atof(arg[2]) - ptype->rotationstartmin;
			ptype->rotationmin			= atof(arg[3]);
			ptype->rotationrand			= atof(arg[4]) - ptype->rotationmin;
			ptype->rotationstartmin		*= M_PI/180;
			ptype->rotationstartrand	*= M_PI/180;
			ptype->rotationmin			*= M_PI/180;
			ptype->rotationrand			*= M_PI/180;
			ptype->rotationstartmin += M_PI/4;
		}
		else
			Con_ThrottlePrintf(&printtimer, 0, "%s.%s: Particle effect token not recognised, or invalid args: %s %s %s %s %s %s\n", *ptype->config?ptype->config:"<NONE>", ptype->name, arg[0], args<2?"":arg[1], args<3?"":arg[2], args<4?"":arg[3], args<5?"":arg[4], args<6?"":arg[5]);
		args = 0;
	}

	if (ptype)
		FinishEffectinfoParticleType(ptype, blooddecalonimpact);

	r_plooksdirty = true;
}

static qboolean P_ImportEffectInfo_Name(char *config)
{
	char *file;

	FS_LoadFile(va("%s.txt", config), (void**)&file);

	if (!file)
	{
		Con_Printf("%s not found\n", config);
		return false;
	}
	P_ImportEffectInfo(config, file);
	FS_FreeFile(file);
	return true;
}
static void P_ConvertEffectInfo_f(void)
{
	int i, assoc, n;
	vfsfile_t *outf;
	char effect[1024];
	char fname[64] = "particles/effectinfo.cfg";

	R_Particles_KillAllEffects();
	P_ImportEffectInfo_Name("effectinfo");

	FS_CreatePath("particles/", FS_GAMEONLY);
	outf = FS_OpenVFS(fname, "wb", FS_GAMEONLY);
	if (!outf)
	{
		FS_DisplayPath(fname, FS_GAMEONLY, effect, sizeof(effect));
		Con_TPrintf("Unable to open file %s\n", effect);
		return;
	}
	for (i = 0; i < numparticletypes; i++)
	{
		part_type_t *ptype = &part_type[i];
		if (strcmp(ptype->config, "effectinfo"))
			continue;	//skip any which are not relevant.

		if (strchr(ptype->name, '+'))
			continue;	//skip assoc chains
		Q_strncpyz(effect, ptype->name, sizeof(effect));

		assoc = i;
		for(;;)
		{
			n = part_type[assoc].assoc;
			part_type[assoc].assoc = P_INVALID;
			VFS_PUTS(outf, "r_part ");
			VFS_PUTS(outf, effect);
			VFS_PUTS(outf, "\n{\n");
			PScript_Query(assoc, 1, effect, sizeof(effect));
			VFS_PUTS(outf, effect);
			VFS_PUTS(outf, "}\n");
			part_type[assoc].assoc = n;
			assoc = n;

			if (assoc == P_INVALID)
				break;

			if (strchr(part_type[assoc].name, '+'))
				Q_snprintfz(effect, sizeof(effect), "+%s", part_type[i].name);
		}
	}
	VFS_CLOSE(outf);

	FS_DisplayPath(fname, FS_GAMEONLY, effect, sizeof(effect));
	Con_Printf("Written %s\n", effect);
}
#endif

/*
===============
R_InitParticles
===============
*/
static qboolean PScript_InitParticles (void)
{
	int		i;

	if (r_numparticles)	//already inited
		return true;

	buildsintable();

	r_numparticles = bound(1, r_part_maxparticles.ival, MAX_PARTICLES);
	r_numdecals = bound(1, r_part_maxdecals.ival, MAX_DECALS);

	r_numbeams = MAX_BEAMSEGS;
	r_numtrailstates = MAX_TRAILSTATES;

	particles = (particle_t *)
			BZ_Malloc (r_numparticles * sizeof(particle_t));

	beams = (beamseg_t *)
			BZ_Malloc (r_numbeams * sizeof(beamseg_t));

	decals = (clippeddecal_t *)
			BZ_Malloc (r_numdecals * sizeof(clippeddecal_t));

	trailstates = (trailstate_t *)
			BZ_Malloc (r_numtrailstates * sizeof(trailstate_t));
	memset(trailstates, 0, r_numtrailstates * sizeof(trailstate_t));
	ts_cycle = 0;

	Cmd_AddCommand("pointfile", P_ReadPointFile_f);	//load the leak info produced from qbsp into the particle system to show a line. :)

	pe_script_enabled = true;

#ifdef HAVE_LEGACY
	Cmd_AddCommand("r_exportbuiltinparticles", P_ExportBuiltinSet_f);
	Cmd_AddCommandD("r_converteffectinfo", P_ConvertEffectInfo_f, "Attempt to convert particle effects made for a certain other engine.");
	Cmd_AddCommand("r_exportalleffects", P_ExportAllEffects_f);
#endif

//#if _DEBUG
	Cmd_AddCommand("r_partinfo", P_PartInfo_f);
	Cmd_AddCommand("r_beaminfo", P_BeamInfo_f);
//#endif

	Cvar_Hook(&r_particledesc, R_ParticleDesc_Callback);
	Cvar_ForceCallback(&r_particledesc);


	for (i = 0; i < (BUFFERVERTS>>2)*6; i += 6)
	{
		pscriptquadindexes[i+0] = ((i/6)<<2)+0;
		pscriptquadindexes[i+1] = ((i/6)<<2)+1;
		pscriptquadindexes[i+2] = ((i/6)<<2)+2;
		pscriptquadindexes[i+3] = ((i/6)<<2)+0;
		pscriptquadindexes[i+4] = ((i/6)<<2)+2;
		pscriptquadindexes[i+5] = ((i/6)<<2)+3;
	}
	pscriptmesh.xyz_array = pscriptverts;
	pscriptmesh.st_array = pscripttexcoords;
	pscriptmesh.colors4f_array[0] = pscriptcolours;
	pscriptmesh.indexes = pscriptquadindexes;
	for (i = 0; i < BUFFERVERTS; i++)
	{
		pscripttriindexes[i] = i;
	}
	pscripttmesh.xyz_array = pscriptverts;
	pscripttmesh.st_array = pscripttexcoords;
	pscripttmesh.colors4f_array[0] = pscriptcolours;
	pscripttmesh.indexes = pscripttriindexes;
	return true;
}

static void PScript_Shutdown (void)
{
	pe_script_enabled = false;

	if (fallback)
		fallback->ShutdownParticles();

	Cvar_Unhook(&r_particledesc);

	Cmd_RemoveCommand("pointfile");	//load the leak info produced from qbsp into the particle system to show a line. :)

	
	Cmd_RemoveCommand("r_converteffectinfo");
	Cmd_RemoveCommand("r_exportalleffects");
	Cmd_RemoveCommand("r_exportbuiltinparticles");
	Cmd_RemoveCommand("r_importeffectinfo");

//#if _DEBUG
	Cmd_RemoveCommand("r_partinfo");
	Cmd_RemoveCommand("r_beaminfo");
//#endif

	pe_default			= P_INVALID;
	pe_size2			= P_INVALID;
	pe_size3			= P_INVALID;
	pe_defaulttrail		= P_INVALID;

	while(loadedconfigs)
	{
		pcfg_t *cfg;
		cfg = loadedconfigs;
		loadedconfigs = cfg->next;
		Z_Free(cfg);
	}

	while (numparticletypes > 0)
	{
		numparticletypes--;
		if (part_type[numparticletypes].models)
			BZ_Free(part_type[numparticletypes].models);
		if (part_type[numparticletypes].sounds)
			BZ_Free(part_type[numparticletypes].sounds);
		if (part_type[numparticletypes].ramp)
			BZ_Free(part_type[numparticletypes].ramp);
	}
	BZ_Free (part_type);
	part_type = NULL;
	part_run_list = NULL;
	fallback = NULL;

	BZ_Free (particles);
	BZ_Free (beams);
	BZ_Free (decals);
	BZ_Free (trailstates);

	r_numparticles = 0;
}


/*
===============
P_ClearParticles
===============
*/
static void PScript_ClearParticles (void)
{
	int		i;

	if (fallback)
		fallback->ClearParticles();

	free_particles = &particles[0];
	for (i=0 ;i<r_numparticles ; i++)
		particles[i].next = &particles[i+1];
	particles[r_numparticles-1].next = NULL;

	free_decals = &decals[0];
	for (i=0 ;i<r_numdecals ; i++)
		decals[i].next = &decals[i+1];
	decals[r_numdecals-1].next = NULL;

	free_beams = &beams[0];
	for (i=0 ;i<r_numbeams ; i++)
	{
		beams[i].p = NULL;
		beams[i].flags = BS_DEAD;
		beams[i].next = &beams[i+1];
	}
	beams[r_numbeams-1].next = NULL;

	particletime = cl.time;

	for (i = 0; i < numparticletypes; i++)
	{
		P_LoadTexture(&part_type[i], false);
	}

	for (i = 0; i < numparticletypes; i++)
	{
		part_type[i].clippeddecals = NULL;
		part_type[i].particles = NULL;
		part_type[i].beams = NULL;
	}
}

#ifdef HAVE_LEGACY
static void P_ExportBuiltinSet_f(void)
{
	char *efname = Cmd_Argv(1);
	char *file = NULL;
	int i;

	if (!*efname)
	{
		Con_Printf("Please name the built in effect (faithful, spikeset, tsshaft, minimal or highfps)\n");
		return;
	}

	for (i = 0; partset_list[i].name; i++)
	{
		if (!stricmp(efname, partset_list[i].name))
		{
			file = *partset_list[i].data;
			if (file)
			{
				COM_WriteFile(va("particles/%s.cfg", efname), FS_GAMEONLY, file, strlen(file));
				Con_Printf("Written particles/%s.cfg\n", efname);
			}
			else
				Con_Printf("nothing to export\n");
			return;
		}
	}

	Con_Printf("'%s' is not a built in particle set\n", efname);
}
#endif

static qboolean P_LoadParticleSet(char *name, qboolean implicit, qboolean showwarning)
{
	char *file;
	int i;
	int restrictlevel = Cmd_FromGamecode() ? RESTRICT_SERVER : RESTRICT_LOCAL;
	pcfg_t *cfg;

	if (!*name)
		return false;

	//protect against configs being loaded multiple times. this can easily happen with namespaces (especially if an effect is missing).
	for (cfg = loadedconfigs; cfg; cfg = cfg->next)
	{
		//already loaded?
		if (!strcmp(cfg->name, name))
			return false;
	}
	cfg = Z_Malloc(sizeof(*cfg) + strlen(name));
	strcpy(cfg->name, name);
	cfg->next = loadedconfigs;
	loadedconfigs = cfg;
	name = cfg->name;

#ifdef PSET_CLASSIC
	if (!strcmp(name, "classic"))
	{
		if (fallback)
			fallback->ShutdownParticles();
		fallback = &pe_classic;
		if (fallback)
		{
			fallback->InitParticles();
			fallback->ClearParticles();
		}
		return true;
	}
#endif

	//built-in particle effects are always favoured. This is annoying, but means the cvar value alone is enough to detect cheats.
	for (i = 0; partset_list[i].name; i++)
	{
		if (!stricmp(name, partset_list[i].name))
		{
			if (partset_list[i].data)
			{
				Cbuf_AddText(va("\nr_part namespace %s %i\n", name, implicit), restrictlevel);
				Cbuf_AddText(*partset_list[i].data, restrictlevel);
				Cbuf_AddText("\nr_part namespace \"\" 0\n", restrictlevel);
			}
			return true;
		}
	}

	if (!strncmp(name, "./", 2))
		FS_LoadFile(va("%s.cfg", name+2), (void**)&file);
	else
		FS_LoadFile(va("particles/%s.cfg", name), (void**)&file);
	if (!file && strchr(name, '/'))
		FS_LoadFile(va("%s.cfg", name), (void**)&file);
	if (file)
	{
		Cbuf_AddText(va("\nr_part namespace %s %i\n", name, implicit), restrictlevel);
		Cbuf_AddText(file, restrictlevel);
		Cbuf_AddText("\nr_part namespace \"\" 0\n", restrictlevel);
		FS_FreeFile(file);
	}
	else
	{
#ifdef HAVE_LEGACY
		if (!strcmp(name, "effectinfo"))
		{
			//FIXME: we're loading this too early to deal with per-map stuff.
			//FIXME: wait until after particle precache info has been received, and only reload if the loaded configs actually changed.
			P_ImportEffectInfo_Name(name);
			return true;
		}
#endif
		if (showwarning)
			Con_Printf(CON_WARNING "Couldn't find particle description %s\n", name);
		return false;
	}
	return true;
}

static void R_Particles_KillAllEffects(void)
{
	int i;
	pcfg_t *cfg;

	for (i = 0; i < numparticletypes; i++)
	{
		*part_type[i].texname = '\0';
		part_type[i].scale = 0;
		part_type[i].loaded = 0;
		if (part_type->ramp)
			BZ_Free(part_type->ramp);
		part_type->ramp = NULL;
	}
//	numparticletypes = 0;
//	BZ_Free(part_type);
//	part_type = NULL;

	f_modified_particles = false;

	if (fallback)
	{
		fallback->ShutdownParticles();
		fallback = NULL;
	}

	while(loadedconfigs)
	{
		cfg = loadedconfigs;
		loadedconfigs = cfg->next;
		Z_Free(cfg);
	}
}

static void QDECL R_ParticleDesc_Callback(struct cvar_s *var, char *oldvalue)
{
	char token[256];
	int count;
	qboolean		failure;

	char *c;

	if (qrenderer == QR_NONE)
		return; // don't bother parsing early

	R_Particles_KillAllEffects();

	if (cls.state != ca_connected)
	{
		failure = false;
		count = 0;
		for (c = COM_ParseStringSet(var->string, token, sizeof(token)); token[0]; c = COM_ParseStringSet(c, token, sizeof(token)))
		{
			if (*token)
			{
				if (!P_LoadParticleSet(token, false, false))
					failure = true;
				count++;
			}
		}

		//if we didn't manage to load any, make sure SOMETHING got loaded...
#ifdef PSET_CLASSIC
		if (!count)
			P_LoadParticleSet("classic", true, true);
		else
#endif
			if (failure)
			P_LoadParticleSet("high", true, true);

		if (cls.state && cl.model_name[1])
		{
			//per-map configs. because we can.
			memcpy(token, "map_", 4);
			COM_FileBase(cl.model_name[1], token+4, sizeof(token)-4);
			P_LoadParticleSet(token, false, false);
		}
	}

//	Cbuf_AddText("r_effect\n", RESTRICT_LOCAL);

	//make sure nothing is stale.
	CL_RegisterParticles();
}

static void P_ReadPointFile_f (void)
{
	vfsfile_t	*f;
	vec3_t	org;
	//int		r; //unreferenced
	int		c;
	char	name[MAX_OSPATH];
	char line[1024];
	char *s;
	int pt_pointfile;
	unsigned int spawned = 0, total = 0;

	COM_StripExtension(cl.worldmodel->name, name, sizeof(name));
	strcat(name, ".pts");

	f = FS_OpenVFS(name, "rb", FS_GAME);
	if (!f)
	{
		Con_Printf ("couldn't open %s\n", name);
		return;
	}

	P_ClearParticles();	//so overflows arn't as bad.

	Con_Printf ("Reading %s...\n", name);
	c = 0;
	pt_pointfile		= PScript_FindParticleType("PT_POINTFILE");

	if (pt_pointfile == P_INVALID)
	{
		if (!fallback)
		{
#ifdef PSET_CLASSIC
			fallback = &pe_classic;
#endif
			if (fallback)
			{
				fallback->InitParticles();
				fallback->ClearParticles();
			}
		}
	}
	while(VFS_GETS(f, line, sizeof(line)))
	{
		s = COM_Parse(line);
		org[0] = atof(com_token);

		s = COM_Parse(s);
		if (!s)
			continue;
		org[1] = atof(com_token);

		s = COM_Parse(s);
		if (!s)
			continue;
		org[2] = atof(com_token);
		if (COM_Parse(s))
			continue;

		c++;

		if (c%8)
			continue;

		total++;
		if (pt_pointfile == P_INVALID)
		{
#ifdef PSET_CLASSIC
			spawned += PClassic_PointFile(c, org);
#endif
		}
		else
		{
			if (free_particles)
				spawned+= 0<P_RunParticleEffectType(org, NULL, 1, pt_pointfile);
		}
	}

	VFS_CLOSE (f);
	Con_Printf ("spawned %i of %i points\n", spawned, c);
}

void PScript_ClearSurfaceParticles(model_t *mod)
{
	mod->skytime = 0;
	mod->skytris = NULL;
	while(mod->skytrimem)
	{
		void *f = mod->skytrimem;
		mod->skytrimem = mod->skytrimem->next;
		Z_Free(f);
	}
	mod->skytime = 0;
	mod->engineflags |= MDLF_RECALCULATERAIN;
}

static void R_Part_SkyTri(model_t *mod, float *v1, float *v2, float *v3, msurface_t *surf, int ptype)
{
	float dot;
	float xm;
	float ym;
	float theta;
	vec3_t xd;
	vec3_t yd;

	skytris_t *st;

	skytriblock_t *mem = mod->skytrimem;
	if (!mem || mem->count >= sizeof(mem->tris)/sizeof(mem->tris[0]))
	{
		mod->skytrimem = BZ_Malloc(sizeof(*mod->skytrimem));
		mod->skytrimem->next = mem;
		mod->skytrimem->count = 0;
		mem = mod->skytrimem;
	}

	st = &mem->tris[mem->count];
	VectorCopy(v1, st->org);
	VectorSubtract(v2, st->org, st->x);
	VectorSubtract(v3, st->org, st->y);

	VectorCopy(st->x, xd);
	VectorCopy(st->y, yd);
/*
	xd[2] = 0;	//prevent area from being valid on vertical surfaces
	yd[2] = 0;
*/
	xm = Length(xd);
	ym = Length(yd);

	dot = DotProduct(xd, yd);
	theta = acos(dot/(xm*ym));
	st->area = sin(theta)*xm*ym;
	st->nexttime = particletime;
	st->face = surf;
	st->ptype = ptype;

	if (st->area<=0)
		return;//bummer.

	mem->count++;
	st->next = mod->skytris;
	mod->skytris = st;
}



static void PScript_EmitSkyEffectTris(model_t *mod, msurface_t 	*fa, int ptype)
{
	vec3_t		verts[64];
	int v1;
	int v2;
	int v3;
	int numverts;
	int i, lindex;
	float *vec;

	if (ptype < 0 || ptype >= numparticletypes)
		return;

	//
	// convert edges back to a normal polygon
	//
	numverts = 0;
	for (i=0 ; i<fa->numedges ; i++)
	{
		lindex = mod->surfedges[fa->firstedge + i];

		if (lindex > 0)
			vec = mod->vertexes[mod->edges[lindex].v[0]].position;
		else
			vec = mod->vertexes[mod->edges[-lindex].v[1]].position;
		VectorCopy (vec, verts[numverts]);
		numverts++;

		if (numverts>=64)
		{
			Con_Printf("Too many verts on sky surface\n");
			return;
		}
	}

	v1 = 0;
	v2 = 1;
	for (v3 = 2; v3 < numverts; v3++)
	{
		R_Part_SkyTri(mod, verts[v1], verts[v2], verts[v3], fa, ptype);

		v2 = v3;
	}
}
static void P_AddRainParticles(model_t *mod, vec3_t axis[3], vec3_t eorg, float contribution)
{
	float x;
	float y;
	part_type_t *type;

	vec3_t org, vdist, worg, wnorm;

	skytris_t *st;
	size_t nc,oc;
	float ot;
	int area;
	int cluster;
	unsigned int contentbits;

	if (mod->engineflags & MDLF_RECALCULATERAIN)
	{
		PScript_ClearSurfaceParticles(mod);
		mod->engineflags &= ~MDLF_RECALCULATERAIN;

		if (mod->type == mod_brush)
		{
			int t, i;
			int ptype;
			msurface_t *surf;

			/*particle emision based upon texture. this is lazy code*/
			for (t = mod->numtextures-1; t >= 0; t--)
			{
				/*FIXME: we should read the shader for the particle names here*/
				/*FIXME: if the paticle system changes mid-map, we should be prepared to reload things*/
				char *pn;
				char *h;
				if (mod->textures[t]->partname)
					pn = mod->textures[t]->partname;
				else
				{
					pn = va("tex_%s", mod->textures[t]->name);
					//strip any trailing data after #, so textures can have shader args
					h = strchr(pn, '#');
					if (h)
						*h = 0;
				}
				ptype = P_FindParticleType(pn);

				if (ptype != P_INVALID)
				{
					for (i=0; i<mod->nummodelsurfaces; i++)
					{
						surf = mod->surfaces + i + mod->firstmodelsurface;
						if (surf->texinfo->texture == mod->textures[t])
						{
							/*FIXME: it would be a good idea to determine the surface's (midpoint) pvs cluster so that we're not spamming for the entire map*/
							PScript_EmitSkyEffectTris(mod, surf, ptype);
						}
					}
				}
			}
		}
	}

	if (!mod->skytris)
		return;

	ot = mod->skytime;
	mod->skytime += contribution;

	for (st = mod->skytris; st; st = st->next)
	{
		if ((unsigned int)st->ptype >= (unsigned int)numparticletypes)
			continue;
		type = &part_type[st->ptype];
		if (!type->loaded)	//woo, batch skipping.
			continue;

		nc = (mod->skytime*st->area*r_part_rain_quantity.value*type->rainfrequency)/10000.0;
		oc = (ot*st->area*r_part_rain_quantity.value*type->rainfrequency)/10000.0;

		while (oc < nc)
		{
			oc++;
			if (!free_particles)
				return;

//			st->nexttime += 10000.0/(st->area*r_part_rain_quantity.value*type->rainfrequency);

			x = frandom()*frandom();
			y = frandom() * (1-x);
			VectorMA(st->org, x, st->x, org);
			VectorMA(org, y, st->y, org);

			worg[0] = DotProduct(org, axis[0]) + eorg[0];
			worg[1] = DotProduct(org, axis[1]) + eorg[1];
			worg[2] = DotProduct(org, axis[2]) + eorg[2];

			//ignore it if its too far away
			VectorSubtract(worg, r_refdef.vieworg, vdist);
			if (VectorLength(vdist) > (1024+512)*frandom())
				continue;

			if (cl.worldmodel->funcs.InfoForPoint)
			{
				cl.worldmodel->funcs.InfoForPoint(cl.worldmodel, worg, &area, &cluster, &contentbits);
				if (contentbits & FTECONTENTS_SOLID)
					continue;
				if (r_refdef.scenevis && !(r_refdef.scenevis[cluster>>3] & (1<<(cluster&7))))
					continue;
			}

			if (st->face->flags & SURF_PLANEBACK)
				VectorScale(st->face->plane->normal, -1, vdist);
			else
				VectorCopy(st->face->plane->normal, vdist);

			wnorm[0] = DotProduct(vdist, axis[0]);
			wnorm[1] = DotProduct(vdist, axis[1]);
			wnorm[2] = DotProduct(vdist, axis[2]);

			VectorMA(worg, 0.5, wnorm, worg);

			P_RunParticleEffectType(worg, wnorm, 1, st->ptype);
		}
	}
}

// Trailstate functions
static void P_CleanTrailstate(trailstate_t *ts)
{
	// clear LASTSEG flag from lastbeam so it can be reused
	if (ts->lastbeam)
	{
		ts->lastbeam->flags &= ~BS_LASTSEG;
		ts->lastbeam->flags |= BS_NODRAW;
	}

	// clean structure
	memset(ts, 0, sizeof(trailstate_t));
}

static void PScript_DelinkTrailstate(trailkey_t *tk)
{
	trailkey_t key;
	trailstate_t *ts;

	key = *tk;
	*tk = 0;

	while (key && key <= r_numtrailstates)
	{
		ts = trailstates + (key - 1);

		if (ts->key != key)
			break; // prevent overwrite

		key = ts->assoc; // next to clean
		P_CleanTrailstate(ts);
	}
}

static trailstate_t *P_NewTrailstate(void)
{
	trailstate_t *ts;

	// bounds check here in case r_numtrailstates changed
	if (ts_cycle >= r_numtrailstates)
		ts_cycle = 0;

	ts = trailstates + ts_cycle;
	P_CleanTrailstate(ts);
	ts_cycle++;
	ts->key = ts_cycle; // key is 1 above index, allows 0 to be invalid

	return ts;
}

static trailstate_t* P_FetchTrailstate(trailkey_t* tk)
{
	trailstate_t* ts;

	// trailstate allocation/deallocation
	if (tk)
	{
		trailkey_t key = *tk;
		if (key == 0 || key > r_numtrailstates)
		{
			ts = P_NewTrailstate();
			*tk = ts->key;
		}
		else
		{
			ts = trailstates + (key - 1);
			if (ts->key != key) // trailstate was overwritten
			{
				ts = P_NewTrailstate(); // so get a new one
				*tk = ts->key;
			}
		}
	}
	else
		ts = NULL;

	return ts;
}

#define NUMVERTEXNORMALS	162
static float	r_avertexnormals[NUMVERTEXNORMALS][3] = {
#include "anorms.h"
};
static vec2_t	avelocities[NUMVERTEXNORMALS];
#define BEAMLENGTH 16
// vec3_t	avelocity = {23, 7, 3};
// float	partstep = 0.01;
// float	timescale = 0.01;



static void PScript_ApplyOrgVel(vec3_t oorg, vec3_t ovel, vec3_t eforg, vec3_t axis[3], int pno, int pmax, part_type_t *ptype)
{
	vec3_t ofsvec, arsvec;

	float k,l,m;
	int spawnspc, i=pno, j;


	l=0;
	j=0;
	k=0;
	m=0;

	spawnspc = 8;
	switch (ptype->spawnmode)
	{
	case SM_UNICIRCLE:
		m = pmax;
		if (ptype->looks.type == PT_BEAM||ptype->looks.type == PT_VBEAM)
			m--;

		if (m < 1)
			m = 0;
		else
			m = (M_PI*2)/m;

		if (ptype->spawnparam1) /* use for weird shape hacks */
			m *= ptype->spawnparam1;
		break;
	case SM_TELEBOX:
		spawnspc = 4;
		l = -ptype->areaspreadvert;
	case SM_LAVASPLASH:
		j = k = -ptype->areaspread;
		if (ptype->spawnparam1)
			m = ptype->spawnparam1;
		else
			m = 0.55752; /* default weird number for tele/lavasplash used in vanilla Q1 */

		if (ptype->spawnparam2)
			spawnspc = (int)ptype->spawnparam2;
		break;
	case SM_FIELD:
		if (!avelocities[0][0])
		{
			for (j=0 ; j<NUMVERTEXNORMALS ; j++)
			{
				avelocities[j][0] = (rand()&255) * 0.01;
				avelocities[j][1] = (rand()&255) * 0.01;
			}
		}

		j = pno%NUMVERTEXNORMALS;
		m = pno/NUMVERTEXNORMALS;
		break;
	default:	//others don't need intitialisation
		break;
	}


	ovel[0] = 0;
	ovel[1] = 0;
	ovel[2] = 0;

	// handle spawn modes (org/vel)
	switch (ptype->spawnmode)
	{
	case SM_BOX:
		ofsvec[0] = crandom();
		ofsvec[1] = crandom();
		ofsvec[2] = crandom();

		arsvec[0] = ofsvec[0]*ptype->areaspread;
		arsvec[1] = ofsvec[1]*ptype->areaspread;
		arsvec[2] = ofsvec[2]*ptype->areaspreadvert;
		break;
	case SM_TELEBOX:
		ofsvec[0] = k;
		ofsvec[1] = j;
		ofsvec[2] = l+4;
		VectorNormalize(ofsvec);
		VectorScale(ofsvec, 1.0-(frandom())*m, ofsvec);

		// org is just like the original
		arsvec[0] = j + (rand()%spawnspc);
		arsvec[1] = k + (rand()%spawnspc);
		arsvec[2] = l + (rand()%spawnspc);

		// advance telebox loop
		j += spawnspc;
		if (j >= ptype->areaspread)
		{
			j = -ptype->areaspread;
			k += spawnspc;
			if (k >= ptype->areaspread)
			{
				k = -ptype->areaspread;
				l += spawnspc;
				if (l >= ptype->areaspreadvert)
					l = -ptype->areaspreadvert;
			}
		}
		break;
	case SM_LAVASPLASH:
		// calc directions, org with temp vector
		ofsvec[0] = k + (rand()%spawnspc);
		ofsvec[1] = j + (rand()%spawnspc);
		ofsvec[2] = 256;

		arsvec[0] = ofsvec[0];
		arsvec[1] = ofsvec[1];
		arsvec[2] = frandom()*ptype->areaspreadvert;

		VectorNormalize(ofsvec);
		VectorScale(ofsvec, 1.0-(frandom())*m, ofsvec);

		// advance splash loop
		j += spawnspc;
		if (j >= ptype->areaspread)
		{
			j = -ptype->areaspread;
			k += spawnspc;
			if (k >= ptype->areaspread)
				k = -ptype->areaspread;
		}
		break;
	case SM_UNICIRCLE:
		ofsvec[0] = cos(m*i);
		ofsvec[1] = sin(m*i);
		ofsvec[2] = 0;
		VectorScale(ofsvec, ptype->areaspread, arsvec);
		break;
	case SM_FIELD:
		arsvec[0] = (cl.time * avelocities[i][0]) + m;
		arsvec[1] = (cl.time * avelocities[i][1]) + m;
		arsvec[2] = cos(arsvec[1]);

		ofsvec[0] = arsvec[2]*cos(arsvec[0]);
		ofsvec[1] = arsvec[2]*sin(arsvec[0]);
		ofsvec[2] = -sin(arsvec[1]);

//		arsvec[0] = r_avertexnormals[j][0]*ptype->areaspread + ofsvec[0]*BEAMLENGTH;
//		arsvec[1] = r_avertexnormals[j][1]*ptype->areaspread + ofsvec[1]*BEAMLENGTH;
//		arsvec[2] = r_avertexnormals[j][2]*ptype->areaspreadvert + ofsvec[2]*BEAMLENGTH;

		l = ptype->spawnparam2 * sin(cl.time+j+m);
		arsvec[0] = r_avertexnormals[j][0]*(ptype->areaspread+l) + ofsvec[0]*ptype->spawnparam1;
		arsvec[1] = r_avertexnormals[j][1]*(ptype->areaspread+l) + ofsvec[1]*ptype->spawnparam1;
		arsvec[2] = r_avertexnormals[j][2]*(ptype->areaspreadvert+l) + ofsvec[2]*ptype->spawnparam1;

		VectorNormalize(ofsvec);

		j++;
		if (j >= NUMVERTEXNORMALS)
		{
			j = 0;
			m += 0.1762891; // some BS number to try to "randomize" things
		}
		break;
	case SM_DISTBALL:
		{
			float rdist;

			rdist = ptype->spawnparam2 - crandom()*(1-(crandom() * ptype->spawnparam1));

			// this is a strange spawntype, which is based on the fact that
			// crandom()*crandom() provides something similar to an exponential
			// probability curve
			ofsvec[0] = hrandom();
			ofsvec[1] = hrandom();
			if (ptype->areaspreadvert)
				ofsvec[2] = hrandom();
			else
				ofsvec[2] = 0;

			VectorNormalize(ofsvec);
			VectorScale(ofsvec, rdist, ofsvec);

			arsvec[0] = ofsvec[0]*ptype->areaspread;
			arsvec[1] = ofsvec[1]*ptype->areaspread;
			arsvec[2] = ofsvec[2]*ptype->areaspreadvert;
		}
		break;
	default: // SM_BALL, SM_CIRCLE
		ofsvec[0] = hrandom();
		ofsvec[1] = hrandom();
		if (ptype->areaspreadvert)
			ofsvec[2] = hrandom();
		else
			ofsvec[2] = 0;

		VectorNormalize(ofsvec);
		if (ptype->spawnmode != SM_CIRCLE)
			VectorScale(ofsvec, frandom(), ofsvec);

		arsvec[0] = ofsvec[0]*ptype->areaspread;
		arsvec[1] = ofsvec[1]*ptype->areaspread;
		arsvec[2] = ofsvec[2]*ptype->areaspreadvert;
		break;
	}

	k = ptype->orgadd + frandom()*ptype->randomorgadd;
	l = ptype->veladd + frandom()*ptype->randomveladd;

#if 1
	VectorMA(ovel, ofsvec[0]*ptype->spawnvel, axis[0], ovel);
	VectorMA(ovel, ofsvec[1]*ptype->spawnvel, axis[1], ovel);
	VectorMA(ovel, l+ofsvec[2]*ptype->spawnvelvert, axis[2], ovel);

	VectorMA(eforg, arsvec[0], axis[0], oorg);
	VectorMA(oorg, arsvec[1], axis[1], oorg);
	VectorMA(oorg, k+arsvec[2], axis[2], oorg);
#else
	oorg[0] = eforg[0] + arsvec[0];
	oorg[1] = eforg[1] + arsvec[1];
	oorg[2] = eforg[2] + arsvec[2];

	// apply arsvec+ofsvec
	if (efdir)
	{
		ovel[0] += efdir[0]*l+ofsvec[0]*ptype->spawnvel;
		ovel[1] += efdir[1]*l+ofsvec[1]*ptype->spawnvel;
		ovel[2] += efdir[2]*l+ofsvec[2]*ptype->spawnvelvert;

		oorg[0] += efdir[0]*k;
		oorg[1] += efdir[1]*k;
		oorg[2] += efdir[2]*k;
	}
	else
	{//efdir is effectively up - '0 0 -1'
		ovel[0] += ofsvec[0]*ptype->spawnvel;
		ovel[1] += ofsvec[1]*ptype->spawnvel;
		ovel[2] += ofsvec[2]*ptype->spawnvelvert - l;

		oorg[2] -= k;
	}
#endif

	if (ptype->flags & PT_WORLDSPACERAND)
	{
		do
		{
			ofsvec[0] = crand();
			ofsvec[1] = crand();
			ofsvec[2] = crand();
		} while(DotProduct(ofsvec,ofsvec)>1);	//crap, but I'm trying to mimic dp
		oorg[0] += ofsvec[0] * ptype->orgwrand[0];
		oorg[1] += ofsvec[1] * ptype->orgwrand[1];
		oorg[2] += ofsvec[2] * ptype->orgwrand[2];
		ovel[0] += ofsvec[0] * ptype->velwrand[0];
		ovel[1] += ofsvec[1] * ptype->velwrand[1];
		ovel[2] += ofsvec[2] * ptype->velwrand[2];
		VectorAdd(ovel, ptype->velbias, ovel);
	}

	VectorAdd(oorg, ptype->orgbias, oorg);
}

static void PScript_EffectSpawned(part_type_t *ptype, vec3_t org, vec3_t axis[3], int dlkey, float countscale)
{
	extern cvar_t r_lightflicker;
	if (ptype->dl_radius[0] || ptype->dl_radius[1])// && r_rocketlight.value)
	{
		float radius;
		dlight_t *dl;

		static int flickertime;
		static int flicker;
		int i = realtime*20;
		if (flickertime != i)
		{
			flickertime = i;
			flicker = rand();
		}
		radius = ptype->dl_radius[0] + (r_lightflicker.ival?((flicker + dlkey*2000)&0xffff)*(1.0f/0xffff):0.5)*ptype->dl_radius[1];

		dl = CL_NewDlight(dlkey, org, radius, ptype->dl_time, ptype->dl_rgb[0], ptype->dl_rgb[1], ptype->dl_rgb[2]);
		dl->channelfade[0]	= ptype->dl_decay[0];
		dl->channelfade[1]	= ptype->dl_decay[1];
		dl->channelfade[2]	= ptype->dl_decay[2];
		dl->decay			= ptype->dl_decay[3];
		dl->corona			= ptype->dl_corona_intensity;
		dl->coronascale		= ptype->dl_corona_scale;
#ifdef RTLIGHTS
		dl->lightcolourscales[0] = ptype->dl_scales[0];
		dl->lightcolourscales[1] = ptype->dl_scales[1];
		dl->lightcolourscales[2] = ptype->dl_scales[2];
#endif
		if (ptype->flags & PT_NODLSHADOW)
			dl->flags |= LFLAG_NOSHADOWS;
		if (ptype->dl_cubemapnum)
			Q_snprintfz(dl->cubemapname, sizeof(dl->cubemapname), "cubemaps/%i", ptype->dl_cubemapnum);
		dl->style = ptype->dl_lightstyle;
	}
	if (ptype->numsounds)
	{
		int i;
		float w,tw;
		for (i = 0, tw = 0; i < ptype->numsounds; i++)
			tw += ptype->sounds[i].weight;
		w = frandom() * tw;	//select the sound by weight
		//and figure out which one that weight corresponds to
		for (i = 0, tw = 0; i < ptype->numsounds; i++)
		{
			tw += ptype->sounds[i].weight;
			if (w <= tw)
			{
				if (*ptype->sounds[i].name && ptype->sounds[i].vol > 0)
					S_StartSound(0, 0, S_PrecacheSound(ptype->sounds[i].name), org, NULL, ptype->sounds[i].vol, ptype->sounds[i].atten, -ptype->sounds[i].delay, ptype->sounds[i].pitch, 0);
				break;
			}
		}
	}
	if (ptype->stain_radius)
		Surf_AddStain(org, ptype->stain_rgb[0], ptype->stain_rgb[1], ptype->stain_rgb[2], ptype->stain_radius);
}

typedef struct
{
	part_type_t *ptype;
	int entity;
	model_t *model;
	vec3_t center;
	vec3_t normal;
	vec3_t tangent1;
	vec3_t tangent2;

	float scale0;
	float scale1;
	float scale2;

	float bias1;
	float bias2;
} decalctx_t;
static void PScript_AddDecals(void *vctx, vec3_t *fte_restrict points, size_t numtris, shader_t *surfshader)
{
	decalctx_t *ctx = vctx;
	part_type_t *ptype = ctx->ptype;
	clippeddecal_t *d;
	unsigned int i;
	vec3_t vec;
	while(numtris-->0)
	{
		if (!free_decals)
			break;

		d = free_decals;
		free_decals = d->next;
		d->next = ptype->clippeddecals;
		ptype->clippeddecals = d;

		for (i = 0; i < 3; i++)
		{
			VectorCopy(points[i], d->vertex[i]);
			VectorSubtract(d->vertex[i], ctx->center, vec);
			d->texcoords[i][0] = (DotProduct(vec, ctx->tangent1)*ctx->scale1)+ctx->bias1;
			d->texcoords[i][1] = (DotProduct(vec, ctx->tangent2)*ctx->scale2)+ctx->bias2;
			if (r_decal_noperpendicular.ival)
			{
				//the decal code is already making sure the surfaces are mostly aligned, which should solve some issues.
				//this means we can make sure that there's NO fading at all, so no issues if the center of the effect is not actually aligned with any surface (yay inprecision).
				d->valpha[i] = 1;
			}
			else
			{
				//fade the alpha depending on the distance from the center)
				//FIXME: should be fabsed by glsl so that linear interpolation works correctly
				d->valpha[i] = 1 - fabs((DotProduct(vec, ctx->normal)*ctx->scale0));
			}
		}
		points += 3;

		d->entity = ctx->entity;
//		d->model = ctx->model;
		d->die = ptype->randdie*frandom();

		if (ptype->die)
			d->rgba[3] = ptype->alpha + d->die*ptype->alphachange;
		else
			d->rgba[3] = ptype->alpha;
		d->rgba[3] += ptype->alpharand*frandom();

		if (ptype->colorindex >= 0)
		{
			int cidx;
			cidx = ptype->colorrand > 0 ? rand() % ptype->colorrand : 0;
			cidx = ptype->colorindex + cidx;
			if (cidx > 255)
				d->rgba[3] = d->rgba[3] / 2; // Hexen 2 style transparency
			cidx = (cidx & 0xff) * 3;
			d->rgba[0] = host_basepal[cidx] * (1/255.0);
			d->rgba[1] = host_basepal[cidx+1] * (1/255.0);
			d->rgba[2] = host_basepal[cidx+2] * (1/255.0);
		}
		else
			VectorCopy(ptype->rgb, d->rgba);

		vec[2] = frandom();
		vec[0] = vec[2]*ptype->rgbrandsync[0] + frandom()*(1-ptype->rgbrandsync[0]);
		vec[1] = vec[2]*ptype->rgbrandsync[1] + frandom()*(1-ptype->rgbrandsync[1]);
		vec[2] = vec[2]*ptype->rgbrandsync[2] + frandom()*(1-ptype->rgbrandsync[2]);
		d->rgba[0] += vec[0]*ptype->rgbrand[0] + ptype->rgbchange[0]*d->die;
		d->rgba[1] += vec[1]*ptype->rgbrand[1] + ptype->rgbchange[1]*d->die;
		d->rgba[2] += vec[2]*ptype->rgbrand[2] + ptype->rgbchange[2]*d->die;

		d->die = particletime + ptype->die - d->die;

		if (ptype->looks.type != PT_CDECAL)
			d->die += 20;

		// maintain run list
		if (!(ptype->state & PS_INRUNLIST))
		{
			ptype->runlink = &part_run_list;
			ptype->nexttorun = part_run_list;
			if (part_run_list)
				part_run_list->runlink = &ptype->nexttorun;
			*ptype->runlink = ptype;
			ptype->state |= PS_INRUNLIST;
		}
	}
}

static int PScript_RunParticleEffectState (vec3_t org, vec3_t dir, float count, int typenum, trailkey_t *tk)
{
	part_type_t *ptype = &part_type[typenum];
	int i, j, k, l, spawnspc;
	float m, pcount;//, orgadd, veladd;
	vec3_t axis[3]={{1,0,0},{0,1,0},{0,0,-1}};
	particle_t	*p;
	beamseg_t *b, *bfirst;
	vec3_t ofsvec, arsvec; // offsetspread vec, areaspread vec

	float orgadd, veladd;
	trailstate_t *ts;

	if (typenum >= FALLBACKBIAS && fallback)
		return fallback->RunParticleEffectState(org, dir, count, typenum-FALLBACKBIAS, NULL);

	if (typenum < 0 || typenum >= numparticletypes)
		return 1;

	if (!ptype->loaded)
		return 1;

	// inwater check, switch only once
	if (r_part_contentswitch.ival && ptype->inwater >= 0 && cl.worldmodel && cl.worldmodel->loadstate == MLS_LOADED)
	{
		int cont;
		cont = cl.worldmodel->funcs.PointContents(cl.worldmodel, NULL, org);

		if (cont & FTECONTENTS_FLUID)
			ptype = &part_type[ptype->inwater];
	}

	// eliminate trailstate if flag set
	if (ptype->flags & PT_NOSTATE)
		tk = NULL;

	ts = P_FetchTrailstate(tk);

	// get msvc to shut up
	j = k = l = 0;
	m = 0;

	while(ptype)
	{
		if (r_part_contentswitch.ival && (ptype->flags & (PT_TRUNDERWATER | PT_TROVERWATER)) && cl.worldmodel && cl.worldmodel->loadstate==MLS_LOADED)
		{
			int cont;
			cont = cl.worldmodel->funcs.PointContents(cl.worldmodel, NULL, org);

			if ((ptype->flags & PT_TROVERWATER) && (cont & ptype->fluidmask))
				goto skip;
			if ((ptype->flags & PT_TRUNDERWATER) && !(cont & ptype->fluidmask))
				goto skip;
		}

		if (dir && (dir[0] || dir[1] || dir[2]))
		{
			void PerpendicularVector( vec3_t dst, const vec3_t src );
			VectorCopy(dir, axis[2]);
			VectorNormalize(axis[2]);
			PerpendicularVector(axis[0], axis[2]);
			VectorNormalize(axis[0]);
			CrossProduct(axis[2], axis[0], axis[1]);
			VectorNormalize(axis[1]);
		}
		PScript_EffectSpawned(ptype, org, axis, 0, count);

		if (ptype->looks.type == PT_CDECAL)
		{
			vec3_t vec={0.5, 0.5, 0.5};
			int i;
			decalctx_t ctx;
			vec3_t bestdir;

			if (!free_decals)
				return 0;

			ctx.entity = 0;
			ctx.model = cl.worldmodel;
			if (!ctx.model)
				return 0;

			VectorCopy(org, ctx.center);
			if (!dir || (dir[0] == 0 && dir[1] == 0 && dir[2] == 0))
			{
				float bestfrac = 1;
				float frac;
				vec3_t impact, normal;
				int what;
				bestdir[0] = 0;
				bestdir[1] = 0.73;
				bestdir[2] = 0.73;
				VectorNormalize(bestdir);
				for (i = 0; i < 6; i++)
				{
					if (i >= 3)
					{
						ctx.tangent1[0] = (i==3)*16;
						ctx.tangent1[1] = (i==4)*16;
						ctx.tangent1[2] = (i==5)*16;
					}
					else
					{
						ctx.tangent1[0] = -(i==0)*16;
						ctx.tangent1[1] = -(i==1)*16;
						ctx.tangent1[2] = -(i==2)*16;
					}
					VectorSubtract(org, ctx.tangent1, ctx.tangent2);
					VectorAdd(org, ctx.tangent1, ctx.tangent1);


					frac = CL_TraceLine(ctx.tangent2, ctx.tangent1, impact, normal, &what);
					if (bestfrac > frac)
					{
						bestfrac = frac;
						VectorCopy(normal, bestdir);
						VectorCopy(impact, ctx.center);
						ctx.entity = what;
					}
				}
				dir = bestdir;
			}
			else
			{	//the dir arg is generally assumed to be facing away from the surface.
				VectorMA(org, 16, dir, ctx.tangent2);
				VectorMA(org, -16, dir, ctx.tangent1);
				CL_TraceLine(ctx.tangent2, ctx.tangent1, ctx.center, bestdir, &ctx.entity);
			}

			if (ctx.entity)
			{
				if (ctx.entity>0 && (unsigned)ctx.entity < (unsigned)cl.maxlerpents)
				{
					lerpents_t *le = cl.lerpents+ctx.entity;
					ctx.model = cl.model_precache[le->entstate->modelindex];
					if (le->entstate)
						VectorSubtract(ctx.center, le->origin, ctx.center);
					else
						ctx.entity = 0;
				}
				else if (ctx.entity<0 && (unsigned)-ctx.entity < (unsigned)csqc_world.num_edicts)
				{
					wedict_t *e = WEDICT_NUM_UB(csqc_world.progs, -ctx.entity);
					if (e)
					{
						ctx.model = csqc_world.Get_CModel(&csqc_world, e->v->modelindex);
						if (!ctx.model)
							continue;
						VectorSubtract(ctx.center, e->v->origin, ctx.center);
					}
					else
						continue;
				}
				else
					ctx.entity = 0;
			}

			VectorNegate(dir, ctx.normal);
			VectorNormalize(ctx.normal);

			VectorNormalize(vec);
			CrossProduct(ctx.normal, vec, ctx.tangent1);
			Matrix4x4_CM_Transform3(Matrix4x4_CM_NewRotation(frandom()*360, ctx.normal[0], ctx.normal[1], ctx.normal[2]), ctx.tangent1, ctx.tangent2);
			CrossProduct(ctx.normal, ctx.tangent2, ctx.tangent1);

			VectorNormalize(ctx.tangent1);
			VectorNormalize(ctx.tangent2);

			ctx.ptype = ptype;
			ctx.scale1 = ptype->s2 - ptype->s1;
			ctx.bias1 = ptype->s1 + ctx.scale1/2;
			ctx.scale2 = ptype->t2 - ptype->t1;
			ctx.bias2 = ptype->t1 + ctx.scale2/2;
			m = ptype->scale + frandom() * ptype->scalerand;
			ctx.scale0 = 2.0 / m;
			ctx.scale1 /= m;
			ctx.scale2 /= m;

			if (ptype->randsmax!=1)
				ctx.bias1 += ptype->texsstride * (rand()%ptype->randsmax);

			//inserts decals through a callback.
			Mod_ClipDecal(ctx.model, ctx.center, ctx.normal, ctx.tangent2, ctx.tangent1, m, ptype->surfflagmask, ptype->surfflagmatch, PScript_AddDecals, &ctx);

			if (ptype->assoc < 0)
				break;

			ptype = &part_type[ptype->assoc];
			continue;
		}

		// init spawn specific variables
		b = bfirst = NULL;
		spawnspc = 8;
		pcount = ptype->countextra + count*(ptype->count+ptype->countrand*frandom());
		if (ptype->flags & PT_INVFRAMETIME)
			pcount /= host_frametime;
		if (ts)
			pcount += ts->effect.emittime;

		pcount *= r_part_density.value;

		switch (ptype->spawnmode)
		{
		case SM_UNICIRCLE:
			m = pcount;
			if (ptype->looks.type == PT_BEAM||ptype->looks.type == PT_VBEAM)
				m--;

			if (m < 1)
				m = 0;
			else
				m = (M_PI*2)/m;

			if (ptype->spawnparam1) /* use for weird shape hacks */
				m *= ptype->spawnparam1;
			break;
		case SM_TELEBOX:
			spawnspc = 4;
			l = -ptype->areaspreadvert;
		case SM_LAVASPLASH:
			j = k = -ptype->areaspread;
			if (ptype->spawnparam1)
				m = ptype->spawnparam1;
			else
				m = 0.55752; /* default weird number for tele/lavasplash used in vanilla Q1 */

			if (ptype->spawnparam2)
				spawnspc = (int)ptype->spawnparam2;
			break;
		case SM_FIELD:
			if (!avelocities[0][0])
			{
				for (j=0 ; j<NUMVERTEXNORMALS ; j++)
				{
					avelocities[j][0] = (rand()&255) * 0.01;
					avelocities[j][1] = (rand()&255) * 0.01;
				}
			}

			j = 0;
			m = 0;
			break;
//		case SM_MESHSURFACE:
//			meshsurface = querymesh;
//			totalarea = gah;
//			density = count / totalarea;
//			area = 0;
//			tri = -1;
//			break;
		case SM_FIXMEWARNING:
			Con_ThrottlePrintf(&throttle, 0, CON_WARNING"Particle effect %s.%s is marked as a placeholder\n", ptype->config, ptype->name);
			return 1;
		default:	//others don't need intitialisation
			break;
		}

		// time limit (for completeness)
		if (ptype->spawntime && ts)
		{
			if (ts->effect.statetime > particletime)
				return 0; // timelimit still in effect

			ts->effect.statetime = particletime + ptype->spawntime; // record old time
		}

		// random chance for point effects
		if (ptype->spawnchance < frandom())
		{
			i = ceil(pcount);
			break;
		}

		if (ptype->nummodels)
		{	//model spawning loop
			partmodels_t *mod;
			if (ptype->count == 0 && ptype->countrand == 0 && ptype->countextra == 0)
				pcount = count;	//if they just gave some models with no counts at all, assume they meant count=1.
			for (i = 0; i < pcount; i++)
			{
				mod = &ptype->models[rand() % ptype->nummodels];
				if (!mod->model)
					mod->model = Mod_ForName(mod->name, MLV_WARN);
				if (mod->model)
				{
					vec3_t morg, mdir;
					float scale = frandom() * (mod->scalemax-mod->scalemin) + mod->scalemin;
					PScript_ApplyOrgVel(morg, mdir, org, axis, i, count, ptype);
					CL_SpawnSpriteEffect(morg, mdir, (mod->rflags&RF_USEORIENTATION)?axis[2]:NULL, mod->model, mod->framestart, mod->framecount, mod->framerate?mod->framerate:10, mod->alpha?mod->alpha:1, scale, ptype->rotationmin*180/M_PI, ptype->gravity, mod->traileffect, (mod->rflags & ~RF_USEORIENTATION) | RF_FORCECOLOURMOD, mod->skin, mod->rgb[0], mod->rgb[1], mod->rgb[2]);
				}
			}
		}
		else
		{
			/*this is a hack, use countextra=1, count=0*/
			if (!ptype->die && ptype->count == 1 && ptype->countrand == 0 && pcount < 1)
				pcount = 1;
			for (i = 0; i < pcount; i++)
			{	// particle spawning loop

				if (!free_particles)
					break;
				p = free_particles;
				if (ptype->looks.type == PT_BEAM||ptype->looks.type == PT_VBEAM)
				{
					if (!free_beams)
						break;
					if (b)
					{
						b = b->next = free_beams;
						free_beams = free_beams->next;
					}
					else
					{
						b = bfirst = free_beams;
						free_beams = free_beams->next;
					}
					b->texture_s = i; // TODO: FIX THIS NUMBER
					b->flags = 0;
					b->p = p;
					VectorClear(b->dir);
				}
				free_particles = p->next;
				p->next = ptype->particles;
				ptype->particles = p;

				p->die = ptype->randdie*frandom();
				p->scale = ptype->scale+ptype->scalerand*frandom();
				if (ptype->die)
					p->rgba[3] = ptype->alpha+p->die*ptype->alphachange;
				else
					p->rgba[3] = ptype->alpha;
				p->rgba[3] += ptype->alpharand*frandom();
				// p->color = 0;
				if (ptype->emittime < 0)
					p->state.trailstate = trailkey_null;
				else
					p->state.nextemit = particletime + ptype->emitstart - p->die;

				p->rotationspeed = ptype->rotationmin + frandom()*ptype->rotationrand;
				p->angle = ptype->rotationstartmin + frandom()*ptype->rotationstartrand;
				p->s1 = ptype->s1;
				p->t1 = ptype->t1;
				p->s2 = ptype->s2;
				p->t2 = ptype->t2;
				if (ptype->randsmax!=1)
				{
					m = ptype->texsstride * (rand()%ptype->randsmax);
					p->s1 += m;
					p->s2 += m;
				}

				if (ptype->colorindex >= 0)
				{
					int cidx;
					cidx = ptype->colorrand > 0 ? rand() % ptype->colorrand : 0;
					cidx = ptype->colorindex + cidx;
					if (cidx > 255)
						p->rgba[3] = p->rgba[3] / 2; // Hexen 2 style transparency
					cidx = (cidx & 0xff) * 3;
					p->rgba[0] = host_basepal[cidx] * (1/255.0);
					p->rgba[1] = host_basepal[cidx+1] * (1/255.0);
					p->rgba[2] = host_basepal[cidx+2] * (1/255.0);
				}
				else
					VectorCopy(ptype->rgb, p->rgba);

				// use org temporarily for rgbsync
				p->org[2] = frandom();
				p->org[0] = p->org[2]*ptype->rgbrandsync[0] + frandom()*(1-ptype->rgbrandsync[0]);
				p->org[1] = p->org[2]*ptype->rgbrandsync[1] + frandom()*(1-ptype->rgbrandsync[1]);
				p->org[2] = p->org[2]*ptype->rgbrandsync[2] + frandom()*(1-ptype->rgbrandsync[2]);

				p->rgba[0] += p->org[0]*ptype->rgbrand[0] + ptype->rgbchange[0]*p->die;
				p->rgba[1] += p->org[1]*ptype->rgbrand[1] + ptype->rgbchange[1]*p->die;
				p->rgba[2] += p->org[2]*ptype->rgbrand[2] + ptype->rgbchange[2]*p->die;

#if 0
				PScript_ApplyOrgVel(p->org, p->vel, org, axis, i, pcount, ptype);
#else
				p->vel[0] = 0;
				p->vel[1] = 0;
				p->vel[2] = 0;

				// handle spawn modes (org/vel)
				switch (ptype->spawnmode)
				{
/*				case SM_MESHSURFACE:
					if (area <= 0)
					{
						tri++;
						area += calcarea(tri);
						arsvec[] = calcnormal(tri);
					}

					ofsvec[] = randompointintriangle(tri);

					area -= density;
					break;
*/
				case SM_BOX:
					ofsvec[0] = crandom();
					ofsvec[1] = crandom();
					ofsvec[2] = crandom();

					arsvec[0] = ofsvec[0]*ptype->areaspread;
					arsvec[1] = ofsvec[1]*ptype->areaspread;
					arsvec[2] = ofsvec[2]*ptype->areaspreadvert;
					break;
				case SM_TELEBOX:
					ofsvec[0] = k;
					ofsvec[1] = j;
					ofsvec[2] = l+4;
					VectorNormalize(ofsvec);
					VectorScale(ofsvec, 1.0-(frandom())*m, ofsvec);

					// org is just like the original
					arsvec[0] = j + (rand()%spawnspc);
					arsvec[1] = k + (rand()%spawnspc);
					arsvec[2] = l + (rand()%spawnspc);

					// advance telebox loop
					j += spawnspc;
					if (j >= ptype->areaspread)
					{
						j = -ptype->areaspread;
						k += spawnspc;
						if (k >= ptype->areaspread)
						{
							k = -ptype->areaspread;
							l += spawnspc;
							if (l >= ptype->areaspreadvert)
								l = -ptype->areaspreadvert;
						}
					}
					break;
				case SM_LAVASPLASH:
					// calc directions, org with temp vector
					ofsvec[0] = k + (rand()%spawnspc);
					ofsvec[1] = j + (rand()%spawnspc);
					ofsvec[2] = 256;

					arsvec[0] = ofsvec[0];
					arsvec[1] = ofsvec[1];
					arsvec[2] = frandom()*ptype->areaspreadvert;

					VectorNormalize(ofsvec);
					VectorScale(ofsvec, 1.0-(frandom())*m, ofsvec);

					// advance splash loop
					j += spawnspc;
					if (j >= ptype->areaspread)
					{
						j = -ptype->areaspread;
						k += spawnspc;
						if (k >= ptype->areaspread)
							k = -ptype->areaspread;
					}
					break;
				case SM_UNICIRCLE:
					ofsvec[0] = cos(m*i);
					ofsvec[1] = sin(m*i);
					ofsvec[2] = 0;
					VectorScale(ofsvec, ptype->areaspread, arsvec);
					break;
				case SM_FIELD:
					arsvec[0] = (cl.time * avelocities[i][0]) + m;
					arsvec[1] = (cl.time * avelocities[i][1]) + m;
					arsvec[2] = cos(arsvec[1]);

					ofsvec[0] = arsvec[2]*cos(arsvec[0]);
					ofsvec[1] = arsvec[2]*sin(arsvec[0]);
					ofsvec[2] = -sin(arsvec[1]);

//					arsvec[0] = r_avertexnormals[j][0]*ptype->areaspread + ofsvec[0]*BEAMLENGTH;
//					arsvec[1] = r_avertexnormals[j][1]*ptype->areaspread + ofsvec[1]*BEAMLENGTH;
//					arsvec[2] = r_avertexnormals[j][2]*ptype->areaspreadvert + ofsvec[2]*BEAMLENGTH;

					orgadd = ptype->spawnparam2 * sin(cl.time+j+m);
					arsvec[0] = r_avertexnormals[j][0]*(ptype->areaspread+orgadd) + ofsvec[0]*ptype->spawnparam1;
					arsvec[1] = r_avertexnormals[j][1]*(ptype->areaspread+orgadd) + ofsvec[1]*ptype->spawnparam1;
					arsvec[2] = r_avertexnormals[j][2]*(ptype->areaspreadvert+orgadd) + ofsvec[2]*ptype->spawnparam1;

					VectorNormalize(ofsvec);

					j++;
					if (j >= NUMVERTEXNORMALS)
					{
						j = 0;
						m += 0.1762891; // some BS number to try to "randomize" things
					}
					break;
				case SM_DISTBALL:
					{
						float rdist;

						rdist = ptype->spawnparam2 - crandom()*(1-(crandom() * ptype->spawnparam1));

						// this is a strange spawntype, which is based on the fact that
						// crandom()*crandom() provides something similar to an exponential
						// probability curve
						ofsvec[0] = hrandom();
						ofsvec[1] = hrandom();
						if (ptype->areaspreadvert)
							ofsvec[2] = hrandom();
						else
							ofsvec[2] = 0;

						VectorNormalize(ofsvec);
						VectorScale(ofsvec, rdist, ofsvec);

						arsvec[0] = ofsvec[0]*ptype->areaspread;
						arsvec[1] = ofsvec[1]*ptype->areaspread;
						arsvec[2] = ofsvec[2]*ptype->areaspreadvert;
					}
					break;
				default: // SM_BALL, SM_CIRCLE
					{
						ofsvec[0] = hrandom();
						ofsvec[1] = hrandom();
						if (ptype->areaspreadvert)
							ofsvec[2] = hrandom();
						else
							ofsvec[2] = 0;

						VectorNormalize(ofsvec);
						if (ptype->spawnmode != SM_CIRCLE)
							VectorScale(ofsvec, frandom(), ofsvec);

						arsvec[0] = ofsvec[0]*ptype->areaspread;
						arsvec[1] = ofsvec[1]*ptype->areaspread;
						arsvec[2] = ofsvec[2]*ptype->areaspreadvert;
					}
					break;
				}

				// apply arsvec+ofsvec
				orgadd = ptype->orgadd + frandom()*ptype->randomorgadd;
				veladd = ptype->veladd + frandom()*ptype->randomveladd;
#if 1
				if (dir)
					veladd *= VectorLength(dir);
				VectorMA(p->vel, ofsvec[0]*ptype->spawnvel, axis[0], p->vel);
				VectorMA(p->vel, ofsvec[1]*ptype->spawnvel, axis[1], p->vel);
				VectorMA(p->vel, veladd+ofsvec[2]*ptype->spawnvelvert, axis[2], p->vel);

				VectorMA(org, arsvec[0], axis[0], p->org);
				VectorMA(p->org, arsvec[1], axis[1], p->org);
				VectorMA(p->org, orgadd+arsvec[2], axis[2], p->org);
#else
				p->org[0] = org[0] + arsvec[0];
				p->org[1] = org[1] + arsvec[1];
				p->org[2] = org[2] + arsvec[2];
				if (dir)
				{
					p->vel[0] += dir[0]*veladd+ofsvec[0]*ptype->spawnvel;
					p->vel[1] += dir[1]*veladd+ofsvec[1]*ptype->spawnvel;
					p->vel[2] += dir[2]*veladd+ofsvec[2]*ptype->spawnvelvert;

					p->org[0] += dir[0]*orgadd;
					p->org[1] += dir[1]*orgadd;
					p->org[2] += dir[2]*orgadd;
				}
				else
				{
					p->vel[0] += ofsvec[0]*ptype->spawnvel;
					p->vel[1] += ofsvec[1]*ptype->spawnvel;
					p->vel[2] += ofsvec[2]*ptype->spawnvelvert - veladd;

					p->org[2] -= orgadd;
				}
#endif
				if (ptype->flags & PT_WORLDSPACERAND)
				{
					do
					{
						ofsvec[0] = crand();
						ofsvec[1] = crand();
						ofsvec[2] = crand();
					} while(DotProduct(ofsvec,ofsvec)>1);	//crap, but I'm trying to mimic dp
					p->org[0] += ofsvec[0] * ptype->orgwrand[0];
					p->org[1] += ofsvec[1] * ptype->orgwrand[1];
					p->org[2] += ofsvec[2] * ptype->orgwrand[2];
					p->vel[0] += ofsvec[0] * ptype->velwrand[0];
					p->vel[1] += ofsvec[1] * ptype->velwrand[1];
					p->vel[2] += ofsvec[2] * ptype->velwrand[2];
					VectorAdd(p->vel, ptype->velbias, p->vel);
				}
				VectorAdd(p->org, ptype->orgbias, p->org);
#endif

				p->die = particletime + ptype->die - p->die;

				VectorCopy(p->org, p->oldorg);
			}
		}

		// update beam list
		if (ptype->looks.type == PT_BEAM||ptype->looks.type == PT_VBEAM)
		{
			if (b)
			{
				// update dir for bfirst for certain modes since it will never get updated
				switch (ptype->spawnmode)
				{
				case SM_UNICIRCLE:
					// kinda hackish here, assuming ofsvec contains the point at i-1
					arsvec[0] = cos(m*(i-2));
					arsvec[1] = sin(m*(i-2));
					arsvec[2] = 0;
					VectorSubtract(b->p->org, arsvec, bfirst->dir);
					VectorNormalize(bfirst->dir);
					break;
				default:
					break;
				}

				b->flags |= BS_NODRAW;
				b->next = ptype->beams;
				ptype->beams = bfirst;
			}
		}

		// save off emit times in trailstate
		if (ts)
			ts->effect.emittime = pcount - i;

		// maintain run list
		if (!(ptype->state & PS_INRUNLIST) && (ptype->particles || ptype->clippeddecals))
		{
			ptype->runlink = &part_run_list;
			ptype->nexttorun = part_run_list;
			if (part_run_list)
				part_run_list->runlink = &ptype->nexttorun;
			*ptype->runlink = ptype;
			ptype->state |= PS_INRUNLIST;
		}

skip:

		// go to next associated effect
		if (ptype->assoc < 0)
			break;

		// new trailstate
		if (ts)
		{
			tk = &(ts->assoc);
			ts = P_FetchTrailstate(tk);
		}

		ptype = &part_type[ptype->assoc];
	}

	return 0;
}

static int PScript_RunParticleEffectTypeString (vec3_t org, vec3_t dir, float count, char *name)
{
	int type = P_FindParticleType(name);
	if (type < 0)
		return 1;

	return P_RunParticleEffectType(org, dir, count, type);
}

/*
===============
P_RunParticleEffect

===============
*/
static void PScript_RunParticleEffect (vec3_t org, vec3_t dir, int color, int count)
{
	int ptype;

	ptype = P_FindParticleType(va("pe_%i", color));
	if (P_RunParticleEffectType(org, dir, count, ptype))
	{
		if (count > 130 && PART_VALID(pe_size3))
		{
			part_type[pe_size3].colorindex = color & ~0x7;
			part_type[pe_size3].colorrand = 8;
			P_RunParticleEffectType(org, dir, count, pe_size3);
		}
		else if (count > 20 && PART_VALID(pe_size2))
		{
			part_type[pe_size2].colorindex = color & ~0x7;
			part_type[pe_size2].colorrand = 8;
			P_RunParticleEffectType(org, dir, count, pe_size2);
		}
		else if (PART_VALID(pe_default))
		{
			part_type[pe_default].colorindex = color & ~0x7;
			part_type[pe_default].colorrand = 8;
			P_RunParticleEffectType(org, dir, count, pe_default);
		}
		else if (fallback)
			fallback->RunParticleEffect(org, dir, color, count);
	}
}

//h2 stylie
static void PScript_RunParticleEffect2 (vec3_t org, vec3_t dmin, vec3_t dmax, int color, int effect, int count)
{
	int			i, j;
	float		num;
	float invcount;
	vec3_t	nvel;

	int ptype = P_FindParticleType(va("pe2_%i_%i", effect, color));
	if (!PART_VALID(ptype))
	{
		ptype = P_FindParticleType(va("pe2_%i", effect));
		if (!PART_VALID(ptype))
		{
			if (fallback)
			{
				fallback->RunParticleEffect2(org, dmin, dmax, color, effect, count);
				return;
			}
			ptype = pe_default;
		}
		if (!PART_VALID(ptype))
			return;
		part_type[ptype].colorindex = color;
	}

	invcount = 1/part_type[ptype].count; // using this to get R_RPET to always spawn 1
	count = count * part_type[ptype].count;

	for (i=0 ; i<count ; i++)
	{
		if (!free_particles)
			return;

		for (j=0 ; j<3 ; j++)
		{
			num = rand() / (float)RAND_MAX;
			nvel[j] = dmin[j] + ((dmax[j] - dmin[j]) * num);
		}
		P_RunParticleEffectType(org, nvel, invcount, ptype);

	}
}

/*
===============
P_RunParticleEffect3

===============
*/
//h2 stylie
static void PScript_RunParticleEffect3 (vec3_t org, vec3_t box, int color, int effect, int count)
{
	int			i, j;
	vec3_t	nvel;
	float		num;
	float invcount;

	int ptype = P_FindParticleType(va("pe3_%i_%i", effect, color));
	if (!PART_VALID(ptype))
	{
		ptype = P_FindParticleType(va("pe3_%i", effect));
		if (!PART_VALID(ptype))
		{
//			if (fallback)
//			{
//				fallback->RunParticleEffect3(org, box, color, effect, count);
//				return;
//			}
			ptype = pe_default;
		}
		if (!PART_VALID(ptype))
			return;
		part_type[ptype].colorindex = color;
	}

	invcount = 1/part_type[ptype].count; // using this to get R_RPET to always spawn 1
	count = count * part_type[ptype].count;

	for (i=0 ; i<count ; i++)
	{
		if (!free_particles)
			return;

		for (j=0 ; j<3 ; j++)
		{
			num = rand() / (float)RAND_MAX;
			nvel[j] = (box[j] * num * 2) - box[j];
		}

		P_RunParticleEffectType(org, nvel, invcount, ptype);
	}
}

/*
===============
P_RunParticleEffect4

===============
*/
//h2 stylie
static void PScript_RunParticleEffect4 (vec3_t org, float radius, int color, int effect, int count)
{
	int			i, j;
	vec3_t	nvel;
	float		num;
	float invcount;

	int ptype = P_FindParticleType(va("pe4_%i_%i", effect, color));
	if (!PART_VALID(ptype))
	{
		ptype = P_FindParticleType(va("pe4_%i", effect));
		if (!PART_VALID(ptype))
		{
//			if (fallback)
//			{
//				fallback->RunParticleEffect4(org, radius, color, effect, count);
//				return;
//			}
			ptype = pe_default;
		}
		if (!PART_VALID(ptype))
			return;
		part_type[ptype].colorindex = color;
	}

	invcount = 1/part_type[ptype].count; // using this to get R_RPET to always spawn 1
	count = count * part_type[ptype].count;

	for (i=0 ; i<count ; i++)
	{
		if (!free_particles)
			return;

		for (j=0 ; j<3 ; j++)
		{
			num = rand() / (float)RAND_MAX;
			nvel[j] = (radius * num * 2) - radius;
		}
		P_RunParticleEffectType(org, nvel, invcount, ptype);
	}
}

static void PScript_RunParticleCube(int ptype, vec3_t minb, vec3_t maxb, vec3_t dir_min, vec3_t dir_max, float count, int colour, qboolean gravity, float jitter)
{
	vec3_t org;
	int			i, j;
	float		num;
	float invcount;

	if (!PART_VALID(ptype))
		ptype = P_FindParticleType(va("te_cube%s_%i", gravity?"_g":"", colour));
	if (!PART_VALID(ptype))
	{
		ptype = P_FindParticleType(va("te_cube%s", gravity?"_g":""));
		if (!PART_VALID(ptype))
			ptype = pe_default;
		if (!PART_VALID(ptype))
			return;
		part_type[ptype].colorindex = colour;
	}

	invcount = 1/part_type[ptype].count; // using this to get R_RPET to always spawn 1
	count = count * part_type[ptype].count;

	for (i=0 ; i<count ; i++)
	{
		if (!free_particles)
			return;

		for (j=0 ; j<3 ; j++)
		{
			num = rand() / (float)RAND_MAX;
			org[j] = minb[j] + num*(maxb[j]-minb[j]);
		}
		P_RunParticleEffectType(org, dir_min, invcount, ptype);
	}
}

static void PScript_RunParticleWeather(vec3_t minb, vec3_t maxb, vec3_t dir, float count, int colour, char *efname)
{
	vec3_t org;
	int			i, j;
	float		num;
	float invcount;

	int ptype = P_FindParticleType(va("te_%s_%i", efname, colour));
	if (!PART_VALID(ptype))
	{
		ptype = P_FindParticleType(va("te_%s", efname));
		if (!PART_VALID(ptype))
			ptype = pe_default;
		if (!PART_VALID(ptype))
			return;
		part_type[ptype].colorindex = colour;
	}

	invcount = 1/part_type[ptype].count; // using this to get R_RPET to always spawn 1
	count = count * part_type[ptype].count;

	for (i=0 ; i<count ; i++)
	{
		if (!free_particles)
			return;

		for (j=0 ; j<3 ; j++)
		{
			num = rand() / (float)RAND_MAX;
			org[j] = minb[j] + num*(maxb[j]-minb[j]);
		}
		P_RunParticleEffectType(org, dir, invcount, ptype);
	}
}

static void PScript_RunParticleEffectPalette (const char *nameprefix, vec3_t org, vec3_t dir, int color, int count)
{
	int ptype;

	ptype = P_FindParticleType(va("%s_%i", nameprefix, color));
	if (ptype != P_INVALID)
		P_RunParticleEffectType(org, dir, count, ptype);
	else
	{
		ptype = P_FindParticleType(nameprefix);
		if (!PART_VALID(ptype))
		{
			part_type[ptype].colorindex = color;
			P_RunParticleEffectType(org, dir, count, ptype);
		}
	}
}

static void P_ParticleTrailSpawn (vec3_t startpos, vec3_t end, part_type_t *ptype, float timeinterval, trailkey_t* tk, int dlkey, vec3_t dlaxis[3])
{
	vec3_t	vec, vstep, right, up, start;
	float	len;
	int			tcount;
	particle_t	*p;
	beamseg_t   *b;
	beamseg_t   *bfirst;
	trailstate_t *ts;
	int count;

	float veladd = -ptype->veladd;
	float step;
	float stop;
	float tdegree = 2.0*M_PI/256; /* MSVC whine */
	float sdegree = 0;
	float nrfirst, nrlast;

	VectorCopy(startpos, start);

	// eliminate trailstate if flag set
	if (ptype->flags & PT_NOSTATE)
		tk = NULL;

	ts = P_FetchTrailstate(tk);

	PScript_EffectSpawned(ptype, start, dlaxis, dlkey, 1);

	if (ptype->assoc>=0)
	{
		if (ts)
			P_ParticleTrail(start, end, ptype->assoc, timeinterval, dlkey, NULL, &(ts->assoc));
		else
			P_ParticleTrail(start, end, ptype->assoc, timeinterval, dlkey, NULL, NULL);
	}

	if (r_part_contentswitch.ival && (ptype->flags & (PT_TRUNDERWATER | PT_TROVERWATER)) && cl.worldmodel)
	{
		int cont;
		cont = cl.worldmodel->funcs.PointContents(cl.worldmodel, NULL, startpos);

		if ((ptype->flags & PT_TROVERWATER) && (cont & ptype->fluidmask))
			return;
		if ((ptype->flags & PT_TRUNDERWATER) && !(cont & ptype->fluidmask))
			return;
	}

	// time limit for trails
	if (ptype->spawntime && ts)
	{
		if (ts->effect.statetime > particletime)
			return; // timelimit still in effect

		ts->effect.statetime = particletime + ptype->spawntime; // record old time
		ts = NULL; // clear trailstate so we don't save length/lastseg
	}

	if (ptype->nummodels)
		return;	//counts are too screwy.

	// random chance for trails
	if (ptype->spawnchance < frandom())
		return; // don't spawn but return success

	if (!ptype->die)
		ts = NULL;

	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	// use ptype step to calc step vector and step size

	//(fractional) extra count
	step = (cl.paused&&(ptype->die||ptype->randdie))?0:ptype->countextra;
	step += ptype->count * r_part_density.value * timeinterval;

	//round it with overflow tracking
	step += ptype->countoverflow;
	count = step;
	ptype->countoverflow = step-count;

	step = ptype->countspacing;					//particles per qu
	step /= r_part_density.value;				//scaled...

	if (ptype->flags & PT_AVERAGETRAIL)
	{
		float tavg;
		// mangle len/step to get last point to be at end
		tavg = len / step;
		tavg = tavg / ceil(tavg);
		step *= tavg;
		len += step;
	}

	VectorScale(vec, step, vstep);

	// store last stop here for lack of a better solution besides vectors
	if (ts)
	{
		ts->trail.laststop = stop = ts->trail.laststop + len;	//when to stop
		len = ts->trail.lastdist;
	}
	else
	{
		stop = len;
		len = 0;
	}

	if (step && len < stop)
	{
		if (count && len)
		{
			//recalculate step to cover the entire distance.
			count += (stop-len) / step;
			step = (stop-len)/count;
		}
		else
			count += (stop-len) / step;
	}
	else
	{
		step = 0;
		VectorClear(vstep);
	}

// add offset
//	VectorAdd(start, ptype->orgbias, start);

	// spawn mode precalculations
	switch(ptype->spawnmode)
	{
	case SM_SPIRAL:
		VectorVectors(vec, right, up);

		// precalculate degree of rotation
		if (ptype->spawnparam1)
			tdegree = 2.0*M_PI/ptype->spawnparam1; /* distance per rotation inversed */
		sdegree = ptype->spawnparam2*(M_PI/180);
		break;
	case SM_CIRCLE:
		VectorVectors(vec, right, up);
		break;
	case SM_FIXMEWARNING:
		Con_ThrottlePrintf(&throttle, 0, CON_WARNING"Particle effect %s.%s is marked as a placeholder\n", ptype->config, ptype->name);
		return;
	default:
		break;
	}

	if (ptype->flags & PT_NOSPREADFIRST)
		nrfirst = len + step*1.5;
	else
		nrfirst = len;

	if (ptype->flags & PT_NOSPREADLAST)
		nrlast = stop;
	else
		nrlast = stop + step;

	b = bfirst = NULL;

	if (ptype->nummodels)
	{	//model spawning loop
		partmodels_t *mod;
		int i;
		for (i = 0; i < count; i++)
		{
			mod = &ptype->models[rand() % ptype->nummodels];
			if (!mod->model)
				mod->model = Mod_ForName(mod->name, MLV_WARN);
			if (mod->model)
			{
				vec3_t morg, mdir;
				float scale = frandom() * (mod->scalemax-mod->scalemin) + mod->scalemin;
				PScript_ApplyOrgVel(morg, mdir, start, dlaxis, i, count, ptype);
				CL_SpawnSpriteEffect(morg, mdir, (mod->rflags&RF_USEORIENTATION)?dlaxis[2]:NULL, mod->model, mod->framestart, mod->framecount, mod->framerate?mod->framerate:10, mod->alpha?mod->alpha:1, scale, ptype->rotationmin*180/M_PI, ptype->gravity, mod->traileffect, (mod->rflags & ~RF_USEORIENTATION) | RF_FORCECOLOURMOD, mod->skin, mod->rgb[0], mod->rgb[1], mod->rgb[2]);
			}
			VectorAdd (start, vstep, start);
		}
	}
	else while (count-->0)//len < stop)
	{
		len += step;

		if (!free_particles)
		{
			len = stop;
			break;
		}

		p = free_particles;
		if (ptype->looks.type == PT_BEAM||ptype->looks.type == PT_VBEAM)
		{
			if (!free_beams)
			{
				len = stop;
				break;
			}
			if (b)
			{
				b = b->next = free_beams;
				free_beams = free_beams->next;
			}
			else
			{
				b = bfirst = free_beams;
				free_beams = free_beams->next;
			}
			b->texture_s = len; // not sure how to calc this
			b->flags = 0;
			b->p = p;
			VectorCopy(vec, b->dir);
		}

		free_particles = p->next;
		p->next = ptype->particles;
		ptype->particles = p;

		p->die = ptype->randdie*frandom();
		p->scale = ptype->scale+ptype->scalerand*frandom();
		if (ptype->die)
			p->rgba[3] = ptype->alpha+p->die*ptype->alphachange;
		else
			p->rgba[3] = ptype->alpha;
		p->rgba[3] += ptype->alpharand*frandom();
//		p->color = 0;

//		if (ptype->spawnmode == SM_TRACER)
		if (ptype->spawnparam1)
			tcount = (int)(len * ptype->count / ptype->spawnparam1);
		else
			tcount = (int)(len * ptype->count);

		if (ptype->colorindex >= 0)
		{
			int cidx;
			cidx = ptype->colorrand > 0 ? rand() % ptype->colorrand : 0;
			if (ptype->flags & PT_CITRACER) // colorindex behavior as per tracers in std Q1
				cidx += ((tcount & 4) << 1);

			cidx = ptype->colorindex + cidx;
			if (cidx > 255)
				p->rgba[3] = p->rgba[3] / 2;
			cidx = (cidx & 0xff) * 3;
			p->rgba[0] = host_basepal[cidx] * (1/255.0);
			p->rgba[1] = host_basepal[cidx+1] * (1/255.0);
			p->rgba[2] = host_basepal[cidx+2] * (1/255.0);
		}
		else
			VectorCopy(ptype->rgb, p->rgba);

		// use org temporarily for rgbsync
		p->org[2] = frandom();
		p->org[0] = p->org[2]*ptype->rgbrandsync[0] + frandom()*(1-ptype->rgbrandsync[0]);
		p->org[1] = p->org[2]*ptype->rgbrandsync[1] + frandom()*(1-ptype->rgbrandsync[1]);
		p->org[2] = p->org[2]*ptype->rgbrandsync[2] + frandom()*(1-ptype->rgbrandsync[2]);

		p->rgba[0] += p->org[0]*ptype->rgbrand[0] + ptype->rgbchange[0]*p->die;
		p->rgba[1] += p->org[1]*ptype->rgbrand[1] + ptype->rgbchange[1]*p->die;
		p->rgba[2] += p->org[2]*ptype->rgbrand[2] + ptype->rgbchange[2]*p->die;

		VectorClear (p->vel);
		if (ptype->emittime < 0)
			p->state.trailstate = trailkey_null; // init trailstate
		else
			p->state.nextemit = particletime + ptype->emitstart - p->die;

		p->rotationspeed = ptype->rotationmin + frandom()*ptype->rotationrand;
		p->angle = ptype->rotationstartmin + frandom()*ptype->rotationstartrand;
		p->s1 = ptype->s1;
		p->t1 = ptype->t1;
		p->s2 = ptype->s2;
		p->t2 = ptype->t2;
		if (ptype->randsmax!=1)
		{
			float offs;
			offs = ptype->texsstride * (rand()%ptype->randsmax);
			p->s1 += offs;
			p->s2 += offs;
			while (p->s1 >= 1)
			{
				p->s1 -= 1;
				p->s2 -= 1;
				p->t1 += ptype->texsstride;
				p->t2 += ptype->texsstride;
			}
		}

		if (len < nrfirst || len >= nrlast)
		{
			// no offset or areaspread for these particles...
			p->vel[0] = vec[0]*veladd;
			p->vel[1] = vec[1]*veladd;
			p->vel[2] = vec[2]*veladd;

			VectorCopy(start, p->org);
		}
		else
		{
			switch(ptype->spawnmode)
			{
			case SM_TRACER:
				if (tcount & 1)
				{
					p->vel[0] = vec[1]*ptype->spawnvel;
					p->vel[1] = -vec[0]*ptype->spawnvel;
					p->org[0] = vec[1]*ptype->areaspread;
					p->org[1] = -vec[0]*ptype->areaspread;
				}
				else
				{
					p->vel[0] = -vec[1]*ptype->spawnvel;
					p->vel[1] = vec[0]*ptype->spawnvel;
					p->org[0] = -vec[1]*ptype->areaspread;
					p->org[1] = vec[0]*ptype->areaspread;
				}

				p->vel[0] += vec[0]*veladd;
				p->vel[1] += vec[1]*veladd;
				p->vel[2] = vec[2]*veladd;

				p->org[0] += start[0];
				p->org[1] += start[1];
				p->org[2] = start[2];
				break;
			case SM_SPIRAL:
				{
					float tsin, tcos;
					float tright, tup;

					tcos = cos(len*tdegree+sdegree);
					tsin = sin(len*tdegree+sdegree);

					tright = tcos*ptype->areaspread;
					tup = tsin*ptype->areaspread;

					p->org[0] = start[0] + right[0]*tright + up[0]*tup;
					p->org[1] = start[1] + right[1]*tright + up[1]*tup;
					p->org[2] = start[2] + right[2]*tright + up[2]*tup;

					tright = tcos*ptype->spawnvel;
					tup = tsin*ptype->spawnvel;

					p->vel[0] = vec[0]*veladd + right[0]*tright + up[0]*tup;
					p->vel[1] = vec[1]*veladd + right[1]*tright + up[1]*tup;
					p->vel[2] = vec[2]*veladd + right[2]*tright + up[2]*tup;
				}
				break;
			// TODO: directionalize SM_BALL/SM_CIRCLE/SM_DISTBALL
			case SM_BALL:
				p->org[0] = crandom();
				p->org[1] = crandom();
				p->org[2] = crandom();
				VectorNormalize(p->org);
				VectorScale(p->org, frandom(), p->org);

				p->vel[0] = vec[0]*veladd + p->org[0]*ptype->spawnvel;
				p->vel[1] = vec[1]*veladd + p->org[1]*ptype->spawnvel;
				p->vel[2] = vec[2]*veladd + p->org[2]*ptype->spawnvelvert;

				p->org[0] = p->org[0]*ptype->areaspread + start[0];
				p->org[1] = p->org[1]*ptype->areaspread + start[1];
				p->org[2] = p->org[2]*ptype->areaspreadvert + start[2];
				break;

			case SM_CIRCLE:
				{
					float tsin, tcos;

					tcos = cos(len*tdegree)*ptype->areaspread;
					tsin = sin(len*tdegree)*ptype->areaspread;

					p->org[0] = start[0] + right[0]*tcos + up[0]*tsin + vstep[0] * (len*tdegree);
					p->org[1] = start[1] + right[1]*tcos + up[1]*tsin + vstep[1] * (len*tdegree);
					p->org[2] = start[2] + right[2]*tcos + up[2]*tsin + vstep[2] * (len*tdegree)*50;

					tcos = cos(len*tdegree)*ptype->spawnvel;
					tsin = sin(len*tdegree)*ptype->spawnvel;

					p->vel[0] = vec[0]*veladd + right[0]*tcos + up[0]*tsin;
					p->vel[1] = vec[1]*veladd + right[1]*tcos + up[1]*tsin;
					p->vel[2] = vec[2]*veladd + right[2]*tcos + up[2]*tsin;
				}
				break;

			case SM_DISTBALL:
				{
					float rdist;

					rdist = ptype->spawnparam2 - crandom()*(1-(crandom() * ptype->spawnparam1));

					// this is a strange spawntype, which is based on the fact that
					// crandom()*crandom() provides something similar to an exponential
					// probability curve
					p->org[0] = crandom();
					p->org[1] = crandom();
					p->org[2] = crandom();

					VectorNormalize(p->org);
					VectorScale(p->org, rdist, p->org);

					p->vel[0] = vec[0]*veladd + p->org[0]*ptype->spawnvel;
					p->vel[1] = vec[1]*veladd + p->org[1]*ptype->spawnvel;
					p->vel[2] = vec[2]*veladd + p->org[2]*ptype->spawnvelvert;

					p->org[0] = p->org[0]*ptype->areaspread + start[0];
					p->org[1] = p->org[1]*ptype->areaspread + start[1];
					p->org[2] = p->org[2]*ptype->areaspreadvert + start[2];
				}
				break;
			default:
				p->org[0] = crandom();
				p->org[1] = crandom();
				p->org[2] = crandom();

				p->vel[0] = vec[0]*veladd + p->org[0]*ptype->spawnvel;
				p->vel[1] = vec[1]*veladd + p->org[1]*ptype->spawnvel;
				p->vel[2] = vec[2]*veladd + p->org[2]*ptype->spawnvelvert;

				p->org[0] = p->org[0]*ptype->areaspread + start[0];
				p->org[1] = p->org[1]*ptype->areaspread + start[1];
				p->org[2] = p->org[2]*ptype->areaspreadvert + start[2];
				break;
			}

			if (ptype->orgadd)
			{
				p->org[0] += vec[0]*ptype->orgadd;
				p->org[1] += vec[1]*ptype->orgadd;
				p->org[2] += vec[2]*ptype->orgadd;
			}
		}
		if (ptype->flags & PT_WORLDSPACERAND)
		{
			vec3_t vtmp;
			do
			{
				vtmp[0] = crand();
				vtmp[1] = crand();
				vtmp[2] = crand();
			} while(DotProduct(vtmp,vtmp)>1);	//crap, but I'm trying to mimic dp
			p->org[0] += vtmp[0] * ptype->orgwrand[0];
			p->org[1] += vtmp[1] * ptype->orgwrand[1];
			p->org[2] += vtmp[2] * ptype->orgwrand[2];
			p->vel[0] += vtmp[0] * ptype->velwrand[0];
			p->vel[1] += vtmp[1] * ptype->velwrand[1];
			p->vel[2] += vtmp[2] * ptype->velwrand[2];
			VectorAdd(p->vel, ptype->velbias, p->vel);
		}
		VectorAdd(p->org, ptype->orgbias, p->org);

		VectorAdd (start, vstep, start);

		if (ptype->countrand)
		{
			float rstep = frandom() / ptype->countrand;
			VectorMA(start, rstep, vec, start);
			step += rstep;
		}

		p->die = particletime + ptype->die - p->die;
		VectorCopy(p->org, p->oldorg);
	}

	if (ts)
	{
		ts->trail.lastdist = len;

		// update beamseg list
		if (ptype->looks.type == PT_BEAM||ptype->looks.type == PT_VBEAM)
		{
			if (b)
			{
				if (ptype->beams)
				{
					if (ts->lastbeam)
					{
						b->next = ts->lastbeam->next;
						ts->lastbeam->next = bfirst;
						ts->lastbeam->flags &= ~BS_LASTSEG;
					}
					else
					{
						b->next = ptype->beams;
						ptype->beams = bfirst;
					}
				}
				else
				{
					ptype->beams = bfirst;
					b->next = NULL;
				}

				b->flags |= BS_LASTSEG;
				ts->lastbeam = b;
			}

			if ((!free_particles || !free_beams) && ts->lastbeam)
			{
				ts->lastbeam->flags &= ~BS_LASTSEG;
				ts->lastbeam->flags |= BS_NODRAW;
				ts->lastbeam = NULL;
			}
		}
	}
	else if (ptype->looks.type == PT_BEAM||ptype->looks.type == PT_VBEAM)
	{
		if (b)
		{
			b->flags |= BS_NODRAW;
			b->next = ptype->beams;
			ptype->beams = bfirst;
		}
	}

	// maintain run list
	if (!(ptype->state & PS_INRUNLIST))
	{
		ptype->runlink = &part_run_list;
		ptype->nexttorun = part_run_list;
		if (part_run_list)
			part_run_list->runlink = &ptype->nexttorun;
		*ptype->runlink = ptype;
		ptype->state |= PS_INRUNLIST;
	}

	return;
}

static int PScript_ParticleTrail (vec3_t startpos, vec3_t end, int type, float timeinterval, int dlkey, vec3_t axis[3], trailkey_t *tk)
{
	part_type_t *ptype = &part_type[type];

	if (type >= FALLBACKBIAS && fallback)
	{
		// this will cause problems if dealing with fallbacks that actually 
		// allocate their own trail state, but for now this should suffice
		//
		// also reusing fallback space for emit/trail info will cause some
		// issues with entities in action during particle reconfiguration 
		// but that shouldn't be happening too often
		trailstate_t* ts = P_FetchTrailstate(tk);
		return fallback->ParticleTrail(startpos, end, type - FALLBACKBIAS, timeinterval, dlkey, axis, ts?&(ts->fallbackkey):NULL);
	}

	if (type < 0 || type >= numparticletypes)
		return 1;	//bad value

	if (!ptype->loaded)
		return 1;

	// inwater check, switch only once
	if (r_part_contentswitch.ival && ptype->inwater >= 0 && cl.worldmodel && cl.worldmodel->loadstate == MLS_LOADED)
	{
		int cont;
		cont = cl.worldmodel->funcs.PointContents(cl.worldmodel, NULL, startpos);

		if (cont & FTECONTENTS_FLUID)
			ptype = &part_type[ptype->inwater];
	}

	P_ParticleTrailSpawn (startpos, end, ptype, timeinterval, tk, dlkey, axis);
	return 0;
}

static void PScript_ParticleTrailIndex (vec3_t start, vec3_t end, int type, float timeinterval, int color, int crnd, trailkey_t *tk)
{
	if (type == P_INVALID)
		type = pe_defaulttrail;
	if (PART_VALID(type))
	{
		part_type[type].colorindex = color;
		part_type[type].colorrand = crnd;
		P_ParticleTrail(start, end, type, timeinterval, 0, NULL, tk);
	}
}

static vec3_t pright, pup;
static float pframetime;

static void GL_DrawTexturedParticle(int count, particle_t **plist, plooks_t *type)
{
	particle_t *p;
	float x,y;
	float scale;

	while (count--)
	{
		p = *plist++;

		if (pscriptmesh.numvertexes >= BUFFERVERTS-4)
		{
			pscriptmesh.numindexes = pscriptmesh.numvertexes/4*6;
			BE_DrawMesh_Single(type->shader, &pscriptmesh, NULL, 0);
			pscriptmesh.numvertexes = 0;
		}

		if (type->scalefactor == 1)
			scale = p->scale*0.25;
		else
		{
			scale = (p->org[0] - r_origin[0])*vpn[0] + (p->org[1] - r_origin[1])*vpn[1]
				+ (p->org[2] - r_origin[2])*vpn[2];
			scale = (scale*p->scale)*(type->invscalefactor) + p->scale * (type->scalefactor*250);
			if (scale < 20)
				scale = 0.25;
			else
				scale = 0.25 + scale * 0.001;
		}

		Vector4Copy(p->rgba, pscriptcolours[pscriptmesh.numvertexes+0]);
		Vector4Copy(p->rgba, pscriptcolours[pscriptmesh.numvertexes+1]);
		Vector4Copy(p->rgba, pscriptcolours[pscriptmesh.numvertexes+2]);
		Vector4Copy(p->rgba, pscriptcolours[pscriptmesh.numvertexes+3]);

		Vector2Set(pscripttexcoords[pscriptmesh.numvertexes+0], p->s1, p->t1);
		Vector2Set(pscripttexcoords[pscriptmesh.numvertexes+1], p->s1, p->t2);
		Vector2Set(pscripttexcoords[pscriptmesh.numvertexes+2], p->s2, p->t2);
		Vector2Set(pscripttexcoords[pscriptmesh.numvertexes+3], p->s2, p->t1);

		if (p->angle)
		{
			x = sin(p->angle)*scale;
			y = cos(p->angle)*scale;

			pscriptverts[pscriptmesh.numvertexes+0][0] = p->org[0] - x*pright[0] - y*pup[0];
			pscriptverts[pscriptmesh.numvertexes+0][1] = p->org[1] - x*pright[1] - y*pup[1];
			pscriptverts[pscriptmesh.numvertexes+0][2] = p->org[2] - x*pright[2] - y*pup[2];
			pscriptverts[pscriptmesh.numvertexes+1][0] = p->org[0] - y*pright[0] + x*pup[0];
			pscriptverts[pscriptmesh.numvertexes+1][1] = p->org[1] - y*pright[1] + x*pup[1];
			pscriptverts[pscriptmesh.numvertexes+1][2] = p->org[2] - y*pright[2] + x*pup[2];
			pscriptverts[pscriptmesh.numvertexes+2][0] = p->org[0] + x*pright[0] + y*pup[0];
			pscriptverts[pscriptmesh.numvertexes+2][1] = p->org[1] + x*pright[1] + y*pup[1];
			pscriptverts[pscriptmesh.numvertexes+2][2] = p->org[2] + x*pright[2] + y*pup[2];
			pscriptverts[pscriptmesh.numvertexes+3][0] = p->org[0] + y*pright[0] - x*pup[0];
			pscriptverts[pscriptmesh.numvertexes+3][1] = p->org[1] + y*pright[1] - x*pup[1];
			pscriptverts[pscriptmesh.numvertexes+3][2] = p->org[2] + y*pright[2] - x*pup[2];
		}
		else
		{
			VectorMA(p->org, -scale, pup, pscriptverts[pscriptmesh.numvertexes+0]);
			VectorMA(p->org, -scale, pright, pscriptverts[pscriptmesh.numvertexes+1]);
			VectorMA(p->org, scale, pup, pscriptverts[pscriptmesh.numvertexes+2]);
			VectorMA(p->org, scale, pright, pscriptverts[pscriptmesh.numvertexes+3]);
		}
		pscriptmesh.numvertexes += 4;
	}

	if (pscriptmesh.numvertexes)
	{
		pscriptmesh.numindexes = pscriptmesh.numvertexes/4*6;
		BE_DrawMesh_Single(type->shader, &pscriptmesh, NULL, 0);
		pscriptmesh.numvertexes = 0;
	}
}

static void GL_DrawTrifanParticle(int count, particle_t **plist, plooks_t *type)
{
	particle_t *p;
	vec3_t v, cr, o2;
	float scale;

	while (count--)
	{
		p = *plist++;

		if (pscripttmesh.numvertexes >= BUFFERVERTS-3)
		{
			pscripttmesh.numindexes = pscripttmesh.numvertexes;
			BE_DrawMesh_Single(type->shader, &pscripttmesh, NULL, 0);
			pscripttmesh.numvertexes = 0;
		}

		scale = (p->org[0] - r_origin[0])*vpn[0] + (p->org[1] - r_origin[1])*vpn[1]
			+ (p->org[2] - r_origin[2])*vpn[2];
		scale = (scale*p->scale)*(type->invscalefactor) + p->scale * (type->scalefactor*250);
		if (scale < 20)
			scale = 0.05;
		else
			scale = 0.05 + scale * 0.0001;

		Vector4Copy(p->rgba, pscriptcolours[pscripttmesh.numvertexes+0]);
		Vector4Copy(p->rgba, pscriptcolours[pscripttmesh.numvertexes+1]);
		Vector4Copy(p->rgba, pscriptcolours[pscripttmesh.numvertexes+2]);

		Vector2Set(pscripttexcoords[pscripttmesh.numvertexes+0], p->s1, p->t1);
		Vector2Set(pscripttexcoords[pscripttmesh.numvertexes+1], p->s1, p->t2);
		Vector2Set(pscripttexcoords[pscripttmesh.numvertexes+2], p->s2, p->t1);


		VectorMA(p->org, -scale, p->vel, o2);
		VectorSubtract(r_refdef.vieworg, o2, v);
		CrossProduct(v, p->vel, cr);
		VectorNormalize(cr);

		VectorCopy(p->org, pscriptverts[pscripttmesh.numvertexes+0]);
		VectorMA(o2, -p->scale, cr, pscriptverts[pscripttmesh.numvertexes+1]);
		VectorMA(o2, p->scale, cr, pscriptverts[pscripttmesh.numvertexes+2]);

		pscripttmesh.numvertexes += 3;
	}

	if (pscripttmesh.numvertexes)
	{
		pscripttmesh.numindexes = pscripttmesh.numvertexes;
		BE_DrawMesh_Single(type->shader, &pscripttmesh, NULL, 0);
		pscripttmesh.numvertexes = 0;
	}
}

//static void R_AddLineSparkParticle(int count, particle_t **plist, plooks_t *type)
static void R_AddLineSparkParticle(scenetris_t *t, particle_t *p, plooks_t *type)
{
	if (cl_numstrisvert+2 > cl_maxstrisvert)
		cl_stris_ExpandVerts(cl_maxstrisvert+64*2);

	Vector4Copy(p->rgba, cl_strisvertc[cl_numstrisvert+0]);
	VectorCopy(p->rgba, cl_strisvertc[cl_numstrisvert+1]);
	cl_strisvertc[cl_numstrisvert+1][3] = 0;
	Vector2Set(cl_strisvertt[cl_numstrisvert+0], p->s1, p->t1);
	Vector2Set(cl_strisvertt[cl_numstrisvert+1], p->s2, p->t2);

	VectorCopy(p->org, cl_strisvertv[cl_numstrisvert+0]);
	VectorMA(p->org, -1.0/10, p->vel, cl_strisvertv[cl_numstrisvert+1]);
	
	if (cl_numstrisidx+2 > cl_maxstrisidx)
	{
		cl_maxstrisidx += 64*2;
		cl_strisidx = BZ_Realloc(cl_strisidx, sizeof(*cl_strisidx)*cl_maxstrisidx);
	}
	cl_strisidx[cl_numstrisidx++] = (cl_numstrisvert - t->firstvert) + 0;
	cl_strisidx[cl_numstrisidx++] = (cl_numstrisvert - t->firstvert) + 1;

	cl_numstrisvert += 2;

	t->numvert += 2;
	t->numidx += 2;
}

static void R_AddTSparkParticle(scenetris_t *t, particle_t *p, plooks_t *type)
{
	vec3_t v, cr, o2;
//	float scale;

	if (cl_numstrisvert+4 > cl_maxstrisvert)
		cl_stris_ExpandVerts(cl_maxstrisvert+64*4);

/*	if (type->scalefactor == 1)
		scale = p->scale*0.25;
	else
	{
		scale = (p->org[0] - r_origin[0])*vpn[0] + (p->org[1] - r_origin[1])*vpn[1]
			+ (p->org[2] - r_origin[2])*vpn[2];
		scale = (scale*p->scale)*(type->invscalefactor) + p->scale * (type->scalefactor*250);
		if (scale < 20)
			scale = 0.25;
		else
			scale = 0.25 + scale * 0.001;
	}
*/
	if (type->premul)
	{
		vec4_t rgba;
		float a = p->rgba[3];
		if (a > 1)
			a = 1;
		rgba[0] = p->rgba[0] * a;
		rgba[1] = p->rgba[1] * a;
		rgba[2] = p->rgba[2] * a;
		rgba[3] = (type->premul==2)?0:a;
		Vector4Copy(rgba, cl_strisvertc[cl_numstrisvert+0]);
		Vector4Copy(rgba, cl_strisvertc[cl_numstrisvert+1]);
		Vector4Copy(rgba, cl_strisvertc[cl_numstrisvert+2]);
		Vector4Copy(rgba, cl_strisvertc[cl_numstrisvert+3]);
	}
	else
	{
		Vector4Copy(p->rgba, cl_strisvertc[cl_numstrisvert+0]);
		Vector4Copy(p->rgba, cl_strisvertc[cl_numstrisvert+1]);
		Vector4Copy(p->rgba, cl_strisvertc[cl_numstrisvert+2]);
		Vector4Copy(p->rgba, cl_strisvertc[cl_numstrisvert+3]);
	}

	Vector2Set(cl_strisvertt[cl_numstrisvert+0], p->s1, p->t1);
	Vector2Set(cl_strisvertt[cl_numstrisvert+1], p->s1, p->t2);
	Vector2Set(cl_strisvertt[cl_numstrisvert+2], p->s2, p->t2);
	Vector2Set(cl_strisvertt[cl_numstrisvert+3], p->s2, p->t1);


/*	if (1)
	{
		vec3_t movedir;
		float length = VectorNormalize2(p->vel, movedir);
		length *= type->stretch;
		if (length < p->scale * 0.25)
			length = p->scale * 0.25;
		VectorMA(p->org, -length, movedir, o2);

		VectorSubtract(r_refdef.vieworg, o2, v);
		CrossProduct(v, p->vel, cr);
		VectorNormalize(cr);
		VectorMA(o2, -p->scale*0.5, cr, cl_strisvertv[cl_numstrisvert+0]);
		VectorMA(o2, p->scale*0.5, cr, cl_strisvertv[cl_numstrisvert+1]);

		VectorMA(p->org, length, movedir, o2);
	}
	else*/
	if (1/*type->stretch < 0*/)
	{
		vec3_t movedir;
		float halfscale = p->scale*0.5;
		float length = VectorNormalize2(p->vel, movedir);
		if (type->stretch < 0)
			length = -type->stretch;	//fixed lengths
		else if (type->stretch)
			length *= type->stretch;	//velocity multiplier
		else
			Sys_Error("type->stretch should be 0.05\n");
//			length *= 0.05;				//fallback

		if (length < halfscale * type->minstretch)
			length = halfscale * type->minstretch;

		VectorMA(p->org, -length, movedir, o2);
		VectorSubtract(r_refdef.vieworg, o2, v);
		CrossProduct(v, p->vel, cr);
		VectorNormalize(cr);

		halfscale = fabs(p->scale);	//gah, I hate dp.
		VectorMA(o2, -halfscale/2, cr, cl_strisvertv[cl_numstrisvert+0]);
		VectorMA(o2, halfscale/2, cr, cl_strisvertv[cl_numstrisvert+1]);

		VectorMA(p->org, length, movedir, o2);
	}
	else if (type->stretch)
	{
		VectorMA(p->org, -type->stretch, p->vel, o2);
		VectorSubtract(r_refdef.vieworg, o2, v);
		
		CrossProduct(v, p->vel, cr);
		VectorNormalize(cr);

		VectorMA(o2, -p->scale/2, cr, cl_strisvertv[cl_numstrisvert+0]);
		VectorMA(o2, p->scale/2, cr, cl_strisvertv[cl_numstrisvert+1]);

		VectorMA(p->org, type->stretch, p->vel, o2);
	}
	else
	{
		VectorMA(p->org, 0.1, p->vel, o2);
		VectorSubtract(r_refdef.vieworg, p->org, v);


		CrossProduct(v, p->vel, cr);
		VectorNormalize(cr);

		VectorMA(p->org, -p->scale/2, cr, cl_strisvertv[cl_numstrisvert+0]);
		VectorMA(p->org, p->scale/2, cr, cl_strisvertv[cl_numstrisvert+1]);
	}
	VectorSubtract(r_refdef.vieworg, o2, v);
	CrossProduct(v, p->vel, cr);
	VectorNormalize(cr);

	VectorMA(o2, p->scale*0.5, cr, cl_strisvertv[cl_numstrisvert+2]);
	VectorMA(o2, -p->scale*0.5, cr, cl_strisvertv[cl_numstrisvert+3]);



	if (cl_numstrisidx+6 > cl_maxstrisidx)
	{
		cl_maxstrisidx += 64*6;
		cl_strisidx = BZ_Realloc(cl_strisidx, sizeof(*cl_strisidx)*cl_maxstrisidx);
	}
	cl_strisidx[cl_numstrisidx++] = (cl_numstrisvert - t->firstvert) + 0;
	cl_strisidx[cl_numstrisidx++] = (cl_numstrisvert - t->firstvert) + 1;
	cl_strisidx[cl_numstrisidx++] = (cl_numstrisvert - t->firstvert) + 2;
	cl_strisidx[cl_numstrisidx++] = (cl_numstrisvert - t->firstvert) + 0;
	cl_strisidx[cl_numstrisidx++] = (cl_numstrisvert - t->firstvert) + 2;
	cl_strisidx[cl_numstrisidx++] = (cl_numstrisvert - t->firstvert) + 3;

	cl_numstrisvert += 4;

	t->numvert += 4;
	t->numidx += 6;
}

static void GL_DrawTexturedSparkParticle(int count, particle_t **plist, plooks_t *type)
{
	particle_t *p;
	vec3_t v, cr, o2;

	while (count--)
	{
		p = *plist++;

		if (pscriptmesh.numvertexes >= BUFFERVERTS-4)
		{
			pscriptmesh.numindexes = pscriptmesh.numvertexes/4*6;
			BE_DrawMesh_Single(type->shader, &pscriptmesh, NULL, 0);
			pscriptmesh.numvertexes = 0;
		}

		Vector4Copy(p->rgba, pscriptcolours[pscriptmesh.numvertexes+0]);
		Vector4Copy(p->rgba, pscriptcolours[pscriptmesh.numvertexes+1]);
		Vector4Copy(p->rgba, pscriptcolours[pscriptmesh.numvertexes+2]);
		Vector4Copy(p->rgba, pscriptcolours[pscriptmesh.numvertexes+3]);

		Vector2Set(pscripttexcoords[pscriptmesh.numvertexes+0], p->s1, p->t1);
		Vector2Set(pscripttexcoords[pscriptmesh.numvertexes+1], p->s1, p->t2);
		Vector2Set(pscripttexcoords[pscriptmesh.numvertexes+2], p->s2, p->t2);
		Vector2Set(pscripttexcoords[pscriptmesh.numvertexes+3], p->s2, p->t1);


		if (type->stretch)
		{
			VectorMA(p->org, type->stretch, p->vel, o2);
			VectorMA(p->org, -type->stretch, p->vel, v);
			VectorSubtract(r_refdef.vieworg, v, v);
		}
		else
		{
			VectorMA(p->org, 0.1, p->vel, o2);
			VectorSubtract(r_refdef.vieworg, p->org, v);
		}

		CrossProduct(v, p->vel, cr);
		VectorNormalize(cr);

		VectorMA(p->org, -p->scale/2, cr, pscriptverts[pscriptmesh.numvertexes+0]);
		VectorMA(p->org, p->scale/2, cr, pscriptverts[pscriptmesh.numvertexes+1]);

		VectorSubtract(r_refdef.vieworg, o2, v);
		CrossProduct(v, p->vel, cr);
		VectorNormalize(cr);

		VectorMA(o2, p->scale/2, cr, pscriptverts[pscriptmesh.numvertexes+2]);
		VectorMA(o2, -p->scale/2, cr, pscriptverts[pscriptmesh.numvertexes+3]);

		pscriptmesh.numvertexes += 4;
	}

	if (pscriptmesh.numvertexes)
	{
		pscriptmesh.numindexes = pscriptmesh.numvertexes/4*6;
		BE_DrawMesh_Single(type->shader, &pscriptmesh, NULL, 0);
		pscriptmesh.numvertexes = 0;
	}
}


static void GL_DrawParticleVBeam(int count, beamseg_t **blist, plooks_t *type)
{
	beamseg_t *b;
	vec3_t v;
	vec3_t cr;
	beamseg_t *c;
	particle_t *p;
	particle_t *q;
	float ts;

	while(count--)
	{
		b = *blist++;

		if (pscriptmesh.numvertexes >= BUFFERVERTS-4)
		{
			pscriptmesh.numindexes = pscriptmesh.numvertexes/4*6;
			BE_DrawMesh_Single(type->shader, &pscriptmesh, NULL, 0);
			pscriptmesh.numvertexes = 0;
		}

		c = b->next;

		q = c->p;
		if (!q)
			continue;
		p = b->p;

//		q->rgba[3] = 1;
//		p->rgba[3] = 1;

		VectorSubtract(r_refdef.vieworg, q->org, v);
		VectorNormalize(v);
		CrossProduct(c->dir, v, cr);
		VectorNormalize(cr);
		ts = c->texture_s*q->angle + particletime*q->rotationspeed;
		Vector4Copy(q->rgba, pscriptcolours[pscriptmesh.numvertexes+0]);
		Vector4Copy(q->rgba, pscriptcolours[pscriptmesh.numvertexes+1]);
		Vector2Set(pscripttexcoords[pscriptmesh.numvertexes+0], p->s1, p->t1);
		Vector2Set(pscripttexcoords[pscriptmesh.numvertexes+1], p->s2, p->t2);
		VectorMA(q->org, -q->scale, cr, pscriptverts[pscriptmesh.numvertexes+0]);
		VectorMA(q->org, q->scale, cr, pscriptverts[pscriptmesh.numvertexes+1]);

		VectorSubtract(r_refdef.vieworg, p->org, v);
		VectorNormalize(v);
		CrossProduct(b->dir, v, cr); // replace with old p->dir?
		VectorNormalize(cr);
		ts = b->texture_s*p->angle + particletime*p->rotationspeed;
		(void)ts;
		Vector4Copy(p->rgba, pscriptcolours[pscriptmesh.numvertexes+2]);
		Vector4Copy(p->rgba, pscriptcolours[pscriptmesh.numvertexes+3]);
		Vector2Set(pscripttexcoords[pscriptmesh.numvertexes+2], p->s1, p->t1);
		Vector2Set(pscripttexcoords[pscriptmesh.numvertexes+3], p->s2, p->t2);
		VectorMA(p->org, p->scale, cr, pscriptverts[pscriptmesh.numvertexes+2]);
		VectorMA(p->org, -p->scale, cr, pscriptverts[pscriptmesh.numvertexes+3]);

		pscriptmesh.numvertexes += 4;
	}

	if (pscriptmesh.numvertexes)
	{
		pscriptmesh.numindexes = pscriptmesh.numvertexes/4*6;
		BE_DrawMesh_Single(type->shader, &pscriptmesh, NULL, 0);
		pscriptmesh.numvertexes = 0;
	}
}
static void GL_DrawParticleBeam(int count, beamseg_t **blist, plooks_t *type)
{
	beamseg_t *b;
	vec3_t v;
	vec3_t cr;
	beamseg_t *c;
	particle_t *p;
	particle_t *q;
	float ts;

	while(count--)
	{
		b = *blist++;

		if (pscriptmesh.numvertexes >= BUFFERVERTS-4)
		{
			pscriptmesh.numindexes = pscriptmesh.numvertexes/4*6;
			BE_DrawMesh_Single(type->shader, &pscriptmesh, NULL, 0);
			pscriptmesh.numvertexes = 0;
		}

		c = b->next;

		q = c->p;
		if (!q)
			continue;
		p = b->p;

//		q->rgba[3] = 1;
//		p->rgba[3] = 1;

		VectorSubtract(r_refdef.vieworg, q->org, v);
		VectorNormalize(v);
		CrossProduct(c->dir, v, cr);
		VectorNormalize(cr);
		ts = c->texture_s*q->angle + particletime*q->rotationspeed;
		Vector4Copy(q->rgba, pscriptcolours[pscriptmesh.numvertexes+0]);
		Vector4Copy(q->rgba, pscriptcolours[pscriptmesh.numvertexes+1]);
		Vector2Set(pscripttexcoords[pscriptmesh.numvertexes+0], ts, p->t1);
		Vector2Set(pscripttexcoords[pscriptmesh.numvertexes+1], ts, p->t2);
		VectorMA(q->org, -q->scale, cr, pscriptverts[pscriptmesh.numvertexes+0]);
		VectorMA(q->org, q->scale, cr, pscriptverts[pscriptmesh.numvertexes+1]);

		VectorSubtract(r_refdef.vieworg, p->org, v);
		VectorNormalize(v);
		CrossProduct(b->dir, v, cr); // replace with old p->dir?
		VectorNormalize(cr);
		ts = b->texture_s*p->angle + particletime*p->rotationspeed;
		Vector4Copy(p->rgba, pscriptcolours[pscriptmesh.numvertexes+2]);
		Vector4Copy(p->rgba, pscriptcolours[pscriptmesh.numvertexes+3]);
		Vector2Set(pscripttexcoords[pscriptmesh.numvertexes+2], ts, p->t2);
		Vector2Set(pscripttexcoords[pscriptmesh.numvertexes+3], ts, p->t1);
		VectorMA(p->org, p->scale, cr, pscriptverts[pscriptmesh.numvertexes+2]);
		VectorMA(p->org, -p->scale, cr, pscriptverts[pscriptmesh.numvertexes+3]);

		pscriptmesh.numvertexes += 4;
	}

	if (pscriptmesh.numvertexes)
	{
		pscriptmesh.numindexes = pscriptmesh.numvertexes/4*6;
		BE_DrawMesh_Single(type->shader, &pscriptmesh, NULL, 0);
		pscriptmesh.numvertexes = 0;
	}
}

static void R_AddClippedDecal(scenetris_t *t, clippeddecal_t *d, plooks_t *type)
{
	if (cl_numstrisvert+4 > cl_maxstrisvert)
		cl_stris_ExpandVerts(cl_maxstrisvert+64*4);

	if (d->entity > 0)
	{
		lerpents_t *le = cl.lerpents+d->entity;
		if (le->sequence != cl.lerpentssequence)
			return;	//hide until its visible again.
		if (le->angles[0] || le->angles[1] || le->angles[2])
		{	//FIXME: deal with rotated entities.
			d->die = -1;
			return;
		}
		VectorAdd(d->vertex[0], le->origin, cl_strisvertv[cl_numstrisvert+0]);
		VectorAdd(d->vertex[1], le->origin, cl_strisvertv[cl_numstrisvert+1]);
		VectorAdd(d->vertex[2], le->origin, cl_strisvertv[cl_numstrisvert+2]);
	}
	else if (d->entity < 0)
	{
		wedict_t *e = WEDICT_NUM_UB(csqc_world.progs, -d->entity);
		if (!e)
			return;
		if (e->ereftype!=ER_ENTITY ||	//kill decals attached to removed ents.
			e->v->angles[0] || e->v->angles[1] || e->v->angles[2])	//FIXME: deal with rotated entities.
		{
			d->die = -1;
			return;
		}
		VectorAdd(d->vertex[0], e->v->origin, cl_strisvertv[cl_numstrisvert+0]);
		VectorAdd(d->vertex[1], e->v->origin, cl_strisvertv[cl_numstrisvert+1]);
		VectorAdd(d->vertex[2], e->v->origin, cl_strisvertv[cl_numstrisvert+2]);
	}
	else
	{
		VectorCopy(d->vertex[0], cl_strisvertv[cl_numstrisvert+0]);
		VectorCopy(d->vertex[1], cl_strisvertv[cl_numstrisvert+1]);
		VectorCopy(d->vertex[2], cl_strisvertv[cl_numstrisvert+2]);
	}

	if (type->premul)
	{
		vec4_t rgba;
		float a = d->rgba[3];
		if (a > 1)
			a = 1;
		rgba[0] = d->rgba[0] * a;
		rgba[1] = d->rgba[1] * a;
		rgba[2] = d->rgba[2] * a;
		rgba[3] = (type->premul==2)?0:a;
		Vector4Scale(rgba, d->valpha[0], cl_strisvertc[cl_numstrisvert+0]);
		Vector4Scale(rgba, d->valpha[1], cl_strisvertc[cl_numstrisvert+1]);
		Vector4Scale(rgba, d->valpha[2], cl_strisvertc[cl_numstrisvert+2]);
	}
	else
	{
		Vector4Copy(d->rgba, cl_strisvertc[cl_numstrisvert+0]);
		cl_strisvertc[cl_numstrisvert+0][3] *= d->valpha[0];
		Vector4Copy(d->rgba, cl_strisvertc[cl_numstrisvert+1]);
		cl_strisvertc[cl_numstrisvert+1][3] *= d->valpha[1];
		Vector4Copy(d->rgba, cl_strisvertc[cl_numstrisvert+2]);
		cl_strisvertc[cl_numstrisvert+2][3] *= d->valpha[2];
	}

	Vector2Copy(d->texcoords[0], cl_strisvertt[cl_numstrisvert+0]);
	Vector2Copy(d->texcoords[1], cl_strisvertt[cl_numstrisvert+1]);
	Vector2Copy(d->texcoords[2], cl_strisvertt[cl_numstrisvert+2]);

	if (cl_numstrisidx+3 > cl_maxstrisidx)
	{
		cl_maxstrisidx += 64*3;
		cl_strisidx = BZ_Realloc(cl_strisidx, sizeof(*cl_strisidx)*cl_maxstrisidx);
	}
	cl_strisidx[cl_numstrisidx++] = (cl_numstrisvert - t->firstvert) + 0;
	cl_strisidx[cl_numstrisidx++] = (cl_numstrisvert - t->firstvert) + 1;
	cl_strisidx[cl_numstrisidx++] = (cl_numstrisvert - t->firstvert) + 2;

	cl_numstrisvert += 3;

	t->numvert += 3;
	t->numidx += 3;
}

static void R_AddUnclippedDecal(scenetris_t *t, particle_t *p, plooks_t *type)
{
	float x, y;
	vec3_t sdir, tdir;

	if (cl_numstrisvert+4 > cl_maxstrisvert)
		cl_stris_ExpandVerts(cl_maxstrisvert+64*4);

	if (type->premul)
	{
		vec4_t rgba;
		float a = p->rgba[3];
		if (a > 1)
			a = 1;
		rgba[0] = p->rgba[0] * a;
		rgba[1] = p->rgba[1] * a;
		rgba[2] = p->rgba[2] * a;
		rgba[3] = (type->premul==2)?0:a;
		Vector4Copy(rgba, cl_strisvertc[cl_numstrisvert+0]);
		Vector4Copy(rgba, cl_strisvertc[cl_numstrisvert+1]);
		Vector4Copy(rgba, cl_strisvertc[cl_numstrisvert+2]);
		Vector4Copy(rgba, cl_strisvertc[cl_numstrisvert+3]);
	}
	else
	{
		Vector4Copy(p->rgba, cl_strisvertc[cl_numstrisvert+0]);
		Vector4Copy(p->rgba, cl_strisvertc[cl_numstrisvert+1]);
		Vector4Copy(p->rgba, cl_strisvertc[cl_numstrisvert+2]);
		Vector4Copy(p->rgba, cl_strisvertc[cl_numstrisvert+3]);
	}

	Vector2Set(cl_strisvertt[cl_numstrisvert+0], p->s1, p->t1);
	Vector2Set(cl_strisvertt[cl_numstrisvert+1], p->s1, p->t2);
	Vector2Set(cl_strisvertt[cl_numstrisvert+2], p->s2, p->t2);
	Vector2Set(cl_strisvertt[cl_numstrisvert+3], p->s2, p->t1);

//	if (p->vel[1] == 1)
	{
		VectorSet(sdir, 1, 0, 0);
		VectorSet(tdir, 0, 1, 0);
	}

	if (p->angle)
	{
		x = sin(p->angle)*p->scale;
		y = cos(p->angle)*p->scale;

		cl_strisvertv[cl_numstrisvert+0][0] = p->org[0] - x*sdir[0] - y*tdir[0];
		cl_strisvertv[cl_numstrisvert+0][1] = p->org[1] - x*sdir[1] - y*tdir[1];
		cl_strisvertv[cl_numstrisvert+0][2] = p->org[2] - x*sdir[2] - y*tdir[2];
		cl_strisvertv[cl_numstrisvert+1][0] = p->org[0] - y*sdir[0] + x*tdir[0];
		cl_strisvertv[cl_numstrisvert+1][1] = p->org[1] - y*sdir[1] + x*tdir[1];
		cl_strisvertv[cl_numstrisvert+1][2] = p->org[2] - y*sdir[2] + x*tdir[2];
		cl_strisvertv[cl_numstrisvert+2][0] = p->org[0] + x*sdir[0] + y*tdir[0];
		cl_strisvertv[cl_numstrisvert+2][1] = p->org[1] + x*sdir[1] + y*tdir[1];
		cl_strisvertv[cl_numstrisvert+2][2] = p->org[2] + x*sdir[2] + y*tdir[2];
		cl_strisvertv[cl_numstrisvert+3][0] = p->org[0] + y*sdir[0] - x*tdir[0];
		cl_strisvertv[cl_numstrisvert+3][1] = p->org[1] + y*sdir[1] - x*tdir[1];
		cl_strisvertv[cl_numstrisvert+3][2] = p->org[2] + y*sdir[2] - x*tdir[2];
	}
	else
	{
		VectorMA(p->org, -p->scale, tdir, cl_strisvertv[cl_numstrisvert+0]);
		VectorMA(p->org, -p->scale, sdir, cl_strisvertv[cl_numstrisvert+1]);
		VectorMA(p->org, p->scale, tdir, cl_strisvertv[cl_numstrisvert+2]);
		VectorMA(p->org, p->scale, sdir, cl_strisvertv[cl_numstrisvert+3]);
	}

	if (cl_numstrisidx+6 > cl_maxstrisidx)
	{
		cl_maxstrisidx += 64*6;
		cl_strisidx = BZ_Realloc(cl_strisidx, sizeof(*cl_strisidx)*cl_maxstrisidx);
	}
	cl_strisidx[cl_numstrisidx++] = (cl_numstrisvert - t->firstvert) + 0;
	cl_strisidx[cl_numstrisidx++] = (cl_numstrisvert - t->firstvert) + 1;
	cl_strisidx[cl_numstrisidx++] = (cl_numstrisvert - t->firstvert) + 2;
	cl_strisidx[cl_numstrisidx++] = (cl_numstrisvert - t->firstvert) + 0;
	cl_strisidx[cl_numstrisidx++] = (cl_numstrisvert - t->firstvert) + 2;
	cl_strisidx[cl_numstrisidx++] = (cl_numstrisvert - t->firstvert) + 3;

	cl_numstrisvert += 4;

	t->numvert += 4;
	t->numidx += 6;
}

static void R_AddTexturedParticle(scenetris_t *t, particle_t *p, plooks_t *type)
{
	float scale, x, y;

	if (cl_numstrisvert+4 > cl_maxstrisvert)
		cl_stris_ExpandVerts(cl_maxstrisvert+64*4);

	if (type->scalefactor == 1)
		scale = p->scale*0.25;
	else
	{
		scale = (p->org[0] - r_origin[0])*vpn[0] + (p->org[1] - r_origin[1])*vpn[1]
			+ (p->org[2] - r_origin[2])*vpn[2];
		scale = (scale*p->scale)*(type->invscalefactor) + p->scale * (type->scalefactor*250);
		if (scale < 20)
			scale = 0.25;
		else
			scale = 0.25 + scale * 0.001;
	}

	if (type->premul)
	{
		vec4_t rgba;
		float a = p->rgba[3];
		if (a > 1)
			a = 1;
		rgba[0] = p->rgba[0] * a;
		rgba[1] = p->rgba[1] * a;
		rgba[2] = p->rgba[2] * a;
		rgba[3] = (type->premul==2)?0:a;
		Vector4Copy(rgba, cl_strisvertc[cl_numstrisvert+0]);
		Vector4Copy(rgba, cl_strisvertc[cl_numstrisvert+1]);
		Vector4Copy(rgba, cl_strisvertc[cl_numstrisvert+2]);
		Vector4Copy(rgba, cl_strisvertc[cl_numstrisvert+3]);
	}
	else
	{
		Vector4Copy(p->rgba, cl_strisvertc[cl_numstrisvert+0]);
		Vector4Copy(p->rgba, cl_strisvertc[cl_numstrisvert+1]);
		Vector4Copy(p->rgba, cl_strisvertc[cl_numstrisvert+2]);
		Vector4Copy(p->rgba, cl_strisvertc[cl_numstrisvert+3]);
	}

	Vector2Set(cl_strisvertt[cl_numstrisvert+0], p->s1, p->t1);
	Vector2Set(cl_strisvertt[cl_numstrisvert+1], p->s1, p->t2);
	Vector2Set(cl_strisvertt[cl_numstrisvert+2], p->s2, p->t2);
	Vector2Set(cl_strisvertt[cl_numstrisvert+3], p->s2, p->t1);

	if (p->angle)
	{
		x = sin(p->angle)*scale;
		y = cos(p->angle)*scale;

		cl_strisvertv[cl_numstrisvert+0][0] = p->org[0] - x*pright[0] - y*pup[0];
		cl_strisvertv[cl_numstrisvert+0][1] = p->org[1] - x*pright[1] - y*pup[1];
		cl_strisvertv[cl_numstrisvert+0][2] = p->org[2] - x*pright[2] - y*pup[2];
		cl_strisvertv[cl_numstrisvert+1][0] = p->org[0] - y*pright[0] + x*pup[0];
		cl_strisvertv[cl_numstrisvert+1][1] = p->org[1] - y*pright[1] + x*pup[1];
		cl_strisvertv[cl_numstrisvert+1][2] = p->org[2] - y*pright[2] + x*pup[2];
		cl_strisvertv[cl_numstrisvert+2][0] = p->org[0] + x*pright[0] + y*pup[0];
		cl_strisvertv[cl_numstrisvert+2][1] = p->org[1] + x*pright[1] + y*pup[1];
		cl_strisvertv[cl_numstrisvert+2][2] = p->org[2] + x*pright[2] + y*pup[2];
		cl_strisvertv[cl_numstrisvert+3][0] = p->org[0] + y*pright[0] - x*pup[0];
		cl_strisvertv[cl_numstrisvert+3][1] = p->org[1] + y*pright[1] - x*pup[1];
		cl_strisvertv[cl_numstrisvert+3][2] = p->org[2] + y*pright[2] - x*pup[2];
	}
	else
	{
		VectorMA(p->org, -scale, pup, cl_strisvertv[cl_numstrisvert+0]);
		VectorMA(p->org, -scale, pright, cl_strisvertv[cl_numstrisvert+1]);
		VectorMA(p->org, scale, pup, cl_strisvertv[cl_numstrisvert+2]);
		VectorMA(p->org, scale, pright, cl_strisvertv[cl_numstrisvert+3]);
	}

	if (cl_numstrisidx+6 > cl_maxstrisidx)
	{
		cl_maxstrisidx += 64*6;
		cl_strisidx = BZ_Realloc(cl_strisidx, sizeof(*cl_strisidx)*cl_maxstrisidx);
	}
	cl_strisidx[cl_numstrisidx++] = (cl_numstrisvert - t->firstvert) + 0;
	cl_strisidx[cl_numstrisidx++] = (cl_numstrisvert - t->firstvert) + 1;
	cl_strisidx[cl_numstrisidx++] = (cl_numstrisvert - t->firstvert) + 2;
	cl_strisidx[cl_numstrisidx++] = (cl_numstrisvert - t->firstvert) + 0;
	cl_strisidx[cl_numstrisidx++] = (cl_numstrisvert - t->firstvert) + 2;
	cl_strisidx[cl_numstrisidx++] = (cl_numstrisvert - t->firstvert) + 3;

	cl_numstrisvert += 4;

	t->numvert += 4;
	t->numidx += 6;
}

static void PScript_DrawParticleTypes (void)
{
	float viewtranslation[16];
	static float lastviewmatrix[16];
//	void (*sparklineparticles)(int count, particle_t **plist, plooks_t *type)=R_AddLineSparkParticle;
	void (*sparkfanparticles)(int count, particle_t **plist, plooks_t *type)=GL_DrawTrifanParticle;
	void (*sparktexturedparticles)(int count, particle_t **plist, plooks_t *type)=GL_DrawTexturedSparkParticle;

	void *pdraw, *bdraw;
	void (*tdraw)(scenetris_t *t, particle_t *p, plooks_t *type);

	vec3_t oldorg;
	vec3_t stop, normal;
	part_type_t *type;
	particle_t		*p, *kill;
	clippeddecal_t *d, *dkill;
	ramp_t *ramp;
	float grav;
	vec3_t friction;
	scenetris_t *scenetri;
	float dist;
	particle_t *kill_list, *kill_first;	//the kill list is to stop particles from being freed and reused whilst still in this loop
										//which is bad because beams need to find out when particles died. Reuse can do wierd things.
										//remember that they're not drawn instantly either.
	beamseg_t *b, *bkill;

	int traces=r_particle_tracelimit.ival;
	int rampind;
	static float oldtime;
	static float flurrytime;
	qboolean doflurry;
	int batchflags;
	int i;
	RSpeedMark();

	if (r_plooksdirty)
	{
		int i, j;
		{
			particleengine_t *tmp = fallback; fallback = NULL;

			pe_default			= PScript_FindParticleType("PE_DEFAULT");
			pe_size2			= PScript_FindParticleType("PE_SIZE2");
			pe_size3			= PScript_FindParticleType("PE_SIZE3");
			pe_defaulttrail		= PScript_FindParticleType("PE_DEFAULTTRAIL");

			if (pe_default == P_INVALID)
			{
			//pe_default			= PScript_FindParticleType("SVC_PARTICLE");
			}

			fallback = tmp;
		}

		for (i = 0; i < numparticletypes; i++)
		{
			//set the fallback
			part_type[i].slooks = &part_type[i].looks;
			for (j = i-1; j-- > 0;)
			{
				if (!memcmp(&part_type[i].looks, &part_type[j].looks, sizeof(plooks_t)))
				{
					part_type[i].slooks = part_type[j].slooks;
					break;
				}
			}
		}
		r_plooksdirty = false;
		CL_RegisterParticles();
	}
#if 1
	pframetime = cl.time - oldtime;
	if (pframetime < 0)
		pframetime = 0;
	oldtime = cl.time;
#else
	pframetime = host_frametime;
	if (cl.paused || r_secondaryview || r_refdef.recurse)
		pframetime = 0;
#endif
	VectorScale (vup, 1.5, pup);
	VectorScale (vright, 1.5, pright);

	kill_list = kill_first = NULL;

	flurrytime -= pframetime;
	if (flurrytime < 0)
	{
		doflurry = true;
		flurrytime = 0.1+frandom()*0.3;
	}
	else
		doflurry = false;


	if (!free_decals)
	{
		//mark some as dead, so we can keep spawning new ones next frame.
		for (i = 0; i < 256; i++)
		{
			decals[r_decalrecycle].die = -1;
			if (++r_decalrecycle >= r_numdecals)
				r_decalrecycle = 0;
		}
	}
	if (!free_particles)
	{
		//mark some as dead.
		for (i = 0; i < 256; i++)
		{
			particles[r_particlerecycle].die = -1;
			if (++r_particlerecycle >= r_numparticles)
				r_particlerecycle = 0;
		}
	}

	{
		float tmp[16];
		Matrix4_Invert(r_refdef.m_view, tmp);
		Matrix4_Multiply(tmp, lastviewmatrix, viewtranslation);
		memcpy(lastviewmatrix, r_refdef.m_view, sizeof(tmp));
	}

	for (type = part_run_list; type != NULL; type = type->nexttorun)
	{
		if (type->clippeddecals)
		{
			if (cl_numstris && cl_stris[cl_numstris-1].shader == type->looks.shader && cl_stris[cl_numstris-1].flags == 0)
				scenetri = &cl_stris[cl_numstris-1];
			else
			{
				if (cl_numstris == cl_maxstris)
				{
					cl_maxstris+=8;
					cl_stris = BZ_Realloc(cl_stris, sizeof(*cl_stris)*cl_maxstris);
				}
				scenetri = &cl_stris[cl_numstris++];
				scenetri->shader = type->looks.shader;
				scenetri->flags = 0;
				scenetri->firstidx = cl_numstrisidx;
				scenetri->firstvert = cl_numstrisvert;
				scenetri->numvert = 0;
				scenetri->numidx = 0;
			}

			for ( ;; )
			{
				dkill = type->clippeddecals;
				if (dkill && dkill->die < particletime)
				{
					type->clippeddecals = dkill->next;
					dkill->next = free_decals;
					free_decals = dkill;
					continue;
				}
				break;
			}
			for (d=type->clippeddecals ; d ; d=d->next)
			{
				for ( ;; )
				{
					dkill = d->next;
					if (dkill && dkill->die < particletime)
					{
						d->next = dkill->next;
						dkill->next = free_decals;
						free_decals = dkill;
						continue;
					}
					break;
				}


				if (d->die - particletime <= type->die)
				{
					switch (type->rampmode)
					{
					case RAMP_NEAREST:
						rampind = (int)(type->rampindexes * (type->die - (d->die - particletime)) / type->die);
						if (rampind >= type->rampindexes)
							rampind = type->rampindexes - 1;
						ramp = type->ramp + rampind;
						VectorCopy(ramp->rgb, d->rgba);
						d->rgba[3] = ramp->alpha;
						break;
					case RAMP_LERP:
						{
							float frac = (type->rampindexes * (type->die - (d->die - particletime)) / type->die);
							int s1, s2;
							s1 = min(type->rampindexes-1, frac);
							s2 = min(type->rampindexes-1, s1+1);
							frac -= s1;
							VectorInterpolate(type->ramp[s1].rgb, frac, type->ramp[s2].rgb, d->rgba);
							FloatInterpolate(type->ramp[s1].alpha, frac, type->ramp[s2].alpha, d->rgba[3]);
						}
						break;
					case RAMP_DELTA:	//particle ramps
						ramp = type->ramp + (int)(type->rampindexes * (type->die - (d->die - particletime)) / type->die);
						VectorMA(d->rgba, pframetime, ramp->rgb, d->rgba);
						d->rgba[3] -= pframetime*ramp->alpha;
						break;
					case RAMP_NONE:	//particle changes acording to it's preset properties.
						if (particletime < (d->die-type->die+type->rgbchangetime))
						{
							d->rgba[0] += pframetime*type->rgbchange[0];
							d->rgba[1] += pframetime*type->rgbchange[1];
							d->rgba[2] += pframetime*type->rgbchange[2];
						}
						d->rgba[3] += pframetime*type->alphachange;
					}
				}

				if (cl_numstrisvert - scenetri->firstvert >= MAX_INDICIES-6)
				{
					//generate a new mesh if the old one overflowed. yay smc...
					if (cl_numstris == cl_maxstris)
					{
						cl_maxstris+=8;
						cl_stris = BZ_Realloc(cl_stris, sizeof(*cl_stris)*cl_maxstris);
					}
					scenetri = &cl_stris[cl_numstris++];
					scenetri->shader = scenetri[-1].shader;
					scenetri->firstidx = cl_numstrisidx;
					scenetri->firstvert = cl_numstrisvert;
					scenetri->flags = scenetri[-1].flags;
					scenetri->numvert = 0;
					scenetri->numidx = 0;
				}
				R_AddClippedDecal(scenetri, d, type->slooks);
			}
		}

		bdraw = NULL;
		pdraw = NULL;
		tdraw = NULL;
		batchflags = 0;

		// set drawing methods by type and cvars and hope branch
		// prediction takes care of the rest
		switch(type->looks.type)
		{
		case PT_INVISIBLE:
			break;
		case PT_BEAM:
			bdraw = GL_DrawParticleBeam;
			break;
		case PT_VBEAM:
			bdraw = GL_DrawParticleVBeam;
			break;
		case PT_CDECAL:
			break;
		case PT_UDECAL:
			tdraw = R_AddUnclippedDecal;
			break;
		case PT_NORMAL:
			pdraw = GL_DrawTexturedParticle;
			tdraw = R_AddTexturedParticle;
			break;
		case PT_SPARK:
			tdraw = R_AddLineSparkParticle;
			batchflags = BEF_LINES;
			break;
		case PT_SPARKFAN:
			pdraw = sparkfanparticles;
			break;
		case PT_TEXTUREDSPARK:
			pdraw = sparktexturedparticles;
			tdraw = R_AddTSparkParticle;
			break;
		}

		if (!tdraw || (type->looks.shader->sort == SHADER_SORT_BLEND && pdraw))
			scenetri = NULL;
		else if (cl_numstris && cl_stris[cl_numstris-1].shader == type->looks.shader && cl_stris[cl_numstris-1].flags == batchflags)
			scenetri = &cl_stris[cl_numstris-1];
		else
		{
			if (cl_numstris == cl_maxstris)
			{
				cl_maxstris+=8;
				cl_stris = BZ_Realloc(cl_stris, sizeof(*cl_stris)*cl_maxstris);
			}
			scenetri = &cl_stris[cl_numstris++];
			scenetri->shader = type->looks.shader;
			scenetri->firstidx = cl_numstrisidx;
			scenetri->firstvert = cl_numstrisvert;
			scenetri->flags = batchflags;
			scenetri->numvert = 0;
			scenetri->numidx = 0;
		}

		if (!type->die)
		{
			while ((p=type->particles))
			{
				if (scenetri)
				{
					if (cl_numstrisvert - scenetri->firstvert >= MAX_INDICIES-6)
					{
						//generate a new mesh if the old one overflowed. yay smc...
						if (cl_numstris == cl_maxstris)
						{
							cl_maxstris+=8;
							cl_stris = BZ_Realloc(cl_stris, sizeof(*cl_stris)*cl_maxstris);
						}
						scenetri = &cl_stris[cl_numstris++];
						scenetri->shader = scenetri[-1].shader;
						scenetri->firstidx = cl_numstrisidx;
						scenetri->firstvert = cl_numstrisvert;
						scenetri->flags = scenetri[-1].flags;
						scenetri->numvert = 0;
						scenetri->numidx = 0;
					}
					tdraw(scenetri, p, type->slooks);
				}
				else if (pdraw)
					RQ_AddDistReorder(pdraw, p, type->slooks, p->org);

				// make sure emitter runs at least once
				if (type->emit >= 0 && type->emitstart <= 0 && pframetime)
					P_RunParticleEffectType(p->org, p->vel, pframetime, type->emit);

				// make sure stain effect runs
				if (type->stainonimpact && r_bloodstains.value)
				{
					if (traces-->0&&CL_TraceLine(oldorg, p->org, stop, normal, NULL)<1)
					{
						Surf_AddStain(stop,	(p->rgba[1]*-10+p->rgba[2]*-10),
											(p->rgba[0]*-10+p->rgba[2]*-10),
											(p->rgba[0]*-10+p->rgba[1]*-10),
											30*p->rgba[3]*type->stainonimpact*r_bloodstains.value);
					}
				}

				type->particles = p->next;
//				p->next = free_particles;
//				free_particles = p;
				p->next = kill_list;
				kill_list = p;
				if (!kill_first) // branch here is probably faster than list traversal later
					kill_first = p;
			}

			if (type->beams)
			{
				b = type->beams;
			}

			while ((b=type->beams) && (b->flags & BS_DEAD))
			{
				type->beams = b->next;
				b->next = free_beams;
				free_beams = b;
			}

			while (b)
			{
				if (!(b->flags & BS_NODRAW))
				{
					// no BS_NODRAW implies b->next != NULL
					// BS_NODRAW should imply b->next == NULL or b->next->flags & BS_DEAD
					VectorCopy(b->next->p->org, stop);
					VectorCopy(b->p->org, oldorg);
					VectorSubtract(stop, oldorg, b->next->dir);
					VectorNormalize(b->next->dir);
					if (bdraw)
					{
						VectorAdd(stop, oldorg, stop);
						VectorScale(stop, 0.5, stop);

						RQ_AddDistReorder(bdraw, b, type->slooks, stop);
					}
				}

				// clean up dead entries ahead of current
				for ( ;; )
				{
					bkill = b->next;
					if (bkill && (bkill->flags & BS_DEAD))
					{
						b->next = bkill->next;
						bkill->next = free_beams;
						free_beams = bkill;
						continue;
					}
					break;
				}

				b->flags |= BS_DEAD;
				b = b->next;
			}

			goto endtype;
		}

		//kill off early ones.
		if (type->emittime < 0)
		{
			for ( ;; )
			{
				kill = type->particles;
				if (kill && kill->die < particletime)
				{
					P_DelinkTrailstate(&kill->state.trailstate);
					type->particles = kill->next;
					kill->next = kill_list;
					kill_list = kill;
					if (!kill_first)
						kill_first = kill;
					continue;
				}
				break;
			}
		}
		else
		{
			for ( ;; )
			{
				kill = type->particles;
				if (kill && kill->die < particletime)
				{
					type->particles = kill->next;
					kill->next = kill_list;
					kill_list = kill;
					if (!kill_first)
						kill_first = kill;
					continue;
				}
				break;
			}
		}

		grav = type->gravity*pframetime;
		friction[0] = 1 - type->friction[0]*pframetime;
		friction[1] = 1 - type->friction[1]*pframetime;
		friction[2] = 1 - type->friction[2]*pframetime;

		for (p=type->particles ; p ; p=p->next)
		{
			if (type->emittime < 0)
			{
				for ( ;; )
				{
					kill = p->next;
					if (kill && kill->die < particletime)
					{
						P_DelinkTrailstate(&kill->state.trailstate);
						p->next = kill->next;
						kill->next = kill_list;
						kill_list = kill;
						if (!kill_first)
							kill_first = kill;
						continue;
					}
					break;
				}
			}
			else
			{
				for ( ;; )
				{
					kill = p->next;
					if (kill && kill->die < particletime)
					{
						p->next = kill->next;
						kill->next = kill_list;
						kill_list = kill;
						if (!kill_first)
							kill_first = kill;
						continue;
					}
					break;
				}
			}

			VectorCopy(p->org, oldorg);
			if (type->flags & PT_VELOCITY)
			{
				p->org[0] += p->vel[0]*pframetime;
				p->org[1] += p->vel[1]*pframetime;
				p->org[2] += p->vel[2]*pframetime;
				if (type->flags & PT_FRICTION)
				{
					p->vel[0] *= friction[0];
					p->vel[1] *= friction[1];
					p->vel[2] *= friction[2];
				}
				if (type->flurry && doflurry)
				{	//these should probably be partially synced, 
					p->vel[0] += crandom() * type->flurry;
					p->vel[1] += crandom() * type->flurry;
				}
				p->vel[2] -= grav;
			}

			if (type->viewspacefrac)
			{
				vec3_t tmp;
				Matrix4x4_CM_Transform3(viewtranslation, p->org, tmp);
				VectorInterpolate(p->org, type->viewspacefrac, tmp, p->org);
				Matrix4x4_CM_Transform3x3(viewtranslation, p->vel, tmp);
				VectorInterpolate(p->vel, type->viewspacefrac, tmp, p->vel);
			}

			p->angle += p->rotationspeed*pframetime;

			switch (type->rampmode)
			{
			case RAMP_NEAREST:
				rampind = (int)(type->rampindexes * (type->die - (p->die - particletime)) / type->die);
				if (rampind >= type->rampindexes)
					rampind = type->rampindexes - 1;
				ramp = type->ramp + rampind;
				VectorCopy(ramp->rgb, p->rgba);
				p->rgba[3] = ramp->alpha;
				p->scale = ramp->scale;
				break;
			case RAMP_LERP:
				{
					float frac = (type->rampindexes * (type->die - (p->die - particletime)) / type->die);
					int s1, s2;
					s1 = min(type->rampindexes-1, frac);
					s2 = min(type->rampindexes-1, s1+1);
					frac -= s1;
					VectorInterpolate(type->ramp[s1].rgb, frac, type->ramp[s2].rgb, p->rgba);
					FloatInterpolate(type->ramp[s1].alpha, frac, type->ramp[s2].alpha, p->rgba[3]);
					FloatInterpolate(type->ramp[s1].scale, frac, type->ramp[s2].scale, p->scale);
				}
				break;
			case RAMP_DELTA:	//particle ramps
				rampind = (int)(type->rampindexes * (type->die - (p->die - particletime)) / type->die);
				if (rampind >= type->rampindexes)
					rampind = type->rampindexes - 1;
				ramp = type->ramp + rampind;
				VectorMA(p->rgba, pframetime, ramp->rgb, p->rgba);
				p->rgba[3] -= pframetime*ramp->alpha;
				p->scale += pframetime*ramp->scale;
				break;
			case RAMP_NONE:	//particle changes acording to it's preset properties.
				if (particletime < (p->die-type->die+type->rgbchangetime))
				{
					p->rgba[0] += pframetime*type->rgbchange[0];
					p->rgba[1] += pframetime*type->rgbchange[1];
					p->rgba[2] += pframetime*type->rgbchange[2];
				}
				p->rgba[3] += pframetime*type->alphachange;
				p->scale += pframetime*type->scaledelta;
			}

			if (type->emit >= 0)
			{
				if (type->emittime < 0)
					P_ParticleTrail(oldorg, p->org, type->emit, pframetime, 0, NULL, &p->state.trailstate);
				else if (p->state.nextemit < particletime)
				{
					p->state.nextemit = particletime + type->emittime + frandom()*type->emitrand;
					P_RunParticleEffectType(p->org, p->vel, 1, type->emit);
				}
			}

			if (type->cliptype>=0 && r_bouncysparks.ival)
			{
				VectorSubtract(p->org, p->oldorg, stop);
				if (DotProduct(stop,stop) > 10*10)
				{
					int e;
					if (traces-->0&&CL_TraceLine(p->oldorg, p->org, stop, normal, &e)<1)
					{
						if (type->stainonimpact && r_bloodstains.value)
							Surf_AddStain(stop,	p->rgba[1]*-10+p->rgba[2]*-10,
												p->rgba[0]*-10+p->rgba[2]*-10,
												p->rgba[0]*-10+p->rgba[1]*-10,
												30*p->rgba[3]*r_bloodstains.value);

						if (type->clipbounce < 0)
						{
							p->die = -1;
							if (type->clipbounce == -2)
							{	//this type of particle splatters itself as a decal when it hits a wall.
								decalctx_t ctx;
								float m;
								vec3_t vec={0.5, 0.5, 0.431};
								model_t *model;
								float rotangle;

								ctx.entity = e;
								if (!ctx.entity)
								{
									model = cl.worldmodel;
									VectorCopy(p->org, ctx.center);
								}
								else if ((unsigned)ctx.entity < (unsigned)cl.maxlerpents)
								{	//this trace hit a door or something.
									lerpents_t *le = cl.lerpents+ctx.entity;
									model = cl.model_precache[le->entstate->modelindex];
									VectorSubtract(p->org, le->origin, ctx.center);
									//FIXME: rotate center+normal around entity.
								}
								else
									continue;	//err, no idea.

								VectorNegate(normal, ctx.normal);
								VectorNormalize(ctx.normal);

								VectorNormalize(vec);
								CrossProduct(ctx.normal, vec, ctx.tangent1);
								rotangle = type->rotationstartmin+frandom()*type->rotationstartrand;
								rotangle *= 180/M_PI; //gah! Matrix4x4_CM_NewRotation takes degrees but we already converted to radians.
								Matrix4x4_CM_Transform3(Matrix4x4_CM_NewRotation(rotangle, ctx.normal[0], ctx.normal[1], ctx.normal[2]), ctx.tangent1, ctx.tangent2);
								CrossProduct(ctx.normal, ctx.tangent2, ctx.tangent1);

								VectorNormalize(ctx.tangent1);
								VectorNormalize(ctx.tangent2);

								ctx.ptype = type;
								ctx.scale1 = type->s2 - type->s1;
								ctx.bias1 = type->s1 + ctx.scale1/2;
								ctx.scale2 = type->t2 - type->t1;
								ctx.bias2 = type->t1 + ctx.scale2/2;
								m = p->scale*(0.5+frandom()*0.5);	//decals should be a little bigger, for some reason.
								ctx.scale0 = 2.0 / m;
								ctx.scale1 /= m;
								ctx.scale2 /= m;

								//inserts decals through a callback.
								Mod_ClipDecal(model, ctx.center, ctx.normal, ctx.tangent2, ctx.tangent1, m, type->surfflagmask, type->surfflagmatch, PScript_AddDecals, &ctx);
							}
							continue;
						}
						else if (part_type + type->cliptype == type)
						{	//bounce
							dist = DotProduct(p->vel, normal);// * (-1-(rand()/(float)0x7fff)/2);
							dist *= -type->clipbounce;
							VectorMA(p->vel, dist, normal, p->vel);
							VectorCopy(stop, p->org);

							if (!*type->texname && Length(p->vel)<1000*pframetime && type->looks.type == PT_NORMAL)
							{
								p->die = -1;
								continue;
							}
						}
						else
						{
							p->die = -1;
							VectorNormalize(p->vel);

							if (type->clipbounce)
							{
								VectorScale(normal, type->clipbounce, normal);
								P_RunParticleEffectType(stop, normal, type->clipcount/part_type[type->cliptype].count, type->cliptype);
							}
							else
								P_RunParticleEffectType(stop, p->vel, type->clipcount/part_type[type->cliptype].count, type->cliptype);
							continue;
						}
					}
					VectorCopy(p->org, p->oldorg);
				}
			}
			else if (type->stainonimpact && r_bloodstains.value)
			{
				VectorSubtract(p->org, p->oldorg, stop);
				if (DotProduct(stop,stop) > 10*10)
				{
					if (traces-->0&&CL_TraceLine(p->oldorg, p->org, stop, normal, NULL)<1)
					{
						if (type->stainonimpact < 0)
							Surf_AddStain(stop,	(p->rgba[0]*-1),
												(p->rgba[1]*-1),
												(p->rgba[2]*-1),
												p->scale*-type->stainonimpact*r_bloodstains.value);
						else
							Surf_AddStain(stop,	(p->rgba[1]*-10+p->rgba[2]*-10),
												(p->rgba[0]*-10+p->rgba[2]*-10),
												(p->rgba[0]*-10+p->rgba[1]*-10),
												30*p->rgba[3]*type->stainonimpact*r_bloodstains.value);
						p->die = -1;
						continue;
					}
					VectorCopy(p->org, p->oldorg);
				}
			}

			if (scenetri)
			{
				if (cl_numstrisvert - scenetri->firstvert >= MAX_INDICIES-6)
				{
					//generate a new mesh if the old one overflowed. yay smc...
					if (cl_numstris == cl_maxstris)
					{
						cl_maxstris+=8;
						cl_stris = BZ_Realloc(cl_stris, sizeof(*cl_stris)*cl_maxstris);
					}
					scenetri = &cl_stris[cl_numstris++];
					scenetri->shader = scenetri[-1].shader;
					scenetri->firstidx = cl_numstrisidx;
					scenetri->firstvert = cl_numstrisvert;
					scenetri->flags = scenetri[-1].flags;
					scenetri->numvert = 0;
					scenetri->numidx = 0;
				}
				tdraw(scenetri, p, type->slooks);
			}
			else if (pdraw)
				RQ_AddDistReorder((void*)pdraw, p, type->slooks, p->org);
		}

		// beams are dealt with here

		// kill early entries
		for ( ;; )
		{
			bkill = type->beams;
			if (bkill && (bkill->flags & BS_DEAD || bkill->p->die < particletime) && !(bkill->flags & BS_LASTSEG))
			{
				type->beams = bkill->next;
				bkill->next = free_beams;
				free_beams = bkill;
				continue;
			}
			break;
		}


		b = type->beams;
		if (b)
		{
			for ( ;; )
			{
				if (b->next)
				{
					// mark dead entries
					if (b->flags & (BS_LASTSEG|BS_DEAD|BS_NODRAW))
					{
						// kill some more dead entries
						for ( ;; )
						{
							bkill = b->next;
							if (bkill && (bkill->flags & BS_DEAD) && !(bkill->flags & BS_LASTSEG))
							{
								b->next = bkill->next;
								bkill->next = free_beams;
								free_beams = bkill;
								continue;
							}
							break;
						}

						if (!bkill) // have to check so we don't hit NULL->next
							continue;
					}
					else
					{
						if (!(b->next->flags & BS_DEAD))
						{
							VectorCopy(b->next->p->org, stop);
							VectorCopy(b->p->org, oldorg);
							VectorSubtract(stop, oldorg, b->next->dir);
							VectorNormalize(b->next->dir);
							if (bdraw)
							{
								VectorAdd(stop, oldorg, stop);
								VectorScale(stop, 0.5, stop);

								RQ_AddDistReorder(bdraw, b, type->slooks, stop);
							}
						}

						if (b->p->die < particletime)
							b->flags |= BS_DEAD;
					}
				}
				else
				{
					if (b->p->die < particletime) // end of the list check
						b->flags |= BS_DEAD;

					break;
				}

				if (b->p->die < particletime)
					b->flags |= BS_DEAD;

				b = b->next;
			}
		}

endtype:

		// delete from run list if necessary
		if (!type->particles && !type->beams && !type->clippeddecals)
		{
			if (type->nexttorun)
				type->nexttorun->runlink = type->runlink;
			*type->runlink = type->nexttorun;
			type->runlink = NULL;
			type->state &= ~PS_INRUNLIST;
		}
	}

	RSpeedEnd(RSPEED_PARTICLES);

	// lazy delete for particles is done here
	if (kill_list)
	{
		kill_first->next = free_particles;
		free_particles = kill_list;
	}

	particletime += pframetime;
}

/*
===============
R_DrawParticles
===============
*/
static void PScript_DrawParticles (void)
{
	if (r_part_rain.value)
	{
		entity_t *ent;
		int i;
		
		P_AddRainParticles(cl.worldmodel, r_worldentity.axis, r_worldentity.origin, pframetime);

		for (i = 0; i < cl_numvisedicts ; i++)
		{
			ent = &cl_visedicts[i];
			if (!ent->model || ent->model->loadstate != MLS_LOADED)
				continue;

			//this timer, as well as the per-tri timer, are unable to deal with certain rates+sizes. it would be good to fix that...
			//it would also be nice to do mdls too...
			P_AddRainParticles(ent->model, ent->axis, ent->origin, pframetime);
		}
	}

	PScript_DrawParticleTypes();

	if (fallback)
		fallback->DrawParticles();
}


particleengine_t pe_script =
{
	"script",
	"fte",

	PScript_FindParticleType,
	PScript_Query,

	PScript_RunParticleEffectTypeString,
	PScript_ParticleTrail,
	PScript_RunParticleEffectState,
	PScript_RunParticleWeather,
	PScript_RunParticleCube,
	PScript_RunParticleEffect,
	PScript_RunParticleEffect2,
	PScript_RunParticleEffect3,
	PScript_RunParticleEffect4,
	PScript_RunParticleEffectPalette,

	PScript_ParticleTrailIndex,
	PScript_InitParticles,
	PScript_Shutdown,
	PScript_DelinkTrailstate,
	PScript_ClearParticles,
	PScript_DrawParticles
};

#endif
#endif

