// https://wiki.zeroy.com/index.php?title=Call_of_Duty_1:_d3dbsp
// https://wiki.zeroy.com/index.php?title=Call_of_Duty_2:_d3dbsp
//yes, shockingly badly documented. that's half the challenge though, right?
//vaguely derived from quake3.

#include "../plugin.h"
#include "../engine/common/com_mesh.h"
#include "../engine/common/com_bih.h"
static plugfsfuncs_t *filefuncs;
static plugmodfuncs_t *modfuncs;
static plugthreadfuncs_t	*threadfuncs;

typedef struct
{
	//materials are used for rendering and collisions
	q2mapsurface_t	*surfaces;	//for collision properties, texturing info, not actual surfaces.

	//generally useful stuff
	unsigned int codbspver;
	mplane_t *planes;
	size_t num_planes;
	struct codnode_s
	{
		mplane_t *plane;
		int childnum[2];
		ivec3_t mins, maxs;
	} *nodes;
	size_t numnodes;
	struct codleaf_s
	{
		int cluster;
		int area;
		//we don't care about what we don't understand.
		//unsigned int firstleafbrush;
		//unsigned int numleafbrushes;
		//unsigned int firstleafsurface;
		//unsigned int numleafsurfaces;
		//int cell;
		unsigned int firstlightindex;
		unsigned int numlightindexes;
	} *leaf;
	size_t numleafs;
	qbyte *pvsdata;

	//rendering stuff. this is pretty simple as its all soup.
	mesh_t soupverts;	//don't trust the indexes!
	struct codsoup_s
	{
		unsigned int vertex_offset;
		unsigned int index_offset;
		unsigned int index_fixup;
	} *soups;	//aka surfs

	//brush collision... this stuff all goes away once we've built our BIH.
	const struct codbsp_brushside_s
	{	//unswapped...
		union{
			unsigned int plane;
			float dist;
		};
		unsigned int material_idx;
	} *brushsides;
	size_t num_brushsides;
	q2cbrush_t *brushes;
	size_t num_brushes;
	//patch/soup collision nightmare.
	const struct codpatch_s
	{	//unswapped...
		short mat;	//material?
		short mode;	//mode
		union
		{
			struct
			{	//patch
				short w;	//width
				short h;	//height
				int unknown;	//unknown. flatness?
				int firstvert;	//first CODLUMP_COLLISIONVERTS
			} mode0;
			struct
			{	//soup
				short numverts;
				short numidx;
				int firstvert;	//first CODLUMP_COLLISIONVERTS
				int firstidx;	//first CODLUMP_COLLISIONINDEXES
			} mode1;
		};
	} *patches;
	size_t numpatches;
	const unsigned int *leafpatches;	//loadtime only, unswapped...
	size_t numleafpatches;
	vecV_t *patchvertexes;
	size_t numpatchvertexes;
	index_t *patchindexes;
	size_t numpatchindexes;

	unsigned short *lightindexes;
	size_t numlightindexes;
	struct codlight_s
	{
		int type;
		vec3_t xyz;
		vec3_t rgb;
		vec3_t dir;
		float scale;
		float fov;
	} *lights;
	size_t numlights;
} codbspinfo_t;

#define COD1BSP_VERSION 0x0000003b
#define COD2BSP_VERSION 0x00000004
enum
{
	COD1LUMP_MATERIALS=0,	//names, surfaceflags, and contentbits for said materials (so dedicated servers don't need to parse anything extra).
	COD1LUMP_LIGHTMAPS=1,	//just 2d images. 512*512 RGB ones.
	COD1LUMP_PLANES=2,		//used for nodes and brushsides
	COD1LUMP_BRUSHSIDES=3,	//which planes+materials to use for each side of the brushes...
	COD1LUMP_BRUSHES=4,		//defines which sets of said sides to group, and their material(contents).
//	COD1LUMP_UNKNOWN=5,
	COD1LUMP_SOUPS=6,		//q3 would call em surfaces. except they're ALL trisoup, none are planar/patches, they're prebatched (within their 'cells').
	COD1LUMP_SOUPVERTS=7,	//the vertex attributes for the soup
	COD1LUMP_SOUPINDEXES=8,	//the indexes, woo. how fancy. it ain't soup without this!
	COD1LUMP_CULLGROUPS=9,
	COD1LUMP_CULLGROUPINDEXES=10,
	COD1LUMP_PORTALVERTS=11,
	COD1LUMP_OCCLUDERS=12,
	COD1LUMP_OCCLUDERPLANES=13,
	COD1LUMP_OCCLUDEREDGES=14,
	COD1LUMP_OCCLUDERINDEXES=15,
	COD1LUMP_AABBTREES=16,		//some sort of tree...
	COD1LUMP_CELLS=17,			//part of the portal rendering system (reduced number vs leafs so there's less to compute)
	COD1LUMP_PORTALS=18,		//for walking between cells?
	COD1LUMP_LIGHTINDEXES=19,	//indexes into LIGHTVALUES
	COD1LUMP_NODES=20,			//tree leading to leafs.
	COD1LUMP_LEAFS=21,			//describes a small convex area
	COD1LUMP_LEAFBRUSHES=22,	//list of brushes so leafs can share them, for collision+pointcontents.
	COD1LUMP_LEAFPATCHES=23,	//list of patches and embedded meshes per leaf, for collision
	COD1LUMP_PATCHCOLLISION=24,	//defines which verts/topology etc to use for each mesh/patch
	COD1LUMP_COLLISIONVERTS=25,	//simple verts used for patch/embedded-mesh collision
	COD1LUMP_COLLISIONINDEXES=26,//indexes for the collision verts, only used for embedded meshes
	COD1LUMP_MODELS=27,			//sub (aka inline) models - the first entry is the worldmodel, the second is your "*1" model and upwards.
	COD1LUMP_VISIBILITY=28,		//numclusters, numrowbytes, then just packed rows per cluster (no compression like q3 unlike q1).
	COD1LUMP_ENTITIES=29,
	COD1LUMP_LIGHTS=30,
//	COD1LUMP_UNKNOWN=31,
//	COD1LUMP_UNKNOWN=32,
	COD1LUMP_COUNT=33
};
enum
{
	COD2LUMP_MATERIALS=0,
	COD2LUMP_LIGHTMAPS=1,
	COD2LUMP_LIGHTGRIDHASH=2,
	COD2LUMP_LIGHTGRIDVALUES=3,
	COD2LUMP_PLANES=4,
	COD2LUMP_BRUSHSIDES=5,
	COD2LUMP_BRUSHES=6,
	COD2LUMP_SOUPS=7,
	COD2LUMP_SOUPVERTS=8,
	COD2LUMP_SOUPINDEXES=9,
	COD2LUMP_CULLGROUPS=10,
	COD2LUMP_CULLGROUPINDEXES=11,
//	COD2LUMP_UNKNOWN=12,
//	COD2LUMP_UNKNOWN=13,
//	COD2LUMP_UNKNOWN=14,
//	COD2LUMP_UNKNOWN=15,
//	COD2LUMP_UNKNOWN=16,
	COD2LUMP_PORTALVERTS=17,
	COD2LUMP_OCCLUDERS=18,
	COD2LUMP_OCCLUDERPLANES=19,
	COD2LUMP_OCCLUDEREDGES=20,
	COD2LUMP_OCCLUDERINDEXES=21,
	COD2LUMP_AABBTREES=22,
	COD2LUMP_CELLS=23,
	COD2LUMP_PORTALS=24,
	COD2LUMP_NODES=25,
	COD2LUMP_LEAFS=26,
	COD2LUMP_LEAFBRUSHES=27,
//	COD2LUMP_LEAFPATCHES=28,	//o.O
	COD2LUMP_COLLISIONVERTS=29,
	COD2LUMP_COLLISIONEDGES=30,
	COD2LUMP_COLLISIONINDEXES=31,
	COD2LUMP_COLLISIONBORDERS=32,
	COD2LUMP_COLLISIONPARTS=33,
	COD2LUMP_COLLISIONAABBS=34,
	COD2LUMP_MODELS=35,
	COD2LUMP_VISIBILITY=36,
	COD2LUMP_ENTITIES=37,
	COD2LUMP_PATHS=38,
	COD2LUMP_COUNT=39
};

static struct codleaf_s *CODBSP_LeafForPoint (model_t *mod, const vec3_t p, int num)
{
	codbspinfo_t *prv = (codbspinfo_t*)mod->meshinfo;
	float		d;
	struct codnode_s	*node;
	mplane_t	*plane;

