#include "quakedef.h"
#ifndef SERVERONLY
#include "glquake.h"
#endif
#include "com_mesh.h"
#include "com_bih.h"

#define MAX_Q3MAP_INDICES 0x8000000	//just a sanity limit
#define	MAX_Q3MAP_VERTEXES	0x800000	//just a sanity limit
//#define MAX_CM_PATCH_VERTS		(4096)
//#define MAX_CM_FACES			(MAX_Q2MAP_FACES)
#ifdef FTE_TARGET_WEB
#define MAX_CM_PATCHES			(0x1000)		//fixme
#else
#define MAX_CM_PATCHES			(0x10000)		//fixme
#endif
//#define MAX_CM_LEAFFACES		(MAX_Q2MAP_LEAFFACES)
#define	MAX_CM_AREAS			MAX_Q2MAP_AREAS

//#define Q3SURF_NODAMAGE		0x00000001
//#define Q3SURF_SLICK			0x00000002
//#define Q3SURF_SKY			0x00000004
//#define Q3SURF_LADDER			0x00000008
//#define Q3SURF_NOIMPACT		0x00000010
//#define Q3SURF_NOMARKS		0x00000020
//#define Q3SURF_FLESH			0x00000040
#define	Q3SURF_NODRAW			0x00000080	// don't generate a drawsurface at all
//#define Q3SURF_HINT			0x00000100
#define	Q3SURF_SKIP				0x00000200	// completely ignore, allowing non-closed brushes
//#define Q3SURF_NOLIGHTMAP		0x00000400
//#define Q3SURF_POINTLIGHT		0x00000800
//#define Q3SURF_METALSTEPS		0x00001000
//#define Q3SURF_NOSTEPS		0x00002000
#define	Q3SURF_NONSOLID			0x00004000	// don't collide against curves with this set
//#define Q3SURF_LIGHTFILTER	0x00008000
//#define Q3SURF_ALPHASHADOW	0x00010000
//#define Q3SURF_NODLIGHT		0x00020000
//#define Q3SURF_DUST			0x00040000
cvar_t q3bsp_surf_meshcollision_flag = CVARD("q3bsp_surf_meshcollision_flag", "0x80000000", "The surfaceparm flag(s) that enables q3bsp trisoup collision");
cvar_t q3bsp_surf_meshcollision_force = CVARD("q3bsp_surf_meshcollision_force", "0", "Force mesh-based collisions on all q3bsp trisoup surfaces.");
cvar_t q3bsp_mergeq3lightmaps = CVARD("q3bsp_mergelightmaps", "1", "Specifies whether to merge lightmaps into atlases in order to boost performance. Unfortunately this breaks tcgen on lightmap passes - if you care, set this to 0.");
cvar_t q3bsp_ignorestyles = CVARD("q3bsp_ignorestyles", "0", "Ignores multiple lightstyles in Raven's q3bsp variant(and derivatives) for better batch/rendering performance.");
cvar_t q3bsp_bihtraces = CVARFD("_q3bsp_bihtraces", /*FIXME: generate BIH leafs more carefully*/"0", CVAR_RENDERERLATCH, "Uses runtime-generated bih collision culling for faster traces.");

#if Q3SURF_NODRAW != TI_NODRAW
#error "nodraw isn't constant"
#endif

extern cvar_t r_shadow_bumpscale_basetexture;

//these are in model.c (or gl_model.c)
qboolean Mod_LoadVertexes (model_t *loadmodel, qbyte *mod_base, lump_t *l);
qboolean Mod_LoadVertexNormals (model_t *loadmodel, bspx_header_t *bspx, qbyte *mod_base, lump_t *l);
qboolean Mod_LoadEdges (model_t *loadmodel, qbyte *mod_base, lump_t *l, subbsp_t lm);
qboolean Mod_LoadMarksurfaces (model_t *loadmodel, qbyte *mod_base, lump_t *l, subbsp_t lm);
qboolean Mod_LoadSurfedges (model_t *loadmodel, qbyte *mod_base, lump_t *l);
void Mod_LoadEntities (model_t *loadmodel, qbyte *mod_base, lump_t *l);

extern void BuildLightMapGammaTable (float g, float c);

#if defined(Q2BSPS) || defined(Q3BSPS)
static qboolean CM_NativeTrace(model_t *model, int forcehullnum, const framestate_t *framestate, const vec3_t axis[3], const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, qboolean capsule, unsigned int contents, trace_t *trace);
static unsigned int CM_NativeContents(struct model_s *model, int hulloverride, const framestate_t *framestate, const vec3_t axis[3], const vec3_t point, const vec3_t mins, const vec3_t maxs);
static unsigned int Q2BSP_PointContents(model_t *mod, const vec3_t axis[3], const vec3_t p);
static int CM_PointCluster (model_t *mod, const vec3_t p, int *area);
static void CM_InfoForPoint (struct model_s *mod, vec3_t pos, int *area, int *cluster, unsigned int *contentbits);
struct cminfo_s;


void CM_Init(void);

static qboolean	VARGS CM_AreasConnected (struct model_s *mod, unsigned int area1, unsigned int area2);
static size_t	CM_WriteAreaBits (struct model_s *mod, qbyte *buffer, size_t buffersize, int area, qboolean merge);
static qbyte	*CM_ClusterPVS (struct model_s *mod, int cluster, pvsbuffer_t *buffer, pvsmerge_t merge);
static qbyte	*CM_ClusterPHS (struct model_s *mod, int cluster, pvsbuffer_t *buffer);

//for gamecode to control portals/areas
static void	CM_SetAreaPortalState (model_t *mod, unsigned int portalnum, unsigned int area1, unsigned int area2, qboolean open);

//for saved games to write the raw state.
static size_t	CM_SaveAreaPortalBlob (model_t *mod, void **data);
static size_t	CM_LoadAreaPortalBlob (model_t *mod, void *ptr, size_t ptrsize);

#ifdef HAVE_SERVER
static unsigned int Q23BSP_FatPVS(model_t *mod, const vec3_t org, pvsbuffer_t *buffer, qboolean merge);
static qboolean Q23BSP_EdictInFatPVS(model_t *mod, const struct pvscache_s *ent, const qbyte *pvs, const int *areas);
static void Q23BSP_FindTouchedLeafs(model_t *mod, struct pvscache_s *ent, const float *mins, const float *maxs);
static qboolean	CM_HeadnodeVisible (struct model_s *mod, int nodenum, const qbyte *visbits);
#endif

#ifdef HAVE_CLIENT
static void CM_PrepareFrame(model_t *mod, refdef_t *refdef, int area, int viewclusters[2], pvsbuffer_t *vis, qbyte **entvis_out, qbyte **surfvis_out);
extern void Q2BSP_GenerateShadowMesh(model_t *mod, dlight_t *dl, const qbyte *lightvis, qbyte *litvis, void(*callback)(msurface_t*));
extern void Q3BSP_GenerateShadowMesh(model_t *mod, dlight_t *dl, const qbyte *lightvis, qbyte *litvis, void(*callback)(msurface_t*));
#endif

#endif

float RadiusFromBounds (const vec3_t mins, const vec3_t maxs)
{
	int		i;
	vec3_t	corner;

	for (i=0 ; i<3 ; i++)
	{
		corner[i] = fabs(mins[i]) > fabs(maxs[i]) ? fabs(mins[i]) : fabs(maxs[i]);
	}

	return Length (corner);
}

void CalcSurfaceExtents (model_t *mod, msurface_t *s)
{
	float	mins[2], maxs[2], val;
	int		i,j, e;
	mvertex_t	*v;
	mtexinfo_t	*tex;
	int		bmins[2], bmaxs[2];
	int idx;

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -999999;

	tex = s->texinfo;

	for (i=0 ; i<s->numedges ; i++)
	{
		e = mod->surfedges[s->firstedge+i];
		idx = e < 0;
		if (idx)
			e = -e;
		if (e < 0 || e >= mod->numedges)
			v = &mod->vertexes[0];
		else
			v = &mod->vertexes[mod->edges[e].v[idx]];

		for (j=0 ; j<2 ; j++)
		{
			//doubles should replicate win32/x87 compiler 80-bit precision better. We have to hope that win64 compilers use the same precision.
			val = DotProduct_Double(v->position, tex->vecs[j]) + tex->vecs[j][3];
			if (val < mins[j])
				mins[j] = val;
			if (val > maxs[j])
				maxs[j] = val;
		}
	}

	for (i=0 ; i<2 ; i++)
	{
		bmins[i] = floor(mins[i]/(1<<s->lmshift));
		bmaxs[i] = ceil(maxs[i]/(1<<s->lmshift));

		s->texturemins[i] = bmins[i] << s->lmshift;
		s->extents[i] = (bmaxs[i] - bmins[i]);

//		if ( !(tex->flags & TEX_SPECIAL) && s->extents[i] > 17 )	//vanilla used 16(+1), glquake used 17(+1). FTE uses 255(+1), but we omit lightmapping instead of crashing if its larger than our limit, so we omit the check here. different engines use different limits here, many of them make no sense.
//			Con_Printf ("Bad surface extents (texture %s, more than %i lightmap samples)\n", s->texinfo->texture->name, s->extents[i]);

		s->extents[i] <<= s->lmshift;
	}
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


void Mod_SortShaders(model_t *mod)
{
	//surely this isn't still needed?
	texture_t *textemp;
	int i, j;

	//sort loadmodel->textures
	for (i = 0; i < mod->numtextures; i++)
	{
		for (j = i+1; j < mod->numtextures; j++)
		{
			if ((mod->textures[i]->shader && mod->textures[j]->shader) && (mod->textures[j]->shader->sort < mod->textures[i]->shader->sort))
			{
				textemp = mod->textures[j];
				mod->textures[j] = mod->textures[i];
				mod->textures[i] = textemp;
			}
		}
	}
}



#if defined(Q2BSPS) || defined(Q3BSPS)

#ifdef IMAGEFMT_PCX
qbyte *ReadPCXPalette(qbyte *buf, int len, qbyte *out);
#endif

#ifdef SERVERONLY
#define Host_Error SV_Error
#endif

extern qbyte *mod_base;

#define capsuledist(dist,plane,mins,maxs)					\
		case shape_iscapsule:								\
			dist = DotProduct(trace_up, plane->normal);		\
			dist = dist*(trace_capsulesize[(dist<0)?1:2]) - trace_capsulesize[0];	\
			dist = plane->dist - dist;						\
			break;

#ifdef Q2BSPS
#ifdef HAVE_CLIENT
static unsigned char q2_palette[256*3];
#endif
#endif




typedef struct {
	char		shader[64];
	int			brushNum;
	int			visibleSide;	// the brush side that ray tests need to clip against (-1 == none)
} dfog_t;

#ifdef Q2BSPS
typedef struct
{
	int		numareaportals;
	int		firstareaportal;
} q2carea_t;
#endif
#ifdef Q3BSPS
typedef struct
{
	int			numareaportals[MAX_CM_AREAS];
} q3carea_t;
#endif
typedef struct
{
	int		floodnum;			// if two areas have equal floodnums, they are connected
	int		floodvalid;			// flags the area as having been visited (sequence numbers matching prv->floodvalid)
} careaflood_t;

typedef struct
{
	int		facetype;

	int		numverts;
	int		firstvert;

	int		shadernum;
	union
	{
		struct
		{
			unsigned short cp[2];
			unsigned short fixedres[2];
		} patch;
		struct
		{
			int firstindex;
			int numindicies;
		} soup;
	};
} q3cface_t;

typedef struct cmodel_s
{
	vec3_t		mins, maxs;
	vec3_t		origin;		// for sounds or lights
	mnode_t		*headnode;
	mleaf_t		*headleaf;
	int		numsurfaces;
	int		firstsurface;

	int firstbrush;	//q3 submodels are considered small enough that you will never need to walk any sort of tree.
	int num_brushes;//the brushes are checked instead.

	//these things are generated at load time.
	int firstpatch;
	int num_patches;
	int firstcmesh;
	int num_cmeshes;
} cmodel_t;

/*used to trace*/
static int			checkcount;

typedef struct cminfo_s
{
	int				numbrushsides;
	q2cbrushside_t *brushsides;

	q2mapsurface_t	*surfaces;

	int				numleafbrushes;
	q2cbrush_t		**leafbrushes;

	int				numcmodels;
	cmodel_t		*cmodels;

	int				numbrushes;
	q2cbrush_t		*brushes;

	int				numvisibility;
	q2dvis_t		*q2vis;
	q3dvis_t		*q3pvs;
	q3dvis_t		*q3phs;
	qbyte			*phscalced;

	int				numareas;
	int				floodvalid;
	careaflood_t	areaflood[MAX_CM_AREAS];
#ifdef Q3BSPS
	//q3's areas are simple bidirectional area1/area2 pairs. refcounted (so two areas can have two doors/openings)
	q3carea_t		q3areas[MAX_CM_AREAS];
#endif
#ifdef Q2BSPS
	//q2's areas have a list of portals that open into other areas.
	q2carea_t	*q2areas;	//indexes into q2areaportals for flooding
	size_t			numq2areaportals;
	q2dareaportal_t	*q2areaportals;

	//and this is the state that is actually changed. booleans.
	qbyte			q2portalopen[MAX_Q2MAP_AREAPORTALS];	//memset will work if it's a qbyte, really it should be a qboolean
#endif


	//list of mesh surfaces within the leaf
	q3cmesh_t	cmeshes[MAX_CM_PATCHES];
	int			numcmeshes;
	int			*leafcmeshes;
	int			numleafcmeshes;
	int			maxleafcmeshes;

	//FIXME: remove the below
	//(deprecated) patch collisions
	q3cpatch_t	patches[MAX_CM_PATCHES];
	int			numpatches;
	int			*leafpatches;
	int			numleafpatches;
	int			maxleafpatches;
	//FIXME: remove the above

	qboolean			mapisq3;



#ifdef Q3BSPS
	//this is for loading stuff. it used to be globals, but we have threads now. and multiple q3bsps at the same time is a problem.
	int			numvertexes;
	vecV_t		*verts;	//3points
	vec2_t		*vertstmexcoords;
	vec2_t		*vertlstmexcoords[MAXRLIGHTMAPS];
	vec4_t		*colors4f_array[MAXRLIGHTMAPS];
	vec3_t		*normals_array;
	//vec3_t		*map_svector_array;
	//vec3_t		*map_tvector_array;

	index_t *surfindexes;
	//int	map_numsurfindexes;

	q3cface_t	*faces;
	int			numfaces;
#endif

#ifdef HAVE_CLIENT
	int			oldclusters[2];
	qbyte		*oldvis;
#endif

//	struct bihnode_s *bihnodes;
} cminfo_t;

static q2mapsurface_t	nullsurface;

cvar_t		map_noareas			= CVAR("map_noareas", "0");	//1 for lack of mod support.
cvar_t		map_noCurves		= CVARF("map_noCurves", "0", CVAR_CHEAT);
cvar_t		map_autoopenportals	= CVARD("map_autoopenportals", "0", "When set to 1, force-opens all area portals. Normally these start closed and are opened by doors when they move, but this requires the gamecode to signal this.");	//1 for lack of mod support.
cvar_t		r_subdivisions		= CVAR("r_subdivisions", "2");

static int		CM_NumInlineModels (model_t *model);
static cmodel_t	*CM_InlineModel (model_t *model, char *name);
static void	CM_InitBoxHull (void);
static void CM_FinalizeBrush(q2cbrush_t *brush);
static void	FloodAreaConnections (cminfo_t	*prv);

qboolean BoundsIntersect (const vec3_t mins1, const vec3_t maxs1, const vec3_t mins2, const vec3_t maxs2)
{
	return (mins1[0] <= maxs2[0] && mins1[1] <= maxs2[1] && mins1[2] <= maxs2[2] &&
		 maxs1[0] >= mins2[0] && maxs1[1] >= mins2[1] && maxs1[2] >= mins2[2]);
}

#ifdef Q3BSPS

static int	PlaneTypeForNormal ( vec3_t normal )
{
	vec_t	ax, ay, az;

// NOTE: should these have an epsilon around 1.0?
	if ( normal[0] >= 1.0)
		return PLANE_X;
	if ( normal[1] >= 1.0 )
		return PLANE_Y;
	if ( normal[2] >= 1.0 )
		return PLANE_Z;

	ax = fabs( normal[0] );
	ay = fabs( normal[1] );
	az = fabs( normal[2] );

	if ( ax >= ay && ax >= az )
		return PLANE_ANYX;
	if ( ay >= ax && ay >= az )
		return PLANE_ANYY;
	return PLANE_ANYZ;
}

void CategorizePlane ( mplane_t *plane )
{
	int i;

	plane->signbits = 0;
	plane->type = PLANE_ANYZ;
	for (i = 0; i < 3; i++)
	{
		if (plane->normal[i] < 0)
			plane->signbits |= 1<<i;
		if (plane->normal[i] == 1.0f)
			plane->type = i;
	}
	plane->type = PlaneTypeForNormal(plane->normal);
}

static void PlaneFromPoints ( vec3_t verts[3], mplane_t *plane )
{
	vec3_t	v1, v2;

	VectorSubtract( verts[1], verts[0], v1 );
	VectorSubtract( verts[2], verts[0], v2 );
	CrossProduct( v2, v1, plane->normal );
	VectorNormalize( plane->normal );
	plane->dist = DotProduct( verts[0], plane->normal );
}

/*
===============
Patch_FlatnessTest
===============
*/
static int Patch_FlatnessTest( float maxflat2, const float *point0, const float *point1, const float *point2 )
{
	float d;
	int ft0, ft1;
	vec3_t t, n;
	vec3_t v1, v2, v3;

	VectorSubtract( point2, point0, n );
	if( !VectorNormalize( n ) )
		return 0;

	VectorSubtract( point1, point0, t );
	d = -DotProduct( t, n );
	VectorMA( t, d, n, t );
	if( DotProduct( t, t ) < maxflat2 )
		return 0;

	VectorAvg( point1, point0, v1 );
	VectorAvg( point2, point1, v2 );
	VectorAvg( v1, v2, v3 );

	ft0 = Patch_FlatnessTest( maxflat2, point0, v1, v3 );
	ft1 = Patch_FlatnessTest( maxflat2, v3, v2, point2 );

	return 1 + (int)( floor( max( ft0, ft1 ) ) + 0.5f );
}

/*
===============
Patch_GetFlatness
===============
*/
static void Patch_GetFlatness( float maxflat, const float *points, int comp, const unsigned short *patch_cp, int *flat )
{
	int i, p, u, v;
	float maxflat2 = maxflat * maxflat;

	flat[0] = flat[1] = 0;
	for( v = 0; v < patch_cp[1] - 1; v += 2 )
	{
		for( u = 0; u < patch_cp[0] - 1; u += 2 )
		{
			p = v * patch_cp[0] + u;

			i = Patch_FlatnessTest( maxflat2, &points[p*comp], &points[( p+1 )*comp], &points[( p+2 )*comp] );
			flat[0] = max( flat[0], i );
			i = Patch_FlatnessTest( maxflat2, &points[( p+patch_cp[0] )*comp], &points[( p+patch_cp[0]+1 )*comp], &points[( p+patch_cp[0]+2 )*comp] );
			flat[0] = max( flat[0], i );
			i = Patch_FlatnessTest( maxflat2, &points[( p+2*patch_cp[0] )*comp], &points[( p+2*patch_cp[0]+1 )*comp], &points[( p+2*patch_cp[0]+2 )*comp] );
			flat[0] = max( flat[0], i );

			i = Patch_FlatnessTest( maxflat2, &points[p*comp], &points[( p+patch_cp[0] )*comp], &points[( p+2*patch_cp[0] )*comp] );
			flat[1] = max( flat[1], i );
			i = Patch_FlatnessTest( maxflat2, &points[( p+1 )*comp], &points[( p+patch_cp[0]+1 )*comp], &points[( p+2*patch_cp[0]+1 )*comp] );
			flat[1] = max( flat[1], i );
			i = Patch_FlatnessTest( maxflat2, &points[( p+2 )*comp], &points[( p+patch_cp[0]+2 )*comp], &points[( p+2*patch_cp[0]+2 )*comp] );
			flat[1] = max( flat[1], i );
		}
	}
}

/*
===============
Patch_Evaluate_QuadricBezier
===============
*/
static void Patch_Evaluate_QuadricBezier( float t, const vec_t *point0, const vec_t *point1, const vec_t *point2, vec_t *out, int comp )
{
	int i;
	vec_t qt = t * t;
	vec_t dt = 2.0f * t, tt, tt2;

	tt = 1.0f - dt + qt;
	tt2 = dt - 2.0f * qt;

	for( i = 0; i < comp; i++ )
		out[i] = point0[i] * tt + point1[i] * tt2 + point2[i] * qt;
}

/*
===============
Patch_Evaluate
===============
*/
#define Patch_Evaluate(p,numcp,tess,dest, comp) Patch_EvaluateStride(p,comp,numcp,tess,dest,comp,comp)
static void Patch_EvaluateStride(const vec_t *p, int pstride, const unsigned short *numcp, const int *tess, vec_t *dest, int deststride, int comp)
{
	int num_patches[2], num_tess[2];
	int index[3], dstpitch, i, u, v, x, y;
	float s, t, step[2];
	vec_t *tvec, *tvec2;
	const vec_t *pv[3][3];
	vec4_t v1, v2, v3;

	if (!tess[0] || !tess[1])
	{	//not really a patch
		for( u = 0; u < numcp[1]*numcp[0]; u++, dest += deststride, p += pstride)
			for( i = 0; i < comp; i++ )
				dest[i] = p[i];
		return;
	}

	num_patches[0] = numcp[0] / 2;
	num_patches[1] = numcp[1] / 2;
	dstpitch = ( num_patches[0] * tess[0] + 1 ) * deststride;

	step[0] = 1.0f / (float)tess[0];
	step[1] = 1.0f / (float)tess[1];

	for( v = 0; v < num_patches[1]; v++ )
	{
		// last patch has one more row
		if( v < num_patches[1] - 1 )
			num_tess[1] = tess[1];
		else
			num_tess[1] = tess[1] + 1;

		for( u = 0; u < num_patches[0]; u++ )
		{
			// last patch has one more column
			if( u < num_patches[0] - 1 )
				num_tess[0] = tess[0];
			else
				num_tess[0] = tess[0] + 1;

			index[0] = ( v * numcp[0] + u ) * 2;
			index[1] = index[0] + numcp[0];
			index[2] = index[1] + numcp[0];

			// current 3x3 patch control points
			for( i = 0; i < 3; i++ )
			{
				pv[i][0] = &p[( index[0]+i ) * pstride];
				pv[i][1] = &p[( index[1]+i ) * pstride];
				pv[i][2] = &p[( index[2]+i ) * pstride];
			}

			tvec = dest + v * tess[1] * dstpitch + u * tess[0] * deststride;
			for( y = 0, t = 0.0f; y < num_tess[1]; y++, t += step[1], tvec += dstpitch )
			{
				Patch_Evaluate_QuadricBezier( t, pv[0][0], pv[0][1], pv[0][2], v1, comp );
				Patch_Evaluate_QuadricBezier( t, pv[1][0], pv[1][1], pv[1][2], v2, comp );
				Patch_Evaluate_QuadricBezier( t, pv[2][0], pv[2][1], pv[2][2], v3, comp );

				for( x = 0, tvec2 = tvec, s = 0.0f; x < num_tess[0]; x++, s += step[0], tvec2 += deststride )
					Patch_Evaluate_QuadricBezier( s, v1, v2, v3, tvec2, comp );
			}
		}
	}
}
#ifdef TERRAIN
#include "gl_terrain.h"
patchtessvert_t *PatchInfo_Evaluate(const qcpatchvert_t *cp, const unsigned short patch_cp[2], const short subdiv[2], unsigned short *size)
{
	int step[2], flat[2];
	float subdivlevel;
	unsigned int numverts;
	patchtessvert_t *out;
	int i;

	if (subdiv[0]>=0 && subdiv[1]>=0)
	{	//fixed
		step[0] = subdiv[0];
		step[1] = subdiv[1];
	}
	else
	{
		// find the degree of subdivision in the u and v directions
		subdivlevel = bound(1, r_subdivisions.ival, 15);
		Patch_GetFlatness ( subdivlevel, cp->v, sizeof(*cp)/sizeof(vec_t), patch_cp, flat );

		step[0] = 1 << flat[0];
		step[1] = 1 << flat[1];
	}
	if (!step[0] || !step[1])
	{
		size[0] = patch_cp[0];
		size[1] = patch_cp[1];
	}
	else
	{
		size[0] = ( patch_cp[0] >> 1 ) * step[0] + 1;
		size[1] = ( patch_cp[1] >> 1 ) * step[1] + 1;
	}
	if( size[0] <= 0 || size[1] <= 0 )
		return NULL;

	numverts = (unsigned int)size[0] * size[1];

// fill in

	out = BZ_Malloc(sizeof(*out) * numverts);
	for (i = 0; i < numverts*sizeof(*out)/sizeof(vec_t); i++)
		((vec_t *)out)[i] = -1;
	Patch_EvaluateStride ( cp->v, sizeof(*cp)/sizeof(vec_t), patch_cp, step, out->v, sizeof(*out)/sizeof(vec_t), countof(cp->v));
	Patch_EvaluateStride ( cp->rgba, sizeof(*cp)/sizeof(vec_t), patch_cp, step, out->rgba, sizeof(*out)/sizeof(vec_t), countof(cp->rgba));
	Patch_EvaluateStride ( cp->tc, sizeof(*cp)/sizeof(vec_t), patch_cp, step, out->tc, sizeof(*out)/sizeof(vec_t), countof(cp->tc));

	return out;
}
unsigned int PatchInfo_EvaluateIndexes(const unsigned short *size, index_t *out_indexes)
{
	int i, u, v, p;
// compute new indexes avoiding adding invalid triangles
	unsigned int numindexes = 0;
	index_t	*indexes = out_indexes;
	for (v = 0, i = 0; v < size[1]-1; v++)
	{
		for (u = 0; u < size[0]-1; u++, i += 6)
		{
			indexes[0] = p = v * size[0] + u;
			indexes[1] = p + size[0];
			indexes[2] = p + 1;

//			if ( !VectorEquals(mesh->xyz_array[indexes[0]], mesh->xyz_array[indexes[1]]) &&
//				!VectorEquals(mesh->xyz_array[indexes[0]], mesh->xyz_array[indexes[2]]) &&
//				!VectorEquals(mesh->xyz_array[indexes[1]], mesh->xyz_array[indexes[2]]) )
			{
				indexes += 3;
				numindexes += 3;
			}

			indexes[0] = p + 1;
			indexes[1] = p + size[0];
			indexes[2] = p + size[0] + 1;

//			if ( !VectorEquals(mesh->xyz_array[indexes[0]], mesh->xyz_array[indexes[1]]) &&
//				!VectorEquals(mesh->xyz_array[indexes[0]], mesh->xyz_array[indexes[2]]) &&
//				!VectorEquals(mesh->xyz_array[indexes[1]], mesh->xyz_array[indexes[2]]) )
			{
				indexes += 3;
				numindexes += 3;
			}
		}
	}

	return numindexes;
}
#endif


#define	PLANE_NORMAL_EPSILON	0.00001
#define	PLANE_DIST_EPSILON	0.01
static qboolean ComparePlanes( const vec3_t p1normal, vec_t p1dist, const vec3_t p2normal, vec_t p2dist )
{
	if( fabs( p1normal[0] - p2normal[0] ) < PLANE_NORMAL_EPSILON
	    && fabs( p1normal[1] - p2normal[1] ) < PLANE_NORMAL_EPSILON
	    && fabs( p1normal[2] - p2normal[2] ) < PLANE_NORMAL_EPSILON
	    && fabs( p1dist - p2dist ) < PLANE_DIST_EPSILON )
		return true;

	return false;
}

static void SnapVector( vec3_t normal )
{
	int i;

	for( i = 0; i < 3; i++ )
	{
		if( fabs( normal[i] - 1 ) < PLANE_NORMAL_EPSILON )
		{
			VectorClear( normal );
			normal[i] = 1;
			break;
		}
		if( fabs( normal[i] + 1 ) < PLANE_NORMAL_EPSILON )
		{
			VectorClear( normal );
			normal[i] = -1;
			break;
		}
	}
}

#define Q_rint( x )   ( ( x ) < 0 ? ( (int)( ( x )-0.5f ) ) : ( (int)( ( x )+0.5f ) ) )
static void SnapPlane( vec3_t normal, vec_t *dist )
{
	SnapVector( normal );

	if( fabs( *dist - Q_rint( *dist ) ) < PLANE_DIST_EPSILON )
	{
		*dist = Q_rint( *dist );
	}
}

/*
===============================================================================

					PATCH LOADING

===============================================================================
*/

#define MAX_FACET_PLANES 32
#define cm_subdivlevel	15

/*
* CM_CreateFacetFromPoints
*/
static int CM_CreateFacetFromPoints(q2cbrush_t *facet, vec3_t *verts, int numverts, q2mapsurface_t *shaderref, mplane_t *brushplanes )
{
	int i, j;
	int axis, dir;
	vec3_t normal;
	float d, dist;
	mplane_t mainplane;
	vec3_t vec, vec2;
	int numbrushplanes;

	// set default values for brush
	facet->numsides = 0;
	facet->brushside = NULL;
	facet->contents = shaderref->c.value;

	// calculate plane for this triangle
	PlaneFromPoints( verts, &mainplane );
	if( ComparePlanes( mainplane.normal, mainplane.dist, vec3_origin, 0 ) )
		return 0;

	// test a quad case
	if( numverts > 3 )
	{
		d = DotProduct( verts[3], mainplane.normal ) - mainplane.dist;
		if( d < -0.1 || d > 0.1 )
			return 0;

		if( 0 )
		{
			vec3_t v[3];
			mplane_t plane;

			// try different combinations of planes
			for( i = 1; i < 4; i++ )
			{
				VectorCopy( verts[i], v[0] );
				VectorCopy( verts[( i+1 )%4], v[1] );
				VectorCopy( verts[( i+2 )%4], v[2] );
				PlaneFromPoints( v, &plane );

				if( fabs( DotProduct( mainplane.normal, plane.normal ) ) < 0.9 )
					return 0;
			}
		}
	}

	numbrushplanes = 0;

	// add front plane
	SnapPlane( mainplane.normal, &mainplane.dist );
	VectorCopy( mainplane.normal, brushplanes[numbrushplanes].normal );
	brushplanes[numbrushplanes].dist = mainplane.dist; numbrushplanes++;

	// calculate mins & maxs
	ClearBounds( facet->absmins, facet->absmaxs );
	for( i = 0; i < numverts; i++ )
		AddPointToBounds( verts[i], facet->absmins, facet->absmaxs );

	// add the axial planes
	for( axis = 0; axis < 3; axis++ )
	{
		for( dir = -1; dir <= 1; dir += 2 )
		{
			for( i = 0; i < numbrushplanes; i++ )
			{
				if( brushplanes[i].normal[axis] == dir )
					break;
			}

			if( i == numbrushplanes )
			{
				VectorClear( normal );
				normal[axis] = dir;
				if( dir == 1 )
					dist = facet->absmaxs[axis];
				else
					dist = -facet->absmins[axis];

				VectorCopy( normal, brushplanes[numbrushplanes].normal );
				brushplanes[numbrushplanes].dist = dist; numbrushplanes++;
			}
		}
	}

	// add the edge bevels
	for( i = 0; i < numverts; i++ )
	{
		j = ( i + 1 ) % numverts;
//		k = ( i + 2 ) % numverts;

		VectorSubtract( verts[i], verts[j], vec );
		if( VectorNormalize( vec ) < 0.5 )
			continue;

		SnapVector( vec );
		for( j = 0; j < 3; j++ )
		{
			if( vec[j] == 1 || vec[j] == -1 )
				break; // axial
		}
		if( j != 3 )
			continue; // only test non-axial edges

		// try the six possible slanted axials from this edge
		for( axis = 0; axis < 3; axis++ )
		{
			for( dir = -1; dir <= 1; dir += 2 )
			{
				// construct a plane
				VectorClear( vec2 );
				vec2[axis] = dir;
				CrossProduct( vec, vec2, normal );
				if( VectorNormalize( normal ) < 0.5 )
					continue;
				dist = DotProduct( verts[i], normal );

				for( j = 0; j < numbrushplanes; j++ )
				{
					// if this plane has already been used, skip it
					if( ComparePlanes( brushplanes[j].normal, brushplanes[j].dist, normal, dist ) )
						break;
				}
				if( j != numbrushplanes )
					continue;

				// if all other points are behind this plane, it is a proper edge bevel
				for( j = 0; j < numverts; j++ )
				{
					if( j != i )
					{
						d = DotProduct( verts[j], normal ) - dist;
						if( d > 0.1 )
							break; // point in front: this plane isn't part of the outer hull
					}
				}
				if( j != numverts )
					continue;

				// add this plane
				VectorCopy( normal, brushplanes[numbrushplanes].normal );
				brushplanes[numbrushplanes].dist = dist; numbrushplanes++;
				if( numbrushplanes == MAX_FACET_PLANES )
					break;
			}
		}
	}

	return ( facet->numsides = numbrushplanes );
}

/*
* CM_CreatePatch
*/
static void CM_CreatePatch(model_t *loadmodel, q3cpatch_t *patch, q2mapsurface_t *shaderref, const vec_t *verts, const unsigned short *patch_cp, const unsigned short *patch_subdiv)
{
	int step[2], size[2], flat[2];
	int i, j, k ,u, v;
	int numsides, totalsides;
	q2cbrush_t *facets, *facet;
	vecV_t *points;
	vec3_t tverts[4];
	qbyte *data;
	mplane_t *brushplanes;
	float subdivlevel;

	patch->surface = shaderref;

	if (patch_subdiv)
	{	//fixed
		step[0] = patch_subdiv[0];
		step[1] = patch_subdiv[1];
	}
	else
	{
		// find the degree of subdivision in the u and v directions
		subdivlevel = cm_subdivlevel;//r_subdivisions.value;
		if ( subdivlevel < 1 )
			subdivlevel = 1;
		Patch_GetFlatness( subdivlevel, verts, sizeof(vecV_t)/sizeof(vec_t), patch_cp, flat );

		step[0] = 1 << flat[0];
		step[1] = 1 << flat[1];
	}
	if (!step[0] || !step[1])
	{
		size[0] = patch_cp[0];
		size[1] = patch_cp[1];
	}
	else
	{
		size[0] = ( patch_cp[0] >> 1 ) * step[0] + 1;
		size[1] = ( patch_cp[1] >> 1 ) * step[1] + 1;
	}
	if( size[0] <= 0 || size[1] <= 0 )
		return;

	data = BZ_Malloc( size[0] * size[1] * sizeof( vecV_t ) +
		( size[0]-1 ) * ( size[1]-1 ) * 2 * ( sizeof( q2cbrush_t ) + 32 * sizeof( mplane_t ) ) );

	points = ( vecV_t * )data; data += size[0] * size[1] * sizeof( vecV_t );
	facets = ( q2cbrush_t * )data; data += ( size[0]-1 ) * ( size[1]-1 ) * 2 * sizeof( q2cbrush_t );
	brushplanes = ( mplane_t * )data; data += ( size[0]-1 ) * ( size[1]-1 ) * 2 * MAX_FACET_PLANES * sizeof( mplane_t );

	// fill in
	Patch_Evaluate(verts, patch_cp, step, points[0], sizeof(vecV_t)/sizeof(vec_t));

	totalsides = 0;
	patch->numfacets = 0;
	patch->facets = NULL;
	ClearBounds( patch->absmins, patch->absmaxs );

	// create a set of facets
	for( v = 0; v < size[1]-1; v++ )
	{
		for( u = 0; u < size[0]-1; u++ )
		{
			i = v * size[0] + u;
			VectorCopy( points[i], tverts[0] );
			VectorCopy( points[i + size[0]], tverts[1] );
			VectorCopy( points[i + size[0] + 1], tverts[2] );
			VectorCopy( points[i + 1], tverts[3] );

			for( i = 0; i < 4; i++ )
				AddPointToBounds( tverts[i], patch->absmins, patch->absmaxs );

			// try to create one facet from a quad
			numsides = CM_CreateFacetFromPoints( &facets[patch->numfacets], tverts, 4, shaderref, brushplanes + totalsides );
			if( !numsides )
			{	// create two facets from triangles
				VectorCopy( tverts[3], tverts[2] );
				numsides = CM_CreateFacetFromPoints( &facets[patch->numfacets], tverts, 3, shaderref, brushplanes + totalsides );
				if( numsides )
				{
					totalsides += numsides;
					patch->numfacets++;
				}

				VectorCopy( tverts[2], tverts[0] );
				VectorCopy( points[v *size[0] + u + size[0] + 1], tverts[2] );
				numsides = CM_CreateFacetFromPoints( &facets[patch->numfacets], tverts, 3, shaderref, brushplanes + totalsides );
			}

			if( numsides )
			{
				totalsides += numsides;
				patch->numfacets++;
			}
		}
	}

	if (patch->numfacets)
	{
		qbyte *data;

		data = ZG_Malloc(&loadmodel->memgroup, patch->numfacets * sizeof( q2cbrush_t ) + totalsides * ( sizeof( q2cbrushside_t ) + sizeof( mplane_t ) ));

		patch->facets = ( q2cbrush_t * )data; data += patch->numfacets * sizeof( q2cbrush_t );
		memcpy( patch->facets, facets, patch->numfacets * sizeof( q2cbrush_t ) );
		for( i = 0, k = 0, facet = patch->facets; i < patch->numfacets; i++, facet++ )
		{
			mplane_t *planes;
			q2cbrushside_t *s;

			facet->brushside = ( q2cbrushside_t * )data; data += facet->numsides * sizeof( q2cbrushside_t );
			planes = ( mplane_t * )data; data += facet->numsides * sizeof( mplane_t );

			for( j = 0, s = facet->brushside; j < facet->numsides; j++, s++ )
			{
				planes[j] = brushplanes[k++];

				s->plane = &planes[j];
				SnapPlane( s->plane->normal, &s->plane->dist );
				CategorizePlane( s->plane );
				s->surface = shaderref;
			}
		}

		for( i = 0; i < 3; i++ )
		{
			// spread the mins / maxs by a pixel
			patch->absmins[i] -= 1;
			patch->absmaxs[i] += 1;
		}
	}

	BZ_Free( points );
}

//======================================================

static qboolean CM_CreatePatchForFace (model_t *loadmodel, cminfo_t *prv, mleaf_t *leaf, int facenum, int *checkout)
{
	size_t u;
	q3cface_t *face;
	q2mapsurface_t *surf;
	q3cpatch_t *patch;
	q3cmesh_t *cmesh;

	face = &prv->faces[facenum];

	if (face->numverts <= 0)
		return true;
	if (face->shadernum < 0 || face->shadernum >= loadmodel->numtextures)
		return true;
	surf = &prv->surfaces[face->shadernum];
	if (!surf->c.value)	//surface has no contents value, so can't ever block anything.
		return true;

	switch(face->facetype)
	{
	case MST_TRIANGLE_SOUP:
		if (!face->soup.numindicies)
			return true;
		//only enable mesh collisions if its meant to be enabled.
		//we haven't parsed any shaders, so we depend upon the stuff that the bsp compiler left lying around.
		if (!(surf->c.flags & q3bsp_surf_meshcollision_flag.ival) && !q3bsp_surf_meshcollision_force.ival)
			return true;

		if (prv->numleafcmeshes >= prv->maxleafcmeshes)
		{
			prv->maxleafcmeshes *= 2;
			prv->maxleafcmeshes += 16;
			if (prv->numleafcmeshes > prv->maxleafcmeshes)
			{	//detect overflow
				Con_Printf (CON_ERROR "CM_CreateCMeshesForLeafs: map is insanely huge!\n");
				return false;
			}
			prv->leafcmeshes = realloc(prv->leafcmeshes, sizeof(*prv->leafcmeshes) * prv->maxleafcmeshes);
		}

		// the patch was already built
		if (checkout[facenum] != -1)
		{
			prv->leafcmeshes[prv->numleafcmeshes] = checkout[facenum];
			cmesh = &prv->cmeshes[checkout[facenum]];
		}
		else
		{
			if (prv->numcmeshes >= MAX_CM_PATCHES)
			{
				Con_Printf (CON_ERROR "CM_CreatePatchesForLeafs: map has too many patches\n");
				return false;
			}

			cmesh = &prv->cmeshes[prv->numcmeshes];
			prv->leafcmeshes[prv->numleafcmeshes] = prv->numcmeshes;
			checkout[facenum] = prv->numcmeshes++;

//gcc warns without this cast

			cmesh->surface = surf;
			cmesh->numverts = face->numverts;
			cmesh->numincidies = face->soup.numindicies;
			cmesh->xyz_array = ZG_Malloc(&loadmodel->memgroup, cmesh->numverts * sizeof(*cmesh->xyz_array) + cmesh->numincidies * sizeof(*cmesh->indicies));
			cmesh->indicies = (index_t*)(cmesh->xyz_array + cmesh->numverts);

			VectorCopy(prv->verts[face->firstvert+0], cmesh->xyz_array[0]);
			VectorCopy(cmesh->xyz_array[0], cmesh->absmaxs);
			VectorCopy(cmesh->xyz_array[0], cmesh->absmins);
			for (u = 1; u < cmesh->numverts; u++)
			{
				VectorCopy(prv->verts[face->firstvert+u], cmesh->xyz_array[u]);
				AddPointToBounds(cmesh->xyz_array[u], cmesh->absmins, cmesh->absmaxs);
			}
			for (u = 0; u < cmesh->numincidies; u++)
				cmesh->indicies[u] = prv->surfindexes[face->soup.firstindex+u];
		}
		leaf->contents |= surf->c.value;
		leaf->numleafcmeshes++;

		prv->numleafcmeshes++;

		break;
	case MST_PATCH:
	case MST_PATCH_FIXED:
		if (face->patch.cp[0] <= 0 || face->patch.cp[1] <= 0)
			return true;

		if ( !surf->c.value || (surf->c.flags & Q3SURF_NONSOLID) )
			return true;

		if (prv->numleafpatches >= prv->maxleafpatches)
		{
			prv->maxleafpatches *= 2;
			prv->maxleafpatches += 16;
			if (prv->numleafpatches > prv->maxleafpatches)
			{	//detect overflow
				Con_Printf (CON_ERROR "CM_CreatePatchesForLeafs: map is insanely huge!\n");
				return false;
			}
			prv->leafpatches = realloc(prv->leafpatches, sizeof(*prv->leafpatches) * prv->maxleafpatches);
		}

		// the patch was already built
		if (checkout[facenum] != -1)
		{
			prv->leafpatches[prv->numleafpatches] = checkout[facenum];
			patch = &prv->patches[checkout[facenum]];
		}
		else
		{
			if (prv->numpatches >= MAX_CM_PATCHES)
			{
				Con_Printf (CON_ERROR "CM_CreatePatchesForLeafs: map has too many patches\n");
				return false;
			}

			patch = &prv->patches[prv->numpatches];
			prv->leafpatches[prv->numleafpatches] = prv->numpatches;
			checkout[facenum] = prv->numpatches++;

//gcc warns without this cast
			CM_CreatePatch (loadmodel, patch, surf, (const vec_t *)(prv->verts + face->firstvert), face->patch.cp, (face->facetype==MST_PATCH_FIXED)?face->patch.fixedres:NULL );
		}
		leaf->contents |= patch->surface->c.value;
		leaf->numleafpatches++;

		prv->numleafpatches++;
		break;
	}
	return true;
}
static qboolean CM_CreatePatchesForLeaf (model_t *loadmodel, cminfo_t *prv, mleaf_t *leaf, int *checkout)
{
	int j, k;

	leaf->numleafpatches = 0;
	leaf->firstleafpatch = prv->numleafpatches;
	leaf->numleafcmeshes = 0;
	leaf->firstleafcmesh = prv->numleafcmeshes;

	if (leaf->cluster == -1)
		return true;

	for (j=0 ; j<leaf->nummarksurfaces ; j++)
	{
		k = leaf->firstmarksurface[j] - loadmodel->surfaces;
		if (k >= prv->numfaces)
		{
			Con_Printf (CON_ERROR "CM_CreatePatchesForLeafs: corrupt map\n");
			break;
		}
		if (!CM_CreatePatchForFace (loadmodel, prv, leaf, k, checkout))
			return false;
	}
	return true;
}

/*
=================
CM_CreatePatchesForLeafs
=================
*/
static qboolean CM_CreatePatchesForLeafs (model_t *loadmodel, cminfo_t *prv)
{
	int i, k;
	mleaf_t *leaf;
	int *checkout = alloca(sizeof(int)*prv->numfaces);

	if (map_noCurves.ival)
		return true;

	memset (checkout, -1, sizeof(int)*prv->numfaces);

	for (i = prv->numcmodels; i-- > 0; )
	{
		prv->cmodels[i].firstpatch = prv->numpatches;
		prv->cmodels[i].firstcmesh = prv->numcmeshes;
		if (i == 0)
		{	//worldmodel's leafs
			for (k = 0, leaf = loadmodel->leafs; k < loadmodel->numleafs; k++, leaf++)
				if (!CM_CreatePatchesForLeaf(loadmodel, prv, leaf, checkout))
					return false;
		}
		else
		{	//submodel uni-leaf thing.
			leaf = prv->cmodels[i].headleaf;
			if (leaf)
			{
				if (!CM_CreatePatchesForLeaf(loadmodel, prv, leaf, checkout))
					return false;
				for (k = 0; k < prv->cmodels[i].numsurfaces; k++)
					CM_CreatePatchForFace(loadmodel, prv, leaf, prv->cmodels[i].firstsurface+k, checkout);
			}
		}
		prv->cmodels[i].num_patches = prv->numpatches-prv->cmodels[i].firstpatch;
		prv->cmodels[i].num_cmeshes = prv->numcmeshes-prv->cmodels[i].firstcmesh;
	}
	return true;
}
#endif


/*
===============================================================================

					MAP LOADING

===============================================================================
*/

static void CMod_SetParent (mnode_t *node, mnode_t *parent)
{
	node->parent = parent;
	if (node->contents != -1)
		return;
	CMod_SetParent (node->children[0], node);
	CMod_SetParent (node->children[1], node);
}

#ifdef Q2BSPS
/*
=================
CMod_LoadSubmodels
=================
*/
static qboolean CModQ2_LoadSubmodels (model_t *loadmodel, qbyte *mod_base, lump_t *l)
{
	cminfo_t	*prv = (cminfo_t*)loadmodel->meshinfo;
	q2dmodel_t	*in;
	cmodel_t	*out;
	int			i, j, count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size\n");
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

	out = prv->cmodels = ZG_Malloc(&loadmodel->memgroup, count * sizeof(*prv->cmodels));
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
	}

	AddPointToBounds(prv->cmodels[0].mins, loadmodel->mins, loadmodel->maxs);
	AddPointToBounds(prv->cmodels[0].maxs, loadmodel->mins, loadmodel->maxs);

	return true;
}


