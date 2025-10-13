/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the included (GNU.txt) GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "quakedef.h"

#ifdef PSET_CLASSIC

#include "glquake.h"
#include "shader.h"
#include "renderque.h"

#define POLYS

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


typedef enum {
	DODGY,

	ROCKET_TRAIL,
	ALT_ROCKET_TRAIL,
	BLOOD_TRAIL,
	GRENADE_TRAIL,
	BIG_BLOOD_TRAIL,
	TRACER1_TRAIL,
	TRACER2_TRAIL,
	VOOR_TRAIL,

	BRIGHTFIELD_POINT,

	BLOBEXPLOSION_POINT,
	LAVASPLASH_POINT,
	EXPLOSION_POINT,
	EXPLOSION2_POINT,
	TELEPORTSPLASH_POINT,
	MUZZLEFLASH_POINT,
	QWGUNSHOT_POINT,	//not actually the same as nq, to deal with higher counts better
	QWSTDBLOOD_POINT,	//same
	QWLGBLOOD_POINT,	//same

	EFFECTTYPE_MAX
} effect_type_t;


typedef struct cparticle_s
{
	avec3_t org;
	float die;
	avec3_t vel;
	float ramp;
	enum
	{
		pt_static,
		pt_fire,
		pt_explode,
		pt_explode2,
		pt_blob,
		pt_blob2,
		pt_grav,
		pt_slowgrav,

		pt_oneframe
	} type;
	unsigned int rgb;
	struct cparticle_s *next;
} cparticle_t;

#define DEFAULT_NUM_PARTICLES	2048
#define ABSOLUTE_MIN_PARTICLES	512
#define ABSOLUTE_MAX_PARTICLES	8192
static int r_numparticles;
static cparticle_t	*particles, *active_particles, *free_particles;
extern cvar_t r_part_density, r_part_classic_expgrav, r_part_classic_opaque;

static unsigned int particleframe;

static int	ramp1[8] = {0x6f, 0x6d, 0x6b, 0x69, 0x67, 0x65, 0x63, 0x61};
static int	ramp2[8] = {0x6f, 0x6e, 0x6d, 0x6c, 0x6b, 0x6a, 0x68, 0x66};
static int	ramp3[8] = {0x6d, 0x6b, 6, 5, 4, 3};

#ifndef POLYS
#define BUFFERVERTS 2048*3
static vecV_t classicverts[BUFFERVERTS];
static union c
{
	byte_vec4_t b;
	unsigned int i;
} classiccolours[BUFFERVERTS];
static vec2_t classictexcoords[BUFFERVERTS];
static index_t classicindexes[BUFFERVERTS];
static mesh_t classicmesh;
#endif
static shader_t *classicshader;



//obtains an index for the name, even if it is unknown (one can be loaded after. will only fail if the effect limit is reached)
//technically this function is not meant to fail often, but thats fine so long as the other functions are meant to safely reject invalid effect numbers.
static int PClassic_FindParticleType(const char *name)
{
	if (!stricmp("tr_rocket", name))
		return ROCKET_TRAIL;
	if (!stricmp("tr_altrocket", name))
		return ALT_ROCKET_TRAIL;
	if (!stricmp("tr_slightblood", name))
		return BLOOD_TRAIL;
	if (!stricmp("tr_grenade", name))
		return GRENADE_TRAIL;
	if (!stricmp("tr_blood", name))
		return BIG_BLOOD_TRAIL;
	if (!stricmp("tr_wizspike", name))
		return TRACER1_TRAIL;
	if (!stricmp("tr_knightspike", name))
		return TRACER2_TRAIL;
	if (!stricmp("tr_vorespike", name))
		return VOOR_TRAIL;

	if (!stricmp("te_tarexplosion", name))
		return BLOBEXPLOSION_POINT;
	if (!stricmp("te_lavasplash", name))
		return LAVASPLASH_POINT;
	if (!stricmp("te_explosion", name))
		return EXPLOSION_POINT;
	if (!strnicmp("te_explosion2_", name, 14))
	{
		char *e;
		int start = strtoul(name+14, &e, 10);
		int len = strtoul((*e == '_')?e+1:e, &e, 10);
		if (!*e && start >= 0 && start <= 255 && len >= 0 && len <= 255)
			return EXPLOSION2_POINT | (start<<8)|(len<<16);
	}
	if (!stricmp("te_teleport", name))
		return TELEPORTSPLASH_POINT;
	if (!stricmp("te_muzzleflash", name))
		return MUZZLEFLASH_POINT;
	if (!stricmp("ef_brightfield", name))
		return BRIGHTFIELD_POINT;

	if (!stricmp("te_qwgunshot", name))
		return QWGUNSHOT_POINT;
	if (!stricmp("te_qwblood", name))
		return QWSTDBLOOD_POINT;
	if (!stricmp("te_lightningblood", name))
		return QWLGBLOOD_POINT;

	return P_INVALID;
}

