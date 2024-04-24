
#ifndef COLLISION_H
#define COLLISION_H

#include "matrixlib.h"
struct mempool_s;
struct model_s;

typedef union plane_s
{
	struct
	{
		vec3_t	normal;
		vec_t	dist;
	};
	vec4_t normal_and_dist;
}
plane_t;

struct texture_s;
typedef struct trace_s
{
	// if true, the entire trace was in solid (see hitsupercontentsmask)
	qbool allsolid;
	// if true, the initial point was in solid (see hitsupercontentsmask)
	qbool startsolid;
	// this is set to true in world.c if startsolid was set in a trace against world
	qbool worldstartsolid;
	// this is set to true in world.c if startsolid was set in a trace against a SOLID_BSP entity, in other words this is true if the entity is stuck in a door or wall, but not if stuck in another normal entity
	qbool bmodelstartsolid;
	// if true, the trace passed through empty somewhere
	// (set only by Q1BSP tracing)
	qbool inopen;
	// if true, the trace passed through water/slime/lava somewhere
	// (set only by Q1BSP tracing)
	qbool inwater;
	// fraction of the total distance that was traveled before impact
	// in case of impact this is actually nudged a bit off the surface
	// (1.0 = did not hit anything)
	double fraction;
	// final position of the trace (simply a point between start and end)
	double endpos[3];
	// surface normal at impact (not really correct for edge collisions)
	plane_t plane;
	// entity the surface is on
	// (not set by trace functions, only by physics)
	void *ent;
	// which SUPERCONTENTS bits to collide with, I.E. to consider solid
	// (this also affects startsolid/allsolid)
	int hitsupercontentsmask;
	// deliberately skip surfaces matching this mask (e.g. SUPERCONTENTS_SKY allows you to bypass sky surfaces in q1bsp/q2bsp which are SUPERCONTENTS_SKY | SUPERCONTENTS_SOLID)
	int skipsupercontentsmask;
	// deliberately skip surfaces matching this mask on materialflags (e.g. MATERIALFLAGMASK_TRANSLUCENT)
	int skipmaterialflagsmask;
	// the supercontents mask at the start point
	int startsupercontents;
	// the supercontents of the impacted surface
	int hitsupercontents;
	// the q3 surfaceflags of the impacted surface
	int hitq3surfaceflags;
	// the texture of the impacted surface
	const struct texture_s *hittexture;
	// initially false, set when the start leaf is found
	// (set only by Q1BSP tracing and entity box tracing)
	qbool startfound;
	// if startsolid, contains the minimum penetration depth found in the
	// trace, and the normal needed to push it out of that solid
	double startdepth;
	double startdepthnormal[3];
	const struct texture_s *starttexture;
}
trace_t;

void Collision_Init(void);
void Collision_ClipTrace_Box(trace_t *trace, const vec3_t cmins, const vec3_t cmaxs, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask, int boxsupercontents, int boxq3surfaceflags, const texture_t *boxtexture);
void Collision_ClipTrace_Point(trace_t *trace, const vec3_t cmins, const vec3_t cmaxs, const vec3_t start, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask, int boxsupercontents, int boxq3surfaceflags, const texture_t *boxtexture);

void Collision_Cache_Reset(qbool resetlimits);
void Collision_Cache_Init(struct mempool_s *mempool);
void Collision_Cache_NewFrame(void);

typedef struct colpointf_s
{
	vec3_t v;
}
colpointf_t;

typedef struct colplanef_s
{
	const struct texture_s *texture;
	int q3surfaceflags;
	union
	{
		struct
		{
			vec3_t normal;
			vec_t dist;
		};
		vec4_t normal_and_dist;
	};
}
colplanef_t;

typedef struct colbrushf_s
{
	// culling box
	vec3_t mins;
	vec3_t maxs;
	// used to avoid tracing against the same brush more than once per sweep
	int markframe;
	// the content flags of this brush
	int supercontents;
	// bounding planes (face planes) of this brush
	int numplanes;
	colplanef_t *planes;
	// edge directions (normals) of this brush
	int numedgedirs;
	colpointf_t *edgedirs;
	// points (corners) of this brush
	int numpoints;
	colpointf_t *points;
	// renderable triangles representing this brush, using the points
	int numtriangles;
	int *elements;
	// texture data for cases where an edgedir is used
	const struct texture_s *texture;
	int q3surfaceflags;
	// optimized collisions for common cases
	int isaabb; // indicates this is an axis aligned box
	int hasaabbplanes; // indicates this has precomputed planes for AABB collisions
}
colbrushf_t;

typedef struct colboxbrushf_s
{
	colpointf_t points[8];
	colpointf_t edgedirs[6];
	colplanef_t planes[6];
	colbrushf_t brush;
}
colboxbrushf_t;