/*
=================
CMod_LoadSurfaces
=================
*/
static qboolean CModQ2_LoadSurfaces (model_t *mod, qbyte *mod_base, lump_t *l)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	q2texinfo_t	*in;
	q2mapsurface_t	*out;
	int			i, count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size\n");
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
	out = prv->surfaces = ZG_Malloc(&mod->memgroup, count * sizeof(*prv->surfaces));

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		Q_strncpyz (out->c.name, in->texture, sizeof(out->c.name));
		Q_strncpyz (out->rname, in->texture, sizeof(out->rname));
		out->c.flags = LittleLong (in->flags);
		out->c.value = LittleLong (in->value);
	}

	return true;
}
#ifdef HAVE_CLIENT
static texture_t *Mod_LoadWall(model_t *loadmodel, char *mapname, char *texname, char *shadername, unsigned int imageflags)
{
	char name[MAX_QPATH];
	q2miptex_t replacementwal;
	texture_t *tex;
	q2miptex_t *wal;
	image_t *base;

	Q_snprintfz (name, sizeof(name), "textures/%s.wal", texname);
	wal = (void *)FS_LoadMallocFile (name, NULL);
	if (!wal)
	{
		wal = &replacementwal;
		memset(wal, 0, sizeof(*wal));
		Q_strncpyz(wal->name, texname, sizeof(wal->name));
		wal->width = 64;
		wal->height = 64;
	}
	else
	{
		wal->width = LittleLong(wal->width);
		wal->height = LittleLong(wal->height);
	}
	{
		int i;

		for (i = 0; i < MIPLEVELS; i++)
			wal->offsets[i] = LittleLong(wal->offsets[i]);
	}

	wal->flags = LittleLong(wal->flags);
	wal->contents = LittleLong(wal->contents);
	wal->value = LittleLong(wal->value);

	tex = ZG_Malloc(&loadmodel->memgroup, sizeof(texture_t));

	tex->vwidth = tex->srcwidth = wal->width;
	tex->vheight = tex->srcheight = wal->height;

	if (!tex->vwidth || !tex->vheight || wal == &replacementwal)
	{
		imageflags |= IF_LOADNOW;	//make sure the size is known BEFORE it returns.
		if (wal->offsets[0])
			base = R_LoadReplacementTexture(wal->name, "bmodels", imageflags, (qbyte *)wal+wal->offsets[0], wal->width, wal->height, TF_SOLID8);
		else
			base = R_LoadHiResTexture(wal->name, "bmodels", imageflags);
	}
	else
		base = NULL;

	if (wal == &replacementwal)
	{
		if (base)
		{
			if (base->status == TEX_LOADED||base->status==TEX_LOADING)
			{
				tex->vwidth = base->width;
				tex->vheight = base->height;
			}
			else
				Con_Printf("Unable to load textures/%s.wal\n", wal->name);
		}

	}
	else
	{
		qbyte *out;
		unsigned int size = 
		(wal->width>>0)*(wal->height>>0) +
		(wal->width>>1)*(wal->height>>1) +
		(wal->width>>2)*(wal->height>>2) +
		(wal->width>>3)*(wal->height>>3);

		tex->srcdata = out = BZ_Malloc(size);
		tex->srcfmt = TF_MIP4_8PAL24_T255;
		tex->palette = q2_palette;
		memcpy(out, (qbyte *)wal + wal->offsets[0], (wal->width>>0)*(wal->height>>0));
		out += (wal->width>>0)*(wal->height>>0);
		memcpy(out, (qbyte *)wal + wal->offsets[1], (wal->width>>1)*(wal->height>>1));
		out += (wal->width>>1)*(wal->height>>1);
		memcpy(out, (qbyte *)wal + wal->offsets[2], (wal->width>>2)*(wal->height>>2));
		out += (wal->width>>2)*(wal->height>>2);
		memcpy(out, (qbyte *)wal + wal->offsets[3], (wal->width>>3)*(wal->height>>3));
		out += (wal->width>>3)*(wal->height>>3);

		BZ_Free(wal);
	}

	return tex;
}

static qboolean CModQ2_LoadTexInfo (model_t *mod, qbyte *mod_base, lump_t *l, char *mapname)	//yes I know these load from the same place
{
	q2texinfo_t *in;
	mtexinfo_t *out;
	int 	i, j, count;
	char	*lwr;
	char	sname[MAX_QPATH];
	int texcount;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf ("MOD_LoadBmodel: funny lump size in %s\n", mod->name);
		return false;
	}
	count = l->filelen / sizeof(*in);
	out = ZG_Malloc(&mod->memgroup, count*sizeof(*out));

	mod->textures = ZG_Malloc(&mod->memgroup, sizeof(texture_t *)*count);
	texcount = 0;

	mod->texinfo = out;
	mod->numtexinfo = count;

	if (in[0].nexttexinfo != -1)
	{
		for (i = 1; i < count && in[i].nexttexinfo == in[0].nexttexinfo; i++)
			;
		if (i == count)
		{
			Con_Printf("WARNING: invalid texture animations in \"%s\"\n", mod->name);
			for (i = 0; i < count; i++) 
				in[i].nexttexinfo = -1;
		}
	}

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->flags = LittleLong (in->flags);

		for (j=0 ; j<4 ; j++)
			out->vecs[0][j] = LittleFloat (in->vecs[0][j]);
		for (j=0 ; j<4 ; j++)
			out->vecs[1][j] = LittleFloat (in->vecs[1][j]);
		out->vecscale[0] = 1.0/Length (out->vecs[0]);
		out->vecscale[1] = 1.0/Length (out->vecs[1]);

		if (out->flags & TI_SKY)
			Q_snprintfz(sname, sizeof(sname), "sky/%s", in->texture);
		else
			Q_snprintfz(sname, sizeof(sname), "%s", in->texture);
		if (out->flags & (TI_WARP))
			Q_strncatz(sname, "#WARP", sizeof(sname));
		if (out->flags & TI_FLOWING)
			Q_strncatz(sname, "#FLOW", sizeof(sname));
		if (out->flags & (TI_N64_SCROLL_X | TI_N64_SCROLL_Y | TI_N64_SCROLL_FLIP))
		{
			Q_snprintfz(sname+strlen(sname), sizeof(sname)-strlen(sname), "#FLOWV=%s%s,%s%s",
						(out->flags&TI_N64_SCROLL_FLIP)?"":"-",
						(out->flags&TI_N64_SCROLL_X)?"1.0":"0.0",
						(out->flags&TI_N64_SCROLL_FLIP)?"-":"",
						(out->flags&TI_N64_SCROLL_Y)?"1.0":"0.0"
						);
		}
		if (out->flags & TI_TRANS66)
			Q_strncatz(sname, "#ALPHA=0.66", sizeof(sname));
		else if (out->flags & TI_TRANS33)
			Q_strncatz(sname, "#ALPHA=0.33", sizeof(sname));
		else if (out->flags & (TI_KINGPIN_ALPHATEST|TI_Q2EX_ALPHATEST)) //kingpin...
			Q_strncatz(sname, "#MASK=0.666#MASKLT", sizeof(sname));
		else if (out->flags & (TI_WARP))
			Q_strncatz(sname, "#ALPHA=1", sizeof(sname));
		if (in->nexttexinfo != -1)	//used to ensure non-looping and looping don't conflict and get confused.
			Q_strncatz(sname, "#ANIMLOOP", sizeof(sname));

		//in q2, 'TEX_SPECIAL' is TI_LIGHT, and that conflicts.
		out->flags &= ~TI_LIGHT;	//TI_LIGHT makes the surface emissive. its for the rad tool, not useful to us, so its safe to just strip it to avoid confusion.
		if (out->flags & (TI_SKY))
			out->flags |= TEX_SPECIAL;

		//compact the textures.
		for (j=0; j < texcount; j++)
		{
			if (!strcmp(sname, mod->textures[j]->name))
			{
				out->texture = mod->textures[j];
				break;
			}
		}
		if (j == texcount)	//load a new one
		{
			for (lwr = in->texture; *lwr; lwr++)
			{
				if (*lwr >= 'A' && *lwr <= 'Z')
					*lwr = *lwr - 'A' + 'a';
			}
			out->texture = Mod_LoadWall (mod, mapname, in->texture, sname, (out->flags&TEX_SPECIAL)?0:IF_NOALPHA);
			if (!out->texture || !out->texture->srcwidth || !out->texture->srcheight)
			{
				out->texture = ZG_Malloc(&mod->memgroup, sizeof(texture_t) + 16*16+8*8+4*4+2*2);

				Con_Printf (CON_WARNING "Couldn't load \"%s.wal\"\n", in->texture);
				memcpy(out->texture, r_notexture_mip, sizeof(texture_t) + 16*16+8*8+4*4+2*2);
			}

			Q_strncpyz(out->texture->name, sname, sizeof(out->texture->name));

			mod->textures[texcount++] = out->texture;
		}

//		if (in->nexttexinfo != -1)
//		{
//			Con_DPrintf("FIXME: %s should animate to %s\n", in->texture, (in->nexttexinfo+(q2texinfo_t *)(mod_base + l->fileofs))->texture);
//		}
	}

	in = (void *)(mod_base + l->fileofs);
	out = mod->texinfo;
	for (i=0 ; i<count ; i++)
	{
		if (in[i].nexttexinfo >= 0 && in[i].nexttexinfo < count)
			out[i].texture->anim_next = out[in[i].nexttexinfo].texture;
	}
	for (i=0 ; i<count ; i++)
	{
		texture_t *tex;
		if (!out[i].texture->anim_next)
			continue;

		out[i].texture->anim_total = 1;
		for (tex = out[i].texture->anim_next ; tex && tex != out[i].texture && out[i].texture->anim_total < 100; tex=tex->anim_next)
			out[i].texture->anim_total++;
	}

	mod->numtextures = texcount;

	Mod_SortShaders(mod);
	return true;
}
#endif
/*
static void CalcSurfaceExtents (msurface_t *s)
{
	float	mins[2], maxs[2], val;
	int		i,j, e;
	mvertex_t	*v;
	mtexinfo_t	*tex;
	int		bmins[2], bmaxs[2];

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -99999;

	tex = s->texinfo;

	for (i=0 ; i<s->numedges ; i++)
	{
		e = loadmodel->surfedges[s->firstedge+i];
		if (e >= 0)
			v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
		else
			v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];

		for (j=0 ; j<2 ; j++)
		{
			val = v->position[0] * tex->vecs[j][0] +
				v->position[1] * tex->vecs[j][1] +
				v->position[2] * tex->vecs[j][2] +
				tex->vecs[j][3];
			if (val < mins[j])
				mins[j] = val;
			if (val > maxs[j])
				maxs[j] = val;
		}
	}

	for (i=0 ; i<2 ; i++)
	{
		bmins[i] = floor(mins[i]/16);
		bmaxs[i] = ceil(maxs[i]/16);

		s->texturemins[i] = bmins[i] * 16;
		s->extents[i] = (bmaxs[i] - bmins[i]) * 16;

//		if ( !(tex->flags & TEX_SPECIAL) && s->extents[i] > 512 )// 256 )
//			Sys_Error ("Bad surface extents");
	}
}*/

/*
=================
Mod_LoadFaces
=================
*/
#ifdef HAVE_CLIENT
static qboolean CModQ2_LoadFaces (model_t *mod, qbyte *mod_base, lump_t *l, lump_t *lightlump, qboolean lightofsisdouble, bspx_header_t *bspx, qboolean isbig)
{
	dsface_t		*ins = NULL;
	dlface_t		*inl = NULL;
	msurface_t 	*out;
	int			i, count, surfnum;
	int			planenum, side;
	int			ti;
	int			style;

	struct decoupled_lm_info_s *decoupledlm;
	size_t dcsize;
	unsigned int lofs;

	unsigned short lmshift, lmscale;
	char buf[64];
	lightmapoverrides_t overrides = {0};
	overrides.defaultshift = LMSHIFT_DEFAULT;

	if (!isbig && !(l->filelen % sizeof(*ins)))
		ins = (void *)(mod_base + l->fileofs);
	else if (isbig && !(l->filelen % sizeof(*inl)))
		inl = (void *)(mod_base + l->fileofs);
	else
	{
		Con_Printf ("MOD_LoadBmodel: funny lump size in %s\n",mod->name);
		return false;
	}
	count = l->filelen / (isbig?sizeof(*inl):sizeof(*ins));
	out = ZG_Malloc(&mod->memgroup, (count+6)*sizeof(*out));	//spare for skybox

	mod->surfaces = out;
	mod->numsurfaces = count;

	Mod_LoadLighting(mod, bspx, mod_base, lightlump, lightofsisdouble, &overrides, sb_none);
	if (overrides.offsets)
		lmshift = overrides.defaultshift;
	else
	{
		lmscale = atoi(Mod_ParseWorldspawnKey(mod, "lightmap_scale", buf, sizeof(buf)));
		if (!lmscale)
			lmshift = LMSHIFT_DEFAULT;
		else
		{
			for(lmshift = 0; lmscale > 1; lmshift++)
				lmscale >>= 1;
		}
	}

	decoupledlm = BSPX_FindLump(bspx, mod_base, "DECOUPLED_LM", &dcsize); //RGB packed data
	if (dcsize == count*sizeof(*decoupledlm))
		mod->facelmvecs = ZG_Malloc(&mod->memgroup, count * sizeof(*mod->facelmvecs));	//seems good.
	else
		decoupledlm	= NULL;	//wrong size somehow... discard it.

	for ( surfnum=0 ; surfnum<count ; surfnum++, out++)
	{
		if (isbig)
		{
			out->firstedge = LittleLong(inl->firstedge);
			out->numedges = (unsigned int)LittleLong(inl->numedges);
			planenum = (unsigned int)LittleLong(inl->planenum);
			side = (unsigned int)LittleLong(inl->side);
			ti = (unsigned int)LittleLong (inl->texinfo);
			lofs = LittleLong(inl->lightofs);
			for (i=0 ; i<4 ; i++)
			{
				style = inl->styles[i];
				if (style == 0xff)
					style = INVALID_LIGHTSTYLE;
				else if (mod->lightmaps.maxstyle < style)
					mod->lightmaps.maxstyle = style;
				out->styles[i] = style;
			}
			inl++;
		}
		else
		{
			out->firstedge = LittleLong(ins->firstedge);
			out->numedges = (unsigned short)LittleShort(ins->numedges);
			planenum = (unsigned short)LittleShort(ins->planenum);
			side = (unsigned short)LittleShort(ins->side);
			ti = (unsigned short)LittleShort (ins->texinfo);
			lofs = LittleLong(ins->lightofs);
			for (i=0 ; i<4 ; i++)
			{
				style = ins->styles[i];
				if (style == 0xff)
					style = INVALID_LIGHTSTYLE;
				else if (mod->lightmaps.maxstyle < style)
					mod->lightmaps.maxstyle = style;
				out->styles[i] = style;
			}
			ins++;
		}
		out->plane = mod->planes + planenum;
		out->flags = 0;
		if (side)
			out->flags |= SURF_PLANEBACK;
		if (ti < 0 || ti >= mod->numtexinfo)
		{
			Con_Printf (CON_ERROR "MOD_LoadBmodel: bad texinfo number\n");
			return false;
		}
		out->texinfo = mod->texinfo + ti;

		if (decoupledlm)
		{
			lofs = LittleLong(decoupledlm->lmoffset);
			out->texturemins[0] = out->texturemins[1] = 0; // should be handled by the now-per-surface vecs[][3] value.
			out->lmshift = 0;	//redundant.
			out->extents[0] = (unsigned short)LittleShort(decoupledlm->lmsize[0]) - 1;
			out->extents[1] = (unsigned short)LittleShort(decoupledlm->lmsize[1]) - 1;
			mod->facelmvecs[surfnum].lmvecs[0][0] = LittleFloat(decoupledlm->lmvecs[0][0]);
			mod->facelmvecs[surfnum].lmvecs[0][1] = LittleFloat(decoupledlm->lmvecs[0][1]);
			mod->facelmvecs[surfnum].lmvecs[0][2] = LittleFloat(decoupledlm->lmvecs[0][2]);
			mod->facelmvecs[surfnum].lmvecs[0][3] = LittleFloat(decoupledlm->lmvecs[0][3]) + 0.5f; //sigh
			mod->facelmvecs[surfnum].lmvecs[1][0] = LittleFloat(decoupledlm->lmvecs[1][0]);
			mod->facelmvecs[surfnum].lmvecs[1][1] = LittleFloat(decoupledlm->lmvecs[1][1]);
			mod->facelmvecs[surfnum].lmvecs[1][2] = LittleFloat(decoupledlm->lmvecs[1][2]);
			mod->facelmvecs[surfnum].lmvecs[1][3] = LittleFloat(decoupledlm->lmvecs[1][3]) + 0.5f; //sigh
			mod->facelmvecs[surfnum].lmvecscale[0] = 1.0f/Length(mod->facelmvecs[surfnum].lmvecs[0]);	//luxels->qu
			mod->facelmvecs[surfnum].lmvecscale[1] = 1.0f/Length(mod->facelmvecs[surfnum].lmvecs[1]);
			decoupledlm++;
		}
		else
		{
			if (overrides.shifts)
				out->lmshift = overrides.shifts[surfnum];
			else
				out->lmshift = lmshift;

			if (overrides.offsets)
				lofs = overrides.offsets[surfnum];

			CalcSurfaceExtents (mod, out);
			if (overrides.extents)
			{
				out->extents[0] = overrides.extents[surfnum*2+0];
				out->extents[1] = overrides.extents[surfnum*2+1];
			}
		}

	// lighting info
		if (overrides.styles16)
		{
			for (i=0 ; i<overrides.stylesperface ; i++)
			{
				style = overrides.styles16[surfnum*overrides.stylesperface+i];
				if (style == 0xffff)
					style = INVALID_LIGHTSTYLE;
				else if (mod->lightmaps.maxstyle < style)
					mod->lightmaps.maxstyle = style;
				out->styles[i] = style;
			}
		}
		else if (overrides.styles8)
		{
			for (i=0 ; i<overrides.stylesperface ; i++)
			{
				style = overrides.styles8[surfnum*overrides.stylesperface+i];
				if (style == 0xff)
					style = INVALID_LIGHTSTYLE;
				else if (mod->lightmaps.maxstyle < style)
					mod->lightmaps.maxstyle = style;
				out->styles[i] = style;
			}
		}
		for ( ; i<MAXCPULIGHTMAPS ; i++)
			out->styles[i] = INVALID_LIGHTSTYLE;
		if (lofs == ~0u)
			out->samples = NULL;
		else if (lightofsisdouble)
			out->samples = mod->lightdata + (lofs/2);
		else
			out->samples = mod->lightdata + lofs;

	// set the drawing flags


		if (out->texinfo->flags & TI_SKY)
			out->flags |= SURF_DRAWSKY|SURF_DRAWTILED;
		if (out->texinfo->flags & TI_WARP)
		{
			out->flags |= SURF_DRAWTURB;
			if (out->styles[0]==0&&out->styles[1]==0&&out->styles[2]==0&&out->styles[3]==0)
				out->flags |= SURF_DRAWTILED;	//normally you won't get the same lightmap 4 times over... assume uninitialised, and therefore unlit.
			else if (!strstr(out->texinfo->texture->name, "#LIT"))
				Q_strncatz(out->texinfo->texture->name, "#LIT", sizeof(out->texinfo->texture->name));
		}
		if (out->flags & SURF_DRAWTILED)
		{	//shouldn't have been lit...
			for (i=0 ; i<2 ; i++)
			{
				out->extents[i] = 16384;
				out->texturemins[i] = -8192;
			}
		}
	}

	return true;
}
#endif

