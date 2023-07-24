// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Door.h"
#include "../Player.h"
#include "../script/Script_Helper.h"
#include "../script/Script_ScriptObject.h"
#include "../ContentMask.h"

idList< doorSpawnInfo_t* > idDoor::s_doorInfo;

/*
===============================================================================

	sdDoorPhysicsNetworkData

===============================================================================
*/

/*
================
sdDoorPhysicsNetworkData::MakeDefault
================
*/
void sdDoorPhysicsNetworkData::MakeDefault( void ) {
	currentPos	= 0.f;
	closing		= true;
}

/*
================
sdDoorPhysicsNetworkData::Write
================
*/
void sdDoorPhysicsNetworkData::Write( idFile* file ) const {
	file->WriteFloat( currentPos );
	file->WriteBool( closing );
}

/*
================
sdDoorPhysicsNetworkData::Read
================
*/
void sdDoorPhysicsNetworkData::Read( idFile* file ) {
	file->ReadFloat( currentPos );
	file->ReadBool( closing );
}

/*
===============================================================================

	sdPhysics_Door

===============================================================================
*/

/*
================
sdPhysics_Door::sdPhysics_Door
================
*/
sdPhysics_Door::sdPhysics_Door( void ) {
	currentPos			= 0.f;
	destPos				= 0.f;
	speed				= 0.25f;
}

/*
================
sdPhysics_Door::~sdPhysics_Door
================
*/
sdPhysics_Door::~sdPhysics_Door( void ) {
	for ( int i = 0; i < bodies.Num(); i++ ) {
		gameLocal.clip.DeleteClipModel( bodies[ i ].clipModel );
	}
}

/*
================
sdPhysics_Door::Open
================
*/
bool sdPhysics_Door::Open( void ) {
	if ( destPos == 1.f ) {
		return false;
	}

	destPos = 1.f;

	self->BecomeActive( TH_PHYSICS );
	self->StartSound( "snd_open", SND_DOOR, 0, NULL );

	return true;
}

/*
================
sdPhysics_Door::Close
================
*/
bool sdPhysics_Door::Close( void ) {
	if ( destPos == 0.f ) {
		return false;
	}

	destPos = 0.f;

	self->BecomeActive( TH_PHYSICS );
	self->StartSound( "snd_close", SND_DOOR, 0, NULL );

	return true;
}

/*
================
sdPhysics_Door::SetBodyPositions
================
*/
void sdPhysics_Door::SetBodyProperties( int id, const idVec3& _pos1, const idVec3& _pos2, const idMat3& _axes, bool pusher ) {
	assert( id >= 0 && id < bodies.Num() );
	if ( id < 0 || id >= bodies.Num() ) {		
		return;
	}

	bodies[ id ].pos1 = _pos1;
	bodies[ id ].pos2 = _pos2;
	bodies[ id ].axes = _axes;
	bodies[ id ].pusher = pusher;

	UpdateBodyPosition( id );
}

/*
================
sdPhysics_Door::GetNumClipModels
================
*/
int sdPhysics_Door::GetNumClipModels( void ) const {
	return bodies.Num();
}

/*
================
sdPhysics_Door::GetOrigin
================
*/
const idVec3& sdPhysics_Door::GetOrigin( int id ) const {
	assert( id >= 0 && id < bodies.Num() );

	if ( id < 0 || id >= bodies.Num() ) {		
		return vec3_origin;
	}

	return bodies[ id ].currentOrigin;
}

/*
================
sdPhysics_Door::Evaluate
================
*/
bool sdPhysics_Door::Evaluate( int timeStepMSec, int endTimeMSec ) {
	if ( currentPos == destPos ) {
		return false;
	}

	float oldPos = currentPos;

	float diff = MS2SEC( timeStepMSec ) * speed;
	float newPos;

	if ( currentPos < destPos ) {
		newPos = currentPos + diff;
		if ( newPos > destPos ) {
			newPos = destPos;
		}
	} else {
		newPos = currentPos - diff;
		if ( newPos < destPos ) {
			newPos = destPos;
		}
	}

	float distance = newPos - oldPos;

	bool collision = false;

	self->OnMoveStarted();

	trace_t tr;
	int collideEntityNum = 0;
	for ( int i = 0; i < bodies.Num(); i++ ) {
		body_t& doorBody = bodies[ i ];
		idClipModel* cm = bodies[ i ].clipModel;

		if ( !doorBody.pusher ) {
			// not a pusher, so just try moving in that direction and stop at the end position
			idVec3 end = Lerp( bodies[ i ].pos1, bodies[ i ].pos2, newPos );

			if ( gameLocal.clip.TranslationEntities( CLIP_DEBUG_PARMS tr, cm->GetOrigin(), end, cm, cm->GetAxis(), bodies[ i ].clipMask, self ) ) {
				distance *= tr.fraction;
				newPos = oldPos + distance;
				collision = true;
				collideEntityNum = tr.c.entityNum;
			}
		} else {
			// pusher, so push through the move
			idVec3 oldOrigin = bodies[ i ].currentOrigin;
			idVec3 newOrigin = Lerp( bodies[ i ].pos1, bodies[ i ].pos2, newPos );
			idMat3 axis = GetAxis( i );

			int pushFlags = PUSHFL_CLIP|PUSHFL_NOGROUNDENTITIES; //|PUSHFL_APPLYIMPULSE;
			gameLocal.push.ClipPush( tr, self, pushFlags, oldOrigin, axis, newOrigin, axis, bodies[ i ].clipModel );
			if ( tr.fraction < 1.0f ) {
				distance *= tr.fraction;
				newPos = oldPos + distance;
				collision = true;
				collideEntityNum = tr.c.entityNum;
			} else {
				newPos = ( newOrigin - bodies[ i ].pos1 ).Length();
				float pathLength = ( bodies[ i ].pos2 - bodies[ i ].pos1 ).Length();
				if ( pathLength > idMath::FLT_EPSILON ) {
					newPos /= pathLength;
				}
			}
		}

		// re-link all the clip models that have moved so far using the new position
		// otherwise they could potentially move into blocking entities		
		currentPos = newPos;
		for ( int j = 0; j <= i; j++ ) {
			UpdateBodyPosition( j );
		}
	}

	// Gordon: as currentPos may have been modified above, reset it back so SetCurrentPos works properly
	currentPos = oldPos;

	self->OnMoveFinished();

	SetCurrentPos( newPos );

	if ( collision ) {
		self->OnTeamBlocked( self, gameLocal.entities[ collideEntityNum ] );
	}

	return currentPos != oldPos;
}

