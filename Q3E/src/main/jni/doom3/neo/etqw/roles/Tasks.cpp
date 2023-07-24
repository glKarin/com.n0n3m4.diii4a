// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Tasks.h"
#include "../Player.h"
#include "../interfaces/TaskInterface.h"
#include "../script/Script_Helper.h"
#include "../script/Script_ScriptObject.h"
#include "../roles/WayPointManager.h"
#include "FireTeams.h"
#include "../rules/GameRules.h"
#include "../rules/VoteManager.h"

/*
===============================================================================

	sdPlayerTask

===============================================================================
*/

const idEventDef EV_PlayerTask_Complete( "complete", '\0', DOC_TEXT( "Marks the task as complete, and gives any bonus associated with it to players on it." ), 0, NULL );
const idEventDef EV_PlayerTask_SetTimeout( "setTimeout", '\0', DOC_TEXT( "Marks the task to auto free itself once the timeout has expired." ), 1, "Timeouts do not apply to mission or objective level tasks.", "f", "timeout", "Timeout in seconds." );
const idEventDef EV_PlayerTask_Free( "free", '\0', DOC_TEXT( "Removes the task." ), 0, "This does the same as $event:remove$." );

const idEventDef EV_PlayerTask_SetUserCreated( "setUserCreated", '\0', DOC_TEXT( "Marks a task as user created, this will demote it to task level from whatever its current level is." ), 0, NULL );
const idEventDef EV_PlayerTask_IsUserCreated( "isUserCreated", 'b', DOC_TEXT( "Returns whether the task has been marked as user created or not." ), 0, NULL );

const idEventDef EV_PlayerTask_GetTaskEntity( "getTaskEntity", 'e', DOC_TEXT( "Returns the entity associated with this task." ), 0, NULL );
const idEventDef EV_PlayerTask_SetTargetPos( "setLocation", '\0', DOC_TEXT( "Sets a fixed location for the specified task waypoint." ), 2, NULL, "d", "index", "Index of the waypoint.", "v", "position", "Fixed position for the waypoint." );
const idEventDef EV_PlayerTask_SetWayPointState( "setWayPointState", '\0', DOC_TEXT( "Sets the visibility state for the specified task waypoint." ), 2, NULL, "d", "index", "Index of the waypoint.", "b", "state", "State to set." );

const idEventDef EV_PlayerTask_GiveObjectiveProficiency( "giveObjectiveProficiency", '\0', DOC_TEXT( "Gives class XP to all players elligible for the task." ), 2, NULL, "f", "count", "Amount of XP to give.", "s", "reason", "Reason to record in log." );

const idEventDef EV_PlayerTask_FlashIcon( "flashWorldIcon", '\0', DOC_TEXT( "Flash the icon for a set time." ), 1, NULL, "d", "time", "Time to flash in milliseconds." );

CLASS_DECLARATION( idClass, sdPlayerTask )
	EVENT( EV_PlayerTask_Complete,				sdPlayerTask::Event_Complete )
	EVENT( EV_PlayerTask_SetTimeout,			sdPlayerTask::Event_SetTimeout )
	EVENT( EV_PlayerTask_Free,					sdPlayerTask::Event_Free )
	EVENT( EV_SafeRemove,						sdPlayerTask::Event_Free ) // Gordon: Also handle remove nicely, in case people use it

	EVENT( EV_PlayerTask_SetUserCreated,		sdPlayerTask::Event_SetUserCreated )
	EVENT( EV_PlayerTask_IsUserCreated,			sdPlayerTask::Event_IsUserCreated )

	EVENT( EV_PlayerTask_GetTaskEntity,			sdPlayerTask::Event_GetTaskEntity )
	EVENT( EV_PlayerTask_SetTargetPos,			sdPlayerTask::Event_SetTargetPos )
	EVENT( EV_PlayerTask_SetWayPointState,		sdPlayerTask::Event_SetWayPointState )

	EVENT( EV_GetKey,							sdPlayerTask::Event_GetKey )
	EVENT( EV_GetIntKey,						sdPlayerTask::Event_GetIntKey )
	EVENT( EV_GetFloatKey,						sdPlayerTask::Event_GetFloatKey )
	EVENT( EV_GetVectorKey,						sdPlayerTask::Event_GetVectorKey )

	EVENT( EV_GetKeyWithDefault,				sdPlayerTask::Event_GetKeyWithDefault )
	EVENT( EV_GetIntKeyWithDefault,				sdPlayerTask::Event_GetIntKeyWithDefault )
	EVENT( EV_GetFloatKeyWithDefault,			sdPlayerTask::Event_GetFloatKeyWithDefault )
	EVENT( EV_GetVectorKeyWithDefault,			sdPlayerTask::Event_GetVectorKeyWithDefault )

	EVENT( EV_PlayerTask_GiveObjectiveProficiency,	sdPlayerTask::Event_GiveObjectiveProficiency )
	EVENT( EV_PlayerTask_FlashIcon,					sdPlayerTask::Event_FlashIcon )
END_CLASS

/*
================
sdPlayerTask::sdPlayerTask
================
*/
sdPlayerTask::sdPlayerTask( void ) : _taskInfo( NULL ), _flags( 0 ), _endTime( -1 ), _scriptObject( NULL ), _xp( 0.0f ), _wayPointEnabled( false ) {
	_node.SetOwner( this );
	_worldNode.SetOwner( this );
	_objectiveNode.SetOwner( this );
}

/*
================
sdPlayerTask::Create
================
*/
void sdPlayerTask::Create( int taskIndex, const sdDeclPlayerTask* taskInfo ) {
	_creationTime = gameLocal.time;
	_taskInfo = taskInfo;
	_taskIndex = taskIndex;
	_xp = _taskInfo->GetData().GetFloat( "xp" );
	_wayPointEnabled = false;

	if ( taskInfo->IsTask() ) {
		_flags |= TF_USERCREATED;
	}

	_wayPointInfo.SetNum( Min( taskInfo->GetNumWayPoints(), MAX_WAYPOINTS ) );
	for ( int i = 0; i < _wayPointInfo.Num(); i++ ) {
		_wayPointInfo[ i ].fixed = false;
		_wayPointInfo[ i ].enabled = true;
		_wayPointInfo[ i ].offset.Zero();
		_wayPointInfo[ i ].wayPoint = NULL;
	}

	_scriptObject = gameLocal.program->AllocScriptObject( this, taskInfo->GetScriptObject() );

	sdScriptHelper h1;
	_scriptObject->CallNonBlockingScriptEvent( _scriptObject->GetPreConstructor(), h1 );
}