/*
=================
CMod_LoadNodes

=================
*/
static qboolean CModQ2_LoadNodes (model_t *mod, qbyte *mod_base, lump_t *l, qboolean isbig)
{
	q2dsnode_t		*ins;
	dl2node_t		*inl;
	int			child;
	mnode_t		*out;
	int			i, j, count;

	if (l->filelen % (isbig?sizeof(*inl):sizeof(*ins)))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / (isbig?sizeof(*inl):sizeof(*ins));

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

	out = ZG_Malloc(&mod->memgroup, sizeof(mnode_t)*count);

	mod->nodes = out;
	mod->numnodes = count;

	if (isbig)
	{
		inl = (void *)(mod_base + l->fileofs);
		for (i=0 ; i<count ; i++, out++, inl++)
		{
			for (j=0 ; j<3 ; j++)
			{
				out->minmaxs[j] = LittleFloat (inl->mins[j]);
				out->minmaxs[3+j] = LittleFloat (inl->maxs[j]);
			}

			out->plane = mod->planes + LittleLong(inl->planenum);

			out->firstsurface = (unsigned int)LittleLong (inl->firstface);
			out->numsurfaces = (unsigned int)LittleLong (inl->numfaces);
			out->contents = -1;	// differentiate from leafs

			for (j=0 ; j<2 ; j++)
			{
				child = LittleLong (inl->children[j]);
				out->childnum[j] = child;
				if (child < 0)
					out->children[j] = (mnode_t *)(mod->leafs + -1-child);
				else
					out->children[j] = mod->nodes + child;
			}
		}
	}
	else
	{
		ins = (void *)(mod_base + l->fileofs);
		for (i=0 ; i<count ; i++, out++, ins++)
		{
			for (j=0 ; j<3 ; j++)
			{
				out->minmaxs[j] = LittleShort (ins->mins[j]);
				out->minmaxs[3+j] = LittleShort (ins->maxs[j]);
			}

			out->plane = mod->planes + LittleLong(ins->planenum);

			out->firstsurface = (unsigned short)LittleShort (ins->firstface);
			out->numsurfaces = (unsigned short)LittleShort (ins->numfaces);
			out->contents = -1;	// differentiate from leafs

			for (j=0 ; j<2 ; j++)
			{
				child = LittleLong (ins->children[j]);
				out->childnum[j] = child;
				if (child < 0)
					out->children[j] = (mnode_t *)(mod->leafs + -1-child);
				else
					out->children[j] = mod->nodes + child;
			}
		}
	}

	CMod_SetParent (mod->nodes, NULL);	// sets nodes and leafs

	return true;
}

/*
=================
CMod_LoadBrushes

=================
*/
static qboolean CModQ2_LoadBrushes (model_t *mod, qbyte *mod_base, lump_t *l)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	q2dbrush_t	*in;
	q2cbrush_t	*out;
	int			i, count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count > SANITY_MAX_MAP_BRUSHES)
	{
		Con_Printf (CON_ERROR "Map has too many brushes");
		return false;
	}

	prv->brushes = ZG_Malloc(&mod->memgroup, sizeof(*out) * (count+1));

	out = prv->brushes;

	prv->numbrushes = count;

	for (i=0 ; i<count ; i++, out++, in++)
	{
		//FIXME: missing bounds checks
		out->brushside = &prv->brushsides[LittleLong(in->firstside)];
		out->numsides = LittleLong(in->numsides);
		out->contents = LittleLong(in->contents);
		CM_FinalizeBrush(out);
	}

	return true;
}

/*
=================
CMod_LoadLeafs
=================
*/
static qboolean CModQ2_LoadLeafs (model_t *mod, qbyte *mod_base, lump_t *l, qboolean isbig)
{
	int			i, j;
	mleaf_t		*out;
	q2dsleaf_t 	*ins;
	q2dlleaf_t 	*inl;
	int			count;

	if (l->filelen % (isbig?sizeof(*inl):sizeof(*ins)))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / (isbig?sizeof(*inl):sizeof(*ins));

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

	out = ZG_Malloc(&mod->memgroup, sizeof(*out) * (count+1));
	mod->numclusters = 0;

	mod->leafs = out;
	mod->numleafs = count;

	memset(out, 0, sizeof(*out)*count);
	if (isbig)
	{
		inl = (void *)(mod_base + l->fileofs);
		for ( i=0 ; i<count ; i++, inl++, out++)
		{
			for (j=0 ; j<3 ; j++)
			{
				out->minmaxs[j] = LittleFloat (inl->mins[j]);
				out->minmaxs[3+j] = LittleFloat (inl->maxs[j]);
			}

			out->contents = LittleLong (inl->contents);
			out->cluster = (unsigned int)LittleLong (inl->cluster);
			if (out->cluster == 0xffffffff)
				out->cluster = -1;

			out->area = (unsigned int)LittleLong (inl->area);
			out->firstleafbrush = (unsigned int)LittleLong (inl->firstleafbrush);
			out->numleafbrushes = (unsigned int)LittleLong (inl->numleafbrushes);

			out->firstmarksurface = mod->marksurfaces +
				(unsigned int)LittleLong(inl->firstleafface);
			out->nummarksurfaces = (unsigned int)LittleLong(inl->numleaffaces);

			if (out->cluster >= mod->numclusters)
				mod->numclusters = out->cluster + 1;
		}
	}
	else
	{
		ins = (void *)(mod_base + l->fileofs);
		for ( i=0 ; i<count ; i++, ins++, out++)
		{
			for (j=0 ; j<3 ; j++)
			{
				out->minmaxs[j] = LittleShort (ins->mins[j]);
				out->minmaxs[3+j] = LittleShort (ins->maxs[j]);
			}

			out->contents = LittleLong (ins->contents);
			out->cluster = (unsigned short)LittleShort (ins->cluster);
			if (out->cluster == 0xffff)
				out->cluster = -1;

			out->area = (unsigned short)LittleShort (ins->area);
			out->firstleafbrush = (unsigned short)LittleShort (ins->firstleafbrush);
			out->numleafbrushes = (unsigned short)LittleShort (ins->numleafbrushes);

			out->firstmarksurface = mod->marksurfaces +
				(unsigned short)LittleShort(ins->firstleafface);
			out->nummarksurfaces = (unsigned short)LittleShort(ins->numleaffaces);

			if (out->cluster >= mod->numclusters)
				mod->numclusters = out->cluster + 1;
		}
	}
	out = mod->leafs;
	mod->pvsbytes = ((mod->numclusters + 31)>>3)&~3;

	if (out[0].contents != Q2CONTENTS_SOLID)
	{
		Con_Printf (CON_ERROR "Map leaf 0 is not CONTENTS_SOLID\n");
		return false;
	}

	return true;
}

/*
=================
CMod_LoadPlanes
=================
*/
static qboolean CModQ2_LoadPlanes (model_t *mod, qbyte *mod_base, lump_t *l)
{
	int			i, j;
	mplane_t	*out;
	dplane_t 	*in;
	int			count;
	int			bits;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size\n");
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

	mod->planes = out = ZG_Malloc(&mod->memgroup, sizeof(*out) * count);
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
static qboolean CModQ2_LoadLeafBrushes (model_t *mod, qbyte *mod_base, lump_t *l, qboolean isbig)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	int			i;
	q2cbrush_t	**out;
	unsigned short 	*ins;
	unsigned int 	*inl;
	int			count;

	if (l->filelen % (isbig?sizeof(*inl):sizeof(*ins)))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / (isbig?sizeof(*inl):sizeof(*ins));

	if (count < 1)
	{
		Con_Printf (CON_ERROR "Map with no planes\n");
		return false;
	}
	// need to save space for box planes
	if (count > SANITY_MAX_MAP_LEAFBRUSHES)
	{
		Con_Printf (CON_ERROR "Map has too many leafbrushes\n");
		return false;
	}

	//prv->numbrushes is because of submodels being weird.
	out = prv->leafbrushes = ZG_Malloc(&mod->memgroup, sizeof(*out) * (count+prv->numbrushes));
	prv->numleafbrushes = count;

	if (isbig)
	{
		inl = (void *)(mod_base + l->fileofs);
		for ( i=0 ; i<count ; i++, inl++, out++)
			*out = prv->brushes + (unsigned int)LittleLong (*inl);
	}
	else
	{
		ins = (void *)(mod_base + l->fileofs);
		for ( i=0 ; i<count ; i++, ins++, out++)
			*out = prv->brushes + (unsigned short)(short)LittleShort (*ins);
	}

	return true;
}

/*
=================
CMod_LoadBrushSides
=================
*/
static qboolean CModQ2_LoadBrushSides (model_t *mod, qbyte *mod_base, lump_t *l, qboolean isbig)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	unsigned int			i, j;
	q2cbrushside_t	*out;
	q2dsbrushside_t 	*ins;
	q2dlbrushside_t 	*inl;
	int			count;
	int			num;

	if (l->filelen % (isbig?sizeof(*inl):sizeof(*ins)))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / (isbig?sizeof(*inl):sizeof(*ins));

	// need to save space for box planes
	if (count > SANITY_MAX_MAP_BRUSHSIDES)
	{
		Con_Printf (CON_ERROR "Map has too many brushsides (%i)\n", count);
		return false;
	}

	out = prv->brushsides = ZG_Malloc(&mod->memgroup, sizeof(*out) * count);
	prv->numbrushsides = count;

	if (isbig)
	{
		inl = (void *)(mod_base + l->fileofs);
		for ( i=0 ; i<count ; i++, inl++, out++)
		{
			num = (unsigned int)LittleLong (inl->planenum);
			out->plane = &mod->planes[num];
			j = (unsigned int)LittleLong (inl->texinfo);
			if (j >= mod->numtexinfo)
				out->surface = &nullsurface;
			else
				out->surface = &prv->surfaces[j];
		}
	}
	else
	{
		ins = (void *)(mod_base + l->fileofs);
		for ( i=0 ; i<count ; i++, ins++, out++)
		{
			num = (unsigned short)LittleShort (ins->planenum);
			out->plane = &mod->planes[num];
			j = (unsigned short)LittleShort (ins->texinfo);
			if (j >= mod->numtexinfo)
				out->surface = &nullsurface;
			else
				out->surface = &prv->surfaces[j];
		}
	}

	return true;
}

/*
=================
CMod_LoadAreas
=================
*/
static qboolean CModQ2_LoadAreas (model_t *mod, qbyte *mod_base, lump_t *l)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	int			i;
	q2carea_t	*out;
	q2darea_t 	*in;
	int			count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count > MAX_Q2MAP_AREAS)
	{
		Con_Printf (CON_ERROR "Map has too many areas\n");
		return false;
	}

	out = prv->q2areas = ZG_Malloc(&mod->memgroup, sizeof(*out) * count);
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
CMod_LoadAreaPortals
=================
*/
static qboolean CModQ2_LoadAreaPortals (model_t *mod, qbyte *mod_base, lump_t *l)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	int			i;
	q2dareaportal_t		*out;
	q2dareaportal_t 	*in;
	int			count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count > MAX_Q2MAP_AREAS)
	{
		Con_Printf (CON_ERROR "Map has too many areas\n");
		return false;
	}

	out = prv->q2areaportals = ZG_Malloc(&mod->memgroup, sizeof(*out) * count);
	prv->numq2areaportals = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->portalnum = LittleLong (in->portalnum);
		out->otherarea = LittleLong (in->otherarea);
	}

	return true;
}

/*
=================
CMod_LoadVisibility
=================
*/
static qboolean CModQ2_LoadVisibility (model_t *mod, qbyte *mod_base, lump_t *l)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	int		i;

	prv->numvisibility = l->filelen;
//	if (l->filelen > MAX_Q2MAP_VISIBILITY)
//	{
//		Con_Printf (CON_ERROR "Map has too large visibility lump\n");
//		return false;
//	}

	prv->q2vis = ZG_Malloc(&mod->memgroup, l->filelen);
	memcpy (prv->q2vis, mod_base + l->fileofs, l->filelen);

	mod->vis = prv->q2vis;

	prv->q2vis->numclusters = LittleLong (prv->q2vis->numclusters);
	for (i=0 ; i<prv->q2vis->numclusters ; i++)
	{
		prv->q2vis->bitofs[i][0] = LittleLong (prv->q2vis->bitofs[i][0]);
		prv->q2vis->bitofs[i][1] = LittleLong (prv->q2vis->bitofs[i][1]);
	}
	mod->numclusters = prv->q2vis->numclusters;
	mod->pvsbytes = ((mod->numclusters + 31)>>3)&~3;

	return true;
}
#endif	//q2bsps

#ifdef Q3BSPS
static qboolean CModQ3_LoadMarksurfaces (model_t *loadmodel, qbyte *mod_base, lump_t *l)
{
	int		i, j, count;
	int		*in;
	msurface_t **out;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "CModQ3_LoadMarksurfaces: funny lump size in %s\n",loadmodel->name);
		return false;
	}
	count = l->filelen / sizeof(*in);
	out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));

	loadmodel->marksurfaces = out;
	loadmodel->nummarksurfaces = count;

	for ( i=0 ; i<count ; i++)
	{
		j = LittleLong(in[i]);
		if (j < 0 || j >= loadmodel->numsurfaces)
		{
			Con_Printf (CON_ERROR "Mod_ParseMarksurfaces: bad surface number\n");
			return false;
		}
		out[i] = loadmodel->surfaces + j;
	}

	return true;
}

static qboolean CModQ3_LoadSubmodels (model_t *mod, qbyte *mod_base, lump_t *l)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	q3dmodel_t	*in;
	cmodel_t	*out;
	int			i, j, count;
	q2cbrush_t **leafbrush;
	mleaf_t		*bleaf;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size\n");
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

	out = prv->cmodels = ZG_Malloc(&mod->memgroup, count * sizeof(*prv->cmodels));
	prv->numcmodels = count;

	if (count > 1)
		bleaf = ZG_Malloc(&mod->memgroup, (count-1) * sizeof(*bleaf));
	else
		bleaf = NULL;

	prv->mapisq3 = true;

	for (i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat (in->mins[j]) - 1;
			out->maxs[j] = LittleFloat (in->maxs[j]) + 1;
			out->origin[j] = (out->maxs[j] + out->mins[j])/2;
		}
		out->firstsurface = LittleLong (in->firstsurface);
		out->numsurfaces = LittleLong (in->num_surfaces);

		out->firstbrush = LittleLong(in->firstbrush);
		out->num_brushes = LittleLong(in->num_brushes);
		if (!i)
		{
			out->headnode = mod->nodes;
			out->headleaf = NULL;
		}
		else
		{
//create a new leaf to hold the brushes and be directly clipped
			out->headleaf = bleaf;
			out->headnode = NULL;

			bleaf->numleafbrushes = LittleLong ( in->num_brushes );
			bleaf->firstleafbrush = prv->numleafbrushes;
			bleaf->contents = 0;

			leafbrush = &prv->leafbrushes[prv->numleafbrushes];
			for ( j = 0; j < bleaf->numleafbrushes; j++, leafbrush++ )
			{
				*leafbrush = prv->brushes + LittleLong ( in->firstbrush ) + j;
				bleaf->contents |= (*leafbrush)->contents;
			}
			prv->numleafbrushes += bleaf->numleafbrushes;
			bleaf++;
		}
		//submodels
	}

	AddPointToBounds(prv->cmodels[0].mins, mod->mins, mod->maxs);
	AddPointToBounds(prv->cmodels[0].maxs, mod->mins, mod->maxs);

	return true;
}

static qboolean CModQ3_LoadShaders (model_t *mod, qbyte *mod_base, lump_t *l)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	dq3shader_t	*in;
	q2mapsurface_t	*out;
	int				i, count;
	texture_t *tex;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count < 1)
	{
		Con_Printf (CON_ERROR "Map with no shaders\n");
		return false;
	}
//	else if (count > MAX_Q2MAP_TEXINFO)
//		Host_Error ("Map has too many shaders");

	mod->numtexinfo = count;
	out = prv->surfaces = ZG_Malloc(&mod->memgroup, count*sizeof(*out));

	mod->textures = ZG_Malloc(&mod->memgroup, (sizeof(texture_t*)+sizeof(mtexinfo_t)+sizeof(texture_t))*(count*2+1));	//+1 is 'noshader' for flares.
	mod->texinfo = (mtexinfo_t*)(mod->textures+(count*2+1));
	tex = (texture_t*)(mod->texinfo+(count*2+1));
	mod->numtextures = count*2+1;

	for ( i=0 ; i<count ; i++, in++, out++ )
	{
		out->c.flags = LittleLong ( in->surfflags );
		out->c.value = LittleLong ( in->contents );

		mod->texinfo[i].texture = tex+i;
		mod->texinfo[i].flags = prv->surfaces[i].c.flags;
		Q_strncpyz(mod->texinfo[i].texture->name, in->shadername, sizeof(mod->texinfo[i].texture->name));
		mod->textures[i] = mod->texinfo[i].texture;
	}
	for ( i=0, in-=count ; i<count ; i++, in++ )
	{
		mod->texinfo[i+count].texture = tex+i+count;
		mod->texinfo[i+count].flags = prv->surfaces[i].c.flags;
		Q_strncpyz(mod->texinfo[i+count].texture->name, in->shadername, sizeof(mod->texinfo[i+count].texture->name));
		mod->textures[i+count] = mod->texinfo[i+count].texture;
	}

	//and for flares, which are not supported at this time.
	mod->texinfo[count*2].texture = tex+count*2;
	mod->texinfo[i+count].flags = 0;
	Q_strncpyz(mod->texinfo[count*2].texture->name, "noshader", sizeof(mod->texinfo[count*2].texture->name));
	mod->textures[count*2] = mod->texinfo[count*2].texture;

	return true;
}

static qboolean CModQ3_LoadVertexes (model_t *mod, qbyte *mod_base, lump_t *l)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	q3dvertex_t	*in;
	vecV_t		*out;
	vec3_t		*nout;
	//, *sout, *tout;
	int			i, count, j;
	vec2_t		*lmout, *stout;
	vec4_t *cout;
	extern cvar_t gl_overbright;
	extern qbyte lmgamma[256];

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "CMOD_LoadVertexes: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count > MAX_Q3MAP_VERTEXES)
	{
		Con_Printf (CON_ERROR "Map has too many vertexes\n");
		return false;
	}

	BuildLightMapGammaTable(1, 1<<(2-gl_overbright.ival));

	out = ZG_Malloc(&mod->memgroup, count*sizeof(*out));
	stout = ZG_Malloc(&mod->memgroup, count*sizeof(*stout));
	lmout = ZG_Malloc(&mod->memgroup, count*sizeof(*lmout));
	cout = ZG_Malloc(&mod->memgroup, count*sizeof(*cout));
	nout = ZG_Malloc(&mod->memgroup, count*sizeof(*nout));
//	sout = ZG_Malloc(&mod->memgroup, count*sizeof(*nout));
//	tout = ZG_Malloc(&mod->memgroup, count*sizeof(*nout));
	prv->verts = out;
	prv->vertstmexcoords = stout;
	for (i = 0; i < MAXRLIGHTMAPS; i++)
	{
		prv->vertlstmexcoords[i] = lmout;
		prv->colors4f_array[i] = cout;
	}
	prv->normals_array = nout;
//	prv->svector_array = sout;
//	prv->tvector_array = tout;
	prv->numvertexes = count;

	for ( i=0 ; i<count ; i++, in++)
	{
		for ( j=0 ; j < 3 ; j++)
		{
			out[i][j] = LittleFloat ( in->point[j] );
			nout[i][j] = LittleFloat (in->normal[j]);
		}
		for ( j=0 ; j < 2 ; j++)
		{
			stout[i][j] = LittleFloat ( ((float *)in->texcoords)[j] );
			lmout[i][j] = LittleFloat ( ((float *)in->texcoords)[j+2] );
		}
		cout[i][0] = (lmgamma[in->color[0]]<<gl_overbright.ival)/255.0f;
		cout[i][1] = (lmgamma[in->color[1]]<<gl_overbright.ival)/255.0f;
		cout[i][2] = (lmgamma[in->color[2]]<<gl_overbright.ival)/255.0f;
		cout[i][3] = in->color[3]/255.0f;
	}

//	if (r_lightmap_saturation.value != 1.0f)
//		SaturateR8G8B8(cout, count*4, r_lightmap_saturation.value);

	return true;
}

#ifdef RFBSPS
static qboolean CModRBSP_LoadVertexes (model_t *mod, qbyte *mod_base, lump_t *l)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	rbspvertex_t	*in;
	vecV_t		*out;
	vec3_t		*nout;
	//, *sout, *tout;
	int			i, count, j;
	vec2_t		*lmout, *stout;
	vec4_t *cout;
	int sty;
	extern qbyte lmgamma[256];

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "CMOD_LoadVertexes: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count > MAX_Q3MAP_VERTEXES)
	{
		Con_Printf (CON_ERROR "Map has too many vertexes\n");
		return false;
	}

	out = ZG_Malloc(&mod->memgroup, count*sizeof(*out));
	stout = ZG_Malloc(&mod->memgroup, count*sizeof(*stout));
	lmout = ZG_Malloc(&mod->memgroup, MAXRLIGHTMAPS*count*sizeof(*lmout));
	cout = ZG_Malloc(&mod->memgroup, MAXRLIGHTMAPS*count*sizeof(*cout));
	nout = ZG_Malloc(&mod->memgroup, count*sizeof(*nout));
//	sout = ZG_Malloc(&mod->memgroup, count*sizeof(*sout));
//	tout = ZG_Malloc(&mod->memgroup, count*sizeof(*tout));
	prv->verts = out;
	prv->vertstmexcoords = stout;
	for (sty = 0; sty < MAXRLIGHTMAPS; sty++)
	{
		prv->vertlstmexcoords[sty] = lmout + sty*count;
		prv->colors4f_array[sty] = cout + sty*count;
	}
	prv->normals_array = nout;
//	prv->svector_array = sout;
//	prv->tvector_array = tout;
	prv->numvertexes = count;

	for ( i=0 ; i<count ; i++, in++)
	{
		for ( j=0 ; j < 3 ; j++)
		{
			out[i][j] = LittleFloat ( in->point[j] );
			nout[i][j] = LittleFloat (in->normal[j]);
		}
		for ( j=0 ; j < 2 ; j++)
		{
			stout[i][j] = LittleFloat (in->stcoords[j]);
			for (sty = 0; sty < min(MAXRLIGHTMAPS, RBSP_STYLESPERSURF); sty++)
				prv->vertlstmexcoords[sty][i][j] = LittleFloat(in->lmtexcoords[sty][j]);
		}
		for (sty = 0; sty < min(MAXRLIGHTMAPS, RBSP_STYLESPERSURF); sty++)
		{
			prv->colors4f_array[sty][i][0] = lmgamma[in->color[sty][0]]/255.0f;
			prv->colors4f_array[sty][i][1] = lmgamma[in->color[sty][1]]/255.0f;
			prv->colors4f_array[sty][i][2] = lmgamma[in->color[sty][2]]/255.0f;
			prv->colors4f_array[sty][i][3] = in->color[sty][3]/255.0f;
		}
	}

	return true;
}
#endif

#ifndef SERVERONLY
static qboolean CModQ3_LoadIndexes (model_t *loadmodel, qbyte *mod_base, lump_t *l)
{
	cminfo_t	*prv = (cminfo_t*)loadmodel->meshinfo;
	int		i, count;
	int		*in;
	index_t	*out;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n", loadmodel->name);
		return false;
	}
	count = l->filelen / sizeof(*in);
	if (count < 1 || count >= MAX_Q3MAP_INDICES)
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: too many indicies in %s: %i\n",
					loadmodel->name, count);
		return false;
	}

	out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));

	prv->surfindexes = out;
//	prv->numsurfindexes = count;

	for ( i=0 ; i<count ; i++)
		out[i] = LittleLong (in[i]);

	return true;
}
#endif

/*
=================
CMod_LoadFaces
=================
*/
static qboolean CModQ3_LoadFaces (model_t *mod, qbyte *mod_base, lump_t *l)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	q3dface_t		*in;
	q3cface_t		*out;
	int			i, count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count > SANITY_LIMIT(*out))
	{
		Con_Printf (CON_ERROR "Map has too many faces\n");
		return false;
	}

	out = BZ_Malloc ( count*sizeof(*out) );
	prv->faces = out;
	prv->numfaces = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->facetype = LittleLong ( in->facetype );
		out->shadernum = LittleLong ( in->shadernum );

		out->numverts = LittleLong ( in->num_vertices );
		out->firstvert = LittleLong ( in->firstvertex );

		if (out->facetype == MST_PATCH || out->facetype == MST_PATCH_FIXED)
		{
			unsigned int pw = LittleLong ( in->patchwidth );
			unsigned int ph = LittleLong ( in->patchheight );
			out->patch.cp[0] = pw&0xffff;
			out->patch.cp[1] = ph&0xffff;
			out->patch.fixedres[0] = pw>>16;
			out->patch.fixedres[1] = ph>>16;
		}
		else
		{
			out->soup.firstindex = LittleLong(in->firstindex);
			out->soup.numindicies = LittleLong(in->num_indexes);
		}
	}

	mod->numsurfaces = i;

	return true;
}

#ifdef RFBSPS
static qboolean CModRBSP_LoadFaces (model_t *mod, qbyte *mod_base, lump_t *l)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	rbspface_t		*in;
	q3cface_t		*out;
	int			i, count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count > SANITY_LIMIT(*out))
	{
		Con_Printf (CON_ERROR "Map has too many faces\n");
		return false;
	}

	out = BZ_Malloc ( count*sizeof(*out) );
	prv->faces = out;
	prv->numfaces = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->facetype = LittleLong ( in->facetype );
		out->shadernum = LittleLong ( in->shadernum );

		out->numverts = LittleLong ( in->num_vertices );
		out->firstvert = LittleLong ( in->firstvertex );

		if (out->facetype == MST_PATCH || out->facetype == MST_PATCH_FIXED)
		{
			unsigned int pw = LittleLong ( in->patchwidth );
			unsigned int ph = LittleLong ( in->patchheight );
			out->patch.cp[0] = pw&0xffff;
			out->patch.cp[1] = ph&0xffff;
			out->patch.fixedres[0] = pw>>16;
			out->patch.fixedres[1] = ph>>16;
		}
		else
		{
			out->soup.firstindex = LittleLong(in->firstindex);
			out->soup.numindicies = LittleLong(in->num_indexes);
		}
	}

	mod->numsurfaces = i;
	return true;
}
#endif

#ifndef SERVERONLY

/*
=================
Mod_LoadFogs
=================
*/
static qboolean CModQ3_LoadFogs (model_t *mod, qbyte *mod_base, lump_t *l)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	dfog_t 	*in;
	mfog_t 	*out;
	q2cbrush_t *brush;
	q2cbrushside_t *visibleside, *brushsides;
	int		i, j, count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n", mod->name);
		return false;
	}
	count = l->filelen / sizeof(*in);
	out = ZG_Malloc(&mod->memgroup, count*sizeof(*out));

	mod->fogs = out;
	mod->numfogs = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		if ( LittleLong ( in->visibleSide ) == -1 )
		{
			continue;
		}

		brush = prv->brushes + LittleLong ( in->brushNum );
		brushsides = brush->brushside;
		visibleside = brushsides + LittleLong ( in->visibleSide );

		out->visibleplane = visibleside->plane;
		Q_strncpyz(out->shadername, in->shader, sizeof(out->shadername));
		out->numplanes = brush->numsides;
		out->planes = ZG_Malloc(&mod->memgroup, out->numplanes*sizeof(cplane_t *));

		for ( j = 0; j < out->numplanes; j++ )
		{
			out->planes[j] = brushsides[j].plane;
		}
	}

	return true;
}

mfog_t *Mod_FogForOrigin(model_t *wmodel, vec3_t org)
{
	int i, j;
	mfog_t 	*ret;
	float dot;

	if (!wmodel || wmodel->loadstate != MLS_LOADED)
		return NULL;

	for ( i=0 , ret=wmodel->fogs ; i<wmodel->numfogs ; i++, ret++)
	{
		if (!ret->shader)
			continue;
		for (j = 0; j < ret->numplanes; j++)
		{
			dot = DotProduct(ret->planes[j]->normal, org);
			if (dot - ret->planes[j]->dist > 0)
				break;
		}
		if (j == ret->numplanes)
		{
			return ret;
		}
	}

	return NULL;
}

//Convert a patch in to a list of glpolys

