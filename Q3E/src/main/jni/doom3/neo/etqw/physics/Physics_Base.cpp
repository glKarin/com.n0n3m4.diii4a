// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Physics_Base.h"
#include "../Entity.h"
#include "Force.h"

#if !defined( _XENON ) && !defined( MONOLITHIC )
idCVar r_debugAxisLength( "r_debugAxisLength", "2", CVAR_RENDERER, "used to set the length of drawn debug axis" );
#else
extern idCVar r_debugAxisLength;
#endif

CLASS_DECLARATION( idPhysics, idPhysics_Base )
END_CLASS

/*
================
idPhysics_Base::idPhysics_Base
================
*/
idPhysics_Base::idPhysics_Base( void ) {
	self = NULL;
	clipMask = 0;
	SetGravity( gameLocal.GetGravity() );
	ClearContacts();
}

/*
================
idPhysics_Base::~idPhysics_Base
================
*/
idPhysics_Base::~idPhysics_Base( void ) {
	if ( self && self->GetPhysics() == this ) {
		self->SetPhysics( NULL );
	}
	idForce::DeletePhysics( this );
	ClearContacts();
}

/*
================
idPhysics_Base::SetSelf
================
*/
void idPhysics_Base::SetSelf( idEntity *e ) {
	assert( e );
	self = e;
	traceCollection.SetSelf( e );
}

/*
================
idPhysics_Base::SetClipModel
================
*/
void idPhysics_Base::SetClipModel( idClipModel *model, float density, int id, bool freeOld ) {
}

/*
================
idPhysics_Base::GetClipModel
================
*/
idClipModel *idPhysics_Base::GetClipModel( int id ) const {
	return NULL;
}

/*
================
idPhysics_Base::GetNumClipModels
================
*/
int idPhysics_Base::GetNumClipModels( void ) const {
	return 0;
}

/*
================
idPhysics_Base::SetMass
================
*/
void idPhysics_Base::SetMass( float mass, int id ) {
}

/*
================
idPhysics_Base::GetMass
================
*/
float idPhysics_Base::GetMass( int id ) const {
	return 0.0f;
}

/*
================
idPhysics_Base::SetContents
================
*/
void idPhysics_Base::SetContents( int contents, int id ) {
}

/*
================
idPhysics_Base::SetClipMask
================
*/
int idPhysics_Base::GetContents( int id ) const {
	return 0;
}

/*
================
idPhysics_Base::SetClipMask
================
*/
void idPhysics_Base::SetClipMask( int mask, int id ) {
	clipMask = mask;
}

/*
================
idPhysics_Base::GetClipMask
================
*/
int idPhysics_Base::GetClipMask( int id ) const {
	return clipMask;
}

/*
================
idPhysics_Base::GetBounds
================
*/
const idBounds &idPhysics_Base::GetBounds( int id ) const {
	return bounds_zero;
}

/*
================
idPhysics_Base::GetAbsBounds
================
*/
const idBounds &idPhysics_Base::GetAbsBounds( int id ) const {
	return bounds_zero;
}

/*
================
idPhysics_Base::Evaluate
================
*/
bool idPhysics_Base::Evaluate( int timeStepMSec, int endTimeMSec ) {
	return false;
}

/*
================
idPhysics_Base::UpdateTime
================
*/
void idPhysics_Base::UpdateTime( int endTimeMSec ) {
}

/*
================
idPhysics_Base::GetTime
================
*/
int idPhysics_Base::GetTime( void ) const {
	return 0;
}

/*
================
idPhysics_Base::GetImpactInfo
================
*/
void idPhysics_Base::GetImpactInfo( const int id, const idVec3 &point, impactInfo_t *info ) const {
	memset( info, 0, sizeof( *info ) );
}

/*
================
idPhysics_Base::ApplyImpulse
================
*/
void idPhysics_Base::ApplyImpulse( const int id, const idVec3 &point, const idVec3 &impulse ) {
}

/*
================
idPhysics_Base::AddForce
================
*/
void idPhysics_Base::AddForce( const int id, const idVec3 &point, const idVec3 &force ) {
}

/*
================
idPhysics_Base::AddTorque
================
*/
void idPhysics_Base::AddTorque( const idVec3& torque ) {
}

