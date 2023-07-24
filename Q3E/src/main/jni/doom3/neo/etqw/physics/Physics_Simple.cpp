// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Physics_Simple.h"

/*
===============================================================================

	sdPhysics_Simple

===============================================================================
*/
CLASS_DECLARATION( idPhysics_Actor, sdPhysics_Simple )
END_CLASS

/*
=====================
sdSimplePhysicsNetworkData::MakeDefault
=====================
*/
void sdSimplePhysicsNetworkData::MakeDefault( void ) {
	origin			= vec3_origin;
	velocity		= vec3_zero;
	orientation.x	= 0.f;
	orientation.y	= 0.f;
	orientation.z	= 0.f;
	angularVelocity = vec3_zero;
	atRest			= -2;			// this will force an update straight away
	locked			= false;
}

/*
=====================
sdSimplePhysicsNetworkData::Write
=====================
*/
void sdSimplePhysicsNetworkData::Write( idFile* file ) const {
	file->WriteVec3( origin );
	file->WriteVec3( velocity );
	file->WriteCQuat( orientation );
	file->WriteVec3( angularVelocity );
	file->WriteInt( atRest );
	file->WriteBool( locked );
}

/*
=====================
sdSimplePhysicsNetworkData::Read
=====================
*/
void sdSimplePhysicsNetworkData::Read( idFile* file ) {
	file->ReadVec3( origin );
	file->ReadVec3( velocity );
	file->ReadCQuat( orientation );
	file->ReadVec3( angularVelocity );
	file->ReadInt( atRest );
	file->ReadBool( locked );

	origin.FixDenormals();
	velocity.FixDenormals();
}

/*
=====================
sdSimplePhysicsBroadcastData::MakeDefault
=====================
*/
void sdSimplePhysicsBroadcastData::MakeDefault( void ) {
	pushVelocity		= vec3_zero;
	atRest				= -2;			// this will force an update straight away
	groundLevel			= 0.f;
	locked				= false;
}

/*
=====================
sdSimplePhysicsBroadcastData::Write
=====================
*/
void sdSimplePhysicsBroadcastData::Write( idFile* file ) const {
	file->WriteVec3( pushVelocity );
	file->WriteInt( atRest );
	file->WriteFloat( groundLevel );
	file->WriteBool( locked );
}

/*
=====================
sdSimplePhysicsBroadcastData::Read
=====================
*/
void sdSimplePhysicsBroadcastData::Read( idFile* file ) {
	file->ReadVec3( pushVelocity );
	file->ReadInt( atRest );
	file->ReadFloat( groundLevel );
	file->ReadBool( locked );
}

/*
================
sdPhysics_Simple::~sdPhysics_Simple
================
*/
sdPhysics_Simple::~sdPhysics_Simple() {
}

/*
================
sdPhysics_Simple::sdPhysics_Simple
================
*/
sdPhysics_Simple::sdPhysics_Simple() {
	memset( &current, 0, sizeof( current ) );
	saved = current;

	atRest		= -1;
	rotates		= false;
	locked		= false;

	groundLevel	= gameLocal.clip.GetWorldBounds().GetMins().z;
}

/*
================
sdPhysics_Simple::SetClipModel
================
*/
#define MAX_INERTIA_SCALE		10.0f

