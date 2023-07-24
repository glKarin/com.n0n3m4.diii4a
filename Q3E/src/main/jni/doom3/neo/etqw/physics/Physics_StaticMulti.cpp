// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Physics_StaticMulti.h"
#include "../Entity.h"
#include "Force.h"
#include "Clip.h"

CLASS_DECLARATION( idPhysics, idPhysics_StaticMulti )
END_CLASS

staticPState_t defaultState;


/*
================
idPhysics_StaticMulti::idPhysics_StaticMulti
================
*/
idPhysics_StaticMulti::idPhysics_StaticMulti( void ) {
	self = NULL;

	defaultState.origin.Zero();
	defaultState.axis.Identity();

	current.SetNum( 1 );
	current[0] = defaultState;
	clipModels.SetNum( 1 );
	clipModels[0] = NULL;
}

/*
================
idPhysics_StaticMulti::~idPhysics_StaticMulti
================
*/
idPhysics_StaticMulti::~idPhysics_StaticMulti( void ) {
	if ( self && self->GetPhysics() == this ) {
		self->SetPhysics( NULL );
	}
	idForce::DeletePhysics( this );
	for ( int i = 0; i < clipModels.Num(); i++ ) {
		gameLocal.clip.DeleteClipModel( clipModels[i] );
	}
}

/*
================
idPhysics_StaticMulti::SetSelf
================
*/
void idPhysics_StaticMulti::SetSelf( idEntity *e ) {
	assert( e );
	self = e;
}

/*
================
idPhysics_StaticMulti::RemoveIndex
================
*/
void idPhysics_StaticMulti::RemoveIndex( int id, bool freeClipModel ) {
	if ( id < 0 || id >= clipModels.Num() ) {
		return;
	}
	if ( clipModels[id] != NULL && freeClipModel ) {
		gameLocal.clip.DeleteClipModel( clipModels[id] );
		clipModels[id] = NULL;
	}
	clipModels.RemoveIndex( id );
	current.RemoveIndex( id );
}

/*
================
idPhysics_StaticMulti::SetClipModel
================
*/
void idPhysics_StaticMulti::SetClipModel( idClipModel *model, float density, int id, bool freeOld ) {
	int i;

	assert( self );

	if ( id >= clipModels.Num() ) {
		current.AssureSize( id+1, defaultState );
		clipModels.AssureSize( id+1, NULL );
	}

	if ( clipModels[id] != NULL && clipModels[id] != model && freeOld ) {
		gameLocal.clip.DeleteClipModel( clipModels[id] );
	}
	clipModels[id] = model;
	if ( clipModels[id] ) {
		clipModels[id]->Link( gameLocal.clip, self, id, current[id].origin, current[id].axis );
	}

	for ( i = clipModels.Num() - 1; i >= 1; i-- ) {
		if ( clipModels[i] ) {
			break;
		}
	}
	current.SetNum( i+1, false );
	clipModels.SetNum( i+1, false );
}

/*
================
idPhysics_StaticMulti::GetClipModel
================
*/
idClipModel *idPhysics_StaticMulti::GetClipModel( int id ) const {
	if ( id >= 0 && id < clipModels.Num() && clipModels[id] ) {
		return clipModels[id];
	}
	return gameLocal.clip.DefaultClipModel();
}

/*
================
idPhysics_StaticMulti::GetNumClipModels
================
*/
int idPhysics_StaticMulti::GetNumClipModels( void ) const {
	return clipModels.Num();
}

/*
================
idPhysics_StaticMulti::SetMass
================
*/
void idPhysics_StaticMulti::SetMass( float mass, int id ) {
}

/*
================
idPhysics_StaticMulti::GetMass
================
*/
float idPhysics_StaticMulti::GetMass( int id ) const {
	return 0.0f;
}

/*
================
idPhysics_StaticMulti::SetContents
================
*/
void idPhysics_StaticMulti::SetContents( int contents, int id ) {
	int i;

	if ( id >= 0 && id < clipModels.Num() ) {
		if ( clipModels[id] ) {
			clipModels[id]->SetContents( contents );
		}
	} else if ( id == -1 ) {
		for ( i = 0; i < clipModels.Num(); i++ ) {
			if ( clipModels[i] ) {
				clipModels[i]->SetContents( contents );
			}
		}
	}
}