static qboolean PClassic_Query(int type, int body, char *outstr, int outstrlen)
{
	char *n = NULL;
	switch(type&0xff)
	{
	case ROCKET_TRAIL:
		n = "tr_rocket";
		break;
	case ALT_ROCKET_TRAIL:
		n = "tr_altrocket";
		break;
	case BLOOD_TRAIL:
		n = "tr_slightblood";
		break;
	case GRENADE_TRAIL:
		n = "tr_grenade";
		break;
	case BIG_BLOOD_TRAIL:
		n = "tr_blood";
		break;
	case TRACER1_TRAIL:
		n = "tr_wizspike";
		break;
	case TRACER2_TRAIL:
		n = "tr_knightspike";
		break;
	case VOOR_TRAIL:
		n = "tr_vorespike";
		break;

	case BLOBEXPLOSION_POINT:
		n = "te_tarexplosion";
		break;
	case LAVASPLASH_POINT:
		n = "te_lavasplash";
		break;
	case EXPLOSION_POINT:
		n = "te_explosion";
		break;
	case EXPLOSION2_POINT:
		n = va("te_explosion2_%i_%i", (type>>8)&0xff, (type>>16)&0xff);
		break;
	case TELEPORTSPLASH_POINT:
		n = "te_teleport";
		break;
	case BRIGHTFIELD_POINT:
		n = "ef_brightfield";
		break;
	}

	if (!n)
		return false;
	
	if (body == 0)
	{
		Q_strncpyz(outstr, n, outstrlen);
		return true;
	}
	return false;
}

//a convienience function.
static int PClassic_RunParticleEffectTypeString (vec3_t org, vec3_t dir, float count, char *name)
{
	int efnum = P_FindParticleType(name);
	return P_RunParticleEffectState(org, dir, count, efnum, NULL);
}

//DP extension: add particles within a box that look like rain or snow.
static void PClassic_RunParticleWeather(vec3_t minb, vec3_t maxb, vec3_t dir, float count, int colour, char *efname)
{
}

//DP extension: add particles within a box.
static void PClassic_RunParticleCube(int ptype, vec3_t minb, vec3_t maxb, vec3_t dir_min, vec3_t dir_max, float count, int colour, qboolean gravity, float jitter)
{
}

//hexen2 support: add particles flying out from a point with a randomized speed
static void PClassic_RunParticleEffect2 (vec3_t org, vec3_t dmin, vec3_t dmax, int color, int effect, int count)
{
}

//hexen2 support: add particles within a box.
static void PClassic_RunParticleEffect3 (vec3_t org, vec3_t box, int color, int effect, int count)
{
}

//hexen2 support: add particles around the spot in a radius. no idea what the 'effect' field is.
static void PClassic_RunParticleEffect4 (vec3_t org, float radius, int color, int effect, int count)
{
}

//this function is used as a fallback in case a trail effect is unknown.
static void PClassic_ParticleTrailIndex (vec3_t start, vec3_t end, int type, float timestep, int color, int crnd, trailkey_t *tk)
{
}

//the one-time initialisation function, called no mater which renderer is active.
static qboolean PClassic_InitParticles (void)
{
	int i;

	if ((i = COM_CheckParm ("-particles")) && i + 1 < com_argc)
	{
		r_numparticles = (int) (Q_atoi(com_argv[i + 1]));
		r_numparticles = bound(ABSOLUTE_MIN_PARTICLES, r_numparticles, ABSOLUTE_MAX_PARTICLES);
	}
	else
	{
		r_numparticles = DEFAULT_NUM_PARTICLES;
	}

	particles = (cparticle_t *) BZ_Malloc (r_numparticles * sizeof(cparticle_t));

#ifndef POLYS
	for (i = 0; i < BUFFERVERTS; i += 3)
	{
		classictexcoords[i+1][0] = 1;
		classictexcoords[i+2][1] = 1;

		classicindexes[i+0] = i+0;
		classicindexes[i+1] = i+1;
		classicindexes[i+2] = i+2;
	}
	classicmesh.xyz_array = classicverts;
	classicmesh.st_array = classictexcoords;
	classicmesh.colors4b_array = (byte_vec4_t*)classiccolours;
	classicmesh.indexes = classicindexes;
#endif
	classicshader = R_RegisterShader("particles_classic", SUF_NONE,
		"{\n"
			"program defaultsprite\n"
			"nomipmaps\n"
			"surfaceparm nodlight\n"
			"{\n"
				"if r_part_classic_square\n"
					"clampmap classicparticle_square\n"
				"else\n"
					"clampmap classicparticle\n"
				"endif\n"
				"rgbgen vertex\n"
				"alphagen vertex\n"
				"blendfunc blend\n"
			"}\n"
		"}\n"
		);

	return true;
}

