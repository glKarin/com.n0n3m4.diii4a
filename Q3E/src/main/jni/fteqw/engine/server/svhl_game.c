#include "quakedef.h"

/*
I think globals.maxentities is the hard cap, rather than current max like in q1.
*/

#ifdef HLSERVER

#include "winquake.h"
#include "svhl_gcapi.h"
#include "pr_common.h"

#include "crc.h"
#include "model_hl.h"

#if defined(_MSC_VER)
 #if _MSC_VER >= 1300
  #define __func__ __FUNCTION__
 #else
  #define __func__ "unknown"
 #endif
#else
 //I hope you're c99 and have a __func__
#endif

//extern cvar_t temp1;
#define ignore(s) Con_DPrintf("Fixme: " s "\n")
#define notimpl(l) Con_Printf("halflife sv builtin not implemented on line %i\n", l)
#define notimpf(f) Con_Printf("halflife sv builtin %s not implemented\n", f)
#define bi_begin() //if (temp1.ival)Con_Printf("enter %s\n", __func__)
#define bi_end() //if (temp1.ival)Con_Printf("leave %s\n", __func__)
#define bi_trace() bi_begin(); bi_end()


dllhandle_t *hlgamecode;
SVHL_Globals_t SVHL_Globals;
SVHL_GameFuncs_t SVHL_GameFuncs;

static zonegroup_t hlmapmemgroup;	//flushed at end-of-map.

#define MAX_HL_EDICTS 2048
hledict_t *SVHL_Edict;
int SVHL_NumActiveEnts;

int lastusermessage;




