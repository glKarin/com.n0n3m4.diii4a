// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "DeliveryVehicle.h"
#include "../script/Script_Helper.h"
#include "../physics/Physics.h"
#include "../ContentMask.h"


#define JOTUN_FLY_HEIGHT			4096
#define JOTUN_FLY_HEIGHT_MIN		400
#define JOTUN_FLY_HEIGHT_RESCUE		2300

#define HOVER_HEIGHT_MIN		400
#define HOVER_HEIGHT_RESCUE		2100
#define HOVER_HEIGHT_AIM		2300
#define HOVER_DOWNCAST_LENGTH	2500


/*
===============================================================================

	sdDeliveryVehicle

===============================================================================
*/
const idEventDef EV_StartJotunDelivery( "startJotunDelivery", "fff" );
const idEventDef EV_StartJotunReturn( "startJotunReturn", "fff" );

const idEventDef EV_StartMagogDelivery( "startMagogDelivery", "fffvf" );
const idEventDef EV_StartMagogReturn( "startMagogReturn", "fffv" );

CLASS_DECLARATION( sdScriptEntity, sdDeliveryVehicle )
	EVENT( EV_StartJotunDelivery,		sdDeliveryVehicle::Event_StartJotunDelivery )
	EVENT( EV_StartJotunReturn,			sdDeliveryVehicle::Event_StartJotunReturn )

	EVENT( EV_StartMagogDelivery,		sdDeliveryVehicle::Event_StartMagogDelivery )
	EVENT( EV_StartMagogReturn,			sdDeliveryVehicle::Event_StartMagogReturn )
END_CLASS

/*
================
sdDeliveryVehicle::Spawn
================
*/
void sdDeliveryVehicle::Spawn( void ) {
	vehicleMode = VMODE_NONE;
	deliveryMode = DMODE_NONE;
	modeStartTime = 0;

	lastRollAccel = 0.0f;
	pathLength = 0.0f;
	pathSpeed = 0.0f;
	leadTime = 0.0f;
	itemRotation = 0.0f;
	endPoint.Zero();

	maxZAccel = spawnArgs.GetFloat( "max_thrust" );
	maxZVel = spawnArgs.GetFloat( "max_z_vel" );
}

/*
================
sdDeliveryVehicle::Think
================
*/
void sdDeliveryVehicle::Think( void ) {
	
	if ( vehicleMode == VMODE_JOTUN ) {
		Jotun_Think();
	} else if ( vehicleMode == VMODE_MAGOG ) {
		Magog_Think();
	}

	sdScriptEntity::Think();
}

/*
================
sdDeliveryVehicle::PostThink
================
*/
void sdDeliveryVehicle::PostThink( void ) {
	sdScriptEntity::PostThink();
}

/*
================
sdDeliveryVehicle::Event_StartJotunDelivery
================
*/
void sdDeliveryVehicle::Event_StartJotunDelivery( float startTime, float _pathSpeed, float _leadTime ) {
	vehicleMode = VMODE_JOTUN;
	deliveryMode = DMODE_DELIVER;
	modeStartTime = SEC2MS( startTime );

	pathSpeed = _pathSpeed;
	pathLength = PathGetLength();
	leadTime = _leadTime;
	endPoint = PathGetPoint( PathGetNumPoints() - 1 );
	endPoint.z += JOTUN_FLY_HEIGHT;
}

/*
================
sdDeliveryVehicle::Event_StartJotunReturn
================
*/
void sdDeliveryVehicle::Event_StartJotunReturn( float startTime, float _pathSpeed, float _leadTime ) {
	vehicleMode = VMODE_JOTUN;
	deliveryMode = DMODE_RETURN;
	modeStartTime = SEC2MS( startTime );

	pathSpeed = _pathSpeed;
	pathLength = PathGetLength();
	leadTime = _leadTime;
	endPoint = PathGetPoint( PathGetNumPoints() - 1 );
	endPoint.z += JOTUN_FLY_HEIGHT;
}