/*
================
idPhysics_Base::AddForce
================
*/
void idPhysics_Base::AddForce( const idVec3& force ) {
}

/*
================
idPhysics_Base::Activate
================
*/
void idPhysics_Base::Activate( void ) {
}

/*
================
idPhysics_Base::PutToRest
================
*/
void idPhysics_Base::PutToRest( void ) {
}

/*
================
idPhysics_Base::IsAtRest
================
*/
bool idPhysics_Base::IsAtRest( void ) const {
	return true;
}

/*
================
idPhysics_Base::GetRestStartTime
================
*/
int idPhysics_Base::GetRestStartTime( void ) const {
	return 0;
}

/*
================
idPhysics_Base::IsPushable
================
*/
bool idPhysics_Base::IsPushable( void ) const {
	return true;
}

/*
================
idPhysics_Base::SaveState
================
*/
void idPhysics_Base::SaveState( void ) {
}

/*
================
idPhysics_Base::RestoreState
================
*/
void idPhysics_Base::RestoreState( void ) {
}

/*
================
idPhysics_Base::SetOrigin
================
*/
void idPhysics_Base::SetOrigin( const idVec3 &newOrigin, int id ) {
}

/*
================
idPhysics_Base::SetAxis
================
*/
void idPhysics_Base::SetAxis( const idMat3 &newAxis, int id ) {
}

/*
================
idPhysics_Base::Translate
================
*/
void idPhysics_Base::Translate( const idVec3 &translation, int id ) {
}

/*
================
idPhysics_Base::Rotate
================
*/
void idPhysics_Base::Rotate( const idRotation &rotation, int id ) {
}

/*
================
idPhysics_Base::GetOrigin
================
*/
const idVec3 &idPhysics_Base::GetOrigin( int id ) const {
	return vec3_origin;
}

/*
================
idPhysics_Base::GetAxis
================
*/
const idMat3 &idPhysics_Base::GetAxis( int id ) const {
	return mat3_identity;
}

/*
================
idPhysics_Base::SetLinearVelocity
================
*/
void idPhysics_Base::SetLinearVelocity( const idVec3 &newLinearVelocity, int id ) {
}

/*
================
idPhysics_Base::SetAngularVelocity
================
*/
void idPhysics_Base::SetAngularVelocity( const idVec3 &newAngularVelocity, int id ) {
}

/*
================
idPhysics_Base::GetLinearVelocity
================
*/
const idVec3 &idPhysics_Base::GetLinearVelocity( int id ) const {
	return vec3_origin;
}

/*
================
idPhysics_Base::GetAngularVelocity
================
*/
const idVec3 &idPhysics_Base::GetAngularVelocity( int id ) const {
	return vec3_origin;
}

/*
================
idPhysics_Base::SetGravity
================
*/
void idPhysics_Base::SetGravity( const idVec3 &newGravity ) {
	gravityVector = newGravity;
	gravityNormal = newGravity;
	gravityNormal.Normalize();
}

/*
================
idPhysics_Base::GetGravity
================
*/
const idVec3 &idPhysics_Base::GetGravity( void ) const {
	return gravityVector;
}

/*
================
idPhysics_Base::GetGravityNormal
================
*/
const idVec3 &idPhysics_Base::GetGravityNormal( void ) const {
	return gravityNormal;
}

/*
================
idPhysics_Base::ClipTranslation
================
*/
void idPhysics_Base::ClipTranslation( trace_t &results, const idVec3 &translation, const idClipModel *model ) const {
	memset( &results, 0, sizeof( trace_t ) );
}

/*
================
idPhysics_Base::ClipRotation
================
*/
void idPhysics_Base::ClipRotation( trace_t &results, const idRotation &rotation, const idClipModel *model ) const {
	memset( &results, 0, sizeof( trace_t ) );
}

/*
================
idPhysics_Base::ClipContents
================
*/
int idPhysics_Base::ClipContents( const idClipModel *model ) const {
	return 0;
}

/*
================
idPhysics_Base::UnlinkClip
================
*/
void idPhysics_Base::UnlinkClip( void ) {
}

/*
================
idPhysics_Base::LinkClip
================
*/
void idPhysics_Base::LinkClip( void ) {
}

