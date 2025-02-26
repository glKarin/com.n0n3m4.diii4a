#include "quakedef.h"
#ifndef SERVERONLY
#include "glquake.h"
#endif
#include "com_mesh.h"
#include "com_bih.h"

//BIH traces are capable of checking each object only once, thus our collision structures can be fully const.
//this also allows traces to be threaded, if we can avoid the temptation to (ab)use globals.
//so don't use any globals!

struct bihnode_s
{
	//in a bih tree there are two values per node instead of a kd-tree's single midpoint
	//this allows the two sides to overlap, which prevents the need to chop large objects into multiple leafs
	//(it also allows gaps in the middle, which can further skip recursion)
	enum bihtype_e type;
	union
	{
		struct{
			int firstchild;
			int numchildren;
		} group;
#ifdef BIH_USEBVH
		struct{
			int firstchild;
			vec3_t min, max;
			float cmin;
			float cmax;
		} bvhnode;
#endif
#ifdef BIH_USEBIH
		struct{
			int firstchild;
			float cmin[2];
			float cmax[2];
		} bihnode;
#endif
		struct bihdata_s data;
	};
};
struct bihbox_s {
	vec3_t min;
	vec3_t max;
};
struct bihtrace_s
{
	struct bihbox_s bounds;
	struct bihbox_s size;
	vec3_t expand;
	vec3_t up;	//capsule's upwards direction
	vec3_t capsulesize;	//radius, up, down
	qboolean negativedir[3];

	enum {
		shape_ispoint,
		shape_isbox,
		shape_iscapsule,
	} shape;
	unsigned int hitcontents;
	vec3_t startpos;	//bounds.[min|max]
	vec3_t totalmove;
	vec3_t endpos;	//bounds.[min|max]
	trace_t trace;
};

static const q2mapsurface_t	nullsurface;

static qboolean BIH_BoundsIntersect (const vec3_t mins1, const vec3_t maxs1, const vec3_t mins2, const vec3_t maxs2)
{
	return (mins1[0] <= maxs2[0] && mins1[1] <= maxs2[1] && mins1[2] <= maxs2[2] &&
		 maxs1[0] >= mins2[0] && maxs1[1] >= mins2[1] && maxs1[2] >= mins2[2]);
}

#define PlaneDiff(point,plane) (((plane)->type < 3 ? (point)[(plane)->type] : DotProduct((point), (plane)->normal)) - (plane)->dist)

#define boxdist(dist,plane)	\
		default:			\
		case shape_isbox:	\
			/* FIXME: needs special case for axial */	\
			for (j=0 ; j<3 ; j++)	\
			{	\
				if (plane->normal[j] < 0)	\
					ofs[j] = tr->size.max[j];	\
				else	\
					ofs[j] = tr->size.min[j];	\
			}	\
			dist = DotProduct (ofs, plane->normal);	\
			dist = plane->dist - dist;	\
			break;
#define capsuledist(dist,plane)					\
		case shape_iscapsule:								\
			dist = DotProduct(tr->up, plane->normal);		\
			dist = dist*(tr->capsulesize[(dist<0)?1:2]) - tr->capsulesize[0];	\
			dist = plane->dist - dist;						\
			break;
#define pointdist(dist,plane)	\
		case shape_ispoint:		\
			dist = plane->dist;	\
			break;
#define calcdist(dist,plane) switch(tr->shape)	{	\
		boxdist(dist,plane)	\
		capsuledist(dist,plane)	\
		pointdist(dist,plane)	\
		}

#define	DIST_EPSILON	(0.03125)
/*static void BIH_ClipBoxToPlanes (struct bihtrace_s *fte_restrict tr, vec3_t plmins, vec3_t plmaxs, const mplane_t *plane, int numplanes, const q2csurface_t *surf)
{
	int			i, j;
	const mplane_t	*clipplane;
	float		dist;
	float		enterfrac, leavefrac;
	vec3_t		ofs;
	float		d1, d2;
	qboolean	getout, startout;
	float		f;
	static const mplane_t	bboxplanes[6] = //we change the dist, but nothing else
	{
		{{1, 0, 0}},
		{{0, 1, 0}},
		{{0, 0, 1}},
		{{-1, 0, 0}},
		{{0, -1, 0}},
		{{0, 0, -1}},
	};
	size_t u;

	float nearfrac=0;
	enterfrac = -1;
	leavefrac = 2;
	clipplane = NULL;

	getout = false;
	startout = false;

	for (i=0 ; i<numplanes ; i++, plane++)
	{
		calcdist(dist, plane)

		d1 = DotProduct (tr->startpos, plane->normal) - dist;
		d2 = DotProduct (tr->endpos, plane->normal) - dist;

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
			}
		}
		else
		{	// leave
			f = (d1) / (d1-d2);
			if (f < leavefrac)
				leavefrac = f;
		}
	}

	if (tr->shape)	//bevel the brush axially (to match the player's bbox), in case that wasn't already done
	for (i=0, plane = bboxplanes; i<countof(bboxplanes) ; i++, plane++)
	{
		if (i < 3)
		{	//positive normal
			dist = tr->size.min[i];
			dist = plmaxs[i] - dist;
			d1 = tr->startpos[i] - dist;
			d2 = tr->endpos[i] - dist;
		}
		else
		{	//negative normal
			j = i-3;
			dist = -tr->size.max[j];
			dist = -plmins[j] - dist;
			d1 = -tr->startpos[j] - dist;
			d2 = -tr->endpos[j] - dist;
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
		tr->trace.startsolid = true;
		if (!getout)
			tr->trace.allsolid = true;
		return;
	}
	if (enterfrac <= leavefrac)
	{
		if (enterfrac > -1 && enterfrac <= tr->trace.truefraction)
		{
			if (enterfrac < 0)
				enterfrac = 0;

			tr->trace.fraction = nearfrac;
			tr->trace.truefraction = enterfrac;

			if ((u=clipplane-bboxplanes) < countof(bboxplanes))	//hit one of the bbox planes. get the proper plane dist.
				tr->trace.plane.dist = (u < 3)?plmaxs[u]:-plmins[u-3];
			else
				tr->trace.plane.dist = clipplane->dist;
			VectorCopy(clipplane->normal, tr->trace.plane.normal);
			tr->trace.surface = surf;
			tr->trace.contents = surf->value;
		}
	}
}*/
static void BIH_ClipToTriangle(struct bihtrace_s *fte_restrict tr, const struct bihdata_s *info)
{
	int i, j;
	float *p1, *p2, *p3;
	vec3_t edge1, edge2, edge3;
	mplane_t planes[5];
	const mplane_t *plane;
	vec3_t tmins, tmaxs;

	const mplane_t	*clipplane;
	float		dist;
	float		enterfrac, leavefrac, nearfrac;
	vec3_t		ofs;
	float		d1, d2;
	qboolean	getout, startout;
	float		f;
	static const mplane_t	bboxplanes[6] =
	{
		{{1, 0, 0}},
		{{0, 1, 0}},
		{{0, 0, 1}},
		{{-1, 0, 0}},
		{{0, -1, 0}},
		{{0, 0, -1}},
	};
	size_t u;

	p1 = info->tri.xyz[info->tri.indexes[0]];
	p2 = info->tri.xyz[info->tri.indexes[1]];
	p3 = info->tri.xyz[info->tri.indexes[2]];

	//determine the triangle extents, and skip the triangle if we're completely out of bounds
	for (j = 0; j < 3; j++)
	{
		tmins[j] = p1[j];
		if (tmins[j] > p2[j])
			tmins[j] = p2[j];
		if (tmins[j] > p3[j])
			tmins[j] = p3[j];
		if (tr->bounds.max[j]+(1/8.f) < tmins[j])
			return;
		tmaxs[j] = p1[j];
		if (tmaxs[j] < p2[j])
			tmaxs[j] = p2[j];
		if (tmaxs[j] < p3[j])
			tmaxs[j] = p3[j];
		if (tr->bounds.min[j]-(1/8.f) > tmaxs[j])
			return;
	}

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

	nearfrac=0;
	enterfrac = -1;
	leavefrac = 2;
	clipplane = NULL;

	getout = false;
	startout = false;

	for (i=0, plane = planes ; i<countof(planes) ; i++, plane++)
	{
		calcdist(dist, plane)

		d1 = DotProduct (tr->startpos, plane->normal) - dist;
		d2 = DotProduct (tr->endpos, plane->normal) - dist;

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
			}
		}
		else
		{	// leave
			f = (d1) / (d1-d2);
			if (f < leavefrac)
				leavefrac = f;
		}
	}

	if (tr->shape)	//bevel the brush axially (to match the player's bbox), in case that wasn't already done
	for (i=0, plane = bboxplanes; i<countof(bboxplanes) ; i++, plane++)
	{
		if (i < 3)
		{	//positive normal
			dist = tr->size.min[i];
			dist = tmaxs[i] - dist;
			d1 = tr->startpos[i] - dist;
			d2 = tr->endpos[i] - dist;
		}
		else
		{	//negative normal
			j = i-3;
			dist = -tr->size.max[j];
			dist = -tmins[j] - dist;
			d1 = -tr->startpos[j] - dist;
			d2 = -tr->endpos[j] - dist;
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
		tr->trace.startsolid = true;
		if (!getout)
			tr->trace.allsolid = true;
		return;
	}
	if (enterfrac <= leavefrac)
	{
		if (enterfrac > -1 && enterfrac <= tr->trace.truefraction)
		{
			if (enterfrac < 0)
				enterfrac = 0;

			tr->trace.fraction = nearfrac;
			tr->trace.truefraction = enterfrac;

			if ((u=clipplane-bboxplanes) < countof(bboxplanes))	//hit one of the bbox planes. get the proper plane dist.
				tr->trace.plane.dist = (u < 3)?tmaxs[u]:-tmins[u-3];
			else
				tr->trace.plane.dist = clipplane->dist;
			VectorCopy(clipplane->normal, tr->trace.plane.normal);
			tr->trace.surface = &nullsurface.c;
			tr->trace.contents = info->contents;
		}
	}
}

