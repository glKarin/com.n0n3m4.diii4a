
#include "quakedef.h"
#include "cl_collision.h"

float CL_SelectTraceLine(const vec3_t start, const vec3_t end, vec3_t impact, vec3_t normal, int *hitent, entity_render_t *ignoreent)
{
	float maxfrac;
	int n;
	entity_render_t *ent;
	vec_t tracemins[3], tracemaxs[3];
	trace_t trace;
	vec_t tempnormal[3], starttransformed[3], endtransformed[3];

	memset (&trace, 0 , sizeof(trace_t));
	trace.fraction = 1;
	VectorCopy (end, trace.endpos);

	if (hitent)
		*hitent = 0;
	if (cl.worldmodel && cl.worldmodel->TraceLine)
		cl.worldmodel->TraceLine(cl.worldmodel, NULL, NULL, &trace, start, end, SUPERCONTENTS_SOLID, 0, 0);

	if (normal)
		VectorCopy(trace.plane.normal, normal);
	maxfrac = trace.fraction;

	tracemins[0] = min(start[0], end[0]);
	tracemaxs[0] = max(start[0], end[0]);
	tracemins[1] = min(start[1], end[1]);
	tracemaxs[1] = max(start[1], end[1]);
	tracemins[2] = min(start[2], end[2]);
	tracemaxs[2] = max(start[2], end[2]);

	// look for embedded bmodels
	for (n = 0;n < cl.num_entities;n++)
	{
		if (!cl.entities_active[n])
			continue;
		ent = &cl.entities[n].render;
		if (!BoxesOverlap(ent->mins, ent->maxs, tracemins, tracemaxs))
			continue;
		if (!ent->model || !ent->model->TraceLine)
			continue;
		if ((ent->flags & RENDER_EXTERIORMODEL) && !chase_active.integer)
			continue;
		// if transparent and not selectable, skip entity
		if (!(cl.entities[n].state_current.effects & EF_SELECTABLE) && (ent->alpha < 1 || (ent->effects & (EF_ADDITIVE | EF_NODEPTHTEST))))
			continue;
		if (ent == ignoreent)
			continue;
		Matrix4x4_Transform(&ent->inversematrix, start, starttransformed);
		Matrix4x4_Transform(&ent->inversematrix, end, endtransformed);
		Collision_ClipTrace_Box(&trace, ent->model->normalmins, ent->model->normalmaxs, starttransformed, vec3_origin, vec3_origin, endtransformed, SUPERCONTENTS_SOLID, 0, 0, SUPERCONTENTS_SOLID, 0, NULL);
		if (maxfrac < trace.fraction)
			continue;

		ent->model->TraceLine(ent->model, ent->frameblend, ent->skeleton, &trace, starttransformed, endtransformed, SUPERCONTENTS_SOLID, 0, 0);

		if (maxfrac > trace.fraction)
		{
			if (hitent)
				*hitent = n;
			maxfrac = trace.fraction;
			if (normal)
			{
				VectorCopy(trace.plane.normal, tempnormal);
				Matrix4x4_Transform3x3(&ent->matrix, tempnormal, normal);
			}
		}
	}
	maxfrac = bound(0, maxfrac, 1);
	//maxrealfrac = bound(0, maxrealfrac, 1);
	//if (maxfrac < 0 || maxfrac > 1) Con_Printf("fraction out of bounds %f %s:%d\n", maxfrac, __FILE__, __LINE__);
	if (impact)
		VectorLerp(start, maxfrac, end, impact);
	return maxfrac;
}

void CL_FindNonSolidLocation(const vec3_t in, vec3_t out, vec_t radius)
{
	// FIXME: check multiple brush models
	if (cl.worldmodel && cl.worldmodel->brush.FindNonSolidLocation)
		cl.worldmodel->brush.FindNonSolidLocation(cl.worldmodel, in, out, radius);
}

model_t *CL_GetModelByIndex(int modelindex)
{
	if(!modelindex)
		return NULL;
	if (modelindex < 0)
	{
		modelindex = -(modelindex+1);
		if (modelindex < MAX_MODELS)
			return cl.csqc_model_precache[modelindex];
	}
	else
	{
		if(modelindex < MAX_MODELS)
			return cl.model_precache[modelindex];
	}
	return NULL;
}

model_t *CL_GetModelFromEdict(prvm_edict_t *ed)
{
	prvm_prog_t *prog = CLVM_prog;
	if (!ed || ed->free)
		return NULL;
	return CL_GetModelByIndex((int)PRVM_clientedictfloat(ed, modelindex));
}