void sdPhysics_Simple::SetClipModel( idClipModel *model, const float density, int id, bool freeOld ) {
	int minIndex;
	idMat3 inertiaScale;

	assert( self );
	assert( model );					// we need a clip model
	assert( density > 0.0f );			// density should be valid

	if ( clipModel != NULL && clipModel != model && freeOld ) {
		gameLocal.clip.DeleteClipModel( clipModel );
	}
	clipModel = model;
	clipModel->Link( gameLocal.clip, self, 0, current.origin, current.axis );

	if ( model->IsTraceModel() ) {
		rotates = true;

		// get mass properties from the trace model
		clipModel->GetMassProperties( density, mass, centerOfMass, inertiaTensor );

		// check whether or not the clip model has valid mass properties
		if ( mass < idMath::FLT_EPSILON || FLOAT_IS_NAN( mass ) ) {
			gameLocal.Warning( "sdPhysics_Simple::SetClipModel: invalid mass for entity '%s' type '%s'",
								self->name.c_str(), self->GetType()->classname );
			mass = 1.0f;
			centerOfMass.Zero();
			inertiaTensor.Identity();
		}

		// check whether or not the inertia tensor is balanced
		minIndex = Min3Index( inertiaTensor[0][0], inertiaTensor[1][1], inertiaTensor[2][2] );
		inertiaScale.Identity();
		inertiaScale[0][0] = inertiaTensor[0][0] / inertiaTensor[minIndex][minIndex];
		inertiaScale[1][1] = inertiaTensor[1][1] / inertiaTensor[minIndex][minIndex];
		inertiaScale[2][2] = inertiaTensor[2][2] / inertiaTensor[minIndex][minIndex];

		if ( inertiaScale[0][0] > MAX_INERTIA_SCALE || inertiaScale[1][1] > MAX_INERTIA_SCALE || inertiaScale[2][2] > MAX_INERTIA_SCALE ) {
			gameLocal.Warning( "sdPhysics_Simple::SetClipModel: unbalanced inertia tensor for entity '%s' type '%s'",
								self->name.c_str(), self->GetType()->classname );

			// correct the inertia tensor by replacing it with that of a box of the same bounds as this
			idTraceModel trm( clipModel->GetBounds() );
			trm.GetMassProperties( density, mass, centerOfMass, inertiaTensor );
		}

		inverseInertiaTensor = inertiaTensor.Inverse() * ( 1.0f / 6.0f );
	}

	invMass = 1.0f / mass;
	current.velocity.Zero();
	current.angularVelocity.Zero();
}

/*
================
sdPhysics_Simple::IsAtRest
================
*/
bool sdPhysics_Simple::IsAtRest( void ) const {
	return ( atRest >= 0 && gameLocal.time > atRest ) || locked;
}

/*
================
sdPhysics_Simple::Evaluate
================
*/
bool sdPhysics_Simple::Evaluate( int timeStepMSec, int endTimeMSec ) {
	if ( IsAtRest() ) {
		current.velocity.Zero();
		current.angularVelocity.Zero();
		
		// evaluate shouldn't become active if this is at rest
		self->BecomeInactive( TH_PHYSICS );
		return false;
	}

	idVec3 oldOrigin = current.origin;

	// move the critter
	float timeStep = MS2SEC( timeStepMSec );
	current.origin += current.velocity * timeStep;

	bool rotated = false;
	if ( rotates ) {
		// rotation
		idVec3 rotAxis = current.angularVelocity;
		float angle = rotAxis.Normalize();
		if ( angle > 0.00001f ) {
			idRotation rotation( vec3_origin, rotAxis, -angle * timeStep );
			current.axis = current.axis * rotation.ToMat3();
			current.axis.OrthoNormalizeSelf();
			rotated = true;
		}
	}

	// add gravity
	current.velocity += gravityVector * timeStep;

	if ( -( gravityNormal * current.origin ) < groundLevel ) {
		current.origin -= gravityNormal * ( ( gravityNormal * current.origin ) + groundLevel );
		PutToRest();
		DisableImpact();
	}

	if ( current.origin == oldOrigin && !rotated ) {
		return false;
	}

	if ( clipModel ) {
		clipModel->Link( gameLocal.clip, self, 0, current.origin, current.axis );
	}

	return true;
}

/*
================
sdPhysics_Simple::SetOrigin
================
*/
void sdPhysics_Simple::SetOrigin( const idVec3 &newOrigin, int id ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	current.origin = newOrigin;

	if ( clipModel ) {
		clipModel->Link( gameLocal.clip, self, 0, current.origin, current.axis );
	}
}

/*
================
sdPhysics_Simple::GetOrigin
================
*/
const idVec3& sdPhysics_Simple::GetOrigin( int id ) const {
	return current.origin;
}

/*
================
sdPhysics_Simple::SetAxis
================
*/
void sdPhysics_Simple::SetAxis( const idMat3 &newAxis, int id ) {
	current.axis = newAxis;

	if ( clipModel ) {
		clipModel->Link( gameLocal.clip, self, 0, current.origin, current.axis );
	}
}

/*
================
sdPhysics_Simple::GetAxis
================
*/
const idMat3& sdPhysics_Simple::GetAxis( int id ) const {
	return current.axis;
}


