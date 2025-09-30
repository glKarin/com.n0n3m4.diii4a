/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2021 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

q3rally source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with q3rally; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
// bg_pmove.c -- both games player movement code
// takes a playerstate and a usercmd as input and returns a modifed playerstate

#include "../qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_local.h"

pmove_t		*pm;
pml_t		pml;

// movement parameters
float	pm_stopspeed = 100.0f;
float	pm_duckScale = 0.25f;
float	pm_swimScale = 0.50f;

float	pm_accelerate = 10.0f;
float	pm_airaccelerate = 1.0f;
float	pm_wateraccelerate = 4.0f;
float	pm_flyaccelerate = 8.0f;

float	pm_friction = 6.0f;
float	pm_waterfriction = 1.0f;
float	pm_flightfriction = 3.0f;
float	pm_spectatorfriction = 5.0f;

int		c_pmove = 0;

// STONELANCE
int		curDelay;
// END

/*
===============
PM_AddEvent

===============
*/
void PM_AddEvent( int newEvent ) {
	BG_AddPredictableEventToPlayerstate( newEvent, 0, pm->ps );
}

/*
===============
PM_AddTouchEnt
===============
*/
// STONELANCE
// void PM_AddTouchEnt( int entityNum ) {
void PM_AddTouchEnt( int entityNum, vec3_t hitOrigin ) {
// END
	int		i;

	if ( entityNum == ENTITYNUM_WORLD ) {
		return;
	}
	if ( pm->numtouch == MAXTOUCH ) {
		return;
	}

	// see if it is already added
	for ( i = 0 ; i < pm->numtouch ; i++ ) {
		if ( pm->touchents[ i ] == entityNum ) {
			return;
		}
	}

	// add it
	pm->touchents[pm->numtouch] = entityNum;
// STONELANCE
	VectorCopy(hitOrigin, pm->touchPos[pm->numtouch]);
// END
	pm->numtouch++;
}

/*
===================
PM_StartTorsoAnim
===================
*/
static void PM_StartTorsoAnim( int anim ) {
	if ( pm->ps->pm_type >= PM_DEAD ) {
		return;
	}
// STONELANCE
/*
	pm->ps->torsoAnim = ( ( pm->ps->torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT )
		| anim;
*/
// END
}
#if 0
static void PM_StartLegsAnim( int anim ) {
	if ( pm->ps->pm_type >= PM_DEAD ) {
		return;
	}
// STONELANCE
/*
	if ( pm->ps->legsTimer > 0 ) {
		return;		// a high priority animation is running
	}
	pm->ps->legsAnim = ( ( pm->ps->legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT )
		| anim;
*/
// END
}

static void PM_ContinueLegsAnim( int anim ) {
	if ( ( pm->ps->legsAnim & ~ANIM_TOGGLEBIT ) == anim ) {
		return;
	}
// STONELANCE
/*
	if ( pm->ps->legsTimer > 0 ) {
		return;		// a high priority animation is running
	}
	PM_StartLegsAnim( anim );
*/
// END
}

static void PM_ContinueTorsoAnim( int anim ) {
	if ( ( pm->ps->torsoAnim & ~ANIM_TOGGLEBIT ) == anim ) {
		return;
	}
// STONELANCE
/*
	if ( pm->ps->torsoTimer > 0 ) {
		return;		// a high priority animation is running
	}
	PM_StartTorsoAnim( anim );
*/
// END
}

static void PM_ForceLegsAnim( int anim ) {
	pm->ps->legsTimer = 0;
	PM_StartLegsAnim( anim );
}
#endif


/*
==================
PM_ClipVelocity

Slide off of the impacting surface
==================
*/
void PM_ClipVelocity( vec3_t in, vec3_t normal, vec3_t out, float overbounce ) {
	float	backoff;
	float	change;
	int		i;
	
	backoff = DotProduct (in, normal);
	
	if ( backoff < 0 ) {
		backoff *= overbounce;
	} else {
		backoff /= overbounce;
	}

	for ( i=0 ; i<3 ; i++ ) {
		change = normal[i]*backoff;
		out[i] = in[i] - change;
	}
}


/*
==================
PM_Friction

Handles both ground friction and water friction
==================
*/
static void PM_Friction( void ) {
	vec3_t	vec;
	float	*vel;
	float	speed, newspeed, control;
	float	drop;
	
	vel = pm->ps->velocity;
	
	VectorCopy( vel, vec );
	if ( pml.walking ) {
		vec[2] = 0;	// ignore slope movement
	}

	speed = VectorLength(vec);
	if (speed < 1) {
		vel[0] = 0;
		vel[1] = 0;		// allow sinking underwater
		// FIXME: still have z friction underwater?
		return;
	}

	drop = 0;

	// apply ground friction
	if ( pm->waterlevel <= 1 ) {
		if ( pml.walking && !(pml.groundTrace.surfaceFlags & SURF_SLICK) ) {
			// if getting knocked back, no friction
			if ( ! (pm->ps->pm_flags & PMF_TIME_KNOCKBACK) ) {
				control = speed < pm_stopspeed ? pm_stopspeed : speed;
				drop += control*pm_friction*pml.frametime;
			}
		}
	}

	// apply water friction even if just wading
	if ( pm->waterlevel ) {
		drop += speed*pm_waterfriction*pm->waterlevel*pml.frametime;
	}

	// apply flying friction
// STONELANCE
/*
	if ( pm->ps->powerups[PW_FLIGHT]) {
		drop += speed*pm_flightfriction*pml.frametime;
	}
*/
// END
	if ( pm->ps->pm_type == PM_SPECTATOR) {
		drop += speed*pm_spectatorfriction*pml.frametime;
	}

	// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0) {
		newspeed = 0;
	}
	newspeed /= speed;

	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
}


/*
==============
PM_Accelerate

Handles user intended acceleration
==============
*/
static void PM_Accelerate( vec3_t wishdir, float wishspeed, float accel ) {
#if 1
	// q2 style
	int			i;
	float		addspeed, accelspeed, currentspeed;

	currentspeed = DotProduct (pm->ps->velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0) {
		return;
	}
	accelspeed = accel*pml.frametime*wishspeed;
	if (accelspeed > addspeed) {
		accelspeed = addspeed;
	}
	
	for (i=0 ; i<3 ; i++) {
		pm->ps->velocity[i] += accelspeed*wishdir[i];	
	}
#else
	// proper way (avoids strafe jump maxspeed bug), but feels bad
	vec3_t		wishVelocity;
	vec3_t		pushDir;
	float		pushLen;
	float		canPush;

	VectorScale( wishdir, wishspeed, wishVelocity );
	VectorSubtract( wishVelocity, pm->ps->velocity, pushDir );
	pushLen = VectorNormalize( pushDir );

	canPush = accel*pml.frametime*wishspeed;
	if (canPush > pushLen) {
		canPush = pushLen;
	}

	VectorMA( pm->ps->velocity, canPush, pushDir, pm->ps->velocity );
#endif
}



/*
============
PM_CmdScale

Returns the scale factor to apply to cmd movements
This allows the clients to use axial -127 to 127 values for all directions
without getting a sqrt(2) distortion in speed.
============
*/
static float PM_CmdScale( usercmd_t *cmd ) {
	int		max;
	float	total;
	float	scale;

	max = abs( cmd->forwardmove );
	if ( abs( cmd->rightmove ) > max ) {
		max = abs( cmd->rightmove );
	}
	if ( abs( cmd->upmove ) > max ) {
		max = abs( cmd->upmove );
	}
	if ( !max ) {
		return 0;
	}

	total = sqrt( cmd->forwardmove * cmd->forwardmove
		+ cmd->rightmove * cmd->rightmove + cmd->upmove * cmd->upmove );
	scale = (float)pm->ps->speed * max / ( 127.0 * total );

	return scale;
}


/*
================
PM_SetMovementDir

Determine the rotation of the legs relative
to the facing dir
================
*/
// STONELANCE - removed
/*
static void PM_SetMovementDir( void ) {
	if ( pm->cmd.forwardmove || pm->cmd.rightmove ) {
		if ( pm->cmd.rightmove == 0 && pm->cmd.forwardmove > 0 ) {
			pm->ps->movementDir = 0;
		} else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove > 0 ) {
			pm->ps->movementDir = 1;
		} else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove == 0 ) {
			pm->ps->movementDir = 2;
		} else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove < 0 ) {
			pm->ps->movementDir = 3;
		} else if ( pm->cmd.rightmove == 0 && pm->cmd.forwardmove < 0 ) {
			pm->ps->movementDir = 4;
		} else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove < 0 ) {
			pm->ps->movementDir = 5;
		} else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove == 0 ) {
			pm->ps->movementDir = 6;
		} else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove > 0 ) {
			pm->ps->movementDir = 7;
		}
	} else {
		// if they aren't actively going directly sideways,
		// change the animation to the diagonal so they
		// don't stop too crooked
		if ( pm->ps->movementDir == 2 ) {
			pm->ps->movementDir = 1;
		} else if ( pm->ps->movementDir == 6 ) {
			pm->ps->movementDir = 7;
		} 
	}
}
*/
// END


/*
=============
PM_CheckJump
=============
*/
// STONELANCE - removed
/*
static qboolean PM_CheckJump( void ) {
	if ( pm->ps->pm_flags & PMF_RESPAWNED ) {
		return qfalse;		// don't allow jump until all buttons are up
	}

	if ( pm->cmd.upmove < 10 ) {
		// not holding jump
		return qfalse;
	}

	// must wait for jump to be released
	if ( pm->ps->pm_flags & PMF_JUMP_HELD ) {
		// clear upmove so cmdscale doesn't lower running speed
		pm->cmd.upmove = 0;
		return qfalse;
	}

	pml.groundPlane = qfalse;		// jumping away
	pml.walking = qfalse;
	pm->ps->pm_flags |= PMF_JUMP_HELD;

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pm->ps->velocity[2] = JUMP_VELOCITY;
	PM_AddEvent( EV_JUMP );

	if ( pm->cmd.forwardmove >= 0 ) {
		PM_ForceLegsAnim( LEGS_JUMP );
		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	} else {
		PM_ForceLegsAnim( LEGS_JUMPB );
		pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
	}

	return qtrue;
}
*/
// END


