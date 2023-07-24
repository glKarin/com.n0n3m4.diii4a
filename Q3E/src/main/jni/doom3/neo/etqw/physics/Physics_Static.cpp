// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Physics_Static.h"
#include "../Entity.h"
#include "Force.h"
#include "Clip.h"
#include "../ContentMask.h"

CLASS_DECLARATION( idPhysics, idPhysics_Static )
END_CLASS

/*
================
sdPhysicsStaticNetworkData::MakeDefault
================
*/
void sdPhysicsStaticNetworkData::MakeDefault( void ) {
	position		= vec3_origin;
	orientation.x	= 0.f;
	orientation.y	= 0.f;
	orientation.z	= 0.f;
}

/*
================
sdPhysicsStaticNetworkData::Write
================
*/
void sdPhysicsStaticNetworkData::Write( idFile* file ) const {
	file->WriteVec3( position );
	file->WriteCQuat( orientation );
}

/*
================
sdPhysicsStaticNetworkData::Read
================
*/
void sdPhysicsStaticNetworkData::Read( idFile* file ) {
	file->ReadVec3( position );
	file->ReadCQuat( orientation );
}


/*
================
sdPhysicsStaticBroadcastData::MakeDefault
================
*/
void sdPhysicsStaticBroadcastData::MakeDefault( void ) {
	localPosition		= vec3_origin;
	localOrientation.x	= 0.f;
	localOrientation.y	= 0.f;
	localOrientation.z	= 0.f;
}

/*
================
sdPhysicsStaticBroadcastData::Write
================
*/
void sdPhysicsStaticBroadcastData::Write( idFile* file ) const {
	file->WriteVec3( localPosition );
	file->WriteCQuat( localOrientation );
}

/*
================
sdPhysicsStaticBroadcastData::Read
================
*/
void sdPhysicsStaticBroadcastData::Read( idFile* file ) {
	file->ReadVec3( localPosition );
	file->ReadCQuat( localOrientation );
}

/*
================
idPhysics_Static::idPhysics_Static
================
*/
idPhysics_Static::idPhysics_Static( void ) {
	self = NULL;
	clipModel = NULL;
	current.origin.Zero();
	current.axis.Identity();
	localOrigin.Zero();
	localAxis.Identity();

	flags.hasMaster = false;
	flags.isOrientated = false;
	flags.noPrediction = false;
}

/*
================
idPhysics_Static::~idPhysics_Static
================
*/
idPhysics_Static::~idPhysics_Static( void ) {
	if ( self && self->GetPhysics() == this ) {
		self->SetPhysics( NULL );
	}
	idForce::DeletePhysics( this );
	if ( clipModel != NULL ) {
		gameLocal.clip.DeleteClipModel( clipModel );
	}
}

/*
================
idPhysics_Static::SetSelf
================
*/
void idPhysics_Static::SetSelf( idEntity *e ) {
	assert( e );
	self = e;
}

/*
================
idPhysics_Static::SetClipModel
================
*/
void idPhysics_Static::SetClipModel( idClipModel *model, float density, int id, bool freeOld ) {
	assert( self );

	if ( clipModel != NULL && clipModel != model && freeOld ) {
		gameLocal.clip.DeleteClipModel( clipModel );
	}
	clipModel = model;
	if ( clipModel && !self->fl.dontLink ) {
		clipModel->Link( gameLocal.clip, self, 0, current.origin, current.axis );
	}
}

/*
================
idPhysics_Static::GetClipModel
================
*/
idClipModel *idPhysics_Static::GetClipModel( int id ) const {
	if ( clipModel ) {
		return clipModel;
	}
	return gameLocal.clip.DefaultClipModel();
}

/*
================
idPhysics_Static::GetNumClipModels
================
*/
int idPhysics_Static::GetNumClipModels( void ) const {
	return ( clipModel != NULL );
}

/*
================
idPhysics_Static::SetMass
================
*/
void idPhysics_Static::SetMass( float mass, int id ) {
}

/*
================
idPhysics_Static::GetMass
================
*/
float idPhysics_Static::GetMass( int id ) const {
	return 0.0f;
}

/*
================
idPhysics_Static::SetContents
================
*/
void idPhysics_Static::SetContents( int contents, int id ) {
	if ( clipModel ) {
		clipModel->SetContents( contents );
	}
}

