/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#include "dmap.h"


interAreaPortal_t interAreaPortals[MAX_INTER_AREA_PORTALS];
int					numInterAreaPortals;
// stgatilov #5129: the arrays below are stored only to avoid duplicate reports
static interAreaPortal_t droppedAreaPortals[MAX_INTER_AREA_PORTALS];
static int					numDroppedAreaPortals;
static interAreaPortal_t overlappingAreaPortals[MAX_INTER_AREA_PORTALS];
static int					numOverlappingAreaPortals;


int		c_active_portals;
int		c_peak_portals;

/*
===========
AllocPortal
===========
*/
uPortal_t	*AllocPortal (void)
{
	uPortal_t	*p;
	
	c_active_portals++;
	if (c_active_portals > c_peak_portals)
		c_peak_portals = c_active_portals;
	
	p = (uPortal_t *)Mem_Alloc (sizeof(uPortal_t ));
	memset (p, 0, sizeof(uPortal_t ));
	
	return p;
}


void FreePortal (uPortal_t  *p)
{
	if (p->winding)
		delete p->winding;
	c_active_portals--;
	Mem_Free (p);
}

//==============================================================

/*
=============
Portal_Passable

Returns true if the portal has non-opaque leafs on both sides
=============
*/
static bool Portal_Passable( uPortal_t  *p ) {
	if (!p->onnode) {
		return false;	// to global outsideleaf
	}

	if (p->nodes[0]->planenum != PLANENUM_LEAF
		|| p->nodes[1]->planenum != PLANENUM_LEAF) {
		common->Error( "Portal_EntityFlood: not a leaf");
	}

	if ( !p->nodes[0]->opaque && !p->nodes[1]->opaque ) {
		return true;
	}

	return false;
}


//=============================================================================

int		c_tinyportals;

/*
=============
AddPortalToNodes
=============
*/
void AddPortalToNodes (uPortal_t  *p, node_t *front, node_t *back) {
	if (p->nodes[0] || p->nodes[1]) {
		common->Error( "AddPortalToNode: allready included");
	}

	p->nodes[0] = front;
	p->next[0] = front->portals;
	front->portals = p;
	
	p->nodes[1] = back;
	p->next[1] = back->portals;
	back->portals = p;
}


/*
=============
RemovePortalFromNode
=============
*/
void RemovePortalFromNode (uPortal_t  *portal, node_t *l)
{
	uPortal_t	**pp, *t;
	
// remove reference to the current portal
	pp = &l->portals;
	while (1)
	{
		t = *pp;
		if (!t)
			common->Error( "RemovePortalFromNode: portal not in leaf");	

		if ( t == portal )
			break;

		if (t->nodes[0] == l)
			pp = &t->next[0];
		else if (t->nodes[1] == l)
			pp = &t->next[1];
		else
			common->Error( "RemovePortalFromNode: portal not bounding leaf");
	}
	
	if ( portal->nodes[0] == l ) {
		*pp = portal->next[0];
		portal->nodes[0] = NULL;
	} else if ( portal->nodes[1] == l ) {
		*pp = portal->next[1];	
		portal->nodes[1] = NULL;
	} else {
		common->Error( "RemovePortalFromNode: mislinked" ); 
	}
}

//============================================================================

void PrintPortal (uPortal_t *p)
{
	int			i;
	idWinding	*w;
	
	w = p->winding;
	for ( i = 0; i < w->GetNumPoints(); i++ )
		common->Printf("(%5.0f,%5.0f,%5.0f)\n",(*w)[i][0], (*w)[i][1], (*w)[i][2]);
}

/*
================
MakeHeadnodePortals

The created portals will face the global outside_node
================
*/
#define	SIDESPACE	8
static void MakeHeadnodePortals( tree_t *tree ) {
	idBounds	bounds;
	int			i, j, n;
	uPortal_t	*p, *portals[6];
	idPlane		bplanes[6], *pl;
	node_t *node;

	node = tree->headnode;

	tree->outside_node.planenum = PLANENUM_LEAF;
	tree->outside_node.brushlist = NULL;
	tree->outside_node.portals = NULL;
	tree->outside_node.opaque = false;

	// if no nodes, don't go any farther
	if ( node->planenum == PLANENUM_LEAF ) {
		return;
	}

	// pad with some space so there will never be null volume leafs
	for (i=0 ; i<3 ; i++) {
		bounds[0][i] = tree->bounds[0][i] - SIDESPACE;
		bounds[1][i] = tree->bounds[1][i] + SIDESPACE;
		if ( bounds[0][i] >= bounds[1][i] ) {
			common->Error( "Backwards tree volume" );
		}
	}
	
	for (i=0 ; i<3 ; i++) {
		for (j=0 ; j<2 ; j++) {
			n = j*3 + i;

			p = AllocPortal ();
			portals[n] = p;
			
			pl = &bplanes[n];
			memset (pl, 0, sizeof(*pl));
			if (j) {
				(*pl)[i] = -1;
				(*pl)[3] = bounds[j][i];
			} else {
				(*pl)[i] = 1;
				(*pl)[3] = -bounds[j][i];
			}
			p->plane = *pl;
			p->winding = new idWinding( *pl );
			AddPortalToNodes (p, node, &tree->outside_node);
		}
	}

	// clip the basewindings by all the other planes
	for (i=0 ; i<6 ; i++) {
		for (j=0 ; j<6 ; j++) {
			if (j == i) {
				continue;
			}
			portals[i]->winding = portals[i]->winding->Clip( bplanes[j], ON_EPSILON );
		}
	}
}

//===================================================

#define	BASE_WINDING_EPSILON	0.001f
#define	SPLIT_WINDING_EPSILON	0.001f

