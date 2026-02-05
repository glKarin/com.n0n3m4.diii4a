/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

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


#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Player.h"
#include "bc_acropoint.h"
#include "Fx.h"
#include "WorldSpawn.h"

#include "physics/Physics_Player.h"

CLASS_DECLARATION( idPhysics_Actor, idPhysics_Player )
END_CLASS

// movement parameters
const float PM_STOPSPEED		= 100.0f;
const float PM_SWIMSCALE		= 0.5f;
const float PM_LADDERSPEED		= 10.0f;
const float PM_LADDER_DASHSPEED = 800.0f;
const int PM_LADDER_DASHTIME	= 300;
const int PM_LADDER_PAUSETIME	= 200;
const float PM_LADDER_DOWNSPEED = 2000.0f;

const float PM_STEPSCALE		= 1.0f;

const float PM_ACCELERATE		= 10.0f;
const float PM_AIRACCELERATE	= 1.0f;
const float PM_WATERACCELERATE	= 4.0f;
const float PM_FLYACCELERATE	= 4.0f;

// Player moves up and down faster than they do laterally in zero-G, because it feels better.
const float PM_FLIGHT_VERTICAL_SPEED_MULTIPLIER = 2.0f; 

const float PM_FRICTION_DIVISOR	= 25.0f;
const float PM_GROUNDFRICTION_MAX = 6.0f;
const float PM_AIRFRICTION		= 0.0f;
const float PM_WATERFRICTION	= 1.0f;
const float PM_FLYFRICTION		= 4.0f;
const float PM_NOCLIPFRICTION	= 12.0f;
const float PM_SPECTATEFRICTION = 4.0f;

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

//BC CLAMBER VARS
const int CLAMBER_MAXHEIGHT					= 100;
const float CLAMBER_MOVETIMESCALAR			= 2.0f; //multiply clambertime by this value. bigger number = longer clamber time
const float CLAMBER_SETTLETIMESCALAR		= 4.0f; //clambertime time scalar for the settling movement.
const float CLAMBER_EASYTIMESCALAR			= 1.0f; //proportional to walking speed
const int ACRO_BUTTONHOLDTIME				= 250;
const int ACRO_VIEWUPDATETIME				= 100; //how often to update acropoints

const float PM_SPLITSSCALE					= 0.8f; //scale splits moving speed.
const float PM_SPLITSACCELERATE				= 3.0f;
const float PM_SPLITSFRICTION				= 7.0f;


const float ACRO_CEILINGHIDE_PLAYERHEIGHT	= 8;
const float ACRO_ACTIVATION_TRACELENGTH		= 112;
const float	ACRO_CLAMBER_PITCHTHRESHOLD		= -50;

const float ACRO_CEILINGHIDE_VIEWARC		= 220;
const float ACRO_CARGOHIDE_VIEWARC			= 45; //For the row cargohide.
const float ACRO_CARGOHIDESTACK_VIEWARC		= 170; //For the stacked cargohide.
const float ACRO_CARGOHIDELAUNDRY_VIEWARC   = 110;
const float ACRO_CEILINGHIDE_VIEWPITCH		= 80;
const float ACRO_SPLITS_VIEWPITCH			= 30;
const float ACRO_SPLITS_VIEWARC				= 200;
const float ACRO_CLAMBER_TRACELENGTH		= -5;

const float PM_DASH_ACCELERATE				= 60.0f;
const float PM_DASH_SCALAR					= 3.0f;
const int PM_DASH_TIME						= 200;
const int PM_DASH_COOLDOWN					= 100; //amount of cooldown time between dashes.
const float DASH_STAMINA_COST               = 48.0f;

const int COYOTE_TIME						= 250; //how much milliseconds after leaving a ledge can a player still jump. Extremely generous since jump is now on spacebar release instead of spacebar press, which screws up the player's timing.

const int LADDER_LERPTIME					= 300; //how much milliseconds it takes to warp player to a ladder mount position.

const int FALLEN_HEADDOWN_TIME = 200; //when fallen, how long is head on ground.
const int FALLEN_HEADRISE_TIME = 300; //how long to raise head after falling down.
const int FALLEN_KICKUP_TIME = 1200;

const int ZIPPAUSETIME = 150; //This is the pause that happens BEFORE the zip movement starts. This is so the player has a chance to briefly see the door/vent/etc visually moving.
const int ZIPPING_TIME = 600; //How long it takes to zip into a ventdoor position.
const int ZIPPING_PITCHTIME = 500;



const float DOWNED_JUMPHEIGHT = 8.0f;
const int DOWNED_MAXCLAMBERHEIGHT = 65;

const int GRABRING_INITIALGRABTIME = 200; //how long it takes for player to lerp to the grab ring.
const int GRABRING_UNGRAB_FORCE = 30; //When ungrabbing the grabring, do a gentle push away.
const int GRABRING_PROPEL_FORCE = 2400; //how hard to propel player.

//how hard player is pushed when drifting toward back of ship in space
const idVec3 SPACE_NUDGE_VELOCITY(0.0f, -6.0f, 0.0f);
const int SPACE_NUDGE_DELAYTIME = 500;
const int SPACE_NUDGE_RAMPTIME = 3000;



int c_pmove = 0;

idCVar pm_debugclamber( "pm_debugclamber", "0", CVAR_CHEAT | CVAR_SYSTEM | CVAR_INTEGER, "enable debug drawing of clamber flag, 0= off, 1=basic -1=all" );
idCVar pm_debugclambertime( "pm_debugclambertime", "5000", CVAR_CHEAT | CVAR_SYSTEM | CVAR_INTEGER, "time in ms to show debug" );

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

Handles user intended acceleration. 

SW: Optional 'clamp vector' keeps us from accelerating beyond a maximum speed in each direction,
but will not slow us down if we are already above that velocity for some reason (being pushed, dashing, etc)
==============
*/
void idPhysics_Player::Accelerate( const idVec3 &wishdir, const float wishspeed, const float accel, const idVec3 &clampVector = vec3_zero ) {
#if 1
	// q2 style
	float addspeed, accelspeed, currentspeed;
	
	// Current speed here is the projection of our velocity onto our desired direction 
	// (i.e. how much of our existing speed gets preserved when we move in the new direction)
	currentspeed = current.velocity * wishdir;
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0) {
		return; // We're not accelerating at all!
	}
	accelspeed = accel * frametime * wishspeed;
	if (accelspeed > addspeed) {
		accelspeed = addspeed;
	}
	idVec3 wishAccel = accelspeed * wishdir;
	
	// SW: We perform component-wise clamping here to prevent various speed boosting issues (i.e. rapidly crouching/jumping in zero-G).
	// The dummied-out 'proper' movement code below would fix this, but as the id programmers noted, it feels dreadful to control.
	// This, on the other hand, should still give us good-feeling straferunning-style movement.
	if (clampVector != vec3_zero)
	{
		if (idMath::Abs(wishAccel.x + current.velocity.x) > clampVector.x)
			wishAccel.x = 0;
		if (idMath::Abs(wishAccel.y + current.velocity.y) > clampVector.y)
			wishAccel.y = 0;
		if (idMath::Abs(wishAccel.z + current.velocity.z) > clampVector.z)
			wishAccel.z = 0;
	}

	current.velocity += wishAccel;
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

			//BC CRASH
			if (!clipModel->GetEntity())
				continue;


			if (clipModel->GetEntity()->IsHidden() || clipModel->GetEntity()->entityNumber < 0
				|| clipModel->GetEntity()->entityNumber >= MAX_GENTITIES - 1 || clipModel->GetEntity()->entityDefNumber < -1
				|| !clipModel->GetEntity()->name || !clipModel->GetEntity()->GetPhysics())
			{
				continue;
			}

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


	//BC if bumping up against wall during zero g, then do an autocrouch check.
	if (gameLocal.GetLocalPlayer()->airless && bumpcount > 0 && gameLocal.time > nextAutocrouchChecktime && command.forwardmove > 0 && this->clamberState == CLAMBERSTATE_NONE)
	{
		nextAutocrouchChecktime = gameLocal.time + 200;
		CheckAutocrouch();
	}


	return (bool)( bumpcount == 0 );
}

