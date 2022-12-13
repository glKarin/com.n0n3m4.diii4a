// Copyright (C) 2004 Id Software, Inc.
//

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

CLASS_DECLARATION( idPhysics_Base, idPhysics_Actor )
END_CLASS

//HUMANHEAD PCF mdl 04/30/06 - Added this in case last minute change below causes chaos
#define LAST_MINUTE_HACK

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

	// HUMANHEAD nla
	numTouch = 0;
	memset( &groundTrace, 0, sizeof(trace_t) );
	hadGroundContacts = false;
	// HUMANHEAD END

	groundEntityPtr = NULL;
}

/*
================
idPhysics_Actor::~idPhysics_Actor
================
*/
idPhysics_Actor::~idPhysics_Actor( void ) {
	if ( clipModel ) {
		delete clipModel;
		clipModel = NULL;
	}
}

/*
================
idPhysics_Actor::Save
================
*/
void idPhysics_Actor::Save( idSaveGame *savefile ) const {

	savefile->WriteClipModel( clipModel );
	savefile->WriteMat3( clipModelAxis );

	savefile->WriteFloat( mass );
	savefile->WriteFloat( invMass );

	savefile->WriteObject( masterEntity );
	savefile->WriteFloat( masterYaw );
	savefile->WriteFloat( masterDeltaYaw );

	groundEntityPtr.Save( savefile );

	// HUMANHEAD mdl
	int i;
	savefile->WriteTrace( groundTrace );
	savefile->WriteInt( numTouch );
	for( i = 0; i < MAXTOUCH; i++ ) {
		savefile->WriteInt( touchEnts[i] );
	}
	savefile->WriteBool( hadGroundContacts );
	for( i = 0; i < c_iNumRotationTraces; i++ ) {
		savefile->WriteVec3( rotationTraceDirectionTable[i] );
	}
	// HUMANHEAD END
}

/*
================
idPhysics_Actor::Restore
================
*/
void idPhysics_Actor::Restore( idRestoreGame *savefile ) {

	savefile->ReadClipModel( clipModel );
	savefile->ReadMat3( clipModelAxis );

	savefile->ReadFloat( mass );
	savefile->ReadFloat( invMass );

	savefile->ReadObject( reinterpret_cast<idClass *&>( masterEntity ) );
	savefile->ReadFloat( masterYaw );
	savefile->ReadFloat( masterDeltaYaw );

	groundEntityPtr.Restore( savefile );

	// HUMANHEAD mdl
	int i;
	savefile->ReadTrace( groundTrace );
	savefile->ReadInt( numTouch );
	for( i = 0; i < MAXTOUCH; i++ ) {
		savefile->ReadInt( touchEnts[i] );
	}
	savefile->ReadBool( hadGroundContacts );
	for( i = 0; i < c_iNumRotationTraces; i++ ) {
		savefile->ReadVec3( rotationTraceDirectionTable[i] );
	}
	// HUMANHEAD END
}

