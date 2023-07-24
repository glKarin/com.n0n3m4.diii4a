// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "../Game_local.h" 
#include "../ContentMask.h"
#include "BotThreadData.h"
#include "BotAI_Main.h"

/*
================
idBotNode::idBotNode
================
*/
idBotNode::idBotNode() {
	active = true;
	flags = NODE_GROUND;
	radius = 256.0f;
	team = NOTEAM;
	origin.Zero();
	name = "";
	links.Clear();
}

/*
===================
idBotNode::RemoveLink
===================
*/
void idBotNode::RemoveLink( const idBotNode * node ) {
	for ( int i = 0; i < links.Num(); i++ ) {
		if ( links[i].node == node ) {
			links.RemoveIndexFast( i );
		}
	}
	//assert( !"Could not find link" );
}

/*
===================
idBotNode::AddLink
===================
*/
void idBotNode::AddLink( const idBotNode * node ) {
	RemoveLink( node ); // cause I'm paranoid
	botLink_t & link = links.Alloc();
	link.node = node;
	link.cost = ( node->origin - origin ).Length();
	link.cost += idMath::Fabs( node->origin.z - origin.z );
}
