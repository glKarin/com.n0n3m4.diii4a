#include "quakedef.h"

#include "prvm_cmds.h"
#include "jpeg.h"

//============================================================================
// Server



const char *vm_sv_extensions[] = {
"BX_WAL_SUPPORT",
"DP_BUTTONCHAT",
"DP_BUTTONUSE",
"DP_CL_LOADSKY",
"DP_CON_ALIASPARAMETERS",
"DP_CON_BESTWEAPON",
"DP_CON_EXPANDCVAR",
"DP_CON_SET",
"DP_CON_SETA",
"DP_CON_STARTMAP",
"DP_COVERAGE",
"DP_CRYPTO",
"DP_CSQC_BINDMAPS",
"DP_CSQC_ENTITYWORLDOBJECT",
"DP_CSQC_ENTITYMODELLIGHT",
"DP_CSQC_ENTITYTRANSPARENTSORTING_OFFSET",
"DP_CSQC_MAINVIEW",
"DP_CSQC_MINFPS_QUALITY",
"DP_CSQC_MULTIFRAME_INTERPOLATION",
"DP_CSQC_BOXPARTICLES",
"DP_CSQC_SPAWNPARTICLE",
"DP_CSQC_QUERYRENDERENTITY",
"DP_CSQC_ROTATEMOVES",
"DP_CSQC_SETPAUSE",
"DP_CSQC_V_CALCREFDEF_WIP1",
"DP_CSQC_V_CALCREFDEF_WIP2",
"DP_EF_ADDITIVE",
"DP_EF_BLUE",
"DP_EF_DOUBLESIDED",
"DP_EF_DYNAMICMODELLIGHT",
"DP_EF_FLAME",
"DP_EF_FULLBRIGHT",
"DP_EF_NODEPTHTEST",
"DP_EF_NODRAW",
"DP_EF_NOGUNBOB",
"DP_EF_NOSELFSHADOW",
"DP_EF_NOSHADOW",
"DP_EF_RED",
"DP_EF_RESTARTANIM_BIT",
"DP_EF_STARDUST",
"DP_EF_TELEPORT_BIT",
"DP_ENT_ALPHA",
"DP_ENT_COLORMOD",
"DP_ENT_CUSTOMCOLORMAP",
"DP_ENT_EXTERIORMODELTOCLIENT",
"DP_ENT_GLOW",
"DP_ENT_GLOWMOD",
"DP_ENT_LOWPRECISION",
"DP_ENT_SCALE",
"DP_ENT_TRAILEFFECTNUM",
"DP_ENT_VIEWMODEL",
"DP_GFX_EXTERNALTEXTURES",
"DP_GFX_EXTERNALTEXTURES_PERMAP",
"DP_GFX_FOG",
"DP_GFX_MODEL_INTERPOLATION",
"DP_GFX_QUAKE3MODELTAGS",
"DP_GFX_SKINFILES",
"DP_GFX_SKYBOX",
"DP_GFX_FONTS",
"DP_GFX_FONTS_FREETYPE",
"DP_UTF8",
"DP_FONT_VARIABLEWIDTH",
"DP_HALFLIFE_MAP",
"DP_HALFLIFE_MAP_CVAR",
"DP_HALFLIFE_SPRITE",
"DP_INPUTBUTTONS",
"DP_LIGHTSTYLE_STATICVALUE",
"DP_LITSPRITES",
"DP_LITSUPPORT",
"DP_MONSTERWALK",
"DP_MOVETYPEBOUNCEMISSILE",
"DP_MOVETYPEFLYWORLDONLY",
"DP_MOVETYPEFOLLOW",
"DP_NULL_MODEL",
"DP_QC_ASINACOSATANATAN2TAN",
"DP_QC_AUTOCVARS",
"DP_QC_CHANGEPITCH",
"DP_QC_CMD",
"DP_QC_COPYENTITY",
"DP_QC_CRC16",
"DP_QC_CVAR_DEFSTRING",
"DP_QC_CVAR_DESCRIPTION",
"DP_QC_CVAR_STRING",
"DP_QC_CVAR_TYPE",
"DP_QC_DIGEST",
"DP_QC_DIGEST_SHA256",
"DP_QC_EDICT_NUM",
"DP_QC_ENTITYDATA",
"DP_QC_ENTITYSTRING",
"DP_QC_ETOS",
"DP_QC_EXTRESPONSEPACKET",
"DP_QC_FINDBOX",
"DP_QC_FINDCHAIN",
"DP_QC_FINDCHAINFLAGS",
"DP_QC_FINDCHAINFLOAT",
"DP_QC_FINDCHAIN_TOFIELD",
"DP_QC_FINDFLAGS",
"DP_QC_FINDFLOAT",
"DP_QC_FS_SEARCH",
"DP_QC_FS_SEARCH_PACKFILE",
"DP_QC_GETLIGHT",
"DP_QC_GETSURFACE",
"DP_QC_GETSURFACETRIANGLE",
"DP_QC_GETSURFACEPOINTATTRIBUTE",
"DP_QC_GETTAGINFO",
"DP_QC_GETTAGINFO_BONEPROPERTIES",
"DP_QC_GETTIME",
"DP_QC_GETTIME_CDTRACK",
"DP_QC_I18N",
"DP_QC_LOG",
"DP_QC_MINMAXBOUND",
"DP_QC_MULTIPLETEMPSTRINGS",
"DP_QC_NUDGEOUTOFSOLID",
"DP_QC_NUM_FOR_EDICT",
"DP_QC_RANDOMVEC",
"DP_QC_SINCOSSQRTPOW",
"DP_QC_SPRINTF",
"DP_QC_STRFTIME",
"DP_QC_STRINGBUFFERS",
"DP_QC_STRINGBUFFERS_CVARLIST",
"DP_QC_STRINGBUFFERS_EXT_WIP",
"DP_QC_STRINGCOLORFUNCTIONS",
"DP_QC_STRING_CASE_FUNCTIONS",
"DP_QC_STRREPLACE",
"DP_QC_TOKENIZEBYSEPARATOR",
"DP_QC_TOKENIZE_CONSOLE",
"DP_QC_TRACEBOX",
"DP_QC_TRACETOSS",
"DP_QC_TRACE_MOVETYPE_HITMODEL",
"DP_QC_TRACE_MOVETYPE_WORLDONLY",
"DP_QC_UNLIMITEDTEMPSTRINGS",
"DP_QC_URI_ESCAPE",
"DP_QC_URI_GET",
"DP_QC_URI_POST",
"DP_QC_VECTOANGLES_WITH_ROLL",
"DP_QC_VECTORVECTORS",
"DP_QC_WHICHPACK",
"DP_QUAKE2_MODEL",
"DP_QUAKE2_SPRITE",
"DP_QUAKE3_MAP",
"DP_QUAKE3_MODEL",
"DP_REGISTERCVAR",
"DP_SKELETONOBJECTS",
"DP_SND_DIRECTIONLESSATTNNONE",
"DP_SND_FAKETRACKS",
"DP_SND_SOUND7_WIP1",
"DP_SND_SOUND7_WIP2",
"DP_SND_OGGVORBIS",
"DP_SND_SETPARAMS",
"DP_SND_STEREOWAV",
"DP_SND_GETSOUNDTIME",
"DP_VIDEO_DPV",
"DP_VIDEO_SUBTITLES",
"DP_SOLIDCORPSE",
"DP_SPRITE32",
"DP_SV_BOTCLIENT",
"DP_SV_BOUNCEFACTOR",
"DP_SV_CLIENTCAMERA",
"DP_SV_CLIENTCOLORS",
"DP_SV_CLIENTNAME",
"DP_SV_CMD",
"DP_SV_CUSTOMIZEENTITYFORCLIENT",
"DP_SV_DISABLECLIENTPREDICTION",
"DP_SV_DISCARDABLEDEMO",
"DP_SV_DRAWONLYTOCLIENT",
"DP_SV_DROPCLIENT",
"DP_SV_EFFECT",
"DP_SV_ENTITYCONTENTSTRANSITION",
"DP_SV_MODELFLAGS_AS_EFFECTS",
"DP_SV_MOVETYPESTEP_LANDEVENT",
"DP_SV_NETADDRESS",
"DP_SV_NODRAWTOCLIENT",
"DP_SV_ONENTITYNOSPAWNFUNCTION",
"DP_SV_ONENTITYPREPOSTSPAWNFUNCTION",
"DP_SV_PING",
"DP_SV_PING_PACKETLOSS",
"DP_SV_PLAYERPHYSICS",
"DP_PHYSICS_ODE",
"DP_SV_POINTPARTICLES",
"DP_SV_POINTSOUND",
"DP_SV_PRECACHEANYTIME",
"DP_SV_PRINT",
"DP_SV_PUNCHVECTOR",
"DP_SV_QCSTATUS",
"DP_SV_ROTATINGBMODEL",
"DP_SV_SETCOLOR",
"DP_SV_SHUTDOWN",
"DP_SV_SLOWMO",
"DP_SV_SPAWNFUNC_PREFIX",
"DP_SV_WRITEPICTURE",
"DP_SV_WRITEUNTERMINATEDSTRING",
"DP_TE_BLOOD",
"DP_TE_BLOODSHOWER",
"DP_TE_CUSTOMFLASH",
"DP_TE_EXPLOSIONRGB",
"DP_TE_FLAMEJET",
"DP_TE_PARTICLECUBE",
"DP_TE_PARTICLERAIN",
"DP_TE_PARTICLESNOW",
"DP_TE_PLASMABURN",
"DP_TE_QUADEFFECTS1",
"DP_TE_SMALLFLASH",
"DP_TE_SPARK",
"DP_TE_STANDARDEFFECTBUILTINS",
"DP_TRACE_HITCONTENTSMASK_SURFACEINFO"
"DP_USERMOVETYPES",
"DP_VIEWZOOM",
"EXT_BITSHIFT",
"FRIK_FILE",
"FTE_CSQC_SKELETONOBJECTS",
"FTE_QC_CHECKPVS",
"FTE_STRINGS",
"KRIMZON_SV_PARSECLIENTCOMMAND",
"NEH_CMD_PLAY2",
"NEH_RESTOREGAME",
"NEXUIZ_PLAYERMODEL",
"NXQ_GFX_LETTERBOX",
"PRYDON_CLIENTCURSOR",
"TENEBRAE_GFX_DLIGHTS",
"TW_SV_STEPCONTROL",
"ZQ_PAUSE",
"DP_RM_CLIPGROUP",
NULL
//"EXT_CSQC" // not ready yet
};

/*
=================
VM_SV_setorigin

This is the only valid way to move an object without using the physics of the world (setting velocity and waiting).  Directly changing origin will not set internal links correctly, so clipping would be messed up.  This should be called when an object is spawned, and then only if it is teleported.

setorigin (entity, origin)
=================
*/
static void VM_SV_setorigin(prvm_prog_t *prog)
{
	prvm_edict_t	*e;

	VM_SAFEPARMCOUNT(2, VM_SV_setorigin);

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
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), PRVM_serveredictvector(e, origin));
	if(e->priv.required->mark == PRVM_EDICT_MARK_WAIT_FOR_SETORIGIN)
		e->priv.required->mark = PRVM_EDICT_MARK_SETORIGIN_CAUGHT;
	SV_LinkEdict(e);
}

// TODO: rotate param isnt used.. could be a bug. please check this and remove it if possible [1/10/2008 Black]
static void SetMinMaxSize (prvm_prog_t *prog, prvm_edict_t *e, float *min, float *max, qbool rotate)
{
	int		i;

	for (i=0 ; i<3 ; i++)
		if (min[i] > max[i])
			prog->error_cmd("SetMinMaxSize: backwards mins/maxs");

// set derived values
	VectorCopy (min, PRVM_serveredictvector(e, mins));
	VectorCopy (max, PRVM_serveredictvector(e, maxs));
	VectorSubtract (max, min, PRVM_serveredictvector(e, size));

	SV_LinkEdict(e);
}

/*
=================
VM_SV_setsize

the size box is rotated by the current angle
LadyHavoc: no it isn't...

setsize (entity, minvector, maxvector)
=================
*/
static void VM_SV_setsize(prvm_prog_t *prog)
{
	prvm_edict_t	*e;
	vec3_t mins, maxs;

	VM_SAFEPARMCOUNT(3, VM_SV_setsize);

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
	SetMinMaxSize(prog, e, mins, maxs, false);
}


/*
=================
VM_SV_setmodel

setmodel(entity, model)
=================
*/
static vec3_t quakemins = {-16, -16, -16}, quakemaxs = {16, 16, 16};
static void VM_SV_setmodel(prvm_prog_t *prog)
{
	prvm_edict_t	*e;
	model_t	*mod;
	int		i;

	VM_SAFEPARMCOUNT(2, VM_SV_setmodel);

	e = PRVM_G_EDICT(OFS_PARM0);
	if (e == prog->edicts)
	{
		VM_Warning(prog, "setmodel: can not modify world entity\n");
		return;
	}
	if (e->free)
	{
		VM_Warning(prog, "setmodel: can not modify free entity\n");
		return;
	}
	i = SV_ModelIndex(PRVM_G_STRING(OFS_PARM1), 1);
	PRVM_serveredictstring(e, model) = PRVM_SetEngineString(prog, sv.model_precache[i]);
	PRVM_serveredictfloat(e, modelindex) = i;

	mod = SV_GetModelByIndex(i);

	if (mod)
	{
		if (mod->type != mod_alias || sv_gameplayfix_setmodelrealbox.integer)
			SetMinMaxSize(prog, e, mod->normalmins, mod->normalmaxs, true);
		else
			SetMinMaxSize(prog, e, quakemins, quakemaxs, true);
	}
	else
		SetMinMaxSize(prog, e, vec3_origin, vec3_origin, true);
}

/*
=================
VM_SV_sprint

single print to a specific client

sprint(clientent, value)
=================
*/
static void VM_SV_sprint(prvm_prog_t *prog)
{
	client_t	*client;
	int			entnum;
	char string[VM_TEMPSTRING_MAXSIZE];

	VM_SAFEPARMCOUNTRANGE(2, 8, VM_SV_sprint);

	VM_VarString(prog, 1, string, sizeof(string));

	entnum = PRVM_G_EDICTNUM(OFS_PARM0);
	// LadyHavoc: div0 requested that sprintto world  operate like print
	if (entnum == 0)
	{
		Con_Print(string);
		return;
	}

	if (entnum < 1 || entnum > svs.maxclients || !svs.clients[entnum-1].active)
	{
		VM_Warning(prog, "tried to centerprint to a non-client\n");
		return;
	}

	client = svs.clients + entnum-1;
	if (!client->netconnection)
		return;

	MSG_WriteChar(&client->netconnection->message,svc_print);
	MSG_WriteString(&client->netconnection->message, string);
}


/*
=================
VM_SV_centerprint

single print to a specific client

centerprint(clientent, value)
=================
*/
static void VM_SV_centerprint(prvm_prog_t *prog)
{
	client_t	*client;
	int			entnum;
	char string[VM_TEMPSTRING_MAXSIZE];

	VM_SAFEPARMCOUNTRANGE(2, 8, VM_SV_centerprint);

	entnum = PRVM_G_EDICTNUM(OFS_PARM0);

	if (entnum < 1 || entnum > svs.maxclients || !svs.clients[entnum-1].active)
	{
		VM_Warning(prog, "tried to centerprint to a non-client\n");
		return;
	}

	client = svs.clients + entnum-1;
	if (!client->netconnection)
		return;

	VM_VarString(prog, 1, string, sizeof(string));
	MSG_WriteChar(&client->netconnection->message,svc_centerprint);
	MSG_WriteString(&client->netconnection->message, string);
}

/*
=================
VM_SV_particle

particle(origin, color, count)
=================
*/
static void VM_SV_particle(prvm_prog_t *prog)
{
	vec3_t		org, dir;
	int		color;
	int		count;

	VM_SAFEPARMCOUNT(4, VM_SV_particle);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), org);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), dir);
	color = (int)PRVM_G_FLOAT(OFS_PARM2);
	count = (int)PRVM_G_FLOAT(OFS_PARM3);
	SV_StartParticle (org, dir, color, count);
}


/*
=================
VM_SV_ambientsound

=================
*/
static void VM_SV_ambientsound(prvm_prog_t *prog)
{
	const char	*samp;
	vec3_t		pos;
	prvm_vec_t	vol, attenuation;
	int			soundnum, large;

	VM_SAFEPARMCOUNT(4, VM_SV_ambientsound);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), pos);
	samp = PRVM_G_STRING(OFS_PARM1);
	vol = PRVM_G_FLOAT(OFS_PARM2);
	attenuation = PRVM_G_FLOAT(OFS_PARM3);

// check to see if samp was properly precached
	soundnum = SV_SoundIndex(samp, 1);
	if (!soundnum)
		return;

	large = false;
	if (soundnum >= 256)
		large = true;

	if(sv.protocol == PROTOCOL_NEHAHRABJP)
		large = false;

	// add an svc_spawnambient command to the level signon packet

	if (large)
		MSG_WriteByte (&sv.signon, svc_spawnstaticsound2);
	else
		MSG_WriteByte (&sv.signon, svc_spawnstaticsound);

	MSG_WriteVector(&sv.signon, pos, sv.protocol);

	if (large || sv.protocol == PROTOCOL_NEHAHRABJP2 || sv.protocol == PROTOCOL_NEHAHRABJP3)
		MSG_WriteShort (&sv.signon, soundnum);
	else
		MSG_WriteByte (&sv.signon, soundnum);

	MSG_WriteByte (&sv.signon, (int)(vol*255));
	MSG_WriteByte (&sv.signon, (int)(attenuation*64));

}