/*
================
sdPhysics_Simple::SetLinearVelocity
================
*/
void sdPhysics_Simple::SetLinearVelocity( const idVec3 &newLinearVelocity, int id ) {
	current.velocity = newLinearVelocity;
}

/*
================
sdPhysics_Simple::GetLinearVelocity
================
*/
const idVec3& sdPhysics_Simple::GetLinearVelocity( int id ) const {
	return current.velocity;
}

/*
================
sdPhysics_Simple::SetAngularVelocity
================
*/
void sdPhysics_Simple::SetAngularVelocity( const idVec3 &newAngularVelocity, int id ) {
	current.angularVelocity = newAngularVelocity;
}

/*
================
sdPhysics_Simple::GetAngularVelocity
================
*/
const idVec3& sdPhysics_Simple::GetAngularVelocity( int id ) const {
	return current.angularVelocity;
}


/*
================
sdPhysics_Simple::SetPushed
================
*/
void sdPhysics_Simple::SetPushed( int deltaTime ) {
	// velocity with which the monster is pushed
	current.pushVelocity += ( current.origin - saved.origin ) / ( deltaTime * idMath::M_MS2SEC );
}

/*
================
sdPhysics_Simple::GetPushedLinearVelocity
================
*/
const idVec3 &sdPhysics_Simple::GetPushedLinearVelocity( const int id ) const {
	return current.pushVelocity;
}

/*
================
sdPhysics_Simple::SaveState
================
*/
void sdPhysics_Simple::SaveState( void ) {
	saved = current;
}

/*
================
sdPhysics_Simple::RestoreState
================
*/
void sdPhysics_Simple::RestoreState( void ) {
	current = saved;

	clipModel->Link( gameLocal.clip, self, 0, current.origin, current.axis );

	EvaluateContacts( CLIP_DEBUG_PARMS_ONLY );
}

/*
================
sdPhysics_Simple::SetMaster

  the binding is never orientated
================
*/
void sdPhysics_Simple::SetMaster( idEntity *master, const bool orientated ) {
	return;
}

/*
================
sdPhysics_Simple::UpdateTime
================
*/
void sdPhysics_Simple::UpdateTime( int endTimeMSec ) {
}

/*
================
sdPhysics_Simple::GetTime
================
*/
int sdPhysics_Simple::GetTime( void ) const {
	return gameLocal.time;
}

/*
================
sdPhysics_Simple::GetImpactInfo
================
*/
void sdPhysics_Simple::GetImpactInfo( const int id, const idVec3 &point, impactInfo_t *info ) const {
	info->invMass = invMass;
	info->invInertiaTensor.Zero();
	info->position.Zero();
	info->velocity = current.velocity;
}

/*
================
sdPhysics_Simple::ApplyImpulse
================
*/
void sdPhysics_Simple::ApplyImpulse( const int id, const idVec3 &point, const idVec3 &impulse ) {
	if ( locked ) {
		return;
	}

	current.velocity += impulse * invMass;

	if ( rotates ) {
		idVec3 angularImpulse = ( point - ( current.origin + centerOfMass * current.axis ) ).Cross( impulse );
		idMat3 inverseWorldInertiaTensor = current.axis.Transpose() * inverseInertiaTensor * current.axis;
		current.angularVelocity += angularImpulse * inverseInertiaTensor;
	}
}

/*
================
sdPhysics_Simple::Translate
================
*/
void sdPhysics_Simple::Translate( const idVec3 &translation, int id ) {
	current.origin += translation;
	clipModel->Link( gameLocal.clip, self, 0, current.origin, current.axis );
}

/*
================
sdPhysics_Simple::Rest
================
*/
void sdPhysics_Simple::Rest( int time ) {
	current.velocity.Zero();
	current.angularVelocity.Zero();

	atRest = time;
	self->BecomeInactive( TH_PHYSICS );
}

/*
================
sdPhysics_Simple::PutToRest
================
*/
void sdPhysics_Simple::PutToRest( void ) {
	Rest( gameLocal.time );
}

/*
================
sdPhysics_Simple::SetComeToRest
================
*/
void sdPhysics_Simple::SetComeToRest( bool value ) {
	if ( value ) {
		PutToRest();
	} else {
		Activate();
	}
}