static void PClassic_ShutdownParticles(void)
{
	BZ_Free(particles);
	particles = NULL;
}

// a classic trailstate key is really just a float
// assuming float alignment/size is more strict than our key type 
static float Classic_GetLeftover(trailkey_t *tk)
{
	float *f = (float *)tk;

	if (!f)
		return 0;

	return *f;
}

static void Classic_SetLeftover(trailkey_t *tk, float leftover)
{
	float *f = (float *)tk;

	if (f)
		*f = leftover;
}

//called when an entity is removed from the world, taking its trailstate with it.
static void PClassic_DelinkTrailstate(trailkey_t *tk)
{
	*tk = 0;
}

//wipes all the particles ready for the next map.
static void PClassic_ClearParticles (void)
{
	int		i;
	
	free_particles = &particles[0];
	active_particles = NULL;

	for (i = 0;i < r_numparticles; i++)
		particles[i].next = &particles[i+1];
	particles[r_numparticles - 1].next = NULL;
}

//some particles (brightfield) must last only one frame
static void PClassic_ClearPerFrame(void)
{
	if (particleframe != -1 && particleframe != cl_framecount)
	{
		cparticle_t **link, *kill;
		for (link = &active_particles; *link; )
		{
			if ((*link)->type == pt_oneframe)
			{
				kill = *link;
				*link = kill->next;
				kill->next = free_particles;
				free_particles = kill;
			}
			else
				link = &(*link)->next;
		}
	}
}

