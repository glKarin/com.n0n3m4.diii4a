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
// cl_tent.c -- client side temporary entities

#include "quakedef.h"
#include "particles.h"
entity_state_t *CL_FindPacketEntity(int num);

#ifdef Q2CLIENT
static const char	*q2efnames[] =
{
	"TEQ2_GUNSHOT",
	"TEQ2_BLOOD",
	"TEQ2_BLASTER",
	"TEQ2_RAILTRAIL",
	"TEQ2_SHOTGUN",
	"TEQ2_EXPLOSION1",
	"TEQ2_EXPLOSION2",
	"TEQ2_ROCKET_EXPLOSION",
	"TEQ2_GRENADE_EXPLOSION",
	"TEQ2_SPARKS",
	NULL,//"TEQ2_SPLASH",
	"TEQ2_BUBBLETRAIL",
	"TEQ2_SCREEN_SPARKS",
	"TEQ2_SHIELD_SPARKS",
	"TEQ2_BULLET_SPARKS",
	NULL,//"TEQ2_LASER_SPARKS",
	NULL,//"TEQ2_PARASITE_ATTACK",
	"TEQ2_ROCKET_EXPLOSION_WATER",
	"TEQ2_GRENADE_EXPLOSION_WATER",
	NULL,//"TEQ2_MEDIC_CABLE_ATTACK",
	"TEQ2_BFG_EXPLOSION",
	"TEQ2_BFG_BIGEXPLOSION",
	"TEQ2_BOSSTPORT",
	NULL,//"TEQ2_BFG_LASER",
	NULL,//"TEQ2_GRAPPLE_CABLE",
	"TEQ2_WELDING_SPARKS",
	"TEQ2_GREENBLOOD",
	"TEQ2_BLUEHYPERBLASTER",
	"TEQ2_PLASMA_EXPLOSION",
	"TEQ2_TUNNEL_SPARKS",
	"TEQ2_BLASTER2",
	"TEQ2_RAILTRAIL2",	//not implemented in vanilla
	NULL,//"TEQ2_FLAME",	//not implemented in vanilla
	NULL,//"TEQ2_LIGHTNING",
	"TEQ2_DEBUGTRAIL",
	"TEQ2_PLAIN_EXPLOSION",
	"TEQ2_FLASHLIGHT",
	"TEQ2_FORCEWALL",
	NULL,//"TEQ2_HEATBEAM",
	NULL,//"TEQ2_MONSTER_HEATBEAM",
	NULL,//"TEQ2_STEAM",
	"TEQ2_BUBBLETRAIL2",
	"TEQ2_MOREBLOOD",
	"TEQ2_HEATBEAM_SPARKS",
	"TEQ2_HEATBEAM_STEAM",
	"TEQ2_CHAINFIST_SMOKE",
	"TEQ2_ELECTRIC_SPARKS",
	"TEQ2_TRACKER_EXPLOSION",
	"TEQ2_TELEPORT_EFFECT",
	"TEQ2_DBALL_GOAL",
	NULL,//"TEQ2_WIDOWBEAMOUT",
	NULL,//"TEQ2_NUKEBLAST",
	"TEQ2_WIDOWSPLASH",
	"TEQ2_EXPLOSION1_BIG",
	"TEQ2_EXPLOSION1_NP",
	"TEQ2_FLECHETTE",

//RERELEASE
	"TEQ2EX_BLUEHYPERBLASTER",
	"TEQ2EX_BFGZAP",
	"TEQ2EX_BERSERK_SLAM",
	"TEQ2EX_GRAPPLE_CABLE_2",
	"TEQ2EX_POWER_SPLASH",
	"TEQ2EX_LIGHTNING_BEAM",
	"TEQ2EX_EXPLOSION1_NL",
	"TEQ2EX_EXPLOSION2_NL",
//RERELEASE


//	NULL,//"TEQ2_CR_LEADERBLASTER",
//	NULL,//"TEQ2_CR_BLASTER_MUZZLEFLASH",
//	NULL,//"TEQ2_CR_BLUE_MUZZLEFLASH",
//	NULL,//"TEQ2_CR_SMART_MUZZLEFLASH",
//	NULL,//"TEQ2_CR_LEADERFIELD",
//	NULL,//"TEQ2_CR_DEATHFIELD",
//	NULL,//"TEQ2_CR_BLASTERBEAM",
//	NULL,//"TEQ2_CR_STAIN",
	NULL,//"TEQ2_CR_FIRE",
	NULL,//"TEQ2_CR_CABLEGUT",
	NULL,//"TEQ2_CR_SMOKE",

	//the rest have no specific value meanings

	//slashes block
	"te_splashunknown",
	"te_splashsparks",
	"te_splashbluewater",
	"te_splashbrownwater",
	"te_splashslime",
	"te_splashlava",
	"te_splashblood",
	"te_splashelect",

	"TR_BLASTERTRAIL",
	"TR_BLASTERTRAIL2",
	"TRQ2_GIB",
	"TRQ2_GREENGIB",
	"TRQ2_ROCKET",
	"TRQ2_GRENADE",

	"TR_TRAP",
	"TR_FLAG1",
	"TR_FLAG2",
	"TR_TAGTRAIL",
	"TR_TRACKER",
	"TR_IONRIPPER",
	"TR_PLASMA",

	"EF_BFGPARTICLES",
	"EF_FLIES",
	"EF_TRAP",
	"EF_TRACKERSHELL",

	
	"ev_item_respawn",
	"ev_player_teleport",
	"ev_footstep",
	"ev_other_footstep",
	"ev_ladder_step",
};
int pt_q2[sizeof(q2efnames)/sizeof(q2efnames[0])];
#endif

int
	pt_muzzleflash=P_INVALID,
	pt_gunshot=P_INVALID,
	ptdp_gunshotquad=P_INVALID,
	pt_spike=P_INVALID,
	ptdp_spikequad=P_INVALID,
	pt_superspike=P_INVALID,
	ptdp_superspikequad=P_INVALID,
	pt_wizspike=P_INVALID,
	pt_knightspike=P_INVALID,
	pt_explosion=P_INVALID,
	ptdp_explosionquad=P_INVALID,
	pt_tarexplosion=P_INVALID,
	pt_teleportsplash=P_INVALID,
	pt_lavasplash=P_INVALID,
	ptdp_smallflash=P_INVALID,
	ptdp_flamejet=P_INVALID,
	ptdp_flame=P_INVALID,
	ptdp_blood=P_INVALID,
	ptdp_spark=P_INVALID,
	ptdp_plasmaburn=P_INVALID,
	ptdp_tei_g3=P_INVALID,
	ptdp_tei_smoke=P_INVALID,
	ptdp_tei_bigexplosion=P_INVALID,
	ptdp_tei_plasmahit=P_INVALID,
	ptdp_stardust=P_INVALID,
	rt_rocket=P_INVALID,
	rt_grenade=P_INVALID,
	rt_blood=P_INVALID,
	rt_wizspike=P_INVALID,
	rt_slightblood=P_INVALID,
	rt_knightspike=P_INVALID,
	rt_vorespike=P_INVALID,
	rtdp_nexuizplasma=P_INVALID,
	rtdp_glowtrail=P_INVALID,

	ptqw_gunshot=P_INVALID,
	ptqw_blood=P_INVALID,
	ptqw_lightningblood=P_INVALID,

	rtqw_railtrail=P_INVALID,
	ptfte_bullet=P_INVALID,
	ptfte_superbullet=P_INVALID;

typedef struct tentmodels_s {
	/*static stuff*/
	char *modelname;
	char *beamparticles;
	char *beamimpactparticle;
	char *beamimpactmodel;
	float impactscale;
	struct tentmodels_s *partner;
	int bflags;

	/*cached stuff*/
	model_t *model;
	model_t *impactmodel;
	int ef_beam;
	int ef_impact;
} tentmodels_t;

struct beam_s {
	tentmodels_t *info;
	int		entity;
	short	tag;
//	short	pad;
//	qbyte	pad;
	qbyte	bflags;
	qbyte	type;
	qbyte	skin;
	unsigned int rflags;

	float	endtime;
	float	alpha;
	vec3_t	start, end;
	vec3_t	offset;	//when attached, this is the offset from the owning entity. probably only z is meaningful.
//	int		particlecolour;	//some effects have specific colours. which is weird.
	trailkey_t trailstate;
	trailkey_t emitstate;
};

beam_t		*cl_beams;
int			cl_beams_max;

typedef struct
{
	vec3_t	origin;
	vec3_t	oldorigin;
	vec3_t	velocity;

	int firstframe;
	int numframes;

	int		type;
	vec3_t	angles;
	vec3_t	avel;
	vec3_t	rgb;
	int		flags;
	float	gravity;
	float	startalpha;
	float	endalpha;
	float	scale;
	float	start;
	float	framerate;
	model_t	*model;
	int skinnum;
	int		traileffect;
	trailkey_t trailstate;
} explosion_t;

static explosion_t	*cl_explosions;
static int			cl_explosions_max;

static int explosions_running;
static int beams_running;

static tentmodels_t beamtypes[] =
{
	{"progs/bolt.mdl",							"TE_LIGHTNING1",				"TE_LIGHTNING1_END"},
	{"progs/bolt2.mdl",							"TE_LIGHTNING2",				"TE_LIGHTNING2_END"},
	{"progs/bolt3.mdl",							"TE_LIGHTNING3",				"TE_LIGHTNING3_END"},
	{"progs/beam.mdl",							"te_beam",						"te_beam_end"},	//a CTF addition, but has other potential uses, sadly.

	{"models/monsters/parasite/segment/tris.md2","te_parasite_attack",			"te_parasite_attack_end"},
	{"models/ctf/segment/tris.md2",				"te_grapple_cable",				"te_grapple_cable_end"},
	{"models/proj/beam/tris.md2",				"te_heatbeam",					"te_heatbeam_end"},
	{"models/proj/lightning/tris.md2",			"TE_LIGHTNING2",				"TE_LIGHTNING2_END"},

	{"models/stltng2.mdl",						"te_stream_lightning_small",	NULL},
	{"models/stchain.mdl",						"te_stream_chain",				NULL},
	{"models/stsunsf1.mdl",						"te_stream_sunstaff1",			NULL,	"models/stsunsf3.mdl", 0.8, &beamtypes[BT_H2SUNSTAFF1_SUB]}, //the core beam
	{"models/stsunsf2.mdl",						NULL,							NULL,	"models/stsunsf4.mdl", 1.5},//the transparenty bit.
	{"models/stsunsf1.mdl",						"te_stream_sunstaff2",			NULL},
	{"models/stlghtng.mdl",						"te_stream_lightning",			NULL},
	{"models/stclrbm.mdl",						"te_stream_colorbeam",			NULL},
	{"models/stice.mdl",						"te_stream_icechunks",			NULL},
	{"models/stmedgaz.mdl",						"te_stream_gaze",				NULL},
	{"models/fambeam.mdl",						"te_stream_famine",				NULL},
};


static sfx_t			*cl_sfx_wizhit;
sfx_t			*cl_sfx_knighthit;
static sfx_t			*cl_sfx_tink1;
static sfx_t			*cl_sfx_ric1;
static sfx_t			*cl_sfx_ric2;
static sfx_t			*cl_sfx_ric3;
sfx_t			*cl_sfx_r_exp3;

cvar_t	cl_expsprite = CVARFD("cl_expsprite", "1", CVAR_ARCHIVE, "Display a central sprite in explosion effects. QuakeWorld typically does so, NQ mods should not (which is problematic when played with the qw protocol).");
cvar_t  r_explosionlight = CVARFC("r_explosionlight", "1", CVAR_ARCHIVE, Cvar_Limiter_ZeroToOne_Callback);
static cvar_t  r_explosionlight_colour = CVARFD("r_explosionlight_colour", "4.0 2.0 0.5", CVAR_ARCHIVE, "This controls the initial RGB values of EF_EXPLOSION effects.");
static cvar_t  r_explosionlight_fade = CVARFD("r_explosionlight_fade", "0.784 0.92 0.48", CVAR_ARCHIVE, "This controls the per-second RGB decay values of EF_EXPLOSION effects.");
cvar_t  r_dimlight_colour = CVARFD("r_dimlight_colour", "2.0 1.0 0.5 200", CVAR_ARCHIVE, "The red, green, blue, radius values for EF_DIMLIGHT effects (used for quad+pent in vanilla quake).");
cvar_t  r_brightlight_colour = CVARFD("r_brightlight_colour", "2.0 1.0 0.5 400", CVAR_ARCHIVE, "The red, green, blue, radius values for EF_BRIGHTLIGHT effects (unused in vanilla quake).");
cvar_t  r_redlight_colour = CVARFD("r_redlight_colour", "3.0 0.5 0.5 200", CVAR_ARCHIVE, "The red, green, blue, radius values for EF_RED effects (typically used for pentagram in quakeworld).");
cvar_t  r_greenlight_colour = CVARFD("r_greenlight_colour", "0.5 3.0 0.5 200", CVAR_ARCHIVE, "The red, green, blue, radius values for EF_GREEN effects (rarely used).");
cvar_t  r_bluelight_colour = CVARFD("r_bluelight_colour", "0.5 0.5 3.0 200", CVAR_ARCHIVE, "The red, green, blue, radius values for EF_BLUE effects (typically used for quad-damage in quakeworld)");
cvar_t  r_rocketlight_colour = CVARFD("r_rocketlight_colour", "2.0 1.0 0.25 200", CVAR_ARCHIVE, "This controls the RGB+radius values of MF_ROCKET effects.");
cvar_t  r_muzzleflash_colour = CVARFD("r_muzzleflash_colour", "1.5 1.3 1.0 200", CVAR_ARCHIVE, "This controls the initial RGB+radius of EF_MUZZLEFLASH/svc_muzzleflash effects.");
cvar_t  r_muzzleflash_fade = CVARFD("r_muzzleflash_fade", "1.5 0.75 0.375 1000", CVAR_ARCHIVE, "This controls the per-second RGB+radius decay of EF_MUZZLEFLASH/svc_muzzleflash effects.");
cvar_t	cl_truelightning = CVARFD("cl_truelightning", "0",	CVAR_SEMICHEAT, "Manipulate the end position of the player's own beams, to hide lag. Can be set to fractional values to reduce the effect.");
static cvar_t  cl_beam_trace = CVARD("cl_beam_trace", "0", "Clips the length of any beams according to any walls they may have impacted.");
static cvar_t  cl_beam_alpha = CVARAFD("cl_beam_alpha", "1", "r_shaftalpha", 0, "Specifies the translucency of the lightning beam segments.");
static cvar_t	cl_legacystains = CVARD("cl_legacystains", "1", "WARNING: this cvar will default to 0 and later removed at some point");	//FIXME: do as the description says!
static cvar_t	cl_shaftlight = CVAR("gl_shaftlight", "0.8");
static cvar_t	cl_part_density_fade_start = CVARD("cl_part_density_fade_start", "1024", "Specifies the distance at which ssqc's pointparticles will start to get less dense.");
static cvar_t	cl_part_density_fade = CVARD("cl_part_density_fade", "1024", "Specifies the distance over which ssqc pointparticles density fades from all to none. If this is set to 0 then particles will spawn at their normal density regardless of location on the map.");

