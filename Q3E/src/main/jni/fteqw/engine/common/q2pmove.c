/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "quakedef.h"

float	pm_q2stepheight = PM_DEFAULTSTEPHEIGHT;
#if defined(Q2CLIENT) || defined(Q2SERVER)

#define	Q2PMF_DUCKED			1
#define	Q2PMF_JUMP_HELD		2
#define	Q2PMF_ON_GROUND		4
#define	Q2PMF_TIME_WATERJUMP	8	// pm_time is waterjump
#define	Q2PMF_TIME_LAND		16	// pm_time is time before rejump
#define	Q2PMF_TIME_TELEPORT	32	// pm_time is non-moving time
#define Q2PMF_NO_PREDICTION	64	// temporarily disables prediction (used for grappling hook)

// all of the locals will be zeroed before each
// pmove, just to make damn sure we don't have
// any differences when running on client or server

typedef struct
{
	vec3_t		origin;			// full float precision
	vec3_t		velocity;		// full float precision

	vec3_t		forward, right, up;
	float		frametime;


	const q2csurface_t	*groundsurface;
	cplane_t	groundplane;
	int			groundcontents;

	vec3_t		previous_origin;
	qboolean	ladder;
} q2pml_t;

static q2pmove_t		*q2pm;
static q2pml_t	q2pml;


// movement parameters
static float	pm_stopspeed = 100;
static float	pm_maxspeed = 300;
static float	pm_duckspeed = 100;
static float	pm_accelerate = 10;
float	pm_airaccelerate = 0;
static float	pm_wateraccelerate = 10;
static float	pm_friction = 6;
static float	pm_waterfriction = 1;
static float	pm_waterspeed = 400;
//float	pm_stepheight;

/*

  walking up a step should kill some velocity

*/


/*
==================
PMQ2_ClipVelocity

Slide off of the impacting object
returns the blocked flags (1 = floor, 2 = step / wall)
==================
*/
#define	STOP_EPSILON	0.1

void PMQ2_ClipVelocity (vec3_t in, vec3_t normal, vec3_t out, float overbounce)
{
	float	backoff;
	float	change;
	int		i;
	
	backoff = DotProduct (in, normal) * overbounce;

	for (i=0 ; i<3 ; i++)
	{
		change = normal[i]*backoff;
		out[i] = in[i] - change;
		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
			out[i] = 0;
	}
}




/*
==================
PMQ2_StepSlideMove

Each intersection will try to step over the obstruction instead of
sliding along it.

Returns a new origin, velocity, and contact entity
Does not modify any world state?
==================
*/
#define	MIN_STEP_NORMAL	0.7		// can't step up onto very steep slopes
#define	MAX_CLIP_PLANES	5
void PMQ2_StepSlideMove_ (void)
{
	int			bumpcount, numbumps;
	vec3_t		dir;
	float		d;
	int			numplanes;
	vec3_t		planes[MAX_CLIP_PLANES];
	vec3_t		primal_velocity;
	int			i, j;
	q2trace_t	trace;
	vec3_t		end;
	float		time_left;
	
	numbumps = 4;
	
	VectorCopy (q2pml.velocity, primal_velocity);
	numplanes = 0;
	
	time_left = q2pml.frametime;

	for (bumpcount=0 ; bumpcount<numbumps ; bumpcount++)
	{
		for (i=0 ; i<3 ; i++)
			end[i] = q2pml.origin[i] + time_left * q2pml.velocity[i];

		trace = q2pm->trace (q2pml.origin, q2pm->mins, q2pm->maxs, end);

		if (trace.allsolid)
		{	// entity is trapped in another solid
			q2pml.velocity[2] = 0;	// don't build up falling damage
			return;
		}

		if (trace.fraction > 0)
		{	// actually covered some distance
			VectorCopy (trace.endpos, q2pml.origin);
			numplanes = 0;
		}

		if (trace.fraction == 1)
			 break;		// moved the entire distance

		// save entity for contact
		if (q2pm->numtouch < MAXTOUCH && trace.ent)
		{
			q2pm->touchents[q2pm->numtouch] = trace.ent;
			q2pm->numtouch++;
		}
		
		time_left -= time_left * trace.fraction;

		// slide along this plane
		if (numplanes >= MAX_CLIP_PLANES)
		{	// this shouldn't really happen
			VectorClear (q2pml.velocity);
			break;
		}

		VectorCopy (trace.plane.normal, planes[numplanes]);
		numplanes++;

#if 0
	float		rub;

		//
		// modify velocity so it parallels all of the clip planes
		//
		if (numplanes == 1)
		{	// go along this plane
			VectorCopy (q2pml.velocity, dir);
			VectorNormalize (dir);
			rub = 1.0 + 0.5 * DotProduct (dir, planes[0]);

			// slide along the plane
			PMQ2_ClipVelocity (q2pml.velocity, planes[0], q2pml.velocity, 1.01);
			// rub some extra speed off on xy axis
			// not on Z, or you can scrub down walls
			q2pml.velocity[0] *= rub;
			q2pml.velocity[1] *= rub;
			q2pml.velocity[2] *= rub;
		}
		else if (numplanes == 2)
		{	// go along the crease
			VectorCopy (q2pml.velocity, dir);
			VectorNormalize (dir);
			rub = 1.0 + 0.5 * DotProduct (dir, planes[0]);

			// slide along the plane
			CrossProduct (planes[0], planes[1], dir);
			d = DotProduct (dir, q2pml.velocity);
			VectorScale (dir, d, q2pml.velocity);

			// rub some extra speed off
			VectorScale (q2pml.velocity, rub, q2pml.velocity);
		}
		else
		{
//			Con_Printf ("clip velocity, numplanes == %i\n",numplanes);
			VectorClear (q2pml.velocity);
			break;
		}

#else
//
// modify original_velocity so it parallels all of the clip planes
//
		for (i=0 ; i<numplanes ; i++)
		{
			PMQ2_ClipVelocity (q2pml.velocity, planes[i], q2pml.velocity, 1.01);
			for (j=0 ; j<numplanes ; j++)
				if (j != i)
				{
					if (DotProduct (q2pml.velocity, planes[j]) < 0)
						break;	// not ok
				}
			if (j == numplanes)
				break;
		}
		
		if (i != numplanes)
		{	// go along this plane
		}
		else
		{	// go along the crease
			if (numplanes != 2)
			{
//				Con_Printf ("clip velocity, numplanes == %i\n",numplanes);
				VectorClear (q2pml.velocity);
				break;
			}
			CrossProduct (planes[0], planes[1], dir);
			d = DotProduct (dir, q2pml.velocity);
			VectorScale (dir, d, q2pml.velocity);
		}
#endif
		//
		// if velocity is against the original velocity, stop dead
		// to avoid tiny occilations in sloping corners
		//
		if (DotProduct (q2pml.velocity, primal_velocity) <= 0)
		{
			VectorClear (q2pml.velocity);
			break;
		}
	}

	if (q2pm->s.pm_time)
	{
		VectorCopy (primal_velocity, q2pml.velocity);
	}
}

