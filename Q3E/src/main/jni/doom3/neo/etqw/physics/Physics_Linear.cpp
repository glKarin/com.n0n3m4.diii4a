// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Physics_Linear.h"
#include "../Entity.h"

/*
================
sdPhysicsLinearBroadcastData::MakeDefault
================
*/
void sdPhysicsLinearBroadcastData::MakeDefault( void ) {
	atRest				= 0;
	localOrigin			= vec3_origin;
	extrapolationType	= EXTRAPOLATION_NONE;
	startTime			= 0;
	duration			= 0;
	startValue			= vec3_origin;
	speed				= vec3_zero;
	baseSpeed			= vec3_zero;
}

/*
================
sdPhysicsLinearBroadcastData::Write
================
*/
void sdPhysicsLinearBroadcastData::Write( idFile* file ) const {
	file->WriteInt( atRest );
	file->WriteVec3( localOrigin );
	file->WriteInt( extrapolationType );
	file->WriteInt( startTime );
	file->WriteInt( duration );
	file->WriteVec3( startValue );
	file->WriteVec3( speed );
	file->WriteVec3( baseSpeed );
}

/*
================
sdPhysicsLinearBroadcastData::Read
================
*/
void sdPhysicsLinearBroadcastData::Read( idFile* file ) {
	file->ReadInt( atRest );
	file->ReadVec3( localOrigin );

	int temp;
	file->ReadInt( temp );
	extrapolationType = static_cast< extrapolation_t >( temp );

	file->ReadInt( startTime );
	file->ReadInt( duration );
	file->ReadVec3( startValue );
	file->ReadVec3( speed );
	file->ReadVec3( baseSpeed );
}


CLASS_DECLARATION( idPhysics_Base, sdPhysics_Linear )
END_CLASS

/*
================
sdPhysics_Linear::Activate
================
*/
void sdPhysics_Linear::Activate( void ) {
	current.atRest = -1;
	self->BecomeActive( TH_PHYSICS );
}

/*
================
sdPhysics_Linear::TestIfAtRest
================
*/
bool sdPhysics_Linear::TestIfAtRest( void ) const {
	if ( ( current.linearExtrapolation.GetExtrapolationType() & ~EXTRAPOLATION_NOSTOP ) == EXTRAPOLATION_NONE ) {
		return true;
	}

	if ( !current.linearExtrapolation.IsDone( static_cast< float >( current.time ) ) ) {
		return false;
	}

	return true;
}

/*
================
sdPhysics_Linear::Rest
================
*/
void sdPhysics_Linear::Rest( void ) {
	current.atRest = gameLocal.time;
//	self->BecomeInactive( TH_PHYSICS );
}

/*
================
sdPhysics_Linear::sdPhysics_Linear
================
*/
sdPhysics_Linear::sdPhysics_Linear( void ) {

	current.time = gameLocal.time;
	current.atRest = -1;
	current.origin.Zero();
	current.localOrigin.Zero();
	current.linearExtrapolation.Init( 0, 0, vec3_zero, vec3_zero, vec3_zero, EXTRAPOLATION_NONE );

	axis.Identity();

	saved			= current;
	isPusher		= false;
	pushFlags		= 0;
	clipModel		= NULL;
	isBlocked		= false;
	hasMaster		= false;
	isOrientated	= false;

	memset( &pushResults, 0, sizeof( pushResults ) );
}

/*
================
sdPhysics_Linear::~sdPhysics_Linear
================
*/
sdPhysics_Linear::~sdPhysics_Linear( void ) {
	gameLocal.clip.DeleteClipModel( clipModel );
}


/*
================
sdPhysics_Linear::SetPusher
================
*/
void sdPhysics_Linear::SetPusher( int flags ) {
	assert( clipModel );
	isPusher = true;
	pushFlags = flags;
}

/*
================
sdPhysics_Linear::IsPusher
================
*/
bool sdPhysics_Linear::IsPusher( void ) const {
	return isPusher;
}