void CL_LinkEdict(prvm_edict_t *ent)
{
	prvm_prog_t *prog = CLVM_prog;
	vec3_t mins, maxs;

	if (ent == prog->edicts)
		return;		// don't add the world

	if (ent->free)
		return;

	// set the abs box

	if (PRVM_clientedictfloat(ent, solid) == SOLID_BSP)
	{
		model_t *model = CL_GetModelByIndex( (int)PRVM_clientedictfloat(ent, modelindex) );
		if (model == NULL)
		{
			Con_Printf("edict %i: SOLID_BSP with invalid modelindex!\n", PRVM_NUM_FOR_EDICT(ent));

			model = CL_GetModelByIndex( 0 );
		}

		if( model != NULL )
		{
			if (!model->TraceBox)
				Con_DPrintf("edict %i: SOLID_BSP with non-collidable model\n", PRVM_NUM_FOR_EDICT(ent));

			if (PRVM_clientedictvector(ent, angles)[0] || PRVM_clientedictvector(ent, angles)[2] || PRVM_clientedictvector(ent, avelocity)[0] || PRVM_clientedictvector(ent, avelocity)[2])
			{
				VectorAdd(PRVM_clientedictvector(ent, origin), model->rotatedmins, mins);
				VectorAdd(PRVM_clientedictvector(ent, origin), model->rotatedmaxs, maxs);
			}
			else if (PRVM_clientedictvector(ent, angles)[1] || PRVM_clientedictvector(ent, avelocity)[1])
			{
				VectorAdd(PRVM_clientedictvector(ent, origin), model->yawmins, mins);
				VectorAdd(PRVM_clientedictvector(ent, origin), model->yawmaxs, maxs);
			}
			else
			{
				VectorAdd(PRVM_clientedictvector(ent, origin), model->normalmins, mins);
				VectorAdd(PRVM_clientedictvector(ent, origin), model->normalmaxs, maxs);
			}
		}
		else
		{
			// SOLID_BSP with no model is valid, mainly because some QC setup code does so temporarily
			VectorAdd(PRVM_clientedictvector(ent, origin), PRVM_clientedictvector(ent, mins), mins);
			VectorAdd(PRVM_clientedictvector(ent, origin), PRVM_clientedictvector(ent, maxs), maxs);
		}
	}
	else
	{
		VectorAdd(PRVM_clientedictvector(ent, origin), PRVM_clientedictvector(ent, mins), mins);
		VectorAdd(PRVM_clientedictvector(ent, origin), PRVM_clientedictvector(ent, maxs), maxs);
	}

	VectorCopy(mins, PRVM_clientedictvector(ent, absmin));
	VectorCopy(maxs, PRVM_clientedictvector(ent, absmax));

	World_LinkEdict(&cl.world, ent, mins, maxs, cl_areagrid_link_SOLID_NOT.integer);
}

int CL_GenericHitSuperContentsMask(const prvm_edict_t *passedict)
{
	prvm_prog_t *prog = CLVM_prog;
	if (passedict)
	{
		int dphitcontentsmask = (int)PRVM_clientedictfloat(passedict, dphitcontentsmask);
		if (dphitcontentsmask)
			return dphitcontentsmask;
		else if (PRVM_clientedictfloat(passedict, solid) == SOLID_SLIDEBOX)
		{
			if ((int)PRVM_clientedictfloat(passedict, flags) & FL_MONSTER)
				return SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY | SUPERCONTENTS_MONSTERCLIP;
			else
				return SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY | SUPERCONTENTS_PLAYERCLIP;
		}
		else if (PRVM_clientedictfloat(passedict, solid) == SOLID_CORPSE)
			return SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY;
		else if (PRVM_clientedictfloat(passedict, solid) == SOLID_TRIGGER)
			return SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY;
		else
			return SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY | SUPERCONTENTS_CORPSE;
	}
	else
		return SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY | SUPERCONTENTS_CORPSE;
}

