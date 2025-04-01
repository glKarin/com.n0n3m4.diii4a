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

int			c_faceLeafs;


extern	int	c_nodes;

void RemovePortalFromNode( uPortal_t *portal, node_t *l );

node_t *NodeForPoint( node_t *node, idVec3 origin ) {
	float	d;

	while( node->planenum != PLANENUM_LEAF ) {
		idPlane &plane = dmapGlobals.mapPlanes[node->planenum];
		d = plane.Distance( origin );
		if ( d >= 0 ) {
			node = node->children[0];
		} else {
			node = node->children[1];
		}
	}

	return node;
}



/*
=============
FreeTreePortals_r
=============
*/
void FreeTreePortals_r (node_t *node)
{
	uPortal_t	*p, *nextp;
	int			s;

	// free children
	if (node->planenum != PLANENUM_LEAF)
	{
		FreeTreePortals_r (node->children[0]);
		FreeTreePortals_r (node->children[1]);
	}

	// free portals
	for (p=node->portals ; p ; p=nextp)
	{
		s = (p->nodes[1] == node);
		nextp = p->next[s];

		RemovePortalFromNode (p, p->nodes[!s]);
		FreePortal (p);
	}
	node->portals = NULL;
}

/*
=============
FreeTree_r
=============
*/
void FreeTree_r (node_t *node)
{
	// free children
	if (node->planenum != PLANENUM_LEAF)
	{
		FreeTree_r (node->children[0]);
		FreeTree_r (node->children[1]);
	}

	// free brushes
	FreeBrushList (node->brushlist);

	// free the node
	c_nodes--;
	Mem_Free (node);
}


/*
=============
FreeTree
=============
*/
void FreeTree( tree_t *tree ) {
	if ( !tree ) {
		return;
	}
	FreeTreePortals_r (tree->headnode);
	FreeTree_r (tree->headnode);
	Mem_Free (tree);
}

//===============================================================

void PrintTree_r (node_t *node, int depth)
{
	int			i;
	uBrush_t	*bb;

	for (i=0 ; i<depth ; i++)
		common->Printf("  ");
	if (node->planenum == PLANENUM_LEAF)
	{
		if (!node->brushlist)
			common->Printf("NULL\n");
		else
		{
			for (bb=node->brushlist ; bb ; bb=bb->next)
				common->Printf("%i ", bb->original->brushnum);
			common->Printf("\n");
		}
		return;
	}

	idPlane &plane = dmapGlobals.mapPlanes[node->planenum];
	common->Printf( "#%i (%5.2f %5.2f %5.2f %5.2f)\n", node->planenum,
					plane[0], plane[1], plane[2], plane[3] );
	PrintTree_r( node->children[0], depth+1 );
	PrintTree_r( node->children[1], depth+1 );
}

/*
================
AllocBspFace
================
*/
bspface_t	*AllocBspFace( void ) {
	bspface_t	*f;

	f = (bspface_t *)Mem_Alloc(sizeof(*f));
	memset( f, 0, sizeof(*f) );

	return f;
}

/*
================
FreeBspFace
================
*/
void	FreeBspFace( bspface_t *f ) {
	if ( f->w ) {
		delete f->w;
	}
	Mem_Free( f );
}