/*
==================
PMQ2_StepSlideMove

==================
*/
void PMQ2_StepSlideMove (void)
{
	vec3_t		start_o, start_v;
	vec3_t		down_o, down_v;
	q2trace_t		trace;
	float		down_dist, up_dist;
//	vec3_t		delta;
	vec3_t		up, down;

	VectorCopy (q2pml.origin, start_o);
	VectorCopy (q2pml.velocity, start_v);

	PMQ2_StepSlideMove_ ();

	VectorCopy (q2pml.origin, down_o);
	VectorCopy (q2pml.velocity, down_v);

	VectorCopy (start_o, up);
	up[2] += pm_q2stepheight;

	trace = q2pm->trace (up, q2pm->mins, q2pm->maxs, up);
	if (trace.allsolid)
		return;		// can't step up

	// try sliding above
	VectorCopy (up, q2pml.origin);
	VectorCopy (start_v, q2pml.velocity);

	PMQ2_StepSlideMove_ ();

	// push down the final amount
	VectorCopy (q2pml.origin, down);
	down[2] -= pm_q2stepheight;
	trace = q2pm->trace (q2pml.origin, q2pm->mins, q2pm->maxs, down);
	if (!trace.allsolid)
	{
		VectorCopy (trace.endpos, q2pml.origin);
	}

#if 0
	VectorSubtract (q2pml.origin, up, delta);
	up_dist = DotProduct (delta, start_v);

	VectorSubtract (down_o, start_o, delta);
	down_dist = DotProduct (delta, start_v);
#else
	VectorCopy(q2pml.origin, up);

	// decide which one went farther
    down_dist = (down_o[0] - start_o[0])*(down_o[0] - start_o[0])
        + (down_o[1] - start_o[1])*(down_o[1] - start_o[1]);
    up_dist = (up[0] - start_o[0])*(up[0] - start_o[0])
        + (up[1] - start_o[1])*(up[1] - start_o[1]);
#endif

	if (down_dist > up_dist || trace.plane.normal[2] < MIN_STEP_NORMAL)
	{
		VectorCopy (down_o, q2pml.origin);
		VectorCopy (down_v, q2pml.velocity);
		return;
	}
	//!! Special case
	// if we were walking along a plane, then we need to copy the Z over
	q2pml.velocity[2] = down_v[2];
}


