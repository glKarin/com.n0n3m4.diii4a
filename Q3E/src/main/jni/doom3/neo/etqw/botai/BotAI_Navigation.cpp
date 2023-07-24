// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#endif

#include "../Game_local.h"
#include "../ContentMask.h"
#include "BotThreadData.h"
#include "BotAI_Main.h"

/*
================
idBotNodeGraph::Clear
================
*/
void idBotNodeGraph::Clear() {
	nodes.DeleteContents( true );
}

/*
================
idBotNodeGraph::Num
================
*/
int idBotNodeGraph::Num() {
	return nodes.Num();
}

/*
================
idBotNodeGraph::DeleteNode
================
*/
void idBotNodeGraph::DeleteNode( idBotNode *node ) {
	// first remove any links to this node
	for ( int i = 0; i < nodes.Num(); i++ ) {
		nodes[ i ]->RemoveLink( node );
	}

	delete node;
	nodes.RemoveFast( node );

	// Removing will rearrange the nodes, so we need to reset the numbers
	for ( int i = 0; i < nodes.Num(); i++ ) {
		nodes[ i ]->num = i;
	}
}

/*
================
idBotNodeGraph::SaveNodes
================
*/
void idBotNodeGraph::SaveNodes( const char * filename ) {

	int i, j;
	idVec3 origin;

	idFile *file = fileSystem->OpenFileWrite( filename );

	if ( file == NULL ) {
		gameLocal.Warning( "Can't open %s!", filename );
		return;
	}

	file->WriteInt( NODE_VERSION );	

	file->WriteInt( nodes.Num() );

	for( i = 0; i < nodes.Num(); i++ ) {
		idBotNode * node = nodes[ i ];
		file->WriteVec3( node->origin );
		file->WriteString( node->name );
		file->WriteFloat( node->radius );
		file->WriteInt( node->team );
		file->WriteBool( node->active );
		file->WriteInt( node->flags );
		file->WriteInt( node->links.Num() );

		for( j = 0; j < node->links.Num(); j++ ) {
			idBotNode::botLink_t & link = node->links[ j ];
			file->WriteInt( link.node->num );
			file->WriteFloat( link.cost );
		}
	}

	fileSystem->CloseFile( file );
}

/*
================
idBotNodeGraph::LoadNodes
Parse the bot nodes and load them up.
================
*/
void idBotNodeGraph::LoadNodes( const char *filename ) {
	int index;
	int team;
	int numNodes = 0;
	int version = 0;
	int numLinks;
	idVec3 origin;

	nodes.DeleteContents( true );

	idFile *file = fileSystem->OpenFileRead( filename );

	if ( file == NULL ) {
		gameLocal.DWarning( "Could not open %s", filename );
		return;
	}

	file->ReadInt( version );

	if ( version == OLD_NODE_VERSION ) { //mal: too late to redo the nodes now, so support the old format for a bit....
		file->ReadInt( numNodes );
	
		nodes.SetNum( numNodes );
		for( int i = 0; i < numNodes; i++ ) {
			nodes[ i ] = new idBotNode;
			nodes[ i ]->num = i;
		}

		for( int i = 0; i < numNodes; i++ ) {
			idBotNode * node = nodes[ i ];
			file->ReadVec3( node->origin );
			file->ReadString( node->name );
			file->ReadFloat( node->radius );
			file->ReadInt( team );
			node->team = ( playerTeamTypes_t ) team;
			file->ReadBool( node->active );
			file->ReadInt( numLinks );

			node->links.SetNum( numLinks );
			for( int j = 0; j < numLinks; j++ ) {
				idBotNode::botLink_t & link = node->links[ j ];
				file->ReadInt( index );
				link.node = nodes[ index ];
				file->ReadFloat( link.cost );
			}
		}
	} else if ( version == NODE_VERSION ) {
		file->ReadInt( numNodes );
	
		nodes.SetNum( numNodes );
		for( int i = 0; i < numNodes; i++ ) {
		    nodes[ i ] = new idBotNode;
			nodes[ i ]->num = i;
		}

		for( int i = 0; i < numNodes; i++ ) {
			idBotNode * node = nodes[ i ];
			file->ReadVec3( node->origin );
			file->ReadString( node->name );
			file->ReadFloat( node->radius );
			file->ReadInt( team );
			node->team = ( playerTeamTypes_t ) team;
			file->ReadBool( node->active );
			file->ReadInt( node->flags );
			file->ReadInt( numLinks );

			node->links.SetNum( numLinks );
			for( int j = 0; j < numLinks; j++ ) {
				idBotNode::botLink_t & link = node->links[ j ];
				file->ReadInt( index );
				link.node = nodes[ index ];
				file->ReadFloat( link.cost );
			}
		}
	} else {
		gameLocal.DWarning("The nodes in this map's nav file are out of date!");
	}

	fileSystem->CloseFile( file );
}