/*
=============
PM_CheckWaterJump
=============
*/
// STOENLANCE - removed function
/*
static qboolean	PM_CheckWaterJump( void ) {
	vec3_t	spot;
	int		cont;
	vec3_t	flatforward;

	if (pm->ps->pm_time) {
		return qfalse;
	}

	// check for water jump
	if ( pm->waterlevel != 2 ) {
		return qfalse;
	}

	flatforward[0] = pml.forward[0];
	flatforward[1] = pml.forward[1];
	flatforward[2] = 0;
	VectorNormalize (flatforward);

	VectorMA (pm->ps->origin, 30, flatforward, spot);
	spot[2] += 4;
	cont = pm->pointcontents (spot, pm->ps->clientNum );
	if ( !(cont & CONTENTS_SOLID) ) {
		return qfalse;
	}

	spot[2] += 16;
	cont = pm->pointcontents (spot, pm->ps->clientNum );
	if ( cont & (CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY) ) {
		return qfalse;
	}

	// jump out of water
	VectorScale (pml.forward, 200, pm->ps->velocity);
	pm->ps->velocity[2] = 350;

	pm->ps->pm_flags |= PMF_TIME_WATERJUMP;
	pm->ps->pm_time = 2000;

	return qtrue;
}
*/
// END

//============================================================================


/*
===================
PM_WaterJumpMove

Flying out of the water
===================
*/
// STONELANCE - removed function
/*
static void PM_WaterJumpMove( void ) {
	// waterjump has no control, but falls

	PM_StepSlideMove( qtrue );

	pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
	if (pm->ps->velocity[2] < 0) {
		// cancel as soon as we are falling down again
		pm->ps->pm_flags &= ~PMF_ALL_TIMES;
		pm->ps->pm_time = 0;
	}
}
*/
// END


/*
===================
PM_WaterMove

===================
*/
// STONELANCE - removed
/*
static void PM_WaterMove( void ) {
	int		i;
	vec3_t	wishvel;
	float	wishspeed;
	vec3_t	wishdir;
	float	scale;
	float	vel;

	if ( PM_CheckWaterJump() ) {
		PM_WaterJumpMove();
		return;
	}
#if 0
	// jump = head for surface
	if ( pm->cmd.upmove >= 10 ) {
		if (pm->ps->velocity[2] > -300) {
			if ( pm->watertype & CONTENTS_WATER ) {
				pm->ps->velocity[2] = 100;
			} else if ( pm->watertype & CONTENTS_SLIME ) {
				pm->ps->velocity[2] = 80;
			} else {
				pm->ps->velocity[2] = 50;
			}
		}
	}
#endif
	PM_Friction ();

	scale = PM_CmdScale( &pm->cmd );
	//
	// user intentions
	//
	if ( !scale ) {
		wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = -60;		// sink towards bottom
	} else {
		for (i=0 ; i<3 ; i++)
			wishvel[i] = scale * pml.forward[i]*pm->cmd.forwardmove + scale * pml.right[i]*pm->cmd.rightmove;

		wishvel[2] += scale * pm->cmd.upmove;
	}

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	if ( wishspeed > pm->ps->speed * pm_swimScale ) {
		wishspeed = pm->ps->speed * pm_swimScale;
	}

	PM_Accelerate (wishdir, wishspeed, pm_wateraccelerate);

	// make sure we can go up slopes easily under water
	if ( pml.groundPlane && DotProduct( pm->ps->velocity, pml.groundTrace.plane.normal ) < 0 ) {
		vel = VectorLength(pm->ps->velocity);
		// slide along the ground plane
		PM_ClipVelocity (pm->ps->velocity, pml.groundTrace.plane.normal, 
			pm->ps->velocity, OVERCLIP );

		VectorNormalize(pm->ps->velocity);
		VectorScale(pm->ps->velocity, vel, pm->ps->velocity);
	}

	PM_SlideMove( qfalse );
}
*/
// END

#ifdef MISSIONPACK
/*
===================
PM_InvulnerabilityMove

Only with the invulnerability powerup
===================
*/
static void PM_InvulnerabilityMove( void ) {
	pm->cmd.forwardmove = 0;
	pm->cmd.rightmove = 0;
	pm->cmd.upmove = 0;
	VectorClear(pm->ps->velocity);
}
#endif

/*
===================
PM_FlyMove

Only with the flight powerup
===================
*/
static void PM_FlyMove( void ) {
	int		i;
	vec3_t	wishvel;
	float	wishspeed;
	vec3_t	wishdir;
	float	scale;

	// normal slowdown
	PM_Friction ();

	scale = PM_CmdScale( &pm->cmd );
	//
	// user intentions
	//
	if ( !scale ) {
		wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = 0;
	} else {
		for (i=0 ; i<3 ; i++) {
			wishvel[i] = scale * pml.forward[i]*pm->cmd.forwardmove + scale * pml.right[i]*pm->cmd.rightmove;
		}

		wishvel[2] += scale * pm->cmd.upmove;
	}

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	PM_Accelerate (wishdir, wishspeed, pm_flyaccelerate);

	PM_StepSlideMove( qfalse );
}


/*
===================
PM_AirMove

===================
*/
// STONELANCE - removed
/*
static void PM_AirMove( void ) {
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		scale;
	usercmd_t	cmd;

	PM_Friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = PM_CmdScale( &cmd );

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;
	VectorNormalize (pml.forward);
	VectorNormalize (pml.right);

	for ( i = 0 ; i < 2 ; i++ ) {
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	}
	wishvel[2] = 0;

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale;

	// not on ground, so little effect on velocity
	PM_Accelerate (wishdir, wishspeed, pm_airaccelerate);

	// we may have a ground plane that is very steep, even
	// though we don't have a groundentity
	// slide along the steep plane
	if ( pml.groundPlane ) {
		PM_ClipVelocity (pm->ps->velocity, pml.groundTrace.plane.normal, 
			pm->ps->velocity, OVERCLIP );
	}

#if 0
	//ZOID:  If we are on the grapple, try stair-stepping
	//this allows a player to use the grapple to pull himself
	//over a ledge
	if (pm->ps->pm_flags & PMF_GRAPPLE_PULL)
		PM_StepSlideMove ( qtrue );
	else
		PM_SlideMove ( qtrue );
#endif

	PM_StepSlideMove ( qtrue );
}
*/
// END

/*
===================
PM_GrappleMove

===================
*/
// STONELANCE - removed
/*
static void PM_GrappleMove( void ) {
	vec3_t vel, v;
	float vlen;

	VectorScale(pml.forward, -16, v);
	VectorAdd(pm->ps->grapplePoint, v, v);
	VectorSubtract(v, pm->ps->origin, vel);
	vlen = VectorLength(vel);
	VectorNormalize( vel );

	if (vlen <= 100)
		VectorScale(vel, 10 * vlen, vel);
	else
		VectorScale(vel, 800, vel);

	VectorCopy(vel, pm->ps->velocity);

	pml.groundPlane = qfalse;
}
*/
// END

/*
===================
PM_WalkMove

===================
*/
// STONELANCE - removed
/*
static void PM_WalkMove( void ) {
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		scale;
	usercmd_t	cmd;
	float		accelerate;
	float		vel;

	if ( pm->waterlevel > 2 && DotProduct( pml.forward, pml.groundTrace.plane.normal ) > 0 ) {
		// begin swimming
		PM_WaterMove();
		return;
	}


	if ( PM_CheckJump () ) {
		// jumped away
		if ( pm->waterlevel > 1 ) {
			PM_WaterMove();
		} else {
			PM_AirMove();
		}
		return;
	}

	PM_Friction ();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = PM_CmdScale( &cmd );

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;

	// project the forward and right directions onto the ground plane
	PM_ClipVelocity (pml.forward, pml.groundTrace.plane.normal, pml.forward, OVERCLIP );
	PM_ClipVelocity (pml.right, pml.groundTrace.plane.normal, pml.right, OVERCLIP );
	//
	VectorNormalize (pml.forward);
	VectorNormalize (pml.right);

	for ( i = 0 ; i < 3 ; i++ ) {
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	}
	// when going up or down slopes the wish velocity should Not be zero
//	wishvel[2] = 0;

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale;

	// clamp the speed lower if ducking
	if ( pm->ps->pm_flags & PMF_DUCKED ) {
		if ( wishspeed > pm->ps->speed * pm_duckScale ) {
			wishspeed = pm->ps->speed * pm_duckScale;
		}
	}

	// clamp the speed lower if wading or walking on the bottom
	if ( pm->waterlevel ) {
		float	waterScale;

		waterScale = pm->waterlevel / 3.0;
		waterScale = 1.0 - ( 1.0 - pm_swimScale ) * waterScale;
		if ( wishspeed > pm->ps->speed * waterScale ) {
			wishspeed = pm->ps->speed * waterScale;
		}
	}

	// when a player gets hit, they temporarily lose
	// full control, which allows them to be moved a bit
	if ( ( pml.groundTrace.surfaceFlags & SURF_SLICK ) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) {
		accelerate = pm_airaccelerate;
	} else {
		accelerate = pm_accelerate;
	}

	PM_Accelerate (wishdir, wishspeed, accelerate);

	//Com_Printf("velocity = %1.1f %1.1f %1.1f\n", pm->ps->velocity[0], pm->ps->velocity[1], pm->ps->velocity[2]);
	//Com_Printf("velocity1 = %1.1f\n", VectorLength(pm->ps->velocity));

	if ( ( pml.groundTrace.surfaceFlags & SURF_SLICK ) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) {
		pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
	} else {
		// don't reset the z velocity for slopes
//		pm->ps->velocity[2] = 0;
	}

	vel = VectorLength(pm->ps->velocity);

	// slide along the ground plane
	PM_ClipVelocity (pm->ps->velocity, pml.groundTrace.plane.normal, 
		pm->ps->velocity, OVERCLIP );

	// don't decrease velocity when going up or down a slope
	VectorNormalize(pm->ps->velocity);
	VectorScale(pm->ps->velocity, vel, pm->ps->velocity);

	// don't do anything if standing still
	if (!pm->ps->velocity[0] && !pm->ps->velocity[1]) {
		return;
	}

	PM_StepSlideMove( qfalse );

	//Com_Printf("velocity2 = %1.1f\n", VectorLength(pm->ps->velocity));

}
*/
// END