/*
=================
VM_SV_sound

Each entity can have eight independant sound sources, like voice,
weapon, feet, etc.

Channel 0 is an auto-allocate channel, the others override anything
already running on that entity/channel pair.

An attenuation of 0 will play full volume everywhere in the level.
Larger attenuations will drop off.

void(entity e, float chan, string samp, float volume[, float atten[, float pitchchange[, float flags]]]) sound (QUAKE)
=================
*/
static void VM_SV_sound(prvm_prog_t *prog)
{
	const char	*sample;
	int			channel;
	prvm_edict_t		*entity;
	int 		nvolume;
	int flags;
	float attenuation;
	float pitchchange;

	VM_SAFEPARMCOUNTRANGE(4, 7, VM_SV_sound);

	entity = PRVM_G_EDICT(OFS_PARM0);
	channel = (int)PRVM_G_FLOAT(OFS_PARM1);
	sample = PRVM_G_STRING(OFS_PARM2);
	nvolume = (int)(PRVM_G_FLOAT(OFS_PARM3) * 255);
	if (prog->argc < 5)
	{
		Con_DPrintf("VM_SV_sound: given only 4 parameters, expected 5, assuming attenuation = ATTN_NORMAL\n");
		attenuation = 1;
	}
	else
		attenuation = PRVM_G_FLOAT(OFS_PARM4);
	if (prog->argc < 6)
		pitchchange = 0;
	else
		pitchchange = PRVM_G_FLOAT(OFS_PARM5) * 0.01f;

	if (prog->argc < 7)
	{
		flags = 0;
		if(channel >= 8 && channel <= 15) // weird QW feature
		{
			flags |= CHANNELFLAG_RELIABLE;
			channel -= 8;
		}
	}
	else
	{
		// LadyHavoc: we only let the qc set certain flags, others are off-limits
		flags = (int)PRVM_G_FLOAT(OFS_PARM6) & (CHANNELFLAG_RELIABLE | CHANNELFLAG_FORCELOOP | CHANNELFLAG_PAUSED | CHANNELFLAG_FULLVOLUME);
	}

	if (nvolume < 0 || nvolume > 255)
	{
		VM_Warning(prog, "SV_StartSound: volume must be in range 0-1\n");
		return;
	}

	if (attenuation < 0 || attenuation > 4)
	{
		VM_Warning(prog, "SV_StartSound: attenuation must be in range 0-4\n");
		return;
	}

	channel = CHAN_USER2ENGINE(channel);

	if (!IS_CHAN(channel))
	{
		VM_Warning(prog, "SV_StartSound: channel must be in range 0-127\n");
		return;
	}

	SV_StartSound (entity, channel, sample, nvolume, attenuation, flags & CHANNELFLAG_RELIABLE, pitchchange);
}

/*
=================
VM_SV_pointsound

Follows the same logic as VM_SV_sound, except instead of
an entity, an origin for the sound is provided, and channel
is omitted (since no entity is being tracked).

=================
*/
static void VM_SV_pointsound(prvm_prog_t *prog)
{
	const char	*sample;
	int 		nvolume;
	float		attenuation;
	float		pitchchange;
	vec3_t		org;

	VM_SAFEPARMCOUNTRANGE(4, 5, VM_SV_pointsound);

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), org);
	sample = PRVM_G_STRING(OFS_PARM1);
	nvolume = (int)(PRVM_G_FLOAT(OFS_PARM2) * 255);
	attenuation = PRVM_G_FLOAT(OFS_PARM3);
	pitchchange = prog->argc < 5 ? 0 : PRVM_G_FLOAT(OFS_PARM4) * 0.01f;

	if (nvolume < 0 || nvolume > 255)
	{
		VM_Warning(prog, "SV_StartPointSound: volume must be in range 0-1\n");
		return;
	}

	if (attenuation < 0 || attenuation > 4)
	{
		VM_Warning(prog, "SV_StartPointSound: attenuation must be in range 0-4\n");
		return;
	}

	SV_StartPointSound (org, sample, nvolume, attenuation, pitchchange);
}

/*
=================
VM_SV_traceline

Used for use tracing and shot targeting
Traces are blocked by bbox and exact bsp entityes, and also slide box entities
if the tryents flag is set.

traceline (vector1, vector2, movetype, ignore)
=================
*/
static void VM_SV_traceline(prvm_prog_t *prog)
{
	vec3_t	v1, v2;
	trace_t	trace;
	int		move;
	prvm_edict_t	*ent;

	VM_SAFEPARMCOUNTRANGE(4, 8, VM_SV_traceline); // allow more parameters for future expansion

	prog->xfunction->builtinsprofile += 30;

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), v1);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), v2);
	move = (int)PRVM_G_FLOAT(OFS_PARM2);
	ent = PRVM_G_EDICT(OFS_PARM3);

	if (isnan(v1[0]) || isnan(v1[1]) || isnan(v1[2]) || isnan(v2[0]) || isnan(v2[1]) || isnan(v2[2]))
		prog->error_cmd("%s: NAN errors detected in traceline('%f %f %f', '%f %f %f', %i, entity %i)\n", prog->name, v1[0], v1[1], v1[2], v2[0], v2[1], v2[2], move, PRVM_EDICT_TO_PROG(ent));

	trace = SV_TraceLine(v1, v2, move, ent, SV_GenericHitSuperContentsMask(ent), 0, 0, collision_extendtracelinelength.value);

	VM_SetTraceGlobals(prog, &trace);
}


/*
=================
VM_SV_tracebox

Used for use tracing and shot targeting
Traces are blocked by bbox and exact bsp entityes, and also slide box entities
if the tryents flag is set.

tracebox (vector1, vector mins, vector maxs, vector2, tryents)
=================
*/
// LadyHavoc: added this for my own use, VERY useful, similar to traceline
static void VM_SV_tracebox(prvm_prog_t *prog)
{
	vec3_t v1, v2, m1, m2;
	trace_t	trace;
	int		move;
	prvm_edict_t	*ent;

	VM_SAFEPARMCOUNTRANGE(6, 8, VM_SV_tracebox); // allow more parameters for future expansion

	prog->xfunction->builtinsprofile += 30;

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), v1);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), m1);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), m2);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM3), v2);
	move = (int)PRVM_G_FLOAT(OFS_PARM4);
	ent = PRVM_G_EDICT(OFS_PARM5);

	if (isnan(v1[0]) || isnan(v1[1]) || isnan(v1[2]) || isnan(v2[0]) || isnan(v2[1]) || isnan(v2[2]))
		prog->error_cmd("%s: NAN errors detected in tracebox('%f %f %f', '%f %f %f', '%f %f %f', '%f %f %f', %i, entity %i)\n", prog->name, v1[0], v1[1], v1[2], m1[0], m1[1], m1[2], m2[0], m2[1], m2[2], v2[0], v2[1], v2[2], move, PRVM_EDICT_TO_PROG(ent));

	trace = SV_TraceBox(v1, m1, m2, v2, move, ent, SV_GenericHitSuperContentsMask(ent), 0, 0, collision_extendtraceboxlength.value);

	VM_SetTraceGlobals(prog, &trace);
}

static trace_t SV_Trace_Toss(prvm_prog_t *prog, prvm_edict_t *tossent, prvm_edict_t *ignore)
{
	int i;
	float gravity;
	vec3_t move, end, tossentorigin, tossentmins, tossentmaxs;
	vec3_t original_origin;
	vec3_t original_velocity;
	vec3_t original_angles;
	vec3_t original_avelocity;
	trace_t trace;

	VectorCopy(PRVM_serveredictvector(tossent, origin)   , original_origin   );
	VectorCopy(PRVM_serveredictvector(tossent, velocity) , original_velocity );
	VectorCopy(PRVM_serveredictvector(tossent, angles)   , original_angles   );
	VectorCopy(PRVM_serveredictvector(tossent, avelocity), original_avelocity);

	gravity = PRVM_serveredictfloat(tossent, gravity);
	if (!gravity)
		gravity = 1.0f;
	gravity *= sv_gravity.value * 0.025;

	for (i = 0;i < 200;i++) // LadyHavoc: sanity check; never trace more than 10 seconds
	{
		SV_CheckVelocity (tossent);
		PRVM_serveredictvector(tossent, velocity)[2] -= gravity;
		VectorMA (PRVM_serveredictvector(tossent, angles), 0.05, PRVM_serveredictvector(tossent, avelocity), PRVM_serveredictvector(tossent, angles));
		VectorScale (PRVM_serveredictvector(tossent, velocity), 0.05, move);
		VectorAdd (PRVM_serveredictvector(tossent, origin), move, end);
		VectorCopy(PRVM_serveredictvector(tossent, origin), tossentorigin);
		VectorCopy(PRVM_serveredictvector(tossent, mins), tossentmins);
		VectorCopy(PRVM_serveredictvector(tossent, maxs), tossentmaxs);
		trace = SV_TraceBox(tossentorigin, tossentmins, tossentmaxs, end, MOVE_NORMAL, tossent, SV_GenericHitSuperContentsMask(tossent), 0, 0, collision_extendmovelength.value);
		VectorCopy (trace.endpos, PRVM_serveredictvector(tossent, origin));
		PRVM_serveredictvector(tossent, velocity)[2] -= gravity;

		if (trace.fraction < 1)
			break;
	}

	VectorCopy(original_origin   , PRVM_serveredictvector(tossent, origin)   );
	VectorCopy(original_velocity , PRVM_serveredictvector(tossent, velocity) );
	VectorCopy(original_angles   , PRVM_serveredictvector(tossent, angles)   );
	VectorCopy(original_avelocity, PRVM_serveredictvector(tossent, avelocity));

	return trace;
}

static void VM_SV_tracetoss(prvm_prog_t *prog)
{
	trace_t	trace;
	prvm_edict_t	*ent;
	prvm_edict_t	*ignore;

	VM_SAFEPARMCOUNT(2, VM_SV_tracetoss);

	prog->xfunction->builtinsprofile += 600;

	ent = PRVM_G_EDICT(OFS_PARM0);
	if (ent == prog->edicts)
	{
		VM_Warning(prog, "tracetoss: can not use world entity\n");
		return;
	}
	ignore = PRVM_G_EDICT(OFS_PARM1);

	trace = SV_Trace_Toss(prog, ent, ignore);

	VM_SetTraceGlobals(prog, &trace);
}

//============================================================================

static unsigned char *checkpvs;

static int VM_SV_newcheckclient(prvm_prog_t *prog, int check)
{
	int		i;
	prvm_edict_t	*ent;
	vec3_t	org;

// cycle to the next one

	check = bound(1, check, svs.maxclients);
	if (check == svs.maxclients)
		i = 1;
	else
		i = check + 1;

	for ( ;  ; i++)
	{
		// count the cost
		prog->xfunction->builtinsprofile++;
		// wrap around
		if (i == svs.maxclients+1)
			i = 1;
		// look up the client's edict
		ent = PRVM_EDICT_NUM(i);
		// check if it is to be ignored, but never ignore the one we started on (prevent infinite loop)
		if (i != check && (ent->free || PRVM_serveredictfloat(ent, health) <= 0 || ((int)PRVM_serveredictfloat(ent, flags) & FL_NOTARGET)))
			continue;
		// found a valid client (possibly the same one again)
		break;
	}

// get the PVS for the entity
	VectorAdd(PRVM_serveredictvector(ent, origin), PRVM_serveredictvector(ent, view_ofs), org);
	if (sv.worldmodel && sv.worldmodel->brush.FatPVS)
		sv.worldmodel->brush.FatPVS(sv.worldmodel, org, 0, &checkpvs, sv_mempool, false);
	else
		checkpvs = NULL;

	return i;
}

/*
=================
VM_SV_checkclient

Returns a client (or object that has a client enemy) that would be a
valid target.

If there is more than one valid option, they are cycled each frame

If (self.origin + self.viewofs) is not in the PVS of the current target,
it is not returned at all.

name checkclient ()
=================
*/
int c_invis, c_notvis;
static void VM_SV_checkclient(prvm_prog_t *prog)
{
	prvm_edict_t	*ent, *self;
	vec3_t	view;

	VM_SAFEPARMCOUNT(0, VM_SV_checkclient);

	// find a new check if on a new frame
	if (sv.time - sv.lastchecktime >= 0.1)
	{
		sv.lastcheck = VM_SV_newcheckclient(prog, sv.lastcheck);
		sv.lastchecktime = sv.time;
	}

	// return check if it might be visible
	ent = PRVM_EDICT_NUM(sv.lastcheck);
	if (ent->free || PRVM_serveredictfloat(ent, health) <= 0)
	{
		VM_RETURN_EDICT(prog->edicts);
		return;
	}

	// if current entity can't possibly see the check entity, return 0
	self = PRVM_PROG_TO_EDICT(PRVM_serverglobaledict(self));
	VectorAdd(PRVM_serveredictvector(self, origin), PRVM_serveredictvector(self, view_ofs), view);
	if (sv.worldmodel && checkpvs && !sv.worldmodel->brush.BoxTouchingPVS(sv.worldmodel, checkpvs, view, view))
	{
		c_notvis++;
		VM_RETURN_EDICT(prog->edicts);
		return;
	}

	// might be able to see it
	c_invis++;
	VM_RETURN_EDICT(ent);
}

//============================================================================

/*
=================
VM_SV_checkpvs

Checks if an entity is in a point's PVS.
Should be fast but can be inexact.

float checkpvs(vector viewpos, entity viewee) = #240;
=================
*/
static void VM_SV_checkpvs(prvm_prog_t *prog)
{
	vec3_t viewpos, absmin, absmax;
	prvm_edict_t *viewee;
#if 1
	unsigned char *pvs;
#else
	unsigned char *fatpvs = NULL;
#endif

	VM_SAFEPARMCOUNT(2, VM_SV_checkpvs);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), viewpos);
	viewee = PRVM_G_EDICT(OFS_PARM1);

	if(viewee->free)
	{
		VM_Warning(prog, "checkpvs: can not check free entity\n");
		PRVM_G_FLOAT(OFS_RETURN) = 4;
		return;
	}

#if 1
	if(!sv.worldmodel || !sv.worldmodel->brush.GetPVS || !sv.worldmodel->brush.BoxTouchingPVS)
	{
		// no PVS support on this worldmodel... darn
		PRVM_G_FLOAT(OFS_RETURN) = 3;
		return;
	}
	pvs = sv.worldmodel->brush.GetPVS(sv.worldmodel, viewpos);
	if(!pvs)
	{
		// viewpos isn't in any PVS... darn
		PRVM_G_FLOAT(OFS_RETURN) = 2;
		return;
	}
	VectorCopy(PRVM_serveredictvector(viewee, absmin), absmin);
	VectorCopy(PRVM_serveredictvector(viewee, absmax), absmax);
	PRVM_G_FLOAT(OFS_RETURN) = sv.worldmodel->brush.BoxTouchingPVS(sv.worldmodel, pvs, absmin, absmax);
#else
	// using fat PVS like FTEQW does (slow)
	if(!sv.worldmodel || !sv.worldmodel->brush.FatPVS || !sv.worldmodel->brush.BoxTouchingPVS)
	{
		// no PVS support on this worldmodel... darn
		PRVM_G_FLOAT(OFS_RETURN) = 3;
		return;
	}
	sv.worldmodel->brush.FatPVS(sv.worldmodel, viewpos, 8, &fatpvs, sv_mempool, false);
	if(!fatpvs)
	{
		// viewpos isn't in any PVS... darn
		PRVM_G_FLOAT(OFS_RETURN) = 2;
		return;
	}
	VectorCopy(PRVM_serveredictvector(viewee, absmin), absmin);
	VectorCopy(PRVM_serveredictvector(viewee, absmax), absmax);
	PRVM_G_FLOAT(OFS_RETURN) = sv.worldmodel->brush.BoxTouchingPVS(sv.worldmodel, fatpvs, absmin, absmax);
#endif
}


/*
=================
VM_SV_stuffcmd

Sends text over to the client's execution buffer

stuffcmd (clientent, value, ...)
=================
*/
static void VM_SV_stuffcmd(prvm_prog_t *prog)
{
	int		entnum;
	client_t	*old;
	char	string[VM_TEMPSTRING_MAXSIZE];

	VM_SAFEPARMCOUNTRANGE(2, 8, VM_SV_stuffcmd);

	entnum = PRVM_G_EDICTNUM(OFS_PARM0);
	if (entnum < 1 || entnum > svs.maxclients || !svs.clients[entnum-1].active)
	{
		VM_Warning(prog, "Can't stuffcmd to a non-client\n");
		return;
	}

	VM_VarString(prog, 1, string, sizeof(string));

	old = host_client;
	host_client = svs.clients + entnum-1;
	SV_ClientCommands ("%s", string);
	host_client = old;
}

