/*
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
	TODO:
		Lightmaps - skip ssbump stuff properly. currently extra lightstyles are screwed.
		Ent Lighting - leafs have some list of light 'cubes' that define lighting, allowing for ents to get backlit etc properly.
		Areaportals - the server 'needs' a way to specify areaportals on a per-player basis. for now the gamecode will have to explicitly force-open most of the portals in the game (map_noareas can be used as a workaround).
		Static Props - these are solid, but use the visible mesh for collisions instead of the special/simpler collision mesh. This makes it a bit easier to climb up them, which may be a gameplay issue.
		Detail Props - we don't attempt to handle these. They need manual batching or something (eg clutter-shader stuff). Their loss should not affect gameplay much as they can be disabled in HL2 too.
		Dynamic Lighting - r_dynamic not enabled.
		Realtime Lighting - screwed. Doesn't light all world surfaces for some reason.
		RBE(Bullet) - probably screwed.
		Skyboxes - doesn't handle the whole per-face skies nor weird hdr encoding.
		Fog - no fog here... this results in issues where the pvs provides distance culling.
		Load Times - materials are loaded on a single thread. this gets slow.
		Materials - all kinds of screwed.
		Portal2 Gels - we need to repurpose stainmap code or something.
		Refraction - this stuff is horribly expensive.
*/

#include "../plugin.h"
#include "quakedef.h"
#ifdef HAVE_CLIENT
#include "glquake.h"
#endif
#include "com_mesh.h"
#include "com_bih.h"

static plugfsfuncs_t		*filefuncs;
static plugmodfuncs_t		*modfuncs;
static plugthreadfuncs_t	*threadfuncs;

#define Q_strncpyz Q_strlcpy
float VectorNormalize2(const vec3_t in,vec3_t out) {float l = sqrt(DotProduct(in,in)); if (l) l = 1.0/l; VectorScale(in,l,out); return l;}
#define VectorNormalize(v) VectorNormalize2(v,v)
fte_inlinebody float M_LinearToSRGB(float x, float mag);
vec3_t vec3_origin;
static refdef_t *refdef;

static vec3_t modelorg;
static qbyte *frustumvis;
static int vbsp_nodesequence; //to track which nodes need walking
static int vbsp_surfsequence; //so we don't draw the same surface if its found multiple ways

r_qrenderer_t qrenderer = QR_OPENGL;

#define	MAX_VBSP_AREAS		MAX_Q2MAP_AREAS
#define SURF_OFFNODE		SURF_DRAWBACKGROUND	//might as well just reuse that.

static cvar_t *hl2_novis;
static cvar_t *hl2_displacement_scale;
static cvar_t *hl2_favour_ldr;
static cvar_t *map_noareas;
static cvar_t *map_autoopenportals;
static cvar_t *hl2_contents_remap;

char *Q_strlwr(char *s)
{
	char *ret=s;
	while(*s)
	{
		if (*s >= 'A' && *s <= 'Z')
			*s=*s-'A'+'a';
		s++;
	}

	return ret;
}

void AddPointToBounds (const vec3_t v, vec3_t mins, vec3_t maxs)
{
	int		i;
	vec_t	val;

	for (i=0 ; i<3 ; i++)
	{
		val = v[i];
		if (val < mins[i])
			mins[i] = val;
		if (val > maxs[i])
			maxs[i] = val;
	}
}

void ClearBounds (vec3_t mins, vec3_t maxs)
{
	mins[0] = mins[1] = mins[2] = FLT_MAX;
	maxs[0] = maxs[1] = maxs[2] = -FLT_MAX;
}


/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2
==================
*/
int VARGS BoxOnPlaneSide (const vec3_t emins, const vec3_t emaxs, const mplane_t *p)
{
	float	dist1, dist2;
	int		sides;

// general case
	switch (p->signbits)
	{
	default:
	case 0:
dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		break;
	case 1:
dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		break;
	case 2:
dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		break;
	case 3:
dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		break;
	case 4:
dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		break;
	case 5:
dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		break;
	case 6:
dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		break;
	case 7:
dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		break;
	}

	sides = 0;
	if (dist1 >= p->dist)
		sides = 1;
	if (dist2 < p->dist)
		sides |= 2;

	return sides;
}


#define Host_Error Sys_Errorf

enum hllumps_e
{	//note how this order matches q1 so well...
	//and yet the format is disturbingly similar to q2... huh... :P
	VLUMP_ENTITIES		= LUMP_ENTITIES,
	VLUMP_PLANES		= LUMP_PLANES,
	VLUMP_TEXTURES		= LUMP_TEXTURES,
	VLUMP_VERTEXES		= LUMP_VERTEXES,
	VLUMP_VISIBILITY	= LUMP_VISIBILITY,
	VLUMP_NODES			= LUMP_NODES,
	VLUMP_TEXINFO		= LUMP_TEXINFO,
	VLUMP_FACES_LDR		= LUMP_FACES,
	VLUMP_LIGHTING_LDR	= LUMP_LIGHTING,
//	VLUMP_FOO			= 9,//LUMP_CLIPNODES
	VLUMP_LEAFS			= LUMP_LEAFS,
//	VLUMP_FOO			= 11,//LUMP_MARKSURFACES
	VLUMP_EDGES			= LUMP_EDGES,
	VLUMP_SURFEDGES		= LUMP_SURFEDGES,
	VLUMP_MODELS		= LUMP_MODELS,
//	VLUMP_FOO			= 15,
	VLUMP_LEAFFACES		= 16,
	VLUMP_LEAFBRUSHES	= 17,
	VLUMP_BRUSHES		= 18,
	VLUMP_BRUSHSIDES	= 19,
	VLUMP_AREAS			= 20,
	VLUMP_AREAPORTALS	= 21,
//	VLUMP_FOO			= 22,
//	VLUMP_FOO			= 23,
//	VLUMP_FOO			= 24,
//	VLUMP_FOO			= 25,
	VLUMP_DISP_INFO		= 26,
//	VLUMP_FOO			= 27,
//	VLUMP_FOO			= 28,
//	VLUMP_FOO			= 29,
//	VLUMP_FOO			= 30,
//	VLUMP_FOO			= 31,
	VLUMP_DISP_LMALPHA	= 32,
	VLUMP_DISP_VERTS	= 33,
	VLUMP_DISP_LMCOORDS	= 34,
	VLUMP_GAMELUMP		= 35,
//	VLUMP_FOO			= 36,
//	VLUMP_FOO			= 37,
//	VLUMP_FOO			= 38,
//	VLUMP_FOO			= 39,

	VLUMP_ZIPFILE		= 40,
	VLUMP_AREAPORTALVERTS = 41,
//	VLUMP_FOO			= 42,
	VLUMP_STRINGDATA	= 43,
	VLUMP_STRINGOFFSETS	= 44,
//	VLUMP_FOO			= 45,
//	VLUMP_FOO			= 46,
//	VLUMP_FOO			= 47,
	VLUMP_DISP_TRIFLAGS	= 48,
//	VLUMP_FOO			= 49,
//	VLUMP_FOO			= 50,
	VLUMP_LEAFLIGHTI_HDR= 51,	//indexes into VLUMP_LEAFLIGHTV_HDR, two shorts per leaf.
	VLUMP_LEAFLIGHTI_LDR= 52,
	VLUMP_LIGHTING_HDR	= 53,
//	VLUMP_FOO			= 54,
	VLUMP_LEAFLIGHTV_HDR= 55,
	VLUMP_LEAFLIGHTV_LDR= 56,
//	VLUMP_FOO			= 57,
	VLUMP_FACES_HDR		= 58,
//	VLUMP_FOO			= 59,
//	VLUMP_FOO			= 60,
//	VLUMP_FOO			= 61,
//	VLUMP_FOO			= 62,
//	VLUMP_FOO			= 63,
	HL2_MAXLUMPS 		= 64
};
typedef struct {
	unsigned int fileofs;
	unsigned int filelen;
	unsigned int version;
	unsigned int fourcc;
} vlump_t;
typedef struct {
	unsigned int magic;
	unsigned int version;
	vlump_t lumps[HL2_MAXLUMPS];
} dvbspheader_t;

typedef struct
{
	int		numareaportals;
	int		firstareaportal;
} carea_t;
typedef struct
{
	int		floodnum;			// if two areas have equal floodnums, they are connected
	int		floodvalid;			// flags the area as having been visited (sequence numbers matching prv->floodvalid)
} careaflood_t;
typedef struct cmodel_s
{
	vec3_t		mins, maxs;
	vec3_t		origin;		// for sounds or lights
	mnode_t		*headnode;
	mleaf_t		*headleaf;
	int		numsurfaces;
	int		firstsurface;

	int firstbrush;
	int num_brushes;
} cmodel_t;
typedef struct dispinfo_s
{
    struct msurface_s *surf;
    vec3_t aamin;
    vec3_t aamax;
    pvscache_t pvs; //which pvs clusters this displacement is visible in...
    unsigned int contents;
    unsigned int width;
    unsigned int height;
    vecV_t *xyz;    //(width+1)*(height+1)
    index_t *idx;   //width*height*6;
    size_t numindexes;
    struct dispvert_s
    {
        vec3_t norm;
        vec2_t st;
        float alpha;
    } *verts;
    //unsigned short *flags;
} dispinfo_t;

typedef struct vbspinfo_s
{
	struct
	{
		qbyte *vis;
		pvsbuffer_t visbuf;
		int viewcluster[2];
	} vcache;

	int				numbrushsides;
	q2cbrushside_t *brushsides;

	q2mapsurface_t	*surfaces;
	dispinfo_t		**surfdisp;

	int				numleafbrushes;
	q2cbrush_t		**leafbrushes;

	int				numcmodels;
	cmodel_t		*cmodels;

	int				numbrushes;
	q2cbrush_t		*brushes;

	int				numvisibility;
	q2dvis_t		*vis;
	qbyte			*phscalced;

	struct vbsptexinfo_s
	{
		vec4_t lmvecs[2];
	} *texinfo;

	int				numareas;
	int				floodvalid;
	careaflood_t	areaflood[MAX_VBSP_AREAS];
	//areas have a list of portals that open into other areas.
	carea_t			*areas;	//indexes into q2areaportals for flooding
	size_t			numareaportals;
	q2dareaportal_t	*areaportals;

	//and this is the state that is actually changed. booleans.
	qbyte			portalopen[MAX_Q2MAP_AREAPORTALS];	//memset will work if it's a qbyte, really it should be a qboolean
	mplane_t		**portalplane;
	qbyte			portalquerying[MAX_Q2MAP_AREAPORTALS];
	mesh_t			*portalpoly;		//[numq2areaportals]
	int				*occlusionqueries; //[numq2areaportals]

	struct mleaflight_s
	{
		struct leaflightpoint_s
		{
			vec3_t rgb[6];
			qbyte x, y, z;
		} *point;
		int count;
	} *leaflight;

	size_t numdisplacements;
	dispinfo_t *displacements;

	size_t numstaticprops;
	struct staticprop_s
	{
		float fademindist;
		float fademaxdist;
		qboolean solid;
		struct bihtransform_s transform;
		vec3_t	lightorg;
		entity_t ent;
	} *staticprops;

	unsigned int contentsremap[32];
} vbspinfo_t;

static q2mapsurface_t	nullsurface;

static int		VBSP_NumInlineModels (model_t *model);
static cmodel_t	*VBSP_InlineModel (model_t *model, char *name);
static void VBSP_FinalizeBrush(q2cbrush_t *brush);
static void	FloodAreaConnections (vbspinfo_t	*prv);

/*
===============================================================================

					MAP LOADING

===============================================================================
*/

static unsigned int VBSP_TranslateContentBits(vbspinfo_t *prv, unsigned int source)
{
	unsigned int ret = 0;
	if (source & 0x0000ffff)
	{
		if (source & 0x0000000f)
		{
			if (source & 0x00000001)	ret |= prv->contentsremap[ 0];
			if (source & 0x00000002)	ret |= prv->contentsremap[ 1];
			if (source & 0x00000004)	ret |= prv->contentsremap[ 2];
			if (source & 0x00000008)	ret |= prv->contentsremap[ 3];
		}
		if (source & 0x000000f0)
		{
			if (source & 0x00000010)	ret |= prv->contentsremap[ 4];
			if (source & 0x00000020)	ret |= prv->contentsremap[ 5];
			if (source & 0x00000040)	ret |= prv->contentsremap[ 6];
			if (source & 0x00000080)	ret |= prv->contentsremap[ 7];
		}
		if (source & 0x000000f00)
		{
			if (source & 0x00000100)	ret |= prv->contentsremap[ 8];
			if (source & 0x00000200)	ret |= prv->contentsremap[ 9];
			if (source & 0x00000400)	ret |= prv->contentsremap[10];
			if (source & 0x00000800)	ret |= prv->contentsremap[11];
		}
		if (source & 0x000000f00)
		{
			if (source & 0x000001000)	ret |= prv->contentsremap[12];
			if (source & 0x000002000)	ret |= prv->contentsremap[13];
			if (source & 0x000004000)	ret |= prv->contentsremap[14];
			if (source & 0x000008000)	ret |= prv->contentsremap[15];
		}
	}
	if (source & 0xffff0000)
	{
		if (source & 0x000f0000)
		{
			if (source & 0x00010000)	ret |= prv->contentsremap[16];
			if (source & 0x00020000)	ret |= prv->contentsremap[17];
			if (source & 0x00040000)	ret |= prv->contentsremap[18];
			if (source & 0x00080000)	ret |= prv->contentsremap[19];
		}
		if (source & 0x00f00000)
		{
			if (source & 0x00100000)	ret |= prv->contentsremap[20];
			if (source & 0x00200000)	ret |= prv->contentsremap[21];
			if (source & 0x00400000)	ret |= prv->contentsremap[22];
			if (source & 0x00800000)	ret |= prv->contentsremap[23];
		}
		if (source & 0x0f000000)
		{
			if (source & 0x01000000)	ret |= prv->contentsremap[24];
			if (source & 0x02000000)	ret |= prv->contentsremap[25];
			if (source & 0x04000000)	ret |= prv->contentsremap[26];
			if (source & 0x08000000)	ret |= prv->contentsremap[27];
		}
		if (source & 0xf0000000)
		{
			if (source & 0x10000000)	ret |= prv->contentsremap[28];
			if (source & 0x20000000)	ret |= prv->contentsremap[29];
			if (source & 0x40000000)	ret |= prv->contentsremap[30];
			if (source & 0x80000000)	ret |= prv->contentsremap[31];
		}
	}
	return ret;
}
static void VBSP_TranslateContentBits_Setup(vbspinfo_t *prv)
{
	size_t i, j;
	char contname[64];
	const char *contremap = hl2_contents_remap->string;
	static const struct {
		const char *name;
		unsigned int contents;
	} knowncontents[] =
	{
		{"empty", FTECONTENTS_EMPTY},
		{"solid", FTECONTENTS_SOLID},
		{"window", FTECONTENTS_WINDOW},
		{"lava", FTECONTENTS_LAVA},
		{"slime", FTECONTENTS_SLIME},
		{"water", FTECONTENTS_WATER},
		{"ladder", FTECONTENTS_LADDER},
		{"playerclip", FTECONTENTS_PLAYERCLIP},
		{"monsterclip", FTECONTENTS_MONSTERCLIP},
		{"clip", FTECONTENTS_PLAYERCLIP|FTECONTENTS_MONSTERCLIP},
		{"body", FTECONTENTS_BODY},
		{"corpse", FTECONTENTS_CORPSE},
		{"detail", FTECONTENTS_DETAIL},
		{"sky", FTECONTENTS_SKY},
		{"Q2SOLID", Q2CONTENTS_SOLID},
		{"Q2WINDOW", Q2CONTENTS_WINDOW},
		{"Q2AUX", Q2CONTENTS_AUX},
		{"Q2LAVA", Q2CONTENTS_LAVA},
		{"Q2SLIME", Q2CONTENTS_SLIME},
		{"Q2WATER", Q2CONTENTS_WATER},
		{"Q2MIST", Q2CONTENTS_MIST},
		{"Q2AREAPORTAL", Q2CONTENTS_AREAPORTAL},
		{"Q2PLAYERCLIP", Q2CONTENTS_PLAYERCLIP},
		{"Q2MONSTERCLIP", Q2CONTENTS_MONSTERCLIP},
		{"Q2CURRENT_0", Q2CONTENTS_CURRENT_0},
		{"Q2CURRENT_90", Q2CONTENTS_CURRENT_90},
		{"Q2CURRENT_180", Q2CONTENTS_CURRENT_180},
		{"Q2CURRENT_270", Q2CONTENTS_CURRENT_270},
		{"Q2CURRENT_UP", Q2CONTENTS_CURRENT_UP},
		{"Q2CURRENT_DOWN", Q2CONTENTS_CURRENT_DOWN},
		{"Q2ORIGIN", Q2CONTENTS_ORIGIN},
		{"Q2MONSTER", Q2CONTENTS_MONSTER},
		{"Q2DEADMONSTER", Q2CONTENTS_DEADMONSTER},
		{"Q2DETAIL", Q2CONTENTS_DETAIL},
		{"Q2TRANSLUCENT", Q2CONTENTS_TRANSLUCENT},
		{"Q2LADDER", Q2CONTENTS_LADDER},
		{"Q3SOLID", Q3CONTENTS_SOLID},
		{"Q3LAVA", Q3CONTENTS_LAVA},
		{"Q3SLIME", Q3CONTENTS_SLIME},
		{"Q3WATER", Q3CONTENTS_WATER},
		{"Q3NOTTEAM1", Q3CONTENTS_NOTTEAM1},
		{"Q3NOTTEAM2", Q3CONTENTS_NOTTEAM2},
		{"Q3NOBOTCLIP", Q3CONTENTS_NOBOTCLIP},
		{"Q3AREAPORTAL", Q3CONTENTS_AREAPORTAL},
		{"Q3PLAYERCLIP", Q3CONTENTS_PLAYERCLIP},
		{"Q3MONSTERCLIP", Q3CONTENTS_MONSTERCLIP},
		{"Q3TELEPORTER", Q3CONTENTS_TELEPORTER},
		{"Q3JUMPPAD", Q3CONTENTS_JUMPPAD},
		{"Q3CLUSTERPORTAL", Q3CONTENTS_CLUSTERPORTAL},
		{"Q3DONOTENTER", Q3CONTENTS_DONOTENTER},
		{"Q3BOTCLIP", Q3CONTENTS_BOTCLIP},
		{"Q3MOVER", Q3CONTENTS_MOVER},
		{"Q3ORIGIN", Q3CONTENTS_ORIGIN},
		{"Q3BODY", Q3CONTENTS_BODY},
		{"Q3CORSE", Q3CONTENTS_CORPSE},
		{"Q3DETAIL", Q3CONTENTS_DETAIL},
		{"Q3STRUCTURAL", Q3CONTENTS_STRUCTURAL},
		{"Q3TRANSLUCENT", Q3CONTENTS_TRANSLUCENT},
		{"Q3TRIGGER", Q3CONTENTS_TRIGGER},
		{"Q3NODROP", Q3CONTENTS_NODROP},
	};
	if (!*contremap)
		for (i = 0; i < 32; i++)
			prv->contentsremap[i] = 1<<i;
	else for (i = 0; i < 32; i++)
	{
		contremap = cmdfuncs->ParseToken(contremap, contname, sizeof(contname), NULL);
		if (!contremap || !*contname)
			prv->contentsremap[i] = 0;
		else
		{
			char *tmp;
			int bit = strtol(contname, &tmp, 10);
			if (!*tmp)
			{
				if (bit >= 0)
					prv->contentsremap[i] = 1<<bit;
			}
			else
			{
				for (j = 0; j < countof(knowncontents); j++)
				{
					if (!Q_strcasecmp(contname, knowncontents[j].name))
					{
						prv->contentsremap[i] = knowncontents[j].contents;
						break;
					}
				}
				if (j == countof(knowncontents))
					Con_Printf(CON_WARNING"%s: Unknown bit name %s\n", hl2_contents_remap->name, contname);
			}
		}
	}
}