#define MAX_ARRAY_VERTS 65535

static index_t tempIndexesArray[MAX_ARRAY_VERTS*6];

static void GL_SizePatchFixed(mesh_t *mesh, int patchwidth, int patchheight, int numverts, int firstvert, cminfo_t *prv)
{
	unsigned short patch_cp[2];
	int step[2], size[2];

	patch_cp[0] = patchwidth&0xffff;
	patch_cp[1] = patchheight&0xffff;

	if (patch_cp[0] <= 0 || patch_cp[1] <= 0 )
	{
		mesh->numindexes = 0;
		mesh->numvertexes = 0;
		return;
	}

	// allocate space for mesh
	step[0] = patchwidth>>16;
	step[1] = patchheight>>16;
	if (!step[0] || !step[1])
	{
		size[0] = patch_cp[0];
		size[1] = patch_cp[1];
	}
	else
	{
		size[0] = (patch_cp[0] / 2) * step[0] + 1;
		size[1] = (patch_cp[1] / 2) * step[1] + 1;
	}

	mesh->numvertexes = size[0] * size[1];
	mesh->numindexes = (size[0]-1) * (size[1]-1) * 6;
}
static void GL_SizePatch(mesh_t *mesh, int patchwidth, int patchheight, int numverts, int firstvert, cminfo_t *prv)
{
	unsigned short patch_cp[2];
	int step[2], size[2], flat[2];
	float subdivlevel;

	patch_cp[0] = patchwidth;
	patch_cp[1] = patchheight;

	if (patch_cp[0] <= 0 || patch_cp[1] <= 0 )
	{
		mesh->numindexes = 0;
		mesh->numvertexes = 0;
		return;
	}

	subdivlevel = r_subdivisions.value;
	if ( subdivlevel < 1 )
		subdivlevel = 1;

// find the degree of subdivision in the u and v directions
	Patch_GetFlatness ( subdivlevel, prv->verts[firstvert], sizeof(vecV_t)/sizeof(vec_t), patch_cp, flat );

// allocate space for mesh
	step[0] = (1 << flat[0]);
	step[1] = (1 << flat[1]);
	size[0] = (patch_cp[0] / 2) * step[0] + 1;
	size[1] = (patch_cp[1] / 2) * step[1] + 1;

	mesh->numvertexes = size[0] * size[1];
	mesh->numindexes = (size[0]-1) * (size[1]-1) * 6;
}

//mesh_t *GL_CreateMeshForPatch ( model_t *mod, q3dface_t *surf )
static void GL_CreateMeshForPatch (model_t *mod, mesh_t *mesh, int patchwidth, int patchheight, int numverts, int firstvert)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	int numindexes, step[2], size[2], flat[2], i, u, v, p;
	unsigned short patch_cp[2];
	index_t	*indexes;
	float subdivlevel;
	int sty;

	patch_cp[0] = patchwidth;
	patch_cp[1] = patchheight;

	if (patch_cp[0] <= 0 || patch_cp[1] <= 0 )
	{
		mesh->numindexes = 0;
		mesh->numvertexes = 0;
		return;
	}

	subdivlevel = r_subdivisions.value;
	if ( subdivlevel < 1 )
		subdivlevel = 1;

// find the degree of subdivision in the u and v directions
	Patch_GetFlatness ( subdivlevel, prv->verts[firstvert], sizeof(vecV_t)/sizeof(vec_t), patch_cp, flat );

// allocate space for mesh
	step[0] = (1 << flat[0]);
	step[1] = (1 << flat[1]);
	size[0] = (patch_cp[0] / 2) * step[0] + 1;
	size[1] = (patch_cp[1] / 2) * step[1] + 1;
	numverts = size[0] * size[1];

	if ( numverts < 0 || numverts > MAX_ARRAY_VERTS )
	{
		mesh->numindexes = 0;
		mesh->numvertexes = 0;
		return;
	}


	if (mesh->numvertexes != numverts)
	{
		mesh->numindexes = 0;
		mesh->numvertexes = 0;
		return;
	}

// fill in

	Patch_Evaluate ( prv->verts[firstvert], patch_cp, step, mesh->xyz_array[0], sizeof(vecV_t)/sizeof(vec_t));
	for (sty = 0; sty < MAXRLIGHTMAPS; sty++)
	{
		if (mesh->colors4f_array[sty])
			Patch_Evaluate ( prv->colors4f_array[sty][firstvert], patch_cp, step, mesh->colors4f_array[sty][0], 4 );
	}
	Patch_Evaluate ( prv->normals_array[firstvert], patch_cp, step, mesh->normals_array[0], 3 );
	Patch_Evaluate ( prv->vertstmexcoords[firstvert], patch_cp, step, mesh->st_array[0], 2 );
	for (sty = 0; sty < MAXRLIGHTMAPS; sty++)
	{
		if (mesh->lmst_array[sty])
			Patch_Evaluate ( prv->vertlstmexcoords[sty][firstvert], patch_cp, step, mesh->lmst_array[sty][0], 2 );
	}

// compute new indexes avoiding adding invalid triangles
	numindexes = 0;
	indexes = tempIndexesArray;
	for (v = 0, i = 0; v < size[1]-1; v++)
	{
		for (u = 0; u < size[0]-1; u++, i += 6)
		{
			indexes[0] = p = v * size[0] + u;
			indexes[1] = p + size[0];
			indexes[2] = p + 1;

//			if ( !VectorEquals(mesh->xyz_array[indexes[0]], mesh->xyz_array[indexes[1]]) &&
//				!VectorEquals(mesh->xyz_array[indexes[0]], mesh->xyz_array[indexes[2]]) &&
//				!VectorEquals(mesh->xyz_array[indexes[1]], mesh->xyz_array[indexes[2]]) )
			{
				indexes += 3;
				numindexes += 3;
			}

			indexes[0] = p + 1;
			indexes[1] = p + size[0];
			indexes[2] = p + size[0] + 1;

//			if ( !VectorEquals(mesh->xyz_array[indexes[0]], mesh->xyz_array[indexes[1]]) &&
//				!VectorEquals(mesh->xyz_array[indexes[0]], mesh->xyz_array[indexes[2]]) &&
//				!VectorEquals(mesh->xyz_array[indexes[1]], mesh->xyz_array[indexes[2]]) )
			{
				indexes += 3;
				numindexes += 3;
			}
		}
	}

// allocate and fill index table

	mesh->numindexes = numindexes;

	memcpy (mesh->indexes, tempIndexesArray, numindexes * sizeof(index_t) );
}

static void GL_CreateMeshForPatchFixed (model_t *mod, mesh_t *mesh, int patchwidth, int patchheight, int numverts, int firstvert)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	int numindexes, step[2], size[2], i, u, v, p;
	unsigned short patch_cp[2];
	index_t	*indexes;
	float subdivlevel;
	int sty;

	patch_cp[0] = patchwidth&0xffff;
	patch_cp[1] = patchheight&0xffff;
	if (patch_cp[0] <= 0 || patch_cp[1] <= 0 )
	{
		mesh->numindexes = 0;
		mesh->numvertexes = 0;
		return;
	}

	subdivlevel = r_subdivisions.value;
	if ( subdivlevel < 1 )
		subdivlevel = 1;

// allocate space for mesh
	step[0] = patchwidth>>16;
	step[1] = patchheight>>16;
	if (!step[0] || !step[1])
	{	//explicit CPs only.
		size[0] = patch_cp[0];
		size[1] = patch_cp[1];
	}
	else
	{
		size[0] = (patch_cp[0] / 2) * step[0] + 1;
		size[1] = (patch_cp[1] / 2) * step[1] + 1;
	}
	numverts = size[0] * size[1];

	if ( numverts < 0 || numverts > MAX_ARRAY_VERTS )
	{
		mesh->numindexes = 0;
		mesh->numvertexes = 0;
		return;
	}


	if (mesh->numvertexes != numverts)
	{
		mesh->numindexes = 0;
		mesh->numvertexes = 0;
		return;
	}

// fill in

	Patch_Evaluate ( prv->verts[firstvert], patch_cp, step, mesh->xyz_array[0], sizeof(vecV_t)/sizeof(vec_t));
	for (sty = 0; sty < MAXRLIGHTMAPS; sty++)
	{
		if (mesh->colors4f_array[sty])
			Patch_Evaluate ( prv->colors4f_array[sty][firstvert], patch_cp, step, mesh->colors4f_array[sty][0], 4 );
	}
	Patch_Evaluate ( prv->normals_array[firstvert], patch_cp, step, mesh->normals_array[0], 3 );
	Patch_Evaluate ( prv->vertstmexcoords[firstvert], patch_cp, step, mesh->st_array[0], 2 );
	for (sty = 0; sty < MAXRLIGHTMAPS; sty++)
	{
		if (mesh->lmst_array[sty])
			Patch_Evaluate ( prv->vertlstmexcoords[sty][firstvert], patch_cp, step, mesh->lmst_array[sty][0], 2 );
	}

// compute new indexes avoiding adding invalid triangles
	numindexes = 0;
	indexes = tempIndexesArray;
	for (v = 0, i = 0; v < size[1]-1; v++)
	{
		for (u = 0; u < size[0]-1; u++, i += 6)
		{
			indexes[0] = p = v * size[0] + u;
			indexes[1] = p + size[0];
			indexes[2] = p + 1;

//			if ( !VectorEquals(mesh->xyz_array[indexes[0]], mesh->xyz_array[indexes[1]]) &&
//				!VectorEquals(mesh->xyz_array[indexes[0]], mesh->xyz_array[indexes[2]]) &&
//				!VectorEquals(mesh->xyz_array[indexes[1]], mesh->xyz_array[indexes[2]]) )
			{
				indexes += 3;
				numindexes += 3;
			}

			indexes[0] = p + 1;
			indexes[1] = p + size[0];
			indexes[2] = p + size[0] + 1;

//			if ( !VectorEquals(mesh->xyz_array[indexes[0]], mesh->xyz_array[indexes[1]]) &&
//				!VectorEquals(mesh->xyz_array[indexes[0]], mesh->xyz_array[indexes[2]]) &&
//				!VectorEquals(mesh->xyz_array[indexes[1]], mesh->xyz_array[indexes[2]]) )
			{
				indexes += 3;
				numindexes += 3;
			}
		}
	}

// allocate and fill index table

	mesh->numindexes = numindexes;

	memcpy (mesh->indexes, tempIndexesArray, numindexes * sizeof(index_t) );
}

#ifdef RFBSPS
static void CModRBSP_BuildSurfMesh(model_t *mod, msurface_t *out, builddata_t *bd)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	rbspface_t *in = (rbspface_t*)(bd+1);
	int idx = (out - mod->surfaces) - mod->firstmodelsurface;
	int sty;
	in += idx;

	if (LittleLong(in->facetype) == MST_PATCH)
	{
		GL_CreateMeshForPatch(mod, out->mesh, LittleLong(in->patchwidth), LittleLong(in->patchheight), LittleLong(in->num_vertices), LittleLong(in->firstvertex));
	}
	else if (LittleLong(in->facetype) == MST_PATCH_FIXED)
	{
		GL_CreateMeshForPatchFixed(mod, out->mesh, LittleLong(in->patchwidth), LittleLong(in->patchheight), LittleLong(in->num_vertices), LittleLong(in->firstvertex));
	}
	else if (LittleLong(in->facetype) == MST_PLANAR || LittleLong(in->facetype) == MST_TRIANGLE_SOUP)
	{
		unsigned int fv = LittleLong(in->firstvertex), i;
		for (i = 0; i < out->mesh->numvertexes; i++)
		{
			VectorCopy(prv->verts[fv + i], out->mesh->xyz_array[i]);
			Vector2Copy(prv->vertstmexcoords[fv + i], out->mesh->st_array[i]);
			for (sty = 0; sty < MAXRLIGHTMAPS; sty++)
			{
				Vector2Copy(prv->vertlstmexcoords[sty][fv + i], out->mesh->lmst_array[sty][i]);
				Vector4Copy(prv->colors4f_array[sty][fv + i], out->mesh->colors4f_array[sty][i]);
			}

			VectorCopy(prv->normals_array[fv + i], out->mesh->normals_array[i]);
		}
		fv = LittleLong(in->firstindex);
		for (i = 0; i < out->mesh->numindexes; i++)
		{
			out->mesh->indexes[i] = prv->surfindexes[fv + i];
		}
	}
	else
	{
/*		//flare
		int r, g, b;
		extern index_t r_quad_indexes[6];
		static vec2_t	st[4] = {{0,0},{0,1},{1,1},{1,0}};

		mesh = out->mesh = (mesh_t *)Hunk_Alloc(sizeof(mesh_t));
		mesh->xyz_array = (vecV_t *)Hunk_Alloc(sizeof(vecV_t)*4);
		mesh->colors4b_array = (byte_vec4_t *)Hunk_Alloc(sizeof(byte_vec4_t)*4);
		mesh->numvertexes = 4;
		mesh->indexes = r_quad_indexes;
		mesh->st_array = st;
		mesh->numindexes = 6;

		VectorCopy (in->lightmap_origin, mesh->xyz_array[0]);
		VectorCopy (in->lightmap_origin, mesh->xyz_array[1]);
		VectorCopy (in->lightmap_origin, mesh->xyz_array[2]);
		VectorCopy (in->lightmap_origin, mesh->xyz_array[3]);

		r = LittleFloat(in->lightmap_vecs[0][0]) * 255.0f;
		r = bound (0, r, 255);
		g = LittleFloat(in->lightmap_vecs[0][1]) * 255.0f;
		g = bound (0, g, 255);
		b = LittleFloat(in->lightmap_vecs[0][2]) * 255.0f;
		b = bound (0, b, 255);

		mesh->colors4b_array[0][0] = r;
		mesh->colors4b_array[0][1] = g;
		mesh->colors4b_array[0][2] = b;
		mesh->colors4b_array[0][3] = 255;
		Vector4Copy(mesh->colors4b_array[0], mesh->colors4b_array[1]);
		Vector4Copy(mesh->colors4b_array[0], mesh->colors4b_array[2]);
		Vector4Copy(mesh->colors4b_array[0], mesh->colors4b_array[3]);
*/
	}

	Mod_AccumulateMeshTextureVectors(out->mesh);
	Mod_NormaliseTextureVectors(out->mesh->normals_array, out->mesh->snormals_array, out->mesh->tnormals_array, out->mesh->numvertexes, false);
}
#endif

static void CModQ3_BuildSurfMesh(model_t *mod, msurface_t *out, builddata_t *bd)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	int idx = (out - mod->surfaces) - mod->firstmodelsurface;
	q3dface_t *in = (q3dface_t*)(bd+1) + idx;
	int facetype = LittleLong(in->facetype);

	if (facetype == MST_PATCH)
	{
		GL_CreateMeshForPatch(mod, out->mesh, LittleLong(in->patchwidth), LittleLong(in->patchheight), LittleLong(in->num_vertices), LittleLong(in->firstvertex));
	}
	else if (facetype == MST_PATCH_FIXED)
	{
		GL_CreateMeshForPatchFixed(mod, out->mesh, LittleLong(in->patchwidth), LittleLong(in->patchheight), LittleLong(in->num_vertices), LittleLong(in->firstvertex));
	}
	else if (facetype == MST_PLANAR || facetype == MST_TRIANGLE_SOUP)
	{
		unsigned int fv = LittleLong(in->firstvertex), fi = LittleLong(in->firstindex), i;
		for (i = 0; i < out->mesh->numvertexes; i++)
		{
			VectorCopy(prv->verts[fv + i], out->mesh->xyz_array[i]);
			Vector2Copy(prv->vertstmexcoords[fv + i], out->mesh->st_array[i]);
			Vector2Copy(prv->vertlstmexcoords[0][fv + i], out->mesh->lmst_array[0][i]);
			Vector4Copy(prv->colors4f_array[0][fv + i], out->mesh->colors4f_array[0][i]);

			VectorCopy(prv->normals_array[fv + i], out->mesh->normals_array[i]);
		}
		for (i = 0; i < out->mesh->numindexes; i++)
		{
			out->mesh->indexes[i] = prv->surfindexes[fi + i];
		}
	}
	else
	{
/*		//flare
		int r, g, b;
		extern index_t r_quad_indexes[6];
		static vec2_t	st[4] = {{0,0},{0,1},{1,1},{1,0}};

		mesh = out->mesh = (mesh_t *)Hunk_Alloc(sizeof(mesh_t));
		mesh->xyz_array = (vecV_t *)Hunk_Alloc(sizeof(vecV_t)*4);
		mesh->colors4b_array = (byte_vec4_t *)Hunk_Alloc(sizeof(byte_vec4_t)*4);
		mesh->numvertexes = 4;
		mesh->indexes = r_quad_indexes;
		mesh->st_array = st;
		mesh->numindexes = 6;

		VectorCopy (in->lightmap_origin, mesh->xyz_array[0]);
		VectorCopy (in->lightmap_origin, mesh->xyz_array[1]);
		VectorCopy (in->lightmap_origin, mesh->xyz_array[2]);
		VectorCopy (in->lightmap_origin, mesh->xyz_array[3]);

		r = LittleFloat(in->lightmap_vecs[0][0]) * 255.0f;
		r = bound (0, r, 255);
		g = LittleFloat(in->lightmap_vecs[0][1]) * 255.0f;
		g = bound (0, g, 255);
		b = LittleFloat(in->lightmap_vecs[0][2]) * 255.0f;
		b = bound (0, b, 255);

		mesh->colors4b_array[0][0] = r;
		mesh->colors4b_array[0][1] = g;
		mesh->colors4b_array[0][2] = b;
		mesh->colors4b_array[0][3] = 255;
		Vector4Copy(mesh->colors4b_array[0], mesh->colors4b_array[1]);
		Vector4Copy(mesh->colors4b_array[0], mesh->colors4b_array[2]);
		Vector4Copy(mesh->colors4b_array[0], mesh->colors4b_array[3]);
*/
	}

	Mod_AccumulateMeshTextureVectors(out->mesh);
	Mod_NormaliseTextureVectors(out->mesh->normals_array, out->mesh->snormals_array, out->mesh->tnormals_array, out->mesh->numvertexes, false);
}

static qboolean CModQ3_LoadRFaces (model_t *mod, qbyte *mod_base, lump_t *l)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	extern cvar_t r_vertexlight;
	q3dface_t *in;
	msurface_t *out;
	mplane_t *pl;

	int facetype;
	int count;
	int surfnum;
	int shadernum;

	int fv;
	int sty; 

	mesh_t *mesh;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",mod->name);
		return false;
	}
	count = l->filelen / sizeof(*in);
	out = ZG_Malloc(&mod->memgroup, count*sizeof(*out));
	pl = ZG_Malloc(&mod->memgroup, count*sizeof(*pl));//create a new array of planes for speed.
	mesh = ZG_Malloc(&mod->memgroup, count*sizeof(*mesh));

	mod->surfaces = out;
	mod->numsurfaces = count;
	mod->lightmaps.first = 0;

	for (surfnum = 0; surfnum < count; surfnum++, out++, in++, pl++)
	{
		out->plane = pl;
		
		shadernum = LittleLong(in->shadernum);
		if (shadernum < 0 || shadernum >= mod->numtexinfo)
		{
			Con_Printf (CON_ERROR "CMod_LoadRFaces: bad shader index %s\n",mod->name);
			return false;
		}
		facetype = LittleLong(in->facetype);
		out->texinfo = mod->texinfo + shadernum;
		out->lightmaptexturenums[0] = LittleLong(in->lightmapnum);
		if (facetype == MST_FLARE)
			out->texinfo = mod->texinfo + mod->numtexinfo*2;
		else if (out->lightmaptexturenums[0] < 0 /*|| facetype == MST_TRIANGLE_SOUP*/ || r_vertexlight.value)
			out->texinfo += mod->numtexinfo;	//various surfaces use a different version of the same shader (with all the lightmaps collapsed)

		out->light_s[0] = LittleLong(in->lightmap_offs[0]);
		out->light_t[0] = LittleLong(in->lightmap_offs[1]);
		out->styles[0] = INVALID_LIGHTSTYLE;
		out->vlstyles[0] = INVALID_VLIGHTSTYLE;
		for (sty = 1; sty < MAXRLIGHTMAPS; sty++)
		{
			out->styles[sty] = INVALID_LIGHTSTYLE;
			out->vlstyles[sty] = INVALID_VLIGHTSTYLE;
			out->lightmaptexturenums[sty] = -1;
		}
		for (;  sty < MAXCPULIGHTMAPS; sty++)
			out->styles[sty] = INVALID_LIGHTSTYLE;
		out->lmshift = LMSHIFT_DEFAULT;
		//fixme: determine texturemins from lightmap_origin
		out->extents[0] = (LittleLong(in->lightmap_width)-1)<<out->lmshift;
		out->extents[1] = (LittleLong(in->lightmap_height)-1)<<out->lmshift;
		out->samples=NULL;

		if (mod->lightmaps.count < out->lightmaptexturenums[0]+1)
			mod->lightmaps.count = out->lightmaptexturenums[0]+1;

		fv = LittleLong(in->firstvertex);
		{
			vec3_t v[3];
			VectorCopy(prv->verts[fv+0], v[0]);
			VectorCopy(prv->verts[fv+1], v[1]);
			VectorCopy(prv->verts[fv+2], v[2]);
			PlaneFromPoints(v, pl);
			CategorizePlane(pl);
		}

		if (prv->surfaces[shadernum].c.value == 0 || prv->surfaces[shadernum].c.value & Q3CONTENTS_TRANSLUCENT)
				//q3dm10's thingie is 0
			out->flags |= SURF_DRAWALPHA;

		if (mod->texinfo[shadernum].flags & TI_SKY)
			out->flags |= SURF_DRAWSKY|SURF_DRAWTILED;

		if (LittleLong(in->fognum) == -1 || !mod->numfogs)
			out->fog = NULL;
		else
			out->fog = mod->fogs + LittleLong(in->fognum);
		if (prv->surfaces[shadernum].c.flags & (Q3SURF_NODRAW | Q3SURF_SKIP))
		{
			out->mesh = &mesh[surfnum];
			out->mesh->numindexes = 0;
			out->mesh->numvertexes = 0;
		}
		else if (facetype == MST_PATCH)
		{
			out->mesh = &mesh[surfnum];
			GL_SizePatch(out->mesh, LittleLong(in->patchwidth), LittleLong(in->patchheight), LittleLong(in->num_vertices), LittleLong(in->firstvertex), prv);
		}
		else if (facetype == MST_PATCH_FIXED)
		{
			out->mesh = &mesh[surfnum];
			GL_SizePatchFixed(out->mesh, LittleLong(in->patchwidth), LittleLong(in->patchheight), LittleLong(in->num_vertices), LittleLong(in->firstvertex), prv);
		}
		else if (facetype == MST_PLANAR || facetype == MST_TRIANGLE_SOUP)
		{
			out->mesh = &mesh[surfnum];
			out->mesh->numindexes = LittleLong(in->num_indexes);
			out->mesh->numvertexes = LittleLong(in->num_vertices);
/*
			Mod_AccumulateMeshTextureVectors(out->mesh);
*/
		}
		else
		{
			out->mesh = &mesh[surfnum];
			out->mesh->numindexes = 6;
			out->mesh->numvertexes = 4;
		}
	}

	Mod_SortShaders(mod);
	return true;
}

#ifdef RFBSPS
static qboolean CModRBSP_LoadRFaces (model_t *mod, qbyte *mod_base, lump_t *l)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	extern cvar_t r_vertexlight;
	rbspface_t *in;
	msurface_t *out;
	mplane_t *pl;
	int facetype;

	int count;
	int surfnum;
	int shadernum;

	int fv;
	int j;

	mesh_t *mesh;

	int maxstyle = q3bsp_ignorestyles.ival?1:min(MAXRLIGHTMAPS, RBSP_STYLESPERSURF);


	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",mod->name);
		return false;
	}
	count = l->filelen / sizeof(*in);
	out = ZG_Malloc(&mod->memgroup, count*sizeof(*out));
	pl = ZG_Malloc(&mod->memgroup, count*sizeof(*pl));//create a new array of planes for speed.
	mesh = ZG_Malloc(&mod->memgroup, count*sizeof(*mesh));

	mod->surfaces = out;
	mod->numsurfaces = count;

	for (surfnum = 0; surfnum < count; surfnum++, out++, in++, pl++)
	{
		out->plane = pl;
		shadernum = LittleLong(in->shadernum);
		if (shadernum < 0 || shadernum >= mod->numtexinfo)
		{
			Con_Printf (CON_ERROR "CMod_LoadRFaces: bad shader index %s\n",mod->name);
			return false;
		}
		facetype = LittleLong(in->facetype);
		out->texinfo = mod->texinfo + shadernum;
		for (j = 0; j < maxstyle; j++)
		{
			out->lightmaptexturenums[j] = LittleLong(in->lightmapnum[j]);
			out->light_s[j] = LittleLong(in->lightmap_offs[0][j]);
			out->light_t[j] = LittleLong(in->lightmap_offs[1][j]);
			out->styles[j] = (in->lm_styles[j]!=255)?in->lm_styles[j]:INVALID_LIGHTSTYLE;
			out->vlstyles[j] = (in->vt_styles[j]!=255)?in->vt_styles[j]:INVALID_VLIGHTSTYLE;

			if (mod->lightmaps.count < out->lightmaptexturenums[j]+1)
				mod->lightmaps.count = out->lightmaptexturenums[j]+1;
		}
		for (; j < MAXRLIGHTMAPS; j++)
		{
			out->lightmaptexturenums[j] = -1;
			out->light_s[j] = 0;
			out->light_t[j] = 0;
			out->styles[j] = INVALID_LIGHTSTYLE;
			out->vlstyles[j] = INVALID_VLIGHTSTYLE;
		}
		for (;  j < MAXCPULIGHTMAPS; j++)
			out->styles[j] = INVALID_LIGHTSTYLE;
		if (facetype == MST_FLARE)
			out->texinfo = mod->texinfo + mod->numtexinfo*2;
		else if (out->lightmaptexturenums[0]<0 || r_vertexlight.value)
			out->texinfo += mod->numtexinfo;	//soup/vertex light uses a different version of the same shader (with all the lightmaps collapsed)

		out->lmshift = LMSHIFT_DEFAULT;
		out->extents[0] = (LittleLong(in->lightmap_width)-1)<<out->lmshift;
		out->extents[1] = (LittleLong(in->lightmap_height)-1)<<out->lmshift;
		out->samples=NULL;

		fv = LittleLong(in->firstvertex);
		{
			vec3_t v[3];
			VectorCopy(prv->verts[fv+0], v[0]);
			VectorCopy(prv->verts[fv+1], v[1]);
			VectorCopy(prv->verts[fv+2], v[2]);
			PlaneFromPoints(v, pl);
			CategorizePlane(pl);
		}

		if (prv->surfaces[shadernum].c.value == 0 || prv->surfaces[shadernum].c.value & Q3CONTENTS_TRANSLUCENT)
				//q3dm10's thingie is 0
			out->flags |= SURF_DRAWALPHA;

		if (mod->texinfo[shadernum].flags & TI_SKY)
			out->flags |= SURF_DRAWSKY|SURF_DRAWTILED;

		if (in->fognum < 0 || in->fognum >= mod->numfogs)
			out->fog = NULL;
		else
			out->fog = mod->fogs + in->fognum;

		if (prv->surfaces[shadernum].c.flags & (Q3SURF_NODRAW | Q3SURF_SKIP))
		{
			out->mesh = &mesh[surfnum];
			out->mesh->numindexes = 0;
			out->mesh->numvertexes = 0;
		}
		else if (facetype == MST_PATCH)
		{
			out->mesh = &mesh[surfnum];
			GL_SizePatch(out->mesh, LittleLong(in->patchwidth), LittleLong(in->patchheight), LittleLong(in->num_vertices), LittleLong(in->firstvertex), prv);
		}
		else if (facetype == MST_PATCH_FIXED)
		{
			out->mesh = &mesh[surfnum];
			GL_SizePatchFixed(out->mesh, LittleLong(in->patchwidth), LittleLong(in->patchheight), LittleLong(in->num_vertices), LittleLong(in->firstvertex), prv);
		}
		else if (facetype == MST_PLANAR || facetype == MST_TRIANGLE_SOUP)
		{
			out->mesh = &mesh[surfnum];
			out->mesh->numindexes = LittleLong(in->num_indexes);
			out->mesh->numvertexes = LittleLong(in->num_vertices);
/*
			Mod_AccumulateMeshTextureVectors(out->mesh);
*/
		}
		else
		{
			out->mesh = &mesh[surfnum];
			out->mesh->numindexes = 6;
			out->mesh->numvertexes = 4;
		}
	}
	
	Mod_SortShaders(mod);
	return true;
}
#endif
#endif

static qboolean CModQ3_LoadNodes (model_t *loadmodel, qbyte *mod_base, lump_t *l)
{
	int			i, j, count, p;
	q3dnode_t	*in;
	mnode_t 	*out;
	//dnode_t

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
		return false;
	}
	count = l->filelen / sizeof(*in);
	out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));

	if (count > SANITY_LIMIT(*out))
	{
		Con_Printf (CON_ERROR "Too many nodes on map\n");
		return false;
	}

	loadmodel->nodes = out;
	loadmodel->numnodes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleLong (in->mins[j]);
			out->minmaxs[3+j] = LittleLong (in->maxs[j]);
		}
		AddPointToBounds(out->minmaxs, loadmodel->mins, loadmodel->maxs);
		AddPointToBounds(out->minmaxs+3, loadmodel->mins, loadmodel->maxs);

		p = LittleLong(in->plane);
		out->plane = loadmodel->planes + p;

		out->firstsurface = 0;//LittleShort (in->firstface);
		out->numsurfaces = 0;//LittleShort (in->numfaces);

		out->contents = -1;

		for (j=0 ; j<2 ; j++)
		{
			p = LittleLong (in->children[j]);
			out->childnum[j] = p;
			if (p >= 0)
			{
				out->children[j] = loadmodel->nodes + p;
			}
			else
				out->children[j] = (mnode_t *)(loadmodel->leafs + (-1 - p));
		}
	}

	CMod_SetParent (loadmodel->nodes, NULL);	// sets nodes and leafs

	return true;
}

static qboolean CModQ3_LoadBrushes (model_t *mod, qbyte *mod_base, lump_t *l)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	q3dbrush_t	*in;
	q2cbrush_t	*out;
	int			i, count;
	int			shaderref;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count > SANITY_MAX_MAP_BRUSHES)
	{
		Con_Printf (CON_ERROR "Map has too many brushes");
		return false;
	}

	prv->brushes = ZG_Malloc(&mod->memgroup, sizeof(*out) * (count+1));

	out = prv->brushes;

	prv->numbrushes = count;

	for (i=0 ; i<count ; i++, out++, in++)
	{
		shaderref = LittleLong ( in->shadernum );
		out->contents = prv->surfaces[shaderref].c.value;
		out->brushside = &prv->brushsides[LittleLong ( in->firstside )];
		out->numsides = LittleLong ( in->num_sides );
		CM_FinalizeBrush(out);
	}

	return true;
}

static qboolean CModQ3_LoadLeafs (model_t *mod, qbyte *mod_base, lump_t *l)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	int			i, j;
	mleaf_t		*out;
	q3dleaf_t 	*in;
	int			count;
	q2cbrush_t	**brush;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count < 1)
	{
		Con_Printf (CON_ERROR "Map with no leafs\n");
		return false;
	}
	// need to save space for box planes

	if (count > SANITY_LIMIT(*out))
	{
		Con_Printf (CON_ERROR "Too many leaves on map");
		return false;
	}

	out = ZG_Malloc(&mod->memgroup, sizeof(*out) * (count+1));

	mod->leafs = out;
	mod->numleafs = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{
			out->minmaxs[0+j] = LittleLong(in->mins[j]);
			out->minmaxs[3+j] = LittleLong(in->maxs[j]);
		}
		out->cluster = LittleLong(in->cluster);
		out->area = LittleLong(in->area);