	while (num >= 0)
	{
		node = prv->nodes + num;
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

	return &prv->leaf[-1 - num];
}
static int	CODBSP_ClusterForPoint	(struct model_s *model, const vec3_t point, int *areaout)
{
	struct codleaf_s *leaf = CODBSP_LeafForPoint(model, point, 0);
	if (areaout)
		*areaout = leaf->area;
	return leaf->cluster;
}
static qbyte *CODBSP_ClusterPVS		(struct model_s *model, int cluster, pvsbuffer_t *pvsbuffer, pvsmerge_t merge)
{
	codbspinfo_t *prv = (codbspinfo_t*)model->meshinfo;
	size_t i;
	if (cluster >= 0 && cluster < model->numclusters)
	{
		qbyte *pvs = prv->pvsdata + cluster*model->pvsbytes;	//packed, without compresion.
		if (merge == PVM_FAST)
			return pvs;
		else
		{
			if (pvsbuffer->buffersize < model->pvsbytes)
				pvsbuffer->buffer = plugfuncs->Realloc(pvsbuffer->buffer, pvsbuffer->buffersize = model->pvsbytes);
			if (merge==PVM_REPLACE)
				memcpy(pvsbuffer->buffer, pvs, model->pvsbytes);
			else for (i = 0; i < model->pvsbytes; i++)
				pvsbuffer->buffer[i] |= pvs[i];	//slooooow
			return pvsbuffer->buffer;
		}
	}

	if (pvsbuffer)
	{
		if (pvsbuffer->buffersize < model->pvsbytes)
			pvsbuffer->buffer = plugfuncs->Realloc(pvsbuffer->buffer, pvsbuffer->buffersize = model->pvsbytes);
		if (merge!=PVM_MERGE)
			memset(pvsbuffer->buffer, 0, model->pvsbytes);
		return pvsbuffer->buffer;
	}
	return NULL;
}
//static qbyte *CODBSP_ClusterPHS		(struct model_s *model, int cluster, pvsbuffer_t *pvsbuffer){return "\xff";}


//static void CODBSP_PurgeModel(struct model_s *mod){}

//static qbyte *CODBSP_ClustersInSphere(struct model_s *model, const vec3_t point, float radius, pvsbuffer_t *pvsbuffer, const qbyte *fte_restrict unionwith){}

//static size_t CODBSP_WriteAreaBits(struct model_s *model, qbyte *buffer, size_t maxbytes, int area, qboolean merge){}
//static qboolean CODBSP_AreasConnected(struct model_s *model, unsigned int area1, unsigned int area2){}
//static void CODBSP_SetAreaPortalState(struct model_s *model, unsigned int portal, unsigned int area1, unsigned int area2, qboolean open){}
//static size_t CODBSP_SaveAreaPortalBlob(struct model_s *model, void **ptr){}
//static size_t CODBSP_LoadAreaPortalBlob(struct model_s *model, void *ptr, size_t size){}


#ifdef HAVE_SERVER
static int		leaf_count, leaf_maxcount;
static struct codleaf_s **leaf_list;
static const float	*leaf_mins, *leaf_maxs;
static int		leaf_topnode;
#define BoxOnPlaneSide CodBoxOnPlaneSide
static int BoxOnPlaneSide (const vec3_t emins, const vec3_t emaxs, const mplane_t *p)
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
static void CODBSP_BoxLeafs_r (codbspinfo_t *prv, int nodenum)
{
	mplane_t	*plane;
	struct codnode_s		*node;
	int		s;
	while (1)
	{
		if (nodenum < 0)
		{
			if (leaf_count >= leaf_maxcount)
				return;
			leaf_list[leaf_count++] = &prv->leaf[-1 - nodenum];
			return;
		}
		node = &prv->nodes[nodenum];
		plane = node->plane;
		s = BOX_ON_PLANE_SIDE(leaf_mins, leaf_maxs, plane);
		if (s == 1)
			nodenum = node->childnum[0];
		else if (s == 2)
			nodenum = node->childnum[1];
		else
		{	// go down both
			if (leaf_topnode == -1)
				leaf_topnode = nodenum;
			CODBSP_BoxLeafs_r (prv, node->childnum[0]);
			nodenum = node->childnum[1];
		}
	}
}
static int	CODBSP_BoxLeafs (model_t *mod, const vec3_t mins, const vec3_t maxs, struct codleaf_s **list, int listsize, int *topnode)
{
	leaf_list = list;
	leaf_count = 0;
	leaf_maxcount = listsize;
	leaf_mins = mins;
	leaf_maxs = maxs;

	leaf_topnode = -1;

	CODBSP_BoxLeafs_r (mod->meshinfo, 0);

	if (topnode)
		*topnode = leaf_topnode;

	return leaf_count;
}
static unsigned int CODBSP_FatPVS		(struct model_s *mod, const vec3_t org, pvsbuffer_t *result, qboolean merge)
{
	struct codleaf_s *leafs[64];
	int		i, j, count;
	vec3_t	mins, maxs;

	for (i=0 ; i<3 ; i++)
	{
		mins[i] = org[i] - 8;
		maxs[i] = org[i] + 8;
	}

	count = CODBSP_BoxLeafs (mod, mins, maxs, leafs, countof(leafs), NULL);
	if (count < 1)
		Sys_Errorf ("CODBSP_FatPVS: count < 1");

	//grow the buffer if needed
	if (result->buffersize < mod->pvsbytes)
		result->buffer = plugfuncs->Realloc(result->buffer, result->buffersize=mod->pvsbytes);

	if (count == 1 && leafs[0]->cluster == -1)
	{	//if the only leaf is the outside then broadcast it.
		memset(result->buffer, 0xff, mod->pvsbytes);
		i = count;
	}
	else
	{
		i = 0;
		if (!merge)
			mod->funcs.ClusterPVS(mod, leafs[i++]->cluster, result, PVM_REPLACE);
		// or in all the other leaf bits
		for ( ; i<count ; i++)
		{
			for (j=0 ; j<i ; j++)
				if (leafs[i] == leafs[j])
					break;
			if (j != i)
				continue;		// already have the cluster we want
			mod->funcs.ClusterPVS(mod, leafs[i]->cluster, result, PVM_MERGE);
		}
	}
	return mod->pvsbytes;
}
static qboolean CODBSP_HeadnodeVisible (codbspinfo_t *prv, int nodenum, const qbyte *visbits)
{
	int		leafnum;
	int		cluster;
	struct codnode_s *node;

	if (nodenum < 0)
	{
		leafnum = -1-nodenum;
		cluster = prv->leaf[leafnum].cluster;
		if (cluster == -1)
			return false;
		if (visbits[cluster>>3] & (1<<(cluster&7)))
			return true;
		return false;
	}

	node = &prv->nodes[nodenum];
	if (CODBSP_HeadnodeVisible(prv, node->childnum[0], visbits))
		return true;
	return CODBSP_HeadnodeVisible(prv, node->childnum[1], visbits);
}
static qboolean CODBSP_EdictInFatPVS	(struct model_s *mod, const struct pvscache_s *ent, const qbyte *pvs, const int *areas)
{
	int i,l;
	/*int nullarea = -1;
	if (areas)
	{	//areas[0] is the count of areas the camera is in, if valid. requires us to track portal states...
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
			else if (CODBSP_AreasConnected (mod, areas[i], ent->areanum))
				break;
			// doors can legally straddle two areas, so
			// we may need to check another one
			else if (ent->areanum2 != nullarea && CODBSP_AreasConnected (mod, areas[i], ent->areanum2))
				break;
		}
	}*/

	if (ent->num_leafs == -1)
	{	// too many leafs for individual check, go by headnode
		if (!CODBSP_HeadnodeVisible (mod->meshinfo, ent->headnode, pvs))
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
static void CODBSP_FindTouchedLeafs	(struct model_s *model, struct pvscache_s *ent, const vec3_t mins, const vec3_t maxs)
{
#define MAX_TOTAL_ENT_LEAFS		MAX_ENT_LEAFS+1
	struct codleaf_s *leafs[MAX_TOTAL_ENT_LEAFS];
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
	num_leafs = CODBSP_BoxLeafs (model, mins, maxs, leafs, MAX_TOTAL_ENT_LEAFS, &topnode);

	// set areas
	for (i=0 ; i<num_leafs ; i++)
	{
		clusters[i] = leafs[i]->cluster;	//could dedupe these.
		area = leafs[i]->area;
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
#endif
#ifdef HAVE_CLIENT
static void CODBSP_LightPointValues	(struct model_s *mod, const vec3_t point, vec3_t res_diffuse, vec3_t res_ambient, vec3_t res_dir)
{
	codbspinfo_t	*prv = (codbspinfo_t*)mod->meshinfo;
	struct codleaf_s *leaf = CODBSP_LeafForPoint(mod, point, 0);
	unsigned short *lightindexes = prv->lightindexes + leaf->firstlightindex;
	size_t i;
	struct codlight_s *light;
//	vec3_t move;
//	float d;
	float scale;

	if (!leaf->numlightindexes)
	{
		VectorSet(res_diffuse, 128,128,128);
		VectorSet(res_ambient, 128,128,128);
		VectorSet(res_dir, 1,0,0);
		return;
	}

	VectorSet(res_diffuse, 0,0,0);
	VectorSet(res_ambient, 0,0,0);
	VectorSet(res_dir, 0,0,0);

	for (i = 0; i < leaf->numlightindexes; i++, lightindexes++)
	{
		if (*lightindexes >= prv->numlights)
		{	// :( don't know what this is meant to signify, happens with the first index more often than not.
			if (!prv->numlights)
				continue;
			light = prv->lights;
		}
		else
			light = prv->lights + *lightindexes;

		switch (light->type)
		{
		case 1:	//sun...
			scale = 256;
			break;
/*		case 4:
			VectorSubtract(point, light->xyz, move);
			d = DotProduct(move,move);
			if (d > light->scale)
				continue;
			scale = (light->scale-d)/d;
			scale *= 256;
			break;*/
/*		case 5:
			break;*/
/*		case 7:
			break;*/
		default:
			continue;
		}
		VectorMA(res_diffuse,	scale, light->rgb, res_diffuse);
		VectorMA(res_ambient,	scale, light->rgb, res_ambient);
		VectorMA(res_dir,		scale, light->dir, res_dir);

		break; //:(
	}

	scale = DotProduct(res_dir,res_dir);
	if (scale <= 0)
		VectorSet(res_dir, 1,0,0);
	else
		VectorScale(res_dir, 1/scale, res_dir);
}
//static void CODBSP_GenerateShadowMesh	(struct model_s *model, dlight_t *dl, const qbyte *lvis, qbyte *truevis, void(*callback)(struct msurface_s*)){}
//static void CODBSP_StainNode			(struct model_s *model, float *parms){}
//static void CODBSP_MarkLights			(struct dlight_s *light, dlightbitmask_t bit, struct mnode_s *node){}

static void CODBSP_BuildSurfMesh(model_t *mod, msurface_t *surf, builddata_t *bd)
{	//just builds the actual mesh data, now that it has per-batch storage allocated.
	codbspinfo_t	*prv = (codbspinfo_t*)mod->meshinfo;
	mesh_t *mesh = surf->mesh;
	struct codsoup_s *soup = &prv->soups[surf-mod->surfaces];
	int i;

	mesh->istrifan = false;

	memcpy(mesh->xyz_array, prv->soupverts.xyz_array+soup->vertex_offset, sizeof(*mesh->xyz_array)*mesh->numvertexes);
	memcpy(mesh->normals_array, prv->soupverts.normals_array+soup->vertex_offset, sizeof(*mesh->normals_array)*mesh->numvertexes);
	for (i = 0;  i < mesh->numvertexes; i++)
		Vector4Scale(prv->soupverts.colors4b_array[soup->vertex_offset+i], 1.f/255, mesh->colors4f_array[0][i]);
	//memcpy(mesh->colors4b_array, prv->soupverts.colors4b_array+soup->vertex_offset, sizeof(*mesh->colors4b_array)*mesh->numvertexes);
	memcpy(mesh->st_array, prv->soupverts.st_array+soup->vertex_offset, sizeof(*mesh->st_array)*mesh->numvertexes);
	memcpy(mesh->lmst_array[0], prv->soupverts.lmst_array[0]+soup->vertex_offset, sizeof(*mesh->lmst_array[0])*mesh->numvertexes);

	if (soup->index_fixup)
	{
		for (i = 0; i < mesh->numindexes; i++)
			mesh->indexes[i] = prv->soupverts.indexes[soup->index_offset+i] - soup->index_fixup;
	}
	else
		memcpy(mesh->indexes, prv->soupverts.indexes+soup->index_offset, sizeof(*mesh->indexes)*mesh->numindexes);

	if (prv->soupverts.snormals_array)
	{	//cod2 made them explicit. yay.
		memcpy(mesh->snormals_array, prv->soupverts.snormals_array+soup->vertex_offset, sizeof(*mesh->snormals_array)*mesh->numvertexes);
		memcpy(mesh->tnormals_array, prv->soupverts.tnormals_array+soup->vertex_offset, sizeof(*mesh->tnormals_array)*mesh->numvertexes);
	}
	else
	{	//compute the tangents for rtlights.
		modfuncs->AccumulateTextureVectors(mesh->xyz_array, mesh->st_array, mesh->normals_array, mesh->snormals_array, mesh->tnormals_array, mesh->indexes, mesh->numindexes, false);
		modfuncs->NormaliseTextureVectors(mesh->normals_array, mesh->snormals_array, mesh->tnormals_array, mesh->numvertexes, false);
	}
}
static void CODBSP_GenerateMaterials(void *ctx, void *data, size_t a, size_t b)
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

static void CODBSP_PrepareFrame(struct model_s *mod, refdef_t *refdef, int area, int clusters[2], pvsbuffer_t *vis, qbyte **entvis_out, qbyte **surfvis_out)
{
	*entvis_out = *surfvis_out = CODBSP_ClusterPVS(mod, clusters[0], vis, false);
	if (clusters[1] != -1)
		CODBSP_ClusterPVS(mod, clusters[1], vis, true);

	/*if (!refdef->areabitsknown)
	{	//generate the info each frame, as the gamecode didn't tell us what to use.
		int leafnum = CODBSP_PointLeafnum (mod, refdef->vieworg);
		int clientarea = CODBSP_LeafArea (mod, leafnum);
		CODBSP_WriteAreaBits(mod, refdef->areabits, clientarea, false);
		refdef->areabitsknown = true;
	}*/

	if (0)
	{
		size_t i;
		msurface_t *surf;
		for (i = mod->firstmodelsurface+mod->nummodelsurfaces; i --> mod->firstmodelsurface; )
		{
			surf = &mod->surfaces[i];
			surf->sbatch->mesh[surf->sbatch->meshes++] = surf->mesh;
		}
	}
	else
	{
		size_t i;
		msurface_t *surf;
		for (i = mod->firstmodelsurface; i < mod->nummodelsurfaces; i++)
		{
			surf = &mod->surfaces[i];
			surf->sbatch->mesh[surf->sbatch->meshes++] = surf->mesh;
		}
	}

	//for static props...
	//ent = modfuncs->NewSceneEntity();
}
static void CODBSP_InfoForPoint(struct model_s *mod, vec3_t pos, int *area, int *cluster, unsigned int *contentbits)
{
	struct codleaf_s *leaf = CODBSP_LeafForPoint(mod, pos, 0);
	*area = leaf->area;
	*cluster = leaf->cluster;
	*contentbits = mod->funcs.PointContents(mod, NULL, pos);	//needs a proper pointcontents.
}
#endif

static qboolean CODBSP_LoadShaders (model_t *mod, qbyte *mod_base, lump_t *l)
{
	codbspinfo_t	*prv = (codbspinfo_t*)mod->meshinfo;
	struct
	{
		char shadername[64];
		unsigned int surfflags;
		unsigned int contents;
	} *in = (void *)(mod_base + l->fileofs);
	q2mapsurface_t	*out;
	int				i, count;
	texture_t *tex;

	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "CODBSP_LoadShaders: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count < 1)
	{
		Con_Printf (CON_ERROR "CODBSP_LoadShaders: Map with no shaders\n");
		return false;
	}

	mod->numtexinfo = count;
	out = prv->surfaces = plugfuncs->GMalloc(&mod->memgroup, count*sizeof(*out));

	mod->textures = plugfuncs->GMalloc(&mod->memgroup, (sizeof(texture_t*)+sizeof(mtexinfo_t)+sizeof(texture_t))*count);	//+1 is 'noshader' for flares.
	mod->texinfo = (mtexinfo_t*)(mod->textures+count);
	tex = (texture_t*)(mod->texinfo+count);
	mod->numtextures = count;

	for ( i=0 ; i<count ; i++, in++, out++ )
	{
		out->c.flags = LittleLong ( in->surfflags );
		out->c.value = LittleLong ( in->contents );
		Q_strlcpy(out->rname, in->shadername, sizeof(out->rname));

		mod->texinfo[i].texture = tex+i;
		mod->texinfo[i].flags = prv->surfaces[i].c.flags;
		Q_strlcpy(mod->texinfo[i].texture->name, in->shadername, sizeof(mod->texinfo[i].texture->name));
		mod->textures[i] = mod->texinfo[i].texture;
	}

	return true;
}

static qboolean COD1BSP_LoadLightmap(model_t *mod, qbyte *mod_base, lump_t *l)
{
//	codbspinfo_t	*prv = (codbspinfo_t*)mod->meshinfo;
	int overbright;
	int bytes;
	size_t i, lev;
	qbyte *in = mod_base+l->fileofs;
	mod->lightmaps.width = 512;
	mod->lightmaps.height = 512;
	mod->lightmaps.prebaked = PTI_RGB8;
	mod->lightmaps.fmt = LM_RGB8;
	bytes = 3;
	mod->lightmaps.surfstyles = 1;	//always style 0...
	mod->lightmaps.maxstyle = 0;
	mod->lightmaps.deluxemapping = false;
	mod->lightmaps.deluxemapping_modelspace = false;
	mod->lightmaps.first = 0;
	mod->lightmaps.count = l->filelen / (mod->lightmaps.width*mod->lightmaps.height*bytes);
	if (l->filelen != mod->lightmaps.count * (mod->lightmaps.width*mod->lightmaps.height*bytes))
	{
		Con_Printf (CON_ERROR "CODBSP_LoadLighting: funny lump size\n");
		return false;	//err... rounded badly.
	}

	mod->lightdata = plugfuncs->GMalloc(&mod->memgroup, l->filelen);
	mod->lightdatasize = l->filelen;

	overbright = cvarfuncs->GetFloat("gl_overbright");
	mod->engineflags = MDLF_NEEDOVERBRIGHT;
	if (overbright == 2)
		memcpy(mod->lightdata, in, l->filelen);
	else
	{
		qbyte *out = mod->lightdata;
		overbright = (1<<(2-overbright));
		for (i = 0; i < l->filelen; i++, in++)
		{
			lev = *in * overbright;
			*out++ = min(255, lev);
		}
	}
	return true;
}
static qboolean COD2BSP_LoadLightmap(model_t *mod, qbyte *mod_base, lump_t *l)
{
	//seems to be sets of 4 images (3 normalmaps and some extra discoloured one). more deluxemap than lightmap. this is not useful to us.
	//cod2 bundles some hlsl code, which may reveal clues.
//	codbspinfo_t	*prv = (codbspinfo_t*)mod->meshinfo;

	mod->lightmaps.width = 512;
	mod->lightmaps.height = 512;
	mod->lightmaps.prebaked = PTI_RGBA8;	//needs glsl to use properly.

	mod->lightmaps.surfstyles = 1;	//always style 0...
	mod->lightmaps.maxstyle = 0;
	mod->lightmaps.deluxemapping = false;	//fixme: uses 4 lightmap textures at a time.
	mod->lightmaps.deluxemapping_modelspace = false;
	mod->lightmaps.first = 0;
	mod->lightmaps.count = 0;

	Con_Printf(CON_WARNING"COD2 lightmaps are not supported\n");
	return true;
}
static qboolean COD1BSP_LoadSoupVertices (model_t *mod, qbyte *mod_base, lump_t *l)
{
	codbspinfo_t	*prv = (codbspinfo_t*)mod->meshinfo;
	struct
	{
		vec3_t position;
		vec2_t tc;
		vec2_t lmtc;
		vec3_t normal;
		byte_vec4_t rgba;
	} *in = (void*)((qbyte*)mod_base + l->fileofs);
	size_t i, count = l->filelen / sizeof(*in);
	mesh_t *mesh = &prv->soupverts;
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "CODBSP_LoadSoupVertices: funny lump size\n");
		return false;
	}

	//allocate lots of space. stoopid separate arrays.
	prv->soupverts.istrifan = false;
	prv->soupverts.xyz_array = plugfuncs->GMalloc(&mod->memgroup, count*(
			sizeof(*mesh->xyz_array)+
			sizeof(*mesh->normals_array)+
			sizeof(*mesh->colors4b_array)+
			sizeof(*mesh->st_array)+
			sizeof(*mesh->lmst_array[0])));
	mesh->normals_array		= (void*)(mesh->xyz_array		+ count);
	mesh->colors4b_array	= (void*)(mesh->normals_array	+ count);
	mesh->st_array			= (void*)(mesh->colors4b_array	+ count);
	mesh->lmst_array[0]		= (void*)(mesh->st_array		+ count);

	//copy it all over.
	for (i = 0; i < count; i++, in++)
	{
		mesh->xyz_array[i][0] = LittleFloat(in->position[0]);
		mesh->xyz_array[i][1] = LittleFloat(in->position[1]);
		mesh->xyz_array[i][2] = LittleFloat(in->position[2]);
		mesh->st_array[i][0] = LittleFloat(in->tc[0]);
		mesh->st_array[i][1] = LittleFloat(in->tc[1]);
		mesh->lmst_array[0][i][0] = LittleFloat(in->lmtc[0]);
		mesh->lmst_array[0][i][1] = LittleFloat(in->lmtc[1]);
		mesh->normals_array[i][0] = LittleFloat(in->normal[0]);
		mesh->normals_array[i][1] = LittleFloat(in->normal[1]);
		mesh->normals_array[i][2] = LittleFloat(in->normal[2]);
		mesh->colors4b_array[i][0] = in->rgba[0];
		mesh->colors4b_array[i][1] = in->rgba[1];
		mesh->colors4b_array[i][2] = in->rgba[2];
		mesh->colors4b_array[i][3] = in->rgba[3];
	}
	return true;
}
static qboolean COD2BSP_LoadSoupVertices (model_t *mod, qbyte *mod_base, lump_t *l)
{
	codbspinfo_t	*prv = (codbspinfo_t*)mod->meshinfo;
	struct
	{	//slightly rearranged for some reason (plus addition of tangents at the end)
		vec3_t position;
		vec3_t normal;
		byte_vec4_t rgba;
		vec2_t tc;
		vec2_t lmtc;
		vec3_t sdir;
		vec3_t tdir;
	} *in = (void*)((qbyte*)mod_base + l->fileofs);
	size_t i, count = l->filelen / sizeof(*in);
	mesh_t *mesh = &prv->soupverts;
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "CODBSP_LoadSoupVertices: funny lump size\n");
		return false;
	}

	//allocate lots of space. stoopid separate arrays.
	prv->soupverts.istrifan = false;
	prv->soupverts.xyz_array = plugfuncs->GMalloc(&mod->memgroup, count*(
			sizeof(*mesh->xyz_array)+
			sizeof(*mesh->normals_array)+
			sizeof(*mesh->snormals_array)+
			sizeof(*mesh->tnormals_array)+
			sizeof(*mesh->colors4b_array)+
			sizeof(*mesh->st_array)+
			sizeof(*mesh->lmst_array[0])));
	mesh->normals_array		= (void*)(mesh->xyz_array		+ count);
	mesh->snormals_array	= (void*)(mesh->normals_array		+ count);
	mesh->tnormals_array	= (void*)(mesh->snormals_array		+ count);
	mesh->colors4b_array	= (void*)(mesh->tnormals_array	+ count);
	mesh->st_array			= (void*)(mesh->colors4b_array	+ count);
	mesh->lmst_array[0]		= (void*)(mesh->st_array		+ count);

	//copy it all over.
	for (i = 0; i < count; i++, in++)
	{
		mesh->xyz_array[i][0] = LittleFloat(in->position[0]);
		mesh->xyz_array[i][1] = LittleFloat(in->position[1]);
		mesh->xyz_array[i][2] = LittleFloat(in->position[2]);
		mesh->st_array[i][0] = LittleFloat(in->tc[0]);
		mesh->st_array[i][1] = LittleFloat(in->tc[1]);
		mesh->lmst_array[0][i][0] = LittleFloat(in->lmtc[0]);
		mesh->lmst_array[0][i][1] = LittleFloat(in->lmtc[1]);
		mesh->normals_array[i][0] = LittleFloat(in->normal[0]);
		mesh->normals_array[i][1] = LittleFloat(in->normal[1]);
		mesh->normals_array[i][2] = LittleFloat(in->normal[2]);
		mesh->snormals_array[i][0] = LittleFloat(in->sdir[0]);
		mesh->snormals_array[i][1] = LittleFloat(in->sdir[1]);
		mesh->snormals_array[i][2] = LittleFloat(in->sdir[2]);
		mesh->tnormals_array[i][0] = LittleFloat(in->tdir[0]);
		mesh->tnormals_array[i][1] = LittleFloat(in->tdir[1]);
		mesh->tnormals_array[i][2] = LittleFloat(in->tdir[2]);
		mesh->colors4b_array[i][0] = in->rgba[0];
		mesh->colors4b_array[i][1] = in->rgba[1];
		mesh->colors4b_array[i][2] = in->rgba[2];
		mesh->colors4b_array[i][3] = in->rgba[2];
	}
	return true;
}
static qboolean CODBSP_LoadSoupIndexes (model_t *mod, qbyte *mod_base, lump_t *l)
{
	codbspinfo_t	*prv = (codbspinfo_t*)mod->meshinfo;
	unsigned short *in = (void*)((qbyte*)mod_base + l->fileofs);
	size_t i, count = l->filelen / sizeof(*in);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "CODBSP_LoadSoupIndexes: funny lump size\n");
		return false;
	}
	prv->soupverts.indexes = plugfuncs->GMalloc(&mod->memgroup, count*sizeof(*prv->soupverts.indexes));
	for (i = 0; i < count; i++, in++)
		prv->soupverts.indexes[i] = LittleShort(*in);

	return true;
}
static qboolean CODBSP_LoadSoups (model_t *mod, qbyte *mod_base, lump_t *l)
{
	codbspinfo_t	*prv = (codbspinfo_t*)mod->meshinfo;
	struct
	{
		unsigned short material_idx;
		unsigned short lightmap_idx;
		unsigned int vertex_offset;
		unsigned short vertex_count;
		unsigned short index_count;
		unsigned int index_offset;
	} *in = (void*)((qbyte*)mod_base + l->fileofs);
	struct codsoup_s *out;
	mesh_t *mesh;
	size_t j, i, count = l->filelen / sizeof(*in);
	unsigned int mn,mx, idx;
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "CODBSP_LoadSoups: funny lump size\n");
		return false;
	}
	prv->soups = out = plugfuncs->GMalloc(&mod->memgroup, count*sizeof(*out));
	mod->numsurfaces = count;
	mod->nummodelsurfaces = count;
	mod->surfaces = plugfuncs->GMalloc(&mod->memgroup,	count*sizeof(*mod->surfaces) +
														count*sizeof(*mesh));
	mesh = (void*)(mod->surfaces+count);
	for (i = 0; i < count; i++, in++, out++)
	{
		unsigned short tex	= LittleShort(in->material_idx);
		unsigned short lmap	= LittleShort(in->lightmap_idx);
		out->vertex_offset	= LittleLong (in->vertex_offset);
		out->index_offset	= LittleLong (in->index_offset);

		if (tex >= mod->numtexinfo)
			return false;

		for (j = 0; j < MAXCPULIGHTMAPS; j++)
			mod->surfaces[i].styles[j] = INVALID_LIGHTSTYLE;
		for (j = 0; j < MAXRLIGHTMAPS; j++)
		{
			mod->surfaces[i].vlstyles[j] = INVALID_VLIGHTSTYLE;
			mod->surfaces[i].lightmaptexturenums[j] = -1;
		}
		mod->surfaces[i].styles[0] = 0;
		mod->surfaces[i].lightmaptexturenums[0] = lmap==(unsigned short)~0u?INVALID_LIGHTSTYLE:lmap;

		mod->surfaces[i].texinfo = &mod->texinfo[tex];
		mod->surfaces[i].mesh = &mesh[i];
		mesh[i].numindexes = LittleShort(in->index_count);
		mesh[i].numvertexes = LittleShort(in->vertex_count);

		//cod2 sucks and is way out and results in horrible memory use. calculate what it should have been.
		mn = ~0i;
		mx = 0;
		for (j = 0; j < mesh[i].numindexes; j++)
		{
			idx = LittleLong(prv->soupverts.indexes[out->index_offset+j]);
			if (mx<= idx)
				mx = idx+1;
			if (mn > idx)
				mn = idx;
		}
		if (mx<mn)
			mx=mn=0;	//erk?
		mesh[i].numvertexes = mx-mn;
		if (mesh[i].numvertexes > 65535)
			return false;
		out->index_fixup = mn;
		out->vertex_offset += mn;
	}

	return true;
}

