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
// world.c -- world query functions

#include "quakedef.h"

#ifdef HLSERVER
#include "svhl_gcapi.h"

hull_t	*World_HullForBox (vec3_t mins, vec3_t maxs);
//qboolean TransformedTrace (struct model_s *model, int hulloverride, int frame, vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, struct trace_s *trace, vec3_t origin, vec3_t angles, unsigned int hitcontentsmask);
/*

entities never clip against themselves, or their owner

line of sight checks trace->crosscontent, but bullets don't

*/

extern cvar_t sv_compatiblehulls;

typedef struct
{
	vec3_t		boxmins, boxmaxs;// enclose the test object along entire move
	float		*mins, *maxs;	// size of the moving object
	vec3_t		mins2, maxs2;	// size when clipping against mosnters
	float		*start, *end;
	trace_t		trace;
	int			type;
	hledict_t		*passedict;
	int hullnum;
	unsigned int clipmask;
} hlmoveclip_t;

/*
===============================================================================

HULL BOXES

===============================================================================
*/


/*
===============================================================================

ENTITY AREA CHECKING

===============================================================================
*/

/*
===============
SV_UnlinkEdict

===============
*/
void SVHL_UnlinkEdict (hledict_t *ent)
{
	if (!ent->area.prev)
		return;		// not linked in anywhere
	RemoveLink (&ent->area);
	ent->area.prev = ent->area.next = NULL;
}


/*
====================
SV_TouchLinks
====================
*/
#define MAX_NODELINKS	256	//all this means is that any more than this will not touch.
hledict_t *nodelinks[MAX_NODELINKS];
void SVHL_TouchLinks ( hledict_t *ent, areanode_t *node )
{	//Spike: rewritten this function to cope with killtargets used on a few maps.
	link_t		*l, *next;
	hledict_t		*touch;

	int linkcount = 0, ln;

	//work out who they are first.
	for (l = node->edicts.next ; l != &node->edicts ; l = next)
	{
		if (linkcount == MAX_NODELINKS)
			break;
		next = l->next;
		touch = HLEDICT_FROM_AREA(l);
		if (touch == ent)
			continue;

		if (touch->v.solid != SOLID_TRIGGER)
			continue;

		if (ent->v.absmin[0] > touch->v.absmax[0]
		|| ent->v.absmin[1] > touch->v.absmax[1]
		|| ent->v.absmin[2] > touch->v.absmax[2]
		|| ent->v.absmax[0] < touch->v.absmin[0]
		|| ent->v.absmax[1] < touch->v.absmin[1]
		|| ent->v.absmax[2] < touch->v.absmin[2] )
			continue;

//		if (!((int)ent->xv.dimension_solid & (int)touch->xv.dimension_hit))
//			continue;

		nodelinks[linkcount++] = touch;
	}

	for (ln = 0; ln < linkcount; ln++)
	{
		touch = nodelinks[ln];

		//make sure nothing moved it away
		if (touch->isfree)
			continue;
		if (touch->v.solid != SOLID_TRIGGER)
			continue;
		if (ent->v.absmin[0] > touch->v.absmax[0]
		|| ent->v.absmin[1] > touch->v.absmax[1]
		|| ent->v.absmin[2] > touch->v.absmax[2]
		|| ent->v.absmax[0] < touch->v.absmin[0]
		|| ent->v.absmax[1] < touch->v.absmin[1]
		|| ent->v.absmax[2] < touch->v.absmin[2] )
			continue;

//		if (!((int)ent->xv->dimension_solid & (int)touch->xv->dimension_hit))	//didn't change did it?...
//			continue;

		SVHL_GameFuncs.DispatchTouch(touch, ent);

		if (ent->isfree)
			break;
	}


// recurse down both sides
	if (node->axis == -1 || ent->isfree)
		return;
	
	if ( ent->v.absmax[node->axis] > node->dist )
		SVHL_TouchLinks ( ent, node->children[0] );
	if ( ent->v.absmin[node->axis] < node->dist )
		SVHL_TouchLinks ( ent, node->children[1] );
}

