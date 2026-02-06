/*
Copyright (C) 1996-1997 Id Software, Inc.

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

movevars_t		movevars;
playermove_t	pmove;
extern cvar_t	pm_noround;	//evile.

#define movevars_dpflags		MOVEFLAG_QWCOMPAT
#define movevars_maxairspeed	30
#define movevars_jumpspeed		270

float		frametime;

vec3_t		forward, right, up;

void PM_Init (void)
{
	PM_InitBoxHull();
}

#define	MIN_STEP_NORMAL	0.7		// roughly 45 degrees

#define	STOP_EPSILON	0.1
#define BLOCKED_FLOOR	1
#define BLOCKED_STEP	2
#define BLOCKED_OTHER	4
#define BLOCKED_ANY		7

/*
** Add an entity to touch list, discarding duplicates
*/
static void PM_AddTouchedEnt (int num)
{
	if (pmove.numtouch == MAX_PHYSENTS)
		return;

	if (pmove.numtouch)
		if (pmove.touchindex[pmove.numtouch - 1] == num)
			return; // already added

	pmove.touchindex[pmove.numtouch] = num;
	VectorCopy(pmove.velocity, pmove.touchvel[pmove.numtouch]);
	pmove.numtouch++;
}


/*
==================
PM_ClipVelocity

Slide off of the impacting object
==================
*/

void PM_ClipVelocity (vec3_t in, vec3_t normal, vec3_t out, float overbounce)
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

#include "pr_common.h"
static qboolean PM_PortalTransform(world_t *w, int portalnum, vec3_t org, vec3_t move, vec3_t newang, vec3_t newvel)
{
	vec3_t rounded;
	qboolean okay = true;
	wedict_t *portal = WEDICT_NUM_UB(w->progs, portalnum);
	int oself = *w->g.self;
	void *pr_globals = PR_globals(w->progs, PR_CURRENT);
	int i;
	int tmp;
	float f;

	*w->g.self = EDICT_TO_PROG(w->progs, portal);
	//transform origin+velocity etc
	VectorCopy(org, G_VECTOR(OFS_PARM0));
	VectorCopy(pmove.angles, G_VECTOR(OFS_PARM1));
	VectorCopy(pmove.velocity, w->g.v_forward);
	VectorCopy(move, w->g.v_right);
	VectorCopy(pmove.gravitydir, w->g.v_up);
	if (!DotProduct(w->g.v_up, w->g.v_up))
		w->g.v_up[2] = -1;

	PR_ExecuteProgram (w->progs, portal->xv->camera_transform);

	for (i = 0; i < 3; i++)
	{
		tmp = floor(G_VECTOR(OFS_RETURN)[i]*8 + 0.5);
		rounded[i] = tmp/8.0;
	}
	//make sure the new origin is okay for the player. back out if its invalid.
	if (!PM_TestPlayerPosition(rounded, true))
		okay = false;
	else
	{
		VectorCopy(rounded, org);
		VectorCopy(w->g.v_forward, newvel);
		VectorCopy(w->g.v_right, move);
//		VectorCopy(w->g.v_up, pmove.gravitydir);

		//floor+floor, ish
		if (DotProduct(w->g.v_up, pmove.gravitydir) < 0.7)
		{
			f = DotProduct(newvel, newvel);
			if (f < 200*200)
			{
				VectorScale(newvel, 200 / sqrt(f), newvel);
			}
		}


		//transform the angles too
		VectorCopy(org, G_VECTOR(OFS_PARM0));
		VectorCopy(pmove.angles, G_VECTOR(OFS_PARM1));
		AngleVectors(pmove.angles, w->g.v_forward, w->g.v_right, w->g.v_up);
		PR_ExecuteProgram (w->progs, portal->xv->camera_transform);
		VectorAngles(w->g.v_forward, w->g.v_up, newang, false);
	}

	*w->g.self = oself;
	return okay;
}

static trace_t	PM_PlayerTracePortals(vec3_t start, vec3_t end, unsigned int solidmask, float *tookportal)
{
	trace_t trace = PM_PlayerTrace (start, end, MASK_PLAYERSOLID);
	if (tookportal)
		*tookportal = 0;
	if (trace.entnum >= 0 && pmove.world)
	{
		physent_t *impact = &pmove.physents[trace.entnum];
		if (impact->isportal)
		{
			vec3_t move;
			vec3_t from;
			vec3_t newang, newvel = {0,0,0};

			VectorCopy(trace.endpos, from);	//just in case
			VectorSubtract(end, trace.endpos, move);
			if (PM_PortalTransform(pmove.world, impact->info, from, move, newang, newvel))
			{
				trace_t exit;
				int i, tmp;
				VectorAdd(from, move, end);
				
				//if we follow the portal, then we basically need to restart from the other side.
				exit = PM_PlayerTrace (from, end, MASK_PLAYERSOLID);

				for (i = 0; i < 3; i++)
				{
					tmp = floor(exit.endpos[i]*8 + 0.5);
					exit.endpos[i] = tmp/8.0;
				}
				if (PM_TestPlayerPosition(exit.endpos, false))
				{
					if (tookportal)
						*tookportal = trace.fraction;
					VectorCopy(newang, pmove.angles);
					VectorCopy(newvel, pmove.velocity);
					return exit;
				}
			}
		}
	}
	return trace;
}

/*
============
PM_SlideMove

The basic solid body movement clip that slides along multiple planes
============
*/
#define	MAX_CLIP_PLANES	5

