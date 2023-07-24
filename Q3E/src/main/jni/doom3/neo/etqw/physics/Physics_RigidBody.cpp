// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Physics_RigidBody.h"
#include "../Entity.h"
#include "../Player.h"
#include "../ContentMask.h"

CLASS_DECLARATION( idPhysics_Base, idPhysics_RigidBody )
END_CLASS

const float STOP_SPEED		= 10.0f;

#undef RB_TIMINGS

#ifdef RB_TIMINGS
static int lastTimerReset = 0;
static int numRigidBodies = 0;
static idTimer timer_total, timer_collision;
#endif

/*
================
sdRigidBodyNetworkState::MakeDefault
================
*/
void sdRigidBodyNetworkState::MakeDefault( void ) {
	position		= vec3_origin;
	orientation.x	= 0.f;
	orientation.y	= 0.f;
	orientation.z	= 0.f;
	linearVelocity	= vec3_zero;
	angularVelocity = vec3_zero;
}

/*
================
sdRigidBodyNetworkState::Write
================
*/
void sdRigidBodyNetworkState::Write( idFile* file ) const {
	file->WriteVec3( position );
	file->WriteCQuat( orientation );
	file->WriteVec3( linearVelocity );
	file->WriteVec3( angularVelocity );
}

/*
================
sdRigidBodyNetworkState::Read
================
*/
void sdRigidBodyNetworkState::Read( idFile* file ) {
	file->ReadVec3( position );
	file->ReadCQuat( orientation );
	file->ReadVec3( linearVelocity );
	file->ReadVec3( angularVelocity );

	position.FixDenormals();
	orientation.FixDenormals();
	angularVelocity.FixDenormals();
	linearVelocity.FixDenormals();
}


/*
================
sdRigidBodyBroadcastState::MakeDefault
================
*/
void sdRigidBodyBroadcastState::MakeDefault( void ) {
	localPosition		= vec3_origin;
	localOrientation.x	= 0.f;
	localOrientation.y	= 0.f;
	localOrientation.z	= 0.f;
	atRest				= -1;
}

/*
================
sdRigidBodyBroadcastState::Write
================
*/
void sdRigidBodyBroadcastState::Write( idFile* file ) const {
	file->WriteVec3( localPosition );
	file->WriteCQuat( localOrientation );
	file->WriteInt( atRest );
}

/*
================
sdRigidBodyBroadcastState::Read
================
*/
void sdRigidBodyBroadcastState::Read( idFile* file ) {
	file->ReadVec3( localPosition );
	file->ReadCQuat( localOrientation );
	file->ReadInt( atRest );
}

/*
================
RigidBodyDerivatives
================
*/
void RigidBodyDerivatives( const float t, const void *clientData, const float *state, float *derivatives ) {
	const idPhysics_RigidBody *p = (idPhysics_RigidBody *) clientData;
	rigidBodyIState_t *s = (rigidBodyIState_t *) state;
	// NOTE: this struct should be build conform rigidBodyIState_t
	struct rigidBodyDerivatives_s {
		idVec3				linearVelocity;
		idMat3				angularMatrix;
		idVec3				force;
		idVec3				torque;
	} *d = (struct rigidBodyDerivatives_s *) derivatives;
	idVec3 angularVelocity;
	idMat3 inverseWorldInertiaTensor;

	inverseWorldInertiaTensor = s->orientation * p->inverseInertiaTensor * s->orientation.Transpose();
	angularVelocity = inverseWorldInertiaTensor * s->angularMomentum;
	// derivatives
	d->linearVelocity = p->inverseMass * s->linearMomentum;
	d->angularMatrix = SkewSymmetric( angularVelocity ) * s->orientation;

	float linearFriction = p->waterLevel ? p->linearFrictionWater : p->linearFriction;
	float angularFriction = p->waterLevel ? p->angularFrictionWater : p->angularFriction;
	d->force = - linearFriction * s->linearMomentum + p->current.externalForce;
	d->torque = - angularFriction * s->angularMomentum + p->current.externalTorque;
}

/*
================
idPhysics_RigidBody::Integrate

  Calculate next state from the current state using an integrator.
================
*/
void idPhysics_RigidBody::Integrate( float deltaTime, rigidBodyPState_t &next ) {
	idVec3 position;

	position = current.i.position;
	current.i.position += centerOfMass * current.i.orientation;

	current.i.orientation.TransposeSelf();

	integrator->Evaluate( (float *) &current.i, (float *) &next.i, 0, deltaTime );
	next.i.orientation.OrthoNormalizeSelf();

	// apply gravity
	next.i.linearMomentum += deltaTime * gravityVector * mass;

	current.i.orientation.TransposeSelf();
	next.i.orientation.TransposeSelf();

	current.i.position = position;
	next.i.position -= centerOfMass * next.i.orientation;

	next.atRest = current.atRest;
}

/*
================
idPhysics_RigidBody::CollisionImpulse

  Calculates the collision impulse using the velocity relative to the collision object.
  The current state should be set to the moment of impact.
================
*/
bool idPhysics_RigidBody::CollisionImpulse( const trace_t &collision, idVec3 &impulse ) {
	// get info from other entity involved
	impactInfo_t info;
	idEntity* ent = gameLocal.entities[collision.c.entityNum];
	idPhysics* phys = ent->GetPhysics();
	ent->GetImpactInfo( self, collision.c.id, collision.c.point, &info );


	// gather information
	float	e		= bouncyness;
	idVec3	normal	= collision.c.normal;

	idVec3	v1A		= inverseMass * current.i.linearMomentum;
	idMat3	invIA	= current.i.orientation.Transpose() * inverseInertiaTensor * current.i.orientation;
	idVec3	w1A		= invIA * current.i.angularMomentum;
	idVec3	rAP		= collision.c.point - ( current.i.position + centerOfMass * current.i.orientation );
	v1A = v1A + w1A.Cross( rAP );

	float	normalVel	= v1A * normal;

	idMat3	invIB	= info.invInertiaTensor;
	idVec3	rBP		= info.position;
	idVec3	v1B		= info.velocity;

	idVec3	v1AB	= v1A - v1B;
	
	float j = STOP_SPEED;


	if ( normalVel > -STOP_SPEED ) {
		j = -normalVel;
	} else {
		// evaluate
		float	vNorm				= v1AB * normal;

		float	invMassA			= inverseMass;
		float	invMassB			= info.invMass;

		float	invMassSum			= invMassA + invMassB;

		idVec3	radiusFactorA		= ( invIA * rAP.Cross( normal ) ).Cross( rAP );
		idVec3	radiusFactorB		= ( invIB * rBP.Cross( normal ) ).Cross( rBP );
		float	radiusFactors		= ( radiusFactorA + radiusFactorB ) * normal;

		float	numerator			= -( 1.0f + e ) * vNorm;
		float	denominator			= invMassSum + radiusFactors;

		j							= numerator / denominator;

		if ( j < -normalVel ) {
			j = -normalVel;
		}
	}

	impulse = j * normal;

	// update linear and angular momentum with impulse
	current.i.linearMomentum += impulse;
	current.i.angularMomentum += rAP.Cross(impulse);

	// if no movement at all don't blow up
	if ( collision.fraction < 0.0001f ) {

		float normalMomentum = current.i.linearMomentum * normal;
		if ( normalMomentum < 0.0f ) {
			// only scale the component of linear momentum that is towards the normal!!
			current.i.linearMomentum -= normalMomentum * normal;
			normalMomentum *= 0.25f;
			current.i.linearMomentum += normalMomentum * normal;
		}
		current.i.angularMomentum *= 0.5f;
	}

	// callback to self to let the entity know about the collision
	ent->Hit( collision, v1A, self );
	return self->Collide( collision, v1A, -1 );
}