/*
================
sdPhysics_Linear::SetLinearExtrapolation
================
*/
void sdPhysics_Linear::SetLinearExtrapolation( extrapolation_t type, int time, int duration, const idVec3& base, const idVec3& speed, const idVec3& baseSpeed ) {
	current.time = gameLocal.time;
	current.linearExtrapolation.Init( time, duration, base, baseSpeed, speed, type );
	current.localOrigin = base;
	Activate();
}

/*
================
sdPhysics_Linear::GetLocalOrigin
================
*/
void sdPhysics_Linear::GetLocalOrigin( idVec3& curOrigin ) const {
	curOrigin = current.localOrigin;
}

/*
================
sdPhysics_Linear::SetClipModel
================
*/
void sdPhysics_Linear::SetClipModel( idClipModel *model, float density, int id, bool freeOld ) {

	assert( self );
	assert( model );

	if ( clipModel != NULL && clipModel != model && freeOld ) {
		gameLocal.clip.DeleteClipModel( clipModel );
	}
	clipModel = model;
	clipModel->Link( gameLocal.clip, self, 0, current.origin, axis );
}

/*
================
sdPhysics_Linear::GetClipModel
================
*/
idClipModel *sdPhysics_Linear::GetClipModel( int id ) const {
	return clipModel;
}

/*
================
sdPhysics_Linear::GetNumClipModels
================
*/
int sdPhysics_Linear::GetNumClipModels( void ) const {
	return ( clipModel != NULL );
}

/*
================
sdPhysics_Linear::SetMass
================
*/
void sdPhysics_Linear::SetMass( float mass, int id ) {
}

/*
================
sdPhysics_Linear::GetMass
================
*/
float sdPhysics_Linear::GetMass( int id ) const {
	return 0.0f;
}

/*
================
sdPhysics_Linear::SetClipMask
================
*/
void sdPhysics_Linear::SetContents( int contents, int id ) {
	if ( clipModel ) {
		clipModel->SetContents( contents );
	}
}

/*
================
sdPhysics_Linear::SetClipMask
================
*/
int sdPhysics_Linear::GetContents( int id ) const {
	if ( clipModel ) {
		return clipModel->GetContents();
	}
	return 0;
}

/*
================
sdPhysics_Linear::GetBounds
================
*/
const idBounds& sdPhysics_Linear::GetBounds( int id ) const {
	return clipModel ? clipModel->GetBounds() : idPhysics_Base::GetBounds();
}

/*
================
sdPhysics_Linear::GetAbsBounds
================
*/
const idBounds& sdPhysics_Linear::GetAbsBounds( int id ) const {
	return clipModel ? clipModel->GetAbsBounds() : idPhysics_Base::GetAbsBounds();
}

/*
================
sdPhysics_Linear::Evaluate
================
*/
bool sdPhysics_Linear::Evaluate( int timeStepMSec, int endTimeMSec ) {
	current.origin.FixDenormals();

	idVec3 oldLocalOrigin, oldOrigin, masterOrigin;
	idMat3 oldAxis, masterAxis;

	isBlocked		= false;
	oldLocalOrigin	= current.localOrigin;
	oldOrigin		= current.origin;
	oldAxis			= axis;

	current.localOrigin = current.linearExtrapolation.GetCurrentValue( endTimeMSec );
	current.origin		= current.localOrigin;

	if ( hasMaster ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		if ( masterAxis.IsRotated() ) {
			current.origin = current.origin * masterAxis + masterOrigin;
			if ( isOrientated ) {
				axis *= masterAxis;
			}
		} else {
			current.origin += masterOrigin;
		}
	}

	if ( isPusher && ( oldOrigin != current.origin ) ) {
		gameLocal.push.ClipPush( pushResults, self, pushFlags, oldOrigin, oldAxis, current.origin, axis, GetClipModel() );
		if ( pushResults.fraction < 1.0f ) {
			clipModel->Link( gameLocal.clip, self, 0, oldOrigin, oldAxis );
			current.localOrigin = oldLocalOrigin;
			current.origin		= oldOrigin;
			axis				= oldAxis;
			isBlocked			= true;
			return false;
		}
	}

	if ( clipModel ) {
		clipModel->Link( gameLocal.clip, self, 0, current.origin, axis );
	}

	current.time = endTimeMSec;

	if ( TestIfAtRest() ) {
		Rest();
	}

	return ( current.origin != oldOrigin );
}