static void BIH_TestToTriangle(struct bihtrace_s *fte_restrict tr, const struct bihdata_s *info)
{
	int j;
	float *p1, *p2, *p3;
	vec3_t edge1, edge2, edge3;
	mplane_t planes[5];
	const mplane_t *plane;
	vec3_t tmins, tmaxs;

	int			i;
	float		dist;
	vec3_t		ofs;
	float		d1;
	static const mplane_t	bboxplanes[6] = //we change the dist, but nothing else
	{
		{{1, 0, 0}},
		{{0, 1, 0}},
		{{0, 0, 1}},
		{{-1, 0, 0}},
		{{0, -1, 0}},
		{{0, 0, -1}},
	};

	p1 = info->tri.xyz[info->tri.indexes[0]];
	p2 = info->tri.xyz[info->tri.indexes[1]];
	p3 = info->tri.xyz[info->tri.indexes[2]];

	//determine the triangle extents, and skip the triangle if we're completely out of bounds
	for (j = 0; j < 3; j++)
	{
		tmins[j] = p1[j];
		if (tmins[j] > p2[j])
			tmins[j] = p2[j];
		if (tmins[j] > p3[j])
			tmins[j] = p3[j];
		if (tr->bounds.max[j]+(1/8.f) < tmins[j])
			return;
		tmaxs[j] = p1[j];
		if (tmaxs[j] < p2[j])
			tmaxs[j] = p2[j];
		if (tmaxs[j] < p3[j])
			tmaxs[j] = p3[j];
		if (tr->bounds.min[j]-(1/8.f) > tmaxs[j])
			return;
	}

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

	for (i=0, plane = planes; i<countof(planes) ; i++, plane++)
	{
		calcdist(dist, plane)
		d1 = DotProduct (tr->startpos, plane->normal) - dist;
		if (d1 > 0)
			return;
	}

	if (tr->shape)	//bevel the brush axially (to match the player's bbox), in case that wasn't already done
	for (i=0, plane = bboxplanes; i<countof(bboxplanes) ; i++, plane++)
	{
		if (i < 3)
		{	//positive normal
			dist = tr->size.min[i];
			dist = tmaxs[i] - dist;
			d1 = tr->startpos[i] - dist;
		}
		else
		{	//negative normal
			j = i-3;
			dist = -tr->size.max[j];
			dist = -tmins[j] - dist;
			d1 = -tr->startpos[j] - dist;
		}

		// if completely in front of face, no intersection
		if (d1 > 0)
			return;
	}

	tr->trace.startsolid = tr->trace.allsolid = true;
	tr->trace.contents |= info->contents;
}

#if defined(Q2BSPS) || defined(Q3BSPS)
static void BIH_ClipBoxToBrush (struct bihtrace_s *fte_restrict tr, const q2cbrush_t *brush)
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

		calcdist(dist, plane)
		d1 = DotProduct (tr->startpos, plane->normal) - dist;
		d2 = DotProduct (tr->endpos, plane->normal) - dist;

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
		tr->trace.startsolid = true;
		if (!getout)
			tr->trace.allsolid = true;
		return;
	}
	if (enterfrac <= leavefrac)
	{
		if (enterfrac > -1 && enterfrac <= tr->trace.truefraction)
		{
			if (enterfrac < 0)
				enterfrac = 0;

			tr->trace.fraction = nearfrac;
			tr->trace.truefraction = enterfrac;

			tr->trace.plane.dist = clipplane->dist;
			VectorCopy(clipplane->normal, tr->trace.plane.normal);
			tr->trace.surface = &(leadside->surface->c);
			tr->trace.contents = brush->contents;
		}
	}
}
static void BIH_TestBoxInBrush (struct bihtrace_s *fte_restrict tr, q2cbrush_t *brush)
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

		calcdist(dist, plane)
		d1 = DotProduct (tr->startpos, plane->normal) - dist;

		// if completely in front of face, no intersection
		if (d1 > 0)
			return;
	}

	// inside this brush
	tr->trace.startsolid = tr->trace.allsolid = true;
	tr->trace.contents |= brush->contents;
}
#endif