/*
================
idPhysics_Actor::SetClipModelAxis
================
*/
void idPhysics_Actor::SetClipModelAxis( void ) {

	idMat3 prevClipModelAxis = clipModelAxis; // HUMANHEAD JRM

	// align clip model to gravity direction
	if ( ( gravityNormal[2] == -1.0f ) || ( gravityNormal == vec3_zero ) ) {
		clipModelAxis.Identity();
	}
	else {

		// HUMANHEAD JRM/PDM/NLA - To get the right clipmodel axis in gravity zones
		idVec3 newx;		
		newx = prevClipModelAxis[0] - (prevClipModelAxis[0] * -gravityNormal)*-gravityNormal;
		if(newx.LengthSqr() < VECTOR_EPSILON) {
			gameLocal.Warning("idPhysics_Actor::SetClipModelAxis invalid newx");
			if(prevClipModelAxis[0] * -gravityNormal > 0.0f) {
				newx = -prevClipModelAxis[2];
			}
			else {
				newx = prevClipModelAxis[2];
			}
		}
		newx.Normalize();
		clipModelAxis[0] = newx;
		clipModelAxis[2] = -gravityNormal;
		clipModelAxis[1] = clipModelAxis[2].Cross(clipModelAxis[0]);
		// HUMANHEAD END
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

	if ( clipModel && clipModel != model && freeOld ) {
		delete clipModel;
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
idPhysics_Actor::SetClipMask
================
*/
void idPhysics_Actor::SetContents( int contents, int id ) {
	clipModel->SetContents( contents );
}

/*
================
idPhysics_Actor::SetClipMask
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
		/* HUMANHEAD: aob - removed because InterativeRotateMove updates bbox
		SetClipModelAxis();
		HUMANHEAD END*/
	}
}

/*
================
idPhysics_Actor::ClipTranslation
================
*/
void idPhysics_Actor::ClipTranslation( trace_t &results, const idVec3 &translation, const idClipModel *model ) const {
	if ( model ) {
		gameLocal.clip.TranslationModel( results, clipModel->GetOrigin(), clipModel->GetOrigin() + translation,
								clipModel, clipModel->GetAxis(), clipMask,
								model->Handle(), model->GetOrigin(), model->GetAxis() );
	}
	else {
		gameLocal.clip.Translation( results, clipModel->GetOrigin(), clipModel->GetOrigin() + translation,
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
		gameLocal.clip.RotationModel( results, clipModel->GetOrigin(), rotation,
								clipModel, clipModel->GetAxis(), clipMask,
								model->Handle(), model->GetOrigin(), model->GetAxis() );
	}
	else {
		gameLocal.clip.Rotation( results, clipModel->GetOrigin(), rotation,
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
		return gameLocal.clip.ContentsModel( clipModel->GetOrigin(), clipModel, clipModel->GetAxis(), -1,
									model->Handle(), model->GetOrigin(), model->GetAxis() );
	}
	else {
		return gameLocal.clip.Contents( clipModel->GetOrigin(), clipModel, clipModel->GetAxis(), -1, NULL );
	}
}

/*
================
idPhysics_Actor::DisableClip
================
*/
void idPhysics_Actor::DisableClip( void ) {
	clipModel->Disable();
}

/*
================
idPhysics_Actor::EnableClip
================
*/
void idPhysics_Actor::EnableClip( void ) {
	clipModel->Enable();
}

/*
================
idPhysics_Actor::UnlinkClip
================
*/
void idPhysics_Actor::UnlinkClip( void ) {
	clipModel->Unlink();
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
idPhysics_Actor::EvaluateContacts
================
*/
bool idPhysics_Actor::EvaluateContacts( void ) {

	// get all the ground contacts
	ClearContacts();
	AddGroundContacts( clipModel );
	AddContactEntitiesForContacts();

	return ( contacts.Num() != 0 );
}


// HUMANHEAD functions

/*
===============
idPhysics_Actor::AddTouchEnt
HUMANHEAD nla
===============
*/
void idPhysics_Actor::AddTouchEnt( int entityNum ) {
	int i;

	if ( entityNum == ENTITYNUM_WORLD ) {
		return;
	}
	if ( numTouch == MAXTOUCH ) {
		return;
	}

	// see if it is already added
	for ( i = 0; i < numTouch; i++ ) {
		if ( touchEnts[ i ] == entityNum ) {
			return;
		}
	}

	// add it
	touchEnts[numTouch] = entityNum;
	numTouch++;
}


/*
===============
idPhysics_Actor::AddTouchEntList
HUMANHEAD nla
===============
*/
void idPhysics_Actor::AddTouchEntList( idList<int> &list ) {
	int i;

	
	for ( i = 0; i < list.Num(); i++ ) {
		AddTouchEnt(list[i]);
	}
}


/*
================
idPhysics_Actor::GetNumTouchEnts
HUMANHEAD nla
================
*/
int idPhysics_Actor::GetNumTouchEnts( void ) const {
	return numTouch;
}

/*
================
idPhysics_Actor::GetTouchEnt
HUMANHEAD nla
================
*/
int idPhysics_Actor::GetTouchEnt( int i ) const {
	if ( i < 0 || i >= numTouch ) {
		return ENTITYNUM_NONE;
	}
	return touchEnts[i];
}

/*
===============
idPhysics_Actor::ResetNumTouchEnt
HUMANHEAD nla
===============
*/
void idPhysics_Actor::ResetNumTouchEnt( int i ) {

	numTouch = i;

}

/*
================
hhPhysics_Player::HadGroundContacts

HUMANHEAD: aob
================
*/
bool idPhysics_Actor::HadGroundContacts() const {
	return hadGroundContacts;
}

/*
================
idPhysics_Actor::HadGroundContacts

HUMANHEAD: aob
================
*/
void idPhysics_Actor::HadGroundContacts( const bool hadGroundContacts ) {
	this->hadGroundContacts = hadGroundContacts;
}

/*
================
idPhysics_Actor::InterativeRotateMove

HUMANHEAD: aob
================
*/
bool idPhysics_Actor::IterativeRotateMove( const idVec3& upVector, const idVec3& idealUpVector, const idVec3& rotationOrigin, const idVec3& rotationCheckOrigin, int numIterations) {
	trace_t rotationTraceInfo;
	trace_t translationTraceInfo;
	idRotation rotator;
	idVec3 rotationVector;
	const idVec3 origPos( rotationOrigin );
	idVec3 currentOrigin( rotationOrigin );
	idMat3 currentAxis( GetAxis() );
	idVec3 averageTranslationVector( vec3_origin );
	float numTranslationVectors = 0.0f;

	if( idealUpVector.Compare(vec3_origin, VECTOR_EPSILON) ) {// For Zero G
		return false;
	}

	if( upVector.Compare(idealUpVector, VECTOR_EPSILON) ) {
		return false;
	}
	
	rotator.SetVec( DetermineRotationVector(upVector, idealUpVector, rotationCheckOrigin) );
	rotator.SetAngle( RAD2DEG(idMath::ACos(upVector * idealUpVector)) );
	rotator.ToMat3();
	
	do {
		rotator.SetOrigin( currentOrigin );
		gameLocal.clip.Rotation( rotationTraceInfo, currentOrigin, rotator, clipModel, currentAxis, clipMask, self );
		currentAxis = rotationTraceInfo.endAxis;

		if( rotationTraceInfo.fraction < 1.0f ) {
			gameLocal.clip.Translation( translationTraceInfo, currentOrigin, currentOrigin + rotationTraceInfo.c.normal * p_iterRotMoveTransDist.GetFloat(), clipModel, currentAxis, clipMask, self );
			currentOrigin = translationTraceInfo.endpos;
			averageTranslationVector += rotationTraceInfo.c.normal;
			numTranslationVectors += 1.0f;
		}
	} while( --numIterations && rotationTraceInfo.fraction < 1.0f );

	if( numTranslationVectors > 0.0f ) {
		averageTranslationVector /= numTranslationVectors;
		averageTranslationVector.Normalize();
		gameLocal.clip.Translation( translationTraceInfo, currentOrigin, currentOrigin + -averageTranslationVector * p_iterRotMoveTransDist.GetFloat(), clipModel, currentAxis, clipMask, self );
		currentOrigin = translationTraceInfo.endpos;
	}
	
	//HUMANHEAD rww - do a translation test before using this origin/axis
	if (!gameLocal.clip.Translation( translationTraceInfo, currentOrigin, currentOrigin, clipModel, currentAxis, clipMask, self )) {
		SetOrigin( currentOrigin );
		SetAxis( currentAxis );
	}
	else { //HUMANHEAD rww - if we fail, try instantly rotating to the desired orientation
		//HUMANHEAD PCF mdl 04/30/06 - Added LAST_MINUTE_HACK in case we want to revert this
#ifndef LAST_MINUTE_HACK
		if (groundTrace.fraction < 1.0f && fabsf(DotProduct(idealUpVector, groundTrace.c.normal)) > 0.8f) { //hitting ground
			const float fudge = 1.0f;
			currentOrigin = rotationCheckOrigin + (groundTrace.c.normal*(fabsf(GetBounds()[0].z)+fudge));

			idMat3 m = idealUpVector.ToMat3();
			currentAxis[0] = m[2];
			currentAxis[2] = m[0];
			currentAxis[1] = m[0].Cross(m[2]);
			if (!gameLocal.clip.Translation( translationTraceInfo, currentOrigin, currentOrigin, clipModel, currentAxis, clipMask, self )) {
				SetOrigin( currentOrigin );
				SetAxis( currentAxis );
			}
		}
#else
		if (groundTrace.fraction < 1.0f && fabsf(DotProduct(idealUpVector, groundTrace.c.normal)) > 0.8f) { //hitting ground
			const float fudge = 1.0f;
			float safeD = (fabsf(GetBounds()[0].z)+fudge);
			currentOrigin = rotationCheckOrigin + (groundTrace.c.normal*safeD);
			idMat3 m = idealUpVector.ToMat3();
			currentAxis[0] = m[2];
			currentAxis[2] = m[0];
			currentAxis[1] = m[0].Cross(m[2]);
			if (!gameLocal.clip.Translation( translationTraceInfo, currentOrigin, currentOrigin, clipModel, currentAxis, clipMask, self )) {
				//HUMANHEAD PCF rww 04/30/06 - trace back down on the normal once we have made sure we have a safe spot.
				const float downFudge = 16.0f;
				safeD = (fabsf(GetBounds()[1].z)+downFudge);
				idVec3 backDown = currentOrigin - (groundTrace.c.normal*safeD);
				gameLocal.clip.Translation( translationTraceInfo, currentOrigin, backDown, clipModel, currentAxis, clipMask, self );

				SetOrigin( translationTraceInfo.endpos );
				SetAxis( translationTraceInfo.endAxis );
			}
		}
#endif // LAST_MINUTE_HACK
	}

	return true;
}

/*
=============
idPhysics_Actor::DetermineRotationVector

HUMANHEAD: aob
=============
*/
idVec3 idPhysics_Actor::DetermineRotationVector( const idVec3& upVector, const idVec3& idealUpVector, const idVec3& checkOrigin ) {
	idVec3 rotationVector = idealUpVector.Cross( upVector );
	if( rotationVector.LengthSqr() <= VECTOR_EPSILON ) {
		BuildRotationTraceDirectionTable( self->GetAxis() );//Actors return viewAxis and plain entities return physics axis

		for( int iIndex = 0; iIndex < c_iNumRotationTraces; ++iIndex ) {
			if( DirectionIsClear(checkOrigin, rotationTraceDirectionTable[iIndex]) ) {
				rotationVector = rotationTraceDirectionTable[iIndex];
				break;
			}
		}
	}

	rotationVector.Normalize();

	return rotationVector;
}

/*
=============
idPhysics_Actor::DirectionIsClear

HUMANHEAD: aob
=============
*/
#define ROTATION_TRACE_DISTANCE ((pm_bboxwidth.GetFloat() * 0.5f) + 10.0f)
bool idPhysics_Actor::DirectionIsClear( const idVec3& checkOrigin, const idVec3& direction ) {
	trace_t traceInfo;

	gameLocal.clip.TracePoint( traceInfo, checkOrigin, checkOrigin + direction * ROTATION_TRACE_DISTANCE, clipMask, self );

	return (traceInfo.fraction == 1.0f);
}

/*
=============
idPhysics_Actor::BuildRotationTraceDirectionTable

HUMANHEAD: aob
=============
*/
void idPhysics_Actor::BuildRotationTraceDirectionTable( const idMat3& Axis ) {
	rotationTraceDirectionTable[0] = Axis[0];
	rotationTraceDirectionTable[1] = -Axis[0];
	rotationTraceDirectionTable[2] = Axis[1];
	rotationTraceDirectionTable[3] = -Axis[1];
}