int PM_SlideMove (void)
{
	int			bumpcount, numbumps;
	vec3_t		dir;
	float		d;
	int			numplanes;
	vec3_t		planes[MAX_CLIP_PLANES];
	vec3_t		primal_velocity, original_velocity;
	int			i, j;
	trace_t		trace;
	vec3_t		end;
	float		time_left;
	int			blocked;
	float		tookportal;
	vec3_t		start;

	numbumps = 4;

	blocked = 0;
	VectorCopy (pmove.velocity, original_velocity);
	VectorCopy (pmove.velocity, primal_velocity);
	numplanes = 0;

	time_left = frametime;

//	VectorAdd(pmove.velocity, pmove.basevelocity, pmove.velocity);

	for (bumpcount=0 ; bumpcount<numbumps ; bumpcount++)
	{
		for (i=0 ; i<3 ; i++)
			end[i] = pmove.origin[i] + time_left * pmove.velocity[i];

		VectorCopy(pmove.origin, start);
		trace = PM_PlayerTracePortals (start, end, MASK_PLAYERSOLID, &tookportal);
		if (tookportal)
		{
			//made progress, but hit a portal
			time_left -= time_left * tookportal;
			VectorCopy (pmove.velocity, primal_velocity);
			VectorCopy (pmove.velocity, original_velocity);
			numplanes = 0;
		}


		if (trace.startsolid || trace.allsolid)
		{	// entity is trapped in another solid
			VectorClear (pmove.velocity);
			return 3;
		}

		if (trace.fraction > 0)
		{	// actually covered some distance
			VectorCopy (trace.endpos, pmove.origin);
			numplanes = 0;
		}

		if (trace.fraction == 1)
			 break;		// moved the entire distance

		// save entity for contact
		PM_AddTouchedEnt (trace.entnum);

		if (trace.plane.normal[2] >= MIN_STEP_NORMAL)
			blocked |= BLOCKED_FLOOR;
		else if (!trace.plane.normal[2])
			blocked |= BLOCKED_STEP;
		else
			blocked |= BLOCKED_OTHER;

		time_left -= time_left * trace.fraction;

	// cliped to another plane
		if (numplanes >= MAX_CLIP_PLANES)
		{	// this shouldn't really happen
			VectorClear (pmove.velocity);
			break;
		}

		VectorCopy (trace.plane.normal, planes[numplanes]);
		numplanes++;

//
// modify original_velocity so it parallels all of the clip planes
//
		for (i=0 ; i<numplanes ; i++)
		{
			if (movevars.walljump == 2)	//just bounce off!
			{	//pinball
				PM_ClipVelocity (original_velocity, planes[i], pmove.velocity, 2);
				return blocked;
			}
			//regular run at a wall and jump off
			if (movevars.walljump && planes[i][2] != 1	//not on floors
				&& Length(pmove.velocity)>200 && pmove.cmd.buttons & 2 && !pmove.jump_held && !pmove.waterjumptime)
			{
				PM_ClipVelocity (original_velocity, planes[i], pmove.velocity, 2);
				if (pmove.velocity[2] < movevars_jumpspeed)
					pmove.velocity[2] = movevars_jumpspeed;
				pmove.jump_msec = pmove.cmd.msec;
				pmove.jump_held = true;
				pmove.waterjumptime = 0;
				return blocked;
			}
			PM_ClipVelocity (original_velocity, planes[i], pmove.velocity, 1);
			for (j=0 ; j<numplanes ; j++)
				if (j != i)
				{
					if (DotProduct (pmove.velocity, planes[j]) < 0)
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
				VectorClear (pmove.velocity);
				break;
			}
			CrossProduct (planes[0], planes[1], dir);
			d = DotProduct (dir, pmove.velocity);
			VectorScale (dir, d, pmove.velocity);
		}

//
// if velocity is against the original velocity, stop dead
// to avoid tiny occilations in sloping corners
//
		if (DotProduct (pmove.velocity, primal_velocity) <= 0)
		{
			VectorClear (pmove.velocity);
			break;
		}
	}

	if (pmove.waterjumptime)
	{
		VectorCopy (primal_velocity, pmove.velocity);
	}
	return blocked;
}

/*
=============
PM_StepSlideMove

Each intersection will try to step over the obstruction instead of
sliding along it.
=============
*/
int PM_StepSlideMove (qboolean in_air)
{
	vec3_t	dest;
	trace_t	trace;
	vec3_t	original, originalvel, down, up, downvel;
	float	downdist, updist;
	int		blocked;
	float	stepsize;

	// try sliding forward both on ground and up 16 pixels
	// take the move that goes farthest
	VectorCopy (pmove.origin, original);
	VectorCopy (pmove.velocity, originalvel);

	blocked = PM_SlideMove ();

	if (!blocked)
	{
		if (!in_air && movevars.stepdown)
		{	//if we were onground, try stepping down after the move to try to stay on said ground.
			VectorMA (pmove.origin, movevars.stepheight, pmove.gravitydir, dest);
			trace = PM_PlayerTracePortals (pmove.origin, dest, MASK_PLAYERSOLID, NULL);
			if (trace.fraction != 1 && -DotProduct(pmove.gravitydir, trace.plane.normal) > MIN_STEP_NORMAL)
			{
				if (!trace.startsolid && !trace.allsolid)
					VectorCopy (trace.endpos, pmove.origin);
			}
		}

		return blocked;		// moved the entire distance
	}

	if (in_air)
	{
		// don't let us step up unless it's indeed a step we bumped in
		// (that is, there's solid ground below)
		float *org;

		if (!(blocked & BLOCKED_STEP))
			return blocked;

		org = (-DotProduct(pmove.gravitydir, originalvel) < 0) ? pmove.origin : original;
		VectorMA (org, movevars.stepheight, pmove.gravitydir, dest);
		trace = PM_PlayerTrace (org, dest, MASK_PLAYERSOLID);
		if (trace.fraction == 1 || -DotProduct(pmove.gravitydir, trace.plane.normal) < MIN_STEP_NORMAL)
			return blocked;

		// adjust stepsize, otherwise it would be possible to walk up a
		// a step higher than STEPSIZE
		//FIXME gravitydir, portals
		stepsize = movevars.stepheight - (org[2] - trace.endpos[2]);
	}
	else
		stepsize = movevars.stepheight;

	VectorCopy (pmove.origin, down);
	VectorCopy (pmove.velocity, downvel);

	VectorCopy (original, pmove.origin);
	VectorCopy (originalvel, pmove.velocity);

// move up a stair height
	VectorMA (pmove.origin, -stepsize, pmove.gravitydir, dest);
	trace = PM_PlayerTracePortals (pmove.origin, dest, MASK_PLAYERSOLID, NULL);
	if (!trace.startsolid && !trace.allsolid)
	{
		VectorCopy (trace.endpos, pmove.origin);
	}

	if (in_air && -DotProduct(pmove.gravitydir, original) < 0)
		VectorMA(pmove.velocity, -DotProduct(pmove.velocity, pmove.gravitydir), pmove.gravitydir, pmove.velocity); //z=0

	PM_SlideMove ();

// press down the stepheight
	VectorMA (pmove.origin, stepsize, pmove.gravitydir, dest);
	trace = PM_PlayerTracePortals (pmove.origin, dest, MASK_PLAYERSOLID, NULL);
	if (trace.fraction != 1 && -DotProduct(pmove.gravitydir, trace.plane.normal) < MIN_STEP_NORMAL)
		goto usedown;
	if (!trace.startsolid && !trace.allsolid)
	{
		VectorCopy (trace.endpos, pmove.origin);
	}

	if (-DotProduct(pmove.gravitydir, pmove.origin) < -DotProduct(pmove.gravitydir, original))
		goto usedown;

	VectorCopy (pmove.origin, up);

	// decide which one went farther (in the forwards direction regardless of step values)
	VectorSubtract(down, original, dest);
	VectorMA(dest, -DotProduct(dest, pmove.gravitydir), pmove.gravitydir, dest); //z=0
	downdist = DotProduct(dest, dest);
	VectorSubtract(up, original, dest);
	VectorMA(dest, -DotProduct(dest, pmove.gravitydir), pmove.gravitydir, dest); //z=0
	updist = DotProduct(dest, dest);

	if (downdist >= updist)
	{
usedown:
		VectorCopy (down, pmove.origin);
		VectorCopy (downvel, pmove.velocity);
		return blocked;
	}

	// copy z value from slide move
	VectorMA(pmove.velocity, DotProduct(downvel, pmove.gravitydir)-DotProduct(pmove.velocity, pmove.gravitydir), pmove.gravitydir, pmove.velocity); //z=downvel

	if (!pmove.onground && pmove.waterlevel < 2 && (blocked & BLOCKED_STEP)) {
		float scale;
		// in pm_airstep mode, walking up a 16 unit high step
		// will kill 16% of horizontal velocity
		scale = 1 - 0.01*(pmove.origin[2] - original[2]);
		//FIXME gravitydir
		pmove.velocity[0] *= scale;
		pmove.velocity[1] *= scale;
	}

	return blocked;
}



