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
#include "prvm_cmds.h"

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

#define	MOVE_EPSILON	0.01

void SV_Physics_Toss (prvm_edict_t *ent);

int SV_GetPitchSign(prvm_prog_t *prog, prvm_edict_t *ent)
{
	model_t *model;
	if (
			(model = SV_GetModelFromEdict(ent))
			?
			model->type == mod_alias
			:
			(
			 (((unsigned char)PRVM_serveredictfloat(ent, pflags)) & PFLAGS_FULLDYNAMIC)
			 ||
			 ((gamemode == GAME_TENEBRAE) && ((unsigned int)PRVM_serveredictfloat(ent, effects) & (16 | 32)))
			)
	   )
		return -1;
	return 1;
}

/*
===============================================================================

LINE TESTING IN HULLS

===============================================================================
*/

int SV_GenericHitSuperContentsMask(const prvm_edict_t *passedict)
{
	prvm_prog_t *prog = SVVM_prog;
	if (passedict)
	{
		int dphitcontentsmask = (int)PRVM_serveredictfloat(passedict, dphitcontentsmask);
		if (dphitcontentsmask)
			return dphitcontentsmask;
		else if (PRVM_serveredictfloat(passedict, solid) == SOLID_SLIDEBOX)
		{
			if ((int)PRVM_serveredictfloat(passedict, flags) & FL_MONSTER)
				return SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY | SUPERCONTENTS_MONSTERCLIP;
			else
				return SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY | SUPERCONTENTS_PLAYERCLIP;
		}
		else if (PRVM_serveredictfloat(passedict, solid) == SOLID_CORPSE)
			return SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY;
		else if (PRVM_serveredictfloat(passedict, solid) == SOLID_TRIGGER)
			return SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY;
		else
			return SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY | SUPERCONTENTS_CORPSE;
	}
	else
		return SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY | SUPERCONTENTS_CORPSE;
}

/*
==================
SV_TracePoint
==================
*/
trace_t SV_TracePoint(const vec3_t start, int type, prvm_edict_t *passedict, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask)
{
	prvm_prog_t *prog = SVVM_prog;
	int i, bodysupercontents;
	int passedictprog;
	float pitchsign = 1;
	prvm_edict_t *traceowner, *touch;
	trace_t trace;
	// temporary storage because prvm_vec_t may differ from vec_t
	vec3_t touchmins, touchmaxs;
	// bounding box of entire move area
	vec3_t clipboxmins, clipboxmaxs;
	// size when clipping against monsters
	vec3_t clipmins2, clipmaxs2;
	// start and end origin of move
	vec3_t clipstart;
	// trace results
	trace_t cliptrace;
	// matrices to transform into/out of other entity's space
	matrix4x4_t matrix, imatrix;
	// model of other entity
	model_t *model;
	// list of entities to test for collisions
	int numtouchedicts;
	static prvm_edict_t *touchedicts[MAX_EDICTS];
	int clipgroup;

	//return SV_TraceBox(start, vec3_origin, vec3_origin, end, type, passedict, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask);

	VectorCopy(start, clipstart);
	VectorClear(clipmins2);
	VectorClear(clipmaxs2);
#if COLLISIONPARANOID >= 3
	Con_Printf("move(%f %f %f)", clipstart[0], clipstart[1], clipstart[2]);
#endif

	// clip to world
	Collision_ClipPointToWorld(&cliptrace, sv.worldmodel, clipstart, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask);
	cliptrace.worldstartsolid = cliptrace.bmodelstartsolid = cliptrace.startsolid;
	if (cliptrace.startsolid || cliptrace.fraction < 1)
		cliptrace.ent = prog->edicts;
	if (type == MOVE_WORLDONLY)
		goto finished;

	if (type == MOVE_MISSILE)
	{
		// LadyHavoc: modified this, was = -15, now -= 15
		for (i = 0;i < 3;i++)
		{
			clipmins2[i] -= 15;
			clipmaxs2[i] += 15;
		}
	}

	// create the bounding box of the entire move
	for (i = 0;i < 3;i++)
	{
		clipboxmins[i] = min(clipstart[i], cliptrace.endpos[i]) + clipmins2[i] - 1;
		clipboxmaxs[i] = max(clipstart[i], cliptrace.endpos[i]) + clipmaxs2[i] + 1;
	}

	// debug override to test against everything
	if (sv_debugmove.integer)
	{
		clipboxmins[0] = clipboxmins[1] = clipboxmins[2] = (vec_t)-999999999;
		clipboxmaxs[0] = clipboxmaxs[1] = clipboxmaxs[2] =  (vec_t)999999999;
	}

	// if the passedict is world, make it NULL (to avoid two checks each time)
	if (passedict == prog->edicts)
		passedict = NULL;
	// precalculate prog value for passedict for comparisons
	passedictprog = PRVM_EDICT_TO_PROG(passedict);
	// precalculate passedict's owner edict pointer for comparisons
	traceowner = passedict ? PRVM_PROG_TO_EDICT(PRVM_serveredictedict(passedict, owner)) : 0;

	clipgroup = passedict ? (int)PRVM_serveredictfloat(passedict, clipgroup) : 0;

	// clip to entities
	// because this uses World_EntitiestoBox, we know all entity boxes overlap
	// the clip region, so we can skip culling checks in the loop below
	numtouchedicts = SV_EntitiesInBox(clipboxmins, clipboxmaxs, MAX_EDICTS, touchedicts);
	if (numtouchedicts > MAX_EDICTS)
	{
		// this never happens
		Con_Printf("SV_EntitiesInBox returned %i edicts, max was %i\n", numtouchedicts, MAX_EDICTS);
		numtouchedicts = MAX_EDICTS;
	}
	for (i = 0;i < numtouchedicts;i++)
	{
		touch = touchedicts[i];

		if (PRVM_serveredictfloat(touch, solid) < SOLID_BBOX)
			continue;
		if (type == MOVE_NOMONSTERS && PRVM_serveredictfloat(touch, solid) != SOLID_BSP)
			continue;

		if (passedict)
		{
			// don't clip against self
			if (passedict == touch)
				continue;
			// don't clip owned entities against owner
			if (traceowner == touch)
				continue;
			// don't clip owner against owned entities
			if (passedictprog == PRVM_serveredictedict(touch, owner))
				continue;
			// don't clip against any entities in the same clipgroup (DP_RM_CLIPGROUP)
			if (clipgroup && clipgroup == (int)PRVM_serveredictfloat(touch, clipgroup))
				continue;
			// don't clip points against points (they can't collide)
			if (VectorCompare(PRVM_serveredictvector(touch, mins), PRVM_serveredictvector(touch, maxs)) && (type != MOVE_MISSILE || !((int)PRVM_serveredictfloat(touch, flags) & FL_MONSTER)))
				continue;
		}

		bodysupercontents = PRVM_serveredictfloat(touch, solid) == SOLID_CORPSE ? SUPERCONTENTS_CORPSE : SUPERCONTENTS_BODY;

		// might interact, so do an exact clip
		model = NULL;
		if ((int) PRVM_serveredictfloat(touch, solid) == SOLID_BSP || type == MOVE_HITMODEL)
		{
			model = SV_GetModelFromEdict(touch);
			pitchsign = SV_GetPitchSign(prog, touch);
		}
		if (model)
			Matrix4x4_CreateFromQuakeEntity(&matrix, PRVM_serveredictvector(touch, origin)[0], PRVM_serveredictvector(touch, origin)[1], PRVM_serveredictvector(touch, origin)[2], pitchsign * PRVM_serveredictvector(touch, angles)[0], PRVM_serveredictvector(touch, angles)[1], PRVM_serveredictvector(touch, angles)[2], 1);
		else
			Matrix4x4_CreateTranslate(&matrix, PRVM_serveredictvector(touch, origin)[0], PRVM_serveredictvector(touch, origin)[1], PRVM_serveredictvector(touch, origin)[2]);
		Matrix4x4_Invert_Simple(&imatrix, &matrix);
		VM_GenerateFrameGroupBlend(prog, touch->priv.server->framegroupblend, touch);
		VM_FrameBlendFromFrameGroupBlend(touch->priv.server->frameblend, touch->priv.server->framegroupblend, model, sv.time);
		VM_UpdateEdictSkeleton(prog, touch, model, touch->priv.server->frameblend);
		VectorCopy(PRVM_serveredictvector(touch, mins), touchmins);
		VectorCopy(PRVM_serveredictvector(touch, maxs), touchmaxs);
		if (type == MOVE_MISSILE && (int)PRVM_serveredictfloat(touch, flags) & FL_MONSTER)
			Collision_ClipToGenericEntity(&trace, model, touch->priv.server->frameblend, &touch->priv.server->skeleton, touchmins, touchmaxs, bodysupercontents, &matrix, &imatrix, clipstart, clipmins2, clipmaxs2, clipstart, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, 0.0f);
		else
			Collision_ClipPointToGenericEntity(&trace, model, touch->priv.server->frameblend, &touch->priv.server->skeleton, touchmins, touchmaxs, bodysupercontents, &matrix, &imatrix, clipstart, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask);

		Collision_CombineTraces(&cliptrace, &trace, (void *)touch, PRVM_serveredictfloat(touch, solid) == SOLID_BSP);
	}

finished:
	return cliptrace;
}

/*
==================
SV_TraceLine
==================
*/
trace_t SV_TraceLine(const vec3_t start, const vec3_t end, int type, prvm_edict_t *passedict, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask, float extend)
{
	prvm_prog_t *prog = SVVM_prog;
	int i, bodysupercontents;
	int passedictprog;
	float pitchsign = 1;
	prvm_edict_t *traceowner, *touch;
	trace_t trace;
	// temporary storage because prvm_vec_t may differ from vec_t
	vec3_t touchmins, touchmaxs;
	// bounding box of entire move area
	vec3_t clipboxmins, clipboxmaxs;
	// size when clipping against monsters
	vec3_t clipmins2, clipmaxs2;
	// start and end origin of move
	vec3_t clipstart, clipend;
	// trace results
	trace_t cliptrace;
	// matrices to transform into/out of other entity's space
	matrix4x4_t matrix, imatrix;
	// model of other entity
	model_t *model;
	// list of entities to test for collisions
	int numtouchedicts;
	static prvm_edict_t *touchedicts[MAX_EDICTS];
	int clipgroup;
	if (VectorCompare(start, end))
		return SV_TracePoint(start, type, passedict, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask);

	//return SV_TraceBox(start, vec3_origin, vec3_origin, end, type, passedict, hitsupercontentsmask);

	VectorCopy(start, clipstart);
	VectorCopy(end, clipend);
	VectorClear(clipmins2);
	VectorClear(clipmaxs2);
#if COLLISIONPARANOID >= 3
	Con_Printf("move(%f %f %f,%f %f %f)", clipstart[0], clipstart[1], clipstart[2], clipend[0], clipend[1], clipend[2]);
#endif

	// clip to world
	Collision_ClipLineToWorld(&cliptrace, sv.worldmodel, clipstart, clipend, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, extend, false);
	cliptrace.worldstartsolid = cliptrace.bmodelstartsolid = cliptrace.startsolid;
	if (cliptrace.startsolid || cliptrace.fraction < 1)
		cliptrace.ent = prog->edicts;
	if (type == MOVE_WORLDONLY)
		goto finished;

	if (type == MOVE_MISSILE)
	{
		// LadyHavoc: modified this, was = -15, now -= 15
		for (i = 0;i < 3;i++)
		{
			clipmins2[i] -= 15;
			clipmaxs2[i] += 15;
		}
	}

	// create the bounding box of the entire move
	for (i = 0;i < 3;i++)
	{
		clipboxmins[i] = min(clipstart[i], cliptrace.endpos[i]) + clipmins2[i] - 1;
		clipboxmaxs[i] = max(clipstart[i], cliptrace.endpos[i]) + clipmaxs2[i] + 1;
	}

	// debug override to test against everything
	if (sv_debugmove.integer)
	{
		clipboxmins[0] = clipboxmins[1] = clipboxmins[2] = (vec_t)-999999999;
		clipboxmaxs[0] = clipboxmaxs[1] = clipboxmaxs[2] =  (vec_t)999999999;
	}

	// if the passedict is world, make it NULL (to avoid two checks each time)
	if (passedict == prog->edicts)
		passedict = NULL;
	// precalculate prog value for passedict for comparisons
	passedictprog = PRVM_EDICT_TO_PROG(passedict);
	// precalculate passedict's owner edict pointer for comparisons
	traceowner = passedict ? PRVM_PROG_TO_EDICT(PRVM_serveredictedict(passedict, owner)) : 0;

	clipgroup = passedict ? (int)PRVM_serveredictfloat(passedict, clipgroup) : 0;

	// clip to entities
	// because this uses World_EntitiestoBox, we know all entity boxes overlap
	// the clip region, so we can skip culling checks in the loop below
	numtouchedicts = SV_EntitiesInBox(clipboxmins, clipboxmaxs, MAX_EDICTS, touchedicts);
	if (numtouchedicts > MAX_EDICTS)
	{
		// this never happens
		Con_Printf("SV_EntitiesInBox returned %i edicts, max was %i\n", numtouchedicts, MAX_EDICTS);
		numtouchedicts = MAX_EDICTS;
	}
	for (i = 0;i < numtouchedicts;i++)
	{
		touch = touchedicts[i];

		if (PRVM_serveredictfloat(touch, solid) < SOLID_BBOX)
			continue;
		if (type == MOVE_NOMONSTERS && PRVM_serveredictfloat(touch, solid) != SOLID_BSP)
			continue;

		if (passedict)
		{
			// don't clip against self
			if (passedict == touch)
				continue;
			// don't clip owned entities against owner
			if (traceowner == touch)
				continue;
			// don't clip owner against owned entities
			if (passedictprog == PRVM_serveredictedict(touch, owner))
				continue;
			// don't clip against any entities in the same clipgroup (DP_RM_CLIPGROUP)
			if (clipgroup && clipgroup == (int)PRVM_serveredictfloat(touch, clipgroup))
				continue;
			// don't clip points against points (they can't collide)
			if (VectorCompare(PRVM_serveredictvector(touch, mins), PRVM_serveredictvector(touch, maxs)) && (type != MOVE_MISSILE || !((int)PRVM_serveredictfloat(touch, flags) & FL_MONSTER)))
				continue;
		}

		bodysupercontents = PRVM_serveredictfloat(touch, solid) == SOLID_CORPSE ? SUPERCONTENTS_CORPSE : SUPERCONTENTS_BODY;

		// might interact, so do an exact clip
		model = NULL;
		if ((int) PRVM_serveredictfloat(touch, solid) == SOLID_BSP || type == MOVE_HITMODEL)
		{
			model = SV_GetModelFromEdict(touch);
			pitchsign = SV_GetPitchSign(prog, touch);
		}
		if (model)
			Matrix4x4_CreateFromQuakeEntity(&matrix, PRVM_serveredictvector(touch, origin)[0], PRVM_serveredictvector(touch, origin)[1], PRVM_serveredictvector(touch, origin)[2], pitchsign * PRVM_serveredictvector(touch, angles)[0], PRVM_serveredictvector(touch, angles)[1], PRVM_serveredictvector(touch, angles)[2], 1);
		else
			Matrix4x4_CreateTranslate(&matrix, PRVM_serveredictvector(touch, origin)[0], PRVM_serveredictvector(touch, origin)[1], PRVM_serveredictvector(touch, origin)[2]);
		Matrix4x4_Invert_Simple(&imatrix, &matrix);
		VM_GenerateFrameGroupBlend(prog, touch->priv.server->framegroupblend, touch);
		VM_FrameBlendFromFrameGroupBlend(touch->priv.server->frameblend, touch->priv.server->framegroupblend, model, sv.time);
		VM_UpdateEdictSkeleton(prog, touch, model, touch->priv.server->frameblend);
		VectorCopy(PRVM_serveredictvector(touch, mins), touchmins);
		VectorCopy(PRVM_serveredictvector(touch, maxs), touchmaxs);
		if (type == MOVE_MISSILE && (int)PRVM_serveredictfloat(touch, flags) & FL_MONSTER)
			Collision_ClipToGenericEntity(&trace, model, touch->priv.server->frameblend, &touch->priv.server->skeleton, touchmins, touchmaxs, bodysupercontents, &matrix, &imatrix, clipstart, clipmins2, clipmaxs2, clipend, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, extend);
		else
			Collision_ClipLineToGenericEntity(&trace, model, touch->priv.server->frameblend, &touch->priv.server->skeleton, touchmins, touchmaxs, bodysupercontents, &matrix, &imatrix, clipstart, clipend, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, extend, false);

		Collision_CombineTraces(&cliptrace, &trace, (void *)touch, PRVM_serveredictfloat(touch, solid) == SOLID_BSP);
	}

finished:
	return cliptrace;
}