/*
================
SelectSplitPlaneNum
================
*/
#define	BLOCK_SIZE	1024
int SelectSplitPlaneNum( node_t *node, bspface_t *list ) {
	bspface_t	*split;
	bspface_t	*check;
	bspface_t	*bestSplit;
	int			splits, facing, front, back;
	int			side;
	idPlane		*mapPlane;
	int			value, bestValue;
	idPlane		plane;
	int			planenum;
	bool	havePortals;
	float		dist;
	idVec3		halfSize;

	// if it is crossing a 1k block boundary, force a split
	// this prevents epsilon problems from extending an
	// arbitrary distance across the map

	halfSize = ( node->bounds[1] - node->bounds[0] ) * 0.5f;
	for ( int axis = 0; axis < 3; axis++ ) {
		if ( halfSize[axis] > BLOCK_SIZE ) {
			dist = BLOCK_SIZE * ( floor( ( node->bounds[0][axis] + halfSize[axis] ) / BLOCK_SIZE ) + 1.0f );
		} else {
			dist = BLOCK_SIZE * ( floor( node->bounds[0][axis] / BLOCK_SIZE ) + 1.0f );
		}
		if ( dist > node->bounds[0][axis] + 1.0f && dist < node->bounds[1][axis] - 1.0f ) {
			plane[0] = plane[1] = plane[2] = 0.0f;
			plane[axis] = 1.0f;
			plane[3] = -dist;
			planenum = FindFloatPlane( plane );
			return planenum;
		}
	}

	// pick one of the face planes
	// if we have any portal faces at all, only
	// select from them, otherwise select from
	// all faces
	bestValue = -999999;
	bestSplit = list;

	havePortals = false;
	for ( split = list ; split ; split = split->next ) {
		split->checked = false;
		if ( split->portal ) {
			havePortals = true;
		}
	}

	for ( split = list ; split ; split = split->next ) {
		if ( split->checked ) {
			continue;
		}
		if ( havePortals != split->portal ) {
			continue;
		}
		mapPlane = &dmapGlobals.mapPlanes[ split->planenum ];
		splits = 0;
		facing = 0;
		front = 0;
		back = 0;
		for ( check = list ; check ; check = check->next ) {
			if ( check->planenum == split->planenum ) {
				facing++;
				check->checked = true;	// won't need to test this plane again
				continue;
			}
			side = check->w->PlaneSide( *mapPlane );
			if ( side == SIDE_CROSS ) {
				splits++;
			} else if ( side == SIDE_FRONT ) {
				front++;
			} else if ( side == SIDE_BACK ) {
				back++;
			}
		}
		value =  5*facing - 5*splits; // - abs(front-back);
		if ( mapPlane->Type() < PLANETYPE_TRUEAXIAL ) {
			value+=5;		// axial is better
		}

		if ( value > bestValue ) {
			bestValue = value;
			bestSplit = split;
		}
	}

	if ( bestValue == -999999 ) {
		return -1;
	}

	return bestSplit->planenum;
}

/*
================
BuildFaceTree_r
================
*/
void	BuildFaceTree_r( node_t *node, bspface_t *list ) {
	bspface_t	*split;
	bspface_t	*next;
	int			side;
	bspface_t	*newFace;
	bspface_t	*childLists[2];
	idWinding	*frontWinding, *backWinding;
	int			i;
	int			splitPlaneNum;

	splitPlaneNum = SelectSplitPlaneNum( node, list );
	// if we don't have any more faces, this is a node
	if ( splitPlaneNum == -1 ) {
		node->planenum = PLANENUM_LEAF;
		c_faceLeafs++;
		return;
	}

	// partition the list
	node->planenum = splitPlaneNum;
	idPlane &plane = dmapGlobals.mapPlanes[ splitPlaneNum ];
	childLists[0] = NULL;
	childLists[1] = NULL;
	for ( split = list ; split ; split = next ) {
		next = split->next;

		if ( split->planenum == node->planenum ) {
			FreeBspFace( split );
			continue;
		}

		side = split->w->PlaneSide( plane );

		if ( side == SIDE_CROSS ) {
			split->w->Split( plane, CLIP_EPSILON * 2, &frontWinding, &backWinding );
			if ( frontWinding ) {
				newFace = AllocBspFace();
				newFace->w = frontWinding;
				newFace->next = childLists[0];
				newFace->planenum = split->planenum;
				childLists[0] = newFace;
			}
			if ( backWinding ) {
				newFace = AllocBspFace();
				newFace->w = backWinding;
				newFace->next = childLists[1];
				newFace->planenum = split->planenum;
				childLists[1] = newFace;
			}
			FreeBspFace( split );
		} else if ( side == SIDE_FRONT ) {
			split->next = childLists[0];
			childLists[0] = split;
		} else if ( side == SIDE_BACK ) {
			split->next = childLists[1];
			childLists[1] = split;
		}
	}


	// recursively process children
	for ( i = 0 ; i < 2 ; i++ ) {
		node->children[i] = AllocNode();
		node->children[i]->parent = node;
		node->children[i]->bounds = node->bounds;
	}

	// split the bounds if we have a nice axial plane
	for ( i = 0 ; i < 3 ; i++ ) {
		if ( idMath::Fabs( plane[i] - 1.0 ) < 0.001 ) {
			node->children[0]->bounds[0][i] = plane.Dist();
			node->children[1]->bounds[1][i] = plane.Dist();
			break;
		}
	}

	for ( i = 0 ; i < 2 ; i++ ) {
		BuildFaceTree_r ( node->children[i], childLists[i]);
	}
}