/*
================
idPhysics_Static::GetContents
================
*/
int idPhysics_Static::GetContents( int id ) const {
	if ( clipModel ) {
		return clipModel->GetContents();
	}
	return 0;
}

/*
================
idPhysics_Static::SetClipMask
================
*/
void idPhysics_Static::SetClipMask( int mask, int id ) {
}

/*
================
idPhysics_Static::GetClipMask
================
*/
int idPhysics_Static::GetClipMask( int id ) const {
	return 0;
}

/*
================
idPhysics_Static::GetBounds
================
*/
const idBounds &idPhysics_Static::GetBounds( int id ) const {
	if ( clipModel ) {
		return clipModel->GetBounds();
	}
	return bounds_zero;
}

/*
================
idPhysics_Static::GetAbsBounds
================
*/
const idBounds &idPhysics_Static::GetAbsBounds( int id ) const {
	static idBounds absBounds;

	if ( clipModel ) {
		return clipModel->GetAbsBounds();
	}
	absBounds[0] = absBounds[1] = current.origin;
	return absBounds;
}

/*
================
idPhysics_Static::Evaluate
================
*/
bool idPhysics_Static::Evaluate( int timeStepMSec, int endTimeMSec ) {
	if ( flags.noPrediction && !gameLocal.isNewFrame ) {
		return false;
	}

	if ( flags.hasMaster ) {

		idVec3 oldOrigin = current.origin;
		idMat3 oldAxis = current.axis;

		idVec3 masterOrigin;
		idMat3 masterAxis;
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + localOrigin * masterAxis;
		if ( flags.isOrientated ) {
			current.axis = localAxis * masterAxis;
		} else {
			current.axis = localAxis;
		}
		if ( clipModel && !self->fl.dontLink ) {
			clipModel->Link( gameLocal.clip, self, 0, current.origin, current.axis );
		}

		return ( current.origin != oldOrigin || current.axis != oldAxis );
	}

	self->BecomeInactive( TH_PHYSICS );

	return false;
}

/*
================
idPhysics_Static::UpdateTime
================
*/
void idPhysics_Static::UpdateTime( int endTimeMSec ) {
}

/*
================
idPhysics_Static::GetTime
================
*/
int idPhysics_Static::GetTime( void ) const {
	return 0;
}

/*
================
idPhysics_Static::GetImpactInfo
================
*/
void idPhysics_Static::GetImpactInfo( const int id, const idVec3 &point, impactInfo_t *info ) const {
	memset( info, 0, sizeof( *info ) );
}

/*
================
idPhysics_Static::ApplyImpulse
================
*/
void idPhysics_Static::ApplyImpulse( const int id, const idVec3 &point, const idVec3 &impulse ) {
}

/*
================
idPhysics_Static::AddForce
================
*/
void idPhysics_Static::AddForce( const int id, const idVec3 &point, const idVec3 &force ) {
}

/*
================
idPhysics_Static::AddForce
================
*/
void idPhysics_Static::AddForce( const idVec3& force ) {
}

/*
================
idPhysics_Static::AddTorque
================
*/
void idPhysics_Static::AddTorque( const idVec3& torque ) {
}

/*
================
idPhysics_Static::Activate
================
*/
void idPhysics_Static::Activate( void ) {
}

/*
================
idPhysics_Static::PutToRest
================
*/
void idPhysics_Static::PutToRest( void ) {
}

/*
================
idPhysics_Static::IsAtRest
================
*/
bool idPhysics_Static::IsAtRest( void ) const {
	return true;
}

/*
================
idPhysics_Static::GetRestStartTime
================
*/
int idPhysics_Static::GetRestStartTime( void ) const {
	return 0;
}

/*
================
idPhysics_Static::IsPushable
================
*/
bool idPhysics_Static::IsPushable( void ) const {
	return false;
}

/*
================
idPhysics_Static::SaveState
================
*/
void idPhysics_Static::SaveState( void ) {
}

/*
================
idPhysics_Static::RestoreState
================
*/
void idPhysics_Static::RestoreState( void ) {
}

/*
================
idPhysics_Static::SetOrigin
================
*/
void idPhysics_Static::SetOrigin( const idVec3 &newOrigin, int id ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	if ( flags.hasMaster ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + newOrigin * masterAxis;
		localOrigin = newOrigin;
	} else {
		current.origin = newOrigin;
	}

	if ( clipModel && !self->fl.dontLink ) {
		clipModel->Link( gameLocal.clip, self, 0, current.origin, current.axis );
	}
}