/*
================
sdPhysics_Linear::UpdateTime
================
*/
void sdPhysics_Linear::UpdateTime( int endTimeMSec ) {
	int timeLeap = endTimeMSec - current.time;

	current.time = endTimeMSec;
	// move the trajectory start times to sync the trajectory with the current endTime
	current.linearExtrapolation.SetStartTime( current.linearExtrapolation.GetStartTime() + timeLeap );
}

/*
================
sdPhysics_Linear::GetTime
================
*/
int sdPhysics_Linear::GetTime( void ) const {
	return current.time;
}

/*
================
sdPhysics_Linear::IsAtRest
================
*/
bool sdPhysics_Linear::IsAtRest( void ) const {
	return current.atRest >= 0;
}

/*
================
sdPhysics_Linear::GetRestStartTime
================
*/
int sdPhysics_Linear::GetRestStartTime( void ) const {
	return current.atRest;
}

/*
================
sdPhysics_Linear::IsPushable
================
*/
bool sdPhysics_Linear::IsPushable( void ) const {
	return false;
}

/*
================
sdPhysics_Linear::SaveState
================
*/
void sdPhysics_Linear::SaveState( void ) {
	saved = current;
}

/*
================
sdPhysics_Linear::RestoreState
================
*/
void sdPhysics_Linear::RestoreState( void ) {

	current = saved;

	if ( clipModel ) {
		clipModel->Link( gameLocal.clip, self, 0, current.origin, axis );
	}
}

/*
================
sdPhysics_Linear::SetOrigin
================
*/
void sdPhysics_Linear::SetOrigin( const idVec3& newOrigin, int id ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	current.linearExtrapolation.SetStartValue( newOrigin );
	current.localOrigin = current.linearExtrapolation.GetCurrentValue( current.time );
	if ( hasMaster ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + current.localOrigin * masterAxis;
	} else {
		current.origin = current.localOrigin;
	}
	if ( clipModel ) {
		clipModel->Link( gameLocal.clip, self, 0, current.origin, axis );
	}
	Activate();
}

/*
================
sdPhysics_Linear::SetAxis
================
*/
void sdPhysics_Linear::SetAxis( const idMat3 &newAxis, int id ) {
	axis = newAxis;

	if ( hasMaster && isOrientated ) {
		idVec3 masterOrigin;
		idMat3 masterAxis;
		self->GetMasterPosition( masterOrigin, masterAxis );
		axis *= masterAxis;
	}

	if ( clipModel ) {
		clipModel->Link( gameLocal.clip, self, 0, current.origin, axis );
	}
	Activate();
}

/*
================
sdPhysics_Linear::Move
================
*/
void sdPhysics_Linear::Translate( const idVec3& translation, int id ) {
}

/*
================
sdPhysics_Linear::Rotate
================
*/
void sdPhysics_Linear::Rotate( const idRotation &rotation, int id ) {
}

/*
================
sdPhysics_Linear::GetOrigin
================
*/
const idVec3& sdPhysics_Linear::GetOrigin( int id ) const {
	return current.origin;
}

/*
================
sdPhysics_Linear::GetAxis
================
*/
const idMat3 &sdPhysics_Linear::GetAxis( int id ) const {
	return axis;
}

/*
================
sdPhysics_Linear::SetLinearVelocity
================
*/
void sdPhysics_Linear::SetLinearVelocity( const idVec3& newLinearVelocity, int id ) {
	SetLinearExtrapolation( extrapolation_t( EXTRAPOLATION_LINEAR | EXTRAPOLATION_NOSTOP ), gameLocal.time, 0, current.origin, newLinearVelocity, vec3_origin );
	Activate();
}