typedef struct {
	sfx_t **sfx;
	char *efname;
} tentsfx_t;
tentsfx_t tentsfx[] =
{
	{&cl_sfx_wizhit, "wizard/hit.wav"},
	{&cl_sfx_knighthit, "hknight/hit.wav"},
	{&cl_sfx_tink1, "weapons/tink1.wav"},
	{&cl_sfx_ric1, "weapons/ric1.wav"},
	{&cl_sfx_ric2, "weapons/ric2.wav"},
	{&cl_sfx_ric3, "weapons/ric3.wav"},
	{&cl_sfx_r_exp3, "weapons/r_exp3.wav"}
};

vec3_t playerbeam_end[MAX_SPLITS];

typedef struct associatedeffect_s
{
	struct associatedeffect_s *next;
	char mname[MAX_QPATH];
	char pname[MAX_QPATH];
	enum
	{
		AE_TRAIL,
		AE_EMIT,
	} type;
	unsigned int meflags;
} associatedeffect_t;
associatedeffect_t *associatedeffect;
char part_parsenamespace[MAX_QPATH];
void CL_AssociateEffect_f(void)
{
	char *modelname = Cmd_Argv(1);
	char *effectname = Cmd_Argv(2);
	int type, i;
	unsigned int flags = 0;
	struct associatedeffect_s *ae;
	if (Cmd_Argc() == 1)
	{
		for(ae = associatedeffect; ae; ae = ae->next)
		{
			Con_Printf("^h%c^h %s: %s%s%s\n", (ae->type==AE_TRAIL)?'t':'e', ae->mname, ae->pname,
				(ae->meflags & MDLF_EMITREPLACE)?" (replace)":"",
				(ae->meflags & MDLF_EMITFORWARDS)?" (foward)":"");
		}
		return;
	}
	if (!strcmp(Cmd_Argv(0), "r_trail"))
		type = AE_TRAIL;
	else
	{
		type = AE_EMIT;

		for (i = 3; i < Cmd_Argc(); i++)
		{
			const char *fn = Cmd_Argv(i);
			if (!strcmp(fn, "replace") || !strcmp(fn, "1"))
				flags |= MDLF_EMITREPLACE;
			else if (!strcmp(fn, "forwards") || !strcmp(fn, "forward"))
				flags |= MDLF_EMITFORWARDS;
			else if (!strcmp(fn, "0"))
				;	//1 or 0 are legacy, meaning replace or not
			else
				Con_DPrintf("%s %s: unknown flag %s\n", Cmd_Argv(0), modelname, fn);
		}
	}

	if (
		strstr(modelname, "player") ||
		strstr(modelname, "eyes") ||
		strstr(modelname, "flag") ||
		strstr(modelname, "tf_stan") ||
		strstr(modelname, ".bsp") ||
		strstr(modelname, "turr"))
	{
		Con_Printf("Sorry: Not allowed to attach effects to model \"%s\"\n", modelname);
		return;
	}

	if (*part_parsenamespace && !strchr(effectname, '.'))
		effectname = va("%s.%s", part_parsenamespace, effectname);

	if (strlen (modelname) >= MAX_QPATH || strlen(effectname) >= MAX_QPATH)
		return;

	/*replace the old one if it exists*/
	for(ae = associatedeffect; ae; ae = ae->next)
	{
		if (!strcmp(ae->mname, modelname))
			if ((ae->type==AE_TRAIL) == (type==AE_TRAIL))
				break;
	}
	if (!ae)
	{
		ae = Z_Malloc(sizeof(*ae));
		strcpy(ae->mname, modelname);
		ae->next = associatedeffect;
		associatedeffect = ae;
	}
	ae->type = type;
	ae->meflags = flags;
	strcpy(ae->pname, effectname);

	if (pe)
		CL_RegisterParticles();
}

void CL_InitTEntSounds (void)
{
	int i;
	for (i = 0; i < sizeof(tentsfx)/sizeof(tentsfx[0]); i++)
	{
		if (COM_FCheckExists(va("sound/%s", tentsfx[i].efname)))
			*tentsfx[i].sfx = S_PrecacheSound (tentsfx[i].efname);
		else
			*tentsfx[i].sfx = NULL;
	}
}

/*
=================
CL_ParseTEnts
=================
*/
void CL_InitTEnts (void)
{
	int i;
	for (i = 0; i < sizeof(tentsfx)/sizeof(tentsfx[0]); i++)
		*tentsfx[i].sfx = NULL;

	Cmd_AddCommand("r_effect", CL_AssociateEffect_f);
	Cmd_AddCommand("r_trail", CL_AssociateEffect_f);

	Cvar_Register (&cl_expsprite, "Temporary entity control");
	Cvar_Register (&cl_truelightning, "Temporary entity control");
	Cvar_Register (&cl_beam_trace, "Temporary entity control");
	Cvar_Register (&cl_beam_alpha, "Temporary entity control");
	Cvar_Register (&r_explosionlight, "Temporary entity control");
	Cvar_Register (&r_explosionlight_colour, "Temporary entity control");
	Cvar_Register (&r_explosionlight_fade, "Temporary entity control");
	Cvar_Register (&r_muzzleflash_colour, "Temporary entity control");
	Cvar_Register (&r_muzzleflash_fade, "Temporary entity control");
	Cvar_Register (&r_dimlight_colour, "Temporary entity control");
	Cvar_Register (&r_redlight_colour, "Temporary entity control");
	Cvar_Register (&r_greenlight_colour, "Temporary entity control");
	Cvar_Register (&r_bluelight_colour, "Temporary entity control");
	Cvar_Register (&r_brightlight_colour, "Temporary entity control");
	Cvar_Register (&r_rocketlight_colour, "Temporary entity control");
	Cvar_Register (&cl_legacystains, "Temporary entity control");
	Cvar_Register (&cl_shaftlight, "Temporary entity control");

	Cvar_Register (&cl_part_density_fade_start, "Temporary entity control");
	Cvar_Register (&cl_part_density_fade, "Temporary entity control");
}

void CL_ShutdownTEnts (void)
{
	struct associatedeffect_s *ae;
	while(associatedeffect)
	{
		ae = associatedeffect;
		associatedeffect = ae->next;
		BZ_Free(ae);
	}
}

void CL_ClearTEntParticleState (void)
{
	int i;
	for (i = 0; i < cl_beams_max; i++)
	{
		if (cl_beams[i].trailstate)
			P_DelinkTrailstate(&(cl_beams[i].trailstate));
		if (cl_beams[i].emitstate)
			P_DelinkTrailstate(&(cl_beams[i].emitstate));
	}
}

void P_LoadedModel(model_t *mod)
{
	struct associatedeffect_s *ae;

	mod->particleeffect = P_INVALID;
	mod->particletrail = P_INVALID;
	mod->engineflags &= ~(MDLF_EMITREPLACE|MDLF_EMITFORWARDS);
	mod->engineflags |= MDLF_RECALCULATERAIN;
	for(ae = associatedeffect; ae; ae = ae->next)
	{
		if (!strcmp(ae->mname, mod->name))
		{
			switch(ae->type)
			{
			case AE_TRAIL:
				mod->particletrail = P_FindParticleType(ae->pname);
				break;
			case AE_EMIT:
				mod->particleeffect = P_FindParticleType(ae->pname);
				mod->engineflags |= ae->meflags;
				break;
			}
		}
	}
	if (mod->particletrail == P_INVALID)
		P_DefaultTrail(0, mod->flags, &mod->particletrail, &mod->traildefaultindex);
}

void CL_RefreshCustomTEnts(void);
void CL_RegisterParticles(void)
{
	model_t *mod;
	extern model_t	*mod_known;
	extern int		mod_numknown;
	int i;
	for (i=0 , mod=mod_known ; i<mod_numknown ; i++, mod++)
	{
		if (mod->loadstate == MLS_LOADED)
		{
			P_LoadedModel(mod);
		}
	}

	pt_muzzleflash			= P_FindParticleType("TE_MUZZLEFLASH");
	pt_gunshot				= P_FindParticleType("TE_GUNSHOT");	/*shotgun*/
	ptdp_gunshotquad		= P_FindParticleType("TE_GUNSHOTQUAD");	/*DP: quadded shotgun*/
	pt_spike				= P_FindParticleType("TE_SPIKE");	/*nailgun*/
	ptdp_spikequad			= P_FindParticleType("TE_SPIKEQUAD");	/*DP: quadded nailgun*/
	pt_superspike			= P_FindParticleType("TE_SUPERSPIKE");	/*nailgun*/
	ptdp_superspikequad		= P_FindParticleType("TE_SUPERSPIKEQUAD");	/*DP: quadded nailgun*/
	pt_wizspike				= P_FindParticleType("TE_WIZSPIKE");	//scrag missile impact
	pt_knightspike			= P_FindParticleType("TE_KNIGHTSPIKE"); //hellknight missile impact
	pt_explosion			= P_FindParticleType("TE_EXPLOSION");/*rocket/grenade launcher impacts/far too many things*/
	ptdp_explosionquad		= P_FindParticleType("TE_EXPLOSIONQUAD");	/*nailgun*/
	pt_tarexplosion			= P_FindParticleType("TE_TAREXPLOSION");//tarbaby/spawn dying.
	pt_teleportsplash		= P_FindParticleType("TE_TELEPORT");/*teleporters*/
	pt_lavasplash			= P_FindParticleType("TE_LAVASPLASH");	//e1m7 boss dying.
	ptdp_smallflash			= P_FindParticleType("TE_SMALLFLASH");	//DP:
	ptdp_flamejet			= P_FindParticleType("TE_FLAMEJET");	//DP:
	ptdp_flame				= P_FindParticleType("EF_FLAME");	//DP:
	ptdp_blood				= P_FindParticleType("TE_BLOOD"); /*when you hit something with the shotgun/axe/nailgun - nq uses the general particle builtin*/
	ptdp_spark				= P_FindParticleType("TE_SPARK");//DPTE_SPARK
	ptdp_plasmaburn			= P_FindParticleType("TE_PLASMABURN");
	ptdp_tei_g3				= P_FindParticleType("TE_TEI_G3");
	ptdp_tei_smoke			= P_FindParticleType("TE_TEI_SMOKE");
	ptdp_tei_bigexplosion	= P_FindParticleType("TE_TEI_BIGEXPLOSION");
	ptdp_tei_plasmahit		= P_FindParticleType("TE_TEI_PLASMAHIT");
	ptdp_stardust			= P_FindParticleType("EF_STARDUST");
	rt_rocket				= P_FindParticleType("TR_ROCKET");	/*rocket trail*/
	rt_grenade				= P_FindParticleType("TR_GRENADE");	/*grenade trail*/
	rt_blood				= P_FindParticleType("TR_BLOOD");	/*blood trail*/
	rt_wizspike				= P_FindParticleType("TR_WIZSPIKE");
	rt_slightblood			= P_FindParticleType("TR_SLIGHTBLOOD");
	rt_knightspike			= P_FindParticleType("TR_KNIGHTSPIKE");
	rt_vorespike			= P_FindParticleType("TR_VORESPIKE");
	//rtdp_neharasmoke		= P_FindParticleType("TR_NEHAHRASMOKE");
	rtdp_nexuizplasma		= P_FindParticleType("TR_NEXUIZPLASMA");
	rtdp_glowtrail			= P_FindParticleType("TR_GLOWTRAIL");
	/*internal to psystem*/   P_FindParticleType("SVC_PARTICLE");

	ptqw_gunshot			= (pt_gunshot!=P_INVALID)?pt_gunshot:P_FindParticleType("TE_QWGUNSHOT");	/*shotgun*/
	ptqw_blood				= (ptdp_blood!=P_INVALID)?ptdp_blood:P_FindParticleType("TE_QWBLOOD");
	ptqw_lightningblood		= P_FindParticleType("TE_LIGHTNINGBLOOD");

#ifdef Q2CLIENT
	if (cls.protocol == CP_QUAKE2)
	{
		for (i = 0; i < sizeof(pt_q2)/sizeof(pt_q2[0]); i++)
		{
			if (!q2efnames[i])
			{
				pt_q2[i] = P_INVALID;
				continue;
			}
			pt_q2[i] = P_FindParticleType(va("q2part.%s", q2efnames[i]));

#ifdef _DEBUG
			if (pt_q2[i] == P_INVALID && pt_q2[0] != P_INVALID)
				Con_Printf("effect q2part.%s was not declared\n", q2efnames[i]);
#endif
		}

		/*ptq2_blood				= P_FindParticleType("q2part.TEQ2_BLOOD");
		rtq2_railtrail			= P_FindParticleType("q2part.TR_RAILTRAIL");
		rtq2_blastertrail		= P_FindParticleType("q2part.TR_BLASTERTRAIL");
		ptq2_blasterparticles	= P_FindParticleType("q2part.TE_BLASTERPARTICLES");
		rtq2_bubbletrail		= P_FindParticleType("q2part.TE_BUBBLETRAIL");
		rtq2_gib				= P_FindParticleType("q2part.TR_GIB");
		rtq2_rocket				= P_FindParticleType("q2part.TR_ROCKET");
		rtq2_grenade			= P_FindParticleType("q2part.TR_GRENADE");
		ptq2_bfgparticles		= P_FindParticleType("q2part.TR_BFGPARTICLES");
		ptq2_flies				= P_FindParticleType("q2part.TR_FLIES");
		ptq2_trap				= P_FindParticleType("q2part.TR_TRAP");
		ptq2_trackershell		= P_FindParticleType("q2part.TR_TRACKERSHELL");*/
	}
	else
	{
		for (i = 0; i < sizeof(pt_q2)/sizeof(pt_q2[0]); i++)
			pt_q2[i] = P_INVALID;
	}
#endif

	rtqw_railtrail			= P_FindParticleType("TE_RAILTRAIL");
	ptfte_bullet			= P_FindParticleType("TE_BULLET");
	ptfte_superbullet		= P_FindParticleType("TE_SUPERBULLET");

	CL_RefreshCustomTEnts();

	for (i = 0; i < countof(beamtypes); i++)
	{
		//we can normally expect the server to have precache_modeled these models, so any lookups should be just a lookup, and thus relatively cheap.
		beamtypes[i].model = NULL;
		beamtypes[i].impactmodel = NULL;
		beamtypes[i].ef_beam = beamtypes[i].beamparticles?P_FindParticleType(beamtypes[i].beamparticles):P_INVALID;
		beamtypes[i].ef_impact = beamtypes[i].beamimpactparticle?P_FindParticleType(beamtypes[i].beamimpactparticle):P_INVALID;
	}

	//FIXME
	for (i = 0; i < cl_explosions_max; i++)
		cl_explosions[i].model = NULL;
}

#ifdef Q2CLIENT
enum {
	q2cl_mod_explode,
	q2cl_mod_smoke,
	q2cl_mod_flash,
	q2cl_mod_parasite_tip,
	q2cl_mod_explo4,
	q2cl_mod_bfg_explo,
	q2cl_mod_powerscreen,
	q2cl_mod_max
};
tentmodels_t q2tentmodels[q2cl_mod_max] = {
	{"models/objects/explode/tris.md2"},
	{"models/objects/smoke/tris.md2"},
	{"models/objects/flash/tris.md2"},
	{"models/monsters/parasite/tip/tris.md2"},
	{"models/objects/r_explode/tris.md2"},
	{"sprites/s_bfg2.sp2"},
	{"models/items/armor/effect/tris.md2"}
};

