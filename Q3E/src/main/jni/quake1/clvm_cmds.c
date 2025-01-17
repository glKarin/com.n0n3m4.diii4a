#include "quakedef.h"

#include "prvm_cmds.h"
#include "csprogs.h"
#include "cl_collision.h"
#include "r_shadow.h"
#include "jpeg.h"
#include "image.h"

//============================================================================
// Client
//[515]: unsolved PROBLEMS
//- finish player physics code (cs_runplayerphysics)
//- EntWasFreed ?
//- RF_DEPTHHACK is not like it should be
//- add builtin that sets cl.viewangles instead of reading "input_angles" global
//- finish lines support for R_Polygon***
//- insert selecttraceline into traceline somehow

//4 feature darkplaces csqc: add builtin to clientside qc for reading triangles of model meshes (useful to orient a ui along a triangle of a model mesh)
//4 feature darkplaces csqc: add builtins to clientside qc for gl calls

extern cvar_t v_flipped;

r_refdef_view_t csqc_original_r_refdef_view;
r_refdef_view_t csqc_main_r_refdef_view;

// #1 void(vector ang) makevectors
static void VM_CL_makevectors (prvm_prog_t *prog)
{
	vec3_t angles, forward, right, up;
	VM_SAFEPARMCOUNT(1, VM_CL_makevectors);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), angles);
	AngleVectors(angles, forward, right, up);
	VectorCopy(forward, PRVM_clientglobalvector(v_forward));
	VectorCopy(right, PRVM_clientglobalvector(v_right));
	VectorCopy(up, PRVM_clientglobalvector(v_up));
}

// #2 void(entity e, vector o) setorigin
static void VM_CL_setorigin (prvm_prog_t *prog)
{
	prvm_edict_t	*e;
	prvm_vec_t	*org;
	VM_SAFEPARMCOUNT(2, VM_CL_setorigin);

	e = PRVM_G_EDICT(OFS_PARM0);
	if (e == prog->edicts)
	{
		VM_Warning(prog, "setorigin: can not modify world entity\n");
		return;
	}
	if (e->free)
	{
		VM_Warning(prog, "setorigin: can not modify free entity\n");
		return;
	}
	org = PRVM_G_VECTOR(OFS_PARM1);
	VectorCopy (org, PRVM_clientedictvector(e, origin));
	if(e->priv.required->mark == PRVM_EDICT_MARK_WAIT_FOR_SETORIGIN)
		e->priv.required->mark = PRVM_EDICT_MARK_SETORIGIN_CAUGHT;
	CL_LinkEdict(e);
}

static void SetMinMaxSizePRVM (prvm_prog_t *prog, prvm_edict_t *e, prvm_vec_t *min, prvm_vec_t *max)
{
	int		i;

	for (i=0 ; i<3 ; i++)
		if (min[i] > max[i])
			prog->error_cmd("SetMinMaxSize: backwards mins/maxs");

	// set derived values
	VectorCopy (min, PRVM_clientedictvector(e, mins));
	VectorCopy (max, PRVM_clientedictvector(e, maxs));
	VectorSubtract (max, min, PRVM_clientedictvector(e, size));

	CL_LinkEdict (e);
}

static void SetMinMaxSize (prvm_prog_t *prog, prvm_edict_t *e, const vec_t *min, const vec_t *max)
{
	prvm_vec3_t mins, maxs;
	VectorCopy(min, mins);
	VectorCopy(max, maxs);
	SetMinMaxSizePRVM(prog, e, mins, maxs);
}

// #3 void(entity e, string m) setmodel
static void VM_CL_setmodel (prvm_prog_t *prog)
{
	prvm_edict_t	*e;
	const char		*m;
	model_t *mod;
	int				i;

	VM_SAFEPARMCOUNT(2, VM_CL_setmodel);

	e = PRVM_G_EDICT(OFS_PARM0);
	PRVM_clientedictfloat(e, modelindex) = 0;
	PRVM_clientedictstring(e, model) = 0;

	m = PRVM_G_STRING(OFS_PARM1);
	mod = NULL;
	for (i = 0;i < MAX_MODELS && cl.csqc_model_precache[i];i++)
	{
		if (!strcmp(cl.csqc_model_precache[i]->name, m))
		{
			mod = cl.csqc_model_precache[i];
			PRVM_clientedictstring(e, model) = PRVM_SetEngineString(prog, mod->name);
			PRVM_clientedictfloat(e, modelindex) = -(i+1);
			break;
		}
	}

	if( !mod ) {
		for (i = 0;i < MAX_MODELS;i++)
		{
			mod = cl.model_precache[i];
			if (mod && !strcmp(mod->name, m))
			{
				PRVM_clientedictstring(e, model) = PRVM_SetEngineString(prog, mod->name);
				PRVM_clientedictfloat(e, modelindex) = i;
				break;
			}
		}
	}

	if( mod ) {
		// TODO: check if this breaks needed consistency and maybe add a cvar for it too?? [1/10/2008 Black]
		// LadyHavoc: erm you broke it by commenting this out - setmodel must do setsize or else the qc can't find out the model size, and ssqc does this by necessity, consistency.
		SetMinMaxSize (prog, e, mod->normalmins, mod->normalmaxs);
	}
	else
	{
		SetMinMaxSize (prog, e, vec3_origin, vec3_origin);
		VM_Warning(prog, "setmodel: model '%s' not precached\n", m);
	}
}

// #4 void(entity e, vector min, vector max) setsize
static void VM_CL_setsize (prvm_prog_t *prog)
{
	prvm_edict_t	*e;
	vec3_t		mins, maxs;
	VM_SAFEPARMCOUNT(3, VM_CL_setsize);

	e = PRVM_G_EDICT(OFS_PARM0);
	if (e == prog->edicts)
	{
		VM_Warning(prog, "setsize: can not modify world entity\n");
		return;
	}
	if (e->free)
	{
		VM_Warning(prog, "setsize: can not modify free entity\n");
		return;
	}
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), mins);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), maxs);

	SetMinMaxSize( prog, e, mins, maxs );

	CL_LinkEdict(e);
}

// #8 void(entity e, float chan, string samp, float volume, float atten[, float pitchchange[, float flags]]) sound
static void VM_CL_sound (prvm_prog_t *prog)
{
	const char			*sample;
	int					channel;
	prvm_edict_t		*entity;
	float 				fvolume;
	float				attenuation;
	float pitchchange;
	float				startposition;
	int flags;
	vec3_t				org;

	VM_SAFEPARMCOUNTRANGE(5, 7, VM_CL_sound);

	entity = PRVM_G_EDICT(OFS_PARM0);
	channel = (int)PRVM_G_FLOAT(OFS_PARM1);
	sample = PRVM_G_STRING(OFS_PARM2);
	fvolume = PRVM_G_FLOAT(OFS_PARM3);
	attenuation = PRVM_G_FLOAT(OFS_PARM4);

	if (fvolume < 0 || fvolume > 1)
	{
		VM_Warning(prog, "VM_CL_sound: volume must be in range 0-1\n");
		return;
	}

	if (attenuation < 0 || attenuation > 4)
	{
		VM_Warning(prog, "VM_CL_sound: attenuation must be in range 0-4\n");
		return;
	}

	if (prog->argc < 6)
		pitchchange = 0;
	else
		pitchchange = PRVM_G_FLOAT(OFS_PARM5);

	if (prog->argc < 7)
		flags = 0;
	else
	{
		// LadyHavoc: we only let the qc set certain flags, others are off-limits
		flags = (int)PRVM_G_FLOAT(OFS_PARM6) & (CHANNELFLAG_RELIABLE | CHANNELFLAG_FORCELOOP | CHANNELFLAG_PAUSED | CHANNELFLAG_FULLVOLUME);
	}

	// sound_starttime exists instead of sound_startposition because in a
	// networking sense you might not know when something is being received,
	// so making sounds match up in sync would be impossible if relative
	// position was sent
	if (PRVM_clientglobalfloat(sound_starttime))
		startposition = cl.time - PRVM_clientglobalfloat(sound_starttime);
	else
		startposition = 0;

	if (!IS_CHAN(channel))
	{
		VM_Warning(prog, "VM_CL_sound: channel must be in range 0-127\n");
		return;
	}

	CL_VM_GetEntitySoundOrigin(MAX_EDICTS + PRVM_NUM_FOR_EDICT(entity), org);
	S_StartSound_StartPosition_Flags(MAX_EDICTS + PRVM_NUM_FOR_EDICT(entity), channel, S_FindName(sample), org, fvolume, attenuation, startposition, flags, pitchchange > 0.0f ? pitchchange * 0.01f : 1.0f);
}

// #483 void(vector origin, string sample, float volume, float attenuation) pointsound
static void VM_CL_pointsound(prvm_prog_t *prog)
{
	const char			*sample;
	float 				fvolume;
	float				attenuation;
	vec3_t				org;

	VM_SAFEPARMCOUNT(4, VM_CL_pointsound);

	VectorCopy( PRVM_G_VECTOR(OFS_PARM0), org);
	sample = PRVM_G_STRING(OFS_PARM1);
	fvolume = PRVM_G_FLOAT(OFS_PARM2);
	attenuation = PRVM_G_FLOAT(OFS_PARM3);

	if (fvolume < 0 || fvolume > 1)
	{
		VM_Warning(prog, "VM_CL_pointsound: volume must be in range 0-1\n");
		return;
	}

	if (attenuation < 0 || attenuation > 4)
	{
		VM_Warning(prog, "VM_CL_pointsound: attenuation must be in range 0-4\n");
		return;
	}

	// Send World Entity as Entity to Play Sound (for CSQC, that is MAX_EDICTS)
	S_StartSound(MAX_EDICTS, 0, S_FindName(sample), org, fvolume, attenuation);
}

// #14 entity() spawn
static void VM_CL_spawn (prvm_prog_t *prog)
{
	prvm_edict_t *ed;
	ed = PRVM_ED_Alloc(prog);
	VM_RETURN_EDICT(ed);
}

static void CL_VM_SetTraceGlobals(prvm_prog_t *prog, const trace_t *trace, int svent)
{
	VM_SetTraceGlobals(prog, trace);
	PRVM_clientglobalfloat(trace_networkentity) = svent;
}

#define CL_HitNetworkBrushModels(move) !((move) == MOVE_WORLDONLY)
#define CL_HitNetworkPlayers(move)     !((move) == MOVE_WORLDONLY || (move) == MOVE_NOMONSTERS)

// #16 void(vector v1, vector v2, float movetype, entity ignore) traceline
static void VM_CL_traceline (prvm_prog_t *prog)
{
	vec3_t	v1, v2;
	trace_t	trace;
	int		move, svent;
	prvm_edict_t	*ent;

//	R_TimeReport("pretraceline");

	VM_SAFEPARMCOUNTRANGE(4, 4, VM_CL_traceline);

	prog->xfunction->builtinsprofile += 30;

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), v1);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), v2);
	move = (int)PRVM_G_FLOAT(OFS_PARM2);
	ent = PRVM_G_EDICT(OFS_PARM3);

	if (isnan(v1[0]) || isnan(v1[1]) || isnan(v1[2]) || isnan(v2[0]) || isnan(v2[1]) || isnan(v2[2]))
		prog->error_cmd("%s: NAN errors detected in traceline('%f %f %f', '%f %f %f', %i, entity %i)\n", prog->name, v1[0], v1[1], v1[2], v2[0], v2[1], v2[2], move, PRVM_EDICT_TO_PROG(ent));

	trace = CL_TraceLine(v1, v2, move, ent, CL_GenericHitSuperContentsMask(ent), 0, 0, collision_extendtracelinelength.value, CL_HitNetworkBrushModels(move), CL_HitNetworkPlayers(move), &svent, true, false);

	CL_VM_SetTraceGlobals(prog, &trace, svent);
//	R_TimeReport("traceline");
}

/*
=================
VM_CL_tracebox

Used for use tracing and shot targeting
Traces are blocked by bbox and exact bsp entityes, and also slide box entities
if the tryents flag is set.

tracebox (vector1, vector mins, vector maxs, vector2, tryents)
=================
*/
// LadyHavoc: added this for my own use, VERY useful, similar to traceline
static void VM_CL_tracebox (prvm_prog_t *prog)
{
	vec3_t	v1, v2, m1, m2;
	trace_t	trace;
	int		move, svent;
	prvm_edict_t	*ent;

//	R_TimeReport("pretracebox");
	VM_SAFEPARMCOUNTRANGE(6, 8, VM_CL_tracebox); // allow more parameters for future expansion

	prog->xfunction->builtinsprofile += 30;

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), v1);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), m1);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), m2);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM3), v2);
	move = (int)PRVM_G_FLOAT(OFS_PARM4);
	ent = PRVM_G_EDICT(OFS_PARM5);

	if (isnan(v1[0]) || isnan(v1[1]) || isnan(v1[2]) || isnan(v2[0]) || isnan(v2[1]) || isnan(v2[2]))
		prog->error_cmd("%s: NAN errors detected in tracebox('%f %f %f', '%f %f %f', '%f %f %f', '%f %f %f', %i, entity %i)\n", prog->name, v1[0], v1[1], v1[2], m1[0], m1[1], m1[2], m2[0], m2[1], m2[2], v2[0], v2[1], v2[2], move, PRVM_EDICT_TO_PROG(ent));

	trace = CL_TraceBox(v1, m1, m2, v2, move, ent, CL_GenericHitSuperContentsMask(ent), 0, 0, collision_extendtraceboxlength.value, CL_HitNetworkBrushModels(move), CL_HitNetworkPlayers(move), &svent, true);

	CL_VM_SetTraceGlobals(prog, &trace, svent);
//	R_TimeReport("tracebox");
}

static trace_t CL_Trace_Toss (prvm_prog_t *prog, prvm_edict_t *tossent, prvm_edict_t *ignore, int *svent)
{
	int i;
	float gravity;
	vec3_t start, end, mins, maxs, move;
	vec3_t original_origin;
	vec3_t original_velocity;
	vec3_t original_angles;
	vec3_t original_avelocity;
	trace_t trace;

	VectorCopy(PRVM_clientedictvector(tossent, origin)   , original_origin   );
	VectorCopy(PRVM_clientedictvector(tossent, velocity) , original_velocity );
	VectorCopy(PRVM_clientedictvector(tossent, angles)   , original_angles   );
	VectorCopy(PRVM_clientedictvector(tossent, avelocity), original_avelocity);

	gravity = PRVM_clientedictfloat(tossent, gravity);
	if (!gravity)
		gravity = 1.0f;
	gravity *= cl.movevars_gravity * 0.05;

	for (i = 0;i < 200;i++) // LadyHavoc: sanity check; never trace more than 10 seconds
	{
		PRVM_clientedictvector(tossent, velocity)[2] -= gravity;
		VectorMA (PRVM_clientedictvector(tossent, angles), 0.05, PRVM_clientedictvector(tossent, avelocity), PRVM_clientedictvector(tossent, angles));
		VectorScale (PRVM_clientedictvector(tossent, velocity), 0.05, move);
		VectorAdd (PRVM_clientedictvector(tossent, origin), move, end);
		VectorCopy(PRVM_clientedictvector(tossent, origin), start);
		VectorCopy(PRVM_clientedictvector(tossent, mins), mins);
		VectorCopy(PRVM_clientedictvector(tossent, maxs), maxs);
		trace = CL_TraceBox(start, mins, maxs, end, MOVE_NORMAL, tossent, CL_GenericHitSuperContentsMask(tossent), 0, 0, collision_extendmovelength.value, true, true, NULL, true);
		VectorCopy (trace.endpos, PRVM_clientedictvector(tossent, origin));

		if (trace.fraction < 1)
			break;
	}

	VectorCopy(original_origin   , PRVM_clientedictvector(tossent, origin)   );
	VectorCopy(original_velocity , PRVM_clientedictvector(tossent, velocity) );
	VectorCopy(original_angles   , PRVM_clientedictvector(tossent, angles)   );
	VectorCopy(original_avelocity, PRVM_clientedictvector(tossent, avelocity));

	return trace;
}

static void VM_CL_tracetoss (prvm_prog_t *prog)
{
	trace_t	trace;
	prvm_edict_t	*ent;
	prvm_edict_t	*ignore;
	int svent = 0;

	prog->xfunction->builtinsprofile += 600;

	VM_SAFEPARMCOUNT(2, VM_CL_tracetoss);

	ent = PRVM_G_EDICT(OFS_PARM0);
	if (ent == prog->edicts)
	{
		VM_Warning(prog, "tracetoss: can not use world entity\n");
		return;
	}
	ignore = PRVM_G_EDICT(OFS_PARM1);

	trace = CL_Trace_Toss (prog, ent, ignore, &svent);

	CL_VM_SetTraceGlobals(prog, &trace, svent);
}


// #20 void(string s) precache_model
static void VM_CL_precache_model (prvm_prog_t *prog)
{
	const char	*name;
	int			i;
	model_t		*m;

	VM_SAFEPARMCOUNT(1, VM_CL_precache_model);

	name = PRVM_G_STRING(OFS_PARM0);
	for (i = 0;i < MAX_MODELS && cl.csqc_model_precache[i];i++)
	{
		if(!strcmp(cl.csqc_model_precache[i]->name, name))
		{
			PRVM_G_FLOAT(OFS_RETURN) = -(i+1);
			return;
		}
	}
	PRVM_G_FLOAT(OFS_RETURN) = 0;
	m = Mod_ForName(name, false, false, name[0] == '*' ? cl.model_name[1] : NULL);
	if(m && m->loaded)
	{
		for (i = 0;i < MAX_MODELS;i++)
		{
			if (!cl.csqc_model_precache[i])
			{
				cl.csqc_model_precache[i] = (model_t*)m;
				PRVM_G_FLOAT(OFS_RETURN) = -(i+1);
				return;
			}
		}
		VM_Warning(prog, "VM_CL_precache_model: no free models\n");
		return;
	}
	VM_Warning(prog, "VM_CL_precache_model: model \"%s\" not found\n", name);
}

// #22 entity(vector org, float rad) findradius
static void VM_CL_findradius (prvm_prog_t *prog)
{
	prvm_edict_t	*ent, *chain;
	vec_t			radius, radius2;
	vec3_t			org, eorg, mins, maxs;
	int				i, numtouchedicts;
	static prvm_edict_t	*touchedicts[MAX_EDICTS];
	int             chainfield;

	VM_SAFEPARMCOUNTRANGE(2, 3, VM_CL_findradius);

	if(prog->argc == 3)
		chainfield = PRVM_G_INT(OFS_PARM2);
	else
		chainfield = prog->fieldoffsets.chain;
	if(chainfield < 0)
		prog->error_cmd("VM_CL_findradius: %s doesnt have the specified chain field !", prog->name);

	chain = (prvm_edict_t *)prog->edicts;

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), org);
	radius = PRVM_G_FLOAT(OFS_PARM1);
	radius2 = radius * radius;

	mins[0] = org[0] - (radius + 1);
	mins[1] = org[1] - (radius + 1);
	mins[2] = org[2] - (radius + 1);
	maxs[0] = org[0] + (radius + 1);
	maxs[1] = org[1] + (radius + 1);
	maxs[2] = org[2] + (radius + 1);
	numtouchedicts = World_EntitiesInBox(&cl.world, mins, maxs, MAX_EDICTS, touchedicts);
	if (numtouchedicts > MAX_EDICTS)
	{
		// this never happens	//[515]: for what then ?
		Con_Printf("CSQC_EntitiesInBox returned %i edicts, max was %i\n", numtouchedicts, MAX_EDICTS);
		numtouchedicts = MAX_EDICTS;
	}
	for (i = 0;i < numtouchedicts;i++)
	{
		ent = touchedicts[i];
		// Quake did not return non-solid entities but darkplaces does
		// (note: this is the reason you can't blow up fallen zombies)
		if (PRVM_clientedictfloat(ent, solid) == SOLID_NOT && !sv_gameplayfix_blowupfallenzombies.integer)
			continue;
		// LadyHavoc: compare against bounding box rather than center so it
		// doesn't miss large objects, and use DotProduct instead of Length
		// for a major speedup
		VectorSubtract(org, PRVM_clientedictvector(ent, origin), eorg);
		if (sv_gameplayfix_findradiusdistancetobox.integer)
		{
			eorg[0] -= bound(PRVM_clientedictvector(ent, mins)[0], eorg[0], PRVM_clientedictvector(ent, maxs)[0]);
			eorg[1] -= bound(PRVM_clientedictvector(ent, mins)[1], eorg[1], PRVM_clientedictvector(ent, maxs)[1]);
			eorg[2] -= bound(PRVM_clientedictvector(ent, mins)[2], eorg[2], PRVM_clientedictvector(ent, maxs)[2]);
		}
		else
			VectorMAMAM(1, eorg, -0.5f, PRVM_clientedictvector(ent, mins), -0.5f, PRVM_clientedictvector(ent, maxs), eorg);
		if (DotProduct(eorg, eorg) < radius2)
		{
			PRVM_EDICTFIELDEDICT(ent, chainfield) = PRVM_EDICT_TO_PROG(chain);
			chain = ent;
		}
	}

	VM_RETURN_EDICT(chain);
}

// #566 entity(vector mins, vector maxs) findbox
// #566 entity(vector mins, vector maxs, .entity tofield) findbox_tofield
static void VM_CL_findbox (prvm_prog_t *prog)
{
	prvm_edict_t *chain;
	int i, numtouchedicts;
	static prvm_edict_t *touchedicts[MAX_EDICTS];
	int chainfield;

	VM_SAFEPARMCOUNTRANGE(2, 3, VM_CL_findbox);

	if(prog->argc == 3)
		chainfield = PRVM_G_INT(OFS_PARM2);
	else
		chainfield = prog->fieldoffsets.chain;
	if(chainfield < 0)
		prog->error_cmd("VM_CL_findbox: %s doesnt have the specified chain field !", prog->name);

	chain = (prvm_edict_t *)prog->edicts;

	numtouchedicts = World_EntitiesInBox(&cl.world, PRVM_G_VECTOR(OFS_PARM0), PRVM_G_VECTOR(OFS_PARM1), MAX_EDICTS, touchedicts);
	if (numtouchedicts > MAX_EDICTS)
	{
		// this never happens	//[515]: for what then ?
		Con_Printf("World_EntitiesInBox returned %i edicts, max was %i\n", numtouchedicts, MAX_EDICTS);
		numtouchedicts = MAX_EDICTS;
	}
	for (i = 0; i < numtouchedicts; ++i)
	{
		PRVM_EDICTFIELDEDICT(touchedicts[i], chainfield) = PRVM_EDICT_TO_PROG(chain);
		chain = touchedicts[i];
	}

	VM_RETURN_EDICT(chain);
}

// #34 float() droptofloor
static void VM_CL_droptofloor (prvm_prog_t *prog)
{
	prvm_edict_t		*ent;
	vec3_t				start, end, mins, maxs;
	trace_t				trace;

	VM_SAFEPARMCOUNTRANGE(0, 2, VM_CL_droptofloor); // allow 2 parameters because the id1 defs.qc had an incorrect prototype

	// assume failure if it returns early
	PRVM_G_FLOAT(OFS_RETURN) = 0;

	ent = PRVM_PROG_TO_EDICT(PRVM_clientglobaledict(self));
	if (ent == prog->edicts)
	{
		VM_Warning(prog, "droptofloor: can not modify world entity\n");
		return;
	}
	if (ent->free)
	{
		VM_Warning(prog, "droptofloor: can not modify free entity\n");
		return;
	}

	VectorCopy(PRVM_clientedictvector(ent, origin), start);
	VectorCopy(PRVM_clientedictvector(ent, mins), mins);
	VectorCopy(PRVM_clientedictvector(ent, maxs), maxs);
	VectorCopy(PRVM_clientedictvector(ent, origin), end);
	if (cl.worldmodel->brush.isq3bsp)
		end[2] -= 4096;
	else if (cl.worldmodel->brush.isq2bsp)
		end[2] -= 128;
	else
		end[2] -= 256; // Quake, QuakeWorld

	trace = CL_TraceBox(start, mins, maxs, end, MOVE_NORMAL, ent, CL_GenericHitSuperContentsMask(ent), 0, 0, collision_extendmovelength.value, true, true, NULL, true);

	if (trace.fraction != 1)
	{
		VectorCopy (trace.endpos, PRVM_clientedictvector(ent, origin));
		PRVM_clientedictfloat(ent, flags) = (int)PRVM_clientedictfloat(ent, flags) | FL_ONGROUND;
		PRVM_clientedictedict(ent, groundentity) = PRVM_EDICT_TO_PROG(trace.ent);
		PRVM_G_FLOAT(OFS_RETURN) = 1;
		// if support is destroyed, keep suspended (gross hack for floating items in various maps)
//		ent->priv.server->suspendedinairflag = true;
	}
}

// #35 void(float style, string value) lightstyle
static void VM_CL_lightstyle (prvm_prog_t *prog)
{
	int			i;
	const char	*c;

	VM_SAFEPARMCOUNT(2, VM_CL_lightstyle);

	i = (int)PRVM_G_FLOAT(OFS_PARM0);
	c = PRVM_G_STRING(OFS_PARM1);
	if (i >= cl.max_lightstyle)
	{
		VM_Warning(prog, "VM_CL_lightstyle >= MAX_LIGHTSTYLES\n");
		return;
	}
	dp_strlcpy (cl.lightstyle[i].map, c, sizeof (cl.lightstyle[i].map));
	cl.lightstyle[i].map[MAX_STYLESTRING - 1] = 0;
	cl.lightstyle[i].length = (int)strlen(cl.lightstyle[i].map);
}

// #40 float(entity e) checkbottom
static void VM_CL_checkbottom (prvm_prog_t *prog)
{
	static int		cs_yes, cs_no;
	prvm_edict_t	*ent;
	vec3_t			mins, maxs, start, stop;
	trace_t			trace;
	int				x, y;
	float			mid, bottom;

	VM_SAFEPARMCOUNT(1, VM_CL_checkbottom);
	ent = PRVM_G_EDICT(OFS_PARM0);
	PRVM_G_FLOAT(OFS_RETURN) = 0;

	VectorAdd (PRVM_clientedictvector(ent, origin), PRVM_clientedictvector(ent, mins), mins);
	VectorAdd (PRVM_clientedictvector(ent, origin), PRVM_clientedictvector(ent, maxs), maxs);

// if all of the points under the corners are solid world, don't bother
// with the tougher checks
// the corners must be within 16 of the midpoint
	start[2] = mins[2] - 1;
	for	(x=0 ; x<=1 ; x++)
		for	(y=0 ; y<=1 ; y++)
		{
			start[0] = x ? maxs[0] : mins[0];
			start[1] = y ? maxs[1] : mins[1];
			if (!(CL_PointSuperContents(start) & (SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY)))
				goto realcheck;
		}

	cs_yes++;
	PRVM_G_FLOAT(OFS_RETURN) = true;
	return;		// we got out easy

realcheck:
	cs_no++;
//
// check it for real...
//
	start[2] = mins[2];

// the midpoint must be within 16 of the bottom
	start[0] = stop[0] = (mins[0] + maxs[0])*0.5;
	start[1] = stop[1] = (mins[1] + maxs[1])*0.5;
	stop[2] = start[2] - 2*sv_stepheight.value;
	trace = CL_TraceLine(start, stop, MOVE_NORMAL, ent, CL_GenericHitSuperContentsMask(ent), 0, 0, collision_extendmovelength.value, true, true, NULL, true, false);

	if (trace.fraction == 1.0)
		return;

	mid = bottom = trace.endpos[2];

// the corners must be within 16 of the midpoint
	for	(x=0 ; x<=1 ; x++)
		for	(y=0 ; y<=1 ; y++)
		{
			start[0] = stop[0] = x ? maxs[0] : mins[0];
			start[1] = stop[1] = y ? maxs[1] : mins[1];

			trace = CL_TraceLine(start, stop, MOVE_NORMAL, ent, CL_GenericHitSuperContentsMask(ent), 0, 0, collision_extendmovelength.value, true, true, NULL, true, false);

			if (trace.fraction != 1.0 && trace.endpos[2] > bottom)
				bottom = trace.endpos[2];
			if (trace.fraction == 1.0 || mid - trace.endpos[2] > sv_stepheight.value)
				return;
		}

	cs_yes++;
	PRVM_G_FLOAT(OFS_RETURN) = true;
}

// #41 float(vector v) pointcontents
static void VM_CL_pointcontents (prvm_prog_t *prog)
{
	vec3_t point;
	VM_SAFEPARMCOUNT(1, VM_CL_pointcontents);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), point);
	PRVM_G_FLOAT(OFS_RETURN) = Mod_Q1BSP_NativeContentsFromSuperContents(CL_PointSuperContents(point));
}

// #48 void(vector o, vector d, float color, float count) particle
static void VM_CL_particle (prvm_prog_t *prog)
{
	vec3_t org, dir;
	int		count;
	unsigned char	color;
	VM_SAFEPARMCOUNT(4, VM_CL_particle);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), org);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), dir);
	color = (int)PRVM_G_FLOAT(OFS_PARM2);
	count = (int)PRVM_G_FLOAT(OFS_PARM3);
	CL_ParticleEffect(EFFECT_SVC_PARTICLE, count, org, org, dir, dir, NULL, color);
}

// #74 void(vector pos, string samp, float vol, float atten) ambientsound
static void VM_CL_ambientsound (prvm_prog_t *prog)
{
	vec3_t f;
	sfx_t	*s;
	VM_SAFEPARMCOUNT(4, VM_CL_ambientsound);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), f);
	s = S_FindName(PRVM_G_STRING(OFS_PARM1));
	S_StaticSound (s, f, PRVM_G_FLOAT(OFS_PARM2), PRVM_G_FLOAT(OFS_PARM3)*64);
}

// #92 vector(vector org[, float lpflag]) getlight (DP_QC_GETLIGHT)
static void VM_CL_getlight (prvm_prog_t *prog)
{
	vec3_t ambientcolor, diffusecolor, diffusenormal;
	vec3_t p;
	int flags = prog->argc >= 2 ? PRVM_G_FLOAT(OFS_PARM1) : LP_LIGHTMAP;

	VM_SAFEPARMCOUNTRANGE(1, 3, VM_CL_getlight);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), p);
	R_CompleteLightPoint(ambientcolor, diffusecolor, diffusenormal, p, flags, r_refdef.scene.lightmapintensity, r_refdef.scene.ambientintensity);
	VectorMA(ambientcolor, 0.5, diffusecolor, PRVM_G_VECTOR(OFS_RETURN));
	if (PRVM_clientglobalvector(getlight_ambient))
		VectorCopy(ambientcolor, PRVM_clientglobalvector(getlight_ambient));
	if (PRVM_clientglobalvector(getlight_diffuse))
		VectorCopy(diffusecolor, PRVM_clientglobalvector(getlight_diffuse));
	if (PRVM_clientglobalvector(getlight_dir))
		VectorCopy(diffusenormal, PRVM_clientglobalvector(getlight_dir));
}

//============================================================================
//[515]: SCENE MANAGER builtins

extern cvar_t v_yshearing;
void CSQC_R_RecalcView (void)
{
	extern matrix4x4_t viewmodelmatrix_nobob;
	extern matrix4x4_t viewmodelmatrix_withbob;
	Matrix4x4_CreateFromQuakeEntity(&r_refdef.view.matrix, cl.csqc_vieworigin[0], cl.csqc_vieworigin[1], cl.csqc_vieworigin[2], cl.csqc_viewangles[0], cl.csqc_viewangles[1], cl.csqc_viewangles[2], 1);
	if (v_yshearing.value > 0)
		Matrix4x4_QuakeToDuke3D(&r_refdef.view.matrix, &r_refdef.view.matrix, v_yshearing.value);
	Matrix4x4_Copy(&viewmodelmatrix_nobob, &r_refdef.view.matrix);
	Matrix4x4_ConcatScale(&viewmodelmatrix_nobob, cl_viewmodel_scale.value);
	Matrix4x4_Concat(&viewmodelmatrix_withbob, &r_refdef.view.matrix, &cl.csqc_viewmodelmatrixfromengine);
}

//#300 void() clearscene (EXT_CSQC)
static void VM_CL_R_ClearScene (prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(0, VM_CL_R_ClearScene);
	// clear renderable entity and light lists
	r_refdef.scene.numentities = 0;
	r_refdef.scene.numlights = 0;
	// restore the view settings to the values that VM_CL_UpdateView received from the client code
	r_refdef.view = csqc_original_r_refdef_view;
	// polygonbegin without draw2d arg has to guess
	prog->polygonbegin_guess2d = false;
	VectorCopy(cl.csqc_vieworiginfromengine, cl.csqc_vieworigin);
	VectorCopy(cl.csqc_viewanglesfromengine, cl.csqc_viewangles);
	cl.csqc_vidvars.drawworld = r_drawworld.integer != 0;
	cl.csqc_vidvars.drawenginesbar = false;
	cl.csqc_vidvars.drawcrosshair = false;
	CSQC_R_RecalcView();
	// clear the CL_Mesh_Scene() used for CSQC polygons and engine effects, they will be added by CSQC_RelinkAllEntities and manually created by CSQC
	CL_MeshEntities_Scene_Clear();
}