static void VBSP_SetParent (mnode_t *node, mnode_t *parent)
{
	node->parent = parent;
	if (node->contents != -1)
		return;
	VBSP_SetParent (node->children[0], node);
	VBSP_SetParent (node->children[1], node);
}

static void VBSP_FindBrushRange (vbspinfo_t	*prv, mnode_t *node, size_t *firstbrush, size_t *lastbrush)
{
	size_t u, b;
	mleaf_t *leaf;
	while (node->contents == -1)
	{	//walk every node to find every leaf...
		VBSP_FindBrushRange(prv, node->children[0], firstbrush, lastbrush);
		node = node->children[1];
	}

	leaf = (mleaf_t*)node;
	for (u = 0; u < leaf->numleafbrushes; u++)
	{
		b = prv->leafbrushes[leaf->firstleafbrush+u]-prv->brushes;
		if (*firstbrush > b)
			*firstbrush = b;
		if (*lastbrush < b+1)
			*lastbrush = b+1;
	}
}

static qboolean VBSP_LoadVertexes (model_t *loadmodel, qbyte *mod_base, vlump_t *l)
{
	dvertex_t	*in;
	mvertex_t	*out;
	size_t			i, count;

	in = (void *)(mod_base + l->fileofs);
	count = l->filelen / sizeof(*in);
	if (l->filelen % sizeof(*in) || count > SANITY_LIMIT(*out))
	{
		Con_Printf (CON_ERROR "VBSP_LoadVertexes: funny lump size in %s\n", loadmodel->name);
		return false;
	}
	out = plugfuncs->GMalloc(&loadmodel->memgroup, count*sizeof(*out));

	loadmodel->vertexes = out;
	loadmodel->numvertexes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->position[0] = LittleFloat (in->point[0]);
		out->position[1] = LittleFloat (in->point[1]);
		out->position[2] = LittleFloat (in->point[2]);
	}

	return true;
}
static qboolean VBSP_LoadEdges (model_t *loadmodel, qbyte *mod_base, vlump_t *l)
{
	medge_t *out;
	size_t 	i, count;

	dsedge_t *in = (void *)(mod_base + l->fileofs);
	count = l->filelen / sizeof(*in);
	if (l->filelen % sizeof(*in) || count > SANITY_LIMIT(*out))
	{
		Con_Printf ("VBSP_LoadEdges: funny lump size in %s\n", loadmodel->name);
		return false;
	}
	out = plugfuncs->GMalloc(&loadmodel->memgroup, (count + 1) * sizeof(*out));

	loadmodel->edges = out;
	loadmodel->numedges = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->v[0] = (unsigned short)LittleShort(in->v[0]);
		out->v[1] = (unsigned short)LittleShort(in->v[1]);
	}

	return true;
}
static qboolean VBSP_LoadSurfedges (model_t *loadmodel, qbyte *mod_base, vlump_t *l)
{
	size_t		i, count;
	unsigned int		*in, *out;

	in = (void *)(mod_base + l->fileofs);
	count = l->filelen / sizeof(*in);
	if (l->filelen % sizeof(*in) || count > SANITY_LIMIT(*out))
	{
		Con_Printf (CON_ERROR "VBSP_LoadSurfedges: funny lump size in %s\n",loadmodel->name);
		return false;
	}
	out = plugfuncs->GMalloc(&loadmodel->memgroup, count*sizeof(*out));

	loadmodel->surfedges = out;
	loadmodel->numsurfedges = count;

	for ( i=0 ; i<count ; i++)
		out[i] = LittleLong (in[i]);

	return true;
}
static qboolean VBSP_LoadMarksurfaces (model_t *loadmodel, qbyte *mod_base, vlump_t *l)
{
	size_t		i, j, count;
	msurface_t **out;

	unsigned short		*ins;
	ins = (void *)(mod_base + l->fileofs);
	count = l->filelen / sizeof(*ins);
	if (l->filelen % sizeof(*ins) || count > SANITY_LIMIT(*out))
	{
		Con_Printf (CON_ERROR "VBSP_LoadMarksurfaces: funny lump size in %s\n",loadmodel->name);
		return false;
	}
	out = plugfuncs->GMalloc(&loadmodel->memgroup, count*sizeof(*out));

	loadmodel->marksurfaces = out;
	loadmodel->nummarksurfaces = count;

	for ( i=0 ; i<count ; i++)
	{
		j = (unsigned short)LittleShort(ins[i]);
		if (j >= loadmodel->numsurfaces)
		{
			Con_Printf (CON_ERROR "VBSP_LoadMarksurfaces: bad surface number\n");
			return false;
		}
		out[i] = loadmodel->surfaces + j;
	}

	return true;
}
static qboolean VBSP_LoadEntities (model_t *loadmodel, qbyte *mod_base, vlump_t *l)
{
	return modfuncs->LoadEntities(loadmodel, mod_base+l->fileofs, l->filelen);
}

/*
=================
CMod_LoadSubmodels
=================
*/
static qboolean VBSP_LoadSubmodels (model_t *loadmodel, qbyte *mod_base, vlump_t *l)
{
	vbspinfo_t	*prv = (vbspinfo_t*)loadmodel->meshinfo;
	q2dmodel_t	*in;
	cmodel_t	*out;
	int			i, j, count;
	size_t		firstbrush, lastbrush;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "VBSP_LoadSubmodels: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count < 1)
	{
		Con_Printf (CON_ERROR "Map with no models\n");
		return false;
	}
	if (count > SANITY_MAX_Q2MAP_MODELS)
	{
		Con_Printf (CON_ERROR "Map has too many models\n");
		return false;
	}

	out = prv->cmodels = plugfuncs->GMalloc(&loadmodel->memgroup, count * sizeof(*prv->cmodels));
	prv->numcmodels = count;

	for (i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat (in->mins[j]) - 1;
			out->maxs[j] = LittleFloat (in->maxs[j]) + 1;
			out->origin[j] = LittleFloat (in->origin[j]);
		}
		out->headnode = loadmodel->nodes + LittleLong (in->headnode);
		out->firstsurface = LittleLong (in->firstface);
		out->numsurfaces = LittleLong (in->numfaces);


		firstbrush = ~0u;
		lastbrush = 0u;
		VBSP_FindBrushRange(prv, out->headnode, &firstbrush, &lastbrush);
		if (lastbrush > firstbrush)
		{
			out->firstbrush = firstbrush;
			out->num_brushes = lastbrush-firstbrush;
		}
	}

	AddPointToBounds(prv->cmodels[0].mins, loadmodel->mins, loadmodel->maxs);
	AddPointToBounds(prv->cmodels[0].maxs, loadmodel->mins, loadmodel->maxs);

	return true;
}

/*
=================
CMod_LoadBrushes

=================
*/
static qboolean VBSP_LoadBrushes (model_t *mod, qbyte *mod_base, vlump_t *l)
{
	vbspinfo_t	*prv = (vbspinfo_t*)mod->meshinfo;
	q2dbrush_t	*in;
	q2cbrush_t	*out;
	int			i, count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "VBSP_LoadBrushes: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count > SANITY_LIMIT(*out))
	{
		Con_Printf (CON_ERROR "Map has too many brushes");
		return false;
	}

	prv->brushes = plugfuncs->GMalloc(&mod->memgroup, sizeof(*out) * (count+1));

	out = prv->brushes;

	prv->numbrushes = count;

	for (i=0 ; i<count ; i++, out++, in++)
	{
		//FIXME: missing bounds checks
		out->brushside = &prv->brushsides[LittleLong(in->firstside)];
		out->numsides = LittleLong(in->numsides);
		out->contents = VBSP_TranslateContentBits(prv, LittleLong(in->contents));
		VBSP_FinalizeBrush(out);
	}

	return true;
}

/*
=================
CMod_LoadPlanes
=================
*/
static qboolean VBSP_LoadPlanes (model_t *mod, qbyte *mod_base, vlump_t *l)
{
	int			i, j;
	mplane_t	*out;
	dplane_t 	*in;
	int			count;
	int			bits;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "VBSP_LoadPlanes: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count < 1)
	{
		Con_Printf (CON_ERROR "Map with no planes\n");
		return false;
	}
	// need to save space for box planes
	if (count > SANITY_LIMIT(*out))
	{
		Con_Printf (CON_ERROR "Map has too many planes (%i)\n", count);
		return false;
	}

	mod->planes = out = plugfuncs->GMalloc(&mod->memgroup, sizeof(*out) * count);
	mod->numplanes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		bits = 0;
		for (j=0 ; j<3 ; j++)
		{
			out->normal[j] = LittleFloat (in->normal[j]);
			if (out->normal[j] < 0)
				bits |= 1<<j;
		}

		out->dist = LittleFloat (in->dist);
		out->type = LittleLong (in->type);
		out->signbits = bits;
	}

	return true;
}

/*
=================
CMod_LoadLeafBrushes
=================
*/
static qboolean VBSP_LoadLeafBrushes (model_t *mod, qbyte *mod_base, vlump_t *l)
{
	vbspinfo_t	*prv = (vbspinfo_t*)mod->meshinfo;
	int			i;
	q2cbrush_t	**out;
	unsigned short 	*in;
	int			count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "VBSP_LoadLeafBrushes: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count < 1)
	{
		Con_Printf (CON_ERROR "Map with no planes\n");
		return false;
	}
	// need to save space for box planes
	if (count > SANITY_LIMIT(**out))
	{
		Con_Printf (CON_ERROR "Map has too many leafbrushes\n");
		return false;
	}

	//prv->numbrushes is because of submodels being weird.
	out = prv->leafbrushes = plugfuncs->GMalloc(&mod->memgroup, sizeof(*out) * (count+prv->numbrushes));
	prv->numleafbrushes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
		*out = prv->brushes + (unsigned short)(short)LittleShort (*in);

	return true;
}

/*
=================
CMod_LoadAreas
=================
*/
static qboolean VBSP_LoadAreas (model_t *mod, qbyte *mod_base, vlump_t *l)
{
	vbspinfo_t	*prv = (vbspinfo_t*)mod->meshinfo;
	int			i;
	carea_t	*out;
	q2darea_t 	*in;
	int			count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "VBSP_LoadAreas: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count > MAX_Q2MAP_AREAS)
	{
		Con_Printf (CON_ERROR "Map has too many areas\n");
		return false;
	}

	out = prv->areas = plugfuncs->GMalloc(&mod->memgroup, sizeof(*out) * count);
	prv->numareas = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->numareaportals = LittleLong (in->numareaportals);
		out->firstareaportal = LittleLong (in->firstareaportal);
	}

	return true;
}

/*
=================
CMod_LoadVisibility
=================
*/
static qboolean VBSP_LoadVisibility (model_t *mod, qbyte *mod_base, vlump_t *l)
{
	vbspinfo_t	*prv = (vbspinfo_t*)mod->meshinfo;
	int		i;

	prv->numvisibility = l->filelen;
//	if (l->filelen > MAX_Q2MAP_VISIBILITY)
//	{
//		Con_Printf (CON_ERROR "Map has too large visibility lump\n");
//		return false;
//	}

	prv->vis = plugfuncs->GMalloc(&mod->memgroup, l->filelen);
	memcpy (prv->vis, mod_base + l->fileofs, l->filelen);

	mod->vis = prv->vis;

	prv->vis->numclusters = LittleLong (prv->vis->numclusters);
	for (i=0 ; i<prv->vis->numclusters ; i++)
	{
		prv->vis->bitofs[i][0] = LittleLong (prv->vis->bitofs[i][0]);
		prv->vis->bitofs[i][1] = LittleLong (prv->vis->bitofs[i][1]);
	}
	mod->numclusters = prv->vis->numclusters;
	mod->pvsbytes = ((mod->numclusters + 31)>>3)&~3;

	return true;
}

/*
static qbyte *CM_LeafnumPVS (model_t *model, int leafnum, qbyte *buffer, unsigned int buffersize)
{
	return CM_ClusterPVS(model, CM_LeafCluster(model, leafnum), buffer, buffersize);
}
*/

#ifdef HAVE_CLIENT

/*extern int	r_dlightframecount;
static void VBSP_MarkLights (dlight_t *light, dlightbitmask_t bit, mnode_t *node)
{
	mplane_t	*splitplane;
	float		dist;
	msurface_t	*surf;
	int			i;

	if (node->contents != -1)
	{
		mleaf_t *leaf = (mleaf_t *)node;
		msurface_t **mark;

		i = leaf->nummarksurfaces;
		mark = leaf->firstmarksurface;
		while(i--!=0)
		{
			surf = *mark++;
			if (surf->dlightframe != r_dlightframecount)
			{
				surf->dlightbits = 0;
				surf->dlightframe = r_dlightframecount;
			}
			surf->dlightbits |= bit;
		}
		return;
	}

	splitplane = node->plane;
	dist = DotProduct (light->origin, splitplane->normal) - splitplane->dist;

	if (dist > light->radius)
	{
		VBSP_MarkLights (light, bit, node->children[0]);
		return;
	}
	if (dist < -light->radius)
	{
		VBSP_MarkLights (light, bit, node->children[1]);
		return;
	}

// mark the polygons
	surf = cl.worldmodel->surfaces + node->firstsurface;
	for (i=0 ; i<node->numsurfaces ; i++, surf++)
	{
		if (surf->dlightframe != r_dlightframecount)
		{
			surf->dlightbits = 0u;
			surf->dlightframe = r_dlightframecount;
		}
		surf->dlightbits |= bit;
	}

	VBSP_MarkLights (light, bit, node->children[0]);
	VBSP_MarkLights (light, bit, node->children[1]);
}

static void VBSP_StainNode (mnode_t *node, float *parms)
{
	mplane_t	*splitplane;
	float		dist;
	msurface_t	*surf;
	int			i;

	if (node->contents != -1)
		return;

	splitplane = node->plane;
	dist = DotProduct ((parms+1), splitplane->normal) - splitplane->dist;

	if (dist > (*parms))
	{
		VBSP_StainNode (node->children[0], parms);
		return;
	}
	if (dist < (-*parms))
	{
		VBSP_StainNode (node->children[1], parms);
		return;
	}

// mark the polygons
	surf = cl.worldmodel->surfaces + node->firstsurface;
	for (i=0 ; i<node->numsurfaces ; i++, surf++)
	{
		if (surf->flags&~(SURF_DONTWARP|SURF_PLANEBACK))
			continue;
		Surf_StainSurf(surf, parms);
	}

	VBSP_StainNode (node->children[0], parms);
	VBSP_StainNode (node->children[1], parms);
}
*/

#endif

typedef struct
{
	float		vecs[2][4];		// [s/t][xyz offset]
	float		lmvecs[2][4];	//well that's awkward
	int			flags;			// miptex flags + overrides
	int			textureindex;
} hltexinfo_t;
static qboolean VBSP_LoadSurfaces (model_t *mod, qbyte *mod_base, vlump_t *l)
{
	vbspinfo_t	*prv = (vbspinfo_t*)mod->meshinfo;
	hltexinfo_t	*in;
	q2mapsurface_t	*out;
	int			i, count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "VBSP_LoadSurfaces: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);
	if (count < 1)
	{
		Con_Printf (CON_ERROR "Map with no surfaces\n");
		return false;
	}
//	if (count > MAX_Q2MAP_TEXINFO)
//		Host_Error ("Map has too many surfaces");

	mod->numtexinfo = count;
	out = prv->surfaces = plugfuncs->GMalloc(&mod->memgroup, count * sizeof(*prv->surfaces));

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		Q_strncpyz (out->c.name, "FIXME", sizeof(out->c.name));
		Q_strncpyz (out->rname, "FIXME", sizeof(out->rname));
		out->c.flags = LittleLong (in->flags);
		out->c.value = 0;
	}

	return true;
}

typedef struct
{
	vec3_t reflectivity;		//not very useful to us.
	unsigned int stringindex;
	unsigned int width;
	unsigned int height;
	unsigned int width2;	//no idea why there's two of these.
	unsigned int height2;
} hltexture_t;
#define TIHL2_LIGHT			TI_LIGHT
#define TIHL2_SKYBOX		0x2
#define TIHL2_SKYROOM		0x4
#define TIHL2_WARP			TI_WARP

#define	TIHL2_TRANS			TI_TRANS33
#define TIHL2_NOPORTAL		0x20
#define TIHL2_TRIGGER		0x40
#define TIHL2_NODRAW		TI_NODRAW
#define TIHL2_HINT		0x100
#define TIHL2_SKIP		0x200
#define TIHL2_NOLIGHT		0x400
//#define TIHL2_BUMPLIGHT	0x800
//#define TIHL2_NOSHADOWS	0x1000
//#define TIHL2_NODECALS	0x2000
//#define TIHL2_NOCHOP		0x4000
//#define TIHL2_HITBOX		0x8000