/*
==================
SV_Move
==================
*/
#if COLLISIONPARANOID >= 1
static trace_t SV_TraceBox_(const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int type, prvm_edict_t *passedict, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask, float extend)
#else
trace_t SV_TraceBox(const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int type, prvm_edict_t *passedict, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask, float extend)
#endif
{
	prvm_prog_t *prog = SVVM_prog;
	vec3_t hullmins, hullmaxs;
	int i, bodysupercontents;
	int passedictprog;
	float pitchsign = 1;
	qbool pointtrace;
	prvm_edict_t *traceowner, *touch;
	trace_t trace;
	// temporary storage because prvm_vec_t may differ from vec_t
	vec3_t touchmins, touchmaxs;
	// bounding box of entire move area
	vec3_t clipboxmins, clipboxmaxs;
	// size of the moving object
	vec3_t clipmins, clipmaxs;
	// size when clipping against monsters
	vec3_t clipmins2, clipmaxs2;
	// start and end origin of move
	vec3_t clipstart, clipend;
	// trace results
	trace_t cliptrace;
	// matrices to transform into/out of other entity's space
	matrix4x4_t matrix, imatrix;
	// model of other entity
	model_t *model;
	// list of entities to test for collisions
	int numtouchedicts;
	static prvm_edict_t *touchedicts[MAX_EDICTS];
	int clipgroup;
	if (VectorCompare(mins, maxs))
	{
		vec3_t shiftstart, shiftend;
		VectorAdd(start, mins, shiftstart);
		VectorAdd(end, mins, shiftend);
		if (VectorCompare(start, end))
			trace = SV_TracePoint(shiftstart, type, passedict, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask);
		else
			trace = SV_TraceLine(shiftstart, shiftend, type, passedict, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, extend);
		VectorSubtract(trace.endpos, mins, trace.endpos);
		return trace;
	}

	VectorCopy(start, clipstart);
	VectorCopy(end, clipend);
	VectorCopy(mins, clipmins);
	VectorCopy(maxs, clipmaxs);
	VectorCopy(mins, clipmins2);
	VectorCopy(maxs, clipmaxs2);
#if COLLISIONPARANOID >= 3
	Con_Printf("move(%f %f %f,%f %f %f)", clipstart[0], clipstart[1], clipstart[2], clipend[0], clipend[1], clipend[2]);
#endif

	// clip to world
	Collision_ClipToWorld(&cliptrace, sv.worldmodel, clipstart, clipmins, clipmaxs, clipend, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, extend);
	cliptrace.worldstartsolid = cliptrace.bmodelstartsolid = cliptrace.startsolid;
	if (cliptrace.startsolid || cliptrace.fraction < 1)
		cliptrace.ent = prog->edicts;
	if (type == MOVE_WORLDONLY)
		goto finished;

	if (type == MOVE_MISSILE)
	{
		// LadyHavoc: modified this, was = -15, now -= 15
		for (i = 0;i < 3;i++)
		{
			clipmins2[i] -= 15;
			clipmaxs2[i] += 15;
		}
	}

	// get adjusted box for bmodel collisions if the world is q1bsp or hlbsp
	if (sv.worldmodel && sv.worldmodel->brush.RoundUpToHullSize)
		sv.worldmodel->brush.RoundUpToHullSize(sv.worldmodel, clipmins, clipmaxs, hullmins, hullmaxs);
	else
	{
		VectorCopy(clipmins, hullmins);
		VectorCopy(clipmaxs, hullmaxs);
	}

	// create the bounding box of the entire move
	for (i = 0;i < 3;i++)
	{
		clipboxmins[i] = min(clipstart[i], cliptrace.endpos[i]) + min(hullmins[i], clipmins2[i]) - 1;
		clipboxmaxs[i] = max(clipstart[i], cliptrace.endpos[i]) + max(hullmaxs[i], clipmaxs2[i]) + 1;
	}

	// debug override to test against everything
	if (sv_debugmove.integer)
	{
		clipboxmins[0] = clipboxmins[1] = clipboxmins[2] = (vec_t)-999999999;
		clipboxmaxs[0] = clipboxmaxs[1] = clipboxmaxs[2] =  (vec_t)999999999;
	}

	// if the passedict is world, make it NULL (to avoid two checks each time)
	if (passedict == prog->edicts)
		passedict = NULL;
	// precalculate prog value for passedict for comparisons
	passedictprog = PRVM_EDICT_TO_PROG(passedict);
	// figure out whether this is a point trace for comparisons
	pointtrace = VectorCompare(clipmins, clipmaxs);
	// precalculate passedict's owner edict pointer for comparisons
	traceowner = passedict ? PRVM_PROG_TO_EDICT(PRVM_serveredictedict(passedict, owner)) : 0;

	clipgroup = passedict ? (int)PRVM_serveredictfloat(passedict, clipgroup) : 0;

	// clip to entities
	// because this uses World_EntitiestoBox, we know all entity boxes overlap
	// the clip region, so we can skip culling checks in the loop below
	numtouchedicts = SV_EntitiesInBox(clipboxmins, clipboxmaxs, MAX_EDICTS, touchedicts);
	if (numtouchedicts > MAX_EDICTS)
	{
		// this never happens
		Con_Printf("SV_EntitiesInBox returned %i edicts, max was %i\n", numtouchedicts, MAX_EDICTS);
		numtouchedicts = MAX_EDICTS;
	}
	for (i = 0;i < numtouchedicts;i++)
	{
		touch = touchedicts[i];

		if (PRVM_serveredictfloat(touch, solid) < SOLID_BBOX)
			continue;
		if (type == MOVE_NOMONSTERS && PRVM_serveredictfloat(touch, solid) != SOLID_BSP)
			continue;

		if (passedict)
		{
			// don't clip against self
			if (passedict == touch)
				continue;
			// don't clip owned entities against owner
			if (traceowner == touch)
				continue;
			// don't clip owner against owned entities
			if (passedictprog == PRVM_serveredictedict(touch, owner))
				continue;
			// don't clip against any entities in the same clipgroup (DP_RM_CLIPGROUP)
			if (clipgroup && clipgroup == (int)PRVM_serveredictfloat(touch, clipgroup))
				continue;
			// don't clip points against points (they can't collide)
			if (pointtrace && VectorCompare(PRVM_serveredictvector(touch, mins), PRVM_serveredictvector(touch, maxs)) && (type != MOVE_MISSILE || !((int)PRVM_serveredictfloat(touch, flags) & FL_MONSTER)))
				continue;
		}

		bodysupercontents = PRVM_serveredictfloat(touch, solid) == SOLID_CORPSE ? SUPERCONTENTS_CORPSE : SUPERCONTENTS_BODY;

		// might interact, so do an exact clip
		model = NULL;
		if ((int) PRVM_serveredictfloat(touch, solid) == SOLID_BSP || type == MOVE_HITMODEL)
		{
			model = SV_GetModelFromEdict(touch);
			pitchsign = SV_GetPitchSign(prog, touch);
		}
		if (model)
			Matrix4x4_CreateFromQuakeEntity(&matrix, PRVM_serveredictvector(touch, origin)[0], PRVM_serveredictvector(touch, origin)[1], PRVM_serveredictvector(touch, origin)[2], pitchsign * PRVM_serveredictvector(touch, angles)[0], PRVM_serveredictvector(touch, angles)[1], PRVM_serveredictvector(touch, angles)[2], 1);
		else
			Matrix4x4_CreateTranslate(&matrix, PRVM_serveredictvector(touch, origin)[0], PRVM_serveredictvector(touch, origin)[1], PRVM_serveredictvector(touch, origin)[2]);
		Matrix4x4_Invert_Simple(&imatrix, &matrix);
		VM_GenerateFrameGroupBlend(prog, touch->priv.server->framegroupblend, touch);
		VM_FrameBlendFromFrameGroupBlend(touch->priv.server->frameblend, touch->priv.server->framegroupblend, model, sv.time);
		VM_UpdateEdictSkeleton(prog, touch, model, touch->priv.server->frameblend);
		VectorCopy(PRVM_serveredictvector(touch, mins), touchmins);
		VectorCopy(PRVM_serveredictvector(touch, maxs), touchmaxs);
		if (type == MOVE_MISSILE && (int)PRVM_serveredictfloat(touch, flags) & FL_MONSTER)
			Collision_ClipToGenericEntity(&trace, model, touch->priv.server->frameblend, &touch->priv.server->skeleton, touchmins, touchmaxs, bodysupercontents, &matrix, &imatrix, clipstart, clipmins2, clipmaxs2, clipend, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, extend);
		else
			Collision_ClipToGenericEntity(&trace, model, touch->priv.server->frameblend, &touch->priv.server->skeleton, touchmins, touchmaxs, bodysupercontents, &matrix, &imatrix, clipstart, clipmins, clipmaxs, clipend, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, extend);

		Collision_CombineTraces(&cliptrace, &trace, (void *)touch, PRVM_serveredictfloat(touch, solid) == SOLID_BSP);
	}

finished:
	return cliptrace;
}

#if COLLISIONPARANOID >= 1
trace_t SV_TraceBox(const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int type, prvm_edict_t *passedict, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask, float extend)
{
	prvm_prog_t *prog = SVVM_prog;
	int endstuck;
	trace_t trace;
	vec3_t temp;
	trace = SV_TraceBox_(start, mins, maxs, end, type, passedict, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, extend);
	if (passedict)
	{
		VectorCopy(trace.endpos, temp);
		endstuck = SV_TraceBox_(temp, mins, maxs, temp, type, passedict, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, extend).startsolid;
#if COLLISIONPARANOID < 3
		if (trace.startsolid || endstuck)
#endif
			Con_Printf("%s{e%i:%f %f %f:%f %f %f:%f:%f %f %f%s%s}\n", (trace.startsolid || endstuck) ? "^3" : "", passedict ? (int)(passedict - prog->edicts) : -1, PRVM_serveredictvector(passedict, origin)[0], PRVM_serveredictvector(passedict, origin)[1], PRVM_serveredictvector(passedict, origin)[2], end[0] - PRVM_serveredictvector(passedict, origin)[0], end[1] - PRVM_serveredictvector(passedict, origin)[1], end[2] - PRVM_serveredictvector(passedict, origin)[2], trace.fraction, trace.endpos[0] - PRVM_serveredictvector(passedict, origin)[0], trace.endpos[1] - PRVM_serveredictvector(passedict, origin)[1], trace.endpos[2] - PRVM_serveredictvector(passedict, origin)[2], trace.startsolid ? " startstuck" : "", endstuck ? " endstuck" : "");
	}
	return trace;
}
#endif

int SV_PointSuperContents(const vec3_t point)
{
	prvm_prog_t *prog = SVVM_prog;
	int supercontents = 0;
	int i;
	prvm_edict_t *touch;
	vec3_t transformed;
	// matrices to transform into/out of other entity's space
	matrix4x4_t matrix, imatrix;
	// model of other entity
	model_t *model;
	int frame;
	// list of entities to test for collisions
	int numtouchedicts;
	static prvm_edict_t *touchedicts[MAX_EDICTS];

	// get world supercontents at this point
	if (sv.worldmodel && sv.worldmodel->PointSuperContents)
		supercontents = sv.worldmodel->PointSuperContents(sv.worldmodel, 0, point);

	// if sv_gameplayfix_swiminbmodels is off we're done
	if (!sv_gameplayfix_swiminbmodels.integer)
		return supercontents;

	// get list of entities at this point
	numtouchedicts = SV_EntitiesInBox(point, point, MAX_EDICTS, touchedicts);
	if (numtouchedicts > MAX_EDICTS)
	{
		// this never happens
		Con_Printf("SV_EntitiesInBox returned %i edicts, max was %i\n", numtouchedicts, MAX_EDICTS);
		numtouchedicts = MAX_EDICTS;
	}
	for (i = 0;i < numtouchedicts;i++)
	{
		touch = touchedicts[i];

		// we only care about SOLID_BSP for pointcontents
		if (PRVM_serveredictfloat(touch, solid) != SOLID_BSP)
			continue;

		// might interact, so do an exact clip
		model = SV_GetModelFromEdict(touch);
		if (!model || !model->PointSuperContents)
			continue;
		Matrix4x4_CreateFromQuakeEntity(&matrix, PRVM_serveredictvector(touch, origin)[0], PRVM_serveredictvector(touch, origin)[1], PRVM_serveredictvector(touch, origin)[2], PRVM_serveredictvector(touch, angles)[0], PRVM_serveredictvector(touch, angles)[1], PRVM_serveredictvector(touch, angles)[2], 1);
		Matrix4x4_Invert_Simple(&imatrix, &matrix);
		Matrix4x4_Transform(&imatrix, point, transformed);
		frame = (int)PRVM_serveredictfloat(touch, frame);
		supercontents |= model->PointSuperContents(model, bound(0, frame, (model->numframes - 1)), transformed);
	}

	return supercontents;
}

/*
===============================================================================

Linking entities into the world culling system

===============================================================================
*/

int SV_EntitiesInBox(const vec3_t mins, const vec3_t maxs, int maxedicts, prvm_edict_t **resultedicts)
{
	prvm_prog_t *prog = SVVM_prog;
	vec3_t paddedmins, paddedmaxs;
	if (maxedicts < 1 || resultedicts == NULL)
		return 0;
	// LadyHavoc: discovered this actually causes its own bugs (dm6 teleporters being too close to info_teleport_destination)
	//VectorSet(paddedmins, mins[0] - 10, mins[1] - 10, mins[2] - 1);
	//VectorSet(paddedmaxs, maxs[0] + 10, maxs[1] + 10, maxs[2] + 1);
	VectorCopy(mins, paddedmins);
	VectorCopy(maxs, paddedmaxs);
	if (sv_areadebug.integer)
	{
		int numresultedicts = 0;
		int edictindex;
		prvm_edict_t *ed;
		for (edictindex = 1;edictindex < prog->num_edicts;edictindex++)
		{
			ed = PRVM_EDICT_NUM(edictindex);
			if (!ed->free && BoxesOverlap(PRVM_serveredictvector(ed, absmin), PRVM_serveredictvector(ed, absmax), paddedmins, paddedmaxs))
			{
				resultedicts[numresultedicts++] = ed;
				if (numresultedicts == maxedicts)
					break;
			}
		}
		return numresultedicts;
	}
	else
		return World_EntitiesInBox(&sv.world, paddedmins, paddedmaxs, maxedicts, resultedicts);
}

void SV_LinkEdict_TouchAreaGrid_Call(prvm_edict_t *touch, prvm_edict_t *ent)
{
	prvm_prog_t *prog = SVVM_prog;
	PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(touch);
	PRVM_serverglobaledict(other) = PRVM_EDICT_TO_PROG(ent);
	PRVM_serverglobalfloat(time) = sv.time;
	PRVM_serverglobalfloat(trace_allsolid) = false;
	PRVM_serverglobalfloat(trace_startsolid) = false;
	PRVM_serverglobalfloat(trace_fraction) = 1;
	PRVM_serverglobalfloat(trace_inwater) = false;
	PRVM_serverglobalfloat(trace_inopen) = true;
	VectorCopy (PRVM_serveredictvector(touch, origin), PRVM_serverglobalvector(trace_endpos));
	VectorSet (PRVM_serverglobalvector(trace_plane_normal), 0, 0, 1);
	PRVM_serverglobalfloat(trace_plane_dist) = 0;
	PRVM_serverglobaledict(trace_ent) = PRVM_EDICT_TO_PROG(ent);
	PRVM_serverglobalfloat(trace_dpstartcontents) = 0;
	PRVM_serverglobalfloat(trace_dphitcontents) = 0;
	PRVM_serverglobalfloat(trace_dphitq3surfaceflags) = 0;
	PRVM_serverglobalstring(trace_dphittexturename) = 0;
	prog->ExecuteProgram(prog, PRVM_serveredictfunction(touch, touch), "QC function self.touch is missing");
}

void SV_LinkEdict_TouchAreaGrid(prvm_edict_t *ent)
{
	prvm_prog_t *prog = SVVM_prog;
	int i, numtouchedicts, old_self, old_other;
	prvm_edict_t *touch;
	static prvm_edict_t *touchedicts[MAX_EDICTS];

	if (ent == prog->edicts)
		return;		// don't add the world

	if (ent->free)
		return;

	if (PRVM_serveredictfloat(ent, solid) == SOLID_NOT)
		return;

	// build a list of edicts to touch, because the link loop can be corrupted
	// by IncreaseEdicts called during touch functions
	numtouchedicts = SV_EntitiesInBox(ent->priv.server->areamins, ent->priv.server->areamaxs, MAX_EDICTS, touchedicts);
	if (numtouchedicts > MAX_EDICTS)
	{
		// this never happens
		Con_Printf("SV_EntitiesInBox returned %i edicts, max was %i\n", numtouchedicts, MAX_EDICTS);
		numtouchedicts = MAX_EDICTS;
	}

	old_self = PRVM_serverglobaledict(self);
	old_other = PRVM_serverglobaledict(other);
	for (i = 0;i < numtouchedicts;i++)
	{
		touch = touchedicts[i];
		if (touch != ent && (int)PRVM_serveredictfloat(touch, solid) == SOLID_TRIGGER && PRVM_serveredictfunction(touch, touch))
		{
			SV_LinkEdict_TouchAreaGrid_Call(touch, ent);
		}
	}
	PRVM_serverglobaledict(self) = old_self;
	PRVM_serverglobaledict(other) = old_other;
}