/*
================
sdPhysics_Linear::SetAngularVelocity
================
*/
void sdPhysics_Linear::SetAngularVelocity( const idVec3& newAngularVelocity, int id ) {
}

/*
================
sdPhysics_Linear::GetLinearVelocity
================
*/
const idVec3& sdPhysics_Linear::GetLinearVelocity( int id ) const {
	static idVec3 curLinearVelocity;
	curLinearVelocity = current.linearExtrapolation.GetCurrentSpeed( gameLocal.time );
	return curLinearVelocity;
}

/*
================
sdPhysics_Linear::GetAngularVelocity
================
*/
const idVec3& sdPhysics_Linear::GetAngularVelocity( int id ) const {
	return vec3_zero;
}

/*
================
sdPhysics_Linear::UnlinkClip
================
*/
void sdPhysics_Linear::UnlinkClip( void ) {
	if ( clipModel != NULL ) {
		clipModel->Unlink( gameLocal.clip );
	}
}

/*
================
sdPhysics_Linear::LinkClip
================
*/
void sdPhysics_Linear::LinkClip( void ) {
	if ( clipModel != NULL ) {
		clipModel->Link( gameLocal.clip, self, 0, current.origin, axis );
	}
}

/*
================
sdPhysics_Linear::DisableClip
================
*/
void sdPhysics_Linear::DisableClip( bool activateContacting ) {
	if ( clipModel != NULL ) {
		if ( activateContacting ) {
			WakeEntitiesContacting( self, clipModel );
		}
		clipModel->Disable();
	}
}

/*
================
sdPhysics_Linear::EnableClip
================
*/
void sdPhysics_Linear::EnableClip( void ) {
	if ( clipModel != NULL ) {
		clipModel->Enable();
	}
}

/*
================
sdPhysics_Linear::GetBlockingInfo
================
*/
const trace_t *sdPhysics_Linear::GetBlockingInfo( void ) const {
	return ( isBlocked ? &pushResults : NULL );
}

/*
================
sdPhysics_Linear::GetBlockingEntity
================
*/
idEntity *sdPhysics_Linear::GetBlockingEntity( void ) const {
	if ( isBlocked ) {
		return gameLocal.entities[ pushResults.c.entityNum ];
	}
	return NULL;
}

/*
================
sdPhysics_Linear::SetMaster
================
*/
void sdPhysics_Linear::SetMaster( idEntity *master, const bool orientated ) {
	if ( master ) {
		if ( !hasMaster ) {

			idVec3 masterOrigin;
			idMat3 masterAxis;

			// transform from world space to master space
			self->GetMasterPosition( masterOrigin, masterAxis );
			current.localOrigin = ( current.origin - masterOrigin ) * masterAxis.Transpose();
			current.linearExtrapolation.SetStartValue( current.localOrigin );

			hasMaster			= true;
			isOrientated		= orientated;
		}
	}
	else {
		if ( hasMaster ) {
			// transform from master space to world space
			current.localOrigin = current.origin;
			SetLinearExtrapolation( EXTRAPOLATION_NONE, 0, 0, current.origin, vec3_origin, vec3_origin );
			hasMaster = false;
		}
	}
}

/*
================
sdPhysics_Linear::GetLinearEndTime
================
*/
int sdPhysics_Linear::GetLinearEndTime( void ) const {
	return current.linearExtrapolation.GetEndTime();
}