int CLQ2_RegisterTEntModels (void)
{
//	int i;
//	for (i = 0; i < q2cl_mod_max; i++)
//		if (!CL_CheckOrDownloadFile(q2tentmodels[i].modelname, false))
//			return false;

	return true;
}
#endif

static void CL_ClearExplosion(explosion_t *exp, vec3_t org)
{
	exp->endalpha = 0;
	exp->startalpha = 1;
	exp->rgb[0] = 1.0f;
	exp->rgb[1] = 1.0f;
	exp->rgb[2] = 1.0f;
	exp->scale = 1;
	exp->gravity = 0;
	exp->flags = 0;
	exp->model = NULL;
	exp->firstframe = -1;
	exp->framerate = 10;
	VectorClear(exp->velocity);
	VectorClear(exp->angles);
	VectorClear(exp->avel);
	if (pe)
		P_DelinkTrailstate(&(exp->trailstate));
	exp->traileffect = P_INVALID;
	VectorCopy(org, exp->origin);
	VectorCopy(org, exp->oldorigin);
}

/*
=================
CL_ClearTEnts
=================
*/
void CL_ClearTEnts (void)
{
	int i;
	CL_ClearTEntParticleState();
	CL_ShutdownTEnts();

	cl_beams_max = 0;
	BZ_Free(cl_beams);
	cl_beams = NULL;
	beams_running = 0;

	for (i = 0; i < cl_explosions_max; i++)
		CL_ClearExplosion(cl_explosions+i, vec3_origin);
	cl_explosions_max = 0;
	BZ_Free(cl_explosions);
	cl_explosions = NULL;
	explosions_running = 0;
}

/*
=================
CL_AllocExplosion
=================
*/
explosion_t *CL_AllocExplosion (vec3_t org)
{
	int		i;
	float	time;
	int		index;

	for (i=0; i < explosions_running; i++)
	{
		if (!cl_explosions[i].model)
		{
			CL_ClearExplosion(&cl_explosions[i], org);
			return &cl_explosions[i];
		}
	}

//	if (i == explosions_running && i < cl_maxexplosions.ival)
	{
		if (i == cl_explosions_max)
		{
			cl_explosions_max = (i+1)*2;
			cl_explosions = BZ_Realloc(cl_explosions, sizeof(*cl_explosions)*cl_explosions_max);
			memset(cl_explosions + i, 0, sizeof(*cl_explosions)*(cl_explosions_max-i));
		}

		explosions_running++;
		CL_ClearExplosion(&cl_explosions[i], org);
		return &cl_explosions[i];
	}

// find the oldest explosion
	time = cl.time;
	index = 0;

	for (i=0 ; i<cl_explosions_max ; i++)
		if (cl_explosions[i].start < time)
		{
			time = cl_explosions[i].start;
			index = i;
		}
	CL_ClearExplosion(&cl_explosions[index], org);
	return &cl_explosions[index];
}

/*
=================
CL_ParseBeam
=================
*/
beam_t	*CL_NewBeam (int entity, int tag, tentmodels_t *btype)
{
	beam_t	*b;
	int i;

// override any beam with the same entity (unless they used world)
	if (entity)
	{
		for (i=0, b=cl_beams; i < beams_running; i++, b++)
			if (b->entity == entity && b->tag == tag)
				goto found;
	}

// find a free beam
	for (i=0, b=cl_beams; i < beams_running; i++, b++)
	{
		if (!b->info)
			goto found;
	}


//	if (i >= cl_maxbeams.ival)
//		return NULL;
	if (i == cl_beams_max)
	{
		int nm = (i+1)*2;
		CL_ClearTEntParticleState();

		cl_beams = BZ_Realloc(cl_beams, nm*sizeof(*cl_beams));
		memset(cl_beams + cl_beams_max, 0, sizeof(*cl_beams)*(nm-cl_beams_max));
		cl_beams_max = nm;
	}

	beams_running++;
	b = &cl_beams[i];

found:
	b->info = btype;
	b->bflags = 0;
	VectorClear(b->offset);
	return b;
}
#define	STREAM_ATTACHTOPLAYER	1	//if owned by the viewentity then attach to camera (but don't for other entities).
#define STREAM_JITTER			2	//moves up to 30qu forward/back (40qu per sec)
#define STREAM_ATTACHED			16	//attach it to any entity's origin
#define STREAM_TRANSLUCENT		32
beam_t *CL_AddBeam (enum beamtype_e tent, int ent, vec3_t start, vec3_t end)	//fixme: use TE_ numbers instead of 0 - 5
{
	beam_t	*b;

	model_t *m;
	int btype, etype;
	int i;
	vec3_t impact, normal;
	vec3_t extra;

	//zquake compat requires some parsing weirdness.
	switch(tent)
	{
	case BT_Q1LIGHTNING1:
		if (ent < 0 && ent >= -512)	//a zquake concept. ent between -1 and -maxplayers is to be taken to be a railtrail from a particular player instead of a beam.
		{
			// TODO: add support for those finnicky colored railtrails...
			if (P_ParticleTrail(start, end, rtqw_railtrail, 0.1, -ent, NULL, NULL))
				P_ParticleTrailIndex(start, end, P_INVALID, 0.1, 208, 8, NULL);
			return NULL;
		}
		break;
	case BT_Q1LIGHTNING2:
		if (ent < 0 && ent >= -MAX_CLIENTS)	//based on the railgun concept - this adds a rogue style TE_BEAM effect.
			tent = BT_Q1BEAM;
		break;
	default:
		break;
	}

	btype = beamtypes[tent].ef_beam;
	etype = beamtypes[tent].ef_impact;

	/*don't bother loading the model if we have a particle effect for it instead*/
	if (ruleset_allow_particle_lightning.ival && btype >= 0)
		m = NULL;
	else
	{
		m = beamtypes[tent].model;
		if (!m)
			m = beamtypes[tent].model = Mod_ForName(beamtypes[tent].modelname, MLV_WARN);
	}

	if (m && m->loadstate != MLS_LOADED)
		CL_CheckOrEnqueDownloadFile(m->name, NULL, 0);

	// save end position for truelightning
	if (ent)
	{
		for (i = 0; i < cl.splitclients; i++)
		{
			playerview_t *pv = &cl.playerview[i];
			if (ent == ((pv->cam_state == CAM_EYECAM)?(pv->cam_spec_track+1):(pv->playernum+1)))
			{
				VectorCopy(end, playerbeam_end[i]);
				break;
			}
		}
	}

	if (etype >= 0 && cls.state == ca_active && etype != P_INVALID)
	{
		if (cl_beam_trace.ival)
		{
			VectorSubtract(end, start, normal);
			VectorNormalize(normal);
			VectorMA(end, 4, normal, extra);	//extend the end-point by four
			if (CL_TraceLine(start, extra, impact, normal, NULL)>=1)
				etype = -1;
		}
		else
		{
			VectorCopy(end, impact);
			normal[0] = normal[1] = normal[2] = 0;
		}
	}

	b = CL_NewBeam(ent, -1, &beamtypes[tent]);
	if (!b)
	{
		Con_Printf ("beam list overflow!\n");
		return NULL;
	}

	b->rflags = RF_NOSHADOW;
	b->entity = ent;
	b->info = &beamtypes[tent];
	b->tag = -1;
	b->bflags |= /*STREAM_ATTACHED|*/STREAM_ATTACHTOPLAYER;
	b->endtime = cl.time + 0.2;
	b->alpha = bound(0, cl_beam_alpha.value, 1);
	if(b->alpha < 1)
		b->rflags |= RF_TRANSLUCENT;
	b->skin = 0;
	VectorCopy (start, b->start);
	VectorCopy (end, b->end);

	if (etype >= 0)
	{
		P_RunParticleEffectState (impact, normal, 1, etype, &(b->emitstate));
		if (cl_legacystains.ival) Surf_AddStain(end, -10, -10, -10, 20);
	}
	return b;
}
void CL_ParseBeamOffset (enum beamtype_e tent)
{
	int		ent;
	vec3_t	start, end, offset;
	beam_t *b;

	ent = MSGCL_ReadEntity ();

	start[0] = MSG_ReadCoord ();
	start[1] = MSG_ReadCoord ();
	start[2] = MSG_ReadCoord ();

	end[0] = MSG_ReadCoord ();
	end[1] = MSG_ReadCoord ();
	end[2] = MSG_ReadCoord ();

	MSG_ReadPos(offset);

	b = CL_AddBeam(tent, ent, start, end);
	if (b)
		VectorCopy(offset, b->offset);
}
beam_t *CL_ParseBeam (enum beamtype_e tent)
{
	int		ent;
	vec3_t	start, end;

	ent = MSGCL_ReadEntity ();

	start[0] = MSG_ReadCoord ();
	start[1] = MSG_ReadCoord ();
	start[2] = MSG_ReadCoord ();

	end[0] = MSG_ReadCoord ();
	end[1] = MSG_ReadCoord ();
	end[2] = MSG_ReadCoord ();

	return CL_AddBeam(tent, ent, start, end);
}

//finds the latest non-lerped position
float *CL_FindLatestEntityOrigin(int entnum)
{
	int i;
	packet_entities_t *pe;
	int framenum = cl.validsequence & UPDATE_MASK;
	pe = &cl.inframes[framenum].packet_entities;
	for (i = 0; i < pe->num_entities; i++)
	{
		if (pe->entities[i].number == entnum)
			return pe->entities[i].origin;
	}
	if (entnum > 0 && entnum <= MAX_CLIENTS)
	{
		entnum--;
		if (cl.inframes[framenum].playerstate[entnum].messagenum == cl.parsecount)
			return cl.inframes[framenum].playerstate[entnum].origin;
	}
	return NULL;
}

void CL_ParseStream (int type)
{
	int		ent;
	vec3_t	start, end;
	beam_t	*b, *b2;
	int flags;
	int tag;
	float duration;
	int skin;
	tentmodels_t *info;

	ent = MSGCL_ReadEntity();
	flags = MSG_ReadByte();
	tag = flags&15;
	flags-=tag;
	duration = (float)MSG_ReadByte()*0.05;
	skin = 0;
	if(type == TEH2_STREAM_COLORBEAM)
	{
		skin = MSG_ReadByte();
	}
	start[0] = MSG_ReadCoord();
	start[1] = MSG_ReadCoord();
	start[2] = MSG_ReadCoord();
	end[0] = MSG_ReadCoord();
	end[1] = MSG_ReadCoord();
	end[2] = MSG_ReadCoord();

	switch(type)
	{
	case TEH2_STREAM_LIGHTNING_SMALL:
		info = &beamtypes[BT_H2LIGHTNING_SMALL];
		flags |= STREAM_JITTER;
		break;
	case TEH2_STREAM_LIGHTNING:
		info = &beamtypes[BT_H2LIGHTNING];
		flags |= STREAM_JITTER;
		break;
	case TEH2_STREAM_ICECHUNKS:
		info = &beamtypes[BT_H2ICECHUNKS];
		flags |= STREAM_JITTER;
		if (cl_legacystains.ival) Surf_AddStain(end, -10, -10, 0, 20);
		break;
	case TEH2_STREAM_SUNSTAFF1:
		info = &beamtypes[BT_H2SUNSTAFF1];
		break;
	case TEH2_STREAM_SUNSTAFF2:
		info = &beamtypes[BT_H2SUNSTAFF2];
		if (cl_legacystains.ival) Surf_AddStain(end, -10, -10, -10, 20);
		break;
	case TEH2_STREAM_COLORBEAM:
		info = &beamtypes[BT_H2COLORBEAM];
		break;
	case TEH2_STREAM_GAZE:
		info = &beamtypes[BT_H2GAZE];
		break;
	case TEH2_STREAM_FAMINE:
		info = &beamtypes[BT_H2FAMINE];
		break;
	case TEH2_STREAM_CHAIN:
		info = &beamtypes[BT_H2CHAIN];
		break;
	default:
		Con_Printf("CL_ParseStream: type %i\n", type);
		info = &beamtypes[BT_H2LIGHTNING];
		break;
	}

	b = CL_NewBeam(ent, tag, info);
	if (!b)
	{
		Con_Printf ("beam list overflow!\n");
		return;
	}
		
	b->rflags = RF_NOSHADOW;
	b->entity = ent;
	b->tag = tag;
	b->bflags = flags;
	b->endtime = cl.time + duration;
	b->alpha = 1;
	b->skin = skin;
	VectorCopy (start, b->start);
	VectorCopy (end, b->end);

	if (b->bflags & STREAM_ATTACHED)
	{
		float *entorg = CL_FindLatestEntityOrigin(ent);
		if (!entorg)
			b->bflags &= ~STREAM_ATTACHED;	//not found, attached isn't valid.
		else
		{
			VectorSubtract(b->start, entorg, b->offset);
		}
	}

	//special handling...
	if (info->partner && info->ef_beam == P_INVALID)
	{
		b2 = CL_NewBeam(ent, tag+128, info->partner);
		if (b2)
		{
			P_DelinkTrailstate(&b2->trailstate);
			P_DelinkTrailstate(&b2->emitstate);
			memcpy(b2, b, sizeof(*b2));
			b2->trailstate = b2->emitstate = 0;
			b2->info = info->partner;
			b2->tag = tag+128;
			b2->trailstate = trailkey_null;
			b2->emitstate = trailkey_null;
			b2->alpha = 0.5;
			b2->rflags = RF_TRANSLUCENT|RF_NOSHADOW;
		}
	}
}

/*
=================
CL_ParseTEnt
=================
*/

