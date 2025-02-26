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

static qboolean PM_TransformedHullCheck (model_t *model, framestate_t *framestate, vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, trace_t *trace, vec3_t origin, vec3_t angles);
int Q1BSP_HullPointContents(hull_t *hull, vec3_t p);
static	hull_t		box_hull;
static	mclipnode_t	box_clipnodes[6];
static	mplane_t	box_planes[6];

/*
===================
PM_InitBoxHull

Set up the planes and clipnodes so that the six floats of a bounding box
can just be stored out and get a proper hull_t structure.
===================
*/
void PM_InitBoxHull (void)
{
	int		i;
	int		side;

	box_hull.clipnodes = box_clipnodes;
	box_hull.planes = box_planes;
	box_hull.firstclipnode = 0;
	box_hull.lastclipnode = 5;

	for (i=0 ; i<6 ; i++)
	{
		box_clipnodes[i].planenum = i;
		
		side = i&1;
		
		box_clipnodes[i].children[side] = Q1CONTENTS_EMPTY;
		if (i != 5)
			box_clipnodes[i].children[side^1] = i + 1;
		else
			box_clipnodes[i].children[side^1] = Q1CONTENTS_SOLID;
		
		box_planes[i].type = i>>1;
		box_planes[i].normal[i>>1] = 1;
	}
	
}


/*
===================
PM_HullForBox

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
===================
*/
static hull_t	*PM_HullForBox (vec3_t mins, vec3_t maxs)
{
	box_planes[0].dist = maxs[0];
	box_planes[1].dist = mins[0];
	box_planes[2].dist = maxs[1];
	box_planes[3].dist = mins[1];
	box_planes[4].dist = maxs[2];
	box_planes[5].dist = mins[2];

	return &box_hull;
}


static int PM_TransformedModelPointContents (model_t *mod, vec3_t p, vec3_t origin, vec3_t angles)
{
	vec3_t p_l, axis[3];
	VectorSubtract (p, origin, p_l);

	if (!mod->funcs.PointContents)
		return FTECONTENTS_EMPTY;

	// rotate start and end into the models frame of reference
	if (angles[0] || angles[1] || angles[2])
	{
		AngleVectors (angles, axis[0], axis[1], axis[2]);
		VectorNegate(axis[1], axis[1]);
		return mod->funcs.PointContents(mod, axis, p_l);
	}

	return mod->funcs.PointContents(mod, NULL, p_l);
}


/*
==================
PM_PointContents

==================
*/
int PM_PointContents (vec3_t p)
{
	int			num;

	int pc;
	physent_t *pe;
	model_t *pm;

	//check world.
	pm = pmove.physents[0].model;
	if (!pm || pm->loadstate != MLS_LOADED)
		return FTECONTENTS_EMPTY;
	pc = pm->funcs.PointContents(pm, NULL, p);

	//we need this for e2m2 - waterjumping on to plats wouldn't work otherwise.
	for (num = 1; num < pmove.numphysent; num++)
	{
		pe = &pmove.physents[num];

		if (pe->info == pmove.skipent)
			continue;

		pm = pe->model;
		if (pm)
		{
			if (p[0] >= pe->origin[0]+pm->mins[0] && p[0] <= pe->origin[0]+pm->maxs[0] && 
				p[1] >= pe->origin[1]+pm->mins[1] && p[1] <= pe->origin[1]+pm->maxs[1] &&
				p[2] >= pe->origin[2]+pm->mins[2] && p[2] <= pe->origin[2]+pm->maxs[2])
			{
				if (pe->forcecontentsmask)
				{
					if (PM_TransformedModelPointContents(pm, p, pe->origin, pe->angles))
						pc |= pe->forcecontentsmask;
				}
				else
				{
					if (pe->nonsolid)
						continue;
					pc |= PM_TransformedModelPointContents(pm, p, pe->origin, pe->angles);
				}
			}
		}
		else if (pe->forcecontentsmask)
		{
			if (p[0] >= pe->origin[0]+pe->mins[0] && p[0] <= pe->origin[0]+pe->maxs[0] && 
				p[1] >= pe->origin[1]+pe->mins[1] && p[1] <= pe->origin[1]+pe->maxs[1] &&
				p[2] >= pe->origin[2]+pe->mins[2] && p[2] <= pe->origin[2]+pe->maxs[2])
				pc |= pe->forcecontentsmask;
		}
	}

	return pc;
}

