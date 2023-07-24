// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../ContentMask.h"
#include "BotThreadData.h"
#include "BotAI_Main.h"
#include "BotAI_VNodes.h"

idBotNode * idBotNodeGraph::lastEditNode = NULL;

/*
================
idBotNodeGraph::GetNearestEditNode
================
*/
idBotNode * idBotNodeGraph::GetNearestEditNode() {
	idVec3 o;
	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( player->GetNoClip() ) {
		// in noclip mode, get the node nearest the cursor
		const idMat3 & viewAxis = player->GetViewAxis();
		const idVec3 & eyePos = player->GetEyePosition();

		trace_t tr;
		gameLocal.TracePoint( tr, eyePos, eyePos + ( viewAxis[0] * ( NODE_MAX_RANGE * 2.0f ) ), MASK_PLAYERSOLID );

		o = tr.endpos;
	} else {
		o = player->GetPhysics()->GetOrigin();
	}
	idBotNode * node = botThreadData.botVehicleNodes.GetNearestNode( mat3_identity, o, NOTEAM );
	idBotNode * vnode = botThreadData.botVehicleNodes.GetNearestNode( mat3_identity, o, NOTEAM );

	if ( botThreadData.AllowDebugData() ) {
		int drawNodes = cvarSystem->GetCVarInteger( "bot_drawNodes" );
		if ( drawNodes == 2 ) {
			return vnode;
		} else if ( drawNodes == 3 ) {
			return node;
		}
	}

	if ( node == NULL || ( vnode != NULL && ( vnode->origin - o ).Length() < ( node->origin - o ).Length() ) ) {
		return vnode;
	} else {
		return node;
	}
}

/*
================
idBotNodeGraph::Cmd_NodeAdd_f
================
*/
void idBotNodeGraph::Cmd_NodeAdd_f ( const idCmdArgs &args ) {
	idVec3 newOrigin;

	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( player->GetNoClip() ) {
		// in no clip mode, we trace a line out and drop the node at that spot
		static const float MIN_WALK_NORMAL = 0.7f; // from physics_player.cpp
		const idMat3 & viewAxis = player->GetViewAxis();
		const idVec3 & eyePos = player->GetEyePosition();
		trace_t tr;
		gameLocal.TracePoint( tr, eyePos, eyePos + ( viewAxis[0] * NODE_MAX_RANGE ), MASK_PLAYERSOLID | MASK_WATER );
		if ( tr.fraction >= 1.0f ) {
			common->Printf( "Could not trace to solid!" );
			return;
		} else if ( ( tr.c.normal * -gameLocal.GetGravity() ) < MIN_WALK_NORMAL ) {
			common->Printf( "Trace hit non-floor!" );
			return;
		} else {
			newOrigin = tr.endpos;
		}
	} else {
		// in non no clip mode, we use the player origin
		newOrigin = player->GetPhysics()->GetOrigin();
	}

	botThreadData.botVehicleNodes.AddNode( newOrigin, 256.0f );
}

/*
================
idBotNodeGraph::Cmd_NodeDel_f
================
*/
void idBotNodeGraph::Cmd_NodeDel_f( const idCmdArgs &args ) {
	idBotNode * node = GetNearestEditNode();
	if ( node == NULL ) {
		common->Printf( "oh no, no nodes!\n" );
		return;
	}

	if ( lastEditNode == node ) {
		lastEditNode = NULL;
	}

	botThreadData.botVehicleNodes.DeleteNode( node );
}
	
/*
================
idBotNodeGraph::Cmd_NodeName_f
================
*/
void idBotNodeGraph::Cmd_NodeName_f( const idCmdArgs &args ) {
	idBotNode * node = GetNearestEditNode();
	if ( node == NULL ) {
		common->Printf( "oh no, no nodes!\n" );
		return;
	}
	node->name = args.Argv( 1 );
}

/*
================
idBotNodeGraph::Cmd_NodeTeam_f
================
*/
void idBotNodeGraph::Cmd_NodeTeam_f( const idCmdArgs &args ) {
	idBotNode * node = GetNearestEditNode();
	if ( node == NULL ) {
		common->Printf( "oh no, no nodes!\n" );
		return;
	}
	if ( args.Argc() != 2 || idStr::Icmp( args.Argv( 1 ), "none" ) == 0 || idStr::Icmp( args.Argv( 1 ), "noteam" ) == 0 ) {
		node->team = NOTEAM;
	} else if ( idStr::Icmp( args.Argv( 1 ), "strogg" ) == 0 ) {
		node->team = STROGG;
	} else if ( idStr::Icmp( args.Argv( 1 ), "gdf" ) == 0 ) {
		node->team = GDF;
	} else {
		common->Printf( "Unknown team: %s\n", args.Argv( 1 ) );
	}
}

/*
================
idBotNodeGraph::Cmd_NodeRadius_f
================
*/
void idBotNodeGraph::Cmd_NodeRadius_f( const idCmdArgs &args ) {
	if ( args.Argc() == 1 ) {
		common->Printf( "syntax: nodeRadius <radius>\n" );
		return;
	}
	idBotNode * node = GetNearestEditNode();
	if ( node == NULL ) {
		common->Printf( "oh no, no nodes!\n" );
		return;
	}
	const char * value = args.Args( 1 );
	if ( value[0] == '*' ) {
		node->radius *= atof( value+1 );
	} else if ( value[0] == '/' ) {
		node->radius /= atof( value+1 );
	} else if ( value[0] == '+' ) {
		node->radius += atof( value+1 );
	} else if ( value[0] == '-' ) {
		node->radius -= atof( value+1 );
	} else {
		node->radius = atof( value );
	}
	node->radius = Min( node->radius, 300.0f );
	node->radius = Max( node->radius, 5.0f );
}

