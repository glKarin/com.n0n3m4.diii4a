/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).  

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"


#ifdef STEAM
#include "../steamstats.h"
#endif


CLASS_DECLARATION( idPhysics_Actor, idPhysics_Player )
END_CLASS

// movement parameters
const float PM_STOPSPEED		= 100.0f;
const float PM_SWIMSCALE		= 0.5f;
const float PM_LADDERSPEED		= 100.0f;
const float PM_STEPSCALE		= 1.0f;

const float PM_ACCELERATE		= 10.0f;
const float PM_AIRACCELERATE	= 1.0f;
const float PM_WATERACCELERATE	= 4.0f;
const float PM_FLYACCELERATE	= 8.0f;

const float PM_FRICTION			= 6.0f;
const float PM_AIRFRICTION		= 0.0f;
const float PM_WATERFRICTION	= 1.0f;
const float PM_FLYFRICTION		= 6.0f; //bc was 3.0
const float PM_NOCLIPFRICTION	= 12.0f;

const float MIN_WALK_NORMAL		= 0.7f;		// can't walk on very steep slopes
const float OVERCLIP			= 1.001f;

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

const int JELLYBONE_ANGLE		= 30;
const int JELLYBONE_SPECTATOR_OFFSET = 28;

int c_pmove = 0;

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
	float addspeed, accelspeed, currentspeed;

	currentspeed = current.velocity * wishdir;
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0) {
		return;
	}
	accelspeed = accel * frametime * wishspeed;
	if (accelspeed > addspeed) {
		accelspeed = addspeed;
	}
	
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

#define LADDER_DISTANCE				24.0f
#define LADDER_DISENGAGETIME		400