static qboolean VBSP_LoadTexInfo (model_t *mod, qbyte *mod_base, vlump_t *lumps, char *mapname)
{	//texinfo->textures->stringoffsets->strings. gah, so many lumps just to find the texture name to use!
	vbspinfo_t	*prv = (vbspinfo_t*)mod->meshinfo;
	hltexinfo_t *in;
	mtexinfo_t *out;
	int 	i, j, count;
	char	sname[256];
	int texcount;
	hltexture_t *textures = (void*)(mod_base + lumps[VLUMP_TEXTURES].fileofs);
	unsigned int *stringoffsets = (void*)(mod_base + lumps[VLUMP_STRINGOFFSETS].fileofs);
	char *strings = mod_base + lumps[VLUMP_STRINGDATA].fileofs;
	unsigned int flags;

	in = (void *)(mod_base + lumps[VLUMP_TEXINFO].fileofs);
	if (lumps[VLUMP_TEXINFO].filelen % sizeof(*in))
	{
		Con_Printf ("VBSP_LoadTexInfo: funny lump size in %s\n", mod->name);
		return false;
	}
	count = lumps[VLUMP_TEXINFO].filelen / sizeof(*in);
	out = plugfuncs->GMalloc(&mod->memgroup, count*sizeof(*out));
	prv->texinfo = plugfuncs->GMalloc(&mod->memgroup, count*sizeof(*prv->texinfo));

	mod->textures = plugfuncs->GMalloc(&mod->memgroup, sizeof(texture_t *)*count);
	texcount = 0;

	mod->texinfo = out;
	mod->numtexinfo = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		hltexture_t *texture = ((in->textureindex>=0)?textures + in->textureindex:NULL);
		char *texturename = (texture?strings + stringoffsets[texture->stringindex]:"INVALID");
		flags = LittleLong (in->flags);

		for (j=0 ; j<4 ; j++)
			out->vecs[0][j] = LittleFloat (in->vecs[0][j]);
		for (j=0 ; j<4 ; j++)
			out->vecs[1][j] = LittleFloat (in->vecs[1][j]);
		out->vecscale[0] = 1.0/Length (out->vecs[0]);
		out->vecscale[1] = 1.0/Length (out->vecs[1]);

		for (j=0 ; j<4 ; j++)
			prv->texinfo[i].lmvecs[0][j] = LittleFloat (in->lmvecs[0][j]);
		for (j=0 ; j<4 ; j++)
			prv->texinfo[i].lmvecs[1][j] = LittleFloat (in->lmvecs[1][j]);

		if (flags & (TIHL2_SKYBOX|TIHL2_SKYROOM))
			Q_snprintfz(sname, sizeof(sname), "sky/%s", texturename);
		else
			Q_snprintfz(sname, sizeof(sname), "%s", texturename);
		if (flags & (TIHL2_WARP))
			Q_strncatz(sname, "#WARP", sizeof(sname));
//		if (out->flags & TIHL2_FLOWING)
//			Q_strncatz(sname, "#FLOW", sizeof(sname));
//		if (out->flags & TIHL2_TRANS66)
//			Q_strncatz(sname, "#ALPHA=0.66", sizeof(sname));
		else if (out->flags & TIHL2_TRANS)
			Q_strncatz(sname, "#ALPHA=1", sizeof(sname));
//		else if (out->flags & (TIHL2_WARP))
//			Q_strncatz(sname, "#ALPHA=1", sizeof(sname));

		if (flags & (TIHL2_SKYBOX|TIHL2_SKYROOM))
			out->flags |= TI_SKY;

		if (flags & (TIHL2_WARP))
			out->flags |= TI_WARP;

		if (flags & TIHL2_NOLIGHT)
			out->flags |= TEX_SPECIAL;

		if (flags & TIHL2_NODRAW)
			out->flags |= TI_NODRAW;

// 		if (flags & TIHL2_HINT)
// 			out->flags |= TI_HINT;

// 		if (flags & TIHL2_SKIP)
// 			out->flags |= TI_SKIP;

		//compact the textures.
		for (j=0; j < texcount; j++)
		{
			if (!Q_strcasecmp(sname, mod->textures[j]->name))
			{
				out->texture = mod->textures[j];
				break;
			}
		}
		if (j == texcount)	//load a new one
		{
			out->texture = plugfuncs->GMalloc(&mod->memgroup, sizeof(texture_t));
			Q_strncpyz(out->texture->name, sname, sizeof(out->texture->name));
			Q_strlwr(out->texture->name);
			if (texture)
			{
				out->texture->vwidth = texture->width;
				out->texture->vheight = texture->height;
			}
			else
			{
				out->texture->vwidth = out->texture->vheight = 128;
			}

			mod->textures[texcount++] = out->texture;
		}
	}

	mod->numtextures = texcount;

	return true;
}

typedef struct
{
	//shared with q2dnode_t
	int			planenum;
	int			children[2];	// negative numbers are -(leafs+1), not nodes
	short		mins[3];		// for frustom culling
	short		maxs[3];
	unsigned short	firstface;
	unsigned short	numfaces;	// counting both sides

	//new for hl2
	unsigned short	area;
	unsigned short pad;
} hl2dnode_t;
static qboolean VBSP_LoadNodes (model_t *mod, qbyte *mod_base, vlump_t *l)
{
	hl2dnode_t *in;
	int			child;
	mnode_t		*out;
	int			i, j, count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "VBSP_LoadNodes: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count < 1)
	{
		Con_Printf (CON_ERROR "Map has no nodes\n");
		return false;
	}
	if (count > SANITY_LIMIT(*out))
	{
		Con_Printf (CON_ERROR "Map has too many nodes\n");
		return false;
	}

	out = plugfuncs->GMalloc(&mod->memgroup, sizeof(mnode_t)*count);

	mod->nodes = out;
	mod->numnodes = count;

	for (i=0 ; i<count ; i++, out++, in++)
	{
		memset(out, 0, sizeof(*out));

		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleShort (in->mins[j]);
			out->minmaxs[3+j] = LittleShort (in->maxs[j]);
		}

		out->plane = mod->planes + LittleLong(in->planenum);

		out->firstsurface = (unsigned short)LittleShort (in->firstface);
		out->numsurfaces = (unsigned short)LittleShort (in->numfaces);
		out->contents = -1;	// differentiate from leafs

		for (j=0 ; j<2 ; j++)
		{
			child = LittleLong (in->children[j]);
			out->childnum[j] = child;
			if (child < 0)
				out->children[j] = (mnode_t *)(mod->leafs + -1-child);
			else
				out->children[j] = mod->nodes + child;
		}
	}

	VBSP_SetParent (mod->nodes, NULL);	// sets nodes and leafs

	return true;
}
typedef struct
{
	//copied from q2dleaf_t
	int				contents;			// OR of all brushes (NOTE: hl2 doesn't seem to have these all set properly, at least for detail brushes)

	short			cluster;
	short			area;

	short			mins[3];			// for frustum culling
	short			maxs[3];

	unsigned short	firstleafface;
	unsigned short	numleaffaces;

	unsigned short	firstleafbrush;
	unsigned short	numleafbrushes;

	//new for hl2
	short leafwaterid;
	struct	//present in v19. gone in v20.
	{
		qbyte rgb[3];
		signed char e;
	} light[6];
	short pad;
} hl2dleaf_t;

static qboolean VBSP_LoadLeafs (model_t *mod, qbyte *mod_base, vlump_t *l, int ver)
{
	vbspinfo_t	*prv = (vbspinfo_t*)mod->meshinfo;
	int			i, j;
	mleaf_t		*out;
	hl2dleaf_t	*in;
	int			count;
	size_t		insize = sizeof(*in);
	struct leaflightpoint_s *lightpoint = NULL;

	if (ver == 19)
		insize = 56;	//older maps have some lighting info here.
	else
		insize = 32;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % insize)
	{
		Con_Printf (CON_ERROR "VBSP_LoadLeafs: funny lump size\n");
		return false;
	}
	count = l->filelen / insize;

	if (count < 1)
	{
		Con_Printf (CON_ERROR "Map with no leafs\n");
		return false;
	}
	// need to save space for box planes
	if (count > SANITY_LIMIT(*out))
	{
		Con_Printf (CON_ERROR "Map has too many leafs\n");
		return false;
	}

	out = plugfuncs->GMalloc(&mod->memgroup, sizeof(*out) * (count+1));
	mod->numclusters = 0;

	mod->leafs = out;
	mod->numleafs = count;

	if (ver == 19)
	{
		prv->leaflight = plugfuncs->GMalloc(&mod->memgroup, sizeof(*out) * count);
		lightpoint = plugfuncs->GMalloc(&mod->memgroup, sizeof(*lightpoint) * count);
	}

	for ( i=0 ; i<count ; i++, in = (hl2dleaf_t*)((qbyte*)in+insize), out++)
	{
		memset(out, 0, sizeof(*out));

		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleShort (in->mins[j]);
			out->minmaxs[3+j] = LittleShort (in->maxs[j]);
		}

		out->contents = VBSP_TranslateContentBits(prv,LittleLong (in->contents));
		out->cluster = (unsigned short)LittleShort (in->cluster);
		if (out->cluster == 0xffff)
			out->cluster = -1;

		out->area = (unsigned short)LittleShort (in->area);
		out->area &= 0x1ff;		//upper part is flags.
		out->firstleafbrush = (unsigned short)LittleShort (in->firstleafbrush);
		out->numleafbrushes = (unsigned short)LittleShort (in->numleafbrushes);

		out->firstmarksurface = mod->marksurfaces +
			(unsigned short)LittleShort(in->firstleafface);
		out->nummarksurfaces = (unsigned short)LittleShort(in->numleaffaces);

		if (out->cluster >= mod->numclusters)
			mod->numclusters = out->cluster + 1;

		if (lightpoint)
		{
			for (j = 0; j < 6; j++)
			{
				float e = pow(2, in->light[j].e);
				lightpoint->rgb[j][0] = e * in->light[j].rgb[0];
				lightpoint->rgb[j][1] = e * in->light[j].rgb[1];
				lightpoint->rgb[j][2] = e * in->light[j].rgb[2];
			}
			prv->leaflight[i].count = 1;
			prv->leaflight[i].point = lightpoint++;
		}
	}
	mod->pvsbytes = ((mod->numclusters + 31)>>3)&~3;

	return true;
}

typedef struct
{
	unsigned short portalnum;
	unsigned short otherarea;

	unsigned short firstvert;
	unsigned short numverts;
	unsigned int planenum;
} hl2dareaportal_t;
static qboolean VBSP_LoadAreaPortals (model_t *mod, qbyte *mod_base, vlump_t *l, vlump_t *lump_verts)
{
	vbspinfo_t	*prv = (vbspinfo_t*)mod->meshinfo;
	int			i;
	q2dareaportal_t		*out;
	hl2dareaportal_t 	*in;
	int			count, vcount;
	vec3_t		*inverts;
	mesh_t		mesh;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "VBSP_LoadAreaPortals: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	inverts = (void *)(mod_base + lump_verts->fileofs);
	if (lump_verts->filelen % sizeof(*inverts))
	{
		Con_Printf (CON_ERROR "VBSP_LoadAreaPortals: funny lump size\n");
		return false;
	}
	vcount = lump_verts->filelen / sizeof(*inverts);

	if (count > MAX_Q2MAP_AREAS)
	{
		Con_Printf (CON_ERROR "Map has too many areas\n");
		return false;
	}

	out = prv->areaportals = plugfuncs->GMalloc(&mod->memgroup, sizeof(*out) * count);
	prv->portalpoly = plugfuncs->GMalloc(&mod->memgroup, sizeof(*prv->portalpoly)*count);
	prv->portalplane = plugfuncs->GMalloc(&mod->memgroup, sizeof(*prv->portalplane)*count);
	prv->numareaportals = count;

	mesh.xyz_array = plugfuncs->GMalloc(&mod->memgroup, sizeof(*mesh.xyz_array)*vcount);
	for (i=0 ; i<vcount ; i++)
	{
		mesh.xyz_array[i][0] = LittleFloat(inverts[i][0]);
		mesh.xyz_array[i][1] = LittleFloat(inverts[i][1]);
		mesh.xyz_array[i][2] = LittleFloat(inverts[i][2]);
	}
	mesh.indexes = plugfuncs->GMalloc(&mod->memgroup, sizeof(*mesh.indexes)*64*3);
	mesh.st_array = plugfuncs->GMalloc(&mod->memgroup, sizeof(*mesh.st_array)*64);
	for (i=0 ; i<64 ; i++)
	{
		Vector2Set(mesh.st_array[i], 0,0);
		mesh.indexes[i*3+0] = 0;
		mesh.indexes[i*3+1] = i+1;
		mesh.indexes[i*3+2] = i+2;
	}

	for (i=0 ; i<count ; i++, in++, out++)
	{
		out->portalnum = LittleShort (in->portalnum);
		out->otherarea = LittleShort (in->otherarea);

		prv->portalplane[i] = mod->planes + LittleLong (in->planenum);

		prv->portalpoly[i].xyz_array = mesh.xyz_array+LittleLong(in->firstvert);
		prv->portalpoly[i].st_array = mesh.st_array;
		prv->portalpoly[i].numvertexes = LittleLong(in->numverts);

		if (prv->portalpoly[i].numvertexes>2)
		{
			prv->portalpoly[i].istrifan = true;
			prv->portalpoly[i].indexes = mesh.indexes;
			prv->portalpoly[i].numindexes = (prv->portalpoly[i].numvertexes-2)*3;
		}
	}

	return true;
}

typedef struct
{	//the main displacement lump
	vec3_t position;	//not really sure how to use this.
	int firstvert;		//((1<<power)+1) squared verts
	int firsttriflags;	//ignored (two shorts per quad)
	int power;
	int minpower;	//ignored... FIXME: add lod...
	float smoothangle;
	unsigned int contents;
	unsigned short faceidx; //mod->surfaces index
	unsigned int lightmapalphaoffset;	//erk, rgba? that's going to restrict lightmap formats.
	unsigned int lightofs;	//extents? cabbage? oh noes we've gone mad again! or is this some special blend weights in addition to the surface's lighting?
	struct
	{	//erk
		unsigned short peer;
		unsigned char orientation;
		unsigned char span;
		unsigned char peerspan;
	} edgepeers[8];	//two per edge
	struct
	{	//no idea how this works
		unsigned short peer[4];
		unsigned char numpeers;
	} cornerpeers[4];
	unsigned int allowedverts[10];
} hl2ddisplacement_t;
typedef struct
{
	vec3_t norm;
	float dist;
	float alpha;	//vertex alpha.
} hl2displacementvert_t;
static qboolean VBSP_LoadDisplacements (model_t *mod, qbyte *mod_base, vlump_t *lumps)
{
	vbspinfo_t			*prv = (vbspinfo_t*)mod->meshinfo;
	vlump_t 			*l = &lumps[VLUMP_DISP_INFO];
	vlump_t 			*vl = &lumps[VLUMP_DISP_VERTS];
	hl2ddisplacement_t	*in;
	dispinfo_t			*out;
	int					i, count, x, y;
	hl2displacementvert_t *inv;
	struct dispvert_s	*verts;
	vecV_t				*xyz;
	index_t 			*indexes;
	size_t				maxverts;
	msurface_t 			*surf;
	float *sverts[4];
	signed int e, idx;
	float fx,fy;
	vec3_t p, base;
	size_t stride;

	int primary;
	float pdist,dist;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf ("VBSP_LoadDisplacements: funny lump size in %s\n",mod->name);
		return false;
	}
	count = l->filelen / sizeof(*in);
	if (!count)
		return true;	//nothing to worry about.
	out = plugfuncs->GMalloc(&mod->memgroup, count*sizeof(*out));

	for (maxverts = 0, i = 0; i < count; i++)
		maxverts += ((1<<in[i].power)) * ((1<<in[i].power));
	indexes = plugfuncs->GMalloc(&mod->memgroup, maxverts*sizeof(*indexes)*6);

	for (maxverts = 0, i = 0; i < count; i++)
		maxverts += ((1<<in[i].power)+1) * ((1<<in[i].power)+1);
	verts = plugfuncs->GMalloc(&mod->memgroup, maxverts*sizeof(*verts));
	xyz = plugfuncs->GMalloc(&mod->memgroup, maxverts*sizeof(*xyz));

	prv->displacements = out;
	prv->numdisplacements = count;
	for (i = 0; i < count; i++, out++, in++)
	{
		surf = mod->surfaces + in->faceidx;
		if (surf->numedges != 4)
		{
			Con_Printf ("VBSP_LoadDisplacements: displacement surface doesn't have 4 edges in %s\n",mod->name);
			return false;
		}

		//find the 4 verts... messy.
		for (x = 0; x < 4; x++)
		{
			e = mod->surfedges[surf->firstedge+x];
			idx = e < 0;
			if (idx)
				e = -e;
			if (e < 0 || e >= mod->numedges)
				sverts[x] = mod->vertexes[0].position;
			else
				sverts[x] = mod->vertexes[mod->edges[e].v[idx]].position;
		}

		//this is just stupid and pointless.
		//the in->position point tells us which point is the primary one, instead of just rotating the edges.
		primary = 0;
		pdist = FLT_MAX;
		for (x = 0; x < 4; x++)
		{
			VectorSubtract(sverts[x], in->position, p);
			dist = DotProduct(p,p);
			if (dist < pdist)
			{
				pdist = dist;
				primary = x;
			}
		}

		out->surf = surf;
		prv->surfdisp[in->faceidx] = out;	//the surface needs to be able to get its proper info when building vbos
		ClearBounds(out->aamin, out->aamax);
		out->contents = VBSP_TranslateContentBits(prv,in->contents);
		out->width = (1<<in->power);
		out->height = (1<<in->power);

		out->idx = indexes;
		stride = out->width+1;
		for (y=0 ; y<out->height ; y++)
		for (x=0 ; x<out->width ; x++)
		{
			if ((x+y)&1)
			{	//a diamond pattern - flipping alternately
				*indexes++ = (x+0)+(y+1)*stride;
				*indexes++ = (x+1)+(y+1)*stride;
				*indexes++ = (x+1)+(y+0)*stride;
				*indexes++ = (x+0)+(y+1)*stride;
				*indexes++ = (x+1)+(y+0)*stride;
				*indexes++ = (x+0)+(y+0)*stride;
			}
			else
			{
				*indexes++ = (x+0)+(y+0)*stride;
				*indexes++ = (x+0)+(y+1)*stride;
				*indexes++ = (x+1)+(y+1)*stride;
				*indexes++ = (x+0)+(y+0)*stride;
				*indexes++ = (x+1)+(y+1)*stride;
				*indexes++ = (x+1)+(y+0)*stride;
			}
		}
		out->numindexes = indexes - out->idx;

		inv = (void *)(mod_base + vl->fileofs);
		inv += in->firstvert;
		out->verts = verts;
		out->xyz = xyz;
		for (y = 0; y <= out->height; y++)
			for (x = 0; x <= out->width; x++)
			{
				//I have no idea if this is right. probably not. oh well.
				fx = (float)x/out->width;
				fy = (float)y/out->height;
				VectorClear(base);
				VectorMA(base, (1-fx)*(1-fy), sverts[(primary+0)&3], base);
				VectorMA(base, (1-fx)*(  fy), sverts[(primary+1)&3], base);
				VectorMA(base, (  fx)*(  fy), sverts[(primary+2)&3], base);
				VectorMA(base, (  fx)*(1-fy), sverts[(primary+3)&3], base);
				verts->st[0] = DotProduct(base, surf->texinfo->vecs[0]) + surf->texinfo->vecs[0][3];
				verts->st[1] = DotProduct(base, surf->texinfo->vecs[1]) + surf->texinfo->vecs[1][3];

				VectorScale(inv->norm, inv->dist*hl2_displacement_scale->value, p);
				VectorNormalize2(p, verts->norm);
				VectorAdd(base, p, (*xyz));
				verts->alpha = inv->alpha;
				AddPointToBounds((*xyz), out->aamin, out->aamax);

				inv++;
				verts++;
				xyz++;
			}

		surf->mesh->numindexes = out->numindexes;
		surf->mesh->numvertexes = verts-out->verts;
	}
	return true;
}