#ifdef NQPROT
void CL_ParseTEnt (qboolean nqprot)
#else
void CL_ParseTEnt (void)
#endif
{
#ifndef NQPROT
#define nqprot false	//it's easier
#endif
	int		type;
	vec3_t	pos, pos2;
	dlight_t	*dl;
	int		rnd;
//	explosion_t	*ex;
	int		cnt, colour;

#ifdef CSQC_DAT
	//I know I'm going to regret this.
	if (CSQC_ParseTempEntity())
		return;
#endif

	type = MSG_ReadByte ();

	if (nqprot)
	{
		//easiest way to handle these
		//should probably also do qwgunshot ones with nq protocols or something
		switch(type)
		{
		case TENQ_EXPLOSION2:
			type = TEQW_EXPLOSION2;
			break;
		case TENQ_BEAM:
			type = TEQW_BEAM;
			break;
		case TENQ_QWEXPLOSION:
			type = TEQW_QWEXPLOSION;
			break;
		case TENQ_NQEXPLOSION:
			type = TEQW_NQEXPLOSION;
			break;
		case TENQ_NQGUNSHOT:
			type = TEQW_NQGUNSHOT;
			break;
		case TENQ_QWGUNSHOT:
			type = TEQW_QWGUNSHOT;
			break;
		case TENQ_RAILTRAIL:
			type = TEQW_RAILTRAIL;
			break;
		case TENQ_NEHLIGHTNING4:
			type = TEQW_NEHLIGHTNING4;
			break;
//		case TENQ_NEHSMOKE:
//			type = TEQW_NEHSMOKE;
//			break;

		default:
			break;
		}
	}
	//else QW values

	//right, nq vs qw doesn't matter now, supposedly.
	
	if (cl_shownet.ival >= 2)
	{
		static char *te_names[] = {
			/* 0*/"spike", "superspike", "qwgunshot", "qwexplosion",
			/* 4*/"tarexplosion", "lightning1", "lightning2", "wizspike",
			/* 8*/"knightspike", "lightning3", "lavasplash", "teleport",
			/*12*/"blood", "lightningblood", "bullet", "superbullet",	//bullets deprecated
			/*16*/"neh_explosion3", "railtrail", "beam", "explosion2",
			/*20*/"nqexplosion", "nqgunshot", NULL, NULL,
			/*24*/"h2lightsml", "h2chain", "h2sunstf1", "h2sunstf2",
			/*28*/"h2light", "h2cb", "h2ic", "h2gaze",
			/*32*/"h2famine", "h2partexp",NULL,NULL,
			/*36*/NULL,NULL,NULL,NULL,
			/*40*/NULL,NULL,NULL,NULL,
			/*44*/NULL,NULL,NULL,NULL,
			/*48*/NULL,NULL,"dpblood","dpspark"
			/*52*/"dpbloodshower","dpexplosionrgb","dpparticlecube","dpparticlerain",
			/*56*/"dpparticlesnow","dpgunshotquad","dpspikequad","dpsuperspikequad",
			/*60*/NULL,NULL,NULL,NULL,
			/*64*/NULL,NULL,NULL,NULL,
			/*68*/NULL,NULL,"dpexplosionquad",NULL,
			/*72*/"dpsmallflash","dpcustomflash","dpflamejet","dpplasmaburn",
			/*76*/"dpteig3","dpsmoke","dpteibigexplosion","dpteiplasmahit",
			/*80*/
		};
		if (type < countof(te_names) && te_names[type])
			Con_Printf("  te_%s\n", te_names[type]);
		else
			Con_Printf("  te_unknown_%i\n", type);
	}

	switch (type)
	{
	case TE_WIZSPIKE:			// spike hitting wall
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		if (cl_legacystains.ival) Surf_AddStain(pos, -10, 0, -10, 20);

		if (P_RunParticleEffectType(pos, NULL, 1, pt_wizspike))
			P_RunParticleEffect (pos, vec3_origin, 20, 30);

		S_StartSound (0, 0, cl_sfx_wizhit, pos, NULL, 1, 1, 0, 0, 0);
		break;

	case TE_KNIGHTSPIKE:			// spike hitting wall
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		if (cl_legacystains.ival) Surf_AddStain(pos, -10, -10, -10, 20);

		if (P_RunParticleEffectType(pos, NULL, 1, pt_knightspike))
			P_RunParticleEffect (pos, vec3_origin, 226, 20);

		S_StartSound (0, 0, cl_sfx_knighthit, pos, NULL, 1, 1, 0, 0, 0);
		break;

	case TEDP_SPIKEQUAD:
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		if (cl_legacystains.ival) Surf_AddStain(pos, -10, -10, -10, 20);

		if (P_RunParticleEffectType(pos, NULL, 1, ptdp_spikequad))
			if (P_RunParticleEffectType(pos, NULL, 1, pt_spike))
				if (P_RunParticleEffectType(pos, NULL, 10, pt_gunshot))
					P_RunParticleEffect (pos, vec3_origin, 0, 10);

		if ( rand() % 5 )
			S_StartSound (0, 0, cl_sfx_tink1, pos, NULL, 1, 1, 0, 0, 0);
		else
		{
			rnd = rand() & 3;
			if (rnd == 1)
				S_StartSound (0, 0, cl_sfx_ric1, pos, NULL, 1, 1, 0, 0, 0);
			else if (rnd == 2)
				S_StartSound (0, 0, cl_sfx_ric2, pos, NULL, 1, 1, 0, 0, 0);
			else
				S_StartSound (0, 0, cl_sfx_ric3, pos, NULL, 1, 1, 0, 0, 0);
		}
		break;
	case TE_SPIKE:			// spike hitting wall
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		if (cl_legacystains.ival) Surf_AddStain(pos, -10, -10, -10, 20);

		if (P_RunParticleEffectType(pos, NULL, 1, pt_spike))
			if (P_RunParticleEffectType(pos, NULL, 10, pt_gunshot))
				P_RunParticleEffect (pos, vec3_origin, 0, 10);

		if ( rand() % 5 )
			S_StartSound (0, 0, cl_sfx_tink1, pos, NULL, 1, 1, 0, 0, 0);
		else
		{
			rnd = rand() & 3;
			if (rnd == 1)
				S_StartSound (0, 0, cl_sfx_ric1, pos, NULL, 1, 1, 0, 0, 0);
			else if (rnd == 2)
				S_StartSound (0, 0, cl_sfx_ric2, pos, NULL, 1, 1, 0, 0, 0);
			else
				S_StartSound (0, 0, cl_sfx_ric3, pos, NULL, 1, 1, 0, 0, 0);
		}
		break;
	case TEDP_SUPERSPIKEQUAD:			// super spike hitting wall
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		if (cl_legacystains.ival) Surf_AddStain(pos, -10, -10, -10, 20);

		if (P_RunParticleEffectType(pos, NULL, 1, ptdp_superspikequad))
			if (P_RunParticleEffectType(pos, NULL, 1, pt_superspike))
				if (P_RunParticleEffectType(pos, NULL, 2, pt_spike))
					if (P_RunParticleEffectType(pos, NULL, 20, pt_gunshot))
						P_RunParticleEffect (pos, vec3_origin, 0, 20);

		if ( rand() % 5 )
			S_StartSound (0, 0, cl_sfx_tink1, pos, NULL, 1, 1, 0, 0, 0);
		else
		{
			rnd = rand() & 3;
			if (rnd == 1)
				S_StartSound (0, 0, cl_sfx_ric1, pos, NULL, 1, 1, 0, 0, 0);
			else if (rnd == 2)
				S_StartSound (0, 0, cl_sfx_ric2, pos, NULL, 1, 1, 0, 0, 0);
			else
				S_StartSound (0, 0, cl_sfx_ric3, pos, NULL, 1, 1, 0, 0, 0);
		}
		break;
	case TE_SUPERSPIKE:			// super spike hitting wall
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		if (cl_legacystains.ival) Surf_AddStain(pos, -10, -10, -10, 20);

		if (P_RunParticleEffectType(pos, NULL, 1, pt_superspike))
			if (P_RunParticleEffectType(pos, NULL, 2, pt_spike))
				if (P_RunParticleEffectType(pos, NULL, 20, pt_gunshot))
					P_RunParticleEffect (pos, vec3_origin, 0, 20);

		if ( rand() % 5 )
			S_StartSound (0, 0, cl_sfx_tink1, pos, NULL, 1, 1, 0, 0, 0);
		else
		{
			rnd = rand() & 3;
			if (rnd == 1)
				S_StartSound (0, 0, cl_sfx_ric1, pos, NULL, 1, 1, 0, 0, 0);
			else if (rnd == 2)
				S_StartSound (0, 0, cl_sfx_ric2, pos, NULL, 1, 1, 0, 0, 0);
			else
				S_StartSound (0, 0, cl_sfx_ric3, pos, NULL, 1, 1, 0, 0, 0);
		}
		break;

#ifdef PEXT_TE_BULLET
	case TE_BULLET:
		if (!(cls.fteprotocolextensions & PEXT_TE_BULLET))
			Host_EndGame("Thought PEXT_TE_BULLET was disabled");
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		if (cl_legacystains.ival) Surf_AddStain(pos, -10, -10, -10, 20);

		if (P_RunParticleEffectType(pos, NULL, 1, ptfte_bullet))
			if (P_RunParticleEffectType(pos, NULL, 10, pt_gunshot))
				P_RunParticleEffect (pos, vec3_origin, 0, 10);

		if ( rand() % 5 )
			S_StartSound (0, 0, cl_sfx_tink1, pos, NULL, 1, 1, 0, 0, 0);
		else
		{
			rnd = rand() & 3;
			if (rnd == 1)
				S_StartSound (0, 0, cl_sfx_ric1, pos, NULL, 1, 1, 0, 0, 0);
			else if (rnd == 2)
				S_StartSound (0, 0, cl_sfx_ric2, pos, NULL, 1, 1, 0, 0, 0);
			else
				S_StartSound (0, 0, cl_sfx_ric3, pos, NULL, 1, 1, 0, 0, 0);
		}
		break;
	case TEQW_SUPERBULLET:
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		if (cl_legacystains.ival) Surf_AddStain(pos, -10, -10, -10, 20);

		if (P_RunParticleEffectType(pos, NULL, 1, ptfte_superbullet))
			if (P_RunParticleEffectType(pos, NULL, 2, ptfte_bullet))
				if (P_RunParticleEffectType(pos, NULL, 20, pt_gunshot))
					P_RunParticleEffect (pos, vec3_origin, 0, 20);

		if ( rand() % 5 )
			S_StartSound (0, 0, cl_sfx_tink1, pos, NULL, 1, 1, 0, 0, 0);
		else
		{
			rnd = rand() & 3;
			if (rnd == 1)
				S_StartSound (0, 0, cl_sfx_ric1, pos, NULL, 1, 1, 0, 0, 0);
			else if (rnd == 2)
				S_StartSound (0, 0, cl_sfx_ric2, pos, NULL, 1, 1, 0, 0, 0);
			else
				S_StartSound (0, 0, cl_sfx_ric3, pos, NULL, 1, 1, 0, 0, 0);
		}
		break;
#endif

	case TEDP_EXPLOSIONQUAD:			// rocket explosion
	// particles
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();
		if (P_RunParticleEffectType(pos, NULL, 1, ptdp_explosionquad))
			if (P_RunParticleEffectType(pos, NULL, 1, pt_explosion))
				P_RunParticleEffect(pos, NULL, 107, 1024); // should be 97-111

		if (cl_legacystains.ival) Surf_AddStain(pos, -1, -1, -1, 100);

	// light
		if (r_explosionlight.value)
		{
			dl = CL_AllocDlight (0);
			VectorCopy (pos, dl->origin);
			dl->radius = 150 + r_explosionlight.value*200;
			dl->die = cl.time + 1;
			dl->decay = 300;

			dl->color[0] = 4.0;
			dl->color[1] = 2.0;
			dl->color[2] = 0.5;
			dl->channelfade[0] = 0.196;
			dl->channelfade[1] = 0.23;
			dl->channelfade[2] = 0.12;
		}


	// sound
		S_StartSound (0, 0, cl_sfx_r_exp3, pos, NULL, 1, 1, 0, 0, 0);

	// sprite
		if (cl_expsprite.ival) // temp hopefully
		{
			explosion_t *ex = CL_AllocExplosion (pos);
			ex->start = cl.time;
			ex->model = Mod_ForName ("progs/s_explod.spr", MLV_WARN);
			ex->endalpha = ex->startalpha;	//don't fade out
		}
		break;
	case TEQW_NQEXPLOSION:	//nq-style, no sprite
	case TEQW_QWEXPLOSION:				//qw-style, with (optional) sprite
	// particles
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();
		if (P_RunParticleEffectType(pos, NULL, 1, pt_explosion))
			P_RunParticleEffect(pos, NULL, 107, 1024); // should be 97-111

		if (cl_legacystains.ival) Surf_AddStain(pos, -1, -1, -1, 100);

	// light
		if (r_explosionlight.value)
		{
			dl = CL_AllocDlight (0);
			VectorCopy (pos, dl->origin);
			dl->radius = 150 + r_explosionlight.value*200;
			dl->die = cl.time + 0.75;
			dl->decay = dl->radius*2;

			VectorCopy(r_explosionlight_colour.vec4, dl->color);
			VectorCopy(r_explosionlight_fade.vec4, dl->channelfade);
		}


	// sound
		S_StartSound (0, 0, cl_sfx_r_exp3, pos, NULL, 1, 1, 0, 0, 0);

	// sprite
		if (type == TEQW_QWEXPLOSION && cl_expsprite.ival) // temp hopefully
		{
			explosion_t *ex = CL_AllocExplosion (pos);
			ex->start = cl.time;
			ex->model = Mod_ForName ("progs/s_explod.spr", MLV_WARN);
			ex->endalpha = ex->startalpha;	//don't fade out
		}
		break;

	case TEQW_EXPLOSION2:
		{
			int colorStart;
			int colorLength;
			int ef;
			pos[0] = MSG_ReadCoord ();
			pos[1] = MSG_ReadCoord ();
			pos[2] = MSG_ReadCoord ();
			colorStart = MSG_ReadByte ();
			colorLength = MSG_ReadByte ();

			ef = P_FindParticleType(va("TE_EXPLOSION2_%i_%i", colorStart, colorLength));
			if (ef == P_INVALID)
				ef = pt_explosion;
			P_RunParticleEffectType(pos, NULL, 1, ef);
			if (r_explosionlight.value)
			{
				dl = CL_AllocDlight (0);
				VectorCopy (pos, dl->origin);
				dl->radius = 350;
				dl->die = cl.time + 0.5;
				dl->decay = 300;
			}
			S_StartSound (0, 0, cl_sfx_r_exp3, pos, NULL, 1, 1, 0, 0, 0);
		}
		break;

	case TE_EXPLOSION3_NEH:
	case TEDP_EXPLOSIONRGB:
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();
		if (P_RunParticleEffectType(pos, NULL, 1, pt_explosion))
			P_RunParticleEffect(pos, NULL, 107, 1024); // should be 97-111

		if (cl_legacystains.ival) Surf_AddStain(pos, -1, -1, -1, 100);

		if (type == TEDP_EXPLOSIONRGB)
		{
			pos2[0] = MSG_ReadByte()/255.0;
			pos2[1] = MSG_ReadByte()/255.0;
			pos2[2] = MSG_ReadByte()/255.0;
		}
		else
		{	//TE_EXPLOSION3_NEH
			pos2[0] = MSG_ReadCoord();
			pos2[1] = MSG_ReadCoord();
			pos2[2] = MSG_ReadCoord();
		}

	// light
		if (r_explosionlight.value)
		{
			dl = CL_AllocDlight (0);
			VectorCopy (pos, dl->origin);
			dl->radius = 150 + r_explosionlight.value*200;
			dl->die = cl.time + 0.5;
			dl->decay = 300;

			dl->color[0] = 0.4f*pos2[0];
			dl->color[1] = 0.4f*pos2[1];
			dl->color[2] = 0.4f*pos2[2];
			dl->channelfade[0] = 0;
			dl->channelfade[1] = 0;
			dl->channelfade[2] = 0;
		}

		S_StartSound (0, 0, cl_sfx_r_exp3, pos, NULL, 1, 1, 0, 0, 0);
		break;

	case TEDP_TEI_BIGEXPLOSION:
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();
		if (P_RunParticleEffectType(pos, NULL, 1, ptdp_tei_bigexplosion))
			if (P_RunParticleEffectType(pos, NULL, 1, pt_explosion))
				P_RunParticleEffect(pos, NULL, 107, 1024); // should be 97-111

		if (cl_legacystains.ival) Surf_AddStain(pos, -1, -1, -1, 100);

	// light
		if (r_explosionlight.value)
		{
			dl = CL_AllocDlight (0);
			VectorCopy (pos, dl->origin);
			// no point in doing this the fuh/ez way
			dl->radius = 500*r_explosionlight.value;
			dl->die = cl.time + 1;
			dl->decay = 500;

			dl->color[0] = 2.0f;
			dl->color[1] = 1.5f;
			dl->color[2] = 0.75f;
			dl->channelfade[0] = 0;
			dl->channelfade[1] = 0;
			dl->channelfade[2] = 0;
		}

		S_StartSound (0, 0, cl_sfx_r_exp3, pos, NULL, 1, 1, 0, 0, 0);
		break;

	case TE_TAREXPLOSION:			// tarbaby explosion
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();
		P_RunParticleEffectType(pos, NULL, 1, pt_tarexplosion);

		S_StartSound (0, 0, cl_sfx_r_exp3, pos, NULL, 1, 1, 0, 0, 0);
		break;

	case TE_LIGHTNING1:				// lightning bolts
		CL_ParseBeam (BT_Q1LIGHTNING1);
		break;
	case TE_LIGHTNING2:				// lightning bolts
		CL_ParseBeam (BT_Q1LIGHTNING2);
		break;
	case TE_LIGHTNING3:				// lightning bolts
		CL_ParseBeam (BT_Q1LIGHTNING3);
		break;
	case TEQW_NEHLIGHTNING4:
		Con_DPrintf("TEQW_NEHLIGHTNING4 not implemented\n");
		MSG_ReadString();
		CL_ParseBeam (BT_Q1LIGHTNING2);
		break;

	case TE_LAVASPLASH:
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();
		P_RunParticleEffectType(pos, NULL, 1, pt_lavasplash);
		break;

	case TE_TELEPORT:
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();
		P_RunParticleEffectType(pos, NULL, 1, pt_teleportsplash);
		break;

	case TEDP_GUNSHOTQUAD:			// bullet hitting wall
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		if (cl_legacystains.ival) Surf_AddStain(pos, -10, -10, -10, 20);

		if (P_RunParticleEffectType(pos, NULL, 1, ptdp_gunshotquad))
			if (P_RunParticleEffectType(pos, NULL, 1, pt_gunshot))
				P_RunParticleEffect (pos, vec3_origin, 0, 20);

		break;

	case TEQW_QWGUNSHOT:			// bullet hitting wall
	case TEQW_NQGUNSHOT:
		if (type == TEQW_NQGUNSHOT)
			cnt = 1;
		else
			cnt = MSG_ReadByte ();
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		if (cl_legacystains.ival) Surf_AddStain(pos, -10, -10, -10, 20);

		if (P_RunParticleEffectType(pos, NULL, cnt, pt_gunshot))
			P_RunParticleEffect (pos, vec3_origin, 0, 20*cnt);

		break;

	case TEQW_QWBLOOD:				// bullets hitting body
		cnt = MSG_ReadByte ();
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		if (cl_legacystains.ival) Surf_AddStain(pos, 0, -10, -10, 40);

		if (P_RunParticleEffectType(pos, NULL, cnt, ptqw_blood))
			if (P_RunParticleEffectType(pos, NULL, cnt, ptdp_blood))
				P_RunParticleEffect (pos, vec3_origin, 73, 20*cnt);

		break;

	case TEQW_LIGHTNINGBLOOD:		// lightning hitting body
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		if (cl_legacystains.ival) Surf_AddStain(pos, 1, -10, -10, 20);

		if (P_RunParticleEffectType(pos, NULL, 1, ptqw_lightningblood))
			P_RunParticleEffect (pos, vec3_origin, 225, 50);

		break;

	case TEQW_BEAM:
		CL_ParseBeam (BT_Q1BEAM);
		break;

	case TEQW_RAILTRAIL:
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		pos2[0] = MSG_ReadCoord ();
		pos2[1] = MSG_ReadCoord ();
		pos2[2] = MSG_ReadCoord ();

		if (P_ParticleTrail(pos, pos2, rtqw_railtrail, 1, 0, NULL, NULL))
			P_ParticleTrailIndex(pos, pos2, P_INVALID, 1, 208, 8, NULL);
		break;

	case TEH2_STREAM_LIGHTNING_SMALL:
	case TEH2_STREAM_CHAIN:
	case TEH2_STREAM_SUNSTAFF1:
	case TEH2_STREAM_SUNSTAFF2:
	case TEH2_STREAM_LIGHTNING:
	case TEH2_STREAM_COLORBEAM:
	case TEH2_STREAM_ICECHUNKS:
	case TEH2_STREAM_GAZE:
	case TEH2_STREAM_FAMINE:
		CL_ParseStream (type);
		break;

	case TEDP_BLOOD:
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		pos2[0] = MSG_ReadChar ();
		pos2[1] = MSG_ReadChar ();
		pos2[2] = MSG_ReadChar ();

		cnt = MSG_ReadByte ();

		P_RunParticleEffectType(pos, pos2, cnt, ptdp_blood);
		break;

	case TEDP_SPARK:
		pos[0] = MSG_ReadCoord ();	//org
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		pos2[0] = MSG_ReadChar ();	//vel
		pos2[1] = MSG_ReadChar ();
		pos2[2] = MSG_ReadChar ();

		cnt = MSG_ReadByte ();
		{
			P_RunParticleEffectType(pos, pos2, cnt, ptdp_spark);
		}
		break;

	case TEDP_BLOODSHOWER:
		{
			vec3_t vel = {0,0,0};
			pos[0] = MSG_ReadCoord ();
			pos[1] = MSG_ReadCoord ();
			pos[2] = MSG_ReadCoord ();

			pos2[0] = MSG_ReadCoord ();
			pos2[1] = MSG_ReadCoord ();
			pos2[2] = MSG_ReadCoord ();

			vel[2] = -MSG_ReadCoord ();

			cnt = MSG_ReadShort ();

			P_RunParticleCube(P_FindParticleType("te_bloodshower"), pos, pos2, vel, vel, cnt, 0, false, 0);
		}
		break;

	case TEDP_SMALLFLASH:
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		// light
		dl = CL_AllocDlight (0);
		VectorCopy (pos, dl->origin);
		dl->radius = 200;
		dl->decay = 1000;
		dl->die = cl.time + 0.2;
		dl->color[0] = 2.0;
		dl->color[1] = 2.0;
		dl->color[2] = 2.0;
		break;

	case TEDP_CUSTOMFLASH:
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

			// light
		dl = CL_AllocDlight (0);
		VectorCopy (pos, dl->origin);
		dl->radius = MSG_ReadByte()*8;
		pos2[0] = (MSG_ReadByte() + 1) * (1.0 / 256.0);
		dl->die = cl.time + pos2[0];
		dl->decay = dl->radius / pos2[0];

		dl->color[0] = MSG_ReadByte()*(1.0f/127.0f);
		dl->color[1] = MSG_ReadByte()*(1.0f/127.5f);
		dl->color[2] = MSG_ReadByte()*(1.0f/127.0f);

		break;

	case TEDP_FLAMEJET:
		// origin
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		// velocity
		pos2[0] = MSG_ReadCoord ();
		pos2[1] = MSG_ReadCoord ();
		pos2[2] = MSG_ReadCoord ();

		// count
		cnt = MSG_ReadByte ();

		if (P_RunParticleEffectType(pos, pos2, cnt, ptdp_flamejet))
			P_RunParticleEffect (pos, pos2, 232, cnt);
		break;

	case TEDP_PLASMABURN:
		// origin
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		// stain (Hopefully this is close to how DP does it)
		if (cl_legacystains.ival) Surf_AddStain(pos, -10, -10, -10, 30);

		if (P_RunParticleEffectType(pos, NULL, 1, P_FindParticleType("te_plasmaburn")))
			P_RunParticleEffect(pos, vec3_origin, 15, 50);
		break;

	case TEDP_TEI_G3:	//nexuiz's nex beam
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		pos2[0] = MSG_ReadCoord ();
		pos2[1] = MSG_ReadCoord ();
		pos2[2] = MSG_ReadCoord ();

		//sigh...
		MSG_ReadCoord ();
		MSG_ReadCoord ();
		MSG_ReadCoord ();

		if (P_ParticleTrail(pos, pos2, P_FindParticleType("te_nexbeam"), 1, 0, NULL, NULL))
			P_ParticleTrailIndex(pos, pos2, P_INVALID, 1, 15, 0, NULL);
		break;

	case TEDP_SMOKE:
		//org
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		//dir
		pos2[0] = MSG_ReadCoord ();
		pos2[1] = MSG_ReadCoord ();
		pos2[2] = MSG_ReadCoord ();

		//count
		cnt = MSG_ReadByte ();
		{
			P_RunParticleEffectType(pos, pos2, cnt, ptdp_tei_smoke);
		}
		break;

	case TEDP_TEI_PLASMAHIT:
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		//dir
		pos2[0] = MSG_ReadCoord ();
		pos2[1] = MSG_ReadCoord ();
		pos2[2] = MSG_ReadCoord ();
		cnt = MSG_ReadByte ();

		{
			P_RunParticleEffectType(pos, pos2, cnt, ptdp_tei_plasmahit);
		}
		break;

	case TEDP_PARTICLECUBE:
		{
			vec3_t dir;
			int jitter;
			int gravity;

			//min
			pos[0] = MSG_ReadCoord();
			pos[1] = MSG_ReadCoord();
			pos[2] = MSG_ReadCoord();

			//max
			pos2[0] = MSG_ReadCoord();
			pos2[1] = MSG_ReadCoord();
			pos2[2] = MSG_ReadCoord();

			//dir
			dir[0] = MSG_ReadCoord();
			dir[1] = MSG_ReadCoord();
			dir[2] = MSG_ReadCoord();

			cnt = MSG_ReadShort();	//count
			colour = MSG_ReadByte ();	//colour
			gravity = MSG_ReadByte ();	//gravity flag
			jitter = MSG_ReadCoord();	//jitter

			P_RunParticleCube(P_INVALID, pos, pos2, dir, dir, cnt, colour, gravity, jitter);
		}
		break;
	case TEDP_PARTICLERAIN:
		{
			vec3_t dir;

			//min
			pos[0] = MSG_ReadCoord();
			pos[1] = MSG_ReadCoord();
			pos[2] = MSG_ReadCoord();

			//max
			pos2[0] = MSG_ReadCoord();
			pos2[1] = MSG_ReadCoord();
			pos2[2] = MSG_ReadCoord();

			//dir
			dir[0] = MSG_ReadCoord();
			dir[1] = MSG_ReadCoord();
			dir[2] = MSG_ReadCoord();

			cnt = (unsigned short)MSG_ReadShort();	//count
			colour = MSG_ReadByte ();	//colour

			P_RunParticleWeather(pos, pos2, dir, cnt, colour, "rain");
		}
		break;
	case TEDP_PARTICLESNOW:
		{
			vec3_t dir;

			//min
			pos[0] = MSG_ReadCoord();
			pos[1] = MSG_ReadCoord();
			pos[2] = MSG_ReadCoord();

			//max
			pos2[0] = MSG_ReadCoord();
			pos2[1] = MSG_ReadCoord();
			pos2[2] = MSG_ReadCoord();

			//dir
			dir[0] = MSG_ReadCoord();
			dir[1] = MSG_ReadCoord();
			dir[2] = MSG_ReadCoord();

			cnt = (unsigned short)MSG_ReadShort();	//count
			colour = MSG_ReadByte ();	//colour

			P_RunParticleWeather(pos, pos2, dir, cnt, colour, "snow");
		}
		break;

//	case TEQW_NEHRAILTRAIL:
//	case TEQW_NEHEXPLOSION3:
//	case TEQW_NEHLIGHTNING4:
//	case TEQW_NEHSMOKE:

	default:
		Host_EndGame ("CL_ParseTEnt: bad type - %i", type);
	}
}