/*
================
sdPhysics_Simple::Activate
================
*/
void sdPhysics_Simple::Activate( void ) {
	if ( !locked ) {
		atRest = -1;
		self->BecomeActive( TH_PHYSICS );
	}
}

/*
================
sdPhysics_Simple::SetGroundPosition
================
*/
void sdPhysics_Simple::SetGroundPosition( const idVec3& position ) {
	groundLevel = position * -gravityNormal;
}

/*
================
sdPhysics_Simple::IsPushable
================
*/
bool sdPhysics_Simple::IsPushable( void ) const {
	return !locked;
}

/*
================
sdPhysics_Simple::EnableImpact
================
*/
void sdPhysics_Simple::EnableImpact( void ) {
	locked = false;
}

/*
================
sdPhysics_Simple::DisableImpact
================
*/
void sdPhysics_Simple::DisableImpact( void ) {
	locked = true;
}



const float	SIMPLE_ORIGIN_MAX				= 32767.0f;
const int	SIMPLE_ORIGIN_TOTAL_BITS		= 24;
const int	SIMPLE_ORIGIN_EXPONENT_BITS	= idMath::BitsForInteger( idMath::BitsForFloat( SIMPLE_ORIGIN_MAX ) ) + 1;
const int	SIMPLE_ORIGIN_MANTISSA_BITS	= SIMPLE_ORIGIN_TOTAL_BITS - 1 - SIMPLE_ORIGIN_EXPONENT_BITS;

const float	SIMPLE_VELOCITY_MAX			= 4000;
const int	SIMPLE_VELOCITY_TOTAL_BITS		= 16;
const int	SIMPLE_VELOCITY_EXPONENT_BITS	= idMath::BitsForInteger( idMath::BitsForFloat( SIMPLE_VELOCITY_MAX ) ) + 1;
const int	SIMPLE_VELOCITY_MANTISSA_BITS	= SIMPLE_VELOCITY_TOTAL_BITS - 1 - SIMPLE_VELOCITY_EXPONENT_BITS;

/*
================
sdPhysics_Simple::CheckNetworkStateChanges
================
*/
bool sdPhysics_Simple::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_BASE( sdSimplePhysicsNetworkData );

		if ( !baseData.origin.Compare( current.origin, 0.01f ) ) {
			return true;
		}

		if ( !baseData.velocity.Compare( current.velocity, 0.01f ) ) {
			return true;
		}

		if ( baseData.orientation != current.axis.ToCQuat() ) {
			return true;
		}

		if ( !baseData.angularVelocity.Compare( current.angularVelocity, 0.01f ) ) {
		}

		if ( baseData.atRest != atRest ) {
			return true;
		}
		if ( baseData.locked != locked ) {
			return true;
		}

		return false;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_BASE( sdSimplePhysicsBroadcastData );

		if ( baseData.pushVelocity != current.pushVelocity ) {
			return true;
		}

		if ( baseData.atRest != atRest ) {
			return true;
		}

		if ( baseData.groundLevel != groundLevel ) {
			return true;
		}

		if ( baseData.locked != locked ) {
			return true;
		}

		return false;
	}

	return false;
}

/*
================
sdPhysics_Simple::WriteNetworkState
================
*/
void sdPhysics_Simple::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdSimplePhysicsNetworkData );

		newData.origin			= current.origin;
		newData.velocity		= current.velocity;
		newData.orientation		= current.axis.ToCQuat();
		newData.angularVelocity	= current.angularVelocity;
		newData.atRest			= atRest;
		newData.locked			= locked;

		msg.WriteDeltaVector( baseData.origin, newData.origin, SIMPLE_ORIGIN_EXPONENT_BITS, SIMPLE_ORIGIN_MANTISSA_BITS );
		msg.WriteDeltaVector( baseData.velocity, newData.velocity, SIMPLE_VELOCITY_EXPONENT_BITS, SIMPLE_VELOCITY_MANTISSA_BITS );
		msg.WriteDeltaCQuat( baseData.orientation, newData.orientation );
		msg.WriteDeltaVector( baseData.angularVelocity, newData.angularVelocity, SIMPLE_VELOCITY_EXPONENT_BITS, SIMPLE_VELOCITY_MANTISSA_BITS );
		msg.WriteDeltaLong( baseData.atRest, newData.atRest );
		msg.WriteBool( newData.locked );

		return;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdSimplePhysicsBroadcastData );

		newData.pushVelocity	= current.pushVelocity;
		newData.atRest			= atRest;
		newData.groundLevel		= groundLevel;
		newData.locked			= locked;

		msg.WriteDeltaVector( baseData.pushVelocity, newData.pushVelocity, SIMPLE_VELOCITY_EXPONENT_BITS, SIMPLE_VELOCITY_MANTISSA_BITS );
		msg.WriteDeltaLong( baseData.atRest, newData.atRest );
		msg.WriteDeltaFloat( baseData.groundLevel, newData.groundLevel );
		msg.WriteBool( newData.locked );

		return;
	}
}