/*
================
idPhysics_StaticMulti::GetContents
================
*/
int idPhysics_StaticMulti::GetContents( int id ) const {
	int i, contents = 0;

	if ( id >= 0 && id < clipModels.Num() ) {
		if ( clipModels[id] ) {
			contents = clipModels[id]->GetContents();
		}
	} else if ( id == -1 ) {
		for ( i = 0; i < clipModels.Num(); i++ ) {
			if ( clipModels[i] ) {
				contents |= clipModels[i]->GetContents();
			}
		}
	}
	return contents;
}

/*
================
idPhysics_StaticMulti::SetClipMask
================
*/
void idPhysics_StaticMulti::SetClipMask( int mask, int id ) {
}

/*
================
idPhysics_StaticMulti::GetClipMask
================
*/
int idPhysics_StaticMulti::GetClipMask( int id ) const {
	return 0;
}

/*
================
idPhysics_StaticMulti::GetBounds
================
*/
const idBounds &idPhysics_StaticMulti::GetBounds( int id ) const {
	int i;
	static idBounds bounds;

	if ( id >= 0 && id < clipModels.Num() ) {
		if ( clipModels[id] ) {
			return clipModels[id]->GetBounds();
		}
	}
	if ( id == -1 ) {
		bounds.Clear();
		for ( i = 0; i < clipModels.Num(); i++ ) {
			if ( clipModels[i] ) {
				bounds.AddBounds( clipModels[i]->GetAbsBounds() );
			}
		}
		for ( i = 0; i < clipModels.Num(); i++ ) {
			if ( clipModels[i] ) {
				bounds[0] -= clipModels[i]->GetOrigin();
				bounds[1] -= clipModels[i]->GetOrigin();
				break;
			}
		}
		return bounds;
	}
	return bounds_zero;
}

/*
================
idPhysics_StaticMulti::GetAbsBounds
================
*/
const idBounds &idPhysics_StaticMulti::GetAbsBounds( int id ) const {
	int i;
	static idBounds absBounds;

	if ( id >= 0 && id < clipModels.Num() ) {
		if ( clipModels[id] ) {
			return clipModels[id]->GetAbsBounds();
		}
	}
	if ( id == -1 ) {
		absBounds.Clear();
		for ( i = 0; i < clipModels.Num(); i++ ) {
			if ( clipModels[i] ) {
				absBounds.AddBounds( clipModels[i]->GetAbsBounds() );
			}
		}
		return absBounds;
	}
	return bounds_zero;
}

/*
================
idPhysics_StaticMulti::Evaluate
================
*/
bool idPhysics_StaticMulti::Evaluate( int timeStepMSec, int endTimeMSec ) {
	self->BecomeInactive( TH_PHYSICS );

	return false;
}

/*
================
idPhysics_StaticMulti::UpdateTime
================
*/
void idPhysics_StaticMulti::UpdateTime( int endTimeMSec ) {
}

/*
================
idPhysics_StaticMulti::GetTime
================
*/
int idPhysics_StaticMulti::GetTime( void ) const {
	return 0;
}

/*
================
idPhysics_StaticMulti::GetImpactInfo
================
*/
void idPhysics_StaticMulti::GetImpactInfo( const int id, const idVec3 &point, impactInfo_t *info ) const {
	memset( info, 0, sizeof( *info ) );
}

/*
================
idPhysics_StaticMulti::ApplyImpulse
================
*/
void idPhysics_StaticMulti::ApplyImpulse( const int id, const idVec3 &point, const idVec3 &impulse ) {
}

/*
================
idPhysics_StaticMulti::AddForce
================
*/
void idPhysics_StaticMulti::AddForce( const int id, const idVec3 &point, const idVec3 &force ) {
}

/*
================
idPhysics_StaticMulti::AddForce
================
*/
void idPhysics_StaticMulti::AddForce( const idVec3& force ) {
}