/*
=================
VM_SV_findradius

Returns a chain of entities that have origins within a spherical area

findradius (origin, radius)
=================
*/
static void VM_SV_findradius(prvm_prog_t *prog)
{
	prvm_edict_t *ent, *chain;
	vec_t radius, radius2;
	vec3_t org, eorg, mins, maxs;
	int i;
	int numtouchedicts;
	static prvm_edict_t *touchedicts[MAX_EDICTS];
	int chainfield;

	VM_SAFEPARMCOUNTRANGE(2, 3, VM_SV_findradius);

	if(prog->argc == 3)
		chainfield = PRVM_G_INT(OFS_PARM2);
	else
		chainfield = prog->fieldoffsets.chain;
	if (chainfield < 0)
		prog->error_cmd("VM_SV_findradius: %s doesnt have the specified chain field !", prog->name);

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
	numtouchedicts = SV_EntitiesInBox(mins, maxs, MAX_EDICTS, touchedicts);
	if (numtouchedicts > MAX_EDICTS)
	{
		// this never happens
		Con_Printf("SV_EntitiesInBox returned %i edicts, max was %i\n", numtouchedicts, MAX_EDICTS);
		numtouchedicts = MAX_EDICTS;
	}
	for (i = 0;i < numtouchedicts;i++)
	{
		ent = touchedicts[i];
		prog->xfunction->builtinsprofile++;
		// Quake did not return non-solid entities but darkplaces does
		// (note: this is the reason you can't blow up fallen zombies)
		if (PRVM_serveredictfloat(ent, solid) == SOLID_NOT && !sv_gameplayfix_blowupfallenzombies.integer)
			continue;
		// LadyHavoc: compare against bounding box rather than center so it
		// doesn't miss large objects, and use DotProduct instead of Length
		// for a major speedup
		VectorSubtract(org, PRVM_serveredictvector(ent, origin), eorg);
		if (sv_gameplayfix_findradiusdistancetobox.integer)
		{
			eorg[0] -= bound(PRVM_serveredictvector(ent, mins)[0], eorg[0], PRVM_serveredictvector(ent, maxs)[0]);
			eorg[1] -= bound(PRVM_serveredictvector(ent, mins)[1], eorg[1], PRVM_serveredictvector(ent, maxs)[1]);
			eorg[2] -= bound(PRVM_serveredictvector(ent, mins)[2], eorg[2], PRVM_serveredictvector(ent, maxs)[2]);
		}
		else
			VectorMAMAM(1, eorg, -0.5f, PRVM_serveredictvector(ent, mins), -0.5f, PRVM_serveredictvector(ent, maxs), eorg);
		if (DotProduct(eorg, eorg) < radius2)
		{
			PRVM_EDICTFIELDEDICT(ent,chainfield) = PRVM_EDICT_TO_PROG(chain);
			chain = ent;
		}
	}

	VM_RETURN_EDICT(chain);
}

/*
=================
VM_SV_findbox

Returns a chain of entities that are touching a box (a simpler findradius); supports DP_QC_FINDCHAIN_TOFIELD

findbox (mins, maxs)
=================
*/
static void VM_SV_findbox(prvm_prog_t *prog)
{
	prvm_edict_t *chain;
	int i, numtouchedicts;
	static prvm_edict_t *touchedicts[MAX_EDICTS];
	int chainfield;

	VM_SAFEPARMCOUNTRANGE(2, 3, VM_SV_findbox);

	if(prog->argc == 3)
		chainfield = PRVM_G_INT(OFS_PARM2);
	else
		chainfield = prog->fieldoffsets.chain;
	if (chainfield < 0)
		prog->error_cmd("VM_SV_findbox: %s doesnt have the specified chain field !", prog->name);

	chain = (prvm_edict_t *)prog->edicts;

	numtouchedicts = SV_EntitiesInBox(PRVM_G_VECTOR(OFS_PARM0), PRVM_G_VECTOR(OFS_PARM1), MAX_EDICTS, touchedicts);
	if (numtouchedicts > MAX_EDICTS)
	{
		// this never happens
		Con_Printf("SV_EntitiesInBox returned %i edicts, max was %i\n", numtouchedicts, MAX_EDICTS);
		numtouchedicts = MAX_EDICTS;
	}
	for (i = 0; i < numtouchedicts; ++i)
	{
		prog->xfunction->builtinsprofile++;
		PRVM_EDICTFIELDEDICT(touchedicts[i], chainfield) = PRVM_EDICT_TO_PROG(chain);
		chain = touchedicts[i];
	}

	VM_RETURN_EDICT(chain);
}

static void VM_SV_precache_sound(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(1, VM_SV_precache_sound);
	PRVM_G_FLOAT(OFS_RETURN) = SV_SoundIndex(PRVM_G_STRING(OFS_PARM0), 2);
}

static void VM_SV_precache_model(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(1, VM_SV_precache_model);
	SV_ModelIndex(PRVM_G_STRING(OFS_PARM0), 2);
	PRVM_G_INT(OFS_RETURN) = PRVM_G_INT(OFS_PARM0);
}

/*
===============
VM_SV_walkmove

float(float yaw, float dist[, settrace]) walkmove
===============
*/
static void VM_SV_walkmove(prvm_prog_t *prog)
{
	prvm_edict_t	*ent;
	float	yaw, dist;
	vec3_t	move;
	mfunction_t	*oldf;
	int 	oldself;
	qbool	settrace;

	VM_SAFEPARMCOUNTRANGE(2, 3, VM_SV_walkmove);

	// assume failure if it returns early
	PRVM_G_FLOAT(OFS_RETURN) = 0;

	ent = PRVM_PROG_TO_EDICT(PRVM_serverglobaledict(self));
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

	if ( !( (int)PRVM_serveredictfloat(ent, flags) & (FL_ONGROUND|FL_FLY|FL_SWIM) ) )
		return;

	yaw = yaw*M_PI*2 / 360;

	move[0] = cos(yaw)*dist;
	move[1] = sin(yaw)*dist;
	move[2] = 0;

// save program state, because SV_movestep may call other progs
	oldf = prog->xfunction;
	oldself = PRVM_serverglobaledict(self);

	PRVM_G_FLOAT(OFS_RETURN) = SV_movestep(ent, move, true, false, settrace);


// restore program state
	prog->xfunction = oldf;
	PRVM_serverglobaledict(self) = oldself;
}

/*
===============
VM_SV_droptofloor

void() droptofloor
===============
*/
inline static qbool droptofloor_bsp_failcond(trace_t *trace)
{
	if (sv.worldmodel->brush.isq3bsp || sv.worldmodel->brush.isq2bsp)
		return trace->startsolid;
	else
		return trace->allsolid || trace->fraction == 1;
}
static void VM_SV_droptofloor(prvm_prog_t *prog)
{
	prvm_edict_t *ent;
	vec3_t        end;
	trace_t       trace;

	VM_SAFEPARMCOUNTRANGE(0, 2, VM_SV_droptofloor); // allow 2 parameters because the id1 defs.qc had an incorrect prototype

	// assume failure if it returns early
	PRVM_G_FLOAT(OFS_RETURN) = 0;

	ent = PRVM_PROG_TO_EDICT(PRVM_serverglobaledict(self));
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

	if (sv_gameplayfix_droptofloorstartsolid_nudgetocorrect.integer)
	{
		int n = PHYS_NudgeOutOfSolid(prog, ent);
		if (!n)
			VM_Warning(prog, "droptofloor at \"%f %f %f\": sv_gameplayfix_droptofloorstartsolid_nudgetocorrect COULD NOT FIX badly placed entity \"%s\" before drop\n", PRVM_gameedictvector(ent, origin)[0], PRVM_gameedictvector(ent, origin)[1], PRVM_gameedictvector(ent, origin)[2], PRVM_GetString(prog, PRVM_gameedictstring(ent, classname)));
		else if (n > 0)
			VM_Warning(prog, "droptofloor at \"%f %f %f\": sv_gameplayfix_droptofloorstartsolid_nudgetocorrect FIXED badly placed entity \"%s\" before drop\n", PRVM_gameedictvector(ent, origin)[0], PRVM_gameedictvector(ent, origin)[1], PRVM_gameedictvector(ent, origin)[2], PRVM_GetString(prog, PRVM_gameedictstring(ent, classname)));
	}

	VectorCopy (PRVM_serveredictvector(ent, origin), end);
	if (sv.worldmodel->brush.isq3bsp)
		end[2] -= 4096;
	else if (sv.worldmodel->brush.isq2bsp)
		end[2] -= 128;
	else
		end[2] -= 256; // Quake, QuakeWorld

	/* bones_was_here: not using SV_GenericHitSuperContentsMask(ent) anymore because it was setting:
	 * items:    SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY
	 * monsters: SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY | SUPERCONTENTS_PLAYERCLIP
	 * explobox: SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY | SUPERCONTENTS_CORPSE
	 * which caused (startsolid == true) when, for example, a health was touching a monster.
	 * Changing MOVE_NORMAL also fixes that, but other engines are using MOVE_NORMAL here.
	 */
	trace = SV_TraceBox(PRVM_serveredictvector(ent, origin), PRVM_serveredictvector(ent, mins), PRVM_serveredictvector(ent, maxs), end, MOVE_NORMAL, ent, SUPERCONTENTS_SOLID, 0, 0, collision_extendmovelength.value);
	if (droptofloor_bsp_failcond(&trace))
	{
		if (sv_gameplayfix_droptofloorstartsolid.integer)
		{
			vec3_t offset, org;

			offset[0] = 0.5f * (PRVM_serveredictvector(ent, mins)[0] + PRVM_serveredictvector(ent, maxs)[0]);
			offset[1] = 0.5f * (PRVM_serveredictvector(ent, mins)[1] + PRVM_serveredictvector(ent, maxs)[1]);
			offset[2] = PRVM_serveredictvector(ent, mins)[2];
			VectorAdd(PRVM_serveredictvector(ent, origin), offset, org);
			VectorAdd(end, offset, end);

			trace = SV_TraceLine(org, end, MOVE_NORMAL, ent, SUPERCONTENTS_SOLID, 0, 0, collision_extendmovelength.value);
			if (droptofloor_bsp_failcond(&trace))
			{
				VM_Warning(prog, "droptofloor at \"%f %f %f\": sv_gameplayfix_droptofloorstartsolid COULD NOT FIX badly placed entity \"%s\"\n", PRVM_serveredictvector(ent, origin)[0], PRVM_serveredictvector(ent, origin)[1], PRVM_serveredictvector(ent, origin)[2], PRVM_GetString(prog, PRVM_gameedictstring(ent, classname)));
				return;
			}
			VM_Warning(prog, "droptofloor at \"%f %f %f\": sv_gameplayfix_droptofloorstartsolid FIXED badly placed entity \"%s\"\n", PRVM_serveredictvector(ent, origin)[0], PRVM_serveredictvector(ent, origin)[1], PRVM_serveredictvector(ent, origin)[2], PRVM_GetString(prog, PRVM_gameedictstring(ent, classname)));
			VectorSubtract(trace.endpos, offset, PRVM_serveredictvector(ent, origin));

			// only because we dropped it without considering its bbox
			if (sv_gameplayfix_droptofloorstartsolid_nudgetocorrect.integer)
				PHYS_NudgeOutOfSolid(prog, ent);
		}
		else
		{
			VM_Warning(prog, "droptofloor at \"%f %f %f\": badly placed entity \"%s\", startsolid: %d allsolid: %d\n", PRVM_serveredictvector(ent, origin)[0], PRVM_serveredictvector(ent, origin)[1], PRVM_serveredictvector(ent, origin)[2], PRVM_GetString(prog, PRVM_gameedictstring(ent, classname)), trace.startsolid, trace.allsolid);
			return;
		}
	}
	else
		VectorCopy(trace.endpos, PRVM_serveredictvector(ent, origin));

	SV_LinkEdict(ent);
	PRVM_serveredictfloat(ent, flags) = (int)PRVM_serveredictfloat(ent, flags) | FL_ONGROUND;
	PRVM_serveredictedict(ent, groundentity) = PRVM_EDICT_TO_PROG(trace.ent);
	PRVM_G_FLOAT(OFS_RETURN) = 1;
	// if support is destroyed, keep suspended (gross hack for floating items in various maps)
	ent->priv.server->suspendedinairflag = true;
}

/*
===============
VM_SV_lightstyle

void(float style, string value) lightstyle
===============
*/
static void VM_SV_lightstyle(prvm_prog_t *prog)
{
	int		style;
	const char	*val;
	client_t	*client;
	int			j;

	VM_SAFEPARMCOUNT(2, VM_SV_lightstyle);

	style = (int)PRVM_G_FLOAT(OFS_PARM0);
	val = PRVM_G_STRING(OFS_PARM1);

	if( (unsigned) style >= MAX_LIGHTSTYLES ) {
		prog->error_cmd( "PF_lightstyle: style: %i >= 64", style );
	}

// change the string in sv
	dp_strlcpy(sv.lightstyles[style], val, sizeof(sv.lightstyles[style]));

// send message to all clients on this server
	if (sv.state != ss_active)
		return;

	for (j = 0, client = svs.clients;j < svs.maxclients;j++, client++)
	{
		if (client->active && client->netconnection)
		{
			MSG_WriteChar (&client->netconnection->message, svc_lightstyle);
			MSG_WriteChar (&client->netconnection->message,style);
			MSG_WriteString (&client->netconnection->message, val);
		}
	}
}

/*
=============
VM_SV_checkbottom
=============
*/
static void VM_SV_checkbottom(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(1, VM_SV_checkbottom);
	PRVM_G_FLOAT(OFS_RETURN) = SV_CheckBottom (PRVM_G_EDICT(OFS_PARM0));
}

/*
=============
VM_SV_pointcontents
=============
*/
static void VM_SV_pointcontents(prvm_prog_t *prog)
{
	vec3_t point;
	VM_SAFEPARMCOUNT(1, VM_SV_pointcontents);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), point);
	PRVM_G_FLOAT(OFS_RETURN) = Mod_Q1BSP_NativeContentsFromSuperContents(SV_PointSuperContents(point));
}

/*
=============
VM_SV_aim

Pick a vector for the player to shoot along
vector aim(entity, missilespeed)
=============
*/
static void VM_SV_aim(prvm_prog_t *prog)
{
	prvm_edict_t	*ent, *check, *bestent;
	vec3_t	start, dir, end, bestdir;
	int		i, j;
	trace_t	tr;
	float	dist, bestdist;
	//float	speed;

	VM_SAFEPARMCOUNT(2, VM_SV_aim);

	// assume failure if it returns early
	VectorCopy(PRVM_serverglobalvector(v_forward), PRVM_G_VECTOR(OFS_RETURN));
	// if sv_aim is so high it can't possibly accept anything, skip out early
	if (sv_aim.value >= 1)
		return;

	ent = PRVM_G_EDICT(OFS_PARM0);
	if (ent == prog->edicts)
	{
		VM_Warning(prog, "aim: can not use world entity\n");
		return;
	}
	if (ent->free)
	{
		VM_Warning(prog, "aim: can not use free entity\n");
		return;
	}
	//speed = PRVM_G_FLOAT(OFS_PARM1);

	VectorCopy (PRVM_serveredictvector(ent, origin), start);
	start[2] += 20;

// try sending a trace straight
	VectorCopy (PRVM_serverglobalvector(v_forward), dir);
	VectorMA (start, 2048, dir, end);
	tr = SV_TraceLine(start, end, MOVE_NORMAL, ent, SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY, 0, 0, collision_extendmovelength.value);
	if (tr.ent && PRVM_serveredictfloat(((prvm_edict_t *)tr.ent), takedamage) == DAMAGE_AIM
	&& (!teamplay.integer || PRVM_serveredictfloat(ent, team) <=0 || PRVM_serveredictfloat(ent, team) != PRVM_serveredictfloat(((prvm_edict_t *)tr.ent), team)) )
	{
		VectorCopy (PRVM_serverglobalvector(v_forward), PRVM_G_VECTOR(OFS_RETURN));
		return;
	}


// try all possible entities
	VectorCopy (dir, bestdir);
	bestdist = sv_aim.value;
	bestent = NULL;

	check = PRVM_NEXT_EDICT(prog->edicts);
	for (i=1 ; i<prog->num_edicts ; i++, check = PRVM_NEXT_EDICT(check) )
	{
		prog->xfunction->builtinsprofile++;
		if (PRVM_serveredictfloat(check, takedamage) != DAMAGE_AIM)
			continue;
		if (check == ent)
			continue;
		if (teamplay.integer && PRVM_serveredictfloat(ent, team) > 0 && PRVM_serveredictfloat(ent, team) == PRVM_serveredictfloat(check, team))
			continue;	// don't aim at teammate
		for (j=0 ; j<3 ; j++)
			end[j] = PRVM_serveredictvector(check, origin)[j]
			+ 0.5*(PRVM_serveredictvector(check, mins)[j] + PRVM_serveredictvector(check, maxs)[j]);
		VectorSubtract (end, start, dir);
		VectorNormalize (dir);
		dist = DotProduct (dir, PRVM_serverglobalvector(v_forward));
		if (dist < bestdist)
			continue;	// to far to turn
		tr = SV_TraceLine(start, end, MOVE_NORMAL, ent, SUPERCONTENTS_SOLID | SUPERCONTENTS_BODY, 0, 0, collision_extendmovelength.value);
		if (tr.ent == check)
		{	// can shoot at this one
			bestdist = dist;
			bestent = check;
		}
	}

	if (bestent)
	{
		VectorSubtract (PRVM_serveredictvector(bestent, origin), PRVM_serveredictvector(ent, origin), dir);
		dist = DotProduct (dir, PRVM_serverglobalvector(v_forward));
		VectorScale (PRVM_serverglobalvector(v_forward), dist, end);
		end[2] = dir[2];
		VectorNormalize (end);
		VectorCopy (end, PRVM_G_VECTOR(OFS_RETURN));
	}
	else
	{
		VectorCopy (bestdir, PRVM_G_VECTOR(OFS_RETURN));
	}
}

/*
===============================================================================

MESSAGE WRITING

===============================================================================
*/

#define	MSG_BROADCAST	0		// unreliable to all
#define	MSG_ONE			1		// reliable to one (msg_entity)
#define	MSG_ALL			2		// reliable to all
#define	MSG_INIT		3		// write to the init string
#define	MSG_ENTITY		5

static sizebuf_t *WriteDest(prvm_prog_t *prog)
{
	int		entnum;
	int		dest;
	prvm_edict_t	*ent;

	dest = (int)PRVM_G_FLOAT(OFS_PARM0);
	switch (dest)
	{
	case MSG_BROADCAST:
		return &sv.datagram;

	case MSG_ONE:
		ent = PRVM_PROG_TO_EDICT(PRVM_serverglobaledict(msg_entity));
		entnum = PRVM_NUM_FOR_EDICT(ent);
		if (entnum < 1 || entnum > svs.maxclients)
		{
			VM_Warning(prog, "WriteDest: tried to write to non-client\n");
			return &sv.reliable_datagram;
		}
		else if (!svs.clients[entnum-1].active)
		{
			VM_Warning(prog, "WriteDest: tried to write to a disconnected client\n");
			return &sv.reliable_datagram;
		}
		else if (!svs.clients[entnum-1].netconnection)
		{
			VM_Warning(prog, "WriteDest: tried to write to a bot client\n");
			return &sv.reliable_datagram;
		}
		else
			return &svs.clients[entnum-1].netconnection->message;

	default:
		VM_Warning(prog, "WriteDest: bad destination\n");
	case MSG_ALL:
		return &sv.reliable_datagram;

	case MSG_INIT:
		return &sv.signon;

	case MSG_ENTITY:
		return sv.writeentitiestoclient_msg;
	}

	//return NULL;
}

