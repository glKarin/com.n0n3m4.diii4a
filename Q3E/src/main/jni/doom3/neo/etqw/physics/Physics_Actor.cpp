// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Physics_Actor.h"
#include "Clip.h"
#include "../Entity.h"

CLASS_DECLARATION( idPhysics_Base, idPhysics_Actor )
END_CLASS

/*
================
idPhysics_Actor::idPhysics_Actor
================
*/
idPhysics_Actor::idPhysics_Actor( void ) {
	clipModel = NULL;
	SetClipModelAxis();
	mass = 100.0f;
	invMass = 1.0f / mass;
	masterEntity = NULL;
	masterYaw = 0.0f;
	masterDeltaYaw = 0.0f;
	groundEntityPtr = NULL;
}

/*
================
idPhysics_Actor::~idPhysics_Actor
================
*/
idPhysics_Actor::~idPhysics_Actor( void ) {
	if ( clipModel != NULL ) {
		gameLocal.clip.DeleteClipModel( clipModel );
		clipModel = NULL;
	}
}

/*
================
idPhysics_Actor::SetClipModelAxis
================
*/
void idPhysics_Actor::SetClipModelAxis( void ) {
	// align clip model to gravity direction
	if ( ( gravityNormal[2] == -1.0f ) || ( gravityNormal == vec3_zero ) ) {
		clipModelAxis.Identity();
	}
	else {
		clipModelAxis[2] = -gravityNormal;
		clipModelAxis[2].NormalVectors( clipModelAxis[0], clipModelAxis[1] );
		clipModelAxis[1] = -clipModelAxis[1];
	}

	if ( clipModel ) {
		clipModel->Link( gameLocal.clip, self, 0, clipModel->GetOrigin(), clipModelAxis );
	}
}

/*
================
idPhysics_Actor::GetGravityAxis
================
*/
const idMat3 &idPhysics_Actor::GetGravityAxis( void ) const {
	return clipModelAxis;
}

/*
================
idPhysics_Actor::GetMasterDeltaYaw
================
*/
float idPhysics_Actor::GetMasterDeltaYaw( void ) const {
	return masterDeltaYaw;
}

/*
================
idPhysics_Actor::GetGroundEntity
================
*/
idEntity *idPhysics_Actor::GetGroundEntity( void ) const {
	return groundEntityPtr.GetEntity();
}

/*
================
idPhysics_Actor::SetClipModel
================
*/
void idPhysics_Actor::SetClipModel( idClipModel *model, const float density, int id, bool freeOld ) {
	assert( self );
	assert( model );					// a clip model is required
	assert( model->IsTraceModel() );	// and it should be a trace model
	assert( density > 0.0f );			// density should be valid

	if ( clipModel != NULL && clipModel != model && freeOld ) {
		gameLocal.clip.DeleteClipModel( clipModel );
	}
	clipModel = model;
	clipModel->Link( gameLocal.clip, self, 0, clipModel->GetOrigin(), clipModelAxis );
}

/*
================
idPhysics_Actor::GetClipModel
================
*/
idClipModel *idPhysics_Actor::GetClipModel( int id ) const {
	return clipModel;
}

/*
================
idPhysics_Actor::GetNumClipModels
================
*/
int idPhysics_Actor::GetNumClipModels( void ) const {
	return 1;
}

/*
================
idPhysics_Actor::SetMass
================
*/
void idPhysics_Actor::SetMass( float _mass, int id ) {
	assert( _mass > 0.0f );
	mass = _mass;
	invMass = 1.0f / _mass;
}

/*
================
idPhysics_Actor::GetMass
================
*/
float idPhysics_Actor::GetMass( int id ) const {
	return mass;
}

/*
================
idPhysics_Actor::SetContents
================
*/
void idPhysics_Actor::SetContents( int contents, int id ) {
	clipModel->SetContents( contents );
}

/*
================
idPhysics_Actor::GetContents
================
*/
int idPhysics_Actor::GetContents( int id ) const {
	return clipModel->GetContents();
}

/*
================
idPhysics_Actor::GetBounds
================
*/
const idBounds &idPhysics_Actor::GetBounds( int id ) const {
	return clipModel->GetBounds();
}