void CL_ParseTEnt_Sized (void)
{
	unsigned short sz = MSG_ReadShort();
	int start = MSG_GetReadCount();

	for(;;)
	{
#ifdef NQPROT
		if (sz&0x8000)
		{
			sz&=~0x8000;
			CL_ParseTEnt(true);
		}
		else
			CL_ParseTEnt(false);
#else
		CL_ParseTEnt();
#endif

		if (MSG_GetReadCount() < start + sz)
		{	//try to be more compatible with xonotic.
			int next = MSG_ReadByte();
			if (next == svc_temp_entity)
				continue;

			Con_Printf("Sized temp_entity data too large (next byte %i, %i bytes unread)\n", next, (start+sz)-MSG_GetReadCount()-1);
			MSG_ReadSkip(start+sz-MSG_GetReadCount());
			return;
		}
		break;
	}


	if (MSG_GetReadCount() != start + sz)
	{
		Con_Printf("Tempentity size did not match parsed size misread a gamecode packet (%i bytes too much)\n", MSG_GetReadCount() - (start+sz));
		MSG_ReadSkip(start+sz-MSG_GetReadCount());
	}
}

typedef struct {
	char name[64];
	int netstyle;
	int particleeffecttype;
	char stain[3];
	qbyte radius;
	vec3_t dlightrgb;
	float dlightradius;
	float dlighttime;
	vec3_t dlightcfade;
} clcustomtents_t;

typedef struct custtentinst_s
{
	struct custtentinst_s *next;
	clcustomtents_t *type;
	int id;
	vec3_t pos;
	vec3_t pos2;
	vec3_t dir;
	int count;
} custtentinst_t;
custtentinst_t *activepcusttents;

void CL_SpawnCustomTEnt(custtentinst_t *info)
{
	clcustomtents_t *t = info->type;
	qboolean failed;
	if (t->netstyle & CTE_ISBEAM)
	{
		if (t->netstyle & (CTE_CUSTOMVELOCITY|CTE_CUSTOMDIRECTION))
		{
			vec3_t org;
			int i, j;
			//FIXME: pvs cull
			if (t->particleeffecttype == -1)
				failed = true;
			else
			{
				failed = false;
				for (i=0 ; i<info->count ; i++)
				{
					for (j=0 ; j<3 ; j++)
					{
						org[j] = info->pos[j] + (info->pos2[j] - info->pos[j])*frandom();
					}

					failed |= P_RunParticleEffectType(org, info->dir, 1, t->particleeffecttype);
				}
			}
		}
		else
			failed = P_ParticleTrail(info->pos, info->pos2, t->particleeffecttype, 1, 0, NULL, NULL);
	}
	else
	{
		if (t->netstyle & (CTE_CUSTOMVELOCITY|CTE_CUSTOMDIRECTION))
			failed = P_RunParticleEffectType(info->pos, info->dir, info->count, t->particleeffecttype);
		else
			failed = P_RunParticleEffectType(info->pos, NULL, info->count, t->particleeffecttype);
	}

	if (failed)
		Con_DPrintf("Failed to create effect %s\n", t->name);

	if (t->netstyle & CTE_STAINS)
	{	//added at pos2 - end of trail
		Surf_AddStain(info->pos2, t->stain[0], t->stain[1], t->stain[2], 40);
	}
	if (t->netstyle & CTE_GLOWS)
	{	//added at pos1 firer's end.
		dlight_t	*dl;
		dl = CL_AllocDlight (0);
		VectorCopy (info->pos, dl->origin);
		dl->radius = t->dlightradius*4;
		dl->die = cl.time + t->dlighttime;
		dl->decay = t->radius/t->dlighttime;

		dl->color[0] = t->dlightrgb[0];
		dl->color[1] = t->dlightrgb[1];
		dl->color[2] = t->dlightrgb[2];

		if (t->netstyle & CTE_CHANNELFADE)
		{
			dl->channelfade[0] = t->dlightcfade[0];
			dl->channelfade[1] = t->dlightcfade[1];
			dl->channelfade[2] = t->dlightcfade[2];
		}

		/*
		if (dl->color[0] < 0)
			dl->channelfade[0] = 0;
		else
			dl->channelfade[0] = dl->color[0]/t->dlighttime;

		if (dl->color[1] < 0)
			dl->channelfade[1] = 0;
		else
			dl->channelfade[1] = dl->color[0]/t->dlighttime;

		if (dl->color[2] < 0)
			dl->channelfade[2] = 0;
		else
			dl->channelfade[2] = dl->color[0]/t->dlighttime;
		*/
	}
}

