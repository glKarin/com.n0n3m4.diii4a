// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Force_Drag.h"
#include "Physics.h"
#include "Clip.h"

CLASS_DECLARATION( idForce, idForce_Drag )
END_CLASS

/*
================
idForce_Drag::idForce_Drag
================
*/
idForce_Drag::idForce_Drag( void ) {
	damping			= 0.5f;
	dragPosition	= vec3_zero;
	physics			= NULL;
	id				= 0;
	p				= vec3_zero;
	dragPosition	= vec3_zero;
	maxForce		= 100.f;
}

/*
================
idForce_Drag::~idForce_Drag
================
*/
idForce_Drag::~idForce_Drag( void ) {
}

/*
================
idForce_Drag::Init
================
*/
void idForce_Drag::Init( float damping, float _maxForce ) {
	if ( damping >= 0.0f && damping < 1.0f ) {
		this->damping = damping;
	}

	maxForce = _maxForce;
}

/*
================
idForce_Drag::SetPhysics
================
*/
void idForce_Drag::SetPhysics( idPhysics *phys, int id, const idVec3 &p ) {
	this->physics = phys;
	this->id = id;
	this->p = p;
}

/*
================
idForce_Drag::SetDragPosition
================
*/
void idForce_Drag::SetDragPosition( const idVec3 &pos ) {
	this->dragPosition = pos;
}

/*
================
idForce_Drag::GetDragPosition
================
*/
const idVec3 &idForce_Drag::GetDragPosition( void ) const {
	return this->dragPosition;
}

/*
================
idForce_Drag::GetDraggedPosition
================
*/
const idVec3 idForce_Drag::GetDraggedPosition( void ) const {
	return ( physics->GetOrigin( id ) + p * physics->GetAxis( id ) );
}

/*
================
idForce_Drag::Evaluate
================
*/
void idForce_Drag::Evaluate( int time ) {
	float l1, l2, mass;
	idVec3 dragOrigin, dir1, dir2, velocity, centerOfMass;
	idMat3 inertiaTensor;
	idRotation rotation;
	idClipModel *clipModel;

	if ( !physics ) {
		return;
	}

	clipModel = physics->GetClipModel( id );
	if ( clipModel != NULL && clipModel->IsTraceModel() ) {
		clipModel->GetMassProperties( 1.0f, mass, centerOfMass, inertiaTensor );
	} else {
		centerOfMass.Zero();
	}

	centerOfMass = physics->GetOrigin( id ) + centerOfMass * physics->GetAxis( id );
	dragOrigin = physics->GetOrigin( id ) + p * physics->GetAxis( id );

	dir1 = dragPosition - centerOfMass;
	dir2 = dragOrigin - centerOfMass;
	l1 = dir1.Normalize();
	l2 = dir2.Normalize();

/*	rotation.Set( centerOfMass, dir2.Cross( dir1 ), RAD2DEG( idMath::ACos( dir1 * dir2 ) ) );
	physics->SetAngularVelocity( physics->GetAngularVelocity() + ( rotation.ToAngularVelocity() / MS2SEC( gameLocal.msec ) ), id );

	idVec3 acceleration = dir1 * ( l1 - l2 ) / MS2SEC( gameLocal.msec );
	idVec3 force = acceleration * physics->GetMass();
	force.Truncate( maxForce );
	acceleration = force / physics->GetMass();

	physics->SetLinearVelocity( physics->GetLinearVelocity( id ) + acceleration * MS2SEC( gameLocal.msec ), id );*/

	idVec3 dir = dragPosition - dragOrigin;
	float len = dir.Normalize();

	len /= 128.f;
	if ( len < 1.f ) {
		len = 1.f;
	}

	dir *= maxForce / len;

	physics->AddForce( 0, dragOrigin, dir );
}

/*
================
idForce_Drag::RemovePhysics
================
*/
void idForce_Drag::RemovePhysics( const idPhysics *phys ) {
	if ( physics == phys ) {
		physics = NULL;
	}
}