#ifdef Q3BSPS
static void BIH_ClipBoxToPatch (struct bihtrace_s *fte_restrict tr, q2cbrush_t *brush)
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

		calcdist(dist, plane)
		d1 = DotProduct (tr->startpos, plane->normal) - dist;
		d2 = DotProduct (tr->endpos, plane->normal) - dist;

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
		tr->trace.startsolid = true;
		return;		// original point is inside the patch
	}

	if (nearfrac <= leavefrac)
	{
		if (leadside && leadside->surface
			&& enterfrac <= tr->trace.truefraction)
		{
			if (enterfrac < 0)
				enterfrac = 0;
			tr->trace.truefraction = enterfrac;
			tr->trace.fraction = nearfrac;
			tr->trace.plane.dist = clipplane->dist;
			VectorCopy(clipplane->normal, tr->trace.plane.normal);
			tr->trace.surface = &leadside->surface->c;
			tr->trace.contents = brush->contents;
		}
		else if (enterfrac < tr->trace.truefraction)
			leavefrac=0;
	}
}
static void BIH_TestBoxInPatch (struct bihtrace_s *fte_restrict tr, q2cbrush_t *brush)
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

		switch(tr->shape)
		{
		default:
		case shape_isbox:
			for (j=0 ; j<3 ; j++)
			{
				if (plane->normal[j] < 0)
					ofs[j] = tr->size.max[j], ofs2[j] = tr->size.min[j];
				else
					ofs[j] = tr->size.min[j], ofs2[j] = tr->size.max[j];
			}

			dist = DotProduct (ofs, plane->normal);
			thickness = DotProduct (ofs2, plane->normal)-dist;
			dist = plane->dist - dist;
			break;
		case shape_iscapsule:
			dist = DotProduct(tr->up, plane->normal);
			thickness = dist*(tr->capsulesize[(dist<0)?2:1]) + tr->capsulesize[0]*2;
			dist = dist*(tr->capsulesize[(dist<0)?1:2]) - tr->capsulesize[0];
			dist = plane->dist - dist;
			break;
		case shape_ispoint:
			dist = plane->dist;
			thickness = 0;
			break;
		}

		d1 = DotProduct (tr->startpos, plane->normal) - dist;

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

		calcdist(dist, plane)
		d1 = DotProduct (tr->startpos, plane->normal) - dist;

		// if completely in front of face, no intersection
		if (d1 > 0)
			return;
	}

	// inside this patch
	tr->trace.startsolid = tr->trace.allsolid = true;
	tr->trace.contents = brush->contents;
}
#endif