/*
===============
SV_LinkEdict

===============
*/
void SVHL_LinkEdict (hledict_t *ent, qboolean touch_triggers)
{
	areanode_t	*node;
	
	if (ent->area.prev)
		SVHL_UnlinkEdict (ent);	// unlink from old position

	if (ent == &SVHL_Edict[0])
		return;		// don't add the world

	if (ent->isfree)
		return;

// set the abs box
	if (ent->v.solid == SOLID_BSP && 
	(ent->v.angles[0] || ent->v.angles[1] || ent->v.angles[2]) )
	{	// expand for rotation

#if 1
		int i;
		float v;
		float max;
		//q2 method
		max = 0;
		for (i=0 ; i<3 ; i++)
		{
			v =fabs( ent->v.mins[i]);
			if (v > max)
				max = v;
			v =fabs( ent->v.maxs[i]);
			if (v > max)
				max = v;
		}
		for (i=0 ; i<3 ; i++)
		{
			ent->v.absmin[i] = ent->v.origin[i] - max;
			ent->v.absmax[i] = ent->v.origin[i] + max;
		}
#else

		int			i;

		vec3_t f, r, u;
		vec3_t mn, mx;

		//we need to link to the correct leaves

		AngleVectors(ent->v.angles, f,r,u);

		mn[0] = DotProduct(ent->v.mins, f);
		mn[1] = -DotProduct(ent->v.mins, r);
		mn[2] = DotProduct(ent->v.mins, u);

		mx[0] = DotProduct(ent->v.maxs, f);
		mx[1] = -DotProduct(ent->v.maxs, r);
		mx[2] = DotProduct(ent->v.maxs, u);
		for (i = 0; i < 3; i++)
		{
			if (mn[i] < mx[i])
			{
				ent->v.absmin[i] = ent->v.origin[i]+mn[i]-0.1;
				ent->v.absmax[i] = ent->v.origin[i]+mx[i]+0.1;
			}
			else
			{	//box went inside out
				ent->v.absmin[i] = ent->v.origin[i]+mx[i]-0.1;
				ent->v.absmax[i] = ent->v.origin[i]+mn[i]+0.1;
			}
		}
#endif
	}
	else
	{
		VectorAdd (ent->v.origin, ent->v.mins, ent->v.absmin);	
		VectorAdd (ent->v.origin, ent->v.maxs, ent->v.absmax);
	}

//
// to make items easier to pick up and allow them to be grabbed off
// of shelves, the abs sizes are expanded
//
	if ((int)ent->v.flags & FL_ITEM)
	{
		ent->v.absmin[0] -= 15;
		ent->v.absmin[1] -= 15;
		ent->v.absmin[2] -= 1;
		ent->v.absmax[0] += 15;
		ent->v.absmax[1] += 15;
		ent->v.absmax[2] += 1;
	}
	else
	{	// because movement is clipped an epsilon away from an actual edge,
		// we must fully check even when bounding boxes don't quite touch
		ent->v.absmin[0] -= 1;
		ent->v.absmin[1] -= 1;
		ent->v.absmin[2] -= 1;
		ent->v.absmax[0] += 1;
		ent->v.absmax[1] += 1;
		ent->v.absmax[2] += 1;
	}
	
// link to PVS leafs
//	sv.worldmodel->funcs.FindTouchedLeafs_Q1(sv.worldmodel, ent, ent->v.absmin, ent->v.absmax);

// find the first node that the ent's box crosses
	node = sv.world.areanodes;
	while (1)
	{
		if (node->axis == -1)
			break;
		if (ent->v.absmin[node->axis] > node->dist)
			node = node->children[0];
		else if (ent->v.absmax[node->axis] < node->dist)
			node = node->children[1];
		else
			break;		// crosses the node
	}
	
// link it in	

	InsertLinkBefore (&ent->area, &node->edicts);
	
// if touch_triggers, touch all entities at this node and decend for more
	if (touch_triggers)
		SVHL_TouchLinks ( ent, sv.world.areanodes );
}


