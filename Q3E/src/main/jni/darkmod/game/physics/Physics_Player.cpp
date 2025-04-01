/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
#include "precompiled.h"
#pragma hdrstop



#include "../Game_local.h"
#include "../DarkModGlobals.h"
#include "../Grabber.h"
#include "../BinaryFrobMover.h"
#include "../FrobDoor.h"
#include "Force_Push.h"

CLASS_DECLARATION( idPhysics_Actor, idPhysics_Player )
END_CLASS

// movement parameters
const float PM_STOPSPEED		= 100.0f;
const float PM_SWIMSCALE		= 1.0f;
const float PM_ROPESPEED		= 100.0f;
const float PM_LADDERSPEED		= 100.0f;
const float PM_STEPSCALE		= 1.0f;

const float PM_ACCELERATE		= 10.0f;
const float PM_AIRACCELERATE	= 1.0f;
const float PM_WATERACCELERATE	= 4.0f;
const float PM_FLYACCELERATE	= 8.0f;

const float PM_FRICTION			= 6.0f;
const float PM_AIRFRICTION		= 0.0f;
const float PM_WATERFRICTION	= 1.0f;
const float PM_FLYFRICTION		= 3.0f;
const float PM_NOCLIPFRICTION	= 12.0f;
const float PM_SLICK			= 0.1f; // grayman - #2409 - for slippery surfaces

/**
* Friction multiplier to stop the player more quickly.
* Should also be tested with slopes as the gravity direction is always treated equally.
**/
const float PM_STOPFRICTIONMUL	= 2.5f;

/**
* Low player speed boundary (squared), below which movement is fully stopped.
* Affects all directions except the current acceleration direction if there is acceleration.
**/
const float PM_MAXSTOPSPEEDSQR	= 14.0f;

/**
*  Height unit increment for mantle test
* This value should be >= 1.0
* A larger value reduces the number of tests during a mantle
* initiation, but may not find some small mantleable "nooks"
* in a surface.
**/
const float MANTLE_TEST_INCREMENT = 1.0;

/**
* Desired player speed below which ground friction is neglected
*
* This was determined for PM_FRICTION = 6.0 and should change if
*	PM_FRICTION changes from 6.0.
**/
const float PM_NOFRICTION_SPEED   = 71.0f;

const float MIN_WALK_NORMAL		  = 0.7f;  // can't walk on very steep slopes (run = 1, rise = 1)
const float MIN_WALK_SLICK_NORMAL = 0.89f; // grayman #2409 - higher value for slippery slopes (run = 1, rise = 0.5)
const float OVERCLIP			  = 1.001f;

// TODO (ishtvan): Move the following to INI file or player def file:

/**
* Defines the spot above the player's origin where they are attached to the rope
**/
const float ROPE_GRABHEIGHT		= 50.0f;

/**
* Distance the player is set back from the rope
**/
const float ROPE_DISTANCE		= 20.0f;

/**
* Angular tolarance for looking at a rope and grabbing it [deg]
**/
const float ROPE_ATTACHANGLE = 45.0f*idMath::PI/180.0f;
/**
* Angular tolerance for when to start rotating around the rope
* (This one doesn't have to be in the def file)
**/
const float ROPE_ROTANG_TOL = 1.0f*idMath::PI/180.0f;

/**
* When moving on a climbable surface, the player's predicted position is checked by this delta ahead
* To make sure it's still on a ladder surface.
**/
const float CLIMB_SURFCHECK_DELTA = 5.0f;

/**
* How far to check from the player origin along the surface normal to hit the surface in the above test
* Needs to allow for worst case overhang
**/
const float CLIMB_SURFCHECK_NORMDELTA = 20.0f;

/**
* how far away is the player allowed to push out from the climbable section?
* Measured from the last good attachment point at the origin
**/
const float LADDER_DISTAWAY = 15.0f;

/**
* Velocity with which the player is shoved over the top of the ladder
* This depends on LADDER_DISTAWAY.  If this velocity is too low, the player will
* fall down before they can make it over the top of the ladder
**/
const float LADDER_TOPVELOCITY = 80.0f;

/**
* Angle at which the player detaches from a ladder when their feet are walking on a surface
**/
const float LADDER_WALKDETACH_ANGLE = 45.0f;
const float LADDER_WALKDETACH_DOT = idMath::Cos( DEG2RAD( LADDER_WALKDETACH_ANGLE ) );

/**
* Player's eye needs to be closer than this in order to peek through an opening.
**/
const float PEEK_MAX_DIST = 22.0f; // grayman #4882

// movementFlags
const int PMF_DUCKED			= 1;		// set when ducking
const int PMF_JUMPED			= 2;		// set when the player jumped this frame
const int PMF_STEPPED_UP		= 4;		// set when the player stepped up this frame
const int PMF_STEPPED_DOWN		= 8;		// set when the player stepped down this frame
const int PMF_JUMP_HELD			= 16;		// set when jump button is held down
const int PMF_TIME_LAND			= 32;		// movementTime is time before rejump
const int PMF_TIME_KNOCKBACK	= 64;		// movementTime is an air-accelerate only time
const int PMF_TIME_WATERJUMP	= 128;		// movementTime is waterjump
const int PMF_ALL_TIMES			= (PMF_TIME_WATERJUMP|PMF_TIME_LAND|PMF_TIME_KNOCKBACK);

int c_pmove = 0;

void idPhysics_Player::SetSelf( idEntity *e )
{
	idPhysics_Base::SetSelf(e);
	m_PushForce->SetOwner(e);
}

/*
============
idPhysics_Player::CmdScale

Returns the scale factor to apply to cmd movements
This allows the clients to use axial -127 to 127 values for all directions
without getting a sqrt(2) distortion in speed.
============
*/
float idPhysics_Player::CmdScale( const usercmd_t &cmd ) const {
	int		max;
	float	total;
	float	scale;
	int		forwardmove;
	int		rightmove;
	int		upmove;

	forwardmove = cmd.forwardmove;
	rightmove = cmd.rightmove;

	// since the crouch key doubles as downward movement, ignore downward movement when we're on the ground
	// otherwise crouch speed will be lower than specified
	if ( walking ) {
		upmove = 0;
	} else {
		upmove = cmd.upmove;
	}

	max = abs( forwardmove );
	if ( abs( rightmove ) > max ) {
		max = abs( rightmove );
	}
	if ( abs( upmove ) > max ) {
		max = abs( upmove );
	}

	if ( !max ) {
		return 0.0f;
	}

	total = idMath::Sqrt( (float) forwardmove * forwardmove + rightmove * rightmove + upmove * upmove );
	scale = (float) playerSpeed * max / ( 127.0f * total );

	return scale;
}

/*
==============
idPhysics_Player::Accelerate

Handles user intended acceleration
==============
*/
void idPhysics_Player::Accelerate( const idVec3 &wishdir, const float wishspeed, const float accel ) {
#if 1
	// q2 style
	//float addspeed;
	float accelspeed, currentspeed;

	currentspeed = current.velocity * wishdir;
	/*addspeed = wishspeed - currentspeed;
	if (addspeed <= 0) {
		return;
	}*/
	// this acceleration has friction-like effects if currentspeed is bigger than wishspeed.
	accelspeed = accel * frametime * (wishspeed - currentspeed); // accel * frametime * wishspeed;
	/*if (accelspeed > addspeed) {
		accelspeed = addspeed;
	}*/
	current.velocity += accelspeed * wishdir;
#else
	// proper way (avoids strafe jump maxspeed bug), but feels bad
	idVec3		wishVelocity;
	idVec3		pushDir;
	float		pushLen;
	float		canPush;

	wishVelocity = wishdir * wishspeed;
	pushDir = wishVelocity - current.velocity;
	pushLen = pushDir.Normalize();

	canPush = accel * frametime * wishspeed;
	if (canPush > pushLen) {
		canPush = pushLen;
	}

	current.velocity += canPush * pushDir;
#endif
}

/*
==================
idPhysics_Player::SlideMove

Returns true if the velocity was clipped in some way
==================
*/
#define	MAX_CLIP_PLANES	5