/*
==================
PMQ2_Friction

Handles both ground friction and water friction
==================
*/
void PMQ2_Friction (void)
{
	float	*vel;
	float	speed, newspeed, control;
	float	friction;
	float	drop;
	
	vel = q2pml.velocity;
	
	speed = sqrt(vel[0]*vel[0] +vel[1]*vel[1] + vel[2]*vel[2]);
	if (speed < 1)
	{
		vel[0] = 0;
		vel[1] = 0;
		return;
	}

	drop = 0;

// apply ground friction
	if ((q2pm->groundentity && q2pml.groundsurface && !(q2pml.groundsurface->flags & TI_SLICK) ) || (q2pml.ladder) )
	{
		friction = pm_friction;
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control*friction*q2pml.frametime;
	}

// apply water friction
	if (q2pm->waterlevel && !q2pml.ladder)
		drop += speed*pm_waterfriction*q2pm->waterlevel*q2pml.frametime;

// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0)
	{
		newspeed = 0;
	}
	newspeed /= speed;

	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
}


/*
==============
PMQ2_Accelerate

Handles user intended acceleration
==============
*/
void PMQ2_Accelerate (vec3_t wishdir, float wishspeed, float accel)
{
	int			i;
	float		addspeed, accelspeed, currentspeed;

	currentspeed = DotProduct (q2pml.velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0)
		return;
	accelspeed = accel*q2pml.frametime*wishspeed;
	if (accelspeed > addspeed)
		accelspeed = addspeed;
	
	for (i=0 ; i<3 ; i++)
		q2pml.velocity[i] += accelspeed*wishdir[i];	
}

void PMQ2_AirAccelerate (vec3_t wishdir, float wishspeed, float accel)
{
	int			i;
	float		addspeed, accelspeed, currentspeed, wishspd = wishspeed;
		
	if (wishspd > 30)
		wishspd = 30;
	currentspeed = DotProduct (q2pml.velocity, wishdir);
	addspeed = wishspd - currentspeed;
	if (addspeed <= 0)
		return;
	accelspeed = accel * wishspeed * q2pml.frametime;
	if (accelspeed > addspeed)
		accelspeed = addspeed;
	
	for (i=0 ; i<3 ; i++)
		q2pml.velocity[i] += accelspeed*wishdir[i];	
}

/*
=============
PMQ2_AddCurrents
=============
*/
void PMQ2_AddCurrents (vec3_t	wishvel)
{
	vec3_t	v;
	float	s;

	//
	// account for ladders
	//

	if (q2pml.ladder && fabs(q2pml.velocity[2]) <= 200)
	{
		if ((q2pm->viewangles[PITCH] <= -15) && (q2pm->cmd.forwardmove > 0))
			wishvel[2] = 200;
		else if ((q2pm->viewangles[PITCH] >= 15) && (q2pm->cmd.forwardmove > 0))
			wishvel[2] = -200;
		else if (q2pm->cmd.upmove > 0)
			wishvel[2] = 200;
		else if (q2pm->cmd.upmove < 0)
			wishvel[2] = -200;
		else
			wishvel[2] = 0;

		// limit horizontal speed when on a ladder
		if (wishvel[0] < -25)
			wishvel[0] = -25;
		else if (wishvel[0] > 25)
			wishvel[0] = 25;

		if (wishvel[1] < -25)
			wishvel[1] = -25;
		else if (wishvel[1] > 25)
			wishvel[1] = 25;
	}


	//
	// add water currents
	//

	if (q2pm->watertype & Q2MASK_CURRENT) /*FIXME: q3bsp*/
	{
		memset(v, 0, sizeof(vec3_t));

		if (q2pm->watertype & Q2CONTENTS_CURRENT_0)
			v[0] += 1;
		if (q2pm->watertype & Q2CONTENTS_CURRENT_90)
			v[1] += 1;
		if (q2pm->watertype & Q2CONTENTS_CURRENT_180)
			v[0] -= 1;
		if (q2pm->watertype & Q2CONTENTS_CURRENT_270)
			v[1] -= 1;
		if (q2pm->watertype & Q2CONTENTS_CURRENT_UP)
			v[2] += 1;
		if (q2pm->watertype & Q2CONTENTS_CURRENT_DOWN)
			v[2] -= 1;

		s = pm_waterspeed;
		if ((q2pm->waterlevel == 1) && (q2pm->groundentity))
			s /= 2;

		VectorMA (wishvel, s, v, wishvel);
	}

	//
	// add conveyor belt velocities
	//

	if (q2pm->groundentity)
	{
		memset(v, 0, sizeof(vec3_t));

		if (q2pml.groundcontents & Q2CONTENTS_CURRENT_0)
			v[0] += 1;
		if (q2pml.groundcontents & Q2CONTENTS_CURRENT_90)
			v[1] += 1;
		if (q2pml.groundcontents & Q2CONTENTS_CURRENT_180)
			v[0] -= 1;
		if (q2pml.groundcontents & Q2CONTENTS_CURRENT_270)
			v[1] -= 1;
		if (q2pml.groundcontents & Q2CONTENTS_CURRENT_UP)
			v[2] += 1;
		if (q2pml.groundcontents & Q2CONTENTS_CURRENT_DOWN)
			v[2] -= 1;

		VectorMA (wishvel, 100 /* q2pm->groundentity->speed */, v, wishvel);
	}
}