typedef struct
{
	short		planenum;
	qbyte		side;
	qbyte		onnode;	//o.O

	int			firstedge;		// we must support > 64k edges
	unsigned short		numedges;
	unsigned short		texinfo;

	unsigned short		dispinfo;
	short		fogvolume;

// lighting info
	qbyte		styles[4];
	int			lightofs;		// start of [numstyles*surfsize] samples
	float		surfacearea;
	int			extents_min[2];
	int			extents_size[2];

	int			origface;
	unsigned short numprims;
	unsigned short firstprim;
	unsigned int smoothinggroup;
} hl2dface_t;


typedef struct
{
	int			padding[8];
	short		planenum;
	qbyte		side;
	qbyte		onnode;	//o.O

	int			firstedge;		// we must support > 64k edges
	unsigned short		numedges;
	unsigned short		texinfo;

	unsigned short		dispinfo;
	short		fogvolume;

// lighting info
	qbyte		styles[8];
	qbyte		day[8];
	qbyte		night[8];
	int			lightofs;		// start of [numstyles*surfsize] samples
	float		surfacearea;
	int			extents_min[2];
	int			extents_size[2];

	int			origface;
	unsigned int smoothinggroup;
} vampiredface_t;

static qboolean VBSP_LoadFaces_Vampire (model_t *mod, qbyte *mod_base, vlump_t *lumps, int version)
{
	vbspinfo_t	*prv = (vbspinfo_t*)mod->meshinfo;
	vlump_t *l = &lumps[VLUMP_FACES_LDR];
	vampiredface_t	*in;
	msurface_t 	*out;
	int			i, count, surfnum;
	int			planenum;
	int			ti, st;
	int			lumpsize = sizeof(*in);

	mesh_t		*meshes;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % lumpsize)
	{
		Con_Printf ("VBSP_LoadFaces_Vampire: funny lump size in %s\n",mod->name);
		return false;
	}
	count = l->filelen / lumpsize;
	out = plugfuncs->GMalloc(&mod->memgroup, count*sizeof(*out));
	prv->surfdisp = plugfuncs->GMalloc(&mod->memgroup, count * sizeof(*prv->surfdisp));

	meshes = plugfuncs->GMalloc(&mod->memgroup, count*sizeof(*meshes));

	mod->surfaces = out;
	mod->numsurfaces = count;

	mod->lightmaps.surfstyles = 1;

	for ( surfnum=0 ; surfnum<count ; surfnum++, in = (void*)((qbyte*)in+lumpsize), out++)
	{
		out->firstedge = LittleLong(in->firstedge);
		out->numedges = (unsigned short)LittleShort(in->numedges);
		out->flags = 0;
		out->mesh = meshes+surfnum;
		out->mesh->numvertexes = out->numedges;
		out->mesh->numindexes = (out->mesh->numvertexes-2)*3;

		planenum = (unsigned short)LittleShort(in->planenum);
		if (in->side)
			out->flags |= SURF_PLANEBACK;
		if (!in->onnode)
			out->flags |= SURF_OFFNODE;

		out->plane = mod->planes + planenum;

		ti = (unsigned short)LittleShort (in->texinfo);
		if (ti < 0 || ti >= mod->numtexinfo)
		{
			Con_Printf (CON_ERROR "VBSP_LoadFaces: bad texinfo number\n");
			return false;
		}
		out->texinfo = mod->texinfo + ti;

		if (out->texinfo->flags & TI_SKY)
		{
			out->flags |= SURF_DRAWSKY|SURF_DRAWTILED;
		}
		if (out->texinfo->flags & TI_WARP)
		{
			out->flags |= SURF_DRAWTURB|SURF_DRAWTILED;
		}

		out->lmshift = 0;
		out->texturemins[0] = in->extents_min[0];
		out->texturemins[1] = in->extents_min[1];
		out->extents[0] = in->extents_size[0];
		out->extents[1] = in->extents_size[1];

	// lighting info

		for (i=0 ; i<Q1Q2BSP_STYLESPERSURF ; i++)
		{
			st = in->styles[i];
			if (st == 255)
				st = INVALID_LIGHTSTYLE;
			else if (mod->lightmaps.maxstyle < st)
				mod->lightmaps.maxstyle = st;
			out->styles[i] = st;
		}
		for (; i<MAXCPULIGHTMAPS ; i++)
			out->styles[i] = INVALID_LIGHTSTYLE;
		for (i = 0; i<MAXRLIGHTMAPS ; i++)
			out->vlstyles[i] = INVALID_VLIGHTSTYLE;
		i = LittleLong(in->lightofs);
		if (i == -1 || !mod->lightdata)
			out->samples = NULL;
		else
			out->samples = mod->lightdata + i;

	// set the drawing flags

		if (out->texinfo->flags & TI_WARP)
			out->flags |= SURF_DRAWTURB;
	}

	return true;
}

static qboolean VBSP_LoadFaces (model_t *mod, qbyte *mod_base, vlump_t *lumps, int version)
{
	vbspinfo_t	*prv = (vbspinfo_t*)mod->meshinfo;
	vlump_t *l = &lumps[VLUMP_FACES_LDR];
	vlump_t *l2 = &lumps[VLUMP_FACES_HDR];
	hl2dface_t	*in;
	msurface_t 	*out;
	int			i, count, surfnum;
	int			planenum;
	int			ti, st;
	int			lumpsize = sizeof(*in);

	mesh_t		*meshes;

	if (version == 17)
	{
		return VBSP_LoadFaces_Vampire(mod, mod_base, lumps, version);
	}

	if (l2->filelen && !(hl2_favour_ldr->ival && lumps[VLUMP_LIGHTING_LDR].filelen))
		l = l2;

	if (version == 18)
	{	//this version seems to have rgbx*4 prefixed
		in = (void *)(mod_base + l->fileofs + 4*4);
		lumpsize += 4*4;
	}
	else
		in = (void *)(mod_base + l->fileofs);
	if (l->filelen % lumpsize)
	{
		Con_Printf ("VBSP_LoadFaces: funny lump size in %s\n",mod->name);
		return false;
	}
	count = l->filelen / lumpsize;
	out = plugfuncs->GMalloc(&mod->memgroup, count*sizeof(*out));
	prv->surfdisp = plugfuncs->GMalloc(&mod->memgroup, count * sizeof(*prv->surfdisp));

	meshes = plugfuncs->GMalloc(&mod->memgroup, count*sizeof(*meshes));

	mod->surfaces = out;
	mod->numsurfaces = count;

	mod->lightmaps.surfstyles = 1;

	for ( surfnum=0 ; surfnum<count ; surfnum++, in = (void*)((qbyte*)in+lumpsize), out++)
	{
		out->firstedge = LittleLong(in->firstedge);
		out->numedges = (unsigned short)LittleShort(in->numedges);
		out->flags = 0;
		out->mesh = meshes+surfnum;
		out->mesh->numvertexes = out->numedges;
		out->mesh->numindexes = (out->mesh->numvertexes-2)*3;

		planenum = (unsigned short)LittleShort(in->planenum);
		if (in->side)
			out->flags |= SURF_PLANEBACK;
		if (!in->onnode)
			out->flags |= SURF_OFFNODE;

		out->plane = mod->planes + planenum;

		ti = (unsigned short)LittleShort (in->texinfo);
		if (ti < 0 || ti >= mod->numtexinfo)
		{
			Con_Printf (CON_ERROR "VBSP_LoadFaces: bad texinfo number\n");
			return false;
		}
		out->texinfo = mod->texinfo + ti;

		if (out->texinfo->flags & TI_SKY)
		{
			out->flags |= SURF_DRAWSKY|SURF_DRAWTILED;
		}
		if (out->texinfo->flags & TI_WARP)
		{
			out->flags |= SURF_DRAWTURB|SURF_DRAWTILED;
		}

		out->lmshift = 0;
		out->texturemins[0] = in->extents_min[0];
		out->texturemins[1] = in->extents_min[1];
		out->extents[0] = in->extents_size[0];
		out->extents[1] = in->extents_size[1];

	// lighting info

		for (i=0 ; i<Q1Q2BSP_STYLESPERSURF ; i++)
		{
			st = in->styles[i];
			if (st == 255)
				st = INVALID_LIGHTSTYLE;
			else if (mod->lightmaps.maxstyle < st)
				mod->lightmaps.maxstyle = st;
			out->styles[i] = st;
		}
		for (; i<MAXCPULIGHTMAPS ; i++)
			out->styles[i] = INVALID_LIGHTSTYLE;
		for (i = 0; i<MAXRLIGHTMAPS ; i++)
			out->vlstyles[i] = INVALID_VLIGHTSTYLE;
		i = LittleLong(in->lightofs);
		if (i == -1 || !mod->lightdata)
			out->samples = NULL;
		else
			out->samples = mod->lightdata + i;

	// set the drawing flags

		if (out->texinfo->flags & TI_WARP)
			out->flags |= SURF_DRAWTURB;
	}

	return true;
}
#ifdef HAVE_CLIENT
static void VBSP_BuildSurfMesh(model_t *mod, msurface_t *surf, builddata_t *bd)
{
	vbspinfo_t	*prv = (vbspinfo_t*)mod->meshinfo;

	unsigned int vertidx;
	int i, lindex, edgevert;
	mesh_t *mesh = surf->mesh;
	float *vec;
	float s, t, miss;
	int sty;
	struct vbsptexinfo_s *vtexinfo;

	//displacement surfaces...
	dispinfo_t *d = prv->surfdisp[surf-mod->surfaces];
	if (d)
	{
		struct dispvert_s *dv = d->verts;

		mesh->istrifan = false;

		memcpy(mesh->indexes, d->idx, sizeof(index_t)*d->numindexes);

		//output the renderable verticies
		for (i=0 ; i<mesh->numvertexes ; i++, dv++)
		{
			//xyz
			VectorCopy (d->xyz[i], mesh->xyz_array[i]);

			//st
			mesh->st_array[i][0] = dv->st[0];
			mesh->st_array[i][1] = dv->st[1];
			if (surf->texinfo->texture->vwidth)
				mesh->st_array[i][0] /= surf->texinfo->texture->vwidth;
			if (surf->texinfo->texture->vheight)
				mesh->st_array[i][1] /= surf->texinfo->texture->vheight;

			//lmst
			s = (float)(i%(d->width+1))/d->width;
			t = (float)(i/(d->width+1))/d->height;
			for (sty = 0; sty < 1; sty++)
			{
				mesh->lmst_array[sty][i][0] = (s*surf->extents[0] + surf->light_s[sty] + 0.5) / (mod->lightmaps.width);
				mesh->lmst_array[sty][i][1] = (t*surf->extents[1] + surf->light_t[sty] + 0.5) / (mod->lightmaps.height);
			}

			//normals
			VectorCopy(surf->plane->normal, mesh->normals_array[i]);
			VectorCopy(surf->texinfo->vecs[0], mesh->snormals_array[i]);
			VectorNegate(surf->texinfo->vecs[1], mesh->tnormals_array[i]);

			//rgba
			for (sty = 0; sty < 1; sty++)
			{
				mesh->colors4f_array[sty][i][0] = 1;
				mesh->colors4f_array[sty][i][1] = 1;
				mesh->colors4f_array[sty][i][2] = 1;
				mesh->colors4f_array[sty][i][3] = ((int)dv->alpha&255)/255.0;
			}
		}
		return;
	}

	//regular surfaces...
	mesh->istrifan = true;

	//output the mesh's indicies
	for (i=0 ; i<mesh->numvertexes-2 ; i++)
	{
		mesh->indexes[i*3] = 0;
		mesh->indexes[i*3+1] = i+1;
		mesh->indexes[i*3+2] = i+2;
	}
	//output the renderable verticies
	for (i=0 ; i<mesh->numvertexes ; i++)
	{
		lindex = mod->surfedges[surf->firstedge + i];
		edgevert = lindex <= 0;
		if (edgevert)
			lindex = -lindex;
		if (lindex < 0 || lindex >= mod->numedges)
			vertidx = 0;
		else
			vertidx = mod->edges[lindex].v[edgevert];
		vec = mod->vertexes[vertidx].position;

		s = DotProduct (vec, surf->texinfo->vecs[0]) + surf->texinfo->vecs[0][3];
		t = DotProduct (vec, surf->texinfo->vecs[1]) + surf->texinfo->vecs[1][3];

		VectorCopy (vec, mesh->xyz_array[i]);

		mesh->st_array[i][0] = s;
		mesh->st_array[i][1] = t;
		if (surf->texinfo->texture->vwidth)
			mesh->st_array[i][0] /= surf->texinfo->texture->vwidth;
		if (surf->texinfo->texture->vheight)
			mesh->st_array[i][1] /= surf->texinfo->texture->vheight;

		vtexinfo = &prv->texinfo[surf->texinfo-mod->texinfo];
		s = DotProduct (vec, vtexinfo->lmvecs[0]) + vtexinfo->lmvecs[0][3];
		t = DotProduct (vec, vtexinfo->lmvecs[1]) + vtexinfo->lmvecs[1][3];
		for (sty = 0; sty < 1; sty++)
		{
			mesh->lmst_array[sty][i][0] = (s - surf->texturemins[0] + (surf->light_s[sty]<<surf->lmshift) + (1<<surf->lmshift)*0.5) / (mod->lightmaps.width<<surf->lmshift);
			mesh->lmst_array[sty][i][1] = (t - surf->texturemins[1] + (surf->light_t[sty]<<surf->lmshift) + (1<<surf->lmshift)*0.5) / (mod->lightmaps.height<<surf->lmshift);
		}

		//figure out the texture directions, for bumpmapping and stuff
// 		if (surf->flags & SURF_PLANEBACK)
// 			VectorNegate(surf->plane->normal, mesh->normals_array[i]);
// 		else
		VectorCopy(surf->plane->normal, mesh->normals_array[i]);
		VectorCopy(surf->texinfo->vecs[0], mesh->snormals_array[i]);
		VectorNegate(surf->texinfo->vecs[1], mesh->tnormals_array[i]);
		//the s+t vectors are axis-aligned, so fiddle them so they're normal aligned instead
		miss = -DotProduct(mesh->normals_array[i], mesh->snormals_array[i]);
		VectorMA(mesh->snormals_array[i], miss, mesh->normals_array[i], mesh->snormals_array[i]);
		miss = -DotProduct(mesh->normals_array[i], mesh->tnormals_array[i]);
		VectorMA(mesh->tnormals_array[i], miss, mesh->normals_array[i], mesh->tnormals_array[i]);
		VectorNormalize(mesh->snormals_array[i]);
		VectorNormalize(mesh->tnormals_array[i]);

		//q1bsp has no colour information (fixme: sample from the lightmap?)
		for (sty = 0; sty < 1; sty++)
		{
			mesh->colors4f_array[sty][i][0] = 1;
			mesh->colors4f_array[sty][i][1] = 1;
			mesh->colors4f_array[sty][i][2] = 1;
			mesh->colors4f_array[sty][i][3] = 1;
		}
	}
}


static void VBSP_LoadLighting (model_t *mod, qbyte *mod_base, vlump_t *ldr, vlump_t *hdr)
{
	qbyte *src;
	unsigned int *out;
	size_t count;
	if (hdr->filelen && !(hl2_favour_ldr->ival && ldr->filelen))
	{
		mod->lightdatasize = hdr->filelen;
		src = (mod_base + hdr->fileofs);
	}
	else if (ldr->filelen)
	{
		mod->lightdatasize = ldr->filelen;
		src = (mod_base + ldr->fileofs);
	}
	else
		return;

	mod->lightmaps.fmt = LM_E5BGR9;
	mod->lightdata = (qbyte*)(out = plugfuncs->GMalloc(&mod->memgroup, mod->lightdatasize));

	//convert from linear e8bgr8 to srgb e5bgr9
	for (count = mod->lightdatasize/4; count --> 0; src+=4)
	{
		int e = 0;
		float m;
		float scale;
		unsigned int hdr;
		vec3_t rgb;

		//decode input
		m = pow(2, (signed char)src[3])/255.0;
		rgb[0] = m * src[0];
		rgb[1] = m * src[1];
		rgb[2] = m * src[2];

		//rescale its gamma ramp to something we can actually use properly
		rgb[0] = M_LinearToSRGB(rgb[0], 1.0);
		rgb[1] = M_LinearToSRGB(rgb[1], 1.0);
		rgb[2] = M_LinearToSRGB(rgb[2], 1.0);

		//encode output
		m = max(max(rgb[0], rgb[1]), rgb[2]);
		if (m < 0)
			m = 0;

		if (m >= 0.5)
		{	//positive exponent
			while (m >= (1<<(e)) && e < 30-15)	//don't do nans.
				e++;
		}
		else
		{	//negative exponent...
			while (m < 1/(1<<-e) && e > -14)	//don't do denormals.
				e--;
		}

		scale = pow(2, e-9);
		hdr = ((e+15)<<27);
		hdr |= bound(0, (int)(rgb[0]/scale + 0.5), 0x1ff)<<0;
		hdr |= bound(0, (int)(rgb[1]/scale + 0.5), 0x1ff)<<9;
		hdr |= bound(0, (int)(rgb[2]/scale + 0.5), 0x1ff)<<18;
		*out++ = hdr;
	}
}
#endif

typedef struct
{
	unsigned short planenum;
	short texinfo;
	unsigned short dispinfo;
	short bevel;
} hl2dbrushside_t;
static qboolean VBSP_LoadBrushSides (model_t *mod, qbyte *mod_base, vlump_t *l)
{
	vbspinfo_t	*prv = (vbspinfo_t*)mod->meshinfo;
	unsigned int			i, j;
	q2cbrushside_t	*out;
	hl2dbrushside_t *in;
	int			count;
	int			num;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "VBSP_LoadBrushSides: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	// need to save space for box planes
	if (count > SANITY_MAX_MAP_BRUSHSIDES)
	{
		Con_Printf (CON_ERROR "Map has too many brushsides (%i)\n", count);
		return false;
	}

	out = prv->brushsides = plugfuncs->GMalloc(&mod->memgroup, sizeof(*out) * count);
	prv->numbrushsides = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		num = (unsigned short)LittleShort (in->planenum);
		out->plane = &mod->planes[num];
		j = (unsigned short)LittleShort (in->texinfo);
		if (j >= mod->numtexinfo)
			out->surface = &nullsurface;
		else
			out->surface = &prv->surfaces[j];
	}

	return true;
}