/*
====================
idBotNodeGraph::ActivateNodes
====================
*/
void idBotNodeGraph::ActivateNodes( const char * nodeName, bool activate ) {
	for ( int i = 0; i < nodes.Num(); i++ ) {
		if ( idStr::Icmp( nodes[i]->name, nodeName ) == 0 ) {
			nodes[i]->active = activate;
		}
	}
}

/*
====================
idBotNodeGraph::SetNodeTeam
====================
*/
void idBotNodeGraph::SetNodeTeam( const char * nodeName, const playerTeamTypes_t playerTeam ) {
	for ( int i = 0; i < nodes.Num(); i++ ) {
		if ( idStr::Icmp( nodes[i]->name, nodeName ) == 0 ) {
			nodes[i]->team = playerTeam;
		}
	}
}

/*
====================
idBotNodeGraph::AddNode
====================
*/
idBotNode * idBotNodeGraph::AddNode( const idVec3 & origin, float maxRadius ) {

	idBounds bounds;
	bounds.GetMins().Set( -16.0f, -16.0f, 0.0f );
	bounds.GetMaxs().Set(  16.0f,  16.0f, 1.0f );

	trace_t tr;
	gameLocal.clip.TraceBounds( CLIP_DEBUG_PARMS tr, origin + idVec3( 0.0f, 0.0f, 64.0f ), origin - idVec3( 0.0f, 0.0f, 2048.0f ), bounds, mat3_identity, MASK_PLAYERSOLID | MASK_WATER, gameLocal.GetLocalPlayer() );

	if ( tr.fraction == 1.0f ) {
		tr.endpos = origin; // we should return NULL here, but the code is set up to assume this function never returns NULL
	}

	idBotNode *node = new idBotNode;
	node->num = nodes.Append( node );
	node->origin = tr.endpos + idVec3( 0.0f, 0.0f, 0.5f );	// place 1/2 a unit above the floor
	node->name.Empty();
	node->active = true;
	node->team = NOTEAM;
	node->radius = maxRadius;
	node->flags = ( tr.c.contents & CONTENTS_WATER ) ? NODE_WATER : NODE_GROUND;
	return node;
}

/*
================
idBotNodeGraph::GetNodeDistance
returns the distance from one node to another
================
*/
float idBotNodeGraph::GetNodeDistance( const idBotNode * node1, const idBotNode * node2 ) const {
	assert( node1 && node2 );
	return ( node1->origin - node2->origin ).LengthFast();
}

