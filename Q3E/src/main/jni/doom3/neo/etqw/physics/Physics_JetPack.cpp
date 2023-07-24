// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Physics_JetPack.h"
#include "../Entity.h"
#include "../Actor.h"
#include "../vehicles/JetPack.h"

#define CHECKCURRENT for ( int index = 0; index < 3; index++ ) { if ( FLOAT_IS_NAN( current.origin[ index ] ) ) { assert( false ); } }

CLASS_DECLARATION( idPhysics_Actor, sdPhysics_JetPack )
END_CLASS

#define ENABLE_JP_FLOAT_CHECKS
#if defined( ENABLE_JP_FLOAT_CHECKS )
	#undef FLOAT_CHECK_BAD
	#undef VEC_CHECK_BAD

#define FLOAT_CHECK_BAD( x ) \
	if ( FLOAT_IS_NAN( x ) ) gameLocal.Error( "Bad floating point number in %s(%i): " #x " is NAN", __FILE__, __LINE__ ); \
	if ( FLOAT_IS_INF( x ) ) gameLocal.Error( "Bad floating point number in %s(%i): " #x " is INF", __FILE__, __LINE__ ); \
	if ( FLOAT_IS_IND( x ) ) gameLocal.Error( "Bad floating point number in %s(%i): " #x " is IND", __FILE__, __LINE__ ); \
	if ( FLOAT_IS_DENORMAL( x ) ) gameLocal.Error( "Bad floating point number in %s(%i): " #x " is DEN", __FILE__, __LINE__ ); \


	#define VEC_CHECK_BAD( vec ) FLOAT_CHECK_BAD( ( vec ).x ); FLOAT_CHECK_BAD( ( vec ).y ); FLOAT_CHECK_BAD( ( vec ).z );
	#define MAT_CHECK_BAD( m ) VEC_CHECK_BAD( m[ 0 ] ); VEC_CHECK_BAD( m[ 1 ] ); VEC_CHECK_BAD( m[ 2 ] );
	#define ANG_CHECK_BAD( ang ) FLOAT_CHECK_BAD( ( ang ).pitch ); FLOAT_CHECK_BAD( ( ang ).roll ); FLOAT_CHECK_BAD( ( ang ).yaw );
#else
	#define MAT_CHECK_BAD( m )
#endif


/*
=====================
sdJetPackPhysicsNetworkData::MakeDefault
=====================
*/
void sdJetPackPhysicsNetworkData::MakeDefault( void ) {
	origin			= vec3_origin;
	velocity		= vec3_zero;
	movementFlags = 0;
}

/*
=====================
sdJetPackPhysicsNetworkData::Write
=====================
*/
void sdJetPackPhysicsNetworkData::Write( idFile* file ) const {
	file->WriteVec3( origin );
	file->WriteVec3( velocity );
	file->WriteInt( movementFlags );
}

/*
=====================
sdJetPackPhysicsNetworkData::Read
=====================
*/
void sdJetPackPhysicsNetworkData::Read( idFile* file ) {
	file->ReadVec3( origin );
	file->ReadVec3( velocity );
	file->ReadInt( movementFlags );

	origin.FixDenormals();
	velocity.FixDenormals();
}

/*
=====================
sdJetPackPhysicsBroadcastData::MakeDefault
=====================
*/
void sdJetPackPhysicsBroadcastData::MakeDefault( void ) {
	pushVelocity	= vec3_zero;
}

/*
=====================
sdJetPackPhysicsBroadcastData::Write
=====================
*/
void sdJetPackPhysicsBroadcastData::Write( idFile* file ) const {
	file->WriteVec3( pushVelocity );
}

/*
=====================
sdJetPackPhysicsBroadcastData::Read
=====================
*/
void sdJetPackPhysicsBroadcastData::Read( idFile* file ) {
	file->ReadVec3( pushVelocity );
}

/*
=====================
sdPhysics_JetPack::EvaluateContacts
=====================
*/
bool sdPhysics_JetPack::EvaluateContacts( CLIP_DEBUG_PARMS_DECLARATION_ONLY ) {
	ClearContacts();

	idVec3 down;

	down = current.origin + gravityNormal; // * CONTACT_EPSILON;
	gameLocal.clip.Translation( CLIP_DEBUG_PARMS groundTrace, current.origin, down, clipModel, clipModel->GetAxis(), clipMask | CONTENTS_WATER, self );

	// add the ground trace as a contact
	if ( groundTrace.fraction < 1.0f ) {
		contacts.SetNum( 1, false );
		contacts[ 0 ] = groundTrace.c;
	} else {
		contacts.SetNum( 0, false );
	}

	AddContactEntitiesForContacts();

	return contacts.Num() != 0;
}

/*
=====================
sdPhysics_JetPack::CheckGround
=====================
*/
void sdPhysics_JetPack::CheckGround( void ) {
	jetPackPState_t& state = current;
	groundTraceValid = true;

	EvaluateContacts( CLIP_DEBUG_PARMS_ONLY );

	if ( groundTrace.fraction == 1.0f ) {
		state.onGround = false;
		state.onWater = false;
		groundEntityPtr = NULL;
		return;
	}

	groundEntityPtr = gameLocal.entities[ groundTrace.c.entityNum ];

	// HACK: Horrible hack needed because at present groundTrace.c.contents isn't always reliable :|
	if ( groundTrace.c.contents == -1 ) {
		idPhysics* physics = groundEntityPtr->GetPhysics();
		if ( physics != NULL ) {
			groundTrace.c.contents = physics->GetContents( groundTrace.c.id );
		} else {
			groundTrace.c.contents = 0;
		}
	}

	if ( groundTrace.c.contents & CONTENTS_WATER ) {
		state.onGround = false;
		state.onWater = true;
	} else {
		state.onGround = true;
		state.onWater = false;

		// let the entity know about the collision
		groundEntityPtr->Hit( groundTrace, state.velocity, self );	
		self->Collide( groundTrace, state.velocity, -1 );
	}
}

/*
================
sdPhysics_JetPack::sdPhysics_JetPack
================
*/
sdPhysics_JetPack::sdPhysics_JetPack( void ) {

	memset( &current, 0, sizeof( current ) );
	saved = current;
	
	maxStepHeight = 18.0f;
	noImpact = false;

	memset( &command, 0, sizeof( command ) );
	viewAngles.Zero();
	movementAllowed = true;
	boost = 0.0f;

	groundTraceValid = false;

	maxSpeed = 320.0f;
	maxBoostSpeed = 500.0f;
	walkForceScale = 0.4f;
	kineticFriction = 10.0f;
	jumpForce = 4800.0f;
	boostForce = 1200.0f;

	fanForce.Zero();
	addedVelocity.Zero();

	disableBoost = false;
	waterFraction = 0.0f;

	jetPackSelf = NULL;
	shotClipModel = NULL;
}

/*
================
sdPhysics_JetPack::~sdPhysics_JetPack
================
*/
sdPhysics_JetPack::~sdPhysics_JetPack( void ) {
	if ( shotClipModel != NULL ) {
		gameLocal.clip.DeleteClipModel( shotClipModel );
		shotClipModel = NULL;
	}
}

/*
================
sdPhysics_JetPack::SetClipModel
================
*/
void sdPhysics_JetPack::SetClipModel( idClipModel *model, const float density, int id, bool freeOld ) {
	assert( self );
	assert( model );					// a clip model is required
	assert( model->IsTraceModel() );	// and it should be a trace model
	assert( density > 0.0f );			// density should be valid

	if ( id == 0 ) {
		idPhysics_Actor::SetClipModel( model, density, 0, freeOld );
	} else {
		if ( shotClipModel != NULL && shotClipModel != model && freeOld ) {
			gameLocal.clip.DeleteClipModel( shotClipModel );
		}
		shotClipModel = model;
		LinkShotModel();
	}
}

/*
================
sdPhysics_JetPack::LinkShotModel
================
*/
void sdPhysics_JetPack::LinkShotModel( void ) {
	if ( shotClipModel != NULL ) {
		// calculate the axis
		idAngles angles = self->GetAxis().ToAngles();
		angles.pitch = 0.0f;
		angles.roll = 0.0f;

		shotClipModel->Link( gameLocal.clip, self, 1, clipModel->GetOrigin(), angles.ToMat3() );
	}
}

/*
================
sdPhysics_JetPack::SetMaxStepHeight
================
*/
void sdPhysics_JetPack::SetMaxStepHeight( const float newMaxStepHeight ) {
	maxStepHeight = newMaxStepHeight;
}

/*
================
sdPhysics_JetPack::GetMaxStepHeight
================
*/
float sdPhysics_JetPack::GetMaxStepHeight( void ) const {
	return maxStepHeight;
}

/*
================
sdPhysics_JetPack::OnGround
================
*/
bool sdPhysics_JetPack::OnGround( void ) const {
	return current.onGround;
}

/*
================
sdPhysics_JetPack::EnableImpact
================
*/
void sdPhysics_JetPack::EnableImpact( void ) {
	noImpact = false;
}

/*
================
sdPhysics_JetPack::DisableImpact
================
*/
void sdPhysics_JetPack::DisableImpact( void ) {
	noImpact = true;
}

/*
================
sdPhysics_JetPack::GroundMoveForce
================
*/
idVec3 sdPhysics_JetPack::GroundMoveForce( bool skiing, const idVec3& desiredMove, const idVec3& forceSoFar, float boost, float timeStep ) {
	VEC_CHECK_BAD( desiredMove );
	if ( desiredMove == vec3_origin ) {
		return vec3_origin;
	}

	FLOAT_CHECK_BAD( boost );
	if ( boost > 0.0f ) {
		return vec3_origin;
	}

	// find the desired direction vector for this slope
	idVec3 surfaceUp = groundTrace.c.normal;
	VEC_CHECK_BAD( surfaceUp );
	if ( surfaceUp == vec3_origin ) {
		surfaceUp = -gravityNormal;
	}

	idVec3 surfaceRight = surfaceUp.Cross( desiredMove );
	VEC_CHECK_BAD( surfaceRight );
	idVec3 surfaceDesired = surfaceRight.Cross( surfaceUp );
	VEC_CHECK_BAD( surfaceDesired );
	surfaceDesired.FixDenormals();
	VEC_CHECK_BAD( surfaceDesired );

	// calculate the current velocity on the movement plane
	idVec3 currentVelocity = current.velocity;
	VEC_CHECK_BAD( currentVelocity );
	currentVelocity -= ( currentVelocity * surfaceUp ) * surfaceUp;
	VEC_CHECK_BAD( currentVelocity );
	float currentSpeed = currentVelocity.Length();
	FLOAT_CHECK_BAD( currentSpeed );

	float maxAccel = walkForceScale * maxSpeed / timeStep;
	FLOAT_CHECK_BAD( maxAccel );
	// make moving have less effect at high speed
	if ( skiing && currentSpeed > maxSpeed ) {
		maxAccel *= 0.25f;
		FLOAT_CHECK_BAD( maxAccel );
	}

	// add in the speed
	idVec3 accel = surfaceDesired * maxAccel;
	VEC_CHECK_BAD( accel );

	// ramp down the acceleration up the slope
	if ( accel.z > 0.0f ) {
		float scaleUp = ( surfaceDesired.z - 0.3f ) / 0.3f;
		FLOAT_CHECK_BAD( scaleUp );
		scaleUp = idMath::ClampFloat( 0.0f, 1.0f, scaleUp );
		accel.z *= scaleUp;
		FLOAT_CHECK_BAD( accel.z );
	}

	idVec3 endVel = current.velocity + accel * timeStep;
	VEC_CHECK_BAD( endVel );

	// factor in the forces applied so far (NOTE: don't have to cancel out the vertical 
	// component since this only includes the friction force!)
	VEC_CHECK_BAD( forceSoFar );
	idVec3 accelFromForceSoFar = ( forceSoFar - ( ( forceSoFar * surfaceUp ) * surfaceUp ) ) * timeStep;
	VEC_CHECK_BAD( accelFromForceSoFar );
	endVel += accelFromForceSoFar;
	VEC_CHECK_BAD( endVel );
	float clampSpeed = maxSpeed;
	FLOAT_CHECK_BAD( clampSpeed );

	if ( skiing && currentSpeed > clampSpeed ) {
		// when skiing, the speed you're going at can be the maximum	
		clampSpeed = currentSpeed;
	}

	// clamp the resultant speed
	idVec3 endVelDir = endVel;
	VEC_CHECK_BAD( endVelDir );
	float endSpeed = endVelDir.Normalize();
	VEC_CHECK_BAD( endVelDir );
	FLOAT_CHECK_BAD( endSpeed );
	if ( endSpeed > clampSpeed ) {
		endSpeed = clampSpeed;
		FLOAT_CHECK_BAD( endSpeed );
		endVel = endVelDir * endSpeed;
		VEC_CHECK_BAD( endVel );
		accel = ( endVel - current.velocity - accelFromForceSoFar ) / timeStep;
		VEC_CHECK_BAD( accel );
	}

	VEC_CHECK_BAD( accel );
	return accel;
}

/*
================
sdPhysics_JetPack::FrictionForce
================
*/
idVec3 sdPhysics_JetPack::FrictionForce( bool skiing, const idVec3& desiredMove, const idVec3& forceSoFar, float boost, float timeStep ) {
	float frictionScale = 1.0f;
	FLOAT_CHECK_BAD( frictionScale );
	if ( skiing ) {
		// skiing simply drastically scales back the friction received
		frictionScale = 0.01f;
	}

	if ( groundTrace.c.normal.z < 0.6f && !current.onWater ) {
		frictionScale *= ( groundTrace.c.normal.z - 0.3f ) / 0.3f;
		frictionScale = idMath::ClampFloat( 0.0f, 1.0f, frictionScale );
	}

	// ignore slope movement, remove all velocity in not on the plane
	VEC_CHECK_BAD( forceSoFar );
	VEC_CHECK_BAD( current.velocity );
	idVec3 vel = current.velocity + forceSoFar * timeStep;
	VEC_CHECK_BAD( vel );
	VEC_CHECK_BAD( groundTrace.c.normal );
	vel -= ( vel * groundTrace.c.normal ) * groundTrace.c.normal;
	VEC_CHECK_BAD( vel );

	float speed = vel.Length();
	FLOAT_CHECK_BAD( speed );
	if ( speed < idMath::FLT_EPSILON ) {
		return vec3_origin;
	}

	float control = speed;
	FLOAT_CHECK_BAD( control );
	if ( !skiing ) {
		control = speed < 100.0f ? 100.0f : speed;
	}

	FLOAT_CHECK_BAD( control );
	float drop = control * kineticFriction * frictionScale * timeStep;
	FLOAT_CHECK_BAD( drop );

	float newspeed = speed - drop;
	FLOAT_CHECK_BAD( newspeed );
	idVec3 newVel;
	if ( newspeed < idMath::FLT_EPSILON || speed < idMath::FLT_EPSILON ) {
		newspeed = 0.0f;
		newVel.Zero();
	} else {
		newVel = vel * ( newspeed / speed );
		VEC_CHECK_BAD( newVel );
	}

	VEC_CHECK_BAD( newVel );
	return ( newVel - vel ) / timeStep;
}

/*
================
sdPhysics_JetPack::JumpForce
================
*/
idVec3 sdPhysics_JetPack::JumpForce( bool skiing, float upMove, float boost, float timeStep ) {
	if ( skiing ) {
		return vec3_origin;
	}

	FLOAT_CHECK_BAD( upMove );
	if ( upMove < 0.5f ) {
		return vec3_origin;
	}

	if ( ( current.movementFlags & JPF_JUMPED ) || ( current.movementFlags & JPF_JUMP_HELD ) ) {
		return vec3_origin;
	}


	// ensure that it doesn't attempt to StepMove now that we've jumped
	current.onGround = false;
	current.onWater = false;
	current.movementFlags |= JPF_JUMPED | JPF_JUMP_HELD;
	return -jumpForce * gravityNormal;
}

/*
================
sdPhysics_JetPack::BoostForce
================
*/
idVec3 sdPhysics_JetPack::BoostForce( bool skiing, const idVec3& desiredMove, float boost, float timeStep ) {
	if ( boost < idMath::FLT_EPSILON ) {
		return vec3_origin;
	}

	idVec3 boostDir = -gravityNormal;
	VEC_CHECK_BAD( boostDir );
	idVec3 origBoostDir = boostDir;
	bool chopBoost = false;
	float upVel = boostDir * current.velocity;
	FLOAT_CHECK_BAD( upVel );
	if ( upVel > maxBoostSpeed ) {
		boostDir.Zero();
		chopBoost = true;
	}

	// add part of the directional movement to the boost
	// as long as its not exceeding the speed of that
	if ( desiredMove != vec3_origin ) {
		float moveDirSpeed = desiredMove * current.velocity;
		FLOAT_CHECK_BAD( moveDirSpeed );
		if ( moveDirSpeed < maxSpeed ) {
			boostDir += desiredMove;
			VEC_CHECK_BAD( boostDir );
		} else {
			// chop the boost so it FEELS like the input is being taken into account
			// instead of getting a vertical speedup whenever max horizontal speed is reached
			chopBoost = true;
		}

		boostDir.Normalize();
		VEC_CHECK_BAD( boostDir );
		if ( chopBoost ) {
			boostDir *= 0.7071f;
			VEC_CHECK_BAD( boostDir );
		}
	}

	idVec3 extraAccel = vec3_origin;
	if ( ( current.onGround || current.onWater ) && skiing ) {
		// boosted whilst on the ground & skiing
		// jump again
		if ( -groundTrace.c.normal * gravityNormal > 0.7f ) {
			current.movementFlags |= JPF_JUMPED;
			extraAccel = -jumpForce * gravityNormal;
			VEC_CHECK_BAD( extraAccel );
		}
	}

	// don't run StepMove
	current.onGround = false;
	current.onWater = false;
	current.movementFlags |= JPF_JUMP_HELD;

	fanForce = boost * origBoostDir * boostForce + extraAccel;
	return boost * boostDir * boostForce + extraAccel;
}

/*
================
sdPhysics_JetPack::Evaluate
================
*/
bool sdPhysics_JetPack::Evaluate( int timeStepMSec, int endTimeMSec ) {

	VEC_CHECK_BAD( current.velocity );
	VEC_CHECK_BAD( current.origin );

	current.velocity.FixDenormals();
	current.origin.FixDenormals();

	fanForce.Zero();

	float timeStep = MS2SEC( timeStepMSec );
	if ( timeStep == 0.f ) {
		return false;
	}

	if ( !groundTraceValid ) {
		CheckGround();
	}

	addedVelocity += current.pushVelocity;

	idVec3 oldOrigin = current.origin;

	// move the velocity into the frame of a pusher
	VEC_CHECK_BAD( current.pushVelocity );
	current.velocity -= current.pushVelocity;
	current.velocity -= addedVelocity;
	VEC_CHECK_BAD( current.velocity );

	// get the axes
	idVec3 viewForward, viewRight;
	ANG_CHECK_BAD( viewAngles );
	viewAngles.ToVectors( &viewForward, NULL, NULL );
	VEC_CHECK_BAD( viewForward );
	viewForward *= clipModelAxis;
	VEC_CHECK_BAD( viewForward );
	viewForward -= ( viewForward * gravityNormal ) * gravityNormal;
	VEC_CHECK_BAD( viewForward );
	viewForward.Normalize();
	VEC_CHECK_BAD( viewForward );
	viewRight = gravityNormal.Cross( viewForward );
	VEC_CHECK_BAD( viewRight );
	viewRight.Normalize();
	VEC_CHECK_BAD( viewRight );

	// calculate the move desired by the player
	float forwardFrac = command.forwardmove / 127.f;
	float rightFrac = command.rightmove / 127.f;
	float upFrac = command.upmove / 127.f;
	FLOAT_CHECK_BAD( forwardFrac );
	FLOAT_CHECK_BAD( rightFrac );
	FLOAT_CHECK_BAD( upFrac );

	idVec3 desired = forwardFrac * viewForward + rightFrac * viewRight;
	VEC_CHECK_BAD( desired );
	desired.Normalize();
	VEC_CHECK_BAD( desired );

	CalcWaterFraction();
	current.onWater |= waterFraction > 0.1f;

	// figure out the boost factor
	float useBoost = boost;
	if ( disableBoost || waterFraction > 0.5f ) {
		useBoost = 0.0f;
	}
	FLOAT_CHECK_BAD( useBoost );

	// scale boost for flight ceiling
	float height = current.origin.z;
	FLOAT_CHECK_BAD( height );
	if ( useBoost > 0.0f && height > gameLocal.flightCeilingLower ) {
		// get our height off the heightmap to help with this too
		float minBoostScale = 0.0f;
		const sdPlayZone* playZoneHeight = gameLocal.GetPlayZone( current.origin, sdPlayZone::PZF_HEIGHTMAP );
		if ( playZoneHeight != NULL ) {
			const sdHeightMapInstance& heightMap = playZoneHeight->GetHeightMap();
			if ( heightMap.IsValid() ) {
				float heightMapHeight = heightMap.GetInterpolatedHeight( current.origin );
				minBoostScale = ( current.origin.z - heightMapHeight ) / 512.0f;
				minBoostScale = 1.0f - idMath::ClampFloat( 0.0f, 1.0f, minBoostScale );
				minBoostScale = idMath::Pow( minBoostScale, 0.1f );
			}
		}

		if ( height > gameLocal.flightCeilingUpper ) {
			useBoost = 0.0f;
		} else {
			float boostScale = ( 1.0f - ( height - gameLocal.flightCeilingLower ) / ( gameLocal.flightCeilingUpper - gameLocal.flightCeilingLower ) );
			FLOAT_CHECK_BAD( boostScale );
			boostScale = idMath::ClampFloat( 0.0f, 1.0f, boostScale );
			FLOAT_CHECK_BAD( boostScale );
			useBoost *= idMath::Sqrt( boostScale );
			FLOAT_CHECK_BAD( useBoost );
		}

		if ( useBoost < minBoostScale ) {
			useBoost = minBoostScale;
		}
	}

	// clear out flags
	current.movementFlags &= ~( JPF_JUMPED | JPF_STEPPED_UP | JPF_STEPPED_DOWN );
	current.stepUp = 0.0f;

	bool skiing = ( current.movementFlags & JPF_JUMP_HELD ) && ( current.onGround || current.onWater );

	//
	// Gravity
	//
	idVec3 currentForce = vec3_origin;
	if ( !current.onWater ) {
		currentForce = gravityVector;
	} else if ( useBoost == 0.0f ) {
		float normalSpeed = current.velocity * gravityNormal;
		if ( jetPackSelf != NULL && jetPackSelf->GetPositionManager().IsEmpty() ) {
			currentForce = gravityVector * 0.5f;
			currentForce -= gravityNormal * normalSpeed * 3.0f;
		} else {
			float normalSpeed = current.velocity * gravityNormal;
			currentForce = -gravityVector * waterFraction - gravityNormal * normalSpeed * 3.0f;
		}
	}
	VEC_CHECK_BAD( currentForce );

	//
	// Booster
	//
	currentForce += BoostForce( skiing, desired, useBoost, timeStep );
	VEC_CHECK_BAD( currentForce );
	fanForce += desired * boostForce;

	//
	// Ground Movement
	//
	if ( current.onGround || current.onWater ) {
		// check if the player doesn't want to ski any more
		if ( upFrac < 0.1f ) {
			current.movementFlags &= ~JPF_JUMP_HELD;
		}

		// drop out of skiing if the velocity has dropped too far
		idVec3 surfaceVelocity = current.velocity - ( current.velocity * groundTrace.c.normal ) * groundTrace.c.normal;
		VEC_CHECK_BAD( surfaceVelocity );
		if ( skiing && surfaceVelocity.Length() < maxSpeed * 0.5f ) {
			skiing = false;
		}

		idVec3 frictionForce = FrictionForce( skiing, desired, currentForce, useBoost, timeStep );
		VEC_CHECK_BAD( frictionForce );
		if ( current.onWater ) {
			// reduced friction in water, and none against the direction of gravity
			frictionForce *= 0.1f;
			frictionForce -= ( frictionForce * gravityNormal ) * gravityNormal;
		}
		currentForce += frictionForce;

		if ( !current.onWater ) {
			// can't jump off water
			currentForce += JumpForce( skiing, upFrac, useBoost, timeStep );
			VEC_CHECK_BAD( currentForce );
		}

		currentForce += GroundMoveForce( skiing, desired, currentForce, useBoost, timeStep );
		VEC_CHECK_BAD( currentForce );
	}

	idVec3 newVelocity = current.velocity + currentForce * timeStep;
	VEC_CHECK_BAD( newVelocity );
	newVelocity += addedVelocity;

	//
	// Test if we can skip all the tracing code
	//
	bool skipTracing = false;
	if ( current.onGround ) {
		// if the velocity on the plane is zero
		idVec3 testVelocity = newVelocity - ( ( newVelocity * groundTrace.c.normal ) * groundTrace.c.normal );
		VEC_CHECK_BAD( testVelocity );
		if ( addedVelocity.IsZero() && testVelocity.Compare( vec3_origin, 0.0001f ) ) {
			skipTracing = true;
			newVelocity = testVelocity;
		}
	}
	if ( current.onWater && skipTracing ) {
		// if the velocity on the plane is zero & its not bobbing much
		idVec3 testVelocity = newVelocity - ( ( newVelocity * gravityNormal ) * gravityNormal );
		VEC_CHECK_BAD( testVelocity );
		float bobbing = idMath::Fabs( newVelocity * gravityNormal );
		if ( addedVelocity.IsZero() && testVelocity.Compare( vec3_origin, 0.1f ) && waterFraction < 0.15f && bobbing < 15.0f ) {
			skipTracing = true;
			newVelocity = testVelocity;
		}
	}

	addedVelocity.Zero();
	if ( skipTracing ) {
		current.velocity = newVelocity;
		current.velocity += current.pushVelocity;
		current.pushVelocity.Zero();
		self->OnPhysicsRested();
		return false;
	}

	//
	// Update to the new velocity and move
	//

	current.velocity = newVelocity;
	VEC_CHECK_BAD( current.velocity );

	ActivateContactEntities();
	clipModel->Unlink( gameLocal.clip );

	VEC_CHECK_BAD( current.origin );
	VEC_CHECK_BAD( current.velocity );
	VEC_CHECK_BAD( current.pushVelocity );
	SlideMove( true, true, true, 0, timeStep );
	current.origin.FixDenormals();
	current.velocity.FixDenormals();
	current.pushVelocity.FixDenormals();
	VEC_CHECK_BAD( current.origin );
	VEC_CHECK_BAD( current.velocity );
	VEC_CHECK_BAD( current.pushVelocity );

	clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );
	LinkShotModel();

	// check if on the ground
	CheckGround();

	// move the monster velocity back into the world frame
	current.velocity += current.pushVelocity;
	VEC_CHECK_BAD( current.velocity );
	current.pushVelocity.Zero();

	return ( current.origin != oldOrigin );
}

/*
==================
sdPhysics_JetPack::SlideMove

Returns true if the velocity was clipped in some way
==================
*/
#define	MAX_CLIP_PLANES			5
const float MIN_WALK_NORMAL		= 0.7f;		// can't walk on very steep slopes
const float PM_OVERCLIP			= 1.001f;
const float CONST_PM_STEPSCALE	= 1.0f;

bool sdPhysics_JetPack::SlideMove( bool stepUp, bool stepDown, bool push, int vehiclePush, float frametime ) {
	int			i, j, k, pushFlags;
	int			bumpcount, numbumps, numplanes;
	float		d, time_left, into, totalMass;
	idVec3		dir, planes[MAX_CLIP_PLANES];
	idVec3		end, stepEnd, primal_velocity, endVelocity, endClipVelocity, clipVelocity;
	trace_t		trace, stepTrace, downTrace;
	bool		nearGround, stepped, pushed, vehiclePushed;

	numbumps = 4;

	primal_velocity = current.velocity;
	bool groundPlane = current.onGround;

	endVelocity = current.velocity;

	time_left = frametime;

	// never turn against the ground plane
	if ( groundPlane ) {
		numplanes = 1;
		planes[0] = groundTrace.c.normal;

		float normalGroundVel = current.velocity * groundTrace.c.normal;
		if ( normalGroundVel < 0.0f ) {
			current.velocity -= normalGroundVel * groundTrace.c.normal;
		}
	} else {
		numplanes = 0;
	}

	// never turn against original velocity
	planes[numplanes] = current.velocity;
	planes[numplanes].Normalize();
	numplanes++;

	for ( bumpcount = 0; bumpcount < numbumps; bumpcount++ ) {

		// calculate position we are trying to move to
		end = current.origin + time_left * current.velocity;

		// see if we can make it there
		gameLocal.clip.Translation( CLIP_DEBUG_PARMS trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self );
 		time_left -= time_left * trace.fraction;
		current.origin = trace.endpos;
		CHECKCURRENT

		// if moved the entire distance
		if ( trace.fraction >= 1.0f ) {
			break;
		}

		stepped = pushed = vehiclePushed = false;

		// if we are allowed to step up
		if ( stepUp && ( trace.c.normal * -gravityNormal ) < MIN_WALK_NORMAL ) {

			nearGround = groundPlane || waterFraction < 0.2f;

			if ( !nearGround ) {
				// trace down to see if the player is near the ground
				// step checking when near the ground allows the player to move up stairs smoothly while jumping
				stepEnd = current.origin + maxStepHeight * gravityNormal;
				gameLocal.clip.Translation( CLIP_DEBUG_PARMS downTrace, current.origin, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );
				nearGround = ( downTrace.fraction < 1.0f && (downTrace.c.normal * -gravityNormal) > MIN_WALK_NORMAL );
			}

			// may only step up if near the ground or on a ladder or only shallowly in the water
			if ( nearGround ) {

				// step up
				stepEnd = current.origin - maxStepHeight * gravityNormal;
				gameLocal.clip.Translation( CLIP_DEBUG_PARMS downTrace, current.origin, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );

				// trace along velocity
				stepEnd = downTrace.endpos + time_left * current.velocity;
				gameLocal.clip.Translation( CLIP_DEBUG_PARMS stepTrace, downTrace.endpos, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );

				// step down
				stepEnd = stepTrace.endpos + maxStepHeight * gravityNormal;
				gameLocal.clip.Translation( CLIP_DEBUG_PARMS downTrace, stepTrace.endpos, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );

				if ( downTrace.fraction >= 1.0f || (downTrace.c.normal * -gravityNormal) > MIN_WALK_NORMAL ) {

					// if moved the entire distance
					if ( stepTrace.fraction >= 1.0f ) {
						time_left = 0;
						current.stepUp -= ( downTrace.endpos - current.origin ) * gravityNormal;
						current.origin = downTrace.endpos;
						CHECKCURRENT
						current.movementFlags |= JPF_STEPPED_UP;
						current.velocity *= CONST_PM_STEPSCALE;
						break;
					}

					// if the move is further when stepping up
					if ( stepTrace.fraction > trace.fraction ) {
						time_left -= time_left * stepTrace.fraction;
						current.stepUp -= ( downTrace.endpos - current.origin ) * gravityNormal;
						current.origin = downTrace.endpos;
						CHECKCURRENT
						current.movementFlags |= JPF_STEPPED_UP;
						current.velocity *= CONST_PM_STEPSCALE;
						trace = stepTrace;
						stepped = true;
					}
				}
			}
		}

		// if we can push other entities and not blocked by the world
		if ( push && trace.c.entityNum != ENTITYNUM_WORLD ) {

			clipModel->SetPosition( current.origin, clipModel->GetAxis(), gameLocal.clip );

			// clip movement, only push idMoveables, don't push entities the player is standing on
			// apply impact to pushed objects
			pushFlags = PUSHFL_CLIP|PUSHFL_ONLYMOVEABLE|PUSHFL_NOGROUNDENTITIES|PUSHFL_APPLYIMPULSE;

			// clip & push
			totalMass = gameLocal.push.ClipTranslationalPush( trace, self, pushFlags, end, end - current.origin, clipModel );

			if ( totalMass > 0.0f ) {
				// decrease velocity based on the total mass of the objects being pushed ?
				current.velocity *= 1.0f - idMath::ClampFloat( 0.0f, 1000.0f, totalMass - 20.0f ) * ( 1.0f / 950.0f );
				pushed = true;
			}
	
			current.origin = trace.endpos;
			CHECKCURRENT
			time_left -= time_left * trace.fraction;

			// if moved the entire distance
			if ( trace.fraction >= 1.0f ) {
				break;
			}
		}

		// try to vehiclepush things out of the way
		if ( !stepped && !pushed && vehiclePush && trace.c.entityNum != ENTITYNUM_WORLD ) {
			idEntity* other = gameLocal.entities[ trace.c.entityNum ];
			idPhysics* otherPhysics = other->GetPhysics();
			idPhysics_Actor* actorPhysics = otherPhysics->Cast< idPhysics_Actor >();

			if ( actorPhysics != NULL ) {
				clipModel->Disable();
				idVec3 move = end - trace.endpos;
				if ( actorPhysics->VehiclePush( false, time_left, move, clipModel, vehiclePush ) == VPUSH_OK ) {
					vehiclePushed = true;
				}
				clipModel->Enable();
			}
		}

		if ( !stepped && !vehiclePushed ) {
			// let the entity know about the collision
			idEntity *ent = gameLocal.entities[ trace.c.entityNum ];
			ent->Hit( trace, current.velocity, self );			
			self->Collide( trace, current.velocity, -1 );
		}

		if ( numplanes >= MAX_CLIP_PLANES ) {
			// MrElusive: I think we have some relatively high poly LWO models with a lot of slanted tris
			// where it may hit the max clip planes
			current.velocity = vec3_origin;
			return true;
		}

		//
		// if this is the same plane we hit before, nudge velocity out along it,
		// which fixes some epsilon issues with non-axial planes
		//
		for ( i = 0; i < numplanes; i++ ) {
			if ( ( trace.c.normal * planes[i] ) > 0.999f ) {
				// clip into the trace normal just in case this normal is almost but not exactly the same as the groundTrace normal
				current.velocity.ProjectOntoPlane( trace.c.normal, PM_OVERCLIP );
				// also add the normal to nudge the velocity out
				current.velocity += trace.c.normal;
				break;
			}
		}
		if ( i < numplanes ) {
			continue;
		}
		planes[numplanes] = trace.c.normal;
		numplanes++;

		//
		// modify velocity so it parallels all of the clip planes
		//

		// find a plane that it enters
		for ( i = 0; i < numplanes; i++ ) {
			into = current.velocity * planes[i];
			if ( into >= 0.1f ) {
				continue;		// move doesn't interact with the plane
			}

			// slide along the plane
			clipVelocity = current.velocity;
			clipVelocity.ProjectOntoPlane( planes[i], PM_OVERCLIP );

			// slide along the plane
			endClipVelocity = endVelocity;
			endClipVelocity.ProjectOntoPlane( planes[i], PM_OVERCLIP );

			// see if there is a second plane that the new move enters
			for ( j = 0; j < numplanes; j++ ) {
				if ( j == i ) {
					continue;
				}
				if ( ( clipVelocity * planes[j] ) >= 0.1f ) {
					continue;		// move doesn't interact with the plane
				}

				// try clipping the move to the plane
				clipVelocity.ProjectOntoPlane( planes[j], PM_OVERCLIP );
				endClipVelocity.ProjectOntoPlane( planes[j], PM_OVERCLIP );

				// see if it goes back into the first clip plane
				if ( ( clipVelocity * planes[i] ) >= 0 ) {
					continue;
				}

				// slide the original velocity along the crease
				dir = planes[i].Cross( planes[j] );
				dir.Normalize();
				d = dir * current.velocity;
				clipVelocity = d * dir;

				dir = planes[i].Cross( planes[j] );
				dir.Normalize();
				d = dir * endVelocity;
				endClipVelocity = d * dir;

				// see if there is a third plane the the new move enters
				for ( k = 0; k < numplanes; k++ ) {
					if ( k == i || k == j ) {
						continue;
					}
					if ( ( clipVelocity * planes[k] ) >= 0.1f ) {
						continue;		// move doesn't interact with the plane
					}

					// stop dead at a tripple plane interaction
					current.velocity = vec3_origin;
					return true;
				}
			}

			// if we have fixed all interactions, try another move
			current.velocity = clipVelocity;
			endVelocity = endClipVelocity;
			break;
		}
	}

	// step down
	if ( stepDown && groundPlane ) {
		// don't evaluate the step if its opposing the direction of movement
		idVec3 stepDelta = gravityNormal * maxStepHeight;
		float stepDirectionality = stepDelta * current.velocity;
		if ( stepDirectionality > 0.0f ) {
			stepEnd = current.origin + stepDelta;
			gameLocal.clip.Translation( CLIP_DEBUG_PARMS downTrace, current.origin, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );
			if ( downTrace.fraction > 1e-4f && downTrace.fraction < 1.0f ) {
				current.stepUp -= ( downTrace.endpos - current.origin ) * gravityNormal;
				current.origin = downTrace.endpos;
				CHECKCURRENT
				current.movementFlags |= JPF_STEPPED_DOWN;
				current.velocity *= CONST_PM_STEPSCALE;
			}
		}
	}

	// come to a dead stop when the velocity orthogonal to the gravity flipped
	clipVelocity = current.velocity - gravityNormal * current.velocity * gravityNormal;
	endClipVelocity = endVelocity - gravityNormal * endVelocity * gravityNormal;
	if ( clipVelocity * endClipVelocity < 0.0f ) {
		current.velocity = gravityNormal * current.velocity * gravityNormal;
	}

	return bumpcount == 0;
}

/*
================
sdPhysics_JetPack::UpdateTime
================
*/
void sdPhysics_JetPack::UpdateTime( int endTimeMSec ) {
}

/*
================
sdPhysics_JetPack::GetTime
================
*/
int sdPhysics_JetPack::GetTime( void ) const {
	return gameLocal.time;
}

/*
================
sdPhysics_JetPack::GetImpactInfo
================
*/
void sdPhysics_JetPack::GetImpactInfo( const int id, const idVec3 &point, impactInfo_t *info ) const {
	info->invMass = invMass;
	info->invInertiaTensor.Zero();
	info->position.Zero();
	info->velocity = current.velocity;
}

/*
================
sdPhysics_JetPack::ApplyImpulse
================
*/
void sdPhysics_JetPack::ApplyImpulse( const int id, const idVec3 &point, const idVec3 &impulse ) {
	if ( noImpact ) {
		return;
	}
	addedVelocity += impulse * invMass;
	current.velocity += impulse * invMass;
	Activate();
}

/*
================
sdPhysics_JetPack::SaveState
================
*/
void sdPhysics_JetPack::SaveState( void ) {
	saved = current;
}

/*
================
sdPhysics_JetPack::RestoreState
================
*/
void sdPhysics_JetPack::RestoreState( void ) {
	current = saved;
	CHECKCURRENT

	clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );
	LinkShotModel();

	CheckGround();
}

/*
================
idPhysics_Player::GetOrigin
================
*/
const idVec3& sdPhysics_JetPack::GetOrigin( int id ) const {
	return current.origin;
}

/*
================
sdPhysics_JetPack::SetOrigin
================
*/
void sdPhysics_JetPack::SetOrigin( const idVec3 &newOrigin, int id ) {
	current.origin = newOrigin;
	CHECKCURRENT
	clipModel->Link( gameLocal.clip, self, 0, newOrigin, clipModel->GetAxis() );
	LinkShotModel();

	Activate();
}

/*
================
sdPhysics_JetPack::SetAxis
================
*/
void sdPhysics_JetPack::SetAxis( const idMat3 &newAxis, int id ) {
	clipModel->Link( gameLocal.clip, self, 0, clipModel->GetOrigin(), newAxis );
	LinkShotModel();

	Activate();
}

/*
================
sdPhysics_JetPack::Translate
================
*/
void sdPhysics_JetPack::Translate( const idVec3 &translation, int id ) {
	current.origin += translation;
	CHECKCURRENT
	clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );
	LinkShotModel();

	Activate();
}

/*
================
sdPhysics_JetPack::Rotate
================
*/
void sdPhysics_JetPack::Rotate( const idRotation &rotation, int id ) {
	current.origin *= rotation;
	CHECKCURRENT
	clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() * rotation.ToMat3() );
	LinkShotModel();
	Activate();
}