//#301 void(float mask) addentities (EXT_CSQC)
static void VM_CL_R_AddEntities (prvm_prog_t *prog)
{
	double t = Sys_DirtyTime();
	int			i, drawmask;
	prvm_edict_t *ed;
	VM_SAFEPARMCOUNT(1, VM_CL_R_AddEntities);
	drawmask = (int)PRVM_G_FLOAT(OFS_PARM0);
	CSQC_RelinkAllEntities(drawmask);

	PRVM_clientglobalfloat(time) = cl.time;
	for(i=1;i<prog->num_edicts;i++)
	{
		// so we can easily check if CSQC entity #edictnum is currently drawn
		cl.csqcrenderentities[i].entitynumber = 0;
		ed = &prog->edicts[i];
		if(ed->free)
			continue;
		CSQC_Think(ed);
		if(ed->free)
			continue;
		// note that for RF_USEAXIS entities, Predraw sets v_forward/v_right/v_up globals that are read by CSQC_AddRenderEdict
		CSQC_Predraw(ed);
		if(ed->free)
			continue;
		if(!((int)PRVM_clientedictfloat(ed, drawmask) & drawmask))
			continue;
		CSQC_AddRenderEdict(ed, i);
	}

	// callprofile fixing hack: do not include this time in what is counted for CSQC_UpdateView
	t = Sys_DirtyTime() - t;if (t < 0 || t >= 1800) t = 0;
	prog->functions[PRVM_clientfunction(CSQC_UpdateView)].totaltime -= t;
}

//#302 void(entity ent) addentity (EXT_CSQC)
static void VM_CL_R_AddEntity (prvm_prog_t *prog)
{
	double t = Sys_DirtyTime();
	VM_SAFEPARMCOUNT(1, VM_CL_R_AddEntity);
	CSQC_AddRenderEdict(PRVM_G_EDICT(OFS_PARM0), 0);
	t = Sys_DirtyTime() - t;if (t < 0 || t >= 1800) t = 0;
	prog->functions[PRVM_clientfunction(CSQC_UpdateView)].totaltime -= t;
}

//#303 float(float property, ...) setproperty (EXT_CSQC)
//#303 float(float property) getproperty
//#303 vector(float property) getpropertyvec
//#309 float(float property) getproperty
//#309 vector(float property) getpropertyvec
// VorteX: make this function be able to return previously set property if new value is not given
static void VM_CL_R_SetView (prvm_prog_t *prog)
{
	int		c;
	prvm_vec_t	*f;
	float	k;

	VM_SAFEPARMCOUNTRANGE(1, 3, VM_CL_R_SetView);

	c = (int)PRVM_G_FLOAT(OFS_PARM0);

	// return value?
	if (prog->argc < 2)
	{
		switch(c)
		{
		case VF_MIN:
			VectorSet(PRVM_G_VECTOR(OFS_RETURN), r_refdef.view.x, r_refdef.view.y, 0);
			break;
		case VF_MIN_X:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.view.x;
			break;
		case VF_MIN_Y:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.view.y;
			break;
		case VF_SIZE:
			VectorSet(PRVM_G_VECTOR(OFS_RETURN), r_refdef.view.width, r_refdef.view.height, 0);
			break;
		case VF_SIZE_X:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.view.width;
			break;
		case VF_SIZE_Y:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.view.height;
			break;
		case VF_VIEWPORT:
			VM_Warning(prog, "VM_CL_R_GetView : VF_VIEWPORT can't be retrieved, use VF_MIN/VF_SIZE instead\n");
			break;
		case VF_FOV:
			VectorSet(PRVM_G_VECTOR(OFS_RETURN), r_refdef.view.ortho_x, r_refdef.view.ortho_y, 0);
			break;
		case VF_FOVX:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.view.ortho_x;
			break;
		case VF_FOVY:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.view.ortho_y;
			break;
		case VF_ORIGIN:
			VectorCopy(cl.csqc_vieworigin, PRVM_G_VECTOR(OFS_RETURN));
			break;
		case VF_ORIGIN_X:
			PRVM_G_FLOAT(OFS_RETURN) = cl.csqc_vieworigin[0];
			break;
		case VF_ORIGIN_Y:
			PRVM_G_FLOAT(OFS_RETURN) = cl.csqc_vieworigin[1];
			break;
		case VF_ORIGIN_Z:
			PRVM_G_FLOAT(OFS_RETURN) = cl.csqc_vieworigin[2];
			break;
		case VF_ANGLES:
			VectorCopy(cl.csqc_viewangles, PRVM_G_VECTOR(OFS_RETURN));
			break;
		case VF_ANGLES_X:
			PRVM_G_FLOAT(OFS_RETURN) = cl.csqc_viewangles[0];
			break;
		case VF_ANGLES_Y:
			PRVM_G_FLOAT(OFS_RETURN) = cl.csqc_viewangles[1];
			break;
		case VF_ANGLES_Z:
			PRVM_G_FLOAT(OFS_RETURN) = cl.csqc_viewangles[2];
			break;
		case VF_DRAWWORLD:
			PRVM_G_FLOAT(OFS_RETURN) = cl.csqc_vidvars.drawworld;
			break;
		case VF_DRAWENGINESBAR:
			PRVM_G_FLOAT(OFS_RETURN) = cl.csqc_vidvars.drawenginesbar;
			break;
		case VF_DRAWCROSSHAIR:
			PRVM_G_FLOAT(OFS_RETURN) = cl.csqc_vidvars.drawcrosshair;
			break;
		case VF_CL_VIEWANGLES:
			VectorCopy(cl.viewangles, PRVM_G_VECTOR(OFS_RETURN));;
			break;
		case VF_CL_VIEWANGLES_X:
			PRVM_G_FLOAT(OFS_RETURN) = cl.viewangles[0];
			break;
		case VF_CL_VIEWANGLES_Y:
			PRVM_G_FLOAT(OFS_RETURN) = cl.viewangles[1];
			break;
		case VF_CL_VIEWANGLES_Z:
			PRVM_G_FLOAT(OFS_RETURN) = cl.viewangles[2];
			break;
		case VF_PERSPECTIVE:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.view.useperspective;
			break;
		case VF_CLEARSCREEN:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.view.isoverlay;
			break;
		case VF_MAINVIEW:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.view.ismain;
			break;
		case VF_FOG_DENSITY:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.fog_density;
			break;
		case VF_FOG_COLOR:
			PRVM_G_VECTOR(OFS_RETURN)[0] = r_refdef.fog_red;
			PRVM_G_VECTOR(OFS_RETURN)[1] = r_refdef.fog_green;
			PRVM_G_VECTOR(OFS_RETURN)[2] = r_refdef.fog_blue;
			break;
		case VF_FOG_COLOR_R:
			PRVM_G_VECTOR(OFS_RETURN)[0] = r_refdef.fog_red;
			break;
		case VF_FOG_COLOR_G:
			PRVM_G_VECTOR(OFS_RETURN)[1] = r_refdef.fog_green;
			break;
		case VF_FOG_COLOR_B:
			PRVM_G_VECTOR(OFS_RETURN)[2] = r_refdef.fog_blue;
			break;
		case VF_FOG_ALPHA:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.fog_alpha;
			break;
		case VF_FOG_START:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.fog_start;
			break;
		case VF_FOG_END:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.fog_end;
			break;
		case VF_FOG_HEIGHT:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.fog_height;
			break;
		case VF_FOG_FADEDEPTH:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.fog_fadedepth;
			break;
		case VF_MINFPS_QUALITY:
			PRVM_G_FLOAT(OFS_RETURN) = r_refdef.view.quality;
			break;
		default:
			PRVM_G_FLOAT(OFS_RETURN) = 0;
			VM_Warning(prog, "VM_CL_R_GetView : unknown parm %i\n", c);
			return;
		}
		if (csqc_lowres.integer)
		{
			switch(c)
			{
				case VF_MIN: case VF_MIN_X: case VF_MIN_Y: case VF_SIZE: case VF_SIZE_X: case VF_SIZE_Y: case VF_VIEWPORT:
					VectorScale(PRVM_G_VECTOR(OFS_RETURN), vid_conwidth.value / vid.mode.width, PRVM_G_VECTOR(OFS_RETURN));
			}
		}
		return;
	}

	if (csqc_lowres.integer)
	{
		float scale = vid.mode.width / vid_conwidth.value;
		switch(c)
		{
			case VF_MIN: case VF_MIN_X: case VF_MIN_Y: case VF_SIZE: case VF_SIZE_X: case VF_SIZE_Y: case VF_VIEWPORT:
				VectorScale(PRVM_G_VECTOR(OFS_PARM1), scale, PRVM_G_VECTOR(OFS_PARM1));
				VectorScale(PRVM_G_VECTOR(OFS_PARM2), scale, PRVM_G_VECTOR(OFS_PARM2));
		}
	}
	f = PRVM_G_VECTOR(OFS_PARM1);
	k = PRVM_G_FLOAT(OFS_PARM1);
	switch(c)
	{
	case VF_MIN:
		r_refdef.view.x = (int)(f[0]);
		r_refdef.view.y = (int)(f[1]);
		DrawQ_RecalcView();
		break;
	case VF_MIN_X:
		r_refdef.view.x = (int)(k);
		DrawQ_RecalcView();
		break;
	case VF_MIN_Y:
		r_refdef.view.y = (int)(k);
		DrawQ_RecalcView();
		break;
	case VF_SIZE:
		r_refdef.view.width = (int)(f[0]);
		r_refdef.view.height = (int)(f[1]);
		DrawQ_RecalcView();
		break;
	case VF_SIZE_X:
		r_refdef.view.width = (int)(k);
		DrawQ_RecalcView();
		break;
	case VF_SIZE_Y:
		r_refdef.view.height = (int)(k);
		DrawQ_RecalcView();
		break;
	case VF_VIEWPORT:
		r_refdef.view.x = (int)(f[0]);
		r_refdef.view.y = (int)(f[1]);
		f = PRVM_G_VECTOR(OFS_PARM2);
		r_refdef.view.width = (int)(f[0]);
		r_refdef.view.height = (int)(f[1]);
		DrawQ_RecalcView();
		break;
	case VF_FOV:
		r_refdef.view.frustum_x = tan(f[0] * M_PI / 360.0);r_refdef.view.ortho_x = f[0];
		r_refdef.view.frustum_y = tan(f[1] * M_PI / 360.0);r_refdef.view.ortho_y = f[1];
		break;
	case VF_FOVX:
		r_refdef.view.frustum_x = tan(k * M_PI / 360.0);r_refdef.view.ortho_x = k;
		break;
	case VF_FOVY:
		r_refdef.view.frustum_y = tan(k * M_PI / 360.0);r_refdef.view.ortho_y = k;
		break;
	case VF_ORIGIN:
		VectorCopy(f, cl.csqc_vieworigin);
		CSQC_R_RecalcView();
		break;
	case VF_ORIGIN_X:
		cl.csqc_vieworigin[0] = k;
		CSQC_R_RecalcView();
		break;
	case VF_ORIGIN_Y:
		cl.csqc_vieworigin[1] = k;
		CSQC_R_RecalcView();
		break;
	case VF_ORIGIN_Z:
		cl.csqc_vieworigin[2] = k;
		CSQC_R_RecalcView();
		break;
	case VF_ANGLES:
		VectorCopy(f, cl.csqc_viewangles);
		CSQC_R_RecalcView();
		break;
	case VF_ANGLES_X:
		cl.csqc_viewangles[0] = k;
		CSQC_R_RecalcView();
		break;
	case VF_ANGLES_Y:
		cl.csqc_viewangles[1] = k;
		CSQC_R_RecalcView();
		break;
	case VF_ANGLES_Z:
		cl.csqc_viewangles[2] = k;
		CSQC_R_RecalcView();
		break;
	case VF_DRAWWORLD:
		cl.csqc_vidvars.drawworld = ((k != 0) && r_drawworld.integer);
		break;
	case VF_DRAWENGINESBAR:
		cl.csqc_vidvars.drawenginesbar = k != 0;
		break;
	case VF_DRAWCROSSHAIR:
		cl.csqc_vidvars.drawcrosshair = k != 0;
		break;
	case VF_CL_VIEWANGLES:
		VectorCopy(f, cl.viewangles);
		break;
	case VF_CL_VIEWANGLES_X:
		cl.viewangles[0] = k;
		break;
	case VF_CL_VIEWANGLES_Y:
		cl.viewangles[1] = k;
		break;
	case VF_CL_VIEWANGLES_Z:
		cl.viewangles[2] = k;
		break;
	case VF_PERSPECTIVE:
		r_refdef.view.useperspective = k != 0;
		break;
	case VF_CLEARSCREEN:
		r_refdef.view.isoverlay = !k;
		break;
	case VF_MAINVIEW:
		PRVM_G_FLOAT(OFS_RETURN) = r_refdef.view.ismain;
		break;
	case VF_FOG_DENSITY:
		r_refdef.fog_density = k;
		break;
	case VF_FOG_COLOR:
		r_refdef.fog_red = f[0];
		r_refdef.fog_green = f[1];
		r_refdef.fog_blue = f[2];
		break;
	case VF_FOG_COLOR_R:
		r_refdef.fog_red = k;
		break;
	case VF_FOG_COLOR_G:
		r_refdef.fog_green = k;
		break;
	case VF_FOG_COLOR_B:
		r_refdef.fog_blue = k;
		break;
	case VF_FOG_ALPHA:
		r_refdef.fog_alpha = k;
		break;
	case VF_FOG_START:
		r_refdef.fog_start = k;
		break;
	case VF_FOG_END:
		r_refdef.fog_end = k;
		break;
	case VF_FOG_HEIGHT:
		r_refdef.fog_height = k;
		break;
	case VF_FOG_FADEDEPTH:
		r_refdef.fog_fadedepth = k;
		break;
	case VF_MINFPS_QUALITY:
		r_refdef.view.quality = k;
		break;
	default:
		PRVM_G_FLOAT(OFS_RETURN) = 0;
		VM_Warning(prog, "VM_CL_R_SetView : unknown parm %i\n", c);
		return;
	}
	PRVM_G_FLOAT(OFS_RETURN) = 1;
}

//#305 void(vector org, float radius, vector lightcolours[, float style, string cubemapname, float pflags]) adddynamiclight (EXT_CSQC)
static void VM_CL_R_AddDynamicLight (prvm_prog_t *prog)
{
	double t = Sys_DirtyTime();
	vec3_t org;
	float radius = 300;
	vec3_t col;
	int style = -1;
	const char *cubemapname = NULL;
	int pflags = PFLAGS_CORONA | PFLAGS_FULLDYNAMIC;
	float coronaintensity = 1;
	float coronasizescale = 0.25;
	qbool castshadow = true;
	float ambientscale = 0;
	float diffusescale = 1;
	float specularscale = 1;
	matrix4x4_t matrix;
	vec3_t forward, left, up;
	VM_SAFEPARMCOUNTRANGE(3, 8, VM_CL_R_AddDynamicLight);

	// if we've run out of dlights, just return
	if (r_refdef.scene.numlights >= MAX_DLIGHTS)
		return;

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), org);
	radius = PRVM_G_FLOAT(OFS_PARM1);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), col);
	if (prog->argc >= 4)
	{
		style = (int)PRVM_G_FLOAT(OFS_PARM3);
		if (style >= MAX_LIGHTSTYLES)
		{
			Con_DPrintf("VM_CL_R_AddDynamicLight: out of bounds lightstyle index %i\n", style);
			style = -1;
		}
	}
	if (prog->argc >= 5)
		cubemapname = PRVM_G_STRING(OFS_PARM4);
	if (prog->argc >= 6)
		pflags = (int)PRVM_G_FLOAT(OFS_PARM5);
	coronaintensity = (pflags & PFLAGS_CORONA) != 0;
	castshadow = (pflags & PFLAGS_NOSHADOW) == 0;

	VectorScale(PRVM_clientglobalvector(v_forward), radius, forward);
	VectorScale(PRVM_clientglobalvector(v_right), -radius, left);
	VectorScale(PRVM_clientglobalvector(v_up), radius, up);
	Matrix4x4_FromVectors(&matrix, forward, left, up, org);

	R_RTLight_Update(&r_refdef.scene.templights[r_refdef.scene.numlights], false, &matrix, col, style, cubemapname, castshadow, coronaintensity, coronasizescale, ambientscale, diffusescale, specularscale, LIGHTFLAG_NORMALMODE | LIGHTFLAG_REALTIMEMODE);
	r_refdef.scene.lights[r_refdef.scene.numlights] = &r_refdef.scene.templights[r_refdef.scene.numlights];r_refdef.scene.numlights++;
	t = Sys_DirtyTime() - t;if (t < 0 || t >= 1800) t = 0;
	prog->functions[PRVM_clientfunction(CSQC_UpdateView)].totaltime -= t;
}

//============================================================================

//#310 vector (vector v) cs_unproject (EXT_CSQC)
static void VM_CL_unproject (prvm_prog_t *prog)
{
	vec3_t f;
	vec3_t temp;
	vec3_t result;

	VM_SAFEPARMCOUNT(1, VM_CL_unproject);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), f);
	VectorSet(temp,
		f[2],
		(-1.0 + 2.0 * (f[0] / vid_conwidth.integer)) * f[2] * -r_refdef.view.frustum_x,
		(-1.0 + 2.0 * (f[1] / vid_conheight.integer)) * f[2] * -r_refdef.view.frustum_y);
	if(v_flipped.integer)
		temp[1] = -temp[1];
	Matrix4x4_Transform(&r_refdef.view.matrix, temp, result);
	VectorCopy(result, PRVM_G_VECTOR(OFS_RETURN));
}

//#311 vector (vector v) cs_project (EXT_CSQC)
static void VM_CL_project (prvm_prog_t *prog)
{
	vec3_t f;
	vec3_t v;
	matrix4x4_t m;

	VM_SAFEPARMCOUNT(1, VM_CL_project);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), f);
	Matrix4x4_Invert_Full(&m, &r_refdef.view.matrix);
	Matrix4x4_Transform(&m, f, v);
	if(v_flipped.integer)
		v[1] = -v[1];
	VectorSet(PRVM_G_VECTOR(OFS_RETURN),
		vid_conwidth.integer * (0.5*(1.0+v[1]/v[0]/-r_refdef.view.frustum_x)),
		vid_conheight.integer * (0.5*(1.0+v[2]/v[0]/-r_refdef.view.frustum_y)),
		v[0]);
	// explanation:
	// after transforming, relative position to viewport (0..1) = 0.5 * (1 + v[2]/v[0]/-frustum_{x \or y})
	// as 2D drawing honors the viewport too, to get the same pixel, we simply multiply this by conwidth/height
}

//=============================================================================
// Draw builtins (client & menu)

/*
========================
VM_drawline

void drawline(float width, vector pos1, vector pos2, vector rgb, float alpha, float flags)
========================
*/
void VM_drawline (prvm_prog_t *prog)
{
	prvm_vec_t	*c1, *c2, *rgb;
	float	alpha, width;
	unsigned char	flags;

	VM_SAFEPARMCOUNT(6, VM_drawline);

	// polygonbegin without draw2d arg has to guess
	prog->polygonbegin_guess2d = true;

	width	= PRVM_G_FLOAT(OFS_PARM0);
	c1		= PRVM_G_VECTOR(OFS_PARM1);
	c2		= PRVM_G_VECTOR(OFS_PARM2);
	rgb		= PRVM_G_VECTOR(OFS_PARM3);
	alpha	= PRVM_G_FLOAT(OFS_PARM4);
	flags	= (int)PRVM_G_FLOAT(OFS_PARM5);
	DrawQ_Line(width, c1[0], c1[1], c2[0], c2[1], rgb[0], rgb[1], rgb[2], alpha, flags);
}

/*
=========
VM_iscachedpic

float	iscachedpic(string pic)
=========
*/
void VM_iscachedpic(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(1,VM_iscachedpic);

	// drawq hasnt such a function, thus always return true
	PRVM_G_FLOAT(OFS_RETURN) = false;
}

/*
=========
VM_precache_pic

string	precache_pic(string pic)
=========
*/
#define PRECACHE_PIC_FROMWAD 1 /* FTEQW, not supported here */
#define PRECACHE_PIC_NOTPERSISTENT 2
//#define PRECACHE_PIC_NOCLAMP 4
#define PRECACHE_PIC_MIPMAP 8
void VM_precache_pic(prvm_prog_t *prog)
{
	const char	*s;
	int flags = CACHEPICFLAG_FAILONMISSING;

	VM_SAFEPARMCOUNTRANGE(1, 2, VM_precache_pic);

	s = PRVM_G_STRING(OFS_PARM0);
	PRVM_G_INT(OFS_RETURN) = PRVM_G_INT(OFS_PARM0);
	VM_CheckEmptyString(prog, s);

	if(prog->argc >= 2)
	{
		int f = PRVM_G_FLOAT(OFS_PARM1);
		if(f & PRECACHE_PIC_NOTPERSISTENT)
			flags |= CACHEPICFLAG_NOTPERSISTENT;
		//if(f & PRECACHE_PIC_NOCLAMP)
		//	flags |= CACHEPICFLAG_NOCLAMP;
		if(f & PRECACHE_PIC_MIPMAP)
			flags |= CACHEPICFLAG_MIPMAP;
	}

	if( !Draw_IsPicLoaded(Draw_CachePic_Flags(s, flags | CACHEPICFLAG_QUIET)) )
		PRVM_G_INT(OFS_RETURN) = OFS_NULL;
}

/*
=========
VM_freepic

freepic(string s)
=========
*/
void VM_freepic(prvm_prog_t *prog)
{
	const char *s;

	VM_SAFEPARMCOUNT(1,VM_freepic);

	s = PRVM_G_STRING(OFS_PARM0);
	VM_CheckEmptyString(prog, s);

	Draw_FreePic(s);
}

static void getdrawfontscale(prvm_prog_t *prog, float *sx, float *sy)
{
	vec3_t v;
	*sx = *sy = 1;
	VectorCopy(PRVM_drawglobalvector(drawfontscale), v);
	if(VectorLength2(v) > 0)
	{
		*sx = v[0];
		*sy = v[1];
	}
}

static dp_font_t *getdrawfont(prvm_prog_t *prog)
{
	int f = (int) PRVM_drawglobalfloat(drawfont);
	if(f < 0 || f >= dp_fonts.maxsize)
		return FONT_DEFAULT;
	return &dp_fonts.f[f];
}

/*
=========
VM_drawcharacter

float	drawcharacter(vector position, float character, vector scale, vector rgb, float alpha, float flag)
=========
*/
void VM_drawcharacter(prvm_prog_t *prog)
{
	prvm_vec_t *pos,*scale,*rgb;
	char   character;
	int flag;
	float sx, sy;
	VM_SAFEPARMCOUNT(6,VM_drawcharacter);

	// polygonbegin without draw2d arg has to guess
	prog->polygonbegin_guess2d = true;

	character = (char) PRVM_G_FLOAT(OFS_PARM1);
	if(character == 0)
	{
		PRVM_G_FLOAT(OFS_RETURN) = -1;
		VM_Warning(prog, "VM_drawcharacter: null character passed!\n");
		return;
	}

	pos = PRVM_G_VECTOR(OFS_PARM0);
	scale = PRVM_G_VECTOR(OFS_PARM2);
	rgb = PRVM_G_VECTOR(OFS_PARM3);
	flag = (int)PRVM_G_FLOAT(OFS_PARM5);

	if(flag < DRAWFLAG_NORMAL || flag >=DRAWFLAG_NUMFLAGS)
	{
		PRVM_G_FLOAT(OFS_RETURN) = -2;
		VM_Warning(prog, "VM_drawcharacter: wrong DRAWFLAG %i !\n", flag);
		return;
	}

	if(pos[2] || scale[2])
		VM_Warning(prog, "VM_drawcharacter: z value%c from %s discarded\n",(pos[2] && scale[2]) ? 's' : 0,((pos[2] && scale[2]) ? "pos and scale" : (pos[2] ? "pos" : "scale")));

	if(!scale[0] || !scale[1])
	{
		PRVM_G_FLOAT(OFS_RETURN) = -3;
		VM_Warning(prog, "VM_drawcharacter: scale %s is null !\n", (scale[0] == 0) ? ((scale[1] == 0) ? "x and y" : "x") : "y");
		return;
	}

	getdrawfontscale(prog, &sx, &sy);
	DrawQ_String_Scale(pos[0], pos[1], &character, 1, scale[0], scale[1], sx, sy, rgb[0], rgb[1], rgb[2], PRVM_G_FLOAT(OFS_PARM4), flag, NULL, true, getdrawfont(prog));
	PRVM_G_FLOAT(OFS_RETURN) = 1;
}

/*
=========
VM_drawstring

float	drawstring(vector position, string text, vector scale, vector rgb, float alpha[, float flag])
=========
*/
void VM_drawstring(prvm_prog_t *prog)
{
	prvm_vec_t *pos,*scale,*rgb;
	const char  *string;
	int flag = 0;
	float sx, sy;
	VM_SAFEPARMCOUNTRANGE(5,6,VM_drawstring);

	// polygonbegin without draw2d arg has to guess
	prog->polygonbegin_guess2d = true;

	string = PRVM_G_STRING(OFS_PARM1);
	pos = PRVM_G_VECTOR(OFS_PARM0);
	scale = PRVM_G_VECTOR(OFS_PARM2);
	rgb = PRVM_G_VECTOR(OFS_PARM3);
	if (prog->argc >= 6)
		flag = (int)PRVM_G_FLOAT(OFS_PARM5);

	if(flag < DRAWFLAG_NORMAL || flag >=DRAWFLAG_NUMFLAGS)
	{
		PRVM_G_FLOAT(OFS_RETURN) = -2;
		VM_Warning(prog, "VM_drawstring: wrong DRAWFLAG %i !\n", flag);
		return;
	}

	if(!scale[0] || !scale[1])
	{
		PRVM_G_FLOAT(OFS_RETURN) = -3;
		VM_Warning(prog, "VM_drawstring: scale %s is null !\n", (scale[0] == 0) ? ((scale[1] == 0) ? "x and y" : "x") : "y");
		return;
	}

	if(pos[2] || scale[2])
		VM_Warning(prog, "VM_drawstring: z value%s from %s discarded\n",(pos[2] && scale[2]) ? "s" : " ",((pos[2] && scale[2]) ? "pos and scale" : (pos[2] ? "pos" : "scale")));

	getdrawfontscale(prog, &sx, &sy);
	DrawQ_String_Scale(pos[0], pos[1], string, 0, scale[0], scale[1], sx, sy, rgb[0], rgb[1], rgb[2], PRVM_G_FLOAT(OFS_PARM4), flag, NULL, true, getdrawfont(prog));
	//Font_DrawString(pos[0], pos[1], string, 0, scale[0], scale[1], rgb[0], rgb[1], rgb[2], PRVM_G_FLOAT(OFS_PARM4), flag, NULL, true);
	PRVM_G_FLOAT(OFS_RETURN) = 1;
}

/*
=========
VM_drawcolorcodedstring

float	drawcolorcodedstring(vector position, string text, vector scale, float alpha, float flag)
/
float	drawcolorcodedstring(vector position, string text, vector scale, vector rgb, float alpha, float flag)
=========
*/
void VM_drawcolorcodedstring(prvm_prog_t *prog)
{
	prvm_vec_t *pos, *scale;
	const char  *string;
	int flag;
	vec3_t rgb;
	float sx, sy, alpha;

	VM_SAFEPARMCOUNTRANGE(5,6,VM_drawcolorcodedstring);

	// polygonbegin without draw2d arg has to guess
	prog->polygonbegin_guess2d = true;

	if (prog->argc == 6) // full 6 parms, like normal drawstring
	{
		pos = PRVM_G_VECTOR(OFS_PARM0);
		string = PRVM_G_STRING(OFS_PARM1);
		scale = PRVM_G_VECTOR(OFS_PARM2);
		VectorCopy(PRVM_G_VECTOR(OFS_PARM3), rgb); 
		alpha = PRVM_G_FLOAT(OFS_PARM4);
		flag = (int)PRVM_G_FLOAT(OFS_PARM5);
	}
	else
	{
		pos = PRVM_G_VECTOR(OFS_PARM0);
		string = PRVM_G_STRING(OFS_PARM1);
		scale = PRVM_G_VECTOR(OFS_PARM2);
		rgb[0] = 1.0;
		rgb[1] = 1.0;
		rgb[2] = 1.0;
		alpha = PRVM_G_FLOAT(OFS_PARM3);
		flag = (int)PRVM_G_FLOAT(OFS_PARM4);
	}

	if(flag < DRAWFLAG_NORMAL || flag >= DRAWFLAG_NUMFLAGS)
	{
		PRVM_G_FLOAT(OFS_RETURN) = -2;
		VM_Warning(prog, "VM_drawcolorcodedstring: wrong DRAWFLAG %i !\n", flag);
		return;
	}

	if(!scale[0] || !scale[1])
	{
		PRVM_G_FLOAT(OFS_RETURN) = -3;
		VM_Warning(prog, "VM_drawcolorcodedstring: scale %s is null !\n", (scale[0] == 0) ? ((scale[1] == 0) ? "x and y" : "x") : "y");
		return;
	}

	if(pos[2] || scale[2])
		VM_Warning(prog, "VM_drawcolorcodedstring: z value%s from %s discarded\n",(pos[2] && scale[2]) ? "s" : " ",((pos[2] && scale[2]) ? "pos and scale" : (pos[2] ? "pos" : "scale")));

	getdrawfontscale(prog, &sx, &sy);
	DrawQ_String_Scale(pos[0], pos[1], string, 0, scale[0], scale[1], sx, sy, rgb[0], rgb[1], rgb[2], alpha, flag, NULL, false, getdrawfont(prog));
	if (prog->argc == 6) // also return vector of last color
		VectorCopy(DrawQ_Color, PRVM_G_VECTOR(OFS_RETURN));
	else
		PRVM_G_FLOAT(OFS_RETURN) = 1;
}
/*
=========
VM_stringwidth

float	stringwidth(string text, float allowColorCodes, float size)
=========
*/
void VM_stringwidth(prvm_prog_t *prog)
{
	const char  *string;
	vec2_t szv;
	float mult; // sz is intended font size so we can later add freetype support, mult is font size multiplier in pixels per character cell
	int colors;
	float sx, sy;
	size_t maxlen = 0;
	VM_SAFEPARMCOUNTRANGE(2, 3, VM_stringwidth);

	getdrawfontscale(prog, &sx, &sy);
	if(prog->argc == 3)
	{
		Vector2Copy(PRVM_G_VECTOR(OFS_PARM2), szv);
		mult = 1;
	}
	else
	{
		// we want the width for 8x8 font size, divided by 8
		Vector2Set(szv, 8, 8);
		mult = 0.125;
		// to make sure snapping is turned off, ALWAYS use a nontrivial scale in this case
		if(sx >= 0.9 && sx <= 1.1)
		{
			mult *= 2;
			sx /= 2;
			sy /= 2;
		}
	}

	string = PRVM_G_STRING(OFS_PARM0);
	colors = (int)PRVM_G_FLOAT(OFS_PARM1);

	PRVM_G_FLOAT(OFS_RETURN) = DrawQ_TextWidth_UntilWidth_TrackColors_Scale(string, &maxlen, szv[0], szv[1], sx, sy, NULL, !colors, getdrawfont(prog), 1000000000) * mult;
/*
	if(prog->argc == 3)
	{
		mult = sz = PRVM_G_FLOAT(OFS_PARM2);
	}
	else
	{
		sz = 8;
		mult = 1;
	}

	string = PRVM_G_STRING(OFS_PARM0);
	colors = (int)PRVM_G_FLOAT(OFS_PARM1);

	PRVM_G_FLOAT(OFS_RETURN) = DrawQ_TextWidth(string, 0, !colors, getdrawfont()) * mult; // 1x1 characters, don't actually draw
*/
}

/*
=========
VM_findfont

float findfont(string s)
=========
*/

static float getdrawfontnum(const char *fontname)
{
	int i;

	for(i = 0; i < dp_fonts.maxsize; ++i)
		if(!strcmp(dp_fonts.f[i].title, fontname))
			return i;
	return -1;
}

