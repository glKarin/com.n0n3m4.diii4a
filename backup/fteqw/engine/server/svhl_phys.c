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
// sv_phys.c

#include "quakedef.h"

#ifdef HLSERVER

#include "svhl_gcapi.h"

/*


pushmove objects do not obey gravity, and do not interact with each other or trigger fields, but block normal movement and push normal objects when they move.

onground is set for toss objects when they come to a complete rest.  it is set for steping or walking objects

doors, plats, etc are SOLID_BSP, and MOVETYPE_PUSH
bonus items are SOLID_TRIGGER touch, and MOVETYPE_TOSS
corpses are SOLID_NOT and MOVETYPE_TOSS
crates are SOLID_BBOX and MOVETYPE_TOSS
walking monsters are SOLID_SLIDEBOX and MOVETYPE_STEP
flying/floating monsters are SOLID_SLIDEBOX and MOVETYPE_FLY

solid_edge items only clip against bsp models.

*/

extern cvar_t	sv_maxvelocity;

extern cvar_t	sv_gravity;
extern cvar_t	sv_stopspeed;
extern cvar_t	sv_maxspeed;
extern cvar_t	sv_spectatormaxspeed;
extern cvar_t 	sv_accelerate;
extern cvar_t	sv_airaccelerate;
extern cvar_t	sv_wateraccelerate;
extern cvar_t	sv_friction;
extern cvar_t	sv_waterfriction;
extern cvar_t	sv_gameplayfix_noairborncorpse;


#define	MOVE_EPSILON	0.01

void SVHL_Physics_Toss (hledict_t *ent);

void SVHL_StartSound (hledict_t *entity, int channel, char *sample, int volume, float attenuation)
{
}

/*
================
SV_CheckAllEnts
================
*/
void SVHL_CheckAllEnts (void)
{
	int			e;
	hledict_t		*check;

// see if any solid entities are inside the final position
	for (e=1 ; e<SVHL_NumActiveEnts ; e++)
	{
		check = &SVHL_Edict[e];
		if (check->isfree)
			continue;
		if (check->v.movetype == MOVETYPE_PUSH
		|| check->v.movetype == MOVETYPE_NONE
		|| check->v.movetype == MOVETYPE_FOLLOW
		|| check->v.movetype == MOVETYPE_NOCLIP)
			continue;

		if (SVHL_TestEntityPosition (check))
			Con_Printf ("entity in invalid position\n");
	}
}

/*
================
SV_CheckVelocity
================
*/
void SVHL_CheckVelocity (hledict_t *ent)
{
	int		i;

//
// bound velocity
//
	for (i=0 ; i<3 ; i++)
	{
		if (IS_NAN(ent->v.velocity[i]))
		{
			Con_Printf ("Got a NaN velocity on %s\n", SVHL_Globals.stringbase+ent->v.classname);
			ent->v.velocity[i] = 0;
		}
		if (IS_NAN(ent->v.origin[i]))
		{
			Con_Printf ("Got a NaN origin on %s\n", SVHL_Globals.stringbase+ent->v.classname);
			ent->v.origin[i] = 0;
		}
	}

	if (Length(ent->v.velocity) > sv_maxvelocity.value)
	{
//		Con_DPrintf("Slowing %s\n", SVHL_Globals.stringbase+ent->v.classname);
		VectorScale (ent->v.velocity, sv_maxvelocity.value/Length(ent->v.velocity), ent->v.velocity);
	}
}

/*
=============
SV_RunThink

Runs thinking code if time.  There is some play in the exact time the think
function will be called, because it is called before any movement is done
in a frame.  Not used for pushmove objects, because they must be exact.
Returns false if the entity removed itself.
=============
*/
qboolean SVHL_RunThink (hledict_t *ent)
{
	float	thinktime;

	if (1)	//try and imitate nq as closeley as possible
	{
		thinktime = ent->v.nextthink;
		if (thinktime <= 0 || thinktime > sv.world.physicstime + host_frametime)
			return true;

		if (thinktime < sv.world.physicstime)
			thinktime = sv.world.physicstime;	// don't let things stay in the past.
									// it is possible to start that way
									// by a trigger with a local time.
		ent->v.nextthink = 0;
		SVHL_Globals.time = thinktime;
		SVHL_GameFuncs.DispatchThink(ent);

		return !ent->isfree;
	}

	do
	{
		thinktime = ent->v.nextthink;
		if (thinktime <= 0)
			return true;
		if (thinktime > sv.world.physicstime + host_frametime)
			return true;

		if (thinktime < sv.world.physicstime)
			thinktime = sv.world.physicstime;	// don't let things stay in the past.
									// it is possible to start that way
									// by a trigger with a local time.
		ent->v.nextthink = 0;

		SVHL_Globals.time = thinktime;
		SVHL_GameFuncs.DispatchThink(ent);

		if (ent->isfree)
			return false;

		if (ent->v.nextthink <= thinktime)	//hmm... infinate loop was possible here.. Quite a few non-QW mods do this.
			return true;
	} while (1);

	return true;
}

/*
==================
SV_Impact

Two entities have touched, so run their touch functions
==================
*/
void SVHL_Impact (hledict_t *e1, hledict_t *e2)
{
	SVHL_Globals.time = sv.world.physicstime;
	if (e1->v.solid != SOLID_NOT)
	{
		SVHL_GameFuncs.DispatchTouch(e1, e2);
	}

	if (e2->v.solid != SOLID_NOT)
	{
		SVHL_GameFuncs.DispatchTouch(e2, e1);
	}
}


/*
==================
ClipVelocity

Slide off of the impacting object
==================
*/
#define	STOP_EPSILON	0.1
//courtesy of darkplaces, it's just more efficient.
static void ClipVelocity (vec3_t in, vec3_t normal, vec3_t out, float overbounce)
{
	int i;
	float backoff;

	backoff = -DotProduct (in, normal) * overbounce;
	VectorMA(in, backoff, normal, out);

	for (i = 0;i < 3;i++)
		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
			out[i] = 0;
}