//draws all the active particles.
static void PClassic_DrawParticles(void)
{
	cparticle_t *p, *kill;
	int i;
	float time2, time3, time1, dvel, frametime, grav;
	vec3_t up, right;
	float dist, scale, r_partscale=0;
#ifdef POLYS
	scenetris_t *scenetri;
#else
	union c usecolours;
#endif
	static float oldtime;
	RSpeedMark();

	if (!active_particles)
	{
		oldtime = cl.time;
		return;
	}

	if (particleframe != -1 && particleframe != cl_framecount)
	{
		PClassic_ClearPerFrame();
		particleframe = -1;
	}

	if (r_refdef.useperspective)
		r_partscale = 0.004 * tan (r_refdef.fov_x * (M_PI / 180) * 0.5f);
	else
		r_partscale = 0;
	VectorScale (vup, 1.5, up);
	VectorScale (vright, 1.5, right);

	frametime = cl.time - oldtime;
	oldtime = cl.time;
	frametime = bound(0, frametime, 1);
	if (cl.paused || r_secondaryview || r_refdef.recurse)
		frametime = 0;
	time3 = frametime * 15;
	time2 = frametime * 10; // 15;
	time1 = frametime * 5;
	grav = frametime * 800 * 0.05;
	dvel = 4 * frametime;

#ifdef POLYS
//	if (cl_numstris && cl_stris[cl_numstris-1].shader == classicshader && cl_stris[cl_numstris-1].numvert + 8 <= MAX_INDICIES)
//		scenetri = &cl_stris[cl_numstris-1];
//	else
	{
		if (cl_numstris == cl_maxstris)
		{
			cl_maxstris+=8;
			cl_stris = BZ_Realloc(cl_stris, sizeof(*cl_stris)*cl_maxstris);
		}
		scenetri = &cl_stris[cl_numstris++];
		scenetri->shader = classicshader;
		scenetri->flags = BEF_NODLIGHT|BEF_NOSHADOWS;
		scenetri->firstidx = cl_numstrisidx;
		scenetri->firstvert = cl_numstrisvert;
		scenetri->numvert = 0;
		scenetri->numidx = 0;
	}
#endif

	while(1)
	{
		kill = active_particles;
		if (kill && kill->die < cl.time)
		{
			active_particles = kill->next;
			kill->next = free_particles;
			free_particles = kill;
			continue;
		}
		break;
	}

	for (p = active_particles; p ; p = p->next)
	{
		while (1)
		{
			kill = p->next;
			if (kill && kill->die < cl.time)
			{
				p->next = kill->next;
				kill->next = free_particles;
				free_particles = kill;
				continue;
			}
			break;
		}

		// hack a scale up to keep particles from disapearing
		dist = (p->org[0] - r_origin[0]) * vpn[0] + (p->org[1] - r_origin[1]) * vpn[1] + (p->org[2] - r_origin[2]) * vpn[2];
		scale = 1 + dist * r_partscale;

#ifdef POLYS
		if (cl_numstrisvert+3 > cl_maxstrisvert)
			cl_stris_ExpandVerts(cl_numstrisvert+1024*3);

//		Vector4Set(cl_strisvertc[cl_numstrisvert+0],1,1,1,1);
//		Vector4Set(cl_strisvertc[cl_numstrisvert+1],1,1,1,1);
//		Vector4Set(cl_strisvertc[cl_numstrisvert+2],1,1,1,1);

		Vector4Set(cl_strisvertc[cl_numstrisvert+0], ((p->rgb&0xff)>>0)/255.0, ((p->rgb&0xff00)>>8)/255.0, ((p->rgb&0xff0000)>>16)/255.0, ((p->type == pt_fire && !r_part_classic_opaque.ival)?((6 - p->ramp) *0.166666):1.0));
		Vector4Copy(cl_strisvertc[cl_numstrisvert+0], cl_strisvertc[cl_numstrisvert+1]);
		Vector4Copy(cl_strisvertc[cl_numstrisvert+0], cl_strisvertc[cl_numstrisvert+2]);

		Vector2Set(cl_strisvertt[cl_numstrisvert+0], 0, 0);
		Vector2Set(cl_strisvertt[cl_numstrisvert+1], 1, 0);
		Vector2Set(cl_strisvertt[cl_numstrisvert+2], 0, 1);

		VectorCopy(p->org, cl_strisvertv[cl_numstrisvert+0]);
		VectorMA(p->org, scale, up, cl_strisvertv[cl_numstrisvert+1]);
		VectorMA(p->org, scale, right, cl_strisvertv[cl_numstrisvert+2]);

		if (cl_numstrisidx+3 > cl_maxstrisidx)
		{
			cl_maxstrisidx += 1024*3;
			cl_strisidx = BZ_Realloc(cl_strisidx, sizeof(*cl_strisidx)*cl_maxstrisidx);
		}
		cl_strisidx[cl_numstrisidx++] = (cl_numstrisvert - scenetri->firstvert) + 0;
		cl_strisidx[cl_numstrisidx++] = (cl_numstrisvert - scenetri->firstvert) + 1;
		cl_strisidx[cl_numstrisidx++] = (cl_numstrisvert - scenetri->firstvert) + 2;

		cl_numstrisvert += 3;

		scenetri->numvert += 3;
		scenetri->numidx += 3;
#else
		if (classicmesh.numvertexes >= BUFFERVERTS-3)
		{
			classicmesh.numindexes = classicmesh.numvertexes;
			BE_DrawMesh_Single(classicshader, &classicmesh, NULL, &classicshader->defaulttextures, 0);
			classicmesh.numvertexes = 0;
		}

		usecolours.i = p->rgb;
		if (p->type == pt_fire)
			usecolours.b[3] = 255 * (6 - p->ramp) / 6;
		else
			usecolours.b[3] = 255;
		classiccolours[classicmesh.numvertexes].i = usecolours.i;
		VectorCopy(p->org, classicverts[classicmesh.numvertexes]);
		classicmesh.numvertexes++;
		classiccolours[classicmesh.numvertexes].i = usecolours.i;
		VectorMA(p->org, scale, up, classicverts[classicmesh.numvertexes]);
		classicmesh.numvertexes++;
		classiccolours[classicmesh.numvertexes].i = usecolours.i;
		VectorMA(p->org, scale, right, classicverts[classicmesh.numvertexes]);
		classicmesh.numvertexes++;
#endif

		p->org[0] += p->vel[0] * frametime;
		p->org[1] += p->vel[1] * frametime;
		p->org[2] += p->vel[2] * frametime;
		
		switch (p->type)
		{
		case pt_oneframe:
		case pt_static:
			break;
		case pt_fire:
			p->ramp += time1;
			if (p->ramp >= 6)
				p->die = -1;
			else
				p->rgb = d_quaketo24srgbtable[ramp3[(int) p->ramp]];
			p->vel[2] += grav;
			break;
		case pt_explode:
			p->ramp += time2;
			if (p->ramp >=8)
				p->die = -1;
			else
				p->rgb = d_quaketo24srgbtable[ramp1[(int) p->ramp]];
			for (i = 0; i < 3; i++)
				p->vel[i] += p->vel[i] * dvel;
			p->vel[2] -= grav*r_part_classic_expgrav.value;
			break;
		case pt_explode2:
			p->ramp += time3;
			if (p->ramp >=8)
				p->die = -1;
			else
				p->rgb = d_quaketo24srgbtable[ramp2[(int) p->ramp]];
			for (i = 0; i < 3; i++)
				p->vel[i] -= p->vel[i] * frametime;
			p->vel[2] -= grav*r_part_classic_expgrav.value;
			break;
		case pt_blob:
			for (i = 0; i < 3; i++)
				p->vel[i] += p->vel[i] * dvel;
			p->vel[2] -= grav;
			break;
		case pt_blob2:
			for (i = 0; i < 2; i++)
				p->vel[i] -= p->vel[i] * dvel;
			p->vel[2] -= grav;
			break;
		case pt_slowgrav:
		case pt_grav:
			p->vel[2] -= grav;
			break;
		}
	}
#ifndef POLYS
	if (classicmesh.numvertexes)
	{
		classicmesh.numindexes = classicmesh.numvertexes;
		BE_DrawMesh_Single(classicshader, &classicmesh, NULL, &classicshader->defaulttextures, 0);
		classicmesh.numvertexes = 0;
	}
#endif
	RSpeedEnd(RSPEED_PARTICLESDRAW);
}