/*
================
sdPhysics_JetPack::SetLinearVelocity
================
*/
void sdPhysics_JetPack::SetLinearVelocity( const idVec3 &newLinearVelocity, int id ) {
	current.velocity = newLinearVelocity;
	Activate();
}

/*
================
sdPhysics_JetPack::GetLinearVelocity
================
*/
const idVec3 &sdPhysics_JetPack::GetLinearVelocity( int id ) const {
	return current.velocity;
}

/*
================
sdPhysics_JetPack::SetPushed
================
*/
void sdPhysics_JetPack::SetPushed( int deltaTime ) {
	// velocity with which the monster is pushed
	current.pushVelocity += ( current.origin - saved.origin ) / ( deltaTime * idMath::M_MS2SEC );
}




/*
================
sdPhysics_JetPack::GetPushedLinearVelocity
================
*/
const idVec3 &sdPhysics_JetPack::GetPushedLinearVelocity( const int id ) const {
	return current.pushVelocity;
}

const float	JETPACK_ORIGIN_MAX				= 32767.0f;
const int	JETPACK_ORIGIN_TOTAL_BITS		= 24;
const int	JETPACK_ORIGIN_EXPONENT_BITS	= idMath::BitsForInteger( idMath::BitsForFloat( JETPACK_ORIGIN_MAX ) ) + 1;
const int	JETPACK_ORIGIN_MANTISSA_BITS	= JETPACK_ORIGIN_TOTAL_BITS - 1 - JETPACK_ORIGIN_EXPONENT_BITS;