/*
================
idPhysics_Static::SetAxis
================
*/
void idPhysics_Static::SetAxis( const idMat3 &newAxis, int id ) {

	if ( flags.hasMaster ) { 
		if ( flags.isOrientated ) {
			idVec3 masterOrigin;
			idMat3 masterAxis;
			self->GetMasterPosition( masterOrigin, masterAxis );
			current.axis = newAxis * masterAxis;
		}
		localAxis = newAxis;
	} else {
		current.axis = newAxis;
	}

	if ( clipModel && !self->fl.dontLink ) {
		clipModel->Link( gameLocal.clip, self, 0, current.origin, current.axis );
	}
}

/*
================
idPhysics_Static::Translate
================
*/
void idPhysics_Static::Translate( const idVec3 &translation, int id ) {

	if ( flags.hasMaster ) {
		localOrigin += translation;
	}
	current.origin += translation;

	if ( clipModel && !self->fl.dontLink ) {
		clipModel->Link( gameLocal.clip, self, 0, current.origin, current.axis );
	}
}

/*
================
idPhysics_Static::Rotate
================
*/
void idPhysics_Static::Rotate( const idRotation &rotation, int id ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	current.origin *= rotation;
	current.axis *= rotation.ToMat3();

	if ( flags.hasMaster ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		localAxis *= rotation.ToMat3();
		localOrigin = ( current.origin - masterOrigin ) * masterAxis.Transpose();
	}

	if ( clipModel && !self->fl.dontLink ) {
		clipModel->Link( gameLocal.clip, self, 0, current.origin, current.axis );
	}
}

/*
================
idPhysics_Static::GetOrigin
================
*/
const idVec3 &idPhysics_Static::GetOrigin( int id ) const {
	return current.origin;
}

/*
================
idPhysics_Static::GetAxis
================
*/
const idMat3 &idPhysics_Static::GetAxis( int id ) const {
	return current.axis;
}

/*
================
idPhysics_Static::SetLinearVelocity
================
*/
void idPhysics_Static::SetLinearVelocity( const idVec3 &newLinearVelocity, int id ) {
}

/*
================
idPhysics_Static::SetAngularVelocity
================
*/
void idPhysics_Static::SetAngularVelocity( const idVec3 &newAngularVelocity, int id ) {
}

/*
================
idPhysics_Static::GetLinearVelocity
================
*/
const idVec3 &idPhysics_Static::GetLinearVelocity( int id ) const {
	return vec3_origin;
}

/*
================
idPhysics_Static::GetAngularVelocity
================
*/
const idVec3 &idPhysics_Static::GetAngularVelocity( int id ) const {
	return vec3_origin;
}

/*
================
idPhysics_Static::SetGravity
================
*/
void idPhysics_Static::SetGravity( const idVec3 &newGravity ) {
}

/*
================
idPhysics_Static::GetGravity
================
*/
const idVec3 &idPhysics_Static::GetGravity( void ) const {
	static idVec3 gravity( 0, 0, -g_gravity.GetFloat() );
	return gravity;
}

/*
================
idPhysics_Static::GetGravityNormal
================
*/
const idVec3 &idPhysics_Static::GetGravityNormal( void ) const {
	static idVec3 gravity( 0, 0, -1 );
	return gravity;
}

/*
================
idPhysics_Static::ClipTranslation
================
*/
void idPhysics_Static::ClipTranslation( trace_t &results, const idVec3 &translation, const idClipModel *model ) const {
	if ( model ) {
		gameLocal.clip.TranslationModel( CLIP_DEBUG_PARMS results, current.origin, current.origin + translation,
			clipModel, current.axis, MASK_SOLID, model, model->GetOrigin(), model->GetAxis() );
	} else {
		gameLocal.clip.Translation( CLIP_DEBUG_PARMS results, current.origin, current.origin + translation,
			clipModel, current.axis, MASK_SOLID, self );
	}	
}

