// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Mover.h"
#include "script/Script_Helper.h"
#include "ContentMask.h"
#include "structures/TeamManager.h"
#include "Player.h"
#include "WorldSpawn.h"

/*
===============================================================================

	sdPortalState

===============================================================================
*/

/*
============
sdPortalState::sdPortalState
============
*/
sdPortalState::sdPortalState( void ) {
	areaPortal	= 0;
	open		= true;
}

/*
============
sdPortalState::~sdPortalState
============
*/
sdPortalState::~sdPortalState( void ) {
	Open();
}

/*
============
sdPortalState::Init
============
*/
void sdPortalState::Init( const idBounds& bounds ) {
	areaPortal = gameRenderWorld->FindPortal( bounds );
	Open();
}

/*
============
sdPortalState::Open
============
*/
void sdPortalState::Open( void ) {
	if ( areaPortal == 0 ) {
		return;
	}

	open = true;
	gameRenderWorld->SetPortalState( areaPortal, PS_BLOCK_NONE );
}

/*
============
sdPortalState::Close
============
*/
void sdPortalState::Close( void ) {
	if ( areaPortal == 0 ) {
		return;
	}

	open = false;
	gameRenderWorld->SetPortalState( areaPortal, PS_BLOCK_ALL );
}

/*
===============================================================================

	idSplinePath, holds a spline path to be used by an idMover

===============================================================================
*/

CLASS_DECLARATION( idEntity, idSplinePath )
END_CLASS

/*
================
idSplinePath::idSplinePath
================
*/
idSplinePath::idSplinePath() {
}

/*
================
idSplinePath::Spawn
================
*/
void idSplinePath::Spawn( void ) {
}

/*
===============================================================================

idMover_Binary

Doors, plats, and buttons are all binary (two position) movers
Pos1 is "at rest", pos2 is "activated"

===============================================================================
*/

extern const idEventDef EV_SetToggle;
extern const idEventDef EV_Disable;
extern const idEventDef EV_Enable;

const idEventDefInternal EV_MatchTeam( "internal_matchteam", "dd" );
const idEventDef EV_Enable( "enable", '\0', DOC_TEXT( "Enables an entity, allowing it to be used by the player." ), 0, "See also $event:disable$." );
const idEventDef EV_Disable( "disable", '\0', DOC_TEXT( "Disables an entity, making it unusable by the player." ), 0, "See also $event:enable$." );
const idEventDef EV_GetMoveMaster( "getMoveMaster", 'e', DOC_TEXT( "Returns the master of the mover team." ), 0, NULL );
const idEventDef EV_GetNextSlave( "getNextSlave", 'e', DOC_TEXT( "Returns the next entity in the mover team." ), 0, NULL );
const idEventDef EV_SetToggle( "setToggle", '\0', DOC_TEXT( "Sets the toggle state for the entity. If toggle mode is enabled, the entity will toggle between the two states, rather than returning to the initial state after a period of time." ), 1, NULL, "b", "toggle", "Set toggle on or off" );

CLASS_DECLARATION( sdScriptEntity, idMover_Binary )
	EVENT( EV_Activate,					idMover_Binary::Event_Use_BinaryMover )
	EVENT( EV_MatchTeam,				idMover_Binary::Event_MatchActivateTeam )
	EVENT( EV_Enable,					idMover_Binary::Event_Enable )
	EVENT( EV_Disable,					idMover_Binary::Event_Disable )
	EVENT( EV_GetMoveMaster,			idMover_Binary::Event_GetMoveMaster )
	EVENT( EV_GetNextSlave,				idMover_Binary::Event_GetNextSlave )
	EVENT( EV_SetToggle,				idMover_Binary::Event_SetToggle )
END_CLASS

/*
================
sdMoverBinaryBroadcastState::MakeDefault
================
*/
void sdMoverBinaryBroadcastState::MakeDefault( void ) {
	sdScriptEntityBroadcastData::MakeDefault();

	closeTime	= 0;
	hidden		= false;
	toggle		= false;
}