typedef struct {
	unsigned int count;
	struct {
		unsigned int id;
		unsigned short flags;
		unsigned short version;
		unsigned int ofs;
		unsigned int len;
	} sl[1];
} hlgamelumpheader_t;
static qboolean VBSP_LoadStaticProps(model_t *mod, qbyte *offset, size_t size, int version)	//present on server, because they're potentially solid.
{
	vbspinfo_t *prv = mod->meshinfo;
	struct {
		const char name[128];
	} *modelref;
	size_t nummodels, numleafrefs, numprops, i;
	unsigned short *leafref, *eleafref;
	struct staticprop_s *sent;
	entity_t *ent;

	size_t modelindex;

	qboolean skip = false;
	int dxlevel = 95, cpulevel=0, gpulevel=0;
	unsigned int leafcount, l;

	qbyte *prop;
	size_t propsize;
	switch(version)
	{
	case 4:
		propsize = 14*4;
		break;
	case 5:	//+scale
		propsize = 15*4;
		break;
	case 6://+dxlevels
		propsize = 16*4;
		break;
	case 7:	//+rgba
	case 8: //-dxlevels+[cg]pulevels
		propsize = 17*4;
		break;
	case 9:	//+360
	case 10://-360+flags
		propsize = 18*4;
		break;
	case 11://+scale
		propsize = 19*4;
		break;
	default:
		return true;	//version not supported, just ignore it entirely. sorry.
	}


	nummodels = LittleLong(*(int*)offset);
	offset	+= 4;
	size	-= 4;
	modelref = (void*)offset;
	offset	+= nummodels*sizeof(*modelref);
	size	-= nummodels*sizeof(*modelref);

	numleafrefs = LittleLong(*(int*)offset);
	offset	+= 4;
	size	-= 4;
	leafref = (void*)offset;
	offset	+= numleafrefs*sizeof(*leafref);
	size	-= numleafrefs*sizeof(*leafref);

	numprops = LittleLong(*(int*)offset);
	offset	+= 4;
	size	-= 4;
	prop = (void*)offset;
	offset	+= numprops*propsize;
	size	-= numprops*propsize;

	if (size)
		return true;	//funny lump size...

	prv->staticprops = plugfuncs->GMalloc(&mod->memgroup, sizeof(*prv->staticprops)*numprops);
	for (i = 0, sent = prv->staticprops; i < numprops; i++)
	{
		ent = &sent->ent;
		skip = false;
		ent->playerindex = -1;
		ent->topcolour = TOP_DEFAULT;
		ent->bottomcolour = BOTTOM_DEFAULT;
		ent->scale = 1;
		ent->flags = RF_NOSHADOW;
		ent->shaderRGBAf[0] = 1;
		ent->shaderRGBAf[1] = 1;
		ent->shaderRGBAf[2] = 1;
		ent->shaderRGBAf[3] = 1;
		ent->framestate.g[FS_REG].frame[0] = 0;
		ent->framestate.g[FS_REG].lerpweight[0] = 1;


		ent->origin[0] = LittleFloat(*(float*)prop);					prop += sizeof(float);
		ent->origin[1] = LittleFloat(*(float*)prop);					prop += sizeof(float);
		ent->origin[2] = LittleFloat(*(float*)prop);					prop += sizeof(float);
		ent->angles[0] = LittleFloat(*(float*)prop);					prop += sizeof(float);
		ent->angles[1] = LittleFloat(*(float*)prop);					prop += sizeof(float);
		ent->angles[2] = LittleFloat(*(float*)prop);					prop += sizeof(float);
		modelindex = (unsigned short)LittleShort(*(short*)prop);		prop += sizeof(unsigned short);
		eleafref = leafref+(unsigned short)LittleShort(*(unsigned short*)prop);			prop += sizeof(unsigned short);
		leafcount = LittleShort(*(unsigned short*)prop);				prop += sizeof(unsigned short);
		sent->solid = *prop;											prop += sizeof(qbyte);
		/*ent->flags = *prop*/;											prop += sizeof(qbyte);
		ent->skinnum = LittleLong(*(unsigned int*)prop);				prop += sizeof(unsigned int);
		sent->fademindist = LittleFloat(*(float*)prop);					prop += sizeof(float);
		sent->fademaxdist = LittleFloat(*(float*)prop);					prop += sizeof(float);
		sent->lightorg[0] = LittleFloat(*(float*)prop);					prop += sizeof(float);
		sent->lightorg[1] = LittleFloat(*(float*)prop);					prop += sizeof(float);
		sent->lightorg[2] = LittleFloat(*(float*)prop);					prop += sizeof(float);
		if (version >= 5)
		{
			/*ent->fadescale = LittleFloat(*(float*)prop);*/			prop += sizeof(float);
		}
		if (version >= 8)
		{
			skip |= (prop[0] > cpulevel || cpulevel > prop[1]);			prop += sizeof(qbyte)*2;
			skip |= (prop[0] > gpulevel || gpulevel > prop[1]);			prop += sizeof(qbyte)*2;
		}
		else if (version >= 6)
		{
			unsigned short minlev, maxlev;
			minlev = LittleShort(*(unsigned short*)prop);				prop += sizeof(unsigned short);
			maxlev = LittleShort(*(unsigned short*)prop);				prop += sizeof(unsigned short);
			skip |=  (minlev > 0) && ((minlev > dxlevel) || (dxlevel > maxlev));
		}
		if (version >= 7)
		{
			VectorScale(prop, 1/255.0, ent->shaderRGBAf);				prop += sizeof(qbyte)*4;
		}

		if (version == 9)
		{
			/*disablex360 = LittleLong(*(int*)prop);*/					prop += sizeof(int);
		}
		if (version >= 10)
		{
			ent->flags = LittleLong(*(int*)prop);						prop += sizeof(int);
		}
		if (version >= 11)
		{
			ent->scale = LittleFloat(*(float*)prop);					prop += sizeof(float);
		}

		//okay, we parsed the prop data now...
		skip |= modelindex >= nummodels;
		if (skip)
			continue;	//we're ignoring it for some reason
		ent->model = modfuncs->BeginSubmodelLoad(modelref[modelindex].name);
		modfuncs->AngleVectors(ent->angles, ent->axis[0], ent->axis[1], ent->axis[2]);
		VectorNegate(ent->axis[1],ent->axis[1]);

		//transforms, in case we need to recursively walk its bih
		VectorCopy(ent->axis[0], prv->staticprops[i].transform.axis[0]);
		VectorCopy(ent->axis[1], prv->staticprops[i].transform.axis[1]);
		VectorCopy(ent->axis[2], prv->staticprops[i].transform.axis[2]);
		VectorCopy(ent->origin, prv->staticprops[i].transform.origin);

		//Hack: special value to flag it for linking once we've loaded its model. this needs to go when we make them solid.
		if (leafcount > countof(ent->pvscache.leafnums))
			ent->pvscache.num_leafs = -2;	//overflow. calculate it later once its loaded.
		else
		{
			ent->pvscache.areanum = -1;	//FIXME
			ent->pvscache.areanum2 = -1;
			ent->pvscache.num_leafs = leafcount;	//overflow. calculate it later once its loaded.
			for (l = 0; l < leafcount; l++)
			{
				mleaf_t *lf = mod->leafs + *eleafref++;
				ent->pvscache.leafnums[l]/*actually clusters*/ = lf->cluster;
				//and try to track the areas too. not quite so reliable...
				if (ent->pvscache.areanum == -1)
					ent->pvscache.areanum = lf->area;
				else
					ent->pvscache.areanum2 = lf->area;
			}
		}
		//Hack: lighting is wrong.
//		ent->light_type = ELT_UNKNOWN;
#if 0
		VectorSet(ent->light_dir, 0, 0.707, 0.707);
		VectorSet(ent->light_avg, 0.75, 0.75, 0.75);
		VectorSet(ent->light_range, 0.5, 0.5, 0.5);
#endif

		//not all props will be emitted, according to d3d levels...
		prv->numstaticprops++;
		sent++;
	}
	return true;
}
static qboolean VBSP_LoadGameLump(model_t *mod, qbyte *mod_base, vlump_t *l)
{
	size_t i;
	hlgamelumpheader_t *blob = (void*)(mod_base + l->fileofs);
	if (!l->filelen)
		return true; //missing
	if (l->filelen < sizeof(*blob) + sizeof(*blob->sl)*(blob->count-1))
		return false;	//not even enough space for the header...
	for (i = 0; i < blob->count; i++)
	{
#define LUMPTYPE(a,b,c,d, minver, maxver) (blob->sl[i].id == (((qbyte)a<<24)|((qbyte)b<<16)|((qbyte)c<<8)|((qbyte)d<<0)) && blob->sl[i].version >= minver && blob->sl[i].version <= maxver)
		if (LUMPTYPE('s','p','r','p', 4,10) && !blob->sl[i].flags)	//static props (placed by mapper)
			VBSP_LoadStaticProps(mod, mod_base+blob->sl[i].ofs/*sigh*/, blob->sl[i].len, blob->sl[i].version);
		else if (LUMPTYPE('d','p','r','p', 4,4) && !blob->sl[i].flags)	//dynamic props (generated by textures)
			;
		else if (LUMPTYPE('d','p','l','t', 0,0) && !blob->sl[i].flags)	//detail prop ldr lighting
			;
		else if (LUMPTYPE('d','p','l','h', 0,0) && !blob->sl[i].flags)	//detail prop hdr lighting
			;
		else
			Con_Printf("Unsupported gamelump id/version %c%c%c%c %i\n", (blob->sl[i].id>>24),(blob->sl[i].id>>16),(blob->sl[i].id>>8),(blob->sl[i].id>>0),blob->sl[i].version);
#undef LUMPTYPE
	}
	return true;
}

#ifdef HAVE_CLIENT
static void VBSP_GenerateMaterials(void *ctx, void *data, size_t a, size_t b)
{
	model_t *mod = ctx;
	const char *script;

	if (!a)
	{	//submodels share textures, so only do this if 'a' is 0 (inline index, 0 = world).
		for(a = 0; a < mod->numtextures; a++)
		{
			script = NULL;
			if (!strncmp(mod->textures[a]->name, "sky/", 4))
				script =
					"{\n"
						"sort sky\n"
						"surfaceparm nodlight\n"
						"skyparms - - -\n"
					"}\n";
			mod->textures[a]->shader = modfuncs->RegisterBasicShader(mod, mod->textures[a]->name, SUF_LIGHTMAP, script, PTI_INVALID, 0, 0, NULL, NULL);
		}
	}
	modfuncs->Batches_Build(mod, data);
	if (data)
		plugfuncs->Free(data);
}
#endif

/*
==================
CM_InlineModel
==================
*/
static cmodel_t	*VBSP_InlineModel (model_t *model, char *name)
{
	vbspinfo_t	*prv = (vbspinfo_t*)model->meshinfo;
	int		num;

	if (!name)
		Host_Error("Bad model\n");
	else if (name[0] != '*')
		Host_Error("Bad model\n");

	num = atoi (name+1);

	if (num < 1 || num >= prv->numcmodels)
		Host_Error ("CM_InlineModel: bad number");

	return &prv->cmodels[num];
}

static int VBSP_NumInlineModels (model_t *model)
{
	vbspinfo_t	*prv = (vbspinfo_t*)model->meshinfo;
	return prv->numcmodels;
}

static int VBSP_LeafContents (model_t *model, int leafnum)
{
	if (leafnum < 0 || leafnum >= model->numleafs)
		Host_Error ("CM_LeafContents: bad number");
	return model->leafs[leafnum].contents;
}

static int VBSP_LeafCluster (model_t *model, int leafnum)
{
	if (leafnum < 0 || leafnum >= model->numleafs)
		Host_Error ("CM_LeafCluster: bad number");
	return model->leafs[leafnum].cluster;
}

static int VBSP_LeafArea (model_t *model, int leafnum)
{
	if (leafnum < 0 || leafnum >= model->numleafs)
		Host_Error ("CM_LeafArea: bad number");
	return model->leafs[leafnum].area;
}

//=======================================================================

/*
==================
CM_PointLeafnum_r

==================
*/
static int VBSP_PointLeafnum_r (model_t *mod, const vec3_t p, int num)
{
	float		d;
	mnode_t		*node;
	mplane_t	*plane;

	while (num >= 0)
	{
		node = mod->nodes + num;
		plane = node->plane;

		if (plane->type < 3)
			d = p[plane->type] - plane->dist;
		else
			d = DotProduct (plane->normal, p) - plane->dist;
		if (d < 0)
			num = node->childnum[1];
		else
			num = node->childnum[0];
	}

	return -1 - num;
}

static int VBSP_PointLeafnum (model_t *mod, const vec3_t p)
{
	if (mod->loadstate != MLS_LOADED)
		return 0;		// sound may call this without map loaded
	return VBSP_PointLeafnum_r (mod, p, 0);
}

static int VBSP_PointCluster (model_t *mod, const vec3_t p, int *area)
{
	int leaf;
	if (mod->loadstate != MLS_LOADED)
		return 0;		// sound may call this without map loaded

	leaf = VBSP_PointLeafnum_r (mod, p, 0);
	if (area)
		*area = VBSP_LeafArea(mod, leaf);
	return VBSP_LeafCluster(mod, leaf);
}

static unsigned int VBSP_PointContents (model_t *mod, const vec3_t axis[3], const vec3_t p)
{
	vec3_t np;
	if (mod->loadstate != MLS_LOADED)
		return 0;

	if (axis)
	{
		np[0] = DotProduct(p, axis[0]);
		np[1] = DotProduct(p, axis[1]);
		np[2] = DotProduct(p, axis[2]);
		p = np;
	}

	return VBSP_LeafContents(mod, VBSP_PointLeafnum_r (mod, p, 0));
}

/*
=============
CM_BoxLeafnums

Fills in a list of all the leafs touched
=============
*/
static int		leaf_count, leaf_maxcount;
static int		*leaf_list;
static const float	*leaf_mins, *leaf_maxs;
static int		leaf_topnode;

static void VBSP_BoxLeafnums_r (model_t *mod, int nodenum)
{
	mplane_t	*plane;
	mnode_t		*node;
	int		s;

	while (1)
	{
		if (nodenum < 0)
		{
			if (leaf_count >= leaf_maxcount)
				return;
			leaf_list[leaf_count++] = -1 - nodenum;
			return;
		}

		node = &mod->nodes[nodenum];
		plane = node->plane;
//		s = BoxOnPlaneSide (leaf_mins, leaf_maxs, plane);
		s = BOX_ON_PLANE_SIDE(leaf_mins, leaf_maxs, plane);
		if (s == 1)
			nodenum = node->childnum[0];
		else if (s == 2)
			nodenum = node->childnum[1];
		else
		{	// go down both
			if (leaf_topnode == -1)
				leaf_topnode = nodenum;
			VBSP_BoxLeafnums_r (mod, node->childnum[0]);
			nodenum = node->childnum[1];
		}

	}
}

static int	VBSP_BoxLeafnums_headnode (model_t *mod, const vec3_t mins, const vec3_t maxs, int *list, int listsize, int headnode, int *topnode)
{
	leaf_list = list;
	leaf_count = 0;
	leaf_maxcount = listsize;
	leaf_mins = mins;
	leaf_maxs = maxs;

	leaf_topnode = -1;

	VBSP_BoxLeafnums_r (mod, headnode);

	if (topnode)
		*topnode = leaf_topnode;

	return leaf_count;
}

static int	VBSP_BoxLeafnums (model_t *mod, const vec3_t mins, const vec3_t maxs, int *list, int listsize, int *topnode)
{
	return VBSP_BoxLeafnums_headnode (mod, mins, maxs, list,
		listsize, mod->hulls[0].firstclipnode, topnode);
}

static void VBSP_FindTouchedLeafs(model_t *model, struct pvscache_s *ent, const float *mins, const float *maxs)
{
#define MAX_TOTAL_ENT_LEAFS		128
	int			leafs[MAX_TOTAL_ENT_LEAFS];
	int			clusters[MAX_TOTAL_ENT_LEAFS];
	int num_leafs;
	int			topnode;
	int i, j;
	int			area;
	int nullarea = -1;

	//ent->num_leafs == q2's ent->num_clusters
	ent->num_leafs = 0;
	ent->areanum = nullarea;
	ent->areanum2 = nullarea;

	if (!mins || !maxs)
		return;

	//get all leafs, including solids
	num_leafs = VBSP_BoxLeafnums (model, mins, maxs,
		leafs, MAX_TOTAL_ENT_LEAFS, &topnode);

	// set areas
	for (i=0 ; i<num_leafs ; i++)
	{
		clusters[i] = VBSP_LeafCluster (model, leafs[i]);
		area = VBSP_LeafArea (model, leafs[i]);
		if (area != nullarea)
		{	// doors may legally straggle two areas,
			// but nothing should ever need more than that
			if (ent->areanum != nullarea && ent->areanum != area)
				ent->areanum2 = area;
			else
				ent->areanum = area;
		}
	}

	if (num_leafs >= MAX_TOTAL_ENT_LEAFS)
	{	// assume we missed some leafs, and mark by headnode
		ent->num_leafs = -1;
		ent->headnode = topnode;
	}
	else
	{
		ent->num_leafs = 0;
		for (i=0 ; i<num_leafs ; i++)
		{
			if (clusters[i] == -1)
				continue;		// not a visible leaf
			for (j=0 ; j<i ; j++)
				if (clusters[j] == clusters[i])
					break;
			if (j == i)
			{
				if (ent->num_leafs == MAX_ENT_LEAFS)
				{	// assume we missed some leafs, and mark by headnode
					ent->num_leafs = -1;
					ent->headnode = topnode;
					break;
				}

				ent->leafnums[ent->num_leafs++] = clusters[i];
			}
		}
	}
}

/*
===============================================================================

BOX TRACING

===============================================================================
*/

static void VBSP_FinalizeBrush(q2cbrush_t *brush)
{
	vecV_t verts[256];
	vec4_t planes[256];
	int i, j;
	ClearBounds(brush->absmins, brush->absmaxs);
	for (i = 0; i < brush->numsides; i++)
	{
		VectorCopy(brush->brushside[i].plane->normal, planes[i]);
		planes[i][3] = brush->brushside[i].plane->dist;
	}
	for (i = 0; i < brush->numsides; i++)
	{
		//most brushes are axial, which can save some a little loadtime
		if (planes[i][0] == 1)
			brush->absmaxs[0] = planes[i][3];
		else if (planes[i][1] == 1)
			brush->absmaxs[1] = planes[i][3];
		else if (planes[i][2] == 1)
			brush->absmaxs[2] = planes[i][3];
		else if (planes[i][0] == -1)
			brush->absmins[0] = -planes[i][3];
		else if (planes[i][1] == -1)
			brush->absmins[1] = -planes[i][3];
		else if (planes[i][2] == -1)
			brush->absmins[2] = -planes[i][3];
		else
		{
			j = modfuncs->ClipPlaneToBrush(verts, countof(verts), planes, sizeof(planes[0]), brush->numsides, planes[i]);
			while (j-- > 0)
				AddPointToBounds(verts[j], brush->absmins, brush->absmaxs);
		}
	}
}

/*
===============================================================================

AREAPORTALS

===============================================================================
*/

