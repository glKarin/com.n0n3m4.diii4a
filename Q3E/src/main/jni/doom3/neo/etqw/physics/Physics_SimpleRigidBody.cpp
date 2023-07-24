// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Physics_SimpleRigidBody.h"
#include "../Entity.h"
#include "../Player.h"
#include "../ContentMask.h"
#include "../script/Script_Helper.h"
#include "../script/Script_ScriptObject.h"

CLASS_DECLARATION( idPhysics_Base, sdPhysics_SimpleRigidBody )
END_CLASS

const float STOP_SPEED		= 10.0f;


/*
================
sdSimpleRigidBodyNetworkState::MakeDefault
================
*/
void sdSimpleRigidBodyNetworkState::MakeDefault( void ) {
	position		= vec3_origin;
	orientation.x	= 0.f;
	orientation.y	= 0.f;
	orientation.z	= 0.f;
	linearVelocity	= vec3_zero;
	angularVelocity = vec3_zero;
}

/*
================
sdSimpleRigidBodyNetworkState::Write
================
*/
void sdSimpleRigidBodyNetworkState::Write( idFile* file ) const {
	file->WriteVec3( position );
	file->WriteCQuat( orientation );
	file->WriteVec3( linearVelocity );
	file->WriteVec3( angularVelocity );
}