/*
===================
PMQ2_WaterMove

===================
*/
void PMQ2_WaterMove (void)
{
	int		i;
	vec3_t	wishvel;
	float	wishspeed;
	vec3_t	wishdir;

//
// user intentions
//
	for (i=0 ; i<3 ; i++)
		wishvel[i] = q2pml.forward[i]*q2pm->cmd.forwardmove + q2pml.right[i]*q2pm->cmd.sidemove;

	if (!q2pm->cmd.forwardmove && !q2pm->cmd.sidemove && !q2pm->cmd.upmove)
		wishvel[2] -= 60;		// drift towards bottom
	else
		wishvel[2] += q2pm->cmd.upmove;

	PMQ2_AddCurrents (wishvel);

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	if (wishspeed > pm_maxspeed)
	{
		VectorScale (wishvel, pm_maxspeed/wishspeed, wishvel);
		wishspeed = pm_maxspeed;
	}
	wishspeed *= 0.5;

	PMQ2_Accelerate (wishdir, wishspeed, pm_wateraccelerate);

	PMQ2_StepSlideMove ();
}


/*
===================
PMQ2_AirMove

===================
*/
void PMQ2_AirMove (void)
{
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		maxspeed;

	fmove = q2pm->cmd.forwardmove;
	smove = q2pm->cmd.sidemove;
	
//!!!!! pitch should be 1/3 so this isn't needed??!
#if 0
	q2pml.forward[2] = 0;
	q2pml.right[2] = 0;
	VectorNormalize (q2pml.forward);
	VectorNormalize (q2pml.right);
#endif

	for (i=0 ; i<2 ; i++)
		wishvel[i] = q2pml.forward[i]*fmove + q2pml.right[i]*smove;
	wishvel[2] = 0;

	PMQ2_AddCurrents (wishvel);

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

//
// clamp to server defined max speed
//
	maxspeed = (q2pm->s.pm_flags & Q2PMF_DUCKED) ? pm_duckspeed : pm_maxspeed;

	if (wishspeed > maxspeed)
	{
		VectorScale (wishvel, maxspeed/wishspeed, wishvel);
		wishspeed = maxspeed;
	}
	
	if ( q2pml.ladder )
	{
		PMQ2_Accelerate (wishdir, wishspeed, pm_accelerate);
		if (!wishvel[2])
		{
			if (q2pml.velocity[2] > 0)
			{
				q2pml.velocity[2] -= q2pm->s.gravity * q2pml.frametime;
				if (q2pml.velocity[2] < 0)
					q2pml.velocity[2]  = 0;
			}
			else
			{
				q2pml.velocity[2] += q2pm->s.gravity * q2pml.frametime;
				if (q2pml.velocity[2] > 0)
					q2pml.velocity[2]  = 0;
			}
		}
		PMQ2_StepSlideMove ();
	}
	else if ( q2pm->groundentity )
	{	// walking on ground
		q2pml.velocity[2] = 0; //!!! this is before the accel
		PMQ2_Accelerate (wishdir, wishspeed, pm_accelerate);

// PGM	-- fix for negative trigger_gravity fields
//		q2pml.velocity[2] = 0;
		if(q2pm->s.gravity > 0)
			q2pml.velocity[2] = 0;
		else
			q2pml.velocity[2] -= q2pm->s.gravity * q2pml.frametime;
// PGM

		if (!q2pml.velocity[0] && !q2pml.velocity[1])
			return;
		PMQ2_StepSlideMove ();
	}
	else
	{	// not on ground, so little effect on velocity
		if (pm_airaccelerate)
			PMQ2_AirAccelerate (wishdir, wishspeed, pm_accelerate);
		else
			PMQ2_Accelerate (wishdir, wishspeed, 1);
		// add gravity
		q2pml.velocity[2] -= q2pm->s.gravity * q2pml.frametime;
		PMQ2_StepSlideMove ();
	}
}