/*
==================
CL_Move
==================
*/
trace_t CL_TracePoint(const vec3_t start, int type, prvm_edict_t *passedict, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask, qbool hitnetworkbrushmodels, qbool hitnetworkplayers, int *hitnetworkentity, qbool hitcsqcentities)
{
	prvm_prog_t *prog = CLVM_prog;
	int i, bodysupercontents;
	int passedictprog;
	prvm_edict_t *traceowner, *touch;
	trace_t trace;
	// temporary storage because prvm_vec_t may need conversion
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

	if (hitnetworkentity)
		*hitnetworkentity = 0;

	VectorCopy(start, clipstart);
	VectorClear(clipmins2);
	VectorClear(clipmaxs2);
#if COLLISIONPARANOID >= 3
	Con_Printf("move(%f %f %f)", clipstart[0], clipstart[1], clipstart[2]);
#endif

	// clip to world
	Collision_ClipPointToWorld(&cliptrace, cl.worldmodel, clipstart, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask);
	cliptrace.worldstartsolid = cliptrace.bmodelstartsolid = cliptrace.startsolid;
	if (cliptrace.startsolid || cliptrace.fraction < 1)
		cliptrace.ent = prog ? prog->edicts : NULL;
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
		clipboxmins[i] = clipstart[i] - 1;
		clipboxmaxs[i] = clipstart[i] + 1;
	}

	// debug override to test against everything
	if (sv_debugmove.integer)
	{
		clipboxmins[0] = clipboxmins[1] = clipboxmins[2] = (vec_t)-999999999;
		clipboxmaxs[0] = clipboxmaxs[1] = clipboxmaxs[2] =  (vec_t)999999999;
	}

	// if the passedict is world, make it NULL (to avoid two checks each time)
	// this checks prog because this function is often called without a CSQC
	// VM context
	if (prog == NULL || passedict == prog->edicts)
		passedict = NULL;
	// precalculate prog value for passedict for comparisons
	passedictprog = prog != NULL ? PRVM_EDICT_TO_PROG(passedict) : 0;
	// precalculate passedict's owner edict pointer for comparisons
	traceowner = passedict ? PRVM_PROG_TO_EDICT(PRVM_clientedictedict(passedict, owner)) : NULL;

	clipgroup = passedict ? (int)PRVM_clientedictfloat(passedict, clipgroup) : 0;

	// collide against network entities
	if (hitnetworkbrushmodels)
	{
		for (i = 0;i < cl.num_brushmodel_entities;i++)
		{
			entity_render_t *ent = &cl.entities[cl.brushmodel_entities[i]].render;
			if (!BoxesOverlap(clipboxmins, clipboxmaxs, ent->mins, ent->maxs))
				continue;
			Collision_ClipPointToGenericEntity(&trace, ent->model, ent->frameblend, ent->skeleton, vec3_origin, vec3_origin, 0, &ent->matrix, &ent->inversematrix, start, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask);
			if (cliptrace.fraction > trace.fraction && hitnetworkentity)
				*hitnetworkentity = cl.brushmodel_entities[i];
			Collision_CombineTraces(&cliptrace, &trace, NULL, true);
		}
	}

	// collide against player entities
	if (hitnetworkplayers)
	{
		vec3_t origin, entmins, entmaxs;
		matrix4x4_t entmatrix, entinversematrix;

		if(IS_OLDNEXUIZ_DERIVED(gamemode))
		{
			// don't hit network players, if we are a nonsolid player
			if(cl.scores[cl.playerentity-1].frags == -666 || cl.scores[cl.playerentity-1].frags == -616)
				goto skipnetworkplayers;
		}

		for (i = 1;i <= cl.maxclients;i++)
		{
			entity_render_t *ent = &cl.entities[i].render;

			// don't hit ourselves
			if (i == cl.playerentity)
				continue;

			// don't hit players that don't exist
			if (!cl.entities_active[i])
				continue;
			if (!cl.scores[i-1].name[0])
				continue;

			if(IS_OLDNEXUIZ_DERIVED(gamemode))
			{
				// don't hit spectators or nonsolid players
				if(cl.scores[i-1].frags == -666 || cl.scores[i-1].frags == -616)
					continue;
			}

			Matrix4x4_OriginFromMatrix(&ent->matrix, origin);
			VectorAdd(origin, cl.playerstandmins, entmins);
			VectorAdd(origin, cl.playerstandmaxs, entmaxs);
			if (!BoxesOverlap(clipboxmins, clipboxmaxs, entmins, entmaxs))
				continue;
			Matrix4x4_CreateTranslate(&entmatrix, origin[0], origin[1], origin[2]);
			Matrix4x4_CreateTranslate(&entinversematrix, -origin[0], -origin[1], -origin[2]);
			Collision_ClipPointToGenericEntity(&trace, NULL, NULL, NULL, cl.playerstandmins, cl.playerstandmaxs, SUPERCONTENTS_BODY, &entmatrix, &entinversematrix, start, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask);
			if (cliptrace.fraction > trace.fraction && hitnetworkentity)
				*hitnetworkentity = i;
			Collision_CombineTraces(&cliptrace, &trace, NULL, false);
		}

skipnetworkplayers:
		;
	}

	// clip to entities
	// because this uses World_EntitiestoBox, we know all entity boxes overlap
	// the clip region, so we can skip culling checks in the loop below
	// note: if prog is NULL then there won't be any linked entities
	numtouchedicts = 0;
	if (hitcsqcentities && prog != NULL)
	{
		numtouchedicts = World_EntitiesInBox(&cl.world, clipboxmins, clipboxmaxs, MAX_EDICTS, touchedicts);
		if (numtouchedicts > MAX_EDICTS)
		{
			// this never happens
			Con_Printf("CL_EntitiesInBox returned %i edicts, max was %i\n", numtouchedicts, MAX_EDICTS);
			numtouchedicts = MAX_EDICTS;
		}
	}
	for (i = 0;i < numtouchedicts;i++)
	{
		touch = touchedicts[i];

		if (PRVM_clientedictfloat(touch, solid) < SOLID_BBOX)
			continue;
		if (type == MOVE_NOMONSTERS && PRVM_clientedictfloat(touch, solid) != SOLID_BSP)
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
			if (passedictprog == PRVM_clientedictedict(touch, owner))
				continue;
			// don't clip against any entities in the same clipgroup (DP_RM_CLIPGROUP)
			if (clipgroup && clipgroup == (int)PRVM_clientedictfloat(touch, clipgroup))
				continue;
			// don't clip points against points (they can't collide)
			if (VectorCompare(PRVM_clientedictvector(touch, mins), PRVM_clientedictvector(touch, maxs)) && (type != MOVE_MISSILE || !((int)PRVM_clientedictfloat(touch, flags) & FL_MONSTER)))
				continue;
		}

		bodysupercontents = PRVM_clientedictfloat(touch, solid) == SOLID_CORPSE ? SUPERCONTENTS_CORPSE : SUPERCONTENTS_BODY;

		// might interact, so do an exact clip
		model = NULL;
		if ((int) PRVM_clientedictfloat(touch, solid) == SOLID_BSP || type == MOVE_HITMODEL)
			model = CL_GetModelFromEdict(touch);
		if (model)
			Matrix4x4_CreateFromQuakeEntity(&matrix, PRVM_clientedictvector(touch, origin)[0], PRVM_clientedictvector(touch, origin)[1], PRVM_clientedictvector(touch, origin)[2], PRVM_clientedictvector(touch, angles)[0], PRVM_clientedictvector(touch, angles)[1], PRVM_clientedictvector(touch, angles)[2], 1);
		else
			Matrix4x4_CreateTranslate(&matrix, PRVM_clientedictvector(touch, origin)[0], PRVM_clientedictvector(touch, origin)[1], PRVM_clientedictvector(touch, origin)[2]);
		Matrix4x4_Invert_Simple(&imatrix, &matrix);
		VectorCopy(PRVM_clientedictvector(touch, mins), touchmins);
		VectorCopy(PRVM_clientedictvector(touch, maxs), touchmaxs);
		if ((int)PRVM_clientedictfloat(touch, flags) & FL_MONSTER)
			Collision_ClipToGenericEntity(&trace, model, touch->priv.server->frameblend, &touch->priv.server->skeleton, touchmins, touchmaxs, bodysupercontents, &matrix, &imatrix, clipstart, clipmins2, clipmaxs2, clipstart, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, 0.0f);
		else
			Collision_ClipPointToGenericEntity(&trace, model, touch->priv.server->frameblend, &touch->priv.server->skeleton, touchmins, touchmaxs, bodysupercontents, &matrix, &imatrix, clipstart, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask);

		if (cliptrace.fraction > trace.fraction && hitnetworkentity)
			*hitnetworkentity = 0;
		Collision_CombineTraces(&cliptrace, &trace, (void *)touch, PRVM_clientedictfloat(touch, solid) == SOLID_BSP);
	}