void CL_RunPCustomTEnts(void)
{
	custtentinst_t *ef;
	static float lasttime;
	float since = cl.time - lasttime;
	if (since < 0)
		lasttime = cl.time;
	else if (since < 1/60.0)
		return;
	lasttime = cl.time;
	for (ef = activepcusttents; ef; ef = ef->next)
	{
		CL_SpawnCustomTEnt(ef);
	}
}

clcustomtents_t customtenttype[255];	//network based.

qboolean CL_WriteCustomTEnt(sizebuf_t *buf, int id)
{
	clcustomtents_t *t;
	if ((unsigned)id >= 255u)
		return false;
	t = &customtenttype[id];
	if (*t->name)
	{
		MSG_WriteByte(buf, svcfte_customtempent);
		MSG_WriteByte(buf, 255);
		MSG_WriteByte(buf, id);
		MSG_WriteByte(buf, t->netstyle);
		MSG_WriteString(buf, t->name);

		if (t->netstyle & CTE_STAINS)
		{
			MSG_WriteChar(buf, t->stain[0]);
			MSG_WriteChar(buf, t->stain[1]);
			MSG_WriteChar(buf, t->stain[2]);
			MSG_WriteByte(buf, t->radius);
		}
		if (t->netstyle & CTE_GLOWS)
		{
			MSG_WriteByte(buf, t->dlightrgb[0]*255);
			MSG_WriteByte(buf, t->dlightrgb[1]*255);
			MSG_WriteByte(buf, t->dlightrgb[2]*255);
			MSG_WriteByte(buf, t->dlightradius);
			MSG_WriteByte(buf, t->dlighttime*16);
			if (t->netstyle & CTE_CHANNELFADE)
			{
				MSG_WriteByte(buf, t->dlightcfade[0]*64);
				MSG_WriteByte(buf, t->dlightcfade[1]*64);
				MSG_WriteByte(buf, t->dlightcfade[2]*64);
			}
		}
	}
	return true;
}

void CL_ParseCustomTEnt(void)
{
	char *str;
	clcustomtents_t *t;
	int type = MSG_ReadByte();
	custtentinst_t info;

	if (type == 255)	//255 is register
	{
		type = MSG_ReadByte();
		if (type == 255)
			Host_EndGame("Custom temp type 255 isn't valid\n");
		t = &customtenttype[type];

		t->netstyle = MSG_ReadByte();
		str = MSG_ReadString();
		Q_strncpyz(t->name, str, sizeof(t->name));
		t->particleeffecttype = P_FindParticleType(str);
		if (cl_shownet.ival >= 3)
			Con_Printf("\tdefine \"%s\" (%x)\n", t->name, t->netstyle);

		if (t->netstyle & CTE_STAINS)
		{
			t->stain[0] = MSG_ReadChar();
			t->stain[1] = MSG_ReadChar();
			t->stain[2] = MSG_ReadChar();
			t->radius = MSG_ReadByte();
		}
		else
			t->radius = 0;
		if (t->netstyle & CTE_GLOWS)
		{
			t->dlightrgb[0] = MSG_ReadByte()/255.0f;
			t->dlightrgb[1] = MSG_ReadByte()/255.0f;
			t->dlightrgb[2] = MSG_ReadByte()/255.0f;
			t->dlightradius = MSG_ReadByte();
			t->dlighttime = MSG_ReadByte()/16.0f;
			if (t->netstyle & CTE_CHANNELFADE)
			{
				t->dlightcfade[0] = MSG_ReadByte()/64.0f;
				t->dlightcfade[1] = MSG_ReadByte()/64.0f;
				t->dlightcfade[2] = MSG_ReadByte()/64.0f;
			}
		}
		else
			t->dlighttime = 0;
		return;
	}

	t = &customtenttype[type];

	if (cl_shownet.ival >= 3)
		Con_Printf("\tspawn \"%s\" (%x)\n", t->name, t->netstyle);

	info.type = t;
	if (t->netstyle & CTE_PERSISTANT)
	{
		info.id = MSG_ReadShort();
	}
	else
		info.id = 0;

	if (info.id & 0x8000)
	{
		VectorClear(info.pos);
		VectorClear(info.pos2);
		info.count = 0;
		VectorClear(info.dir);
	}
	else
	{
		MSG_ReadPos (info.pos);

		if (t->netstyle & CTE_ISBEAM)
			MSG_ReadPos (info.pos2);
		else
			VectorCopy(info.pos, info.pos2);

		if (t->netstyle & CTE_CUSTOMCOUNT)
			info.count = MSG_ReadByte();
		else
			info.count = 1;

		if (t->netstyle & CTE_CUSTOMVELOCITY)
		{
			info.dir[0] = MSG_ReadCoord();
			info.dir[1] = MSG_ReadCoord();
			info.dir[2] = MSG_ReadCoord();
		}
		else if (t->netstyle & CTE_CUSTOMDIRECTION)
			MSG_ReadDir (info.dir);
		else
			VectorClear(info.dir);
	}

	if (t->netstyle & CTE_PERSISTANT)
	{
		if (info.id & 0x8000)
		{
			custtentinst_t **link, *o;
			for (link = &activepcusttents; *link; )
			{
				o = *link;
				if (o->id == info.id)
				{
					*link = o->next;
					Z_Free(o);
				}
				else
					link = &(*link)->next;
			}
		}
		else
		{
			//heap fragmentation is going to suck here.
			custtentinst_t *n = Z_Malloc(sizeof(*n));
			info.next = activepcusttents;
			*n = info;
			activepcusttents = n;
		}
	}
	else
		CL_SpawnCustomTEnt(&info);
}
void CL_RefreshCustomTEnts(void)
{
	int i;
	for (i = 0; i < sizeof(customtenttype)/sizeof(customtenttype[0]); i++)
	{
		customtenttype[i].particleeffecttype = (!*customtenttype[i].name)?-1:P_FindParticleType(customtenttype[i].name);

//		if (customtenttype[i].particleeffecttype == P_INVALID && *customtenttype[i].name)
//			Con_DPrintf("%s was not loaded\n", customtenttype[i].name);
	}

	if (cl.particle_ssprecaches)
	{
		for (i = 0; i < MAX_SSPARTICLESPRE; i++)
		{
			if (cl.particle_ssname[i])
				cl.particle_ssprecache[i] = P_FindParticleType(cl.particle_ssname[i]);
			else
				cl.particle_ssprecache[i] = P_INVALID;
		}
	}
	if (cl.particle_csprecaches)
	{
		for (i = 0; i < MAX_CSPARTICLESPRE; i++)
		{
			if (cl.particle_csname[i])
				cl.particle_csprecache[i] = P_FindParticleType(cl.particle_csname[i]);
			else
				cl.particle_csprecache[i] = P_INVALID;
		}
	}
#ifdef CSQC_DAT
	CSQC_ResetTrails();
#endif
}
void CL_ClearCustomTEnts(void)
{
	int i;
	custtentinst_t *p;
	while(activepcusttents)
	{
		p = activepcusttents;
		activepcusttents = p->next;
		Z_Free(p);
	}
	for (i = 0; i < sizeof(customtenttype)/sizeof(customtenttype[0]); i++)
	{
		*customtenttype[i].name = 0;
		customtenttype[i].particleeffecttype = -1;
	}
}

int CL_TranslateParticleFromServer(int qceffect)
{
	if (cl.particle_ssprecaches && qceffect >= 0 && qceffect < MAX_SSPARTICLESPRE)
	{
		/*proper precaches*/
		return cl.particle_ssprecache[qceffect];
	}
	else if (-qceffect >= 0 && -qceffect < MAX_CSPARTICLESPRE)
	{
		qceffect = -qceffect;
		return cl.particle_csprecache[qceffect];
	}
	else
		return P_INVALID;
}

void CL_ParseTrailParticles(void)
{
	int entityindex;
	int effectindex;
	vec3_t start, end;
	trailkey_t *tk;

	entityindex = MSGCL_ReadEntity();
	effectindex = (unsigned short)MSG_ReadShort();

	start[0] = MSG_ReadCoord();
	start[1] = MSG_ReadCoord();
	start[2] = MSG_ReadCoord();
	end[0] = MSG_ReadCoord();
	end[1] = MSG_ReadCoord();
	end[2] = MSG_ReadCoord();

	effectindex = CL_TranslateParticleFromServer(effectindex);

	if (entityindex>0 && (unsigned int)entityindex < cl.maxlerpents)
		tk = &cl.lerpents[entityindex].trailstate;
	else
		tk = NULL;

	if (P_ParticleTrail(start, end, effectindex, 1, entityindex, NULL, tk))
		P_ParticleTrail(start, end, rt_blood, 1, entityindex, NULL, tk);
}

void CL_ParsePointParticles(qboolean compact)
{
	vec3_t		org, dir;
	unsigned int effectindex;
	float count;

	effectindex = (unsigned short)MSG_ReadShort();
	org[0] = MSG_ReadCoord();
	org[1] = MSG_ReadCoord();
	org[2] = MSG_ReadCoord();
	if (compact)
	{
		dir[0] = dir[1] = dir[2] = 0;
		count = 1;
	}
	else
	{
		dir[0] = MSG_ReadCoord();
		dir[1] = MSG_ReadCoord();
		dir[2] = MSG_ReadCoord();
		count = (unsigned short)MSG_ReadShort();
	}

	effectindex = CL_TranslateParticleFromServer(effectindex);

	if (cl.splitclients <= 1 && cl_part_density_fade.value > 0)
	{
		vec3_t move;
		float dist;
		VectorSubtract(org, cl.playerview[0].audio.origin, move);
		dist = VectorLength(move);
		if (dist > cl_part_density_fade_start.value)
		{
			dist -= cl_part_density_fade_start.value;
			count = count - dist/cl_part_density_fade.value;
			if (count < 0)
				return;
		}
	}

	if (P_RunParticleEffectType(org, dir, count, effectindex))
		P_RunParticleEffect (org, dir, 15, 15);
}

void CLNQ_ParseParticleEffect (void)
{
	vec3_t		org, dir;
	int			i, msgcount, color;

	for (i=0 ; i<3 ; i++)
		org[i] = MSG_ReadCoord ();
	for (i=0 ; i<3 ; i++)
		dir[i] = MSG_ReadChar () * (1.0/16);
	msgcount = MSG_ReadByte ();
	color = MSG_ReadByte ();

	if (msgcount == 255)
	{
		// treat as spriteless explosion (qtest/some mods require this)
		if (P_RunParticleEffectType(org, NULL, 1, pt_explosion))
			P_RunParticleEffect(org, NULL, 107, 1024); // should be 97-111
	}
	else
		P_RunParticleEffect (org, dir, color, msgcount);
}
void CL_ParseParticleEffect2 (void)
{
	vec3_t		org, dmin, dmax;
	int			i, msgcount, color, effect;

	for (i=0 ; i<3 ; i++)
		org[i] = MSG_ReadCoord ();
	for (i=0 ; i<3 ; i++)
		dmin[i] = MSG_ReadFloat ();
	for (i=0 ; i<3 ; i++)
		dmax[i] = MSG_ReadFloat ();
	color = MSG_ReadShort ();
	msgcount = MSG_ReadByte ();
	effect = MSG_ReadByte ();

	P_RunParticleEffect2 (org, dmin, dmax, color, effect, msgcount);
}
void CL_ParseParticleEffect3 (void)
{
	vec3_t		org, box;
	int			i, msgcount, color, effect;

	for (i=0 ; i<3 ; i++)
		org[i] = MSG_ReadCoord ();
	for (i=0 ; i<3 ; i++)
		box[i] = MSG_ReadByte ();
	color = MSG_ReadShort ();
	msgcount = MSG_ReadByte ();
	effect = MSG_ReadByte ();

	P_RunParticleEffect3 (org, box, color, effect, msgcount);
}
void CL_ParseParticleEffect4 (void)
{
	vec3_t		org;
	int			i, msgcount, color, effect;
	float		radius;

	for (i=0 ; i<3 ; i++)
		org[i] = MSG_ReadCoord ();
	radius = MSG_ReadByte();
	color = MSG_ReadShort ();
	msgcount = MSG_ReadByte ();
	effect = MSG_ReadByte ();

	P_RunParticleEffect4 (org, radius, color, effect, msgcount);
}

void CL_SpawnSpriteEffect(vec3_t org, vec3_t dir, vec3_t orientationup, model_t *model, int startframe, int framecount, float framerate, float alpha, float scale, float randspin, float gravity, int traileffect, unsigned int renderflags, int skinnum, float red, float green, float blue)
{
	explosion_t	*ex;

	ex = CL_AllocExplosion (org);
	ex->start = cl.time;
	ex->model = model;
	ex->firstframe = startframe;
	ex->numframes = framecount;
	ex->framerate = framerate;
	ex->skinnum = skinnum;
	ex->traileffect = traileffect;
	ex->scale = scale;
	ex->rgb[0] = red;
	ex->rgb[1] = green;
	ex->rgb[2] = blue;

	ex->flags |= renderflags;

	//sprites always use a fixed alpha. models can too if the alpha is < 0
	if (model->type == mod_sprite || alpha < 0)
		ex->endalpha = fabs(alpha);
	ex->startalpha = fabs(alpha);

	if (ex->endalpha < 1 || ex->startalpha < 1)
		ex->flags |= RF_TRANSLUCENT;

	if (randspin)
	{
		ex->angles[0] = frandom()*360;
		ex->angles[1] = frandom()*360;
		ex->angles[2] = frandom()*360;

		ex->avel[0] = crandom()*randspin;
		ex->avel[1] = crandom()*randspin;
		ex->avel[2] = crandom()*randspin;
	}
	ex->gravity = gravity;

	if (orientationup)
	{
		ex->angles[0] = acos(orientationup[2])/M_PI*180;
		if (orientationup[0])
			ex->angles[1] = atan2(orientationup[1], orientationup[0])/M_PI*180;
		else if (orientationup[1] > 0)
			ex->angles[1] = 90;
		else if (orientationup[1] < 0)
			ex->angles[1] = 270;
		else
			ex->angles[1] = 0;
		ex->angles[0]*=r_meshpitch.value;
		ex->angles[2]*=r_meshroll.value;
	}


	if (dir)
	{
//		vec3_t spos;
//		float dlen;
//		dlen = -10/VectorLength(dir);
//		VectorMA(ex->origin, dlen, dir, spos);
//		TraceLineN(spos, org, ex->origin, NULL);

		VectorCopy(dir, ex->velocity);
	}
	else
		VectorClear(ex->velocity);
}

