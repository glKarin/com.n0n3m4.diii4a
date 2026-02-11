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
#include "pr_common.h"

#if defined(CSQC_DAT) || !defined(CLIENTONLY)
/*
=============
SV_CheckBottom

Returns false if any part of the bottom of the entity is off an edge that
is not a staircase.

=============
*/
//int c_yes, c_no;

hull_t *Q1BSP_ChooseHull(model_t *model, int hullnum, vec3_t mins, vec3_t maxs, vec3_t offset);

//this function is axial. major axis determines ground. if it switches slightly, a new axis may become the ground...
qboolean World_CheckBottom (world_t *world, wedict_t *ent, vec3_t up)
{
	vec3_t	mins, maxs, start, stop;
	trace_t	trace;
	int		x, y;
	float	mid;

	int a0,a1,a2;	//logical x, y, z
	int sign;

	mins[0] = fabs(up[0]);
	mins[1] = fabs(up[1]);
	mins[2] = fabs(up[2]);
	if (mins[2] > mins[0] && mins[2] > mins[1])
	{
		a0 = 0;
		a1 = 1;
		a2 = 2;
	}
	else
	{
		a2 = mins[1] > mins[0];
		a0 = 1 - a2;
		a1 = 2;
	}
	sign = (up[a2]>0)?1:-1;
	
	VectorAdd (ent->v->origin, ent->v->mins, mins);
#ifdef Q1BSPS
	if (world->worldmodel->fromgame == fg_quake || world->worldmodel->fromgame == fg_halflife)
	{
		//quake's hulls are weird. sizes are defined as from mins to mins+hullsize. the actual maxs is ignored other than for its size.
		hull_t *hull;
		hull = Q1BSP_ChooseHull(world->worldmodel, ent->xv->hull, ent->v->mins, ent->v->maxs, start);
		//ignore the hull's offset. the minpoint is the minpoint. lets fix up the size though, just in case.
		VectorSubtract (mins, hull->clip_mins, maxs);
		VectorAdd (maxs, hull->clip_maxs, maxs);
	}
	else
#endif
		VectorAdd (ent->v->origin, ent->v->maxs, maxs);

// if all of the points under the corners are solid world, don't bother
// with the tougher checks
// the corners must be within 16 of the midpoint
	start[a2] = (sign<0)?maxs[a2]:mins[a2] - sign;
	for	(x=0 ; x<=1 ; x++)
		for	(y=0 ; y<=1 ; y++)
		{
			start[a0] = x ? maxs[a0] : mins[a0];
			start[a1] = y ? maxs[a1] : mins[a1];
			if (!(World_PointContentsWorldOnly (world, start) & FTECONTENTS_SOLID))
				goto realcheck;
		}

//	c_yes++;
	return true;		// we got out easy

realcheck:
//	c_no++;
//
// check it for real...
//
	start[a2] = (sign<0)?maxs[a2]:mins[a2];
	
// the midpoint must be within 16 of the bottom
	start[a0] = stop[a0] = (mins[a0] + maxs[a0])*0.5;
	start[a1] = stop[a1] = (mins[a1] + maxs[a1])*0.5;
	stop[a2] = start[a2] - 2*movevars.stepheight*sign;
	trace = World_Move (world, start, vec3_origin, vec3_origin, stop, true|MOVE_IGNOREHULL, ent);

	if (trace.fraction == 1.0)
		return false;

	mid = trace.endpos[2];

	mid = (mid-start[a2]-(movevars.stepheight*sign)) / (stop[a2]-start[a2]);
	
// the corners must be within 16 of the midpoint	
	for	(x=0 ; x<=1 ; x++)
		for	(y=0 ; y<=1 ; y++)
		{
			start[a0] = stop[a0] = x ? maxs[a0] : mins[a0];
			start[a1] = stop[a1] = y ? maxs[a1] : mins[a1];
			
			trace = World_Move (world, start, vec3_origin, vec3_origin, stop, true|MOVE_IGNOREHULL, ent);
	
			if (trace.fraction == 1.0 || trace.fraction > mid)//mid - trace.endpos[2] > movevars.stepheight)
				return false;
		}

//	c_yes++;
	return true;
}