/*
===============================================================================

POINT TESTING IN HULLS

===============================================================================
*/

/*
==================
SV_PointContents

==================
*/
int SVHL_PointContents (vec3_t p)
{
	return sv.world.worldmodel->funcs.PointContents(sv.world.worldmodel, NULL, p);
}

//===========================================================================

/*
============
SV_TestEntityPosition

A small wrapper around SV_BoxInSolidEntity that never clips against the
supplied entity.
============
*/
hledict_t	*SVHL_TestEntityPosition (hledict_t *ent)
{
	trace_t	trace;

	trace = SVHL_Move (ent->v.origin, ent->v.mins, ent->v.maxs, ent->v.origin, 0, 0, ent);
	
	if (trace.startsolid)
		return &SVHL_Edict[0];
		
	return NULL;
}

/*
==================
SV_ClipMoveToEntity

Handles selection or creation of a clipping hull, and offseting (and
eventually rotation) of the end points
==================
*/
trace_t SVHL_ClipMoveToEntity (hledict_t *ent, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int hullnum, qboolean hitmodel, unsigned int clipmask)	//hullnum overrides min/max for q1 style bsps
{
	trace_t		trace;
	model_t		*model;

/*
#ifdef Q2BSPS
	if (ent->v->solid == SOLID_BSP)
		if (sv.models[(int)ent->v->modelindex] && (sv.models[(int)ent->v->modelindex]->fromgame == fg_quake2 || sv.models[(int)ent->v->modelindex]->fromgame == fg_quake3))
		{
			trace = CM_TransformedBoxTrace (start, end, mins, maxs, sv.models[(int)ent->v->modelindex]->hulls[0].firstclipnode, MASK_PLAYERSOLID, ent->v->origin, ent->v->angles);
			if (trace.fraction < 1 || trace.startsolid)
				trace.ent = ent;
			return trace;
		}
#endif
*/

// get the clipping hull
	if (ent->v.solid == SOLID_BSP)
	{
		model = sv.models[(int)ent->v.modelindex];
		if (!model || (model->type != mod_brush && model->type != mod_heightmap))
			SV_Error("SOLID_BSP with non bsp model (classname: %s)", SVHL_Globals.stringbase+ent->v.classname);
	}
	else
	{
		vec3_t boxmins, boxmaxs;
		VectorSubtract (ent->v.mins, maxs, boxmins);
		VectorSubtract (ent->v.maxs, mins, boxmaxs);
		World_HullForBox(boxmins, boxmaxs);
		model = NULL;
	}

// trace a line through the apropriate clipping hull
	if (ent->v.solid != SOLID_BSP)
	{
		ent->v.angles[0]*=r_meshpitch.value;	//carmack made bsp models rotate wrongly.
		World_TransformedTrace(model, hullnum, ent->v.frame, start, end, mins, maxs, false, &trace, ent->v.origin, ent->v.angles, clipmask);
		ent->v.angles[0]*=r_meshpitch.value;
	}
	else
	{
		World_TransformedTrace(model, hullnum, ent->v.frame, start, end, mins, maxs, false, &trace, ent->v.origin, ent->v.angles, clipmask);
	}

// fix trace up by the offset
	if (trace.fraction != 1)
	{
		if (!model && hitmodel && ent->v.solid != SOLID_BSP && ent->v.modelindex > 0)
		{
			//okay, we hit the bbox

			model_t *model;
			if (ent->v.modelindex < 1 || ent->v.modelindex >= MAX_PRECACHE_MODELS)
				SV_Error("SV_ClipMoveToEntity: modelindex out of range\n");
			model = sv.models[ (int)ent->v.modelindex ];
			if (!model)
			{	//if the model isn't loaded, load it.
				//this saves on memory requirements with mods that don't ever use this.
				model = sv.models[(int)ent->v.modelindex] = Mod_ForName(sv.strings.model_precache[(int)ent->v.modelindex], false);
			}

			if (model && model->funcs.NativeTrace)
			{
				//do the second trace
				World_TransformedTrace(model, hullnum, ent->v.frame, start, end, mins, maxs, false, &trace, ent->v.origin, ent->v.angles, MASK_WORLDSOLID);
			}
		}

		if (trace.startsolid)
		{
			if (ent != &SVHL_Edict[0])
				Con_Printf("Trace started solid\n");
		}
	}

// did we clip the move?
	if (trace.fraction < 1 || trace.startsolid  )
		trace.ent = ent;

	return trace;
}