/*
================
sdPhysics_Linear::CheckNetworkStateChanges
================
*/
bool sdPhysics_Linear::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_BASE( sdPhysicsLinearBroadcastData );

		if ( baseData.atRest != current.atRest ) {
			return true;
		}

		if ( baseData.baseSpeed != current.linearExtrapolation.GetBaseSpeed() ) {
			return true;
		}

		if ( baseData.duration != SEC2MS( current.linearExtrapolation.GetDuration() ) ) {
			return true;
		}

		if ( baseData.extrapolationType != current.linearExtrapolation.GetExtrapolationType() ) {
			return true;
		}

		if ( baseData.localOrigin != current.localOrigin ) {
			return true;
		}

		if ( baseData.speed != current.linearExtrapolation.GetSpeed() ) {
			return true;
		}

		if ( baseData.startTime != SEC2MS( current.linearExtrapolation.GetStartTime() ) ) {
			return true;
		}

		if ( baseData.startValue != current.linearExtrapolation.GetStartValue() ) {
			return true;
		}

		return false;
	}

	return false;
}

/*
================
sdPhysics_Linear::WriteNetworkState
================
*/
void sdPhysics_Linear::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdPhysicsLinearBroadcastData );

		// update state
		newData.atRest				= current.atRest;
		newData.localOrigin			= current.localOrigin;
		newData.baseSpeed			= current.linearExtrapolation.GetBaseSpeed();
		newData.duration			= SEC2MS( current.linearExtrapolation.GetDuration() );
		newData.extrapolationType	= current.linearExtrapolation.GetExtrapolationType();
		newData.speed				= current.linearExtrapolation.GetSpeed();
		newData.startTime			= SEC2MS( current.linearExtrapolation.GetStartTime() );
		newData.startValue			= current.linearExtrapolation.GetStartValue();

		// write state
		msg.WriteDeltaLong( baseData.atRest, newData.atRest );
		msg.WriteDeltaVector( baseData.localOrigin, newData.localOrigin );
		msg.WriteDeltaVector( baseData.baseSpeed, newData.baseSpeed );
		msg.WriteDeltaLong( baseData.duration, newData.duration );
		msg.WriteDelta( baseData.extrapolationType, newData.extrapolationType, 8 );
		msg.WriteDeltaVector( baseData.speed, newData.speed );
		msg.WriteDeltaLong( baseData.startTime, newData.startTime );
		msg.WriteDeltaVector( baseData.startValue, newData.startValue );

		return;
	}
}

/*
================
sdPhysics_Linear::ApplyNetworkState
================
*/
void sdPhysics_Linear::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	traceCollection.ForceNextUpdate();
	if ( mode == NSM_BROADCAST ) {
		NET_GET_NEW( sdPhysicsLinearBroadcastData );

		// update state
		current.atRest				= newData.atRest;
		current.localOrigin			= newData.localOrigin;

		current.linearExtrapolation.Init( MS2SEC( newData.startTime ), MS2SEC( newData.duration ), newData.startValue, 
											newData.baseSpeed, newData.speed, newData.extrapolationType );

		self->UpdateVisuals();

		return;
	}
}

/*
================
sdPhysics_Linear::ReadNetworkState
================
*/
void sdPhysics_Linear::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdPhysicsLinearBroadcastData );

		// read state
		newData.atRest				= msg.ReadDeltaLong( baseData.atRest );
		newData.localOrigin			= msg.ReadDeltaVector( baseData.localOrigin );
		newData.baseSpeed			= msg.ReadDeltaVector( baseData.baseSpeed );
		newData.duration			= msg.ReadDeltaLong( baseData.duration );
		newData.extrapolationType	= ( extrapolation_t )msg.ReadDelta( baseData.extrapolationType, 8 );
		newData.speed				= msg.ReadDeltaVector( baseData.speed );
		newData.startTime			= msg.ReadDeltaLong( baseData.startTime );
		newData.startValue			= msg.ReadDeltaVector( baseData.startValue );

		return;
	}
}

/*
================
sdPhysics_Linear::CreateNetworkStructure
================
*/
sdEntityStateNetworkData* sdPhysics_Linear::CreateNetworkStructure( networkStateMode_t mode ) const {
	if ( mode == NSM_BROADCAST ) {
		return new sdPhysicsLinearBroadcastData();
	}
	return NULL;
}