void VM_findfont(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(1,VM_findfont);
	PRVM_G_FLOAT(OFS_RETURN) = getdrawfontnum(PRVM_G_STRING(OFS_PARM0));
}

/*
=========
VM_loadfont

float loadfont(string fontname, string fontmaps, string sizes, float slot)
=========
*/

void VM_loadfont(prvm_prog_t *prog)
{
	const char *fontname, *filelist, *sizes, *c, *cm;
	char mainfont[MAX_QPATH];
	int i, numsizes;
	float sz, scale, voffset;
	dp_font_t *f;

	VM_SAFEPARMCOUNTRANGE(3,6,VM_loadfont);

	fontname = PRVM_G_STRING(OFS_PARM0);
	if (!fontname[0])
		fontname = "default";

	filelist = PRVM_G_STRING(OFS_PARM1);
	if (!filelist[0])
		filelist = "gfx/conchars";

	sizes = PRVM_G_STRING(OFS_PARM2);
	if (!sizes[0])
		sizes = "10";

	// find a font
	f = NULL;
	if (prog->argc >= 4)
	{
		i = PRVM_G_FLOAT(OFS_PARM3);
		if (i >= 0 && i < dp_fonts.maxsize)
		{
			f = &dp_fonts.f[i];
			dp_strlcpy(f->title, fontname, sizeof(f->title)); // replace name
		}
	}
	if (!f)
		f = FindFont(fontname, true);
	if (!f)
	{
		PRVM_G_FLOAT(OFS_RETURN) = -1;
		return; // something go wrong
	}

	memset(f->fallbacks, 0, sizeof(f->fallbacks));
	memset(f->fallback_faces, 0, sizeof(f->fallback_faces));

	// first font is handled "normally"
	c = strchr(filelist, ':');
	cm = strchr(filelist, ',');
	if(c && (!cm || c < cm))
		f->req_face = atoi(c+1);
	else
	{
		f->req_face = 0;
		c = cm;
	}
	if(!c || (c - filelist) >= MAX_QPATH)
		dp_strlcpy(mainfont, filelist, sizeof(mainfont));
	else
	{
		memcpy(mainfont, filelist, c - filelist);
		mainfont[c - filelist] = 0;
	}

	// handle fallbacks
	for(i = 0; i < MAX_FONT_FALLBACKS; ++i)
	{
		c = strchr(filelist, ',');
		if(!c)
			break;
		filelist = c + 1;
		if(!*filelist)
			break;
		c = strchr(filelist, ':');
		cm = strchr(filelist, ',');
		if(c && (!cm || c < cm))
			f->fallback_faces[i] = atoi(c+1);
		else
		{
			f->fallback_faces[i] = 0; // f->req_face; could make it stick to the default-font's face index
			c = cm;
		}
		if(!c || (c-filelist) >= MAX_QPATH)
		{
			dp_strlcpy(f->fallbacks[i], filelist, sizeof(mainfont));
		}
		else
		{
			memcpy(f->fallbacks[i], filelist, c - filelist);
			f->fallbacks[i][c - filelist] = 0;
		}
	}

	// handle sizes
	for(i = 0; i < MAX_FONT_SIZES; ++i)
		f->req_sizes[i] = -1;
	for (numsizes = 0,c = sizes;;)
	{
		if (!COM_ParseToken_VM_Tokenize(&c, 0))
			break;
		sz = atof(com_token);
		// detect crap size
		if (sz < 0.001f || sz > 1000.0f)
		{
			VM_Warning(prog, "VM_loadfont: crap size %s", com_token);
			continue;
		}
		// check overflow
		if (numsizes == MAX_FONT_SIZES)
		{
			VM_Warning(prog, "VM_loadfont: MAX_FONT_SIZES = %i exceeded", MAX_FONT_SIZES);
			break;
		}
		f->req_sizes[numsizes] = sz;
		numsizes++;
	}

	// additional scale/hoffset parms
	scale = 1;
	voffset = 0;
	if (prog->argc >= 5)
	{
		scale = PRVM_G_FLOAT(OFS_PARM4);
		if (scale <= 0)
			scale = 1;
	}
	if (prog->argc >= 6)
		voffset = PRVM_G_FLOAT(OFS_PARM5);

	// load
	LoadFont(true, mainfont, f, scale, voffset);

	// return index of loaded font
	PRVM_G_FLOAT(OFS_RETURN) = (f - dp_fonts.f);
}

/*
=========
VM_drawpic

float	drawpic(vector position, string pic, vector size, vector rgb, float alpha, float flag)
=========
*/
void VM_drawpic(prvm_prog_t *prog)
{
	const char *picname;
	prvm_vec_t *size, *pos, *rgb;
	int flag = 0;

	VM_SAFEPARMCOUNTRANGE(5,6,VM_drawpic);

	// polygonbegin without draw2d arg has to guess
	prog->polygonbegin_guess2d = true;

	picname = PRVM_G_STRING(OFS_PARM1);
	VM_CheckEmptyString(prog, picname);

	// is pic cached ? no function yet for that
	if(!1)
	{
		PRVM_G_FLOAT(OFS_RETURN) = -4;
		VM_Warning(prog, "VM_drawpic: %s not cached !\n", picname);
		return;
	}

	pos = PRVM_G_VECTOR(OFS_PARM0);
	size = PRVM_G_VECTOR(OFS_PARM2);
	rgb = PRVM_G_VECTOR(OFS_PARM3);
	if (prog->argc >= 6)
		flag = (int) PRVM_G_FLOAT(OFS_PARM5);

	if(flag < DRAWFLAG_NORMAL || flag >=DRAWFLAG_NUMFLAGS)
	{
		PRVM_G_FLOAT(OFS_RETURN) = -2;
		VM_Warning(prog, "VM_drawpic: wrong DRAWFLAG %i !\n", flag);
		return;
	}

	if(pos[2] || size[2])
		VM_Warning(prog, "VM_drawpic: z value%s from %s discarded\n",(pos[2] && size[2]) ? "s" : " ",((pos[2] && size[2]) ? "pos and size" : (pos[2] ? "pos" : "size")));

	DrawQ_Pic(pos[0], pos[1], Draw_CachePic_Flags (picname, CACHEPICFLAG_NOTPERSISTENT), size[0], size[1], rgb[0], rgb[1], rgb[2], PRVM_G_FLOAT(OFS_PARM4), flag);
	PRVM_G_FLOAT(OFS_RETURN) = 1;
}
/*
=========
VM_drawrotpic

float	drawrotpic(vector position, string pic, vector size, vector org, float angle, vector rgb, float alpha, float flag)
=========
*/
void VM_drawrotpic(prvm_prog_t *prog)
{
	const char *picname;
	prvm_vec_t *size, *pos, *org, *rgb;
	int flag;

	VM_SAFEPARMCOUNT(8,VM_drawrotpic);

	// polygonbegin without draw2d arg has to guess
	prog->polygonbegin_guess2d = true;

	picname = PRVM_G_STRING(OFS_PARM1);
	VM_CheckEmptyString(prog, picname);

	// is pic cached ? no function yet for that
	if(!1)
	{
		PRVM_G_FLOAT(OFS_RETURN) = -4;
		VM_Warning(prog, "VM_drawrotpic: %s not cached !\n", picname);
		return;
	}

	pos = PRVM_G_VECTOR(OFS_PARM0);
	size = PRVM_G_VECTOR(OFS_PARM2);
	org = PRVM_G_VECTOR(OFS_PARM3);
	rgb = PRVM_G_VECTOR(OFS_PARM5);
	flag = (int) PRVM_G_FLOAT(OFS_PARM7);

	if(flag < DRAWFLAG_NORMAL || flag >=DRAWFLAG_NUMFLAGS)
	{
		PRVM_G_FLOAT(OFS_RETURN) = -2;
		VM_Warning(prog, "VM_drawrotpic: wrong DRAWFLAG %i !\n", flag);
		return;
	}

	if(pos[2] || size[2] || org[2])
		VM_Warning(prog, "VM_drawrotpic: z value from pos/size/org discarded\n");

	DrawQ_RotPic(pos[0], pos[1], Draw_CachePic_Flags(picname, CACHEPICFLAG_NOTPERSISTENT), size[0], size[1], org[0], org[1], PRVM_G_FLOAT(OFS_PARM4), rgb[0], rgb[1], rgb[2], PRVM_G_FLOAT(OFS_PARM6), flag);
	PRVM_G_FLOAT(OFS_RETURN) = 1;
}
/*
=========
VM_drawsubpic

float	drawsubpic(vector position, vector size, string pic, vector srcPos, vector srcSize, vector rgb, float alpha, float flag)

=========
*/
void VM_drawsubpic(prvm_prog_t *prog)
{
	const char *picname;
	prvm_vec_t *size, *pos, *rgb, *srcPos, *srcSize, alpha;
	int flag;

	VM_SAFEPARMCOUNT(8,VM_drawsubpic);

	// polygonbegin without draw2d arg has to guess
	prog->polygonbegin_guess2d = true;

	picname = PRVM_G_STRING(OFS_PARM2);
	VM_CheckEmptyString(prog, picname);

	// is pic cached ? no function yet for that
	if(!1)
	{
		PRVM_G_FLOAT(OFS_RETURN) = -4;
		VM_Warning(prog, "VM_drawsubpic: %s not cached !\n", picname);
		return;
	}

	pos = PRVM_G_VECTOR(OFS_PARM0);
	size = PRVM_G_VECTOR(OFS_PARM1);
	srcPos = PRVM_G_VECTOR(OFS_PARM3);
	srcSize = PRVM_G_VECTOR(OFS_PARM4);
	rgb = PRVM_G_VECTOR(OFS_PARM5);
	alpha = PRVM_G_FLOAT(OFS_PARM6);
	flag = (int) PRVM_G_FLOAT(OFS_PARM7);

	if(flag < DRAWFLAG_NORMAL || flag >=DRAWFLAG_NUMFLAGS)
	{
		PRVM_G_FLOAT(OFS_RETURN) = -2;
		VM_Warning(prog, "VM_drawsubpic: wrong DRAWFLAG %i !\n", flag);
		return;
	}

	if(pos[2] || size[2])
		VM_Warning(prog, "VM_drawsubpic: z value%s from %s discarded\n",(pos[2] && size[2]) ? "s" : " ",((pos[2] && size[2]) ? "pos and size" : (pos[2] ? "pos" : "size")));

	DrawQ_SuperPic(pos[0], pos[1], Draw_CachePic_Flags (picname, CACHEPICFLAG_NOTPERSISTENT),
		size[0], size[1],
		srcPos[0],              srcPos[1],              rgb[0], rgb[1], rgb[2], alpha,
		srcPos[0] + srcSize[0], srcPos[1],              rgb[0], rgb[1], rgb[2], alpha,
		srcPos[0],              srcPos[1] + srcSize[1], rgb[0], rgb[1], rgb[2], alpha,
		srcPos[0] + srcSize[0], srcPos[1] + srcSize[1], rgb[0], rgb[1], rgb[2], alpha,
		flag);
	PRVM_G_FLOAT(OFS_RETURN) = 1;
}

/*
=========
VM_drawfill

float drawfill(vector position, vector size, vector rgb, float alpha, float flag)
=========
*/
void VM_drawfill(prvm_prog_t *prog)
{
	prvm_vec_t *size, *pos, *rgb;
	int flag;

	VM_SAFEPARMCOUNT(5,VM_drawfill);

	// polygonbegin without draw2d arg has to guess
	prog->polygonbegin_guess2d = true;

	pos = PRVM_G_VECTOR(OFS_PARM0);
	size = PRVM_G_VECTOR(OFS_PARM1);
	rgb = PRVM_G_VECTOR(OFS_PARM2);
	flag = (int) PRVM_G_FLOAT(OFS_PARM4);

	if(flag < DRAWFLAG_NORMAL || flag >=DRAWFLAG_NUMFLAGS)
	{
		PRVM_G_FLOAT(OFS_RETURN) = -2;
		VM_Warning(prog, "VM_drawfill: wrong DRAWFLAG %i !\n", flag);
		return;
	}

	if(pos[2] || size[2])
		VM_Warning(prog, "VM_drawfill: z value%s from %s discarded\n",(pos[2] && size[2]) ? "s" : " ",((pos[2] && size[2]) ? "pos and size" : (pos[2] ? "pos" : "size")));

	DrawQ_Fill(pos[0], pos[1], size[0], size[1], rgb[0], rgb[1], rgb[2], PRVM_G_FLOAT(OFS_PARM3), flag);
	PRVM_G_FLOAT(OFS_RETURN) = 1;
}

/*
=========
VM_drawsetcliparea

drawsetcliparea(float x, float y, float width, float height)
=========
*/
void VM_drawsetcliparea(prvm_prog_t *prog)
{
	float x,y,w,h;
	VM_SAFEPARMCOUNT(4,VM_drawsetcliparea);

	// polygonbegin without draw2d arg has to guess
	prog->polygonbegin_guess2d = true;

	x = bound(0, PRVM_G_FLOAT(OFS_PARM0), vid_conwidth.integer);
	y = bound(0, PRVM_G_FLOAT(OFS_PARM1), vid_conheight.integer);
	w = bound(0, PRVM_G_FLOAT(OFS_PARM2) + PRVM_G_FLOAT(OFS_PARM0) - x, (vid_conwidth.integer  - x));
	h = bound(0, PRVM_G_FLOAT(OFS_PARM3) + PRVM_G_FLOAT(OFS_PARM1) - y, (vid_conheight.integer - y));

	DrawQ_SetClipArea(x, y, w, h);
}

/*
=========
VM_drawresetcliparea

drawresetcliparea()
=========
*/
void VM_drawresetcliparea(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(0,VM_drawresetcliparea);

	// polygonbegin without draw2d arg has to guess
	prog->polygonbegin_guess2d = true;

	DrawQ_ResetClipArea();
}

/*
=========
VM_getimagesize

vector	getimagesize(string pic)
=========
*/
void VM_getimagesize(prvm_prog_t *prog)
{
	const char *p;
	cachepic_t *pic;

	VM_SAFEPARMCOUNT(1,VM_getimagesize);

	p = PRVM_G_STRING(OFS_PARM0);
	VM_CheckEmptyString(prog, p);

	pic = Draw_CachePic_Flags (p, CACHEPICFLAG_QUIET | CACHEPICFLAG_NOTPERSISTENT);
	if (!Draw_IsPicLoaded(pic))
	{
		PRVM_G_VECTOR(OFS_RETURN)[0] = 0;
		PRVM_G_VECTOR(OFS_RETURN)[1] = 0;
	}
	else
	{
		PRVM_G_VECTOR(OFS_RETURN)[0] = Draw_GetPicWidth(pic);
		PRVM_G_VECTOR(OFS_RETURN)[1] = Draw_GetPicHeight(pic);
	}
	PRVM_G_VECTOR(OFS_RETURN)[2] = 0;
}

//#330 float(float stnum) getstatf (EXT_CSQC)
static void VM_CL_getstatf (prvm_prog_t *prog)
{
	int i;
	union
	{
		float f;
		int l;
	}dat;
	VM_SAFEPARMCOUNT(1, VM_CL_getstatf);
	i = (int)PRVM_G_FLOAT(OFS_PARM0);
	if(i < 0 || i >= MAX_CL_STATS)
	{
		PRVM_G_FLOAT(OFS_RETURN) = 0;
		VM_Warning(prog, "VM_CL_getstatf: index>=MAX_CL_STATS or index<0\n");
		return;
	}
	dat.l = cl.stats[i];
	PRVM_G_FLOAT(OFS_RETURN) =  dat.f;
}

//#331 float(float stnum) getstati (EXT_CSQC)
static void VM_CL_getstati (prvm_prog_t *prog)
{
	int index, firstbit, bitcount;

	VM_SAFEPARMCOUNTRANGE(1, 3, VM_CL_getstati);

	index = (int)PRVM_G_FLOAT(OFS_PARM0);
	if(index < 0 || index >= MAX_CL_STATS)
	{
		PRVM_G_FLOAT(OFS_RETURN) = 0;
		VM_Warning(prog, "VM_CL_getstati: index(%i) is >=MAX_CL_STATS(%i) or <0\n", index, MAX_CL_STATS);
		return;
	}

	if (prog->argc > 1)
	{
		firstbit = (int)PRVM_G_FLOAT(OFS_PARM1);
		if (prog->argc > 2)
			bitcount = (int)PRVM_G_FLOAT(OFS_PARM2);
		else
			bitcount = 1;
	}
	else
	{
		firstbit = 0;
		bitcount = 32;
	}

	if (bitcount < 32)	//32 causes the mask to overflow, so there's nothing to subtract from.
		PRVM_G_FLOAT(OFS_RETURN) = cl.stats[index]>>firstbit & ((1<<bitcount)-1);
	else
		PRVM_G_FLOAT(OFS_RETURN) = cl.stats[index];
}

//#332 string(float firststnum) getstats (EXT_CSQC)
static void VM_CL_getstats (prvm_prog_t *prog)
{
	int i;
	char t[17];
	size_t t_len;
	VM_SAFEPARMCOUNT(1, VM_CL_getstats);
	i = (int)PRVM_G_FLOAT(OFS_PARM0);
	if(i < 0 || i > MAX_CL_STATS-4)
	{
		PRVM_G_INT(OFS_RETURN) = OFS_NULL;
		VM_Warning(prog, "VM_CL_getstats: index>MAX_CL_STATS-4 or index<0\n");
		return;
	}
	t_len = dp_strlcpy(t, (char*)&cl.stats[i], sizeof(t));
	PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, t, t_len);
}

//#333 void(entity e, float mdlindex) setmodelindex (EXT_CSQC)
static void VM_CL_setmodelindex (prvm_prog_t *prog)
{
	int				i;
	prvm_edict_t	*t;
	struct model_s	*model;

	VM_SAFEPARMCOUNT(2, VM_CL_setmodelindex);

	t = PRVM_G_EDICT(OFS_PARM0);

	i = (int)PRVM_G_FLOAT(OFS_PARM1);

	PRVM_clientedictstring(t, model) = 0;
	PRVM_clientedictfloat(t, modelindex) = 0;

	if (!i)
		return;

	model = CL_GetModelByIndex(i);
	if (!model)
	{
		VM_Warning(prog, "VM_CL_setmodelindex: null model\n");
		return;
	}
	PRVM_clientedictstring(t, model) = PRVM_SetEngineString(prog, model->name);
	PRVM_clientedictfloat(t, modelindex) = i;

	// TODO: check if this breaks needed consistency and maybe add a cvar for it too?? [1/10/2008 Black]
	if (model)
	{
		SetMinMaxSize (prog, t, model->normalmins, model->normalmaxs);
	}
	else
		SetMinMaxSize (prog, t, vec3_origin, vec3_origin);
}

//#334 string(float mdlindex) modelnameforindex (EXT_CSQC)
static void VM_CL_modelnameforindex (prvm_prog_t *prog)
{
	model_t *model;

	VM_SAFEPARMCOUNT(1, VM_CL_modelnameforindex);

	PRVM_G_INT(OFS_RETURN) = OFS_NULL;
	model = CL_GetModelByIndex((int)PRVM_G_FLOAT(OFS_PARM0));
	PRVM_G_INT(OFS_RETURN) = model ? PRVM_SetEngineString(prog, model->name) : 0;
}

//#335 float(string effectname) particleeffectnum (EXT_CSQC)
static void VM_CL_particleeffectnum (prvm_prog_t *prog)
{
	int			i;
	VM_SAFEPARMCOUNT(1, VM_CL_particleeffectnum);
	i = CL_ParticleEffectIndexForName(PRVM_G_STRING(OFS_PARM0));
	if (i == 0)
		i = -1;
	PRVM_G_FLOAT(OFS_RETURN) = i;
}

// #336 void(entity ent, float effectnum, vector start, vector end[, float color]) trailparticles (EXT_CSQC)
static void VM_CL_trailparticles (prvm_prog_t *prog)
{
	int				i;
	vec3_t			start, end, velocity;
	prvm_edict_t	*t;
	VM_SAFEPARMCOUNTRANGE(4, 5, VM_CL_trailparticles);

	t = PRVM_G_EDICT(OFS_PARM0);
	i		= (int)PRVM_G_FLOAT(OFS_PARM1);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), start);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM3), end);
	VectorCopy(PRVM_clientedictvector(t, velocity), velocity);

	if (i < 0)
		return;
	CL_ParticleTrail(i, 1, start, end, velocity, velocity, NULL, prog->argc >= 5 ? (int)PRVM_G_FLOAT(OFS_PARM4) : 0, true, true, NULL, NULL, 1);
}

//#337 void(float effectnum, vector origin, vector dir, float count[, float color]) pointparticles (EXT_CSQC)
static void VM_CL_pointparticles (prvm_prog_t *prog)
{
	int			i;
	float n;
	vec3_t f, v;
	VM_SAFEPARMCOUNTRANGE(4, 5, VM_CL_pointparticles);
	i = (int)PRVM_G_FLOAT(OFS_PARM0);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), f);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), v);
	n = PRVM_G_FLOAT(OFS_PARM3);
	if (i < 0)
		return;
	CL_ParticleEffect(i, n, f, f, v, v, NULL, prog->argc >= 5 ? (int)PRVM_G_FLOAT(OFS_PARM4) : 0);
}

//#502 void(float effectnum, entity own, vector origin_from, vector origin_to, vector dir_from, vector dir_to, float count, float extflags) boxparticles (DP_CSQC_BOXPARTICLES)
static void VM_CL_boxparticles (prvm_prog_t *prog)
{
	int effectnum;
	// prvm_edict_t *own;
	vec3_t origin_from, origin_to, dir_from, dir_to;
	float count;
	int flags;
	qbool istrail;
	float tintmins[4], tintmaxs[4], fade;
	VM_SAFEPARMCOUNTRANGE(7, 8, VM_CL_boxparticles);

	effectnum = (int)PRVM_G_FLOAT(OFS_PARM0);
	if (effectnum < 0)
		return;
	// own = PRVM_G_EDICT(OFS_PARM1); // TODO find use for this
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), origin_from);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM3), origin_to  );
	VectorCopy(PRVM_G_VECTOR(OFS_PARM4), dir_from   );
	VectorCopy(PRVM_G_VECTOR(OFS_PARM5), dir_to     );
	count = PRVM_G_FLOAT(OFS_PARM6);
	if(prog->argc >= 8)
		flags = PRVM_G_FLOAT(OFS_PARM7);
	else
		flags = 0;

	Vector4Set(tintmins, 1, 1, 1, 1);
	Vector4Set(tintmaxs, 1, 1, 1, 1);
	fade = 1;
	istrail = false;

	if(flags & 1) // read alpha
	{
		tintmins[3] = PRVM_clientglobalfloat(particles_alphamin);
		tintmaxs[3] = PRVM_clientglobalfloat(particles_alphamax);
	}
	if(flags & 2) // read color
	{
		VectorCopy(PRVM_clientglobalvector(particles_colormin), tintmins);
		VectorCopy(PRVM_clientglobalvector(particles_colormax), tintmaxs);
	}
	if(flags & 4) // read fade
	{
		fade = PRVM_clientglobalfloat(particles_fade);
	}
	if(flags & 128) // draw as trail
	{
		istrail = true;
	}

	if (istrail)
		CL_ParticleTrail(effectnum, count, origin_from, origin_to, dir_from, dir_to, NULL, 0, true, true, tintmins, tintmaxs, fade);
	else
		CL_ParticleBox(effectnum, count, origin_from, origin_to, dir_from, dir_to, NULL, 0, true, true, tintmins, tintmaxs, fade);
}

//#531 void(float pause) setpause
static void VM_CL_setpause(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(1, VM_CL_setpause);
	if(cl.islocalgame)
	{
		if ((int)PRVM_G_FLOAT(OFS_PARM0) != 0)
			host.paused = true;
		else
			host.paused = false;
	}
}

//#343 void(float usecursor) setcursormode (DP_CSQC)
static void VM_CL_setcursormode (prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(1, VM_CL_setcursormode);
	cl.csqc_wantsmousemove = PRVM_G_FLOAT(OFS_PARM0) != 0;
	cl_ignoremousemoves = 2;
}

//#344 vector() getmousepos (DP_CSQC)
static void VM_CL_getmousepos(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(0,VM_CL_getmousepos);

	if (key_consoleactive || key_dest != key_game)
		VectorSet(PRVM_G_VECTOR(OFS_RETURN), 0, 0, 0);
	else if (cl.csqc_wantsmousemove)
		VectorSet(PRVM_G_VECTOR(OFS_RETURN), in_windowmouse_x * vid_conwidth.integer / vid.mode.width, in_windowmouse_y * vid_conheight.integer / vid.mode.height, 0);
	else
		VectorSet(PRVM_G_VECTOR(OFS_RETURN), in_mouse_x * vid_conwidth.integer / vid.mode.width, in_mouse_y * vid_conheight.integer / vid.mode.height, 0);
}

//#345 float(float framenum) getinputstate (EXT_CSQC)
static void VM_CL_getinputstate (prvm_prog_t *prog)
{
	unsigned int i, frame;
	VM_SAFEPARMCOUNT(1, VM_CL_getinputstate);
	frame = (unsigned int)PRVM_G_FLOAT(OFS_PARM0);
	PRVM_G_FLOAT(OFS_RETURN) = false;
	for (i = 0;i < CL_MAX_USERCMDS;i++)
	{
		if (cl.movecmd[i].sequence == frame)
		{
			VectorCopy(cl.movecmd[i].viewangles, PRVM_clientglobalvector(input_angles));
			PRVM_clientglobalfloat(input_buttons) = cl.movecmd[i].buttons; // FIXME: this should not be directly exposed to csqc (translation layer needed?)
			PRVM_clientglobalvector(input_movevalues)[0] = cl.movecmd[i].forwardmove;
			PRVM_clientglobalvector(input_movevalues)[1] = cl.movecmd[i].sidemove;
			PRVM_clientglobalvector(input_movevalues)[2] = cl.movecmd[i].upmove;
			PRVM_clientglobalfloat(input_timelength) = cl.movecmd[i].frametime;
			// this probably shouldn't be here
			if(cl.movecmd[i].crouch)
			{
				VectorCopy(cl.playercrouchmins, PRVM_clientglobalvector(pmove_mins));
				VectorCopy(cl.playercrouchmaxs, PRVM_clientglobalvector(pmove_maxs));
			}
			else
			{
				VectorCopy(cl.playerstandmins, PRVM_clientglobalvector(pmove_mins));
				VectorCopy(cl.playerstandmaxs, PRVM_clientglobalvector(pmove_maxs));
			}
			PRVM_G_FLOAT(OFS_RETURN) = true;
		}
	}
}

//#346 void(float sens) setsensitivityscaler (EXT_CSQC)
static void VM_CL_setsensitivityscale (prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(1, VM_CL_setsensitivityscale);
	cl.sensitivityscale = PRVM_G_FLOAT(OFS_PARM0);
}

//#347 void() runstandardplayerphysics (EXT_CSQC)
#define PMF_JUMP_HELD 1 // matches FTEQW
#define PMF_LADDER 2 // not used by DP, FTEQW sets this in runplayerphysics but does not read it
#define PMF_DUCKED 4 // FIXME FTEQW doesn't have this for Q1 like movement because Q1 cannot crouch
#define PMF_ONGROUND 8 // FIXME FTEQW doesn't have this for Q1 like movement and expects CSQC code to do its own trace, this is stupid CPU waste
static void VM_CL_runplayerphysics (prvm_prog_t *prog)
{
	cl_clientmovement_state_t s;
	prvm_edict_t *ent;

	memset(&s, 0, sizeof(s));

	VM_SAFEPARMCOUNTRANGE(0, 1, VM_CL_runplayerphysics);

	ent = (prog->argc == 1 ? PRVM_G_EDICT(OFS_PARM0) : prog->edicts);
	if(ent == prog->edicts)
	{
		// deprecated use
		s.self = NULL;
		VectorCopy(PRVM_clientglobalvector(pmove_org), s.origin);
		VectorCopy(PRVM_clientglobalvector(pmove_vel), s.velocity);
		VectorCopy(PRVM_clientglobalvector(pmove_mins), s.mins);
		VectorCopy(PRVM_clientglobalvector(pmove_maxs), s.maxs);
		s.crouched = 0;
		s.waterjumptime = PRVM_clientglobalfloat(pmove_waterjumptime);
		s.cmd.canjump = (int)PRVM_clientglobalfloat(pmove_jump_held) == 0;
	}
	else
	{
		// new use
		s.self = ent;
		VectorCopy(PRVM_clientedictvector(ent, origin), s.origin);
		VectorCopy(PRVM_clientedictvector(ent, velocity), s.velocity);
		VectorCopy(PRVM_clientedictvector(ent, mins), s.mins);
		VectorCopy(PRVM_clientedictvector(ent, maxs), s.maxs);
		s.crouched = ((int)PRVM_clientedictfloat(ent, pmove_flags) & PMF_DUCKED) != 0;
		s.waterjumptime = 0; // FIXME where do we get this from? FTEQW lacks support for this too
		s.cmd.canjump = ((int)PRVM_clientedictfloat(ent, pmove_flags) & PMF_JUMP_HELD) == 0;
	}

	VectorCopy(PRVM_clientglobalvector(input_angles), s.cmd.viewangles);
	s.cmd.forwardmove = PRVM_clientglobalvector(input_movevalues)[0];
	s.cmd.sidemove = PRVM_clientglobalvector(input_movevalues)[1];
	s.cmd.upmove = PRVM_clientglobalvector(input_movevalues)[2];
	s.cmd.buttons = PRVM_clientglobalfloat(input_buttons);
	s.cmd.frametime = PRVM_clientglobalfloat(input_timelength);
	s.cmd.jump = (s.cmd.buttons & 2) != 0;
	s.cmd.crouch = (s.cmd.buttons & 16) != 0;

	CL_ClientMovement_PlayerMove_Frame(&s);

	if(ent == prog->edicts)
	{
		// deprecated use
		VectorCopy(s.origin, PRVM_clientglobalvector(pmove_org));
		VectorCopy(s.velocity, PRVM_clientglobalvector(pmove_vel));
		PRVM_clientglobalfloat(pmove_jump_held) = !s.cmd.canjump;
		PRVM_clientglobalfloat(pmove_waterjumptime) = s.waterjumptime;
	}
	else
	{
		// new use
		VectorCopy(s.origin, PRVM_clientedictvector(ent, origin));
		VectorCopy(s.velocity, PRVM_clientedictvector(ent, velocity));
		PRVM_clientedictfloat(ent, pmove_flags) =
			(s.crouched ? PMF_DUCKED : 0) |
			(s.cmd.canjump ? 0 : PMF_JUMP_HELD) |
			(s.onground ? PMF_ONGROUND : 0);
	}
}

//#348 string(float playernum, string keyname) getplayerkeyvalue (EXT_CSQC)
static void VM_CL_getplayerkey (prvm_prog_t *prog)
{
	int i;
	char t[128];
	size_t t_len;
	const char *c;

	VM_SAFEPARMCOUNT(2, VM_CL_getplayerkey);

	i = (int)PRVM_G_FLOAT(OFS_PARM0);
	c = PRVM_G_STRING(OFS_PARM1);
	PRVM_G_INT(OFS_RETURN) = OFS_NULL;
	Sbar_SortFrags();

	if (i < 0)
		i = Sbar_GetSortedPlayerIndex(-1-i);
	if(i < 0 || i >= cl.maxclients)
		return;

	t[0] = 0;
	t_len = 0;

	if(!strcasecmp(c, "name"))
		t_len = dp_strlcpy(t, cl.scores[i].name, sizeof(t));
	else
		if(!strcasecmp(c, "frags"))
			t_len = dpsnprintf(t, sizeof(t), "%i", cl.scores[i].frags);
	else
		if(!strcasecmp(c, "ping"))
			t_len = dpsnprintf(t, sizeof(t), "%i", cl.scores[i].qw_ping);
	else
		if(!strcasecmp(c, "pl"))
			t_len = dpsnprintf(t, sizeof(t), "%i", cl.scores[i].qw_packetloss);
	else
		if(!strcasecmp(c, "movementloss"))
			t_len = dpsnprintf(t, sizeof(t), "%i", cl.scores[i].qw_movementloss);
	else
		if(!strcasecmp(c, "entertime"))
			t_len = dpsnprintf(t, sizeof(t), "%f", cl.scores[i].qw_entertime);
	else
		if(!strcasecmp(c, "colors"))
			t_len = dpsnprintf(t, sizeof(t), "%i", cl.scores[i].colors);
	else
		if(!strcasecmp(c, "topcolor"))
			t_len = dpsnprintf(t, sizeof(t), "%i", cl.scores[i].colors & 0xf0);
	else
		if(!strcasecmp(c, "bottomcolor"))
			t_len = dpsnprintf(t, sizeof(t), "%i", (cl.scores[i].colors &15)<<4);
	else
		if(!strcasecmp(c, "viewentity"))
			t_len = dpsnprintf(t, sizeof(t), "%i", i+1);
	if(!t[0])
		return;
	PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, t, t_len);
}