static void Classic_ParticleExplosion (vec3_t org)
{
	int	i, j;
	cparticle_t	*p;
	int count;

	count = 1024 * r_part_density.value;
	
	for (i = 0; i < count; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->die = cl.time + 5;
		p->rgb = d_8to24srgbtable[ramp1[0]];
		p->ramp = rand() & 3;
		if (i & 1)
		{
			p->type = pt_explode;
			for (j = 0; j < 3; j++)
			{
				p->org[j] = org[j] + ((rand() % 32) - 16);
				p->vel[j] = (rand() % 512) - 256;
			}
		}
		else
		{
			p->type = pt_explode2;
			for (j = 0; j < 3; j++)
			{
				p->org[j] = org[j] + ((rand() % 32) - 16);
				p->vel[j] = (rand()%512) - 256;
			}
		}
	}
}

static void Classic_ParticleExplosion2 (vec3_t org, int colorStart, int colorLength)
{
	int			i, j;
	cparticle_t	*p;
	int			colorMod = 0;

	for (i=0; i<512; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->die = cl.time + 0.3;
		p->rgb = d_8to24srgbtable[(colorStart + (colorMod % colorLength)) & 255];
		colorMod++;

		p->type = pt_blob;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + ((rand()%32)-16);
			p->vel[j] = (rand()%512)-256;
		}
	}
}

static void Classic_BlobExplosion (vec3_t org)
{
	int i, j;
	cparticle_t *p;
	int count;

	count = 1024 * r_part_density.value;
	
	for (i = 0; i < count; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->die = cl.time + 1 + (rand() & 8) * 0.05;

		if (i & 1)
		{
			p->type = pt_blob;
			p->rgb = d_8to24srgbtable[66 + rand() % 6];
			for (j = 0; j < 3; j++)
			{
				p->org[j] = org[j] + ((rand() % 32) - 16);
				p->vel[j] = (rand() % 512) - 256;
			}
		}
		else
		{
			p->type = pt_blob2;
			p->rgb = d_8to24srgbtable[150 + rand() % 6];
			for (j = 0; j < 3; j++)
			{
				p->org[j] = org[j] + ((rand() % 32) - 16);
				p->vel[j] = (rand() % 512) - 256;
			}
		}
	}
}