/*
================
sdPhysics_Door::SetCurrentPos
================
*/
bool sdPhysics_Door::SetCurrentPos( float newPos ) {
	if ( currentPos == newPos ) {
		return false;
	}

	currentPos = newPos;

	for ( int i = 0; i < bodies.Num(); i++ ) {
		UpdateBodyPosition( i );
	}

	if ( currentPos == destPos ) {
		self->BecomeInactive( TH_PHYSICS );
		self->ReachedPosition();
	} else {
		self->BecomeActive( TH_PHYSICS );
	}

	return true;
}

/*
================
sdPhysics_Door::UpdateBodyPosition
================
*/
void sdPhysics_Door::UpdateBodyPosition( int id ) {
	bodies[ id ].currentOrigin = Lerp( bodies[ id ].pos1, bodies[ id ].pos2, currentPos );
	bodies[ id ].clipModel->Link( gameLocal.clip, self, id, bodies[ id ].currentOrigin, bodies[ id ].axes );
}

/*
================
sdPhysics_Door::SetClipModel
================
*/
void sdPhysics_Door::SetClipModel( idClipModel *model, float density, int id, bool freeOld ) {
	assert( self );
	assert( model );					// we need a clip model
	assert( model->IsTraceModel() );	// and it should be a trace model
	assert( density > 0.0f );			// density should be valid
	assert( id >= 0 );

	int max = Max( id + 1, bodies.Num() );
	if ( max > bodies.Num() ) {
		bodies.AssureSize( max );
	}

	body_t& body		= bodies[ id ];
	if ( freeOld ) {
		if ( ( body.clipModel != NULL ) && ( body.clipModel != model ) ) {
			gameLocal.clip.DeleteClipModel( body.clipModel );
		}
	}

	body.axes			= mat3_identity;
	body.clipMask		= 0;
	body.clipModel		= model;
	body.currentOrigin	= vec3_origin;
	body.pos1			= vec3_origin;
	body.pos2			= vec3_origin;
}

/*
================
sdPhysics_Door::EvaluateContacts
================
*/
bool sdPhysics_Door::EvaluateContacts( CLIP_DEBUG_PARMS_DECLARATION_ONLY ) {
	return false;
}

/*
================
sdPhysics_Door::IsAtRest
================
*/
bool sdPhysics_Door::IsAtRest( void ) const {
	return currentPos == destPos;
}

/*
================
sdPhysics_Door::Pushable
================
*/
bool sdPhysics_Door::IsPushable( void ) const {
	return false;
}

/*
================
sdPhysics_Door::GetBounds
================
*/
const idBounds&	sdPhysics_Door::GetBounds( int id ) const {
	static idBounds bounds;
	bounds.Clear();

	if( id >= 0 && id < bodies.Num() ) {
		bounds = bodies[ id ].clipModel->GetBounds();
	} else {
		for( int i = 0; i < bodies.Num(); i++ ) {
			bounds += bodies[ i ].clipModel->GetBounds();
		}
	}

	return bounds;
}

/*
================
sdPhysics_Door::GetAbsBounds
================
*/
const idBounds&	sdPhysics_Door::GetAbsBounds( int id ) const {
	static idBounds bounds;
	bounds.Clear();

	if( id >= 0 && id < bodies.Num() ) {
		bounds = bodies[ id ].clipModel->GetAbsBounds();
	} else {
		for( int i = 0; i < bodies.Num(); i++ ) {
			bounds += bodies[ i ].clipModel->GetAbsBounds();
		}
	}

	return bounds;
}

/*
================
sdPhysics_Door::GetClipModel
================
*/
idClipModel* sdPhysics_Door::GetClipModel( int id ) const {
	assert( id >= 0 && id < bodies.Num() );

	if ( id < 0 || id >= bodies.Num() ) {		
		return NULL;
	}

	return bodies[ id ].clipModel;
}

/*
================
sdPhysics_Door::GetContents
================
*/
int sdPhysics_Door::GetContents( int id ) const {
	if ( id >= 0 && id < bodies.Num() ) {
		return bodies[ id ].clipModel->GetContents();
	}

	int contents = 0;
	for ( int i = 0; i < bodies.Num(); i++ ) {
		contents |= bodies[ i ].clipModel->GetContents();
	}
	return contents;	
}

/*
================
sdPhysics_Door::SetContents
================
*/
void sdPhysics_Door::SetContents( int contents, int id ) {
	assert( id >= 0 && id < bodies.Num() );

	if ( id < 0 || id >= bodies.Num() ) {		
		return;
	}

	bodies[ id ].clipModel->SetContents( contents );
}