bool idPhysics_Player::SlideMove( bool gravity, bool stepUp, bool stepDown, bool push, const float velocityLimit /*= -1.0*/ ) {
	int			i, j, k, pushFlags;
	int			bumpcount, numbumps, numplanes;
	float		d, time_left, into;
	float		totalMass = 0.0f;
	idVec3		dir, planes[MAX_CLIP_PLANES];
	idVec3		end, stepEnd, primal_velocity, endVelocity, endClipVelocity, clipVelocity;
	trace_t		trace, stepTrace, downTrace;
	bool		nearGround, stepped, pushed;

	numbumps = 4;

	primal_velocity = current.velocity;

	if ( gravity ) {
		endVelocity = current.velocity + gravityVector * frametime;
		current.velocity = ( current.velocity + endVelocity ) * 0.5f;
		primal_velocity = endVelocity;
		if ( groundPlane ) {
			// slide along the ground plane
			current.velocity.ProjectOntoPlane( groundTrace.c.normal, OVERCLIP );
		}
	}
	else {
		endVelocity = current.velocity;
	}

	time_left = frametime;

	// never turn against the ground plane
	if ( groundPlane ) {
		numplanes = 1;
		planes[0] = groundTrace.c.normal;
	} else {
		numplanes = 0;
	}

	// never turn against original velocity
	planes[numplanes] = current.velocity;
	planes[numplanes].Normalize();
	numplanes++;

	// BluePill : Push test tweaked for low frametimes (relies on hardcoded values)
	// needs further testing whether it can negatively affect mission gameplay
	if ( push && current.velocity.LengthSqr() > 0.0F )
	{
		// calculate the "real time" velocity as if the framerate was constantly 60fps
		// idPhysics_RigidBody::Evaluate (-> idODE_*::Evaluate) already takes account of the frametime
		// just applying current.velocity would cause the actual movement to be velocity*frametime² per frame, while it should be velocity*frametime.
		idVec3 realtimeVelocity = current.velocity;
		// prevent potential division by zero (minimum frametime should be 1ms)
		if (frametime > 0.0001F)
			realtimeVelocity *= (MS2SEC(USERCMD_MSEC) / frametime);

		// BluePill : Hardcoded trace range of 5.0, looked good for different crouch/walk speeds
		// calculate the end of the reachable range to push entities
		end = current.origin + current.velocity * ( 5.0F / current.velocity.LengthFast() );

		// see if we can make it there
		gameLocal.clip.Translation( trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self );

		// if we are not blocked by the world
		if ( trace.c.entityNum != ENTITYNUM_WORLD ) {

			clipModel->SetPosition( current.origin, clipModel->GetAxis() );

			// clip movement, only push idMoveables, don't push entities the player is standing on
			// apply impact to pushed objects
			pushFlags = PUSHFL_CLIP | PUSHFL_ONLYMOVEABLE | PUSHFL_NOGROUNDENTITIES | PUSHFL_APPLYIMPULSE;

			// clip & push

			// greebo: Don't use the idPusher
			//totalMass = gameLocal.push.ClipTranslationalPush( trace, self, pushFlags, end, end - current.origin, cv_pm_pushmod.GetFloat() );

			// Set the trace result to zero, we're pushing into things here
			// trace.fraction = 0.0f;
			trace.endpos = current.origin;

			// greebo: Check the entity in front of us
			idEntity* pushedEnt = gameLocal.entities[trace.c.entityNum];
			if (pushedEnt != NULL)
			{
				// Register the blocking physics object with our push force
				m_PushForce->SetPushEntity(pushedEnt, 0);
				m_PushForce->SetContactInfo(trace, realtimeVelocity);

				totalMass = pushedEnt->GetPhysics()->GetMass();
			}

			if (totalMass > 0.0f) {
				// decrease velocity based on the total mass of the objects being pushed ?
				current.velocity *= 1.0f - idMath::ClampFloat(0.0f, 1000.0f, totalMass - 20.0f) * (1.0f / 950.0f);

				pushed = true;
			}
		}
	}

	for ( bumpcount = 0; bumpcount < numbumps; bumpcount++ )
	{
		// calculate position we are trying to move to
		end = current.origin + time_left * current.velocity;

		// see if we can make it there
		gameLocal.clip.Translation( trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self );

		time_left -= time_left * trace.fraction;
		current.origin = trace.endpos;

		// if moved the entire distance
		if ( trace.fraction >= 1.0f )
		{
			break;
		}

		stepped = pushed = false;

		// if we are allowed to step up
		if ( stepUp ) 
		{
			nearGround = groundPlane || m_bOnClimb || m_bClimbDetachThisFrame;

			if ( !nearGround ) 
			{
				// trace down to see if the player is near the ground
				// step checking when near the ground allows the player to move up stairs smoothly while jumping
				stepEnd = current.origin + maxStepHeight * gravityNormal;
				gameLocal.clip.Translation( downTrace, current.origin, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );
				nearGround = ( downTrace.fraction < 1.0f && (downTrace.c.normal * -gravityNormal) > MIN_WALK_NORMAL );
			}

			// may only step up if near the ground or climbing
			if ( nearGround ) 
			{

				// step up
				stepEnd = current.origin - maxStepHeight * gravityNormal;
				gameLocal.clip.Translation( downTrace, current.origin, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );

				// greebo: Trace along the current velocity, but trace further ahead than we need to judge the situation
				idVec3 velocityNorm(current.velocity);
				velocityNorm.NormalizeFast();

				idVec3 fullForward = downTrace.endpos + maxStepHeight * velocityNorm;
				gameLocal.clip.Translation( stepTrace, downTrace.endpos, fullForward, clipModel, clipModel->GetAxis(), clipMask, self );

				// This is the max. distance we can move forward in this frame
				idVec3 forward = time_left * current.velocity;
				//float forwardDist = forward.LengthFast();

				// step down
				idVec3 topStartPoint = downTrace.endpos + forward;

				stepEnd = topStartPoint + maxStepHeight * gravityNormal;
				gameLocal.clip.Translation( downTrace, topStartPoint, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );

				if ( downTrace.fraction >= 1.0f || (downTrace.c.normal * -gravityNormal) > MIN_WALK_NORMAL ) 
				{
					// if moved the entire distance
					if ( stepTrace.fraction >= 1.0f ) 
					{
						time_left = 0;
						current.stepUp -= ( downTrace.endpos - current.origin ) * gravityNormal;
						current.origin = downTrace.endpos;
						current.movementFlags |= PMF_STEPPED_UP;
						current.velocity *= PM_STEPSCALE;
						break;
					}

					// if the move is further when stepping up
					if ( stepTrace.fraction > trace.fraction ) 
					{
						time_left -= time_left * stepTrace.fraction;
						current.stepUp -= ( downTrace.endpos - current.origin ) * gravityNormal;
						current.origin = downTrace.endpos;
						current.movementFlags |= PMF_STEPPED_UP;
						current.velocity *= PM_STEPSCALE;
						trace = stepTrace;
						stepped = true;
					}
				}
				else
				{
					// greebo: We have a sloped obstacle in front of us

					if (stepTrace.fraction >= 1.0f)
					{
						// We can step onto this obstacle, the stepTrace has shown that there is enough room 
						// in front of us (if we translate a bit upwards)
						current.stepUp -= ( downTrace.endpos - current.origin ) * gravityNormal;
						//static int factor = 10;
						current.origin = downTrace.endpos + time_left*current.velocity; //(fullForward - current.origin)*stepTrace.fraction;
						time_left = 0;
						current.movementFlags |= PMF_STEPPED_UP;
						current.velocity *= PM_STEPSCALE;
						// greebo: HACK ALARM: We add a "counter-gravity" this frame to avoid us from being dragged down again in the next frame
						// TODO: Maybe we can do this somewhere else, where the player is sliding off slopes.
						current.velocity -= gravityVector * frametime * 3;
						
						// BluePill: HACK to prevent sliding down curbs on high framerates.
						m_SlopeIgnoreTimer = (framemsec < USERCMD_MSEC) ? 250 : 0;
					}
				}

				DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING ("performing step up, velocity now %.4f %.4f %.4f\r",  current.velocity.x, current.velocity.y, current.velocity.z);
			}
		}

		// original push test code; the new push test is located above this loop
		/*// if we can push other entities and not blocked by the world
		if ( push && trace.c.entityNum != ENTITYNUM_WORLD ) {

			clipModel->SetPosition( current.origin, clipModel->GetAxis() );

			// clip movement, only push idMoveables, don't push entities the player is standing on
			// apply impact to pushed objects
			pushFlags = PUSHFL_CLIP | PUSHFL_ONLYMOVEABLE | PUSHFL_NOGROUNDENTITIES | PUSHFL_APPLYIMPULSE;

			// clip & push

			// greebo: Don't use the idPusher
			//totalMass = gameLocal.push.ClipTranslationalPush( trace, self, pushFlags, end, end - current.origin, cv_pm_pushmod.GetFloat() );

			// Set the trace result to zero, we're pushing into things here
			trace.fraction = 0.0f;
			trace.endpos = current.origin;

			// greebo: Check the entity in front of us
			idEntity* pushedEnt = gameLocal.entities[trace.c.entityNum];
			if (pushedEnt != NULL)
			{
				// Register the blocking physics object with our push force
				m_PushForce->SetPushEntity(pushedEnt, 0);
				m_PushForce->SetContactInfo(trace, current.velocity);

				totalMass = pushedEnt->GetPhysics()->GetMass();
			}

			if ( totalMass > 0.0f ) {
				// decrease velocity based on the total mass of the objects being pushed ?
				current.velocity *= 1.0f - idMath::ClampFloat( 0.0f, 1000.0f, totalMass - 20.0f ) * ( 1.0f / 950.0f );
				pushed = true;
			}

			current.origin = trace.endpos;
			time_left -= time_left * trace.fraction;

			// if moved the entire distance
			if ( trace.fraction >= 1.0f ) {
				break;
			}
		}*/

		if ( !stepped ) {
			// let the entity know about the collision
			self->Collide( trace, current.velocity );
		}

		if ( numplanes >= MAX_CLIP_PLANES ) {
			// MrElusive: I think we have some relatively high poly LWO models with a lot of slanted tris
			// where it may hit the max clip planes
			current.velocity = vec3_origin;
			return true;
		}

		//
		// if this is the same plane we hit before, nudge velocity
		// out along it, which fixes some epsilon issues with
		// non-axial planes
		//
		for ( i = 0; i < numplanes; i++ ) {
			if ( ( trace.c.normal * planes[i] ) > 0.999f ) {
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
			clipVelocity.ProjectOntoPlane( planes[i], OVERCLIP );

			// slide along the plane
			endClipVelocity = endVelocity;
			endClipVelocity.ProjectOntoPlane( planes[i], OVERCLIP );

			// see if there is a second plane that the new move enters
			for ( j = 0; j < numplanes; j++ ) {
				if ( j == i ) {
					continue;
				}
				if ( ( clipVelocity * planes[j] ) >= 0.1f ) {
					continue;		// move doesn't interact with the plane
				}

				// try clipping the move to the plane
				clipVelocity.ProjectOntoPlane( planes[j], OVERCLIP );
				endClipVelocity.ProjectOntoPlane( planes[j], OVERCLIP );

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
	if ( stepDown && groundPlane ) 
	{
		stepEnd = current.origin + gravityNormal * maxStepHeight;
		gameLocal.clip.Translation( downTrace, current.origin, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );
		if ( downTrace.fraction > 1e-4f && downTrace.fraction < 1.0f ) 
		{
			current.stepUp -= ( downTrace.endpos - current.origin ) * gravityNormal;
			current.origin = downTrace.endpos;
			current.movementFlags |= PMF_STEPPED_DOWN;
			current.velocity *= PM_STEPSCALE;

			DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING ("performing step down, velocity now %.4f %.4f %.4f\r",  current.velocity.x, current.velocity.y, current.velocity.z);
		}
	}

	if ( gravity ) {
		current.velocity = endVelocity;
	}

	// come to a dead stop when the velocity orthogonal to the gravity flipped
	clipVelocity = current.velocity - gravityNormal * current.velocity * gravityNormal;
	endClipVelocity = endVelocity - gravityNormal * endVelocity * gravityNormal;
	if ( clipVelocity * endClipVelocity < 0.0f ) {
		current.velocity = gravityNormal * current.velocity * gravityNormal;
	}

	// Limit velocity
	if (velocityLimit >= 0.0)
	{
		const float fSqrdVelocity = current.velocity.LengthSqr();
		if (fSqrdVelocity > velocityLimit*velocityLimit)
		{
			current.velocity *= idMath::RSqrt(fSqrdVelocity) * velocityLimit;
		}
	}

	return (bool)( bumpcount == 0 );
}

/*
==================
idPhysics_Player::Friction

Handles both ground friction and water friction
==================
*/
void idPhysics_Player::Friction( const idVec3 &wishdir, const float forceFriction )
{
	idVec3 vel = current.velocity;

	if ( walking ) {
		// ignore slope movement, remove all velocity in gravity direction
		vel -= (vel * gravityNormal) * gravityNormal;
	}

	/*
	 * stgatilov: this does not allow to start walking sometimes:
	 *   https://forums.thedarkmod.com/index.php?/topic/22294-beta-testing-212/&do=findComment&comment=491835
	 * (all these snapping-like tweaks are bad in general, so nothing to lose here)
	float speed = vel.Length();
	if ( speed < 1.0f ) {
		// remove all movement orthogonal to gravity, allows for sinking underwater
		if ( fabs( current.velocity * gravityNormal ) < 1e-5f ) {
			current.velocity.Zero();
		} else {
			current.velocity = (current.velocity * gravityNormal) * gravityNormal;
		}
		// greebo: We still want the player to slow down when reaching the surface
		// This is where velocities are getting really small, but never actually reach 0,
		// that's why this z-friction is necessary.
		current.velocity.z *= cv_pm_water_z_friction.GetFloat();
		return;
	}
	*/

	// float drop = 0;
	float friction = 0.0f;

	if (forceFriction > 1e-3f) {
		friction = forceFriction;
	}
	// spectator friction
	else if ( current.movementType == PM_SPECTATOR ) {
		// TODO if anyone is crazy enough to add multiplayer and spectator mode to TDM : Check whether this works as intented!
		friction = PM_FLYFRICTION;
		//drop += speed * PM_FLYFRICTION * frametime;
	}
	// apply ground friction
	else if ( walking && waterLevel <= WATERLEVEL_FEET )
	{
		if ( !(current.movementFlags & PMF_TIME_KNOCKBACK) ) // if getting knocked back, no friction
		{
			// grayman - #2409 - less friction on slick surfaces

			friction = PM_FRICTION; // default
			if (groundMaterial && (groundMaterial->IsSlick()))
			{
				friction *= PM_SLICK; // reduce friction
			}

			/*float control = (speed < PM_STOPSPEED) ? PM_STOPSPEED : speed;
			drop += control * friction * frametime;*/
		}
	}
	// apply water friction even if just wading
	else if (waterLevel) {
		friction = PM_WATERFRICTION * waterLevel;
		//drop += speed * PM_WATERFRICTION * waterLevel * frametime;
	}
	// apply air friction
	else {
		friction = PM_AIRFRICTION;
		//drop += speed * PM_AIRFRICTION * frametime;
	}

	// if there is no player intended movement
	if (wishdir.LengthSqr() <= 1e-5f) {
		friction *= PM_STOPFRICTIONMUL;
	}

	// bluepill: don't apply friction to the current acceleration direction as the acceleration calculation does that already.
	// don't set drop as this friction calculation doesn't treat all velocity components equally
	idVec3 frictionComponent = vel - ((vel * wishdir) * wishdir);
	/*if (frictionComponent.LengthSqr() <= PM_MAXSTOPSPEEDSQR) {
		// fully stop movement for slow speeds
		// stgatilov #6333: this causes player to hang on the top of jump with high FPS
		// moreover, I'm not sure this is necessary... there is similar code in this function above
		current.velocity -= frictionComponent;
	}
	else*/ {
		current.velocity -= frictionComponent * (friction * frametime); // -1.0 for 100% frictionComponent
	}

	// scale the velocity
	/*float newspeed = speed - drop;
	if (newspeed < 0) {
		newspeed = 0;
	}
	current.velocity *= ( newspeed / speed );*/
}

/*
===================
idPhysics_Player::WaterJumpMove

Flying out of the water

REMOVED from DarkMod
===================
*/


/*
===================
idPhysics_Player::WaterMove
===================
*/
void idPhysics_Player::WaterMove()
{
	// Keep track of whether jump is held down for mantling out of water
	if ( command.upmove > 10 ) 
	{
		current.movementFlags |= PMF_JUMP_HELD;
	}
	else
	{
		current.movementFlags &= ~PMF_JUMP_HELD;
	}

	// Lower ranged weapons while swimming
	static_cast<idPlayer*>(self)->SetImmobilization( "WaterMove", EIM_ATTACK_RANGED );

	//Friction();

	float scale = CmdScale( command );

	idVec3 wishvel;

	// user intentions
	if ( !scale ) {
		// greebo: Standard downwards velocity is configurable via this CVAR
		if (waterLevel >= WATERLEVEL_HEAD)
		{
			// greebo: Player is completely submersed, apply upwards velocity, but let it raise over time
			float factor = (gameLocal.framenum - submerseFrame) / 60;
			if (factor > 1) {
				factor = 1;
			}

			if (static_cast<idPlayer*>(self)->m_CrouchIntent)
			{
				// greebo: we probably have crouch mode activated
				// Set the factor to 0, toggle crouch can here be used to keep the player at the current height
				factor = 0;
			}

			// This makes the player slowly rise/sink in water
			wishvel = gravityNormal * cv_pm_water_downwards_velocity.GetFloat() * factor;
		}
		else
		{
			wishvel.Zero();
		}
	} else {

		// Regular swim speed
		wishvel = scale * (viewForward * command.forwardmove + viewRight * command.rightmove);
		wishvel -= scale * gravityNormal * command.upmove;

		// stifu #3550: Simulate swimming motion via additive modulation of speed
		// 1) Sinosodial lead-in
		// 2) max-speed plateau
		// 3) Sinosodial lead-out
		// Sum of all phase portions has to be 1.0f
		static const float fLeadInPortion = 0.4f;
		static const float fLeadOutPortion = 0.2f;
		assert(fLeadInPortion + fLeadOutPortion < 1.0f);

		static const float fMaxSpeedPortion = 1.0f - fLeadInPortion - fLeadOutPortion;
		assert(fMaxSpeedPortion >= 0.0f && fMaxSpeedPortion < 1.0f);

		// (Re-)initialize members
		if (cv_pm_swimspeed_frequency.IsModified() || cv_pm_swimspeed_variation.IsModified()
			|| m_fSwimLeadInDuration_s < 0.0f	   || m_fSwimLeadOutStart_s < 0.0f 
			|| m_fSwimLeadOutDuration_s < 0.0f     || m_fSwimSpeedModCompensation < 0.0f)
		{
			const float fFrequency = cv_pm_swimspeed_frequency.GetFloat();
			m_fSwimLeadInDuration_s = fLeadInPortion / fFrequency;
			const float fSwimMaxSpeedDuration_s = fMaxSpeedPortion / fFrequency;
			m_fSwimLeadOutStart_s = m_fSwimLeadInDuration_s + fSwimMaxSpeedDuration_s;
			m_fSwimLeadOutDuration_s = fLeadOutPortion / fFrequency;

			// Max-speed plateau changes avg. speed -> Compensate that by subtracting
			// a constant offset from the speed
			for (int i = 0; i < 2; i++) // In case the first computation is invalid
			{
				const float fMaxSpeedPortionTotal = fSwimMaxSpeedDuration_s
					/ (m_fSwimLeadInDuration_s + fSwimMaxSpeedDuration_s + m_fSwimLeadOutDuration_s);

				m_fSwimSpeedModCompensation = 1.0f - cv_pm_swimspeed_variation.GetFloat() * fMaxSpeedPortionTotal;

				// NOTE: cv_pm_swimspeed_variation could be incompatible with function.
				//       Try to correct the value in that case and recompute compensation
				if (m_fSwimSpeedModCompensation - cv_pm_swimspeed_variation.GetFloat() < 0.0f)
					cv_pm_swimspeed_variation.SetFloat(1/fMaxSpeedPortionTotal);
				else
					break;
			}
			
			cv_pm_swimspeed_frequency.ClearModified();
			cv_pm_swimspeed_variation.ClearModified();
		}

		// Modulate swimming speed for the animation
		if (m_fSwimTimeStart_s < m_fSwimLeadInDuration_s)
		{
			wishvel *= (m_fSwimSpeedModCompensation - cv_pm_swimspeed_variation.GetFloat() 
				* idMath::Cos(idMath::PI * m_fSwimTimeStart_s / m_fSwimLeadInDuration_s));
		} else if (m_fSwimTimeStart_s < m_fSwimLeadOutStart_s)
		{
			wishvel *= (m_fSwimSpeedModCompensation + cv_pm_swimspeed_variation.GetFloat());
			if (!m_bSwimSoundStarted)
				PlaySwimBurstSound();
		}
		else if (m_fSwimTimeStart_s < (m_fSwimLeadOutStart_s + m_fSwimLeadOutDuration_s))
		{
			const float fTimeInLeadOut_s = m_fSwimTimeStart_s - m_fSwimLeadOutStart_s;
			wishvel *= (m_fSwimSpeedModCompensation + cv_pm_swimspeed_variation.GetFloat() 
				* idMath::Cos(idMath::PI * fTimeInLeadOut_s / m_fSwimLeadOutDuration_s));
		}
		else
		{
			// Animation finished. Restart it!
			m_fSwimTimeStart_s = -frametime;
			m_bSwimSoundStarted = false;
		}
		
		m_fSwimTimeStart_s += frametime;
	}

	idVec3 wishdir = wishvel;
	float wishspeed = wishdir.Normalize();

	if ( wishspeed > playerSpeed * PM_SWIMSCALE ) {
		wishspeed = playerSpeed * PM_SWIMSCALE;
	}

	idPhysics_Player::Accelerate( wishdir, wishspeed, PM_WATERACCELERATE );

	// don't apply friction on the wishdir direction
	idPhysics_Player::Friction( wishdir );

	// make sure we can go up slopes easily under water
	if ( groundPlane && ( current.velocity * groundTrace.c.normal ) < 0.0f ) {
		float vel = current.velocity.Length();
		// slide along the ground plane
		current.velocity.ProjectOntoPlane( groundTrace.c.normal, OVERCLIP );

		current.velocity.Normalize();
		current.velocity *= vel;
	}

	idPhysics_Player::SlideMove( false, true, false, false );
}

void idPhysics_Player::PlaySwimBurstSound()
{
	idStr sSound("snd_swim_burst");
	
	idPlayer* pPlayer = static_cast<idPlayer*>(self);
	if (pPlayer == nullptr)
		return;

	// speed mod
	if (pPlayer->m_CreepIntent)
	{
		sSound += "_creep";
	} else if (pPlayer->usercmd.buttons & BUTTON_RUN)
	{
		sSound += "_run";
	}
	else
	{
		sSound += "_walk";
	}	

	// waterlevel mod
	if (waterLevel >= WATERLEVEL_HEAD)
		sSound += "_underwater";
	
	pPlayer->StartSound(sSound, SND_CHANNEL_BODY, 0, false, nullptr);

	m_bSwimSoundStarted = true;
}

/*
===================
idPhysics_Player::FlyMove
===================
*/
void idPhysics_Player::FlyMove( void ) {
	idVec3	wishvel;
	float	wishspeed;
	idVec3	wishdir;
	float	scale;

	// normal slowdown
	//idPhysics_Player::Friction();

	scale = idPhysics_Player::CmdScale( command );

	if ( !scale ) {
		wishvel = vec3_origin;
	} else {
		wishvel = scale * (viewForward * command.forwardmove + viewRight * command.rightmove);
		wishvel -= scale * gravityNormal * command.upmove;
	}

	wishdir = wishvel;
	wishspeed = wishdir.Normalize();

	idPhysics_Player::Accelerate( wishdir, wishspeed, PM_FLYACCELERATE );

	// don't apply friction on the wishdir direction
	idPhysics_Player::Friction( wishdir );

	idPhysics_Player::SlideMove( false, false, false, false );
}

/*
===================
idPhysics_Player::AirMove
===================
*/
void idPhysics_Player::AirMove( void ) {
	idVec3		wishvel;
	idVec3		wishdir;
	float		wishspeed;
	float		scale;

	//idPhysics_Player::Friction();

	scale = idPhysics_Player::CmdScale( command );

	// project moves down to flat plane
	viewForward -= (viewForward * gravityNormal) * gravityNormal;
	viewRight -= (viewRight * gravityNormal) * gravityNormal;
	viewForward.Normalize();
	viewRight.Normalize();

	wishvel = viewForward * command.forwardmove + viewRight * command.rightmove;
	wishvel -= (wishvel * gravityNormal) * gravityNormal;
	wishdir = wishvel;
	wishspeed = wishdir.Normalize();
	wishspeed *= scale;

	// not on ground, so little effect on velocity
	idPhysics_Player::Accelerate( wishdir, wishspeed, PM_AIRACCELERATE * m_AirAccelerate );
	
	// don't apply friction on the wishdir direction
	idPhysics_Player::Friction( wishdir );

	// we may have a ground plane that is very steep, even
	// though we don't have a groundentity
	// slide along the steep plane
	if ( groundPlane ) {
		current.velocity.ProjectOntoPlane( groundTrace.c.normal, OVERCLIP );
	}

	idPhysics_Player::SlideMove( true, false, false, false );

	m_bMidAir = true;
}

/*
===================
idPhysics_Player::WalkMove
===================
*/
void idPhysics_Player::WalkMove( void ) 
{
	

	if ( waterLevel > WATERLEVEL_WAIST && (viewForward * groundTrace.c.normal) > 0.0f )
	{
		// begin swimming
		WaterMove();
		return;
	}

	if ( CheckJump() ) {
		// jumped away
		if ( waterLevel > WATERLEVEL_FEET ) {
			WaterMove();
		}
		else {
			AirMove();
		}
		return;
	}

	// BluePill : Move friction calculation after acceleration.
	//Friction();

	float scale = CmdScale( command );

	// project moves down to flat plane
	viewForward -= (viewForward * gravityNormal) * gravityNormal;
	viewRight -= (viewRight * gravityNormal) * gravityNormal;

	// project the forward and right directions onto the ground plane
	viewForward.ProjectOntoPlane( groundTrace.c.normal, OVERCLIP );
	viewRight.ProjectOntoPlane( groundTrace.c.normal, OVERCLIP );
	//
	viewForward.Normalize();
	viewRight.Normalize();

	idVec3 wishdir = viewForward * command.forwardmove + viewRight * command.rightmove;
	float wishspeed = wishdir.Normalize();
	wishspeed *= scale;

	// clamp the speed lower if wading or walking on the bottom
	if ( waterLevel )
	{
		float waterScale = waterLevel / 3.0f;
		waterScale = 1.0f - ( 1.0f - PM_SWIMSCALE ) * waterScale;
		if ( wishspeed > playerSpeed * waterScale ) {
			wishspeed = playerSpeed * waterScale;
		}
	}

	float accelerate = 0;

	// when a player gets hit, they temporarily lose full control, which allows them to be moved a bit
	if ( /*( groundMaterial && groundMaterial->IsSlick() ) || grayman #2409 */ current.movementFlags & PMF_TIME_KNOCKBACK ) {
		accelerate = PM_AIRACCELERATE;
	}
	else 
	{
		accelerate = PM_ACCELERATE;
		
		//FIX: If the player is moving very slowly, bump up their acceleration
		// so they don't get stuck to the floor by friction.
		if( playerSpeed < PM_NOFRICTION_SPEED )
		{
			accelerate *= 3.0f;
		}
	}

	Accelerate( wishdir, wishspeed, accelerate );

	if ( /*( groundMaterial && groundMaterial->IsSlick() ) || grayman #2409 */ current.movementFlags & PMF_TIME_KNOCKBACK )
	{
		current.velocity += gravityVector * frametime;
	}

	// BluePill : don't apply friction on the wishdir direction, Accelerate does that already.
	Friction( wishdir );

	idVec3 oldVelocity = current.velocity;

	// slide along the ground plane
	current.velocity.ProjectOntoPlane( groundTrace.c.normal, OVERCLIP );

	// if not clipped into the opposite direction
	if ( oldVelocity * current.velocity > 0.0f )
	{
		float newVel = current.velocity.LengthSqr();

		if ( newVel > 1.0f )
		{
			float oldVel = oldVelocity.LengthSqr();

			if ( oldVel > 1.0f )
			{
				// don't decrease velocity when going up or down a slope
				current.velocity *= idMath::Sqrt( oldVel / newVel );
			}
		}
	}

	// don't do anything if standing still
	idVec3 vel = current.velocity - (current.velocity * gravityNormal) * gravityNormal;

	if ( !vel.LengthSqr() )
	{
		// greebo: We're not moving, so let's clear the push entity
		m_PushForce->SetPushEntity(NULL);
		return;
	}

	gameLocal.push.InitSavingPushedEntityPositions();

	SlideMove( false, true, true, true );
}

/*
==============
idPhysics_Player::DeadMove
==============
*/
void idPhysics_Player::DeadMove( void ) {
	float	forward;

	if ( !walking ) {
		return;
	}

	// extra friction
	forward = current.velocity.Length();
	forward -= 20;
	if ( forward <= 0 ) {
		current.velocity = vec3_origin;
	}
	else {
		current.velocity.Normalize();
		current.velocity *= forward;
	}
}

/*
===============
idPhysics_Player::NoclipMove
===============
*/
void idPhysics_Player::NoclipMove( void ) {
	//float		speed, drop, friction, newspeed, stopspeed;
	float		scale, wishspeed;
	idVec3		wishdir;

	// friction
	/*speed = current.velocity.Length();
	if ( speed < 20.0f ) {
		current.velocity = vec3_origin;
	}
	else {
		stopspeed = playerSpeed * 0.3f;
		if ( speed < stopspeed ) {
			speed = stopspeed;
		}
		friction = PM_NOCLIPFRICTION;
		drop = speed * friction * frametime;

		// scale the velocity
		newspeed = speed - drop;
		if (newspeed < 0) {
			newspeed = 0;
		}

		current.velocity *= newspeed / speed;
	}*/

	// accelerate
	scale = idPhysics_Player::CmdScale( command );

	wishdir = scale * (viewForward * command.forwardmove + viewRight * command.rightmove);
	wishdir -= scale * gravityNormal * command.upmove;
	wishspeed = wishdir.Normalize();
	wishspeed *= scale;

	idPhysics_Player::Accelerate( wishdir, wishspeed, PM_ACCELERATE );

	// don't apply friction on the wishdir direction
	idPhysics_Player::Friction( wishdir, PM_NOCLIPFRICTION );

	// move
	current.origin += frametime * current.velocity;
}

/*
===============
idPhysics_Player::SpectatorMove
===============
*/
void idPhysics_Player::SpectatorMove( void ) {
	idVec3	wishvel;
	float	wishspeed;
	idVec3	wishdir;
	float	scale;

	// fly movement

	// idPhysics_Player::Friction();

	scale = idPhysics_Player::CmdScale( command );

	if ( !scale ) {
		wishvel = vec3_origin;
	} else {
		wishvel = scale * (viewForward * command.forwardmove + viewRight * command.rightmove);
	}

	wishdir = wishvel;
	wishspeed = wishdir.Normalize();

	idPhysics_Player::Accelerate( wishdir, wishspeed, PM_FLYACCELERATE );
	
	// don't apply friction on the wishdir direction
	idPhysics_Player::Friction( wishdir );

	idPhysics_Player::SlideMove( false, false, false, false );
}

/*
============
idPhysics_Player::RopeMove
============
*/
// TODO: This will probably reverse the up/down controls for upside-down ropes
// Would need some fix where desired direction of travel is flipped
// if climbDir * gravNormal is < 0
#pragma warning( disable : 4533 )
void idPhysics_Player::RopeMove( void ) 
{
	idVec3	wishvel, ropeBodyOrig;
	idVec3	ropePoint, offset, newOrigin, climbDir;
	float	wishspeed, scale, temp, deltaYaw, deltaAng1, deltaAng2;
	float	upscale, ropeTop, ropeBot; // z coordinates of the top and bottom of rope
	idBounds ropeBounds;
	trace_t transTrace; // used for clipping tests when moving the player
	idVec3 transVec, forward, playerVel(0,0,0), PlayerPoint(0,0,0);
	// shaft of rope in the rope coordinates, may need to be a spawnarg if rope models are inconsistent
	idVec3 ropeShaft( 0.0f, 0.0f, 1.0f );
	int bodID(0);
	idPhysics_AF *ropePhys;
	idPlayer *player = static_cast<idPlayer *>(self);

	if( !m_RopeEntity.GetEntity() )
	{
		RopeDetach();
		return; // early exit
	}

	ropePhys = static_cast<idPhysics_AF *>(m_RopeEntity.GetEntity()->GetPhysics());

	// stick the player to the rope at an AF origin point closest to their arms
	PlayerPoint = current.origin - gravityNormal*ROPE_GRABHEIGHT;
	ropeBodyOrig = ropePhys->NearestBodyOrig( PlayerPoint, &bodID );
	climbDir = ropePhys->GetAxis( bodID ) * ropeShaft;
	// Find the rope attachment point, which now moves along the individual AF body axis
	// Drawing a diagram, we find:
	// (orig - ropeOrig) * gravNormal = | offset along rope axis | * (rope axis * gravNormal)
	// Solve for offset along rope axis
	idVec3 deltaPlayerBody = PlayerPoint - ropeBodyOrig;

	// float offsetMag = (deltaPlayerBody * gravityNormal) / (climbDir * gravityNormal);

	// angua: the other method got huge values for offsetMag at large angles 
	// between the climbDir and the gravity axis.
	float offsetMag = deltaPlayerBody * climbDir;
	ropePoint = ropeBodyOrig + offsetMag * climbDir;
	
	// Uncomment for climb axis debugging
	// gameRenderWorld->DebugArrow( colorRed, ropeBodyOrig, ropePoint, 1 );

	SetRefEntVel( m_RopeEntity.GetEntity(), bodID );
	// move the player velocity into the rope reference frame
	current.velocity -= m_RefEntVelocity;

	// store and then kill the player's transverse velocity
	playerVel = current.velocity;
	current.velocity = (current.velocity * climbDir) * climbDir;

	// apply the player's weight to the AF body - COMMENTED OUT DUE TO AF CRAZINESS
//	ropePhys->AddForce(bodID, ropePoint, mass * gravityVector );
	
	// if the player has hit the rope this frame, apply an impulse based on their velocity
	// pretend the deceleration takes place over a number of frames for realism (100 ms?)
	if( m_bJustHitRope )
	{
		m_bJustHitRope = false;

		// greebo: Apply an impulse on the entity the rope arrow is bound to
		idEntity* ropeBindMaster = m_RopeEntity.GetEntity()->GetBindMaster();
		if (ropeBindMaster != NULL)
		{
			idVec3 direction = GetGravityNormal();

			idPhysics* bindMasterPhysics = ropeBindMaster->GetPhysics();
			
			const idVec3& ropeOrigin = ropePhys->GetOrigin();

			idAFBody* topMostBody = ropePhys->GetBody(0);
			if (topMostBody != NULL)
			{
				// Correct the pull direction using the orientation of the topmost body.
				//const idMat3& axis = topMostBody->GetWorldAxis();
				direction = topMostBody->GetWorldAxis() * ropeShaft;
			}

			bindMasterPhysics->ApplyImpulse(0, ropeOrigin, direction * mass * cv_tdm_rope_pull_force_factor.GetFloat());
		}

		idVec3 vImpulse = playerVel - (playerVel * climbDir) * climbDir;
		vImpulse *= mass;

		// ishtvan fix: Always translational force, do not torque the rope body
		// ropePhys->AddForce( bodID, ropePoint, vImpulse/0.1f );
		ropePhys->AddForce( bodID, ropePhys->GetOrigin(bodID), vImpulse/0.1f, this );
	}

// ======================== Rope Swinging =====================
	if ( player->usercmd.buttons & BUTTON_ATTACK ) {
		bool newKick = !(player->oldButtons & BUTTON_ATTACK) && (gameLocal.time - m_RopeKickTime) > cv_pm_rope_swing_reptime.GetInteger();
		bool kickContinued = (gameLocal.time - m_RopeKickTime) < cv_pm_rope_swing_duration.GetInteger();
		if (newKick || kickContinued) {
			// default kick direction is forward
			idVec3 kickDir = player->firstPersonViewAxis[0];
			idVec3 bodyOrig = ropePhys->GetOrigin(bodID);
			idMat3 rotDir = mat3_identity;
			// apply modifiers if holding left/right/back
			if( common->ButtonState(UB_MOVELEFT) )
			{
				rotDir = idAngles(0.0f, 90.0f, 0.0f).ToMat3();
			}
			else if( common->ButtonState(UB_MOVERIGHT) )
			{
				rotDir = idAngles(0.0f, 270.0f, 0.0f).ToMat3();
			}
			else if( common->ButtonState(UB_BACK) )
			{
				rotDir = idAngles(0.0f, 180.0f, 0.0f).ToMat3();
			}
			kickDir = rotDir * kickDir;

			if (newKick) {
				// do a trace to see if a solid is in the way, if so, kick off of it
				trace_t trKick;
		
				gameLocal.clip.TracePoint
					( 
						trKick, bodyOrig, 
						bodyOrig + cv_pm_rope_swing_kickdist.GetFloat()*kickDir,
						MASK_SOLID, self 
					);
				if( trKick.fraction < 1.0f )
				{
					// reverse direction to kick off
					kickDir *= -1.0f;
					// apply reaction force to entity kicked (TODO: watch out for exploits)
					idEntity *kickedEnt = gameLocal.entities[trKick.c.entityNum];
					float kickMag = cv_pm_rope_swing_impulse.GetFloat();// / 25.0f; // divide by 25, it takes a lot to move AFs for some reason
					kickedEnt->ApplyImpulse( self, trKick.c.id, trKick.c.point, -kickMag * kickDir );
				}

				// test: apply velocity to all bodies lower as well?
				m_RopeKickTime = gameLocal.time;
			}

			// project to XY plane
			kickDir -= GetGravityNormal() * (kickDir*GetGravityNormal());
			kickDir.Normalize();
		
			float force = cv_pm_rope_swing_impulse.GetFloat() / MS2SEC(cv_pm_rope_swing_duration.GetFloat());
			ropePhys->AddForce( bodID, bodyOrig, kickDir * force, this );
		}
	}

// ==== Translate the player to the rope attachment point =====

	offset = (current.origin - ropePoint);
	offset.ProjectOntoPlane( -gravityNormal );
	offset.Normalize();
	offset *= ROPE_DISTANCE;

	newOrigin = ropePoint + offset;
	transVec = newOrigin - current.origin;
	transVec -= (transVec * gravityNormal)*gravityNormal;

	// check whether the player will clip anything, and only translate up to that point
	ClipTranslation(transTrace, transVec, NULL);
	newOrigin = current.origin + (transVec * transTrace.fraction); 
	Translate( newOrigin - current.origin );


	// Find the top and bottom of the rope
	// This must be done every frame since the rope may be deforming
	ropeBounds = m_RopeEntity.GetEntity()->GetPhysics()->GetAbsBounds();
	ropeTop = ropeBounds[0] * -gravityNormal;
	ropeBot = ropeBounds[1] * -gravityNormal;

	if( ropeTop < ropeBot )
	{
		// switch 'em
		temp = ropeTop;
		ropeTop = ropeBot;
		ropeBot = temp;
	}

	// ============== read mouse input and orbit around the rope ===============

	// recalculate offset because the player may have moved to stick point
	offset = (ropePoint - current.origin);
	offset.ProjectOntoPlane( -gravityNormal );
	offset.Normalize();

	// forward vector orthogonal to gravity
	forward = viewForward - (gravityNormal * viewForward) * gravityNormal;
	forward.Normalize();

	deltaAng1 = offset * forward;
	deltaYaw = m_DeltaViewYaw;

	// use a different tolerance for rotating toward the rope vs away
	// rotate forward by deltaAng to see if we are rotating towards or away from the rope
	idRotation rotateView( vec3_origin, -gravityNormal, -deltaYaw );
	rotateView.RotatePoint( forward );

	deltaAng2 = offset * forward;

	
	// only rotate around the rope if looking at the rope to within some angular tolerance
	// always rotate if shifting view away
	if( deltaAng1 >= idMath::Cos( ROPE_ROTANG_TOL ) 
		|| ( (deltaAng2 < deltaAng1) && deltaAng1 > 0 ) )
	{

		newOrigin = current.origin;

		// define the counter-rotation around the rope point using gravity axis
		idRotation rotatePlayer( ropePoint, -gravityNormal, -deltaYaw );
		rotatePlayer.RotatePoint( newOrigin );

		// check whether the player will clip anything when orbiting
		transVec = newOrigin - current.origin;
		ClipTranslation(transTrace, transVec, NULL);
		newOrigin = current.origin + (transVec * transTrace.fraction); 

		Translate( newOrigin - current.origin );
	}
	

	// ================ read control input for climbing movement ===============

	upscale = (climbDir * viewForward - 0.5f) * 2.5f;
	if ( upscale > 1.0f ) 
	{
		upscale = 1.0f;
	}
	else if ( upscale < -1.0f ) 
	{
		upscale = -1.0f;
	}

	scale = idPhysics_Player::CmdScale( command );
	// up down movement
	
	wishvel = 0.9f * climbDir * upscale * scale * (float)command.forwardmove;

	/* greebo: Removed command.upmove portion from wishvel. I guess this was to support
	   the old crouching code which set command.upmove to negative values. This code
	   is no longer there so command.upmove is only non-zero during jumping, 
	   which we can ignore here.

	if ( command.upmove ) 
	{
		wishvel += 0.5f * climbDir * scale * (float) command.upmove;
	}*/

	// detach the player from the rope if they jump
	if ( idPhysics_Player::CheckRopeJump()) 
	{
		RopeDetach();
		goto Quit;
	}

	// if the player is above the top of the rope, don't climb up
	// if the player is at the bottom of the rope, don't climb down
	// subtract some amount to represent hanging on with arms above head
	if  ( 
			(
				wishvel * gravityNormal <= 0.0f 
				&& (((current.origin * -gravityNormal) + ROPE_GRABHEIGHT ) > ropeTop)
			)
			||
			(
				wishvel * gravityNormal >= 0.0f
				&& (((current.origin * -gravityNormal) + ROPE_GRABHEIGHT + 35.0f) < ropeBot)
			)
		)
	{
		current.velocity -= (current.velocity * climbDir) * climbDir;
		goto Quit;
	}

	// accelerate
	wishspeed = wishvel.Normalize();
	idPhysics_Player::Accelerate( wishvel, wishspeed, PM_ACCELERATE );

	// cap the climb velocity
	upscale = current.velocity * -climbDir;
	if ( upscale < -PM_ROPESPEED ) 
	{
		current.velocity += climbDir * (upscale + PM_ROPESPEED);
	}
	else if ( upscale > PM_ROPESPEED ) 
	{
		current.velocity += climbDir * (upscale - PM_ROPESPEED);
	}

	// stop the player from sliding when they let go of the button
	if ( (wishvel * gravityNormal) == 0.0f ) 
	{
		if ( current.velocity * gravityNormal < 0.0f ) 
		{
			current.velocity += gravityVector * frametime;
			if ( current.velocity * gravityNormal > 0.0f ) 
			{
				current.velocity -= (gravityNormal * current.velocity) * gravityNormal;
			}
		}
		else 
		{
			current.velocity -= gravityVector * frametime;
			if ( current.velocity * gravityNormal < 0.0f ) 
			{
				current.velocity -= (gravityNormal * current.velocity) * gravityNormal;
			}
		}
	}

	// If the player is climbing down and hits the ground, detach them from the rope
	if ( (wishvel * gravityNormal) > 0.0f && groundPlane )
	{
		RopeDetach();
		goto Quit;
	}
	
	// move the player velocity back into the world frame
	current.velocity += m_RefEntVelocity;

	// slide the player up and down with their calculated velocity
	idPhysics_Player::SlideMove( false, ( command.forwardmove > 0 ), false, false );

Quit:
	return;
}
#pragma warning( default : 4533 )

/*
============
idPhysics_Player::RopeDetach
============
*/
void idPhysics_Player::RopeDetach() 
{
	m_bOnRope = false;

	// start the reattach timer
	SetNextAttachTime(gameLocal.time + cv_tdm_reattach_delay.GetFloat());

	static_cast<idPlayer*>(self)->SetImmobilization( "RopeMove", 0 );

	// move the player velocity back into the world frame
	current.velocity += m_RefEntVelocity;

	// switch movement modes to the appropriate one
	if ( waterLevel > WATERLEVEL_FEET ) 
	{
		WaterMove();
	}
	else 
	{
		AirMove();
	}
}

/*
============
idPhysics_Player::ClimbDetach
============
*/
void idPhysics_Player::ClimbDetach( bool bStepUp ) 
{
	m_bOnClimb = false;
	m_ClimbingOnEnt = NULL;
	m_bClimbDetachThisFrame = true;

	static_cast<idPlayer *>(self)->SetImmobilization("ClimbMove", 0);

	current.velocity += m_RefEntVelocity;

	// switch movement modes to the appropriate one
	if( bStepUp )
	{
		// Step up at the top of a ladder
		idVec3 ClimbNormXY = m_vClimbNormal - (gravityNormal * m_vClimbNormal) * gravityNormal;
		ClimbNormXY.Normalize();
		current.velocity += -ClimbNormXY * LADDER_TOPVELOCITY;
		idPhysics_Player::SlideMove( false, true, false, true );
	}
	else if ( waterLevel > WATERLEVEL_FEET ) 
	{
		WaterMove();
	}
	else 
	{
		AirMove();
	}

	SetNextAttachTime(gameLocal.time + cv_tdm_reattach_delay.GetFloat());

}

/*
============
idPhysics_Player::LadderMove
============
*/
void idPhysics_Player::LadderMove( void ) 
{
	idVec3	wishdir( vec3_zero ), wishvel( vec3_zero ), right( vec3_zero );
	idVec3  dir( vec3_zero ), start( vec3_zero ), end( vec3_zero ), delta( vec3_zero );
	idVec3	AttachVel( vec3_zero ), RefFrameVel( vec3_zero );
	idVec3	vReqVert( vec3_zero ), vReqHoriz( vec3_zero ), vHorizVect( vec3_zero );
	float	wishspeed(0.0f), scale(0.0f), accel(PM_ACCELERATE);
	float	upscale(0.0f), horizscale(0.0f), NormalDot(0.0f);
	trace_t SurfTrace;
	bool	bMoveAllowed( true );


	// jump off the climbable surface if they jump, or fall off if they hit crouch
	// angua: detaching when hitting crouch is handled in idPlayer::PerformImpulse
	if (idPhysics_Player::CheckRopeJump()) 
	{
		ClimbDetach();
		return;
	}

	idVec3 ClimbNormXY = m_vClimbNormal - (m_vClimbNormal * gravityNormal) * gravityNormal;
	ClimbNormXY.Normalize();

	NormalDot = ClimbNormXY * viewForward;
	// detach if their feet are on the ground walking away from the surface
	if ( walking && -NormalDot * command.forwardmove < LADDER_WALKDETACH_DOT )
	{
		ClimbDetach();
		return;
	}


	// ====================== stick to the ladder ========================
	// Do a trace to figure out where to attach the player:
	start = current.origin;
	end = start - 48.0f * ClimbNormXY;
	gameLocal.clip.Translation( SurfTrace, start, end, clipModel, clipModel->GetAxis(), clipMask, self );

	// if there is a climbable surface in front of the player, stick to it

	idEntity* testEnt = gameLocal.entities[SurfTrace.c.entityNum]; // grayman #2787
	bool isVine = ( testEnt && testEnt->IsType( tdmVine::Type ) ); // grayman #2787
	if ( ( SurfTrace.fraction != 1.0f ) &&
		( ( SurfTrace.c.material && SurfTrace.c.material->IsLadder() ) || isVine ) ) // grayman #2787
	{
		// grayman #2787 - smooth out the end position if you struck a climbable vine piece.
		// This fixes the choppy sideways movement on a vine.

		idVec3 endPoint = SurfTrace.endpos;
		idVec3 p = endPoint;
		if ( isVine )
		{
			idVec3 vineOrigin = testEnt->GetPhysics()->GetOrigin();
			idVec3 vinePeak = vineOrigin + 3.875 * ClimbNormXY; // 3.875 is the distance from the vine origin to its peak
			float c = ( vinePeak - start ) * ClimbNormXY;
			float e = -ClimbNormXY * ClimbNormXY;
			idVec3 size = GetBounds().GetSize();
			p = start - ( c/e - size.x/2 )*ClimbNormXY;
		}

		m_vClimbPoint = p + cv_pm_climb_distance.GetFloat() * ClimbNormXY;
//		m_vClimbPoint = endPoint + cv_pm_climb_distance.GetFloat() * ClimbNormXY;
		AttachVel = 10 * (m_vClimbPoint - current.origin);

		// Now that we have a valid point, don't need to use the initial one
		m_bClimbInitialPhase = false;

		// Update sounds and movement speed caps for the surface if we change surfaces
		idStr surfName = g_Global.GetSurfName(SurfTrace.c.material);

		if (surfName != m_ClimbSurfName)
		{
			idStr LookUpName, TempStr;
			const idKeyValue *kv = NULL;
			
			m_ClimbSurfName = surfName;

			LookUpName = "climb_max_speed_vert_";
			TempStr = LookUpName + surfName;
			if( ( kv = self->spawnArgs.FindKey(LookUpName.c_str()) ) != NULL )
				m_ClimbMaxVelVert = atof( kv->GetValue().c_str() );
			else
			{
				TempStr = LookUpName + "default";
				m_ClimbMaxVelVert = self->spawnArgs.GetFloat( LookUpName.c_str(), "1.0" );
			}

			LookUpName = "climb_max_speed_horiz_";
			TempStr = LookUpName + surfName;
			if( ( kv = const_cast<idKeyValue *>( self->spawnArgs.FindKey(LookUpName.c_str())) ) != NULL )
				m_ClimbMaxVelHoriz = atof( kv->GetValue().c_str() );
			else
			{
				TempStr = LookUpName + "default";
				m_ClimbMaxVelHoriz = self->spawnArgs.GetFloat( LookUpName.c_str(), "2.3" );
			}

			// sound repitition distances
			LookUpName = "climb_snd_repdist_vert_";
			TempStr = LookUpName + surfName;
			if( ( kv = const_cast<idKeyValue *>( self->spawnArgs.FindKey(LookUpName.c_str())) ) != NULL )
				m_ClimbSndRepDistVert = atoi( kv->GetValue().c_str() );
			else
			{
				TempStr = LookUpName + "default";
				m_ClimbSndRepDistVert = self->spawnArgs.GetInt( LookUpName.c_str(), "32" );
			}

			// sound repitition distances
			LookUpName = "climb_snd_repdist_horiz_";
			TempStr = LookUpName + surfName;
			if( ( kv = const_cast<idKeyValue *>( self->spawnArgs.FindKey(LookUpName.c_str())) ) != NULL )
				m_ClimbSndRepDistHoriz = atoi( kv->GetValue().c_str() );
			else
			{
				TempStr = LookUpName + "default";
				m_ClimbSndRepDistHoriz = self->spawnArgs.GetInt( LookUpName.c_str(), "32" );
			}
		}
	}
	else if( m_bClimbInitialPhase )
	{
		// We should already have m_vClimbPoint stored from the initial trace
		AttachVel = 12.0f * (m_vClimbPoint - current.origin);
	}

	// stifu #4948: Do a ladder slide with non-damange terminal velocity
	if (m_bSlideOrDetachClimb)
	{
		idPhysics_Player::SlideMove(true, false, false, false, cv_pm_ladderSlide_speedLimit.GetFloat());
		return;
	}

	// TODO: Support non-rope climbable AFs by storing the AF body hit in the trace?
	SetRefEntVel(m_ClimbingOnEnt.GetEntity());

	// Move player into climbed on ent reference frame
	current.velocity -= m_RefEntVelocity;

	current.velocity = (gravityNormal * current.velocity) * gravityNormal + AttachVel;

	scale = idPhysics_Player::CmdScale( command );

	float lenVert = viewForward * -gravityNormal;
	float lenTransv = idMath::Fabs(viewForward * gravityNormal.Cross(ClimbNormXY));
	// Dump everything that's not in the transverse direction into the vertical direction
	float lenVert2 = idMath::Sqrt(NormalDot * NormalDot + lenVert * lenVert);

	// resolve up/down, with some tolerance so player can still go up looking slightly down
	if( lenVert < -0.3 )
		lenVert2 = -lenVert2;

	vReqVert = lenVert2 * -gravityNormal * scale * (float)command.forwardmove;
	vReqVert *= m_ClimbMaxVelVert;

	// obtain the horizontal direction
	vReqHoriz = viewForward - (ClimbNormXY * viewForward) * ClimbNormXY;
	vReqHoriz -= (vReqHoriz * gravityNormal) * gravityNormal;
	vReqHoriz.Normalize();
	vReqHoriz *= lenTransv * scale * (float)command.forwardmove;
	vReqHoriz *= m_ClimbMaxVelHoriz;


	// Pure horizontal motion if looking close enough to horizontal:
	if( lenTransv > 0.906 )
		wishvel = vReqHoriz;
	else
		wishvel = vReqVert + vReqHoriz;
	
	// strafe
	if ( command.rightmove ) 
	{
		// right vector orthogonal to gravity
		right = viewRight - (gravityNormal * viewRight) * gravityNormal;
		// project right vector into ladder plane
		right = right - (ClimbNormXY * right) * ClimbNormXY;
		right.Normalize();

		wishvel += m_ClimbMaxVelHoriz * right * scale * (float) command.rightmove;
	}

	// ========================== Surface Extent Test ======================
	// This now just checks distance from the last valid climbing point
	dir = wishvel;
	dir.Normalize();

	end = start + wishvel * frametime + dir * CLIMB_SURFCHECK_DELTA;
	delta = m_vClimbPoint - end;
	if( delta.LengthSqr() > LADDER_DISTAWAY * LADDER_DISTAWAY )
		bMoveAllowed = false;

	if( !bMoveAllowed )
	{
		// If we were trying to go up and reached the extent, attempt to step off the ladder
		// Make sure we are really trying to go up, not first going off to the side and then up
		// TODO: Tweak this delta.lengthsqr parameter of 25.0, only measure in the horizontal axis?
		delta = current.origin - m_vClimbPoint;
		delta -= (delta * gravityNormal) * gravityNormal;

		if( NormalDot < 0.0f && -wishvel * gravityNormal > 0 && delta.LengthSqr() < 25.0f )
		{
			ClimbDetach( true );
			return;
		}

		accel = idMath::INFINITY;
		wishvel = vec3_zero;
	}

	// ========================== End Surface Extent Test ==================

	// do strafe friction
	// idPhysics_Player::Friction();

	// accelerate
	wishspeed = wishvel.Normalize();
	idPhysics_Player::Accelerate( wishvel, wishspeed, accel );

	// don't apply friction on the wishdir direction
	idPhysics_Player::Friction( wishdir );

	// cap the vertical travel velocity
	upscale = current.velocity * -gravityNormal;
	if ( upscale < -m_ClimbMaxVelVert * playerSpeed )
		current.velocity += gravityNormal * (upscale + m_ClimbMaxVelVert * playerSpeed );
	else if ( upscale > m_ClimbMaxVelVert * playerSpeed  )
		current.velocity += gravityNormal * (upscale - m_ClimbMaxVelVert * playerSpeed );

	// cap the horizontal travel velocity
	vHorizVect = current.velocity - (current.velocity * gravityNormal) * gravityNormal;
	horizscale = vHorizVect.Normalize();
	float horizDelta = horizscale;
	horizscale = idMath::ClampFloat( -m_ClimbMaxVelHoriz * playerSpeed, m_ClimbMaxVelHoriz * playerSpeed, horizscale );
	horizDelta -= horizscale;
	current.velocity -= vHorizVect * horizDelta;
	
	if ( (wishvel * gravityNormal) == 0.0f ) 
	{
		if ( current.velocity * gravityNormal < 0.0f ) 
		{
			current.velocity += gravityVector * frametime;
			if ( current.velocity * gravityNormal > 0.0f ) 
			{
				current.velocity -= (gravityNormal * current.velocity) * gravityNormal;
			}
		}
		else {
			current.velocity -= gravityVector * frametime;
			if ( current.velocity * gravityNormal < 0.0f ) 
			{
				current.velocity -= (gravityNormal * current.velocity) * gravityNormal;
			}
		}
	}
	
	// Move player velocity back into the world frame
	current.velocity += m_RefEntVelocity;

	idPhysics_Player::SlideMove( false, ( command.forwardmove > 0 ), false, false );
}

/*
=============
idPhysics_Player::CorrectAllSolid
=============
*/
void idPhysics_Player::CorrectAllSolid( trace_t &trace, int contents ) {
	if ( debugLevel ) {
		gameLocal.Printf( "%i:allsolid\n", c_pmove );
	}

	// SophisticatedZombie
	//DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING ("performing CorrectAllSolid due to player inside solid object\n");
	//
	// Don't bump player up if they're standing in a previously picked up objects.
	// This is complicated but because we want free object movement, we have to temporarily disable player clipping.
	// But, if a players releases an object when they're inside it they float to the surface.  By doing this check
	// we can avoid that.
	// ishtvan: Only use the "rising code" if we're still clipping something after a mantle has finished
	if( m_mantlePhase == fixClipping_DarkModMantlePhase 
		&& !gameLocal.m_Grabber->HasClippedEntity() ) 
	{
		current.origin -= (GetGravityNormal() * 0.2f);
	}


	// FIXME: jitter around to find a free spot ?

	if ( trace.fraction >= 1.0f ) {
		memset( &trace, 0, sizeof( trace ) );
		trace.endpos = current.origin;
		trace.endAxis = clipModelAxis;
		trace.fraction = 0.0f;
		trace.c.dist = current.origin.z;
		trace.c.normal.Set( 0, 0, 1 );
		trace.c.point = current.origin;
		trace.c.entityNum = ENTITYNUM_WORLD;
		trace.c.id = 0;
		trace.c.type = CONTACT_TRMVERTEX;
		trace.c.material = NULL;
		trace.c.contents = contents;
	}
}

/*
=============
idPhysics_Player::CheckGround
=============
*/
void idPhysics_Player::CheckGround( void ) {
	int i, contents;
	bool hadGroundContacts;

	hadGroundContacts = HasGroundContacts();

	// set the clip model origin before getting the contacts
	clipModel->SetPosition( current.origin, clipModel->GetAxis() );

	EvaluateContacts();

	// setup a ground trace from the contacts
	groundTrace.endpos = current.origin;
	groundTrace.endAxis = clipModel->GetAxis();
	if ( contacts.Num() ) {
		groundTrace.fraction = 0.0f;
		groundTrace.c = contacts[0];
		for ( i = 1; i < contacts.Num(); i++ ) {
			groundTrace.c.normal += contacts[i].normal;
		}
		groundTrace.c.normal.Normalize();
	} else {
		groundTrace.fraction = 1.0f;
	}

	contents = gameLocal.clip.Contents( current.origin, clipModel, clipModel->GetAxis(), -1, self );
	if ( contents & MASK_SOLID ) 
	{
		// do something corrective if stuck in solid
		idPhysics_Player::CorrectAllSolid( groundTrace, contents );
	}
	//stgatilov: call hacky method CorrectAllSolid only once per mantle
	//otherwise, player can start levitating up in a really unlucky case
	/*else */if ( m_mantlePhase == fixClipping_DarkModMantlePhase )
	{
		// the mantle stage can advance to done if we're not currently clipping
		m_mantlePhase = notMantling_DarkModMantlePhase;
	}

	// if the trace didn't hit anything, we are in free fall
	if ( groundTrace.fraction == 1.0f ) 
	{
		groundPlane = false;
		walking = false;
		groundEntityPtr = NULL;
		return;
	}

	groundMaterial = groundTrace.c.material;

	// Store the ground entity
	idEntity* groundEnt = gameLocal.entities[ groundTrace.c.entityNum ];
	groundEntityPtr = groundEnt;

	// check if getting thrown off the ground
	if ( (current.velocity * -gravityNormal) > 0.0f && ( current.velocity * groundTrace.c.normal ) > 10.0f ) {
		if ( debugLevel ) {
			gameLocal.Printf( "%i:kickoff\n", c_pmove );
		}

		groundPlane = false;
		walking = false;
		return;
	}
	
	// grayman #2409 - apply velocity change due to friction loss on slick surfaces
	
	bool slick = (groundMaterial && (groundMaterial->IsSlick()));
	float walkNormal = MIN_WALK_NORMAL;
	if (slick)
	{
		idVec3 velocityChange = groundTrace.c.normal;
		velocityChange.z = 0; // no vertical component
		current.velocity += 3*velocityChange;
		walkNormal = MIN_WALK_SLICK_NORMAL;
	}
	
		// don't check for slopes for a short time (only used for high framerates)
	if (m_SlopeIgnoreTimer <= 0) {
		// slopes that are too steep will not be considered onground
		if ( ( groundTrace.c.normal * -gravityNormal ) < walkNormal ) // grayman #2409
		{
			if ( debugLevel ) {
				gameLocal.Printf( "%i:steep\n", c_pmove );
			}

		// FIXME: if they can't slide down the slope, let them walk (sharp crevices)
		
		// make sure we don't die from sliding down a steep slope
			if ( current.velocity * gravityNormal > 150.0f ) {
				current.velocity -= ( current.velocity * gravityNormal - 150.0f ) * gravityNormal;
			}

		groundPlane = true;
		walking = false;
		return;
	    }
	}

	groundPlane = true;
	walking = true;

	// hitting solid ground will end a waterjump
	if ( current.movementFlags & PMF_TIME_WATERJUMP ) {
		current.movementFlags &= ~( PMF_TIME_WATERJUMP | PMF_TIME_LAND );
		current.movementTime = 0;
	}

	// if the player didn't have ground contacts the previous frame
	if ( !hadGroundContacts ) {
		// greebo: Set the "jump" deadtime in any case to 100 msec to disallow jumps for small timespan
		current.movementTime = 100;

		// don't do landing time if we were just going down a slope
		if ( (current.velocity * -gravityNormal) < -200.0f ) {
			// don't allow another jump for a little while
			current.movementFlags |= PMF_TIME_LAND;
			current.movementTime = 100;
		}
	}

	// let the entity know about the collision
	self->Collide( groundTrace, current.velocity );

	groundEnt = groundEntityPtr.GetEntity();

	if ( groundTrace.c.entityNum != ENTITYNUM_WORLD && groundEnt != NULL )
	{
		idPhysics* groundPhysics = groundEnt->GetPhysics();

		impactInfo_t info;
		groundEnt->GetImpactInfo( self, groundTrace.c.id, groundTrace.c.point, &info );

		// greebo: Don't push entities that already have a velocity towards the ground.
		if (groundPhysics != NULL && info.invMass != 0.0f && 
			idMath::Fabs(groundPhysics->GetLinearVelocity()*gravityNormal) < VECTOR_EPSILON)
		{
			// greebo: Apply a force to the entity below the player
			//gameRenderWorld->DebugArrow(colorCyan, current.origin, current.origin + gravityNormal*20, 1, 16);
			groundPhysics->AddForce(0, current.origin, gravityNormal*mass*cv_pm_weightmod.GetFloat(), this);
		}
	}
}

/*
==============
idPhysics_Player::CheckDuck

Sets clip model size
==============
*/
void idPhysics_Player::CheckDuck( void ) {
	float maxZ;

	idPlayer* player = static_cast<idPlayer*>(self);
	int oldMovementFlags = current.movementFlags;

	bool idealCrouchState = player->GetIdealCrouchState();

	if ( current.movementType == PM_DEAD ) {
		maxZ = pm_deadheight.GetFloat();
	} else {
		// stand up when climbing a ladder or rope
		if (idealCrouchState == true && !m_bOnClimb && !m_bOnRope)
		{
			if (waterLevel >= WATERLEVEL_WAIST)
			{
				// greebo: We're waist-deep in water, trace down a few units to see if we're standing on ground
				trace_t	trace;
				idVec3 end = current.origin + gravityNormal * 20;
				gameLocal.clip.Translation( trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self );
				
				if (trace.fraction < 1.0f)
				{
					// We're not floating in deep water, we're standing in waist-deep water, duck as requested.
					current.movementFlags |= PMF_DUCKED;
				}
			}
			else
			{
				// We're outside of water, just duck as requested
				current.movementFlags |= PMF_DUCKED;
			}

			// greebo: Update the lean physics when crouching
			UpdateLeanPhysics();
		}
		else if (!IsMantling() && !IsShouldering() && idealCrouchState == false) // MantleMod: SophisticatedZombie (DH): Don't stand up if crouch during mantle
		{
			// ideal crouch state is not negative anymore, check if we are still in crouch mode
			// stand up if appropriate
			if ( current.movementFlags & PMF_DUCKED ) 
			{
				bool canStandUp = true;

				if (waterLevel >= WATERLEVEL_HEAD)
				{
					// greebo: We're still in water, check if there is ground below us so that we can stand up
					trace_t	trace;
					float viewHeightDelta = pm_normalviewheight.GetFloat() - pm_crouchviewheight.GetFloat();
					idVec3 end = current.origin + gravityNormal * viewHeightDelta;
					gameLocal.clip.Translation( trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self );

					if (trace.fraction == 1.0f)
					{
						canStandUp = false; // trace missed, no ground below
					}
					else 
					{
						// The ground trace hit a target, see if our eyes would emerge from the water
						idVec3 pointAboveHead = player->GetEyePosition() - gravityNormal * viewHeightDelta * (1.0f - trace.fraction);

						int contents = gameLocal.clip.Contents( pointAboveHead, NULL, mat3_identity, -1, self );
						if (contents & MASK_WATER)
						{
							canStandUp = false; // The point above our head is in water
						}
					}
				}

				if (canStandUp)
				{
					// greebo: We're not in deep water, so try to stand up
					idVec3 end = current.origin - ( pm_normalheight.GetFloat() - pm_crouchheight.GetFloat() ) * gravityNormal;

					// Now perform the upwards trace to check if we have room to stand up
					trace_t	trace;
					gameLocal.clip.Translation( trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self );
					if ( trace.fraction >= 1.0f ) {
						// We have room above us, so turn off the crouch flag
						current.movementFlags &= ~PMF_DUCKED;
					}
				}
			}
		}

		if ( current.movementFlags & PMF_DUCKED ) 
		{
			// #5961 - Don't use crouch speed while climbing
			if (!(OnRope() || OnLadder()))
			{
				playerSpeed = crouchSpeed;
			}
			maxZ = pm_crouchheight.GetFloat();
		}
		else 
		{
			maxZ = pm_normalheight.GetFloat();
		}

		if (waterLevel == WATERLEVEL_HEAD)
		{
			// greebo: We're underwater, set the clipmodel to the crouched size
			maxZ = pm_crouchheight.GetFloat();

			// greebo: But still let the player swim as fast as if he was uncrouched
			playerSpeed = walkSpeed;
		}

		// greebo: Check if we've submersed in a liquid. If yes: set the clipmodel to crouchheight
		// And set the DUCKED flag to 1.
		if (waterLevelChanged)
		{
			if (waterLevel == WATERLEVEL_HEAD)
			{
				// We've just submersed into water, set the model to ducked
				current.movementFlags |= PMF_DUCKED;

				// Translate the origin a bit upwards to prevent the player head from "jumping" downwards
				SetOrigin(player->GetEyePosition() + gravityNormal * pm_crouchviewheight.GetFloat());

				// Set the Eye height directly to the new value, to avoid the smoothing happening in idPlayer::Move()
				player->SetEyeHeight(pm_crouchviewheight.GetFloat());
			}
 			else if ((oldMovementFlags & PMF_DUCKED) && waterLevel == WATERLEVEL_WAIST && previousWaterLevel == WATERLEVEL_HEAD)
			{
				// angua: only stand up when we should not be crouching
				if (idealCrouchState == false)
				{
					// We're just floating up to the water surface, switch back to non-crouch mode
					// Clear the flag again, just to be sure
					current.movementFlags &= ~PMF_DUCKED;

					// greebo: Perform a trace to see how far we can move downwards
					trace_t	trace;
					idVec3 end = player->GetEyePosition() + gravityNormal * pm_normalviewheight.GetFloat();
					gameLocal.clip.Translation(trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self);

					// Set the origin to the end position of the trace
					SetOrigin(trace.endpos);

					maxZ = pm_normalheight.GetFloat();

					// Set the Eye height directly to the new value, to avoid the smoothing happening in idPlayer::Move()
					player->SetEyeHeight(pm_normalviewheight.GetFloat());
				}
			}
		}

		// greebo: Update the lean physics when not crouching
		UpdateLeanPhysics();
	}

	// if the clipModel height should change
	if ( clipModel->GetBounds()[1][2] != maxZ ) 
	{
		idBounds bounds = clipModel->GetBounds();
		bounds[1][2] = maxZ;
		if ( pm_usecylinder.GetBool() ) {
			clipModel->LoadModel( idTraceModel( bounds, 8 ) );
		} else {
			clipModel->LoadModel( idTraceModel( bounds ) );
		}
	}
}

/*
================
idPhysics_Player::CheckClimbable
DarkMod: Checks ropes, ladders and other climbables
================
*/
void idPhysics_Player::CheckClimbable( void ) 
{
	if( current.movementTime ) 
		return;

	// if on the ground moving backwards
	// TODO: This causes problems when rope-climbing down a steep incline
	if( walking && command.forwardmove <= 0 ) 
		return;

	// Don't attach to ropes or ladders in the middle of a mantle
	if ( IsMantling() )
		return;

	if ( m_bOnRope && m_bSlideOrDetachClimb )
		return;
	// stifu #4948: Continue checking when sliding vine and ladder. A non-damaging
	// slide speed is only achieved when the player is looking at the climb. 
	// Otherwise, player will be detached.

/*
	// Don't attach if we are holding an object in our hands
	if( gameLocal.m_Grabber->GetSelected() != NULL )
		goto Quit;
*/

	// forward vector orthogonal to gravity
	idVec3 forward = viewForward - (gravityNormal * viewForward) * gravityNormal;
	forward.Normalize();

	float tracedist;
	if ( walking || ( waterLevel >= WATERLEVEL_WAIST ) )
	{
		// don't want to get sucked towards the ladder when still walking or when climbing
		// nbohr1more; #625 prevent player from being sucked onto ladders from far away when underwater
		tracedist = 1.0f;
	} 
	else 
	{
		tracedist = 48.0f;
	}

	idVec3 end = current.origin + tracedist * forward;
	// modified to check contents_corpse to check for ropes
	trace_t		trace;
	gameLocal.clip.Translation( trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask | CONTENTS_CORPSE, self );

	idVec3 delta;
	float angleOff;
	float dist;
	// if near a surface
	if ( trace.fraction < 1.0f ) 
	{
		idEntity* testEnt = gameLocal.entities[trace.c.entityNum];
	
		// DarkMod: Check if we're looking at a rope and airborne
		// TODO: Check the class type instead of the stringname, make new rope class

		if ( testEnt && testEnt->m_bIsClimbableRope )
		{
			m_RopeEntTouched = static_cast<idAFEntity_Base *>(testEnt);

			delta = (trace.c.point - current.origin);
			delta = delta - (gravityNormal * delta) * gravityNormal;
			dist = delta.LengthFast();

			delta.Normalize();
			angleOff = delta * forward;

			// must be in the air to attach to the rope
			// this is kind've a hack, but the rope has a different attach distance than the ladder
			if( 
				!m_bOnRope
				&& ( (trace.endpos - current.origin).Length() <= 2.0f )
				&& !groundPlane
				&& angleOff >= idMath::Cos( ROPE_ATTACHANGLE )
				&& (testEnt != m_RopeEntity.GetEntity() || gameLocal.time > m_NextAttachTime)
			)
			{
				// make sure rope segment is not touching the ground
				int bodyID = m_RopeEntTouched.GetEntity()->BodyForClipModelId( trace.c.id );
				if( !static_cast<idPhysics_AF *>(m_RopeEntTouched.GetEntity()->GetPhysics())->HasGroundContactsAtJoint( bodyID ) )
				{
					m_bRopeContact = true;
					m_bJustHitRope = true;
					m_RopeEntity = static_cast<idAFEntity_Base *>(testEnt);

					return;
				}
			}
		}

		// if a climbable surface
		// grayman #2787 - add a test for a climbable vine.

		bool isVine = ( testEnt && testEnt->IsType( tdmVine::Type ) );
		if ( ( ( trace.c.material && ( trace.c.material->IsLadder() ) ) || isVine )
			&& 	( gameLocal.time > m_NextAttachTime ) )
		{
			m_bClimbableAhead = true;

			idVec3 vStickPoint = trace.endpos;
			// check a step height higher
			end = current.origin - gravityNormal * ( maxStepHeight * 0.75f );
			gameLocal.clip.Translation( trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self );
			idVec3 start = trace.endpos;
			end = start + tracedist * forward;
			gameLocal.clip.Translation( trace, start, end, clipModel, clipModel->GetAxis(), clipMask, self );

			// if also near a surface a step height higher
			if ( trace.fraction < 1.0f ) 
			{
				// if it also is a ladder surface
				// grayman #2787 - add a test for a climbable vine

				idEntity* testEntHigher = gameLocal.entities[trace.c.entityNum]; // grayman #2787
				bool isVineHigher = ( testEntHigher && testEntHigher->IsType( tdmVine::Type ) );
				if ( ( trace.c.material && trace.c.material->IsLadder() ) || isVineHigher ) // grayman #2787
				{
					m_vClimbNormal = trace.c.normal;
					if ( isVineHigher ) // grayman #2787 - if climbing a vine, flatten out the normal
					{
						m_vClimbNormal = testEntHigher->GetPhysics()->GetAxis().ToAngles().ToForward();
					}
					m_ClimbingOnEnt = testEntHigher;
					
					// Initial climbing attachment
					// FIX: Used to get stuck hovering in some cases, now there's an initial phase
					if ( !m_bOnClimb )
					{
						m_bClimbInitialPhase = true;
						m_vClimbPoint = vStickPoint;
						static_cast<idPlayer *>(self)->SetImmobilization( "ClimbMove", EIM_ATTACK );
					}

					m_bOnClimb = true;					

					return;
				}
			}
		}
	}


	if (!m_bClimbableAhead && m_bOnClimb && m_bSlideOrDetachClimb)
	{
		// Not facing towards climbable surface. Cancel slide.
		ClimbDetach();
		m_bSlideOrDetachClimb = false;
	}

	// Rope attachment failsafe: Check intersection with the rope as well
	if 
		( 
			!m_bOnRope 
			&& m_RopeEntTouched.GetEntity() != NULL
			&& m_RopeEntTouched.GetEntity()->GetPhysics()->GetAbsBounds().IntersectsBounds( self->GetPhysics()->GetAbsBounds() )
			&& !groundPlane
		)
	{
		// test distance against the nearest rope body
		int touchedBody = -1;
		idVec3 PlayerPoint = current.origin + -gravityNormal*ROPE_GRABHEIGHT;
		idVec3 RopeSegPoint = static_cast<idPhysics_AF *>(m_RopeEntTouched.GetEntity()->GetPhysics())->NearestBodyOrig( PlayerPoint, &touchedBody );

		delta = ( RopeSegPoint - PlayerPoint);
		delta = delta - (gravityNormal * delta) * gravityNormal;
		dist = delta.LengthFast();

		delta.Normalize();
		angleOff = delta * forward;

		// if the player is looking high up, override the angle check
		float lookUpAng = viewForward * -gravityNormal;
		// set lookup to true if the player is looking 60 deg up or more
		bool bLookingUp = lookUpAng >= idMath::Cos(idMath::PI/6);

		if
			(	
				dist <= ROPE_DISTANCE
				&& ( angleOff >= idMath::Cos( ROPE_ATTACHANGLE ) || bLookingUp )
				&& (m_RopeEntTouched.GetEntity() != m_RopeEntity.GetEntity() || gameLocal.time > m_NextAttachTime)
				&& !static_cast<idPhysics_AF *>(m_RopeEntTouched.GetEntity()->GetPhysics())->HasGroundContactsAtJoint( touchedBody )
			)
		{
				m_bRopeContact = true;
				m_bJustHitRope = true;
				m_RopeEntity = m_RopeEntTouched.GetEntity();
				return;
		}
	}
}

/*
=============
idPhysics_Player::CheckJump
=============
*/
bool idPhysics_Player::CheckJump()
{
	if ( command.upmove < 10 )
	{
		// not holding jump
		return false;
	}

	// must wait for jump to be released or dead time to have passed
	if ( current.movementFlags & PMF_JUMP_HELD || current.movementTime > 0)
	{
		return false;
	}

	groundPlane = false;		// jumping away
	walking = false;
	current.movementFlags |= PMF_JUMP_HELD | PMF_JUMPED;

	idVec3 extraSpeedForward = viewForward - (gravityNormal * viewForward) * gravityNormal;
	extraSpeedForward.Normalize();

	// back paddling ?
	if ( command.forwardmove < 0 )
	{
		// greebo: Apply a modifier when doing backwards jumps
		extraSpeedForward = -extraSpeedForward * cv_tdm_backwards_jump_modifier.GetFloat();
	}

	// strafing right?
	if ( command.rightmove > 0)
	{
		extraSpeedForward = viewRight - (gravityNormal * viewRight) * gravityNormal;
		extraSpeedForward.Normalize();
	}
	// strafing left?
	else if ( command.rightmove < 0 )
	{
		extraSpeedForward = viewRight - (gravityNormal * viewRight) * gravityNormal;
		extraSpeedForward = -extraSpeedForward;
		extraSpeedForward.Normalize();
	}

	idVec3 addVelocity;
	float curVelocity = current.velocity.LengthFast();

	// are we walking?
	if ( curVelocity >= cv_tdm_min_vel_jump.GetFloat() && 
		curVelocity >= pm_walkspeed.GetFloat() && 
		curVelocity < ( pm_walkspeed.GetFloat() * cv_pm_runmod.GetFloat() ) ) 
	{	
		addVelocity = cv_tdm_walk_jump_vel.GetFloat() * maxJumpHeight * -gravityVector;
		extraSpeedForward *= cv_tdm_fwd_jump_vel.GetFloat();
	}
	// running ?
	else if ( curVelocity >= cv_tdm_min_vel_jump.GetFloat()  
		&& curVelocity >= cv_pm_runmod.GetFloat())
	{
		addVelocity = cv_tdm_run_jump_vel.GetFloat() * maxJumpHeight * -gravityVector;
		extraSpeedForward *= cv_tdm_fwd_jump_vel.GetFloat();
	}
	// stationary
	else
	{
		addVelocity = 2.0f * maxJumpHeight * -gravityVector;
	}

	// angua: reduce jump velocity when crouching, unless we are on a ladder or rope
	// cv_tdm_crouch_jump_vel can also be set to 0 to disable jumping while crouching
	if ( current.movementFlags & PMF_DUCKED && !OnRope() && !OnLadder())
	{
		addVelocity *= cv_tdm_crouch_jump_vel.GetFloat();
		extraSpeedForward *= cv_tdm_crouch_jump_vel.GetFloat();
	}

	if (addVelocity.LengthFast() > 0)
	{
		addVelocity *= idMath::Sqrt( addVelocity.Normalize() );
	}

	// greebo: Consider jump stamina
	float jumpStaminaFactor = 1.0f;

	if (lastJumpTime > -1)
	{
		int timeSinceLastJump = gameLocal.time - lastJumpTime;

		float jumpRelaxationTime = SEC2MS(cv_tdm_jump_relaxation_time.GetFloat());

		float factor = timeSinceLastJump * timeSinceLastJump / (jumpRelaxationTime * jumpRelaxationTime);

		jumpStaminaFactor = idMath::ClampFloat(0.2f, 1, factor);
	}

	current.velocity += addVelocity + extraSpeedForward * jumpStaminaFactor;

	// Remember the last jump time
	if (jumpStaminaFactor > 0.1f)
	{
		lastJumpTime = gameLocal.time;
	}

	return true;
}

/*
=============
idPhysics_Player::CheckRopeJump
=============
*/
bool idPhysics_Player::CheckRopeJump( void ) 
{
	if ( command.upmove < 10 ) {
		// not holding jump
		return false;
	}

	// must wait for jump to be released
	if ( current.movementFlags & PMF_JUMP_HELD ) {
		return false;
	}

	// angua: should be able to jump on a rope or ladder, even when crouched
	// don't jump if we can't stand up
	// if ( current.movementFlags & PMF_DUCKED ) {
	// 	return false;
	// }

	groundPlane = false;		// jumping away
	walking = false;
	current.movementFlags |= PMF_JUMP_HELD | PMF_JUMPED;

	// the jump direction is an equal sum of up and the direction we're looking
	idVec3 jumpDir = viewForward - gravityNormal;
	jumpDir *= 1.0f/idMath::Sqrt(2.0f);

// TODO: Make this an adjustable cvar, currently too high?
	idVec3 addVelocity = 2.0f * maxJumpHeight * gravityVector.Length() * jumpDir;
	addVelocity *= idMath::Sqrt( addVelocity.Normalize() );

	current.velocity += addVelocity;

	return true;
}

/*
=============
idPhysics_Player::CheckWaterJump

REMOVED from DarkMod
=============
*/

/*

=============

idPhysics_Player::SetWaterLevel

For MOD_WATERPHYSICS this is moved to Physics_Actor.cpp

=============

*/

#ifndef MOD_WATERPHYSICS

void idPhysics_Player::SetWaterLevel( void ) {
	idVec3		point;
	idBounds	bounds;
	int			contents;

	//
	// get waterlevel, accounting for ducking
	//
	waterLevel = WATERLEVEL_NONE;
	waterType = 0;

	bounds = clipModel->GetBounds();

	// check at feet level
	point = current.origin - ( bounds[0][2] + 1.0f ) * gravityNormal;
	contents = gameLocal.clip.Contents( point, NULL, mat3_identity, -1, self );
	if ( contents & MASK_WATER ) {

		waterType = contents;
		waterLevel = WATERLEVEL_FEET;

		// check at waist level
		point = current.origin - ( bounds[1][2] - bounds[0][2] ) * 0.5f * gravityNormal;
		contents = gameLocal.clip.Contents( point, NULL, mat3_identity, -1, self );
		if ( contents & MASK_WATER ) {

			waterLevel = WATERLEVEL_WAIST;

			// check at head level
			point = current.origin - ( bounds[1][2] - 1.0f ) * gravityNormal;
			contents = gameLocal.clip.Contents( point, NULL, mat3_identity, -1, self );
			if ( contents & MASK_WATER ) {
				waterLevel = WATERLEVEL_HEAD;
			}
		}
	}
}
#endif

/*
================
idPhysics_Player::DropTimers
================
*/
void idPhysics_Player::DropTimers( void ) {
	// drop misc timing counter
	if ( current.movementTime ) {
		if ( framemsec >= current.movementTime ) {
			current.movementFlags &= ~PMF_ALL_TIMES;
			current.movementTime = 0;
		}
		else {
			current.movementTime -= framemsec;
		}
	}
}

/*
================
idPhysics_Player::MovePlayer
================
*/
void idPhysics_Player::MovePlayer( int msec ) {
	// determine the time
	framemsec = msec;
	frametime = framemsec * 0.001f;

	// this counter lets us debug movement problems with a journal
	// by setting a conditional breakpoint for the previous frame
	c_pmove++;

	walking = false;
	groundPlane = false;
	
	m_bRopeContact = false;
	m_bClimbableAhead = false;
	m_bClimbDetachThisFrame = false;
	m_bMidAir = false;

	// default speed
	playerSpeed = walkSpeed;

	// remove jumped and stepped up flag
	current.movementFlags &= ~(PMF_JUMPED|PMF_STEPPED_UP|PMF_STEPPED_DOWN);
	current.stepUp = 0.0f;
	
	// update the slope ignore timer
	if (m_SlopeIgnoreTimer > 0)
	m_SlopeIgnoreTimer -= framemsec;

	if ( command.upmove < 10 ) {
		// not holding jump
		current.movementFlags &= ~PMF_JUMP_HELD;

		if (m_mantlePhase == notMantling_DarkModMantlePhase
			|| m_mantlePhase == fixClipping_DarkModMantlePhase)
		{
			// greebo: Jump button is released and no mantle phase is active, 
			// we can allow the next mantling process.
			m_mantleStartPossible = true;
		}
	}

	// if no movement at all
	if ( current.movementType == PM_FREEZE ) {
		return;
	}

	// move the player velocity into the frame of a pusher
	current.velocity -= current.pushVelocity;

	// view vectors
	viewAngles.ToVectors( &viewForward, NULL, NULL );
	viewForward *= clipModelAxis;
	viewRight = gravityNormal.Cross( viewForward );
	viewRight.Normalize();

	// fly in spectator mode
	if ( current.movementType == PM_SPECTATOR ) {
		SpectatorMove();
		idPhysics_Player::DropTimers();
		return;
	}

	// special no clip mode
	if ( current.movementType == PM_NOCLIP ) {
		idPhysics_Player::NoclipMove();
		idPhysics_Player::DropTimers();
		return;
	}

	// no control when dead
	if ( current.movementType == PM_DEAD ) {
		command.forwardmove = 0;
		command.rightmove = 0;
		command.upmove = 0;
	}

	// set watertype and waterlevel
	idPhysics_Player::SetWaterLevel(true); // greebo: Update the previousWaterLevel here

	// check for ground
	idPhysics_Player::CheckGround();

	// check if a ladder or a rope is straight ahead
	idPhysics_Player::CheckClimbable();

	// set clip model size
	idPhysics_Player::CheckDuck();

	// handle timers
	idPhysics_Player::DropTimers();

	// Mantle Mod: SophisticatdZombie (DH)
	idPhysics_Player::UpdateMantleTimers();

	// Lean Mod: Zaccheus and SophisticatedZombie (DH)
	idPhysics_Player::LeanMove();

	// Check if holding down jump
	if (CheckJumpHeldDown())
	{
		PerformMantle();
	}

	// move
	if ( current.movementType == PM_DEAD ) {
		// dead
		DeadMove();
	}
	else if (m_eShoulderAnimState == eShoulderingAnimation_Active)
	{
		// Shouldering viewport animation
		ShoulderingMove();
	}
	// continue moving on the rope if still attached
	else if ( m_bOnRope )
	{
		// Check rope movement and let go if the rope is moving too fast
		float maxRopeVelocity = cv_pm_rope_velocity_letgo.GetFloat();
		idEntity* ropeEntity = m_RopeEntity.GetEntity();
		if (ropeEntity != NULL)
		{

			float ropeVelocity = ropeEntity->/*GetBindMaster()->*/GetPhysics()->GetLinearVelocity().z;

			//gameLocal.Printf("Rope Velocity: %f\n", m_RopeEntity.GetEntity()->GetBindMaster()->GetPhysics()->GetLinearVelocity().LengthFast());
			if (ropeVelocity > maxRopeVelocity)
			{
				gameLocal.Printf("Rope is too fast (%f)! Letting go...\n", ropeVelocity);
				RopeDetach();
			}
		}

		RopeMove();
	}
	// Mantle MOD
	// SophisticatedZombie (DH)
	// greebo: Do the MantleMove before checking the rope contacts
	else if ( !(m_mantlePhase == notMantling_DarkModMantlePhase || m_mantlePhase == fixClipping_DarkModMantlePhase) ) 
	{
		MantleMove();
	}
	else if ( m_bRopeContact ) 
	{
		// toggle m_bOnRope
		m_bOnRope = true;

		// lower weapon
		static_cast<idPlayer*>(self)->SetImmobilization( "RopeMove", EIM_ATTACK | EIM_ITEM_DROP );

		RopeMove();
	}
	else if ( m_bOnClimb ) 
	{
		// going up or down a ladder
		LadderMove();
	}
	else if ( waterLevel > WATERLEVEL_FEET )
	{
		// swimming
		WaterMove();
	}
	else if ( walking ) {
		// walking on ground
		WalkMove();
	}
	else {
		// airborne
		AirMove();
	}

	if (waterLevel <= WATERLEVEL_FEET && m_fSwimTimeStart_s != 0.0f)
		// Reset swimming animation timer
		m_fSwimTimeStart_s = 0.0f;

	if (m_eShoulderAnimState == eShoulderingAnimation_Scheduled)
		// Try to start shouldering animation
		StartShoulderingAnim();

	// enable weapon if not swimming
	if ( ( waterLevel <= WATERLEVEL_FEET ) && static_cast<idPlayer*>(self)->GetImmobilization("WaterMove") && walking ) // grayman #3413
	{
		static_cast<idPlayer*>(self)->SetImmobilization("WaterMove", 0);
	}

	// set watertype, waterlevel and groundentity
	SetWaterLevel(false); // greebo: Don't update the previousWaterLevel this time
	CheckGround();

	// move the player velocity back into the world frame
	current.velocity += current.pushVelocity;
	current.pushVelocity.Zero();

	// DEBUG
	/*
	gameRenderWorld->DebugBounds
	(
		idVec4 (1.0, 0.0, 1.0, 1.0), 
		clipModel->GetAbsBounds(),
		idVec3 (0.0, 0.0, 0.0)
	);
	*/

	m_lastCommandViewYaw = command.angles[1];
	m_lastCommandViewPitch = command.angles[0];

}

#ifndef MOD_WATERPHYSICS

/*
================
idPhysics_Player::GetWaterLevel

For MOD_WATERPHYSICS this is moved to Physics_Actor.cpp

================
*/
waterLevel_t idPhysics_Player::GetWaterLevel( void ) const {
	return waterLevel;
}

/*
================
idPhysics_Player::GetWaterType

For MOD_WATERPHYSICS this is moved to Physics_Actor.cpp

================
*/
int idPhysics_Player::GetWaterType( void ) const {
	return waterType;
}

#endif


/*
================
idPhysics_Player::HasJumped
================
*/
bool idPhysics_Player::HasJumped( void ) const {
   	return ( ( current.movementFlags & PMF_JUMPED ) != 0 && current.movementTime <= 0);
}

/*
================
idPhysics_Player::HasSteppedUp
================
*/
bool idPhysics_Player::HasSteppedUp( void ) const {
	return ( ( current.movementFlags & ( PMF_STEPPED_UP | PMF_STEPPED_DOWN ) ) != 0 );
}

/*
================
idPhysics_Player::GetStepUp
================
*/
float idPhysics_Player::GetStepUp( void ) const {
	return current.stepUp;
}

/*
================
idPhysics_Player::IsCrouching
================
*/
bool idPhysics_Player::IsCrouching( void ) const {
	return ( ( current.movementFlags & PMF_DUCKED ) != 0 );
}

/*
================
idPhysics_Player::IsHangMantle
================
*/
bool idPhysics_Player::IsHangMantle( void ) const {
	return m_mantlePhase == hang_DarkModMantlePhase;
}

/*
================
idPhysics_Player::IsPullMantle
================
*/
bool idPhysics_Player::IsPullMantle( void ) const {
	return m_mantlePhase == pull_DarkModMantlePhase;
}

/*
================
idPhysics_Player::GetLastJumpTime
================
*/
int	idPhysics_Player::GetLastJumpTime() const {
	return lastJumpTime;
}

idEntity* idPhysics_Player::GetRopeEntity()
{
	return (m_bOnRope) ? m_RopeEntity.GetEntity() : NULL;
}

/*
================
idPhysics_Player::OnRope
================
*/
bool idPhysics_Player::OnRope( void ) const 
{
	return m_bOnRope;
}


/*
================
idPhysics_Player::OnLadder
================
*/
bool idPhysics_Player::OnLadder( void ) const {
	return m_bOnClimb;
}

/*
================
idPhysics_Player::idPhysics_Player
================
*/
idPhysics_Player::idPhysics_Player( void ) 
	: m_eShoulderAnimState(eShoulderingAnimation_NotStarted)
	, m_fShoulderingTime(0.0f)
    , m_bShouldering_SkipDucking(false)
	, m_fShouldering_TimeToNextSound(0.0f)
	, m_bMidAir(false)
	, m_fPrevShoulderingPitchOffset(0.0f)
	, m_PrevShoulderingPosOffset(vec3_zero)
	, m_ShoulderingStartPosRelative(vec3_zero)
	, m_ShoulderingCurrentPosRelative(vec3_zero)
	, m_pShoulderingGroundEntity(NULL)
	, m_fSwimTimeStart_s(0.0f)
	, m_fSwimLeadInDuration_s(-1.0f)
	, m_fSwimLeadOutStart_s(-1.0f)
	, m_fSwimLeadOutDuration_s(-1.0f)
	, m_fSwimSpeedModCompensation(-1.0f)
	, m_bSwimSoundStarted(false)
	, m_mantlePullStartPos(vec3_zero)
	, m_mantlePullEndPos(vec3_zero)
	, m_mantlePushEndPos(vec3_zero)
	, m_mantleCancelStartRoll(0.0f)
	, m_fmantleCancelDist(0.0f)
	, m_mantleCancelStartPos(vec3_zero)
	, m_mantleCancelEndPos(vec3_zero)
	, m_mantleStartPosWorld(vec3_zero)
{
	debugLevel = false;
	clipModel = NULL;
	clipMask = 0;
	memset( &current, 0, sizeof( current ) );
	saved = current;
	walkSpeed = 0;
	crouchSpeed = 0;
	maxStepHeight = 0;
	maxJumpHeight = 0;
	m_AirAccelerate = 1.0f;
	memset( &command, 0, sizeof( command ) );

	lastJumpTime = -1;

	viewAngles.Zero();
	framemsec = 0;
	frametime = 0;
	playerSpeed = 0;
	viewForward.Zero();
	viewRight.Zero();
	walking = false;
	groundPlane = false;
	memset( &groundTrace, 0, sizeof( groundTrace ) );
	groundMaterial = NULL;

	m_RefEntVelocity.Zero();
	
	// rope climbing
	m_bRopeContact = false;
	m_bOnRope = false;
	m_bJustHitRope = false;
	m_RopeEntity = NULL;
	m_RopeEntTouched = NULL;
	m_RopeKickTime = 0;

	// wall/ladder climbing
	m_bClimbableAhead = false;
	m_bOnClimb = false;
	m_bClimbDetachThisFrame = false;
	m_bClimbInitialPhase = false;
	m_vClimbNormal.Zero();
	m_vClimbPoint.Zero();
	m_ClimbingOnEnt = NULL;
	m_ClimbSurfName.Clear();
	m_ClimbMaxVelHoriz = 0.0f;
	m_ClimbMaxVelVert = 0.0f;
	m_ClimbSndRepDistVert = 0;
	m_ClimbSndRepDistHoriz = 0;
	m_bSlideOrDetachClimb = false;
	m_bSlideInitialized = false;

	m_NextAttachTime = -1;

	m_PushForce = CForcePushPtr(new CForcePush);
	
	m_SlopeIgnoreTimer = 0;

	// swimming
	waterLevel = WATERLEVEL_NONE;
	waterType = 0;

	// Mantle Mod
	m_mantlePhase = notMantling_DarkModMantlePhase;
	m_mantleTime = 0.0;
	m_p_mantledEntity = NULL;
	m_mantledEntityID = 0;
	m_jumpHeldDownTime = 0.0;
	m_mantleStartPossible = true;

	// Leaning Mod
	m_leanYawAngleDegrees = 0.0;
	m_CurrentLeanTiltDegrees = 0.0;
	m_b_leanFinished = true;
	m_leanTime = 0.0f;
	m_leanMoveStartTilt = 0.0;
	m_leanMoveEndTilt = 0.0;
	m_leanMoveMaxAngle = 0.0;

	m_lastCommandViewYaw = 0;
	m_lastCommandViewPitch = 0;
	m_viewLeanAngles = ang_zero;
	m_viewLeanTranslation = vec3_zero;

	m_LeanListenPos = vec3_zero;
	m_LeanEnt = NULL;

	m_DeltaViewYaw = 0.0;
	m_DeltaViewPitch = 0.0;

	// Initialize lean view bounds used for collision
	m_LeanViewBounds.Zero();
	m_LeanViewBounds.ExpandSelf( 4.0f );
	// bounds extend downwards so that player can't lean over very high ledges
	idVec3 lowerPoint(0,0,-15.0f);
	m_LeanViewBounds.AddPoint( lowerPoint );
}

/*
================
idPhysics_Player_SavePState
================
*/
void idPhysics_Player_SavePState( idSaveGame *savefile, const playerPState_t &state ) {
	savefile->WriteVec3( state.origin );
	savefile->WriteVec3( state.velocity );
	savefile->WriteVec3( state.localOrigin );
	savefile->WriteVec3( state.pushVelocity );
	savefile->WriteFloat( state.stepUp );
	savefile->WriteInt( state.movementType );
	savefile->WriteInt( state.movementFlags );
	savefile->WriteInt( state.movementTime );
}

/*
================
idPhysics_Player_RestorePState
================
*/
void idPhysics_Player_RestorePState( idRestoreGame *savefile, playerPState_t &state ) {
	savefile->ReadVec3( state.origin );
	savefile->ReadVec3( state.velocity );
	savefile->ReadVec3( state.localOrigin );
	savefile->ReadVec3( state.pushVelocity );
	savefile->ReadFloat( state.stepUp );
	savefile->ReadInt( state.movementType );
	savefile->ReadInt( state.movementFlags );
	savefile->ReadInt( state.movementTime );
}

/*
================
idPhysics_Player::Save
================
*/
void idPhysics_Player::Save( idSaveGame *savefile ) const {

	idPhysics_Player_SavePState( savefile, current );
	idPhysics_Player_SavePState( savefile, saved );

	savefile->WriteFloat( walkSpeed );
	savefile->WriteFloat( crouchSpeed );
	savefile->WriteFloat( maxStepHeight );
	savefile->WriteFloat( maxJumpHeight );
	savefile->WriteFloat( m_AirAccelerate );
	savefile->WriteInt( debugLevel );
	savefile->WriteInt(lastJumpTime);

	savefile->WriteUsercmd( command );
	savefile->WriteAngles( viewAngles );

	savefile->WriteInt( framemsec );
	savefile->WriteFloat( frametime );
	savefile->WriteFloat( playerSpeed );
	savefile->WriteVec3( viewForward );
	savefile->WriteVec3( viewRight );

	savefile->WriteBool( walking );
	savefile->WriteBool( groundPlane );
	savefile->WriteTrace( groundTrace );
	savefile->WriteMaterial( groundMaterial );

	savefile->WriteVec3( m_RefEntVelocity );

	savefile->WriteBool( m_bRopeContact );
	savefile->WriteBool( m_bJustHitRope );
	savefile->WriteBool( m_bOnRope );
	savefile->WriteInt( m_RopeKickTime );
	m_RopeEntity.Save( savefile );
	m_RopeEntTouched.Save( savefile );

	savefile->WriteBool( m_bClimbableAhead );
	savefile->WriteBool( m_bOnClimb );
	savefile->WriteBool( m_bClimbDetachThisFrame );
	savefile->WriteBool( m_bClimbInitialPhase );
	savefile->WriteVec3( m_vClimbNormal );
	savefile->WriteVec3( m_vClimbPoint );
	savefile->WriteString( m_ClimbSurfName.c_str() );
	savefile->WriteFloat( m_ClimbMaxVelHoriz );
	savefile->WriteFloat( m_ClimbMaxVelVert );
	savefile->WriteInt( m_ClimbSndRepDistVert );
	savefile->WriteInt( m_ClimbSndRepDistHoriz );
	m_ClimbingOnEnt.Save( savefile );

	savefile->WriteInt( m_NextAttachTime );

	savefile->WriteInt( (int)waterLevel );
	savefile->WriteInt( waterType );

	// Mantle mod
	savefile->WriteInt(m_mantlePhase);
	savefile->WriteBool(m_mantleStartPossible);
	savefile->WriteVec3(m_mantlePullStartPos);
	savefile->WriteVec3(m_mantlePullEndPos);
	savefile->WriteVec3(m_mantlePushEndPos);
	savefile->WriteObject(m_p_mantledEntity);
	savefile->WriteInt(m_mantledEntityID);
	savefile->WriteFloat(m_mantleTime);
	savefile->WriteFloat(m_jumpHeldDownTime);

	// Mantle cancel animation
	savefile->WriteFloat(m_mantleCancelStartRoll);
	savefile->WriteFloat(m_fmantleCancelDist);
	savefile->WriteVec3(m_mantleCancelStartPos);
	savefile->WriteVec3(m_mantleCancelEndPos);
	savefile->WriteVec3(m_mantleStartPosWorld);
	
	// Lean mod
	savefile->WriteFloat (m_leanYawAngleDegrees);
	savefile->WriteFloat (m_CurrentLeanTiltDegrees);
	savefile->WriteFloat (m_leanMoveStartTilt);
	savefile->WriteFloat (m_leanMoveEndTilt);
	savefile->WriteFloat (m_leanMoveMaxAngle);
	savefile->WriteBool (m_b_leanFinished);
	savefile->WriteFloat (m_leanTime);
	savefile->WriteAngles (m_lastPlayerViewAngles);
	savefile->WriteAngles (m_viewLeanAngles);
	savefile->WriteVec3 (m_viewLeanTranslation);
	savefile->WriteVec3 (m_LeanListenPos);
	m_LeanEnt.Save( savefile );

	savefile->WriteStaticObject(*m_PushForce);

	savefile->WriteBool(m_bSlideInitialized);

	// Shouldering anim
	savefile->WriteInt(m_eShoulderAnimState);
	savefile->WriteFloat(m_fShoulderingTime);
	savefile->WriteVec3(m_PrevShoulderingPosOffset);
	savefile->WriteVec3(m_ShoulderingStartPosRelative);
	savefile->WriteVec3(m_ShoulderingCurrentPosRelative);
	savefile->WriteObject(m_pShoulderingGroundEntity);
	savefile->WriteBool(m_bShouldering_SkipDucking);
	savefile->WriteFloat(m_fShouldering_TimeToNextSound);
	savefile->WriteFloat(m_fPrevShoulderingPitchOffset);
	savefile->WriteBool(m_bMidAir);

	// Swimming
	savefile->WriteFloat(m_fSwimTimeStart_s);
	savefile->WriteBool(m_bSwimSoundStarted);
}

/*
================
idPhysics_Player::Restore
================
*/
void idPhysics_Player::Restore( idRestoreGame *savefile ) {

	idPhysics_Player_RestorePState( savefile, current );
	idPhysics_Player_RestorePState( savefile, saved );

	savefile->ReadFloat( walkSpeed );
	savefile->ReadFloat( crouchSpeed );
	savefile->ReadFloat( maxStepHeight );
	savefile->ReadFloat( maxJumpHeight );
	savefile->ReadFloat( m_AirAccelerate );
	savefile->ReadInt( debugLevel );
	savefile->ReadInt(lastJumpTime);

	savefile->ReadUsercmd( command );
	savefile->ReadAngles( viewAngles );

	savefile->ReadInt( framemsec );
	savefile->ReadFloat( frametime );
	savefile->ReadFloat( playerSpeed );
	savefile->ReadVec3( viewForward );
	savefile->ReadVec3( viewRight );

	savefile->ReadBool( walking );
	savefile->ReadBool( groundPlane );
	savefile->ReadTrace( groundTrace );
	savefile->ReadMaterial( groundMaterial );

	savefile->ReadVec3( m_RefEntVelocity );

	savefile->ReadBool( m_bRopeContact );
	savefile->ReadBool( m_bJustHitRope );
	savefile->ReadBool( m_bOnRope );
	savefile->ReadInt( m_RopeKickTime );
	m_RopeEntity.Restore( savefile );
	m_RopeEntTouched.Restore( savefile );
	// Angle storage vars need to be reset on a restore, since D3 resets the command angle to 0
	m_lastCommandViewYaw = 0.0f;
	m_lastCommandViewPitch = 0.0f;
	m_DeltaViewYaw = 0.0f;
	m_DeltaViewPitch = 0.0f;

	savefile->ReadBool( m_bClimbableAhead );
	savefile->ReadBool( m_bOnClimb );
	savefile->ReadBool( m_bClimbDetachThisFrame );
	savefile->ReadBool( m_bClimbInitialPhase );
	savefile->ReadVec3( m_vClimbNormal );
	savefile->ReadVec3( m_vClimbPoint );
	savefile->ReadString( m_ClimbSurfName );
	savefile->ReadFloat( m_ClimbMaxVelHoriz );
	savefile->ReadFloat( m_ClimbMaxVelVert );
	savefile->ReadInt( m_ClimbSndRepDistVert );
	savefile->ReadInt( m_ClimbSndRepDistHoriz );
	m_ClimbingOnEnt.Restore( savefile );

	savefile->ReadInt( m_NextAttachTime );
	
	m_SlopeIgnoreTimer = 0;

	savefile->ReadInt( (int &)waterLevel );
	savefile->ReadInt( waterType );

	// Mantle mod
	{
		int temp;
		savefile->ReadInt(temp);
		assert(temp >= 0 && temp < NumMantlePhases); // sanity check
		m_mantlePhase = static_cast<EMantlePhase>(temp);
	}

	savefile->ReadBool(m_mantleStartPossible);
	savefile->ReadVec3(m_mantlePullStartPos);
	savefile->ReadVec3(m_mantlePullEndPos);
	savefile->ReadVec3(m_mantlePushEndPos);
	savefile->ReadObject(reinterpret_cast<idClass*&>(m_p_mantledEntity));
	savefile->ReadInt(m_mantledEntityID);
	savefile->ReadFloat(m_mantleTime);
	savefile->ReadFloat(m_jumpHeldDownTime);

	// Mantle Cancel animation
	savefile->ReadFloat(m_mantleCancelStartRoll);
	savefile->ReadFloat(m_fmantleCancelDist);
	savefile->ReadVec3(m_mantleCancelStartPos);
	savefile->ReadVec3(m_mantleCancelEndPos);
	savefile->ReadVec3(m_mantleStartPosWorld);

	// Lean mod
	savefile->ReadFloat (m_leanYawAngleDegrees);
	savefile->ReadFloat (m_CurrentLeanTiltDegrees);
	savefile->ReadFloat (m_leanMoveStartTilt);
	savefile->ReadFloat (m_leanMoveEndTilt);
	savefile->ReadFloat (m_leanMoveMaxAngle);
	savefile->ReadBool (m_b_leanFinished);
	savefile->ReadFloat (m_leanTime);
	savefile->ReadAngles (m_lastPlayerViewAngles);
	savefile->ReadAngles (m_viewLeanAngles);
	savefile->ReadVec3 (m_viewLeanTranslation);
	savefile->ReadVec3 (m_LeanListenPos);
	m_LeanEnt.Restore( savefile );

	// Cancel all leaning on load
	if (m_CurrentLeanTiltDegrees > 0.0f)
	{
		m_leanMoveStartTilt = m_CurrentLeanTiltDegrees;
		m_leanTime = 0.0f;
		m_b_leanFinished = false;
		m_leanMoveEndTilt = 0.0f;
	}

	savefile->ReadStaticObject( *m_PushForce );

	// ishtvan: To avoid accidental latching, clear held crouch key var
	m_bSlideOrDetachClimb = false;

	savefile->ReadBool(m_bSlideInitialized);

	// Shouldering anim
	{
		int iSAS = 0;
		savefile->ReadInt(iSAS);
		assert(iSAS >= eShoulderingAnimation_NotStarted 
			&& iSAS <= eShoulderingAnimation_Active); // sanity check
		m_eShoulderAnimState = static_cast<eShoulderingAnimation>(iSAS);
	}
	savefile->ReadFloat(m_fShoulderingTime);
	savefile->ReadVec3(m_PrevShoulderingPosOffset);
	savefile->ReadVec3(m_ShoulderingStartPosRelative);
	savefile->ReadVec3(m_ShoulderingCurrentPosRelative);
	savefile->ReadObject(reinterpret_cast<idClass*&>(m_pShoulderingGroundEntity));
	savefile->ReadBool(m_bShouldering_SkipDucking);
	savefile->ReadFloat(m_fShouldering_TimeToNextSound);
	savefile->ReadFloat(m_fPrevShoulderingPitchOffset);
	savefile->ReadBool(m_bMidAir);

	// Swimming
	savefile->ReadFloat(m_fSwimTimeStart_s);
	savefile->ReadBool(m_bSwimSoundStarted);
	m_fSwimLeadInDuration_s = -1.0f;
	m_fSwimLeadOutStart_s = -1.0f;
	m_fSwimLeadOutDuration_s = -1.0f;
	m_fSwimSpeedModCompensation = -1.0f;

	DM_LOG (LC_MOVEMENT, LT_DEBUG)LOGSTRING ("Restore finished\n");
}

/*
================
idPhysics_Player::SetPlayerInput
================
*/
void idPhysics_Player::SetPlayerInput( const usercmd_t &cmd, const idAngles &newViewAngles ) 
{
	command = cmd;

	m_DeltaViewYaw = command.angles[1] - m_lastCommandViewYaw;
	m_DeltaViewPitch = command.angles[0] - m_lastCommandViewPitch;
	m_DeltaViewYaw = SHORT2ANGLE(m_DeltaViewYaw);
	m_DeltaViewPitch = SHORT2ANGLE(m_DeltaViewPitch);

	// Test for pitch clamping
	float TestPitch = viewAngles.pitch + m_DeltaViewPitch;
	TestPitch = idMath::AngleNormalize180( TestPitch );

	if( TestPitch > pm_maxviewpitch.GetFloat() )
		m_DeltaViewPitch = pm_maxviewpitch.GetFloat() - viewAngles.pitch;
	else if( TestPitch < pm_minviewpitch.GetFloat() )
		m_DeltaViewPitch = pm_minviewpitch.GetFloat() - viewAngles.pitch;

	// zero the delta angles if the player's view is locked in place
	if( static_cast<idPlayer *>(self)->GetImmobilization() & EIM_VIEW_ANGLE )
	{
		m_DeltaViewYaw = 0.0f;
		m_DeltaViewPitch = 0.0f;
	}

	viewAngles = newViewAngles;	// can't use cmd.angles cause of the delta_angles

	m_lastCommandViewYaw = command.angles[1];
	m_lastCommandViewPitch = command.angles[0];
}

/*
================
idPhysics_Player::SetSpeed
================
*/
void idPhysics_Player::SetSpeed( const float newWalkSpeed, const float newCrouchSpeed ) 
{
	walkSpeed = newWalkSpeed;
	crouchSpeed = newCrouchSpeed;
}

/*
================
idPhysics_Player::SetMaxStepHeight
================
*/
void idPhysics_Player::SetMaxStepHeight( const float newMaxStepHeight ) {
	maxStepHeight = newMaxStepHeight;
}

/*
================
idPhysics_Player::GetMaxStepHeight
================
*/
float idPhysics_Player::GetMaxStepHeight( void ) const {
	return maxStepHeight;
}

/*
================
idPhysics_Player::SetMaxJumpHeight
================
*/
void idPhysics_Player::SetMaxJumpHeight( const float newMaxJumpHeight ) {
	maxJumpHeight = newMaxJumpHeight;
}

/*
================
idPhysics_Player::SetMovementType
================
*/
void idPhysics_Player::SetMovementType( const pmtype_t type ) {
	current.movementType = type;
}

/*
================
idPhysics_Player::GetMovementType - grayman #2345
================
*/
int idPhysics_Player::GetMovementType( void ) {
	return current.movementType;
}

/*
================
idPhysics_Player::SetAirAccelerate
================
*/
void idPhysics_Player::SetAirAccelerate( const float newAirAccelerate ) 
{
	m_AirAccelerate = newAirAccelerate;
}

/*
================
idPhysics_Player::SetKnockBack
================
*/
void idPhysics_Player::SetKnockBack( const int knockBackTime ) {
	if ( current.movementTime ) {
		return;
	}
	current.movementFlags |= PMF_TIME_KNOCKBACK;
	current.movementTime = knockBackTime;
}

/*
================
idPhysics_Player::SetDebugLevel
================
*/
void idPhysics_Player::SetDebugLevel( bool set ) {
	debugLevel = set;
}

/*
================
idPhysics_Player::Evaluate
================
*/
bool idPhysics_Player::Evaluate( int timeStepMSec, int endTimeMSec ) {
	idVec3 masterOrigin, oldOrigin;
	idMat3 masterAxis;

	// greebo: Don't clear the WATERLEVELs each frame, they are updated anyway.
	//waterLevel = WATERLEVEL_NONE;
	//waterType = 0;
	oldOrigin = current.origin;

	clipModel->Unlink();

	// if bound to a master
	if ( masterEntity ) 
	{
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + current.localOrigin * masterAxis;
		clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );
		current.velocity = ( current.origin - oldOrigin ) / ( timeStepMSec * 0.001f );
		masterDeltaYaw = masterYaw;
		masterYaw = masterAxis[0].ToYaw();
		masterDeltaYaw = masterYaw - masterDeltaYaw;
		return true;
	}

	ActivateContactEntities();

	MovePlayer( timeStepMSec );

	// Apply the push force to all objects encountered during MovePlayer
	m_PushForce->Evaluate( endTimeMSec );

	clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );

	if ( IsOutsideWorld() ) {
		gameLocal.Warning( "clip model outside world bounds for entity '%s' at (%s)", self->name.c_str(), current.origin.ToString(0) );
	}

	return true; //( current.origin != oldOrigin );
}

/*
================
idPhysics_Player::UpdateTime
================
*/
void idPhysics_Player::UpdateTime( int endTimeMSec ) {
}

/*
================
idPhysics_Player::GetTime
================
*/
int idPhysics_Player::GetTime( void ) const {
	return gameLocal.time;
}

/*
================
idPhysics_Player::GetImpactInfo
================
*/
void idPhysics_Player::GetImpactInfo( const int id, const idVec3 &point, impactInfo_t *info ) const {
	info->invMass = invMass;
	info->invInertiaTensor.Zero();
	info->position.Zero();
	info->velocity = current.velocity;
}

/*
================
idPhysics_Player::ApplyImpulse
================
*/
void idPhysics_Player::ApplyImpulse( const int id, const idVec3 &point, const idVec3 &impulse ) {
	if ( current.movementType != PM_NOCLIP ) {
		current.velocity += impulse * invMass;
	}
}

/*
================
idPhysics_Player::IsAtRest
================
*/
bool idPhysics_Player::IsAtRest( void ) const {
	return false;
}

/*
================
idPhysics_Player::GetRestStartTime
================
*/
int idPhysics_Player::GetRestStartTime( void ) const {
	return -1;
}

/*
================
idPhysics_Player::SaveState
================
*/
void idPhysics_Player::SaveState( void ) {
	saved = current;
}

/*
================
idPhysics_Player::RestoreState
================
*/
void idPhysics_Player::RestoreState( void ) {
	current = saved;

	clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );

	EvaluateContacts();
}

/*
================
idPhysics_Player::SetOrigin
================
*/
void idPhysics_Player::SetOrigin( const idVec3 &newOrigin, int id ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	current.localOrigin = newOrigin;
	if ( masterEntity ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + newOrigin * masterAxis;
	}
	else {
		current.origin = newOrigin;
	}

	clipModel->Link( gameLocal.clip, self, 0, newOrigin, clipModel->GetAxis() );
}

/*
================
idPhysics_Player::GetOrigin
================
*/
const idVec3 & idPhysics_Player::PlayerGetOrigin( void ) const {
	return current.origin;
}

/*
================
idPhysics_Player::SetAxis
================
*/
void idPhysics_Player::SetAxis( const idMat3 &newAxis, int id ) {
	clipModel->Link( gameLocal.clip, self, 0, clipModel->GetOrigin(), newAxis );
}

/*
================
idPhysics_Player::Translate
================
*/
void idPhysics_Player::Translate( const idVec3 &translation, int id ) {

	current.localOrigin += translation;
	current.origin += translation;

	clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );
}