static void BIH_RecursiveTrace (struct bihtrace_s *fte_restrict tr, const struct bihnode_s *fte_restrict node, const struct bihbox_s *fte_restrict movesubbounds, const struct bihbox_s *fte_restrict nodebox)
{
	//if the tree were 1d, we wouldn't need to be so careful with the bounds, but if the trace is long then we want to avoid hitting all surfaces within that entire-map-encompassing move aabb
	switch(node->type)
	{	//leaf
#if defined(Q2BSPS) || defined(Q3BSPS)
	case BIH_BRUSH:
		if (node->data.contents & tr->hitcontents)
		{
			q2cbrush_t *b = node->data.brush;
			if (BIH_BoundsIntersect(b->absmins, b->absmaxs, movesubbounds->min, movesubbounds->max))
				BIH_ClipBoxToBrush (tr, b);
		}
		return;
#endif
#ifdef Q3BSPS
	case BIH_PATCHBRUSH:
		if (node->data.contents & tr->hitcontents)
		{
			q2cbrush_t *b = node->data.patchbrush;
			if (BIH_BoundsIntersect(b->absmins, b->absmaxs, movesubbounds->min, movesubbounds->max))
				BIH_ClipBoxToPatch (tr, b);
		}
		return;
	case BIH_TRISOUP:
		/*if (node->data.contents & tr->hitcontents)
		{
			q3cmesh_t *cmesh = node->data.cmesh;
			if (BIH_BoundsIntersect(cmesh->absmins, cmesh->absmaxs, movesubbounds->min, movesubbounds->max))
				Mod_Trace_Trisoup_(cmesh->xyz_array, cmesh->indicies, cmesh->numincidies, trace_start, trace_end, trace_mins, trace_maxs, &trace_trace, &cmesh->surface->c);
		}*/
		return;
#endif
	case BIH_TRIANGLE:
		if (node->data.contents & tr->hitcontents)
			BIH_ClipToTriangle(tr, &node->data);
		return;
	case BIH_MODEL:
		{
			trace_t sub;
			vec3_t start_l;
			vec3_t end_l;

			model_t *submod = node->data.mesh.model;

			if (submod->loadstate != MLS_LOADED)
			{
				static float throttle;
				COM_AssertMainThread("BIH_RecursiveTrace embedded model reloading");
				if (submod->loadstate == MLS_NOTLOADED)	//pull it back in if it was flushed.
				Mod_LoadModel(submod, MLV_WARN);
				while(submod->loadstate == MLS_LOADING)
					COM_WorkerPartialSync(submod, &submod->loadstate, MLS_LOADING);
				if (submod->loadstate != MLS_LOADED)
				{
					Con_ThrottlePrintf(&throttle, 1, "BIH: embedded model %s failed to load\n", submod->name);
					return; //something bad happened...
				}
				Con_DPrintf("BIH: embedded model ^[%s\\modelviewer\\%s^] now loading\n", submod->name,submod->name);
			}

			VectorSubtract (tr->startpos, node->data.mesh.tr->origin, start_l);
			VectorSubtract (tr->endpos, node->data.mesh.tr->origin, end_l);
			submod->funcs.NativeTrace(submod, 0, NULLFRAMESTATE, node->data.mesh.tr->axis, start_l, end_l, tr->size.min, tr->size.max, tr->shape==shape_iscapsule, tr->hitcontents, &sub);

			if (sub.truefraction < tr->trace.truefraction)
			{
				tr->trace.truefraction = sub.truefraction;
				tr->trace.fraction = sub.fraction;
				tr->trace.plane.dist = sub.plane.dist;
				VectorCopy(sub.plane.normal, tr->trace.plane.normal);
				tr->trace.surface = sub.surface;
				tr->trace.contents = sub.contents;
				tr->trace.startsolid |= sub.startsolid;
				tr->trace.allsolid = sub.allsolid;
				VectorAdd (sub.endpos, node->data.mesh.tr->origin, tr->trace.endpos);
			}
			else
			{
				tr->trace.startsolid |= sub.startsolid;
				tr->trace.allsolid &= sub.allsolid;
			}
		}
		return;
	case BIH_GROUP:
		{
			int i;
			for (i = 0; i < node->group.numchildren; i++)
				BIH_RecursiveTrace(tr, node+node->group.firstchild+i, movesubbounds, nodebox);
		}
		return;
#ifdef BIH_USEBIH
	case BIH_X:
	case BIH_Y:
	case BIH_Z:
		{
			struct bihbox_s bounds;
			struct bihbox_s newbounds;
			float distnear, distfar, nearfrac, farfrac, min, max;
			unsigned int axis = node->type-BIH_X, child, a, s;
			vec3_t points[2];

			if (!tr->totalmove[axis])
			{	//doesn't move with respect to this axis. don't allow infinities.
				for (child = 0; child < 2; child++)
				{	//only recurse if we are actually within the child
					min = node->bihnode.cmin[child] - tr->expand[axis];
					max = node->bihnode.cmax[child] + tr->expand[axis];
					if (min <= tr->startpos[axis] && tr->startpos[axis] <= max)
					{
						bounds = *nodebox;
						bounds.min[axis] = min;
						bounds.max[axis] = max;
						BIH_RecursiveTrace(tr, node+node->bihnode.firstchild+child, movesubbounds, &bounds);
					}
				}
			}
			else if (tr->negativedir[axis])
			{	//trace goes from right to left so favour the right.
				bounds = *nodebox;
				for (child = 2; child-- > 0;)
				{
					bounds.min[axis] = node->bihnode.cmin[child] - tr->expand[axis];
					bounds.max[axis] = node->bihnode.cmax[child] + tr->expand[axis];	//expand the bounds according to the player's size

					if (!BIH_BoundsIntersect(movesubbounds->min, movesubbounds->max, bounds.min, bounds.max))
						continue;
//					if (movesubbounds->max[axis] < bounds.min[axis])
//						continue;	//(clipped) move bounds is outside this child
//					if (bounds.max[axis] < movesubbounds->min[axis])
//						continue;	//(clipped) move bounds is outside this child

					distnear = bounds.max[axis] - tr->startpos[axis];
					nearfrac = distnear/tr->totalmove[axis];
					if (nearfrac <= tr->trace.truefraction)
					{
						VectorMA(tr->startpos, nearfrac, tr->totalmove, points[0]);	//clip the new movebounds (this is more to clip the other axis too)
						distfar = bounds.min[axis] - tr->startpos[axis];
						farfrac = distfar/tr->totalmove[axis];
						VectorMA(tr->startpos, farfrac, tr->totalmove, points[1]);	//clip the new movebounds (this is more to clip the other axis too)

						for (a = 0; a < 3; a++)
						{
							s = points[0][a] > points[1][a];
							newbounds.min[a] = max(movesubbounds->min[a], points[s][a] - tr->expand[a]);
							newbounds.max[a] = min(movesubbounds->max[a], points[!s][a] + tr->expand[a]);
						}
						BIH_RecursiveTrace(tr, node+node->bihnode.firstchild+child, &newbounds, &bounds);
					}
				}
			}
			else
			{	//trace goes from left to right
				bounds = *nodebox;
				for (child = 0; child < 2; child++)
				{
					bounds.min[axis] = node->bihnode.cmin[child] - tr->expand[axis];
					bounds.max[axis] = node->bihnode.cmax[child] + tr->expand[axis];	//expand the bounds according to the player's size

					if (!BIH_BoundsIntersect(movesubbounds->min, movesubbounds->max, bounds.min, bounds.max))
						continue;
//					if (movesubbounds->max[axis] < bounds.min[axis])
//						continue;	//(clipped) move bounds is outside this child
//					if (bounds.max[axis] < movesubbounds->min[axis])
//						continue;	//(clipped) move bounds is outside this child

					distnear = bounds.min[axis] - tr->startpos[axis];
					nearfrac = distnear/tr->totalmove[axis];
					if (nearfrac <= tr->trace.truefraction)
					{
						VectorMA(tr->startpos, nearfrac, tr->totalmove, points[0]);	//clip the new movebounds (this is more to clip the other axis too)
						distfar = bounds.max[axis] - tr->startpos[axis];
						farfrac = distfar/tr->totalmove[axis];
						VectorMA(tr->startpos, farfrac, tr->totalmove, points[1]);	//clip the new movebounds (this is more to clip the other axis too)

						for (a = 0; a < 3; a++)
						{
							s = points[0][a] > points[1][a];
							newbounds.min[a] = max(movesubbounds->min[a], points[s][a] - tr->expand[a]);
							newbounds.max[a] = min(movesubbounds->max[a], points[!s][a] + tr->expand[a]);
						}
						BIH_RecursiveTrace(tr, node+node->bihnode.firstchild+child, &newbounds, &bounds);
					}
				}
			}
		}
		return;
#endif
#ifdef BIH_USEBVH
	case BVH_X:
	case BVH_Y:
	case BVH_Z:
		{
			struct bihbox_s bounds;
			struct bihbox_s newbounds;
			float distnear, distfar, nearfrac, farfrac, min, max;
			unsigned int axis = node->type-BVH_X, child, a, s;
			vec3_t points[2];

			if (!tr->totalmove[axis])
			{	//doesn't move with respect to this axis. don't allow infinities.
				for (child = 0; child < 2; child++)
				{	//only recurse if we are actually within the child
					if (child == 0)
					{
						min = node->bvhnode.min[axis] - tr->expand[axis];
						max = node->bvhnode.cmax + tr->expand[axis];
					}
					else
					{
						min = node->bvhnode.cmin - tr->expand[axis];
						max = node->bvhnode.max[axis] + tr->expand[axis];
					}
					if (min <= tr->startpos[axis] && tr->startpos[axis] <= max)
					{
						VectorCopy(node->bvhnode.min, bounds.min);
						VectorCopy(node->bvhnode.max, bounds.max);
						bounds.min[axis] = min;
						bounds.max[axis] = max;
						CM_RecursiveBIHTrace(tr, node+node->bvhnode.firstchild+child, movesubbounds, &bounds);
					}
				}
			}
			else if (tr->negativedir[axis])
			{	//trace goes from right to left so favour the right.
				VectorCopy(node->bvhnode.min, bounds.min);
				VectorCopy(node->bvhnode.max, bounds.max);
				for (child = 2; child-- > 0;)
				{
					if (child == 0)
					{
						bounds.min[axis] = node->bvhnode.min[axis] - tr->expand[axis];
						bounds.max[axis] = node->bvhnode.cmax + tr->expand[axis];	//expand the bounds according to the player's size
					}
					else
					{
						bounds.min[axis] = node->bvhnode.cmin - tr->expand[axis];
						bounds.max[axis] = node->bvhnode.max[axis] + tr->expand[axis];	//expand the bounds according to the player's size
					}

					if (!BIH_BoundsIntersect(movesubbounds->min, movesubbounds->max, bounds.min, bounds.max))
						continue;
//					if (movesubbounds->max[axis] < bounds.min[axis])
//						continue;	//(clipped) move bounds is outside this child
//					if (bounds.max[axis] < movesubbounds->min[axis])
//						continue;	//(clipped) move bounds is outside this child

					distnear = bounds.max[axis] - tr->startpos[axis];
					nearfrac = (distnear+DIST_EPSILON)/tr->totalmove[axis];
					if (nearfrac <= trace_truefraction)
					{
						VectorMA(tr->startpos, nearfrac, tr->totalmove, points[0]);	//clip the new movebounds (this is more to clip the other axis too)
						distfar = bounds.min[axis] - tr->startpos[axis];
						farfrac = (distfar-DIST_EPSILON)/tr->totalmove[axis];
						VectorMA(tr->startpos, farfrac, tr->totalmove, points[1]);	//clip the new movebounds (this is more to clip the other axis too)

						for (a = 0; a < 3; a++)
						{
							s = points[0][a] > points[1][a];
							newbounds.min[a] = max(movesubbounds->min[a], points[s][a] - tr->expand[a]);
							newbounds.max[a] = min(movesubbounds->max[a], points[!s][a] + tr->expand[a]);
						}
						CM_RecursiveBIHTrace(tr, node+node->bvhnode.firstchild+child, &newbounds, &bounds);
					}
				}
			}
			else
			{	//trace goes from left to right
				VectorCopy(node->bvhnode.min, bounds.min);
				VectorCopy(node->bvhnode.max, bounds.max);
				for (child = 0; child < 2; child++)
				{
					if (child == 0)
					{
						bounds.min[axis] = node->bvhnode.min[axis] - tr->expand[axis];
						bounds.max[axis] = node->bvhnode.cmax + tr->expand[axis];	//expand the bounds according to the player's size
					}
					else
					{
						bounds.min[axis] = node->bvhnode.cmin - tr->expand[axis];
						bounds.max[axis] = node->bvhnode.max[axis] + tr->expand[axis];	//expand the bounds according to the player's size
					}

					if (!BIH_BoundsIntersect(movesubbounds->min, movesubbounds->max, bounds.min, bounds.max))
						continue;
//					if (movesubbounds->max[axis] < bounds.min[axis])
//						continue;	//(clipped) move bounds is outside this child
//					if (bounds.max[axis] < movesubbounds->min[axis])
//						continue;	//(clipped) move bounds is outside this child

					distnear = bounds.min[axis] - tr->startpos[axis];
					nearfrac = (distnear-DIST_EPSILON)/tr->totalmove[axis];
					if (nearfrac <= trace_truefraction)
					{
						VectorMA(tr->startpos, nearfrac, tr->totalmove, points[0]);	//clip the new movebounds (this is more to clip the other axis too)
						distfar = bounds.max[axis] - tr->startpos[axis];
						farfrac = (distfar+DIST_EPSILON)/tr->totalmove[axis];
						VectorMA(tr->startpos, farfrac, tr->totalmove, points[1]);	//clip the new movebounds (this is more to clip the other axis too)

						for (a = 0; a < 3; a++)
						{
							s = points[0][a] > points[1][a];
							newbounds.min[a] = max(movesubbounds->min[a], points[s][a] - tr->expand[a]);
							newbounds.max[a] = min(movesubbounds->max[a], points[!s][a] + tr->expand[a]);
						}
						CM_RecursiveBIHTrace(tr, node+node->bvhnode.firstchild+child, &newbounds, &bounds);
					}
				}
			}
		}
		return;
#endif
	}
	FTE_UNREACHABLE;
}