//#351 void(vector origin, vector forward, vector right, vector up) SetListener (EXT_CSQC)
static void VM_CL_setlistener (prvm_prog_t *prog)
{
	vec3_t origin, forward, left, up;
	VM_SAFEPARMCOUNT(4, VM_CL_setlistener);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), origin);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), forward);
	VectorNegate(PRVM_G_VECTOR(OFS_PARM2), left);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM3), up);
	Matrix4x4_FromVectors(&cl.csqc_listenermatrix, forward, left, up, origin);
	cl.csqc_usecsqclistener = true;	//use csqc listener at this frame
}

//#352 void(string cmdname) registercommand (EXT_CSQC)
static void VM_CL_registercmd (prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(1, VM_CL_registercmd);
	Cmd_AddCommand(CF_CLIENT, PRVM_G_STRING(OFS_PARM0), NULL, "console command created by QuakeC");
}

//#360 float() ReadByte (EXT_CSQC)
static void VM_CL_ReadByte (prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(0, VM_CL_ReadByte);
	PRVM_G_FLOAT(OFS_RETURN) = MSG_ReadByte(&cl_message);
}

//#361 float() ReadChar (EXT_CSQC)
static void VM_CL_ReadChar (prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(0, VM_CL_ReadChar);
	PRVM_G_FLOAT(OFS_RETURN) = MSG_ReadChar(&cl_message);
}

//#362 float() ReadShort (EXT_CSQC)
static void VM_CL_ReadShort (prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(0, VM_CL_ReadShort);
	PRVM_G_FLOAT(OFS_RETURN) = MSG_ReadShort(&cl_message);
}

//#363 float() ReadLong (EXT_CSQC)
static void VM_CL_ReadLong (prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(0, VM_CL_ReadLong);
	PRVM_G_FLOAT(OFS_RETURN) = MSG_ReadLong(&cl_message);
}

//#364 float() ReadCoord (EXT_CSQC)
static void VM_CL_ReadCoord (prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(0, VM_CL_ReadCoord);
	PRVM_G_FLOAT(OFS_RETURN) = MSG_ReadCoord(&cl_message, cls.protocol);
}

//#365 float() ReadAngle (EXT_CSQC)
static void VM_CL_ReadAngle (prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(0, VM_CL_ReadAngle);
	PRVM_G_FLOAT(OFS_RETURN) = MSG_ReadAngle(&cl_message, cls.protocol);
}

//#366 string() ReadString (EXT_CSQC)
static void VM_CL_ReadString (prvm_prog_t *prog)
{
	size_t cl_readstring_len;

	VM_SAFEPARMCOUNT(0, VM_CL_ReadString);
	cl_readstring_len = MSG_ReadString_len(&cl_message, cl_readstring, sizeof(cl_readstring));
	PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, cl_readstring, cl_readstring_len);
}

//#367 float() ReadFloat (EXT_CSQC)
static void VM_CL_ReadFloat (prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(0, VM_CL_ReadFloat);
	PRVM_G_FLOAT(OFS_RETURN) = MSG_ReadFloat(&cl_message);
}

//#501 string() ReadPicture (DP_CSQC_READWRITEPICTURE)
extern cvar_t cl_readpicture_force;
static void VM_CL_ReadPicture (prvm_prog_t *prog)
{
	const char *name;
	size_t name_len;
	unsigned char *data;
	unsigned char *buf;
	unsigned short size;
	int i;
	cachepic_t *pic;

	VM_SAFEPARMCOUNT(0, VM_CL_ReadPicture);

	name_len = MSG_ReadString_len(&cl_message, cl_readstring, sizeof(cl_readstring));
	name = cl_readstring;
	size = (unsigned short) MSG_ReadShort(&cl_message);

	// check if a texture of that name exists
	// if yes, it is used and the data is discarded
	// if not, the (low quality) data is used to build a new texture, whose name will get returned

	pic = Draw_CachePic_Flags(name, CACHEPICFLAG_NOTPERSISTENT | CACHEPICFLAG_FAILONMISSING);

	if(size)
	{
		if (Draw_IsPicLoaded(pic) && !cl_readpicture_force.integer)
		{
			// texture found and loaded
			// skip over the jpeg as we don't need it
			for(i = 0; i < size; ++i)
				(void) MSG_ReadByte(&cl_message);
		}
		else
		{
			// texture not found
			// use the attached jpeg as texture
			buf = (unsigned char *) Mem_Alloc(tempmempool, size);
			MSG_ReadBytes(&cl_message, size, buf);
			data = JPEG_LoadImage_BGRA(buf, size, NULL);
			Mem_Free(buf);
			Draw_NewPic(name, image_width, image_height, data, TEXTYPE_BGRA, TEXF_CLAMP);
			Mem_Free(data);
		}
	}

	PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, name, name_len);
}

//////////////////////////////////////////////////////////

static void VM_CL_makestatic (prvm_prog_t *prog)
{
	prvm_edict_t *ent;

	VM_SAFEPARMCOUNT(1, VM_CL_makestatic);

	ent = PRVM_G_EDICT(OFS_PARM0);
	if (ent == prog->edicts)
	{
		VM_Warning(prog, "makestatic: can not modify world entity\n");
		return;
	}
	if (ent->free)
	{
		VM_Warning(prog, "makestatic: can not modify free entity\n");
		return;
	}

	if (cl.num_static_entities < cl.max_static_entities)
	{
		int renderflags;
		entity_t *staticent = &cl.static_entities[cl.num_static_entities++];

		// copy it to the current state
		memset(staticent, 0, sizeof(*staticent));
		staticent->render.model = CL_GetModelByIndex((int)PRVM_clientedictfloat(ent, modelindex));
		staticent->render.framegroupblend[0].frame = (int)PRVM_clientedictfloat(ent, frame);
		staticent->render.framegroupblend[0].lerp = 1;
		// make torchs play out of sync
		staticent->render.framegroupblend[0].start = lhrandom(-10, -1);
		staticent->render.skinnum = (int)PRVM_clientedictfloat(ent, skin);
		staticent->render.effects = (int)PRVM_clientedictfloat(ent, effects);
		staticent->render.alpha = PRVM_clientedictfloat(ent, alpha);
		staticent->render.scale = PRVM_clientedictfloat(ent, scale);
		VectorCopy(PRVM_clientedictvector(ent, colormod), staticent->render.colormod);
		VectorCopy(PRVM_clientedictvector(ent, glowmod), staticent->render.glowmod);

		// sanitize values
		if (!staticent->render.alpha)
			staticent->render.alpha = 1.0f;
		if (!staticent->render.scale)
			staticent->render.scale = 1.0f;
		if (!VectorLength2(staticent->render.colormod))
			VectorSet(staticent->render.colormod, 1, 1, 1);
		if (!VectorLength2(staticent->render.glowmod))
			VectorSet(staticent->render.glowmod, 1, 1, 1);

		renderflags = (int)PRVM_clientedictfloat(ent, renderflags);
		if (renderflags & RF_USEAXIS)
		{
			vec3_t forward, left, up, origin;
			VectorCopy(PRVM_clientglobalvector(v_forward), forward);
			VectorNegate(PRVM_clientglobalvector(v_right), left);
			VectorCopy(PRVM_clientglobalvector(v_up), up);
			VectorCopy(PRVM_clientedictvector(ent, origin), origin);
			Matrix4x4_FromVectors(&staticent->render.matrix, forward, left, up, origin);
			Matrix4x4_Scale(&staticent->render.matrix, staticent->render.scale, 1);
		}
		else
			Matrix4x4_CreateFromQuakeEntity(&staticent->render.matrix, PRVM_clientedictvector(ent, origin)[0], PRVM_clientedictvector(ent, origin)[1], PRVM_clientedictvector(ent, origin)[2], PRVM_clientedictvector(ent, angles)[0], PRVM_clientedictvector(ent, angles)[1], PRVM_clientedictvector(ent, angles)[2], staticent->render.scale);

		// either fullbright or lit
		if(!r_fullbright.integer)
		{
			if (!(staticent->render.effects & EF_FULLBRIGHT))
				staticent->render.flags |= RENDER_LIGHT;
		}
		// turn off shadows from transparent objects
		if (!(staticent->render.effects & (EF_NOSHADOW | EF_ADDITIVE | EF_NODEPTHTEST)) && (staticent->render.alpha >= 1))
			staticent->render.flags |= RENDER_SHADOW;
		if (staticent->render.effects & EF_NODEPTHTEST)
			staticent->render.flags |= RENDER_NODEPTHTEST;
		if (staticent->render.effects & EF_ADDITIVE)
			staticent->render.flags |= RENDER_ADDITIVE;
		if (staticent->render.effects & EF_DOUBLESIDED)
			staticent->render.flags |= RENDER_DOUBLESIDED;

		staticent->render.allowdecals = true;
		CL_UpdateRenderEntity(&staticent->render);
	}
	else
		Con_Printf("Too many static entities");

// throw the entity away now
	PRVM_ED_Free(prog, ent);
}

//=================================================================//

/*
=================
VM_CL_copyentity

copies data from one entity to another

copyentity(src, dst)
=================
*/
static void VM_CL_copyentity (prvm_prog_t *prog)
{
	prvm_edict_t *in, *out;
	VM_SAFEPARMCOUNT(2, VM_CL_copyentity);
	in = PRVM_G_EDICT(OFS_PARM0);
	if (in == prog->edicts)
	{
		VM_Warning(prog, "copyentity: can not read world entity\n");
		return;
	}
	if (in->free)
	{
		VM_Warning(prog, "copyentity: can not read free entity\n");
		return;
	}
	out = PRVM_G_EDICT(OFS_PARM1);
	if (out == prog->edicts)
	{
		VM_Warning(prog, "copyentity: can not modify world entity\n");
		return;
	}
	if (out->free)
	{
		VM_Warning(prog, "copyentity: can not modify free entity\n");
		return;
	}
	memcpy(out->fields.fp, in->fields.fp, prog->entityfields * sizeof(prvm_vec_t));

	CL_LinkEdict(out);
}

//=================================================================//

// #404 void(vector org, string modelname, float startframe, float endframe, float framerate) effect (DP_SV_EFFECT)
static void VM_CL_effect (prvm_prog_t *prog)
{
	model_t *model;
	vec3_t org;
	VM_SAFEPARMCOUNT(5, VM_CL_effect);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), org);

	model = Mod_FindName(PRVM_G_STRING(OFS_PARM1), NULL);
	if(model->loaded)
		CL_Effect(org, model, (int)PRVM_G_FLOAT(OFS_PARM2), (int)PRVM_G_FLOAT(OFS_PARM3), PRVM_G_FLOAT(OFS_PARM4));
	else
		Con_Printf(CON_ERROR "VM_CL_effect: Could not load model '%s'\n", PRVM_G_STRING(OFS_PARM1));
}

// #405 void(vector org, vector velocity, float howmany) te_blood (DP_TE_BLOOD)
static void VM_CL_te_blood (prvm_prog_t *prog)
{
	vec3_t pos, vel, pos2;
	VM_SAFEPARMCOUNT(3, VM_CL_te_blood);
	if (PRVM_G_FLOAT(OFS_PARM2) < 1)
		return;
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), vel);
	CL_FindNonSolidLocation(pos, pos2, 4);
	CL_ParticleEffect(EFFECT_TE_BLOOD, PRVM_G_FLOAT(OFS_PARM2), pos2, pos2, vel, vel, NULL, 0);
}

// #406 void(vector mincorner, vector maxcorner, float explosionspeed, float howmany) te_bloodshower (DP_TE_BLOODSHOWER)
static void VM_CL_te_bloodshower (prvm_prog_t *prog)
{
	vec_t speed;
	vec3_t mincorner, maxcorner, vel1, vel2;
	VM_SAFEPARMCOUNT(4, VM_CL_te_bloodshower);
	if (PRVM_G_FLOAT(OFS_PARM3) < 1)
		return;
	speed = PRVM_G_FLOAT(OFS_PARM2);
	vel1[0] = -speed;
	vel1[1] = -speed;
	vel1[2] = -speed;
	vel2[0] = speed;
	vel2[1] = speed;
	vel2[2] = speed;
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), mincorner);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), maxcorner);
	CL_ParticleEffect(EFFECT_TE_BLOOD, PRVM_G_FLOAT(OFS_PARM3), mincorner, maxcorner, vel1, vel2, NULL, 0);
}

// #407 void(vector org, vector color) te_explosionrgb (DP_TE_EXPLOSIONRGB)
static void VM_CL_te_explosionrgb (prvm_prog_t *prog)
{
	vec3_t		pos;
	vec3_t		pos2;
	matrix4x4_t	tempmatrix;
	VM_SAFEPARMCOUNT(2, VM_CL_te_explosionrgb);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_FindNonSolidLocation(pos, pos2, 10);
	CL_ParticleExplosion(pos2);
	Matrix4x4_CreateTranslate(&tempmatrix, pos2[0], pos2[1], pos2[2]);
	CL_AllocLightFlash(NULL, &tempmatrix, 350, PRVM_G_VECTOR(OFS_PARM1)[0], PRVM_G_VECTOR(OFS_PARM1)[1], PRVM_G_VECTOR(OFS_PARM1)[2], 700, 0.5, NULL, -1, true, 1, 0.25, 0.25, 1, 1, LIGHTFLAG_NORMALMODE | LIGHTFLAG_REALTIMEMODE);
}

// #408 void(vector mincorner, vector maxcorner, vector vel, float howmany, float color, float gravityflag, float randomveljitter) te_particlecube (DP_TE_PARTICLECUBE)
static void VM_CL_te_particlecube (prvm_prog_t *prog)
{
	vec3_t mincorner, maxcorner, vel;
	VM_SAFEPARMCOUNT(7, VM_CL_te_particlecube);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), mincorner);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), maxcorner);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), vel);
	CL_ParticleCube(mincorner, maxcorner, vel, (int)PRVM_G_FLOAT(OFS_PARM3), (int)PRVM_G_FLOAT(OFS_PARM4), PRVM_G_FLOAT(OFS_PARM5), PRVM_G_FLOAT(OFS_PARM6));
}

// #409 void(vector mincorner, vector maxcorner, vector vel, float howmany, float color) te_particlerain (DP_TE_PARTICLERAIN)
static void VM_CL_te_particlerain (prvm_prog_t *prog)
{
	vec3_t mincorner, maxcorner, vel;
	VM_SAFEPARMCOUNT(5, VM_CL_te_particlerain);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), mincorner);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), maxcorner);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), vel);
	CL_ParticleRain(mincorner, maxcorner, vel, (int)PRVM_G_FLOAT(OFS_PARM3), (int)PRVM_G_FLOAT(OFS_PARM4), 0);
}

// #410 void(vector mincorner, vector maxcorner, vector vel, float howmany, float color) te_particlesnow (DP_TE_PARTICLESNOW)
static void VM_CL_te_particlesnow (prvm_prog_t *prog)
{
	vec3_t mincorner, maxcorner, vel;
	VM_SAFEPARMCOUNT(5, VM_CL_te_particlesnow);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), mincorner);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), maxcorner);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), vel);
	CL_ParticleRain(mincorner, maxcorner, vel, (int)PRVM_G_FLOAT(OFS_PARM3), (int)PRVM_G_FLOAT(OFS_PARM4), 1);
}

// #411 void(vector org, vector vel, float howmany) te_spark
static void VM_CL_te_spark (prvm_prog_t *prog)
{
	vec3_t pos, pos2, vel;
	VM_SAFEPARMCOUNT(3, VM_CL_te_spark);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), vel);
	CL_FindNonSolidLocation(pos, pos2, 4);
	CL_ParticleEffect(EFFECT_TE_SPARK, PRVM_G_FLOAT(OFS_PARM2), pos2, pos2, vel, vel, NULL, 0);
}

extern cvar_t cl_sound_ric_gunshot;
// #412 void(vector org) te_gunshotquad (DP_QUADEFFECTS1)
static void VM_CL_te_gunshotquad (prvm_prog_t *prog)
{
	vec3_t		pos, pos2;
	int			rnd;
	VM_SAFEPARMCOUNT(1, VM_CL_te_gunshotquad);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_FindNonSolidLocation(pos, pos2, 4);
	CL_ParticleEffect(EFFECT_TE_GUNSHOTQUAD, 1, pos2, pos2, vec3_origin, vec3_origin, NULL, 0);
	if(cl_sound_ric_gunshot.integer >= 2)
	{
		if (rand() % 5)			S_StartSound(-1, 0, cl.sfx_tink1, pos2, 1, 1);
		else
		{
			rnd = rand() & 3;
			if (rnd == 1)		S_StartSound(-1, 0, cl.sfx_ric1, pos2, 1, 1);
			else if (rnd == 2)	S_StartSound(-1, 0, cl.sfx_ric2, pos2, 1, 1);
			else				S_StartSound(-1, 0, cl.sfx_ric3, pos2, 1, 1);
		}
	}
}

// #413 void(vector org) te_spikequad (DP_QUADEFFECTS1)
static void VM_CL_te_spikequad (prvm_prog_t *prog)
{
	vec3_t		pos, pos2;
	int			rnd;
	VM_SAFEPARMCOUNT(1, VM_CL_te_spikequad);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_FindNonSolidLocation(pos, pos2, 4);
	CL_ParticleEffect(EFFECT_TE_SPIKEQUAD, 1, pos2, pos2, vec3_origin, vec3_origin, NULL, 0);
	if (rand() % 5)			S_StartSound(-1, 0, cl.sfx_tink1, pos2, 1, 1);
	else
	{
		rnd = rand() & 3;
		if (rnd == 1)		S_StartSound(-1, 0, cl.sfx_ric1, pos2, 1, 1);
		else if (rnd == 2)	S_StartSound(-1, 0, cl.sfx_ric2, pos2, 1, 1);
		else				S_StartSound(-1, 0, cl.sfx_ric3, pos2, 1, 1);
	}
}

// #414 void(vector org) te_superspikequad (DP_QUADEFFECTS1)
static void VM_CL_te_superspikequad (prvm_prog_t *prog)
{
	vec3_t		pos, pos2;
	int			rnd;
	VM_SAFEPARMCOUNT(1, VM_CL_te_superspikequad);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_FindNonSolidLocation(pos, pos2, 4);
	CL_ParticleEffect(EFFECT_TE_SUPERSPIKEQUAD, 1, pos2, pos2, vec3_origin, vec3_origin, NULL, 0);
	if (rand() % 5)			S_StartSound(-1, 0, cl.sfx_tink1, pos, 1, 1);
	else
	{
		rnd = rand() & 3;
		if (rnd == 1)		S_StartSound(-1, 0, cl.sfx_ric1, pos2, 1, 1);
		else if (rnd == 2)	S_StartSound(-1, 0, cl.sfx_ric2, pos2, 1, 1);
		else				S_StartSound(-1, 0, cl.sfx_ric3, pos2, 1, 1);
	}
}

// #415 void(vector org) te_explosionquad (DP_QUADEFFECTS1)
static void VM_CL_te_explosionquad (prvm_prog_t *prog)
{
	vec3_t		pos, pos2;
	VM_SAFEPARMCOUNT(1, VM_CL_te_explosionquad);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_FindNonSolidLocation(pos, pos2, 10);
	CL_ParticleEffect(EFFECT_TE_EXPLOSIONQUAD, 1, pos2, pos2, vec3_origin, vec3_origin, NULL, 0);
	S_StartSound(-1, 0, cl.sfx_r_exp3, pos2, 1, 1);
}

// #416 void(vector org) te_smallflash (DP_TE_SMALLFLASH)
static void VM_CL_te_smallflash (prvm_prog_t *prog)
{
	vec3_t		pos, pos2;
	VM_SAFEPARMCOUNT(1, VM_CL_te_smallflash);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_FindNonSolidLocation(pos, pos2, 10);
	CL_ParticleEffect(EFFECT_TE_SMALLFLASH, 1, pos2, pos2, vec3_origin, vec3_origin, NULL, 0);
}

// #417 void(vector org, float radius, float lifetime, vector color) te_customflash (DP_TE_CUSTOMFLASH)
static void VM_CL_te_customflash (prvm_prog_t *prog)
{
	vec3_t		pos, pos2;
	matrix4x4_t	tempmatrix;
	VM_SAFEPARMCOUNT(4, VM_CL_te_customflash);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_FindNonSolidLocation(pos, pos2, 4);
	Matrix4x4_CreateTranslate(&tempmatrix, pos2[0], pos2[1], pos2[2]);
	CL_AllocLightFlash(NULL, &tempmatrix, PRVM_G_FLOAT(OFS_PARM1), PRVM_G_VECTOR(OFS_PARM3)[0], PRVM_G_VECTOR(OFS_PARM3)[1], PRVM_G_VECTOR(OFS_PARM3)[2], PRVM_G_FLOAT(OFS_PARM1) / PRVM_G_FLOAT(OFS_PARM2), PRVM_G_FLOAT(OFS_PARM2), NULL, -1, true, 1, 0.25, 1, 1, 1, LIGHTFLAG_NORMALMODE | LIGHTFLAG_REALTIMEMODE);
}

// #418 void(vector org) te_gunshot (DP_TE_STANDARDEFFECTBUILTINS)
static void VM_CL_te_gunshot (prvm_prog_t *prog)
{
	vec3_t		pos, pos2;
	int			rnd;
	VM_SAFEPARMCOUNT(1, VM_CL_te_gunshot);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_FindNonSolidLocation(pos, pos2, 4);
	CL_ParticleEffect(EFFECT_TE_GUNSHOT, 1, pos2, pos2, vec3_origin, vec3_origin, NULL, 0);
	if(cl_sound_ric_gunshot.integer == 1 || cl_sound_ric_gunshot.integer == 3)
	{
		if (rand() % 5)			S_StartSound(-1, 0, cl.sfx_tink1, pos2, 1, 1);
		else
		{
			rnd = rand() & 3;
			if (rnd == 1)		S_StartSound(-1, 0, cl.sfx_ric1, pos2, 1, 1);
			else if (rnd == 2)	S_StartSound(-1, 0, cl.sfx_ric2, pos2, 1, 1);
			else				S_StartSound(-1, 0, cl.sfx_ric3, pos2, 1, 1);
		}
	}
}

// #419 void(vector org) te_spike (DP_TE_STANDARDEFFECTBUILTINS)
static void VM_CL_te_spike (prvm_prog_t *prog)
{
	vec3_t		pos, pos2;
	int			rnd;
	VM_SAFEPARMCOUNT(1, VM_CL_te_spike);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_FindNonSolidLocation(pos, pos2, 4);
	CL_ParticleEffect(EFFECT_TE_SPIKE, 1, pos2, pos2, vec3_origin, vec3_origin, NULL, 0);
	if (rand() % 5)			S_StartSound(-1, 0, cl.sfx_tink1, pos2, 1, 1);
	else
	{
		rnd = rand() & 3;
		if (rnd == 1)		S_StartSound(-1, 0, cl.sfx_ric1, pos2, 1, 1);
		else if (rnd == 2)	S_StartSound(-1, 0, cl.sfx_ric2, pos2, 1, 1);
		else				S_StartSound(-1, 0, cl.sfx_ric3, pos2, 1, 1);
	}
}

// #420 void(vector org) te_superspike (DP_TE_STANDARDEFFECTBUILTINS)
static void VM_CL_te_superspike (prvm_prog_t *prog)
{
	vec3_t		pos, pos2;
	int			rnd;
	VM_SAFEPARMCOUNT(1, VM_CL_te_superspike);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_FindNonSolidLocation(pos, pos2, 4);
	CL_ParticleEffect(EFFECT_TE_SUPERSPIKE, 1, pos2, pos2, vec3_origin, vec3_origin, NULL, 0);
	if (rand() % 5)			S_StartSound(-1, 0, cl.sfx_tink1, pos2, 1, 1);
	else
	{
		rnd = rand() & 3;
		if (rnd == 1)		S_StartSound(-1, 0, cl.sfx_ric1, pos2, 1, 1);
		else if (rnd == 2)	S_StartSound(-1, 0, cl.sfx_ric2, pos2, 1, 1);
		else				S_StartSound(-1, 0, cl.sfx_ric3, pos2, 1, 1);
	}
}

// #421 void(vector org) te_explosion (DP_TE_STANDARDEFFECTBUILTINS)
static void VM_CL_te_explosion (prvm_prog_t *prog)
{
	vec3_t		pos, pos2;
	VM_SAFEPARMCOUNT(1, VM_CL_te_explosion);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_FindNonSolidLocation(pos, pos2, 10);
	CL_ParticleEffect(EFFECT_TE_EXPLOSION, 1, pos2, pos2, vec3_origin, vec3_origin, NULL, 0);
	S_StartSound(-1, 0, cl.sfx_r_exp3, pos2, 1, 1);
}

// #422 void(vector org) te_tarexplosion (DP_TE_STANDARDEFFECTBUILTINS)
static void VM_CL_te_tarexplosion (prvm_prog_t *prog)
{
	vec3_t		pos, pos2;
	VM_SAFEPARMCOUNT(1, VM_CL_te_tarexplosion);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_FindNonSolidLocation(pos, pos2, 10);
	CL_ParticleEffect(EFFECT_TE_TAREXPLOSION, 1, pos2, pos2, vec3_origin, vec3_origin, NULL, 0);
	S_StartSound(-1, 0, cl.sfx_r_exp3, pos2, 1, 1);
}

// #423 void(vector org) te_wizspike (DP_TE_STANDARDEFFECTBUILTINS)
static void VM_CL_te_wizspike (prvm_prog_t *prog)
{
	vec3_t		pos, pos2;
	VM_SAFEPARMCOUNT(1, VM_CL_te_wizspike);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_FindNonSolidLocation(pos, pos2, 4);
	CL_ParticleEffect(EFFECT_TE_WIZSPIKE, 1, pos2, pos2, vec3_origin, vec3_origin, NULL, 0);
	S_StartSound(-1, 0, cl.sfx_wizhit, pos2, 1, 1);
}

// #424 void(vector org) te_knightspike (DP_TE_STANDARDEFFECTBUILTINS)
static void VM_CL_te_knightspike (prvm_prog_t *prog)
{
	vec3_t		pos, pos2;
	VM_SAFEPARMCOUNT(1, VM_CL_te_knightspike);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_FindNonSolidLocation(pos, pos2, 4);
	CL_ParticleEffect(EFFECT_TE_KNIGHTSPIKE, 1, pos2, pos2, vec3_origin, vec3_origin, NULL, 0);
	S_StartSound(-1, 0, cl.sfx_knighthit, pos2, 1, 1);
}

// #425 void(vector org) te_lavasplash (DP_TE_STANDARDEFFECTBUILTINS)
static void VM_CL_te_lavasplash (prvm_prog_t *prog)
{
	vec3_t		pos;
	VM_SAFEPARMCOUNT(1, VM_CL_te_lavasplash);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_ParticleEffect(EFFECT_TE_LAVASPLASH, 1, pos, pos, vec3_origin, vec3_origin, NULL, 0);
}

// #426 void(vector org) te_teleport (DP_TE_STANDARDEFFECTBUILTINS)
static void VM_CL_te_teleport (prvm_prog_t *prog)
{
	vec3_t		pos;
	VM_SAFEPARMCOUNT(1, VM_CL_te_teleport);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_ParticleEffect(EFFECT_TE_TELEPORT, 1, pos, pos, vec3_origin, vec3_origin, NULL, 0);
}

// #427 void(vector org, float colorstart, float colorlength) te_explosion2 (DP_TE_STANDARDEFFECTBUILTINS)
static void VM_CL_te_explosion2 (prvm_prog_t *prog)
{
	vec3_t		pos, pos2, color;
	matrix4x4_t	tempmatrix;
	int			colorStart, colorLength;
	unsigned char		*tempcolor;
	VM_SAFEPARMCOUNT(3, VM_CL_te_explosion2);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	colorStart = (int)PRVM_G_FLOAT(OFS_PARM1);
	colorLength = (int)PRVM_G_FLOAT(OFS_PARM2);
	CL_FindNonSolidLocation(pos, pos2, 10);
	CL_ParticleExplosion2(pos2, colorStart, colorLength);
	tempcolor = palette_rgb[(rand()%colorLength) + colorStart];
	color[0] = tempcolor[0] * (2.0f / 255.0f);
	color[1] = tempcolor[1] * (2.0f / 255.0f);
	color[2] = tempcolor[2] * (2.0f / 255.0f);
	Matrix4x4_CreateTranslate(&tempmatrix, pos2[0], pos2[1], pos2[2]);
	CL_AllocLightFlash(NULL, &tempmatrix, 350, color[0], color[1], color[2], 700, 0.5, NULL, -1, true, 1, 0.25, 0.25, 1, 1, LIGHTFLAG_NORMALMODE | LIGHTFLAG_REALTIMEMODE);
	S_StartSound(-1, 0, cl.sfx_r_exp3, pos2, 1, 1);
}


// #428 void(entity own, vector start, vector end) te_lightning1 (DP_TE_STANDARDEFFECTBUILTINS)
static void VM_CL_te_lightning1 (prvm_prog_t *prog)
{
	vec3_t		start, end;
	VM_SAFEPARMCOUNT(3, VM_CL_te_lightning1);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), start);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), end);
	CL_NewBeam(PRVM_G_EDICTNUM(OFS_PARM0), start, end, cl.model_bolt, true);
}

// #429 void(entity own, vector start, vector end) te_lightning2 (DP_TE_STANDARDEFFECTBUILTINS)
static void VM_CL_te_lightning2 (prvm_prog_t *prog)
{
	vec3_t		start, end;
	VM_SAFEPARMCOUNT(3, VM_CL_te_lightning2);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), start);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), end);
	CL_NewBeam(PRVM_G_EDICTNUM(OFS_PARM0), start, end, cl.model_bolt2, true);
}

// #430 void(entity own, vector start, vector end) te_lightning3 (DP_TE_STANDARDEFFECTBUILTINS)
static void VM_CL_te_lightning3 (prvm_prog_t *prog)
{
	vec3_t		start, end;
	VM_SAFEPARMCOUNT(3, VM_CL_te_lightning3);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), start);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), end);
	CL_NewBeam(PRVM_G_EDICTNUM(OFS_PARM0), start, end, cl.model_bolt3, false);
}

// #431 void(entity own, vector start, vector end) te_beam (DP_TE_STANDARDEFFECTBUILTINS)
static void VM_CL_te_beam (prvm_prog_t *prog)
{
	vec3_t		start, end;
	VM_SAFEPARMCOUNT(3, VM_CL_te_beam);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), start);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), end);
	CL_NewBeam(PRVM_G_EDICTNUM(OFS_PARM0), start, end, cl.model_beam, false);
}

// #433 void(vector org) te_plasmaburn (DP_TE_PLASMABURN)
static void VM_CL_te_plasmaburn (prvm_prog_t *prog)
{
	vec3_t		pos, pos2;
	VM_SAFEPARMCOUNT(1, VM_CL_te_plasmaburn);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	CL_FindNonSolidLocation(pos, pos2, 4);
	CL_ParticleEffect(EFFECT_TE_PLASMABURN, 1, pos2, pos2, vec3_origin, vec3_origin, NULL, 0);
}

// #457 void(vector org, vector velocity, float howmany) te_flamejet (DP_TE_FLAMEJET)
static void VM_CL_te_flamejet (prvm_prog_t *prog)
{
	vec3_t		pos, pos2, vel;
	VM_SAFEPARMCOUNT(3, VM_CL_te_flamejet);
	if (PRVM_G_FLOAT(OFS_PARM2) < 1)
		return;
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), vel);
	CL_FindNonSolidLocation(pos, pos2, 4);
	CL_ParticleEffect(EFFECT_TE_FLAMEJET, PRVM_G_FLOAT(OFS_PARM2), pos2, pos2, vel, vel, NULL, 0);
}