/*
================
sdPhysics_Door::GetClipMask
================
*/
int sdPhysics_Door::GetClipMask( int id ) const {
	int contents = 0;

	if( id >= 0 && id < bodies.Num() ) {
		contents = bodies[ id ].clipMask;
	} else {
		for( int i = 0; i < bodies.Num(); i++ ) {
			contents |= bodies[ i ].clipMask;
		}
	}

	return contents;
}

/*
================
sdPhysics_Door::SetClipMask
================
*/
void sdPhysics_Door::SetClipMask( int mask, int id ) {
	assert( id >= 0 && id < bodies.Num() );

	if ( id < 0 || id >= bodies.Num() ) {		
		return;
	}

	bodies[ id ].clipMask = mask;
}

/*
================
sdPhysics_Door::GetAxis
================
*/
const idMat3& sdPhysics_Door::GetAxis( int id ) const {
	assert( id >= 0 && id < bodies.Num() );

	if ( id < 0 || id >= bodies.Num() ) {		
		return mat3_identity;
	}

	return bodies[ id ].axes;
}

/*
================
sdPhysics_Door::EnableClip
================
*/
void sdPhysics_Door::EnableClip( void ) {
	for ( int i = 0; i < bodies.Num(); i++ ) {
		bodies[ i ].clipModel->Enable();
	}
}

/*
================
sdPhysics_Door::DisableClip
================
*/
void sdPhysics_Door::DisableClip( bool activateContacting ) {
	for ( int i = 0; i < bodies.Num(); i++ ) {
		if ( activateContacting ) {
			WakeEntitiesContacting( self, bodies[ i ].clipModel );
		}
		bodies[ i ].clipModel->Disable();
	}
}

/*
================
sdPhysics_Door::CreateNetworkStructure
================
*/
sdEntityStateNetworkData* sdPhysics_Door::CreateNetworkStructure( networkStateMode_t mode ) const {
	if ( mode == NSM_VISIBLE ) {
		return new sdDoorPhysicsNetworkData();
	}
	return NULL;
}

/*
================
sdPhysics_Door::CheckNetworkStateChanges
================
*/
bool sdPhysics_Door::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_BASE( sdDoorPhysicsNetworkData );

		bool wantClose = destPos == 0.f;
		if ( wantClose != baseData.closing ) {
			return true;
		}
		NET_CHECK_FIELD( currentPos, currentPos );
	}

	return false;
}

/*
================
sdPhysics_Door::ApplyNetworkState
================
*/
void sdPhysics_Door::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_NEW( sdDoorPhysicsNetworkData );

		if ( newData.closing ) {
			Close();
		} else {
			Open();
		}
		if ( SetCurrentPos( newData.currentPos ) ) {
			self->UpdateVisuals();
		}
	}
}

/*
================
sdPhysics_Door::ReadNetworkState
================
*/
void sdPhysics_Door::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdDoorPhysicsNetworkData );

		newData.currentPos	= msg.ReadDeltaFloat( baseData.currentPos );
		newData.closing		= msg.ReadBool();
	}
}

/*
================
sdPhysics_Door::WriteNetworkState
================
*/
void sdPhysics_Door::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdDoorPhysicsNetworkData );

		newData.currentPos	= currentPos;
		newData.closing		= destPos == 0.f;

		msg.WriteDeltaFloat( baseData.currentPos, newData.currentPos );
		msg.WriteBool( newData.closing );
	}
}

/*
===============================================================================

	sdDoorNetworkData

===============================================================================
*/

/*
================
sdDoorNetworkData::MakeDefault
================
*/
void sdDoorNetworkData::MakeDefault( void ) {
	closeTime		= 0;

	sdScriptEntityNetworkData::MakeDefault();
}

/*
================
sdDoorNetworkData::Write
================
*/
void sdDoorNetworkData::Write( idFile* file ) const {
	file->WriteInt( closeTime );

	sdScriptEntityNetworkData::Write( file );
}

/*
================
sdDoorNetworkData::Read
================
*/
void sdDoorNetworkData::Read( idFile* file ) {
	file->ReadInt( closeTime );

	sdScriptEntityNetworkData::Read( file );
}

/*
===============================================================================

	idDoor

===============================================================================
*/

const idEventDef EV_Door_Open( "open", '\0', DOC_TEXT( "Opens the door, the door will naturally close again, if not in toggle mode." ), 0, "This will ignore any script checks which would normally keep the door locked." );
const idEventDef EV_Door_Close( "close", '\0', DOC_TEXT( "Closes the door." ), 0, "This will ignore any script checks which would normally keep the door locked." );
const idEventDef EV_Door_IsOpen( "isOpen", 'b', DOC_TEXT( "Returns whether the door is fully open or not." ), 0, NULL );
const idEventDef EV_Door_IsClosed( "isClosed", 'b', DOC_TEXT( "Returns whether the door is fully closed or not." ), 0, NULL );
const idEventDef EV_Door_IsOpening( "isOpening", 'b', DOC_TEXT( "Return whether the door is currently opening or not." ), 0, NULL );
const idEventDef EV_Door_IsClosing( "isClosing", 'b', DOC_TEXT( "Return whether the door is currently closing or not." ), 0, NULL );
extern const idEventDef EV_SetSkin;
extern const idEventDef EV_SetToggle;

CLASS_DECLARATION( sdScriptEntity, idDoor )
	EVENT( EV_Activate,					idDoor::Event_Activate )
	EVENT( EV_Door_Open,				idDoor::Event_Open )
	EVENT( EV_Door_Close,				idDoor::Event_Close )
	EVENT( EV_Door_IsOpen,				idDoor::Event_IsOpen )
	EVENT( EV_Door_IsClosed,			idDoor::Event_IsClosed )
	EVENT( EV_Door_IsOpening,			idDoor::Event_IsOpening )
	EVENT( EV_Door_IsClosing,			idDoor::Event_IsClosing )
	EVENT( EV_SetSkin,					idDoor::Event_SetSkin )
	EVENT( EV_SetToggle,				idDoor::Event_SetToggle )