/*
==============
PM_DeadMove
==============
*/
// STONELANCE - removed function
/*
static void PM_DeadMove( void ) {
	float	forward;

	if ( !pml.walking ) {
		return;
	}

	// extra friction

	forward = VectorLength (pm->ps->velocity);
	forward -= 20;
	if ( forward <= 0 ) {
		VectorClear (pm->ps->velocity);
	} else {
		VectorNormalize (pm->ps->velocity);
		VectorScale (pm->ps->velocity, forward, pm->ps->velocity);
	}
}
*/
// END


/*
===============
PM_NoclipMove
===============
*/
static void PM_NoclipMove( void ) {
	float	speed, drop, friction, control, newspeed;
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		scale;

	pm->ps->viewheight = DEFAULT_VIEWHEIGHT;

	// friction

	speed = VectorLength (pm->ps->velocity);
	if (speed < 1)
	{
		VectorCopy (vec3_origin, pm->ps->velocity);
	}
	else
	{
		drop = 0;

		friction = pm_friction*1.5;	// extra friction
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control*friction*pml.frametime;

		// scale the velocity
		newspeed = speed - drop;
		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;

		VectorScale (pm->ps->velocity, newspeed, pm->ps->velocity);
	}

	// accelerate
	scale = PM_CmdScale( &pm->cmd );

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;
	
	for (i=0 ; i<3 ; i++)
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	wishvel[2] += pm->cmd.upmove;

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale;

	PM_Accelerate( wishdir, wishspeed, pm_accelerate );

	// move
	VectorMA (pm->ps->origin, pml.frametime, pm->ps->velocity, pm->ps->origin);

// STONELANCE
	pm->ps->viewangles[YAW] = pm->ps->damageAngles[YAW];
	PM_InitializeVehicle(pm->car, pm->ps->origin, pm->ps->viewangles, pm->ps->velocity /* , pm->car_frontweight_dist */ );
// END
}

//============================================================================

/*
================
PM_FootstepForSurface

Returns an event number appropriate for the groundsurface
================
*/
// STONELANCE - removed function
/*
static int PM_FootstepForSurface( void ) {
	if ( pml.groundTrace.surfaceFlags & SURF_NOSTEPS ) {
		return 0;
	}
	if ( pml.groundTrace.surfaceFlags & SURF_METALSTEPS ) {
		return EV_FOOTSTEP_METAL;
	}
	return EV_FOOTSTEP;
}
*/
// END


/*
=================
PM_CrashLand

Check for hard landings that generate sound events
=================
*/
// STONELANCE - removed function
/*
static void PM_CrashLand( void ) {
	float		delta;
	float		dist;
	float		vel, acc;
	float		t;
	float		a, b, c, den;

	// decide which landing animation to use
	if ( pm->ps->pm_flags & PMF_BACKWARDS_JUMP ) {
		PM_ForceLegsAnim( LEGS_LANDB );
	} else {
		PM_ForceLegsAnim( LEGS_LAND );
	}

	pm->ps->legsTimer = TIMER_LAND;

	// calculate the exact velocity on landing
	dist = pm->ps->origin[2] - pml.previous_origin[2];
	vel = pml.previous_velocity[2];
	acc = -pm->ps->gravity;

	a = acc / 2;
	b = vel;
	c = -dist;

	den =  b * b - 4 * a * c;
	if ( den < 0 ) {
		return;
	}
	t = (-b - sqrt( den ) ) / ( 2 * a );

	delta = vel + t * acc;
	delta = delta*delta * 0.0001;

	// ducking while falling doubles damage
	if ( pm->ps->pm_flags & PMF_DUCKED ) {
		delta *= 2;
	}

	// never take falling damage if completely underwater
	if ( pm->waterlevel == 3 ) {
		return;
	}

	// reduce falling damage if there is standing water
	if ( pm->waterlevel == 2 ) {
		delta *= 0.25;
	}
	if ( pm->waterlevel == 1 ) {
		delta *= 0.5;
	}

	if ( delta < 1 ) {
		return;
	}

	// create a local entity event to play the sound

	// SURF_NODAMAGE is used for bounce pads where you don't ever
	// want to take damage or play a crunch sound
	if ( !(pml.groundTrace.surfaceFlags & SURF_NODAMAGE) )  {
		if ( delta > 60 ) {
			PM_AddEvent( EV_FALL_FAR );
		} else if ( delta > 40 ) {
			// this is a pain grunt, so don't play it if dead
			if ( pm->ps->stats[STAT_HEALTH] > 0 ) {
				PM_AddEvent( EV_FALL_MEDIUM );
			}
		} else if ( delta > 7 ) {
			PM_AddEvent( EV_FALL_SHORT );
		} else {
			PM_AddEvent( PM_FootstepForSurface() );
		}
	}

	// start footstep cycle over
	pm->ps->bobCycle = 0;
}
*/
// END

/*
=============
PM_CheckStuck
=============
*/
/*
void PM_CheckStuck(void) {
	trace_t trace;

	pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, pm->ps->origin, pm->ps->clientNum, pm->tracemask);
	if (trace.allsolid) {
		//int shit = qtrue;
	}
}
*/

/*
=============
PM_CorrectAllSolid
=============
*/
// STONELANCE - removed
/*
static int PM_CorrectAllSolid( trace_t *trace ) {
	int			i, j, k;
	vec3_t		point;

	if ( pm->debugLevel ) {
		Com_Printf("%i:allsolid\n", c_pmove);
	}

	// jitter around
	for (i = -1; i <= 1; i++) {
		for (j = -1; j <= 1; j++) {
			for (k = -1; k <= 1; k++) {
				VectorCopy(pm->ps->origin, point);
				point[0] += (float) i;
				point[1] += (float) j;
				point[2] += (float) k;
				pm->trace (trace, point, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
				if ( !trace->allsolid ) {
					point[0] = pm->ps->origin[0];
					point[1] = pm->ps->origin[1];
					point[2] = pm->ps->origin[2] - 0.25;

					pm->trace (trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
					pml.groundTrace = *trace;
					return qtrue;
				}
			}
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;

	return qfalse;
}
*/
// END


/*
=============
PM_GroundTraceMissed

The ground trace didn't hit a surface, so we are in freefall
=============
*/
static void PM_GroundTraceMissed( void ) {
	trace_t		trace;
	vec3_t		point;

	if ( pm->ps->groundEntityNum != ENTITYNUM_NONE ) {
		// we just transitioned into freefall
		if ( pm->debugLevel ) {
			Com_Printf("%i:lift\n", c_pmove);
		}

		// if they aren't in a jumping animation and the ground is a ways away, force into it
		// if we didn't do the trace, the player would be backflipping down staircases
		VectorCopy( pm->ps->origin, point );
		point[2] -= 64;

		pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
// STONELANCE
/*
		if ( trace.fraction == 1.0 ) {
			if ( pm->cmd.forwardmove >= 0 ) {
				PM_ForceLegsAnim( LEGS_JUMP );
				pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
			} else {
				PM_ForceLegsAnim( LEGS_JUMPB );
				pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
			}
		}
*/
// END
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;
}


/*
=============
PM_GroundTrace
=============
*/
static void PM_GroundTrace( void ) {
	vec3_t		point;
	trace_t		trace;

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] - 0.25;

	pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
	pml.groundTrace = trace;

	// do something corrective if the trace starts in a solid...
// STONELANCE - dont bother with this
/*
	if ( trace.allsolid ) {
		if ( !PM_CorrectAllSolid(&trace) )
			return;
	}
*/
// END

	// if the trace didn't hit anything, we are in free fall
	if ( trace.fraction == 1.0 ) {
		PM_GroundTraceMissed();
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

	// check if getting thrown off the ground
	if ( pm->ps->velocity[2] > 0 && DotProduct( pm->ps->velocity, trace.plane.normal ) > 10 ) {
		if ( pm->debugLevel ) {
			Com_Printf("%i:kickoff\n", c_pmove);
		}
		// go into jump animation

// STONELANCE
/*
		if ( pm->cmd.forwardmove >= 0 ) {
			PM_ForceLegsAnim( LEGS_JUMP );
			pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
		} else {
			PM_ForceLegsAnim( LEGS_JUMPB );
			pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
		}
*/
// END

		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}
	
	// slopes that are too steep will not be considered onground
	if ( trace.plane.normal[2] < MIN_WALK_NORMAL ) {
		if ( pm->debugLevel ) {
			Com_Printf("%i:steep\n", c_pmove);
		}
		// FIXME: if they can't slide down the slope, let them
		// walk (sharp crevices)
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qtrue;
		pml.walking = qfalse;
		return;
	}

	pml.groundPlane = qtrue;
	pml.walking = qtrue;

	// hitting solid ground will end a waterjump
	if (pm->ps->pm_flags & PMF_TIME_WATERJUMP)
	{
		pm->ps->pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND);
		pm->ps->pm_time = 0;
	}

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE ) {
		// just hit the ground
		if ( pm->debugLevel ) {
			Com_Printf("%i:Land\n", c_pmove);
		}
		
// STONELANCE - removed function
//		PM_CrashLand();
// END

		// don't do landing time if we were just going down a slope
		if ( pml.previous_velocity[2] < -200 ) {
			// don't allow another jump for a little while
			pm->ps->pm_flags |= PMF_TIME_LAND;
			pm->ps->pm_time = 250;
		}
	}

	pm->ps->groundEntityNum = trace.entityNum;

	// don't reset the z velocity for slopes
//	pm->ps->velocity[2] = 0;

// STONELANCE - handled in bg_physics trace_points
//	PM_AddTouchEnt( trace.entityNum );
// END
}


/*
=============
PM_SetWaterLevel	FIXME: avoid this twice?  certainly if not moving
=============
*/
static void PM_SetWaterLevel( void ) {
	vec3_t		point;
	int			cont;
	int			sample1;
	int			sample2;

	//
	// get waterlevel, accounting for ducking
	//
	pm->waterlevel = 0;
	pm->watertype = 0;

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] + MINS_Z + 1;	
	cont = pm->pointcontents( point, pm->ps->clientNum );

	if ( cont & MASK_WATER ) {
		sample2 = pm->ps->viewheight - MINS_Z;
		sample1 = sample2 / 2;

		pm->watertype = cont;
		pm->waterlevel = 1;
		point[2] = pm->ps->origin[2] + MINS_Z + sample1;
		cont = pm->pointcontents (point, pm->ps->clientNum );
		if ( cont & MASK_WATER ) {
			pm->waterlevel = 2;
			point[2] = pm->ps->origin[2] + MINS_Z + sample2;
			cont = pm->pointcontents (point, pm->ps->clientNum );
			if ( cont & MASK_WATER ){
				pm->waterlevel = 3;
			}
		}
	}

}