/*
================
idPhysics_StaticMulti::AddTorque
================
*/
void idPhysics_StaticMulti::AddTorque( const idVec3& torque ) {
}

/*
================
idPhysics_StaticMulti::Activate
================
*/
void idPhysics_StaticMulti::Activate( void ) {
}

/*
================
idPhysics_StaticMulti::PutToRest
================
*/
void idPhysics_StaticMulti::PutToRest( void ) {
}

/*
================
idPhysics_StaticMulti::IsAtRest
================
*/
bool idPhysics_StaticMulti::IsAtRest( void ) const {
	return true;
}

/*
================
idPhysics_StaticMulti::GetRestStartTime
================
*/
int idPhysics_StaticMulti::GetRestStartTime( void ) const {
	return 0;
}

/*
================
idPhysics_StaticMulti::IsPushable
================
*/
bool idPhysics_StaticMulti::IsPushable( void ) const {
	return false;
}

/*
================
idPhysics_StaticMulti::SaveState
================
*/
void idPhysics_StaticMulti::SaveState( void ) {
}

/*
================
idPhysics_StaticMulti::RestoreState
================
*/
void idPhysics_StaticMulti::RestoreState( void ) {
}

/*
================
idPhysics_StaticMulti::SetOrigin
================
*/
void idPhysics_StaticMulti::SetOrigin( const idVec3 &newOrigin, int id ) {
	if ( id >= 0 && id < clipModels.Num() ) {
		current[id].origin = newOrigin;
		if ( clipModels[id] ) {
			clipModels[id]->Link( gameLocal.clip, self, id, current[id].origin, current[id].axis );
		}
	} else if ( id == -1 ) {
		Translate( newOrigin - current[0].origin );
	}
}

/*
================
idPhysics_StaticMulti::SetAxis
================
*/
void idPhysics_StaticMulti::SetAxis( const idMat3 &newAxis, int id ) {
	if ( id >= 0 && id < clipModels.Num() ) {
		current[id].axis = newAxis;
		if ( clipModels[id] ) {
			clipModels[id]->Link( gameLocal.clip, self, id, current[id].origin, current[id].axis );
		}
	} else if ( id == -1 ) {
		idMat3 axis;
		idRotation rotation;

		axis = current[0].axis.Transpose() * newAxis;
		rotation = axis.ToRotation();
		rotation.SetOrigin( current[0].origin );

		Rotate( rotation );
	}
}

/*
================
idPhysics_StaticMulti::Translate
================
*/
void idPhysics_StaticMulti::Translate( const idVec3 &translation, int id ) {
	int i;

	if ( id >= 0 && id < clipModels.Num() ) {
		current[id].origin += translation;

		if ( clipModels[id] ) {
			clipModels[id]->Link( gameLocal.clip, self, id, current[id].origin, current[id].axis );
		}
	} else if ( id == -1 ) {
		for ( i = 0; i < clipModels.Num(); i++ ) {
			current[i].origin += translation;

			if ( clipModels[i] ) {
				clipModels[i]->Link( gameLocal.clip, self, i, current[i].origin, current[i].axis );
			}
		}
	}
}

/*
================
idPhysics_StaticMulti::Rotate
================
*/
void idPhysics_StaticMulti::Rotate( const idRotation &rotation, int id ) {
	int i;

	if ( id >= 0 && id < clipModels.Num() ) {
		current[id].origin *= rotation;
		current[id].axis *= rotation.ToMat3();

		if ( clipModels[id] ) {
			clipModels[id]->Link( gameLocal.clip, self, id, current[id].origin, current[id].axis );
		}
	} else if ( id == -1 ) {
		for ( i = 0; i < clipModels.Num(); i++ ) {
			current[i].origin *= rotation;
			current[i].axis *= rotation.ToMat3();

			if ( clipModels[i] ) {
				clipModels[i]->Link( gameLocal.clip, self, i, current[i].origin, current[i].axis );
			}
		}
	}
}

/*
================
idPhysics_StaticMulti::GetOrigin
================
*/
const idVec3 &idPhysics_StaticMulti::GetOrigin( int id ) const {
	if ( id >= 0 && id < clipModels.Num() ) {
		return current[id].origin;
	}
	if ( clipModels.Num() ) {
		return current[0].origin;
	} else {
		return vec3_origin;
	}
}