static void RotateBBox(const vec3_t mins, const vec3_t maxs, const vec3_t angles, vec3_t rotatedmins, vec3_t rotatedmaxs)
{
	vec3_t v, u;
	matrix4x4_t m;
	Matrix4x4_CreateFromQuakeEntity(&m, 0, 0, 0, angles[PITCH], angles[YAW], angles[ROLL], 1.0);

	v[0] = mins[0]; v[1] = mins[1]; v[2] = mins[2]; Matrix4x4_Transform(&m, v, u);
		VectorCopy(u, rotatedmins); VectorCopy(u, rotatedmaxs);
	v[0] = maxs[0]; v[1] = mins[1]; v[2] = mins[2]; Matrix4x4_Transform(&m, v, u);
		if(rotatedmins[0] > u[0]) { rotatedmins[0] = u[0]; } if(rotatedmins[1] > u[1]) { rotatedmins[1] = u[1]; } if(rotatedmins[2] > u[2]) { rotatedmins[2] = u[2]; }
		if(rotatedmaxs[0] < u[0]) { rotatedmaxs[0] = u[0]; } if(rotatedmaxs[1] < u[1]) { rotatedmaxs[1] = u[1]; } if(rotatedmaxs[2] < u[2]) { rotatedmaxs[2] = u[2]; }
	v[0] = mins[0]; v[1] = maxs[1]; v[2] = mins[2]; Matrix4x4_Transform(&m, v, u);
		if(rotatedmins[0] > u[0]) { rotatedmins[0] = u[0]; } if(rotatedmins[1] > u[1]) { rotatedmins[1] = u[1]; } if(rotatedmins[2] > u[2]) { rotatedmins[2] = u[2]; }
		if(rotatedmaxs[0] < u[0]) { rotatedmaxs[0] = u[0]; } if(rotatedmaxs[1] < u[1]) { rotatedmaxs[1] = u[1]; } if(rotatedmaxs[2] < u[2]) { rotatedmaxs[2] = u[2]; }
	v[0] = maxs[0]; v[1] = maxs[1]; v[2] = mins[2]; Matrix4x4_Transform(&m, v, u);
		if(rotatedmins[0] > u[0]) { rotatedmins[0] = u[0]; } if(rotatedmins[1] > u[1]) { rotatedmins[1] = u[1]; } if(rotatedmins[2] > u[2]) { rotatedmins[2] = u[2]; }
		if(rotatedmaxs[0] < u[0]) { rotatedmaxs[0] = u[0]; } if(rotatedmaxs[1] < u[1]) { rotatedmaxs[1] = u[1]; } if(rotatedmaxs[2] < u[2]) { rotatedmaxs[2] = u[2]; }
	v[0] = mins[0]; v[1] = mins[1]; v[2] = maxs[2]; Matrix4x4_Transform(&m, v, u);
		if(rotatedmins[0] > u[0]) { rotatedmins[0] = u[0]; } if(rotatedmins[1] > u[1]) { rotatedmins[1] = u[1]; } if(rotatedmins[2] > u[2]) { rotatedmins[2] = u[2]; }
		if(rotatedmaxs[0] < u[0]) { rotatedmaxs[0] = u[0]; } if(rotatedmaxs[1] < u[1]) { rotatedmaxs[1] = u[1]; } if(rotatedmaxs[2] < u[2]) { rotatedmaxs[2] = u[2]; }
	v[0] = maxs[0]; v[1] = mins[1]; v[2] = maxs[2]; Matrix4x4_Transform(&m, v, u);
		if(rotatedmins[0] > u[0]) { rotatedmins[0] = u[0]; } if(rotatedmins[1] > u[1]) { rotatedmins[1] = u[1]; } if(rotatedmins[2] > u[2]) { rotatedmins[2] = u[2]; }
		if(rotatedmaxs[0] < u[0]) { rotatedmaxs[0] = u[0]; } if(rotatedmaxs[1] < u[1]) { rotatedmaxs[1] = u[1]; } if(rotatedmaxs[2] < u[2]) { rotatedmaxs[2] = u[2]; }
	v[0] = mins[0]; v[1] = maxs[1]; v[2] = maxs[2]; Matrix4x4_Transform(&m, v, u);
		if(rotatedmins[0] > u[0]) { rotatedmins[0] = u[0]; } if(rotatedmins[1] > u[1]) { rotatedmins[1] = u[1]; } if(rotatedmins[2] > u[2]) { rotatedmins[2] = u[2]; }
		if(rotatedmaxs[0] < u[0]) { rotatedmaxs[0] = u[0]; } if(rotatedmaxs[1] < u[1]) { rotatedmaxs[1] = u[1]; } if(rotatedmaxs[2] < u[2]) { rotatedmaxs[2] = u[2]; }
	v[0] = maxs[0]; v[1] = maxs[1]; v[2] = maxs[2]; Matrix4x4_Transform(&m, v, u);
		if(rotatedmins[0] > u[0]) { rotatedmins[0] = u[0]; } if(rotatedmins[1] > u[1]) { rotatedmins[1] = u[1]; } if(rotatedmins[2] > u[2]) { rotatedmins[2] = u[2]; }
		if(rotatedmaxs[0] < u[0]) { rotatedmaxs[0] = u[0]; } if(rotatedmaxs[1] < u[1]) { rotatedmaxs[1] = u[1]; } if(rotatedmaxs[2] < u[2]) { rotatedmaxs[2] = u[2]; }
}

/*
===============
SV_LinkEdict

===============
*/
void SV_LinkEdict (prvm_edict_t *ent)
{
	prvm_prog_t *prog = SVVM_prog;
	model_t *model;
	vec3_t mins, maxs, entmins, entmaxs, entangles;
	int modelindex;

	if (ent == prog->edicts)
		return;		// don't add the world

	if (ent->free)
		return;

	modelindex = (int)PRVM_serveredictfloat(ent, modelindex);
	if (modelindex < 0 || modelindex >= MAX_MODELS)
	{
		Con_Printf("edict %i: SOLID_BSP with invalid modelindex!\n", PRVM_NUM_FOR_EDICT(ent));
		modelindex = 0;
	}
	model = SV_GetModelByIndex(modelindex);

	VM_GenerateFrameGroupBlend(prog, ent->priv.server->framegroupblend, ent);
	VM_FrameBlendFromFrameGroupBlend(ent->priv.server->frameblend, ent->priv.server->framegroupblend, model, sv.time);
	VM_UpdateEdictSkeleton(prog, ent, model, ent->priv.server->frameblend);

// set the abs box

	if (PRVM_serveredictfloat(ent, movetype) == MOVETYPE_PHYSICS)
	{
		// TODO maybe should do this for rotating SOLID_BSP too? Would behave better with rotating doors
		// TODO special handling for spheres?
		VectorCopy(PRVM_serveredictvector(ent, mins), entmins);
		VectorCopy(PRVM_serveredictvector(ent, maxs), entmaxs);
		VectorCopy(PRVM_serveredictvector(ent, angles), entangles);
		RotateBBox(entmins, entmaxs, entangles, mins, maxs);
		VectorAdd(PRVM_serveredictvector(ent, origin), mins, mins);
		VectorAdd(PRVM_serveredictvector(ent, origin), maxs, maxs);
	}
	else if (PRVM_serveredictfloat(ent, solid) == SOLID_BSP)
	{
		if (model != NULL)
		{
			if (!model->TraceBox)
				Con_DPrintf("edict %i: SOLID_BSP with non-collidable model\n", PRVM_NUM_FOR_EDICT(ent));

			if (PRVM_serveredictvector(ent, angles)[0] || PRVM_serveredictvector(ent, angles)[2] || PRVM_serveredictvector(ent, avelocity)[0] || PRVM_serveredictvector(ent, avelocity)[2])
			{
				VectorAdd(PRVM_serveredictvector(ent, origin), model->rotatedmins, mins);
				VectorAdd(PRVM_serveredictvector(ent, origin), model->rotatedmaxs, maxs);
			}
			else if (PRVM_serveredictvector(ent, angles)[1] || PRVM_serveredictvector(ent, avelocity)[1])
			{
				VectorAdd(PRVM_serveredictvector(ent, origin), model->yawmins, mins);
				VectorAdd(PRVM_serveredictvector(ent, origin), model->yawmaxs, maxs);
			}
			else
			{
				VectorAdd(PRVM_serveredictvector(ent, origin), model->normalmins, mins);
				VectorAdd(PRVM_serveredictvector(ent, origin), model->normalmaxs, maxs);
			}
		}
		else
		{
			// SOLID_BSP with no model is valid, mainly because some QC setup code does so temporarily
			VectorAdd(PRVM_serveredictvector(ent, origin), PRVM_serveredictvector(ent, mins), mins);
			VectorAdd(PRVM_serveredictvector(ent, origin), PRVM_serveredictvector(ent, maxs), maxs);
		}
	}
	else
	{
		VectorAdd(PRVM_serveredictvector(ent, origin), PRVM_serveredictvector(ent, mins), mins);
		VectorAdd(PRVM_serveredictvector(ent, origin), PRVM_serveredictvector(ent, maxs), maxs);
	}

	if (sv_legacy_bbox_expand.integer)
	{
		if ((int)PRVM_serveredictfloat(ent, flags) & FL_ITEM)
		{
			// to make items easier to pick up and allow them to be grabbed off
			// of shelves, the abs sizes are expanded
			mins[0] -= 15;
			mins[1] -= 15;
			mins[2] -= 1;
			maxs[0] += 15;
			maxs[1] += 15;
			maxs[2] += 1;
		}
		else
		{
			// because movement is clipped an epsilon away from an actual edge,
			// we must fully check even when bounding boxes don't quite touch
			mins[0] -= 1;
			mins[1] -= 1;
			mins[2] -= 1;
			maxs[0] += 1;
			maxs[1] += 1;
			maxs[2] += 1;
		}
	}

	VectorCopy(mins, PRVM_serveredictvector(ent, absmin));
	VectorCopy(maxs, PRVM_serveredictvector(ent, absmax));

	World_LinkEdict(&sv.world, ent, mins, maxs, sv_areagrid_link_SOLID_NOT.integer);
}

/*
===============================================================================

Utility functions

===============================================================================
*/

/*
============
SV_TestEntityPosition

returns true if the entity is in solid currently
============
*/
static int SV_TestEntityPosition (prvm_edict_t *ent, vec3_t offset)
{
	prvm_prog_t *prog = SVVM_prog;
	int hitsupercontentsmask = SV_GenericHitSuperContentsMask(ent);
	int skipsupercontentsmask = 0;
	int skipmaterialflagsmask = 0;
	vec3_t org, entorigin, entmins, entmaxs;
	trace_t trace;
	VectorAdd(PRVM_serveredictvector(ent, origin), offset, org);
	VectorCopy(PRVM_serveredictvector(ent, origin), entorigin);
	VectorCopy(PRVM_serveredictvector(ent, mins), entmins);
	VectorCopy(PRVM_serveredictvector(ent, maxs), entmaxs);
	trace = SV_TraceBox(org, entmins, entmaxs, entorigin, ((PRVM_serveredictfloat(ent, movetype) == MOVETYPE_FLY_WORLDONLY) ? MOVE_WORLDONLY : MOVE_NOMONSTERS), ent, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, collision_extendmovelength.value);
	if (trace.startsupercontents & hitsupercontentsmask)
		return true;
	else
	{
		if (sv.worldmodel->brushq1.numclipnodes && !VectorCompare(PRVM_serveredictvector(ent, mins), PRVM_serveredictvector(ent, maxs)))
		{
			// q1bsp/hlbsp use hulls and if the entity does not exactly match
			// a hull size it is incorrectly tested, so this code tries to
			// 'fix' it slightly...
			// FIXME: this breaks entities larger than the hull size
			int i;
			vec3_t v, m1, m2, s;
			VectorAdd(org, entmins, m1);
			VectorAdd(org, entmaxs, m2);
			VectorSubtract(m2, m1, s);
#define EPSILON (1.0f / 32.0f)
			if (s[0] >= EPSILON*2) {m1[0] += EPSILON;m2[0] -= EPSILON;}
			if (s[1] >= EPSILON*2) {m1[1] += EPSILON;m2[1] -= EPSILON;}
			if (s[2] >= EPSILON*2) {m1[2] += EPSILON;m2[2] -= EPSILON;}
			for (i = 0;i < 8;i++)
			{
				v[0] = (i & 1) ? m2[0] : m1[0];
				v[1] = (i & 2) ? m2[1] : m1[1];
				v[2] = (i & 4) ? m2[2] : m1[2];
				if (SV_PointSuperContents(v) & hitsupercontentsmask)
					return true;
			}
		}
	}
	// if the trace found a better position for the entity, move it there
	if (VectorDistance2(trace.endpos, PRVM_serveredictvector(ent, origin)) >= 0.0001)
	{
#if 0
		// please switch back to this code when trace.endpos sometimes being in solid bug is fixed
		VectorCopy(trace.endpos, PRVM_serveredictvector(ent, origin));
#else
		// verify if the endpos is REALLY outside solid
		VectorCopy(trace.endpos, org);
		trace = SV_TraceBox(org, entmins, entmaxs, org, MOVE_NOMONSTERS, ent, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, collision_extendmovelength.value);
		if(trace.startsolid)
			Con_Printf("SV_TestEntityPosition: trace.endpos detected to be in solid. NOT using it.\n");
		else
			VectorCopy(org, PRVM_serveredictvector(ent, origin));
#endif
	}
	return false;
}

// DRESK - Support for Entity Contents Transition Event
/*
================
SV_CheckContentsTransition

returns true if entity had a valid contentstransition function call
================
*/
static int SV_CheckContentsTransition(prvm_edict_t *ent, const int nContents)
{
	prvm_prog_t *prog = SVVM_prog;
	int bValidFunctionCall;

	// Default Valid Function Call to False
	bValidFunctionCall = false;

	if(PRVM_serveredictfloat(ent, watertype) != nContents)
	{ // Changed Contents
		// Acquire Contents Transition Function from QC
		if(PRVM_serveredictfunction(ent, contentstransition))
		{ // Valid Function; Execute
			// Assign Valid Function
			bValidFunctionCall = true;
			// Prepare Parameters (Original Contents, New Contents)
			// Original Contents
			PRVM_G_FLOAT(OFS_PARM0) = PRVM_serveredictfloat(ent, watertype);
			// New Contents
			PRVM_G_FLOAT(OFS_PARM1) = nContents;
			// Assign Self
			PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(ent);
			// Set Time
			PRVM_serverglobalfloat(time) = sv.time;
			// Execute VM Function
			prog->ExecuteProgram(prog, PRVM_serveredictfunction(ent, contentstransition), "contentstransition: NULL function");
		}
	}

	// Return if Function Call was Valid
	return bValidFunctionCall;
}