/*
================
idPhysics_Player::Rotate
================
*/
void idPhysics_Player::Rotate( const idRotation &rotation, int id ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	current.origin *= rotation;
	if ( masterEntity ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.localOrigin = ( current.origin - masterOrigin ) * masterAxis.Transpose();
	}
	else {
		current.localOrigin = current.origin;
	}

	clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() * rotation.ToMat3() );
}

/*
================
idPhysics_Player::SetLinearVelocity
================
*/
void idPhysics_Player::SetLinearVelocity( const idVec3 &newLinearVelocity, int id ) {
	current.velocity = newLinearVelocity;
}

/*
================
idPhysics_Player::GetLinearVelocity
================
*/
const idVec3 &idPhysics_Player::GetLinearVelocity( int id ) const {
	return current.velocity;
}

bool idPhysics_Player::HasRunningVelocity()
{
	// Return true when about 7% above walkspeed
	return (current.velocity.LengthSqr() > Square(pm_walkspeed.GetFloat()) * 1.15);
}

/*
================
idPhysics_Player::SetPushed
================
*/
void idPhysics_Player::SetPushed( int deltaTime ) {
	idVec3 velocity;
	float d;

	// velocity with which the player is pushed
	velocity = ( current.origin - saved.origin ) / ( deltaTime * idMath::M_MS2SEC );

	// remove any downward push velocity
	d = velocity * gravityNormal;
	if ( d > 0.0f ) {
		velocity -= d * gravityNormal;
	}

	current.pushVelocity += velocity;
}