/*
============
SV_FlyMove

The basic solid body movement clip that slides along multiple planes
Returns the clipflags if the velocity was modified (hit something solid)
1 = floor
2 = wall / step
4 = dead stop
If steptrace is not NULL, the trace of any vertical wall hit will be stored
============
*/
#define	MAX_CLIP_PLANES	5
int SVHL_FlyMove (hledict_t *ent, float time, trace_t *steptrace)
{
	int			bumpcount, numbumps;
	vec3_t		dir;
	float		d;
	int			numplanes;
	vec3_t		planes[MAX_CLIP_PLANES];
	vec3_t		primal_velocity, original_velocity, new_velocity;
	int			i, j;
	trace_t		trace;
	vec3_t		end;
	float		time_left;
	int			blocked;
	vec3_t diff;

	vec3_t startorg;

	numbumps = 4;

	blocked = 0;
	VectorCopy (ent->v.velocity, original_velocity);
	VectorCopy (ent->v.velocity, primal_velocity);
	numplanes = 0;

	time_left = time;

	VectorCopy (ent->v.origin, startorg);

	for (bumpcount=0 ; bumpcount<numbumps ; bumpcount++)
	{
		for (i=0 ; i<3 ; i++)
			end[i] = ent->v.origin[i] + time_left * ent->v.velocity[i];

		trace = SVHL_Move (ent->v.origin, ent->v.mins, ent->v.maxs, end, false, 0, ent);

		if (trace.startsolid)
		{	// entity is trapped in another solid
			VectorClear (ent->v.velocity);
			return 3;
		}

		if (trace.fraction > 0)
		{	// actually covered some distance
			VectorCopy (trace.endpos, ent->v.origin);
			VectorCopy (ent->v.velocity, original_velocity);
			numplanes = 0;
		}

		if (trace.fraction == 1)
			 break;		// moved the entire distance

		if (!trace.ent)
			SV_Error ("SV_FlyMove: !trace.ent");

		if (trace.plane.normal[2] > 0.7)
		{
			blocked |= 1;		// floor
			if (((hledict_t *)trace.ent)->v.solid == SOLID_BSP)
			{
				ent->v.flags =	(int)ent->v.flags | FL_ONGROUND;
				ent->v.groundentity = trace.ent;
			}
		}
		if (!trace.plane.normal[2])
		{
			blocked |= 2;		// step
			if (steptrace)
				*steptrace = trace;	// save for player extrafriction
		}

//
// run the impact function
//
		SVHL_Impact (ent, trace.ent);
		if (ent->isfree)
			break;		// removed by the impact function


		time_left -= time_left * trace.fraction;

	// cliped to another plane
		if (numplanes >= MAX_CLIP_PLANES)
		{	// this shouldn't really happen
			VectorClear (ent->v.velocity);
			if (steptrace)
				*steptrace = trace;	// save for player extrafriction
			return 3;
		}

		if (0)
		{
			ClipVelocity(ent->v.velocity, trace.plane.normal, ent->v.velocity, 1);
			break;
		}
		else
		{
			if (numplanes)
			{
				VectorSubtract(planes[0], trace.plane.normal, diff);
				if (Length(diff) < 0.01)
					continue;	//hit this plane already
			}

			VectorCopy (trace.plane.normal, planes[numplanes]);
			numplanes++;

	//
	// modify original_velocity so it parallels all of the clip planes
	//
			for (i=0 ; i<numplanes ; i++)
			{
				ClipVelocity (original_velocity, planes[i], new_velocity, 1);
				for (j=0 ; j<numplanes ; j++)
					if (j != i)
					{
						if (DotProduct (new_velocity, planes[j]) < 0)
							break;	// not ok
					}
				if (j == numplanes)
					break;
			}

			if (i != numplanes)
			{	// go along this plane
//				Con_Printf ("%5.1f %5.1f %5.1f   ",ent->v.velocity[0], ent->v.velocity[1], ent->v.velocity[2]);
				VectorCopy (new_velocity, ent->v.velocity);
//				Con_Printf ("%5.1f %5.1f %5.1f\n",ent->v.velocity[0], ent->v.velocity[1], ent->v.velocity[2]);
			}
			else
			{	// go along the crease
				if (numplanes != 2)
				{
//					Con_Printf ("clip velocity, numplanes == %i\n",numplanes);
//					Con_Printf ("%5.1f %5.1f %5.1f   ",ent->v.velocity[0], ent->v.velocity[1], ent->v.velocity[2]);
					VectorClear (ent->v.velocity);
//					Con_Printf ("%5.1f %5.1f %5.1f\n",ent->v.velocity[0], ent->v.velocity[1], ent->v.velocity[2]);
					return 7;
				}
//				Con_Printf ("%5.1f %5.1f %5.1f   ",ent->v.velocity[0], ent->v.velocity[1], ent->v.velocity[2]);
				CrossProduct (planes[0], planes[1], dir);
				VectorNormalize(dir);	//fixes slow falling in corners
				d = DotProduct (dir, ent->v.velocity);
				VectorScale (dir, d, ent->v.velocity);
//				Con_Printf ("%5.1f %5.1f %5.1f\n",ent->v.velocity[0], ent->v.velocity[1], ent->v.velocity[2]);
			}
		}

//
// if original velocity is against the original velocity, stop dead
// to avoid tiny occilations in sloping corners
//
		if (DotProduct (ent->v.velocity, primal_velocity) <= 0)
		{
			VectorClear (ent->v.velocity);
			return blocked;
		}
	}

	return blocked;
}


/*
============
SV_AddGravity

============
*/
void SVHL_AddGravity (hledict_t *ent, float scale)
{
	if (!scale && progstype != PROG_QW)
		scale = 1;
	ent->v.velocity[2] -= scale * sv_gravity.value/*movevars.gravity*/ * host_frametime;
}

/*
===============================================================================

PUSHMOVE

===============================================================================
*/

/*
============
SV_PushEntity

Does not change the entities velocity at all
============
*/
trace_t SVHL_PushEntity (hledict_t *ent, vec3_t push)
{
	trace_t	trace;
	vec3_t	end;

	VectorAdd (ent->v.origin, push, end);

	if (ent->v.movetype == MOVETYPE_FLYMISSILE)
		trace = SVHL_Move (ent->v.origin, ent->v.mins, ent->v.maxs, end, MOVE_MISSILE, 0, ent);
	else if (ent->v.solid == SOLID_TRIGGER || ent->v.solid == SOLID_NOT)
	// only clip against bmodels
		trace = SVHL_Move (ent->v.origin, ent->v.mins, ent->v.maxs, end, MOVE_NOMONSTERS, 0, ent);
	else
		trace = SVHL_Move (ent->v.origin, ent->v.mins, ent->v.maxs, end, MOVE_NORMAL, 0, ent);

//	if (trace.ent)
//		VectorMA(trace.endpos, sv_impactpush.value, trace.plane.normal, ent->v.origin);
//	else
		VectorCopy (trace.endpos, ent->v.origin);
	SVHL_LinkEdict (ent, true);

	if (trace.ent)
		SVHL_Impact (ent, trace.ent);

	return trace;
}




typedef struct
{
	hledict_t	*ent;
	vec3_t	origin;
	vec3_t	angles;
//	float	deltayaw;
} hlpushed_t;
hlpushed_t	pushed[1024], *pushed_p;