/*
================
SV_CheckVelocity
================
*/
void SV_CheckVelocity (prvm_edict_t *ent)
{
	prvm_prog_t *prog = SVVM_prog;
	int i;
	float wishspeed;

//
// bound velocity
//
	for (i=0 ; i<3 ; i++)
	{
		if (isnan(PRVM_serveredictvector(ent, velocity)[i]))
		{
			Con_Printf("Got a NaN velocity on entity #%i (%s)\n", PRVM_NUM_FOR_EDICT(ent), PRVM_GetString(prog, PRVM_serveredictstring(ent, classname)));
			PRVM_serveredictvector(ent, velocity)[i] = 0;
		}
		if (isnan(PRVM_serveredictvector(ent, origin)[i]))
		{
			Con_Printf("Got a NaN origin on entity #%i (%s)\n", PRVM_NUM_FOR_EDICT(ent), PRVM_GetString(prog, PRVM_serveredictstring(ent, classname)));
			PRVM_serveredictvector(ent, origin)[i] = 0;
		}
	}

	// LadyHavoc: a hack to ensure that the (rather silly) id1 quakec
	// player_run/player_stand1 does not horribly malfunction if the
	// velocity becomes a denormalized float
	if (VectorLength2(PRVM_serveredictvector(ent, velocity)) < 0.0000001)
		VectorClear(PRVM_serveredictvector(ent, velocity));

	// LadyHavoc: max velocity fix, inspired by Maddes's source fixes, but this is faster
	wishspeed = DotProduct(PRVM_serveredictvector(ent, velocity), PRVM_serveredictvector(ent, velocity));
	if (wishspeed > sv_maxvelocity.value * sv_maxvelocity.value)
	{
		wishspeed = sv_maxvelocity.value / sqrt(wishspeed);
		PRVM_serveredictvector(ent, velocity)[0] *= wishspeed;
		PRVM_serveredictvector(ent, velocity)[1] *= wishspeed;
		PRVM_serveredictvector(ent, velocity)[2] *= wishspeed;
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
static qbool SV_RunThink (prvm_edict_t *ent)
{
	prvm_prog_t *prog = SVVM_prog;
	int iterations;

	// don't let things stay in the past.
	// it is possible to start that way by a trigger with a local time.
	if (PRVM_serveredictfloat(ent, nextthink) <= 0 || PRVM_serveredictfloat(ent, nextthink) > sv.time + sv.frametime)
		return true;

	for (iterations = 0;iterations < 128  && !ent->free;iterations++)
	{
		PRVM_serverglobalfloat(time) = max(sv.time, PRVM_serveredictfloat(ent, nextthink));
		PRVM_serveredictfloat(ent, nextthink) = 0;
		PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(ent);
		PRVM_serverglobaledict(other) = PRVM_EDICT_TO_PROG(prog->edicts);
		prog->ExecuteProgram(prog, PRVM_serveredictfunction(ent, think), "QC function self.think is missing");
		// mods often set nextthink to time to cause a think every frame,
		// we don't want to loop in that case, so exit if the new nextthink is
		// <= the time the qc was told, also exit if it is past the end of the
		// frame
		if (PRVM_serveredictfloat(ent, nextthink) <= PRVM_serverglobalfloat(time) || PRVM_serveredictfloat(ent, nextthink) > sv.time + sv.frametime || !sv_gameplayfix_multiplethinksperframe.integer)
			break;
	}
	return !ent->free;
}

/*
==================
SV_Impact

Two entities have touched, so run their touch functions
==================
*/
static void SV_Impact (prvm_edict_t *e1, trace_t *trace)
{
	prvm_prog_t *prog = SVVM_prog;
	int restorevm_tempstringsbuf_cursize;
	int old_self, old_other;
	prvm_edict_t *e2 = (prvm_edict_t *)trace->ent;

	old_self = PRVM_serverglobaledict(self);
	old_other = PRVM_serverglobaledict(other);
	restorevm_tempstringsbuf_cursize = prog->tempstringsbuf.cursize;

	VM_SetTraceGlobals(prog, trace);

	if (!e1->free && !e2->free && PRVM_serveredictfunction(e1, touch) && PRVM_serveredictfloat(e1, solid) != SOLID_NOT)
	{
		PRVM_serverglobalfloat(time) = sv.time;
		PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(e1);
		PRVM_serverglobaledict(other) = PRVM_EDICT_TO_PROG(e2);
		prog->ExecuteProgram(prog, PRVM_serveredictfunction(e1, touch), "QC function self.touch is missing");
	}

	if (!e1->free && !e2->free && PRVM_serveredictfunction(e2, touch) && PRVM_serveredictfloat(e2, solid) != SOLID_NOT)
	{
		PRVM_serverglobalfloat(time) = sv.time;
		PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(e2);
		PRVM_serverglobaledict(other) = PRVM_EDICT_TO_PROG(e1);
		VectorCopy(PRVM_serveredictvector(e2, origin), PRVM_serverglobalvector(trace_endpos));
		VectorNegate(trace->plane.normal, PRVM_serverglobalvector(trace_plane_normal));
		PRVM_serverglobalfloat(trace_plane_dist) = -trace->plane.dist;
		PRVM_serverglobaledict(trace_ent) = PRVM_EDICT_TO_PROG(e1);
		PRVM_serverglobalfloat(trace_dpstartcontents) = 0;
		PRVM_serverglobalfloat(trace_dphitcontents) = 0;
		PRVM_serverglobalfloat(trace_dphitq3surfaceflags) = 0;
		PRVM_serverglobalstring(trace_dphittexturename) = 0;
		prog->ExecuteProgram(prog, PRVM_serveredictfunction(e2, touch), "QC function self.touch is missing");
	}

	PRVM_serverglobaledict(self) = old_self;
	PRVM_serverglobaledict(other) = old_other;
	prog->tempstringsbuf.cursize = restorevm_tempstringsbuf_cursize;
}


/*
==================
ClipVelocity

Slide off of the impacting object
returns the blocked flags (1 = floor, 2 = step / wall)
==================
*/
#define STOP_EPSILON 0.1
static void ClipVelocity (prvm_vec3_t in, vec3_t normal, prvm_vec3_t out, prvm_vec_t overbounce)
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
8 = teleported by touch method
If stepnormal is not NULL, the plane normal of any vertical wall hit will be stored
============
*/
static float SV_Gravity (prvm_edict_t *ent);
static qbool SV_PushEntity (trace_t *trace, prvm_edict_t *ent, vec3_t push, qbool dolink, qbool checkstuck);
#define MAX_CLIP_PLANES 5
static int SV_FlyMove (prvm_edict_t *ent, float time, qbool applygravity, float *stepnormal, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask, float stepheight)
{
	prvm_prog_t *prog = SVVM_prog;
	unsigned int i, j, numplanes, blocked, bumpcount;
	float d, time_left, gravity;
	vec3_t dir, push, planes[MAX_CLIP_PLANES];
	prvm_vec3_t primal_velocity, original_velocity, new_velocity, restore_velocity;
#if 0
	vec3_t end;
#endif
	trace_t trace;
	if (time <= 0)
		return 0;
	gravity = 0;

	VectorCopy(PRVM_serveredictvector(ent, velocity), restore_velocity);

	if(applygravity)
	{
		gravity = SV_Gravity(ent);

		if(!sv_gameplayfix_nogravityonground.integer || !((int)PRVM_serveredictfloat(ent, flags) & FL_ONGROUND))
		{
			if (sv_gameplayfix_gravityunaffectedbyticrate.integer)
				PRVM_serveredictvector(ent, velocity)[2] -= gravity * 0.5f;
			else
				PRVM_serveredictvector(ent, velocity)[2] -= gravity;
		}
	}

	blocked = 0;
	VectorCopy(PRVM_serveredictvector(ent, velocity), original_velocity);
	VectorCopy(PRVM_serveredictvector(ent, velocity), primal_velocity);
	numplanes = 0;
	time_left = time;
	for (bumpcount = 0;bumpcount < MAX_CLIP_PLANES;bumpcount++)
	{
		if (!PRVM_serveredictvector(ent, velocity)[0] && !PRVM_serveredictvector(ent, velocity)[1] && !PRVM_serveredictvector(ent, velocity)[2])
			break;

		VectorScale(PRVM_serveredictvector(ent, velocity), time_left, push);
		if(!SV_PushEntity(&trace, ent, push, false, true))
		{
			// we got teleported by a touch function
			// let's abort the move
			blocked |= 8;
			break;
		}

		// this code is used by MOVETYPE_WALK and MOVETYPE_STEP and SV_UnstickEntity
		// abort move if we're stuck in the world (and didn't make it out)
		if (trace.worldstartsolid && trace.allsolid && trace.startdepth < 0)
		{
			VectorCopy(restore_velocity, PRVM_serveredictvector(ent, velocity));
			return 3;
		}

		if (trace.fraction == 1)
			break;

		time_left *= 1 - trace.fraction;

		if (trace.plane.normal[2])
		{
			if (trace.plane.normal[2] > 0.7)
			{
				// floor
				blocked |= 1;

				if (!trace.ent)
				{
					Con_Printf ("SV_FlyMove: !trace.ent");
					trace.ent = prog->edicts;
				}

				PRVM_serveredictfloat(ent, flags) = (int)PRVM_serveredictfloat(ent, flags) | FL_ONGROUND;
				PRVM_serveredictedict(ent, groundentity) = PRVM_EDICT_TO_PROG(trace.ent);
			}
		}
		else if (stepheight)
		{
			// step - handle it immediately
			vec3_t org;
			vec3_t steppush;
			trace_t steptrace;
			trace_t steptrace2;
			trace_t steptrace3;
			//Con_Printf("step %f %f %f : ", PRVM_serveredictvector(ent, origin)[0], PRVM_serveredictvector(ent, origin)[1], PRVM_serveredictvector(ent, origin)[2]);
			VectorSet(steppush, 0, 0, stepheight);
			VectorScale(PRVM_serveredictvector(ent, velocity), time_left, push);
			VectorCopy(PRVM_serveredictvector(ent, origin), org);
			if(!SV_PushEntity(&steptrace, ent, steppush, false, true))
			{
				blocked |= 8;
				break;
			}
			//Con_Printf("%f %f %f : ", PRVM_serveredictvector(ent, origin)[0], PRVM_serveredictvector(ent, origin)[1], PRVM_serveredictvector(ent, origin)[2]);
			if(!SV_PushEntity(&steptrace2, ent, push, false, true))
			{
				blocked |= 8;
				break;
			}
			//Con_Printf("%f %f %f : ", PRVM_serveredictvector(ent, origin)[0], PRVM_serveredictvector(ent, origin)[1], PRVM_serveredictvector(ent, origin)[2]);
			VectorSet(steppush, 0, 0, org[2] - PRVM_serveredictvector(ent, origin)[2]);
			if(!SV_PushEntity(&steptrace3, ent, steppush, false, true))
			{
				blocked |= 8;
				break;
			}
			//Con_Printf("%f %f %f : ", PRVM_serveredictvector(ent, origin)[0], PRVM_serveredictvector(ent, origin)[1], PRVM_serveredictvector(ent, origin)[2]);
			// accept the new position if it made some progress...
			// previously this checked if absolute distance >= 0.03125 which made stepping up unreliable
			if (PRVM_serveredictvector(ent, origin)[0] - org[0] || PRVM_serveredictvector(ent, origin)[1] - org[1])
			{
				//Con_Printf("accepted (delta %f %f %f)\n", PRVM_serveredictvector(ent, origin)[0] - org[0], PRVM_serveredictvector(ent, origin)[1] - org[1], PRVM_serveredictvector(ent, origin)[2] - org[2]);
				trace = steptrace2;
				VectorCopy(PRVM_serveredictvector(ent, origin), trace.endpos);
				time_left *= 1 - trace.fraction;
				numplanes = 0;
				continue;
			}
			else
			{
				//Con_Printf("REJECTED (delta %f %f %f)\n", PRVM_serveredictvector(ent, origin)[0] - org[0], PRVM_serveredictvector(ent, origin)[1] - org[1], PRVM_serveredictvector(ent, origin)[2] - org[2]);
				VectorCopy(org, PRVM_serveredictvector(ent, origin));
			}
		}
		else
		{
			// step - return it to caller
			blocked |= 2;
			// save the trace for player extrafriction
			if (stepnormal)
				VectorCopy(trace.plane.normal, stepnormal);
		}

		// Unlike some other movetypes Quake's SV_FlyMove calls SV_Impact only after setting ONGROUND which id1 fiends rely on.
		// If we stepped up (sv_gameplayfix_stepmultipletimes) this will impact the steptrace2 plane instead of the original.
		if (PRVM_serveredictfloat(ent, solid) >= SOLID_TRIGGER && trace.ent)
			SV_Impact(ent, &trace);
		if (ent->free)
			return blocked; // removed by the impact function

		if (trace.fraction >= 0.001)
		{
			// actually covered some distance
			VectorCopy(PRVM_serveredictvector(ent, velocity), original_velocity);
			numplanes = 0;
		}

		// clipped to another plane
		if (numplanes >= MAX_CLIP_PLANES)
		{
			// this shouldn't really happen
			VectorClear(PRVM_serveredictvector(ent, velocity));
			blocked = 3;
			break;
		}

		/*
		for (i = 0;i < numplanes;i++)
			if (DotProduct(trace.plane.normal, planes[i]) > 0.99)
				break;
		if (i < numplanes)
		{
			VectorAdd(PRVM_serveredictvector(ent, velocity), trace.plane.normal, PRVM_serveredictvector(ent, velocity));
			continue;
		}
		*/

		VectorCopy(trace.plane.normal, planes[numplanes]);
		numplanes++;

		// modify original_velocity so it parallels all of the clip planes
		for (i = 0;i < numplanes;i++)
		{
			ClipVelocity(original_velocity, planes[i], new_velocity, 1);
			for (j = 0;j < numplanes;j++)
			{
				if (j != i)
				{
					// not ok
					if (DotProduct(new_velocity, planes[j]) < 0)
						break;
				}
			}
			if (j == numplanes)
				break;
		}

		if (i != numplanes)
		{
			// go along this plane
			VectorCopy(new_velocity, PRVM_serveredictvector(ent, velocity));
		}
		else
		{
			// go along the crease
			if (numplanes != 2)
			{
				VectorClear(PRVM_serveredictvector(ent, velocity));
				blocked = 7;
				break;
			}
			CrossProduct(planes[0], planes[1], dir);
			// LadyHavoc: thanks to taniwha of QuakeForge for pointing out this fix for slowed falling in corners
			VectorNormalize(dir);
			d = DotProduct(dir, PRVM_serveredictvector(ent, velocity));
			VectorScale(dir, d, PRVM_serveredictvector(ent, velocity));
		}

		// if current velocity is against the original velocity,
		// stop dead to avoid tiny occilations in sloping corners
		if (DotProduct(PRVM_serveredictvector(ent, velocity), primal_velocity) <= 0)
		{
			VectorClear(PRVM_serveredictvector(ent, velocity));
			break;
		}
	}

	//Con_Printf("entity %i final: blocked %i velocity %f %f %f\n", ent - prog->edicts, blocked, PRVM_serveredictvector(ent, velocity)[0], PRVM_serveredictvector(ent, velocity)[1], PRVM_serveredictvector(ent, velocity)[2]);

	/*
	if ((blocked & 1) == 0 && bumpcount > 1)
	{
		// LadyHavoc: fix the 'fall to your death in a wedge corner' glitch
		// flag ONGROUND if there's ground under it
		trace = SV_TraceBox(PRVM_serveredictvector(ent, origin), PRVM_serveredictvector(ent, mins), PRVM_serveredictvector(ent, maxs), end, MOVE_NORMAL, ent, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask);
	}
	*/

	// LadyHavoc: this came from QW and allows you to get out of water more easily
	if (sv_gameplayfix_easierwaterjump.integer && ((int)PRVM_serveredictfloat(ent, flags) & FL_WATERJUMP) && !(blocked & 8))
		VectorCopy(primal_velocity, PRVM_serveredictvector(ent, velocity));

	if(applygravity)
	{
		if(!sv_gameplayfix_nogravityonground.integer || !((int)PRVM_serveredictfloat(ent, flags) & FL_ONGROUND))
		{
			if (sv_gameplayfix_gravityunaffectedbyticrate.integer)
				PRVM_serveredictvector(ent, velocity)[2] -= gravity * 0.5f;
		}
	}

	return blocked;
}

/*
============
SV_Gravity

============
*/
static float SV_Gravity (prvm_edict_t *ent)
{
	prvm_prog_t *prog = SVVM_prog;
	float ent_gravity;

	ent_gravity = PRVM_serveredictfloat(ent, gravity);
	if (!ent_gravity)
		ent_gravity = 1.0f;
	return ent_gravity * sv_gravity.value * sv.frametime;
}


/*
===============================================================================

PUSHMOVE

===============================================================================
*/

static qbool SV_NudgeOutOfSolid_PivotIsKnownGood(prvm_edict_t *ent, vec3_t pivot)
{
	prvm_prog_t *prog = SVVM_prog;
	int bump;
	trace_t stucktrace;
	vec3_t stuckorigin;
	vec3_t stuckmins, stuckmaxs;
	vec3_t goodmins, goodmaxs;
	vec3_t testorigin;
	vec_t nudge;
	vec3_t move;
	VectorCopy(PRVM_serveredictvector(ent, origin), stuckorigin);
	VectorCopy(PRVM_serveredictvector(ent, mins), stuckmins);
	VectorCopy(PRVM_serveredictvector(ent, maxs), stuckmaxs);
	VectorCopy(pivot, goodmins);
	VectorCopy(pivot, goodmaxs);
	for (bump = 0;bump < 6;bump++)
	{
		int coord = 2-(bump >> 1);
		//int coord = (bump >> 1);
		int dir = (bump & 1);
		int subbump;

		for(subbump = 0; ; ++subbump)
		{
			VectorCopy(stuckorigin, testorigin);
			if(dir)
			{
				// pushing maxs
				testorigin[coord] += stuckmaxs[coord] - goodmaxs[coord];
			}
			else
			{
				// pushing mins
				testorigin[coord] += stuckmins[coord] - goodmins[coord];
			}

			stucktrace = SV_TraceBox(stuckorigin, goodmins, goodmaxs, testorigin, MOVE_NOMONSTERS, ent, SV_GenericHitSuperContentsMask(ent), 0, 0, collision_extendmovelength.value);
			if (stucktrace.bmodelstartsolid)
			{
				// BAD BAD, can't fix that
				return false;
			}

			if (stucktrace.fraction >= 1)
				break; // it WORKS!

			if(subbump >= 10)
			{
				// BAD BAD, can't fix that
				return false;
			}

			// we hit something... let's move out of it
			VectorSubtract(stucktrace.endpos, testorigin, move);
			nudge = DotProduct(stucktrace.plane.normal, move) + 0.03125f; // FIXME cvar this constant
			VectorMA(stuckorigin, nudge, stucktrace.plane.normal, stuckorigin);
		}
		/*
		if(subbump > 0)
			Con_Printf("subbump: %d\n", subbump);
		*/

		if(dir)
		{
			// pushing maxs
			goodmaxs[coord] = stuckmaxs[coord];
		}
		else
		{
			// pushing mins
			goodmins[coord] = stuckmins[coord];
		}
	}

	// WE WIN
	VectorCopy(stuckorigin, PRVM_serveredictvector(ent, origin));

	return true;
}

/*
============
SV_PushEntity

Does not change the entities velocity at all
The trace struct is filled with the trace that has been done.
Returns true if the push did not result in the entity being teleported by QC code.
============
*/
static qbool SV_PushEntity (trace_t *trace, prvm_edict_t *ent, vec3_t push, qbool dolink, qbool checkstuck)
{
	prvm_prog_t *prog = SVVM_prog;
	int solid;
	int movetype;
	int type;
	vec3_t mins, maxs;
	vec3_t start;
	vec3_t end;

	solid = (int)PRVM_serveredictfloat(ent, solid);
	movetype = (int)PRVM_serveredictfloat(ent, movetype);
	VectorCopy(PRVM_serveredictvector(ent, mins), mins);
	VectorCopy(PRVM_serveredictvector(ent, maxs), maxs);

	VectorCopy(PRVM_serveredictvector(ent, origin), start);
	VectorAdd(start, push, end);

	if (movetype == MOVETYPE_FLYMISSILE)
		type = MOVE_MISSILE;
	else if (movetype == MOVETYPE_FLY_WORLDONLY)
		type = MOVE_WORLDONLY;
	else if (solid == SOLID_TRIGGER || solid == SOLID_NOT)
		type = MOVE_NOMONSTERS; // only clip against bmodels
	else
		type = MOVE_NORMAL;

	*trace = SV_TraceBox(start, mins, maxs, end, type, ent, SV_GenericHitSuperContentsMask(ent), 0, 0, collision_extendmovelength.value);
	// abort move if we're stuck in the world (and didn't make it out)
	if (trace->worldstartsolid && trace->allsolid && trace->startdepth < 0 && checkstuck)
	{
		// checking startdepth eliminates many false positives on Q1BSP with mod_q1bsp_polygoncollisions 0
		// but it's still not guaranteed that we're stuck in a bmodel at this point
		if (sv_gameplayfix_nudgeoutofsolid.integer && sv_gameplayfix_nudgeoutofsolid_separation.value >= 0)
		{
			switch (PHYS_NudgeOutOfSolid(prog, ent))
			{
				case 0:
					Con_Printf(CON_WARN "NudgeOutOfSolid couldn't fix stuck entity %i (classname \"%s\").\n", (int)PRVM_EDICT_TO_PROG(ent), PRVM_GetString(prog, PRVM_serveredictstring(ent, classname)));
					return true; // definitely stuck in a bmodel
				case 1:
					Con_DPrintf("NudgeOutOfSolid fixed stuck entity %i (classname \"%s\") with offset %f %f %f.\n", (int)PRVM_EDICT_TO_PROG(ent), PRVM_GetString(prog, PRVM_serveredictstring(ent, classname)), PRVM_serveredictvector(ent, origin)[0] - start[0], PRVM_serveredictvector(ent, origin)[1] - start[1], PRVM_serveredictvector(ent, origin)[2] - start[2]);
					VectorCopy(PRVM_serveredictvector(ent, origin), start);
					VectorAdd(start, push, end);
					*trace = SV_TraceBox(start, mins, maxs, end, type, ent, SV_GenericHitSuperContentsMask(ent), 0, 0, collision_extendmovelength.value);

				// definitely not stuck in a bmodel, move may proceed
			}
		}
		else if (sv_gameplayfix_unstickentities.integer && SV_UnstickEntity(ent))
		{
			// bones_was_here: pretty sure we can deprecate sv_gameplayfix_unstickentities, sv_gameplayfix_nudgeoutofsolid is much nicer
			VectorCopy(PRVM_serveredictvector(ent, origin), start);
			VectorAdd(start, push, end);
			*trace = SV_TraceBox(start, mins, maxs, end, type, ent, SV_GenericHitSuperContentsMask(ent), 0, 0, collision_extendmovelength.value);
		}
		else
			return true; // assuming stuck, bones_was_here TODO: always use PHYS_NudgeOutOfSolid (remove sv_gameplayfix_nudgeoutofsolid)?
	}

	VectorCopy(trace->endpos, PRVM_serveredictvector(ent, origin));

	ent->priv.required->mark = PRVM_EDICT_MARK_WAIT_FOR_SETORIGIN; // -2: setorigin running

	SV_LinkEdict(ent);

#if 0
	if(!trace->startsolid)
	if(SV_TraceBox(PRVM_serveredictvector(ent, origin), PRVM_serveredictvector(ent, mins), PRVM_serveredictvector(ent, maxs), PRVM_serveredictvector(ent, origin), type, ent, SV_GenericHitSuperContentsMask(ent), 0).startsolid)
	{
		Con_Printf("something eeeeevil happened\n");
	}
#endif

	if (dolink)
	{
		SV_LinkEdict_TouchAreaGrid(ent);

		if((PRVM_serveredictfloat(ent, solid) >= SOLID_TRIGGER && trace->ent && (!((int)PRVM_serveredictfloat(ent, flags) & FL_ONGROUND) || PRVM_serveredictedict(ent, groundentity) != PRVM_EDICT_TO_PROG(trace->ent))))
			SV_Impact (ent, trace);
	}

	if(ent->priv.required->mark == PRVM_EDICT_MARK_SETORIGIN_CAUGHT)
	{
		ent->priv.required->mark = 0;
		return false;
	}
	else if(ent->priv.required->mark == PRVM_EDICT_MARK_WAIT_FOR_SETORIGIN)
	{
		ent->priv.required->mark = 0;
		return true;
	}
	else
	{
		Con_Printf("The edict mark had been overwritten! Please debug this.\n");
		return true;
	}
}


/*
============
SV_PushMove

============
*/
static void SV_PushMove (prvm_edict_t *pusher, float movetime)
{
	prvm_prog_t *prog = SVVM_prog;
	int i, e, index;
	int pusherowner, pusherprog;
	int checkcontents;
	qbool rotated;
	float savesolid, movetime2, pushltime;
	vec3_t mins, maxs, move, move1, moveangle, pushorig, pushang, a, forward, left, up, org, pushermins, pushermaxs, checkorigin, checkmins, checkmaxs;
	int num_moved;
	int numcheckentities;
	static prvm_edict_t *checkentities[MAX_EDICTS];
	model_t *pushermodel;
	trace_t trace, trace2;
	matrix4x4_t pusherfinalmatrix, pusherfinalimatrix;
	static unsigned short moved_edicts[MAX_EDICTS];
	vec3_t pivot;

	if (!PRVM_serveredictvector(pusher, velocity)[0] && !PRVM_serveredictvector(pusher, velocity)[1] && !PRVM_serveredictvector(pusher, velocity)[2] && !PRVM_serveredictvector(pusher, avelocity)[0] && !PRVM_serveredictvector(pusher, avelocity)[1] && !PRVM_serveredictvector(pusher, avelocity)[2])
	{
		PRVM_serveredictfloat(pusher, ltime) += movetime;
		return;
	}

	switch ((int) PRVM_serveredictfloat(pusher, solid))
	{
	// LadyHavoc: valid pusher types
	case SOLID_BSP:
	case SOLID_BBOX:
	case SOLID_SLIDEBOX:
	case SOLID_CORPSE: // LadyHavoc: this would be weird...
		break;
	// LadyHavoc: no collisions
	case SOLID_NOT:
	case SOLID_TRIGGER:
		VectorMA (PRVM_serveredictvector(pusher, origin), movetime, PRVM_serveredictvector(pusher, velocity), PRVM_serveredictvector(pusher, origin));
		VectorMA (PRVM_serveredictvector(pusher, angles), movetime, PRVM_serveredictvector(pusher, avelocity), PRVM_serveredictvector(pusher, angles));
		PRVM_serveredictvector(pusher, angles)[0] -= 360.0 * floor(PRVM_serveredictvector(pusher, angles)[0] * (1.0 / 360.0));
		PRVM_serveredictvector(pusher, angles)[1] -= 360.0 * floor(PRVM_serveredictvector(pusher, angles)[1] * (1.0 / 360.0));
		PRVM_serveredictvector(pusher, angles)[2] -= 360.0 * floor(PRVM_serveredictvector(pusher, angles)[2] * (1.0 / 360.0));
		PRVM_serveredictfloat(pusher, ltime) += movetime;
		SV_LinkEdict(pusher);
		return;
	default:
		Con_Printf("SV_PushMove: entity #%i, unrecognized solid type %f\n", PRVM_NUM_FOR_EDICT(pusher), PRVM_serveredictfloat(pusher, solid));
		return;
	}
	index = (int) PRVM_serveredictfloat(pusher, modelindex);
	if (index < 1 || index >= MAX_MODELS)
	{
		Con_Printf("SV_PushMove: entity #%i has an invalid modelindex %f\n", PRVM_NUM_FOR_EDICT(pusher), PRVM_serveredictfloat(pusher, modelindex));
		return;
	}
	pushermodel = SV_GetModelByIndex(index);
	pusherowner = PRVM_serveredictedict(pusher, owner);
	pusherprog = PRVM_EDICT_TO_PROG(pusher);

	rotated = VectorLength2(PRVM_serveredictvector(pusher, angles)) + VectorLength2(PRVM_serveredictvector(pusher, avelocity)) > 0;

	movetime2 = movetime;
	VectorScale(PRVM_serveredictvector(pusher, velocity), movetime2, move1);
	VectorScale(PRVM_serveredictvector(pusher, avelocity), movetime2, moveangle);
	if (moveangle[0] || moveangle[2])
	{
		for (i = 0;i < 3;i++)
		{
			if (move1[i] > 0)
			{
				mins[i] = pushermodel->rotatedmins[i] + PRVM_serveredictvector(pusher, origin)[i] - 1;
				maxs[i] = pushermodel->rotatedmaxs[i] + move1[i] + PRVM_serveredictvector(pusher, origin)[i] + 1;
			}
			else
			{
				mins[i] = pushermodel->rotatedmins[i] + move1[i] + PRVM_serveredictvector(pusher, origin)[i] - 1;
				maxs[i] = pushermodel->rotatedmaxs[i] + PRVM_serveredictvector(pusher, origin)[i] + 1;
			}
		}
	}
	else if (moveangle[1])
	{
		for (i = 0;i < 3;i++)
		{
			if (move1[i] > 0)
			{
				mins[i] = pushermodel->yawmins[i] + PRVM_serveredictvector(pusher, origin)[i] - 1;
				maxs[i] = pushermodel->yawmaxs[i] + move1[i] + PRVM_serveredictvector(pusher, origin)[i] + 1;
			}
			else
			{
				mins[i] = pushermodel->yawmins[i] + move1[i] + PRVM_serveredictvector(pusher, origin)[i] - 1;
				maxs[i] = pushermodel->yawmaxs[i] + PRVM_serveredictvector(pusher, origin)[i] + 1;
			}
		}
	}
	else
	{
		for (i = 0;i < 3;i++)
		{
			if (move1[i] > 0)
			{
				mins[i] = pushermodel->normalmins[i] + PRVM_serveredictvector(pusher, origin)[i] - 1;
				maxs[i] = pushermodel->normalmaxs[i] + move1[i] + PRVM_serveredictvector(pusher, origin)[i] + 1;
			}
			else
			{
				mins[i] = pushermodel->normalmins[i] + move1[i] + PRVM_serveredictvector(pusher, origin)[i] - 1;
				maxs[i] = pushermodel->normalmaxs[i] + PRVM_serveredictvector(pusher, origin)[i] + 1;
			}
		}
	}

	VectorNegate (moveangle, a);
	AngleVectorsFLU (a, forward, left, up);

	VectorCopy (PRVM_serveredictvector(pusher, origin), pushorig);
	VectorCopy (PRVM_serveredictvector(pusher, angles), pushang);
	pushltime = PRVM_serveredictfloat(pusher, ltime);

// move the pusher to its final position

	VectorMA (PRVM_serveredictvector(pusher, origin), movetime, PRVM_serveredictvector(pusher, velocity), PRVM_serveredictvector(pusher, origin));
	VectorMA (PRVM_serveredictvector(pusher, angles), movetime, PRVM_serveredictvector(pusher, avelocity), PRVM_serveredictvector(pusher, angles));
	PRVM_serveredictfloat(pusher, ltime) += movetime;
	SV_LinkEdict(pusher);

	pushermodel = SV_GetModelFromEdict(pusher);
	Matrix4x4_CreateFromQuakeEntity(&pusherfinalmatrix, PRVM_serveredictvector(pusher, origin)[0], PRVM_serveredictvector(pusher, origin)[1], PRVM_serveredictvector(pusher, origin)[2], PRVM_serveredictvector(pusher, angles)[0], PRVM_serveredictvector(pusher, angles)[1], PRVM_serveredictvector(pusher, angles)[2], 1);
	Matrix4x4_Invert_Simple(&pusherfinalimatrix, &pusherfinalmatrix);

	savesolid = PRVM_serveredictfloat(pusher, solid);

// see if any solid entities are inside the final position
	num_moved = 0;

	if (PRVM_serveredictfloat(pusher, movetype) == MOVETYPE_FAKEPUSH) // Tenebrae's MOVETYPE_PUSH variant that doesn't push...
		numcheckentities = 0;
	else // MOVETYPE_PUSH
	        numcheckentities = SV_EntitiesInBox(mins, maxs, MAX_EDICTS, checkentities);
	for (e = 0;e < numcheckentities;e++)
	{
		prvm_edict_t *check = checkentities[e];
		int movetype = (int)PRVM_serveredictfloat(check, movetype);
		switch(movetype)
		{
		case MOVETYPE_NONE:
		case MOVETYPE_PUSH:
		case MOVETYPE_FOLLOW:
		case MOVETYPE_NOCLIP:
		case MOVETYPE_FLY_WORLDONLY:
			continue;
		default:
			break;
		}

		if (PRVM_serveredictedict(check, owner) == pusherprog)
			continue;

		if (pusherowner == PRVM_EDICT_TO_PROG(check))
			continue;

		//Con_Printf("%i %s ", PRVM_NUM_FOR_EDICT(check), PRVM_GetString(PRVM_serveredictstring(check, classname)));

		// tell any MOVETYPE_STEP entity that it may need to check for water transitions
		check->priv.server->waterposition_forceupdate = true;

		checkcontents = SV_GenericHitSuperContentsMask(check);

		// if the entity is standing on the pusher, it will definitely be moved
		// if the entity is not standing on the pusher, but is in the pusher's
		// final position, move it
		if (!((int)PRVM_serveredictfloat(check, flags) & FL_ONGROUND) || PRVM_PROG_TO_EDICT(PRVM_serveredictedict(check, groundentity)) != pusher)
		{
			VectorCopy(PRVM_serveredictvector(pusher, mins), pushermins);
			VectorCopy(PRVM_serveredictvector(pusher, maxs), pushermaxs);
			VectorCopy(PRVM_serveredictvector(check, origin), checkorigin);
			VectorCopy(PRVM_serveredictvector(check, mins), checkmins);
			VectorCopy(PRVM_serveredictvector(check, maxs), checkmaxs);
			Collision_ClipToGenericEntity(&trace, pushermodel, pusher->priv.server->frameblend, &pusher->priv.server->skeleton, pushermins, pushermaxs, SUPERCONTENTS_BODY, &pusherfinalmatrix, &pusherfinalimatrix, checkorigin, checkmins, checkmaxs, checkorigin, checkcontents, 0, 0, collision_extendmovelength.value);
			//trace = SV_TraceBox(PRVM_serveredictvector(check, origin), PRVM_serveredictvector(check, mins), PRVM_serveredictvector(check, maxs), PRVM_serveredictvector(check, origin), MOVE_NOMONSTERS, check, checkcontents);
			if (!trace.startsolid)
			{
				//Con_Printf("- not in solid\n");
				continue;
			}
		}

		VectorLerp(PRVM_serveredictvector(check, mins), 0.5f, PRVM_serveredictvector(check, maxs), pivot);
		//VectorClear(pivot);

		if (rotated)
		{
			vec3_t org2;
			VectorSubtract (PRVM_serveredictvector(check, origin), PRVM_serveredictvector(pusher, origin), org);
			VectorAdd (org, pivot, org);
			org2[0] = DotProduct (org, forward);
			org2[1] = DotProduct (org, left);
			org2[2] = DotProduct (org, up);
			VectorSubtract (org2, org, move);
			VectorAdd (move, move1, move);
		}
		else
			VectorCopy (move1, move);

		//Con_Printf("- pushing %f %f %f\n", move[0], move[1], move[2]);

		VectorCopy (PRVM_serveredictvector(check, origin), check->priv.server->moved_from);
		VectorCopy (PRVM_serveredictvector(check, angles), check->priv.server->moved_fromangles);
		moved_edicts[num_moved++] = PRVM_NUM_FOR_EDICT(check);

		// physics objects need better collisions than this code can do
		if (movetype == MOVETYPE_PHYSICS)
		{
			VectorAdd(PRVM_serveredictvector(check, origin), move, PRVM_serveredictvector(check, origin));
			SV_LinkEdict(check);
			SV_LinkEdict_TouchAreaGrid(check);
			continue;
		}

		// try moving the contacted entity
		PRVM_serveredictfloat(pusher, solid) = SOLID_NOT;
		if(!SV_PushEntity(&trace, check, move, true, true))
		{
			// entity "check" got teleported
			PRVM_serveredictvector(check, angles)[1] += trace.fraction * moveangle[1];
			PRVM_serveredictfloat(pusher, solid) = savesolid; // was SOLID_BSP
			continue; // pushed enough
		}
		// FIXME: turn players specially
		PRVM_serveredictvector(check, angles)[1] += trace.fraction * moveangle[1];
		PRVM_serveredictfloat(pusher, solid) = savesolid; // was SOLID_BSP
		//Con_Printf("%s:%d frac %f startsolid %d bmodelstartsolid %d allsolid %d\n", __FILE__, __LINE__, trace.fraction, trace.startsolid, trace.bmodelstartsolid, trace.allsolid);

		// this trace.fraction < 1 check causes items to fall off of pushers
		// if they pass under or through a wall
		// the groundentity check causes items to fall off of ledges
		if (PRVM_serveredictfloat(check, movetype) != MOVETYPE_WALK && (trace.fraction < 1 || PRVM_PROG_TO_EDICT(PRVM_serveredictedict(check, groundentity)) != pusher))
			PRVM_serveredictfloat(check, flags) = (int)PRVM_serveredictfloat(check, flags) & ~FL_ONGROUND;

		// if it is still inside the pusher, block
		VectorCopy(PRVM_serveredictvector(pusher, mins), pushermins);
		VectorCopy(PRVM_serveredictvector(pusher, maxs), pushermaxs);
		VectorCopy(PRVM_serveredictvector(check, origin), checkorigin);
		VectorCopy(PRVM_serveredictvector(check, mins), checkmins);
		VectorCopy(PRVM_serveredictvector(check, maxs), checkmaxs);
		Collision_ClipToGenericEntity(&trace, pushermodel, pusher->priv.server->frameblend, &pusher->priv.server->skeleton, pushermins, pushermaxs, SUPERCONTENTS_BODY, &pusherfinalmatrix, &pusherfinalimatrix, checkorigin, checkmins, checkmaxs, checkorigin, checkcontents, 0, 0, collision_extendmovelength.value);
		if (trace.startsolid)
		{
			vec3_t move2;
			if(SV_NudgeOutOfSolid_PivotIsKnownGood(check, pivot))
			{
				// hack to invoke all necessary movement triggers
				VectorClear(move2);
				if(!SV_PushEntity(&trace2, check, move2, true, true))
				{
					// entity "check" got teleported
					continue;
				}
				// we could fix it
				continue;
			}

			// still inside pusher, so it's really blocked

			// fail the move
			if (PRVM_serveredictvector(check, mins)[0] == PRVM_serveredictvector(check, maxs)[0])
				continue;
			if (PRVM_serveredictfloat(check, solid) == SOLID_NOT || PRVM_serveredictfloat(check, solid) == SOLID_TRIGGER)
			{
				// corpse
				PRVM_serveredictvector(check, mins)[0] = PRVM_serveredictvector(check, mins)[1] = 0;
				VectorCopy (PRVM_serveredictvector(check, mins), PRVM_serveredictvector(check, maxs));
				continue;
			}

			VectorCopy (pushorig, PRVM_serveredictvector(pusher, origin));
			VectorCopy (pushang, PRVM_serveredictvector(pusher, angles));
			PRVM_serveredictfloat(pusher, ltime) = pushltime;
			SV_LinkEdict(pusher);

			// move back any entities we already moved
			for (i = 0;i < num_moved;i++)
			{
				prvm_edict_t *ed = PRVM_EDICT_NUM(moved_edicts[i]);
				VectorCopy (ed->priv.server->moved_from, PRVM_serveredictvector(ed, origin));
				VectorCopy (ed->priv.server->moved_fromangles, PRVM_serveredictvector(ed, angles));
				SV_LinkEdict(ed);
			}

			// if the pusher has a "blocked" function, call it, otherwise just stay in place until the obstacle is gone
			if (PRVM_serveredictfunction(pusher, blocked))
			{
				PRVM_serverglobalfloat(time) = sv.time;
				PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(pusher);
				PRVM_serverglobaledict(other) = PRVM_EDICT_TO_PROG(check);
				prog->ExecuteProgram(prog, PRVM_serveredictfunction(pusher, blocked), "QC function self.blocked is missing");
			}
			break;
		}
	}
	PRVM_serveredictvector(pusher, angles)[0] -= 360.0 * floor(PRVM_serveredictvector(pusher, angles)[0] * (1.0 / 360.0));
	PRVM_serveredictvector(pusher, angles)[1] -= 360.0 * floor(PRVM_serveredictvector(pusher, angles)[1] * (1.0 / 360.0));
	PRVM_serveredictvector(pusher, angles)[2] -= 360.0 * floor(PRVM_serveredictvector(pusher, angles)[2] * (1.0 / 360.0));
}

/*
================
SV_Physics_Pusher

================
*/
static void SV_Physics_Pusher (prvm_edict_t *ent)
{
	prvm_prog_t *prog = SVVM_prog;
	prvm_vec_t thinktime, oldltime, movetime;

	oldltime = PRVM_serveredictfloat(ent, ltime);

	thinktime = PRVM_serveredictfloat(ent, nextthink);
	if (thinktime < PRVM_serveredictfloat(ent, ltime) + sv.frametime)
	{
		movetime = thinktime - PRVM_serveredictfloat(ent, ltime);
		if (movetime < 0)
			movetime = 0;
	}
	else
		movetime = sv.frametime;

	if (movetime)
		// advances PRVM_serveredictfloat(ent, ltime) if not blocked
		SV_PushMove (ent, movetime);

	if (thinktime > oldltime && thinktime <= PRVM_serveredictfloat(ent, ltime))
	{
		PRVM_serveredictfloat(ent, nextthink) = 0;
		PRVM_serverglobalfloat(time) = sv.time;
		PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(ent);
		PRVM_serverglobaledict(other) = PRVM_EDICT_TO_PROG(prog->edicts);
		prog->ExecuteProgram(prog, PRVM_serveredictfunction(ent, think), "QC function self.think is missing");
	}
}


/*
===============================================================================

CLIENT MOVEMENT

===============================================================================
*/

static float unstickoffsets[] =
{
	// poutting -/+z changes first as they are least weird
	 0,  0,  -1,
	 0,  0,  1,
	 // x or y changes
	-1,  0,  0,
	 1,  0,  0,
	 0, -1,  0,
	 0,  1,  0,
	 // x and y changes
	-1, -1,  0,
	 1, -1,  0,
	-1,  1,  0,
	 1,  1,  0,
};

typedef enum unstickresult_e
{
	UNSTICK_STUCK = 0,
	UNSTICK_GOOD = 1,
	UNSTICK_UNSTUCK = 2
}
unstickresult_t;

static unstickresult_t SV_UnstickEntityReturnOffset (prvm_edict_t *ent, vec3_t offset)
{
	prvm_prog_t *prog = SVVM_prog;
	int i, maxunstick;

	// if not stuck in a bmodel, just return
	if (!SV_TestEntityPosition(ent, vec3_origin))
		return UNSTICK_GOOD;

	for (i = 0;i < (int)(sizeof(unstickoffsets) / sizeof(unstickoffsets[0]));i += 3)
	{
		if (!SV_TestEntityPosition(ent, unstickoffsets + i))
		{
			VectorCopy(unstickoffsets + i, offset);
			SV_LinkEdict(ent);
			//SV_LinkEdict_TouchAreaGrid(ent);
			return UNSTICK_UNSTUCK;
		}
	}

	maxunstick = (int) ((PRVM_serveredictvector(ent, maxs)[2] - PRVM_serveredictvector(ent, mins)[2]) * 0.36);
	// magic number 0.36 allows unsticking by up to 17 units with the largest supported bbox

	for(i = 2; i <= maxunstick; ++i)
	{
		VectorClear(offset);
		offset[2] = -i;
		if (!SV_TestEntityPosition(ent, offset))
		{
			SV_LinkEdict(ent);
			//SV_LinkEdict_TouchAreaGrid(ent);
			return UNSTICK_UNSTUCK;
		}
		offset[2] = i;
		if (!SV_TestEntityPosition(ent, offset))
		{
			SV_LinkEdict(ent);
			//SV_LinkEdict_TouchAreaGrid(ent);
			return UNSTICK_UNSTUCK;
		}
	}

	return UNSTICK_STUCK;
}

qbool SV_UnstickEntity (prvm_edict_t *ent)
{
	prvm_prog_t *prog = SVVM_prog;
	vec3_t offset;
	switch(SV_UnstickEntityReturnOffset(ent, offset))
	{
		case UNSTICK_GOOD:
			return true;
		case UNSTICK_UNSTUCK:
			Con_DPrintf("Unstuck entity %i (classname \"%s\") with offset %f %f %f.\n", (int)PRVM_EDICT_TO_PROG(ent), PRVM_GetString(prog, PRVM_serveredictstring(ent, classname)), offset[0], offset[1], offset[2]);
			return true;
		case UNSTICK_STUCK:
			if (developer_extra.integer)
				Con_DPrintf("Stuck entity %i (classname \"%s\").\n", (int)PRVM_EDICT_TO_PROG(ent), PRVM_GetString(prog, PRVM_serveredictstring(ent, classname)));
			return false;
		default:
			Con_Printf("SV_UnstickEntityReturnOffset returned a value outside its enum.\n");
			return false;
	}
}

/*
=============
SV_CheckStuck

This is a big hack to try and fix the rare case of getting stuck in the world
clipping hull.
=============
*/
static void SV_CheckStuck (prvm_edict_t *ent)
{
	prvm_prog_t *prog = SVVM_prog;
	vec3_t offset;

	switch(SV_UnstickEntityReturnOffset(ent, offset))
	{
		case UNSTICK_GOOD:
			VectorCopy (PRVM_serveredictvector(ent, origin), PRVM_serveredictvector(ent, oldorigin));
			break;
		case UNSTICK_UNSTUCK:
			Con_DPrintf("Unstuck player entity %i (classname \"%s\") with offset %f %f %f.\n", (int)PRVM_EDICT_TO_PROG(ent), PRVM_GetString(prog, PRVM_serveredictstring(ent, classname)), offset[0], offset[1], offset[2]);
			break;
		case UNSTICK_STUCK:
			VectorSubtract(PRVM_serveredictvector(ent, oldorigin), PRVM_serveredictvector(ent, origin), offset);
			if (!SV_TestEntityPosition(ent, offset))
			{
				Con_DPrintf("Unstuck player entity %i (classname \"%s\") by restoring oldorigin.\n", (int)PRVM_EDICT_TO_PROG(ent), PRVM_GetString(prog, PRVM_serveredictstring(ent, classname)));
				SV_LinkEdict(ent);
				//SV_LinkEdict_TouchAreaGrid(ent);
			}
			else
				Con_DPrintf("Stuck player entity %i (classname \"%s\").\n", (int)PRVM_EDICT_TO_PROG(ent), PRVM_GetString(prog, PRVM_serveredictstring(ent, classname)));
			break;
		default:
			Con_Printf("SV_UnstickEntityReturnOffset returned a value outside its enum.\n");
	}
}


/*
=============
SV_CheckWater
=============
*/
static qbool SV_CheckWater (prvm_edict_t *ent)
{
	prvm_prog_t *prog = SVVM_prog;
	int cont;
	int nNativeContents;
	vec3_t point;

	point[0] = PRVM_serveredictvector(ent, origin)[0];
	point[1] = PRVM_serveredictvector(ent, origin)[1];
	point[2] = PRVM_serveredictvector(ent, origin)[2] + PRVM_serveredictvector(ent, mins)[2] + 1;

	// DRESK - Support for Entity Contents Transition Event
	// NOTE: Some logic needed to be slightly re-ordered
	// to not affect performance and allow for the feature.

	// Acquire Super Contents Prior to Resets
	cont = SV_PointSuperContents(point);
	// Acquire Native Contents Here
	nNativeContents = Mod_Q1BSP_NativeContentsFromSuperContents(cont);

	// DRESK - Support for Entity Contents Transition Event
	if(PRVM_serveredictfloat(ent, watertype))
		// Entity did NOT Spawn; Check
		SV_CheckContentsTransition(ent, nNativeContents);


	PRVM_serveredictfloat(ent, waterlevel) = 0;
	PRVM_serveredictfloat(ent, watertype) = CONTENTS_EMPTY;
	cont = SV_PointSuperContents(point);
	if (cont & (SUPERCONTENTS_LIQUIDSMASK))
	{
		PRVM_serveredictfloat(ent, watertype) = nNativeContents;
		PRVM_serveredictfloat(ent, waterlevel) = 1;
		point[2] = PRVM_serveredictvector(ent, origin)[2] + (PRVM_serveredictvector(ent, mins)[2] + PRVM_serveredictvector(ent, maxs)[2])*0.5;
		if (SV_PointSuperContents(point) & (SUPERCONTENTS_LIQUIDSMASK))
		{
			PRVM_serveredictfloat(ent, waterlevel) = 2;
			point[2] = PRVM_serveredictvector(ent, origin)[2] + PRVM_serveredictvector(ent, view_ofs)[2];
			if (SV_PointSuperContents(point) & (SUPERCONTENTS_LIQUIDSMASK))
				PRVM_serveredictfloat(ent, waterlevel) = 3;
		}
	}

	return PRVM_serveredictfloat(ent, waterlevel) > 1;
}

/*
============
SV_WallFriction

============
*/
static void SV_WallFriction (prvm_edict_t *ent, float *stepnormal)
{
	prvm_prog_t *prog = SVVM_prog;
	float d, i;
	vec3_t forward, into, side, v_angle;

	VectorCopy(PRVM_serveredictvector(ent, v_angle), v_angle);
	AngleVectors (v_angle, forward, NULL, NULL);
	if ((d = DotProduct (stepnormal, forward) + 0.5) < 0)
	{
		// cut the tangential velocity
		i = DotProduct (stepnormal, PRVM_serveredictvector(ent, velocity));
		VectorScale (stepnormal, i, into);
		VectorSubtract (PRVM_serveredictvector(ent, velocity), into, side);
		PRVM_serveredictvector(ent, velocity)[0] = side[0] * (1 + d);
		PRVM_serveredictvector(ent, velocity)[1] = side[1] * (1 + d);
	}
}

#if 0
/*
=====================
SV_TryUnstick

Player has come to a dead stop, possibly due to the problem with limited
float precision at some angle joins in the BSP hull.

Try fixing by pushing one pixel in each direction.

This is a hack, but in the interest of good gameplay...
======================
*/
int SV_TryUnstick (prvm_edict_t *ent, vec3_t oldvel)
{
	int i, clip;
	vec3_t oldorg, dir;

	VectorCopy (PRVM_serveredictvector(ent, origin), oldorg);
	VectorClear (dir);

	for (i=0 ; i<8 ; i++)
	{
		// try pushing a little in an axial direction
		switch (i)
		{
			case 0: dir[0] = 2; dir[1] = 0; break;
			case 1: dir[0] = 0; dir[1] = 2; break;
			case 2: dir[0] = -2; dir[1] = 0; break;
			case 3: dir[0] = 0; dir[1] = -2; break;
			case 4: dir[0] = 2; dir[1] = 2; break;
			case 5: dir[0] = -2; dir[1] = 2; break;
			case 6: dir[0] = 2; dir[1] = -2; break;
			case 7: dir[0] = -2; dir[1] = -2; break;
		}

		SV_PushEntity (&trace, ent, dir, false, true);

		// retry the original move
		PRVM_serveredictvector(ent, velocity)[0] = oldvel[0];
		PRVM_serveredictvector(ent, velocity)[1] = oldvel[1];
		PRVM_serveredictvector(ent, velocity)[2] = 0;
		clip = SV_FlyMove (ent, 0.1, NULL, SV_GenericHitSuperContentsMask(ent), 0);

		if (fabs(oldorg[1] - PRVM_serveredictvector(ent, origin)[1]) > 4
		 || fabs(oldorg[0] - PRVM_serveredictvector(ent, origin)[0]) > 4)
		{
			Con_DPrint("TryUnstick - success.\n");
			return clip;
		}

		// go back to the original pos and try again
		VectorCopy (oldorg, PRVM_serveredictvector(ent, origin));
	}

	// still not moving
	VectorClear (PRVM_serveredictvector(ent, velocity));
	Con_DPrint("TryUnstick - failure.\n");
	return 7;
}
#endif

/*
=====================
SV_WalkMove

Only used by players
======================
*/
static void SV_WalkMove (prvm_edict_t *ent)
{
	prvm_prog_t *prog = SVVM_prog;
	int clip;
	int oldonground;
	//int originalmove_clip;
	int originalmove_flags;
	int originalmove_groundentity;
	int hitsupercontentsmask = SV_GenericHitSuperContentsMask(ent);
	int skipsupercontentsmask = 0;
	int skipmaterialflagsmask = 0;
	int type;
	vec3_t upmove, downmove, start_origin, start_velocity, stepnormal, originalmove_origin, originalmove_velocity, entmins, entmaxs;
	trace_t downtrace, trace;
	qbool applygravity;

	// if frametime is 0 (due to client sending the same timestamp twice),
	// don't move
	if (sv.frametime <= 0)
		return;

	if (sv_gameplayfix_unstickplayers.integer)
		SV_CheckStuck (ent);

	applygravity = !SV_CheckWater (ent) && PRVM_serveredictfloat(ent, movetype) == MOVETYPE_WALK && ! ((int)PRVM_serveredictfloat(ent, flags) & FL_WATERJUMP);

	SV_CheckVelocity(ent);

	// do a regular slide move unless it looks like you ran into a step
	oldonground = (int)PRVM_serveredictfloat(ent, flags) & FL_ONGROUND;

	VectorCopy (PRVM_serveredictvector(ent, origin), start_origin);
	VectorCopy (PRVM_serveredictvector(ent, velocity), start_velocity);

	clip = SV_FlyMove (ent, sv.frametime, applygravity, NULL, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, sv_gameplayfix_stepmultipletimes.integer ? sv_stepheight.value : 0);

	if(sv_gameplayfix_downtracesupportsongroundflag.integer)
	if(!(clip & 1))
	{
		// only try this if there was no floor in the way in the trace (no,
		// this check seems to be not REALLY necessary, because if clip & 1,
		// our trace will hit that thing too)
		VectorSet(upmove, PRVM_serveredictvector(ent, origin)[0], PRVM_serveredictvector(ent, origin)[1], PRVM_serveredictvector(ent, origin)[2] + 1);
		VectorSet(downmove, PRVM_serveredictvector(ent, origin)[0], PRVM_serveredictvector(ent, origin)[1], PRVM_serveredictvector(ent, origin)[2] - 1);
		if (PRVM_serveredictfloat(ent, movetype) == MOVETYPE_FLYMISSILE)
			type = MOVE_MISSILE;
		else if (PRVM_serveredictfloat(ent, movetype) == MOVETYPE_FLY_WORLDONLY)
			type = MOVE_WORLDONLY;
		else if (PRVM_serveredictfloat(ent, solid) == SOLID_TRIGGER || PRVM_serveredictfloat(ent, solid) == SOLID_NOT)
			type = MOVE_NOMONSTERS; // only clip against bmodels
		else
			type = MOVE_NORMAL;
		VectorCopy(PRVM_serveredictvector(ent, mins), entmins);
		VectorCopy(PRVM_serveredictvector(ent, maxs), entmaxs);
		trace = SV_TraceBox(upmove, entmins, entmaxs, downmove, type, ent, SV_GenericHitSuperContentsMask(ent), skipsupercontentsmask, skipmaterialflagsmask, collision_extendmovelength.value);
		if(trace.fraction < 1 && trace.plane.normal[2] > 0.7)
		{
			clip |= 1; // but we HAVE found a floor
			// set groundentity so we get carried when walking onto a mover with sv_gameplayfix_nogravityonground
			PRVM_serveredictedict(ent, groundentity) = PRVM_EDICT_TO_PROG(trace.ent);
		}
	}

	// if the move did not hit the ground at any point, we're not on ground
	if(!(clip & 1))
		PRVM_serveredictfloat(ent, flags) = (int)PRVM_serveredictfloat(ent, flags) & ~FL_ONGROUND;

	SV_CheckVelocity(ent);
	SV_LinkEdict(ent);
	SV_LinkEdict_TouchAreaGrid(ent);

	if(clip & 8) // teleport
		return;

	if ((int)PRVM_serveredictfloat(ent, flags) & FL_WATERJUMP)
		return;

	if (sv_nostep.integer)
		return;

	VectorCopy(PRVM_serveredictvector(ent, origin), originalmove_origin);
	VectorCopy(PRVM_serveredictvector(ent, velocity), originalmove_velocity);
	//originalmove_clip = clip;
	originalmove_flags = (int)PRVM_serveredictfloat(ent, flags);
	originalmove_groundentity = PRVM_serveredictedict(ent, groundentity);

	// if move didn't block on a step, return
	if (clip & 2)
	{
		// if move was not trying to move into the step, return
		if (fabs(start_velocity[0]) < 0.03125 && fabs(start_velocity[1]) < 0.03125)
			return;

		if (PRVM_serveredictfloat(ent, movetype) != MOVETYPE_FLY)
		{
			// return if gibbed by a trigger
			if (PRVM_serveredictfloat(ent, movetype) != MOVETYPE_WALK)
				return;

			// return if attempting to jump while airborn (unless sv_jumpstep)
			if (!sv_jumpstep.integer)
				if (!oldonground && PRVM_serveredictfloat(ent, waterlevel) == 0)
					return;
		}

		// try moving up and forward to go up a step
		// back to start pos
		VectorCopy (start_origin, PRVM_serveredictvector(ent, origin));
		VectorCopy (start_velocity, PRVM_serveredictvector(ent, velocity));

		// move up
		VectorClear (upmove);
		upmove[2] = sv_stepheight.value;
		if(!SV_PushEntity(&trace, ent, upmove, true, true))
		{
			// we got teleported when upstepping... must abort the move
			return;
		}

		// move forward
		PRVM_serveredictvector(ent, velocity)[2] = 0;
		clip = SV_FlyMove (ent, sv.frametime, applygravity, stepnormal, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, 0);
		PRVM_serveredictvector(ent, velocity)[2] += start_velocity[2];
		if(clip & 8)
		{
			// we got teleported when upstepping... must abort the move
			// note that z velocity handling may not be what QC expects here, but we cannot help it
			return;
		}

		SV_CheckVelocity(ent);
		SV_LinkEdict(ent);
		SV_LinkEdict_TouchAreaGrid(ent);

		// check for stuckness, possibly due to the limited precision of floats
		// in the clipping hulls
		if (clip
		 && fabs(originalmove_origin[1] - PRVM_serveredictvector(ent, origin)[1]) < 0.03125
		 && fabs(originalmove_origin[0] - PRVM_serveredictvector(ent, origin)[0]) < 0.03125)
		{
			//Con_Printf("wall\n");
			// stepping up didn't make any progress, revert to original move
			VectorCopy(originalmove_origin, PRVM_serveredictvector(ent, origin));
			VectorCopy(originalmove_velocity, PRVM_serveredictvector(ent, velocity));
			//clip = originalmove_clip;
			PRVM_serveredictfloat(ent, flags) = originalmove_flags;
			PRVM_serveredictedict(ent, groundentity) = originalmove_groundentity;
			// now try to unstick if needed
			//clip = SV_TryUnstick (ent, oldvel);
			return;
		}

		//Con_Printf("step - ");

		// extra friction based on view angle
		if (clip & 2 && sv_wallfriction.integer)
			SV_WallFriction (ent, stepnormal);
	}
	// don't do the down move if stepdown is disabled, moving upward, not in water, or the move started offground or ended onground
	else if (!sv_gameplayfix_stepdown.integer || PRVM_serveredictfloat(ent, waterlevel) >= 3 || start_velocity[2] >= (1.0 / 32.0) || !oldonground || ((int)PRVM_serveredictfloat(ent, flags) & FL_ONGROUND))
		return;

	// move down
	VectorClear (downmove);
	downmove[2] = -sv_stepheight.value + start_velocity[2]*sv.frametime;
	if(!SV_PushEntity(&downtrace, ent, downmove, true, true))
	{
		// we got teleported when downstepping... must abort the move
		return;
	}

	if (downtrace.fraction < 1 && downtrace.plane.normal[2] > 0.7)
	{
		// this has been disabled so that you can't jump when you are stepping
		// up while already jumping (also known as the Quake2 double jump bug)
#if 0
		// LadyHavoc: disabled this check so you can walk on monsters/players
		//if (PRVM_serveredictfloat(ent, solid) == SOLID_BSP)
		{
			//Con_Printf("onground\n");
			PRVM_serveredictfloat(ent, flags) =	(int)PRVM_serveredictfloat(ent, flags) | FL_ONGROUND;
			PRVM_serveredictedict(ent, groundentity) = PRVM_EDICT_TO_PROG(downtrace.ent);
		}
#endif
	}
	else
	{
		//Con_Printf("slope\n");
		// if the push down didn't end up on good ground, use the move without
		// the step up.  This happens near wall / slope combinations, and can
		// cause the player to hop up higher on a slope too steep to climb
		VectorCopy(originalmove_origin, PRVM_serveredictvector(ent, origin));
		VectorCopy(originalmove_velocity, PRVM_serveredictvector(ent, velocity));
		//clip = originalmove_clip;
		PRVM_serveredictfloat(ent, flags) = originalmove_flags;
		PRVM_serveredictedict(ent, groundentity) = originalmove_groundentity;
	}

	SV_CheckVelocity(ent);
	SV_LinkEdict(ent);
	SV_LinkEdict_TouchAreaGrid(ent);
}

//============================================================================

/*
=============
SV_Physics_Follow

Entities that are "stuck" to another entity
=============
*/
static void SV_Physics_Follow (prvm_edict_t *ent)
{
	prvm_prog_t *prog = SVVM_prog;
	vec3_t vf, vr, vu, angles, v;
	prvm_edict_t *e;

	// LadyHavoc: implemented rotation on MOVETYPE_FOLLOW objects
	e = PRVM_PROG_TO_EDICT(PRVM_serveredictedict(ent, aiment));
	if (PRVM_serveredictvector(e, angles)[0] == PRVM_serveredictvector(ent, punchangle)[0] && PRVM_serveredictvector(e, angles)[1] == PRVM_serveredictvector(ent, punchangle)[1] && PRVM_serveredictvector(e, angles)[2] == PRVM_serveredictvector(ent, punchangle)[2])
	{
		// quick case for no rotation
		VectorAdd(PRVM_serveredictvector(e, origin), PRVM_serveredictvector(ent, view_ofs), PRVM_serveredictvector(ent, origin));
	}
	else
	{
		angles[0] = -PRVM_serveredictvector(ent, punchangle)[0];
		angles[1] =  PRVM_serveredictvector(ent, punchangle)[1];
		angles[2] =  PRVM_serveredictvector(ent, punchangle)[2];
		AngleVectors (angles, vf, vr, vu);
		v[0] = PRVM_serveredictvector(ent, view_ofs)[0] * vf[0] + PRVM_serveredictvector(ent, view_ofs)[1] * vr[0] + PRVM_serveredictvector(ent, view_ofs)[2] * vu[0];
		v[1] = PRVM_serveredictvector(ent, view_ofs)[0] * vf[1] + PRVM_serveredictvector(ent, view_ofs)[1] * vr[1] + PRVM_serveredictvector(ent, view_ofs)[2] * vu[1];
		v[2] = PRVM_serveredictvector(ent, view_ofs)[0] * vf[2] + PRVM_serveredictvector(ent, view_ofs)[1] * vr[2] + PRVM_serveredictvector(ent, view_ofs)[2] * vu[2];
		angles[0] = -PRVM_serveredictvector(e, angles)[0];
		angles[1] =  PRVM_serveredictvector(e, angles)[1];
		angles[2] =  PRVM_serveredictvector(e, angles)[2];
		AngleVectors (angles, vf, vr, vu);
		PRVM_serveredictvector(ent, origin)[0] = v[0] * vf[0] + v[1] * vf[1] + v[2] * vf[2] + PRVM_serveredictvector(e, origin)[0];
		PRVM_serveredictvector(ent, origin)[1] = v[0] * vr[0] + v[1] * vr[1] + v[2] * vr[2] + PRVM_serveredictvector(e, origin)[1];
		PRVM_serveredictvector(ent, origin)[2] = v[0] * vu[0] + v[1] * vu[1] + v[2] * vu[2] + PRVM_serveredictvector(e, origin)[2];
	}
	VectorAdd (PRVM_serveredictvector(e, angles), PRVM_serveredictvector(ent, v_angle), PRVM_serveredictvector(ent, angles));
	SV_LinkEdict(ent);
	//SV_LinkEdict_TouchAreaGrid(ent);
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
static void SV_CheckWaterTransition (prvm_edict_t *ent)
{
	vec3_t entorigin;
	prvm_prog_t *prog = SVVM_prog;
	// LadyHavoc: bugfixes in this function are keyed to the sv_gameplayfix_bugfixedcheckwatertransition cvar - if this cvar is 0 then all the original bugs should be reenabled for compatibility
	int cont;
	VectorCopy(PRVM_serveredictvector(ent, origin), entorigin);
	cont = Mod_Q1BSP_NativeContentsFromSuperContents(SV_PointSuperContents(entorigin));
	if (!PRVM_serveredictfloat(ent, watertype))
	{
		// just spawned here
		if (!sv_gameplayfix_fixedcheckwatertransition.integer)
		{
			PRVM_serveredictfloat(ent, watertype) = cont;
			PRVM_serveredictfloat(ent, waterlevel) = 1;
			return;
		}
	}
	// DRESK - Support for Entity Contents Transition Event
	// NOTE: Call here BEFORE updating the watertype below,
	// and suppress watersplash sound if a valid function
	// call was made to allow for custom "splash" sounds.
	else if( !SV_CheckContentsTransition(ent, cont) )
	{ // Contents Transition Function Invalid; Potentially Play Water Sound
		// check if the entity crossed into or out of water
		if (sv_sound_watersplash.string && ((PRVM_serveredictfloat(ent, watertype) == CONTENTS_WATER || PRVM_serveredictfloat(ent, watertype) == CONTENTS_SLIME) != (cont == CONTENTS_WATER || cont == CONTENTS_SLIME)))
			SV_StartSound (ent, 0, sv_sound_watersplash.string, 255, 1, false, 1.0f);
	}

	if (cont <= CONTENTS_WATER)
	{
		PRVM_serveredictfloat(ent, watertype) = cont;
		PRVM_serveredictfloat(ent, waterlevel) = 1;
	}
	else
	{
		PRVM_serveredictfloat(ent, watertype) = CONTENTS_EMPTY;
		PRVM_serveredictfloat(ent, waterlevel) = sv_gameplayfix_fixedcheckwatertransition.integer ? 0 : cont;
	}
}

/*
=============
SV_Physics_Toss

Toss, bounce, and fly movement.  When onground, do nothing.
=============
*/

void SV_Physics_Toss (prvm_edict_t *ent)
{
	prvm_prog_t *prog = SVVM_prog;
	trace_t trace;
	vec3_t move;
	vec_t movetime;
	int bump;
	prvm_edict_t *groundentity;
	float d, ent_gravity;
	float bouncefactor;
	float bouncestop;

// if onground, return without moving
	if ((int)PRVM_serveredictfloat(ent, flags) & FL_ONGROUND)
	{
		groundentity = PRVM_PROG_TO_EDICT(PRVM_serveredictedict(ent, groundentity));
		if (PRVM_serveredictvector(ent, velocity)[2] >= (1.0 / 32.0) && sv_gameplayfix_upwardvelocityclearsongroundflag.integer)
		{
			// don't stick to ground if onground and moving upward
			PRVM_serveredictfloat(ent, flags) -= FL_ONGROUND;
		}
		else if (!PRVM_serveredictedict(ent, groundentity) || !sv_gameplayfix_noairborncorpse.integer)
		{
			// we can trust FL_ONGROUND if groundentity is world because it never moves
			return;
		}
		else if (ent->priv.server->suspendedinairflag && groundentity->free)
		{
			// if ent was supported by a brush model on previous frame,
			// and groundentity is now freed, set groundentity to 0 (world)
			// which leaves it suspended in the air
			PRVM_serveredictedict(ent, groundentity) = 0;
			if (sv_gameplayfix_noairborncorpse_allowsuspendeditems.integer)
				return;
		}
		else if (BoxesOverlap(ent->priv.server->cullmins, ent->priv.server->cullmaxs, groundentity->priv.server->cullmins, groundentity->priv.server->cullmaxs))
		{
			// don't slide if still touching the groundentity
			return;
		}
	}
	ent->priv.server->suspendedinairflag = false;

	SV_CheckVelocity (ent);

// add gravity
	if (PRVM_serveredictfloat(ent, movetype) == MOVETYPE_TOSS || PRVM_serveredictfloat(ent, movetype) == MOVETYPE_BOUNCE)
		PRVM_serveredictvector(ent, velocity)[2] -= SV_Gravity(ent);

// move angles
	VectorMA (PRVM_serveredictvector(ent, angles), sv.frametime, PRVM_serveredictvector(ent, avelocity), PRVM_serveredictvector(ent, angles));

	movetime = sv.frametime;
	for (bump = 0;bump < MAX_CLIP_PLANES && movetime > 0;bump++)
	{
	// move origin
		VectorScale(PRVM_serveredictvector(ent, velocity), movetime, move);
		// The buzzsaw traps in r2m6 and r2m7 use MOVETYPE_FLY and rely on moving while stuck in the world.
		// Quake movetypes checked allsolid only in SV_FlyMove().
		if(!SV_PushEntity(&trace, ent, move, true, PRVM_serveredictfloat(ent, movetype) != MOVETYPE_FLY))
			return; // teleported
		if (ent->free)
			return;
		if (trace.fraction == 1)
			break;
		movetime *= 1 - min(1, trace.fraction);
		switch((int)PRVM_serveredictfloat(ent, movetype))
		{
		case MOVETYPE_BOUNCEMISSILE:
			bouncefactor = PRVM_serveredictfloat(ent, bouncefactor);
			if (!bouncefactor)
				bouncefactor = 1.0f;

			ClipVelocity(PRVM_serveredictvector(ent, velocity), trace.plane.normal, PRVM_serveredictvector(ent, velocity), 1 + bouncefactor);
			PRVM_serveredictfloat(ent, flags) = (int)PRVM_serveredictfloat(ent, flags) & ~FL_ONGROUND;
			if (!sv_gameplayfix_slidemoveprojectiles.integer)
				movetime = 0;
			break;
		case MOVETYPE_BOUNCE:
			bouncefactor = PRVM_serveredictfloat(ent, bouncefactor);
			if (!bouncefactor)
				bouncefactor = 0.5f;

			bouncestop = PRVM_serveredictfloat(ent, bouncestop);
			if (!bouncestop)
				bouncestop = 60.0f / 800.0f;

			ClipVelocity(PRVM_serveredictvector(ent, velocity), trace.plane.normal, PRVM_serveredictvector(ent, velocity), 1 + bouncefactor);
			ent_gravity = PRVM_serveredictfloat(ent, gravity);
			if (!ent_gravity)
				ent_gravity = 1.0f;
			// LadyHavoc: fixed grenades not bouncing when fired down a slope
			if (sv_gameplayfix_grenadebouncedownslopes.integer)
				d = fabs(DotProduct(trace.plane.normal, PRVM_serveredictvector(ent, velocity)));
			else
				d = PRVM_serveredictvector(ent, velocity)[2];
			if (trace.plane.normal[2] > 0.7 && d < sv_gravity.value * bouncestop * ent_gravity)
			{
				PRVM_serveredictfloat(ent, flags) = (int)PRVM_serveredictfloat(ent, flags) | FL_ONGROUND;
				PRVM_serveredictedict(ent, groundentity) = PRVM_EDICT_TO_PROG(trace.ent);
				VectorClear(PRVM_serveredictvector(ent, velocity));
				VectorClear(PRVM_serveredictvector(ent, avelocity));
				movetime = 0;
			}
			else
			{
				PRVM_serveredictfloat(ent, flags) = (int)PRVM_serveredictfloat(ent, flags) & ~FL_ONGROUND;
				if (!sv_gameplayfix_slidemoveprojectiles.integer)
					movetime = 0;
			}
			break;
		default:
			ClipVelocity (PRVM_serveredictvector(ent, velocity), trace.plane.normal, PRVM_serveredictvector(ent, velocity), 1.0);
			if (trace.plane.normal[2] > 0.7)
			{
				PRVM_serveredictfloat(ent, flags) = (int)PRVM_serveredictfloat(ent, flags) | FL_ONGROUND;
				PRVM_serveredictedict(ent, groundentity) = PRVM_EDICT_TO_PROG(trace.ent);
				if (PRVM_serveredictfloat(((prvm_edict_t *)trace.ent), solid) == SOLID_BSP)
					ent->priv.server->suspendedinairflag = true;
				VectorClear (PRVM_serveredictvector(ent, velocity));
				VectorClear (PRVM_serveredictvector(ent, avelocity));
				movetime = 0;
			}
			else
			{
				PRVM_serveredictfloat(ent, flags) = (int)PRVM_serveredictfloat(ent, flags) & ~FL_ONGROUND;
				if (!sv_gameplayfix_slidemoveprojectiles.integer)
					movetime = 0;
			}
			break;
		}
	}

// check for in water
	SV_CheckWaterTransition (ent);
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
=============
*/
static void SV_Physics_Step (prvm_edict_t *ent)
{
	prvm_prog_t *prog = SVVM_prog;
	int flags = (int)PRVM_serveredictfloat(ent, flags);

	// DRESK
	// Backup Velocity in the event that movetypesteplandevent is called,
	// to provide a parameter with the entity's velocity at impact.
	vec3_t backupVelocity;
	VectorCopy(PRVM_serveredictvector(ent, velocity), backupVelocity);
	// don't fall at all if fly/swim
	if (!(flags & (FL_FLY | FL_SWIM)))
	{
		if (flags & FL_ONGROUND)
		{
			// freefall if onground and moving upward
			// freefall if not standing on a world surface (it may be a lift or trap door)
			if (PRVM_serveredictvector(ent, velocity)[2] >= (1.0 / 32.0) && sv_gameplayfix_upwardvelocityclearsongroundflag.integer)
			{
				PRVM_serveredictfloat(ent, flags) -= FL_ONGROUND;
				SV_CheckVelocity(ent);
				SV_FlyMove(ent, sv.frametime, true, NULL, SV_GenericHitSuperContentsMask(ent), 0, 0, 0);
				SV_LinkEdict(ent);
				SV_LinkEdict_TouchAreaGrid(ent);
				ent->priv.server->waterposition_forceupdate = true;
			}
		}
		else
		{
			// freefall if not onground
			int hitsound = PRVM_serveredictvector(ent, velocity)[2] < sv_gravity.value * -0.1;

			SV_CheckVelocity(ent);
			SV_FlyMove(ent, sv.frametime, true, NULL, SV_GenericHitSuperContentsMask(ent), 0, 0, 0);
			SV_LinkEdict(ent);
			SV_LinkEdict_TouchAreaGrid(ent);

			// just hit ground
			if (hitsound && (int)PRVM_serveredictfloat(ent, flags) & FL_ONGROUND)
			{
				// DRESK - Check for Entity Land Event Function
				if(PRVM_serveredictfunction(ent, movetypesteplandevent))
				{ // Valid Function; Execute
					// Prepare Parameters
					// Assign Velocity at Impact
					PRVM_G_VECTOR(OFS_PARM0)[0] = backupVelocity[0];
					PRVM_G_VECTOR(OFS_PARM0)[1] = backupVelocity[1];
					PRVM_G_VECTOR(OFS_PARM0)[2] = backupVelocity[2];
					// Assign Self
					PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(ent);
					// Set Time
					PRVM_serverglobalfloat(time) = sv.time;
					// Execute VM Function
					prog->ExecuteProgram(prog, PRVM_serveredictfunction(ent, movetypesteplandevent), "movetypesteplandevent: NULL function");
				}
				else
				// Check for Engine Landing Sound
				if(sv_sound_land.string)
					SV_StartSound(ent, 0, sv_sound_land.string, 255, 1, false, 1.0f);
			}
			ent->priv.server->waterposition_forceupdate = true;
		}
	}
}

//============================================================================

static void SV_Physics_Entity (prvm_edict_t *ent)
{
	prvm_prog_t *prog = SVVM_prog;
	// don't run think/move on newly spawned projectiles as it messes up
	// movement interpolation and rocket trails, and is inconsistent with
	// respect to entities spawned in the same frame
	// (if an ent spawns a higher numbered ent, it moves in the same frame,
	//  but if it spawns a lower numbered ent, it doesn't - this never moves
	//  ents in the first frame regardless)
	qbool runmove = ent->priv.server->move;
	ent->priv.server->move = true;
	if (!runmove && sv_gameplayfix_delayprojectiles.integer > 0)
		return;
	switch ((int) PRVM_serveredictfloat(ent, movetype))
	{
	case MOVETYPE_PUSH:
	case MOVETYPE_FAKEPUSH:
		SV_Physics_Pusher (ent);
		break;
	case MOVETYPE_NONE:
		// LadyHavoc: manually inlined the thinktime check here because MOVETYPE_NONE is used on so many objects
		if (PRVM_serveredictfloat(ent, nextthink) > 0 && PRVM_serveredictfloat(ent, nextthink) <= sv.time + sv.frametime)
			SV_RunThink (ent);
		break;
	case MOVETYPE_FOLLOW:
		if(SV_RunThink(ent))
			SV_Physics_Follow (ent);
		break;
	case MOVETYPE_NOCLIP:
		if (SV_RunThink(ent))
		{
			SV_CheckWater(ent);
			VectorMA(PRVM_serveredictvector(ent, origin), sv.frametime, PRVM_serveredictvector(ent, velocity), PRVM_serveredictvector(ent, origin));
			VectorMA(PRVM_serveredictvector(ent, angles), sv.frametime, PRVM_serveredictvector(ent, avelocity), PRVM_serveredictvector(ent, angles));
		}
		SV_LinkEdict(ent);
		break;
	case MOVETYPE_STEP:
		SV_Physics_Step (ent);
		// regular thinking
		if (SV_RunThink(ent))
		if (ent->priv.server->waterposition_forceupdate || !VectorCompare(PRVM_serveredictvector(ent, origin), ent->priv.server->waterposition_origin))
		{
			ent->priv.server->waterposition_forceupdate = false;
			VectorCopy(PRVM_serveredictvector(ent, origin), ent->priv.server->waterposition_origin);
			SV_CheckWaterTransition(ent);
		}
		break;
	case MOVETYPE_WALK:
		if (SV_RunThink (ent))
			SV_WalkMove (ent);
		break;
	case MOVETYPE_TOSS:
	case MOVETYPE_BOUNCE:
	case MOVETYPE_BOUNCEMISSILE:
	case MOVETYPE_FLYMISSILE:
	case MOVETYPE_FLY:
	case MOVETYPE_FLY_WORLDONLY:
		// regular thinking
		if (SV_RunThink (ent))
			SV_Physics_Toss (ent);
		break;
	case MOVETYPE_PHYSICS:
		if (SV_RunThink(ent))
		{
			SV_LinkEdict(ent);
			SV_LinkEdict_TouchAreaGrid(ent);
		}
		break;
	default:
		if((int) PRVM_serveredictfloat(ent, movetype) >= MOVETYPE_USER_FIRST && (int) PRVM_serveredictfloat(ent, movetype) <= MOVETYPE_USER_LAST)
			break;
		Con_Printf ("SV_Physics: bad movetype %i\n", (int)PRVM_serveredictfloat(ent, movetype));
		break;
	}
}

static void SV_Physics_ClientEntity_NoThink (prvm_edict_t *ent)
{
	prvm_prog_t *prog = SVVM_prog;

	// don't run think at all, that is done during server frames
	// instead, call the movetypes directly so they match client input

	// This probably only makes sense for CSQC-networked (SendEntity field set) player entities
	switch ((int) PRVM_serveredictfloat(ent, movetype))
	{
	case MOVETYPE_PUSH:
	case MOVETYPE_FAKEPUSH:
		// push physics relies heavily on think times and calls, and so cannot be predicted currently
		Con_Printf ("SV_Physics_ClientEntity_NoThink: bad movetype %i\n", (int)PRVM_serveredictfloat(ent, movetype));
		break;
	case MOVETYPE_NONE:
		break;
	case MOVETYPE_FOLLOW:
		SV_Physics_Follow (ent);
		break;
	case MOVETYPE_NOCLIP:
		VectorMA(PRVM_serveredictvector(ent, origin), sv.frametime, PRVM_serveredictvector(ent, velocity), PRVM_serveredictvector(ent, origin));
		VectorMA(PRVM_serveredictvector(ent, angles), sv.frametime, PRVM_serveredictvector(ent, avelocity), PRVM_serveredictvector(ent, angles));
		break;
	case MOVETYPE_STEP:
		SV_Physics_Step (ent);
		break;
	case MOVETYPE_WALK:
		SV_WalkMove (ent);
		break;
	case MOVETYPE_TOSS:
	case MOVETYPE_BOUNCE:
	case MOVETYPE_BOUNCEMISSILE:
	case MOVETYPE_FLYMISSILE:
		SV_Physics_Toss (ent);
		break;
	case MOVETYPE_FLY:
	case MOVETYPE_FLY_WORLDONLY:
		SV_WalkMove (ent);
		break;
	case MOVETYPE_PHYSICS:
		break;
	default:
		if((int) PRVM_serveredictfloat(ent, movetype) >= MOVETYPE_USER_FIRST && (int) PRVM_serveredictfloat(ent, movetype) <= MOVETYPE_USER_LAST)
			break;
		Con_Printf ("SV_Physics_ClientEntity_NoThink: bad movetype %i\n", (int)PRVM_serveredictfloat(ent, movetype));
		break;
	}
}

// asynchronous path
void SV_Physics_ClientMove(void)
{
	prvm_prog_t *prog = SVVM_prog;
	prvm_edict_t *ent;
	ent = host_client->edict;

	// call player physics, this needs the proper frametime
	PRVM_serverglobalfloat(frametime) = sv.frametime;
	SV_PlayerPhysics();

	// call standard client pre-think, with frametime = 0
	PRVM_serverglobalfloat(time) = sv.time;
	PRVM_serverglobalfloat(frametime) = 0;
	PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(ent);
	prog->ExecuteProgram(prog, PRVM_serverfunction(PlayerPreThink), "QC function PlayerPreThink is missing");
	PRVM_serverglobalfloat(frametime) = sv.frametime;

	// make sure the velocity is sane (not a NaN)
	SV_CheckVelocity(ent);

	// perform movetype behaviour
	// note: will always be MOVETYPE_WALK if disableclientprediction = 0
	SV_Physics_ClientEntity_NoThink (ent);

	// call standard player post-think, with frametime = 0
	PRVM_serverglobalfloat(time) = sv.time;
	PRVM_serverglobalfloat(frametime) = 0;
	PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(ent);
	prog->ExecuteProgram(prog, PRVM_serverfunction(PlayerPostThink), "QC function PlayerPostThink is missing");
	PRVM_serverglobalfloat(frametime) = sv.frametime;

	if(PRVM_serveredictfloat(ent, fixangle))
	{
		// angle fixing was requested by physics code...
		// so store the current angles for later use
		VectorCopy(PRVM_serveredictvector(ent, angles), host_client->fixangle_angles);
		host_client->fixangle_angles_set = true;

		// and clear fixangle for the next frame
		PRVM_serveredictfloat(ent, fixangle) = 0;
	}
}

static void SV_Physics_ClientEntity_PreThink(prvm_edict_t *ent)
{
	prvm_prog_t *prog = SVVM_prog;
	// don't do physics on disconnected clients, FrikBot relies on this
	if (!host_client->begun)
		return;

	// make sure the velocity is sane (not a NaN)
	SV_CheckVelocity(ent);

	// don't run physics here if running asynchronously
	if (host_client->clmovement_inputtimeout <= 0)
	{
		SV_PlayerPhysics();
		//host_client->cmd.time = max(host_client->cmd.time, sv.time);
	}

	// make sure the velocity is still sane (not a NaN)
	SV_CheckVelocity(ent);

	// call standard client pre-think
	PRVM_serverglobalfloat(time) = sv.time;
	PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(ent);
	prog->ExecuteProgram(prog, PRVM_serverfunction(PlayerPreThink), "QC function PlayerPreThink is missing");

	// make sure the velocity is still sane (not a NaN)
	SV_CheckVelocity(ent);
}

static void SV_Physics_ClientEntity_PostThink(prvm_edict_t *ent)
{
	prvm_prog_t *prog = SVVM_prog;
	// don't do physics on disconnected clients, FrikBot relies on this
	if (!host_client->begun)
		return;

	// make sure the velocity is sane (not a NaN)
	SV_CheckVelocity(ent);

	// call standard player post-think
	PRVM_serverglobalfloat(time) = sv.time;
	PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(ent);
	prog->ExecuteProgram(prog, PRVM_serverfunction(PlayerPostThink), "QC function PlayerPostThink is missing");

	// make sure the velocity is still sane (not a NaN)
	SV_CheckVelocity(ent);

	if(PRVM_serveredictfloat(ent, fixangle))
	{
		// angle fixing was requested by physics code...
		// so store the current angles for later use
		VectorCopy(PRVM_serveredictvector(ent, angles), host_client->fixangle_angles);
		host_client->fixangle_angles_set = true;

		// and clear fixangle for the next frame
		PRVM_serveredictfloat(ent, fixangle) = 0;
	}

	// decrement the countdown variable used to decide when to go back to
	// synchronous physics
	if (host_client->clmovement_inputtimeout > sv.frametime)
		host_client->clmovement_inputtimeout -= sv.frametime;
	else
		host_client->clmovement_inputtimeout = 0;
}

static void SV_Physics_ClientEntity(prvm_edict_t *ent)
{
	prvm_prog_t *prog = SVVM_prog;
	// don't do physics on disconnected clients, FrikBot relies on this
	if (!host_client->begun)
	{
		memset(&host_client->cmd, 0, sizeof(host_client->cmd));
		return;
	}

	// make sure the velocity is sane (not a NaN)
	SV_CheckVelocity(ent);

	switch ((int) PRVM_serveredictfloat(ent, movetype))
	{
	case MOVETYPE_PUSH:
	case MOVETYPE_FAKEPUSH:
		SV_Physics_Pusher (ent);
		break;
	case MOVETYPE_NONE:
		// LadyHavoc: manually inlined the thinktime check here because MOVETYPE_NONE is used on so many objects
		if (PRVM_serveredictfloat(ent, nextthink) > 0 && PRVM_serveredictfloat(ent, nextthink) <= sv.time + sv.frametime)
			SV_RunThink (ent);
		break;
	case MOVETYPE_FOLLOW:
		SV_RunThink (ent);
		if (host_client->clmovement_inputtimeout <= 0) // don't run physics here if running asynchronously
			SV_Physics_Follow (ent);
		break;
	case MOVETYPE_NOCLIP:
		SV_RunThink(ent);
		if (host_client->clmovement_inputtimeout <= 0) // don't run physics here if running asynchronously
		{
			SV_CheckWater(ent);
			VectorMA(PRVM_serveredictvector(ent, origin), sv.frametime, PRVM_serveredictvector(ent, velocity), PRVM_serveredictvector(ent, origin));
			VectorMA(PRVM_serveredictvector(ent, angles), sv.frametime, PRVM_serveredictvector(ent, avelocity), PRVM_serveredictvector(ent, angles));
		}
		break;
	case MOVETYPE_STEP:
		if (host_client->clmovement_inputtimeout <= 0) // don't run physics here if running asynchronously
			SV_Physics_Step (ent);
		if (SV_RunThink(ent))
		if (ent->priv.server->waterposition_forceupdate || !VectorCompare(PRVM_serveredictvector(ent, origin), ent->priv.server->waterposition_origin))
		{
			ent->priv.server->waterposition_forceupdate = false;
			VectorCopy(PRVM_serveredictvector(ent, origin), ent->priv.server->waterposition_origin);
			SV_CheckWaterTransition(ent);
		}
		break;
	case MOVETYPE_WALK:
		SV_RunThink (ent);
		// don't run physics here if running asynchronously
		if (host_client->clmovement_inputtimeout <= 0)
			SV_WalkMove (ent);
		break;
	case MOVETYPE_TOSS:
	case MOVETYPE_BOUNCE:
	case MOVETYPE_BOUNCEMISSILE:
	case MOVETYPE_FLYMISSILE:
		// regular thinking
		SV_RunThink (ent);
		if (host_client->clmovement_inputtimeout <= 0) // don't run physics here if running asynchronously
			SV_Physics_Toss (ent);
		break;
	case MOVETYPE_FLY:
	case MOVETYPE_FLY_WORLDONLY:
		SV_RunThink (ent);
		if (host_client->clmovement_inputtimeout <= 0) // don't run physics here if running asynchronously
			SV_WalkMove (ent);
		break;
	case MOVETYPE_PHYSICS:
		SV_RunThink (ent);
		break;
	default:
		if((int) PRVM_serveredictfloat(ent, movetype) >= MOVETYPE_USER_FIRST && (int) PRVM_serveredictfloat(ent, movetype) <= MOVETYPE_USER_LAST)
			break;
		Con_Printf ("SV_Physics_ClientEntity: bad movetype %i\n", (int)PRVM_serveredictfloat(ent, movetype));
		break;
	}

	SV_CheckVelocity (ent);

	SV_LinkEdict(ent);
	SV_LinkEdict_TouchAreaGrid(ent);

	SV_CheckVelocity (ent);
}

/*
================
SV_Physics

================
*/
void SV_Physics (void)
{
	prvm_prog_t *prog = SVVM_prog;
	int i;
	prvm_edict_t *ent;

	// free memory for resources that are no longer referenced
	PRVM_GarbageCollection(prog);

// let the progs know that a new frame has started
	PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(prog->edicts);
	PRVM_serverglobaledict(other) = PRVM_EDICT_TO_PROG(prog->edicts);
	PRVM_serverglobalfloat(time) = sv.time;
	PRVM_serverglobalfloat(frametime) = sv.frametime;
	prog->ExecuteProgram(prog, PRVM_serverfunction(StartFrame), "QC function StartFrame is missing");

#ifdef USEODE
	// run physics engine
	World_Physics_Frame(&sv.world, sv.frametime, sv_gravity.value);
#endif

//
// treat each object in turn
//

	// if force_retouch, relink all the entities
	if (PRVM_serverglobalfloat(force_retouch) > 0)
		for (i = 1, ent = PRVM_EDICT_NUM(i);i < prog->num_edicts;i++, ent = PRVM_NEXT_EDICT(ent))
			if (!ent->free)
				SV_LinkEdict_TouchAreaGrid(ent); // force retouch even for stationary

	if (sv_gameplayfix_consistentplayerprethink.integer)
	{
		// run physics on the client entities in 3 stages
		for (i = 1, ent = PRVM_EDICT_NUM(i), host_client = svs.clients;i <= svs.maxclients;i++, ent = PRVM_NEXT_EDICT(ent), host_client++)
			if (!ent->free)
				SV_Physics_ClientEntity_PreThink(ent);

		for (i = 1, ent = PRVM_EDICT_NUM(i), host_client = svs.clients;i <= svs.maxclients;i++, ent = PRVM_NEXT_EDICT(ent), host_client++)
			if (!ent->free)
				SV_Physics_ClientEntity(ent);

		for (i = 1, ent = PRVM_EDICT_NUM(i), host_client = svs.clients;i <= svs.maxclients;i++, ent = PRVM_NEXT_EDICT(ent), host_client++)
			if (!ent->free)
				SV_Physics_ClientEntity_PostThink(ent);
	}
	else
	{
		// run physics on the client entities
		for (i = 1, ent = PRVM_EDICT_NUM(i), host_client = svs.clients;i <= svs.maxclients;i++, ent = PRVM_NEXT_EDICT(ent), host_client++)
		{
			if (!ent->free)
			{
				SV_Physics_ClientEntity_PreThink(ent);
				SV_Physics_ClientEntity(ent);
				SV_Physics_ClientEntity_PostThink(ent);
			}
		}
	}

	// run physics on all the non-client entities
	if (!sv_freezenonclients.integer)
	{
		for (;i < prog->num_edicts;i++, ent = PRVM_NEXT_EDICT(ent))
			if (!ent->free)
				SV_Physics_Entity(ent);
		// make a second pass to see if any ents spawned this frame and make
		// sure they run their move/think
		if (sv_gameplayfix_delayprojectiles.integer < 0)
			for (i = svs.maxclients + 1, ent = PRVM_EDICT_NUM(i);i < prog->num_edicts;i++, ent = PRVM_NEXT_EDICT(ent))
				if (!ent->priv.server->move && !ent->free)
					SV_Physics_Entity(ent);
	}

	if (PRVM_serverglobalfloat(force_retouch) > 0)
		PRVM_serverglobalfloat(force_retouch) = max(0, PRVM_serverglobalfloat(force_retouch) - 1);

	// LadyHavoc: endframe support
	if (PRVM_serverfunction(EndFrame))
	{
		PRVM_serverglobaledict(self) = PRVM_EDICT_TO_PROG(prog->edicts);
		PRVM_serverglobaledict(other) = PRVM_EDICT_TO_PROG(prog->edicts);
		PRVM_serverglobalfloat(time) = sv.time;
		prog->ExecuteProgram(prog, PRVM_serverfunction(EndFrame), "QC function EndFrame is missing");
	}

	// decrement prog->num_edicts if the highest number entities died
	for (;PRVM_ED_CanAlloc(prog, PRVM_EDICT_NUM(prog->num_edicts - 1));prog->num_edicts--);

	if (!sv_freezenonclients.integer)
		sv.time += sv.frametime;
}