/*
================
idPhysics_Base::DisableClip
================
*/
void idPhysics_Base::DisableClip( bool activateContacting ) {
}

/*
================
idPhysics_Base::EnableClip
================
*/
void idPhysics_Base::EnableClip( void ) {
}

/*
================
idPhysics_Base::EvaluateContacts
================
*/
bool idPhysics_Base::EvaluateContacts( CLIP_DEBUG_PARMS_DECLARATION_ONLY ) {
	return false;
}

/*
================
idPhysics_Base::GetNumContacts
================
*/
int idPhysics_Base::GetNumContacts( void ) const {
	return contacts.Num();
}

/*
================
idPhysics_Base::GetContact
================
*/
const contactInfo_t& idPhysics_Base::GetContact( int num ) const {
	return contacts[num];
}

/*
================
idPhysics_Base::ClearContacts
================
*/
void idPhysics_Base::ClearContacts( void ) {
	int i;
	idEntity *ent;

	for ( i = 0; i < contacts.Num(); i++ ) {
		ent = gameLocal.entities[ contacts[i].entityNum ];
		if ( ent ) {
			ent->RemoveContactEntity( self );
		}
	}
	contacts.SetNum( 0, false );
}

/*
================
idPhysics_Base::AddContactEntity
================
*/
void idPhysics_Base::AddContactEntity( idEntity *e ) {
	int i;
	idEntity *ent;
	bool found = false;

	for ( i = 0; i < contactEntities.Num(); i++ ) {
		ent = contactEntities[i].GetEntity();
		if ( ent == NULL ) {
			contactEntities.RemoveIndex( i-- );
		}
		if ( ent == e ) {
			found = true;
		}
	}
	if ( !found ) {
		contactEntities.Alloc() = e;
	}
}

/*
================
idPhysics_Base::RemoveContactEntity
================
*/
void idPhysics_Base::RemoveContactEntity( idEntity *e ) {
	int i;
	idEntity *ent;

	for ( i = 0; i < contactEntities.Num(); i++ ) {
		ent = contactEntities[i].GetEntity();
		if ( !ent ) {
			contactEntities.RemoveIndex( i-- );
			continue;
		}
		if ( ent == e ) {
			contactEntities.RemoveIndex( i-- );
			return;
		}
	}
}

/*
================
idPhysics_Base::HasGroundContacts
================
*/
bool idPhysics_Base::HasGroundContacts( void ) const {
	int i;

	for ( i = 0; i < contacts.Num(); i++ ) {
		if ( contacts[i].normal * -gravityNormal > 0.0f ) {
			return true;
		}
	}
	return false;
}

/*
================
idPhysics_Base::IsGroundEntity
================
*/
bool idPhysics_Base::IsGroundEntity( int entityNum ) const {
	int i;

	for ( i = 0; i < contacts.Num(); i++ ) {
		if ( contacts[i].entityNum == entityNum && ( contacts[i].normal * -gravityNormal > 0.0f ) ) {
			return true;
		}
	}
	return false;
}

/*
================
idPhysics_Base::IsGroundClipModel
================
*/
bool idPhysics_Base::IsGroundClipModel( int entityNum, int id ) const {
	int i;

	for ( i = 0; i < contacts.Num(); i++ ) {
		if ( contacts[i].entityNum == entityNum && contacts[i].id == id && ( contacts[i].normal * -gravityNormal > 0.0f ) ) {
			return true;
		}
	}
	return false;
}

/*
================
idPhysics_Base::SetPushed
================
*/
void idPhysics_Base::SetPushed( int deltaTime ) {
}

/*
================
idPhysics_Base::GetPushedLinearVelocity
================
*/
const idVec3 &idPhysics_Base::GetPushedLinearVelocity( const int id ) const {
	return vec3_origin;
}

/*
================
idPhysics_Base::GetPushedAngularVelocity
================
*/
const idVec3 &idPhysics_Base::GetPushedAngularVelocity( const int id ) const {
	return vec3_origin;
}

/*
================
idPhysics_Base::SetMaster
================
*/
void idPhysics_Base::SetMaster( idEntity *master, const bool orientated ) {
}

/*
================
idPhysics_Base::GetBlockingInfo
================
*/
const trace_t *idPhysics_Base::GetBlockingInfo( void ) const {
	return NULL;
}