/*
================
idPhysics_Player::GetPushedLinearVelocity
================
*/
const idVec3 &idPhysics_Player::GetPushedLinearVelocity( const int id ) const {
	return current.pushVelocity;
}

/*
================
idPhysics_Player::ClearPushedVelocity
================
*/
void idPhysics_Player::ClearPushedVelocity( void ) {
	current.pushVelocity.Zero();
}

/*
================
idPhysics_Player::SetMaster

  the binding is never orientated
================
*/
void idPhysics_Player::SetMaster( idEntity *master, const bool orientated ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	if ( master ) {
		if ( !masterEntity ) {
			// transform from world space to master space
			self->GetMasterPosition( masterOrigin, masterAxis );
			current.localOrigin = ( current.origin - masterOrigin ) * masterAxis.Transpose();
			masterEntity = master;
			masterYaw = masterAxis[0].ToYaw();
		}
		ClearContacts();
	}
	else {
		if ( masterEntity ) {
			masterEntity = NULL;
		}
	}
}

const float	PLAYER_VELOCITY_MAX				= 4000;
const int	PLAYER_VELOCITY_TOTAL_BITS		= 16;
const int	PLAYER_VELOCITY_EXPONENT_BITS	= idMath::BitsForInteger( idMath::BitsForFloat( PLAYER_VELOCITY_MAX ) ) + 1;
const int	PLAYER_VELOCITY_MANTISSA_BITS	= PLAYER_VELOCITY_TOTAL_BITS - 1 - PLAYER_VELOCITY_EXPONENT_BITS;
const int	PLAYER_MOVEMENT_TYPE_BITS		= 3;
const int	PLAYER_MOVEMENT_FLAGS_BITS		= 8;