/*
==================
PM_Friction

Handles both ground friction and water friction
==================
*/
void PM_Friction (void)
{
	float	speed, newspeed, control;
	float	friction;
	float	drop;
	vec3_t	start, stop;
	trace_t	trace;

	if (pmove.waterjumptime)
		return;

	speed = Length(pmove.velocity);
	if (speed < 1)
	{
//fixme: gravitydir fix needed
		pmove.velocity[0] = 0;
		pmove.velocity[1] = 0;
		if (pmove.pm_type == PM_FLY || pmove.pm_type == PM_6DOF)
			pmove.velocity[2] = 0;
		return;
	}

	if (pmove.waterlevel >= 2)
		// apply water friction, even if in fly mode
		drop = speed*movevars.waterfriction*pmove.waterlevel*frametime;
	else if (pmove.pm_type == PM_FLY || pmove.pm_type == PM_6DOF) {
		// apply flymode friction
		drop = speed * movevars.flyfriction * frametime;
	}
	else if (pmove.onground) {
		// apply ground friction
		friction = movevars.friction;
		if (movevars.edgefriction != 1.0)
		{
			// if the leading edge is over a dropoff, increase friction
			start[0] = stop[0] = pmove.origin[0] + pmove.velocity[0]/speed*16;
			start[1] = stop[1] = pmove.origin[1] + pmove.velocity[1]/speed*16;
			//FIXME: gravitydir.
			//id quirk: this is a tracebox, NOT a traceline, yet still starts BELOW the player.
			start[2] = pmove.origin[2] + pmove.player_mins[2];
			stop[2] = start[2] - 34;
			if (movevars.flags & MOVEFLAG_QWEDGEBOX)	//vanilla qw behaviour is to use a tracebox, which makes edge friction almost unnoticable.
				trace = PM_PlayerTrace (start, stop, MASK_PLAYERSOLID);
			else
			{	//traceline instead.
				vec3_t min, max;
				VectorCopy(pmove.player_mins, min);
				VectorCopy(pmove.player_maxs, max);
				VectorClear(pmove.player_mins);
				VectorClear(pmove.player_maxs);
				trace = PM_PlayerTrace (start, stop, MASK_PLAYERSOLID);
				VectorCopy(min, pmove.player_mins);
				VectorCopy(max, pmove.player_maxs);
			}
			if (trace.fraction == 1 && !trace.startsolid)
				friction *= movevars.edgefriction;
		}
		control = speed < movevars.stopspeed ? movevars.stopspeed : speed;
		drop = control*friction*frametime;
	}
	else if (pmove.onladder)
	{
		control = speed < movevars.stopspeed ? movevars.stopspeed : speed;
		drop = control*movevars.friction*frametime*6;
	}
	else
		return;		// in air, no friction

// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0)
		newspeed = 0;

	VectorScale (pmove.velocity, newspeed / speed, pmove.velocity);
}


/*
==============
PM_Accelerate
==============
*/
void PM_Accelerate (vec3_t wishdir, float wishspeed, float accel)
{
	int			i;
	float		addspeed, accelspeed, currentspeed;

	if (pmove.pm_type == PM_DEAD)
		return;
	if (pmove.waterjumptime)
		return;

	currentspeed = DotProduct (pmove.velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0)
		return;
	accelspeed = accel*frametime*wishspeed;
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	for (i=0 ; i<3 ; i++)
		pmove.velocity[i] += accelspeed*wishdir[i];
}