/*
================
idPhysics_Actor::GetAbsBounds
================
*/
const idBounds &idPhysics_Actor::GetAbsBounds( int id ) const {
	return clipModel->GetAbsBounds();
}

/*
================
idPhysics_Actor::IsPushable
================
*/
bool idPhysics_Actor::IsPushable( void ) const {
	return ( masterEntity == NULL );
}

/*
================
idPhysics_Actor::GetOrigin
================
*/
const idVec3 &idPhysics_Actor::GetOrigin( int id ) const {
	return clipModel->GetOrigin();
}

/*
================
idPhysics_Player::GetAxis
================
*/
const idMat3 &idPhysics_Actor::GetAxis( int id ) const {
	return clipModel->GetAxis();
}

/*
================
idPhysics_Actor::SetGravity
================
*/
void idPhysics_Actor::SetGravity( const idVec3 &newGravity ) {
	if ( newGravity != gravityVector ) {
		idPhysics_Base::SetGravity( newGravity );
		SetClipModelAxis();
	}
}

/*
================
idPhysics_Actor::ClipTranslation
================
*/
void idPhysics_Actor::ClipTranslation( trace_t &results, const idVec3 &translation, const idClipModel *model ) const {
	if ( model ) {
		gameLocal.clip.TranslationModel( CLIP_DEBUG_PARMS results, clipModel->GetOrigin(), clipModel->GetOrigin() + translation,
								clipModel, clipModel->GetAxis(), clipMask,
								model, model->GetOrigin(), model->GetAxis() );
	}
	else {
		gameLocal.clip.Translation( CLIP_DEBUG_PARMS results, clipModel->GetOrigin(), clipModel->GetOrigin() + translation,
								clipModel, clipModel->GetAxis(), clipMask, self );
	}
}

/*
================
idPhysics_Actor::ClipRotation
================
*/
void idPhysics_Actor::ClipRotation( trace_t &results, const idRotation &rotation, const idClipModel *model ) const {
	if ( model ) {
		gameLocal.clip.RotationModel( CLIP_DEBUG_PARMS results, clipModel->GetOrigin(), rotation,
								clipModel, clipModel->GetAxis(), clipMask,
								model, model->GetOrigin(), model->GetAxis() );
	}
	else {
		gameLocal.clip.Rotation( CLIP_DEBUG_PARMS results, clipModel->GetOrigin(), rotation,
								clipModel, clipModel->GetAxis(), clipMask, self );
	}
}

/*
================
idPhysics_Actor::ClipContents
================
*/
int idPhysics_Actor::ClipContents( const idClipModel *model ) const {
	if ( model ) {
		return gameLocal.clip.ContentsModel( CLIP_DEBUG_PARMS clipModel->GetOrigin(), clipModel, clipModel->GetAxis(), -1,
									model, model->GetOrigin(), model->GetAxis() );
	}
	else {
		return gameLocal.clip.Contents( CLIP_DEBUG_PARMS clipModel->GetOrigin(), clipModel, clipModel->GetAxis(), -1, NULL );
	}
}

/*
================
idPhysics_Actor::UnlinkClip
================
*/
void idPhysics_Actor::UnlinkClip( void ) {
	clipModel->Unlink( gameLocal.clip );
}

/*
================
idPhysics_Actor::LinkClip
================
*/
void idPhysics_Actor::LinkClip( void ) {
	clipModel->Link( gameLocal.clip, self, 0, clipModel->GetOrigin(), clipModel->GetAxis() );
}

/*
================
idPhysics_Actor::DisableClip
================
*/
void idPhysics_Actor::DisableClip( bool activateContacting ) {
	if ( activateContacting ) {
		WakeEntitiesContacting( self, clipModel );
	}
	clipModel->Disable();
}

/*
================
idPhysics_Actor::EnableClip
================
*/
void idPhysics_Actor::EnableClip( void ) {
	clipModel->Enable();
	LinkClip();
}

/*
================
idPhysics_Actor::EvaluateContacts
================
*/
bool idPhysics_Actor::EvaluateContacts( CLIP_DEBUG_PARMS_DECLARATION_ONLY ) {
	// get all the ground contacts
	ClearContacts();
	AddGroundContacts( CLIP_DEBUG_PARMS_PASSTHRU clipModel );
	AddContactEntitiesForContacts();

	return ( contacts.Num() != 0 );
}
