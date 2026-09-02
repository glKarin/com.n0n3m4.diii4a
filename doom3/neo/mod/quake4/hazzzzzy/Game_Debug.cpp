//----------------------------------------------------------------
// Game_Debug.cpp
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

rvGameDebug	gameDebug;

/*
===============================================================================

	rvGameDebug

===============================================================================
*/

/*
================
rvGameDebug::rvGameDebug
================
*/
rvGameDebug::rvGameDebug ( void ) {
}

/*
================
rvGameDebug::Init
================
*/
void rvGameDebug::Init ( void ) {
	focusEntity		= NULL;
	overrideEntity	= NULL;
	currentHud		= NULL;
	memset ( &hud, 0, sizeof(hud) );

	jumpIndex = -1;
	jumpPoints.Clear ( );
}

/*
================
rvGameDebug::Shutdown
================
*/
void rvGameDebug::Shutdown ( void ) {
	nonGameState.Clear ( );
	gameStats.Clear ( );

	currentHud		= NULL;
	focusEntity		= NULL;
	overrideEntity	= NULL;

	memset ( &hud, 0, sizeof(hud) );
}	

/*
================
rvGameDebug::Think
================
*/
void rvGameDebug::BeginFrame ( void ) {
	int hudIndex;

	inFrame = true;

	hudIndex = g_showDebugHud.GetInteger();
	if ( hudIndex <= 0 ) {
		focusEntity = NULL;
		currentHud	 = NULL;
		return;
	}

	// Update the current debug hud if the cvar has changed
	if ( g_showDebugHud.IsModified() || !currentHud ) {
		if ( hudIndex > DBGHUD_MAX ) {
			g_showDebugHud.SetInteger( 0 );
			focusEntity = NULL;
			currentHud  = NULL;
			return;
		}
	
		g_showDebugHud.ClearModified( );

		// If the debug hud hasnt been loaded yet then load it now
		if ( !hud[hudIndex] ) {
			hud[hudIndex] = uiManager->FindGui( va("guis/debug/hud%d.gui",hudIndex), true, true, true );
			
			// If the hud wasnt found auto-generate one
			if ( !hud[hudIndex] ) {
				hud[hudIndex] = uiManager->Alloc();
			}
		}

		// Cache the debug hud state.
		currentHud = hud[hudIndex];	
	}
	
	currentHud->ClearState ( );
		
	// IF there is an override entity just use that, otherwise find one that
	// is in front of the players crosshair	
	if ( overrideEntity ) {
		focusEntity = overrideEntity;
		overrideEntity = NULL;
	} else {
		idPlayer*	player;
		idVec3		start;
		idVec3		end;
		trace_t		tr;

		player = gameLocal.GetLocalPlayer ( );
		start  = player->GetEyePosition();
		end    = start + player->viewAngles.ToForward() * 4096.0f;
  			
		gameLocal.TracePoint( player, tr, start, end, MASK_SHOT_BOUNDINGBOX, player );
  		if ( tr.fraction < 1.0 && tr.c.entityNum != ENTITYNUM_WORLD ) {
  			focusEntity = static_cast<idEntity*>(gameLocal.entities[ tr.c.entityNum ]);		
  		} else {
  			focusEntity = NULL;
  		}
	}

	// Automatically add some basic entity information
	if ( focusEntity )	{
		SetInt ( "entityNumber", focusEntity->entityNumber );
		SetInt ( "entityHealth", focusEntity->health );
		SetString ( "entityName", focusEntity->name );
		SetString ( "entityClass", focusEntity->GetClassname ( ) );
	}

	// General map information	
	SetString ( "mapname", gameLocal.GetMapName ( ) );
	SetString ( "version", cvarSystem->GetCVarString ( "si_version" ) );
	if ( gameLocal.GetLocalPlayer() ) {
		SetString ( "viewpos", gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin().ToString() );
	}
}

/*
================
rvGameDebug::EndFrame
================
*/
void rvGameDebug::EndFrame ( void ) {
	inFrame = false;
}

/*
================
rvGameDebug::DrawHud
================
*/
void rvGameDebug::DrawHud ( void ) {
	if ( !currentHud ) {
		return;
	}

	// The scratch hud displays key value pairs in a list so
	// we need to push the keys into the list
	if ( IsHudActive ( DBGHUD_SCRATCH ) ) {
		int		index;
		idDict	tempState;
		
		tempState.Copy ( currentHud->State() );
		currentHud->ClearState ( );
		
		for ( index = 0; index < tempState.GetNumKeyVals(); index ++ ) {
			const idKeyValue* kv;
		
			kv = tempState.GetKeyVal ( index );
			SetString ( va("scratchKey_item_%d", index ), kv->GetKey() );
			SetString ( va("scratchValue_item_%d", index ), kv->GetValue() );
		}
	}
	else {
		int index;
		for ( index = 0; index < nonGameState.GetNumKeyVals(); index ++ ) {
			const idKeyValue* kv;		
			kv = nonGameState.GetKeyVal ( index );
			currentHud->SetStateString ( kv->GetKey(), kv->GetValue() );			
		}
	}	
	
	// Activate the hud to ensure lists get updated and redraw it
	currentHud->StateChanged ( gameLocal.time );
	currentHud->Redraw( gameLocal.time );			
}