/*
==================
MakeNodePortal

create the new portal by taking the full plane winding for the cutting plane
and clipping it by all of parents of this node
==================
*/
static void MakeNodePortal( node_t *node ) {

	idPlane plane = dmapGlobals.mapPlanes[node->planenum];
	idList<idPlane> cuttingPlanes;

	// clip by all the parents
	for ( node_t *curr = node, *n = node->parent ; n ; ) {
		idPlane &plane = dmapGlobals.mapPlanes[n->planenum];

		if ( n->children[0] == curr ) {
			// take front
			cuttingPlanes.AddGrow(plane);
		} else {
			// take back
			cuttingPlanes.AddGrow(-plane);
		}
		curr = n;
		n = n->parent;
	}

	int side;
	// clip the portal by all the other portals in the node
	for (uPortal_t *p = node->portals ; p; p = p->next[side])	
	{
		if (p->nodes[0] == node)
		{
			side = 0;
			cuttingPlanes.AddGrow(p->plane);
		}
		else if (p->nodes[1] == node)
		{
			side = 1;
			cuttingPlanes.AddGrow(-p->plane);
		}
		else {
			common->Error( "CutNodePortals_r: mislinked portal");
			side = 0;	// quiet a compiler warning
		}
	}

	idWinding *w = idWinding::CreateTrimmedPlane(plane, cuttingPlanes.Num(), cuttingPlanes.Ptr(), BASE_WINDING_EPSILON);
	if (!w)
	{
		return;
	}

	if ( w->IsTiny() )
	{
		c_tinyportals++;
		delete w;
		return;
	}

	uPortal_t *new_portal = AllocPortal ();
	new_portal->plane = dmapGlobals.mapPlanes[node->planenum];
	new_portal->onnode = node;
	new_portal->winding = w;	
	AddPortalToNodes (new_portal, node->children[0], node->children[1]);
}


/*
==============
SplitNodePortals

Move or split the portals that bound node so that the node's
children have portals instead of node.
==============
*/
static void SplitNodePortals( node_t *node ) {
	uPortal_t	*p, *next_portal, *new_portal;
	node_t		*f, *b, *other_node;
	int			side;
	idPlane		*plane;
	idWinding	*frontwinding, *backwinding;

	plane = &dmapGlobals.mapPlanes[node->planenum];
	f = node->children[0];
	b = node->children[1];

	for ( p = node->portals ; p ; p = next_portal ) {
		if (p->nodes[0] == node ) {
			side = 0;
		} else if ( p->nodes[1] == node ) {
			side = 1;
		} else {
			common->Error( "SplitNodePortals: mislinked portal" );
			side = 0;	// quiet a compiler warning
		}
		next_portal = p->next[side];

		other_node = p->nodes[!side];
		RemovePortalFromNode (p, p->nodes[0]);
		RemovePortalFromNode (p, p->nodes[1]);

	//
	// cut the portal into two portals, one on each side of the cut plane
	//
		p->winding->Split( *plane, SPLIT_WINDING_EPSILON, &frontwinding, &backwinding);

		if ( frontwinding && frontwinding->IsTiny() )
		{
			delete frontwinding;
			frontwinding = NULL;
			c_tinyportals++;
		}

		if ( backwinding && backwinding->IsTiny() )
		{
			delete backwinding;
			backwinding = NULL;
			c_tinyportals++;
		}

		if ( !frontwinding && !backwinding )
		{	// tiny windings on both sides
			continue;
		}

		if (!frontwinding)
		{
			delete backwinding;
			if (side == 0)
				AddPortalToNodes (p, b, other_node);
			else
				AddPortalToNodes (p, other_node, b);
			continue;
		}
		if (!backwinding)
		{
			delete frontwinding;
			if (side == 0)
				AddPortalToNodes (p, f, other_node);
			else
				AddPortalToNodes (p, other_node, f);
			continue;
		}
		
	// the winding is split
		new_portal = AllocPortal ();
		*new_portal = *p;
		new_portal->winding = backwinding;
		delete p->winding;
		p->winding = frontwinding;

		if (side == 0)
		{
			AddPortalToNodes (p, f, other_node);
			AddPortalToNodes (new_portal, b, other_node);
		}
		else
		{
			AddPortalToNodes (p, other_node, f);
			AddPortalToNodes (new_portal, other_node, b);
		}
	}

	node->portals = NULL;
}


/*
================
CalcNodeBounds
================
*/
void CalcNodeBounds (node_t *node)
{
	uPortal_t	*p;
	int			s;
	int			i;

	// calc mins/maxs for both leafs and nodes
	node->bounds.Clear();
	for (p = node->portals ; p ; p = p->next[s]) {
		s = (p->nodes[1] == node);
		for ( i = 0; i < p->winding->GetNumPoints(); i++ ) {
			node->bounds.AddPoint( (*p->winding)[i].ToVec3() );
		}
	}
}


/*
==================
MakeTreePortals_r
==================
*/
void MakeTreePortals_r (node_t *node)
{
	int		i;

	CalcNodeBounds( node );

	if ( node->bounds[0][0] >= node->bounds[1][0]) {
		PrintIfVerbosityAtLeast( VL_ORIGDEFAULT, "node without a volume" ); // #4123 downgrade from a warning. This is normal operation.
	}

	for ( i = 0; i < 3; i++ ) {
		if ( node->bounds[0][i] < MIN_WORLD_COORD || node->bounds[1][i] > MAX_WORLD_COORD ) {
			common->Warning( "node with unbounded volume");
			break;
		}
	}
	if ( node->planenum == PLANENUM_LEAF ) {
		return;
	}

	MakeNodePortal (node);
	SplitNodePortals (node);

	MakeTreePortals_r (node->children[0]);
	MakeTreePortals_r (node->children[1]);
}

/*
==================
MakeTreePortals
==================
*/
void MakeTreePortals (tree_t *tree)
{
	TRACE_CPU_SCOPE("MakeTreePortals")
	PrintIfVerbosityAtLeast( VL_ORIGDEFAULT, "----- MakeTreePortals -----\n");
	MakeHeadnodePortals (tree);
	MakeTreePortals_r (tree->headnode);
}

/*
=============
FindLeafNodeAtPoint
=============
*/
node_t *FindLeafNodeAtPoint( node_t *headnode, idVec3 origin ) {
	// find the leaf to start in
	node_t *node = headnode;
	while ( node->planenum != PLANENUM_LEAF ) {
		const idPlane &plane = dmapGlobals.mapPlanes[node->planenum];
		float d = plane.Distance( origin );
		if ( d >= 0.0f ) {
			node = node->children[0];
		} else {
			node = node->children[1];
		}
	}
	return node;
}

/*
=========================================================

FLOOD AREAS

=========================================================
*/

static	int		c_areas;
static	int		c_areaFloods;