/*
================
idPhysics_RigidBody::CheckForCollisions

  Check for collisions between the current and next state.
  If there is a collision the next state is set to the state at the moment of impact.
================
*/
bool idPhysics_RigidBody::CheckForCollisions( const float deltaTime, rigidBodyPState_t &next, trace_t &collision ) {
//#define TEST_COLLISION_DETECTION
	idMat3 axis;
	idRotation rotation;
	bool collided = false;
	collision.fraction = 1.0f;

#ifdef TEST_COLLISION_DETECTION
	bool startsolid = false;
	if ( gameLocal.clip.Contents( current.i.position, clipModel, current.i.orientation, clipMask, self ) ) {
		startsolid = true;
	}
#endif

	// calculate the position of the center of mass at current and next
	idVec3 CoMstart = current.i.position + centerOfMass * current.i.orientation;
	idVec3 CoMend = next.i.position + centerOfMass * next.i.orientation;

//	gameRenderWorld->DebugSphere( colorYellow, idSphere( CoMstart, 1.0f ) );
//	gameRenderWorld->DebugSphere( colorGreen, idSphere( CoMend, 1.0f ) );

	TransposeMultiply( current.i.orientation, next.i.orientation, axis );
	rotation = axis.ToRotation();
	rotation.SetOrigin( CoMstart );

	// if there was a collision
	if ( gameLocal.clip.Motion( CLIP_DEBUG_PARMS_ENTINFO( self ) collision, CoMstart, CoMend, rotation, centeredClipModel, current.i.orientation, clipMask, self ) ) {
		// adjust the collision's end pos back into the entity space
		idVec3 temp = collision.endpos - centerOfMass * collision.endAxis;
//gameRenderWorld->DebugSphere( idVec4( 1.0f, 0.0f, 1.0f, 1.0f ), idSphere( temp, 1.0f ) );
//gameRenderWorld->DebugSphere( idVec4( 1.0f, 1.0f, 1.0f, 1.0f ), idSphere( collision.endpos, 1.0f ) );
//gameRenderWorld->DebugSphere( idVec4( 0.0f, 0.0f, 1.0f, 1.0f ), idSphere( collision.c.point, 1.0f ) );

		// set the next state to the state at the moment of impact
		next.i.position = temp;
		next.i.orientation = collision.endAxis;
		next.i.linearMomentum = current.i.linearMomentum;
		next.i.angularMomentum = current.i.angularMomentum;
		collided = true;
	}

#ifdef TEST_COLLISION_DETECTION
	if ( gameLocal.clip.Contents( next.i.position, clipModel, next.i.orientation, clipMask, self ) ) {
		if ( !startsolid ) {
			int bah = 1;
		}
	}
#endif
	return collided;
}

/*
================
idPhysics_RigidBody::ContactFriction

  Does not solve friction for multiple simultaneous contacts but applies contact friction in isolation.
  Uses absolute velocity at the contact points instead of the velocity relative to the contact object.
================
*/
void idPhysics_RigidBody::ContactFriction( float deltaTime ) {
	int i;
	float magnitude, impulseNumerator, impulseDenominator;
	idMat3 inverseWorldInertiaTensor;
	idVec3 linearVelocity, angularVelocity;
	idVec3 massCenter, r, velocity, normal, impulse, normalVelocity;

	inverseWorldInertiaTensor = current.i.orientation.Transpose() * inverseInertiaTensor * current.i.orientation;

	massCenter = current.i.position + centerOfMass * current.i.orientation;

	for ( i = 0; i < contacts.Num(); i++ ) {

		r = contacts[i].point - massCenter;

		// calculate velocity at contact point
		linearVelocity = inverseMass * current.i.linearMomentum;
		angularVelocity = inverseWorldInertiaTensor * current.i.angularMomentum;
		velocity = linearVelocity + angularVelocity.Cross(r);

		// velocity along normal vector
		normalVelocity = ( velocity * contacts[i].normal ) * contacts[i].normal;

		// calculate friction impulse
		normal = -( velocity - normalVelocity );
		magnitude = normal.Normalize();
		impulseNumerator = contactFriction * magnitude;
		impulseDenominator = inverseMass + ( ( inverseWorldInertiaTensor * r.Cross( normal ) ).Cross( r ) * normal );
		impulse = (impulseNumerator / impulseDenominator) * normal;

		// apply friction impulse
		current.i.linearMomentum += impulse;
		current.i.angularMomentum += r.Cross(impulse);

		// if moving towards the surface at the contact point
		if ( normalVelocity * contacts[i].normal < 0.0f ) {
			// calculate impulse
			normal = -normalVelocity;
			impulseNumerator = normal.Normalize();
			impulseDenominator = inverseMass + ( ( inverseWorldInertiaTensor * r.Cross( normal ) ).Cross( r ) * normal );
			impulse = (impulseNumerator / impulseDenominator) * normal;

			// apply impulse
			current.i.linearMomentum += impulse;
			current.i.angularMomentum += r.Cross( impulse );
		}
	}
}

/*
================
idPhysics_RigidBody::TestIfAtRest

  Returns true if the body is considered at rest.
  Does not catch all cases where the body is at rest but is generally good enough.
================
*/
bool idPhysics_RigidBody::TestIfAtRest( void ) const {
	int i;
	float gv;
	idVec3 v, av, normal, point;
	idMat3 inverseWorldInertiaTensor;
	idFixedWinding contactWinding;

	if ( current.atRest >= 0 ) {
		return true;
	}

	// need at least 3 contact points to come to rest
	if ( contacts.Num() < 3 ) {
		return false;
	}

	// get average contact plane normal
	normal.Zero();
	for ( i = 0; i < contacts.Num(); i++ ) {
		normal += contacts[i].normal;
	}
	normal /= (float) contacts.Num();
	normal.Normalize();

	// if on a too steep surface
	if ( (normal * gravityNormal) > -0.7f ) {
		return false;
	}

	// create bounds for contact points
	contactWinding.Clear();
	for ( i = 0; i < contacts.Num(); i++ ) {
		// project point onto plane through origin orthogonal to the gravity
		point = contacts[i].point - (contacts[i].point * gravityNormal) * gravityNormal;
		contactWinding.AddToConvexHull( point, gravityNormal );
	}

	// need at least 3 contact points to come to rest
	if ( contactWinding.GetNumPoints() < 3 ) {
		return false;
	}

	// center of mass in world space
	point = current.i.position + centerOfMass * current.i.orientation;
	point -= (point * gravityNormal) * gravityNormal;

	// if the point is not inside the winding
	if ( !contactWinding.PointInside( gravityNormal, point, 0 ) ) {
		return false;
	}

	// linear velocity of body
	v = inverseMass * current.i.linearMomentum;
	// linear velocity in gravity direction
	gv = v * gravityNormal;
	// linear velocity orthogonal to gravity direction
	v -= gv * gravityNormal;

	// if too much velocity orthogonal to gravity direction
	if ( v.Length() > STOP_SPEED ) {
		return false;
	}
	// if too much velocity in gravity direction
	if ( gv > 2.0f * STOP_SPEED || gv < -2.0f * STOP_SPEED ) {
		return false;
	}

	// calculate rotational velocity
	inverseWorldInertiaTensor = current.i.orientation * inverseInertiaTensor * current.i.orientation.Transpose();
	av = inverseWorldInertiaTensor * current.i.angularMomentum;

	// if too much rotational velocity
	if ( av.LengthSqr() > STOP_SPEED ) {
		return false;
	}

	return true;
}