static void FloodArea_r (vbspinfo_t	*prv, size_t areaidx, int floodnum)
{
	size_t		i;

	careaflood_t *flood = &prv->areaflood[areaidx];
	if (flood->floodvalid == prv->floodvalid)
	{
		if (flood->floodnum == floodnum)
			return;
		Con_Printf ("FloodArea_r: reflooded\n");
		return;
	}

	flood->floodnum = floodnum;
	flood->floodvalid = prv->floodvalid;

	{
		carea_t *area = &prv->areas[areaidx];
		q2dareaportal_t	*p = &prv->areaportals[area->firstareaportal];
		for (i=0 ; i<area->numareaportals ; i++, p++)
		{
			if (prv->portalopen[p->portalnum])
				FloodArea_r (prv, p->otherarea, floodnum);
		}
	}
}

/*
====================
FloodAreaConnections


====================
*/
static void	FloodAreaConnections (vbspinfo_t	*prv)
{
	size_t		i;
	int		floodnum;

	// all current floods are now invalid
	prv->floodvalid++;
	floodnum = 0;

	// area 0 is not used
	for (i=0 ; i<prv->numareas ; i++)
	{
		if (prv->areaflood[i].floodvalid == prv->floodvalid)
			continue;		// already flooded into
		floodnum++;
		FloodArea_r (prv, i, floodnum);
	}
}

static void	VBSP_SetAreaPortalState (model_t *mod, unsigned int portalnum, unsigned int area1, unsigned int area2, qboolean open)
{
	vbspinfo_t	*prv;
	prv = (vbspinfo_t*)mod->meshinfo;
	if (portalnum > prv->numareaportals)
		return;
	if (prv->portalopen[portalnum] == open)
		return;
	prv->portalopen[portalnum] = open;
	FloodAreaConnections (prv);
}

static qboolean	VBSP_AreasConnected (model_t *mod, unsigned int area1, unsigned int area2)
{
	vbspinfo_t	*prv = (vbspinfo_t*)mod->meshinfo;

	if (map_noareas->value)
		return true;

	if (area1 == ~0 || area2 == ~0)
		return area1 == area2;
	if (area1 > prv->numareas || area2 > prv->numareas)
		Host_Error ("area > numareas");

	if (prv->areaflood[area1].floodnum == prv->areaflood[area2].floodnum)
		return true;
	return false;
}


/*
=================
CM_WriteAreaBits

Writes a length qbyte followed by a bit vector of all the areas
that area in the same flood as the area parameter

This is used by the client refreshes to cull visibility
=================
*/
static int VBSP_WriteAreaBits (model_t *mod, qbyte *buffer, int area, qboolean merge)
{
	vbspinfo_t	*prv = (vbspinfo_t*)mod->meshinfo;
	int		i;
	int		floodnum;
	int		bytes;
	int		nullarea = 0;

	bytes = (prv->numareas+7)>>3;

	if (map_noareas->value || (area == nullarea && !merge))
	{	// for debugging, send everything
		if (!merge)
			memset (buffer, 255, bytes);
	}
	else
	{
		if (!merge)
			memset (buffer, 0, bytes);

		floodnum = prv->areaflood[area].floodnum;
		for (i=0 ; i<prv->numareas ; i++)
		{
			if (prv->areaflood[i].floodnum == floodnum)
				buffer[i>>3] |= 1<<(i&7);
		}
	}

	return bytes;
}

/*
===================
CM_WritePortalState

Returns a size+pointer to the data that needs to be written into a saved game. 
===================
*/
static size_t VBSP_SaveAreaPortalBlob (model_t *mod, void **data)
{
	vbspinfo_t	*prv = (vbspinfo_t*)mod->meshinfo;

	*data = prv->portalopen;
	return sizeof(prv->portalopen);
}

/*
===================
CM_ReadPortalState

Reads the portal state from a savegame file
and recalculates the area connections
===================
*/
static size_t VBSP_LoadAreaPortalBlob (model_t *mod, void *ptr, size_t ptrsize)
{
	vbspinfo_t	*prv = (vbspinfo_t*)mod->meshinfo;

	memcpy(prv->portalopen, ptr, min(ptrsize,sizeof(prv->portalopen)));

	FloodAreaConnections (prv);
	return sizeof(prv->portalopen);
}


/*
===============================================================================

PVS / PHS

===============================================================================
*/

/*
===================
CM_DecompressVis
===================
*/
static void VBSP_DecompressVis (model_t *mod, qbyte *in, qbyte *out, qboolean merge)
{
	vbspinfo_t	*prv = (vbspinfo_t*)mod->meshinfo;
	int		c;
	qbyte	*out_p;
	int		row;

	row = (mod->numclusters+7)>>3;
	out_p = out;

	if (!in || !prv->numvisibility)
	{	// no vis info, so make all visible
		while (row)
		{
			*out_p++ = 0xff;
			row--;
		}
		return;
	}

	if (merge)
	{
		do
		{
			if (*in)
			{
				*out_p++ |= *in++;
				continue;
			}

			out_p += in[1];
			in += 2;
		} while (out_p - out < row);
	}
	else
	{
		do
		{
			if (*in)
			{
				*out_p++ = *in++;
				continue;
			}

			c = in[1];
			in += 2;
			if ((out_p - out) + c > row)
			{
				c = row - (out_p - out);
				Con_DPrintf ("warning: Vis decompression overrun\n");
			}
			while (c)
			{
				*out_p++ = 0;
				c--;
			}
		} while (out_p - out < row);
	}
}

static pvsbuffer_t	pvsrow;
static pvsbuffer_t	phsrow;



static qbyte	*VBSP_ClusterPVS (model_t *mod, int cluster, pvsbuffer_t *buffer, pvsmerge_t merge)
{
	vbspinfo_t	*prv = (vbspinfo_t*)mod->meshinfo;
	if (!buffer)
		buffer = &pvsrow;
	if (buffer->buffersize < mod->pvsbytes)
		buffer->buffer = plugfuncs->Realloc(buffer->buffer, buffer->buffersize=mod->pvsbytes);

	if (cluster == -1)
		memset (buffer->buffer, 0, (mod->numclusters+7)>>3);
	else
		VBSP_DecompressVis (mod, ((qbyte*)prv->vis) + prv->vis->bitofs[cluster][DVIS_PVS], buffer->buffer, merge==PVM_MERGE);
	return buffer->buffer;
}

static qbyte	*VBSP_ClusterPHS (model_t *mod, int cluster, pvsbuffer_t *buffer)
{
	vbspinfo_t	*prv = (vbspinfo_t*)mod->meshinfo;

	if (!buffer)
		buffer = &phsrow;
	if (buffer->buffersize < mod->pvsbytes)
		buffer->buffer = plugfuncs->Realloc(buffer->buffer, buffer->buffersize=mod->pvsbytes);

	if (cluster == -1)
		memset (buffer->buffer, 0, (mod->numclusters+7)>>3);
	else
		VBSP_DecompressVis (mod, ((qbyte*)prv->vis) + prv->vis->bitofs[cluster][DVIS_PHS], buffer->buffer, false);
	return buffer->buffer;
}

static unsigned int  VBSP_FatPVS (model_t *mod, const vec3_t org, pvsbuffer_t *result, qboolean merge)
{
	int	leafs[64];
	int		i, j, count;
	vec3_t	mins, maxs;

	for (i=0 ; i<3 ; i++)
	{
		mins[i] = org[i] - 8;
		maxs[i] = org[i] + 8;
	}

	count = VBSP_BoxLeafnums (mod, mins, maxs, leafs, countof(leafs), NULL);
	if (count < 1)
		Sys_Errorf ("SV_Q2FatPVS: count < 1");

	// convert leafs to clusters
	for (i=0 ; i<count ; i++)
		leafs[i] = VBSP_LeafCluster(mod, leafs[i]);

	//grow the buffer if needed
	if (result->buffersize < mod->pvsbytes)
		result->buffer = plugfuncs->Realloc(result->buffer, result->buffersize=mod->pvsbytes);

	if (count == 1 && leafs[0] == -1)
	{	//if the only leaf is the outside then broadcast it.
		memset(result->buffer, 0xff, mod->pvsbytes);
		i = count;
	}
	else
	{
		i = 0;
		if (!merge)
			mod->funcs.ClusterPVS(mod, leafs[i++], result, PVM_REPLACE);
		// or in all the other leaf bits
		for ( ; i<count ; i++)
		{
			for (j=0 ; j<i ; j++)
				if (leafs[i] == leafs[j])
					break;
			if (j != i)
				continue;		// already have the cluster we want
			mod->funcs.ClusterPVS(mod, leafs[i], result, PVM_MERGE);
		}
	}
	return mod->pvsbytes;
}

/*
=============
CM_HeadnodeVisible

Returns true if any leaf under headnode has a cluster that
is potentially visible
=============
*/
static qboolean VBSP_HeadnodeVisible (model_t *mod, int nodenum, const qbyte *visbits)
{
	int		leafnum;
	int		cluster;
	mnode_t	*node;

	if (nodenum < 0)
	{
		leafnum = -1-nodenum;
		cluster = mod->leafs[leafnum].cluster;
		if (cluster == -1)
			return false;
		if (visbits[cluster>>3] & (1<<(cluster&7)))
			return true;
		return false;
	}

	node = &mod->nodes[nodenum];
	if (VBSP_HeadnodeVisible(mod, node->childnum[0], visbits))
		return true;
	return VBSP_HeadnodeVisible(mod, node->childnum[1], visbits);
}

static qboolean VBSP_EdictInFatPVS(model_t *mod, const pvscache_t *ent, const qbyte *pvs, const int *areas)
{
	int i,l;
	int nullarea = 0;
	if (areas)
	{
		for (i = 1; ; i++)
		{
			if (i > areas[0])
				return false;	//none of the camera's areas could see the entity
			if (areas[i] == ent->areanum)
			{
				if (areas[i] != nullarea)
					break;
				//else entity is fully outside the world, invisible to all...
			}
			else if (VBSP_AreasConnected (mod, areas[i], ent->areanum))
				break;
			// doors can legally straddle two areas, so
			// we may need to check another one
			else if (ent->areanum2 != nullarea && VBSP_AreasConnected (mod, areas[i], ent->areanum2))
				break;
		}
	}

	if (ent->num_leafs == -1)
	{	// too many leafs for individual check, go by headnode
		if (!VBSP_HeadnodeVisible (mod, ent->headnode, pvs))
			return false;
	}
	else
	{	// check individual leafs
		for (i=0 ; i < ent->num_leafs ; i++)
		{
			l = ent->leafnums[i];
			if (pvs[l >> 3] & (1 << (l&7) ))
				break;
		}
		if (i == ent->num_leafs)
			return false;		// not visible
	}
	return true;
}


/*
===============================================================================

Collision Stuff.

===============================================================================
*/

static void VBSP_BuildBIHSubmodel(model_t *mod, int submodel)
{
	vbspinfo_t	*prv = (vbspinfo_t*)mod->meshinfo;
	cmodel_t	*sub = &prv->cmodels[submodel];

	struct bihleaf_s *bihleaf, *l;
	size_t i;

	bihleaf = l = plugfuncs->Malloc(sizeof(*bihleaf)*sub->num_brushes);
	for (i = 0; i < sub->num_brushes; i++)
	{
		q2cbrush_t *b = &prv->brushes[sub->firstbrush+i];
		l->type = BIH_BRUSH;
		l->data.brush = b;

		l->data.contents = b->contents;
		VectorCopy(b->absmins, l->mins);
		VectorCopy(b->absmaxs, l->maxs);
		l++;
	}

	modfuncs->BIH_Build(mod, bihleaf, l-bihleaf);
	plugfuncs->Free(bihleaf);
}
static void VBSP_BuildBIHMain(void *ctx, void *unusedp, size_t unuseda, size_t unusedb)
{	//NOTE: must be on main thread because we're waiting for submodels for their size.
	model_t		*mod = ctx;
	vbspinfo_t	*prv = (vbspinfo_t*)mod->meshinfo;
	cmodel_t	*sub = &prv->cmodels[0];

	struct bihleaf_s *bihleaf, *l;
	size_t bihleafs, i;

	bihleafs = sub->num_brushes;
	for (i = 0; i < prv->numdisplacements; i++)
		bihleafs += prv->displacements[i].numindexes/3;
	for (i = 0; i < prv->numstaticprops; i++)
	{
		if (prv->staticprops[i].solid)
		{
			if (prv->staticprops[i].ent.model && prv->staticprops[i].ent.model->loadstate == MLS_NOTLOADED)
				modfuncs->GetModel(prv->staticprops[i].ent.model->publicname, MLV_WARN);	//we use threads, so these'll load in time.
			bihleafs++;
		}
	}

	bihleaf = l = plugfuncs->Malloc(sizeof(*bihleaf)*bihleafs);

	//now we have enough storage, spit them out providing bounds info.
	for (i = 0; i < sub->num_brushes; i++)
	{
		q2cbrush_t *b = &prv->brushes[sub->firstbrush+i];
		l->type = BIH_BRUSH;
		l->data.brush = b;

		l->data.contents = b->contents;
		VectorCopy(b->absmins, l->mins);
		VectorCopy(b->absmaxs, l->maxs);
		l++;
	}
	for (i = 0; i < prv->numdisplacements; i++)
	{
		dispinfo_t *d = &prv->displacements[i];

		size_t j;
		for (j = 0; j < d->numindexes; j+=3)
		{
			index_t *v = d->idx+j;
			vec_t *v1 = d->xyz[v[0]], *v2 = d->xyz[v[1]], *v3 = d->xyz[v[2]];

			l->type = BIH_TRIANGLE;
			l->data.tri.xyz = d->xyz;
			l->data.tri.indexes = v;

			l->data.contents = d->contents;
			VectorCopy(v1, l->mins);
			VectorCopy(v1, l->maxs);
			AddPointToBounds(v2, l->mins, l->maxs);
			AddPointToBounds(v3, l->mins, l->maxs);
			l++;
		}
	}
	for (i = 0; i < prv->numstaticprops; i++)
	{
		if (!prv->staticprops[i].solid)
			continue;

		l->type = BIH_MODEL;
		l->data.mesh.model = prv->staticprops[i].ent.model;
		l->data.mesh.tr = &prv->staticprops[i].transform;

		//*cry*
		while (l->data.mesh.model->loadstate==MLS_LOADING)
			threadfuncs->WaitForCompletion(l->data.mesh.model, &l->data.mesh.model->loadstate, MLS_LOADING);

		if (!l->data.mesh.model || !l->data.mesh.model->funcs.NativeTrace || !l->data.mesh.model->funcs.NativeContents)
		{
			Con_Printf("%s has no collision info and must not be used as a solid static prop\n", l->data.mesh.model->name);
			continue;
		}

		l->data.contents = ~0u; //yuck!

		//a clever person could probably do a better job.
		l->mins[0] = prv->staticprops[i].ent.origin[0] - l->data.mesh.model->radius;
		l->mins[1] = prv->staticprops[i].ent.origin[1] - l->data.mesh.model->radius;
		l->mins[2] = prv->staticprops[i].ent.origin[2] - l->data.mesh.model->radius;
		l->maxs[0] = prv->staticprops[i].ent.origin[0] + l->data.mesh.model->radius;
		l->maxs[1] = prv->staticprops[i].ent.origin[1] + l->data.mesh.model->radius;
		l->maxs[2] = prv->staticprops[i].ent.origin[2] + l->data.mesh.model->radius;
		l++;
	}

	modfuncs->BIH_Build(mod, bihleaf, l-bihleaf);
	plugfuncs->Free(bihleaf);

	mod->funcs.PointContents		= VBSP_PointContents;
}

/*
===============================================================================

Rendering stuff

===============================================================================
*/

#ifdef HAVE_CLIENT
static qboolean VBSP_CullBox (vec3_t mins, vec3_t maxs)
{
	//this isn't very precise.
	//checking each plane individually can be problematic
	//if you have a large object behind the view, it can cross multiple planes, and be infront of each one at some point, yet should still be outside the view.
	//this is quite noticable with terrain where the potential height of a section is essentually infinite.
	//note that this is not a concern for spheres, just boxes.
	int		i;

	for (i = 0; i < refdef->frustum_numplanes; i++)
		if (BOX_ON_PLANE_SIDE (mins, maxs, &refdef->frustum[i]) == 2)
			return true;
	return false;
}
static void VBSP_RecursiveWorldNode (model_t *model, mnode_t *node)
{
	int			c, side;
	mplane_t	*plane;
	msurface_t	*surf, **mark;
	mleaf_t		*pleaf;
	double		dot;

	int sidebit;

	if (node->contents == FTECONTENTS_SOLID)
		return;		// solid

	if (node->visframe != vbsp_nodesequence)
		return;
	if (VBSP_CullBox (node->minmaxs, node->minmaxs+3))
		return;

// if a leaf node, draw stuff
	if (node->contents != -1)
	{
		pleaf = (mleaf_t *)node;

		// check for door connected areas
		if (! (refdef->areabits[pleaf->area>>3] & (1<<(pleaf->area&7)) ) )
			return;		// not visible

		c = pleaf->cluster;
		if (c >= 0)
			frustumvis[c>>3] |= 1<<(c&7);

		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		if (c)
		{
			do
			{
				surf = *mark++;
				if (surf->flags & SURF_OFFNODE)
				{
					if (surf->visframe != vbsp_surfsequence)
					{	//only add once, it might be in multiple leafs.
						surf->visframe = vbsp_surfsequence;
						modfuncs->RenderDynamicLightmaps (surf);
						surf->sbatch->mesh[surf->sbatch->meshes++] = surf->mesh;
					}
				}
				else
					surf->visframe = vbsp_surfsequence;
			} while (--c);
		}
		return;
	}

// node is just a decision point, so go down the apropriate sides

// find which side of the node we are on
	plane = node->plane;

	switch (plane->type)
	{
	case PLANE_X:
		dot = modelorg[0] - plane->dist;
		break;
	case PLANE_Y:
		dot = modelorg[1] - plane->dist;
		break;
	case PLANE_Z:
		dot = modelorg[2] - plane->dist;
		break;
	default:
		dot = DotProduct (modelorg, plane->normal) - plane->dist;
		break;
	}

	if (dot >= 0)
	{
		side = 0;
		sidebit = 0;
	}
	else
	{
		side = 1;
		sidebit = SURF_PLANEBACK;
	}

// recurse down the children, front side first
	VBSP_RecursiveWorldNode (model, node->children[side]);

	// draw stuff
	for ( c = node->numsurfaces, surf = model->surfaces + node->firstsurface; c ; c--, surf++)
	{
		if (surf->visframe != vbsp_surfsequence)
			continue;

		if ( (surf->flags & SURF_PLANEBACK) != sidebit )
			continue;		// wrong side

		surf->visframe = 0;//vbsp_surfsequence;//-1;

		modfuncs->RenderDynamicLightmaps (surf);

		surf->sbatch->mesh[surf->sbatch->meshes++] = surf->mesh;
	}


// recurse down the back side
	VBSP_RecursiveWorldNode (model, node->children[!side]);
}
static qbyte *VBSP_MarkLeaves (model_t *model, int clusters[2])
{
	vbspinfo_t	*prv = (vbspinfo_t*)model->meshinfo;
	mnode_t	*node;
	int		i;

	int cluster;
	mleaf_t	*leaf;
	qbyte *vis;

	int portal = refdef->recurse;

	if (refdef->forcevis)
	{
		vis = refdef->forcedvis;
		prv->vcache.vis = NULL;
	}
	else if (portal || hl2_novis->ival || clusters[0] == -1 || !model->vis)
		return NULL;	//use some blind whole-model thing
	else
	{
		vis = prv->vcache.vis;
		if (prv->vcache.viewcluster[0] == clusters[0] && prv->vcache.viewcluster[1] == clusters[1] && vis)
			return vis;

		if (clusters[1] != clusters[0])	// may have to combine two clusters because of solid water boundaries
		{
			vis = VBSP_ClusterPVS (model, clusters[0], &prv->vcache.visbuf, PVM_REPLACE);
			vis = VBSP_ClusterPVS (model, clusters[1], &prv->vcache.visbuf, PVM_MERGE);
		}
		else
			vis = VBSP_ClusterPVS (model, clusters[0], &prv->vcache.visbuf, PVM_FAST);
		prv->vcache.vis = vis;
		prv->vcache.viewcluster[0] = clusters[0];
		prv->vcache.viewcluster[1] = clusters[1];
	}

	vbsp_nodesequence++;

	for (i=0,leaf=model->leafs ; i<model->numleafs ; i++, leaf++)
	{
		cluster = leaf->cluster;
		if (cluster == -1)
			continue;
		if (vis[cluster>>3] & (1<<(cluster&7)))
		{
			node = (mnode_t *)leaf;
			do
			{
				if (node->visframe == vbsp_nodesequence)
					break;
				node->visframe = vbsp_nodesequence;
				node = node->parent;
			} while (node);
		}
	}
	return vis;
}