/*
================
idPhysics_Player::WriteToSnapshot
================
*/
void idPhysics_Player::WriteToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteFloat( current.origin[0] );
	msg.WriteFloat( current.origin[1] );
	msg.WriteFloat( current.origin[2] );
	msg.WriteFloat( current.velocity[0], PLAYER_VELOCITY_EXPONENT_BITS, PLAYER_VELOCITY_MANTISSA_BITS );
	msg.WriteFloat( current.velocity[1], PLAYER_VELOCITY_EXPONENT_BITS, PLAYER_VELOCITY_MANTISSA_BITS );
	msg.WriteFloat( current.velocity[2], PLAYER_VELOCITY_EXPONENT_BITS, PLAYER_VELOCITY_MANTISSA_BITS );
	msg.WriteDeltaFloat( current.origin[0], current.localOrigin[0] );
	msg.WriteDeltaFloat( current.origin[1], current.localOrigin[1] );
	msg.WriteDeltaFloat( current.origin[2], current.localOrigin[2] );
	msg.WriteDeltaFloat( 0.0f, current.pushVelocity[0], PLAYER_VELOCITY_EXPONENT_BITS, PLAYER_VELOCITY_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.pushVelocity[1], PLAYER_VELOCITY_EXPONENT_BITS, PLAYER_VELOCITY_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.pushVelocity[2], PLAYER_VELOCITY_EXPONENT_BITS, PLAYER_VELOCITY_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.stepUp );
	msg.WriteBits( current.movementType, PLAYER_MOVEMENT_TYPE_BITS );
	msg.WriteBits( current.movementFlags, PLAYER_MOVEMENT_FLAGS_BITS );
	msg.WriteDeltaLong( 0, current.movementTime );
}

/*
================
idPhysics_Player::ReadFromSnapshot
================
*/
void idPhysics_Player::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	current.origin[0] = msg.ReadFloat();
	current.origin[1] = msg.ReadFloat();
	current.origin[2] = msg.ReadFloat();
	current.velocity[0] = msg.ReadFloat( PLAYER_VELOCITY_EXPONENT_BITS, PLAYER_VELOCITY_MANTISSA_BITS );
	current.velocity[1] = msg.ReadFloat( PLAYER_VELOCITY_EXPONENT_BITS, PLAYER_VELOCITY_MANTISSA_BITS );
	current.velocity[2] = msg.ReadFloat( PLAYER_VELOCITY_EXPONENT_BITS, PLAYER_VELOCITY_MANTISSA_BITS );
	current.localOrigin[0] = msg.ReadDeltaFloat( current.origin[0] );
	current.localOrigin[1] = msg.ReadDeltaFloat( current.origin[1] );
	current.localOrigin[2] = msg.ReadDeltaFloat( current.origin[2] );
	current.pushVelocity[0] = msg.ReadDeltaFloat( 0.0f, PLAYER_VELOCITY_EXPONENT_BITS, PLAYER_VELOCITY_MANTISSA_BITS );
	current.pushVelocity[1] = msg.ReadDeltaFloat( 0.0f, PLAYER_VELOCITY_EXPONENT_BITS, PLAYER_VELOCITY_MANTISSA_BITS );
	current.pushVelocity[2] = msg.ReadDeltaFloat( 0.0f, PLAYER_VELOCITY_EXPONENT_BITS, PLAYER_VELOCITY_MANTISSA_BITS );
	current.stepUp = msg.ReadDeltaFloat( 0.0f );
	current.movementType = msg.ReadBits( PLAYER_MOVEMENT_TYPE_BITS );
	current.movementFlags = msg.ReadBits( PLAYER_MOVEMENT_FLAGS_BITS );
	current.movementTime = msg.ReadDeltaLong( 0 );

	if ( clipModel ) {
		clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );
	}
}

//################################################################
// Start Mantling Mod
//################################################################

float idPhysics_Player::GetMantleTimeForPhase(EMantlePhase mantlePhase)
{
	// Current implementation uses constants
	switch (mantlePhase)
	{
	case hang_DarkModMantlePhase:
		return cv_pm_mantle_hang_msecs.GetFloat();

	case pull_DarkModMantlePhase:
		return cv_pm_mantle_pull_msecs.GetFloat();

	case pullFast_DarkModMantlePhase:
		return cv_pm_mantle_pullFast_msecs.GetFloat();

	case shiftHands_DarkModMantlePhase:
		return cv_pm_mantle_shift_hands_msecs.GetFloat();

	case push_DarkModMantlePhase:
		return cv_pm_mantle_push_msecs.GetFloat();

	case pushNonCrouched_DarkModMantlePhase:
		return cv_pm_mantle_pushNonCrouched_msecs.GetFloat();

	case canceling_DarkModMantlePhase:
		assert(m_fmantleCancelDist >= 0.0f);
		return m_fmantleCancelDist * 1000.0 / cv_pm_mantle_cancel_speed.GetFloat();

	default:
		return 0.0f;
	}
}

//----------------------------------------------------------------------

void idPhysics_Player::MantleMove()
{
	idVec3 newPosition = current.origin;
	idVec3 totalMove(0,0,0);
	float timeForMantlePhase = GetMantleTimeForPhase(m_mantlePhase);

	// Compute proportion into the current movement phase which we are
	float timeRatio = 1.0f;

	if (timeForMantlePhase != 0)
	{
		timeRatio = (timeForMantlePhase - m_mantleTime) /  timeForMantlePhase;
	}

	// Branch based on phase
	if (m_mantlePhase == hang_DarkModMantlePhase)
	{
		// Starting at current position, hanging, rocking a bit.
		float rockDistance = 0.5f * cv_pm_mantle_roll_mod.GetFloat();

		newPosition = m_mantlePullStartPos;
		float timeRadians = idMath::PI * timeRatio;
		viewAngles.roll = idMath::Sin(timeRadians) * rockDistance;
		newPosition += (idMath::Sin(timeRadians) * rockDistance) * viewRight;
		
		if (self != NULL)
		{
			static_cast<idPlayer*>(self)->SetViewAngles(viewAngles);
		}
	}
	else if (m_mantlePhase == pull_DarkModMantlePhase || m_mantlePhase == pullFast_DarkModMantlePhase)
	{
		// Player pulls themself up to shoulder even with the surface
		totalMove = m_mantlePullEndPos - m_mantlePullStartPos;
		float factor = 0.5f * ( 1.0f + idMath::Sin( (timeRatio * 2.0f - 1.0f) * idMath::PI/2 ) );
		newPosition = m_mantlePullStartPos + (totalMove * factor);
	}
	else if (m_mantlePhase == shiftHands_DarkModMantlePhase)
	{
		// Rock back and forth a bit?
		float rockDistance = 1.0f;

		// Adjust rockDistance based on duration to reduce abrupt view change
		if (cv_pm_mantle_shift_hands_msecs.GetFloat() < 500.0f)
			rockDistance = cv_pm_mantle_shift_hands_msecs.GetFloat() / 500.0f;

		// Apply mantle roll mod
		rockDistance *= cv_pm_mantle_roll_mod.GetFloat();

		newPosition = m_mantlePullEndPos;
		float timeRadians = idMath::PI * timeRatio;
		newPosition += (idMath::Sin(timeRadians) * rockDistance) * viewRight;
		viewAngles.roll = idMath::Sin(timeRadians) * rockDistance;

		if (self != NULL)
		{
			static_cast<idPlayer*>(self)->SetViewAngles(viewAngles);
		}
	}
	else if (m_mantlePhase == push_DarkModMantlePhase || m_mantlePhase == pushNonCrouched_DarkModMantlePhase)
	{
		// Rocking back and forth to get legs up over edge
		// STiFU #4930: Reduce rockdistance for pushNonCrouched
		const float rockDistance =
			((m_mantlePhase == push_DarkModMantlePhase) ? 10.0f : 5.0f)
			* cv_pm_mantle_roll_mod.GetFloat();

		// Player pushes themselves upward to get their legs onto the surface
		totalMove = m_mantlePushEndPos - m_mantlePullEndPos;
		newPosition = m_mantlePullEndPos + (totalMove * idMath::Sin(timeRatio * (idMath::PI/2)) );

		// We go into duck during this phase and stay there until end
		if (m_mantlePhase == push_DarkModMantlePhase)
			current.movementFlags |= PMF_DUCKED;

		float timeRadians = idMath::PI * timeRatio;
		newPosition += (idMath::Sin (timeRadians) * rockDistance) * viewRight;
		viewAngles.roll = idMath::Sin (timeRadians) * rockDistance;

		// stgatilov: set precise values at the very end of animation
		if (timeRatio == 1.0f)
		{
			assert((newPosition - m_mantlePushEndPos).Length() <= 0.1f);
			newPosition = m_mantlePushEndPos;
			assert(fabs(viewAngles.roll) <= 0.1f);
			viewAngles.roll = 0.0f;
		}

		if (self != NULL)
		{
			static_cast<idPlayer*>(self)->SetViewAngles(viewAngles);
		}
	}
	else if (m_mantlePhase == canceling_DarkModMantlePhase)
	{
		// STiFU #4509: Use linear animation instead of sinus here so that we can
		// maintain speed when the animation is finished resulting in a smoother 
		// transition to AirMove()
		totalMove = m_mantleCancelEndPos - m_mantleCancelStartPos;
		newPosition = m_mantleCancelStartPos + timeRatio*totalMove;

		float timeRadians = idMath::HALF_PI * timeRatio;
		viewAngles.roll = idMath::Cos(timeRadians) * m_mantleCancelStartRoll * cv_pm_mantle_roll_mod.GetFloat();

		if (self != NULL)
		{
			static_cast<idPlayer*>(self)->SetViewAngles(viewAngles);
		}
	}

	// If there is a mantled entity, positions are relative to it.
	// Transform position to be relative to world origin.
	// (For now, translation only, TODO: Add rotation)
	if (m_p_mantledEntity != NULL)
	{
		idPhysics* p_physics = m_p_mantledEntity->GetPhysics();
		if (p_physics != NULL)
		{
			// Ishtvan: Track rotation as well
			// newPosition += p_physics->GetOrigin();
			newPosition = p_physics->GetOrigin() + p_physics->GetAxis() * newPosition;

			if (IsMantleEndPosClipping(p_physics))
				CancelMantle();
		}
	}

	SetOrigin(newPosition);
}