/*
================
idBotNodeGraph::CreateNodePath
================
*/
void idBotNodeGraph::CreateNodePath( const struct clientInfo_t *botInfo, const idBotNode *start, const idBotNode *end, idList<idBotNode::botLink_t> & botPathList, int vehicleFlags ) const {
    bool foundPath = false;

	int i, j, k, l, m; // n, o, p, q, r, s, t, u, v, w, x, y, z

	enum {
		IS_NULL,
		IS_OPEN,
		IS_CLOSED
	};

	botPathList.Clear();

	if ( start == NULL || end == NULL || start == end ) {
		return;
	}

//mal: max number of nodes we'll search thru.
	int MAX_OPEN = 200;
	int MAX_SEARCH = 200;

	int numOpen = 0;
	int numSearched = 0;

	// (re)initialize all our arrays

	static idList<int> nodeList;
	static idList<float> gCost;
	static idList<float> fCost;
	static idList<int> openList;
	static idList<int> parentList;

	nodeList.SetNum( 0 );
	gCost.SetNum( 0 );
	fCost.SetNum( 0 );
	parentList.SetNum( 0 );

	nodeList.AssureSize( nodes.Num(), IS_NULL );
	gCost.AssureSize( nodes.Num(), -1.0f );
	fCost.AssureSize( nodes.Num(), -1.0f );
	parentList.AssureSize( nodes.Num(), 0 );

	openList.AssureSize( MAX_OPEN, 0 );

	// Add the starting node to the list. Its costs will always be 0
	openList[ 1 ] = start->num; 
	gCost[ start->num ] = 0.0f;
	fCost[ start->num ] = 0.0f;
	numOpen++;

	while( numOpen > 0 && numOpen < MAX_OPEN && numSearched < MAX_SEARCH ) {
		numSearched ++;

		idBotNode * currentNode = nodes[ openList[ 1 ] ];
		nodeList[ openList[ 1 ] ] = IS_CLOSED;
		numOpen--;

		openList[ 1 ] = openList[ numOpen + 1 ];
		l = 1;

		// put the node with the lowest cost at the top of the openList
		while( true ) {
			k = l;

			if ( ( 2 * k + 1 ) < numOpen ) {
				if ( fCost[ openList[ k ] ] >= fCost[ openList[ 2 * k ] ] ) {
					l = 2 * k;
				}

				if ( fCost[ openList[ l ] ] >= fCost[ openList[ 2 * k + 1 ] ] ) {
					l = 2 * k + 1;
				}
			} else {
				if ( ( 2 * k ) < numOpen ) {
					if ( fCost[ openList[ k ] ] >= fCost[ openList[ 2 * k ] ] ) {
						l = 2 * k;
					}
				}
			}

            if ( k != l ) {
				int tempNodeIndex = openList[ k ];
				openList[ k ] = openList[ l ];
				openList[ l ] = tempNodeIndex;
			} else {
				break;
			}
		}

        for( j = 0; j < currentNode->links.Num(); j++ ) {
            
			const idBotNode * newNode = currentNode->links[ j ].node;

			if ( newNode->team != botInfo->team && newNode->team != NOTEAM ) {
				continue;
			}

			if ( !newNode->active ) {
				continue;
			}

			if ( !( newNode->flags & vehicleFlags ) ) {
				continue;
			}

			if ( nodeList[ newNode->num ] == IS_CLOSED ) {
				continue;
			}

			if ( nodeList[ newNode->num ] == IS_OPEN ) {

				// already open, see if we can re-open it with a lower cost

                float gc = gCost[ currentNode->num ] + currentNode->links[ j ].cost;

				if ( gc < gCost[ newNode->num ] ) {
					parentList[ newNode->num ] = currentNode->num;
					gCost[ newNode->num ] = gc;
					fCost[ newNode->num ] = gc + GetNodeDistance( newNode, end );

					for( i = 0; i < numOpen; i++ ) {
						if ( openList[ i ] == newNode->num ) {

							m = i;

							while( m != 1 ) {
								if ( fCost[ openList[ m ] ] < fCost[ openList[ m / 2 ] ] ) {
									int tempNodeIndex = openList[ m / 2 ];
									openList[ m / 2 ] = openList[ m ];
									openList[ m ] = tempNodeIndex;
									m /= 2;
								} else {
                                    break;
								}
							}
							break;
						}
					}
				}
			} else {
				openList[ ++numOpen ] = newNode->num;
				nodeList[ newNode->num ] = IS_OPEN;
				parentList[ newNode->num ] = currentNode->num;

				if ( newNode == end ) {
					break;
				}

				if ( gCost[ newNode->num ] < 0.0f ) {
					if ( currentNode->num == -1 ) {
						gCost[ newNode->num ] = 0.0f;
					} else {
						gCost[ newNode->num ] = gCost[ currentNode->num ] + currentNode->links[ j ].cost;
					}
				}

				fCost[ newNode->num ] = gCost[ newNode->num ] + GetNodeDistance( newNode, end );

				m = numOpen;

				while( m != 1 ) {
					if ( fCost[ openList[ m ] ] <= fCost[ openList[ m / 2 ] ] ) {
						int tempNodeIndex = openList[ m / 2 ];
						openList[ m / 2 ] = openList[ m ];
						openList[ m ] = tempNodeIndex;
						m /= 2;
					} else {
						break;
					}
				}
			}
		}

		if ( nodeList[ end->num ] == IS_OPEN ) {
			foundPath = true;
			break;
		}
	}

	botThreadData.Printf( "Searched: %d\n", numSearched );

	if ( foundPath ) {
		int tempNodeIndex = end->num;
		while( tempNodeIndex != start->num ) {
			idBotNode::botLink_t & link = botPathList.Alloc();
			link.node = nodes[ tempNodeIndex ];
			link.cost = gCost[ tempNodeIndex ];
			tempNodeIndex = parentList[ tempNodeIndex ];
		}
		idBotNode::botLink_t & link = botPathList.Alloc();
		link.node = start;
		link.cost = 0.0f;
	}
}