/*
==============
PM_CheckDuck

Sets mins, maxs, and pm->ps->viewheight
==============
*/
static void PM_CheckDuck (void)
{
	trace_t	trace;

#ifdef MISSIONPACK
	if ( pm->ps->powerups[PW_INVULNERABILITY] ) {
		if ( pm->ps->pm_flags & PMF_INVULEXPAND ) {
			// invulnerability sphere has a 42 units radius
			VectorSet( pm->mins, -42, -42, -42 );
			VectorSet( pm->maxs, 42, 42, 42 );
		}
		else {
			VectorSet( pm->mins, -15, -15, MINS_Z );
			VectorSet( pm->maxs, 15, 15, 16 );
		}
		pm->ps->pm_flags |= PMF_DUCKED;
		pm->ps->viewheight = CROUCH_VIEWHEIGHT;
		return;
	}
	pm->ps->pm_flags &= ~PMF_INVULEXPAND;
#endif

	pm->mins[0] = -15;
	pm->mins[1] = -15;

	pm->maxs[0] = 15;
	pm->maxs[1] = 15;

	pm->mins[2] = MINS_Z;

	if (pm->ps->pm_type == PM_DEAD)
	{
		pm->maxs[2] = -8;
		pm->ps->viewheight = DEAD_VIEWHEIGHT;
		return;
	}

	if (pm->cmd.upmove < 0)
	{	// duck
		pm->ps->pm_flags |= PMF_DUCKED;
	}
	else
	{	// stand up if possible
		if (pm->ps->pm_flags & PMF_DUCKED)
		{
			// try to stand up
			pm->maxs[2] = 32;
			pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, pm->ps->origin, pm->ps->clientNum, pm->tracemask );
			if (!trace.allsolid)
				pm->ps->pm_flags &= ~PMF_DUCKED;
		}
	}

	if (pm->ps->pm_flags & PMF_DUCKED)
	{
		pm->maxs[2] = 16;
		pm->ps->viewheight = CROUCH_VIEWHEIGHT;
	}
	else
	{
		pm->maxs[2] = 32;
		pm->ps->viewheight = DEFAULT_VIEWHEIGHT;
	}
}



//===================================================================


/*
===============
PM_Footsteps
===============
*/
// STOENLANCE - no feet = no footstep sounds - removed function
#if 0
static void PM_Footsteps( void ) {
	float		bobmove;
	int			old;
	qboolean	footstep;

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	pm->xyspeed = sqrt( pm->ps->velocity[0] * pm->ps->velocity[0]
		+  pm->ps->velocity[1] * pm->ps->velocity[1] );

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE ) {

		if ( pm->ps->powerups[PW_INVULNERABILITY] ) {
			PM_ContinueLegsAnim( LEGS_IDLECR );
		}
		// airborne leaves position in cycle intact, but doesn't advance
		if ( pm->waterlevel > 1 ) {
			PM_ContinueLegsAnim( LEGS_SWIM );
		}
		return;
	}

	// if not trying to move
	if ( !pm->cmd.forwardmove && !pm->cmd.rightmove ) {
		if (  pm->xyspeed < 5 ) {
			pm->ps->bobCycle = 0;	// start at beginning of cycle again
			if ( pm->ps->pm_flags & PMF_DUCKED ) {
				PM_ContinueLegsAnim( LEGS_IDLECR );
			} else {
				PM_ContinueLegsAnim( LEGS_IDLE );
			}
		}
		return;
	}
	

	footstep = qfalse;

	if ( pm->ps->pm_flags & PMF_DUCKED ) {
		bobmove = 0.5;	// ducked characters bob much faster
		if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
			PM_ContinueLegsAnim( LEGS_BACKCR );
		}
		else {
			PM_ContinueLegsAnim( LEGS_WALKCR );
		}
		// ducked characters never play footsteps
	/*
	} else 	if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
		if ( !( pm->cmd.buttons & BUTTON_WALKING ) ) {
			bobmove = 0.4;	// faster speeds bob faster
			footstep = qtrue;
		} else {
			bobmove = 0.3;
		}
		PM_ContinueLegsAnim( LEGS_BACK );
	*/
	} else {
		if ( !( pm->cmd.buttons & BUTTON_WALKING ) ) {
			bobmove = 0.4f;	// faster speeds bob faster
			if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
				PM_ContinueLegsAnim( LEGS_BACK );
			}
			else {
				PM_ContinueLegsAnim( LEGS_RUN );
			}
			footstep = qtrue;
		} else {
			bobmove = 0.3f;	// walking bobs slow
			if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
				PM_ContinueLegsAnim( LEGS_BACKWALK );
			}
			else {
				PM_ContinueLegsAnim( LEGS_WALK );
			}
		}
	}

	// check for footstep / splash sounds
	old = pm->ps->bobCycle;
	pm->ps->bobCycle = (int)( old + bobmove * pml.msec ) & 255;

	// if we just crossed a cycle boundary, play an appropriate footstep event
	if ( ( ( old + 64 ) ^ ( pm->ps->bobCycle + 64 ) ) & 128 ) {
		if ( pm->waterlevel == 0 ) {
			// on ground will only play sounds if running
			if ( footstep && !pm->noFootsteps ) {
				PM_AddEvent( PM_FootstepForSurface() );
			}
		} else if ( pm->waterlevel == 1 ) {
			// splashing
			PM_AddEvent( EV_FOOTSPLASH );
		} else if ( pm->waterlevel == 2 ) {
			// wading / swimming at surface
			PM_AddEvent( EV_SWIM );
		} else if ( pm->waterlevel == 3 ) {
			// no sound when completely underwater

		}
	}
}
#endif
// END


/*
==============
PM_WaterEvents

Generate sound events for entering and leaving water
==============
*/
static void PM_WaterEvents( void ) {		// FIXME?
	//
	// if just entered a water volume, play a sound
	//
	if (!pml.previous_waterlevel && pm->waterlevel) {
		PM_AddEvent( EV_WATER_TOUCH );
	}

	//
	// if just completely exited a water volume, play a sound
	//
	if (pml.previous_waterlevel && !pm->waterlevel) {
		PM_AddEvent( EV_WATER_LEAVE );
	}

	//
	// check for head just going under water
	//
	if (pml.previous_waterlevel != 3 && pm->waterlevel == 3) {
		PM_AddEvent( EV_WATER_UNDER );
	}

	//
	// check for head just coming out of water
	//
	if (pml.previous_waterlevel == 3 && pm->waterlevel != 3) {
		PM_AddEvent( EV_WATER_CLEAR );
	}
}


/*
===============
PM_BeginWeaponChange
===============
*/
static void PM_BeginWeaponChange( int weapon ) {
// STONELANCE
//	if ( weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS ) {
	if ( weapon <= WP_NONE || weapon >= RWP_SMOKE ) {
// END
		return;
	}

	if ( !( pm->ps->stats[STAT_WEAPONS] & ( 1 << weapon ) ) ) {
		return;
	}
	
	if ( pm->ps->weaponstate == WEAPON_DROPPING ) {
		return;
	}

	PM_AddEvent( EV_CHANGE_WEAPON );
	pm->ps->weaponstate = WEAPON_DROPPING;
	pm->ps->weaponTime += 200;
	PM_StartTorsoAnim( TORSO_DROP );
}


/*
===============
PM_FinishWeaponChange
===============
*/
static void PM_FinishWeaponChange( void ) {
	int		weapon;

	weapon = pm->cmd.weapon;
	if ( weapon < WP_NONE || weapon >= WP_NUM_WEAPONS ) {
		weapon = WP_NONE;
	}

	if ( !( pm->ps->stats[STAT_WEAPONS] & ( 1 << weapon ) ) ) {
		weapon = WP_NONE;
	}

	pm->ps->weapon = weapon;
	pm->ps->weaponstate = WEAPON_RAISING;
	pm->ps->weaponTime += 250;
	PM_StartTorsoAnim( TORSO_RAISE );
}


/*
==============
PM_TorsoAnimation

==============
*/
// STONELANCE - removed
/*
static void PM_TorsoAnimation( void ) {
	if ( pm->ps->weaponstate == WEAPON_READY ) {
		if ( pm->ps->weapon == WP_GAUNTLET ) {
			PM_ContinueTorsoAnim( TORSO_STAND2 );
		} else {
			PM_ContinueTorsoAnim( TORSO_STAND );
		}
		return;
	}
}
*/
// END


