#include "quakedef.h"
#include "particles.h"

#ifdef Q2CLIENT
#include "shader.h"

//q2pro's framerate scaling runs the entire server at a higher rate (including gamecode).
//this allows lower latency on player movements without breaking things too much
//animations are still assumed to run at 10fps, so those need some fixup too
//events are keyed to not renew twice within the same 10fps window, unless the entity was actually updated.

extern cvar_t r_drawviewmodel;

extern cvar_t cl_nopred;
typedef enum
{
	Q2EV_NONE,
	Q2EV_ITEM_RESPAWN,
	Q2EV_FOOTSTEP,
	Q2EV_FALLSHORT,
	Q2EV_FALL,
	Q2EV_FALLFAR,
	Q2EV_PLAYER_TELEPORT,
	Q2EV_OTHER_TELEPORT,
	Q2EXEV_OTHER_FOOTSTEP,
	Q2EXEV_LADDER_STEP,
} q2entity_event_t;

#define Q2PMF_NO_PREDICTION	64	// temporarily disables prediction (used for grappling hook)

float LerpAngle (float a2, float a1, float frac)
{
	if (a1 - a2 > 180)
		a1 -= 360;
	if (a1 - a2 < -180)
		a1 += 360;
	return a2 + frac * (a1 - a2);
}

// entity_state_t->effects
// Effects are things handled on the client side (lights, particles, frame animations)
// that happen constantly on the given entity.
// An entity that has effects will be sent to the client
// even if it has a zero index model.
#define	Q2EF_ROTATE				0x00000001		// rotate (bonus items)
#define	Q2EF_GIB				0x00000002		// leave a trail
#define	Q2EF_BLASTER			0x00000008		// redlight + trail
#define	Q2EF_ROCKET				0x00000010		// redlight + trail
#define	Q2EF_GRENADE			0x00000020
#define	Q2EF_HYPERBLASTER		0x00000040
#define	Q2EF_BFG				0x00000080
#define Q2EF_COLOR_SHELL		0x00000100
#define Q2EF_POWERSCREEN		0x00000200
#define	Q2EF_ANIM01				0x00000400		// automatically cycle between frames 0 and 1 at 2 hz
#define	Q2EF_ANIM23				0x00000800		// automatically cycle between frames 2 and 3 at 2 hz
#define Q2EF_ANIM_ALL			0x00001000		// automatically cycle through all frames at 2hz
#define Q2EF_ANIM_ALLFAST		0x00002000		// automatically cycle through all frames at 10hz
#define	Q2EF_FLIES				0x00004000
#define	Q2EF_QUAD				0x00008000
#define	Q2EF_PENT				0x00010000
#define	Q2EF_TELEPORTER			0x00020000		// particle fountain
#define Q2EF_FLAG1				0x00040000
#define Q2EF_FLAG2				0x00080000
// RAFAEL
#define Q2EF_IONRIPPER			0x00100000
#define Q2EF_GREENGIB			0x00200000
#define	Q2EF_BLUEHYPERBLASTER	0x00400000
#define Q2EF_SPINNINGLIGHTS		0x00800000
#define Q2EF_PLASMA				0x01000000
#define Q2EF_TRAP				0x02000000

//ROGUE
#define Q2EF_TRACKER			0x04000000
#define	Q2EF_DOUBLE				0x08000000
#define	Q2EF_SPHERETRANS		0x10000000
#define Q2EF_TAGTRAIL			0x20000000
#define Q2EF_HALF_DAMAGE		0x40000000
#define Q2EF_TRACKERTRAIL		0x80000000
//ROGUE




#define Q2MAX_STATS	32

typedef struct q2centity_s
{
	entity_state_t	baseline;		// delta from this if not from a previous frame
	entity_state_t	current;
	entity_state_t	prev;			// will always be valid, but might just be a copy of current

	int			serverframe;		// if not current, this ent isn't in the frame

	trailkey_t trailstate;
	trailkey_t emitstate;
//	float		trailcount;			// for diminishing grenade trails
	vec3_t		lerp_origin;		// for trails (variable hz)
	int			oldframe;
	float		oldframetime;

	float		fly_stoptime;
} q2centity_t;

static void Q2S_StartSound(vec3_t origin, int entnum, int entchannel, sfx_t *sfx, float fvol, float attenuation, float delay)
{
	S_StartSound(entnum, entchannel, sfx, origin, NULL, fvol, attenuation, -delay, 0, 0);
}
sfx_t *S_PrecacheSexedSound(int entnum, const char *soundname)
{
	if (soundname[0] == '*')
	{	//a 'sexed' sound
		if (entnum > 0 && entnum <= MAX_CLIENTS)
		{
			char *model = InfoBuf_ValueForKey(&cl.players[entnum-1].userinfo, "skin");
			char *skin;
			skin = strchr(model, '/');
			if (skin)
				*skin = '\0';
			if (*model)
			{
				sfx_t *sfx = S_PrecacheSound(va("players/%s/%s", model, soundname+1));
				if (sfx && sfx->loadstate != SLS_FAILED)	//warning: the sound might still be loading (and later fail).
					return sfx;
			}
		}
		return S_PrecacheSound(va("players/male/%s", soundname+1));
	}
	return S_PrecacheSound(soundname);
}


void CLQ2_EntityEvent(entity_state_t *es)
{
	switch (es->u.q2.event)
	{
	case Q2EV_NONE:
	case Q2EV_OTHER_TELEPORT:	//inihibits interpolation. not an event in itself.
		break;
	case Q2EV_ITEM_RESPAWN:
		pe->RunParticleEffectState(es->origin, NULL, 1, pt_q2[Q2PT_RESPAWN], NULL);
		break;
	case Q2EV_PLAYER_TELEPORT:
		pe->RunParticleEffectState(es->origin, NULL, 1, pt_q2[Q2PT_PLAYER_TELEPORT], NULL);
		break;
	case Q2EV_FOOTSTEP:
		pe->RunParticleEffectState(es->origin, NULL, 1, pt_q2[Q2PT_FOOTSTEP], NULL);
		break;
	case Q2EV_FALLSHORT:
		Q2S_StartSound (NULL, es->number, CHAN_AUTO, S_PrecacheSound ("player/land1.wav"), 1, ATTN_NORM, 0);
		break;
	case Q2EV_FALL:
		Q2S_StartSound (NULL, es->number, CHAN_AUTO, S_PrecacheSexedSound (es->number, "*fall2.wav"), 1, ATTN_NORM, 0);
		break;
	case Q2EV_FALLFAR:
		Q2S_StartSound (NULL, es->number, CHAN_AUTO, S_PrecacheSexedSound (es->number, "*fall1.wav"), 1, ATTN_NORM, 0);
		break;
	case Q2EXEV_OTHER_FOOTSTEP:
		pe->RunParticleEffectState(es->origin, NULL, 1, pt_q2[Q2PT_OTHER_FOOTSTEP], NULL);
		break;
	case Q2EXEV_LADDER_STEP:
		pe->RunParticleEffectState(es->origin, NULL, 1, pt_q2[Q2PT_LADDER_STEP], NULL);
		break;
	default:
		Con_Printf("event %u not supported\n", es->u.q2.event);
		break;
	}
};
void CLQ2_TeleporterParticles(entity_state_t *es){};

/*these are emissive effects (ie: emitted each frame), but they're also mutually exclusive, so sharing emitstate is fine*/
void CLQ2_Tracker_Shell(q2centity_t *ent, vec3_t org)
{
	P_EmitEffect (org, NULL, 0, pt_q2[Q2PT_TRACKERSHELL], &(ent->emitstate));
};
void CLQ2_BfgParticles(q2centity_t *ent, vec3_t org)
{
	P_EmitEffect (org, NULL, 0, pt_q2[Q2PT_BFGPARTICLES], &(ent->emitstate));
};
void CLQ2_FlyEffect(q2centity_t *ent, vec3_t org)
{
	float starttime, n;
	float cltime = cl.time;
	int count;
	if (cl.paused)
		return;

	//q2 ramps up to 162 within the first 20 secs, sits at 162 for the next 20, then ramps back down for the 20 after that.
	//I have no idea how pvs issues are handled.

	if (ent->fly_stoptime < cltime)
	{
		starttime = cltime;
		ent->fly_stoptime = cltime + 60;
	}
	else
	{
		starttime = ent->fly_stoptime - 60;
	}

	n = cltime - starttime;
	if (n < 20)
		count = n * 162 / 20.0;
	else
	{
		n = ent->fly_stoptime - cltime;
		if (n < 20)
			count = n * 162 / 20.0;
		else
			count = 162;
	}
	if (count < 0)
		return;

	//these are assumed to be spawned anew 
	pe->RunParticleEffectState(org, NULL, count, pt_q2[Q2PT_FLIES], &(ent->emitstate));
};


#define MAX_Q2EDICTS 1024
#define	MAX_PARSE_ENTITIES	1024


static q2centity_t cl_entities[MAX_Q2EDICTS];
entity_state_t	clq2_parse_entities[MAX_PARSE_ENTITIES];

void CL_SmokeAndFlash(vec3_t origin);

void CL_GetNumberedEntityInfo (int num, float *org, float *ang)
{
	q2centity_t	*ent;

	if (num < 0 || num >= MAX_Q2EDICTS)
		Host_EndGame ("CL_GetNumberedEntityInfo: bad ent");
	ent = &cl_entities[num];

	if (org)
		VectorCopy (ent->current.origin, org);
	if (ang)
		VectorCopy (ent->current.angles, ang);


	// FIXME: bmodel issues...
}

#ifdef Q2SERVER
void CLQ2_WriteDemoBaselines(sizebuf_t *buf)
{
	int i;
	q2entity_state_t	nullstate = {0};
	for (i = 0; i < MAX_Q2EDICTS; i++)
	{
		q2entity_state_t	es;
		entity_state_t		*base = &cl_entities[i].baseline;
		if (!base->modelindex)
			continue;

		//I brought these copies on myself...
		es.number = i;
		VectorCopy(base->origin, es.origin);
		VectorCopy(base->angles, es.angles);
		VectorCopy(base->u.q2.old_origin, es.old_origin);
		es.modelindex = base->modelindex;
		es.modelindex2 = base->modelindex2;
		es.modelindex3 = base->u.q2.modelindex3;
		es.modelindex4 = base->u.q2.modelindex4;
		es.frame = base->frame;
		es.skinnum = base->skinnum;
		es.effects = base->effects;
		es.renderfx = base->u.q2.renderfx;
		es.solid = base->solidsize;
		es.sound = base->u.q2.sound;
		es.event = base->u.q2.event;

		if (buf->cursize > buf->maxsize/2)
		{
			CL_WriteRecordQ2DemoMessage (buf);
			SZ_Clear (buf);
		}

		MSG_WriteByte (buf, svcq2_spawnbaseline);
		MSGQ2_WriteDeltaEntity(&nullstate, &es, buf, true, true, cls.protocol_q2==PROTOCOL_VERSION_Q2EX);
	}
}
#endif

void CLQ2_ClearState(void)
{
	int i;
	memset(cl_entities, 0, sizeof(cl_entities));
	for (i = 0; i < MAX_Q2EDICTS; i++)
		cl_entities[i].baseline = nullentitystate;
}