/*
================
sdDeliveryVehicle::Jotun_Think
================
*/
void sdDeliveryVehicle::Jotun_Think( void ) {
	if ( deliveryMode == DMODE_NONE || gameLocal.IsPaused() ) {
		return;
	}
	if ( PathGetNumPoints() < 2 ) {
		return;
	}

	float time = MS2SEC( gameLocal.time - modeStartTime );
	float frameTime = MS2SEC( gameLocal.msec );
	float aheadPosition = ( time + leadTime ) * pathSpeed;

	// look ahead by a couple of seconds
	idVec3 aheadPoint;
	idVec3 aheadPointDir;
	PathGetPosition( aheadPosition, aheadPoint );
	PathGetDirection( aheadPosition, aheadPointDir );
	aheadPoint.z += JOTUN_FLY_HEIGHT;
	
	if ( deliveryMode == DMODE_DELIVER ) {
		bool levelOut = false;
		if ( aheadPosition > pathLength - 10000.0f ) {
			levelOut = true;
		}
	
		Jotun_DoMove( aheadPoint, aheadPointDir, endPoint, levelOut, false, pathSpeed );
	} else if ( deliveryMode == DMODE_RETURN ) { 
		Jotun_DoMove( aheadPoint, aheadPointDir, endPoint, false, true, pathSpeed );
	}
}