static void VM_SV_WriteByte(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(2, VM_SV_WriteByte);
	MSG_WriteByte (WriteDest(prog), (int)PRVM_G_FLOAT(OFS_PARM1));
}

static void VM_SV_WriteChar(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(2, VM_SV_WriteChar);
	MSG_WriteChar (WriteDest(prog), (int)PRVM_G_FLOAT(OFS_PARM1));
}

static void VM_SV_WriteShort(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(2, VM_SV_WriteShort);
	MSG_WriteShort (WriteDest(prog), (int)PRVM_G_FLOAT(OFS_PARM1));
}

static void VM_SV_WriteLong(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(2, VM_SV_WriteLong);
	MSG_WriteLong (WriteDest(prog), (int)PRVM_G_FLOAT(OFS_PARM1));
}

static void VM_SV_WriteAngle(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(2, VM_SV_WriteAngle);
	MSG_WriteAngle (WriteDest(prog), PRVM_G_FLOAT(OFS_PARM1), sv.protocol);
}

static void VM_SV_WriteCoord(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(2, VM_SV_WriteCoord);
	MSG_WriteCoord (WriteDest(prog), PRVM_G_FLOAT(OFS_PARM1), sv.protocol);
}

static void VM_SV_WriteString(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(2, VM_SV_WriteString);
	MSG_WriteString (WriteDest(prog), PRVM_G_STRING(OFS_PARM1));
}

static void VM_SV_WriteUnterminatedString(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(2, VM_SV_WriteUnterminatedString);
	MSG_WriteUnterminatedString (WriteDest(prog), PRVM_G_STRING(OFS_PARM1));
}


static void VM_SV_WriteEntity(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(2, VM_SV_WriteEntity);
	MSG_WriteShort (WriteDest(prog), PRVM_G_EDICTNUM(OFS_PARM1));
}

// writes a picture as at most size bytes of data
// message:
//   IMGNAME \0 SIZE(short) IMGDATA
// if failed to read/compress:
//   IMGNAME \0 \0 \0
//#501 void(float dest, string name, float maxsize) WritePicture (DP_SV_WRITEPICTURE))
static void VM_SV_WritePicture(prvm_prog_t *prog)
{
	const char *imgname;
	void *buf;
	size_t size;

	VM_SAFEPARMCOUNT(3, VM_SV_WritePicture);

	imgname = PRVM_G_STRING(OFS_PARM1);
	size = (size_t) PRVM_G_FLOAT(OFS_PARM2);
	if(size > 65535)
		size = 65535;

	MSG_WriteString(WriteDest(prog), imgname);
	if(Image_Compress(imgname, size, &buf, &size))
	{
		// actual picture
		MSG_WriteShort(WriteDest(prog), (int)size);
		SZ_Write(WriteDest(prog), (unsigned char *) buf, (int)size);
	}
	else
	{
		// placeholder
		MSG_WriteShort(WriteDest(prog), 0);
	}
}

//////////////////////////////////////////////////////////

static void VM_SV_makestatic(prvm_prog_t *prog)
{
	prvm_edict_t *ent;
	int i, large;

	// allow 0 parameters due to an id1 qc bug in which this function is used
	// with no parameters (but directly after setmodel with self in OFS_PARM0)
	VM_SAFEPARMCOUNTRANGE(0, 1, VM_SV_makestatic);

	if (prog->argc >= 1)
		ent = PRVM_G_EDICT(OFS_PARM0);
	else
		ent = PRVM_PROG_TO_EDICT(PRVM_serverglobaledict(self));
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

	large = false;
	if (PRVM_serveredictfloat(ent, modelindex) >= 256 || PRVM_serveredictfloat(ent, frame) >= 256)
		large = true;

	if (sv.protocol == PROTOCOL_NEHAHRABJP || sv.protocol == PROTOCOL_NEHAHRABJP2 || sv.protocol == PROTOCOL_NEHAHRABJP3)
	{
		MSG_WriteByte (&sv.signon,svc_spawnstatic);
		MSG_WriteShort (&sv.signon, (int)PRVM_serveredictfloat(ent, modelindex));
		MSG_WriteByte (&sv.signon, (int)PRVM_serveredictfloat(ent, frame));
	}
	else if (large)
	{
		MSG_WriteByte (&sv.signon,svc_spawnstatic2);
		MSG_WriteShort (&sv.signon, (int)PRVM_serveredictfloat(ent, modelindex));
		MSG_WriteShort (&sv.signon, (int)PRVM_serveredictfloat(ent, frame));
	}
	else
	{
		MSG_WriteByte (&sv.signon,svc_spawnstatic);
		MSG_WriteByte (&sv.signon, (int)PRVM_serveredictfloat(ent, modelindex));
		MSG_WriteByte (&sv.signon, (int)PRVM_serveredictfloat(ent, frame));
	}

	MSG_WriteByte (&sv.signon, (int)PRVM_serveredictfloat(ent, colormap));
	MSG_WriteByte (&sv.signon, (int)PRVM_serveredictfloat(ent, skin));
	for (i=0 ; i<3 ; i++)
	{
		MSG_WriteCoord(&sv.signon, PRVM_serveredictvector(ent, origin)[i], sv.protocol);
		MSG_WriteAngle(&sv.signon, PRVM_serveredictvector(ent, angles)[i], sv.protocol);
	}

// throw the entity away now
	PRVM_ED_Free(prog, ent);
}

//=============================================================================

/*
==============
VM_SV_setspawnparms
==============
*/
static void VM_SV_setspawnparms(prvm_prog_t *prog)
{
	prvm_edict_t	*ent;
	int		i;
	client_t	*client;

	VM_SAFEPARMCOUNT(1, VM_SV_setspawnparms);

	ent = PRVM_G_EDICT(OFS_PARM0);
	i = PRVM_NUM_FOR_EDICT(ent);
	if (i < 1 || i > svs.maxclients || !svs.clients[i-1].active)
	{
		Con_Print("tried to setspawnparms on a non-client\n");
		return;
	}

	// copy spawn parms out of the client_t
	client = svs.clients + i-1;
	for (i=0 ; i< NUM_SPAWN_PARMS ; i++)
		(&PRVM_serverglobalfloat(parm1))[i] = client->spawn_parms[i];
}

/*
=================
VM_SV_getlight

Returns a color vector indicating the lighting at the requested point.

(Internal Operation note: actually measures the light beneath the point, just like
                          the model lighting on the client)

getlight(vector)
=================
*/
static void VM_SV_getlight(prvm_prog_t *prog)
{
	vec3_t ambientcolor, diffusecolor, diffusenormal;
	vec3_t p;
	VM_SAFEPARMCOUNT(1, VM_SV_getlight);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), p);
	VectorClear(ambientcolor);
	VectorClear(diffusecolor);
	VectorClear(diffusenormal);
	if (sv.worldmodel && sv.worldmodel->brush.LightPoint)
		sv.worldmodel->brush.LightPoint(sv.worldmodel, p, ambientcolor, diffusecolor, diffusenormal);
	VectorMA(ambientcolor, 0.5, diffusecolor, PRVM_G_VECTOR(OFS_RETURN));
}

typedef struct
{
	unsigned char	type;	// 1/2/8 or 0 to indicate unused
	int		fieldoffset;
}customstat_t;

static customstat_t vm_customstats[MAX_CL_STATS]; // matches the regular stat numbers, but only MIN_VM_STAT to MAX_VM_STAT range is used if things are working properly (can register stats from MAX_VM_STAT to MAX_CL_STATS but will warn)
static int vm_customstats_last;

void VM_CustomStats_Clear (void)
{
	memset(vm_customstats, 0, sizeof(vm_customstats));
	vm_customstats_last = -1;
}

void VM_SV_UpdateCustomStats (client_t *client, prvm_edict_t *ent, sizebuf_t *msg, int *stats)
{
	prvm_prog_t *prog = SVVM_prog;
	int			i;
	char		s[17];
	union {
		int i;
		float f;
	} u;

	for(i=MIN_VM_STAT; i<=vm_customstats_last ;i++)
	{
		if(!vm_customstats[i].type)
			continue;
		switch(vm_customstats[i].type)
		{
		//string as 16 bytes
		case 1:
			memset(s, 0, 17);
			dp_strlcpy(s, PRVM_E_STRING(ent, vm_customstats[i].fieldoffset), 16);
			stats[i] = s[ 0] + s[ 1] * 256 + s[ 2] * 65536 + s[ 3] * 16777216;
			stats[i+1] = s[ 4] + s[ 5] * 256 + s[ 6] * 65536 + s[ 7] * 16777216;
			stats[i+2] = s[ 8] + s[ 9] * 256 + s[10] * 65536 + s[11] * 16777216;
			stats[i+3] = s[12] + s[13] * 256 + s[14] * 65536 + s[15] * 16777216;
			break;
		//float field sent as-is
		case 8:
			// can't directly use PRVM_E_INT on the field because it may be PRVM_64 and a double is not the representation we want to send
			u.f = PRVM_E_FLOAT(ent, vm_customstats[i].fieldoffset);
			stats[i] = u.i;
			break;
		//integer value of float field
		case 2:
			stats[i] = (int)PRVM_E_FLOAT(ent, vm_customstats[i].fieldoffset);
			break;
		default:
			break;
		}
	}
}

extern cvar_t sv_qcstats;

// void(float index, float type, .void field) SV_AddStat = #232;
// Set up an auto-sent player stat.
// Client's get thier own fields sent to them. Index may not be less than 32.
// Type is a value equating to the ev_ values found in qcc to dictate types. Valid ones are:
//          1: string (4 stats carrying a total of 16 charactures)
//          2: float (one stat, float converted to an integer for transportation)
//          8: integer (one stat, not converted to an int, so this can be used to transport floats as floats - what a unique idea!)
static void VM_SV_AddStat(prvm_prog_t *prog)
{
	int		off, i, type;

	VM_SAFEPARMCOUNT(3, VM_SV_AddStat);

	i		= (int)PRVM_G_FLOAT(OFS_PARM0);
	type	= (int)PRVM_G_FLOAT(OFS_PARM1);
	off		= PRVM_G_INT  (OFS_PARM2);

	switch (type)
	{
	case 1:
	case 2:
	case 8:
		break;
	default:
		VM_Warning(prog, "PF_SV_AddStat: unrecognized type %i - supported types are 1 (string up to 16 bytes, takes 4 stat slots), 2 (truncate to int32), 8 (send as float)", type);
		return;
	}

	if (i < 0)
	{
		VM_Warning(prog, "PF_SV_AddStat: index (%i) may not be less than %i\n", i, MIN_VM_STAT);
		return;
	}

	if (i >= MAX_CL_STATS)
	{
		VM_Warning(prog, "PF_SV_AddStat: index (%i) >= MAX_CL_STATS (%i), not supported by protocol, and AddStat beyond MAX_VM_STAT (%i) conflicts with engine MOVEVARS\n", i, MAX_CL_STATS, MAX_VM_STAT);
		return;
	}

	if (i > (MAX_CL_STATS - 4) && type == 1)
	{
		VM_Warning(prog, "PF_SV_AddStat: index (%i) > (MAX_CL_STATS (%i) - 4) with string type won't fit in the protocol, and AddStat beyond MAX_VM_STAT conflicts with engine MOVEVARS\n", i, MAX_CL_STATS);
		return;
	}

	// these are hazardous to override but sort of allowed if one wants to be adventurous...  and enjoys warnings.
	if (i < MIN_VM_STAT)
		VM_Warning(prog, "PF_SV_AddStat: index (%i) < MIN_VM_STAT (%i) may conflict with engine stats - allowed, but this may break things\n", i, MIN_VM_STAT);
	else if (i >= MAX_VM_STAT && !sv_qcstats.integer)
		VM_Warning(prog, "PF_SV_AddStat: index (%i) >= MAX_VM_STAT (%i) conflicts with engine stats - allowed, but this may break slowmo and stuff\n", i, MAX_VM_STAT);
	else if (i > (MAX_VM_STAT - 4) && type == 1 && !sv_qcstats.integer)
		VM_Warning(prog, "PF_SV_AddStat: index (%i) >= MAX_VM_STAT (%i) - 4 with string type won't fit within MAX_VM_STAT, thus conflicting with engine stats - allowed, but this may break slowmo and stuff\n", i, MAX_VM_STAT);

	vm_customstats[i].type		= type;
	vm_customstats[i].fieldoffset	= off;
	if(vm_customstats_last < i)
		vm_customstats_last = i;
}

/*
=================
VM_SV_copyentity

copies data from one entity to another

copyentity(src, dst)
=================
*/
static void VM_SV_copyentity(prvm_prog_t *prog)
{
	prvm_edict_t *in, *out;
	VM_SAFEPARMCOUNT(2, VM_SV_copyentity);
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

	SV_LinkEdict(out);
}


/*
=================
VM_SV_setcolor

sets the color of a client and broadcasts the update to all connected clients

setcolor(clientent, value)
=================
*/
static void VM_SV_setcolor(prvm_prog_t *prog)
{
	client_t *client;
	int entnum, i;

	VM_SAFEPARMCOUNT(2, VM_SV_setcolor);
	entnum = PRVM_G_EDICTNUM(OFS_PARM0);
	i = (int)PRVM_G_FLOAT(OFS_PARM1);

	if (entnum < 1 || entnum > svs.maxclients || !svs.clients[entnum-1].active)
	{
		Con_Print("tried to setcolor a non-client\n");
		return;
	}

	client = svs.clients + entnum-1;
	if (client->edict)
	{
		PRVM_serveredictfloat(client->edict, clientcolors) = i;
		PRVM_serveredictfloat(client->edict, team) = (i & 15) + 1;
	}
	client->colors = i;
	if (client->old_colors != client->colors)
	{
		client->old_colors = client->colors;
		// send notification to all clients
		MSG_WriteByte (&sv.reliable_datagram, svc_updatecolors);
		MSG_WriteByte (&sv.reliable_datagram, client - svs.clients);
		MSG_WriteByte (&sv.reliable_datagram, client->colors);
	}
}

/*
=================
VM_SV_effect

effect(origin, modelname, startframe, framecount, framerate)
=================
*/
static void VM_SV_effect(prvm_prog_t *prog)
{
	int i;
	const char *s;
	vec3_t org;
	VM_SAFEPARMCOUNT(5, VM_SV_effect);
	s = PRVM_G_STRING(OFS_PARM1);
	if (!s[0])
	{
		VM_Warning(prog, "effect: no model specified\n");
		return;
	}

	i = SV_ModelIndex(s, 1);
	if (!i)
	{
		VM_Warning(prog, "effect: model not precached\n");
		return;
	}

	if (PRVM_G_FLOAT(OFS_PARM3) < 1)
	{
		VM_Warning(prog, "effect: framecount < 1\n");
		return;
	}

	if (PRVM_G_FLOAT(OFS_PARM4) < 1)
	{
		VM_Warning(prog, "effect: framerate < 1\n");
		return;
	}

	VectorCopy(PRVM_G_VECTOR(OFS_PARM0), org);
	SV_StartEffect(org, i, (int)PRVM_G_FLOAT(OFS_PARM2), (int)PRVM_G_FLOAT(OFS_PARM3), (int)PRVM_G_FLOAT(OFS_PARM4));
}

static void VM_SV_te_blood(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(3, VM_SV_te_blood);
	if (PRVM_G_FLOAT(OFS_PARM2) < 1)
		return;
	MSG_WriteByte(&sv.datagram, svc_temp_entity);
	MSG_WriteByte(&sv.datagram, TE_BLOOD);
	// origin
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[2], sv.protocol);
	// velocity
	MSG_WriteChar(&sv.datagram, bound(-128, (int) PRVM_G_VECTOR(OFS_PARM1)[0], 127));
	MSG_WriteChar(&sv.datagram, bound(-128, (int) PRVM_G_VECTOR(OFS_PARM1)[1], 127));
	MSG_WriteChar(&sv.datagram, bound(-128, (int) PRVM_G_VECTOR(OFS_PARM1)[2], 127));
	// count
	MSG_WriteByte(&sv.datagram, bound(0, (int) PRVM_G_FLOAT(OFS_PARM2), 255));
	SV_FlushBroadcastMessages();
}

static void VM_SV_te_bloodshower(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(4, VM_SV_te_bloodshower);
	if (PRVM_G_FLOAT(OFS_PARM3) < 1)
		return;
	MSG_WriteByte(&sv.datagram, svc_temp_entity);
	MSG_WriteByte(&sv.datagram, TE_BLOODSHOWER);
	// min
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[2], sv.protocol);
	// max
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM1)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM1)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM1)[2], sv.protocol);
	// speed
	MSG_WriteCoord(&sv.datagram, PRVM_G_FLOAT(OFS_PARM2), sv.protocol);
	// count
	MSG_WriteShort(&sv.datagram, (int)bound(0, PRVM_G_FLOAT(OFS_PARM3), 65535));
	SV_FlushBroadcastMessages();
}