/*
================
sdPlayerTask::SetEntity
================
*/
void sdPlayerTask::SetEntity( int spawnId ) {
	_entity.ForceSpawnId( spawnId );
}

/*
================
sdPlayerTask::~sdPlayerTask
================
*/
sdPlayerTask::~sdPlayerTask( void ) {
	if ( _scriptObject ) {
		sdScriptHelper h1;
		_scriptObject->CallNonBlockingScriptEvent( _scriptObject->GetDestructor(), h1 );

		gameLocal.program->FreeScriptObject( _scriptObject );
	}

	HideWayPoint();

	if ( gameLocal.isServer ) {
		sdTaskMessage msg( this, TM_REMOVE );
		msg.Send( sdReliableMessageClientInfoAll() );
	}
}

/*
================
sdPlayerTask::Think
================
*/
void sdPlayerTask::Think( void ) {
	for ( int i = 0; i < _wayPointInfo.Num(); i++ ) {
		if ( _wayPointInfo[ i ].wayPoint == NULL ) {
			continue;
		}
		_wayPointInfo[ i ].wayPoint->SetText( GetTitle() );
	}
}

/*
================
sdPlayerTask::GetHandle
================
*/
taskHandle_t sdPlayerTask::GetHandle( void ) const {
	return _taskIndex;
}

/*
================
sdPlayerTask::GetTimeLimit
================
*/
const char* sdPlayerTask::GetTimeLimit( void ) const {
	int limit = _taskInfo->GetTimeLimit();
	if ( limit <= 0 ) {
		return idStr::MS2HMS( gameLocal.rules->GetGameTime() );
	}

	int ms = limit - ( gameLocal.time - _creationTime );
	return idStr::MS2HMS( ms );
}

/*
================
sdPlayerTask::UpdateObjectiveWayPoint
================
*/
void sdPlayerTask::UpdateObjectiveWayPoint( void ) {
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( localPlayer == NULL ) {
		return;
	}

	sdTeamInfo* team = localPlayer->GetGameTeam();
	if ( team == NULL || team != _taskInfo->GetTeam() ) {
		HideWayPoint();
	} else {
		ShowWayPoint();
	}
}

/*
============
sdPlayerTask::GetXP
============
*/
float sdPlayerTask::GetXP() const {
	return _xp;
}

/*
================
sdPlayerTask::ShowWayPoint
================
*/
void sdPlayerTask::ShowWayPoint( void ) {
	_wayPointEnabled = true;
	for ( int i = 0; i < _wayPointInfo.Num(); i++ ) {
		ShowWayPoint( i );
	}
}

/*
================
sdPlayerTask::ShowWayPoint
================
*/
void sdPlayerTask::ShowWayPoint( int i ) {
	if ( _wayPointInfo[ i ].wayPoint != NULL ) {
		return;
	}

	if ( !_wayPointInfo[ i ].enabled ) {
		return;
	}

	const idDict& waypointData = _taskInfo->GetWayPointData( i );

	const char* materialName;
	const char* offScreenMaterialName;
	if ( IsUserCreated() ) {
		materialName = waypointData.GetString( "mtr_user_waypoint" );
		offScreenMaterialName = waypointData.GetString( "mtr_user_waypoint_offscreen" );
	} else {
		materialName = waypointData.GetString( "mtr_waypoint" );
		offScreenMaterialName = waypointData.GetString( "mtr_waypoint_offscreen" );
	}

	const idMaterial* material = gameLocal.declMaterialType[ materialName ];
	if ( material == NULL ) {
		gameLocal.Warning( "sdPlayerTask::ShowWayPoint Invalid Material '%s'", materialName );
		return;
	}

	const idMaterial* offScreenMaterial = gameLocal.declMaterialType[ offScreenMaterialName ];
	if ( offScreenMaterial == NULL ) {
		gameLocal.Warning( "sdPlayerTask::ShowWayPoint Invalid Offscreen Material '%s'", offScreenMaterialName );
		return;
	}

	sdWayPoint* wayPoint = sdWayPointManager::GetInstance().AllocWayPoint();
	if ( wayPoint == NULL ) {
		return;
	}		
	_wayPointInfo[ i ].wayPoint = wayPoint;

	if ( _wayPointInfo[ i ].fixed ) {
		wayPoint->SetOrigin( _wayPointInfo[ i ].offset );
	}
	wayPoint->SetOwner( _entity );

	wayPoint->SetMinRange( waypointData.GetFloat( "min_range", "320" ) );

	wayPoint->SetMaterial( material );
	wayPoint->SetOffScreenMaterial( offScreenMaterial );

	if ( IsUserCreated() ) {
		float range;
		if ( waypointData.GetFloat( "range", "", range ) ) {
			wayPoint->SetRange( range );
		}
	} else {
		wayPoint->SetAlwaysActive();
	}

	if ( waypointData.GetBool( "team_colored" ) ) {
		wayPoint->SetTeamColored();
	}

	if ( IsObjective() || IsMission() || _taskInfo->NoOcclusion() ) {
		wayPoint->SetCheckLineOfSight( false );
	}
	
	wayPoint->SetBracketed( waypointData.GetBool( "bracketed" ) );
	if ( waypointData.GetBool( "bracket_use_rendermodel" ) ) {
		wayPoint->UseRenderModel();
	}

	idVec3 mins, maxs;
	if ( waypointData.GetVector( "bracket_mins", NULL, mins ) && waypointData.GetVector( "bracket_maxs", NULL, maxs ) ) {
		wayPoint->SetBounds( idBounds( mins, maxs ) );
	}
}

/*
================
sdPlayerTask::HideWayPoint
================
*/
void sdPlayerTask::HideWayPoint( void ) {
	_wayPointEnabled = false;
	for ( int i = 0; i < _wayPointInfo.Num(); i++ ) {
		HideWayPoint( i );
	}
}

/*
================
sdPlayerTask::HideWayPoint
================
*/
void sdPlayerTask::HideWayPoint( int i ) {
	if ( _wayPointInfo[ i ].wayPoint == NULL ) {
		return;
	}

	sdWayPointManager::GetInstance().FreeWayPoint( _wayPointInfo[ i ].wayPoint );
	_wayPointInfo[ i ].wayPoint = NULL;
}