//tracebox-with-no-movement, can be a little faster.
static void BIH_RecursiveTest (struct bihtrace_s *fte_restrict tr, const struct bihnode_s *fte_restrict node)
{
	//with BIH, its possible for a large child node to have a box larger than its sibling.
	switch(node->type)
	{
#if defined(Q2BSPS) || defined(Q3BSPS)
	case BIH_BRUSH:
		if (node->data.contents & tr->hitcontents)
		{
			q2cbrush_t *b = node->data.brush;
//			if (BIH_BoundsIntersect(tr->bounds.min, tr->bounds.max, b->absmins, b->absmaxs))
				BIH_TestBoxInBrush (tr, b);
		}
		return;
#endif
#ifdef Q3BSPS
	case BIH_PATCHBRUSH:
		if (node->data.contents & tr->hitcontents)
		{
			q2cbrush_t *b = node->data.patchbrush;
//			if (BIH_BoundsIntersect(tr->bounds.min, tr->bounds.max, b->absmins, b->absmaxs))
				BIH_TestBoxInPatch (tr, b);
		}
		return;
	case BIH_TRISOUP:
		/*if (node->data.contents & tr->hitcontents)
//			if (BIH_BoundsIntersect(cmesh->absmins, cmesh->absmaxs, tr->bounds.min, tr->bounds.max))
			{
				q3cmesh_t *cmesh = node->data.cmesh;
				Mod_Trace_Trisoup_(cmesh->xyz_array, cmesh->indicies, cmesh->numincidies, trace_start, trace_end, trace_mins, trace_maxs, &trace_trace, &cmesh->surface->c);
			}*/
		return;
#endif
	case BIH_TRIANGLE:
		if (node->data.contents & tr->hitcontents)
			BIH_TestToTriangle(tr, &node->data);
		return;
	case BIH_MODEL:
		{	//lame...
			trace_t sub;
			vec3_t start_l;
			vec3_t end_l;

			VectorSubtract (tr->startpos, node->data.mesh.tr->origin, start_l);
			VectorSubtract (tr->endpos, node->data.mesh.tr->origin, end_l);
			node->data.mesh.model->funcs.NativeTrace(node->data.mesh.model, 0, NULLFRAMESTATE, node->data.mesh.tr->axis, start_l, end_l, tr->size.min, tr->size.max, tr->shape==shape_iscapsule, tr->hitcontents, &sub);

			if (sub.truefraction < tr->trace.truefraction)
			{
				tr->trace.truefraction = sub.truefraction;
				tr->trace.fraction = sub.fraction;
				tr->trace.plane.dist = sub.plane.dist;
				VectorCopy(sub.plane.normal, tr->trace.plane.normal);
				tr->trace.surface = sub.surface;
				tr->trace.contents = sub.contents;
				tr->trace.startsolid |= sub.startsolid;
				tr->trace.allsolid = sub.allsolid;
				VectorAdd (sub.endpos, node->data.mesh.tr->origin, tr->trace.endpos);
			}
			else
			{
				tr->trace.startsolid |= sub.startsolid;
				tr->trace.allsolid &= sub.allsolid;
			}
		}
		return;
	case BIH_GROUP:
		{
			int i;
			for (i = 0; i < node->group.numchildren; i++)
			{
				BIH_RecursiveTest(tr, node+node->group.firstchild+i);
				if (tr->trace.allsolid)
					break;
			}
		}
		return;
#ifdef BIH_USEBIH
	case BIH_X:
	case BIH_Y:
	case BIH_Z:
		{	//node (x y or z)
			float min; float max;
			int axis = node->type - BIH_X;
			min = node->bihnode.cmin[0] - tr->expand[axis];
			max = node->bihnode.cmax[0] + tr->expand[axis];	//expand the bounds according to the player's size

			//the point can potentially be within both children, or neither.
			//it doesn't really matter which order we walk the tree, just be sure to do it efficiently.
			if (min <= tr->startpos[axis] && tr->startpos[axis] <= max)
			{
				BIH_RecursiveTest(tr, node+node->bihnode.firstchild+0);
				if (tr->trace.allsolid)
					return;
			}

			min = node->bihnode.cmin[1] - tr->expand[axis];
			max = node->bihnode.cmax[1] + tr->expand[axis];
			if (min <= tr->startpos[axis] && tr->startpos[axis] <= max)
				BIH_RecursiveTest(tr, node+node->bihnode.firstchild+1);
		}
		return;
#endif
#ifdef BIH_USEBVH
	case BVH_X:
	case BVH_Y:
	case BVH_Z:
		{	//node (x y or z)
			float min; float max;
			int axis = node->type - BVH_X;
			min = node->bvhnode.min[axis] - tr->expand[axis];
			max = node->bvhnode.cmax + tr->expand[axis];	//expand the bounds according to the player's size

			//the point can potentially be within both children, or neither.
			//it doesn't really matter which order we walk the tree, just be sure to do it efficiently.
			if (min <= tr->startpos[axis] && tr->startpos[axis] <= max)
			{
				CM_RecursiveBIHTest(tr, node+node->bvhnode.firstchild+0);
				if (trace_trace.allsolid)
					return;
			}

			min = node->bvhnode.cmin - tr->expand[axis];
			max = node->bvhnode.max[axis] + tr->expand[axis];
			if (min <= tr->startpos[axis] && tr->startpos[axis] <= max)
				CM_RecursiveBIHTest(tr, node+node->bvhnode.firstchild+1);
		}
		return;
#endif
	}
	FTE_UNREACHABLE;
}
static qboolean BIH_Trace(model_t *model, int forcehullnum, const framestate_t *framestate, const vec3_t axis[3], const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, qboolean capsule, unsigned int contents, trace_t *out_trace)
{
	int		i;
	vec3_t point;
	struct bihtrace_s tr;

	if (axis)
	{	//rotate everything
		VectorSet(tr.startpos, DotProduct(start, axis[0]), DotProduct(start, axis[1]), DotProduct(start, axis[2]));
		VectorSet(tr.endpos,   DotProduct(end,   axis[0]), DotProduct(end,   axis[1]), DotProduct(end,   axis[2]));
		VectorSet(tr.up, axis[0][2], -axis[1][2], axis[2][2]);
	}
	else
	{	//axial bboxes. woo.
		VectorCopy(start, tr.startpos);
		VectorCopy(end, tr.endpos);
		VectorSet(tr.up, 0, 0, 1);
	}


	// fill in a default trace
	memset (&tr.trace, 0, sizeof(tr.trace));
	tr.trace.fraction = tr.trace.truefraction = 1;
	tr.trace.surface = &(nullsurface.c);

	if (model)	// map is loaded...
	{
		tr.hitcontents = contents;
		VectorCopy (mins, tr.size.min);
		VectorCopy (maxs, tr.size.max);

		if (1)	//center the point of the trace in the middle...
		{
			VectorAdd(tr.size.max, tr.size.min, point);
			VectorScale(point, 0.5, point);

			VectorAdd(tr.startpos, point, tr.startpos);
			VectorAdd(tr.endpos, point, tr.endpos);
			VectorSubtract(tr.size.min, point, tr.size.min);
			VectorSubtract(tr.size.max, point, tr.size.max);
		}



		// build a bounding box of the entire move (for patches)
		ClearBounds (tr.bounds.min, tr.bounds.max);

		//determine the type of trace that we're going to use, and the max extents
		if (tr.size.min[0] == 0 && tr.size.min[1] == 0 && tr.size.min[2] == 0 && tr.size.max[0] == 0 && tr.size.max[1] == 0 && tr.size.max[2] == 0)
		{
			tr.shape = shape_ispoint;
			VectorSet (tr.expand, 1/32.0, 1/32.0, 1/32.0);
			//acedemic
			AddPointToBounds (tr.startpos, tr.bounds.min, tr.bounds.max);
			AddPointToBounds (tr.endpos, tr.bounds.min, tr.bounds.max);
		}
		else if (capsule)
		{
			float ext;
			tr.shape = shape_iscapsule;
			//determine the capsule sizes
			tr.capsulesize[0] = ((tr.size.max[0]-tr.size.min[0]) + (tr.size.max[1]-tr.size.min[1]))/4.0;
			tr.capsulesize[1] = tr.size.max[2];
			tr.capsulesize[2] = tr.size.min[2];
			//make sure the mins_z/maxs_z isn't screwed.
	//		if (tr.capsulesize[1]-tr.capsulesize[2] < tr.capsulesize[0])
	//			tr.capsulesize[1] = tr.capsulesize[0]+tr.capsulesize[2];
			ext = (tr.capsulesize[1] > -tr.capsulesize[2])?tr.capsulesize[1]:-tr.capsulesize[2];
			tr.capsulesize[1] -= tr.capsulesize[0];
			tr.capsulesize[2] += tr.capsulesize[0];
			tr.expand[0] = ext+1;
			tr.expand[1] = ext+1;
			tr.expand[2] = ext+1;

			//determine the total range
			VectorSubtract (tr.startpos, tr.expand, point);
			AddPointToBounds (point, tr.bounds.min, tr.bounds.max);
			VectorAdd (tr.startpos, tr.expand, point);
			AddPointToBounds (point, tr.bounds.min, tr.bounds.max);
			VectorSubtract (tr.endpos, tr.expand, point);
			AddPointToBounds (point, tr.bounds.min, tr.bounds.max);
			VectorAdd (tr.endpos, tr.expand, point);
			AddPointToBounds (point, tr.bounds.min, tr.bounds.max);
		}
		else
		{
			VectorAdd (tr.startpos, tr.size.min, point);
			AddPointToBounds (point, tr.bounds.min, tr.bounds.max);
			VectorAdd (tr.startpos, tr.size.max, point);
			AddPointToBounds (point, tr.bounds.min, tr.bounds.max);
			VectorAdd (tr.endpos, tr.size.min, point);
			AddPointToBounds (point, tr.bounds.min, tr.bounds.max);
			VectorAdd (tr.endpos, tr.size.max, point);
			AddPointToBounds (point, tr.bounds.min, tr.bounds.max);

			tr.shape = shape_isbox;
			tr.expand[0] = ((-tr.size.min[0] > tr.size.max[0]) ? -tr.size.min[0] : tr.size.max[0])+1;
			tr.expand[1] = ((-tr.size.min[1] > tr.size.max[1]) ? -tr.size.min[1] : tr.size.max[1])+1;
			tr.expand[2] = ((-tr.size.min[2] > tr.size.max[2]) ? -tr.size.min[2] : tr.size.max[2])+1;
		}

		tr.bounds.min[0] -= 1.0;
		tr.bounds.min[1] -= 1.0;
		tr.bounds.min[2] -= 1.0;
		tr.bounds.max[0] += 1.0;
		tr.bounds.max[1] += 1.0;
		tr.bounds.max[2] += 1.0;


		for (i = 0; i < 3; i++)
			tr.negativedir[i] = (tr.endpos[i] - tr.startpos[i]) < 0;
		VectorSubtract(tr.endpos, tr.startpos, tr.totalmove);
		if (tr.startpos[0] == tr.endpos[0] && tr.startpos[1] == tr.endpos[1] && tr.startpos[2] == tr.endpos[2])
			BIH_RecursiveTest(&tr, model->cnodes);
		else
		{
			struct bihbox_s worldsize;
			VectorCopy(model->mins, worldsize.min);
			VectorCopy(model->maxs, worldsize.max);
			BIH_RecursiveTrace(&tr, model->cnodes, &tr.bounds, &worldsize);
		}

		if (tr.trace.fraction<0)
			tr.trace.fraction=0;
	}

	*out_trace = tr.trace;
#ifdef TERRAIN
	if (model->terrain)
	{	//terrain is weird.
		trace_t hmt;
		Heightmap_Trace(model, forcehullnum, framestate, NULL, tr.startpos, tr.endpos, mins, maxs, capsule, contents, &hmt);
		if (hmt.fraction < out_trace->fraction)
			*out_trace = hmt;
	}
#endif

	if (out_trace->fraction == 1)
	{
		VectorCopy (end, out_trace->endpos);
	}
	else
	{
		VectorInterpolate(start, out_trace->fraction, end, out_trace->endpos);	//too lazy to compute the endpos for each impact
		if (axis)
		{
			vec3_t iaxis[3];
			vec3_t norm;
			Matrix3x3_RM_Invert_Simple((const void *)axis, iaxis);
			VectorCopy(out_trace->plane.normal, norm);
			out_trace->plane.normal[0] = DotProduct(norm, iaxis[0]);
			out_trace->plane.normal[1] = DotProduct(norm, iaxis[1]);
			out_trace->plane.normal[2] = DotProduct(norm, iaxis[2]);

			/*just interpolate it, its easier than inverse matrix rotations*/
			VectorInterpolate(start, out_trace->fraction, end, out_trace->endpos);
		}
	}
	return out_trace->fraction != 1;
}