/*
============
SV_Push

Objects need to be moved back on a failed push,
otherwise riders would continue to slide.
============
*/
qboolean SVHL_PushAngles (hledict_t *pusher, vec3_t move, vec3_t amove)
{
	int			i, e;
	hledict_t	*check, *block;
	vec3_t		mins, maxs;
	float oldsolid;
	hlpushed_t	*p;
	vec3_t		org, org2, move2, forward, right, up;

	pushed_p = pushed;

	// find the bounding box
	for (i=0 ; i<3 ; i++)
	{
		mins[i] = pusher->v.absmin[i] + move[i];
		maxs[i] = pusher->v.absmax[i] + move[i];
	}

// we need this for pushing things later
	VectorNegate (amove, org);
	AngleVectors (org, forward, right, up);

// save the pusher's original position
	pushed_p->ent = pusher;
	VectorCopy (pusher->v.origin, pushed_p->origin);
	VectorCopy (pusher->v.angles, pushed_p->angles);
//	if (pusher->client)
//		pushed_p->deltayaw = pusher->client->ps.pmove.delta_angles[YAW];
	pushed_p++;

// move the pusher to it's final position
	VectorAdd (pusher->v.origin, move, pusher->v.origin);
	VectorAdd (pusher->v.angles, amove, pusher->v.angles);
	SVHL_LinkEdict (pusher, false);

// see if any solid entities are inside the final position
	for (e = 1; e < SVHL_NumActiveEnts; e++)
	{
		check = &SVHL_Edict[e];
		if (check->isfree)
			continue;

		if (check->v.movetype == MOVETYPE_PUSH
		|| check->v.movetype == MOVETYPE_NONE
		|| check->v.movetype == MOVETYPE_NOCLIP)
			continue;

#if 1
		oldsolid = pusher->v.solid;
		pusher->v.solid = SOLID_NOT;
		block = SVHL_TestEntityPosition (check);
		pusher->v.solid = oldsolid;
		if (block)
			continue;
#else
		if (!check->area.prev)
			continue;		// not linked in anywhere
#endif

	// if the entity is standing on the pusher, it will definitely be moved
		if ( ! ( ((int)check->v.flags & FL_ONGROUND)
			&& check->v.groundentity == pusher) )
		{
			// see if the ent needs to be tested
			if ( check->v.absmin[0] >= maxs[0]
			|| check->v.absmin[1] >= maxs[1]
			|| check->v.absmin[2] >= maxs[2]
			|| check->v.absmax[0] <= mins[0]
			|| check->v.absmax[1] <= mins[1]
			|| check->v.absmax[2] <= mins[2] )
				continue;


			// see if the ent's bbox is inside the pusher's final position
			if (!SVHL_TestEntityPosition (check))
				continue;
		}

		if ((pusher->v.movetype == MOVETYPE_PUSH) || (check->v.groundentity == pusher))
		{
			// move this entity
			pushed_p->ent = check;
			VectorCopy (check->v.origin, pushed_p->origin);
			VectorCopy (check->v.angles, pushed_p->angles);
			pushed_p++;

			// try moving the contacted entity
			VectorAdd (check->v.origin, move, check->v.origin);
//			if (check->client)
//			{	// FIXME: doesn't rotate monsters?
//				check->client->ps.pmove.delta_angles[YAW] += amove[YAW];
//			}
			VectorAdd (check->v.angles, amove, check->v.angles);

			// figure movement due to the pusher's amove
			VectorSubtract (check->v.origin, pusher->v.origin, org);
			org2[0] = DotProduct (org, forward);
			org2[1] = -DotProduct (org, right);
			org2[2] = DotProduct (org, up);
			VectorSubtract (org2, org, move2);
			VectorAdd (check->v.origin, move2, check->v.origin);

			check->v.flags = (int)check->v.flags & ~FL_ONGROUND;

			// may have pushed them off an edge
			if (check->v.groundentity != pusher)
				check->v.groundentity = 0;

			block = SVHL_TestEntityPosition (check);
			if (!block)
			{	// pushed ok
				SVHL_LinkEdict (check, false);
				// impact?
				continue;
			}



			// if it is ok to leave in the old position, do it
			// this is only relevent for riding entities, not pushed
			// FIXME: this doesn't acount for rotation
			VectorSubtract (check->v.origin, move, check->v.origin);
			block = SVHL_TestEntityPosition (check);
			if (!block)
			{
				pushed_p--;
				continue;
			}
		}

		// if it is sitting on top. Do not block.
		if (check->v.mins[0] == check->v.maxs[0])
		{
			SVHL_LinkEdict (check, false);
			continue;
		}

//		Con_Printf("Pusher hit %s\n", PR_GetString(svprogfuncs, check->v.classname));
		SVHL_GameFuncs.DispatchBlocked(pusher, check);

		// move back any entities we already moved
		// go backwards, so if the same entity was pushed
		// twice, it goes back to the original position
		for (p=pushed_p-1 ; p>=pushed ; p--)
		{
			VectorCopy (p->origin, p->ent->v.origin);
			VectorCopy (p->angles, p->ent->v.angles);
//			if (p->ent->client)
//			{
//				p->ent->client->ps.pmove.delta_angles[YAW] = p->deltayaw;
//			}
			SVHL_LinkEdict (p->ent, false);
		}
		return false;
	}

//FIXME: is there a better way to handle this?
	// see if anything we moved has touched a trigger
	for (p=pushed_p-1 ; p>=pushed ; p--)
		SVHL_TouchLinks ( p->ent, sv.world.areanodes );

	return true;
}

/*
============
SV_Push

============
*/
#define MAX_PUSHED_ENTITIES 1024
qboolean SVHL_Push (hledict_t *pusher, vec3_t move, vec3_t amove)
{
	int			i, e;
	hledict_t	*check, *block;
	vec3_t		mins, maxs;
	vec3_t		pushorig;
	int			num_moved;
	hledict_t	*moved_edict[MAX_PUSHED_ENTITIES];
	vec3_t		moved_from[MAX_PUSHED_ENTITIES];
	float oldsolid;

	if (amove[0] || amove[1] || amove[2])
	{
		return SVHL_PushAngles(pusher, move, amove);
	}

	for (i=0 ; i<3 ; i++)
	{
		mins[i] = pusher->v.absmin[i] + move[i];
		maxs[i] = pusher->v.absmax[i] + move[i];
	}

	VectorCopy (pusher->v.origin, pushorig);

// move the pusher to it's final position

	VectorAdd (pusher->v.origin, move, pusher->v.origin);
	SVHL_LinkEdict (pusher, false);

// see if any solid entities are inside the final position
	num_moved = 0;
	for (e=1 ; e<SVHL_NumActiveEnts ; e++)
	{
		check = &SVHL_Edict[e];
		if (check->isfree)
			continue;
		if (check->v.movetype == MOVETYPE_PUSH
		|| check->v.movetype == MOVETYPE_NONE
		|| check->v.movetype == MOVETYPE_FOLLOW
		|| check->v.movetype == MOVETYPE_NOCLIP)
			continue;

		oldsolid = pusher->v.solid;
		pusher->v.solid = SOLID_NOT;
		block = SVHL_TestEntityPosition (check);
		pusher->v.solid = oldsolid;
		if (block)
			continue;

	// if the entity is standing on the pusher, it will definately be moved
		if ( ! ( ((int)check->v.flags & FL_ONGROUND)
		&&
			check->v.groundentity == pusher) )
		{
			if ( check->v.absmin[0] >= maxs[0]
			|| check->v.absmin[1] >= maxs[1]
			|| check->v.absmin[2] >= maxs[2]
			|| check->v.absmax[0] <= mins[0]
			|| check->v.absmax[1] <= mins[1]
			|| check->v.absmax[2] <= mins[2] )
				continue;

		// see if the ent's bbox is inside the pusher's final position
			if (!SVHL_TestEntityPosition (check))
				continue;
		}

		if (num_moved < MAX_PUSHED_ENTITIES)
		{
			VectorCopy (check->v.origin, moved_from[num_moved]);
			moved_edict[num_moved] = check;
			num_moved++;

	//		check->v.flags = (int)check->v.flags & ~FL_ONGROUND;

			// try moving the contacted entity
			VectorAdd (check->v.origin, move, check->v.origin);
			block = SVHL_TestEntityPosition (check);
			if (!block)
			{	// pushed ok
				SVHL_LinkEdict (check, false);
				continue;
			}

			// if it is ok to leave in the old position, do it
			VectorSubtract (check->v.origin, move, check->v.origin);
			block = SVHL_TestEntityPosition (check);
			if (!block)
			{
				//if leaving it where it was, allow it to drop to the floor again (useful for plats that move downward)
				check->v.flags = (int)check->v.flags & ~FL_ONGROUND;

				num_moved--;
				continue;
			}

			// if it is still inside the pusher, block
			if (check->v.mins[0] == check->v.maxs[0])
			{
				SVHL_LinkEdict (check, false);
				continue;
			}
			if (check->v.solid == SOLID_NOT || check->v.solid == SOLID_TRIGGER)
			{	// corpse
				check->v.mins[0] = check->v.mins[1] = 0;
				VectorCopy (check->v.mins, check->v.maxs);
				SVHL_LinkEdict (check, false);
				continue;
			}
		}

		VectorCopy (pushorig, pusher->v.origin);
		SVHL_LinkEdict (pusher, false);

		// if the pusher has a "blocked" function, call it
		// otherwise, just stay in place until the obstacle is gone
		SVHL_GameFuncs.DispatchBlocked(pusher, check);

	// move back any entities we already moved
		for (i=0 ; i<num_moved ; i++)
		{
			VectorCopy (moved_from[i], moved_edict[i]->v.origin);
			SVHL_LinkEdict (moved_edict[i], false);
		}
		return false;
	}

	return true;
}