/*
================
idPhysics_RigidBody::DropToFloorAndRest

  Drops the object straight down to the floor and verifies if the object is at rest on the floor.
================
*/
void idPhysics_RigidBody::DropToFloorAndRest( void ) {
	idVec3 down;
	trace_t tr;

	if ( testSolid ) {

		testSolid = false;

		if ( gameLocal.clip.Contents( CLIP_DEBUG_PARMS_ENTINFO( self ) current.i.position, clipModel, current.i.orientation, clipMask, self ) ) {
			gameLocal.DWarning( "rigid body in solid for entity '%s' type '%s' at (%s)",
								self->name.c_str(), self->GetType()->classname, current.i.position.ToString(0) );
			Rest();
			dropToFloor = false;
			return;
		}
	}

	// put the body on the floor
	down = current.i.position + gravityNormal * 128.0f;
	gameLocal.clip.Translation( CLIP_DEBUG_PARMS_ENTINFO( self ) tr, current.i.position, down, clipModel, current.i.orientation, clipMask, self );
	current.i.position = tr.endpos;
	clipModel->Link( gameLocal.clip, self, clipModel->GetId(), tr.endpos, current.i.orientation );

	// if on the floor already
	if ( tr.fraction == 0.0f ) {
		// test if we are really at rest
		EvaluateContacts( CLIP_DEBUG_PARMS_ENTINFO_ONLY( self ) );
		if ( !TestIfAtRest() ) {
			gameLocal.DWarning( "rigid body not at rest for entity '%s' type '%s' at (%s)",
								self->name.c_str(), self->GetType()->classname, current.i.position.ToString(0) );
		}
		Rest();
		dropToFloor = false;
	} else if ( IsOutsideWorld() ) {
//		gameLocal.Warning( "rigid body outside world bounds for entity '%s' type '%s' at (%s)",
//							self->name.c_str(), self->GetType()->classname, current.i.position.ToString(0) );
		Rest();
		dropToFloor = false;
	}
}

/*
================
idPhysics_RigidBody::DebugDraw
================
*/
void idPhysics_RigidBody::DebugDraw( void ) {

	if ( rb_showBodies.GetBool() || ( rb_showActive.GetBool() && current.atRest < 0 ) ) {
		clipModel->Draw();
	}

	if ( rb_showMass.GetBool() ) {
		gameRenderWorld->DrawText( va( "\n%1.2f", mass ), current.i.position, 0.08f, colorCyan, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1 );
	}

	if ( rb_showInertia.GetBool() ) {
		idMat3 &I = inertiaTensor;
		gameRenderWorld->DrawText( va( "\n\n\n( %.1f %.1f %.1f )\n( %.1f %.1f %.1f )\n( %.1f %.1f %.1f )",
									I[0].x, I[0].y, I[0].z,
									I[1].x, I[1].y, I[1].z,
									I[2].x, I[2].y, I[2].z ),
									current.i.position, 0.05f, colorCyan, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1 );
	}

	if ( rb_showVelocity.GetBool() ) {
		DrawVelocity( clipModel->GetId(), 0.1f, 4.0f );
	}
}

/*
================
idPhysics_RigidBody::idPhysics_RigidBody
================
*/
idPhysics_RigidBody::idPhysics_RigidBody( void ) {

	// set default rigid body properties
	SetClipMask( MASK_SOLID | CONTENTS_BODY | CONTENTS_SLIDEMOVER );
	SetBouncyness( 0.6f );
	SetFriction( 0.6f, 0.6f, 0.0f );
	SetWaterFriction( 1.f, 1.f );
	clipModel = NULL;
	centeredClipModel = new idClipModel();

	memset( &current, 0, sizeof( current ) );

	current.atRest = -1;
	current.lastTimeStep = gameLocal.msec;

	current.i.position.Zero();
	current.i.orientation.Identity();

	current.i.linearMomentum.Zero();
	current.i.angularMomentum.Zero();

	saved = current;

	mass = 1.0f;
	inverseMass = 1.0f;
	centerOfMass.Zero();
	inertiaTensor.Identity();
	inverseInertiaTensor.Identity();
	SetBuoyancy( 1.f );
	waterLevel = 0.0f;

	// use the least expensive euler integrator
	integrator = new idODE_Euler( sizeof(rigidBodyIState_t) / sizeof(float), RigidBodyDerivatives, this );

	dropToFloor = false;
	noImpact = false;
	noContact = false;
	noApplyImpulse = false;

	hasMaster = false;
	isOrientated = false;

#ifdef RB_TIMINGS
	lastTimerReset = 0;
#endif
}

/*
================
idPhysics_RigidBody::~idPhysics_RigidBody
================
*/
idPhysics_RigidBody::~idPhysics_RigidBody( void ) {
	gameLocal.clip.DeleteClipModel( clipModel );
	gameLocal.clip.DeleteClipModel( centeredClipModel );
	delete integrator;
}

/*
================
idPhysics_RigidBody::SetClipModel
================
*/
#define MAX_INERTIA_SCALE		10.0f