/*
=============
SV_movestep

Called by monster program code.
The move will be adjusted for slopes and stairs, but if the move isn't
possible, no move is done, false is returned, and
pr_global_struct->trace_normal is set to the normal of the blocking wall
=============
*/
qboolean World_movestep (world_t *world, wedict_t *ent, vec3_t move, vec3_t axis[3], qboolean relink, qboolean noenemy, void (*set_move_trace)(pubprogfuncs_t *prinst, trace_t *trace))
{
	float		dz;
	vec3_t		oldorg, neworg, end;
	trace_t		trace;
	int			i;
	wedict_t	*enemy = world->edicts;
	int eflags = ent->v->flags;
	vec3_t		eaxis[3];

	if (!axis)
	{
		//fixme?
		World_GetEntGravityAxis(ent, eaxis);
		axis = eaxis;
	}

// try the move
	VectorCopy (ent->v->origin, oldorg);
	VectorAdd (ent->v->origin, move, neworg);

// flying monsters don't step up
	if ((eflags & (FL_SWIM | FL_FLY))
#if defined(HEXEN2) && defined(HAVE_SERVER)
			//hexen2 has some extra logic for FLH2_HUNTFACE, but its buggy and thus never used.
			//it would be nice to redefine the NOZ flag to instead force noenemy here, but that's not hexen2-compatible and FLH2_NOZ is bound to conflict with some quake mod.
			&& (world != &sv.world || progstype != PROG_H2 || !(eflags & (FLH2_NOZ|FLH2_HUNTFACE)))
#endif
			)
	{
	// try one move with vertical motion, then one without
		for (i=0 ; i<2 ; i++)
		{
			VectorAdd (ent->v->origin, move, neworg);
			if (!noenemy)
			{
				enemy = (wedict_t*)PROG_TO_EDICT(world->progs, ent->v->enemy);
				if (i == 0 && enemy->entnum)
				{
					VectorSubtract(ent->v->origin, ((wedict_t*)PROG_TO_EDICT(world->progs, ent->v->enemy))->v->origin, end);
					dz = DotProduct(end, axis[2]);
					if (dz > 40)
						VectorMA(neworg, -8, axis[2], neworg);
					if (dz < 30)
						VectorMA(neworg, 8, axis[2], neworg);
				}
			}
			trace = World_Move (world, ent->v->origin, ent->v->mins, ent->v->maxs, neworg, false, ent);
			if (set_move_trace)
				set_move_trace(world->progs, &trace);

			if (trace.fraction == 1)
			{
				if ( (eflags & FL_SWIM) && !(World_PointContentsWorldOnly(world, trace.endpos) & FTECONTENTS_FLUID))
					continue;	// swim monster left water
	
				VectorCopy (trace.endpos, ent->v->origin);
				if (relink)
					World_LinkEdict (world, ent, true);
				return true;
			}

			if (noenemy || !enemy->entnum)
				break;
		}

		return false;
	}

// push down from a step height above the wished position
	VectorMA(neworg, movevars.stepheight, axis[2], neworg);
	VectorMA(neworg, movevars.stepheight*-2, axis[2], end);

	trace = World_Move (world, neworg, ent->v->mins, ent->v->maxs, end, false, ent);
	if (set_move_trace)
		set_move_trace(world->progs, &trace);

	if (trace.allsolid)
		return false;

	if (trace.startsolid)
	{
		//move up by an extra step, if needed
		VectorMA(neworg, -movevars.stepheight, axis[2], neworg);
		trace = World_Move (world, neworg, ent->v->mins, ent->v->maxs, end, false, ent);
		if (set_move_trace)
			set_move_trace(world->progs, &trace);
		if (trace.allsolid || trace.startsolid)
			return false;
	}
	if (trace.fraction == 1)
	{
	// if monster had the ground pulled out, go ahead and fall
		if ( (int)ent->v->flags & FL_PARTIALGROUND )
		{
			VectorAdd (ent->v->origin, move, ent->v->origin);
			if (relink)
				World_LinkEdict (world, ent, true);
			ent->v->flags = (int)ent->v->flags & ~FL_ONGROUND;
//	Con_Printf ("fall down\n"); 
			return true;
		}
	
		return false;		// walked off an edge
	}

// check point traces down for dangling corners
	VectorCopy (trace.endpos, ent->v->origin);
	
	if (!World_CheckBottom (world, ent, axis[2]))
	{
		if ( (int)ent->v->flags & FL_PARTIALGROUND )
		{	// entity had floor mostly pulled out from underneath it
			// and is trying to correct
			if (relink)
				World_LinkEdict (world, ent, true);
			return true;
		}
		VectorCopy (oldorg, ent->v->origin);
		return false;
	}

	if ( (int)ent->v->flags & FL_PARTIALGROUND )
	{
//		Con_Printf ("back on ground\n"); 
		ent->v->flags = (int)ent->v->flags & ~FL_PARTIALGROUND;
	}
	ent->v->groundentity = EDICT_TO_PROG(world->progs, trace.ent);

// the move is ok
	if (relink)
		World_LinkEdict (world, ent, true);
	return true;
}


//============================================================================

qboolean World_GetEntGravityAxis(wedict_t *ent, vec3_t axis[3])
{
	if (ent->xv->gravitydir[0] || ent->xv->gravitydir[1] || ent->xv->gravitydir[2])
	{
		void PerpendicularVector( vec3_t dst, const vec3_t src );
		VectorNegate(ent->xv->gravitydir, axis[2]);
		VectorNormalize(axis[2]);
		PerpendicularVector(axis[0], axis[2]);
		VectorNormalize(axis[0]);
		CrossProduct(axis[2], axis[0], axis[1]);
		VectorNormalize(axis[1]);
		return true;
	}
	else
	{
		VectorSet(axis[0], 1, 0, 0);
		VectorSet(axis[1], 0, 1, 0);
		VectorSet(axis[2], 0, 0, 1);
		return false;
	}
}

/*
==============
PF_changeyaw

This was a major timewaster in progs, so it was converted to C
==============
*/
float World_changeyaw (wedict_t *ent)
{
	float		ideal, current, move, speed;
	vec3_t surf[3];
	if (World_GetEntGravityAxis(ent, surf))
	{
		//complex matrix stuff
		float mat[16];
		float surfm[16], invsurfm[16];
		float viewm[16];
		vec3_t view[4];
		vec3_t vang;

		/*calc current view matrix relative to the surface*/
		AngleVectorsMesh(ent->v->angles, view[0], view[1], view[2]);
		VectorNegate(view[1], view[1]);

		World_GetEntGravityAxis(ent, surf);

		Matrix4x4_RM_FromVectors(surfm, surf[0], surf[1], surf[2], vec3_origin);
		Matrix3x4_InvertTo4x4_Simple(surfm, invsurfm);

		/*calc current view matrix relative to the surface*/
		Matrix4x4_RM_FromVectors(viewm, view[0], view[1], view[2], vec3_origin);
		Matrix4_Multiply(viewm, invsurfm, mat);
		/*convert that back to angles*/
		Matrix3x4_RM_ToVectors(mat, view[0], view[1], view[2], view[3]);
		VectorAngles(view[0], view[2], vang, true);

		/*edit it*/

		ideal = ent->v->ideal_yaw;
		speed = ent->v->yaw_speed;
		move = ideal - anglemod(vang[YAW]);
		if (move > 180)
			move -= 360;
		else if (move < -180)
			move += 360;
		if (move > 0)
		{
			if (move > speed)
				move = speed;
		}
		else
		{
			if (move < -speed)
				move = -speed;
		}
		vang[YAW] = anglemod(vang[YAW] + move);

		/*clamp pitch, kill roll. monsters don't pitch/roll.*/
		vang[PITCH] = 0;
		vang[ROLL] = 0;

		move = ideal - vang[YAW];

		/*turn those angles back to a matrix*/
		AngleVectors(vang, view[0], view[1], view[2]);
		VectorNegate(view[1], view[1]);
		Matrix4x4_RM_FromVectors(mat, view[0], view[1], view[2], vec3_origin);
		/*rotate back into world space*/
		Matrix4_Multiply(mat, surfm, viewm);
		/*and figure out the final result*/
		Matrix3x4_RM_ToVectors(viewm, view[0], view[1], view[2], view[3]);
		VectorAngles(view[0], view[2], ent->v->angles, true);

		//make sure everything is sane
		ent->v->angles[PITCH] = anglemod(ent->v->angles[PITCH]);
		ent->v->angles[YAW] = anglemod(ent->v->angles[YAW]);
		ent->v->angles[ROLL] = anglemod(ent->v->angles[ROLL]);
		return move;
	}


	//FIXME: gravitydir. reorient the angles to change the yaw with respect to the current ground surface.

	current = anglemod( ent->v->angles[1] );
	ideal = ent->v->ideal_yaw;
	speed = ent->v->yaw_speed;

	if (current == ideal)
		return 0;
	move = ideal - current;
	if (ideal > current)
	{
		if (move >= 180)
			move = move - 360;
	}
	else
	{
		if (move <= -180)
			move = move + 360;
	}
	if (move > 0)
	{
		if (move > speed)
			move = speed;
	}
	else
	{
		if (move < -speed)
			move = -speed;
	}

	ent->v->angles[1] = anglemod (current + move);

	return ideal - ent->v->angles[1];
}