/*
============
SV_PushMove

============
*/
void SVHL_PushMove (hledict_t *pusher, float movetime)
{
	int			i;
	vec3_t		move;
	vec3_t		amove;

	if (!pusher->v.velocity[0] && !pusher->v.velocity[1] && !pusher->v.velocity[2]
		&& !pusher->v.avelocity[0] && !pusher->v.avelocity[1] && !pusher->v.avelocity[2])
	{
		pusher->v.ltime += movetime;
		return;
	}

	for (i=0 ; i<3 ; i++)
	{
		move[i] = pusher->v.velocity[i] * movetime;
		amove[i] = pusher->v.avelocity[i] * movetime;
	}

	if (SVHL_Push (pusher, move, amove))
		pusher->v.ltime += movetime;
}


/*
================
SV_Physics_Pusher

================
*/
void SVHL_Physics_Pusher (hledict_t *ent)
{
	float	thinktime;
	float	oldltime;
	float	movetime;
vec3_t oldorg, move;
vec3_t oldang, amove;
float	l;

	oldltime = ent->v.ltime;

	thinktime = ent->v.nextthink;
	if (thinktime < ent->v.ltime + host_frametime)
	{
		movetime = thinktime - ent->v.ltime;
		if (movetime < 0)
			movetime = 0;
	}
	else
		movetime = host_frametime;

	if (movetime)
	{
		SVHL_PushMove (ent, movetime);	// advances ent->v.ltime if not blocked
	}

	if (thinktime > oldltime && thinktime <= ent->v.ltime)
	{
		VectorCopy (ent->v.origin, oldorg);
		VectorCopy (ent->v.angles, oldang);

		ent->v.nextthink = 0;
		SVHL_Globals.time = sv.world.physicstime;
		SVHL_GameFuncs.DispatchThink(ent);
		if (ent->isfree)
			return;

		VectorSubtract (ent->v.origin, oldorg, move);
		VectorSubtract (ent->v.angles, oldang, amove);

		l = Length(move)+Length(amove);
		if (l > 1.0/64)
		{
		//	Con_Printf ("**** snap: %f\n", Length (l));
			VectorCopy (oldorg, ent->v.origin);
			SVHL_Push (ent, move, amove);
		}

	}
	else if (ent->v.flags & (1<<21))
	{
		ent->v.nextthink = 0;
		SVHL_Globals.time = sv.world.physicstime;
		SVHL_GameFuncs.DispatchThink(ent);
		if (ent->isfree)
			return;
	}

}


/*
=============
SV_Physics_Follow

Entities that are "stuck" to another entity
=============
*/
void SVHL_Physics_Follow (hledict_t *ent)
{
//	vec3_t vf, vr, vu, angles, v;
	hledict_t *e;

	// regular thinking
	if (!SVHL_RunThink (ent))
		return;

	// LordHavoc: implemented rotation on MOVETYPE_FOLLOW objects
	e = ent->v.aiment;
//	if (e->v.angles[0] == ent->xv->punchangle[0] && e->v.angles[1] == ent->xv->punchangle[1] && e->v.angles[2] == ent->xv->punchangle[2])
	{
		// quick case for no rotation
		VectorAdd(e->v.origin, ent->v.view_ofs, ent->v.origin);
	}
/*	else
	{
		angles[0] = -ent->xv->punchangle[0];
		angles[1] =  ent->xv->punchangle[1];
		angles[2] =  ent->xv->punchangle[2];
		AngleVectors (angles, vf, vr, vu);
		v[0] = ent->v.view_ofs[0] * vf[0] + ent->v.view_ofs[1] * vr[0] + ent->v.view_ofs[2] * vu[0];
		v[1] = ent->v.view_ofs[0] * vf[1] + ent->v.view_ofs[1] * vr[1] + ent->v.view_ofs[2] * vu[1];
		v[2] = ent->v.view_ofs[0] * vf[2] + ent->v.view_ofs[1] * vr[2] + ent->v.view_ofs[2] * vu[2];
		angles[0] = -e->v.angles[0];
		angles[1] =  e->v.angles[1];
		angles[2] =  e->v.angles[2];
		AngleVectors (angles, vf, vr, vu);
		ent->v.origin[0] = v[0] * vf[0] + v[1] * vf[1] + v[2] * vf[2] + e->v.origin[0];
		ent->v.origin[1] = v[0] * vr[0] + v[1] * vr[1] + v[2] * vr[2] + e->v.origin[1];
		ent->v.origin[2] = v[0] * vu[0] + v[1] * vu[1] + v[2] * vu[2] + e->v.origin[2];
	}
*/	VectorAdd (e->v.angles, ent->v.v_angle, ent->v.angles);
	SVHL_LinkEdict (ent, true);
}

/*
=============
SV_Physics_Noclip

A moving object that doesn't obey physics
=============
*/
void SVHL_Physics_Noclip (hledict_t *ent)
{
// regular thinking
	if (!SVHL_RunThink (ent))
		return;

	VectorMA (ent->v.angles, host_frametime, ent->v.avelocity, ent->v.angles);
	VectorMA (ent->v.origin, host_frametime, ent->v.velocity, ent->v.origin);

	SVHL_LinkEdict (ent, false);
}

/*
==============================================================================

TOSS / BOUNCE

==============================================================================
*/