/*
================
rvGameDebug::AppendList
================
*/
void rvGameDebug::AppendList ( const char* listname, const char* value ) {
	if ( !currentHud ) {
		return;
	}
	
	int		count;
	char	countName[1024];
	char	itemName[1024];

	idStr::snPrintf ( countName, 1023, "%sCount", listname );
	count = GetInt ( countName );

	idStr::snPrintf ( itemName, 1023, "%s_item_%d", listname, count );	
	SetString ( va("%s_item_%d", listname, count ), value );
	
	SetInt ( countName, count + 1 );	
}

/*
================
rvGameDebug::Set
================
*/
void rvGameDebug::SetInt ( const char* key, int value ) {
	if ( inFrame ) {
		if ( currentHud ) {
			currentHud->SetStateInt( key, value );
		}
	} else  {
		nonGameState.SetInt ( key, value );
	}
}

void rvGameDebug::SetFloat ( const char* key, float value ) {
	if ( inFrame ) {
		if ( currentHud ) {
			currentHud->SetStateFloat( key, value );
		}
	} else {
		nonGameState.SetFloat ( key, value );
	}	
}

void rvGameDebug::SetString ( const char* key, const char* value ) {
	if ( inFrame )	{
		if ( currentHud ) {
			currentHud->SetStateString( key, value );
		}
	} else {
		nonGameState.Set ( key, value );
	}
}

/*
================
rvGameDebug::Get
================
*/
int rvGameDebug::GetInt ( const char* key ) { 
	return currentHud ? currentHud->State().GetInt ( key ) : 0;
}

float rvGameDebug::GetFloat ( const char* key ) {
	return currentHud ? currentHud->State().GetFloat ( key ) : 0.0f;
}

const char* rvGameDebug::GetString ( const char* key ) {
	return currentHud ? currentHud->State().GetString  ( key ) : "";
}

/*
================
rvGameDebug::SetStat
================
*/
void rvGameDebug::SetStatInt ( const char* key, int value ) {
	gameStats.SetInt ( key, value );
}

void rvGameDebug::SetStatFloat ( const char* key, float value ) {
	gameStats.SetFloat ( key, value );
}

void rvGameDebug::SetStatString ( const char* key, const char* value ) {
	gameStats.Set ( key, value );
}

/*
================
rvGameDebug::GetStat
================
*/
int rvGameDebug::GetStatInt ( const char* key ) { 
	return gameStats.GetInt ( key );
}

float rvGameDebug::GetStatFloat ( const char* key ) {
	return gameStats.GetFloat ( key );
}

const char* rvGameDebug::GetStatString ( const char* key ) {
	return gameStats.GetString ( key );
}


/*
================
rvGameDebug::JumpAdd
================
*/
void rvGameDebug::JumpAdd ( const char* name, const idVec3& origin, const idAngles& angles ) {
	debugJumpPoint_t jump;
	jump.name = name;
	jump.origin = origin;
	jump.angles = angles;
	jumpPoints.Append ( jump );
}

/*
================
rvGameDebug::JumpTo
================
*/
void rvGameDebug::JumpTo ( const char* name ) {
	int index;
	for ( index = 0; index < jumpPoints.Num(); index ++ ) {
		if ( !jumpPoints[index].name.Icmp ( name ) ) {
			JumpTo ( index );
			return;
		}
	}
}

/*
================
rvGameDebug::JumpTo
================
*/
void rvGameDebug::JumpTo ( int jumpIndex ) {
	if ( jumpIndex >= jumpPoints.Num() ) { 
		return;
	}

	jumpIndex = jumpIndex;

	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player ) {
		player->Teleport( jumpPoints[jumpIndex].origin, jumpPoints[jumpIndex].angles, NULL );
	}	
}

/*
================
rvGameDebug::JumpNext
================
*/
void rvGameDebug::JumpNext ( void ) {
	if ( !jumpPoints.Num() ) {
		return;
	}
	JumpTo ( ( jumpIndex + 1 ) % jumpPoints.Num() );
}

/*
================
rvGameDebug::JumpPrev
================
*/
void rvGameDebug::JumpPrev ( void ) {
	if ( !jumpPoints.Num() ) {
		return;
	}
	JumpTo ( ( jumpIndex + jumpPoints.Num() - 1 ) % jumpPoints.Num() );
}
