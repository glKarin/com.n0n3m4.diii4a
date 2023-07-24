// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Parachute.h"
#include "../PredictionErrorDecay.h"

/*
===============================================================================

	sdParachute

===============================================================================
*/
extern const idEventDef EV_SetOwner;

const idEventDef EV_SetDeployStart( "setDeployStart", '\0', DOC_TEXT( "Sets the game time at which the parachute should start applying force." ), 1, NULL, "f", "time", "Game time in seconds." );
const idEventDef EV_IsMovingTooSlow( "isMovingTooSlow", 'b', DOC_TEXT( "Returns whether the owner is moving too slow to be considered parachuting any more." ), 0, NULL );

CLASS_DECLARATION( sdScriptEntity, sdParachute )
	EVENT( EV_SetOwner,					sdParachute::Event_SetOwner )
	EVENT( EV_SetDeployStart,			sdParachute::Event_SetDeployStart )
	EVENT( EV_IsMovingTooSlow,			sdParachute::Event_IsMovingTooSlow )
END_CLASS

/*
================
sdParachute::Spawn
================
*/
void sdParachute::Spawn( void ) {
	maxSpeed = spawnArgs.GetFloat( "min_speed" );
	deployStartTime = -1;
	deployTime = SEC2MS( spawnArgs.GetFloat( "deploy_time", "1" ) );
	tooSlowTime = 0;

	radius = InchesToMetres( spawnArgs.GetFloat( "chute_diameter" ) / 2 );
	height = InchesToMetres( spawnArgs.GetFloat( "chute_height" ) / 2 );
	forceHeight = spawnArgs.GetFloat( "force_height" );

	Cd_up = spawnArgs.GetFloat( "drag_coefficient_up" );
	Cd_side = spawnArgs.GetFloat( "drag_coefficient_side" );
	Cl_side = spawnArgs.GetFloat( "lift_coefficient_side" );
	rho = spawnArgs.GetFloat( "air_density" );

	maxSideDrag = spawnArgs.GetFloat( "drag_side_max", "600" );

	scale = 1.0f;

	ownerOffset.Zero();
}

/*
================
sdParachute::Think
================
*/
void sdParachute::Think( void ) {
	if ( owner.IsValid() ) {
		// convert velocity to metric
		idVec3 velocity = InchesToMetres( owner->GetPhysics()->GetLinearVelocity() );
		
		float speedSquared = velocity.LengthSqr();		
		if ( velocity.z > -maxSpeed ) {
			if ( !gameLocal.isClient ) {
				if ( !tooSlowTime ) {
					tooSlowTime = gameLocal.time;
				}
			}
		} else if ( speedSquared > 0.01f ) {
			tooSlowTime = 0;

			if ( deployStartTime > 0 ) {
				float canopyScale = 1.0f;
				float time = gameLocal.time - deployStartTime;
				if ( time < deployTime ) {
					canopyScale = time / deployTime;
				}
			
				ApplyParachute( owner, canopyScale );
			}
		}
	}

	sdScriptEntity::Think();
}