/*
======================
SV_StepDirection

Turns to the movement direction, and walks the current distance if
facing it.

======================
*/
qboolean World_StepDirection (world_t *world, wedict_t *ent, float yaw, float dist, vec3_t axis[3])
{
	vec3_t		move, oldorigin;
	float		delta, s;
	
	ent->v->ideal_yaw = yaw;

	delta = World_changeyaw(ent);

	yaw = yaw*M_PI*2 / 360;

	s = cos(yaw)*dist;
	VectorScale(axis[0], s, move);
	s = sin(yaw)*dist;
	VectorMA(move, s, axis[1], move);

	//FIXME: Hexen2: ent flags & FL_SET_TRACE

	VectorCopy (ent->v->origin, oldorigin);
	if (World_movestep (world, ent, move, axis, false, false, NULL))
	{
		delta = anglemod(delta);
		if (delta > 45 && delta < 315)
		{	// not turned far enough, so don't take the step
			//FIXME: surely this is noticably inefficient?
			VectorCopy (oldorigin, ent->v->origin);
		}
		World_LinkEdict (world, ent, true);
		return true;
	}
	World_LinkEdict (world, ent, true);
		
	return false;
}

/*
======================
SV_FixCheckBottom

======================
*/
void World_FixCheckBottom (wedict_t *ent)
{
//	Con_Printf ("SV_FixCheckBottom\n");
	
	ent->v->flags = (int)ent->v->flags | FL_PARTIALGROUND;
}



/*
================
SV_NewChaseDir

================
*/
#define	DI_NODIR	-1

void World_NewChaseDir (world_t *world, wedict_t *actor, wedict_t *enemy, float dist, vec3_t axis[3])
{
	float		deltax,deltay;
	float			d[3];
	float		tdir, olddir, turnaround;

	olddir = anglemod( (int)(actor->v->ideal_yaw/45)*45 );
	turnaround = anglemod(olddir - 180);

	VectorSubtract(enemy->v->origin, actor->v->origin, d);
	deltax = DotProduct(d, axis[0]);
	deltay = DotProduct(d, axis[1]);
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
			
		if (tdir != turnaround && World_StepDirection(world, actor, tdir, dist, axis))
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
	&& World_StepDirection(world, actor, d[1], dist, axis))
			return;

	if (d[2]!=DI_NODIR && d[2]!=turnaround
	&& World_StepDirection(world, actor, d[2], dist, axis))
			return;

/* there is no direct path to the player, so pick another direction */

	if (olddir!=DI_NODIR && World_StepDirection(world, actor, olddir, dist, axis))
			return;

	if (rand()&1) 	/*randomly determine direction of search*/
	{
		for (tdir=0 ; tdir<=315 ; tdir += 45)
			if (tdir!=turnaround && World_StepDirection(world, actor, tdir, dist, axis) )
					return;
	}
	else
	{
		for (tdir=315 ; tdir >=0 ; tdir -= 45)
			if (tdir!=turnaround && World_StepDirection(world, actor, tdir, dist, axis) )
					return;
	}

	if (turnaround != DI_NODIR && World_StepDirection(world, actor, turnaround, dist, axis) )
			return;

	actor->v->ideal_yaw = olddir;		// can't move

// if a bridge was pulled out from underneath a monster, it may not have
// a valid standing position at all

	if (!World_CheckBottom (world, actor, axis[2]))
		World_FixCheckBottom (actor);

}

/*
======================
SV_CloseEnough

======================
*/
qboolean World_CloseEnough (wedict_t *ent, wedict_t *goal, float dist)
{
	int		i;
	
	for (i=0 ; i<3 ; i++)
	{
		if (goal->v->absmin[i] > ent->v->absmax[i] + dist)
			return false;
		if (goal->v->absmax[i] < ent->v->absmin[i] - dist)
			return false;
	}
	return true;
}

/*
======================
SV_MoveToGoal

======================
*/
qboolean World_MoveToGoal (world_t *world, wedict_t *ent, float dist)
{
	wedict_t	*goal;
	vec3_t axis[3];

	ent = (wedict_t*)PROG_TO_EDICT(world->progs, *world->g.self);	
	goal = (wedict_t*)PROG_TO_EDICT(world->progs, ent->v->goalentity);

	if ( !( (int)ent->v->flags & (FL_ONGROUND|FL_FLY|FL_SWIM) ) )
	{
		return false;
	}

// if the next step hits the enemy, return immediately
	if ( PROG_TO_EDICT(world->progs, ent->v->enemy) != (edict_t*)world->edicts && World_CloseEnough (ent, goal, dist) )
		return true;


	World_GetEntGravityAxis(ent, axis);

// bump around...
	if ( (rand()&3)==1 ||
	!World_StepDirection (world, ent, ent->v->ideal_yaw, dist, axis))
	{
		World_NewChaseDir (world, ent, goal, dist, axis);
	}
	return true;
}