void idPhysics_RigidBody::SetClipModel( idClipModel *model, const float density, int id, bool freeOld ) {
	int minIndex;
	idMat3 inertiaScale;

	assert( self );
	assert( model );					// we need a clip model
	assert( model->IsTraceModel() );	// and it should be a trace model
	assert( density > 0.0f );			// density should be valid

	if ( clipModel != NULL && clipModel != model && freeOld ) {
		gameLocal.clip.DeleteClipModel( clipModel );
	}
	clipModel = model;
	clipModel->Link( gameLocal.clip, self, 0, current.i.position, current.i.orientation );

	// get mass properties from the trace model
	clipModel->GetMassProperties( density, mass, centerOfMass, inertiaTensor );

	// check whether or not the clip model has valid mass properties
	if ( mass < idMath::FLT_EPSILON || FLOAT_IS_NAN( mass ) ) {
		gameLocal.Warning( "idPhysics_RigidBody::SetClipModel: invalid mass for entity '%s' type '%s'",
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
		gameLocal.Warning( "idPhysics_RigidBody::SetClipModel: unbalanced inertia tensor for entity '%s' type '%s'",
							self->name.c_str(), self->GetType()->classname );

		// correct the inertia tensor by replacing it with that of a box of the same bounds as this
		idTraceModel trm( clipModel->GetBounds() );
		trm.GetMassProperties( density, mass, centerOfMass, inertiaTensor );
	}

	inverseMass = 1.0f / mass;
	inverseInertiaTensor = inertiaTensor.Inverse() * ( 1.0f / 6.0f );

	current.i.linearMomentum.Zero();
	current.i.angularMomentum.Zero();

	// set up the centered clip model
	idTraceModel tempTrace = *clipModel->GetTraceModel();
	tempTrace.Translate( -centerOfMass );
	centeredClipModel->LoadTraceModel( tempTrace, false );
}

/*
================
idPhysics_RigidBody::GetClipModel
================
*/
idClipModel *idPhysics_RigidBody::GetClipModel( int id ) const {
	return clipModel;
}

/*
================
idPhysics_RigidBody::GetNumClipModels
================
*/
int idPhysics_RigidBody::GetNumClipModels( void ) const {
	return 1;
}

/*
================
idPhysics_RigidBody::SetMass
================
*/
void idPhysics_RigidBody::SetMass( float mass, int id ) {
	assert( mass > 0.0f );
	inertiaTensor *= mass / this->mass;
	inverseInertiaTensor = inertiaTensor.Inverse() * (1.0f / 6.0f);
	this->mass = mass;
	inverseMass = 1.0f / mass;
}

/*
================
idPhysics_RigidBody::GetMass
================
*/
float idPhysics_RigidBody::GetMass( int id ) const {
	return mass;
}

/*
================
idPhysics_RigidBody::SetFriction
================
*/
void idPhysics_RigidBody::SetFriction( const float linear, const float angular, const float contact ) {
	if (	linear < 0.0f || linear > 1.0f ||
			angular < 0.0f || angular > 1.0f ||
			contact < 0.0f || contact > 1.0f ) {
		gameLocal.Warning( "idPhysics_RigidBody::SetFriction: friction out of range, linear = %.1f, angular = %.1f, contact = %.1f", linear, angular, contact );
		return;
	}
	linearFriction = linear;
	angularFriction = angular;
	contactFriction = contact;
}

/*
================
idPhysics_RigidBody::SetWaterFriction
================
*/
void idPhysics_RigidBody::SetWaterFriction( const float linear, const float angular ) {
	if (	linear < 0.0f || linear > 1.0f ||
			angular < 0.0f || angular > 1.0f ) {
		return;
	}
	linearFrictionWater = linear;
	angularFrictionWater = angular;
}

/*
================
idPhysics_RigidBody::SetBuoyancy
================
*/
void idPhysics_RigidBody::SetBuoyancy( float b ) {
	buoyancy = b;
}

/*
================
idPhysics_RigidBody::SetBouncyness
================
*/
void idPhysics_RigidBody::SetBouncyness( const float b ) {
	if ( b < 0.0f || b > 1.0f ) {
		return;
	}
	bouncyness = b;
}

/*
================
idPhysics_RigidBody::Rest
================
*/
void idPhysics_RigidBody::Rest( void ) {
	current.atRest = gameLocal.time;
	current.i.linearMomentum.Zero();
	current.i.angularMomentum.Zero();
	self->BecomeInactive( TH_PHYSICS );
}

/*
================
idPhysics_RigidBody::DropToFloor
================
*/
void idPhysics_RigidBody::DropToFloor( void ) {
	dropToFloor = true;
	testSolid = true;
}

/*
================
idPhysics_RigidBody::NoContact
================
*/
void idPhysics_RigidBody::NoContact( void ) {
	noContact = true;
}

/*
================
idPhysics_RigidBody::Activate
================
*/
void idPhysics_RigidBody::Activate( void ) {
	current.atRest = -1;
	self->BecomeActive( TH_PHYSICS );
}

/*
================
idPhysics_RigidBody::PutToRest

  put to rest untill something collides with this physics object
================
*/
void idPhysics_RigidBody::PutToRest( void ) {
	Rest();
}

/*
================
idPhysics_RigidBody::EnableImpact
================
*/
void idPhysics_RigidBody::EnableImpact( void ) {
	noImpact = false;
}

/*
================
idPhysics_RigidBody::DisableImpact
================
*/
void idPhysics_RigidBody::DisableImpact( void ) {
	noImpact = true;
}

/*
================
idPhysics_RigidBody::SetContents
================
*/
void idPhysics_RigidBody::SetContents( int contents, int id ) {
	if ( clipModel ) {
		clipModel->SetContents( contents );
	}
}

/*
================
idPhysics_RigidBody::GetContents
================
*/
int idPhysics_RigidBody::GetContents( int id ) const {
	return clipModel->GetContents();
}

/*
================
idPhysics_RigidBody::GetBounds
================
*/
const idBounds &idPhysics_RigidBody::GetBounds( int id ) const {
	return clipModel->GetBounds();
}

/*
================
idPhysics_RigidBody::GetAbsBounds
================
*/
const idBounds &idPhysics_RigidBody::GetAbsBounds( int id ) const {
	return clipModel->GetAbsBounds();
}

/*
================
idPhysics_RigidBody::Evaluate

  Evaluate the impulse based rigid body physics.
  When a collision occurs an impulse is applied at the moment of impact but
  the remaining time after the collision is ignored.
================
*/
bool idPhysics_RigidBody::Evaluate( int timeStepMSec, int endTimeMSec ) {
	rigidBodyPState_t next;
	idAngles angles;
	trace_t collision;
	idVec3 impulse;
	idEntity *ent;
	idVec3 oldOrigin, masterOrigin;
	idMat3 oldAxis, masterAxis;
	float timeStep;
	bool collided, cameToRest = false;

	current.i.angularMomentum.FixDenormals();
	current.i.linearMomentum.FixDenormals();
	current.i.orientation.FixDenormals();
	current.i.position.FixDenormals();

	timeStep = MS2SEC( timeStepMSec );
	current.lastTimeStep = timeStep;
	const float minTimeStep = MS2SEC( 1 );

	if ( hasMaster ) {
		if ( timeStepMSec <= 0 ) {
			return true;
		}
		oldOrigin = current.i.position;
		oldAxis = current.i.orientation;
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.i.position = masterOrigin + localOrigin * masterAxis;
		if ( isOrientated ) {
			current.i.orientation = localAxis * masterAxis;
		} else {
			current.i.orientation = localAxis;
		}
		clipModel->Link( gameLocal.clip, self, clipModel->GetId(), current.i.position, current.i.orientation );
		current.i.linearMomentum = mass * ( ( current.i.position - oldOrigin ) / timeStep );
		current.i.angularMomentum = inertiaTensor * ( ( current.i.orientation * oldAxis.Transpose() ).ToAngularVelocity() / timeStep );
		current.externalForce.Zero();
		current.externalTorque.Zero();

		return ( current.i.position != oldOrigin || current.i.orientation != oldAxis );
	}

	// if the body is at rest
	if ( current.atRest >= 0 || timeStep <= 0.0f ) {
		DebugDraw();
		return false;
	}

#ifdef RB_TIMINGS
	timer_total.Start();
#endif

	// move the rigid body velocity into the frame of a pusher
	current.i.linearMomentum -= current.pushVelocity.SubVec3( 0 ) * mass;
	current.i.angularMomentum -= current.pushVelocity.SubVec3( 1 ) * inertiaTensor;

	clipModel->Unlink( gameLocal.clip );

	int count = 0;
	int	maxRepetitions = 3;

	// HACK: Don't let cliententities run many repetitions
	if ( self->IsType( rvClientPhysics::Type ) ) {
		maxRepetitions = 1;
	}

	do {

		next = current;

		CheckWater();

		// calculate next position and orientation
		Integrate( timeStep, next );

	#ifdef RB_TIMINGS
		timer_collision.Start();
	#endif

		// check for collisions from the current to the next state
		collided = CheckForCollisions( timeStep, next, collision );
		float timeStepUsed = collision.fraction * timeStep;


	#ifdef RB_TIMINGS
		timer_collision.Stop();
	#endif

		// set the new state
		current = next;

		if ( collided ) {
			// apply collision impulse
			if ( CollisionImpulse( collision, impulse ) ) {
				current.atRest = gameLocal.time;
			}
		}

		// update the position of the clip model
		clipModel->Link( gameLocal.clip, self, clipModel->GetId(), current.i.position, current.i.orientation );

		DebugDraw();

		if ( !noContact ) {

	#ifdef RB_TIMINGS
			timer_collision.Start();
	#endif
			// get contacts
			EvaluateContacts( CLIP_DEBUG_PARMS_ENTINFO_ONLY( self ) );

	#ifdef RB_TIMINGS
			timer_collision.Stop();
	#endif

			// check if the body has come to rest
			if ( TestIfAtRest() ) {
				// put to rest
				Rest();
				cameToRest = true;
			}  else {
				// apply contact friction
				ContactFriction( timeStep );
			}
		}

		if ( current.atRest < 0 ) {
			ActivateContactEntities();
		}

		if ( collided && !noApplyImpulse ) {
			// if the rigid body didn't come to rest or the other entity is not at rest
			ent = gameLocal.entities[collision.c.entityNum];
			if ( ent && ( !cameToRest || !ent->IsAtRest() ) && ent->IsCollisionPushable() ) {
				// apply impact to other entity
				ent->ApplyImpulse( self, collision.c.id, collision.c.point, -impulse );
			}
		}

		timeStep -= timeStepUsed;
		count++;

		current.externalForce.Zero();
		current.externalTorque.Zero();
	} while( count < maxRepetitions && timeStep >= minTimeStep );



	// move the rigid body velocity back into the world frame
	current.i.linearMomentum += current.pushVelocity.SubVec3( 0 ) * mass;
	current.i.angularMomentum += current.pushVelocity.SubVec3( 1 ) * inertiaTensor;
	current.pushVelocity.Zero();

	current.lastTimeStep = timeStep;

	if ( IsOutsideWorld() ) {
//		gameLocal.Warning( "rigid body moved outside world bounds for entity '%s' type '%s' at (%s)",
//					self->name.c_str(), self->GetType()->classname, current.i.position.ToString(0) );
		Rest();
	}

#ifdef RB_TIMINGS
	timer_total.Stop();

	if ( rb_showTimings->integer == 1 ) {
		gameLocal.Printf( "%12s: t %1.4f cd %1.4f\n",
						self->name.c_str(),
						timer_total.Milliseconds(), timer_collision.Milliseconds() );
		lastTimerReset = 0;
	}
	else if ( rb_showTimings->integer == 2 ) {
		numRigidBodies++;
		if ( endTimeMSec > lastTimerReset ) {
			gameLocal.Printf( "rb %d: t %1.4f cd %1.4f\n",
							numRigidBodies,
							timer_total.Milliseconds(), timer_collision.Milliseconds() );
		}
	}
	if ( endTimeMSec > lastTimerReset ) {
		lastTimerReset = endTimeMSec;
		numRigidBodies = 0;
		timer_total.Clear();
		timer_collision.Clear();
	}
#endif

	return true;
}

/*
================
idPhysics_RigidBody::UpdateTime
================
*/
void idPhysics_RigidBody::UpdateTime( int endTimeMSec ) {
}

/*
================
idPhysics_RigidBody::GetTime
================
*/
int idPhysics_RigidBody::GetTime( void ) const {
	return gameLocal.time;
}

/*
================
idPhysics_RigidBody::GetImpactInfo
================
*/
void idPhysics_RigidBody::GetImpactInfo( const int id, const idVec3 &point, impactInfo_t *info ) const {
	idVec3 linearVelocity, angularVelocity;
	idMat3 inverseWorldInertiaTensor;

	linearVelocity = inverseMass * current.i.linearMomentum;
	inverseWorldInertiaTensor = current.i.orientation.Transpose() * inverseInertiaTensor * current.i.orientation;
	angularVelocity = inverseWorldInertiaTensor * current.i.angularMomentum;

	info->invMass = inverseMass;
	info->invInertiaTensor = inverseWorldInertiaTensor;
	info->position = point - ( current.i.position + centerOfMass * current.i.orientation );
	info->velocity = linearVelocity + angularVelocity.Cross( info->position );
}

/*
================
idPhysics_RigidBody::ApplyImpulse
================
*/
void idPhysics_RigidBody::ApplyImpulse( const int id, const idVec3 &point, const idVec3 &impulse ) {
	if ( noImpact ) {
		return;
	}
	if ( hasMaster ) {
		self->GetMaster()->GetPhysics()->ApplyImpulse( 0, point, impulse );
		return;
	}
	current.i.linearMomentum += impulse;
	current.i.angularMomentum += ( point - ( current.i.position + centerOfMass * current.i.orientation ) ).Cross( impulse );
	Activate();
}

/*
================
idPhysics_RigidBody::AddForce
================
*/
void idPhysics_RigidBody::AddForce( const int id, const idVec3 &point, const idVec3 &force ) {
	if ( noImpact ) {
		return;
	}
	if ( hasMaster ) {
		self->GetMaster()->GetPhysics()->AddForce( 0, point, force );
		return;
	}
	current.externalForce += force;
	current.externalTorque += ( point - ( current.i.position + centerOfMass * current.i.orientation ) ).Cross( force );
	Activate();
}

/*
================
idPhysics_RigidBody::IsAtRest
================
*/
bool idPhysics_RigidBody::IsAtRest( void ) const {
	return current.atRest >= 0;
}

/*
================
idPhysics_RigidBody::GetRestStartTime
================
*/
int idPhysics_RigidBody::GetRestStartTime( void ) const {
	return current.atRest;
}

/*
================
idPhysics_RigidBody::IsPushable
================
*/
bool idPhysics_RigidBody::IsPushable( void ) const {
	return ( !noImpact && !hasMaster );
}

/*
================
idPhysics_RigidBody::SaveState
================
*/
void idPhysics_RigidBody::SaveState( void ) {
	saved = current;
}

/*
================
idPhysics_RigidBody::RestoreState
================
*/
void idPhysics_RigidBody::RestoreState( void ) {
	current = saved;

	clipModel->Link( gameLocal.clip, self, clipModel->GetId(), current.i.position, current.i.orientation );

	EvaluateContacts( CLIP_DEBUG_PARMS_ENTINFO_ONLY( self ) );
}

/*
================
idPhysics::SetOrigin
================
*/
void idPhysics_RigidBody::SetOrigin( const idVec3 &newOrigin, int id ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	if ( hasMaster ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.i.position = masterOrigin + newOrigin * masterAxis;
		localOrigin = newOrigin;
	} else {
		current.i.position = newOrigin;
	}

	clipModel->Link( gameLocal.clip, self, clipModel->GetId(), current.i.position, clipModel->GetAxis() );

	Activate();
}

/*
================
idPhysics::SetAxis
================
*/
void idPhysics_RigidBody::SetAxis( const idMat3 &newAxis, int id ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	if ( hasMaster ) {
		if ( isOrientated ) {
			self->GetMasterPosition( masterOrigin, masterAxis );
			current.i.orientation = newAxis * masterAxis;
		}
		localAxis = newAxis;
	} else {
		current.i.orientation = newAxis;
	}

	clipModel->Link( gameLocal.clip, self, clipModel->GetId(), clipModel->GetOrigin(), current.i.orientation );

	Activate();
}

/*
================
idPhysics::Move
================
*/
void idPhysics_RigidBody::Translate( const idVec3 &translation, int id ) {

	if ( hasMaster ) {
		localOrigin += translation;
	}
	current.i.position += translation;

	clipModel->Link( gameLocal.clip, self, clipModel->GetId(), current.i.position, clipModel->GetAxis() );

	Activate();
}

/*
================
idPhysics::Rotate
================
*/
void idPhysics_RigidBody::Rotate( const idRotation &rotation, int id ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	current.i.orientation *= rotation.ToMat3();
	current.i.position *= rotation;

	if ( hasMaster ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		localAxis *= rotation.ToMat3();
		localOrigin = ( current.i.position - masterOrigin ) * masterAxis.Transpose();
	}

	clipModel->Link( gameLocal.clip, self, clipModel->GetId(), current.i.position, current.i.orientation );

	Activate();
}

/*
================
idPhysics_RigidBody::GetOrigin
================
*/
const idVec3 &idPhysics_RigidBody::GetOrigin( int id ) const {
	return current.i.position;
}

/*
================
idPhysics_RigidBody::GetAxis
================
*/
const idMat3 &idPhysics_RigidBody::GetAxis( int id ) const {
	return current.i.orientation;
}

/*
================
idPhysics_RigidBody::SetLinearVelocity
================
*/
void idPhysics_RigidBody::SetLinearVelocity( const idVec3 &newLinearVelocity, int id ) {
	current.i.linearMomentum = newLinearVelocity * mass;
	Activate();
}

/*
================
idPhysics_RigidBody::SetAngularVelocity
================
*/
void idPhysics_RigidBody::SetAngularVelocity( const idVec3 &newAngularVelocity, int id ) {
	idMat3 worldInertiaTensor = 6.0f * ( current.i.orientation.Transpose() * inertiaTensor * current.i.orientation );
	current.i.angularMomentum = worldInertiaTensor * newAngularVelocity;
	Activate();
}

/*
================
idPhysics_RigidBody::GetLinearVelocity
================
*/
const idVec3 &idPhysics_RigidBody::GetLinearVelocity( int id ) const {
	static idVec3 curLinearVelocity;
	curLinearVelocity = current.i.linearMomentum * inverseMass;
	return curLinearVelocity;
}

/*
================
idPhysics_RigidBody::GetAngularVelocity
================
*/
const idVec3 &idPhysics_RigidBody::GetAngularVelocity( int id ) const {
	static idVec3 curAngularVelocity;
	idMat3 inverseWorldInertiaTensor;

	inverseWorldInertiaTensor = current.i.orientation.Transpose() * inverseInertiaTensor * current.i.orientation;
	curAngularVelocity = inverseWorldInertiaTensor * current.i.angularMomentum;
	return curAngularVelocity;
}

/*
================
idPhysics_RigidBody::ClipTranslation
================
*/
void idPhysics_RigidBody::ClipTranslation( trace_t &results, const idVec3 &translation, const idClipModel *model ) const {
	if ( model ) {
		gameLocal.clip.TranslationModel( CLIP_DEBUG_PARMS_ENTINFO( self ) results, clipModel->GetOrigin(), clipModel->GetOrigin() + translation,
											clipModel, clipModel->GetAxis(), clipMask,
											model, model->GetOrigin(), model->GetAxis() );
	}
	else {
		gameLocal.clip.Translation( CLIP_DEBUG_PARMS_ENTINFO( self ) results, clipModel->GetOrigin(), clipModel->GetOrigin() + translation,
											clipModel, clipModel->GetAxis(), clipMask, self );
	}
}

/*
================
idPhysics_RigidBody::ClipRotation
================
*/
void idPhysics_RigidBody::ClipRotation( trace_t &results, const idRotation &rotation, const idClipModel *model ) const {
	if ( model ) {
		gameLocal.clip.RotationModel( CLIP_DEBUG_PARMS_ENTINFO( self ) results, clipModel->GetOrigin(), rotation,
											clipModel, clipModel->GetAxis(), clipMask,
											model, model->GetOrigin(), model->GetAxis() );
	}
	else {
		gameLocal.clip.Rotation( CLIP_DEBUG_PARMS_ENTINFO( self ) results, clipModel->GetOrigin(), rotation,
											clipModel, clipModel->GetAxis(), clipMask, self );
	}
}

/*
================
idPhysics_RigidBody::ClipContents
================
*/
int idPhysics_RigidBody::ClipContents( const idClipModel *model ) const {
	if ( model ) {
		return gameLocal.clip.ContentsModel( CLIP_DEBUG_PARMS_ENTINFO( self ) clipModel->GetOrigin(), clipModel, clipModel->GetAxis(), -1,
									model, model->GetOrigin(), model->GetAxis() );
	}
	else {
		return gameLocal.clip.Contents( CLIP_DEBUG_PARMS_ENTINFO( self ) clipModel->GetOrigin(), clipModel, clipModel->GetAxis(), -1, NULL );
	}
}

/*
================
idPhysics_RigidBody::UnlinkClip
================
*/
void idPhysics_RigidBody::UnlinkClip( void ) {
	clipModel->Unlink( gameLocal.clip );
}

/*
================
idPhysics_RigidBody::LinkClip
================
*/
void idPhysics_RigidBody::LinkClip( void ) {
	clipModel->Link( gameLocal.clip, self, clipModel->GetId(), current.i.position, current.i.orientation );
}

/*
================
idPhysics_RigidBody::DisableClip
================
*/
void idPhysics_RigidBody::DisableClip( bool activateContacting ) {
	if ( activateContacting ) {
		WakeEntitiesContacting( self, clipModel );
	}
	clipModel->Disable();
}

/*
================
idPhysics_RigidBody::EnableClip
================
*/
void idPhysics_RigidBody::EnableClip( void ) {
	clipModel->Enable();
}

/*
================
idPhysics_RigidBody::EvaluateContacts
================
*/
bool idPhysics_RigidBody::EvaluateContacts( CLIP_DEBUG_PARMS_DECLARATION_ONLY ) {
	idVec3 dir;
	int num;

	ClearContacts();

	contacts.SetNum( 10, false );

	dir = current.i.linearMomentum + current.lastTimeStep * gravityVector * mass;
	dir.Normalize();
	num = gameLocal.clip.Contacts( CLIP_DEBUG_PARMS_ENTINFO( self ) &contacts[0], 10, clipModel->GetOrigin(),
					&dir, CONTACT_EPSILON, clipModel, clipModel->GetAxis(), clipMask, self );
	contacts.SetNum( num, false );

	AddContactEntitiesForContacts();

	return ( contacts.Num() != 0 );
}

/*
================
idPhysics_RigidBody::SetPushed
================
*/
void idPhysics_RigidBody::SetPushed( int deltaTime ) {
	idRotation rotation;

	rotation = ( saved.i.orientation * current.i.orientation ).ToRotation();

	// velocity with which the af is pushed
	current.pushVelocity.SubVec3(0) += ( current.i.position - saved.i.position ) / ( deltaTime * idMath::M_MS2SEC );
	current.pushVelocity.SubVec3(1) += rotation.GetVec() * -DEG2RAD( rotation.GetAngle() ) / ( deltaTime * idMath::M_MS2SEC );
}

/*
================
idPhysics_RigidBody::GetPushedLinearVelocity
================
*/
const idVec3 &idPhysics_RigidBody::GetPushedLinearVelocity( const int id ) const {
	return current.pushVelocity.SubVec3(0);
}

/*
================
idPhysics_RigidBody::GetPushedAngularVelocity
================
*/
const idVec3 &idPhysics_RigidBody::GetPushedAngularVelocity( const int id ) const {
	return current.pushVelocity.SubVec3(1);
}

/*
================
idPhysics_RigidBody::SetMaster
================
*/
void idPhysics_RigidBody::SetMaster( idEntity *master, const bool orientated ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	if ( master ) {
		if ( !hasMaster ) {
			// transform from world space to master space
			self->GetMasterPosition( masterOrigin, masterAxis );
			localOrigin = ( current.i.position - masterOrigin ) * masterAxis.Transpose();
			if ( orientated ) {
				localAxis = current.i.orientation * masterAxis.Transpose();
			} else {
				localAxis = current.i.orientation;
			}
			hasMaster = true;
			isOrientated = orientated;
			ClearContacts();
		}
	}
	else {
		localOrigin = vec3_origin;
		localAxis = mat3_identity;

		if ( hasMaster ) {
			hasMaster = false;
			Activate();
		}
	}
}

/*
================
idPhysics_RigidBody::CheckWater
================
*/
void idPhysics_RigidBody::CheckWater( void ) {
	waterLevel = 0.0f;

	const idBounds& absBounds = GetAbsBounds( -1 );

	const idClipModel* clipModel;
	int count = gameLocal.clip.ClipModelsTouchingBounds( CLIP_DEBUG_PARMS_ENTINFO( self ) absBounds, CONTENTS_WATER, &clipModel, 1, NULL );
	if ( !count ) {
		return;
	}

	if ( !clipModel->GetNumCollisionModels() ) {
		return;
	}

	idCollisionModel* model = clipModel->GetCollisionModel( 0 );
	int numPlanes = model->GetNumBrushPlanes();
	if ( !numPlanes ) {
		return;
	}
	const idBounds& modelBounds = model->GetBounds();

	self->CheckWater( clipModel->GetOrigin(), clipModel->GetAxis(), model );

	idMat3 transpose = current.i.orientation.Transpose();

	idVec3 waterCurrent;
	clipModel->GetEntity()->GetWaterCurrent( waterCurrent );

	float scratch[ MAX_TRACEMODEL_WATER_POINTS ];
	float totalVolume = 0.0f;
	float waterVolume = 0.0f;

	{
		idClipModel* bodyClip = this->clipModel;
		const traceModelWater_t* waterPoints = bodyClip->GetWaterPoints();
		if ( waterPoints == NULL ) {
			return;
		}
		float volume = bodyClip->GetTraceModelVolume();
		totalVolume += volume;

		for ( int i = 0; i < MAX_TRACEMODEL_WATER_POINTS; i++ ) {
			scratch[ i ] = waterPoints[ i ].weight * volume;
		}

		for ( int l = 0; l < numPlanes; l++ ) {
			idPlane plane = model->GetBrushPlane( l );
			plane.TranslateSelf( clipModel->GetOrigin() - bodyClip->GetOrigin() );
			plane.Normal() *= clipModel->GetAxis();
			plane.Normal() *= transpose;

			for ( int i = 0; i < MAX_TRACEMODEL_WATER_POINTS; i++ ) {
				if ( plane.Distance( waterPoints[ i ].xyz ) > 0 ) {
					scratch[ i ] = 0.f;
				}
			}
		}

		float height = clipModel->GetOrigin().z - bodyClip->GetOrigin().z + modelBounds.GetMaxs().z;
		idPlane plane( transpose[ 2 ], height );

		for ( int i = 0; i < MAX_TRACEMODEL_WATER_POINTS; i++ ) {
			if ( !scratch[ i ] ) {
				continue;
			}

			scratch[ i ] *= Min( -plane.Distance( waterPoints[ i ].xyz ) / 16.f, 1.f );

			idVec3 impulse = scratch[ i ] * ( ( -gravityNormal * buoyancy ) + ( waterCurrent * inverseMass ) );

			idVec3 org = bodyClip->GetOrigin() + ( bodyClip->GetAxis() * waterPoints[ i ].xyz );

			current.i.linearMomentum += impulse;
			current.i.angularMomentum += ( org - ( current.i.position + centerOfMass * current.i.orientation ) ).Cross( impulse );

			waterVolume += scratch[ i ];
		}
	}

	if ( totalVolume > idMath::FLT_EPSILON ) {
		waterLevel = waterVolume / totalVolume;
	}

	if ( waterLevel ) {
		Activate();
	}
}

const float	RB_ORIGIN_MAX				= 32767;
const int	RB_ORIGIN_TOTAL_BITS		= 24;
const int	RB_ORIGIN_EXPONENT_BITS		= idMath::BitsForInteger( idMath::BitsForFloat( RB_ORIGIN_MAX ) ) + 1;
const int	RB_ORIGIN_MANTISSA_BITS		= RB_ORIGIN_TOTAL_BITS - 1 - RB_ORIGIN_EXPONENT_BITS;

const float	RB_LINEAR_VELOCITY_MIN				= 0.05f;
const float	RB_LINEAR_VELOCITY_MAX				= 8192.0f;
const int	RB_LINEAR_VELOCITY_TOTAL_BITS		= 20;
const int	RB_LINEAR_VELOCITY_EXPONENT_BITS	= idMath::BitsForInteger( idMath::BitsForFloat( RB_LINEAR_VELOCITY_MAX, RB_LINEAR_VELOCITY_MIN ) ) + 1;
const int	RB_LINEAR_VELOCITY_MANTISSA_BITS	= RB_LINEAR_VELOCITY_TOTAL_BITS - 1 - RB_LINEAR_VELOCITY_EXPONENT_BITS;

const float	RB_ANGULAR_VELOCITY_MIN				= 0.00001f;
const float	RB_ANGULAR_VELOCITY_MAX				= idMath::PI * 8.0f;
const int	RB_ANGULAR_VELOCITY_TOTAL_BITS		= 20;
const int	RB_ANGULAR_VELOCITY_EXPONENT_BITS	= idMath::BitsForInteger( idMath::BitsForFloat( RB_ANGULAR_VELOCITY_MAX, RB_ANGULAR_VELOCITY_MIN ) ) + 1;
const int	RB_ANGULAR_VELOCITY_MANTISSA_BITS	= RB_ANGULAR_VELOCITY_TOTAL_BITS - 1 - RB_ANGULAR_VELOCITY_EXPONENT_BITS;

/*
================
idPhysics_RigidBody::CheckNetworkStateChanges
================
*/
bool idPhysics_RigidBody::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_BASE( sdRigidBodyNetworkState );

		if ( baseData.angularVelocity != GetAngularVelocity() ) {
			return true;
		}

		if ( baseData.linearVelocity != GetLinearVelocity() ) {
			return true;
		}

		if ( baseData.orientation != current.i.orientation.ToCQuat() ) {
			return true;
		}

		if ( baseData.position != current.i.position ) {
			return true;
		}
		return false;
	}

	if ( mode == NSM_BROADCAST ) { 
		NET_GET_BASE( sdRigidBodyBroadcastState );

		if ( baseData.localPosition != localOrigin ) {
			return true;
		}

		if ( baseData.localOrientation != localAxis.ToCQuat() ) {
			return true;
		}

		if ( baseData.atRest != current.atRest ) {
			return true;
		}

		return false;
	}

	return false;
}