qboolean HL2_CalcModelLighting(entity_t *e, model_t *clmodel, refdef_t *r_refdef, model_t *mod)
{
	vec3_t lightdir;
	vec3_t shadelight, ambientlight;

	e->light_dir[0] = 0; e->light_dir[1] = 1; e->light_dir[2] = 0;
	mod->funcs.LightPointValues(mod, e->origin, shadelight, ambientlight, lightdir);

	e->light_dir[0] = DotProduct(lightdir, e->axis[0]);
	e->light_dir[1] = DotProduct(lightdir, e->axis[1]);
	e->light_dir[2] = DotProduct(lightdir, e->axis[2]);

	VectorNormalize(e->light_dir);

	shadelight[0] *= 1/255.0f;
	shadelight[1] *= 1/255.0f;
	shadelight[2] *= 1/255.0f;
	ambientlight[0] *= 1/255.0f;
	ambientlight[1] *= 1/255.0f;
	ambientlight[2] *= 1/255.0f;

	//calculate average and range, to allow for negative lighting dotproducts
	VectorCopy(shadelight, e->light_avg);
	VectorCopy(ambientlight, e->light_range);

	e->light_known = 1;
	return e->light_known-1;
}

static void VBSP_PrepareFrame(model_t *mod, refdef_t *r_refdef, int area, int clusters[2], pvsbuffer_t *vis, qbyte **entvis_out, qbyte **surfvis_out)
{
	vbspinfo_t	*prv = (vbspinfo_t*)mod->meshinfo;
	qbyte *surfvis, *entvis;

	refdef = r_refdef;

	if (vis->buffersize < mod->pvsbytes)
		vis->buffer = plugfuncs->Realloc(vis->buffer, vis->buffersize=mod->pvsbytes);
	frustumvis = vis->buffer;
	memset(frustumvis, 0, mod->pvsbytes);

	if (!r_refdef->areabitsknown)
	{	//generate the info each frame, as the gamecode didn't tell us what to use.
		int leafnum = VBSP_PointLeafnum (mod, r_refdef->vieworg);
		int clientarea = VBSP_LeafArea (mod, leafnum);
		VBSP_WriteAreaBits(mod, r_refdef->areabits, clientarea, false);
		r_refdef->areabitsknown = true;
	}


	entvis = surfvis = VBSP_MarkLeaves(mod, clusters);
	VectorCopy (r_refdef->vieworg, modelorg);
	if (!surfvis)
	{
		size_t i;
		msurface_t *surf;
		for (i = 0; i < mod->nummodelsurfaces; i++)
		{
			surf = &mod->surfaces[i];
			modfuncs->RenderDynamicLightmaps (surf);
			surf->sbatch->mesh[surf->sbatch->meshes++] = surf->mesh;
		}
	}
	else
	{
		size_t i;
		dispinfo_t *disp;
		msurface_t *surf;
		int areas[2];
		areas[0] = 1;
		areas[1] = area;
		vbsp_surfsequence++;
		VBSP_RecursiveWorldNode (mod, mod->nodes);
		for (i = 0; i < prv->numdisplacements; i++)
		{
			disp = &prv->displacements[i];
			if (VBSP_EdictInFatPVS(mod, &disp->pvs, surfvis, areas))
			{
				surf = disp->surf;
				modfuncs->RenderDynamicLightmaps (surf);
				surf->sbatch->mesh[surf->sbatch->meshes++] = surf->mesh;
			}
		}
	}

	*surfvis_out = frustumvis;
	*entvis_out = entvis;



	if (prv->numstaticprops)
	{
		struct staticprop_s *sent;
		entity_t *src, *ent;
		float d;
		size_t i;
		vec3_t disp;
		for (i = 0; i < prv->numstaticprops; i++)
		{
			sent = &prv->staticprops[i];
			src = &sent->ent;

			if (sent->fademaxdist)
			{
				VectorSubtract(refdef->vieworg, src->origin, disp);
				d = VectorLength(disp);
				if (d > sent->fademaxdist)
					continue;	//skip it.
				d -= sent->fademindist;
				d /= sent->fademaxdist-sent->fademindist;
				if (d < 0)
					d = 0;
			}
			else
				d = 0;

			if (!src->model || src->model->loadstate != MLS_LOADED)
			{
				if (src->model && src->model->loadstate == MLS_NOTLOADED)
					modfuncs->GetModel(src->model->publicname, MLV_WARN);	//we use threads, so these'll load in time.
				continue;
			}
#if 1
			if (!src->light_known)
			{
				vec3_t tmp;
				VectorCopy(src->origin, tmp);
				VectorCopy(sent->lightorg, src->origin);
				HL2_CalcModelLighting(src, src->model, r_refdef, mod);
				VectorCopy(tmp, src->origin);

				/* HACK: If it's dark, might as well sample the model pos directly. */
				if (VectorLength(src->light_range) < 0.25)
				{
					HL2_CalcModelLighting(src, src->model, r_refdef, mod);
				}
			}
#endif
			if (src->pvscache.num_leafs==-2)
			{
				vec3_t absmin, absmax;
				float r = src->model->radius;
				VectorSet(absmin, -r,-r,-r);
				VectorSet(absmax, r,r,r);
				VectorAdd(absmin, src->origin, absmin);
				VectorAdd(absmax, src->origin, absmax);
				VBSP_FindTouchedLeafs(mod, &src->pvscache, absmin, absmax);
			}

			ent = modfuncs->NewSceneEntity();
			if (!ent)
				return;
			*ent = *src;
			ent->framestate.g[FS_REG].frametime[0] = refdef->time;
			ent->framestate.g[FS_REG].frametime[1] = refdef->time;
			if (d)
			{
				ent->shaderRGBAf[3] *= 1-d;
				ent->flags |= RF_TRANSLUCENT;
			}
		}
	}
}
static void VBSP_InfoForPoint (struct model_s *mod, vec3_t pos, int *area, int *cluster, unsigned int *contentbits)
{
	int leaf = VBSP_PointLeafnum_r (mod, pos, 0);
	*area = VBSP_LeafArea(mod, leaf);
	*cluster = VBSP_LeafCluster(mod, leaf);
	*contentbits = VBSP_LeafContents(mod, leaf);
}


#ifdef RTLIGHTS
static int vbsp_shadowsequence;
static qbyte *shadowedpvs;
static model_t *shadowmodel;
static void VBSP_WalkShadows (dlight_t *dl, void (*callback)(msurface_t *surf), mnode_t *node)
{
	int			c, side;
	mplane_t	*plane;
	msurface_t	*surf, **mark;
	mleaf_t		*pleaf;
	double		dot;

	float		l, maxdist;
	int			j, s, t;
	vec3_t		impact;
	struct vbsptexinfo_s *vtexinfo;
	vbspinfo_t *prv;

	if (node->shadowframe != vbsp_shadowsequence)
		return;

	//if light areabox is outside node, ignore node + children
	for (c = 0; c < 3; c++)
	{
		if (dl->origin[c] + dl->radius < node->minmaxs[c])
			return;
		if (dl->origin[c] - dl->radius > node->minmaxs[3+c])
			return;
	}

// if a leaf node, draw stuff
	if (node->contents != -1)
	{
		pleaf = (mleaf_t *)node;

		if (pleaf->cluster >= 0)
			shadowedpvs[pleaf->cluster>>3] |= 1<<(pleaf->cluster&7);

		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		if (c)
		{
			do
			{
				surf = *mark++;
				if (surf->flags & SURF_OFFNODE)
				{
					if (surf->shadowframe != vbsp_shadowsequence)
					{	//if its not on a node then its probably not a nice flat surface, so don't bother trying to cull it in fancy ways that depend on its plane.
						surf->shadowframe = vbsp_shadowsequence;
						callback(surf);
					}
				}
				else
					surf->shadowframe = vbsp_shadowsequence;
			} while (--c);
		}
		return;
	}

// node is just a decision point, so go down the apropriate sides

// find which side of the node we are on
	plane = node->plane;

	switch (plane->type)
	{
	case PLANE_X:
		dot = dl->origin[0] - plane->dist;
		break;
	case PLANE_Y:
		dot = dl->origin[1] - plane->dist;
		break;
	case PLANE_Z:
		dot = dl->origin[2] - plane->dist;
		break;
	default:
		dot = DotProduct (dl->origin, plane->normal) - plane->dist;
		break;
	}

	if (dot >= 0)
		side = 0;
	else
		side = 1;

// recurse down the children, front side first
	VBSP_WalkShadows (dl, callback, node->children[side]);

// draw stuff
  	c = node->numsurfaces;
	if (c)
	{
		prv = shadowmodel->meshinfo;
		surf = shadowmodel->surfaces + node->firstsurface;
		maxdist = dl->radius*dl->radius;
		for ( ; c ; c--, surf++)
		{
			if (surf->shadowframe != vbsp_shadowsequence)
				continue;

			if ((dot < 0) ^ !!(surf->flags & SURF_PLANEBACK))
				continue;		// wrong side

			/*if (surf->flags & (SURF_DRAWALPHA | SURF_DRAWTILED))
			{	// no shadows
				continue;
			}*/

			//is the light on the right side?
			if (surf->flags & SURF_PLANEBACK)
			{//inverted normal.
				if (-DotProduct(surf->plane->normal, dl->origin)+surf->plane->dist >= dl->radius)
					continue;
			}
			else
			{
				if (DotProduct(surf->plane->normal, dl->origin)-surf->plane->dist >= dl->radius)
					continue;
			}

			//Yeah, you can blame LordHavoc for this alternate code here.
			for (j=0 ; j<3 ; j++)
				impact[j] = dl->origin[j] - surf->plane->normal[j]*dot;

			vtexinfo = &prv->texinfo[surf->texinfo-shadowmodel->texinfo];
			// clamp center of light to corner and check brightness
			l = DotProduct (impact, vtexinfo->lmvecs[0]) + vtexinfo->lmvecs[0][3] - surf->texturemins[0];
			s = l;if (s < 0) s = 0;else if (s > surf->extents[0]) s = surf->extents[0];
			s = (l - s)*surf->texinfo->vecscale[0];
			l = DotProduct (impact, vtexinfo->lmvecs[1]) + vtexinfo->lmvecs[1][3] - surf->texturemins[1];
			t = l;if (t < 0) t = 0;else if (t > surf->extents[1]) t = surf->extents[1];
			t = (l - t)*surf->texinfo->vecscale[1];
			// compare to minimum light
			if ((s*s+t*t+dot*dot) < maxdist)
				callback(surf);
		}
	}

// recurse down the back side
	VBSP_WalkShadows (dl, callback, node->children[!side]);
}

static void VBSP_MarkShadows(model_t *model, dlight_t *dl, const qbyte *lvis)
{
	mnode_t *node;
	int i;
	mleaf_t *leaf;
	int cluster;

//	if (!dl->die)
	{
		//static
		//variation on mark leaves
		for (i=0,leaf=model->leafs ; i<model->numleafs ; i++, leaf++)
		{
			cluster = leaf->cluster;
			if (cluster == -1)
				continue;
			if (lvis[cluster>>3] & (1<<(cluster&7)))
			{
				node = (mnode_t *)leaf;
				do
				{
					if (node->shadowframe == vbsp_shadowsequence)
						break;
					node->shadowframe = vbsp_shadowsequence;
					node = node->parent;
				} while (node);
			}
		}
	}
/*	else
	{
		//dynamic lights will be discarded after this frame anyway, so only include leafs that are visible
		//variation on mark leaves
		for (i=0,leaf=model->leafs ; i<model->numleafs ; i++, leaf++)
		{
			cluster = leaf->cluster;
			if (cluster == -1)
				continue;
			if (lvis[cluster>>3] & (1<<(cluster&7)))
			{
				node = (mnode_t *)leaf;
				do
				{
					if (node->shadowframe == vbsp_shadowsequence)
						break;
					node->shadowframe = vbsp_shadowsequence;
					node = node->parent;
				} while (node);
			}
		}
	}*/
}
static void VBSP_GenerateShadowMesh(model_t *model, dlight_t *dl, const qbyte *lightvis, qbyte *litvis, void (*callback)(msurface_t *surf))
{
	vbspinfo_t *prv = model->meshinfo;
	dispinfo_t *disp;
	int i;
	//globals are evil
	shadowmodel = model;
	shadowedpvs = litvis;	//this is an output
	vbsp_shadowsequence++;

	VBSP_MarkShadows(model, dl, lightvis);

	VBSP_WalkShadows(dl, callback, model->nodes);

	for (i = 0; i < prv->numdisplacements; i++)
	{
		disp = &prv->displacements[i];
		if (VBSP_EdictInFatPVS(model, &disp->pvs, litvis, NULL))
		{
			callback(disp->surf);
		}
	}
}
#endif




static void VBSP_LoadLeafLight (model_t *mod, qbyte *mod_base, vlump_t *hdridx, vlump_t *ldridx, vlump_t *hdrvals, vlump_t *ldrvals, int version)
{
	vbspinfo_t	*prv = (vbspinfo_t*)mod->meshinfo;
	vlump_t *lump_idx, *lump_vals;
	struct leaflightpoint_s *point;
	size_t i, j;
	unsigned short *in;
	qbyte *inpoint;

	if (version == 17 || version == 19)
		return; //nope. this info is in the leafs.

	if (hdridx && hdrvals)
		lump_idx = hdridx, lump_vals = hdrvals;
	else if (ldridx && ldrvals)
		lump_idx = ldridx, lump_vals = ldrvals;
	else
		return;	//unsupported.

	if (lump_vals->filelen%(7*4))
		return;

	if (lump_idx->filelen != mod->numleafs*sizeof(short)*2)
		return;	//erk?

	//easy enough to load some of the data...
	point = plugfuncs->GMalloc(&mod->memgroup, sizeof(*point)*(lump_vals->filelen/(7*4)));
	inpoint = mod_base + lump_vals->fileofs;
	for (i = 0; i < lump_vals->filelen/(7*4); i++)
	{
		for (j = 0; j < 6; j++, inpoint+=4)
		{
			float e = pow(2, (signed char)inpoint[3]);
			point[i].rgb[j][0] = e * inpoint[0];
			point[i].rgb[j][1] = e * inpoint[1];
			point[i].rgb[j][2] = e * inpoint[2];
		}
		point[i].x = *inpoint++;
		point[i].y = *inpoint++;
		point[i].z = *inpoint++;
		inpoint++;
	}

	prv->leaflight = plugfuncs->GMalloc(&mod->memgroup, sizeof(*prv->leaflight)*mod->numleafs);
	in = (unsigned short*)(mod_base + lump_idx->fileofs);
	for (i = 0; i < mod->numleafs; i++)
	{
		prv->leaflight[i].count = LittleShort(*in++);
		prv->leaflight[i].point = point + (unsigned short)LittleShort(*in++);
	}
}
static void VBSP_LightPointValues	(struct model_s *model, const vec3_t point, vec3_t res_diffuse, vec3_t res_ambient, vec3_t res_dir)
{
	vbspinfo_t	*prv = (vbspinfo_t*)model->meshinfo;
	int leafnum = VBSP_PointLeafnum(model,point);
	mleaf_t *leaf = model->leafs+leafnum;
	struct mleaflight_s *leaflight = prv->leaflight+leafnum;
	struct leaflightpoint_s *best, *lp;
	size_t i, j, d, bd=~0;
	int xyz[3];
	vec3_t diff[6];
	float sig[6];

	static cvar_t *srgbmag, *scale, *forceface;
	if (!srgbmag)	srgbmag		= cvarfuncs->GetNVFDG("hl2_lt_srgb_mag","1",	0, "TEST", "TEST");
	if (!scale)		scale		= cvarfuncs->GetNVFDG("hl2_lt_scale",	"256",	0, "TEST", "TEST");
	if (!forceface)	forceface	= cvarfuncs->GetNVFDG("hl2_lt_face",	"-1",	0, "TEST", "TEST");

	if (prv->leaflight && leaflight->count)
	{
		for (i = 0; i < 3; i++)
			xyz[i] = 255*(point[i] - leaf->minmaxs[i]) / (leaf->minmaxs[3+i]-leaf->minmaxs[i]);
		for (i = 0, best=lp = leaflight->point; i < leaflight->count; i++, lp++)
		{
			int m[3];
			m[0] = xyz[0] - lp->x;
			m[1] = xyz[1] - lp->y;
			m[2] = xyz[2] - lp->z;
			d = DotProduct(m,m);
			if (bd > d)
			{
				bd = d;
				best = lp;
			}
		}


		VectorClear(res_ambient);
		for (j = 0; j < 6; j++)
			VectorAdd(res_ambient, best->rgb[j], res_ambient);
		VectorScale(res_ambient, 1.0/6, res_ambient);

		//try and figure out an average dir for the brightest direction
		for (j = 0; j < 6; j++)
		{
			VectorSubtract(best->rgb[j], res_ambient, diff[j]);
			sig[j] = VectorLength(diff[j]);
		}
		for (j = 0; j < 3; j++)
			res_dir[j] = sig[j*2+1] - sig[j*2];
		VectorNormalize(res_dir);

		//figure out how much light there should be in that direction.
		VectorCopy(res_ambient, res_diffuse);
		for (j = 0; j < 3; j++)
		{
			if (res_dir[0]>=0)
				VectorMA(res_diffuse, res_dir[0], diff[j*2+1], res_diffuse);
			else
				VectorMA(res_diffuse, -res_dir[0], diff[j*2+0], res_diffuse);
		}

		if (forceface->ival >= 0)
		{
			VectorCopy(best->rgb[forceface->ival], res_diffuse);
			VectorCopy(best->rgb[forceface->ival], res_ambient);
			VectorClear(res_dir);
			res_dir[forceface->ival/3] = (forceface->ival&1)?-1:1;
		}

		if (srgbmag->value)
		{
			for (j = 0; j < 3; j++)
			{
				res_diffuse[j] = M_LinearToSRGB(res_diffuse[j], srgbmag->value)*scale->value;
				res_ambient[j] = M_LinearToSRGB(res_ambient[j], srgbmag->value)*scale->value;
			}

			/*for (j = 0; j < 6; j++)
			{
				res_cube[j][0] = M_LinearToSRGB(best->rgb[j][0], srgbmag->value)*scale->value;
				res_cube[j][1] = M_LinearToSRGB(best->rgb[j][1], srgbmag->value)*scale->value;
				res_cube[j][2] = M_LinearToSRGB(best->rgb[j][2], srgbmag->value)*scale->value;
			}*/
		}
		else
		{
			VectorScale(res_diffuse, scale->value, res_diffuse);
			VectorScale(res_ambient, scale->value, res_ambient);

			/*for (j = 0; j < 6; j++)
				VectorScale(best->rgb[j], scale->value, res_cube[j]);*/
		}

		return;
	}
	VectorSet(res_dir, 0,0.707,0.707);
	VectorSet(res_diffuse, 64,64,64);
	VectorSet(res_ambient, 192,192,192);
}
#else
static void VBSP_LightPointValues	(struct model_s *model, const vec3_t point, vec3_t res_diffuse, vec3_t res_ambient, vec3_t res_dir)
{
	VectorSet(res_diffuse, 64,64,64);
	VectorSet(res_ambient, 192,192,192);
	VectorSet(res_dir, 0,0.707,0.707);
}
static void VBSP_LoadLeafLight (model_t *mod, qbyte *mod_base, vlump_t *hdridx, vlump_t *ldridx, vlump_t *hdrvals, vlump_t *ldrvals, int version)
{
}
#endif