static qboolean CODBSP_LoadLights (model_t *mod, qbyte *mod_base, lump_t *l)
{
	codbspinfo_t	*prv = (codbspinfo_t*)mod->meshinfo;
	struct
	{	//18...
		int type;	//1=sunlight?
					//4=omni?
					//5=oriented non-spot?
					//7=oriented spot light?
		vec3_t rgb;	//these values are completely fucked in most modes. :(
		vec3_t xyz;	//actually matches a lightsource
		vec3_t dir;
		float pointnineish;	//some sort of exponent? falloff?
		float scale;		//?big float. radius? sometimes denormalised?
		float fov;		//very fovy
		int naught0;	//no info. could be floats.
		int naught1;	//no info. could be floats.
		int naught2;	//no info. could be floats.
		int naught3;	//no info. could be floats.
		int naught4;	//no info. could be floats.
	} *in = (void*)((qbyte*)mod_base + l->fileofs);
	struct codlight_s *out;
	size_t i, count = l->filelen / sizeof(*in);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "CODBSP_LoadLightValues: funny lump size\n");
		return false;
	}
	prv->lights = out = plugfuncs->GMalloc(&mod->memgroup, count*sizeof(*out));
	prv->numlights = count;
	for (i = 0; i < count; i++, in++, out++)
	{
		out->type	= LittleLong(in->type);
		out->xyz[0] = LittleFloat(in->xyz[0]);
		out->xyz[1] = LittleFloat(in->xyz[1]);
		out->xyz[2] = LittleFloat(in->xyz[2]);
		out->rgb[0] = LittleFloat(in->rgb[0]);
		out->rgb[1] = LittleFloat(in->rgb[1]);
		out->rgb[2] = LittleFloat(in->rgb[2]);
		out->dir[0] = LittleFloat(in->dir[0]);
		out->dir[1] = LittleFloat(in->dir[1]);
		out->dir[2] = LittleFloat(in->dir[2]);
		out->scale	= LittleFloat(in->scale);
		out->fov	= LittleFloat(in->fov);
	}

	return true;
}
static qboolean CODBSP_LoadLightIndexes (model_t *mod, qbyte *mod_base, lump_t *l)
{
	codbspinfo_t	*prv = (codbspinfo_t*)mod->meshinfo;
	unsigned short *in = (void*)((qbyte*)mod_base + l->fileofs), *out, v;
	size_t i, count = l->filelen / sizeof(*in);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "CODBSP_LoadLightIndexes: funny lump size\n");
		return false;
	}
	prv->lightindexes = out = plugfuncs->GMalloc(&mod->memgroup, count*sizeof(*out));
	prv->numlightindexes = count;
	for (i = 0; i < count; i++)
	{
		v = LittleShort(*in++);
		if (v == 0xffff)
			; //o.O
		else if (v >= prv->numlights)
		{
			Con_Printf (CON_ERROR "CODBSP_LoadLightIndexes: invalid index %i\n", v);
			return false;
		}
		*out++ = v;
	}
	return true;
}
static qboolean CODBSP_LoadEntities (model_t *mod, qbyte *mod_base, lump_t *l)
{	//just quake-style { "field" "value" "field2" "value2" } blocks.
	return modfuncs->LoadEntities(mod, mod_base+l->fileofs, l->filelen);
}