/*
================
sdSimpleRigidBodyNetworkState::Read
================
*/
void sdSimpleRigidBodyNetworkState::Read( idFile* file ) {
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
sdSimpleRigidBodyBroadcastState::MakeDefault
================
*/
void sdSimpleRigidBodyBroadcastState::MakeDefault( void ) {
	localPosition		= vec3_origin;
	localOrientation.x	= 0.f;
	localOrientation.y	= 0.f;
	localOrientation.z	= 0.f;
	atRest				= -1;
	orientedClip		= false;
}

/*
================
sdSimpleRigidBodyBroadcastState::Write
================
*/
void sdSimpleRigidBodyBroadcastState::Write( idFile* file ) const {
	file->WriteVec3( localPosition );
	file->WriteCQuat( localOrientation );
	file->WriteInt( atRest );
	file->WriteBool( orientedClip );
}

/*
================
sdSimpleRigidBodyBroadcastState::Read
================
*/
void sdSimpleRigidBodyBroadcastState::Read( idFile* file ) {
	file->ReadVec3( localPosition );
	file->ReadCQuat( localOrientation );
	file->ReadInt( atRest );
	file->ReadBool( orientedClip );
}

/*
================
sdPhysics_SimpleRigidBody::Integrate

  Very coarse & naive
================
*/
void sdPhysics_SimpleRigidBody::Integrate( float deltaTime, simpleRigidBodyPState_t &next ) {

	next = current;

	// apply gravity
	next.linearVelocity += deltaTime * gravityVector;

	// remove any contribution towards the last normal
	float velocityTowardsLast = -lastCollideNormal * next.linearVelocity;
	if ( velocityTowardsLast > 0.0f ) {
		next.linearVelocity += lastCollideNormal * velocityTowardsLast;
	}

	next.position = next.position + deltaTime * next.linearVelocity;

	// rotation
	idVec3 rotAxis = current.angularVelocity;
	float angle = RAD2DEG( rotAxis.Normalize() );
	if ( angle > 0.00001f ) {
		idRotation rotation( vec3_origin, rotAxis, -angle * deltaTime );
		next.orientation = current.orientation * rotation.ToMat3();
		next.orientation.OrthoNormalizeSelf();
	}
}

/*
================
sdPhysics_SimpleRigidBody::CollisionResponse
================
*/
bool sdPhysics_SimpleRigidBody::CollisionResponse( const trace_t &collision, idVec3 &impulse ) {

	idVec3&	velocity		= current.linearVelocity;
	idVec3	hitVelocity		= velocity;

	float	normalVel		= velocity * collision.c.normal;
	idVec3	tangent			= velocity - normalVel * collision.c.normal;
	float	tangentVel		= tangent.Normalize();
	if ( tangentVel < idMath::FLT_EPSILON ) {
		tangent.Set( 0.0f, 0.0f, 1.0f );
		tangentVel = 0.0f;
	}

	// HACK - scale the bouncyness so that things can't stop on steep walls
	float bounceScale = 1.0f;
	if ( idMath::Fabs( collision.c.normal.z ) < 0.7071f ) {
		float bounceNeeded = Min( 1.0f, -90.0f / normalVel );
		if ( normalBouncyness > idMath::FLT_EPSILON ) {
			bounceScale = Max( 1.0f, bounceNeeded / normalBouncyness );
		}
	}

	// bounce the velocity, always bounce off of noplant surfaces.
	if ( collision.c.material != NULL && ( collision.c.material->GetSurfaceFlags() & SURF_NOPLANT ) ) {
		normalVel = -normalVel;
	} else {
		normalVel *= -normalBouncyness * bounceScale;
		tangentVel *= tangentialBouncyness * bounceScale;
	}
	velocity = normalVel*collision.c.normal + tangentVel*tangent;

	// check for stopping
	if ( collision.c.normal.z > 0.2f && velocity.LengthSqr() < Square( stopSpeed ) ) {
		velocity.Zero();
		current.angularVelocity.Zero();
	} else {
		if ( collision.c.normal.z > 0.2f ) {
			// small velocity -> zero
			velocity.FixDenormals( 0.1f );
		} else {
			velocity.FixDenormals( 0.000001f );
		}

		// push out from collision surface
		current.position += collision.c.normal * 0.01f;

		// reflect the angular velocity about the normal too
		current.angularVelocity -= ( ( 1.0f - angularBouncyness ) * current.angularVelocity*collision.c.normal )*collision.c.normal;
		current.angularVelocity.FixDenormals( 0.000001f );
	}

	idEntity* ent = gameLocal.entities[ collision.c.entityNum ];

	ent->Hit( collision, hitVelocity, self );
	return self->Collide( collision, hitVelocity, -1 );
}

/*
================
sdPhysics_SimpleRigidBody::CheckForCollisions

  Check for collisions between the current and next state.
  If there is a collision the next state is set to the state at the moment of impact.
================
*/
bool sdPhysics_SimpleRigidBody::CheckForCollisions( const float deltaTime, simpleRigidBodyPState_t &next, trace_t &collision ) {
	collision.fraction = 1.0f;

	// calculate the position of the center of mass at current and next
	idVec3 CoMstart = current.position + centerOfMass;
	idVec3 CoMend = next.position + centerOfMass;

	// if there was a collision
	if ( gameLocal.clip.Translation( CLIP_DEBUG_PARMS_ENTINFO( self ) collision, CoMstart, CoMend, centeredClipModel, mat3_identity, clipMask, self ) ) {
		// set the next state to the state at the moment of impact
		next.position = collision.endpos - centerOfMass;
		next.orientation = current.orientation;
		next.linearVelocity = current.linearVelocity;
		next.angularVelocity = current.angularVelocity;
		return true;
	}

	return false;
}

/*
================
sdPhysics_SimpleRigidBody::TestIfAtRest

  Returns true if the body is considered at rest.
  Does not catch all cases where the body is at rest but is generally good enough.
================
*/
bool sdPhysics_SimpleRigidBody::TestIfAtRest( void ) const {
	if ( current.atRest >= 0 ) {
		return true;
	}

	// when its finished bouncing the collision impulse code manually sets the 
	// velocities to zero
	if ( current.linearVelocity != vec3_origin || current.angularVelocity != vec3_origin ) {
		return false;
	}

	// needs to be touching the ground
	if ( contacts.Num() < 1 ) {
		return false;
	}

	return true;
}

/*
================
sdPhysics_SimpleRigidBody::DropToFloorAndRest

  Drops the object straight down to the floor and verifies if the object is at rest on the floor.
================
*/
void sdPhysics_SimpleRigidBody::DropToFloorAndRest( void ) {
	idVec3 down;
	trace_t tr;

	if ( testSolid ) {

		testSolid = false;

		if ( gameLocal.clip.Contents( CLIP_DEBUG_PARMS_ENTINFO( self ) current.position, clipModel, mat3_identity, clipMask, self ) ) {
			gameLocal.DWarning( "rigid body in solid for entity '%s' type '%s' at (%s)",
								self->name.c_str(), self->GetType()->classname, current.position.ToString(0) );
			Rest();
			dropToFloor = false;
			return;
		}
	}

	// put the body on the floor
	down = current.position + gravityNormal * 128.0f;
	gameLocal.clip.Translation( CLIP_DEBUG_PARMS_ENTINFO( self ) tr, current.position, down, clipModel, mat3_identity, clipMask, self );
	current.position = tr.endpos;
	if ( !orientedClip ) {
		clipModel->Link( gameLocal.clip, self, clipModel->GetId(), tr.endpos, mat3_identity );
	} else {
		clipModel->Link( gameLocal.clip, self, clipModel->GetId(), tr.endpos, current.orientation );
	}

	// if on the floor already
	if ( tr.fraction == 0.0f ) {
		// test if we are really at rest
		EvaluateContacts( CLIP_DEBUG_PARMS_ENTINFO_ONLY( self ) );
		if ( !TestIfAtRest() ) {
			gameLocal.DWarning( "rigid body not at rest for entity '%s' type '%s' at (%s)",
								self->name.c_str(), self->GetType()->classname, current.position.ToString(0) );
		}
		Rest();
		dropToFloor = false;
	} else if ( IsOutsideWorld() ) {
//		gameLocal.Warning( "rigid body outside world bounds for entity '%s' type '%s' at (%s)",
//							self->name.c_str(), self->GetType()->classname, current.position.ToString(0) );
		Rest();
		dropToFloor = false;
	}
}

/*
================
sdPhysics_SimpleRigidBody::DebugDraw
================
*/
void sdPhysics_SimpleRigidBody::DebugDraw( void ) {

	if ( rb_showBodies.GetBool() || ( rb_showActive.GetBool() && current.atRest < 0 ) ) {
		clipModel->Draw();
	}

	if ( rb_showMass.GetBool() ) {
		gameRenderWorld->DrawText( va( "\n%1.2f", mass ), current.position, 0.08f, colorCyan, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1 );
	}

	if ( rb_showVelocity.GetBool() ) {
		DrawVelocity( clipModel->GetId(), 0.1f, 4.0f );
	}
}

/*
================
sdPhysics_SimpleRigidBody::sdPhysics_SimpleRigidBody
================
*/
sdPhysics_SimpleRigidBody::sdPhysics_SimpleRigidBody( void ) {

	// set default rigid body properties
	SetClipMask( MASK_SOLID | CONTENTS_BODY | CONTENTS_SLIDEMOVER );
	SetBouncyness( 0.6f );
	SetStopSpeed( 40.0f );
	SetFriction( 0.6f, 0.6f, 0.0f );
	SetWaterFriction( 1.f, 1.f );
	clipModel = NULL;
	centeredClipModel = new idClipModel();

	memset( &current, 0, sizeof( current ) );

	current.atRest = -1;
	current.lastTimeStep = MS2SEC( gameLocal.msec );

	current.position.Zero();
	current.orientation.Identity();

	current.linearVelocity.Zero();
	current.angularVelocity.Zero();
	lastCollideNormal.Zero();

	saved = current;

	mass = 1.0f;
	inverseMass = 1.0f;
	centerOfMass.Zero();
	inertiaTensor.Identity();
	inverseInertiaTensor.Identity();
	SetBuoyancy( 1.f );
	waterLevel = 0.0f;

	dropToFloor = false;
	noImpact = false;
	noContact = false;

	hasMaster = false;
	isOrientated = false;

	orientedClip = false;

	restFunc = NULL;
}

/*
================
sdPhysics_SimpleRigidBody::~sdPhysics_SimpleRigidBody
================
*/
sdPhysics_SimpleRigidBody::~sdPhysics_SimpleRigidBody( void ) {
	gameLocal.clip.DeleteClipModel( clipModel );
	gameLocal.clip.DeleteClipModel( centeredClipModel );
}

/*
================
sdPhysics_SimpleRigidBody::SetClipModel
================
*/
#define MAX_INERTIA_SCALE		10.0f

void sdPhysics_SimpleRigidBody::SetClipModel( idClipModel *model, const float density, int id, bool freeOld ) {
	idMat3 inertiaScale;

	assert( self );
	assert( model );					// we need a clip model
	assert( model->IsTraceModel() );	// and it should be a trace model
	assert( density > 0.0f );			// density should be valid

	if ( clipModel != NULL && clipModel != model && freeOld ) {
		gameLocal.clip.DeleteClipModel( clipModel );
	}
	clipModel = model;
	if ( !orientedClip ) {
		clipModel->Link( gameLocal.clip, self, 0, current.position, mat3_identity );
	} else {
		clipModel->Link( gameLocal.clip, self, 0, current.position, current.orientation );
	}

	// get mass properties from the trace model
	clipModel->GetMassProperties( density, mass, centerOfMass, inertiaTensor );

	// check whether or not the clip model has valid mass properties
	if ( mass < idMath::FLT_EPSILON || FLOAT_IS_NAN( mass ) ) {
		gameLocal.Warning( "sdPhysics_SimpleRigidBody::SetClipModel: invalid mass for entity '%s' type '%s'",
							self->name.c_str(), self->GetType()->classname );
		mass = 1.0f;
		centerOfMass.Zero();
	}

	// check whether or not the inertia tensor is balanced
	int minIndex = Min3Index( inertiaTensor[0][0], inertiaTensor[1][1], inertiaTensor[2][2] );
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

	current.linearVelocity.Zero();
	current.angularVelocity.Zero();

	// set up the centered clip model
	idTraceModel tempTrace = *clipModel->GetTraceModel();
	tempTrace.Translate( -centerOfMass );
	centeredClipModel->LoadTraceModel( tempTrace, false );
}

/*
================
sdPhysics_SimpleRigidBody::GetClipModel
================
*/
idClipModel *sdPhysics_SimpleRigidBody::GetClipModel( int id ) const {
	return clipModel;
}

/*
================
sdPhysics_SimpleRigidBody::GetNumClipModels
================
*/
int sdPhysics_SimpleRigidBody::GetNumClipModels( void ) const {
	return 1;
}

/*
================
sdPhysics_SimpleRigidBody::SetMass
================
*/
void sdPhysics_SimpleRigidBody::SetMass( float mass, int id ) {
	assert( mass > 0.0f );
	this->mass = mass;
	inverseMass = 1.0f / mass;
}

/*
================
sdPhysics_SimpleRigidBody::GetMass
================
*/
float sdPhysics_SimpleRigidBody::GetMass( int id ) const {
	return mass;
}

/*
================
sdPhysics_SimpleRigidBody::SetWaterFriction
================
*/
void sdPhysics_SimpleRigidBody::SetWaterFriction( const float linear, const float angular ) {
	if (	linear < 0.0f || linear > 1.0f ||
			angular < 0.0f || angular > 1.0f ) {
		return;
	}
	linearFrictionWater = linear;
	angularFrictionWater = angular;
}

/*
================
sdPhysics_SimpleRigidBody::SetBuoyancy
================
*/
void sdPhysics_SimpleRigidBody::SetBuoyancy( float b ) {
	buoyancy = b;
}

/*
================
sdPhysics_SimpleRigidBody::SetBouncyness
================
*/
void sdPhysics_SimpleRigidBody::SetBouncyness( const float b ) {
	if ( b < 0.0f || b > 1.0f ) {
		return;
	}
	normalBouncyness = b;
	tangentialBouncyness = b;
	angularBouncyness = -1.0f;
}

/*
================
sdPhysics_SimpleRigidBody::SetBouncyness
================
*/
void sdPhysics_SimpleRigidBody::SetBouncyness( float normal, float tangential, float angular ) {
	normalBouncyness = normal;
	tangentialBouncyness = tangential;
	angularBouncyness = angular;
}

/*
================
sdPhysics_SimpleRigidBody::SetStopSpeed
================
*/
void sdPhysics_SimpleRigidBody::SetStopSpeed( float _stopSpeed ) {
	stopSpeed = _stopSpeed;
}

/*
================
sdPhysics_SimpleRigidBody::Rest
================
*/
void sdPhysics_SimpleRigidBody::Rest( void ) {
	int restTime = current.atRest;

	current.atRest = gameLocal.time;
	current.linearVelocity.Zero();
	current.angularVelocity.Zero();
	self->BecomeInactive( TH_PHYSICS );

	if ( current.atRest != restTime ) {
		sdScriptHelper h1;
		self->GetScriptObject()->CallNonBlockingScriptEvent( self->GetScriptObject()->GetFunction( "OnRest" ), h1 );
	}
}

/*
================
sdPhysics_SimpleRigidBody::DropToFloor
================
*/
void sdPhysics_SimpleRigidBody::DropToFloor( void ) {
	dropToFloor = true;
	testSolid = true;
}

/*
================
sdPhysics_SimpleRigidBody::NoContact
================
*/
void sdPhysics_SimpleRigidBody::NoContact( void ) {
	noContact = true;
}

/*
================
sdPhysics_SimpleRigidBody::Activate
================
*/
void sdPhysics_SimpleRigidBody::Activate( void ) {
	current.atRest = -1;
	self->BecomeActive( TH_PHYSICS );
}

/*
================
sdPhysics_SimpleRigidBody::PutToRest

  put to rest untill something collides with this physics object
================
*/
void sdPhysics_SimpleRigidBody::PutToRest( void ) {
	Rest();
}

/*
================
sdPhysics_SimpleRigidBody::EnableImpact
================
*/
void sdPhysics_SimpleRigidBody::EnableImpact( void ) {
	noImpact = false;
}

/*
================
sdPhysics_SimpleRigidBody::DisableImpact
================
*/
void sdPhysics_SimpleRigidBody::DisableImpact( void ) {
	noImpact = true;
}

/*
================
sdPhysics_SimpleRigidBody::SetContents
================
*/
void sdPhysics_SimpleRigidBody::SetContents( int contents, int id ) {
	if ( clipModel ) {
		clipModel->SetContents( contents );
	}
}

/*
================
sdPhysics_SimpleRigidBody::GetContents
================
*/
int sdPhysics_SimpleRigidBody::GetContents( int id ) const {
	return clipModel->GetContents();
}

/*
================
sdPhysics_SimpleRigidBody::GetBounds
================
*/
const idBounds &sdPhysics_SimpleRigidBody::GetBounds( int id ) const {
	return clipModel->GetBounds();
}

/*
================
sdPhysics_SimpleRigidBody::GetAbsBounds
================
*/
const idBounds &sdPhysics_SimpleRigidBody::GetAbsBounds( int id ) const {
	return clipModel->GetAbsBounds();
}

/*
================
sdPhysics_SimpleRigidBody::Evaluate

  Evaluate the impulse based rigid body physics.
  When a collision occurs an impulse is applied at the moment of impact but
  the remaining time after the collision is ignored.
================
*/
bool sdPhysics_SimpleRigidBody::Evaluate( int timeStepMSec, int endTimeMSec ) {
	simpleRigidBodyPState_t next;
	idAngles angles;
	trace_t collision;
	idVec3 impulse;
	idVec3 oldOrigin, masterOrigin;
	idMat3 oldAxis, masterAxis;
	float timeStep, minTimeStep;
	bool collided, cameToRest = false;

	current.angularVelocity.FixDenormals();
	current.linearVelocity.FixDenormals();
	current.orientation.FixDenormals();
	current.position.FixDenormals();

	timeStep = MS2SEC( timeStepMSec );
	minTimeStep = MS2SEC( 1 );
	current.lastTimeStep = timeStep;

	if ( hasMaster ) {
		if ( timeStepMSec <= 0 ) {
			return true;
		}
		oldOrigin = current.position;
		oldAxis = current.orientation;
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.position = masterOrigin + localOrigin * masterAxis;
		if ( isOrientated ) {
			current.orientation = localAxis * masterAxis;
		} else {
			current.orientation = localAxis;
		}
		LinkClip();
		current.linearVelocity = ( current.position - oldOrigin ) / timeStep;
		current.angularVelocity = ( current.orientation * oldAxis.Transpose() ).ToAngularVelocity() / timeStep;

		return ( current.position != oldOrigin || current.orientation != oldAxis );
	}

	// if the body is at rest
	if ( current.atRest >= 0 || timeStep <= 0.0f ) {
		DebugDraw();
		return false;
	}

	// move the rigid body velocity into the frame of a pusher
	current.linearVelocity -= current.pushVelocity.SubVec3( 0 );
	current.angularVelocity -= current.pushVelocity.SubVec3( 1 );

	clipModel->Unlink( gameLocal.clip );

	int count = 0;
	int	maxRepetitions = 2;

	do {

		CheckWater();

		// calculate next position and orientation
		Integrate( timeStep, next );

		// check for collisions from the current to the next state
		collided = CheckForCollisions( timeStep, next, collision );
		float timeStepUsed = collision.fraction * timeStep;

		// set the new state
		current = next;

		if ( collided ) {
			// apply collision impulse
			if ( CollisionResponse( collision, impulse ) ) {
				current.atRest = gameLocal.time;
			}
			lastCollideNormal = collision.c.normal;
		} else {
			lastCollideNormal.Zero();
		}

		// update the position of the clip model
		LinkClip();

		DebugDraw();

		if ( !noContact ) {
			// get contacts
			EvaluateContacts( CLIP_DEBUG_PARMS_ENTINFO_ONLY( self ) );

			// check if the body has come to rest
			if ( TestIfAtRest() ) {
				// put to rest
				Rest();
				cameToRest = true;
			}
		}

		if ( current.atRest < 0 ) {
			ActivateContactEntities();
		}

		timeStep -= timeStepUsed;
		count++;
	} while( count < maxRepetitions && timeStep >= minTimeStep );

	// move the rigid body velocity back into the world frame
	current.linearVelocity += current.pushVelocity.SubVec3( 0 );
	current.angularVelocity += current.pushVelocity.SubVec3( 1 );
	current.pushVelocity.Zero();

	if ( IsOutsideWorld() ) {
//		gameLocal.Warning( "rigid body moved outside world bounds for entity '%s' type '%s' at (%s)",
//					self->name.c_str(), self->GetType()->classname, current.position.ToString(0) );
		Rest();
	}

	return true;
}

/*
================
sdPhysics_SimpleRigidBody::UpdateTime
================
*/
void sdPhysics_SimpleRigidBody::UpdateTime( int endTimeMSec ) {
}

/*
================
sdPhysics_SimpleRigidBody::GetTime
================
*/
int sdPhysics_SimpleRigidBody::GetTime( void ) const {
	return gameLocal.time;
}

/*
================
sdPhysics_SimpleRigidBody::GetImpactInfo
================
*/
void sdPhysics_SimpleRigidBody::GetImpactInfo( const int id, const idVec3 &point, impactInfo_t *info ) const {
	idVec3 linearVelocity, angularVelocity;
	idMat3 inverseWorldInertiaTensor;

	linearVelocity = current.linearVelocity;
	angularVelocity = current.angularVelocity;

	info->invMass = inverseMass;
	info->invInertiaTensor.Zero();
	info->position = point - ( current.position + centerOfMass * current.orientation );
	info->velocity = linearVelocity + angularVelocity.Cross( info->position );
}

/*
================
sdPhysics_SimpleRigidBody::ApplyImpulse
================
*/
void sdPhysics_SimpleRigidBody::ApplyImpulse( const int id, const idVec3 &point, const idVec3 &impulse ) {
	if ( noImpact ) {
		return;
	}
	if ( hasMaster ) {
		self->GetMaster()->GetPhysics()->ApplyImpulse( 0, point, impulse );
		return;
	}

	current.linearVelocity += impulse * inverseMass;
	const idVec3 momentumDelta = ( point - ( current.position + centerOfMass * current.orientation ) ).Cross( impulse );
	const idMat3 inverseWorldInertiaTensor = current.orientation.Transpose() * inverseInertiaTensor * current.orientation;
	current.angularVelocity += RAD2DEG( inverseWorldInertiaTensor * momentumDelta );

	Activate();
}

/*
================
sdPhysics_SimpleRigidBody::AddForce
================
*/
void sdPhysics_SimpleRigidBody::AddForce( const int id, const idVec3 &point, const idVec3 &force ) {
	if ( noImpact ) {
		return;
	}
	if ( hasMaster ) {
		self->GetMaster()->GetPhysics()->AddForce( 0, point, force );
		return;
	}

	idVec3 impulse = force * current.lastTimeStep;
	ApplyImpulse( id, point, impulse );
}

/*
================
sdPhysics_SimpleRigidBody::IsAtRest
================
*/
bool sdPhysics_SimpleRigidBody::IsAtRest( void ) const {
	return current.atRest >= 0;
}

/*
================
sdPhysics_SimpleRigidBody::GetRestStartTime
================
*/
int sdPhysics_SimpleRigidBody::GetRestStartTime( void ) const {
	return current.atRest;
}

/*
================
sdPhysics_SimpleRigidBody::IsPushable
================
*/
bool sdPhysics_SimpleRigidBody::IsPushable( void ) const {
	return ( !noImpact && !hasMaster );
}

/*
================
sdPhysics_SimpleRigidBody::SaveState
================
*/
void sdPhysics_SimpleRigidBody::SaveState( void ) {
	saved = current;
}

/*
================
sdPhysics_SimpleRigidBody::RestoreState
================
*/
void sdPhysics_SimpleRigidBody::RestoreState( void ) {
	current = saved;

	LinkClip();

	EvaluateContacts( CLIP_DEBUG_PARMS_ENTINFO_ONLY( self ) );
}

/*
================
idPhysics::SetOrigin
================
*/
void sdPhysics_SimpleRigidBody::SetOrigin( const idVec3 &newOrigin, int id ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	if ( hasMaster ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.position = masterOrigin + newOrigin * masterAxis;
		localOrigin = newOrigin;
	} else {
		current.position = newOrigin;
	}

	LinkClip();

	Activate();
}

/*
================
idPhysics::SetAxis
================
*/
void sdPhysics_SimpleRigidBody::SetAxis( const idMat3 &newAxis, int id ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	if ( hasMaster ) {
		if ( isOrientated ) {
			self->GetMasterPosition( masterOrigin, masterAxis );
			current.orientation = newAxis * masterAxis;
		}
		localAxis = newAxis;
	} else {
		current.orientation = newAxis;
	}

	LinkClip();

	Activate();
}

/*
================
idPhysics::Move
================
*/
void sdPhysics_SimpleRigidBody::Translate( const idVec3 &translation, int id ) {

	if ( hasMaster ) {
		localOrigin += translation;
	}
	current.position += translation;

	LinkClip();

	Activate();
}

/*
================
idPhysics::Rotate
================
*/
void sdPhysics_SimpleRigidBody::Rotate( const idRotation &rotation, int id ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	current.orientation *= rotation.ToMat3();
	current.position *= rotation;

	if ( hasMaster ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		localAxis *= rotation.ToMat3();
		localOrigin = ( current.position - masterOrigin ) * masterAxis.Transpose();
	}

	LinkClip();

	Activate();
}

/*
================
sdPhysics_SimpleRigidBody::GetOrigin
================
*/
const idVec3 &sdPhysics_SimpleRigidBody::GetOrigin( int id ) const {
	return current.position;
}

/*
================
sdPhysics_SimpleRigidBody::GetAxis
================
*/
const idMat3 &sdPhysics_SimpleRigidBody::GetAxis( int id ) const {
	return current.orientation;
}

/*
================
sdPhysics_SimpleRigidBody::SetLinearVelocity
================
*/
void sdPhysics_SimpleRigidBody::SetLinearVelocity( const idVec3 &newLinearVelocity, int id ) {
	current.linearVelocity = newLinearVelocity;
	Activate();
}

/*
================
sdPhysics_SimpleRigidBody::SetAngularVelocity
================
*/
void sdPhysics_SimpleRigidBody::SetAngularVelocity( const idVec3 &newAngularVelocity, int id ) {
	current.angularVelocity = newAngularVelocity;
	Activate();
}

/*
================
sdPhysics_SimpleRigidBody::GetLinearVelocity
================
*/
const idVec3 &sdPhysics_SimpleRigidBody::GetLinearVelocity( int id ) const {
	return current.linearVelocity;
}

/*
================
sdPhysics_SimpleRigidBody::GetAngularVelocity
================
*/
const idVec3 &sdPhysics_SimpleRigidBody::GetAngularVelocity( int id ) const {
	return current.angularVelocity;
}

/*
================
sdPhysics_SimpleRigidBody::ClipTranslation
================
*/
void sdPhysics_SimpleRigidBody::ClipTranslation( trace_t &results, const idVec3 &translation, const idClipModel *model ) const {
	if ( model ) {
		gameLocal.clip.TranslationModel( CLIP_DEBUG_PARMS_ENTINFO( self ) results, clipModel->GetOrigin(), clipModel->GetOrigin() + translation,
											clipModel, mat3_identity, clipMask,
											model, model->GetOrigin(), model->GetAxis() );
	}
	else {
		gameLocal.clip.Translation( CLIP_DEBUG_PARMS_ENTINFO( self ) results, clipModel->GetOrigin(), clipModel->GetOrigin() + translation,
											clipModel, mat3_identity, clipMask, self );
	}
}

/*
================
sdPhysics_SimpleRigidBody::ClipRotation
================
*/
void sdPhysics_SimpleRigidBody::ClipRotation( trace_t &results, const idRotation &rotation, const idClipModel *model ) const {
	// physical presence not affected by rotation
}

/*
================
sdPhysics_SimpleRigidBody::ClipContents
================
*/
int sdPhysics_SimpleRigidBody::ClipContents( const idClipModel *model ) const {
	if ( model ) {
		return gameLocal.clip.ContentsModel( CLIP_DEBUG_PARMS_ENTINFO( self ) clipModel->GetOrigin(), clipModel, mat3_identity, -1,
									model, model->GetOrigin(), model->GetAxis() );
	}
	else {
		return gameLocal.clip.Contents( CLIP_DEBUG_PARMS_ENTINFO( self ) clipModel->GetOrigin(), clipModel, mat3_identity, -1, NULL );
	}
}

/*
================
sdPhysics_SimpleRigidBody::UnlinkClip
================
*/
void sdPhysics_SimpleRigidBody::UnlinkClip( void ) {
	clipModel->Unlink( gameLocal.clip );
}

/*
================
sdPhysics_SimpleRigidBody::LinkClip
================
*/
void sdPhysics_SimpleRigidBody::LinkClip( void ) {
	if ( !orientedClip ) {
		clipModel->Link( gameLocal.clip, self, clipModel->GetId(), current.position, mat3_identity );
	} else {
		clipModel->Link( gameLocal.clip, self, clipModel->GetId(), current.position, current.orientation );
	}
}

/*
================
sdPhysics_SimpleRigidBody::DisableClip
================
*/
void sdPhysics_SimpleRigidBody::DisableClip( bool activateContacting ) {
	if ( activateContacting ) {
		WakeEntitiesContacting( self, clipModel );
	}
	clipModel->Disable();
}

/*
================
sdPhysics_SimpleRigidBody::EnableClip
================
*/
void sdPhysics_SimpleRigidBody::EnableClip( void ) {
	clipModel->Enable();
}

/*
================
sdPhysics_SimpleRigidBody::EvaluateContacts
================
*/
bool sdPhysics_SimpleRigidBody::EvaluateContacts( CLIP_DEBUG_PARMS_DECLARATION_ONLY ) {
	idVec3 dir;
	int num;

	ClearContacts();

	contacts.SetNum( 10, false );

	dir = current.linearVelocity + current.lastTimeStep * gravityVector;
	dir.Normalize();
	num = gameLocal.clip.Contacts( CLIP_DEBUG_PARMS_ENTINFO( self ) &contacts[0], 10, clipModel->GetOrigin(),
					&dir, CONTACT_EPSILON, clipModel, mat3_identity, clipMask, self );
	contacts.SetNum( num, false );

	AddContactEntitiesForContacts();

	return ( contacts.Num() != 0 );
}

/*
================
sdPhysics_SimpleRigidBody::SetPushed
================
*/
void sdPhysics_SimpleRigidBody::SetPushed( int deltaTime ) {
	idRotation rotation;

	rotation = ( saved.orientation * current.orientation ).ToRotation();

	// velocity with which the af is pushed
	current.pushVelocity.SubVec3(0) += ( current.position - saved.position ) / ( deltaTime * idMath::M_MS2SEC );
	current.pushVelocity.SubVec3(1) += rotation.GetVec() * -DEG2RAD( rotation.GetAngle() ) / ( deltaTime * idMath::M_MS2SEC );
}

/*
================
sdPhysics_SimpleRigidBody::GetPushedLinearVelocity
================
*/
const idVec3 &sdPhysics_SimpleRigidBody::GetPushedLinearVelocity( const int id ) const {
	return current.pushVelocity.SubVec3(0);
}

/*
================
sdPhysics_SimpleRigidBody::GetPushedAngularVelocity
================
*/
const idVec3 &sdPhysics_SimpleRigidBody::GetPushedAngularVelocity( const int id ) const {
	return current.pushVelocity.SubVec3(1);
}

/*
================
sdPhysics_SimpleRigidBody::SetMaster
================
*/
void sdPhysics_SimpleRigidBody::SetMaster( idEntity *master, const bool orientated ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	if ( master ) {
		if ( !hasMaster ) {
			// transform from world space to master space
			self->GetMasterPosition( masterOrigin, masterAxis );
			localOrigin = ( current.position - masterOrigin ) * masterAxis.Transpose();
			if ( orientated ) {
				localAxis = current.orientation * masterAxis.Transpose();
			} else {
				localAxis = current.orientation;
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
sdPhysics_SimpleRigidBody::CheckWater
================
*/
void sdPhysics_SimpleRigidBody::CheckWater( void ) {
	waterLevel = WATERLEVEL_NONE;

	const idBounds& absBounds = GetAbsBounds( -1 );

	const idClipModel* waterModel;
	idCollisionModel* model;
	int count = gameLocal.clip.ClipModelsTouchingBounds( CLIP_DEBUG_PARMS_ENTINFO( self ) absBounds, CONTENTS_WATER, &waterModel, 1, NULL );
	if ( !count ) {
		self->CheckWaterEffectsOnly();
		return;
	}

	if ( !waterModel->GetNumCollisionModels() ) {
		self->CheckWaterEffectsOnly();
		return;
	}

	model = waterModel->GetCollisionModel( 0 );
	int numPlanes = model->GetNumBrushPlanes();
	if ( !numPlanes ) {
		self->CheckWaterEffectsOnly();
		return;
	}
	const idBounds& modelBounds = model->GetBounds();

	idBounds worldbb;
	worldbb.FromTransformedBounds( GetBounds(), GetOrigin(), GetAxis() );
	bool submerged = worldbb.GetMaxs()[2] < (modelBounds.GetMaxs()[2] + waterModel->GetOrigin().z);

	if ( submerged ) {
		waterLevel = WATERLEVEL_HEAD;
	}

	self->CheckWater( waterModel->GetOrigin(), waterModel->GetAxis(), model );
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
sdPhysics_SimpleRigidBody::CheckNetworkStateChanges
================
*/
bool sdPhysics_SimpleRigidBody::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_BASE( sdSimpleRigidBodyNetworkState );

		if ( baseData.angularVelocity != GetAngularVelocity() ) {
			return true;
		}

		if ( baseData.linearVelocity != GetLinearVelocity() ) {
			return true;
		}

		if ( baseData.orientation != current.orientation.ToCQuat() ) {
			return true;
		}

		if ( baseData.position != current.position ) {
			return true;
		}
		return false;
	}

	if ( mode == NSM_BROADCAST ) { 
		NET_GET_BASE( sdSimpleRigidBodyBroadcastState );

		if ( baseData.localPosition != localOrigin ) {
			return true;
		}

		if ( baseData.localOrientation != localAxis.ToCQuat() ) {
			return true;
		}

		if ( baseData.atRest != current.atRest ) {
			return true;
		}

		if ( baseData.orientedClip != orientedClip ) {
			return true;
		}

		return false;
	}

	return false;
}

/*
================
sdPhysics_SimpleRigidBody::WriteNetworkState
================
*/
void sdPhysics_SimpleRigidBody::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdSimpleRigidBodyNetworkState );

		// update state
		newData.position		= current.position;
		newData.orientation		= current.orientation.ToCQuat();
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
		NET_GET_STATES( sdSimpleRigidBodyBroadcastState );

		// update state
		newData.localPosition		= localOrigin;
		newData.localOrientation	= localAxis.ToCQuat();
		newData.atRest				= current.atRest;
		newData.orientedClip		= orientedClip;

		// write state
		msg.WriteDeltaVector( baseData.localPosition, newData.localPosition );
		msg.WriteDeltaCQuat( baseData.localOrientation, newData.localOrientation );
		msg.WriteDeltaLong( baseData.atRest, newData.atRest );
		msg.WriteBool( newData.orientedClip );

		return;
	}
}

/*
================
sdPhysics_SimpleRigidBody::ApplyNetworkState
================
*/
void sdPhysics_SimpleRigidBody::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	traceCollection.ForceNextUpdate();
	if ( mode == NSM_VISIBLE ) {
		NET_GET_NEW( sdSimpleRigidBodyNetworkState );

		// update state
		current.position			= newData.position;
		current.orientation		= newData.orientation.ToMat3();
		if ( !newData.linearVelocity.Compare( current.linearVelocity, idMath::FLT_EPSILON ) ) {
			SetLinearVelocity( newData.linearVelocity );
		}
		if ( !newData.angularVelocity.Compare( current.angularVelocity, idMath::FLT_EPSILON ) ) {
			SetAngularVelocity( newData.angularVelocity );
		}

		if ( clipModel ) {
			LinkClip();
		}
		self->UpdateVisuals();
		CheckWater();

		return;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_NEW( sdSimpleRigidBodyBroadcastState );

		// update state
		localOrigin					= newData.localPosition;
		localAxis					= newData.localOrientation.ToMat3();
		current.atRest				= newData.atRest;

		if ( orientedClip != newData.orientedClip ) {
			orientedClip = newData.orientedClip;
			LinkClip();
		}

		self->UpdateVisuals();
		CheckWater();

		return;
	}
}

/*
================
sdPhysics_SimpleRigidBody::ReadNetworkState
================
*/
void sdPhysics_SimpleRigidBody::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdSimpleRigidBodyNetworkState );

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
		NET_GET_STATES( sdSimpleRigidBodyBroadcastState );

		// read state
		newData.localPosition		= msg.ReadDeltaVector( baseData.localPosition );
		newData.localOrientation	= msg.ReadDeltaCQuat( baseData.localOrientation );
		newData.atRest				= msg.ReadDeltaLong( baseData.atRest );
		newData.orientedClip		= msg.ReadBool();

		return;
	}
}

/*
================
sdPhysics_SimpleRigidBody::CreateNetworkStructure
================
*/
sdEntityStateNetworkData* sdPhysics_SimpleRigidBody::CreateNetworkStructure( networkStateMode_t mode ) const {
	if ( mode == NSM_VISIBLE ) {
		return new sdSimpleRigidBodyNetworkState();
	}
	if ( mode == NSM_BROADCAST ) {
		return new sdSimpleRigidBodyBroadcastState();
	}
	return NULL;
}