/*
================
sdMoverBinaryBroadcastState::Write
================
*/
void sdMoverBinaryBroadcastState::Write( idFile* file ) const {
	sdScriptEntityBroadcastData::Write( file );

	file->WriteInt( closeTime );
	file->WriteBool( hidden );
	file->WriteBool( toggle );
}

/*
================
sdMoverBinaryBroadcastState::Read
================
*/
void sdMoverBinaryBroadcastState::Read( idFile* file ) {
	sdScriptEntityBroadcastData::Read( file );

	file->ReadInt( closeTime );
	file->ReadBool( hidden );
	file->ReadBool( toggle );
}


/*
================
idMover_Binary::idMover_Binary()
================
*/
idMover_Binary::idMover_Binary( void ) {
	pos1.Zero();
	pos2.Zero();
	moverState = MOVER_POS1;
	moveMaster = NULL;
	activateChain = NULL;
	soundPos1 = 0;
	sound1to2 = 0;
	sound2to1 = 0;
	soundPos2 = 0;
	soundLoop = 0;
	wait = 0.0f;
	damage = 0.0f;
	duration = 0;
	accelTime = 0;
	decelTime = 0;
	activatedBy = this;
	enabled = false;
	toggle = false;
}

/*
================
idMover_Binary::~idMover_Binary
================
*/
idMover_Binary::~idMover_Binary() {
	idMover_Binary* mover;

	// if this is the mover master
	if ( this == moveMaster ) {
		// make the next mover in the chain the move master
		for ( mover = moveMaster; mover; mover = mover->activateChain ) {
			mover->moveMaster = this->activateChain;
		}
	} else {
		// remove mover from the activate chain
		for ( mover = moveMaster; mover; mover = mover->activateChain ) {
			if ( mover->activateChain == this ) {
				mover->activateChain = this->activateChain;
				break;
			}
		}
	}
}

/*
================
idMover_Binary::Damage
================
*/
void idMover_Binary::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const sdDeclDamage* damageDecl, const float damageScale, trace_t* collision, bool forceKill ) {
	if ( moveMaster != this ) {
		moveMaster->Damage( inflictor, attacker, dir, damageDecl, damageScale, collision, forceKill );
	} else {
		sdScriptEntity::Damage( inflictor, attacker, dir, damageDecl, damageScale, collision, forceKill );
	}
}

/*
================
idMover_Binary::Think
================
*/
void idMover_Binary::Think( void ) {
	sdScriptEntity::Think();

	if ( closeTime != -1 ) {
		if ( gameLocal.time > closeTime ) {
			MatchActivateTeam( MOVER_2TO1, gameLocal.time );
		}
	}
}

/*
================
idMover_Binary::Spawn

Base class for all movers.

"wait"		wait before returning (3 default, -1 = never return)
"speed"		movement speed
================
*/
void idMover_Binary::Spawn( void ) {
	enabled			= true;

	closeTime		= -1;

	activateChain = NULL;

	spawnArgs.GetFloat( "wait", "0", wait );

	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );
	physicsObj.SetAxis( GetPhysics()->GetAxis() );
	physicsObj.SetClipMask( MASK_SOLID | CONTENTS_BODY | CONTENTS_SLIDEMOVER );
	if ( !spawnArgs.GetBool( "solid", "1" ) ) {
		physicsObj.SetContents( 0 );
	}
	if ( !spawnArgs.GetBool( "nopush" ) ) {
		physicsObj.SetPusher( 0 );
	}
	toggle = spawnArgs.GetBool( "toggle" );
	physicsObj.SetLinearExtrapolation( EXTRAPOLATION_NONE, 0, 0, GetPhysics()->GetOrigin(), vec3_origin, vec3_origin );
//	physicsObj.SetAngularExtrapolation( EXTRAPOLATION_NONE, 0, 0, GetPhysics()->GetAxis().ToAngles(), ang_zero, ang_zero );
	SetPhysics( &physicsObj );
}