const bool idPhysics_Player::IsMantleEndPosClipping(idPhysics* pPhysicsMantledEntity)
{
	if (pPhysicsMantledEntity == nullptr)
		return false;

	// Transform coordinates relative to entity to world
	const idVec3 mantleEndWorld = pPhysicsMantledEntity->GetOrigin() 
		+ pPhysicsMantledEntity->GetAxis() * m_mantlePushEndPos;

	// Load appropriate clipping model
	if (current.movementFlags & PMF_DUCKED)
	{
		// Load crouching model
		idBounds bounds = clipModel->GetBounds();
		bounds[1][2] = pm_crouchheight.GetFloat();

		clipModel->LoadModel(pm_usecylinder.GetBool() ? idTraceModel(bounds, 8) : idTraceModel(bounds));
	}
	else
	{
		// Load standing model
		idBounds bounds = clipModel->GetBounds();
		bounds[1][2] = pm_normalheight.GetFloat();

		clipModel->LoadModel(pm_usecylinder.GetBool() ? idTraceModel(bounds, 8) : idTraceModel(bounds));
	}

	// Check clipping
	trace_t endPosTrace;
	gameLocal.clip.Translation(endPosTrace, mantleEndWorld, mantleEndWorld,
		clipModel, clipModel->GetAxis(), clipMask, self);
	if (endPosTrace.fraction >= 1.0f)
	{
		// No clipping
		return false;
	}
	else
	{
		// We intersect with world geometry

		if ((current.movementFlags & PMF_DUCKED) == 0)
		{
			// We will clip standing up. Go to ducked state and retry
			DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING("MantleMod: Clipping into world. Going to crouched state\r");
			current.movementFlags |= PMF_DUCKED;
			return IsMantleEndPosClipping(pPhysicsMantledEntity);
		}

		DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING("MantleMod: Clipping into world. Canceling mantle.\r");
		return true;
	}		
}

//----------------------------------------------------------------------

void idPhysics_Player::UpdateMantleTimers()
{
	// Frame seconds left
	float framemSecLeft = framemsec;

	// Update jump held down timer: This actually grows, not drops
	if (!( current.movementFlags & PMF_JUMP_HELD ) ) 
	{
		m_jumpHeldDownTime = 0;
	}
	else
	{
		m_jumpHeldDownTime += framemsec;
	}

	// Skip all this if done mantling
	if (m_mantlePhase != notMantling_DarkModMantlePhase && m_mantlePhase != fixClipping_DarkModMantlePhase)
	{
		idPlayer* player = static_cast<idPlayer*>(self); // grayman #3010
		// Handle expiring mantle phases
		while (framemSecLeft >= m_mantleTime && m_mantlePhase != notMantling_DarkModMantlePhase && m_mantlePhase != fixClipping_DarkModMantlePhase)
		{
			framemSecLeft -= m_mantleTime;
			m_mantleTime = 0;

			// Advance mantle phase
			switch (m_mantlePhase)
			{
			case hang_DarkModMantlePhase:
				DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING ("MantleMod: Pulling up...\r");
				m_mantlePhase = pull_DarkModMantlePhase;
				player->StartSound("snd_player_mantle_rustle_short", SND_CHANNEL_BODY3, 0, false, NULL);
				player->StartSound("snd_player_mantle_pull", SND_CHANNEL_VOICE, 0, false, NULL); // grayman #3010
				break;

			case pull_DarkModMantlePhase:
				DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING ("MantleMod: Shifting hand position...\r");
				m_mantlePhase = shiftHands_DarkModMantlePhase;
				break;

			case pullFast_DarkModMantlePhase: // STiFU #4945: Skip shift hands
				DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING("MantleMod: Quickly pushing self up...\r");
				m_mantlePhase = pushNonCrouched_DarkModMantlePhase;

				// Go into crouch
				current.movementFlags |= PMF_DUCKED;

				player->StartSound("snd_player_mantle_rustle_short", SND_CHANNEL_BODY3, 0, false, NULL);
				player->StartSound("snd_player_mantle_push", SND_CHANNEL_VOICE, 0, false, NULL); // grayman #3010
				break;

			case shiftHands_DarkModMantlePhase:
				DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING ("MantleMod: Pushing self up...\r");
				m_mantlePhase = push_DarkModMantlePhase;

				// Go into crouch
				current.movementFlags |= PMF_DUCKED;

				player->StartSound("snd_player_mantle_rustle_short", SND_CHANNEL_BODY3, 0, false, NULL);
				player->StartSound("snd_player_mantle_push", SND_CHANNEL_VOICE, 0, false, NULL); // grayman #3010
				break;

			case pushNonCrouched_DarkModMantlePhase:
			case push_DarkModMantlePhase:
				DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING ("MantleMod: mantle completed\r");

				//stgatilov: finish mantling animation completely (#4435)
				//(set position of player exactly to the mantle endpoint)
				MantleMove();

				// check for clipping problems after mantling
				// will advance to notMantling when the player isn't clipping
				m_mantlePhase = fixClipping_DarkModMantlePhase;

				break;

			case canceling_DarkModMantlePhase:
				DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING("MantleMod: cancel mantle completed\r");
				MantleMove();
				static_cast<idPlayer*>(self)->SetImmobilization("MantleMove", 0);
				m_fmantleCancelDist = -1.0f;
				m_mantlePhase = notMantling_DarkModMantlePhase;
				m_mantleTime = 0.0f;
				break;

			default:
				m_mantlePhase = notMantling_DarkModMantlePhase;
				break;
			}

			// Get time it takes to perform a mantling phase
			m_mantleTime = GetMantleTimeForPhase(m_mantlePhase);
			
			// Handle end of mantle
			if (m_mantlePhase == fixClipping_DarkModMantlePhase)
			{
				// Handle end of mantle
				// Ishtvan 11/20/05 - Raise weapons after mantle is done
				static_cast<idPlayer*>(self)->SetImmobilization("MantleMove", 0);

				// The mantle consumes all velocity
				current.velocity.Zero();
			}
		}

		// Reduce mantle timer
		if (m_mantlePhase == fixClipping_DarkModMantlePhase)
		{
			m_mantleTime = 0;
		}
		else
		{
			m_mantleTime -= framemSecLeft;
		}
	} // This code block is executed only if phase != notMantling && phase != fixClipping
}

//----------------------------------------------------------------------

bool idPhysics_Player::IsMantling() const
{
	return m_mantlePhase != notMantling_DarkModMantlePhase && m_mantlePhase != fixClipping_DarkModMantlePhase;
}

//----------------------------------------------------------------------

EMantlePhase idPhysics_Player::GetMantlePhase() const
{
	return m_mantlePhase;
}

//----------------------------------------------------------------------

void idPhysics_Player::CancelMantle()
{
	DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING ("Mantle cancelled\r");

	// STiFU #4509: Add canceling animation
	// - Move back to last non-clipping location
	// - Rotate back viewAngles.roll to 0
	m_p_mantledEntity = NULL;
	m_mantlePhase = canceling_DarkModMantlePhase;

	m_mantleCancelStartPos = GetOrigin();
	m_mantleCancelStartRoll = viewAngles.roll;

	// Find last non clipping location
	// NOTE: There is no guarantee the end position does not clip by the time the 
	// canceling animation is finished. But there is nothing we can do about that.	
	trace_t cancelEndPosTrace;
	gameLocal.clip.Translation(cancelEndPosTrace, m_mantleStartPosWorld, m_mantleCancelStartPos,
		clipModel, clipModel->GetAxis(), clipMask, self);
	m_mantleCancelEndPos = m_mantleStartPosWorld + cancelEndPosTrace.fraction * (m_mantleCancelStartPos - m_mantleStartPosWorld);

	// Set current velocity: Null the XY-component so the player falls down after the cancel animation
	idVec3 mantleCancelDir = m_mantleCancelEndPos - m_mantleCancelStartPos;
	m_fmantleCancelDist = fabs(mantleCancelDir.NormalizeFast());
	current.velocity = ((mantleCancelDir * cv_pm_mantle_cancel_speed.GetFloat()) * gravityNormal) * gravityNormal;

	// Set mantle time. Must be called AFTER setting m_fmantleCancelDist
	m_mantleTime = GetMantleTimeForPhase(canceling_DarkModMantlePhase);

	// Prevent awkward returning to uncrouched state while falling after
	// canceling the mantle
	idPlayer* pPlayer = static_cast<idPlayer*>(self);
	if (pPlayer == nullptr)
		return;	
	if ((current.movementFlags & PMF_DUCKED) != 0)
	{
		pPlayer->m_CrouchIntent = true;
		pPlayer->m_IdealCrouchState = true;
	}

	// Play a canceling sound
 	pPlayer->StartSound("snd_player_mantle_cancel", SND_CHANNEL_ANY, 0, false, NULL);
}

//----------------------------------------------------------------------

//----------------------------------------------------------------------

void idPhysics_Player::StartMantle
(
	EMantlePhase initialMantlePhase,
	idVec3 eyePos,
	idVec3 startPos,
	idVec3 endPos
)
{
	// Ishtvan 10/16/05
	// If mantling starts while on a rope, detach from that rope
	if ( m_bOnRope )
	{
		RopeDetach();
	}

	// If mantling starts while climbing, detach from climbing surface
	if ( m_bOnClimb )
	{
		ClimbDetach();
	}

	// Ishtvan 11/20/05 - Lower weapons when mantling
	static_cast<idPlayer*>(self)->SetImmobilization( "MantleMove", EIM_ATTACK );

	// greebo: Disable the next mantle start here, this is set to TRUE again 
	// when the jump key is released outside a mantle phase
	m_mantleStartPossible = false;

	// Calculate mantle distance
	idVec3 mantleDistanceVec = endPos - startPos;

	idPlayer* player = static_cast<idPlayer*>(self); // grayman #3010

	// Init forced-crouch mantle handling
	player->ResetForcedCrouchMantle();

	// Log starting phase
	if (initialMantlePhase == hang_DarkModMantlePhase)
	{
		DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING("Mantle starting with hang\r");
		player->StartSound("snd_player_mantle_impact", SND_CHANNEL_BODY2, 0, false, NULL);
		player->StartSound("snd_player_mantle_rustle", SND_CHANNEL_BODY3, 0, false, NULL);

		// Impart a force on mantled object?
		if (m_p_mantledEntity != NULL && self != NULL)
		{
			impactInfo_t info;
			m_p_mantledEntity->GetImpactInfo(self, m_mantledEntityID, endPos, &info);

			if (info.invMass != 0.0f) 
			{
				m_p_mantledEntity->ActivatePhysics(self);
				m_p_mantledEntity->ApplyImpulse( self, m_mantledEntityID, endPos, current.velocity / ( info.invMass * 2.0f ) );
			}
		}
	}
	else if (initialMantlePhase == pull_DarkModMantlePhase)
	{
		DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING("Mantle starting with pull upward\r");
		player->StartSound("snd_player_mantle_impact", SND_CHANNEL_BODY2, 0, false, NULL);
		player->StartSound("snd_player_mantle_rustle", SND_CHANNEL_BODY3, 0, false, NULL);
		player->StartSound("snd_player_mantle_pull", SND_CHANNEL_VOICE, 0, false, NULL); // grayman #3010
	}
	else if (initialMantlePhase == pullFast_DarkModMantlePhase)
	{
		DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING("Mantle starting with quick silent pull upward\r"); 
		player->StartSound("snd_player_mantle_impact_subtle", SND_CHANNEL_BODY2, 0, false, NULL);
		player->StartSound("snd_player_mantle_rustle", SND_CHANNEL_BODY3, 0, false, NULL);
	}
	else if (initialMantlePhase == shiftHands_DarkModMantlePhase)
	{
		DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING("Mantle starting with shift hands\r");
	}
	else if (initialMantlePhase == push_DarkModMantlePhase)
	{
		// Go into crouch
		current.movementFlags |= PMF_DUCKED;

		// Start with push upward
		DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING("Mantle starting with push upward\r");
		player->StartSound("snd_player_mantle_impact_subtle", SND_CHANNEL_BODY2, 0, false, NULL);
		player->StartSound("snd_player_mantle_rustle", SND_CHANNEL_BODY3, 0, false, NULL);
		player->StartSound("snd_player_mantle_push", SND_CHANNEL_VOICE, 0, false, NULL); // grayman #3010
	}
	else if (initialMantlePhase == pushNonCrouched_DarkModMantlePhase)
	{
		DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING("Mantle starting with push non-crouched upward\r");

		// We make contact with the feet. Play footstep sound and the endpos
		idPlayer* pPlayer = dynamic_cast<idPlayer*>(self);
		if (pPlayer)
			pPlayer->PlayPlayerFootStepSound(&endPos, true);

		// Play grunt at high velocity
		if (current.velocity.LengthSqr() >
			pow(cv_pm_mantle_pushNonCrouched_playgrunt_speedthreshold.GetFloat(), 2))
		{
			player->StartSound("snd_player_mantle_push", SND_CHANNEL_VOICE, 0, false, NULL);
		}
	}

	// #6231: Delayed fall damage on mantle
	// Zero out all velocity sooner than at the end of the mantle so
	// that the player receives damage feedback immediately on impact
	// from a fall rather than at the end of the mantle animation.
	// Also, zeroing out all velocity at mantle start makes sense,
	// because the animation is stop, mantle, stop.
	current.velocity.Zero();

	m_mantlePhase = initialMantlePhase;
	m_mantleTime = GetMantleTimeForPhase(m_mantlePhase);

	// Make positions relative to entity
	if (m_p_mantledEntity != NULL)
	{
		idPhysics* p_physics = m_p_mantledEntity->GetPhysics();
		if (p_physics != NULL)
		{
			const idVec3& mantledEntityOrigin = p_physics->GetOrigin();
			const idMat3& mantledEntityAxis = p_physics->GetAxis();

			// ishtvan 1/3/2010: Incorporate entity rotation as well as translation
			startPos = (startPos - mantledEntityOrigin) * mantledEntityAxis.Transpose();
			eyePos = (eyePos - mantledEntityOrigin) * mantledEntityAxis.Transpose();
			endPos = (endPos - mantledEntityOrigin) * mantledEntityAxis.Transpose();
		}
	}

	// Set end position
	m_mantlePushEndPos = endPos;

	if (	initialMantlePhase == pull_DarkModMantlePhase 
		||	initialMantlePhase == hang_DarkModMantlePhase )
	{
		// Pull from start position up to about 2/3 of eye height
		m_mantlePullStartPos = startPos;
		m_mantlePullEndPos = eyePos;

		m_mantlePullEndPos += GetGravityNormal() * pm_normalheight.GetFloat() / 4.5f;
	}
	else if (initialMantlePhase == pullFast_DarkModMantlePhase)
	{
		// Pull from start position up to eye height
		m_mantlePullStartPos = startPos;
		m_mantlePullEndPos = eyePos;
	}
	else
	{
		// Starting with push from current position
		m_mantlePullEndPos = startPos;
	}

	m_mantleStartPosWorld = GetOrigin();
}

//----------------------------------------------------------------------

bool idPhysics_Player::CheckJumpHeldDown()
{
	return m_jumpHeldDownTime > cv_pm_mantle_jump_hold_trigger.GetInteger();
}

//----------------------------------------------------------------------

void idPhysics_Player::GetCurrentMantlingReachDistances
(
	float& out_maxVerticalReachDistance,
	float& out_maxHorizontalReachDistance,
	float& out_maxMantleTraceDistance
)
{
	// Determine arm reach in each direction
	float armReach = pm_normalheight.GetFloat() * cv_pm_mantle_reach.GetFloat();
	float armVerticalReach = pm_normalheight.GetFloat() * cv_pm_mantle_height.GetFloat();

	// Trace out as far as horizontal arm length from player
	out_maxMantleTraceDistance = armReach;

	// Determine maximum vertical and horizontal distance components for
	// a mantleable surface
	if (current.movementFlags & PMF_DUCKED /*&& !OnRope() && !OnLadder()*/)
	{
		out_maxVerticalReachDistance = pm_crouchheight.GetFloat() + armVerticalReach;
		out_maxHorizontalReachDistance = armReach;
	}
	/*else if (OnRope() || OnLadder())
	{
		// angua: need larger reach when on rope
		out_maxVerticalReachDistance = pm_normalheight.GetFloat() + armVerticalReach;
		out_maxHorizontalReachDistance = 2* armReach;
		out_maxMantleTraceDistance *= 2;
	}*/
	else
	{
		// This vertical distance is up from the players feet
		out_maxVerticalReachDistance = pm_normalheight.GetFloat() + armVerticalReach;
		out_maxHorizontalReachDistance = armReach;
	}
}

//----------------------------------------------------------------------

void idPhysics_Player::MantleTargetTrace
(
	float maxMantleTraceDistance,
	const idVec3& eyePos,
	const idVec3& forwardVec,
	trace_t& out_trace
)
{
	// Calculate end point of gaze trace
	idVec3 end = eyePos + (maxMantleTraceDistance * forwardVec);

	// Run gaze trace
	gameLocal.clip.TracePoint( out_trace, eyePos, end, MASK_SOLID, self );

	// If that trace didn't hit anything, try a taller trace forward along the midline
	// of the player's body for the full player's height out the trace distance.
	if ( out_trace.fraction >= 1.0f ) 
	{
		idVec3 upVector = -GetGravityNormal();

		// Project forward vector onto the a plane perpendicular to gravity
		idVec3 forwardPerpGrav = forwardVec;
		forwardPerpGrav.ProjectOntoPlane(upVector);

		// Create bounds for translation trace model
		idBounds bounds = clipModel->GetBounds();
		idBounds savedBounds = bounds;

		bounds[0][1] = (savedBounds[0][1] + savedBounds[1][1]) / 2;
		bounds[0][1] -= 0.01f;
		bounds[1][1] = bounds[0][1] + 0.02f;
		bounds[0][0] = bounds[0][1];
		bounds[1][0] = bounds[1][1];

		clipModel->LoadModel( pm_usecylinder.GetBool() ? idTraceModel(bounds, 8) : idTraceModel(bounds) );
		
		DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING("Mantle gaze trace didn't hit anything, so doing forward movement trace for mantle target\r");

		gameLocal.clip.Translation 
		(
			out_trace, 
			current.origin, 
			current.origin + (maxMantleTraceDistance * forwardPerpGrav), 
			clipModel, 
			clipModel->GetAxis(), 
			MASK_SOLID, 
			self
		);

		//gameRenderWorld->DebugBounds(colorCyan, bounds, current.origin, 2000);
		//gameRenderWorld->DebugBounds(colorBlue, bounds, current.origin + (maxMantleTraceDistance * forwardPerpGrav), 2000);

		// Restore player clip model to normal
		clipModel->LoadModel( pm_usecylinder.GetBool() ? idTraceModel(savedBounds, 8) : idTraceModel(savedBounds) );
	}
	
	// Get the entity to be mantled
	if (out_trace.c.entityNum != ENTITYNUM_NONE)
	{
		// Track entity which is was the chosen target
		m_p_mantledEntity = gameLocal.entities[out_trace.c.entityNum];
		
		if (m_p_mantledEntity->IsMantleable())
		{
			m_mantledEntityID = out_trace.c.id;

			DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING("Mantle target entity is called '%s'\r", m_p_mantledEntity->name.c_str());
		}
		else
		{
			// Oops, this entity isn't mantleable
			m_p_mantledEntity = NULL;
			out_trace.fraction = 1.0f; // Pretend we didn't hit anything
		}
	}
}

//----------------------------------------------------------------------

idPhysics_Player::EMantleable idPhysics_Player::DetermineIfMantleTargetHasMantleableSurface
(	
	float maxVerticalReachDistance,
	float maxHorizontalReachDistance,
	trace_t& in_targetTraceResult,
	idVec3& out_mantleEndPoint
)
{
	// Never mantle onto non-mantleable entities (early exit)
	if (in_targetTraceResult.fraction < 1.0f)
	{
		idEntity* ent = gameLocal.entities[in_targetTraceResult.c.entityNum];

		if (ent == NULL || !ent->IsMantleable())
		{
			// The mantle target is an unmantleable entity
			return EMantleable_No;
		}
	}

	// Try moving player's bounding box up from the trace hit point
	// in steps up to the maximum distance and see if at any point
	// there are no collisions. If so, we can mantle.

	// First point to test has gravity orthogonal coordinates set
	// to the ray trace collision point. It then has gravity non-orthogonal
	// coordinates set from the current player origin.  However,
	// for the non-orthogonal-to-gravity coordinates, the trace.c.point
	// location is a better starting place.  Because of rear surface occlusion,
	// it will always be closer to the actual "upper" surface than the player
	// origin unless the object is "below" the player relative to gravity.
	// And, in that "below" case, mantling isn't possible anyway.
	
	// This sets coordinates to their components which are orthogonal
	// to gravity.
	idVec3 componentOrthogonalToGravity = in_targetTraceResult.c.point;
	componentOrthogonalToGravity.ProjectOntoPlane(-gravityNormal);

	// This sets coordintes to their components parallel to gravity
	idVec3 componentParallelToGravity;
	componentParallelToGravity.x = -gravityNormal.x * in_targetTraceResult.c.point.x;
	componentParallelToGravity.y = -gravityNormal.y * in_targetTraceResult.c.point.y;
	componentParallelToGravity.z = -gravityNormal.z * in_targetTraceResult.c.point.z;

	// What parallel to gravity reach distance is already used up at this point
	idVec3 originParallelToGravity;
	originParallelToGravity.x = -gravityNormal.x * current.origin.x;
	originParallelToGravity.y = -gravityNormal.y * current.origin.y;
	originParallelToGravity.z = -gravityNormal.z * current.origin.z;

	float verticalReachDistanceUsed = (componentParallelToGravity - originParallelToGravity).Length();
	
	DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING
	(
		"Initial vertical reach distance used = %f out of maximum of %f\r", 
		verticalReachDistanceUsed, 
		maxVerticalReachDistance
	);

	// The first test point 
	idVec3 testPosition = componentOrthogonalToGravity + componentParallelToGravity;

	// Load crouch model
	// as mantling ends in a crouch
	if (!(current.movementFlags & PMF_DUCKED))
	{
		idBounds bounds = clipModel->GetBounds();
		bounds[1][2] = pm_crouchheight.GetFloat();

		clipModel->LoadModel( pm_usecylinder.GetBool() ? idTraceModel(bounds, 8) : idTraceModel(bounds) );
	}

	// We try moving it up by the step distance up to the maximum height until
	// there are no collisions
	bool b_keepTesting = verticalReachDistanceUsed < maxVerticalReachDistance;
	bool b_mantlePossible = false;
	bool b_lastCollisionWasMantleable = true;

	trace_t worldMantleTrace;

	while (b_keepTesting)
	{
		// Try collision in_targetTraceResult
		idVec3 mantleTraceStart = testPosition;
		gameLocal.clip.Translation( worldMantleTrace, mantleTraceStart, testPosition, clipModel, clipModel->GetAxis(), clipMask, self );

		if (worldMantleTrace.fraction >= 1.0f)
		{
			// We can mantle to there, unless the last test collided with something non-mantleable.
			// Either way we're done here.
			b_keepTesting = false;

			if (b_lastCollisionWasMantleable)
			{
				b_mantlePossible = true;
			}
		}
		else
		{
			idEntity* ent = gameLocal.entities[ worldMantleTrace.c.entityNum ];

			if (ent && !ent->IsMantleable())
			{
				// If we collided with a non-mantleable entity, then flag that.
				// This is to prevent situations where we start out mantling on a low ledge
				// (like a stair) on which a non-mantleable entity (like an AI) is standing,
				// and proceed to mantle over the AI.
				b_lastCollisionWasMantleable = false;
			}
			else
			{
				// On the other hand, if there's a shelf above the AI, then we can still mantle
				// the shelf.
				b_lastCollisionWasMantleable = true;
			}

			if (verticalReachDistanceUsed < maxVerticalReachDistance)
			{
				// Try next test position

				float testIncrementAmount = maxVerticalReachDistance - verticalReachDistanceUsed;

				// Establish upper bound for increment test size
				if (testIncrementAmount > MANTLE_TEST_INCREMENT)
				{
					testIncrementAmount = MANTLE_TEST_INCREMENT;
				}

				// Establish absolute minimum increment size so that
				// we don't approach increment size below floating point precision,
				// which would cause an infinite loop.
				if (testIncrementAmount < 1.0f)
				{
					testIncrementAmount = 1.0f;
				}

				// Update location by increment size
				componentParallelToGravity += (-gravityNormal * testIncrementAmount);
				verticalReachDistanceUsed = (componentParallelToGravity - originParallelToGravity).Length();

				DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING
				(
					"Ledge Search: Vertical reach distance used = %f out of maximum of %f\r", 
					verticalReachDistanceUsed, 
					maxVerticalReachDistance
				);

				// Modify test position
				testPosition = componentOrthogonalToGravity + componentParallelToGravity;
			}
			else
			{
				// No surface we could fit on against gravity from raytrace hit point
				// up as far as we can reach
				b_keepTesting = false;
				DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING("No mantleable surface within reach distance\r");
			}
		}
		
	}
	
	// Don't mantle onto surfaces that are too steep.
	// Any surface with an angle whose cosine is
	// smaller than MIN_FLATNESS is too steep.
	float minFlatness = cv_pm_mantle_minflatness.GetFloat();

	if (b_mantlePossible)
	{
		// Attempt to get the normal of the surface we'd be standing on
		// In rare cases this may not collide
		trace_t floorTrace;
		gameLocal.clip.Translation( floorTrace, testPosition,
			testPosition + (gravityNormal * MANTLE_TEST_INCREMENT),
			clipModel, clipModel->GetAxis(), clipMask, self );
		
		if (floorTrace.fraction < 1.0f)
		{
			// Uses the dot product to compare against the cosine of an angle.
			// Comparing to cos(90)=0 means we can mantle on top of any surface
			// Comparing to cos(0)=1 means we can only mantle on top of perfectly flat surfaces
			
			float flatness = floorTrace.c.normal * (-gravityNormal);
			
			DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING(
				"Floor %.2f,%.2f,%.2f; grav %.2f,%.2f,%.2f; dot %f; %s\r",
				floorTrace.c.normal.x, floorTrace.c.normal.y, floorTrace.c.normal.z,
				gravityNormal.x, gravityNormal.y, gravityNormal.z,
				flatness, flatness < minFlatness ? "too steep" : "OK");
			
			if (flatness < minFlatness)
			{
				b_mantlePossible = false;
			}
		}
	}

	// Return Val
	EMantleable Mantleable = EMantleable_No;
	if (b_mantlePossible)
	{
		out_mantleEndPoint = testPosition;
		Mantleable = EMantleable_YesCrouched;
	}
	
	// Must restore standing model if player is not crouched
	if (!(current.movementFlags & PMF_DUCKED))
	{
		// Load back standing model
		idBounds bounds = clipModel->GetBounds();
		bounds[1][2] = pm_normalheight.GetFloat();

		clipModel->LoadModel( pm_usecylinder.GetBool() ? idTraceModel(bounds, 8) : idTraceModel(bounds) );

		// STiFU #4930: Test if the end position can also be reached standing up.
		// Do this only if player is non-crouched
		if (b_mantlePossible)
		{
			trace_t ceilTrace;
			gameLocal.clip.Translation(ceilTrace, testPosition, testPosition,
				clipModel, clipModel->GetAxis(), clipMask, self);
			if (ceilTrace.fraction >= 1.0f)
			{
				Mantleable = EMantleable_YesUpstraight;
				DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING("Surface can be mantled upstraight\r");
			}
		}
	}
	
	return Mantleable;
}