/*
====================
idBotNodeGraph::GetNearestNode
====================
*/
idBotNode* idBotNodeGraph::GetNearestNode( const idMat3& axis, const idVec3& p, const playerTeamTypes_t team, const moveDirections_t moveDir, bool reachableOnly, bool activeOnly, bool closeNodeOnly, idBotNode* ignoreNode, int vehicleFlags ) {
	idBotNode* nearestNode = NULL;
	float nearestDist = ( closeNodeOnly ) ? 1024.0f : NODE_MAX_RANGE;

	for( int i = 0; i < nodes.Num(); i++ ) {

		idBotNode* node = nodes[ i ];

		if ( node == NULL ) {
			continue;
		}

		if ( node == ignoreNode ) {
			continue;
		}

		if ( team != NOTEAM && node->team != NOTEAM && team != node->team ) {
			continue;
		}

		if ( !( node->flags & vehicleFlags ) && vehicleFlags != 0 ) {
			continue;
		}

		if ( activeOnly ) {
			if ( !node->active ) {
				continue;
			}
		}

		float dist = ( node->origin - p ).ToVec2().LengthFast() - node->radius;

		if ( dist >= nearestDist ) {
			continue;
		}

		if ( reachableOnly ) {
			if ( dist < node->radius ) { //mal: if we're inside a node - our search is over.
				nearestNode = node;
				break;
			}

			if ( node->links.Num() == 0 ) {
				continue;
			}
		}

		if ( axis != mat3_identity && moveDir != NULL_DIR ) {
			idVec3 dir = node->origin - p;
			dir.NormalizeFast();

			if ( moveDir == FORWARD ) {
				if ( dir * axis[ 0 ] < 0.70f ) {
					continue;
				}
			} else if ( moveDir == BACK ) {
				if ( -dir * axis[ 0 ] < 0.50f ) {
					continue;
				}
			} else if ( moveDir == LEFT ) {
				if ( dir * axis[ 1 ] < 0.50f ) {
					continue;
				}
			} else { //mal: RIGHT
				if ( -dir * axis[ 1 ] < 0.50f ) {
					continue;
				}
			}
		}

		if ( reachableOnly ) {
			if ( !botThreadData.Nav_IsDirectPath( AAS_VEHICLE, team, NULL_AREANUM, p, node->origin ) ) {
				continue;
			}
		}

		nearestNode = node;
		nearestDist = dist;
	}

	return nearestNode;
}

