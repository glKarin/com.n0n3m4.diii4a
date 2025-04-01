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

#ifndef __BRUSHBSP_H__
#define __BRUSHBSP_H__

/*
===============================================================================

	BrushBSP

===============================================================================
*/

class idBrushBSP;
class idBrushBSPNode;
class idBrushBSPPortal;


//===============================================================
//
//	idBrushBSPPortal
//
//===============================================================

class idBrushBSPPortal {

	friend class idBrushBSP;
	friend class idBrushBSPNode;

public:
							idBrushBSPPortal( void );
							~idBrushBSPPortal( void );
	void					AddToNodes( idBrushBSPNode *front, idBrushBSPNode *back );
	void					RemoveFromNode( idBrushBSPNode *l );
	void					Flip( void );
	int						Split( const idPlane &splitPlane, idBrushBSPPortal **front, idBrushBSPPortal **back );
	idWinding *				GetWinding( void ) const { return winding; }
	const idPlane &			GetPlane( void ) const { return plane; }
	void					SetFaceNum( int num ) { faceNum = num; }
	int						GetFaceNum( void ) const { return faceNum; }
	int						GetFlags( void ) const { return flags; }
	void					SetFlag( int flag ) { flags |= flag; }
	void					RemoveFlag( int flag ) { flags &= ~flag; }
	idBrushBSPPortal *		Next( int side ) const { return next[side]; }
	idBrushBSPNode *		GetNode( int side ) const { return nodes[side]; }

private:
	idPlane					plane;			// portal plane
	int						planeNum;		// number of plane this portal is on
	idWinding *				winding;		// portal winding
	idBrushBSPNode *		nodes[2];		// nodes this portal seperates
	idBrushBSPPortal *		next[2];		// next portal in list for both nodes
	int						flags;			// portal flags
	int						faceNum;		// number of the face created for this portal
};


//===============================================================
//
//	idBrushBSPNode
//
//===============================================================

// stgatilov #5212: this node was merged and deleted; parent points to the node it merged into
// used in idAASBuild::MergeLeafNodes for reliable substitution of references
#define NODE_ZOMBIE			BIT(29)
#define NODE_VISITED		BIT(30)
#define NODE_DONE			BIT(31)

class idBrushBSPNode {

	friend class idBrushBSP;
	friend class idBrushBSPPortal;

public:
							idBrushBSPNode( void );
							~idBrushBSPNode( void );
	void					SetContentsFromBrushes( void );
	idBounds				GetPortalBounds( void );
	idBrushBSPNode *		GetChild( int index ) const { return children[index]; }
	idBrushBSPNode *		GetParent( void ) const { return parent; }
	void					SetContents( int contents ) { this->contents = contents; }
	int						GetContents( void ) const { return contents; }
	const idPlane &			GetPlane( void ) const { return plane; }
	idBrushBSPPortal *		GetPortals( void ) const { return portals; }
	void					SetAreaNum( int num ) { areaNum = num; }
	int						GetAreaNum( void ) const { return areaNum; }
	int						GetFlags( void ) const { return flags; }
	void					SetFlag( int flag ) { flags |= flag; }
	void					RemoveFlag( int flag ) { flags &= ~flag; }
	bool					TestLeafNode( void );
							// remove the flag from nodes found by flooding through portals to nodes with the flag set
	void					RemoveFlagFlood( int flag );
							// recurse down the tree and remove the flag from all visited nodes
	void					RemoveFlagRecurse( int flag );
							// first recurse down the tree and flood from there
	void					RemoveFlagRecurseFlood( int flag );
							// returns side of the plane the node is on
	int						PlaneSide( const idPlane &plane, float epsilon = ON_EPSILON ) const;
							// split the leaf node with a plane
	bool					Split( const idPlane &splitPlane, int splitPlaneNum );


private:
	idPlane					plane;			// split plane if this is not a leaf node
	idBrush *				volume;			// node volume
	int						contents;		// node contents
	idBrushList				brushList;		// list with brushes for this node
	idBrushBSPNode *		parent;			// parent of this node
	idBrushBSPNode *		children[2];	// both are NULL if this is a leaf node
	idBrushBSPPortal *		portals;		// portals of this node
	int						flags;			// node flags
	int						areaNum;		// number of the area created for this node
	int						occupied;		// true when portal is occupied
};


//===============================================================
//
//	idBrushBSP
//
//===============================================================