/*
================
idPhysics_Base::GetBlockingEntity
================
*/
idEntity *idPhysics_Base::GetBlockingEntity( void ) const {
	return NULL;
}

/*
================
idPhysics_Base::GetLinearEndTime
================
*/
int idPhysics_Base::GetLinearEndTime( void ) const {
	return 0;
}

/*
================
idPhysics_Base::GetAngularEndTime
================
*/
int idPhysics_Base::GetAngularEndTime( void ) const {
	return 0;
}

/*
================
idPhysics_Base::AddGroundContacts
================
*/
void idPhysics_Base::AddGroundContacts( CLIP_DEBUG_PARMS_DECLARATION const idClipModel *clipModel, unsigned int max ) {
	assert( max > 0 );

	int index = contacts.Num();
	contacts.SetNum( index + max, false );

	idVec3 dir = gravityNormal;
//	int num = GetTraceCollection().Contacts( CLIP_DEBUG_PARMS_PASSTHRU &contacts[index], max, clipModel->GetOrigin(),
//											&dir, CONTACT_EPSILON, clipModel, clipModel->GetAxis(), clipMask );
//	contacts.SetNum( index + num, false );

	trace_t tr;
	idVec3 start = clipModel->GetOrigin() - dir * CONTACT_EPSILON;
	idVec3 end = start + dir * CONTACT_EPSILON * 2.0f;
	GetTraceCollection().Translation( CLIP_DEBUG_PARMS_PASSTHRU tr, start, end, clipModel, clipModel->GetAxis(), clipMask );
	if ( tr.fraction < 1.0f ) {
		contacts.SetNum( index + 1, false );
		contacts[ index ] = tr.c;
	} else {
		contacts.SetNum( index, false );
	}
}

/*
================
idPhysics_Base::AddContactEntitiesForContacts
================
*/
void idPhysics_Base::AddContactEntitiesForContacts( void ) {
	int i;
	idEntity *ent;

	for ( i = 0; i < contacts.Num(); i++ ) {
		ent = gameLocal.entities[ contacts[i].entityNum ];
		if ( ent && ent != self ) {
			ent->AddContactEntity( self );
		}
	}
}

/*
================
idPhysics_Base::ActivateContactEntities
================
*/
void idPhysics_Base::ActivateContactEntities( void ) {
	int i;
	idEntity *ent;

	for ( i = 0; i < contactEntities.Num(); i++ ) {
		ent = contactEntities[i].GetEntity();
		if ( ent ) {
			ent->ActivatePhysics();
		} else {
			contactEntities.RemoveIndex( i-- );
		}
	}
}

/*
================
idPhysics_Base::IsOutsideWorld
================
*/
bool idPhysics_Base::IsOutsideWorld( void ) const {
	if ( !gameLocal.clip.GetWorldBounds().Expand( 128.0f ).IntersectsBounds( GetAbsBounds() ) ) {
		return true;
	}
	return false;
}

/*
================
idPhysics_Base::DrawVelocity
================
*/
void idPhysics_Base::DrawVelocity( int id, float linearScale, float angularScale ) const {
	idVec3 dir, org, vec, start, end;
	idMat3 axis;
	float length, a;

	dir = GetLinearVelocity( id );
	dir *= linearScale;
	if ( dir.LengthSqr() > Square( 0.1f ) ) {
		dir.Truncate( 10.0f );
		org = GetOrigin( id );
		gameRenderWorld->DebugArrow( colorRed, org, org + dir, r_debugAxisLength.GetInteger() );
	}

	dir = GetAngularVelocity( id );
	length = dir.Normalize();
	length *= angularScale;
	if ( length > 0.1f ) {
		if ( length < 60.0f ) {
			length = 60.0f;
		}
		else if ( length > 360.0f ) {
			length = 360.0f;
		}
		axis = GetAxis( id );
		vec = axis[2];
		if ( idMath::Fabs( dir * vec ) > 0.99f ) {
			vec = axis[0];
		}
		vec -= vec * dir * vec;
		vec.Normalize();
		vec *= 4.0f;
		start = org + vec;
		for ( a = 20.0f; a < length; a += 20.0f ) {
			end = org + idRotation( vec3_origin, dir, -a ).ToMat3() * vec;
			gameRenderWorld->DebugLine( colorBlue, start, end, r_debugAxisLength.GetInteger() );
			start = end;
		}
		end = org + idRotation( vec3_origin, dir, -length ).ToMat3() * vec;
		gameRenderWorld->DebugArrow( colorBlue, start, end, r_debugAxisLength.GetInteger() );
	}
}