//----------------------------------------------------------------------

bool idPhysics_Player::DetermineIfPathToMantleSurfaceIsPossible
(
	float maxVerticalReachDistance,
	float maxHorizontalReachDistance,
	bool  testCrouched,
	const idVec3& in_eyePos,
	const idVec3& in_mantleStartPoint,
	const idVec3& in_mantleEndPoint
)
{
	// Make sure path from current location
	// upward can be traversed.
	trace_t roomForMoveUpTrace;
	idVec3 MoveUpStart = in_mantleStartPoint;
	idVec3 MoveUpEnd;

	// Go to coordinate components against gravity from current location
	idVec3 componentOrthogonalToGravity;
	componentOrthogonalToGravity = in_mantleStartPoint;
	componentOrthogonalToGravity.ProjectOntoPlane (-gravityNormal);
	MoveUpEnd = componentOrthogonalToGravity;

	MoveUpEnd.x += -gravityNormal.x * in_mantleEndPoint.x;
	MoveUpEnd.y += -gravityNormal.y * in_mantleEndPoint.y;
	MoveUpEnd.z += -gravityNormal.z * in_mantleEndPoint.z;

	// Change clip model if needed
	bool clipModelChanged = false;
	if (testCrouched && !(current.movementFlags & PMF_DUCKED))
	{
		// Load crouching model
		idBounds bounds = clipModel->GetBounds();
		bounds[1][2] = pm_crouchheight.GetFloat();

		clipModel->LoadModel( pm_usecylinder.GetBool() ? idTraceModel(bounds, 8) : idTraceModel(bounds) );
		clipModelChanged = true;
	}
	else if (!testCrouched && (current.movementFlags & PMF_DUCKED))
	{
		// Load standing model
		idBounds bounds = clipModel->GetBounds();
		bounds[1][2] = pm_normalheight.GetFloat();

		clipModel->LoadModel(pm_usecylinder.GetBool() ? idTraceModel(bounds, 8) : idTraceModel(bounds));
		clipModelChanged = true;
	}

	gameLocal.clip.Translation
	(
		roomForMoveUpTrace, 
		MoveUpStart, 
		MoveUpEnd, 
		clipModel, 
		clipModel->GetAxis(), 
		clipMask, 
		self 
	);

	// Change back clip model
	if (clipModelChanged)
	{
		if (testCrouched)
		{
			// Load back standing model
			idBounds bounds = clipModel->GetBounds();
			bounds[1][2] = pm_normalheight.GetFloat();

			clipModel->LoadModel(pm_usecylinder.GetBool() ? idTraceModel(bounds, 8) : idTraceModel(bounds));
		}
		else
		{
			// Load back crouching model
			idBounds bounds = clipModel->GetBounds();
			bounds[1][2] = pm_crouchheight.GetFloat();

			clipModel->LoadModel(pm_usecylinder.GetBool() ? idTraceModel(bounds, 8) : idTraceModel(bounds));
		}
	}

	// Log
	if (roomForMoveUpTrace.fraction < 1.0)
	{
		DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING
		(
			"Collision test from (%f %f %f) to (%f %f %f) yieled trace fraction %f\r",
			MoveUpStart.x,
			MoveUpStart.y,
			MoveUpStart.z,
			MoveUpEnd.x,
			MoveUpEnd.y,
			MoveUpEnd.z,
			roomForMoveUpTrace.fraction
		);
		DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING ("Not enough vertical clearance along mantle path\r");
		return false;
	}
	else
	{
		return true;
	}

}

//----------------------------------------------------------------------

idPhysics_Player::EMantleable idPhysics_Player::ComputeMantlePathForTarget
(	
	float maxVerticalReachDistance,
	float maxHorizontalReachDistance,
	const idVec3& eyePos,
	trace_t& in_targetTraceResult,
	idVec3& out_mantleEndPoint
)
{
	// Up vector
	idVec3 upVector = -GetGravityNormal();

	// Mantle start point is origin
	const idVec3& mantleStartPoint = GetOrigin();

	// Check if trace target has a mantleable surface
	EMantleable IsSurfaceMantleable = DetermineIfMantleTargetHasMantleableSurface
	(
		maxVerticalReachDistance,
		maxHorizontalReachDistance,
		in_targetTraceResult,
		out_mantleEndPoint
	);

	if (IsSurfaceMantleable > EMantleable_No)
	{
		// Check if path to mantle end point is not blocked
		if (IsSurfaceMantleable == EMantleable_YesUpstraight)
		{
			// STiFU #4930: Try standing up first, if that fails, try crouched
			static const bool bCrouchedTest = false;
			const bool bPathClearStandingUp = DetermineIfPathToMantleSurfaceIsPossible(
				maxVerticalReachDistance,
				maxHorizontalReachDistance,
				bCrouchedTest,
				eyePos,
				mantleStartPoint,
				out_mantleEndPoint
			);
			if (!bPathClearStandingUp)
				IsSurfaceMantleable = EMantleable_YesCrouched;
		}
		if (IsSurfaceMantleable != EMantleable_YesUpstraight)
		{
			static const bool bCrouchedTest = true;
			const bool bPathClearCrouched = DetermineIfPathToMantleSurfaceIsPossible(
				maxVerticalReachDistance,
				maxHorizontalReachDistance,
				bCrouchedTest,
				eyePos,
				mantleStartPoint,
				out_mantleEndPoint
			);
			if (!bPathClearCrouched)
				IsSurfaceMantleable = EMantleable_No;
		}
		

		if (IsSurfaceMantleable > EMantleable_No)
		{
			// Is end point too far away?
			idVec3 endDistanceVector = out_mantleEndPoint - eyePos;
			float endDistance = endDistanceVector.Length();
			idVec3 upDistance = endDistanceVector;
			
			upDistance.x *= upVector.x;
			upDistance.y *= upVector.y;
			upDistance.z *= upVector.z;
			float upDist = upDistance.Length();

			float nonUpDist = idMath::Sqrt(endDistance*endDistance - upDist*upDist);

			// Check the calculated distances
			if (upDist < 0.0)
			{
				DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING("Mantleable surface was below player's feet. No belly slide allowed.\r");
				IsSurfaceMantleable = EMantleable_No;
			}
			else if	(upDist > maxVerticalReachDistance || nonUpDist > maxHorizontalReachDistance)
			{
				// Its too far away either horizontally or vertically
				DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING
				(
					"Distance to end point was (%f, %f) (horizontal, vertical) which is greater than limits of (%f %f), so mantle cannot be done\n",
					upDist,
					nonUpDist,
					maxVerticalReachDistance,
					maxHorizontalReachDistance
				);

				IsSurfaceMantleable = EMantleable_No;
			}

			// Distances are reasonable
		}
	}

	// Return result
	return IsSurfaceMantleable;
}

//----------------------------------------------------------------------

void idPhysics_Player::PerformMantle()
{
	// Can't start mantle if already mantling or not yet possible (jump button not yet released)
	if ( !(m_mantlePhase == notMantling_DarkModMantlePhase || m_mantlePhase == fixClipping_DarkModMantlePhase)
			|| !m_mantleStartPossible )
	{
		return;
	}

	idPlayer* p_player = static_cast<idPlayer*>(self);
	if (p_player == NULL)
	{
		DM_LOG(LC_MOVEMENT, LT_ERROR)LOGSTRING("p_player is NULL\r");
		return;
	}

	if (p_player->GetImmobilization() & EIM_MANTLE)
	{
		return; // greebo: Mantling disabled by immobilization system
	}

	// Ishtvan: Do not attempt to mantle if holding an object
	if (cv_pm_mantle_while_carrying.GetInteger() == 0
	    && (p_player->IsShoulderingBody() || gameLocal.m_Grabber->GetSelected()))
	{
		return;
	}

	if (waterLevel >= WATERLEVEL_HEAD)
	{
		return; // STiFU: #1037: Do not mantle underwater
	}

	// Clear mantled entity members to indicate nothing is
	// being mantled
	m_p_mantledEntity = NULL;
	m_mantledEntityID = 0;

	// Forward vector is direction player is looking
	idVec3 forward = viewAngles.ToForward();
	forward.Normalize();

	// We use gravity alot here...
	const idVec3& gravityNormal = GetGravityNormal();

	// Get maximum reach distances for mantling
	float maxVerticalReachDistance; 
	float maxHorizontalReachDistance;
	float maxMantleTraceDistance;

	GetCurrentMantlingReachDistances
	(
		maxVerticalReachDistance,
		maxHorizontalReachDistance,
		maxMantleTraceDistance
	);

	// Get start position of gaze trace, which is player's eye position
	DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING ("Getting eye position\r");
	idVec3 eyePos = p_player->GetEyePosition();

	// Run mantle trace
	trace_t trace;

	MantleTargetTrace
	(
		maxMantleTraceDistance,
		eyePos,
		forward,
		trace
	);

	// If the trace found a target, see if it is mantleable
	if ( trace.fraction < 1.0f ) 
	{
		// mantle target found

		// Log trace hit point
		DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING
		(
			"Mantle target trace collision point (%f %f %f)\r", 
			trace.c.point.x,
			trace.c.point.y,
			trace.c.point.z
		);

		// Find mantle end point and make sure mantle is
		// possible
		idVec3 mantleEndPoint;
		EMantleable IsMantleable = ComputeMantlePathForTarget
		(
			maxVerticalReachDistance,
			maxHorizontalReachDistance,
			eyePos,
			trace,
			mantleEndPoint
		);
		if (IsMantleable > EMantleable_No)
		{
			// Mantle target passed mantleability tests

			// Log the end point
			DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING 
			(
				"Mantle end position = (%f %f %f)\r", 
				mantleEndPoint.x,
				mantleEndPoint.y,
				mantleEndPoint.z
			);

			// Start with mantle phase dependent on position relative
			// to the mantle end point
			const float mantleEndHeight = -(mantleEndPoint * gravityNormal);			
			float floorHeight = -FLT_MAX;
			{
				idVec3 floorPos;
				if (self->GetFloorPos(pm_normalviewheight.GetFloat(), floorPos))
					floorHeight = -floorPos * gravityNormal;
			}
			const float eyeHeight = -eyePos * gravityNormal;
			const float feetHeight = -(GetOrigin() * gravityNormal);
			const float obstacleHeight = mantleEndHeight - feetHeight;

			const bool bPullOrHang = eyeHeight < mantleEndHeight;
			const bool bFallingFast = 
				(current.velocity * gravityNormal) > 
				cv_pm_mantle_fallingFast_speedthreshold.GetFloat();

			// Daft Mugi #5892: While shouldering a body, allow mantling at approximately waist height.
			// Also, allow mantling while holding small objects at approximately waist height.
			if (cv_pm_mantle_while_carrying.GetInteger() == 1
				&& (p_player->IsShoulderingBody() || gameLocal.m_Grabber->GetSelected())
				&& (bPullOrHang // no pull or hang allowed
					|| obstacleHeight > cv_pm_mantle_maxShoulderingObstacleHeight.GetFloat() // restrict max height
					|| bFallingFast)) // must not be falling fast
			{
				return;
			}

			// If holding a small object and in midair, reduce allowed obstacle height.
			if (cv_pm_mantle_while_carrying.GetInteger() == 1
				&& gameLocal.m_Grabber->GetSelected()
				&& !groundPlane
				&& obstacleHeight > cv_pm_mantle_maxHoldingMidairObstacleHeight.GetFloat())
			{
				return;
			}

			if (cv_pm_mantle_fastLowObstaces.GetBool()) // STiFU #4930
			{
				if (   IsMantleable == EMantleable_YesUpstraight	// Upstraight mantle possible
					&& !bFallingFast
					&& (mantleEndHeight < floorHeight + cv_pm_mantle_maxLowObstacleHeight.GetFloat() // Only allow the full obstacle height when near the floor
					|| mantleEndHeight < feetHeight + cv_pm_mantle_maxLowObstacleHeight.GetFloat()*0.66f)) // Reduce allowed obstacle height midair
				{
					// Do a fast mantle over low obstacle
					StartMantle(pushNonCrouched_DarkModMantlePhase, eyePos, GetOrigin(), mantleEndPoint);
					return;
				}
			}
			if (cv_pm_mantle_fastMediumObstaclesCrouched.GetBool()) // STiFU #4945
			{
				// Use floorHeight instead of feetHeight to allow this mantle also when jump-mantling medium sized obstacles
				const bool bIsCrouched = current.movementFlags & PMF_DUCKED;

				if (   bIsCrouched
					&& !bFallingFast
					&& bPullOrHang // When endheight lower than eyes, use the regular push mantle
					&& mantleEndHeight < floorHeight + pm_normalviewheight.GetFloat())
				{
					// Do a fast pull-push mantle over medium sized obstacle
					StartMantle(pullFast_DarkModMantlePhase, eyePos, GetOrigin(), mantleEndPoint);
					return;
				}
			}
			if (bPullOrHang)
			{
				// Start with pull if on the ground, hang if not
				if (groundPlane)
				{
					StartMantle(pull_DarkModMantlePhase, eyePos, GetOrigin(), mantleEndPoint);
				}
				else
				{
					StartMantle(hang_DarkModMantlePhase, eyePos, GetOrigin(), mantleEndPoint);
				}
			}
			else
			{
				StartMantle(push_DarkModMantlePhase, eyePos, GetOrigin(), mantleEndPoint);
			}
		}
	}
}

//####################################################################
// End Mantle Mod
// SophisticatedZombie (DH)
//####################################################################

//####################################################################
// Start Leaning Mod
//	Zaccheus (some original geometric drawings)
//	SophisticatedZombie (DH) 
//
//####################################################################

void idPhysics_Player::ToggleLean(float leanYawAngleDegrees)
{
	idPlayer* pPlayer = static_cast<idPlayer*>(self);
	if (pPlayer == NULL)
	{
		DM_LOG(LC_MOVEMENT, LT_ERROR)LOGSTRING("pPlayer is NULL\r");
		return;
	}
	if (pPlayer->GetImmobilization() & EIM_LEAN)
		// If lean immobilization is set, do nothing!
		return;

	const bool isLeaning = m_CurrentLeanTiltDegrees >= 0.1f;
	const bool isSameDirection = m_leanYawAngleDegrees == leanYawAngleDegrees;
	const bool isStopping = m_leanMoveEndTilt == 0;

	const bool stopLeanOnKeyUp = !cv_tdm_toggle_lean.GetBool()
		&& isLeaning                     // already leaning
		&& isSameDirection               // only stop when same direction
		&& !isStopping;                  // not already stopping
	const bool stopLeanOnToggle = cv_tdm_toggle_lean.GetBool()
		&& isLeaning;                    // already leaning

	if (stopLeanOnKeyUp || stopLeanOnToggle)
	{
		UnLean(leanYawAngleDegrees);
		DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING("ToggleLean ending lean\r");
		return;
	}

	const bool isForwardLean = leanYawAngleDegrees == 90.0f;
	float angle = isForwardLean ? 8.0f : cv_pm_lean_angle.GetFloat();
	float standingViewHeight = pm_normalviewheight.GetFloat();
	float eyeHeight = pPlayer->EyeHeight();
	float eyeHeightDelta = standingViewHeight - eyeHeight;
	float crouchAngleAdjustment = (14 + (2 * (eyeHeight / standingViewHeight))) / 16.0f;
	float forwardAngleAdjustment = isForwardLean ? (15.0f/16.0f) : 1.0f;

	if (!isLeaning || (isStopping && isSameDirection))
	{
		// Start the lean
		m_leanMoveStartTilt = m_CurrentLeanTiltDegrees;
		m_leanYawAngleDegrees = leanYawAngleDegrees;

		m_leanTime = cv_pm_lean_time_to_lean.GetFloat();
		m_leanMoveEndTilt = angle
			* forwardAngleAdjustment
			* crouchAngleAdjustment;
		m_leanMoveMaxAngle = m_leanMoveEndTilt;

		m_b_leanFinished = false;

		DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING("ToggleLean starting lean\r");
	}
}

void idPhysics_Player::UnLean(float leanYawAngleDegrees)
{
	if (m_leanYawAngleDegrees != leanYawAngleDegrees)
	{
		// Not same direction
		return;
	}

	if (m_leanTime > 0 && m_leanMoveEndTilt == 0)
	{
		// Already un-leaning
		return;
	}

	// End the lean
	m_leanMoveStartTilt = m_CurrentLeanTiltDegrees;
	m_leanMoveEndTilt = 0.0;
	m_b_leanFinished = false;
	m_leanTime = cv_pm_lean_time_to_unlean.GetFloat();

	// greebo: Leave the rest of the variables as they are
	// to avoid view-jumping issues due to leaning back.
}

//----------------------------------------------------------------------

bool idPhysics_Player::IsLeaning()
{
	if (m_CurrentLeanTiltDegrees > 0.0f)
	{
		// entering, exiting, or holding lean
		return true;
	}
	else
	{
		return false;
	}
}	

//----------------------------------------------------------------------

const idAngles& idPhysics_Player::GetViewLeanAngles() const
{
	return m_viewLeanAngles;
}

//----------------------------------------------------------------------

idVec3 idPhysics_Player::GetViewLeanTranslation()
{
	idAngles viewAngNoPitch = viewAngles;
	viewAngNoPitch.pitch = 0.0f;
	return viewAngNoPitch.ToMat4() * m_viewLeanTranslation;
}

//----------------------------------------------------------------------

void idPhysics_Player::ProcessPeek(idEntity* peekEntity, idEntity* door, idVec3 normal) // grayman #4882
{
	const function_t* func = gameLocal.program.FindFunction("peekThread");
	if ( func != NULL )
	{
		idThread *pThread = new idThread(func);
		int n = pThread->GetThreadNum();
		pThread->CallFunctionArgs(func, true, "eev", peekEntity, door, &normal);
		pThread->DelayedStart(0);
		pThread->Execute();
	}
}

//----------------------------------------------------------------------

void idPhysics_Player::UpdateLeanAngle (float deltaLeanTiltDegrees)
{
	// What would the new lean angle be?
	float newLeanTiltDegrees = m_CurrentLeanTiltDegrees + deltaLeanTiltDegrees;

	DM_LOG(LC_MOVEMENT,LT_DEBUG)LOGSTRING("newLeanTiltDegrees = %f\r", newLeanTiltDegrees );

	if (newLeanTiltDegrees < 0.0)
	{
		// Adjust delta
		deltaLeanTiltDegrees = 0.0 - m_CurrentLeanTiltDegrees;
		m_leanTime = 0.0;
		m_b_leanFinished = true;
	}
	else if (newLeanTiltDegrees > m_leanMoveMaxAngle)
	{
		// Adjust delta
		deltaLeanTiltDegrees = m_leanMoveMaxAngle - m_CurrentLeanTiltDegrees;
		m_leanTime = 0.0;
		m_b_leanFinished = true;
	}

	// Log max possible lean
	DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING
	(
		"Currently leaning %.2f degrees, can lean up to %.2f more degrees this frame\r",
		m_CurrentLeanTiltDegrees,
		deltaLeanTiltDegrees
	);

	newLeanTiltDegrees = m_CurrentLeanTiltDegrees + deltaLeanTiltDegrees;

    // Collision test: do not change lean angles any more if collision has occurred
	// convert proposed angle to a viewpoint in space:
	idVec3 newPoint = LeanParmsToPoint( newLeanTiltDegrees );

	idPlayer* player = static_cast<idPlayer*>(self);
	idVec3 origPoint = player->GetEyePosition();

	// Add some delta so we can lean back afterwards without already being clipped
	idVec3 vDelta = newPoint - origPoint;
	vDelta.Normalize();

	float fLeanTestDelta = 6.0f;
	vDelta *= fLeanTestDelta;

	// Perform the trace (greebo: use PLAYERSOLID to include player_clip collisions)
	trace_t trTest;
	gameLocal.clip.TraceBounds( trTest, origPoint, newPoint + vDelta, m_LeanViewBounds, MASK_PLAYERSOLID, player );

	bool bWouldClip = trTest.fraction < 1.0f;
	//DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING("Collision trace between old view point ( %d, %d, %d ) and newPoint: ( %d, %d, %d )\r", origPoint.x, origPoint.y, origPoint.z, newPoint.x, newPoint.y, newPoint.z );

	// Do not lean farther if the player would hit the wall
	if ( bWouldClip )
	{
		idEntity* traceEnt = gameLocal.GetTraceEntity( trTest );

		DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING("Lean test point within solid, lean motion stopped.\r" );

		if ( traceEnt != NULL )
		{
			// Door leaning test
			if (traceEnt->IsType(CFrobDoor::Type))
			{
				if ( m_LeanEnt.GetEntity() == NULL ) // not set up yet?
				{
					CFrobDoor* door = static_cast<CFrobDoor*>(traceEnt);
					if ( (m_leanYawAngleDegrees == 0.0f) ||
					     (m_leanYawAngleDegrees == 180.0f) ||
					     (m_leanYawAngleDegrees == 90.0f) )
					{
						if ( !door->IsOpen() )
						{
							// can the door be listened through?
							if ( FindLeanListenPos(trTest.c.point) ) // grayman #4882
							{
								m_LeanEnt = door;
							}
						}
					}

					// grayman #4882 - If the door has a peek entity attached to it, and leaning forward, pass control to the peek code.

					// get the peek entity for the door we're leaning on, if it exists
					idEntityPtr<idPeek> peekEntity = door->GetDoorPeekEntity();

					if ( peekEntity.GetEntity() && (m_leanYawAngleDegrees == 90.0f) )
					{
						// Process the peek if the player's eyes are close enough to "see through".

						idVec3 peekOrigin = peekEntity.GetEntity()->GetPhysics()->GetOrigin(); // grayman #4882
						idVec3 playerEyeLocation = player->GetEyePosition(); // grayman #4882
						float distFromEye2PeakEntity = (playerEyeLocation - peekOrigin).LengthFast();

						if ( distFromEye2PeakEntity < PEEK_MAX_DIST )
						{
							ProcessPeek(peekEntity.GetEntity(), door, trTest.c.normal);
						}
					}
				}
			}
			// Detect AI collision
			else if (traceEnt->IsType(idAI::Type) )
			{
				static_cast<idAI*>(traceEnt)->HadTactile(player);
			}
			else
			{
				// Is a peek entity nearby? (Non-door situation, like a crack in a wall.)

				idPeek* peekEntity = NULL;
				if ( traceEnt->IsType(idPeek::Type) ) // did we hit the peek entity directly?
				{
					peekEntity = static_cast<idPeek*>(traceEnt);
				}
				else
				{
					idClip_EntityList entityList;
					int num;
					num = gameLocal.EntitiesWithinRadius(player->GetEyePosition(), PEEK_MAX_DIST, entityList);
					for ( int i = 0 ; i < num ; i++ )
					{
						idEntity *candidate = entityList[i];

						if ( candidate == NULL ) // just in case
						{
							continue;
						}

						if ( candidate->IsType(idPeek::Type) )
						{
							// grayman #4882 - If leaning forward, pass control to the peek code.

							// get the peek entity
							peekEntity = static_cast<idPeek*>(candidate);
						}
					}
				}

				if ( peekEntity && (m_leanYawAngleDegrees == 90.0f) )
				{
					// Process the peek if the player's eyes are close enough to "see through".

					idVec3 peekOrigin = peekEntity->GetPhysics()->GetOrigin(); // grayman #4882
					idVec3 playerEyeLocation = player->GetEyePosition(); // grayman #4882
					float distFromEye2PeekEntity = (playerEyeLocation - peekOrigin).LengthFast();

					if ( distFromEye2PeekEntity < PEEK_MAX_DIST )
					{
						ProcessPeek(peekEntity, NULL, trTest.c.normal);

						// Can listen while in a forward lean

						if ( FindLeanListenPos(trTest.c.point) ) // grayman #4882
						{
							m_LeanEnt = peekEntity;
						}
					}
				}
			}
		}

		return;
	}

	// Adjust lean angle by delta which was allowed
	m_CurrentLeanTiltDegrees += deltaLeanTiltDegrees;

	// Update the physics:
	UpdateLeanPhysics();
		
	// Log activity
	DM_LOG(LC_MOVEMENT, LT_DEBUG)LOGSTRING(
		"Lean tilt is now %.2f degrees\r", m_CurrentLeanTiltDegrees
	);
}

//----------------------------------------------------------------------

void idPhysics_Player::LeanMove()
{
	// Test for leaning immobilization
	idPlayer* pPlayer = static_cast<idPlayer*>(self);
	if (pPlayer == NULL)
	{
		DM_LOG(LC_MOVEMENT, LT_ERROR)LOGSTRING("pPlayer is NULL\r");
		return;
	}
	if (pPlayer->GetImmobilization() & EIM_LEAN)
	{
		// Cancel all leaning
		if (m_leanMoveEndTilt > 0.0f)
		{
			m_leanMoveStartTilt = m_CurrentLeanTiltDegrees;
			m_leanTime = cv_pm_lean_time_to_unlean.GetFloat();
			m_b_leanFinished = false;
			m_leanMoveEndTilt = 0.0f;
		}
	}

	// Change in lean tilt this frame
	float deltaLeanTiltDegrees = 0.0;
	float newLeanTiltDegrees = 0.0;

	if ( !m_b_leanFinished )
	{
		// Update lean time
		m_leanTime -= framemsec;
		if (m_leanTime <= 0.0)
		{
			m_leanTime = 0.0;
			m_b_leanFinished = true;
		}

		// Time ratio
		float t = (m_leanMoveEndTilt > 0.0f)
			? m_leanTime /  cv_pm_lean_time_to_lean.GetFloat()
			: m_leanTime /  cv_pm_lean_time_to_unlean.GetFloat();

		// Cubic bezier params
		float p[4] = {0.0f, 0.02f, 0.80f, 1.0f};

		// Cubic bezier curve movement
		float cb = (pow(1 - t, 3) * p[0])
			+ (3 * pow(1 - t, 2) * t * p[1])
			+ (3 * (1 - t) * pow(t, 2) * p[2])
			+ (pow(t, 3) * p[3]);
		cb = 1.0f - cb;

		if (m_leanMoveEndTilt > m_leanMoveStartTilt)
			newLeanTiltDegrees = (cb * (m_leanMoveEndTilt - m_leanMoveStartTilt)) + m_leanMoveStartTilt;
		else if (m_leanMoveStartTilt > m_leanMoveEndTilt)
			newLeanTiltDegrees = m_leanMoveStartTilt - (cb * (m_leanMoveStartTilt - m_leanMoveEndTilt));

		deltaLeanTiltDegrees = newLeanTiltDegrees - m_CurrentLeanTiltDegrees;
	}

	// Perform any change to leaning
	if (deltaLeanTiltDegrees != 0.0)
	{
		// Re-orient clip model before change so that collision tests
		// are accurate (player may have rotated mid-lean)
		UpdateLeanAngle(deltaLeanTiltDegrees);
	}

	// If player is leaned at all, do an additional clip test and unlean them
	// In case they lean and walk into something, or a moveable moves into them, etc.
	if (m_CurrentLeanTiltDegrees != 0.0 && TestLeanClip())
	{
		DM_LOG(LC_MOVEMENT,LT_DEBUG)LOGSTRING("Leaned player clipped solid, unleaning to valid position \r");
		UnleanToValidPosition();
	}

	// Lean door test
	if (IsLeaning())
	{
		UpdateLean();
	}
}