/*
=============
SV_CheckWaterTransition

=============
*/
void SVHL_CheckWaterTransition (hledict_t *ent)
{
	int		cont;

	cont = SVHL_PointContents (ent->v.origin);

	//needs to be q1 progs compatible
	if (cont & FTECONTENTS_LAVA)
		cont = Q1CONTENTS_LAVA;
	else if (cont & FTECONTENTS_SLIME)
		cont = Q1CONTENTS_SLIME;
	else if (cont & FTECONTENTS_WATER)
		cont = Q1CONTENTS_WATER;
	else
		cont = Q1CONTENTS_EMPTY;

	if (!ent->v.watertype)
	{	// just spawned here
		ent->v.watertype = cont;
		ent->v.waterlevel = 1;
		return;
	}

	if (cont <= Q1CONTENTS_WATER)
	{
		if (ent->v.watertype == Q1CONTENTS_EMPTY)
		{	// just crossed into water
			SVHL_StartSound (ent, 0, "misc/h2ohit1.wav", 255, 1);
		}
		ent->v.watertype = cont;
		ent->v.waterlevel = 1;
	}
	else
	{
		if (ent->v.watertype != Q1CONTENTS_EMPTY)
		{	// just crossed into open
			SVHL_StartSound (ent, 0, "misc/h2ohit1.wav", 255, 1);
		}
		ent->v.watertype = Q1CONTENTS_EMPTY;
		ent->v.waterlevel = cont;
	}
}

/*
=============
SV_Physics_Toss

Toss, bounce, and fly movement.  When onground, do nothing.
=============
*/
void SVHL_Physics_Toss (hledict_t *ent)
{
	trace_t	trace;
	vec3_t	move;
	float	backoff;

	vec3_t temporg;

	SVHL_CheckVelocity (ent);

// regular thinking
	if (!SVHL_RunThink (ent))
		return;

// if onground, return without moving
	if ( ((int)ent->v.flags & FL_ONGROUND) )
	{
		if (ent->v.velocity[2] >= (1.0f/32.0f))
			ent->v.flags = (int)ent->v.flags & ~FL_ONGROUND;
		else
		{
			if (sv_gameplayfix_noairborncorpse.value)
			{
				hledict_t *onent;
				onent = ent->v.groundentity;
				if (!onent->isfree)
					return;	//don't drop if our fround is still valid
			}
			else
				return;	//don't drop, even if the item we were on was removed (certain dm maps do this for q3 style stuff).
		}
	}

// add gravity
	if (ent->v.movetype != MOVETYPE_FLY
		&& ent->v.movetype != MOVETYPE_FLYMISSILE
		&& ent->v.movetype != MOVETYPE_BOUNCEMISSILE
		&& ent->v.movetype != MOVETYPE_H2SWIM)
		SVHL_AddGravity (ent, 1.0);

// move angles
	VectorMA (ent->v.angles, host_frametime, ent->v.avelocity, ent->v.angles);

// move origin
	VectorScale (ent->v.velocity, host_frametime, move);
	VectorCopy(ent->v.origin, temporg);
	VectorCopy(temporg, ent->v.origin);
	trace = SVHL_PushEntity (ent, move);

	if (trace.allsolid)
		trace.fraction = 0;
	if (trace.fraction == 1)
		return;
	if (ent->isfree)
		return;

	VectorCopy(trace.endpos, move);

	if (ent->v.movetype == MOVETYPE_BOUNCE)
		backoff = 1.5;
	else if (ent->v.movetype == MOVETYPE_BOUNCEMISSILE)
		backoff = 2;
	else
		backoff = 1;

	ClipVelocity (ent->v.velocity, trace.plane.normal, ent->v.velocity, backoff);


// stop if on ground
	if ((trace.plane.normal[2] > 0.7) && (ent->v.movetype != MOVETYPE_BOUNCEMISSILE))
	{
		if (ent->v.velocity[2] < 60 || ent->v.movetype != MOVETYPE_BOUNCE )
		{
			ent->v.flags = (int)ent->v.flags | FL_ONGROUND;
			ent->v.groundentity = trace.ent;
			VectorClear (ent->v.velocity);
			VectorClear (ent->v.avelocity);
		}
	}

// check for in water
	SVHL_CheckWaterTransition (ent);
}

/*
===============================================================================

STEPPING MOVEMENT

===============================================================================
*/

/*
=============
SV_Physics_Step

Monsters freefall when they don't have a ground entity, otherwise
all movement is done with discrete steps.

This is also used for objects that have become still on the ground, but
will fall if the floor is pulled out from under them.
FIXME: is this true?
=============
*/
void SVHL_Physics_Step (hledict_t *ent)
{
	qboolean	hitsound;

	if (ent->v.velocity[2] >= (1.0 / 32.0) && ((int)ent->v.flags & FL_ONGROUND))
		ent->v.flags = (int)ent->v.flags & ~FL_ONGROUND;

// frefall if not onground
	if ( ! ((int)ent->v.flags & (FL_ONGROUND | FL_FLY | FL_SWIM) ) )
	{
		hitsound = ent->v.velocity[2] < movevars.gravity*-0.1;

		SVHL_AddGravity (ent, 1.0);
		SVHL_CheckVelocity (ent);
		SVHL_FlyMove (ent, host_frametime, NULL);
		SVHL_LinkEdict (ent, true);

		if ( (int)ent->v.flags & FL_ONGROUND )	// just hit ground
		{
			if (hitsound)
			{
				SVHL_StartSound (ent, 0, "demon/dland2.wav", 255, 1);
			}
		}
	}

// regular thinking
	SVHL_RunThink (ent);

	SVHL_CheckWaterTransition (ent);
}

//============================================================================













/*
=============
SV_CheckStuck

This is a big hack to try and fix the rare case of getting stuck in the world
clipping hull.
=============
*/
void SVHL_CheckStuck (hledict_t *ent)
{
	int		i, j;
	int		z;
	vec3_t	org;
//return;
	if (!SVHL_TestEntityPosition(ent))
	{
		VectorCopy (ent->v.origin, ent->v.oldorigin);
		return;
	}

	VectorCopy (ent->v.origin, org);
	VectorCopy (ent->v.oldorigin, ent->v.origin);
	if (!SVHL_TestEntityPosition(ent))
	{
		Con_DPrintf ("Unstuck.\n");
		SVHL_LinkEdict (ent, true);
		return;
	}

	for (z=0 ; z < movevars.stepheight ; z++)
		for (i=-1 ; i <= 1 ; i++)
			for (j=-1 ; j <= 1 ; j++)
			{
				ent->v.origin[0] = org[0] + i;
				ent->v.origin[1] = org[1] + j;
				ent->v.origin[2] = org[2] + z;
				if (!SVHL_TestEntityPosition(ent))
				{
					Con_DPrintf ("Unstuck.\n");
					SVHL_LinkEdict (ent, true);
					return;
				}
			}

	VectorCopy (org, ent->v.origin);
	Con_DPrintf ("player is stuck.\n");
}