//		out->firstleafface = LittleLong(in->firstleafsurface);
//		out->numleaffaces = LittleLong(in->num_leafsurfaces);
		out->contents = 0;
		out->firstleafbrush = LittleLong(in->firstleafbrush);
		out->numleafbrushes = LittleLong(in->num_leafbrushes);

		out->firstmarksurface = mod->marksurfaces + LittleLong(in->firstleafsurface);
		out->nummarksurfaces = LittleLong(in->num_leafsurfaces);

		if (out->minmaxs[0] > out->minmaxs[3+0] || out->minmaxs[1] > out->minmaxs[3+1] ||
			out->minmaxs[2] > out->minmaxs[3+2])// || VectorEquals (out->minmaxs, out->minmaxs+3))
		{
			out->nummarksurfaces = 0;
		}

		brush = &prv->leafbrushes[out->firstleafbrush];
		for (j=0 ; j<out->numleafbrushes ; j++)
		{
			out->contents |= brush[j]->contents;
		}

		if (out->area >= prv->numareas)
		{
			prv->numareas = out->area + 1;
		}
	}

	return true;
}

static qboolean CModQ3_LoadPlanes (model_t *loadmodel, qbyte *mod_base, lump_t *l)
{
	int			i, j;
	mplane_t	*out;
	Q3PLANE_t 	*in;
	int			count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count > SANITY_LIMIT(*out))
	{
		Con_Printf (CON_ERROR "Too many planes on map (%i)\n", count);
		return false;
	}

	loadmodel->planes = out = ZG_Malloc(&loadmodel->memgroup, sizeof(*out) * count);
	loadmodel->numplanes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->normal[j] = LittleFloat (in->n[j]);
		}
		out->dist = LittleFloat (in->d);

		CategorizePlane(out);
	}

	return true;
}

static qboolean CModQ3_LoadLeafBrushes (model_t *mod, qbyte *mod_base, lump_t *l)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	int			i;
	q2cbrush_t  **out;
	int 	*in;
	int			count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count < 1)
	{
		Con_Printf (CON_ERROR "Map with no leafbrushes\n");
		return false;
	}
	// need to save space for box planes
	if (count > SANITY_MAX_MAP_LEAFBRUSHES)
	{
		Con_Printf (CON_ERROR "Map has too many leafbrushes\n");
		return false;
	}

	//prv->numbrushes is because of submodels being weird.
	out = prv->leafbrushes = ZG_Malloc(&mod->memgroup, sizeof(*out) * (count+prv->numbrushes));
	prv->numleafbrushes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
		*out = prv->brushes + (unsigned int)LittleLong (*in);

	return true;
}

static qboolean CModQ3_LoadBrushSides (model_t *mod, qbyte *mod_base, lump_t *l)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	int			i, j;
	q2cbrushside_t	*out;
	q3dbrushside_t 	*in;
	int			count;
	int			num;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	// need to save space for box planes
	if (count > SANITY_MAX_MAP_BRUSHSIDES)
	{
		Con_Printf (CON_ERROR "Map has too many brushsides (%i)\n", count);
		return false;
	}

	out = prv->brushsides = ZG_Malloc(&mod->memgroup, sizeof(*out) * count);
	prv->numbrushsides = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		num = LittleLong (in->planenum);
		out->plane = &mod->planes[num];
		j = LittleLong (in->texinfo);
		if (j >= mod->numtexinfo)
		{
			Con_Printf (CON_ERROR "Bad brushside texinfo\n");
			return false;
		}
		out->surface = &prv->surfaces[j];
	}

	return true;
}

#ifdef RFBSPS
static qboolean CModRBSP_LoadBrushSides (model_t *mod, qbyte *mod_base, lump_t *l)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	int			i, j;
	q2cbrushside_t	*out;
	rbspbrushside_t 	*in;
	int			count;
	int			num;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	// need to save space for box planes
	if (count > SANITY_MAX_MAP_BRUSHSIDES)
	{
		Con_Printf (CON_ERROR "Map has too many brushsides (%i)\n", count);
		return false;
	}

	out = prv->brushsides = ZG_Malloc(&mod->memgroup, sizeof(*out) * count);
	prv->numbrushsides = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		num = LittleLong (in->planenum);
		out->plane = &mod->planes[num];
		j = LittleLong (in->texinfo);
		if (j >= mod->numtexinfo)
		{
			Con_Printf (CON_ERROR "Bad brushside texinfo\n");
			return false;
		}
		out->surface = &prv->surfaces[j];
	}

	return true;
}
#endif

static qboolean CModQ3_LoadVisibility (model_t *mod, qbyte *mod_base, lump_t *l)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	unsigned int numclusters;
	if (l->filelen == 0)
	{
		int i;
#if 0
		//the 'correct' code
		numclusters = 0;
		for (i = 0; i < mod->numleafs; i++)
			if (numclusters < mod->leafs[i].cluster+1)
				numclusters = mod->leafs[i].cluster+1;

		numclusters++;
#else
		//but its much faster to merge all leafs into a single pvs cluster. no vis is no vis.
		numclusters = 8*sizeof(int);
		for (i = 0; i < mod->numleafs; i++)
			mod->leafs[i].cluster = !!mod->leafs[i].cluster;
#endif
		prv->q3pvs = ZG_Malloc(&mod->memgroup, sizeof(*prv->q3pvs) + (numclusters+7)/8 * numclusters);
		memset (prv->q3pvs, 0xff, sizeof(*prv->q3pvs) + (numclusters+7)/8 * numclusters);
		prv->q3pvs->numclusters = numclusters;
		prv->numvisibility = 0;
		prv->q3pvs->rowsize = (prv->q3pvs->numclusters+7)/8;
	}
	else
	{
		prv->numvisibility = l->filelen;

		prv->q3pvs = ZG_Malloc(&mod->memgroup, l->filelen);
		mod->vis = (q2dvis_t *)prv->q3pvs;
		memcpy (prv->q3pvs, mod_base + l->fileofs, l->filelen);

		numclusters = prv->q3pvs->numclusters = LittleLong (prv->q3pvs->numclusters);
		prv->q3pvs->rowsize = LittleLong (prv->q3pvs->rowsize);
	}
	mod->numclusters = numclusters;
	mod->pvsbytes = ((mod->numclusters + 31)>>3)&~3;

	return true;
}

#ifndef SERVERONLY
static void CModQ3_LoadLighting (model_t *loadmodel, qbyte *mod_base, lump_t *l)
{
	qbyte *in = mod_base + l->fileofs;
	qbyte *out;
	unsigned int samples = l->filelen;
	int m, s, t;
	int mapstride = loadmodel->lightmaps.width*3;
	int mapsize = mapstride*loadmodel->lightmaps.height;
	int maps;
	int merge;
	int mergestride;

	extern cvar_t gl_overbright;
	float scale = (1<<(2-gl_overbright.ival));

	loadmodel->lightmaps.fmt = LM_RGB8;
	loadmodel->lightmaps.prebaked = PTI_RGB8;

	//round up the samples, in case the last one is partial.
	maps = ((samples+mapsize-1)&~(mapsize-1)) / mapsize;

	//q3 maps have built in 4-fold overbright.
	//if we're not rendering with that, we need to brighten the lightmaps in order to keep the darker parts the same brightness. we loose the 2 upper bits. those bright areas become uniform and indistinct.
	gl_overbright.flags |= CVAR_RENDERERLATCH;
	BuildLightMapGammaTable(1, (1<<(2-gl_overbright.ival)));

	loadmodel->lightmaps.mergew = 0;
	loadmodel->lightmaps.mergeh = 0;

	loadmodel->engineflags |= MDLF_NEEDOVERBRIGHT;

	if (!samples)
		return;

	if (loadmodel->lightmaps.deluxemapping)
		maps /= 2;

	{
		int limitw = sh_config.texture2d_maxsize / loadmodel->lightmaps.width;
		int limith = sh_config.texture2d_maxsize / loadmodel->lightmaps.height;
		if (!q3bsp_mergeq3lightmaps.ival)
		{
			limitw = 1;
			limith = 1;
		}
		loadmodel->lightmaps.mergeh = loadmodel->lightmaps.mergew = 1;
		while (loadmodel->lightmaps.mergew*loadmodel->lightmaps.mergeh < maps)
		{	//this could probably be smarter.
			if (loadmodel->lightmaps.mergew*2 <= limitw && loadmodel->lightmaps.mergew < loadmodel->lightmaps.mergeh)
				loadmodel->lightmaps.mergew *= 2;
			else if (loadmodel->lightmaps.mergeh*2 <= limith)
				loadmodel->lightmaps.mergeh *= 2;
			else if (loadmodel->lightmaps.mergew*2 <= limitw)
				loadmodel->lightmaps.mergew *= 2;
			else
				break;	//can't expand in either direction.
		}
	}
	merge = loadmodel->lightmaps.mergew*loadmodel->lightmaps.mergeh;
	mergestride = loadmodel->lightmaps.mergew*mapstride;

	//q3bsp itself does not support deluxemapping.
	//the way it works is by interleaving the data in lightmap+deluxemap pairs.
	//the surface data makes no references to the deluxemap maps, they're implied by lightmap+1
	//if no surface references an odd lightmap index then we know we have deluxemaps... assuming there are at least two lightmaps.
	//q3map2 likes generating null lightmaps, so beware false positives.

	//note that external lighting makes this even more fun.

	//if we have deluxemapping data then we split it here. beware externals.
	if (loadmodel->lightmaps.deluxemapping)
	{
		m = merge;
		while (m < maps)
			m += merge;
		loadmodel->lightdata = ZG_Malloc(&loadmodel->memgroup, mapsize*m*2);
		loadmodel->lightdatasize = mapsize*m*2;
	}
	else
	{
		m = merge;
		while (m < maps)
			m += merge;
		loadmodel->lightdata = ZG_Malloc(&loadmodel->memgroup, mapsize*m);
		loadmodel->lightdatasize = mapsize*m;
	}

	if (!loadmodel->lightdata)
		return;


	//be careful here, q3bsp deluxemapping is done using interleaving. we want to unoverbright ONLY lightmaps and not deluxemaps.
	for (m = 0; m < maps; m++)
	{
		out = loadmodel->lightdata;
		//figure out which merged lightmap we're putting it into
		out += (m/merge)*merge*mapsize * (loadmodel->lightmaps.deluxemapping?2:1);
		//and the submap
		s = m%merge;
		t = s/loadmodel->lightmaps.mergew;
		s = s%loadmodel->lightmaps.mergew;
		out += s*mapstride;
		out += t*mergestride*loadmodel->lightmaps.height;

		//q3bsp has 4-fold overbrights, so if we're not using overbrights then we basically need to scale the values up by 4
		//this will require clamping, which can result in oversaturation of channels, meaning discolouration
		for (t = 0; t < loadmodel->lightmaps.height; t++)
		{
			for (s = 0; s < loadmodel->lightmaps.width; s++)
			{
				float i;
				vec3_t l;
				l[0] = *in++;
				l[1] = *in++;
				l[2] = *in++;
				VectorScale(l, scale, l);		//it should be noted that this maths is wrong if you're trying to use srgb lightmaps.
				i = max(l[0], max(l[1], l[2]));
				if (i > 255)
					VectorScale(l, 255/i, l);	//clamp the brightest channel, scaling the others down to retain chromiance.
				*out++ = l[0];
				*out++ = l[1];
				*out++ = l[2];
			}
			out += mergestride-mapstride;
		}

		if (r_lightmap_saturation.value != 1.0f)
			SaturateR8G8B8(out, mapsize, r_lightmap_saturation.value);
		
		if (loadmodel->lightmaps.deluxemapping)
		{
			out -= mergestride*loadmodel->lightmaps.height;
			out += merge*mapsize;

			//no gamma for deluxemap
			for (t = 0; t < loadmodel->lightmaps.height; t++)
			{
				for (s = 0; s < loadmodel->lightmaps.width; s++)
				{
					*out++ = in[0];
					*out++ = in[1];
					*out++ = in[2];
					in += 3;
				}
				out += mergestride-mapstride;
			}
		}
	}
	/*for (; m%merge; m++)
	{
		out = loadmodel->lightdata;
		//figure out which merged lightmap we're putting it into
		out += (m/merge)*merge*mapsize * (loadmodel->lightmaps.deluxemapping?2:1);
		//and the submap
		out += (m%merge)*mapsize;

		for(s = 0; s < mapsize; s+=3)
		{
			out[s+0] = 0;
			out[s+1] = 255;
			out[s+2] = 0;
		}
	}*/
}

static qboolean CModQ3_LoadLightgrid (model_t *loadmodel, qbyte *mod_base, lump_t *l)
{
	dq3gridlight_t 	*in;
	dq3gridlight_t 	*out;
	q3lightgridinfo_t *grid;
	int	count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
		return false;
	}
	count = l->filelen / sizeof(*in);
	grid = ZG_Malloc(&loadmodel->memgroup, sizeof(q3lightgridinfo_t) + count*sizeof(*out));
	grid->lightgrid = (dq3gridlight_t*)(grid+1);
	out = grid->lightgrid;

	loadmodel->lightgrid = grid;
	grid->numlightgridelems = count;

	// lightgrid is all 8 bit
	memcpy ( out, in, count*sizeof(*out) );

	return true;
}

#ifdef RFBSPS
static qboolean CModRBSP_LoadLightgrid (model_t *loadmodel, qbyte *mod_base, lump_t *elements, lump_t *indexes)
{
	unsigned short	*iin;
	rbspgridlight_t	*ein;
	unsigned short	*iout;
	rbspgridlight_t	*eout;
	q3lightgridinfo_t *grid;
	int	ecount;
	int icount;

	int i;

	ein = (void *)(mod_base + elements->fileofs);
	iin = (void *)(mod_base + indexes->fileofs);
	if (indexes->filelen % sizeof(*iin) || elements->filelen % sizeof(*ein))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
		return false;
	}
	icount = indexes->filelen / sizeof(*iin);
	ecount = elements->filelen / sizeof(*ein);

	grid = ZG_Malloc(&loadmodel->memgroup, sizeof(q3lightgridinfo_t) + ecount*sizeof(*eout) + icount*sizeof(*iout));
	grid->rbspelements = (rbspgridlight_t*)((char *)grid + sizeof(q3lightgridinfo_t));
	grid->rbspindexes = (unsigned short*)((char *)grid + sizeof(q3lightgridinfo_t) + ecount*sizeof(*eout));
	eout = grid->rbspelements;
	iout = grid->rbspindexes;

	loadmodel->lightgrid = grid;

	grid->numlightgridelems = icount;

	// elements are all 8 bit
	memcpy ( eout, ein, ecount*sizeof(*eout) );

	for (i = 0; i < icount; i++)
		iout[i] = LittleShort(iin[i]);

	return true;
}
#endif
#endif
#endif

#if !defined(SERVERONLY) && (defined(Q2BSPS) || defined(Q3BSPS))
#ifdef IMAGEFMT_PCX
qbyte *ReadPCXPalette(qbyte *buf, int len, qbyte *out);
static int CM_GetQ2Palette (void)
{
	char *f;
	size_t sz;
	sz = FS_LoadFile("pics/colormap.pcx", (void**)&f);
	if (!f)
	{
		Con_Printf (CON_WARNING "Couldn't find pics/colormap.pcx\n");
		return -1;
	}
	if (!ReadPCXPalette(f, sz, q2_palette))
	{
		Con_Printf (CON_WARNING "Couldn't read pics/colormap.pcx\n");
		FS_FreeFile(f);
		return -1;
	}

	FS_FreeFile(f);


#if 0
	{
		float	inf;
		qbyte	palette[768];
		qbyte *pal;
		int		i;

		pal = q2_palette;

		for (i=0 ; i<768 ; i++)
		{
			inf = ((pal[i]+1)/256.0)*255 + 0.5;
			if (inf < 0)
				inf = 0;
			if (inf > 255)
				inf = 255;
			palette[i] = inf;
		}

		memcpy (q2_palette, palette, sizeof(palette));
	}
#endif
	return 0;
}
#endif
#endif

#if 0
static void CM_OpenAllPortals(model_t *mod, char *ents)	//this is a compleate hack. About as compleate as possible.
{	//q2 levels contain a thingie called area portals. Basically, doors can seperate two areas and
	//the engine knows when this portal is open, and weather to send ents from both sides of the door
	//or not. It's not just ents, but also walls. We want to just open them by default and hope the
	//progs knows how to close them.

	char style[8];
	char name[64];

	if (!map_autoopenportals.value)
		return;

	while(*ents)
	{
		if (*ents == '{')	//an entity
		{
			ents++;
			*style = '\0';
			*name = '\0';
			while (*ents)
			{
				ents = COM_Parse(ents);

				if (!strcmp(com_token, "classname"))
				{
					ents = COM_ParseOut(ents, name, sizeof(name));
				}
				else if (!strcmp(com_token, "style"))
				{
					ents = COM_ParseOut(ents, style, sizeof(style));
				}
				else if (*com_token == '}')
					break;
				else
					ents = COM_Parse(ents);	//other field
				ents++;
			}

			if (!strcmp(name, "func_areaportal"))
			{
				CMQ2_SetAreaPortalState(mod, atoi(style), true);
			}
		}

		ents++;
	}
}
#endif


#if defined(Q3BSPS)
static void CalcClusterPHS(cminfo_t	*prv, int cluster)
{
	int j, k, l, index;
	int bitbyte;
	unsigned int *dest, *src;
	qbyte *scan;

	int numclusters = prv->q3pvs->numclusters;
	int rowbytes = prv->q3pvs->rowsize;
	int rowwords = rowbytes / sizeof(int);

	scan = (qbyte *)prv->q3pvs->data;
	dest = (unsigned int *)(prv->q3phs->data);

	dest += rowwords*cluster;
	scan += rowbytes*cluster;
	for (j=0 ; j<rowbytes ; j++)
	{
		bitbyte = scan[j];
		if (!bitbyte)
			continue;
		for (k=0 ; k<8 ; k++)
		{
			if (! (bitbyte & (1<<k)) )
				continue;
			// OR this pvs row into the phs
			index = (j<<3) + k;
			if (index >= numclusters)
			{
//				if (!buggytools)
//					Con_Printf ("CM_CalcPHS: Bad bit(s) in PVS (%i >= %i)\n", index, numclusters);	// pad bits should be 0
//				buggytools = true;
			}
			else
			{
				src = (unsigned int *)(prv->q3pvs->data) + index*rowwords;
				for (l=0 ; l<rowwords ; l++)
					dest[l] |= src[l];
			}
		}
	}
	prv->phscalced[cluster>>3] |= 1<<(cluster&7);
}
#endif
#if defined(HAVE_SERVER) && defined(Q3BSPS)
static void CMQ3_CalcPHS (model_t *mod)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	int		rowbytes, rowwords;
	int		i, j, k, l, index;
	int		bitbyte;
	unsigned int	*dest, *src;
	qbyte	*scan;
	int		count, vcount;
	int		numclusters;
	qboolean buggytools = false;
	extern cvar_t sv_calcphs;

	Con_DPrintf ("Building PHS...\n");

	prv->q3phs = ZG_Malloc(&mod->memgroup, sizeof(*prv->q3phs) + prv->q3pvs->rowsize * prv->q3pvs->numclusters);

	rowwords = prv->q3pvs->rowsize / sizeof(int);
	rowbytes = prv->q3pvs->rowsize;

	memset ( prv->q3phs, 0, sizeof(*prv->q3phs) + prv->q3pvs->rowsize * prv->q3pvs->numclusters );

	prv->q3phs->rowsize = prv->q3pvs->rowsize;
	prv->q3phs->numclusters = numclusters = prv->q3pvs->numclusters;
	if (!numclusters)
		return;

	vcount = 0;
	for (i=0 ; i<numclusters ; i++)
	{
		scan = CM_ClusterPVS (mod, i, NULL, PVM_FAST);
		for (j=0 ; j<numclusters ; j++)
		{
			if ( scan[j>>3] & (1<<(j&7)) )
			{
				vcount++;
			}
		}
	}

	count = 0;
	scan = (qbyte *)prv->q3pvs->data;
	dest = (unsigned int *)(prv->q3phs->data);

	if (sv_calcphs.ival >= 2)
	{	//delay-calculate it.
		prv->phscalced = ZG_Malloc(&mod->memgroup, (prv->q3pvs->numclusters+7)/8);
		memcpy(dest, scan, rowbytes*numclusters);
		Con_DPrintf ("Average clusters visible / total: %i / %i\n"
			, vcount/numclusters, numclusters);
	}
	else if (!sv_calcphs.ival)
	{	//disable calcs (behaves like broadcast so just fill with 1s)
		memset(dest, 0xff, rowbytes*numclusters);
	}
	else
	{	//the original slow logic like q3.
		for (i=0 ; i<numclusters ; i++, dest += rowwords, scan += rowbytes)
		{
			memcpy (dest, scan, rowbytes);
			for (j=0 ; j<rowbytes ; j++)
			{
				bitbyte = scan[j];
				if (!bitbyte)
					continue;
				for (k=0 ; k<8 ; k++)
				{
					if (! (bitbyte & (1<<k)) )
						continue;
					// OR this pvs row into the phs
					index = (j<<3) + k;
					if (index >= numclusters)
					{
						if (!buggytools)
							Con_Printf ("CM_CalcPHS: Bad bit(s) in PVS (%i >= %i)\n", index, numclusters);	// pad bits should be 0
						buggytools = true;
					}
					else
					{
						src = (unsigned int *)(prv->q3pvs->data) + index*rowwords;
						for (l=0 ; l<rowwords ; l++)
							dest[l] |= src[l];
					}
				}
			}
			for (j=0 ; j<numclusters ; j++)
				if ( ((qbyte *)dest)[j>>3] & (1<<(j&7)) )
					count++;
		}

		Con_DPrintf ("Average clusters visible / hearable / total: %i / %i / %i\n"
			, vcount/numclusters, count/numclusters, numclusters);
	}
}
#endif

/*
static qbyte *CM_LeafnumPVS (model_t *model, int leafnum, qbyte *buffer, unsigned int buffersize)
{
	return CM_ClusterPVS(model, CM_LeafCluster(model, leafnum), buffer, buffersize);
}
*/

#ifndef SERVERONLY
#define GLQ2BSP_LightPointValues GLQ1BSP_LightPointValues

extern int	r_dlightframecount;
static void Q2BSP_MarkLights (dlight_t *light, dlightbitmask_t bit, mnode_t *node)
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
		Q2BSP_MarkLights (light, bit, node->children[0]);
		return;
	}
	if (dist < -light->radius)
	{
		Q2BSP_MarkLights (light, bit, node->children[1]);
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

	Q2BSP_MarkLights (light, bit, node->children[0]);
	Q2BSP_MarkLights (light, bit, node->children[1]);
}

#ifndef SERVERONLY
static void GLR_Q2BSP_StainNode_r (model_t *model, mnode_t *node, float *parms)
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
		GLR_Q2BSP_StainNode_r (model, node->children[0], parms);
		return;
	}
	if (dist < (-*parms))
	{
		GLR_Q2BSP_StainNode_r (model, node->children[1], parms);
		return;
	}

// mark the polygons
	surf = model->surfaces + node->firstsurface;
	for (i=0 ; i<node->numsurfaces ; i++, surf++)
	{
		if (surf->flags&~(SURF_DONTWARP|SURF_PLANEBACK))
			continue;
		Surf_StainSurf(model, surf, parms);
	}

	GLR_Q2BSP_StainNode_r (model, node->children[0], parms);
	GLR_Q2BSP_StainNode_r (model, node->children[1], parms);
}
static void GLR_Q2BSP_StainNode (model_t *model, float *parms)
{
	GLR_Q2BSP_StainNode_r(model, model->rootnode, parms);
}
#endif

#endif

static void CM_BuildBIH(model_t *mod, int submodel)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	cmodel_t	*sub = &prv->cmodels[submodel];

	struct bihleaf_s *bihleaf, *l;
	size_t bihleafs, i;

	//undo any bih damage on the copy
	mod->cnodes = NULL;
	mod->funcs.NativeTrace			= CM_NativeTrace;
	mod->funcs.NativeContents		= CM_NativeContents;
	mod->funcs.PointContents		= Q2BSP_PointContents;
	if (!q3bsp_bihtraces.ival)
		return;	//skip this. fall back on other stuff.

	if (mod->fromgame != fg_quake3)
		return;

	bihleafs = sub->num_brushes;
	for (i = 0; i < sub->num_patches; i++)
		bihleafs += prv->patches[sub->firstpatch + i].numfacets;
	for (i = 0; i < sub->num_cmeshes; i++)
		bihleafs += prv->cmeshes[sub->firstcmesh + i].numincidies/3;
	bihleaf = l = BZ_Malloc(sizeof(*bihleaf)*bihleafs);

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
#ifdef Q3BSPS
	for (i = 0; i < sub->num_patches; i++)
	{
		q3cpatch_t *p = &prv->patches[sub->firstpatch+i];
		size_t j;
		for (j = 0; j < p->numfacets; j++)
		{
			q2cbrush_t *b = &p->facets[j];
			l->type = BIH_PATCHBRUSH;
			l->data.patchbrush = b;
			l->data.contents = b->contents;
			VectorCopy(b->absmins, l->mins);
			VectorCopy(b->absmaxs, l->maxs);
			l++;
		}
	}
#endif
#ifdef Q3BSPS
	for (i = 0; i < sub->num_cmeshes; i++)
	{
		q3cmesh_t *m = &prv->cmeshes[sub->firstcmesh+i];
		size_t j;
		for (j = 0; j+2 < m->numincidies; j+=3)
		{
			index_t *v = m->indicies+j;
			vec_t *v1 = m->xyz_array[v[0]], *v2 = m->xyz_array[v[1]], *v3 = m->xyz_array[v[2]];

			l->type = BIH_TRIANGLE;
			l->data.tri.xyz = m->xyz_array;
			l->data.tri.indexes = v;

			l->data.contents = m->surface->c.value;
			VectorCopy(v1, l->mins);
			VectorCopy(v1, l->maxs);
			AddPointToBounds(v2, l->mins, l->maxs);
			AddPointToBounds(v3, l->mins, l->maxs);
			l++;
		}
	}
#endif

	BIH_Build(mod, bihleaf, l-bihleaf);
	BZ_Free(bihleaf);
}

