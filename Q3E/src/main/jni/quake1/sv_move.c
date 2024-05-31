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
// sv_move.c -- monster movement

#include "quakedef.h"
#include "prvm_cmds.h"

/*
=============
SV_CheckBottom

Returns false if any part of the bottom of the entity is off an edge that
is not a staircase.

=============
*/
int c_yes, c_no;

qbool SV_CheckBottom (prvm_edict_t *ent)
{
	prvm_prog_t *prog = SVVM_prog;
	vec3_t	mins, maxs, start, stop;
	trace_t	trace;
	int		x, y;
	float	mid, bottom;

	VectorAdd (PRVM_serveredictvector(ent, origin), PRVM_serveredictvector(ent, mins), mins);
	VectorAdd (PRVM_serveredictvector(ent, origin), PRVM_serveredictvector(ent, maxs), maxs);

// if all of the points under the corners are solid world, don't bother
// with the tougher checks
// the corners must be within 16 of the midpoint
	start[2] = mins[2] - 1;
	for	(x=0 ; x<=1 ; x++)
		for	(y=0 ; y<=1 ; y++)
		{
			start[0] = x ? maxs[0] : mins[0];
			start[1] = y ? maxs[1] : mins[1];
			if (!(SV_PointSuperContents(start) & (SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY)))
				goto realcheck;
		}

	c_yes++;
	return true;		// we got out easy

realcheck:
	c_no++;
//
// check it for real...
//
	start[2] = mins[2];

// the midpoint must be within 16 of the bottom
	start[0] = stop[0] = (mins[0] + maxs[0])*0.5;
	start[1] = stop[1] = (mins[1] + maxs[1])*0.5;
	stop[2] = start[2] - 2*sv_stepheight.value;
	trace = SV_TraceLine(start, stop, MOVE_NOMONSTERS, ent, SV_GenericHitSuperContentsMask(ent), 0, 0, collision_extendmovelength.value);

	if (trace.fraction == 1.0)
		return false;
	mid = bottom = trace.endpos[2];

// the corners must be within 16 of the midpoint
	for	(x=0 ; x<=1 ; x++)
		for	(y=0 ; y<=1 ; y++)
		{
			start[0] = stop[0] = x ? maxs[0] : mins[0];
			start[1] = stop[1] = y ? maxs[1] : mins[1];

			trace = SV_TraceLine(start, stop, MOVE_NOMONSTERS, ent, SV_GenericHitSuperContentsMask(ent), 0, 0, collision_extendmovelength.value);

			if (trace.fraction != 1.0 && trace.endpos[2] > bottom)
				bottom = trace.endpos[2];
			if (trace.fraction == 1.0 || mid - trace.endpos[2] > sv_stepheight.value)
				return false;
		}

	c_yes++;
	return true;
}