static void VM_SV_te_explosionrgb(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(2, VM_SV_te_explosionrgb);
	MSG_WriteByte(&sv.datagram, svc_temp_entity);
	MSG_WriteByte(&sv.datagram, TE_EXPLOSIONRGB);
	// origin
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[2], sv.protocol);
	// color
	MSG_WriteByte(&sv.datagram, bound(0, (int) (PRVM_G_VECTOR(OFS_PARM1)[0] * 255), 255));
	MSG_WriteByte(&sv.datagram, bound(0, (int) (PRVM_G_VECTOR(OFS_PARM1)[1] * 255), 255));
	MSG_WriteByte(&sv.datagram, bound(0, (int) (PRVM_G_VECTOR(OFS_PARM1)[2] * 255), 255));
	SV_FlushBroadcastMessages();
}

static void VM_SV_te_particlecube(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(7, VM_SV_te_particlecube);
	if (PRVM_G_FLOAT(OFS_PARM3) < 1)
		return;
	MSG_WriteByte(&sv.datagram, svc_temp_entity);
	MSG_WriteByte(&sv.datagram, TE_PARTICLECUBE);
	// min
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[2], sv.protocol);
	// max
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM1)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM1)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM1)[2], sv.protocol);
	// velocity
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM2)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM2)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM2)[2], sv.protocol);
	// count
	MSG_WriteShort(&sv.datagram, (int)bound(0, PRVM_G_FLOAT(OFS_PARM3), 65535));
	// color
	MSG_WriteByte(&sv.datagram, (int)PRVM_G_FLOAT(OFS_PARM4));
	// gravity true/false
	MSG_WriteByte(&sv.datagram, ((int) PRVM_G_FLOAT(OFS_PARM5)) != 0);
	// randomvel
	MSG_WriteCoord(&sv.datagram, PRVM_G_FLOAT(OFS_PARM6), sv.protocol);
	SV_FlushBroadcastMessages();
}

static void VM_SV_te_particlerain(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(5, VM_SV_te_particlerain);
	if (PRVM_G_FLOAT(OFS_PARM3) < 1)
		return;
	MSG_WriteByte(&sv.datagram, svc_temp_entity);
	MSG_WriteByte(&sv.datagram, TE_PARTICLERAIN);
	// min
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[2], sv.protocol);
	// max
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM1)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM1)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM1)[2], sv.protocol);
	// velocity
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM2)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM2)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM2)[2], sv.protocol);
	// count
	MSG_WriteShort(&sv.datagram, (int)bound(0, PRVM_G_FLOAT(OFS_PARM3), 65535));
	// color
	MSG_WriteByte(&sv.datagram, (int)PRVM_G_FLOAT(OFS_PARM4));
	SV_FlushBroadcastMessages();
}

static void VM_SV_te_particlesnow(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(5, VM_SV_te_particlesnow);
	if (PRVM_G_FLOAT(OFS_PARM3) < 1)
		return;
	MSG_WriteByte(&sv.datagram, svc_temp_entity);
	MSG_WriteByte(&sv.datagram, TE_PARTICLESNOW);
	// min
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[2], sv.protocol);
	// max
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM1)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM1)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM1)[2], sv.protocol);
	// velocity
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM2)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM2)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM2)[2], sv.protocol);
	// count
	MSG_WriteShort(&sv.datagram, (int)bound(0, PRVM_G_FLOAT(OFS_PARM3), 65535));
	// color
	MSG_WriteByte(&sv.datagram, (int)PRVM_G_FLOAT(OFS_PARM4));
	SV_FlushBroadcastMessages();
}

static void VM_SV_te_spark(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(3, VM_SV_te_spark);
	if (PRVM_G_FLOAT(OFS_PARM2) < 1)
		return;
	MSG_WriteByte(&sv.datagram, svc_temp_entity);
	MSG_WriteByte(&sv.datagram, TE_SPARK);
	// origin
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[2], sv.protocol);
	// velocity
	MSG_WriteChar(&sv.datagram, bound(-128, (int) PRVM_G_VECTOR(OFS_PARM1)[0], 127));
	MSG_WriteChar(&sv.datagram, bound(-128, (int) PRVM_G_VECTOR(OFS_PARM1)[1], 127));
	MSG_WriteChar(&sv.datagram, bound(-128, (int) PRVM_G_VECTOR(OFS_PARM1)[2], 127));
	// count
	MSG_WriteByte(&sv.datagram, bound(0, (int) PRVM_G_FLOAT(OFS_PARM2), 255));
	SV_FlushBroadcastMessages();
}

static void VM_SV_te_gunshotquad(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(1, VM_SV_te_gunshotquad);
	MSG_WriteByte(&sv.datagram, svc_temp_entity);
	MSG_WriteByte(&sv.datagram, TE_GUNSHOTQUAD);
	// origin
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[2], sv.protocol);
	SV_FlushBroadcastMessages();
}

static void VM_SV_te_spikequad(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(1, VM_SV_te_spikequad);
	MSG_WriteByte(&sv.datagram, svc_temp_entity);
	MSG_WriteByte(&sv.datagram, TE_SPIKEQUAD);
	// origin
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[2], sv.protocol);
	SV_FlushBroadcastMessages();
}

static void VM_SV_te_superspikequad(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(1, VM_SV_te_superspikequad);
	MSG_WriteByte(&sv.datagram, svc_temp_entity);
	MSG_WriteByte(&sv.datagram, TE_SUPERSPIKEQUAD);
	// origin
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[2], sv.protocol);
	SV_FlushBroadcastMessages();
}

static void VM_SV_te_explosionquad(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(1, VM_SV_te_explosionquad);
	MSG_WriteByte(&sv.datagram, svc_temp_entity);
	MSG_WriteByte(&sv.datagram, TE_EXPLOSIONQUAD);
	// origin
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[2], sv.protocol);
	SV_FlushBroadcastMessages();
}

static void VM_SV_te_smallflash(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(1, VM_SV_te_smallflash);
	MSG_WriteByte(&sv.datagram, svc_temp_entity);
	MSG_WriteByte(&sv.datagram, TE_SMALLFLASH);
	// origin
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[2], sv.protocol);
	SV_FlushBroadcastMessages();
}

static void VM_SV_te_customflash(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(4, VM_SV_te_customflash);
	if (PRVM_G_FLOAT(OFS_PARM1) < 8 || PRVM_G_FLOAT(OFS_PARM2) < (1.0 / 256.0))
		return;
	MSG_WriteByte(&sv.datagram, svc_temp_entity);
	MSG_WriteByte(&sv.datagram, TE_CUSTOMFLASH);
	// origin
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[2], sv.protocol);
	// radius
	MSG_WriteByte(&sv.datagram, (int)bound(0, PRVM_G_FLOAT(OFS_PARM1) / 8 - 1, 255));
	// lifetime
	MSG_WriteByte(&sv.datagram, (int)bound(0, PRVM_G_FLOAT(OFS_PARM2) * 256 - 1, 255));
	// color
	MSG_WriteByte(&sv.datagram, (int)bound(0, PRVM_G_VECTOR(OFS_PARM3)[0] * 255, 255));
	MSG_WriteByte(&sv.datagram, (int)bound(0, PRVM_G_VECTOR(OFS_PARM3)[1] * 255, 255));
	MSG_WriteByte(&sv.datagram, (int)bound(0, PRVM_G_VECTOR(OFS_PARM3)[2] * 255, 255));
	SV_FlushBroadcastMessages();
}

static void VM_SV_te_gunshot(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(1, VM_SV_te_gunshot);
	MSG_WriteByte(&sv.datagram, svc_temp_entity);
	MSG_WriteByte(&sv.datagram, TE_GUNSHOT);
	// origin
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[2], sv.protocol);
	SV_FlushBroadcastMessages();
}

static void VM_SV_te_spike(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(1, VM_SV_te_spike);
	MSG_WriteByte(&sv.datagram, svc_temp_entity);
	MSG_WriteByte(&sv.datagram, TE_SPIKE);
	// origin
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[2], sv.protocol);
	SV_FlushBroadcastMessages();
}

static void VM_SV_te_superspike(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(1, VM_SV_te_superspike);
	MSG_WriteByte(&sv.datagram, svc_temp_entity);
	MSG_WriteByte(&sv.datagram, TE_SUPERSPIKE);
	// origin
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[2], sv.protocol);
	SV_FlushBroadcastMessages();
}

static void VM_SV_te_explosion(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(1, VM_SV_te_explosion);
	MSG_WriteByte(&sv.datagram, svc_temp_entity);
	MSG_WriteByte(&sv.datagram, TE_EXPLOSION);
	// origin
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[2], sv.protocol);
	SV_FlushBroadcastMessages();
}

static void VM_SV_te_tarexplosion(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(1, VM_SV_te_tarexplosion);
	MSG_WriteByte(&sv.datagram, svc_temp_entity);
	MSG_WriteByte(&sv.datagram, TE_TAREXPLOSION);
	// origin
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[2], sv.protocol);
	SV_FlushBroadcastMessages();
}

static void VM_SV_te_wizspike(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(1, VM_SV_te_wizspike);
	MSG_WriteByte(&sv.datagram, svc_temp_entity);
	MSG_WriteByte(&sv.datagram, TE_WIZSPIKE);
	// origin
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[2], sv.protocol);
	SV_FlushBroadcastMessages();
}

static void VM_SV_te_knightspike(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(1, VM_SV_te_knightspike);
	MSG_WriteByte(&sv.datagram, svc_temp_entity);
	MSG_WriteByte(&sv.datagram, TE_KNIGHTSPIKE);
	// origin
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[2], sv.protocol);
	SV_FlushBroadcastMessages();
}

static void VM_SV_te_lavasplash(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(1, VM_SV_te_lavasplash);
	MSG_WriteByte(&sv.datagram, svc_temp_entity);
	MSG_WriteByte(&sv.datagram, TE_LAVASPLASH);
	// origin
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[2], sv.protocol);
	SV_FlushBroadcastMessages();
}

static void VM_SV_te_teleport(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(1, VM_SV_te_teleport);
	MSG_WriteByte(&sv.datagram, svc_temp_entity);
	MSG_WriteByte(&sv.datagram, TE_TELEPORT);
	// origin
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[2], sv.protocol);
	SV_FlushBroadcastMessages();
}

static void VM_SV_te_explosion2(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(3, VM_SV_te_explosion2);
	MSG_WriteByte(&sv.datagram, svc_temp_entity);
	MSG_WriteByte(&sv.datagram, TE_EXPLOSION2);
	// origin
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[2], sv.protocol);
	// color
	MSG_WriteByte(&sv.datagram, (int)PRVM_G_FLOAT(OFS_PARM1));
	MSG_WriteByte(&sv.datagram, (int)PRVM_G_FLOAT(OFS_PARM2));
	SV_FlushBroadcastMessages();
}

static void VM_SV_te_lightning1(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(3, VM_SV_te_lightning1);
	MSG_WriteByte(&sv.datagram, svc_temp_entity);
	MSG_WriteByte(&sv.datagram, TE_LIGHTNING1);
	// owner entity
	MSG_WriteShort(&sv.datagram, PRVM_G_EDICTNUM(OFS_PARM0));
	// start
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM1)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM1)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM1)[2], sv.protocol);
	// end
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM2)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM2)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM2)[2], sv.protocol);
	SV_FlushBroadcastMessages();
}

static void VM_SV_te_lightning2(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(3, VM_SV_te_lightning2);
	MSG_WriteByte(&sv.datagram, svc_temp_entity);
	MSG_WriteByte(&sv.datagram, TE_LIGHTNING2);
	// owner entity
	MSG_WriteShort(&sv.datagram, PRVM_G_EDICTNUM(OFS_PARM0));
	// start
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM1)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM1)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM1)[2], sv.protocol);
	// end
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM2)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM2)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM2)[2], sv.protocol);
	SV_FlushBroadcastMessages();
}

static void VM_SV_te_lightning3(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(3, VM_SV_te_lightning3);
	MSG_WriteByte(&sv.datagram, svc_temp_entity);
	MSG_WriteByte(&sv.datagram, TE_LIGHTNING3);
	// owner entity
	MSG_WriteShort(&sv.datagram, PRVM_G_EDICTNUM(OFS_PARM0));
	// start
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM1)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM1)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM1)[2], sv.protocol);
	// end
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM2)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM2)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM2)[2], sv.protocol);
	SV_FlushBroadcastMessages();
}

static void VM_SV_te_beam(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(3, VM_SV_te_beam);
	MSG_WriteByte(&sv.datagram, svc_temp_entity);
	MSG_WriteByte(&sv.datagram, TE_BEAM);
	// owner entity
	MSG_WriteShort(&sv.datagram, PRVM_G_EDICTNUM(OFS_PARM0));
	// start
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM1)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM1)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM1)[2], sv.protocol);
	// end
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM2)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM2)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM2)[2], sv.protocol);
	SV_FlushBroadcastMessages();
}

static void VM_SV_te_plasmaburn(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(1, VM_SV_te_plasmaburn);
	MSG_WriteByte(&sv.datagram, svc_temp_entity);
	MSG_WriteByte(&sv.datagram, TE_PLASMABURN);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[2], sv.protocol);
	SV_FlushBroadcastMessages();
}

static void VM_SV_te_flamejet(prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(3, VM_SV_te_flamejet);
	MSG_WriteByte(&sv.datagram, svc_temp_entity);
	MSG_WriteByte(&sv.datagram, TE_FLAMEJET);
	// org
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM0)[2], sv.protocol);
	// vel
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM1)[0], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM1)[1], sv.protocol);
	MSG_WriteCoord(&sv.datagram, PRVM_G_VECTOR(OFS_PARM1)[2], sv.protocol);
	// count
	MSG_WriteByte(&sv.datagram, (int)PRVM_G_FLOAT(OFS_PARM2));
	SV_FlushBroadcastMessages();
}

//void(entity e, string s) clientcommand = #440; // executes a command string as if it came from the specified client
//this function originally written by KrimZon, made shorter by LadyHavoc
static void VM_SV_clientcommand(prvm_prog_t *prog)
{
	client_t *temp_client;
	int i;
	VM_SAFEPARMCOUNT(2, VM_SV_clientcommand);

	//find client for this entity
	i = (PRVM_NUM_FOR_EDICT(PRVM_G_EDICT(OFS_PARM0)) - 1);
	if (i < 0 || i >= svs.maxclients || !svs.clients[i].active)
	{
		Con_Print("PF_clientcommand: entity is not a client\n");
		return;
	}

	temp_client = host_client;
	host_client = svs.clients + i;
	Cmd_ExecuteString(cmd_serverfromclient, PRVM_G_STRING(OFS_PARM1), strlen(PRVM_G_STRING(OFS_PARM1)), src_client, true);
	host_client = temp_client;
}

//void(entity e, entity tagentity, string tagname) setattachment = #443; // attachs e to a tag on tagentity (note: use "" to attach to entity origin/angles instead of a tag)
static void VM_SV_setattachment(prvm_prog_t *prog)
{
	prvm_edict_t *e = PRVM_G_EDICT(OFS_PARM0);
	prvm_edict_t *tagentity = PRVM_G_EDICT(OFS_PARM1);
	const char *tagname = PRVM_G_STRING(OFS_PARM2);
	model_t *model;
	int tagindex;
	VM_SAFEPARMCOUNT(3, VM_SV_setattachment);

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
		model = SV_GetModelFromEdict(tagentity);
		if (model)
		{
			tagindex = Mod_Alias_GetTagIndexForName(model, (int)PRVM_serveredictfloat(tagentity, skin), tagname);
			if (tagindex == 0)
				Con_DPrintf("setattachment(edict %i, edict %i, string \"%s\"): tried to find tag named \"%s\" on entity %i (model \"%s\") but could not find it\n", PRVM_NUM_FOR_EDICT(e), PRVM_NUM_FOR_EDICT(tagentity), tagname, tagname, PRVM_NUM_FOR_EDICT(tagentity), model->name);
		}
		else
			Con_DPrintf("setattachment(edict %i, edict %i, string \"%s\"): tried to find tag named \"%s\" on entity %i but it has no model\n", PRVM_NUM_FOR_EDICT(e), PRVM_NUM_FOR_EDICT(tagentity), tagname, tagname, PRVM_NUM_FOR_EDICT(tagentity));
	}

	PRVM_serveredictedict(e, tag_entity) = PRVM_EDICT_TO_PROG(tagentity);
	PRVM_serveredictfloat(e, tag_index) = tagindex;
}

/////////////////////////////////////////
// DP_MD3_TAGINFO extension coded by VorteX

static int SV_GetTagIndex (prvm_prog_t *prog, prvm_edict_t *e, const char *tagname)
{
	int i;

	i = (int)PRVM_serveredictfloat(e, modelindex);
	if (i < 1 || i >= MAX_MODELS)
		return -1;

	return Mod_Alias_GetTagIndexForName(SV_GetModelByIndex(i), (int)PRVM_serveredictfloat(e, skin), tagname);
}

static int SV_GetExtendedTagInfo (prvm_prog_t *prog, prvm_edict_t *e, int tagindex, int *parentindex, const char **tagname, matrix4x4_t *tag_localmatrix)
{
	int r;
	model_t *model;

	*tagname = NULL;
	*parentindex = 0;
	Matrix4x4_CreateIdentity(tag_localmatrix);

	if (tagindex >= 0 && (model = SV_GetModelFromEdict(e)) && model->num_bones)
	{
		r = Mod_Alias_GetExtendedTagInfoForIndex(model, (int)PRVM_serveredictfloat(e, skin), e->priv.server->frameblend, &e->priv.server->skeleton, tagindex - 1, parentindex, tagname, tag_localmatrix);

		if(!r) // success?
			*parentindex += 1;

		return r;
	}

	return 1;
}