const float	JETPACK_VELOCITY_MAX			= 4000.0f;
const int	JETPACK_VELOCITY_TOTAL_BITS		= 16;
const int	JETPACK_VELOCITY_EXPONENT_BITS	= idMath::BitsForInteger( idMath::BitsForFloat( JETPACK_VELOCITY_MAX ) ) + 1;
const int	JETPACK_VELOCITY_MANTISSA_BITS	= JETPACK_VELOCITY_TOTAL_BITS - 1 - JETPACK_VELOCITY_EXPONENT_BITS;

const int	JETPACK_MOVEMENT_FLAGS_BITS		= 4;

/*
================
sdPhysics_JetPack::CheckNetworkStateChanges
================
*/
bool sdPhysics_JetPack::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_BASE( sdJetPackPhysicsNetworkData );

		if ( !baseData.origin.Compare( current.origin, idMath::FLT_EPSILON ) ) {
			return true;
		}

		if ( !baseData.velocity.Compare( current.velocity, idMath::FLT_EPSILON ) ) {
			return true;
		}

		NET_CHECK_FIELD( movementFlags, current.movementFlags );

		return false;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_BASE( sdJetPackPhysicsBroadcastData );

		NET_CHECK_FIELD( pushVelocity, current.pushVelocity );

		return false;
	}

	return false;
}