static void VBSP_ComputeChecksum(model_t *mod, void *data, size_t length)
{
	unsigned int checksum = LittleLong (filefuncs->BlockChecksum(data, length));
	mod->checksum = mod->checksum2 = checksum;
}

static qboolean VBSP_LoadModel(model_t *mod, qbyte *mod_base, size_t filelen, char *loadname)
{
	dvbspheader_t *srcheader = (void*)mod_base;
	dvbspheader_t header;
	vbspinfo_t *prv = mod->meshinfo;
	size_t i;
	qboolean noerrors = true;
#ifdef HAVE_CLIENT
	qboolean haverenderer = qrenderer != QR_NONE;
#endif

	VBSP_TranslateContentBits_Setup(prv);

	mod->lightmaps.width = LMBLOCK_SIZE_MAX;
	mod->lightmaps.height = LMBLOCK_SIZE_MAX;

	mod->fromgame = fg_new;
	mod->engineflags |= MDLF_NEEDOVERBRIGHT;
	header.version = LittleLong(srcheader->version);
	for (i=0 ; i<HL2_MAXLUMPS ; i++)
	{
		header.lumps[i].filelen = LittleLong (srcheader->lumps[i].filelen);
		header.lumps[i].fileofs = LittleLong (srcheader->lumps[i].fileofs);
		//fixme: truncate lumps if they go off the end
	}

	if (header.lumps[VLUMP_ZIPFILE].filelen)
		modfuncs->LoadMapArchive(mod, mod_base+header.lumps[VLUMP_ZIPFILE].fileofs, header.lumps[VLUMP_ZIPFILE].filelen);

	// load into heap
	noerrors = noerrors && VBSP_LoadVertexes		(mod, mod_base, &header.lumps[VLUMP_VERTEXES]);
	noerrors = noerrors && VBSP_LoadEdges			(mod, mod_base, &header.lumps[VLUMP_EDGES]);
	noerrors = noerrors && VBSP_LoadSurfedges		(mod, mod_base, &header.lumps[VLUMP_SURFEDGES]);
#ifdef HAVE_CLIENT
	if (noerrors && haverenderer)
		VBSP_LoadLighting							(mod, mod_base, &header.lumps[VLUMP_LIGHTING_LDR], &header.lumps[VLUMP_LIGHTING_HDR]);
#endif
	noerrors = noerrors && VBSP_LoadSurfaces		(mod, mod_base, &header.lumps[VLUMP_TEXINFO]);
	noerrors = noerrors && VBSP_LoadPlanes			(mod, mod_base, &header.lumps[VLUMP_PLANES]);
	noerrors = noerrors && VBSP_LoadTexInfo			(mod, mod_base, header.lumps, loadname);
	if (noerrors)
		VBSP_LoadEntities							(mod, mod_base, &header.lumps[VLUMP_ENTITIES]);
	noerrors = noerrors && VBSP_LoadFaces			(mod, mod_base, header.lumps, header.version);
	noerrors = noerrors && VBSP_LoadDisplacements	(mod, mod_base, header.lumps);
	noerrors = noerrors && VBSP_LoadMarksurfaces	(mod, mod_base, &header.lumps[VLUMP_LEAFFACES]);
	noerrors = noerrors && VBSP_LoadVisibility		(mod, mod_base, &header.lumps[VLUMP_VISIBILITY]);
	noerrors = noerrors && VBSP_LoadBrushSides		(mod, mod_base, &header.lumps[VLUMP_BRUSHSIDES]);
	noerrors = noerrors && VBSP_LoadBrushes			(mod, mod_base, &header.lumps[VLUMP_BRUSHES]);
	noerrors = noerrors && VBSP_LoadLeafBrushes		(mod, mod_base, &header.lumps[VLUMP_LEAFBRUSHES]);
	noerrors = noerrors && VBSP_LoadLeafs			(mod, mod_base, &header.lumps[VLUMP_LEAFS], header.version);
	noerrors = noerrors && VBSP_LoadNodes			(mod, mod_base, &header.lumps[VLUMP_NODES]);
	noerrors = noerrors && VBSP_LoadSubmodels		(mod, mod_base, &header.lumps[VLUMP_MODELS]);
	noerrors = noerrors && VBSP_LoadAreas			(mod, mod_base, &header.lumps[VLUMP_AREAS]);
	noerrors = noerrors && VBSP_LoadAreaPortals		(mod, mod_base, &header.lumps[VLUMP_AREAPORTALS], &header.lumps[VLUMP_AREAPORTALVERTS]);
#ifdef HAVE_CLIENT
	if (noerrors && haverenderer)
		VBSP_LoadLeafLight							(mod, mod_base, &header.lumps[VLUMP_LEAFLIGHTI_HDR], &header.lumps[VLUMP_LEAFLIGHTI_LDR],
																	&header.lumps[VLUMP_LEAFLIGHTV_HDR], &header.lumps[VLUMP_LEAFLIGHTV_LDR], header.version);
#endif
	noerrors = noerrors && VBSP_LoadGameLump		(mod, mod_base, &header.lumps[VLUMP_GAMELUMP]);

	if (!noerrors)
		return false;
#ifdef HAVE_SERVER
	mod->funcs.FatPVS				= VBSP_FatPVS;
	mod->funcs.EdictInFatPVS		= VBSP_EdictInFatPVS;
	mod->funcs.FindTouchedLeafs		= VBSP_FindTouchedLeafs;
#endif
#ifdef HAVE_CLIENT
	mod->funcs.LightPointValues		= VBSP_LightPointValues;
//	mod->funcs.StainNode			= VBSP_StainNode;
//	mod->funcs.MarkLights			= VBSP_MarkLights;
	mod->funcs.GenerateShadowMesh	= VBSP_GenerateShadowMesh;
#endif
	mod->funcs.ClusterPVS			= VBSP_ClusterPVS;
	mod->funcs.ClusterPHS			= VBSP_ClusterPHS;
	mod->funcs.ClusterForPoint		= VBSP_PointCluster;

	mod->funcs.SetAreaPortalState	= VBSP_SetAreaPortalState;
	mod->funcs.AreasConnected		= VBSP_AreasConnected;
	mod->funcs.LoadAreaPortalBlob	= VBSP_LoadAreaPortalBlob;
	mod->funcs.SaveAreaPortalBlob	= VBSP_SaveAreaPortalBlob;

	mod->funcs.PrepareFrame			= VBSP_PrepareFrame;
	mod->funcs.InfoForPoint			= VBSP_InfoForPoint;

	//displacements suck
	for (i = 0; i < prv->numdisplacements; i++)
		VBSP_FindTouchedLeafs(mod, &prv->displacements[i].pvs, prv->displacements[i].aamin, prv->displacements[i].aamax);
//	if (noerrors)
//		CM_CreatePatchesForLeafs (mod, prv);

	return true;
}

/*
==================
CM_LoadMap

Loads in the map and all submodels
==================
*/
static qboolean VBSP_LoadMap (model_t *mod, void *filein, size_t filelen)
{
	unsigned		*buf;
	int				i;
	dvbspheader_t	header;
	model_t			*wmod = mod;
	char			loadname[32];

#ifdef HAVE_CLIENT
	qbyte *facedata = NULL;
	unsigned int facesize = 0;
#endif
	vbspinfo_t	*prv;

	filefuncs->FileBase (mod->name, loadname, sizeof(loadname));

	// free old stuff, just in case.
	mod->meshinfo = prv = plugfuncs->GMalloc(&mod->memgroup, sizeof(*prv));

	mod->type = mod_brush;

	//
	// load the file
	//
	buf = (unsigned	*)filein;
	if (!buf)
	{
		Con_Printf (CON_ERROR "Couldn't load %s\n", mod->name);
		return false;
	}

	header = *(dvbspheader_t *)(buf);
	header.magic = LittleLong(header.magic);
	header.version = LittleLong(header.version);

	ClearBounds(mod->mins, mod->maxs);

	switch(header.version)
	{
	case 17:	//vampire
	case 18:	//beta
	case 19:	//hl2,cs:s,hl2dm
	case 20:	//portal, l4d, hl2ep2
	case 21:	//cs:go, portal 2, l4d2
	//case 22:	//dota 2
	//case 23:	//dota 2
	//case 27:	//'contagion'
	//case 29:	//'titanfall'
		if (!VBSP_LoadModel(mod, filein, filelen, loadname))
			return false;
		break;
	default:
		Con_Printf (CON_ERROR "VBSP with unknown version (%s: %i should be 18, 19, 20, or 21)\n"
			, mod->name, header.version);
		return false;
	}

	if (map_autoopenportals->value)
		memset (prv->portalopen, 1, sizeof(prv->portalopen));	//open them all. Used for progs that havn't got a clue.
	else
		memset (prv->portalopen, 0, sizeof(prv->portalopen));	//make them start closed.
	FloodAreaConnections (prv);

	mod->nummodelsurfaces = mod->numsurfaces;
	memset(&mod->batches, 0, sizeof(mod->batches));
	mod->vbos = NULL;

	mod->numsubmodels = VBSP_NumInlineModels(mod);

	//in case anyone wants to know the typical player size...
	VectorSet(mod->hulls[1].clip_mins, -16,-16,-36);
	VectorSet(mod->hulls[1].clip_maxs, 16,16,36);
	mod->hulls[0].firstclipnode = prv->cmodels[0].headnode-mod->nodes;
	mod->rootnode = prv->cmodels[0].headnode;
	mod->nummodelsurfaces = prv->cmodels[0].numsurfaces;

#ifdef HAVE_CLIENT
	if (qrenderer != QR_NONE)
	{
		builddata_t *bd = plugfuncs->Malloc(sizeof(*bd) + facesize*mod->nummodelsurfaces);
		bd->buildfunc = VBSP_BuildSurfMesh;
		bd->paintlightmaps = true;
		memcpy(bd+1, facedata + mod->firstmodelsurface*facesize, facesize*mod->nummodelsurfaces);
		threadfuncs->AddWork(WG_MAIN, VBSP_GenerateMaterials, mod, bd, 0, 0);
	}
#endif

	for (i=1 ; i< mod->numsubmodels ; i++)
	{
		cmodel_t	*bm;

		char	name[MAX_QPATH];

		Q_snprintfz (name, sizeof(name), "*%i:%s", i, wmod->publicname);
		mod = modfuncs->BeginSubmodelLoad(name);
		*mod = *wmod;
		mod->archive = NULL;
		mod->entities_raw = NULL;
		mod->submodelof = wmod;
		Q_strncpyz(mod->publicname, name, sizeof(mod->publicname));
		Q_snprintfz (mod->name, sizeof(mod->name), "*%i:%s", i, wmod->name);
		memset(&mod->memgroup, 0, sizeof(mod->memgroup));

		bm = VBSP_InlineModel (wmod, name);

		mod->hulls[0].firstclipnode = -1;	//no nodes,
		if (bm->headleaf)
		{
			mod->leafs = bm->headleaf;
			mod->nodes = NULL;
			mod->hulls[0].firstclipnode = -1;	//make it refer directly to the first leaf, for things that still use numbers.
			mod->rootnode = (mnode_t*)bm->headleaf;
		}
		else
		{
			mod->leafs = wmod->leafs;
			mod->nodes = wmod->nodes;
			mod->hulls[0].firstclipnode = bm->headnode - mod->nodes;	//determine the correct node index
			mod->rootnode = bm->headnode;
		}
		mod->nummodelsurfaces = bm->numsurfaces;
		mod->firstmodelsurface = bm->firstsurface;

		VBSP_BuildBIHSubmodel(mod, i);

		memset(&mod->batches, 0, sizeof(mod->batches));
		mod->vbos = NULL;

		VectorCopy (bm->maxs, mod->maxs);
		VectorCopy (bm->mins, mod->mins);
#ifdef HAVE_CLIENT
		mod->radius = RadiusFromBounds (mod->mins, mod->maxs);

		if (qrenderer != QR_NONE)
		{
			builddata_t *bd = plugfuncs->Malloc(sizeof(*bd) + facesize*mod->nummodelsurfaces);
			bd->buildfunc = VBSP_BuildSurfMesh;
			bd->paintlightmaps = true;
			memcpy(bd+1, facedata + mod->firstmodelsurface*facesize, facesize*mod->nummodelsurfaces);
			threadfuncs->AddWork(WG_MAIN, VBSP_GenerateMaterials, mod, bd, i, 0);
		}
#endif
		modfuncs->EndSubmodelLoad(mod, MLS_LOADED);
	}

	//urgh, we need to wait for models to load in order to get their sizes. that requires being on the main thread and the caller will think we're loaded on completion so we can't safely pingpong it back before generating the bih tree
	threadfuncs->AddWork(WG_MAIN, VBSP_BuildBIHMain, wmod, NULL, 0, 0);

	//main thread should have a load of work to do now. worker thread should now be free to compute the hash before its finally marked as loaded and the temp file memory goes away.
	VBSP_ComputeChecksum(mod, filein, filelen);
	return true;
}


qboolean VBSP_Init(void)
{
	filefuncs = plugfuncs->GetEngineInterface(plugfsfuncs_name, sizeof(*filefuncs));
	modfuncs = plugfuncs->GetEngineInterface(plugmodfuncs_name, sizeof(*modfuncs));
	if (modfuncs && modfuncs->version != MODPLUGFUNCS_VERSION)
		modfuncs = NULL;
	threadfuncs = plugfuncs->GetEngineInterface(plugthreadfuncs_name, sizeof(*threadfuncs));

	if (modfuncs && filefuncs && threadfuncs)
	{
		modfuncs->RegisterModelFormatMagic("Source (V)BSP", "VBSP",4, VBSP_LoadMap);

#define MAPOPTIONS "Map Cvar Options"
		hl2_novis = cvarfuncs->GetNVFDG("r_novis", "0", 0, "Multiplier for how far displacements can move.", MAPOPTIONS);
		hl2_displacement_scale = cvarfuncs->GetNVFDG("hl2_displacement_scale", "1", CVAR_RENDERERLATCH|CVAR_CHEAT, "Multiplier for how far displacements can move.", MAPOPTIONS);
		hl2_favour_ldr = cvarfuncs->GetNVFDG("hl2_favour_ldr", "0", CVAR_RENDERERLATCH|CVAR_CHEAT, "Favour LDR data instead of HDR (when both are present).", MAPOPTIONS);
		map_noareas = cvarfuncs->GetNVFDG("map_noareas", "0", 0, "Ignore areaportals.", MAPOPTIONS);
		map_autoopenportals = cvarfuncs->GetNVFDG("map_autoopenportals", "0", CVAR_RENDERERLATCH, "When set to 1, force-opens all area portals. Normally these start closed and are opened by doors when they move, but this requires the gamecode to signal this.", MAPOPTIONS);
		hl2_contents_remap = cvarfuncs->GetNVFDG("hl2_contents_remap",
			"/*solid*/SOLID "
			"/*window*/WINDOW "
			"/*aux*/Q2AUX "
			"/*grate*/CLIP "		//would otherwise be LAVA
			"/*slime*/SLIME "
			"/*water*/WATER "
			"/*mist*/Q2MIST "
			"/*opaque*/7 "
			"/*testfogvolume*/8 "
			"/*??*/9 "
			"/*??*/10 "
			"/*team1*/11 "
			"/*team2*/12 "
			"/*ignorenodrawopaque*/13 "
			"/*movable*/-1 "	//would otherwise be LADDER
			"/*areaportal*/Q2AREAPORTAL "
			"/*playerclip*/PLAYERCLIP "
			"/*monsterclip*/MONSTERCLIP "
			"/*current_0*/Q2CURRENT_0 "
			"/*current_90*/Q2CURRENT_90 "
			"/*current_180*/Q2CURRENT_180 "
			"/*current_270*/Q2CURRENT_270 "
			"/*current_up*/Q2CURRENT_UP "
			"/*current_down*/Q2CURRENT_DOWN "
			"/*origin*/Q2ORIGIN "
			"/*monster*/BODY "
			"/*deadmonster*/CORPSE "
			"/*detail*/DETAIL "
			"/*translucent*/Q2TRANSLUCENT "
			"/*ladder*/LADDER "	//would otherwise be Q2LADDER
			"/*hitbox*/30 "
			"/*??*/SKY"
			,CVAR_RENDERERLATCH, "Specifies a table for hl2->internal contentbits (one entry for each source bit).", MAPOPTIONS);

		return true;
	}
	return false;
}