/*
=============
PMQ2_CatagorizePosition
=============
*/
void PMQ2_CatagorizePosition (void)
{
	vec3_t		point;
	int			cont;
	q2trace_t		trace;
	int			sample1;
	int			sample2;

// if the player hull point one unit down is solid, the player
// is on ground

// see if standing on something solid	
	point[0] = q2pml.origin[0];
	point[1] = q2pml.origin[1];
	point[2] = q2pml.origin[2] - 0.25;
	if (q2pml.velocity[2] > 180) //!!ZOID changed from 100 to 180 (ramp accel)
	{
		q2pm->s.pm_flags &= ~Q2PMF_ON_GROUND;
		q2pm->groundentity = NULL;
	}
	else
	{
		trace = q2pm->trace (q2pml.origin, q2pm->mins, q2pm->maxs, point);
		q2pml.groundplane = trace.plane;
		q2pml.groundsurface = trace.surface;
		q2pml.groundcontents = trace.contents;

		if (!trace.ent || (trace.plane.normal[2] < 0.7 && !trace.startsolid) )
		{
			q2pm->groundentity = NULL;
			q2pm->s.pm_flags &= ~Q2PMF_ON_GROUND;
		}
		else
		{
			q2pm->groundentity = trace.ent;

			// hitting solid ground will end a waterjump
			if (q2pm->s.pm_flags & Q2PMF_TIME_WATERJUMP)
			{
				q2pm->s.pm_flags &= ~(Q2PMF_TIME_WATERJUMP | Q2PMF_TIME_LAND | Q2PMF_TIME_TELEPORT);
				q2pm->s.pm_time = 0;
			}

			if (! (q2pm->s.pm_flags & Q2PMF_ON_GROUND) )
			{	// just hit the ground
				q2pm->s.pm_flags |= Q2PMF_ON_GROUND;
				// don't do landing time if we were just going down a slope
				if (q2pml.velocity[2] < -200)
				{
					q2pm->s.pm_flags |= Q2PMF_TIME_LAND;
					// don't allow another jump for a little while
					if (q2pml.velocity[2] < -400)
						q2pm->s.pm_time = 25;	
					else
						q2pm->s.pm_time = 18;
				}
			}
		}

#if 0
		if (trace.fraction < 1.0 && trace.ent && q2pml.velocity[2] < 0)
			q2pml.velocity[2] = 0;
#endif

		if (q2pm->numtouch < MAXTOUCH && trace.ent)
		{
			q2pm->touchents[q2pm->numtouch] = trace.ent;
			q2pm->numtouch++;
		}
	}

//
// get waterlevel, accounting for ducking
//
	q2pm->waterlevel = 0;
	q2pm->watertype = 0;

	sample2 = q2pm->viewheight - q2pm->mins[2];
	sample1 = sample2 / 2;

	point[2] = q2pml.origin[2] + q2pm->mins[2] + 1;	
	cont = q2pm->pointcontents (point);

	if (cont & MASK_WATER)
	{
		q2pm->watertype = cont;
		q2pm->waterlevel = 1;
		point[2] = q2pml.origin[2] + q2pm->mins[2] + sample1;
		cont = q2pm->pointcontents (point);
		if (cont & MASK_WATER)
		{
			q2pm->waterlevel = 2;
			point[2] = q2pml.origin[2] + q2pm->mins[2] + sample2;
			cont = q2pm->pointcontents (point);
			if (cont & MASK_WATER)
				q2pm->waterlevel = 3;
		}
	}

}


/*
=============
PMQ2_CheckJump
=============
*/
void PMQ2_CheckJump (void)
{
	if (q2pm->s.pm_flags & Q2PMF_TIME_LAND)
	{	// hasn't been long enough since landing to jump again
		return;
	}

	if (q2pm->cmd.upmove < 10)
	{	// not holding jump
		q2pm->s.pm_flags &= ~Q2PMF_JUMP_HELD;
		return;
	}

	// must wait for jump to be released
	if (q2pm->s.pm_flags & Q2PMF_JUMP_HELD)
		return;

	if (q2pm->s.pm_type == Q2PM_DEAD)
		return;

	if (q2pm->waterlevel >= 2)
	{	// swimming, not jumping
		q2pm->groundentity = NULL;

		if (q2pml.velocity[2] <= -300)
			return;

		if (q2pm->watertype == Q2CONTENTS_WATER)
			q2pml.velocity[2] = 100;
		else if (q2pm->watertype == Q2CONTENTS_SLIME)
			q2pml.velocity[2] = 80;
		else
			q2pml.velocity[2] = 50;
		return;
	}

	if (q2pm->groundentity == NULL)
		return;		// in air, so no effect

	q2pm->s.pm_flags |= Q2PMF_JUMP_HELD;

	q2pm->groundentity = NULL;
	q2pml.velocity[2] += 270;
	if (q2pml.velocity[2] < 270)
		q2pml.velocity[2] = 270;
}


/*
=============
PMQ2_CheckSpecialMovement
=============
*/
void PMQ2_CheckSpecialMovement (void)
{
	vec3_t	spot;
	int		cont;
	vec3_t	flatforward;
	q2trace_t	trace;

	if (q2pm->s.pm_time)
		return;

	q2pml.ladder = false;

	// check for ladder
	flatforward[0] = q2pml.forward[0];
	flatforward[1] = q2pml.forward[1];
	flatforward[2] = 0;
	VectorNormalize (flatforward);

	VectorMA (q2pml.origin, 1, flatforward, spot);
	trace = q2pm->trace (q2pml.origin, q2pm->mins, q2pm->maxs, spot);
	if ((trace.fraction < 1) && (trace.contents & Q2CONTENTS_LADDER))
		q2pml.ladder = true;

	// check for water jump
	if (q2pm->waterlevel != 2)
		return;

	VectorMA (q2pml.origin, 30, flatforward, spot);
	spot[2] += 4;
	cont = q2pm->pointcontents (spot);
	if (!(cont & Q2CONTENTS_SOLID))
		return;

	spot[2] += 16;
	cont = q2pm->pointcontents (spot);
	if (cont)
		return;
	// jump out of water
	VectorScale (flatforward, 50, q2pml.velocity);
	q2pml.velocity[2] = 350;

	q2pm->s.pm_flags |= Q2PMF_TIME_WATERJUMP;
	q2pm->s.pm_time = 255;
}