void PM_AirAccelerate (vec3_t wishdir, float wishspeed, float accel)
{
	int			i;
	float		addspeed, accelspeed, currentspeed, wishspd = wishspeed;
	float		originalspeed, newspeed, speedcap;

	if (pmove.pm_type == PM_DEAD)
		return;
	if (pmove.waterjumptime)
		return;

	if (movevars.bunnyspeedcap > 0)
	{
		originalspeed = sqrt(pmove.velocity[0]*pmove.velocity[0] +
						pmove.velocity[1]*pmove.velocity[1]);
	}
	else
		originalspeed = 0;	//shh compiler.

	if (wishspd > movevars_maxairspeed)
		wishspd = movevars_maxairspeed;
	currentspeed = DotProduct (pmove.velocity, wishdir);
	addspeed = wishspd - currentspeed;
	if (addspeed <= 0)
		return;
	accelspeed = accel * wishspeed * frametime;
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	for (i=0 ; i<3 ; i++)
		pmove.velocity[i] += accelspeed*wishdir[i];

	if (movevars.bunnyspeedcap > 0)
	{
		newspeed = sqrt(pmove.velocity[0]*pmove.velocity[0] +
					pmove.velocity[1]*pmove.velocity[1]);
		if (newspeed > originalspeed)
		{
			speedcap = movevars.maxspeed * movevars.bunnyspeedcap;
			if (newspeed > speedcap)
			{
				if (originalspeed < speedcap)
					originalspeed = speedcap;
				pmove.velocity[0] *= originalspeed / newspeed;
				pmove.velocity[1] *= originalspeed / newspeed;
			}
		}
	}
}



/*
===================
PM_WaterMove
===================
*/
void PM_WaterMove (void)
{
	int		i;
	vec3_t	wishvel;
	float	wishspeed;
	vec3_t	wishdir;

//
// user intentions
//
	for (i=0 ; i<3 ; i++)
		wishvel[i] = forward[i]*pmove.cmd.forwardmove + right[i]*pmove.cmd.sidemove;

	if (pmove.pm_type != PM_FLY && !pmove.cmd.forwardmove && !pmove.cmd.sidemove && !pmove.cmd.upmove && !pmove.onladder)
	{
		VectorMA(wishvel, movevars.watersinkspeed, pmove.gravitydir, wishvel);
	}
	else
	{
		VectorMA(wishvel, -pmove.cmd.upmove, pmove.gravitydir, wishvel);
	}

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	if (wishspeed > movevars.maxspeed) {
		VectorScale (wishvel, movevars.maxspeed/wishspeed, wishvel);
		wishspeed = movevars.maxspeed;
	}
	wishspeed *= 0.7;

//
// water acceleration
//
	PM_Accelerate (wishdir, wishspeed, movevars.wateraccelerate);

	PM_StepSlideMove (false);
}


/*
*/
void PM_FlyMove (void)
{
	int		i;
	vec3_t	wishvel;
	float	wishspeed;
	vec3_t	wishdir;

	if (pmove.pm_type == PM_6DOF)
	{
		for (i=0 ; i<3 ; i++)
			wishvel[i] = forward[i]*pmove.cmd.forwardmove + right[i]*pmove.cmd.sidemove + up[i]*pmove.cmd.upmove;
	}
	else
	{
		for (i=0 ; i<3 ; i++)
			wishvel[i] = forward[i]*pmove.cmd.forwardmove + right[i]*pmove.cmd.sidemove;

		VectorMA(wishvel, -pmove.cmd.upmove, pmove.gravitydir, wishvel);
	}

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	if (wishspeed > movevars.maxspeed) {
		VectorScale (wishvel, movevars.maxspeed/wishspeed, wishvel);
		wishspeed = movevars.maxspeed;
	}

	PM_Accelerate (wishdir, wishspeed, movevars.accelerate);

	PM_StepSlideMove (false);
}

void PM_LadderMove (void)
{
	int		i;
	vec3_t	wishvel;
	float	wishspeed;
	vec3_t	wishdir;
	vec3_t	start, dest;
	trace_t	trace;

//
// user intentions
//
	for (i=0 ; i<3 ; i++)
		wishvel[i] = forward[i]*pmove.cmd.forwardmove + right[i]*pmove.cmd.sidemove + up[i]*pmove.cmd.upmove;

	if (wishvel[2] >= 100 || wishvel[2] <= -100)	//large up/down move
		wishvel[2]*=10;

	if (pmove.cmd.buttons & 2)
	{
		VectorMA(wishvel, -movevars.maxspeed, pmove.gravitydir, wishvel);
	}

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	if (wishspeed > movevars.maxspeed)
	{
		VectorScale (wishvel, movevars.maxspeed/wishspeed, wishvel);
		wishspeed = movevars.maxspeed;
	}

	PM_Accelerate (wishdir, wishspeed, movevars.wateraccelerate);

// assume it is a stair or a slope, so press down from stepheight above
	VectorMA (pmove.origin, frametime, pmove.velocity, dest);
	VectorMA(dest, -(movevars.stepheight + 1), pmove.gravitydir, start);
	trace = PM_PlayerTrace (start, dest, MASK_PLAYERSOLID);
	if (!trace.startsolid && !trace.allsolid)	// FIXME: check steep slope?
	{	// walked up the step
		VectorCopy (trace.endpos, pmove.origin);
		return;
	}

	PM_FlyMove ();

}