/*
================
idPhysics_RigidBody::WriteNetworkState
================
*/
void idPhysics_RigidBody::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdRigidBodyNetworkState );

		// update state
		newData.position		= current.i.position;
		newData.orientation		= current.i.orientation.ToCQuat();
		newData.linearVelocity	= GetLinearVelocity();
		newData.angularVelocity	= GetAngularVelocity();

		// write state
		msg.WriteDeltaVector( baseData.position, newData.position, RB_ORIGIN_EXPONENT_BITS, RB_ORIGIN_MANTISSA_BITS );
		msg.WriteDeltaCQuat( baseData.orientation, newData.orientation );
		msg.WriteDeltaVector( baseData.linearVelocity, newData.linearVelocity, RB_LINEAR_VELOCITY_EXPONENT_BITS, RB_LINEAR_VELOCITY_MANTISSA_BITS );
		msg.WriteDeltaVector( baseData.angularVelocity, newData.angularVelocity, RB_ANGULAR_VELOCITY_EXPONENT_BITS, RB_ANGULAR_VELOCITY_MANTISSA_BITS );

		return;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdRigidBodyBroadcastState );

		// update state
		newData.localPosition		= localOrigin;
		newData.localOrientation	= localAxis.ToCQuat();
		newData.atRest				= current.atRest;

		// write state
		msg.WriteDeltaVector( baseData.localPosition, newData.localPosition );
		msg.WriteDeltaCQuat( baseData.localOrientation, newData.localOrientation );
		msg.WriteDeltaLong( baseData.atRest, newData.atRest );

		return;
	}
}