/*
===============
PMQ2_FlyMove
===============
*/
void PMQ2_FlyMove (qboolean doclip)
{
	float	speed, drop, friction, control, newspeed;
	float	currentspeed, addspeed, accelspeed;
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	vec3_t		end;
	q2trace_t	trace;

	q2pm->viewheight = DEFAULT_VIEWHEIGHT;

	// friction

	speed = Length (q2pml.velocity);
	if (speed < 1)
	{
		VectorClear (q2pml.velocity);
	}
	else
	{
		drop = 0;

		friction = pm_friction*1.5;	// extra friction
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control*friction*q2pml.frametime;

		// scale the velocity
		newspeed = speed - drop;
		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;

		VectorScale (q2pml.velocity, newspeed, q2pml.velocity);
	}

	// accelerate
	fmove = q2pm->cmd.forwardmove;
	smove = q2pm->cmd.sidemove;
	
	VectorNormalize (q2pml.forward);
	VectorNormalize (q2pml.right);

	for (i=0 ; i<3 ; i++)
		wishvel[i] = q2pml.forward[i]*fmove + q2pml.right[i]*smove;
	wishvel[2] += q2pm->cmd.upmove;

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	//
	// clamp to server defined max speed
	//
	if (wishspeed > pm_maxspeed)
	{
		VectorScale (wishvel, pm_maxspeed/wishspeed, wishvel);
		wishspeed = pm_maxspeed;
	}


	currentspeed = DotProduct(q2pml.velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0)
		return;
	accelspeed = pm_accelerate*q2pml.frametime*wishspeed;
	if (accelspeed > addspeed)
		accelspeed = addspeed;
	
	for (i=0 ; i<3 ; i++)
		q2pml.velocity[i] += accelspeed*wishdir[i];	

	if (doclip) {
		for (i=0 ; i<3 ; i++)
			end[i] = q2pml.origin[i] + q2pml.frametime * q2pml.velocity[i];

		trace = q2pm->trace (q2pml.origin, q2pm->mins, q2pm->maxs, end);

		VectorCopy (trace.endpos, q2pml.origin);
	} else {
		// move
		VectorMA (q2pml.origin, q2pml.frametime, q2pml.velocity, q2pml.origin);
	}
}


/*
==============
PMQ2_CheckDuck

Sets mins, maxs, and q2pm->viewheight
==============
*/
void PMQ2_CheckDuck (void)
{
	q2trace_t	trace;

	q2pm->mins[0] = -16;
	q2pm->mins[1] = -16;

	q2pm->maxs[0] = 16;
	q2pm->maxs[1] = 16;

	if (q2pm->s.pm_type == Q2PM_GIB)
	{
		q2pm->mins[2] = 0;
		q2pm->maxs[2] = 16;
		q2pm->viewheight = 8;
		return;
	}

	q2pm->mins[2] = -24;

	if (q2pm->s.pm_type == Q2PM_DEAD)
	{
		q2pm->s.pm_flags |= Q2PMF_DUCKED;
	}
	else if (q2pm->cmd.upmove < 0 && (q2pm->s.pm_flags & Q2PMF_ON_GROUND) )
	{	// duck
		q2pm->s.pm_flags |= Q2PMF_DUCKED;
	}
	else
	{	// stand up if possible
		if (q2pm->s.pm_flags & Q2PMF_DUCKED)
		{
			// try to stand up
			q2pm->maxs[2] = 32;
			trace = q2pm->trace (q2pml.origin, q2pm->mins, q2pm->maxs, q2pml.origin);
			if (!trace.allsolid)
				q2pm->s.pm_flags &= ~Q2PMF_DUCKED;
		}
	}

	if (q2pm->s.pm_flags & Q2PMF_DUCKED)
	{
		q2pm->maxs[2] = 4;
		q2pm->viewheight = -2;
	}
	else
	{
		q2pm->maxs[2] = 32;
		q2pm->viewheight = DEFAULT_VIEWHEIGHT;
	}
}


/*
==============
PMQ2_DeadMove
==============
*/
void PMQ2_DeadMove (void)
{
	float	forward;

	if (!q2pm->groundentity)
		return;

	// extra friction

	forward = Length (q2pml.velocity);
	forward -= 20;
	if (forward <= 0)
	{
		memset(q2pml.velocity, 0, sizeof(vec3_t));
	}
	else
	{
		VectorNormalize (q2pml.velocity);
		VectorScale (q2pml.velocity, forward, q2pml.velocity);
	}
}