END_CLASS

/*
================
idDoor::idDoor
================
*/
idDoor::idDoor( void ) {
	trigger				= NULL;
	sndTrigger			= NULL;
	nextSndTriggerTime	= 0;
	normalAxisIndex		= 0;
	closeTime			= 0;
	moving				= false;
	toggle				= false;
	reverseOnBlock		= true;
	isSlave				= false;
}

/*
================
idDoor::~idDoor
================
*/
idDoor::~idDoor( void ) {
	parts.DeleteContents( true );
	gameLocal.clip.DeleteClipModel( trigger );
	gameLocal.clip.DeleteClipModel( sndTrigger );
}

/*
================
idDoor::OnTeamBlocked
================
*/
void idDoor::OnTeamBlocked( idEntity *blockedEntity, idEntity *blockingEntity ) {
	if ( crushOnBlock ) {
		blockingEntity->Damage( NULL, NULL, vec3_zero, DAMAGE_FOR_NAME( "damage_mover_crush" ), 1.f, NULL );
		return;
	}

	if ( reverseOnBlock ) {
		if ( physicsObj.IsOpening() ) {
			Close();
			return;
		}

		if ( physicsObj.IsClosing() ) {
			Open();
			return;
		}
	}
}

/*
================
idDoor::VectorForDir
================
*/
void idDoor::VectorForDir( float angle, idVec3 &vec ) {
	idAngles ang;

	switch( ( int )angle ) {
	case DIR_UP :
		vec.Set( 0, 0, 1 );
		break;

	case DIR_DOWN :
		vec.Set( 0, 0, -1 );
		break;

	case DIR_LEFT :
		ang			= physicsObj.GetAxis().ToAngles();
		ang.pitch	= 0;
		ang.roll	= 0;
		ang.yaw		+= 90;
		vec			= ang.ToForward();
		break;

	case DIR_RIGHT :
		ang			= physicsObj.GetAxis().ToAngles();
		ang.pitch	= 0;
		ang.roll	= 0;
		ang.yaw		-= 90;
		vec			= ang.ToForward();
		break;

	case DIR_FORWARD :
		ang			= physicsObj.GetAxis().ToAngles();
		ang.pitch	= 0;
		ang.roll	= 0;
		vec			= ang.ToForward();
		break;

	case DIR_BACK :
		ang			= physicsObj.GetAxis().ToAngles();
		ang.pitch	= 0;
		ang.roll	= 0;
		ang.yaw		+= 180;
		vec			= ang.ToForward();
		break;

	case DIR_REL_UP :
		vec.Set( 0, 0, 1 );
		break;

	case DIR_REL_DOWN :
		vec.Set( 0, 0, -1 );
		break;

	case DIR_REL_LEFT :
		ang			= physicsObj.GetAxis().ToAngles();
		ang.ToVectors( NULL, &vec );
		vec *= -1;
		break;

	case DIR_REL_RIGHT :
		ang			= physicsObj.GetAxis().ToAngles();
		ang.ToVectors( NULL, &vec );
		break;

	case DIR_REL_FORWARD :
		ang			= physicsObj.GetAxis().ToAngles();
		vec			= ang.ToForward();
		break;

	case DIR_REL_BACK :
		ang			= physicsObj.GetAxis().ToAngles();
		vec			= ang.ToForward() * -1;
		break;

	default:
		ang.Set( 0, angle, 0 );
		vec			= GetWorldVector( ang.ToForward() );
		break;
	}
}

/*
================
idDoor::Spawn
================
*/
void idDoor::Spawn( void ) {
	baseOrg		= GetPhysics()->GetOrigin();
	baseAxis	= GetPhysics()->GetAxis();

	isSlave		= !spawnArgs.GetBool( "groupmaster" );

	maxHealth	= health = spawnArgs.GetInt( "health" );

	// default wait of 3 seconds
	waitTime = SEC2MS( spawnArgs.GetFloat( "wait", "3" ) );

	reverseOnBlock = spawnArgs.GetBool( "reverse_on_block", "1" );
	crushOnBlock = spawnArgs.GetBool( "crush_on_block", "1" );

	toggle = spawnArgs.GetBool( "toggle" );

	if ( spawnArgs.GetBool( "continuous" ) ) {
		PostEventSec( &EV_Activate, spawnArgs.GetFloat( "delay" ), this );
	}

	// sounds have a habit of stuttering when portals close, so make them unoccluded
	refSound.parms.soundShaderFlags |= SSF_NO_OCCLUSION;

	checkOpenFunc = scriptObject->GetFunction( "allowOpen" );

	BecomeActive( TH_THINK );

	int i;
	for ( i = 0; i < s_doorInfo.Num(); i++ ) {
		if ( !s_doorInfo[ i ]->name.Icmp( GetName() ) ) {
			break;
		}
	}

	if ( i == s_doorInfo.Num() ) {
		doorSpawnInfo_t* info = new doorSpawnInfo_t;

		info->renderEnt = renderEntity;
		CalcPositions( info->pos1, info->pos2 );
		info->axes		= GetPhysics()->GetAxis();
		GetTraceModel( info->trm );
		info->name		= GetName();
		info->group		= spawnArgs.GetString( "group" );
		info->pusher	= spawnArgs.GetBool( "pusher" );

		s_doorInfo.Alloc() = info;
	}
}