/*
================
sdPlayerTask::ProcessTitle
================
*/
idWStr& sdPlayerTask::ProcessTitle( const wchar_t* text, idWStr& output ) const {
	output.Empty();

	const wchar_t* p = text;
	while ( *p ) {
		if ( *p == L'%' ) {
			p++;
			if( !*p ) {
				break;
			}

			switch ( *p ) {
				case L'%':
					output += L'%';
					p++;
					break;

				case L'r': {
					idEntity* other = GetEntity();
					idPlayer* player = gameLocal.GetLocalPlayer();
					if ( !other || !player ) {
						output += L"???";
					} else {
						float range = idMath::Sqrt( gameLocal.RangeSquare( player, other ) );
						output += va( L"%im", ( int )( InchesToMetres( range ) ) );
					}
					p++;
					break;
				}

				case L'n': {
					idEntity* other = GetEntity();
					if ( other ) {
						idWStr name;
						other->GetTaskName( name );
						output += name;
					}
					p++;
					break;
				}

				case L'm': {
					idWStr timeLimit( va( L"%hs", GetTimeLimit() ) );
					timeLimit += L" ";
					if ( timeLimit.Find( L':' ) != idWStr::INVALID_POSITION ) {
						timeLimit += common->LocalizeText( "game/tasks/min" );
					} else {
						timeLimit += common->LocalizeText( "game/tasks/sec" );
					}
					output += timeLimit;
					p++;
					break;
				}

				default:
					output += L'%';
					break;
			}
		} else {
			output += *p;

			p++;
		}
	}

	return output;
}

/*
================
sdPlayerTask::GetTitle
================
*/
const wchar_t* sdPlayerTask::GetTitle( idPlayer* player ) const {
	const sdDeclLocStr* title = NULL;

	if ( IsObjective() ) {
		title = _taskInfo->GetFriendlyTitle();
		if ( player == NULL ) {
			player = gameLocal.GetLocalPlayer();
		}
		if ( player != NULL ) {
			if ( IsPlayerEligible( player ) ) {
				title = _taskInfo->GetTitle();
			}
		}
	} else {
		title = _taskInfo->GetTitle();
	}

	if ( title == NULL ) {
		return L"";
	}

	return ProcessTitle( title->GetText(), _titleBuffer ).c_str();
}

/*
================
sdPlayerTask::GetCompletedTitle
================
*/
const wchar_t* sdPlayerTask::GetCompletedTitle( idPlayer* player ) const {
	const sdDeclLocStr* title = NULL;

	if ( IsObjective() ) {
		if( player == NULL ) {
			player = gameLocal.GetLocalPlayer();
		}
		if ( player != NULL ) {
			if ( IsPlayerEligible( player ) ) {
				title = _taskInfo->GetCompletedTitle();
			} else {
				title = _taskInfo->GetCompletedFriendlyTitle();
			}
		}
	} else {
		title = _taskInfo->GetCompletedTitle();
	}

	if ( title == NULL ) {
		return L"";
	}

	return ProcessTitle( title->GetText(), _titleBuffer ).c_str();
}

/*
================
sdPlayerTask::SetWayPointState
================
*/
bool sdPlayerTask::SetWayPointState( int index, bool state ) {
	if ( index < 0 || index >= _wayPointInfo.Num() ) {
		gameLocal.Warning( "sdPlayerTask::SetWayPointState Index Out Of Range" );
		return false;
	}

	if ( _wayPointInfo[ index ].enabled == state ) {
		return false;
	}

	_wayPointInfo[ index ].enabled = state;
	if ( state ) {
		if ( _wayPointEnabled ) {
			ShowWayPoint( index );
		}
	} else {
		HideWayPoint( index );
	}

	return true;
}

/*
================
sdPlayerTask::SetTargetPos
================
*/
bool sdPlayerTask::SetTargetPos( int index, const idVec3& target ) {
	if ( index < 0 || index >= _wayPointInfo.Num() ) {
		gameLocal.Warning( "sdPlayerTask::SetTargetPos Index Out Of Range" );
		return false;
	}
	_wayPointInfo[ index ].fixed = true;
	_wayPointInfo[ index ].offset = target;
	if ( _wayPointInfo[ index ].wayPoint != NULL ) {
		_wayPointInfo[ index ].wayPoint->SetOrigin( target );
	}
	return true;
}

/*
================
sdPlayerTask::SetComplete
================
*/
void sdPlayerTask::SetComplete( void ) {
	if ( IsComplete() ) {
		return;
	}

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( localPlayer != NULL ) {
		if ( localPlayer->GetActiveTaskHandle() == GetHandle() ) {
			gameLocal.localPlayerProperties.OnTaskCompleted( this );
		} else if ( IsObjective() ) {
			if ( _taskInfo->GetTeam() == localPlayer->GetGameTeam() ) {
				gameLocal.localPlayerProperties.OnTaskCompleted( this );
			}
		}
	}

	_flags |= TF_COMPLETE;

	if ( !gameLocal.isClient ) {
		sdProficiencyManager::GetInstance().GiveMissionProficiency( this, GetInfo()->GetXPBonus() );
	}

	if ( gameLocal.isServer ) {
		sdTaskMessage msg( this, TM_COMPLETE );
		msg.Send( sdReliableMessageClientInfoAll() );
	}
}

/*
================
sdPlayerTask::SetUserCreated
================
*/
void sdPlayerTask::SetUserCreated( void ) {
	if ( IsUserCreated() ) {
		return;
	}

	_flags |= TF_USERCREATED;

	if ( gameLocal.isServer ) {
		sdTaskMessage msg( this, TM_USER_CREATED );
		msg.Send( sdReliableMessageClientInfoAll() );
	}
}


/*
================
sdPlayerTask::SetTimeout
================
*/
void sdPlayerTask::SetTimeout( int duration ) {
	_endTime = gameLocal.time + duration;
}

/*
================
sdPlayerTask::IsAvailable
================
*/
bool sdPlayerTask::IsAvailable( idPlayer* player ) const {
	if ( _flags & TF_COMPLETE || IsUserCreated() || IsObjective() || !IsPlayerEligible( player ) ) {
		return false;
	}

	return true;
}

/*
================
sdPlayerTask::OnPlayerJoined
================
*/
void sdPlayerTask::OnPlayerJoined( idPlayer* player ) {
	*_players.Alloc() = player;
}

/*
================
sdPlayerTask::OnPlayerLeft
================
*/
void sdPlayerTask::OnPlayerLeft( idPlayer* player ) {
	_players.Remove( player );
}