/*
===================
PM_AirMove

===================
*/
void PM_AirMove (void)
{
	int			i;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;

	if (pmove.gravitydir[2] == -1 && (pmove.angles[0] == 90 || pmove.angles[0] == -90))
	{	//HACK: attempt to avoid a stupid numerical precision issue.
		//You know its a hack because I'm comparing exact angles.
		vec3_t tmp;
		VectorSet(tmp, pmove.angles[0]*0.99, pmove.angles[1], pmove.angles[2]);
		AngleVectors (tmp, forward, right, up);
	}

	fmove = pmove.cmd.forwardmove;
	smove = pmove.cmd.sidemove;
	VectorMA(forward, -DotProduct(forward, pmove.gravitydir), pmove.gravitydir, forward); //z=0
	VectorMA(right, -DotProduct(right, pmove.gravitydir), pmove.gravitydir, right); //z=0
	VectorNormalize (forward);
	VectorNormalize (right);

	for (i=0 ; i<3 ; i++)
		wishdir[i] = forward[i]*fmove + right[i]*smove;
	VectorMA(wishdir, -DotProduct(wishdir, pmove.gravitydir), pmove.gravitydir, wishdir); //z=0

	wishspeed = VectorNormalize(wishdir);

//
// clamp to server defined max speed
//
	if (wishspeed > movevars.maxspeed)
	{
		wishspeed = movevars.maxspeed;
	}

	if (pmove.onground)
	{
		if (movevars.slidefix)
		{
			if (DotProduct(pmove.velocity, pmove.gravitydir) < 0)
			{
				VectorMA(pmove.velocity, -DotProduct(pmove.velocity, pmove.gravitydir), pmove.gravitydir, pmove.velocity); //z=0
				//pmove.velocity[2] = min(pmove.velocity[2], 0);	// bound above by 0
			}
			PM_Accelerate (wishdir, wishspeed, movevars.accelerate);
			// add gravity
			VectorMA(pmove.velocity, movevars.entgravity * movevars.gravity * frametime, pmove.gravitydir, pmove.velocity);
		}
		else
		{
			VectorMA(pmove.velocity, -DotProduct(pmove.velocity, pmove.gravitydir), pmove.gravitydir, pmove.velocity); //z=0
			PM_Accelerate (wishdir, wishspeed, movevars.accelerate);
		}

		//clear the z out, so we can test if we're moving horizontally relative to gravity
		VectorMA(pmove.velocity, -DotProduct(pmove.velocity, pmove.gravitydir), pmove.gravitydir, wishdir);
		if (!DotProduct(wishdir, wishdir) && !movevars.slidyslopes)
		{
			//clear z if we're not moving
			VectorClear(pmove.velocity);
			return;
		}
		else if (!movevars.slidefix && !movevars.slidyslopes)
			VectorMA(pmove.velocity, -DotProduct(pmove.velocity, pmove.gravitydir), pmove.gravitydir, pmove.velocity); //z=0

		PM_StepSlideMove(false);
	}
	else
	{
		int blocked;

		// not on ground, so little effect on velocity
		PM_AirAccelerate (wishdir, wishspeed, movevars.accelerate);

		// add gravity
		VectorMA(pmove.velocity, movevars.entgravity * movevars.gravity * frametime, pmove.gravitydir, pmove.velocity);

		if (DotProduct(pmove.velocity,pmove.velocity) > 1000*1000)
		{
			//when in a windtunnel, step up from where we are rather than the actual ground in order to more closely match nq.
			//this is needed for r1m5 (770 800 192), just beyond the silver key door.
			blocked = PM_StepSlideMove (false);
		}
		else if (movevars.airstep)
			blocked = PM_StepSlideMove (true);
		else
			blocked = PM_SlideMove ();

		if (movevars.pground && (blocked & BLOCKED_FLOOR))
			pmove.onground = true;
	}
}


cplane_t	groundplane;