// [vector] org [byte] modelindex [byte] startframe [byte] framecount [byte] framerate
// [vector] org [short] modelindex [short] startframe [byte] framecount [byte] framerate
void CL_ParseEffect (qboolean effect2)
{
	vec3_t org;
	int modelindex;
	int startframe;
	int framecount;
	int framerate;
	model_t *mod;

	org[0] = MSG_ReadCoord();
	org[1] = MSG_ReadCoord();
	org[2] = MSG_ReadCoord();

	if (effect2)
		modelindex = MSG_ReadShort();
	else
		modelindex = MSG_ReadByte();

	if (effect2)
		startframe = MSG_ReadShort();
	else
		startframe = MSG_ReadByte();

	framecount = MSG_ReadByte();
	framerate = MSG_ReadByte();

	mod = cl.model_precache[modelindex];
	CL_SpawnSpriteEffect(org, NULL, NULL, mod, startframe, framecount, framerate, mod->type==mod_sprite?-1:1, 1, 0, 0, P_INVALID, 0, 0, 1.0f, 1.0f, 1.0f);
}

#ifdef Q2CLIENT
void CL_SmokeAndFlash(vec3_t origin)
{
	explosion_t	*ex;

	ex = CL_AllocExplosion (origin);
	VectorClear(ex->angles);
//	ex->type = ex_misc;
	ex->numframes = 4;
	ex->flags = RF_TRANSLUCENT;
	ex->start = cl.time;
	ex->model = Mod_ForName (q2tentmodels[q2cl_mod_smoke].modelname, MLV_WARN);

	ex = CL_AllocExplosion (origin);
	VectorClear(ex->angles);
//	ex->type = ex_flash;
	ex->flags = RF_FULLBRIGHT;
	ex->numframes = 2;
	ex->start = cl.time;
	ex->model = Mod_ForName (q2tentmodels[q2cl_mod_flash].modelname, MLV_WARN);
}

void CL_Laser (vec3_t start, vec3_t end, int colors)
{
	explosion_t	*ex = CL_AllocExplosion(start);
	ex->firstframe = 0;
	ex->numframes = 10;
	ex->startalpha = 0.33f;
	ex->endalpha = 0;
	ex->model = NULL;
	ex->skinnum = (colors >> ((rand() % 4)*8)) & 0xff;
	VectorCopy (start, ex->origin);
	VectorCopy (end, ex->oldorigin);
	ex->flags = RF_TRANSLUCENT | Q2RF_BEAM;
	ex->start = cl.time;
	ex->framerate = 100; // smoother fading
}

void CLQ2_ParseSteam(void)
{
	vec3_t pos, dir;
	/*qbyte colour;
	short magnitude;
	unsigned int duration;*/
	signed int id = MSG_ReadShort();
	/*qbyte count =*/ MSG_ReadByte();
	MSG_ReadPos(pos);
	MSG_ReadDir(dir);
	/*colour =*/ MSG_ReadByte();
	/*magnitude =*/ MSG_ReadShort();

	if (id != -1)
		/*duration =*/ MSG_ReadLong();
	else
		/*duration = 0;*/

	Con_Printf("FIXME: CLQ2_ParseSteam: stub\n");
}

void CLQ2_ParseTEnt (void)
{
	beam_t *b;
	q2particleeffects_t		type;
	int		pt;
	vec3_t	pos, pos2, dir;
	int		cnt;
	int		color;
	int		r;
	int		ent;
//	int		magnitude;
//	explosion_t	*ex;

	type = MSG_ReadByte ();

	if (type <= Q2TE_MAX)
	{
		if (cl_shownet.ival > 1)
			Con_Printf("\t%s\n", q2efnames[type]);
		pt = pt_q2[type];
	}
	else
	{
		if (cl_shownet.ival > 1)
			Con_Printf("\tTE%u\n", type);
		pt = P_INVALID;
	}
	switch (type)
	{
	case Q2TE_GUNSHOT:	//grey tall thing with smoke+sparks
	case Q2TE_BLOOD:		//red tall thing
	case Q2TE_SPARKS:		//orange tall thing (with not many particles)
	case Q2TE_BLASTER:	//regular blaster
	case Q2TE_SHOTGUN:	//gunshot with less particles
	case Q2TE_SCREEN_SPARKS://green+grey tall
	case Q2TE_SHIELD_SPARKS://blue+grey tall
	case Q2TE_BULLET_SPARKS://orange+grey tall+smoke
	case Q2TE_GREENBLOOD:	//yellow...
	case Q2TE_BLASTER2:	//green version of te_blaster
	case Q2TE_MOREBLOOD:	//te_blood*2
	case Q2TE_HEATBEAM_SPARKS://white outwards puffs
	case Q2TE_HEATBEAM_STEAM://orange outwards puffs
	case Q2TE_ELECTRIC_SPARKS://blue tall
	case Q2TE_FLECHETTE:	//grey version of te_blaster
		MSG_ReadPos (pos);
		MSG_ReadDir (dir);
		P_RunParticleEffectType(pos, dir, 1, pt);
		break;
	case Q2TE_BFG_LASER:
		MSG_ReadPos (pos);
		MSG_ReadPos (pos2);
		CL_Laser(pos, pos2, 0xd0d1d2d3);
		break;
	case Q2TE_RAILTRAIL:	//blue spiral, grey particles
	case Q2TE_BUBBLETRAIL:	//grey sparse trail, slow riser
//	case Q2TE_BFG_LASER:	//green laser
	case Q2TE_DEBUGTRAIL:	//long lived blue trail
	case Q2TE_BUBBLETRAIL2:	//grey rising trail
	case Q2TE_BLUEHYPERBLASTER:	//TE_BLASTER without model+light
	case Q2TEEX_RAILTRAIL2:
	case Q2TEEX_BFGZAP:
		MSG_ReadPos (pos);
		MSG_ReadPos (pos2);
		P_ParticleTrail(pos, pos2, pt, 1, 0, NULL, NULL);
		P_RunParticleEffectTypeString(pos2, NULL, 1, va("%s_end", q2efnames[type]));
		break;
	case Q2TE_EXPLOSION1:	//column
	case Q2TE_EXPLOSION2:	//splits
	case Q2TE_ROCKET_EXPLOSION://top blob/column
	case Q2TE_GRENADE_EXPLOSION://indistinguishable from TE_EXPLOSION2
	case Q2TE_ROCKET_EXPLOSION_WATER://rocket but with different sound
	case Q2TE_GRENADE_EXPLOSION_WATER://different sound
	case Q2TE_BFG_EXPLOSION://green light+sprite
	case Q2TE_BFG_BIGEXPLOSION://green+white fast particles
	case Q2TE_BOSSTPORT://splitting+merging+upwards particles.
	case Q2TE_PLASMA_EXPLOSION://looks like rocket explosion to me
	case Q2TE_PLAIN_EXPLOSION://looks like rocket explosion to me
	case Q2TE_CHAINFIST_SMOKE://small smoke
	case Q2TE_TRACKER_EXPLOSION://black light, slow particles
	case Q2TE_TELEPORT_EFFECT://q1-style teleport
	case Q2TE_DBALL_GOAL://q1-style teleport
	case Q2TE_NUKEBLAST://dome expansion (blue/white particles)
	case Q2TE_WIDOWSPLASH://dome (orange+gravity)
	case Q2TE_EXPLOSION1_BIG://buggy model
	case Q2TE_EXPLOSION1_NP://looks like a rocket explosion to me
		MSG_ReadPos (pos);
		P_RunParticleEffectType(pos, NULL, 1, pt);
		break;
	case Q2TE_SPLASH:
		cnt = MSG_ReadByte ();
		MSG_ReadPos (pos);
		MSG_ReadDir (dir);
		r = MSG_ReadByte () + Q2SPLASH_UNKNOWN;
		if (r > Q2SPLASH_MAX)
			r = Q2SPLASH_UNKNOWN;
		pt = pt_q2[r];
		P_RunParticleEffectType(pos, NULL, 1, pt);
		break;

	case Q2TE_PARASITE_ATTACK:
	case Q2TE_MEDIC_CABLE_ATTACK:
		CL_ParseBeam (BT_Q2PARASITE);
		break;
	case Q2TE_HEATBEAM:
		b = CL_ParseBeam (BT_Q2HEATBEAM);	//2, 7, -3
		if (b)
		{
			b->bflags |= STREAM_ATTACHED;
			VectorSet(b->offset, 2, 7, -3);
		}
		break;
	case Q2TE_MONSTER_HEATBEAM:
		b = CL_ParseBeam (BT_Q2HEATBEAM);
		if (b)
			b->bflags |= STREAM_ATTACHED;
		break;
	case Q2TE_GRAPPLE_CABLE:
		CL_ParseBeamOffset (BT_Q2GRAPPLE);
		break;

	case Q2TE_LIGHTNING:
		ent = MSGCL_ReadEntity ();
		/*toent =*/ MSGCL_ReadEntity ();	//ident only.
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();
		pos2[0] = MSG_ReadCoord ();
		pos2[1] = MSG_ReadCoord ();
		pos2[2] = MSG_ReadCoord ();
		CL_AddBeam(BT_Q2LIGHTNING, ent, pos, pos2);
		break;

	case Q2TE_LASER_SPARKS:
	case Q2TE_WELDING_SPARKS:
	case Q2TE_TUNNEL_SPARKS:
		cnt = MSG_ReadByte ();
		MSG_ReadPos (pos);
		MSG_ReadDir (dir);
		color = MSG_ReadByte ();
		P_RunParticleEffectPalette(va("q2part.%s", q2efnames[type]), pos, dir, color, cnt);
		break;

	case Q2TE_STEAM:
		CLQ2_ParseSteam();
		break;

	case Q2TE_FORCEWALL:
		MSG_ReadPos (pos);
		MSG_ReadPos (pos2);
		color = MSG_ReadByte ();
		P_ParticleTrailIndex(pos, pos2, pt, 1, color, 0, NULL);
		break;

	case Q2TE_FLASHLIGHT:	//white 400-radius dlight
		MSG_ReadPos(pos);
		ent = MSG_ReadShort();
		P_ParticleTrail(pos, pos, pt, 1, ent, NULL, NULL);
		break;
	case Q2TE_WIDOWBEAMOUT:		/*requires state tracking to keep it splurting constantly for 2.1 secs*/
		ent = MSG_ReadShort();
		MSG_ReadPos(pos);
		Con_Printf("FIXME: Q2TE_WIDOWBEAMOUT not implemented\n");
		break;

	//case Q2TE_RAILTRAIL2:		/*not implemented in vanilla*/
	case Q2TE_FLAME:			/*not implemented in vanilla*/
		Host_EndGame ("CLQ2_ParseTEnt: bad/non-implemented type %i", type);
		break;

	case Q2TEEX_BLUEHYPERBLASTER:
	case Q2TEEX_BERSERK_SLAM:
		MSG_ReadPos (pos);
		MSG_ReadDir (pos2);
		P_RunParticleEffectType(pos, dir, 1, pt);
		break;

	case Q2TEEX_GRAPPLE_CABLE_2:
		CL_ParseBeam (BT_Q2GRAPPLE);
		break;

	case Q2TEEX_POWER_SPLASH:
		MSGCL_ReadEntity();
		MSG_ReadByte();
		break;
	case Q2TEEX_LIGHTNING_BEAM:
		CL_ParseBeam (BT_Q2LIGHTNING);
		break;
	case Q2TEEX_EXPLOSION1_NL:
	case Q2TEEX_EXPLOSION2_NL:
		MSG_ReadPos (pos);
		P_RunParticleEffectType(pos, NULL, 1, pt);
		break;


	//My old attempt at running AlienArena years ago. probably not enough now. Other engines will have other effects.
/*	case CRTE_LEADERBLASTER:
		Host_EndGame ("CLQ2_ParseTEnt: bad/non-implemented type %i", type);
	case CRTE_BLASTER_MUZZLEFLASH:
		MSG_ReadPos (pos);
		ex = CL_AllocExplosion (pos);
		ex->flags = RF_FULLBRIGHT|RF_NOSHADOW;
		ex->start = cl.q2frame.servertime - 100;
		CL_NewDlight(0, pos, 350, 0.5, 0.2*5, 0.1*5, 0*5);
		P_RunParticleEffectTypeString(pos, NULL, 1, "te_muzzleflash");
		break;
	case CRTE_BLUE_MUZZLEFLASH:
		MSG_ReadPos (pos);
		ex = CL_AllocExplosion (pos);
		ex->flags = RF_FULLBRIGHT|RF_NOSHADOW;
		ex->start = cl.q2frame.servertime - 100;
		CL_NewDlight(0, pos, 350, 0.5, 0.2*5, 0.1*5, 0*5);
		P_RunParticleEffectTypeString(pos, NULL, 1, "te_blue_muzzleflash");
		break;
	case CRTE_SMART_MUZZLEFLASH:
		MSG_ReadPos (pos);
		ex = CL_AllocExplosion (pos);
		ex->flags = RF_FULLBRIGHT|RF_NOSHADOW;
		ex->start = cl.q2frame.servertime - 100;
		CL_NewDlight(0, pos, 350, 0.5, 0.2*5, 0*5, 0.2*5);
		P_RunParticleEffectTypeString(pos, NULL, 1, "te_smart_muzzleflash");
		break;
	case CRTE_LEADERFIELD:
		Host_EndGame ("CLQ2_ParseTEnt: bad/non-implemented type %i", type);
	case CRTE_DEATHFIELD:
		MSG_ReadPos (pos);
		ex = CL_AllocExplosion (pos);
		VectorCopy (pos, ex->origin);
		ex->flags = RF_FULLBRIGHT|RF_NOSHADOW;
		ex->start = cl.q2frame.servertime - 100;
		CL_NewDlight(0, pos, 350, 0.5, 0.2*5, 0*5, 0.2*5);
		P_RunParticleEffectTypeString(pos, NULL, 1, "te_deathfield");
		break;
	case CRTE_BLASTERBEAM:
		MSG_ReadPos (pos);
		MSG_ReadPos (pos2);
		P_ParticleTrail(pos, pos2, P_FindParticleType("q2part.TR_BLASTERTRAIL2"), 1, 0, NULL, NULL);
		break;
	case CRTE_STAIN:
		Host_EndGame ("CLQ2_ParseTEnt: bad/non-implemented type %i", type);
	case CRTE_FIRE:
		Host_EndGame ("CLQ2_ParseTEnt: bad/non-implemented type %i", type);
	case CRTE_CABLEGUT:
		Host_EndGame ("CLQ2_ParseTEnt: bad/non-implemented type %i", type);
	case CRTE_SMOKE:
		Host_EndGame ("CLQ2_ParseTEnt: bad/non-implemented type %i", type);
*/

	default:
		Host_EndGame ("CLQ2_ParseTEnt: bad/non-implemented type %i", type);
		break;
	}
}
#endif