void SV_GetEntityMatrix (prvm_prog_t *prog, prvm_edict_t *ent, matrix4x4_t *out, qbool viewmatrix)
{
	float scale;
	float pitchsign = 1;

	scale = PRVM_serveredictfloat(ent, scale);
	if (!scale)
		scale = 1.0f;
	
	if (viewmatrix)
		Matrix4x4_CreateFromQuakeEntity(out, PRVM_serveredictvector(ent, origin)[0], PRVM_serveredictvector(ent, origin)[1], PRVM_serveredictvector(ent, origin)[2] + PRVM_serveredictvector(ent, view_ofs)[2], PRVM_serveredictvector(ent, v_angle)[0], PRVM_serveredictvector(ent, v_angle)[1], PRVM_serveredictvector(ent, v_angle)[2], scale * cl_viewmodel_scale.value);
	else
	{
		pitchsign = SV_GetPitchSign(prog, ent);
		Matrix4x4_CreateFromQuakeEntity(out, PRVM_serveredictvector(ent, origin)[0], PRVM_serveredictvector(ent, origin)[1], PRVM_serveredictvector(ent, origin)[2], pitchsign * PRVM_serveredictvector(ent, angles)[0], PRVM_serveredictvector(ent, angles)[1], PRVM_serveredictvector(ent, angles)[2], scale);
	}
}

static int SV_GetEntityLocalTagMatrix(prvm_prog_t *prog, prvm_edict_t *ent, int tagindex, matrix4x4_t *out)
{
	model_t *model;
	if (tagindex >= 0 && (model = SV_GetModelFromEdict(ent)) && model->animscenes)
	{
		VM_GenerateFrameGroupBlend(prog, ent->priv.server->framegroupblend, ent);
		VM_FrameBlendFromFrameGroupBlend(ent->priv.server->frameblend, ent->priv.server->framegroupblend, model, sv.time);
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
static int SV_GetTagMatrix (prvm_prog_t *prog, matrix4x4_t *out, prvm_edict_t *ent, int tagindex)
{
	int ret;
	int modelindex, attachloop;
	matrix4x4_t entitymatrix, tagmatrix, attachmatrix;
	model_t *model;

	*out = identitymatrix; // warnings and errors return identical matrix

	if (ent == prog->edicts)
		return 1;
	if (ent->free)
		return 2;

	modelindex = (int)PRVM_serveredictfloat(ent, modelindex);
	if (modelindex <= 0 || modelindex >= MAX_MODELS)
		return 3;

	model = SV_GetModelByIndex(modelindex);

	VM_GenerateFrameGroupBlend(prog, ent->priv.server->framegroupblend, ent);
	VM_FrameBlendFromFrameGroupBlend(ent->priv.server->frameblend, ent->priv.server->framegroupblend, model, sv.time);
	VM_UpdateEdictSkeleton(prog, ent, model, ent->priv.server->frameblend);

	tagmatrix = identitymatrix;
	// DP_GFX_QUAKE3MODELTAGS, scan all chain and stop on unattached entity
	attachloop = 0;
	for (;;)
	{
		if (attachloop >= 256) // prevent runaway looping
			return 5;
		// apply transformation by child's tagindex on parent entity and then
		// by parent entity itself
		ret = SV_GetEntityLocalTagMatrix(prog, ent, tagindex - 1, &attachmatrix);
		if (ret && attachloop == 0)
			return ret;
		SV_GetEntityMatrix(prog, ent, &entitymatrix, false);
		Matrix4x4_Concat(&tagmatrix, &attachmatrix, out);
		Matrix4x4_Concat(out, &entitymatrix, &tagmatrix);
		// next iteration we process the parent entity
		if (PRVM_serveredictedict(ent, tag_entity))
		{
			tagindex = (int)PRVM_serveredictfloat(ent, tag_index);
			ent = PRVM_EDICT_NUM(PRVM_serveredictedict(ent, tag_entity));
		}
		else
			break;
		attachloop++;
	}

	// RENDER_VIEWMODEL magic
	if (PRVM_serveredictedict(ent, viewmodelforclient))
	{
		Matrix4x4_Copy(&tagmatrix, out);
		ent = PRVM_EDICT_NUM(PRVM_serveredictedict(ent, viewmodelforclient));

		SV_GetEntityMatrix(prog, ent, &entitymatrix, true);
		Matrix4x4_Concat(out, &entitymatrix, &tagmatrix);
	}
	return 0;
}

//float(entity ent, string tagname) gettagindex;

static void VM_SV_gettagindex(prvm_prog_t *prog)
{
	prvm_edict_t *ent;
	const char *tag_name;
	int tag_index;

	VM_SAFEPARMCOUNT(2, VM_SV_gettagindex);

	ent = PRVM_G_EDICT(OFS_PARM0);
	tag_name = PRVM_G_STRING(OFS_PARM1);

	if (ent == prog->edicts)
	{
		VM_Warning(prog, "VM_SV_gettagindex(entity #%i): can't affect world entity\n", PRVM_NUM_FOR_EDICT(ent));
		return;
	}
	if (ent->free)
	{
		VM_Warning(prog, "VM_SV_gettagindex(entity #%i): can't affect free entity\n", PRVM_NUM_FOR_EDICT(ent));
		return;
	}

	tag_index = 0;
	if (!SV_GetModelFromEdict(ent))
		Con_DPrintf("VM_SV_gettagindex(entity #%i): null or non-precached model\n", PRVM_NUM_FOR_EDICT(ent));
	else
	{
		tag_index = SV_GetTagIndex(prog, ent, tag_name);
		if (tag_index == 0)
			if(developer_extra.integer)
				Con_DPrintf("VM_SV_gettagindex(entity #%i): tag \"%s\" not found\n", PRVM_NUM_FOR_EDICT(ent), tag_name);
	}
	PRVM_G_FLOAT(OFS_RETURN) = tag_index;
}

//vector(entity ent, float tagindex) gettaginfo;
static void VM_SV_gettaginfo(prvm_prog_t *prog)
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

	VM_SAFEPARMCOUNT(2, VM_SV_gettaginfo);

	e = PRVM_G_EDICT(OFS_PARM0);
	tagindex = (int)PRVM_G_FLOAT(OFS_PARM1);

	returncode = SV_GetTagMatrix(prog, &tag_matrix, e, tagindex);
	Matrix4x4_ToVectors(&tag_matrix, forward, left, up, origin);
	VectorCopy(forward, PRVM_serverglobalvector(v_forward));
	VectorNegate(left, PRVM_serverglobalvector(v_right));
	VectorCopy(up, PRVM_serverglobalvector(v_up));
	VectorCopy(origin, PRVM_G_VECTOR(OFS_RETURN));
	model = SV_GetModelFromEdict(e);
	VM_GenerateFrameGroupBlend(prog, e->priv.server->framegroupblend, e);
	VM_FrameBlendFromFrameGroupBlend(e->priv.server->frameblend, e->priv.server->framegroupblend, model, sv.time);
	VM_UpdateEdictSkeleton(prog, e, model, e->priv.server->frameblend);
	SV_GetExtendedTagInfo(prog, e, tagindex, &parentindex, &tagname, &tag_localmatrix);
	Matrix4x4_ToVectors(&tag_localmatrix, forward, left, up, origin);

	PRVM_serverglobalfloat(gettaginfo_parent) = parentindex;
	PRVM_serverglobalstring(gettaginfo_name) = tagname ? PRVM_SetTempString(prog, tagname, strlen(tagname)) : 0;
	VectorCopy(forward, PRVM_serverglobalvector(gettaginfo_forward));
	VectorNegate(left, PRVM_serverglobalvector(gettaginfo_right));
	VectorCopy(up, PRVM_serverglobalvector(gettaginfo_up));
	VectorCopy(origin, PRVM_serverglobalvector(gettaginfo_offset));

	switch(returncode)
	{
		case 1:
			VM_Warning(prog, "gettagindex: can't affect world entity\n");
			break;
		case 2:
			VM_Warning(prog, "gettagindex: can't affect free entity\n");
			break;
		case 3:
			Con_DPrintf("SV_GetTagMatrix(entity #%i): null or non-precached model\n", PRVM_NUM_FOR_EDICT(e));
			break;
		case 4:
			Con_DPrintf("SV_GetTagMatrix(entity #%i): model has no tag with requested index %i\n", PRVM_NUM_FOR_EDICT(e), tagindex);
			break;
		case 5:
			Con_DPrintf("SV_GetTagMatrix(entity #%i): runaway loop at attachment chain\n", PRVM_NUM_FOR_EDICT(e));
			break;
	}
}

//void(entity clent) dropclient (DP_SV_DROPCLIENT)
static void VM_SV_dropclient(prvm_prog_t *prog)
{
	int clientnum;
	client_t *oldhostclient;
	VM_SAFEPARMCOUNT(1, VM_SV_dropclient);
	clientnum = PRVM_G_EDICTNUM(OFS_PARM0) - 1;
	if (clientnum < 0 || clientnum >= svs.maxclients)
	{
		VM_Warning(prog, "dropclient: not a client\n");
		return;
	}
	if (!svs.clients[clientnum].active)
	{
		VM_Warning(prog, "dropclient: that client slot is not connected\n");
		return;
	}
	oldhostclient = host_client;
	host_client = svs.clients + clientnum;
	SV_DropClient(false, "Client dropped");
	host_client = oldhostclient;
}

//entity() spawnclient (DP_SV_BOTCLIENT)
static void VM_SV_spawnclient(prvm_prog_t *prog)
{
	int i;
	prvm_edict_t	*ed;
	VM_SAFEPARMCOUNT(0, VM_SV_spawnclient);
	prog->xfunction->builtinsprofile += 2;
	ed = prog->edicts;
	for (i = 0;i < svs.maxclients;i++)
	{
		if (!svs.clients[i].active)
		{
			prog->xfunction->builtinsprofile += 100;
			SV_ConnectClient (i, NULL);
			// this has to be set or else ClientDisconnect won't be called
			// we assume the qc will call ClientConnect...
			svs.clients[i].clientconnectcalled = true;
			ed = PRVM_EDICT_NUM(i + 1);
			break;
		}
	}
	VM_RETURN_EDICT(ed);
}

//float(entity clent) clienttype (DP_SV_BOTCLIENT)
static void VM_SV_clienttype(prvm_prog_t *prog)
{
	int clientnum;
	VM_SAFEPARMCOUNT(1, VM_SV_clienttype);
	clientnum = PRVM_G_EDICTNUM(OFS_PARM0) - 1;
	if (clientnum < 0 || clientnum >= svs.maxclients)
		PRVM_G_FLOAT(OFS_RETURN) = 3; // CLIENTTYPE_NOTACLIENT
	else if (!svs.clients[clientnum].active)
		PRVM_G_FLOAT(OFS_RETURN) = 0; // CLIENTTYPE_DISCONNECTED
	else if (svs.clients[clientnum].netconnection)
		PRVM_G_FLOAT(OFS_RETURN) = 1; // CLIENTTYPE_REAL
	else
		PRVM_G_FLOAT(OFS_RETURN) = 2; // CLIENTTYPE_BOT
}

/*
===============
VM_SV_serverkey

string(string key) serverkey
===============
*/
static void VM_SV_serverkey(prvm_prog_t *prog)
{
	char string[VM_TEMPSTRING_MAXSIZE];

	VM_SAFEPARMCOUNT(1, VM_SV_serverkey);
	InfoString_GetValue(svs.serverinfo, PRVM_G_STRING(OFS_PARM0), string, sizeof(string));
	PRVM_G_INT(OFS_RETURN) = PRVM_SetTempString(prog, string, strlen(string));
}

//#333 void(entity e, float mdlindex) setmodelindex (EXT_CSQC)
static void VM_SV_setmodelindex(prvm_prog_t *prog)
{
	prvm_edict_t	*e;
	model_t	*mod;
	int		i;
	VM_SAFEPARMCOUNT(2, VM_SV_setmodelindex);

	e = PRVM_G_EDICT(OFS_PARM0);
	if (e == prog->edicts)
	{
		VM_Warning(prog, "setmodelindex: can not modify world entity\n");
		return;
	}
	if (e->free)
	{
		VM_Warning(prog, "setmodelindex: can not modify free entity\n");
		return;
	}
	i = (int)PRVM_G_FLOAT(OFS_PARM1);
	if (i <= 0 || i >= MAX_MODELS)
	{
		VM_Warning(prog, "setmodelindex: invalid modelindex\n");
		return;
	}
	if (!sv.model_precache[i][0])
	{
		VM_Warning(prog, "setmodelindex: model not precached\n");
		return;
	}

	PRVM_serveredictstring(e, model) = PRVM_SetEngineString(prog, sv.model_precache[i]);
	PRVM_serveredictfloat(e, modelindex) = i;

	mod = SV_GetModelByIndex(i);

	if (mod)
	{
		if (mod->type != mod_alias || sv_gameplayfix_setmodelrealbox.integer)
			SetMinMaxSize(prog, e, mod->normalmins, mod->normalmaxs, true);
		else
			SetMinMaxSize(prog, e, quakemins, quakemaxs, true);
	}
	else
		SetMinMaxSize(prog, e, vec3_origin, vec3_origin, true);
}

//#334 string(float mdlindex) modelnameforindex (EXT_CSQC)
static void VM_SV_modelnameforindex(prvm_prog_t *prog)
{
	int i;
	VM_SAFEPARMCOUNT(1, VM_SV_modelnameforindex);

	PRVM_G_INT(OFS_RETURN) = OFS_NULL;

	i = (int)PRVM_G_FLOAT(OFS_PARM0);
	if (i <= 0 || i >= MAX_MODELS)
	{
		VM_Warning(prog, "modelnameforindex: invalid modelindex\n");
		return;
	}
	if (!sv.model_precache[i][0])
	{
		VM_Warning(prog, "modelnameforindex: model not precached\n");
		return;
	}

	PRVM_G_INT(OFS_RETURN) = PRVM_SetEngineString(prog, sv.model_precache[i]);
}

//#335 float(string effectname) particleeffectnum (EXT_CSQC)
static void VM_SV_particleeffectnum(prvm_prog_t *prog)
{
	int			i;
	VM_SAFEPARMCOUNT(1, VM_SV_particleeffectnum);
	i = SV_ParticleEffectIndex(PRVM_G_STRING(OFS_PARM0));
	if (i == 0)
		i = -1;
	PRVM_G_FLOAT(OFS_RETURN) = i;
}

// #336 void(entity ent, float effectnum, vector start, vector end) trailparticles (EXT_CSQC)
static void VM_SV_trailparticles(prvm_prog_t *prog)
{
	vec3_t start, end;
	VM_SAFEPARMCOUNT(4, VM_SV_trailparticles);

	if ((int)PRVM_G_FLOAT(OFS_PARM0) < 0)
		return;

	MSG_WriteByte(&sv.datagram, svc_trailparticles);
	MSG_WriteShort(&sv.datagram, PRVM_G_EDICTNUM(OFS_PARM0));
	MSG_WriteShort(&sv.datagram, (int)PRVM_G_FLOAT(OFS_PARM1));
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), start);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM3), end);
	MSG_WriteVector(&sv.datagram, start, sv.protocol);
	MSG_WriteVector(&sv.datagram, end, sv.protocol);
	SV_FlushBroadcastMessages();
}

//#337 void(float effectnum, vector origin, vector dir, float count) pointparticles (EXT_CSQC)
static void VM_SV_pointparticles(prvm_prog_t *prog)
{
	int effectnum, count;
	vec3_t org, vel;
	VM_SAFEPARMCOUNTRANGE(4, 8, VM_SV_pointparticles);

	if ((int)PRVM_G_FLOAT(OFS_PARM0) < 0)
		return;

	effectnum = (int)PRVM_G_FLOAT(OFS_PARM0);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM1), org);
	VectorCopy(PRVM_G_VECTOR(OFS_PARM2), vel);
	count = bound(0, (int)PRVM_G_FLOAT(OFS_PARM3), 65535);
	if (count == 1 && !VectorLength2(vel))
	{
		// 1+2+12=15 bytes
		MSG_WriteByte(&sv.datagram, svc_pointparticles1);
		MSG_WriteShort(&sv.datagram, effectnum);
		MSG_WriteVector(&sv.datagram, org, sv.protocol);
	}
	else
	{
		// 1+2+12+12+2=29 bytes
		MSG_WriteByte(&sv.datagram, svc_pointparticles);
		MSG_WriteShort(&sv.datagram, effectnum);
		MSG_WriteVector(&sv.datagram, org, sv.protocol);
		MSG_WriteVector(&sv.datagram, vel, sv.protocol);
		MSG_WriteShort(&sv.datagram, count);
	}

	SV_FlushBroadcastMessages();
}

qbool SV_VM_ConsoleCommand(const char *text, size_t textlen)
{
	prvm_prog_t *prog = SVVM_prog;
	return PRVM_ConsoleCommand(prog, text, textlen, &prog->funcoffsets.ConsoleCmd, true, PRVM_EDICT_TO_PROG(sv.world.prog->edicts), sv.time, "QC function ConsoleCmd is missing");
}

// #352 void(string cmdname) registercommand (EXT_CSQC)
static void VM_SV_registercommand (prvm_prog_t *prog)
{
	VM_SAFEPARMCOUNT(1, VM_SV_registercmd);
	Cmd_AddCommand(CF_SERVER, PRVM_G_STRING(OFS_PARM0), NULL, "console command created by QuakeC");
}

//PF_setpause,    // void(float pause) setpause	= #531;
static void VM_SV_setpause(prvm_prog_t *prog) {
	int pauseValue;
	pauseValue = (int)PRVM_G_FLOAT(OFS_PARM0);
	if (pauseValue != 0) { //pause the game
		sv.paused = 1;
		sv.pausedstart = host.realtime;
	} else { //disable pause, in case it was enabled
		if (sv.paused != 0) {
			sv.paused = 0;
			sv.pausedstart = 0;
		}
	}
	// send notification to all clients
	MSG_WriteByte(&sv.reliable_datagram, svc_setpause);
	MSG_WriteByte(&sv.reliable_datagram, sv.paused);
}