#ifdef Q2BSPS
float	*area_mins, *area_maxs;
hledict_t	**area_list;
int		area_count, area_maxcount;
void SVHL_AreaEdicts_r (areanode_t *node)
{
	link_t		*l, *next, *start;
	hledict_t		*check;
	int			count;

	count = 0;

	// touch linked edicts
	start = &node->edicts;

	for (l=start->next  ; l != start ; l = next)
	{
		next = l->next;
		check = HLEDICT_FROM_AREA(l);

//		if (check->v.solid == SOLID_NOT)
//			continue;		// deactivated
		if (check->v.absmin[0] > area_maxs[0]
		|| check->v.absmin[1] > area_maxs[1]
		|| check->v.absmin[2] > area_maxs[2]
		|| check->v.absmax[0] < area_mins[0]
		|| check->v.absmax[1] < area_mins[1]
		|| check->v.absmax[2] < area_mins[2])
			continue;		// not touching

		if (area_count == area_maxcount)
		{
			Con_Printf ("SV_AreaEdicts: MAXCOUNT\n");
			return;
		}

		area_list[area_count] = check;
		area_count++;
	}
	
	if (node->axis == -1)
		return;		// terminal node

	// recurse down both sides
	if ( area_maxs[node->axis] > node->dist )
		SVHL_AreaEdicts_r ( node->children[0] );
	if ( area_mins[node->axis] < node->dist )
		SVHL_AreaEdicts_r ( node->children[1] );
}

/*
================
SV_AreaEdicts
================
*/
int SVHL_AreaEdicts (vec3_t mins, vec3_t maxs, hledict_t **list, int maxcount)
{
	area_mins = mins;
	area_maxs = maxs;
	area_list = list;
	area_count = 0;
	area_maxcount = maxcount;

	SVHL_AreaEdicts_r (sv.world.areanodes);

	return area_count;
}

#endif
//===========================================================================


/*
====================
SV_ClipToEverything

like SV_ClipToLinks, but doesn't use the links part. This can be used for checking triggers, solid entities, not-solid entities.
Sounds pointless, I know.
====================
*/
void SVHL_ClipToEverything (hlmoveclip_t *clip)
{
	int e;
	trace_t		trace;
	hledict_t		*touch;
	for (e=1 ; e<sv.world.num_edicts ; e++)
	{
		touch = &SVHL_Edict[e];

		if (touch->isfree)
			continue;                 
		if (touch->v.solid == SOLID_NOT && !((int)touch->v.flags & FL_FINDABLE_NONSOLID))
			continue;
		if (touch->v.solid == SOLID_TRIGGER && !((int)touch->v.flags & FL_FINDABLE_NONSOLID))
			continue;

		if (touch == clip->passedict)
			continue;

		if (clip->type & MOVE_NOMONSTERS && touch->v.solid != SOLID_BSP)
			continue;

		if (clip->passedict)
		{
			// don't clip corpse against character
			if (clip->passedict->v.solid == SOLID_CORPSE && (touch->v.solid == SOLID_SLIDEBOX || touch->v.solid == SOLID_CORPSE))
				continue;
			// don't clip character against corpse
			if (clip->passedict->v.solid == SOLID_SLIDEBOX && touch->v.solid == SOLID_CORPSE)
				continue;

//			if (!((int)clip->passedict->v.dimension_hit & (int)touch->v.dimension_solid))
//				continue;
		}

		if (clip->boxmins[0] > touch->v.absmax[0]
				|| clip->boxmins[1] > touch->v.absmax[1]
				|| clip->boxmins[2] > touch->v.absmax[2]
				|| clip->boxmaxs[0] < touch->v.absmin[0]
				|| clip->boxmaxs[1] < touch->v.absmin[1]
				|| clip->boxmaxs[2] < touch->v.absmin[2] )
			continue;

		if (clip->passedict && clip->passedict->v.size[0] && !touch->v.size[0])
			continue;	// points never interact

	// might intersect, so do an exact clip
		if (clip->trace.allsolid)
			return;
		if (clip->passedict)
		{
		 	if (touch->v.owner == clip->passedict)
				continue;	// don't clip against own missiles
			if (clip->passedict->v.owner == touch)
				continue;	// don't clip against owner
		}

		if ((int)touch->v.flags & FL_MONSTER)
			trace = SVHL_ClipMoveToEntity (touch, clip->start, clip->mins2, clip->maxs2, clip->end, clip->hullnum, clip->type & MOVE_HITMODEL, clip->clipmask);
		else
			trace = SVHL_ClipMoveToEntity (touch, clip->start, clip->mins, clip->maxs, clip->end, clip->hullnum, clip->type & MOVE_HITMODEL, clip->clipmask);
		if (trace.allsolid || trace.startsolid ||
				trace.fraction < clip->trace.fraction)
		{
			trace.ent = touch;
			clip->trace = trace;
		}
	}
}