//simplest form. no movement, no size.
unsigned int BIH_TestContents (const struct bihnode_s *fte_restrict node, const vec3_t p)
{
restart:
	switch(node->type)
	{	//leaf
#if defined(Q2BSPS) || defined(Q3BSPS)
	case BIH_BRUSH:
		{
			q2cbrush_t *b = node->data.brush;
			q2cbrushside_t *brushside = b->brushside;
			size_t j;
			if (!BIH_BoundsIntersect(p, p, b->absmins, b->absmaxs))
				return 0;

			for ( j = 0; j < b->numsides; j++, brushside++ )
			{
				if ( PlaneDiff (p, brushside->plane) > 0 )
					return 0;
			}
			return b->contents;	//inside all planes
		}
#endif
#ifdef Q3BSPS
	case BIH_PATCHBRUSH:
		{	//patches have no contents...
			return 0;
		}
	case BIH_TRISOUP:
		{
			//trisoup has no contents... depending upon epsilons would be crazy.
			return 0;
		}
#endif
	case BIH_TRIANGLE:
		return 0;
	case BIH_MODEL:
		{
			vec3_t pos;
			VectorSubtract (p, node->data.mesh.tr->origin, pos);
			return node->data.mesh.model->funcs.PointContents(node->data.mesh.model, node->data.mesh.tr->axis, pos);
		}
	case BIH_GROUP:
		{
			int i;
			unsigned int contents = 0;
			for (i = 0; i < node->group.numchildren; i++)
				contents |= BIH_TestContents(node+node->group.firstchild+i, p);
			return contents;
		}
#ifdef BIH_USEBIH
	case BIH_X:
	case BIH_Y:
	case BIH_Z:
		{	//node (x y or z)
			unsigned int axis = node->type - BIH_X;

			//the point can potentially be within both children, or neither.
			//it doesn't really matter which order we walk the tree, just be sure to do it efficiently.
			if (node->bihnode.cmin[0] <= p[axis] && p[axis] <= node->bihnode.cmax[0])
			{
				if (node->bihnode.cmin[1] <= p[axis] && p[axis] <= node->bihnode.cmax[1])
				{	//need to walk both
					return
						BIH_TestContents(node+node->bihnode.firstchild+0, p) |
						BIH_TestContents(node+node->bihnode.firstchild+1, p);
				}
				//only need the left side.
				node = node+node->bihnode.firstchild+0;
				goto restart;
			}
			else
			{
				if (node->bihnode.cmin[1] <= p[axis] && p[axis] <= node->bihnode.cmax[1])
					;
				else
					return 0;	//walk neither.
				//only need to walk the right
				node = node+node->bihnode.firstchild+1;
				goto restart;

			}
		}
#endif
#ifdef BIH_USEBVH
	case BVH_X:
	case BVH_Y:
	case BVH_Z:
		{	//node (x y or z)
			unsigned int contents;
			unsigned int axis = node->type - BVH_X;

			//the point can potentially be within both children, or neither.
			//it doesn't really matter which order we walk the tree, just be sure to do it efficiently.
			if (node->bvhnode.min[axis] <= p[axis] && p[axis] <= node->bvhnode.cmax)
				contents = BIH_TestContents(node+node->bvhnode.firstchild+0, p);
			else
				contents = 0;

			if (node->bvhnode.cmin <= p[axis] && p[axis] <= node->bvhnode.max[axis])
				contents |= BIH_TestContents(node+node->bvhnode.firstchild+1, p);
			return contents;
		}
#endif
	}
	FTE_UNREACHABLE;
	return 0;
}
static unsigned int BIH_PointContents(struct model_s *mod, const vec3_t axis[3], const vec3_t p)
{
	unsigned int contents;
	vec3_t n;
	if (axis)
	{
		VectorSet(n, DotProduct(p, axis[0]), DotProduct(p, axis[1]), DotProduct(p, axis[2]));
		p = n;
	}
	contents = BIH_TestContents (mod->cnodes, p);
#ifdef TERRAIN
	if (mod->terrain)
		contents |= Heightmap_PointContents(mod, NULL, p);
#endif
	return contents;
}

