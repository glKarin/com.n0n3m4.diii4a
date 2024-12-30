// for physics functions shared by the client and server

#include "phys.h"

#include "quakedef.h"
#include "cl_collision.h"


// TODO handle this in a nicer way...
static inline trace_t PHYS_TraceBox(prvm_prog_t *prog, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int type, prvm_edict_t *passedict, int hitsupercontentsmask, int skipsupercontentsmask, int skipmaterialflagsmask, float extend, qbool hitnetworkbrushmodels, qbool hitnetworkplayers, int *hitnetworkentity, qbool hitcsqcentities)
{
	return (prog == SVVM_prog)
		? SV_TraceBox(start, mins, maxs, end, type, passedict, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, extend)
		: CL_TraceBox(start, mins, maxs, end, type, passedict, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, extend, hitnetworkbrushmodels, hitnetworkplayers, hitnetworkentity, hitcsqcentities);
}

/*
============
PHYS_TestEntityPosition

returns true if the entity is in solid currently
============
*/
qbool PHYS_TestEntityPosition (prvm_prog_t *prog, prvm_edict_t *ent, vec3_t offset)
{
	int hitsupercontentsmask = SV_GenericHitSuperContentsMask(ent);
	int skipsupercontentsmask = 0;
	int skipmaterialflagsmask = 0;
	vec3_t org, entorigin, entmins, entmaxs;
	trace_t trace;

	VectorAdd(PRVM_serveredictvector(ent, origin), offset, org);
	VectorCopy(PRVM_serveredictvector(ent, origin), entorigin);
	VectorCopy(PRVM_serveredictvector(ent, mins), entmins);
	VectorCopy(PRVM_serveredictvector(ent, maxs), entmaxs);
	trace = PHYS_TraceBox(prog, org, entmins, entmaxs, entorigin, ((PRVM_serveredictfloat(ent, movetype) == MOVETYPE_FLY_WORLDONLY) ? MOVE_WORLDONLY : MOVE_NOMONSTERS), ent, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, collision_extendmovelength.value, true, false, NULL, false);
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
		trace = PHYS_TraceBox(prog, org, entmins, entmaxs, org, MOVE_NOMONSTERS, ent, hitsupercontentsmask, skipsupercontentsmask, skipmaterialflagsmask, collision_extendmovelength.value, true, false, NULL, false);
		if(trace.startsolid)
			Con_Printf("PHYS_TestEntityPosition: trace.endpos detected to be in solid. NOT using it.\n");
		else
			VectorCopy(org, PRVM_serveredictvector(ent, origin));
#endif
	}
	return false;
}

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

unstickresult_t PHYS_UnstickEntityReturnOffset (prvm_prog_t *prog, prvm_edict_t *ent, vec3_t offset)
{
	int i, maxunstick;

	// if not stuck in a bmodel, just return
	if (!PHYS_TestEntityPosition(prog, ent, vec3_origin))
		return UNSTICK_GOOD;

	for (i = 0;i < (int)(sizeof(unstickoffsets) / sizeof(unstickoffsets[0]));i += 3)
	{
		if (!PHYS_TestEntityPosition(prog, ent, unstickoffsets + i))
		{
			VectorCopy(unstickoffsets + i, offset);
			return UNSTICK_UNSTUCK;
		}
	}

	maxunstick = (int) ((PRVM_serveredictvector(ent, maxs)[2] - PRVM_serveredictvector(ent, mins)[2]) * 0.36);
	// magic number 0.36 allows unsticking by up to 17 units with the largest supported bbox

	for(i = 2; i <= maxunstick; ++i)
	{
		VectorClear(offset);
		offset[2] = -i;
		if (!PHYS_TestEntityPosition(prog, ent, offset))
			return UNSTICK_UNSTUCK;
		offset[2] = i;
		if (!PHYS_TestEntityPosition(prog, ent, offset))
			return UNSTICK_UNSTUCK;
	}

	return UNSTICK_STUCK;
}

unstickresult_t PHYS_NudgeOutOfSolid(prvm_prog_t *prog, prvm_edict_t *ent)
{
	int bump, pass;
	trace_t stucktrace;
	vec3_t testorigin, targetorigin;
	vec3_t stuckmins, stuckmaxs;
	vec_t separation;
	model_t *worldmodel;

	if (prog == SVVM_prog)
	{
		worldmodel = sv.worldmodel;
		separation = sv_gameplayfix_nudgeoutofsolid_separation.value;
	}
	else if (prog == CLVM_prog)
	{
		worldmodel = cl.worldmodel;
		separation = cl_gameplayfix_nudgeoutofsolid_separation.value;
	}
	else
		Sys_Error("PHYS_NudgeOutOfSolid: cannot be called from %s VM\n", prog->name);

	VectorCopy(PRVM_serveredictvector(ent, mins), stuckmins);
	VectorCopy(PRVM_serveredictvector(ent, maxs), stuckmaxs);
	if (worldmodel && worldmodel->TraceBox != Mod_CollisionBIH_TraceBox)
	{
		separation = 0.0f; // when using hulls, it can not be enlarged

		// FIXME: Mod_Q1BSP_TraceBox() doesn't support startdepth and startdepthnormal
		return PHYS_UnstickEntityReturnOffset(prog, ent, testorigin); // fallback
	}
	else
	{
		stuckmins[0] -= separation;
		stuckmins[1] -= separation;
		stuckmins[2] -= separation;
		stuckmaxs[0] += separation;
		stuckmaxs[1] += separation;
		stuckmaxs[2] += separation;
	}

	// first pass we try to get it out of brush entities
	// second pass we try to get it out of world only (can't win them all)
	for (pass = 0;pass < 2;pass++)
	{
		VectorCopy(PRVM_serveredictvector(ent, origin), testorigin);
		for (bump = 0;bump < 10;bump++)
		{
			stucktrace = PHYS_TraceBox(prog, testorigin, stuckmins, stuckmaxs, testorigin, pass ? MOVE_WORLDONLY : MOVE_NOMONSTERS, ent, SV_GenericHitSuperContentsMask(ent), 0, 0, collision_extendmovelength.value, pass ? false : true, false, NULL, false);

			// Separation compared here to ensure a good location will be recognised reliably.
			if (-stucktrace.startdepth <= separation
			|| (!stucktrace.bmodelstartsolid && !stucktrace.worldstartsolid)
			|| (pass && !stucktrace.worldstartsolid))
			{
				// found a good location, use it
				VectorCopy(testorigin, PRVM_serveredictvector(ent, origin));
				return bump || pass ? UNSTICK_UNSTUCK : UNSTICK_GOOD;
			}

			VectorMA(testorigin, -stucktrace.startdepth, stucktrace.startdepthnormal, targetorigin);
			// Trace to targetorigin so we don't set it out of the world in complex cases.
			stucktrace = PHYS_TraceBox(prog, testorigin, stuckmins, stuckmaxs, targetorigin, pass ? MOVE_WORLDONLY : MOVE_NOMONSTERS, ent, SV_GenericHitSuperContentsMask(ent), 0, 0, collision_extendmovelength.value, pass ? false : true, false, NULL, false);
			if (stucktrace.fraction)
				VectorCopy(stucktrace.endpos, testorigin);
			else
				break; // Can't move it so no point doing more iterations on this pass.
		}
	}
	return UNSTICK_STUCK;
}