/*
================
idPhysics_Static::ClipRotation
================
*/
void idPhysics_Static::ClipRotation( trace_t &results, const idRotation &rotation, const idClipModel *model ) const {
	if ( model ) {
		gameLocal.clip.RotationModel( CLIP_DEBUG_PARMS results, current.origin, rotation,
			clipModel, current.axis, MASK_SOLID, model, model->GetOrigin(), model->GetAxis() );
	} else {
		gameLocal.clip.Rotation( CLIP_DEBUG_PARMS results, current.origin, rotation, clipModel, current.axis, MASK_SOLID, self );
	}
}

/*
================
idPhysics_Static::ClipContents
================
*/
int idPhysics_Static::ClipContents( const idClipModel *model ) const {
	if ( clipModel ) {
		if ( model ) {
			return gameLocal.clip.ContentsModel( CLIP_DEBUG_PARMS clipModel->GetOrigin(), clipModel, clipModel->GetAxis(), -1,
				model, model->GetOrigin(), model->GetAxis() );
		} else {
			return gameLocal.clip.Contents( CLIP_DEBUG_PARMS clipModel->GetOrigin(), clipModel, clipModel->GetAxis(), -1, NULL );
		}
	}
	return 0;
}

/*
================
idPhysics_Static::UnlinkClip
================
*/
void idPhysics_Static::UnlinkClip( void ) {
	if ( clipModel != NULL ) {
		clipModel->Unlink( gameLocal.clip );
	}
}

/*
================
idPhysics_Static::LinkClip
================
*/
void idPhysics_Static::LinkClip( void ) {
	if ( clipModel != NULL && !self->fl.dontLink ) {
		clipModel->Link( gameLocal.clip, self, 0, current.origin, current.axis );
	}
}

/*
================
idPhysics_Static::DisableClip
================
*/
void idPhysics_Static::DisableClip( bool activateContacting ) {
	if ( clipModel != NULL ) {
		if ( activateContacting ) {
			WakeEntitiesContacting( self, clipModel );
		}
		clipModel->Disable();
	}
}

/*
================
idPhysics_Static::EnableClip
================
*/
void idPhysics_Static::EnableClip( void ) {
	if ( clipModel != NULL ) {
		clipModel->Enable();
	}
}

/*
================
idPhysics_Static::EvaluateContacts
================
*/
bool idPhysics_Static::EvaluateContacts( CLIP_DEBUG_PARMS_DECLARATION_ONLY ) {
	return false;
}

/*
================
idPhysics_Static::GetNumContacts
================
*/
int idPhysics_Static::GetNumContacts( void ) const {
	return 0;
}

/*
================
idPhysics_Static::GetContact
================
*/
const contactInfo_t &idPhysics_Static::GetContact( int num ) const {
	static contactInfo_t info;
	memset( &info, 0, sizeof( info ) );
	return info;
}

/*
================
idPhysics_Static::ClearContacts
================
*/
void idPhysics_Static::ClearContacts( void ) {
}

/*
================
idPhysics_Static::AddContactEntity
================
*/
void idPhysics_Static::AddContactEntity( idEntity *e ) {
}

/*
================
idPhysics_Static::RemoveContactEntity
================
*/
void idPhysics_Static::RemoveContactEntity( idEntity *e ) {
}

/*
================
idPhysics_Static::HasGroundContacts
================
*/
bool idPhysics_Static::HasGroundContacts( void ) const {
	return false;
}

/*
================
idPhysics_Static::IsGroundEntity
================
*/
bool idPhysics_Static::IsGroundEntity( int entityNum ) const {
	return false;
}

/*
================
idPhysics_Static::IsGroundClipModel
================
*/
bool idPhysics_Static::IsGroundClipModel( int entityNum, int id ) const {
	return false;
}

/*
================
idPhysics_Static::SetPushed
================
*/
void idPhysics_Static::SetPushed( int deltaTime ) {
}

/*
================
idPhysics_Static::GetPushedLinearVelocity
================
*/
const idVec3 &idPhysics_Static::GetPushedLinearVelocity( const int id ) const {
	return vec3_origin;
}

/*
================
idPhysics_Static::GetPushedAngularVelocity
================
*/
const idVec3 &idPhysics_Static::GetPushedAngularVelocity( const int id ) const {
	return vec3_origin;
}

