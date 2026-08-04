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



#include "AASBuild_local.h"

/*
============
idAASBuild::AllGapsLeadToOtherNode
============
*/
bool idAASBuild::AllGapsLeadToOtherNode( idBrushBSPNode *nodeWithGaps, idBrushBSPNode *otherNode ) {
	int s;
	idBrushBSPPortal *p;

	for ( p = nodeWithGaps->GetPortals(); p; p = p->Next(s) ) {
		s = (p->GetNode(1) == nodeWithGaps);

		if ( !PortalIsGap( p, s ) ) {
			continue;
		}

		if ( p->GetNode(!s) != otherNode ) {
			return false;
		}
	}
	return true;
}

/*
============
idAASBuild::MergeWithAdjacentLeafNodes
============
*/
bool idAASBuild::MergeWithAdjacentLeafNodes( idBrushBSP &bsp, idBrushBSPNode *node ) {
	int s, numMerges = 0, otherNodeFlags;
	idBrushBSPPortal *p;

	do {
		for ( p = node->GetPortals(); p; p = p->Next(s) ) {
			s = (p->GetNode(1) == node);

			// both leaf nodes must have the same contents
			if ( node->GetContents() != p->GetNode(!s)->GetContents() ) {
				continue;
			}

			// cannot merge leaf nodes if one is near a ledge and the other is not
			if ( (node->GetFlags() & AREA_LEDGE) != (p->GetNode(!s)->GetFlags() & AREA_LEDGE) ) {
				continue;
			}

			// cannot merge leaf nodes if one has a floor portal and the other a gap portal
			if ( node->GetFlags() & AREA_FLOOR ) {
				if ( p->GetNode(!s)->GetFlags() & AREA_GAP ) {
					if ( !AllGapsLeadToOtherNode( p->GetNode(!s), node ) ) {
						continue;
					}
				}
			}
			else if ( node->GetFlags() & AREA_GAP ) {
				if ( p->GetNode(!s)->GetFlags() & AREA_FLOOR ) {
					if ( !AllGapsLeadToOtherNode( node, p->GetNode(!s) ) ) {
						continue;
					}
				}
			}

			otherNodeFlags = p->GetNode(!s)->GetFlags();

			// try to merge the leaf nodes
			if ( bsp.TryMergeLeafNodes( p, s, zombieNodes ) ) {
				node->SetFlag( otherNodeFlags );
				if ( node->GetFlags() & AREA_FLOOR ) {
					node->RemoveFlag( AREA_GAP );
				}
				numMerges++;
				DisplayRealTimeString( "\r%6d", ++numMergedLeafNodes );
				break;
			}
		}
	} while( p );

	if ( numMerges ) {
		return true;
	}
	return false;
}

/*
============
idAASBuild::MergeLeafNodes_r
============
*/
void idAASBuild::MergeLeafNodes_r( idBrushBSP &bsp, idBrushBSPNode *node ) {

	if ( !node ) {
		return;
	}

	// stgatilov #5212: any zombie node was merged into some node X
	// the node X is surely marked as "done", so we can ignore it for now
	if ( node->GetFlags() & NODE_ZOMBIE ) {
		return;
	}

	if ( node->GetContents() & AREACONTENTS_SOLID ) {
		return;
	}

	if ( node->GetFlags() & NODE_DONE ) {
		return;
	}

	if ( !node->GetChild(0) && !node->GetChild(1) ) {
		MergeWithAdjacentLeafNodes( bsp, node );
		node->SetFlag( NODE_DONE );
		return;
	}

	MergeLeafNodes_r( bsp, node->GetChild(0) );
	MergeLeafNodes_r( bsp, node->GetChild(1) );

	return;
}

/*
============
idAASBuild::MergeLeafNodes
============
*/
void idAASBuild::MergeLeafNodes( idBrushBSP &bsp ) {
	numMergedLeafNodes = 0;

	TRACE_CPU_SCOPE("MergeLeafNodes")
	common->Printf( "[Merge Leaf Nodes]\n" );

	MergeLeafNodes_r( bsp, bsp.GetRootNode() );
	//bsp.GetRootNode()->RemoveFlagRecurse( NODE_DONE );	//stgatilov #5212: this is done inside PruneMergedTree_r
	bsp.PruneMergedTree_r( bsp.GetRootNode() );

	//stgatilov #5212: now that zombies are no longer referenced, delete them
	for ( int i = 0; i < zombieNodes.Num(); i++ ) {
		//note: we call destructor on already destructed object here
		//I checked this to be OK because zombies are fully zeroed
		delete zombieNodes[i];	
	}
	zombieNodes.ClearFree();

	common->Printf( "\r%6d leaf nodes merged\n", numMergedLeafNodes );
}