/*
================
sdPhysics_JetPack::WriteNetworkState
================
*/
void sdPhysics_JetPack::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdJetPackPhysicsNetworkData );

		newData.origin			= current.origin;
		newData.velocity		= current.velocity;
		newData.movementFlags	= current.movementFlags;

		msg.WriteDeltaVector( baseData.origin, newData.origin, JETPACK_ORIGIN_EXPONENT_BITS, JETPACK_ORIGIN_MANTISSA_BITS );
		msg.WriteDeltaVector( baseData.velocity, newData.velocity, JETPACK_VELOCITY_EXPONENT_BITS, JETPACK_VELOCITY_MANTISSA_BITS );
		msg.WriteDelta( baseData.movementFlags, newData.movementFlags, JETPACK_MOVEMENT_FLAGS_BITS );

		return;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdJetPackPhysicsBroadcastData );

		newData.pushVelocity	= current.pushVelocity;

		msg.WriteDeltaVector( baseData.pushVelocity, newData.pushVelocity, JETPACK_VELOCITY_EXPONENT_BITS, JETPACK_VELOCITY_MANTISSA_BITS );

		return;
	}
}

/*
================
sdPhysics_JetPack::ApplyNetworkState
================
*/
void sdPhysics_JetPack::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	traceCollection.ForceNextUpdate();
	if ( mode == NSM_VISIBLE ) {
		NET_GET_NEW( sdJetPackPhysicsNetworkData );

		current.origin			= newData.origin;
		CHECKCURRENT
		current.velocity		= newData.velocity;
		current.movementFlags	= newData.movementFlags;

		clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );
		LinkShotModel();

		groundTraceValid = false;

		self->UpdateVisuals();
		return;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_NEW( sdJetPackPhysicsBroadcastData );

		current.pushVelocity	= newData.pushVelocity;

		return;
	}
}