/*
=============
SV_CheckWater
=============
*/
qboolean SVHL_CheckWater (hledict_t *ent)
{
	vec3_t	point;
	int		cont;

	point[0] = ent->v.origin[0];
	point[1] = ent->v.origin[1];
	point[2] = ent->v.origin[2] + ent->v.mins[2] + 1;

	ent->v.waterlevel = 0;
	ent->v.watertype = Q1CONTENTS_EMPTY;
	cont = SVHL_PointContents (point);
	if (cont & FTECONTENTS_FLUID)
	{
		if (cont & FTECONTENTS_LAVA)
			ent->v.watertype = Q1CONTENTS_LAVA;
		else if (cont & FTECONTENTS_SLIME)
			ent->v.watertype = Q1CONTENTS_SLIME;
		else if (cont & FTECONTENTS_WATER)
			ent->v.watertype = Q1CONTENTS_WATER;
		else
			ent->v.watertype = Q1CONTENTS_SKY;
		ent->v.waterlevel = 1;
		point[2] = ent->v.origin[2] + (ent->v.mins[2] + ent->v.maxs[2])*0.5;
		cont = SVHL_PointContents (point);
		if (cont & FTECONTENTS_FLUID)
		{
			ent->v.waterlevel = 2;
			point[2] = ent->v.origin[2] + ent->v.view_ofs[2];
			cont = SVHL_PointContents (point);
			if (cont & FTECONTENTS_FLUID)
				ent->v.waterlevel = 3;
		}
	}

	return ent->v.waterlevel > 1;
}


/*
============
SV_WallFriction

============
*/
void SVHL_WallFriction (hledict_t *ent, trace_t *trace)
{
	vec3_t		forward, right, up;
	float		d, i;
	vec3_t		into, side;

	AngleVectors (ent->v.v_angle, forward, right, up);
	d = DotProduct (trace->plane.normal, forward);

	d += 0.5;
	if (d >= 0 || IS_NAN(d))
		return;

// cut the tangential velocity
	i = DotProduct (trace->plane.normal, ent->v.velocity);
	VectorScale (trace->plane.normal, i, into);
	VectorSubtract (ent->v.velocity, into, side);

	ent->v.velocity[0] = side[0] * (1 + d);
	ent->v.velocity[1] = side[1] * (1 + d);
}

/*
=====================
SV_TryUnstick

Player has come to a dead stop, possibly due to the problem with limited
float precision at some angle joins in the BSP hull.

Try fixing by pushing one pixel in each direction.

This is a hack, but in the interest of good gameplay...
======================
*/
int SVHL_TryUnstick (hledict_t *ent, vec3_t oldvel)
{
	int		i;
	vec3_t	oldorg;
	vec3_t	dir;
	int		clip;
	trace_t	steptrace;

	VectorCopy (ent->v.origin, oldorg);
	VectorClear (dir);

	for (i=0 ; i<8 ; i++)
	{
// try pushing a little in an axial direction
		switch (i)
		{
			case 0:	dir[0] = 2; dir[1] = 0; break;
			case 1:	dir[0] = 0; dir[1] = 2; break;
			case 2:	dir[0] = -2; dir[1] = 0; break;
			case 3:	dir[0] = 0; dir[1] = -2; break;
			case 4:	dir[0] = 2; dir[1] = 2; break;
			case 5:	dir[0] = -2; dir[1] = 2; break;
			case 6:	dir[0] = 2; dir[1] = -2; break;
			case 7:	dir[0] = -2; dir[1] = -2; break;
		}

		SVHL_PushEntity (ent, dir);

// retry the original move
		ent->v.velocity[0] = oldvel[0];
		ent->v. velocity[1] = oldvel[1];
		ent->v. velocity[2] = 0;
		clip = SVHL_FlyMove (ent, 0.1, &steptrace);

		if ( fabs(oldorg[1] - ent->v.origin[1]) > 4
		|| fabs(oldorg[0] - ent->v.origin[0]) > 4 )
		{
//Con_DPrintf ("unstuck!\n");
			return clip;
		}

// go back to the original pos and try again
		VectorCopy (oldorg, ent->v.origin);
	}

	VectorClear (ent->v.velocity);
	return 7;		// still not moving
}

/*
=====================
SV_WalkMove

Only used by players
======================
*/
#if 0
#define	SMSTEPSIZE	4
void SVHL_WalkMove (hledict_t *ent)
{
	vec3_t		upmove, downmove;
	vec3_t		oldorg, oldvel;
	vec3_t		nosteporg, nostepvel;
	int			clip;
	int			oldonground;
	trace_t		steptrace, downtrace;

//
// do a regular slide move unless it looks like you ran into a step
//
	oldonground = (int)ent->v.flags & FL_ONGROUND;
	ent->v.flags = (int)ent->v.flags & ~FL_ONGROUND;

	VectorCopy (ent->v.origin, oldorg);
	VectorCopy (ent->v.velocity, oldvel);

	clip = SVHL_FlyMove (ent, host_frametime, &steptrace);

	if ( !(clip & 2) )
		return;		// move didn't block on a step

	if (!oldonground && ent->v.waterlevel == 0)
		return;		// don't stair up while jumping

	if (ent->v.movetype != MOVETYPE_WALK)
		return;		// gibbed by a trigger

//	if (sv_nostep.value)
//		return;

	if ( (int)ent->v.flags & FL_WATERJUMP )
		return;

	VectorCopy (ent->v.origin, nosteporg);
	VectorCopy (ent->v.velocity, nostepvel);

//
// try moving up and forward to go up a step
//
	VectorCopy (oldorg, ent->v.origin);	// back to start pos

	VectorCopy (vec3_origin, upmove);
	VectorCopy (vec3_origin, downmove);
	upmove[2] = movevars.stepheight;
	downmove[2] = -movevars.stepheight + oldvel[2]*host_frametime;

// move up
	SVHL_PushEntity (ent, upmove);	// FIXME: don't link?

// move forward
	ent->v.velocity[0] = oldvel[0];
	ent->v.velocity[1] = oldvel[1];
	ent->v.velocity[2] = 0;
	clip = SVHL_FlyMove (ent, host_frametime, &steptrace);

// check for stuckness, possibly due to the limited precision of floats
// in the clipping hulls
	if (clip)
	{
		if ( fabs(oldorg[1] - ent->v.origin[1]) < 0.03125
		&& fabs(oldorg[0] - ent->v.origin[0]) < 0.03125 )
		{	// stepping up didn't make any progress
			clip = SVHL_TryUnstick (ent, oldvel);

//			Con_Printf("Try unstick fwd\n");
		}
	}

// extra friction based on view angle
	if ( clip & 2 )
	{
		vec3_t lastpos, lastvel, lastdown;

//		Con_Printf("couldn't do it\n");

		//retry with a smaller step (allows entering smaller areas with a step of 4)
		VectorCopy (downmove, lastdown);
		VectorCopy (ent->v.origin, lastpos);
		VectorCopy (ent->v.velocity, lastvel);

	//
	// try moving up and forward to go up a step
	//
		VectorCopy (oldorg, ent->v.origin);	// back to start pos

		VectorCopy (vec3_origin, upmove);
		VectorCopy (vec3_origin, downmove);
		upmove[2] = SMSTEPSIZE;
		downmove[2] = -SMSTEPSIZE + oldvel[2]*host_frametime;

	// move up
		SVHL_PushEntity (ent, upmove);	// FIXME: don't link?

	// move forward
		ent->v.velocity[0] = oldvel[0];
		ent->v.velocity[1] = oldvel[1];
		ent->v.velocity[2] = 0;
		clip = SVHL_FlyMove (ent, host_frametime, &steptrace);

	// check for stuckness, possibly due to the limited precision of floats
	// in the clipping hulls
		if (clip)
		{
			if ( fabs(oldorg[1] - ent->v.origin[1]) < 0.03125
			&& fabs(oldorg[0] - ent->v.origin[0]) < 0.03125 )
			{	// stepping up didn't make any progress
				clip = SVHL_TryUnstick (ent, oldvel);

//				Con_Printf("Try unstick up\n");
			}
		}

		if ( fabs(oldorg[1] - ent->v.origin[1])+fabs(oldorg[0] - ent->v.origin[0]) < fabs(oldorg[1] - lastpos[1])+fabs(oldorg[1] - lastpos[1]))
		{	// stepping up didn't make any progress
				//go back
				VectorCopy (lastdown, downmove);
				VectorCopy (lastpos, ent->v.origin);
				VectorCopy (lastvel, ent->v.velocity);

				SVHL_WallFriction (ent, &steptrace);

//				Con_Printf("wall friction\n");
			}

		else if (clip & 2)
		{
			SVHL_WallFriction (ent, &steptrace);
//			Con_Printf("wall friction 2\n");
		}
	}

// move down
	downtrace = SVHL_PushEntity (ent, downmove);	// FIXME: don't link?

	if (downtrace.plane.normal[2] > 0.7)
	{
		if (ent->v.solid == SOLID_BSP)
		{
			ent->v.flags =	(int)ent->v.flags | FL_ONGROUND;
			ent->v.groundentity = EDICT_TO_PROG(svprogfuncs, downtrace.ent);
		}
	}
	else
	{
// if the push down didn't end up on good ground, use the move without
// the step up.  This happens near wall / slope combinations, and can
// cause the player to hop up higher on a slope too steep to climb
		VectorCopy (nosteporg, ent->v.origin);
		VectorCopy (nostepvel, ent->v.velocity);

//		Con_Printf("down not good\n");
	}
}
#else