string_t QDECL GHL_AllocString(const char *string)
{
	char *news;
	bi_begin();
	if (!string)
		return 0;
	news = ZG_Malloc(&hlmapmemgroup, strlen(string)+1);
	memcpy(news, string, strlen(string)+1);
	bi_end();
	return news - SVHL_Globals.stringbase;
}
int QDECL GHL_PrecacheModel(const char *name)
{
	int		i;
	bi_trace();

	if (name[0] <= ' ')
	{
		Con_Printf ("precache_model: empty string\n");
		return 0;
	}

	for (i=1 ; i<MAX_PRECACHE_MODELS ; i++)
	{
		if (!sv.strings.model_precache[i])
		{
			if (strlen(name)>=MAX_QPATH-1)	//probably safest to keep this.
			{
				SV_Error ("Precache name too long");
				return 0;
			}
			name = sv.strings.model_precache[i] = SVHL_Globals.stringbase+GHL_AllocString(name);

			if (!strcmp(name + strlen(name) - 4, ".bsp"))
				sv.models[i] = Mod_FindName(name);

			if (sv.state != ss_loading)
			{
				Con_DPrintf("Delayed model precache: %s\n", name);
				MSG_WriteByte(&sv.reliable_datagram, svcfte_precache);
				MSG_WriteShort(&sv.reliable_datagram, i);
				MSG_WriteString(&sv.reliable_datagram, name);
#ifdef NQPROT
				MSG_WriteByte(&sv.nqreliable_datagram, svcdp_precache);
				MSG_WriteShort(&sv.nqreliable_datagram, i);
				MSG_WriteString(&sv.nqreliable_datagram, name);
#endif
			}

			return i;
		}
		if (!strcmp(sv.strings.model_precache[i], name))
		{
			return i;
		}
	}
	SV_Error ("GHL_precache_model: overflow");
	return 0;
}
int QDECL GHL_PrecacheSound(char *name)
{
	int		i;
	bi_trace();

	if (name[0] <= ' ')
	{
		Con_Printf ("precache_sound: empty string\n");
		return 0;
	}

	for (i=1 ; i<MAX_PRECACHE_SOUNDS ; i++)
	{
		if (!*sv.strings.sound_precache[i])
		{
			if (strlen(name)>=MAX_QPATH-1)	//probably safest to keep this.
			{
				SV_Error ("Precache name too long");
				return 0;
			}
			strcpy(sv.strings.sound_precache[i], name);
			name = sv.strings.sound_precache[i];

			if (sv.state != ss_loading)
			{
				Con_DPrintf("Delayed sound precache: %s\n", name);
				MSG_WriteByte(&sv.reliable_datagram, svcfte_precache);
				MSG_WriteShort(&sv.reliable_datagram, -i);
				MSG_WriteString(&sv.reliable_datagram, name);
#ifdef NQPROT
				MSG_WriteByte(&sv.nqreliable_datagram, svcdp_precache);
				MSG_WriteShort(&sv.nqreliable_datagram, -i);
				MSG_WriteString(&sv.nqreliable_datagram, name);
#endif
			}

			return i;
		}
		if (!strcmp(sv.strings.sound_precache[i], name))
		{
			return i;
		}
	}
	SV_Error ("GHL_precache_sound: overflow");
	return 0;
}
void QDECL GHL_SetModel(hledict_t *ed, char *modelname)
{
	model_t *mod;
	int mdlidx = GHL_PrecacheModel(modelname);
	bi_trace();
	ed->v.modelindex = mdlidx;
	ed->v.model = sv.strings.model_precache[mdlidx] - SVHL_Globals.stringbase;

	mod = sv.models[mdlidx];
	if (mod)
	{
		VectorCopy(mod->mins, ed->v.mins);
		VectorCopy(mod->maxs, ed->v.maxs);
		VectorSubtract(mod->maxs, mod->mins, ed->v.size);
	}
	SVHL_LinkEdict(ed, false);
}
unk QDECL GHL_ModelIndex(unk){notimpf(__func__);}
int QDECL GHL_ModelFrames(int midx)
{
	bi_trace();
	//returns the number of frames(sequences I assume) this model has
	return Mod_GetFrameCount(sv.models[mdlidx], surfaceidx);
}
void QDECL GHL_SetSize(hledict_t *ed, float *mins, float *maxs)
{
	bi_trace();
	VectorCopy(mins, ed->v.mins);
	VectorCopy(maxs, ed->v.maxs);
	SVHL_LinkEdict(ed, false);
}
void QDECL GHL_ChangeLevel(char *nextmap, char *startspot)
{
	bi_trace();
	Cbuf_AddText(va("changelevel %s %s@%f@%f@%f\n", nextmap, startspot, SVHL_Globals.landmark[0], SVHL_Globals.landmark[1], SVHL_Globals.landmark[2]), RESTRICT_PROGS);
}
unk QDECL GHL_GetSpawnParms(unk){notimpf(__func__);}
unk QDECL GHL_SaveSpawnParms(unk){notimpf(__func__);}
float QDECL GHL_VecToYaw(float *inv)
{
	vec3_t outa;
	bi_trace();

	VectorAngles(inv, NULL, outa);
	return outa[1];
}
void QDECL GHL_VecToAngles(float *inv, float *outa)
{
	bi_trace();
	VectorAngles(inv, NULL, outa);
}
#define DEG2RAD( a ) ( a * M_PI ) / 180.0F
void QDECL GHL_MoveToOrigin(hledict_t *ent, vec3_t dest, float dist, int moveflags)
{
	//mode 0: move_normal
	//mode 1: no idea
	//mode 2: test only
//	float		dz;
	vec3_t		oldorg, neworg, end;
	trace_t		trace;
//	int			i;
	int eflags = ent->v.flags;
	const vec3_t up = {0, 0, 1};
	vec3_t move;
	qboolean relink = true;
	qboolean domove = true;

	bi_trace();

	if (moveflags)
	{	//strafe. just move directly.
		VectorSubtract(dest, ent->v.origin, move);
		move[2] = 0;
		VectorNormalize(move);
		VectorMA(ent->v.origin, dist, move, move);
	}
	else
	{
		float yaw = DEG2RAD(ent->v.angles[1]);
		move[0] = cos(yaw) * dist;
		move[1] = sin(yaw) * dist;
		move[2] = 0;
	}

// try the move	
	VectorCopy (ent->v.origin, oldorg);
	VectorAdd (ent->v.origin, move, neworg);

#if 0
// flying monsters don't step up
	if (eflags & (FL_SWIM | FL_FLY))
	{
	// try one move with vertical motion, then one without
		for (i=0 ; i<2 ; i++)
		{
			VectorAdd (ent->v.origin, move, neworg);
			if (!noenemy)
			{
				enemy = (wedict_t*)PROG_TO_EDICT(world->progs, ent->v->enemy);
				if (i == 0 && enemy->entnum)
				{
					VectorSubtract(ent->v->origin, ((wedict_t*)PROG_TO_EDICT(world->progs, ent->v->enemy))->v->origin, end);
					dz = DotProduct(end, axis[2]);
					if (eflags & FLH2_HUNTFACE) /*get the ent's origin_z to match its victims face*/
						dz += ((wedict_t*)PROG_TO_EDICT(world->progs, ent->v->enemy))->v->view_ofs[2];
					if (dz > 40)
						VectorMA(neworg, -8, up, neworg);
					if (dz < 30)
						VectorMA(neworg, 8, up, neworg);
				}
			}
			trace = World_Move (world, ent->v->origin, ent->v->mins, ent->v->maxs, neworg, false, ent);
			if (set_move_trace)
				set_move_trace(world->progs, set_trace_globs, &trace);
	
			if (trace.fraction == 1)
			{
				if ( (eflags & FL_SWIM) && !(World_PointContents(world, trace.endpos) & FTECONTENTS_FLUID))
					continue;	// swim monster left water
	
				if (domove)
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
#endif

// push down from a step height above the wished position
	VectorMA(neworg, movevars.stepheight, up, neworg);
	VectorMA(neworg, movevars.stepheight*-2, up, end);

	trace = SVHL_Move(neworg, ent->v.mins, ent->v.maxs, end, 0, 0, ent);

	if (trace.allsolid)
		return false;

	if (trace.startsolid)
	{
		//move up by an extra step, if needed
		VectorMA(neworg, -movevars.stepheight, up, neworg);
		trace = SVHL_Move (neworg, ent->v.mins, ent->v.maxs, end, 0, 0, ent);
		if (trace.allsolid || trace.startsolid)
			return false;
	}
	if (trace.fraction == 1)
	{
	// if monster had the ground pulled out, go ahead and fall
		if ( (int)eflags & FL_PARTIALGROUND )
		{
			if (domove)
			{
				VectorAdd (ent->v.origin, move, ent->v.origin);
				if (relink)
					SVHL_LinkEdict (ent, true);
				ent->v.flags = (int)eflags & ~FL_ONGROUND;
			}
//	Con_Printf ("fall down\n"); 
			return true;
		}
	
		return false;		// walked off an edge
	}

// check point traces down for dangling corners
	if (domove)
		VectorCopy (trace.endpos, ent->v.origin);
	
/*	if (!World_CheckBottom (world, ent, up))
	{
		if ( (int)ent->v->flags & FL_PARTIALGROUND )
		{	// entity had floor mostly pulled out from underneath it
			// and is trying to correct
			if (relink)
				SVHL_LinkEdict (ent, true);
			return true;
		}

		if (domove)
			VectorCopy (oldorg, ent->v->origin);
		return false;
	}
*/
	if ( (int)ent->v.flags & FL_PARTIALGROUND )
	{
//		Con_Printf ("back on ground\n"); 
		ent->v.flags &= ~FL_PARTIALGROUND;
	}
	ent->v.groundentity = trace.ent;

// the move is ok
	if (relink)
		SVHL_LinkEdict (ent, true);
	return true;
}
unk QDECL GHL_ChangeYaw(unk){notimpf(__func__);}
unk QDECL GHL_ChangePitch(unk){notimpf(__func__);}
hledict_t *QDECL GHL_FindEntityByString(hledict_t *last, char *field, char *value)
{
	hledict_t *ent;
	int i;
	int ofs;
	string_t str;
	bi_trace();
	if (!strcmp(field, "targetname"))
		ofs = (char*)&((hledict_t *)NULL)->v.targetname - (char*)NULL;
	else if (!strcmp(field, "classname"))
		ofs = (char*)&((hledict_t *)NULL)->v.classname - (char*)NULL;
	else
	{
		Con_Printf("Fixme: Look for %s=%s\n", field, value);
		return NULL;
	}
	
	if (last)
		i=last-SVHL_Edict+1;
	else
		i = 0;
	for (; i<SVHL_Globals.maxentities; i++)
	{
		ent = &SVHL_Edict[i];
		if (ent->isfree)
			continue;
		str = *(string_t*)((char*)ent+ofs);
		if (str && !strcmp(SVHL_Globals.stringbase+str, value))
			return ent;
	}
	return SVHL_Edict;
}
void Sh_CalcPointLight(vec3_t point, vec3_t light);
int QDECL GHL_GetEntityIllum(hledict_t *ent)
{
	vec3_t diffuse, ambient, dir;
	float lev = 0;
#if defined(RTLIGHTS) && !defined(SERVERONLY)
	Sh_CalcPointLight(ent->v.origin, ambient);
	lev += VectorLength(ambient);

	if (!r_shadow_realtime_world.ival || r_shadow_realtime_world_lightmaps.value)
#endif
	{
		sv.world.worldmodel->funcs.LightPointValues(sv.world.worldmodel, ent->v.origin, ambient, diffuse, dir);
		lev += (VectorLength(ambient) + VectorLength(diffuse)/2.0)/256;
	}
	return lev * 255; //I assume its 0-255, no idea
}
hledict_t *QDECL GHL_FindEntityInSphere(hledict_t *last, float *org, float radius)
{
	int i, j;
	vec3_t eorg;
	hledict_t *ent;

	bi_trace();

	radius = radius*radius;

	if (last)
		i=last-SVHL_Edict+1;
	else
		i = 0;
	for (; i<SVHL_Globals.maxentities; i++)
	{
		ent = &SVHL_Edict[i];
		if (ent->isfree)
			continue;
		if (!ent->v.solid)
			continue;
		for (j=0; j<3; j++)
			eorg[j] = org[j] - (ent->v.origin[j] + (ent->v.mins[j] + ent->v.maxs[j])*0.5);
		if (DotProduct(eorg,eorg) > radius)
			continue;

		//its close enough
		return ent;
	}
	return NULL;
}
hledict_t *QDECL GHL_FindClientInPVS(hledict_t *ed)
{
	qbyte	*viewerpvs;
	int best = 0, i;
	float bestdist = 99999999;	//HL maps are limited in size anyway
	float d;
	int clusternum;
	vec3_t ofs;
	hledict_t *other;

	bi_trace();

	//fixme: we need to track some state
	//a different client should be returned each call _per ent_ (so it can be used once per frame)

	viewerpvs = sv.world.worldmodel->funcs.ClusterPVS(sv.world.worldmodel, sv.world.worldmodel->funcs.ClusterForPoint(sv.world.worldmodel, ed->v.origin), NULL, 0);

	for (i = 0; i < svs.allocated_client_slots; i++)
	{
		if (svs.clients[i].state == cs_spawned)
		{
			other = &SVHL_Edict[i+1];
			if (ed == other)
				continue;	//ignore yourself.
			if (svs.clients[i].spectator)
				continue;	//ignore spectators

			clusternum = sv.world.worldmodel->funcs.ClusterForPoint(sv.world.worldmodel, other->v.origin)-1;/*pvs is 1 based, leafs are 0 based*/
			if (viewerpvs[clusternum>>3] & (1<<(clusternum&7)))
			{
				VectorSubtract(ed->v.origin, other->v.origin, ofs);
				d = DotProduct(ofs, ofs);

				if (d < bestdist)
				{
					bestdist = d;
					best = i+1;
				}
			}
		}
	}

	if (best)
		return &SVHL_Edict[best];
	return NULL;
}
unk QDECL GHL_EntitiesInPVS(unk){notimpf(__func__);}
void QDECL GHL_MakeVectors(float *angles)
{
	bi_trace();
	AngleVectors(angles, SVHL_Globals.v_forward, SVHL_Globals.v_right, SVHL_Globals.v_up);
}
void QDECL GHL_AngleVectors(float *angles, float *forward, float *right, float *up)
{
	bi_trace();
	AngleVectors(angles, forward, right, up);
}

///////////////////////////////////////////////////////////

hledict_t *QDECL GHL_CreateEntity(void)
{	
	int i;
	static int spawnnumber;
	bi_trace();
	spawnnumber++;

	for (i = sv.allocated_client_slots; i < SVHL_NumActiveEnts; i++)
	{
		if (SVHL_Edict[i].isfree)
		{
			if (SVHL_Edict[i].freetime > sv.time)
				continue;

			memset(&SVHL_Edict[i], 0, sizeof(SVHL_Edict[i]));
			SVHL_Edict[i].spawnnumber = spawnnumber;
			SVHL_Edict[i].v.edict = &SVHL_Edict[i];
			return &SVHL_Edict[i];
		}
	}
	if (i < MAX_HL_EDICTS)
	{
		SVHL_NumActiveEnts++;
		memset(&SVHL_Edict[i], 0, sizeof(SVHL_Edict[i]));
		SVHL_Edict[i].spawnnumber = spawnnumber;
		SVHL_Edict[i].v.edict = &SVHL_Edict[i];
		return &SVHL_Edict[i];
	}
	SV_Error("Ran out of free edicts");
	return NULL;
}
void QDECL GHL_RemoveEntity(hledict_t *ed)
{
	bi_trace();
	SVHL_UnlinkEdict(ed);
	ed->isfree = true;
	ed->freetime = sv.time+2;
}
hledict_t *QDECL GHL_CreateNamedEntity(string_t name)
{
	void (QDECL *spawnfunc)(hlentvars_t *evars);
	hledict_t *ed;
	bi_trace();
	ed = GHL_CreateEntity();
	if (!ed)
		return NULL;
	ed->v.classname = name;

	spawnfunc = (void(QDECL *)(hlentvars_t*))GetProcAddress((HINSTANCE)hlgamecode, name+SVHL_Globals.stringbase);
	if (spawnfunc)
		spawnfunc(&ed->v);
	return ed;
}
void *QDECL GHL_PvAllocEntPrivateData(hledict_t *ed, long quant)
{
	bi_trace();
	if (!ed)
		return NULL;
	ed->moddata = ZG_Malloc(&hlmapmemgroup, quant);
	return ed->moddata;
}
unk QDECL GHL_PvEntPrivateData(unk)
{
	bi_trace();
	notimpf(__func__);
}
unk QDECL GHL_FreeEntPrivateData(unk)
{
	bi_trace();
	notimpf(__func__);
}
unk QDECL GHL_GetVarsOfEnt(unk)
{
	bi_trace();
	notimpf(__func__);
}
hledict_t *QDECL GHL_PEntityOfEntOffset(int ednum)
{
	bi_trace();
	return (hledict_t *)(ednum + (char*)SVHL_Edict);
}
int QDECL GHL_EntOffsetOfPEntity(hledict_t *ed)
{
	bi_trace();
	return (char*)ed - (char*)SVHL_Edict;
}
int QDECL GHL_IndexOfEdict(hledict_t *ed)
{
	bi_trace();
	return ed - SVHL_Edict;
}
hledict_t *QDECL GHL_PEntityOfEntIndex(int idx)
{
	bi_trace();
	return &SVHL_Edict[idx];
}
unk QDECL GHL_FindEntityByVars(unk)
{
	bi_trace();
	notimpf(__func__);
}

///////////////////////////////////////////////////////

unk QDECL GHL_MakeStatic(unk){notimpf(__func__);}
unk QDECL GHL_EntIsOnFloor(unk){notimpf(__func__);}
int QDECL GHL_DropToFloor(hledict_t *ed)
{
	vec3_t top;
	vec3_t bottom;
	trace_t tr;
	bi_trace();
	VectorCopy(ed->v.origin, top);
	VectorCopy(ed->v.origin, bottom);
	top[2] += 1;
	bottom[2] -= 256;
	tr = SVHL_Move(top, ed->v.mins, ed->v.maxs, bottom, 0, 0, ed);
	VectorCopy(tr.endpos, ed->v.origin);
	return tr.fraction != 0 && tr.fraction != 1;
}
int QDECL GHL_WalkMove(hledict_t *ent, float yaw, float dist, int mode)
{
	//mode 0: no idea
	//mode 1: no idea
	//mode 2: test only
//	float		dz;
	vec3_t		oldorg, neworg, end;
	trace_t		trace;
//	int			i;
	int eflags = ent->v.flags;
	const vec3_t up = {0, 0, 1};
	vec3_t move;
	qboolean relink = mode != 2;
	qboolean domove = mode != 2;

	bi_trace();

	yaw = DEG2RAD(yaw);
	move[0] = cos(yaw) * dist;
	move[1] = sin(yaw) * dist;
	move[2] = 0;

// try the move	
	VectorCopy (ent->v.origin, oldorg);
	VectorAdd (ent->v.origin, move, neworg);

#if 0
// flying monsters don't step up
	if (eflags & (FL_SWIM | FL_FLY))
	{
	// try one move with vertical motion, then one without
		for (i=0 ; i<2 ; i++)
		{
			VectorAdd (ent->v.origin, move, neworg);
			if (!noenemy)
			{
				enemy = (wedict_t*)PROG_TO_EDICT(world->progs, ent->v->enemy);
				if (i == 0 && enemy->entnum)
				{
					VectorSubtract(ent->v->origin, ((wedict_t*)PROG_TO_EDICT(world->progs, ent->v->enemy))->v->origin, end);
					dz = DotProduct(end, axis[2]);
					if (eflags & FLH2_HUNTFACE) /*get the ent's origin_z to match its victims face*/
						dz += ((wedict_t*)PROG_TO_EDICT(world->progs, ent->v->enemy))->v->view_ofs[2];
					if (dz > 40)
						VectorMA(neworg, -8, up, neworg);
					if (dz < 30)
						VectorMA(neworg, 8, up, neworg);
				}
			}
			trace = World_Move (world, ent->v->origin, ent->v->mins, ent->v->maxs, neworg, false, ent);
			if (set_move_trace)
				set_move_trace(world->progs, set_trace_globs, &trace);
	
			if (trace.fraction == 1)
			{
				if ( (eflags & FL_SWIM) && !(World_PointContents(world, trace.endpos) & FTECONTENTS_FLUID))
					continue;	// swim monster left water
	
				if (domove)
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
#endif

// push down from a step height above the wished position
	VectorMA(neworg, movevars.stepheight, up, neworg);
	VectorMA(neworg, movevars.stepheight*-2, up, end);

	trace = SVHL_Move(neworg, ent->v.mins, ent->v.maxs, end, 0, 0, ent);

	if (trace.allsolid)
		return false;

	if (trace.startsolid)
	{
		//move up by an extra step, if needed
		VectorMA(neworg, -movevars.stepheight, up, neworg);
		trace = SVHL_Move (neworg, ent->v.mins, ent->v.maxs, end, 0, 0, ent);
		if (trace.allsolid || trace.startsolid)
			return false;
	}
	if (trace.fraction == 1)
	{
	// if monster had the ground pulled out, go ahead and fall
		if ( (int)eflags & FL_PARTIALGROUND )
		{
			if (domove)
			{
				VectorAdd (ent->v.origin, move, ent->v.origin);
				if (relink)
					SVHL_LinkEdict (ent, true);
				ent->v.flags = (int)eflags & ~FL_ONGROUND;
			}
//	Con_Printf ("fall down\n"); 
			return true;
		}
	
		return false;		// walked off an edge
	}

// check point traces down for dangling corners
	if (domove)
		VectorCopy (trace.endpos, ent->v.origin);
	
/*	if (!World_CheckBottom (world, ent, up))
	{
		if ( (int)ent->v->flags & FL_PARTIALGROUND )
		{	// entity had floor mostly pulled out from underneath it
			// and is trying to correct
			if (relink)
				SVHL_LinkEdict (ent, true);
			return true;
		}

		if (domove)
			VectorCopy (oldorg, ent->v->origin);
		return false;
	}
*/
	if ( (int)ent->v.flags & FL_PARTIALGROUND )
	{
//		Con_Printf ("back on ground\n"); 
		ent->v.flags &= ~FL_PARTIALGROUND;
	}
	ent->v.groundentity = trace.ent;

// the move is ok
	if (relink)
		SVHL_LinkEdict (ent, true);
	return true;
}
void QDECL GHL_SetOrigin(hledict_t *ed, float *neworg)
{
	bi_trace();
	VectorCopy(neworg, ed->v.origin);
	SVHL_LinkEdict(ed, false);
}
void QDECL GHL_EmitSound(hledict_t *ed, int chan, char *soundname, float vol, float atten, int flags, int pitch)
{
	bi_trace();
	if (*soundname == '!')
		return;	//would need us to parse sound/sentances.txt I guess
	SV_StartSound(ed-SVHL_Edict, ed->v.origin, ~0, chan, soundname, vol*255, atten, pitch, 0, 0);
}
void QDECL GHL_EmitAmbientSound(hledict_t *ed, float *org, char *soundname, float vol, float atten, unsigned int flags, int pitch)
{
	bi_trace();
	SV_StartSound(0, org, ~0, 0, soundname, vol*255, atten, pitch, 0, 0);
}
void QDECL GHL_TraceLine(float *start, float *end, int flags, hledict_t *ignore, hltraceresult_t *result)
{
	trace_t t;
	bi_trace();
	
	t = SVHL_Move(start, vec3_origin, vec3_origin, end, flags, 0, ignore);

	result->allsolid = t.allsolid;
	result->startsolid = t.startsolid;
	result->inopen = t.inopen;
	result->inwater = t.inwater;
	result->fraction = t.fraction;
	VectorCopy(t.endpos, result->endpos);
	result->planedist = t.plane.dist;
	VectorCopy(t.plane.normal, result->planenormal);
	result->touched = t.ent;
	if (!result->touched)
		result->touched = &SVHL_Edict[0];
	result->hitgroup = 0;
}
unk QDECL GHL_TraceToss(unk){notimpf(__func__);}
unk QDECL GHL_TraceMonsterHull(unk){notimpf(__func__);}
void QDECL GHL_TraceHull(float *start, float *end, int flags, int hullnum, hledict_t *ignore, hltraceresult_t *result)
{
	trace_t t;
	bi_trace();

	t = SVHL_Move(start, sv.world.worldmodel->hulls[hullnum].clip_mins, sv.world.worldmodel->hulls[hullnum].clip_maxs, end, flags, 0, ignore);

	result->allsolid = t.allsolid;
	result->startsolid = t.startsolid;
	result->inopen = t.inopen;
	result->inwater = t.inwater;
	result->fraction = t.fraction;
	VectorCopy(t.endpos, result->endpos);
	result->planedist = t.plane.dist;
	VectorCopy(t.plane.normal, result->planenormal);
	result->touched = t.ent;
	result->hitgroup = 0;
}
unk QDECL GHL_TraceModel(unk){notimpf(__func__);}
char *QDECL GHL_TraceTexture(hledict_t *againstent, vec3_t start, vec3_t end)
{
	trace_t tr;
	bi_trace();
	sv.world.worldmodel->funcs.NativeTrace(sv.world.worldmodel, 0, NULLFRAMESTATE, NULL, start, end, vec3_origin, vec3_origin, false, MASK_WORLDSOLID, &tr);
	return tr.surface->name;
}
unk QDECL GHL_TraceSphere(unk){notimpf(__func__);}
unk QDECL GHL_GetAimVector(unk){notimpf(__func__);}
void QDECL GHL_ServerCommand(char *cmd)
{
	bi_trace();
	if (!strcmp(cmd, "reload\n"))
		cmd = "restart\n";
	Cbuf_AddText(cmd, RESTRICT_PROGS);
}
void QDECL GHL_ServerExecute(void)
{
	bi_trace();
	Cbuf_ExecuteLevel(RESTRICT_PROGS);
}
unk QDECL GHL_ClientCommand(unk){notimpf(__func__);}
unk QDECL GHL_ParticleEffect(unk){notimpf(__func__);}
void QDECL GHL_LightStyle(int stylenum, char *stylestr)
{
	vec3_t rgb = {1,1,1};
	bi_trace();
	PF_applylightstyle(stylenum, stylestr, rgb);
}
int QDECL GHL_DecalIndex(char *decalname)
{
	bi_trace();
	Con_Printf("Fixme: precache decal %s\n", decalname);
	return 0;
}
int QDECL GHL_PointContents(float *org)
{
	bi_trace();
	return Q1CONTENTS_EMPTY;
}

int svhl_messagedest;
vec3_t svhl_messageorigin;
hledict_t *svhl_messageent;
void QDECL GHL_MessageBegin(int dest, int type, float *org, hledict_t *ent)
{
	bi_trace();

	svhl_messagedest = dest;
	if (org)
		VectorCopy(org, svhl_messageorigin);
	else
		VectorClear(svhl_messageorigin);
	svhl_messageent = ent;

	if (sv.multicast.cursize)
	{
		Con_Printf("MessageBegin called without MessageEnd\n");
		SZ_Clear (&sv.multicast);
	}
	MSG_WriteByte(&sv.multicast, svcfte_cgamepacket);
	MSG_WriteShort(&sv.multicast, 0);
	MSG_WriteByte(&sv.multicast, type);
}
void QDECL GHL_MessageEnd(unk)
{
	unsigned short len;
	client_t *cl;

	bi_trace();

	if (!sv.multicast.cursize)
	{
		Con_Printf("MessageEnd called without MessageBegin\n");
		return;
	}

	//update the length
	len = sv.multicast.cursize - 3;
	sv.multicast.data[1] = len&0xff;
	sv.multicast.data[2] = len>>8;

	switch(svhl_messagedest)
	{
	case MSG_BROADCAST:
		SZ_Write(&sv.datagram, sv.multicast.data, sv.multicast.cursize);
		break;
	case MSG_ONE:
		cl = &svs.clients[svhl_messageent - SVHL_Edict - 1];
		if (cl->state >= cs_spawned)
		{
			ClientReliableCheckBlock(cl, sv.multicast.cursize);
			ClientReliableWrite_SZ(cl, sv.multicast.data, sv.multicast.cursize);
		}
		break;
	case MSG_ALL:
		SZ_Write(&sv.reliable_datagram, sv.multicast.data, sv.multicast.cursize);
		break;
	case MSG_MULTICAST:
		SV_Multicast(svhl_messageorigin, MULTICAST_PVS);
		break;
	case MSG_MULTICAST+1:
		SV_Multicast(svhl_messageorigin, MULTICAST_PHS);
		break;
	case 9:
		//spectators only
		break;
	default:
		Con_Printf("GHL_MessageEnd: dest type %i not supported\n", svhl_messagedest);
		break;
	}

	SZ_Clear (&sv.multicast);
}
void QDECL GHL_WriteByte(int value)
{
	bi_trace();
	MSG_WriteByte(&sv.multicast, value & 0xff);
}
void QDECL GHL_WriteChar(int value)
{
	bi_trace();
	MSG_WriteChar(&sv.multicast, value);
}
void QDECL GHL_WriteShort(int value)
{
	bi_trace();
	MSG_WriteShort(&sv.multicast, value);
}
void QDECL GHL_WriteLong(int value)
{
	bi_trace();
	MSG_WriteLong(&sv.multicast, value);
}
void QDECL GHL_WriteAngle(float value)
{
	bi_trace();
	MSG_WriteAngle8(&sv.multicast, value);
}
void QDECL GHL_WriteCoord(float value)
{
	coorddata i = MSG_ToCoord(value, 2);
	bi_trace();
	SZ_Write (&sv.multicast, (void*)&i, 2);
}
void QDECL GHL_WriteString(char *string)
{
	bi_trace();
	MSG_WriteString(&sv.multicast, string);
}
void QDECL GHL_WriteEntity(int entnum)
{
	bi_trace();
	MSG_WriteEntity(&sv.multicast, entnum);
}


void QDECL GHL_AlertMessage(int level, char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];

	bi_trace();

	if (level == 2 && !developer.ival)
		return;

	va_start (argptr, fmt);
	vsnprintf (string,sizeof(string)-1, fmt,argptr);
	va_end (argptr);

	Con_Printf("%s\n", string);
}
void QDECL GHL_EngineFprintf(FILE *f, char *fmt, ...)
{
	bi_trace();
	SV_Error("Halflife gamecode tried to use EngineFprintf\n");
}
unk QDECL GHL_SzFromIndex(unk){notimpf(__func__);}
void *QDECL GHL_GetModelPtr(hledict_t *ed)
{
	bi_trace();
#ifdef SERVERONLY
	return NULL;
#else
	if (!ed->v.modelindex)
		return NULL;
	if (!sv.models[ed->v.modelindex])
		sv.models[ed->v.modelindex] = Mod_ForName(sv.strings.model_precache[ed->v.modelindex], false);
	return Mod_GetHalfLifeModelData(sv.models[ed->v.modelindex]);
#endif
}
int QDECL GHL_RegUserMsg(char *msgname, int msgsize)
{
	bi_trace();
	//we use 1 as the code to choose others.
	if (lastusermessage <= 1)
		return -1;

	SV_FlushSignon ();

	//for new clients
	MSG_WriteByte(&sv.signon, svcfte_cgamepacket);
	MSG_WriteShort(&sv.signon, strlen(msgname)+3);
	MSG_WriteByte(&sv.signon, 1);
	MSG_WriteByte(&sv.signon, lastusermessage);
	MSG_WriteString(&sv.signon, msgname);

	//and if the client is already spawned...
	MSG_WriteByte(&sv.reliable_datagram, svcfte_cgamepacket);
	MSG_WriteShort(&sv.reliable_datagram, strlen(msgname)+3);
	MSG_WriteByte(&sv.reliable_datagram, 1);
	MSG_WriteByte(&sv.reliable_datagram, lastusermessage);
	MSG_WriteString(&sv.reliable_datagram, msgname);

	return lastusermessage--;
}
unk QDECL GHL_AnimationAutomove(unk){notimpf(__func__);}

static void GHL_GetFrameState(hledict_t *ent, framestate_t *fstate)
{
	memset(fstate, 0, sizeof(*fstate));
		
	fstate->g[FS_REG].frametime[0] = (SVHL_Globals.time - ent->v.framestarttime) * ent->v.framerate;
	fstate->g[FS_REG].frame[0] = ent->v.frame;
	fstate->g[FS_REG].lerpweight[0] = 1;
	fstate->g[FS_REG].subblendfrac = ent->v.blending[0];
	fstate->g[FS_REG].subblend2frac = ent->v.blending[1];

	//FIXME: no lower parts set here.
	
	fstate->bonecontrols[0] = ent->v.controller[0] / 255.0;
	fstate->bonecontrols[1] = ent->v.controller[1] / 255.0;
	fstate->bonecontrols[2] = ent->v.controller[2] / 255.0;
	fstate->bonecontrols[3] = ent->v.controller[3] / 255.0;
}
static void bonemat_fromqcvectors(float *out, const float vx[3], const float vy[3], const float vz[3], const float t[3])
{
	out[0] = vx[0];
	out[1] = -vy[0];
	out[2] = vz[0];
	out[3] = t[0];
	out[4] = vx[1];
	out[5] = -vy[1];
	out[6] = vz[1];
	out[7] = t[1];
	out[8] = vx[2];
	out[9] = -vy[2];
	out[10] = vz[2];
	out[11] = t[2];
}
static void bonemat_fromidentity(float *out)
{
	out[0] = 1;
	out[1] = 0;
	out[2] = 0;
	out[3] = 0;

	out[4] = 0;
	out[5] = 1;
	out[6] = 0;
	out[7] = 0;

	out[8] = 0;
	out[9] = 0;
	out[10] = 1;
	out[11] = 0;
}
static void bonemat_toqcvectors(const float *in, float vx[3], float vy[3], float vz[3], float t[3])
{
	vx[0] = in[0];
	vx[1] = in[4];
	vx[2] = in[8];
	vy[0] = -in[1];
	vy[1] = -in[5];
	vy[2] = -in[9];
	vz[0] = in[2];
	vz[1] = in[6];
	vz[2] = in[10];
	t [0] = in[3];
	t [1] = in[7];
	t [2] = in[11];
}
static void bonemat_fromhlentity(hledict_t *ed, float *trans)
{
	vec3_t d[3], a;
	a[0] = -ed->v.angles[0];
	a[1] = ed->v.angles[1];
	a[2] = ed->v.angles[2];
	AngleVectors(a, d[0], d[1], d[2]);
	bonemat_fromqcvectors(trans, d[0], d[1], d[2], ed->v.origin);
}
void QDECL GHL_GetBonePosition(hledict_t *ed, int bone, vec3_t org, vec3_t ang)
{
	float transent[12];
	float transforms[12];
	float result[12];
	model_t *mod = sv.models[ed->v.modelindex];
	vec3_t axis[3];
	framestate_t fstate;
	GHL_GetFrameState(ed, &fstate);

	bone += 1;	//I *think* the bones are 0-based unlike our tag-based bone numbers

	if (!Mod_GetTag(mod, bone, &fstate, transforms))
	{
		bonemat_fromidentity(transforms);
	}

	bonemat_fromhlentity(ed, transent);
	R_ConcatTransforms((void*)transent, (void*)transforms, (void*)result);

	bonemat_toqcvectors(result, axis[0], axis[1], axis[2], org);
	VectorAngles(axis[0], axis[2], ang);
}

hlintptr_t QDECL GHL_FunctionFromName(char *name)
{
	bi_trace();
	return (hlintptr_t)Sys_GetAddressForName(hlgamecode, name);
}
char *QDECL GHL_NameForFunction(hlintptr_t function)
{
	bi_trace();
	return Sys_GetNameForAddress(hlgamecode, (void*)function);
}

unk QDECL GHL_ClientPrintf(unk)
{
	bi_trace();
//	SV_ClientPrintf(
	notimpf(__func__);
}
void QDECL GHL_ServerPrint(char *msg)
{
	bi_trace();
	Con_Printf("%s", msg);	
}
char *QDECL GHL_Cmd_Args(void)
{
	bi_trace();
	return Cmd_Args();
}
char *QDECL GHL_Cmd_Argv(int arg)
{
	bi_trace();
	return Cmd_Argv(arg);
}
int QDECL GHL_Cmd_Argc(unk)
{
	bi_trace();
	return Cmd_Argc();
}
unk QDECL GHL_GetAttachment(unk){notimpf(__func__);}
void QDECL GHL_CRC32_Init(hlcrc_t *crc)
{
	unsigned short crc16;
	bi_trace();
	QCRC_Init(&crc16);
	*crc = crc16;
}
void QDECL GHL_CRC32_ProcessBuffer(hlcrc_t *crc, qbyte *p, int len)
{
	unsigned short crc16 = *crc;
	bi_trace();
	while(len-->0)
	{
		QCRC_ProcessByte(&crc16, *p++);
	}
	*crc = crc16;
}
void QDECL GHL_CRC32_ProcessByte(hlcrc_t *crc, qbyte b)
{
	unsigned short crc16 = *crc;
	bi_trace();
	QCRC_ProcessByte(&crc16, b);
	*crc = crc16;
}
hlcrc_t QDECL GHL_CRC32_Final(hlcrc_t crc)
{
	unsigned short crc16 = crc;
	bi_trace();
	return QCRC_Value(crc16);
}
long QDECL GHL_RandomLong(long minv, long maxv)
{
	bi_trace();
	return minv + frandom()*(maxv-minv);
}
float QDECL GHL_RandomFloat(float minv, float maxv)
{
	bi_trace();
	return minv + frandom()*(maxv-minv);
}
unk QDECL GHL_SetView(unk){notimpf(__func__);}
unk QDECL GHL_Time(unk){notimpf(__func__);}
unk QDECL GHL_CrosshairAngle(unk){notimpf(__func__);}
void *QDECL GHL_LoadFileForMe(char *name, int *size_out)
{
	int fsize;
	void *fptr;
	bi_trace();
	fsize = FS_LoadFile(name, &fptr);
	if (size_out)
		*size_out = fsize;
	if (fsize == -1)
		return NULL;
	return fptr;
}
void QDECL GHL_FreeFile(void *fptr)
{
	bi_trace();
	FS_FreeFile(fptr);
}
unk QDECL GHL_EndSection(unk){notimpf(__func__);}
#include <sys/stat.h>
int QDECL GHL_CompareFileTime(char *fname1, char *fname2, int *result)
{
	flocation_t loc1, loc2;
	struct stat stat1, stat2;
	bi_trace();

	//results:
	//1 = f1 is newer
	//0 = equal age
	//-1 = f2 is newer
	//at least I think that's what it means.
	*result = 0;
	if (!FS_FLocateFile(fname1, FSLFRT_IFFOUND, &loc1) || !FS_FLocateFile(fname2, FSLFRT_IFFOUND, &loc2))
		return 0;

	if (stat(loc1.rawname, &stat1) || stat(loc2.rawname, &stat2))
		return 0;

	if (stat1.st_mtime > stat2.st_mtime)
		*result = 1;
	else if (stat1.st_mtime < stat2.st_mtime)
		*result = -1;
	else
		*result = 0;

	return 1;
}
void QDECL GHL_GetGameDir(char *gamedir)
{
	extern char gamedirfile[];
	bi_trace();
	//warning: the output buffer size is not specified!
	Q_strncpyz(gamedir, gamedirfile, MAX_QPATH);
}
unk QDECL GHL_Cvar_RegisterVariable(unk){notimpf(__func__);}
unk QDECL GHL_FadeClientVolume(unk){notimpf(__func__);}
unk QDECL GHL_SetClientMaxspeed(unk)
{
	bi_trace();
	notimpf(__func__);
}
unk QDECL GHL_CreateFakeClient(unk){notimpf(__func__);}
unk QDECL GHL_RunPlayerMove(unk){notimpf(__func__);}
int QDECL GHL_NumberOfEntities(void)
{
	bi_trace();
	return 0;
}
char *QDECL GHL_GetInfoKeyBuffer(hledict_t *ed)
{
	bi_trace();
	if (!ed)
		return svs.info;

	return svs.clients[ed - SVHL_Edict - 1].userinfo;
}
char *QDECL GHL_InfoKeyValue(char *infostr, char *key)
{
	bi_trace();
	return Info_ValueForKey(infostr, key);
}
unk QDECL GHL_SetKeyValue(unk){notimpf(__func__);}
unk QDECL GHL_SetClientKeyValue(unk){notimpf(__func__);}
unk QDECL GHL_IsMapValid(unk){notimpf(__func__);}
unk QDECL GHL_StaticDecal(unk){notimpf(__func__);}
unk QDECL GHL_PrecacheGeneric(unk){notimpf(__func__);}
int QDECL GHL_GetPlayerUserId(hledict_t *ed)
{
	unsigned int clnum = (ed - SVHL_Edict) - 1;
	bi_trace();
	if (clnum >= sv.allocated_client_slots)
		return -1;
	return svs.clients[clnum].userid;
}
unk QDECL GHL_BuildSoundMsg(unk){notimpf(__func__);}

int QDECL GHL_IsDedicatedServer(void)
{
	bi_trace();
#ifdef SERVERONLY
	return 1;
#else
	return qrenderer == QR_NONE;
#endif
}

hlcvar_t *hlcvar_malloced;
hlcvar_t *hlcvar_stored;
void SVHL_UpdateCvar(cvar_t *var)
{
	if (!var->hlcvar)
		return;	//nothing to do
	var->hlcvar->string = var->string;
	var->hlcvar->value = var->value;
}

void SVHL_FreeCvars(void)
{
	cvar_t *nc;
	hlcvar_t *n;

	//forget all
	while (hlcvar_malloced)
	{
		n = hlcvar_malloced;
		hlcvar_malloced = n->next;

		nc = Cvar_FindVar(n->name);
		if (nc)
			nc->hlcvar = NULL;
		Z_Free(n);
	}

	while (hlcvar_stored)
	{
		n = hlcvar_stored;
		hlcvar_stored = n->next;

		nc = Cvar_FindVar(n->name);
		if (nc)
			nc->hlcvar = NULL;
	}
}
void SVHL_FreeCvar(hlcvar_t *var)
{
	cvar_t *nc;
	hlcvar_t **ref;

	//unlink (free if it was malloced)
	ref = &hlcvar_malloced;
	while (*ref)
	{
		if (*ref == var)
		{
			(*ref) = (*ref)->next;

			nc = Cvar_FindVar(var->name);
			if (nc)
				nc->hlcvar = NULL;
			Z_Free(var);
			return;
		}
		ref = &(*ref)->next;
	}

	ref = &hlcvar_stored;
	while (*ref)
	{
		if (*ref == var)
		{
			(*ref) = (*ref)->next;

			nc = Cvar_FindVar(var->name);
			if (nc)
				nc->hlcvar = NULL;
			return;
		}
		ref = &(*ref)->next;
	}
}

hlcvar_t *QDECL GHL_CVarGetPointer(char *varname)
{
	cvar_t *var;
	hlcvar_t *hlvar;
	bi_trace();
	var = Cvar_Get(varname, "", 0, "HalfLife");
	if (!var)
	{
		Con_Printf("Not giving cvar \"%s\" to game\n", varname);
		return NULL;
	}
	hlvar = var->hlcvar;
	if (!hlvar)
	{
		hlvar = var->hlcvar = Z_Malloc(sizeof(hlcvar_t));
		hlvar->name = var->name;
		hlvar->string = var->string;
		hlvar->value = var->value;

		hlvar->next = hlcvar_malloced;
		hlcvar_malloced = hlvar;
	}
	return hlvar;
}
void QDECL GHL_CVarRegister(hlcvar_t *hlvar)
{
	cvar_t *var;
	bi_trace();
	var = Cvar_Get(hlvar->name, hlvar->string, 0, "HalfLife");
	if (!var)
	{
		Con_Printf("Not giving cvar \"%s\" to game\n", hlvar->name);
		return;
	}
	if (var->hlcvar)
	{
		SVHL_FreeCvar(var->hlcvar);

		hlvar->next = hlcvar_stored;
		hlcvar_stored = hlvar;
	}
	var->hlcvar = hlvar;
}
float QDECL GHL_CVarGetFloat(char *vname)
{
	cvar_t *var = Cvar_FindVar(vname);
	bi_trace();
	if (var)
		return var->value;
	Con_Printf("cvar %s does not exist\n", vname);
	return 0;
}
char *QDECL GHL_CVarGetString(char *vname)
{
	cvar_t *var = Cvar_FindVar(vname);
	bi_trace();
	if (var)
		return var->string;
	Con_Printf("cvar %s does not exist\n", vname);
	return "";
}
void QDECL GHL_CVarSetFloat(char *vname, float value)
{
	cvar_t *var = Cvar_FindVar(vname);
	bi_trace();
	if (var)
		Cvar_SetValue(var, value);
	else
		Con_Printf("cvar %s does not exist\n", vname);
}
void QDECL GHL_CVarSetString(char *vname, char *value)
{
	cvar_t *var = Cvar_FindVar(vname);
	bi_trace();
	if (var)
		Cvar_Set(var, value);
	else
		Con_Printf("cvar %s does not exist\n", vname);
}

unk QDECL GHL_GetPlayerWONId(unk){notimpf(__func__);}
unk QDECL GHL_Info_RemoveKey(unk){notimpf(__func__);}
unk QDECL GHL_GetPhysicsKeyValue(unk){notimpf(__func__);}
void QDECL GHL_SetPhysicsKeyValue(hledict_t *ent, char *key, char *value)
{
	bi_begin();
	notimpf(__func__);
	bi_end();
}
unk QDECL GHL_GetPhysicsInfoString(unk){notimpf(__func__);}
unsigned short QDECL GHL_PrecacheEvent(int eventtype, char *eventname)
{
	bi_trace();
	Con_Printf("Fixme: GHL_PrecacheEvent: %s\n", eventname);
	return 0;
}
void QDECL GHL_PlaybackEvent(int flags, hledict_t *ent, unsigned short eventidx, float delay, float *origin, float *angles, float f1, float f2, int i1, int i2, int b1, int b2)
{
	bi_trace();
	ignore("GHL_PlaybackEvent not implemented");
}
unk QDECL GHL_SetFatPVS(unk){notimpf(__func__);}
unk QDECL GHL_SetFatPAS(unk){notimpf(__func__);}
unk QDECL GHL_CheckVisibility(unk){notimpf(__func__);}
unk QDECL GHL_DeltaSetField(unk){notimpf(__func__);}
unk QDECL GHL_DeltaUnsetField(unk){notimpf(__func__);}
unk QDECL GHL_DeltaAddEncoder(unk){notimpf(__func__);}
unk QDECL GHL_GetCurrentPlayer(unk){notimpf(__func__);}
int QDECL GHL_CanSkipPlayer(hledict_t *playerent)
{
	bi_trace();
	return false;
//	notimpf(__func__);
}
unk QDECL GHL_DeltaFindField(unk){notimpf(__func__);}
unk QDECL GHL_DeltaSetFieldByIndex(unk){notimpf(__func__);}
unk QDECL GHL_DeltaUnsetFieldByIndex(unk){notimpf(__func__);}
unk QDECL GHL_SetGroupMask(unk){notimpf(__func__);}
unk QDECL GHL_CreateInstancedBaseline(unk){notimpf(__func__);}
unk QDECL GHL_Cvar_DirectSet(unk){notimpf(__func__);}
unk QDECL GHL_ForceUnmodified(unk){notimpf(__func__);}
unk QDECL GHL_GetPlayerStats(unk){notimpf(__func__);}
unk QDECL GHL_AddServerCommand(unk){notimpf(__func__);}
unk QDECL GHL_Voice_GetClientListening(unk){notimpf(__func__);}
qboolean QDECL GHL_Voice_SetClientListening(int listener, int sender, int shouldlisten)
{
	bi_trace();
	return false;
}
char *QDECL GHL_GetPlayerAuthId(hledict_t *playered)
{
	unsigned int clnum = (playered - SVHL_Edict) - 1;
	bi_trace();
	if (clnum >= sv.allocated_client_slots)
		return NULL;
	return svs.clients[clnum].guid;
}
unk QDECL GHL_SequenceGet(unk){notimpf(__func__);}
unk QDECL GHL_SequencePickSentence(unk){notimpf(__func__);}
unk QDECL GHL_GetFileSize(unk){notimpf(__func__);}
unk QDECL GHL_GetApproxWavePlayLen(unk){notimpf(__func__);}
unk QDECL GHL_IsCareerMatch(unk){notimpf(__func__);}
unk QDECL GHL_GetLocalizedStringLength(unk){notimpf(__func__);}
unk QDECL GHL_RegisterTutorMessageShown(unk){notimpf(__func__);}
unk QDECL GHL_GetTimesTutorMessageShown(unk){notimpf(__func__);}
unk QDECL GHL_ProcessTutorMessageDecayBuffer(unk){notimpf(__func__);}
unk QDECL GHL_ConstructTutorMessageDecayBuffer(unk){notimpf(__func__);}
unk QDECL GHL_ResetTutorMessageDecayData(unk){notimpf(__func__);}
unk QDECL GHL_QueryClientCvarValue(unk){notimpf(__func__);}
unk QDECL GHL_QueryClientCvarValue2(unk){notimpf(__func__);}




//====================================================================================================





SVHL_Builtins_t SVHL_Builtins =
{
	GHL_PrecacheModel,
	GHL_PrecacheSound,
	GHL_SetModel,
	GHL_ModelIndex,
	GHL_ModelFrames,
	GHL_SetSize,
	GHL_ChangeLevel,
	GHL_GetSpawnParms,
	GHL_SaveSpawnParms,
	GHL_VecToYaw,
	GHL_VecToAngles,
	GHL_MoveToOrigin,
	GHL_ChangeYaw,
	GHL_ChangePitch,
	GHL_FindEntityByString,
	GHL_GetEntityIllum,
	GHL_FindEntityInSphere,
	GHL_FindClientInPVS,
	GHL_EntitiesInPVS,
	GHL_MakeVectors,
	GHL_AngleVectors,
	GHL_CreateEntity,
	GHL_RemoveEntity,
	GHL_CreateNamedEntity,
	GHL_MakeStatic,
	GHL_EntIsOnFloor,
	GHL_DropToFloor,
	GHL_WalkMove,
	GHL_SetOrigin,
	GHL_EmitSound,
	GHL_EmitAmbientSound,
	GHL_TraceLine,
	GHL_TraceToss,
	GHL_TraceMonsterHull,
	GHL_TraceHull,
	GHL_TraceModel,
	GHL_TraceTexture,
	GHL_TraceSphere,
	GHL_GetAimVector,
	GHL_ServerCommand,
	GHL_ServerExecute,
	GHL_ClientCommand,
	GHL_ParticleEffect,
	GHL_LightStyle,
	GHL_DecalIndex,
	GHL_PointContents,
	GHL_MessageBegin,
	GHL_MessageEnd,
	GHL_WriteByte,
	GHL_WriteChar,
	GHL_WriteShort,
	GHL_WriteLong,
	GHL_WriteAngle,
	GHL_WriteCoord,
	GHL_WriteString,
	GHL_WriteEntity,
	GHL_CVarRegister,
	GHL_CVarGetFloat,
	GHL_CVarGetString,
	GHL_CVarSetFloat,
	GHL_CVarSetString,
	GHL_AlertMessage,
	GHL_EngineFprintf,
	GHL_PvAllocEntPrivateData,
	GHL_PvEntPrivateData,
	GHL_FreeEntPrivateData,
	GHL_SzFromIndex,
	GHL_AllocString,
	GHL_GetVarsOfEnt,
	GHL_PEntityOfEntOffset,
	GHL_EntOffsetOfPEntity,
	GHL_IndexOfEdict,
	GHL_PEntityOfEntIndex,
	GHL_FindEntityByVars,
	GHL_GetModelPtr,
	GHL_RegUserMsg,
	GHL_AnimationAutomove,
	GHL_GetBonePosition,
	GHL_FunctionFromName,
	GHL_NameForFunction,
	GHL_ClientPrintf,
	GHL_ServerPrint,
	GHL_Cmd_Args,
	GHL_Cmd_Argv,
	GHL_Cmd_Argc,
	GHL_GetAttachment,
	GHL_CRC32_Init,
	GHL_CRC32_ProcessBuffer,
	GHL_CRC32_ProcessByte,
	GHL_CRC32_Final,
	GHL_RandomLong,
	GHL_RandomFloat,
	GHL_SetView,
	GHL_Time,
	GHL_CrosshairAngle,
	GHL_LoadFileForMe,
	GHL_FreeFile,
	GHL_EndSection,
	GHL_CompareFileTime,
	GHL_GetGameDir,
	GHL_Cvar_RegisterVariable,
	GHL_FadeClientVolume,
	GHL_SetClientMaxspeed,
	GHL_CreateFakeClient,
	GHL_RunPlayerMove,
	GHL_NumberOfEntities,
	GHL_GetInfoKeyBuffer,
	GHL_InfoKeyValue,
	GHL_SetKeyValue,
	GHL_SetClientKeyValue,
	GHL_IsMapValid,
	GHL_StaticDecal,
	GHL_PrecacheGeneric,
	GHL_GetPlayerUserId,
	GHL_BuildSoundMsg,
	GHL_IsDedicatedServer,
#if HALFLIFE_API_VERSION > 138
	GHL_CVarGetPointer,
	GHL_GetPlayerWONId,
	GHL_Info_RemoveKey,
	GHL_GetPhysicsKeyValue,
	GHL_SetPhysicsKeyValue,
	GHL_GetPhysicsInfoString,
	GHL_PrecacheEvent,
	GHL_PlaybackEvent,
	GHL_SetFatPVS,
	GHL_SetFatPAS,
	GHL_CheckVisibility,
	GHL_DeltaSetField,
	GHL_DeltaUnsetField,
	GHL_DeltaAddEncoder,
	GHL_GetCurrentPlayer,
	GHL_CanSkipPlayer,
	GHL_DeltaFindField,
	GHL_DeltaSetFieldByIndex,
	GHL_DeltaUnsetFieldByIndex,
	GHL_SetGroupMask,
	GHL_CreateInstancedBaseline,
	GHL_Cvar_DirectSet,
	GHL_ForceUnmodified,
	GHL_GetPlayerStats,
	GHL_AddServerCommand,
	GHL_Voice_GetClientListening,
	GHL_Voice_SetClientListening,
	GHL_GetPlayerAuthId,
	GHL_SequenceGet,
	GHL_SequencePickSentence,
	GHL_GetFileSize,
	GHL_GetApproxWavePlayLen,
	GHL_IsCareerMatch,
	GHL_GetLocalizedStringLength,
	GHL_RegisterTutorMessageShown,
	GHL_GetTimesTutorMessageShown,
	GHL_ProcessTutorMessageDecayBuffer,
	GHL_ConstructTutorMessageDecayBuffer,
	GHL_ResetTutorMessageDecayData,
	GHL_QueryClientCvarValue,
	GHL_QueryClientCvarValue2, 
#endif

	0xdeadbeef
};

void SV_ReadLibListDotGam(void)
{
	char key[1024];
	char value[1024];
	char *file;
	char *s;
	size_t fsize;

	Info_SetValueForStarKey(svs.info, "*gamedll", "", sizeof(svs.info));
	Info_SetValueForStarKey(svs.info, "*cldll", "", sizeof(svs.info));

	file = COM_LoadTempFile("liblist.gam", &fsize);
	if (!file)
		return;

	Info_SetValueForStarKey(svs.info, "*cldll", "1", sizeof(svs.info));

	while ((file = COM_ParseOut(file, key, sizeof(key))))
	{
		file = COM_ParseOut(file, value, sizeof(value));

		while((s = strchr(value, '\\')))
			*s = '/';

		if (!strcmp(key, "gamedll"
#ifdef __linux__
			"_linux"
#endif
			))
			Info_SetValueForStarKey(svs.info, "*gamedll", value, sizeof(svs.info));
		if (!strcmp(key, "cldll"))
			Info_SetValueForStarKey(svs.info, "*cldll", atoi(value)?"1":"", sizeof(svs.info));
	}
}

int SVHL_InitGame(void)
{
	char *gamedll;
	void *iterator;
	char path[MAX_OSPATH];
	char fullname[MAX_OSPATH];
	void (WINAPI *GiveFnptrsToDll) (funcs, globals);
	int (QDECL *GetEntityAPI)(SVHL_GameFuncs_t *pFunctionTable, int apivers);

	dllfunction_t hlgamefuncs[] =
	{
		{(void**)&GiveFnptrsToDll, "GiveFnptrsToDll"},
		{(void**)&GetEntityAPI, 	"GetEntityAPI"},
		{NULL, NULL}
	};

	memset(&SVHL_Globals, 0, sizeof(SVHL_Globals));

	if (sizeof(long) != sizeof(void*))
	{
		Con_Printf("sizeof(long)!=sizeof(ptr): Cannot run halflife gamecode on this platform\n");
		return 0;
	}

	SV_ReadLibListDotGam();

	if (hlgamecode)
	{
		ZG_FreeGroup(&hlmapmemgroup);

		SVHL_Edict = ZG_Malloc(&hlmapmemgroup, sizeof(*SVHL_Edict) * MAX_HL_EDICTS);
		SVHL_Globals.maxentities = MAX_HL_EDICTS;	//I think this is correct
		return 1;
	}

	gamedll = Info_ValueForKey(svs.info, "*gamedll");
	iterator = NULL;
	//FIXME: game dlls from game paths are evil/exploitable.
	while(COM_IteratePaths(&iterator, path, sizeof(path), NULL, 0))
	{
		snprintf (fullname, sizeof(fullname), "%s%s", path, gamedll);
		hlgamecode = Sys_LoadLibrary(fullname, hlgamefuncs);
		if (hlgamecode)
			break;
	}

	if (!hlgamecode)
		return 0;

	SVHL_Edict = ZG_Malloc(&hlmapmemgroup, sizeof(*SVHL_Edict) * MAX_HL_EDICTS);
	SVHL_Globals.maxentities = MAX_HL_EDICTS;	//I think this is correct
	GiveFnptrsToDll(&SVHL_Builtins, &SVHL_Globals);

	if (!GetEntityAPI(&SVHL_GameFuncs, HALFLIFE_API_VERSION))
	{
		Con_Printf(CON_ERROR "Error: %s is incompatible (Engine is compiled for %i)\n", fullname, HALFLIFE_API_VERSION);
		if (GetEntityAPI(&SVHL_GameFuncs, 138))
			Con_Printf(CON_ERROR "mod is 138\n");
		Sys_CloseLibrary(hlgamecode);
		hlgamecode = NULL;
		return 0;
	}

	bi_begin();
	SVHL_GameFuncs.GameDLLInit();
	bi_end();
	return 1;
}

void SVHL_SaveLevelCache(char *filename)
{

}

void SVHL_SetupGame(void)
{
	lastusermessage = 255;
	//called on new map
}

void SVHL_SpawnEntities(char *entstring)
{
	char key[256];
	char value[1024];
	char classname[1024];
	hlfielddef_t fdef;
	hledict_t *ed, *world;
	extern cvar_t	coop;	//who'd have thought it, eh?
	char *ts;
	int i;

	//initialise globals
	SVHL_Globals.stringbase = "";	//uninitialised strings are considered empty and not crashy. this ensures that is true.
	SVHL_Globals.maxclients = svs.allocated_client_slots;
	SVHL_Globals.deathmatch = deathmatch.value;
	SVHL_Globals.coop = coop.value;
	SVHL_Globals.serverflags = 0;
	if (!strncmp(sv.modelname, "maps/", 5))
		COM_StripExtension(sv.modelname+5, value, sizeof(value));
	else
		COM_StripExtension(sv.modelname, value, sizeof(value));
	SVHL_Globals.mapname = GHL_AllocString(value);
	SVHL_Globals.time = 0;

	SVHL_NumActiveEnts = 0;

	sv.allocated_client_slots = 0;

	//touch world.
	world = GHL_CreateNamedEntity(GHL_AllocString("worldspawn"));
	world->v.solid = SOLID_BSP;
	GHL_SetModel(world, sv.modelname);

	//spawn player ents
	for (i = 0; i < SVHL_Globals.maxclients; i++)
	{
		sv.allocated_client_slots++;
		ed = GHL_CreateNamedEntity(GHL_AllocString("player"));
	}
	for (i = 0; i < sv.allocated_client_slots; i++)
	{
		SVHL_Edict[i].isfree = true;
	}
	sv.allocated_client_slots = i;

	//precache the inline models (and touch them).
	sv.strings.model_precache[0] = "";
	sv.strings.model_precache[1] = sv.modelname;	//the qvm doesn't have access to this array
	for (i=1 ; i<sv.world.worldmodel->numsubmodels ; i++)
	{
		const char *s = va("*%i", i);
		char *n;
		n = ZG_Malloc(&hlmapmemgroup, strlen(s)+1);
		strcpy(n, s);
		sv.strings.model_precache[1+i] = n;
		sv.models[i+1] = Mod_ForName (Mod_FixName(n, sv.modelname), false);
	}

	while (entstring)
	{
		entstring = COM_ParseOut(entstring, key, sizeof(key));
		if (strcmp(key, "{"))
			break;

		*classname = 0;

		ts = entstring;
		while (ts)
		{
			ts = COM_ParseOut(ts, key, sizeof(key));
			if (!strcmp(key, "}"))
				break;
			ts = COM_ParseOut(ts, value, sizeof(value));

			if (!strcmp(key, "classname"))
			{
				memcpy(classname, value, strlen(value)+1);
				break;
			}
		}

		if (world)
		{
			if (strcmp(classname, "worldspawn"))
				SV_Error("first entity is not worldspawn");
			ed = world;
			world = NULL;
		}
		else
			ed = GHL_CreateNamedEntity(GHL_AllocString(classname));

		while (entstring)
		{
			entstring = COM_ParseOut(entstring, key, sizeof(key));
			if (!strcmp(key, "}"))
				break;
			entstring = COM_ParseOut(entstring, value, sizeof(value));

			if (*key == '_')
				continue;

			if (!strcmp(key, "classname"))
				memcpy(classname, value, strlen(value)+1);

			fdef.handled = false;
			if (!*classname)
				fdef.classname = NULL;
			else
				fdef.classname = classname;
			fdef.key = key;
			fdef.value = value;
			SVHL_GameFuncs.DispatchKeyValue(ed, &fdef);
			if (!fdef.handled)
			{
				if (!strcmp(key, "angle"))
				{
					float a = atof(value);
					sprintf(value, "%f %f %f", 0.0f, a, 0.0f);
					strcpy(key, "angles");
					SVHL_GameFuncs.DispatchKeyValue(ed, &fdef);
				}
				if (!fdef.handled)
					Con_Printf("Bad field on %s, %s\n", classname, key);
			}
		}
		SVHL_GameFuncs.DispatchSpawn(ed);
	}

	bi_begin();
	SVHL_GameFuncs.ServerActivate(SVHL_Edict, SVHL_NumActiveEnts, sv.allocated_client_slots);
	bi_end();
}

void SVHL_ShutdownGame(void)
{
	SVHL_FreeCvars();

	//gametype changed, or server shutdown
	Sys_CloseLibrary(hlgamecode);
	hlgamecode = NULL;

	SVHL_Edict = NULL;
	memset(&SVHL_Globals, 0, sizeof(SVHL_Globals));
	memset(&SVHL_GameFuncs, 0, sizeof(SVHL_GameFuncs));
	memset(&SVHL_GameFuncsEx, 0, sizeof(SVHL_GameFuncsEx));

	ZG_FreeGroup(&hlmapmemgroup);
}

qboolean HLSV_ClientCommand(client_t *client)
{
	hledict_t *ed = &SVHL_Edict[client - svs.clients + 1];
	if (!hlgamecode)
		return false;
	if (!strcmp("noclip", Cmd_Argv(0)))
	{
		if (ed->v.movetype != MOVETYPE_NOCLIP)
			ed->v.movetype = MOVETYPE_NOCLIP;
		else
			ed->v.movetype = MOVETYPE_WALK;
		return true;
	}
	if (!strcmp("kill", Cmd_Argv(0)))
	{
		SVHL_GameFuncs.ClientKill(ed);
		return true;
	}
	bi_begin();
	SVHL_GameFuncs.ClientCommand(ed);
	bi_end();
	return true;
}

qboolean SVHL_ClientConnect(client_t *client, netadr_t adr, char rejectmessage[128])
{
	qboolean result;
	char ipadr[256];
	NET_AdrToString(ipadr, sizeof(ipadr), &adr);
	strcpy(rejectmessage, "Rejected by gamecode");

	sv.skipbprintclient = client;
	bi_begin();
	client->hledict = &SVHL_Edict[client-svs.clients+1];
	result = SVHL_GameFuncs.ClientConnect(client->hledict, client->name, ipadr, rejectmessage);
	bi_end();
	sv.skipbprintclient = NULL;

	return result;
}

void SVHL_BuildStats(client_t *client, int *si, float *sf, char **ss)
{
	hledict_t *ed = &SVHL_Edict[client - svs.clients + 1];

	si[STAT_HEALTH] = ed->v.health;
	si[STAT_VIEWHEIGHT] = ed->v.view_ofs[2];
	si[STAT_WEAPONMODELI] = SV_ModelIndex(SVHL_Globals.stringbase+ed->v.vmodelindex);
	si[STAT_ITEMS] = ed->v.weapons;
}

void SVHL_PutClientInServer(client_t *client)
{
	hledict_t *ed = &SVHL_Edict[client - svs.clients + 1];
	ed->isfree = false;
	bi_begin();
	SVHL_GameFuncs.ClientPutInServer(&SVHL_Edict[client-svs.clients+1]);
	bi_end();
}

void SVHL_DropClient(client_t *drop)
{
	hledict_t *ed = &SVHL_Edict[drop - svs.clients + 1];
	bi_begin();
	SVHL_GameFuncs.ClientDisconnect(&SVHL_Edict[drop-svs.clients+1]);
	bi_end();
	drop->hledict = NULL;
	ed->isfree = true;
}

static void SVHL_RunCmdR(hledict_t *ed, usercmd_t *ucmd)
{
	int i;
	hledict_t *other;

	// chop up very long commands
	if (ucmd->msec > 50)
	{
		usercmd_t cmd = *ucmd;

		cmd.msec = ucmd->msec/2;
		cmd.msec_compat = floor(cmd.msec);
		SVHL_RunCmdR (ed, &cmd);
		cmd.msec_compat = ucmd->msec - cmd.msec_compat;
		cmd.impulse = 0;
		SVHL_RunCmdR (ed, &cmd);
		return;
	}

	host_frametime = ucmd->msec * 0.001;
	host_frametime *= sv.gamespeed;
	if (host_frametime > 0.1)
		host_frametime = 0.1;

	pmove.cmd = *ucmd;
	switch(ed->v.movetype)
	{
	default:
	case MOVETYPE_WALK:
		pmove.pm_type = PM_NORMAL;
		break;
	case MOVETYPE_FLY:
		pmove.pm_type = PM_FLY;
		break;
	case MOVETYPE_NOCLIP:
		pmove.pm_type = PM_SPECTATOR;
		break;
	case MOVETYPE_NONE:
		pmove.pm_type = PM_NONE;
		break;
	}
	pmove.numphysent = 1;
	pmove.physents[0].model = sv.world.worldmodel;
	pmove.physents[0].info = 0;
	pmove.skipent = -1;
	pmove.onladder = false;
	pmove.capsule = false;

	if (ed->v.flags & (1<<24))
	{
		pmove.cmd.forwardmove = 0;
		pmove.cmd.sidemove = 0;
		pmove.cmd.upmove = 0;
	}

	{
		hledict_t *list[256];
		int count;
		physent_t *pe;
		vec3_t	pmove_mins, pmove_maxs;

		for (i = 0; i < 3; i++)
		{
			pmove_mins[i] = pmove.origin[i] - 256;
			pmove_maxs[i] = pmove.origin[i] + 256;
		}

		count = SVHL_AreaEdicts(pmove_mins, pmove_maxs, list, sizeof(list)/sizeof(list[0]));
		for (i = 0; i < count; i++)
		{
			other = list[i];
			if (other == ed)
				continue;
			if (other->v.owner == ed)
				continue;
			if (other->v.flags & (1<<23))	//has monsterclip flag set, so ignore it
				continue;

			pe = &pmove.physents[pmove.numphysent];
			switch(other->v.skin)
			{
			case Q1CONTENTS_EMPTY:
				pe->forcecontentsmask = FTECONTENTS_EMPTY;
				break;
			case Q1CONTENTS_SOLID:
				pe->forcecontentsmask = FTECONTENTS_SOLID;
				break;
			case Q1CONTENTS_WATER:
				pe->forcecontentsmask = FTECONTENTS_WATER;
				break;
			case Q1CONTENTS_SLIME:
				pe->forcecontentsmask = FTECONTENTS_SLIME;
				break;
			case Q1CONTENTS_LAVA:
				pe->forcecontentsmask = FTECONTENTS_LAVA;
				break;
			case Q1CONTENTS_SKY:
				pe->forcecontentsmask = FTECONTENTS_SKY;
				break;
			case Q1CONTENTS_LADDER:
				pe->forcecontentsmask = FTECONTENTS_LADDER;
				break;
			default:
				pe->forcecontentsmask = 0;
				break;
			}

			if (other->v.solid == SOLID_NOT || other->v.solid == SOLID_TRIGGER)
			{
				if (!pe->forcecontentsmask)
					continue;
				pe->nonsolid = true;
			}
			else
				pe->nonsolid = false;

			if (other->v.modelindex)
			{
				pe->model = sv.models[other->v.modelindex];
				if (pe->model && pe->model->type != mod_brush)
					pe->model = NULL;
			}
			else
				pe->model = NULL;
			pmove.numphysent++;
			pe->info = other - SVHL_Edict;
			VectorCopy(other->v.origin, pe->origin);
			VectorCopy(other->v.mins, pe->mins);
			VectorCopy(other->v.maxs, pe->maxs);
			VectorCopy(other->v.angles, pe->angles);
		}
	}

	VectorCopy(ed->v.mins, pmove.player_mins);
	VectorCopy(ed->v.maxs, pmove.player_maxs);

	VectorCopy(ed->v.origin, pmove.origin);
	VectorCopy(ed->v.velocity, pmove.velocity);
	if (ed->v.flags & (1<<22))
	{
		VectorCopy(ed->v.basevelocity, pmove.basevelocity);
	}
	else
		VectorClear(pmove.basevelocity);

	PM_PlayerMove(sv.gamespeed);

	VectorCopy(pmove.origin, ed->v.origin);
	VectorCopy(pmove.velocity, ed->v.velocity);

	if (pmove.onground)
	{
		ed->v.flags |= FL_ONGROUND;
		ed->v.groundentity = &SVHL_Edict[pmove.physents[pmove.groundent].info];
	}
	else
		ed->v.flags &= ~FL_ONGROUND;

	for (i = 0; i < pmove.numtouch; i++)
	{
		other = &SVHL_Edict[pmove.physents[pmove.touchindex[i]].info];
		SVHL_GameFuncs.DispatchTouch(other, ed);
	}

	SVHL_LinkEdict(ed, true);
}

void SVHL_RunCmd(client_t *cl, usercmd_t *ucmd)
{
	hledict_t *ed = &SVHL_Edict[cl - svs.clients + 1];

#if HALFLIFE_API_VERSION >= 140
	ed->v.buttons = ucmd->buttons;
#else
	//assume they're not running halflife cgame
	ed->v.buttons = 0;

	if (ucmd->buttons & 1)
		ed->v.buttons |= (1<<0);	//shoot
	if (ucmd->buttons & 2)
		ed->v.buttons |= (1<<1);	//jump
	if (ucmd->buttons & 8)
		ed->v.buttons |= (1<<2);	//duck
	if (ucmd->forwardmove > 0)
		ed->v.buttons |= (1<<3);	//forward
	if (ucmd->forwardmove < 0)
		ed->v.buttons |= (1<<4);	//back
	if (ucmd->buttons & 4)
		ed->v.buttons |= (1<<5);	//use
	//ed->v.buttons |= (1<<6);	//cancel
	//ed->v.buttons |= (1<<7);	//turn left
	//ed->v.buttons |= (1<<8);	//turn right
	if (ucmd->sidemove > 0)
		ed->v.buttons |= (1<<9);	//move left
	if (ucmd->sidemove < 0)
		ed->v.buttons |= (1<<10);	//move right
	//ed->v.buttons |= (1<<11);	//shoot2
	//ed->v.buttons |= (1<<12);	//run
	if (ucmd->buttons & 16)
		ed->v.buttons |= (1<<13);	//reload
	//ed->v.buttons |= (1<<14);	//alt1
	//ed->v.buttons |= (1<<15);	//alt2
#endif

	if (ucmd->impulse)
		ed->v.impulse = ucmd->impulse;
	ed->v.v_angle[0] = SHORT2ANGLE(ucmd->angles[0]);
	ed->v.v_angle[1] = SHORT2ANGLE(ucmd->angles[1]);
	ed->v.v_angle[2] = SHORT2ANGLE(ucmd->angles[2]);

	ed->v.angles[0] = 0;
	ed->v.angles[1] = SHORT2ANGLE(ucmd->angles[1]);
	ed->v.angles[2] = SHORT2ANGLE(ucmd->angles[2]);

	if (IS_NAN(ed->v.velocity[0]) || IS_NAN(ed->v.velocity[1]) || IS_NAN(ed->v.velocity[2]))
		VectorClear(ed->v.velocity);

	bi_begin();
	SVHL_GameFuncs.PlayerPreThink(ed);
	SVHL_RunCmdR(ed, ucmd);

	if (ed->v.nextthink && ed->v.nextthink < sv.time)
	{
		ed->v.nextthink = 0;
		SVHL_GameFuncs.DispatchThink(ed);
	}

	SVHL_GameFuncs.PlayerPostThink(ed);

	bi_end();
}


void SVHL_RunPlayerCommand(client_t *cl, usercmd_t *oldest, usercmd_t *oldcmd, usercmd_t *newcmd)
{
	hledict_t *e = &SVHL_Edict[cl - svs.clients + 1];

	SVHL_Globals.time = sv.time;
	if (net_drop < 20)
	{
		while (net_drop > 2)
		{
			SVHL_RunCmd (cl, &cl->lastcmd);
			net_drop--;
		}
		if (net_drop > 1)
			SVHL_RunCmd (cl, oldest);
		if (net_drop > 0)
			SVHL_RunCmd (cl, oldcmd);
	}
	SVHL_RunCmd (cl, newcmd);
}

void SVHL_Snapshot_Build(client_t *client, packet_entities_t *pack, qbyte *pvs, edict_t *clent, qboolean ignorepvs)
{
	hledict_t *e;
	entity_state_t *s;
	int i;

	pack->servertime = sv.time;
	pack->num_entities = 0;

	for (i = 1; i < MAX_HL_EDICTS; i++)
	{
		e = &SVHL_Edict[i];
		if (!e)
			break;
		if (e->isfree)
			continue;

		if (!e->v.modelindex || !e->v.model)
			continue;
		if (e->v.effects & 128)
			continue;

		if (pack->num_entities == pack->max_entities)
			break;

		s = &pack->entities[pack->num_entities++];

		s->number = i;
		s->modelindex = e->v.modelindex;
		s->frame = e->v.sequence1;
		s->effects = e->v.effects & 0x0f;
		s->dpflags = 0;
		s->skinnum = e->v.skin;
		s->scale = 16;
		s->trans = 255;
		s->colormod[0] = nullentitystate.colormod[0];
		s->colormod[1] = nullentitystate.colormod[1];
		s->colormod[2] = nullentitystate.colormod[2];
		VectorCopy(e->v.angles, s->angles);
		VectorCopy(e->v.origin, s->origin);

		if (!e->v.velocity[0] && !e->v.velocity[1] && !e->v.velocity[2])
			s->dpflags |= RENDER_STEP;

		s->trans = e->v.renderamt*255;
		switch (e->v.rendermode)
		{
		case 0:
			s->trans = 255;
			break;
		case 1:	//used on laser beams, apparently. disables textures or something.
			break;
		case 2:	//transparent windows.
			break;
		case 3:	//used on coronarey sprites. both additive and resizing to give constant distance
			s->effects |= NQEF_ADDITIVE;
			break;
		case 4:	//used on fence textures, apparently. we already deal with these clientside.
			s->trans = 255;
			break;
		case 5:	//used on the torch at the start.
			s->effects |= NQEF_ADDITIVE;
			break;
		default:
			Con_Printf("Rendermode %s %i\n", SVHL_Globals.stringbase+e->v.model, e->v.rendermode);
			break;
		}
	}
}

qbyte *SVHL_Snapshot_SetupPVS(client_t *client, qbyte *pvs, unsigned int pvsbufsize)
{
	vec3_t org;

	if (client->hledict)
	{
		VectorAdd (client->hledict->v.origin, client->hledict->v.view_ofs, org);
		sv.world.worldmodel->funcs.FatPVS(sv.world.worldmodel, org, pvs, pvsbufsize, false);
	}

	return pvs;
}

#endif