// #443 void(entity e, entity tagentity, string tagname) setattachment
static void VM_CL_setattachment (prvm_prog_t *prog)
{
	prvm_edict_t *e;
	prvm_edict_t *tagentity;
	const char *tagname;
	int modelindex;
	int tagindex;
	model_t *model;
	VM_SAFEPARMCOUNT(3, VM_CL_setattachment);

	e = PRVM_G_EDICT(OFS_PARM0);
	tagentity = PRVM_G_EDICT(OFS_PARM1);
	tagname = PRVM_G_STRING(OFS_PARM2);

	if (e == prog->edicts)
	{
		VM_Warning(prog, "setattachment: can not modify world entity\n");
		return;
	}
	if (e->free)
	{
		VM_Warning(prog, "setattachment: can not modify free entity\n");
		return;
	}

	if (tagentity == NULL)
		tagentity = prog->edicts;

	tagindex = 0;
	if (tagentity != NULL && tagentity != prog->edicts && tagname && tagname[0])
	{
		modelindex = (int)PRVM_clientedictfloat(tagentity, modelindex);
		model = CL_GetModelByIndex(modelindex);
		if (model)
		{
			tagindex = Mod_Alias_GetTagIndexForName(model, (int)PRVM_clientedictfloat(tagentity, skin), tagname);
			if (tagindex == 0)
				Con_DPrintf("setattachment(edict %i, edict %i, string \"%s\"): tried to find tag named \"%s\" on entity %i (model \"%s\") but could not find it\n", PRVM_NUM_FOR_EDICT(e), PRVM_NUM_FOR_EDICT(tagentity), tagname, tagname, PRVM_NUM_FOR_EDICT(tagentity), model->name);
		}
		else
			Con_DPrintf("setattachment(edict %i, edict %i, string \"%s\"): tried to find tag named \"%s\" on entity %i but it has no model\n", PRVM_NUM_FOR_EDICT(e), PRVM_NUM_FOR_EDICT(tagentity), tagname, tagname, PRVM_NUM_FOR_EDICT(tagentity));
	}

	PRVM_clientedictedict(e, tag_entity) = PRVM_EDICT_TO_PROG(tagentity);
	PRVM_clientedictfloat(e, tag_index) = tagindex;
}

/////////////////////////////////////////
// DP_MD3_TAGINFO extension coded by VorteX

static int CL_GetTagIndex (prvm_prog_t *prog, prvm_edict_t *e, const char *tagname)
{
	model_t *model = CL_GetModelFromEdict(e);
	if (model)
		return Mod_Alias_GetTagIndexForName(model, (int)PRVM_clientedictfloat(e, skin), tagname);
	else
		return -1;
}

static int CL_GetExtendedTagInfo (prvm_prog_t *prog, prvm_edict_t *e, int tagindex, int *parentindex, const char **tagname, matrix4x4_t *tag_localmatrix)
{
	int r;
	model_t *model;

	*tagname = NULL;
	*parentindex = 0;
	Matrix4x4_CreateIdentity(tag_localmatrix);

	if (tagindex >= 0
	 && (model = CL_GetModelFromEdict(e))
	 && model->animscenes)
	{
		r = Mod_Alias_GetExtendedTagInfoForIndex(model, (int)PRVM_clientedictfloat(e, skin), e->priv.server->frameblend, &e->priv.server->skeleton, tagindex - 1, parentindex, tagname, tag_localmatrix);

		if(!r) // success?
			*parentindex += 1;

		return r;
	}

	return 1;
}

int CL_GetPitchSign(prvm_prog_t *prog, prvm_edict_t *ent)
{
	model_t *model;
	if ((model = CL_GetModelFromEdict(ent)) && model->type == mod_alias)
		return -1;
	return 1;
}

void CL_GetEntityMatrix (prvm_prog_t *prog, prvm_edict_t *ent, matrix4x4_t *out, qbool viewmatrix)
{
	float scale;
	float pitchsign = 1;

	scale = PRVM_clientedictfloat(ent, scale);
	if (!scale)
		scale = 1.0f;

	if(viewmatrix)
		*out = r_refdef.view.matrix;
	else if ((int)PRVM_clientedictfloat(ent, renderflags) & RF_USEAXIS)
	{
		vec3_t forward;
		vec3_t left;
		vec3_t up;
		vec3_t origin;
		VectorScale(PRVM_clientglobalvector(v_forward), scale, forward);
		VectorScale(PRVM_clientglobalvector(v_right), -scale, left);
		VectorScale(PRVM_clientglobalvector(v_up), scale, up);
		VectorCopy(PRVM_clientedictvector(ent, origin), origin);
		Matrix4x4_FromVectors(out, forward, left, up, origin);
	}
	else
	{
		pitchsign = CL_GetPitchSign(prog, ent);
		Matrix4x4_CreateFromQuakeEntity(out, PRVM_clientedictvector(ent, origin)[0], PRVM_clientedictvector(ent, origin)[1], PRVM_clientedictvector(ent, origin)[2], pitchsign * PRVM_clientedictvector(ent, angles)[0], PRVM_clientedictvector(ent, angles)[1], PRVM_clientedictvector(ent, angles)[2], scale);
	}
}

static int CL_GetEntityLocalTagMatrix(prvm_prog_t *prog, prvm_edict_t *ent, int tagindex, matrix4x4_t *out)
{
	model_t *model;
	if (tagindex >= 0
	 && (model = CL_GetModelFromEdict(ent))
	 && model->animscenes)
	{
		VM_GenerateFrameGroupBlend(prog, ent->priv.server->framegroupblend, ent);
		VM_FrameBlendFromFrameGroupBlend(ent->priv.server->frameblend, ent->priv.server->framegroupblend, model, cl.time);
		VM_UpdateEdictSkeleton(prog, ent, model, ent->priv.server->frameblend);
		return Mod_Alias_GetTagMatrix(model, ent->priv.server->frameblend, &ent->priv.server->skeleton, tagindex, out);
	}
	*out = identitymatrix;
	return 0;
}

// Warnings/errors code:
// 0 - normal (everything all-right)
// 1 - world entity
// 2 - free entity
// 3 - null or non-precached model
// 4 - no tags with requested index
// 5 - runaway loop at attachment chain
extern cvar_t cl_bob;
extern cvar_t cl_bobcycle;
extern cvar_t cl_bobup;
int CL_GetTagMatrix (prvm_prog_t *prog, matrix4x4_t *out, prvm_edict_t *ent, int tagindex, prvm_vec_t *returnshadingorigin)
{
	int ret;
	int attachloop;
	matrix4x4_t entitymatrix, tagmatrix, attachmatrix;
	model_t *model;
	vec3_t shadingorigin;

	*out = identitymatrix; // warnings and errors return identical matrix

	if (ent == prog->edicts)
		return 1;
	if (ent->free)
		return 2;

	model = CL_GetModelFromEdict(ent);
	if(!model)
		return 3;

	tagmatrix = identitymatrix;
	attachloop = 0;
	for(;;)
	{
		if(attachloop >= 256)
			return 5;
		// apply transformation by child's tagindex on parent entity and then
		// by parent entity itself
		ret = CL_GetEntityLocalTagMatrix(prog, ent, tagindex - 1, &attachmatrix);
		if(ret && attachloop == 0)
			return ret;
		CL_GetEntityMatrix(prog, ent, &entitymatrix, false);
		Matrix4x4_Concat(&tagmatrix, &attachmatrix, out);
		Matrix4x4_Concat(out, &entitymatrix, &tagmatrix);
		// next iteration we process the parent entity
		if (PRVM_clientedictedict(ent, tag_entity))
		{
			tagindex = (int)PRVM_clientedictfloat(ent, tag_index);
			ent = PRVM_EDICT_NUM(PRVM_clientedictedict(ent, tag_entity));
		}
		else
			break;
		attachloop++;
	}

	// RENDER_VIEWMODEL magic
	if ((int)PRVM_clientedictfloat(ent, renderflags) & RF_VIEWMODEL)
	{
		Matrix4x4_Copy(&tagmatrix, out);

		CL_GetEntityMatrix(prog, prog->edicts, &entitymatrix, true);
		Matrix4x4_Concat(out, &entitymatrix, &tagmatrix);

		/*
		// Cl_bob, ported from rendering code
		if (PRVM_clientedictfloat(ent, health) > 0 && cl_bob.value && cl_bobcycle.value)
		{
			double bob, cycle;
			// LadyHavoc: this code is *weird*, but not replacable (I think it
			// should be done in QC on the server, but oh well, quake is quake)
			// LadyHavoc: figured out bobup: the time at which the sin is at 180
			// degrees (which allows lengthening or squishing the peak or valley)
			cycle = cl.time/cl_bobcycle.value;
			cycle -= (int)cycle;
			if (cycle < cl_bobup.value)
				cycle = sin(M_PI * cycle / cl_bobup.value);
			else
				cycle = sin(M_PI + M_PI * (cycle-cl_bobup.value)/(1.0 - cl_bobup.value));
			// bob is proportional to velocity in the xy plane
			// (don't count Z, or jumping messes it up)
			bob = sqrt(PRVM_clientedictvector(ent, velocity)[0]*PRVM_clientedictvector(ent, velocity)[0] + PRVM_clientedictvector(ent, velocity)[1]*PRVM_clientedictvector(ent, velocity)[1])*cl_bob.value;
			bob = bob*0.3 + bob*0.7*cycle;
			Matrix4x4_AdjustOrigin(out, 0, 0, bound(-7, bob, 4));
		}
		*/

		// return the origin of the view
		Matrix4x4_OriginFromMatrix(&r_refdef.view.matrix, shadingorigin);
	}
	else
	{
		// return the origin of the root entity in the chain
		Matrix4x4_OriginFromMatrix(out, shadingorigin);
	}
	if (returnshadingorigin)
		VectorCopy(shadingorigin, returnshadingorigin);
	return 0;
}

// #451 float(entity ent, string tagname) gettagindex (DP_QC_GETTAGINFO)
static void VM_CL_gettagindex (prvm_prog_t *prog)
{
	prvm_edict_t *ent;
	const char *tag_name;
	int tag_index;

	VM_SAFEPARMCOUNT(2, VM_CL_gettagindex);

	ent = PRVM_G_EDICT(OFS_PARM0);
	tag_name = PRVM_G_STRING(OFS_PARM1);
	if (ent == prog->edicts)
	{
		VM_Warning(prog, "VM_CL_gettagindex(entity #%i): can't affect world entity\n", PRVM_NUM_FOR_EDICT(ent));
		return;
	}
	if (ent->free)
	{
		VM_Warning(prog, "VM_CL_gettagindex(entity #%i): can't affect free entity\n", PRVM_NUM_FOR_EDICT(ent));
		return;
	}

	tag_index = 0;
	if (!CL_GetModelFromEdict(ent))
		Con_DPrintf("VM_CL_gettagindex(entity #%i): null or non-precached model\n", PRVM_NUM_FOR_EDICT(ent));
	else
	{
		tag_index = CL_GetTagIndex(prog, ent, tag_name);
		if (tag_index == 0)
			if(developer_extra.integer)
				Con_DPrintf("VM_CL_gettagindex(entity #%i): tag \"%s\" not found\n", PRVM_NUM_FOR_EDICT(ent), tag_name);
	}
	PRVM_G_FLOAT(OFS_RETURN) = tag_index;
}

// #452 vector(entity ent, float tagindex) gettaginfo (DP_QC_GETTAGINFO)
static void VM_CL_gettaginfo (prvm_prog_t *prog)
{
	prvm_edict_t *e;
	int tagindex;
	matrix4x4_t tag_matrix;
	matrix4x4_t tag_localmatrix;
	int parentindex;
	const char *tagname;
	int returncode;
	vec3_t forward, left, up, origin;
	const model_t *model;

	VM_SAFEPARMCOUNT(2, VM_CL_gettaginfo);

	e = PRVM_G_EDICT(OFS_PARM0);
	tagindex = (int)PRVM_G_FLOAT(OFS_PARM1);
	returncode = CL_GetTagMatrix(prog, &tag_matrix, e, tagindex, NULL);
	Matrix4x4_ToVectors(&tag_matrix, forward, left, up, origin);
	VectorCopy(forward, PRVM_clientglobalvector(v_forward));
	VectorScale(left, -1, PRVM_clientglobalvector(v_right));
	VectorCopy(up, PRVM_clientglobalvector(v_up));
	VectorCopy(origin, PRVM_G_VECTOR(OFS_RETURN));
	model = CL_GetModelFromEdict(e);
	VM_GenerateFrameGroupBlend(prog, e->priv.server->framegroupblend, e);
	VM_FrameBlendFromFrameGroupBlend(e->priv.server->frameblend, e->priv.server->framegroupblend, model, cl.time);
	VM_UpdateEdictSkeleton(prog, e, model, e->priv.server->frameblend);
	CL_GetExtendedTagInfo(prog, e, tagindex, &parentindex, &tagname, &tag_localmatrix);
	Matrix4x4_ToVectors(&tag_localmatrix, forward, left, up, origin);

	PRVM_clientglobalfloat(gettaginfo_parent) = parentindex;
	PRVM_clientglobalstring(gettaginfo_name) = tagname ? PRVM_SetTempString(prog, tagname, strlen(tagname)) : 0;
	VectorCopy(forward, PRVM_clientglobalvector(gettaginfo_forward));
	VectorScale(left, -1, PRVM_clientglobalvector(gettaginfo_right));
	VectorCopy(up, PRVM_clientglobalvector(gettaginfo_up));
	VectorCopy(origin, PRVM_clientglobalvector(gettaginfo_offset));

	switch(returncode)
	{
		case 1:
			VM_Warning(prog, "gettagindex: can't affect world entity\n");
			break;
		case 2:
			VM_Warning(prog, "gettagindex: can't affect free entity\n");
			break;
		case 3:
			Con_DPrintf("CL_GetTagMatrix(entity #%i): null or non-precached model\n", PRVM_NUM_FOR_EDICT(e));
			break;
		case 4:
			Con_DPrintf("CL_GetTagMatrix(entity #%i): model has no tag with requested index %i\n", PRVM_NUM_FOR_EDICT(e), tagindex);
			break;
		case 5:
			Con_DPrintf("CL_GetTagMatrix(entity #%i): runaway loop at attachment chain\n", PRVM_NUM_FOR_EDICT(e));
			break;
	}
}

//============================================================================

//====================
// DP_CSQC_SPAWNPARTICLE
// a QC hook to engine's CL_NewParticle
//====================

// particle theme struct
typedef struct vmparticletheme_s
{
	unsigned short typeindex;
	qbool initialized;
	pblend_t blendmode;
	porientation_t orientation;
	int color1;
	int color2;
	int tex;
	float size;
	float sizeincrease;
	float alpha;
	float alphafade;
	float gravity;
	float bounce;
	float airfriction;
	float liquidfriction;
	float originjitter;
	float velocityjitter;
	qbool qualityreduction;
	float lifetime;
	float stretch;
	int staincolor1;
	int staincolor2;
	int staintex;
	float stainalpha;
	float stainsize;
	float delayspawn;
	float delaycollision;
	float angle;
	float spin;
}vmparticletheme_t;

// particle spawner
typedef struct vmparticlespawner_s
{
	mempool_t			*pool;
	qbool			initialized;
	qbool			verified;
	vmparticletheme_t	*themes;
	int					max_themes;
}vmparticlespawner_t;

vmparticlespawner_t vmpartspawner;

// TODO: automatic max_themes grow
static void VM_InitParticleSpawner (prvm_prog_t *prog, int maxthemes)
{
	// bound max themes to not be an insane value
	if (maxthemes < 4)
		maxthemes = 4;
	if (maxthemes > 2048)
		maxthemes = 2048;
	// allocate and set up structure
	if (vmpartspawner.initialized) // reallocate
	{
		Mem_FreePool(&vmpartspawner.pool);
		memset(&vmpartspawner, 0, sizeof(vmparticlespawner_t));
	}
	vmpartspawner.pool = Mem_AllocPool("VMPARTICLESPAWNER", 0, NULL);
	vmpartspawner.themes = (vmparticletheme_t *)Mem_Alloc(vmpartspawner.pool, sizeof(vmparticletheme_t)*maxthemes);
	vmpartspawner.max_themes = maxthemes;
	vmpartspawner.initialized = true;
	vmpartspawner.verified = true;
}

// reset particle theme to default values
static void VM_ResetParticleTheme (vmparticletheme_t *theme)
{
	theme->initialized = true;
	theme->typeindex = pt_static;
	theme->blendmode = PBLEND_ADD;
	theme->orientation = PARTICLE_BILLBOARD;
	theme->color1 = 0x808080;
	theme->color2 = 0xFFFFFF;
	theme->tex = 63;
	theme->size = 2;
	theme->sizeincrease = 0;
	theme->alpha = 256;
	theme->alphafade = 512;
	theme->gravity = 0.0f;
	theme->bounce = 0.0f;
	theme->airfriction = 1.0f;
	theme->liquidfriction = 4.0f;
	theme->originjitter = 0.0f;
	theme->velocityjitter = 0.0f;
	theme->qualityreduction = false;
	theme->lifetime = 4;
	theme->stretch = 1;
	theme->staincolor1 = -1;
	theme->staincolor2 = -1;
	theme->staintex = -1;
	theme->delayspawn = 0.0f;
	theme->delaycollision = 0.0f;
	theme->angle = 0.0f;
	theme->spin = 0.0f;
}

// particle theme -> QC globals
static void VM_CL_ParticleThemeToGlobals(vmparticletheme_t *theme, prvm_prog_t *prog)
{
	PRVM_clientglobalfloat(particle_type) = theme->typeindex;
	PRVM_clientglobalfloat(particle_blendmode) = theme->blendmode;
	PRVM_clientglobalfloat(particle_orientation) = theme->orientation;
	// VorteX: int only can store 0-255, not 0-256 which means 0 - 0,99609375...
	VectorSet(PRVM_clientglobalvector(particle_color1), (theme->color1 >> 16) & 0xFF, (theme->color1 >> 8) & 0xFF, (theme->color1 >> 0) & 0xFF);
	VectorSet(PRVM_clientglobalvector(particle_color2), (theme->color2 >> 16) & 0xFF, (theme->color2 >> 8) & 0xFF, (theme->color2 >> 0) & 0xFF);
	PRVM_clientglobalfloat(particle_tex) = (prvm_vec_t)theme->tex;
	PRVM_clientglobalfloat(particle_size) = theme->size;
	PRVM_clientglobalfloat(particle_sizeincrease) = theme->sizeincrease;
	PRVM_clientglobalfloat(particle_alpha) = theme->alpha/256;
	PRVM_clientglobalfloat(particle_alphafade) = theme->alphafade/256;
	PRVM_clientglobalfloat(particle_time) = theme->lifetime;
	PRVM_clientglobalfloat(particle_gravity) = theme->gravity;
	PRVM_clientglobalfloat(particle_bounce) = theme->bounce;
	PRVM_clientglobalfloat(particle_airfriction) = theme->airfriction;
	PRVM_clientglobalfloat(particle_liquidfriction) = theme->liquidfriction;
	PRVM_clientglobalfloat(particle_originjitter) = theme->originjitter;
	PRVM_clientglobalfloat(particle_velocityjitter) = theme->velocityjitter;
	PRVM_clientglobalfloat(particle_qualityreduction) = theme->qualityreduction;
	PRVM_clientglobalfloat(particle_stretch) = theme->stretch;
	VectorSet(PRVM_clientglobalvector(particle_staincolor1), ((int)theme->staincolor1 >> 16) & 0xFF, ((int)theme->staincolor1 >> 8) & 0xFF, ((int)theme->staincolor1 >> 0) & 0xFF);
	VectorSet(PRVM_clientglobalvector(particle_staincolor2), ((int)theme->staincolor2 >> 16) & 0xFF, ((int)theme->staincolor2 >> 8) & 0xFF, ((int)theme->staincolor2 >> 0) & 0xFF);
	PRVM_clientglobalfloat(particle_staintex) = (prvm_vec_t)theme->staintex;
	PRVM_clientglobalfloat(particle_stainalpha) = (prvm_vec_t)theme->stainalpha/256;
	PRVM_clientglobalfloat(particle_stainsize) = (prvm_vec_t)theme->stainsize;
	PRVM_clientglobalfloat(particle_delayspawn) = theme->delayspawn;
	PRVM_clientglobalfloat(particle_delaycollision) = theme->delaycollision;
	PRVM_clientglobalfloat(particle_angle) = theme->angle;
	PRVM_clientglobalfloat(particle_spin) = theme->spin;
}

// QC globals ->  particle theme
static void VM_CL_ParticleThemeFromGlobals(vmparticletheme_t *theme, prvm_prog_t *prog)
{
	theme->typeindex = (unsigned short)PRVM_clientglobalfloat(particle_type);
	theme->blendmode = (pblend_t)(int)PRVM_clientglobalfloat(particle_blendmode);
	theme->orientation = (porientation_t)(int)PRVM_clientglobalfloat(particle_orientation);
	theme->color1 = ((int)PRVM_clientglobalvector(particle_color1)[0] << 16) + ((int)PRVM_clientglobalvector(particle_color1)[1] << 8) + ((int)PRVM_clientglobalvector(particle_color1)[2]);
	theme->color2 = ((int)PRVM_clientglobalvector(particle_color2)[0] << 16) + ((int)PRVM_clientglobalvector(particle_color2)[1] << 8) + ((int)PRVM_clientglobalvector(particle_color2)[2]);
	theme->tex = (int)PRVM_clientglobalfloat(particle_tex);
	theme->size = PRVM_clientglobalfloat(particle_size);
	theme->sizeincrease = PRVM_clientglobalfloat(particle_sizeincrease);
	theme->alpha = PRVM_clientglobalfloat(particle_alpha)*256;
	theme->alphafade = PRVM_clientglobalfloat(particle_alphafade)*256;
	theme->lifetime = PRVM_clientglobalfloat(particle_time);
	theme->gravity = PRVM_clientglobalfloat(particle_gravity);
	theme->bounce = PRVM_clientglobalfloat(particle_bounce);
	theme->airfriction = PRVM_clientglobalfloat(particle_airfriction);
	theme->liquidfriction = PRVM_clientglobalfloat(particle_liquidfriction);
	theme->originjitter = PRVM_clientglobalfloat(particle_originjitter);
	theme->velocityjitter = PRVM_clientglobalfloat(particle_velocityjitter);
	theme->qualityreduction = PRVM_clientglobalfloat(particle_qualityreduction) != 0 ? true : false;
	theme->stretch = PRVM_clientglobalfloat(particle_stretch);
	theme->staincolor1 = ((int)PRVM_clientglobalvector(particle_staincolor1)[0])*65536 + (int)(PRVM_clientglobalvector(particle_staincolor1)[1])*256 + (int)(PRVM_clientglobalvector(particle_staincolor1)[2]);
	theme->staincolor2 = (int)(PRVM_clientglobalvector(particle_staincolor2)[0])*65536 + (int)(PRVM_clientglobalvector(particle_staincolor2)[1])*256 + (int)(PRVM_clientglobalvector(particle_staincolor2)[2]);
	theme->staintex =(int)PRVM_clientglobalfloat(particle_staintex);
	theme->stainalpha = PRVM_clientglobalfloat(particle_stainalpha)*256;
	theme->stainsize = PRVM_clientglobalfloat(particle_stainsize);
	theme->delayspawn = PRVM_clientglobalfloat(particle_delayspawn);
	theme->delaycollision = PRVM_clientglobalfloat(particle_delaycollision);
	theme->angle = PRVM_clientglobalfloat(particle_angle);
	theme->spin = PRVM_clientglobalfloat(particle_spin);
}

// init particle spawner interface
// # float(float max_themes) initparticlespawner
static void VM_CL_InitParticleSpawner (prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNTRANGE(0, 1, VM_CL_InitParticleSpawner);
	VM_InitParticleSpawner(prog, (int)PRVM_G_FLOAT(OFS_PARM0));
	vmpartspawner.themes[0].initialized = true;
	VM_ResetParticleTheme(&vmpartspawner.themes[0]);
	PRVM_G_FLOAT(OFS_RETURN) = (vmpartspawner.verified == true) ? 1 : 0;
}

// void() resetparticle
static void VM_CL_ResetParticle (prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(0, VM_CL_ResetParticle);
	if (vmpartspawner.verified == false)
	{
		VM_Warning(prog, "VM_CL_ResetParticle: particle spawner not initialized\n");
		return;
	}
	VM_CL_ParticleThemeToGlobals(&vmpartspawner.themes[0], prog);
}

// void(float themenum) particletheme
static void VM_CL_ParticleTheme (prvm_prog_t *prog)
{
	int themenum;

	VM_SAFEPARMCOUNT(1, VM_CL_ParticleTheme);
	if (vmpartspawner.verified == false)
	{
		VM_Warning(prog, "VM_CL_ParticleTheme: particle spawner not initialized\n");
		return;
	}
	themenum = (int)PRVM_G_FLOAT(OFS_PARM0);
	if (themenum < 0 || themenum >= vmpartspawner.max_themes)
	{
		VM_Warning(prog, "VM_CL_ParticleTheme: bad theme number %i\n", themenum);
		VM_CL_ParticleThemeToGlobals(&vmpartspawner.themes[0], prog);
		return;
	}
	if (vmpartspawner.themes[themenum].initialized == false)
	{
		VM_Warning(prog, "VM_CL_ParticleTheme: theme #%i not exists\n", themenum);
		VM_CL_ParticleThemeToGlobals(&vmpartspawner.themes[0], prog);
		return;
	}
	// load particle theme into globals
	VM_CL_ParticleThemeToGlobals(&vmpartspawner.themes[themenum], prog);
}

// float() saveparticletheme
// void(float themenum) updateparticletheme
static void VM_CL_ParticleThemeSave (prvm_prog_t *prog)
{
	int themenum;

	VM_SAFEPARMCOUNTRANGE(0, 1, VM_CL_ParticleThemeSave);
	if (vmpartspawner.verified == false)
	{
		VM_Warning(prog, "VM_CL_ParticleThemeSave: particle spawner not initialized\n");
		return;
	}
	// allocate new theme, save it and return
	if (prog->argc < 1)
	{
		for (themenum = 0; themenum < vmpartspawner.max_themes; themenum++)
			if (vmpartspawner.themes[themenum].initialized == false)
				break;
		if (themenum >= vmpartspawner.max_themes)
		{
			if (vmpartspawner.max_themes == 2048)
				VM_Warning(prog, "VM_CL_ParticleThemeSave: no free theme slots\n");
			else
				VM_Warning(prog, "VM_CL_ParticleThemeSave: no free theme slots, try initparticlespawner() with highter max_themes\n");
			PRVM_G_FLOAT(OFS_RETURN) = -1;
			return;
		}
		vmpartspawner.themes[themenum].initialized = true;
		VM_CL_ParticleThemeFromGlobals(&vmpartspawner.themes[themenum], prog);
		PRVM_G_FLOAT(OFS_RETURN) = themenum;
		return;
	}
	// update existing theme
	themenum = (int)PRVM_G_FLOAT(OFS_PARM0);
	if (themenum < 0 || themenum >= vmpartspawner.max_themes)
	{
		VM_Warning(prog, "VM_CL_ParticleThemeSave: bad theme number %i\n", themenum);
		return;
	}
	vmpartspawner.themes[themenum].initialized = true;
	VM_CL_ParticleThemeFromGlobals(&vmpartspawner.themes[themenum], prog);
}

// void(float themenum) freeparticletheme
static void VM_CL_ParticleThemeFree (prvm_prog_t *prog)
{
	int themenum;

	VM_SAFEPARMCOUNT(1, VM_CL_ParticleThemeFree);
	if (vmpartspawner.verified == false)
	{
		VM_Warning(prog, "VM_CL_ParticleThemeFree: particle spawner not initialized\n");
		return;
	}
	themenum = (int)PRVM_G_FLOAT(OFS_PARM0);
	// check parms
	if (themenum <= 0 || themenum >= vmpartspawner.max_themes)
	{
		VM_Warning(prog, "VM_CL_ParticleThemeFree: bad theme number %i\n", themenum);
		return;
	}
	if (vmpartspawner.themes[themenum].initialized == false)
	{
		VM_Warning(prog, "VM_CL_ParticleThemeFree: theme #%i already freed\n", themenum);
		VM_CL_ParticleThemeToGlobals(&vmpartspawner.themes[0], prog);
		return;
	}
	// free theme
	VM_ResetParticleTheme(&vmpartspawner.themes[themenum]);
	vmpartspawner.themes[themenum].initialized = false;
}

// float(vector org, vector dir, [float theme]) particle
// returns 0 if failed, 1 if succesful
static void VM_CL_SpawnParticle (prvm_prog_t *prog)
{
	vec3_t org, dir;
	vmparticletheme_t *theme;
	particle_t *part;
	int themenum;

	VM_SAFEPARMCOUNTRANGE(2, 3, VM_CL_SpawnParticle);
	if (vmpartspawner.verified == false)
	{
		VM_Warning(prog, "VM_CL_SpawnParticle: particle spawner not initialized\n");
		PRVM_G_FLOAT(OFS_RETURN) = 0;
		return;
	}
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), org);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), dir);

	if (prog->argc < 3) // global-set particle
	{
		part = CL_NewParticle(org,
			(unsigned short)PRVM_clientglobalfloat(particle_type),
			((int)PRVM_clientglobalvector(particle_color1)[0] << 16) + ((int)PRVM_clientglobalvector(particle_color1)[1] << 8) + ((int)PRVM_clientglobalvector(particle_color1)[2]),
			((int)PRVM_clientglobalvector(particle_color2)[0] << 16) + ((int)PRVM_clientglobalvector(particle_color2)[1] << 8) + ((int)PRVM_clientglobalvector(particle_color2)[2]),
			(int)PRVM_clientglobalfloat(particle_tex),
			PRVM_clientglobalfloat(particle_size),
			PRVM_clientglobalfloat(particle_sizeincrease),
			PRVM_clientglobalfloat(particle_alpha)*256,
			PRVM_clientglobalfloat(particle_alphafade)*256,
			PRVM_clientglobalfloat(particle_gravity),
			PRVM_clientglobalfloat(particle_bounce),
			org[0],
			org[1],
			org[2],
			dir[0],
			dir[1],
			dir[2],
			PRVM_clientglobalfloat(particle_airfriction),
			PRVM_clientglobalfloat(particle_liquidfriction),
			PRVM_clientglobalfloat(particle_originjitter),
			PRVM_clientglobalfloat(particle_velocityjitter),
			(PRVM_clientglobalfloat(particle_qualityreduction)) ? true : false,
			PRVM_clientglobalfloat(particle_time),
			PRVM_clientglobalfloat(particle_stretch),
			(pblend_t)(int)PRVM_clientglobalfloat(particle_blendmode),
			(porientation_t)(int)PRVM_clientglobalfloat(particle_orientation),
			(int)(PRVM_clientglobalvector(particle_staincolor1)[0])*65536 + (int)(PRVM_clientglobalvector(particle_staincolor1)[1])*256 + (int)(PRVM_clientglobalvector(particle_staincolor1)[2]),
			(int)(PRVM_clientglobalvector(particle_staincolor2)[0])*65536 + (int)(PRVM_clientglobalvector(particle_staincolor2)[1])*256 + (int)(PRVM_clientglobalvector(particle_staincolor2)[2]),
			(int)PRVM_clientglobalfloat(particle_staintex),
			PRVM_clientglobalfloat(particle_stainalpha)*256,
			PRVM_clientglobalfloat(particle_stainsize),
			PRVM_clientglobalfloat(particle_angle),
			PRVM_clientglobalfloat(particle_spin),
			NULL);
		if (!part)
		{
			PRVM_G_FLOAT(OFS_RETURN) = 0;
			return;
		}
		if (PRVM_clientglobalfloat(particle_delayspawn))
			part->delayedspawn = cl.time + PRVM_clientglobalfloat(particle_delayspawn);
		//if (PRVM_clientglobalfloat(particle_delaycollision))
		//	part->delayedcollisions = cl.time + PRVM_clientglobalfloat(particle_delaycollision);
	}
	else // quick themed particle
	{
		themenum = (int)PRVM_G_FLOAT(OFS_PARM2);
		if (themenum <= 0 || themenum >= vmpartspawner.max_themes)
		{
			VM_Warning(prog, "VM_CL_SpawnParticle: bad theme number %i\n", themenum);
			PRVM_G_FLOAT(OFS_RETURN) = 0;
			return;
		}
		theme = &vmpartspawner.themes[themenum];
		part = CL_NewParticle(org,
			theme->typeindex,
			theme->color1,
			theme->color2,
			theme->tex,
			theme->size,
			theme->sizeincrease,
			theme->alpha,
			theme->alphafade,
			theme->gravity,
			theme->bounce,
			org[0],
			org[1],
			org[2],
			dir[0],
			dir[1],
			dir[2],
			theme->airfriction,
			theme->liquidfriction,
			theme->originjitter,
			theme->velocityjitter,
			theme->qualityreduction,
			theme->lifetime,
			theme->stretch,
			theme->blendmode,
			theme->orientation,
			theme->staincolor1,
			theme->staincolor2,
			theme->staintex,
			theme->stainalpha,
			theme->stainsize,
			theme->angle,
			theme->spin,
			NULL);
		if (!part)
		{
			PRVM_G_FLOAT(OFS_RETURN) = 0;
			return;
		}
		if (theme->delayspawn)
			part->delayedspawn = cl.time + theme->delayspawn;
		//if (theme->delaycollision)
		//	part->delayedcollisions = cl.time + theme->delaycollision;
	}
	PRVM_G_FLOAT(OFS_RETURN) = 1;
}

