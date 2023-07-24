// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "General.h"
#include "../ContentMask.h"

/*
===============================================================================

	sdGeneralMoverPhysicsNetworkData

===============================================================================
*/

/*
================
sdGeneralMoverPhysicsNetworkData::MakeDefault
================
*/
void sdGeneralMoverPhysicsNetworkData::MakeDefault( void ) {
	currentFraction = 0.f;
}

/*
================
sdGeneralMoverPhysicsNetworkData::Write
================
*/
void sdGeneralMoverPhysicsNetworkData::Write( idFile* file ) const {
	file->WriteFloat( currentFraction );
}

/*
================
sdGeneralMoverPhysicsNetworkData::Read
================
*/
void sdGeneralMoverPhysicsNetworkData::Read( idFile* file ) {
	file->ReadFloat( currentFraction );
}

/*
===============================================================================

	sdPhysics_GeneralMover

===============================================================================
*/

/*
================
sdPhysics_GeneralMover::sdPhysics_GeneralMover
================
*/
sdPhysics_GeneralMover::sdPhysics_GeneralMover( void ) {
	clipModel			= NULL;
}

/*
================
sdPhysics_GeneralMover::~sdPhysics_GeneralMover
================
*/
sdPhysics_GeneralMover::~sdPhysics_GeneralMover( void ) {
	gameLocal.clip.DeleteClipModel( clipModel );
}

/*
================
sdPhysics_GeneralMover::GetNumClipModels
================
*/
int sdPhysics_GeneralMover::GetNumClipModels( void ) const {
	return 1;
}

/*
================
sdPhysics_GeneralMover::GetOrigin
================
*/
const idVec3& sdPhysics_GeneralMover::GetOrigin( int id ) const {
	return clipModel->GetOrigin();
}

/*
================
sdPhysics_GeneralMover::Evaluate
================
*/
bool sdPhysics_GeneralMover::Evaluate( int timeStepMSec, int endTimeMSec ) {
	if ( IsAtRest() ) {
		return false;
	}

	float oldPos = currentFraction;
	float diff = MS2SEC( timeStepMSec ) * rate;

	float newPos = currentFraction + diff;
	if ( newPos > 1.f ) {
		newPos = 1.f;
	}

	float distance = newPos - oldPos;

	self->OnMoveStarted();

	idVec3 startPos			= Lerp( move.startPos, move.endPos, oldPos );
	idVec3 endPos			= Lerp( move.startPos, move.endPos, newPos );

	idMat3 startAxis		= Lerp( move.startAngles, move.endAngles, oldPos ).ToMat3();
	idMat3 endAxis			= Lerp( move.startAngles, move.endAngles, newPos ).ToMat3();

	bool collision = false;

	trace_t pushResults;
	gameLocal.push.ClipPush( pushResults, self, 0, startPos, startAxis, endPos, endAxis, GetClipModel( 0 ) );
	if ( pushResults.fraction < 1.0f ) {
		collision = true;
	}

	self->OnMoveFinished();

	if ( collision ) {
		self->OnTeamBlocked( self, gameLocal.entities[ pushResults.c.entityNum ] );
	} else {
		SetCurrentPos( newPos );
	}

	return currentFraction != oldPos;
}

/*
================
sdPhysics_GeneralMover::SetCurrentPos
================
*/
void sdPhysics_GeneralMover::SetCurrentPos( float newPos, bool force ) {
	if ( currentFraction == newPos && !force ) {
		return;
	}

	currentFraction = newPos;

	UpdateClipModel();

	if ( currentFraction == 1.f ) {
		self->ReachedPosition();
		self->BecomeInactive( TH_PHYSICS );
	} else {
		self->BecomeActive( TH_PHYSICS );
	}
}

/*
================
sdPhysics_GeneralMover::StartMove
================
*/
void sdPhysics_GeneralMover::StartMove( const idVec3& startPos, const idVec3& endPos, const idAngles& startAngles, const idAngles& endAngles, int startTime, int length ) {
	move.startPos		= startPos;
	move.endPos			= endPos;
	move.startAngles	= startAngles;
	move.endAngles		= endAngles;

	float newFraction;
	if ( length == 0 ) {
		rate				= 0.f;
		newFraction			= 1.f;
	} else {
		newFraction			= ( gameLocal.time - startTime ) / ( float )length;
		if ( newFraction > 1.f ) {
			newFraction		= 1.f;
		}

		rate				= 1.f / MS2SEC( length );
	}

	SetCurrentPos( newFraction, true );

	UpdateClipModel();
}