/*
================
idBotNodeGraph::DrawNodes

Show the nodes in debug mode.
================
*/
void idBotNodeGraph::DrawNodes() {
	int i, j;
	idVec3 end;
	idMat3 viewAxis;
	idPlayer *player = gameLocal.GetLocalPlayer();

	if ( !botThreadData.AllowDebugData() ) {
		return;
	}

	if ( !bot_drawNodes.GetBool() ) {
		return;
	}

	if ( !player ) {
		return;
	}

	idVec3 playerOrigin = player->GetPhysics()->GetOrigin();

	viewAxis = player->GetViewAxis();

	idBotNode * nearestNode = GetNearestEditNode();

    for( i = 0; i < nodes.Num(); i++ ) { 

		if ( bot_drawNodeNumber.GetInteger() != -1 && bot_drawNodeNumber.GetInteger() != i ) {
			continue;
		}

		idBotNode * node = nodes[ i ];

        if ( node == NULL ) {
			continue;
		}

		if ( bot_drawNodeNumber.GetInteger() == -1 ) {
			if ( ( node->origin - player->GetPhysics()->GetOrigin() ).LengthSqr() > Square( NODE_MAX_RANGE ) ) {
				continue;
			}
		}

		if ( bot_drawNodes.GetBool() ) {

			end = node->origin;
			end.z += 48.0f;
			gameRenderWorld->DebugLine( colorRed, node->origin, end, 16 );
			
			end = node->origin;
			end.z += ( 48.0f + 8.0f );

			gameRenderWorld->DrawText( va( "Node Name: %s     Node #: %i", node->name.c_str(), i ), end, 0.4f, colorWhite, viewAxis );

			if ( node->team != NOTEAM ) {
				end += viewAxis[2] * 8.0f;
				gameRenderWorld->DrawText( node->team == STROGG ? "Strogg" : "GDF", end, 0.4f, colorYellow, viewAxis );
			}

			if ( !node->active ) {
				end += viewAxis[2] * 8.0f;
				gameRenderWorld->DrawText( "**INACTIVE**", end, 0.2f, colorRed, viewAxis );
			}

			end += viewAxis[2] * 8.0f;
			gameRenderWorld->DrawText( node->name.c_str(), end, 0.2f, colorWhite, viewAxis );

			end += viewAxis[2] * 8.0f;
			gameRenderWorld->DrawText( va( "NumLinks: %i", node->links.Num() ), end, 0.2f, colorWhite, viewAxis );

			end += viewAxis[2] * 8.0f;
			gameRenderWorld->DrawText( va( "Flags: %i", node->flags ), end, 0.2f, colorGreen, viewAxis );

			if ( node == nearestNode ) {
				gameRenderWorld->DebugCircle( colorRed, node->origin, idVec3( 0, 0, 1 ), node->radius + 1.0f, 8, 0, false );
				gameRenderWorld->DebugCircle( colorWhite, node->origin, idVec3( 0, 0, 1 ), node->radius + 2.0f, 8, 0, false );
			}
			gameRenderWorld->DebugCircle( colorYellow, node->origin, idVec3( 0, 0, 1 ), node->radius, 8, 0, true );
		}

		for( j = 0; j < node->links.Num(); j++ ) {
			bool bidi = false;
			const idBotNode * other = node->links[ j ].node;
			for ( int k = 0; k < other->links.Num(); k++ ) {
				if ( other->links[ k ].node == node ) {
					bidi = true;
					break;
				}
			}
			idVec3 o1 = node->origin + idVec3( 0.0, 0.0f, 32.0f );
			idVec3 o2 = other->origin + idVec3( 0.0, 0.0f, 32.0f );
			if ( bidi ) {
				gameRenderWorld->DebugLine( colorCyan, o1, o2, 0, true );
			} else {
				gameRenderWorld->DebugArrow( colorLtGrey, o1, o2, 16, 0, true );
			}
		}
	}
}

/*
====================
idBotNodeGraph::ActiveVehicleNodeNearby
====================
*/
bool idBotNodeGraph::ActiveVehicleNodeNearby( const idVec3& p, float range ) {
	bool closeNodeNearby = false;

	for( int i = 0; i < nodes.Num(); i++ ) {

		idBotNode* node = nodes[ i ];

		if ( node == NULL ) {
			continue;
		}

		if ( !node->active ) {
			continue;
		}

		float dist = ( node->origin - p ).ToVec2().LengthFast() - node->radius;

		if ( dist >= range ) {
			continue;
		}

		closeNodeNearby = true;
		break;
	}

	return closeNodeNearby;
}