// float(vector org, vector dir, float spawndelay, float collisiondelay, [float theme]) delayedparticle
// returns 0 if failed, 1 if success
static void VM_CL_SpawnParticleDelayed (prvm_prog_t *prog)
{
	vec3_t org, dir;
	vmparticletheme_t *theme;
	particle_t *part;
	int themenum;

	VM_SAFEPARMCOUNTRANGE(4, 5, VM_CL_SpawnParticleDelayed);
	if (vmpartspawner.verified == false)
	{
		VM_Warning(prog, "VM_CL_SpawnParticleDelayed: particle spawner not initialized\n");
		PRVM_G_FLOAT(OFS_RETURN) = 0;
		return;
	}
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), org);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), dir);
	if (prog->argc < 5) // global-set particle
		part = CL_NewParticle(org,
			(unsigned short)PRVM_clientglobalfloat(particle_type),
			((int)PRVM_clientglobalvector(particle_color1)[0] << 16) + ((int)PRVM_clientglobalvector(particle_color1)[1] << 8) + ((int)PRVM_clientglobalvector(particle_color1)[2]),
			((int)PRVM_clientglobalvector(particle_color2)[0] << 16) + ((int)PRVM_clientglobalvector(particle_color2)[1] << 8) + ((int)PRVM_clientglobalvector(particle_color2)[2]),
			(int)PRVM_clientglobalfloat(particle_tex),
			PRVM_clientglobalfloat(particle_size),
			PRVM_clientglobalfloat(particle_sizeincrease),
			PRVM_clientglobalfloat(particle_alpha)*256,
			PRVM_clientglobalfloat(particle_alphafade)*256,
			PRVM_clientglobalfloat(particle_gravity),
			PRVM_clientglobalfloat(particle_bounce),
			org[0],
			org[1],
			org[2],
			dir[0],
			dir[1],
			dir[2],
			PRVM_clientglobalfloat(particle_airfriction),
			PRVM_clientglobalfloat(particle_liquidfriction),
			PRVM_clientglobalfloat(particle_originjitter),
			PRVM_clientglobalfloat(particle_velocityjitter),
			(PRVM_clientglobalfloat(particle_qualityreduction)) ? true : false,
			PRVM_clientglobalfloat(particle_time),
			PRVM_clientglobalfloat(particle_stretch),
			(pblend_t)(int)PRVM_clientglobalfloat(particle_blendmode),
			(porientation_t)(int)PRVM_clientglobalfloat(particle_orientation),
			((int)PRVM_clientglobalvector(particle_staincolor1)[0] << 16) + ((int)PRVM_clientglobalvector(particle_staincolor1)[1] << 8) + ((int)PRVM_clientglobalvector(particle_staincolor1)[2]),
			((int)PRVM_clientglobalvector(particle_staincolor2)[0] << 16) + ((int)PRVM_clientglobalvector(particle_staincolor2)[1] << 8) + ((int)PRVM_clientglobalvector(particle_staincolor2)[2]),
			(int)PRVM_clientglobalfloat(particle_staintex),
			PRVM_clientglobalfloat(particle_stainalpha)*256,
			PRVM_clientglobalfloat(particle_stainsize),
			PRVM_clientglobalfloat(particle_angle),
			PRVM_clientglobalfloat(particle_spin),
			NULL);
	else // themed particle
	{
		themenum = (int)PRVM_G_FLOAT(OFS_PARM4);
		if (themenum <= 0 || themenum >= vmpartspawner.max_themes)
		{
			VM_Warning(prog, "VM_CL_SpawnParticleDelayed: bad theme number %i\n", themenum);
			PRVM_G_FLOAT(OFS_RETURN) = 0;
			return;
		}
		theme = &vmpartspawner.themes[themenum];
		part = CL_NewParticle(org,
			theme->typeindex,
			theme->color1,
			theme->color2,
			theme->tex,
			theme->size,
			theme->sizeincrease,
			theme->alpha,
			theme->alphafade,
			theme->gravity,
			theme->bounce,
			org[0],
			org[1],
			org[2],
			dir[0],
			dir[1],
			dir[2],
			theme->airfriction,
			theme->liquidfriction,
			theme->originjitter,
			theme->velocityjitter,
			theme->qualityreduction,
			theme->lifetime,
			theme->stretch,
			theme->blendmode,
			theme->orientation,
			theme->staincolor1,
			theme->staincolor2,
			theme->staintex,
			theme->stainalpha,
			theme->stainsize,
			theme->angle,
			theme->spin,
			NULL);
	}
	if (!part)
	{
		PRVM_G_FLOAT(OFS_RETURN) = 0;
		return;
	}
	part->delayedspawn = cl.time + PRVM_G_FLOAT(OFS_PARM2);
	//part->delayedcollisions = cl.time + PRVM_G_FLOAT(OFS_PARM3);
	PRVM_G_FLOAT(OFS_RETURN) = 0;
}

//====================
//CSQC engine entities query
//====================

// float(float entitynum, float whatfld) getentity;
// vector(float entitynum, float whatfld) getentityvec;
// querying engine-drawn entity
// VorteX: currently it's only tested with whatfld = 1..7
static void VM_CL_GetEntity (prvm_prog_t *prog)
{
	int entnum, fieldnum;
	vec3_t forward, left, up, org;
	VM_SAFEPARMCOUNT(2, VM_CL_GetEntity);

	entnum = PRVM_G_FLOAT(OFS_PARM0);
	if (entnum < 0 || entnum >= cl.num_entities)
	{
		PRVM_G_FLOAT(OFS_RETURN) = 0;
		return;
	}
	fieldnum = PRVM_G_FLOAT(OFS_PARM1);
	switch(fieldnum)
	{
		case 0: // active state
			PRVM_G_FLOAT(OFS_RETURN) = cl.entities_active[entnum];
			break;
		case 1: // origin
			Matrix4x4_OriginFromMatrix(&cl.entities[entnum].render.matrix, org);
			VectorCopy(org, PRVM_G_VECTOR(OFS_RETURN));
			break;
		case 2: // forward
			Matrix4x4_ToVectors(&cl.entities[entnum].render.matrix, forward, left, up, org);
			VectorCopy(forward, PRVM_G_VECTOR(OFS_RETURN));
			break;
		case 3: // right
			Matrix4x4_ToVectors(&cl.entities[entnum].render.matrix, forward, left, up, org);
			VectorNegate(left, PRVM_G_VECTOR(OFS_RETURN));
			break;
		case 4: // up
			Matrix4x4_ToVectors(&cl.entities[entnum].render.matrix, forward, left, up, org);
			VectorCopy(up, PRVM_G_VECTOR(OFS_RETURN));
			break;
		case 5: // scale
			PRVM_G_FLOAT(OFS_RETURN) = Matrix4x4_ScaleFromMatrix(&cl.entities[entnum].render.matrix);
			break;
		case 6: // origin + v_forward, v_right, v_up
			Matrix4x4_ToVectors(&cl.entities[entnum].render.matrix, forward, left, up, org);
			VectorCopy(forward, PRVM_clientglobalvector(v_forward));
			VectorNegate(left, PRVM_clientglobalvector(v_right));
			VectorCopy(up, PRVM_clientglobalvector(v_up));
			VectorCopy(org, PRVM_G_VECTOR(OFS_RETURN));
			break;
		case 7: // alpha
			PRVM_G_FLOAT(OFS_RETURN) = cl.entities[entnum].render.alpha;
			break;
		case 8: // colormor
			VectorCopy(cl.entities[entnum].render.colormod, PRVM_G_VECTOR(OFS_RETURN));
			break;
		case 9: // pants colormod
			VectorCopy(cl.entities[entnum].render.colormap_pantscolor, PRVM_G_VECTOR(OFS_RETURN));
			break;
		case 10: // shirt colormod
			VectorCopy(cl.entities[entnum].render.colormap_shirtcolor, PRVM_G_VECTOR(OFS_RETURN));
			break;
		case 11: // skinnum
			PRVM_G_FLOAT(OFS_RETURN) = cl.entities[entnum].render.skinnum;
			break;
		case 12: // mins
			VectorCopy(cl.entities[entnum].render.mins, PRVM_G_VECTOR(OFS_RETURN));
			break;
		case 13: // maxs
			VectorCopy(cl.entities[entnum].render.maxs, PRVM_G_VECTOR(OFS_RETURN));
			break;
		case 14: // absmin
			Matrix4x4_OriginFromMatrix(&cl.entities[entnum].render.matrix, org);
			VectorAdd(cl.entities[entnum].render.mins, org, PRVM_G_VECTOR(OFS_RETURN));
			break;
		case 15: // absmax
			Matrix4x4_OriginFromMatrix(&cl.entities[entnum].render.matrix, org);
			VectorAdd(cl.entities[entnum].render.maxs, org, PRVM_G_VECTOR(OFS_RETURN));
			break;
		case 16: // light
			VectorMA(cl.entities[entnum].render.render_modellight_ambient, 0.5, cl.entities[entnum].render.render_modellight_diffuse, PRVM_G_VECTOR(OFS_RETURN));
			break;
		default:
			PRVM_G_FLOAT(OFS_RETURN) = 0;
			break;
	}
}

//====================
//QC POLYGON functions
//====================

//#304 void() renderscene (EXT_CSQC)
// moved that here to reset the polygons,
// resetting them earlier causes R_Mesh_Draw to be called with numvertices = 0
// --blub
static void VM_CL_R_RenderScene (prvm_prog_t *prog)
{
	qbool ismain = r_refdef.view.ismain;
	double t = Sys_DirtyTime();
	VM_SAFEPARMCOUNT(0, VM_CL_R_RenderScene);

	// update the views
	if(ismain)
	{
		// set the main view
		csqc_main_r_refdef_view = r_refdef.view;
	}

	// now after all of the predraw we know the geometry in the scene mesh and can finalize it for rendering
	CL_MeshEntities_Scene_FinalizeRenderEntity();

	// we need to update any RENDER_VIEWMODEL entities at this point because
	// csqc supplies its own view matrix
	CL_UpdateViewEntities();
	CL_UpdateEntityShading();

	// now draw stuff!
	R_RenderView(0, NULL, NULL, r_refdef.view.x, r_refdef.view.y, r_refdef.view.width, r_refdef.view.height);

	// callprofile fixing hack: do not include this time in what is counted for CSQC_UpdateView
	t = Sys_DirtyTime() - t;if (t < 0 || t >= 1800) t = 0;
	prog->functions[PRVM_clientfunction(CSQC_UpdateView)].totaltime -= t;

	// polygonbegin without draw2d arg has to guess
	prog->polygonbegin_guess2d = false;

	// update the views
	if (ismain)
	{
		// clear the flags so no other view becomes "main" unless CSQC sets VF_MAINVIEW
		r_refdef.view.ismain = false;
		csqc_original_r_refdef_view.ismain = false;
	}
}

//void(string texturename, float flag[, float is2d]) R_BeginPolygon
static void VM_CL_R_PolygonBegin (prvm_prog_t *prog)
{
	const char *texname;
	int drawflags;
	qbool draw2d;
	model_t *mod;

	VM_SAFEPARMCOUNTRANGE(2, 3, VM_CL_R_PolygonBegin);

	texname = PRVM_G_STRING(OFS_PARM0);
	drawflags = (int)PRVM_G_FLOAT(OFS_PARM1);
	if (prog->argc >= 3)
		draw2d = PRVM_G_FLOAT(OFS_PARM2) != 0;
	else
	{
		// weird hacky way to figure out if this is a 2D HUD polygon or a scene
		// polygon, for compatibility with mods aimed at old darkplaces versions
		// - polygonbegin_guess2d is 0 if the most recent major call was
		// clearscene, 1 if the most recent major call was drawpic (and similar)
		// or renderscene
		draw2d = prog->polygonbegin_guess2d;
	}

	// we need to remember whether this is a 2D or 3D mesh we're adding to
	mod = draw2d ? CL_Mesh_UI() : CL_Mesh_Scene();
	prog->polygonbegin_model = mod;
	if (texname == NULL || texname[0] == 0)
		texname = "$whiteimage";
	dp_strlcpy(prog->polygonbegin_texname, texname, sizeof(prog->polygonbegin_texname));
	prog->polygonbegin_drawflags = drawflags;
	prog->polygonbegin_numvertices = 0;
}

//void(vector org, vector texcoords, vector rgb, float alpha) R_PolygonVertex
static void VM_CL_R_PolygonVertex (prvm_prog_t *prog)
{
	const prvm_vec_t *v = PRVM_G_VECTOR(OFS_PARM0);
	const prvm_vec_t *tc = PRVM_G_VECTOR(OFS_PARM1);
	const prvm_vec_t *c = PRVM_G_VECTOR(OFS_PARM2);
	const prvm_vec_t a = PRVM_G_FLOAT(OFS_PARM3);
	float *o;
	model_t *mod = prog->polygonbegin_model;

	VM_SAFEPARMCOUNT(4, VM_CL_R_PolygonVertex);

	if (!mod)
	{
		VM_Warning(prog, "VM_CL_R_PolygonVertex: VM_CL_R_PolygonBegin wasn't called\n");
		return;
	}

	if (prog->polygonbegin_maxvertices <= prog->polygonbegin_numvertices)
	{
		prog->polygonbegin_maxvertices = max(16, prog->polygonbegin_maxvertices * 2);
		prog->polygonbegin_vertexdata = (float *)Mem_Realloc(prog->progs_mempool, prog->polygonbegin_vertexdata, prog->polygonbegin_maxvertices * sizeof(float[10]));
	}
	o = prog->polygonbegin_vertexdata + prog->polygonbegin_numvertices++ * 10;

	o[0] = v[0];
	o[1] = v[1];
	o[2] = v[2];
	o[3] = tc[0];
	o[4] = tc[1];
	o[5] = tc[2];
	o[6] = c[0];
	o[7] = c[1];
	o[8] = c[2];
	o[9] = a;
}

//void() R_EndPolygon
static void VM_CL_R_PolygonEnd (prvm_prog_t *prog)
{
	int i;
	qbool hascolor;
	qbool hasalpha;
	int e0 = 0, e1 = 0, e2 = 0;
	float *o;
	model_t *mod = prog->polygonbegin_model;
	msurface_t *surf;
	texture_t *tex;
	int materialflags;

	VM_SAFEPARMCOUNT(0, VM_CL_R_PolygonEnd);
	if (!mod)
	{
		VM_Warning(prog, "VM_CL_R_PolygonEnd: VM_CL_R_PolygonBegin wasn't called\n");
		return;
	}

	// determine if vertex alpha is being used so we can provide that hint to GetTexture...
	hascolor = false;
	hasalpha = false;
	for (i = 0; i < prog->polygonbegin_numvertices; i++)
	{
		o = prog->polygonbegin_vertexdata + 10 * i;
		if (o[6] != 1.0f || o[7] != 1.0f || o[8] != 1.0f)
			hascolor = true;
		if (o[9] != 1.0f)
			hasalpha = true;
	}

	// create the surface, looking up the best matching texture/shader
	materialflags = MATERIALFLAG_WALL | MATERIALFLAG_FULLBRIGHT;
	if (csqc_polygons_defaultmaterial_nocullface.integer)
		materialflags |= MATERIALFLAG_NOCULLFACE;
	if (hascolor)
		materialflags |= MATERIALFLAG_VERTEXCOLOR;
	if (hasalpha)
		materialflags |= MATERIALFLAG_ALPHAGEN_VERTEX | MATERIALFLAG_ALPHA | MATERIALFLAG_BLENDED | MATERIALFLAG_NOSHADOW;
	tex = Mod_Mesh_GetTexture(mod, prog->polygonbegin_texname, prog->polygonbegin_drawflags, TEXF_ALPHA, materialflags);
	surf = Mod_Mesh_AddSurface(mod, tex, false);
	// create triangle fan
	for (i = 0; i < prog->polygonbegin_numvertices; i++)
	{
		o = prog->polygonbegin_vertexdata + 10 * i;
		e2 = Mod_Mesh_IndexForVertex(mod, surf, o[0], o[1], o[2], 0, 0, 0, o[3], o[4], 0, 0, o[6], o[7], o[8], o[9]);
		if (i >= 2)
			Mod_Mesh_AddTriangle(mod, surf, e0, e1, e2);
		else if (i == 0)
			e0 = e2;
		e1 = e2;
	}
	// build normals (since they are not provided)
	Mod_BuildNormals(surf->num_firstvertex, surf->num_vertices, surf->num_triangles, mod->surfmesh.data_vertex3f, mod->surfmesh.data_element3i + 3 * surf->num_firsttriangle, mod->surfmesh.data_normal3f, true);

	// reset state
	prog->polygonbegin_model = NULL;
	prog->polygonbegin_texname[0] = 0;
	prog->polygonbegin_drawflags = 0;
	prog->polygonbegin_numvertices = 0;
}

/*
=============
CL_CheckBottom

Returns false if any part of the bottom of the entity is off an edge that
is not a staircase.

=============
*/
static qbool CL_CheckBottom (prvm_edict_t *ent)
{
	prvm_prog_t *prog = CLVM_prog;
	vec3_t	mins, maxs, start, stop;
	trace_t	trace;
	int		x, y;
	float	mid, bottom;

	VectorAdd (PRVM_clientedictvector(ent, origin), PRVM_clientedictvector(ent, mins), mins);
	VectorAdd (PRVM_clientedictvector(ent, origin), PRVM_clientedictvector(ent, maxs), maxs);

// if all of the points under the corners are solid world, don't bother
// with the tougher checks
// the corners must be within 16 of the midpoint
	start[2] = mins[2] - 1;
	for	(x=0 ; x<=1 ; x++)
		for	(y=0 ; y<=1 ; y++)
		{
			start[0] = x ? maxs[0] : mins[0];
			start[1] = y ? maxs[1] : mins[1];
			if (!(CL_PointSuperContents(start) & (SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY)))
				goto realcheck;
		}

	return true;		// we got out easy

realcheck:
//
// check it for real...
//
	start[2] = mins[2];

// the midpoint must be within 16 of the bottom
	start[0] = stop[0] = (mins[0] + maxs[0])*0.5;
	start[1] = stop[1] = (mins[1] + maxs[1])*0.5;
	stop[2] = start[2] - 2*sv_stepheight.value;
	trace = CL_TraceLine(start, stop, MOVE_NOMONSTERS, ent, CL_GenericHitSuperContentsMask(ent), 0, 0, collision_extendmovelength.value, true, false, NULL, true, false);

	if (trace.fraction == 1.0)
		return false;
	mid = bottom = trace.endpos[2];

// the corners must be within 16 of the midpoint
	for	(x=0 ; x<=1 ; x++)
		for	(y=0 ; y<=1 ; y++)
		{
			start[0] = stop[0] = x ? maxs[0] : mins[0];
			start[1] = stop[1] = y ? maxs[1] : mins[1];

			trace = CL_TraceLine(start, stop, MOVE_NOMONSTERS, ent, CL_GenericHitSuperContentsMask(ent), 0, 0, collision_extendmovelength.value, true, false, NULL, true, false);

			if (trace.fraction != 1.0 && trace.endpos[2] > bottom)
				bottom = trace.endpos[2];
			if (trace.fraction == 1.0 || mid - trace.endpos[2] > sv_stepheight.value)
				return false;
		}

	return true;
}

/*
=============
CL_movestep

Called by monster program code.
The move will be adjusted for slopes and stairs, but if the move isn't
possible, no move is done and false is returned
=============
*/
static qbool CL_movestep (prvm_edict_t *ent, vec3_t move, qbool relink, qbool noenemy, qbool settrace)
{
	prvm_prog_t *prog = CLVM_prog;
	float		dz;
	vec3_t		oldorg, neworg, end, traceendpos;
	vec3_t		mins, maxs, start;
	trace_t		trace;
	int			i, svent;
	prvm_edict_t		*enemy;

// try the move
	VectorCopy(PRVM_clientedictvector(ent, mins), mins);
	VectorCopy(PRVM_clientedictvector(ent, maxs), maxs);
	VectorCopy (PRVM_clientedictvector(ent, origin), oldorg);
	VectorAdd (PRVM_clientedictvector(ent, origin), move, neworg);

// flying monsters don't step up
	if ( (int)PRVM_clientedictfloat(ent, flags) & (FL_SWIM | FL_FLY) )
	{
	// try one move with vertical motion, then one without
		for (i=0 ; i<2 ; i++)
		{
			VectorAdd (PRVM_clientedictvector(ent, origin), move, neworg);
			enemy = PRVM_PROG_TO_EDICT(PRVM_clientedictedict(ent, enemy));
			if (i == 0 && enemy != prog->edicts)
			{
				dz = PRVM_clientedictvector(ent, origin)[2] - PRVM_clientedictvector(PRVM_PROG_TO_EDICT(PRVM_clientedictedict(ent, enemy)), origin)[2];
				if (dz > 40)
					neworg[2] -= 8;
				if (dz < 30)
					neworg[2] += 8;
			}
			VectorCopy(PRVM_clientedictvector(ent, origin), start);
			trace = CL_TraceBox(start, mins, maxs, neworg, MOVE_NORMAL, ent, CL_GenericHitSuperContentsMask(ent), 0, 0, collision_extendmovelength.value, true, true, &svent, true);
			if (settrace)
				CL_VM_SetTraceGlobals(prog, &trace, svent);

			if (trace.fraction == 1)
			{
				VectorCopy(trace.endpos, traceendpos);
				if (((int)PRVM_clientedictfloat(ent, flags) & FL_SWIM) && !(CL_PointSuperContents(traceendpos) & SUPERCONTENTS_LIQUIDSMASK))
					return false;	// swim monster left water

				VectorCopy (traceendpos, PRVM_clientedictvector(ent, origin));
				if (relink)
					CL_LinkEdict(ent);
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

	trace = CL_TraceBox(neworg, mins, maxs, end, MOVE_NORMAL, ent, CL_GenericHitSuperContentsMask(ent), 0, 0, collision_extendmovelength.value, true, true, &svent, true);
	if (settrace)
		CL_VM_SetTraceGlobals(prog, &trace, svent);

	if (trace.startsolid)
	{
		neworg[2] -= sv_stepheight.value;
		trace = CL_TraceBox(neworg, mins, maxs, end, MOVE_NORMAL, ent, CL_GenericHitSuperContentsMask(ent), 0, 0, collision_extendmovelength.value, true, true, &svent, true);
		if (settrace)
			CL_VM_SetTraceGlobals(prog, &trace, svent);
		if (trace.startsolid)
			return false;
	}
	if (trace.fraction == 1)
	{
	// if monster had the ground pulled out, go ahead and fall
		if ( (int)PRVM_clientedictfloat(ent, flags) & FL_PARTIALGROUND )
		{
			VectorAdd (PRVM_clientedictvector(ent, origin), move, PRVM_clientedictvector(ent, origin));
			if (relink)
				CL_LinkEdict(ent);
			PRVM_clientedictfloat(ent, flags) = (int)PRVM_clientedictfloat(ent, flags) & ~FL_ONGROUND;
			return true;
		}

		return false;		// walked off an edge
	}

// check point traces down for dangling corners
	VectorCopy (trace.endpos, PRVM_clientedictvector(ent, origin));

	if (!CL_CheckBottom (ent))
	{
		if ( (int)PRVM_clientedictfloat(ent, flags) & FL_PARTIALGROUND )
		{	// entity had floor mostly pulled out from underneath it
			// and is trying to correct
			if (relink)
				CL_LinkEdict(ent);
			return true;
		}
		VectorCopy (oldorg, PRVM_clientedictvector(ent, origin));
		return false;
	}

	if ( (int)PRVM_clientedictfloat(ent, flags) & FL_PARTIALGROUND )
		PRVM_clientedictfloat(ent, flags) = (int)PRVM_clientedictfloat(ent, flags) & ~FL_PARTIALGROUND;

	PRVM_clientedictedict(ent, groundentity) = PRVM_EDICT_TO_PROG(trace.ent);

// the move is ok
	if (relink)
		CL_LinkEdict(ent);
	return true;
}

/*
===============
VM_CL_walkmove

float(float yaw, float dist[, settrace]) walkmove
===============
*/
static void VM_CL_walkmove (prvm_prog_t *prog)
{
	prvm_edict_t	*ent;
	float	yaw, dist;
	vec3_t	move;
	mfunction_t	*oldf;
	int 	oldself;
	qbool	settrace;

	VM_SAFEPARMCOUNTRANGE(2, 3, VM_CL_walkmove);

	// assume failure if it returns early
	PRVM_G_FLOAT(OFS_RETURN) = 0;

	ent = PRVM_PROG_TO_EDICT(PRVM_clientglobaledict(self));
	if (ent == prog->edicts)
	{
		VM_Warning(prog, "walkmove: can not modify world entity\n");
		return;
	}
	if (ent->free)
	{
		VM_Warning(prog, "walkmove: can not modify free entity\n");
		return;
	}
	yaw = PRVM_G_FLOAT(OFS_PARM0);
	dist = PRVM_G_FLOAT(OFS_PARM1);
	settrace = prog->argc >= 3 && PRVM_G_FLOAT(OFS_PARM2);

	if ( !( (int)PRVM_clientedictfloat(ent, flags) & (FL_ONGROUND|FL_FLY|FL_SWIM) ) )
		return;

	yaw = yaw*M_PI*2 / 360;

	move[0] = cos(yaw)*dist;
	move[1] = sin(yaw)*dist;
	move[2] = 0;

// save program state, because CL_movestep may call other progs
	oldf = prog->xfunction;
	oldself = PRVM_clientglobaledict(self);

	PRVM_G_FLOAT(OFS_RETURN) = CL_movestep(ent, move, true, false, settrace);


// restore program state
	prog->xfunction = oldf;
	PRVM_clientglobaledict(self) = oldself;
}

/*
===============
VM_CL_serverkey

string(string key) serverkey
===============
*/
static void VM_CL_serverkey(prvm_prog_t *prog)
{
	char string[VM_TEMPSTRING_MAXSIZE];
	VM_SAFEPARMCOUNT(1, VM_CL_serverkey);
	InfoString_GetValue(cl.qw_serverinfo, PRVM_G_STRING(OFS_PARM0), string, sizeof(string));
	PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, string, strlen(string));
}

/*
=================
VM_CL_checkpvs

Checks if an entity is in a point's PVS.
Should be fast but can be inexact.

float checkpvs(vector viewpos, entity viewee) = #240;
=================
*/
static void VM_CL_checkpvs (prvm_prog_t *prog)
{
	vec3_t viewpos;
	prvm_edict_t *viewee;
	vec3_t mi, ma;
#if 1
	unsigned char *pvs;
#else
	unsigned char *fatpvs = NULL;
#endif

	VM_SAFEPARMCOUNT(2, VM_CL_checkpvs);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), viewpos);
	viewee = PRVM_G_EDICT(OFS_PARM1);

	if(viewee->free)
	{
		VM_Warning(prog, "checkpvs: can not check free entity\n");
		PRVM_G_FLOAT(OFS_RETURN) = 4;
		return;
	}

	VectorAdd(PRVM_serveredictvector(viewee, origin), PRVM_serveredictvector(viewee, mins), mi);
	VectorAdd(PRVM_serveredictvector(viewee, origin), PRVM_serveredictvector(viewee, maxs), ma);

#if 1
	if(!cl.worldmodel || !cl.worldmodel->brush.GetPVS || !cl.worldmodel->brush.BoxTouchingPVS)
	{
		// no PVS support on this worldmodel... darn
		PRVM_G_FLOAT(OFS_RETURN) = 3;
		return;
	}
	pvs = cl.worldmodel->brush.GetPVS(cl.worldmodel, viewpos);
	if(!pvs)
	{
		// viewpos isn't in any PVS... darn
		PRVM_G_FLOAT(OFS_RETURN) = 2;
		return;
	}
	PRVM_G_FLOAT(OFS_RETURN) = cl.worldmodel->brush.BoxTouchingPVS(cl.worldmodel, pvs, mi, ma);
#else
	// using fat PVS like FTEQW does (slow)
	if(!cl.worldmodel || !cl.worldmodel->brush.FatPVS || !cl.worldmodel->brush.BoxTouchingPVS)
	{
		// no PVS support on this worldmodel... darn
		PRVM_G_FLOAT(OFS_RETURN) = 3;
		return;
	}
	cl.worldmodel->brush.FatPVS(cl.worldmodel, viewpos, 8, &fatpvs, cls.levelmempool, false);
	if(!fatpvs)
	{
		// viewpos isn't in any PVS... darn
		PRVM_G_FLOAT(OFS_RETURN) = 2;
		return;
	}
	PRVM_G_FLOAT(OFS_RETURN) = cl.worldmodel->brush.BoxTouchingPVS(cl.worldmodel, fatpvs, mi, ma);
#endif
}

// #263 float(float modlindex) skel_create = #263; // (FTE_CSQC_SKELETONOBJECTS) create a skeleton (be sure to assign this value into .skeletonindex for use), returns skeleton index (1 or higher) on success, returns 0 on failure  (for example if the modelindex is not skeletal), it is recommended that you create a new skeleton if you change modelindex.
static void VM_CL_skel_create(prvm_prog_t *prog)
{
	int modelindex = (int)PRVM_G_FLOAT(OFS_PARM0);
	model_t *model = CL_GetModelByIndex(modelindex);
	skeleton_t *skeleton;
	int i;
	PRVM_G_FLOAT(OFS_RETURN) = 0;
	if (!model || !model->num_bones)
		return;
	for (i = 0;i < MAX_EDICTS;i++)
		if (!prog->skeletons[i])
			break;
	if (i == MAX_EDICTS)
		return;
	prog->skeletons[i] = skeleton = (skeleton_t *)Mem_Alloc(cls.levelmempool, sizeof(skeleton_t) + model->num_bones * sizeof(matrix4x4_t));
	PRVM_G_FLOAT(OFS_RETURN) = i + 1;
	skeleton->model = model;
	skeleton->relativetransforms = (matrix4x4_t *)(skeleton+1);
	// initialize to identity matrices
	for (i = 0;i < skeleton->model->num_bones;i++)
		skeleton->relativetransforms[i] = identitymatrix;
}

// #264 float(float skel, entity ent, float modlindex, float retainfrac, float firstbone, float lastbone) skel_build = #264; // (FTE_CSQC_SKELETONOBJECTS) blend in a percentage of standard animation, 0 replaces entirely, 1 does nothing, 0.5 blends half, etc, and this only alters the bones in the specified range for which out of bounds values like 0,100000 are safe (uses .frame, .frame2, .frame3, .frame4, .lerpfrac, .lerpfrac3, .lerpfrac4, .frame1time, .frame2time, .frame3time, .frame4time), returns skel on success, 0 on failure
static void VM_CL_skel_build(prvm_prog_t *prog)
{
	int skeletonindex = (int)PRVM_G_FLOAT(OFS_PARM0) - 1;
	skeleton_t *skeleton;
	prvm_edict_t *ed = PRVM_G_EDICT(OFS_PARM1);
	int modelindex = (int)PRVM_G_FLOAT(OFS_PARM2);
	float retainfrac = PRVM_G_FLOAT(OFS_PARM3);
	int firstbone = PRVM_G_FLOAT(OFS_PARM4) - 1;
	int lastbone = PRVM_G_FLOAT(OFS_PARM5) - 1;
	model_t *model = CL_GetModelByIndex(modelindex);
	int numblends;
	int bonenum;
	int blendindex;
	framegroupblend_t framegroupblend[MAX_FRAMEGROUPBLENDS];
	frameblend_t frameblend[MAX_FRAMEBLENDS];
	matrix4x4_t bonematrix;
	matrix4x4_t matrix;
	PRVM_G_FLOAT(OFS_RETURN) = 0;
	if (skeletonindex < 0 || skeletonindex >= MAX_EDICTS || !(skeleton = prog->skeletons[skeletonindex]))
		return;
	firstbone = max(0, firstbone);
	lastbone = min(lastbone, model->num_bones - 1);
	lastbone = min(lastbone, skeleton->model->num_bones - 1);
	VM_GenerateFrameGroupBlend(prog, framegroupblend, ed);
	VM_FrameBlendFromFrameGroupBlend(frameblend, framegroupblend, model, cl.time);
	for (numblends = 0;numblends < MAX_FRAMEBLENDS && frameblend[numblends].lerp;numblends++)
		;
	for (bonenum = firstbone;bonenum <= lastbone;bonenum++)
	{
		memset(&bonematrix, 0, sizeof(bonematrix));
		for (blendindex = 0;blendindex < numblends;blendindex++)
		{
			Matrix4x4_FromBonePose7s(&matrix, model->num_posescale, model->data_poses7s + 7 * (frameblend[blendindex].subframe * model->num_bones + bonenum));
			Matrix4x4_Accumulate(&bonematrix, &matrix, frameblend[blendindex].lerp);
		}
		Matrix4x4_Normalize3(&bonematrix, &bonematrix);
		Matrix4x4_Interpolate(&skeleton->relativetransforms[bonenum], &bonematrix, &skeleton->relativetransforms[bonenum], retainfrac);
	}
	PRVM_G_FLOAT(OFS_RETURN) = skeletonindex + 1;
}