bool idPhysics_Player::TestLeanClip()
{
	idPlayer *p_player = static_cast<idPlayer*>(self);
	// convert proposed angle to a viewpoint in space:

	idVec3 vTest = p_player->GetEyePosition();
	idVec3 vEyeOffset = -GetGravityNormal()*p_player->EyeHeight();
	
	trace_t trTest;
	gameLocal.clip.TraceBounds( trTest, current.origin + vEyeOffset, vTest, m_LeanViewBounds, MASK_SOLID | CONTENTS_BODY, self );

	idEntity *TrEnt(NULL);

	// Detect AI collision, if entity hit or its bindmaster is an AI:
	if( trTest.fraction != 1.0f 
		&& ( TrEnt = gameLocal.GetTraceEntity( trTest ) ) != NULL
		&& TrEnt->IsType(idAI::Type) )
	{
		static_cast<idAI *>( TrEnt )->HadTactile( (idActor *) self );
	}

	// Uncomment for debug bounds display
	//gameRenderWorld->DebugBounds( colorGreen, m_LeanViewBounds, vTest ); 
	
	return (trTest.fraction != 1.0f);
}

idVec3 idPhysics_Player::LeanParmsToPoint(float tilt)
{
	idPlayer* p_player = static_cast<idPlayer*>(self);

	// Set slide distance - adjust for player height
	const bool isForwardLean = m_leanYawAngleDegrees == 90.0f;
	float standingViewHeight = pm_normalviewheight.GetFloat();
	float eyeHeight = p_player->EyeHeight();
	float eyeHeightDelta = standingViewHeight - eyeHeight;
	float slide = isForwardLean ? 200.0f : cv_pm_lean_slide.GetFloat();
	float slideDist = slide - eyeHeightDelta;

	// Set lean view angles
	float pitchAngle = tilt * idMath::Sin(DEG2RAD(m_leanYawAngleDegrees));
	float rollAngle = tilt * idMath::Cos(DEG2RAD(m_leanYawAngleDegrees));

	// This will be the point in space relative to the player's eye position
	idVec3 vPoint(
		slideDist * idMath::Sin(DEG2RAD(-pitchAngle)),
		slideDist * idMath::Sin(DEG2RAD(rollAngle)),
		0.0
	);

	// Calculate the z-coordinate of the point, by projecting it
	// onto a sphere of radius <slideDist>
	vPoint.ProjectSelfOntoSphere(slideDist);

	// Subtract the radius, we only need the difference relative to the eyepos
	vPoint.z -= slideDist;

	// Rotate to player's facing
	// this worked for yaw, but had issues with pitch, try something instead
	//idMat4 rotMat = viewAngles.ToMat4();
	idAngles viewAngNoPitch = viewAngles;
	viewAngNoPitch.pitch = 0.0f;
	idMat4 rotMat = viewAngNoPitch.ToMat4();

	vPoint *= rotMat;

	// Sign h4x0rx
	vPoint.x *= -1;
	vPoint.y *= -1;

	// Extract what the player's eye position would be without lean
	// Need to do this rather than just adding origin and eye offset due to smoothing
	vPoint += p_player->GetEyePosition() - rotMat * m_viewLeanTranslation;

	return vPoint;
}

void idPhysics_Player::RopeRemovalCleanup( idEntity *RopeEnt )
{
	if( RopeEnt && m_RopeEntity.GetEntity() && m_RopeEntity.GetEntity() == RopeEnt )
		m_RopeEntity = NULL;
	if( RopeEnt && m_RopeEntTouched.GetEntity() && m_RopeEntTouched.GetEntity() == RopeEnt )
		m_RopeEntTouched = NULL;
}

bool idPhysics_Player::CheckPushEntity(idEntity *entity) // grayman #4603
{
	return (entity == m_PushForce->GetPushEntity());
}

void idPhysics_Player::ClearPushEntity() // grayman #4603
{
	m_PushForce->SetPushEntity(NULL);
}

void idPhysics_Player::UpdateLeanPhysics()
{
	idPlayer *p_player = static_cast<idPlayer*>(self);

	idAngles viewAngNoPitch = viewAngles;
	viewAngNoPitch.pitch = 0;

	idMat4 rotPlayerToWorld = viewAngNoPitch.ToMat4();
	idMat4 rotWorldToPlayer = rotPlayerToWorld.Transpose();

	// unleaned player view origin
	idVec3 viewOrig = p_player->GetEyePosition();
	// convert angle to a viewpoint in space:
	idVec3 newPoint = LeanParmsToPoint( m_CurrentLeanTiltDegrees );

	// This is cumbersome, but it lets us extract the smoothed view origin from idPlayer
	m_viewLeanTranslation = newPoint - (viewOrig - rotPlayerToWorld * m_viewLeanTranslation);
	m_viewLeanTranslation *= rotWorldToPlayer;

	const bool isForwardLean = m_leanYawAngleDegrees == 90.0f;
	float lean_angle_mod = isForwardLean ? 0.5f : cv_pm_lean_angle_mod.GetFloat();

	float angle = m_CurrentLeanTiltDegrees * lean_angle_mod;

	m_viewLeanAngles.pitch = angle * idMath::Sin(DEG2RAD(m_leanYawAngleDegrees));
	m_viewLeanAngles.roll = angle * idMath::Cos(DEG2RAD(m_leanYawAngleDegrees));
}

float idPhysics_Player::GetDeltaViewYaw( void )
{
	return m_DeltaViewYaw;
}

float idPhysics_Player::GetDeltaViewPitch( void )
{
	return m_DeltaViewPitch;
}


bool idPhysics_Player::IsMidAir() const
{
	return m_bMidAir;
}

void idPhysics_Player::UpdateLeanedInputYaw( idAngles &InputAngles )
{
	if (!IsLeaning()) return; // nothing to do

	/**
	* Leaned view yaw check for clipping
	**/

	// Have a delta so that we don't get stuck on the wall due to floating point errors
	float AddedYawDelt = 4.0f; // amount to check ahead of the yaw change, in degrees
	float TestDeltaYaw = idMath::AngleNormalize180( InputAngles.yaw - viewAngles.yaw );

	// Add delta
	if( TestDeltaYaw < 0.0f )
		TestDeltaYaw -= AddedYawDelt;
	else
		TestDeltaYaw += AddedYawDelt;

	idPlayer* player = static_cast<idPlayer*>(self);

	idVec3 vEyeOffset = -GetGravityNormal() * player->EyeHeight();

	// make the test bounds go back to the unleaned eye point
	idBounds ViewBoundsExp = m_LeanViewBounds;
	ViewBoundsExp.AddPoint( -m_viewLeanTranslation );

	idClipModel ViewClip( ViewBoundsExp );
	idAngles viewAngYaw;
	viewAngYaw.Zero();
	viewAngYaw.yaw = viewAngles.yaw;

	ViewClip.SetPosition( player->GetEyePosition(), viewAngYaw.ToMat3() );

	//idRotation ViewYawRot( current.origin, -GetGravityNormal(), TestDeltaYaw );
	idRotation ViewYawRot( current.origin, GetGravityNormal(), TestDeltaYaw );

	idVec3 startPoint = player->GetEyePosition();
	idVec3 endPoint = startPoint;
	ViewYawRot.RotatePoint( endPoint );

	trace_t TrResults;
	gameLocal.clip.Rotation( TrResults, startPoint, ViewYawRot, &ViewClip, ViewClip.GetAxis(), MASK_SOLID | CONTENTS_BODY, self );

	DM_LOG(LC_MOVEMENT,LT_DEBUG)LOGSTRING("Leaned View Yaw Test: Original viewpoint (%f, %f, %f) Tested viewpoint: (%f, %f, %f) \r", startPoint.x, startPoint.y, startPoint.z, endPoint.x, endPoint.y, endPoint.z );

	// Cancel rotation if check-ahead rotation trace fails
	if( TrResults.fraction != 1.0f )
	{
		DM_LOG(LC_MOVEMENT,LT_DEBUG)LOGSTRING("Leaned View Yaw Test: Clipped with rotation trace fraction %f.  Delta yaw not allowed \r", TrResults.fraction );
		InputAngles.yaw = viewAngles.yaw;

		idEntity* TrEnt = gameLocal.GetTraceEntity( TrResults );

		// Detect AI collision, if entity hit or its bindmaster is an AI:
		if( TrEnt != NULL && TrEnt->IsType(idAI::Type) )
		{
			static_cast<idAI *>( TrEnt )->HadTactile( static_cast<idActor*>(self) );
		}
		
		return;
	}

	// debug draw the test clip model
	/*
	collisionModelManager->DrawModel( ViewClip.Handle(), ViewClip.GetOrigin(),
									ViewClip.GetAxis(), vec3_origin, 0.0f );
	*/
}

void idPhysics_Player::UnleanToValidPosition( void )
{
	trace_t trTest;
	idVec3 vTest(vec3_zero);

	idPlayer *p_player = (idPlayer *) self;
	idVec3 vEyeOffset = -GetGravityNormal()*p_player->EyeHeight();

	float TestLeanDegrees = m_CurrentLeanTiltDegrees;
	float DeltaDeg = TestLeanDegrees / (float) cv_pm_lean_to_valid_increments.GetInteger();

	// Must temporarily set these to get proper behavior from max angle
	m_leanMoveMaxAngle = m_CurrentLeanTiltDegrees;
	m_leanMoveStartTilt = m_CurrentLeanTiltDegrees;
	m_leanMoveEndTilt = 0.0f;

	for( int i=0; i < cv_pm_lean_to_valid_increments.GetInteger(); i++ )
	{
		// Lean degrees are always positive
		TestLeanDegrees -= DeltaDeg;

		// convert proposed angle to a viewpoint in world space:
		vTest = LeanParmsToPoint( TestLeanDegrees );
		gameLocal.clip.TraceBounds( trTest, current.origin + vEyeOffset, vTest, m_LeanViewBounds, MASK_SOLID | CONTENTS_BODY, self );

		// break if valid point was found
		if( trTest.fraction == 1.0f )
			break;
	}

	// transform lean parameters with final answer
	m_CurrentLeanTiltDegrees = TestLeanDegrees;
	
	UpdateLeanPhysics();
}

bool idPhysics_Player::FindLeanListenPos(const idVec3& incidencePoint) // grayman #4882
{
	bool bFoundEmptySpace( false );
	int contents = -1;
	idVec3 vTest(incidencePoint), vDirTest(0,0,0), vLeanDir( 1.0f, 0.0f, 0.0f );
	idAngles LeanYaw;
	idAngles viewYawOnly = viewAngles;
	
	LeanYaw.Zero();
	LeanYaw.yaw = m_leanYawAngleDegrees - 90.0f;

	viewYawOnly.pitch = 0.0f;
	viewYawOnly.roll = 0.0f;

	vDirTest = viewYawOnly.ToMat3() * LeanYaw.ToMat3() * vLeanDir;
	vDirTest.Normalize();

	int MaxCount = cv_pm_lean_door_increments.GetInteger();

	for( int count = 1; count < MaxCount; count++ )
	{
		vTest += vDirTest * ( (float) count / (float) MaxCount ) * cv_pm_lean_door_max.GetFloat();
		
		contents = gameLocal.clip.Contents( vTest, NULL, mat3_identity, CONTENTS_SOLID, self );
		
		// found empty space on other side of door
		if( !( (contents & MASK_SOLID) > 0 ) )
		{
			DM_LOG(LC_MOVEMENT,LT_DEBUG)LOGSTRING("Lean Into Door: Found empty space on other side of door.  Incidence point: %s Empty space point: %s \r", incidencePoint.ToString(), vTest.ToString() );
			
			bFoundEmptySpace = true;
			m_LeanListenPos = vTest;
			break;
		}
	}

	// uncomment to debug the lean direction line
	// gameRenderWorld->DebugArrow( colorBlue, IncidencePoint, IncidencePoint + 15.0f * vDirTest, 5.0f, 10000 );
	
	return bFoundEmptySpace;
}

void idPhysics_Player::UpdateLean( void ) // grayman #4882 - expanded to handle wall cracks
{
	idPlayer* player = static_cast<idPlayer*>(self);

	// If there's a peek entity, is it bound to a door?

	idEntity* e = m_LeanEnt.GetEntity();

	// Are we leaning against a door?
	if ( e && e->IsType(CFrobDoor::Type) )
	{
		CFrobDoor* door = static_cast<CFrobDoor*>(e);

		if ( player )
		{
			if ( !m_LeanEnt.IsValid() || door->IsOpen() || !IsLeaning() )
			{
				m_LeanEnt = NULL;
				return;
			}

			idBounds TestBounds = m_LeanViewBounds;
			TestBounds.ExpandSelf(cv_pm_lean_door_bounds_exp.GetFloat());
			TestBounds.TranslateSelf(player->GetEyePosition());

			/** More precise test (Not currently used)

			int numEnts = 0;
			idEntity *ent = NULL;
			bool bMatchedDoor(false);

			idClip_EntityList ents;
			numEnts = gameLocal.clip.EntitiesTouchingBounds( TestBounds, CONTENTS_SOLID, ents );
			for( int i=0; i < numEnts; i++ )
			{
			if( ents[i] == (idEntity *) door )
			{
			bMatchedDoor = true;
			break;
			}
			}

			if( !bMatchedDoor )
			{
			m_LeanEnt = NULL;
			goto Quit;
			}
			**/

			if ( !TestBounds.IntersectsBounds(door->GetPhysics()->GetAbsBounds()) )
			{
				m_LeanEnt = NULL;
				return;
			}

			// We are leaning into a door
			// overwrite the current player listener loc with that calculated for the door
			player->SetListenLoc(m_LeanListenPos);
		}
	}
	else if ( e && e->IsType(idPeek::Type) )// are we leaning near a peek entity that isn't a keyhole?
	{
		if ( player )
		{
			if ( !m_LeanEnt.IsValid() || !IsLeaning() )
			{
				m_LeanEnt = NULL;
				return;
			}

			idBounds TestBounds = m_LeanViewBounds;
			TestBounds.ExpandSelf(cv_pm_lean_door_bounds_exp.GetFloat());
			TestBounds.TranslateSelf(player->GetEyePosition());

			if ( !TestBounds.IntersectsBounds(e->GetPhysics()->GetAbsBounds()) )
			{
				m_LeanEnt = NULL;
				return;
			}

			player->SetListenLoc(m_LeanListenPos);
		}
	}
	// Obsttorte: #5899 
	else
	{
		if (player)
		{
			m_LeanEnt = NULL;
			player->SetListenLoc(vec3_zero);
		}
	}
}

bool idPhysics_Player::IsPeekLeaning( void )
{
	return (m_LeanEnt.GetEntity() != NULL) && m_LeanEnt.IsValid();
}

idStr idPhysics_Player::GetClimbSurfaceType() const
{
	return m_bOnClimb ? m_ClimbSurfName : "";
}

float idPhysics_Player::GetClimbLateralCoord(const idVec3& origVec) const
{
	if (m_bOnClimb)
	{
		idVec3 orig = origVec - (origVec * gravityNormal) * gravityNormal;

		idVec3 climbNormXY = m_vClimbNormal - (m_vClimbNormal * gravityNormal) * gravityNormal;
		idVec3 latNormal = climbNormXY.Cross(gravityNormal);
		latNormal.NormalizeFast();

		return orig * latNormal;
	}
	
	return 0.0f;
}

void idPhysics_Player::SetRefEntVel( idEntity *ent, int bodID)
{
	idVec3 EntCOM, ThisCOM, r, AngVel;
	idMat3 DummyMat;
	float dummy;

	if( !ent )
	{
		m_RefEntVelocity.Zero();
	}
	else
	{
		idPhysics *phys = ent->GetPhysics();

		// Sometimes we have statics bound to movers, and the static doesn't return a velocity
		if( phys->IsType(idPhysics_Static::Type) 
			|| phys->IsType(idPhysics_StaticMulti::Type) )
		{
			if( ent->GetBindMaster() )
			{
				phys = ent->GetBindMaster()->GetPhysics();
				// TODO: Replace this with ent->bindbody
				bodID = 0;
				DM_LOG(LC_MOVEMENT,LT_DEBUG)LOGSTRING("SetRefVel switched from ent %s to bindmaster %s\r", ent->name.c_str(), ent->GetBindMaster()->name.c_str() );
			}
		}

		m_RefEntVelocity = phys->GetLinearVelocity( bodID );
		// Apparently this is needed for moveables riding on movers:
		m_RefEntVelocity += phys->GetPushedLinearVelocity( bodID );

		// Add orbit velocity from angular vel of mover
		// v_tangential = omega X r, where r is the distance between COM
		AngVel = phys->GetAngularVelocity( bodID );
		AngVel += phys->GetPushedAngularVelocity( bodID );

		// Find our center of mass:
		GetClipModel()->GetMassProperties( 1.0f, dummy, ThisCOM, DummyMat );
		ThisCOM = GetOrigin() + ThisCOM * GetAxis();
		// Find entity center of mass
		if( phys->GetClipModel( bodID)->IsTraceModel() )
		{
			phys->GetClipModel( bodID )->GetMassProperties( 1.0f, dummy, EntCOM, DummyMat );
			EntCOM = phys->GetOrigin( bodID ) + EntCOM * phys->GetAxis( bodID );
		}
		else
			EntCOM = phys->GetOrigin( bodID );

		// r = ThisCOM - EntCOM;
		r = GetOrigin() - phys->GetOrigin( bodID );

// Linear component due to angular velocity commented out for now
// Is not correct and tends to kill the player.

//		m_RefEntVelocity += AngVel.Cross( r );

		// Uncomment for debugging
		// DM_LOG(LC_MOVEMENT,LT_DEBUG)LOGSTRING("SetRefEntVelocity: Ent angular velocity is %s, Ent COM is %s, player COM is %s, added linear velocity due to angular rotation of ent is: %s\r",AngVel.ToString(),EntCOM.ToString(),ThisCOM.ToString(),AngVel.Cross(r).ToString() ); 
	}
}

idVec3 idPhysics_Player::GetRefEntVel( void ) const
{
	return m_RefEntVelocity;
}

int idPhysics_Player::GetMovementFlags( void )
{
	return current.movementFlags;
}

void idPhysics_Player::SetMovementFlags( int flags )
{
	current.movementFlags = flags;
}


void idPhysics_Player::StartShouldering(idEntity const * const pBody)
{
	if (cv_pm_shoulderAnim_msecs.GetFloat() <= 0.0f)
		return;

	// Initialize
	if (m_eShoulderAnimState == eShoulderingAnimation_NotStarted)
	{
		if (pBody == nullptr)
		{
			DM_LOG(LC_MOVEMENT, LT_ERROR)LOGSTRING("Shouldering: pBody is NULL\r");
			return;
		}

		idPlayer* pPlayer = static_cast<idPlayer*>(self);
		if (pPlayer == nullptr)
		{
			DM_LOG(LC_MOVEMENT, LT_ERROR)LOGSTRING("Shouldering: pPlayer is NULL\r");
			return;
		}

		static const int iImmobilization =
			EIM_CLIMB | EIM_ITEM_SELECT | EIM_WEAPON_SELECT | EIM_ATTACK | EIM_ITEM_USE
			| EIM_MANTLE | EIM_FROB_COMPLEX | EIM_MOVEMENT | EIM_CROUCH_HOLD
			| EIM_CROUCH | EIM_JUMP | EIM_FROB | EIM_FROB_HILIGHT | EIM_LEAN;

		pPlayer->SetImmobilization("ShoulderingAnimation", iImmobilization);

		// Check height of body: If heigher than crouched, do not go to crouched state
		const float fBodyHeight = pBody->GetPhysics()->GetOrigin() * (-gravityNormal);
		const float fCrouchedHeight = GetOrigin() * (-gravityNormal) + pm_crouchviewheight.GetFloat();
		m_bShouldering_SkipDucking = fBodyHeight > fCrouchedHeight || cv_pm_shoulderAnim_delay_msecs.GetFloat() <= 0.0f;

		// Get rustle sound to play while shouldering
		const idKeyValue* const pKeyValue = pBody->spawnArgs.FindKey("snd_rustle");
		if (pKeyValue != nullptr)
		{
			pPlayer->spawnArgs.Set("snd_shouldering_rustle", pKeyValue->GetValue());
			m_fShouldering_TimeToNextSound = 0.0f;
		}
		else
		{
			// No appropriate rustle sound found. Skip playing rustle sound
			m_fShouldering_TimeToNextSound = FLT_MAX;
			pPlayer->spawnArgs.Set("snd_shouldering_rustle", "");
		}
				
		m_eShoulderAnimState = eShoulderingAnimation_Initialized;
	}

	StartShoulderingAnim();
}

bool idPhysics_Player::IsShouldering() const
{
	return m_eShoulderAnimState == eShoulderingAnimation_Active;
}

void idPhysics_Player::StartShoulderingAnim()
{
	// Try starting the animation
	if (m_eShoulderAnimState == eShoulderingAnimation_Initialized
		|| m_eShoulderAnimState == eShoulderingAnimation_Scheduled)
	{
		if (IsLeaning())
		{
			// Wait for unlean first
			m_eShoulderAnimState = eShoulderingAnimation_Scheduled;
		}
		else
		{
			// Start animation right away
			m_fShoulderingTime = cv_pm_shoulderAnim_msecs.GetFloat();
			m_fPrevShoulderingPitchOffset = 0.0f;
			m_PrevShoulderingPosOffset = vec3_zero;
			m_ShoulderingStartPosRelative = GetOrigin();
			
			// #6259: Make shouldering start pos relative to potentially moving ground entity
			idEntity* groundEnt = groundEntityPtr.GetEntity();
			if (groundEnt != NULL)
			{
				m_pShoulderingGroundEntity = groundEnt;
				idPhysics* p_physics = groundEnt->GetPhysics();
				if (p_physics != NULL)
				{
					m_ShoulderingStartPosRelative = 
						(m_ShoulderingStartPosRelative - p_physics->GetOrigin()) * p_physics->GetAxis().Transpose();
				}
			}
			m_ShoulderingCurrentPosRelative = m_ShoulderingStartPosRelative;
			
			m_eShoulderAnimState = eShoulderingAnimation_Active;
			if (!m_bShouldering_SkipDucking && !IsCrouching())
			{
				current.movementFlags |= PMF_DUCKED;
				
				// Reserve some additional time for going to ducked state
				// before playing the animation
				m_fShoulderingTime += cv_pm_shoulderAnim_delay_msecs.GetFloat();
			}
		}
	}
}

void idPhysics_Player::ShoulderingMove()
{
	if (m_eShoulderAnimState != eShoulderingAnimation_Active)
		return;

	idPlayer* pPlayer = static_cast<idPlayer*>(self);
	if (pPlayer == nullptr)
	{
		DM_LOG(LC_MOVEMENT, LT_ERROR)LOGSTRING("ShoulderingMove: pPlayer is NULL\r");

		// Cancel
		static_cast<idPlayer*>(self)->SetImmobilization("ShoulderingAnimation", 0);
		m_eShoulderAnimState = eShoulderingAnimation_NotStarted;
		return;
	}

	// Are we allowed to play the view animation already?
	if (m_fShoulderingTime <= cv_pm_shoulderAnim_msecs.GetFloat())
	{
		// Play a rustle sound if the time is up
		if (m_fShouldering_TimeToNextSound <= 0.0f)
		{
			const idKeyValue* const pKeyValue = pPlayer->spawnArgs.FindKey("snd_shouldering_rustle");
			if (pKeyValue != nullptr && pKeyValue->GetValue().Length() > 0)
			{
				int iLength_ms = 0;
				pPlayer->StartSound("snd_shouldering_rustle", SND_CHANNEL_ANY, SSF_GLOBAL, 0, &iLength_ms);
				m_fShouldering_TimeToNextSound = static_cast<float>(iLength_ms) * 0.5 * (1 + gameLocal.random.RandomFloat());
			}
			else
				m_fShouldering_TimeToNextSound = FLT_MAX;
		}

		// Compute view angles and position for lean
		const float fTimeRadians = idMath::PI * m_fShoulderingTime / cv_pm_shoulderAnim_msecs.GetFloat();
		idVec3 newPositionOffset = (idMath::Sin(fTimeRadians) * cv_pm_shoulderAnim_rockDist.GetFloat()) * viewForward;		
		const float fPitchOffset = idMath::Sin(fTimeRadians) * cv_pm_shoulderAnim_rockDist.GetFloat();

		// Add vertical dip animation
		const float fAbsoluteDipDuration = 
			cv_pm_shoulderAnim_dip_duration.GetFloat()*cv_pm_shoulderAnim_msecs.GetFloat();
		const float fDipStart =
			cv_pm_shoulderAnim_msecs.GetFloat()*0.5f + fAbsoluteDipDuration * 0.5f;
		const float fDipEnd = 
			cv_pm_shoulderAnim_msecs.GetFloat()*0.5f - fAbsoluteDipDuration * 0.5f;
		if (m_fShoulderingTime >= fDipEnd && m_fShoulderingTime < fDipStart)
		{
			const float fDipTimeRadians = idMath::PI * (fDipEnd - m_fShoulderingTime) / fAbsoluteDipDuration;
			newPositionOffset += (-idMath::Sin(fDipTimeRadians) * cv_pm_shoulderAnim_dip_dist.GetFloat()) * gravityNormal;
		}

		// Apply animation to player position and view angle
		idVec3 newPosition = m_ShoulderingCurrentPosRelative + (newPositionOffset - m_PrevShoulderingPosOffset);
		m_ShoulderingCurrentPosRelative = newPosition;
		m_PrevShoulderingPosOffset = newPositionOffset;

		if (m_pShoulderingGroundEntity != NULL)
		{
			// #6259: Shouldering animation must be relative to potentially moving ground entity
			idPhysics* p_physics = m_pShoulderingGroundEntity->GetPhysics();
			if (p_physics != NULL)
			{
				newPosition = p_physics->GetOrigin() + p_physics->GetAxis() * newPosition;
			}
		}

		SetOrigin(newPosition);

		viewAngles.pitch += (fPitchOffset - m_fPrevShoulderingPitchOffset);
		m_fPrevShoulderingPitchOffset = fPitchOffset;
		pPlayer->SetViewAngles(viewAngles);		
	}

	// Are we done?
	if (m_fShoulderingTime == 0.0f) // We explicitly set 0.0f below, so equality check with 0.0f is ok.
	{
		static_cast<idPlayer*>(self)->SetImmobilization("ShoulderingAnimation", 0);
		m_eShoulderAnimState = eShoulderingAnimation_NotStarted;

		// Explicitly return to start position to avoid clipping due to quantization errors
		idVec3 endPos = m_ShoulderingStartPosRelative;
		if (m_pShoulderingGroundEntity != NULL)
		{
			// #6259: Shouldering animation must be relative to potentially moving ground entity
			idPhysics* p_physics = m_pShoulderingGroundEntity->GetPhysics();
			if (p_physics != NULL)
			{
				endPos = p_physics->GetOrigin() + p_physics->GetAxis() * endPos;
			}
			m_pShoulderingGroundEntity = NULL;
		}
		SetOrigin(endPos);

		return;
	}

	// Update animation timer
	m_fShouldering_TimeToNextSound -= framemsec;
	m_fShoulderingTime -= framemsec;
	if (m_fShoulderingTime < 0.0f)
		m_fShoulderingTime = 0.0f;
}