/*
================
sdPhysics_JetPack::ReadNetworkState
================
*/
void sdPhysics_JetPack::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdJetPackPhysicsNetworkData );

		newData.origin			= msg.ReadDeltaVector( baseData.origin, JETPACK_ORIGIN_EXPONENT_BITS, JETPACK_ORIGIN_MANTISSA_BITS );
		newData.velocity		= msg.ReadDeltaVector( baseData.velocity, JETPACK_VELOCITY_EXPONENT_BITS, JETPACK_VELOCITY_MANTISSA_BITS );
		newData.movementFlags	= msg.ReadDelta( baseData.movementFlags, JETPACK_MOVEMENT_FLAGS_BITS );

		newData.origin.FixDenormals();
		newData.velocity.FixDenormals();

		self->OnNewOriginRead( newData.origin );
		return;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdJetPackPhysicsBroadcastData );

		newData.pushVelocity	= msg.ReadDeltaVector( baseData.pushVelocity, JETPACK_VELOCITY_EXPONENT_BITS, JETPACK_VELOCITY_MANTISSA_BITS );

		return;
	}
}

/*
================
sdPhysics_JetPack::CreateNetworkStructure
================
*/
sdEntityStateNetworkData* sdPhysics_JetPack::CreateNetworkStructure( networkStateMode_t mode ) const {
	if ( mode == NSM_VISIBLE ) {
		return new sdJetPackPhysicsNetworkData();
	}
	if ( mode == NSM_BROADCAST ) {
		return new sdJetPackPhysicsBroadcastData();
	}
	return NULL;
}