/*
====================
SV_ClipToLinks

Mins and maxs enclose the entire area swept by the move
====================
*/
void SVHL_ClipToLinks ( areanode_t *node, hlmoveclip_t *clip )
{
	link_t		*l, *next;
	hledict_t		*touch;
	trace_t		trace;

// touch linked edicts
	for (l = node->edicts.next ; l != &node->edicts ; l = next)
	{
		next = l->next;
		touch = HLEDICT_FROM_AREA(l);
		if (touch->v.solid == SOLID_NOT)
			continue;
		if (touch == clip->passedict)
			continue;
		if (touch->v.solid == SOLID_TRIGGER)
			continue;

		if (clip->type & MOVE_NOMONSTERS && touch->v.solid != SOLID_BSP)
			continue;

		if (clip->passedict)
		{
			// don't clip corpse against character
			if (clip->passedict->v.solid == SOLID_CORPSE && (touch->v.solid == SOLID_SLIDEBOX || touch->v.solid == SOLID_CORPSE))
				continue;
			// don't clip character against corpse
			if (clip->passedict->v.solid == SOLID_SLIDEBOX && touch->v.solid == SOLID_CORPSE)
				continue;

//			if (!((int)clip->passedict->xv->dimension_hit & (int)touch->xv->dimension_solid))
//				continue;
		}

		if (clip->boxmins[0] > touch->v.absmax[0]
		|| clip->boxmins[1] > touch->v.absmax[1]
		|| clip->boxmins[2] > touch->v.absmax[2]
		|| clip->boxmaxs[0] < touch->v.absmin[0]
		|| clip->boxmaxs[1] < touch->v.absmin[1]
		|| clip->boxmaxs[2] < touch->v.absmin[2] )
			continue;

		if (clip->passedict && clip->passedict->v.size[0] && !touch->v.size[0])
			continue;	// points never interact

	// might intersect, so do an exact clip
		if (clip->trace.allsolid)
			return;
		if (clip->passedict)
		{
		 	if (touch->v.owner == clip->passedict)
				continue;	// don't clip against own missiles
			if (clip->passedict->v.owner == touch)
				continue;	// don't clip against owner
		}

		if ((int)touch->v.flags & FL_MONSTER)
			trace = SVHL_ClipMoveToEntity (touch, clip->start, clip->mins2, clip->maxs2, clip->end, clip->hullnum, clip->type & MOVE_HITMODEL, clip->clipmask);
		else
			trace = SVHL_ClipMoveToEntity (touch, clip->start, clip->mins, clip->maxs, clip->end, clip->hullnum, clip->type & MOVE_HITMODEL, clip->clipmask);
		if (trace.allsolid || trace.startsolid ||
		trace.fraction < clip->trace.fraction)
		{
			trace.ent = touch;
			clip->trace = trace;
		}
	}
	
// recurse down both sides
	if (node->axis == -1)
		return;

	if ( clip->boxmaxs[node->axis] > node->dist )
		SVHL_ClipToLinks ( node->children[0], clip );
	if ( clip->boxmins[node->axis] < node->dist )
		SVHL_ClipToLinks ( node->children[1], clip );
}