/*
================
sdDeliveryVehicle::Jotun_DoMove
================
*/
void sdDeliveryVehicle::Jotun_DoMove( const idVec3& aheadPointIdeal, const idVec3& aheadPointDirIdeal, const idVec3& endPoint, bool levelOut, bool leaving, float pathSpeed ) {
	float frameTime = MS2SEC( gameLocal.msec );

	idVec3 aheadPoint = aheadPointIdeal;
	idVec3 aheadPointDir = aheadPointDirIdeal;

	const idVec3& origin = GetPhysics()->GetOrigin();
	const idVec3& velocity = GetPhysics()->GetLinearVelocity();
	const idMat3& axis = GetPhysics()->GetAxis();
	const idAngles angles = axis.ToAngles();
	const idVec3& angVel = GetPhysics()->GetAngularVelocity();
	
	const idVec3& currentFwd = axis[ 0 ];
	const idVec3& currentRight = axis[ 1 ];
	const idVec3& currentUp = axis[ 2 ];
	
	float rollVel = angVel * currentFwd;
	float pitchVel = angVel * currentRight;
	float yawVel = angVel * currentUp;

	float aheadToEnd = ( aheadPoint - endPoint ).Length();
	if ( aheadToEnd < 4096.0f && !leaving ) {
		aheadPoint.z += 4096.0f;
	}

	float currentHeight = JOTUN_FLY_HEIGHT_RESCUE;
	
	// check out ahead and see that we're not going to collide with something
	if ( velocity != vec3_origin ) {
		idVec3 futureSpot = velocity * 1.0f;
		futureSpot.z -= 4096.0f;
		futureSpot.Normalize();;
		futureSpot *= 3500.0f;
		futureSpot += origin;

		trace_t trace;
		gameLocal.clip.TraceBounds( CLIP_DEBUG_PARMS trace, origin, futureSpot, GetPhysics()->GetBounds(), axis, MASK_SOLID | MASK_OPAQUE, this );
		futureSpot = trace.endpos;
		float futureFrac = trace.fraction;
				
		float futureDist = futureFrac * 3500.0f;
		if ( futureDist < 3500.0f ) {
			int surfaceFlags = trace.c.material != NULL ? trace.c.material->GetSurfaceFlags() : 0;
			if ( !( surfaceFlags & SURF_NOIMPACT ) ) {
				float moveUp = ( 3500.0f - futureDist ) / 3500.0f;
				moveUp = idMath::Sqrt( moveUp );

				if ( aheadToEnd < 32.0f || leaving ) {
					aheadPoint = origin + currentFwd * 3500.0f;
					aheadPoint.z += moveUp * 4096.0f;
					aheadPointDir.z += moveUp * 10.0f;
				}
				
				aheadPoint.z += moveUp * 4096.0f;
				aheadPointDir.z += moveUp * 2.0f;
				aheadPointDir.Normalize();
			} else {
				futureDist = 3500.0f;
			}
		}
		
		currentHeight = futureDist;
		
//		gameRenderWorld->DebugCircle( colorRed, origin, idVec3( 0.0f, 0.0f, 1.0f ), 16, 8 );
//		gameRenderWorld->DebugArrow( colorRed, origin, futureSpot, 256 );
//		gameRenderWorld->DebugCircle( colorRed, futureSpot, idVec3( 0.0f, 0.0f, 1.0f ), 16, 8 );
	}
//	gameRenderWorld->DebugCircle( colorGreen, aheadPoint, idVec3( 0.0f, 0.0f, 1.0f ), 256, 16 );
	
	
	// generate a cubic spline between the two points
	idVec3 point = vec3_origin;

	{
		idVec3 x0 = origin;
		idVec3 x1 = aheadPoint;
		idVec3 dx0 = velocity * 3.0f;					// maintaining our current velocity is more important
		idVec3 dx1 = aheadPointDir * pathSpeed * 1.0f;	// than matching the destination vector
		
		// calculate coefficients
		idVec3 D = x0;
		idVec3 C = dx0;
		idVec3 B = 3*x1 - dx1 - 2*C - 3*D;
		idVec3 A = x1 - B - C - D;
		
		float distanceLeft = ( endPoint - origin ).Length();
		float lookaheadFactor = ( distanceLeft / 4096.0f ) * 0.5f;
		lookaheadFactor = idMath::ClampFloat( 0.2f, 0.5f, lookaheadFactor );
		
		point = ( A * lookaheadFactor + B )*lookaheadFactor*lookaheadFactor + C*lookaheadFactor + D;
	}

	idVec3 idealNewForward = point - origin;
	idealNewForward.Normalize();
	
	idVec3 delta = point - origin;
	idVec3 endDelta = endPoint - origin;
	endDelta.Normalize();
	endDelta.z = 0.0f;
	
	idAngles endAngles = endDelta.ToAngles().Normalize180();
	
	// treat the local delta as if the thing is on a flat plane - simplifies everything
	idVec3 localDelta = axis.TransposeMultiply( delta );

	idVec3 newVelocity = vec3_origin;
	idVec3 newAngVel = angVel;
	
	//
	// A flying vehicle needs to roll and then pull up to turn (to look cool)
	//
	
	//
	// Roll!
	float targetRoll = endAngles.roll;
	if ( !levelOut ) {
		targetRoll = -( localDelta.y / 1024.0f ) * 45.0f;
		targetRoll = idMath::ClampFloat( -30.0f, 30.0f, targetRoll );
	}

	float rollAmount = idMath::AngleNormalize180( targetRoll - angles.roll );
	
	// so we know how much we need to roll to get to our new angle
	// how fast do we want to go to get there?
	float targetRollVel = ( rollAmount / 15 ) * 30;
	targetRollVel = idMath::ClampFloat( -30.0f, 30.0f, targetRollVel );

	float maxRollAccel = 15.0f;
	if ( levelOut ) {
		maxRollAccel = 35.0f;
	}
	
	// now figure out what roll acceleration that needs, clamp it
	float rollAccel = idMath::AngleNormalize180( targetRollVel - rollVel ) / frameTime;
	rollAccel = idMath::ClampFloat( -maxRollAccel, maxRollAccel, rollAccel );
	
	// dampen out the rolling inputs, like its a guy on a stick controlling it
	rollAccel = rollAccel * 0.2f + lastRollAccel * 0.8f;
	if ( idMath::Fabs( rollAccel ) < idMath::FLT_EPSILON ) {
		rollAccel = 0.f;
	}
	newAngVel += rollAccel * frameTime * currentFwd;
	 
	lastRollAccel = rollAccel;
	
	//
	// Pull up!
	float targetPitch = -( localDelta.z / 256.0f ) * 15.0f;
	if ( !leaving ) {
		targetPitch = idMath::ClampFloat( -25.0f, 15.0f, targetPitch );
	} else {
		targetPitch = idMath::ClampFloat( -25.0f, 15.0f, targetPitch );
	}
		
	if ( leaving && targetPitch > -15.0f ) {
		targetPitch = -15.0f;
	}

	float pitchAmount = idMath::AngleNormalize180( targetPitch - angles.pitch );
	
	// so we know how much we need to pitch to get to our new angle
	// how fast do we want to go to get there?
	float targetPitchVel = ( pitchAmount / 8 ) * 15;
	if ( !leaving ) {
		targetPitchVel = idMath::ClampFloat( -25.0f, 15.0f, targetPitchVel );
	} else {
		targetPitchVel = idMath::ClampFloat( -35.0f, 15.0f, targetPitchVel );	
	}
	
	// now figure out what roll acceleration that needs, clamp it
	float pitchAccel = idMath::AngleNormalize180( targetPitchVel - pitchVel ) / frameTime;
	if ( !leaving ) {
		pitchAccel = idMath::ClampFloat( -12.0f, 12.0f, pitchAccel );
	} else { 
		pitchAccel = idMath::ClampFloat( -36.0f, 12.0f, pitchAccel );
	}
	
	newAngVel += pitchAccel * frameTime * currentRight;
		
	//
	// Yaw
	float targetYaw = angles.yaw;
	if ( !levelOut ) {
		targetRoll = -( localDelta.y / 1024.0f ) * 5.0f;
		targetRoll = idMath::ClampFloat( -8.0f, 8.0f, targetRoll );
	} else {
		targetYaw = endAngles.yaw;
	}
	
	float yawAmount = idMath::AngleNormalize180( targetYaw - angles.yaw );

	// so we know how much we need to yaw to get to our new angle
	// how fast do we want to go to get there?
	float targetYawVel = ( yawAmount / 8 ) * 15;
	targetYawVel = idMath::ClampFloat( -15.0f, 15.0f, targetYawVel );
	
	// now figure out what yaw acceleration that needs, clamp it
	float yawAccel = idMath::AngleNormalize180( targetYawVel - yawVel ) / frameTime;
	yawAccel = idMath::ClampFloat( -10.0f, 10.0f, yawAccel );
	
	newAngVel += yawAccel * frameTime * currentUp;		

	newVelocity = currentFwd * pathSpeed;
	if ( levelOut ) {
		// HACK: Drift back towards the target, to try to ensure it gets there ok
		newVelocity = newVelocity * 0.7f + endDelta * pathSpeed * 0.3f;
	}
	
	
	// HACK: ensure it never gets below the minimum height
	if ( currentHeight < JOTUN_FLY_HEIGHT_MIN ) {
		idVec3 newOrigin = origin;
		newOrigin.z = newOrigin.z - currentHeight + JOTUN_FLY_HEIGHT_MIN;
		GetPhysics()->SetOrigin( newOrigin );			
	} else if ( currentHeight < JOTUN_FLY_HEIGHT_RESCUE ) {
		float oldNewVelLength = newVelocity.Length();
	
		float scale = ( JOTUN_FLY_HEIGHT_RESCUE - currentHeight ) / ( JOTUN_FLY_HEIGHT_RESCUE - JOTUN_FLY_HEIGHT_MIN );
		scale = idMath::Sqrt( scale );
		float rescueVelocity = scale * ( JOTUN_FLY_HEIGHT_RESCUE - JOTUN_FLY_HEIGHT_MIN ) / 0.5f;
		float rescueAcceleration = ( rescueVelocity - velocity.z ) / frameTime;
		rescueAcceleration = idMath::ClampFloat( 0.0f, 1800.0f, rescueAcceleration );
		rescueVelocity = velocity.z + rescueAcceleration * frameTime;
		if ( rescueVelocity > newVelocity.z ) {
			newVelocity.z = rescueVelocity;
		}
		
		newVelocity.Normalize();
		newVelocity *= oldNewVelLength;
	}
	
	//
	// set the new stuff
	//
	GetPhysics()->SetLinearVelocity( newVelocity );
	GetPhysics()->SetAngularVelocity( newAngVel );
}