static qboolean CODBSP_LoadPlanes (model_t *mod, qbyte *mod_base, lump_t *l)
{
	codbspinfo_t *prv = (codbspinfo_t*)mod->meshinfo;
	vec4_t *in = (void*)((qbyte*)mod_base + l->fileofs);
	mplane_t *out;
	size_t j, i, count = l->filelen / sizeof(*in);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "CODBSP_LoadPlanes: funny lump size\n");
		return false;
	}
	prv->planes = out = plugfuncs->GMalloc(&mod->memgroup, count*sizeof(*out));
	prv->num_planes = count;
	for (i = 0; i < count; i++, out++)
	{
		out->normal[0] = LittleFloat(in[i][0]);
		out->normal[1] = LittleFloat(in[i][1]);
		out->normal[2] = LittleFloat(in[i][2]);
		out->dist = LittleFloat(in[i][3]);

		out->type = PLANE_ANYZ;
		out->signbits = 0;
		for (j=0 ; j<3 ; j++)
		{
			if (out->normal[j] < 0)
				out->signbits |= 1<<j;
			else if (out->normal[j] == 1)
				out->type = j;
		}
	}
	return true;
}
static qboolean CODBSP_LoadBrushSides (model_t *mod, qbyte *mod_base, lump_t *l)
{
	codbspinfo_t	*prv = (codbspinfo_t*)mod->meshinfo;
	struct codbsp_brushside_s *in = (void*)((qbyte*)mod_base + l->fileofs);
	size_t count = l->filelen / sizeof(*in);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "CODBSP_LoadBrushSides: funny lump size\n");
		return false;
	}
	prv->brushsides = in;	//used elsewhere in the loader
	prv->num_brushsides = count;
	return true;
}
static qboolean CODBSP_LoadBrushes (model_t *mod, qbyte *mod_base, lump_t *l)
{
	codbspinfo_t	*prv = (codbspinfo_t*)mod->meshinfo;
	const struct
	{
		unsigned short sides;
		unsigned short material;
	} *in = (void*)((qbyte*)mod_base + l->fileofs);
	const struct codbsp_brushside_s *inside = prv->brushsides;
	q2cbrush_t *out;
	q2cbrushside_t *outside;
	mplane_t *aplane;
	size_t j, i, count = l->filelen / sizeof(*in);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "CODBSP_LoadBrushes: funny lump size\n");
		return false;
	}
	prv->brushes = out = plugfuncs->GMalloc(&mod->memgroup, count*sizeof(*out));
	aplane = plugfuncs->GMalloc(&mod->memgroup, count*6*sizeof(*aplane));
	//read the data
	for (i = 0, j = 0; i < count; i++, in++)
	{
		unsigned int mat = LittleShort(in->material);
		out[i].numsides = (unsigned short)LittleShort(in->sides);
		out[i].contents = prv->surfaces[mat].c.value;	//is this right? seems to kinda work? feels wrong though.
		j += out[i].numsides;
	}
	//fix up the planes...
	outside = plugfuncs->GMalloc(&mod->memgroup, j*sizeof(*outside));
	prv->num_brushes = count;
	for (i = 0, j = 0; i < count; i++, out++)
	{
		out->brushside = outside;
		for (j = 0; j < out->numsides; j++, inside++, outside++)
		{
			unsigned int mat = LittleLong(inside->material_idx);
			if (j < 6)
			{
				aplane->dist = LittleFloat(inside->dist);
				if (j&1)
				{	//stored nx px ny py nz pz
					aplane->normal[j>>1] = 1;
					out->absmaxs[j>>1] = aplane->dist;
				}
				else
				{
					aplane->normal[j>>1] = -1;
					out->absmins[j>>1] = aplane->dist;
					aplane->dist *= -1;
				}
				outside->plane = aplane++;
			}
			else
				outside->plane = prv->planes + LittleLong(inside->plane);
			outside->surface = &prv->surfaces[mat];
		}
	}
	return true;
}
/*static qboolean CODBSP_LoadLeafBrushes (model_t *mod, qbyte *mod_base, lump_t *l)
{	//we don't really care about this, as we're using our BIH stuff for collisions instead.
//	codbspinfo_t *prv = (codbspinfo_t*)mod->meshinfo;
	struct
	{
		int a;
	} *in = (void*)((qbyte*)mod_base + l->fileofs);
	size_t i, count = l->filelen / sizeof(*in);
	int highest=0;
	int lowest=0;
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "CODBSP_LoadLeafBrushes: funny lump size\n");
		return false;
	}
	for (i = 0; i < count; i++, in++)
	{
		if (lowest > in->a)
			lowest = in->a;
		if (highest < in->a)
			highest = in->a;
//		Con_Printf("%i: %i\n", (int)i, in->a);
	}
	Con_Printf("leaf brushes: %i - %i\n", lowest, highest);
	return true;
}*/