/*
==============
PM_Weapon

Generates weapon events and modifes the weapon counter
==============
*/
static void PM_Weapon( void ) {
	int		addTime;

	// don't allow attack until all buttons are up
	if ( pm->ps->pm_flags & PMF_RESPAWNED ) {
		return;
	}

	// ignore if spectator
	if ( pm->ps->persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		return;
	}

	// check for dead player
	if ( pm->ps->stats[STAT_HEALTH] <= 0 ) {
		pm->ps->weapon = WP_NONE;
		return;
	}

	// check for item using
	if ( pm->cmd.buttons & BUTTON_USE_HOLDABLE ) {
		if ( ! ( pm->ps->pm_flags & PMF_USE_ITEM_HELD ) ) {
			if ( bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag == HI_MEDKIT
				&& pm->ps->stats[STAT_HEALTH] >= (pm->ps->stats[STAT_MAX_HEALTH] + 25) ) {
				// don't use medkit if at max health
			} else {
				pm->ps->pm_flags |= PMF_USE_ITEM_HELD;
				PM_AddEvent( EV_USE_ITEM0 + bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag );
				pm->ps->stats[STAT_HOLDABLE_ITEM] = 0;
			}
			return;
		}
	} else {
		pm->ps->pm_flags &= ~PMF_USE_ITEM_HELD;
	}


	// make weapon function
// STONELANCE
//	if ( pm->ps->weaponTime > 0 ) {
//		pm->ps->weaponTime -= pml.msec;
	if ( (pm->ps->weaponTime & NORMAL_WEAPON_TIME_MASK) > 0 ) {
		if ((pml.msec & NORMAL_WEAPON_TIME_MASK) >= (pm->ps->weaponTime & NORMAL_WEAPON_TIME_MASK))
			pm->ps->weaponTime &= REAR_WEAPON_TIME_MASK; // = 0
		else
			pm->ps->weaponTime -= (pml.msec & NORMAL_WEAPON_TIME_MASK);
// END
	}

	// check for weapon change
	// can't change if weapon is firing, but can change
	// again if lowering or raising
// STONELANCE
//	if ( pm->ps->weaponTime <= 0 || pm->ps->weaponstate != WEAPON_FIRING ) {
	if ( (pm->ps->weaponTime & NORMAL_WEAPON_TIME_MASK) <= 0 || pm->ps->weaponstate != WEAPON_FIRING ) {
// END
		if ( pm->ps->weapon != pm->cmd.weapon ) {
			PM_BeginWeaponChange( pm->cmd.weapon );
		}
	}

// STONELANCE
//	if ( pm->ps->weaponTime > 0 ) {
	if ( (pm->ps->weaponTime & NORMAL_WEAPON_TIME_MASK) > 0 ) {
		return;
	}
// END

	// change weapon if time
	if ( pm->ps->weaponstate == WEAPON_DROPPING ) {
		PM_FinishWeaponChange();
		return;
	}

	if ( pm->ps->weaponstate == WEAPON_RAISING ) {
		pm->ps->weaponstate = WEAPON_READY;
// STONELANCE
/*
		if ( pm->ps->weapon == WP_GAUNTLET ) {
			PM_StartTorsoAnim( TORSO_STAND2 );
		} else {
			PM_StartTorsoAnim( TORSO_STAND );
		}
*/
// END
		return;
	}

// STONELANCE
	if (pm->ps->weapon == WP_NONE){
		return;
	}
// END

	// check for fire
	if ( ! (pm->cmd.buttons & BUTTON_ATTACK) ) {
// STONELANCE
//		pm->ps->weaponTime = 0;
		pm->ps->weaponTime &= REAR_WEAPON_TIME_MASK;
// END
		pm->ps->weaponstate = WEAPON_READY;
		return;
	}

	// start the animation even if out of ammo
	if ( pm->ps->weapon == WP_GAUNTLET ) {
		// the guantlet only "fires" when it actually hits something
		if ( !pm->gauntletHit ) {
// STONELANCE
//			pm->ps->weaponTime = 0;
			pm->ps->weaponTime &= REAR_WEAPON_TIME_MASK;
// END
			pm->ps->weaponstate = WEAPON_READY;
			return;
		}
// STONELANCE
	}
/*
		PM_StartTorsoAnim( TORSO_ATTACK2 );
	} else {
		PM_StartTorsoAnim( TORSO_ATTACK );
	}
*/
// END

	pm->ps->weaponstate = WEAPON_FIRING;

	// check for out of ammo
	if ( ! pm->ps->ammo[ pm->ps->weapon ] ) {
		PM_AddEvent( EV_NOAMMO );
// STONELANCE
//		pm->ps->weaponTime += 500;
		pm->ps->weaponTime += (500 & NORMAL_WEAPON_TIME_MASK);
// END
		return;
	}

	// take an ammo away if not infinite
	if ( pm->ps->ammo[ pm->ps->weapon ] != -1 ) {
		pm->ps->ammo[ pm->ps->weapon ]--;
	}

	// fire weapon
	PM_AddEvent( EV_FIRE_WEAPON );

	switch( pm->ps->weapon ) {
	default:
	case WP_GAUNTLET:
		addTime = 400;
		break;
	case WP_LIGHTNING:
		addTime = 50;
		break;
	case WP_SHOTGUN:
		addTime = 1000;
		break;
	case WP_MACHINEGUN:
		addTime = 100;
		break;
	case WP_GRENADE_LAUNCHER:
		//addTime = 800; //TBB - too fast
		addTime = 1000; //TBB
		break;
	case WP_ROCKET_LAUNCHER:
		//addTime = 800; //TBB - too fast
		addTime = 1200; //TBB
		break;
	case WP_PLASMAGUN:
		//addTime = 100; //TBB - too fast
		addTime = 300; //TBB - compensate for slight increase damage relative to machine gun
		break;
	case WP_RAILGUN:
		addTime = 1500;
		break;
	case WP_BFG:
		addTime = 200;
		break;
// Q3Rally Code Start
/*
	case WP_GRAPPLING_HOOK:
		addTime = 400;
		break;
*/
  case WP_FLAME_THROWER:
    //addTime = 40;	//TBB bit too short
	  addTime = 150; 
    break;
// Q3Rally Code END
#ifdef MISSIONPACK
	case WP_NAILGUN:
		addTime = 1000;
		break;
	case WP_PROX_LAUNCHER:
		addTime = 800;
		break;
	case WP_CHAINGUN:
		addTime = 30;
		break;
#endif
	}

#ifdef MISSIONPACK
	if( bg_itemlist[pm->ps->stats[STAT_PERSISTANT_POWERUP]].giTag == PW_SCOUT ) {
		addTime /= 1.5;
	}
	else
	if( bg_itemlist[pm->ps->stats[STAT_PERSISTANT_POWERUP]].giTag == PW_AMMOREGEN ) {
		addTime /= 1.3;
	}
	else
#endif

	if ( pm->ps->powerups[PW_HASTE] ) {
// STONELANCE
//		addTime /= 1.3;
		addTime /= 2.0;
// END
	}

// STONELANCE
//	pm->ps->weaponTime += addTime;
	pm->ps->weaponTime += (addTime & NORMAL_WEAPON_TIME_MASK);
// END
}

/*
==============
PM_ALT_Weapon

Generates alt_weapon events and modifes the alt_weapon counter
==============
*/
static void PM_Alt_Weapon( void ) {
	int		addTime;
	int		ammo_use;

	// don't allow attack until all buttons are up
	if ( pm->ps->pm_flags & PMF_RESPAWNED ) {
		return;
	}

	// ignore if spectator
	if ( pm->ps->persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		return;
	}

	// check for dead player
	if ( pm->ps->stats[STAT_HEALTH] <= 0 ) {
		pm->ps->weapon = WP_NONE;
		return;
	}

	// check for item using
	if ( pm->cmd.buttons & BUTTON_USE_HOLDABLE ) {
		if ( ! ( pm->ps->pm_flags & PMF_USE_ITEM_HELD ) ) {
			if ( bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag == HI_MEDKIT
				&& pm->ps->stats[STAT_HEALTH] >= (pm->ps->stats[STAT_MAX_HEALTH] + 25) ) {
				// don't use medkit if at max health
			} else {
				pm->ps->pm_flags |= PMF_USE_ITEM_HELD;
				PM_AddEvent( EV_USE_ITEM0 + bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag );
				pm->ps->stats[STAT_HOLDABLE_ITEM] = 0;
			}
			return;
		}
	} else {
		pm->ps->pm_flags &= ~PMF_USE_ITEM_HELD;
	}


	// make weapon function
// STONELANCE
//	if ( pm->ps->weaponTime > 0 ) {
//		pm->ps->weaponTime -= pml.msec;
	if ( (pm->ps->weaponTime & NORMAL_WEAPON_TIME_MASK) > 0 ) {
		if ((pml.msec & NORMAL_WEAPON_TIME_MASK) >= (pm->ps->weaponTime & NORMAL_WEAPON_TIME_MASK))
			pm->ps->weaponTime &= REAR_WEAPON_TIME_MASK; // = 0
		else
			pm->ps->weaponTime -= (pml.msec & NORMAL_WEAPON_TIME_MASK);
// END
	}

	// check for weapon change
	// can't change if weapon is firing, but can change
	// again if lowering or raising
// STONELANCE
//	if ( pm->ps->weaponTime <= 0 || pm->ps->weaponstate != WEAPON_FIRING ) {
	if ( (pm->ps->weaponTime & NORMAL_WEAPON_TIME_MASK) <= 0 || pm->ps->weaponstate != WEAPON_FIRING ) {
// END
		if ( pm->ps->weapon != pm->cmd.weapon ) {
			PM_BeginWeaponChange( pm->cmd.weapon );
		}
	}

// STONELANCE
//	if ( pm->ps->weaponTime > 0 ) {
	if ( (pm->ps->weaponTime & NORMAL_WEAPON_TIME_MASK) > 0 ) {
		return;
	}
// END

	// change weapon if time
	if ( pm->ps->weaponstate == WEAPON_DROPPING ) {
		PM_FinishWeaponChange();
		return;
	}

	if ( pm->ps->weaponstate == WEAPON_RAISING ) {
		pm->ps->weaponstate = WEAPON_READY;
// STONELANCE
/*
		if ( pm->ps->weapon == WP_GAUNTLET ) {
			PM_StartTorsoAnim( TORSO_STAND2 );
		} else {
			PM_StartTorsoAnim( TORSO_STAND );
		}
*/
// END
		return;
	}

// STONELANCE
	if (pm->ps->weapon == WP_NONE){
		return;
	}
// END

	// check for fire
	if ( ! (pm->cmd.buttons & BUTTON_ALT_ATTACK) ) {
// STONELANCE
//		pm->ps->weaponTime = 0;
		pm->ps->weaponTime &= REAR_WEAPON_TIME_MASK;
// END
		pm->ps->weaponstate = WEAPON_READY;
		return;
	}

	// start the animation even if out of ammo
	if ( pm->ps->weapon == WP_GAUNTLET ) {
		// the guantlet only "fires" when it actually hits something
		if ( !pm->gauntletHit ) {
// STONELANCE
//			pm->ps->weaponTime = 0;
			pm->ps->weaponTime &= REAR_WEAPON_TIME_MASK;
// END
			pm->ps->weaponstate = WEAPON_READY;
			return;
		}
// STONELANCE
	}
/*
		PM_StartTorsoAnim( TORSO_ATTACK2 );
	} else {
		PM_StartTorsoAnim( TORSO_ATTACK );
	}
*/
// END

	pm->ps->weaponstate = WEAPON_FIRING;

	// check for out of ammo
	if ( ! pm->ps->ammo[ pm->ps->weapon ] ){
	//if ( ! pm->ps->ammo[ pm->ps->weapon ] && pm->ps->ammo[ pm->ps->weapon ] < -1 ) {
		PM_AddEvent( EV_NOAMMO );
// STONELANCE
//		pm->ps->weaponTime += 500;
		pm->ps->weaponTime += (500 & NORMAL_WEAPON_TIME_MASK);
// END
		return;
	}
	
 /*
	// take an ammo away if not infinite
	if ( pm->ps->ammo[ pm->ps->weapon ] != -1 ) {
		pm->ps->ammo[ pm->ps->weapon ]--;
	}
   */
   
	/* TBB - AMMO MANAGEMENT, the goal:
	if there is not enough ammo to activate alt,
	keep the same ammo amount without switching the weapon       */

	switch( pm->ps->weapon ) {	
	case WP_RAILGUN:
		ammo_use = 10; //megarailgun
		break;
	case WP_LIGHTNING:
		ammo_use = 1;
		break;
	case WP_MACHINEGUN:
		ammo_use = 1;
		break;
	case WP_SHOTGUN:
		ammo_use = 1;
		break;
	case WP_GRENADE_LAUNCHER:
		ammo_use = 3; //there are 6 trajectories in g_missile.c: G_ExplodeCluster()
		break;
	case WP_ROCKET_LAUNCHER:
		ammo_use = 3; //homing
		break;
	case WP_PLASMAGUN:
		ammo_use = 1; //rotational defense
		break;
	case WP_BFG:
		ammo_use = 1;
		break;
	case WP_FLAME_THROWER:
		ammo_use = 3;
		break;
	default:
		ammo_use = 1;
		break;
	}
	
	if ( pm->ps->ammo[ pm->ps->weapon ] > ammo_use ) {
		 pm->ps->ammo[ pm->ps->weapon ] -= ammo_use;
	} else if ( pm->ps->ammo[ pm->ps->weapon ] == ammo_use ) {
		pm->ps->ammo[ pm->ps->weapon ] = 0;
	} else if ( pm->ps->ammo[ pm->ps->weapon ] < ammo_use) {
		pm->ps->ammo[ pm->ps->weapon ] = pm->ps->ammo[ pm->ps->weapon ];			
		return;
	}
//TBB FIN 

	// fire weapon
	PM_AddEvent( EV_ALTFIRE_WEAPON );

	switch( pm->ps->weapon ) {
	default:
	case WP_GAUNTLET:
		addTime = 400;
		break;
	case WP_LIGHTNING:
		addTime = 50;
		break;
	case WP_SHOTGUN:
		addTime = 1000;
		break;
	case WP_MACHINEGUN:
		addTime = 100;
		break;
	case WP_GRENADE_LAUNCHER:
		addTime = 1200;
		break;
	case WP_ROCKET_LAUNCHER:
		addTime = 2000;
		break;
	case WP_PLASMAGUN:
		//TBB - using alt plasma, will use up 2 more cells from regular 1
		addTime = 1000; //TBB new plasmagun 
		//TBB FIN
		break;
	case WP_RAILGUN:
		addTime = 3000;
		break;
	case WP_BFG:
		addTime = 200;
		break;
// Q3Rally Code Start
/*
	case WP_GRAPPLING_HOOK:
		addTime = 400;
		break;
*/
  case WP_FLAME_THROWER:
	//TBB using alt flame, will use up 2 more flames from regular 1
	//addTime = 40;
	addTime = 300;	//increased addtime for more control of ammo use per mouse click
	//TBB FIN
    break;
// Q3Rally Code END
#ifdef MISSIONPACK
	case WP_NAILGUN:
		addTime = 1000;
		break;
	case WP_PROX_LAUNCHER:
		addTime = 800;
		break;
	case WP_CHAINGUN:
		addTime = 30;
		break;
#endif
	}

#ifdef MISSIONPACK
	if( bg_itemlist[pm->ps->stats[STAT_PERSISTANT_POWERUP]].giTag == PW_SCOUT ) {
		addTime /= 1.5;
	}
	else
	if( bg_itemlist[pm->ps->stats[STAT_PERSISTANT_POWERUP]].giTag == PW_AMMOREGEN ) {
		addTime /= 1.3;
  }
  else
#endif

	if ( pm->ps->powerups[PW_HASTE] ) {
// STONELANCE
//		addTime /= 1.3;
		addTime /= 2.0;
// END
	}

// STONELANCE
//	pm->ps->weaponTime += addTime;
	pm->ps->weaponTime += (addTime & NORMAL_WEAPON_TIME_MASK);
// END
}