/*
================
idBotNodeGraph::Cmd_SaveNodes_f

Saves the bot nodes out to < mapname >.nav
================
*/
void idBotNodeGraph::Cmd_SaveNodes_f( const idCmdArgs &args ) {
	idStr navMap = gameLocal.GetMapName();
	navMap.SetFileExtension("nav");
	botThreadData.botVehicleNodes.SaveNodes( navMap );
}

/*
================
idBotNodeGraph::Cmd_NodeActive_f

sets a node to be active or inactive
================
*/
void idBotNodeGraph::Cmd_NodeActive_f( const idCmdArgs &args ) {
	idBotNode * node = GetNearestEditNode();
	if ( node == NULL ) {
		return;
	}
	node->active = !node->active;
}

/*
================
idBotNodeGraph::Cmd_NodeFlags_f

sets a node's flags
================
*/
void idBotNodeGraph::Cmd_NodeFlags_f( const idCmdArgs &args ) {
	idBotNode * node = GetNearestEditNode();
	if ( node == NULL ) {
		return;
	}

	if ( args.Argc() < 2 || idStr::Icmp( args.Argv( 1 ), "ground" ) == 0 ) {
		node->flags = NODE_GROUND;
	} else if ( idStr::Icmp( args.Argv( 1 ), "water" ) == 0 ) {
		node->flags = NODE_WATER;
	} else if ( idStr::Icmp( args.Argv( 1 ), "husky" ) == 0 ) {
		node->flags = NODE_HUSKY_ONLY;
	} else if ( idStr::Icmp( args.Argv( 1 ), "custom" ) == 0 ) {
		int flags = atoi( args.Argv( 2 ) );
		node->flags = flags;
	} else {
		common->Printf( "Unknown flag: %s\n", args.Argv( 1 ) );
	}
}

/*
================
idBotNodeGraph::Cmd_NodeView_f
================
*/
void idBotNodeGraph::Cmd_NodeView_f( const idCmdArgs &args ) {
	idBotNode * node = botThreadData.botVehicleNodes.nodes[ atoi( args.Argv( 1 ) ) ];
	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, va( "setviewpos %f %f %f", node->origin.x, node->origin.y, node->origin.z ) );
}


/*
================
idBotNodeGraph::Cmd_CreateLink_f
creates a link from the last node to this one
last node being the last node added or the last node we linked to
================
*/
void idBotNodeGraph::Cmd_CreateLink_f( const idCmdArgs &args ) {
	if ( idStr::Icmp( args.Argv( 1 ), "start" ) == 0 || idStr::Icmp( args.Argv( 1 ), "clear" ) == 0) {
		common->Printf( "Cleared last node\n" );
		lastEditNode = NULL;
		return;
	}
	if ( lastEditNode == NULL ) {
		common->Printf( "Set last node\n" );
		lastEditNode = GetNearestEditNode();
		return;
	}
	if ( botThreadData.botVehicleNodes.nodes.FindIndex( lastEditNode ) == -1 ) {
		common->Printf( S_COLOR_RED " OH NO! last node is invalid!?\n" );
		lastEditNode = NULL;
		return;
	}
	idBotNode * node = GetNearestEditNode();
	if ( node == NULL ) {
		return;
	}
	if ( node != lastEditNode ) {
		lastEditNode->AddLink( node );
		if ( idStr::Icmp( args.Argv( 1 ), "oneway" ) != 0 ) {
			node->AddLink( lastEditNode );
		}
		lastEditNode = node;
	}
}

/*
================
idBotNodeGraph::Cmd_GenerateFromBotActions_f
generates nodes from vehicle camp/roam bot actions
================
*/
void idBotNodeGraph::Cmd_GenerateFromBotActions_f( const idCmdArgs &args ) {

	for ( int i = 0; i < botThreadData.botActions.Num(); i++ ) {
		idBotActions * action = botThreadData.botActions[ i ];
		if ( !action->ActionIsValid() ) {
			continue;
		}

		if ( action->GetHumanObj() != ACTION_VEHICLE_CAMP && action->GetHumanObj() != ACTION_VEHICLE_ROAM && action->GetStroggObj() != ACTION_VEHICLE_CAMP && action->GetStroggObj() != ACTION_VEHICLE_ROAM ) {
			continue;
		}

		bool alreadyExists = false;

		for ( int j = 0; j < botThreadData.botVehicleNodes.nodes.Num(); j++ ) {
			idBotNode * node = botThreadData.botVehicleNodes.nodes[ j ];
			if ( node == NULL ) {
				continue;
			}
			if ( ( node->origin - action->GetActionOrigin() ).LengthSqr() < 1.0f ) {
				alreadyExists = true;
				break;
			}
		}
		if ( alreadyExists ) {
			continue;
		}

		idBotNode *node = new idBotNode;
		node->num = botThreadData.botVehicleNodes.nodes.Append( node );
		node->origin = action->GetActionOrigin();
		node->name.Empty();

		if ( action->GetHumanObj() != ACTION_NULL && action->GetStroggObj() != ACTION_NULL ) {
			node->team = NOTEAM;
		} else if ( action->GetHumanObj() != ACTION_NULL ) {
			node->team = GDF;
		} else {
			node->team = STROGG;
		}
		node->radius = action->GetRadius();

		gameLocal.DPrintf( "Created node for action %s\n", action->GetActionName() );
	}
}