qboolean	PMQ2_GoodPosition (void)
{
	q2trace_t	trace;
	vec3_t	origin, end;
	int		i;

	if (q2pm->s.pm_type == Q2PM_SPECTATOR)
		return true;

	for (i=0 ; i<3 ; i++)
		origin[i] = end[i] = q2pm->s.origin[i]*0.125;
	trace = q2pm->trace (origin, q2pm->mins, q2pm->maxs, end);

	return !trace.allsolid;
}

/*
================
PMQ2_SnapPosition

On exit, the origin will have a value that is pre-quantized to the 0.125
precision of the network channel and in a valid position.
================
*/
void PMQ2_SnapPosition (void)
{
	int		sign[3];
	int		i, j, bits;
	short	base[3];
	// try all single bits first
	static int jitterbits[8] = {0,4,1,2,3,5,6,7};

	// snap velocity to eigths
	for (i=0 ; i<3 ; i++)
		q2pm->s.velocity[i] = (int)(q2pml.velocity[i]*8);

	for (i=0 ; i<3 ; i++)
	{
		if (q2pml.origin[i] >= 0)
			sign[i] = 1;
		else 
			sign[i] = -1;
		q2pm->s.origin[i] = (int)(q2pml.origin[i]*8);
		if (q2pm->s.origin[i]*0.125 == q2pml.origin[i])
			sign[i] = 0;
	}
	VectorCopy (q2pm->s.origin, base);

	// try all combinations
	for (j=0 ; j<8 ; j++)
	{
		bits = jitterbits[j];
		VectorCopy (base, q2pm->s.origin);
		for (i=0 ; i<3 ; i++)
			if (bits & (1<<i) )
				q2pm->s.origin[i] += sign[i];

		if (PMQ2_GoodPosition ())
			return;
	}

	// go back to the last position
	VectorCopy (q2pml.previous_origin, q2pm->s.origin);
//	Con_DPrintf ("using previous_origin\n");
}

#if 0
//NO LONGER USED
/*
================
PMQ2_InitialSnapPosition

================
*/
void PMQ2_InitialSnapPosition (void)
{
	int		x, y, z;
	short	base[3];

	VectorCopy (q2pm->s.origin, base);

	for (z=1 ; z>=-1 ; z--)
	{
		q2pm->s.origin[2] = base[2] + z;
		for (y=1 ; y>=-1 ; y--)
		{
			q2pm->s.origin[1] = base[1] + y;
			for (x=1 ; x>=-1 ; x--)
			{
				q2pm->s.origin[0] = base[0] + x;
				if (PMQ2_GoodPosition ())
				{
					q2pml.origin[0] = q2pm->s.origin[0]*0.125;
					q2pml.origin[1] = q2pm->s.origin[1]*0.125;
					q2pml.origin[2] = q2pm->s.origin[2]*0.125;
					VectorCopy (q2pm->s.origin, q2pml.previous_origin);
					return;
				}
			}
		}
	}

	Con_DPrintf ("Bad InitialSnapPosition\n");
}
#else
/*
================
PMQ2_InitialSnapPosition

================
*/
void PMQ2_InitialSnapPosition(void)
{
	int        x, y, z;
	short      base[3];
	static int offset[3] = { 0, -1, 1 };

	VectorCopy (q2pm->s.origin, base);

	for ( z = 0; z < 3; z++ ) {
		q2pm->s.origin[2] = base[2] + offset[ z ];
		for ( y = 0; y < 3; y++ ) {
			q2pm->s.origin[1] = base[1] + offset[ y ];
			for ( x = 0; x < 3; x++ ) {
				q2pm->s.origin[0] = base[0] + offset[ x ];
				if (PMQ2_GoodPosition ()) {
					q2pml.origin[0] = q2pm->s.origin[0]*0.125;
					q2pml.origin[1] = q2pm->s.origin[1]*0.125;
					q2pml.origin[2] = q2pm->s.origin[2]*0.125;
					VectorCopy (q2pm->s.origin, q2pml.previous_origin);
					return;
				}
			}
		}
	}

	Con_DPrintf ("Bad InitialSnapPosition\n");
}

#endif