int PM_ExtraBoxContents (vec3_t p)
{
	int			num;

	int pc = 0;
	physent_t *pe;
	model_t *pm;
	trace_t tr;

	for (num = 1; num < pmove.numphysent; num++)
	{
		pe = &pmove.physents[num];
		if (!pe->nonsolid)
			continue;
		pm = pe->model;
		if (pm)
		{
			if (pe->forcecontentsmask)
			{
				if (!PM_TransformedHullCheck(pm, PE_FRAMESTATE, p, p, pmove.player_mins, pmove.player_maxs, &tr, pe->origin, pe->angles))
					continue;
				if (tr.startsolid || tr.inwater)
					pc |= pe->forcecontentsmask;
			}
		}
		else if (pe->forcecontentsmask)
		{
			if (p[0]+pmove.player_maxs[0] >= pe->origin[0]+pe->mins[0] && p[0]+pmove.player_mins[0] <= pe->origin[0]+pe->maxs[0] && 
				p[1]+pmove.player_maxs[1] >= pe->origin[1]+pe->mins[1] && p[1]+pmove.player_mins[1] <= pe->origin[1]+pe->maxs[1] &&
				p[2]+pmove.player_maxs[2] >= pe->origin[2]+pe->mins[2] && p[2]+pmove.player_mins[2] <= pe->origin[2]+pe->maxs[2])
				pc |= pe->forcecontentsmask;
		}
	}

	return pc;
}

/*
===============================================================================

LINE TESTING IN HULLS

===============================================================================
*/

/*returns if it actually did a trace*/
static qboolean PM_TransformedHullCheck (model_t *model, framestate_t *framestate, vec3_t start, vec3_t end, vec3_t player_mins, vec3_t player_maxs, trace_t *trace, vec3_t origin, vec3_t angles)
{
	vec3_t		start_l, end_l;
	int i;
	vec3_t		axis[3];

	// subtract origin offset
	VectorSubtract (start, origin, start_l);
	VectorSubtract (end, origin, end_l);

	// sweep the box through the model
	if (model && model->funcs.NativeTrace)
	{
		if (angles[0] || angles[1] || angles[2])
		{
			AngleVectors (angles, axis[0], axis[1], axis[2]);
			VectorNegate(axis[1], axis[1]);
			model->funcs.NativeTrace(model, 0, framestate, axis, start_l, end_l, player_mins, player_maxs, pmove.capsule, MASK_PLAYERSOLID, trace);
		}
		else
		{
			for (i = 0; i < 3; i++)
			{
				if (start_l[i]+player_mins[i] > model->maxs[i] && end_l[i] + player_mins[i] > model->maxs[i])
					return false;
				if (start_l[i]+player_maxs[i] < model->mins[i] && end_l[i] + player_maxs[i] < model->mins[i])
					return false;
			}
			model->funcs.NativeTrace(model, 0, framestate, NULL, start_l, end_l, player_mins, player_maxs, pmove.capsule, MASK_PLAYERSOLID, trace);
		}
	}
	else
	{
		for (i = 0; i < 3; i++)
		{
			if (start_l[i]+player_mins[i] > box_planes[0+i*2].dist && end_l[i] + player_mins[i] > box_planes[0+i*2].dist)
				return false;
			if (start_l[i]+player_maxs[i] < box_planes[1+i*2].dist && end_l[i] + player_maxs[i] < box_planes[1+i*2].dist)
				return false;
		}

		memset (trace, 0, sizeof(trace_t));
		trace->fraction = 1;
		trace->allsolid = true;
		Q1BSP_RecursiveHullCheck (&box_hull, box_hull.firstclipnode, start_l, end_l, MASK_PLAYERSOLID, trace);
	}

	trace->endpos[0] += origin[0];
	trace->endpos[1] += origin[1];
	trace->endpos[2] += origin[2];
	return true;
}