void Collision_CalcPlanesForTriangleBrushFloat(colbrushf_t *brush);
colbrushf_t *Collision_NewBrushFromPlanes(struct mempool_s *mempool, int numoriginalplanes, const colplanef_t *originalplanes, int supercontents, int q3surfaceflags, const texture_t *texture, int hasaabbplanes);
void Collision_TraceBrushBrushFloat(trace_t *trace, const colbrushf_t *thisbrush_start, const colbrushf_t *thisbrush_end, const colbrushf_t *thatbrush_start, const colbrushf_t *thatbrush_end);
void Collision_TraceBrushTriangleMeshFloat(trace_t *trace, const colbrushf_t *thisbrush_start, const colbrushf_t *thisbrush_end, int numtriangles, const int *element3i, const float *vertex3f, int stride, float *bbox6f, int supercontents, int q3surfaceflags, const texture_t *texture, const vec3_t segmentmins, const vec3_t segmentmaxs);
void Collision_TraceLineBrushFloat(trace_t *trace, const vec3_t linestart, const vec3_t lineend, const colbrushf_t *thatbrush_start, const colbrushf_t *thatbrush_end);
void Collision_TraceLineTriangleMeshFloat(trace_t *trace, const vec3_t linestart, const vec3_t lineend, int numtriangles, const int *element3i, const float *vertex3f, int stride, float *bbox6f, int supercontents, int q3surfaceflags, const texture_t *texture, const vec3_t segmentmins, const vec3_t segmentmaxs);
void Collision_TracePointBrushFloat(trace_t *trace, const vec3_t point, const colbrushf_t *thatbrush);
qbool Collision_PointInsideBrushFloat(const vec3_t point, const colbrushf_t *brush);

void Collision_BrushForBox(colboxbrushf_t *boxbrush, const vec3_t mins, const vec3_t maxs, int supercontents, int q3surfaceflags, const texture_t *texture);

void Collision_BoundingBoxOfBrushTraceSegment(const colbrushf_t *start, const colbrushf_t *end, vec3_t mins, vec3_t maxs, float startfrac, float endfrac);

float Collision_ClipTrace_Line_Sphere(double *linestart, double *lineend, double *sphereorigin, double sphereradius, double *impactpoint, double *impactnormal);
void Collision_TraceLineTriangleFloat(trace_t *trace, const vec3_t linestart, const vec3_t lineend, const float *point0, const float *point1, const float *point2, int supercontents, int q3surfaceflags, const texture_t *texture);
void Collision_TraceBrushTriangleFloat(trace_t *trace, const colbrushf_t *thisbrush_start, const colbrushf_t *thisbrush_end, const float *v0, const float *v1, const float *v2, int supercontents, int q3surfaceflags, const texture_t *texture);

// traces a box move against a single entity
// mins and maxs are relative
//
// if the entire move stays in a single solid brush, trace.allsolid will be set
//
// if the starting point is in a solid, it will be allowed to move out to an
// open area, and trace.startsolid will be set
//
// type is one of the MOVE_ values such as MOVE_NOMONSTERS which skips box
// entities, only colliding with SOLID_BSP entities (doors, lifts)
//
// passedict is excluded from clipping checks
struct frameblend_s;
struct skeleton_s;
void Collision_ClipToGenericEntity(trace_t *trace, struct model_s *model, const struct frameblend_s *frameblend, const struct skeleton_s *skeleton, const vec3_t bodymins, const vec3_t bodymaxs, int bodysupercontents, matrix4x4_t *matrix, matrix4x4_t *inversematrix, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask, float extend);
void Collision_ClipLineToGenericEntity(trace_t *trace, struct model_s *model, const struct frameblend_s *frameblend, const struct skeleton_s *skeleton, const vec3_t bodymins, const vec3_t bodymaxs, int bodysupercontents, matrix4x4_t *matrix, matrix4x4_t *inversematrix, const vec3_t start, const vec3_t end, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask, float extend, qbool hitsurfaces);
void Collision_ClipPointToGenericEntity(trace_t *trace, struct model_s *model, const struct frameblend_s *frameblend, const struct skeleton_s *skeleton, const vec3_t bodymins, const vec3_t bodymaxs, int bodysupercontents, matrix4x4_t *matrix, matrix4x4_t *inversematrix, const vec3_t start, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask);
// like above but does not do a transform and does nothing if model is NULL
void Collision_ClipToWorld(trace_t *trace, struct model_s *model, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask, float extend);
void Collision_ClipLineToWorld(trace_t *trace, struct model_s *model, const vec3_t start, const vec3_t end, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask, float extend, qbool hitsurfaces);
void Collision_ClipPointToWorld(trace_t *trace, struct model_s *model, const vec3_t start, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask);
// caching surface trace for renderer (NOT THREAD SAFE)
void Collision_Cache_ClipLineToGenericEntitySurfaces(trace_t *trace, struct model_s *model, matrix4x4_t *matrix, matrix4x4_t *inversematrix, const vec3_t start, const vec3_t end, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask);
void Collision_Cache_ClipLineToWorldSurfaces(trace_t *trace, struct model_s *model, const vec3_t start, const vec3_t end, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask);
// combines data from two traces:
// merges contents flags, startsolid, allsolid, inwater
// updates fraction, endpos, plane and surface info if new fraction is shorter
void Collision_CombineTraces(trace_t *cliptrace, const trace_t *trace, void *touch, qbool isbmodel);

// this enables rather large debugging spew!
// settings:
// 0 = no spew
// 1 = spew trace calls if something odd is happening
// 2 = spew trace calls always
// 3 = spew detailed trace flow (bsp tree recursion info)
#define COLLISIONPARANOID 0

extern struct cvar_s collision_impactnudge;
extern struct cvar_s collision_extendtracelinelength;
extern struct cvar_s collision_extendtraceboxlength;
extern struct cvar_s collision_extendmovelength;
extern struct cvar_s collision_bih_fullrecursion;

#endif