/*
================
PMQ2_ClampAngles

================
*/
void PMQ2_ClampAngles (void)
{
	short	temp;
	int		i;

	if (q2pm->s.pm_flags & Q2PMF_TIME_TELEPORT)
	{
		q2pm->viewangles[YAW] = SHORT2ANGLE(q2pm->cmd.angles[YAW] + q2pm->s.delta_angles[YAW]);
		q2pm->viewangles[PITCH] = 0;
		q2pm->viewangles[ROLL] = 0;
	}
	else
	{
		// circularly clamp the angles with deltas
		for (i=0 ; i<3 ; i++)
		{
			temp = q2pm->cmd.angles[i] + q2pm->s.delta_angles[i];
			q2pm->viewangles[i] = SHORT2ANGLE(temp);
		}

		// don't let the player look up or down more than 90 degrees
		if (q2pm->viewangles[PITCH] > 89 && q2pm->viewangles[PITCH] < 180)
			q2pm->viewangles[PITCH] = 89;
		else if (q2pm->viewangles[PITCH] < 271 && q2pm->viewangles[PITCH] >= 180)
			q2pm->viewangles[PITCH] = 271;
	}
	AngleVectors (q2pm->viewangles, q2pml.forward, q2pml.right, q2pml.up);
}

/*
================
Pmove

Can be called by either the server or the client
================
*/
void VARGS Q2_Pmove (q2pmove_t *pmove)
{
	q2pm = pmove;

	// clear results
	q2pm->numtouch = 0;
	memset (q2pm->viewangles, 0, sizeof(vec3_t));
	q2pm->viewheight = 0;
	q2pm->groundentity = 0;
	q2pm->watertype = 0;
	q2pm->waterlevel = 0;

	// clear all pmove local vars
	memset (&q2pml, 0, sizeof(q2pml));

	// convert origin and velocity to float values
	q2pml.origin[0] = q2pm->s.origin[0]*0.125;
	q2pml.origin[1] = q2pm->s.origin[1]*0.125;
	q2pml.origin[2] = q2pm->s.origin[2]*0.125;

	q2pml.velocity[0] = q2pm->s.velocity[0]*0.125;
	q2pml.velocity[1] = q2pm->s.velocity[1]*0.125;
	q2pml.velocity[2] = q2pm->s.velocity[2]*0.125;

	// save old org in case we get stuck
	VectorCopy (q2pm->s.origin, q2pml.previous_origin);

	q2pml.frametime = q2pm->cmd.msec * 0.001;

	PMQ2_ClampAngles ();

	if (q2pm->s.pm_type == Q2PM_SPECTATOR)
	{
		PMQ2_FlyMove (false);
		PMQ2_SnapPosition ();
		return;
	}

	if (q2pm->s.pm_type >= Q2PM_DEAD)
	{
		q2pm->cmd.forwardmove = 0;
		q2pm->cmd.sidemove = 0;
		q2pm->cmd.upmove = 0;
	}

	if (q2pm->s.pm_type == Q2PM_FREEZE)
		return;		// no movement at all

	// set mins, maxs, and viewheight
	PMQ2_CheckDuck ();

	if (q2pm->snapinitial)
		PMQ2_InitialSnapPosition ();

	// set groundentity, watertype, and waterlevel
	PMQ2_CatagorizePosition ();

	if (q2pm->s.pm_type == Q2PM_DEAD)
		PMQ2_DeadMove ();

	PMQ2_CheckSpecialMovement ();

	// drop timing counter
	if (q2pm->s.pm_time)
	{
		int		msec;

		msec = q2pm->cmd.msec >> 3;
		if (!msec)
			msec = 1;
		if ( msec >= q2pm->s.pm_time) 
		{
			q2pm->s.pm_flags &= ~(Q2PMF_TIME_WATERJUMP | Q2PMF_TIME_LAND | Q2PMF_TIME_TELEPORT);
			q2pm->s.pm_time = 0;
		}
		else
			q2pm->s.pm_time -= msec;
	}

	if (q2pm->s.pm_flags & Q2PMF_TIME_TELEPORT)
	{	// teleport pause stays exactly in place
	}
	else if (q2pm->s.pm_flags & Q2PMF_TIME_WATERJUMP)
	{	// waterjump has no control, but falls
		q2pml.velocity[2] -= q2pm->s.gravity * q2pml.frametime;
		if (q2pml.velocity[2] < 0)
		{	// cancel as soon as we are falling down again
			q2pm->s.pm_flags &= ~(Q2PMF_TIME_WATERJUMP | Q2PMF_TIME_LAND | Q2PMF_TIME_TELEPORT);
			q2pm->s.pm_time = 0;
		}

		PMQ2_StepSlideMove ();
	}
	else
	{
		PMQ2_CheckJump ();

		PMQ2_Friction ();

		if (q2pm->waterlevel >= 2)
			PMQ2_WaterMove ();
		else {
			vec3_t	angles;

			VectorCopy(q2pm->viewangles, angles);
			if (angles[PITCH] > 180)
				angles[PITCH] = angles[PITCH] - 360;
			angles[PITCH] /= 3;

			AngleVectors (angles, q2pml.forward, q2pml.right, q2pml.up);

			PMQ2_AirMove ();
		}
	}

	// set groundentity, watertype, and waterlevel for final spot
	PMQ2_CatagorizePosition ();

	PMQ2_SnapPosition ();
}


#endif