#ifdef ENGINE_ROUTING
#ifdef HAVE_CLIENT
static cvar_t route_shownodes = CVAR("route_shownodes", "0");
#endif

#define LF_EDGE			0x00000001
#define LF_JUMP			0x00000002
#define LF_CROUCH		0x00000004
#define LF_TELEPORT		0x00000008
#define LF_USER			0x7fffff00
#define LF_DESTINATION	0x80000000	//You have reached your destination...
struct waypointnetwork_s
{
	size_t refs;
	size_t numwaypoints;
	model_t *worldmodel;

	struct resultnodes_s
	{
		vec3_t pos;
		int linkflags;
		float radius;
	} *displaynode;
	size_t displaynodes;

	struct waypoint_s
	{
		vec3_t org;
		float radius;	//used for picking the closest waypoint. aka proximity weight. also relaxes routes inside the area.
		struct wpneighbour_s
		{
			int node;
			float linkcost;//might be much lower in the case of teleports, or expensive if someone wanted it to be a lower priority link.
			int linkflags; //LF_*
			struct wphint_s
			{
				vec3_t pos[3];
			} *hints;
		} *neighbour;
		size_t neighbours;
	} waypoints[1];
};
void WayNet_Done(struct waypointnetwork_s *net)
{
	if (net)
	if (0 == --net->refs)
	{
		Z_Free(net);
	}
}
static qboolean WayNet_TokenizeLine(char **linestart)
{
	char *end = *linestart;
	if (!end || !*end)
	{	//clear it out...
		Cmd_TokenizeString("", false, false);
		return false;
	}
	for (; *end; end++)
	{
		if (*end == '\n')
		{
			*end++ = 0;
			break;
		}
	}
	Cmd_TokenizeString(*linestart, false, false);
	*linestart = end;
	return true;
}
static struct waypointnetwork_s *WayNet_Begin(void **ctxptr, model_t *worldmodel)
{
	struct waypointnetwork_s *net = *ctxptr;
	if (!net)
	{
		char *wf = NULL;
		qofs_t fsize = 0;
		if (!worldmodel)
			return NULL;
		if (!wf && !strncmp(worldmodel->name, "maps/", 5))
		{
			char n[MAX_QPATH];
			COM_StripExtension(worldmodel->name+5, n, sizeof(n));
			wf = FS_MallocFile(va("data/%s.way", n), FS_GAME, &fsize);
			if (!wf)
				wf = FS_MallocFile(va("bots/navigation/%s.nav", n), FS_GAME, &fsize);
		}
		if (!wf)
			wf = FS_MallocFile(va("%s.way", worldmodel->name), FS_GAME, &fsize);

		if (wf && fsize >= 8 && (
			  (wf[0] == 'N' && wf[1] == 'A' && wf[2] == 'V' && wf[3] == '2'  &&  (wf[4]|(wf[5]<<8)|(wf[6]<<8)|(wf[7]<<8)) >= 12 && (wf[4]|(wf[5]<<8)|(wf[6]<<8)|(wf[7]<<8)) <= 17)	//q1e
			||(wf[0] == 'N' && wf[1] == 'A' && wf[2] == 'V' && wf[3] == '3'  &&  (wf[4]|(wf[5]<<8)|(wf[6]<<8)|(wf[7]<<8)) >= 2  && (wf[4]|(wf[5]<<8)|(wf[6]<<8)|(wf[7]<<8)) <= 6)	//q2e
			))
		{	//rerelease's format(s)
			sizebuf_t sb = {wf, fsize, fsize};
			qboolean error = false;
			unsigned int u;
			unsigned int ver, numnodes, numlinks, numhints, numents;
			struct wphint_s *hints;
			struct wpneighbour_s *links;

			MSG_BeginReading(&sb, msg_nullnetprim);
			/*magic(already checked)*/MSG_ReadLong();
			ver = MSG_ReadLong();
			if (wf[3] == '3') ver |= 0x80000000;	//convert from q2 to q1 version numbers...
			numnodes = MSG_ReadLong();
			numlinks = MSG_ReadLong();
			numhints = MSG_ReadLong();
			if (ver >= 16)
				/*some sort of scale?*/MSG_ReadFloat();

			net = Z_Malloc(sizeof(*net)-sizeof(net->waypoints) + (numnodes*sizeof(struct waypoint_s)) + (numlinks*sizeof(struct wpneighbour_s)) + (numhints*sizeof(struct wphint_s)));
			net->numwaypoints = numnodes;

			links = (struct wpneighbour_s*)(net->waypoints+numnodes);
			hints = (struct wphint_s*)(links+numlinks);

			for (u = 0; u < numnodes && !error; u++)
			{
				unsigned short flags = MSG_ReadShort();	//some sort of conditional info
				unsigned short nodelinks = MSG_ReadShort();
				unsigned short firstlink = MSG_ReadShort();
				unsigned short radius = MSG_ReadShort();

				(void)flags;
				net->waypoints[u].neighbours = nodelinks;
				net->waypoints[u].neighbour = links + firstlink;
				net->waypoints[u].radius = radius;

				if (net->waypoints[u].neighbour+net->waypoints[u].neighbours > &links[numlinks])
					error = true;
			}
			for (u = 0; u < numnodes; u++)
			{	//positions are split for some reason
				net->waypoints[u].org[0] = MSG_ReadFloat();
				net->waypoints[u].org[1] = MSG_ReadFloat();
				net->waypoints[u].org[2] = MSG_ReadFloat()+24;	//rerelease waypoints are aligned to the ground.
			}

			for (u = 0; u < numlinks; u++)
			{
				unsigned short peernode = MSG_ReadShort();
				unsigned short type = MSG_ReadShort();	//0=normal, 1=
				unsigned short hint = MSG_ReadShort();	//can be ~0

				links[u].linkcost = 16;
				links[u].linkflags = 1u<<type;
				links[u].node = peernode;
				links[u].hints = NULL;
				if (hint < numhints)
					links[u].hints = hints+hint;
				else if (hint != 0xffff)
					error = true;
				if (links[u].node >= numnodes)
					error = true;
			}
			for (u = 0; u < numhints; u++)
			{
				hints[u].pos[0][0] = MSG_ReadFloat();
				hints[u].pos[0][1] = MSG_ReadFloat();
				hints[u].pos[0][2] = MSG_ReadFloat();

				hints[u].pos[1][0] = MSG_ReadFloat();
				hints[u].pos[1][1] = MSG_ReadFloat();
				hints[u].pos[1][2] = MSG_ReadFloat();

				hints[u].pos[2][0] = MSG_ReadFloat();
				hints[u].pos[2][1] = MSG_ReadFloat();
				hints[u].pos[2][2] = MSG_ReadFloat();

				if (ver >= 0x80000006)
				{
					/*hints[u].pos[3][0] =*/ MSG_ReadFloat();
					/*hints[u].pos[3][1] =*/ MSG_ReadFloat();
					/*hints[u].pos[3][2] =*/ MSG_ReadFloat();
				}
			}

			numents = MSG_ReadLong();	//grr
			for (u = 0; u < numents; u++)
			{
				unsigned short link = MSG_ReadShort();
				if (link >= numlinks)
					error = true;	//err?.. probably we don't really care, but for the sake of sanity lets bail.
				/*mins = */MSG_ReadFloat();
				MSG_ReadFloat();
				MSG_ReadFloat();

				/*maxs = */MSG_ReadFloat();
				MSG_ReadFloat();
				MSG_ReadFloat();

				if (ver <= 13)	//FIXME: true for 12, verify v13
					;
				else if (ver == 14)
				{
					/*targ*/MSG_ReadLong();
					/*class*/MSG_ReadLong();
				}
				else if (ver >= 15)
					/*entnum*/MSG_ReadLong();
			}

			for (u = 0; u < numnodes && !error; u++)
			{	//compute costs (no reading here)
				size_t v;
				for (v = 0; v < net->waypoints[u].neighbours; v++)
				{
					vec3_t move;
					VectorSubtract(net->waypoints[net->waypoints[u].neighbour[v].node].org, net->waypoints[u].org, move);
					net->waypoints[u].neighbour[v].linkcost = sqrt(DotProduct(move,move));
				}
			}

			if(msg_badread)
				error = true;
			MSG_ReadByte();
			if (!msg_badread)
				error = true;	//should have taken us over the end. there's trailing junk here.

			if (error)
			{	//some sort of corrupt
				Z_Free(net);
				net = NULL;
			}
		}
		else if (wf)
		{	//our qc-friendly format (predates remaster)
			//read the number of waypoints
			char *l=wf, *e;
			int numwaypoints, maxlinks, numlinks;
			struct wpneighbour_s *nextlink;
			WayNet_TokenizeLine(&l);
			numwaypoints = atoi(Cmd_Argv(0));
			//count lines and guess the link count.
			for (e = l, maxlinks=0; *e; e++)
				if (*e == '\n')
					maxlinks++;
			maxlinks -= numwaypoints;

			net = Z_Malloc(sizeof(*net)-sizeof(net->waypoints) + (numwaypoints*sizeof(struct waypoint_s)) + (maxlinks*sizeof(struct wpneighbour_s)));
			net->worldmodel = worldmodel;

			nextlink = (struct wpneighbour_s*)(net->waypoints+numwaypoints);

			while (WayNet_TokenizeLine(&l) && net->numwaypoints < numwaypoints)
			{
				if (!Cmd_Argc())
					continue;	//a comment line?
				net->waypoints[net->numwaypoints].org[0] = atof(Cmd_Argv(0));
				net->waypoints[net->numwaypoints].org[1] = atof(Cmd_Argv(1));
				net->waypoints[net->numwaypoints].org[2] = atof(Cmd_Argv(2));
				net->waypoints[net->numwaypoints].radius = atof(Cmd_Argv(3));
				numlinks = bound(0, atoi(Cmd_Argv(4)), maxlinks);

				//make sure the links are valid, and clamp to avoid problems (even if we're then going to mis-parse.
				net->waypoints[net->numwaypoints].neighbour = nextlink;
				while (numlinks-- > 0 && WayNet_TokenizeLine(&l))
				{
					if (!Cmd_Argc())
						continue;	//a comment line?
					nextlink[net->waypoints[net->numwaypoints].neighbours].node = atoi(Cmd_Argv(0));
					nextlink[net->waypoints[net->numwaypoints].neighbours].linkcost = atof(Cmd_Argv(1));
					nextlink[net->waypoints[net->numwaypoints].neighbours++].linkflags = atoi(Cmd_Argv(2));
				}
				maxlinks -= net->waypoints[net->numwaypoints].neighbours;
				nextlink += net->waypoints[net->numwaypoints++].neighbours;
			}
		}
		BZ_Free(wf);
	}