idCVar dmap_fixVisportalOutOfBoundaryEffects(
	"dmap_fixVisportalOutOfBoundaryEffects", "1", CVAR_BOOL | CVAR_SYSTEM,
	"If set to 0, then visportal sometimes blocks areas on its plane outside of its boundary polygon. "
	"This is a bug fixed in TDM 2.08. "
);

// complete information about one visportal found for a BSP portal
// see also FindVisportalsAtPortal
typedef struct visportalInfo_s {
	side_t *side;			// brush side being visportal (original brush)
	uBrush_t *brush;		// brush with this side (original brush)
	node_t *node;			// incident BSP node where the brush was found
	uBrush_t *brushPiece;	// piece of the brush inside BSP node
} visportalInfo_t;

/*
=================
FindVisportalsAtPortal

This function is used to check if BSP portal is covered by visportal(s).
It also returns a bunch of useful information about detected visportals.
=================
*/
static int FindVisportalsAtPortal( uPortal_t *p, visportalInfo_t *arrFound, int arrCapacity ) {
	int numFound = 0;
	idVec3 pctr = p->winding->GetCenter();
	// scan both bordering nodes brush lists for a portal brush
	// that shares the plane
	for ( int i = 0 ; i < 2 ; i++ ) {
		node_t *node = p->nodes[i];
		for ( uBrush_t *b = node->brushlist ; b ; b = b->next ) {
			if ( !( b->contents & CONTENTS_AREAPORTAL ) ) {
				continue;
			}
			uBrush_t *orig = b->original;
			for ( int j = 0 ; j < orig->numsides ; j++ ) {
				side_t *s = orig->sides + j;
				if ( !s->visibleHull ) {
					continue;
				}
				if ( !( s->material->GetContentFlags() & CONTENTS_AREAPORTAL ) ) {
					continue;
				}
				if ( ( s->planenum & ~1 ) != ( p->onnode->planenum & ~1 ) ) {
					continue;
				}
				if ( dmap_fixVisportalOutOfBoundaryEffects.GetBool() ) {
					// stgatilov #5129: visportal side boundary must contain BSP portal to have effect
					idPlane sidePlane;
					s->winding->GetPlane( sidePlane );
					if ( !s->winding->PointInsideDst( sidePlane.Normal(), pctr, CLIP_EPSILON ) ) {
						continue;
					}
				}
				// remove the visible hull from any other portal sides of this portal brush
				for ( int k = 0; k < orig->numsides; k++ ) {
					if ( k == j ) {
						continue;
					}
					side_t *s2 = orig->sides + k;
					if ( s2->visibleHull == NULL ) {
						continue;
					}
					if ( !( s2->material->GetContentFlags() & CONTENTS_AREAPORTAL ) ) {
						continue;
					}
					common->Warning( "brush %d has multiple area portal sides at %s", b->brushnum, s2->visibleHull->GetCenter().ToString() );
					delete s2->visibleHull;
					s2->visibleHull = NULL;
				}
				visportalInfo_t info;
				info.side = s;
				info.brush = orig;
				info.brushPiece = b;
				info.node = node;
				if (numFound < arrCapacity) {
					arrFound[numFound++] = info;
				}
			}
		}
	}
	return numFound;
}

/*
=================
FindSideForPortal
=================
*/
static side_t	*FindSideForPortal( uPortal_t *p ) {
	visportalInfo_s info;
	if ( FindVisportalsAtPortal(p, &info, 1) )
		return info.side;
	else
		return NULL;
}

/*
=============
FloodAreas_r
=============
*/
void FloodAreas_r (node_t *node)
{
	uPortal_t	*p;
	int			s;

	if ( node->area != -1 ) {
		return;		// allready got it
	}
	if ( node->opaque ) {
		return;
	}

	c_areaFloods++;
	node->area = c_areas;

	for ( p=node->portals ; p ; p = p->next[s] ) {
		node_t	*other;

		s = (p->nodes[1] == node);
		other = p->nodes[!s];

		if ( !Portal_Passable(p) ) {
			continue;
		}

		// can't flood through an area portal
		if ( FindSideForPortal( p ) ) {
			continue;
		}

		FloodAreas_r( other );
	}
}

/*
=============
FindAreas_r

Just decend the tree, and for each node that hasn't had an
area set, flood fill out from there
=============
*/
void FindAreas_r( node_t *node ) {
	if ( node->planenum != PLANENUM_LEAF ) {
		FindAreas_r (node->children[0]);
		FindAreas_r (node->children[1]);
		return;
	}

	if ( node->opaque ) {
		return;
	}

	if ( node->area != -1 ) {
		return;		// allready got it
	}

	c_areaFloods = 0;
	FloodAreas_r (node);
	PrintIfVerbosityAtLeast( VL_ORIGDEFAULT, "area %i has %i leafs\n", c_areas, c_areaFloods );
	c_areas++;
}

/*
============
CheckAreas_r
============
*/
void CheckAreas_r( node_t *node ) {
	if ( node->planenum != PLANENUM_LEAF ) {
		CheckAreas_r (node->children[0]);
		CheckAreas_r (node->children[1]);
		return;
	}
	if ( !node->opaque && node->area < 0 ) {
		common->Error( "CheckAreas_r: area = %i", node->area );
	}
}

/*
============
ClearAreas_r

Set all the areas to -1 before filling
============
*/
void ClearAreas_r( node_t *node ) {
	if ( node->planenum != PLANENUM_LEAF ) {
		ClearAreas_r (node->children[0]);
		ClearAreas_r (node->children[1]);
		return;
	}
	node->area = -1;
}

void ClearOccupied_r( node_t *node ) {
	if ( node->planenum != PLANENUM_LEAF ) {
		ClearOccupied_r (node->children[0]);
		ClearOccupied_r (node->children[1]);
		return;
	}
	node->occupied = 0;
}

//=============================================================

/*
=================
IsPortalSame
=================
*/
bool IsPortalSame( interAreaPortal_s *a, interAreaPortal_s *b ) {
	return a->side == b->side && (
		a->area0 == b->area0 && a->area1 == b->area1 ||
		a->area1 == b->area0 && a->area0 == b->area1
	);
}