static unsigned int BIH_NativeContents(struct model_s *mod, int hulloverride, const framestate_t *framestate, const vec3_t axis[3], const vec3_t p, const vec3_t mins, const vec3_t maxs)
{	//we don't support boxcontents... sorry.
	unsigned int contents;
	vec3_t n;
	if (axis)
	{
		VectorSet(n, DotProduct(p, axis[0]), DotProduct(p, axis[1]), DotProduct(p, axis[2]));
		p = n;
	}
	contents = BIH_TestContents (mod->cnodes, p);
#ifdef TERRAIN
	if (mod->terrain)
		contents |= Heightmap_PointContents(mod, NULL, p);
#endif
	return contents;
}









#if defined(BIH_USEBIH) || defined(BIH_USEBVH)
static int QDECL BIH_Sort_X (const void *va, const void *vb)
{
	const struct bihleaf_s *a = va, *b = vb;
	float am = a->maxs[0]+a->mins[0];
	float bm = b->maxs[0]+b->mins[0];
	if (am == bm)
		return 0;
	return am > bm;
}
static int QDECL BIH_Sort_Y (const void *va, const void *vb)
{
	const struct bihleaf_s *a = va, *b = vb;
	float am = a->maxs[1]+a->mins[1];
	float bm = b->maxs[1]+b->mins[1];
	if (am == bm)
		return 0;
	return am > bm;
}
static int QDECL BIH_Sort_Z (const void *va, const void *vb)
{
	const struct bihleaf_s *a = va, *b = vb;
	float am = a->maxs[2]+a->mins[2];
	float bm = b->maxs[2]+b->mins[2];
	if (am == bm)
		return 0;
	return am > bm;
}
#endif
static struct bihbox_s BIH_BuildNode (struct bihnode_s *node, struct bihnode_s **freenodes, struct bihleaf_s *leafs, size_t numleafs)
{
	struct bihbox_s bounds;
	if (numleafs == 1)	//the leaf just gives the brush pointer.
	{
		size_t i;
		VectorCopy(leafs[0].mins, bounds.min);
		VectorCopy(leafs[0].maxs, bounds.max);
		node->type = leafs[0].type;
		node->data = leafs[0].data;

		//expand by 1qu, to avoid precision issues.
		for (i = 0; i < 3; i++)
		{
			bounds.min[i] -= 1;
			bounds.max[i] += 1;
		}
	}
#ifdef BIH_USEBIH
	else if (numleafs >= 8)	//the leaf just gives the brush pointer.
	{
		size_t i, j;
		size_t numleft = numleafs / 2;	//this ends up splitting at the median point.
		size_t numright = numleafs - numleft;
		struct bihbox_s left, right;
		struct bihnode_s *cnodes;
		static int (QDECL *sorts[3]) (const void *va, const void *vb) = {BIH_Sort_X, BIH_Sort_Y, BIH_Sort_Z};
		VectorCopy(leafs[0].mins, bounds.min);
		VectorCopy(leafs[0].maxs, bounds.max);
		for (i = 1; i < numleafs; i++)
		{
			for(j = 0; j < 3; j++)
			{
				if (bounds.min[j] > leafs[i].mins[j])
					bounds.min[j] = leafs[i].mins[j];
				if (bounds.max[j] < leafs[i].maxs[j])
					bounds.max[j] = leafs[i].maxs[j];
			}
		}
#if 1
		{	//balanced by counts
			vec3_t mid;
			int onleft[3], onright[3], weight[3];
			VectorAvg(bounds.max, bounds.min, mid);
			VectorClear(onleft);
			VectorClear(onright);
			for (i = 0; i < numleafs; i++)
			{
				for (j = 0; j < 3; j++)
				{	//ignore leafs that split the node.
					if (leafs[i].maxs[j] < mid[j])
						onleft[j]++;
					if (mid[j] > leafs[i].mins[j])
						onright[j]++;
				}
			}
			for (j = 0; j < 3; j++)
				weight[j] = onleft[j]+onright[j] - abs(onleft[j]-onright[j]);
			//pick the most balanced.
			if (weight[0] > weight[1] && weight[0] > weight[2])
				node->type = BIH_X;
			else if (weight[1] > weight[2])
				node->type = BIH_Y;
			else
				node->type = BIH_Z;
		}
#else
		{	//balanced by volume
			vec3_t size;
			VectorSubtract(bounds.max, bounds.min, size);
			if (size[0] > size[1] && size[0] > size[2])
				node->type = BIH_X;
			else if (size[1] > size[2])
				node->type = BIH_Y;
			else
				node->type = BIH_Z;*/
		}
#endif
		qsort(leafs, numleafs, sizeof(*leafs), sorts[node->type-BIH_X]);

		cnodes = *freenodes;
		*freenodes += 2;
		node->bihnode.firstchild = cnodes - node;
		left = BIH_BuildNode (cnodes+0, freenodes, leafs, numleft);
		right = BIH_BuildNode (cnodes+1, freenodes, &leafs[numleft], numright);

		node->bihnode.cmin[0] = left.min[node->type-BIH_X];
		node->bihnode.cmax[0] = left.max[node->type-BIH_X];
		node->bihnode.cmin[1] = right.min[node->type-BIH_X];
		node->bihnode.cmax[1] = right.max[node->type-BIH_X];

		bounds = left;
		AddPointToBounds(right.min, bounds.min, bounds.max);
		AddPointToBounds(right.max, bounds.min, bounds.max);
	}
#endif
#ifdef BIH_USEBVH
	else if (numleafs >= 8)	//the leaf just gives the brush pointer.
	{
		size_t i, j;
		size_t numleft = numleafs / 2;	//this ends up splitting at the median point.
		size_t numright = numleafs - numleft;
		struct bihbox_s left, right;
		struct bihnode_s *cnodes;
		static int (QDECL *sorts[3]) (const void *va, const void *vb) = {CM_SortBIH_X, CM_SortBIH_Y, CM_SortBIH_Z};
		VectorCopy(leafs[0].mins, bounds.min);
		VectorCopy(leafs[0].maxs, bounds.max);
		for (i = 1; i < numleafs; i++)
		{
			for(j = 0; j < 3; j++)
			{
				if (bounds.min[j] > leafs[i].mins[j])
					bounds.min[j] = leafs[i].mins[j];
				if (bounds.max[j] < leafs[i].maxs[j])
					bounds.max[j] = leafs[i].maxs[j];
			}
		}
#if 1
		{	//balanced by counts
			vec3_t mid;
			int onleft[3], onright[3], weight[3];
			VectorAvg(bounds.max, bounds.min, mid);
			VectorClear(onleft);
			VectorClear(onright);
			for (i = 0; i < numleafs; i++)
			{
				for (j = 0; j < 3; j++)
				{	//ignore leafs that split the node.
					if (leafs[i].maxs[j] < mid[j])
						onleft[j]++;
					if (mid[j] > leafs[i].mins[j])
						onright[j]++;
				}
			}
			for (j = 0; j < 3; j++)
				weight[j] = onleft[j]+onright[j] - abs(onleft[j]-onright[j]);
			//pick the most balanced.
			if (weight[0] > weight[1] && weight[0] > weight[2])
				node->type = BVH_X;
			else if (weight[1] > weight[2])
				node->type = BVH_Y;
			else
				node->type = BVH_Z;
		}
#else
		{	//balanced by volume
			vec3_t size;
			VectorSubtract(bounds.max, bounds.min, size);
			if (size[0] > size[1] && size[0] > size[2])
				node->type = BVH_X;
			else if (size[1] > size[2])
				node->type = BVH_Y;
			else
				node->type = BVH_Z;*/
		}
#endif
		qsort(leafs, numleafs, sizeof(*leafs), sorts[node->type-BVH_X]);

		cnodes = *freenodes;
		*freenodes += 2;
		node->bvhnode.firstchild = cnodes - node;
		left = BIH_BuildNode (cnodes+0, freenodes, leafs, numleft);
		right = BIH_BuildNode (cnodes+1, freenodes, &leafs[numleft], numright);

		node->bvhnode.min[0] = min(left.min[0], right.min[0]);
		node->bvhnode.min[1] = min(left.min[1], right.min[1]);
		node->bvhnode.min[2] = min(left.min[2], right.min[2]);
		node->bvhnode.cmax = left.max[node->type-BVH_X];
		node->bvhnode.cmin = right.min[node->type-BVH_X];
		node->bvhnode.max[0] = max(left.max[0], right.max[0]);
		node->bvhnode.max[1] = max(left.max[1], right.max[1]);
		node->bvhnode.max[2] = max(left.max[2], right.max[2]);

		bounds = left;
		AddPointToBounds(right.min, bounds.min, bounds.max);
		AddPointToBounds(right.max, bounds.min, bounds.max);
	}
#endif
	else
	{
		struct bihnode_s *cnodes;
		struct bihbox_s cb;
		size_t i;
		node->type = BIH_GROUP;

		cnodes = *freenodes;
		*freenodes += numleafs;
		node->group.firstchild = cnodes - node;
		node->group.numchildren = numleafs;

		bounds = BIH_BuildNode(cnodes+0, freenodes, leafs+0, 1);
		for (i = 1; i < numleafs; i++)
		{
			cb = BIH_BuildNode(cnodes+i, freenodes, leafs+i, 1);
			AddPointToBounds(cb.min, bounds.min, bounds.max);
			AddPointToBounds(cb.max, bounds.min, bounds.max);
		}
	}
	return bounds;
}