#ifdef AVAIL_ZLIB
#include <zlib.h>	//for crc32.
#endif
/*
==================
CM_LoadMap

Loads in the map and all submodels
==================
*/
static cmodel_t *CM_LoadMap (model_t *mod, qbyte *filein, size_t filelen, qboolean clientload)
{
	unsigned		*buf;
	int				i;
	q2dheader_t		header;
	int				length;
	qboolean noerrors = true;
	model_t			*wmod = mod;
	char			loadname[32];
	qbyte			*mod_base = (qbyte *)filein;
	bspx_header_t	*bspx = NULL;
	unsigned int	checksum1, checksum2;
#ifdef Q3BSPS
	extern cvar_t	gl_overbright;
#endif

#ifndef SERVERONLY
	void (*buildmeshes)(model_t *mod, msurface_t *surf, builddata_t *cookie) = NULL;
	qbyte *facedata = NULL;
	unsigned int facesize = 0;
#endif
	cminfo_t	*prv;
	qboolean isbig;

	COM_FileBase (mod->name, loadname, sizeof(loadname));

	// free old stuff
	mod->meshinfo = prv = ZG_Malloc(&mod->memgroup, sizeof(*prv));
	prv->numcmodels = 0;
	prv->numvisibility = 0;

	mod->type = mod_brush;

	if (!mod->name[0])
	{
		prv->cmodels = ZG_Malloc(&mod->memgroup, 1 * sizeof(*prv->cmodels));
		mod->leafs = ZG_Malloc(&mod->memgroup, 1 * sizeof(*mod->leafs));
		mod->funcs.AreasConnected		= CM_AreasConnected;
		prv->numcmodels = 1;
		prv->numareas = 1;
		mod->checksum = mod->checksum2 = 0;
		prv->cmodels[0].headnode = (mnode_t*)mod->leafs;	//directly start with the empty leaf
		return &prv->cmodels[0];			// cinematic servers won't have anything at all
	}

	//
	// load the file
	//
	buf = (unsigned	*)filein;
	length = filelen;
	if (!buf)
	{
		Con_Printf (CON_ERROR "Couldn't load %s\n", mod->name);
		return NULL;
	}

	checksum1 = LittleLong (CalcHashInt(&hash_md4, buf, length));
#ifdef AVAIL_ZLIB
	checksum2 = crc32(0, (void*)buf, length);	//q2rerelease uses crc32 instead... *sigh*
#else
	checksum2 = checksum1;	//we accept either, so wimp out.
#endif

	header = *(q2dheader_t *)(buf);
	header.ident = LittleLong(header.ident);
	header.version = LittleLong(header.version);

	ClearBounds(mod->mins, mod->maxs);

	switch(header.version)
	{
	default:
		Con_Printf (CON_ERROR "Quake 2 or Quake 3 based BSP with unknown header (%s: %i should be %i or %i)\n"
			, mod->name, header.version, BSPVERSION_Q2, BSPVERSION_Q3);
		return NULL;
		break;
#ifdef Q3BSPS
#ifdef RFBSPS
	case BSPVERSION_RBSP: //rbsp/fbsp
#endif
	case BSPVERSION_RTCW:	//rtcw
	case BSPVERSION_Q3:
#ifdef RFBSPS
		if (header.ident == (('F'<<0)+('B'<<8)+('S'<<16)+('P'<<24)))
		{
			mod->lightmaps.width = 512;
			mod->lightmaps.height = 512;
		}
		else
#endif
		{
			mod->lightmaps.width = 128;
			mod->lightmaps.height = 128;
		}

		prv->mapisq3 = true;
		mod->fromgame = fg_quake3;
		for (i=0 ; i<Q3LUMPS_TOTAL ; i++)
		{
#ifdef RFBSPS
			if (i == RBSPLUMP_LIGHTINDEXES && header.version != BSPVERSION_RBSP)
			{
				header.lumps[i].filelen = 0;
				header.lumps[i].fileofs = 0;
			}
			else
#endif
			{
				header.lumps[i].filelen = LittleLong (header.lumps[i].filelen);
				header.lumps[i].fileofs = LittleLong (header.lumps[i].fileofs);

				if (header.lumps[i].filelen && header.lumps[i].fileofs + header.lumps[i].filelen > filelen)
				{
					Con_Printf (CON_ERROR "WARNING: q3bsp %s truncated (lump %i, %i+%i > %u)\n", mod->name, i, header.lumps[i].fileofs, header.lumps[i].filelen, (unsigned int)filelen);
					header.lumps[i].filelen = filelen - header.lumps[i].fileofs;
					if (header.lumps[i].filelen < 0)
						header.lumps[i].filelen = 0;
				}
			}
		}
		/*
		#ifndef SERVERONLY
			GLMod_LoadVertexes		(mod, cmod_base, &header.lumps[Q3LUMP_DRAWVERTS]);
//			GLMod_LoadEdges			(mod, cmod_base, &header.lumps[Q3LUMP_EDGES]);
//			GLMod_LoadSurfedges		(mod, cmod_base, &header.lumps[Q3LUMP_SURFEDGES]);
			GLMod_LoadLighting		(mod, cmod_base, &header.lumps[Q3LUMP_LIGHTMAPS]);
		#endif
			CModQ3_LoadShaders		(mod, cmod_base, &header.lumps[Q3LUMP_SHADERS]);
			CModQ3_LoadPlanes		(mod, cmod_base, &header.lumps[Q3LUMP_PLANES]);
			CModQ3_LoadLeafBrushes	(mod, cmod_base, &header.lumps[Q3LUMP_LEAFBRUSHES]);
			CModQ3_LoadBrushes		(mod, cmod_base, &header.lumps[Q3LUMP_BRUSHES]);
			CModQ3_LoadBrushSides	(mod, cmod_base, &header.lumps[Q3LUMP_BRUSHSIDES]);
		#ifndef SERVERONLY
			CMod_LoadTexInfo		(mod, cmod_base, &header.lumps[Q3LUMP_SHADERS]);
			CMod_LoadFaces			(mod, cmod_base, &header.lumps[Q3LUMP_SURFACES]);
//			GLMod_LoadMarksurfaces	(mod, cmod_base, &header.lumps[Q3LUMP_LEAFFACES]);
		#endif
			CMod_LoadVisibility		(mod, cmod_base, &header.lumps[Q3LUMP_VISIBILITY]);
			CModQ3_LoadSubmodels	(mod, cmod_base, &header.lumps[Q3LUMP_MODELS]);
			CModQ3_LoadLeafs		(mod, cmod_base, &header.lumps[Q3LUMP_LEAFS]);
			CModQ3_LoadNodes		(mod, cmod_base, &header.lumps[Q3LUMP_NODES]);
//			CMod_LoadAreas			(mod, cmod_base, &header.lumps[Q3LUMP_AREAS]);
//			CMod_LoadAreaPortals	(mod, cmod_base, &header.lumps[Q3LUMP_AREAPORTALS]);
			CMod_LoadEntityString	(mod, cmod_base, &header.lumps[Q3LUMP_ENTITIES]);
*/

		prv->faces = NULL;

		bspx = BSPX_Setup(mod, mod_base, filelen, header.lumps, Q3LUMPS_TOTAL);

		//q3 maps have built in 4-fold overbright.
		//if we're not rendering with that, we need to brighten the lightmaps in order to keep the darker parts the same brightness. we loose the 2 upper bits. those bright areas become uniform and indistinct.
		//this is used for both the lightmap AND vertex lighting
		//FIXME: when not using overbrights, we suffer a loss of precision.
		gl_overbright.flags |= CVAR_RENDERERLATCH;
		BuildLightMapGammaTable(1, (1<<(2-gl_overbright.ival)));

		prv->mapisq3 = true;
		noerrors = noerrors && CModQ3_LoadShaders				(mod, mod_base, &header.lumps[Q3LUMP_SHADERS]);
		noerrors = noerrors && CModQ3_LoadPlanes				(mod, mod_base, &header.lumps[Q3LUMP_PLANES]);
#ifdef RFBSPS
		if (header.version == BSPVERSION_RBSP)
		{
			noerrors = noerrors && CModRBSP_LoadBrushSides		(mod, mod_base, &header.lumps[Q3LUMP_BRUSHSIDES]);
			noerrors = noerrors && CModRBSP_LoadVertexes		(mod, mod_base, &header.lumps[Q3LUMP_DRAWVERTS]);
		}
		else
#endif
		{
			noerrors = noerrors && CModQ3_LoadBrushSides		(mod, mod_base, &header.lumps[Q3LUMP_BRUSHSIDES]);
			noerrors = noerrors && CModQ3_LoadVertexes			(mod, mod_base, &header.lumps[Q3LUMP_DRAWVERTS]);
		}
		noerrors = noerrors && CModQ3_LoadBrushes				(mod, mod_base, &header.lumps[Q3LUMP_BRUSHES]);
		noerrors = noerrors && CModQ3_LoadLeafBrushes			(mod, mod_base, &header.lumps[Q3LUMP_LEAFBRUSHES]);
#ifdef RFBSPS
		if (header.version == BSPVERSION_RBSP)
			noerrors = noerrors && CModRBSP_LoadFaces			(mod, mod_base, &header.lumps[Q3LUMP_SURFACES]);
		else
#endif
			noerrors = noerrors && CModQ3_LoadFaces				(mod, mod_base, &header.lumps[Q3LUMP_SURFACES]);

		if (noerrors)
			Mod_LoadEntities								(mod, mod_base, &header.lumps[Q3LUMP_ENTITIES]);
#ifndef SERVERONLY
		if (qrenderer != QR_NONE)
		{
#ifdef RFBSPS
			if (header.version == BSPVERSION_RBSP)
				noerrors = noerrors && CModRBSP_LoadLightgrid	(mod, mod_base, &header.lumps[Q3LUMP_LIGHTGRID], &header.lumps[RBSPLUMP_LIGHTINDEXES]);
			else
#endif
				noerrors = noerrors && CModQ3_LoadLightgrid		(mod, mod_base, &header.lumps[Q3LUMP_LIGHTGRID]);
			noerrors = noerrors && CModQ3_LoadIndexes			(mod, mod_base, &header.lumps[Q3LUMP_DRAWINDEXES]);

			if (header.version != BSPVERSION_RTCW)
				noerrors = noerrors && CModQ3_LoadFogs			(mod, mod_base, &header.lumps[Q3LUMP_FOGS]);
			else
				mod->numfogs = 0;

			facedata = (void *)(mod_base + header.lumps[Q3LUMP_SURFACES].fileofs);
#ifdef RFBSPS
			if (header.version == BSPVERSION_RBSP)
			{
				noerrors = noerrors && CModRBSP_LoadRFaces		(mod, mod_base, &header.lumps[Q3LUMP_SURFACES]);
				buildmeshes = CModRBSP_BuildSurfMesh;
				facesize = sizeof(rbspface_t);
				mod->lightmaps.surfstyles = 4;
			}
			else
#endif
			{
				noerrors = noerrors && CModQ3_LoadRFaces		(mod, mod_base, &header.lumps[Q3LUMP_SURFACES]);
				buildmeshes = CModQ3_BuildSurfMesh;
				facesize = sizeof(q3dface_t);
				mod->lightmaps.surfstyles = 1;
			}
			if (noerrors)
			{
				i = header.lumps[Q3LUMP_LIGHTMAPS].filelen / (mod->lightmaps.width*mod->lightmaps.height*3);
				mod->lightmaps.deluxemapping = !(i&1);
				mod->lightmaps.count = max(mod->lightmaps.count, i);
				mod->lightmaps.deluxemapping_modelspace = true;	//we assume true for q3bsp.

				for (i = 0; i < mod->numsurfaces && mod->lightmaps.deluxemapping; i++)
				{
					if (mod->surfaces[i].lightmaptexturenums[0] >= 0 && (mod->surfaces[i].lightmaptexturenums[0] & 1))
						mod->lightmaps.deluxemapping = false;
				}

				{
					char deluxeMaps[64], *key;
					key = (char*)Mod_ParseWorldspawnKey(mod, "deluxeMaps", deluxeMaps, sizeof(deluxeMaps));
					if (*key)
					{
						switch(atoi(key))
						{
						case 0:
							mod->lightmaps.deluxemapping = false;
							break;
						case 1:
		//					mod->lightmaps.deluxemapping = true;
							mod->lightmaps.deluxemapping_modelspace = true;
							break;
						case 2:
		//					mod->lightmaps.deluxemapping = true;
							mod->lightmaps.deluxemapping_modelspace = false;
							break;
						}
					}
				}
			}

			if (noerrors)
				CModQ3_LoadLighting								(mod, mod_base, &header.lumps[Q3LUMP_LIGHTMAPS]);	//fixme: duplicated loading.
		}
#endif
		noerrors = noerrors && CModQ3_LoadMarksurfaces			(mod, mod_base, &header.lumps[Q3LUMP_LEAFSURFACES]);
		noerrors = noerrors && CModQ3_LoadLeafs					(mod, mod_base, &header.lumps[Q3LUMP_LEAFS]);
		noerrors = noerrors && CModQ3_LoadNodes					(mod, mod_base, &header.lumps[Q3LUMP_NODES]);
		noerrors = noerrors && CModQ3_LoadSubmodels				(mod, mod_base, &header.lumps[Q3LUMP_MODELS]);
		noerrors = noerrors && CModQ3_LoadVisibility			(mod, mod_base, &header.lumps[Q3LUMP_VISIBILITY]);

		if (!noerrors)
		{
			if (prv->faces)
				BZ_Free(prv->faces);
			return NULL;
		}

#ifdef HAVE_SERVER
		mod->funcs.FatPVS				= Q23BSP_FatPVS;
		mod->funcs.EdictInFatPVS		= Q23BSP_EdictInFatPVS;
		mod->funcs.FindTouchedLeafs		= Q23BSP_FindTouchedLeafs;
#endif
		mod->funcs.ClusterPVS			= CM_ClusterPVS;
		mod->funcs.ClusterPHS			= CM_ClusterPHS;
		mod->funcs.ClusterForPoint		= CM_PointCluster;

#ifdef HAVE_CLIENT
		mod->funcs.LightPointValues		= GLQ3_LightGrid;
		mod->funcs.StainNode			= GLR_Q2BSP_StainNode;
		mod->funcs.MarkLights			= Q2BSP_MarkLights;
		mod->funcs.PrepareFrame			= CM_PrepareFrame;
#ifdef RTLIGHTS
		mod->funcs.GenerateShadowMesh	= Q3BSP_GenerateShadowMesh;
#endif
#endif
		mod->funcs.PointContents		= Q2BSP_PointContents;
		mod->funcs.NativeTrace			= CM_NativeTrace;
		mod->funcs.NativeContents		= CM_NativeContents;

		mod->funcs.InfoForPoint			= CM_InfoForPoint;
		mod->funcs.AreasConnected		= CM_AreasConnected;
		mod->funcs.SetAreaPortalState	= CM_SetAreaPortalState;
		mod->funcs.WriteAreaBits		= CM_WriteAreaBits;
		mod->funcs.LoadAreaPortalBlob	= CM_LoadAreaPortalBlob;
		mod->funcs.SaveAreaPortalBlob	= CM_SaveAreaPortalBlob;

#ifdef HAVE_CLIENT
		//light grid info
		if (mod->lightgrid)
		{
			char gridsize[256], *key;
			char val[64];
			float maxs;
			q3lightgridinfo_t *lg = mod->lightgrid;
			key = (char*)Mod_ParseWorldspawnKey(mod, "gridsize", gridsize, sizeof(gridsize));

			key = COM_ParseOut(key, val, sizeof(val));
			lg->gridSize[0] = atof(val);
			key = COM_ParseOut(key, val, sizeof(val));
			lg->gridSize[1] = atof(val);
			key = COM_ParseOut(key, val, sizeof(val));
			lg->gridSize[2] = atof(val);

			if ( lg->gridSize[0] < 1 || lg->gridSize[1] < 1 || lg->gridSize[2] < 1 )
			{
				lg->gridSize[0] = 64;
				lg->gridSize[1] = 64;
				lg->gridSize[2] = 128;
			}

			for ( i = 0; i < 3; i++ )
			{
				lg->gridMins[i] = lg->gridSize[i] * ceil( (prv->cmodels->mins[i] + 1) / lg->gridSize[i] );
				maxs = lg->gridSize[i] * floor( (prv->cmodels->maxs[i] - 1) / lg->gridSize[i] );
				lg->gridBounds[i] = (maxs - lg->gridMins[i])/lg->gridSize[i] + 1;
			}

			lg->gridBounds[3] = lg->gridBounds[1] * lg->gridBounds[0];
		}
#endif

		if (!CM_CreatePatchesForLeafs (mod, prv))	//for clipping
		{
			BZ_Free(prv->faces);
			return NULL;
		}
#ifdef HAVE_SERVER
		CMQ3_CalcPHS(mod);
#endif
//			BZ_Free(map_verts);
		BZ_Free(prv->faces);
		break;
#endif
#ifdef Q2BSPS
	case BSPVERSION_Q2:
	case BSPVERSION_Q2W:
		isbig = *mod_base == 'Q';	//'qbism'

		mod->lightmaps.width = LMBLOCK_SIZE_MAX;
		mod->lightmaps.height = LMBLOCK_SIZE_MAX;

		prv->mapisq3 = false;
		mod->engineflags |= MDLF_NEEDOVERBRIGHT;
		for (i=0 ; i<Q2HEADER_LUMPS ; i++)
		{
			header.lumps[i].filelen = LittleLong (header.lumps[i].filelen);
			header.lumps[i].fileofs = LittleLong (header.lumps[i].fileofs);
		}
		if (header.version == BSPVERSION_Q2W)
		{
			header.lumps[i].filelen = LittleLong (header.lumps[i].filelen);
			header.lumps[i].fileofs = LittleLong (header.lumps[i].fileofs);
			i++;
		}
		bspx = BSPX_Setup(mod, mod_base, filelen, header.lumps, i);

#if defined(HAVE_CLIENT) && defined(IMAGEFMT_PCX)
		if (CM_GetQ2Palette())
			memcpy(q2_palette, host_basepal, 768);
#endif


#ifdef HAVE_SERVER
		mod->funcs.FatPVS				= Q23BSP_FatPVS;
		mod->funcs.EdictInFatPVS		= Q23BSP_EdictInFatPVS;
		mod->funcs.FindTouchedLeafs		= Q23BSP_FindTouchedLeafs;
#endif
		mod->funcs.LightPointValues		= NULL;
		mod->funcs.StainNode			= NULL;
		mod->funcs.MarkLights			= NULL;
		mod->funcs.ClusterPVS			= CM_ClusterPVS;
		mod->funcs.ClusterPHS			= CM_ClusterPHS;
		mod->funcs.ClusterForPoint		= CM_PointCluster;
		mod->funcs.PointContents		= Q2BSP_PointContents;
		mod->funcs.NativeTrace			= CM_NativeTrace;
		mod->funcs.NativeContents		= CM_NativeContents;

		mod->funcs.InfoForPoint			= CM_InfoForPoint;
		mod->funcs.AreasConnected		= CM_AreasConnected;
		mod->funcs.SetAreaPortalState	= CM_SetAreaPortalState;
		mod->funcs.WriteAreaBits		= CM_WriteAreaBits;
		mod->funcs.LoadAreaPortalBlob	= CM_LoadAreaPortalBlob;
		mod->funcs.SaveAreaPortalBlob	= CM_SaveAreaPortalBlob;
		mod->funcs.PrepareFrame			= NULL;

		switch(qrenderer)
		{
		case QR_NONE:	//dedicated only
			noerrors = noerrors && CModQ2_LoadSurfaces		(mod, mod_base, &header.lumps[Q2LUMP_TEXINFO]);
			noerrors = noerrors && CModQ2_LoadPlanes		(mod, mod_base, &header.lumps[Q2LUMP_PLANES]);
			noerrors = noerrors && CModQ2_LoadVisibility	(mod, mod_base, &header.lumps[Q2LUMP_VISIBILITY]);
			noerrors = noerrors && CModQ2_LoadBrushSides	(mod, mod_base, &header.lumps[Q2LUMP_BRUSHSIDES], isbig);
			noerrors = noerrors && CModQ2_LoadBrushes		(mod, mod_base, &header.lumps[Q2LUMP_BRUSHES]);
			noerrors = noerrors && CModQ2_LoadLeafBrushes	(mod, mod_base, &header.lumps[Q2LUMP_LEAFBRUSHES], isbig);
			noerrors = noerrors && CModQ2_LoadLeafs			(mod, mod_base, &header.lumps[Q2LUMP_LEAFS], isbig);
			noerrors = noerrors && CModQ2_LoadNodes			(mod, mod_base, &header.lumps[Q2LUMP_NODES], isbig);
			noerrors = noerrors && CModQ2_LoadSubmodels		(mod, mod_base, &header.lumps[Q2LUMP_MODELS]);
			noerrors = noerrors && CModQ2_LoadAreas			(mod, mod_base, &header.lumps[Q2LUMP_AREAS]);
			noerrors = noerrors && CModQ2_LoadAreaPortals	(mod, mod_base, &header.lumps[Q2LUMP_AREAPORTALS]);
			if (noerrors)
				Mod_LoadEntities							(mod, mod_base, &header.lumps[Q2LUMP_ENTITIES]);
			break;
#ifdef HAVE_CLIENT
		default:
			// load into heap
			noerrors = noerrors && Mod_LoadVertexes			(mod, mod_base, &header.lumps[Q2LUMP_VERTEXES]);
			noerrors = noerrors && Mod_LoadEdges			(mod, mod_base, &header.lumps[Q2LUMP_EDGES], isbig?sb_long2:sb_none);
			noerrors = noerrors && Mod_LoadSurfedges		(mod, mod_base, &header.lumps[Q2LUMP_SURFEDGES]);
			noerrors = noerrors && CModQ2_LoadSurfaces		(mod, mod_base, &header.lumps[Q2LUMP_TEXINFO]);
			noerrors = noerrors && CModQ2_LoadPlanes		(mod, mod_base, &header.lumps[Q2LUMP_PLANES]);
			noerrors = noerrors && CModQ2_LoadTexInfo		(mod, mod_base, &header.lumps[Q2LUMP_TEXINFO], loadname);
			if (noerrors)
				Mod_LoadEntities							(mod, mod_base, &header.lumps[Q2LUMP_ENTITIES]);
			noerrors = noerrors && CModQ2_LoadFaces			(mod, mod_base, &header.lumps[Q2LUMP_FACES], &header.lumps[Q2LUMP_LIGHTING], header.version == BSPVERSION_Q2W, bspx, isbig);
								   Mod_LoadVertexNormals(mod, bspx, mod_base, (header.version == BSPVERSION_Q2W)?&header.lumps[19]:NULL);
			noerrors = noerrors && Mod_LoadMarksurfaces		(mod, mod_base, &header.lumps[Q2LUMP_LEAFFACES], isbig?sb_long2:sb_none);
			noerrors = noerrors && CModQ2_LoadVisibility	(mod, mod_base, &header.lumps[Q2LUMP_VISIBILITY]);
			noerrors = noerrors && CModQ2_LoadBrushSides	(mod, mod_base, &header.lumps[Q2LUMP_BRUSHSIDES], isbig);
			noerrors = noerrors && CModQ2_LoadBrushes		(mod, mod_base, &header.lumps[Q2LUMP_BRUSHES]);
			noerrors = noerrors && CModQ2_LoadLeafBrushes	(mod, mod_base, &header.lumps[Q2LUMP_LEAFBRUSHES], isbig);
			noerrors = noerrors && CModQ2_LoadLeafs			(mod, mod_base, &header.lumps[Q2LUMP_LEAFS], isbig);
			noerrors = noerrors && CModQ2_LoadNodes			(mod, mod_base, &header.lumps[Q2LUMP_NODES], isbig);
			noerrors = noerrors && CModQ2_LoadSubmodels		(mod, mod_base, &header.lumps[Q2LUMP_MODELS]);
			noerrors = noerrors && CModQ2_LoadAreas			(mod, mod_base, &header.lumps[Q2LUMP_AREAS]);
			noerrors = noerrors && CModQ2_LoadAreaPortals	(mod, mod_base, &header.lumps[Q2LUMP_AREAPORTALS]);

			if (!noerrors)
			{
				return NULL;
			}
			mod->funcs.LightPointValues		= GLQ2BSP_LightPointValues;
			mod->funcs.StainNode			= GLR_Q2BSP_StainNode;
			mod->funcs.MarkLights			= Q2BSP_MarkLights;
			mod->funcs.PrepareFrame			= CM_PrepareFrame;
#ifdef RTLIGHTS
			mod->funcs.GenerateShadowMesh	= Q2BSP_GenerateShadowMesh;
#endif
			break;
#endif
		}
#endif
	}

	BSPX_LoadEnvmaps(mod, bspx, mod_base);

#ifdef Q3BSPS
	{
		int x, y;
		for (x = 0; x < prv->numareas; x++)
			for (y = 0; y < prv->numareas; y++)
				prv->q3areas[x].numareaportals[y] = map_autoopenportals.ival;
	}
#endif
#ifdef Q2BSPS
	if (map_autoopenportals.value)
		memset (prv->q2portalopen, 1, sizeof(prv->q2portalopen));	//open them all. Used for progs that havn't got a clue.
	else
		memset (prv->q2portalopen, 0, sizeof(prv->q2portalopen));	//make them start closed.
#endif
	FloodAreaConnections (prv);

	mod->checksum = checksum1;
	mod->checksum2 = checksum2;

	mod->nummodelsurfaces = mod->numsurfaces;
	memset(&mod->batches, 0, sizeof(mod->batches));
	mod->vbos = NULL;

	mod->numsubmodels = CM_NumInlineModels(mod);

	mod->hulls[0].firstclipnode = prv->cmodels[0].headnode-mod->nodes;
	mod->rootnode = prv->cmodels[0].headnode;
	mod->nummodelsurfaces = prv->cmodels[0].numsurfaces;

#ifdef HAVE_CLIENT
	prv->oldclusters[0] = prv->oldclusters[1] = -1;
	if (qrenderer != QR_NONE)
	{
		builddata_t *bd = NULL;
		if (buildmeshes)
		{
			bd = Z_Malloc(sizeof(*bd) + facesize*mod->nummodelsurfaces);
			bd->buildfunc = buildmeshes;
			memcpy(bd+1, facedata + mod->firstmodelsurface*facesize, facesize*mod->nummodelsurfaces);
		}
		COM_AddWork(WG_MAIN, ModBrush_LoadGLStuff, mod, bd, 0, 0);
	}
#endif

	//FIXME: q2bsp apparently doesn't report which brushes are part of which submodels.
	//FIXME: ALL patches? not just worldmodel?
	CM_BuildBIH(mod, 0);

	for (i=1 ; i< mod->numsubmodels ; i++)
	{
		cmodel_t	*bm;

		char	name[MAX_QPATH];

		Q_snprintfz (name, sizeof(name), "*%i:%s", i, wmod->publicname);
		mod = Mod_FindName (name);
		*mod = *wmod;
		mod->archive = NULL;
		mod->entities_raw = NULL;
		mod->submodelof = wmod;
		Q_strncpyz(mod->publicname, name, sizeof(mod->publicname));
		Q_snprintfz (mod->name, sizeof(mod->name), "*%i:%s", i, wmod->name);
		memset(&mod->memgroup, 0, sizeof(mod->memgroup));

		bm = CM_InlineModel (wmod, name);
		
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

		CM_BuildBIH(mod, i);

		memset(&mod->batches, 0, sizeof(mod->batches));
		mod->vbos = NULL;

		VectorCopy (bm->maxs, mod->maxs);
		VectorCopy (bm->mins, mod->mins);
#ifndef SERVERONLY
		mod->radius = RadiusFromBounds (mod->mins, mod->maxs);

		if (qrenderer != QR_NONE)
		{
			builddata_t *bd = NULL;
			if (buildmeshes)
			{
				bd = Z_Malloc(sizeof(*bd) + facesize*mod->nummodelsurfaces);
				bd->buildfunc = buildmeshes;
				memcpy(bd+1, facedata + mod->firstmodelsurface*facesize, facesize*mod->nummodelsurfaces);
			}
			COM_AddWork(WG_MAIN, ModBrush_LoadGLStuff, mod, bd, i, 0);
		}
#endif
		COM_AddWork(WG_MAIN, Mod_ModelLoaded, mod, NULL, MLS_LOADED, 0);
	}

#ifdef TERRAIN
	wmod->terrain = Mod_LoadTerrainInfo(wmod, loadname, false);
#endif

	return &prv->cmodels[0];
}

/*
==================
CM_InlineModel
==================
*/
static cmodel_t	*CM_InlineModel (model_t *model, char *name)
{
	cminfo_t	*prv = (cminfo_t*)model->meshinfo;
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

static int		CM_NumInlineModels (model_t *model)
{
	cminfo_t	*prv = (cminfo_t*)model->meshinfo;
	return prv->numcmodels;
}

static int		CM_LeafContents (model_t *model, int leafnum)
{
	if (leafnum < 0 || leafnum >= model->numleafs)
		Host_Error ("CM_LeafContents: bad number");
	return model->leafs[leafnum].contents;
}

static int		CM_LeafCluster (model_t *model, int leafnum)
{
	if (leafnum < 0 || leafnum >= model->numleafs)
		Host_Error ("CM_LeafCluster: bad number");
	return model->leafs[leafnum].cluster;
}

static int		CM_LeafArea (model_t *model, int leafnum)
{
	if (leafnum < 0 || leafnum >= model->numleafs)
		Host_Error ("CM_LeafArea: bad number");
	return model->leafs[leafnum].area;
}

//=======================================================================

#define PlaneDiff(point,plane) (((plane)->type < 3 ? (point)[(plane)->type] : DotProduct((point), (plane)->normal)) - (plane)->dist)
#define PlaneDiffPush(point,plane,pushdist) (((plane)->type < 3 ? (point)[(plane)->type] : DotProduct((point), (plane)->normal)) - (plane)->dist - pushdist)

static mplane_t			box_planes[6];
static model_t			box_model;
static q2cbrush_t		box_brush;
static q2cbrushside_t	box_sides[6];
static qboolean BM_NativeTrace(model_t *model, int forcehullnum, const framestate_t *framestate, const vec3_t axis[3], const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, qboolean capsule, unsigned int contents, trace_t *trace);
static unsigned int BM_NativeContents(struct model_s *model, int hulloverride, const framestate_t *framestate, const vec3_t axis[3], const vec3_t p, const vec3_t mins, const vec3_t maxs)
{
	unsigned int j;
	q2cbrushside_t *brushside = box_sides;
	for ( j = 0; j < 6; j++, brushside++ )
	{
		if ( PlaneDiff (p, brushside->plane) > 0 )
			return 0;
	}
	return FTECONTENTS_BODY;
}

/*
===================
CM_InitBoxHull

Set up the planes and nodes so that the six floats of a bounding box
can just be stored out and get a proper clipping hull structure.
===================
*/
static void CM_InitBoxHull (void)
{
	int			i;
	mplane_t	*p;
	q2cbrushside_t	*s;

	box_model.funcs.NativeContents		= BM_NativeContents;
	box_model.funcs.NativeTrace			= BM_NativeTrace;


	box_model.loadstate = MLS_LOADED;

	box_brush.contents = FTECONTENTS_BODY;
	box_brush.numsides = 6;
	box_brush.brushside = box_sides;

	for (i=0 ; i<6 ; i++)
	{
		//the pointers
		s = &box_sides[i];
		p = &box_planes[i];

		// brush sides
		s->plane = 	p;
		s->surface = &nullsurface;

		// planes
		p->type = ((i>=3)?i-3:i);
		p->signbits = 0;
		VectorClear (p->normal);
		p->normal[p->type] = ((i>=3)?-1:1);
	}
}


/*
===================
CM_HeadnodeForBox

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
===================
*/
static void CM_SetTempboxSize (const vec3_t mins, const vec3_t maxs)
{
	box_planes[0].dist = maxs[0];
	box_planes[1].dist = maxs[1];
	box_planes[2].dist = maxs[2];
	box_planes[3].dist = -mins[0];
	box_planes[4].dist = -mins[1];
	box_planes[5].dist = -mins[2];
}

model_t *CM_TempBoxModel(const vec3_t mins, const vec3_t maxs)
{
	CM_SetTempboxSize(mins, maxs);
	return &box_model;
}

/*
==================
CM_PointLeafnum_r

==================
*/
static int CM_PointLeafnum_r (model_t *mod, const vec3_t p, int num)
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

#ifdef HAVE_SERVER
static int CM_PointLeafnum (model_t *mod, const vec3_t p)
{
	if (!mod || mod->loadstate != MLS_LOADED)
		return 0;		// sound may call this without map loaded
	return CM_PointLeafnum_r (mod, p, 0);
}
#endif

static int CM_PointCluster (model_t *mod, const vec3_t p, int *area)
{
	int leaf;
	if (!mod || mod->loadstate != MLS_LOADED)
		return 0;		// sound may call this without map loaded

	leaf = CM_PointLeafnum_r (mod, p, 0);
	if (area)
		*area = CM_LeafArea(mod, leaf);
	return CM_LeafCluster(mod, leaf);
}

static int CM_PointContents (model_t *mod, const vec3_t p);
static void CM_InfoForPoint (struct model_s *mod, vec3_t pos, int *area, int *cluster, unsigned int *contentbits)
{
	int leaf = CM_PointLeafnum_r (mod, pos, 0);

	*area = CM_LeafArea(mod, leaf);
	*cluster = CM_LeafCluster(mod, leaf);
	*contentbits = CM_LeafContents(mod, leaf);

	//q3 needs to use brush contents (its leafs no longer need to strictly follow brushes)
	if (mod->fromgame != fg_quake2)
		*contentbits = CM_PointContents (mod, pos);
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

static void CM_BoxLeafnums_r (model_t *mod, int nodenum)
{
	mplane_t	*plane;
	mnode_t		*node;
	int		s;

	while (1)
	{
		if (nodenum < 0)
		{
			if (leaf_count >= leaf_maxcount)
			{
//				Com_Printf ("CM_BoxLeafnums_r: overflow\n");
				return;
			}
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
			CM_BoxLeafnums_r (mod, node->childnum[0]);
			nodenum = node->childnum[1];
		}

	}
}

static int	CM_BoxLeafnums_headnode (model_t *mod, const vec3_t mins, const vec3_t maxs, int *list, int listsize, int headnode, int *topnode)
{
	leaf_list = list;
	leaf_count = 0;
	leaf_maxcount = listsize;
	leaf_mins = mins;
	leaf_maxs = maxs;

	leaf_topnode = -1;

	CM_BoxLeafnums_r (mod, headnode);

	if (topnode)
		*topnode = leaf_topnode;

	return leaf_count;
}

static int	CM_BoxLeafnums (model_t *mod, const vec3_t mins, const vec3_t maxs, int *list, int listsize, int *topnode)
{
	return CM_BoxLeafnums_headnode (mod, mins, maxs, list,
		listsize, mod->hulls[0].firstclipnode, topnode);
}



/*
==================
CM_PointContents

==================
*/
static int CM_PointContents (model_t *mod, const vec3_t p)
{
	cminfo_t		*prv = (cminfo_t*)mod->meshinfo;
	int				i, j, contents;
	mleaf_t			*leaf;
	q2cbrush_t		*brush;
	q2cbrushside_t	*brushside;

	if (!mod)	// map not loaded
		return 0;

	i = CM_PointLeafnum_r (mod, p, mod->hulls[0].firstclipnode);

	if (mod->fromgame == fg_quake2)
		contents = mod->leafs[i].contents;	//q2 is simple.
	else
	{
		leaf = &mod->leafs[i];

	//	if ( leaf->contents & CONTENTS_NODROP ) {
	//		contents = CONTENTS_NODROP;
	//	} else {
			contents = 0;
	//	}

		for (i = 0; i < leaf->numleafbrushes; i++)
		{
			brush = prv->leafbrushes[leaf->firstleafbrush + i];

			// check if brush actually adds something to contents
			if ( (contents & brush->contents) == brush->contents ) {
				continue;
			}

			brushside = brush->brushside;
			for ( j = 0; j < brush->numsides; j++, brushside++ )
			{
				if ( PlaneDiff (p, brushside->plane) > 0 )
					break;
			}

			if (j == brush->numsides)
				contents |= brush->contents;
		}
	}

#ifdef TERRAIN
	if (mod->terrain)
		contents |= Heightmap_PointContents(mod, NULL, p);
#endif
	return contents;
}

static unsigned int CM_NativeContents(struct model_s *model, int hulloverride, const framestate_t *framestate, const vec3_t axis[3], const vec3_t point, const vec3_t mins, const vec3_t maxs)
{
	cminfo_t	*prv = (cminfo_t*)model->meshinfo;
	int	contents;
	if (!DotProduct(mins, mins) && !DotProduct(maxs, maxs))
		return CM_PointContents(model, point);

	if (!model)	// map not loaded
		return 0;


	{
		int i, j, k;
		mleaf_t			*leaf;
		q2cbrush_t		*brush;
		q2cbrushside_t	*brushside;
		vec3_t absmin, absmax, ofs;

		int leaflist[64];

		VectorAdd(point, mins, absmin);
		VectorAdd(point, maxs, absmax);

		k = CM_BoxLeafnums (model, absmin, absmax, leaflist, 64, NULL);

		contents = 0;
		for (k--; k >= 0; k--)
		{
			leaf = &model->leafs[leaflist[k]];
			if (model->fromgame != fg_quake2)
			{	//q3 is more complex
				for (i = 0; i < leaf->numleafbrushes; i++)
				{
					brush = prv->leafbrushes[leaf->firstleafbrush + i];

					// check if brush actually adds something to contents
					if ( (contents & brush->contents) == brush->contents ) {
						continue;
					}


					brushside = brush->brushside;
					for ( j = 0; j < brush->numsides; j++, brushside++ )
					{
						for (j=0 ; j<3 ; j++)
						{
							if (brushside->plane->normal[j] < 0)
								ofs[j] = maxs[j];
							else
								ofs[j] = mins[j];
						}
						if (PlaneDiffPush (point, brushside->plane, DotProduct (ofs, brushside->plane->normal)) > 0)
							break;
					}

					if (j == brush->numsides)
						contents |= brush->contents;
				}
			}
			else	//q2 is simple
				contents |= leaf->contents;
		}
	}

	return contents;
}

/*
===============================================================================

BOX TRACING

===============================================================================
*/

// 1/32 epsilon to keep floating point happy
#define	DIST_EPSILON	(0.03125)

static vec3_t	trace_start, trace_end;
static vec3_t	trace_mins, trace_maxs;
static vec3_t	trace_extents;
static vec3_t	trace_absmins, trace_absmaxs;
static vec3_t	trace_up;	//capsule points upwards in this direction
static vec3_t	trace_capsulesize;	//radius, up, down
static float	trace_truefraction;
static float	trace_nearfraction;

static trace_t	trace_trace;
static int		trace_contents;
static enum
{
	shape_isbox,
	shape_iscapsule,
	shape_ispoint
} trace_shape;		// optimized case


static void CM_FinalizeBrush(q2cbrush_t *brush)
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
			j = Fragment_ClipPlaneToBrush(verts, countof(verts), planes, sizeof(planes[0]), brush->numsides, planes[i]);
			while (j-- > 0)
				AddPointToBounds(verts[j], brush->absmins, brush->absmaxs);
		}
	}
}