/*
================
idPhysics_Base::GetTraceCollection
================
*/
const sdClipModelCollection& idPhysics_Base::GetTraceCollection( void ) {
	if ( g_useTraceCollection.GetBool() ) {
		traceCollection.Update( GetBounds(), GetOrigin(), GetAxis(), GetLinearVelocity(), 
								GetAngularVelocity(), GetClipMask(), self );
	}
	return traceCollection;
}

/*
============
sdClipModelCollection::SetSelf
============
*/
void sdClipModelCollection::SetSelf( idEntity* e ) {
	assert( e );
	self = e;
}

/*
============
sdClipModelCollection::Update
============
*/
void sdClipModelCollection::Update( const idBounds& bounds, const idVec3& origin, const idMat3& axis, 
									const idVec3& linearVelocity, const idVec3& angularVelocity, 
									int clipMask, idEntity* passEntity ) {

	if ( gameLocal.time != lastUpdate ) {
		// construct a bounds from the movement
		idBounds currentBounds;
		currentBounds.FromTransformedBounds( bounds, origin, axis );

		// estimate future position & orientation
		float timeStep = MS2SEC( gameLocal.msec );
		idVec3 futurePosition = origin + linearVelocity * timeStep;
		idVec3 angDelta = angularVelocity * timeStep;
		idVec3 rotationAxis = angDelta;
		float rotationAngle = RAD2DEG( rotationAxis.Normalize() );
		idRotation rotation( vec3_origin, rotationAxis, rotationAngle );
		idMat3 futureOrientation = axis * rotation.ToMat3();

		idBounds futureBounds;
		futureBounds.FromTransformedBounds( bounds, futurePosition, futureOrientation );

		idBounds totalBounds = currentBounds;
		totalBounds.AddBounds( futureBounds );

		// expand it a bit
		totalBounds.ExpandSelf( 64.0f );


		// find all the clipmodels touching the bounds (including the world)
		collection.AssureSize( MAX_GENTITIES );
		int num = gameLocal.clip.ClipModelsTouchingBounds( CLIP_DEBUG_PARMS totalBounds, clipMask, &collection.Front(), MAX_GENTITIES, passEntity, true );
		collection.SetNum( num, false );

//		gameRenderWorld->DebugBounds( colorRed, totalBounds );
	}

	lastUpdate = gameLocal.time;
}

/*
============
sdClipModelCollection::RemoveEntitiesOfCollection
============
*/
void sdClipModelCollection::RemoveEntitiesOfCollection( const char* collectionName ) {
	const sdEntityCollection* entCollection = gameLocal.GetEntityCollection( collectionName );
	if ( entCollection == NULL ) {
		return;
	}

	for ( int i = 0; i < collection.Num(); i++ ) {
		const idClipModel* model = collection[ i ];

		if ( model->GetEntity() == NULL ) {
			continue;
		}

		if ( entCollection->Contains( model->GetEntity() ) ) {
			collection.RemoveIndexFast( i );
			i--;
		}
	}
}

/*
============
sdClipModelCollection::ForceNextUpdate
============
*/
void sdClipModelCollection::ForceNextUpdate() {
	lastUpdate = -1;
}

/*
============
sdClipModelCollection::Translation
============
*/
bool sdClipModelCollection::TracePoint( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, 
									   const idVec3 &end, int contentMask ) const {
	return Translation( CLIP_DEBUG_PARMS_PASSTHRU results, start, end, NULL, mat3_identity, contentMask );
}