/*
============
sdPlayerTask::SelectWayPoints
============
*/
void sdPlayerTask::SelectWayPoints( int time ) {
	for( int i = 0; i < _wayPointInfo.Num(); i++ ) {
		if( _wayPointInfo[ i ].wayPoint != NULL ) {
			_wayPointInfo[ i ].wayPoint->SetSelected( time );
		}
	}
}

/*
================
sdPlayerTask::IsPlayerEligible
================
*/
bool sdPlayerTask::IsPlayerEligible( idPlayer* player ) const {
	return _taskInfo->GetEligibility().Check( player, _entity );
}

/*
================
sdPlayerTask::HandleMessage
================
*/
void sdPlayerTask::HandleMessage( taskMessageType_t type, const idBitMsg& msg ) {
	switch ( type ) {
		case TM_USER_CREATED: {
			SetUserCreated();
			break;
		}
		case TM_COMPLETE: {
			SetComplete();
			break;
		}
		case TM_SETLOCATIION: {
			int index		= msg.ReadLong();
			idVec3 location = msg.ReadVector();
			SetTargetPos( index, location );
			break;
		}
		case TM_VISSTATE: {
			int index		= msg.ReadLong();
			bool state		= msg.ReadBool();
			SetWayPointState( index, state );
			break;
		}
		default: {
			gameLocal.Error( "sdPlayerTask::HandleMessage Invalid Message Type '%i'", type );
			break;
		}
	}
}

/*
================
sdPlayerTask::HandleMessage
================
*/
void sdPlayerTask::HandleMessage( const idBitMsg& msg ) {
	if ( gameLocal.isServer ) {
		return; // listen servers should ignore these
	}

	taskMessageType_t type = ( taskMessageType_t )msg.ReadBits( idMath::BitsForInteger( TM_NUM_MESSAGES ) );
	taskHandle_t handle = msg.ReadBits( sdPlayerTask::TASK_BITS );

	switch ( type ) {
		case TM_FULLSTATE: {
			int taskInfoIndex = msg.ReadBits( idMath::BitsForInteger( gameLocal.declPlayerTaskType.Num() ) );
			int spawnId = msg.ReadLong();
			sdTaskManager::GetInstance().AllocTask( handle, gameLocal.declPlayerTaskType[ taskInfoIndex ], spawnId );
			sdPlayerTask* task = sdTaskManager::GetInstance().TaskForHandle( handle );
			assert( task );
			task->ReadInitialState( msg );
			break;
		}

		case TM_CREATE: {
			int taskInfoIndex = msg.ReadBits( idMath::BitsForInteger( gameLocal.declPlayerTaskType.Num() ) );
			int spawnId = msg.ReadLong();
			sdTaskManager::GetInstance().AllocTask( handle, gameLocal.declPlayerTaskType[ taskInfoIndex ], spawnId );
			break;
		}

		case TM_REMOVE: {
			sdTaskManager::GetInstance().FreeTask( handle );
			break;
		}

		default: {
			sdPlayerTask* task = sdTaskManager::GetInstance().TaskForHandle( handle );
			if ( task ) {
				task->HandleMessage( type, msg );
			} else {
				gameLocal.Warning( "sdPlayerTask::HandleMessage Received Message for Un-allocated task" );
			}
		}
	}
}

/*
================
sdPlayerTask::Write
================
*/
void sdPlayerTask::Write( idFile* file ) const {
	file->WriteInt( GetHandle() );
	file->WriteInt( _taskInfo->Index() );
	file->WriteInt( _entity.GetSpawnId() );

	file->WriteInt( _flags );
	
	int count = _wayPointInfo.Num();
	file->WriteInt( count );
	for ( int i = 0; i < count; i++ ) {
		file->WriteBool( _wayPointInfo[ i ].enabled );
		file->WriteBool( _wayPointInfo[ i ].fixed );
		file->WriteVec3( _wayPointInfo[ i ].offset );
	}
}

/*
================
sdPlayerTask::Read
================
*/
void sdPlayerTask::Read( idFile* file ) {
	int handle;
	file->ReadInt( handle );

	int taskInfoIndex;
	file->ReadInt( taskInfoIndex );

	int spawnId;
	file->ReadInt( spawnId );

	sdTaskManager::GetInstance().AllocTask( handle, gameLocal.declPlayerTaskType[ taskInfoIndex ], spawnId );
	sdPlayerTask* task = sdTaskManager::GetInstance().TaskForHandle( handle );
	assert( task );
	task->ReadInitialState( file );
}

/*
================
sdPlayerTask::WriteInitialState
================
*/
void sdPlayerTask::WriteInitialState( const sdReliableMessageClientInfoBase& target ) const {
	sdTaskMessage msg( this, TM_FULLSTATE );
	msg.WriteBits( _taskInfo->Index(), idMath::BitsForInteger( gameLocal.declPlayerTaskType.Num() ) );
	msg.WriteLong( _entity.GetSpawnId() );
	msg.WriteBits( _flags, NUM_TASKFLAGS );

	for ( int i = 0; i < _wayPointInfo.Num(); i++ ) {
		msg.WriteBool( _wayPointInfo[ i ].fixed );
		msg.WriteBool( _wayPointInfo[ i ].enabled );
		if ( _wayPointInfo[ i ].fixed ) {
			msg.WriteVector( _wayPointInfo[ i ].offset );
		}
	}

	msg.Send( target );
}

/*
================
sdPlayerTask::ReadInitialState
================
*/
void sdPlayerTask::ReadInitialState( const idBitMsg& msg ) {
	_flags			= msg.ReadBits( NUM_TASKFLAGS );

	for ( int i = 0; i < _wayPointInfo.Num(); i++ ) {
		_wayPointInfo[ i ].fixed = msg.ReadBool();
		_wayPointInfo[ i ].enabled = msg.ReadBool();
		if ( _wayPointInfo[ i ].fixed ) {
			_wayPointInfo[ i ].offset = msg.ReadVector();
		}
	}
}

/*
================
sdPlayerTask::ReadInitialState
================
*/
void sdPlayerTask::ReadInitialState( idFile* file ) {
	file->ReadInt( _flags );

	int count;
	file->ReadInt( count );
	_wayPointInfo.SetNum( count );
	for ( int i = 0; i < count; i++ ) {
		file->ReadBool( _wayPointInfo[ i ].enabled );
		file->ReadBool( _wayPointInfo[ i ].fixed );
		file->ReadVec3( _wayPointInfo[ i ].offset );
	}
}