static qboolean CODBSP_LoadPatchVertexes (model_t *mod, qbyte *mod_base, lump_t *l)
{	//for collision
	codbspinfo_t *prv = (codbspinfo_t*)mod->meshinfo;
	const vec3_t *in = (void*)((qbyte*)mod_base + l->fileofs);
	vecV_t *out;
	size_t i, count = l->filelen / sizeof(*in);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "CODBSP_LoadPatchVertexes: funny lump size\n");
		return false;
	}
	prv->patchvertexes = out = plugfuncs->GMalloc(&mod->memgroup, count*sizeof(*out));
	prv->numpatchvertexes = count;
	for (i = 0; i < count; i++)
	{
		out[i][0] = LittleFloat(in[i][0]);
		out[i][1] = LittleFloat(in[i][1]);
		out[i][2] = LittleFloat(in[i][2]);
	}
	return true;
}
static qboolean CODBSP_LoadPatchIndexes (model_t *mod, qbyte *mod_base, lump_t *l)
{	//for collision
	codbspinfo_t *prv = (codbspinfo_t*)mod->meshinfo;
	unsigned short *in = (void*)((qbyte*)mod_base + l->fileofs);
	index_t *out;
	size_t i, count = l->filelen / sizeof(*in);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "CODBSP_LoadPatchIndexes: funny lump size\n");
		return false;
	}
	prv->patchindexes = out = plugfuncs->GMalloc(&mod->memgroup, count*sizeof(*out));
	prv->numpatchindexes = count;
	for (i = 0; i < count; i++)
	{
		*out++ = (unsigned short)LittleShort(*in++);
	}
	return true;
}
static qboolean CODBSP_LoadPatchCollision (model_t *mod, qbyte *mod_base, lump_t *l)
{
	codbspinfo_t *prv = (codbspinfo_t*)mod->meshinfo;
	struct codpatch_s *in = (void*)((qbyte*)mod_base + l->fileofs);
	size_t i, count = l->filelen / sizeof(*in);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "CODBSP_LoadPatchesCollision: funny lump size\n");
		return false;
	}
	prv->patches = in;
	prv->numpatches = count;
	for (i = 0; i < count; i++, in++)
	{
		if (in->mode==0)
			;//Con_Printf("p%i: %s ?+%i*%-4i v%i+%i %i\n", (int)i, mod->textures[in->mat]->name, in->mode0.w, in->mode0.h, in->mode0.firstvert,in->mode0.w*in->mode0.h, in->mode0.unknown);
		else if (in->mode==1)
			;//Con_Printf("s%i: %s %4i+%-4i v%i+%i\n", (int)i, mod->textures[in->mat]->name, in->mode1.firstidx,in->mode1.numidx, in->mode1.firstvert,in->mode1.numverts);
		else
		{
			Con_Printf("?%i: %s %i ?!?!?!?!?\n", (int)i, mod->textures[in->mat]->name, in->mode);
			return false;	//nope.
		}
	}

	return true;
}
static qboolean CODBSP_LoadLeafPatches (model_t *mod, qbyte *mod_base, lump_t *l)
{	//for collision
	codbspinfo_t *prv = (codbspinfo_t*)mod->meshinfo;
	unsigned int *in = (void*)((qbyte*)mod_base + l->fileofs);
	size_t count = l->filelen / sizeof(*in);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "CODBSP_LoadLeafPatches: funny lump size\n");
		return false;
	}
	prv->leafpatches = in;
	prv->numleafpatches = count;
	return true;
}