/*
================
idPhysics_Static::SetMaster
================
*/
void idPhysics_Static::SetMaster( idEntity *master, const bool orientated ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	if ( master ) {
		if ( !flags.hasMaster ) {
			// transform from world space to master space
			self->GetMasterPosition( masterOrigin, masterAxis );
			localOrigin = ( current.origin - masterOrigin ) * masterAxis.Transpose();
			if ( orientated ) {
				localAxis = current.axis * masterAxis.Transpose();
			} else {
				localAxis = current.axis;
			}
			flags.hasMaster = true;
			flags.isOrientated = orientated;
		}
	} else {
		if ( self ) {
			self->BecomeActive( TH_PHYSICS );
		}
		flags.hasMaster = false;
	}
}

/*
================
idPhysics_Static::GetBlockingInfo
================
*/
const trace_t *idPhysics_Static::GetBlockingInfo( void ) const {
	return NULL;
}

/*
================
idPhysics_Static::GetBlockingEntity
================
*/
idEntity *idPhysics_Static::GetBlockingEntity( void ) const {
	return NULL;
}

/*
================
idPhysics_Static::GetLinearEndTime
================
*/
int idPhysics_Static::GetLinearEndTime( void ) const {
	return 0;
}

/*
================
idPhysics_Static::GetAngularEndTime
================
*/
int idPhysics_Static::GetAngularEndTime( void ) const {
	return 0;
}

/*
================
idPhysics_Static::CheckNetworkStateChanges
================
*/
bool idPhysics_Static::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_BASE( sdPhysicsStaticNetworkData );
		
		if ( baseData.position != current.origin ) {
			return true;
		}

		if ( baseData.orientation != current.axis.ToCQuat() ) {
			return true;
		}
		return false;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_BASE( sdPhysicsStaticBroadcastData );

		if ( baseData.localPosition != localOrigin ) {
			return true;
		}

		if ( baseData.localOrientation != localAxis.ToCQuat() ) {
			return true;
		}

		return false;
	}

	return false;
}

/*
================
idPhysics_Static::WriteNetworkState
================
*/
void idPhysics_Static::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdPhysicsStaticNetworkData );

		// update state
		newData.position		= current.origin;
		newData.orientation		= current.axis.ToCQuat();

		// write state
		msg.WriteDeltaVector( baseData.position, newData.position );
		msg.WriteDeltaCQuat( baseData.orientation, newData.orientation );

		return;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdPhysicsStaticBroadcastData );

		// update state
		newData.localPosition		= localOrigin;
		newData.localOrientation	= localAxis.ToCQuat();

		// write state
		msg.WriteDeltaVector( baseData.localPosition, newData.localPosition );
		msg.WriteDeltaCQuat( baseData.localOrientation, newData.localOrientation );

		return;
	}
}

/*
================
idPhysics_Static::ApplyNetworkState
================
*/
void idPhysics_Static::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_NEW( sdPhysicsStaticNetworkData );

		// update state
		current.origin			= newData.position;
		current.axis			= newData.orientation.ToMat3();

		if ( clipModel && !self->fl.dontLink ) {
			clipModel->Link( gameLocal.clip, self, 0, current.origin, current.axis );
		}
		self->UpdateVisuals();

		return;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_NEW( sdPhysicsStaticBroadcastData );

		// update state
		localOrigin			= newData.localPosition;
		localAxis			= newData.localOrientation.ToMat3();

		return;
	}
}

/*
================
idPhysics_Static::ReadNetworkState
================
*/
void idPhysics_Static::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdPhysicsStaticNetworkData );

		// read state
		newData.position		= msg.ReadDeltaVector( baseData.position );
		newData.orientation		= msg.ReadDeltaCQuat( baseData.orientation );

		self->OnNewOriginRead( newData.position );
		self->OnNewAxesRead( newData.orientation.ToMat3() );
		return;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdPhysicsStaticBroadcastData );

		// read state
		newData.localPosition		= msg.ReadDeltaVector( baseData.localPosition );
		newData.localOrientation	= msg.ReadDeltaCQuat( baseData.localOrientation );

		return;
	}
}

/*
================
idPhysics_Static::CreateNetworkStructure
================
*/
sdEntityStateNetworkData* idPhysics_Static::CreateNetworkStructure( networkStateMode_t mode ) const {
	if ( mode == NSM_VISIBLE ) {
		return new sdPhysicsStaticNetworkData();
	}
	if ( mode == NSM_BROADCAST ) {
		return new sdPhysicsStaticBroadcastData();
	}
	return NULL;
}