/*
================
idDoor::CalcPositions
================
*/
void idDoor::CalcPositions( idVec3& pos1, idVec3& pos2 ) {
	pos1 = GetPhysics()->GetOrigin();

	idVec3 moveDelta;
	if ( spawnArgs.GetVector( "move_delta", "0 0 0", moveDelta ) ) {
		bool absolute = spawnArgs.GetBool( "move_absolute", "0" );
		if( !absolute ) {
			pos2 = pos1 + moveDelta * GetPhysics()->GetAxis();
		} else {
			pos2 = pos1 + moveDelta;
		}
	} else {
		float dir;
		// get the direction to move
		if ( !spawnArgs.GetFloat( "movedir", "0", dir ) ) {
			// no movedir, so angle defines movement direction and not orientation,
			// a la oldschool Quake
			SetAngles( ang_zero );
			dir = spawnArgs.GetFloat( "angle" );
		} 

		idVec3 movedir;
		// jrad - support for all the other motion types
		VectorForDir( dir, movedir );
		// default lip of 8 units
		float lip = spawnArgs.GetFloat( "lip", "8" );

		// calculate second position
		idVec3 absMovedir;
		absMovedir[ 0 ] = idMath::Fabs( movedir[ 0 ] );
		absMovedir[ 1 ] = idMath::Fabs( movedir[ 1 ] );
		absMovedir[ 2 ] = idMath::Fabs( movedir[ 2 ] );
		idVec3 size = GetPhysics()->GetAbsBounds().Size();
		float distance = ( absMovedir * size ) - lip;
		pos2 = pos1 + distance * movedir;
	}
}

/*
================
idDoor::OnMasterReady
================
*/
void idDoor::OnMasterReady( void ) {
	idBounds absBounds = physicsObj.GetAbsBounds( -1 );
	portal.Init( absBounds );

	refSound.origin = absBounds.GetCenter();

	CalcTriggerBounds( 0.f, baseBounds );

	ClosePortals( false );

	if ( health ) {
		fl.takedamage = true;
	}

	const char* sndtemp = spawnArgs.GetString( "snd_locked" );
	if ( *sndtemp ) {
		SpawnSoundTrigger();
	}

	if ( !spawnArgs.GetBool( "no_touch" ) ) {
		// spawn trigger
		SpawnDoorTrigger();
	}
}

/*
================
idDoor::EnableClip
================
*/
void idDoor::EnableClip( void ) {
	if ( fl.forceDisableClip ) {
		return;
	}

	if ( trigger ) {
		LinkTrigger();
	}
	if ( sndTrigger ) {
		LinkSoundTrigger();
	}

	sdScriptEntity::EnableClip();
}

/*
================
idDoor::DisableClip
================
*/
void idDoor::DisableClip( bool activateContacting ) {
	if ( trigger != NULL ) {
		trigger->Unlink( gameLocal.clip );
	}
	if ( sndTrigger != NULL ) {
		sndTrigger->Unlink( gameLocal.clip );
	}

	sdScriptEntity::DisableClip( activateContacting );
}

/*
================
idDoor::Hide
================
*/
void idDoor::Hide( void ) {
	if ( IsHidden() ) {
		return;
	}

	fl.hidden = true;

	OpenPortals();

	for ( int i = 0; i < parts.Num(); i++ ) {
		parts[ i ]->Hide();
	}
}

/*
================
idDoor::Show
================
*/
void idDoor::Show( void ) {
	if ( !IsHidden() ) {
		return;
	}

	fl.hidden = false;

	if ( physicsObj.IsClosed() ) {
		ClosePortals( true );
	}

	for ( int i = 0; i < parts.Num(); i++ ) {
		parts[ i ]->Show();
	}
}

/*
================
idDoor::Use
================
*/
void idDoor::Use( idEntity *activator ) {
	activator = activator;

	if ( physicsObj.IsClosed() ) {
		Open();
		return;
	}

	if ( physicsObj.IsOpen() ) {
		if ( toggle ) {
			Close();
			return;
		}

		if ( waitTime < 0 ) {
			return;
		}

		// if all the way up, just delay before coming down
		SetCloseTime( gameLocal.time + waitTime );
		return;
	}

	// only partway down before reversing
	if ( physicsObj.IsClosing() ) {
		Open();
		return;
	}

	// only partway up before reversing
	if ( physicsObj.IsOpening() ) {
		Close();
		return;
	}
}

/*
================
idDoor::Open
================
*/
void idDoor::Open( void ) {
	if ( physicsObj.IsOpening() ) {
		// already there, or on the way
		return;
	}

	physicsObj.Open();

	if ( physicsObj.IsClosed() ) {
		// open areaportal
		OpenPortals();
		return;
	}
}

/*
================
idDoor::Close
================
*/
void idDoor::Close( void ) {
	if ( physicsObj.IsClosing() ) {
		// already there, or on the way
		return;
	}

	physicsObj.Close();
}

/*
================
idDoor::IsOpen
================
*/
bool idDoor::IsOpen( void ) const {
	return physicsObj.IsOpen();
}

/*
================
idDoor::IsPermanentlyOpen
================
*/
bool idDoor::IsPermanentlyOpen( void ) const {
	bool open = physicsObj.IsOpen();
	if ( open && waitTime < 0 ) {
		return true;
	}

	return false;
}