static void SVHL_MoveBounds (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, vec3_t boxmins, vec3_t boxmaxs)
{
#if 0
// debug to test against everything
boxmins[0] = boxmins[1] = boxmins[2] = -9999;
boxmaxs[0] = boxmaxs[1] = boxmaxs[2] = 9999;
#else
	int		i;
	
	for (i=0 ; i<3 ; i++)
	{
		if (end[i] > start[i])
		{
			boxmins[i] = start[i] + mins[i] - 1;
			boxmaxs[i] = end[i] + maxs[i] + 1;
		}
		else
		{
			boxmins[i] = end[i] + mins[i] - 1;
			boxmaxs[i] = start[i] + maxs[i] + 1;
		}
	}
#endif
}

/*
==================
SV_Move
==================
*/
trace_t SVHL_Move (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int type, int forcehull, hledict_t *passedict)
{
	hlmoveclip_t	clip;
	int			i;
	int hullnum;

	memset ( &clip, 0, sizeof ( clip ) );
	if (forcehull)
		hullnum = forcehull;
	else if (sv_compatiblehulls.value)
		hullnum = 0;
	else
	{
		int diff;
		int best;
		hullnum = 0;
		best = 8192;
		//x/y pos/neg are assumed to be the same magnitute.
		//z pos/height are assumed to be different from all the others.
		for (i = 0; i < MAX_MAP_HULLSM; i++)
		{
			if (!sv.world.worldmodel->hulls[i].available)
				continue;
#define sq(x) ((x)*(x))
			diff = sq(sv.world.worldmodel->hulls[i].clip_maxs[2] - maxs[2]) +
				sq(sv.world.worldmodel->hulls[i].clip_mins[2] - mins[2]) +
				sq(sv.world.worldmodel->hulls[i].clip_maxs[1] - maxs[1]) +
				sq(sv.world.worldmodel->hulls[i].clip_mins[0] - mins[0]);
			if (diff < best)
			{
				best = diff;
				hullnum=i;
			}
		}
		hullnum++;
	}

	if (type & MOVE_NOMONSTERS)
		clip.clipmask = MASK_WORLDSOLID; /*solid only to world*/
	else if (maxs[0] - mins[0])
		clip.clipmask = MASK_BOXSOLID;	/*impacts playerclip*/
	else
		clip.clipmask = MASK_POINTSOLID;		/*ignores playerclip but hits everything else*/

// clip to world
	clip.trace = SVHL_ClipMoveToEntity ( &SVHL_Edict[0], start, mins, maxs, end, hullnum, false, clip.clipmask);

	clip.start = start;
	clip.end = end;
	clip.mins = mins;
	clip.maxs = maxs;
	clip.type = type;
	clip.passedict = passedict;
	clip.hullnum = hullnum;

	if (type & MOVE_MISSILE)
	{
		for (i=0 ; i<3 ; i++)
		{
			clip.mins2[i] = -15;
			clip.maxs2[i] = 15;
		}
	}
	else
	{
		VectorCopy (mins, clip.mins2);
		VectorCopy (maxs, clip.maxs2);
	}
	
// create the bounding box of the entire move
	SVHL_MoveBounds ( start, clip.mins2, clip.maxs2, end, clip.boxmins, clip.boxmaxs );

// clip to entities
	if (clip.type & MOVE_EVERYTHING)
		SVHL_ClipToEverything (&clip);
	else
		SVHL_ClipToLinks ( sv.world.areanodes, &clip );

	if (clip.trace.startsolid)
		clip.trace.fraction = 0;

	if (!clip.trace.ent)
		return clip.trace;

	return clip.trace;
}

#endif