/*
================
idPhysics_StaticMulti::GetAxis
================
*/
const idMat3 &idPhysics_StaticMulti::GetAxis( int id ) const {
	if ( id >= 0 && id < clipModels.Num() ) {
		return current[id].axis;
	}
	if ( clipModels.Num() ) {
		return current[0].axis;
	} else {
		return mat3_identity;
	}
}

/*
================
idPhysics_StaticMulti::SetLinearVelocity
================
*/
void idPhysics_StaticMulti::SetLinearVelocity( const idVec3 &newLinearVelocity, int id ) {
}

/*
================
idPhysics_StaticMulti::SetAngularVelocity
================
*/
void idPhysics_StaticMulti::SetAngularVelocity( const idVec3 &newAngularVelocity, int id ) {
}

/*
================
idPhysics_StaticMulti::GetLinearVelocity
================
*/
const idVec3 &idPhysics_StaticMulti::GetLinearVelocity( int id ) const {
	return vec3_origin;
}

/*
================
idPhysics_StaticMulti::GetAngularVelocity
================
*/
const idVec3 &idPhysics_StaticMulti::GetAngularVelocity( int id ) const {
	return vec3_origin;
}

/*
================
idPhysics_StaticMulti::SetGravity
================
*/
void idPhysics_StaticMulti::SetGravity( const idVec3 &newGravity ) {
}

/*
================
idPhysics_StaticMulti::GetGravity
================
*/
const idVec3 &idPhysics_StaticMulti::GetGravity( void ) const {
	static idVec3 gravity( 0, 0, -g_gravity.GetFloat() );
	return gravity;
}

/*
================
idPhysics_StaticMulti::GetGravityNormal
================
*/
const idVec3 &idPhysics_StaticMulti::GetGravityNormal( void ) const {
	static idVec3 gravity( 0, 0, -1 );
	return gravity;
}

/*
================
idPhysics_StaticMulti::ClipTranslation
================
*/
void idPhysics_StaticMulti::ClipTranslation( trace_t &results, const idVec3 &translation, const idClipModel *model ) const {
	memset( &results, 0, sizeof( trace_t ) );
	gameLocal.Warning( "idPhysics_StaticMulti::ClipTranslation called" );
}

/*
================
idPhysics_StaticMulti::ClipRotation
================
*/
void idPhysics_StaticMulti::ClipRotation( trace_t &results, const idRotation &rotation, const idClipModel *model ) const {
	memset( &results, 0, sizeof( trace_t ) );
	gameLocal.Warning( "idPhysics_StaticMulti::ClipRotation called" );
}

/*
================
idPhysics_StaticMulti::ClipContents
================
*/
int idPhysics_StaticMulti::ClipContents( const idClipModel *model ) const {
	int i, contents;

	contents = 0;
	for ( i = 0; i < clipModels.Num(); i++ ) {
		if ( clipModels[i] ) {
			if ( model ) {
				contents |= gameLocal.clip.ContentsModel( CLIP_DEBUG_PARMS clipModels[i]->GetOrigin(), clipModels[i], clipModels[i]->GetAxis(), -1,
											model, model->GetOrigin(), model->GetAxis() );
			} else {
				contents |= gameLocal.clip.Contents( CLIP_DEBUG_PARMS clipModels[i]->GetOrigin(), clipModels[i], clipModels[i]->GetAxis(), -1, NULL );
			}
		}
	}
	return contents;
}

/*
================
idPhysics_StaticMulti::UnlinkClip
================
*/
void idPhysics_StaticMulti::UnlinkClip( void ) {
	int i;

	for ( i = 0; i < clipModels.Num(); i++ ) {
        if ( clipModels[i] != NULL ) {
			clipModels[i]->Unlink( gameLocal.clip );
		}
	}
}

/*
================
idPhysics_StaticMulti::LinkClip
================
*/
void idPhysics_StaticMulti::LinkClip( void ) {
	int i;

	for ( i = 0; i < clipModels.Num(); i++ ) {
		if ( clipModels[i] != NULL ) {
			clipModels[i]->Link( gameLocal.clip, self, i, current[i].origin, current[i].axis );
		}
	}
}