/*
================
CM_ClipBoxToBrush
================
*/
static void CM_ClipBoxToBrush (vec3_t mins, vec3_t maxs, vec3_t p1, vec3_t p2,
					  trace_t *trace, q2cbrush_t *brush)
{
	int			i, j;
	mplane_t	*plane, *clipplane;
	float		dist;
	float		enterfrac, leavefrac;
	vec3_t		ofs;
	float		d1, d2;
	qboolean	getout, startout;
	float		f;
	q2cbrushside_t	*side, *leadside;

	float nearfrac=0;
	enterfrac = -1;
	leavefrac = 2;
	clipplane = NULL;

	if (!brush->numsides)
		return;

	getout = false;
	startout = false;
	leadside = NULL;

	for (i=0 ; i<brush->numsides ; i++)
	{
		side = brush->brushside+i;
		plane = side->plane;

		switch(trace_shape)
		{
		default:
		case shape_isbox: // general box case
			// push the plane out apropriately for mins/maxs

			// FIXME: use signbits into 8 way lookup for each mins/maxs
			for (j=0 ; j<3 ; j++)
			{
				if (plane->normal[j] < 0)
					ofs[j] = maxs[j];
				else
					ofs[j] = mins[j];
			}
			dist = DotProduct (ofs, plane->normal);
			dist = plane->dist - dist;
			break;
		capsuledist(dist,plane,mins,maxs)
		case shape_ispoint: // special point case
			dist = plane->dist;
			break;
		}

		d1 = DotProduct (p1, plane->normal) - dist;
		d2 = DotProduct (p2, plane->normal) - dist;

		if (d2 > 0)
			getout = true;	// endpoint is not in solid
		if (d1 > 0)
			startout = true;

		// if completely in front of face, no intersection
		if (d1 > 0 && d2 >= d1)
			return;

		if (d1 <= 0 && d2 <= 0)
			continue;

		// crosses face
		if (d1 > d2)
		{	// enter
			f = (d1) / (d1-d2);
			if (f > enterfrac)
			{
				enterfrac = f;
				nearfrac = (d1-DIST_EPSILON) / (d1-d2);
				clipplane = plane;
				leadside = side;
			}
		}
		else
		{	// leave
			f = (d1) / (d1-d2);
			if (f < leavefrac)
				leavefrac = f;
		}
	}

	if (!startout)
	{	// original point was inside brush
		trace->startsolid = true;
		if (!getout)
			trace->allsolid = true;
		return;
	}
	if (enterfrac <= leavefrac)
	{
		if (enterfrac > -1 && enterfrac <= trace_truefraction)
		{
			if (enterfrac < 0)
				enterfrac = 0;

			trace_nearfraction = nearfrac;
			trace_truefraction = enterfrac;

			trace->plane.dist = clipplane->dist;
			VectorCopy(clipplane->normal, trace->plane.normal);
			trace->surface = &(leadside->surface->c);
			trace->contents = brush->contents;
		}
	}
}

#ifdef Q3BSPS
static void CM_ClipBoxToPlanes (vec3_t trmins, vec3_t trmaxs, vec3_t p1, vec3_t p2, trace_t *trace, vec3_t plmins, vec3_t plmaxs, mplane_t *plane, int numplanes, q2csurface_t *surf)
{
	int			i, j;
	mplane_t	*clipplane;
	float		dist;
	float		enterfrac, leavefrac;
	vec3_t		ofs;
	float		d1, d2;
	qboolean	getout, startout;
	float		f;
//	q2cbrushside_t	*side, *leadside;
	static mplane_t	bboxplanes[6] = //we change the dist, but nothing else
	{
		{{1, 0, 0}},
		{{0, 1, 0}},
		{{0, 0, 1}},
		{{-1, 0, 0}},
		{{0, -1, 0}},
		{{0, 0, -1}},
	};

	float nearfrac=0;
	enterfrac = -1;
	leavefrac = 2;
	clipplane = NULL;

	getout = false;
	startout = false;
//	leadside = NULL;

	for (i=0 ; i<numplanes ; i++, plane++)
	{
		switch(trace_shape)
		{
		default:
		case shape_isbox:	// general box case
			// push the plane out apropriately for mins/maxs

			// FIXME: special case for axial
			// FIXME: use signbits into 8 way lookup for each mins/maxs
			for (j=0 ; j<3 ; j++)
			{
				if (plane->normal[j] < 0)
					ofs[j] = trmaxs[j];
				else
					ofs[j] = trmins[j];
			}
			dist = DotProduct (ofs, plane->normal);
			dist = plane->dist - dist;
			break;
		capsuledist(dist,plane,trmins,trmaxs)
		case shape_ispoint:	// special point case
			dist = plane->dist;
			break;
		}

		d1 = DotProduct (p1, plane->normal) - dist;
		d2 = DotProduct (p2, plane->normal) - dist;

		if (d2 > 0)
			getout = true;	// endpoint is not in solid
		if (d1 > 0)
			startout = true;

		// if completely in front of face, no intersection
		if (d1 > 0 && d2 >= d1)
			return;

		if (d1 <= 0 && d2 <= 0)
			continue;

		// crosses face
		if (d1 > d2)
		{	// enter
			f = (d1) / (d1-d2);
			if (f > enterfrac)
			{
				enterfrac = f;
				nearfrac = (d1-DIST_EPSILON) / (d1-d2);
				clipplane = plane;
//				leadside = side;
			}
		}
		else
		{	// leave
			f = (d1) / (d1-d2);
			if (f < leavefrac)
				leavefrac = f;
		}
	}

	//bevel the brush axially (to match the player's bbox), in case that wasn't already done
	for (i=0, plane = bboxplanes; i<6 ; i++, plane++)
	{
		if (i < 3)
		{	//positive normal
			dist = trmins[i];
			plane->dist = plmaxs[i];
			dist = plane->dist - dist;
			d1 = p1[i] - dist;
			d2 = p2[i] - dist;
		}
		else
		{	//negative normal
			j = i-3;
			dist = -trmaxs[j];
			plane->dist = -plmins[j];
			dist = plane->dist - dist;
			d1 = -p1[j] - dist;
			d2 = -p2[j] - dist;
		}

		if (d2 > 0)
			getout = true;	// endpoint is not in solid
		if (d1 > 0)
			startout = true;

		// if completely in front of face, no intersection
		if (d1 > 0 && d2 >= d1)
			return;

		if (d1 <= 0 && d2 <= 0)
			continue;

		// crosses face
		if (d1 > d2)
		{	// enter
			f = (d1) / (d1-d2);
			if (f > enterfrac)
			{
				enterfrac = f;
				nearfrac = (d1-DIST_EPSILON) / (d1-d2);
				clipplane = plane;
//				leadside = side;
			}
		}
		else
		{	// leave
			f = (d1) / (d1-d2);
			if (f < leavefrac)
				leavefrac = f;
		}
	}

	if (!startout)
	{	// original point was inside brush
		trace->startsolid = true;
		if (!getout)
			trace->allsolid = true;
		return;
	}
	if (enterfrac <= leavefrac)
	{
		if (enterfrac > -1 && enterfrac <= trace_truefraction)
		{
			if (enterfrac < 0)
				enterfrac = 0;

			trace_nearfraction = nearfrac;
			trace_truefraction = enterfrac;

			trace->plane.dist = clipplane->dist;
			VectorCopy(clipplane->normal, trace->plane.normal);
			trace->surface = surf;
			trace->contents = surf->value;
		}
	}
}

static void Mod_Trace_Trisoup_(vecV_t *posedata, index_t *indexes, size_t numindexes, vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, trace_t *trace, q2csurface_t *surf)
{
	size_t i;
	int j;
	float *p1, *p2, *p3;
	vec3_t edge1, edge2, edge3;
	mplane_t planes[5];
	vec3_t tmins, tmaxs;

	for (i = 0; i < numindexes; i+=3)
	{
		p1 = posedata[indexes[i+0]];
		p2 = posedata[indexes[i+1]];
		p3 = posedata[indexes[i+2]];

		//determine the triangle extents, and skip the triangle if we're completely out of bounds
		for (j = 0; j < 3; j++)
		{
			tmins[j] = p1[j];
			if (tmins[j] > p2[j])
				tmins[j] = p2[j];
			if (tmins[j] > p3[j])
				tmins[j] = p3[j];
			if (trace_absmaxs[j]+(1/8.f) < tmins[j])
				break;
			tmaxs[j] = p1[j];
			if (tmaxs[j] < p2[j])
				tmaxs[j] = p2[j];
			if (tmaxs[j] < p3[j])
				tmaxs[j] = p3[j];
			if (trace_absmins[j]-(1/8.f) > tmaxs[j])
				break;
		}
		//skip any triangles which are completely outside the trace bounds
		if (j < 3)
			continue;

		VectorSubtract(p1, p2, edge1);
		VectorSubtract(p3, p2, edge2);
		VectorSubtract(p1, p3, edge3);
		CrossProduct(edge1, edge2, planes[0].normal);
		VectorNormalize(planes[0].normal);
		planes[0].dist = DotProduct(p1, planes[0].normal);
		VectorNegate(planes[0].normal, planes[1].normal);
		planes[1].dist = -planes[0].dist + 4;

		//determine edges
		//FIXME: use adjacency info
		CrossProduct(edge1, planes[0].normal, planes[2].normal);
		VectorNormalize(planes[2].normal);
		planes[2].dist = DotProduct(p2, planes[2].normal);

		CrossProduct(planes[0].normal, edge2, planes[3].normal);
		VectorNormalize(planes[3].normal);
		planes[3].dist = DotProduct(p3, planes[3].normal);

		CrossProduct(planes[0].normal, edge3, planes[4].normal);
		VectorNormalize(planes[4].normal);
		planes[4].dist = DotProduct(p1, planes[4].normal);

		CM_ClipBoxToPlanes(mins, maxs, start, end, trace, tmins, tmaxs, planes, 5, surf); 
	}
}

/*
static void CM_ClipBoxToMesh (vec3_t mins, vec3_t maxs, vec3_t p1, vec3_t p2, trace_t *trace, mesh_t *mesh)
{
	trace_truefraction = trace->truefraction;
	trace_nearfraction = trace->fraction;
	Mod_Trace_Trisoup_(mesh->xyz_array, mesh->indexes, mesh->numindexes, p1, p2, mins, maxs, trace, &nullsurface.c);
	trace->truefraction = trace_truefraction;
	trace->fraction = trace_nearfraction;
}
*/

static void CM_ClipBoxToPatch (vec3_t mins, vec3_t maxs, vec3_t p1, vec3_t p2,
					  trace_t *trace, q2cbrush_t *brush)
{
	int			i, j;
	mplane_t	*plane, *clipplane;
	float		enterfrac, leavefrac, nearfrac = 0;
	vec3_t		ofs;
	float		d1, d2;
	float dist;
	qboolean	startout;
	float		f;
	q2cbrushside_t	*side, *leadside;

	if (!brush->numsides)
		return;

	enterfrac = -1;
	leavefrac = 2;
	clipplane = NULL;
	startout = false;
	leadside = NULL;

	for (i=0 ; i<brush->numsides ; i++)
	{
		side = brush->brushside+i;
		plane = side->plane;

		// push the plane out apropriately for mins/maxs
		switch(trace_shape)
		{
		default:
		case shape_isbox:	// general box case
			// FIXME: use signbits into 8 way lookup for each mins/maxs
			for (j=0 ; j<3 ; j++)
			{
				if (plane->normal[j] < 0)
					ofs[j] = maxs[j];
				else
					ofs[j] = mins[j];
			}
			dist = DotProduct (ofs, plane->normal);
			dist = plane->dist - dist;
			break;
		capsuledist(dist,plane,mins,maxs)
		case shape_ispoint:	// special point case
			dist = plane->dist;
			break;
		}

		d1 = DotProduct (p1, plane->normal) - dist;
		d2 = DotProduct (p2, plane->normal) - dist;

		// if completely in front of face, no intersection
		if (d1 > 0 && d2 >= d1)
			return;

		if (d1 > 0)
			startout = true;

		if (d1 <= 0 && d2 <= 0)
			continue;

		// crosses face
		if (d1 > d2)
		{	// enter
			f = (d1) / (d1-d2);
			if (f > enterfrac)
			{
				enterfrac = f;
				nearfrac = (d1-DIST_EPSILON) / (d1-d2);
				clipplane = plane;
				leadside = side;
			}
		}
		else
		{	// leave
			f = (d1) / (d1-d2);
			if (f < leavefrac)
				leavefrac = f;
		}
	}

	if (!startout)
	{
		trace->startsolid = true;
		return;		// original point is inside the patch
	}

	if (nearfrac <= leavefrac)
	{
		if (leadside && leadside->surface
			&& enterfrac <= trace_truefraction)
		{
			if (enterfrac < 0)
				enterfrac = 0;
			trace_truefraction = enterfrac;
			trace_nearfraction = nearfrac;
			trace->plane.dist = clipplane->dist;
			VectorCopy(clipplane->normal, trace->plane.normal);
			trace->surface = &leadside->surface->c;
			trace->contents = brush->contents;
		}
		else if (enterfrac < trace_truefraction)
			leavefrac=0;
	}
}
#endif

/*
================
CM_TestBoxInBrush
================
*/
static void CM_TestBoxInBrush (vec3_t mins, vec3_t maxs, vec3_t p1,
					  trace_t *trace, q2cbrush_t *brush)
{
	int			i, j;
	mplane_t	*plane;
	float		dist;
	vec3_t		ofs;
	float		d1;
	q2cbrushside_t	*side;

	if (!brush->numsides)
		return;

	for (i=0 ; i<brush->numsides ; i++)
	{
		side = brush->brushside+i;
		plane = side->plane;

		switch(trace_shape)
		{
		default:
		case shape_isbox:	// general box case

			// push the plane out apropriately for mins/maxs

			// FIXME: use signbits into 8 way lookup for each mins/maxs
			for (j=0 ; j<3 ; j++)
			{
				if (plane->normal[j] < 0)
					ofs[j] = maxs[j];
				else
					ofs[j] = mins[j];
			}
			dist = DotProduct (ofs, plane->normal);
			dist = plane->dist - dist;
			break;
		capsuledist(dist,plane,mins,maxs)
		case shape_ispoint:
			dist = plane->dist;
			break;
		}

		d1 = DotProduct (p1, plane->normal) - dist;

		// if completely in front of face, no intersection
		if (d1 > 0)
			return;
	}

	// inside this brush
	trace->startsolid = trace->allsolid = true;
	trace->contents |= brush->contents;
}

#ifdef Q3BSPS
static void CM_TestBoxInPatch (vec3_t mins, vec3_t maxs, vec3_t p1,
					  trace_t *trace, q2cbrush_t *brush)
{
	int			i, j;
	mplane_t	*plane;
	vec3_t		ofs, ofs2;
	float dist, thickness;
	float		d1;
	q2cbrushside_t	*side;

	if (!brush->numsides)
		return;

	i = 0;	//front plane
	{
		side = brush->brushside+i;
		plane = side->plane;

		switch(trace_shape)
		{
		default:
		case shape_isbox:
			for (j=0 ; j<3 ; j++)
			{
				if (plane->normal[j] < 0)
					ofs[j] = maxs[j], ofs2[j] = mins[j];
				else
					ofs[j] = mins[j], ofs2[j] = maxs[j];
			}

			dist = DotProduct (ofs, plane->normal);
			thickness = DotProduct (ofs2, plane->normal)-dist;
			dist = plane->dist - dist;
			break;
		case shape_iscapsule:
			dist = DotProduct(trace_up, plane->normal);
			thickness = dist*(trace_capsulesize[(dist<0)?2:1]) + trace_capsulesize[0]*2;
			dist = dist*(trace_capsulesize[(dist<0)?1:2]) - trace_capsulesize[0];
			dist = plane->dist - dist;
			break;
		case shape_ispoint:
			dist = plane->dist;
			thickness = 0;
			break;
		}

		d1 = DotProduct (p1, plane->normal) - dist;

		// if completely in front of face, no intersection
		if (d1 > 0)
			return;

		//point is behind the front plane, so no real intersection.
		if (thickness < 0.25)
			thickness = 0.25; //FIXME: patches should probably be infinitely thin, but that makes stuff messy.
		if (d1 < -thickness)
			return;
	}

	for (i=1 ; i<brush->numsides ; i++)
	{
		side = brush->brushside+i;
		plane = side->plane;

		switch(trace_shape)
		{
		default:
		case shape_isbox:
			// general box case

			// push the plane out apropriately for mins/maxs

			// FIXME: use signbits into 8 way lookup for each mins/maxs
			for (j=0 ; j<3 ; j++)
			{
				if (plane->normal[j] < 0)
					ofs[j] = maxs[j];
				else
					ofs[j] = mins[j];
			}

			dist = DotProduct (ofs, plane->normal);
			dist = plane->dist - dist;
			break;
		capsuledist(dist,plane,mins,maxs)
		case shape_ispoint:
			dist = plane->dist;
			break;
		}

		d1 = DotProduct (p1, plane->normal) - dist;

		// if completely in front of face, no intersection
		if (d1 > 0)
			return;
	}

	// inside this patch
	trace->startsolid = trace->allsolid = true;
	trace->contents = brush->contents;
}
#endif

/*
================
CM_TraceToLeaf
================
*/
static void CM_TraceToLeaf (cminfo_t	*prv, mleaf_t		*leaf)
{
	int			k;
	q2cbrush_t	*b;

#ifdef Q3BSPS
	int patchnum, j;
	q3cpatch_t *patch;
	q3cmesh_t *cmesh;
#endif

	if ( !(leaf->contents & trace_contents))
		return;
	// trace line against all brushes in the leaf
	for (k=0 ; k<leaf->numleafbrushes ; k++)
	{
		b = prv->leafbrushes[leaf->firstleafbrush+k];
		if (b->checkcount == checkcount)
			continue;	// already checked this brush in another leaf
		b->checkcount = checkcount;

		if ( !(b->contents & trace_contents))
			continue;
		if (!BoundsIntersect(b->absmins, b->absmaxs, trace_absmins, trace_absmaxs))
			continue;
		CM_ClipBoxToBrush (trace_mins, trace_maxs, trace_start, trace_end, &trace_trace, b);
		if (trace_nearfraction <= 0)
			return;
	}

#ifdef Q3BSPS
	if (!prv->mapisq3 || map_noCurves.value)
		return;

	// trace line against all patches in the leaf
	for (k = 0; k < leaf->numleafpatches; k++)
	{
		patchnum = prv->leafpatches[leaf->firstleafpatch+k];

		patch = &prv->patches[patchnum];
		if (patch->checkcount == checkcount)
			continue;	// already checked this patch in another leaf
		patch->checkcount = checkcount;
		if ( !(patch->surface->c.value & trace_contents) )
			continue;
		if ( !BoundsIntersect(patch->absmins, patch->absmaxs, trace_absmins, trace_absmaxs) )
			continue;
		for (j = 0; j < patch->numfacets; j++)
		{
			CM_ClipBoxToPatch (trace_mins, trace_maxs, trace_start, trace_end, &trace_trace, &patch->facets[j]);
			if (trace_nearfraction<=0)
				return;
		}
	}

	for (k = 0; k < leaf->numleafcmeshes; k++)
	{
		patchnum = prv->leafcmeshes[leaf->firstleafcmesh+k];
		cmesh = &prv->cmeshes[patchnum];
		if (cmesh->checkcount == checkcount)
			continue;	// already checked this patch in another leaf
		cmesh->checkcount = checkcount;
		if ( !(cmesh->surface->c.value & trace_contents) )
			continue;
		if ( !BoundsIntersect(cmesh->absmins, cmesh->absmaxs, trace_absmins, trace_absmaxs) )
			continue;

		Mod_Trace_Trisoup_(cmesh->xyz_array, cmesh->indicies, cmesh->numincidies, trace_start, trace_end, trace_mins, trace_maxs, &trace_trace, &cmesh->surface->c);
		if (trace_nearfraction<=0)
			return;
	}
#endif
}


/*
================
CM_TestInLeaf
================
*/
static void CM_TestInLeaf (cminfo_t *prv, mleaf_t *leaf)
{
	int			k;
	q2cbrush_t	*b;
#ifdef Q3BSPS
	int patchnum, j;
	q3cmesh_t	*cmesh;
	q3cpatch_t *patch;
#endif

	if ( !(leaf->contents & trace_contents))
		return;
	// trace line against all brushes in the leaf
	for (k=0 ; k<leaf->numleafbrushes ; k++)
	{
		b = prv->leafbrushes[leaf->firstleafbrush+k];
		if (b->checkcount == checkcount)
			continue;	// already checked this brush in another leaf
		b->checkcount = checkcount;

		if (!(b->contents & trace_contents))
			continue;
		if (!BoundsIntersect(b->absmins, b->absmaxs, trace_absmins, trace_absmaxs))
			continue;
		CM_TestBoxInBrush (trace_mins, trace_maxs, trace_start, &trace_trace, b);
		if (!trace_trace.fraction)
			return;
	}

#ifdef Q3BSPS
	if (!prv->mapisq3 || map_noCurves.value)
		return;

	// trace line against all patches in the leaf
	for (k = 0; k < leaf->numleafpatches; k++)
	{
		patchnum = prv->leafpatches[leaf->firstleafpatch+k];

		patch = &prv->patches[patchnum];
		if (patch->checkcount == checkcount)
			continue;	// already checked this patch in another leaf
		patch->checkcount = checkcount;
		if ( !(patch->surface->c.value & trace_contents) )
			continue;
		if ( !BoundsIntersect(patch->absmins, patch->absmaxs, trace_absmins, trace_absmaxs) )
			continue;
		for (j = 0; j < patch->numfacets; j++)
		{
			CM_TestBoxInPatch (trace_mins, trace_maxs, trace_start, &trace_trace, &patch->facets[j]);
			if (!trace_trace.fraction)
				return;
		}
	}

	for (k = 0; k < leaf->numleafcmeshes; k++)
	{
		patchnum = prv->leafcmeshes[leaf->firstleafcmesh+k];
		cmesh = &prv->cmeshes[patchnum];
		if (cmesh->checkcount == checkcount)
			continue;	// already checked this patch in another leaf
		cmesh->checkcount = checkcount;
		if ( !(cmesh->surface->c.value & trace_contents) )
			continue;
		if ( !BoundsIntersect(cmesh->absmins, cmesh->absmaxs, trace_absmins, trace_absmaxs) )
			continue;

		Mod_Trace_Trisoup_(cmesh->xyz_array, cmesh->indicies, cmesh->numincidies, trace_start, trace_end, trace_mins, trace_maxs, &trace_trace, &cmesh->surface->c);
		if (trace_nearfraction<=0)
			return;
	}
#endif
}


/*
==================
CM_RecursiveHullCheck

==================
*/
static void CM_RecursiveHullCheck (model_t *mod, int num, float p1f, float p2f, vec3_t p1, vec3_t p2)
{
	mnode_t		*node;
	mplane_t	*plane;
	float		t1, t2, offset;
	float		frac, frac2;
	float		idist;
	int			i;
	vec3_t		mid;
	int			side;
	float		midf;

	if (trace_truefraction <= p1f)
		return;		// already hit something nearer

	// if < 0, we are in a leaf node
	if (num < 0)
	{
		CM_TraceToLeaf (mod->meshinfo, &mod->leafs[-1-num]);
		return;
	}

	//
	// find the point distances to the seperating plane
	// and the offset for the size of the box
	//
	node = mod->nodes + num;
	plane = node->plane;

	if (plane->type < 3)
	{
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
		offset = trace_extents[plane->type];
	}
	else
	{
		t1 = DotProduct (plane->normal, p1) - plane->dist;
		t2 = DotProduct (plane->normal, p2) - plane->dist;
		if (trace_shape == shape_ispoint)
			offset = 0;
		else
			offset = fabs(trace_extents[0]*plane->normal[0]) +
				fabs(trace_extents[1]*plane->normal[1]) +
				fabs(trace_extents[2]*plane->normal[2]);
	}


#if 0
CM_RecursiveHullCheck (node->childnum[0], p1f, p2f, p1, p2);
CM_RecursiveHullCheck (node->childnum[1], p1f, p2f, p1, p2);
return;
#endif

	// see which sides we need to consider
	if (t1 >= offset && t2 >= offset)
	{
		CM_RecursiveHullCheck (mod, node->childnum[0], p1f, p2f, p1, p2);
		return;
	}
	if (t1 < -offset && t2 < -offset)
	{
		CM_RecursiveHullCheck (mod, node->childnum[1], p1f, p2f, p1, p2);
		return;
	}

	// put the crosspoint DIST_EPSILON pixels on the near side
	if (t1 < t2)
	{
		idist = 1.0/(t1-t2);
		side = 1;
		frac2 = (t1 + offset + DIST_EPSILON)*idist;
		frac = (t1 - offset + DIST_EPSILON)*idist;
	}
	else if (t1 > t2)
	{
		idist = 1.0/(t1-t2);
		side = 0;
		frac2 = (t1 - offset - DIST_EPSILON)*idist;
		frac = (t1 + offset + DIST_EPSILON)*idist;
	}
	else
	{
		side = 0;
		frac = 1;
		frac2 = 0;
	}

	// move up to the node
	if (frac < 0)
		frac = 0;
	if (frac > 1)
		frac = 1;

	midf = p1f + (p2f - p1f)*frac;
	for (i=0 ; i<3 ; i++)
		mid[i] = p1[i] + frac*(p2[i] - p1[i]);

	CM_RecursiveHullCheck (mod, node->childnum[side], p1f, midf, p1, mid);


	// go past the node
	if (frac2 < 0)
		frac2 = 0;
	if (frac2 > 1)
		frac2 = 1;

	midf = p1f + (p2f - p1f)*frac2;
	for (i=0 ; i<3 ; i++)
		mid[i] = p1[i] + frac2*(p2[i] - p1[i]);

	CM_RecursiveHullCheck (mod, node->childnum[side^1], midf, p2f, mid, p2);
}

//======================================================================