/*
================
idMover_Binary::Event_GetMoveMaster
================
*/
void idMover_Binary::Event_GetMoveMaster( void ) {
	sdProgram::ReturnEntity( moveMaster );
}

/*
================
idMover_Binary::Event_GetNextSlave
================
*/
void idMover_Binary::Event_GetNextSlave( void ) {
	sdProgram::ReturnEntity( activateChain );
}

/*
================
idMover_Binary::Event_GotoPosition1
================
*/
void idMover_Binary::Event_GotoPosition1( void ) {
	GotoPosition1();
}

/*
================
idMover_Binary::Event_GotoPosition2
================
*/
void idMover_Binary::Event_GotoPosition2( void ) {
	GotoPosition2();
}

/*
================
idMover_Binary::Event_SetToggle
================
*/
void idMover_Binary::Event_SetToggle( bool toggle ) {
	this->toggle = toggle;
}

/*
================
idMover_Binary::PostMapSpawn
================
*/
void idMover_Binary::PostMapSpawn( void ) {
	sdScriptEntity::PostMapSpawn();

	bool groupMaster = spawnArgs.GetBool( "groupmaster" );

	moveMaster = NULL;
	if ( !groupMaster ) {
		const char* groupName = spawnArgs.GetString( "group" );
		if ( *groupName ) {
			sdInstanceCollector< idMover_Binary > instances( true );

			for ( int i = 0; i < instances.Num(); i++ ) {
				idMover_Binary* mover = instances[ i ];
				const char* otherGroupName = mover->spawnArgs.GetString( "group" );
				if ( !idStr::Icmp( groupName, otherGroupName ) && mover->spawnArgs.GetBool( "groupmaster" ) ) {
					moveMaster = mover;
					break;
				}
			}
		}
	}

	// create a physics team for the binary mover parts
	if ( moveMaster ) {
		JoinTeam( moveMaster );
		JoinActivateTeam( moveMaster );

		idBounds soundOrigin;

		soundOrigin.Clear();

		idMover_Binary* slave;
		for ( slave = moveMaster; slave; slave = slave->activateChain ) {
			soundOrigin += slave->GetPhysics()->GetAbsBounds();			
		}
		moveMaster->refSound.origin = soundOrigin.GetCenter();
	} else {
		moveMaster = this;
		OnMasterReady();
	}
}

/*
===============
idMover_Binary::GetMovedir

The editor only specifies a single value for angles (yaw),
but we have special constants to generate an up or down direction.
Angles will be cleared, because it is being used to represent a direction
instead of an orientation.
===============
*/
void idMover_Binary::GetMovedir( float angle, idVec3 &movedir ) {
	if ( angle == -1 ) {
		movedir.Set( 0, 0, 1 );
	} else if ( angle == -2 ) {
		movedir.Set( 0, 0, -1 );
	} else {
		movedir = idAngles( 0, angle, 0 ).ToForward();
	}
}

/*
===============
idMover_Binary::UpdateMoverSound
===============
*/
void idMover_Binary::UpdateMoverSound( moverState_t state ) {
	if ( moveMaster == this ) {
		switch( state ) {
			case MOVER_POS1:
				break;
			case MOVER_POS2:
				break;
			case MOVER_1TO2:
				StartSound( "snd_open", SND_DOOR, 0, NULL );
				break;
			case MOVER_2TO1:
				StartSound( "snd_close", SND_DOOR, 0, NULL );
				break;
		}
	}
}