/*
=============
SV_movestep

Called by monster program code.
The move will be adjusted for slopes and stairs, but if the move isn't
possible, no move is done and false is returned
=============
*/
qbool SV_movestep (prvm_edict_t *ent, vec3_t move, qbool relink, qbool noenemy, qbool settrace)
{
	prvm_prog_t *prog = SVVM_prog;
	float		dz;
	vec3_t		oldorg, neworg, end, traceendpos, entorigin, entmins, entmaxs;
	trace_t		trace;
	int			i;
	prvm_edict_t		*enemy;

// try the move
	VectorCopy (PRVM_serveredictvector(ent, origin), oldorg);
	VectorAdd (PRVM_serveredictvector(ent, origin), move, neworg);
	VectorCopy(PRVM_serveredictvector(ent, mins), entmins);
	VectorCopy(PRVM_serveredictvector(ent, maxs), entmaxs);

// flying monsters don't step up
	if ( (int)PRVM_serveredictfloat(ent, flags) & (FL_SWIM | FL_FLY) )
	{
	// try one move with vertical motion, then one without
		for (i=0 ; i<2 ; i++)
		{
			VectorAdd (PRVM_serveredictvector(ent, origin), move, neworg);
			if (noenemy)
				enemy = prog->edicts;
			else
			{
				enemy = PRVM_PROG_TO_EDICT(PRVM_serveredictedict(ent, enemy));
				if (i == 0 && enemy != prog->edicts)
				{
					dz = PRVM_serveredictvector(ent, origin)[2] - PRVM_serveredictvector(enemy, origin)[2];
					if (dz > 40)
						neworg[2] -= 8;
					if (dz < 30)
						neworg[2] += 8;
				}
			}
			VectorCopy(PRVM_serveredictvector(ent, origin), entorigin);
			trace = SV_TraceBox(entorigin, entmins, entmaxs, neworg, MOVE_NORMAL, ent, SV_GenericHitSuperContentsMask(ent), 0, 0, collision_extendmovelength.value);

			if (trace.fraction == 1)
			{
				VectorCopy(trace.endpos, traceendpos);
				if (((int)PRVM_serveredictfloat(ent, flags) & FL_SWIM) && !(SV_PointSuperContents(traceendpos) & SUPERCONTENTS_LIQUIDSMASK))
					return false;	// swim monster left water

				VectorCopy (traceendpos, PRVM_serveredictvector(ent, origin));
				if (relink)
				{
					SV_LinkEdict(ent);
					SV_LinkEdict_TouchAreaGrid(ent);
				}
				return true;
			}

			if (enemy == prog->edicts)
				break;
		}

		return false;
	}

// push down from a step height above the wished position
	neworg[2] += sv_stepheight.value;
	VectorCopy (neworg, end);
	end[2] -= sv_stepheight.value*2;

	trace = SV_TraceBox(neworg, entmins, entmaxs, end, MOVE_NORMAL, ent, SV_GenericHitSuperContentsMask(ent), 0, 0, collision_extendmovelength.value);

	if (trace.startsolid)
	{
		neworg[2] -= sv_stepheight.value;
		trace = SV_TraceBox(neworg, entmins, entmaxs, end, MOVE_NORMAL, ent, SV_GenericHitSuperContentsMask(ent), 0, 0, collision_extendmovelength.value);
		if (trace.startsolid)
			return false;
	}
	if (trace.fraction == 1)
	{
	// if monster had the ground pulled out, go ahead and fall
		if ( (int)PRVM_serveredictfloat(ent, flags) & FL_PARTIALGROUND )
		{
			VectorAdd (PRVM_serveredictvector(ent, origin), move, PRVM_serveredictvector(ent, origin));
			if (relink)
			{
				SV_LinkEdict(ent);
				SV_LinkEdict_TouchAreaGrid(ent);
			}
			PRVM_serveredictfloat(ent, flags) = (int)PRVM_serveredictfloat(ent, flags) & ~FL_ONGROUND;
			return true;
		}

		return false;		// walked off an edge
	}

// check point traces down for dangling corners
	VectorCopy (trace.endpos, PRVM_serveredictvector(ent, origin));

	if (!SV_CheckBottom (ent))
	{
		if ( (int)PRVM_serveredictfloat(ent, flags) & FL_PARTIALGROUND )
		{	// entity had floor mostly pulled out from underneath it
			// and is trying to correct
			if (relink)
			{
				SV_LinkEdict(ent);
				SV_LinkEdict_TouchAreaGrid(ent);
			}
			return true;
		}
		VectorCopy (oldorg, PRVM_serveredictvector(ent, origin));
		return false;
	}

	if ( (int)PRVM_serveredictfloat(ent, flags) & FL_PARTIALGROUND )
		PRVM_serveredictfloat(ent, flags) = (int)PRVM_serveredictfloat(ent, flags) & ~FL_PARTIALGROUND;

// gameplayfix: check if reached pretty steep plane and bail
	if ( ! ( (int)PRVM_serveredictfloat(ent, flags) & (FL_SWIM | FL_FLY) ) && sv_gameplayfix_nostepmoveonsteepslopes.integer )
	{
		if (trace.plane.normal[ 2 ] < 0.5)
		{
			VectorCopy (oldorg, PRVM_serveredictvector(ent, origin));
			return false;
		}
	}

	PRVM_serveredictedict(ent, groundentity) = PRVM_EDICT_TO_PROG(trace.ent);

// the move is ok
	if (relink)
	{
		SV_LinkEdict(ent);
		SV_LinkEdict_TouchAreaGrid(ent);
	}
	return true;
}


//============================================================================

/*
======================
SV_StepDirection

Turns to the movement direction, and walks the current distance if
facing it.

======================
*/
static qbool SV_StepDirection (prvm_edict_t *ent, float yaw, float dist)
{
	prvm_prog_t *prog = SVVM_prog;
	vec3_t		move, oldorigin;
	float		delta;

	PRVM_serveredictfloat(ent, ideal_yaw) = yaw;
	VM_changeyaw(prog);

	yaw = yaw*M_PI*2 / 360;
	move[0] = cos(yaw)*dist;
	move[1] = sin(yaw)*dist;
	move[2] = 0;

	VectorCopy (PRVM_serveredictvector(ent, origin), oldorigin);
	if (SV_movestep (ent, move, false, false, false))
	{
		delta = PRVM_serveredictvector(ent, angles)[YAW] - PRVM_serveredictfloat(ent, ideal_yaw);
		if (delta > 45 && delta < 315)
		{		// not turned far enough, so don't take the step
			VectorCopy (oldorigin, PRVM_serveredictvector(ent, origin));
		}
		SV_LinkEdict(ent);
		SV_LinkEdict_TouchAreaGrid(ent);
		return true;
	}
	SV_LinkEdict(ent);
	SV_LinkEdict_TouchAreaGrid(ent);

	return false;
}