/*
================
sdPlayerTask::FlashIcon
================
*/
void sdPlayerTask::FlashIcon( int time ) {
	for ( int i = 0; i < _wayPointInfo.Num(); i++ ) {
		if ( _wayPointInfo[ i ].wayPoint != NULL ) {
			_wayPointInfo[ i ].wayPoint->SetFlashEndTime( gameLocal.ToGuiTime( gameLocal.time ) + time );
		}
	}
}

/*
================
sdPlayerTask::Event_Complete
================
*/
void sdPlayerTask::Event_Complete( void ) {
	if ( gameLocal.isClient ) {
		assert( false );
		return;
	}
	SetComplete();
}

/*
================
sdPlayerTask::Event_SetTimeout
================
*/
void sdPlayerTask::Event_SetTimeout( float duration ) {
	if ( gameLocal.isClient ) {
		assert( false );
		return;
	}
	SetTimeout( SEC2MS( duration ) );
}

/*
================
sdPlayerTask::Event_Free
================
*/
void sdPlayerTask::Event_Free( void ) {
	if ( gameLocal.isClient ) {
		assert( false );
		return;
	}
	sdTaskManager::GetInstance().FreeTask( _taskIndex );
}

/*
================
sdPlayerTask::Event_GetTaskEntity
================
*/
void sdPlayerTask::Event_GetTaskEntity( void ) {
	sdProgram::ReturnEntity( GetEntity() );
}

/*
================
sdPlayerTask::Event_SetTargetPos
================
*/
void sdPlayerTask::Event_SetTargetPos( int index, const idVec3& position ) {
	if ( gameLocal.isClient ) {
		assert( false );
		return;
	}

	if ( SetTargetPos( index, position ) ) {
		if ( gameLocal.isServer ) {
			sdTaskMessage msg( this, TM_SETLOCATIION );
			msg.WriteLong( index );
			msg.WriteVector( position );
			msg.Send( sdReliableMessageClientInfoAll() );
		}
	}
}

/*
================
sdPlayerTask::Event_SetWayPointState
================
*/
void sdPlayerTask::Event_SetWayPointState( int index, bool state ) {
	if ( gameLocal.isClient ) {
		assert( false );
		return;
	}

	if ( SetWayPointState( index, state ) ) {
		if ( gameLocal.isServer ) {
			sdTaskMessage msg( this, TM_VISSTATE );
			msg.WriteLong( index );
			msg.WriteBool( state );
			msg.Send( sdReliableMessageClientInfoAll() );
		}
	}
}

/*
================
sdPlayerTask::Event_GetKey
================
*/
void sdPlayerTask::Event_GetKey( const char *key ) {
	sdProgram::ReturnString( _taskInfo->GetData().GetString( key ) );
}

/*
================
sdPlayerTask::Event_GetIntKey
================
*/
void sdPlayerTask::Event_GetIntKey( const char *key ) {
	sdProgram::ReturnInteger( _taskInfo->GetData().GetInt( key ) );
}

/*
================
sdPlayerTask::Event_GetFloatKey
================
*/
void sdPlayerTask::Event_GetFloatKey( const char *key ) {
	sdProgram::ReturnFloat( _taskInfo->GetData().GetFloat( key ) );
}

/*
================
sdPlayerTask::Event_GetVectorKey
================
*/
void sdPlayerTask::Event_GetVectorKey( const char *key ) {
	sdProgram::ReturnVector( _taskInfo->GetData().GetVector( key ) );
}

/*
================
sdPlayerTask::Event_GetKeyWithDefault
================
*/
void sdPlayerTask::Event_GetKeyWithDefault( const char *key, const char* defaultvalue ) {
	sdProgram::ReturnString( _taskInfo->GetData().GetString( key, defaultvalue ) );
}

/*
================
sdPlayerTask::Event_GetIntKeyWithDefault
================
*/
void sdPlayerTask::Event_GetIntKeyWithDefault( const char *key, int defaultvalue ) {
	sdProgram::ReturnFloat( _taskInfo->GetData().GetInt( key, va( "%i", defaultvalue ) ) );
}

/*
================
sdPlayerTask::Event_GetFloatKeyWithDefault
================
*/
void sdPlayerTask::Event_GetFloatKeyWithDefault( const char *key, float defaultvalue ) {
	sdProgram::ReturnFloat( _taskInfo->GetData().GetFloat( key, va( "%f", defaultvalue ) ) );
}

/*
================
sdPlayerTask::Event_GetVectorKeyWithDefault
================
*/
void sdPlayerTask::Event_GetVectorKeyWithDefault( const char *key, const idVec3& defaultvalue ) {
	sdProgram::ReturnVector( _taskInfo->GetData().GetVector( key, defaultvalue.ToString() ) );
}

/*
================
sdPlayerTask::Event_GiveObjectiveProficiency
================
*/
void sdPlayerTask::Event_GiveObjectiveProficiency( float count, const char* reason ) {
	if ( !IsObjective() ) {
		return;
	}

	sdTeamInfo* team = _taskInfo->GetTeam();
	if ( team == NULL ) {
		return;
	}

	sdProficiencyManagerLocal& manager = sdProficiencyManager::GetInstance();

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( player == NULL ) {
			continue;
		}

		if ( player->GetGameTeam() != team ) {
			continue;
		}

		const sdDeclPlayerClass* cls = player->GetInventory().GetClass();
		if ( cls == NULL ) {
			continue;
		}

		if ( cls->GetNumProficiencies() < 1 ) {
			continue;
		}

		manager.GiveProficiency( cls->GetProficiency( 0 ).index, count, player, 1.f, reason );
	}
}

/*
================
sdPlayerTask::Event_SetUserCreated
================
*/
void sdPlayerTask::Event_SetUserCreated( void ) {
	if ( gameLocal.isClient ) {
		return;
	}

	SetUserCreated();
}

/*
================
sdPlayerTask::Event_IsUserCreated
================
*/
void sdPlayerTask::Event_IsUserCreated( void ) {
	sdProgram::ReturnBoolean( IsUserCreated() );
}

/*
================
sdPlayerTask::Event_FlashIcon
================
*/
void sdPlayerTask::Event_FlashIcon( int time ) {
	FlashIcon( time );
}

/*
===============================================================================

	sdTaskManagerLocal

===============================================================================
*/

const idEventDef EV_TaskManager_AllocEntityTask( "allocEntityTask", 'o', DOC_TEXT( "Allocates a mission and returns the object. If the allocation fails for any reason the result will be $null$." ), 2, "Task objects are of type $class:sdPlayerTask$.", "d", "index", "Index of the $decl:task$ to use.", "e", "owner", "Entity which the task will be associated with." );