/*
======================
idDoor::CalcTriggerBounds

Calcs bounds for a trigger.
======================
*/
void idDoor::CalcTriggerBounds( float size, idBounds &bounds ) {
	// find the bounds of everything on the team
	bounds.Clear();

	const idVec3& org = baseOrg;
	idMat3 transpose = baseAxis.Transpose();
	
	if ( health ) {
		fl.takedamage = true;
	}

	for ( int i = 0; i < parts.Num(); i++ ) {
		const idVec3& otherOrg	= physicsObj.GetOrigin( i );
		const idMat3& otherAxis = physicsObj.GetAxis( i );
		
		idMat3 axisDiff = otherAxis * transpose;

		idVec3 diff = otherOrg - org;
		diff = diff * transpose;

		idBounds otherBounds = physicsObj.GetBounds( i );
		otherBounds.Translate( diff );

		idBounds addBounds;
		addBounds.FromTransformedBounds( otherBounds, diff, axisDiff );

		bounds.AddBounds( addBounds );
	}

	// find the thinnest axis, which will be the one we expand
	int best = 0;
	for ( int i = 1 ; i < 3 ; i++ ) {
		if ( bounds[ 1 ][ i ] - bounds[ 0 ][ i ] < bounds[ 1 ][ best ] - bounds[ 0 ][ best ] ) {
			best = i;
		}
	}
	normalAxisIndex = best;

	bounds[ 0 ][ best ] -= size;
	bounds[ 1 ][ best ] += size;
}

/*
======================
idDoor::LinkTrigger
======================
*/
void idDoor::LinkTrigger( void ) {
	trigger->Link( gameLocal.clip, this, TRIGGER_ID, baseOrg, baseAxis );
}

/*
======================
idDoor::SpawnDoorTrigger
======================
*/
void idDoor::SpawnDoorTrigger( void ) {
	if ( trigger ) {
		// already have a trigger, so don't spawn a new one.
		LinkTrigger();
		return;
	}

	float triggerSize = spawnArgs.GetFloat( "triggersize", "120" );

	idBounds bounds;
	CalcTriggerBounds( triggerSize, bounds );

	// create a trigger clip model
	trigger = new idClipModel( idTraceModel( bounds ), true );
	trigger->SetContents( CONTENTS_TRIGGER );
	trigger->SetPosition( baseOrg, baseAxis, gameLocal.clip );
	LinkTrigger();
}

/*
======================
idDoor::LinkSoundTrigger
======================
*/
void idDoor::LinkSoundTrigger( void ) {
	sndTrigger->Link( gameLocal.clip, this, SND_TRIGGER_ID, baseOrg, baseAxis );
}

/*
======================
idDoor::SpawnSoundTrigger
======================
*/
void idDoor::SpawnSoundTrigger( void ) {
	if ( sndTrigger ) {
		LinkSoundTrigger();
		return;
	}

	float triggerSize = spawnArgs.GetFloat( "triggersize", "120" );

	idBounds bounds;
	CalcTriggerBounds( triggerSize * 0.5f, bounds );

	// create a trigger clip model
	sndTrigger = new idClipModel( idTraceModel( bounds ), true );
	sndTrigger->SetContents( CONTENTS_TRIGGER );
	sndTrigger->SetPosition( baseOrg, baseAxis, gameLocal.clip );
	LinkSoundTrigger();
}

/*
================
idDoor::Script_AllowOpen
================
*/
bool idDoor::Script_AllowOpen( idEntity* other ) {
	sdScriptHelper helper;
	helper.Push( other->GetScriptObject() );
	return CallFloatNonBlockingScriptEvent( checkOpenFunc, helper ) != 0.f;
}

/*
================
idDoor::OnTouch
================
*/
void idDoor::OnTouch( idEntity *other, const trace_t& trace ) {
	if ( IsHidden() ) {
		return;
	}

	idPlayer* p = other->Cast< idPlayer >();
	if ( p && p->IsSpectating() ) {
		SpectatorTouch( p, trace );
		return;
	}

	if ( p != NULL && p->GetHealth() <= 0 ) {
		return;
	}

	bool hasRequirements = true;

	if ( checkOpenFunc && !Script_AllowOpen( other ) ) {
		hasRequirements = false;
	}

	if ( trigger && trace.c.id == trigger->GetId() ) {
		if ( hasRequirements && !physicsObj.IsOpening() ) {
			Use( other );
		}

		return;
	}
	
	if ( sndTrigger && trace.c.id == sndTrigger->GetId() ) {
		if ( !hasRequirements && gameLocal.time > nextSndTriggerTime ) {
			StartSound( "snd_locked", SND_ANY, 0, NULL );
			nextSndTriggerTime = gameLocal.time + SEC2MS( 10.f );
		}

		return;
	}
}

/*
================
idDoor::SpectatorTouch
================
*/
void idDoor::SpectatorTouch( idPlayer* p, const trace_t& trace ) {
	assert( p && p->IsSpectating() );

	if ( IsPermanentlyOpen() ) {
		return;
	}

	// use sndTrigger as it should always be smaller than trigger
	if ( sndTrigger && trace.c.id == sndTrigger->GetId() ) {

		idVec3 relativeOrg = ( trace.endpos - baseOrg ) * baseAxis.Transpose();

		const idBounds& bounds = sndTrigger->GetBounds();
		
		idVec3 translate = bounds.GetCenter();

		idVec3 playerSize = p->GetPhysics()->GetBounds().Size();
		playerSize.z = 0.f;

		if ( relativeOrg[ normalAxisIndex ] > bounds.GetCenter()[ normalAxisIndex ] ) {
			translate[ normalAxisIndex ] = bounds.GetMins()[ normalAxisIndex ];
			translate[ normalAxisIndex ] -= playerSize.Length();
		} else {
			translate[ normalAxisIndex ] = bounds.GetMaxs()[ normalAxisIndex ];
			translate[ normalAxisIndex ] += playerSize.Length();
		}

		translate = baseOrg + ( translate * baseAxis );

		p->SetOrigin( translate );
		p->lastSpectateTeleport = gameLocal.time;
	}
}

/*
================
idDoor::Event_Activate
================
*/
void idDoor::Event_Activate( idEntity *activator ) {
	Use( activator );
}