/*
================
sdPhysics_GeneralMover::UpdateClipModel
================
*/
void sdPhysics_GeneralMover::UpdateClipModel( void ) {
	if ( !clipModel ) {
		return;
	}

	idVec3 org		= Lerp( move.startPos,		move.endPos,	currentFraction );
	idMat3 axes		= Lerp( move.startAngles,	move.endAngles,	currentFraction ).ToMat3();

	clipModel->Link( gameLocal.clip, self, 0, org, axes );
}

/*
================
sdPhysics_GeneralMover::SetInitialPosition
================
*/
void sdPhysics_GeneralMover::SetInitialPosition( const idVec3& org, const idMat3& axes ) {
	move.startPos		= org;
	move.endPos			= org;

	move.startAngles	= axes.ToAngles();
	move.endAngles		= axes.ToAngles();

	SetCurrentPos( 1.f, true );
	rate				= 0.f;

	UpdateClipModel();
}

/*
================
sdPhysics_GeneralMover::SetClipModel
================
*/
void sdPhysics_GeneralMover::SetClipModel( idClipModel *model, float density, int id, bool freeOld ) {
	assert( self );
	assert( model );

	if ( clipModel != NULL && clipModel != model && freeOld ) {
		gameLocal.clip.DeleteClipModel( clipModel );
	}
	clipModel = model;

	UpdateClipModel();
}

/*
================
sdPhysics_GeneralMover::EvaluateContacts
================
*/
bool sdPhysics_GeneralMover::EvaluateContacts( CLIP_DEBUG_PARMS_DECLARATION_ONLY ) {
	return false;
}

/*
================
sdPhysics_GeneralMover::IsAtRest
================
*/
bool sdPhysics_GeneralMover::IsAtRest( void ) const {
	return currentFraction == 1.f;
}

/*
================
sdPhysics_GeneralMover::Pushable
================
*/
bool sdPhysics_GeneralMover::IsPushable( void ) const {
	return false;
}

/*
================
sdPhysics_GeneralMover::GetBounds
================
*/
const idBounds&	sdPhysics_GeneralMover::GetBounds( int id ) const {
	return clipModel->GetBounds();
}

/*
================
sdPhysics_GeneralMover::GetAbsBounds
================
*/
const idBounds&	sdPhysics_GeneralMover::GetAbsBounds( int id ) const {
	return clipModel->GetAbsBounds();
}

/*
================
sdPhysics_GeneralMover::GetClipModel
================
*/
idClipModel* sdPhysics_GeneralMover::GetClipModel( int id ) const {
	return clipModel;
}

/*
================
sdPhysics_GeneralMover::GetContents
================
*/
int sdPhysics_GeneralMover::GetContents( int id ) const {
	return clipModel->GetContents();
}

/*
================
sdPhysics_GeneralMover::SetContents
================
*/
void sdPhysics_GeneralMover::SetContents( int contents, int id ) {
	clipModel->SetContents( contents );
}

/*
================
sdPhysics_GeneralMover::GetClipMask
================
*/
int sdPhysics_GeneralMover::GetClipMask( int id ) const {
	return clipMask;
}

/*
================
sdPhysics_GeneralMover::SetClipMask
================
*/
void sdPhysics_GeneralMover::SetClipMask( int mask, int id ) {
	clipMask = mask;
}

/*
================
sdPhysics_GeneralMover::GetAxis
================
*/
const idMat3& sdPhysics_GeneralMover::GetAxis( int id ) const {
	return clipModel->GetAxis();
}

/*
================
sdPhysics_GeneralMover::EnableClip
================
*/
void sdPhysics_GeneralMover::EnableClip( void ) {
	clipModel->Enable();
}

/*
================
sdPhysics_GeneralMover::DisableClip
================
*/
void sdPhysics_GeneralMover::DisableClip( bool activateContacting ) {
	if ( activateContacting ) {
		WakeEntitiesContacting( self, clipModel );
	}
	clipModel->Disable();
}

/*
================
sdPhysics_GeneralMover::CreateNetworkStructure
================
*/
sdEntityStateNetworkData* sdPhysics_GeneralMover::CreateNetworkStructure( networkStateMode_t mode ) const {
	if ( mode == NSM_VISIBLE ) {
		return new sdGeneralMoverPhysicsNetworkData();
	}
	return NULL;
}

/*
================
sdPhysics_GeneralMover::CheckNetworkStateChanges
================
*/
bool sdPhysics_GeneralMover::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_BASE( sdGeneralMoverPhysicsNetworkData );

		NET_CHECK_FIELD( currentFraction, currentFraction );
	}

	return false;
}

/*
================
sdPhysics_GeneralMover::ApplyNetworkState
================
*/
void sdPhysics_GeneralMover::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_NEW( sdGeneralMoverPhysicsNetworkData );

		SetCurrentPos( newData.currentFraction );

		UpdateClipModel();
	}
}