/*
============
sdClipModelCollection::Translation
============
*/
bool sdClipModelCollection::Translation( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idVec3 &end,
											const idClipModel *mdl, const idMat3 &trmAxis, int contentMask ) const {

	if ( !g_useTraceCollection.GetBool() ) {
		return gameLocal.clip.Translation( CLIP_DEBUG_PARMS_PASSTHRU results, start, end, mdl, trmAxis, contentMask, self );
	}

	// TWTODO: Add verification stuff to ensure no points of the clip model are outside the bounds that were checked over

	idBounds traceBounds;
	if ( mdl == NULL ) {
		traceBounds.FromPointTranslation( start, end - start );
	} else {
		traceBounds.FromBoundsTranslation( mdl->GetBounds(), start, trmAxis, end - start );
	}
	traceBounds.ExpandSelf( CM_BOX_EPSILON );

	results.fraction = 1.0f;
	results.endpos = end;
	results.endAxis = trmAxis;
	for ( int i = 0; i < collection.Num(); i++ ) {
		const idClipModel* model = collection[ i ];

		bool isDead = false;
		if ( model->GetEntity() == NULL ) {
			if ( model->GetCollisionModel() != NULL ) {
				isDead = !model->GetCollisionModel()->IsWorld();
			} else {
				isDead = true;
			}
		}

		if ( isDead ) {
			// the entity has been removed between updating the collection & now
			continue;
		}

		if ( !model->GetAbsBounds().IntersectsBounds( traceBounds ) ) {
			continue;
		}

		if ( !( model->GetContents() & contentMask ) ) {
			continue;
		}

		trace_t trace;
		gameLocal.clip.TranslationModel( CLIP_DEBUG_PARMS_PASSTHRU trace, start, end, mdl, trmAxis, contentMask, model, model->GetOrigin(), model->GetAxis() );
		if ( trace.fraction < results.fraction ) {
			results = trace;
		}
	}

	if ( mdl != NULL ) {
		results.c.selfId = mdl->GetId();
	} else {
		results.c.selfId = 0;
	}
	return ( results.fraction != 1.0f );
}


/*
============
sdClipModelCollection::Rotation
============
*/
bool sdClipModelCollection::Rotation( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idRotation &rotation,
										const idClipModel *mdl, const idMat3 &trmAxis, int contentMask ) const {

	if ( !g_useTraceCollection.GetBool() ) {
		return gameLocal.clip.Rotation( CLIP_DEBUG_PARMS_PASSTHRU results, start, rotation, mdl, trmAxis, contentMask, self );
	}

	// TWTODO: Add verification stuff to ensure no points of the clip model are outside the bounds that were checked over

	idBounds traceBounds;
	if ( mdl == NULL ) {
		traceBounds.FromPointTranslation( start, vec3_origin );
	} else {
		traceBounds.FromBoundsTranslation( mdl->GetBounds(), start, trmAxis, vec3_origin );
	}
	traceBounds.ExpandSelf( CM_BOX_EPSILON );

	results.fraction = 1.0f;
	results.endpos = start;
	results.endAxis = trmAxis * rotation.ToMat3();
	rotation.RotatePoint( results.endpos );

	for ( int i = 0; i < collection.Num(); i++ ) {
		const idClipModel* model = collection[ i ];

		bool isDead = false;
		if ( model->GetEntity() == NULL ) {
			if ( model->GetCollisionModel() != NULL ) {
				isDead = !model->GetCollisionModel()->IsWorld();
			} else {
				isDead = true;
			}
		}

		if ( isDead ) {
			// the entity has been removed between updating the collection & now
			continue;
		}

		if ( !model->GetAbsBounds().IntersectsBounds( traceBounds ) ) {
			continue;
		}

		if ( !( model->GetContents() & contentMask ) ) {
			continue;
		}

		trace_t trace;
		gameLocal.clip.RotationModel( CLIP_DEBUG_PARMS_PASSTHRU trace, start, rotation, mdl, trmAxis, contentMask, model, model->GetOrigin(), model->GetAxis() );
		if ( trace.fraction < results.fraction ) {
			results = trace;
		}
	}

	if ( mdl != NULL ) {
		results.c.selfId = mdl->GetId();
	} else {
		results.c.selfId = 0;
	}
	return ( results.fraction != 1.0f );
}