/*
===============
idMover_Binary::SetMoverState
===============
*/
void idMover_Binary::SetMoverState( moverState_t newstate, int time ) {
	idVec3 	delta;

	moverState = newstate;

	UpdateMoverSound( newstate );

	switch( moverState ) {
		case MOVER_POS1: {
			physicsObj.SetLinearExtrapolation( EXTRAPOLATION_NONE, time, 0, pos1, vec3_origin, vec3_origin );
			break;
		}
		case MOVER_POS2: {
			physicsObj.SetLinearExtrapolation( EXTRAPOLATION_NONE, time, 0, pos2, vec3_origin, vec3_origin );
			break;
		}
		case MOVER_1TO2: {
			physicsObj.SetLinearExtrapolation( EXTRAPOLATION_LINEAR, time, duration, pos1, ( pos2 - pos1 ) * 1000.0f / duration, vec3_origin );
//			if ( accelTime != 0 || decelTime != 0 ) {
//				physicsObj.SetLinearInterpolation( time, accelTime, decelTime, duration, pos1, pos2 );
//			} else {
//				physicsObj.SetLinearInterpolation( 0, 0, 0, 0, pos1, pos2 );
//			}
			break;
		}
		case MOVER_2TO1: {
			closeTime = -1;
			physicsObj.SetLinearExtrapolation( EXTRAPOLATION_LINEAR, time, duration, pos2, ( pos1 - pos2 ) * 1000.0f / duration, vec3_origin );
//			if ( accelTime != 0 || decelTime != 0 ) {
//				physicsObj.SetLinearInterpolation( time, accelTime, decelTime, duration, pos2, pos1 );
//			} else {
//				physicsObj.SetLinearInterpolation( 0, 0, 0, 0, pos1, pos2 );
//			}
			break;
		}
	}
}

/*
================
idMover_Binary::OpenPortals
================
*/
void idMover_Binary::OpenPortals( void ) {
	for ( idMover_Binary* slave = moveMaster; slave; slave = slave->GetActivateChain() ) {
		slave->OpenPortal();
	}
}

/*
================
idMover_Binary::ClosePortals
================
*/
void idMover_Binary::ClosePortals( bool force ) {
	for ( idMover_Binary* slave = moveMaster; slave; slave = slave->GetActivateChain() ) {
		if ( !force && slave->IsHidden() ) {
			continue;
		}

		slave->ClosePortal();
	}
}

/*
================
idMover_Binary::MatchActivateTeam

All entities in a mover team will move from pos1 to pos2
in the same amount of time
================
*/
void idMover_Binary::MatchActivateTeam( moverState_t newstate, int time ) {
	idMover_Binary *slave;

	for ( slave = this; slave != NULL; slave = slave->activateChain ) {
		slave->SetMoverState( newstate, time );
	}
}

/*
================
idMover_Binary::Enable
================
*/
void idMover_Binary::Enable( bool b ) {
	enabled = b;
}

/*
================
idMover_Binary::Event_MatchActivateTeam
================
*/
void idMover_Binary::Event_MatchActivateTeam( moverState_t newstate, int time ) {
	MatchActivateTeam( newstate, time );
}

/*
================
idMover_Binary::BindTeam

All entities in a mover team will be bound 
================
*/
void idMover_Binary::BindTeam( idEntity *bindTo ) {
	idMover_Binary *slave;

	for ( slave = this; slave != NULL; slave = slave->activateChain ) {
		slave->Bind( bindTo, true );
	}
}

/*
================
idMover_Binary::JoinActivateTeam

Set all entities in a mover team to be enabled
================
*/
void idMover_Binary::JoinActivateTeam( idMover_Binary *master ) {
	this->activateChain = master->activateChain;
	master->activateChain = this;
}

/*
================
idMover_Binary::Event_Enable

Set all entities in a mover team to be enabled
================
*/
void idMover_Binary::Event_Enable( void ) {
	for ( idMover_Binary* slave = moveMaster; slave != NULL; slave = slave->activateChain ) {
		slave->Enable( true );
	}
}

/*
================
idMover_Binary::Event_Disable

Set all entities in a mover team to be disabled
================
*/
void idMover_Binary::Event_Disable( void ) {
	for ( idMover_Binary* slave = moveMaster; slave != NULL; slave = slave->activateChain ) {
		slave->Enable( false );
	}
}

/*
================
idMover_Binary::Event_OpenPortal

Sets the portal associtated with this mover to be open
================
*/
void idMover_Binary::Event_OpenPortal( void ) {
	for ( idMover_Binary* slave = moveMaster; slave != NULL; slave = slave->activateChain ) {
		slave->OpenPortal();
	}
}