/*
================
sdPhysics_GeneralMover::ReadNetworkState
================
*/
void sdPhysics_GeneralMover::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdGeneralMoverPhysicsNetworkData );

		newData.currentFraction = msg.ReadDeltaFloat( baseData.currentFraction );
	}
}

/*
================
sdPhysics_GeneralMover::WriteNetworkState
================
*/
void sdPhysics_GeneralMover::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdGeneralMoverPhysicsNetworkData );

		newData.currentFraction = currentFraction;

		msg.WriteDeltaFloat( baseData.currentFraction, newData.currentFraction );
	}
}

/*
===============================================================================

sdGeneralMover

===============================================================================
*/

#define net_moverStateBits idMath::BitsForInteger( GMS_NUM_STATES )

extern const idEventDef EV_GetNumPositions;

const idEventDef EV_AddPosition( "addPosition", 'd', "Adds a position for use by this entity to move itself, and returns the index of the added position.", 2, NULL, "v", "position", "World space origin for this position.", "v", "orientation", "World space angles for this position." );
const idEventDef EV_GetState( "getMoverState", 'd', "Returns the movement state for this mover, either GMS_WAITING or GMS_MOVING.", 0, NULL );
const idEventDef EV_StartTimedMove( "startTimedMove", '\0', "Starts a move from previously added positions, taking the time specified.", 3, "A warning will be given if either of the position indices are out of range\n", "d", "start", "Index for the start position.", "d", "end", "Index for the end position.", "f", "time", "Time in seconds for the move to take." );
const idEventDef EV_SetPosition( "setPosition", '\0', "Sets the current position of the mover directly.", 1, "A warning will be given if the index is out of range.\n", "d", "position", "Index of the previously added position." );
const idEventDef EV_GetNumPositions( "getNumPositions", 'd', "Returns the number of added positions.", 0, NULL );
const idEventDef EV_KillBlockingEntity( "killBlockingEntity", '\0', "Sets the flag which controls whether the mover will apply damage to any entity blocking its path.", 1, NULL, "b", "kill", "Should the mover apply damage or not." );

CLASS_DECLARATION( sdScriptEntity,	sdGeneralMover )
	EVENT( EV_AddPosition,			sdGeneralMover::Event_AddPosition )
	EVENT( EV_GetState,				sdGeneralMover::Event_GetState )
	EVENT( EV_StartTimedMove,		sdGeneralMover::Event_StartTimedMove )
	EVENT( EV_SetPosition,			sdGeneralMover::Event_SetPosition )
	EVENT( EV_KillBlockingEntity,	sdGeneralMover::Event_KillBlockingEntity )
	EVENT( EV_GetNumPositions,		sdGeneralMover::Event_GetNumPositions )
END_CLASS

/*
============
sdGeneralMover::sdGeneralMover
============
*/
sdGeneralMover::sdGeneralMover( void ) 
	: _killBlocked( true ) {
}

/*
============
sdGeneralMover::~sdGeneralMover
============
*/
sdGeneralMover::~sdGeneralMover( void ) {
}

/*
============
sdGeneralMover::Spawn
============
*/
void sdGeneralMover::Spawn( void ) {
	_physicsObj.SetSelf( this );
	_physicsObj.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.f, 0 );
	_physicsObj.SetInitialPosition( GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
	_physicsObj.SetClipMask( MASK_PLAYERSOLID, 0 );
	SetPhysics( &_physicsObj );

	_curMove.startPos	= -1;
	_curMove.endPos		= -1;
	_curMove.startTime	= -1;
	_curMove.moveTime	= -1;

	scriptEntityFlags.writeBind		= true;
}

/*
================
sdGeneralMover::PostMapSpawn
================
*/
void sdGeneralMover::PostMapSpawn( void ) {
	sdScriptEntity::PostMapSpawn();

	const idKeyValue* kv = NULL;
	while ( kv = spawnArgs.MatchPrefix( "position_", kv ) ) {
		idEntity* other = gameLocal.FindEntity( kv->GetValue() );
		if ( !other ) {
			gameLocal.Warning( "sdGeneralMover::PostMapSpawn Couldn't Find Entity '%s'", kv->GetValue().c_str() );
			continue;
		}

		AddPosition( other->GetPhysics()->GetOrigin(), other->GetPhysics()->GetAxis().ToAngles() );
	}
}

/*
============
sdGeneralMover::AddPosition
============
*/
int sdGeneralMover::AddPosition( const idVec3& pos, const idAngles& angles ) {
	int index = _positions.Num();

	positionInfo_t info;
	info.pos = pos;
	info.angles = angles;
	_positions.Append( info );

	return index;
}

/*
============
sdGeneralMover::WriteDemoBaseData
============
*/
void sdGeneralMover::WriteDemoBaseData( idFile* file ) const {
	sdScriptEntity::WriteDemoBaseData( file );

	file->WriteInt( _curMove.startPos );
	file->WriteInt( _curMove.endPos );
	file->WriteInt( _curMove.moveTime );
	file->WriteInt( _curMove.startTime );
}