finished:
	return cliptrace;
}

/*
==================
CL_TraceLine
==================
*/
trace_t CL_TraceLine(const vec3_t start, const vec3_t end, int type, prvm_edict_t *passedict, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask, float extend, qbool hitnetworkbrushmodels, qbool hitnetworkplayers, int *hitnetworkentity, qbool hitcsqcentities, qbool hitsurfaces)
{
	prvm_prog_t *prog = CLVM_prog;
	int i, bodysupercontents;
	int passedictprog;
	prvm_edict_t *traceowner, *touch;
	trace_t trace;
	// temporary storage because prvm_vec_t may need conversion
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
		return CL_TracePoint(start, type, passedict, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, hitnetworkbrushmodels, hitnetworkplayers, hitnetworkentity, hitcsqcentities);

	if (hitnetworkentity)
		*hitnetworkentity = 0;

	VectorCopy(start, clipstart);
	VectorCopy(end, clipend);
	VectorClear(clipmins2);
	VectorClear(clipmaxs2);
#if COLLISIONPARANOID >= 3
	Con_Printf("move(%f %f %f,%f %f %f)", clipstart[0], clipstart[1], clipstart[2], clipend[0], clipend[1], clipend[2]);
#endif

	// clip to world
	Collision_ClipLineToWorld(&cliptrace, cl.worldmodel, clipstart, clipend, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, extend, hitsurfaces);
	cliptrace.worldstartsolid = cliptrace.bmodelstartsolid = cliptrace.startsolid;
	if (cliptrace.startsolid || cliptrace.fraction < 1)
		cliptrace.ent = prog ? prog->edicts : NULL;
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
	// this checks prog because this function is often called without a CSQC
	// VM context
	if (prog == NULL || passedict == prog->edicts)
		passedict = NULL;
	// precalculate prog value for passedict for comparisons
	passedictprog = prog != NULL ? PRVM_EDICT_TO_PROG(passedict) : 0;
	// precalculate passedict's owner edict pointer for comparisons
	traceowner = passedict ? PRVM_PROG_TO_EDICT(PRVM_clientedictedict(passedict, owner)) : NULL;

	clipgroup = passedict ? (int)PRVM_clientedictfloat(passedict, clipgroup) : 0;

	// collide against network entities
	if (hitnetworkbrushmodels)
	{
		for (i = 0;i < cl.num_brushmodel_entities;i++)
		{
			entity_render_t *ent = &cl.entities[cl.brushmodel_entities[i]].render;
			if (!BoxesOverlap(clipboxmins, clipboxmaxs, ent->mins, ent->maxs))
				continue;
			Collision_ClipLineToGenericEntity(&trace, ent->model, ent->frameblend, ent->skeleton, vec3_origin, vec3_origin, 0, &ent->matrix, &ent->inversematrix, start, end, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, extend, hitsurfaces);
			if (cliptrace.fraction > trace.fraction && hitnetworkentity)
				*hitnetworkentity = cl.brushmodel_entities[i];
			Collision_CombineTraces(&cliptrace, &trace, NULL, true);
		}
	}

	// collide against player entities
	if (hitnetworkplayers)
	{
		vec3_t origin, entmins, entmaxs;
		matrix4x4_t entmatrix, entinversematrix;

		if(IS_OLDNEXUIZ_DERIVED(gamemode))
		{
			// don't hit network players, if we are a nonsolid player
			if(cl.scores[cl.playerentity-1].frags == -666 || cl.scores[cl.playerentity-1].frags == -616)
				goto skipnetworkplayers;
		}

		for (i = 1;i <= cl.maxclients;i++)
		{
			entity_render_t *ent = &cl.entities[i].render;

			// don't hit ourselves
			if (i == cl.playerentity)
				continue;

			// don't hit players that don't exist
			if (!cl.entities_active[i])
				continue;
			if (!cl.scores[i-1].name[0])
				continue;

			if(IS_OLDNEXUIZ_DERIVED(gamemode))
			{
				// don't hit spectators or nonsolid players
				if(cl.scores[i-1].frags == -666 || cl.scores[i-1].frags == -616)
					continue;
			}

			Matrix4x4_OriginFromMatrix(&ent->matrix, origin);
			VectorAdd(origin, cl.playerstandmins, entmins);
			VectorAdd(origin, cl.playerstandmaxs, entmaxs);
			if (!BoxesOverlap(clipboxmins, clipboxmaxs, entmins, entmaxs))
				continue;
			Matrix4x4_CreateTranslate(&entmatrix, origin[0], origin[1], origin[2]);
			Matrix4x4_CreateTranslate(&entinversematrix, -origin[0], -origin[1], -origin[2]);
			Collision_ClipLineToGenericEntity(&trace, NULL, NULL, NULL, cl.playerstandmins, cl.playerstandmaxs, SUPERCONTENTS_BODY, &entmatrix, &entinversematrix, start, end, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, extend, hitsurfaces);
			if (cliptrace.fraction > trace.fraction && hitnetworkentity)
				*hitnetworkentity = i;
			Collision_CombineTraces(&cliptrace, &trace, NULL, false);
		}

skipnetworkplayers:
		;
	}

	// clip to entities
	// because this uses World_EntitiestoBox, we know all entity boxes overlap
	// the clip region, so we can skip culling checks in the loop below
	// note: if prog is NULL then there won't be any linked entities
	numtouchedicts = 0;
	if (hitcsqcentities && prog != NULL)
	{
		numtouchedicts = World_EntitiesInBox(&cl.world, clipboxmins, clipboxmaxs, MAX_EDICTS, touchedicts);
		if (numtouchedicts > MAX_EDICTS)
		{
			// this never happens
			Con_Printf("CL_EntitiesInBox returned %i edicts, max was %i\n", numtouchedicts, MAX_EDICTS);
			numtouchedicts = MAX_EDICTS;
		}
	}
	for (i = 0;i < numtouchedicts;i++)
	{
		touch = touchedicts[i];

		if (PRVM_clientedictfloat(touch, solid) < SOLID_BBOX)
			continue;
		if (type == MOVE_NOMONSTERS && PRVM_clientedictfloat(touch, solid) != SOLID_BSP)
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
			if (passedictprog == PRVM_clientedictedict(touch, owner))
				continue;
			// don't clip against any entities in the same clipgroup (DP_RM_CLIPGROUP)
			if (clipgroup && clipgroup == (int)PRVM_clientedictfloat(touch, clipgroup))
				continue;
			// don't clip points against points (they can't collide)
			if (VectorCompare(PRVM_clientedictvector(touch, mins), PRVM_clientedictvector(touch, maxs)) && (type != MOVE_MISSILE || !((int)PRVM_clientedictfloat(touch, flags) & FL_MONSTER)))
				continue;
		}

		bodysupercontents = PRVM_clientedictfloat(touch, solid) == SOLID_CORPSE ? SUPERCONTENTS_CORPSE : SUPERCONTENTS_BODY;

		// might interact, so do an exact clip
		model = NULL;
		if ((int) PRVM_clientedictfloat(touch, solid) == SOLID_BSP || type == MOVE_HITMODEL)
			model = CL_GetModelFromEdict(touch);
		if (model)
			Matrix4x4_CreateFromQuakeEntity(&matrix, PRVM_clientedictvector(touch, origin)[0], PRVM_clientedictvector(touch, origin)[1], PRVM_clientedictvector(touch, origin)[2], PRVM_clientedictvector(touch, angles)[0], PRVM_clientedictvector(touch, angles)[1], PRVM_clientedictvector(touch, angles)[2], 1);
		else
			Matrix4x4_CreateTranslate(&matrix, PRVM_clientedictvector(touch, origin)[0], PRVM_clientedictvector(touch, origin)[1], PRVM_clientedictvector(touch, origin)[2]);
		Matrix4x4_Invert_Simple(&imatrix, &matrix);
		VectorCopy(PRVM_clientedictvector(touch, mins), touchmins);
		VectorCopy(PRVM_clientedictvector(touch, maxs), touchmaxs);
		if (type == MOVE_MISSILE && (int)PRVM_clientedictfloat(touch, flags) & FL_MONSTER)
			Collision_ClipToGenericEntity(&trace, model, touch->priv.server->frameblend, &touch->priv.server->skeleton, touchmins, touchmaxs, bodysupercontents, &matrix, &imatrix, clipstart, clipmins2, clipmaxs2, clipend, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, extend);
		else
			Collision_ClipLineToGenericEntity(&trace, model, touch->priv.server->frameblend, &touch->priv.server->skeleton, touchmins, touchmaxs, bodysupercontents, &matrix, &imatrix, clipstart, clipend, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, extend, hitsurfaces);

		if (cliptrace.fraction > trace.fraction && hitnetworkentity)
			*hitnetworkentity = 0;
		Collision_CombineTraces(&cliptrace, &trace, (void *)touch, PRVM_clientedictfloat(touch, solid) == SOLID_BSP);
	}