/*
================
idMover_Binary::Event_ClosePortal

Sets the portal associtated with this mover to be closed
================
*/
void idMover_Binary::Event_ClosePortal( void ) {
	for ( idMover_Binary* slave = moveMaster; slave != NULL; slave = slave->activateChain ) {
		if ( slave->IsHidden() ) {
			continue;
		}
		slave->ClosePortal();
	}
}

/*
================
idMover_Binary::ReachedPosition
================
*/
void idMover_Binary::ReachedPosition( void ) {

	if ( moverState == MOVER_1TO2 ) {
		// reached pos2
		if ( moveMaster == this ) {
			StartSound( "snd_opened", SND_DOOR, 0, NULL );
		}

		SetMoverState( MOVER_POS2, gameLocal.time );

		if ( enabled && wait >= 0 && !toggle ) {
			closeTime = gameLocal.time + SEC2MS( wait );
		}
	} else if ( moverState == MOVER_2TO1 ) {
		// reached pos1
		if ( moveMaster == this ) {
			StartSound( "snd_closed", SND_DOOR, 0, NULL );
		}

		SetMoverState( MOVER_POS1, gameLocal.time );

		// close areaportals
		if ( moveMaster == this ) {
			ClosePortals( false );
		}

		if ( enabled && wait >= 0 && spawnArgs.GetBool( "continuous" ) ) {
			PostEventSec( &EV_Activate, wait, this );
		}
	}
}

/*
================
idMover_Binary::GotoPosition1
================
*/
void idMover_Binary::GotoPosition1( void ) {
	idMover_Binary *slave;
	int	partial;

	// only the master should control this
	if ( moveMaster != this ) {
		moveMaster->GotoPosition1();
		return;
	}

	if ( ( moverState == MOVER_POS1 ) || ( moverState == MOVER_2TO1 ) ) {
		// already there, or on the way
		return;
	}

	if ( moverState == MOVER_POS2 ) {
		for ( slave = this; slave != NULL; slave = slave->activateChain ) {
			slave->closeTime = -1;
			if ( !toggle ) {
				slave->closeTime = gameLocal.time;
			}
		}
		return;
	}

	// only partway up before reversing
	if ( moverState == MOVER_1TO2 ) {
		// use the physics times because this might be executed during the physics simulation
		partial = physicsObj.GetLinearEndTime() - physicsObj.GetTime();
		assert( partial >= 0 );
		if ( partial < 0 ) {
			partial = 0;
		}
		MatchActivateTeam( MOVER_2TO1, physicsObj.GetTime() - partial );
		// if already at at position 1 (partial == duration) execute the reached event
		if ( partial >= duration ) {
			ReachedPosition();
		}
	}
}

/*
================
idMover_Binary::GotoPosition2
================
*/
void idMover_Binary::GotoPosition2( void ) {
	int	partial;

	// only the master should control this
	if ( moveMaster != this ) {
		moveMaster->GotoPosition2();
		return;
	}

	if ( ( moverState == MOVER_POS2 ) || ( moverState == MOVER_1TO2 ) ) {
		// already there, or on the way
		return;
	}

	if ( moverState == MOVER_POS1 ) {
		MatchActivateTeam( MOVER_1TO2, gameLocal.time );

		// open areaportal
		OpenPortals();
		return;
	}


	// only partway up before reversing
	if ( moverState == MOVER_2TO1 ) {
		// use the physics times because this might be executed during the physics simulation
		partial = physicsObj.GetLinearEndTime() - physicsObj.GetTime();
		assert( partial >= 0 );
		if ( partial < 0 ) {
			partial = 0;
		}
		MatchActivateTeam( MOVER_1TO2, physicsObj.GetTime() - partial );
		// if already at at position 2 (partial == duration) execute the reached event
		if ( partial >= duration ) {
			ReachedPosition();
		}
	}
}

