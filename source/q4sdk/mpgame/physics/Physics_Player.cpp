#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

CLASS_DECLARATION( idPhysics_Actor, idPhysics_Player )
END_CLASS

// movement parameters
const float PM_STOPSPEED		= 100.0f;
const float PM_SWIMSCALE		= 0.5f;
const float PM_LADDERSPEED		= 100.0f;
const float PM_STEPSCALE		= 1.0f;

const float PM_ACCELERATE_SP	= 10.0f;
const float PM_AIRACCELERATE_SP	= 1.0f;
const float PM_ACCELERATE_MP	= 15.0f;
const float PM_AIRACCELERATE_MP	= 1.18f;
const float PM_WATERACCELERATE	= 4.0f;
const float PM_FLYACCELERATE	= 8.0f;

const float PM_FRICTION			= 6.0f;
const float PM_AIRFRICTION		= 0.0f;
const float PM_WATERFRICTION	= 2.0f;
const float PM_FLYFRICTION		= 3.0f;
const float PM_NOCLIPFRICTION	= 12.0f;
const float PM_SLIDEFRICTION    = 0.5f;

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

float idPhysics_Player::Pm_Accelerate( void ) {
	return gameLocal.IsMultiplayer() ? PM_ACCELERATE_MP : PM_ACCELERATE_SP;
}

float idPhysics_Player::Pm_AirAccelerate( void ) {
	return gameLocal.IsMultiplayer() ? PM_AIRACCELERATE_MP : PM_AIRACCELERATE_SP;
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
	float addspeed, accelspeed, currentspeed;

	currentspeed = current.velocity * wishdir;
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0) {
		return;
	}
// RAVEN BEGIN
// nmckenzie: added ability to try alternate accelerations.
	if ( pm_acceloverride.GetFloat() > 0.0f ) {
		accelspeed = pm_acceloverride.GetFloat() * frametime * wishspeed;
	} else {
		accelspeed = accel * frametime * wishspeed;
	}
// RAVEN END
	if ( accelspeed > addspeed ) {
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
	if ( canPush > pushLen ) {
		canPush = pushLen;
	}

	current.velocity += canPush * pushDir;
#endif
}