/*
============
sdClipModelCollection::Contacts
============
*/
int sdClipModelCollection::Contacts( CLIP_DEBUG_PARMS_DECLARATION contactInfo_t *contacts, const int maxContacts, 
										const idVec3 &start, const idVec3 *dir, const float depth,
										const idClipModel *mdl, const idMat3 &trmAxis, int contentMask ) const {

	if ( !g_useTraceCollection.GetBool() ) {
		return gameLocal.clip.Contacts( CLIP_DEBUG_PARMS_PASSTHRU contacts, maxContacts, start, dir, depth, mdl, trmAxis, contentMask, self );
	}

	idBounds traceBounds;
	if ( mdl != NULL ) {
		traceBounds = mdl->GetBounds();
		traceBounds.RotateSelf( trmAxis );
	} else {
		traceBounds[ 0 ].Set( -CM_BOX_EPSILON, -CM_BOX_EPSILON, -CM_BOX_EPSILON );
		traceBounds[ 1 ].Set( CM_BOX_EPSILON, CM_BOX_EPSILON, CM_BOX_EPSILON );
	}

	traceBounds.TranslateSelf( start );
	traceBounds.ExpandSelf( idMath::Fabs( depth ) + CM_BOX_EPSILON );

	int numContacts = 0;
	for ( int i = 0; i < collection.Num(); i++ ) {
		const idClipModel* model = collection[ i ];

		bool isDead = false;
		if ( model->GetEntity() == NULL ) {
			if ( model->GetCollisionModel() != NULL ) {
				isDead = !model->GetCollisionModel()->IsWorld();
			} else {
				isDead = true;
			}
		}

		if ( isDead ) {
			// the entity has been removed between updating the collection & now
			continue;
		}

		if ( !model->GetAbsBounds().IntersectsBounds( traceBounds ) ) {
			continue;
		}

		if ( !( model->GetContents() & contentMask ) ) {
			continue;
		}

		contactInfo_t* contactStart = contacts + numContacts;
		int numClipContacts = gameLocal.clip.ContactsModel( CLIP_DEBUG_PARMS_PASSTHRU contactStart, maxContacts - numContacts, start, dir, depth, mdl, trmAxis, contentMask, model, model->GetOrigin(), model->GetAxis() );
		for ( int j = 0; j < numClipContacts; j++ ) {
			contactStart[ j ].entityNum = ( model->GetCollisionModel()->IsWorld() ) ? ENTITYNUM_WORLD : model->GetEntity()->entityNumber;;
			contactStart[ j ].id = model->GetId();
			contactStart[ j ].selfId = 0;
		}

		numContacts += numClipContacts;
	}

	return numContacts;
}

/*
============
sdClipModelCollection::Contents
============
*/
int sdClipModelCollection::Contents( CLIP_DEBUG_PARMS_DECLARATION const idVec3 &start,
										const idClipModel *mdl, const idMat3 &trmAxis, int contentMask ) const {
	if ( !g_useTraceCollection.GetBool() ) {
		return gameLocal.clip.Contents( CLIP_DEBUG_PARMS_PASSTHRU start, mdl, trmAxis, contentMask, self );
	}

	idBounds traceBounds;
	if ( mdl != NULL ) {
		traceBounds = mdl->GetBounds();
		traceBounds.RotateSelf( trmAxis );
	} else {
		traceBounds[ 0 ].Set( -CM_BOX_EPSILON, -CM_BOX_EPSILON, -CM_BOX_EPSILON );
		traceBounds[ 1 ].Set( CM_BOX_EPSILON, CM_BOX_EPSILON, CM_BOX_EPSILON );
	}

	traceBounds.TranslateSelf( start );
	traceBounds.ExpandSelf( CM_BOX_EPSILON );

	int contents = 0;
	for ( int i = 0; i < collection.Num(); i++ ) {
		const idClipModel* model = collection[ i ];

		bool isDead = false;
		if ( model->GetEntity() == NULL ) {
			if ( model->GetCollisionModel() != NULL ) {
				isDead = !model->GetCollisionModel()->IsWorld();
			} else {
				isDead = true;
			}
		}

		if ( isDead ) {
			// the entity has been removed between updating the collection & now
			continue;
		}

		if ( !model->GetAbsBounds().IntersectsBounds( traceBounds ) ) {
			continue;
		}

		if ( !( model->GetContents() & contentMask ) ) {
			continue;
		}

		contents |= gameLocal.clip.ContentsModel( CLIP_DEBUG_PARMS_PASSTHRU start, mdl, trmAxis, contentMask, model, model->GetOrigin(), model->GetAxis()  );
	}

	return contents;
}