/*
=================
ReportOverlappingPortals

Given an array of visportals which all cover given BSP portal,
reports this problem to user (with warning and pointfile).
=================
*/
static void ReportOverlappingPortals( uPortal_t *portal, visportalInfo_t *visportals, int multiplicity ) {
	idStr brushlist;
	for ( int i = 0; i < multiplicity; i++ ) {
		if (i) brushlist += ',';
		brushlist += idStr(visportals[i].brush->brushnum);
	}
	idStr pos = portal->winding->GetCenter().ToString();
	common->Warning ( "Portals [%s] at (%s) overlap", brushlist.c_str(), pos.c_str() );
	pos.Replace('.', 'd');
	pos.Replace('-', 'm');
	pos.Replace(' ', '_');
	idStr filename;
	sprintf( filename, "%s_portalO_%s.lin", dmapGlobals.mapFileBase, pos.c_str() );

	idStr ospath = fileSystem->RelativePathToOSPath( filename, "fs_devpath", "" );
	FILE *linefile = fopen( ospath, "w" );
	if ( !linefile )
		common->Error( "Couldn't open %s\n", ospath.c_str() );
	int pn = portal->winding->GetNumPoints();
	for ( int i = 0; i <= pn; i++ ) {
		idVec3 p = (*portal->winding)[i % pn].ToVec3();
		fprintf( linefile, "%f %f %f\n", p.x, p.y, p.z );
	}
	fclose( linefile );

	common->Printf( "saved %s (%i points)\n", filename.c_str(), pn + 1 );
}

/*
=================
FindInterAreaPortals_r

=================
*/
static void FindInterAreaPortals_r( node_t *node ) {
	uPortal_t	*p;
	int			s;
	int			i;
	idWinding	*w;
	side_t		*side;

	if ( node->planenum != PLANENUM_LEAF ) {
		FindInterAreaPortals_r( node->children[0] );
		FindInterAreaPortals_r( node->children[1] );
		return;
	}

	if ( node->opaque ) {
		return;
	}

	for ( p=node->portals ; p ; p = p->next[s] ) {
		node_t	*other;

		s = (p->nodes[1] == node);
		other = p->nodes[!s];

		if ( other->opaque ) {
			continue;
		}

		// only report areas going from lower number to higher number
		// so we don't report the portal twice
		if ( other->area <= node->area ) {
			continue;
		}

		visportalInfo_t info[32];
		int num = FindVisportalsAtPortal( p, info, 32 );
		if ( num == 0 ) {
			common->Warning( "FindVisportalsAtPortal failed at %s", p->winding->GetCenter().ToString() );
			continue;
		}
		side = info[0].side;
		w = side->visibleHull;
		if ( !w ) {
			continue;
		}

		interAreaPortal_t iap;
		if ( side->planenum == p->onnode->planenum ) {
			iap.area0 = p->nodes[0]->area;
			iap.area1 = p->nodes[1]->area;
		} else {
			iap.area0 = p->nodes[1]->area;
			iap.area1 = p->nodes[0]->area;
		}
		iap.side = side;
		iap.brush = info[0].brush;

		if ( dmap_bspAllSidesOfVisportal.GetBool() && num > 1 ) {
			// stgatilov #5129: given that all sides were inserted
			// every found visportal side must cover this entire BSP portal
			for ( i = 0 ; i < numOverlappingAreaPortals ; i++ ) {
				if ( IsPortalSame( &iap, &overlappingAreaPortals[i] ) )
					break;
			}
			if ( i == numOverlappingAreaPortals ) {
				ReportOverlappingPortals( p, info, num );
				// avoid reporting ALL overlapping sides between these areas in future
				for ( i = 0; i < num; i++ ) {
					interAreaPortal_t temp = iap;
					temp.side = info[i].side;
					temp.brush = info[i].brush;
					overlappingAreaPortals[numOverlappingAreaPortals++] = temp;
				}
			}
		}

		// see if we have created this portal before
		for ( i = 0 ; i < numInterAreaPortals ; i++ ) {
			if ( IsPortalSame( &iap, &interAreaPortals[i] ) )
				break;
		}
		if ( i != numInterAreaPortals ) {
			continue;	// already emited
		}

		if (numInterAreaPortals == MAX_INTER_AREA_PORTALS)
			common->Error("Exceeded limit on number of visportals (%d)", numInterAreaPortals);
		interAreaPortals[numInterAreaPortals++] = iap;
	}
}

/*
=============
FindShortestPathThroughBspNodes

Runs Breadth-First Search over entity's BSP tree: leaf nodes = vertices, portals = edges.
Tries to find shortest path from start to end node.
=============
*/
static bool FindShortestPathThroughBspNodes(
	uEntity_t *entity, const idList<node_t*> &startNodes, node_t *endNode,
	bool (*canPass)(void *ctx, node_t *from, uPortal_t *through, node_t *to), void *ctx,
	idList<node_t*> *resNodes, idList<uPortal_t*> *resPortals, idList<node_t*> *resAllVisited
) {
	if (resNodes)
		resNodes->Clear();
	if (resPortals)
		resPortals->Clear();

	idList<node_t*> nodesQueue;
	idList<int> prevIdx;
	idList<uPortal_t*> byPortal;

	if ( entity ) {
		// occupied is used as visited mark and (1 + shortest distance) at once
		ClearOccupied_r( entity->tree->headnode );
	}

	bool found = false;
	for ( int i = 0; i < startNodes.Num(); i++ ) {
		node_t *node = startNodes[i];
		if ( node->occupied )
			continue;
		nodesQueue.Append( node );
		node->occupied = 1;
		prevIdx.Append( -1 );
		byPortal.Append( NULL );
		if ( node == endNode )
			found = true;
	}

	// pretty standard Breadth-First-Search over leaf-nodes follows:
	for ( int done = 0; done < nodesQueue.Num() && !found; done++ ) {
		node_t *node = nodesQueue[done];

		for ( uPortal_t *p = node->portals, *np; p; p = np ) {
			int s = (p->nodes[1] == node);
			np = p->next[s];
			node_t *otherNode = p->nodes[!s];

			if ( otherNode->occupied > 0 )
				continue;					// already visited that node
			if ( !canPass( ctx, node, p, otherNode ) )
				continue;					// forbidden to go through this portal

			otherNode->occupied = node->occupied + 1;
			nodesQueue.Append(otherNode);
			prevIdx.Append(done);
			byPortal.Append(p);

			if ( otherNode == endNode ) {
				// terminate as soon as path found: hope to avoid visiting the whole map
				found = true;
				break;
			}
		}
	}

	if ( resAllVisited )
		*resAllVisited = nodesQueue;

	if ( !found )
		return false;

	// backtrace shortest path found by BFS
	for ( int idx = nodesQueue.Num() - 1; idx >= 0; idx = prevIdx[idx] ) {
		node_t *node = nodesQueue[idx];
		uPortal_t *p = byPortal[idx];
		if (resNodes)
			resNodes->AddGrow(node);
		if (resPortals && p)
			resPortals->AddGrow(p);
	}
	if (resNodes)
		resNodes->Reverse();
	if (resPortals)
		resPortals->Reverse();

	if (resNodes && resPortals)
		assert( resNodes->Num() == resPortals->Num() + 1 );
	return true;
}