/*
===============
idPhysics_Player::AdjustVertically
given a walkable plane normal, adjust z to be in the plane without changing the horizontal direction
===============
*/
idVec3 idPhysics_Player::AdjustVertically( const idVec3 &normal, const idVec3 &_in, int forceLength ) const {
	idVec3 out;
	idVec3 in = _in;

	idVec3 velocityPlane;
	velocityPlane[0] = in[1];
	velocityPlane[1] = -in[0];
	velocityPlane[2] = 0.0f;

	// same length as horizontal input, and in the plane
	out = velocityPlane.Cross( normal );

	// make sure direction is ok
	if ( in * out < 0 ) {
		out = -out;
	}

	switch ( forceLength ) {
		case 0: {
			if ( pm_slidevelocity.GetInteger() == 1 ) {
				double inSpeed = in.Normalize();
				double hSpeed = out.Normalize();
				// incidence angle, between 0 (just touching, result speed at inSpeed) and M_PI/2 (perpendicular hit, result speed at hSpeed)
				float angle = idMath::ACos( in * out );
				double ratio = idMath::Pow64( angle * 2.0f / idMath::PI, pm_powerslide.GetFloat() );
				float targetSpeed = ratio * hSpeed + ( 1.0 - ratio ) * inSpeed;
				out *= targetSpeed;
			}
		break;
		}
		case 1: {
			double inSpeed = in.Normalize();
			out.Normalize();
			out *= inSpeed;
			break;
		}
		case 2: {
			// scale the out vector so horizontal speed is maintained
			idVec3 hout = out; hout.z = 0.0f;
			float ratio = velocityPlane.Length() / hout.Length();
			out *= ratio;
			break;
		}
	}

	// and maintain an OVERCLIP
	if ( out.z >= 0 ) {
		out.z *= OVERCLIP;
	} else {
		out.z /= OVERCLIP;
	}

	return out;
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
	idVec3		dir;
	idVec3		planes[MAX_CLIP_PLANES];
	idVec3		end, stepEnd, primal_velocity, endVelocity, endClipVelocity, clipVelocity;
	trace_t		trace, stepTrace, downTrace;
	bool		nearGround, stepped, pushed;
	bool		sliding = ( current.crouchSlideTime > 0 ) && ( command.upmove < 0 ) && groundPlane;
	int			forceLength = sliding ? 2 : 0;

	numbumps = 4;

	primal_velocity = current.velocity;

	if ( gravity ) {
		endVelocity = current.velocity + gravityVector * frametime;
		current.velocity = ( current.velocity + endVelocity ) * 0.5f;
		primal_velocity = endVelocity;
		if ( groundPlane ) {
			// slide along the ground plane
			if ( groundTrace.c.normal.z >= MIN_WALK_NORMAL ) {
				current.velocity = AdjustVertically( groundTrace.c.normal, current.velocity, forceLength );
			} else {
				current.velocity.ProjectOntoPlane( groundTrace.c.normal, OVERCLIP );
			}
		}
	} else {
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
		gameLocal.Translation( self, trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self );
		current.origin = trace.endpos;

		// if moved the entire distance
		if ( trace.fraction >= 1.0f ) {
			break;
		}

		time_left -= time_left * trace.fraction;
		stepped = pushed = false;

		// if we are allowed to step up
		if ( stepUp && ( trace.c.normal * -gravityNormal ) < MIN_WALK_NORMAL ) {
			nearGround = groundPlane | ladder;

			if ( !nearGround ) {
				// trace down to see if the player is near the ground
				// step checking when near the ground allows the player to move up stairs smoothly while jumping
				stepEnd = current.origin + maxStepHeight * gravityNormal;
				gameLocal.Translation( self, downTrace, current.origin, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );
				nearGround = ( downTrace.fraction < 1.0f && ( downTrace.c.normal * -gravityNormal ) > MIN_WALK_NORMAL );
			}

			// may only step up if near the ground or on a ladder
			if ( nearGround ) {

				// step up
				stepEnd = current.origin - maxStepHeight * gravityNormal;
				gameLocal.Translation( self, downTrace, current.origin, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );

				// trace along velocity
				stepEnd = downTrace.endpos + time_left * current.velocity;
				gameLocal.Translation( self, stepTrace, downTrace.endpos, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );

				// step down
				stepEnd = stepTrace.endpos + maxStepHeight * gravityNormal;
				gameLocal.Translation( self, downTrace, stepTrace.endpos, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );

  				if ( downTrace.fraction >= 1.0f || ( downTrace.c.normal * -gravityNormal ) > MIN_WALK_NORMAL ) {
					// if moved the entire distance
   					if ( stepTrace.fraction >= 1.0f ) {
						time_left = 0;
  						current.stepUp -= ( downTrace.endpos - current.origin ) * gravityNormal;
						current.origin = downTrace.endpos;
						current.movementFlags |= PMF_STEPPED_UP;
						break;
					}

					// if the move is further when stepping up
					if ( stepTrace.fraction > trace.fraction ) {
 						time_left -= time_left * stepTrace.fraction;
						current.stepUp -= ( downTrace.endpos - current.origin ) * gravityNormal;
						current.origin = downTrace.endpos;
						current.movementFlags |= PMF_STEPPED_UP;
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

		if ( !stepped && self ) {
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
		// if this is the same plane we hit before, nudge velocity out along it,
		// which fixes some epsilon issues with non-axial planes
		//
		for ( i = 0; i < numplanes; i++ ) {
			if ( ( trace.c.normal * planes[i] ) > 0.999f ) {

				if ( planes[i].z >= MIN_WALK_NORMAL ) {
					// cargo cult?
					current.velocity = AdjustVertically( trace.c.normal, current.velocity, forceLength );
				} else {
					// clip into the trace normal just in case this normal is almost but not exactly the same as the groundTrace normal
					current.velocity.ProjectOntoPlane( trace.c.normal, OVERCLIP );
					// also add the normal to nudge the velocity out
					current.velocity += trace.c.normal;
				}

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

			if ( planes[i].z >= MIN_WALK_NORMAL ) {
				clipVelocity = AdjustVertically( planes[i], current.velocity, forceLength );
				endClipVelocity = AdjustVertically( planes[i], endVelocity, forceLength );
			} else {
				// slide along the plane
				clipVelocity = current.velocity;
				clipVelocity.ProjectOntoPlane( planes[i], OVERCLIP );

				// slide along the plane
				endClipVelocity = endVelocity;
				endClipVelocity.ProjectOntoPlane( planes[i], OVERCLIP );
			}

			// see if there is a second plane that the new move enters
			for ( j = 0; j < numplanes; j++ ) {
				if ( j == i ) {
					continue;
				}
				if ( ( clipVelocity * planes[j] ) >= 0.1f ) {
					continue;		// move doesn't interact with the plane
				}

				if ( planes[j].z >= MIN_WALK_NORMAL ) {
					clipVelocity = AdjustVertically( planes[j], clipVelocity, forceLength );
					endClipVelocity = AdjustVertically( planes[j], endClipVelocity, forceLength );
				} else {
					// try clipping the move to the plane
					clipVelocity.ProjectOntoPlane( planes[j], OVERCLIP );
					endClipVelocity.ProjectOntoPlane( planes[j], OVERCLIP );
				}

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

					// stop dead at a triple plane interaction (duh)
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
		gameLocal.Translation( self, downTrace, current.origin, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );
		if ( downTrace.fraction > 1e-4f && downTrace.fraction < 1.0f ) {
			current.stepUp -= ( downTrace.endpos - current.origin ) * gravityNormal;
			current.origin = downTrace.endpos;
			current.movementFlags |= PMF_STEPPED_DOWN;
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

	return ( bumpcount == 0 );
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
		current.velocity.Zero();
		return;
	}

	drop = 0;

	// spectator friction
	if ( pm_frictionoverride.GetFloat() > -1 ) {
		drop += speed * pm_frictionoverride.GetFloat() * frametime;
	} else if ( current.movementType == PM_SPECTATOR ) {
		drop += speed * PM_FLYFRICTION * frametime;
	}
	// apply ground friction
	else if ( walking && waterLevel <= WATERLEVEL_FEET ) {
		// no friction on slick surfaces
		if ( !(groundMaterial && groundMaterial->GetSurfaceFlags() & SURF_SLICK) ) {
			// if getting knocked back, no friction
			if ( !(current.movementFlags & PMF_TIME_KNOCKBACK) ) {
				control = speed < PM_STOPSPEED ? PM_STOPSPEED : speed;
				if ( current.crouchSlideTime > 0 ) {
					drop += control * PM_SLIDEFRICTION * frametime;
				} else {
					drop += control * PM_FRICTION * frametime;
				}
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
	if ( newspeed < 0 ) {
		newspeed = 0;
	}
	current.velocity *= ( newspeed / speed );

	// TTimo - snap to avoid denormals
	if ( fabs( current.velocity.x ) < 1.0e-5f ) {
		current.velocity.x = 0.0f;
	}
	if ( fabs( current.velocity.y ) < 1.0e-5f ) {
		current.velocity.y = 0.0f;
	}
	if ( fabs( current.velocity.z ) < 1.0e-5f ) {
		current.velocity.z = 0.0f;
	}
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

	// if the player isnt pressing crouch and heading down then accumulate slide time
	if ( command.upmove >= 0 && current.velocity * gravityNormal > 0 ) {	
		current.crouchSlideTime = idMath::ClampInt( 0, 2000, current.crouchSlideTime + framemsec * 2 ); 
	}

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
	idPhysics_Player::Accelerate( wishdir, wishspeed, Pm_AirAccelerate() );

	// we may have a ground plane that is very steep, even
	// though we don't have a groundentity
	// slide along the steep plane
	if ( groundPlane ) {
		current.velocity.ProjectOntoPlane( groundTrace.c.normal, OVERCLIP );
	}

	// NOTE: enable stair checking while moving through the air in multiplayer to allow bunny hopping onto stairs
	idPhysics_Player::SlideMove( true, gameLocal.isMultiplayer, false, false );
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
	idVec3		vel;

	if ( waterLevel > WATERLEVEL_WAIST && ( viewForward * groundTrace.c.normal ) > 0.0f ) {
		// begin swimming
		idPhysics_Player::WaterMove();
		return;
	}

	if ( idPhysics_Player::CheckJump() ) {
		// jumped away
		if ( waterLevel > WATERLEVEL_FEET ) {
			idPhysics_Player::WaterMove();
		} else {
			idPhysics_Player::AirMove();
		}
		return;
	}

	idPhysics_Player::Friction();

	scale = idPhysics_Player::CmdScale( command );

	// project moves down to flat plane
	viewForward -= (viewForward * gravityNormal) * gravityNormal;
	viewRight -= (viewRight * gravityNormal) * gravityNormal;

	assert( groundTrace.c.normal.z >= MIN_WALK_NORMAL );
	viewForward = AdjustVertically( groundTrace.c.normal, viewForward );
	viewRight = AdjustVertically( groundTrace.c.normal, viewRight );

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

	// lower acceleration (control) when on slippery stuff or being smacked around
	bool fLowControl = ( ( current.movementFlags & PMF_TIME_KNOCKBACK ) || ( groundMaterial && groundMaterial->GetSurfaceFlags() & SURF_SLICK ) );
	accelerate = fLowControl ? Pm_AirAccelerate() : Pm_Accelerate();
	idPhysics_Player::Accelerate( wishdir, wishspeed, accelerate );
	if ( fLowControl ) {
		current.velocity += gravityVector * frametime;
	}

	// slide along the ground plane
	current.velocity = AdjustVertically( groundTrace.c.normal, current.velocity, 1 );

	// don't do anything if standing still
	vel = current.velocity - (current.velocity * gravityNormal) * gravityNormal;
	if ( vel.IsZero() ) {
		return;
	}

	gameLocal.push.InitSavingPushedEntityPositions();

	idPhysics_Player::SlideMove( false, true, true, !gameLocal.isMultiplayer );
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

// RAVEN BEGIN
// nmckenzie: allow trying custom frictions
	if ( pm_frictionoverride.GetFloat ( ) > -1 ) {
		idPhysics_Player::Friction();
	} else {
// RAVEN END

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

// RAVEN BEGIN
// nmckenzie: allow trying custom frictions
	}
// RAVEN END

	// accelerate
	scale = idPhysics_Player::CmdScale( command );

	wishdir = scale * (viewForward * command.forwardmove + viewRight * command.rightmove);
	wishdir -= scale * gravityNormal * command.upmove;
	wishspeed = wishdir.Normalize();
	wishspeed *= scale;

	idPhysics_Player::Accelerate( wishdir, wishspeed, Pm_Accelerate() );

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
	if ( command.upmove ) {
		wishvel += -0.5f * gravityNormal * scale * (float) command.upmove;
	}

	// do strafe friction
	idPhysics_Player::Friction();

	// accelerate
	wishspeed = wishvel.Normalize();
	idPhysics_Player::Accelerate( wishvel, wishspeed, Pm_Accelerate() );

	// cap the vertical velocity
	upscale = current.velocity * -gravityNormal;
	if ( upscale < -PM_LADDERSPEED ) {
		current.velocity += gravityNormal * (upscale + PM_LADDERSPEED);
	}
	else if ( upscale > PM_LADDERSPEED ) {
		current.velocity += gravityNormal * (upscale - PM_LADDERSPEED);
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
void idPhysics_Player::CheckGround( bool checkStuck ) {
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

	if ( checkStuck && contacts.Num() ) {
		contents = gameLocal.Contents( self, current.origin, clipModel, clipModel->GetAxis(), -1, self );
		if ( contents & MASK_SOLID ) {
			// do something corrective if stuck in solid
			idPhysics_Player::CorrectAllSolid( groundTrace, contents );
		}
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
	if ( ( current.velocity * -gravityNormal ) > 0.0f && ( current.velocity * groundTrace.c.normal ) > 1.0f ) {
		groundPlane = false;
		walking = false;
		return;
	}

	// slopes that are too steep will not be considered onground
	if ( ( groundTrace.c.normal * -gravityNormal ) < MIN_WALK_NORMAL ) {
		// in multiplayer, instead of sliding push the player out from the normal for some free fall
		current.origin += groundTrace.c.normal;
			
		groundPlane = false;
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
	if ( self ) {
		self->Collide( groundTrace, current.velocity );
	}

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
		} else {
			// stand up if possible
			if ( current.movementFlags & PMF_DUCKED ) {
				// try to stand up
				end = current.origin - ( pm_normalheight.GetFloat() - pm_crouchheight.GetFloat() ) * gravityNormal;
				gameLocal.Translation( self, trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self );
				if ( trace.fraction >= 1.0f ) {
					current.movementFlags &= ~PMF_DUCKED;
				}
			}
		}

		if ( current.movementFlags & PMF_DUCKED ) {
			if ( !current.crouchSlideTime ) {
				playerSpeed = crouchSpeed;
			}
			maxZ = pm_crouchheight.GetFloat();			
		} else {
			maxZ = pm_normalheight.GetFloat();
			if ( groundPlane && current.crouchSlideTime ) {
				current.crouchSlideTime = 0;
			}
		}
	}
	// if the clipModel height should change
	if ( clipModel->GetBounds()[1][2] != maxZ ) {
		
		bounds = clipModel->GetBounds();
		bounds[1][2] = maxZ;

		// < 0: don't use alternate alignment, > 0: use alternate alignment (faces aligned with axes for even-sided cylinders)
		// 0: use AABB; 1: use 8-sided cylinder; 3+: use custom number of sides for cylinder
		int sides = pm_usecylinder.GetInteger();
		bool alt_align = (sides > 0);
		sides = idMath::Abs( sides );

		if ( sides >= 1 ) {
			if ( sides < 3 ) {
				sides = 8;
			}
			idTraceModel trm( bounds, sides, alt_align );
//			trm.findWalkSurfaces = true;
			clipModel->LoadModel( trm, NULL );
		} else {
			idTraceModel trm( bounds );
//			trm.findWalkSurfaces = true;
			clipModel->LoadModel( trm, NULL );
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

	if ( walking ) {
		// don't want to get sucked towards the ladder when still walking
		tracedist = 1.0f;
	} else {
		tracedist = 48.0f;
	}

	end = current.origin + tracedist * forward;
	gameLocal.Translation( self, trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self );
	// if near a surface
	if ( trace.fraction < 1.0f ) {

		// if a ladder surface
		if ( trace.c.material && ( trace.c.material->GetSurfaceFlags() & SURF_LADDER ) ) {

			// check a step height higher
			end = current.origin - gravityNormal * ( maxStepHeight * 0.75f );
			gameLocal.Translation( self, trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self );
			start = trace.endpos;
			end = start + tracedist * forward;
			gameLocal.Translation( self, trace, start, end, clipModel, clipModel->GetAxis(), clipMask, self );

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

	// CheckJump only called from WalkMove, therefore with walking == true
	// in MP game we always have groundPlane == walking
	// (this mostly matters to velocity clipping against ground when the jump is ok'ed)
	assert( groundPlane );

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

	// start by setting up the normal ground slide velocity
	// this will make sure that when we add the jump velocity we actually get off of the ground plane
	if ( current.velocity * groundTrace.c.normal < 0.0f ) {
		current.velocity = AdjustVertically( groundTrace.c.normal, current.velocity );
	}
	
	addVelocity = 2.0f * maxJumpHeight * -gravityVector;
	addVelocity *= idMath::Sqrt( addVelocity.Normalize() );
	current.velocity += addVelocity;

	groundPlane = false;		// jumping away
	walking = false;
	groundEntityPtr = NULL;
	current.movementFlags |= PMF_JUMP_HELD | PMF_JUMPED;

	// crouch slide
	current.crouchSlideTime = 0;

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
	cont = gameLocal.Contents( self, spot, NULL, mat3_identity, -1, self );
	if ( !(cont & CONTENTS_SOLID) ) {
		return false;
	}

	spot -= 16.0f * gravityNormal;
	cont = gameLocal.Contents( self, spot, NULL, mat3_identity, -1, self );
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

// AReis: Get back the water entity (if there is one), so we can grab his density
// then apply some force to the fluid since we're moving through it.
	idEntity *other = NULL;

	// check at feet level
	point = current.origin - ( bounds[0][2] + 1.0f ) * gravityNormal;
	contents = gameLocal.Contents( self, point, NULL, mat3_identity, -1, self, &other );	
	if ( contents & MASK_WATER ) {

		waterType = contents;
		waterLevel = WATERLEVEL_FEET;

		// check at waist level
		point = current.origin - ( bounds[1][2] - bounds[0][2] ) * 0.5f * gravityNormal;
		contents = gameLocal.Contents( self, point, NULL, mat3_identity, -1, self );
		if ( contents & MASK_WATER ) {

			waterLevel = WATERLEVEL_WAIST;

			// check at head level
			point = current.origin - ( bounds[1][2] - 1.0f ) * gravityNormal;
			contents = gameLocal.Contents( self, point, NULL, mat3_identity, -1, self );
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
	
	if ( groundPlane && current.crouchSlideTime ) {
		if ( framemsec >= current.crouchSlideTime ) {
			current.crouchSlideTime = 0;
		} else {
			current.crouchSlideTime -= framemsec;
		}
	}
}

/*
================
idPhysics_Player::MovePlayer
================
*/
void idPhysics_Player::MovePlayer( int msec ) {
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
// RAVEN BEGIN
// nmckenzie: Allowing ways to force spectator movement.
	if ( current.movementType == PM_SPECTATOR || pm_forcespectatormove.GetBool() ) {
// RAVEN END
		SpectatorMove();
		idPhysics_Player::DropTimers();
// RAVEN BEGIN
// abahr: need to clear pushVelocity.  Was causing problems when noclipping while on a mover
		ClearPushedVelocity();
// RAVEN END
		return;
	}

	// special no clip mode
	if ( current.movementType == PM_NOCLIP ) {
		idPhysics_Player::NoclipMove();
		idPhysics_Player::DropTimers();
// RAVEN BEGIN
// abahr: need to clear pushVelocity.  Was causing problems when noclipping while on a mover
		ClearPushedVelocity();
// RAVEN END
		return;
	}

	// no control when dead
	if ( current.movementType == PM_DEAD ) {
		command.forwardmove = 0;
		command.rightmove = 0;
		command.upmove = 0;
	}

	// set watertype and waterlevel
// RAVEN BEGIN
// ddynerman: water disabled in MP
	if ( !gameLocal.isMultiplayer ) {
		idPhysics_Player::SetWaterLevel();
	}
// RAVEN END

	// check for ground
	idPhysics_Player::CheckGround( true );

	// check if up against a ladder
// RAVEN BEGIN
// MrE: no ladders in MP
	if ( !gameLocal.isMultiplayer ) {
		idPhysics_Player::CheckLadder();
	}
// RAVEN END

	// set clip model size
	idPhysics_Player::CheckDuck();

	// handle timers
	idPhysics_Player::DropTimers();

	// move
	if ( current.movementType == PM_DEAD ) {
		// dead
		idPhysics_Player::DeadMove();
	}
	else if ( ladder ) {
		// going up or down a ladder
		idPhysics_Player::LadderMove();
	}
// RAVEN BEGIN
// ddynerman: water disabled in MP
	else if ( !gameLocal.isMultiplayer && current.movementFlags & PMF_TIME_WATERJUMP ) {

		// jumping out of water
		idPhysics_Player::WaterJumpMove();
	}
	else if ( !gameLocal.isMultiplayer && waterLevel > 1 ) {
// RAVEN END
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
	}

	if ( !gameLocal.isMultiplayer ) {
		idPhysics_Player::SetWaterLevel();
	}

	idPhysics_Player::CheckGround( false );

	// move the player velocity back into the world frame
	current.velocity += current.pushVelocity;
	current.lastPushVelocity = current.pushVelocity;
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
	//MCG: if bound, never think we're crouched
	return ( !masterEntity&&( current.movementFlags & PMF_DUCKED ) != 0 );
}

/*
================
idPhysics_Player::IsJumping
================
*/
bool idPhysics_Player::IsJumping( void ) const {
	return ( !masterEntity&&( current.movementFlags & PMF_JUMP_HELD ) != 0 );
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
	
	savefile->WriteVec3( state.lastPushVelocity );	// cnicholson Added unsaved var
	
	savefile->WriteFloat( state.stepUp );
	savefile->WriteInt( state.movementType );
	savefile->WriteInt( state.movementFlags );
	savefile->WriteInt( state.movementTime );

	savefile->WriteInt( state.crouchSlideTime );	// cnicholson Added unsaved var
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

	savefile->ReadVec3( state.lastPushVelocity );	// cnicholson Added unrestored var

	savefile->ReadFloat( state.stepUp );
	savefile->ReadInt( state.movementType );
	savefile->ReadInt( state.movementFlags );
	savefile->ReadInt( state.movementTime );

	savefile->ReadInt( state.crouchSlideTime );		// cnicholson Added unrestored var
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
		assert( self );

		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + current.localOrigin * masterAxis;
		clipModel->Link( self, 0, current.origin, clipModel->GetAxis() );
		current.velocity = ( current.origin - oldOrigin ) / ( timeStepMSec * 0.001f );
		masterDeltaYaw = masterYaw;
		masterYaw = masterAxis[0].ToYaw();
		masterDeltaYaw = masterYaw - masterDeltaYaw;
		return true;
	}

	ActivateContactEntities();

	idPhysics_Player::MovePlayer( timeStepMSec );
	// I think the self is a remnant from early TV free fly code, can be removed now?
	if ( self ) {
		clipModel->Link( self, 0, current.origin, clipModel->GetAxis() );

		// IsOutsideWorld uses self, so it needs to be non null
		if ( IsOutsideWorld() ) {
			gameLocal.Warning( "clip model outside world bounds for entity '%s' at (%s)", self ? "NULL" : self->name.c_str(), current.origin.ToString(0) );
		}
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
// RAVEN BEGIN
// ddynerman: multiple clip worlds
	clipModel->Link( self, 0, current.origin, clipModel->GetAxis() );
// RAVEN END

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
		assert( self );
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + newOrigin * masterAxis;
	}
	else {
		current.origin = newOrigin;
	}
// RAVEN BEGIN
// ddynerman: multiple clip worlds
// TTimo: only if tied to an ent
	if ( self ) {
		clipModel->Link( self, 0, newOrigin, clipModel->GetAxis() );
	}
// RAVEN END
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
// RAVEN BEGIN
// ddynerman: multiple clip worlds
	clipModel->Link( self, 0, clipModel->GetOrigin(), newAxis );
// RAVEN END
}

/*
================
idPhysics_Player::Translate
================
*/
void idPhysics_Player::Translate( const idVec3 &translation, int id ) {

	current.localOrigin += translation;
	current.origin += translation;
// RAVEN BEGIN
// ddynerman: multiple clip worlds
	clipModel->Link( self, 0, current.origin, clipModel->GetAxis() );
// RAVEN END
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
// RAVEN BEGIN
// ddynerman: multiple clip worlds
	clipModel->Link( self, 0, current.origin, clipModel->GetAxis() * rotation.ToMat3() );
// RAVEN END
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
	return current.lastPushVelocity;
}

/*
================
idPhysics_Player::ClearPushedVelocity
================
*/
void idPhysics_Player::ClearPushedVelocity( void ) {
	current.pushVelocity.Zero();
	current.lastPushVelocity.Zero();
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
	} else {
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
	msg.WriteDeltaFloat( 0.0f, current.velocity[0] );
	msg.WriteDeltaFloat( 0.0f, current.velocity[1] );
	msg.WriteDeltaFloat( 0.0f, current.velocity[2] );

	msg.WriteDeltaFloat( current.origin[0], current.localOrigin[0] );
	msg.WriteDeltaFloat( current.origin[1], current.localOrigin[1] );
	msg.WriteDeltaFloat( current.origin[2], current.localOrigin[2] );
	msg.WriteDeltaFloat( 0.0f, current.pushVelocity[0] );
	msg.WriteDeltaFloat( 0.0f, current.pushVelocity[1] );
	msg.WriteDeltaFloat( 0.0f, current.pushVelocity[2] );

	msg.WriteDeltaFloat( 0.0f, current.stepUp );
	msg.WriteBits( current.movementType, PLAYER_MOVEMENT_TYPE_BITS );
	msg.WriteBits( current.movementFlags, PLAYER_MOVEMENT_FLAGS_BITS );
	msg.WriteDeltaLong( 0, current.movementTime );
	msg.WriteDeltaLong( 0, current.crouchSlideTime );
}

/*
================
idPhysics_Player::ReadFromSnapshot
================
*/
void idPhysics_Player::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	
	idVec3 oldOrigin = current.origin;

	current.origin[0] = msg.ReadFloat();
	current.origin[1] = msg.ReadFloat();
	current.origin[2] = msg.ReadFloat();

	GAMELOG_SET( "origin_delta_x", (current.origin - oldOrigin).x );
	GAMELOG_SET( "origin_delta_y", (current.origin - oldOrigin).y );
	GAMELOG_SET( "origin_delta_z", (current.origin - oldOrigin).z );

	idVec3 oldVelocity = current.velocity;

	current.velocity[0] = msg.ReadDeltaFloat( 0.0f );
	current.velocity[1] = msg.ReadDeltaFloat( 0.0f );
	current.velocity[2] = msg.ReadDeltaFloat( 0.0f );

	GAMELOG_SET( "velocity_delta_x", (current.velocity - oldVelocity).x );
	GAMELOG_SET( "velocity_delta_y", (current.velocity - oldVelocity).y );
	GAMELOG_SET( "velocity_delta_z", (current.velocity - oldVelocity).z );

	current.localOrigin[0] = msg.ReadDeltaFloat( current.origin[0] );
	current.localOrigin[1] = msg.ReadDeltaFloat( current.origin[1] );
	current.localOrigin[2] = msg.ReadDeltaFloat( current.origin[2] );
	current.pushVelocity[0] = msg.ReadDeltaFloat( 0.0f );
	current.pushVelocity[1] = msg.ReadDeltaFloat( 0.0f );
	current.pushVelocity[2] = msg.ReadDeltaFloat( 0.0f );

	current.stepUp = msg.ReadDeltaFloat( 0.0f );
	current.movementType = msg.ReadBits( PLAYER_MOVEMENT_TYPE_BITS );
	
	current.movementFlags = msg.ReadBits( PLAYER_MOVEMENT_FLAGS_BITS );

	current.movementTime = msg.ReadDeltaLong( 0 );
	current.crouchSlideTime = msg.ReadDeltaLong( 0 );

	if ( clipModel ) {
		clipModel->Link( self, 0, current.origin, clipModel->GetAxis() );
	}
}

/*
===============
idPhysics_Player::SetClipModelNoLink
===============
*/
void idPhysics_Player::SetClipModelNoLink( idClipModel *model ) {
	assert( model );
	assert( model->IsTraceModel() );

	if ( clipModel && clipModel != model ) {
		delete clipModel;
	}
	clipModel = model;
}