CLASS_DECLARATION( idClass, sdTaskManagerLocal )
	EVENT( EV_TaskManager_AllocEntityTask,						sdTaskManagerLocal::Event_AllocEntityTask )
END_CLASS


/*
================
sdTaskManagerLocal::sdTaskManagerLocal
================
*/
sdTaskManagerLocal::sdTaskManagerLocal( void ) : _scriptObject( NULL ) {
}

/*
================
sdTaskManagerLocal::~sdTaskManagerLocal
================
*/
sdTaskManagerLocal::~sdTaskManagerLocal( void ) {
}

/*
================
sdTaskManagerLocal::Init
================
*/
void sdTaskManagerLocal::Init( void ) {
	Shutdown();

	_objectiveTasks.SetNum( sdTeamManager::GetInstance().GetNumTeams() );
	_taskList.SetNum( sdPlayerTask::MAX_TASKS );
	_freeTasks.SetNum( sdPlayerTask::MAX_TASKS );
	for ( int i = 0; i < sdPlayerTask::MAX_TASKS; i++ ) {
		_taskList[ i ] = NULL;
		_freeTasks[ i ] = sdPlayerTask::MAX_TASKS - i - 1;
	}
}

/*
================
sdTaskManagerLocal::Shutdown
================
*/
void sdTaskManagerLocal::Shutdown( void ) {
	CheckForLeaks();

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( player == NULL ) {
			continue;
		}

		player->GetTaskList().SetNum( 0 );
		player->SetManualTaskSelection( false );
	}

	_taskList.DeleteContents( true );
	_freeTasks.Clear();
	_objectiveTasks.SetNum( 0, false );
}

/*
================
sdTaskManagerLocal::OnNewTask
================
*/
void sdTaskManagerLocal::OnNewTask( sdPlayerTask* task ) {
	task->GetNode().AddToEnd( _activeTasks );

	if ( task->IsObjective() ) {
		sdTeamInfo* taskTeam = task->GetInfo()->GetTeam();
		if ( taskTeam != NULL ) {
			task->GetObjectiveNode().AddToEnd( _objectiveTasks[ taskTeam->GetIndex() ] );
			task->UpdateObjectiveWayPoint();
		}
	}

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( localPlayer != NULL ) {
		if ( task->IsAvailable( localPlayer ) ) {
			gameLocal.localPlayerProperties.OnNewTask( task );
		}
	}
}

/*
================
sdTaskManagerLocal::AllocTask
================
*/
sdPlayerTask* sdTaskManagerLocal::AllocTask( const sdDeclPlayerTask* taskInfo, idEntity* object ) {
	if ( gameLocal.isClient ) {
		return NULL;
	}

	if ( _freeTasks.Num() == 0 ) {
		gameLocal.Warning( "sdTaskManagerLocal::AllocTask No Free Tasks" );
		return NULL;
	}

	int index = _freeTasks[ _freeTasks.Num() - 1 ];
	_freeTasks.RemoveIndex( _freeTasks.Num() - 1 );

	assert( _taskList[ index ] == NULL );

	_taskList[ index ] = new sdPlayerTask();
	_taskList[ index ]->Create( index, taskInfo );
	_taskList[ index ]->SetEntity( gameLocal.GetSpawnId( object ) );
	_taskList[ index ]->WriteInitialState( sdReliableMessageClientInfoAll() );

	OnNewTask( _taskList[ index ] );

	return _taskList[ index ];
}

/*
================
sdTaskManagerLocal::AllocTask
================
*/
void sdTaskManagerLocal::AllocTask( taskHandle_t handle, const sdDeclPlayerTask* taskInfo, int spawnId ) {
	assert( _taskList[ handle ] == NULL );

	_taskList[ handle ] = new sdPlayerTask();
	_taskList[ handle ]->Create( handle, taskInfo );
	_taskList[ handle ]->SetEntity( spawnId );

	OnNewTask( _taskList[ handle ] );
}

/*
================
sdTaskManagerLocal::FreeTask
================
*/
void sdTaskManagerLocal::FreeTask( taskHandle_t handle ) {
	sdPlayerTask* task = TaskForHandle( handle );
	if ( !task ) {
		return;
	}

	if ( !task->IsComplete() ) {
		idPlayer* localPlayer = gameLocal.GetLocalPlayer();
		if ( localPlayer != NULL && localPlayer->GetActiveTaskHandle() == handle ) {
			gameLocal.localPlayerProperties.OnTaskExpired( task );
		}
	}

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( player == NULL ) {
			continue;
		}

		player->GetTaskList().Remove( task );

		if ( !gameLocal.isClient ) {
			if ( player->GetActiveTaskHandle() == handle ) {
				player->SetActiveTask( taskHandle_t() );
				player->SetManualTaskSelection( false );
			}
		}
	}

	int index = task->GetIndex();
	delete task;
	_taskList[ index ] = NULL;

	if ( !gameLocal.isClient ) {
		if ( _freeTasks.FindIndex( index ) != -1 ) {
			gameLocal.Warning( "sdTaskManagerLocal::FreeTask - Freed an already free task '%d'", index );
		} else {
			int* freeTask = _freeTasks.Alloc();
			if ( freeTask == NULL ) {
				gameLocal.Error( "sdTaskManagerLocal::FreeTask - free tasks exceeded MAX_TASKS" );
			}
			*freeTask = index;
		}
	}
}

/*
================
sdTaskManagerLocal::Write
================
*/
void sdTaskManagerLocal::Write( idFile* file ) const {
	file->WriteInt( _activeTasks.Num() );
	for ( sdPlayerTask* task = _activeTasks.Next(); task; task = task->GetNode().Next() ) {
		task->Write( file );
	}
}

/*
================
sdTaskManagerLocal::Read
================
*/
void sdTaskManagerLocal::Read( idFile* file ) {
	int count;
	file->ReadInt( count );

	for ( int i = 0; i < count; i++ ) {
		sdPlayerTask::Read( file );
	}
}