void BIH_Build (model_t *mod, struct bihleaf_s *leafs, size_t numleafs)
{
	size_t numnodes;
	struct bihnode_s *nodes, *tmpnodes;

	if (!numleafs)
	{	//if we don't actually have anything solid, we still need SOMETHING so we don't crash.
		//we can just use an empty group node for that.
		nodes = ZG_Malloc(&mod->memgroup, sizeof(*nodes));
		nodes->type = BIH_GROUP;
		nodes->group.numchildren = 0;
	}
	else
	{
		numnodes = numleafs*2-1;
		nodes = ZG_Malloc(&mod->memgroup, sizeof(*nodes)*numnodes);

		tmpnodes = nodes+1;
		BIH_BuildNode(nodes, &tmpnodes, leafs, numleafs);
		if (tmpnodes > nodes+numnodes)
			Sys_Error("CM_BuildBIH: generated wrong number of nodes");
	}
	mod->cnodes = nodes;
	mod->funcs.NativeTrace			= BIH_Trace;
	mod->funcs.PointContents		= BIH_PointContents;
	mod->funcs.NativeContents		= BIH_NativeContents;
}
#ifdef SKELETALMODELS
void BIH_BuildAlias (model_t *mod, galiasinfo_t *meshes)
{
	size_t numleafs, i;
	struct bihleaf_s *leafs, *leaf;
	galiasinfo_t *submesh;

	numleafs = 0;
	for (submesh = meshes; submesh; submesh = submesh->nextsurf)
		numleafs+=submesh->numindexes/3;
	leaf = leafs = BZ_Malloc(sizeof(*leafs)*numleafs);
	for (submesh = meshes; submesh; submesh = submesh->nextsurf)
	{
		for (i = 0; i < submesh->numindexes; i+=3)
		{
			vec_t *v1,*v2,*v3;
			leaf->type = BIH_TRIANGLE;
			leaf->data.contents = submesh->contents;
			leaf->data.tri.indexes = submesh->ofs_indexes+i;
			leaf->data.tri.xyz = submesh->ofs_skel_xyz;

			v1 = leaf->data.tri.xyz[leaf->data.tri.indexes[0]];
			v2 = leaf->data.tri.xyz[leaf->data.tri.indexes[1]];
			v3 = leaf->data.tri.xyz[leaf->data.tri.indexes[2]];

			VectorCopy(v1, leaf->mins);
			VectorCopy(v1, leaf->maxs);
			AddPointToBounds(v2, leaf->mins, leaf->maxs);
			AddPointToBounds(v3, leaf->mins, leaf->maxs);
			leaf++;
		}
	}
	BIH_Build(mod, leafs, leaf-leafs);
}
#endif