/*
static qboolean CODBSP_LoadAABBs (model_t *mod, qbyte *mod_base, lump_t *l)
{
//	codbspinfo_t *prv = (codbspinfo_t*)mod->meshinfo;
	struct
	{
		int a;	//surfaceindex (submodel0 only, so stops short of the full range implied by the lump's count)
		int b;	//numsurfaces
		int c;	//some sort of offset?
	} *in = (void*)((qbyte*)mod_base + l->fileofs);
	size_t i, count = l->filelen / sizeof(*in);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "CODBSP_LoadAABBs: funny lump size\n");
		return false;
	}
	for (i = 0; i < count; i++, in++)
	{
//		Con_Printf("%i: %i+%i %i\n", (int)i, in->a, in->b, in->c);
	}
	return true;
}
static qboolean CODBSP_LoadCells (model_t *mod, qbyte *mod_base, lump_t *l)
{
//	codbspinfo_t *prv = (codbspinfo_t*)mod->meshinfo;
	struct
	{
		vec3_t mins;
		vec3_t maxs;

		int aabtree; //lump 16ish? CODLUMP_AABBTREES
		int firstportal;	//lump 18? CODLUMP_PORTALS
		int numportals;
		int firstcullgroupindex;	//lump 10? CODLUMP_CULLGROUPINDEXES
		int numcullgroupindexes;
		int firstoccluderindex;
		int numoccluders;
	} *in = (void*)((qbyte*)mod_base + l->fileofs);
	size_t i, count = l->filelen / sizeof(*in);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "CODBSP_LoadCells: funny lump size\n");
		return false;
	}
	for (i = 0; i < count; i++, in++)
	{
		Con_Printf("%i: [%f %f %f] [%f %f %f] %i %i+%i %i+%i %i+%i\n", (int)i,
			in->mins[0], in->mins[1], in->mins[2],
			in->maxs[0], in->maxs[1], in->maxs[2],
			in->aabtree, in->firstportal, in->numportals, in->firstcullgroupindex,
			in->numcullgroupindexes, in->firstoccluderindex, in->numoccluders);
	}
	return true;
}*/