/*
================
idMover_Binary::Use_BinaryMover
================
*/
void idMover_Binary::Use_BinaryMover( idEntity *activator ) {
	// only the master should be used
	if ( moveMaster != this ) {
		moveMaster->Use_BinaryMover( activator );
		return;
	}

	if ( !enabled ) {
		return;
	}

	activatedBy = activator;

	if ( moverState == MOVER_POS1 ) {
		MatchActivateTeam( MOVER_1TO2, gameLocal.time + gameLocal.msec );

		// open areaportal
		OpenPortals();
		return;
	}

	// if all the way up, just delay before coming down
	if ( moverState == MOVER_POS2 ) {
		idMover_Binary *slave;

		if ( wait == -1 ) {
			return;
		}

		for ( slave = this; slave != NULL; slave = slave->activateChain ) {
			if ( toggle ) {
				slave->closeTime = gameLocal.time;
			} else {
				slave->closeTime = gameLocal.time + SEC2MS( wait );
			}
		}
		return;
	}

	// only partway down before reversing
	if ( moverState == MOVER_2TO1 ) {
		GotoPosition2();
		return;
	}

	// only partway up before reversing
	if ( moverState == MOVER_1TO2 ) {
		GotoPosition1();
		return;
	}
}

/*
================
idMover_Binary::Event_Use_BinaryMover
================
*/
void idMover_Binary::Event_Use_BinaryMover( idEntity *activator ) {
	Use_BinaryMover( activator );
}

/*
================
idMover_Binary::PreBind
================
*/
void idMover_Binary::PreBind( void ) {
	pos1 = GetWorldCoordinates( pos1 );
	pos2 = GetWorldCoordinates( pos2 );
}

/*
================
idMover_Binary::PostBind
================
*/
void idMover_Binary::PostBind( void ) {
	pos1 = GetLocalCoordinates( pos1 );
	pos2 = GetLocalCoordinates( pos2 );
}

/*
================
idMover_Binary::InitSpeed

pos1, pos2, and speed are passed in so the movement delta can be calculated
================
*/
void idMover_Binary::InitSpeed( idVec3 &mpos1, idVec3 &mpos2, float mspeed, float maccelTime, float mdecelTime ) {
	idVec3		move;
	float		distance;
	float		speed;

	pos1		= mpos1;
	pos2		= mpos2;

	accelTime	= idPhysics::SnapTimeToPhysicsFrame( SEC2MS( maccelTime ) );
	decelTime	= idPhysics::SnapTimeToPhysicsFrame( SEC2MS( mdecelTime ) );

	speed		= mspeed ? mspeed : 100;

	// calculate time to reach second position from speed
	move = pos2 - pos1;
	distance = move.Length();
	duration = idPhysics::SnapTimeToPhysicsFrame( distance * 1000 / speed );
	if ( duration <= 0 ) {
		duration = 1;
	}

	moverState = MOVER_POS1;

	physicsObj.SetLinearExtrapolation( EXTRAPOLATION_NONE, 0, 0, pos1, vec3_origin, vec3_origin );
//	physicsObj.SetLinearInterpolation( 0, 0, 0, 0, vec3_origin, vec3_origin );
	SetOrigin( pos1 );
}

/*
================
idMover_Binary::InitTime

pos1, pos2, and time are passed in so the movement delta can be calculated
================
*/
void idMover_Binary::InitTime( idVec3 &mpos1, idVec3 &mpos2, float mtime, float maccelTime, float mdecelTime ) {

	pos1		= mpos1;
	pos2		= mpos2;

	accelTime	= idPhysics::SnapTimeToPhysicsFrame( SEC2MS( maccelTime ) );
	decelTime	= idPhysics::SnapTimeToPhysicsFrame( SEC2MS( mdecelTime ) );

	duration	= idPhysics::SnapTimeToPhysicsFrame( SEC2MS( mtime ) );
	if ( duration <= 0 ) {
		duration = 1;
	}

	moverState = MOVER_POS1;

	physicsObj.SetLinearExtrapolation( EXTRAPOLATION_NONE, 0, 0, pos1, vec3_origin, vec3_origin );
//	physicsObj.SetLinearInterpolation( 0, 0, 0, 0, vec3_origin, vec3_origin );
	SetOrigin( pos1 );
}