/*
================
sdPhysics_JetPack::ResetNetworkState
================
*/
void sdPhysics_JetPack::ResetNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	traceCollection.ForceNextUpdate();
	if ( mode == NSM_BROADCAST ) {
		NET_GET_NEW( sdJetPackPhysicsBroadcastData );

		current.pushVelocity	= newData.pushVelocity;

		return;
	}
}

/*
================
sdPhysics_JetPack::SetPlayerInput
================
*/
void sdPhysics_JetPack::SetPlayerInput( const usercmd_t &cmd, const idAngles &newViewAngles, bool allowMovement ) {
	command			= cmd;
	viewAngles		= newViewAngles;		// can't use cmd.angles cause of the delta_angles
	movementAllowed	= allowMovement;

	ANG_CHECK_BAD( viewAngles );
}

/*
================
sdPhysics_JetPack::Activate
================
*/
void sdPhysics_JetPack::Activate( void ) {
	groundTraceValid = false;
	self->BecomeActive( TH_PHYSICS );
}

/*
=============
sdPhysics_JetPack::CalcWaterFraction
=============
*/
void sdPhysics_JetPack::CalcWaterFraction( void ) {
	waterFraction	= 0.f;
	idBounds absBounds = GetAbsBounds();

	const idClipModel* waterModel;
	int count = gameLocal.clip.ClipModelsTouchingBounds( CLIP_DEBUG_PARMS absBounds, CONTENTS_WATER, &waterModel, 1, NULL );
	if ( count && waterModel->GetNumCollisionModels() ) {
		idCollisionModel* model = waterModel->GetCollisionModel( 0 );
		int numPlanes = model->GetNumBrushPlanes();

		const idBounds& modelBounds = model->GetBounds();

		self->CheckWater( waterModel->GetOrigin(), waterModel->GetAxis(), model );

		for ( int i = 0; i < numPlanes; i++ ) {
			idPlane plane = model->GetBrushPlane( i );
			plane.TranslateSelf( waterModel->GetOrigin() );
			plane.Normal() *= waterModel->GetAxis();

			if ( plane.Distance( current.origin ) > 0 ) {
				// outside of water clipmodel
				return;
			}
		}

		float height = waterModel->GetOrigin().z - current.origin.z + modelBounds.GetMaxs().z;
		waterFraction = height / absBounds.Size().z;
		if ( waterFraction > 1.f ) {
			waterFraction = 1.f;
		}

	}
}