/*
=============
PM_CategorizePosition
=============
*/
trace_t PM_TraceLine (vec3_t start, vec3_t end);
void PM_CategorizePosition (void)
{
	vec3_t		point;
	int			cont;
	trace_t		trace;

	if (pmove.gravitydir[0] == 0 && pmove.gravitydir[1] == 0 && pmove.gravitydir[2] == 0)
	{
		pmove.gravitydir[0] = 0;
		pmove.gravitydir[1] = 0;
		pmove.gravitydir[2] = -1;
	}
	if (pmove.pm_type == PM_WALLWALK)
	{
		vec3_t tmin,tmax;
		VectorCopy(pmove.player_mins, tmin);
		VectorCopy(pmove.player_maxs, tmax);

//		//try tracing forwards+down
//		VectorMA(pmove.origin, -48, up, point);
//		VectorMA(point, 48, forward, point);
//		trace = PM_TraceLine(pmove.origin, point);
//		trace.fraction = 1;
//		if (1)//trace.fraction == 1)
		{	//getting desparate
			VectorMA(pmove.origin, -48, up, point);
			VectorMA(point, 48, forward, point);
			trace = PM_TraceLine(pmove.origin, point);
		}
		if (trace.fraction == 1)
		{
			//try tracing directly down only (we may be stepping off a cliff)
			VectorMA(pmove.origin, -48, up, point);
			trace = PM_TraceLine(pmove.origin, point);
		}
		if (trace.fraction == 1)
		{
			vec3_t point2;
			//try tracing back from the cliff to see if we can find the ground beyond
			VectorMA(point, 48, forward, point2);
			VectorMA(point2, 48, forward, point);
			trace = PM_TraceLine(point2, point);
		}
		if (trace.fraction == 1)
		{	//getting desparate
			VectorMA(pmove.origin, -48, up, point);
			VectorMA(point, -48, forward, point);
			trace = PM_TraceLine(pmove.origin, point);
		}

		VectorCopy(tmin, pmove.player_mins);
		VectorCopy(tmax, pmove.player_maxs);

		if (trace.fraction < 1)
			VectorNegate(trace.plane.normal, pmove.gravitydir);
	}

// if the player hull point one unit down is solid, the player
// is on ground

// see if standing on something solid
	VectorAdd(pmove.origin, pmove.gravitydir, point);
	trace.startsolid = trace.allsolid = true;
	VectorClear(trace.endpos);
	if (-DotProduct(pmove.gravitydir, pmove.velocity) > 180)
	{
		pmove.onground = false;
	}
	else if (!movevars.pground || pmove.onground)
	{
		trace = PM_PlayerTracePortals (pmove.origin, point, MASK_PLAYERSOLID, NULL);
		if (!trace.startsolid && trace.fraction < 1 && -DotProduct(pmove.gravitydir, trace.plane.normal) < MIN_STEP_NORMAL)
		{	//if the trace hit a slope, slide down the slope to see if we can find ground below. this should fix the 'base-of-slope-is-slide' bug.
			vec3_t bounce;
			PM_ClipVelocity (pmove.gravitydir, trace.plane.normal, bounce, 2);
			VectorMA(trace.endpos, 1-trace.fraction, bounce, point);
			trace = PM_PlayerTracePortals (trace.endpos, point, MASK_PLAYERSOLID, NULL);
		}

		if (!trace.startsolid && (trace.fraction == 1 || -DotProduct(pmove.gravitydir, trace.plane.normal) < MIN_STEP_NORMAL))
			pmove.onground = false;
		else
		{
			pmove.onground = !trace.startsolid;
			pmove.groundent = trace.entnum;
			groundplane = trace.plane;
			pmove.waterjumptime = 0;
		}

		// standing on an entity other than the world
		if (trace.entnum > 0)
			PM_AddTouchedEnt (trace.entnum);
	}

//
// get waterlevel
//
	pmove.waterlevel = 0;
	pmove.watertype = FTECONTENTS_EMPTY;

	//FIXME: gravitydir
	VectorCopy(pmove.origin, point);
	point[2] = pmove.origin[2] + pmove.player_mins[2] + 1;
	cont = PM_PointContents (point);

	if (cont & FTECONTENTS_FLUID)
	{
		pmove.watertype = cont;
		pmove.waterlevel = 1;
		point[2] = pmove.origin[2] + (pmove.player_mins[2] + pmove.player_maxs[2])*0.5;
		cont = PM_PointContents (point);
		if (cont & FTECONTENTS_FLUID)
		{
			pmove.waterlevel = 2;
			point[2] = pmove.origin[2] + pmove.player_mins[2]+24+DEFAULT_VIEWHEIGHT;
			cont = PM_PointContents (point);
			if (cont & FTECONTENTS_FLUID)
				pmove.waterlevel = 3;
		}
	}

	//bsp objects marked as ladders mark regions to stand in to be classed as on a ladder.
	cont = PM_ExtraBoxContents(pmove.origin);

	if (pmove.physents[0].model)
	{
#ifdef Q3BSPS
		//q3 has surfaceflag-based ladders
		if (pmove.physents[0].model->fromgame == fg_quake3)
		{
			trace_t t;
			vec3_t flatforward, fwd1;

			flatforward[0] = forward[0];
			flatforward[1] = forward[1];
			flatforward[2] = 0;
			VectorNormalize (flatforward);

			VectorMA (pmove.origin, 24, flatforward, fwd1);

			pmove.physents[0].model->funcs.NativeTrace(pmove.physents[0].model, 0, PE_FRAMESTATE, NULL, pmove.origin, fwd1, pmove.player_mins, pmove.player_maxs, pmove.capsule, MASK_PLAYERSOLID, &t);
			if (t.surface && t.surface->flags & Q3SURFACEFLAG_LADDER)
			{
				pmove.onladder = true;
				pmove.onground = false;	// too steep
			}
		}
#endif
		//q2 has contents-based ladders
		if ((cont & FTECONTENTS_LADDER) || ((cont & Q2CONTENTS_LADDER) && pmove.physents[0].model->fromgame == fg_quake2))
		{
			trace_t t;
			vec3_t flatforward, fwd1;

			flatforward[0] = forward[0];
			flatforward[1] = forward[1];
			flatforward[2] = 0;
			VectorNormalize (flatforward);

			VectorMA (pmove.origin, 24, flatforward, fwd1);

			//if we hit a wall when going forwards and we are in a ladder region, then we are on a ladder.
			t = PM_PlayerTrace(pmove.origin, fwd1, MASK_PLAYERSOLID);
			if (t.fraction < 1)
			{
				pmove.onladder = true;
				pmove.onground = false;	// too steep
			}
		}
	}

	if (!movevars.pground && pmove.onground && pmove.pm_type != PM_FLY && pmove.waterlevel < 2)
	{
		// snap to ground so that we can't jump higher than we're supposed to
		if (!trace.startsolid && !trace.allsolid)
			VectorCopy (trace.endpos, pmove.origin);
	}
}


/*
=============
PM_CheckJump
=============
*/
static void PM_CheckJump (void)
{
	if (pmove.pm_type == PM_FLY)
		return;

	if (pmove.pm_type == PM_DEAD)
	{
		pmove.jump_held = true;	// don't jump on respawn
		return;
	}

	if (!(pmove.cmd.buttons & BUTTON_JUMP))
	{
		pmove.jump_held = false;
		return;
	}

	if (pmove.waterjumptime)
		return;

	if (pmove.waterlevel >= 2)
	{	// swimming, not jumping
		float speed;
		pmove.onground = false;

		if (pmove.watertype == FTECONTENTS_WATER)
			speed = 100;
		else if (pmove.watertype == FTECONTENTS_SLIME)
			speed = 80;
		else
			speed = 50;

		VectorMA(pmove.velocity, -speed-DotProduct(pmove.velocity, pmove.gravitydir), pmove.gravitydir, pmove.velocity);
		return;
	}

	if (!pmove.onground)
		return;		// in air, so no effect

	if (pmove.jump_held && !pmove.jump_msec)
		return;		// don't pogo stick

	// check for jump bug
	// groundplane normal was set in the call to PM_CategorizePosition
	if (!movevars.pground && -DotProduct(pmove.gravitydir, pmove.velocity) < 0 && DotProduct(pmove.velocity, groundplane.normal) < -0.1)
	{
		// pmove.velocity is pointing into the ground, clip it
		PM_ClipVelocity (pmove.velocity, groundplane.normal, pmove.velocity, 1);
	}

	pmove.onground = false;
	VectorMA(pmove.velocity, -movevars_jumpspeed, pmove.gravitydir, pmove.velocity);

	if (movevars.ktjump > 0 && pmove.pm_type != PM_WALLWALK)
	{
		if (movevars.ktjump > 1)
			movevars.ktjump = 1;
		if (pmove.velocity[2] < movevars_jumpspeed)
			pmove.velocity[2] = pmove.velocity[2] * (1 - movevars.ktjump)
				+ movevars_jumpspeed * movevars.ktjump;
	}

	pmove.jump_held = true;		// don't jump again until released
	pmove.jump_msec = pmove.cmd.msec;
}