	if (!*ctxptr)
	{	//no network yet.
		if (!net)
		{	//don't spam reload attempts.
			if (!worldmodel)
				return NULL;
			net = Z_Malloc(sizeof(*net)-sizeof(net->waypoints));
			net->numwaypoints = 0;
		}
		net->worldmodel = worldmodel;

		//link to the world state
		net->refs = 1;
		*ctxptr = net;
	}


	net->refs++;
	return net;
}

struct waydist_s
{
	int node;
	float sdist;
};
int QDECL WayNet_Prioritise(const void *a, const void *b)
{
	const struct waydist_s *w1 = a, *w2 = b;
	if (w1->sdist < w2->sdist)
		return -1;
	if (w1->sdist == w2->sdist)
		return 0;
	return 1;
}
int WayNet_FindNearestNode(struct waypointnetwork_s *net, vec3_t pos)
{
	if (net && net->numwaypoints)
	{
		//we qsort the possible nodes, in an attempt to reduce traces.
		struct waydist_s *sortedways = alloca(sizeof(*sortedways) * net->numwaypoints);
		size_t u;
		vec3_t disp;
		float sradius;
		trace_t tr;
		for (u = 0; u < net->numwaypoints; u++)
		{
			sortedways[u].node = u;
			VectorSubtract(net->waypoints[u].org, pos, disp);
			sortedways[u].sdist = DotProduct(disp, disp);
			sradius = net->waypoints[u].radius*net->waypoints[u].radius;
			if (sortedways[u].sdist < sradius)
				sortedways[u].sdist -= sradius;	//if we're inside the waypoint's radius, push inwards resulting in negatives, so these are always highly prioritised
		}
		qsort(sortedways, net->numwaypoints, sizeof(struct waydist_s), WayNet_Prioritise);

		//can't trace yet...
		if (net->worldmodel->loadstate != MLS_LOADED)
			return sortedways[0].node;
		for (u = 0; u < net->numwaypoints; u++)
		{
			if (sortedways[u].sdist > 0)
			{	//if we're outside the node, we need to do a trace to make sure we can actually reach it.
				net->worldmodel->funcs.NativeTrace(net->worldmodel, 0, NULL, NULL, pos, net->waypoints[sortedways[u].node].org, vec3_origin, vec3_origin, false, MASK_WORLDSOLID, &tr);
				if (tr.fraction < 1)
					continue;	//this node is blocked. just move on to the next.
			}
			return sortedways[u].node;
		}
	}
	return -1;
}