/*
================
sdDeliveryVehicle::Event_StartMagogDelivery
================
*/
void sdDeliveryVehicle::Event_StartMagogDelivery( float startTime, float _pathSpeed, float _leadTime, const idVec3& _endPoint, float _itemRotation ) {
	vehicleMode = VMODE_MAGOG;
	deliveryMode = DMODE_DELIVER;
	modeStartTime = SEC2MS( startTime );

	pathSpeed = _pathSpeed;
	pathLength = PathGetLength();
	leadTime = _leadTime;

	endPoint = _endPoint;
	itemRotation = _itemRotation;
}

/*
================
sdDeliveryVehicle::Event_StartMagogReturn
================
*/
void sdDeliveryVehicle::Event_StartMagogReturn( float startTime, float _pathSpeed, float _leadTime, const idVec3& _endPoint ) {
	vehicleMode = VMODE_MAGOG;
	deliveryMode = DMODE_RETURN;
	modeStartTime = SEC2MS( startTime );

	pathSpeed = _pathSpeed;
	pathLength = PathGetLength();
	leadTime = _leadTime;

	endPoint = _endPoint;
}

/*
================
sdDeliveryVehicle::Magog_Think
================
*/
void sdDeliveryVehicle::Magog_Think() {
	int numPoints = PathGetNumPoints();
	if ( numPoints < 2 ) {
		return;
	}
	if ( deliveryMode == DMODE_NONE || gameLocal.IsPaused() ) {
		return;
	}
	assert( gameLocal.msec != 0 );

	float time = MS2SEC( gameLocal.time - modeStartTime );
	float frameTime = MS2SEC( gameLocal.msec );
	
	float position = time * pathSpeed;

	// look ahead by a couple of seconds
	float aheadPosition = position + leadTime * pathSpeed;
	idVec3 aheadPoint;
	idVec3 aheadPointDir;
	PathGetPosition( aheadPosition, aheadPoint );
	PathGetDirection( aheadPosition, aheadPointDir );

	bool approachingEnd = false;
	bool clampRoll = true;
	float yawScale = 1.0f;
	bool slowNearEnd = false;

	if ( deliveryMode == DMODE_DELIVER ) {
		slowNearEnd = true;
		
		if ( position > pathLength - 6000.0f ) {
			approachingEnd = true;
			clampRoll = false;
			
			// scale the max yaw
			yawScale = ( ( pathLength - position ) / 4096.0f );
			yawScale = 5.0f - 5.0f * yawScale * yawScale * yawScale;
			yawScale = idMath::ClampFloat( 0.0f, 5.0f, yawScale );
		}

	} else {
		aheadPoint.z += HOVER_HEIGHT_AIM;

		if ( position < 4096.0f ) {
			clampRoll = false;
			
			// scale the max yaw
			yawScale = ( 4096.0f - position ) / 4096.0f;
			yawScale = 5.0f - 5.0f * yawScale * yawScale * yawScale;
			yawScale = idMath::ClampFloat( 0.0f, 5.0f, yawScale );
		}
	}

	Magog_DoMove( aheadPoint, aheadPointDir, endPoint, itemRotation, yawScale, approachingEnd, clampRoll, slowNearEnd, pathSpeed );
}