/*
================
sdTaskManagerLocal::IsTaskValid
================
*/
bool sdTaskManagerLocal::IsTaskValid( idPlayer* player, taskHandle_t taskHandle, bool onlyCheckInitialFT ) {
	if ( taskHandle < 0 || taskHandle >= sdPlayerTask::MAX_TASKS ) {
		return false;
	}

	sdPlayerTask* task = TaskForHandle( taskHandle );
	if ( task == NULL ) {
		return false;
	}

	sdFireTeam* fireTeam = gameLocal.rules->GetPlayerFireTeam( player->entityNumber );
	if ( fireTeam != NULL ) {
		if ( fireTeam->GetCommander() != player ) {
			return false;
		}

		if ( player->GetTaskList().FindIndex( task ) == -1 ) {
			bool pass = false;
			for ( int i = 0; i < fireTeam->GetNumMembers(); i++ ) {
				idPlayer* p = fireTeam->GetMember( i );
				if ( p == player ) {
					continue;
				}

				if ( onlyCheckInitialFT ) {
					if ( p->GetTaskList().Num() < 1 ) {
						continue;
					}

					if ( p->GetTaskList()[ 0 ] == task ) {
						pass = true;
						break;
					}
				} else {
					if ( p->GetTaskList().FindIndex( task ) != -1 ) {
						pass = true;
						break;
					}
				}
			}

			if ( !pass ) {
				return false;
			}
		}
	} else {
		if ( player->GetTaskList().FindIndex( task ) == -1 ) {
			return false;
		}
	}

	return true;
}

/*
================
sdTaskManagerLocal::SelectTask
================
*/
void sdTaskManagerLocal::SelectTask( idPlayer* player, taskHandle_t taskHandle ) {
	if ( !taskHandle.IsValid() ) { // Gordon: always let the player choose "no task" == objective
		player->SetActiveTask( taskHandle );
		player->SetManualTaskSelection( true );
		return;
	}

	if ( player->GetActiveTaskHandle() == taskHandle ) {
		return;
	}

	if ( !IsTaskValid( player, taskHandle, true ) ) {
		return;
	}

	player->SetActiveTask( taskHandle );
	player->SetManualTaskSelection( true );
}

/*
================
sdTaskManagerLocal::WriteInitialReliableMessages
================
*/
void sdTaskManagerLocal::WriteInitialReliableMessages( const sdReliableMessageClientInfoBase& target ) const {
	for ( sdPlayerTask* task = _activeTasks.Next(); task; task = task->GetNode().Next() ) {
		task->WriteInitialState( target );
	}
}

idCVar g_debugWorldTasks( "g_debugWorldTasks", "0", CVAR_BOOL | CVAR_CHEAT, "draws a sphere around ranged based tasks" );

/*
================
sdTaskManagerLocal::Think
================
*/
void sdTaskManagerLocal::Think( void ) {
	if ( !gameLocal.isClient ) {
		sdPlayerTask* task;
		sdPlayerTask* nextTask;
		for ( task = _activeTasks.Next(); task; task = nextTask ) {
			nextTask = task->GetNode().Next();

			int endTime = task->GetEndTime();
			if ( endTime >= 0 && gameLocal.time > endTime && !( task->IsObjective() || task->IsMission() ) ) {
				FreeTask( task->GetHandle() );
				continue;
			}

			int timeLimit = task->GetInfo()->GetTimeLimit();
			if ( timeLimit != -1 && !( task->IsObjective() || task->IsMission() ) ) {
				endTime = task->GetStartTime() + timeLimit;
				if ( gameLocal.time > endTime ) {
					FreeTask( task->GetHandle() );
					continue;
				}
			}
		}

		sdTeamManagerLocal& manager = sdTeamManager::GetInstance();

		idEntity* entity;
		sdTaskInterface* iface;
		for ( entity = _taskEntities.Next(); entity; entity = iface->GetNode().Next() ) {
			iface = entity->GetTaskInterface();

			if ( entity->fl.spotted ) {
				continue;
			}

			sdTeamInfo* team = entity->GetGameTeam();
			if ( team == NULL ) {
				continue;
			}

			const idVec3& origin = entity->GetPhysics()->GetOrigin();

			if ( team->PointInJammer( origin, RM_RADAR ) ) {
				continue;
			}

			for ( int i = 0; i < manager.GetNumTeams(); i++ ) {
				sdTeamInfo& otherTeam = manager.GetTeamByIndex( i );
				if ( otherTeam == *team ) {
					continue;
				}

				idEntity* other = otherTeam.PointInRadar( origin, RM_RADAR );
				if ( other == NULL ) {
					continue;
				}

				entity->SetSpotted( other );
				break;
			}
		}
	}

	for ( sdPlayerTask* task = _activeTasks.Next(); task; task = task->GetNode().Next() ) {
		task->Think();
	}

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	sdFireTeam* ft = NULL;
	if ( localPlayer != NULL ) {
		ft = gameLocal.rules->GetPlayerFireTeam( localPlayer->entityNumber );
		if ( ft && !ft->IsCommander( localPlayer ) ) {
			ft = NULL;
		}
	}
	
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( player == NULL ) {
			continue;
		}

		if ( gameLocal.isClient ) {
			if ( ft == NULL ) {
				if ( player != localPlayer ) {
					continue;
				}
			} else {
				if ( !ft->IsMember( player ) ) {
					continue;
				}
			}
		}

		playerTaskList_t& taskList = player->GetTaskList();
		taskList.SetNum( 0 );

		for ( sdPlayerTask* task = _activeTasks.Next(); task; task = task->GetNode().Next() ) {
			bool isAvailable = task->IsAvailable( player );
			if ( isAvailable ) {
				sdPlayerTask** newTask = taskList.Alloc();
				if ( newTask == NULL ) {
					gameLocal.Error( "sdTaskManagerLocal::Think - taskList exceeded MAX_TASKS" );
				}

				*newTask = task;

				if ( player == localPlayer ) {
					float currentRange = 0.f;
					idEntity* other = task->GetEntity();
					if ( other != NULL ) {
						currentRange = idMath::Sqrt( gameLocal.RangeSquare( other, player ) );
					}
					task->SetCurrentRange( currentRange );
				}
			}

			if ( player == localPlayer ) {
				if ( task->HasEligibleWayPoint() ) {
					if ( isAvailable || task->IsPlayerEligible( player ) ) {
						task->ShowWayPoint();
					} else {
						task->HideWayPoint();
					}
				}
			}
		}

		taskList.Sort( SortTasksByPriority );

		if ( !gameLocal.isClient && !player->userInfo.isBot ) {
			bool canSelectTask = true;
			sdFireTeam* fireTeam = gameLocal.rules->GetPlayerFireTeam( player->entityNumber );
			if ( fireTeam != NULL ) {
				canSelectTask = fireTeam->GetCommander() == player;
			}

			if ( canSelectTask && !player->GetManualTaskSelection() ) {
				if ( player->GetActiveTask() == NULL ) {
					if ( taskList.Num() > 0 ) {
						player->SetActiveTask( taskList[ 0 ]->GetHandle() );
					}
				}
			}
		}
	}

	if ( !gameLocal.isClient ) {
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			idPlayer* player = gameLocal.GetClient( i );
			if ( player == NULL ) {
				continue;
			}

			taskHandle_t taskHandle = player->GetActiveTaskHandle();
			if ( taskHandle.IsValid() ) {
				if ( !IsTaskValid( player, taskHandle, false ) ) {
					player->SetActiveTask( taskHandle_t() );
					player->SetManualTaskSelection( false );
				}
			}
		}
	}
}