struct routecalc_s
{
	world_t *world;
	wedict_t *ed;
	int spawncount;	//so we don't confuse stuff if the map gets restarted.
//	float spawnid;	//so the route fails if the ent is removed.
	func_t callback;

	vec3_t start;
	vec3_t end;
	int denylinkflags;

	int startn;
	int endn;

	int numresultnodes;
	struct resultnodes_s *resultnodes;

	struct waypointnetwork_s *waynet;
};
//main thread
void Route_Calculated(void *ctx, void *data, size_t a, size_t b)
{
	struct routecalc_s *route = data;
	pubprogfuncs_t *prinst = route->world->progs;
	//let the gamecode know the results

	if (!route->callback)
	{
		if (route->waynet)
		{
			BZ_Free(route->waynet->displaynode);
			route->waynet->displaynode = BZ_Malloc(sizeof(struct resultnodes_s) * route->numresultnodes);
			route->waynet->displaynodes = route->numresultnodes;
			memcpy(route->waynet->displaynode, route->resultnodes, sizeof(struct resultnodes_s) * route->numresultnodes);
		}
	}
	else if (route->callback && route->world->spawncount == route->spawncount/* && route->spawnid == route->ed->xv->uniquespawnid*/)
	{
		struct globalvars_s * pr_globals = PR_globals(prinst, PR_CURRENT);
		struct resultnodes_s *ptr = prinst->AddressableAlloc(prinst, sizeof(struct resultnodes_s) * route->numresultnodes);
		memcpy(ptr, route->resultnodes, sizeof(struct resultnodes_s) * route->numresultnodes);

		G_INT(OFS_PARM0) = EDICT_TO_PROG(prinst, route->ed);
		VectorCopy(route->end, G_VECTOR(OFS_PARM1));
		G_INT(OFS_PARM2) = route->numresultnodes;
		G_INT(OFS_PARM3) = (char*)ptr-prinst->stringtable;
		PR_ExecuteProgram(prinst, route->callback);
	}

	//and we're done. destroy everything.
	WayNet_Done(route->waynet);
	Z_Free(route->resultnodes);
	Z_Free(route);
}

//#define FLOODALL
#define COST_INFINITE FLT_MAX

typedef struct
{
	int id;
	int flags;
} nodefrom_t;

static qboolean Route_Completed(struct routecalc_s *r, nodefrom_t *nodecamefrom)
{
	size_t u;
	struct waypointnetwork_s *n = r->waynet;
	r->resultnodes = Z_Malloc(sizeof(*r->resultnodes)*(n->numwaypoints+1)*3);

	r->numresultnodes = 0;

	//target point is first. yay.
	VectorCopy(r->end, r->resultnodes[0].pos);
	r->resultnodes[0].linkflags = LF_DESTINATION;
	r->resultnodes[0].radius = 32;
	r->numresultnodes++;

	u = r->endn;
	for (;;)
	{
		VectorCopy(n->waypoints[u].org, r->resultnodes[r->numresultnodes].pos);
		r->resultnodes[r->numresultnodes].linkflags = nodecamefrom[u].flags;
		r->resultnodes[r->numresultnodes].radius = n->waypoints[u].radius;
		r->numresultnodes++;
		if (u == r->startn)
			break;
		u = nodecamefrom[u].id;
	}

	//and include the start point, because we can
	VectorCopy(r->start, r->resultnodes[r->numresultnodes].pos);
	r->resultnodes[r->numresultnodes].linkflags = 0;
	r->resultnodes[r->numresultnodes].radius = 32;
	r->numresultnodes++;
	return true;
}