/*
=============
PM_CheckWaterJump
=============
*/
static void PM_CheckWaterJump (void)
{
	vec3_t	spot, spot2;
//	int		cont;
	vec3_t	flatforward;
	trace_t tr;
	vec3_t oldmin, oldmax;

	if (pmove.waterjumptime>0)
		return;
	if (pmove.pm_type == PM_DEAD)
		return;

	// don't hop out if we just jumped in
	if (pmove.velocity[2] < -180)
		return;

	// see if near an edge
	flatforward[0] = forward[0];
	flatforward[1] = forward[1];
	flatforward[2] = 0;
	VectorNormalize (flatforward);

#if 1	//NQ does a traceline, which gives more permissive results. This is required for various maps that have awkward water jumps.
	VectorCopy(pmove.player_mins, oldmin);
	VectorCopy(pmove.player_maxs, oldmax);
	VectorCopy(pmove.origin, spot);
	spot[2] += 8 + 24+pmove.player_mins[2];	//hexen2 fix. calculated from the normal bottom of bbox
	VectorMA (spot, 24, flatforward, spot2);
	tr = PM_TraceLine(spot, spot2);
	VectorCopy(oldmin, pmove.player_mins);
	VectorCopy(oldmax, pmove.player_maxs);
	if (tr.fraction == 1)	//(possibly) give up if open at waist
	{	//NQ bug workaround: NQ does waterjump checks inside prethink, and THEN sets waterlevel after.
		//The player then moves to where waterlevel SHOULD be 3, except you're still allowed to waterjump because of last frame.
		//Which is horrible buggy framerate-dependant behaviour...
		//so lets just try again 2qu up.
		//This'll cause slight prediction issues with other qw engines, and maybe some newly bugged maps, but those maps were probably already buggy with a low enough nq framerate.
		spot[2] += 2;
		spot2[2] += 2;
		tr = PM_TraceLine(spot, spot2);
		VectorCopy(oldmin, pmove.player_mins);
		VectorCopy(oldmax, pmove.player_maxs);
		if (tr.fraction == 1)	//give up if open at waist
			return;
	}
	spot[2] += 24;
	spot2[2] += 24;
	tr = PM_TraceLine(spot, spot2);
	VectorCopy(oldmin, pmove.player_mins);
	VectorCopy(oldmax, pmove.player_maxs);
	if (tr.fraction < 1)	//give up if blocked at eye
		return;
#else
	VectorMA (pmove.origin, 24, flatforward, spot);
	spot[2] += 8 + 24+pmove.player_mins[2];	//hexen2 fix. calculated from the normal bottom of bbox
	cont = PM_PointContents (spot);
	if (!(cont & FTECONTENTS_SOLID))
		return;
	spot[2] += 24;
	cont = PM_PointContents (spot);
	if (cont != FTECONTENTS_EMPTY)
		return;
#endif
	// jump out of water
	VectorScale (flatforward, 50, pmove.velocity);
	pmove.velocity[2] = 310;
	pmove.waterjumptime = 2;	// safety net
	pmove.jump_held = true;		// don't jump again until released
}

/*
=================
PM_NudgePosition

If pmove.origin is in a solid position,
try nudging slightly on all axis to
allow for the cut precision of the net coordinates
=================
*/
static void PM_NudgePosition (void)
{
	vec3_t	base, nudged;
	int		x, y, z;
	int		i;
	static float	sign[] = {0, -1/8.0, 1/8.0};

	VectorCopy (pmove.origin, base);

	//really we want to just use this here
	//base[i] = MSG_FromCoord(MSG_ToCoord(pmove.origin[i], movevars.coordsize), movevars.coordsize);
	//but it has overflow issues, so do things the painful way instead.
	//this stuff is so annoying because we're trying to avoid biasing the position towards 0. you'll see the effects of that if you use a low forwardspeed or low sv_gamespeed etc, but its also noticable with default settings too.
	if (
#ifdef HAVE_LEGACY
			pm_noround.ival ||
#endif
			movevars.coordtype == COORDTYPE_FLOAT_32)	//float precision on the network. no need to truncate.
	{
		VectorCopy (base, nudged);
	}
	else if (movevars.coordtype == COORDTYPE_FIXED_13_3)	//1/8th precision, but don't truncate because that screws everything up.
	{
		for (i=0 ; i<3 ; i++)
		{
			/*if (pmove.velocity[i])
			{	//round in the direction of velocity, which means we're less likely to get stuck.
				if (pmove.velocity[i] >= 0)
					nudged[i] = (qintptr_t)(base[i]*8+0.5f) / 8.0;
				else
					nudged[i] = (qintptr_t)(base[i]*8-0.5f) / 8.0;
			}
			else*/
			{
				if (base[i] >= 0)
					nudged[i] = (qintptr_t)(base[i]*8+0.5f) / 8.0;
				else
					nudged[i] = (qintptr_t)(base[i]*8-0.5f) / 8.0;
			}
		}
	}
	else for (i=0 ; i<3 ; i++)
		nudged[i] = ((qintptr_t) (pmove.origin[i] * 8)) * 0.125;	//legacy compat, which biases towards the origin.

//	VectorCopy (base, pmove.origin);

	//if we're moving, allow that spot without snapping to any grid
//	if (pmove.velocity[0] || pmove.velocity[1] || pmove.velocity[2])
//		if (PM_TestPlayerPosition (pmove.origin, false))
//			return;

	//this is potentially 27 tests, and required for qw compat...
	//with unquantized floors it often succeeds only after 19 checks. which sucks.
	for (z=0 ; z<countof(sign) ; z++)
	{
		for (x=0 ; x<countof(sign) ; x++)
		{
			for (y=0 ; y<countof(sign) ; y++)
			{
				pmove.origin[0] = nudged[0] + sign[x];
				pmove.origin[1] = nudged[1] + sign[y];
				pmove.origin[2] = nudged[2] + sign[z];
				if (PM_TestPlayerPosition (pmove.origin, false))
					return;
			}
		}
	}

	//still not managed it... be more agressive axially.
	for (z=0 ; z<3; z++)
	{
		VectorCopy(base, pmove.origin);
		pmove.origin[z] = nudged[z] + (2/8.0);
		if (PM_TestPlayerPosition (pmove.origin, false))
			return;

		VectorCopy(base, pmove.origin);
		pmove.origin[z] = nudged[z] - (2/8.0);
		if (PM_TestPlayerPosition (pmove.origin, false))
			return;
	}

	//be more aggresssive at moving up, to match NQ
	for (z=1 ; z<movevars.stepheight ; z++)
	{
		for (x=0 ; x<3 ; x++)
		{
			for (y=0 ; y<3 ; y++)
			{
				pmove.origin[0] = nudged[0] + sign[x];
				pmove.origin[1] = nudged[1] + sign[y];
				pmove.origin[2] = nudged[2] + z;
				if (PM_TestPlayerPosition (pmove.origin, false))
					return;
			}
		}
	}

	if (pmove.safeorigin_known && PM_TestPlayerPosition(pmove.safeorigin, false))
		VectorCopy (pmove.safeorigin, pmove.origin);
	else
		VectorCopy (base, pmove.origin);
//	Con_DPrintf ("NudgePosition: stuck\n");
}

