
// Shadow Volume BSP code written by Ashley Rose Hale (LadyHavoc) on 2003-11-06 and placed into public domain.
// Modified by LadyHavoc (to make it work and other nice things like that) on 2007-01-24 and 2007-01-25

#ifndef SVBSP_H
#define SVBSP_H

typedef struct svbsp_node_s
{
	// notes:
	// leaf nodes are not stored, these are always structural nodes
	// (they always have a plane and two children)
	// children[] can be -1 for empty leaf or -2 for shadow leaf, >= 0 is node
	// parent can be -1 if this is the root node, >= 0 is a node
	int parent, children[2], padding;
	// node plane, splits space
	float plane[4];
}
svbsp_node_t;

typedef struct svbsp_s
{
	// lightsource or view origin
	float origin[3];
	// current number of nodes in the svbsp
	int numnodes;
	// how big the nodes array is
	int maxnodes;
	// first node is the root node
	svbsp_node_t *nodes;
	// non-zero indicates that an insertion failed because of lack of nodes
	int ranoutofnodes;
	// tree statistics
	// note: do not use multithreading when gathering statistics!
	// (the code updating these counters is not thread-safe, increments may
	//  sometimes fail when multithreaded)
	int stat_occluders_rejected;
	int stat_occluders_accepted;
	int stat_occluders_fragments_rejected;
	int stat_occluders_fragments_accepted;
	int stat_queries_rejected;
	int stat_queries_accepted;
	int stat_queries_fragments_rejected;
	int stat_queries_fragments_accepted;
}
svbsp_t;

// this function initializes a tree to prepare for polygon insertions
//
// the maxnodes needed for a given polygon set can vary wildly, if there are
// not enough maxnodes then later polygons will not be inserted and the field
// svbsp_t->ranoutofnodes will be non-zero
//
// as a rule of thumb the minimum nodes needed for a polygon set is
// numpolygons * (averagepolygonvertices + 1)
void SVBSP_Init(svbsp_t *b, const float *origin, int maxnodes, svbsp_node_t *nodes);

// this function tests if any part of a polygon is not in shadow, and returns
// non-zero if the polygon is not completely shadowed
//
// returns 0 if the polygon was rejected (not facing origin or no points)
// returns 1 if all of the polygon is in shadow
// returns 2 if all of the polygon is unshadowed
// returns 3 if some of the polygon is shadowed and some unshadowed
//
// it also can add a new shadow volume (insertoccluder parameter)
//
// additionally it calls your fragmentcallback on each unshadowed clipped
// part of the polygon
// (beware that polygons often get split heavily, even if entirely unshadowed)
//
// thread-safety notes: do not multi-thread insertions!
int SVBSP_AddPolygon(svbsp_t *b, int numpoints, const float *points, int insertoccluder, void (*fragmentcallback)(void *fragmentcallback_pointer1, int fragmentcallback_number1, svbsp_t *b, int numpoints, const float *points), void *fragmentcallback_pointer1, int fragmentcallback_number1);

#endif