finished:
	return cliptrace;
}

/*
==================
CL_Move
==================
*/
trace_t CL_TraceBox(const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int type, prvm_edict_t *passedict, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask, float extend, qbool hitnetworkbrushmodels, qbool hitnetworkplayers, int *hitnetworkentity, qbool hitcsqcentities)
{
	prvm_prog_t *prog = CLVM_prog;
	vec3_t hullmins, hullmaxs;
	int i, bodysupercontents;
	int passedictprog;
	qbool pointtrace;
	prvm_edict_t *traceowner, *touch;
	trace_t trace;
	// temporary storage because prvm_vec_t may need conversion
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
			trace = CL_TracePoint(shiftstart, type, passedict, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, hitnetworkbrushmodels, hitnetworkplayers, hitnetworkentity, hitcsqcentities);
		else
			trace = CL_TraceLine(shiftstart, shiftend, type, passedict, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, extend, hitnetworkbrushmodels, hitnetworkplayers, hitnetworkentity, hitcsqcentities, false);
		VectorSubtract(trace.endpos, mins, trace.endpos);
		return trace;
	}

	if (hitnetworkentity)
		*hitnetworkentity = 0;

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
	Collision_ClipToWorld(&cliptrace, cl.worldmodel, clipstart, clipmins, clipmaxs, clipend, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, extend);
	cliptrace.worldstartsolid = cliptrace.bmodelstartsolid = cliptrace.startsolid;
	if (cliptrace.startsolid || cliptrace.fraction < 1)
		cliptrace.ent = prog ? prog->edicts : NULL;
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
	if (cl.worldmodel && cl.worldmodel->brush.RoundUpToHullSize)
		cl.worldmodel->brush.RoundUpToHullSize(cl.worldmodel, clipmins, clipmaxs, hullmins, hullmaxs);
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
	// this checks prog because this function is often called without a CSQC
	// VM context
	if (prog == NULL || passedict == prog->edicts)
		passedict = NULL;
	// precalculate prog value for passedict for comparisons
	passedictprog = prog != NULL ? PRVM_EDICT_TO_PROG(passedict) : 0;
	// figure out whether this is a point trace for comparisons
	pointtrace = VectorCompare(clipmins, clipmaxs);
	// precalculate passedict's owner edict pointer for comparisons
	traceowner = passedict ? PRVM_PROG_TO_EDICT(PRVM_clientedictedict(passedict, owner)) : NULL;

	clipgroup = passedict ? (int)PRVM_clientedictfloat(passedict, clipgroup) : 0;

	// collide against network entities
	if (hitnetworkbrushmodels)
	{
		for (i = 0;i < cl.num_brushmodel_entities;i++)
		{
			entity_render_t *ent = &cl.entities[cl.brushmodel_entities[i]].render;
			if (!BoxesOverlap(clipboxmins, clipboxmaxs, ent->mins, ent->maxs))
				continue;
			Collision_ClipToGenericEntity(&trace, ent->model, ent->frameblend, ent->skeleton, vec3_origin, vec3_origin, 0, &ent->matrix, &ent->inversematrix, start, mins, maxs, end, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, extend);
			if (cliptrace.fraction > trace.fraction && hitnetworkentity)
				*hitnetworkentity = cl.brushmodel_entities[i];
			Collision_CombineTraces(&cliptrace, &trace, NULL, true);
		}
	}

	// collide against player entities
	if (hitnetworkplayers)
	{
		vec3_t origin, entmins, entmaxs;
		matrix4x4_t entmatrix, entinversematrix;

		if(IS_OLDNEXUIZ_DERIVED(gamemode))
		{
			// don't hit network players, if we are a nonsolid player
			if(cl.scores[cl.playerentity-1].frags == -666 || cl.scores[cl.playerentity-1].frags == -616)
				goto skipnetworkplayers;
		}

		for (i = 1;i <= cl.maxclients;i++)
		{
			entity_render_t *ent = &cl.entities[i].render;

			// don't hit ourselves
			if (i == cl.playerentity)
				continue;

			// don't hit players that don't exist
			if (!cl.entities_active[i])
				continue;
			if (!cl.scores[i-1].name[0])
				continue;

			if(IS_OLDNEXUIZ_DERIVED(gamemode))
			{
				// don't hit spectators or nonsolid players
				if(cl.scores[i-1].frags == -666 || cl.scores[i-1].frags == -616)
					continue;
			}

			Matrix4x4_OriginFromMatrix(&ent->matrix, origin);
			VectorAdd(origin, cl.playerstandmins, entmins);
			VectorAdd(origin, cl.playerstandmaxs, entmaxs);
			if (!BoxesOverlap(clipboxmins, clipboxmaxs, entmins, entmaxs))
				continue;
			Matrix4x4_CreateTranslate(&entmatrix, origin[0], origin[1], origin[2]);
			Matrix4x4_CreateTranslate(&entinversematrix, -origin[0], -origin[1], -origin[2]);
			Collision_ClipToGenericEntity(&trace, NULL, NULL, NULL, cl.playerstandmins, cl.playerstandmaxs, SUPERCONTENTS_BODY, &entmatrix, &entinversematrix, start, mins, maxs, end, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, extend);
			if (cliptrace.fraction > trace.fraction && hitnetworkentity)
				*hitnetworkentity = i;
			Collision_CombineTraces(&cliptrace, &trace, NULL, false);
		}

skipnetworkplayers:
		;
	}

	// clip to entities
	// because this uses World_EntitiestoBox, we know all entity boxes overlap
	// the clip region, so we can skip culling checks in the loop below
	// note: if prog is NULL then there won't be any linked entities
	numtouchedicts = 0;
	if (hitcsqcentities && prog != NULL)
	{
		numtouchedicts = World_EntitiesInBox(&cl.world, clipboxmins, clipboxmaxs, MAX_EDICTS, touchedicts);
		if (numtouchedicts > MAX_EDICTS)
		{
			// this never happens
			Con_Printf("CL_EntitiesInBox returned %i edicts, max was %i\n", numtouchedicts, MAX_EDICTS);
			numtouchedicts = MAX_EDICTS;
		}
	}
	for (i = 0;i < numtouchedicts;i++)
	{
		touch = touchedicts[i];

		if (PRVM_clientedictfloat(touch, solid) < SOLID_BBOX)
			continue;
		if (type == MOVE_NOMONSTERS && PRVM_clientedictfloat(touch, solid) != SOLID_BSP)
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
			if (passedictprog == PRVM_clientedictedict(touch, owner))
				continue;
			// don't clip against any entities in the same clipgroup (DP_RM_CLIPGROUP)
			if (clipgroup && clipgroup == (int)PRVM_clientedictfloat(touch, clipgroup))
				continue;
			// don't clip points against points (they can't collide)
			if (pointtrace && VectorCompare(PRVM_clientedictvector(touch, mins), PRVM_clientedictvector(touch, maxs)) && (type != MOVE_MISSILE || !((int)PRVM_clientedictfloat(touch, flags) & FL_MONSTER)))
				continue;
		}

		bodysupercontents = PRVM_clientedictfloat(touch, solid) == SOLID_CORPSE ? SUPERCONTENTS_CORPSE : SUPERCONTENTS_BODY;

		// might interact, so do an exact clip
		model = NULL;
		if ((int) PRVM_clientedictfloat(touch, solid) == SOLID_BSP || type == MOVE_HITMODEL)
			model = CL_GetModelFromEdict(touch);
		if (model)
			Matrix4x4_CreateFromQuakeEntity(&matrix, PRVM_clientedictvector(touch, origin)[0], PRVM_clientedictvector(touch, origin)[1], PRVM_clientedictvector(touch, origin)[2], PRVM_clientedictvector(touch, angles)[0], PRVM_clientedictvector(touch, angles)[1], PRVM_clientedictvector(touch, angles)[2], 1);
		else
			Matrix4x4_CreateTranslate(&matrix, PRVM_clientedictvector(touch, origin)[0], PRVM_clientedictvector(touch, origin)[1], PRVM_clientedictvector(touch, origin)[2]);
		Matrix4x4_Invert_Simple(&imatrix, &matrix);
		VectorCopy(PRVM_clientedictvector(touch, mins), touchmins);
		VectorCopy(PRVM_clientedictvector(touch, maxs), touchmaxs);
		if ((int)PRVM_clientedictfloat(touch, flags) & FL_MONSTER)
			Collision_ClipToGenericEntity(&trace, model, touch->priv.server->frameblend, &touch->priv.server->skeleton, touchmins, touchmaxs, bodysupercontents, &matrix, &imatrix, clipstart, clipmins2, clipmaxs2, clipend, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, extend);
		else
			Collision_ClipToGenericEntity(&trace, model, touch->priv.server->frameblend, &touch->priv.server->skeleton, touchmins, touchmaxs, bodysupercontents, &matrix, &imatrix, clipstart, clipmins, clipmaxs, clipend, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, extend);

		if (cliptrace.fraction > trace.fraction && hitnetworkentity)
			*hitnetworkentity = 0;
		Collision_CombineTraces(&cliptrace, &trace, (void *)touch, PRVM_clientedictfloat(touch, solid) == SOLID_BSP);
	}