//a portal is flush with a world surface behind it.
//this causes problems. namely that we can't pass through the portal plane if the bsp behind it prevents out origin from getting through.
//so if the trace was clipped and ended infront of the portal, continue the trace to the edges of the portal cutout instead.
static void PM_PortalCSG(physent_t *portal, int entnum, float *trmin, float *trmax, vec3_t start, vec3_t end, trace_t *trace)
{
	vec4_t planes[6];	//far, near, right, left, up, down
	int plane;
	vec3_t worldpos;
	float portalradius = 128;
	int hitplane = -1;
	float bestfrac;
	//only run this code if we impacted on the portal's parent.
	if (trace->fraction == 1 && !trace->startsolid)
		return;
	if (!portalradius)
		return;
	
	if (trace->startsolid)
		VectorCopy(start, worldpos);	//make sure we use a sane valid position.
	else
		VectorCopy(trace->endpos, worldpos);

	//determine the csg area. normals should be facing in
	AngleVectors(portal->angles, planes[1], planes[3], planes[5]);
	VectorNegate(planes[1], planes[0]);
	VectorNegate(planes[3], planes[2]);
	VectorNegate(planes[5], planes[4]);

	portalradius/=2;
	planes[0][3] = DotProduct(portal->origin, planes[0]) - (4.0/32);
	planes[1][3] = DotProduct(portal->origin, planes[1]) - (4.0/32);	//an epsilon beyond the portal. this needs to cover funny angle differences
	planes[2][3] = DotProduct(portal->origin, planes[2]) - portalradius;
	planes[3][3] = DotProduct(portal->origin, planes[3]) - portalradius;
	planes[4][3] = DotProduct(portal->origin, planes[4]) - portalradius;
	planes[5][3] = DotProduct(portal->origin, planes[5]) - portalradius;

	//if we're actually inside the csg region
	for (plane = 0; plane < 6; plane++)
	{
		vec3_t nearest;
		float d = DotProduct(worldpos, planes[plane]);
		int k;
		for (k = 0; k < 3; k++)
			nearest[k] = (planes[plane][k]>=0)?trmax[k]:trmin[k];
		if (!plane)	//front plane gets further away with side
			planes[plane][3] -= DotProduct(nearest, planes[plane]);
		else if (plane>1)	//side planes get nearer with size
			planes[plane][3] += 24;//+= DotProduct(nearest, planes[plane]);
		if (d - planes[plane][3] >= 0)
			continue;	//endpos is inside
		else
			return;		//end is already outside
	}
	//yup, we're inside, the trace shouldn't end where it actually did
	bestfrac = 1;
	hitplane = -1;
	for (plane = 0; plane < 6; plane++)
	{
		float ds = DotProduct(start, planes[plane]) - planes[plane][3];
		float de = DotProduct(end, planes[plane]) - planes[plane][3];
		float frac;
		if (ds >= 0 && de < 0)
		{
			frac = (ds - (1/32.0)) / (ds - de);
			if (frac < bestfrac)
			{
				if (frac < 0)
					frac = 0;
				hitplane = plane;
				bestfrac = frac;
				VectorInterpolate(start, frac, end, trace->endpos);
			}
		}
	}
	trace->startsolid = trace->allsolid = false;
	//if we cross the front of the portal, don't shorten the trace, that will artificially clip us
	if (hitplane == 0 && trace->fraction > bestfrac)
		return;
	//okay, elongate to clip to the portal hole properly.
	trace->fraction = bestfrac;
	VectorInterpolate(start, bestfrac, end, trace->endpos);

	if (hitplane >= 0)
	{
		VectorCopy(planes[hitplane], trace->plane.normal);
		trace->plane.dist = planes[hitplane][3];
		if (hitplane == 1)
			trace->entnum = entnum;
	}
}

/*
================
PM_TestPlayerPosition

Returns false if the given player position is not valid (in solid)
================
*/
qboolean PM_TestPlayerPosition (vec3_t pos, qboolean ignoreportals)
{
	int			i, j;
	physent_t	*pe;
	vec3_t		mins, maxs;
	hull_t		*hull;
	trace_t		trace;
	int			csged = false;

	for (i=0 ; i< pmove.numphysent ; i++)
	{
		pe = &pmove.physents[i];

		if (pe->info == pmove.skipent)
			continue;

		if (pe->nonsolid)
			continue;

		if (pe->forcecontentsmask && !(pe->forcecontentsmask & MASK_PLAYERSOLID))
			continue;

	// get the clipping hull
		if (pe->isportal)
		{
			if (ignoreportals)
				continue;
			//if the trace ended up inside a portal region, then its not valid.
			if (pe->model)
			{
				if (!PM_TransformedHullCheck (pe->model, PE_FRAMESTATE, pos, pos, vec3_origin, vec3_origin, &trace, pe->origin, pe->angles))
					continue;
				if (trace.allsolid)
					return false;
			}
			else
			{
				hull = PM_HullForBox (pe->mins, pe->maxs);
				VectorSubtract(pos, pe->origin, mins);
				if (Q1BSP_HullPointContents(hull, mins) & MASK_PLAYERSOLID)
					return false;
			}
		}
		else
		{
			if (pe->model)
			{
				if (!PM_TransformedHullCheck (pe->model, PE_FRAMESTATE, pos, pos, pmove.player_mins, pmove.player_maxs, &trace, pe->origin, pe->angles))
					continue;
				if (trace.allsolid)
				{
					for (j = i+1; j < pmove.numphysent && trace.allsolid; j++)
					{
						pe = &pmove.physents[j];
						if (pe->isportal)
							PM_PortalCSG(pe, j, pmove.player_mins, pmove.player_maxs, pos, pos, &trace);
					}
					if (trace.allsolid)
						return false;
					csged = true;
				}
			}
			else
			{
				VectorSubtract (pe->mins, pmove.player_maxs, mins);
				VectorSubtract (pe->maxs, pmove.player_mins, maxs);
				hull = PM_HullForBox (mins, maxs);
				VectorSubtract(pos, pe->origin, mins);

				if (Q1BSP_HullPointContents(hull, mins) & MASK_PLAYERSOLID)
					return false;
			}
		}
	}

	if (!csged && !ignoreportals)
	{
		//the point the player is returned to if the portal dissipates
		pmove.safeorigin_known = true;
		VectorCopy (pmove.origin, pmove.safeorigin);
	}

	return true;
}