/*
=============
ReportDroppedPortal

given a BSP portal having same area on both of its sides with visportal on it,
finds and reports a path from one side to the other one
=============
*/
static bool ReportDroppedPortal( uEntity_t *entity, uPortal_t *startPortal ) {
	int brushnum = -1;
	visportalInfo_t info;
	if ( FindVisportalsAtPortal( startPortal, &info, 1 ) )
		brushnum = info.brush->brushnum;

	auto CanPass_Generic = [](node_t *from, uPortal_t *through, node_t *to ) -> bool {
		if ( !Portal_Passable( through ) )
			return false;					// going into solid
		if ( FindSideForPortal( through ) )
			return false;					// going through visportal
		return true;
	};

	auto CanPass_BlockSidesOfVisportalBrush = [info]( node_t *from, uPortal_t *through, node_t *to ) -> bool {
		if ( !Portal_Passable( through ) )
			return false;					// going into solid
		if ( FindSideForPortal( through ) )
			return false;					// going through visportal

		uBrush_t *visportalBrush = info.brush;
		const idPlane &visportalPlane = dmapGlobals.mapPlanes[info.side->planenum];

		// check all sides of the visportal brush
		for ( int u = 0; u < visportalBrush->numsides; u++ ) {
			side_t *uside = &visportalBrush->sides[u];
			const idPlane &usidePlane = dmapGlobals.mapPlanes[uside->planenum];

			if ( (through->onnode->planenum & ~1) != (uside->planenum & ~1) )
				continue;	// not on side (wrong plane)

			idPlane windingPlane;
			uside->winding->GetPlane( windingPlane );
			if ( !uside->winding->PointInsideDst( windingPlane.Normal(), through->winding->GetCenter(), CLIP_EPSILON ) )
				continue;	// not on side (out of polygon)

			if ( usidePlane.Normal().Cross( visportalPlane.Normal() ).LengthSqr() <= VECTOR_EPSILON * VECTOR_EPSILON )
				continue;	// side parallel to visportalled one: can pass

			// cannot pass through nonparallel side of visportal brush
			return false;
		}

		return true;
	};

	auto CanPass_GoAlongVisportal = [info]( node_t *from, uPortal_t *through, node_t *to ) {
		if ( !Portal_Passable( through ) )
			return false;					// going into solid
		if ( FindSideForPortal( through ) )
			return false;					// going through visportal

		const idWinding &portalWinding = *info.side->winding;

		for ( uPortal_t *p = to->portals, *np; p; p = np) {
			int s = (p->nodes[1] == to);
			np = p->next[s];

			const idWinding &w = *p->winding;
			if (portalWinding.PointLiesOn(w.GetCenter(), CLIP_EPSILON))
				return true;		// dest node touches visportal by face

			int cnt = w.GetNumPoints();
			for (int i = 0; i < cnt; i++) {
				idVec3 beg = w[i].ToVec3();
				idVec3 end = w[(i+1) % cnt].ToVec3();
				if (portalWinding.PointLiesOn((beg + end) * 0.5, CLIP_EPSILON))
					return true;	// dest node touches visportal by edge
			}
		}

		return false;
	};

	idList<node_t*> pathNodes;
	idList<uPortal_t*> pathPortals;

	// check if we can find cycle over BSP nodes which touch visportal
	// if yes, then some part of visportal boundary does not contact opaque geometry (or other visportal)
	bool leakyBoundary = FindShortestPathThroughBspNodes(
		entity, {startPortal->nodes[0]}, startPortal->nodes[1],
		LambdaToFuncPtr(CanPass_GoAlongVisportal), &CanPass_GoAlongVisportal,
		NULL, NULL, NULL
	);

	// try to find nice-looking cycle with additional constraint:
	// it does NOT go through side faces of visportal brush
	bool foundNicePath = FindShortestPathThroughBspNodes(
		entity, {startPortal->nodes[0]}, startPortal->nodes[1],
		LambdaToFuncPtr(CanPass_BlockSidesOfVisportalBrush), &CanPass_BlockSidesOfVisportalBrush,
		&pathNodes, &pathPortals, NULL
	);

	bool found = foundNicePath;
	if (!found) {
		// try to find find arbitrary cycle
		found = FindShortestPathThroughBspNodes(
			entity, {startPortal->nodes[0]}, startPortal->nodes[1],
			LambdaToFuncPtr(CanPass_Generic), &CanPass_Generic,
			&pathNodes, &pathPortals, NULL
		);
	}

	if (!found)
		return false;	//still not found? should not happen
	
	// produce polyline loop
	idList<idVec3> pathPoints;
	pathPoints.AddGrow( startPortal->winding->GetCenter() );
	for (int i = 0; i < pathPortals.Num(); i++)
		pathPoints.AddGrow( pathPortals[i]->winding->GetCenter() );
	pathPoints.AddGrow( startPortal->winding->GetCenter() );

	idStr pos = startPortal->winding->GetCenter().ToString();
	common->Warning ( "Portal %d at (%s) dropped%s", brushnum, pos.c_str(), (leakyBoundary ? " as leaky" : "") );
	pos.Replace('.', 'd');
	pos.Replace('-', 'm');
	pos.Replace(' ', '_');
	idStr filename;
	sprintf( filename, "%s_portal%c_%s.lin", dmapGlobals.mapFileBase, (leakyBoundary ? 'L' : 'D'), pos.c_str() );

	idStr ospath = fileSystem->RelativePathToOSPath( filename, "fs_devpath", "" );
	FILE *linefile = fopen( ospath, "w" );
	if ( !linefile )
		common->Error( "Couldn't open %s\n", ospath.c_str() );
	for ( idVec3 p : pathPoints )
		fprintf( linefile, "%f %f %f\n", p.x, p.y, p.z );
	fclose( linefile );

	common->Printf( "saved %s (%i points)\n", filename.c_str(), pathPoints.Num() );
	return true;
}