finished:
	return cliptrace;
}

/*
==================
CL_Cache_TraceLine
==================
*/
trace_t CL_Cache_TraceLineSurfaces(const vec3_t start, const vec3_t end, int type, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask)
{
	prvm_prog_t *prog = CLVM_prog;
	int i;
	prvm_edict_t *touch;
	trace_t trace;
	// bounding box of entire move area
	vec3_t clipboxmins, clipboxmaxs;
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

	VectorCopy(start, clipstart);
	VectorCopy(end, clipend);
#if COLLISIONPARANOID >= 3
	Con_Printf("move(%f %f %f,%f %f %f)", clipstart[0], clipstart[1], clipstart[2], clipend[0], clipend[1], clipend[2]);
#endif

	// clip to world
	Collision_Cache_ClipLineToWorldSurfaces(&cliptrace, cl.worldmodel, clipstart, clipend, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask);
	cliptrace.worldstartsolid = cliptrace.bmodelstartsolid = cliptrace.startsolid;
	if (cliptrace.startsolid || cliptrace.fraction < 1)
		cliptrace.ent = prog ? prog->edicts : NULL;
	if (type == MOVE_WORLDONLY)
		goto finished;

	// create the bounding box of the entire move
	for (i = 0;i < 3;i++)
	{
		clipboxmins[i] = min(clipstart[i], cliptrace.endpos[i]) - 1;
		clipboxmaxs[i] = max(clipstart[i], cliptrace.endpos[i]) + 1;
	}

	// if the passedict is world, make it NULL (to avoid two checks each time)
	// this checks prog because this function is often called without a CSQC
	// VM context

	// collide against network entities
	for (i = 0;i < cl.num_brushmodel_entities;i++)
	{
		entity_render_t *ent = &cl.entities[cl.brushmodel_entities[i]].render;
		if (!BoxesOverlap(clipboxmins, clipboxmaxs, ent->mins, ent->maxs))
			continue;
		Collision_Cache_ClipLineToGenericEntitySurfaces(&trace, ent->model, &ent->matrix, &ent->inversematrix, start, end, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask);
		Collision_CombineTraces(&cliptrace, &trace, NULL, true);
	}

	// clip to entities
	// because this uses World_EntitiestoBox, we know all entity boxes overlap
	// the clip region, so we can skip culling checks in the loop below
	// note: if prog is NULL then there won't be any linked entities
	numtouchedicts = 0;
	if (prog != NULL)
	{
		numtouchedicts = World_EntitiesInBox(&cl.world, clipboxmins, clipboxmaxs, MAX_EDICTS, touchedicts);
		if (numtouchedicts > MAX_EDICTS)
		{
			// this never happens
			Con_Printf("CL_EntitiesInBox returned %i edicts, max was %i\n", numtouchedicts, MAX_EDICTS);
			numtouchedicts = MAX_EDICTS;
		}
	}
	for (i = 0;i < numtouchedicts;i++)
	{
		touch = touchedicts[i];
		// might interact, so do an exact clip
		// only hit entity models, not collision shapes
		model = CL_GetModelFromEdict(touch);
		if (!model)
			continue;
		// animated models are not suitable for caching
		if ((touch->priv.server->frameblend[0].lerp != 1.0 || touch->priv.server->frameblend[0].subframe != 0) || touch->priv.server->skeleton.relativetransforms)
			continue;
		if (type == MOVE_NOMONSTERS && PRVM_clientedictfloat(touch, solid) != SOLID_BSP)
			continue;
		Matrix4x4_CreateFromQuakeEntity(&matrix, PRVM_clientedictvector(touch, origin)[0], PRVM_clientedictvector(touch, origin)[1], PRVM_clientedictvector(touch, origin)[2], PRVM_clientedictvector(touch, angles)[0], PRVM_clientedictvector(touch, angles)[1], PRVM_clientedictvector(touch, angles)[2], 1);
		Matrix4x4_Invert_Simple(&imatrix, &matrix);
		Collision_Cache_ClipLineToGenericEntitySurfaces(&trace, model, &matrix, &imatrix, clipstart, clipend, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask);
		Collision_CombineTraces(&cliptrace, &trace, (void *)touch, PRVM_clientedictfloat(touch, solid) == SOLID_BSP);
	}

finished:
	return cliptrace;
}