/*
================
sdParachute::ApplyParachute
================
*/
void sdParachute::ApplyParachute( idEntity* owner, float canopyScale ) {
	// calculate how much to scale the parachute size by based on the owner's mass
	if ( owner->GetPhysics()->GetMass() > 0.0f ) {
		scale = idMath::Sqrt( owner->GetPhysics()->GetMass() / 100.0f );
	} else {
		scale = 1.0f;
	}

	canopyScale *= scale;

	const idMat3& axis = GetPhysics()->GetAxis();
	const idVec3& trueUp = axis[ 2 ];
	idVec3 forceUp( 0, 0, 1.0f );

	idVec3 velocity = InchesToMetres( owner->GetPhysics()->GetLinearVelocity() );
	// take angular velocity into account
	idVec3 angVel = owner->GetPhysics()->GetAngularVelocity();
	angVel.z = 0.0f;		// ignore yaw
	// calculate axis
	idVec3 angVelAxis = angVel;
	float angVelMagnitude = -angVelAxis.Normalize();
	// calculate direction along circle of rotation
	idVec3 tangent = trueUp.Cross( angVelAxis );

	idVec3 angVelResult = angVelMagnitude * InchesToMetres( forceHeight ) * canopyScale * tangent * 2.0f;
	angVelResult = angVelResult * axis;

	velocity += angVelResult;

	float upSpeed = velocity * forceUp;
	idVec3 sideVel = velocity - upSpeed * forceUp;
	float sideSpeedSq = sideVel.LengthSqr();

	
	idVec3 upDragDirection = forceUp;
	idVec3 sideDragDirection = velocity; //velocity.Normalize();
	sideDragDirection.Normalize();
	float upComponent = -( upDragDirection * sideDragDirection );

	sideDragDirection += upComponent * upDragDirection;
	
	//
	// Up axis
	//

	// don't react if in the opposite direction
	if ( upComponent < 0 ) {
		upComponent = 0;
	}
	float currentRadius = radius * canopyScale;
	float canopyArea = idMath::PI * currentRadius * currentRadius * upComponent;

	// now do drag on this axis
	float upDragMagnitude = 0.5 *  Cd_up * canopyArea * rho * upSpeed * upSpeed;
	idVec3 dragForce = upDragMagnitude * upDragDirection;

	//
	// Side axis
	//

	// calculate the cross-sectional area of the side profile of the canopy
	float sideArea = 2 * currentRadius * height * canopyScale * canopyScale;

	// now do drag on this axis
	float sideDragMagnitude = 0.5 *  Cd_side * sideArea * rho * sideSpeedSq;
	if ( sideDragMagnitude > maxSideDrag ) {
		sideDragMagnitude = maxSideDrag;
	} else if ( sideDragMagnitude < -maxSideDrag ) {
		sideDragMagnitude = -maxSideDrag;
	}

	dragForce = dragForce + sideDragMagnitude * sideDragDirection;


	//
	// Apply the forces
	//

	// convert this to an impulse
	idVec3 dragImpulse = dragForce * MS2SEC( gameLocal.msec );

	// this impulse is in kg.m/s - convert to game units (kg.in/s)
	dragImpulse = MetresToInches( dragImpulse );

	// apply it to the object
	owner->GetPhysics()->ApplyImpulse( 0, GetPhysics()->GetOrigin() + trueUp * forceHeight, dragImpulse );
}

/*
================
sdParachute::UpdateModelTransform
================
*/
void sdParachute::UpdateModelTransform( void ) {
	if ( IsBound() ) {
		// HACK: get the offset & angles just like the script does
		idVec3 origin;
		idMat3 axis;

		idEntity* master = GetBindMaster();
		idPlayer* playerMaster = master->Cast< idPlayer >();
		if ( playerMaster != NULL ) {
			origin = GetBindMaster()->GetLastPushedOrigin();
			origin.z += 80.0f;
			idAngles angles( 0.0f, 0.0f, 0.0f );
			angles.yaw = GetBindMaster()->GetLastPushedAxis().ToAngles().yaw;
			axis = angles.ToMat3();
		} else {
			origin = master->GetPhysics()->GetCenterOfMass();
			origin = origin * GetBindMaster()->GetLastPushedAxis() + GetBindMaster()->GetLastPushedOrigin();
			axis = GetBindMaster()->GetLastPushedAxis();
		}

		origin += axis * ownerOffset;

		// get the offset from the bind master
		renderEntity.origin	= origin;
		renderEntity.axis	= axis;
	} else {
		renderEntity.origin	= GetPhysics()->GetOrigin();
		renderEntity.axis	= GetPhysics()->GetAxis();
	}

	DoPredictionErrorDecay();
}

/*
================
sdParachute::Present
================
*/
void sdParachute::Present( void ) {
	// HACK: ensure its in the right position relative to the owner before pushing to renderer
	UpdateModelTransform();
	sdScriptEntity::Present();

	// make sure this entity is being interpolated after the entity it is bound to
	if ( !gameLocal.unlock.unlockedDraw && IsBound() ) {
		idEntity* master = GetBindMaster();
		if ( master->interpolateNode.InList() ) {
			interpolateNode.InsertAfter( master->interpolateNode );
		}
	}
}

/*
================
sdParachute::Event_SetOwner
================
*/
void sdParachute::Event_SetOwner( idEntity* _owner ) {
	owner = _owner;

	if( owner == NULL ) {
		ownerOffset.Zero();
	} else {
		ownerOffset = owner->spawnArgs.GetVector( "parachute_offset" );
	}
}

/*
================
sdParachute::Event_SetDeployStart
================
*/
void sdParachute::Event_SetDeployStart( float time ) {
	deployStartTime = SEC2MS( time );
}

/*
================
sdParachute::Event_IsMovingTooSlow
================
*/
void sdParachute::Event_IsMovingTooSlow( void ) {
	sdProgram::ReturnBoolean( tooSlowTime != 0 && ( gameLocal.time - tooSlowTime > SEC2MS( 0.1f ) ) );
}