/*
================
idDoor::Event_Open
================
*/
void idDoor::Event_Open( void ) {
	Open();
}

/*
================
idDoor::Event_Close
================
*/
void idDoor::Event_Close( void ) {
	Close();
}

/*
================
idDoor::Event_IsOpen
================
*/
void idDoor::Event_IsOpen( void ) {
	sdProgram::ReturnBoolean( IsOpen() );
}

/*
================
idDoor::Event_IsClosed
================
*/
void idDoor::Event_IsClosed( void ) {
	sdProgram::ReturnBoolean( physicsObj.IsClosed() );
}

/*
================
idDoor::Event_IsOpening
================
*/
void idDoor::Event_IsOpening( void ) {
	sdProgram::ReturnBoolean( physicsObj.IsOpening() );
}

/*
================
idDoor::Event_IsClosing
================
*/
void idDoor::Event_IsClosing( void ) {
	sdProgram::ReturnBoolean( physicsObj.IsClosing() );
}

/*
================
idDoor::OpenPortals
================
*/
void idDoor::OpenPortals( void ) {
	if ( !portal.IsValid() ) {
		return;
	}

	portal.Open();

	if ( gameLocal.isServer ) {
		sdEntityBroadcastEvent msg( this, EVENT_PORTALSTATE );
		msg.WriteBool( true );
		msg.Send( true, sdReliableMessageClientInfoAll() );
	}
}

/*
================
idDoor::ClosePortals
================
*/
void idDoor::ClosePortals( bool force ) {
	if ( !portal.IsValid() ) {
		return;
	}

	if ( !force && IsHidden() ) {
		return;
	}

	portal.Close();

	if ( gameLocal.isServer ) {
		sdEntityBroadcastEvent msg( this, EVENT_PORTALSTATE );
		msg.WriteBool( false );
		msg.Send( true, sdReliableMessageClientInfoAll() );
	}
}

/*
================
idDoor::Event_OpenPortal

Sets the portal associtated with this door to be open
================
*/
void idDoor::Event_OpenPortal( void ) {
	OpenPortals();
}

/*
================
idDoor::Event_ForceClosePortal

Sets the portal associtated with this door to be closed
================
*/
void idDoor::Event_ForceClosePortal( void ) {
	ClosePortals( true );
}

/*
================
idDoor::Event_ClosePortal

Sets the portal associtated with this door to be closed
================
*/
void idDoor::Event_ClosePortal( void ) {
	ClosePortals( false );
}

/*
================
idDoor::GetTraceModel
================
*/
void idDoor::GetTraceModel( idTraceModel& trm ) {
	const char* clipModelName = GetClipModelName();
	if ( !gameLocal.clip.LoadTraceModel( clipModelName, trm ) ) {
		idClipModel *mdl = new idClipModel( clipModelName );
		trm.SetupBox( mdl->GetBounds() );
		gameLocal.clip.DeleteClipModel( mdl );
//		gameLocal.Warning( "idDoor '%s': cannot load trace model %s", name.c_str(), clipModelName );
	}
}

/*
================
idDoor::PostMapSpawn
================
*/
void idDoor::PostMapSpawn( void ) {
	sdScriptEntity::PostMapSpawn();

	if ( isSlave ) {
		if ( !gameLocal.isClient ) {
			PostEventMS( &EV_Remove, 0 );
		}
		return;
	}

	SetPhysics( &physicsObj );
	physicsObj.SetSelf( this );

	const char* groupName = spawnArgs.GetString( "group" );
	for ( int i = 0; i < s_doorInfo.Num(); i++ ) {
		doorSpawnInfo_t& info = *s_doorInfo[ i ];

		if ( ( !*groupName || info.group.Icmp( groupName ) ) && info.name.Icmp( GetName() ) ) {
			continue;
		}

		sdRenderEntityBundle* bundle = new sdRenderEntityBundle();
		bundle->Copy( info.renderEnt );
		bundle->Show();

		int newIndex = physicsObj.GetNumClipModels();

		physicsObj.SetClipModel( new idClipModel( info.trm, false ), 1.f, newIndex );
		physicsObj.SetBodyProperties( newIndex, info.pos1, info.pos2, info.axes, info.pusher );
		physicsObj.SetClipMask( MASK_PLAYERSOLID | CONTENTS_MONSTER, newIndex );
		physicsObj.SetContents( CONTENTS_SOLID, newIndex );

		parts.Alloc() = bundle;
	}

	float time;
	if ( !spawnArgs.GetFloat( "time", "1", time ) ) {
		idVec3 pos1, pos2;
		CalcPositions( pos1, pos2 );

		// default speed of 400
		float dist = ( pos2 - pos1 ).Length();
		float speed = spawnArgs.GetFloat( "speed", "400" );

		time = dist / speed;

		gameLocal.Warning( "idDoor::PostMapSpawn No Time Set, Using Speed Instead" );
	}

	physicsObj.SetSpeed( 1 / time );

	OnMasterReady();
}

/*
================
idDoor::ReachedPosition
================
*/
void idDoor::ReachedPosition( void ) {
	if ( physicsObj.IsOpen() ) {
		StartSound( "snd_opened", SND_DOOR, 0, NULL );
		if ( waitTime > 0 && !toggle ) {
			SetCloseTime( gameLocal.time + waitTime );
		}
		return;
	}

	if ( physicsObj.IsClosed() ) {
		StartSound( "snd_closed", SND_DOOR, 0, NULL );
		ClosePortals( false );
		return;
	}
}

/*
================
idDoor::Think
================
*/
void idDoor::Think( void ) {
	sdScriptEntity::Think();

	if ( closeTime != 0 && gameLocal.time > closeTime ) {
		Close();
		SetCloseTime( 0 );
	}
}