static qboolean CODBSP_LoadLeafs (model_t *mod, qbyte *mod_base, lump_t *l)
{
	codbspinfo_t	*prv = (codbspinfo_t*)mod->meshinfo;
	struct codinlinemodel_s{
		int cluster;	//-1 for invalid
		int area;		//-1 for invalid
		unsigned int firstleafsurfs;
		unsigned int numsurfaces;
		unsigned int firstleafbrushes;
		unsigned int numbrushes;

		int cell;		//-1 for invalid
		unsigned int firstlightindex;
		unsigned int numlightindexes;
	} *in = (void*)((qbyte*)mod_base + l->fileofs);
	size_t count = l->filelen / sizeof(*in);
	size_t i;
	if (l->filelen % sizeof(*in) || count < 1)
	{
		Con_Printf (CON_ERROR "CODBSP_LoadLeafs: funny lump size\n");
		return false;
	}
	prv->leaf = plugfuncs->GMalloc(&mod->memgroup, count*sizeof(*prv->leaf));
	prv->numleafs = count;
	for (i = 0; i < count; i++, in++)
	{
		prv->leaf[i].cluster = LittleLong(in->cluster);
		prv->leaf[i].area = LittleLong(in->area);

		prv->leaf[i].firstlightindex = LittleLong(in->firstlightindex);
		prv->leaf[i].numlightindexes = LittleLong(in->numlightindexes);
	}
	return true;
}
static qboolean CODBSP_LoadNodes (model_t *mod, qbyte *mod_base, lump_t *l)
{
	codbspinfo_t	*prv = (codbspinfo_t*)mod->meshinfo;
	struct codinlinemodel_s{
		unsigned int plane;
		int child[2]; //negative for leaf
		ivec3_t mins;
		ivec3_t maxs;
	} *in = (void*)((qbyte*)mod_base + l->fileofs);
	size_t count = l->filelen / sizeof(*in);
	size_t i;
	if (l->filelen % sizeof(*in) || count < 1)
	{
		Con_Printf (CON_ERROR "CODBSP_LoadNodes: funny lump size\n");
		return false;
	}
	prv->nodes = plugfuncs->GMalloc(&mod->memgroup, count*sizeof(*prv->nodes));
	prv->numnodes = count;
	for (i = 0; i < count; i++, in++)
	{
		prv->nodes[i].plane = &prv->planes[LittleLong(in->plane)];
		prv->nodes[i].childnum[0] = LittleLong(in->child[0]);
		prv->nodes[i].childnum[1] = LittleLong(in->child[1]);

		prv->nodes[i].mins[0] = LittleLong(in->mins[0]);
		prv->nodes[i].mins[1] = LittleLong(in->mins[1]);
		prv->nodes[i].mins[2] = LittleLong(in->mins[2]);
		prv->nodes[i].maxs[0] = LittleLong(in->maxs[0]);
		prv->nodes[i].maxs[1] = LittleLong(in->maxs[1]);
		prv->nodes[i].maxs[2] = LittleLong(in->maxs[2]);
	}
	return true;
}
static qboolean CODBSP_LoadVisibility (model_t *mod, qbyte *mod_base, lump_t *l)
{
	codbspinfo_t	*prv = (codbspinfo_t*)mod->meshinfo;
	qbyte *in = (void*)((qbyte*)mod_base + l->fileofs);
	if (!l->filelen)
	{	//unvised.
		mod->numclusters = 0;
		mod->pvsbytes = 0;
		prv->pvsdata = NULL;
		return true;
	}
	if (l->filelen < 8)
	{
		Con_Printf (CON_ERROR "CODBSP_LoadVisibility: funny lump size\n");
		return false;
	}
	mod->numclusters = LittleLong(((int*)in)[0]);
	mod->pvsbytes = LittleLong(((int*)in)[1]);
	if (l->filelen != 8 + mod->numclusters*mod->pvsbytes)
	{
		Con_Printf (CON_ERROR "CODBSP_LoadVisibility: funny lump size\n");
		return false;
	}

	prv->pvsdata = plugfuncs->GMalloc(&mod->memgroup, l->filelen-8);
	memcpy(prv->pvsdata, in+8, mod->numclusters*mod->pvsbytes);
	return true;
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
static void CODBSP_BuildBIH (model_t *mod, size_t firstbrush, size_t numbrushes, size_t firstleafpatch, size_t numleafpatches)
{
	codbspinfo_t	*prv = (codbspinfo_t*)mod->meshinfo;
	size_t numtriangles = 0;
	size_t numquads = 0;

	index_t *silly;
	struct bihleaf_s *bihleaf, *l;
	size_t i, j, lp;
	qbyte *patches = memset(alloca((prv->numpatches+7)>>3), 0, (prv->numpatches+7)>>3);
	for (i = firstleafpatch; i < numleafpatches; i++)
	{	//de-dupe them... *sigh*
		lp = LittleLong(prv->leafpatches[i]);
		if (lp < prv->numpatches)
			patches[lp>>3] |= 1u<<(lp&7);
	}
	for (i = 0; i < prv->numpatches; i++)
	{
		if (patches[i>>3] & (1u<<(i&7)))
		{
			if (prv->patches[i].mode)
				numtriangles+=(unsigned short)LittleShort(prv->patches[i].mode1.numidx)/3;
			else
				numquads+=(unsigned int)((unsigned short)LittleShort(prv->patches[i].mode0.w)-1) * ((unsigned short)LittleShort(prv->patches[i].mode0.h)-1);
		}
	}

	bihleaf = l = plugfuncs->Malloc(sizeof(*bihleaf)*(numbrushes+numtriangles+numquads*2));

	silly = plugfuncs->GMalloc(&mod->memgroup, sizeof(*silly)*6*numquads);

	//now we have enough storage, spit them out providing bounds info.
	for (i = 0; i < prv->numpatches; i++)
	{
		if (patches[i>>3] & (1u<<(i&7)))
		{
			if (prv->patches[i].mode)
			{
				unsigned int numidx = (unsigned short)LittleShort(prv->patches[i].mode1.numidx);
				for (j = 0; j < numidx; j+=3)
				{
					vec_t *v1,*v2,*v3;
					l->type = BIH_TRIANGLE;
					l->data.contents = prv->surfaces[LittleLong(prv->patches[i].mat)].c.value;
					l->data.tri.xyz = prv->patchvertexes + (unsigned int)LittleLong(prv->patches[i].mode1.firstvert);
					l->data.tri.indexes = prv->patchindexes + (unsigned int)LittleLong(prv->patches[i].mode1.firstidx) + j;

					v1 = l->data.tri.xyz[l->data.tri.indexes[0]];
					v2 = l->data.tri.xyz[l->data.tri.indexes[1]];
					v3 = l->data.tri.xyz[l->data.tri.indexes[2]];

					VectorCopy(v1, l->mins);
					VectorCopy(v1, l->maxs);
					AddPointToBounds(v2, l->mins, l->maxs);
					AddPointToBounds(v3, l->mins, l->maxs);
					l++;
				}
			}
			else
			{
				unsigned int w = (unsigned short)LittleShort(prv->patches[i].mode0.w);
				unsigned int h = (unsigned short)LittleShort(prv->patches[i].mode0.h);
				unsigned int x, y;
				for (y = 0; y < h-1; y++)
				for (x = 0; x < w-1; x++)
				{
					const vec_t *v1,*v2,*v3;

					silly[0] = x+y*w;
					silly[1] = silly[0]+1;
					silly[2] = silly[0]+w;
					silly[3] = silly[1];
					silly[4] = silly[1]+w;
					silly[5] = silly[2];

					l->type = BIH_TRIANGLE;
					l->data.contents = FTECONTENTS_SOLID; //prv->surfaces[LittleLong(prv->patches[i].mat)].c.value;
					l->data.tri.xyz = prv->patchvertexes + (unsigned int)LittleLong(prv->patches[i].mode0.firstvert);
					l->data.tri.indexes = silly;

					v1 = l->data.tri.xyz[l->data.tri.indexes[0]];
					v2 = l->data.tri.xyz[l->data.tri.indexes[1]];
					v3 = l->data.tri.xyz[l->data.tri.indexes[2]];

					VectorCopy(v1, l->mins);
					VectorCopy(v1, l->maxs);
					AddPointToBounds(v2, l->mins, l->maxs);
					AddPointToBounds(v3, l->mins, l->maxs);
					l++;
					silly+=3;


					l->type = BIH_TRIANGLE;
					l->data.contents = FTECONTENTS_SOLID; //prv->surfaces[LittleLong(prv->patches[i].mat)].c.value;
					l->data.tri.xyz = prv->patchvertexes + (unsigned int)LittleLong(prv->patches[i].mode0.firstvert);
					l->data.tri.indexes = silly;

					v1 = l->data.tri.xyz[l->data.tri.indexes[0]];
					v2 = l->data.tri.xyz[l->data.tri.indexes[1]];
					v3 = l->data.tri.xyz[l->data.tri.indexes[2]];

					VectorCopy(v1, l->mins);
					VectorCopy(v1, l->maxs);
					AddPointToBounds(v2, l->mins, l->maxs);
					AddPointToBounds(v3, l->mins, l->maxs);
					l++;
					silly+=3;
				}
			}
		}
	}

	//now we have enough storage, spit them out providing bounds info.
	for (i = 0; i < numbrushes; i++)
	{
		q2cbrush_t *b = &prv->brushes[firstbrush+i];
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

static qboolean CODBSP_LoadInlineModels (model_t *wmod, qbyte *mod_base, lump_t *l)
{
//	codbspinfo_t	*prv = (codbspinfo_t*)wmod->meshinfo;
	struct codinlinemodel_s{
		vec3_t mins;
		vec3_t maxs;
		unsigned int firstsurf;
		unsigned int numsurfs;
		unsigned int firstleafpatch;	//seems to match lump 23,
		unsigned int numleafpatches;
		unsigned int firstbrush;
		unsigned int numbrushes;
	} *in = (void*)((qbyte*)mod_base + l->fileofs);
	size_t count = l->filelen / sizeof(*in);
	size_t i, j;
	if (l->filelen % sizeof(*in) || count < 1)
	{
		Con_Printf (CON_ERROR "CODBSP_LoadInlineModels: funny lump size\n");
		return false;
	}

	for (i = 0; i < count; i++, in++)
	{
		char name[MAX_QPATH];
		model_t *mod;
		if (i)
		{	//submodels
			Q_snprintfz (name, sizeof(name), "*%u:%s", (unsigned int)i, wmod->publicname);
			mod = modfuncs->BeginSubmodelLoad(name);
			*mod = *wmod;
			mod->archive = NULL;
			mod->entities_raw = NULL;
			mod->submodelof = wmod;
			Q_strlcpy(mod->publicname, name, sizeof(mod->publicname));
			Q_snprintfz (mod->name, sizeof(mod->name), "*%u:%s", (unsigned int)i, wmod->name);
			memset(&mod->memgroup, 0, sizeof(mod->memgroup));
		}
		else	//handle the world model here too
			mod = wmod;

		mod->hulls[0].firstclipnode = i?-1:0;
		for (j = 1; j < countof(mod->hulls); j++)
			mod->hulls[j].firstclipnode = -1;	//no nodes,
		mod->nodes = mod->rootnode = NULL;
		mod->leafs = NULL;

		mod->nummodelsurfaces = LittleLong(in->numsurfs);
		mod->firstmodelsurface = LittleLong(in->firstsurf);

		VectorCopy(in->mins, mod->mins);
		VectorCopy(in->maxs, mod->maxs);
		CODBSP_BuildBIH(mod, LittleLong(in->firstbrush), LittleLong(in->numbrushes), LittleLong(in->firstleafpatch), LittleLong(in->numleafpatches));

		memset(&mod->batches, 0, sizeof(mod->batches));
		mod->vbos = NULL;

#ifdef HAVE_CLIENT
//		mod->radius = RadiusFromBounds (mod->mins, mod->maxs);

//		if (qrenderer != QR_NONE)
		{
			builddata_t *bd = plugfuncs->Malloc(sizeof(*bd));
			bd->buildfunc = CODBSP_BuildSurfMesh;
			bd->paintlightmaps = false;	//q3like with prebaked lightmaps.
			threadfuncs->AddWork(WG_MAIN, CODBSP_GenerateMaterials, mod, bd, 0, 0);
		}
#endif

		if (mod != wmod)
			modfuncs->EndSubmodelLoad(mod, MLS_LOADED);
	}
	return true;
}

static qboolean QDECL Mod_LoadCodBSP(struct model_s *mod, void *buffer, size_t fsize)
{
	codbspinfo_t	*prv;
	qboolean okay = true;
	int i;
	int ver = LittleLong(((int*)buffer)[1]);
	lump_t lumps[max((int)COD1LUMP_COUNT, (int)COD2LUMP_COUNT)];

	mod->fromgame = fg_new;
#ifdef HAVE_SERVER
	mod->funcs.FatPVS				= CODBSP_FatPVS;
	mod->funcs.EdictInFatPVS		= CODBSP_EdictInFatPVS;
	mod->funcs.FindTouchedLeafs		= CODBSP_FindTouchedLeafs;
#endif
#ifdef HAVE_CLIENT
	mod->funcs.LightPointValues		= CODBSP_LightPointValues;
//	mod->funcs.StainNode			= CODBSP_StainNode;
//	mod->funcs.MarkLights			= CODBSP_MarkLights;
//	mod->funcs.GenerateShadowMesh	= CODBSP_GenerateShadowMesh;
#endif
	mod->funcs.ClusterPVS			= CODBSP_ClusterPVS;
//	mod->funcs.ClusterPHS			= CODBSP_ClusterPHS;
	mod->funcs.ClusterForPoint		= CODBSP_ClusterForPoint;
//	mod->funcs.SetAreaPortalState	= CODBSP_SetAreaPortalState;
//	mod->funcs.AreasConnected		= CODBSP_AreasConnected;
//	mod->funcs.LoadAreaPortalBlob	= CODBSP_LoadAreaPortalBlob;
//	mod->funcs.SaveAreaPortalBlob	= CODBSP_SaveAreaPortalBlob;
	mod->funcs.PrepareFrame			= CODBSP_PrepareFrame;
	mod->funcs.InfoForPoint			= CODBSP_InfoForPoint;

	if (ver == COD1BSP_VERSION)
	{
		memcpy(lumps, (char*)buffer+8, sizeof(*lumps)*COD1LUMP_COUNT);
		for (i = 0; i < COD1LUMP_COUNT; i++)
		{
			int ffs = lumps[i].filelen;	//ffs
			lumps[i].filelen = LittleLong(lumps[i].fileofs);
			lumps[i].fileofs = LittleLong(ffs);
			if (lumps[i].filelen && lumps[i].fileofs+(size_t)lumps[i].filelen > fsize)
			{
				Con_Printf(CON_ERROR"Truncated BSP file\n");
				return false;
			}
		}

		mod->meshinfo = prv = plugfuncs->GMalloc(&mod->memgroup, sizeof(codbspinfo_t));
		prv->codbspver = ver;

		//basic trisoup info
		okay = okay && CODBSP_LoadShaders(mod, buffer, &lumps[COD1LUMP_MATERIALS]);
		okay = okay && COD1BSP_LoadLightmap(mod, buffer, &lumps[COD1LUMP_LIGHTMAPS]);
		okay = okay && COD1BSP_LoadSoupVertices(mod, buffer, &lumps[COD1LUMP_SOUPVERTS]);
		okay = okay && CODBSP_LoadSoupIndexes(mod, buffer, &lumps[COD1LUMP_SOUPINDEXES]);
		okay = okay && CODBSP_LoadSoups(mod, buffer, &lumps[COD1LUMP_SOUPS]);

		//gamecode needs to know what's around
		okay = okay && CODBSP_LoadLights(mod, buffer, &lumps[COD1LUMP_LIGHTS]);
		okay = okay && CODBSP_LoadLightIndexes(mod, buffer, &lumps[COD1LUMP_LIGHTINDEXES]);
		okay = okay && CODBSP_LoadEntities(mod, buffer, &lumps[COD1LUMP_ENTITIES]);

		//basic collision
		okay = okay && CODBSP_LoadPlanes(mod, buffer, &lumps[COD1LUMP_PLANES]);
		okay = okay && CODBSP_LoadBrushSides(mod, buffer, &lumps[COD1LUMP_BRUSHSIDES]);
		okay = okay && CODBSP_LoadBrushes(mod, buffer, &lumps[COD1LUMP_BRUSHES]);
		//okay = okay && CODBSP_LoadLeafBrushes(mod, buffer, &lumps[COD1LUMP_LEAFBRUSHES]);

		//patch collision...
		okay = okay && CODBSP_LoadPatchVertexes(mod, buffer, &lumps[COD1LUMP_COLLISIONVERTS]);
		okay = okay && CODBSP_LoadPatchIndexes(mod, buffer, &lumps[COD1LUMP_COLLISIONINDEXES]);
		okay = okay && CODBSP_LoadPatchCollision(mod, buffer, &lumps[COD1LUMP_PATCHCOLLISION]);
		okay = okay && CODBSP_LoadLeafPatches(mod, buffer, &lumps[COD1LUMP_LEAFPATCHES]);

		//seems like cod is a portal engine?
		//okay = okay && CODBSP_LoadAABBs(mod, buffer, &lumps[COD1LUMP_AABBTREES]);
		//okay = okay && CODBSP_LoadCells(mod, buffer, &lumps[COD1LUMP_CELLS]);

		//but we still have pvs with its clusters+areas that are node based (also required to determine which 'cell' we're inside).
		okay = okay && CODBSP_LoadVisibility(mod, buffer, &lumps[COD1LUMP_VISIBILITY]);
		okay = okay && CODBSP_LoadLeafs(mod, buffer, &lumps[COD1LUMP_LEAFS]);
		okay = okay && CODBSP_LoadNodes(mod, buffer, &lumps[COD1LUMP_NODES]);

		okay = okay && CODBSP_LoadInlineModels(mod, buffer, &lumps[COD1LUMP_MODELS]);
	}
	else if (ver == COD2BSP_VERSION)
	{
		memcpy(lumps, (char*)buffer+8, sizeof(*lumps)*COD2LUMP_COUNT);
		for (i = 0; i < COD2LUMP_COUNT; i++)
		{
			int ffs = lumps[i].filelen;	//ffs
			lumps[i].filelen = LittleLong(lumps[i].fileofs);
			lumps[i].fileofs = LittleLong(ffs);
			if (lumps[i].filelen && lumps[i].fileofs+(size_t)lumps[i].filelen > fsize)
			{
				Con_Printf(CON_ERROR"Truncated BSP file\n");
				return false;
			}
		}

		mod->meshinfo = prv = plugfuncs->GMalloc(&mod->memgroup, sizeof(codbspinfo_t));
		prv->codbspver = ver;

		//basic trisoup info
		okay = okay && CODBSP_LoadShaders(mod, buffer, &lumps[COD2LUMP_MATERIALS]);
		okay = okay && COD2BSP_LoadLightmap(mod, buffer, &lumps[COD2LUMP_LIGHTMAPS]);
		okay = okay && COD2BSP_LoadSoupVertices(mod, buffer, &lumps[COD2LUMP_SOUPVERTS]);
		okay = okay && CODBSP_LoadSoupIndexes(mod, buffer, &lumps[COD2LUMP_SOUPINDEXES]);
		okay = okay && CODBSP_LoadSoups(mod, buffer, &lumps[COD2LUMP_SOUPS]);

		//gamecode needs to know what's around
		okay = okay && CODBSP_LoadEntities(mod, buffer, &lumps[COD2LUMP_ENTITIES]);

		//basic collision
		okay = okay && CODBSP_LoadPlanes(mod, buffer, &lumps[COD2LUMP_PLANES]);
		okay = okay && CODBSP_LoadBrushSides(mod, buffer, &lumps[COD2LUMP_BRUSHSIDES]);
		okay = okay && CODBSP_LoadBrushes(mod, buffer, &lumps[COD2LUMP_BRUSHES]);
		//okay = okay && CODBSP_LoadLeafBrushes(mod, buffer, &lumps[COD2LUMP_LEAFBRUSHES]);

		//patch collision...
//		okay = okay && CODBSP_LoadPatchVertexes(mod, buffer, &lumps[COD1LUMP_COLLISIONVERTS]);
//		okay = okay && CODBSP_LoadPatchIndexes(mod, buffer, &lumps[COD1LUMP_COLLISIONINDEXES]);
//		okay = okay && CODBSP_LoadPatchCollision(mod, buffer, &lumps[COD2LUMP_PATCHCOLLISION]);
//		okay = okay && CODBSP_LoadLeafPatches(mod, buffer, &lumps[COD2LUMP_LEAFPATCHES]);

		//seems like cod is a portal engine?
		//okay = okay && CODBSP_LoadAABBs(mod, buffer, &lumps[COD2LUMP_AABBTREES]);
		//okay = okay && CODBSP_LoadCells(mod, buffer, &lumps[COD2LUMP_CELLS]);

		//but we still have pvs with its clusters+areas that are node based (also required to determine which 'cell' we're inside).
		okay = okay && CODBSP_LoadVisibility(mod, buffer, &lumps[COD2LUMP_VISIBILITY]);
		okay = okay && CODBSP_LoadLeafs(mod, buffer, &lumps[COD2LUMP_LEAFS]);
		okay = okay && CODBSP_LoadNodes(mod, buffer, &lumps[COD2LUMP_NODES]);

		okay = okay && CODBSP_LoadInlineModels(mod, buffer, &lumps[COD2LUMP_MODELS]);
	}
	else
	{
		Con_Printf(CON_ERROR"Bad COD Version...\n");	//should have already been checked, so this ain't possible.
		okay = false;
	}
	return okay;
}

qboolean CODBSP_Init(void)
{
	filefuncs = plugfuncs->GetEngineInterface(plugfsfuncs_name, sizeof(*filefuncs));
	modfuncs = plugfuncs->GetEngineInterface(plugmodfuncs_name, sizeof(*modfuncs));
	threadfuncs = plugfuncs->GetEngineInterface(plugthreadfuncs_name, sizeof(*threadfuncs));
	if (modfuncs && modfuncs->version != MODPLUGFUNCS_VERSION)
		modfuncs = NULL;

	if (modfuncs && filefuncs && threadfuncs)
	{
		modfuncs->RegisterModelFormatMagic("CoD(1) Maps", "IBSP\x3b\0\0\0",8, Mod_LoadCodBSP);
		modfuncs->RegisterModelFormatMagic("CoD2 Maps", "IBSP\x04\0\0\0",8, Mod_LoadCodBSP);
		return true;
	}
	return false;
}