// STONELANCE
/*
==============
PM_RearWeapon

Generates rearweapon events and modifes the weapon counter
==============
*/
void PM_RearWeapon( void ) {
	int			weapon, addTime, i;

	// don't allow attack until all buttons are up
	if ( pm->ps->pm_flags & PMF_RESPAWNED ) {
		return;
	}

	// ignore if spectator
	if ( pm->ps->persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		return;
	}

	// check for dead player
	if ( pm->ps->stats[STAT_HEALTH] <= 0 ) {
		pm->ps->weapon = WP_NONE;
		return;
	}

	for (i = RWP_SMOKE; i < WP_NUM_WEAPONS; i++){
		if (pm->ps->stats[STAT_WEAPONS] & ( 1 << i ) && !pm->ps->ammo[ i ]){
			pm->ps->stats[STAT_WEAPONS] &= ~( 1 << i );
		}

		if (pm->ps->stats[STAT_WEAPONS] & ( 1 << i ) && pm->ps->ammo[ i ])
			break;
	}

	if (i < WP_NUM_WEAPONS){
		weapon = i;
	}
	else {
		return; // no weapon
	}

	// make weapon function
	if ( (pm->ps->weaponTime & REAR_WEAPON_TIME_MASK) > 0 ) {
		if ((pml.msec & NORMAL_WEAPON_TIME_MASK) >= ((pm->ps->weaponTime & REAR_WEAPON_TIME_MASK) >> 16))
			pm->ps->weaponTime &= NORMAL_WEAPON_TIME_MASK; // = 0
		else
			pm->ps->weaponTime -= ((pml.msec & NORMAL_WEAPON_TIME_MASK) << 16);
	}

	if ( (pm->ps->weaponTime & REAR_WEAPON_TIME_MASK) > 0 ) {
		return;
	}

	// check for fire
	if ( !(pm->cmd.buttons & BUTTON_REARATTACK) ) {
		pm->ps->weaponTime &= NORMAL_WEAPON_TIME_MASK;
		return;
	}

	// check for out of ammo
	if ( !pm->ps->ammo[ weapon ] ) {
//		PM_AddEvent( EV_NOAMMO );
		pm->ps->weaponTime += (500 & NORMAL_WEAPON_TIME_MASK) << 16;
		return;
	}

	// take an ammo away if not infinite
	if ( pm->ps->ammo[ weapon ] != -1 ) {
		pm->ps->ammo[ weapon ]--;
	}

//	Com_Printf( "pm->ps->weaponTime %i\n", ( pm->ps->weaponTime & REAR_WEAPON_TIME_MASK ) >> 16 );

	// fire weapon
#ifndef CGAME
	BG_AddPredictableEventToPlayerstate( EV_FIRE_REARWEAPON, weapon, pm->ps );
#endif

	switch( weapon ) {
	default:
	case RWP_OIL:
		addTime = 200;
		break;
	case RWP_SMOKE:
		addTime = 600;
		break;
	case RWP_BIO:
		addTime = 200;
		break;
	case RWP_MINE:
		addTime = 1500;
		break;
	case RWP_FLAME:
		addTime = 600;
		break;
	}

	if ( pm->ps->powerups[PW_HASTE] ) {
		addTime /= 2.0;
	}

	pm->ps->weaponTime += ((addTime & NORMAL_WEAPON_TIME_MASK) << 16);

//	Com_Printf( "pm->ps->weaponTime %i\n", ( pm->ps->weaponTime & REAR_WEAPON_TIME_MASK ) >> 16 );
}
// END


/*
================
PM_Animate
================
*/
// STONELANCE (removed function)
/*
static void PM_Animate( void ) {
	if ( pm->cmd.buttons & BUTTON_GESTURE ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_GESTURE );
			pm->ps->torsoTimer = TIMER_GESTURE;
			PM_AddEvent( EV_TAUNT );
		}
#ifdef MISSIONPACK
	} else if ( pm->cmd.buttons & BUTTON_GETFLAG ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_GETFLAG );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	} else if ( pm->cmd.buttons & BUTTON_GUARDBASE ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_GUARDBASE );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	} else if ( pm->cmd.buttons & BUTTON_PATROL ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_PATROL );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	} else if ( pm->cmd.buttons & BUTTON_FOLLOWME ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_FOLLOWME );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	} else if ( pm->cmd.buttons & BUTTON_AFFIRMATIVE ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_AFFIRMATIVE);
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	} else if ( pm->cmd.buttons & BUTTON_NEGATIVE ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_NEGATIVE );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
#endif
	}
}
*/
// END


/*
================
PM_DropTimers
================
*/
static void PM_DropTimers( void ) {
	// drop misc timing counter
	if ( pm->ps->pm_time ) {
		if ( pml.msec >= pm->ps->pm_time ) {
			pm->ps->pm_flags &= ~PMF_ALL_TIMES;
			pm->ps->pm_time = 0;
		} else {
			pm->ps->pm_time -= pml.msec;
		}
	}

	// drop animation counter
// STONELANCE
/*
	if ( pm->ps->legsTimer > 0 ) {
		pm->ps->legsTimer -= pml.msec;
		if ( pm->ps->legsTimer < 0 ) {
			pm->ps->legsTimer = 0;
		}
	}

	if ( pm->ps->torsoTimer > 0 ) {
		pm->ps->torsoTimer -= pml.msec;
		if ( pm->ps->torsoTimer < 0 ) {
			pm->ps->torsoTimer = 0;
		}
	}
*/
// END
}

/*
================
PM_UpdateViewAngles

This can be used as another entry point when only the viewangles
are being updated instead of a full move
================
*/
// STONELANCE
void PM_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd, int controlMode ) {
// void PM_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd ) {
// END
	short		temp;
	int		i;

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPINTERMISSION) {
		ps->damageAngles[PITCH] = BYTE2ANGLE( ps->damagePitch );
		ps->damageAngles[YAW] = BYTE2ANGLE( ps->damageYaw );
		ps->damageAngles[ROLL] = 0;
		return;		// no view changes at all
	}