static void Classic_RunParticleEffect (vec3_t org, vec3_t dir, int color, int count, qboolean qwstyle)
{
	int i, j, scale;
	cparticle_t *p;

	if (!dir)
		dir = vec3_origin;

	if (qwstyle)
		scale = (count > 130) ? 3 : (count > 20) ? 2  : 1;	//QW
	else
		scale = 1;	//NQ

	count = ceil(count*r_part_density.value);	//round-to-0 was resulting in blood being far too hard to see, especially when blood is often spawned with multiple points all rounded down

	for (i = 0; i < count; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->die = cl.time + 0.1 * (rand() % 5);
		p->rgb = d_8to24srgbtable[(color & ~7) + (rand() & 7)];
		if (qwstyle)
			p->type = pt_grav;	//QW
		else
			p->type = pt_slowgrav;	//NQ
		for (j = 0; j < 3; j++)
		{
			p->org[j] = org[j] + scale * ((rand() & 15) - 8);
			p->vel[j] = dir[j] * 15;
		}
	}
}

static void Classic_LavaSplash (vec3_t org)
{
	int i, j, k;
	cparticle_t *p;
	float vel;
	vec3_t dir;

	for (i = -16; i < 16; i++)
	{
		for (j = -16; j < 16; j++)
		{
			for (k = 0; k < 1; k++)
			{
				if (!free_particles)
					return;
				p = free_particles;
				free_particles = p->next;
				p->next = active_particles;
				active_particles = p;

				p->die = cl.time + 2 + (rand() & 31) * 0.02;
				p->rgb = d_8to24srgbtable[224 + (rand() & 7)];
				p->type = pt_grav;

				dir[0] = j * 8 + (rand() & 7);
				dir[1] = i * 8 + (rand() & 7);
				dir[2] = 256;

				p->org[0] = org[0] + dir[0];
				p->org[1] = org[1] + dir[1];
				p->org[2] = org[2] + (rand() & 63);

				VectorNormalizeFast (dir);
				vel = 50 + (rand() & 63);
				VectorScale (dir, vel, p->vel);
			}
		}
	}
}

static void Classic_TeleportSplash (vec3_t org)
{
	int i, j, k;
	cparticle_t *p;
	float vel;
	vec3_t dir;

	int st = 4 / r_part_density.value;
	if (st == 0)
		st = 1;

	for (i = -16; i < 16; i += st)
	{
		for (j = -16; j < 16; j += st)
		{
			for (k = -24; k < 32; k += st)
			{
				if (!free_particles)
					return;
				p = free_particles;
				free_particles = p->next;
				p->next = active_particles;
				active_particles = p;

				p->die = cl.time + 0.2 + (rand() & 7) * 0.02;
				p->rgb = d_8to24srgbtable[7 + (rand() & 7)];
				p->type = pt_grav;

				dir[0] = j * 8;
				dir[1] = i * 8;
				dir[2] = k * 8;

				p->org[0] = org[0] + i + (rand() & 3);
				p->org[1] = org[1] + j + (rand() & 3);
				p->org[2] = org[2] + k + (rand() & 3);

				VectorNormalizeFast (dir);
				vel = 50 + (rand() & 63);
				VectorScale (dir, vel, p->vel);
			}
		}
	}
}

#define NUMVERTEXNORMALS	162
//vec3_t	avelocity = {23, 7, 3};
//float	partstep = 0.01;
//float	timescale = 0.01;
static	vec3_t	avelocities[NUMVERTEXNORMALS];
static void Classic_BrightField (vec3_t org)
{
	extern	float	r_avertexnormals[NUMVERTEXNORMALS][3];
	float	beamlength = 16;

	int			i;
	cparticle_t	*p;
	float		angle;
	float		sp, sy, cp, cy;
	vec3_t		forward;
	float		dist;

	PClassic_ClearPerFrame();
	particleframe = cl_framecount;

	dist = 64;

	if (!avelocities[0][0])
	{
		for (i=0 ; i<NUMVERTEXNORMALS ; i++)
		{
			avelocities[i][0] = (rand()&255) * 0.01;
			avelocities[i][1] = (rand()&255) * 0.01;
			avelocities[i][2] = (rand()&255) * 0.01;
		}
	}

	for (i=0 ; i<NUMVERTEXNORMALS ; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		angle = cl.time * avelocities[i][0];
		sy = sin(angle);
		cy = cos(angle);
		angle = cl.time * avelocities[i][1];
		sp = sin(angle);
		cp = cos(angle);

		//fixme: is roll important?

		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;

		p->die = cl.time;// + 0.01;
		p->rgb = d_8to24srgbtable[0x6f];
		p->type = pt_oneframe;

		p->org[0] = org[0] + r_avertexnormals[i][0]*dist + forward[0]*beamlength;			
		p->org[1] = org[1] + r_avertexnormals[i][1]*dist + forward[1]*beamlength;			
		p->org[2] = org[2] + r_avertexnormals[i][2]*dist + forward[2]*beamlength;			
	}
}