// #265 float(float skel) skel_get_numbones = #265; // (FTE_CSQC_SKELETONOBJECTS) returns how many bones exist in the created skeleton
static void VM_CL_skel_get_numbones(prvm_prog_t *prog)
{
	int skeletonindex = (int)PRVM_G_FLOAT(OFS_PARM0) - 1;
	skeleton_t *skeleton;
	PRVM_G_FLOAT(OFS_RETURN) = 0;
	if (skeletonindex < 0 || skeletonindex >= MAX_EDICTS || !(skeleton = prog->skeletons[skeletonindex]))
		return;
	PRVM_G_FLOAT(OFS_RETURN) = skeleton->model->num_bones;
}

// #266 string(float skel, float bonenum) skel_get_bonename = #266; // (FTE_CSQC_SKELETONOBJECTS) returns name of bone (as a tempstring)
static void VM_CL_skel_get_bonename(prvm_prog_t *prog)
{
	int skeletonindex = (int)PRVM_G_FLOAT(OFS_PARM0) - 1;
	int bonenum = (int)PRVM_G_FLOAT(OFS_PARM1) - 1;
	skeleton_t *skeleton;
	PRVM_G_INT(OFS_RETURN) = 0;
	if (skeletonindex < 0 || skeletonindex >= MAX_EDICTS || !(skeleton = prog->skeletons[skeletonindex]))
		return;
	if (bonenum < 0 || bonenum >= skeleton->model->num_bones)
		return;
	PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, skeleton->model->data_bones[bonenum].name, strlen(skeleton->model->data_bones[bonenum].name));
}

// #267 float(float skel, float bonenum) skel_get_boneparent = #267; // (FTE_CSQC_SKELETONOBJECTS) returns parent num for supplied bonenum, 0 if bonenum has no parent or bone does not exist (returned value is always less than bonenum, you can loop on this)
static void VM_CL_skel_get_boneparent(prvm_prog_t *prog)
{
	int skeletonindex = (int)PRVM_G_FLOAT(OFS_PARM0) - 1;
	int bonenum = (int)PRVM_G_FLOAT(OFS_PARM1) - 1;
	skeleton_t *skeleton;
	PRVM_G_FLOAT(OFS_RETURN) = 0;
	if (skeletonindex < 0 || skeletonindex >= MAX_EDICTS || !(skeleton = prog->skeletons[skeletonindex]))
		return;
	if (bonenum < 0 || bonenum >= skeleton->model->num_bones)
		return;
	PRVM_G_FLOAT(OFS_RETURN) = skeleton->model->data_bones[bonenum].parent + 1;
}

// #268 float(float skel, string tagname) skel_find_bone = #268; // (FTE_CSQC_SKELETONOBJECTS) get number of bone with specified name, 0 on failure, tagindex (bonenum+1) on success, same as using gettagindex on the modelindex
static void VM_CL_skel_find_bone(prvm_prog_t *prog)
{
	int skeletonindex = (int)PRVM_G_FLOAT(OFS_PARM0) - 1;
	const char *tagname = PRVM_G_STRING(OFS_PARM1);
	skeleton_t *skeleton;
	PRVM_G_FLOAT(OFS_RETURN) = 0;
	if (skeletonindex < 0 || skeletonindex >= MAX_EDICTS || !(skeleton = prog->skeletons[skeletonindex]))
		return;
	PRVM_G_FLOAT(OFS_RETURN) = Mod_Alias_GetTagIndexForName(skeleton->model, 0, tagname);
}

// #269 vector(float skel, float bonenum) skel_get_bonerel = #269; // (FTE_CSQC_SKELETONOBJECTS) get matrix of bone in skeleton relative to its parent - sets v_forward, v_right, v_up, returns origin (relative to parent bone)
static void VM_CL_skel_get_bonerel(prvm_prog_t *prog)
{
	int skeletonindex = (int)PRVM_G_FLOAT(OFS_PARM0) - 1;
	int bonenum = (int)PRVM_G_FLOAT(OFS_PARM1) - 1;
	skeleton_t *skeleton;
	matrix4x4_t matrix;
	vec3_t forward, left, up, origin;
	VectorClear(PRVM_G_VECTOR(OFS_RETURN));
	VectorClear(PRVM_clientglobalvector(v_forward));
	VectorClear(PRVM_clientglobalvector(v_right));
	VectorClear(PRVM_clientglobalvector(v_up));
	if (skeletonindex < 0 || skeletonindex >= MAX_EDICTS || !(skeleton = prog->skeletons[skeletonindex]))
		return;
	if (bonenum < 0 || bonenum >= skeleton->model->num_bones)
		return;
	matrix = skeleton->relativetransforms[bonenum];
	Matrix4x4_ToVectors(&matrix, forward, left, up, origin);
	VectorCopy(forward, PRVM_clientglobalvector(v_forward));
	VectorNegate(left, PRVM_clientglobalvector(v_right));
	VectorCopy(up, PRVM_clientglobalvector(v_up));
	VectorCopy(origin, PRVM_G_VECTOR(OFS_RETURN));
}

// #270 vector(float skel, float bonenum) skel_get_boneabs = #270; // (FTE_CSQC_SKELETONOBJECTS) get matrix of bone in skeleton in model space - sets v_forward, v_right, v_up, returns origin (relative to entity)
static void VM_CL_skel_get_boneabs(prvm_prog_t *prog)
{
	int skeletonindex = (int)PRVM_G_FLOAT(OFS_PARM0) - 1;
	int bonenum = (int)PRVM_G_FLOAT(OFS_PARM1) - 1;
	skeleton_t *skeleton;
	matrix4x4_t matrix;
	matrix4x4_t temp;
	vec3_t forward, left, up, origin;
	VectorClear(PRVM_G_VECTOR(OFS_RETURN));
	VectorClear(PRVM_clientglobalvector(v_forward));
	VectorClear(PRVM_clientglobalvector(v_right));
	VectorClear(PRVM_clientglobalvector(v_up));
	if (skeletonindex < 0 || skeletonindex >= MAX_EDICTS || !(skeleton = prog->skeletons[skeletonindex]))
		return;
	if (bonenum < 0 || bonenum >= skeleton->model->num_bones)
		return;
	matrix = skeleton->relativetransforms[bonenum];
	// convert to absolute
	while ((bonenum = skeleton->model->data_bones[bonenum].parent) >= 0)
	{
		temp = matrix;
		Matrix4x4_Concat(&matrix, &skeleton->relativetransforms[bonenum], &temp);
	}
	Matrix4x4_ToVectors(&matrix, forward, left, up, origin);
	VectorCopy(forward, PRVM_clientglobalvector(v_forward));
	VectorNegate(left, PRVM_clientglobalvector(v_right));
	VectorCopy(up, PRVM_clientglobalvector(v_up));
	VectorCopy(origin, PRVM_G_VECTOR(OFS_RETURN));
}

// #271 void(float skel, float bonenum, vector org) skel_set_bone = #271; // (FTE_CSQC_SKELETONOBJECTS) set matrix of bone relative to its parent, reads v_forward, v_right, v_up, takes origin as parameter (relative to parent bone)
static void VM_CL_skel_set_bone(prvm_prog_t *prog)
{
	int skeletonindex = (int)PRVM_G_FLOAT(OFS_PARM0) - 1;
	int bonenum = (int)PRVM_G_FLOAT(OFS_PARM1) - 1;
	vec3_t forward, left, up, origin;
	skeleton_t *skeleton;
	matrix4x4_t matrix;
	if (skeletonindex < 0 || skeletonindex >= MAX_EDICTS || !(skeleton = prog->skeletons[skeletonindex]))
		return;
	if (bonenum < 0 || bonenum >= skeleton->model->num_bones)
		return;
	VectorCopy(PRVM_clientglobalvector(v_forward), forward);
	VectorNegate(PRVM_clientglobalvector(v_right), left);
	VectorCopy(PRVM_clientglobalvector(v_up), up);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), origin);
	Matrix4x4_FromVectors(&matrix, forward, left, up, origin);
	skeleton->relativetransforms[bonenum] = matrix;
}

// #272 void(float skel, float bonenum, vector org) skel_mul_bone = #272; // (FTE_CSQC_SKELETONOBJECTS) transform bone matrix (relative to its parent) by the supplied matrix in v_forward, v_right, v_up, takes origin as parameter (relative to parent bone)
static void VM_CL_skel_mul_bone(prvm_prog_t *prog)
{
	int skeletonindex = (int)PRVM_G_FLOAT(OFS_PARM0) - 1;
	int bonenum = (int)PRVM_G_FLOAT(OFS_PARM1) - 1;
	vec3_t forward, left, up, origin;
	skeleton_t *skeleton;
	matrix4x4_t matrix;
	matrix4x4_t temp;
	if (skeletonindex < 0 || skeletonindex >= MAX_EDICTS || !(skeleton = prog->skeletons[skeletonindex]))
		return;
	if (bonenum < 0 || bonenum >= skeleton->model->num_bones)
		return;
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), origin);
	VectorCopy(PRVM_clientglobalvector(v_forward), forward);
	VectorNegate(PRVM_clientglobalvector(v_right), left);
	VectorCopy(PRVM_clientglobalvector(v_up), up);
	Matrix4x4_FromVectors(&matrix, forward, left, up, origin);
	temp = skeleton->relativetransforms[bonenum];
	Matrix4x4_Concat(&skeleton->relativetransforms[bonenum], &matrix, &temp);
}

// #273 void(float skel, float startbone, float endbone, vector org) skel_mul_bones = #273; // (FTE_CSQC_SKELETONOBJECTS) transform bone matrices (relative to their parents) by the supplied matrix in v_forward, v_right, v_up, takes origin as parameter (relative to parent bones)
static void VM_CL_skel_mul_bones(prvm_prog_t *prog)
{
	int skeletonindex = (int)PRVM_G_FLOAT(OFS_PARM0) - 1;
	int firstbone = PRVM_G_FLOAT(OFS_PARM1) - 1;
	int lastbone = PRVM_G_FLOAT(OFS_PARM2) - 1;
	int bonenum;
	vec3_t forward, left, up, origin;
	skeleton_t *skeleton;
	matrix4x4_t matrix;
	matrix4x4_t temp;
	if (skeletonindex < 0 || skeletonindex >= MAX_EDICTS || !(skeleton = prog->skeletons[skeletonindex]))
		return;
	VectorCopy(PRVM_G_VECTOR(OFS_PARM3), origin);
	VectorCopy(PRVM_clientglobalvector(v_forward), forward);
	VectorNegate(PRVM_clientglobalvector(v_right), left);
	VectorCopy(PRVM_clientglobalvector(v_up), up);
	Matrix4x4_FromVectors(&matrix, forward, left, up, origin);
	firstbone = max(0, firstbone);
	lastbone = min(lastbone, skeleton->model->num_bones - 1);
	for (bonenum = firstbone;bonenum <= lastbone;bonenum++)
	{
		temp = skeleton->relativetransforms[bonenum];
		Matrix4x4_Concat(&skeleton->relativetransforms[bonenum], &matrix, &temp);
	}
}

// #274 void(float skeldst, float skelsrc, float startbone, float endbone) skel_copybones = #274; // (FTE_CSQC_SKELETONOBJECTS) copy bone matrices (relative to their parents) from one skeleton to another, useful for copying a skeleton to a corpse
static void VM_CL_skel_copybones(prvm_prog_t *prog)
{
	int skeletonindexdst = (int)PRVM_G_FLOAT(OFS_PARM0) - 1;
	int skeletonindexsrc = (int)PRVM_G_FLOAT(OFS_PARM1) - 1;
	int firstbone = PRVM_G_FLOAT(OFS_PARM2) - 1;
	int lastbone = PRVM_G_FLOAT(OFS_PARM3) - 1;
	int bonenum;
	skeleton_t *skeletondst;
	skeleton_t *skeletonsrc;
	if (skeletonindexdst < 0 || skeletonindexdst >= MAX_EDICTS || !(skeletondst = prog->skeletons[skeletonindexdst]))
		return;
	if (skeletonindexsrc < 0 || skeletonindexsrc >= MAX_EDICTS || !(skeletonsrc = prog->skeletons[skeletonindexsrc]))
		return;
	firstbone = max(0, firstbone);
	lastbone = min(lastbone, skeletondst->model->num_bones - 1);
	lastbone = min(lastbone, skeletonsrc->model->num_bones - 1);
	for (bonenum = firstbone;bonenum <= lastbone;bonenum++)
		skeletondst->relativetransforms[bonenum] = skeletonsrc->relativetransforms[bonenum];
}

// #275 void(float skel) skel_delete = #275; // (FTE_CSQC_SKELETONOBJECTS) deletes skeleton at the beginning of the next frame (you can add the entity, delete the skeleton, renderscene, and it will still work)
static void VM_CL_skel_delete(prvm_prog_t *prog)
{
	int skeletonindex = (int)PRVM_G_FLOAT(OFS_PARM0) - 1;
	skeleton_t *skeleton;
	if (skeletonindex < 0 || skeletonindex >= MAX_EDICTS || !(skeleton = prog->skeletons[skeletonindex]))
		return;
	Mem_Free(skeleton);
	prog->skeletons[skeletonindex] = NULL;
}

// #276 float(float modlindex, string framename) frameforname = #276; // (FTE_CSQC_SKELETONOBJECTS) finds number of a specified frame in the animation, returns -1 if no match found
static void VM_CL_frameforname(prvm_prog_t *prog)
{
	int modelindex = (int)PRVM_G_FLOAT(OFS_PARM0);
	model_t *model = CL_GetModelByIndex(modelindex);
	const char *name = PRVM_G_STRING(OFS_PARM1);
	int i;
	PRVM_G_FLOAT(OFS_RETURN) = -1;
	if (!model || !model->animscenes)
		return;
	for (i = 0;i < model->numframes;i++)
	{
		if (!strcasecmp(model->animscenes[i].name, name))
		{
			PRVM_G_FLOAT(OFS_RETURN) = i;
			break;
		}
	}
}

// #277 float(float modlindex, float framenum) frameduration = #277; // (FTE_CSQC_SKELETONOBJECTS) returns the intended play time (in seconds) of the specified framegroup, if it does not exist the result is 0, if it is a single frame it may be a small value around 0.1 or 0.
static void VM_CL_frameduration(prvm_prog_t *prog)
{
	int modelindex = (int)PRVM_G_FLOAT(OFS_PARM0);
	model_t *model = CL_GetModelByIndex(modelindex);
	int framenum = (int)PRVM_G_FLOAT(OFS_PARM1);
	PRVM_G_FLOAT(OFS_RETURN) = 0;
	if (!model || !model->animscenes || framenum < 0 || framenum >= model->numframes)
		return;
	if (model->animscenes[framenum].framerate)
		PRVM_G_FLOAT(OFS_RETURN) = model->animscenes[framenum].framecount / model->animscenes[framenum].framerate;
}

static void VM_CL_RotateMoves(prvm_prog_t *prog)
{
	/*
	 * Obscure builtin used by GAME_XONOTIC.
	 *
	 * Edits the input history of cl_movement by rotating all move commands
	 * currently in the queue using the given transform.
	 *
	 * The vector passed is an "angles transform" as used by warpzonelib, i.e.
	 * v_angle-like (non-inverted) euler angles that perform the rotation
	 * of the space that is to be done.
	 *
	 * This is meant to be used as a fixangle replacement after passing
	 * through a warpzone/portal: the client is told about the warp transform,
	 * and calls this function in the same frame as the one on which the
	 * client's origin got changed by the serverside teleport. Then this code
	 * transforms the pre-warp input (which matches the empty space behind
	 * the warp plane) into post-warp input (which matches the target area
	 * of the warp). Also, at the same time, the client has to use
	 * R_SetView to adjust VF_CL_VIEWANGLES according to the same transform.
	 *
	 * This together allows warpzone motion to be perfectly predicted by
	 * the client!
	 *
	 * Furthermore, for perfect warpzone behaviour, the server side also
	 * has to detect input the client sent before it received the origin
	 * update, but after the warp occurred on the server, and has to adjust
	 * input appropriately.
    */
	matrix4x4_t m;
	vec3_t v = {0, 0, 0};
	vec3_t a, x, y, z;
	VM_SAFEPARMCOUNT(1, VM_CL_RotateMoves);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), a);
	AngleVectorsFLU(a, x, y, z);
	Matrix4x4_FromVectors(&m, x, y, z, v);
	CL_RotateMoves(&m);
}

// #358 void(string cubemapname) loadcubemap
static void VM_CL_loadcubemap(prvm_prog_t *prog)
{
	const char *name;

	VM_SAFEPARMCOUNT(1, VM_CL_loadcubemap);
	name = PRVM_G_STRING(OFS_PARM0);
	R_GetCubemap(name);
}

#define REFDEFFLAG_TELEPORTED 1
#define REFDEFFLAG_JUMPING 2
#define REFDEFFLAG_DEAD 4
#define REFDEFFLAG_INTERMISSION 8
static void VM_CL_V_CalcRefdef(prvm_prog_t *prog)
{
	matrix4x4_t entrendermatrix;
	vec3_t clviewangles;
	vec3_t clvelocity;
	qbool teleported;
	qbool clonground;
	qbool clcmdjump;
	qbool cldead;
	float clstatsviewheight;
	prvm_edict_t *ent;
	int flags;

	VM_SAFEPARMCOUNT(2, VM_CL_V_CalcRefdef);
	ent = PRVM_G_EDICT(OFS_PARM0);
	flags = PRVM_G_FLOAT(OFS_PARM1);

	// use the CL_GetTagMatrix function on self to ensure consistent behavior (duplicate code would be bad)
	CL_GetTagMatrix(prog, &entrendermatrix, ent, 0, NULL);

	VectorCopy(cl.csqc_viewangles, clviewangles);
	teleported = (flags & REFDEFFLAG_TELEPORTED) != 0;
	clonground = ((int)PRVM_clientedictfloat(ent, pmove_flags) & PMF_ONGROUND) != 0;
	clcmdjump = (flags & REFDEFFLAG_JUMPING) != 0;
	clstatsviewheight = PRVM_clientedictvector(ent, view_ofs)[2];
	cldead = (flags & REFDEFFLAG_DEAD) != 0;
	cl.intermission = (flags & REFDEFFLAG_INTERMISSION) != 0;
	VectorCopy(PRVM_clientedictvector(ent, velocity), clvelocity);

	V_CalcRefdefUsing(&entrendermatrix, clviewangles, teleported, clonground, clcmdjump, clstatsviewheight, cldead, clvelocity);

	VectorCopy(cl.csqc_vieworiginfromengine, cl.csqc_vieworigin);
	VectorCopy(cl.csqc_viewanglesfromengine, cl.csqc_viewangles);
	CSQC_R_RecalcView();
}

//============================================================================

// To create a almost working builtin file from this replace:
// "^NULL.*" with ""
// "^{.*//.*}:Wh\(.*\)" with "\1"
// "\:" with "//"
// "^.*//:Wh{\#:d*}:Wh{.*}" with "\2 = \1;"
// "\n\n+" with "\n\n"