/*
============
sdGeneralMover::ReadDemoBaseData
============
*/
void sdGeneralMover::ReadDemoBaseData( idFile* file ) {
	sdScriptEntity::ReadDemoBaseData( file );

	file->ReadInt( _curMove.startPos );
	file->ReadInt( _curMove.endPos );
	file->ReadInt( _curMove.moveTime );
	file->ReadInt( _curMove.startTime );

	StartTimedMove( _curMove.startPos, _curMove.endPos, _curMove.moveTime, _curMove.startTime );
}

/*
============
sdGeneralMover::ClientReceiveEvent
============
*/
bool sdGeneralMover::ClientReceiveEvent( int event, int time, const idBitMsg& msg ) {
	switch ( event ) {
		case EVENT_MOVE: {
			int startPos	= msg.ReadLong();
			int endPos		= msg.ReadLong();
			int moveTime	= msg.ReadLong();
			int startTime	= msg.ReadLong();

			StartTimedMove( startPos, endPos, moveTime, startTime );

			return true;
		}
	}

	return sdScriptEntity::ClientReceiveEvent( event, time, msg );
}

/*
============
sdGeneralMover::StartTimedMove
============
*/
void sdGeneralMover::StartTimedMove( int from, int to, int ms, int startTime ) {
	if( from < 0 || from >= _positions.Num() || to < 0 || to >= _positions.Num() ) {
		gameLocal.Warning( "sdGeneralMover: one or more index out of range: from = %d, to = %d", from, to );
		return;
	}

	_curMove.startPos		= from;
	_curMove.endPos			= to;
	_curMove.moveTime		= ms;
	_curMove.startTime		= startTime;

	_physicsObj.StartMove(	_positions[ _curMove.startPos ].pos, _positions[ _curMove.endPos ].pos,
							_positions[ _curMove.startPos ].angles, _positions[ _curMove.endPos ].angles, _curMove.startTime, _curMove.moveTime );

	if ( gameLocal.isServer ) {
		sdEntityBroadcastEvent event( this, EVENT_MOVE );
		event.WriteLong( _curMove.startPos );
		event.WriteLong( _curMove.endPos );
		event.WriteLong( _curMove.moveTime );
		event.WriteLong( _curMove.startTime );
		event.Send( true, sdReliableMessageClientInfoAll() );
	}

	UpdateVisuals();
}

/*
============
sdGeneralMover::GetState
============
*/
sdGeneralMover::state_t sdGeneralMover::GetState( void ) const {
	if ( _physicsObj.IsAtRest() ) {
		return GMS_WAITING;
	}
	return GMS_MOVING;
}

/*
============
sdGeneralMover::ReachedPosition
============
*/
void sdGeneralMover::ReachedPosition( void ) {
}

/*
============
sdGeneralMover::OnTeamBlocked
============
*/
void sdGeneralMover::OnTeamBlocked( idEntity* blockedPart, idEntity* blockingEntity ) {
	if ( _killBlocked ) {
		blockingEntity->Damage( NULL, NULL, vec3_zero, DAMAGE_FOR_NAME( "damage_mover_crush" ), 1.f, NULL );
	}
}

/*
============
sdGeneralMover::Event_AddPosition
============
*/
void sdGeneralMover::Event_AddPosition( const idVec3& pos, const idAngles& dir ) {
	sdProgram::ReturnInteger( AddPosition( pos, dir ) );
}

/*
============
sdGeneralMover::Event_GetState
============
*/
void sdGeneralMover::Event_GetState() {
	sdProgram::ReturnFloat( GetState() );
}

/*
============
sdGeneralMover::Event_StartTimedMove
============
*/
void sdGeneralMover::Event_StartTimedMove( int from, int to, float seconds ) {
	StartTimedMove( from, to, SEC2MS( seconds ), gameLocal.time );
}

/*
============
sdGeneralMover::Event_SetPosition
============
*/
void sdGeneralMover::Event_SetPosition( int index ) {
	if( index < 0 || index >= _positions.Num() ) {
		gameLocal.Warning( "sdGeneralMover::Event_SetPosition: index out of range: %d", index );
		return;
	}

	StartTimedMove( index, index, 0, gameLocal.time );
	UpdateVisuals();
}

/*
============
sdGeneralMover::Event_KillBlockingEntity
============
*/
void sdGeneralMover::Event_KillBlockingEntity( bool kill ) {
	_killBlocked = kill;
}

/*
============
sdGeneralMover::Event_GetNumPositions
============
*/
void sdGeneralMover::Event_GetNumPositions( void ) {
	sdProgram::ReturnInteger( _positions.Num() );
}