/*
=============
DetectUnusedAreaPortals_r

traverses whole BSP tree, finds "dropped" visportals (i.e. having same area on both sides)
=============
*/
static void DetectUnusedAreaPortals_r( uEntity_t *entity, node_t *node ) {
	if ( node->planenum != PLANENUM_LEAF ) {
		DetectUnusedAreaPortals_r( entity, node->children[0] );
		DetectUnusedAreaPortals_r( entity, node->children[1] );
		return;
	}

	if ( node->opaque )
		return;

	int s;
	for ( uPortal_t *p = node->portals; p; p = p->next[s] ) {
		s = (p->nodes[1] == node);

		node_t *other = other = p->nodes[!s];
		if ( other->opaque )
			continue;

		visportalInfo_s info;
		if ( !FindVisportalsAtPortal(p, &info, 1) )
			continue;
		side_t *side = info.side;
		if ( !side )
			continue;

		idWinding *w = side->visibleHull;
		if ( !w )
			continue;

		interAreaPortal_t iap;
		if ( side->planenum == p->onnode->planenum ) {
			iap.area0 = p->nodes[0]->area;
			iap.area1 = p->nodes[1]->area;
		} else {
			iap.area0 = p->nodes[1]->area;
			iap.area1 = p->nodes[0]->area;
		}
		iap.side = side;
		iap.brush = info.brush;

		// see if we have created visportal here
		int i;
		for ( i = 0 ; i < numInterAreaPortals ; i++ ) {
			if ( IsPortalSame( &iap, &interAreaPortals[i] ) )
				break;
		}
		if ( i != numInterAreaPortals )
			continue;

		// see if we already reported it as dropped
		for ( i = 0 ; i < numDroppedAreaPortals ; i++ ) {
			if ( IsPortalSame( &iap, &droppedAreaPortals[i] ) )
				break;
		}
		if ( i != numDroppedAreaPortals )
			continue;

		// TODO: what about dropped visportals separating two areas?
		// is it true that such situation never happens?
		if ( other->area != node->area ) {
			common->Warning("Inter-area portal dropped at %s", p->winding->GetCenter().ToString());
			continue;
		}

		// stop wasting time if there are too many dropped portals already
		if (numDroppedAreaPortals < 100) {
			ReportDroppedPortal(entity, p);
		}
		droppedAreaPortals[numDroppedAreaPortals++] = iap;
	}
}

/*
=============
ReportUnreferencedAreaPortals

find any portals which did not get into level and was not otherwise reported yet
=============
*/
void ReportUnreferencedAreaPortals( uEntity_t *entity ) {
	for ( primitive_t *prim = entity->primitives; prim; prim = prim->next ) {
		uBrush_t *b = prim->brush;
		if ( !b )
			continue;
		if ( !(b->contents & CONTENTS_AREAPORTAL) )
			continue;

		for ( int i = 0 ; i < b->numsides ; i++ ) {
			side_t *side = &b->sides[i];
			if ( !(side->material->GetContentFlags() & CONTENTS_AREAPORTAL) )
				continue;

			bool referenced = false;
			for ( int j = 0 ; !referenced && j < numInterAreaPortals ; j++ )
				if ( interAreaPortals[j].side == side )
					referenced = true;
			for ( int j = 0 ; !referenced && j < numDroppedAreaPortals ; j++ )
				if ( droppedAreaPortals[j].side == side )
					referenced = true;
			for ( int j = 0 ; !referenced && j < numOverlappingAreaPortals ; j++ )
				if ( overlappingAreaPortals[j].side == side )
					referenced = true;

			if ( !referenced ) {
				common->Warning( "Portal %d at (%s) is useless", b->brushnum, side->winding->GetCenter().ToString() );
			}
			break;
		}
	}
}