void idPhysics_Player::CheckAutocrouch()
{
	idVec3	clamberCandidatePos;

	clamberCandidatePos = idPhysics_Player::GetPossibleClamberPos_Cubby(((idPlayer *)self)->GetEyePosition(), 3);

	if (clamberCandidatePos.x == 0 && clamberCandidatePos.y == 0 && clamberCandidatePos.z == 0)
		return;

	acroType = ACROTYPE_NONE;
	StartClamber(clamberCandidatePos);
	((idPlayer *)self)->SetViewPitchLerp(ACRO_CLAMBER_TRACELENGTH);
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
    if (current.movementType == PM_SPECTATOR || gameLocal.GetLocalPlayer()->isTitleFlyMode)
    {
        //friction for spectator mode
        drop += speed * PM_SPECTATEFRICTION * frametime;
    }
	else if (gameLocal.GetLocalPlayer()->airless )
	{
		//BC if zero G, then apply fly friction.
		drop += speed * PM_FLYFRICTION * frametime;
	}
	// apply ground friction
	else if ( walking && waterLevel <= WATERLEVEL_FEET ) {
		// no friction on slick surfaces
		if ( !(groundMaterial && groundMaterial->GetSurfaceFlags() & SURF_SLICK) ) {
			// if getting knocked back, no friction
			if ( !(current.movementFlags & PMF_TIME_KNOCKBACK) ) {
				float friction = Min( walkSpeed / PM_FRICTION_DIVISOR, PM_GROUNDFRICTION_MAX );
				control = speed < PM_STOPSPEED ? PM_STOPSPEED : speed;
				drop += control * friction * frametime;
			}
		}
	}
	// apply water friction even if just wading
	else if ( waterLevel )
	{
		drop += speed * PM_WATERFRICTION * waterLevel * frametime;
	}
	else
	{
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
	if (gameLocal.GetLocalPlayer()->health > 0)
	{
		idPhysics_Player::Friction();
	}

	scale = idPhysics_Player::CmdScale( command );

	if (scale > 0)
	{
		//when player stops moving, reset the timer for the space nudge delay.
		lastAirlessMoveTime = gameLocal.time;
		spacenudgeState = SN_NONE;
	}
	
	

	if ( !scale || !canFlymoveUp )
	{
		wishvel = vec3_origin;
	}
	else
	{
		wishvel = scale * (viewForward * command.forwardmove + viewRight * command.rightmove);

		//Handle up/down movement.
		//wishvel -= (scale * (command.upmove > 0 ? 2 : 4)) * gravityNormal * command.upmove; //BC We used to scale the up/down speed differently, but we don't anymore since the duck code is partially removed during zero g movement.
		wishvel -= (scale * PM_FLIGHT_VERTICAL_SPEED_MULTIPLIER) * gravityNormal * command.upmove;
	}

	wishdir = wishvel;
	
	wishspeed = wishdir.Normalize();
	float walkSpeed = pm_spacespeed.GetFloat();
	idPhysics_Player::Accelerate( wishdir, wishspeed, PM_FLYACCELERATE, idVec3(walkSpeed, walkSpeed, walkSpeed * PM_FLIGHT_VERTICAL_SPEED_MULTIPLIER) ); // Note the modified clamp vector

	if (groundPlane && (current.velocity * groundTrace.c.normal) < 0.0f)
	{
		//Player's feet is touching ground.
		float vel = current.velocity.Length();
		vel = min(vel, pm_spacespeed.GetInteger()); //BC clamp ground movement speed when in vacuum... for some reason, crouch made it go ultra fast.

		current.velocity.ProjectOntoPlane(groundTrace.c.normal, OVERCLIP);
		current.velocity.Normalize();
		current.velocity *= vel;
	}

	if (command.upmove > 0 && groundPlane)
	{
		current.velocity += idVec3(0, 0, 64);
	}

	
	if (gameLocal.GetLocalPlayer()->IsPlayerNearSpacenudgeEnt())
	{
		//do nothing. don't space nudge the player.
		spacenudgeState = SN_NONE;
	}
	else if ( gameLocal.GetLocalPlayer()->isInOuterSpace() && gameLocal.time > lastAirlessMoveTime + SPACE_NUDGE_DELAYTIME && gameLocal.world->doSpacePush)
	{
		idVec3 nudgeamount = SPACE_NUDGE_VELOCITY;

		if (spacenudgeState == SN_NONE)
		{
			//initialize the nudge ramp up.
			spacenudgeState = SN_RAMPINGUP;
			spacenudgeRampTimer = gameLocal.time;
		}
		else if (spacenudgeState == SN_RAMPINGUP)
		{
			//lerp the nudge.
			float lerp = (gameLocal.time - spacenudgeRampTimer) / (float)SPACE_NUDGE_RAMPTIME;
			lerp = idMath::ClampFloat(0, 1, lerp);
			lerp = idMath::CubicEaseIn(lerp);
			nudgeamount.Lerp(vec3_zero, SPACE_NUDGE_VELOCITY, lerp);

			if (lerp >= 1)
			{
				spacenudgeState = SN_NUDGING;
			}
		}

		current.velocity += nudgeamount;
	}

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

	idPhysics_Player::SlideMove( true, false, false, true );
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

	idVec3	end;

	//Tiny bounding box. So player can easily slurp into vents, etc.
	if (clipModel->GetBounds()[1][2] != 5)
	{
		idBounds bounds = clipModel->GetBounds();
		bounds[1][2] = 5;
		clipModel->LoadModel(idTraceModel(bounds));
	}


	// fly movement

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
============
idPhysics_Player::LadderMove
============
*/
void idPhysics_Player::LadderMove( void ) {
	idVec3	wishdir, wishvel, right;
	float	wishspeed, scale;
	float	upscale;

	float maxladderspeed;

	// stick to the ladder
	wishvel = -300.0f * ladderNormal;
	current.velocity = (gravityNormal * current.velocity) * gravityNormal + wishvel;

	//upscale = (-gravityNormal * viewForward + 0.5f) * 2.5f;
	upscale = (-gravityNormal * viewForward + .25f) ;

	if ( upscale > 1.0f || upscale > 0)
	{
		upscale = 1.0f;
	}
	else if ( upscale < -1.0f || upscale < 0)
	{
		upscale = -1.0f;
	}	


	scale = idPhysics_Player::CmdScale( command );

	//common->Printf("%f\n", upscale * (float)command.forwardmove * scale * gravityNormal);

	wishvel = gravityNormal * upscale * scale * (float)command.forwardmove;

	if (wishvel.z < 0)
	{
		wishvel *= -.9f; //going up.
	}
	else if (wishvel.z > 0)
	{
		wishvel *= -1.5f; //slide down ladders.
	}

	// strafe
	if ( command.rightmove )
	{
		// right vector orthogonal to gravity
		right = viewRight - (gravityNormal * viewRight) * gravityNormal;
		// project right vector into ladder plane
		right = right - (ladderNormal * right) * ladderNormal;
		right.Normalize();

		// if we are looking away from the ladder, reverse the right vector
		if ( ladderNormal * viewForward > 0.0f )
		{
			right = -right;
		}

		wishvel += 2.0f * right * scale * (float) command.rightmove;
	}

	

	// up down movement
	//if ( command.upmove )
	{
		//wishvel += -0.5f * gravityNormal * scale * (float) command.upmove;
	}

	//clamber up?
	if (command.upmove)
	{
		ladder = false;
		return;
	}


	if (command.forwardmove != 0 && wishvel.z > 0)
	{
		//climbing up.
		if (gameLocal.time > ladderTimer + PM_LADDER_DASHTIME + PM_LADDER_PAUSETIME )
		{
			ladderTimer = gameLocal.time;
		}
	}

	if (wishvel.z > 0)
		maxladderspeed = (gameLocal.time < ladderTimer + PM_LADDER_DASHTIME) ? PM_LADDER_DASHSPEED : PM_LADDERSPEED;
	else
		maxladderspeed = PM_LADDER_DOWNSPEED;


	// do strafe friction
	idPhysics_Player::Friction();

	// accelerate
	wishspeed = wishvel.Normalize();
	idPhysics_Player::Accelerate( wishvel, wishspeed, PM_ACCELERATE );

	// cap the vertical velocity
	upscale = current.velocity * -gravityNormal;
	if ( upscale < -maxladderspeed)
	{
		current.velocity += gravityNormal * (upscale + maxladderspeed);
	}
	else if ( upscale > maxladderspeed)
	{
		current.velocity += gravityNormal * (upscale - maxladderspeed);
	}

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

	idPhysics_Player::SlideMove( false, ( command.forwardmove > 0 ), false, false );


	//dismount logic.
	if (wishvel.z > 0 && command.forwardmove != 0)
	{
		CheckLadderDismount();
	}
}

void idPhysics_Player::CheckLadderDismount()
{
	trace_t dismountTr;
	idVec3 eyePos;
	idVec3 endPos;

	eyePos = ((idPlayer *)self)->GetEyePosition() + idVec3(0,0,-8); //start clambering when player chest is above the ladder-well.
	endPos = eyePos + (ladderNormal * -24);
	


	gameLocal.clip.TracePoint(dismountTr, eyePos, endPos, clipMask, self);

	//if no ladder detected......
	if (dismountTr.fraction >= 1.0f)
	{
		//Check for dismounting. Do a clamber check.
		trace_t forwardTr, downTr;
		idVec3 clamberPos;

		//forward trace.
		gameLocal.clip.TracePoint(forwardTr, eyePos, eyePos + (ladderNormal * -32), MASK_SOLID, self);

		if (forwardTr.fraction < 1)
			return; //hit something. abort.

		//ground trace.
		gameLocal.clip.TracePoint(downTr, forwardTr.endpos, forwardTr.endpos + idVec3(0, 0, -64), MASK_SOLID, self);

		if (downTr.fraction >= 1)
			return; //hit nothing. abort.

		clamberPos = CheckClamberBounds(downTr.endpos);

		if (clamberPos == vec3_zero)
			return; //invalid spot. abort.

		if (developer.GetInteger() >= 2)
		{
			idBounds playerBounds;

			playerBounds = clipModel->GetBounds();
			playerBounds[1][2] = pm_crouchheight.GetFloat();
			gameRenderWorld->DebugBounds(colorCyan, playerBounds, clamberPos, 10000);
		}

		acroType = ACROTYPE_NONE;
		StartClamber(clamberPos);
	}
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

	// SW 26th Feb 2025:
	// Don't push our ground contact if it's a monster the player is jockeying!
	// This causes the player (and the monster) to build up ridiculous speeds while ascending ramps
	// (I'd like to make it speedrun-exploitable, but it happens so consistently it just reads as a bug)
	if ( groundEntityPtr.GetEntity() && !gameLocal.GetLocalPlayer()->IsJockeying()) {
		impactInfo_t info;
		groundEntityPtr.GetEntity()->GetImpactInfo( self, groundTrace.c.id, groundTrace.c.point, &info );
		if ( info.invMass != 0.0f ) {
			groundEntityPtr.GetEntity()->ApplyImpulse( self, groundTrace.c.id, groundTrace.c.point, current.velocity / ( info.invMass * 10.0f ) );
		}
	}
}

/*
==============
idPhysics_Player::CheckUnduckOverheadClear
blendo eric: checks is there is clearance overhead to unduck, slow
==============
*/
bool idPhysics_Player::CheckUnduckOverheadClear(void) {
	trace_t	trace, lineTrace;
	idVec3 end = current.origin - (pm_normalheight.GetFloat() - pm_crouchheight.GetFloat()) * gravityNormal;
	gameLocal.clip.Translation(trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self);

	//BC do a traceline because the translation check returns a false positive if player is already embedded in a wall.
	gameLocal.clip.TracePoint(lineTrace, current.origin, end, clipMask, self);

	return trace.fraction >= 1.0f && lineTrace.fraction >= 1.0f;
}

/*
==============
idPhysics_Player::CheckDuck

Sets clip model size
==============
*/
void idPhysics_Player::CheckDuck( void ) {
	idBounds bounds;
	float maxZ;

	if ( current.movementType == PM_DEAD )
	{
		maxZ = pm_deadheight.GetFloat();
	}
	else if (acroType == ACROTYPE_CEILINGHIDE && clamberState == CLAMBERSTATE_ACRO)
	{
		maxZ = ACRO_CEILINGHIDE_PLAYERHEIGHT;
	}
	else if (acroType == ACROTYPE_SPLITS && clamberState == CLAMBERSTATE_ACRO)
	{
		maxZ = ACRO_CEILINGHIDE_PLAYERHEIGHT;
	}
	else if (gameLocal.GetLocalPlayer()->IsJockeying())
	{
		//Player is jockeying. Ignore crouch.
		return;
	}
	else
	{
		// Try enabling force crouching in zero-g
		if ( gameLocal.GetLocalPlayer()->airless && pm_airlessCrouch.GetBool() )
		{
			current.movementFlags |= PMF_DUCKED;
		}
		// stand up when up against a ladder
		else if ( (command.upmove < 0 && !ladder)  || gameLocal.GetLocalPlayer()->inDownedState || gameLocal.time <= forceduckTimer)
		{
			// duck

			if (!gameLocal.GetLocalPlayer()->airless) //ignore manual ducking during zero g.
			{
				current.movementFlags |= PMF_DUCKED; //Normal duck.
			}
		}
		else if (clamberState == CLAMBERSTATE_NONE)
		{
			// stand up if possible
			if ( current.movementFlags & PMF_DUCKED && fallenState == FALLEN_NONE)
			{
				if (CheckUnduckOverheadClear())
				{
					current.movementFlags &= ~PMF_DUCKED;

					//todo: let player exit togglecrouch here.
					
				}
				else
				{
					current.movementFlags |= PMF_DUCKED;
				}
			}
		}

		if ( current.movementFlags & PMF_DUCKED )
		{
			playerSpeed = crouchSpeed;

			//When in fallen state, move slower.
			// SM: Disable this so speed matches constants
// 			if (fallenState != FALLEN_NONE)
// 				playerSpeed = crouchSpeed * .6f;

			maxZ = pm_crouchheight.GetFloat();
		}
		else
		{
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

	// forward vector orthogonal to gravity
	forward = viewForward - (gravityNormal * viewForward) * gravityNormal;
	forward.Normalize();


	if ( walking )
	{
		// don't want to get sucked towards the ladder when still walking		
		tracedist = 1.0f;

		//bc don't attach to ladder while walking.
		//return;
	}
	else
	{
		tracedist = 48.0f;
	}


	end = current.origin + tracedist * forward;
	gameLocal.clip.Translation( trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self );

	// if near a surface
	if ( trace.fraction < 1.0f )
	{
		// if a ladder surface
		if ( trace.c.material && ( trace.c.material->GetSurfaceFlags() & SURF_LADDER ) )
		{
			// check a step height higher
			end = current.origin - gravityNormal * ( maxStepHeight * 0.75f );
			gameLocal.clip.Translation( trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self );
			start = trace.endpos;
			end = start + tracedist * forward;
			gameLocal.clip.Translation( trace, start, end, clipModel, clipModel->GetAxis(), clipMask, self );

			// if also near a surface a step height higher
			if ( trace.fraction < 1.0f ) {

				// if it also is a ladder surface
				if ( trace.c.material && trace.c.material->GetSurfaceFlags() & SURF_LADDER )
				{
					ladder = true;
					ladderNormal = trace.c.normal;
				}
			}
		}
	}
}

// Puts the player in a state where they're getting up off their back. This must be called in order to exit fall state 'normally'
void idPhysics_Player::BeginGetUp(void)
{
	gameLocal.GetLocalPlayer()->SetFallGetupState();
	fallenState = FALLEN_GETUP_KICKUP;
	fallenTimer = gameLocal.time;

	((idPlayer*)self)->SetViewPitchLerp(-30, 500);
	((idPlayer*)self)->SetViewYawLerp(gameLocal.GetLocalPlayer()->viewAngles.yaw, 500);
}

/*
=============
idPhysics_Player::CheckJump
=============
*/
bool idPhysics_Player::CheckJump( void )
{
	// SW 9th April 2025: Moving this to the top so it isn't affected by player's fallen state 
	// (it's possible to jockey from a fallen state and still technically be fallen)
	if (gameLocal.GetLocalPlayer()->IsJockeying() && command.upmove >= 10)
	{
		//Player is jockeying. Don't do a normal jump. Exit jockey state.
		gameLocal.GetLocalPlayer()->SetJockeyMode(false);
		current.movementFlags |= PMF_JUMP_HELD;
		return false;
	}

	// SM: If already in a jump don't jump again
 	if ( inJump )
		return false;

	//if (gameLocal.GetLocalPlayer()->inDownedState || gameLocal.GetLocalPlayer()->IsInMech() || gameLocal.GetLocalPlayer()->IsInHealState())
    if (gameLocal.GetLocalPlayer()->IsInMech() || gameLocal.GetLocalPlayer()->IsInHealState())
		return false;

	if (fallenState == FALLEN_HEADONGROUND || fallenState == FALLEN_RISING || fallenState == FALLEN_GETUP_KICKUP)
	{
		return false;
	}
	else if (fallenState == FALLEN_IDLE)
	{
		if (command.upmove >= 10)
		{
			//Player is in fallen state and presses spacebar to get up.
			BeginGetUp();
		}

		return false;
	}
	else if (gameLocal.time < fallenTimer)
	{
		return false;
	}
	
	
	//BC new jump code to replace old acro view mode.
	if (current.movementFlags & PMF_JUMP_HELD)
	{
		return false;
	}
	if (current.movementFlags & PMF_DUCKED && !gameLocal.GetLocalPlayer()->inDownedState) {
		return false;
	}

	if (command.upmove >= 10)
	{
		if (HasGroundContacts())
		{ // update the current pos of player from ground
#if 0
			trace_t heightTrace;
			gameLocal.clip.TracePoint(heightTrace, current.origin, current.origin - idVec3(0,0,pm_stepsize.GetFloat()), MASK_SOLID, self);
			clamberGroundPosInitial = heightTrace.endpos;
#endif
			clamberGroundPosInitial = current.origin;
		}

		return DoPrejumpLogic();
	}

	

	return false;
}

bool idPhysics_Player::DoPrejumpLogic(void)
{
	//if player is moving forward, or completely still.
	if (clamberState == CLAMBERSTATE_NONE)
	{
        if (gameLocal.GetLocalPlayer()->inDownedState)
        {
            DoJump();
        }
		else if (command.forwardmove >= 10 || current.velocity.Length() <= 0 || (command.forwardmove == 0 && command.rightmove == 0)) //if moving forward or standing still
		{
			//if player is looking down, first do a check to see whether player is trying to mount a ladder.
			//if (this->viewAngles.pitch >= 45)
			//{
			//	trace_t ladderTr;
			//	idVec3 eyePos;
			//
			//	eyePos = ((idPlayer *)self)->GetEyePosition();
			//	gameLocal.clip.TracePoint(ladderTr, eyePos, eyePos + (this->viewAngles.ToForward() * 256), clipMask, self);
			//
			//	if (ladderTr.fraction < 1.0f)
			//	{
			//		// if a ladder surface
			//		if (ladderTr.c.material && (ladderTr.c.material->GetSurfaceFlags() & SURF_LADDER))
			//		{
			//			//found ladder!
			//
			//			//now attempt to find the point on this ladder that's closest to player.
			//			int i;
			//			idVec3 lastHit;
			//
			//			for (i = 0; i < 20; i++)
			//			{
			//				trace_t tr;
			//				idVec3 trStart, trEnd;							
			//
			//				trStart = ladderTr.endpos + (ladderTr.c.normal * 1) + idVec3(0, 0, i * 8);
			//				trEnd = trStart + (ladderTr.c.normal * -4);
			//				
			//				gameLocal.clip.TracePoint(tr, trStart, trEnd, clipMask, self);
			//
			//				if (tr.fraction < 1)
			//				{
			//					lastHit = tr.endpos;
			//				}
			//				else
			//				{
			//					//trace hit NOTHING.
			//					//this means trace is probably at the top of the ladder.
			//
			//					idVec3 candidatePos;
			//					idBounds playerBounds;
			//					trace_t boundTr;
			//
			//					playerBounds = clipModel->GetBounds();
			//
			//					candidatePos = lastHit + idVec3(0, 0, -32) + (ladderTr.c.normal * ((pm_bboxwidth.GetFloat() / 2.0f) + 2.0f));
			//
			//
			//					//don't warp to a ladder above player.
			//					if (candidatePos.z > this->GetOrigin().z)
			//						break;
			//
			//					//check if bounding box collides with anything.
			//					gameLocal.clip.TraceBounds(boundTr, candidatePos, candidatePos, playerBounds, MASK_SOLID, self);
			//
			//					if (boundTr.fraction >= 1)
			//					{
			//						//all clear. do the ladder lerp.
			//						ladderNormal = ladderTr.c.normal;
			//
			//						//Lerp the player to a ladder mount position.
			//						ladderLerpTimer = gameLocal.time;
			//						ladderLerpStart = this->GetOrigin();
			//						ladderLerpTarget = candidatePos;
			//						ladderLerpActive = true;
			//						return false;
			//					}
			//
			//					break;
			//				}
			//			}						
			//		}
			//	}
			//}

			
			DoJump();
			
			
		}
		else if (!gameLocal.GetLocalPlayer()->IsInHealState())
		{
			//Player is starting a dash.

			if (gameLocal.world->spawnArgs.GetBool("vignette")) //disallow dash during vignette
				return false;

			if (((idPlayer *)self)->stamina < DASH_STAMINA_COST)
			{
				//insufficient stamina.
				if (gameLocal.time > dashWarningTimer)
				{
					dashWarningTimer = gameLocal.time + 300;
					gameLocal.GetLocalPlayer()->StartSound("snd_error", SND_CHANNEL_ANY);
					gameLocal.GetLocalPlayer()->hud->HandleNamedEvent("no_stamina");

					//Reset the stamina recharge.
					gameLocal.GetLocalPlayer()->SetStaminaRechargeDelay();
				}
				return false;
			}

			if (dashStartTime + PM_DASH_COOLDOWN > gameLocal.time) //cooldown between dashes.
				return false;

			idAngles fxAng;
			idVec3 fxPos;
			float playerYaw = gameLocal.GetLocalPlayer()->viewAngles.yaw;

			//do a dash.
			dashStartTime = gameLocal.time + PM_DASH_TIME;
			dashSlopeScale = 1.0f;
			self->StartSound("snd_dash", SND_CHANNEL_ANY, 0, false, NULL);

			current.movementFlags |= PMF_JUMP_HELD | PMF_JUMPED;

			if (command.rightmove != 0 && command.forwardmove == 0)
			{
				//Dashing laterally.				
				float modifier = (command.rightmove > 0) ? -90 : 90;
				idVec3 forwardVec, rightVec;
				idAngles straightView = idAngles(0, gameLocal.GetLocalPlayer()->viewAngles.yaw, 0);
				
				straightView.ToVectors(&forwardVec, &rightVec, NULL);

				fxAng = idAngles(90, playerYaw + modifier, 0);		
				
				fxPos = current.origin + idVec3(0, 0, 48) + (forwardVec * 32) + (rightVec * ((command.rightmove > 0) ? 128 : -128));

				//gameRenderWorld->DebugSphere(colorGreen, idSphere(fxPos, 4), 5000);
			}
			else
			{
				//Dashing backward.
				float modifier = 180;

				if (command.rightmove > 0)
					modifier += 45;
				else if (command.rightmove < 0)
					modifier -= 45;

				fxAng = idAngles(90, playerYaw + modifier, 0);
				fxPos = current.origin + idVec3(0, 0, 48);
			}

			gameLocal.GetLocalPlayer()->SetStaminaDelta(-DASH_STAMINA_COST);

			idEntityFx::StartFx("fx/wind_local_short", fxPos, fxAng.ToForward().ToMat3());
		}
	}
	else
	{
		//Player jumped while in an acro mode.
		//when in acrobatic mode, do not do dash move.

		
		acroType = ACROTYPE_NONE;
		clamberState = CLAMBERSTATE_NONE;

		((idPlayer *)self)->SetViewPosActive(false, idVec3(0, 0, 0));

		if (command.forwardmove != 0 || command.rightmove != 0)
		{
			DoJump();
		}
	}

	return true;
}

void idPhysics_Player::DoJump(void)
{
	idVec3 addVelocity;	

	groundPlane = false;		// jumping away
	walking = false;
	current.movementFlags |= PMF_JUMP_HELD | PMF_JUMPED;

    //gameLocal.GetLocalPlayer()->inDownedState

    float jumpHeight = maxJumpHeight;

    if (gameLocal.GetLocalPlayer()->inDownedState)
    {
        jumpHeight = DOWNED_JUMPHEIGHT;
    }

	addVelocity = 2.0f * jumpHeight * -gravityVector;
	addVelocity *= idMath::Sqrt(addVelocity.Normalize());
	current.velocity += addVelocity;

	this->inJump = true;
}


void idPhysics_Player::UpdateLadderLerp(void)
{
	float lerp;
	idVec3 newPosition;

	if (ladder == true)
	{
		ladderLerpActive = false;
		return;
	}

	lerp = (gameLocal.time - ladderLerpTimer) / (float)LADDER_LERPTIME;

	if (lerp > 1)
		lerp = 1;
	else if (lerp < 0)
		lerp = 0;

	lerp = idMath::CubicEaseOut(lerp);
	
	newPosition.x = idMath::Lerp(this->ladderLerpStart.x, ladderLerpTarget.x, lerp);
	newPosition.y = idMath::Lerp(this->ladderLerpStart.y, ladderLerpTarget.y, lerp);
	newPosition.z = idMath::Lerp(this->ladderLerpStart.z, ladderLerpTarget.z, lerp);
	SetOrigin(newPosition);


	if (gameLocal.time >= ladderLerpTimer + LADDER_LERPTIME)
	{
		//end of lerp.
		ladderLerpActive = false;
		ladder = true;
	}
}



// ======================================= END BC =======================================



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

/*
================
idPhysics_Player::MovePlayer
================
*/
void idPhysics_Player::MovePlayer(int msec)
{
	//BC this gets called every frame.

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
	current.movementFlags &= ~(PMF_JUMPED | PMF_STEPPED_UP | PMF_STEPPED_DOWN);
	current.stepUp = 0.0f;

	if (command.upmove < 10) {
		// not holding jump
		current.movementFlags &= ~PMF_JUMP_HELD;
	}

	// if no movement at all
	if (current.movementType == PM_FREEZE) {
		// SW 10th March 2025:
		// Check jump if the player is jockeying (we need to let them jump off)
		if (gameLocal.GetLocalPlayer()->IsJockeying())
		{
			idPhysics_Player::CheckJump();
		}
		return;
	}

	// move the player velocity into the frame of a pusher
	// SM: Don't apply push velocity if no clip
	if (current.movementType != PM_NOCLIP) {
		current.velocity -= current.pushVelocity;
	}

	// view vectors
	viewAngles.ToVectors(&viewForward, NULL, NULL);
	viewForward *= clipModelAxis;
	viewRight = gravityNormal.Cross(viewForward);
	viewRight.Normalize();



	// fly in spectator mode
	if (current.movementType == PM_SPECTATOR) {
		SpectatorMove();
		idPhysics_Player::DropTimers();
		return;
	}

	// special no clip mode
	if (current.movementType == PM_NOCLIP) {
		idPhysics_Player::NoclipMove();
		idPhysics_Player::DropTimers();
		return;
	}

	

	// no control when dead
	if (current.movementType == PM_DEAD)
	{
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

	//Update warping player to ladder. Be sure to call this before the LadderMove() call.
	if (ladderLerpActive)
	{
		UpdateLadderLerp();
	}

	if (command.upmove == 0 && !canFlymoveUp)
	{
		canFlymoveUp = true;
	}

	// move
	if (current.movementType == PM_DEAD) {
		// dead
		idPhysics_Player::DeadMove();
	}
	else if (fallenState != FALLEN_NONE)
	{
		idPhysics_Player::UpdateFallState();
	}
	else if (grabringState == GR_GRABIDLE || grabringState == GR_GRABSTART)
	{
		UpdateGrabRing();
	}
	else if (this->clamberState == CLAMBERSTATE_ACRO)
	{
		idPhysics_Player::UpdateAcro();
	}
	else if (this->clamberState != CLAMBERSTATE_NONE)
	{
		idPhysics_Player::UpdateClamber();
		current.pushVelocity.Zero();
		current.velocity.Zero();
	}
	else if (zippingState != ZIPPINGSTATE_NONE)
	{
		idPhysics_Player::UpdateZipping();
	}
	else if (swoopState != SWOOPSTATE_NONE)
	{
		idPhysics_Player::UpdateSwooping();
	}
	else if (vacuumSplineMover.IsValid())
	{
		idPhysics_Player::UpdateVacuumSplineMoving();
		idPhysics_Player::SlideMove(false, true, true, true);
	}
	else if (ladder)
	{
		// going up or down a ladder
		idPhysics_Player::LadderMove();
	}
	else if (movelerping)
	{
		//doing the movelerp.
		idPhysics_Player::UpdateMovelerp();
	}
	else if (current.movementFlags & PMF_TIME_WATERJUMP) {
		// jumping out of water
		idPhysics_Player::WaterJumpMove();
	}
	else if (waterLevel > 1) {
		// swimming
		idPhysics_Player::WaterMove();
	}
	else if (gameLocal.GetLocalPlayer()->airless || gameLocal.GetLocalPlayer()->isTitleFlyMode)
	{
		FlyMove();
	}
	else if (walking)
	{
		// walking on ground
		idPhysics_Player::WalkMove();
	}
	else
	{
		if (gameLocal.time < coyotetimeTimer + COYOTE_TIME)
		{
			idPhysics_Player::CheckJump();
		}

		// airborne
		idPhysics_Player::AirMove();

		//player is airborne. check if they can clamber.
		idPhysics_Player::TryClamber();
	}

	if (dashStartTime > gameLocal.time)
	{  //Player is currently doing a dash. Propel the player.

		// calculate how much the slope is interefering with the dash
		if (groundPlane && dashSlopeScale > 0.0f)
		{
			float groundDot = -(groundTrace.c.normal * gravityNormal);
			if (groundDot < MIN_WALK_NORMAL)
			{
				gameLocal.voManager.SayVO(gameLocal.GetLocalPlayer(), "snd_softfall", VO_CATEGORY_GRUNT);
				dashSlopeScale = 0.0f;
			}
			else
			{
				// decreases to zero as it approaches steepest walkable slope
				groundDot = (groundDot - MIN_WALK_NORMAL) / (1.0f - MIN_WALK_NORMAL);
				groundDot = groundDot * 0.5f + 0.5f; // halve the effect;
				float newSlopeScale = idMath::ClampFloat(0.0f, 1.0f, groundDot);
				dashSlopeScale = newSlopeScale < dashSlopeScale ? newSlopeScale : dashSlopeScale;
			}
		}

		float wishspeed;
		idVec3 wishvel, wishdir;

		wishvel = PM_DASH_SCALAR * (viewForward * command.forwardmove + viewRight * command.rightmove);
		wishdir = wishvel;
		wishspeed = wishdir.Normalize();

		idPhysics_Player::Accelerate(wishdir, wishspeed * dashSlopeScale, PM_DASH_ACCELERATE* dashSlopeScale);
		idPhysics_Player::SlideMove(false, true, true, true);
	}

	// set watertype, waterlevel and groundentity
	idPhysics_Player::SetWaterLevel();
	idPhysics_Player::CheckGround();

	if (this->groundPlane)
	{
		this->inJump = false;
		coyotetimeTimer = gameLocal.time;
	}

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


	//BC INITIALIZE
	clamberState = CLAMBERSTATE_NONE;
	clamberForceDuck = true;
	inJump = false;
	
	
	acroViewAngleArc = 0;
	coyotetimeTimer = 0; //coyote time lets us jump after leaving a ledge.	
	dashStartTime = 0;	
	dashWarningTimer = 0;
	dashSlopeScale = 1.0f;
	ladderTimer = 0;
	acroType = ACROTYPE_NONE;
	ladderLerpActive = false;
	fallenState = FALLEN_NONE;
	fallenTimer = 0;
	forceduckTimer = 0;
	zippingState = ZIPPINGSTATE_NONE;
	zippingTimer = 0;
	zippingOrigin = vec3_zero;
	zippingCameraMidpoint = vec3_zero;

	swoopState = SWOOPSTATE_NONE;


	movelerpStartPoint = vec3_zero;
	movelerpDestinationPoint = vec3_zero;
	movelerpTimer = 0;
	movelerping = false;

	nextAutocrouchChecktime = 0;
	clamberGroundPosInitial = vec3_zero;
	clamberGroundPosCurrent = vec3_zero;

	hideType = 0;

	grabringStartPos = vec3_zero;
	grabringDestinationPos = vec3_zero;
	grabringTimer = 0;
	grabringState = GR_NONE;
	grabringEnt = NULL;

	canFlymoveUp = true;

	lastAirlessMoveTime = 0;
	spacenudgeRampTimer = 0;
	spacenudgeState = SN_NONE;

	vacuumSplineMover = NULL;

	swoopStartEnt = 0;
	swoopEndEnt = 0;

	//BC INIT
	//BC RESET
}

/*
================
idPhysics_Player_SavePState
================
*/
void idPhysics_Player_SavePState( idSaveGame *savefile, const playerPState_t &state ) {
	savefile->WriteVec3( state.origin ); //  idVec3 origin
	savefile->WriteVec3( state.velocity ); //  idVec3 velocity
	savefile->WriteVec3( state.localOrigin ); //  idVec3 localOrigin
	savefile->WriteVec3( state.pushVelocity ); //  idVec3 pushVelocity
	savefile->WriteFloat( state.stepUp ); //  float stepUp
	savefile->WriteInt( state.movementType ); //  int movementType
	savefile->WriteInt( state.movementFlags ); //  int movementFlags
	savefile->WriteInt( state.movementTime ); //  int movementTime
}

/*
================
idPhysics_Player_RestorePState
================
*/
void idPhysics_Player_RestorePState( idRestoreGame *savefile, playerPState_t &state ) {
	savefile->ReadVec3( state.origin ); //  idVec3 origin
	savefile->ReadVec3( state.velocity ); //  idVec3 velocity
	savefile->ReadVec3( state.localOrigin ); //  idVec3 localOrigin
	savefile->ReadVec3( state.pushVelocity ); //  idVec3 pushVelocity
	savefile->ReadFloat( state.stepUp ); //  float stepUp
	savefile->ReadInt( state.movementType ); //  int movementType
	savefile->ReadInt( state.movementFlags ); //  int movementFlags
	savefile->ReadInt( state.movementTime ); //  int movementTime
}

/*
================
idPhysics_Player::Save
================
*/
void idPhysics_Player::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( hideType ); //  int hideType
	
	idPhysics_Player_SavePState( savefile, current ); //  playerPState_t current
	idPhysics_Player_SavePState( savefile, saved ); //  playerPState_t saved
	
	savefile->WriteFloat( walkSpeed ); //  float walkSpeed
	savefile->WriteFloat( crouchSpeed ); //  float crouchSpeed
	savefile->WriteFloat( maxStepHeight ); //  float maxStepHeight
	savefile->WriteFloat( maxJumpHeight ); //  float maxJumpHeight
	savefile->WriteInt( debugLevel ); //  int debugLevel
	savefile->WriteUsercmd( command ); //  usercmd_t command
	savefile->WriteAngles( viewAngles ); //  idAngles viewAngles
	savefile->WriteInt( framemsec ); //  int framemsec
	savefile->WriteFloat( frametime ); //  float frametime
	savefile->WriteFloat( playerSpeed ); //  float playerSpeed
	savefile->WriteVec3( viewForward ); //  idVec3 viewForward
	savefile->WriteVec3( viewRight ); //  idVec3 viewRight
	savefile->WriteBool( walking ); //  bool walking
	savefile->WriteBool( groundPlane ); //  bool groundPlane
	savefile->WriteTrace( groundTrace ); //  trace_t groundTrace
	savefile->WriteMaterial( groundMaterial ); // const  idMaterial * groundMaterial
	savefile->WriteBool( ladder ); //  bool ladder
	savefile->WriteVec3( ladderNormal ); //  idVec3 ladderNormal
	savefile->WriteInt( waterLevel ); //  waterLevel_t waterLevel
	savefile->WriteInt( waterType ); //  int waterType
	
	savefile->WriteInt( clamberState ); //  int clamberState
	savefile->WriteVec3( clamberOrigin ); //  idVec3 clamberOrigin
	savefile->WriteVec3( clamberTransition ); //  idVec3 clamberTransition
	savefile->WriteVec3( clamberDestination ); //  idVec3 clamberDestination
	savefile->WriteInt( clamberStartTime ); //  int clamberStartTime
	savefile->WriteInt( clamberStateTime ); //  int clamberStateTime
	savefile->WriteFloat( clamberTotalMoveTime ); //  float clamberTotalMoveTime
	savefile->WriteBool( clamberForceDuck ); //  bool clamberForceDuck
	savefile->WriteVec3( clamberGroundPosInitial ); //  idVec3 clamberGroundPosInitial
	savefile->WriteVec3( clamberGroundPosCurrent ); //  idVec3 clamberGroundPosCurrent
	
	savefile->WriteInt( acroUpdateTimer ); //  int acroUpdateTimer
	
	savefile->WriteInt( acroType ); //  int acroType
	savefile->WriteFloat( acroAngle ); //  float acroAngle
	savefile->WriteVec3( lastgoodAcroPosition ); //  idVec3 lastgoodAcroPosition
	savefile->WriteFloat( acroViewAngleArc ); //  float acroViewAngleArc
	savefile->WriteInt( dashStartTime ); //  int dashStartTime
	savefile->WriteInt( dashWarningTimer ); //  int dashWarningTimer
	savefile->WriteFloat( dashSlopeScale ); //  float dashSlopeScale
	
	savefile->WriteBool( inJump ); //  bool inJump
	savefile->WriteInt( coyotetimeTimer ); //  int coyotetimeTimer
	
	savefile->WriteInt( ladderLerpTimer ); //  int ladderLerpTimer
	savefile->WriteBool( ladderLerpActive ); //  bool ladderLerpActive
	savefile->WriteVec3( ladderLerpTarget ); //  idVec3 ladderLerpTarget
	savefile->WriteVec3( ladderLerpStart ); //  idVec3 ladderLerpStart
	savefile->WriteInt( ladderTimer ); //  int ladderTimer
	
	savefile->WriteInt( fallenState ); //  int fallenState
	savefile->WriteInt( fallenTimer ); //  int fallenTimer
	
	savefile->WriteInt( forceduckTimer ); //  int forceduckTimer
	
	savefile->WriteInt( zippingState ); //  int zippingState
	savefile->WriteInt( zippingTimer ); //  int zippingTimer
	savefile->WriteVec3( zippingOrigin ); //  idVec3 zippingOrigin
	savefile->WriteVec3( zippingCameraStart ); //  idVec3 zippingCameraStart
	savefile->WriteVec3( zippingCameraEnd ); //  idVec3 zippingCameraEnd
	savefile->WriteVec3( zippingCameraMidpoint ); //  idVec3 zippingCameraMidpoint
	savefile->WriteFloat( zipForceDuckDuration ); //  float zipForceDuckDuration
	
	savefile->WriteInt( swoopState ); //  int swoopState
	savefile->WriteInt( swoopTimer ); //  int swoopTimer
	savefile->WriteVec3( swoopStartPoint ); //  idVec3 swoopStartPoint
	savefile->WriteVec3( swoopDestinationPoint ); //  idVec3 swoopDestinationPoint
	savefile->WriteObject( swoopStartEnt ); //  idEntity * swoopStartEnt
	savefile->WriteObject( swoopEndEnt ); //  idEntity * swoopEndEnt
	savefile->WriteInt( swoopParticletype ); //  int swoopParticletype
	
	savefile->WriteObject( cargohideEnt ); //  idEntityPtr<idEntity> cargohideEnt
	
	savefile->WriteVec3( movelerpStartPoint ); //  idVec3 movelerpStartPoint
	savefile->WriteVec3( movelerpDestinationPoint ); //  idVec3 movelerpDestinationPoint
	savefile->WriteInt( movelerpTimer ); //  int movelerpTimer
	savefile->WriteBool( movelerping ); //  bool movelerping
	savefile->WriteInt( movelerp_Duration ); //  int movelerp_Duration
	
	savefile->WriteInt( nextAutocrouchChecktime ); //  int nextAutocrouchChecktime
	savefile->WriteVec3( grabringStartPos ); //  idVec3 grabringStartPos
	savefile->WriteVec3( grabringDestinationPos ); //  idVec3 grabringDestinationPos
	savefile->WriteInt( grabringTimer ); //  int grabringTimer
	savefile->WriteInt( grabringState ); //  int grabringState
	
	savefile->WriteObject( grabringEnt ); //  idEntityPtr<idEntity> grabringEnt
	
	savefile->WriteBool( canFlymoveUp ); //  bool canFlymoveUp
	
	savefile->WriteInt( lastAirlessMoveTime ); //  int lastAirlessMoveTime
	savefile->WriteInt( spacenudgeState ); //  int spacenudgeState
	
	savefile->WriteInt( spacenudgeRampTimer ); //  int spacenudgeRampTimer
	vacuumSplineMover.Save( savefile ); //  idEntityPtr<idMover> vacuumSplineMover
}

/*
================
idPhysics_Player::Restore
================
*/
void idPhysics_Player::Restore( idRestoreGame *savefile ) {

	savefile->ReadInt( hideType ); //  int hideType
	
	idPhysics_Player_RestorePState( savefile, current ); //  playerPState_t current
	idPhysics_Player_RestorePState( savefile, saved ); //  playerPState_t saved
	
	savefile->ReadFloat( walkSpeed ); //  float walkSpeed
	savefile->ReadFloat( crouchSpeed ); //  float crouchSpeed
	savefile->ReadFloat( maxStepHeight ); //  float maxStepHeight
	savefile->ReadFloat( maxJumpHeight ); //  float maxJumpHeight
	savefile->ReadInt( debugLevel ); //  int debugLevel
	savefile->ReadUsercmd( command ); //  usercmd_t command
	savefile->ReadAngles( viewAngles ); //  idAngles viewAngles
	savefile->ReadInt( framemsec ); //  int framemsec
	savefile->ReadFloat( frametime ); //  float frametime
	savefile->ReadFloat( playerSpeed ); //  float playerSpeed
	savefile->ReadVec3( viewForward ); //  idVec3 viewForward
	savefile->ReadVec3( viewRight ); //  idVec3 viewRight
	savefile->ReadBool( walking ); //  bool walking
	savefile->ReadBool( groundPlane ); //  bool groundPlane
	savefile->ReadTrace( groundTrace ); //  trace_t groundTrace
	savefile->ReadMaterial( groundMaterial ); // const  idMaterial * groundMaterial
	savefile->ReadBool( ladder ); //  bool ladder
	savefile->ReadVec3( ladderNormal ); //  idVec3 ladderNormal
	savefile->ReadInt( (int&)waterLevel ); //  waterLevel_t waterLevel
	savefile->ReadInt( waterType ); //  int waterType
	
	savefile->ReadInt( clamberState ); //  int clamberState
	savefile->ReadVec3( clamberOrigin ); //  idVec3 clamberOrigin
	savefile->ReadVec3( clamberTransition ); //  idVec3 clamberTransition
	savefile->ReadVec3( clamberDestination ); //  idVec3 clamberDestination
	savefile->ReadInt( clamberStartTime ); //  int clamberStartTime
	savefile->ReadInt( clamberStateTime ); //  int clamberStateTime
	savefile->ReadFloat( clamberTotalMoveTime ); //  float clamberTotalMoveTime
	savefile->ReadBool( clamberForceDuck ); //  bool clamberForceDuck
	savefile->ReadVec3( clamberGroundPosInitial ); //  idVec3 clamberGroundPosInitial
	savefile->ReadVec3( clamberGroundPosCurrent ); //  idVec3 clamberGroundPosCurrent
	
	savefile->ReadInt( acroUpdateTimer ); //  int acroUpdateTimer
	
	savefile->ReadInt( acroType ); //  int acroType
	savefile->ReadFloat( acroAngle ); //  float acroAngle
	savefile->ReadVec3( lastgoodAcroPosition ); //  idVec3 lastgoodAcroPosition
	savefile->ReadFloat( acroViewAngleArc ); //  float acroViewAngleArc
	savefile->ReadInt( dashStartTime ); //  int dashStartTime
	savefile->ReadInt( dashWarningTimer ); //  int dashWarningTimer
	savefile->ReadFloat( dashSlopeScale ); //  float dashSlopeScale
	
	savefile->ReadBool( inJump ); //  bool inJump
	savefile->ReadInt( coyotetimeTimer ); //  int coyotetimeTimer
	
	savefile->ReadInt( ladderLerpTimer ); //  int ladderLerpTimer
	savefile->ReadBool( ladderLerpActive ); //  bool ladderLerpActive
	savefile->ReadVec3( ladderLerpTarget ); //  idVec3 ladderLerpTarget
	savefile->ReadVec3( ladderLerpStart ); //  idVec3 ladderLerpStart
	savefile->ReadInt( ladderTimer ); //  int ladderTimer
	
	savefile->ReadInt( fallenState ); //  int fallenState
	savefile->ReadInt( fallenTimer ); //  int fallenTimer
	
	savefile->ReadInt( forceduckTimer ); //  int forceduckTimer
	
	savefile->ReadInt( zippingState ); //  int zippingState
	savefile->ReadInt( zippingTimer ); //  int zippingTimer
	savefile->ReadVec3( zippingOrigin ); //  idVec3 zippingOrigin
	savefile->ReadVec3( zippingCameraStart ); //  idVec3 zippingCameraStart
	savefile->ReadVec3( zippingCameraEnd ); //  idVec3 zippingCameraEnd
	savefile->ReadVec3( zippingCameraMidpoint ); //  idVec3 zippingCameraMidpoint
	savefile->ReadFloat( zipForceDuckDuration ); //  float zipForceDuckDuration
	
	savefile->ReadInt( swoopState ); //  int swoopState
	savefile->ReadInt( swoopTimer ); //  int swoopTimer
	savefile->ReadVec3( swoopStartPoint ); //  idVec3 swoopStartPoint
	savefile->ReadVec3( swoopDestinationPoint ); //  idVec3 swoopDestinationPoint
	savefile->ReadObject( swoopStartEnt ); //  idEntity * swoopStartEnt
	savefile->ReadObject( swoopEndEnt ); //  idEntity * swoopEndEnt
	savefile->ReadInt( swoopParticletype ); //  int swoopParticletype
	
	savefile->ReadObject( cargohideEnt ); //  idEntityPtr<idEntity> cargohideEnt
	
	savefile->ReadVec3( movelerpStartPoint ); //  idVec3 movelerpStartPoint
	savefile->ReadVec3( movelerpDestinationPoint ); //  idVec3 movelerpDestinationPoint
	savefile->ReadInt( movelerpTimer ); //  int movelerpTimer
	savefile->ReadBool( movelerping ); //  bool movelerping
	savefile->ReadInt( movelerp_Duration ); //  int movelerp_Duration
	
	savefile->ReadInt( nextAutocrouchChecktime ); //  int nextAutocrouchChecktime
	savefile->ReadVec3( grabringStartPos ); //  idVec3 grabringStartPos
	savefile->ReadVec3( grabringDestinationPos ); //  idVec3 grabringDestinationPos
	savefile->ReadInt( grabringTimer ); //  int grabringTimer
	savefile->ReadInt( grabringState ); //  int grabringState
	
	savefile->ReadObject( grabringEnt ); //  idEntityPtr<idEntity> grabringEnt
	
	savefile->ReadBool( canFlymoveUp ); //  bool canFlymoveUp
	
	savefile->ReadInt( lastAirlessMoveTime ); //  int lastAirlessMoveTime
	savefile->ReadInt( spacenudgeState ); //  int spacenudgeState
	
	savefile->ReadInt( spacenudgeRampTimer ); //  int spacenudgeRampTimer
	vacuumSplineMover.Restore( savefile ); //  idEntityPtr<idMover> vacuumSplineMover
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

	DebugDraw(); // blendo eric: debug viz

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
	msg.WriteDeltaInt( 0, current.movementTime );
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
	current.movementTime = msg.ReadDeltaInt( 0 );

	if ( clipModel ) {
		clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );
	}
}



//=================================================================
//
//                    BC START CLAMBER STUFF
const float CLAMBER_REACH_INITIAL = 24; // this is the best distance to check for a player directly against the wall
const float STAIR_RISE_OVER_RUN_MAX = idMath::Tan( DEG2RAD(45.0f) );
const int DEBUG_CLAMBER_SIZE = 2;
const int DEBUG_CLAMBER_FLAG_ALL = ~0;
void DEBUG_CLAMBER_ARROW(const idVec4& color, const idVec3& start, const idVec3& end, int flag = 1)
{
#ifdef _DEBUG
	if( (pm_debugclamber.GetInteger() & flag) || (flag == 0 && pm_debugclamber.GetInteger() != 0) )
	{
		gameRenderWorld->DebugArrow(color, start, end, DEBUG_CLAMBER_SIZE, pm_debugclambertime.GetInteger());
	}
#endif
}

void DEBUG_CLAMBER_TEXT(const char *str, const idVec4& color, const idVec3& pos, int flag = 1)
{
#ifdef _DEBUG
	if( (pm_debugclamber.GetInteger() & flag) || (flag == 0 && pm_debugclamber.GetInteger() != 0) )
	{
		static idList<idStr> prevTexts = idList<idStr>();
		static idList<idBounds> prevTextsBounds = idList<idBounds>();
		static idList<int> prevTextsExpiration = idList<int>();

		// don't draw text next to another
		bool found = false;
		for( int idx = 0; idx < prevTexts.Num(); ++idx)
		{
			if(prevTexts[idx] == str)
			{
				if(prevTextsExpiration[idx] < gameLocal.time)
				{
					prevTexts.RemoveIndex(idx);
					prevTextsBounds.RemoveIndex(idx);
					prevTextsExpiration.RemoveIndex(idx);
					break;
				}
				found = true;

				bool tooClose = prevTextsBounds[idx].ContainsPoint(pos);
				prevTextsBounds[idx].AddBounds(idBounds(pos - idVec3(0.5f,0.5f,0.5f), pos + idVec3(0.5f,0.5f,0.5f)));

				if( tooClose )
				{
					return;
				}
				break;
			}
		}

		if(!found)
		{
			prevTexts.Append(str);
			prevTextsBounds.Append(idBounds(pos - idVec3(0.5f,0.5f,0.5f), pos + idVec3(0.5f,0.5f,0.5f)) );
			prevTextsExpiration.Append(gameLocal.time+1000);
		}

		gameRenderWorld->DrawText(str, pos - idVec3(5.0f,5.0f,10.0f), 0.1f, colorRed,gameLocal.GetLocalPlayer()->viewAxis, true, pm_debugclambertime.GetInteger());
	}
#endif
}

int idPhysics_Player::GetClamberMaxHeightLocal()
{
	return gameLocal.GetLocalPlayer()->inDownedState ? DOWNED_MAXCLAMBERHEIGHT : g_clambermaxheight.GetInteger();
}

int idPhysics_Player::GetClamberMaxHeightWorld()
{
	int calculatedHeight = GetClamberMaxHeightLocal() + clamberGroundPosInitial.z + 0.5f;
	return calculatedHeight;
}

int idPhysics_Player::CheckClamberHeight(float z)
{
	return z <= GetClamberMaxHeightWorld();
}

int idPhysics_Player::GetAcroType()
{
	return acroType;
}

int idPhysics_Player::GetClamberState()
{
	return clamberState;
}

void idPhysics_Player::GetAcroAngleRestrictions(float& baseAngle, int& arcSize)
{
	baseAngle = acroAngle;
	arcSize = acroViewAngleArc;
}

float idPhysics_Player::GetAcroAngle(void)
{
	return acroAngle;
}

// returns whether the clamber pos is likely on stairs based upon jump and current ground positions
// or if it is low enough to still hop onto (high stair)
// note: landing pos should be the trace down to the ground, rather than the hovering bounds point, or this will be inaccurate
bool idPhysics_Player::ClamberCheckSteppable(idVec3& newClamberPos)
{
	const int DEBUG_CLAMBER_STEP_FLAG = 4;

	idVec3 curGroundFromInitial = clamberGroundPosCurrent - clamberGroundPosInitial;
	idVec3 clamberFromCurGround = newClamberPos - clamberGroundPosCurrent;
	idVec3 clamberTotalVec = newClamberPos - clamberGroundPosInitial;
	float clamberRunDist = clamberTotalVec.ToVec2().LengthFast(); // horiztonal distance

	// if player is moving backwards or initial and current ground pos are too close, just use overall vec
	if( clamberFromCurGround.ToVec2() * curGroundFromInitial.ToVec2() < 0.0f || curGroundFromInitial.ToVec2().LengthSqr() <= 1.0f )
	{
		curGroundFromInitial = clamberTotalVec*0.5f;
		clamberFromCurGround = clamberTotalVec*0.5f;
	}

	// likely already a step in front of the clamber pos
	if( clamberFromCurGround.z < pm_stepsize.GetFloat() + 1.0f )
	{
		DEBUG_CLAMBER_TEXT( "low step", colorRed, newClamberPos );
		return true;
	}
	// check if player trajectory can still reach step without assistance
	float calculatedTrajectory = 0.5f*idMath::Fabs(current.velocity.z)*current.velocity.z / (g_gravity.GetFloat()+0.0001f);
	if ((calculatedTrajectory + current.origin.z) > newClamberPos.z + 1.0f)
	{
		if( clamberRunDist*clamberRunDist < idVec2(current.velocity.x,current.velocity.y).LengthSqr() )
		{
			DEBUG_CLAMBER_TEXT( "can hop on", colorRed, newClamberPos );
			return true;
		}
	}

	// if there's a dip, it's not continous stairs
	if( curGroundFromInitial.z < -maxJumpHeight )
	{
		if( (pm_debugclamber.GetInteger() & DEBUG_CLAMBER_STEP_FLAG) )
		{
			gameLocal.Printf( "\n not stairs: dip found " );
			DEBUG_CLAMBER_ARROW( colorBlue, clamberGroundPosInitial, clamberGroundPosCurrent, DEBUG_CLAMBER_STEP_FLAG );
			DEBUG_CLAMBER_ARROW( colorCyan, clamberGroundPosCurrent, newClamberPos, DEBUG_CLAMBER_STEP_FLAG );
		}

   		return false;
	}
	
	if( clamberTotalVec.z < 0.0f ) // if no dip, then downstep is steppable
	{
		DEBUG_CLAMBER_TEXT( "down step", colorRed, newClamberPos );
		return true;
	}


	// check if the rise over run ratio is under the limit for stairs
	float clamberVertAdj = clamberTotalVec.z - pm_stepsize.GetFloat(); // include step error margin
   	bool isStairs = (clamberVertAdj / clamberRunDist) < STAIR_RISE_OVER_RUN_MAX;

	if(isStairs)
	{ // now check from current ground pos
		float clamberCurGroundRunDist = clamberFromCurGround.ToVec2().LengthFast(); // horiztonal distance
		float clamberCurGroundVertAdj = clamberFromCurGround.z - pm_stepsize.GetFloat(); // include step error margin
		isStairs = (clamberCurGroundVertAdj / clamberCurGroundRunDist) < STAIR_RISE_OVER_RUN_MAX;
	}

	if ( (pm_debugclamber.GetInteger() & DEBUG_CLAMBER_STEP_FLAG) )
	{
		DEBUG_CLAMBER_ARROW(colorLtGrey, clamberGroundPosInitial, clamberGroundPosCurrent, DEBUG_CLAMBER_STEP_FLAG);
		DEBUG_CLAMBER_ARROW(colorLtGrey, clamberGroundPosCurrent, newClamberPos, DEBUG_CLAMBER_STEP_FLAG); 

		if(isStairs)
		{
			DEBUG_CLAMBER_ARROW( colorRed, newClamberPos, newClamberPos - idVec3(0,0,pm_stepsize.GetFloat()), DEBUG_CLAMBER_STEP_FLAG);
		}
		else
		{
			DEBUG_CLAMBER_ARROW( colorGreen, newClamberPos, newClamberPos - idVec3(0,0,pm_stepsize.GetFloat()), DEBUG_CLAMBER_STEP_FLAG);
		}

		gameRenderWorld->DebugCircle( colorDkGrey, clamberGroundPosInitial,idVec3(0,0,1), DEBUG_CLAMBER_SIZE, 5, pm_debugclambertime.GetInteger());
		gameRenderWorld->DebugSphere( colorLtGrey, idSphere( newClamberPos, DEBUG_CLAMBER_SIZE), pm_debugclambertime.GetInteger());
	}

#if _DEBUG
	if(isStairs) DEBUG_CLAMBER_TEXT( "is stair", colorRed, newClamberPos );
#endif

     return isStairs;
}

bool idPhysics_Player::TryClamber(bool checkFromCrouch, int numIterations)
{
	//abort clambercheck if ducked, or in mid-clamber, or not moving forward.
	if (!checkFromCrouch && ((current.movementFlags & PMF_DUCKED && !gameLocal.GetLocalPlayer()->inDownedState) 
		|| this->clamberState != CLAMBERSTATE_NONE || command.forwardmove <= 0 || !this->inJump  || gameLocal.GetLocalPlayer()->IsInHealState()))
		return false;
	
	if (acroType == ACROTYPE_CARGOHIDE || clamberState != CLAMBERSTATE_NONE)
		return false;

	idVec3	clamberCandidatePos = vec3_zero;

	{ // update the current height of player from ground
		// use a bounds sweep, since ray trace is inaccurate when player hovers over ledges
		idVec3 downPos = current.origin + idVec3(0,0,-2.0f*pm_jumpheight.GetFloat());
		idBounds feetBounds = clipModel->GetBounds();
		feetBounds[0][2] = 0; //lower vertical bound.
		feetBounds[1][2] = 8.0f; //upper vert
		trace_t boundsTrace;
		gameLocal.clip.TraceBounds(boundsTrace, current.origin, downPos, feetBounds, MASK_SOLID | CONTENTS_CLIMBCLIP, self);
		clamberGroundPosCurrent = boundsTrace.endpos;
		clamberGroundPosCurrent.z = Min(clamberGroundPosCurrent.z,current.origin.z);
	}

	if ( pm_debugclamber.GetInteger() >= 511 ) // 511 = 0001 1111 1111
	{
		gameRenderWorld->DebugClearLines(0);
		gameRenderWorld->DebugClearLines(gameLocal.time);
	}

	if ( pm_debugclamber.GetInteger() != 0 )
	{
		gameRenderWorld->DebugCircle( colorCyan, clamberGroundPosInitial, idVec3::Up(), DEBUG_CLAMBER_SIZE, 5, pm_debugclambertime.GetInteger());
	}

	idVec3 shiftBack = vec3_zero;

	//prioritize the 2 clamber checks based on view pitch.
	if (viewAngles.pitch < ACRO_CLAMBER_PITCHTHRESHOLD)
	{
		//player is looking upward. prioritize the high ledge check.
		clamberCandidatePos = GetPossibleClamberPos_Ledge(numIterations, shiftBack);

		if (clamberCandidatePos == vec3_zero)
		{
			//failed the ledge check.
			//attempt the cubby check.
			clamberCandidatePos = GetPossibleClamberPos_Cubby(((idPlayer *)self)->GetEyePosition(), numIterations - 1);
		}
	}
	else
	{
		//player is looking straight ahead. prioritize the cubby check.
		clamberCandidatePos = GetPossibleClamberPos_Cubby(((idPlayer *)self)->GetEyePosition(), numIterations - 1);

		if (clamberCandidatePos == vec3_zero)
		{
			//failed the cubby check.
			//attempt the ledge check.
			clamberCandidatePos = GetPossibleClamberPos_Ledge(numIterations, shiftBack);
			
			if (clamberCandidatePos == vec3_zero)
			{
				//failed the ledge check.
			
				//Player might be trying to clamber into a knee-high passage.
				clamberCandidatePos = GetPossibleClamberPos_Cubby(((idPlayer *)self)->GetEyePosition() + idVec3(0,0,-24), numIterations - 1);
			}
		}
	}

	if (clamberCandidatePos == vec3_zero)
	{
		//failed all checks. do not clamber. exit here.
		return false;
	}

	//Check if it's a valid height delta. This is so that the player doesn't clamber up staircases or ramps.
	float heightDelta = fabs(clamberCandidatePos.z - this->PlayerGetOrigin().z);
	if ( current.velocity.z > 0.0f && heightDelta <= pm_stepsize.GetFloat()) //it's less than a staircase step. Exit.
	{
		DEBUG_CLAMBER_TEXT( "ramp delta", colorRed, clamberCandidatePos );
		return false;
	}

	// Checking max clamber height moved to check bounds, to allow some leeway for climbing on ramps

	acroType = ACROTYPE_NONE;

	bool quickClamber = false;

	// check if this clamber was easily jumpable, like a large step
	bool inJumpHeight = ((clamberCandidatePos.z - clamberGroundPosInitial.z) <= maxJumpHeight + 1.0f);
	bool inJumpRange = (clamberCandidatePos - clamberGroundPosInitial).LengthFast() < pm_walkspeed.GetFloat()*0.75f;

	bool notFallingBelow = current.origin.z + pm_stepsize.GetFloat() > clamberCandidatePos.z || (current.velocity.z > 0.0);
   	if ( notFallingBelow && inJumpHeight && inJumpRange )
	{
		// check if there's standing room on landing
		trace_t standingTrace;
		if( !gameLocal.clip.TraceBounds(standingTrace, clamberCandidatePos, clamberCandidatePos, clipModel->GetBounds(), MASK_SOLID, self) )
		{
			quickClamber = true;
		}
	}

	if ( (pm_debugclamber.GetInteger() != 0) ) gameLocal.Printf( "%s move\n", quickClamber ? " quick" : " slow");

	if ( (pm_debugclamber.GetInteger() & 1) )
	{
		DEBUG_CLAMBER_ARROW(idVec4(1.0f,0.25f,0.5f,1.0f), clamberGroundPosInitial, clamberCandidatePos);
		DEBUG_CLAMBER_ARROW(idVec4(1.0f,0.25f,0.5f,1.0f), current.origin, clamberGroundPosCurrent);
	}
	
	if( quickClamber )
	{ // easy clamber smooths horiztonal motion, without ducking
		StartClamberQuick(clamberCandidatePos);
		//((idPlayer *)self)->SetViewPitchLerp(viewAngles.pitch+10,500);
	}
	else
	{
		StartClamber(clamberCandidatePos, shiftBack);
		((idPlayer *)self)->SetViewPitchLerp(ACRO_CLAMBER_TRACELENGTH);
	}
	return true;
}


/*
==============
idPhysics_Player::CheckClamberExitCubby
blendo eric: checks is there is clearance nearby to stand outside the cubby
==============
*/
idVec3 idPhysics_Player::CheckClamberExitCubby(int testSidesCount) {
	const float BOUNDS_TEST_DIST = 16.0f;
	float frontDist = pm_clamberCubbyExitDistFront.GetFloat();
	float backDist = pm_clamberCubbyExitDistBack.GetFloat();
	float angleStep = 360.0f/(float)testSidesCount;

	idBounds playerBounds = clipModel->GetBounds();
	playerBounds.Expand(1.0f);
	playerBounds[1][2] = pm_normalheight.GetFloat();

	float closestDist = idMath::INFINITY;
	idVec3 bestPos = vec3_zero;

	idVec3 heightVec = idVec3(0.0f, 0.0f, pm_normalheight.GetFloat());

	idVec3 startPos = current.origin + idVec3(0.0f,0.0f,1.0f);
	idVec3 upPos = startPos + heightVec;

	// start preferred exit angle towards movement direction, or facing direction
	float preferredYaw = viewAngles.yaw;
	idVec3 velDir = current.velocity;
	velDir.z = 0.0f;
	if(velDir.LengthSqr() > 1.0f)
	{
		velDir.NormalizeFast();
		preferredYaw = velDir.ToAngles().yaw;
	}
	idVec3 preferredFacingDir = idAngles(0.0f,preferredYaw,0.0f).ToForward();
	DEBUG_CLAMBER_ARROW(colorWhite,startPos,startPos + preferredFacingDir*frontDist);

	float curAngle = idMath::Rint(preferredYaw/angleStep)*angleStep;

	trace_t trRay;
	trace_t	trBounds;
	for( int idx = 0; idx < testSidesCount; ++idx) {
		idVec3 curDir = idAngles(0.0f,curAngle,0.0f).ToForward();
		curAngle += angleStep;

		float exitDist = idMath::Lerp(backDist,frontDist, curDir*preferredFacingDir*0.5f+0.5f );
		idVec3 landingPos = startPos + curDir*exitDist;

		gameLocal.clip.TracePoint(trRay, startPos, landingPos, clipMask, self);
		DEBUG_CLAMBER_ARROW(colorCyan,startPos,trRay.endpos);

		const float boundDistFraction = BOUNDS_TEST_DIST/exitDist;
		if( trRay.fraction <= boundDistFraction )
		{ // wall too close to player
			continue;
		}

		// if hit wall, move inwards to make room
		if( trRay.fraction < 1.0f )
		{
			landingPos = trRay.endpos - curDir*(BOUNDS_TEST_DIST+0.5f);
		}

		// check for landing height
		gameLocal.clip.TracePoint(trRay, landingPos, landingPos+heightVec, clipMask, self);
		DEBUG_CLAMBER_ARROW(colorBlue,landingPos,trRay.endpos);
		idVec3 landingCeil = trRay.endpos;
		idVec3 testEntranceStart = landingCeil;

		if( trRay.fraction < 1.0f )
		{
			// if not enough room, check for drop first
			idVec3 dropStart = landingCeil;
			idVec3 dropEnd = landingCeil-heightVec;
			gameLocal.clip.TracePoint(trRay, dropStart, dropEnd, clipMask, self);
			DEBUG_CLAMBER_ARROW(colorPurple,dropStart,trRay.endpos);
			if( trRay.fraction < 1.0f )
			{ // not enough standing height
				continue;
			}
			landingPos = trRay.endpos + idVec3(0.0f,0.0f,0.5f);
			testEntranceStart = landingPos;
		}

		// final bounds check
		gameLocal.clip.TraceBounds(trBounds, landingPos, landingPos, playerBounds, clipMask, self);

		bool boundsOK = trBounds.fraction >= 1.0f;
		if ( pm_debugclamber.GetInteger() & 1 )
		{
			gameRenderWorld->DebugBounds(boundsOK ? colorCyan : colorRed, playerBounds, landingPos, pm_debugclambertime.GetInteger());
		}

		if( boundsOK )
		{
			// if aligned with view, use it immediately
			if(idx == 0)
			{
				bestPos = landingPos;
				break;
			}

			// try to find the exit closest to the player by sending a ray back to the cubby edge
			gameLocal.clip.TracePoint(trRay, testEntranceStart, startPos, clipMask, self);

			float distToEntrance = (trRay.endpos-startPos).LengthFast();
			if( distToEntrance < closestDist ) {
				closestDist = distToEntrance;
				bestPos = landingPos;
			}
		}
	}
	return bestPos;
}


bool idPhysics_Player::TryClamberOutOfCubby(bool checkInCubby)
{
	if(checkInCubby && CheckUnduckOverheadClear() )
	{
		return false;
	}

	if( fallenState != FALLEN_NONE || !IsCrouching() )
	{
		return false;
	}

	idVec3 uncrouchNearLocation = CheckClamberExitCubby();
	if(uncrouchNearLocation == vec3_zero)
	{
		return false;
	}

	StartClamber(uncrouchNearLocation);
	return true;
}

void idPhysics_Player::StartClamber( idVec3 targetPos, idVec3 shiftBack )
{
	// defaults from previous rev
	idPhysics_Player::StartClamber( targetPos, shiftBack, CLAMBER_MOVETIMESCALAR, CLAMBER_SETTLETIMESCALAR, 1.0f, 0.3f, true );
}

void idPhysics_Player::StartClamberQuick( idVec3 targetPos, idVec3 shiftBack )
{ // easy clamber smooths horiztonal motion, without ducking
	float newClamberTimeScale = CLAMBER_EASYTIMESCALAR * 1000.0f/ (pm_walkspeed.GetFloat()+0.00001f); // seconds per unit pace
	idPhysics_Player::StartClamber(targetPos, shiftBack, newClamberTimeScale,newClamberTimeScale, 0.4f, 0.5f, false);
}

void idPhysics_Player::StartClamber( idVec3 targetPos, idVec3 shiftBack, float raiseTimeScale, float settleTimeScale, float verticalMix, float horizontalMix, bool forceDuck, bool ignoreLipCheck )
{
	// raiseTimeScale/settleTimeScale = x milliseconds per 1 unit moved
	// verticalMix/horizontalMix = contribution of vertical/horiztonal component to first clamber state

	clamberForceDuck = forceDuck;
	//stop all movement.
	current.velocity.Zero();

	this->clamberState = CLAMBERSTATE_RAISING;	
	this->clamberStartTime = gameLocal.time;
	this->clamberDestination = targetPos;
	this->clamberOrigin = current.origin;

	idVec3 clamberVec = (targetPos - current.origin);

	// check for a high lip on ledge and prevent player phasing through it when clambering up
	idVec3 clamberVecBack = -clamberVec;
	clamberVecBack.z = pm_crouchviewheight.GetFloat();
	// check for lip behind
	trace_t findLip;
	gameLocal.clip.TracePoint(findLip, targetPos, targetPos + clamberVecBack, clipMask, self);

	if (shiftBack != vec3_zero)
	{
		horizontalMix = 0.0f;
		verticalMix = 1.0f;
	}

	// SW 23rd April 2025: Letting certain clambers ignore lip check (mostly those entering cargohides or acropoints)
	bool edgeLipExists = ignoreLipCheck ? false : findLip.fraction < 1.0f;
	float extraClamberTimeMS = 0.0f;
	if (edgeLipExists)
	{
		horizontalMix *= 0.5f;
		verticalMix = 1.0f;
		extraClamberTimeMS = 100.0f; // extra pause
	}

	// calc the middle position the player transitions to
	idVec3 transitionVec = idVec3( clamberVec.x*horizontalMix, clamberVec.y*horizontalMix, clamberVec.z*verticalMix );

	if (edgeLipExists)
	{ // add height of a potential lip
		transitionVec.z = clamberVec.z + pm_normalviewheight.GetFloat();
	}
	if (shiftBack != vec3_zero)
	{
		transitionVec += shiftBack;
	}

	this->clamberTransition = transitionVec + current.origin;

	// timeScale is ms per 1 unit moved
	float clamberRaiseTime = transitionVec.LengthFast() * raiseTimeScale + extraClamberTimeMS;
	float clamberSettleTime = (clamberVec - transitionVec).LengthFast() * settleTimeScale;

	this->clamberTotalMoveTime = gameLocal.time + clamberRaiseTime + clamberSettleTime;
	this->clamberStateTime = gameLocal.time + clamberRaiseTime;
	

	// unset the togglecrouch unless it was the command that started the clamber
	bool crouchPressed = command.prevButtonState[UB_DOWN] <= 0 && command.buttonState[UB_DOWN] > 0 && !common->IsConsoleActive();
	if(!crouchPressed)
	{
		gameLocal.GetLocalPlayer()->UnsetToggleCrouch();
	}

	if(clamberForceDuck)
	{
		current.movementFlags |= PMF_DUCKED;

		// SW 11th Feb 2025: changed to use body channel (we've had problems with it interrupting VO)
		gameLocal.GetLocalPlayer()->StartSound("snd_clamber", SND_CHANNEL_BODY, 0, false, NULL);

		//BC 2-13-2025: clamber grunt now on a cooldown timer.
		gameLocal.GetLocalPlayer()->SayVO_WithIntervalDelay((gameLocal.GetLocalPlayer()->GetWoundCount() > 0) ? "snd_vo_exertion" : "snd_vo_clamber", VO_CATEGORY_GRUNT);
	}
	else
	{
		gameLocal.GetLocalPlayer()->StartSound("snd_softfall", SND_CHANNEL_BODY, 0, false, NULL);
	}
}

void idPhysics_Player::UpdateClamber(void)
{
	if(clamberForceDuck)
	{
		current.movementFlags |= PMF_DUCKED;
	}

	if (clamberState == CLAMBERSTATE_RAISING)
	{
		float clamberLerp;
		idVec3 newPosition;

		//First part of the clamber. Rising upward.
		
		clamberLerp = GetClamberLerp();
		clamberLerp = idMath::CubicEaseOut(clamberLerp);

		newPosition.Lerp (this->clamberOrigin, this->clamberTransition, clamberLerp );
		
		SetOrigin(newPosition);

		if ( developer.GetInteger() > 1 || (pm_debugclamber.GetInteger() & 2) )
		{
			gameRenderWorld->DebugBounds(idVec4(1, 0, 0, 1), this->GetBounds(), newPosition, pm_debugclambertime.GetInteger());
		}

		if (clamberLerp >= 1)
		{
			clamberState = CLAMBERSTATE_SETTLING;

			this->clamberStartTime = this->clamberStateTime;
 			this->clamberStateTime = this->clamberTotalMoveTime;
			this->clamberOrigin = this->PlayerGetOrigin();
		}
	}
	else if (clamberState == CLAMBERSTATE_SETTLING)
	{
		//push forward so player settles onto the ledge.
		float clamberLerp =  GetClamberLerp();
		idVec3 newPosition;

		clamberLerp = GetClamberLerp();
		//clamberLerp = idMath::CubicEaseOut(clamberLerp);
		newPosition.Lerp(this->clamberTransition, this->clamberDestination, clamberLerp);

		SetOrigin(newPosition);

		if ( developer.GetInteger() > 1 || (pm_debugclamber.GetInteger() & 2) )
		{
			gameRenderWorld->DebugBounds(idVec4(0, 1, 0, 1), this->GetBounds(), newPosition, pm_debugclambertime.GetInteger());
		}

		if (clamberLerp >= 1)
		{
			//clamber is done.
			current.velocity.Zero();

			if (acroType == ACROTYPE_NONE)
			{
				//clamber onto a ledge.
				clamberState = CLAMBERSTATE_NONE;
			}	
			else
			{
				//enter an acro state.

				if (acroType == ACROTYPE_SPLITS)
				{
					//for splits, stand up.
					current.movementFlags &= ~PMF_DUCKED;

					acroViewAngleArc = ACRO_SPLITS_VIEWARC;
				}
				else if (acroType == ACROTYPE_CARGOHIDE)
				{
					if (hideType == CARGOHIDETYPE_ROW)
						acroViewAngleArc = ACRO_CARGOHIDE_VIEWARC;
					else if (hideType == CARGOHIDETYPE_STACK)
						acroViewAngleArc = ACRO_CARGOHIDESTACK_VIEWARC;
					else if (hideType == CARGOHIDETYPE_LAUNDRYMACHINE)
						acroViewAngleArc = ACRO_CARGOHIDELAUNDRY_VIEWARC;

				}
				else
				{
					acroViewAngleArc = ACRO_CEILINGHIDE_VIEWARC;
				}

				clamberState = CLAMBERSTATE_ACRO;
			}
		}
	}
}

//The ledge check checks for ledges above the player.
idVec3 idPhysics_Player::GetPossibleClamberPos_Ledge(int numIterations, idVec3 & shiftBack)
{
	idVec3 returnValue = GetPossibleClamberPos_Ledgecheck(false, numIterations, shiftBack);

	if( returnValue == vec3_zero )
	{
		returnValue = GetPossibleClamberPos_Ledgecheck(true, numIterations, shiftBack);

		if( (pm_debugclamber.GetInteger() != 0) && returnValue != vec3_zero ) gameLocal.Printf( "\nclamber spot ledge shiftback" );
	}

	if ( (pm_debugclamber.GetInteger() & 1) && returnValue != vec3_zero)
	{
		gameRenderWorld->DebugCircle( colorGreen, returnValue, idVec3::Up(), DEBUG_CLAMBER_SIZE, 5, pm_debugclambertime.GetInteger());
	}

	if (returnValue == vec3_zero)
	{
		shiftBack = vec3_zero;
	}

	return returnValue;
}


//backwardscheck = if player is under an overhang ledge, do a check that peeks over the ledge. This is for situations where there's a slight lip on the ledge.
idVec3 idPhysics_Player::GetPossibleClamberPos_Ledgecheck(bool backwardsCheck, int numIterations, idVec3 & shiftBack)
{	
	int DEBUG_CLAMBER_LEDGE_FLAG = 1;

	const float playerRadius = 16;// = (clipModel->GetBounds().Max().x);

	trace_t		sideCheck, verticalCheck1, forwardCheck1, landingCheck1;
	idAngles	viewAng;
	idVec3		forward, right, up;
	bool		foundLedge;
	idVec3		startPos;
	idVec3		firstValidGround;

	firstValidGround = vec3_zero;

	foundLedge = false;
	viewAng = this->viewAngles;
	viewAng.pitch = 0;
	viewAng.roll = 0;
	viewAng.ToVectors( &forward, &right, &up );
	forward.Normalize();
	right.Normalize();
	up.Normalize();

	startPos = current.origin;

	// check for ledges slightly behind
	if (backwardsCheck)
	{
		shiftBack = (forward * -playerRadius);
		startPos += shiftBack;
		DEBUG_CLAMBER_LEDGE_FLAG = 8;
	}

#if 1
	{ // check side bounds, this prevents the player from clambering into/clipping side walls when up against corners
		float playerRadiusPadded = playerRadius*1.25f;
		idVec3 centeredPos = vec3_zero;
		if( gameLocal.clip.TracePoint(sideCheck, startPos, startPos + right*playerRadiusPadded, MASK_SOLID | CONTENTS_CLIMBCLIP, self, CONTENTS_NOCLIMBCLIP) )
		{
			centeredPos += ((1.0f-sideCheck.fraction)*playerRadiusPadded) * sideCheck.c.normal;
		}
		else
		{
			centeredPos += right*playerRadiusPadded;
		}
		if( gameLocal.clip.TracePoint(sideCheck, startPos, startPos - right*playerRadiusPadded, MASK_SOLID | CONTENTS_CLIMBCLIP, self, CONTENTS_NOCLIMBCLIP) )
		{
			centeredPos += -((1.0f-sideCheck.fraction)*playerRadiusPadded) * sideCheck.c.normal;
		}
		else
		{
			centeredPos += -right*playerRadiusPadded;
		}
		centeredPos = startPos + centeredPos*0.5f;
		DEBUG_CLAMBER_ARROW(colorBlue, startPos, centeredPos,DEBUG_CLAMBER_LEDGE_FLAG);
		startPos = centeredPos;
	}
#endif

	// change height of clamber check based on if player is looking up,
	// preventing unwanted ledge grabs, similar to cubby check
	idVec3 forward3d = viewAngles.ToForward().Normalized();
	float vertDistViewAdjust = idMath::ClampFloat(0.0f,1.0f,forward3d.z*1.5f + 1.0f);
	float clamberHeightReach = GetClamberMaxHeightLocal() + (backwardsCheck ? 18 : 0);
	clamberHeightReach = idMath::Lerp(pm_normalheight.GetFloat(), clamberHeightReach, vertDistViewAdjust );

	float horizontalDistAdjust = Min(1.0f,forward3d.ToVec2().LengthFast())*0.5f+0.5f;
	float clamberForwardReach = (pm_clamberReachDist.GetInteger() - CLAMBER_REACH_INITIAL)*horizontalDistAdjust;

	//Trace that goes vertically; start at player position and go upward.
	gameLocal.clip.TracePoint(verticalCheck1, startPos, startPos + idVec3(0, 0, clamberHeightReach), MASK_SOLID | CONTENTS_CLIMBCLIP, self, CONTENTS_NOCLIMBCLIP);
	DEBUG_CLAMBER_ARROW(backwardsCheck ? colorBlue: colorCyan, startPos, verticalCheck1.endpos,DEBUG_CLAMBER_LEDGE_FLAG);
	
	// leave room for player
	if (verticalCheck1.fraction < 1.0f)
	{
		verticalCheck1.endpos.z -= pm_crouchheight.GetFloat() * 0.5f;
		assert((verticalCheck1.endpos.z - current.origin.z) > 0.0f);
	}

	//downward vertical distance we travel.
	idVec3 landingTraceDownVec = idVec3(0.0f,0.0f, idMath::Fabs(verticalCheck1.endpos.z - current.origin.z) * -1.f );

	// split first and end distances by the number of iterations
	const float forwardCheckDistLast = clamberForwardReach + CLAMBER_REACH_INITIAL;
	float forwardCheckDistFirst = CLAMBER_REACH_INITIAL;
	float forwardCheckDistDelta = (forwardCheckDistLast-forwardCheckDistFirst)/( numIterations > 2 ? numIterations -1 : 1 );

	idVec3 forwardTraceStart = verticalCheck1.endpos;
	idVec3 forwardTraceNext = forwardTraceStart + (forward * forwardCheckDistFirst);
	//do a series of forward checks.
	for (int i = 0; i < numIterations; i++)
	{
		//Trace that goes FORWARD.
		gameLocal.clip.TracePoint(forwardCheck1, forwardTraceStart, forwardTraceNext, MASK_SOLID | CONTENTS_CLIMBCLIP, self, CONTENTS_NOCLIMBCLIP);
		DEBUG_CLAMBER_ARROW(backwardsCheck ? colorBlue: colorCyan, forwardTraceStart, forwardCheck1.endpos,DEBUG_CLAMBER_LEDGE_FLAG | 512);

		bool forwardHit = forwardCheck1.fraction < 1.0f;

		//Trace that checks for the LANDING POSITION.
		gameLocal.clip.TracePoint(landingCheck1, forwardCheck1.endpos, forwardCheck1.endpos + landingTraceDownVec, MASK_SOLID | CONTENTS_CLIMBCLIP, self, CONTENTS_NOCLIMBCLIP);
		DEBUG_CLAMBER_ARROW(colorWhite, forwardCheck1.endpos, landingCheck1.endpos,DEBUG_CLAMBER_LEDGE_FLAG | 512);

		if (landingCheck1.endpos.z > GetClamberMaxHeightWorld())
		{
			// if there's geo blocking higher than clamber height, don't bother continuing
			DEBUG_CLAMBER_TEXT( "too high", colorRed, landingCheck1.endpos );
			break;
		}


		bool landingOK = landingCheck1.fraction < 1.0f;

		idEntity* ent = landingOK ? gameLocal.entities[landingCheck1.c.entityNum] : nullptr;

		landingOK = ent && ent->CanClamber();

		idMat3 playerViewaxis;
		idVec3 playerOrigin;
		gameLocal.GetLocalPlayer()->GetViewPos(playerOrigin, playerViewaxis);

		// check there is a large enough path for the player
		// as it is possible to trace through small holes or cracks
		if (landingOK)
		{
			idVec3 midPoint = (forwardCheck1.endpos + landingCheck1.endpos)*0.5f;
			idVec3 diagBound = (0.5f * playerRadius) * (forward + right) + (midPoint - landingCheck1.endpos);
			idVec3 topBound = idVec3(idMath::Fabs(diagBound.x), idMath::Fabs(diagBound.y), idMath::Fabs(diagBound.z));
			idVec3 bottomBound = -topBound;

			// make sure there's room above the landing, to allow a path to drop down onto
			float openingOverheadRoom = pm_crouchheight.GetFloat();
			topBound.z += openingOverheadRoom*0.5f;
			bottomBound.z += openingOverheadRoom*0.5f;

			// enforce bounds min height
			if ((topBound.z - bottomBound.z) < openingOverheadRoom)
			{
				bottomBound.z = topBound.z - openingOverheadRoom;
			}

			idBounds openingBound = idBounds( bottomBound, topBound );
			trace_t openingCheck;
			gameLocal.clip.TraceBounds(openingCheck, midPoint, midPoint, openingBound, MASK_SOLID | CONTENTS_CLIMBCLIP, self);

			if (openingCheck.fraction < 1.0f)
			{
				landingOK = false;
			}

			if ( developer.GetInteger() >= 2 || (pm_debugclamber.GetInteger() & 512) )
			{
				openingBound.TranslateSelf(midPoint);
				gameRenderWorld->DebugBounds(landingOK ? colorCyan : colorRed, openingBound, vec3_origin, pm_debugclambertime.GetInteger());
			}
		}

		if (landingOK)
		{
			// prevent player from hovering over ledge, when pushed out by steppable geo
			// if wall already hit, skip edge detection and "step" forward
			if(!forwardHit && pm_clamberEdgeDetection.GetBool())
			{ // if too close to edge, find edge wall position, then find a landing a step further in
			  // 
			  //                   5.trace forward a step (+player bounds) using wall pos, to check for wall
			  //                         x--step-length-->--bounds-->
			  //                         ^                          |
			  //                                         <--bounds-- 6.readjust without player bounds
			  //                            1.start      | 
			  //  2.step back & down           v         v 7.find landing
			  //               <--step-length--x         x
			  //               |          ================================
			  // 3.check floor v-------->x|| CORNER     FLOOR     
			  //          4.find wall pos ||
			  //                          || WALL
			  //                          ||

				const float desiredDistFromEdge = playerRadius*0.5f;

				// step back and drop down slightly, in front of the edge wall
				idVec3 outsideWallPos = landingCheck1.endpos - forward*desiredDistFromEdge - idVec3(0, 0, 1.0f);

				// if there is edge contact, it means the clamber pos is already beyond the desired distance from edge
				bool wallClipped = gameLocal.clip.Contents( outsideWallPos, NULL, mat3_identity, CONTENTS_SOLID, NULL ) & MASK_SOLID;
				if( !wallClipped )
				{
  					trace_t wallTrace; // find the position of the ledge edge, by tracing into the front wall
					gameLocal.clip.TracePoint( wallTrace, outsideWallPos, outsideWallPos+forward*desiredDistFromEdge, MASK_SOLID | CONTENTS_CLIMBCLIP, self );
					bool edgeOK = wallTrace.fraction > 0.0f && wallTrace.fraction < 1.0f;


					DEBUG_CLAMBER_ARROW (colorOrange, outsideWallPos, wallTrace.endpos, 512);

					if( edgeOK )
					{ // we can use the discovered wall position to do a very accurate stair check on the corner edge
						idVec3 ledgeEdgePosition = idVec3(wallTrace.endpos.x,wallTrace.endpos.y,landingCheck1.endpos.z);
						edgeOK = !ClamberCheckSteppable(ledgeEdgePosition);
					}

					if( edgeOK )
					{ // try to find a new landing spot a "step" forward from the wall position
					  // position at wall, but move up to original forward trace height
						idVec3 posFromWall = idVec3( wallTrace.endpos.x, wallTrace.endpos.y, forwardTraceStart.z );
						idVec3 finalStepInPos = posFromWall + forward*(desiredDistFromEdge+playerRadius);

						trace_t stepInTrace;
						// trace forward a "step" from our high up forward trace
						gameLocal.clip.TracePoint( stepInTrace, posFromWall, finalStepInPos, MASK_SOLID | CONTENTS_CLIMBCLIP, self );

						DEBUG_CLAMBER_ARROW (colorOrange, posFromWall, stepInTrace.endpos, 512);

						/// remove bounds margin from drop in
						idVec3 dropStart = stepInTrace.endpos - forward*playerRadius;
						trace_t dropTrace; // drop down again to find new landing
						gameLocal.clip.TracePoint( dropTrace, dropStart, dropStart + landingTraceDownVec, MASK_SOLID | CONTENTS_CLIMBCLIP, self, CONTENTS_NOCLIMBCLIP );

						DEBUG_CLAMBER_ARROW (colorOrange, dropStart, dropTrace.endpos, 512);

						if( dropTrace.fraction < 1.0f )
						{
							idVec3 clamberCheckPos = CheckClamberBounds( dropTrace.endpos + idVec3(0, 0, .1f), (forwardTraceStart + forwardCheck1.endpos)*0.5f, false );
							if (clamberCheckPos != vec3_zero)
							{
								if( (pm_debugclamber.GetInteger() != 0) ) gameLocal.Printf( "\nclamber spot ledge-lip" );
								return clamberCheckPos; 
							}
						}
					}
				}
			}

			landingOK = !ClamberCheckSteppable(landingCheck1.endpos);


			if(landingOK)
			{
				idVec3 clamberCheckPos = CheckClamberBounds( landingCheck1.endpos + idVec3(0, 0, .1f), (forwardTraceStart + forwardCheck1.endpos)*0.5f , false );
				if (clamberCheckPos != vec3_zero)
				{
					if( (pm_debugclamber.GetInteger() != 0) ) gameLocal.Printf( "\nclamber spot ledge" );
					return clamberCheckPos; 
				}
			}

		}

		if (landingOK && firstValidGround == vec3_zero)
		{
			firstValidGround = landingCheck1.endpos;
		}

		// if the forward trace already hit a wall, next iterations will hit the same place, so bail now
		if(forwardHit) { break; }

		forwardTraceStart = forwardCheck1.endpos;
		forwardTraceNext = forwardTraceStart + (forward * forwardCheckDistDelta);
	}

	//ok we've FAILED to find a spot.
	if (firstValidGround != vec3_zero)
	{
		//idVec3 myGravityDir;
		idVec3 perpendicularDir;
		//idMat3 myViewAxis;
		//idVec3 playerOrigin;

		//gameLocal.GetLocalPlayer()->GetViewPos(playerOrigin, myViewAxis);
		//myGravityDir = gameLocal.GetLocalPlayer()->GetPhysics()->GetGravityNormal();
		//perpendicularDir = ( myViewAxis[0] - myGravityDir * (myGravityDir * myViewAxis[0])).Cross(myGravityDir); //Get perpendicular angle

		//Depending on player's viewangle, do a cardinal direction check. This assumes most world geometry is orthogonal.
		if ((viewAngles.yaw < 135 && viewAngles.yaw > 45) || (viewAngles.yaw > -135 && viewAngles.yaw < -45))
		{
			perpendicularDir = idVec3(1, 0, 0);
		}		
		else
		{
			perpendicularDir = idVec3(0, 1, 0);
		}


		//So here we nudge the clamberposition LEFT and RIGHT to try to find a valid spot.
		int distanceArray[] = { 16, -16, 32, -32 };
		for (int i = 0; i < 4; i++)
		{
			trace_t candidateTr;
			int penetrationContents;

			idVec3 candidateSpot = firstValidGround + (perpendicularDir * distanceArray[i]);			

			penetrationContents = gameLocal.clip.Contents(candidateSpot, NULL, mat3_identity, CONTENTS_SOLID, NULL);
			if (penetrationContents & MASK_SOLID)
			{
				//If starting point is inside a solid, then skip...
				continue;
			}
			
			//Check if spot has LOS, to verify we're not digging into a wall or something.
			gameLocal.clip.TracePoint(candidateTr, firstValidGround + idVec3(0, 0, 1), candidateSpot + idVec3(0,0,1), MASK_SOLID | CONTENTS_CLIMBCLIP, NULL);
			
			if (candidateTr.fraction >= 1.0f)
			{
				// check down, we should hit a climbable
				gameLocal.clip.TracePoint(candidateTr, candidateSpot, candidateSpot - idVec3(0,0,playerRadius), MASK_SOLID | CONTENTS_CLIMBCLIP, NULL, CONTENTS_NOCLIMBCLIP);
				idEntity* ent = candidateTr.fraction < 1.0f ? gameLocal.entities[landingCheck1.c.entityNum] : nullptr;
				if (ent && ent->CanClamber() )
				{
					DEBUG_CLAMBER_ARROW(colorOrange,candidateSpot, candidateTr.endpos, DEBUG_CLAMBER_LEDGE_FLAG );

					if( !ClamberCheckSteppable( candidateTr.endpos ) )
					{
						//No obstructions. Do a bounding box check.
						idVec3 clambercheckPos = CheckClamberBounds(candidateTr.endpos, false);
						if (clambercheckPos != vec3_zero)
						{
							if( (pm_debugclamber.GetInteger() != 0) ) gameLocal.Printf( "\nclamber spot ledge-perpendicular" );
							return clambercheckPos;
						}
					}
				}
			}
		}
	}	

	return idVec3(0, 0, 0); //Failed to find a spot.
}

//The cubby check handles layouts where the player is looking into a horizontal shaft. The check shoots into the shaft and finds a suitable place to clamber to.
//We do this check FIRST if the player is looking straight ahead.
idVec3 idPhysics_Player::GetPossibleClamberPos_Cubby(idVec3 eyePos, int numIterations)
{
	const float CUBBY_REACH_HEIGHT = 80;

	idVec3		forward;
	trace_t		forwardCheck, downCheck, upCheck;
	float		clearanceHeight;

	forward = viewAngles.ToForward();
	forward.Normalize();

	// use the horizontal dist instead of the abs dist when moving forward to keep reach consistent
	idVec3 forwardHorizontal = idVec3(forward.x,forward.y, 0);
	float horizontalDistRatio = forwardHorizontal.NormalizeFast();
	horizontalDistRatio = horizontalDistRatio > 0.0f ? 1.0f / horizontalDistRatio : 1.0f;

	// split first and end distances by the number of iterations
	const float forwardCheckDistLast = pm_clamberReachDist.GetInteger();
	float forwardCheckDistFirst = CLAMBER_REACH_INITIAL;
	float forwardCheckDistDelta = (forwardCheckDistLast-forwardCheckDistFirst)/( numIterations > 2 ? numIterations -1 : 1 );

	// don't exceed max cubby height on the forward.z
	if( forward.z * forwardCheckDistLast * horizontalDistRatio > CUBBY_REACH_HEIGHT )
	{
		horizontalDistRatio = CUBBY_REACH_HEIGHT / (forward.z * forwardCheckDistLast + 0.0001f);
	}

	idVec3 forwardAdjusted = forward*horizontalDistRatio;

	// adjust so prone players can't ray trace through ground
	if( eyePos.z < current.origin.z + 0.5f )
	{
		eyePos.z = current.origin.z + 0.5f;
	}

	bool forwardCheckHit = false;
	idVec3 forwardTraceStart = eyePos;
	idVec3 forwardTraceNext = forwardTraceStart;

	for (int i = 0; i < numIterations; i++)
	{
		if( forwardCheckHit )
		{ // if we've hit a wall before, just use the same horizontal position, but with a different height
			forwardTraceNext.z += forward.z*forwardCheckDistDelta;
		}
		else
		{
			forwardTraceStart = forwardTraceNext;
			forwardTraceNext = eyePos + forward*(forwardCheckDistFirst + i*forwardCheckDistDelta);
		}

		//do forward check from eyeball.
		gameLocal.clip.TracePoint(forwardCheck, forwardTraceStart, forwardTraceNext, MASK_SOLID | CONTENTS_CLIMBCLIP, self, CONTENTS_NOCLIMBCLIP);

		if( !forwardCheckHit && forwardCheck.fraction < 1.0f )
		{
			forwardCheckHit = true;
			forwardTraceNext = forwardCheck.endpos + forwardHorizontal*0.5f; // nudge a bit futher horizontally to be past wall collision
		}

		DEBUG_CLAMBER_ARROW(forwardCheckHit ? colorOrange : colorBlue, forwardTraceStart, forwardCheck.endpos);

		//do downward ground check.
		gameLocal.clip.TracePoint(downCheck, forwardCheck.endpos, forwardCheck.endpos + idVec3(0, 0, -CUBBY_REACH_HEIGHT), MASK_SOLID | CONTENTS_CLIMBCLIP, self, CONTENTS_NOCLIMBCLIP);

		DEBUG_CLAMBER_ARROW(colorYellow, forwardCheck.endpos, downCheck.endpos );

		// was ground found
		if (downCheck.fraction >= 1)
		{
			continue;
		}
			
		// Make sure we can clamber onto this
		idEntity* ent = gameLocal.entities[downCheck.c.entityNum];
		if ( ent && !ent->CanClamber() )
		{
			DEBUG_CLAMBER_TEXT( "bad ent", colorRed, downCheck.endpos );
			continue;
		}

		//Make sure the floor is flat enough to stand on
		//common->Printf("downcheck normal %f %f %f\n", downCheck.c.normal.x, downCheck.c.normal.y, downCheck.c.normal.z);
		if (downCheck.c.normal.z < .7f)  //1.0 = flat floor, 0.0 = vertical wall.
			continue;

		//check ceiling clearance.
		gameLocal.clip.TracePoint(upCheck, forwardCheck.endpos, forwardCheck.endpos + idVec3(0, 0, 80), MASK_SOLID | CONTENTS_CLIMBCLIP, self);

		clearanceHeight = upCheck.endpos.z - downCheck.endpos.z;

		bool isCubby = clearanceHeight < pm_normalheight.GetFloat();

		// don't clamber onto low ledges or stairs unless they're actually cubbies
		if( !isCubby && (downCheck.endpos.z <= current.origin.z || ClamberCheckSteppable( downCheck.endpos ) ) )
		{
			DEBUG_CLAMBER_TEXT( "not cubby step", colorRed, downCheck.endpos );
			continue;
		}

		// ceiling too low
		if (clearanceHeight < (pm_crouchheight.GetFloat() + 0.5f))
		{
			DEBUG_CLAMBER_TEXT( "low cubby ceil", colorRed, downCheck.endpos );
			continue;
		}		

		DEBUG_CLAMBER_ARROW( colorBlue, downCheck.endpos + idVec3(0, 0, 8), downCheck.endpos );

		idVec3 candidatePos = downCheck.endpos;

		idMat3 playerViewaxis;
		idVec3 playerOrigin;
		gameLocal.GetLocalPlayer()->GetViewPos(playerOrigin, playerViewaxis);

		// check that there's a clear path to the spot first
		// to avoid phasing through vents, etc
		idVec3 matchedHeightPos = playerOrigin;
		matchedHeightPos.z = candidatePos.z + pm_crouchheight.GetFloat();
		trace_t intoCubbyTrace;
		gameLocal.clip.TracePoint(intoCubbyTrace, matchedHeightPos, candidatePos, MASK_SOLID | CONTENTS_CLIMBCLIP, self);

		idVec3 result = vec3_origin;
		if (intoCubbyTrace.fraction >= 1.0f)
		{
			result = CheckClamberBounds(candidatePos);
		}

		if (result != vec3_zero)
		{
			if( (pm_debugclamber.GetInteger() != 0) ) gameLocal.Printf( "\nclamber spot cubby" );
		}
		else
		{
			//Jiggle the position left and right a bit. This is for situations like clambering up the side of a jagged staircase.
			idVec3 playerDir;
			idVec3 playerGravityDir;

			playerGravityDir = gameLocal.GetLocalPlayer()->GetPhysics()->GetGravityNormal();
			playerDir = (playerViewaxis[0] - playerGravityDir * (playerGravityDir * playerViewaxis[0])).Cross(playerGravityDir); //Get angle perpendicular to 'playerViewaxis'.

			int distanceArray[] = { 8, -8, 16, -16, 24, -24, 32, -32 }; //Do LOS checks on head. Check a few different offsets.

			for (int k = 0; k < 8; k++)
			{
				candidatePos = downCheck.endpos + (playerDir * distanceArray[i]);

				// check that there's a clear path to the spot first
				// to avoid phasing through vents/walls
				matchedHeightPos.z = candidatePos.z + pm_crouchheight.GetFloat();
				gameLocal.clip.TracePoint(intoCubbyTrace, matchedHeightPos, candidatePos, MASK_SOLID | CONTENTS_CLIMBCLIP, self);

				DEBUG_CLAMBER_ARROW(intoCubbyTrace.fraction >= 1.0f ? colorWhite : colorYellow, matchedHeightPos, candidatePos, 512);

				if (intoCubbyTrace.fraction >= 1.0f)
				{ // path clear, check the clamber spot to see if player can fit
					result = CheckClamberBounds(candidatePos);
					if (result != vec3_zero)
					{
						if ((pm_debugclamber.GetInteger() != 0)) gameLocal.Printf("\nclamber spot cubby jiggle");
						break;
					}
				}
			}
		}

		// if cubby on floor, check for low ledge, ie table or chair
		if( isCubby && result != vec3_zero && ClamberCheckCubbyLowLedge(result) )
		{
			result = vec3_zero;
		}

		return result; //No spot found.
	}

	//TODO: jiggle boundingbox check upward in case it's a ramped surface.

	return idVec3(0, 0, 0);
}

float idPhysics_Player::GetClamberLerp(void)
{
	if (gameLocal.time >= this->clamberStateTime)
		return 1.0f;

	float currentTime = gameLocal.time;
	float lookTimeMax = this->clamberStateTime;
	//float lookTimeMin = this->clamberStartTime;

	currentTime -= this->clamberStartTime;
	lookTimeMax -= this->clamberStartTime;

	return (currentTime / lookTimeMax);
}

//This checks if a given position has clearance for the player to clamber to it.
idVec3 idPhysics_Player::CheckClamberBounds(idVec3 basePos, idVec3 sweepStart, bool doExtraChecks)
{
	if(!CheckClamberHeight(basePos.z))
	{
		DEBUG_CLAMBER_TEXT( "too high", colorRed, basePos );
		return vec3_zero;
	}

	DEBUG_CLAMBER_ARROW (colorBrown,  sweepStart, basePos, 512);

	const int MAX_VERTICAL_CHECKDISTANCE = 8; //max height player can be from ground surface.
	const int CHECKDISTANCE_STEP = 4;

	idBounds	playerBounds;
	trace_t		trace, searchTrace, downTrace;
	idAngles	viewAng;
	idVec3		backward;

	//Get player bounding box. This is for crouch position, to make it viable for player to enter smaller spaces.
	playerBounds = clipModel->GetBounds();
	playerBounds.ExpandSelf(1);
	playerBounds[0][2] = 0; //lower vertical bound.
	playerBounds[1][2] = pm_crouchheight.GetFloat() + 1; //upper vertical bound. if we don't Expand the bounding box a bit, the player sometimes clips into the wall and it creates all sorts of chaos


	{ //Do a tracebounds check at the position.
		gameLocal.clip.TraceBounds(trace, basePos, basePos, playerBounds, MASK_SOLID | CONTENTS_CLIMBCLIP, self);
		bool boundsOK = trace.fraction >= 1;

		if ( developer.GetInteger() >= 2 || (pm_debugclamber.GetInteger() & 2) )
		{
			gameRenderWorld->DebugBounds(boundsOK ? colorCyan : colorRed, playerBounds, basePos, pm_debugclambertime.GetInteger());
		}

		//The position is unobstructed. Success. Return the value.
		if (boundsOK)
		{
			if( (pm_debugclamber.GetInteger() != 0) ) gameLocal.Printf( "\nbounds basic ok" );
			return basePos; //Success. Spot is good.
		}
	}

#if 1
	{ 
		// blendo eric: try normal nudge, then try sweeping bounds trace
		idVec3 basePosAdj = basePos + trace.c.normal*(playerBounds.GetRadius()*0.5f+0.01f);
		trace_t traceNorm;
		gameLocal.clip.TraceBounds(traceNorm, basePosAdj, basePosAdj, playerBounds, MASK_SOLID | CONTENTS_CLIMBCLIP, self);
		bool boundsOK = traceNorm.fraction >= 1 && CheckClamberHeight(basePosAdj.z);

		if ( developer.GetInteger() >= 2 || (pm_debugclamber.GetInteger() & 1) )
		{
			gameRenderWorld->DebugBounds(colorMagenta, playerBounds, basePosAdj, 10000);
		}

		if( !boundsOK )
		{ // if nudging doesn't work, try bounds trace
			idVec3 adjustedSweepStart = sweepStart;

			// make sure sweep already starts within max clamber range
			adjustedSweepStart.z = adjustedSweepStart.z < GetClamberMaxHeightWorld() ? adjustedSweepStart.z : GetClamberMaxHeightWorld();

			trace_t sweepTrace;
			// pre-check initial sweep position (workaround for trace fraction error in tracebounds code, which doesn't return 0 on starting collision )
			gameLocal.clip.TraceBounds(sweepTrace, adjustedSweepStart, adjustedSweepStart, playerBounds, MASK_SOLID | CONTENTS_CLIMBCLIP, self, 0);
			boundsOK = sweepTrace.fraction >= 1;

			if ( (pm_debugclamber.GetInteger() & 1) )
			{
				gameRenderWorld->DebugBounds(boundsOK ? colorDkGrey : colorBlack, playerBounds, adjustedSweepStart, pm_debugclambertime.GetInteger());
			}

			if(boundsOK)
			{
				// start actual sweep
				gameLocal.clip.TraceBounds(sweepTrace, adjustedSweepStart, basePos, playerBounds, MASK_SOLID | CONTENTS_CLIMBCLIP, self, 0);
				boundsOK = sweepTrace.fraction > 0;
				if( boundsOK )
				{
					// derive point of collision, then nudge it to prevent it clipping
					idVec3 sweepDir = (adjustedSweepStart - basePos);
					float sweepDist = sweepDir.NormalizeFast();
					basePosAdj = (sweepDist*(1.0f-sweepTrace.fraction) + 0.1f)*sweepDir + basePos;

					boundsOK = CheckClamberHeight(basePosAdj.z);
				}
			}
		}

		if (boundsOK)
		{  
			// if a valid ledge landing was found (below max clamber height)
			// see if ww can step up to the original position (possibly above clamber height)
			// to prevent hanging off the ledge

			// calculate the amount needed to step up based on ramp angle
			idVec3 runVec = trace.c.normal;
			runVec.z = 0.0f;
			float pctGrade = ((runVec.LengthFast()+0.00001f)/(traceNorm.c.normal.z+0.00001f)); // rise over run
			if( pctGrade > 2.0f ) pctGrade = 2.0f;
			float stepUpDist = 1.0f + playerBounds.GetRadius() * 0.5f * pctGrade;

			// basePos is already under max height, so at most this should be maxHeight + stepUpDist
			idVec3 basePosStepped = basePos + idVec3(0, 0, stepUpDist);

			trace_t stepTrace;
			gameLocal.clip.TraceBounds(stepTrace, basePosStepped, basePosStepped, playerBounds, MASK_SOLID | CONTENTS_CLIMBCLIP, self);

			// use the stepped up position if valid, otherwise fallback onto the old adjusted position
			if( stepTrace.fraction >= 1 )
			{	
				// no need to check if it's in max clamber range
				// as we already know the previous spot was valid, and this one is only a step away
				basePosAdj = basePosStepped;
			}

			if ( (pm_debugclamber.GetInteger() & 1) )
			{
				gameRenderWorld->DebugBounds(colorYellow, playerBounds, basePosStepped+idVec3(0, 0, 0.1f), pm_debugclambertime.GetInteger());
				gameRenderWorld->DebugBounds(colorGreen, playerBounds, basePosAdj, pm_debugclambertime.GetInteger());

				// debug checking for any issues
				if( gameLocal.clip.TraceBounds(traceNorm, basePosAdj, basePosAdj, playerBounds, MASK_SOLID | CONTENTS_CLIMBCLIP, self) )
				{
					common->Warning("\nclamber bounds check returned inconsistent results in normal nudge check");
				}
			}

			if( (pm_debugclamber.GetInteger() != 0) ) gameLocal.Printf( "\nbounds sweep ok" );
			return basePosAdj; //Success. Spot is good.
		}
	}
#endif
#if 0
		{ // blendo eric: use sweeping trace
			trace_t sweepTrace;
			idVec3 adjustedSweepStart = sweepStart;

			// make sure sweep already starts within max clamber range
			if( adjustedSweepStart.z > GetClamberMaxHeightWorld() )
			{
				adjustedSweepStart.z = GetClamberMaxHeightWorld();
			}

			// pre-check initial sweep position (workaround for trace fraction error in tracebounds code, which doesn't return 0 on starting collision )
			gameLocal.clip.TraceBounds(sweepTrace, adjustedSweepStart, adjustedSweepStart, playerBounds, MASK_SOLID | CONTENTS_CLIMBCLIP, self, 0);
			bool boundsOK = sweepTrace.fraction >= 1;

			if ( (pm_debugclamber.GetInteger() & 1) )
			{
				gameRenderWorld->DebugBounds(boundsOK ? colorDkGrey : colorBlack, playerBounds, adjustedSweepStart, pm_debugclambertime.GetInteger());
			}

			if(boundsOK)
			{
				// start actual sweep
				gameLocal.clip.TraceBounds(sweepTrace, adjustedSweepStart, basePos, playerBounds, MASK_SOLID | CONTENTS_CLIMBCLIP, self, 0);
				boundsOK = sweepTrace.fraction > 0;
				if( boundsOK )
				{
					// derive point of collision
					const float nudgeDist = 0.1f; // keep it from clipping exactly into wall
					idVec3 sweepDir = (adjustedSweepStart - basePos);
					float sweepDist = sweepDir.NormalizeFast();
					idVec3 basePosAdj = (sweepDist*(1.0f-sweepTrace.fraction) + nudgeDist)*sweepDir + basePos;

					boundsOK = CheckClamberHeight(basePosAdj.z);
			
					if ( boundsOK )
					{ // if we have a proper ledge landing
						// see if there's a further in landing to step up to so were not hanging off the ledge

						// calculate the amount needed to step up based on ramp angle
						idVec3 runVec = sweepTrace.c.normal;
						runVec.z = 0.0f;
						float pctGrade = ((runVec.LengthFast()+0.00001f)/(sweepTrace.c.normal.z+0.00001f)); // rise over run
						if( pctGrade > 2.0f ) pctGrade = 2.0f;
						float stepUpDist = 1.0f + playerBounds.GetRadius() * 0.5f * pctGrade;

						// basePos is already under max height, so at most this should be maxHeight + stepUpDist
						idVec3 basePosStepped = basePos + idVec3(0, 0, stepUpDist);

						trace_t stepTrace;
						gameLocal.clip.TraceBounds(stepTrace, basePosStepped, basePosStepped, playerBounds, MASK_SOLID | CONTENTS_CLIMBCLIP, self);
 						if( stepTrace.fraction >= 1 )
						{ // stepped up position is good, so use it for final position
							basePosAdj = basePosStepped;
						}

						if ( (pm_debugclamber.GetInteger() & 1) )
						{
							gameRenderWorld->DebugBounds(colorYellow, playerBounds, basePosStepped+idVec3(0, 0, 0.1f), pm_debugclambertime.GetInteger());
							gameRenderWorld->DebugBounds(colorGreen, playerBounds, basePosAdj, pm_debugclambertime.GetInteger());

							// debug checking for any issues
							if( gameLocal.clip.TraceBounds(sweepTrace, basePosAdj, basePosAdj, playerBounds, MASK_SOLID | CONTENTS_CLIMBCLIP, self) )
							{
								common->Warning("\nclamber bounds check returned inconsistent results in sweeping check");
							}
						}

						return basePosAdj;
					}
				}
			}
		}
#endif

	//Failed.
	{ //Try again but with the box rotated up by 45 degrees. TODO: Add comment here why we do this.
		idAngles angles;
		angles.Set( -45.0f, 0.0f, 0.0f );
		gameLocal.clip.TraceBounds( trace, basePos, basePos, playerBounds, MASK_SOLID | CONTENTS_CLIMBCLIP, self, 0, angles.ToMat3());

		bool boundsOK = trace.fraction >= 1;

		if ( developer.GetInteger() >= 2 || (pm_debugclamber.GetInteger() & 2) )
		{
			idBox box;
			idVec3 points[8];
			playerBounds.ToPoints( points );
			box.FromPoints( points, 8 );
			box.RotateSelf( angles.ToMat3() );
			box.TranslateSelf( basePos );
			gameRenderWorld->DebugBox( boundsOK ? colorCyan : colorRed, box, pm_debugclambertime.GetInteger() );
		}

		//The rotated box worked. Success. Return the value.
		if ( boundsOK )
		{
			if( (pm_debugclamber.GetInteger() != 0) ) gameLocal.Printf( "\nbounds rot ok" );
			return basePos; //Success. Spot is good.
		}
	}


	//Failed.
	//Do another check where we elevate the position up a few units. This is to handle the situation where surface we're clambering onto
	//is not perfectly flat. For example, if it's a sloped surface, the bounding box check will fail ; therefore we'll need to do an
	//additional check where we raise up the position a little bit.
	for (int i = CHECKDISTANCE_STEP; i <= MAX_VERTICAL_CHECKDISTANCE; i += CHECKDISTANCE_STEP)
	{
		idVec3 adjustedPosition = basePos + idVec3(0, 0, i);

		if(!CheckClamberHeight(adjustedPosition.z))
		{
			break;
		}

		gameLocal.clip.TraceBounds(trace, adjustedPosition, adjustedPosition, playerBounds, MASK_SOLID | CONTENTS_CLIMBCLIP, self);

		bool boundsOK = trace.fraction >= 1;

		if ( developer.GetInteger() >= 2 || (pm_debugclamber.GetInteger() & 2) )
		{
			gameRenderWorld->DebugBounds(boundsOK ? colorCyan : colorRed, playerBounds, adjustedPosition, pm_debugclambertime.GetInteger());
		}

		if (boundsOK)
		{
			if( (pm_debugclamber.GetInteger() != 0) ) gameLocal.Printf( "\nbounds elevate ok" );
			return adjustedPosition; //Success. Spot is good.
		}
	}

	
	if (doExtraChecks)
	{
		//Hunt for a spot that the player can clamber to.
		viewAng = this->viewAngles;
		viewAng.pitch = 0;
		viewAng.roll = 0;
		viewAng.yaw = viewAng.yaw + 180;
		backward = viewAng.ToForward();
		backward.Normalize();

		//Iterate backwards, toward the player's location. 
		for (int i = 1; i < 16; i++)
		{
			idVec3 newPos = basePos + (backward * (i * 4));
			gameLocal.clip.TraceBounds(searchTrace, newPos, newPos, playerBounds, MASK_SOLID | CONTENTS_CLIMBCLIP, self);

			if (searchTrace.fraction >= 1)
			{
				break;
			}
		}

		if (searchTrace.fraction < 1)
		{
			//Failed to find spot. Exit.
			return idVec3(0, 0, 0);
		}

		//trace down and verify this spot still has ground beneath it.
		gameLocal.clip.TraceBounds(downTrace, searchTrace.endpos, searchTrace.endpos + idVec3(0, 0, -16), playerBounds, MASK_SOLID | CONTENTS_CLIMBCLIP, self);

		//Verify the final spot is solid ground & is higher than the starting position.
		if (downTrace.fraction < 1 && downTrace.endpos.z > current.origin.z && CheckClamberHeight(searchTrace.endpos.z) )
		{
			if ( developer.GetInteger() >= 2 || (pm_debugclamber.GetInteger() & 2) )
			{
				gameRenderWorld->DebugBounds(colorCyan, playerBounds, searchTrace.endpos, pm_debugclambertime.GetInteger());
			}

			if( (pm_debugclamber.GetInteger() != 0) ) gameLocal.Printf( "\nbounds backstep ok" );
			return searchTrace.endpos;
		}
	}

	DEBUG_CLAMBER_TEXT( "no space", colorRed, basePos );
	return idVec3(0, 0, 0);
}

bool idPhysics_Player::ClamberCheckCubbyLowLedge(idVec3 cubbyPos)
{
	const float FLOOR_CHECK_MARGIN = 2.5f;
	const float LEDGE_ABOVE_HEIGHT = 24.0f;
	const idVec3 LEDGE_ABOVE_VEC = idVec3(0.0f, 0.0f, LEDGE_ABOVE_HEIGHT);

	// if cubby on floor, check for low ledge, ie table or chair
	if( idMath::Abs(cubbyPos.z - clamberGroundPosCurrent.z) <= FLOOR_CHECK_MARGIN )
	{
		idVec3 ledgeAbovePos = cubbyPos + LEDGE_ABOVE_VEC;
		idVec3 ledgeAboveSweepPos = current.origin + LEDGE_ABOVE_VEC;
		idVec3 ledgeAboveResult =  CheckClamberBounds(ledgeAbovePos,ledgeAboveSweepPos, false );

		DEBUG_CLAMBER_ARROW(ledgeAboveResult!= vec3_zero ? colorMagenta : colorBlue,ledgeAboveSweepPos,ledgeAbovePos,4);

		if( ledgeAboveResult != vec3_zero )
		{
			DEBUG_CLAMBER_TEXT( "ledge above cubby", colorRed, cubbyPos );
			return true;
		}
		else
		{
			DEBUG_CLAMBER_TEXT( "cubby no ledge", colorBlue, cubbyPos );
		}
	}
	return false;
}

void idPhysics_Player::UpdateAcro()
{
	//if (acroType == ACROTYPE_SPLITS)
	//{
	//	//splits mode.
	//	float	scale, wishspeed;
	//	idVec3	wishvel, wishdir, acroLeft, eyePos;
	//	trace_t traceLeft, traceRight;
	//	
	//	//exit splits mode.
	//	idPhysics_Player::CheckJump();
	//}
	//else

	bool crouchPressed = command.prevButtonState[UB_DOWN] <= 0 && command.buttonState[UB_DOWN] > 0 && !common->IsConsoleActive();
	bool jumpPressed = command.prevButtonState[UB_UP] <= 0 && command.buttonState[UB_UP] > 0 && !common->IsConsoleActive();

	if (acroType == ACROTYPE_CEILINGHIDE || acroType == ACROTYPE_SPLITS)
	{
		if (crouchPressed || jumpPressed)
		{
			//exit acro mode. fall down.

			if (acroType == ACROTYPE_CEILINGHIDE)
			{
				//move player down a bit so that their head doesn't get stuck in the ceiling.
				trace_t tr;
				idBounds playerBound;

				playerBound = idBounds(idVec3(-16, -16, 0), idVec3(16, 16, 1));
				gameLocal.clip.TraceBounds(tr, self->GetPhysics()->GetOrigin(), self->GetPhysics()->GetOrigin() + idVec3(0, 0, 32), playerBound, MASK_SOLID, self);

				//move the player down to prevent their head clipping into ceiling geometry.
				self->GetPhysics()->SetOrigin(tr.endpos + idVec3(0, 0, -pm_normalheight.GetInteger() ));
			}
			else
			{
				if (cargohideEnt.IsValid())
				{
					//Exiting splits position. Move player forward a bit.
					idAngles forwardAngle = cargohideEnt.GetEntity()->GetPhysics()->GetAxis().ToAngles();
					idVec3 exitPos = self->GetPhysics()->GetOrigin() + (forwardAngle.ToForward() * 12);
					self->GetPhysics()->SetOrigin(exitPos);
				}
			}

			this->acroType = ACROTYPE_NONE;
			this->clamberState = CLAMBERSTATE_NONE;
			((idPlayer *)self)->SetViewPosActive(false, idVec3(0, 0, 0));
			
			if (cargohideEnt.IsValid())
			{
				cargohideEnt.GetEntity()->isFrobbable = true; //make acropoint frobbable again.
			}

			((idPlayer *)self)->SetBodyAngleLock(false);

			return;
		}
	}
	else if (acroType == ACROTYPE_CARGOHIDE)
	{
		if (jumpPressed)
		{		
			this->acroType = ACROTYPE_NONE;
			this->clamberState = CLAMBERSTATE_NONE;

			if (cargohideEnt.IsValid())
			{
				idBounds playerBounds;
				float cargoLength;
				float playerRadius;
				trace_t exitTr;
				idVec3 exitPos;
				bool exitSuccess = false;
				idVec3 cargoForward;
				

				playerBounds = clipModel->GetBounds(); //player bounds.
				playerBounds[1][2] = pm_crouchheight.GetFloat();
				playerRadius = playerBounds[1].x;
				cargoLength = cargohideEnt.GetEntity()->GetPhysics()->GetBounds()[1].x; //Get the length of the cargo prop. [0].x, [0].y, [0].z = mins, [1].x, [1].y, [1].z = maxs.				
				cargohideEnt.GetEntity()->GetPhysics()->GetAxis().ToAngles().ToVectors(&cargoForward, NULL, NULL);
				
				//First, attempt to go to a position forward of the cargo.
				for (int i = 0; i < 64; i += 16)
				{
					exitPos = cargohideEnt.GetEntity()->GetPhysics()->GetOrigin();
					exitPos.z = self->GetPhysics()->GetOrigin().z;
					exitPos += (cargoForward * (playerRadius + cargoLength + 1)) + idVec3(0, 0, -16) + (cargoForward * i);

					gameLocal.clip.TraceBounds(exitTr, exitPos, exitPos, playerBounds, MASK_SOLID, self);
					//gameRenderWorld->DebugBounds(colorLtGrey, playerBounds, exitPos, 100000);

					if (exitTr.fraction >= 1)
					{
						exitSuccess = true;
						i = 9999; //break out of loop.
					}
				}				

				if (!exitSuccess)
				{
					//Ok, failed to find a spot forward of the cargo prop. So, attempt to find a spot beneat the cargo prop.
					float cargoHeight;
					cargoHeight = cargohideEnt.GetEntity()->GetPhysics()->GetBounds()[0].z; //Get height of cargo prop.

					exitPos = cargohideEnt.GetEntity()->GetPhysics()->GetOrigin();
					exitPos += (cargoForward * 16) + idVec3(0, 0, cargoHeight) + idVec3(0, 0, -pm_crouchheight.GetFloat()) + idVec3(0,0, -1);

					gameLocal.clip.TraceBounds(exitTr, exitPos, exitPos, playerBounds, MASK_SOLID, self);
					//gameRenderWorld->DebugBounds(colorLtGrey, playerBounds, exitPos, 100000);

					if (exitTr.fraction >= 1)
					{
						//all clear! Great.
						exitSuccess = true;
					}
				}

				if (exitSuccess)
				{
					//Found a exit spot. Great!					
					cargohideEnt.GetEntity()->StartSound("snd_exit", SND_CHANNEL_ANY, 0, false, NULL);
					cargohideEnt.GetEntity()->isFrobbable = true; //TODO: add a short time delay to this....					
					StartClamber(exitPos);
				}
				else
				{
					//FAiled to find an exit spot.
					self->StartSound("snd_error", SND_CHANNEL_ANY, 0, false, NULL);
					this->acroType = ACROTYPE_CARGOHIDE;
					this->clamberState = CLAMBERSTATE_ACRO;
					return;
				}				

				cargohideEnt = NULL;
			}
			
			return;
		}

		// don't allow toggle crouch, since crouch doesn't exit cargohides
		// otherwise it'll require two jumps (first to untoggle) to exit
		gameLocal.GetLocalPlayer()->UnsetToggleCrouch();
	}
}



void idPhysics_Player::SetFallState(bool hardFall)
{
	//if already down, then don't start new fall.
	if (fallenState != FALLEN_NONE)
		return;

	if (hardFall)
		fallenState = FALLEN_HEADONGROUND;
	else
		fallenState = FALLEN_IDLE;

	fallenTimer = gameLocal.time + FALLEN_HEADDOWN_TIME;
}

int idPhysics_Player::GetFallState()
{
	return fallenState;
}

//This IMMEDIATELY exits the fall state. This skips the player get-up animation. This should only be used if you absolutely have to exit the fall state in one frame.
void idPhysics_Player::SetImmediateExitFallState()
{
	fallenState = FALLEN_NONE;
}

void idPhysics_Player::UpdateFallState()
{
	current.movementFlags |= PMF_DUCKED;

	if (fallenState == FALLEN_HEADONGROUND)
	{
		//Head is on ground. Stay here for a moment.
		if (gameLocal.time >= fallenTimer)
		{
			ClearPushedVelocity();
			current.velocity = vec3_origin;

			fallenState = FALLEN_RISING;
			fallenTimer = gameLocal.time + FALLEN_HEADRISE_TIME;

			if (!gameLocal.GetLocalPlayer()->IsInMech())
			{
				gameLocal.GetLocalPlayer()->SetViewPitchLerp(0);
			}
		}
	}
	else if (fallenState == FALLEN_RISING)
	{
		//Head was on ground, player is now rising upward to a crouch position.
		if (gameLocal.time >= fallenTimer)
		{
			//Player now has control of character again.
			fallenState = FALLEN_IDLE;
			gameLocal.GetLocalPlayer()->SetArmVisibility(false);
		}
	}
	else if (fallenState == FALLEN_IDLE)
	{
		//Player is crawling/rolling around in fallen state.
		if ( groundPlane )
		{
			idPhysics_Player::WalkMove();
		}
		else
		{
			idPhysics_Player::AirMove();
		}
	}
	else if (fallenState == FALLEN_GETUP_KICKUP)
	{
		//Player pressed spacebar and is playing animation to rise back to an upright position.
		if (gameLocal.time >= fallenTimer + FALLEN_KICKUP_TIME)
		{
			idAngles viewForwardAng = idAngles(0, gameLocal.GetLocalPlayer()->viewAngles.yaw, 0);
			idAngles fxAng = idAngles(180, 0, 0);
			idVec3 fxPos = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() + (viewForwardAng.ToForward() * 32) + idVec3(0,0,64);

			//Player done with kickup animation. Exit fall state.
			fallenState = FALLEN_NONE;
			fallenTimer = gameLocal.time + 300;
			gameLocal.GetLocalPlayer()->SetFallState(false, false);

			((idPlayer *)self)->SetViewPitchLerp(0, 300);

			//gameRenderWorld->DebugSphere(colorGreen, idSphere(fxPos, 3), 10000);
			idEntityFx::StartFx("fx/wind_local_short", fxPos, fxAng.ToMat3());			

			DoJump(); //Do a little hop when getting back up.
		}
	}
}

void idPhysics_Player::ForceDuck(int howlong)
{
	if (howlong > 0)
	{
		forceduckTimer = gameLocal.time + howlong;
	}

	current.movementFlags |= PMF_DUCKED;	
}

//Make player exit duck/crouch state.
void idPhysics_Player::ForceUnduck()
{
	current.movementFlags &= ~PMF_DUCKED;
}

void idPhysics_Player::ZippingTo(idVec3 destination, idWinding* aperture, float forceDuckDuration)
{
	//if player is in fallen state, then un-fall the player
	fallenState = FALLEN_NONE;
	gameLocal.GetLocalPlayer()->SetFallState(false, false);

	zippingOrigin = current.origin;
	zippingCameraStart = gameLocal.GetLocalPlayer()->firstPersonViewOrigin;
	zippingCameraEnd = destination;
	zippingTimer = gameLocal.time;
	zippingState = ZIPPINGSTATE_PAUSE;
	zipForceDuckDuration = forceDuckDuration;

	if (aperture)
	{
		// Get a shrunken version of the aperture for our camera to pass through
		idWinding* shrunkenAperture = new idWinding();
		int i;
		for (i = 0; i < aperture->GetNumPoints(); i++)
		{
			idVec3 currentPoint = (*aperture)[i].ToVec3();
			idVec3 prevNeighbour = ((i > 0) ? (*aperture)[i - 1] : (*aperture)[aperture->GetNumPoints() - 1]).ToVec3();
			idVec3 nextNeighbour = ((i < aperture->GetNumPoints() - 1) ? (*aperture)[i + 1] : (*aperture)[0]).ToVec3();

			// For a corner of the portal, get a vector pointing inward and travel along it according to our margin size
			idVec3 a = prevNeighbour - currentPoint;
			idVec3 b = nextNeighbour - currentPoint;
			a.NormalizeFast();
			b.NormalizeFast();

			idVec3 internalVector = a + b;
			internalVector.NormalizeFast();

			internalVector *= APERTURE_MARGIN;

			shrunkenAperture->AddPoint(idVec5(currentPoint + internalVector, idVec2(0, 0)));
		}

		// Push the aperture slightly out from the vent's actual position, to account for the player approaching from very skew angles
		idPlane plane;
		shrunkenAperture->GetPlane(plane);
		idVec3 apertureOffset = idVec3(plane.Normal());
		
		apertureOffset *= APERTURE_OFFSET_DISTANCE;

		// If we're on the back side of the plane, flip the offset vector so it pushes 'out'
		if (plane.Side(zippingCameraStart) == PLANESIDE_BACK)
			apertureOffset *= -1;

		for (i = 0; i < shrunkenAperture->GetNumPoints(); i++)
		{
			(*shrunkenAperture)[i].x += apertureOffset.x;
			(*shrunkenAperture)[i].y += apertureOffset.y;
			(*shrunkenAperture)[i].z += apertureOffset.z;
		}

		// Debug: draw the outline of the shrunken aperture
		if (developer.GetInteger() > 0)
		{
			for (i = 0; i < shrunkenAperture->GetNumPoints(); i++)
			{
				idVec3 lineStart = (*shrunkenAperture)[i].ToVec3();
				idVec3 lineEnd = (i == shrunkenAperture->GetNumPoints() - 1) ? (*shrunkenAperture)[0].ToVec3() : (*shrunkenAperture)[i + 1].ToVec3();
				gameRenderWorld->DebugLine(idVec4(1, 1, 0, 1), lineStart, lineEnd, 10000);
			}
		}

		// SW: We're passing through the aperture between the start point and the end point, and need to pick a midpoint that is somewhere in the aperture
		// Imagine pulling a piece of string taut through the aperture. Either it passes through cleanly, or it wraps around one of the edges.
		// First we draw a straight line between the start and end, and find where it passes through the plane defined by the aperture.

		idVec3 planePoint = shrunkenAperture->GetCenter();
		idVec3 planeNormal = plane.Normal();
		idVec3 lineDirection = zippingCameraEnd - zippingCameraStart;

		// We know we *will* pass through the plane, so we can solve a line-plane intersection like this:
		float d = DotProduct(planePoint - zippingCameraStart, planeNormal) / DotProduct(lineDirection, planeNormal);
		idVec3 intersectionPoint = zippingCameraStart + lineDirection * d;

		// SW: The point of intersection is either inside the aperture (in which case who cares? it's good),
		// or outside the aperture (in which case we need to find the closest point to it around the edges)
		//
		// A point is inside a convex polygon if it is on the same 'side' of every edge.
		// In the below loop, we pick an initial 'side' on the first iteration, then compare each subsequent iteration to make sure they're all the same.
		// 
		// Rather than go through a bunch of headache-inducing transformations to turn this into a 2D problem,
		// we just turn each side of the aperture into a plane with a normal facing 'out', then check plane sides instead of polygon sides

		int lastSide = 0;
		bool isInside = true;
		for (i = 0; i < shrunkenAperture->GetNumPoints(); i++)
		{
			idVec3 currentPoint = (*shrunkenAperture)[i].ToVec3();
			idVec3 nextNeighbour = ((i < shrunkenAperture->GetNumPoints() - 1) ? (*shrunkenAperture)[i + 1] : (*shrunkenAperture)[0]).ToVec3();
			idVec3 n = (nextNeighbour - currentPoint).Cross(planeNormal);
			idPlane* sidePlane = new idPlane(n, 0);
			sidePlane->FitThroughPoint(currentPoint);
			int side = sidePlane->Side(intersectionPoint);

			if (side != PLANESIDE_FRONT && side != PLANESIDE_BACK) // Resolve ambiguity
			{
				side = PLANESIDE_FRONT;
			}

			if (i == 0) // We only want to compare ourselves to the last iteration. If this is the first iteration, just populate the data.
			{
				lastSide = side;
			}
			else
			{
				if (lastSide != side)
				{
					isInside = false;
					break;
				}
			}
		}

		if (!isInside)
		{
			// Our point is outside the aperture, so we need to find the closest point around the edge of the aperture.
			// We can do this by treating each side of the aperture as a plane (as above).
			// By drawing a line from our current point to the center of the aperture, we'll intersect through one or more of these planes.
			// The closest one to the destination will be the 'real' intersection.
			idVec3 closestIntersection = vec3_origin;
			float closestDistance = -1;
			for (i = 0; i < shrunkenAperture->GetNumPoints(); i++)
			{
				idVec3 currentPoint = (*shrunkenAperture)[i].ToVec3();
				idVec3 nextNeighbour = ((i < shrunkenAperture->GetNumPoints() - 1) ? (*shrunkenAperture)[i + 1] : (*shrunkenAperture)[0]).ToVec3();
				idVec3 n = (nextNeighbour - currentPoint).Cross(planeNormal);
				n.NormalizeFast();

				// Debug: draw the normal of our side plane
				if (developer.GetInteger() > 0)
				{
					idVec3 startPoint = currentPoint + (nextNeighbour - currentPoint) * 0.5f;
					idVec3 endPoint = startPoint + n * 8;
					gameRenderWorld->DebugArrow(idVec4(0, 0.5, 0.5, 1), startPoint, endPoint, 1, 10000);
				}

				idPlane* sidePlane = new idPlane(n, 0);
				sidePlane->FitThroughPoint(currentPoint);

				// If we start and end on different sides of the plane, we need to figure out where we cross over -- otherwise, we can ignore it
				if (sidePlane->Side(intersectionPoint) != sidePlane->Side(shrunkenAperture->GetCenter()))
				{
					idVec3 lineDirection = shrunkenAperture->GetCenter() - intersectionPoint;
					float d = DotProduct(currentPoint - intersectionPoint, n) / DotProduct(lineDirection, n);

					idVec3 candidateClosestIntersection = intersectionPoint + lineDirection * d;
					float candidateClosestDistance = (candidateClosestIntersection - shrunkenAperture->GetCenter()).LengthFast();

					// Assign our initial value *or* a better value
					if (closestDistance == -1 || candidateClosestDistance < closestDistance)
					{
						closestDistance = candidateClosestDistance;
						closestIntersection = candidateClosestIntersection;
					}
				}
			}

			// Use whatever we have by the end of the loop -- it *should* be a point around the edge of the aperture
			zippingCameraMidpoint = closestIntersection;
		}
		else
		{
			// We can use the intersection point as-is
			zippingCameraMidpoint = intersectionPoint;
		}
		// Debug: draw where we wrap our line around
	}
	else
	{
		// We don't have a valid aperture for whatever reason, fall back on the centre of the line between the start and end
		zippingCameraMidpoint = zippingCameraStart + (zippingCameraEnd - zippingCameraStart) * 0.5f;
	}
	if (developer.GetInteger() > 0)
	{
		gameRenderWorld->DebugLine(idVec4(1, 0, 0, 1), zippingCameraStart, zippingCameraMidpoint, 10000);
		gameRenderWorld->DebugLine(idVec4(0.8, 0.2, 0, 1), zippingCameraMidpoint, zippingCameraEnd, 10000);
	}
	
}

//Zipping = entering a vent.
//return TRUE if player is zipping.
bool idPhysics_Player::GetZippingState()
{
	return (zippingState != ZIPPINGSTATE_NONE);
}

void idPhysics_Player::UpdateZipping()
{
	if (zippingState == ZIPPINGSTATE_PAUSE)
	{
		//This is the pause that happens BEFORE the zip movement starts. This is so the player has a chance to briefly see the door/vent/etc visually moving.
		float		lerp;
		lerp = (gameLocal.time - zippingTimer) / ZIPPAUSETIME;
		lerp = idMath::ClampFloat(0, 1, lerp);

		SetOrigin(zippingOrigin); //Lock player in place.

		if (lerp >= 1)
		{
			if ( zipForceDuckDuration > 0.0f ) {
				ForceDuck( zipForceDuckDuration );
			}
			zippingState = ZIPPINGSTATE_DASHING;
			gameLocal.GetLocalPlayer()->SetViewPitchLerp(0, ZIPPING_PITCHTIME);
			zippingTimer = gameLocal.time;
		}
	}
	else if (zippingState == ZIPPINGSTATE_DASHING)
	{
		idVec3		newPosition;
		float		lerp;
		
		lerp = (gameLocal.time - zippingTimer) / (float)ZIPPING_TIME;
		
		if (lerp > 1)
			lerp = 1;

		lerp = idMath::CubicEaseOut(lerp);

		// SW: For the first half of the dash, we lerp from the start to the midpoint,
		// and for the second half of the dash, we lerp from the midpoint to the end.
		//
		// The eye position is what really matters here, but we still need to set the player's position,
		// hence the offset based on eye height. This offset will change over time (due to the player crouching),
		// so it's important that we re-check it every time we UpdateZipping()
		idVec3 origin;
		idVec3 dest;
		if (lerp <= 0.5f)
		{
			origin = zippingCameraStart - idVec3(0, 0, 1) * gameLocal.GetLocalPlayer()->EyeHeight();
			dest = zippingCameraMidpoint - idVec3(0, 0, 1) * gameLocal.GetLocalPlayer()->EyeHeight();
			lerp *= 2.0f;
		}
		else
		{
			origin = zippingCameraMidpoint - idVec3(0, 0, 1) * gameLocal.GetLocalPlayer()->EyeHeight();
			dest = zippingCameraEnd;
			lerp = (lerp - 0.5f) * 2.0f;
		}

		
		newPosition.x = idMath::Lerp(origin.x, dest.x, lerp);
		newPosition.y = idMath::Lerp(origin.y, dest.y, lerp);
		newPosition.z = idMath::Lerp(origin.z, dest.z, lerp);
		SetOrigin(newPosition);

		if (lerp >= 1)
		{
			zippingState = ZIPPINGSTATE_NONE;
		}
	}
}

//Return TRUE if swooping. Return FALSE if not swooping.
bool idPhysics_Player::GetSwoopState()
{
	if (swoopState == SWOOPSTATE_NONE)
		return false;

	return true;
}

const float SWOOP_STARTTIME = 150;
const float SWOOP_MOVETIME = 900;
const float SWOOP_FORWARDOFFSET = 12;
const float SWOOP_SIDEOFFSET = 6;

void idPhysics_Player::CableswoopTo(idEntity * startPoint, idEntity * endPoint)
{
	idVec3 rightDir;

	swoopStartEnt = startPoint;
	swoopEndEnt = endPoint;

	swoopState = SWOOPSTATE_MOVETOSTART;
	swoopTimer = gameLocal.time;

	swoopStartPoint = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin();

	swoopEndEnt->GetPhysics()->GetAxis().ToAngles().ToVectors(NULL, &rightDir, NULL);

	swoopDestinationPoint = swoopStartEnt->GetPhysics()->GetOrigin() + (startPoint->GetPhysics()->GetAxis().ToAngles().ToForward() * SWOOP_FORWARDOFFSET) + (rightDir * SWOOP_SIDEOFFSET);
	swoopDestinationPoint.z = swoopStartPoint.z;

	fallenState = FALLEN_NONE;
	gameLocal.GetLocalPlayer()->SetFallState(false, false);

	swoopParticletype = SWOOP_VERTICALZIPCORD;
}

void idPhysics_Player::SpaceCableswoopTo(idEntity * startPoint, idEntity * endPoint)
{
	swoopStartEnt = startPoint;
	swoopEndEnt = endPoint;

	swoopState = SWOOPSTATE_MOVETOSTART;
	swoopTimer = gameLocal.time;

	//Where the lerp starts. The player position.
	swoopStartPoint = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin();

	//Find where the player attaches to the cable. Find the closest point on the zipline.
	swoopDestinationPoint = gameLocal.GetLocalPlayer()->GetClosestPointOnLine(gameLocal.GetLocalPlayer()->firstPersonViewOrigin, startPoint->GetPhysics()->GetOrigin(), endPoint->GetPhysics()->GetOrigin());

	fallenState = FALLEN_NONE;
	gameLocal.GetLocalPlayer()->SetFallState(false, false);

	swoopParticletype = SWOOP_SPACECABLE;
}




//TODO: consolidate setacrostate and sethidestate.
void idPhysics_Player::SetAcroState(idEntity * hideEnt)
{
	idAngles playerAng;
	idVec3 clamberposOffset;

	playerAng = gameLocal.GetLocalPlayer()->viewAngles;
	playerAng.pitch = 0;
	playerAng.roll = 0;

	acroType = ((idAcroPoint *)hideEnt)->state;
	acroAngle = hideEnt->GetPhysics()->GetAxis().ToAngles().yaw;

	cargohideEnt = hideEnt; //store it, so that we can mark it as frobbable after we exit acro state.

	if (acroType == ACROTYPE_CEILINGHIDE)
	{
		idAngles acroForward;
		idVec3 acroDir;

		clamberposOffset = vec3_zero;
		((idPlayer *)self)->SetViewPitchLerp(ACRO_CEILINGHIDE_VIEWPITCH);

		//position eyes at head position.
		acroForward.yaw = acroAngle;
		acroForward.pitch = 0;
		acroForward.roll = 0;
		acroForward.ToVectors(&acroDir, NULL, NULL);

		((idPlayer *)self)->SetViewPosActive(true, idVec3(0, 0, -17) + (acroDir * 18));
		//((idPlayer *)self)->SetViewPosActive(true, idVec3(0, 0, 32) );
	}
	else if (acroType == ACROTYPE_SPLITS)
	{
		idAngles acroForward;
		idVec3 acroDir;

		clamberposOffset = idVec3(0, 0, -25);

		acroForward.yaw = acroAngle;
		acroForward.pitch = 0;
		acroForward.roll = 0;
		acroForward.ToVectors(&acroDir, NULL, NULL);

		((idPlayer *)self)->SetViewPosActive(true, (acroDir * 8));

		((idPlayer *)self)->SetViewPitchLerp(ACRO_SPLITS_VIEWPITCH);
	}

	// SW 23rd April 2025: Skip lip check for acropoint clambers (messes with the lerps, causes player to clip into walls/ceilings)
	StartClamber(hideEnt->GetPhysics()->GetOrigin() + clamberposOffset, vec3_zero, CLAMBER_MOVETIMESCALAR, CLAMBER_SETTLETIMESCALAR, 1.0f, 1.0f, true, true);

	((idPlayer *)self)->SetViewYawLerp(acroAngle);

	
}

void idPhysics_Player::SetHideState(idEntity * hideEnt, int _hideType)
{
	idVec3 forward, up, finalPos, dustPos;
	int verticalOffset = 0;
	int forwardOffset = 8;
	float targetPitch = hideEnt->spawnArgs.GetFloat("targetpitch", "0"); // SW 5th May 2025: Making this value data-driven

	cargohideEnt = hideEnt;	
	hideEnt->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);

	//bc Going to hardcode some values here.............. ugh!
	if (_hideType == CARGOHIDETYPE_ROW)
	{
		verticalOffset = -20;
		dustPos = hideEnt->GetPhysics()->GetOrigin() + (forward * 24);
	}
	else if (_hideType == CARGOHIDETYPE_STACK)
	{
		verticalOffset = -16;
		dustPos = hideEnt->GetPhysics()->GetOrigin() + (forward * 24);
	}
	else if (_hideType == CARGOHIDETYPE_LAUNDRYMACHINE)
	{
		verticalOffset = -16;
		forwardOffset = -22;
		dustPos = hideEnt->GetPhysics()->GetOrigin() + (up * 16) + (forward*4) ;
	}
	hideType = _hideType;


	((idPlayer *)self)->SetViewPitchLerp(targetPitch, 200);
	((idPlayer *)self)->SetViewYawLerp(hideEnt->GetPhysics()->GetAxis().ToAngles().yaw, 200);

	//Viewangle locking is done via player.cpp, in UpdateViewAngles()
	acroAngle = hideEnt->GetPhysics()->GetAxis().ToAngles().yaw;

	
	acroType = ACROTYPE_CARGOHIDE;
	finalPos = hideEnt->GetPhysics()->GetOrigin() + (forward * forwardOffset) + (up * verticalOffset);

	// SW 23rd April 2025: Skip lip check for acropoint clambers (messes with the lerps, causes player to clip into walls/ceilings)
	StartClamber(finalPos, vec3_zero, CLAMBER_MOVETIMESCALAR, CLAMBER_SETTLETIMESCALAR, 1.0f, 1.0f, true, true);
	
	idEntityFx::StartFx("fx/dustfall01", &dustPos, &mat3_identity, NULL, false);
}

//Swoop is the vertical elevator/zipline
void idPhysics_Player::UpdateSwooping()
{

	if (swoopState == SWOOPSTATE_MOVETOSTART) //Attach to cable start point.
	{
		idVec3 fxPos;
		idVec3 newPosition;
		float lerp = (gameLocal.time - swoopTimer) / (float)SWOOP_STARTTIME;

		if (lerp > 1)
			lerp = 1;

		newPosition.Lerp(swoopStartPoint, swoopDestinationPoint, lerp);
		SetOrigin(newPosition);

		if (lerp >= 1)
		{
			idVec3 endDir = swoopEndEnt->GetPhysics()->GetAxis().ToAngles().ToForward();
			idVec3 rightDir;
			

			swoopEndEnt->GetPhysics()->GetAxis().ToAngles().ToVectors(NULL, &rightDir, NULL);

			swoopState = SWOOPSTATE_MOVETOEND;

			swoopStartPoint = this->GetOrigin();
			swoopDestinationPoint = swoopEndEnt->GetPhysics()->GetOrigin() + (endDir * -SWOOP_FORWARDOFFSET) + (rightDir * SWOOP_SIDEOFFSET);

			swoopTimer = gameLocal.time;

			((idPlayer *)self)->SetViewYawLerp(swoopEndEnt->GetPhysics()->GetAxis().ToAngles().yaw, 600);
			((idPlayer *)self)->SetViewPitchLerp(0, 800);


			if (swoopParticletype == SWOOP_VERTICALZIPCORD)
			{
				if (swoopStartPoint.z > swoopDestinationPoint.z)
				{
					self->StartSound("snd_cabledescend", SND_CHANNEL_ANY, 0, false, NULL);

					fxPos.Lerp(swoopStartPoint, swoopDestinationPoint, 0.3f);
					idEntityFx::StartFx("fx/wind_local", &fxPos, &mat3_default, NULL, false);
				}
				else
				{
					idAngles fxAng = idAngles(180, 0, 0);
					self->StartSound("snd_cableascend", SND_CHANNEL_ANY, 0, false, NULL);

					fxPos.Lerp(swoopStartPoint, swoopDestinationPoint, 0.7f);
					idEntityFx::StartFx("fx/wind_local", fxPos, fxAng.ToMat3());
				}
			}
			else if (swoopParticletype == SWOOP_SPACECABLE)
			{
				self->StartSound("snd_cableascend", SND_CHANNEL_ANY, 0, false, NULL);

				idVec3 dirToEnd = swoopDestinationPoint - swoopStartPoint;
				dirToEnd.Normalize();
				fxPos = gameLocal.GetLocalPlayer()->firstPersonViewOrigin + (dirToEnd * 256);

				idAngles fxAng = dirToEnd.ToAngles();
				fxAng.pitch += 90;

				gameLocal.DoParticle("wind_local_big.prt", fxPos, fxAng.ToForward());
			}

			
			
			//gameRenderWorld->DebugSphere(colorGreen, idSphere(swoopDestinationPoint, 8), 3000);			
		}
	}
	else if (swoopState == SWOOPSTATE_MOVETOEND)
	{
		idVec3 newPosition;
		float lerp = (gameLocal.time - swoopTimer) / (float)SWOOP_MOVETIME;

		if (lerp > 1)
			lerp = 1;

		newPosition.Lerp(swoopStartPoint, swoopDestinationPoint, lerp);
		SetOrigin(newPosition);

		if (lerp >= 1)
		{
			//Allow the jump angle to be decoupled from the view angle.
			idVec3 jumpDir;
			float flingAngle;

			flingAngle = swoopEndEnt->spawnArgs.GetInt("flingangle", "0");

			if (flingAngle == 0)
			{
				//No custom fling angle. So just default to the "angle" key.
				jumpDir = swoopEndEnt->GetPhysics()->GetAxis().ToAngles().ToForward();
			}
			else
			{
				//Has a custom fling angle.
				jumpDir = idAngles(0, flingAngle, 0).ToForward();
			}

			swoopState = SWOOPSTATE_NONE;
			this->SetLinearVelocity(jumpDir * 300);

			self->StartSound("snd_jump", SND_CHANNEL_BODY3, 0, false, NULL);
		}
	}
}

void idPhysics_Player::UpdateMovelerp()
{
	float lerp = (gameLocal.time - movelerpTimer) / (float)movelerp_Duration;
	idVec3 newPosition;

	if (lerp > 1)
		lerp = 1;

	newPosition.Lerp(movelerpStartPoint, movelerpDestinationPoint, lerp);
	SetOrigin(newPosition);

	if (gameLocal.time >= movelerpTimer + movelerp_Duration)
	{
		movelerping = false;

		current.pushVelocity.Zero();
		current.velocity.Zero();
	}
}

void idPhysics_Player::StartMovelerp(idVec3 destination, int moveTime)
{
	movelerpDestinationPoint = destination;
	movelerpStartPoint = this->GetOrigin();
	movelerpTimer = gameLocal.time ;
	movelerping = true;
	movelerp_Duration = moveTime;
}

void idPhysics_Player::StartGrabRing(idVec3 grabPosition, idEntity *grabring)
{
	//Player has just frobbed a grab ring. Start the grab ring sequence.

	grabringStartPos = this->GetOrigin();
	grabringDestinationPos = grabPosition;
	grabringTimer = gameLocal.time + GRABRING_INITIALGRABTIME;
	grabringState = GR_GRABSTART;
	grabringEnt = grabring;

	if (grabring)
	{
		grabring->isFrobbable = false;
	}
}

bool idPhysics_Player::GetGrabringState()
{
	return (grabringState == GR_GRABIDLE);
}

void idPhysics_Player::UpdateGrabRing()
{
	//Make sure player doesn't move.
	current.pushVelocity.Zero();
	current.velocity.Zero();

	if (grabringState == GR_GRABSTART)
	{
		//Lerp player to the grab ring.
		float lerp = (grabringTimer - gameLocal.time) / (float)GRABRING_INITIALGRABTIME;
		lerp = idMath::ClampFloat(0, 1, 1 - lerp);
		lerp = idMath::CubicEaseOut(lerp);		

		if (gameLocal.time >= grabringTimer)
		{
			grabringState = GR_GRABIDLE;
		}
		
		idVec3 lerpedPosition;
		lerpedPosition.Lerp(grabringStartPos, grabringDestinationPos, lerp);
		current.origin = lerpedPosition;
	}
	else if (grabringState == GR_GRABIDLE)
	{
		if ((command.buttons & BUTTON_FROB) && !(gameLocal.GetLocalPlayer()->oldButtons & BUTTON_FROB))
		{
			//Player is holding onto a grab bar and pressed FROB

			//Propel forward.
			grabringState = GR_NONE;

			if (grabringEnt.IsValid())
			{
				grabringEnt.GetEntity()->isFrobbable = true;
				grabringEnt.GetEntity()->StartSound("snd_propel", SND_CHANNEL_ANY);

				idVec3 propelDirection = ((idPlayer *)self)->viewAngles.ToForward();
				this->SetLinearVelocity(GRABRING_PROPEL_FORCE * propelDirection); //Propel.

				((idPlayer *)self)->playerView.DoDurabilityFlash();
			}
			return;
		}

		if (command.upmove > 10)
		{
			//Player is holding onto a grab bar and pressed JUMP

			//Let go of the grab ring.
			grabringState = GR_NONE;
			canFlymoveUp = false;

			if (grabringEnt.IsValid())
			{
				grabringEnt.GetEntity()->isFrobbable = true;

				//Gently push player away from the grab ring.
				idVec3 directionAwayFromGrabring = ((idPlayer *)self)->GetEyePosition() - grabringEnt.GetEntity()->GetPhysics()->GetOrigin();
				directionAwayFromGrabring.Normalize();
				this->SetLinearVelocity(GRABRING_UNGRAB_FORCE * directionAwayFromGrabring);

				grabringEnt.GetEntity()->StartSound("snd_ungrab", SND_CHANNEL_ANY);
			}
			return;
		}

	}
}

void idPhysics_Player::SetVacuumSplineMover(idMover* mover)
{
	vacuumSplineMover = mover;
}

idMover* idPhysics_Player::GetVacuumSplineMover(void)
{
	return vacuumSplineMover.IsValid() ? vacuumSplineMover.GetEntity() : NULL;
}

// SW 17th Feb 2025
// The vacuum spline mover has the potential to move the player outside the world, 
// because it is following a dynamically generated curve that cannot be guaranteed to stay inside the world at all times.
// We must follow it as closely as we can, but ensure that we remain inside the world
void idPhysics_Player::UpdateVacuumSplineMoving(void)
{
	idVec3 moverOrigin = vacuumSplineMover.GetEntity()->GetPhysics()->GetOrigin();
	idVec3 difference;

	trace_t results;
	gameLocal.clip.TraceBounds(results, this->current.origin, moverOrigin, clipModel->GetBounds(), CONTENTS_SOLID, this->self);

	if (results.fraction < 1)
	{
		// Player would exit the world if they followed the mover directly.
		// Instead, try to follow along the wall
		difference = (moverOrigin - current.origin);

		// Project the difference onto the collision plane and slide along it
		difference = difference - (DotProduct(difference, results.c.normal) / DotProduct(results.c.normal, results.c.normal)) * results.c.normal;
		current.origin = current.origin + difference;
	}
	else
	{
		// It's safe to be directly where the mover is
		current.origin = moverOrigin;
	}
}