/*
================
sdDeliveryVehicle::Magog_DoMove
================
*/
void sdDeliveryVehicle::Magog_DoMove( const idVec3& aheadPointIdeal, const idVec3& aheadPointDir, const idVec3& endPoint, float itemRotation, float maxYawScale, bool orientToEnd, bool clampRoll, bool slowNearEnd, float pathSpeed ) {
	float frameTime = MS2SEC( gameLocal.msec );

	idVec3 aheadPoint = aheadPointIdeal;
	const idVec3& origin = GetPhysics()->GetOrigin();
	const idVec3& velocity = GetPhysics()->GetLinearVelocity();
	const idMat3& axis = GetPhysics()->GetAxis();
	const idAngles angles = axis.ToAngles();
	const idVec3& angVel = GetPhysics()->GetAngularVelocity();
	
	const idVec3& currentFwd = axis[ 0 ];
	const idVec3& currentRight = axis[ 1 ];
	const idVec3& currentUp = axis[ 2 ];
	
	float rollVel = angVel * currentFwd;
	float pitchVel = angVel * currentRight;
	float yawVel = angVel * currentUp;

	// find the current height of the vehicle
	idVec3 futureContribution = velocity * 1.0f;
	futureContribution.z = 0.0f;
	float futureContributionLength = futureContribution.Length();
	if ( futureContributionLength > 2000.0f ) {
		futureContribution = futureContribution * ( 2000.0f / futureContributionLength );
	}

	idVec3 futureSpot = origin + futureContribution;
	futureSpot.z -= HOVER_DOWNCAST_LENGTH;

	trace_t trace;
	gameLocal.clip.TraceBounds( CLIP_DEBUG_PARMS trace, origin, futureSpot, GetPhysics()->GetBounds(), axis, MASK_SOLID | MASK_OPAQUE, this );
	futureSpot = trace.endpos;

	// shift the ahead point based on the trace
	float currentHeight = origin.z - futureSpot.z;
	if ( currentHeight < HOVER_DOWNCAST_LENGTH ) { 
		aheadPoint.z = origin.z - currentHeight + HOVER_HEIGHT_AIM;
	} else {
		aheadPoint.z = origin.z - HOVER_DOWNCAST_LENGTH + HOVER_HEIGHT_AIM;
	}

//	sys.debugCircle( g_colorRed, origin, '0 0 1', 16, 8, 0 );
//	sys.debugArrow( g_colorRed, origin, futureSpot, 256, 0 );
//	sys.debugCircle( g_colorRed, futureSpot, '0 0 1', 16, 8, 0 );
//	sys.debugCircle( '0 1 0', aheadPoint, '0 0 1', 256.0f, 16, 0 );


	idVec3 point;
	float lookaheadFactor;

	{
		// generate a cubic spline between the two points
		idVec3 x0 = origin;
		idVec3 x1 = aheadPoint;
		idVec3 dx0 = velocity * 3.0f;					// maintaining our current velocity is more important
		idVec3 dx1 = aheadPointDir * pathSpeed * 1.0f;	// than matching the destination vector
		
		// calculate coefficients
		idVec3 D = x0;
		idVec3 C = dx0;
		idVec3 B = 3*x1 - dx1 - 2*C - 3*D;
		idVec3 A = x1 - B - C - D;
		
		float distanceLeft = ( endPoint - origin ).Length();
		lookaheadFactor = ( distanceLeft / 6096.0f ) * 0.5f;
		lookaheadFactor = idMath::ClampFloat( 0.2f, 0.5f, lookaheadFactor );
		if ( !slowNearEnd ) {
			lookaheadFactor = 0.5f;
		}
		
		point = ( A * lookaheadFactor + B )*lookaheadFactor*lookaheadFactor + C*lookaheadFactor + D;
	}


	//
	// Follower logic
	//
	idVec3 delta = point - origin;
	idVec3 aheadDelta = aheadPoint - origin;

	idVec3 newVelocity = vec3_origin;

	//
	// Z axis
	//

	// figure out what Z velocity is needed to get where we want to go within a frame
	float Zvel = aheadDelta.z / frameTime;
	Zvel = idMath::ClampFloat( -maxZVel, maxZVel, Zvel );
	
	// figure out what Z acceleration is neccessary
	float ZAccel = ( Zvel - velocity.z ) / frameTime;
	ZAccel = idMath::ClampFloat( -maxZAccel, maxZAccel, ZAccel );
	
	// chop the Z acceleration when its nearing the end - helps to avoid settling issues
	if ( lookaheadFactor < 0.5f ) {
		ZAccel *= idMath::Sqrt( lookaheadFactor * 2.0f );
	}
	
	Zvel = velocity.z + ZAccel * frameTime;
	
	// rapidly prevent acceleration downwards when its below the target
	if ( aheadDelta.z > 0.0f && Zvel < 0.0f ) {
		Zvel *= 0.9f;
	}
			
	//
	// X & Y
	//
	
	// ignore Z
	delta.z = 0.0f;
	aheadDelta.z = 0.0f;
	idVec3 flatVelocity = velocity;
	flatVelocity.z = 0.0f;

	float distance = delta.Length();
	idVec3 direction;
	if ( distance > idMath::FLT_EPSILON ) {
		direction = delta / distance;
	} else {
		direction = vec3_origin;
	}
	
	// figure out how fast it needs to go to get there in time
	float vel = distance / 0.5f;
	vel = idMath::ClampFloat( -pathSpeed, pathSpeed, vel );
	
	idVec3 vecVel = vel * direction;
	
	// figure out what acceleration is neccessary
	idVec3 velDelta = vecVel - flatVelocity;
	idVec3 velDeltaDir = velDelta;
	float velDeltaLength = velDeltaDir.Normalize();

	float accel = velDeltaLength / frameTime;
	accel = idMath::ClampFloat( -600.0f, 600.0f, accel );
	
	idVec3 vecAccel = accel * frameTime * velDeltaDir;
	newVelocity = flatVelocity + vecAccel;
	
	
	newVelocity.z = Zvel;
	
	//
	// Angles
	//
	idVec3 velDir = velocity;
	float velLength = velDir.Normalize();
	
	// calculate what acceleration we are undergoing
	idVec3 velAccel = ( newVelocity - velocity ) / frameTime;
	
	// calculate a component due to air resistance
	float speed = InchesToMetres( velLength );
	float rho = 1.2f;
	float sideArea = InchesToMetres( 650.0f ) * InchesToMetres( 650.0f );
	float Cd = 0.6f;
	float dragForceMagnitude = MetresToInches( 0.5 *  Cd * sideArea * rho * speed * speed );
	// assume mass is 10,000 -> I know this works nicely
	idVec3 dragAccel = ( dragForceMagnitude / 10000.f ) * velDir;
	
	idVec3 desiredAccel = velAccel + dragAccel;
	desiredAccel *= 0.4f;
	desiredAccel.z += MetresToInches( 9.8f );
	
	// ok, so we desire to be looking at the target
	idVec3 forwards = endPoint - origin;
	forwards.z = 0.0f;
	forwards.Normalize();
			
	if ( orientToEnd ) {
		idAngles targetAngles = ang_zero;
		targetAngles.yaw = idMath::AngleNormalize180( itemRotation );
		forwards = targetAngles.ToForward();
	}
	
	// figure out the axes corresponding to this orientation
	idVec3 up = desiredAccel;
	up.Normalize();
	idVec3 right = up.Cross( forwards );
	right.Normalize();
	forwards = right.Cross( up );
	forwards.Normalize();
	
	// convert that to an angles
	idAngles desiredAngles = ( idMat3( forwards, right, up ) ).ToAngles();
	if ( clampRoll ) {
		desiredAngles.roll = idMath::ClampFloat( -9.0f, 9.0f, desiredAngles.roll );
	} else {
		desiredAngles.roll = idMath::ClampFloat( -30.0f, 30.0f, desiredAngles.roll );
	}
	desiredAngles.pitch = idMath::ClampFloat( -30.0f, 30.0f, desiredAngles.pitch );

	// find the diff between that and what we currently have
	idAngles diffAngles = ( desiredAngles - angles ).Normalize180();
	diffAngles = diffAngles * 0.1f;
	diffAngles = diffAngles / frameTime;
	diffAngles *= 0.1f;

	
	// translate the old angular velocity back to an angle diff style value
	idAngles oldDiffAngles;
	oldDiffAngles.pitch = angVel * currentRight;
	oldDiffAngles.yaw = angVel * currentUp;
	oldDiffAngles.roll = angVel * currentFwd;
	
	// blend the old and the new to soften the quick changes
	diffAngles = oldDiffAngles * 0.9f + diffAngles * 0.1f;
	
	// figure out how much we're trying to change by in a single frame
	idAngles angleAccel = diffAngles - oldDiffAngles;
	float maxAngleAccel = 45.0f * frameTime;
	float maxYawAccel = maxAngleAccel * maxYawScale;
	
	angleAccel.pitch = idMath::ClampFloat( -maxAngleAccel, maxAngleAccel, angleAccel.pitch );
	angleAccel.yaw = idMath::ClampFloat( -maxYawAccel, maxYawAccel, angleAccel.yaw );
	angleAccel.roll = idMath::ClampFloat( -maxAngleAccel, maxAngleAccel, angleAccel.roll );

	diffAngles = oldDiffAngles + angleAccel;	
	idVec3 newAngVel = diffAngles.pitch * currentRight + 
						diffAngles.yaw * currentUp + 
						diffAngles.roll * currentFwd;

	// HACK: ensure it never gets below the minimum height
	if ( currentHeight < HOVER_HEIGHT_MIN ) {
		idVec3 newOrigin = origin;
		newOrigin.z = origin.z - currentHeight + HOVER_HEIGHT_MIN;
		GetPhysics()->SetOrigin( newOrigin );			
	} else if ( currentHeight < HOVER_HEIGHT_RESCUE ) {
		float oldNewVelLength = newVelocity.Length();
	
		float scale = ( HOVER_HEIGHT_RESCUE - currentHeight ) / ( HOVER_HEIGHT_RESCUE - HOVER_HEIGHT_MIN );
		scale = idMath::Sqrt( scale );
		float rescueVelocity = scale * ( HOVER_HEIGHT_RESCUE - HOVER_HEIGHT_MIN ) / 0.5f;
		float rescueAcceleration = ( rescueVelocity - velocity.z ) / frameTime;
		rescueAcceleration = idMath::ClampFloat( 0.0f, 1800.0f, rescueAcceleration );
		rescueVelocity = velocity.z + rescueAcceleration * frameTime;
		if ( rescueVelocity > newVelocity.z ) {
			newVelocity.z = rescueVelocity;
		}
		
		newVelocity.Normalize();
		newVelocity *= oldNewVelLength;
	}
			
	GetPhysics()->SetLinearVelocity( newVelocity );
	GetPhysics()->SetAxis( angles.ToMat3() );
	GetPhysics()->SetAngularVelocity( newAngVel );
}