/*
================
idDoor::UpdateModelTransform
================
*/
void idDoor::UpdateModelTransform( void ) {
	for ( int i = 0; i < parts.Num(); i++ ) {
		renderEntity_t& rEnt = parts[ i ]->GetEntity();

		rEnt.axis	= physicsObj.GetAxis( i );
		rEnt.origin	= physicsObj.GetOrigin( i );
	}
}

/*
================
idDoor::Present
================
*/
void idDoor::Present( void ) {
	if ( ( thinkFlags & TH_UPDATEVISUALS ) == 0 ) {
		return;
	}
	BecomeInactive( TH_UPDATEVISUALS );

	OnUpdateVisuals();

	if ( IsHidden() ) {
		return;
	}

	for ( int i = 0; i < parts.Num(); i++ ) {
		parts[ i ]->Update();
	}
}

/*
================
idDoor::CanCollide
================
*/
bool idDoor::CanCollide( const idEntity* other, int traceId ) const {
	return !moving || other->GetPhysics()->IsPushable() || other->fl.forceDoorCollision;
}

/*
================
idDoor::OnNewMapLoad
================
*/
void idDoor::OnNewMapLoad( void ) {
	s_doorInfo.DeleteContents( true );
}

/*
================
idDoor::OnMapClear
================
*/
void idDoor::OnMapClear( void ) {
	s_doorInfo.DeleteContents( true );
}

/*
================
idDoor::SetCloseTime
================
*/
void idDoor::SetCloseTime( int time ) {
	closeTime = time;
	if ( closeTime != 0 ) {
		BecomeActive( TH_THINK );
	}
}

/*
================
idDoor::ApplyNetworkState
================
*/
void idDoor::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_NEW( sdDoorNetworkData );

		SetCloseTime( newData.closeTime );
	}

	sdScriptEntity::ApplyNetworkState( mode, newState );
}

/*
================
idDoor::ReadNetworkState
================
*/
void idDoor::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdDoorNetworkData );

		newData.closeTime = msg.ReadDeltaLong( baseData.closeTime );
	}

	sdScriptEntity::ReadNetworkState( mode, baseState, newState, msg );
}

/*
================
idDoor::WriteNetworkState
================
*/
void idDoor::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdDoorNetworkData );

		newData.closeTime = closeTime;

		msg.WriteDeltaLong( baseData.closeTime, newData.closeTime );
	}

	sdScriptEntity::WriteNetworkState( mode, baseState, newState, msg );
}

/*
================
idDoor::CheckNetworkStateChanges
================
*/
bool idDoor::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	if ( isSlave ) {
		return false;
	}

	if ( mode == NSM_VISIBLE ) {
		NET_GET_BASE( sdDoorNetworkData );

		NET_CHECK_FIELD( closeTime, closeTime );
	}

	return sdScriptEntity::CheckNetworkStateChanges( mode, baseState );
}

/*
================
idDoor::CreateNetworkStructure
================
*/
sdEntityStateNetworkData* idDoor::CreateNetworkStructure( networkStateMode_t mode ) const {
	if ( isSlave ) {
		return NULL;
	}

	if ( mode == NSM_VISIBLE ) {
		sdDoorNetworkData* newData = new sdDoorNetworkData();
		newData->physicsData = physicsObj.CreateNetworkStructure( mode );
		return newData;
	}
	if ( mode == NSM_BROADCAST ) {
		sdScriptEntityBroadcastData* newData = new sdScriptEntityBroadcastData();
		newData->physicsData = physicsObj.CreateNetworkStructure( mode );
		return newData;
	}
	return NULL;
}

/*
================
idDoor::WantsToThink
================
*/
bool idDoor::WantsToThink( void ) const {
	return closeTime != 0 || sdScriptEntity::WantsToThink();
}

/*
================
idDoor::Event_SetSkin
================
*/
void idDoor::Event_SetSkin( const char* skinname ) {
	const idDeclSkin* skin = *skinname ? declHolder.declSkinType.LocalFind( skinname ) : NULL;
	for( int i = 0; i < parts.Num(); i++ ) {
		parts[ i ]->GetEntity().customSkin = skin;
	}

	UpdateVisuals();
}

/*
================
idDoor::Event_SetToggle
================
*/
void idDoor::Event_SetToggle( bool t ) {
	toggle = t;
	closeTime = 0;
}

/*
================
idDoor::ClientReceiveEvent
================
*/
bool idDoor::ClientReceiveEvent( int event, int time, const idBitMsg& msg ) {
	switch ( event ) {
		case EVENT_PORTALSTATE: {
			bool state = msg.ReadBool();
			if ( state ) {
				OpenPortals();
			} else {
				ClosePortals( true );
			}
			return true;
		}
	}

	return sdScriptEntity::ClientReceiveEvent( event, time, msg );
}

/*
===============
idDoor::WriteDemoBaseData
==============
*/
void idDoor::WriteDemoBaseData( idFile* file ) const {
	idEntity::WriteDemoBaseData( file );

	file->WriteBool( portal.IsOpen() );
}

/*
===============
idDoor::ReadDemoBaseData
==============
*/
void idDoor::ReadDemoBaseData( idFile* file ) {
	idEntity::ReadDemoBaseData( file );

	bool state;
	file->ReadBool( state );
	if ( state ) {
		OpenPortals();
	} else {
		ClosePortals( true );
	}
}


/*
===============
idDoor::GetModelDefHandle
==============
*/
int	idDoor::GetModelDefHandle( int id ) {
	if ( id < 0 || id >= parts.Num() ) {
		return -1;
	} else {
		return parts[id]->GetHandle();
	}
}