/*
=================
CL_NewTempEntity
=================
*/
entity_t *CL_NewTempEntity (void)
{
	entity_t	*ent;

	if (cl_numvisedicts == cl_maxvisedicts)
		return NULL;
	ent = &cl_visedicts[cl_numvisedicts];
	cl_numvisedicts++;
	ent->keynum = 0;

	memset (ent, 0, sizeof(*ent));
	ent->playerindex = -1;
	ent->topcolour = TOP_DEFAULT;
	ent->bottomcolour = BOTTOM_DEFAULT;

#ifdef PEXT_SCALE
	ent->scale = 1;
#endif
	ent->shaderRGBAf[0] = 1;
	ent->shaderRGBAf[1] = 1;
	ent->shaderRGBAf[2] = 1;
	ent->shaderRGBAf[3] = 1;
	return ent;
}

qboolean CSQC_GetEntityOrigin(unsigned int csqcent, float *out);

/*
=================
CL_UpdateBeams
=================
*/
void CL_UpdateBeams (float frametime)
{
	int bnum;
	int			i, j;
	beam_t		*b;
	vec3_t		dist, org;
	float		*vieworg;
	float		d;
	entity_t	*ent;
	entity_state_t *st;
	float		yaw, pitch;
	float		forward, offset;
	int lastrunningbeam = -1;
	tentmodels_t *type;

	extern cvar_t cl_truelightning, v_viewheight;

// update lightning
	for (bnum=0, b=cl_beams; bnum < beams_running; bnum++, b++)
	{
		type = b->info;
		if (!type)
			continue;

		if (b->endtime < cl.time)
		{
			if (!cl.paused)
			{	/*don't let lightning decay while paused*/
				P_DelinkTrailstate(&b->trailstate);
				P_DelinkTrailstate(&b->emitstate);
				b->info = NULL;
				continue;
			}
		}

		lastrunningbeam = bnum;

	// if coming from the player, update the start position
		if ((b->bflags & STREAM_ATTACHTOPLAYER) && b->entity > 0 && b->entity <= cl.allocated_client_slots)
		{
			for (j = 0; j < cl.splitclients; j++)
			{
				playerview_t *pv = &cl.playerview[j];
				if (b->entity == pv->viewentity)
				{
//					player_state_t	*pl;
		//			VectorSubtract(cl.simorg, b->start, org);
		//			VectorAdd(b->end, org, b->end);		//move the end point by simorg-start

//					pl = &cl.inframes[cl.parsecount&UPDATE_MASK].playerstate[b->entity-1];
//					if (pl->messagenum == cl.parsecount || cls.protocol == CP_NETQUAKE)
					{
						vec3_t	fwd, org, ang, viewang;
						float	delta, f, len;

//						if (cl.spectator && pv->cam_auto)
//						{	//if we're tracking someone, use their origin explicitly.
//							vieworg = pl->origin;
//						}
//						else
#ifdef Q2CLIENT
						if (cls.protocol == CP_QUAKE2)
							vieworg = pv->predicted_origin;
						else
#endif
							vieworg = pv->simorg;

						if (cl_truelightning.ival > 1 && cl.movesequence > cl_truelightning.ival)
						{
							outframe_t *frame = &cl.outframes[(cl.movesequence-cl_truelightning.ival)&UPDATE_MASK];
							viewang[0] = SHORT2ANGLE(frame->cmd[j].angles[0]);
							viewang[1] = SHORT2ANGLE(frame->cmd[j].angles[1]);
							viewang[2] = SHORT2ANGLE(frame->cmd[j].angles[2]);
						}
						else
							VectorCopy(pv->simangles, viewang);

						VectorCopy (vieworg, b->start);
						b->start[2] += pv->crouch + bound(-7, v_viewheight.value, 4);

						f = bound(0, cl_truelightning.value, 1);

						if (!f)
							break;

						VectorSubtract (playerbeam_end[j], vieworg, org);
						len = VectorLength(org);
						org[2] -= 22;		// adjust for view height
						VectorAngles (org, NULL, ang, false);

						// lerp pitch
						delta = anglemod(viewang[0] - ang[0]);
						if (delta > 180)
							delta -= 360;
						if (ang[0] < -180)
							ang[0] += 360;
						ang[0] += delta * f;

						// lerp yaw
						delta = anglemod(viewang[1] - ang[1]);
						if (delta > 180)
							delta -= 360;
						if (delta < -180)
							delta += 360;
						ang[1] += delta * f;
						ang[2] = 0;

						AngleVectors (ang, fwd, ang, ang);
						VectorCopy(fwd, ang);
						VectorScale (fwd, len, fwd);
						VectorCopy (vieworg, org);
						org[2] += 16;
						VectorAdd (org, fwd, b->end);

						if (cl_beam_trace.ival)
						{
							vec3_t normal;
							VectorMA(org, len+4, ang, fwd);
							if (CL_TraceLine(org, fwd, ang, normal, NULL) < 1)
								VectorCopy (ang, b->end);
						}
						break;
					}
				}
			}
			VectorCopy (b->start, org);
		}
#ifdef CSQC_DAT
		else if ((b->bflags & 1) && b->entity > MAX_EDICTS)
		{
			CSQC_GetEntityOrigin(b->entity-MAX_EDICTS, org);
			VectorCopy (b->start, org);
		}
#endif
		else if (b->bflags & STREAM_ATTACHED)
		{
			player_state_t	*pl;
			st = CL_FindPacketEntity(b->entity);
			if (st)
			{
				VectorCopy(st->origin, b->start);
			}
			else if (b->entity <= cl.allocated_client_slots && b->entity > 0)
			{
				pl = &cl.inframes[cl.parsecount&UPDATE_MASK].playerstate[b->entity-1];
				VectorCopy(pl->origin, b->start);
			}
	
			VectorCopy (b->start, org);
		}
		else
			VectorCopy (b->start, org);
		VectorAdd(org, b->offset, org);

	// calculate pitch and yaw
		VectorSubtract (b->end, org, dist);

		if (dist[1] == 0 && dist[0] == 0)
		{
			yaw = 0;
			if (dist[2] > 0)
				pitch = 90;
			else
				pitch = 270;
		}
		else
		{
			yaw = (atan2(dist[1], dist[0]) * 180 / M_PI);
			if (yaw < 0)
				yaw += 360;

			forward = sqrt (dist[0]*dist[0] + dist[1]*dist[1]);
			pitch = (atan2(dist[2], forward) * 180 / M_PI);
			if (pitch < 0)
				pitch += 360;
		}

		if (ruleset_allow_particle_lightning.ival || !type->modelname)
			if (type->ef_beam >= 0 && !P_ParticleTrail(org, b->end, type->ef_beam, frametime, b->entity, NULL, &b->trailstate))
				continue;
		if (!type->model)
		{
			type->model = type->modelname?Mod_ForName(type->modelname, MLV_WARN):NULL;
			if (!type->model)
				continue;
		}

	// add new entities for the lightning
		d = VectorNormalize(dist);

		if(b->bflags & STREAM_JITTER)
		{
			offset = (int)(cl.time*40)%30;
			for(i = 0; i < 3; i++)
			{
				org[i] += dist[i]*offset;
			}
		}

		while (d > 0)
		{
			ent = CL_NewTempEntity ();
			if (!ent)
				return;
			VectorCopy (org, ent->origin);
			ent->model = type->model;
			ent->skinnum = b->skin;
#ifdef HEXEN2
			ent->drawflags |= MLS_ABSLIGHT;
			ent->abslight = 64 + 128 * bound(0, cl_shaftlight.value, 1);
#endif
			ent->shaderRGBAf[3] = b->alpha;
			ent->flags = b->rflags;

			ent->angles[0] = -pitch;
			ent->angles[1] = yaw;
			ent->angles[2] = rand()%360;
			AngleVectorsFLU(ent->angles, ent->axis[0], ent->axis[1], ent->axis[2]);
			ent->angles[0] = pitch;
			ent->framestate.g[FS_REG].lerpweight[0] = 1;
			ent->framestate.g[FS_REG].frame[0] = 0;
			ent->framestate.g[FS_REG].frametime[0] = cl.time - (b->endtime - 0.2);

			for (i=0 ; i<3 ; i++)
				org[i] += dist[i]*30;
			d -= 30;
		}

		if (type->beamimpactmodel)
		{
			if (!type->impactmodel)
			{
				type->impactmodel = Mod_ForName(type->beamimpactmodel, MLV_WARN);
				if (!type->impactmodel)
					continue;
			}
			ent = CL_NewTempEntity ();
			if (!ent)
				return;
			VectorCopy (b->end, ent->origin);
			ent->model = type->impactmodel;
#ifdef HEXEN2
			ent->drawflags |= MLS_ABSLIGHT;
			ent->abslight = 128;
#endif
			ent->shaderRGBAf[3] = b->alpha;
			ent->flags = b->rflags;
			ent->scale = type->impactscale;

			ent->angles[0] = -pitch;
			ent->angles[1] = yaw;
			ent->angles[2] = rand()%360;
			AngleVectorsFLU(ent->angles, ent->axis[0], ent->axis[1], ent->axis[2]);
			ent->angles[0] = pitch;
			ent->framestate.g[FS_REG].lerpweight[0] = 1;
			ent->framestate.g[FS_REG].frame[0] = 0;
			ent->framestate.g[FS_REG].frametime[0] = cl.time - (b->endtime - 0.2);
		}
	}

	beams_running = lastrunningbeam+1;
}

/*
=================
CL_UpdateExplosions
=================
*/
void CL_UpdateExplosions (float frametime)
{
	int			i;
	float		f;
	int			of;
	int numframes;
	int firstframe;
	explosion_t	*ex;
	entity_t	*ent;
	int lastrunningexplosion = -1;
	vec3_t pos, norm;
	float		scale;

	for (i=0, ex=cl_explosions; i < explosions_running; i++, ex++)
	{
		if (!ex->model && !(ex->flags&Q2RF_BEAM))
			continue;

		lastrunningexplosion = i;
		if (ex->model)
		{
			if (ex->model->loadstate == MLS_LOADING)
				continue;
			if (ex->model->loadstate != MLS_LOADED)
			{
				ex->model = NULL;
				ex->flags = 0;
				P_DelinkTrailstate(&(ex->trailstate));
				continue;
			}
		}

		f = ex->framerate*(cl.time - ex->start);

		if (ex->firstframe >= 0)
		{
			firstframe = ex->firstframe;
			numframes = ex->numframes;

			if (!numframes)
				numframes = ex->model->numframes - firstframe;
		}
		else
		{
			firstframe = 0;
			numframes = ex->model->numframes;
		}

		of = (int)f-1;
		if (ex->endalpha && (int)f == numframes)
		{
			scale = 1-(f-(int)f);	//if we have endalpha not 0, then there is a final 'bonus' frame where the model scales down to 0. use numframes+framerate to control how fast it fades.
			f = of;	//clamp it to the old frame
		}
		else if ((int)f >= numframes || (int)f < 0)
		{
			ex->model = NULL;
			ex->flags = 0;
			P_DelinkTrailstate(&(ex->trailstate));
			continue;
		}
		else
			scale = 1;
		if (of < 0)
			of = 0;

		ent = CL_NewTempEntity ();
		if (!ent)
			return;

		if (ex->gravity)
		{
			VectorMA(ex->origin, frametime, ex->velocity, pos);
			if (ex->velocity[0] || ex->velocity[1] || ex->velocity[2])
			{
				VectorClear(norm);
				if (CL_TraceLine(ex->origin, pos, ent->origin, norm, NULL) < 1)
				{
					float sc = DotProduct(ex->velocity, norm) * -1.5;
					VectorMA(ex->velocity, sc, norm, ex->velocity);
					VectorScale(ex->velocity, 0.9, ex->velocity);
					if (norm[2] > 0.7 && DotProduct(ex->velocity, ex->velocity) < 100)
					{
						VectorClear(ex->velocity);
						VectorClear(ex->avel);
					}
				}
				else
					ex->velocity[2] -= ex->gravity * frametime;
				VectorCopy(ent->origin, ex->origin);
			}
			else
				VectorCopy(ex->origin, ent->origin);
		}
		else
		{
			VectorMA (ex->origin, f, ex->velocity, ent->origin);
		}
		VectorMA(ex->angles, frametime, ex->avel, ex->angles);

		VectorCopy (ex->oldorigin, ent->oldorigin);
		VectorCopy (ex->angles, ent->angles);
		ent->skinnum = ex->skinnum;
		ent->angles[0]*=r_meshpitch.value;
		ent->angles[2]*=r_meshroll.value;
		AngleVectors(ent->angles, ent->axis[0], ent->axis[1], ent->axis[2]);
		VectorInverse(ent->axis[1]);
		ent->model = ex->model;
		ent->framestate.g[FS_REG].frame[1] = (int)f+firstframe;
		ent->framestate.g[FS_REG].frame[0] = of+firstframe;
		ent->framestate.g[FS_REG].lerpweight[1] = (f - (int)f);
		ent->framestate.g[FS_REG].lerpweight[0] = 1-ent->framestate.g[FS_REG].lerpweight[1];
		ent->shaderRGBAf[0] = ex->rgb[0];
		ent->shaderRGBAf[1] = ex->rgb[1];
		ent->shaderRGBAf[2] = ex->rgb[2];
		ent->shaderRGBAf[3] = (1.0 - f/(numframes))*(ex->startalpha-ex->endalpha) + ex->endalpha;
		ent->flags = ex->flags;
		ent->scale = scale*ex->scale;
#ifdef HEXEN2
		ent->drawflags = SCALE_ORIGIN_ORIGIN;
#endif
		cl.worldmodel->funcs.FindTouchedLeafs(cl.worldmodel, &ent->pvscache, ent->origin, ent->origin);

		if (ex->traileffect != P_INVALID)
			pe->ParticleTrail(ent->oldorigin, ent->origin, ex->traileffect, frametime, 0, ent->axis, &(ex->trailstate));
		if (!(ex->flags & Q2RF_BEAM))
			VectorCopy(ent->origin, ex->oldorigin);	//don't corrupt q2 beams
		if (ex->flags & Q2RF_BEAM)
		{
			ent->rtype = RT_BEAM;
			ent->shaderRGBAf[0] = ((d_8to24srgbtable[ex->skinnum & 0xFF] >>  0) & 0xFF)/255.0;
			ent->shaderRGBAf[1] = ((d_8to24srgbtable[ex->skinnum & 0xFF] >>  8) & 0xFF)/255.0;
			ent->shaderRGBAf[2] = ((d_8to24srgbtable[ex->skinnum & 0xFF] >> 16) & 0xFF)/255.0;
		}
		else if (ex->skinnum < 0)
		{
			ent->skinnum = 7*f/(numframes);
		}
	}

	explosions_running = lastrunningexplosion + 1;
}

entity_state_t *CL_FindPacketEntity(int num);

/*
=================
CL_UpdateTEnts
=================
*/
void CL_UpdateTEnts (void)
{
	static float oldtime;
	float frametime = cl.time - oldtime;
	if (frametime < 0 || frametime > 100)
		frametime = 0;
	oldtime = cl.time;

	CL_UpdateBeams (frametime);
	CL_UpdateExplosions (frametime);
	CL_RunPCustomTEnts ();
}