/*
==============
sdDeliveryVehicleBroadcastData::MakeDefault
==============
*/
void sdDeliveryVehicleBroadcastData::MakeDefault( void ) {
	sdScriptEntityBroadcastData::MakeDefault();

	lastRollAccel = 0.0f;
}

/*
==============
sdDeliveryVehicleBroadcastData::Write
==============
*/
void sdDeliveryVehicleBroadcastData::Write( idFile* file ) const {
	sdScriptEntityBroadcastData::Write( file );

	file->WriteFloat( lastRollAccel );
}

/*
==============
sdDeliveryVehicleBroadcastData::Read
==============
*/
void sdDeliveryVehicleBroadcastData::Read( idFile* file ) {
	sdScriptEntityBroadcastData::Read( file );

	file->ReadFloat( lastRollAccel );
}


/*
================
sdDeliveryVehicle::ApplyNetworkState
================
*/
void sdDeliveryVehicle::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_NEW( sdDeliveryVehicleBroadcastData );

		lastRollAccel = newData.lastRollAccel;
	}

	sdScriptEntity::ApplyNetworkState( mode, newState );
}

/*
==============
sdDeliveryVehicle::ReadNetworkState
==============
*/
void sdDeliveryVehicle::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdDeliveryVehicleBroadcastData );

		// read state
		newData.lastRollAccel = msg.ReadDeltaFloat( baseData.lastRollAccel );
	}

	sdScriptEntity::ReadNetworkState( mode, baseState, newState, msg );
}

/*
==============
sdDeliveryVehicle::WriteNetworkState
==============
*/
void sdDeliveryVehicle::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdDeliveryVehicleBroadcastData );

		// update state
		newData.lastRollAccel = lastRollAccel;

		// write state
		msg.WriteDeltaFloat( baseData.lastRollAccel, newData.lastRollAccel );
	}

	sdScriptEntity::WriteNetworkState( mode, baseState, newState, msg );
}

/*
==============
sdDeliveryVehicle::CheckNetworkStateChanges
==============
*/
bool sdDeliveryVehicle::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_BASE( sdDeliveryVehicleBroadcastData );

		// note that lastRollAccel changing on its own will not make
		// this send a new packet, however it will be sent if any new packets are sent!
	}
	return sdScriptEntity::CheckNetworkStateChanges( mode, baseState );
}

/*
==============
sdDeliveryVehicle::CreateNetworkStructure
==============
*/
sdEntityStateNetworkData* sdDeliveryVehicle::CreateNetworkStructure( networkStateMode_t mode ) const {
	if ( mode == NSM_BROADCAST ) {
		return new sdDeliveryVehicleBroadcastData();
	}
	return sdScriptEntity::CreateNetworkStructure( mode );
}