/*
=============
CheckInfoLocations

Verify that info_location entities are in different map areas.
Report any issues as warnings and pointfiles.
=============
*/
static void CheckInfoLocations(uEntity_t *e) {
	// is there location separator at each inter-area portal?
	idList<const char *> separatorPerVisportal;
	separatorPerVisportal.SetNum(numInterAreaPortals);
	separatorPerVisportal.FillZero();
	idList<bool> visportalBlocked;
	visportalBlocked.SetNum(numInterAreaPortals);
	visportalBlocked.FillZero();

	// read location separators and fill separatorPerVisportal
	for (int entnum = 1; entnum < dmapGlobals.num_entities; entnum++) {
		const idDict &spawnargs = dmapGlobals.uEntities[entnum].mapEntity->epairs;

		const char *classname = spawnargs.GetString("classname", "");
		bool isSeparator = idStr::Icmp(classname, "info_locationseparator") == 0;
		bool isSettings = idStr::Icmp(classname, "info_portalsettings") == 0;	// #6224
		if ( !isSeparator && !isSettings )
			continue;
		const char *name = spawnargs.GetString("name", "???");
		idVec3 origin = spawnargs.GetVector("origin");

		idBounds box = idPortalEntity::GetBounds(origin);
		int cnt = 0, prevJ = -1;
		for (int j = 0; j < numInterAreaPortals; j++) {
			const idWinding &w = *interAreaPortals[j].side->winding;
			int brushnum = interAreaPortals[j].brush->brushnum;
			if (!idRenderWorldLocal::DoesVisportalContactBox(w, box))
				continue;

			if (isSeparator)
				visportalBlocked[j] = true;	// #6224: only separator blocks pathfinding (see below)

			const char* &refSep = separatorPerVisportal[j];
			if (refSep) {
				common->Warning(
					"Separators %s and %s cover same portal %d at %s",
					refSep, name, brushnum, w.GetCenter().ToString()
				);
			}
			else
				refSep = name;

			if (prevJ >= 0) {
				const idWinding &prevW = *interAreaPortals[prevJ].side->winding;
				int prevNum = interAreaPortals[prevJ].brush->brushnum;
				common->Warning(
					"Separator %s covers both portal %d at %s and portal %d at %s",
					name, prevNum, prevW.GetCenter().ToString(), brushnum, w.GetCenter().ToString()
				);
			}
			cnt++;
			prevJ = j;
		}

		if (cnt == 0) {
			common->Warning("Separator %s does not cover any portal", name);
		}
	}

	auto CanPass_Locations = [&visportalBlocked](node_t *from, uPortal_t *through, node_t *to) -> bool {
		if ( !Portal_Passable( through ) )
			return false;					// going into solid

		visportalInfo_t info;
		if ( !FindVisportalsAtPortal( through, &info, 1 ) )
			return true;					// can go
		interAreaPortal_t iap = {0};
		iap.side = info.side;
		iap.area0 = through->nodes[0]->area;
		iap.area1 = through->nodes[1]->area;

		// find visportal we are going through
		for (int i = 0; i < numInterAreaPortals; i++) {
			if (IsPortalSame(&iap, &interAreaPortals[i])) {
				// check if it is covered by location separator (portal settings ignored here)
				if (visportalBlocked[i])
					return false;
			}
		}

		return true;
	};

	struct ILTag {
		const char *name = NULL;
		idVec3 origin;
		node_t *node = NULL;
	};
	// which info_location occupies each area
	idList<ILTag> ilInArea;
	ilInArea.SetNum(e->numAreas);

	// clear occupied marks now
	// we will run many searches without clearing marks in-between
	ClearOccupied_r( e->tree->headnode );

	// read location entities and flood BSP tree from each one
	for (int entnum = 1; entnum < dmapGlobals.num_entities; entnum++) {
		const idDict &spawnargs = dmapGlobals.uEntities[entnum].mapEntity->epairs;

		const char *classname = spawnargs.GetString("classname", "");
		if ( idStr::Icmp(classname, "info_location") != 0)
			continue;
		const char *name = spawnargs.GetString("name", "???");
		idVec3 origin = spawnargs.GetVector("origin");

		node_t *node = FindLeafNodeAtPoint( e->tree->headnode, origin );
		if (node->opaque) {
			common->Warning("Location %s is inside opaque geometry", name);
			continue;
		}

		int area = node->area;
		if (!ilInArea[area].node) {
			// there is no info_location in this area yet
			// mark all reachable areas as occupied by this info_location
			idList<node_t*> visited;
			FindShortestPathThroughBspNodes(
				NULL, {node}, NULL,
				LambdaToFuncPtr(CanPass_Locations), &CanPass_Locations,
				NULL, NULL, &visited
			);
			for (int i = 0; i < visited.Num(); i++) {
				int visArea = visited[i]->area;
				ilInArea[visArea].name = name;
				ilInArea[visArea].node = node;
				ilInArea[visArea].origin = origin;
			}
		}
		else {
			// this area already occupied by another location entity
			// find path between them and report it
			const ILTag &tag = ilInArea[area];

			// we have to clear marks now, since we search path in already visited area
			ClearOccupied_r( e->tree->headnode );

			idList<node_t*> pathNodes;
			idList<uPortal_t*> pathPortals;
			bool found = FindShortestPathThroughBspNodes(
				NULL, {node}, tag.node,
				LambdaToFuncPtr(CanPass_Locations), &CanPass_Locations,
				&pathNodes, &pathPortals, NULL
			);
			if (!found) {
				common->Warning("Failed to find path between overlapping info_locations");
				return;
			}

			idList<idVec3> pathPoints;
			pathPoints.AddGrow(origin);
			for (int j = 0; j < pathPortals.Num(); j++)
				pathPoints.AddGrow(pathPortals[j]->winding->GetCenter());
			pathPoints.AddGrow(tag.origin);

			common->Warning ( "Locations %s and %s overlap", name, tag.name );
			idStr filename;
			sprintf( filename, "%s_locationO_%s_%s.lin", dmapGlobals.mapFileBase, name, tag.name );

			idStr ospath = fileSystem->RelativePathToOSPath( filename, "fs_devpath", "" );
			FILE *linefile = fopen( ospath, "w" );
			if ( !linefile )
				common->Error( "Couldn't open %s\n", ospath.c_str() );
			for ( idVec3 p : pathPoints )
				fprintf( linefile, "%f %f %f\n", p.x, p.y, p.z );
			fclose( linefile );

			common->Printf( "saved %s (%i points)\n", filename.c_str(), pathPoints.Num() );
		}
	}
}


/*
=============
FloodAreas

Mark each leaf with an area, bounded by CONTENTS_AREAPORTAL
Sets e->areas.numAreas
=============
*/
void FloodAreas( uEntity_t *e ) {
	TRACE_CPU_SCOPE_TEXT("FloodAreas", e->nameEntity)
	PrintIfVerbosityAtLeast( VL_ORIGDEFAULT, "--- FloodAreas ---\n");

	// set all areas to -1
	ClearAreas_r( e->tree->headnode );

	// flood fill from non-opaque areas
	c_areas = 0;
	FindAreas_r( e->tree->headnode );

	PrintIfVerbosityAtLeast( VL_ORIGDEFAULT, "%5i areas\n", c_areas);
	e->numAreas = c_areas;

	// make sure we got all of them
	CheckAreas_r( e->tree->headnode );

	// identify all portals between areas if this is the world
	if ( e == &dmapGlobals.uEntities[0] ) {
		numInterAreaPortals = 0;
		numOverlappingAreaPortals = 0;
		FindInterAreaPortals_r( e->tree->headnode );

		// stgatilov #5129: detecting dropped portals
		numDroppedAreaPortals = 0;
		DetectUnusedAreaPortals_r(e, e->tree->headnode);
		// stgatilov #5129: detecting other issues
		// for instance, portals fully inside opaque or on opaque surface
		ReportUnreferencedAreaPortals(e);

		// stgatilov #5354: check info_location-s
		CheckInfoLocations(e);
	}
}

/*
=========================================================

FLOOD ENTITIES

=========================================================
*/