/*
================
idPhysics_StaticMulti::DisableClip
================
*/
void idPhysics_StaticMulti::DisableClip( bool activateContacting ) {
	int i;

	for ( i = 0; i < clipModels.Num(); i++ ) {
        if ( clipModels[ i ] ) {
			if ( activateContacting ) {
				WakeEntitiesContacting( self, clipModels[ i ] );
			}
			clipModels[ i ]->Disable();
		}
	}
}

/*
================
idPhysics_StaticMulti::EnableClip
================
*/
void idPhysics_StaticMulti::EnableClip( void ) {
	int i;

	for ( i = 0; i < clipModels.Num(); i++ ) {
		if ( clipModels[ i ] ) {
			clipModels[ i ]->Enable();
		}
	}
}
/*
================
idPhysics_StaticMulti::EvaluateContacts
================
*/
bool idPhysics_StaticMulti::EvaluateContacts( CLIP_DEBUG_PARMS_DECLARATION_ONLY ) {
	return false;
}

/*
================
idPhysics_StaticMulti::GetNumContacts
================
*/
int idPhysics_StaticMulti::GetNumContacts( void ) const {
	return 0;
}

/*
================
idPhysics_StaticMulti::GetContact
================
*/
const contactInfo_t &idPhysics_StaticMulti::GetContact( int num ) const {
	static contactInfo_t info;
	memset( &info, 0, sizeof( info ) );
	return info;
}

/*
================
idPhysics_StaticMulti::ClearContacts
================
*/
void idPhysics_StaticMulti::ClearContacts( void ) {
}

/*
================
idPhysics_StaticMulti::AddContactEntity
================
*/
void idPhysics_StaticMulti::AddContactEntity( idEntity *e ) {
}

/*
================
idPhysics_StaticMulti::RemoveContactEntity
================
*/
void idPhysics_StaticMulti::RemoveContactEntity( idEntity *e ) {
}

/*
================
idPhysics_StaticMulti::HasGroundContacts
================
*/
bool idPhysics_StaticMulti::HasGroundContacts( void ) const {
	return false;
}

/*
================
idPhysics_StaticMulti::IsGroundEntity
================
*/
bool idPhysics_StaticMulti::IsGroundEntity( int entityNum ) const {
	return false;
}

/*
================
idPhysics_StaticMulti::IsGroundClipModel
================
*/
bool idPhysics_StaticMulti::IsGroundClipModel( int entityNum, int id ) const {
	return false;
}

/*
================
idPhysics_StaticMulti::SetPushed
================
*/
void idPhysics_StaticMulti::SetPushed( int deltaTime ) {
}

/*
================
idPhysics_StaticMulti::GetPushedLinearVelocity
================
*/
const idVec3 &idPhysics_StaticMulti::GetPushedLinearVelocity( const int id ) const {
	return vec3_origin;
}

/*
================
idPhysics_StaticMulti::GetPushedAngularVelocity
================
*/
const idVec3 &idPhysics_StaticMulti::GetPushedAngularVelocity( const int id ) const {
	return vec3_origin;
}

/*
================
idPhysics_StaticMulti::SetMaster
================
*/
void idPhysics_StaticMulti::SetMaster( idEntity *master, const bool orientated ) {
}

/*
================
idPhysics_StaticMulti::GetBlockingInfo
================
*/
const trace_t *idPhysics_StaticMulti::GetBlockingInfo( void ) const {
	return NULL;
}

/*
================
idPhysics_StaticMulti::GetBlockingEntity
================
*/
idEntity *idPhysics_StaticMulti::GetBlockingEntity( void ) const {
	return NULL;
}

/*
================
idPhysics_StaticMulti::GetLinearEndTime
================
*/
int idPhysics_StaticMulti::GetLinearEndTime( void ) const {
	return 0;
}

/*
================
idPhysics_StaticMulti::GetAngularEndTime
================
*/
int idPhysics_StaticMulti::GetAngularEndTime( void ) const {
	return 0;
}