/*
===============
PM_SpectatorMove
===============
*/
void PM_SpectatorMove (void)
{
	float	speed, drop, friction, control, newspeed;
	float	currentspeed, addspeed, accelspeed;
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;

	// friction

	speed = Length (pmove.velocity);
	if (speed < 1)
	{
		VectorClear (pmove.velocity);
	}
	else
	{
		drop = 0;

		friction = movevars.friction*1.5;	// extra friction
		control = speed < movevars.stopspeed ? movevars.stopspeed : speed;
		drop += control*friction*frametime;

		// scale the velocity
		newspeed = speed - drop;
		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;

		VectorScale (pmove.velocity, newspeed, pmove.velocity);
	}

	// accelerate
	fmove = pmove.cmd.forwardmove;
	smove = pmove.cmd.sidemove;

	VectorNormalize (forward);
	VectorNormalize (right);

	for (i=0 ; i<3 ; i++)
		wishvel[i] = forward[i]*fmove + right[i]*smove;
	wishvel[2] += pmove.cmd.upmove;

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	//
	// clamp to server defined max speed
	//
	if (wishspeed > movevars.spectatormaxspeed)
	{
		VectorScale (wishvel, movevars.spectatormaxspeed/wishspeed, wishvel);
		wishspeed = movevars.spectatormaxspeed;
	}

	currentspeed = DotProduct(pmove.velocity, wishdir);
	addspeed = wishspeed - currentspeed;

	// Buggy QW spectator mode, kept for compatibility
	if (pmove.pm_type == PM_OLD_SPECTATOR)
	{
		if (addspeed <= 0)
			return;
	}

	if (addspeed > 0) {
		accelspeed = movevars.accelerate*frametime*wishspeed;
		if (accelspeed > addspeed)
			accelspeed = addspeed;

		for (i=0 ; i<3 ; i++)
			pmove.velocity[i] += accelspeed*wishdir[i];
	}

	// move
	VectorMA (pmove.origin, frametime, pmove.velocity, pmove.origin);
}

/*
=============
PM_PlayerMove

Returns with origin, angles, and velocity modified in place.

Numtouch and touchindex[] will be set if any of the physents
were contacted during the move.
=============
*/
void PM_PlayerMove (float gamespeed)
{
//	int i;
//	int tmp;	//for rounding

	frametime = pmove.cmd.msec * 0.001*gamespeed;
	pmove.numtouch = 0;

	if (pmove.pm_type == PM_NONE || pmove.pm_type == PM_FREEZE)
	{
		PM_CategorizePosition ();
		return;
	}

	// take angles directly from command
	pmove.angles[0] = SHORT2ANGLE(pmove.cmd.angles[0]);
	pmove.angles[1] = SHORT2ANGLE(pmove.cmd.angles[1]);
	pmove.angles[2] = SHORT2ANGLE(pmove.cmd.angles[2]);

	AngleVectors (pmove.angles, forward, right, up);

	if (pmove.pm_type == PM_SPECTATOR || pmove.pm_type == PM_OLD_SPECTATOR)
	{
		PM_SpectatorMove ();
		pmove.onground = false;
		return;
	}

	PM_NudgePosition ();

	// set onground, watertype, and waterlevel
	PM_CategorizePosition ();

	if (movevars.autobunny && !pmove.onground)
		pmove.jump_held = false;

	if (pmove.waterlevel == 2 && pmove.pm_type != PM_FLY)
		PM_CheckWaterJump ();

	if (-DotProduct(pmove.gravitydir, pmove.velocity) < 0 || pmove.pm_type == PM_DEAD)
		pmove.waterjumptime = 0;

	if (pmove.waterjumptime)
	{
		pmove.waterjumptime -= frametime;
		if (pmove.waterjumptime < 0)
			pmove.waterjumptime = 0;
	}

	if (pmove.jump_msec)
	{
		pmove.jump_msec += pmove.cmd.msec;
		if (pmove.jump_msec > 50)
			pmove.jump_msec = 0;
	}


	if (!movevars.bunnyfriction)
		PM_CheckJump ();	//qw-style bunny
	PM_Friction ();

	if (pmove.waterlevel >= 2)
		PM_WaterMove ();
	else if (pmove.pm_type == PM_FLY || pmove.pm_type == PM_6DOF)
		PM_FlyMove ();
	else if (pmove.onladder)
		PM_LadderMove ();
	else
		PM_AirMove ();

	if (movevars.bunnyfriction)
		PM_CheckJump ();	//nq-style bunny. note tick rate differences too.

/*	//round to network precision
	for (i = 0; i < 3; i++)
	{
		tmp = floor(pmove.velocity[i]*8 + 0.5);
		pmove.velocity[i] = tmp/8.0;
		tmp = floor(pmove.origin[i]*8 + 0.5);
		pmove.origin[i] = tmp/8.0;
	}
	PM_NudgePosition ();
*/
	// set onground, watertype, and waterlevel for final spot
	PM_CategorizePosition ();

	// this is to make sure landing sound is not played twice
	// and falling damage is calculated correctly
	if (!movevars.pground && pmove.onground && -DotProduct(pmove.gravitydir, pmove.velocity) < -300
		&& DotProduct(pmove.velocity, groundplane.normal) < -0.1)
	{
		PM_ClipVelocity (pmove.velocity, groundplane.normal, pmove.velocity, 1);
	}
}