/*
=============
FloodEntities

Marks all nodes that can be reached by entites
stgatilov #5592: also report leak if it exists
=============
*/
bool FloodEntities( tree_t *tree ) {
	PrintIfVerbosityAtLeast( VL_ORIGDEFAULT, "--- FloodEntities ---\n");
	TRACE_CPU_SCOPE("FloodEntities")

	// stgatilov: list of nodes where entities are put
	idList<node_t*> entNodes;
	idList<int> entIds;

	// collect all entities to flood from
	for (int i = 1; i < dmapGlobals.num_entities; i++) {
		idMapEntity	*mapEnt = dmapGlobals.uEntities[i].mapEntity;

		idVec3 origin;
		if ( !mapEnt->epairs.GetVector( "origin", "", origin) ) {
			continue;
		}

		// any entity can have "noFlood" set to skip it
		const char *cl;
		if ( mapEnt->epairs.GetString( "noFlood", "", &cl ) ) {
			continue;
		}

		mapEnt->epairs.GetString( "classname", "", &cl );

		if ( !strcmp( cl, "light" ) ) {
			const char	*v;

			// don't place lights that have a light_start field, because they can still
			// be valid if their origin is outside the world
			mapEnt->epairs.GetString( "light_start", "", &v);
			if ( v[0] ) {
				continue;
			}

			// don't place fog lights, because they often
			// have origins outside the light
			mapEnt->epairs.GetString( "texture", "", &v);
			if ( v[0] ) {
				const idMaterial *mat = declManager->FindMaterial( v );
				if ( mat->IsFogLight() ) {
					continue;
				}
			}
		}

		node_t *node = FindLeafNodeAtPoint( tree->headnode, origin );
		if ( node->opaque ) {
			continue;
		}

		entNodes.AddGrow(node);
		entIds.AddGrow(i);
	}

	ClearOccupied_r( tree->headnode );
	tree->outside_node.occupied = 0;
	auto CanPass_NonOpaque = [tree](node_t *from, uPortal_t *through, node_t *to) -> bool {
		if ( Portal_Passable(through) )
			return true;
		if ( to == &tree->outside_node )
			return true;
		return false;
	};
	// run flood algorithm from all nodes containing entities simultaneously
	idList<node_t*> pathNodes;
	idList<uPortal_t*> pathPortals;
	bool found = FindShortestPathThroughBspNodes(
		NULL, entNodes, &tree->outside_node,
		LambdaToFuncPtr(CanPass_NonOpaque), &CanPass_NonOpaque,
		&pathNodes, &pathPortals, NULL
	);

	if ( found ) {
		// find any entity in the node path starts from
		node_t *startNode = pathNodes[0];
		int q = entNodes.FindIndex(startNode);
		assert(q >= 0);

		int i = entIds[q];
		idMapEntity	*mapEnt = dmapGlobals.uEntities[i].mapEntity;

		PrintIfVerbosityAtLeast( VL_CONCISE, "Leak on entity # %d\n", i );
		const char *classname = mapEnt->epairs.GetString( "classname", "" );
		PrintIfVerbosityAtLeast( VL_CONCISE, "Entity classname was: %s\n", classname );
		const char *name = mapEnt->epairs.GetString( "name", "" );
		PrintIfVerbosityAtLeast( VL_CONCISE, "Entity name was: %s\n", name );
		idVec3 origin = mapEnt->epairs.GetVector( "origin" );
		PrintIfVerbosityAtLeast( VL_CONCISE, "Entity origin is: %f %f %f\n", origin.x, origin.y, origin.z);

		idList<idVec3> pathPoints;
		pathPoints.AddGrow(origin);
		for (int j = 0; j < pathPortals.Num(); j++)
			pathPoints.AddGrow(pathPortals[j]->winding->GetCenter());
		pathPoints.Reverse();

		common->Warning ( "Leak detected to entity %s", name );
		idStr filename;
		sprintf( filename, "%s.lin", dmapGlobals.mapFileBase );

		idStr ospath = fileSystem->RelativePathToOSPath( filename, "fs_devpath", "" );
		FILE *linefile = fopen( ospath, "w" );
		if ( !linefile )
			common->Error( "Couldn't open %s\n", ospath.c_str() );
		for ( idVec3 p : pathPoints )
			fprintf( linefile, "%f %f %f\n", p.x, p.y, p.z );
		fclose( linefile );

		common->Printf( "saved %s (%i points)\n", filename.c_str(), pathPoints.Num() );
	}

	if ( entNodes.Num() == 0 ) {
		PrintIfVerbosityAtLeast( VL_CONCISE, "no entities in open -- no filling\n");
		return false;
	}
	else if ( tree->outside_node.occupied ) {
		PrintIfVerbosityAtLeast( VL_CONCISE, "entity reached from outside -- no filling\n");
		return false;
	}
	return true;
}

/*
======================================================

FILL OUTSIDE

======================================================
*/

static	int		c_outside;
static	int		c_inside;
static	int		c_solid;

void FillOutside_r (node_t *node)
{
	if (node->planenum != PLANENUM_LEAF)
	{
		FillOutside_r (node->children[0]);
		FillOutside_r (node->children[1]);
		return;
	}

	// anything not reachable by an entity
	// can be filled away
	if (!node->occupied) {
		if ( !node->opaque ) {
			c_outside++;
			node->opaque = true;
		} else {
			c_solid++;
		}
	} else {
		c_inside++;
	}

}

/*
=============
FillOutside

Fill (set node->opaque = true) all nodes that can't be reached by entities
=============
*/
void FillOutside( uEntity_t *e ) {
	TRACE_CPU_SCOPE("FillOutside")
	c_outside = 0;
	c_inside = 0;
	c_solid = 0;
	PrintIfVerbosityAtLeast( VL_ORIGDEFAULT, "--- FillOutside ---\n" );
	FillOutside_r( e->tree->headnode );
	PrintIfVerbosityAtLeast( VL_ORIGDEFAULT, "%5i solid leafs\n", c_solid );
	PrintIfVerbosityAtLeast( VL_ORIGDEFAULT, "%5i leafs filled\n", c_outside );
	PrintIfVerbosityAtLeast( VL_ORIGDEFAULT, "%5i inside leafs\n", c_inside );
}