class idBrushBSP {

public:
							idBrushBSP( void );
							~idBrushBSP( void );
							// build a bsp tree from a set of brushes
	void					Build( idBrushList brushList, int skipContents,
											bool (*ChopAllowed)( idBrush *b1, idBrush *b2 ),
														bool (*MergeAllowed)( idBrush *b1, idBrush *b2 ) );
							// remove splits in subspaces with the given contents
	void					PruneTree( int contents );
							// portalize the bsp tree
	void					Portalize( void );
							// remove subspaces outside the map not reachable by entities
	bool					RemoveOutside( const idMapFile *mapFile, int contents, const idStrList &classNames );
							// write file with a trace going through a leak
	void					LeakFile( const idStr &fileName );
							// try to merge portals
	void					MergePortals( int skipContents );
							// try to merge the two leaf nodes at either side of the portal
	bool					TryMergeLeafNodes( idBrushBSPPortal *portal, int side, idList<idBrushBSPNode*> &zombieNodes );
	void					PruneMergedTree_r( idBrushBSPNode *node );
							// melt portal windings
	void					MeltPortals( int skipContents );
							// write a map file with a brush for every leaf node that has the given contents
	void					WriteBrushMap( const idStr &fileName, const idStr &ext, int contents );
							// bounds for the whole tree
	const idBounds &		GetTreeBounds( void ) const { return treeBounds; }
							// root node of the tree
	idBrushBSPNode *		GetRootNode( void ) const { return root; }

private:
	idBrushBSPNode *		root;
	idBrushBSPNode *		outside;
	idBounds				treeBounds;
	idPlaneSet				portalPlanes;
	int						numGridCells;
	int						numSplits;
	int						numGridCellSplits;
	int						numPrunedSplits;
	int						numPortals;
	int						solidLeafNodes;
	int						outsideLeafNodes;
	int						insideLeafNodes;
	int						numMergedPortals;
	int						numInsertedPoints;
	idVec3					leakOrigin;
	int						brushMapContents;
	idBrushMap *			brushMap;

	bool					(*BrushChopAllowed)( idBrush *b1, idBrush *b2 );
	bool					(*BrushMergeAllowed)( idBrush *b1, idBrush *b2 );

private:
	void					RemoveMultipleLeafNodeReferences_r( idBrushBSPNode *node );
	void					Free_r( idBrushBSPNode *node );
	void					IncreaseNumSplits( void );
	bool					IsValidSplitter( const idBrushSide *side );
	int						BrushSplitterStats( const idBrush *brush, int planeNum, const idPlaneSet &planeList, bool *testedPlanes, struct splitterStats_s &stats );
	int						FindSplitter( idBrushBSPNode *node, const idPlaneSet &planeList, bool *testedPlanes, struct splitterStats_s &bestStats );
	void					SetSplitterUsed( idBrushBSPNode *node, int planeNum );
	idBrushBSPNode *		BuildBrushBSP_r( idBrushBSPNode *node, const idPlaneSet &planeList, bool *testedPlanes, int skipContents );
	idBrushBSPNode *		ProcessGridCell( idBrushBSPNode *node, int skipContents );
	void					BuildGrid_r( idList<idBrushBSPNode *> &gridCells, idBrushBSPNode *node );
	void					PruneTree_r( idBrushBSPNode *node, int contents );
	void					MakeOutsidePortals( void );
	void					MakeNodePortal( idBrushBSPNode *node );
	void					SplitNodePortals( idBrushBSPNode *node );
	void					MakeTreePortals_r( idBrushBSPNode *node );
	void					FloodThroughPortals_r( idBrushBSPNode *node, int contents, int depth );
	bool					FloodFromOrigin( const idVec3 &origin, int contents );
	bool					FloodFromEntities( const idMapFile *mapFile, int contents, const idStrList &classNames );
	void					RemoveOutside_r( idBrushBSPNode *node, int contents );
	void					SetPortalPlanes_r( idBrushBSPNode *node, idPlaneSet &planeList );
	void					SetPortalPlanes( void );
	void					MergePortals_r( idBrushBSPNode *node, int skipContents );
	void					MergeLeafNodePortals( idBrushBSPNode *node, int skipContents );
	void					UpdateTreeAfterMerge_r( idBrushBSPNode *node, const idBounds &bounds, idBrushBSPNode *oldNode, idBrushBSPNode *newNode );
	void					RemoveLeafNodeColinearPoints( idBrushBSPNode *node );
	void					RemoveColinearPoints_r( idBrushBSPNode *node, int skipContents );
	void					MeltFlood_r( idBrushBSPNode *node, int skipContents, idBounds &bounds, idVectorSet<idVec3,3> &vertexList );
	void					MeltLeafNodePortals( idBrushBSPNode *node, int skipContents, idVectorSet<idVec3,3> &vertexList );
	void					MeltPortals_r( idBrushBSPNode *node, int skipContents, idVectorSet<idVec3,3> &vertexList );
};

#endif /* !__BRUSHBSP_H__ */