//svc_tempentity support: this is the function that handles 'special' point effects.
//use the trail state so fast/slow frames keep the correct particle counts on certain every-frame effects
static int PClassic_RunParticleEffectState (vec3_t org, vec3_t dir, float count, int typenum, trailkey_t *tk)
{
	switch(typenum&0xff)
	{
	case BRIGHTFIELD_POINT:
		Classic_BrightField(org);
		break;
	case BLOBEXPLOSION_POINT:
		Classic_BlobExplosion(org);
		break;
	case LAVASPLASH_POINT:
		Classic_LavaSplash(org);
		break;
	case EXPLOSION_POINT:
		Classic_ParticleExplosion(org);
		break;
	case EXPLOSION2_POINT:
		Classic_ParticleExplosion2(org, (typenum>>8)&0xff, (typenum>>16)&0xff);
		break;
	case TELEPORTSPLASH_POINT:
		Classic_TeleportSplash(org);
		break;
	case MUZZLEFLASH_POINT:
		{
			dlight_t *dl = CL_AllocDlight (0);
			if (dir)
				VectorCopy(dir, dl->axis[0]);
			else
				VectorSet(dl->axis[0], 0, 0, 1);
			VectorVectors(dl->axis[0], dl->axis[1], dl->axis[2]);
			VectorInverse(dl->axis[1]);
			if (dir)
				VectorMA (org, 15, dl->axis[0], dl->origin);
			else
				VectorCopy (org, dl->origin);

			dl->radius = 200 + (rand()&31);
			dl->minlight = 32;
			dl->die = cl.time + 0.1;
			dl->color[0] = 1.5;
			dl->color[1] = 1.3;
			dl->color[2] = 1.0;

			dl->channelfade[0] = 1.5;
			dl->channelfade[1] = 0.75;
			dl->channelfade[2] = 0.375;
			dl->decay = 1000;
#ifdef RTLIGHTS
			dl->lightcolourscales[2] = 4;
#endif
		}
		break;
	case QWGUNSHOT_POINT:
		Classic_RunParticleEffect(org, dir, 0, count*20, true);
		break;
	case QWSTDBLOOD_POINT:
		Classic_RunParticleEffect(org, dir, 73, count*20, true);
		break;
	case QWLGBLOOD_POINT:
		Classic_RunParticleEffect(org, dir, 225, count*50, true);
		break;
	default:
		return 1;
	}
	return 0;
}