// STONELANCE - can still look around when dead
/*
	if ( ps->pm_type != PM_SPECTATOR && ps->stats[STAT_HEALTH] <= 0 ) {
		return;		// no view changes at all
	}
*/
// END

	// circularly clamp the angles with deltas
	for (i=0 ; i<3 ; i++) {
		temp = cmd->angles[i] + ps->delta_angles[i];
		if ( i == PITCH ) {
			// don't let the player look up or down more than 90 degrees
			if ( temp > 16000 ) {
				ps->delta_angles[i] = 16000 - cmd->angles[i];
				temp = 16000;
			} else if ( temp < -16000 ) {
				ps->delta_angles[i] = -16000 - cmd->angles[i];
				temp = -16000;
			}
		}

// Q3Rally
//		ps->viewangles[i] = SHORT2ANGLE(temp);
		if ( controlMode == CT_MOUSE ) {
			ps->damageAngles[i] = SHORT2ANGLE(temp);
		}
// END
	}

// STONELANCE use damage yaw and pitch for view angles
	if ( controlMode == CT_MOUSE ) {
		// camera view angle
		ps->damagePitch = ANGLE2BYTE( SHORT2ANGLE( ps->damageAngles[PITCH] ) );
		ps->damageYaw = ANGLE2BYTE( SHORT2ANGLE( ps->damageAngles[YAW] ) );
	} else /* CT_JOYSTICK */ {
		// wheel angle
		ps->damageAngles[PITCH] = BYTE2ANGLE( ps->damagePitch );
		ps->damageAngles[YAW] = BYTE2ANGLE( ps->damageYaw );
		ps->damageAngles[ROLL] = 0;
	}
// END
}


/*
================
PmoveSingle

================
*/
void trap_SnapVector( float *v );

void PmoveSingle (pmove_t *pmove) {
// STONELANCE
	//vec3_t	delta;
	float	dot;
// END

	pm = pmove;

	// this counter lets us debug movement problems with a journal
	// by setting a conditional breakpoint fot the previous frame
	c_pmove++;

	// clear results
	pm->numtouch = 0;
	pm->watertype = 0;
	pm->waterlevel = 0;

// STONELANCE
/*
	if ( pm->ps->stats[STAT_HEALTH] <= 0 ) {
		pm->tracemask &= ~CONTENTS_BODY;	// corpses can fly through bodies
	}
*/
// END

	// make sure walking button is clear if they are running, to avoid
	// proxy no-footsteps cheats
// STONELANCE
/*
	if ( abs( pm->cmd.forwardmove ) > 64 || abs( pm->cmd.rightmove ) > 64 ) {
		pm->cmd.buttons &= ~BUTTON_WALKING;
	}
*/
// END

	// set the talk balloon flag
	if ( pm->cmd.buttons & BUTTON_TALK ) {
		pm->ps->eFlags |= EF_TALK;
	} else {
		pm->ps->eFlags &= ~EF_TALK;
	}

	// set the firing flag for continuous beam weapons
	if ( !(pm->ps->pm_flags & PMF_RESPAWNED) && pm->ps->pm_type != PM_INTERMISSION && pm->ps->pm_type != PM_NOCLIP
		&& ( pm->cmd.buttons & BUTTON_ATTACK ) && pm->ps->ammo[ pm->ps->weapon ] ) {
		pm->ps->eFlags |= EF_FIRING;
	} else {
		pm->ps->eFlags &= ~EF_FIRING;
	}

	// clear the respawned flag if attack and use are cleared
	if ( pm->ps->stats[STAT_HEALTH] > 0 && 
		!( pm->cmd.buttons & (BUTTON_ATTACK | BUTTON_USE_HOLDABLE) ) ) {
		pm->ps->pm_flags &= ~PMF_RESPAWNED;
	}

	// if talk button is down, dissallow all other input
	// this is to prevent any possible intercept proxy from
	// adding fake talk balloons
	if ( pmove->cmd.buttons & BUTTON_TALK ) {
		// keep the talk button set tho for when the cmd.serverTime > 66 msec
		// and the same cmd is used multiple times in Pmove
		pmove->cmd.buttons = BUTTON_TALK;
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove = 0;
		pmove->cmd.upmove = 0;
	}

	// clear all pmove local vars
	memset (&pml, 0, sizeof(pml));

	// determine the time
	pml.msec = pmove->cmd.serverTime - pm->ps->commandTime;
	if ( pml.msec < 1 ) {
		pml.msec = 1;
	} else if ( pml.msec > 200 ) {
		pml.msec = 200;
	}
	pm->ps->commandTime = pmove->cmd.serverTime;

	// save old org in case we get stuck
	VectorCopy (pm->ps->origin, pml.previous_origin);

	// save old velocity for crashlanding
	VectorCopy (pm->ps->velocity, pml.previous_velocity);

	pml.frametime = pml.msec * 0.001;

	// update the viewangles
// STONELANCE
//	PM_UpdateViewAngles( pm->ps, &pm->cmd );
	PM_UpdateViewAngles( pm->ps, &pm->cmd, pm->controlMode );

//	AngleVectors (pm->ps->viewangles, pml.forward, pml.right, pml.up);
	AngleVectors (pm->ps->damageAngles, pml.forward, pml.right, pml.up);
// END

	if ( pm->cmd.upmove < 10 ) {
		// not holding jump
		pm->ps->pm_flags &= ~PMF_JUMP_HELD;
	}

	// decide if backpedaling animations should be used
// STONELANCE
/*
	if ( pm->cmd.forwardmove < 0 ) {
		pm->ps->pm_flags |= PMF_BACKWARDS_RUN;
	} else if ( pm->cmd.forwardmove > 0 || ( pm->cmd.forwardmove == 0 && pm->cmd.rightmove ) ) {
		pm->ps->pm_flags &= ~PMF_BACKWARDS_RUN;
	}
*/
// END

	if ( pm->ps->pm_type >= PM_DEAD ) {
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
	}

	if ( pm->ps->pm_type == PM_SPECTATOR ) {
		PM_CheckDuck ();
		PM_FlyMove ();
		PM_DropTimers ();
		return;
	}

	if ( pm->ps->pm_type == PM_NOCLIP ) {
		PM_NoclipMove ();
		PM_DropTimers ();
		return;
	}

	if (pm->ps->pm_type == PM_FREEZE) {
		return;		// no movement at all
	}

	if ( pm->ps->pm_type == PM_INTERMISSION || pm->ps->pm_type == PM_SPINTERMISSION) {
		return;		// no movement at all
	}

	// set watertype, and waterlevel
// STONELANCE - FIXME calculate water levels in bg_physics
	PM_SetWaterLevel();
	pml.previous_waterlevel = pmove->waterlevel;

	// set mins, maxs, and viewheight
// STONELANCE
//	PM_CheckDuck ();
	VectorSet(pm->mins, -CAR_WIDTH/2, -CAR_WIDTH/2, -CAR_WIDTH/2);
	VectorSet(pm->maxs, CAR_WIDTH/2, CAR_WIDTH/2, CAR_WIDTH/2);
	pm->ps->viewheight = 0;
// END

	// set groundentity
// STONELANCE
/*
	PM_GroundTrace();
	if ( pm->ps->pm_type == PM_DEAD ) {
		PM_DeadMove ();
	}
*/
// END

	PM_DropTimers();

// SKWID( PM_DriveMove does it all )
// STONELANCE
	// dont do physics if the player has been gibed
	if ( pm->ps->stats[STAT_HEALTH] > GIB_HEALTH ){
		int		i;

		pml.physicsSplit = 0;
		PM_DriveMove(pm->car, pml.frametime, qtrue);

		if( VectorNAN( pm->car->sBody.r ) )
			VectorClear( pm->car->sBody.r );
		if( VectorNAN( pm->car->sBody.v ) )
			VectorClear( pm->car->sBody.v );
		if( VectorNAN( pm->car->sBody.L ) )
			VectorClear( pm->car->sBody.L );

		// translate car values to player angles, etc
		VectorCopy(pm->car->sBody.v, pm->ps->velocity);
		VectorCopy(pm->car->sBody.r, pm->ps->origin);
		VectorCopy(pm->car->sBody.L, pm->ps->angularMomentum);
		OrientationToAngles(pm->car->sBody.t, pm->ps->viewangles);

		if( VectorNAN( pm->ps->viewangles ) )
			VectorClear( pm->ps->viewangles );

//		VectorSubtract(pm->car->sPoints[FL_FRAME].r, pm->car->sPoints[FL_WHEEL].r, delta);
//		dot = DotProduct(delta, pm->car->sBody.up);
		dot = pm->car->sBody.curSpringLengths[FL_WHEEL] - CP_SPRING_MINLEN;
		if (dot > CP_SPRING_MAXLEN - CP_SPRING_MINLEN)
			dot = CP_SPRING_MAXLEN - CP_SPRING_MINLEN;
		else if (dot < 0)
			dot = 0;
		pm->ps->legsTimer = (int)(CP_SPRING_SCALE * dot);

//		VectorSubtract(pm->car->sPoints[FR_FRAME].r, pm->car->sPoints[FR_WHEEL].r, delta);
//		dot = DotProduct(delta, pm->car->sBody.up);
		dot = pm->car->sBody.curSpringLengths[FR_WHEEL] - CP_SPRING_MINLEN;
		if (dot > CP_SPRING_MAXLEN - CP_SPRING_MINLEN)
			dot = CP_SPRING_MAXLEN - CP_SPRING_MINLEN;
		else if (dot < 0)
			dot = 0;
		pm->ps->legsAnim = (int)(CP_SPRING_SCALE * dot);

//		VectorSubtract(pm->car->sPoints[RL_FRAME].r, pm->car->sPoints[RL_WHEEL].r, delta);
//		dot = DotProduct(delta, pm->car->sBody.up);
		dot = pm->car->sBody.curSpringLengths[RL_WHEEL] - CP_SPRING_MINLEN;
		if (dot > CP_SPRING_MAXLEN - CP_SPRING_MINLEN)
			dot = CP_SPRING_MAXLEN - CP_SPRING_MINLEN;
		else if (dot < 0)
			dot = 0;
		pm->ps->torsoTimer = (int)(CP_SPRING_SCALE * dot);

//		VectorSubtract(pm->car->sPoints[RR_FRAME].r, pm->car->sPoints[RR_WHEEL].r, delta);
//		dot = DotProduct(delta, pm->car->sBody.up);
		dot = pm->car->sBody.curSpringLengths[RR_WHEEL] - CP_SPRING_MINLEN;
		if (dot > CP_SPRING_MAXLEN - CP_SPRING_MINLEN)
			dot = CP_SPRING_MAXLEN - CP_SPRING_MINLEN;
		else if (dot < 0)
			dot = 0;
		pm->ps->torsoAnim = (int)(CP_SPRING_SCALE * dot);

		pm->ps->stats[STAT_RPM] = pm->car->rpm;
		pm->ps->stats[STAT_GEAR] = pm->car->gear;

		// used to keep track of time since last onGround for resetCar
/*
		VectorSet(delta, 0, 0, 1);
		for (i = 0; i < NUM_CAR_POINTS; i++){
			if (pm->car->sPoints[i].onGround && DotProduct(pm->car->sPoints[i].normals[0], delta) > 0.3){
				pm->car->sPoints[i].onGroundTime = pm->cmd.serverTime;
				pm->car->tPoints[i].onGroundTime = pm->cmd.serverTime;
			}
			else {
				pm->car->sPoints[i].offGroundTime = pm->cmd.serverTime;
				pm->car->tPoints[i].offGroundTime = pm->cmd.serverTime;
			}
		}
*/

		for (i = 0; i < FL_FRAME; i++)
		{
			if( pm->car->sPoints[i].onGround && pm->car->sPoints[i].normals[0][2] > 0.3f )
			{
				pm->car->wheelOnGroundTime = pm->cmd.serverTime;
				break;
			}
		}

		if( pm->car->wheelOnGroundTime != pm->cmd.serverTime )
			pm->car->wheelsOffGroundTime = pm->cmd.serverTime;

		for (i = FL_FRAME; i < NUM_CAR_POINTS; i++)
		{
			if( pm->car->sPoints[i].onGround )
			{
				pm->car->onGroundTime = pm->cmd.serverTime;
				break;
			}
		}

		if( pm->car->onGroundTime < pm->cmd.serverTime - 100 )
			pm->car->offGroundTime = pm->cmd.serverTime;
	}
// END
/*

#ifdef MISSIONPACK
	if ( pm->ps->powerups[PW_INVULNERABILITY] ) {
		PM_InvulnerabilityMove();
	} else
#endif
	if ( pm->ps->powerups[PW_FLIGHT] ) {
		// flight powerup doesn't allow jump and has different friction
		PM_FlyMove();
	} else if (pm->ps->pm_flags & PMF_GRAPPLE_PULL) {
		PM_GrappleMove();
		// We can wiggle a bit
		PM_AirMove();
	} else if (pm->ps->pm_flags & PMF_TIME_WATERJUMP) {
		PM_WaterJumpMove();
	} else if ( pm->waterlevel > 1 ) {
		// swimming
		PM_WaterMove();
	} else if ( pml.walking ) {
		// walking on ground
		PM_WalkMove();
	} else {
		// airborne
		PM_AirMove();
	}
*/
// END

// STONELANCE
//	PM_Animate();
// END

	// set groundentity, watertype, and waterlevel
	PM_GroundTrace();
	PM_SetWaterLevel();

	// weapons
	PM_Weapon();
	PM_Alt_Weapon();
// STONELANCE
	PM_RearWeapon();
// END

	// torso animation
// STONELANCE
//	PM_TorsoAnimation();
// END

	// footstep events / legs animations
// SKWID( removed function )
//	PM_Footsteps();
// END

	// entering / leaving water splashes
	PM_WaterEvents();

	// snap some parts of playerstate to save network bandwidth
// STONELANCE
//	trap_SnapVector( pm->ps->velocity );
// END

	pm = NULL;
}