/*
================
idMover_Binary::GetActivator
================
*/
idEntity *idMover_Binary::GetActivator( void ) const {
	return activatedBy.GetEntity();
}

/*
================
idMover_Binary::CheckNetworkStateChanges
================
*/
bool idMover_Binary::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_BASE( sdMoverBinaryBroadcastState );
		
		if ( baseData.closeTime != closeTime ) {
			return true;
		}

		if ( baseData.hidden != fl.hidden ) {
			return true;
		}

		if ( baseData.toggle != toggle ) {
			return true;
		}
	}

	return sdScriptEntity::CheckNetworkStateChanges( mode, baseState );
}

/*
================
idMover_Binary::WriteNetworkState
================
*/
void idMover_Binary::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdMoverBinaryBroadcastState );

		// update state
		newData.closeTime		= closeTime;
		newData.hidden			= fl.hidden;
		newData.toggle			= toggle;

		// write state
		msg.WriteDeltaLong( baseData.closeTime, newData.closeTime );
		msg.WriteBool( newData.hidden );
		msg.WriteBool( newData.toggle );
	}

	sdScriptEntity::WriteNetworkState( mode, baseState, newState, msg );
}

/*
================
idMover_Binary::ApplyNetworkState
================
*/
void idMover_Binary::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_NEW( sdMoverBinaryBroadcastState );

		// update state
		closeTime			= newData.closeTime;
		toggle				= newData.toggle;
		if ( newData.hidden ) {
			Hide();
		} else {
			Show();
		}
	}

	sdScriptEntity::ApplyNetworkState( mode, newState );
}

/*
================
idMover_Binary::ReadNetworkState
================
*/
void idMover_Binary::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdMoverBinaryBroadcastState );

		// read state
		newData.closeTime	= msg.ReadDeltaLong( baseData.closeTime );
		newData.hidden		= msg.ReadBool();
		newData.toggle		= msg.ReadBool();
	}

	sdScriptEntity::ReadNetworkState( mode, baseState, newState, msg );
}

/*
================
idMover_Binary::CreateNetworkStructure
================
*/
sdEntityStateNetworkData* idMover_Binary::CreateNetworkStructure( networkStateMode_t mode ) const {
	if ( mode == NSM_BROADCAST ) {
		return new sdMoverBinaryBroadcastState();
	}

	return sdScriptEntity::CreateNetworkStructure( mode );
}

/*
===============================================================================

idPlat

===============================================================================
*/

CLASS_DECLARATION( idMover_Binary, idPlat )
END_CLASS

/*
===============
idPlat::idPlat
===============
*/
idPlat::idPlat( void ) {
	trigger = NULL;
	localTriggerOrigin.Zero();
	localTriggerAxis.Identity();
}

/*
===============
idPlat::~idPlat
===============
*/
idPlat::~idPlat( void ) {
	gameLocal.clip.DeleteClipModel( trigger );
}

/*
===============
idPlat::Spawn
===============
*/
void idPlat::Spawn( void ) {
	float	lip;
	float	height;
	float	time;
	float	speed;
	float	accel;
	float	decel;
	bool	noTouch;

	spawnArgs.GetFloat( "speed", "100", speed );
	spawnArgs.GetFloat( "damage", "0", damage );
	spawnArgs.GetFloat( "wait", "1", wait );
	spawnArgs.GetFloat( "lip", "8", lip );
	spawnArgs.GetFloat( "accel_time", "0.25", accel );
	spawnArgs.GetFloat( "decel_time", "0.25", decel );

	// create second position
	if ( !spawnArgs.GetFloat( "height", "0", height ) ) {
		height = ( GetPhysics()->GetBounds()[1][2] - GetPhysics()->GetBounds()[0][2] ) - lip;
	}

	spawnArgs.GetBool( "no_touch", "0", noTouch );

	pos1 = GetPhysics()->GetOrigin();
	pos2 = pos1;
	pos2[2] += height;

	if ( spawnArgs.GetFloat( "time", "1", time ) ) {
		InitTime( pos1, pos2, time, accel, decel );
	} else {
		InitSpeed( pos1, pos2, speed, accel, decel );
	}

	SetMoverState( MOVER_POS1, gameLocal.time );
	UpdateVisuals();

	// spawn the trigger if one hasn't been custom made
	if ( !noTouch ) {
		// spawn trigger
		SpawnPlatTrigger( pos1 );
	}
}