static float Classic_ParticleTrail (vec3_t start, vec3_t end, float leftover, effect_type_t type)
{
	vec3_t point, delta, dir, step;
	float len, rlen, scale;
	int i, j, num_particles;
	cparticle_t *p;
	static int tracercount;

	if (type >= BRIGHTFIELD_POINT)
	{
		PClassic_RunParticleEffectState(end, vec3_origin, 1, type, NULL);
		return 0;
	}

	VectorCopy (start, point);
	VectorSubtract (end, start, delta);
	if (!(len = VectorLength (delta)))
		goto done;
	VectorScale(delta, 1 / len, dir);	//unit vector in direction of trail

	VectorMA(point, -leftover, dir, point);
	len += leftover;
	rlen = len;

	switch (type)
	{
	case ALT_ROCKET_TRAIL:
		scale = 1.5; break;
	case BLOOD_TRAIL:
		scale = 6; break;
	default:
		scale = 3; break;
	case TRACER1_TRAIL:
	case TRACER2_TRAIL:
		scale = (r_part_density.value < 0.5)?6*r_part_density.value:3;
		break;
	}

	scale /= r_part_density.value;

	VectorScale (dir, scale, step);

	len /= scale;
	leftover = rlen - ((int)(len) * scale);

	num_particles = (int) len;

	for (i = 0; i < num_particles && free_particles; i++)
	{
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		VectorClear (p->vel);
		p->die = cl.time + 2;

		switch(type)
		{		
		case GRENADE_TRAIL:
			p->ramp = (rand() & 3) + 2;
			p->rgb = d_8to24srgbtable[ramp3[(int) p->ramp]];
			p->type = pt_fire;
			for (j = 0; j < 3; j++)
				p->org[j] = point[j] + ((rand() % 6) - 3);
			break;
		case BLOOD_TRAIL:
			p->type = pt_slowgrav;
			p->rgb = d_8to24srgbtable[67 + (rand() & 3)];
			for (j = 0; j < 3; j++)
				p->org[j] = point[j] + ((rand() % 6) - 3);
			break;
		case BIG_BLOOD_TRAIL:
			p->type = pt_slowgrav;
			p->rgb = d_8to24srgbtable[67 + (rand() & 3)];
			for (j = 0; j < 3; j++)
				p->org[j] = point[j] + ((rand() % 6) - 3);
			break;
		case TRACER1_TRAIL:
		case TRACER2_TRAIL:
			p->die = cl.time + 0.5;
			p->type = pt_static;
			if (type == TRACER1_TRAIL)
				p->rgb = d_8to24srgbtable[52 + ((tracercount & 4) << 1)];
			else
				p->rgb = d_8to24srgbtable[230 + ((tracercount & 4) << 1)];

			tracercount++;

			VectorCopy (point, p->org);
			if (tracercount & 1)
			{	//the addition of /scale here counters dir being rescaled
				p->vel[0] = 30 * dir[1];
				p->vel[1] = 30 * -dir[0];
			}
			else
			{
				p->vel[0] = 30 * -dir[1];
				p->vel[1] = 30 * dir[0];
			}
			break;
		case VOOR_TRAIL:
			p->rgb = d_8to24srgbtable[9 * 16 + 8 + (rand() & 3)];
			p->type = pt_static;
			p->die = cl.time + 0.3;
			for (j = 0; j < 3; j++)
				p->org[j] = point[j] + ((rand() & 15) - 8);
			break;
		case ALT_ROCKET_TRAIL:
			p->ramp = (rand() & 3);
			p->rgb = d_8to24srgbtable[ramp3[(int) p->ramp]];
			p->type = pt_fire;
			for (j = 0; j < 3; j++)
				p->org[j] = point[j] + ((rand() % 6) - 3);
			break;
		case ROCKET_TRAIL:
		default:		
			p->ramp = (rand() & 3);
			p->rgb = d_8to24srgbtable[ramp3[(int) p->ramp]];
			p->type = pt_fire;
			for (j = 0; j < 3; j++)
				p->org[j] = point[j] + ((rand() % 6) - 3);
			break;
		}
		VectorAdd (point, step, point);
	}
done:
	return leftover;
}

int PClassic_PointFile(int c, vec3_t point)
{
	cparticle_t *p;

	if (!free_particles)
		return 0;
	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;

	VectorClear (p->vel);
	p->die = 99999;
	p->rgb = d_8to24srgbtable[(-c) & 0xff];
	p->type = pt_static;
	VectorCopy(point, p->org);

	return 1;
}

//builds a trail from here to there. The trail state can be used to remember how far you got last frame.
static int PClassic_ParticleTrail (vec3_t startpos, vec3_t end, int type, float timestep, int dlkey, vec3_t dlaxis[3], trailkey_t *tk)
{
	float leftover;

	if (type == P_INVALID)
		return 1;

	leftover = Classic_ParticleTrail(startpos, end, Classic_GetLeftover(tk), type);
	Classic_SetLeftover(tk, leftover);
	return 0;
}

//svc_particle support: add X particles with the given colour, velocity, and aproximate origin.
static void PClassic_RunParticleEffect (vec3_t org, vec3_t dir, int color, int count)
{
	Classic_RunParticleEffect(org, dir, color, count, false);
}
static void PClassic_RunParticleEffectPalette (const char *nameprefix, vec3_t org, vec3_t dir, int color, int count)
{
	Classic_RunParticleEffect(org, dir, color, count, false);
}

particleengine_t pe_classic =
{
	"Classic",
	NULL,

	PClassic_FindParticleType,
	PClassic_Query,

	PClassic_RunParticleEffectTypeString,
	PClassic_ParticleTrail,
	PClassic_RunParticleEffectState,
	PClassic_RunParticleWeather,
	PClassic_RunParticleCube,
	PClassic_RunParticleEffect,
	PClassic_RunParticleEffect2,
	PClassic_RunParticleEffect3,
	PClassic_RunParticleEffect4,
	PClassic_RunParticleEffectPalette,

	PClassic_ParticleTrailIndex,
	PClassic_InitParticles,
	PClassic_ShutdownParticles,
	PClassic_DelinkTrailstate,
	PClassic_ClearParticles,
	PClassic_DrawParticles
};

#endif