bool idPhysics_Player::SlideMove( bool gravity, bool stepUp, bool stepDown, bool push ) {
	int			i, j, k, pushFlags;
	int			bumpcount, numbumps, numplanes;
	float		d, time_left, into, totalMass;
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

	for ( bumpcount = 0; bumpcount < numbumps; bumpcount++ ) {

		// calculate position we are trying to move to
		end = current.origin + time_left * current.velocity;

		// see if we can make it there
		gameLocal.clip.Translation( trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self );

		time_left -= time_left * trace.fraction;
		current.origin = trace.endpos;

		// if moved the entire distance
		if ( trace.fraction >= 1.0f ) {
			break;
		}

		stepped = pushed = false;

		// if we are allowed to step up
		if ( stepUp ) {

			nearGround = groundPlane | ladder;

			if ( !nearGround ) {
				// trace down to see if the player is near the ground
				// step checking when near the ground allows the player to move up stairs smoothly while jumping
				stepEnd = current.origin + maxStepHeight * gravityNormal;
				gameLocal.clip.Translation( downTrace, current.origin, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );
				nearGround = ( downTrace.fraction < 1.0f && (downTrace.c.normal * -gravityNormal) > MIN_WALK_NORMAL );
			}

			// may only step up if near the ground or on a ladder
			if ( nearGround ) {

				// step up
				stepEnd = current.origin - maxStepHeight * gravityNormal;
				gameLocal.clip.Translation( downTrace, current.origin, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );

				// trace along velocity
				stepEnd = downTrace.endpos + time_left * current.velocity;
				gameLocal.clip.Translation( stepTrace, downTrace.endpos, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );

				// step down
				stepEnd = stepTrace.endpos + maxStepHeight * gravityNormal;
				gameLocal.clip.Translation( downTrace, stepTrace.endpos, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );

				if ( downTrace.fraction >= 1.0f || (downTrace.c.normal * -gravityNormal) > MIN_WALK_NORMAL ) {

					// if moved the entire distance
					if ( stepTrace.fraction >= 1.0f ) {
						time_left = 0;
						current.stepUp -= ( downTrace.endpos - current.origin ) * gravityNormal;
						current.origin = downTrace.endpos;
						current.movementFlags |= PMF_STEPPED_UP;
						current.velocity *= PM_STEPSCALE;
						break;
					}

					// if the move is further when stepping up
					if ( stepTrace.fraction > trace.fraction ) {
						time_left -= time_left * stepTrace.fraction;
						current.stepUp -= ( downTrace.endpos - current.origin ) * gravityNormal;
						current.origin = downTrace.endpos;
						current.movementFlags |= PMF_STEPPED_UP;
						current.velocity *= PM_STEPSCALE;
						trace = stepTrace;
						stepped = true;
					}
				}
			}
		}

		// if we can push other entities and not blocked by the world
		if ( push && trace.c.entityNum != ENTITYNUM_WORLD ) {

			clipModel->SetPosition( current.origin, clipModel->GetAxis() );

			// clip movement, only push idMoveables, don't push entities the player is standing on
			// apply impact to pushed objects
			pushFlags = PUSHFL_CLIP|PUSHFL_ONLYMOVEABLE|PUSHFL_NOGROUNDENTITIES|PUSHFL_APPLYIMPULSE;

			// clip & push
			totalMass = gameLocal.push.ClipTranslationalPush( trace, self, pushFlags, end, end - current.origin );

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
		}

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
	if ( stepDown && groundPlane ) {
		stepEnd = current.origin + gravityNormal * maxStepHeight;
		gameLocal.clip.Translation( downTrace, current.origin, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );
		if ( downTrace.fraction > 1e-4f && downTrace.fraction < 1.0f ) {
			current.stepUp -= ( downTrace.endpos - current.origin ) * gravityNormal;
			current.origin = downTrace.endpos;
			current.movementFlags |= PMF_STEPPED_DOWN;
			current.velocity *= PM_STEPSCALE;
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



	idPlayer* localplayer = static_cast<idPlayer*>(self);
	if (bumpcount > 0 &&    (localplayer->PowerUpActive(ADRENALINE) || current.movementType == PM_SPECTATOR  )  )
	{
		//bc
		//Player is stopped by something.
		//check if can squeeze through.
		CheckJellyboning();
	}



	return (bool)( bumpcount == 0 );
}

void idPhysics_Player::UpdateJellyboning( void )
{
	float lerp;

	if (gameLocal.time >= this->clamberEndTime)
		lerp = 1;

	float currentTime = gameLocal.time;
	float lookTimeMax = this->jellyEndTime;
	float lookTimeMin = this->jellyStartTime;
			
	currentTime -= this->jellyStartTime;
	lookTimeMax -= this->jellyStartTime;

	lerp = currentTime / lookTimeMax;

	idVec3 newPosition;

	idVec3 startPosition;
	idVec3 endPosition;

	if (jellyState == 1)
	{
		startPosition = jellyOrigin0;
		endPosition = jellyOrigin1;


	}
	else
	{
		startPosition = jellyOrigin1;
		endPosition = jellyDestination;

	}

	newPosition.x = idMath::Lerp(startPosition.x, endPosition.x, lerp);
	newPosition.y = idMath::Lerp(startPosition.y, endPosition.y, lerp);
	newPosition.z = idMath::Lerp(startPosition.z, endPosition.z, lerp);

	if (lerp < 1)
	{
		newPosition.z += jelly_offset_z;

		if (current.movementType == PM_SPECTATOR)
		{
			newPosition.z += JELLYBONE_SPECTATOR_OFFSET;
		}
	}
	
	newPosition.z += 0.1f; //raise a little to prevent player from digging into ground (and turning sky black)

	SetOrigin(newPosition);

	if (lerp >= 1)
	{
		if (jellyState == 1)
		{
			jellyState = 2;

			float dist = (jellyDestination - jellyOrigin1).LengthFast();
			float movetime = (dist * .00625f) * 1000;

			//tweak jelly move time according to value on the jellypoint.
			movetime = movetime * jelly_timefactor;
			

			jellyStartTime = gameLocal.time;
			jellyEndTime = gameLocal.time + movetime;
		}
		else
		{
			current.velocity.Zero();
			jellyState = 0;
		}
	}
}

void idPhysics_Player::CheckJellyboning( void )
{
	if (jellyState != 0)
	{
		return;
	}

	if (nextJellyupdate > gameLocal.time)
	{
		return;		
	}

	nextJellyupdate = gameLocal.time + 200;



	int i;

	float closestDist = 9999;
	int closestIndex = -1;

	for ( i = 0; i < gameLocal.num_entities; i++ )
	{
		if (!gameLocal.entities[i])
			continue;

		if (!gameLocal.entities[ i ]->IsType(idJellyPoint::Type))
			continue;

		if (!gameLocal.entities[ i ]->spawnArgs.GetString("target0"))
			continue;		

		//distance check.
		float physicalDistance = (gameLocal.entities[ i ]->GetPhysics()->GetOrigin() -  this->PlayerGetOrigin() ).LengthFast();

		int checkDistance = 24;
		if (gameLocal.GetLocalPlayer()->isCasing)
			checkDistance = 90;

		if (physicalDistance > checkDistance)
			continue;

		trace_t trace;
		idVec3 p1 = this->PlayerGetOrigin();
		idVec3 p2 = gameLocal.entities[ i ]->GetPhysics()->GetOrigin();
		gameLocal.clip.TracePoint(trace, this->PlayerGetOrigin() + idVec3(0,0,1), gameLocal.entities[ i ]->GetPhysics()->GetOrigin()+ idVec3(0,0,1),  MASK_SOLID, self);

		if (trace.fraction < 0.95f)
			continue;

		if (physicalDistance < closestDist)
		{
			closestDist = physicalDistance;
			closestIndex = i;
		}
	}

	if (closestIndex < 0)
		return;


	//Found the start jellypoint.
		
	//Now find the destination jellypoint.
	const char *destName = gameLocal.entities[ closestIndex ]->spawnArgs.GetString("target0");

	idEntity *destEnt = gameLocal.FindEntity(destName);

	if (!destEnt)
		return;

	//bc CHECK IF DESTINATION IS IN FRONT OF PLAYER.
	float diff = idMath::AngleDelta( ( destEnt->GetPhysics()->GetOrigin() - PlayerGetOrigin() ).ToAngles().yaw, gameLocal.GetLocalPlayer()->viewAngles.yaw );

	if (diff <= JELLYBONE_ANGLE && diff >= -JELLYBONE_ANGLE)
	{
		//good.
	}
	else
	{
		//invalid. too extreme angle.
		return;
	}

	float dist = (gameLocal.entities[ closestIndex ]->GetPhysics()->GetOrigin() - PlayerGetOrigin()).LengthFast();
	float movetime = dist * 8.333f;

	

	if (movetime < 20)
	{
		movetime = 20;
	}


	//Found the destination jellypoint.
	jellyOrigin0 = PlayerGetOrigin();
	jellyOrigin1 = gameLocal.entities[ closestIndex ]->GetPhysics()->GetOrigin();
	jellyDestination = destEnt->GetPhysics()->GetOrigin();
	jellyStartTime = gameLocal.time;
	jellyEndTime = gameLocal.time + movetime;
	jellyState = 1;

	current.velocity.Zero();
	current.movementFlags |= PMF_DUCKED;

	idPlayer* localplayer = static_cast<idPlayer*>(self);
	localplayer->StartSound( "snd_exhale", SND_CHANNEL_ANY, 0, false, NULL );
	localplayer->StartSound( "snd_growl" , SND_CHANNEL_ANY, 0, false, NULL );

	jelly_offset_z = gameLocal.entities[ closestIndex ]->spawnArgs.GetInt("offset", "0");
	jelly_timefactor = gameLocal.entities[ closestIndex ]->spawnArgs.GetFloat("timefactor", "1");


		
}

void idPhysics_Player::ForceStand()
{
	current.velocity.Zero();
	current.movementFlags = 0;
}

/*
==================
idPhysics_Player::Friction

Handles both ground friction and water friction
==================
*/
void idPhysics_Player::Friction( void ) {
	idVec3	vel;
	float	speed, newspeed, control;
	float	drop;
	
	vel = current.velocity;
	if ( walking ) {
		// ignore slope movement, remove all velocity in gravity direction
		vel += (vel * gravityNormal) * gravityNormal;
	}

	speed = vel.Length();
	if ( speed < 1.0f ) {
		// remove all movement orthogonal to gravity, allows for sinking underwater
		if ( fabs( current.velocity * gravityNormal ) < 1e-5f ) {
			current.velocity.Zero();
		} else {
			current.velocity = (current.velocity * gravityNormal) * gravityNormal;
		}
		// FIXME: still have z friction underwater?
		return;
	}

	drop = 0;

	// spectator friction
	if ( current.movementType == PM_SPECTATOR ) {
		drop += speed * PM_FLYFRICTION * frametime;
	}
	// apply ground friction
	else if ( walking && waterLevel <= WATERLEVEL_FEET ) {
		// no friction on slick surfaces
		if ( !(groundMaterial && groundMaterial->GetSurfaceFlags() & SURF_SLICK) ) {
			// if getting knocked back, no friction
			if ( !(current.movementFlags & PMF_TIME_KNOCKBACK) ) {
				control = speed < PM_STOPSPEED ? PM_STOPSPEED : speed;
				drop += control * PM_FRICTION * frametime;
			}
		}
	}
	// apply water friction even if just wading
	else if ( waterLevel ) {
		drop += speed * PM_WATERFRICTION * waterLevel * frametime;
	}
	// apply air friction
	else {
		drop += speed * PM_AIRFRICTION * frametime;
	}

	// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0) {
		newspeed = 0;
	}
	current.velocity *= ( newspeed / speed );
}

/*
===================
idPhysics_Player::WaterJumpMove

Flying out of the water
===================
*/
void idPhysics_Player::WaterJumpMove( void ) {

	// waterjump has no control, but falls
	idPhysics_Player::SlideMove( true, true, false, false );

	// add gravity
	current.velocity += gravityNormal * frametime;
	// if falling down
	if ( current.velocity * gravityNormal > 0.0f ) {
		// cancel as soon as we are falling down again
		current.movementFlags &= ~PMF_ALL_TIMES;
		current.movementTime = 0;
	}
}

/*
===================
idPhysics_Player::WaterMove
===================
*/
void idPhysics_Player::WaterMove( void ) {
	idVec3	wishvel;
	float	wishspeed;
	idVec3	wishdir;
	float	scale;
	float	vel;

	if ( idPhysics_Player::CheckWaterJump() ) {
		idPhysics_Player::WaterJumpMove();
		return;
	}

	idPhysics_Player::Friction();

	scale = idPhysics_Player::CmdScale( command );

	// user intentions
	if ( !scale ) {
		wishvel = gravityNormal * 60; // sink towards bottom
	} else {
		wishvel = scale * (viewForward * command.forwardmove + viewRight * command.rightmove);
		wishvel -= scale * gravityNormal * command.upmove;
	}

	wishdir = wishvel;
	wishspeed = wishdir.Normalize();

	if ( wishspeed > playerSpeed * PM_SWIMSCALE ) {
		wishspeed = playerSpeed * PM_SWIMSCALE;
	}

	idPhysics_Player::Accelerate( wishdir, wishspeed, PM_WATERACCELERATE );

	// make sure we can go up slopes easily under water
	if ( groundPlane && ( current.velocity * groundTrace.c.normal ) < 0.0f ) {
		vel = current.velocity.Length();
		// slide along the ground plane
		current.velocity.ProjectOntoPlane( groundTrace.c.normal, OVERCLIP );

		current.velocity.Normalize();
		current.velocity *= vel;
	}

	idPhysics_Player::SlideMove( false, true, false, false );
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
	idPhysics_Player::Friction();

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

	idPhysics_Player::Friction();

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
	idPhysics_Player::Accelerate( wishdir, wishspeed, PM_AIRACCELERATE );

	// we may have a ground plane that is very steep, even
	// though we don't have a groundentity
	// slide along the steep plane
	if ( groundPlane ) {
		current.velocity.ProjectOntoPlane( groundTrace.c.normal, OVERCLIP );
	}

	idPhysics_Player::SlideMove( true, false, false, false );
}

/*
===================
idPhysics_Player::WalkMove
===================
*/
void idPhysics_Player::WalkMove( void ) {
	idVec3		wishvel;
	idVec3		wishdir;
	float		wishspeed;
	float		scale;
	float		accelerate;
	idVec3		oldVelocity, vel;
	float		oldVel, newVel;

	if ( waterLevel > WATERLEVEL_WAIST && ( viewForward * groundTrace.c.normal ) > 0.0f ) {
		// begin swimming
		idPhysics_Player::WaterMove();
		return;
	}

	if ( idPhysics_Player::CheckJump() ) {
		// jumped away
		if ( waterLevel > WATERLEVEL_FEET ) {
			idPhysics_Player::WaterMove();
		}
		else {
			idPhysics_Player::AirMove();
		}
		return;
	}

	idPhysics_Player::Friction();

	scale = idPhysics_Player::CmdScale( command );

	// project moves down to flat plane
	viewForward -= (viewForward * gravityNormal) * gravityNormal;
	viewRight -= (viewRight * gravityNormal) * gravityNormal;

	// project the forward and right directions onto the ground plane
	viewForward.ProjectOntoPlane( groundTrace.c.normal, OVERCLIP );
	viewRight.ProjectOntoPlane( groundTrace.c.normal, OVERCLIP );
	//
	viewForward.Normalize();
	viewRight.Normalize();

	wishvel = viewForward * command.forwardmove + viewRight * command.rightmove;
	wishdir = wishvel;
	wishspeed = wishdir.Normalize();
	wishspeed *= scale;

	// clamp the speed lower if wading or walking on the bottom
	if ( waterLevel ) {
		float	waterScale;

		waterScale = waterLevel / 3.0f;
		waterScale = 1.0f - ( 1.0f - PM_SWIMSCALE ) * waterScale;
		if ( wishspeed > playerSpeed * waterScale ) {
			wishspeed = playerSpeed * waterScale;
		}
	}

	// when a player gets hit, they temporarily lose full control, which allows them to be moved a bit
	if ( ( groundMaterial && groundMaterial->GetSurfaceFlags() & SURF_SLICK ) || current.movementFlags & PMF_TIME_KNOCKBACK ) {
		accelerate = PM_AIRACCELERATE;
	}
	else {
		accelerate = PM_ACCELERATE;
	}

	idPhysics_Player::Accelerate( wishdir, wishspeed, accelerate );

	if ( ( groundMaterial && groundMaterial->GetSurfaceFlags() & SURF_SLICK ) || current.movementFlags & PMF_TIME_KNOCKBACK ) {
		current.velocity += gravityVector * frametime;
	}

	oldVelocity = current.velocity;

	// slide along the ground plane
	current.velocity.ProjectOntoPlane( groundTrace.c.normal, OVERCLIP );

	// if not clipped into the opposite direction
	if ( oldVelocity * current.velocity > 0.0f ) {
		newVel = current.velocity.LengthSqr();
		if ( newVel > 1.0f ) {
			oldVel = oldVelocity.LengthSqr();
			if ( oldVel > 1.0f ) {
				// don't decrease velocity when going up or down a slope
				current.velocity *= idMath::Sqrt( oldVel / newVel );
			}
		}
	}

	// don't do anything if standing still
	vel = current.velocity - (current.velocity * gravityNormal) * gravityNormal;
	if ( !vel.LengthSqr() ) {
		return;
	}

	gameLocal.push.InitSavingPushedEntityPositions();

	idPhysics_Player::SlideMove( false, true, true, true );
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
	float		speed, drop, friction, newspeed, stopspeed;
	float		scale, wishspeed;
	idVec3		wishdir;

	// friction
	speed = current.velocity.Length();
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
	}

	// accelerate
	scale = idPhysics_Player::CmdScale( command );

	wishdir = scale * (viewForward * command.forwardmove + viewRight * command.rightmove);
	wishdir -= scale * gravityNormal * command.upmove;
	wishspeed = wishdir.Normalize();
	wishspeed *= scale;

	idPhysics_Player::Accelerate( wishdir, wishspeed, PM_ACCELERATE );

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

	trace_t	trace;
	idVec3	end;
	idBounds bounds;

	//bounds = clipModel->GetBounds();
	bounds = idBounds(idVec3(-8,-8,0), idVec3(8,8,8));
	//bounds[1][2] = 8;
	clipModel->LoadModel( idTraceModel( bounds ) );


	if (jellyState > 0)
	{
		UpdateJellyboning();
		return;
	}


	// fly movement

	idPhysics_Player::Friction();

	scale = idPhysics_Player::CmdScale( command );

	if ( !scale ) {
		wishvel = vec3_origin;
	} else {
		wishvel = scale * (viewForward * command.forwardmove + viewRight * command.rightmove);

		//bc
		wishvel *= 2.0f;
	}

	wishdir = wishvel;
	wishspeed = wishdir.Normalize();


	if ( command.upmove != 0 )
	{
		float jumpFactor = 1.0f;
		idVec3 addVelocity;		

		if ( command.upmove < 0 )
			jumpFactor *= -1.0f;
		
		addVelocity = jumpFactor * -gravityVector;
		addVelocity *= idMath::Sqrt( addVelocity.Normalize() );
		current.velocity += addVelocity;
	}



	idPhysics_Player::Accelerate( wishdir, wishspeed, PM_FLYACCELERATE );

	idPhysics_Player::SlideMove( false, false, false, false );
}

/*
============
idPhysics_Player::LadderMove
============
*/
void idPhysics_Player::LadderMove( void ) {
	idVec3	wishdir, wishvel, right;
	float	wishspeed, scale;
	float	upscale;

	// stick to the ladder
	wishvel = -100.0f * ladderNormal;
	current.velocity = (gravityNormal * current.velocity) * gravityNormal + wishvel;

	upscale = (-gravityNormal * viewForward + 0.5f) * 2.5f;
	if ( upscale > 1.0f ) {
		upscale = 1.0f;
	}
	else if ( upscale < -1.0f ) {
		upscale = -1.0f;
	}

	scale = idPhysics_Player::CmdScale( command );
	wishvel = -0.9f * gravityNormal * upscale * scale * (float)command.forwardmove;

	// strafe
	if ( command.rightmove ) {
		// right vector orthogonal to gravity
		right = viewRight - (gravityNormal * viewRight) * gravityNormal;
		// project right vector into ladder plane
		right = right - (ladderNormal * right) * ladderNormal;
		right.Normalize();

		// if we are looking away from the ladder, reverse the right vector
		if ( ladderNormal * viewForward > 0.0f ) {
			right = -right;
		}
		wishvel += 2.0f * right * scale * (float) command.rightmove;
	}

	// up down movement
	
	if ( command.upmove > 0 )
	{
		//wishvel += -0.5f * gravityNormal * scale * (float) command.upmove;
		//idVec3 newPosition;

		//newPosition = current.origin + ( ladderNormal * ( LADDER_DISTANCE + 2 ) );
		this->ladder = false;

		idVec3 forward = viewAngles.ToForward();
		forward.x = 0;
		forward.z = 0;
		current.velocity = (72.0f * forward) + ( 64.0f * this->ladderNormal );


		this->nextLadderTime = gameLocal.time + LADDER_DISENGAGETIME;

		//SetOrigin( newPosition );
		return;
	}

	// do strafe friction
	idPhysics_Player::Friction();

	// accelerate
	wishspeed = wishvel.Normalize();
	idPhysics_Player::Accelerate( wishvel, wishspeed, PM_ACCELERATE );

	float ladderspeed = PM_LADDERSPEED;
	idPlayer* localplayer = static_cast<idPlayer*>(self);
	if (localplayer->PowerUpActive(ADRENALINE))
	{
		ladderspeed *= 64.0f;
	}


	// cap the vertical velocity
	upscale = current.velocity * -gravityNormal;
	if ( upscale < -ladderspeed ) {
		current.velocity += gravityNormal * (upscale + ladderspeed);
	}
	else if ( upscale > ladderspeed ) {
		current.velocity += gravityNormal * (upscale - ladderspeed);
	}

	if ( (wishvel * gravityNormal) == 0.0f ) {
		if ( current.velocity * gravityNormal < 0.0f ) {
			current.velocity += gravityVector * frametime;
			if ( current.velocity * gravityNormal > 0.0f ) {
				current.velocity -= (gravityNormal * current.velocity) * gravityNormal;
			}
		}
		else {
			current.velocity -= gravityVector * frametime;
			if ( current.velocity * gravityNormal < 0.0f ) {
				current.velocity -= (gravityNormal * current.velocity) * gravityNormal;
			}
		}
	}

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
	idVec3 point;
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
	if ( contents & MASK_SOLID ) {
		// do something corrective if stuck in solid
		idPhysics_Player::CorrectAllSolid( groundTrace, contents );
	}

	// if the trace didn't hit anything, we are in free fall
	if ( groundTrace.fraction == 1.0f ) {
		groundPlane = false;
		walking = false;
		groundEntityPtr = NULL;
		return;
	}

	groundMaterial = groundTrace.c.material;
	groundEntityPtr = gameLocal.entities[ groundTrace.c.entityNum ];

	// check if getting thrown off the ground
	if ( (current.velocity * -gravityNormal) > 0.0f && ( current.velocity * groundTrace.c.normal ) > 10.0f ) {
		if ( debugLevel ) {
			gameLocal.Printf( "%i:kickoff\n", c_pmove );
		}

		groundPlane = false;
		walking = false;
		return;
	}
	
	// slopes that are too steep will not be considered onground
	if ( ( groundTrace.c.normal * -gravityNormal ) < MIN_WALK_NORMAL ) {
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

	groundPlane = true;
	walking = true;

	// hitting solid ground will end a waterjump
	if ( current.movementFlags & PMF_TIME_WATERJUMP ) {
		current.movementFlags &= ~( PMF_TIME_WATERJUMP | PMF_TIME_LAND );
		current.movementTime = 0;
	}

	// if the player didn't have ground contacts the previous frame
	if ( !hadGroundContacts ) {

		// don't do landing time if we were just going down a slope
		if ( (current.velocity * -gravityNormal) < -200.0f ) {
			// don't allow another jump for a little while
			current.movementFlags |= PMF_TIME_LAND;
			current.movementTime = 250;
		}
	}

	// let the entity know about the collision
	self->Collide( groundTrace, current.velocity );

	if ( groundEntityPtr.GetEntity() ) {
		impactInfo_t info;
		groundEntityPtr.GetEntity()->GetImpactInfo( self, groundTrace.c.id, groundTrace.c.point, &info );
		if ( info.invMass != 0.0f ) {
			groundEntityPtr.GetEntity()->ApplyImpulse( self, groundTrace.c.id, groundTrace.c.point, current.velocity / ( info.invMass * 10.0f ) );
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
	trace_t	trace;
	idVec3 end;
	idBounds bounds;
	float maxZ;

	if ( current.movementType == PM_DEAD ) {
		maxZ = pm_deadheight.GetFloat();
	} else {
		// stand up when up against a ladder
		if ( command.upmove < 0 && !ladder ) {
			// duck
			current.movementFlags |= PMF_DUCKED;
		}
		else if (this->clamberState <= 0 && this->jellyState <= 0)
		{
			// stand up if possible
			if ( current.movementFlags & PMF_DUCKED ) {
				// try to stand up
				end = current.origin - ( pm_normalheight.GetFloat() - pm_crouchheight.GetFloat() ) * gravityNormal;
				gameLocal.clip.Translation( trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self );
				if ( trace.fraction >= 1.0f ) {
					current.movementFlags &= ~PMF_DUCKED;
				}
			}
		}

		if ( current.movementFlags & PMF_DUCKED ) {
			playerSpeed = crouchSpeed;
			maxZ = pm_crouchheight.GetFloat();
		} else {
			maxZ = pm_normalheight.GetFloat();
		}
	}
	// if the clipModel height should change
	if ( clipModel->GetBounds()[1][2] != maxZ ) {

		bounds = clipModel->GetBounds();
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
idPhysics_Player::CheckLadder
================
*/
void idPhysics_Player::CheckLadder( void ) {
	idVec3		forward, start, end;
	trace_t		trace;
	float		tracedist;
	
	if ( current.movementTime ) {
		return;
	}

	// if on the ground moving backwards
	if ( walking && command.forwardmove <= 0 ) {
		return;
	}

	if ( gameLocal.time < this->nextLadderTime )
	{
		return;
	}

	// forward vector orthogonal to gravity
	forward = viewForward - (gravityNormal * viewForward) * gravityNormal;
	forward.Normalize();

	if ( walking ) {
		// don't want to get sucked towards the ladder when still walking
		tracedist = 1.0f;
	} else {
		tracedist = LADDER_DISTANCE;
	}

	end = current.origin + tracedist * forward;
	gameLocal.clip.Translation( trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self );

	// if near a surface
	if ( trace.fraction < 1.0f ) {

		// if a ladder surface
		if ( trace.c.material && ( trace.c.material->GetSurfaceFlags() & SURF_LADDER ) ) {

			// check a step height higher
			end = current.origin - gravityNormal * ( maxStepHeight * 0.75f );
			gameLocal.clip.Translation( trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self );
			start = trace.endpos;
			end = start + tracedist * forward;
			gameLocal.clip.Translation( trace, start, end, clipModel, clipModel->GetAxis(), clipMask, self );

			// if also near a surface a step height higher
			if ( trace.fraction < 1.0f ) {

				// if it also is a ladder surface
				if ( trace.c.material && trace.c.material->GetSurfaceFlags() & SURF_LADDER ) {
					ladder = true;
					ladderNormal = trace.c.normal;
				}
			}
		}
	}
}

/*
=============
idPhysics_Player::CheckJump
=============
*/
bool idPhysics_Player::CheckJump( void ) {
	idVec3 addVelocity;

	if ( command.upmove < 10 ) {
		// not holding jump
		return false;
	}

	// must wait for jump to be released
	if ( current.movementFlags & PMF_JUMP_HELD ) {
		return false;
	}

	// don't jump if we can't stand up
	if ( current.movementFlags & PMF_DUCKED ) {
		return false;
	}

	groundPlane = false;		// jumping away
	walking = false;
	current.movementFlags |= PMF_JUMP_HELD | PMF_JUMPED;


	//greaseman jump.
	float jumpFactor = 2.0f;
	idPlayer* localplayer = static_cast<idPlayer*>(self);
	if (localplayer->PowerUpActive(ADRENALINE))
	{
		jumpFactor = 4.0f;
	}

	addVelocity = jumpFactor * maxJumpHeight * -gravityVector;
	addVelocity *= idMath::Sqrt( addVelocity.Normalize() );
	current.velocity += addVelocity;

	//bc
	this->inJump = true;

	return true;
}

/*
=============
idPhysics_Player::CheckWaterJump
=============
*/
bool idPhysics_Player::CheckWaterJump( void ) {
	idVec3	spot;
	int		cont;
	idVec3	flatforward;

	if ( current.movementTime ) {
		return false;
	}

	// check for water jump
	if ( waterLevel != WATERLEVEL_WAIST ) {
		return false;
	}

	flatforward = viewForward - (viewForward * gravityNormal) * gravityNormal;
	flatforward.Normalize();

	spot = current.origin + 30.0f * flatforward;
	spot -= 4.0f * gravityNormal;
	cont = gameLocal.clip.Contents( spot, NULL, mat3_identity, -1, self );
	if ( !(cont & CONTENTS_SOLID) ) {
		return false;
	}

	spot -= 16.0f * gravityNormal;
	cont = gameLocal.clip.Contents( spot, NULL, mat3_identity, -1, self );
	if ( cont ) {
		return false;
	}

	// jump out of water
	current.velocity = 200.0f * viewForward - 350.0f * gravityNormal;
	current.movementFlags |= PMF_TIME_WATERJUMP;
	current.movementTime = 2000;

	return true;
}

/*
=============
idPhysics_Player::SetWaterLevel
=============
*/
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

float idPhysics_Player::GetClamberLerp( void )
{
	if (gameLocal.time >= this->clamberEndTime)
		return 1;

	float currentTime = gameLocal.time;
	float lookTimeMax = this->clamberEndTime;
	float lookTimeMin = this->clamberStartTime;
			
	currentTime -= this->clamberStartTime;
	lookTimeMax -= this->clamberStartTime;

	return (currentTime / lookTimeMax);
}

//bc
idVec3 idPhysics_Player::GetClamberPosCubby( void )
{
	idVec3		eyePos;
	idVec3		forward;
	idPlayer*	player;
	trace_t		upCheck, downCheck, forwardCheck;
	float		clearanceHeight;

	player = static_cast<idPlayer*>(self);
	if (player == NULL)
		return idVec3(0,0,0);

	eyePos = player->GetEyePosition();
	forward = viewAngles.ToForward();
	forward.Normalize();
	
	gameLocal.clip.TracePoint(forwardCheck, eyePos, eyePos + (forward * 48), MASK_SOLID|CONTENTS_FLASHLIGHT_TRIGGER, self);

	/*
	gameRenderWorld->DebugArrow( colorWhite,  eyePos, forwardCheck.endpos, 2, 5000);
	if (forwardCheck.fraction < 1)
		gameRenderWorld->DebugCircle( colorWhite,  forwardCheck.endpos   , idVec3(0,0,1), 4, 8, 5000 );
	*/

	gameLocal.clip.TracePoint(downCheck, forwardCheck.endpos, forwardCheck.endpos + idVec3(0,0,-80), MASK_SOLID|CONTENTS_FLASHLIGHT_TRIGGER, self);


	//check if the clamberobject is the thing being held.
	idPlayer* localplayer = static_cast<idPlayer*>(self);
	if (localplayer->pickerState >= 2)
	{
		int tracenum = downCheck.c.entityNum;
		int frobnum = localplayer->pickerWeapon.dragEnt.GetEntity()->entityNumber;

		if ( tracenum == frobnum )
		{
			return idVec3(0,0,0);
		}
	}


	//check ground plane.
	if (downCheck.fraction >= 1 || downCheck.endpos.z <= current.origin.z)
		return idVec3(0,0,0);

	gameLocal.clip.TracePoint(upCheck, forwardCheck.endpos, forwardCheck.endpos + idVec3(0,0,80), MASK_SOLID|CONTENTS_FLASHLIGHT_TRIGGER, self);

	//get clearance height to check whether player can fit.
	clearanceHeight = upCheck.endpos.z - downCheck.endpos.z;
	if (clearanceHeight < (pm_crouchheight.GetFloat() + 0.5f))
		return idVec3(0,0,0);

	//bc 2-17-2016
	//prevent clambering onto slanted surfaces. this is to prevent weirdness when bunnyhopping up ramps.
	if (!CheckClamberNormal(downCheck))
	{
		return idVec3(0,0,0);
	}




	if (downCheck.c.entityNum != ENTITYNUM_WORLD)
	{
		clamberEnt = gameLocal.entities[ downCheck.c.entityNum ];
	}
	else
	{
		clamberEnt = NULL;
	}


	return CheckClamberBounds( downCheck.endpos );
}


//return TRUE if it's okay to clamber here. return FALSE if it's a bad spot.
bool	idPhysics_Player::CheckClamberNormal( trace_t trace )
{
	trace_t eyeTrace;
	gameLocal.clip.TracePoint(eyeTrace, gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() + idVec3(0,0,0.1f), trace.endpos + idVec3(0,0,0.1f), MASK_SOLID, self);

	//if (eyeTrace.fraction < 1)
	//	common->Printf("eye trace %f\n", eyeTrace.fraction);

	//if the clamberspot is sloped AND player's feet have LOS to the clamberspot THEN it return *INVALID*

	if ((idMath::Fabs(trace.c.normal.x) > 0.1f || idMath::Fabs(trace.c.normal.y) > 0.1f) && eyeTrace.fraction >= 1)
	{
		
		return false;
	}

	return true;
}

idVec3 idPhysics_Player::GetClamberPos( void )
{
	trace_t		verticalCheck1, forwardCheck1, landingCheck1;
	idVec3		forward;
	idAngles	viewAng;
	float		landingTraceDist;
	int			i;
	bool		foundLedge = false;

	viewAng = this->viewAngles;
	viewAng.pitch = 0;
	viewAng.roll = 0;
	forward = viewAng.ToForward();
	forward.Normalize();


	int clamberHeight = 140;
	idPlayer* localplayer = static_cast<idPlayer*>(self);
	if (localplayer->PowerUpActive(ADRENALINE))
	{
		clamberHeight = 320;
	}

	gameLocal.clip.TracePoint(verticalCheck1, current.origin, current.origin + idVec3(0,0,clamberHeight),
		MASK_SOLID|CONTENTS_FLASHLIGHT_TRIGGER, self);

	landingTraceDist = idMath::Fabs(verticalCheck1.endpos.z - current.origin.z) * -1.f;

	if (developer.GetInteger() >= 2)
	{
		gameRenderWorld->DebugArrow(colorCyan, current.origin, verticalCheck1.endpos, 4, 9000);
	}

	for ( i = 0; i < 3; i++ )
	{
		float forwardCheckDist;

		if (i == 0)
			forwardCheckDist = 32;
		else if (i == 1)
			forwardCheckDist = 48;
		else
			forwardCheckDist = 64;

		gameLocal.clip.TracePoint(forwardCheck1, verticalCheck1.endpos, verticalCheck1.endpos + (forward * forwardCheckDist),
			MASK_SOLID|CONTENTS_FLASHLIGHT_TRIGGER, self);

		gameLocal.clip.TracePoint(landingCheck1, forwardCheck1.endpos, forwardCheck1.endpos + idVec3(0,0,landingTraceDist),
			MASK_SOLID|CONTENTS_FLASHLIGHT_TRIGGER, self);


		if (developer.GetInteger() >= 2)
		{
			gameRenderWorld->DebugArrow(colorGreen, verticalCheck1.endpos, forwardCheck1.endpos, 4, 9000);
			gameRenderWorld->DebugArrow(colorCyan, forwardCheck1.endpos, landingCheck1.endpos, 4, 9000);
		}


		//check if the clamberobject is the thing being held.
		idPlayer* localplayer = static_cast<idPlayer*>(self);
		if (localplayer->pickerState >= 2)
		{
			int tracenum = landingCheck1.c.entityNum;
			int frobnum = localplayer->pickerWeapon.dragEnt.GetEntity()->entityNumber;

			if ( tracenum == frobnum )
			{
				foundLedge = false;
				continue;
			}
		}

		if (landingCheck1.fraction < 1)
		{
			foundLedge = true;
			break;
		}
	}

	//common->Printf("landcheck normal %f %f %f\n", landingCheck1.c.normal.x, landingCheck1.c.normal.y, landingCheck1.c.normal.z);

	//bc 2-17-2016
	//prevent clambering onto slanted surfaces. this is to prevent weirdness when bunnyhopping up ramps.
	if (!CheckClamberNormal(landingCheck1))
	{
		return idVec3(0,0,0);
	}



	if (landingCheck1.fraction >= 1 || !foundLedge)
		return idVec3(0,0,0);

	//gameRenderWorld->DebugCircle( colorWhite,  landingCheck1.endpos   , idVec3(0,0,1), 4, 8, 5000 );
	//gameRenderWorld->DebugArrow( colorWhite,  current.origin  /*start*/,    current.origin + idVec3(0,0,140) /*end*/, 2, 5000);
	//gameRenderWorld->DebugArrow( colorWhite,  verticalCheck1.endpos, verticalCheck1.endpos + (forward * 32)   ,2 , 5000 );
	//gameRenderWorld->DebugArrow( colorOrange,  forwardCheck1.endpos, forwardCheck1.endpos + idVec3(0,0,landingTraceDist)   ,2 , 5000 );	

	if (landingCheck1.c.entityNum != ENTITYNUM_WORLD)
	{
		clamberEnt = gameLocal.entities[ landingCheck1.c.entityNum ];
	}
	else
	{
		clamberEnt = NULL;
	}


	return CheckClamberBounds( landingCheck1.endpos );
}

idVec3 idPhysics_Player::CheckClamberBounds( idVec3 basePos )
{
	idBounds	playerBounds;
	trace_t		trace;	
	int			i;
	idVec3		dirPoint;
	trace_t		searchTrace;
	trace_t		downTrace;
	idAngles	viewAng;
	idVec3		backward;
	

	viewAng = this->viewAngles;
	viewAng.pitch = 0;
	viewAng.roll = 0;
	viewAng.yaw = viewAng.yaw + 180;
	backward = viewAng.ToForward();
	backward.Normalize();


	playerBounds = clipModel->GetBounds();
	playerBounds[1][2] = pm_crouchheight.GetFloat();

	gameLocal.clip.TraceBounds(trace, basePos, basePos, playerBounds, MASK_SOLID, self);

	//if clear, then send back original value.
	if (trace.fraction >= 1)
	{
		if (developer.GetInteger() >= 2)
			gameRenderWorld->DebugBounds(colorWhite, playerBounds, basePos, 10000);

		return basePos;
	}

	//do another check elevated up by 4 units, just in case there's a lip.
	for (int i = 4; i < 32; i += 4)
	{
		trace_t		raisedTrace;
		trace_t		raisedCheck;

		gameLocal.clip.TraceBounds(raisedTrace, basePos + idVec3(0,0, i), basePos + idVec3(0,0, i), playerBounds, MASK_SOLID, self);

		//do a trace from the basePos to the new spot.

		gameLocal.clip.TracePoint(raisedCheck, basePos, basePos + idVec3(0,0,i), MASK_SOLID, self);



		//if clear, then return value.
		if (raisedTrace.fraction >= 1 && raisedCheck.fraction >= 1)
		{
			if (developer.GetInteger() >= 2)
			{
				gameRenderWorld->DebugBounds(colorWhite, playerBounds, basePos + idVec3(0,0, i), 10000);
				gameRenderWorld->DebugArrow( colorYellow,  basePos, basePos + idVec3(0,0,i), 3, 10000);
			}

			return basePos + idVec3(0,0, i);
		}
	}




	if (developer.GetInteger() >= 2)
		gameRenderWorld->DebugBounds(colorRed, playerBounds, trace.endpos, 10000);

	dirPoint.x = trace.c.point.x;
	dirPoint.y = trace.c.point.y;
	dirPoint.z = basePos.z;	

	idVec3 dirToCheck =   backward;
	dirToCheck.Normalize();

	if (developer.GetInteger() >= 2)
		gameRenderWorld->DebugArrow( colorPink,  basePos, basePos + (dirToCheck * 32), 2, 15000);

	for (int i = 1; i < 8; i++)
	{
		idVec3 newPos = basePos + (dirToCheck * (i * 4));
		gameLocal.clip.TraceBounds(searchTrace, newPos , newPos, playerBounds, MASK_SOLID, self);

		if (searchTrace.fraction >= 1)
		{
			break;
		}
	}

	if (searchTrace.fraction < 1)
		return idVec3(0,0,0);

	//trace down and verify this spot still has ground beneath it.
	gameLocal.clip.TraceBounds(downTrace, searchTrace.endpos , searchTrace.endpos + idVec3(0,0,-16), playerBounds, MASK_SOLID, self);

	if (downTrace.fraction < 1 && downTrace.endpos.z > current.origin.z)
	{
		if (developer.GetInteger() >= 2)
			gameRenderWorld->DebugBounds(colorWhite, playerBounds, searchTrace.endpos, 10000);

		return searchTrace.endpos;
	}

	return idVec3(0,0,0);
}

//check whether to start the clamber.
void idPhysics_Player::CheckClamber( void )
{
	//PLAYER IS AIRBORNE.
	
	if ( current.movementFlags & PMF_DUCKED || this->clamberState > 0 || command.forwardmove <= 0 || !this->inJump)
		return;



	bool isGreaseman = false;
	float	playerDistToEndpos;
	idPlayer* localplayer;
	idVec3	clamberEndPos;

	localplayer = static_cast<idPlayer*>(self);
	if (localplayer->PowerUpActive(ADRENALINE))
	{
		isGreaseman = true;
	}


	//prioritize checks based on viewangle pitch.

	if (viewAngles.pitch < -50)
	{
		//prioritize the high ledge check.
		clamberEndPos = GetClamberPos();

		if (clamberEndPos.x == 0 && clamberEndPos.y == 0 && clamberEndPos.z == 0)
		{
			clamberEndPos = GetClamberPosCubby();

			if (clamberEndPos.x == 0 && clamberEndPos.y == 0 && clamberEndPos.z == 0)
				return;
		}
	}
	else
	{
		//attempt the alternate method first.
		clamberEndPos = GetClamberPosCubby();

		if (clamberEndPos.x == 0 && clamberEndPos.y == 0 && clamberEndPos.z == 0)
		{
			clamberEndPos = GetClamberPos();

			if (clamberEndPos.x == 0 && clamberEndPos.y == 0 && clamberEndPos.z == 0)
				return;
		}
	}

	playerDistToEndpos = (current.origin - clamberEndPos).Length();


	
	





	this->clamberMoveTime = playerDistToEndpos * 6.f;
	this->clamberIntensity = idMath::Fabs(playerDistToEndpos - 70.0f) * 0.0143f;


	if ( isGreaseman )
	{
		this->clamberMoveTime *= .25f;
		this->clamberIntensity = 0.1f;
	}


	
	
	if (this->clamberIntensity < 0.1f)
		this->clamberIntensity = 0.1f;
	if (this->clamberIntensity > 1)
		this->clamberIntensity = 1;

	//found a position.

	//zero out all current movement.
	current.velocity.Zero();

	this->clamberStartTime = gameLocal.time;
	this->clamberEndTime = gameLocal.time + this->clamberMoveTime;

	this->clamberOrigin = current.origin;
	this->clamberDestination = clamberEndPos;


	if (clamberEnt.IsValid())
	{
		this->clamberEntityOffset = clamberDestination - clamberEnt.GetEntity()->GetPhysics()->GetOrigin();
	}
	else
	{
		this->clamberEntityOffset = idVec3(0,0,0);
	}


	
	if (localplayer)
	{
		if (isGreaseman)
		{
			localplayer->StartSound( "snd_growlclamber", SND_CHANNEL_BODY, 0, false, NULL );
		}
		else
		{
			localplayer->StartSound( "snd_clamber" , SND_CHANNEL_BODY, 0, false, NULL );
		}
	}

	this->clamberStartPitch = viewAngles.pitch;

	//proceed to next clamberState.
	clamberState = 1;

#ifdef STEAM
	g_SteamStats->SetStatDelta("stat_clambers", 1);
#endif
}


void idPhysics_Player::clamberScripted( const idVec3 &targetPos, float intensity, float movetime )
{
	//float	playerDistToEndpos;
	idPlayer* localplayer;

	current.velocity.Zero();




	this->clamberEnt = NULL;
	this->clamberEntityOffset = idVec3(0,0,0);
	this->clamberOrigin = current.origin;
	this->clamberDestination = targetPos;

	this->clamberStartPitch = viewAngles.pitch;

	localplayer = static_cast<idPlayer*>(self);
	if (localplayer)
	{
		localplayer->StartSound( "snd_clamber", SND_CHANNEL_BODY, 0, false, NULL );
	}

	//playerDistToEndpos = (this->clamberOrigin - this->clamberDestination).Length();
	//this->clamberMoveTime = playerDistToEndpos * 6.f;
	this->clamberMoveTime = movetime;

	

	this->clamberIntensity = idMath::ClampFloat(0.001f, 1.0f, intensity);
	


	this->clamberStartTime = gameLocal.time;
	this->clamberEndTime = gameLocal.time + this->clamberMoveTime;

	this->clamberState = 1;
}

void idPhysics_Player::UpdateClamber( void )
{
	if (clamberState <= 0)
		return;

	idVec3	newPosition = current.origin;
	float	clamberLerp	= 0;
	idPlayer* localplayer = static_cast<idPlayer*>(self);

	//clamberState	0 = not clambering.
	//				1 = rising.
	//				2 = pushing onto ledge.


	idVec3 targetDestination;

	if (this->clamberEnt.IsValid())
	{
		targetDestination = this->clamberEnt.GetEntity()->GetPhysics()->GetOrigin() + this->clamberEntityOffset + idVec3(0,0, 0);
	}
	else
	{
		targetDestination = this->clamberDestination;
	}

	if (clamberState == 1)
	{
		//RISING.
		float pitchAmount, rollAmount;
		current.movementFlags |= PMF_DUCKED;

		clamberLerp = GetClamberLerp();
		
		newPosition.x = idMath::Lerp(this->clamberOrigin.x, idMath::Lerp(this->clamberOrigin.x, targetDestination.x, 0.5f), clamberLerp);
		newPosition.y = idMath::Lerp(this->clamberOrigin.y, idMath::Lerp(this->clamberOrigin.y, targetDestination.y, 0.5f), clamberLerp);

		newPosition.z = idMath::Lerp(this->clamberOrigin.z, targetDestination.z, clamberLerp);
		SetOrigin(newPosition);

		float timeRadians = idMath::PI * clamberLerp;
		

		pitchAmount = idMath::Lerp(-5, -50, this->clamberIntensity);
		rollAmount = idMath::Lerp(5, 10, this->clamberIntensity);

		viewAngles.roll = idMath::Sin(timeRadians) * rollAmount;

		if (clamberLerp <= 0.5)			
		{
			float adjustedLerp = clamberLerp * 2.f;
			viewAngles.pitch = idMath::Lerp(this->clamberStartPitch, pitchAmount, adjustedLerp);  //lerp to -50
		}
		else
			viewAngles.pitch = idMath::Sin(timeRadians) * pitchAmount;


		if (localplayer)
			localplayer->SetViewAngles(viewAngles);

		if (clamberLerp >= 1)
		{			
			if (localplayer)
			{
				if (!localplayer->PowerUpActive(ADRENALINE)) //greaser doesn't clamber exhale
				{
					localplayer->StartSound( "snd_exhale" , SND_CHANNEL_BODY2, 0, false, NULL );
				}
			}

			clamberState = 2;

			this->clamberStartTime = gameLocal.time;
			this->clamberEndTime = gameLocal.time + this->clamberMoveTime;


			this->clamberOrigin = this->PlayerGetOrigin();
		}
	}
	else if (clamberState == 2)
	{
		//PUSH ONTO LEDGE.
		float pitchAmount, rollAmount;
		current.movementFlags |= PMF_DUCKED;

		clamberLerp = GetClamberLerp();
		newPosition.x = idMath::Lerp(this->clamberOrigin.x, targetDestination.x, clamberLerp);
		newPosition.y = idMath::Lerp(this->clamberOrigin.y, targetDestination.y, clamberLerp);
		newPosition.z = idMath::Lerp(targetDestination.z, targetDestination.z, clamberLerp);
		SetOrigin(newPosition);

		pitchAmount = idMath::Lerp(2, 30, this->clamberIntensity);
		rollAmount = idMath::Lerp(-3, -7, this->clamberIntensity);

		float timeRadians = idMath::PI * clamberLerp;
		viewAngles.roll = idMath::Sin(timeRadians) * rollAmount;
		viewAngles.pitch = idMath::Sin(timeRadians) * pitchAmount;
		if (localplayer)
			localplayer->SetViewAngles(viewAngles);

		if (clamberLerp >= 1)
		{
			//end it.
			clamberState = 0;
			current.velocity.Zero();
		}
	}
}

/*
================
idPhysics_Player::MovePlayer
================
*/
void idPhysics_Player::MovePlayer( int msec ) {

	// this counter lets us debug movement problems with a journal
	// by setting a conditional breakpoint for the previous frame
	c_pmove++;

	walking = false;
	groundPlane = false;
	ladder = false;

	// determine the time
	framemsec = msec;
	frametime = framemsec * 0.001f;

	// default speed
	playerSpeed = walkSpeed;

	// remove jumped and stepped up flag
	current.movementFlags &= ~(PMF_JUMPED|PMF_STEPPED_UP|PMF_STEPPED_DOWN);
	current.stepUp = 0.0f;

	if ( command.upmove < 10 ) {
		// not holding jump
		current.movementFlags &= ~PMF_JUMP_HELD;
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
	idPhysics_Player::SetWaterLevel();

	// check for ground
	idPhysics_Player::CheckGround();

	// check if up against a ladder
	idPhysics_Player::CheckLadder();

	// set clip model size
	idPhysics_Player::CheckDuck();

	// handle timers
	idPhysics_Player::DropTimers();




	// move
	if ( current.movementType == PM_DEAD ) {
		// dead
		idPhysics_Player::DeadMove();
	}
	else if (jellyState > 0)
	{
		UpdateJellyboning();
	}
	else if (clamberState > 0)
	{
		UpdateClamber();
	}
	else if ( ladder ) {
		// going up or down a ladder
		idPhysics_Player::LadderMove();
	}
	else if ( current.movementFlags & PMF_TIME_WATERJUMP ) {
		// jumping out of water
		idPhysics_Player::WaterJumpMove();
	}
	else if ( waterLevel > 1 ) {
		// swimming
		idPhysics_Player::WaterMove();
	}
	else if ( walking ) {
		// walking on ground
		idPhysics_Player::WalkMove();
	}
	else {
		// airborne
		idPhysics_Player::AirMove();

		//bc clamber start.		
		CheckClamber();
	}


	// set watertype, waterlevel and groundentity
	idPhysics_Player::SetWaterLevel();
	idPhysics_Player::CheckGround();

	//bc
	if (this->groundPlane)
		this->inJump = false;

	// move the player velocity back into the world frame
	current.velocity += current.pushVelocity;
	current.pushVelocity.Zero();
}

/*
================
idPhysics_Player::GetWaterLevel
================
*/
waterLevel_t idPhysics_Player::GetWaterLevel( void ) const {
	return waterLevel;
}

/*
================
idPhysics_Player::GetWaterType
================
*/
int idPhysics_Player::GetWaterType( void ) const {
	return waterType;
}

/*
================
idPhysics_Player::HasJumped
================
*/
bool idPhysics_Player::HasJumped( void ) const {
	return ( ( current.movementFlags & PMF_JUMPED ) != 0 );
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
idPhysics_Player::OnLadder
================
*/
bool idPhysics_Player::OnLadder( void ) const {
	return ladder;
}

/*
================
idPhysics_Player::idPhysics_Player
================
*/
idPhysics_Player::idPhysics_Player( void ) {
	debugLevel = false;
	clipModel = NULL;
	clipMask = 0;
	memset( &current, 0, sizeof( current ) );
	saved = current;
	walkSpeed = 0;
	crouchSpeed = 0;
	maxStepHeight = 0;
	maxJumpHeight = 0;
	memset( &command, 0, sizeof( command ) );
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
	ladder = false;
	ladderNormal.Zero();
	waterLevel = WATERLEVEL_NONE;
	waterType = 0;

	//bc
	this->clamberState = 0;
	this->clamberEndTime = 0;
	this->clamberStartTime = 0;
	this->nextLadderTime = 0;

	jellyState = 0;
	nextJellyupdate = 0;
	jelly_offset_z = 0;
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
	savefile->WriteInt( debugLevel );

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

	savefile->WriteBool( ladder );
	savefile->WriteVec3( ladderNormal );

	savefile->WriteInt( (int)waterLevel );
	savefile->WriteInt( waterType );


	//bc
	savefile->WriteInt(clamberState);
	savefile->WriteBool(inJump);
	savefile->WriteInt(nextLadderTime);
	savefile->WriteInt(jellyStartTime);
	savefile->WriteInt(jellyEndTime);
	savefile->WriteVec3(jellyOrigin1);
	savefile->WriteVec3(jellyDestination);
	savefile->WriteInt(jellyState);
	savefile->WriteVec3(jellyOrigin0);
	savefile->WriteInt(nextJellyupdate);
	savefile->WriteInt(jelly_offset_z);
	savefile->WriteFloat(jelly_timefactor);
	savefile->WriteVec3(clamberEntityOffset);
	savefile->WriteVec3(clamberOrigin);
	savefile->WriteVec3(clamberDestination);
	savefile->WriteInt(clamberStartTime);
	savefile->WriteInt(clamberEndTime);
	savefile->WriteFloat(clamberMoveTime);
	savefile->WriteFloat(clamberStartPitch);
	savefile->WriteFloat(clamberIntensity);
	clamberEnt.Save(savefile);

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
	savefile->ReadInt( debugLevel );

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

	savefile->ReadBool( ladder );
	savefile->ReadVec3( ladderNormal );

	savefile->ReadInt( (int &)waterLevel );
	savefile->ReadInt( waterType );

	//bc
	savefile->ReadInt(clamberState);
	savefile->ReadBool(inJump);
	savefile->ReadInt(nextLadderTime);
	savefile->ReadInt(jellyStartTime);
	savefile->ReadInt(jellyEndTime);
	savefile->ReadVec3(jellyOrigin1);
	savefile->ReadVec3(jellyDestination);
	savefile->ReadInt(jellyState);
	savefile->ReadVec3(jellyOrigin0);
	savefile->ReadInt(nextJellyupdate);
	savefile->ReadInt(jelly_offset_z);
	savefile->ReadFloat(jelly_timefactor);
	savefile->ReadVec3(clamberEntityOffset);
	savefile->ReadVec3(clamberOrigin);
	savefile->ReadVec3(clamberDestination);
	savefile->ReadInt(clamberStartTime);
	savefile->ReadInt(clamberEndTime);
	savefile->ReadFloat(clamberMoveTime);
	savefile->ReadFloat(clamberStartPitch);
	savefile->ReadFloat(clamberIntensity);
	clamberEnt.Restore(savefile);
}

/*
================
idPhysics_Player::SetPlayerInput
================
*/
void idPhysics_Player::SetPlayerInput( const usercmd_t &cmd, const idAngles &newViewAngles ) {
	command = cmd;
	viewAngles = newViewAngles;		// can't use cmd.angles cause of the delta_angles
}

/*
================
idPhysics_Player::SetSpeed
================
*/
void idPhysics_Player::SetSpeed( const float newWalkSpeed, const float newCrouchSpeed ) {
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

	waterLevel = WATERLEVEL_NONE;
	waterType = 0;
	oldOrigin = current.origin;

	clipModel->Unlink();

	// if bound to a master
	if ( masterEntity ) {
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

	idPhysics_Player::MovePlayer( timeStepMSec );

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