/*
================
FaceBSP

List will be freed before returning
================
*/
tree_t *FaceBSP( bspface_t *list ) {
	tree_t		*tree;
	bspface_t	*face;
	int			i;
	int			count;
	int			start, end;

	TRACE_CPU_SCOPE("FaceBSP")
	start = Sys_Milliseconds();

	PrintIfVerbosityAtLeast( VL_ORIGDEFAULT, "--- FaceBSP ---\n" );

	tree = AllocTree ();

	count = 0;
	tree->bounds.Clear();
	for ( face = list ; face ; face = face->next ) {
		count++;
		for ( i = 0 ; i < face->w->GetNumPoints() ; i++ ) {
			tree->bounds.AddPoint( (*face->w)[i].ToVec3() );
		}
	}
	PrintIfVerbosityAtLeast( VL_ORIGDEFAULT, "%5i faces\n", count );

	c_faceLeafs = 0;
	c_nodes = 0;
	tree->headnode = AllocNode();
	tree->headnode->bounds = tree->bounds;

	BuildFaceTree_r ( tree->headnode, list );
	tree->nodeCnt = c_nodes;
	tree->leafCnt = c_faceLeafs;

	PrintIfVerbosityAtLeast( VL_ORIGDEFAULT, "%5i leafs\n", c_faceLeafs );

	end = Sys_Milliseconds();

	verbosityLevel_t timingMsgVerbosity = ( end - start >= 50 ? VL_CONCISE : VL_ORIGDEFAULT ); // Only print if it'll show something other than 0.0
	PrintIfVerbosityAtLeast( timingMsgVerbosity, "%5.1f seconds faceBsp\n", ( end - start ) / 1000.0 );

	return tree;
}

//==========================================================================

idCVar dmap_bspAllSidesOfVisportal(
	"dmap_bspAllSidesOfVisportal", "1", CVAR_BOOL | CVAR_SYSTEM,
	"If set to 1, then all sides of a visportal brush are inserted into BSP tree. "
	"If set to 0, then only the side with visportal material is inserted (old style). "
	"When dmapping old missions, either fix all visportal warnings or set this to 0. "
);

/*
=================
MakeStructuralBspFaceList
=================
*/
bspface_t	*MakeStructuralBspFaceList( primitive_t *list ) {
	uBrush_t	*b;
	int			i;
	side_t		*s;
	idWinding	*w;
	bspface_t	*f, *flist;
	TRACE_CPU_SCOPE("MakeStructuralBspFaceList")

	flist = NULL;
	for ( ; list ; list = list->next ) {
		b = list->brush;
		if ( !b ) {
			continue;
		}
		if ( !b->opaque && !( b->contents & CONTENTS_AREAPORTAL ) ) {
			continue;
		}
		for ( i = 0 ; i < b->numsides ; i++ ) {
			s = &b->sides[i];
			w = s->winding;
			if ( !w ) {
				continue;
			}
			if ( !dmap_bspAllSidesOfVisportal.GetBool() &&	//stgatilov #5129
				( b->contents & CONTENTS_AREAPORTAL ) && ! ( s->material->GetContentFlags() & CONTENTS_AREAPORTAL ) ) {
				continue;
			}
			f = AllocBspFace();
			if ( s->material->GetContentFlags() & CONTENTS_AREAPORTAL ) {
				f->portal = true;
			}
			f->w = w->Copy();
			f->planenum = s->planenum & ~1;
			f->next = flist;
			flist = f;
		}
	}

	return flist;
}

/*
=================
MakeVisibleBspFaceList
=================
*/
bspface_t	*MakeVisibleBspFaceList( primitive_t *list ) {
	uBrush_t	*b;
	int			i;
	side_t		*s;
	idWinding	*w;
	bspface_t	*f, *flist;

	flist = NULL;
	for ( ; list ; list = list->next ) {
		b = list->brush;
		if ( !b ) {
			continue;
		}
		if ( !b->opaque && !( b->contents & CONTENTS_AREAPORTAL ) ) {
			continue;
		}
		for ( i = 0 ; i < b->numsides ; i++ ) {
			s = &b->sides[i];
			w = s->visibleHull;
			if ( !w ) {
				continue;
			}
			f = AllocBspFace();
			if ( s->material->GetContentFlags() & CONTENTS_AREAPORTAL ) {
				f->portal = true;
			}
			f->w = w->Copy();
			f->planenum = s->planenum & ~1;
			f->next = flist;
			flist = f;
		}
	}

	return flist;
}