/*
================
sdPhysics_JetPack::VehiclePush
================
*/
#define VEHICLE_PUSH_EPSILON	0.5f

int sdPhysics_JetPack::VehiclePush( bool stuck, float timeDelta, idVec3& move, idClipModel* pusher, int pushCount ) {
	if ( pushCount > 3 ) {
		move.Zero();
		return VPUSH_BLOCKED;
	}

VEC_CHECK_BAD( move );

	// remove small components into the ground
	// not strictly necessary but it makes the player physics do one less trace
	if ( current.onGround ) {
		float groundMove = move * groundTrace.c.normal;
		if ( groundMove < 0.0f && groundMove > -0.1f ) {
			move -= groundMove * groundTrace.c.normal;
		}
	}

VEC_CHECK_BAD( move );

	if ( !stuck ) {
		// change the velocity so it satisfies the push
		// note it has to be changed, then restored back
		// could combine it and the required velocity of the move now, but it'd
		// mean that you get double-moved by the current velocity - exploits++
		idVec3 oldVelocity = current.velocity;
		idVec3 oldOrigin = current.origin;
VEC_CHECK_BAD( oldVelocity );
VEC_CHECK_BAD( oldOrigin );

		if ( timeDelta < MS2SEC( 1 ) ) {
			timeDelta = MS2SEC( 1 );
		}
		current.velocity = move / timeDelta;
VEC_CHECK_BAD( current.velocity );
		
//		pusher->Disable();
		SlideMove( true, true, false, pushCount + 1, timeDelta );
//		pusher->Enable();
		current.velocity.FixDenormals();
		current.origin.FixDenormals();
VEC_CHECK_BAD( current.velocity );
VEC_CHECK_BAD( current.origin );

		clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );	
		LinkShotModel();
		CheckGround();
		// HACK - force the driver to update its position
		if ( jetPackSelf != NULL ) {
			idPlayer* driver = jetPackSelf->GetPositionManager().FindDriver();
			if ( driver != NULL ) {
				driver->GetPhysics()->Evaluate( SEC2MS( timeDelta ), gameLocal.time + gameLocal.msec );
				driver->fl.allowPredictionErrorDecay = true;
				driver->UpdateAnimation();
			}
			jetPackSelf->UpdateJetPackVisuals();
		}

VEC_CHECK_BAD( current.velocity );
VEC_CHECK_BAD( current.origin );

		// see if we ended up moving the right way
		idVec3 moveDirection = move;
VEC_CHECK_BAD( moveDirection );
		float moveNeeded = moveDirection.Normalize();
FLOAT_CHECK_BAD( moveNeeded );
		idVec3 movedDelta = current.origin - oldOrigin;
VEC_CHECK_BAD( movedDelta );
		float movedAmount = moveDirection * movedDelta;
FLOAT_CHECK_BAD( movedAmount );

		current.velocity = oldVelocity;
VEC_CHECK_BAD( current.velocity );
		float oldMoveDirSpeed = current.velocity * moveDirection;
FLOAT_CHECK_BAD( oldMoveDirSpeed );
		current.pushVelocity = ( moveNeeded - oldMoveDirSpeed ) * moveDirection;
		current.pushVelocity.FixDenormals();
VEC_CHECK_BAD( current.pushVelocity );
		current.velocity += current.pushVelocity ;
VEC_CHECK_BAD( current.velocity );

//		gameRenderWorld->DebugArrow( colorYellow, current.origin + idVec3(0,0,64), current.origin + idVec3(0,0,64) + moveDirection * 128.0f, 4 );
		if ( movedAmount < moveNeeded - VEHICLE_PUSH_EPSILON ) {
			// didn't move all the way!
			// update the amount that we move
			move = move * movedAmount / moveNeeded;
VEC_CHECK_BAD( move );
			return VPUSH_BLOCKED;
		}
	} else {
		// that means we're inside the vehicle and its trying to push us out
		// this is normally caused because we're being pushed by the vehicle
		// and we try to walk towards it

		// try to find a point on the outside of the vehicle that we can teleport to
		// start by finding the normal of a nearby surface
		contactInfo_t contacts[ 2 ];
		int numContacts = gameLocal.clip.ContactsModel( CLIP_DEBUG_PARMS contacts, 2, current.origin, NULL, 4.0f, clipModel, clipModel->GetAxis(), -1, pusher, pusher->GetOrigin(), pusher->GetAxis() );
		idVec3 foundNormal = vec3_origin;

		if ( numContacts ) {
			// average out the normals of the contacts - this provides a good approximation
			for ( int i = 0; i < numContacts; i++ ) {
//				gameRenderWorld->DebugSphere( colorGreen, idSphere( contacts[ i ].point, 4.0f ) );
//				gameRenderWorld->DebugArrow( colorGreen, contacts[ i ].point, contacts[ i ].point + contacts[ i ].normal * 128.0f, 4 );

				foundNormal += contacts[ i ].normal;
			}
			foundNormal /= numContacts;
		} else {
			// embedded so far it doesn't find any contacts O_o
			// use a trace to try to find the normal
			idVec3 traceDir = pusher->GetAbsBounds().GetCenter() - current.origin;
			float traceDirLength = traceDir.Normalize();
VEC_CHECK_BAD( traceDir );
			if ( traceDirLength == 0.0f ) {
				// well and truly embedded - nothing we can do here
				move.Zero();
				return VPUSH_BLOCKED;
			}

			idVec3 normalFinderStart = current.origin - traceDir * 128.0f;
			idVec3 normalFinderEnd = current.origin + traceDir * 128.0f;
VEC_CHECK_BAD( normalFinderStart );
VEC_CHECK_BAD( normalFinderEnd );

			trace_t normalFinder;
			gameLocal.clip.TranslationModel( CLIP_DEBUG_PARMS normalFinder, normalFinderStart, normalFinderEnd, clipModel, 
																clipModel->GetAxis(), GetClipMask(), pusher,
																pusher->GetOrigin(), pusher->GetAxis() );
			if ( normalFinder.fraction == 1.0f ) {
				// well and truly embedded - nothing we can do here
				move.Zero();
				return VPUSH_BLOCKED;
			}

			foundNormal = normalFinder.c.normal;
		}

VEC_CHECK_BAD( foundNormal );

//			gameRenderWorld->DebugArrow( colorYellow, current.origin, current.origin + foundNormal * 128.0f, 4 );
		idVec3 start = current.origin + foundNormal * 32.0f;
		idVec3 end = current.origin - foundNormal * 32.0f;
VEC_CHECK_BAD( start );
VEC_CHECK_BAD( end );

		trace_t tr;
		gameLocal.clip.TranslationModel( CLIP_DEBUG_PARMS tr, start, end, clipModel, clipModel->GetAxis(), GetClipMask(), pusher,
																	pusher->GetOrigin(), pusher->GetAxis() );
		if ( tr.fraction < 1.0f ) {
VEC_CHECK_BAD( tr.endpos );

			// push to the new spot
			idEntity* other = pusher->GetEntity();
			if ( other != NULL ) {
				other->DisableClip( false );
			} else {
				pusher->Disable();
			}

			idVec3 pushOutMove = tr.endpos - current.origin;
			int pushOutResult = VehiclePush( false, timeDelta, pushOutMove, pusher, pushCount );

			if ( other != NULL ) {
				other->EnableClip();
			} else {
				pusher->Enable();
			}

			if ( pushOutResult == VPUSH_OK ) {
				// heres a point thats not inside the vehicle, and not inside the world! move there, use it as our new origin
				// proceed with the move, adjust it for the new origin
				return VehiclePush( false, timeDelta, move, pusher, pushCount );
			} else {
				return VPUSH_BLOCKED;
			}
		}
	}

	return VPUSH_OK;
}