#if 1
static float Route_GuessCost(struct routecalc_s *r, float *fromorg)
{	//if we want to guarentee the shortest route, then we MUST always return a value <= to the actual cost here.
	//unfortunately we don't know how many teleporters are between the two points.
	//on the plus side, a little randomness here means we'll find alternative (longer) routes some times, which will reduce flash points and help flag carriers...
	vec3_t disp;
	VectorSubtract(r->end, fromorg, disp);
	return sqrt(DotProduct(disp,disp));
}
static qboolean Route_Process(struct routecalc_s *r)
{
	struct waypointnetwork_s *n = r->waynet;
	int opennodes = 0;
	int u, j;
	float guesscost;
	struct opennode_s {
		int node;
		float cost;
	} *open = alloca(sizeof(*open)*n->numwaypoints);
	float *nodecost = alloca(sizeof(*nodecost)*n->numwaypoints);
	nodefrom_t *nodecamefrom = alloca(sizeof(*nodecamefrom)*n->numwaypoints);

	for(u = 0; u < n->numwaypoints; u++)
		nodecost[u] = COST_INFINITE;

	if (r->startn >= 0)
	{
		nodecost[r->startn] = 0;
		open[0].node = r->startn;
		open[0].cost = 0;
		opennodes++;
	}

	while(opennodes)
	{
		int nodeidx = open[--opennodes].node;
		struct waypoint_s *wp = &n->waypoints[nodeidx];
#ifdef _DEBUG
		if (nodeidx < 0 || nodeidx >= n->numwaypoints)
		{
			Con_Printf("Bad node index in open list\n");
			return false;
		}
#endif
		if (nodeidx == r->endn)
		{	//we found the end!
			return Route_Completed(r, nodecamefrom);
		}
		for (u = 0; u < wp->neighbours; u++)
		{
			struct wpneighbour_s *l = &wp->neighbour[u];
			int linkidx = l->node;
			float realcost = nodecost[nodeidx] + l->linkcost;

			if (l->linkflags & r->denylinkflags)
				continue;
#ifdef _DEBUG
			if (linkidx < 0 || linkidx >= n->numwaypoints)
			{
				Con_Printf("Bad node link index in routing table\n");
				return false;
			}
#endif
			if (realcost >= nodecost[linkidx])
				continue;

			nodecamefrom[linkidx].id = nodeidx;
			nodecamefrom[linkidx].flags = l->linkflags;
			nodecost[linkidx] = realcost;

			for (j = opennodes-1; j >= 0; j--)
			{
				if (open[j].node == linkidx)
					break;
			}
			guesscost = realcost + Route_GuessCost(r, n->waypoints[linkidx].org);

			if (j < 0)
			{	//not already in the list
				//tbh, we should probably just directly bubble in this loop instead of doing the memcpy (with its internal second loop).
				for (j = opennodes-1; j >= 0; j--)
					if (guesscost <= open[j].cost)
						break;
				j++;
				//move them up
				memmove(&open[j+1], &open[j], sizeof(*open)*(opennodes-j));
				open[j].node = linkidx;
				open[j].cost = guesscost;
				opennodes++;
			}
			else if (guesscost < open[j].cost)
			{	//if it got cheaper, be prepared to move the node towards the higher addresses (these will be checked first).
				for (; j+1 < opennodes && open[j+1].cost > guesscost; j++)
					open[j] = open[j+1];
				//okay, so we can't keep going... this is our new slot!
				open[j].node = linkidx;
				open[j].cost = guesscost;
			}
			//otherwise it got more expensive, and we don't care about that
		}
	}

	return false;
}
#else
static qboolean Route_Process(struct routecalc_s *r)
{
	struct waypointnetwork_s *n = r->waynet;
	int opennodes = 0;
	int u, j;

	//we use an open list in a desperate attempt to avoid recursing the entire network
	int *open = alloca(sizeof(*open)*n->numwaypoints);
	float *nodecost = alloca(sizeof(*nodecost)*n->numwaypoints);
	nodefrom_t *nodecamefrom = alloca(sizeof(*nodecamefrom)*n->numwaypoints);

	for(u = 0; u < n->numwaypoints; u++)
		nodecost[u] = COST_INFINITE;

	nodecost[r->startn] = 0;
	open[opennodes++] = r->startn;

	while(opennodes)
	{
		int nodeidx = open[--opennodes];
		struct waypoint_s *wp = &n->waypoints[nodeidx];
//		if (nodeidx < 0 || nodeidx >= n->numwaypoints)
//			return false;

		for (u = 0; u < wp->neighbours; u++)
		{
			struct wpneighbour_s *l = &wp->neighbour[u];
			int linkidx = l->node;

			float realcost = nodecost[nodeidx] + l->linkcost;
//			if (linkidx < 0 || linkidx >= n->numwaypoints)
//				return false;
			if (realcost >= nodecost[linkidx])
				continue;

			nodecamefrom[linkidx].id = nodeidx;
			nodecamefrom[linkidx].flags = l->linkflags;
			nodecost[linkidx] = realcost;

			for (j = 0; j < opennodes; j++)
			{
				if (open[j] == linkidx)
					break;
			}

			if (j == opennodes)	//not already queued
				open[opennodes++] = linkidx;
		}
	}
	
	if (r->endn >= 0 && nodecost[r->endn] < COST_INFINITE)
	{	//we found the end! we can build the route from end to start.
		return Route_Completed(r, nodecamefrom);
	}
	return false;
}
#endif

//worker thread
void Route_Calculate(void *ctx, void *data, size_t a, size_t b)
{
	struct routecalc_s *route = data;

	//first thing is to find the start+end nodes.

	if (route->waynet && route->startn >= 0 && route->endn >= 0 && Route_Process(route))
		;
	else
	{
		route->numresultnodes = 0;
		route->resultnodes = Z_Malloc(sizeof(*route->resultnodes)*2);
		VectorCopy(route->end, route->resultnodes[0].pos);
		route->resultnodes[0].linkflags = LF_DESTINATION;
		route->numresultnodes++;

		VectorCopy(route->start, route->resultnodes[route->numresultnodes].pos);
		route->resultnodes[route->numresultnodes].linkflags = 0;
		route->numresultnodes++;
	}

	COM_AddWork(WG_MAIN, Route_Calculated, NULL, route, 0, 0);
}