// #263 float(float modlindex) skel_create = #263; // (FTE_CSQC_SKELETONOBJECTS) create a skeleton (be sure to assign this value into .skeletonindex for use), returns skeleton index (1 or higher) on success, returns 0 on failure  (for example if the modelindex is not skeletal), it is recommended that you create a new skeleton if you change modelindex.
static void VM_SV_skel_create(prvm_prog_t *prog)
{
	int modelindex = (int)PRVM_G_FLOAT(OFS_PARM0);
	model_t *model = SV_GetModelByIndex(modelindex);
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
	prog->skeletons[i] = skeleton = (skeleton_t *)Mem_Alloc(prog->progs_mempool, sizeof(skeleton_t) + model->num_bones * sizeof(matrix4x4_t));
	PRVM_G_FLOAT(OFS_RETURN) = i + 1;
	skeleton->model = model;
	skeleton->relativetransforms = (matrix4x4_t *)(skeleton+1);
	// initialize to identity matrices
	for (i = 0;i < skeleton->model->num_bones;i++)
		skeleton->relativetransforms[i] = identitymatrix;
}

// #264 float(float skel, entity ent, float modlindex, float retainfrac, float firstbone, float lastbone) skel_build = #264; // (FTE_CSQC_SKELETONOBJECTS) blend in a percentage of standard animation, 0 replaces entirely, 1 does nothing, 0.5 blends half, etc, and this only alters the bones in the specified range for which out of bounds values like 0,100000 are safe (uses .frame, .frame2, .frame3, .frame4, .lerpfrac, .lerpfrac3, .lerpfrac4, .frame1time, .frame2time, .frame3time, .frame4time), returns skel on success, 0 on failure
static void VM_SV_skel_build(prvm_prog_t *prog)
{
	int skeletonindex = (int)PRVM_G_FLOAT(OFS_PARM0) - 1;
	skeleton_t *skeleton;
	prvm_edict_t *ed = PRVM_G_EDICT(OFS_PARM1);
	int modelindex = (int)PRVM_G_FLOAT(OFS_PARM2);
	float retainfrac = PRVM_G_FLOAT(OFS_PARM3);
	int firstbone = PRVM_G_FLOAT(OFS_PARM4) - 1;
	int lastbone = PRVM_G_FLOAT(OFS_PARM5) - 1;
	model_t *model = SV_GetModelByIndex(modelindex);
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
	VM_FrameBlendFromFrameGroupBlend(frameblend, framegroupblend, model, sv.time);
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
static void VM_SV_skel_get_numbones(prvm_prog_t *prog)
{
	int skeletonindex = (int)PRVM_G_FLOAT(OFS_PARM0) - 1;
	skeleton_t *skeleton;
	PRVM_G_FLOAT(OFS_RETURN) = 0;
	if (skeletonindex < 0 || skeletonindex >= MAX_EDICTS || !(skeleton = prog->skeletons[skeletonindex]))
		return;
	PRVM_G_FLOAT(OFS_RETURN) = skeleton->model->num_bones;
}

// #266 string(float skel, float bonenum) skel_get_bonename = #266; // (FTE_CSQC_SKELETONOBJECTS) returns name of bone (as a tempstring)
static void VM_SV_skel_get_bonename(prvm_prog_t *prog)
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
static void VM_SV_skel_get_boneparent(prvm_prog_t *prog)
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
static void VM_SV_skel_find_bone(prvm_prog_t *prog)
{
	int skeletonindex = (int)PRVM_G_FLOAT(OFS_PARM0) - 1;
	const char *tagname = PRVM_G_STRING(OFS_PARM1);
	skeleton_t *skeleton;
	PRVM_G_FLOAT(OFS_RETURN) = 0;
	if (skeletonindex < 0 || skeletonindex >= MAX_EDICTS || !(skeleton = prog->skeletons[skeletonindex]))
		return;
	PRVM_G_FLOAT(OFS_RETURN) = Mod_Alias_GetTagIndexForName(skeleton->model, 0, tagname) + 1;
}

// #269 vector(float skel, float bonenum) skel_get_bonerel = #269; // (FTE_CSQC_SKELETONOBJECTS) get matrix of bone in skeleton relative to its parent - sets v_forward, v_right, v_up, returns origin (relative to parent bone)
static void VM_SV_skel_get_bonerel(prvm_prog_t *prog)
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
static void VM_SV_skel_get_boneabs(prvm_prog_t *prog)
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
static void VM_SV_skel_set_bone(prvm_prog_t *prog)
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
static void VM_SV_skel_mul_bone(prvm_prog_t *prog)
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
static void VM_SV_skel_mul_bones(prvm_prog_t *prog)
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
static void VM_SV_skel_copybones(prvm_prog_t *prog)
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
static void VM_SV_skel_delete(prvm_prog_t *prog)
{
	int skeletonindex = (int)PRVM_G_FLOAT(OFS_PARM0) - 1;
	skeleton_t *skeleton;
	if (skeletonindex < 0 || skeletonindex >= MAX_EDICTS || !(skeleton = prog->skeletons[skeletonindex]))
		return;
	Mem_Free(skeleton);
	prog->skeletons[skeletonindex] = NULL;
}

// #276 float(float modlindex, string framename) frameforname = #276; // (FTE_CSQC_SKELETONOBJECTS) finds number of a specified frame in the animation, returns -1 if no match found
static void VM_SV_frameforname(prvm_prog_t *prog)
{
	int modelindex = (int)PRVM_G_FLOAT(OFS_PARM0);
	model_t *model = SV_GetModelByIndex(modelindex);
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
static void VM_SV_frameduration(prvm_prog_t *prog)
{
	int modelindex = (int)PRVM_G_FLOAT(OFS_PARM0);
	model_t *model = SV_GetModelByIndex(modelindex);
	int framenum = (int)PRVM_G_FLOAT(OFS_PARM1);
	PRVM_G_FLOAT(OFS_RETURN) = 0;
	if (!model || !model->animscenes || framenum < 0 || framenum >= model->numframes)
		return;
	if (model->animscenes[framenum].framerate)
		PRVM_G_FLOAT(OFS_RETURN) = model->animscenes[framenum].framecount / model->animscenes[framenum].framerate;
}


prvm_builtin_t vm_sv_builtins[] = {
NULL,							// #0 NULL function (not callable) (QUAKE)
VM_makevectors,					// #1 void(vector ang) makevectors (QUAKE)
VM_SV_setorigin,				// #2 void(entity e, vector o) setorigin (QUAKE)
VM_SV_setmodel,					// #3 void(entity e, string m) setmodel (QUAKE)
VM_SV_setsize,					// #4 void(entity e, vector min, vector max) setsize (QUAKE)
NULL,							// #5 void(entity e, vector min, vector max) setabssize (QUAKE)
VM_break,						// #6 void() break (QUAKE)
VM_random,						// #7 float() random (QUAKE)
VM_SV_sound,					// #8 void(entity e, float chan, string samp, float volume[, float atten[, float pitchchange[, float flags]]]) sound (QUAKE)
VM_normalize,					// #9 vector(vector v) normalize (QUAKE)
VM_error,						// #10 void(string e) error (QUAKE)
VM_objerror,					// #11 void(string e) objerror (QUAKE)
VM_vlen,						// #12 float(vector v) vlen (QUAKE)
VM_vectoyaw,					// #13 float(vector v) vectoyaw (QUAKE)
VM_spawn,						// #14 entity() spawn (QUAKE)
VM_remove,						// #15 void(entity e) remove (QUAKE)
VM_SV_traceline,				// #16 void(vector v1, vector v2, float tryents) traceline (QUAKE)
VM_SV_checkclient,				// #17 entity() checkclient (QUAKE)
VM_find,						// #18 entity(entity start, .string fld, string match) find (QUAKE)
VM_SV_precache_sound,			// #19 void(string s) precache_sound (QUAKE)
VM_SV_precache_model,			// #20 void(string s) precache_model (QUAKE)
VM_SV_stuffcmd,					// #21 void(entity client, string s, ...) stuffcmd (QUAKE)
VM_SV_findradius,				// #22 entity(vector org, float rad) findradius (QUAKE)
VM_bprint,						// #23 void(string s, ...) bprint (QUAKE)
VM_SV_sprint,					// #24 void(entity client, string s, ...) sprint (QUAKE)
VM_dprint,						// #25 void(string s, ...) dprint (QUAKE)
VM_ftos,						// #26 string(float f) ftos (QUAKE)
VM_vtos,						// #27 string(vector v) vtos (QUAKE)
VM_coredump,					// #28 void() coredump (QUAKE)
VM_traceon,						// #29 void() traceon (QUAKE)
VM_traceoff,					// #30 void() traceoff (QUAKE)
VM_eprint,						// #31 void(entity e) eprint (QUAKE)
VM_SV_walkmove,					// #32 float(float yaw, float dist) walkmove (QUAKE)
NULL,							// #33 (QUAKE)
VM_SV_droptofloor,				// #34 float() droptofloor (QUAKE)
VM_SV_lightstyle,				// #35 void(float style, string value) lightstyle (QUAKE)
VM_rint,						// #36 float(float v) rint (QUAKE)
VM_floor,						// #37 float(float v) floor (QUAKE)
VM_ceil,						// #38 float(float v) ceil (QUAKE)
NULL,							// #39 (QUAKE)
VM_SV_checkbottom,				// #40 float(entity e) checkbottom (QUAKE)
VM_SV_pointcontents,			// #41 float(vector v) pointcontents (QUAKE)
NULL,							// #42 (QUAKE)
VM_fabs,						// #43 float(float f) fabs (QUAKE)
VM_SV_aim,						// #44 vector(entity e, float speed) aim (QUAKE)
VM_cvar,						// #45 float(string s) cvar (QUAKE)
VM_localcmd,					// #46 void(string s) localcmd (QUAKE)
VM_nextent,						// #47 entity(entity e) nextent (QUAKE)
VM_SV_particle,					// #48 void(vector o, vector d, float color, float count) particle (QUAKE)
VM_changeyaw,					// #49 void() ChangeYaw (QUAKE)
NULL,							// #50 (QUAKE)
VM_vectoangles,					// #51 vector(vector v) vectoangles (QUAKE)
VM_SV_WriteByte,				// #52 void(float to, float f) WriteByte (QUAKE)
VM_SV_WriteChar,				// #53 void(float to, float f) WriteChar (QUAKE)
VM_SV_WriteShort,				// #54 void(float to, float f) WriteShort (QUAKE)
VM_SV_WriteLong,				// #55 void(float to, float f) WriteLong (QUAKE)
VM_SV_WriteCoord,				// #56 void(float to, float f) WriteCoord (QUAKE)
VM_SV_WriteAngle,				// #57 void(float to, float f) WriteAngle (QUAKE)
VM_SV_WriteString,				// #58 void(float to, string s) WriteString (QUAKE)
VM_SV_WriteEntity,				// #59 void(float to, entity e) WriteEntity (QUAKE)
VM_sin,							// #60 float(float f) sin (DP_QC_SINCOSSQRTPOW) (QUAKE)
VM_cos,							// #61 float(float f) cos (DP_QC_SINCOSSQRTPOW) (QUAKE)
VM_sqrt,						// #62 float(float f) sqrt (DP_QC_SINCOSSQRTPOW) (QUAKE)
VM_changepitch,					// #63 void(entity ent) changepitch (DP_QC_CHANGEPITCH) (QUAKE)
VM_SV_tracetoss,				// #64 void(entity e, entity ignore) tracetoss (DP_QC_TRACETOSS) (QUAKE)
VM_etos,						// #65 string(entity ent) etos (DP_QC_ETOS) (QUAKE)
NULL,							// #66 (QUAKE)
VM_SV_MoveToGoal,				// #67 void(float step) movetogoal (QUAKE)
VM_precache_file,				// #68 string(string s) precache_file (QUAKE)
VM_SV_makestatic,				// #69 void(entity e) makestatic (QUAKE)
VM_changelevel,					// #70 void(string s) changelevel (QUAKE)
NULL,							// #71 (QUAKE)
VM_cvar_set,					// #72 void(string var, string val) cvar_set (QUAKE)
VM_SV_centerprint,				// #73 void(entity client, strings) centerprint (QUAKE)
VM_SV_ambientsound,				// #74 void(vector pos, string samp, float vol, float atten) ambientsound (QUAKE)
VM_SV_precache_model,			// #75 string(string s) precache_model2 (QUAKE)
VM_SV_precache_sound,			// #76 string(string s) precache_sound2 (QUAKE)
VM_precache_file,				// #77 string(string s) precache_file2 (QUAKE)
VM_SV_setspawnparms,			// #78 void(entity e) setspawnparms (QUAKE)
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
VM_SV_tracebox,					// #90 void(vector v1, vector min, vector max, vector v2, float nomonsters, entity forent) tracebox (DP_QC_TRACEBOX)
VM_randomvec,					// #91 vector() randomvec (DP_QC_RANDOMVEC)
VM_SV_getlight,					// #92 vector(vector org) getlight (DP_QC_GETLIGHT)
VM_registercvar,				// #93 float(string name, string value) registercvar (DP_REGISTERCVAR)
VM_min,							// #94 float(float a, floats) min (DP_QC_MINMAXBOUND)
VM_max,							// #95 float(float a, floats) max (DP_QC_MINMAXBOUND)
VM_bound,						// #96 float(float minimum, float val, float maximum) bound (DP_QC_MINMAXBOUND)
VM_pow,							// #97 float(float f, float f) pow (DP_QC_SINCOSSQRTPOW)
VM_findfloat,					// #98 entity(entity start, .float fld, float match) findfloat (DP_QC_FINDFLOAT)
VM_checkextension,				// #99 float(string s) checkextension (the basis of the extension system)
// FrikaC and Telejano range  #100-#199
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
NULL,							// #177
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
VM_SV_AddStat,					// #232 void(float index, float type, .void field) SV_AddStat (EXT_CSQC)
NULL,							// #233
NULL,							// #234
NULL,							// #235
NULL,							// #236
NULL,							// #237
NULL,							// #238
NULL,							// #239
VM_SV_checkpvs,					// #240 float(vector viewpos, entity viewee) checkpvs;
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
VM_SV_skel_create,				// #263 float(float modlindex) skel_create = #263; // (DP_SKELETONOBJECTS) create a skeleton (be sure to assign this value into .skeletonindex for use), returns skeleton index (1 or higher) on success, returns 0 on failure  (for example if the modelindex is not skeletal), it is recommended that you create a new skeleton if you change modelindex.
VM_SV_skel_build,				// #264 float(float skel, entity ent, float modlindex, float retainfrac, float firstbone, float lastbone) skel_build = #264; // (DP_SKELETONOBJECTS) blend in a percentage of standard animation, 0 replaces entirely, 1 does nothing, 0.5 blends half, etc, and this only alters the bones in the specified range for which out of bounds values like 0,100000 are safe (uses .frame, .frame2, .frame3, .frame4, .lerpfrac, .lerpfrac3, .lerpfrac4, .frame1time, .frame2time, .frame3time, .frame4time), returns skel on success, 0 on failure
VM_SV_skel_get_numbones,		// #265 float(float skel) skel_get_numbones = #265; // (DP_SKELETONOBJECTS) returns how many bones exist in the created skeleton
VM_SV_skel_get_bonename,		// #266 string(float skel, float bonenum) skel_get_bonename = #266; // (DP_SKELETONOBJECTS) returns name of bone (as a tempstring)
VM_SV_skel_get_boneparent,		// #267 float(float skel, float bonenum) skel_get_boneparent = #267; // (DP_SKELETONOBJECTS) returns parent num for supplied bonenum, -1 if bonenum has no parent or bone does not exist (returned value is always less than bonenum, you can loop on this)
VM_SV_skel_find_bone,			// #268 float(float skel, string tagname) skel_find_bone = #268; // (DP_SKELETONOBJECTS) get number of bone with specified name, 0 on failure, tagindex (bonenum+1) on success, same as using gettagindex on the modelindex
VM_SV_skel_get_bonerel,			// #269 vector(float skel, float bonenum) skel_get_bonerel = #269; // (DP_SKELETONOBJECTS) get matrix of bone in skeleton relative to its parent - sets v_forward, v_right, v_up, returns origin (relative to parent bone)
VM_SV_skel_get_boneabs,			// #270 vector(float skel, float bonenum) skel_get_boneabs = #270; // (DP_SKELETONOBJECTS) get matrix of bone in skeleton in model space - sets v_forward, v_right, v_up, returns origin (relative to entity)
VM_SV_skel_set_bone,			// #271 void(float skel, float bonenum, vector org) skel_set_bone = #271; // (DP_SKELETONOBJECTS) set matrix of bone relative to its parent, reads v_forward, v_right, v_up, takes origin as parameter (relative to parent bone)
VM_SV_skel_mul_bone,			// #272 void(float skel, float bonenum, vector org) skel_mul_bone = #272; // (DP_SKELETONOBJECTS) transform bone matrix (relative to its parent) by the supplied matrix in v_forward, v_right, v_up, takes origin as parameter (relative to parent bone)
VM_SV_skel_mul_bones,			// #273 void(float skel, float startbone, float endbone, vector org) skel_mul_bones = #273; // (DP_SKELETONOBJECTS) transform bone matrices (relative to their parents) by the supplied matrix in v_forward, v_right, v_up, takes origin as parameter (relative to parent bones)
VM_SV_skel_copybones,			// #274 void(float skeldst, float skelsrc, float startbone, float endbone) skel_copybones = #274; // (DP_SKELETONOBJECTS) copy bone matrices (relative to their parents) from one skeleton to another, useful for copying a skeleton to a corpse
VM_SV_skel_delete,				// #275 void(float skel) skel_delete = #275; // (DP_SKELETONOBJECTS) deletes skeleton at the beginning of the next frame (you can add the entity, delete the skeleton, renderscene, and it will still work)
VM_SV_frameforname,				// #276 float(float modlindex, string framename) frameforname = #276; // (DP_SKELETONOBJECTS) finds number of a specified frame in the animation, returns -1 if no match found
VM_SV_frameduration,			// #277 float(float modlindex, float framenum) frameduration = #277; // (DP_SKELETONOBJECTS) returns the intended play time (in seconds) of the specified framegroup, if it does not exist the result is 0, if it is a single frame it may be a small value around 0.1 or 0.
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
NULL,							// #300 void() clearscene (EXT_CSQC)
NULL,							// #301 void(float mask) addentities (EXT_CSQC)
NULL,							// #302 void(entity ent) addentity (EXT_CSQC)
NULL,							// #303 float(float property, ...) setproperty (EXT_CSQC)
NULL,							// #304 void() renderscene (EXT_CSQC)
NULL,							// #305 void(vector org, float radius, vector lightcolours) adddynamiclight (EXT_CSQC)
NULL,							// #306 void(string texturename, float flag[, float is2d, float lines]) R_BeginPolygon
NULL,							// #307 void(vector org, vector texcoords, vector rgb, float alpha) R_PolygonVertex
NULL,							// #308 void() R_EndPolygon
NULL,							// #309
NULL,							// #310 vector (vector v) cs_unproject (EXT_CSQC)
NULL,							// #311 vector (vector v) cs_project (EXT_CSQC)
NULL,							// #312
NULL,							// #313
NULL,							// #314
NULL,							// #315 void(float width, vector pos1, vector pos2, float flag) drawline (EXT_CSQC)
NULL,							// #316 float(string name) iscachedpic (EXT_CSQC)
NULL,							// #317 string(string name, float trywad) precache_pic (EXT_CSQC)
NULL,							// #318 vector(string picname) draw_getimagesize (EXT_CSQC)
NULL,							// #319 void(string name) freepic (EXT_CSQC)
NULL,							// #320 float(vector position, float character, vector scale, vector rgb, float alpha, float flag) drawcharacter (EXT_CSQC)
NULL,							// #321 float(vector position, string text, vector scale, vector rgb, float alpha, float flag) drawstring (EXT_CSQC)
NULL,							// #322 float(vector position, string pic, vector size, vector rgb, float alpha, float flag) drawpic (EXT_CSQC)
NULL,							// #323 float(vector position, vector size, vector rgb, float alpha, float flag) drawfill (EXT_CSQC)
NULL,							// #324 void(float x, float y, float width, float height) drawsetcliparea
NULL,							// #325 void(void) drawresetcliparea
NULL,							// #326
NULL,							// #327
NULL,							// #328
NULL,							// #329
NULL,							// #330 float(float stnum) getstatf (EXT_CSQC)
NULL,							// #331 float(float stnum) getstati (EXT_CSQC)
NULL,							// #332 string(float firststnum) getstats (EXT_CSQC)
VM_SV_setmodelindex,			// #333 void(entity e, float mdlindex) setmodelindex (EXT_CSQC)
VM_SV_modelnameforindex,		// #334 string(float mdlindex) modelnameforindex (EXT_CSQC)
VM_SV_particleeffectnum,		// #335 float(string effectname) particleeffectnum (EXT_CSQC)
VM_SV_trailparticles,			// #336 void(entity ent, float effectnum, vector start, vector end) trailparticles (EXT_CSQC)
VM_SV_pointparticles,			// #337 void(float effectnum, vector origin [, vector dir, float count]) pointparticles (EXT_CSQC)
NULL,							// #338 void(string s, ...) centerprint (EXT_CSQC)
VM_print,						// #339 void(string s, ...) print (EXT_CSQC, DP_SV_PRINT)
NULL,							// #340 string(float keynum) keynumtostring (EXT_CSQC)
NULL,							// #341 float(string keyname) stringtokeynum (EXT_CSQC)
NULL,							// #342 string(float keynum) getkeybind (EXT_CSQC)
NULL,							// #343 void(float usecursor) setcursormode (EXT_CSQC)
NULL,							// #344 vector() getmousepos (EXT_CSQC)
NULL,							// #345 float(float framenum) getinputstate (EXT_CSQC)
NULL,							// #346 void(float sens) setsensitivityscaler (EXT_CSQC)
NULL,							// #347 void() runstandardplayerphysics (EXT_CSQC)
NULL,							// #348 string(float playernum, string keyname) getplayerkeyvalue (EXT_CSQC)
NULL,							// #349 float() isdemo (EXT_CSQC)
VM_isserver,					// #350 float() isserver (EXT_CSQC)
NULL,							// #351 void(vector origin, vector forward, vector right, vector up) SetListener (EXT_CSQC)
VM_SV_registercommand,			// #352 void(string cmdname) registercommand (EXT_CSQC)
VM_wasfreed,					// #353 float(entity ent) wasfreed (EXT_CSQC) (should be availabe on server too)
VM_SV_serverkey,				// #354 string(string key) serverkey (EXT_CSQC)
NULL,							// #355
NULL,							// #356
NULL,							// #357
NULL,							// #358
NULL,							// #359
NULL,							// #360 float() readbyte (EXT_CSQC)
NULL,							// #361 float() readchar (EXT_CSQC)
NULL,							// #362 float() readshort (EXT_CSQC)
NULL,							// #363 float() readlong (EXT_CSQC)
NULL,							// #364 float() readcoord (EXT_CSQC)
NULL,							// #365 float() readangle (EXT_CSQC)
NULL,							// #366 string() readstring (EXT_CSQC)
NULL,							// #367 float() readfloat (EXT_CSQC)
NULL,							// #368
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
VM_SV_copyentity,				// #400 void(entity from, entity to) copyentity (DP_QC_COPYENTITY)
VM_SV_setcolor,					// #401 void(entity ent, float colors) setcolor (DP_QC_SETCOLOR)
VM_findchain,					// #402 entity(.string fld, string match) findchain (DP_QC_FINDCHAIN)
VM_findchainfloat,				// #403 entity(.float fld, float match) findchainfloat (DP_QC_FINDCHAINFLOAT)
VM_SV_effect,					// #404 void(vector org, string modelname, float startframe, float endframe, float framerate) effect (DP_SV_EFFECT)
VM_SV_te_blood,					// #405 void(vector org, vector velocity, float howmany) te_blood (DP_TE_BLOOD)
VM_SV_te_bloodshower,			// #406 void(vector mincorner, vector maxcorner, float explosionspeed, float howmany) te_bloodshower (DP_TE_BLOODSHOWER)
VM_SV_te_explosionrgb,			// #407 void(vector org, vector color) te_explosionrgb (DP_TE_EXPLOSIONRGB)
VM_SV_te_particlecube,			// #408 void(vector mincorner, vector maxcorner, vector vel, float howmany, float color, float gravityflag, float randomveljitter) te_particlecube (DP_TE_PARTICLECUBE)
VM_SV_te_particlerain,			// #409 void(vector mincorner, vector maxcorner, vector vel, float howmany, float color) te_particlerain (DP_TE_PARTICLERAIN)
VM_SV_te_particlesnow,			// #410 void(vector mincorner, vector maxcorner, vector vel, float howmany, float color) te_particlesnow (DP_TE_PARTICLESNOW)
VM_SV_te_spark,					// #411 void(vector org, vector vel, float howmany) te_spark (DP_TE_SPARK)
VM_SV_te_gunshotquad,			// #412 void(vector org) te_gunshotquad (DP_QUADEFFECTS1)
VM_SV_te_spikequad,				// #413 void(vector org) te_spikequad (DP_QUADEFFECTS1)
VM_SV_te_superspikequad,		// #414 void(vector org) te_superspikequad (DP_QUADEFFECTS1)
VM_SV_te_explosionquad,			// #415 void(vector org) te_explosionquad (DP_QUADEFFECTS1)
VM_SV_te_smallflash,			// #416 void(vector org) te_smallflash (DP_TE_SMALLFLASH)
VM_SV_te_customflash,			// #417 void(vector org, float radius, float lifetime, vector color) te_customflash (DP_TE_CUSTOMFLASH)
VM_SV_te_gunshot,				// #418 void(vector org) te_gunshot (DP_TE_STANDARDEFFECTBUILTINS)
VM_SV_te_spike,					// #419 void(vector org) te_spike (DP_TE_STANDARDEFFECTBUILTINS)
VM_SV_te_superspike,			// #420 void(vector org) te_superspike (DP_TE_STANDARDEFFECTBUILTINS)
VM_SV_te_explosion,				// #421 void(vector org) te_explosion (DP_TE_STANDARDEFFECTBUILTINS)
VM_SV_te_tarexplosion,			// #422 void(vector org) te_tarexplosion (DP_TE_STANDARDEFFECTBUILTINS)
VM_SV_te_wizspike,				// #423 void(vector org) te_wizspike (DP_TE_STANDARDEFFECTBUILTINS)
VM_SV_te_knightspike,			// #424 void(vector org) te_knightspike (DP_TE_STANDARDEFFECTBUILTINS)
VM_SV_te_lavasplash,			// #425 void(vector org) te_lavasplash (DP_TE_STANDARDEFFECTBUILTINS)
VM_SV_te_teleport,				// #426 void(vector org) te_teleport (DP_TE_STANDARDEFFECTBUILTINS)
VM_SV_te_explosion2,			// #427 void(vector org, float colorstart, float colorlength) te_explosion2 (DP_TE_STANDARDEFFECTBUILTINS)
VM_SV_te_lightning1,			// #428 void(entity own, vector start, vector end) te_lightning1 (DP_TE_STANDARDEFFECTBUILTINS)
VM_SV_te_lightning2,			// #429 void(entity own, vector start, vector end) te_lightning2 (DP_TE_STANDARDEFFECTBUILTINS)
VM_SV_te_lightning3,			// #430 void(entity own, vector start, vector end) te_lightning3 (DP_TE_STANDARDEFFECTBUILTINS)
VM_SV_te_beam,					// #431 void(entity own, vector start, vector end) te_beam (DP_TE_STANDARDEFFECTBUILTINS)
VM_vectorvectors,				// #432 void(vector dir) vectorvectors (DP_QC_VECTORVECTORS)
VM_SV_te_plasmaburn,			// #433 void(vector org) te_plasmaburn (DP_TE_PLASMABURN)
VM_getsurfacenumpoints,		// #434 float(entity e, float s) getsurfacenumpoints (DP_QC_GETSURFACE)
VM_getsurfacepoint,			// #435 vector(entity e, float s, float n) getsurfacepoint (DP_QC_GETSURFACE)
VM_getsurfacenormal,			// #436 vector(entity e, float s) getsurfacenormal (DP_QC_GETSURFACE)
VM_getsurfacetexture,		// #437 string(entity e, float s) getsurfacetexture (DP_QC_GETSURFACE)
VM_getsurfacenearpoint,		// #438 float(entity e, vector p) getsurfacenearpoint (DP_QC_GETSURFACE)
VM_getsurfaceclippedpoint,	// #439 vector(entity e, float s, vector p) getsurfaceclippedpoint (DP_QC_GETSURFACE)
VM_SV_clientcommand,			// #440 void(entity e, string s) clientcommand (KRIMZON_SV_PARSECLIENTCOMMAND)
VM_tokenize,					// #441 float(string s) tokenize (KRIMZON_SV_PARSECLIENTCOMMAND)
VM_argv,						// #442 string(float n) argv (KRIMZON_SV_PARSECLIENTCOMMAND)
VM_SV_setattachment,			// #443 void(entity e, entity tagentity, string tagname) setattachment (DP_GFX_QUAKE3MODELTAGS)
VM_search_begin,				// #444 float(string pattern, float caseinsensitive, float quiet) search_begin (DP_QC_FS_SEARCH)
VM_search_end,					// #445 void(float handle) search_end (DP_QC_FS_SEARCH)
VM_search_getsize,				// #446 float(float handle) search_getsize (DP_QC_FS_SEARCH)
VM_search_getfilename,			// #447 string(float handle, float num) search_getfilename (DP_QC_FS_SEARCH)
VM_cvar_string,					// #448 string(string s) cvar_string (DP_QC_CVAR_STRING)
VM_findflags,					// #449 entity(entity start, .float fld, float match) findflags (DP_QC_FINDFLAGS)
VM_findchainflags,				// #450 entity(.float fld, float match) findchainflags (DP_QC_FINDCHAINFLAGS)
VM_SV_gettagindex,				// #451 float(entity ent, string tagname) gettagindex (DP_QC_GETTAGINFO)
VM_SV_gettaginfo,				// #452 vector(entity ent, float tagindex) gettaginfo (DP_QC_GETTAGINFO)
VM_SV_dropclient,				// #453 void(entity clent) dropclient (DP_SV_DROPCLIENT)
VM_SV_spawnclient,				// #454 entity() spawnclient (DP_SV_BOTCLIENT)
VM_SV_clienttype,				// #455 float(entity clent) clienttype (DP_SV_BOTCLIENT)
VM_SV_WriteUnterminatedString,	// #456 void(float to, string s) WriteUnterminatedString (DP_SV_WRITEUNTERMINATEDSTRING)
VM_SV_te_flamejet,				// #457 void(vector org, vector vel, float howmany) te_flamejet = #457 (DP_TE_FLAMEJET)
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
NULL,							// #470
VM_asin,						// #471 float(float s) VM_asin (DP_QC_ASINACOSATANATAN2TAN)
VM_acos,						// #472 float(float c) VM_acos (DP_QC_ASINACOSATANATAN2TAN)
VM_atan,						// #473 float(float t) VM_atan (DP_QC_ASINACOSATANATAN2TAN)
VM_atan2,						// #474 float(float c, float s) VM_atan2 (DP_QC_ASINACOSATANATAN2TAN)
VM_tan,							// #475 float(float a) VM_tan (DP_QC_ASINACOSATANATAN2TAN)
VM_strlennocol,					// #476 float(string s) : DRESK - String Length (not counting color codes) (DP_QC_STRINGCOLORFUNCTIONS)
VM_strdecolorize,				// #477 string(string s) : DRESK - Decolorized String (DP_SV_STRINGCOLORFUNCTIONS)
VM_strftime,					// #478 string(float uselocaltime, string format, ...) (DP_QC_STRFTIME)
VM_tokenizebyseparator,			// #479 float(string s) tokenizebyseparator (DP_QC_TOKENIZEBYSEPARATOR)
VM_strtolower,					// #480 string(string s) VM_strtolower (DP_QC_STRING_CASE_FUNCTIONS)
VM_strtoupper,					// #481 string(string s) VM_strtoupper (DP_QC_STRING_CASE_FUNCTIONS)
VM_cvar_defstring,				// #482 string(string s) cvar_defstring (DP_QC_CVAR_DEFSTRING)
VM_SV_pointsound,				// #483 void(vector origin, string sample, float volume, float attenuation) (DP_SV_POINTSOUND)
VM_strreplace,					// #484 string(string search, string replace, string subject) strreplace (DP_QC_STRREPLACE)
VM_strireplace,					// #485 string(string search, string replace, string subject) strireplace (DP_QC_STRREPLACE)
VM_getsurfacepointattribute,// #486 vector(entity e, float s, float n, float a) getsurfacepointattribute = #486;
NULL,							// #487
NULL,							// #488
NULL,							// #489
NULL,							// #490
NULL,							// #491
NULL,							// #492
NULL,							// #493
VM_crc16,						// #494 float(float caseinsensitive, string s, ...) crc16 = #494 (DP_QC_CRC16)
VM_cvar_type,					// #495 float(string name) cvar_type = #495; (DP_QC_CVAR_TYPE)
VM_numentityfields,				// #496 float() numentityfields = #496; (DP_QC_ENTITYDATA)
VM_entityfieldname,				// #497 string(float fieldnum) entityfieldname = #497; (DP_QC_ENTITYDATA)
VM_entityfieldtype,				// #498 float(float fieldnum) entityfieldtype = #498; (DP_QC_ENTITYDATA)
VM_getentityfieldstring,		// #499 string(float fieldnum, entity ent) getentityfieldstring = #499; (DP_QC_ENTITYDATA)
VM_putentityfieldstring,		// #500 float(float fieldnum, entity ent, string s) putentityfieldstring = #500; (DP_QC_ENTITYDATA)
VM_SV_WritePicture,				// #501
NULL,							// #502
VM_whichpack,					// #503 string(string) whichpack = #503;
NULL,							// #504
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
NULL,							// #520
NULL,							// #521
NULL,							// #522
NULL,							// #523
NULL,							// #524
NULL,							// #525
NULL,							// #526
NULL,							// #527
NULL,							// #528
VM_loadfromdata,				// #529
VM_loadfromfile,				// #530
VM_SV_setpause,					// #531 void(float pause) setpause = #531;
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
VM_SV_findbox,					// #566 entity(vector mins, vector maxs) findbox = #566; (DP_QC_FINDBOX)
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
NULL,							// #610
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
VM_SV_getextresponse,			// #624 string getextresponse(void)
NULL,							// #625
NULL,							// #626
VM_sprintf,                     // #627 string sprintf(string format, ...)
VM_getsurfacenumtriangles,		// #628 float(entity e, float s) getsurfacenumpoints (DP_QC_GETSURFACETRIANGLE)
VM_getsurfacetriangle,			// #629 vector(entity e, float s, float n) getsurfacepoint (DP_QC_GETSURFACETRIANGLE)
NULL,							// #630
NULL,							// #631
NULL,							// #632
NULL,							// #633
NULL,							// #634
NULL,							// #635
NULL,							// #636
NULL,							// #637
NULL,							// #638
VM_digest_hex,						// #639
NULL,							// #640
NULL,							// #641
VM_coverage,						// #642
NULL,							// #643
};

const int vm_sv_numbuiltins = sizeof(vm_sv_builtins) / sizeof(prvm_builtin_t);

void SVVM_init_cmd(prvm_prog_t *prog)
{
	VM_Cmd_Init(prog);
}

void SVVM_reset_cmd(prvm_prog_t *prog)
{
	World_End(&sv.world);
	VM_Cmd_Reset(prog);
}
