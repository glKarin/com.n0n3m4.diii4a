#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef MA_CheckReactions( "<checkreactions>", "e" );

CLASS_DECLARATION( hhMonsterAI, hhAIInspector )
	EVENT( EV_PostSpawn,			hhAIInspector::Event_PostSpawn )
	EVENT( MA_CheckReactions,		hhAIInspector::Event_CheckReactions )
END_CLASS

void hhAIInspector::Spawn() {
}

void hhAIInspector::Event_PostSpawn() {
	const function_t* func = GetScriptFunction( "state_InspectorIdle" );
	if( !func ) {
		gameLocal.Warning( "unable to find state_InspectorIdle" );
		return;
	}
	SetState( func );
}

void hhAIInspector::CheckReactions( idEntity* entity ) {
	PostEventMS( &MA_CheckReactions, 1000, entity );
}

void hhAIInspector::Event_CheckReactions( idEntity* entity ) {
	checkReaction = entity;
	targetReaction.entity = entity;
	targetReaction.reactionIndex = 0;
	if( !targetReaction.GetReaction() ) {
		targetReaction.reactionIndex = -1;
		gameLocal.Warning( "unable to properly use requested reaction... possibly no reactions on this entity" );
		return;
	}

	const function_t* func = GetScriptFunction( "state_InspectorReactions" );
	if( !func ) {
		gameLocal.Warning( "unable to find checkreaction state on inspector..." );
		return;
	}
	SetState( func );
}

void hhAIInspector::RestartInspector( const char* inspectClass, idEntity* newReaction, idPlayer* starter ) {
	float yaw;
	idVec3 org;
	idDict spawnDict;
	idEntity* react_scout;
	idEntity* use_reaction = NULL;

//Find our dictionary to use
	const idDict* edict = gameLocal.FindEntityDefDict( inspectClass, false );
	if( !edict ) {
		gameLocal.Printf( "Unable to find entitfydef '%s'", inspectClass );
		return;
	}
	spawnDict = *edict;		//copy the values to an editable dict

//Setup values for position/orientation
	yaw = starter->viewAngles.yaw;
	org = starter->GetPhysics()->GetOrigin() + idAngles( 0, yaw, 0 ).ToForward() * 180 + idVec3( 0, 0, 1 );

// Set dictionary values...
	spawnDict.Set( "angle", va("%f", yaw + 180) );
	spawnDict.Set( "origin", org.ToString() );
	spawnDict.Set( "spawnClass", "hhAIInspector" );

// Perform actual spawn
	react_scout = static_cast<hhAIInspector*>(gameLocal.SpawnEntityType(hhAIInspector::Type, &spawnDict));
	if( !react_scout ) {
		gameLocal.Printf( "failed to spawn inspector monster..." );
		return;
	}

	use_reaction = newReaction;

	if( gameLocal.inspector != NULL ) {
//save our old use_reaction if we haven't specified one
		if( !use_reaction && gameLocal.inspector.IsValid() && gameLocal.inspector->checkReaction.IsValid() ) {
			use_reaction = gameLocal.inspector->checkReaction.GetEntity();
		}
// Remove our old inspector
		gameLocal.inspector->PostEventMS( &EV_Remove, 0 );
	}
	gameLocal.inspector = react_scout;
	if( use_reaction ) {
		gameLocal.inspector->CheckReactions( use_reaction );
	}
}

void hhAIInspector::RestartInspector( idEntity* newReaction, idPlayer* starter ) {
	idEntity* react_scout;
	float yaw;
	idVec3 org;

	yaw = starter->viewAngles.yaw;
	org = starter->GetPhysics()->GetOrigin() + idAngles( 0, yaw, 0 ).ToForward() * 180 + idVec3( 0, 0, 1 );

	idDict dict = gameLocal.inspector.GetEntity()->spawnArgs;

	dict.Set( "angle", va("%f", yaw + 180) );
	dict.Set( "origin", org.ToString() );
	react_scout = static_cast<hhAIInspector*>(gameLocal.SpawnEntityType(hhAIInspector::Type, &dict));
	if( !react_scout ) {
		gameLocal.Printf( "failed to spawn inspector monster..." );
		return;
	}

	if( gameLocal.inspector != NULL ) {
		gameLocal.inspector->PostEventMS( &EV_Remove, 0 );
	}
	gameLocal.inspector = react_scout;
	gameLocal.inspector->CheckReactions( newReaction );
}

/*
=====================
hhAIInspector::Save
=====================
*/
void hhAIInspector::Save( idSaveGame *savefile ) const {
	checkReaction.Save( savefile );
}

/*
=====================
hhAIInspector::Restore
=====================
*/
void hhAIInspector::Restore( idRestoreGame *savefile ) {
	checkReaction.Restore( savefile );
}