/*
================
idPlat::Think
================
*/
void idPlat::Think( void ) {
	idMover_Binary::Think();

	RunPhysics();

	const idVec3& org = physicsObj.GetOrigin();

	// update trigger position
	if ( trigger ) {
		trigger->Link( gameLocal.clip, this, 0, org, mat3_identity );
	}
}

/*
================
idPlat::PreBind
================
*/
void idPlat::PreBind( void ) {
	idMover_Binary::PreBind();
}

/*
================
idPlat::PostBind
================
*/
void idPlat::PostBind( void ) {
	idMover_Binary::PostBind();
	GetLocalTriggerPosition( trigger );
}

/*
================
idPlat::GetLocalTriggerPosition
================
*/
void idPlat::GetLocalTriggerPosition( const idClipModel *trigger ) {
	idVec3 origin;
	idMat3 axis;

	if ( !trigger ) {
		return;
	}

	GetMasterPosition( origin, axis );
	localTriggerOrigin = ( trigger->GetOrigin() - origin ) * axis.Transpose();
	localTriggerAxis = trigger->GetAxis() * axis.Transpose();
}

/*
==============
idPlat::SpawnPlatTrigger
===============
*/
void idPlat::SpawnPlatTrigger( idVec3 &pos ) {
	idBounds		bounds;
	idVec3			tmin;
	idVec3			tmax;

	// the middle trigger will be a thin trigger just
	// above the starting position

	bounds = GetPhysics()->GetBounds();

	tmin[0] = bounds[0][0] + 33;
	tmin[1] = bounds[0][1] + 33;
	tmin[2] = bounds[0][2];

	tmax[0] = bounds[1][0] - 33;
	tmax[1] = bounds[1][1] - 33;
	tmax[2] = bounds[1][2] + 8;

	if ( tmax[0] <= tmin[0] ) {
		tmin[0] = ( bounds[0][0] + bounds[1][0] ) * 0.5f;
		tmax[0] = tmin[0] + 1;
	}
	if ( tmax[1] <= tmin[1] ) {
		tmin[1] = ( bounds[0][1] + bounds[1][1] ) * 0.5f;
		tmax[1] = tmin[1] + 1;
	}
	
	trigger = new idClipModel( idTraceModel( idBounds( tmin, tmax ) ), true );
	trigger->Link( gameLocal.clip, this, 255, GetPhysics()->GetOrigin(), mat3_identity );
	trigger->SetContents( CONTENTS_TRIGGER );
}

/*
==============
idPlat::OnTouch
===============
*/
void idPlat::OnTouch( idEntity *other, const trace_t& trace ) {
	idPlayer* player = other->Cast< idPlayer >();
	if ( !player ) {
		return;
	}

	if ( player->GetHealth() <= 0 ) {
		return;
	}

	if ( ( GetMoverState() == MOVER_POS1 ) && trigger && ( trace.c.id == trigger->GetId() ) ) {
		Use_BinaryMover( other );
	}
}

/*
================
idPlat::OnTeamBlocked
================
*/
void idPlat::OnTeamBlocked( idEntity *blockedEntity, idEntity *blockingEntity ) {
	// reverse direction
	Use_BinaryMover( activatedBy.GetEntity() );
}

/*
===============
idPlat::OnPartBlocked
===============
*/
void idPlat::OnPartBlocked( idEntity *blockingEntity ) {
	if ( damage > 0.0f ) {
		blockingEntity->Damage( this, this, vec3_origin, DAMAGE_FOR_NAME( "damage_moverCrush" ), damage, NULL );
	}
}