/*
================
sdPhysics_Simple::ApplyNetworkState
================
*/
void sdPhysics_Simple::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	traceCollection.ForceNextUpdate();
	if ( mode == NSM_VISIBLE ) {
		NET_GET_NEW( sdSimplePhysicsNetworkData );

		current.origin			= newData.origin;
		current.velocity		= newData.velocity;
		current.axis			= newData.orientation.ToMat3();
		current.angularVelocity	= newData.angularVelocity;
		
		locked					= newData.locked;
		if ( newData.atRest != atRest ) {
			if ( newData.atRest == -1 ) {
				Activate();
			} else {
				Rest( newData.atRest );
			}
		}

		clipModel->Link( gameLocal.clip, self, 0, current.origin, current.axis );

		self->UpdateVisuals();
		return;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_NEW( sdSimplePhysicsBroadcastData );

		current.pushVelocity	= newData.pushVelocity;
		groundLevel				= newData.groundLevel;
		locked					= newData.locked;

		if ( newData.atRest != atRest ) {
			if ( newData.atRest == -1 ) {
				Activate();
			} else {
				Rest( newData.atRest );
			}
		}
		return;
	}
}

/*
================
sdPhysics_Simple::ReadNetworkState
================
*/
void sdPhysics_Simple::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdSimplePhysicsNetworkData );

		newData.origin			= msg.ReadDeltaVector( baseData.origin, SIMPLE_ORIGIN_EXPONENT_BITS, SIMPLE_ORIGIN_MANTISSA_BITS );
		newData.velocity		= msg.ReadDeltaVector( baseData.velocity, SIMPLE_VELOCITY_EXPONENT_BITS, SIMPLE_VELOCITY_MANTISSA_BITS );
		newData.orientation		= msg.ReadDeltaCQuat( baseData.orientation );
		newData.angularVelocity	= msg.ReadDeltaVector( baseData.angularVelocity, SIMPLE_VELOCITY_EXPONENT_BITS, SIMPLE_VELOCITY_MANTISSA_BITS );
		newData.atRest			= msg.ReadDeltaLong( baseData.atRest );
		newData.locked			= msg.ReadBool();

		newData.origin.FixDenormals();
		newData.velocity.FixDenormals();

		self->OnNewOriginRead( newData.origin );
		self->OnNewAxesRead( newData.orientation.ToMat3() );
		return;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdSimplePhysicsBroadcastData );

		newData.pushVelocity	= msg.ReadDeltaVector( baseData.pushVelocity, SIMPLE_VELOCITY_EXPONENT_BITS, SIMPLE_VELOCITY_MANTISSA_BITS );
		newData.atRest			= msg.ReadDeltaLong( baseData.atRest );
		newData.groundLevel		= msg.ReadDeltaFloat( baseData.groundLevel );
		newData.locked			= msg.ReadBool();

		newData.pushVelocity.FixDenormals();
		return;
	}
}

/*
================
sdPhysics_Simple::CreateNetworkStructure
================
*/
sdEntityStateNetworkData* sdPhysics_Simple::CreateNetworkStructure( networkStateMode_t mode ) const {
	if ( mode == NSM_VISIBLE ) {
		return new sdSimplePhysicsNetworkData();
	}
	if ( mode == NSM_BROADCAST ) {
		return new sdSimplePhysicsBroadcastData();
	}
	return NULL;
}