// 1/32 epsilon to keep floating point happy
#define	DIST_EPSILON	(0.03125)
int SVHL_SetOnGround (hledict_t *ent)
{
	vec3_t end;
	trace_t trace;
	if ((int)ent->v.flags & FL_ONGROUND)
		return 1;
	end[0] = ent->v.origin[0];
	end[1] = ent->v.origin[1];
	end[2] = ent->v.origin[2] - 1;
	trace = SVHL_Move(ent->v.origin, ent->v.mins, ent->v.maxs, end, MOVE_NORMAL, 0, ent);
	if (trace.fraction <= DIST_EPSILON && trace.plane.normal[2] >= 0.7)
	{
		ent->v.flags = (int)ent->v.flags | FL_ONGROUND;
		ent->v.groundentity = trace.ent;
		return 1;
	}
	return 0;
}
void SVHL_WalkMove (hledict_t *ent)
{
	int clip, oldonground, originalmove_clip, originalmove_flags;
	hledict_t *originalmove_groundentity;
	vec3_t upmove, downmove, start_origin, start_velocity, originalmove_origin, originalmove_velocity;
	trace_t downtrace, steptrace;

	SVHL_CheckVelocity(ent);

	// do a regular slide move unless it looks like you ran into a step
	oldonground = (int)ent->v.flags & FL_ONGROUND;
	ent->v.flags = (int)ent->v.flags & ~FL_ONGROUND;

	VectorCopy (ent->v.origin, start_origin);
	VectorCopy (ent->v.velocity, start_velocity);

	clip = SVHL_FlyMove (ent, host_frametime, NULL);

	SVHL_SetOnGround (ent);
	SVHL_CheckVelocity(ent);

	VectorCopy(ent->v.origin, originalmove_origin);
	VectorCopy(ent->v.velocity, originalmove_velocity);
	originalmove_clip = clip;
	originalmove_flags = (int)ent->v.flags;
	originalmove_groundentity = ent->v.groundentity;

	if ((int)ent->v.flags & FL_WATERJUMP)
		return;

//	if (sv_nostep.value)
//		return;

	// if move didn't block on a step, return
	if (clip & 2)
	{
		// if move was not trying to move into the step, return
		if (fabs(start_velocity[0]) < 0.03125 && fabs(start_velocity[1]) < 0.03125)
			return;

		if (ent->v.movetype != MOVETYPE_FLY)
		{
			// return if gibbed by a trigger
			if (ent->v.movetype != MOVETYPE_WALK)
				return;

			// only step up while jumping if that is enabled
//			if (!(sv_jumpstep.value && sv_gameplayfix_stepwhilejumping.value))
				if (!oldonground && ent->v.waterlevel == 0)
					return;
		}

		// try moving up and forward to go up a step
		// back to start pos
		VectorCopy (start_origin, ent->v.origin);
		VectorCopy (start_velocity, ent->v.velocity);

		// move up
		VectorClear (upmove);
		upmove[2] = movevars.stepheight;
		// FIXME: don't link?
		SVHL_PushEntity(ent, upmove);

		// move forward
		ent->v.velocity[2] = 0;
		clip = SVHL_FlyMove (ent, host_frametime, &steptrace);
		ent->v.velocity[2] += start_velocity[2];

		SVHL_CheckVelocity(ent);

		// check for stuckness, possibly due to the limited precision of floats
		// in the clipping hulls
		if (clip
		 && fabs(originalmove_origin[1] - ent->v.origin[1]) < 0.03125
		 && fabs(originalmove_origin[0] - ent->v.origin[0]) < 0.03125)
		{
//			Con_Printf("wall\n");
			// stepping up didn't make any progress, revert to original move
			VectorCopy(originalmove_origin, ent->v.origin);
			VectorCopy(originalmove_velocity, ent->v.velocity);
			//clip = originalmove_clip;
			ent->v.flags = originalmove_flags;
			ent->v.groundentity = originalmove_groundentity;
			// now try to unstick if needed
			//clip = SVHL_TryUnstick (ent, oldvel);
			return;
		}

		//Con_Printf("step - ");

		// extra friction based on view angle
		if (clip & 2)// && sv_wallfriction.value)
		{
//			Con_Printf("wall\n");
			SVHL_WallFriction (ent, &steptrace);
		}
	}
	else if (/*!sv_gameplayfix_stepdown.integer || */!oldonground || start_velocity[2] > 0 || ((int)ent->v.flags & FL_ONGROUND) || ent->v.waterlevel >= 2)
		return;

	// move down
	VectorClear (downmove);
	downmove[2] = -movevars.stepheight + start_velocity[2]*host_frametime;
	// FIXME: don't link?
	downtrace = SVHL_PushEntity (ent, downmove);

	if (downtrace.fraction < 1 && downtrace.plane.normal[2] > 0.7)
	{
		// LordHavoc: disabled this check so you can walk on monsters/players
		//if (ent->v.solid == SOLID_BSP)
		{
			//Con_Printf("onground\n");
			ent->v.flags =	(int)ent->v.flags | FL_ONGROUND;
			ent->v.groundentity = downtrace.ent;
		}
	}
	else
	{
		//Con_Printf("slope\n");
		// if the push down didn't end up on good ground, use the move without
		// the step up.  This happens near wall / slope combinations, and can
		// cause the player to hop up higher on a slope too steep to climb
		VectorCopy(originalmove_origin, ent->v.origin);
		VectorCopy(originalmove_velocity, ent->v.velocity);
		//clip = originalmove_clip;
		ent->v.flags = originalmove_flags;
		ent->v.groundentity = originalmove_groundentity;
	}

	SVHL_SetOnGround (ent);
	SVHL_CheckVelocity(ent);
}
#endif