/*
======================
SV_FixCheckBottom

======================
*/
static void SV_FixCheckBottom (prvm_edict_t *ent)
{
	prvm_prog_t *prog = SVVM_prog;
	PRVM_serveredictfloat(ent, flags) = (int)PRVM_serveredictfloat(ent, flags) | FL_PARTIALGROUND;
}



/*
================
SV_NewChaseDir

================
*/
#define	DI_NODIR	-1
static void SV_NewChaseDir (prvm_edict_t *actor, prvm_edict_t *enemy, float dist)
{
	prvm_prog_t *prog = SVVM_prog;
	float		deltax,deltay;
	float			d[3];
	float		tdir, olddir, turnaround;

	olddir = ANGLEMOD((int)(PRVM_serveredictfloat(actor, ideal_yaw)/45)*45);
	turnaround = ANGLEMOD(olddir - 180);

	deltax = PRVM_serveredictvector(enemy, origin)[0] - PRVM_serveredictvector(actor, origin)[0];
	deltay = PRVM_serveredictvector(enemy, origin)[1] - PRVM_serveredictvector(actor, origin)[1];
	if (deltax>10)
		d[1]= 0;
	else if (deltax<-10)
		d[1]= 180;
	else
		d[1]= DI_NODIR;
	if (deltay<-10)
		d[2]= 270;
	else if (deltay>10)
		d[2]= 90;
	else
		d[2]= DI_NODIR;

// try direct route
	if (d[1] != DI_NODIR && d[2] != DI_NODIR)
	{
		if (d[1] == 0)
			tdir = d[2] == 90 ? 45 : 315;
		else
			tdir = d[2] == 90 ? 135 : 215;

		if (tdir != turnaround && SV_StepDirection(actor, tdir, dist))
			return;
	}

// try other directions
	if ( ((rand()&3) & 1) ||  fabs(deltay)>fabs(deltax))
	{
		tdir=d[1];
		d[1]=d[2];
		d[2]=tdir;
	}

	if (d[1]!=DI_NODIR && d[1]!=turnaround
	&& SV_StepDirection(actor, d[1], dist))
			return;

	if (d[2]!=DI_NODIR && d[2]!=turnaround
	&& SV_StepDirection(actor, d[2], dist))
			return;

/* there is no direct path to the player, so pick another direction */

	if (olddir!=DI_NODIR && SV_StepDirection(actor, olddir, dist))
			return;

	if (rand()&1) 	/*randomly determine direction of search*/
	{
		for (tdir=0 ; tdir<=315 ; tdir += 45)
			if (tdir!=turnaround && SV_StepDirection(actor, tdir, dist) )
					return;
	}
	else
	{
		for (tdir=315 ; tdir >=0 ; tdir -= 45)
			if (tdir!=turnaround && SV_StepDirection(actor, tdir, dist) )
					return;
	}

	if (turnaround != DI_NODIR && SV_StepDirection(actor, turnaround, dist) )
			return;

	PRVM_serveredictfloat(actor, ideal_yaw) = olddir;		// can't move

// if a bridge was pulled out from underneath a monster, it may not have
// a valid standing position at all

	if (!SV_CheckBottom (actor))
		SV_FixCheckBottom (actor);

}

/*
======================
SV_CloseEnough

======================
*/
static qbool SV_CloseEnough (prvm_edict_t *ent, prvm_edict_t *goal, float dist)
{
	int		i;

	for (i=0 ; i<3 ; i++)
	{
		if (goal->priv.server->areamins[i] > ent->priv.server->areamaxs[i] + dist)
			return false;
		if (goal->priv.server->areamaxs[i] < ent->priv.server->areamins[i] - dist)
			return false;
	}
	return true;
}

/*
======================
VM_SV_MoveToGoal

======================
*/
void VM_SV_MoveToGoal(prvm_prog_t *prog)
{
	prvm_edict_t		*ent, *goal;
	float		dist;

	VM_SAFEPARMCOUNT(1, VM_SV_MoveToGoal);

	ent = PRVM_PROG_TO_EDICT(PRVM_serverglobaledict(self));
	goal = PRVM_PROG_TO_EDICT(PRVM_serveredictedict(ent, goalentity));
	dist = PRVM_G_FLOAT(OFS_PARM0);

	if ( !( (int)PRVM_serveredictfloat(ent, flags) & (FL_ONGROUND|FL_FLY|FL_SWIM) ) )
	{
		PRVM_G_FLOAT(OFS_RETURN) = 0;
		return;
	}

// if the next step hits the enemy, return immediately
	if ( PRVM_PROG_TO_EDICT(PRVM_serveredictedict(ent, enemy)) != prog->edicts &&  SV_CloseEnough (ent, goal, dist) )
		return;

// bump around...
	if ( (rand()&3)==1 ||
	!SV_StepDirection (ent, PRVM_serveredictfloat(ent, ideal_yaw), dist))
	{
		SV_NewChaseDir (ent, goal, dist);
	}
}