/*
================
sdTaskManagerLocal::AddTaskEntity
================
*/
void sdTaskManagerLocal::AddTaskEntity( idLinkList< idEntity >& entity ) {
	entity.AddToEnd( _taskEntities );
}

/*
================
sdTaskManagerLocal::SortTasksByPriority
================
*/
int sdTaskManagerLocal::SortTasksByPriority( const taskPtr_t* a, const taskPtr_t* b ) {
	int diff = ( *a )->GetPriority() - ( *b )->GetPriority();
	if ( diff != 0 ) {
		return diff;
	}

	diff = idMath::Ftoi( idMath::Ceil( ( *a )->GetCurrentRange() - ( *b )->GetCurrentRange() ) );
	return diff;
}

/*
================
sdTaskManagerLocal::CheckForLeaks
================
*/
void sdTaskManagerLocal::CheckForLeaks( void ) {
#if 0
	for ( sdPlayerTask* task = _activeTasks.Next(); task; task = task->GetNode().Next() ) {
		gameLocal.Printf( "Task '%ls' not freed\n", task->GetTitle() );
	}
#endif // 0
}

/*
================
sdTaskManagerLocal::OnNewScriptLoad
================
*/
void sdTaskManagerLocal::OnNewScriptLoad( void ) {
	_scriptObject = gameLocal.program->AllocScriptObject( this, "taskManagerType" );

	sdScriptHelper h1;
	_scriptObject->CallNonBlockingScriptEvent( _scriptObject->GetPreConstructor(), h1 );

	const sdProgram::sdFunction* constructor = _scriptObject->GetConstructor();
	if ( constructor ) {
		sdProgramThread* scriptThread = gameLocal.program->CreateThread();
		scriptThread->SetName( "sdTaskManagerLocal" );
		scriptThread->CallFunction( _scriptObject, constructor );
		scriptThread->DelayedStart( 0 );
		scriptThread->GetAutoNode().AddToEnd( _scriptObject->GetAutoThreads() );
	}
}


/*
================
sdTaskManagerLocal::OnMapStart
================
*/
void sdTaskManagerLocal::OnMapStart( void ) {
	if ( _scriptObject ) {
		const sdProgram::sdFunction* callback = _scriptObject->GetFunction( "OnMapStart" );
		if ( callback ) {
			sdProgramThread* scriptThread = gameLocal.program->CreateThread();
			scriptThread->SetName( "sdTaskManagerLocal" );
			scriptThread->CallFunction( _scriptObject, callback );
			scriptThread->DelayedStart( 0 );
			scriptThread->GetAutoNode().AddToEnd( _scriptObject->GetAutoThreads() );
		}
	}
}

/*
================
sdTaskManagerLocal::OnMapShutdown
================
*/
void sdTaskManagerLocal::OnMapShutdown( void ) {
	if ( _scriptObject ) {
		sdScriptHelper h1;
		_scriptObject->CallNonBlockingScriptEvent( _scriptObject->GetFunction( "OnMapShutdown" ), h1 );
	}

	Shutdown();
}

/*
================
sdTaskManagerLocal::OnScriptChange
================
*/
void sdTaskManagerLocal::OnScriptChange( void ) {
	if ( _scriptObject ) {
		sdScriptHelper h1;
		_scriptObject->CallNonBlockingScriptEvent( _scriptObject->GetDestructor(), h1 );

		gameLocal.program->FreeScriptObject( _scriptObject );
	}
}

/*
============
sdTaskManagerLocal::Event_AllocEntityTask
============
*/
void sdTaskManagerLocal::Event_AllocEntityTask( int taskIndex, idEntity* object ) {
	const sdDeclPlayerTask* taskInfo = gameLocal.declPlayerTaskType[ taskIndex ];
	if ( !taskInfo ) {
		sdProgram::ReturnObject( NULL );
		return;
	}

	sdPlayerTask* task = AllocTask( taskInfo, object );
	if ( !task ) {
		sdProgram::ReturnObject( NULL );
		return;
	}
	sdProgram::ReturnObject( task->GetScriptObject() );
}

/*
============
sdTaskManagerLocal::BuildTaskList
============
*/
void sdTaskManagerLocal::BuildTaskList( idPlayer* player, playerTaskList_t& list ) {
	sdFireTeam* fireTeam = gameLocal.rules->GetPlayerFireTeam( player->entityNumber );
	if ( fireTeam != NULL ) {
		if ( fireTeam->GetCommander() == player ) {
			list = player->GetTaskList();
			for ( int i = 0; i < fireTeam->GetNumMembers(); i++ ) {
				idPlayer* other = fireTeam->GetMember( i );
				if ( other == player ) {
					continue;
				}

				if ( other->GetTaskList().Num() < 1 ) {
					continue;
				}

				list.AddUnique( other->GetTaskList()[ 0 ] );
			}
		}
	} else {
		list = player->GetTaskList();
	}
}

/*
============
sdTaskManagerLocal::GetObjectiveTasks
============
*/
const sdPlayerTask::nodeType_t& sdTaskManagerLocal::GetObjectiveTasks( const sdTeamInfo* team ) {
	return _objectiveTasks[ team->GetIndex() ];
}

/*
============
sdTaskManagerLocal::OnLocalTeamChanged
============
*/
void sdTaskManagerLocal::OnLocalTeamChanged( void ) {
	for ( int i = 0; i < _objectiveTasks.Num(); i++ ) {
		sdPlayerTask::nodeType_t& node = _objectiveTasks[ i ];
		for ( sdPlayerTask* task = node.Next(); task != NULL; task = task->GetObjectiveNode().Next() ) {
			task->UpdateObjectiveWayPoint();
		}
	}
}