/*
================
SV_RunEntity

================
*/
void SVHL_RunEntity (hledict_t *ent)
{
	if (ent-SVHL_Edict > 0 && ent-SVHL_Edict <= sv.allocated_client_slots)
	{	//a client woo.
	/*	if ( svs.clients[ent->entnum-1].state < cs_spawned )
			return;		// unconnected slot


		if (svs.clients[ent->entnum-1].protocol == SCP_BAD)
			svs.clients[ent->entnum-1].edict->v.fixangle = 0;	//bots never get fixangle cleared otherwise

		host_client = &svs.clients[ent->entnum-1];
		SVHL_ClientThink();

	//
	// call standard client pre-think
	//
		pr_global_struct->time = sv.physicstime;
		pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, ent);
#ifdef VM_Q1
		if (svs.gametype == GT_Q1QVM)
			Q1QVM_PlayerPreThink();
		else
#endif
			if (pr_global_struct->PlayerPreThink)
				PR_ExecuteProgram (svprogfuncs, pr_global_struct->PlayerPreThink);

			*/
	}

	if (ent->v.flags & (1<<30))
	{
		GHL_RemoveEntity(ent);
		return;
	}

	switch ( (int)ent->v.movetype)
	{
	case MOVETYPE_PUSH:
		SVHL_Physics_Pusher (ent);
		break;
	case MOVETYPE_NONE:
		if (!SVHL_RunThink (ent))
			return;
		break;
	case MOVETYPE_NOCLIP:
		SVHL_Physics_Noclip (ent);
		break;
	case MOVETYPE_STEP:
	case MOVETYPE_H2PUSHPULL:
		SVHL_Physics_Step (ent);
		break;
	case MOVETYPE_FOLLOW:
		SVHL_Physics_Follow (ent);
		break;
	case MOVETYPE_TOSS:
	case MOVETYPE_BOUNCE:
	case MOVETYPE_BOUNCEMISSILE:
	case MOVETYPE_FLY:
	case MOVETYPE_FLYMISSILE:
	case MOVETYPE_H2SWIM:
		SVHL_Physics_Toss (ent);
		break;
	case MOVETYPE_WALK:
		if (!SVHL_RunThink (ent))
			return;
		if (!SVHL_CheckWater (ent) && ! ((int)ent->v.flags & FL_WATERJUMP) )
			SVHL_AddGravity (ent, ent->v.gravity);
		SVHL_CheckStuck (ent);

		SVHL_WalkMove (ent);

//		if (!(ent->entnum > 0 && ent->entnum <= sv.allocated_client_slots))
			SVHL_LinkEdict (ent, true);

		break;
	default:
		SV_Error ("SV_Physics: bad movetype %i on %s", (int)ent->v.movetype, SVHL_Globals.stringbase+ent->v.classname);
	}

	/*
	if (ent->entnum > 0 && ent->entnum <= sv.allocated_client_slots)
	{
		SVhL_LinkEdict (ent, true);

		pr_global_struct->time = sv.physicstime;
		pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, ent);
#ifdef VM_Q1
		if (svs.gametype == GT_Q1QVM)
			Q1QVM_PostThink();
		else
#endif
		{
			if (pr_global_struct->PlayerPostThink)
				PR_ExecuteProgram (svprogfuncs, pr_global_struct->PlayerPostThink);
		}
	}
	*/
}

trace_t SVHL_Trace_Toss (hledict_t *tossent, hledict_t *ignore)
{
	int i;
	float gravity;
	vec3_t move, end;
	trace_t trace;

	vec3_t origin, velocity;

	// this has to fetch the field from the original edict, since our copy is truncated
	gravity = tossent->v.gravity;
	if (!gravity)
		gravity = 1.0;
	gravity *= sv_gravity.value * 0.05;

	VectorCopy (tossent->v.origin, origin);
	VectorCopy (tossent->v.velocity, velocity);

	SVHL_CheckVelocity (tossent);

	for (i = 0;i < 200;i++) // LordHavoc: sanity check; never trace more than 10 seconds
	{
		velocity[2] -= gravity;
		VectorScale (velocity, 0.05, move);
		VectorAdd (origin, move, end);
		trace = SVHL_Move (origin, tossent->v.mins, tossent->v.maxs, end, MOVE_NORMAL, 0, tossent);
		VectorCopy (trace.endpos, origin);

		if (trace.fraction < 1 && trace.ent && trace.ent != ignore)
			break;

		if (Length(velocity) > sv_maxvelocity.value)
		{
//			Con_DPrintf("Slowing %s\n", PR_GetString(svprogfuncs, tossent->v.classname));
			VectorScale (velocity, sv_maxvelocity.value/Length(velocity), velocity);
		}
	}

	trace.fraction = 0; // not relevant
	return trace;
}

/*
================
SV_Physics

================
*/
void SVHL_RunFrame (void)
{
	qboolean retouch;
	int i;
	hledict_t *ent;

	//only run physics tics if there's a client on the server.
	//this fixes the bug where the train moves out before the player spawns, so the player doesn't fall to his death
	if (sv.state == ss_active)
	{
		for (i = 0; i < sv.allocated_client_slots; i++)
		{
			if (svs.clients[i].state == cs_spawned)
				break;
		}
		if (i == sv.allocated_client_slots)
			return;
	}


	SVHL_Globals.frametime = host_frametime;
	SVHL_Globals.time = sv.world.physicstime;

	SVHL_GameFuncs.StartFrame ();


	retouch = SVHL_Globals.force_retouch;

//
// treat each object in turn
// even the world gets a chance to think
//
	for (i=0 ; i<SVHL_NumActiveEnts ; i++)
	{
		ent = &SVHL_Edict[i];
		if (ent->isfree)
			continue;

		if (retouch)
			SVHL_LinkEdict (ent, true);	// force retouch even for stationary

		if (i > 0 && i <= sv.allocated_client_slots)
		{
			if (!svs.clients[i-1].isindependant)
			{
			//	SVHL_RunEntity(ent);
			}
			else
				SVHL_LinkEdict(ent, true);
			continue;		// clients are run directly from packets
		}

		SVHL_RunEntity (ent);
	}

	if (retouch)
		SVHL_Globals.force_retouch-=1;
}

#endif