//void route_linkitem(entity item, int ittype)	//-1 to unlink
//void route_choosedest(entity ent, int numitemtypes, float *itemweights)
/*
=============
PF_route_calculate

engine reads+caches the nodes from a file.
the route's nodes must be memfreed on completion.
the first node in the nodelist is the destination.

typedef struct {
	vector dest;
	int linkflags;
	//float anglehint;
} nodeslist_t;
void(entity ent, vector dest, int denylinkflags, void(entity ent, vector dest, int numnodes, nodeslist_t *nodelist) callback) route_calculate = #0;
=============
*/
void QCBUILTIN PF_route_calculate (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	struct routecalc_s *route = Z_Malloc(sizeof(*route));
	float *end;
	route->world = prinst->parms->user;
	route->spawncount = route->world->spawncount;
	route->ed = G_WEDICT(prinst, OFS_PARM0);
//	route->spawnid = route->ed->xv->uniquespawnid;
	end = G_VECTOR(OFS_PARM1);
	route->denylinkflags = G_INT(OFS_PARM2);
	route->callback = G_INT(OFS_PARM3);

	VectorCopy(route->ed->v->origin, route->start);
	VectorCopy(end, route->end);

	route->waynet = WayNet_Begin(&route->world->waypoints, route->world->worldmodel);
	if (!route->waynet)
	{
		Z_Free(route);
		return;
	}

	//tracelines use some sequence info to avoid retracing the same brush multiple times.
	//	this means that we can't reliably trace on worker threads (would break the main thread occasionally).
	//	so we have to do this here.
	//FIXME: find a safe way to NOT do this here.
	route->startn = WayNet_FindNearestNode(route->waynet, route->start);
	route->endn = WayNet_FindNearestNode(route->waynet, route->end);

	COM_AddWork(WG_LOADER, Route_Calculate, NULL, route, 0, 0);
}
#ifdef HAVE_CLIENT
static void Route_Visualise_f(void)
{
	extern world_t csqc_world;
	vec3_t targ = {atof(Cmd_Argv(1)),atof(Cmd_Argv(2)),atof(Cmd_Argv(3))};
	struct routecalc_s *route = Z_Malloc(sizeof(*route));
	route->world = &csqc_world;
	route->spawncount = route->world->spawncount;
	route->ed = route->world->edicts;
//	route->spawnid = route->ed->xv->uniquespawnid;
	VectorCopy(r_refdef.vieworg, route->start);
	VectorCopy(targ, route->end);

	route->waynet = WayNet_Begin(&route->world->waypoints, cl.worldmodel);
	if (!route->waynet)
	{
		Z_Free(route);
		return;
	}

	//tracelines use some sequence info to avoid retracing the same brush multiple times.
	//	this means that we can't reliably trace on worker threads (would break the main thread occasionally).
	//	so we have to do this here.
	//FIXME: find a safe way to NOT do this here.
	route->startn = WayNet_FindNearestNode(route->waynet, route->start);
	route->endn = WayNet_FindNearestNode(route->waynet, route->end);

	COM_AddWork(WG_LOADER, Route_Calculate, NULL, route, 0, 0);
}

#include "shader.h"
void PR_Route_Visualise (void)
{
	extern world_t csqc_world;
	world_t *w = &csqc_world;
	struct waypointnetwork_s *wn;
	size_t u;

	wn = (w && (w->waypoints || route_shownodes.ival))?WayNet_Begin(&w->waypoints, cl.worldmodel):NULL;
	if (wn)
	{
		if (route_shownodes.ival)
		{
			float mat[12] = {1,0,0,0, 0,1,0,0, 0,0,1,0};
			shader_t *shader_out = R_RegisterShader("waypointvolume_out", SUF_NONE,
					"{\n"
						"polygonoffset\n"
						"nodepth\n"
						"{\n"
							"map $whiteimage\n"
							"blendfunc add\n"
							"rgbgen vertex\n"
							"alphagen vertex\n"
						"}\n"
					"}\n");
			shader_t *shader_in = R_RegisterShader("waypointvolume_in", SUF_NONE,
					"{\n"
						"polygonoffset\n"
						"cull disable\n"
						"nodepth\n"
						"{\n"
							"map $whiteimage\n"
							"blendfunc add\n"
							"rgbgen vertex\n"
							"alphagen vertex\n"
						"}\n"
					"}\n");
			float radius;
			vec3_t dir;
			//should probably use a different colour for the node you're inside.
			int nearest = WayNet_FindNearestNode(wn, r_origin);
			for (u = 0; u < wn->numwaypoints; u++)
			{
				mat[3] = wn->waypoints[u].org[0];
				mat[7] = wn->waypoints[u].org[1];
				mat[11] = wn->waypoints[u].org[2];
				radius = wn->waypoints[u].radius;
				if (radius <= 0)
					radius = 1; //waypoints shouldn't really have a radius of 0, but if they do we'll still want to draw something.

				VectorSubtract(wn->waypoints[u].org, r_refdef.vieworg, dir);
				if (DotProduct(dir,dir) < radius*radius)
					CLQ1_AddOrientedSphere(shader_in, radius, mat, 0.0, 0.1, (nearest==u)?0.2:0.0, 1);
				else
					CLQ1_AddOrientedSphere(shader_out, radius, mat, 0.2, 0.0, (nearest==u)?0.2:0.0, 1);
			}
			for (u = 0; u < wn->numwaypoints; u++)
			{
				size_t n;
				for (n = 0; n < wn->waypoints[u].neighbours; n++)
				{
					struct waypoint_s *r = wn->waypoints + wn->waypoints[u].neighbour[n].node;
					CLQ1_DrawLine(shader_out, wn->waypoints[u].org, r->org, 0, 0, (nearest==u)?1:0.2, 1);
				}
			}
		}
		if (wn->displaynodes)
		{	//FIXME: we should probably use beams here
			shader_t *shader_route = R_RegisterShader("waypointroute", SUF_NONE,
					"{\n"
						"polygonoffset\n"
						"nodepth\n"
						"{\n"
							"map $whiteimage\n"
							"blendfunc add\n"
							"rgbgen vertex\n"
							"alphagen vertex\n"
						"}\n"
					"}\n");

			for (u = wn->displaynodes-1; u > 0; u--)
			{
				vec_t *start = wn->displaynode[u].pos;
				vec_t *end = wn->displaynode[u-1].pos;
				CLQ1_DrawLine(shader_route, start, end, 0.5, 0.5, 0.5, 1);
			}
		}
	}
	WayNet_Done(wn);
}
#endif

//destroys the routing waypoint cache.
void PR_Route_Shutdown (world_t *world)
{
	WayNet_Done(world->waypoints);
	world->waypoints = NULL;
}

static void Route_Reload_f(void)
{
#if defined(HAVE_CLIENT) && defined(CSQC_DAT)
	extern world_t csqc_world;
	PR_Route_Shutdown(&csqc_world);
#endif
#ifdef HAVE_SERVER
	PR_Route_Shutdown(&sv.world);
#endif
}
void PR_Route_Init (void)
{
#if defined(HAVE_CLIENT) && defined(CSQC_DAT)
	Cvar_Register(&route_shownodes, NULL);
	Cmd_AddCommand("route_visualise", Route_Visualise_f);
#endif
	Cmd_AddCommand("route_reload", Route_Reload_f);
}

//route_force
//COM_WorkerPartialSync
#endif

#endif