/*
================
sdPhysics_JetPack::SetSelf
================
*/
void sdPhysics_JetPack::SetSelf( idEntity *e ) {
	jetPackSelf = e->Cast< sdJetPack >();
	assert( jetPackSelf != NULL );

	idPhysics_Actor::SetSelf( e );
}

/*
================
sdPhysics_JetPack::UnlinkClip
================
*/
void sdPhysics_JetPack::UnlinkClip( void ) {
	clipModel->Unlink( gameLocal.clip );
	shotClipModel->Unlink( gameLocal.clip );
}

/*
================
sdPhysics_JetPack::LinkClip
================
*/
void sdPhysics_JetPack::LinkClip( void ) {
	clipModel->Link( gameLocal.clip, self, 0, clipModel->GetOrigin(), clipModel->GetAxis() );
	LinkShotModel();
}

/*
================
sdPhysics_JetPack::DisableClip
================
*/
void sdPhysics_JetPack::DisableClip( bool activateContacting ) {
	if ( activateContacting ) {
		WakeEntitiesContacting( self, clipModel );
	}
	clipModel->Disable();
	shotClipModel->Disable();
}

/*
================
sdPhysics_JetPack::EnableClip
================
*/
void sdPhysics_JetPack::EnableClip( void ) {
	clipModel->Enable();
	shotClipModel->Enable();
	LinkClip();
}