/*
================
idPhysics_RigidBody::ApplyNetworkState
================
*/
void idPhysics_RigidBody::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	traceCollection.ForceNextUpdate();
	if ( mode == NSM_VISIBLE ) {
		NET_GET_NEW( sdRigidBodyNetworkState );

		// update state
		current.i.position			= newData.position;
		current.i.orientation		= newData.orientation.ToMat3();
		SetLinearVelocity( newData.linearVelocity );
		SetAngularVelocity( newData.angularVelocity );

		if ( clipModel ) {
			clipModel->Link( gameLocal.clip, self, 0, current.i.position, current.i.orientation );
		}
		self->UpdateVisuals();
		CheckWater();

		return;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_NEW( sdRigidBodyBroadcastState );

		// update state
		localOrigin					= newData.localPosition;
		localAxis					= newData.localOrientation.ToMat3();
		current.atRest				= newData.atRest;

		self->UpdateVisuals();
		CheckWater();

		return;
	}
}

/*
================
idPhysics_RigidBody::ReadNetworkState
================
*/
void idPhysics_RigidBody::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdRigidBodyNetworkState );

		// read state
		newData.position			= msg.ReadDeltaVector( baseData.position, RB_ORIGIN_EXPONENT_BITS, RB_ORIGIN_MANTISSA_BITS );
		newData.orientation			= msg.ReadDeltaCQuat( baseData.orientation );
		newData.linearVelocity		= msg.ReadDeltaVector( baseData.linearVelocity, RB_LINEAR_VELOCITY_EXPONENT_BITS, RB_LINEAR_VELOCITY_MANTISSA_BITS );
		newData.angularVelocity		= msg.ReadDeltaVector( baseData.angularVelocity, RB_ANGULAR_VELOCITY_EXPONENT_BITS, RB_ANGULAR_VELOCITY_MANTISSA_BITS );

		newData.position.FixDenormals();
		newData.orientation.FixDenormals();
		newData.angularVelocity.FixDenormals();
		newData.linearVelocity.FixDenormals();

		self->OnNewOriginRead( newData.position );
		self->OnNewAxesRead( newData.orientation.ToMat3() );
		return;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdRigidBodyBroadcastState );

		// read state
		newData.localPosition		= msg.ReadDeltaVector( baseData.localPosition );
		newData.localOrientation	= msg.ReadDeltaCQuat( baseData.localOrientation );
		newData.atRest				= msg.ReadDeltaLong( baseData.atRest );

		return;
	}
}

/*
================
idPhysics_RigidBody::CreateNetworkStructure
================
*/
sdEntityStateNetworkData* idPhysics_RigidBody::CreateNetworkStructure( networkStateMode_t mode ) const {
	if ( mode == NSM_VISIBLE ) {
		return new sdRigidBodyNetworkState();
	}
	if ( mode == NSM_BROADCAST ) {
		return new sdRigidBodyBroadcastState();
	}
	return NULL;
}