#include "q2m_flash.c"
void CLQ2_RunMuzzleFlash2 (int ent, int flash_number)
{
	vec3_t		origin;
	dlight_t	*dl;
	vec3_t		forward, right, up;
	char		soundname[64];
	int ef;

	if (flash_number < 0 || flash_number >= sizeof(monster_flash_offset)/sizeof(monster_flash_offset[0]))
		return;

	// locate the origin
	AngleVectors (cl_entities[ent].current.angles, forward, right, up);
	origin[0] = cl_entities[ent].current.origin[0] + forward[0] * monster_flash_offset[flash_number].offset[0] + right[0] * monster_flash_offset[flash_number].offset[1];
	origin[1] = cl_entities[ent].current.origin[1] + forward[1] * monster_flash_offset[flash_number].offset[0] + right[1] * monster_flash_offset[flash_number].offset[1];
	origin[2] = cl_entities[ent].current.origin[2] + forward[2] * monster_flash_offset[flash_number].offset[0] + right[2] * monster_flash_offset[flash_number].offset[1] + monster_flash_offset[flash_number].offset[2];

	ef = P_FindParticleType(monster_flash_offset[flash_number].name);
	if (ef != P_INVALID)
	{
		P_RunParticleEffectType(origin, NULL, 1, ef);
		return;
	}

	//the rest of the function is legacy code.

	dl = CL_AllocDlight (ent);
	VectorCopy (origin,  dl->origin);
	dl->radius = 200 + (rand()&31);
//	dl->minlight = 32;
	dl->die = cl.time + 0.1;

	switch (flash_number)
	{
	case Q2MZ2_INFANTRY_MACHINEGUN_1:
	case Q2MZ2_INFANTRY_MACHINEGUN_2:
	case Q2MZ2_INFANTRY_MACHINEGUN_3:
	case Q2MZ2_INFANTRY_MACHINEGUN_4:
	case Q2MZ2_INFANTRY_MACHINEGUN_5:
	case Q2MZ2_INFANTRY_MACHINEGUN_6:
	case Q2MZ2_INFANTRY_MACHINEGUN_7:
	case Q2MZ2_INFANTRY_MACHINEGUN_8:
	case Q2MZ2_INFANTRY_MACHINEGUN_9:
	case Q2MZ2_INFANTRY_MACHINEGUN_10:
	case Q2MZ2_INFANTRY_MACHINEGUN_11:
	case Q2MZ2_INFANTRY_MACHINEGUN_12:
	case Q2MZ2_INFANTRY_MACHINEGUN_13:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		P_RunParticleEffect (origin, vec3_origin, 0, 40);
		CL_SmokeAndFlash(origin);
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("infantry/infatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_SOLDIER_MACHINEGUN_1:
	case Q2MZ2_SOLDIER_MACHINEGUN_2:
	case Q2MZ2_SOLDIER_MACHINEGUN_3:
	case Q2MZ2_SOLDIER_MACHINEGUN_4:
	case Q2MZ2_SOLDIER_MACHINEGUN_5:
	case Q2MZ2_SOLDIER_MACHINEGUN_6:
	case Q2MZ2_SOLDIER_MACHINEGUN_7:
	case Q2MZ2_SOLDIER_MACHINEGUN_8:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		P_RunParticleEffect (origin, vec3_origin, 0, 40);
		CL_SmokeAndFlash(origin);
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("soldier/solatck3.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_GUNNER_MACHINEGUN_1:
	case Q2MZ2_GUNNER_MACHINEGUN_2:
	case Q2MZ2_GUNNER_MACHINEGUN_3:
	case Q2MZ2_GUNNER_MACHINEGUN_4:
	case Q2MZ2_GUNNER_MACHINEGUN_5:
	case Q2MZ2_GUNNER_MACHINEGUN_6:
	case Q2MZ2_GUNNER_MACHINEGUN_7:
	case Q2MZ2_GUNNER_MACHINEGUN_8:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		P_RunParticleEffect (origin, vec3_origin, 0, 40);
		CL_SmokeAndFlash(origin);
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("gunner/gunatck2.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_ACTOR_MACHINEGUN_1:
	case Q2MZ2_SUPERTANK_MACHINEGUN_1:
	case Q2MZ2_SUPERTANK_MACHINEGUN_2:
	case Q2MZ2_SUPERTANK_MACHINEGUN_3:
	case Q2MZ2_SUPERTANK_MACHINEGUN_4:
	case Q2MZ2_SUPERTANK_MACHINEGUN_5:
	case Q2MZ2_SUPERTANK_MACHINEGUN_6:
	case Q2MZ2_TURRET_MACHINEGUN:			// PGM
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;

		P_RunParticleEffect (origin, vec3_origin, 0, 40);
		CL_SmokeAndFlash(origin);
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("infantry/infatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_BOSS2_MACHINEGUN_L1:
	case Q2MZ2_BOSS2_MACHINEGUN_L2:
	case Q2MZ2_BOSS2_MACHINEGUN_L3:
	case Q2MZ2_BOSS2_MACHINEGUN_L4:
	case Q2MZ2_BOSS2_MACHINEGUN_L5:
	case Q2MZ2_CARRIER_MACHINEGUN_L1:		// PMM
	case Q2MZ2_CARRIER_MACHINEGUN_L2:		// PMM
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;

		P_RunParticleEffect (origin, vec3_origin, 0, 40);
		CL_SmokeAndFlash(origin);
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("infantry/infatck1.wav"), 1, ATTN_NONE, 0);
		break;

	case Q2MZ2_SOLDIER_BLASTER_1:
	case Q2MZ2_SOLDIER_BLASTER_2:
	case Q2MZ2_SOLDIER_BLASTER_3:
	case Q2MZ2_SOLDIER_BLASTER_4:
	case Q2MZ2_SOLDIER_BLASTER_5:
	case Q2MZ2_SOLDIER_BLASTER_6:
	case Q2MZ2_SOLDIER_BLASTER_7:
	case Q2MZ2_SOLDIER_BLASTER_8:
	case Q2MZ2_TURRET_BLASTER:			// PGM
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("soldier/solatck2.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_FLYER_BLASTER_1:
	case Q2MZ2_FLYER_BLASTER_2:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("flyer/flyatck3.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_MEDIC_BLASTER_1:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("medic/medatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_HOVER_BLASTER_1:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("hover/hovatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_FLOAT_BLASTER_1:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("floater/fltatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_SOLDIER_SHOTGUN_1:
	case Q2MZ2_SOLDIER_SHOTGUN_2:
	case Q2MZ2_SOLDIER_SHOTGUN_3:
	case Q2MZ2_SOLDIER_SHOTGUN_4:
	case Q2MZ2_SOLDIER_SHOTGUN_5:
	case Q2MZ2_SOLDIER_SHOTGUN_6:
	case Q2MZ2_SOLDIER_SHOTGUN_7:
	case Q2MZ2_SOLDIER_SHOTGUN_8:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		CL_SmokeAndFlash(origin);
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("soldier/solatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_TANK_BLASTER_1:
	case Q2MZ2_TANK_BLASTER_2:
	case Q2MZ2_TANK_BLASTER_3:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("tank/tnkatck3.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_TANK_MACHINEGUN_1:
	case Q2MZ2_TANK_MACHINEGUN_2:
	case Q2MZ2_TANK_MACHINEGUN_3:
	case Q2MZ2_TANK_MACHINEGUN_4:
	case Q2MZ2_TANK_MACHINEGUN_5:
	case Q2MZ2_TANK_MACHINEGUN_6:
	case Q2MZ2_TANK_MACHINEGUN_7:
	case Q2MZ2_TANK_MACHINEGUN_8:
	case Q2MZ2_TANK_MACHINEGUN_9:
	case Q2MZ2_TANK_MACHINEGUN_10:
	case Q2MZ2_TANK_MACHINEGUN_11:
	case Q2MZ2_TANK_MACHINEGUN_12:
	case Q2MZ2_TANK_MACHINEGUN_13:
	case Q2MZ2_TANK_MACHINEGUN_14:
	case Q2MZ2_TANK_MACHINEGUN_15:
	case Q2MZ2_TANK_MACHINEGUN_16:
	case Q2MZ2_TANK_MACHINEGUN_17:
	case Q2MZ2_TANK_MACHINEGUN_18:
	case Q2MZ2_TANK_MACHINEGUN_19:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		P_RunParticleEffect (origin, vec3_origin, 0, 40);
		CL_SmokeAndFlash(origin);
		snprintf(soundname, sizeof(soundname), "tank/tnkatk2%c.wav", 'a' + rand() % 5);
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound(soundname), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_CHICK_ROCKET_1:
	case Q2MZ2_TURRET_ROCKET:			// PGM
		dl->color[0] = 1;dl->color[1] = 0.5;dl->color[2] = 0.2;
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("chick/chkatck2.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_TANK_ROCKET_1:
	case Q2MZ2_TANK_ROCKET_2:
	case Q2MZ2_TANK_ROCKET_3:
		dl->color[0] = 1;dl->color[1] = 0.5;dl->color[2] = 0.2;
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("tank/tnkatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_SUPERTANK_ROCKET_1:
	case Q2MZ2_SUPERTANK_ROCKET_2:
	case Q2MZ2_SUPERTANK_ROCKET_3:
	case Q2MZ2_BOSS2_ROCKET_1:
	case Q2MZ2_BOSS2_ROCKET_2:
	case Q2MZ2_BOSS2_ROCKET_3:
	case Q2MZ2_BOSS2_ROCKET_4:
	case Q2MZ2_CARRIER_ROCKET_1:
//	case Q2MZ2_CARRIER_ROCKET_2:
//	case Q2MZ2_CARRIER_ROCKET_3:
//	case Q2MZ2_CARRIER_ROCKET_4:
		dl->color[0] = 1;dl->color[1] = 0.5;dl->color[2] = 0.2;
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("tank/rocket.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_GUNNER_GRENADE_1:
	case Q2MZ2_GUNNER_GRENADE_2:
	case Q2MZ2_GUNNER_GRENADE_3:
	case Q2MZ2_GUNNER_GRENADE_4:
		dl->color[0] = 1;dl->color[1] = 0.5;dl->color[2] = 0;
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("gunner/gunatck3.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_GLADIATOR_RAILGUN_1:
	// PMM
	case Q2MZ2_CARRIER_RAILGUN:
	case Q2MZ2_WIDOW_RAIL:
	// pmm
		dl->color[0] = 0.5;dl->color[1] = 0.5;dl->color[2] = 1.0;
		break;

// --- Xian's shit starts ---
	case Q2MZ2_MAKRON_BFG:
		dl->color[0] = 0.5;dl->color[1] = 1 ;dl->color[2] = 0.5;
		//Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_RegisterSound("makron/bfg_fire.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_MAKRON_BLASTER_1:
	case Q2MZ2_MAKRON_BLASTER_2:
	case Q2MZ2_MAKRON_BLASTER_3:
	case Q2MZ2_MAKRON_BLASTER_4:
	case Q2MZ2_MAKRON_BLASTER_5:
	case Q2MZ2_MAKRON_BLASTER_6:
	case Q2MZ2_MAKRON_BLASTER_7:
	case Q2MZ2_MAKRON_BLASTER_8:
	case Q2MZ2_MAKRON_BLASTER_9:
	case Q2MZ2_MAKRON_BLASTER_10:
	case Q2MZ2_MAKRON_BLASTER_11:
	case Q2MZ2_MAKRON_BLASTER_12:
	case Q2MZ2_MAKRON_BLASTER_13:
	case Q2MZ2_MAKRON_BLASTER_14:
	case Q2MZ2_MAKRON_BLASTER_15:
	case Q2MZ2_MAKRON_BLASTER_16:
	case Q2MZ2_MAKRON_BLASTER_17:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("makron/blaster.wav"), 1, ATTN_NORM, 0);
		break;
	
	case Q2MZ2_JORG_MACHINEGUN_L1:
	case Q2MZ2_JORG_MACHINEGUN_L2:
	case Q2MZ2_JORG_MACHINEGUN_L3:
	case Q2MZ2_JORG_MACHINEGUN_L4:
	case Q2MZ2_JORG_MACHINEGUN_L5:
	case Q2MZ2_JORG_MACHINEGUN_L6:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		P_RunParticleEffect (origin, vec3_origin, 0, 40);
		CL_SmokeAndFlash(origin);
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("boss3/xfire.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_JORG_MACHINEGUN_R1:
	case Q2MZ2_JORG_MACHINEGUN_R2:
	case Q2MZ2_JORG_MACHINEGUN_R3:
	case Q2MZ2_JORG_MACHINEGUN_R4:
	case Q2MZ2_JORG_MACHINEGUN_R5:
	case Q2MZ2_JORG_MACHINEGUN_R6:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		P_RunParticleEffect (origin, vec3_origin, 0, 40);
		CL_SmokeAndFlash(origin);
		break;

	case Q2MZ2_JORG_BFG_1:
		dl->color[0] = 0.5;dl->color[1] = 1 ;dl->color[2] = 0.5;
		break;

	case Q2MZ2_BOSS2_MACHINEGUN_R1:
	case Q2MZ2_BOSS2_MACHINEGUN_R2:
	case Q2MZ2_BOSS2_MACHINEGUN_R3:
	case Q2MZ2_BOSS2_MACHINEGUN_R4:
	case Q2MZ2_BOSS2_MACHINEGUN_R5:
	case Q2MZ2_CARRIER_MACHINEGUN_R1:			// PMM
	case Q2MZ2_CARRIER_MACHINEGUN_R2:			// PMM

		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;

		P_RunParticleEffect (origin, vec3_origin, 0, 40);
		CL_SmokeAndFlash(origin);
		break;

// ======
// ROGUE
	case Q2MZ2_STALKER_BLASTER:
	case Q2MZ2_DAEDALUS_BLASTER:
	case Q2MZ2_MEDIC_BLASTER_2:
	case Q2MZ2_WIDOW_BLASTER:
	case Q2MZ2_WIDOW_BLASTER_SWEEP1:
	case Q2MZ2_WIDOW_BLASTER_SWEEP2:
	case Q2MZ2_WIDOW_BLASTER_SWEEP3:
	case Q2MZ2_WIDOW_BLASTER_SWEEP4:
	case Q2MZ2_WIDOW_BLASTER_SWEEP5:
	case Q2MZ2_WIDOW_BLASTER_SWEEP6:
	case Q2MZ2_WIDOW_BLASTER_SWEEP7:
	case Q2MZ2_WIDOW_BLASTER_SWEEP8:
	case Q2MZ2_WIDOW_BLASTER_SWEEP9:
	case Q2MZ2_WIDOW_BLASTER_100:
	case Q2MZ2_WIDOW_BLASTER_90:
	case Q2MZ2_WIDOW_BLASTER_80:
	case Q2MZ2_WIDOW_BLASTER_70:
	case Q2MZ2_WIDOW_BLASTER_60:
	case Q2MZ2_WIDOW_BLASTER_50:
	case Q2MZ2_WIDOW_BLASTER_40:
	case Q2MZ2_WIDOW_BLASTER_30:
	case Q2MZ2_WIDOW_BLASTER_20:
	case Q2MZ2_WIDOW_BLASTER_10:
	case Q2MZ2_WIDOW_BLASTER_0:
	case Q2MZ2_WIDOW_BLASTER_10L:
	case Q2MZ2_WIDOW_BLASTER_20L:
	case Q2MZ2_WIDOW_BLASTER_30L:
	case Q2MZ2_WIDOW_BLASTER_40L:
	case Q2MZ2_WIDOW_BLASTER_50L:
	case Q2MZ2_WIDOW_BLASTER_60L:
	case Q2MZ2_WIDOW_BLASTER_70L:
	case Q2MZ2_WIDOW_RUN_1:
	case Q2MZ2_WIDOW_RUN_2:
	case Q2MZ2_WIDOW_RUN_3:
	case Q2MZ2_WIDOW_RUN_4:
	case Q2MZ2_WIDOW_RUN_5:
	case Q2MZ2_WIDOW_RUN_6:
	case Q2MZ2_WIDOW_RUN_7:
	case Q2MZ2_WIDOW_RUN_8:
		dl->color[0] = 0;dl->color[1] = 1;dl->color[2] = 0;
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("tank/tnkatck3.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_WIDOW_DISRUPTOR:
		dl->color[0] = -1;dl->color[1] = -1;dl->color[2] = -1;
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("weapons/disint2.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_WIDOW_PLASMABEAM:
	case Q2MZ2_WIDOW2_BEAMER_1:
	case Q2MZ2_WIDOW2_BEAMER_2:
	case Q2MZ2_WIDOW2_BEAMER_3:
	case Q2MZ2_WIDOW2_BEAMER_4:
	case Q2MZ2_WIDOW2_BEAMER_5:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_1:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_2:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_3:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_4:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_5:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_6:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_7:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_8:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_9:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_10:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_11:
		dl->radius = 300 + (rand()&100);
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		dl->die = cl.time + 200;
		break;
// ROGUE
// ======

// --- Xian's shit ends ---

  //hmm... he must take AGES on the loo.... :p

	default:
		Con_Printf(CON_WARNING"CLQ2_RunMuzzleFlash2: %i not implemented\n", flash_number);
		break;
	}
}

void CLQ2_ParseMuzzleFlash (void)
{
	vec3_t		fv, rv, dummy;
	dlight_t	*dl;
	int			i, weapon;
	vec3_t		org, ang;
	int			silenced;
	float		volume;
	char		soundname[64];

	i = (unsigned short)(short)MSG_ReadShort ();
	if (i < 1 || i >= Q2MAX_EDICTS)
		Host_Error ("CL_ParseMuzzleFlash: bad entity");

	weapon = MSG_ReadByte ();
	silenced = weapon & Q2MZ_SILENCED;
	weapon &= ~Q2MZ_SILENCED;

	CL_GetNumberedEntityInfo(i, org, ang);

	dl = CL_AllocDlight (i);
	VectorCopy (org,  dl->origin);
	AngleVectors (ang, fv, rv, dummy);
	VectorMA (dl->origin, 18, fv, dl->origin);
	VectorMA (dl->origin, 16, rv, dl->origin);
	if (silenced)
		dl->radius = 100 + (rand()&31);
	else
		dl->radius = 200 + (rand()&31);
	dl->minlight = 32;
	dl->die = cl.time+0.05; //+ 0.1;
	dl->decay = 1;

	dl->channelfade[0] = 2;
	dl->channelfade[1] = 2;
	dl->channelfade[2] = 2;

	if (silenced)
		volume = 0.2;
	else
		volume = 1;


	switch (weapon)
	{
	case Q2MZ_BLASTER:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/blastf1a.wav"), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_BLUEHYPERBLASTER:
		dl->color[0] = 0;dl->color[1] = 0;dl->color[2] = 1;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/hyprbf1a.wav"), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_HYPERBLASTER:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/hyprbf1a.wav"), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_MACHINEGUN:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		Q_snprintfz(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound(soundname), volume, ATTN_NORM, 0);
		break;

	case Q2MZ_SHOTGUN:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/shotgf1b.wav"), volume, ATTN_NORM, 0);
		Q2S_StartSound (NULL, i, CHAN_AUTO,   S_PrecacheSound("weapons/shotgr1b.wav"), volume, ATTN_NORM, 0.1);
		break;
	case Q2MZ_SSHOTGUN:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/sshotf1b.wav"), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_CHAINGUN1:
		dl->radius = 200 + (rand()&31);
		dl->color[0] = 1;dl->color[1] = 0.25;dl->color[2] = 0;
		Q_snprintfz(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound(soundname), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_CHAINGUN2:
		dl->radius = 225 + (rand()&31);
		dl->color[0] = 1;dl->color[1] = 0.5;dl->color[2] = 0;
		dl->die = cl.time  + 0.1;	// long delay
		Q_snprintfz(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound(soundname), volume, ATTN_NORM, 0);
		Q_snprintfz(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		Q2S_StartSound (NULL, i, CHAN_AUTO, S_PrecacheSound(soundname), volume, ATTN_NORM, 0.05);
		break;
	case Q2MZ_CHAINGUN3:
		dl->radius = 250 + (rand()&31);
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		dl->die = cl.time  + 0.1;	// long delay
		Q_snprintfz(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound(soundname), volume, ATTN_NORM, 0);
		Q_snprintfz(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		Q2S_StartSound (NULL, i, CHAN_AUTO, S_PrecacheSound(soundname), volume, ATTN_NORM, 0.033);
		Q_snprintfz(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		Q2S_StartSound (NULL, i, CHAN_AUTO, S_PrecacheSound(soundname), volume, ATTN_NORM, 0.066);
		break;

	case Q2MZ_RAILGUN:
		dl->color[0] = 0.5;dl->color[1] = 0.5;dl->color[2] = 1;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/railgf1a.wav"), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_ROCKET:
		dl->color[0] = 1;dl->color[1] = 0.5;dl->color[2] = 0.2;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/rocklf1a.wav"), volume, ATTN_NORM, 0);
		Q2S_StartSound (NULL, i, CHAN_AUTO,   S_PrecacheSound("weapons/rocklr1b.wav"), volume, ATTN_NORM, 0.1);
		break;
	case Q2MZ_GRENADE:
		dl->color[0] = 1;dl->color[1] = 0.5;dl->color[2] = 0;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/grenlf1a.wav"), volume, ATTN_NORM, 0);
		Q2S_StartSound (NULL, i, CHAN_AUTO,   S_PrecacheSound("weapons/grenlr1b.wav"), volume, ATTN_NORM, 0.1);
		break;
	case Q2MZ_BFG:
		dl->color[0] = 0;dl->color[1] = 1;dl->color[2] = 0;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/bfg__f1y.wav"), volume, ATTN_NORM, 0);
		break;

	case Q2MZ_LOGIN:
		dl->color[0] = 0;dl->color[1] = 1; dl->color[2] = 0;
		dl->die = cl.time + 1.0;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/grenlf1a.wav"), 1, ATTN_NORM, 0);
//		CL_LogoutEffect (pl->current.origin, weapon);
		break;
	case Q2MZ_LOGOUT:
		dl->color[0] = 1;dl->color[1] = 0; dl->color[2] = 0;
		dl->die = cl.time + 1.0;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/grenlf1a.wav"), 1, ATTN_NORM, 0);
//		CL_LogoutEffect (pl->current.origin, weapon);
		break;
	case Q2MZ_RESPAWN:
		dl->color[0] = 1;dl->color[1] = 1; dl->color[2] = 0;
		dl->die = cl.time + 1.0;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/grenlf1a.wav"), 1, ATTN_NORM, 0);
//		CL_LogoutEffect (pl->current.origin, weapon);
		break;
	// RAFAEL
	case Q2MZ_PHALANX:
		dl->color[0] = 1;dl->color[1] = 0.5; dl->color[2] = 0.5;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/plasshot.wav"), volume, ATTN_NORM, 0);
		break;
	// RAFAEL
	case Q2MZ_IONRIPPER:
		dl->color[0] = 1;dl->color[1] = 0.5; dl->color[2] = 0.5;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/rippfire.wav"), volume, ATTN_NORM, 0);
		break;

// ======================
// PGM
	case Q2MZ_ETF_RIFLE:
		dl->color[0] = 0.9;dl->color[1] = 0.7;dl->color[2] = 0;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/nail1.wav"), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_SHOTGUN2:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/shotg2.wav"), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_HEATBEAM:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		dl->die = cl.time + 100;
	//	Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/bfg__l1a.wav"), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_BLASTER2:
		dl->color[0] = 0;dl->color[1] = 1;dl->color[2] = 0;
		// FIXME - different sound for blaster2 ??
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/blastf1a.wav"), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_TRACKER:
		// negative flashes handled the same in gl/soft until CL_AddDLights
		dl->color[0] = -1;dl->color[1] = -1;dl->color[2] = -1;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/disint2.wav"), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_NUKE1:
		dl->color[0] = 1;dl->color[1] = 0;dl->color[2] = 0;
		dl->die = cl.time + 100;
		break;
	case Q2MZ_NUKE2:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		dl->die = cl.time + 100;
		break;
	case Q2MZ_NUKE4:
		dl->color[0] = 0;dl->color[1] = 0;dl->color[2] = 1;
		dl->die = cl.time + 100;
		break;
	case Q2MZ_NUKE8:
		dl->color[0] = 0;dl->color[1] = 1;dl->color[2] = 1;
		dl->die = cl.time + 100;
		break;
// PGM
// ======================

	case Q2EXMZ_BFG2:
	case Q2EXMZ_PHALANX2:
	case Q2EXMZ_PROX:
	//case Q2EXMZ_ETF_RIFLE_2:	//erk?
		Con_Printf(CON_WARNING"CLQ2_ParseMuzzleFlash: %i not implemented\n", weapon);
		break;
	default:
		Host_EndGame ("CL_ParseMuzzleFlash: bad effect index");
	}
}

void CLQ2_ParseMuzzleFlash2 (void)
{
	int			ent;
	int			flash_number;

	ent = (unsigned short)(short)MSG_ReadShort ();
	if (ent < 1 || ent >= Q2MAX_EDICTS)
		Host_EndGame ("CL_ParseMuzzleFlash2: bad entity");

	flash_number = MSG_ReadByte ();

	CLQ2_RunMuzzleFlash2(ent, flash_number);
}
void CLQ2EX_ParseMuzzleFlash3 (void)
{	//same as 2, but with a short for more numbers.
	int			ent;
	int			flash_number;

	ent = (unsigned short)(short)MSG_ReadShort ();
	if (ent < 1 || ent >= Q2MAX_EDICTS)
		Host_EndGame ("CL_ParseMuzzleFlash3: bad entity");

	flash_number = MSG_ReadUInt16 ();

	CLQ2_RunMuzzleFlash2(ent, flash_number);
}

void CLQ2_ParseInventory (int seat)
{
	unsigned int		i;
	for (i=0 ; i<Q2MAX_ITEMS ; i++)
		cl.inventory[seat][i] = MSG_ReadShort ();
}

/*
=========================================================================

FRAME PARSING

=========================================================================
*/

/*
=================
CL_ParseEntityBits

Returns the entity number and the header bits
=================
*/
//static int	bitcounts[32];	/// just for protocol profiling
static int CLQ2_ParseEntityBits (quint64_t *bits)
{
	quint64_t	b, total;
//	int			i;
	int			number;

	total = MSG_ReadByte ();
	if (total & Q2U_MOREBITS1)
	{
		b = MSG_ReadByte ();
		total |= b<<8;
	}
	if (total & Q2U_MOREBITS2)
	{
		b = MSG_ReadByte ();
		total |= b<<16;
	}
	if (total & Q2U_MOREBITS3)
	{
		b = MSG_ReadByte ();
		total |= b<<24;
	}

	if (total & Q2UEX_MOREBITS4)
	{
		b = MSG_ReadByte ();
		total |= b<<32;
	}

	// count the bits for net profiling
/*	for (i=0 ; i<32 ; i++)
		if (total&(1<<i))
			bitcounts[i]++;
*/
	if (total & Q2U_NUMBER16)
		number = (unsigned short)MSG_ReadShort ();
	else
		number = MSG_ReadByte ();

	*bits = total;

	return number;
}

static unsigned int MSG_ReadSizeQ2E (void)
{
	unsigned int solid = MSG_ReadLong();
	if (solid != ES_SOLID_BSP && solid != ES_SOLID_NOT)
		solid =	(((solid>>0)&0xff)<<0)
			|	(((solid>>16)&0xff)<<8)
			|	((((solid>>24)&0xff)+32768-32)<<16);	/*up can be negative*/
	return solid;
}

/*
==================
CL_ParseDelta

Can go from either a baseline or a previous packet_entity
==================
*/
static void CLQ2_ParseDelta (entity_state_t *from, entity_state_t *to, int number, quint64_t bits)
{
	// set everything to the state we are delta'ing from
	*to = *from;

	VectorCopy (from->origin, to->u.q2.old_origin);
	to->number = number;

	if (bits & Q2U_MODEL)
	{
		if (bits & Q2UX_INDEX16)
			to->modelindex = MSG_ReadShort();
		else
			to->modelindex = MSG_ReadByte ();
	}
	if (bits & Q2U_MODEL2)
	{
		if (bits & Q2UX_INDEX16)
			to->modelindex2 = MSG_ReadShort();
		else
			to->modelindex2 = MSG_ReadByte ();
	}
	if (bits & Q2U_MODEL3)
	{
		if (bits & Q2UX_INDEX16)
			to->u.q2.modelindex3 = MSG_ReadShort();
		else
			to->u.q2.modelindex3 = MSG_ReadByte ();
	}
	if (bits & Q2U_MODEL4)
	{
		if (bits & Q2UX_INDEX16)
			to->u.q2.modelindex4 = MSG_ReadShort();
		else
			to->u.q2.modelindex4 = MSG_ReadByte ();
	}
		
	if (bits & Q2U_FRAME8)
		to->frame = MSG_ReadByte ();
	if (bits & Q2U_FRAME16)
		to->frame = MSG_ReadUInt16();
	if ((bits & Q2U_FRAME8) && (bits & Q2U_FRAME16))
		Con_Printf("\t8bit AND 16bit frame?\n");

	if ((bits & Q2U_SKIN8) && (bits & Q2U_SKIN16))		//used for laser colors
		to->skinnum = MSG_ReadLong();
	else if (bits & Q2U_SKIN8)
		to->skinnum = MSG_ReadByte();
	else if (bits & Q2U_SKIN16)
		to->skinnum = MSG_ReadUInt16();

	if (bits & (Q2U_EFFECTS8|Q2U_EFFECTS16|Q2UEX_EFFECTS64))
	{
		unsigned int lo = 0;
		if (bits&Q2UEX_EFFECTS64)
			lo = MSG_ReadLong();

		if ( (bits & (Q2U_EFFECTS8|Q2U_EFFECTS16)) == (Q2U_EFFECTS8|Q2U_EFFECTS16) )
			to->effects = MSG_ReadLong();
		else if (bits & Q2U_EFFECTS16)
			to->effects = MSG_ReadUInt16();
		else
			to->effects = MSG_ReadByte();

		if (bits&Q2UEX_EFFECTS64)
			to->effects = ((quint64_t)to->effects<<32) | lo;
	}

	if ( (bits & (Q2U_RENDERFX8|Q2U_RENDERFX16)) == (Q2U_RENDERFX8|Q2U_RENDERFX16) )
		to->u.q2.renderfx = MSG_ReadLong() & 0x07ffffff;	//only the standard ones actually supported by vanilla q2 without corrupting fte's own ones.
	else if (bits & Q2U_RENDERFX8)
		to->u.q2.renderfx = MSG_ReadByte();
	else if (bits & Q2U_RENDERFX16)
		to->u.q2.renderfx = MSG_ReadUInt16();

	if (cls.protocol_q2 == PROTOCOL_VERSION_Q2EX)
	{
		if (bits & Q2U_SOLID)
			to->solidsize = MSG_ReadSizeQ2E();

		if (!to->solidsize)
		{	//demos reportedly compress these.
			if (bits & Q2U_ORIGIN1)
				to->origin[0] = MSG_ReadCoord ();
			if (bits & Q2U_ORIGIN2)
				to->origin[1] = MSG_ReadCoord ();
			if (bits & Q2U_ORIGIN3)
				to->origin[2] = MSG_ReadCoord ();

			if (bits & Q2U_OLDORIGIN)
			{
				to->u.q2.old_origin[0] = MSG_ReadCoord();
				to->u.q2.old_origin[1] = MSG_ReadCoord();
				to->u.q2.old_origin[2] = MSG_ReadCoord();
			}
		}
		else
		{
			if (bits & Q2U_ORIGIN1)
				to->origin[0] = MSG_ReadFloat ();
			if (bits & Q2U_ORIGIN2)
				to->origin[1] = MSG_ReadFloat ();
			if (bits & Q2U_ORIGIN3)
				to->origin[2] = MSG_ReadFloat ();

			if (bits & Q2U_OLDORIGIN)
			{
				to->u.q2.old_origin[0] = MSG_ReadFloat();
				to->u.q2.old_origin[1] = MSG_ReadFloat();
				to->u.q2.old_origin[2] = MSG_ReadFloat();
			}
		}

		if (bits & Q2U_ANGLE1)
			to->angles[0] = MSG_ReadFloat();
		if (bits & Q2U_ANGLE2)
			to->angles[1] = MSG_ReadFloat();
		if (bits & Q2U_ANGLE3)
			to->angles[2] = MSG_ReadFloat();
	}
	else
	{
		if (bits & Q2U_ORIGIN1)
			to->origin[0] = MSG_ReadCoord ();
		if (bits & Q2U_ORIGIN2)
			to->origin[1] = MSG_ReadCoord ();
		if (bits & Q2U_ORIGIN3)
			to->origin[2] = MSG_ReadCoord ();

		if ((bits & Q2UX_ANGLE16) && (net_message.prim.flags & NPQ2_ANG16))
		{
			if (bits & Q2U_ANGLE1)
				to->angles[0] = MSG_ReadAngle16();
			if (bits & Q2U_ANGLE2)
				to->angles[1] = MSG_ReadAngle16();
			if (bits & Q2U_ANGLE3)
				to->angles[2] = MSG_ReadAngle16();
		}
		else
		{
			if (bits & Q2U_ANGLE1)
				to->angles[0] = MSG_ReadAngle();
			if (bits & Q2U_ANGLE2)
				to->angles[1] = MSG_ReadAngle();
			if (bits & Q2U_ANGLE3)
				to->angles[2] = MSG_ReadAngle();
		}

		if (bits & Q2U_OLDORIGIN)
			MSG_ReadPos (to->u.q2.old_origin);
	}

	if (bits & Q2U_SOUND)
	{
		if (cls.protocol_q2 == PROTOCOL_VERSION_Q2EX)
		{
			to->u.q2.sound = MSG_ReadUInt16();
			/*to->u.q2.soundvol =*/ (to->u.q2.sound&0x4000)?MSG_ReadByte():255;
			/*to->u.q2.soundattn =*/ (to->u.q2.sound&0x8000)?MSG_ReadByte():3;
			to->u.q2.sound &= 0x3fff;
		}
		else if (bits & Q2UX_INDEX16)
			to->u.q2.sound = MSG_ReadUInt16();
		else
			to->u.q2.sound = MSG_ReadByte ();
	}

	if (bits & Q2U_EVENT)
		to->u.q2.event = MSG_ReadByte ();
	else
		to->u.q2.event = 0;

	if (cls.protocol_q2 != PROTOCOL_VERSION_Q2EX)
	{
		if (bits & Q2U_SOLID)
		{
			if (net_message.prim.flags & NPQ2_SOLID32)
				to->solidsize = MSG_ReadLong();
			else
				to->solidsize = MSG_ReadSize16 (&net_message);
		}
	}

	if (bits & Q2UEX_ALPHA)
		to->trans = MSG_ReadByte();
	if (bits & Q2UEX_SCALE)
		to->scale = MSG_ReadByte();
	if (bits & Q2UEX_INSTANCE)
		to->u.q2.instance = MSG_ReadByte();
	if (bits & Q2UEX_OWNER)
		to->u.q2.owner = MSG_ReadShort();
	if (bits & Q2UEX_OLDFRAME)
		to->u.q2.oldframe = MSG_ReadUInt16();
}

void CLQ2_ClearParticleState(void)
{
	int i;
	for (i = 0; i < MAX_Q2EDICTS; i++)
	{
		P_DelinkTrailstate(&cl_entities[i].trailstate);
	}
}

/*
==================
CL_DeltaEntity

Parses deltas from the given base and adds the resulting entity
to the current frame
==================
*/
static void CLQ2_DeltaEntity (q2frame_t *frame, int newnum, entity_state_t *old, uint64_t bits)
{
	q2centity_t	*ent;
	entity_state_t	*state;

	ent = &cl_entities[newnum];

	state = &clq2_parse_entities[cl.parse_entities & (MAX_PARSE_ENTITIES-1)];
	cl.parse_entities++;
	frame->num_entities++;

	CLQ2_ParseDelta (old, state, newnum, bits);

	// some data changes will force no lerping
	if (state->modelindex != ent->current.modelindex
		|| state->modelindex2 != ent->current.modelindex2
		|| state->u.q2.modelindex3 != ent->current.u.q2.modelindex3
		|| state->u.q2.modelindex4 != ent->current.u.q2.modelindex4
		|| fabs(state->origin[0] - ent->current.origin[0]) > 512
		|| fabs(state->origin[1] - ent->current.origin[1]) > 512
		|| fabs(state->origin[2] - ent->current.origin[2]) > 512
		|| state->u.q2.event == Q2EV_PLAYER_TELEPORT
		|| state->u.q2.event == Q2EV_OTHER_TELEPORT
		)
	{
		ent->serverframe = -99;
	}

	if (ent->serverframe != cl.q2frame.serverframe - 1)
	{	// wasn't in last update, so initialize some things
		// clear trailstate
		P_DelinkTrailstate(&ent->trailstate);

		// duplicate the current state so lerping doesn't hurt anything
		ent->prev = *state;
		if (state->u.q2.event == Q2EV_OTHER_TELEPORT)
		{
			VectorCopy (state->origin, ent->prev.origin);
			VectorCopy (state->origin, ent->lerp_origin);
		}
		else
		{
			VectorCopy (state->u.q2.old_origin, ent->prev.origin);
			VectorCopy (state->u.q2.old_origin, ent->lerp_origin);
		}

		ent->oldframe = state->u.q2.oldframe;
		ent->oldframetime = cl.oldgametime;
	}
	else
	{	// shuffle the last state to previous
		ent->prev = ent->current;

		if (old->frame != state->frame)
		{
			ent->oldframe = old->frame;
			ent->oldframetime = cl.oldgametime;
		}
	}

	ent->serverframe = cl.q2frame.serverframe;
	ent->current = *state;
}

/*
==================
CL_ParsePacketEntities

An svc_packetentities has just been parsed, deal with the
rest of the data stream.
==================
*/
static void CLQ2_ParsePacketEntities (q2frame_t *oldframe, q2frame_t *newframe)
{
	unsigned int		newnum;
	quint64_t			bits;
	entity_state_t	*oldstate=NULL;
	unsigned int		oldindex, oldnum;

	cl.validsequence = cls.netchan.incoming_sequence;
	cl.ackedmovesequence = cls.netchan.incoming_acknowledged;

	cl.outframes[cl.ackedmovesequence&UPDATE_MASK].latency = realtime - cl.outframes[cl.ackedmovesequence&UPDATE_MASK].senttime;

	newframe->parse_entities = cl.parse_entities;
	newframe->num_entities = 0;

	// delta from the entities present in oldframe
	oldindex = 0;
	if (!oldframe)
		oldnum = 99999;
	else
	{
		if (oldindex >= oldframe->num_entities)
			oldnum = 99999;
		else
		{
			oldstate = &clq2_parse_entities[(oldframe->parse_entities+oldindex) & (MAX_PARSE_ENTITIES-1)];
			oldnum = oldstate->number;
		}
	}

	while (1)
	{
		newnum = CLQ2_ParseEntityBits (&bits);
		if (newnum >= MAX_Q2EDICTS)
			Host_EndGame ("CL_ParsePacketEntities: bad number:%i", newnum);

		if (MSG_GetReadCount() > net_message.cursize)
			Host_EndGame ("CL_ParsePacketEntities: end of message");

		if (!newnum)
			break;

		while (oldnum < newnum)
		{	// one or more entities from the old packet are unchanged
			if (cl_shownet.ival == 3)
				Con_Printf ("%i:   unchanged: %i\n", MSG_GetReadCount(), oldnum);
			CLQ2_DeltaEntity (newframe, oldnum, oldstate, 0);
			
			oldindex++;

			if (!oldframe || oldindex >= oldframe->num_entities)
				oldnum = 99999;
			else
			{
				oldstate = &clq2_parse_entities[(oldframe->parse_entities+oldindex) & (MAX_PARSE_ENTITIES-1)];
				oldnum = oldstate->number;
			}
		}

		if (bits & Q2U_REMOVE)
		{	// the entity present in oldframe is not in the current frame
			if (cl_shownet.ival == 3)
				Con_Printf ("%3i:   remove: %i\n", MSG_GetReadCount(), newnum);
			if (oldnum != newnum)
				Con_Printf ("U_REMOVE: oldnum != newnum\n");

			oldindex++;

			if (!oldframe || oldindex >= oldframe->num_entities)
				oldnum = 99999;
			else
			{
				oldstate = &clq2_parse_entities[(oldframe->parse_entities+oldindex) & (MAX_PARSE_ENTITIES-1)];
				oldnum = oldstate->number;
			}
			continue;
		}

		if (oldnum == newnum)
		{	// delta from previous state
			if (cl_shownet.ival == 3)
				Con_Printf ("%3i:   delta: %i\n", MSG_GetReadCount(), newnum);
			CLQ2_DeltaEntity (newframe, newnum, oldstate, bits);

			oldindex++;

			if (oldindex >= oldframe->num_entities)
				oldnum = 99999;
			else
			{
				oldstate = &clq2_parse_entities[(oldframe->parse_entities+oldindex) & (MAX_PARSE_ENTITIES-1)];
				oldnum = oldstate->number;
			}
			continue;
		}

		if (oldnum > newnum)
		{	// delta from baseline
			if (cl_shownet.ival == 3)
				Con_Printf ("%3i:   baseline: %i\n", MSG_GetReadCount(), newnum);
			CLQ2_DeltaEntity (newframe, newnum, &cl_entities[newnum].baseline, bits);
			continue;
		}

	}

	// any remaining entities in the old frame are copied over
	while (oldnum != 99999)
	{	// one or more entities from the old packet are unchanged
		if (cl_shownet.ival == 3)
			Con_Printf ("%3i:   unchanged: %i\n", MSG_GetReadCount(), oldnum);
		CLQ2_DeltaEntity (newframe, oldnum, oldstate, 0);
		
		oldindex++;

		if (oldindex >= oldframe->num_entities)
			oldnum = 99999;
		else
		{
			oldstate = &clq2_parse_entities[(oldframe->parse_entities+oldindex) & (MAX_PARSE_ENTITIES-1)];
			oldnum = oldstate->number;
		}
	}
}

void CLQ2_ParseBaseline (void)
{
	entity_state_t	*es;
	quint64_t				bits;
	int				newnum;

	newnum = CLQ2_ParseEntityBits (&bits);
	es = &cl_entities[newnum].baseline;
	CLQ2_ParseDelta (&nullentitystate, es, newnum, bits);
}


/*
===================
CL_ParsePlayerstate
===================
*/
void CLQ2_ParsePlayerstate (int seat, q2frame_t *oldframe, q2frame_t *newframe, int extflags)
{
	int			flags;
	q2player_state_t	*state;
	int			i;
	int			statbits;
	qboolean q2e = cls.protocol_q2==PROTOCOL_VERSION_Q2EX;

	state = &newframe->seat[seat].playerstate;

	// clear to old value before delta parsing
	if (oldframe)
	{
		*state = oldframe->seat[seat].playerstate;
		newframe->seat[seat].clientnum = oldframe->seat[seat].clientnum;
	}
	else
	{
		memset (state, 0, sizeof(*state));
		newframe->seat[seat].clientnum = cl.playerview[seat].playernum;
	}

	flags = MSG_ReadUInt16 ();
	if (flags & Q2PS_EXTRABITS)
		flags |= (q2e?MSG_ReadUInt16():MSG_ReadByte())<<16;

	//
	// parse the pmove_state_t
	//
	if (flags & Q2PS_M_TYPE)
	{
		state->pmove.pm_type = MSG_ReadByte ();
		if (q2e)
		{	//sigh... q2e added some extra pmove types that we don't support.
			switch((int)state->pmove.pm_type)
			{
			case Q2EPM_NORMAL: state->pmove.pm_type = Q2PM_NORMAL; break;
			case Q2EPM_GRAPPLE: state->pmove.pm_type = Q2PM_NORMAL; break;	//FIXME: not supported
			case Q2EPM_SPECTATOR: state->pmove.pm_type = Q2PM_SPECTATOR; break;
			case Q2EPM_SPECTATOR2: state->pmove.pm_type = Q2PM_SPECTATOR; break;	//FIXME: not supported.
			case Q2EPM_DEAD: state->pmove.pm_type = Q2PM_DEAD; break;
			case Q2EPM_GIB: state->pmove.pm_type = Q2PM_GIB; break;
			case Q2EPM_FREEZE: state->pmove.pm_type = Q2PM_FREEZE; break;
			}
		}
	}

	if (flags & Q2PS_M_ORIGIN)
	{
		if (q2e)
		{
			state->pmove.origin[0] = MSG_ReadFloat ()*8;
			state->pmove.origin[1] = MSG_ReadFloat ()*8;
			if (extflags & Q2PSX_OLD)
				state->pmove.origin[2] = MSG_ReadFloat ()*8;
		}
		else
		{
			state->pmove.origin[0] = MSG_ReadShort ();
			state->pmove.origin[1] = MSG_ReadShort ();
			if (extflags & Q2PSX_OLD)
				state->pmove.origin[2] = MSG_ReadShort ();
		}
	}
	if (extflags & Q2PSX_M_ORIGIN2)
		state->pmove.origin[2] = MSG_ReadShort ();

	if (flags & Q2PS_M_VELOCITY)
	{
		if (q2e)
		{
			state->pmove.velocity[0] = MSG_ReadFloat ()*8;
			state->pmove.velocity[1] = MSG_ReadFloat ()*8;
			if (extflags & Q2PSX_OLD)
				state->pmove.velocity[2] = MSG_ReadFloat ()*8;
		}
		else
		{
			state->pmove.velocity[0] = MSG_ReadShort ();
			state->pmove.velocity[1] = MSG_ReadShort ();
			if (extflags & Q2PSX_OLD)
				state->pmove.velocity[2] = MSG_ReadShort ();
		}
	}
	if (extflags & Q2PSX_M_VELOCITY2)
		state->pmove.velocity[2] = MSG_ReadShort ();

	if (flags & Q2PS_M_TIME)
		state->pmove.pm_time = q2e?MSG_ReadUInt16():MSG_ReadByte ();

	if (flags & Q2PS_M_FLAGS)
		state->pmove.pm_flags = q2e?MSG_ReadUInt16():MSG_ReadByte ();

	if (flags & Q2PS_M_GRAVITY)
		state->pmove.gravity = MSG_ReadShort ();

	if (flags & Q2PS_M_DELTA_ANGLES)
	{
		if (q2e)
		{
			state->pmove.delta_angles[0] = ANGLE2SHORT(MSG_ReadFloat ());
			state->pmove.delta_angles[1] = ANGLE2SHORT(MSG_ReadFloat ());
			state->pmove.delta_angles[2] = ANGLE2SHORT(MSG_ReadFloat ());
		}
		else
		{
			state->pmove.delta_angles[0] = MSG_ReadShort ();
			state->pmove.delta_angles[1] = MSG_ReadShort ();
			state->pmove.delta_angles[2] = MSG_ReadShort ();
		}
	}

//	if (cl.attractloop)
//		state->pmove.pm_type = Q2PM_FREEZE;		// demo playback

	//
	// parse the rest of the player_state_t
	//
	if (flags & Q2PS_VIEWOFFSET)
	{
		if (q2e)
		{
			state->viewoffset[0] = MSG_ReadShort () * (1/16.f);
			state->viewoffset[1] = MSG_ReadShort () * (1/16.f);
			state->viewoffset[2] = MSG_ReadShort () * (1/16.f);
			state->viewoffset[2] += MSG_ReadChar();
		}
		else
		{
			state->viewoffset[0] = MSG_ReadChar () * 0.25;
			state->viewoffset[1] = MSG_ReadChar () * 0.25;
			state->viewoffset[2] = MSG_ReadChar () * 0.25;
		}
	}

	if (flags & Q2PS_VIEWANGLES)
	{
		if (q2e)
		{
			state->viewangles[0] = MSG_ReadFloat ();
			state->viewangles[1] = MSG_ReadFloat ();
			if (extflags & Q2PSX_OLD)
				state->viewangles[2] = MSG_ReadFloat ();
		}
		else
		{
			state->viewangles[0] = MSG_ReadAngle16 ();
			state->viewangles[1] = MSG_ReadAngle16 ();
			if (extflags & Q2PSX_OLD)
				state->viewangles[2] = MSG_ReadAngle16 ();
		}
	}
	if (extflags & Q2PSX_VIEWANGLE2)
		state->viewangles[2] = MSG_ReadAngle16 ();

	if (flags & Q2PS_KICKANGLES)
	{
		if (q2e)
		{
			state->kick_angles[0] = MSG_ReadShort () * (1/1024.f);
			state->kick_angles[1] = MSG_ReadShort () * (1/1024.f);
			state->kick_angles[2] = MSG_ReadShort () * (1/1024.f);
		}
		else
		{
			state->kick_angles[0] = MSG_ReadChar () * 0.25;
			state->kick_angles[1] = MSG_ReadChar () * 0.25;
			state->kick_angles[2] = MSG_ReadChar () * 0.25;
		}
	}

	if (flags & Q2PS_WEAPONINDEX)
	{
		if (q2e)
		{
			state->gunindex = MSG_ReadUInt16 ();
			//state->gunskin = state->gunindex>>13
			state->gunindex &= 0x1fff;
		}
		else
		{
			if (flags & Q2FTEPS_INDEX16)
				state->gunindex = MSG_ReadShort ();
			else
				state->gunindex = MSG_ReadByte ();
		}
	}

	if (flags & Q2PS_WEAPONFRAME)
	{
		if (q2e)
		{
			unsigned int fl = MSG_ReadUInt16 ();
			state->gunframe = fl & 0x1ff;
			if (fl & (1<< 9)) state->gunoffset[0] = MSG_ReadFloat ();
			if (fl & (1<<10)) state->gunoffset[1] = MSG_ReadFloat ();
			if (fl & (1<<11)) state->gunoffset[2] = MSG_ReadFloat ();
			if (fl & (1<<12)) state->gunangles[0] = MSG_ReadFloat ();
			if (fl & (1<<13)) state->gunangles[1] = MSG_ReadFloat ();
			if (fl & (1<<14)) state->gunangles[2] = MSG_ReadFloat ();
			if (fl & (1<<15)) /*state->gunangles[2] =*/ MSG_ReadByte ();
		}
		else
		{
			if (flags & Q2FTEPS_INDEX16)
				state->gunframe = MSG_ReadShort ();
			else
				state->gunframe = MSG_ReadByte ();
			if (extflags & Q2PSX_OLD)
			{
				state->gunoffset[0] = MSG_ReadChar ()*0.25;
				state->gunoffset[1] = MSG_ReadChar ()*0.25;
				state->gunoffset[2] = MSG_ReadChar ()*0.25;
				state->gunangles[0] = MSG_ReadChar ()*0.25;
				state->gunangles[1] = MSG_ReadChar ()*0.25;
				state->gunangles[2] = MSG_ReadChar ()*0.25;
			}
		}
	}
	if (extflags & Q2PSX_GUNOFFSET)
	{
		state->gunoffset[0] = MSG_ReadChar ()*0.25;
		state->gunoffset[1] = MSG_ReadChar ()*0.25;
		state->gunoffset[2] = MSG_ReadChar ()*0.25;
	}
	if (extflags & Q2PSX_GUNANGLES)
	{
		state->gunangles[0] = MSG_ReadChar ()*0.25;
		state->gunangles[1] = MSG_ReadChar ()*0.25;
		state->gunangles[2] = MSG_ReadChar ()*0.25;
	}

	if (flags & Q2PS_BLEND)
	{
		state->blend[0] = MSG_ReadByte ()/255.0;
		state->blend[1] = MSG_ReadByte ()/255.0;
		state->blend[2] = MSG_ReadByte ()/255.0;
		state->blend[3] = MSG_ReadByte ()/255.0;
	}

	if (flags & Q2PS_FOV)
		state->fov = MSG_ReadByte ();

	if (flags & Q2PS_RDFLAGS)
		state->rdflags = MSG_ReadByte ();

	// parse stats
	if (extflags & (Q2PSX_OLD|Q2PSX_STATS))
		statbits = MSG_ReadLong ();
	else
		statbits = 0;
	if (statbits)
	{
		for (i=0 ; i<min(32,Q2MAX_STATS) ; i++)
			if (statbits & (1<<i) )
				state->stats[i] = MSG_ReadShort();
	}
	if (q2e)
	{
		// parse stats
		statbits = MSG_ReadLong ();
		if (statbits)
		{
			for (i=0 ; i<Q2MAX_STATS-32 ; i++)
				if (statbits & (1<<i) )
					state->stats[32+i] = MSG_ReadShort();
			for ( ; i<32 ; i++)
				if (statbits & (1<<i) )
					MSG_ReadShort();
		}

		if (flags & Q2EXPS_DAMAGEBLEND)
		{
			/*state->damageblend[0] = (1/255.f) */ MSG_ReadByte ();
			/*state->damageblend[1] = (1/255.f) */ MSG_ReadByte ();
			/*state->damageblend[2] = (1/255.f) */ MSG_ReadByte ();
			/*state->damageblend[3] = (1/255.f) */ MSG_ReadByte ();
		}
		if (flags & Q2EXPS_TEAMID)
			MSG_ReadByte();
	}
	else
	{
		if ((extflags & Q2PSX_CLIENTNUM) || (flags & Q2FTEPS_CLIENTNUM))
			newframe->seat[seat].clientnum = MSG_ReadByte();
	}
}


/*
==================
CL_FireEntityEvents

==================
*/
void CLQ2_FireEntityEvents (q2frame_t *frame)
{
	entity_state_t		*s1;
	int					pnum, num;

	for (pnum = 0 ; pnum<frame->num_entities ; pnum++)
	{
		num = (frame->parse_entities + pnum)&(MAX_PARSE_ENTITIES-1);
		s1 = &clq2_parse_entities[num];
		if (s1->u.q2.event)
			CLQ2_EntityEvent (s1);

		// EF_TELEPORTER acts like an event, but is not cleared each frame
		if (s1->effects & Q2EF_TELEPORTER)
			CLQ2_TeleporterParticles (s1);
	}
}

void CLR1Q2_ParsePlayerUpdate(void)
{
	unsigned int framenum = MSG_ReadLong();
	q2frame_t	*frame = &cl.q2frames[framenum & Q2UPDATE_MASK];
	int seat;
	if (frame->serverframe != framenum)
		Con_DPrintf("svcr1q2_playerupdate: stale frame\n");
	else if (!frame->valid)
		Con_DPrintf("svcr1q2_playerupdate: invalid frame\n");	//excrement happens.
	else
	{
		int pnum;
		vec3_t neworg;
		entity_state_t *st;
		for (pnum = 0; pnum < frame->num_entities; pnum++)
		{
			st = &clq2_parse_entities[(frame->parse_entities+pnum) & (MAX_PARSE_ENTITIES-1)];

			//I don't like how r1q2 does its maxclients, so I'm just going to go on message size instead
			if (MSG_GetReadCount() == net_message.cursize)
				break;

			//the local client(s) is not included, thanks to prediction covering that.
			for (seat = 0; seat < cl.splitclients; seat++)
			{
				if (st->number == cl.playerview[0].playernum+1)
					break;
			}
			if (seat != cl.splitclients)
				continue;

			if (st->number != 1)
				continue;

			//FIXME: handle this, with lerping and stuff.
			MSG_ReadPos(neworg);
		}

		//just for sanity's sake
		if (MSG_GetReadCount() != net_message.cursize)
			msg_badread = true;
	}
	//this should be the only/last thing in these packets, because if it isn't then we're screwed when a packet got lost
	net_message.currentbit = net_message.cursize<<3;
}

#define SHOWNET(x) if(cl_shownet.value>=2)Con_Printf ("%3i:%s\n", MSG_GetReadCount()-1, x);
/*
================
CL_ParseFrame
================
*/
void CLQ2_ParseFrame (int extrabits)
{
	int			cmd;
	int			len;
	q2frame_t		*old;
	int i,j, chokecount;
	static float throttle;

	memset (&cl.q2frame, 0, sizeof(cl.q2frame));

#if 0
	CLQ2_ClearProjectiles(); // clear projectiles for new frame
#endif

	if (cls.protocol_q2 == PROTOCOL_VERSION_R1Q2 || cls.protocol_q2 == PROTOCOL_VERSION_Q2PRO)
	{
		unsigned int bits = MSG_ReadLong();
		cl.q2frame.serverframe = bits & 0x07ffffff;
		i = bits >> 27;
		if (i == 31)
			cl.q2frame.deltaframe = -1;
		else
			cl.q2frame.deltaframe = cl.q2frame.serverframe - i;
		bits = MSG_ReadByte();
		chokecount = bits & 0xf;
		extrabits = (extrabits<<4) | (bits>>4);
	}
	else
	{
		cl.q2frame.serverframe = MSG_ReadLong ();
		cl.q2frame.deltaframe = MSG_ReadLong ();
		if (cls.protocol_q2 > 26)
			chokecount = MSG_ReadByte ();
		else
			chokecount = 0;

		extrabits = Q2PSX_OLD;
	}

	cl.q2frame.servertime = cl.q2frame.serverframe*(1000/cl.q2svnetrate);

	cl.oldgametime = cl.gametime;
	cl.oldgametimemark = cl.gametimemark;
	cl.gametime = cl.q2frame.servertime/1000.;
	cl.gametimemark = realtime;

	for (j=0 ; j<chokecount ; j++)
		cl.outframes[ (cls.netchan.incoming_acknowledged-1-j)&UPDATE_MASK ].latency = -2;

	if (cl_shownet.value == 3)
		Con_Printf ("   frame:%i  delta:%i\n", cl.q2frame.serverframe, cl.q2frame.deltaframe);

	// If the frame is delta compressed from data that we
	// no longer have available, we must suck up the rest of
	// the frame, but not use it, then ask for a non-compressed
	// message 
	if (cl.q2frame.deltaframe <= 0)
	{
		cl.q2frame.valid = true;		// uncompressed frame
		old = NULL;
		cls.demohadkeyframe = true;	//yay! all is right with the world!
	}
	else
	{
		old = &cl.q2frames[cl.q2frame.deltaframe & Q2UPDATE_MASK];
		if (!old->valid)
		{	// should never happen
			Con_ThrottlePrintf (&throttle, 0, CON_WARNING"Delta from invalid frame (not supposed to happen!).\n");
			old = NULL;
		}
		else if (old->serverframe != cl.q2frame.deltaframe)
		{	// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly.
			Con_ThrottlePrintf (&throttle, 0, CON_WARNING"Delta frame too old.\n");
		}
		else if (cl.parse_entities - old->parse_entities > MAX_PARSE_ENTITIES-128)
		{
			Con_ThrottlePrintf (&throttle, 0, CON_WARNING"Delta parse_entities too old.\n");
		}
		else
			cl.q2frame.valid = true;	// valid delta parse
	}

	// clamp time 
	if (cl.time > cl.q2frame.servertime/1000.0)
		cl.time = cl.q2frame.servertime/1000.0;
	else if (cl.time < (cl.q2frame.servertime - 100)/1000.0)
		cl.time = (cl.q2frame.servertime - 100)/1000.0;

	if (cls.protocol_q2 == PROTOCOL_VERSION_Q2EX)
	{
		for (i = 0; i<cl.splitclients; i++)
		{
			len = MSG_ReadByte ();
			if (len > countof(cl.q2frame.seat[i].areabits))
				Host_EndGame ("CL_ParseFrame: too many areas");
			MSG_ReadData (&cl.q2frame.seat[i].areabits, len);

			cmd = MSG_ReadByte ();
			if (cmd != svcq2_playerinfo)
				Host_EndGame ("CL_ParseFrame: not playerinfo");
			SHOWNET("svcq2_playerinfo");
			CLQ2_ParsePlayerstate (i, old, &cl.q2frame, extrabits);
		}
		cmd = MSG_ReadByte ();
//		SHOWNET(svc_strings[cmd]);
		if (cmd != svcq2_packetentities)
			Host_EndGame ("CL_ParseFrame: not packetentities");
		SHOWNET("svcq2_packetentities");
	}
	else
	{
		// read areabits
		len = MSG_ReadByte ();
		if (len > MAX_Q2MAP_AREAS/8)
			Host_EndGame ("CL_ParseFrame: too many areas");
		MSG_ReadData (&cl.q2frame.seat[0].areabits, len);

		// normally playerstate then packet entities
		//in splitscreen we may have multiple player states, one per player.
		if (cls.protocol_q2 != PROTOCOL_VERSION_R1Q2 && cls.protocol_q2 != PROTOCOL_VERSION_Q2PRO)
		{
			for (cl.splitclients = 0; ; )
			{
				cmd = MSG_ReadByte ();
	//			SHOWNET(svc_strings[cmd]);
				if (cmd == svcq2_playerinfo && cl.splitclients < MAX_SPLITS)
				{
					if (cl.splitclients != 0)
						memcpy(cl.q2frame.seat[cl.splitclients].areabits, cl.q2frame.seat[0].areabits, len);
					SHOWNET("svcq2_playerinfo");
					CLQ2_ParsePlayerstate (cl.splitclients++, old, &cl.q2frame, extrabits);
				}
				else
					break;
			}
			if (!cl.splitclients)
				Host_EndGame ("CL_ParseFrame: no playerinfo");
			if (cmd != svcq2_packetentities)
				Host_EndGame ("CL_ParseFrame: not packetentities");
			SHOWNET("svcq2_packetentities");
		}
		else
		{
			cl.splitclients = 1;
			CLQ2_ParsePlayerstate (0, old, &cl.q2frame, extrabits);
		}
	}
	CLQ2_ParsePacketEntities (old, &cl.q2frame);

	for (cmd = 0; cmd < MAX_SPLITS; cmd++)
		cl.playerview[cmd].viewentity = cl.q2frame.seat[cmd].clientnum+1;

	// save the frame off in the backup array for later delta comparisons
	cl.q2frames[cl.q2frame.serverframe & Q2UPDATE_MASK] = cl.q2frame;

	cl.parsecount = cl.q2frame.serverframe;
	if (cl.q2frame.valid)
	{
		// getting a valid frame message ends the connection process
		if (cls.state != ca_active)
		{
			CL_MakeActive("Quake2");

//			cl.force_refdef = true;
//			cl.predicted_origin[0] = cl.q2frame.playerstate[0].pmove.origin[0]*0.125;
//			cl.predicted_origin[1] = cl.q2frame.playerstate[0].pmove.origin[1]*0.125;
//			cl.predicted_origin[2] = cl.q2frame.playerstate[0].pmove.origin[2]*0.125;
//			VectorCopy (cl.q2frame.playerstate[0].viewangles, cl.predicted_angles);
//			if (cls.disable_servercount != cl.servercount
//				&& cl.refresh_prepped)
				SCR_EndLoadingPlaque ();	// get rid of loading plaque
		}
//		cl.sound_prepped = true;	// can start mixing ambient sounds
	
		// fire entity events
		CLQ2_FireEntityEvents (&cl.q2frame);
#ifdef Q2BSPS
		CLQ2_CheckPredictionError ();
#endif

		cl.validsequence = cl.q2frame.serverframe;
	}
}

/*
==========================================================================

INTERPOLATE BETWEEN FRAMES TO GET RENDERING PARMS

==========================================================================
*/
/*
struct model_s *S_RegisterSexedModel (entity_state_t *ent, char *base)
{
	int				n;
	char			*p;
	struct model_s	*mdl;
	char			model[MAX_QPATH];
	char			buffer[MAX_QPATH];

	// determine what model the client is using
	model[0] = 0;
	n = CS_PLAYERSKINS + ent->number - 1;
	if (cl.configstrings[n][0])
	{
		p = strchr(cl.configstrings[n], '\\');
		if (p)
		{
			p += 1;
			strcpy(model, p);
			p = strchr(model, '/');
			if (p)
				*p = 0;
		}
	}
	// if we can't figure it out, they're male
	if (!model[0])
		strcpy(model, "male");

	Com_sprintf (buffer, sizeof(buffer), "players/%s/%s", model, base+1);
	mdl = re.RegisterModel(buffer);
	if (!mdl) {
		// not found, try default weapon model
		Com_sprintf (buffer, sizeof(buffer), "players/%s/weapon.md2", model);
		mdl = re.RegisterModel(buffer);
		if (!mdl) {
			// no, revert to the male model
			Com_sprintf (buffer, sizeof(buffer), "players/%s/%s", "male", base+1);
			mdl = re.RegisterModel(buffer);
			if (!mdl) {
				// last try, default male weapon.md2
				Com_sprintf (buffer, sizeof(buffer), "players/male/weapon.md2");
				mdl = re.RegisterModel(buffer);
			}
		} 
	}

	return mdl;
}

*/

//returns a list of all the ents currently trying to play a sound.
unsigned int CLQ2_GatherSounds(vec3_t *positions, unsigned int *entnums, sfx_t **sounds, unsigned int max)
{
	entity_state_t		*s1;
	sfx_t *sfx;
	unsigned int pnum;
	unsigned int count = 0;
	q2frame_t *frame = &cl.q2frame;
	for (pnum = 0 ; pnum<frame->num_entities ; pnum++)
	{
		s1 = &clq2_parse_entities[(frame->parse_entities+pnum)&(MAX_PARSE_ENTITIES-1)];
		if (s1->u.q2.sound > 0 && s1->u.q2.sound < MAX_PRECACHE_SOUNDS)
		{
			sfx = cl.sound_precache[s1->u.q2.sound];
			if (sfx)
			{
				if (count == max)
				{
					Con_DPrintf("Exceeded limit of %d looped sounds\n", max);
					break;
				}
				//fixme: sexed sounds
				entnums[count] = s1->number;
				VectorCopy(s1->origin, positions[count]);
				sounds[count] = sfx;
				count++;
			}
		}
	}
	return count;
}


static struct q2ex_rtlight_s
{
	qboolean dirty;
	int key, type, res, style;
	float radius, intensity, fade[2], cone, conedir[3];
	vec3_t axis[3];
	vec3_t angles;
} q2ex_rtlight[Q2EXMAX_RTLIGHTS];
void CLQ2EX_ParseLightConfigString(int i, const char *s)
{
	struct q2ex_rtlight_s *l = &q2ex_rtlight[i];
	l->dirty = true;
	s = COM_ParseToken(s, ";");	l->key			= atoi(com_token);	s = COM_ParseToken(s, ";");
	s = COM_ParseToken(s, ";");	l->type			= atoi(com_token);	s = COM_ParseToken(s, ";");
	s = COM_ParseToken(s, ";");	l->radius		= atof(com_token);	s = COM_ParseToken(s, ";");
	s = COM_ParseToken(s, ";");	l->res			= atoi(com_token);	s = COM_ParseToken(s, ";");
	s = COM_ParseToken(s, ";");	l->intensity	= atof(com_token);	s = COM_ParseToken(s, ";");
	s = COM_ParseToken(s, ";");	l->fade[0]		= atof(com_token);	s = COM_ParseToken(s, ";");
	s = COM_ParseToken(s, ";");	l->fade[1]		= atof(com_token);	s = COM_ParseToken(s, ";");
	s = COM_ParseToken(s, ";");	l->style		= atoi(com_token);	s = COM_ParseToken(s, ";");
	s = COM_ParseToken(s, ";");	l->cone			= atof(com_token);	s = COM_ParseToken(s, ";");
	s = COM_ParseToken(s, ";");	l->axis[0][0]	= atof(com_token);	s = COM_ParseToken(s, ";");
	s = COM_ParseToken(s, ";");	l->axis[0][1]	= atof(com_token);	s = COM_ParseToken(s, ";");
	s = COM_ParseToken(s, ";");	l->axis[0][2]	= atof(com_token);	s = COM_ParseToken(s, ";");

	//faff around with angles and stuff.
	VectorVectors(l->axis[0], l->axis[1], l->axis[2]);
	VectorNegate(l->axis[1], l->axis[1]);
	VectorAngles(l->axis[0], l->axis[2], l->angles, false);
}
static void CLQ2EX_UpdateRTLight(int key, vec3_t org)
{
	int			i;
	dlight_t	*dl;
	struct q2ex_rtlight_s *l;

	for (i = 0, l = q2ex_rtlight; i < countof(q2ex_rtlight); i++, l++)
	{
		if (l->key == key)
		{	//okay, we found the configstring entry...

			//see if its current or not...
			dl = cl_dlights+rtlights_first;
			for (i=rtlights_first ; i<rtlights_max ; i++, dl++)
			{
				if (dl->key == key)
					break;
			}

			if (i==rtlights_max)
			{
				dl = CL_AllocSlight();
				dl->flags = LFLAG_NORMALMODE|LFLAG_REALTIMEMODE;
				dl->key = key;
				l->dirty = true;
			}

			if (!l->dirty && !(dl->flags&LFLAG_FORCECACHE))
			{
				if (!VectorCompare(org, dl->origin))
				{	//it moved :( keep the light tracking the ent and invalidate the cache.
					VectorCopy(org, dl->origin);
					dl->rebuildcache = true;
				}
			}
			else
			{	//LOTS of stuff changed.
				l->dirty = false;
				dl->flags |= LFLAG_FORCECACHE;

				//ent info
				VectorCopy(org, dl->origin);

				//config string info
				dl->radius = l->radius;
//FIXME: we're not using the suggested resolution.
				VectorSet(dl->color, l->intensity,l->intensity,l->intensity);
				dl->fade[0] = l->fade[0];
				dl->fade[1] = l->fade[1];
				dl->style = l->style;

				if (l->type)
					dl->fov = l->cone*2;
				VectorCopy(l->angles, dl->angles);
				VectorCopy(l->axis[0], dl->axis[0]);
				VectorCopy(l->axis[1], dl->axis[1]);
				VectorCopy(l->axis[2], dl->axis[2]);

				dl->corona = 0;
				dl->rebuildcache = true;
			}
			break;
		}
	}
}

/*
===============
CL_AddPacketEntities

===============
*/
void R_AddItemTimer(vec3_t shadoworg, float yaw, float radius, float percent, vec3_t rgb);
static void CLQ2_AddPacketEntities (q2frame_t *frame)
{
	entity_t			ent;
	entity_state_t		*s1;
	float				autorotate;
	int					i;
	int					pnum;
	q2centity_t			*cent;
	int					autoanim;
//	q2clientinfo_t		*ci;
	player_info_t		*player;
	unsigned int		effects, renderfx;
	float back, fwds;

	float timestep = cl.gametime-cl.oldgametime;
	struct itemtimer_s **timerlink, *timer;

	// bonus items rotate at a fixed rate
	autorotate = anglemod(cl.time*100);

	// brush models can auto animate their frames
	autoanim = 2*cl.time;

	memset (&ent, 0, sizeof(ent));

	for (timerlink = &cl.itemtimers; (timer=*timerlink); )
	{
		if (cl.time > timer->end)
		{
			*timerlink = timer->next;
			Z_Free(timer);
		}
		else
		{
			timerlink = &(*timerlink)->next;
			/*if (timer->entnum)
			{
				if (timer->entnum >= cl.maxlerpents)
					continue;
				le = &cl.lerpents[timer->entnum];
				if (le->sequence != cl.lerpentssequence)
					continue;
//				VectorCopy(le->origin, timer->origin);
			}*/
			R_AddItemTimer(timer->origin, cl.time*90 + timer->origin[0] + timer->origin[1] + timer->origin[2], timer->radius, (cl.time - timer->start) / timer->duration, timer->rgb);
		}
	}

	for (pnum = 0 ; pnum<frame->num_entities ; pnum++)
	{
		s1 = &clq2_parse_entities[(frame->parse_entities+pnum)&(MAX_PARSE_ENTITIES-1)];

		cent = &cl_entities[s1->number];

		effects = s1->effects;
		renderfx = s1->u.q2.renderfx;

		ent.rtype = RT_MODEL;
		ent.keynum = s1->number;

		ent.scale = s1->scale/16.f;
		ent.shaderRGBAf[0] = 1;
		ent.shaderRGBAf[1] = 1;
		ent.shaderRGBAf[2] = 1;
		ent.shaderRGBAf[3] = 1;
		ent.glowmod[0] = 1;
		ent.glowmod[1] = 1;
		ent.glowmod[2] = 1;
		ent.fatness = 0;
		ent.topcolour = 1;
		ent.bottomcolour = 1;
#ifdef HEXEN2
		ent.h2playerclass = 0;
#endif
		ent.playerindex = -1;
		ent.customskin = 0;

		// set frame
		if (effects & Q2EF_ANIM01)
			ent.framestate.g[FS_REG].frame[0] = autoanim & 1;
		else if (effects & Q2EF_ANIM23)
			ent.framestate.g[FS_REG].frame[0] = 2 + (autoanim & 1);
		else if (effects & Q2EF_ANIM_ALL)
			ent.framestate.g[FS_REG].frame[0] = autoanim;
		else if (effects & Q2EF_ANIM_ALLFAST)
			ent.framestate.g[FS_REG].frame[0] = cl.time / 100;
		else
			ent.framestate.g[FS_REG].frame[0] = s1->frame;

		// quad and pent can do different things on client
		if (effects & Q2EF_PENT)
		{
			effects &= ~Q2EF_PENT;
			effects |= Q2EF_COLOR_SHELL;
			renderfx |= Q2RF_SHELL_RED;
		}

		if (effects & Q2EF_QUAD)
		{
			effects &= ~Q2EF_QUAD;
			effects |= Q2EF_COLOR_SHELL;
			renderfx |= Q2RF_SHELL_BLUE;
		}
//======
// PMM
		if (effects & Q2EF_DOUBLE)
		{
			effects &= ~Q2EF_DOUBLE;
			effects |= Q2EF_COLOR_SHELL;
			renderfx |= Q2RF_SHELL_DOUBLE;
		}

		if (effects & Q2EF_HALF_DAMAGE)
		{
			effects &= ~Q2EF_HALF_DAMAGE;
			effects |= Q2EF_COLOR_SHELL;
			renderfx |= Q2RF_SHELL_HALF_DAM;
		}
// pmm
//======
		ent.framestate.g[FS_REG].frame[1] = cent->prev.frame;
		ent.framestate.g[FS_REG].lerpweight[0] = cl.lerpfrac;

		if (renderfx & (Q2RF_FRAMELERP|Q2RF_BEAM))
		{	// step origin discretely, because the frames
			// do the animation properly
			VectorCopy (cent->current.origin, ent.origin);
			VectorCopy (cent->current.u.q2.old_origin, ent.oldorigin);
		}
		else
		{	// interpolate origin
			for (i=0 ; i<3 ; i++)
			{
				ent.origin[i] = ent.oldorigin[i] = cent->prev.origin[i] + cl.lerpfrac * 
					(cent->current.origin[i] - cent->prev.origin[i]);
			}
		}

		if (ent.framestate.g[FS_REG].frame[0] != cent->oldframe)
		{
			ent.framestate.g[FS_REG].frame[1] = cent->oldframe;
			ent.framestate.g[FS_REG].lerpweight[0] = 10*(cl.time - cent->oldframetime);	//anim rate is still 1ms... could guess at it, but urgh

			if (ent.framestate.g[FS_REG].lerpweight[0] >= 1)
			{
				ent.framestate.g[FS_REG].lerpweight[0] = 1;
				cent->oldframe = ent.framestate.g[FS_REG].frame[0];
			}
		}

		ent.framestate.g[FS_REG].lerpweight[1] = 1-ent.framestate.g[FS_REG].lerpweight[0];

		if (renderfx & Q2EXRF_FLARE)
		{
/*This flag marks an entity as being rendered with a flare instead of the usual entity rendering. Flares overload some fields:
* `s.renderfx & RF_SHELL_RED` causes the flare to have an outer red rim.
* `s.renderfx & RF_SHELL_GREEN` causes the flare to have an outer green rim.
* `s.renderfx & RF_SHELL_BLUE` causes the flare to have an outer blue rim.
* `s.renderfx & RF_FLARE_LOCK_ANGLE` causes the flare to not rotate towards the viewer.
* `s.renderfx & RF_CUSTOMSKIN` causes the flare to use the custom image index in `s.frame`.
* `s.modelindex2` is the start distance of fading the flare out.
* `s.modelindex3` is the end distance of fading the flare out.
* `s.skinnum` is the RGBA of the flare.*/
			continue;
		}
		if (renderfx & Q2EXRF_CUSTOM_LIGHT)
		{	//0xRRGGBBXX apparently
			V_AddLight (ent.keynum, ent.origin,
						ent.framestate.g[FS_REG].frame[0]/*radius*/,
						((ent.skinnum >> 24) & 0xFF)/255.0/*r*/,
						((ent.skinnum >> 16) & 0xFF)/255.0/*g*/,
						((ent.skinnum >>  8) & 0xFF)/255.0/*b*/);
			continue;	//probably modelindex 1. we don't want to actually draw that. but probably do want to lerp it.
		}
		if (renderfx & Q2REX_CASTSHADOW)
		{
			CLQ2EX_UpdateRTLight(ent.keynum, ent.origin);
			continue;
		}

		// create a new entity
	
		// tweak the color of beams
		if ( renderfx & Q2RF_BEAM )
		{	// the four beam colors are encoded in 32 bits of skinnum (hack)
			ent.skinnum = (s1->skinnum >> ((rand() % 4)*8)) & 0xff;
			ent.shaderRGBAf[0] = ((d_8to24srgbtable[ent.skinnum & 0xFF] >>  0) & 0xFF)/255.0;
			ent.shaderRGBAf[1] = ((d_8to24srgbtable[ent.skinnum & 0xFF] >>  8) & 0xFF)/255.0;
			ent.shaderRGBAf[2] = ((d_8to24srgbtable[ent.skinnum & 0xFF] >> 16) & 0xFF)/255.0;
			ent.shaderRGBAf[3] = 0.30;
			ent.model = NULL;
			ent.framestate.g[FS_REG].lerpweight[0] = 1;
			ent.framestate.g[FS_REG].lerpweight[1] = 0;
			ent.rtype = RT_BEAM;
		}
		else
		{
			// set skin
			if (s1->modelindex == 255)
			{	// use custom player skin
				ent.skinnum = 0;

				player = &cl.players[(s1->skinnum&0xff)%cl.allocated_client_slots];
				ent.model = player->model;
				if (!ent.model || ent.model->loadstate != MLS_LOADED)	//we need to do better than this
				{
					ent.model = Mod_ForName("players/male/tris.md2", MLV_SILENT);
					ent.customskin = Mod_RegisterSkinFile("players/male/grunt.skin");
					if (!ent.customskin)
						ent.customskin = Mod_ReadSkinFile("players/male/grunt.skin", "replace \"\" \"players/male/grunt.pcx\"");
				}
				else
					ent.customskin = player->skinid;
				ent.playerindex = (s1->skinnum&0xff)%cl.allocated_client_slots;
/*				ci = &cl.clientinfo[s1->skinnum & 0xff];
//				ent.skin = ci->skin;
				ent.model = ci->model;
				if (!ent.skin || !ent.model)
				{
					ent.skin = cl.baseclientinfo.skin;
					ent.model = cl.baseclientinfo.model;
				}

//============
//PGM
				if (renderfx & Q2RF_USE_DISGUISE)
				{
					if(!strncmp((char *)ent.skin, "players/male", 12))
					{
						ent.skin = re.RegisterSkin ("players/male/disguise.pcx");
						ent.model = re.RegisterModel ("players/male/tris.md2");
					}
					else if(!strncmp((char *)ent.skin, "players/female", 14))
					{
						ent.skin = re.RegisterSkin ("players/female/disguise.pcx");
						ent.model = re.RegisterModel ("players/female/tris.md2");
					}
					else if(!strncmp((char *)ent.skin, "players/cyborg", 14))
					{
						ent.skin = re.RegisterSkin ("players/cyborg/disguise.pcx");
						ent.model = re.RegisterModel ("players/cyborg/tris.md2");
					}
				}*/
//PGM
//============
			}
			else
			{
				ent.skinnum = s1->skinnum;
//				ent.skin = NULL;
				ent.model = cl.model_precache[s1->modelindex];
			}
		}

		// only used for black hole model right now, FIXME: do better
		if (renderfx == RF_TRANSLUCENT)
			ent.shaderRGBAf[3] = 0.70;
		else if (s1->trans != 255)
		{
			renderfx |= RF_TRANSLUCENT;
			ent.shaderRGBAf[3] = s1->trans/255.0f;
		}
		else
			ent.shaderRGBAf[3] = 1;

		// render effects (fullbright, translucent, etc)
		if ((effects & Q2EF_COLOR_SHELL))
			ent.flags = 0;	// renderfx go on color shell entity
		else
			ent.flags = renderfx;

		// calculate angles
		if (effects & Q2EF_ROTATE)
		{	// some bonus items auto-rotate
			ent.angles[0] = 0;
			ent.angles[1] = autorotate;
			ent.angles[2] = 0;
		}
		// RAFAEL
		else if (effects & Q2EF_SPINNINGLIGHTS)
		{
			ent.angles[0] = 0;
			ent.angles[1] = anglemod(cl.time/2) + s1->angles[1];
			ent.angles[2] = 180;
			{
				vec3_t forward;
				vec3_t start;

				AngleVectors (ent.angles, forward, NULL, NULL);
				VectorMA (ent.origin, 64, forward, start);
				V_AddLight (ent.keynum, start, 100, 0.2, 0, 0);
			}
		}
		else
		{	// interpolate angles
			float	a1, a2;

			for (i=0 ; i<3 ; i++)
			{
				a1 = cent->current.angles[i];
				a2 = cent->prev.angles[i];
				ent.angles[i] = LerpAngle (a2, a1, cl.lerpfrac);
			}
		}

		ent.angles[0]*=r_meshpitch.value;	//q2 has it fixed. consistency...
		ent.angles[2]*=r_meshroll.value;	//h2 doesn't. consistency...

		if (s1->number == cl.playerview[0].playernum+1)	//woo! this is us!
		{
//			VectorCopy(cl.predicted_origin, ent.origin);
//			VectorCopy(cl.predicted_origin, ent.oldorigin);
			ent.flags |= RF_EXTERNALMODEL;	// only draw from mirrors
			renderfx |= RF_EXTERNALMODEL;

			if (effects & Q2EF_FLAG1)
				V_AddLight (ent.keynum, ent.origin, 225, 0.2, 0.05, 0.05);
			else if (effects & Q2EF_FLAG2)
				V_AddLight (ent.keynum, ent.origin, 225, 0.05, 0.05, 0.2);
			else if (effects & Q2EF_TAGTRAIL)						//PGM
				V_AddLight (ent.keynum, ent.origin, 225, 0.2, 0.2, 0.0);	//PGM
			else if (effects & Q2EF_TRACKERTRAIL)					//PGM
				V_AddLight (ent.keynum, ent.origin, 225, -0.2, -0.2, -0.2);	//PGM
		}

		// if set to invisible, skip
		if (!s1->modelindex)
			continue;

		if (effects & Q2EF_BFG)
		{
			ent.flags |= RF_TRANSLUCENT;
			ent.shaderRGBAf[3] = 0.30;
		}

		// RAFAEL
		if (effects & Q2EF_PLASMA)
		{
			ent.flags |= RF_TRANSLUCENT;
			ent.shaderRGBAf[3] = 0.6;
		}

		if (effects & Q2EF_SPHERETRANS)
		{
			ent.flags |= RF_TRANSLUCENT;
			// PMM - *sigh*  yet more EF overloading
			if (effects & Q2EF_TRACKERTRAIL)
				ent.shaderRGBAf[3] = 0.6;
			else
				ent.shaderRGBAf[3] = 0.3;
		}
//pmm

		/*lerp the ent now*/
		fwds = ent.framestate.g[FS_REG].lerpweight[0];
		back = ent.framestate.g[FS_REG].lerpweight[1];
		for (i = 0; i < 3; i++)
		{
			ent.origin[i] = ent.origin[i]*fwds + ent.oldorigin[i]*back;
		}

		if ((renderfx & Q2RF_IR_VISIBLE) && (r_refdef.flags & Q2RDF_IRGOGGLES))
		{
			//IR googles make ir visible ents visible in pure red.
			ent.shaderRGBAf[0] = 1;
			ent.shaderRGBAf[1] = 0;
			ent.shaderRGBAf[2] = 0;
			//bypasses world lighting
			ent.light_known = true;
			VectorSet(ent.light_avg, 1, 1, 1);
			VectorSet(ent.light_range, 0, 0, 0);
			//(yes, its a bit shit. not even a post-process thing)
		}

		// add to refresh list
		V_AddEntity (&ent);
		ent.light_known = false;
		ent.customskin = 0;


		// color shells generate a seperate entity for the main model
		if (effects & Q2EF_COLOR_SHELL)
		{
			// PMM - at this point, all of the shells have been handled
			// if we're in the rogue pack, set up the custom mixing, otherwise just
			// keep going

			// all of the solo colors are fine.  we need to catch any of the combinations that look bad
			// (double & half) and turn them into the appropriate color, and make double/quad something special
			if (renderfx & Q2RF_SHELL_HALF_DAM)
			{
				
				{
					// ditch the half damage shell if any of red, blue, or double are on
					if (renderfx & (Q2RF_SHELL_RED|Q2RF_SHELL_BLUE|Q2RF_SHELL_DOUBLE))
						renderfx &= ~Q2RF_SHELL_HALF_DAM;
				}
			}

			if (renderfx & Q2RF_SHELL_DOUBLE)
			{

				{
					// lose the yellow shell if we have a red, blue, or green shell
					if (renderfx & (Q2RF_SHELL_RED|Q2RF_SHELL_BLUE|Q2RF_SHELL_GREEN))
						renderfx &= ~Q2RF_SHELL_DOUBLE;
					// if we have a red shell, turn it to purple by adding blue
					if (renderfx & Q2RF_SHELL_RED)
						renderfx |= Q2RF_SHELL_BLUE;
					// if we have a blue shell (and not a red shell), turn it to cyan by adding green
					else if (renderfx & Q2RF_SHELL_BLUE)
					{
						// go to green if it's on already, otherwise do cyan (flash green)
						if (renderfx & Q2RF_SHELL_GREEN)
							renderfx &= ~Q2RF_SHELL_BLUE;
						else
							renderfx |= Q2RF_SHELL_GREEN;
					}
				}
			}
			// pmm
			ent.flags = renderfx;
			ent.shaderRGBAf[3] = 0.20;
			ent.shaderRGBAf[0] = (!!(renderfx & Q2RF_SHELL_RED));
			ent.shaderRGBAf[1] = (!!(renderfx & Q2RF_SHELL_GREEN));
			ent.shaderRGBAf[2] = (!!(renderfx & Q2RF_SHELL_BLUE));
			ent.forcedshader = R_RegisterCustom(NULL, "q2/shell", SUF_NONE, Shader_DefaultSkinShell, NULL);
			ent.fatness = 2;
			V_AddEntity (&ent);
			ent.fatness = 0;
		}
		ent.forcedshader = NULL;

//		ent.skin = NULL;		// never use a custom skin on others
		ent.skinnum = 0;
		ent.flags &= RF_EXTERNALMODEL;
		ent.shaderRGBAf[3] = 1;

		// duplicate for linked models
		if (s1->modelindex2)
		{
			if (s1->modelindex2 == 255)
			{	// custom weapon
				char *modelname;
				char *skin;
				ent.model=NULL;

				player = &cl.players[(s1->skinnum&0xff)%MAX_CLIENTS];
				modelname = InfoBuf_ValueForKey(&player->userinfo, "skin");
				if (!modelname[0])
					modelname = "male";
				skin = strchr(modelname, '/');
				if (skin) *skin = '\0';

				i = (s1->skinnum >> 8); // 0 is default weapon model
				if (i < 0 || i >= cl.numq2visibleweapons)
					i = 0;	//0 is always valid
				ent.model = Mod_ForName(va("players/%s/%s", modelname, cl.q2visibleweapons[i]), MLV_WARN);
				if (ent.model->loadstate == MLS_FAILED && i)
				{
					i = 0;
					ent.model = Mod_ForName(va("players/%s/%s", modelname, cl.q2visibleweapons[i]), MLV_WARN);
				}
				if (ent.model->loadstate == MLS_FAILED && strcmp(modelname, "male"))
					ent.model = Mod_ForName(va("players/%s/%s", "male", cl.q2visibleweapons[i]), MLV_WARN);
			}
			else
				ent.model = cl.model_precache[s1->modelindex2];

			// PMM - check for the defender sphere shell .. make it translucent
			// replaces the previous version which used the high bit on modelindex2 to determine transparency
/*			if (!Q_strcasecmp (cl.model_name[(s1->modelindex2)], "models/items/shell/tris.md2"))
			{
				ent.alpha = 0.32;
				ent.flags |= Q2RF_TRANSLUCENT;

				V_AddEntity (&ent);

				// make sure these get reset.
				ent.flags &= RF_EXTERNALMODEL;
				ent.shaderRGBAf[3] = 1;
			}
			else
*/			// pmm

			V_AddEntity (&ent);
		}
		if (s1->u.q2.modelindex3)
		{
			ent.model = cl.model_precache[s1->u.q2.modelindex3];
			V_AddEntity (&ent);
		}
		if (s1->u.q2.modelindex4)
		{
			ent.model = cl.model_precache[s1->u.q2.modelindex4];
			V_AddEntity (&ent);
		}

		if ( effects & Q2EF_POWERSCREEN )
		{
			ent.model = Mod_ForName("models/items/armor/effect/tris.md2", MLV_WARN);
			ent.framestate.g[FS_REG].frame[0] = 0;
			ent.framestate.g[FS_REG].frame[0] = 0;
			ent.flags |= (RF_TRANSLUCENT | Q2RF_SHELL_GREEN);
			ent.shaderRGBAf[3] = 0.30;
			V_AddEntity (&ent);
		}

		// add automatic particle trails
		if ( (effects&~Q2EF_ROTATE) )
		{
			if (effects & Q2EF_ROCKET)
			{
				//FIXME: cubemap orientation
				if (P_ParticleTrail(cent->lerp_origin, ent.origin, pt_q2[Q2RT_ROCKET], timestep, ent.keynum, NULL, &cent->trailstate))
					if (P_ParticleTrail(cent->lerp_origin, ent.origin, rt_rocket, timestep, ent.keynum, NULL, &cent->trailstate))
					{
						P_ParticleTrailIndex(cent->lerp_origin, ent.origin, P_INVALID, timestep, 0xdc, 4, &cent->trailstate);
						V_AddLight (ent.keynum, ent.origin, 200, 0.2, 0.1, 0.05);
					}
			}
			// PGM - Do not reorder EF_BLASTER and EF_HYPERBLASTER. 
			// EF_BLASTER | EF_TRACKER is a special case for EF_BLASTER2... Cheese!
			else if (effects & Q2EF_BLASTER)
			{
//PGM
				if (effects & Q2EF_TRACKER)	// lame... problematic?
				{
					if (P_ParticleTrail(cent->lerp_origin, ent.origin, pt_q2[Q2RT_BLASTERTRAIL2], timestep, ent.keynum, NULL, &cent->trailstate))
					{
						P_ParticleTrailIndex(cent->lerp_origin, ent.origin, P_INVALID, timestep, 0xd0, 1, &cent->trailstate);
						V_AddLight (ent.keynum, ent.origin, 200, 0, 0.2, 0);
					}
				}
				else
				{
					if (P_ParticleTrail(cent->lerp_origin, ent.origin, pt_q2[Q2RT_BLASTERTRAIL], timestep, ent.keynum, NULL, &cent->trailstate))
					{
						P_ParticleTrailIndex(cent->lerp_origin, ent.origin, P_INVALID, timestep, 0xe0, 1, &cent->trailstate);
						V_AddLight (ent.keynum, ent.origin, 200, 0.2, 0.2, 0);
					}
				}
//PGM
			}
			else if (effects & Q2EF_HYPERBLASTER)
			{
				if (effects & Q2EF_TRACKER)						// PGM	overloaded for blaster2.
					V_AddLight (ent.keynum, ent.origin, 200, 0, 0.2, 0);		// PGM
				else											// PGM
					V_AddLight (ent.keynum, ent.origin, 200, 0.2, 0.2, 0);
			}
			else if (effects & Q2EF_GIB)
			{
				if (P_ParticleTrail(cent->lerp_origin, ent.origin, pt_q2[Q2RT_GIB], timestep, ent.keynum, NULL, &cent->trailstate))
					if (P_ParticleTrail(cent->lerp_origin, ent.origin, rt_blood, timestep, ent.keynum, NULL, &cent->trailstate))
						P_ParticleTrailIndex(cent->lerp_origin, ent.origin, P_INVALID, timestep, 0xe8, 8, &cent->trailstate);
			}
			else if (effects & Q2EF_GRENADE)
			{
				if (P_ParticleTrail(cent->lerp_origin, ent.origin, pt_q2[Q2RT_GRENADE], timestep, ent.keynum, NULL, &cent->trailstate))
					if (P_ParticleTrail(cent->lerp_origin, ent.origin, rt_grenade, timestep, ent.keynum, NULL, &cent->trailstate))
						P_ParticleTrailIndex(cent->lerp_origin, ent.origin, P_INVALID, timestep, 4, 8, &cent->trailstate);
			}
			else if (effects & Q2EF_FLIES)
			{
				CLQ2_FlyEffect (cent, ent.origin);
			}
			else if (effects & Q2EF_BFG)
			{
				static int bfg_lightramp[6] = {300, 400, 600, 300, 150, 75};

				if (effects & Q2EF_ANIM_ALLFAST)
				{
					CLQ2_BfgParticles (cent, ent.origin);
					i = 200;
				}
				else
				{
					i = bfg_lightramp[s1->frame];
				}
				V_AddLight (ent.keynum, ent.origin, i, 0, 0.2, 0);
			}
			// RAFAEL
			else if (effects & Q2EF_TRAP)
			{
				ent.origin[2] += 32;
				P_ParticleTrail(cent->lerp_origin, ent.origin, pt_q2[Q2RT_TRAP], timestep, ent.keynum, NULL, &cent->trailstate);
			}
			else if (effects & Q2EF_FLAG1)
			{
				if (P_ParticleTrail(cent->lerp_origin, ent.origin, pt_q2[Q2RT_FLAG1], timestep, ent.keynum, NULL, &cent->trailstate))
				{
					P_ParticleTrailIndex(cent->lerp_origin, ent.origin, P_INVALID, timestep, 242, 1, &cent->trailstate);
					V_AddLight (ent.keynum, ent.origin, 225, 0.2, 0.05, 0.05);
				}
			}
			else if (effects & Q2EF_FLAG2)
			{
				if (P_ParticleTrail(cent->lerp_origin, ent.origin, pt_q2[Q2RT_FLAG2], timestep, ent.keynum, NULL, &cent->trailstate))
				{
					P_ParticleTrailIndex(cent->lerp_origin, ent.origin, P_INVALID, timestep, 115, 1, &cent->trailstate);
					V_AddLight (ent.keynum, ent.origin, 225, 0.05, 0.05, 0.2);
				}
			}
//======
//ROGUE
			else if (effects & Q2EF_TAGTRAIL)
			{
				if (P_ParticleTrail(cent->lerp_origin, ent.origin, pt_q2[Q2RT_TAGTRAIL], timestep, ent.keynum, NULL, &cent->trailstate))
				{
					P_ParticleTrailIndex(cent->lerp_origin, ent.origin, P_INVALID, timestep, 220, 1, &cent->trailstate);
					V_AddLight (ent.keynum, ent.origin, 225, 0.2, 0.2, 0.0);
				}
			}
			else if (effects & Q2EF_TRACKERTRAIL)
			{
				if (effects & Q2EF_TRACKER)
				{
					float intensity;

					intensity = 50 + (500 * (sin(cl.time/500.0) + 1.0));

					// FIXME - check out this effect in rendition
					V_AddLight (ent.keynum, ent.origin, intensity, -0.2, -0.2, -0.2);
				}
				else
				{
					CLQ2_Tracker_Shell (cent, cent->lerp_origin);
					V_AddLight (ent.keynum, ent.origin, 155, -0.2, -0.2, -0.2);
				}
			}
			else if (effects & Q2EF_TRACKER)
			{
				if (P_ParticleTrail(cent->lerp_origin, ent.origin, pt_q2[Q2RT_TRACKER], timestep, ent.keynum, NULL, &cent->trailstate))
				{
					P_ParticleTrailIndex(cent->lerp_origin, ent.origin, P_INVALID, timestep, 0, 1, &cent->trailstate);
					V_AddLight (ent.keynum, ent.origin, 200, -0.2, -0.2, -0.2);
				}
			}
//ROGUE
//======
			// RAFAEL
			else if (effects & Q2EF_GREENGIB)
			{
				if (P_ParticleTrail(cent->lerp_origin, ent.origin, pt_q2[Q2RT_GREENGIB], timestep, ent.keynum, NULL, &cent->trailstate))
					P_ParticleTrailIndex(cent->lerp_origin, ent.origin, P_INVALID, timestep, 219, 8, &cent->trailstate);
			}
			// RAFAEL
			else if (effects & Q2EF_IONRIPPER)
			{
				if (P_ParticleTrail(cent->lerp_origin, ent.origin, pt_q2[Q2RT_IONRIPPER], timestep, ent.keynum, NULL, &cent->trailstate))
				{
					P_ParticleTrailIndex(cent->lerp_origin, ent.origin, P_INVALID, timestep, 228, 4, &cent->trailstate);
					V_AddLight (ent.keynum, ent.origin, 100, 0.2, 0.1, 0.1);
				}
			}
			// RAFAEL
			else if (effects & Q2EF_BLUEHYPERBLASTER)
			{
				V_AddLight (ent.keynum, ent.origin, 200, 0, 0, 0.2);
			}
			// RAFAEL
			else if (effects & Q2EF_PLASMA)
			{
				if (effects & Q2EF_ANIM_ALLFAST)
				{
					P_ParticleTrail(cent->lerp_origin, ent.origin, pt_q2[Q2RT_PLASMA], timestep, ent.keynum, NULL, &cent->trailstate);
				}
				V_AddLight (ent.keynum, ent.origin, 130, 0.2, 0.1, 0.1);
			}
		}

		VectorCopy (ent.origin, cent->lerp_origin);
	}
}



/*
==============
CL_AddViewWeapon
==============
*/
static void CLQ2_AddViewWeapon (int seat, q2player_state_t *ps, q2player_state_t *ops)
{
	playerview_t *pv = &cl.playerview[seat];
	struct model_s *om = pv->vm.oldmodel;

	pv->vm.oldmodel = NULL;

	// allow the gun to be completely removed
	if (!r_drawviewmodel.value)
		return;

	if (!Cam_DrawViewModel(pv))
		return;

	// don't draw gun if in wide angle view
	if (ps->fov > 90 && !*scr_fov_viewmodel.string)
		return;

	//generate root matrix..
	VectorCopy(pv->simorg, r_refdef.weaponmatrix[3]);
	AngleVectors(pv->simangles, r_refdef.weaponmatrix[0], r_refdef.weaponmatrix[1], r_refdef.weaponmatrix[2]);
	VectorInverse(r_refdef.weaponmatrix[1]);
	memcpy(r_refdef.weaponmatrix_bob, r_refdef.weaponmatrix, sizeof(r_refdef.weaponmatrix_bob));

	if (om != cl.model_precache[ps->gunindex])
	{
		pv->vm.prevframe = pv->vm.oldframe = ps->gunframe;
		pv->vm.lerptime = pv->vm.oldlerptime = cl.oldgametime;
	}
	pv->vm.oldmodel = cl.model_precache[ps->gunindex];
	if (!pv->vm.oldmodel)
		return;

	if (ps->gunframe != pv->vm.prevframe)
	{
		pv->vm.oldframe = pv->vm.prevframe;
		pv->vm.oldlerptime = pv->vm.lerptime;

		pv->vm.prevframe = ps->gunframe;
		pv->vm.lerptime = cl.time;
	}
}


/*
===============
CL_CalcViewValues

Sets r_refdef view values
===============
*/
void CLQ2_CalcViewValues (int seat)
{
	int			i;
	float		lerp, backlerp;
	q2frame_t		*oldframe;
	q2player_state_t	*ps, *ops;
	playerview_t *pv = &cl.playerview[seat];

	r_refdef.areabitsknown = true;
	memcpy(r_refdef.areabits, cl.q2frame.seat[seat].areabits, sizeof(r_refdef.areabits));

	r_refdef.useperspective = true;
	r_refdef.mindist = bound(0.1, gl_mindist.value, 4);
	r_refdef.maxdist = gl_maxdist.value;

	// find the previous frame to interpolate from
	ps = &cl.q2frame.seat[seat].playerstate;
	i = (cl.q2frame.serverframe - 1) & Q2UPDATE_MASK;
	oldframe = &cl.q2frames[i];
	if (oldframe->serverframe != cl.q2frame.serverframe-1 || !oldframe->valid)
		oldframe = &cl.q2frame;		// previous frame was dropped or involid
	ops = &oldframe->seat[seat].playerstate;

	// see if the player entity was teleported this frame
	if ( abs(ops->pmove.origin[0] - ps->pmove.origin[0]) > 256*8
		|| abs(ops->pmove.origin[1] - ps->pmove.origin[1]) > 256*8
		|| abs(ops->pmove.origin[2] - ps->pmove.origin[2]) > 256*8)
		ops = ps;		// don't interpolate

	lerp = cl.lerpfrac;

	// calculate the origin
	if (cl.worldmodel && (!cl_nopred.value) && !(cl.q2frame.seat[seat].playerstate.pmove.pm_flags & Q2PMF_NO_PREDICTION) && !cls.demoplayback)
	{	// use predicted values
		float	delta;

		backlerp = 1.0 - lerp;
		for (i=0 ; i<3 ; i++)
		{
			pv->simorg[i] = pv->predicted_origin[i] + ops->viewoffset[i] 
				+ lerp * (ps->viewoffset[i] - ops->viewoffset[i])
				- backlerp * pv->prediction_error[i];
		}

		// smooth out stair climbing
		delta = realtime - pv->predicted_step_time;
		if (delta < 0.1)
			pv->simorg[2] -= pv->predicted_step * (0.1 - delta)*10;
	}
	else
	{	// just use interpolated values
		for (i=0 ; i<3 ; i++)
			pv->simorg[i] =  ops->pmove.origin[i]*0.125 + ops->viewoffset[i]
				+ lerp *	( ps->pmove.origin[i]*0.125 +  ps->viewoffset[i]
				-			(ops->pmove.origin[i]*0.125 + ops->viewoffset[i]) );
	}

	// if not running a demo or on a locked frame, add the local angle movement
	if (cl.worldmodel && ps->pmove.pm_type < Q2PM_DEAD && !cls.demoplayback)
	{	// use predicted values
		for (i=0 ; i<3 ; i++)
			pv->simangles[i] = pv->predicted_angles[i];
	}
	else
	{	// just use interpolated values
		for (i=0 ; i<3 ; i++)
			pv->simangles[i] = LerpAngle (ops->viewangles[i], ps->viewangles[i], lerp);
	}

	for (i=0 ; i<3 ; i++)
		pv->simangles[i] += v_gunkick_q2.value * LerpAngle (ops->kick_angles[i], ps->kick_angles[i], lerp);

//	VectorCopy(r_refdef.viewangles, cl.viewangles);

//	AngleVectors (r_refdef.viewangles, v_forward, v_right, v_up);

	// interpolate field of view
	pv->forcefov = ops->fov + lerp * (ps->fov - ops->fov);
	pv->handedness = atoi(InfoBuf_ValueForKey(&cls.userinfo[seat], "hand"));

	//do interpolate blend alpha, but only if the rgb didn't change
	// don't interpolate blend color
	for (i=0 ; i<3 ; i++)
		pv->screentint[i] = ps->blend[i];
	if (ps->blend[0] == ops->blend[0] && ps->blend[1] == ops->blend[1] && ps->blend[2] == ops->blend[2] && (!ps->blend[3]) == (!ops->blend[3]))
		pv->screentint[3] = (ops->blend[3] + lerp * (ps->blend[3]-ops->blend[3]))*gl_cshiftenabled.value;
	else
		pv->screentint[3] = ps->blend[3]*gl_cshiftenabled.value;

	// add the weapon
	CLQ2_AddViewWeapon (seat, ps, ops);
}

/*
===============
CL_AddEntities

Emits all entities, particles, and lights to the refresh
===============
*/
void CLQ2_AddEntities (void)
{
	int seat;
	if (cls.state != ca_active)
		return;

	cl.lerpfrac = 1.0 - (cl.q2frame.servertime/1000.0 - cl.time) * cl.q2svnetrate;
//	Con_Printf("%g: %g\n", cl.q2frame.servertime - (cl.time*1000), cl.lerpfrac);
	cl.lerpfrac = bound(0, cl.lerpfrac, 1);

	for (seat = 0; seat < cl.splitclients; seat++)
		CLQ2_CalcViewValues (seat);
	CLQ2_AddPacketEntities (&cl.q2frame);
#if 0
	CLQ2_AddProjectiles ();
#endif
	CL_UpdateTEnts ();

#ifdef _DEBUG
	if (chase_active.ival)
	{
		playerview_t *pv = &cl.playerview[0];
		vec3_t axis[3];
		vec3_t camorg;
//		trace_t tr;
		AngleVectors(r_refdef.viewangles, axis[0], axis[1], axis[2]);
		VectorMA(r_refdef.vieworg, -chase_back.value, axis[0], camorg);
		VectorMA(camorg, -chase_up.value, pv->gravitydir, camorg);
//		if (cl.worldmodel && cl.worldmodel->funcs.NativeTrace(cl.worldmodel, 0, NULLFRAMESTATE, NULL, r_refdef.vieworg, camorg, vec3_origin, vec3_origin, true, MASK_WORLDSOLID, &tr))
		VectorCopy(camorg, r_refdef.vieworg);

		V_EditExternalModels(0, NULL, 0);
	}
#endif

#ifdef RTLIGHTS
	R_EditLights_DrawLights();
#endif
}

#endif