prvm_builtin_t vm_cl_builtins[] = {
NULL,							// #0 NULL function (not callable) (QUAKE)
VM_CL_makevectors,				// #1 void(vector ang) makevectors (QUAKE)
VM_CL_setorigin,				// #2 void(entity e, vector o) setorigin (QUAKE)
VM_CL_setmodel,					// #3 void(entity e, string m) setmodel (QUAKE)
VM_CL_setsize,					// #4 void(entity e, vector min, vector max) setsize (QUAKE)
NULL,							// #5 void(entity e, vector min, vector max) setabssize (QUAKE)
VM_break,						// #6 void() break (QUAKE)
VM_random,						// #7 float() random (QUAKE)
VM_CL_sound,					// #8 void(entity e, float chan, string samp, float volume, float atten[, float pitchchange[, float flags]]) sound (QUAKE)
VM_normalize,					// #9 vector(vector v) normalize (QUAKE)
VM_error,						// #10 void(string e) error (QUAKE)
VM_objerror,					// #11 void(string e) objerror (QUAKE)
VM_vlen,						// #12 float(vector v) vlen (QUAKE)
VM_vectoyaw,					// #13 float(vector v) vectoyaw (QUAKE)
VM_CL_spawn,					// #14 entity() spawn (QUAKE)
VM_remove,						// #15 void(entity e) remove (QUAKE)
VM_CL_traceline,				// #16 void(vector v1, vector v2, float tryents, entity ignoreentity) traceline (QUAKE)
NULL,							// #17 entity() checkclient (QUAKE)
VM_find,						// #18 entity(entity start, .string fld, string match) find (QUAKE)
VM_precache_sound,				// #19 void(string s) precache_sound (QUAKE)
VM_CL_precache_model,			// #20 void(string s) precache_model (QUAKE)
NULL,							// #21 void(entity client, string s, ...) stuffcmd (QUAKE)
VM_CL_findradius,				// #22 entity(vector org, float rad) findradius (QUAKE)
NULL,							// #23 void(string s, ...) bprint (QUAKE)
NULL,							// #24 void(entity client, string s, ...) sprint (QUAKE)
VM_dprint,						// #25 void(string s, ...) dprint (QUAKE)
VM_ftos,						// #26 string(float f) ftos (QUAKE)
VM_vtos,						// #27 string(vector v) vtos (QUAKE)
VM_coredump,					// #28 void() coredump (QUAKE)
VM_traceon,						// #29 void() traceon (QUAKE)
VM_traceoff,					// #30 void() traceoff (QUAKE)
VM_eprint,						// #31 void(entity e) eprint (QUAKE)
VM_CL_walkmove,					// #32 float(float yaw, float dist[, float settrace]) walkmove (QUAKE)
NULL,							// #33 (QUAKE)
VM_CL_droptofloor,				// #34 float() droptofloor (QUAKE)
VM_CL_lightstyle,				// #35 void(float style, string value) lightstyle (QUAKE)
VM_rint,						// #36 float(float v) rint (QUAKE)
VM_floor,						// #37 float(float v) floor (QUAKE)
VM_ceil,						// #38 float(float v) ceil (QUAKE)
NULL,							// #39 (QUAKE)
VM_CL_checkbottom,				// #40 float(entity e) checkbottom (QUAKE)
VM_CL_pointcontents,			// #41 float(vector v) pointcontents (QUAKE)
NULL,							// #42 (QUAKE)
VM_fabs,						// #43 float(float f) fabs (QUAKE)
NULL,							// #44 vector(entity e, float speed) aim (QUAKE)
VM_cvar,						// #45 float(string s) cvar (QUAKE)
VM_localcmd,					// #46 void(string s) localcmd (QUAKE)
VM_nextent,						// #47 entity(entity e) nextent (QUAKE)
VM_CL_particle,					// #48 void(vector o, vector d, float color, float count) particle (QUAKE)
VM_changeyaw,					// #49 void() ChangeYaw (QUAKE)
NULL,							// #50 (QUAKE)
VM_vectoangles,					// #51 vector(vector v) vectoangles (QUAKE)
NULL,							// #52 void(float to, float f) WriteByte (QUAKE)
NULL,							// #53 void(float to, float f) WriteChar (QUAKE)
NULL,							// #54 void(float to, float f) WriteShort (QUAKE)
NULL,							// #55 void(float to, float f) WriteLong (QUAKE)
NULL,							// #56 void(float to, float f) WriteCoord (QUAKE)
NULL,							// #57 void(float to, float f) WriteAngle (QUAKE)
NULL,							// #58 void(float to, string s) WriteString (QUAKE)
NULL,							// #59 (QUAKE)
VM_sin,							// #60 float(float f) sin (DP_QC_SINCOSSQRTPOW)
VM_cos,							// #61 float(float f) cos (DP_QC_SINCOSSQRTPOW)
VM_sqrt,						// #62 float(float f) sqrt (DP_QC_SINCOSSQRTPOW)
VM_changepitch,					// #63 void(entity ent) changepitch (DP_QC_CHANGEPITCH)
VM_CL_tracetoss,				// #64 void(entity e, entity ignore) tracetoss (DP_QC_TRACETOSS)
VM_etos,						// #65 string(entity ent) etos (DP_QC_ETOS)
NULL,							// #66 (QUAKE)
NULL,							// #67 void(float step) movetogoal (QUAKE)
VM_precache_file,				// #68 string(string s) precache_file (QUAKE)
VM_CL_makestatic,				// #69 void(entity e) makestatic (QUAKE)
NULL,							// #70 void(string s) changelevel (QUAKE)
NULL,							// #71 (QUAKE)
VM_cvar_set,					// #72 void(string var, string val) cvar_set (QUAKE)
NULL,							// #73 void(entity client, strings) centerprint (QUAKE)
VM_CL_ambientsound,				// #74 void(vector pos, string samp, float vol, float atten) ambientsound (QUAKE)
VM_CL_precache_model,			// #75 string(string s) precache_model2 (QUAKE)
VM_precache_sound,				// #76 string(string s) precache_sound2 (QUAKE)
VM_precache_file,				// #77 string(string s) precache_file2 (QUAKE)
NULL,							// #78 void(entity e) setspawnparms (QUAKE)
NULL,							// #79 void(entity killer, entity killee) logfrag (QUAKEWORLD)
NULL,							// #80 string(entity e, string keyname) infokey (QUAKEWORLD)
VM_stof,						// #81 float(string s) stof (FRIK_FILE)
NULL,							// #82 void(vector where, float set) multicast (QUAKEWORLD)
NULL,							// #83 (QUAKE)
NULL,							// #84 (QUAKE)
NULL,							// #85 (QUAKE)
NULL,							// #86 (QUAKE)
NULL,							// #87 (QUAKE)
NULL,							// #88 (QUAKE)
NULL,							// #89 (QUAKE)
VM_CL_tracebox,					// #90 void(vector v1, vector min, vector max, vector v2, float nomonsters, entity forent) tracebox (DP_QC_TRACEBOX)
VM_randomvec,					// #91 vector() randomvec (DP_QC_RANDOMVEC)
VM_CL_getlight,					// #92 vector(vector org) getlight (DP_QC_GETLIGHT)
VM_registercvar,				// #93 float(string name, string value) registercvar (DP_REGISTERCVAR)
VM_min,							// #94 float(float a, floats) min (DP_QC_MINMAXBOUND)
VM_max,							// #95 float(float a, floats) max (DP_QC_MINMAXBOUND)
VM_bound,						// #96 float(float minimum, float val, float maximum) bound (DP_QC_MINMAXBOUND)
VM_pow,							// #97 float(float f, float f) pow (DP_QC_SINCOSSQRTPOW)
VM_findfloat,					// #98 entity(entity start, .float fld, float match) findfloat (DP_QC_FINDFLOAT)
VM_checkextension,				// #99 float(string s) checkextension (the basis of the extension system)
// FrikaC and Telejano range #100-#199
NULL,							// #100
NULL,							// #101
NULL,							// #102
NULL,							// #103
NULL,							// #104
NULL,							// #105
NULL,							// #106
NULL,							// #107
NULL,							// #108
NULL,							// #109
VM_fopen,						// #110 float(string filename, float mode) fopen (FRIK_FILE)
VM_fclose,						// #111 void(float fhandle) fclose (FRIK_FILE)
VM_fgets,						// #112 string(float fhandle) fgets (FRIK_FILE)
VM_fputs,						// #113 void(float fhandle, string s) fputs (FRIK_FILE)
VM_strlen,						// #114 float(string s) strlen (FRIK_FILE)
VM_strcat,						// #115 string(string s, string...) strcat (FRIK_FILE)
VM_substring,					// #116 string(string s, float start, float length) substring (FRIK_FILE)
VM_stov,						// #117 vector(string) stov (FRIK_FILE)
VM_strzone,						// #118 string(string s) strzone (FRIK_FILE)
VM_strunzone,					// #119 void(string s) strunzone (FRIK_FILE)
NULL,							// #120
NULL,							// #121
NULL,							// #122
NULL,							// #123
NULL,							// #124
NULL,							// #125
NULL,							// #126
NULL,							// #127
NULL,							// #128
NULL,							// #129
NULL,							// #130
NULL,							// #131
NULL,							// #132
NULL,							// #133
NULL,							// #134
NULL,							// #135
NULL,							// #136
NULL,							// #137
NULL,							// #138
NULL,							// #139
NULL,							// #140
NULL,							// #141
NULL,							// #142
NULL,							// #143
NULL,							// #144
NULL,							// #145
NULL,							// #146
NULL,							// #147
NULL,							// #148
NULL,							// #149
NULL,							// #150
NULL,							// #151
NULL,							// #152
NULL,							// #153
NULL,							// #154
NULL,							// #155
NULL,							// #156
NULL,							// #157
NULL,							// #158
NULL,							// #159
NULL,							// #160
NULL,							// #161
NULL,							// #162
NULL,							// #163
NULL,							// #164
NULL,							// #165
NULL,							// #166
NULL,							// #167
NULL,							// #168
NULL,							// #169
NULL,							// #170
NULL,							// #171
NULL,							// #172
NULL,							// #173
NULL,							// #174
NULL,							// #175
NULL,							// #176
VM_localsound,					// #177
NULL,							// #178
NULL,							// #179
NULL,							// #180
NULL,							// #181
NULL,							// #182
NULL,							// #183
NULL,							// #184
NULL,							// #185
NULL,							// #186
NULL,							// #187
NULL,							// #188
NULL,							// #189
NULL,							// #190
NULL,							// #191
NULL,							// #192
NULL,							// #193
NULL,							// #194
NULL,							// #195
NULL,							// #196
NULL,							// #197
NULL,							// #198
NULL,							// #199
// FTEQW range #200-#299
NULL,							// #200
NULL,							// #201
NULL,							// #202
NULL,							// #203
NULL,							// #204
NULL,							// #205
NULL,							// #206
NULL,							// #207
NULL,							// #208
NULL,							// #209
NULL,							// #210
NULL,							// #211
NULL,							// #212
NULL,							// #213
NULL,							// #214
NULL,							// #215
NULL,							// #216
NULL,							// #217
VM_bitshift,					// #218 float(float number, float quantity) bitshift (EXT_BITSHIFT)
NULL,							// #219
NULL,							// #220
VM_strstrofs,					// #221 float(string str, string sub[, float startpos]) strstrofs (FTE_STRINGS)
VM_str2chr,						// #222 float(string str, float ofs) str2chr (FTE_STRINGS)
VM_chr2str,						// #223 string(float c, ...) chr2str (FTE_STRINGS)
VM_strconv,						// #224 string(float ccase, float calpha, float cnum, string s, ...) strconv (FTE_STRINGS)
VM_strpad,						// #225 string(float chars, string s, ...) strpad (FTE_STRINGS)
VM_infoadd,						// #226 string(string info, string key, string value, ...) infoadd (FTE_STRINGS)
VM_infoget,						// #227 string(string info, string key) infoget (FTE_STRINGS)
VM_strncmp,						// #228 float(string s1, string s2, float len) strncmp (FTE_STRINGS)
VM_strncasecmp,					// #229 float(string s1, string s2) strcasecmp (FTE_STRINGS)
VM_strncasecmp,					// #230 float(string s1, string s2, float len) strncasecmp (FTE_STRINGS)
NULL,							// #231
NULL,							// #232 void(float index, float type, .void field) SV_AddStat (EXT_CSQC)
NULL,							// #233
NULL,							// #234
NULL,							// #235
NULL,							// #236
NULL,							// #237
NULL,							// #238
NULL,							// #239
VM_CL_checkpvs,					// #240
NULL,							// #241
NULL,							// #242
NULL,							// #243
NULL,							// #244
VM_modulo,						// #245
NULL,							// #246
NULL,							// #247
NULL,							// #248
NULL,							// #249
NULL,							// #250
NULL,							// #251
NULL,							// #252
NULL,							// #253
NULL,							// #254
NULL,							// #255
NULL,							// #256
NULL,							// #257
NULL,							// #258
NULL,							// #259
NULL,							// #260
NULL,							// #261
NULL,							// #262
VM_CL_skel_create,				// #263 float(float modlindex) skel_create = #263; // (FTE_CSQC_SKELETONOBJECTS) create a skeleton (be sure to assign this value into .skeletonindex for use), returns skeleton index (1 or higher) on success, returns 0 on failure  (for example if the modelindex is not skeletal), it is recommended that you create a new skeleton if you change modelindex.
VM_CL_skel_build,				// #264 float(float skel, entity ent, float modlindex, float retainfrac, float firstbone, float lastbone) skel_build = #264; // (FTE_CSQC_SKELETONOBJECTS) blend in a percentage of standard animation, 0 replaces entirely, 1 does nothing, 0.5 blends half, etc, and this only alters the bones in the specified range for which out of bounds values like 0,100000 are safe (uses .frame, .frame2, .frame3, .frame4, .lerpfrac, .lerpfrac3, .lerpfrac4, .frame1time, .frame2time, .frame3time, .frame4time), returns skel on success, 0 on failure
VM_CL_skel_get_numbones,		// #265 float(float skel) skel_get_numbones = #265; // (FTE_CSQC_SKELETONOBJECTS) returns how many bones exist in the created skeleton
VM_CL_skel_get_bonename,		// #266 string(float skel, float bonenum) skel_get_bonename = #266; // (FTE_CSQC_SKELETONOBJECTS) returns name of bone (as a tempstring)
VM_CL_skel_get_boneparent,		// #267 float(float skel, float bonenum) skel_get_boneparent = #267; // (FTE_CSQC_SKELETONOBJECTS) returns parent num for supplied bonenum, -1 if bonenum has no parent or bone does not exist (returned value is always less than bonenum, you can loop on this)
VM_CL_skel_find_bone,			// #268 float(float skel, string tagname) skel_find_bone = #268; // (FTE_CSQC_SKELETONOBJECTS) get number of bone with specified name, 0 on failure, tagindex (bonenum+1) on success, same as using gettagindex on the modelindex
VM_CL_skel_get_bonerel,			// #269 vector(float skel, float bonenum) skel_get_bonerel = #269; // (FTE_CSQC_SKELETONOBJECTS) get matrix of bone in skeleton relative to its parent - sets v_forward, v_right, v_up, returns origin (relative to parent bone)
VM_CL_skel_get_boneabs,			// #270 vector(float skel, float bonenum) skel_get_boneabs = #270; // (FTE_CSQC_SKELETONOBJECTS) get matrix of bone in skeleton in model space - sets v_forward, v_right, v_up, returns origin (relative to entity)
VM_CL_skel_set_bone,			// #271 void(float skel, float bonenum, vector org) skel_set_bone = #271; // (FTE_CSQC_SKELETONOBJECTS) set matrix of bone relative to its parent, reads v_forward, v_right, v_up, takes origin as parameter (relative to parent bone)
VM_CL_skel_mul_bone,			// #272 void(float skel, float bonenum, vector org) skel_mul_bone = #272; // (FTE_CSQC_SKELETONOBJECTS) transform bone matrix (relative to its parent) by the supplied matrix in v_forward, v_right, v_up, takes origin as parameter (relative to parent bone)
VM_CL_skel_mul_bones,			// #273 void(float skel, float startbone, float endbone, vector org) skel_mul_bones = #273; // (FTE_CSQC_SKELETONOBJECTS) transform bone matrices (relative to their parents) by the supplied matrix in v_forward, v_right, v_up, takes origin as parameter (relative to parent bones)
VM_CL_skel_copybones,			// #274 void(float skeldst, float skelsrc, float startbone, float endbone) skel_copybones = #274; // (FTE_CSQC_SKELETONOBJECTS) copy bone matrices (relative to their parents) from one skeleton to another, useful for copying a skeleton to a corpse
VM_CL_skel_delete,				// #275 void(float skel) skel_delete = #275; // (FTE_CSQC_SKELETONOBJECTS) deletes skeleton at the beginning of the next frame (you can add the entity, delete the skeleton, renderscene, and it will still work)
VM_CL_frameforname,				// #276 float(float modlindex, string framename) frameforname = #276; // (FTE_CSQC_SKELETONOBJECTS) finds number of a specified frame in the animation, returns -1 if no match found
VM_CL_frameduration,			// #277 float(float modlindex, float framenum) frameduration = #277; // (FTE_CSQC_SKELETONOBJECTS) returns the intended play time (in seconds) of the specified framegroup, if it does not exist the result is 0, if it is a single frame it may be a small value around 0.1 or 0.
NULL,							// #278
NULL,							// #279
NULL,							// #280
NULL,							// #281
NULL,							// #282
NULL,							// #283
NULL,							// #284
NULL,							// #285
NULL,							// #286
NULL,							// #287
NULL,							// #288
NULL,							// #289
NULL,							// #290
NULL,							// #291
NULL,							// #292
NULL,							// #293
NULL,							// #294
NULL,							// #295
NULL,							// #296
NULL,							// #297
NULL,							// #298
NULL,							// #299
// CSQC range #300-#399
VM_CL_R_ClearScene,				// #300 void() clearscene (EXT_CSQC)
VM_CL_R_AddEntities,			// #301 void(float mask) addentities (EXT_CSQC)
VM_CL_R_AddEntity,				// #302 void(entity ent) addentity (EXT_CSQC)
VM_CL_R_SetView,				// #303 float(float property, ...) setproperty (EXT_CSQC)
VM_CL_R_RenderScene,			// #304 void() renderscene (EXT_CSQC)
VM_CL_R_AddDynamicLight,		// #305 void(vector org, float radius, vector lightcolours) adddynamiclight (EXT_CSQC)
VM_CL_R_PolygonBegin,			// #306 void(string texturename, float flag, float is2d[NYI: , float lines]) R_BeginPolygon
VM_CL_R_PolygonVertex,			// #307 void(vector org, vector texcoords, vector rgb, float alpha) R_PolygonVertex
VM_CL_R_PolygonEnd,				// #308 void() R_EndPolygon
VM_CL_R_SetView,				// #309 float(float property) getproperty (EXT_CSQC)
VM_CL_unproject,				// #310 vector (vector v) cs_unproject (EXT_CSQC)
VM_CL_project,					// #311 vector (vector v) cs_project (EXT_CSQC)
NULL,							// #312
NULL,							// #313
NULL,							// #314
VM_drawline,					// #315 void(float width, vector pos1, vector pos2, float flag) drawline (EXT_CSQC)
VM_iscachedpic,					// #316 float(string name) iscachedpic (EXT_CSQC)
VM_precache_pic,				// #317 string(string name, float trywad) precache_pic (EXT_CSQC)
VM_getimagesize,				// #318 vector(string picname) draw_getimagesize (EXT_CSQC)
VM_freepic,						// #319 void(string name) freepic (EXT_CSQC)
VM_drawcharacter,				// #320 float(vector position, float character, vector scale, vector rgb, float alpha, float flag) drawcharacter (EXT_CSQC)
VM_drawstring,					// #321 float(vector position, string text, vector scale, vector rgb, float alpha[, float flag]) drawstring (EXT_CSQC, DP_CSQC)
VM_drawpic,						// #322 float(vector position, string pic, vector size, vector rgb, float alpha[, float flag]) drawpic (EXT_CSQC)
VM_drawfill,					// #323 float(vector position, vector size, vector rgb, float alpha, float flag) drawfill (EXT_CSQC)
VM_drawsetcliparea,				// #324 void(float x, float y, float width, float height) drawsetcliparea
VM_drawresetcliparea,			// #325 void(void) drawresetcliparea
VM_drawcolorcodedstring,		// #326 float drawcolorcodedstring(vector position, string text, vector scale, vector rgb, float alpha, float flag) (EXT_CSQC)
VM_stringwidth,					// #327 // FIXME is this okay?
VM_drawsubpic,					// #328 // FIXME is this okay?
VM_drawrotpic,					// #329 // FIXME is this okay?
VM_CL_getstatf,					// #330 float(float stnum) getstatf (EXT_CSQC)
VM_CL_getstati,					// #331 float(float stnum) getstati (EXT_CSQC)
VM_CL_getstats,					// #332 string(float firststnum) getstats (EXT_CSQC)
VM_CL_setmodelindex,			// #333 void(entity e, float mdlindex) setmodelindex (EXT_CSQC)
VM_CL_modelnameforindex,		// #334 string(float mdlindex) modelnameforindex (EXT_CSQC)
VM_CL_particleeffectnum,		// #335 float(string effectname) particleeffectnum (EXT_CSQC)
VM_CL_trailparticles,			// #336 void(entity ent, float effectnum, vector start, vector end) trailparticles (EXT_CSQC)
VM_CL_pointparticles,			// #337 void(float effectnum, vector origin [, vector dir, float count]) pointparticles (EXT_CSQC)
VM_centerprint,					// #338 void(string s, ...) centerprint (EXT_CSQC)
VM_print,						// #339 void(string s, ...) print (EXT_CSQC, DP_SV_PRINT)
VM_keynumtostring,				// #340 string(float keynum) keynumtostring (EXT_CSQC)
VM_stringtokeynum,				// #341 float(string keyname) stringtokeynum (EXT_CSQC)
VM_getkeybind,					// #342 string(float keynum[, float bindmap]) getkeybind (EXT_CSQC)
VM_CL_setcursormode,			// #343 void(float usecursor) setcursormode (DP_CSQC)
VM_CL_getmousepos,				// #344 vector() getmousepos (DP_CSQC)
VM_CL_getinputstate,			// #345 float(float framenum) getinputstate (EXT_CSQC)
VM_CL_setsensitivityscale,		// #346 void(float sens) setsensitivityscale (EXT_CSQC)
VM_CL_runplayerphysics,			// #347 void() runstandardplayerphysics (EXT_CSQC)
VM_CL_getplayerkey,				// #348 string(float playernum, string keyname) getplayerkeyvalue (EXT_CSQC)
VM_CL_isdemo,					// #349 float() isdemo (EXT_CSQC)
VM_isserver,					// #350 float() isserver (EXT_CSQC)
VM_CL_setlistener,				// #351 void(vector origin, vector forward, vector right, vector up) SetListener (EXT_CSQC)
VM_CL_registercmd,				// #352 void(string cmdname) registercommand (EXT_CSQC)
VM_wasfreed,					// #353 float(entity ent) wasfreed (EXT_CSQC) (should be availabe on server too)
VM_CL_serverkey,				// #354 string(string key) serverkey (EXT_CSQC)
VM_CL_videoplaying,				// #355
VM_findfont,					// #356 float(string fontname) loadfont (DP_GFX_FONTS)
VM_loadfont,					// #357 float(string fontname, string fontmaps, string sizes, float slot) loadfont (DP_GFX_FONTS)
VM_CL_loadcubemap,				// #358 void(string cubemapname) loadcubemap (DP_GFX_)
NULL,							// #359
VM_CL_ReadByte,					// #360 float() ReadByte (EXT_CSQC)
VM_CL_ReadChar,					// #361 float() ReadChar (EXT_CSQC)
VM_CL_ReadShort,				// #362 float() ReadShort (EXT_CSQC)
VM_CL_ReadLong,					// #363 float() ReadLong (EXT_CSQC)
VM_CL_ReadCoord,				// #364 float() ReadCoord (EXT_CSQC)
VM_CL_ReadAngle,				// #365 float() ReadAngle (EXT_CSQC)
VM_CL_ReadString,				// #366 string() ReadString (EXT_CSQC)
VM_CL_ReadFloat,				// #367 float() ReadFloat (EXT_CSQC)
NULL,						// #368
NULL,							// #369
NULL,							// #370
NULL,							// #371
NULL,							// #372
NULL,							// #373
NULL,							// #374
NULL,							// #375
NULL,							// #376
NULL,							// #377
NULL,							// #378
NULL,							// #379
NULL,							// #380
NULL,							// #381
NULL,							// #382
NULL,							// #383
NULL,							// #384
NULL,							// #385
NULL,							// #386
NULL,							// #387
NULL,							// #388
NULL,							// #389
NULL,							// #390
NULL,							// #391
NULL,							// #392
NULL,							// #393
NULL,							// #394
NULL,							// #395
NULL,							// #396
NULL,							// #397
NULL,							// #398
NULL,							// #399
// LadyHavoc's range #400-#499
VM_CL_copyentity,				// #400 void(entity from, entity to) copyentity (DP_QC_COPYENTITY)
NULL,							// #401 void(entity ent, float colors) setcolor (DP_QC_SETCOLOR)
VM_findchain,					// #402 entity(.string fld, string match) findchain (DP_QC_FINDCHAIN)
VM_findchainfloat,				// #403 entity(.float fld, float match) findchainfloat (DP_QC_FINDCHAINFLOAT)
VM_CL_effect,					// #404 void(vector org, string modelname, float startframe, float endframe, float framerate) effect (DP_SV_EFFECT)
VM_CL_te_blood,					// #405 void(vector org, vector velocity, float howmany) te_blood (DP_TE_BLOOD)
VM_CL_te_bloodshower,			// #406 void(vector mincorner, vector maxcorner, float explosionspeed, float howmany) te_bloodshower (DP_TE_BLOODSHOWER)
VM_CL_te_explosionrgb,			// #407 void(vector org, vector color) te_explosionrgb (DP_TE_EXPLOSIONRGB)
VM_CL_te_particlecube,			// #408 void(vector mincorner, vector maxcorner, vector vel, float howmany, float color, float gravityflag, float randomveljitter) te_particlecube (DP_TE_PARTICLECUBE)
VM_CL_te_particlerain,			// #409 void(vector mincorner, vector maxcorner, vector vel, float howmany, float color) te_particlerain (DP_TE_PARTICLERAIN)
VM_CL_te_particlesnow,			// #410 void(vector mincorner, vector maxcorner, vector vel, float howmany, float color) te_particlesnow (DP_TE_PARTICLESNOW)
VM_CL_te_spark,					// #411 void(vector org, vector vel, float howmany) te_spark (DP_TE_SPARK)
VM_CL_te_gunshotquad,			// #412 void(vector org) te_gunshotquad (DP_QUADEFFECTS1)
VM_CL_te_spikequad,				// #413 void(vector org) te_spikequad (DP_QUADEFFECTS1)
VM_CL_te_superspikequad,		// #414 void(vector org) te_superspikequad (DP_QUADEFFECTS1)
VM_CL_te_explosionquad,			// #415 void(vector org) te_explosionquad (DP_QUADEFFECTS1)
VM_CL_te_smallflash,			// #416 void(vector org) te_smallflash (DP_TE_SMALLFLASH)
VM_CL_te_customflash,			// #417 void(vector org, float radius, float lifetime, vector color) te_customflash (DP_TE_CUSTOMFLASH)
VM_CL_te_gunshot,				// #418 void(vector org) te_gunshot (DP_TE_STANDARDEFFECTBUILTINS)
VM_CL_te_spike,					// #419 void(vector org) te_spike (DP_TE_STANDARDEFFECTBUILTINS)
VM_CL_te_superspike,			// #420 void(vector org) te_superspike (DP_TE_STANDARDEFFECTBUILTINS)
VM_CL_te_explosion,				// #421 void(vector org) te_explosion (DP_TE_STANDARDEFFECTBUILTINS)
VM_CL_te_tarexplosion,			// #422 void(vector org) te_tarexplosion (DP_TE_STANDARDEFFECTBUILTINS)
VM_CL_te_wizspike,				// #423 void(vector org) te_wizspike (DP_TE_STANDARDEFFECTBUILTINS)
VM_CL_te_knightspike,			// #424 void(vector org) te_knightspike (DP_TE_STANDARDEFFECTBUILTINS)
VM_CL_te_lavasplash,			// #425 void(vector org) te_lavasplash (DP_TE_STANDARDEFFECTBUILTINS)
VM_CL_te_teleport,				// #426 void(vector org) te_teleport (DP_TE_STANDARDEFFECTBUILTINS)
VM_CL_te_explosion2,			// #427 void(vector org, float colorstart, float colorlength) te_explosion2 (DP_TE_STANDARDEFFECTBUILTINS)
VM_CL_te_lightning1,			// #428 void(entity own, vector start, vector end) te_lightning1 (DP_TE_STANDARDEFFECTBUILTINS)
VM_CL_te_lightning2,			// #429 void(entity own, vector start, vector end) te_lightning2 (DP_TE_STANDARDEFFECTBUILTINS)
VM_CL_te_lightning3,			// #430 void(entity own, vector start, vector end) te_lightning3 (DP_TE_STANDARDEFFECTBUILTINS)
VM_CL_te_beam,					// #431 void(entity own, vector start, vector end) te_beam (DP_TE_STANDARDEFFECTBUILTINS)
VM_vectorvectors,				// #432 void(vector dir) vectorvectors (DP_QC_VECTORVECTORS)
VM_CL_te_plasmaburn,			// #433 void(vector org) te_plasmaburn (DP_TE_PLASMABURN)
VM_getsurfacenumpoints,		// #434 float(entity e, float s) getsurfacenumpoints (DP_QC_GETSURFACE)
VM_getsurfacepoint,			// #435 vector(entity e, float s, float n) getsurfacepoint (DP_QC_GETSURFACE)
VM_getsurfacenormal,			// #436 vector(entity e, float s) getsurfacenormal (DP_QC_GETSURFACE)
VM_getsurfacetexture,		// #437 string(entity e, float s) getsurfacetexture (DP_QC_GETSURFACE)
VM_getsurfacenearpoint,		// #438 float(entity e, vector p) getsurfacenearpoint (DP_QC_GETSURFACE)
VM_getsurfaceclippedpoint,	// #439 vector(entity e, float s, vector p) getsurfaceclippedpoint (DP_QC_GETSURFACE)
NULL,							// #440 void(entity e, string s) clientcommand (KRIMZON_SV_PARSECLIENTCOMMAND)
VM_tokenize,					// #441 float(string s) tokenize (KRIMZON_SV_PARSECLIENTCOMMAND)
VM_argv,						// #442 string(float n) argv (KRIMZON_SV_PARSECLIENTCOMMAND)
VM_CL_setattachment,			// #443 void(entity e, entity tagentity, string tagname) setattachment (DP_GFX_QUAKE3MODELTAGS)
VM_search_begin,				// #444 float(string pattern, float caseinsensitive, float quiet) search_begin (DP_QC_FS_SEARCH)
VM_search_end,					// #445 void(float handle) search_end (DP_QC_FS_SEARCH)
VM_search_getsize,				// #446 float(float handle) search_getsize (DP_QC_FS_SEARCH)
VM_search_getfilename,			// #447 string(float handle, float num) search_getfilename (DP_QC_FS_SEARCH)
VM_cvar_string,					// #448 string(string s) cvar_string (DP_QC_CVAR_STRING)
VM_findflags,					// #449 entity(entity start, .float fld, float match) findflags (DP_QC_FINDFLAGS)
VM_findchainflags,				// #450 entity(.float fld, float match) findchainflags (DP_QC_FINDCHAINFLAGS)
VM_CL_gettagindex,				// #451 float(entity ent, string tagname) gettagindex (DP_QC_GETTAGINFO)
VM_CL_gettaginfo,				// #452 vector(entity ent, float tagindex) gettaginfo (DP_QC_GETTAGINFO)
NULL,							// #453 void(entity clent) dropclient (DP_SV_DROPCLIENT)
NULL,							// #454 entity() spawnclient (DP_SV_BOTCLIENT)
NULL,							// #455 float(entity clent) clienttype (DP_SV_BOTCLIENT)
NULL,							// #456 void(float to, string s) WriteUnterminatedString (DP_SV_WRITEUNTERMINATEDSTRING)
VM_CL_te_flamejet,				// #457 void(vector org, vector vel, float howmany) te_flamejet (DP_TE_FLAMEJET)
NULL,							// #458
VM_ftoe,						// #459 entity(float num) entitybyindex (DP_QC_EDICT_NUM)
VM_buf_create,					// #460 float() buf_create (DP_QC_STRINGBUFFERS)
VM_buf_del,						// #461 void(float bufhandle) buf_del (DP_QC_STRINGBUFFERS)
VM_buf_getsize,					// #462 float(float bufhandle) buf_getsize (DP_QC_STRINGBUFFERS)
VM_buf_copy,					// #463 void(float bufhandle_from, float bufhandle_to) buf_copy (DP_QC_STRINGBUFFERS)
VM_buf_sort,					// #464 void(float bufhandle, float sortpower, float backward) buf_sort (DP_QC_STRINGBUFFERS)
VM_buf_implode,					// #465 string(float bufhandle, string glue) buf_implode (DP_QC_STRINGBUFFERS)
VM_bufstr_get,					// #466 string(float bufhandle, float string_index) bufstr_get (DP_QC_STRINGBUFFERS)
VM_bufstr_set,					// #467 void(float bufhandle, float string_index, string str) bufstr_set (DP_QC_STRINGBUFFERS)
VM_bufstr_add,					// #468 float(float bufhandle, string str, float order) bufstr_add (DP_QC_STRINGBUFFERS)
VM_bufstr_free,					// #469 void(float bufhandle, float string_index) bufstr_free (DP_QC_STRINGBUFFERS)
NULL,							// #470 void(float index, float type, .void field) SV_AddStat (EXT_CSQC)
VM_asin,						// #471 float(float s) VM_asin (DP_QC_ASINACOSATANATAN2TAN)
VM_acos,						// #472 float(float c) VM_acos (DP_QC_ASINACOSATANATAN2TAN)
VM_atan,						// #473 float(float t) VM_atan (DP_QC_ASINACOSATANATAN2TAN)
VM_atan2,						// #474 float(float c, float s) VM_atan2 (DP_QC_ASINACOSATANATAN2TAN)
VM_tan,							// #475 float(float a) VM_tan (DP_QC_ASINACOSATANATAN2TAN)
VM_strlennocol,					// #476 float(string s) : DRESK - String Length (not counting color codes) (DP_QC_STRINGCOLORFUNCTIONS)
VM_strdecolorize,				// #477 string(string s) : DRESK - Decolorized String (DP_QC_STRINGCOLORFUNCTIONS)
VM_strftime,					// #478 string(float uselocaltime, string format, ...) (DP_QC_STRFTIME)
VM_tokenizebyseparator,			// #479 float(string s) tokenizebyseparator (DP_QC_TOKENIZEBYSEPARATOR)
VM_strtolower,					// #480 string(string s) VM_strtolower (DP_QC_STRING_CASE_FUNCTIONS)
VM_strtoupper,					// #481 string(string s) VM_strtoupper (DP_QC_STRING_CASE_FUNCTIONS)
VM_cvar_defstring,				// #482 string(string s) cvar_defstring (DP_QC_CVAR_DEFSTRING)
VM_CL_pointsound,				// #483 void(vector origin, string sample, float volume, float attenuation) pointsound (DP_SV_POINTSOUND)
VM_strreplace,					// #484 string(string search, string replace, string subject) strreplace (DP_QC_STRREPLACE)
VM_strireplace,					// #485 string(string search, string replace, string subject) strireplace (DP_QC_STRREPLACE)
VM_getsurfacepointattribute,// #486 vector(entity e, float s, float n, float a) getsurfacepointattribute
VM_gecko_create,					// #487 float gecko_create( string name )
VM_gecko_destroy,					// #488 void gecko_destroy( string name )
VM_gecko_navigate,				// #489 void gecko_navigate( string name, string URI )
VM_gecko_keyevent,				// #490 float gecko_keyevent( string name, float key, float eventtype )
VM_gecko_movemouse,				// #491 void gecko_mousemove( string name, float x, float y )
VM_gecko_resize,					// #492 void gecko_resize( string name, float w, float h )
VM_gecko_get_texture_extent,	// #493 vector gecko_get_texture_extent( string name )
VM_crc16,						// #494 float(float caseinsensitive, string s, ...) crc16 = #494 (DP_QC_CRC16)
VM_cvar_type,					// #495 float(string name) cvar_type = #495; (DP_QC_CVAR_TYPE)
VM_numentityfields,				// #496 float() numentityfields = #496; (QP_QC_ENTITYDATA)
VM_entityfieldname,				// #497 string(float fieldnum) entityfieldname = #497; (DP_QC_ENTITYDATA)
VM_entityfieldtype,				// #498 float(float fieldnum) entityfieldtype = #498; (DP_QC_ENTITYDATA)
VM_getentityfieldstring,		// #499 string(float fieldnum, entity ent) getentityfieldstring = #499; (DP_QC_ENTITYDATA)
VM_putentityfieldstring,		// #500 float(float fieldnum, entity ent, string s) putentityfieldstring = #500; (DP_QC_ENTITYDATA)
VM_CL_ReadPicture,				// #501 string() ReadPicture = #501;
VM_CL_boxparticles,				// #502 void(float effectnum, entity own, vector origin_from, vector origin_to, vector dir_from, vector dir_to, float count) boxparticles (DP_CSQC_BOXPARTICLES)
VM_whichpack,					// #503 string(string) whichpack = #503;
VM_CL_GetEntity,				// #504 float(float entitynum, float fldnum) getentity = #504; vector(float entitynum, float fldnum) getentityvec = #504;
NULL,							// #505
NULL,							// #506
NULL,							// #507
NULL,							// #508
NULL,							// #509
VM_uri_escape,					// #510 string(string in) uri_escape = #510;
VM_uri_unescape,				// #511 string(string in) uri_unescape = #511;
VM_etof,					// #512 float(entity ent) num_for_edict = #512 (DP_QC_NUM_FOR_EDICT)
VM_uri_get,						// #513 float(string uri, float id, [string post_contenttype, string post_delim, [float buf]]) uri_get = #513; (DP_QC_URI_GET, DP_QC_URI_POST)
VM_tokenize_console,					// #514 float(string str) tokenize_console = #514; (DP_QC_TOKENIZE_CONSOLE)
VM_argv_start_index,					// #515 float(float idx) argv_start_index = #515; (DP_QC_TOKENIZE_CONSOLE)
VM_argv_end_index,						// #516 float(float idx) argv_end_index = #516; (DP_QC_TOKENIZE_CONSOLE)
VM_buf_cvarlist,						// #517 void(float buf, string prefix, string antiprefix) buf_cvarlist = #517; (DP_QC_STRINGBUFFERS_CVARLIST)
VM_cvar_description,					// #518 float(string name) cvar_description = #518; (DP_QC_CVAR_DESCRIPTION)
VM_gettime,						// #519 float(float timer) gettime = #519; (DP_QC_GETTIME)
VM_keynumtostring,				// #520 string keynumtostring(float keynum)
VM_findkeysforcommand,			// #521 string findkeysforcommand(string command[, float bindmap])
VM_CL_InitParticleSpawner,		// #522 void(float max_themes) initparticlespawner (DP_CSQC_SPAWNPARTICLE)
VM_CL_ResetParticle,			// #523 void() resetparticle (DP_CSQC_SPAWNPARTICLE)
VM_CL_ParticleTheme,			// #524 void(float theme) particletheme (DP_CSQC_SPAWNPARTICLE)
VM_CL_ParticleThemeSave,		// #525 void() particlethemesave, void(float theme) particlethemeupdate (DP_CSQC_SPAWNPARTICLE)
VM_CL_ParticleThemeFree,		// #526 void() particlethemefree (DP_CSQC_SPAWNPARTICLE)
VM_CL_SpawnParticle,			// #527 float(vector org, vector vel, [float theme]) particle (DP_CSQC_SPAWNPARTICLE)
VM_CL_SpawnParticleDelayed,		// #528 float(vector org, vector vel, float delay, float collisiondelay, [float theme]) delayedparticle (DP_CSQC_SPAWNPARTICLE)
VM_loadfromdata,				// #529
VM_loadfromfile,				// #530
VM_CL_setpause,					// #531 float(float ispaused) setpause = #531 (DP_CSQC_SETPAUSE)
VM_log,							// #532
VM_getsoundtime,				// #533 float(entity e, float channel) getsoundtime = #533; (DP_SND_GETSOUNDTIME)
VM_soundlength,					// #534 float(string sample) soundlength = #534; (DP_SND_GETSOUNDTIME)
VM_buf_loadfile,                // #535 float(string filename, float bufhandle) buf_loadfile (DP_QC_STRINGBUFFERS_EXT_WIP)
VM_buf_writefile,               // #536 float(float filehandle, float bufhandle, float startpos, float numstrings) buf_writefile (DP_QC_STRINGBUFFERS_EXT_WIP)
VM_bufstr_find,                 // #537 float(float bufhandle, string match, float matchrule, float startpos) bufstr_find (DP_QC_STRINGBUFFERS_EXT_WIP)
VM_matchpattern,                // #538 float(string s, string pattern, float matchrule) matchpattern (DP_QC_STRINGBUFFERS_EXT_WIP)
NULL,							// #539
VM_physics_enable,				// #540 void(entity e, float physics_enabled) physics_enable = #540; (DP_PHYSICS_ODE)
VM_physics_addforce,			// #541 void(entity e, vector force, vector relative_ofs) physics_addforce = #541; (DP_PHYSICS_ODE)
VM_physics_addtorque,			// #542 void(entity e, vector torque) physics_addtorque = #542; (DP_PHYSICS_ODE)
NULL,							// #543
NULL,							// #544
NULL,							// #545
NULL,							// #546
NULL,							// #547
NULL,							// #548
NULL,							// #549
NULL,							// #550
NULL,							// #551
NULL,							// #552
NULL,							// #553
NULL,							// #554
NULL,							// #555
NULL,							// #556
NULL,							// #557
NULL,							// #558
NULL,							// #559
NULL,							// #560
NULL,							// #561
NULL,							// #562
NULL,							// #563
NULL,							// #564
NULL,							// #565
VM_CL_findbox,					// #566 entity(vector mins, vector maxs) findbox = #566; (DP_QC_FINDBOX)
VM_nudgeoutofsolid,				// #567 float(entity ent) nudgeoutofsolid = #567; (DP_QC_NUDGEOUTOFSOLID)
NULL,							// #568
NULL,							// #569
NULL,							// #570
NULL,							// #571
NULL,							// #572
NULL,							// #573
NULL,							// #574
NULL,							// #575
NULL,							// #576
NULL,							// #577
NULL,							// #578
NULL,							// #579
NULL,							// #580
NULL,							// #581
NULL,							// #582
NULL,							// #583
NULL,							// #584
NULL,							// #585
NULL,							// #586
NULL,							// #587
NULL,							// #588
NULL,							// #589
NULL,							// #590
NULL,							// #591
NULL,							// #592
NULL,							// #593
NULL,							// #594
NULL,							// #595
NULL,							// #596
NULL,							// #597
NULL,							// #598
NULL,							// #599
NULL,							// #600
NULL,							// #601
NULL,							// #602
NULL,							// #603
NULL,							// #604
VM_callfunction,				// #605
VM_writetofile,					// #606
VM_isfunction,					// #607
NULL,							// #608
NULL,							// #609
VM_findkeysforcommand,			// #610 string findkeysforcommand(string command[, float bindmap])
NULL,							// #611
NULL,							// #612
VM_parseentitydata,				// #613
NULL,							// #614
NULL,							// #615
NULL,							// #616
NULL,							// #617
NULL,							// #618
NULL,							// #619
NULL,							// #620
NULL,							// #621
NULL,							// #622
NULL,							// #623
VM_CL_getextresponse,			// #624 string getextresponse(void)
NULL,							// #625
NULL,							// #626
VM_sprintf,                     // #627 string sprintf(string format, ...)
VM_getsurfacenumtriangles,		// #628 float(entity e, float s) getsurfacenumpoints (DP_QC_GETSURFACETRIANGLE)
VM_getsurfacetriangle,			// #629 vector(entity e, float s, float n) getsurfacepoint (DP_QC_GETSURFACETRIANGLE)
VM_setkeybind,						// #630 float(float key, string bind[, float bindmap]) setkeybind
VM_getbindmaps,						// #631 vector(void) getbindmap
VM_setbindmaps,						// #632 float(vector bm) setbindmap
NULL,							// #633
NULL,							// #634
NULL,							// #635
NULL,							// #636
NULL,							// #637
VM_CL_RotateMoves,					// #638
VM_digest_hex,						// #639
VM_CL_V_CalcRefdef,					// #640 void(entity e) V_CalcRefdef (DP_CSQC_V_CALCREFDEF)
NULL,							// #641
VM_coverage,						// #642
NULL
};

const int vm_cl_numbuiltins = sizeof(vm_cl_builtins) / sizeof(prvm_builtin_t);

void CLVM_init_cmd(prvm_prog_t *prog)
{
	VM_Cmd_Init(prog);
	prog->polygonbegin_model = NULL;
	prog->polygonbegin_guess2d = 0;
}

void CLVM_reset_cmd(prvm_prog_t *prog)
{
	World_End(&cl.world);
	VM_Cmd_Reset(prog);
	prog->polygonbegin_model = NULL;
	prog->polygonbegin_guess2d = 0;
}