/*
================
Pmove

Can be called by either the server or the client
================
*/
void Pmove (pmove_t *pmove) {
	int			finalTime;

	finalTime = pmove->cmd.serverTime;

	if ( finalTime < pmove->ps->commandTime ) {
		return;	// should not happen
	}

	if ( finalTime > pmove->ps->commandTime + 1000 ) {
		pmove->ps->commandTime = finalTime - 1000;
	}

	pmove->ps->pmove_framecount = (pmove->ps->pmove_framecount+1) & ((1<<PS_PMOVEFRAMECOUNTBITS)-1);

// STONELANCE
/*
	if (pmove->client){
		Com_Printf("client time %d, msec %d\n", pmove->ps->commandTime, finalTime - pmove->ps->commandTime);
	else
		Com_Printf("server time %d, msec %d\n", pmove->ps->commandTime, finalTime - pmove->ps->commandTime);

	Com_Printf("r %f, %f, %f\n", pmove->car->sBody.r[0], pmove->car->sBody.r[1], pmove->car->sBody.r[2]);
	Com_Printf("v %f, %f, %f\n", pmove->car->sBody.v[0], pmove->car->sBody.v[1], pmove->car->sBody.v[2]);
	Com_Printf("w %f, %f, %f\n", pmove->car->sBody.w[0], pmove->car->sBody.w[1], pmove->car->sBody.w[2]);
	Com_Printf("L %f, %f, %f\n", pmove->car->sBody.L[0], pmove->car->sBody.L[1], pmove->car->sBody.L[2]);
	Com_Printf("CoM %f, %f, %f\n", pmove->car->sBody.CoM[0], pmove->car->sBody.CoM[1], pmove->car->sBody.CoM[2]);
	Com_Printf("t:\n");
	Com_Printf("%f, %f, %f\n", pmove->car->sBody.t[0][0], pmove->car->sBody.t[0][1], pmove->car->sBody.t[0][2]);
	Com_Printf("%f, %f, %f\n", pmove->car->sBody.t[1][0], pmove->car->sBody.t[1][1], pmove->car->sBody.t[1][2]);
	Com_Printf("%f, %f, %f\n", pmove->car->sBody.t[2][0], pmove->car->sBody.t[2][1], pmove->car->sBody.t[2][2]);
*/

/*
	if (pmove->client){
		Com_Printf("client time %d, msec %d\n", pmove->ps->commandTime, finalTime - pmove->ps->commandTime);
	else
		Com_Printf("server time %d, msec %d\n", pmove->ps->commandTime, finalTime - pmove->ps->commandTime);

	Com_Printf("springStrength %f\n", pmove->car->springStrength);
	Com_Printf("springMaxLength %f\n", pmove->car->springMaxLength);
	Com_Printf("springMinLength %f\n", pmove->car->springMinLength);
	Com_Printf("shockStrength %f\n", pmove->car->shockStrength);
	Com_Printf("wheelAngle %f\n", pmove->car->wheelAngle);
	Com_Printf("throttle %f\n", pmove->car->throttle);
	Com_Printf("gear %d\n", pmove->car->gear);
	Com_Printf("rpm %f\n", pmove->car->rpm);
	Com_Printf("aCOF %f\n", pmove->car->aCOF);
	Com_Printf("sCOF %f\n", pmove->car->sCOF);
	Com_Printf("kCOF %f\n", pmove->car->kCOF);
	Com_Printf("dfCOF %f\n", pmove->car->dfCOF);
	Com_Printf("ewCOF %f\n", pmove->car->ewCOF);
	Com_Printf("inverseBodyInertiaTensor:\n");
	Com_Printf("%f, %f, %f\n", pmove->car->inverseBodyInertiaTensor[0][0], pmove->car->inverseBodyInertiaTensor[0][1], pmove->car->inverseBodyInertiaTensor[0][2]);
	Com_Printf("%f, %f, %f\n", pmove->car->inverseBodyInertiaTensor[1][0], pmove->car->inverseBodyInertiaTensor[1][1], pmove->car->inverseBodyInertiaTensor[1][2]);
	Com_Printf("%f, %f, %f\n", pmove->car->inverseBodyInertiaTensor[2][0], pmove->car->inverseBodyInertiaTensor[2][1], pmove->car->inverseBodyInertiaTensor[2][2]);
*/

/*
	if (pmove->client){
		Com_Printf("client time %d, msec %d\n", pmove->ps->commandTime, finalTime - pmove->ps->commandTime);
	else
		Com_Printf("server time %d, msec %d\n", pmove->ps->commandTime, finalTime - pmove->ps->commandTime);

	Com_Printf("r %f, %f, %f\n", pmove->car->sPoints[i].r[0], pmove->car->sPoints[i].r[1], pmove->car->sPoints[i].r[2]);
	Com_Printf("v %f, %f, %f\n", pmove->car->sPoints[i].v[0], pmove->car->sPoints[i].v[1], pmove->car->sPoints[i].v[2]);
	Com_Printf("w %f\n", pmove->car->sPoints[i].w);
	Com_Printf("netForce %f, %f, %f\n", pmove->car->sPoints[i].netForce[0], pmove->car->sPoints[i].netForce[1], pmove->car->sPoints[i].netForce[2]);
	Com_Printf("netMoment %f\n", pmove->car->sPoints[i].netMoment);
	Com_Printf("normals %f, %f, %f\n", pmove->car->sPoints[i].normals[0][0], pmove->car->sPoints[i].normals[0][1], pmove->car->sPoints[i].normals[0][2]);
	Com_Printf("mass %f\n", pmove->car->sPoints[i].mass);
	Com_Printf("elasticity %f\n", pmove->car->sPoints[i].elasticity);
	Com_Printf("kcof %f\n", pmove->car->sPoints[i].kcof);
	Com_Printf("scof %f\n", pmove->car->sPoints[i].scof);
	Com_Printf("fluidDensity %f\n", pmove->car->sPoints[i].fluidDensity);
	Com_Printf("onGround %d\n", pmove->car->sPoints[i].onGround);
	Com_Printf("slipping %d\n", pmove->car->sPoints[i].slipping);
*/	
// END

	// chop the move up if it is too long, to prevent framerate
	// dependent behavior
	while ( pmove->ps->commandTime != finalTime ) {
		int		msec;

		msec = finalTime - pmove->ps->commandTime;

		if ( pmove->pmove_fixed ) {
			if ( msec > pmove->pmove_msec ) {
				msec = pmove->pmove_msec;
			}
		}
		else {
// STONELANCE
//			if ( msec > 66 ) {
//				msec = 66;
//			}

			if ( msec > 12 ) {
				msec = 12;
			}
// END
		}

		pmove->cmd.serverTime = pmove->ps->commandTime + msec;
		PmoveSingle( pmove );

// STONELANCE
//		if ( pmove->ps->pm_flags & PMF_JUMP_HELD ) {
//			pmove->cmd.upmove = 20;
//		}
// END
	}

	//PM_CheckStuck();

}