/*
================
PM_PlayerTrace
================
*/
trace_t PM_PlayerTrace (vec3_t start, vec3_t end, unsigned int solidmask)
{
	trace_t		trace, total;
	int			i, j;
	physent_t	*pe;

// fill in a default trace
	memset (&total, 0, sizeof(trace_t));
	total.fraction = 1;
	total.entnum = -1;
	VectorCopy (end, total.endpos);

	for (i=0 ; i< pmove.numphysent ; i++)
	{
		pe = &pmove.physents[i];

		if (pe->nonsolid)
			continue;
		if (pe->info == pmove.skipent)
			continue;
		if (pe->forcecontentsmask && !(pe->forcecontentsmask & solidmask))
			continue;

		if (!pe->model || pe->model->loadstate != MLS_LOADED)
		{
			vec3_t mins, maxs;

			VectorSubtract (pe->mins, pmove.player_maxs, mins);
			VectorSubtract (pe->maxs, pmove.player_mins, maxs);
			PM_HullForBox (mins, maxs);

			// trace a line through the apropriate clipping hull
			if (!PM_TransformedHullCheck (NULL, NULL, start, end, pmove.player_mins, pmove.player_maxs, &trace, pe->origin, pe->angles))
				continue;
		}
		else if (pe->isportal)
		{
			//make sure we don't hit the world if we're inside the portal
			PM_PortalCSG(pe, i, pmove.player_mins, pmove.player_maxs, start, end, &total);

			// trace a line through the apropriate clipping hull
			if (!PM_TransformedHullCheck (pe->model, PE_FRAMESTATE, start, end, vec3_origin, vec3_origin, &trace, pe->origin, pe->angles))
				continue;
		}
		else
		{
			// trace a line through the apropriate clipping hull
			if (!PM_TransformedHullCheck (pe->model, PE_FRAMESTATE, start, end, pmove.player_mins, pmove.player_maxs, &trace, pe->origin, pe->angles))
				continue;

			if (trace.allsolid)
			{
				for (j = i+1; j < pmove.numphysent && trace.allsolid; j++)
				{
					pe = &pmove.physents[j];
					if (pe->isportal)
						PM_PortalCSG(pe, j, pmove.player_mins, pmove.player_maxs, start, end, &trace);
				}
				pe = &pmove.physents[i];
			}
		}

		if (trace.allsolid)
			trace.startsolid = true;
		if (trace.startsolid && pe->isportal)
			trace.startsolid = false;
//		if (trace.startsolid)
//			trace.fraction = 0;

	// did we clip the move?
		if (trace.fraction < total.fraction || (trace.startsolid && !total.startsolid))
		{
			// fix trace up by the offset
			total = trace;
			total.entnum = i;
		}
	}

//	//this is needed to avoid *2 friction. some id bug.
	if (total.startsolid)
		total.fraction = 0;
	return total;
}

//for use outside the pmove code. lame, but works.
trace_t PM_TraceLine (vec3_t start, vec3_t end)
{
	VectorClear(pmove.player_mins);
	VectorClear(pmove.player_maxs);
	return PM_PlayerTrace(start, end, MASK_PLAYERSOLID);
}