/*
==================
CM_BoxTrace
==================
*/
static trace_t		CM_BoxTrace (model_t *mod, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs, qboolean capsule,
						  int brushmask)
{
	int		i;
	vec3_t point;


	checkcount++;		// for multi-check avoidance

	// fill in a default trace
	memset (&trace_trace, 0, sizeof(trace_trace));
	trace_truefraction = 1;
	trace_nearfraction = 1;
	trace_trace.fraction = 1;
	trace_trace.truefraction = 1;
	trace_trace.surface = &(nullsurface.c);

	if (!mod)	// map not loaded
		return trace_trace;

	trace_contents = brushmask;
	VectorCopy (start, trace_start);
	VectorCopy (end, trace_end);
	VectorCopy (mins, trace_mins);
	VectorCopy (maxs, trace_maxs);

	if (1)
	{
		VectorAdd(trace_maxs, trace_mins, point);
		VectorScale(point, 0.5, point);

		VectorAdd(trace_start, point, trace_start);
		VectorAdd(trace_end, point, trace_end);
		VectorSubtract(trace_mins, point, trace_mins);
		VectorSubtract(trace_maxs, point, trace_maxs);
	}



	// build a bounding box of the entire move (for patches)
	ClearBounds (trace_absmins, trace_absmaxs);

	//determine the type of trace that we're going to use, and the max extents
	if (trace_mins[0] == 0 && trace_mins[1] == 0 && trace_mins[2] == 0 && trace_maxs[0] == 0 && trace_maxs[1] == 0 && trace_maxs[2] == 0)
	{
		trace_shape = shape_ispoint;
		VectorSet (trace_extents, 1/32.0, 1/32.0, 1/32.0);
		//acedemic
		AddPointToBounds (trace_start, trace_absmins, trace_absmaxs);
		AddPointToBounds (trace_end, trace_absmins, trace_absmaxs);
	}
	else if (capsule)
	{
		float ext;
		trace_shape = shape_iscapsule;
		//determine the capsule sizes
		trace_capsulesize[0] = ((trace_maxs[0]-trace_mins[0]) + (trace_maxs[1]-trace_mins[1]))/4.0;
		trace_capsulesize[1] = trace_maxs[2];
		trace_capsulesize[2] = trace_mins[2];
		//make sure the mins_z/maxs_z isn't screwed.
//		if (trace_capsulesize[1]-trace_capsulesize[2] < trace_capsulesize[0])
//			trace_capsulesize[1] = trace_capsulesize[0]+trace_capsulesize[2];
		ext = (trace_capsulesize[1] > -trace_capsulesize[2])?trace_capsulesize[1]:-trace_capsulesize[2];
		trace_capsulesize[1] -= trace_capsulesize[0];
		trace_capsulesize[2] += trace_capsulesize[0];
		trace_extents[0] = ext+1;
		trace_extents[1] = ext+1;
		trace_extents[2] = ext+1;

		//determine the total range
		VectorSubtract (trace_start, trace_extents, point);
		AddPointToBounds (point, trace_absmins, trace_absmaxs);
		VectorAdd (trace_start, trace_extents, point);
		AddPointToBounds (point, trace_absmins, trace_absmaxs);
		VectorSubtract (trace_end, trace_extents, point);
		AddPointToBounds (point, trace_absmins, trace_absmaxs);
		VectorAdd (trace_end, trace_extents, point);
		AddPointToBounds (point, trace_absmins, trace_absmaxs);
	}
	else
	{
		VectorAdd (trace_start, trace_mins, point);
		AddPointToBounds (point, trace_absmins, trace_absmaxs);
		VectorAdd (trace_start, trace_maxs, point);
		AddPointToBounds (point, trace_absmins, trace_absmaxs);
		VectorAdd (trace_end, trace_mins, point);
		AddPointToBounds (point, trace_absmins, trace_absmaxs);
		VectorAdd (trace_end, trace_maxs, point);
		AddPointToBounds (point, trace_absmins, trace_absmaxs);

		trace_shape = shape_isbox;
		trace_extents[0] = ((-trace_mins[0] > trace_maxs[0]) ? -trace_mins[0] : trace_maxs[0])+1;
		trace_extents[1] = ((-trace_mins[1] > trace_maxs[1]) ? -trace_mins[1] : trace_maxs[1])+1;
		trace_extents[2] = ((-trace_mins[2] > trace_maxs[2]) ? -trace_mins[2] : trace_maxs[2])+1;
	}

	trace_absmins[0] -= 1.0;
	trace_absmins[1] -= 1.0;
	trace_absmins[2] -= 1.0;
	trace_absmaxs[0] += 1.0;
	trace_absmaxs[1] += 1.0;
	trace_absmaxs[2] += 1.0;

#if 0
	if (0)
	{	//treat *ALL* tests against the actual geometry instead of using any brushes.
		//also ignores the bsp etc. not fast. testing only.

		trace_ispoint = trace_mins[0] == 0 && trace_mins[1] == 0 && trace_mins[2] == 0
				&& trace_maxs[0] == 0 && trace_maxs[1] == 0 && trace_maxs[2] == 0;
	
		for (i = 0; i < mod->numsurfaces; i++)
		{
			CM_ClipBoxToMesh(trace_mins, trace_maxs, trace_start, trace_end, &trace_trace, mod->surfaces[i].mesh);
		}
	}
	else
	if (0)
	{
		trace_ispoint = trace_mins[0] == 0 && trace_mins[1] == 0 && trace_mins[2] == 0
				&& trace_maxs[0] == 0 && trace_maxs[1] == 0 && trace_maxs[2] == 0;
	
		for (i = 0; i < mod->numleafs; i++)
			CM_TraceToLeaf(&mod->leafs[i]);
	}
	else
#endif
	//
	// check for position test special case
	//
	if (start[0] == end[0] && start[1] == end[1] && start[2] == end[2])
	{
		int		leafs[1024];
		int		i, numleafs;
		int		topnode;

		numleafs = CM_BoxLeafnums_headnode (mod, trace_absmins, trace_absmaxs, leafs, sizeof(leafs)/sizeof(leafs[0]), mod->hulls[0].firstclipnode, &topnode);
		for (i=0 ; i<numleafs ; i++)
		{
			CM_TestInLeaf (mod->meshinfo, &mod->leafs[leafs[i]]);
			if (trace_trace.allsolid)
				break;
		}
		VectorCopy (start, trace_trace.endpos);
		return trace_trace;
	}
	//
	// general aabb trace
	//
	else
	{
		CM_RecursiveHullCheck (mod, mod->hulls[0].firstclipnode, 0, 1, trace_start, trace_end);
	}

	if (trace_nearfraction == 1)
	{
		trace_trace.fraction = 1;
		VectorCopy (end, trace_trace.endpos);
	}
	else
	{
		if (trace_nearfraction<0)
			trace_nearfraction=0;
		trace_trace.fraction = trace_nearfraction;
		for (i=0 ; i<3 ; i++)
			trace_trace.endpos[i] = start[i] + trace_trace.fraction * (end[i] - start[i]);
	}
	return trace_trace;
}

static qboolean BM_NativeTrace(model_t *model, int forcehullnum, const framestate_t *framestate, const vec3_t axis[3], const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, qboolean capsule, unsigned int contents, trace_t *trace)
{
	int i;
	memset (trace, 0, sizeof(*trace));
	trace_truefraction = 1;
	trace_nearfraction = 1;
	trace->fraction = 1;
	trace->truefraction = 1;
	trace->surface = &(nullsurface.c);

	if (contents & FTECONTENTS_BODY)
	{
		trace_contents = contents;
		VectorCopy (start, trace_start);
		VectorCopy (end, trace_end);
		VectorCopy (mins, trace_mins);
		VectorCopy (maxs, trace_maxs);

		if (trace_mins[0] == 0 && trace_mins[1] == 0 && trace_mins[2] == 0 && trace_maxs[0] == 0 && trace_maxs[1] == 0 && trace_maxs[2] == 0)
			trace_shape = shape_ispoint;
		else if (capsule)
			trace_shape = shape_iscapsule;
		else
			trace_shape = shape_isbox;

		CM_ClipBoxToBrush (trace_mins, trace_maxs, trace_start, trace_end, trace, &box_brush);
	}

	if (trace_nearfraction == 1)
	{
		trace->fraction = 1;
		VectorCopy (trace_end, trace->endpos);
	}
	else
	{
		if (trace_nearfraction<0)
			trace_nearfraction=0;
		trace->fraction = trace_nearfraction;
		trace->truefraction = trace_truefraction;
		for (i=0 ; i<3 ; i++)
			trace->endpos[i] = trace_start[i] + trace->fraction * (trace_end[i] - trace_start[i]);
	}
	return trace->fraction != 1;
}
static qboolean CM_NativeTrace(model_t *model, int forcehullnum, const framestate_t *framestate, const vec3_t axis[3], const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, qboolean capsule, unsigned int contents, trace_t *trace)
{
	if (axis)
	{
		vec3_t start_l;
		vec3_t end_l;
		start_l[0] = DotProduct(start, axis[0]);
		start_l[1] = DotProduct(start, axis[1]);
		start_l[2] = DotProduct(start, axis[2]);
		end_l[0] = DotProduct(end, axis[0]);
		end_l[1] = DotProduct(end, axis[1]);
		end_l[2] = DotProduct(end, axis[2]);
		VectorSet(trace_up, axis[0][2], -axis[1][2], axis[2][2]);
		*trace = CM_BoxTrace(model, start_l, end_l, mins, maxs, capsule, contents);
#ifdef TERRAIN
		if (model->terrain)
		{
			trace_t hmt;
			Heightmap_Trace(model, forcehullnum, framestate, NULL, start, end, mins, maxs, capsule, contents, &hmt);
			if (hmt.fraction < trace->fraction)
				*trace = hmt;
		}
#endif

		if (trace->fraction == 1)
		{
			VectorCopy (end, trace->endpos);
		}
		else
		{
			vec3_t iaxis[3];
			vec3_t norm;
			Matrix3x3_RM_Invert_Simple((void *)axis, iaxis);
			VectorCopy(trace->plane.normal, norm);
			trace->plane.normal[0] = DotProduct(norm, iaxis[0]);
			trace->plane.normal[1] = DotProduct(norm, iaxis[1]);
			trace->plane.normal[2] = DotProduct(norm, iaxis[2]);

			/*just interpolate it, its easier than inverse matrix rotations*/
			VectorInterpolate(start, trace->fraction, end, trace->endpos);
		}
	}
	else
	{
		VectorSet(trace_up, 0, 0, 1);
		*trace = CM_BoxTrace(model, start, end, mins, maxs, capsule, contents);
#ifdef TERRAIN
		if (model->terrain)
		{
			trace_t hmt;
			Heightmap_Trace(model, forcehullnum, framestate, NULL, start, end, mins, maxs, capsule, contents, &hmt);
			if (hmt.fraction < trace->fraction)
				*trace = hmt;
		}
#endif
	}
	return trace->fraction != 1;
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

/*
qbyte *Mod_Q2DecompressVis (qbyte *in, model_t *model)
{
	static qbyte	decompressed[MAX_MAP_LEAFS/8];
	int		c;
	qbyte	*out;
	int		row;

	row = (model->vis->numclusters+7)>>3;
	out = decompressed;

	if (!in)
	{	// no vis info, so make all visible
		while (row)
		{
			*out++ = 0xff;
			row--;
		}
		return decompressed;
	}

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}

		c = in[1];
		in += 2;
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);

	return decompressed;
}

#define	DVIS_PVS	0
#define	DVIS_PHS	1
qbyte *Mod_ClusterPVS (int cluster, model_t *model)
{
	if (cluster == -1 || !model->vis)
		return mod_novis;
	return Mod_Q2DecompressVis ( (qbyte *)model->vis + model->vis->bitofs[cluster][DVIS_PVS],
		model);
}
*/
static void CM_DecompressVis (model_t *mod, qbyte *in, qbyte *out, qboolean merge)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
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



static qbyte	*CM_ClusterPVS (model_t *mod, int cluster, pvsbuffer_t *buffer, pvsmerge_t merge)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	if (!buffer)
		buffer = &pvsrow;
	if (buffer->buffersize < mod->pvsbytes)
		buffer->buffer = BZ_Realloc(buffer->buffer, buffer->buffersize=mod->pvsbytes);

	if (mod->fromgame == fg_quake2)
	{
		if (cluster == -1)
			memset (buffer->buffer, 0, (mod->numclusters+7)>>3);
		else
			CM_DecompressVis (mod, ((qbyte*)prv->q2vis) + prv->q2vis->bitofs[cluster][DVIS_PVS], buffer->buffer, merge==PVM_MERGE);
		return buffer->buffer;
	}
	else
	{
		if (cluster != -1 && prv->q3pvs->numclusters)
		{
			if (merge == PVM_FAST)
				return (qbyte *)prv->q3pvs->data + cluster * prv->q3pvs->rowsize;
			else if (merge == PVM_REPLACE)
				memcpy(buffer->buffer, prv->q3pvs->data + cluster * prv->q3pvs->rowsize, mod->pvsbytes);
			else
			{
				int c;
				char *in = prv->q3pvs->data + cluster * prv->q3pvs->rowsize;
				for (c = 0; c < mod->pvsbytes; c+=4)
					*(int*)&buffer->buffer[c] |= *(int*)&in[c];
			}
		}
		else
		{
			if (merge != PVM_MERGE)
				memset (buffer->buffer, 0, (mod->numclusters+7)>>3);
		}
		return buffer->buffer;
	}
}

static qbyte	*CM_ClusterPHS (model_t *mod, int cluster, pvsbuffer_t *buffer)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;

	if (!buffer)
		buffer = &phsrow;
	if (buffer->buffersize < mod->pvsbytes)
		buffer->buffer = BZ_Realloc(buffer->buffer, buffer->buffersize=mod->pvsbytes);

#ifdef Q3BSPS
	if (mod->fromgame != fg_quake2)
	{
		if (cluster != -1 && prv->q3phs->numclusters)
		{
			if (prv->phscalced && !(prv->phscalced[cluster>>3] & (1<<(cluster&7))))
				CalcClusterPHS(prv, cluster);
			return (qbyte *)prv->q3phs->data + cluster * prv->q3phs->rowsize;
		}
		else
		{
			memset (buffer->buffer, 0, (mod->numclusters+7)>>3);
			return buffer->buffer;
		}
	}
#endif

	if (cluster == -1)
		memset (buffer->buffer, 0, (mod->numclusters+7)>>3);
	else
		CM_DecompressVis (mod, ((qbyte*)prv->q2vis) + prv->q2vis->bitofs[cluster][DVIS_PHS], buffer->buffer, false);
	return buffer->buffer;
}

#ifdef HAVE_SERVER
static unsigned int  SV_Q2BSP_FatPVS (model_t *mod, const vec3_t org, pvsbuffer_t *result, qboolean merge)
{
	int	leafs[64];
	int		i, j, count;
	vec3_t	mins, maxs;

	for (i=0 ; i<3 ; i++)
	{
		mins[i] = org[i] - 8;
		maxs[i] = org[i] + 8;
	}

	count = CM_BoxLeafnums (mod, mins, maxs, leafs, countof(leafs), NULL);
	if (count < 1)
		Sys_Error ("SV_Q2FatPVS: count < 1");

	// convert leafs to clusters
	for (i=0 ; i<count ; i++)
		leafs[i] = CM_LeafCluster(mod, leafs[i]);

	//grow the buffer if needed
	if (result->buffersize < mod->pvsbytes)
		result->buffer = BZ_Realloc(result->buffer, result->buffersize=mod->pvsbytes);

	if (count == 1 && leafs[0] == -1)
	{	//if the only leaf is the outside then broadcast it.
		memset(result->buffer, 0xff, mod->pvsbytes);
		i = count;
	}
	else
	{
		i = 0;
		if (!merge)
			CM_ClusterPVS(mod, leafs[i++], result, PVM_REPLACE);
		// or in all the other leaf bits
		for ( ; i<count ; i++)
		{
			for (j=0 ; j<i ; j++)
				if (leafs[i] == leafs[j])
					break;
			if (j != i)
				continue;		// already have the cluster we want
			CM_ClusterPVS(mod, leafs[i], result, PVM_MERGE);
		}
	}
	return mod->pvsbytes;
}

static int		clientarea;
static unsigned int Q23BSP_FatPVS(model_t *mod, const vec3_t org, pvsbuffer_t *fte_restrict buffer, qboolean merge)
{//fixme: this doesn't add areas
	int		leafnum;
	leafnum = CM_PointLeafnum (mod, org);
	clientarea = CM_LeafArea (mod, leafnum);

	return SV_Q2BSP_FatPVS (mod, org, buffer, merge);
}

static qboolean Q23BSP_EdictInFatPVS(model_t *mod, const pvscache_t *ent, const qbyte *pvs, const int *areas)
{
	int i,l;
	int nullarea = (mod->fromgame == fg_quake2)?0:-1;
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
			else if (CM_AreasConnected (mod, areas[i], ent->areanum))
				break;
			// doors can legally straddle two areas, so
			// we may need to check another one
			else if (ent->areanum2 != nullarea && CM_AreasConnected (mod, areas[i], ent->areanum2))
				break;
		}
	}

	if (ent->num_leafs == -1)
	{	// too many leafs for individual check, go by headnode
		if (!CM_HeadnodeVisible (mod, ent->headnode, pvs))
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

static void Q23BSP_FindTouchedLeafs(model_t *model, struct pvscache_s *ent, const float *mins, const float *maxs)
{
#define MAX_TOTAL_ENT_LEAFS		128
	int			leafs[MAX_TOTAL_ENT_LEAFS];
	int			clusters[MAX_TOTAL_ENT_LEAFS];
	int num_leafs;
	int			topnode;
	int i, j;
	int			area;
	int nullarea = (model->fromgame == fg_quake2)?0:-1;

	//ent->num_leafs == q2's ent->num_clusters
	ent->num_leafs = 0;
	ent->areanum = nullarea;
	ent->areanum2 = nullarea;

	if (!mins || !maxs)
		return;

	//get all leafs, including solids
	num_leafs = CM_BoxLeafnums (model, mins, maxs,
		leafs, MAX_TOTAL_ENT_LEAFS, &topnode);

	// set areas
	for (i=0 ; i<num_leafs ; i++)
	{
		clusters[i] = CM_LeafCluster (model, leafs[i]);
		area = CM_LeafArea (model, leafs[i]);
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

/*
===============================================================================

AREAPORTALS

===============================================================================
*/

static void FloodArea_r (cminfo_t	*prv, size_t areaidx, int floodnum)
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
	switch(prv->mapisq3)
	{
	case true:
#ifdef Q3BSPS
		for (i=0 ; i<prv->numareas ; i++)
		{
			if (prv->q3areas[areaidx].numareaportals[i]>0)
				FloodArea_r (prv, i, floodnum);
		}
#endif
		break;
	case false:
#ifdef Q2BSPS
		{
			q2carea_t *area = &prv->q2areas[areaidx];
			q2dareaportal_t	*p = &prv->q2areaportals[area->firstareaportal];
			for (i=0 ; i<area->numareaportals ; i++, p++)
			{
				if (prv->q2portalopen[p->portalnum])
					FloodArea_r (prv, p->otherarea, floodnum);
			}
		}
#endif
		break;
	}
}

/*
====================
FloodAreaConnections


====================
*/
static void	FloodAreaConnections (cminfo_t	*prv)
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

static void	CM_SetAreaPortalState (model_t *mod, unsigned int portalnum, unsigned int area1, unsigned int area2, qboolean open)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	switch(prv->mapisq3)
	{
#ifdef Q3BSPS
	case true:
		if (area1 >= prv->numareas || area2 >= prv->numareas || area1==area2)
			return;

		if (open)
		{
			prv->q3areas[area1].numareaportals[area2]++;
			prv->q3areas[area2].numareaportals[area1]++;
		}
		else
		{
			if (!prv->q3areas[area1].numareaportals[area2])
			{
				Con_Printf(CON_WARNING"CM_SetAreaPortalState: Areaportal closed more than opened...\n");
				return;
			}
			prv->q3areas[area1].numareaportals[area2]--;
			prv->q3areas[area2].numareaportals[area1]--;
		}
		break;
#endif
#ifdef Q2BSPS
	case false:
		if (portalnum > prv->numq2areaportals)
			return;

		if (prv->q2portalopen[portalnum] == open)
			return;
		prv->q2portalopen[portalnum] = open;
		break;
#endif
	default: break;
	}
	FloodAreaConnections (prv);
}

static qboolean	VARGS CM_AreasConnected (model_t *mod, unsigned int area1, unsigned int area2)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;

	if (map_noareas.value)
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
static size_t CM_WriteAreaBits (model_t *mod, qbyte *buffer, size_t buffersize, int area, qboolean merge)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;
	int		i;
	int		floodnum;
	int		bytes;

	bytes = (prv->numareas+7)>>3;
	if (bytes > buffersize)
		bytes = buffersize;

	if (map_noareas.value || (area < 0 && !merge))
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
			if (prv->areaflood[i].floodnum == floodnum || !area)
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
static size_t CM_SaveAreaPortalBlob (model_t *mod, void **data)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;

	if (mod->type == mod_brush && (mod->fromgame == fg_quake2 || mod->fromgame == fg_quake3))
	{
		switch(prv->mapisq3)
		{
#ifdef Q3BSPS
		case true:
			//endian issues. oh well.
			*data = prv->q3areas;
			return sizeof(prv->q3areas);
#endif
#ifdef Q2BSPS
		case false:
			*data = prv->q2portalopen;
			return sizeof(prv->q2portalopen);
#endif
		default: break;
		}
	}
	*data = NULL;
	return 0;
}

/*
===================
CM_ReadPortalState

Reads the portal state from a savegame file
and recalculates the area connections
===================
*/
static size_t	CM_LoadAreaPortalBlob (model_t *mod, void *ptr, size_t ptrsize)
{
	cminfo_t	*prv = (cminfo_t*)mod->meshinfo;

	if (mod->type == mod_brush && (mod->fromgame == fg_quake2 || mod->fromgame == fg_quake3))
	{
		switch(prv->mapisq3)
		{
#ifdef Q3BSPS
		case 1:	//area*area refcounts. byte sizes don't tell us how many areas there were - would need sqrt(ptrsize/4) and I cba.
			if (ptrsize != sizeof(prv->q3areas))
			{	//don't bother trying to handle graceful expansion/truncation, just reset the entire thing.
				size_t x,y;
				if (ptrsize)
					Con_Printf("CM_ReadPortalState() expected %u, but only %u available\n",(unsigned int)sizeof(prv->q3areas),(unsigned int)ptrsize);
				for (x = 0; x < countof(prv->q3areas); x++)
					for (y = 0; y < countof(prv->q3areas[x].numareaportals); y++)
						prv->q3areas[x].numareaportals[y] = map_autoopenportals.ival;
			}
			else
				memcpy(prv->q3areas, ptr, ptrsize);
			FloodAreaConnections (prv);
			return sizeof(prv->q3areas);
#endif
#ifdef Q2BSPS
		case 0:	//per-portal booleans. we can just pad any missing portals.
			if (ptrsize && ptrsize != sizeof(prv->q2portalopen))
				Con_Printf("CM_ReadPortalState() expected %u, but only %u available\n",(unsigned int)sizeof(prv->q2portalopen),(unsigned int)ptrsize);
			if (ptrsize > sizeof(prv->q2portalopen))
				ptrsize = sizeof(prv->q2portalopen);
			memcpy(prv->q2portalopen, ptr, ptrsize);
			memset(prv->q2portalopen+ptrsize, map_autoopenportals.ival, sizeof(prv->q2portalopen)-ptrsize);
			FloodAreaConnections (prv);
			return sizeof(prv->q2portalopen);
#endif
		default: break;
		}
	}
	return 0;
}

#ifdef HAVE_SERVER
/*
=============
CM_HeadnodeVisible

Returns true if any leaf under headnode has a cluster that
is potentially visible
=============
*/
static qboolean CM_HeadnodeVisible (model_t *mod, int nodenum, const qbyte *visbits)
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
	if (CM_HeadnodeVisible(mod, node->childnum[0], visbits))
		return true;
	return CM_HeadnodeVisible(mod, node->childnum[1], visbits);
}
#endif

static unsigned int Q2BSP_PointContents(model_t *mod, const vec3_t axis[3], const vec3_t p)
{
	int pc;
	pc = CM_PointContents (mod, p);
	return pc;
}



#ifdef HAVE_CLIENT
static qbyte *frustumvis;
static vec3_t modelorg;
static unsigned int scenesequence;
static unsigned int vissequence;
/*
===============
R_MarkLeaves
===============
*/
#ifdef Q3BSPS
qbyte *R_MarkLeaves_Q3 (model_t *mod, int clusters[2])
{
	static pvsbuffer_t	curframevis[R_MAX_RECURSE];
	qbyte *vis;
	int		i;

	int cluster;
	mleaf_t	*leaf;
	mnode_t *node;
	int portal = r_refdef.recurse;
	cminfo_t	*prv = mod->meshinfo;

	if (!portal)
	{
		if (prv->oldclusters[0] == clusters[0] && !r_novis.value && clusters[0] != -1)
			return prv->oldvis;
	}

	// development aid to let you run around and see exactly where
	// the pvs ends
//		if (r_lockpvs->value)
//			return;

	vissequence++;
	prv->oldclusters[0] = clusters[0];

	if (r_novis.ival || clusters[0] == -1 || !mod->vis )
	{
		vis = NULL;
		// mark everything
		for (i=0,leaf=mod->leafs ; i<mod->numleafs ; i++, leaf++)
		{
//			if (!leaf->nummarksurfaces)
//			{
//				continue;
//			}

#if 1
			for (node = (mnode_t*)leaf; node; node = node->parent)
			{
				if (node->visframe == vissequence)
					break;
				node->visframe = vissequence;
			}
#else
			leaf->visframe = vissequence;
			leaf->vischain = r_vischain;
			r_vischain = leaf;
#endif
		}
	}
	else
	{
		vis = CM_ClusterPVS (mod, clusters[0], &curframevis[portal], PVM_FAST);
		for (i=0,leaf=mod->leafs ; i<mod->numleafs ; i++, leaf++)
		{
			cluster = leaf->cluster;
			if (cluster == -1)// || !leaf->nummarksurfaces)
			{
				continue;
			}
			if (vis[cluster>>3] & (1<<(cluster&7)))
			{
#if 1
				for (node = (mnode_t*)leaf; node; node = node->parent)
				{
					if (node->visframe == vissequence)
						break;
					node->visframe = vissequence;
				}
#else
				leaf->visframe = vissequence;
				leaf->vischain = r_vischain;
				r_vischain = leaf;
#endif
			}
		}
	}
	prv->oldvis = vis;
	return vis;
}

static void Surf_RecursiveQ3WorldNode (mnode_t *node, unsigned int clipflags)
{
	int			c, side, clipped;
	mplane_t	*plane, *clipplane;
	msurface_t	*surf, **mark;
	mleaf_t		*pleaf;
	double		dot;

start:

	if (node->visframe != vissequence)
		return;

	for (c = 0, clipplane = r_refdef.frustum; c < r_refdef.frustum_numworldplanes; c++, clipplane++)
	{
		if (!(clipflags & (1 << c)))
			continue;	// don't need to clip against it

		clipped = BOX_ON_PLANE_SIDE (node->minmaxs, node->minmaxs + 3, clipplane);
		if (clipped == 2)
			return;
		else if (clipped == 1)
			clipflags -= (1<<c);	// node is entirely on screen
	}

// if a leaf node, draw stuff
	if (node->contents != -1)
	{
		pleaf = (mleaf_t *)node;

		if (! (r_refdef.areabits[pleaf->area>>3] & (1<<(pleaf->area&7)) ) )
			return;		// not visible

		c = pleaf->cluster;
		if (c >= 0)
			frustumvis[c>>3] |= 1<<(c&7);

		mark = pleaf->firstmarksurface;
		for (c = pleaf->nummarksurfaces; c; c--)
		{
			surf = *mark++;
			if (surf->visframe == scenesequence)
				continue;
			surf->visframe = scenesequence;

//			if (((dot < 0) ^ !!(surf->flags & SURF_PLANEBACK)))
//				continue;		// wrong side

			surf->sbatch->mesh[surf->sbatch->meshes++] = surf->mesh;
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
		side = 0;
	else
		side = 1;

// recurse down the children, front side first
	Surf_RecursiveQ3WorldNode (node->children[side], clipflags);

// q3 nodes contain no drawables

// recurse down the back side
	//GLR_RecursiveWorldNode (node->children[!side], clipflags);
	node = node->children[!side];
	goto start;
}
#endif

#ifdef Q2BSPS
qbyte *R_MarkLeaves_Q2 (model_t *mod, int viewclusters[2])
{
	static pvsbuffer_t	curframevis[R_MAX_RECURSE];
	static qbyte	*cvis[R_MAX_RECURSE];
	mnode_t	*node;
	int		i;

	int cluster;
	mleaf_t	*leaf;
	qbyte *vis;

	int portal = r_refdef.recurse;
	cminfo_t	*prv = mod->meshinfo;

	if (r_refdef.forcevis)
	{
		vis = cvis[portal] = r_refdef.forcedvis;

		prv->oldclusters[0] = -1;
		prv->oldclusters[1] = -1;
	}
	else
	{
		vis = cvis[portal];
		if (!portal)
		{
			if (prv->oldclusters[0] == viewclusters[0] && prv->oldclusters[1] == viewclusters[1])
				return vis;

			prv->oldclusters[0] = viewclusters[0];
			prv->oldclusters[1] = viewclusters[1];
		}
		else
		{
			prv->oldclusters[0] = -1;
			prv->oldclusters[1] = -1;
		}

		if (r_novis.ival == 2)
			return vis;

		if (r_novis.ival || r_viewcluster == -1 || !mod->vis)
		{
			// mark everything
			for (i=0 ; i<mod->numleafs ; i++)
				mod->leafs[i].visframe = vissequence;
			for (i=0 ; i<mod->numnodes ; i++)
				mod->nodes[i].visframe = vissequence;
			return vis;
		}

		if (viewclusters[1] != viewclusters[0])	// may have to combine two clusters because of solid water boundaries
		{
			vis = CM_ClusterPVS (mod, viewclusters[0], &curframevis[portal], PVM_REPLACE);
			vis = CM_ClusterPVS (mod, viewclusters[1], &curframevis[portal], PVM_MERGE);
		}
		else
			vis = CM_ClusterPVS (mod, viewclusters[0], &curframevis[portal], PVM_FAST);
		cvis[portal] = vis;
	}

	vissequence++;

	for (i=0,leaf=mod->leafs ; i<mod->numleafs ; i++, leaf++)
	{
		cluster = leaf->cluster;
		if (cluster == -1)
			continue;
		if (vis[cluster>>3] & (1<<(cluster&7)))
		{
			node = (mnode_t *)leaf;
			do
			{
				if (node->visframe == vissequence)
					break;
				node->visframe = vissequence;
				node = node->parent;
			} while (node);
		}
	}
	return vis;
}
static void Surf_RecursiveQ2WorldNode (mnode_t *node)
{
	int			c, side;
	mplane_t	*plane;
	msurface_t	*surf, **mark;
	mleaf_t		*pleaf;
	double		dot;

	int sidebit;

	if (node->contents == Q2CONTENTS_SOLID)
		return;		// solid

	if (node->visframe != vissequence)
		return;
	if (R_CullBox (node->minmaxs, node->minmaxs+3))
		return;

// if a leaf node, draw stuff
	if (node->contents != -1)
	{
		pleaf = (mleaf_t *)node;

		// check for door connected areas
		if (! (r_refdef.areabits[pleaf->area>>3] & (1<<(pleaf->area&7)) ) )
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
				(*mark)->visframe = scenesequence;
				mark++;
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
	Surf_RecursiveQ2WorldNode (node->children[side]);

	// draw stuff
	for ( c = node->numsurfaces, surf = currentmodel->surfaces + node->firstsurface; c ; c--, surf++)
	{
		if (surf->visframe != scenesequence)
			continue;

		if ( (surf->flags & SURF_PLANEBACK) != sidebit )
			continue;		// wrong side

		surf->visframe = 0;//scenesequence+1;//-1;

		Surf_RenderDynamicLightmaps (surf);

		surf->sbatch->mesh[surf->sbatch->meshes++] = surf->mesh;
	}


// recurse down the back side
	Surf_RecursiveQ2WorldNode (node->children[!side]);
}
#endif


static void CM_PrepareFrame(model_t *mod, refdef_t *refdef, int area, int viewclusters[2], pvsbuffer_t *vis, qbyte **entvis_out, qbyte **surfvis_out)
{
	qbyte *surfvis, *entvis;
//	qbyte *frustumvis;

	if (vis->buffersize < mod->pvsbytes)
		vis->buffer = BZ_Realloc(vis->buffer, vis->buffersize=mod->pvsbytes);
	frustumvis = vis->buffer;
	memset(frustumvis, 0, mod->pvsbytes);

	VectorCopy (r_refdef.vieworg, modelorg);

	scenesequence++;
#ifdef Q3BSPS
	if (mod->fromgame == fg_quake3)
	{
		entvis = surfvis = R_MarkLeaves_Q3 (mod, viewclusters);
		Surf_RecursiveQ3WorldNode (mod->nodes, (1<<r_refdef.frustum_numworldplanes)-1);
	}
	else
#endif
#ifdef Q2BSPS
	if (mod->fromgame == fg_quake2)
	{
		entvis = surfvis = R_MarkLeaves_Q2 (mod, viewclusters);
		Surf_RecursiveQ2WorldNode (mod->nodes);
	}
	else
#endif
	{
		entvis = surfvis = NULL;
	}

	*surfvis_out = frustumvis;
	*entvis_out = entvis;
}
#endif




qboolean QDECL Mod_LoadQ2BrushModel (model_t *mod, void *buffer, size_t fsize)
{
	mod->fromgame = fg_quake2;
	return CM_LoadMap(mod, buffer, fsize, true) != NULL;
}

void CM_Init(void)	//register cvars.
{
#define MAPOPTIONS "Map Cvar Options"
	Cvar_Register(&map_noareas, MAPOPTIONS);
	Cvar_Register(&map_noCurves, MAPOPTIONS);
	Cvar_Register(&map_autoopenportals, MAPOPTIONS);
	Cvar_Register(&q3bsp_surf_meshcollision_flag, MAPOPTIONS);
	Cvar_Register(&q3bsp_surf_meshcollision_force, MAPOPTIONS);
	Cvar_Register(&q3bsp_mergeq3lightmaps, MAPOPTIONS);
	Cvar_Register(&q3bsp_ignorestyles, MAPOPTIONS);
	Cvar_Register(&q3bsp_bihtraces, MAPOPTIONS);
	Cvar_Register(&r_subdivisions, MAPOPTIONS);

	CM_InitBoxHull ();
}
#endif
