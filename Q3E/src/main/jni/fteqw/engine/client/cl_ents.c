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
// cl_ents.c -- entity parsing and management

#include "quakedef.h"
#include "particles.h"
#include "shader.h"
#include "glquake.h"

extern	cvar_t	cl_predict_players;
extern	cvar_t	cl_predict_players_frac;
extern	cvar_t	cl_predict_players_latency;
extern	cvar_t	cl_predict_players_nudge;
extern	cvar_t	cl_lerp_players;
extern  cvar_t	cl_lerp_maxinterval;
extern	cvar_t	cl_lerp_maxdistance;
extern	cvar_t	cl_solid_players;
extern	cvar_t	cl_item_bobbing;

extern	cvar_t	r_rocketlight;
extern	cvar_t	r_lightflicker;
extern	cvar_t	r_dimlight_colour;
extern	cvar_t	r_brightlight_colour;
extern	cvar_t	r_redlight_colour;
extern	cvar_t	r_greenlight_colour;
extern	cvar_t	r_bluelight_colour;
extern	cvar_t	cl_r2g;
extern	cvar_t	r_powerupglow;
extern	cvar_t	v_powerupshell;
extern	cvar_t	cl_nolerp;
extern	cvar_t	cl_nolerp_netquake;
extern	cvar_t	r_torch;
extern  cvar_t r_shadows;
extern	cvar_t	r_showbboxes;
extern	cvar_t gl_simpleitems;
float r_blobshadows;

extern	cvar_t	cl_gibfilter, cl_deadbodyfilter;
extern int cl_playerindex;
static qboolean cl_expandvisents;

extern world_t csqc_world;

static struct predicted_player
{
	int flags;
	qboolean active;
	vec3_t origin;	// predicted origin

	vec3_t	oldo;
	vec3_t	olda;
	vec3_t	oldv;
	qboolean predict;
	player_state_t *oldstate;
} predicted_players[MAX_CLIENTS];

static void CL_LerpNetFrameState(framestate_t *fs, lerpents_t *le);
void CL_PlayerFrameUpdated(player_state_t *plstate, entity_state_t *state, int sequence);
void CL_AckedInputFrame(int inseq, int outseq, qboolean worldstateokay);


extern int cl_playerindex, cl_h_playerindex, cl_rocketindex, cl_grenadeindex, cl_gib1index, cl_gib2index, cl_gib3index;

qboolean CL_FilterModelindex(int modelindex, int frame)
{
	if (modelindex == cl_playerindex)
	{
		if (cl_deadbodyfilter.ival == 2)
		{
			if (frame >= 41 && frame <= 102)
				return true;
		}
		else if (cl_deadbodyfilter.ival)
		{
			if (frame == 49 || frame == 60 || frame == 69 || frame == 84 || frame == 93 || frame == 102)
				return true;
		}
	}

	if (cl_gibfilter.ival && (
			modelindex == cl_h_playerindex ||
			modelindex == cl_gib1index ||
			modelindex == cl_gib2index ||
			modelindex == cl_gib3index))
		return true;
	return false;
}


static void *AllocateBoneSpace(packet_entities_t *pack, unsigned char bonecount, unsigned int *allocationpos)
{
	size_t space = bonecount * sizeof(short)*7;
	void *r;
	if (pack->bonedatacur + space > pack->bonedatamax)
	{	//expand the storage as needed. messy, but whatever.
		pack->bonedatamax = pack->bonedatacur + space;
		pack->bonedata = BZ_Realloc(pack->bonedata, pack->bonedatamax);
	}
	r = pack->bonedata + pack->bonedatacur;
	*allocationpos = pack->bonedatacur;
	pack->bonedatacur += space;
	return r;
}
void *GetBoneSpace(packet_entities_t *pack, unsigned int allocationpos)
{
	if (allocationpos >= pack->bonedatacur)
		return NULL;
	return pack->bonedata + allocationpos;
}

//============================================================

void CL_FreeDlights(void)
{
#ifdef RTLIGHTS
	int i;
	if (cl_dlights)
		for (i = 0; i < rtlights_max; i++)
		{
			if (cl_dlights[i].customstyle)
				Z_Free(cl_dlights[i].customstyle);
			if (cl_dlights[i].worldshadowmesh)
				SH_FreeShadowMesh(cl_dlights[i].worldshadowmesh);

#ifdef GLQUAKE
			if (cl_dlights[i].coronaocclusionquery)
				qglDeleteQueriesARB(1, &cl_dlights[i].coronaocclusionquery);
#endif
		}
#endif

	rtlights_max = cl_maxdlights = 0;
	BZ_Free(cl_dlights);
	cl_dlights = NULL;
}
void CL_InitDlights(void)
{
	CL_FreeDlights();
	rtlights_max = cl_maxdlights = RTL_FIRST;
	cl_dlights = BZ_Realloc(cl_dlights, sizeof(*cl_dlights)*cl_maxdlights);
	memset(cl_dlights, 0, sizeof(*cl_dlights)*cl_maxdlights);
}

void CL_CloneDlight(dlight_t *dl, dlight_t *src)
{
	char *customstyle = dl->customstyle;
	void *sm = dl->worldshadowmesh;
	unsigned int oq = dl->coronaocclusionquery;
	unsigned int oqr = (dl->key == src->key)?dl->coronaocclusionresult:false;
	memcpy (dl, src, sizeof(*dl));
	dl->coronaocclusionquery = oq;
	dl->coronaocclusionresult = oqr;
	dl->rebuildcache = true;
	dl->worldshadowmesh = sm;
	dl->customstyle = src->customstyle?Z_StrDup(src->customstyle):NULL;
	Z_Free(customstyle);
}
static void CL_ClearDlight(dlight_t *dl, int key, qboolean reused)
{
	void *sm = dl->worldshadowmesh;
	unsigned int oq = dl->coronaocclusionquery;
	unsigned int oqr = reused?dl->coronaocclusionresult:false;
	Z_Free(dl->customstyle);
	memset (dl, 0, sizeof(*dl));
	dl->coronaocclusionquery = oq;
	dl->coronaocclusionresult = oqr;
	dl->rebuildcache = true;
	dl->worldshadowmesh = sm;
	dl->axis[0][0] = 1;
	dl->axis[1][1] = 1;
	dl->axis[2][2] = 1;
	dl->key = key;
	dl->flags = LFLAG_DYNAMIC;
	dl->color[0] = 1;
	dl->color[1] = 1;
	dl->color[2] = 1;
	dl->corona = bound(0, 1 * 0.25, 1);
	dl->coronascale = bound(0, r_flashblendscale.value, 1);
	dl->style = -1;
#ifdef RTLIGHTS
	dl->lightcolourscales[0] = r_shadow_realtime_dlight_ambient.value;
	dl->lightcolourscales[1] = r_shadow_realtime_dlight_diffuse.value;
	dl->lightcolourscales[2] = r_shadow_realtime_dlight_specular.value;
#endif
//	if (r_shadow_realtime_dlight_shadowmap.value)
//		dl->flags |= LFLAG_SHADOWMAP;
}

dlight_t *CL_AllocSlight(void)
{
	dlight_t	*dl;
	int i;
	for (i = RTL_FIRST; i < rtlights_max; i++)
	{
		if (cl_dlights[i].radius <= 0)
			break;
	}
	if (i == rtlights_max)
	{
		if (rtlights_max == cl_maxdlights)
		{
			cl_maxdlights = rtlights_max+8;
			cl_dlights = BZ_Realloc(cl_dlights, sizeof(*cl_dlights)*cl_maxdlights);
			memset(&cl_dlights[rtlights_max], 0, sizeof(*cl_dlights)*(cl_maxdlights-rtlights_max));
		}
		i = rtlights_max++;
	}
	dl = &cl_dlights[i];

	CL_ClearDlight(dl, 0, false);
	dl->flags = LFLAG_REALTIMEMODE;
	dl->corona = 0;
	return dl;
}

/*
===============
CL_AllocDlight

===============
*/
dlight_t *CL_AllocDlight (int key)
{
	int		i;
	dlight_t	*dl;

// first look for an exact key match
	if (key)
	{
		dl = cl_dlights+rtlights_first;
		for (i=rtlights_first ; i<RTL_FIRST ; i++, dl++)
		{
			if (dl->key == key)
			{
				CL_ClearDlight(dl, key, true);
				return dl;
			}
		}
	}

	//default to the first
	dl = &cl_dlights[rtlights_first?rtlights_first-1:0];
	//try and find one that is free
	for (i=RTL_FIRST; i > rtlights_first && i > 0; )
	{
		i--;
		if (!cl_dlights[i].radius)
		{
			dl = &cl_dlights[i];
			break;
		}
	}
	if (rtlights_first > dl - cl_dlights)
		rtlights_first = dl - cl_dlights;

	CL_ClearDlight(dl, key, false);
	return dl;
}

dlight_t *CL_AllocDlightOrg (int keyidx, vec3_t keyorg)
{
	int		i;
	dlight_t	*dl;

// first look for an exact key match
	dl = cl_dlights+rtlights_first;
	for (i=rtlights_first ; i<RTL_FIRST ; i++, dl++)
	{
		if (dl->key == keyidx && VectorCompare(dl->origin, keyorg))
		{
			CL_ClearDlight(dl, keyidx, true);
			VectorCopy(keyorg, dl->origin);
			return dl;
		}
	}

	//default to the first
	dl = &cl_dlights[rtlights_first?rtlights_first-1:0];
	//try and find one that is free
	for (i=RTL_FIRST; i > rtlights_first && i > 0; )
	{
		i--;
		if (!cl_dlights[i].radius)
		{
			dl = &cl_dlights[i];
			break;
		}
	}
	if (rtlights_first > dl - cl_dlights)
		rtlights_first = dl - cl_dlights;

	CL_ClearDlight(dl, keyidx, false);
	VectorCopy(keyorg, dl->origin);
	return dl;
}


/*
===============
CL_NewDlight
===============
*/
dlight_t *CL_NewDlight (int key, const vec3_t org, float radius, float time,
				   float r, float g, float b)
{
	dlight_t	*dl;

	dl = CL_AllocDlight (key);
	VectorCopy(org, dl->origin);
	dl->radius = radius;
	dl->die = cl.time + time;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;

	return dl;
}


/*
===============
CL_DecayLights

===============
*/
void CL_DecayLights (void)
{
	int			i;
	dlight_t	*dl;
	float frametime = host_frametime;

	if (cl.paused)	//DON'T DO IT!!!
		frametime = 0;

	dl = cl_dlights+rtlights_first;
	for (i=rtlights_first ; i<RTL_FIRST ; i++, dl++)
	{
		if (!dl->radius)
		{
			continue;
		}

		if (!dl->die)
		{
			continue;
		}

		if (dl->die < (float)cl.time)
		{
			if (i==rtlights_first)
				rtlights_first++;
			dl->radius = 0;
			continue;
		}

		if (r_dynamic.ival == 2)
		{	//don't decay quite so fast, this should aproximate winquake a bit better.
			dl->die -= frametime * 0.5;
			dl->radius -= frametime*dl->decay * 0.5;
		}
		else
			dl->radius -= frametime*dl->decay;
		if (dl->radius < 0)
		{
			if (i==rtlights_first)
				rtlights_first++;
			dl->radius = 0;
			continue;
		}

		if (dl->channelfade[0])
		{
			dl->color[0] -= frametime*dl->channelfade[0];
			if (dl->color[0] < 0)
				dl->color[0] = 0;
		}

		if (dl->channelfade[1])
		{
			dl->color[1] -= frametime*dl->channelfade[1];
			if (dl->color[1] < 0)
				dl->color[1] = 0;
		}

		if (dl->channelfade[2])
		{
			dl->color[2] -= frametime*dl->channelfade[2];
			if (dl->color[2] < 0)
				dl->color[2] = 0;
		}
	}
}


/*
=========================================================================

PACKET ENTITY PARSING / LINKING

=========================================================================
*/

/*
==================
CL_ParseDelta

Can go from either a baseline or a previous packet_entity
==================
*/
//int	bitcounts[32];	/// just for protocol profiling
void CLQW_ParseDelta (entity_state_t *from, entity_state_t *to, int bits)
{
	int			i;
#ifdef PROTOCOLEXTENSIONS
	int morebits=0;
#endif

	// set everything to the state we are delta'ing from
	*to = *from;

	to->number = bits & 511;
	to->sequence = cls.netchan.incoming_sequence;
	bits &= ~511;

	if (bits & U_MOREBITS)
	{	// read in the low order bits
		i = MSG_ReadByte ();
		bits |= i;
	}

	// count the bits for net profiling
//	for (i=0 ; i<16 ; i++)
//		if (bits&(1<<i))
//			bitcounts[i]++;

#ifdef PROTOCOLEXTENSIONS
	if ((bits & U_EVENMORE) && (cls.fteprotocolextensions & (PEXT_SCALE|PEXT_TRANS|PEXT_FATNESS|PEXT_HEXEN2|PEXT_COLOURMOD|PEXT_DPFLAGS|PEXT_MODELDBL|PEXT_ENTITYDBL|PEXT_ENTITYDBL2)))
		morebits = MSG_ReadByte ();
	if (morebits & U_YETMORE)
		morebits |= MSG_ReadByte()<<8;
#endif

	if ((morebits & U_ENTITYDBL) && (cls.fteprotocolextensions & PEXT_ENTITYDBL))
		to->number += 512;
	if ((morebits & U_ENTITYDBL2) && (cls.fteprotocolextensions & PEXT_ENTITYDBL2))
		to->number += 1024;

	if (bits & U_MODEL)
	{
		to->modelindex = MSG_ReadByte ();
		if (morebits & U_MODELDBL && (cls.fteprotocolextensions & PEXT_MODELDBL))
			to->modelindex += 256;
	}
	else if (morebits & U_MODELDBL && (cls.fteprotocolextensions & PEXT_MODELDBL))
		to->modelindex = MSG_ReadShort();

	if (bits & U_FRAME)
		to->frame = MSG_ReadByte ();

	if (bits & U_COLORMAP)
		to->colormap = MSG_ReadByte();

	if (bits & U_SKIN)
	{
		to->skinnum = MSG_ReadByte();
		if (to->skinnum >= 256-32) /*final 32 skins are taken as a content value instead*/
			to->skinnum = (char)to->skinnum;
	}

	if (bits & U_EFFECTS)
		to->effects = (to->effects&0xff00)|MSG_ReadByte();

	if (bits & U_ORIGIN1)
	{
		if (cls.ezprotocolextensions1 & EZPEXT1_FLOATENTCOORDS)
			to->origin[0] = MSG_ReadCoordFloat ();
		else
			to->origin[0] = MSG_ReadCoord ();
	}

	if (bits & U_ANGLE1)
		to->angles[0] = MSG_ReadAngle ();

	if (bits & U_ORIGIN2)
	{
		if (cls.ezprotocolextensions1 & EZPEXT1_FLOATENTCOORDS)
			to->origin[1] = MSG_ReadCoordFloat ();
		else
			to->origin[1] = MSG_ReadCoord ();
	}

	if (bits & U_ANGLE2)
		to->angles[1] = MSG_ReadAngle ();

	if (bits & U_ORIGIN3)
	{
		if (cls.ezprotocolextensions1 & EZPEXT1_FLOATENTCOORDS)
			to->origin[2] = MSG_ReadCoordFloat ();
		else
			to->origin[2] = MSG_ReadCoord ();
	}

	if (bits & U_ANGLE3)
		to->angles[2] = MSG_ReadAngle ();

	to->solidsize = ES_SOLID_BSP;
	if (bits & U_SOLID)
	{
		//doesn't mean anything in vanilla. solidity is infered instead.

	}

#ifdef PEXT_SCALE
	if ((morebits & U_SCALE) && (cls.fteprotocolextensions & PEXT_SCALE))
		to->scale = MSG_ReadByte();
#endif
#ifdef PEXT_TRANS
	if ((morebits & U_TRANS) && (cls.fteprotocolextensions & PEXT_TRANS))
		to->trans = MSG_ReadByte();
#endif
#ifdef PEXT_FATNESS
	if ((morebits & U_FATNESS) && (cls.fteprotocolextensions & PEXT_FATNESS))
		to->fatness = MSG_ReadChar();
#endif

	if ((morebits & U_DRAWFLAGS) && (cls.fteprotocolextensions & PEXT_HEXEN2))
		to->hexen2flags = MSG_ReadByte();
	if ((morebits & U_ABSLIGHT) && (cls.fteprotocolextensions & PEXT_HEXEN2))
		to->abslight = MSG_ReadByte();

	if ((morebits & U_COLOURMOD) && (cls.fteprotocolextensions & PEXT_COLOURMOD))
	{
		to->colormod[0] = MSG_ReadByte();
		to->colormod[1] = MSG_ReadByte();
		to->colormod[2] = MSG_ReadByte();
	}

	if (morebits & U_DPFLAGS)// && cls.fteprotocolextensions & PEXT_DPFLAGS)
	{
		// these are bits for the 'flags' field of the entity_state_t

		i = MSG_ReadByte();
		to->dpflags = i;
	}
	if (!(cls.fteprotocolextensions & PEXT_DPFLAGS))
	{
		if (to->frame)
			to->dpflags |= RENDER_STEP;
	}
	if (morebits & U_TAGINFO)
	{
		to->tagentity = MSG_ReadShort();
		to->tagindex = MSG_ReadShort();
	}
	if (morebits & U_LIGHT)
	{
		to->light[0] = MSG_ReadShort();
		to->light[1] = MSG_ReadShort();
		to->light[2] = MSG_ReadShort();
		to->light[3] = MSG_ReadShort();
		to->lightstyle = MSG_ReadByte();
		to->lightpflags = MSG_ReadByte();
	}

	if (morebits & U_EFFECTS16)
		to->effects = (to->effects&0x00ff)|(MSG_ReadByte()<<8);
}


/*
=================
FlushEntityPacket
=================
*/
void FlushEntityPacket (void)
{
	int			word;
	entity_state_t	olde, newe;

	Con_DPrintf ("FlushEntityPacket\n");

	memset (&olde, 0, sizeof(olde));

	if ((cl.validsequence&UPDATE_MASK) == (cls.netchan.incoming_sequence&UPDATE_MASK))
		cl.validsequence = 0;		// last-known-good sequence is becoming invalid.
	cl.inframes[cls.netchan.incoming_sequence&UPDATE_MASK].invalid = true;

	// read it all, but ignore it
	while (1)
	{
		word = (unsigned short)MSG_ReadShort ();
		if (msg_badread)
		{	// something didn't parse right...
			Host_EndGame ("msg_badread in packetentities");
			return;
		}

		if (!word)
			break;	// done

		CLQW_ParseDelta (&olde, &newe, word);
	}
}

void CLFTE_ReadDelta(unsigned int entnum, entity_state_t *news, entity_state_t *olds, entity_state_t *baseline, packet_entities_t *newp, packet_entities_t *oldp)
{
	unsigned int predbits = 0;
	unsigned int bits;

	bits = MSG_ReadByte();
	if (bits & UF_EXTEND1)
		bits |= MSG_ReadByte()<<8;
	if (bits & UF_EXTEND2)
		bits |= MSG_ReadByte()<<16;
	if (bits & UF_EXTEND3)
		bits |= MSG_ReadByte()<<24;
	if (bits & UF_EXTEND4)
		Host_EndGame("ent update bit %#x\n", UF_EXTEND4);

	if (cl_shownet.ival >= 3)
		Con_Printf("%3i:     Update %4i 0x%x\n", MSG_GetReadCount(), entnum, bits);

	if (bits & UF_RESET)
	{
//		Con_Printf("%3i: Reset %i @ %i\n", msg_readcount, entnum, cls.netchan.incoming_sequence);
		*news = *baseline;
	}
	else if (!olds)
	{
		/*reset got lost, probably the data will be filled in later - FIXME: we should probably ignore this entity*/
		Con_DPrintf("New entity %i without reset\n", entnum);
		*news = nullentitystate;
//		*news = *baseline;
	}
	else
		*news = *olds;
	news->number = entnum;
	news->sequence = cls.netchan.incoming_sequence;
	
	if (bits & UF_FRAME)
	{
		if (cls.fteprotocolextensions2 & PEXT2_LERPTIME)
			news->frame = MSG_ReadULEB128();
		else
		{
			if (bits & UF_16BIT_LERPTIME)
				news->frame = MSG_ReadShort();
			else
				news->frame = MSG_ReadByte();
		}
	}

	if (cls.ezprotocolextensions1 & EZPEXT1_FLOATENTCOORDS)
	{
		if (bits & UF_ORIGINXY)
		{
			news->origin[0] = MSG_ReadFloat();
			news->origin[1] = MSG_ReadFloat();
		}
		if (bits & UF_ORIGINZ)
			news->origin[2] = MSG_ReadFloat();
	}
	else
	{
		if (bits & UF_ORIGINXY)
		{
			news->origin[0] = MSG_ReadCoord();
			news->origin[1] = MSG_ReadCoord();
		}
		if (bits & UF_ORIGINZ)
			news->origin[2] = MSG_ReadCoord();
	}

	if ((bits & UF_PREDINFO) && !(cls.fteprotocolextensions2 & PEXT2_PREDINFO))
	{
		/*predicted stuff gets more precise angles*/
		if (bits & UF_ANGLESXZ)
		{
			news->angles[0] = MSG_ReadAngle16();
			news->angles[2] = MSG_ReadAngle16();
		}
		if (bits & UF_ANGLESY)
			news->angles[1] = MSG_ReadAngle16();
	}
	else
	{
		if (bits & UF_ANGLESXZ)
		{
			news->angles[0] = MSG_ReadAngle();
			news->angles[2] = MSG_ReadAngle();
		}
		if (bits & UF_ANGLESY)
			news->angles[1] = MSG_ReadAngle();
	}

	if (cls.fteprotocolextensions2 & PEXT2_LERPTIME)
	{
		if (bits & UF_16BIT_LERPTIME)
			news->lerpend = cl.gametime + MSG_ReadULEB128()*(1/1000.0);	//most things will animate at 100ms, so this will usually fit a single byte, without capping out.
		if (bits & UF_EFFECTS)
			news->effects = MSG_ReadULEB128();
		if (bits & UF_EFFECTS2_OLD)
			Host_EndGame("Received unexpected (redefined) bit %#x\n", UF_EFFECTS2_OLD);
	}
	else
	{
		if ((bits & (UF_EFFECTS | UF_EFFECTS2_OLD)) == (UF_EFFECTS | UF_EFFECTS2_OLD))
			news->effects = MSG_ReadLong();
		else if (bits & UF_EFFECTS2_OLD)
			news->effects = (unsigned short)MSG_ReadShort();
		else if (bits & UF_EFFECTS)
			news->effects = MSG_ReadByte();
	}

	news->u.q1.movement[0] = 0;
	news->u.q1.movement[1] = 0;
	news->u.q1.movement[2] = 0;
	news->u.q1.velocity[0] = 0;
	news->u.q1.velocity[1] = 0;
	news->u.q1.velocity[2] = 0;
	if (bits & UF_PREDINFO)
	{
		predbits = MSG_ReadByte();

		if (predbits & UFP_FORWARD)
			news->u.q1.movement[0] = MSG_ReadShort();
		else
			news->u.q1.movement[0] = 0;
		if (predbits & UFP_SIDE)
			news->u.q1.movement[1] = MSG_ReadShort();
		else
			news->u.q1.movement[1] = 0;
		if (predbits & UFP_UP)
			news->u.q1.movement[2] = MSG_ReadShort();
		else
			news->u.q1.movement[2] = 0;
		if (predbits & UFP_MOVETYPE)
			news->u.q1.pmovetype = MSG_ReadByte();
		if (predbits & UFP_VELOCITYXY)
		{
			news->u.q1.velocity[0] = MSG_ReadShort();
			news->u.q1.velocity[1] = MSG_ReadShort();
		}
		else
		{
			news->u.q1.velocity[0] = 0;
			news->u.q1.velocity[1] = 0;
		}
		if (predbits & UFP_VELOCITYZ)
			news->u.q1.velocity[2] = MSG_ReadShort();
		else
			news->u.q1.velocity[2] = 0;
		if (predbits & UFP_MSEC)
			news->u.q1.msec = MSG_ReadByte();
		else
			news->u.q1.msec = 0;

		if (cls.fteprotocolextensions2 & PEXT2_PREDINFO)
		{
			if (predbits & UFP_VIEWANGLE)
			{
				if (bits & UF_ANGLESXZ)
				{
					news->u.q1.vangle[0] = MSG_ReadShort();
					news->u.q1.vangle[2] = MSG_ReadShort();
				}
				if (bits & UF_ANGLESY)
					news->u.q1.vangle[1] = MSG_ReadShort();
			}
		}
		else
		{
			if (predbits & UFP_WEAPONFRAME_OLD)
			{
				news->u.q1.weaponframe = MSG_ReadByte();
				if (news->u.q1.weaponframe & 0x80)
					news->u.q1.weaponframe = (news->u.q1.weaponframe & 127) | (MSG_ReadByte()<<7);
			}
		}
	}
	else
	{
		news->u.q1.msec = 0;
	}

	if (!(predbits & UFP_VIEWANGLE) || !(cls.fteprotocolextensions2 & PEXT2_PREDINFO))
	{
		if (bits & UF_ANGLESXZ)
			news->u.q1.vangle[0] = ANGLE2SHORT(news->angles[0] * ((bits & UF_PREDINFO)?-3:-1));
		if (bits & UF_ANGLESY)
			news->u.q1.vangle[1] = ANGLE2SHORT(news->angles[1]);
		if (bits & UF_ANGLESXZ)
			news->u.q1.vangle[2] = ANGLE2SHORT(news->angles[2]);
	}

	if (bits & UF_MODEL)
	{
		if (cls.fteprotocolextensions2 & PEXT2_LERPTIME)
			news->modelindex = MSG_ReadULEB128();
		else
		{
			if (bits & UF_16BIT_LERPTIME)
				news->modelindex = MSG_ReadShort();
			else
				news->modelindex = MSG_ReadByte();
		}
	}
	if (bits & UF_SKIN)
	{
		if (cls.fteprotocolextensions2 & PEXT2_LERPTIME)
			news->skinnum = MSG_ReadULEB128()-64;	//biased for content overrides
		else
		{
			if (bits & UF_16BIT_LERPTIME)
				news->skinnum = MSG_ReadShort();
			else
				news->skinnum = MSG_ReadByte();
		}
	}
	if (bits & UF_COLORMAP)
	{
		if (cls.fteprotocolextensions2 & PEXT2_LERPTIME)
			news->colormap = MSG_ReadULEB128();
		else
			news->colormap = MSG_ReadByte();
	}

	if (bits & UF_SOLID)
	{
		if (cls.fteprotocolextensions2 & PEXT2_NEWSIZEENCODING)
		{
			qbyte enc = MSG_ReadByte();
			if (enc == 0)
				news->solidsize = ES_SOLID_NOT;
			else if (enc == 1)
				news->solidsize = ES_SOLID_BSP;
			else if (enc == 2)
				news->solidsize = ES_SOLID_HULL1;
			else if (enc == 3)
				news->solidsize = ES_SOLID_HULL2;
			else if (enc == 16)
				news->solidsize = MSG_ReadSize16(&net_message);
			else if (enc == 32)
				news->solidsize = MSG_ReadLong();
			else
				Sys_Error("Solid+Size encoding not known");
		}
		else
			news->solidsize = MSG_ReadSize16(&net_message);
	}

	if (bits & UF_FLAGS)
	{
		if (cls.fteprotocolextensions2 & PEXT2_LERPTIME)
			news->dpflags = MSG_ReadULEB128();
		else
			news->dpflags = MSG_ReadByte();
	}

	if (bits & UF_ALPHA)
		news->trans = MSG_ReadByte();
	if (bits & UF_SCALE)
		news->scale = MSG_ReadByte();
	if (bits & UF_BONEDATA)
	{
		unsigned char fl = MSG_ReadByte();
		if (fl & 0x80)
		{
			//this is NOT finalized
			short *bonedata;
			int i;
			news->bonecount = MSG_ReadByte();
			bonedata = AllocateBoneSpace(newp, news->bonecount, &news->boneoffset);
			for (i = 0; i < news->bonecount*7; i++)
				bonedata[i] = MSG_ReadShort();
		}
		else
			news->bonecount = 0;	//oo, it went away.
		if (fl & 0x40)
		{
			if (cls.fteprotocolextensions2 & PEXT2_LERPTIME)
			{
				news->basebone = MSG_ReadULEB128();
				news->baseframe = MSG_ReadULEB128();
			}
			else
			{
				news->basebone = MSG_ReadByte();
				news->baseframe = MSG_ReadShort();
			}
		}
		else
		{
			news->basebone = 0;
			news->baseframe = 0;
		}

		//fixme: basebone, baseframe, etc.
		if (fl & 0x3f)
			Host_EndGame("unsupported entity delta info\n");
	}
	else if (news->bonecount)
	{	//still has bone data from the previous frame.
		short *bonedata = AllocateBoneSpace(newp, news->bonecount, &news->boneoffset);
		memcpy(bonedata, oldp->bonedata+olds->boneoffset, sizeof(short)*7*news->bonecount);
	}

	if (bits & UF_DRAWFLAGS)
	{
		news->hexen2flags = MSG_ReadByte();
		if ((news->hexen2flags & MLS_MASK) >= MLS_ADDLIGHT)
			news->abslight = MSG_ReadByte();
		else
			news->abslight = 0;
	}
	if (bits & UF_TAGINFO)
	{
		news->tagentity = MSGCL_ReadEntity();
		if (cls.fteprotocolextensions2 & PEXT2_LERPTIME)
			news->tagindex = MSG_ReadULEB128()-1;	//biased for q3-like portals.
		else
		{
			news->tagindex = MSG_ReadByte();
			if (news->tagindex == 0xff)
				news->tagindex = ~0;
		}
	}
	if (bits & UF_LIGHT)
	{
		news->light[0] = MSG_ReadShort();
		news->light[1] = MSG_ReadShort();
		news->light[2] = MSG_ReadShort();
		news->light[3] = MSG_ReadShort();
		if (cls.fteprotocolextensions2 & PEXT2_LERPTIME)
			news->lightstyle = MSG_ReadULEB128();
		else
			news->lightstyle = MSG_ReadByte();
		news->lightpflags = MSG_ReadByte();
	}
	if (bits & UF_TRAILEFFECT)
	{
		if (cls.fteprotocolextensions2 & PEXT2_LERPTIME)
		{
			news->u.q1.traileffectnum = MSG_ReadULEB128();
			news->u.q1.emiteffectnum = MSG_ReadULEB128();
		}
		else
		{
			unsigned short s;
			s = MSG_ReadShort();
			news->u.q1.traileffectnum = s & 0x3fff;
			if (s & 0x8000)
				news->u.q1.emiteffectnum = MSG_ReadShort() & 0x3fff;
			else
				news->u.q1.emiteffectnum = 0;
		}

		if (news->u.q1.traileffectnum >= countof(cl.particle_ssprecache))
			news->u.q1.traileffectnum = 0;
		if (news->u.q1.emiteffectnum >= countof(cl.particle_ssprecache))
			news->u.q1.emiteffectnum = 0;
	}

	if (bits & UF_COLORMOD)
	{
		news->colormod[0] = MSG_ReadByte();
		news->colormod[1] = MSG_ReadByte();
		news->colormod[2] = MSG_ReadByte();
	}
	if (bits & UF_GLOW)
	{
		news->glowsize = MSG_ReadByte();
		news->glowcolour = MSG_ReadByte();
		news->glowmod[0] = MSG_ReadByte();
		news->glowmod[1] = MSG_ReadByte();
		news->glowmod[2] = MSG_ReadByte();
	}
	if (bits & UF_FATNESS)
		news->fatness = MSG_ReadByte();
	if (bits & UF_MODELINDEX2)
	{
		if (cls.fteprotocolextensions2 & PEXT2_LERPTIME)
			news->modelindex2 = MSG_ReadULEB128();
		else
		{
			if (bits & UF_16BIT_LERPTIME)
				news->modelindex2 = MSG_ReadShort();
			else
				news->modelindex2 = MSG_ReadByte();
		}
	}
	if (bits & UF_GRAVITYDIR)
	{
		news->u.q1.gravitydir[0] = MSG_ReadByte();
		news->u.q1.gravitydir[1] = MSG_ReadByte();
	}
	if (bits & UF_UNUSED1)
	{
		Host_EndGame("ent update bit %#x\n", UF_UNUSED1);
	}
}

void CLFTE_ParseBaseline(entity_state_t *es, qboolean numberisimportant)
{
	int entnum = 0;
	if (numberisimportant)
		entnum = MSGCL_ReadEntity();
	CLFTE_ReadDelta(entnum, es, &nullentitystate, &nullentitystate, NULL, NULL);
}


void CL_PredictEntityMovement(entity_state_t *estate, float age);

/*
Note: strictly speaking, you don't need multiple frames, just two and flip between them.
FTE retains the full 64 frames because its interpolation will go multiple packets back in time to cover packet loss.
*/
void CLFTE_ParseEntities(void)
{
	int			oldpacket, newpacket;
	packet_entities_t	*oldp, *newp, nullp;
	entity_state_t *news, *olds;
	unsigned int newnum, oldnum;
	int			oldindex;
	qboolean	isvalid = false;
	qboolean removeflag;
	int inputframe = cls.netchan.incoming_sequence;
#if defined(QUAKESTATS) || defined(NQPROT)
	int i;
#endif

//	int i;
//	for (i = cl.validsequence+1; i < cls.netchan.incoming_sequence; i++)
//	{
//		Con_Printf("CL: Dropped %i\n", i);
//	}

	if (cls.demoplayback == DPB_MVD)
	{
		cls.netchan.incoming_sequence++;
		cls.netchan.incoming_acknowledged++;
	}
#ifdef NQPROT
	else if (cls.protocol == CP_NETQUAKE)
	{
		cls.netchan.incoming_sequence++;
		cl.last_servermessage = realtime;
		if (cls.fteprotocolextensions2 & PEXT2_PREDINFO)
		{
			inputframe = (unsigned short)MSG_ReadShort();
			inputframe = (cl.movesequence&0xffff0000) | inputframe;
			if (inputframe > cl.movesequence)
				inputframe -= 0x00010000;	//err, if its in the future then cl.movesequence must have wrapped.
		}
		else
			inputframe = cl.movesequence;

		if (cl.numackframes == sizeof(cl.ackframes)/sizeof(cl.ackframes[0]))
			cl.numackframes--;
		if (!cl.validsequence)
			cl.ackframes[cl.numackframes++] = -1;
		else
			cl.ackframes[cl.numackframes++] = cls.netchan.incoming_unreliable;

		{
			extern vec3_t demoangles;
			int fr = cls.netchan.incoming_sequence&UPDATE_MASK;
			for (i = 0; i < MAX_SPLITS; i++)
				cl.inframes[fr&UPDATE_MASK].packet_entities.fixangles[i] = false;
			if (cls.demoplayback)
			{
				cl.inframes[fr&UPDATE_MASK].packet_entities.fixangles[0] = 2;
				VectorCopy(demoangles, cl.inframes[fr&UPDATE_MASK].packet_entities.fixedangles[0]);
			}
		}

//		if (cl.validsequence != cls.netchan.incoming_sequence-1)
//			Con_Printf("CLIENT: Dropped a frame\n");
	}
#endif

	newpacket = cls.netchan.incoming_sequence&UPDATE_MASK;
	oldpacket = cl.validsequence&UPDATE_MASK;
	newp = &cl.inframes[newpacket].packet_entities;
	oldp = &cl.inframes[oldpacket].packet_entities;
	cl.inframes[newpacket].invalid = true;
	cl.inframes[newpacket].receivedtime = realtime;
	cl.inframes[newpacket].frameid = cls.netchan.incoming_sequence;

#ifdef QUAKESTATS
	for (i = 0; i < cl.splitclients; i++)
	{
		cl.inframes[newpacket&UPDATE_MASK].packet_entities.punchangle[i][0] = cl.playerview[i].statsf[STAT_PUNCHANGLE_X];
		cl.inframes[newpacket&UPDATE_MASK].packet_entities.punchangle[i][1] = cl.playerview[i].statsf[STAT_PUNCHANGLE_Y];
		cl.inframes[newpacket&UPDATE_MASK].packet_entities.punchangle[i][2] = cl.playerview[i].statsf[STAT_PUNCHANGLE_Z];
		cl.inframes[newpacket&UPDATE_MASK].packet_entities.punchorigin[i][0] = cl.playerview[i].statsf[STAT_PUNCHVECTOR_X];
		cl.inframes[newpacket&UPDATE_MASK].packet_entities.punchorigin[i][1] = cl.playerview[i].statsf[STAT_PUNCHVECTOR_Y];
		cl.inframes[newpacket&UPDATE_MASK].packet_entities.punchorigin[i][2] = cl.playerview[i].statsf[STAT_PUNCHVECTOR_Z];
	}
#endif


	if (!cl.validsequence || cls.netchan.incoming_sequence-cl.validsequence >= UPDATE_BACKUP-1 || oldp == newp)
	{
		//yes, this results in a load of invalid packets for a while.
		//server is meant to notice and send a reset packet, which causes it to become valid again
		oldp = &nullp;
		oldp->num_entities = 0;
		oldp->max_entities = 0;
	}
	else
		isvalid = true;

	newp->servertime = MSG_ReadFloat();

	if (cl.gametime != newp->servertime)
	{
		cl.oldgametime = cl.gametime;
		cl.oldgametimemark = cl.gametimemark;
		cl.gametime = newp->servertime;
		cl.gametimemark = realtime;
	}

	/*clear all entities*/
	newp->num_entities = 0;
	newp->bonedatacur = 0;
	oldindex = 0;
	while(1)
	{
		//high bit means remove, second high bit means 22bit index
		newnum = (unsigned short)(short)MSG_ReadShort();
		removeflag = !!(newnum & 0x8000);
		if (newnum & 0x4000)
			newnum = (newnum & 0x3fff) | (MSG_ReadByte()<<14);
		else
			newnum &= ~0x8000;

		if ((!newnum && !removeflag) || msg_badread)
		{
			/*reached the end, don't forget old entities*/
			while(oldindex < oldp->num_entities)
			{
				if (newp->num_entities >= newp->max_entities)
				{
					newp->max_entities = newp->num_entities+1;
					newp->entities = BZ_Realloc(newp->entities, sizeof(entity_state_t)*newp->max_entities);
				}

				//copy it over
				news = &newp->entities[newp->num_entities++];
				olds = &oldp->entities[oldindex++];
				*news = *olds;
				if (news->bonecount)
				{	//still has bone data somehow.
					short *bonedata = AllocateBoneSpace(newp, news->bonecount, &news->boneoffset);
					memcpy(bonedata, oldp->bonedata+olds->boneoffset, sizeof(short)*7*news->bonecount);
				}
			}
			break;
		}

		oldnum = (oldindex >= oldp->num_entities) ? 0xffffffff : oldp->entities[oldindex].number;

		/*if we skipped some, then they were unchanged*/
		while (newnum > oldnum)
		{
			if (newp->num_entities >= newp->max_entities)
			{
				newp->max_entities = newp->num_entities+1;
				newp->entities = BZ_Realloc(newp->entities, sizeof(entity_state_t)*newp->max_entities);
			}

			//copy it over
			news = &newp->entities[newp->num_entities++];
			olds = &oldp->entities[oldindex++];
			*news = *olds;
			if (news->bonecount)
			{	//still has bone data somehow.
				short *bonedata = AllocateBoneSpace(newp, news->bonecount, &news->boneoffset);
				memcpy(bonedata, oldp->bonedata+olds->boneoffset, sizeof(short)*7*news->bonecount);
			}

			oldnum = (oldindex >= oldp->num_entities) ? 0xffffffff : oldp->entities[oldindex].number;
		}

		if (removeflag)
		{
			if (cl_shownet.ival >= 3)
				Con_Printf("%3i:     Remove %i @ %i\n", MSG_GetReadCount(), newnum, cls.netchan.incoming_sequence);

			if (!newnum)
			{
				/*removal of world - means forget all entities*/
				if (cl_shownet.ival >= 3)
					Con_Printf("%3i:     Reset all\n", MSG_GetReadCount());
				newp->num_entities = 0;
				oldp = &nullp;
				oldp->num_entities = 0;
				oldp->max_entities = 0;
				isvalid = true;

				cls.demohadkeyframe = true;	//we can reactivate deltas when recording now.
				continue;
			}

			if (oldnum == newnum)
				oldindex++;
			continue;
		}
		else
		{
			if (!CL_CheckBaselines(newnum))
				Host_EndGame("CL_ParsePacketEntities: check baselines failed with size %i", newnum);

			if (newp->num_entities >= newp->max_entities)
			{
				newp->max_entities = newp->num_entities+1;
				newp->entities = BZ_Realloc(newp->entities, sizeof(entity_state_t)*newp->max_entities);
			}

			if (oldnum == newnum)
				CLFTE_ReadDelta(newnum, &newp->entities[newp->num_entities++], &oldp->entities[oldindex++], cl_baselines + newnum, newp, oldp);
			else
				CLFTE_ReadDelta(newnum, &newp->entities[newp->num_entities++], NULL, cl_baselines + newnum, newp, NULL);
		}
	}

	if (cl.do_lerp_players)
	{
		float packetage = (realtime - cl.outframes[cl.ackedmovesequence & UPDATE_MASK].senttime) - cls.latency*cl_predict_players_latency.value + cl_predict_players_nudge.value;
		//predict in-place based upon calculated latencies and stuff, stuff can then be interpolated properly
		for (oldindex = 0; oldindex < newp->num_entities; oldindex++)
		{
			CL_PredictEntityMovement(newp->entities + oldindex, (newp->entities[oldindex].u.q1.msec / 1000.0f + packetage) *0.5);
		}
	}

	if (isvalid)
	{
		cl.oldvalidsequence = cl.validsequence;
		cl.validsequence = cls.netchan.incoming_sequence;
		CL_AckedInputFrame(cls.netchan.incoming_sequence, inputframe, true);
		cl.inframes[newpacket].invalid = false;
	}
	else
	{
		newp->num_entities = 0;
		cl.validsequence = 0;

		CL_AckedInputFrame(cls.netchan.incoming_sequence, inputframe, false);
	}
}

/*
==================
CL_ParsePacketEntities

An svc_packetentities has just been parsed, deal with the
rest of the data stream.
==================
*/
void CLQW_ParsePacketEntities (qboolean delta)
{
	int			oldpacket, newpacket;
	packet_entities_t	*oldp, *newp, dummy;
	int			oldindex, newindex;
	int			word, newnum, oldnum;
	qboolean	full;
	int		from;

	newpacket = cls.netchan.incoming_sequence&UPDATE_MASK;
	newp = &cl.inframes[newpacket].packet_entities;
	cl.inframes[newpacket].invalid = false;
	cl.inframes[newpacket].frameid = cls.netchan.incoming_sequence;
	cl.inframes[newpacket].receivedtime = realtime;

	if (cls.protocol == CP_QUAKEWORLD && cls.demoplayback == DPB_MVD)
	{
		extern float olddemotime;	//time from the most recent demo packet
		cl.oldgametime = cl.gametime;
		cl.oldgametimemark = cl.gametimemark;
		cl.gametime = olddemotime + cl.demogametimebias;
		cl.gametimemark = realtime;
	}
	else if (!(cls.fteprotocolextensions & PEXT_ACCURATETIMINGS) && cls.protocol == CP_QUAKEWORLD)
	{
		extern cvar_t cl_demospeed;
		float scale = cls.demoplayback?cl_demospeed.value:1;
		cl.oldgametime = cl.gametime;
		cl.oldgametimemark = cl.gametimemark;
		if (realtime - cl.gametimemark > 0)
			cl.gametime += (realtime - cl.gametimemark)*scale;//cl.frames[newpacket].senttime - cl.frames[(newpacket-1)&UPDATE_MASK].senttime;
		cl.gametimemark = realtime;
	}

	newp->servertime = cl.gametime;

	if (delta)
	{
		from = MSG_ReadByte ();

//		Con_Printf("%i %i from %i\n", cls.netchan.outgoing_sequence, cls.netchan.incoming_sequence, from);
		if (cls.demoplayback == DPB_MVD)
			from = oldpacket = cls.netchan.incoming_sequence - 1;
		oldpacket = cl.inframes[from & UPDATE_MASK].frameid;

		if (cl.inframes[from&UPDATE_MASK].invalid ||	//old frame is unusable
			cls.netchan.outgoing_sequence - oldpacket >= UPDATE_BACKUP - 1)	// we must have lost the sequence its trying to delta from (or just too old).
		{
			FlushEntityPacket ();
			return;
		}

		oldp = &cl.inframes[from & UPDATE_MASK].packet_entities;
		full = false;
	}
	else
	{	// this is a full update that we can start delta compressing from now
		oldp = &dummy;
		dummy.num_entities = 0;
		full = true;
	}

	//FIXME
	cl.oldvalidsequence = cl.validsequence;
	cl.validsequence = cls.netchan.incoming_sequence;
	CL_AckedInputFrame(cls.netchan.incoming_sequence, cls.netchan.incoming_sequence, true);

	oldindex = 0;
	newindex = 0;
	newp->num_entities = 0;

	while (1)
	{
		word = (unsigned short)MSG_ReadShort ();
		if (msg_badread)
		{	// something didn't parse right...
			Host_EndGame ("msg_badread in packetentities");
			return;
		}

		if (!word)
		{
			while (oldindex < oldp->num_entities)
			{	// copy all the rest of the entities from the old packet
//Con_Printf ("copy %i\n", oldp->entities[oldindex].number);
				if (newindex >= newp->max_entities)
				{
					newp->max_entities = newindex+1;
					newp->entities = BZ_Realloc(newp->entities, sizeof(entity_state_t)*newp->max_entities);
				}
				if (oldindex >= oldp->max_entities)
					Host_EndGame("Old packet entity too big\n");
				newp->entities[newindex] = oldp->entities[oldindex];
				newindex++;
				oldindex++;
			}
			break;
		}
		newnum = word&511;

		if (word & U_MOREBITS)
		{
			int oldpos = MSG_GetReadCount();
			int excessive;
			excessive = MSG_ReadByte();
			if (excessive & U_EVENMORE)
			{
				excessive = MSG_ReadByte();
				if (excessive & U_ENTITYDBL)
					newnum += 512;
				if (excessive & U_ENTITYDBL2)
					newnum += 1024;
			}

			MSG_ReadSkip(oldpos-MSG_GetReadCount());//undo the read...
		}
		oldnum = oldindex >= oldp->num_entities ? 9999 : oldp->entities[oldindex].number;

		while (newnum > oldnum)
		{
			if (full)
			{
				Con_Printf ("WARNING: oldcopy on full update");
				FlushEntityPacket ();
				return;
			}

//Con_Printf ("copy %i\n", oldnum);
			// copy one of the old entities over to the new packet unchanged
			if (newindex >= newp->max_entities)
			{
				newp->max_entities = newindex+1;
				newp->entities = BZ_Realloc(newp->entities, sizeof(entity_state_t)*newp->max_entities);
			}
			if (oldindex >= oldp->max_entities)
				Host_EndGame("Old packet entity too big\n");
			newp->entities[newindex] = oldp->entities[oldindex];
			newindex++;
			oldindex++;
			oldnum = oldindex >= oldp->num_entities ? 9999 : oldp->entities[oldindex].number;
		}

		if (newnum < oldnum)
		{	// new from baseline
//Con_Printf ("baseline %i\n", newnum);
			if (word & U_REMOVE)
			{	//really read the extra entity number if required
				if (word & U_MOREBITS)
					if (MSG_ReadByte() & U_EVENMORE)
						MSG_ReadByte();

				if (full)
				{
					cl.validsequence = 0;
					Con_Printf ("WARNING: U_REMOVE on full update\n");
					FlushEntityPacket ();
					return;
				}
				continue;
			}
			if (newindex >= newp->max_entities)
			{
				newp->max_entities = newindex+1;
				newp->entities = BZ_Realloc(newp->entities, sizeof(entity_state_t)*newp->max_entities);
			}

			if (!CL_CheckBaselines(newnum))
				Host_EndGame("CL_ParsePacketEntities: check baselines failed with size %i", newnum);
			CLQW_ParseDelta (cl_baselines + newnum, &newp->entities[newindex], word);
			newindex++;
			continue;
		}

		if (newnum == oldnum)
		{	// delta from previous
			if (full)
			{
				cl.validsequence = 0;
				Con_Printf ("WARNING: delta on full update");
			}
			if (word & U_REMOVE)
			{
				if (word & U_MOREBITS)
					if (MSG_ReadByte() & U_EVENMORE)
						MSG_ReadByte();
				oldindex++;
				continue;
			}

			if (newindex >= newp->max_entities)
			{
				newp->max_entities = newindex+1;
				newp->entities = BZ_Realloc(newp->entities, sizeof(entity_state_t)*newp->max_entities);
			}

//Con_Printf ("delta %i\n",newnum);
			CLQW_ParseDelta (&oldp->entities[oldindex], &newp->entities[newindex], word);
			newindex++;
			oldindex++;
		}

	}

	newp->num_entities = newindex;
}


entity_state_t *CL_FindOldPacketEntity(int num)
{
	int					pnum;
	entity_state_t		*s1;
	packet_entities_t	*pack;
	if (!cl.validsequence)
		return NULL;
	pack = &cl.inframes[(cls.netchan.incoming_sequence-1)&UPDATE_MASK].packet_entities;

	for (pnum=0 ; pnum<pack->num_entities ; pnum++)
	{
		s1 = &pack->entities[pnum];

		if (num == s1->number)
			return s1;
	}
	return NULL;
}
#ifdef NQPROT
void DP5_ParseDelta(entity_state_t *s, packet_entities_t *pack)
{
	unsigned int bits;

	if (cl_shownet.ival >= 3)
		Con_Printf("%3i:     Update %i", MSG_GetReadCount(), s->number);

	bits = MSG_ReadByte();
	if (bits & E5_EXTEND1)
	{
		bits |= MSG_ReadByte() << 8;
		if (bits & E5_EXTEND2)
		{
			bits |= MSG_ReadByte() << 16;
			if (bits & E5_EXTEND3)
				bits |= MSG_ReadByte() << 24;
		}
	}

	if (cl_shownet.ival >= 3)
	{
		if (bits & E5_FULLUPDATE)		Con_Printf(" full");
		if (bits & E5_ORIGIN)			Con_Printf(" origin");
		if (bits & E5_ANGLES)			Con_Printf(" angles");
		if (bits & E5_MODEL)			Con_Printf(" model");
		if (bits & E5_FRAME)			Con_Printf(" frame");
		if (bits & E5_SKIN)				Con_Printf(" kin");
		if (bits & E5_EFFECTS)			Con_Printf(" effects");
		if (bits & E5_EXTEND1)			Con_Printf(" extend1");
		if (bits & E5_FLAGS)			Con_Printf(" flags");
		if (bits & E5_ALPHA)			Con_Printf(" alpha");
		if (bits & E5_SCALE)			Con_Printf(" scale");
		if (bits & E5_ORIGIN32)			Con_Printf(" origin32");
		if (bits & E5_ANGLES16)			Con_Printf(" angles16");
		if (bits & E5_MODEL16)			Con_Printf(" model16");
		if (bits & E5_COLORMAP)			Con_Printf(" colormap");
		if (bits & E5_EXTEND2)			Con_Printf(" extend2");
		if (bits & E5_ATTACHMENT)		Con_Printf(" attachment");
		if (bits & E5_LIGHT)			Con_Printf(" light");
		if (bits & E5_GLOW)				Con_Printf(" glow");
		if (bits & E5_EFFECTS16)		Con_Printf(" effects16");
		if (bits & E5_EFFECTS32)		Con_Printf(" effects32");
		if (bits & E5_FRAME16)			Con_Printf(" frame16");
		if (bits & E5_COLORMOD)			Con_Printf(" colormod");
		if (bits & E5_EXTEND3)			Con_Printf(" extend3");
		if (bits & E5_GLOWMOD)			Con_Printf(" glowmod");
		if (bits & E5_COMPLEXANIMATION)	Con_Printf(" complexanimation");
		if (bits & E5_TRAILEFFECTNUM)	Con_Printf(" traileffectnum");
		if (bits & E5_UNUSED27)			Con_Printf(" unused27");
		if (bits & E5_UNUSED28)			Con_Printf(" unused28");
		if (bits & E5_UNUSED29)			Con_Printf(" unused29");
		if (bits & E5_UNUSED30)			Con_Printf(" unused30");
		if (bits & E5_EXTEND4)			Con_Printf(" extend4");
		Con_Printf("\n");
	}

	if (bits & E5_ALLUNUSED)
	{
		Host_EndGame("Detected 'unused' bits in DP5+ entity delta - %x (%x)\n", bits, (bits & E5_ALLUNUSED));
	}

	if (bits & E5_FULLUPDATE)
	{
		int num;
		num = s->number;
		*s = nullentitystate;
		s->number = num;
		s->solidsize = ES_SOLID_BSP;
//		s->active = true;
	}
	if (bits & E5_FLAGS)
	{
		int i = MSG_ReadByte();
		s->dpflags = i;
	}
	if (bits & E5_ORIGIN)
	{
		if (bits & E5_ORIGIN32)
		{
			s->origin[0] = MSG_ReadFloat();
			s->origin[1] = MSG_ReadFloat();
			s->origin[2] = MSG_ReadFloat();
		}
		else
		{
			s->origin[0] = MSG_ReadShort()*(1/8.0f);
			s->origin[1] = MSG_ReadShort()*(1/8.0f);
			s->origin[2] = MSG_ReadShort()*(1/8.0f);
		}
	}
	if (bits & E5_ANGLES)
	{
		if (bits & E5_ANGLES16)
		{
			s->angles[0] = MSG_ReadAngle16();
			s->angles[1] = MSG_ReadAngle16();
			s->angles[2] = MSG_ReadAngle16();
		}
		else
		{
			s->angles[0] = MSG_ReadChar() * (360.0/256);
			s->angles[1] = MSG_ReadChar() * (360.0/256);
			s->angles[2] = MSG_ReadChar() * (360.0/256);
		}
	}
	if (bits & E5_MODEL)
	{
		if (bits & E5_MODEL16)
			s->modelindex = (unsigned short) MSG_ReadShort();
		else
			s->modelindex = MSG_ReadByte();
	}
	if (bits & E5_FRAME)
	{
		if (bits & E5_FRAME16)
			s->frame = (unsigned short) MSG_ReadShort();
		else
			s->frame = MSG_ReadByte();
	}
	if (bits & E5_SKIN)
		s->skinnum = MSG_ReadByte();
	if (bits & E5_EFFECTS)
	{
		if (bits & E5_EFFECTS32)
			s->effects = (unsigned int) MSG_ReadLong();
		else if (bits & E5_EFFECTS16)
			s->effects = (unsigned short) MSG_ReadShort();
		else
			s->effects = MSG_ReadByte();
	}
	if (bits & E5_ALPHA)
		s->trans = MSG_ReadByte();
	if (bits & E5_SCALE)
		s->scale = MSG_ReadByte();
	if (bits & E5_COLORMAP)
		s->colormap = MSG_ReadByte();
	if (bits & E5_ATTACHMENT)
	{
		s->tagentity = MSGCL_ReadEntity();
		s->tagindex = MSG_ReadByte();
	}
	if (bits & E5_LIGHT)
	{
		s->light[0] = MSG_ReadShort();
		s->light[1] = MSG_ReadShort();
		s->light[2] = MSG_ReadShort();
		s->light[3] = MSG_ReadShort();
		s->lightstyle = MSG_ReadByte();
		s->lightpflags = MSG_ReadByte();
	}
	if (bits & E5_GLOW)
	{
		s->glowsize = MSG_ReadByte();
		s->glowcolour = MSG_ReadByte();
	}
	if (bits & E5_COLORMOD)
	{
		s->colormod[0] = MSG_ReadByte();
		s->colormod[1] = MSG_ReadByte();
		s->colormod[2] = MSG_ReadByte();
	}
	if (bits & E5_GLOWMOD)
	{
		s->glowmod[0] = MSG_ReadByte();
		s->glowmod[1] = MSG_ReadByte();
		s->glowmod[2] = MSG_ReadByte();
	}
	if (bits & E5_COMPLEXANIMATION)
	{
		int type = MSG_ReadByte();
		int i, numbones;
		if (type == 4)
		{
			short *bonedata;

			/*modelindex = */MSG_ReadShort();
			numbones = MSG_ReadByte();

			bonedata = AllocateBoneSpace(pack, numbones, &s->boneoffset);
			s->bonecount = numbones;
			for (i = 0; i < numbones*7; i++)
				bonedata[i] = MSG_ReadShort();
		}
		else if (type < 4)
		{	//n-way blends
			s->bonecount = 0;
			type++;
			for (i = 0; i < type; i++)
				/*frame = */MSG_ReadShort();
			for (i = 0; i < type; i++)
				/*age = */MSG_ReadShort();
			for (i = 0; i < type; i++)
				/*frac = */(type==1)?255:MSG_ReadByte();
		}
		else
			Host_Error("E5_COMPLEXANIMATION: Parse error - unknown type %i\n", type);
	}
	if (bits & E5_TRAILEFFECTNUM)
		s->u.q1.traileffectnum = MSG_ReadShort();
}

static int QDECL CLDP_SortEntities(const void *va, const void *vb)
{
	const entity_state_t *a = va, *b = vb;
	if (a->inactiveflag != b->inactiveflag)
		return a->inactiveflag?1:-1;
	if (a->number != b->number)
		return a->number < b->number?-1:1;
	return 0;
}

void CLDP_ParseDarkPlaces5Entities(void)	//the things I do.. :o(
{
	//the incoming entities do not come in in any order. :(
	//well, they come in in order of priorities, but that's not useful to us.
	//I guess this means we'll have to go slowly.

	//dp deltas update in-place
	//this gets in the way of tracking multiple frames, and thus doesn't match fte too well


	packet_entities_t	*oldpack, *newpack;

	entity_state_t		*to, *from;
	unsigned int read;
	int oldi;
	qboolean remove;

	//server->client sequence
	if (cl.numackframes == sizeof(cl.ackframes)/sizeof(cl.ackframes[0]))
		cl.numackframes--;
	cl.ackframes[cl.numackframes++] = MSG_ReadLong(); /*server sequence to be acked*/

	//client->server sequence ack
	if (cls.protocol_nq >= CPNQ_DP7)
		CL_AckedInputFrame(cls.netchan.incoming_sequence, MSG_ReadLong(), true); /*client input sequence which has been acked*/

	if (cl.validsequence)
		oldpack = &cl.inframes[(cl.validsequence)&UPDATE_MASK].packet_entities;
	else
		oldpack = NULL;
	cl.validsequence = cls.netchan.incoming_sequence;
	cl.inframes[(cls.netchan.incoming_sequence)&UPDATE_MASK].receivedtime = realtime;
	cl.inframes[(cls.netchan.incoming_sequence)&UPDATE_MASK].frameid = cls.netchan.incoming_sequence;
	newpack = &cl.inframes[(cls.netchan.incoming_sequence)&UPDATE_MASK].packet_entities;
	newpack->servertime = cl.gametime;

	//copy old state to new state
	if (newpack != oldpack)
	{
		if (oldpack)
		{
			newpack->num_entities = oldpack->num_entities;
			newpack->max_entities = newpack->num_entities+16;	//for slop for new ents, to reduce reallocs
			newpack->entities = BZ_Realloc(newpack->entities, sizeof(entity_state_t)*newpack->max_entities);
			memcpy(newpack->entities, oldpack->entities, sizeof(entity_state_t)*newpack->num_entities);
		}
		else
			newpack->num_entities = 0;
		newpack->bonedatacur = 0;

		//flag them all as having old bones
		//they'll be renewed after parsing
		for (oldi=0 ; oldi<newpack->num_entities ; oldi++)
			newpack->entities[oldi].boneoffset |= 0x80000000;
	}

	for (;;)
	{
		read = MSG_ReadShort();
		if (msg_badread)
			Host_EndGame("Corrupt entity message packet\n");
		remove = !!(read&0x8000);
		read&=0x7fff;
		if (remove && !read)
			break;	//remove world signals end of packet.

		if (read >= MAX_EDICTS)
			Host_EndGame("Too many entities.\n");

		from = &nullentitystate;
		to = NULL;

		for (oldi=0 ; oldi<newpack->num_entities ; oldi++)
		{
			if (read == newpack->entities[oldi].number)
			{
				from = &newpack->entities[oldi];
				to = &newpack->entities[oldi];
				break;
			}
		}

		if (!to)
		{	//okay, so this is new
			if (newpack->num_entities==newpack->max_entities)
			{
				newpack->max_entities = newpack->num_entities+16;
				newpack->entities = BZ_Realloc(newpack->entities, sizeof(entity_state_t)*newpack->max_entities);
			}

			to = &newpack->entities[newpack->num_entities];
			newpack->num_entities++;
		}

		memcpy(to, from, sizeof(*to));
		to->number = read;

		if (remove)
		{	//ent is meant to be removed. flag it as such. we'll strip it out later.
			if (cl_shownet.ival >= 3)
				Con_Printf("Remove %i\n", read);
			to->inactiveflag = 1;
			to->bonecount = 0;
		}
		else
		{
			if (cl_shownet.ival > 3)
				Con_Printf("Update %i\n", read);
			DP5_ParseDelta(to, newpack);
			to->sequence = cls.netchan.incoming_sequence;
			to->inactiveflag = 0;
		}
	}

	qsort(newpack->entities, newpack->num_entities, sizeof(entity_state_t), CLDP_SortEntities);

	//get rid of any removed ents (we sorted these to the end)
	while (newpack->num_entities)
	{
		if (newpack->entities[newpack->num_entities-1].inactiveflag)
			newpack->num_entities--;
		else
			break;
	}

	//make sure any bone states are refreshed
	for (oldi=0, to = newpack->entities; oldi<newpack->num_entities ; oldi++, to++)
	{
		if (to->bonecount && (to->boneoffset & 0x80000000))
		{
			unsigned int oldoffset = to->boneoffset & 0x7fffffff;
			void *dest = AllocateBoneSpace(newpack, to->bonecount, &to->boneoffset);
			void *src = GetBoneSpace(oldpack, oldoffset);
			memcpy(dest, src, to->bonecount * sizeof(short)*7);
		}
	}
}

#ifdef HEXEN2
#define UH2_MOREBITS	(1u<<0)
#define UH2_ORIGIN1		(1u<<1)
#define UH2_ORIGIN2		(1u<<2)
#define UH2_ORIGIN3		(1u<<3)
#define UH2_ANGLE2		(1u<<4)
#define UH2_STEP		(1u<<5)
#define UH2_FRAME		(1u<<6)
#define UH2_SIGNAL		(1u<<7)

#define UH2_ANGLE1		(1u<<8)
#define UH2_ANGLE3		(1u<<9)
#define UH2_MODEL		(1u<<10)
//#define UH2_			(1u<<11)
//#define UH2_			(1u<<12)
//#define UH2_			(1u<<13)
#define UH2_LONGENTITY	(1u<<14)
#define UH2_EVENMORE	(1u<<15)

#define UH2_SKIN		(1u<<16)
#define UH2_EFFECTS		(1u<<17)
#define UH2_SCALE		(1u<<18)
#define UH2_COLORMAP	(1u<<19)

void CLH2_ParseEntities(void)
{
	//h2mp apparently uses some sort of delta compression
	//there's three parts to this, the start, the updates, and the removes at the end.
	//so we can be a bit lazy and parse the 'fast updates' here and assert they end with a final clear.
	//entities are ordered.

	packet_entities_t	*oldpack, *newpack;

	entity_state_t		*to, *from;
	unsigned int read, bits;
	int oldi;
	unsigned int removecount;

	int frame = MSG_ReadByte();
	int seq = MSG_ReadByte();

	//not really sure what to do with this.
	(void)frame;
	(void)seq;

	if (cl.validsequence)
		oldpack = &cl.inframes[(cl.validsequence)&UPDATE_MASK].packet_entities;
	else
		oldpack = NULL;
	cl.validsequence = cls.netchan.incoming_sequence;
	cl.inframes[(cls.netchan.incoming_sequence)&UPDATE_MASK].receivedtime = realtime;
	cl.inframes[(cls.netchan.incoming_sequence)&UPDATE_MASK].frameid = cls.netchan.incoming_sequence;
	newpack = &cl.inframes[(cls.netchan.incoming_sequence)&UPDATE_MASK].packet_entities;
	newpack->servertime = cl.gametime;

	//copy old state to new state
	if (newpack != oldpack)
	{
		if (oldpack)
		{
			newpack->num_entities = oldpack->num_entities;
			newpack->max_entities = newpack->num_entities+16;	//for slop for new ents, to reduce reallocs
			newpack->entities = BZ_Realloc(newpack->entities, sizeof(entity_state_t)*newpack->max_entities);
			memcpy(newpack->entities, oldpack->entities, sizeof(entity_state_t)*newpack->num_entities);
		}
		else
			newpack->num_entities = 0;
		newpack->bonedatacur = 0;

		//flag them all as having old bones
		//they'll be renewed after parsing
		for (oldi=0 ; oldi<newpack->num_entities ; oldi++)
			newpack->entities[oldi].boneoffset |= 0x80000000;
	}

	for (;;)
	{
		bits = MSG_ReadByte();
		if ((bits&0x80) == 0)
			break;	//no fast-update bit!
		if (bits & UH2_MOREBITS)
			bits |= MSG_ReadByte()<<8;
		if (bits & UH2_EVENMORE)
			bits |= MSG_ReadByte()<<16;
		if (bits & UH2_LONGENTITY)
			read = MSG_ReadUInt16();
		else
			read = MSG_ReadByte();

		if (msg_badread)
			Host_EndGame("Corrupt entity message packet\n");

		if (!read)
			break;	//remove world signals end of packet.

		if (read >= MAX_EDICTS)
			Host_EndGame("Too many entities.\n");

		if (!CL_CheckBaselines(read))
			Host_EndGame("CLNQ_ParseEntity: check baselines failed with size %i", read);
		from = &cl_baselines[read];
		to = NULL;

		for (oldi=0 ; oldi<newpack->num_entities ; oldi++)
		{
			if (read == newpack->entities[oldi].number)
			{
				from = &newpack->entities[oldi];
				to = &newpack->entities[oldi];
				break;
			}
		}

		if (!to)
		{	//okay, so this is new
			if (newpack->num_entities==newpack->max_entities)
			{
				newpack->max_entities = newpack->num_entities+16;
				newpack->entities = BZ_Realloc(newpack->entities, sizeof(entity_state_t)*newpack->max_entities);
			}

			to = &newpack->entities[newpack->num_entities];
			newpack->num_entities++;

			if (cl_shownet.ival >= 3)
				Con_Printf("%3i:     New %i %x\n", MSG_GetReadCount(), to->number, bits);
		}
		else if (cl_shownet.ival >= 3)
			Con_Printf("%3i:     Update %i %x\n", MSG_GetReadCount(), to->number, bits);

		memcpy(to, from, sizeof(*to));
		to->number = read;

		if (bits & UH2_MODEL)	to->modelindex = MSG_ReadShort();
		if (bits & UH2_FRAME)	to->frame = MSG_ReadByte();
		if (bits & UH2_COLORMAP)to->colormap = MSG_ReadByte();
		if (bits & UH2_SKIN)	to->skinnum = MSG_ReadByte();
		if (bits & UH2_SKIN)	to->hexen2flags = MSG_ReadByte();	//yes, shared with skin
		if (bits & UH2_EFFECTS)	to->effects = MSG_ReadByte();
		if (bits & UH2_ORIGIN1)	to->origin[0] = MSG_ReadCoord();
		if (bits & UH2_ANGLE1)	to->angles[0] = MSG_ReadAngle();
		if (bits & UH2_ORIGIN2)	to->origin[1] = MSG_ReadCoord();
		if (bits & UH2_ANGLE2)	to->angles[1] = MSG_ReadAngle();
		if (bits & UH2_ORIGIN3)	to->origin[2] = MSG_ReadCoord();
		if (bits & UH2_ANGLE3)	to->angles[2] = MSG_ReadAngle();
		if (bits & UH2_SCALE)	to->scale = (MSG_ReadByte()/100.0)*16;
		if (bits & UH2_SCALE)	to->abslight = MSG_ReadByte();

		to->sequence = cls.netchan.incoming_sequence;
		to->inactiveflag = 0;
	}

	//handle the removes
	if (bits != 48)
		Host_EndGame("Corrupt entity message packet\n");
	removecount = (qbyte)MSG_ReadByte();
	while (removecount --> 0)
	{
		read = MSG_ReadUInt16();
		for (oldi=0 ; oldi<newpack->num_entities ; oldi++)
		{
			if (read == newpack->entities[oldi].number)
			{
				newpack->entities[oldi].inactiveflag = true;
				break;
			}
		}
	}

	//sort them, just in case. the removes will bubble to the end.
	qsort(newpack->entities, newpack->num_entities, sizeof(entity_state_t), CLDP_SortEntities);
	while (newpack->num_entities)
	{	//pop those removes.
		if (newpack->entities[newpack->num_entities-1].inactiveflag)
			newpack->num_entities--;
		else
			break;
	}


	//make sure any bone states are refreshed
	for (oldi=0, to = newpack->entities; oldi<newpack->num_entities ; oldi++, to++)
	{
		if (to->bonecount && (to->boneoffset & 0x80000000))
		{
			unsigned int oldoffset = to->boneoffset & 0x7fffffff;
			void *dest = AllocateBoneSpace(newpack, to->bonecount, &to->boneoffset);
			void *src = GetBoneSpace(oldpack, oldoffset);
			memcpy(dest, src, to->bonecount * sizeof(short)*7);
		}
	}
}
#endif

void CLNQ_ParseEntity(unsigned int bits)
{
	int i;
	int num;
	entity_state_t		*state;//, *from;
	entity_state_t	*base;
	packet_entities_t	*pack;

	qboolean isnehahra = CPNQ_IS_BJP||(cls.protocol_nq == CPNQ_NEHAHRA);
	qboolean floatcoords;

	if (cls.signon == 4 - 1)
	{	// first update is the final signon stage
		cls.signon = 4;
		CLNQ_SignonReply ();
	}
	pack = &cl.inframes[cls.netchan.incoming_sequence&UPDATE_MASK].packet_entities;


	if (bits & NQU_MOREBITS)
	{
		i = MSG_ReadByte ();
		bits |= (i<<8);
	}

	if (bits & DPU_EXTEND1)
	{
		if (!isnehahra)
		{
			i = MSG_ReadByte ();
			bits |= (i<<16);
		
			if (bits & DPU_EXTEND2)
			{
				i = MSG_ReadByte ();
				bits |= (i<<24);
			}
		}
	}

	if (bits & NQU_LONGENTITY)
		num = MSGCL_ReadEntity ();
	else
		num = MSG_ReadByte ();

//	state = CL_FindPacketEntity(num);
//	if (!state)
	{
//		if ((int)(lasttime*100) != (int)(realtime*100))
//			pack->num_entities=0;
//		else
			if (pack->num_entities==pack->max_entities)
		{
			pack->max_entities = pack->num_entities+1;
			pack->entities = BZ_Realloc(pack->entities, sizeof(entity_state_t)*pack->max_entities);
			memset(pack->entities + pack->num_entities, 0, sizeof(entity_state_t));
		}
		state = &pack->entities[pack->num_entities++];
	}

//	from = CL_FindOldPacketEntity(num);	//this could be optimised.

	if (!CL_CheckBaselines(num))
		Host_EndGame("CLNQ_ParseEntity: check baselines failed with size %i", num);
	base = cl_baselines + num;
	memcpy(state, base, sizeof(*state));

	state->number = num;
	state->sequence = cls.netchan.incoming_sequence;
	state->solidsize = ES_SOLID_BSP;

	state->dpflags = 0;

	floatcoords = cls.qex && (bits & QE_U_FLOATCOORDS);

	if (bits & NQU_MODEL)
	{
		if (CPNQ_IS_BJP)
			state->modelindex = MSG_ReadShort ();
		else
			state->modelindex = MSG_ReadByte ();
	}

	if (bits & NQU_FRAME)
		state->frame = MSG_ReadByte();

	if (bits & NQU_COLORMAP)
		state->colormap = MSG_ReadByte();

	if (bits & NQU_SKIN)
		state->skinnum = MSG_ReadByte();

	if (bits & NQU_EFFECTS)
	{
		i = MSG_ReadByte();
		if (cls.qex)
		{
			unsigned fixed = i & ~(REEF_QUADLIGHT|REEF_PENTLIGHT|REEF_CANDLELIGHT);
			if (i & REEF_QUADLIGHT)
				fixed |= EF_BLUE;
			if (i & REEF_PENTLIGHT)
				fixed |= EF_RED;
			if (i & REEF_CANDLELIGHT)
				fixed |= 0;	//tiny light
			i = fixed;
		}
		state->effects = i;
	}

	if (bits & NQU_ORIGIN1)
		state->origin[0] = floatcoords?MSG_ReadFloat():MSG_ReadCoord ();
	if (bits & NQU_ANGLE1)
		state->angles[0] = MSG_ReadAngle();

	if (bits & NQU_ORIGIN2)
		state->origin[1] = floatcoords?MSG_ReadFloat():MSG_ReadCoord ();
	if (bits & NQU_ANGLE2)
		state->angles[1] = MSG_ReadAngle();

	if (bits & NQU_ORIGIN3)
		state->origin[2] = floatcoords?MSG_ReadFloat():MSG_ReadCoord ();
	if (bits & NQU_ANGLE3)
		state->angles[2] = MSG_ReadAngle();

	if (bits & NQU_NOLERP)
		state->dpflags |= RENDER_STEP;

	if (isnehahra)
	{
		if (bits & DPU_EXTEND1)	//U_TRANS
		{
			float tmp = MSG_ReadFloat();
			float alpha = MSG_ReadFloat();
			if (tmp == 2)
			{
				if (MSG_ReadFloat() > 0.5)
					state->effects |= EF_FULLBRIGHT;
			}
			if (!alpha)
				alpha = 1;
			state->trans = bound(0, 255 * alpha, 255);
		}
	}
	else if (cls.protocol_nq == CPNQ_FITZ666)
	{
		if (bits & FITZU_ALPHA)
			state->trans = (MSG_ReadByte()-1)&0xff;

		if (bits & RMQU_SCALE)
			state->scale = MSG_ReadByte();

		if (bits & FITZU_FRAME2)
			state->frame = (state->frame & 0xff) | (MSG_ReadByte() << 8);

		if (bits & FITZU_MODEL2)
			state->modelindex = (state->modelindex & 0xff) | (MSG_ReadByte() << 8);

		if (bits & FITZU_LERPFINISH)
			state->lerpend = cl.gametime + MSG_ReadByte()/255.0f;

		if (cls.qex)
		{
			if (bits & QE_U_SOLIDTYPE)	/*state->solidsize =*/ MSG_ReadByte();		//needed for correct prediction
			if (bits & QE_U_ENTFLAGS)	/*state->entflags = */ MSG_ReadULEB128();	//for onground/etc state
			if (bits & QE_U_HEALTH)		/*state->health =*/ MSG_ReadSignedQEX();	//health... not really sure why, I suppose it changes player physics (they should have sent movetype instead though).
			if (bits & QE_U_UNKNOWN26)	/*unknown =*/MSG_ReadByte();
			if (bits & QE_U_UNUSED27)	Con_Printf(CON_WARNING"QE_U_UNUSED27: %u\n", MSG_ReadByte());

			if (bits & QE_U_UNUSED28)	Con_Printf(CON_WARNING"QE_U_UNUSED28: %u\n", MSG_ReadByte());
			if (bits & QE_U_UNUSED29)	Con_Printf(CON_WARNING"QE_U_UNUSED29: %u\n", MSG_ReadByte());
			if (bits & QE_U_UNUSED30)	Con_Printf(CON_WARNING"QE_U_UNUSED30: %u\n", MSG_ReadByte());
			if (bits & QE_U_UNUSED31)	Con_Printf(CON_WARNING"QE_U_UNUSED31: %u\n", MSG_ReadByte());
		}
	}
	else
	{	//dp tends to leak stuff, so parse as quakedp if the normal protocol doesn't define it as something better.

//		if (bits & DPU_DELTA)	//should delta from the previous frame. DP doesn't generate this any more, so whatever.
//			Host_EndGame("CLNQ_ParseEntity: DPU_DELTA not supported");

		if (bits & DPU_ALPHA)
			state->trans = MSG_ReadByte();

		if (bits & DPU_SCALE)
			state->scale = MSG_ReadByte();

		if (bits & DPU_EFFECTS2)
			state->effects |= MSG_ReadByte() << 8;

		if (bits & DPU_GLOWSIZE)
			state->glowsize = MSG_ReadByte();

		if (bits & DPU_GLOWCOLOR)
			state->glowcolour = MSG_ReadByte();

		if (bits & DPU_COLORMOD)
		{
			i = MSG_ReadByte(); // follows format RRRGGGBB
			state->colormod[0] = (qbyte)(((i >> 5) & 7) * (32.0f / 7.0f));
			state->colormod[1] = (qbyte)(((i >> 2) & 7) * (32.0f / 7.0f));
			state->colormod[2] = (qbyte)((i & 3) * (32.0f / 3.0f));
		}

		if (bits & DPU_GLOWTRAIL)
			state->dpflags |= RENDER_GLOWTRAIL;

		if (bits & DPU_FRAME2)
			state->frame |= MSG_ReadByte() << 8;

		if (bits & DPU_MODEL2)
			state->modelindex |= MSG_ReadByte() << 8;

		if (bits & DPU_VIEWMODEL)
			state->dpflags |= RENDER_VIEWMODEL;
		if (bits & DPU_EXTERIORMODEL)
			state->dpflags |= RENDER_EXTERIORMODEL;
	}
}
#endif
#ifdef PEXT_SETVIEW
entity_state_t *CL_FindPacketEntity(int num)
{
	int					pnum;
	entity_state_t		*s1;
	packet_entities_t	*pack = cl.currentpackentities;
	if (pack)
		for (pnum=0 ; pnum<pack->num_entities ; pnum++)
		{
			s1 = &pack->entities[pnum];

			if (num == s1->number)
				return s1;
		}
	return NULL;
}
#endif

void CL_RotateAroundTag(entity_t *ent, int entnum, int parenttagent, int parenttagnum)
{
	entity_state_t *ps;
	float *org=NULL, *ang=NULL;
	vec3_t axis[3];
	float transform[12], parent[12], result[12], old[12], temp[12];

	model_t *model;
	framestate_t fstate;

	if (parenttagent >= cl.maxlerpents)
	{
		Con_Printf("tag entity out of range!\n");
		return;
	}

	//old is the entity's relative transform (relative to the parent entity's tag)
	old[0] = ent->axis[0][0];
	old[1] = ent->axis[1][0];
	old[2] = ent->axis[2][0];
	old[3] = ent->origin[0];
	old[4] = ent->axis[0][1];
	old[5] = ent->axis[1][1];
	old[6] = ent->axis[2][1];
	old[7] = ent->origin[1];
	old[8] = ent->axis[0][2];
	old[9] = ent->axis[1][2];
	old[10] = ent->axis[2][2];
	old[11] = ent->origin[2];

	memset(&fstate, 0, sizeof(fstate));

	//for visibility checks
	ent->keynum = parenttagent;

	ps = CL_FindPacketEntity(parenttagent);
	if (ps)
	{
		if (parenttagent >= cl.maxlerpents)
		{
			org = ps->origin;
			ang = ps->angles;
		}
		else
		{
			lerpents_t *le = &cl.lerpents[parenttagent];
			org = le->origin;
			ang = le->angles;
		}

		if (ps->modelindex <= countof(cl.model_precache) && cl.model_precache[ps->modelindex] && cl.model_precache[ps->modelindex]->loadstate == MLS_LOADED)
			model = cl.model_precache[ps->modelindex];
		else
			model = NULL;
		if (model && model->type == mod_alias)
			AngleVectorsMesh(ang, axis[0], axis[1], axis[2]);
		else
			AngleVectors(ang, axis[0], axis[1], axis[2]);
		VectorInverse(axis[1]);

		parent[0] = axis[0][0];
		parent[1] = axis[1][0];
		parent[2] = axis[2][0];
		parent[3] = org[0];
		parent[4] = axis[0][1];
		parent[5] = axis[1][1];
		parent[6] = axis[2][1];
		parent[7] = org[1];
		parent[8] = axis[0][2];
		parent[9] = axis[1][2];
		parent[10] = axis[2][2];
		parent[11] = org[2];

		CL_LerpNetFrameState(&fstate, &cl.lerpents[parenttagent]);

		/*inherit certain properties from the parent entity*/
		if (ps->dpflags & RENDER_VIEWMODEL)
			ent->flags |= RF_WEAPONMODEL|Q2RF_MINLIGHT|RF_DEPTHHACK;
		if ((ps->dpflags & RENDER_EXTERIORMODEL) || r_refdef.playerview->viewentity == ps->number)
			ent->flags |= RF_EXTERNALMODEL;

		//hack for xonotic.
		if ((ent->flags & RF_WEAPONMODEL) && ent->playerindex == -1 && ps->colormap > 0 && ps->colormap <= cl.allocated_client_slots)
		{
			ent->playerindex = ps->colormap-1;
			ent->topcolour    = cl.players[ent->playerindex].dtopcolor;
			ent->bottomcolour = cl.players[ent->playerindex].dbottomcolor;
		}
	}
	else
	{
		extern int parsecountmod;
//		Con_Printf("tagent %i\n", tagent);
		if (parenttagent <= cl.allocated_client_slots && parenttagent > 0)
		{
			if (parenttagent == cl.playerview[0].playernum+1)
			{
				org = cl.playerview[0].simorg;
				ang = cl.playerview[0].simangles;
			}
			else
			{
				org = cl.inframes[parsecountmod].playerstate[parenttagent-1].origin;
				ang = cl.inframes[parsecountmod].playerstate[parenttagent-1].viewangles;
			}
			model = cl.model_precache[cl.inframes[parsecountmod].playerstate[parenttagent-1].modelindex];

			CL_LerpNetFrameState(&fstate, &cl.lerpplayers[parenttagent-1]);
		}
		else
		{
			CL_LerpNetFrameState(&fstate, &cl.lerpents[parenttagent]);
			model = 0;
		}
	}

	{
//		fstate.g[FS_REG].lerpfrac = CL_EntLerpFactor(tagent);
//		fstate.g[FS_REG].frametime[0] = cl.time - cl.lerpents[tagent].framechange;
//		fstate.g[FS_REG].frametime[1] = cl.time - cl.lerpents[tagent].oldframechange;

		if (Mod_GetTag(model, parenttagnum, &fstate, transform))
		{
//			parent -> transform -> old

			R_ConcatTransforms((void*)parent, (void*)transform, (void*)temp);
			R_ConcatTransforms((void*)temp, (void*)old, (void*)result);

			ent->axis[0][0] = result[0];
			ent->axis[1][0] = result[1];
			ent->axis[2][0] = result[2];
			ent->origin[0] = result[3];
			ent->axis[0][1] = result[4];
			ent->axis[1][1] = result[5];
			ent->axis[2][1] = result[6];
			ent->origin[1] = result[7];
			ent->axis[0][2] = result[8];
			ent->axis[1][2] = result[9];
			ent->axis[2][2] = result[10];
			ent->origin[2] = result[11];
		}
		else	//hrm.
		{
			R_ConcatTransforms((void*)parent, (void*)old, (void*)result);

			ent->axis[0][0] = result[0];
			ent->axis[1][0] = result[1];
			ent->axis[2][0] = result[2];
			ent->origin[0] = result[3];
			ent->axis[0][1] = result[4];
			ent->axis[1][1] = result[5];
			ent->axis[2][1] = result[6];
			ent->origin[1] = result[7];
			ent->axis[0][2] = result[8];
			ent->axis[1][2] = result[9];
			ent->axis[2][2] = result[10];
			ent->origin[2] = result[11];
		}
	}

	if (ps && ps->tagentity)
		CL_RotateAroundTag(ent, entnum, ps->tagentity, ps->tagindex);
}

void V_AddAxisEntity(entity_t *in)
{
	entity_t *ent;

	if (cl_numvisedicts == cl_maxvisedicts)
	{
		return;		// object list is full
	}
	ent = &cl_visedicts[cl_numvisedicts];
	cl_numvisedicts++;

	*ent = *in;
}
void V_ClearEntity(entity_t *e)
{
	memset(e, 0, sizeof(*e));
	e->pvscache.num_leafs = -1;
	e->playerindex = -1;
	e->topcolour = TOP_DEFAULT;
	e->bottomcolour = BOTTOM_DEFAULT;
}
entity_t *V_AddEntity(entity_t *in)
{
	entity_t *ent;

	if (cl_numvisedicts == cl_maxvisedicts)
	{
		return NULL;		// object list is full
	}
	ent = &cl_visedicts[cl_numvisedicts];
	cl_numvisedicts++;

	*ent = *in;

	AngleVectorsMesh(ent->angles, ent->axis[0], ent->axis[1], ent->axis[2]);
	VectorInverse(ent->axis[1]);

	return ent;
}
entity_t *V_AddNewEntity(void)
{
	entity_t *ent;

	if (cl_numvisedicts == cl_maxvisedicts)
	{
		cl_expandvisents = true;
		return NULL;		// object list is full
	}
	ent = &cl_visedicts[cl_numvisedicts];
	cl_numvisedicts++;
	return ent;
}
/*
void VQ2_AddLerpEntity(entity_t *in)	//a convienience function
{
	entity_t *ent;
	float fwds, back;
	int i;

	if (cl_numvisedicts == MAX_VISEDICTS)
		return;		// object list is full
	ent = &cl_visedicts[cl_numvisedicts];
	cl_numvisedicts++;

	*ent = *in;

	fwds = ent->framestate.g[FS_REG].lerpfrac;
	back = 1 - ent->framestate.g[FS_REG].lerpfrac;
	for (i = 0; i < 3; i++)
	{
		ent->origin[i] = in->origin[i]*fwds + in->oldorigin[i]*back;
	}

	ent->framestate.g[FS_REG].lerpfrac = back;

	AngleVectorsMesh(ent->angles, ent->axis[0], ent->axis[1], ent->axis[2]);
	VectorInverse(ent->axis[1]);
}
*/
int V_AddLight (int entsource, vec3_t org, float quant, float r, float g, float b)
{
	return CL_NewDlight (entsource, org, quant, -0.1, r*5, g*5, b*5) - cl_dlights;
}

void CLQ1_AddOrientedHalfSphere(shader_t *shader, float radius, float gap, float *matrix, float r, float g, float b, float a)
{
	//use simple algo
	//a series of cylinders that gets progressively narrower
	const int latsteps = 16;
	const int lngsteps = 8;//16;
	float cradius;
	int v, i, j;
	scenetris_t *t;
	vec3_t corner;
	float x,y;
	int flags = BEF_NODLIGHT|BEF_NOSHADOWS;

	if (!r && !g && !b)
		return;

	/*reuse the previous trigroup if its the same shader*/
	if (cl_numstris && cl_stris[cl_numstris-1].shader == shader && cl_stris[cl_numstris-1].flags == flags && cl_stris[cl_numstris-1].numvert < MAX_INDICIES-(latsteps-1)*(lngsteps-1))
		t = &cl_stris[cl_numstris-1];
	else
	{
		if (cl_numstris == cl_maxstris)
		{
			cl_maxstris += 8;
			cl_stris = BZ_Realloc(cl_stris, sizeof(*cl_stris)*cl_maxstris);
		}
		t = &cl_stris[cl_numstris++];
		t->shader = shader;
		t->numidx = 0;
		t->numvert = 0;
		t->firstidx = cl_numstrisidx;
		t->firstvert = cl_numstrisvert;
		t->flags = flags;
	}

	if (cl_numstrisvert + latsteps*lngsteps > cl_maxstrisvert)
		cl_stris_ExpandVerts(cl_numstrisvert + latsteps*lngsteps);
	if (cl_maxstrisidx < cl_numstrisidx+latsteps*(lngsteps-1)*6)
	{
		cl_maxstrisidx = cl_numstrisidx+latsteps*(lngsteps-1)*6 + 64;
		cl_strisidx = BZ_Realloc(cl_strisidx, sizeof(*cl_strisidx)*cl_maxstrisidx);
	}

	for (i = 0; i < latsteps; i++)
	{
		x = sin(i * 2 * M_PI / latsteps);
		y = cos(i * 2 * M_PI / latsteps);
		for (j = 0; j < lngsteps; j++)
		{
			v = i*lngsteps + j;
			cradius = sin(j * 0.5 * M_PI / (lngsteps-1))*radius;
			corner[0] = x*cradius;
			corner[1] = y*cradius;
			corner[2] = (cos(j * 0.5 * M_PI / (lngsteps-1))*-radius) - gap;
			Matrix3x4_RM_Transform3(matrix, corner, cl_strisvertv[cl_numstrisvert+v]);

			cl_strisvertt[cl_numstrisvert+v][0] = 0;
			cl_strisvertt[cl_numstrisvert+v][1] = 0;

			cl_strisvertc[cl_numstrisvert+v][0] = r;
			cl_strisvertc[cl_numstrisvert+v][1] = g;
			cl_strisvertc[cl_numstrisvert+v][2] = b;
			cl_strisvertc[cl_numstrisvert+v][3] = a;
		}
	}

	if (radius < 0)
	{
		for (i = 0; i < lngsteps-1; i++)
		{
			v = latsteps-1;
			for (v = 0; v < latsteps-1; v++)
			{
				cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+lngsteps	+ v*lngsteps + i;
				cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+0			+ v*lngsteps + i;
				cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+1			+ v*lngsteps + i;
				cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+lngsteps+1	+ v*lngsteps + i;
				cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+lngsteps	+ v*lngsteps + i;
				cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+1			+ v*lngsteps + i;
			}
			cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert					+ i;
			cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+0				+ v*lngsteps + i;
			cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+1				+ v*lngsteps + i;
			cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+1				+ i;
			cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert					+ i;
			cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+1				+ v*lngsteps + i;
		}
	}
	else
	{
		for (i = 0; i < lngsteps-1; i++)
		{
			v = latsteps-1;
			for (v = 0; v < latsteps-1; v++)
			{
				cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+0			+ v*lngsteps + i;
				cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+lngsteps	+ v*lngsteps + i;
				cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+1			+ v*lngsteps + i;
				cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+lngsteps	+ v*lngsteps + i;
				cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+lngsteps+1	+ v*lngsteps + i;
				cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+1			+ v*lngsteps + i;
			}
			cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+0				+ v*lngsteps + i;
			cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert					+ i;
			cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+1				+ v*lngsteps + i;
			cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert					+ i;
			cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+1				+ i;
			cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+1				+ v*lngsteps + i;
		}
	}

	t->numvert += lngsteps*latsteps;
	t->numidx = cl_numstrisidx - t->firstidx;
	cl_numstrisvert += lngsteps*latsteps;
}

void CLQ1_AddOrientedSphere(shader_t *shader, float radius, float *matrix, float r, float g, float b, float a)
{
	CLQ1_AddOrientedHalfSphere(shader, radius, 0, matrix, r, g, b, a);
	CLQ1_AddOrientedHalfSphere(shader, -radius, 0, matrix, r, g, b, a);
}

void CLQ1_AddOrientedCylinder(shader_t *shader, float radius, float height, qboolean capsule, float *matrix, float r, float g, float b, float a)
{
	int sides = 16;
	int v;
	scenetris_t *t;
	vec3_t corner;
	int flags = BEF_NODLIGHT|BEF_NOSHADOWS;

	if (!r && !g && !b)
		return;

	radius *= 0.5;
	height *= 0.5;

	if (capsule)
		height -= radius;

	if (height > 0)
	{
		/*reuse the previous trigroup if its the same shader*/
		if (cl_numstris && cl_stris[cl_numstris-1].shader == shader && cl_stris[cl_numstris-1].flags == flags)
			t = &cl_stris[cl_numstris-1];
		else
		{
			if (cl_numstris == cl_maxstris)
			{
				cl_maxstris += 8;
				cl_stris = BZ_Realloc(cl_stris, sizeof(*cl_stris)*cl_maxstris);
			}
			t = &cl_stris[cl_numstris++];
			t->shader = shader;
			t->numidx = 0;
			t->numvert = 0;
			t->firstidx = cl_numstrisidx;
			t->firstvert = cl_numstrisvert;
			t->flags = flags;
		}

		if (cl_numstrisvert + sides*2 > cl_maxstrisvert)
			cl_stris_ExpandVerts(cl_numstrisvert + sides*2);
		if (cl_maxstrisidx < cl_numstrisidx+sides*6)
		{
			cl_maxstrisidx = cl_numstrisidx+sides*6 + 64;
			cl_strisidx = BZ_Realloc(cl_strisidx, sizeof(*cl_strisidx)*cl_maxstrisidx);
		}


		for (v = 0; v < sides*2; v++)
		{
			corner[0] = sin((v>>1) * 2 * M_PI / sides)*radius;
			corner[1] = cos((v>>1) * 2 * M_PI / sides)*radius;
			corner[2] = (v & 1)?height:-height;
			Matrix3x4_RM_Transform3(matrix, corner, cl_strisvertv[cl_numstrisvert+v]);

			cl_strisvertt[cl_numstrisvert+v][0] = 0;
			cl_strisvertt[cl_numstrisvert+v][1] = 0;

			cl_strisvertc[cl_numstrisvert+v][0] = r;
			cl_strisvertc[cl_numstrisvert+v][1] = g;
			cl_strisvertc[cl_numstrisvert+v][2] = b;
			cl_strisvertc[cl_numstrisvert+v][3] = a;
		}
		for (v = 0; v < sides-1; v++)
		{
			cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+2 + v*2;
			cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+1 + v*2;
			cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+0 + v*2;
			cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+3 + v*2;
			cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+1 + v*2;
			cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+2 + v*2;
		}
		cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+0;
		cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+1 + v*2;
		cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+0 + v*2;
		cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+1;
		cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+1 + v*2;
		cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+0;

		if (!capsule)
		{
			for (v = 4; v < sides*2; v+=2)
			{
				cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+v;
				cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+(v-2);
				cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+0;

				cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+1;
				cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+(v-2)+1;
				cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+v+1;
			}
		}

		t->numvert += sides*2;
		t->numidx = cl_numstrisidx - t->firstidx;
		cl_numstrisvert += sides*2;
	}

	if (capsule)
	{
		CLQ1_AddOrientedHalfSphere(shader, radius, height, matrix, r, g, b, a);
		CLQ1_AddOrientedHalfSphere(shader, -radius, -height, matrix, r, g, b, a);
	}
}
void CLQ1_DrawLine(shader_t *shader, vec3_t v1, vec3_t v2, float r, float g, float b, float a)
{
	scenetris_t *t;
	int flags = BEF_NODLIGHT|BEF_NOSHADOWS|BEF_LINES;

	if (cl_numstris && cl_stris[cl_numstris-1].shader == shader && cl_stris[cl_numstris-1].flags == flags)
		t = &cl_stris[cl_numstris-1];
	else
	{
		if (cl_numstris == cl_maxstris)
		{
			cl_maxstris += 8;
			cl_stris = BZ_Realloc(cl_stris, sizeof(*cl_stris)*cl_maxstris);
		}
		t = &cl_stris[cl_numstris++];
		t->shader = shader;
		t->numidx = 0;
		t->numvert = 0;
		t->firstidx = cl_numstrisidx;
		t->firstvert = cl_numstrisvert;
		t->flags = flags;
	}
	if (cl_numstrisvert + 2 > cl_maxstrisvert)
		cl_stris_ExpandVerts(cl_numstrisvert + 2);
	if (cl_maxstrisidx < cl_numstrisidx+2)
	{
		cl_maxstrisidx = cl_numstrisidx+2;
		cl_strisidx = BZ_Realloc(cl_strisidx, sizeof(*cl_strisidx)*cl_maxstrisidx);
	}

	VectorCopy(v1, cl_strisvertv[cl_numstrisvert+0]);
	cl_strisvertt[cl_numstrisvert+0][0] = 0;
	cl_strisvertt[cl_numstrisvert+0][1] = 0;
	cl_strisvertc[cl_numstrisvert+0][0] = r;
	cl_strisvertc[cl_numstrisvert+0][1] = g;
	cl_strisvertc[cl_numstrisvert+0][2] = b;
	cl_strisvertc[cl_numstrisvert+0][3] = a;

	VectorCopy(v2, cl_strisvertv[cl_numstrisvert+1]);
	cl_strisvertt[cl_numstrisvert+1][0] = 0;
	cl_strisvertt[cl_numstrisvert+1][1] = 0;
	cl_strisvertc[cl_numstrisvert+1][0] = r;
	cl_strisvertc[cl_numstrisvert+1][1] = g;
	cl_strisvertc[cl_numstrisvert+1][2] = b;
	cl_strisvertc[cl_numstrisvert+1][3] = a;

	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+0;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert-t->firstvert+1;

	t->numvert += 2;
	t->numidx = cl_numstrisidx - t->firstidx;
	cl_numstrisvert += 2;
}
void CLQ1_AddSpriteQuad(shader_t *shader, vec3_t mid, float radius)
{
	float r=1, g=1, b=1;
	scenetris_t *t;
	int flags = BEF_NODLIGHT|BEF_NOSHADOWS;

	if (cl_numstris && cl_stris[cl_numstris-1].shader == shader && cl_stris[cl_numstris-1].flags == flags && cl_stris[cl_numstris-1].numvert + 4 <= MAX_INDICIES)
		t = &cl_stris[cl_numstris-1];
	else
	{
		if (cl_numstris == cl_maxstris)
		{
			cl_maxstris+=8;
			cl_stris = BZ_Realloc(cl_stris, sizeof(*cl_stris)*cl_maxstris);
		}
		t = &cl_stris[cl_numstris++];
		t->shader = shader;
		t->firstidx = cl_numstrisidx;
		t->firstvert = cl_numstrisvert;
		t->numvert = 0;
		t->numidx = 0;
		t->flags = flags;
	}

	if (cl_numstrisidx+6 > cl_maxstrisidx)
	{
		cl_maxstrisidx=cl_numstrisidx+6 + 64;
		cl_strisidx = BZ_Realloc(cl_strisidx, sizeof(*cl_strisidx)*cl_maxstrisidx);
	}
	if (cl_numstrisvert+4 > cl_maxstrisvert)
		cl_stris_ExpandVerts(cl_maxstrisvert+64);

	{
		VectorMA(mid, radius, vright,     cl_strisvertv[cl_numstrisvert]);
		VectorMA(cl_strisvertv[cl_numstrisvert], radius, vup,   cl_strisvertv[cl_numstrisvert]);
		Vector4Set(cl_strisvertc[cl_numstrisvert], r, g, b, 0.2);
		Vector2Set(cl_strisvertt[cl_numstrisvert], 1, 1);
		cl_numstrisvert++;

		VectorMA(mid, radius, vright,  cl_strisvertv[cl_numstrisvert]);
		VectorMA(cl_strisvertv[cl_numstrisvert], -radius, vup, cl_strisvertv[cl_numstrisvert]);
		Vector4Set(cl_strisvertc[cl_numstrisvert], r, g, b, 0.2);
		Vector2Set(cl_strisvertt[cl_numstrisvert], 1, 0);
		cl_numstrisvert++;

		VectorMA(mid, -radius, vright,    cl_strisvertv[cl_numstrisvert]);
		VectorMA(cl_strisvertv[cl_numstrisvert], -radius, vup,  cl_strisvertv[cl_numstrisvert]);
		Vector4Set(cl_strisvertc[cl_numstrisvert], r, g, b, 0.2);
		Vector2Set(cl_strisvertt[cl_numstrisvert], 0, 0);
		cl_numstrisvert++;

		VectorMA(mid, -radius, vright,    cl_strisvertv[cl_numstrisvert]);
		VectorMA(cl_strisvertv[cl_numstrisvert], radius, vup,   cl_strisvertv[cl_numstrisvert]);
		Vector4Set(cl_strisvertc[cl_numstrisvert], r, g, b, 0.2);
		Vector2Set(cl_strisvertt[cl_numstrisvert], 0, 1);
		cl_numstrisvert++;
	}

	/*build the triangles*/
	cl_strisidx[cl_numstrisidx++] = t->numvert + 0;
	cl_strisidx[cl_numstrisidx++] = t->numvert + 1;
	cl_strisidx[cl_numstrisidx++] = t->numvert + 2;

	cl_strisidx[cl_numstrisidx++] = t->numvert + 0;
	cl_strisidx[cl_numstrisidx++] = t->numvert + 2;
	cl_strisidx[cl_numstrisidx++] = t->numvert + 3;


	t->numidx = cl_numstrisidx - t->firstidx;
	t->numvert += 4;
}
#include "shader.h"
void CL_DrawDebugPlane(float *normal, float dist, float r, float g, float b, qboolean enqueue)
{
	const float radius = 8192;	//infinite is quite small nowadays.
	scenetris_t *t;
	if (!enqueue)
		cl_numstris = 0;

	if (cl_numstris == cl_maxstris)
	{
		cl_maxstris+=8;
		cl_stris = BZ_Realloc(cl_stris, sizeof(*cl_stris)*cl_maxstris);
	}
	t = &cl_stris[cl_numstris++];
	t->shader = R_RegisterShader("testplane", SUF_NONE, "{\n{\nmap $whiteimage\nrgbgen vertex\nalphagen vertex\nblendfunc add\nnodepth\n}\n}\n");
	t->firstidx = cl_numstrisidx;
	t->firstvert = cl_numstrisvert;
	t->numvert = 0;
	t->numidx = 0;

	if (cl_numstrisidx+6 > cl_maxstrisidx)
	{
		cl_maxstrisidx=cl_numstrisidx+6 + 64;
		cl_strisidx = BZ_Realloc(cl_strisidx, sizeof(*cl_strisidx)*cl_maxstrisidx);
	}
	if (cl_numstrisvert+4 > cl_maxstrisvert)
		cl_stris_ExpandVerts(cl_maxstrisvert+64);

	{
		vec3_t tmp = {0,0.04,0.96};
		vec3_t right, forward;
		CrossProduct(normal, tmp, right);
		VectorNormalize(right);
		CrossProduct(normal, right, forward);
		VectorNormalize(forward);

		VectorScale(                        normal,    dist,      cl_strisvertv[cl_numstrisvert]);
		VectorMA(cl_strisvertv[cl_numstrisvert], radius, right,     cl_strisvertv[cl_numstrisvert]);
		VectorMA(cl_strisvertv[cl_numstrisvert], radius, forward,   cl_strisvertv[cl_numstrisvert]);
		Vector4Set(cl_strisvertc[cl_numstrisvert], r, g, b, 0.2);
		cl_numstrisvert++;

		VectorScale(                             normal,    dist, cl_strisvertv[cl_numstrisvert]);
		VectorMA(cl_strisvertv[cl_numstrisvert], radius, right,  cl_strisvertv[cl_numstrisvert]);
		VectorMA(cl_strisvertv[cl_numstrisvert], -radius, forward, cl_strisvertv[cl_numstrisvert]);
		Vector4Set(cl_strisvertc[cl_numstrisvert], r, g, b, 0.2);
		cl_numstrisvert++;

		VectorScale(                             normal,    dist, cl_strisvertv[cl_numstrisvert]);
		VectorMA(cl_strisvertv[cl_numstrisvert], -radius, right,    cl_strisvertv[cl_numstrisvert]);
		VectorMA(cl_strisvertv[cl_numstrisvert], -radius, forward,  cl_strisvertv[cl_numstrisvert]);
		Vector4Set(cl_strisvertc[cl_numstrisvert], r, g, b, 0.2);
		cl_numstrisvert++;

		VectorScale(                             normal,    dist, cl_strisvertv[cl_numstrisvert]);
		VectorMA(cl_strisvertv[cl_numstrisvert], -radius, right,    cl_strisvertv[cl_numstrisvert]);
		VectorMA(cl_strisvertv[cl_numstrisvert], radius, forward,   cl_strisvertv[cl_numstrisvert]);
		Vector4Set(cl_strisvertc[cl_numstrisvert], r, g, b, 0.2);
		cl_numstrisvert++;
	}




	/*build the triangles*/
	cl_strisidx[cl_numstrisidx++] = t->numvert + 0;
	cl_strisidx[cl_numstrisidx++] = t->numvert + 1;
	cl_strisidx[cl_numstrisidx++] = t->numvert + 2;

	cl_strisidx[cl_numstrisidx++] = t->numvert + 0;
	cl_strisidx[cl_numstrisidx++] = t->numvert + 2;
	cl_strisidx[cl_numstrisidx++] = t->numvert + 3;


	t->numidx = cl_numstrisidx - t->firstidx;
	t->numvert += 4;

	if (!enqueue)
	{
//		int oldents = cl_numvisedicts;
//		cl_numvisedicts = 0;
		r_refdef.scenevis = NULL;
		BE_DrawWorld(NULL);
		cl_numstris = 0;
//		cl_numvisedicts = oldents;
	}
}
void CLQ1_AddOrientedCube(shader_t *shader, vec3_t mins, vec3_t maxs, float *matrix, float r, float g, float b, float a)
{
	int v;
	scenetris_t *t;
	vec3_t corner;
	int flags = BEF_NODLIGHT|BEF_NOSHADOWS;

	if (!r && !g && !b)
		return;

	/*reuse the previous trigroup if its the same shader*/
	if (cl_numstris && cl_stris[cl_numstris-1].shader == shader && cl_stris[cl_numstris-1].flags == flags && cl_stris[cl_numstris-1].numvert + 8 <= MAX_INDICIES)
		t = &cl_stris[cl_numstris-1];
	else
	{
		if (cl_numstris == cl_maxstris)
		{
			cl_maxstris += 8;
			cl_stris = BZ_Realloc(cl_stris, sizeof(*cl_stris)*cl_maxstris);
		}
		t = &cl_stris[cl_numstris++];
		t->shader = shader;
		t->numidx = 0;
		t->numvert = 0;
		t->firstidx = cl_numstrisidx;
		t->firstvert = cl_numstrisvert;
		t->flags = flags;
	}


	if (cl_numstrisvert + 8 > cl_maxstrisvert)
		cl_stris_ExpandVerts(cl_numstrisvert + 8 + 1024);

	if (cl_maxstrisidx < cl_numstrisidx+6*6)
	{
		cl_maxstrisidx = cl_numstrisidx + 6*6 + 1024;
		cl_strisidx = BZ_Realloc(cl_strisidx, sizeof(*cl_strisidx)*cl_maxstrisidx);
	}


	for (v = 0; v < 8; v++)
	{
		corner[0] = (v & 1)?mins[0]:maxs[0];
		corner[1] = (v & 2)?mins[1]:maxs[1];
		corner[2] = (v & 4)?mins[2]:maxs[2];
		if (matrix)
			Matrix3x4_RM_Transform3(matrix, corner, cl_strisvertv[cl_numstrisvert+v]);
		else
			VectorCopy(corner, cl_strisvertv[cl_numstrisvert+v]);

		cl_strisvertt[cl_numstrisvert+v][0] = 0;
		cl_strisvertt[cl_numstrisvert+v][1] = 0;

		cl_strisvertc[cl_numstrisvert+v][0] = r;
		cl_strisvertc[cl_numstrisvert+v][1] = g;
		cl_strisvertc[cl_numstrisvert+v][2] = b;
		cl_strisvertc[cl_numstrisvert+v][3] = a;
	}

	/*top*/
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+2 - t->firstvert;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+1 - t->firstvert;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+0 - t->firstvert;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+3 - t->firstvert;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+1 - t->firstvert;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+2 - t->firstvert;

	/*bottom*/
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+4 - t->firstvert;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+5 - t->firstvert;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+6 - t->firstvert;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+6 - t->firstvert;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+5 - t->firstvert;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+7 - t->firstvert;

	/*'left'*/
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+5 - t->firstvert;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+4 - t->firstvert;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+0 - t->firstvert;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+1 - t->firstvert;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+5 - t->firstvert;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+0 - t->firstvert;

	/*right*/
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+2 - t->firstvert;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+6 - t->firstvert;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+7 - t->firstvert;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+2 - t->firstvert;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+7 - t->firstvert;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+3 - t->firstvert;

	/*urm, the other way*/
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+2 - t->firstvert;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+4 - t->firstvert;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+6 - t->firstvert;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+4 - t->firstvert;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+2 - t->firstvert;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+0 - t->firstvert;

	/*and its oposite*/
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+7 - t->firstvert;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+5 - t->firstvert;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+3 - t->firstvert;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+1 - t->firstvert;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+3 - t->firstvert;
	cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+5 - t->firstvert;

	t->numvert += 8;
	t->numidx = cl_numstrisidx - t->firstidx;
	cl_numstrisvert += 8;
}
#include "pr_common.h"
void CLQ1_AddVisibleBBoxes(void)
{
	world_t *w;
	wedict_t *e;
	int i;
	shader_t *s;
	vec3_t min, max, size;

	#pragma message("Temporary Code: BBoxes calling R2D_Flush")
	/*
	* HACK(fhomolka): For some reason, bboxes like to mess with progs-drawn Polygons.
	* The clean way would be to understand WHY they mess with eachother, for now this must do.
	* TODO(fhomolka)
	* Comment by Spike: "qc's polys should have been flushed inside renderscene"
	*/
	if(R2D_Flush) R2D_Flush();

	switch(r_showbboxes.ival & 3)
	{
	default:
		return;

	#ifndef CLIENTONLY
	case 1:
		w = &sv.world;
		break;
	#endif
	#ifdef CSQC_DAT
	case 2:
		{
			extern world_t csqc_world;
			w = &csqc_world;
		}
		break;
	#endif
	case 3:
		{
			inframe_t *frame;
			packet_entities_t *pak;
			entity_state_t *state;
			model_t *mod;
			s = R_RegisterShader("bboxshader", SUF_NONE,
				"{\n"
					"polygonoffset\n"
					"sort additive\n"
					"{\n"
						"map $whiteimage\n"
						"blendfunc add\n"
						"rgbgen vertex\n"
						"alphagen vertex\n"
					"}\n"
				"}\n");
			frame = &cl.inframes[cl.parsecount & UPDATE_MASK];
			pak = &frame->packet_entities;

			for (i=0 ; i<pak->num_entities ; i++)
			{
				state = &pak->entities[i];

				if (state->solidsize == ES_SOLID_NOT && !state->skinnum)
					continue;

				if (state->solidsize == ES_SOLID_BSP)
				{	/*bsp model size*/
					if (state->modelindex <= 0)
						continue;
					if (!cl.model_precache[state->modelindex])
						continue;
					/*this makes non-inline bsp objects non-solid for prediction*/
					if ((cls.fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS) || ((*cl.model_precache[state->modelindex]->name == '*' || cl.model_precache[state->modelindex]->numsubmodels) && cl.model_precache[state->modelindex]->hulls[1].firstclipnode))
					{
						mod = cl.model_precache[state->modelindex];
						VectorAdd(state->origin, mod->mins, min);
						VectorAdd(state->origin, mod->maxs, max);
						CLQ1_AddOrientedCube(s, min, max, NULL, 0.1, 0, 0, 1);
					}
				}
				else
				{
					/*don't bother with angles*/
					COM_DecodeSize(state->solidsize, min, max);
					VectorAdd(state->origin, min, min);
					VectorAdd(state->origin, max, max);
					CLQ1_AddOrientedCube(s, min, max, NULL, 0.1, 0, 0, 1);

					COM_DecodeSize(state->solidsize, min, max);
					VectorAdd(state->u.q1.predorg, min, min);
					VectorAdd(state->u.q1.predorg, max, max);
					CLQ1_AddOrientedCube(s, min, max, NULL, 0, 0, 0.1, 1);
				}
			}
		}
		return;
	}

	if (!w->progs)
		return;
	
	s = R_RegisterShader("bboxshader", SUF_NONE,
		"{\n"
			"polygonoffset\n"
			"sort additive\n"
			"{\n"
				"map $whiteimage\n"
				"blendfunc add\n"
				"rgbgen vertex\n"
				"alphagen vertex\n"
			"}\n"
		"}\n");
	for (i = 1; i < w->num_edicts; i++)
	{
		e = WEDICT_NUM_PB(w->progs, i);
		if (ED_ISFREE(e))
			continue;

		if (r_showbboxes.ival & 4)
		{
			//shows the hulls instead

			/*mins is easy*/
			VectorAdd(e->v->origin, e->v->mins, min);

			/*maxs is weeeeird*/
			VectorSubtract (e->v->maxs, e->v->mins, size);
			if (size[0] < 3)
				VectorCopy(min, max);
			else if (size[0] <= 32)
			{
				max[0] = min[0] + 32;
				max[1] = min[1] + 32;
				max[2] = min[2] + 56;
			}
			else
			{
				max[0] = min[0] + 64;
				max[1] = min[1] + 64;
				max[2] = min[2] + 88;
			}
		}
		else
		{
			if (e->v->solid == SOLID_BSP)
			{
				VectorCopy(e->v->absmin, min);
				VectorCopy(e->v->absmax, max);
			}
			else
			{
				VectorAdd(e->v->origin, e->v->mins, min);
				VectorAdd(e->v->origin, e->v->maxs, max);
			}
		}
		if (e->xv->geomtype == GEOMTYPE_CAPSULE)
		{
			float rad = ((e->v->maxs[0]-e->v->mins[0]) + (e->v->maxs[1]-e->v->mins[1]))/4.0;
			float height = (e->v->maxs[2]-e->v->mins[2])/2;
			float matrix[12] = {1,0,0,0,0,1,0,0,0,0,1,0};
			matrix[3] = e->v->origin[0];
			matrix[7] = e->v->origin[1];
			matrix[11] = e->v->origin[2] + (e->v->maxs[2]-height);
			CLQ1_AddOrientedCylinder(s, rad*2, height*2, true, matrix, (e->v->solid || e->v->movetype)?0.1:0, (e->v->movetype == MOVETYPE_STEP || e->v->movetype == MOVETYPE_TOSS || e->v->movetype == MOVETYPE_BOUNCE)?0.1:0, ((int)e->v->flags & (FL_ONGROUND | ((e->v->movetype == MOVETYPE_STEP)?FL_FLY:0)))?0.1:0, 1);
		}
		else
		{
			if (!e->v->solid && !e->v->movetype)
			{
				vec3_t ep = {1,1,1};
				VectorAdd(max, ep, max);
				VectorSubtract(min, ep, min);
				CLQ1_AddOrientedCube(s, min, max, NULL, 0, 0.1, 0, 1);
			}
			else
				CLQ1_AddOrientedCube(s, min, max, NULL, (e->v->solid || e->v->movetype)?0.1:0, (e->v->movetype == MOVETYPE_STEP || e->v->movetype == MOVETYPE_TOSS || e->v->movetype == MOVETYPE_BOUNCE)?0.1:0, ((int)e->v->flags & (FL_ONGROUND | ((e->v->movetype == MOVETYPE_STEP)?FL_FLY:0)))?0.1:0, 1);
		}
	}
}

typedef struct
{
	scenetris_t *t;
	vec4_t rgbavalue;

	vec3_t axis[3];
	float offset[3];
	float scale[3];
} cl_adddecal_ctx_t;
static void CL_AddDecal_Callback(void *vctx, vec3_t *fte_restrict points, size_t numtris, shader_t *shader)
{
	cl_adddecal_ctx_t *ctx = vctx;
	scenetris_t *t = ctx->t;
	size_t numpoints = numtris*3;
	size_t v;

	
	if (cl_numstrisvert + numpoints > cl_maxstrisvert)
		cl_stris_ExpandVerts(cl_numstrisvert + numpoints);
	if (cl_maxstrisidx < cl_numstrisidx+numpoints)
	{
		cl_maxstrisidx = cl_numstrisidx+numpoints + 64;
		cl_strisidx = BZ_Realloc(cl_strisidx, sizeof(*cl_strisidx)*cl_maxstrisidx);
	}


	for (v = 0; v < numpoints; v++)
	{
		VectorCopy(points[v], cl_strisvertv[cl_numstrisvert+v]);
		cl_strisvertt[cl_numstrisvert+v][0] = 1+(DotProduct(points[v], ctx->axis[1]) - ctx->offset[1]) * ctx->scale[1];
		cl_strisvertt[cl_numstrisvert+v][1] = -(DotProduct(points[v], ctx->axis[2]) - ctx->offset[2]) * ctx->scale[2];
		cl_strisvertc[cl_numstrisvert+v][0] = ctx->rgbavalue[0];
		cl_strisvertc[cl_numstrisvert+v][1] = ctx->rgbavalue[1];
		cl_strisvertc[cl_numstrisvert+v][2] = ctx->rgbavalue[2];
		cl_strisvertc[cl_numstrisvert+v][3] = ctx->rgbavalue[3] * (1-fabs(DotProduct(points[v], ctx->axis[0]) - ctx->offset[0]) * ctx->scale[0]);
	}
	for (v = 0; v < numpoints; v++)
	{
		cl_strisidx[cl_numstrisidx++] = cl_numstrisvert+v - t->firstvert;
	}

	t->numvert += numpoints;
	t->numidx += numpoints;
	cl_numstrisvert += numpoints;
}

void CL_AddDecal(shader_t *shader, vec3_t origin, vec3_t up, vec3_t side, vec3_t rgbvalue, float alphavalue)
{
	scenetris_t *t;
	float l, s, radius, vradius;
	cl_adddecal_ctx_t ctx;

	VectorNegate(up, ctx.axis[0]);
	VectorCopy(side, ctx.axis[2]);

	s = DotProduct(ctx.axis[2], ctx.axis[2]);
	l = DotProduct(ctx.axis[0], ctx.axis[0]);
	vradius = 1/sqrt(l);
	radius = 1/sqrt(s);

	VectorScale(ctx.axis[0], vradius, ctx.axis[0]);
	VectorScale(ctx.axis[2], radius, ctx.axis[2]);

	CrossProduct(ctx.axis[0], ctx.axis[2], ctx.axis[1]);

	ctx.offset[2] = DotProduct(origin, ctx.axis[2]) + 0.5*radius;
	ctx.offset[1] = DotProduct(origin, ctx.axis[1]) + 0.5*radius;
	ctx.offset[0] = DotProduct(origin, ctx.axis[0]);

	ctx.scale[2] = 1/radius;
	ctx.scale[1] = 1/radius;
	ctx.scale[0] = 2/vradius;

	if (R2D_Flush)
		R2D_Flush();

	/*reuse the previous trigroup if its the same shader*/
	if (cl_numstris && cl_stris[cl_numstris-1].shader == shader && cl_stris[cl_numstris-1].flags == (BEF_NODLIGHT|BEF_NOSHADOWS))
		t = &cl_stris[cl_numstris-1];
	else
	{
		if (cl_numstris == cl_maxstris)
		{
			cl_maxstris += 8;
			cl_stris = BZ_Realloc(cl_stris, sizeof(*cl_stris)*cl_maxstris);
		}
		t = &cl_stris[cl_numstris++];
		t->shader = shader;
		t->numidx = 0;
		t->numvert = 0;
		t->flags = BEF_NODLIGHT|BEF_NOSHADOWS;
		t->firstidx = cl_numstrisidx;
		t->firstvert = cl_numstrisvert;
	}

	ctx.t = t;
	VectorCopy(rgbvalue, ctx.rgbavalue);
	ctx.rgbavalue[3] = alphavalue;
	Mod_ClipDecal(cl.worldmodel, origin, ctx.axis[0], ctx.axis[1], ctx.axis[2], max(radius, vradius), 0,0, CL_AddDecal_Callback, &ctx);

	if (!t->numidx)
		cl_numstris--;
}

void R_AddItemTimer(vec3_t shadoworg, float yaw, float radius, float percent, vec3_t rgb)
{
	vec3_t eang;
	shader_t *s;
	scenetris_t *t;
	cl_adddecal_ctx_t ctx;

//	if (!r_shadows.value)
//		return;

	s = R_RegisterShader("timershader", SUF_NONE,
		"{\n"
			"polygonoffset\n"
			"fte_program itemtimer\n"
			"{\n"
				"map $diffuse\n"
				"blendfunc src_alpha one\n"
				"rgbgen vertex\n"
				"alphagen vertex\n"
			"}\n"
		"}\n");
	if (!s->prog)
		return;
	TEXASSIGN(s->defaulttextures->base, balltexture);


	eang[0] = 0;
	eang[1] = yaw;
	eang[2] = 0;
	AngleVectors(eang, ctx.axis[1], ctx.axis[2], ctx.axis[0]);
	VectorNegate(ctx.axis[0], ctx.axis[0]);

	ctx.offset[2] = DotProduct(shadoworg, ctx.axis[2]) + 0.5*radius;
	ctx.offset[1] = DotProduct(shadoworg, ctx.axis[1]) + 0.5*radius;
	ctx.offset[0] = DotProduct(shadoworg, ctx.axis[0]);
	ctx.scale[1] = 1/radius;
	ctx.scale[2] = 1/radius;
	ctx.scale[0] = 0;//.5/radius;

	if (R2D_Flush)
		R2D_Flush();

	/*reuse the previous trigroup if its the same shader*/
	if (cl_numstris && cl_stris[cl_numstris-1].shader == s && cl_stris[cl_numstris-1].flags == (BEF_NODLIGHT|BEF_NOSHADOWS))
		t = &cl_stris[cl_numstris-1];
	else
	{
		if (cl_numstris == cl_maxstris)
		{
			cl_maxstris += 8;
			cl_stris = BZ_Realloc(cl_stris, sizeof(*cl_stris)*cl_maxstris);
		}
		t = &cl_stris[cl_numstris++];
		t->shader = s;
		t->flags = BEF_NODLIGHT|BEF_NOSHADOWS;
		t->numidx = 0;
		t->numvert = 0;
		t->firstidx = cl_numstrisidx;
		t->firstvert = cl_numstrisvert;
	}

	ctx.t = t;
	Vector4Set(ctx.rgbavalue, rgb[0], rgb[1], rgb[2], percent);
	Mod_ClipDecal(cl.worldmodel, shadoworg, ctx.axis[0], ctx.axis[1], ctx.axis[2], radius, 0,0, CL_AddDecal_Callback, &ctx);
	if (!t->numidx)
		cl_numstris--;
}
void CLQ1_AddShadow(entity_t *ent)
{
	float radius;
	vec3_t shadoworg;
	vec3_t eang;
	float tx, ty;
	shader_t *s;
	scenetris_t *t;
	cl_adddecal_ctx_t ctx;

	if (!r_blobshadows || !ent->model || (ent->model->type != mod_alias && ent->model->type != mod_halflife) || (ent->flags & RF_NOSHADOW))
		return;

	s = R_RegisterShader("shadowshader", SUF_NONE,
		"{\n"
			"polygonoffset\n"
			"{\n"
				"map $diffuse\n"
				"blendfunc blend\n"
				"rgbgen vertex\n"
				"alphagen vertex\n"
			"}\n"
		"}\n");
	TEXASSIGN(s->defaulttextures->base, balltexture);

	tx = ent->model->maxs[0] - ent->model->mins[0];
	ty = ent->model->maxs[1] - ent->model->mins[1];

	if (tx > ty)
		radius = tx;
	else
		radius = ty;
	radius/=2;

	shadoworg[0] = ent->origin[0];
	shadoworg[1] = ent->origin[1];
	shadoworg[2] = ent->origin[2] + ent->model->mins[2];

	eang[0] = 0;
	eang[1] = ent->angles[1];
	eang[2] = 0;
	AngleVectors(eang, ctx.axis[1], ctx.axis[2], ctx.axis[0]);
	VectorNegate(ctx.axis[0], ctx.axis[0]);

	ctx.offset[2] = DotProduct(shadoworg, ctx.axis[2]) + 0.5*radius;
	ctx.offset[1] = DotProduct(shadoworg, ctx.axis[1]) + 0.5*radius;
	ctx.offset[0] = DotProduct(shadoworg, ctx.axis[0]);
	ctx.scale[1] = 1/radius;
	ctx.scale[2] = 1/radius;
	ctx.scale[0] = 0.5/radius;

	if (R2D_Flush)
		R2D_Flush();

	/*reuse the previous trigroup if its the same shader*/
	if (cl_numstris && cl_stris[cl_numstris-1].shader == s && cl_stris[cl_numstris-1].flags == (BEF_NODLIGHT|BEF_NOSHADOWS))
		t = &cl_stris[cl_numstris-1];
	else
	{
		if (cl_numstris == cl_maxstris)
		{
			cl_maxstris += 8;
			cl_stris = BZ_Realloc(cl_stris, sizeof(*cl_stris)*cl_maxstris);
		}
		t = &cl_stris[cl_numstris++];
		t->shader = s;
		t->flags = BEF_NODLIGHT|BEF_NOSHADOWS;
		t->numidx = 0;
		t->numvert = 0;
		t->firstidx = cl_numstrisidx;
		t->firstvert = cl_numstrisvert;
	}

	ctx.t = t;
	Vector4Set(ctx.rgbavalue, 0, 0, 0, r_blobshadows*((ent->flags & RF_TRANSLUCENT)?ent->shaderRGBAf[3]:1));
	Mod_ClipDecal(cl.worldmodel, shadoworg, ctx.axis[0], ctx.axis[1], ctx.axis[2], radius, 0,0, CL_AddDecal_Callback, &ctx);
	if (!t->numidx)
		cl_numstris--;
}
void CLQ1_AddPowerupShell(entity_t *ent, qboolean viewweap, unsigned int effects)
{
	entity_t *shell;
	if (!(effects & (EF_BLUE | EF_RED | EF_GREEN)) || !v_powerupshell.value || !ent)
		return;

	if (cl_numvisedicts == cl_maxvisedicts)
		return;		// object list is full
	shell = &cl_visedicts[cl_numvisedicts++];

	*shell = *ent;

	/*view weapons are much closer to the screen, the scales don't work too well, so use a different shader with a smaller expansion*/
	if (viewweap)
	{
		shell->forcedshader = R_RegisterShader("powerups/shellweapon", SUF_NONE,
				"{\n"
					"program defaultpowerupshell\n"
					"sort additive\n"
					"deformVertexes wave 100 sin 0.5 0 0 0\n"
					"noshadows\n"
					"surfaceparm nodlight\n"
					"{\n"
						"map $whiteimage\n"
						"rgbgen entity\n"
						"alphagen entity\n"
						"blendfunc src_alpha one\n"
					"}\n"
				"}\n"
			);
	}
	else
	{
		shell->forcedshader = R_RegisterShader("powerups/shell", SUF_NONE,
				"{\n"
					"program defaultpowerupshell\n"
					"sort additive\n"
					"deformVertexes wave 100 sin 3 0 0 0\n"
					"noshadows\n"
					"surfaceparm nodlight\n"
					"{\n"
						"map $whiteimage\n"
						"rgbgen entity\n"
						"alphagen entity\n"
						"blendfunc src_alpha one\n"
					"}\n"
				"}\n"
			);
	}
	shell->shaderRGBAf[0] *= (effects & EF_RED)?1:0;
	shell->shaderRGBAf[1] *= (effects & EF_GREEN)?1:0;
	shell->shaderRGBAf[2] *= (effects & EF_BLUE)?1:0;
	shell->shaderRGBAf[3] *= v_powerupshell.value;
	/*let the shader do all the work*/
	shell->flags &= ~RF_TRANSLUCENT|RF_ADDITIVE;
}

static void CL_LerpNetFrameState(framestate_t *fs, lerpents_t *le)
{
	int fsanim;
	for (fsanim = 0; fsanim < FS_COUNT; fsanim++)
	{
		fs->g[fsanim].frame[0] = le->newframe[fsanim];
		fs->g[fsanim].frame[1] = le->oldframe[fsanim];

		fs->g[fsanim].frametime[0] = cl.servertime - le->newframestarttime[fsanim];
		fs->g[fsanim].frametime[1] = cl.servertime - le->oldframestarttime[fsanim];

		fs->g[fsanim].lerpweight[0] = (fs->g[fsanim].frametime[0]) / le->framelerpdeltatime[fsanim];
		fs->g[fsanim].lerpweight[0] = bound(0, fs->g[FS_REG].lerpweight[0], 1);
		fs->g[fsanim].lerpweight[1] = 1 - fs->g[fsanim].lerpweight[0];
	}
	fs->g[0].endbone = le->basebone;
}

static void CL_UpdateNetFrameLerpState(qboolean force, int curframe, int curbaseframe, int curbasebone, lerpents_t *le, float lerpend)
{
	int fst, frame;
	if (curbasebone != le->basebone)
	{
		//FIXME: we should be able to treat 0 and 255 specially by ignoring the change and locking the respective value to the other's value.
		if (!curbasebone)
			curbaseframe = curframe;
		else if (curbasebone == 255)
			curframe = curbaseframe;
		le->basebone = curbasebone;
	}
	for (fst = 0; fst < FS_COUNT; fst++)
	{
		frame = (fst==FST_BASE)?curbaseframe:curframe;
		if (force || frame != le->newframe[fst])
		{
			if (lerpend)
				le->framelerpdeltatime[fst] = bound(0, lerpend - cl.servertime, cl_lerp_maxinterval.value);	//clamp to 10 tics per second
			else
				le->framelerpdeltatime[fst] = bound(0, cl.servertime - le->newframestarttime[fst], cl_lerp_maxinterval.value);	//clamp to 10 tics per second

			if (!force)
			{
				le->oldframe[fst] = le->newframe[fst];
				le->oldframestarttime[fst] = le->newframestarttime[fst];
			}
			else
			{
				le->oldframe[fst] = frame;
				le->oldframestarttime[fst] = cl.servertime;
			}
			le->newframe[fst] = frame;
			le->newframestarttime[fst] = cl.servertime;

//			if (force)
//			{
//				//if its new, we need to tweak the age of the animation. looping anims won't appear any different, while non-looping ones will clamp to the last pose of the animation when its new.
//				le->oldframestarttime[fst] -= Mod_GetFrameDuration(le->model, 0, le->oldframe[fst]);
//				le->newframestarttime[fst] -= Mod_GetFrameDuration(le->model, 0, le->newframe[fst]);
//			}
		}
	}
}

void CL_ClearLerpEntsParticleState(void)
{
	int i;
	for (i = 0; i < cl.maxlerpents; i++)
	{
		pe->DelinkTrailstate(&(cl.lerpents[i].trailstate));
		pe->DelinkTrailstate(&(cl.lerpents[i].emitstate));
	}
}

void CL_LinkStaticEntities(void *pvs, int *areas)
{
	int i;
	entity_t *ent;
	model_t		*clmodel;
	static_entity_t *stat;
	extern cvar_t r_drawflame, gl_part_flame;
	vec3_t mins, maxs;

	if (r_drawflame.ival < 0 || r_drawentities.ival == 0)
		return;

	if (!cl.worldmodel)
		return;

	for (i = 0; i < cl.num_statics; i++)
	{
		if (cl_numvisedicts == cl_maxvisedicts)
		{
			cl_expandvisents=true;
			break;
		}
		stat = &cl_static_entities[i];

		clmodel = stat->ent.model;

		if (!clmodel)
		{
			if (stat->mdlidx < 0)
			{
				if (stat->mdlidx > -MAX_CSMODELS)
					clmodel = cl.model_csqcprecache[-stat->mdlidx];
			}
			else
			{
				if (stat->mdlidx < MAX_PRECACHE_MODELS)
					clmodel = cl.model_precache[stat->mdlidx];
			}
			if (!clmodel || clmodel->loadstate == MLS_LOADING)
				continue;
			if (!cl.worldmodel || cl.worldmodel->loadstate != MLS_LOADED)
				continue;

			stat->ent.model = clmodel;

			//figure out the correct axis for the model
			if (clmodel && clmodel->type == mod_alias && (cls.protocol == CP_QUAKEWORLD || cls.protocol == CP_NETQUAKE))
			{	//q2 is fixed, but q1 pitches the wrong way, and hexen2 rolls the wrong way too.
				AngleVectorsMesh(stat->state.angles, stat->ent.axis[0], stat->ent.axis[1], stat->ent.axis[2]);
			}
			else
				AngleVectors(stat->state.angles, stat->ent.axis[0], stat->ent.axis[1], stat->ent.axis[2]);
			VectorInverse(stat->ent.axis[1]);


			if (clmodel)
			{
				//FIXME: wait for model to load so we know the correct size?
				/*FIXME: compensate for angle*/
				VectorAdd(stat->state.origin, clmodel->mins, mins);
				VectorAdd(stat->state.origin, clmodel->maxs, maxs);
			}
			else
			{
				VectorCopy(stat->state.origin, mins);
				VectorCopy(stat->state.origin, maxs);
			}
			cl.worldmodel->funcs.FindTouchedLeafs(cl.worldmodel, &stat->ent.pvscache, mins, maxs);
		}
		else if (clmodel->loadstate != MLS_LOADED)
		{
			if (clmodel->loadstate == MLS_NOTLOADED)	//flushed?
				Mod_LoadModel(clmodel, MLV_WARN);		//load it, but don't otherwise care for now.
			continue;
		}

		/*pvs test*/
		if (pvs && !cl.worldmodel->funcs.EdictInFatPVS(cl.worldmodel, &stat->ent.pvscache, pvs, areas))
			continue;


		// emit particles for statics (we don't need to cheat check statics)
		if (stat->state.u.q1.emiteffectnum)
			P_EmitEffect (stat->ent.origin, stat->ent.axis, MDLF_EMITFORWARDS, CL_TranslateParticleFromServer(stat->state.u.q1.emiteffectnum), &(stat->emit));
		else if (clmodel)
		{
			if (clmodel->particleeffect >= 0 && gl_part_flame.ival)
				P_EmitEffect(stat->ent.origin, stat->ent.axis, clmodel->engineflags, clmodel->particleeffect, &stat->emit);
			if ((!r_drawflame.ival) && (clmodel->engineflags & MDLF_FLAME))
				continue;
		}

		//prepare to draw it
		if (!clmodel || clmodel->loadstate != MLS_LOADED)
			continue;

		ent = &cl_visedicts[cl_numvisedicts++];
		*ent = stat->ent;
		ent->framestate.g[FS_REG].frametime[0] = cl.time;
		ent->framestate.g[FS_REG].frametime[1] = cl.time;

//  FIXME: no effects on static ents
//		CLQ1_AddPowerupShell(ent, false, stat->effects);
	}
}

//returns cos(angle)
static float CompareAngles (const vec3_t angles1, const vec3_t angles2)
{
	float		angle;
	vec3_t dir1, dir2;

	angle = angles1[YAW] * (M_PI*2 / 360);
	dir1[1] = sin(angle);
	dir1[0] = cos(angle);
	if (angles1[PITCH])
	{
		angle = angles1[PITCH] * (M_PI*2 / 360);
		dir1[2] = -sin(angle);
		angle = cos(angle);
		dir1[0] *= angle;
		dir1[1] *= angle;
	}
	else
		dir1[2] = 0;

	angle = angles2[YAW] * (M_PI*2 / 360);
	dir2[1] = sin(angle);
	dir2[0] = cos(angle);
	if (angles2[PITCH])
	{
		angle = angles2[PITCH] * (M_PI*2 / 360);
		dir2[2] = -sin(angle);
		angle = cos(angle);
		dir2[0] *= angle;
		dir2[1] *= angle;
	}
	else
		dir2[2] = 0;

	return DotProduct(dir1,dir2);
}

/*
===============
CL_LinkPacketEntities

===============
*/
void R_FlameTrail(vec3_t start, vec3_t end, float seperation);

/*
Interpolates the two packets by the given time, writes its results into the lerpentities array.
*/
static void CL_TransitionPacketEntities(int newsequence, packet_entities_t *newpack, packet_entities_t *oldpack, float frac, float servertime)
{
	lerpents_t		*le;
	entity_state_t		*snew, *sold;
	int					i;
	int					oldpnum, newpnum;
	float				*snew__origin;
	float				*sold__origin;
	float				cos_theta;
	int oldsequence;
	extern cvar_t r_nolerp;

	qboolean			isnew;

	vec3_t move;

	float a1, a2;
	float maxdist = cl_lerp_maxdistance.value*cl_lerp_maxdistance.value;

	/*
		seeing as how dropped packets cannot be filled in due to the reliable networking stuff,
		We can simply detect changes and lerp towards them
	*/

	//we have two index-sorted lists of entities
	//we figure out which ones are new,
	//we don't care about old, as our caller will use the lerpents array we fill, and the entity numbers from the 'new' packet.

	oldsequence = cl.lerpentssequence;
	if (!oldsequence)
		oldsequence = -1;	//something invalid, so everything is new
	cl.lerpentssequence = newsequence;

	cl.packfrac = frac;
	cl.currentpacktime = servertime;
	cl.currentpackentities = newpack;
	cl.previouspackentities = oldpack;

	oldpnum=0;
	for (newpnum=0 ; newpnum<newpack->num_entities ; newpnum++)
	{
		snew = &newpack->entities[newpnum];

		sold = NULL;
		for ( ; oldpnum<oldpack->num_entities ; )
		{
			sold = &oldpack->entities[oldpnum];
			if (sold->number >= snew->number)
			{
				if (sold->number > snew->number)
					sold = NULL;	//woo, it's a new entity.
				else
					oldpnum++;
				break;
			}
			oldpnum++;

#ifdef RAGDOLL
			//note: not entirely reliable
			le = &cl.lerpents[sold->number];
			if (sold->number < cl.maxlerpents && le->skeletalobject)
				rag_removedeltaent(le);
#endif
		}

		if (snew->number >= cl.maxlerpents)
		{
			int newmaxle = snew->number+16;
			cl.lerpents = BZ_Realloc(cl.lerpents, newmaxle*sizeof(lerpents_t));
			memset(cl.lerpents + cl.maxlerpents, 0, sizeof(lerpents_t)*(newmaxle - cl.maxlerpents));
			cl.maxlerpents = newmaxle;
		}

		if (!sold)
		{
			isnew = true;
			sold = snew;	//don't crash if anything tries poking sold
		}
		else
			isnew = false;

		le = &cl.lerpents[snew->number];
		if (le->sequence != oldsequence)
			isnew = true;
		le->sequence = newsequence;
		le->entstate = snew;

		if (snew->u.q1.pmovetype)
		{
			if (!cl.do_lerp_players)
			{
				entity_state_t *from;
				float age;
				packet_entities_t *latest;
				if (isnew)
				{
					/*keep trails correct*/
					le->isnew = true;
					VectorCopy(le->origin, le->lastorigin);
				}
				CL_UpdateNetFrameLerpState(sold == snew, snew->frame, snew->baseframe, snew->basebone, le, snew->lerpend);


				from = sold;	//eww
				age = servertime - oldpack->servertime;
				latest = &cl.inframes[cl.validsequence & UPDATE_MASK].packet_entities;
				for (i = 0; i < latest->num_entities; i++)
				{
					if (latest->entities[i].number == snew->number)
					{
						from = &latest->entities[i];
						//use realtime instead.
						//also, use the sent timings instead of received as those are assumed to be more reliable
						age = (realtime - cl.outframes[cl.ackedmovesequence & UPDATE_MASK].senttime) - cls.latency*cl_predict_players_latency.value + cl_predict_players_nudge.value;
						break;
					}
				}
				if (age > 1)
					age = 1;

				if (cl_predict_players.ival && pmove.numphysent)
				{
					CL_PredictEntityMovement(from, age);
					VectorCopy(from->u.q1.predorg, le->origin);
				}
				else
					VectorCopy(from->origin, le->origin);
				VectorCopy(from->angles, le->angles);
				continue;
			}

			//FIXME: find a packet where this entity changed.

			snew__origin = snew->u.q1.predorg;
			sold__origin = sold->u.q1.predorg;
			cos_theta = 1;	//don't cut off lerping when the player spins too fast.
		}
		else
		{
			snew__origin = snew->origin;
			sold__origin = sold->origin;
			cos_theta = CompareAngles(sold->angles, snew->angles);
		}

		VectorSubtract(snew__origin, sold__origin, move);
		if (DotProduct(move, move) > maxdist || cos_theta < 0.707 || snew->modelindex != sold->modelindex || ((sold->effects ^ snew->effects) & EF_TELEPORT_BIT))
		{
			isnew = true;	//disable lerping (and indirectly trails)
//			VectorClear(move);
		}

		VectorCopy(le->origin, le->lastorigin);
		if (isnew)
		{
#ifdef RAGDOLL	//make sure nothing gets stale
			if (le->skeletalobject)
				rag_removedeltaent(le);
#endif

			le->newsequence = snew->sequence;

			//new this frame (or we noticed something changed significantly)
			VectorCopy(snew__origin, le->origin);
			VectorCopy(snew->angles, le->angles);

			VectorCopy(snew__origin, le->oldorigin);
			VectorCopy(snew->angles, le->oldangle);
			VectorCopy(snew__origin, le->neworigin);
			VectorCopy(snew->angles, le->newangle);

			if (snew->lerpend)
				le->orglerpdeltatime = bound(0.001, snew->lerpend - newpack->servertime, cl_lerp_maxinterval.value);
			else
				le->orglerpdeltatime = newpack->servertime - oldpack->servertime;
			le->orglerpstarttime = oldpack->servertime;

			le->isnew = true;
			VectorCopy(le->origin, le->lastorigin);
		}
		else
		{
			if ((sold->effects ^ snew->effects) & EF_RESTARTANIM_BIT)
				isnew = true;

			if (snew->dpflags & RENDER_STEP)
			{
				float lfrac;
				//ignore the old packet entirely, except for maybe its time.
				if (!VectorEquals(le->neworigin, snew__origin) || !VectorEquals(le->newangle, snew->angles))
				{
					le->newsequence = snew->sequence;
					le->orglerpdeltatime = bound(0, oldpack->servertime - le->orglerpstarttime, cl_lerp_maxinterval.value);	//clamp to 10 tics per second
					le->orglerpstarttime = oldpack->servertime;

					VectorCopy(le->neworigin, le->oldorigin);
					VectorCopy(le->newangle, le->oldangle);
 
					VectorCopy(snew__origin, le->neworigin);
					VectorCopy(snew->angles, le->newangle);
				}

				if (snew->lerpend)
					le->orglerpdeltatime = bound(0.001, snew->lerpend - le->orglerpstarttime, cl_lerp_maxinterval.value);
				lfrac = (servertime - le->orglerpstarttime) / le->orglerpdeltatime;
				lfrac = bound(0, lfrac, 1);
				if (r_nolerp.ival)
				{
					lfrac = 1;
					isnew = true;
				}
				for (i = 0; i < 3; i++)
				{
					le->origin[i] = le->oldorigin[i] + lfrac*(le->neworigin[i] - le->oldorigin[i]);

					a1 = le->oldangle[i];
					a2 = le->newangle[i];
					if (a1 - a2 > 180)
						a1 -= 360;
					if (a1 - a2 < -180)
						a1 += 360;
					le->angles[i] = a1 + lfrac * (a2 - a1);
				}
			}
			else
			{
				float lfrac;

				if (le->newsequence != snew->sequence)
				{
					le->newsequence = snew->sequence;
					VectorCopy(le->neworigin, le->oldorigin);
					VectorCopy(le->newangle, le->oldangle);
					VectorCopy(snew__origin, le->neworigin);
					VectorCopy(snew->angles, le->newangle);

					//fixme: should be oldservertime
					le->orglerpdeltatime = bound(0.001, servertime-le->orglerpstarttime, cl_lerp_maxinterval.value);
					le->orglerpstarttime = servertime;
				}

				if (snew->lerpend)
					le->orglerpdeltatime = bound(0.001, snew->lerpend - le->orglerpstarttime, cl_lerp_maxinterval.value);
				lfrac = (servertime - le->orglerpstarttime) / le->orglerpdeltatime;
				lfrac = bound(0, lfrac, 1);

				//lerp based purely on the packet times,
				for (i = 0; i < 3; i++)
				{
					le->origin[i] = le->oldorigin[i] + lfrac*(le->neworigin[i] - le->oldorigin[i]);

					a1 = le->oldangle[i];
					a2 = le->newangle[i];
					if (a1 - a2 > 180)
						a1 -= 360;
					if (a1 - a2 < -180)
						a1 += 360;
					le->angles[i] = a1 + lfrac * (a2 - a1);
				}
			}
		}

#ifdef RAGDOLL //this preprocessor is misnamed, but oh well
		if (snew->bonecount)
		{
			void *newbones = GetBoneSpace(newpack, snew->boneoffset);
			if (sold && snew->bonecount == sold->bonecount)
				rag_lerpdeltaent(le, snew->bonecount, newbones, r_nolerp.ival?1:frac, GetBoneSpace(oldpack, sold->boneoffset));
			else
				rag_lerpdeltaent(le, snew->bonecount, newbones, 1, newbones);
		}
#endif

		CL_UpdateNetFrameLerpState(isnew, snew->frame, snew->baseframe, snew->basebone, le, snew->lerpend);
	}
}

static qboolean CL_ChooseInterpolationFrames(int *newf, int *oldf, float servertime)
{
	int i;
	float newtime = 0;
	*oldf = -1;
	*newf = -1;

	//choose the two packets.
	//we should be picking the packet just after the server time, and the one just before
	for (i = cls.netchan.incoming_sequence; i >= cls.netchan.incoming_sequence-UPDATE_MASK; i--)
	{
		if (cl.inframes[i&UPDATE_MASK].frameid != i || cl.inframes[i&UPDATE_MASK].invalid)
			continue;	//packetloss/choke, it's really only a problem for the oldframe, but...

		if (cl.inframes[i&UPDATE_MASK].packet_entities.servertime >= servertime)
		{
			if (cl.inframes[i&UPDATE_MASK].packet_entities.servertime)
			{
				if (!newtime || newtime != cl.inframes[i&UPDATE_MASK].packet_entities.servertime)	//if it's a duplicate, pick the latest (so just-shot rockets are still present)
				{
					newtime = cl.inframes[i&UPDATE_MASK].packet_entities.servertime;
					*newf = i;
				}
			}
		}
		else if (newtime)
		{
			if (cl.inframes[i&UPDATE_MASK].packet_entities.servertime != newtime)
			{	//it does actually lerp, and isn't an identical frame.
				*oldf = i;
				break;
			}
		}
	}

	if (*newf == -1)
	{
		/*
		This can happen if the client's predicted time is greater than the most recently received packet.
		This should of course not happen...
		*/
//		Con_DPrintf("Warning: No lerp-to frame packet\n");

		/*just grab the most recent frame that is valid*/
		for (i = cls.netchan.incoming_sequence; i >= cls.netchan.incoming_sequence-UPDATE_MASK; i--)
		{
			if (cl.inframes[i&UPDATE_MASK].frameid != i || cl.inframes[i&UPDATE_MASK].invalid)
				continue;	//packetloss/choke, it's really only a problem for the oldframe, but...
			*oldf = *newf = i;
			return true;
		}
		return false;
	}
	else if (*oldf == -1)	//can happen at map start, and really laggy games, but really shouldn't in a normal game
	{
		*oldf = *newf;
	}
	return true;
}

qboolean CL_MayLerp(void)
{
	//force lerping when playing low-framerate demos.
	if (cls.demoplayback == DPB_MVD)
		return true;
#ifdef NQPROT
	if (cls.demoplayback == DPB_NETQUAKE)
		return true;

	if (cls.protocol == CP_NETQUAKE)	//this includes DP protocols.
		return !cl_nolerp_netquake.ival;
#endif
	if (cl_nolerp.ival == 2 && !cls.deathmatch)
		return true;
	return !cl_nolerp.ival;
}

/*fills in cl.lerpents and cl.currentpackentities*/
void CL_TransitionEntities (void)
{
	packet_entities_t	*packnew, *packold;
	int newf, newff, oldf, i;
	qboolean nolerp;
	float servertime, frac;

	if (cls.protocol == CP_QUAKEWORLD && cls.demoplayback == DPB_MVD)
	{
		nolerp = false;
	}
	else
	{
		nolerp = !CL_MayLerp() && cls.demoplayback != DPB_MVD;
	}

	if (cl.demonudge < 0)
	{	//demo playback allows nudging to earlier frames, generally only when paused though...
		servertime = cl.inframes[(cls.netchan.incoming_sequence+cl.demonudge)&UPDATE_MASK].packet_entities.servertime;
		nolerp = true;
	}
	else if (nolerp)
	{
		//force our emulated time to as late as we can, if we're not using interpolation, which has the effect of disabling all interpolation
		servertime = cl.inframes[cls.netchan.incoming_sequence&UPDATE_MASK].packet_entities.servertime;
	}
	else
	{
		//otherwise go for the latest frame we can.
		servertime = cl.servertime;
	}

//	servertime -= 0.1;

	/*make sure we have some info for it, on failure keep the info from the last frame (its possible that the frame data can be changed by a network packet, but mneh, but chances are if there's no info then there are NO packets at all)*/
	if (!CL_ChooseInterpolationFrames(&newf, &oldf, servertime))
		return;

	newff = newf;
	newf&=UPDATE_MASK;
	oldf&=UPDATE_MASK;
	/*transition the ents and stuff*/
	packnew = &cl.inframes[newf].packet_entities;
	packold = &cl.inframes[oldf].packet_entities;
	if (packnew->servertime == packold->servertime)
		frac = 1; //lerp totally into the new (avoid any division-by-0 issues here)
	else
		frac = (servertime-packold->servertime)/(packnew->servertime-packold->servertime);

//	if (!cl.paused)
//		Con_DPrintf("%f %s%f^7 %f (%f) (%i) %f %s%f^7 %f\n", packold->servertime, (servertime<packold->servertime||packnew->servertime<servertime)?"^1":"",servertime, packnew->servertime, frac, newff, cl.oldgametime, (servertime<cl.oldgametime||cl.gametime<servertime)?"^3":"", servertime, cl.gametime);

	CL_TransitionPacketEntities(newff, packnew, packold, frac, servertime);

	for (i = 0; i < cl.splitclients; i++)
	{
		VectorInterpolate(packold->punchangle[i], frac, packnew->punchangle[i], cl.playerview[i].punchangle_sv);
		VectorInterpolate(packold->punchorigin[i], frac, packnew->punchorigin[i], cl.playerview[i].punchorigin);
	}


	/*and transition players too*/
	{
		float frac, a1, a2;
		int i, p;
		vec3_t move;
		lerpents_t *le;
		player_state_t *pnew, *pold;
		if (!cl.do_lerp_players)
		{
			newf = newff = oldf = cl.parsecount;
			newf&=UPDATE_MASK;
			oldf&=UPDATE_MASK;
		}
		if (packnew->servertime == packold->servertime)
			frac = 1; //lerp totally into the new
		else
			frac = (servertime-packold->servertime)/(packnew->servertime-packold->servertime);
		pnew = &cl.inframes[newf].playerstate[0];
		pold = &cl.inframes[oldf].playerstate[0];
		for (p = 0; p < cl.allocated_client_slots; p++, pnew++, pold++)
		{
			if (pnew->messagenum != newff)
			{
				continue;
			}
		
			le = &cl.lerpplayers[p];
			VectorSubtract(pnew->predorigin, pold->predorigin, move);

			if (DotProduct(move, move) > 120*120)
				frac = 1;

			//lerp based purely on the packet times,
			for (i = 0; i < 3; i++)
			{
				le->origin[i] = pold->predorigin[i] + frac*(move[i]);

				a1 = SHORT2ANGLE(pold->command.angles[i]);
				a2 = SHORT2ANGLE(pnew->command.angles[i]);
				if (a1 - a2 > 180)
					a1 -= 360;
				if (a1 - a2 < -180)
					a1 += 360;
				le->angles[i] = a1 + frac * (a2 - a1);
			}
			le->orglerpdeltatime = 0.1;
			le->orglerpstarttime = packold->servertime;
		}
	}
}

void CL_LinkPacketEntities (void)
{
	extern cvar_t gl_part_flame;
	entity_t			*ent;
	packet_entities_t	*pack;
	entity_state_t		*state;
	lerpents_t		*le;
	model_t				*model, *model2;
	vec3_t				old_origin;
	float				autorotate;
	int					i;
	int					newpnum;
	//, spnum;
	dlight_t			*dl;
	vec3_t				angles;
	static int flickertime;
	static int flicker;
	int trailef, trailidx;
	int modelflags;
	struct itemtimer_s	*timer, **timerlink;
	float timestep = cl.time-cl.lastlinktime;
	extern cvar_t r_ignoreentpvs;
	vec3_t absmin, absmax;
	cl.lastlinktime = cl.time;
	timestep = bound(0, timestep, 0.1);

	pack = cl.currentpackentities;
	if (!pack)
		return;

	i = cl.currentpacktime*20;
	if (flickertime != i)
	{
		flickertime = i;
		flicker = rand();
	}

	autorotate = anglemod(100*cl.currentpacktime);

#ifdef CSQC_DAT
	CSQC_DeltaStart(cl.currentpacktime);
#endif


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
/*			if (timer->entnum>0)
			{
				if (timer->entnum < cl.maxlerpents)
				{
					le = &cl.lerpents[timer->entnum];
					if (le->sequence == cl.lerpentssequence)
						VectorCopy(le->origin, timer->origin);
				}
			}*/
			R_AddItemTimer(timer->origin, cl.time*90 + timer->origin[0] + timer->origin[1] + timer->origin[2], timer->radius, (cl.time - timer->start) / timer->duration, timer->rgb);
		}
	}

	for (newpnum=0 ; newpnum<pack->num_entities ; newpnum++)
	{
		state = &pack->entities[newpnum];

#ifdef CSQC_DAT
		if (CSQC_DeltaUpdate(state))
			continue;
#endif

		if (cl_numvisedicts == cl_maxvisedicts)
			break;

		if (state->number >= cl.maxlerpents)
			continue;

		le = &cl.lerpents[state->number];

		ent = &cl_visedicts[cl_numvisedicts];

		ent->rtype = RT_MODEL;
		ent->playerindex = -1;
		ent->customskin = 0;
		ent->topcolour = TOP_DEFAULT;
		ent->bottomcolour = BOTTOM_DEFAULT;
#ifdef HEXEN2
		ent->h2playerclass = 0;
#endif
		ent->light_known = 0;
		ent->forcedshader = NULL;
		ent->shaderTime = 0;

		memset(&ent->framestate, 0, sizeof(ent->framestate));

		VectorCopy(le->origin, ent->origin);

		//bots or powerup glows. items always glow, bots can be disabled
		if (state->modelindex != cl_playerindex || r_powerupglow.ival)
		if (state->effects & (EF_GREEN | EF_BLUE | EF_RED | EF_BRIGHTLIGHT | EF_DIMLIGHT))
		{
			vec3_t colour;
			float radius;
			colour[0] = 0;
			colour[1] = 0;
			colour[2] = 0;
			radius = 0;

			if (state->effects & EF_BRIGHTLIGHT)
			{
				radius = max(radius,r_brightlight_colour.vec4[3]);
				if (!(state->effects & (EF_RED|EF_GREEN|EF_BLUE)))
					VectorAdd(colour, r_brightlight_colour.vec4, colour);
			}
			if (state->effects & EF_DIMLIGHT)
			{
				radius = max(radius,r_dimlight_colour.vec4[3]);
				if (!(state->effects & (EF_RED|EF_GREEN|EF_BLUE)))
					VectorAdd(colour, r_dimlight_colour.vec4, colour);
			}
			if (state->effects & EF_BLUE)
			{
				radius = max(radius,r_bluelight_colour.vec4[3]);
				VectorAdd(colour, r_bluelight_colour.vec4, colour);
			}
			if (state->effects & EF_RED)
			{
				radius = max(radius,r_redlight_colour.vec4[3]);
				VectorAdd(colour, r_redlight_colour.vec4, colour);
			}
			if (state->effects & EF_GREEN)
			{
				radius = max(radius,r_greenlight_colour.vec4[3]);
				VectorAdd(colour, r_greenlight_colour.vec4, colour);
			}

			if (radius)
			{
				radius += r_lightflicker.value?((flicker + state->number)&31):0;
				dl = CL_NewDlight(state->number, ent->origin, radius, 0.1, colour[0], colour[1], colour[2]);

				if (state->effects & EF_BRIGHTLIGHT)
				{	//urgh. apparently correct for vanilla quake. puts the bright effect about where the firing point is. broken for hexen2, yet still consistent with the hexen2 engine...
					dl->origin[2] += 16;
				}
			}
		}
		if ((state->lightpflags & (PFLAGS_FULLDYNAMIC|PFLAGS_CORONA)) && ((state->lightpflags&PFLAGS_FULLDYNAMIC)||state->light[3]))
		{
			vec3_t colour;
			if (!state->light[0] && !state->light[1] && !state->light[2])
			{
				colour[0] = colour[1] = colour[2] = 1;
			}
			else
			{
				colour[0] = state->light[0]/1024.0f;
				colour[1] = state->light[1]/1024.0f;
				colour[2] = state->light[2]/1024.0f;
			}
			dl = CL_NewDlight(state->number, ent->origin, state->light[3]?state->light[3]:350, 0.1, colour[0], colour[1], colour[2]);
			if (!(state->lightpflags & PFLAGS_FULLDYNAMIC))	//corona-only lights shouldn't do much else.
			{
				dl->flags &= ~(LFLAG_LIGHTMAP|LFLAG_FLASHBLEND);
#ifdef RTLIGHTS
				/*make sure there's no rtlight*/
				memset(dl->lightcolourscales, 0, sizeof(dl->lightcolourscales));
#endif
			}
			dl->corona = (state->lightpflags & PFLAGS_CORONA)?1:0;
			dl->coronascale = 0.25;
			dl->style = state->lightstyle;
			dl->flags &= ~LFLAG_FLASHBLEND;
			dl->flags |= (state->lightpflags & PFLAGS_NOSHADOW)?LFLAG_NOSHADOWS:0;
#ifdef RTLIGHTS
			if (state->skinnum)
			{
				VectorCopy(le->angles, angles);
				//if (model && model->type == mod_alias)
					AngleVectorsMesh(angles, dl->axis[0], dl->axis[1], dl->axis[2]);	//pflags matches alias models.
				//else
				//	AngleVectors(angles, dl->axis[0], dl->axis[1], dl->axis[2]);
				VectorInverse(dl->axis[1]);
				R_LoadNumberedLightTexture(dl, state->skinnum);
			}
#endif
		}

		if (r_torch.ival && state->number <= cl.allocated_client_slots)
		{
			dlight_t *dl;
			dl = CL_NewDlight(state->number, ent->origin, 300, r_torch.ival, 0.9, 0.9, 0.6);
			dl->flags |= LFLAG_SHADOWMAP|LFLAG_FLASHBLEND;
			dl->fov = 90;
			VectorCopy(le->angles, angles);
			angles[0] *= 3;
//			angles[1] += sin(realtime)*8;
//			angles[0] += cos(realtime*1.13)*5;
			AngleVectorsMesh(angles, dl->axis[0], dl->axis[1], dl->axis[2]);
			VectorInverse(dl->axis[1]);

			VectorMA(dl->origin, 16, dl->axis[0], dl->origin);
		}

		// if set to invisible, skip
		if (state->modelindex<1 || (state->effects & NQEF_NODRAW))
		{
			if (state->tagindex == 0xffff)
			{
				if (state->tagentity)
				{
					ent->rtype = RT_PORTALCAMERA;
					ent->keynum = state->tagentity;
				}
				else
				{
					ent->rtype = RT_PORTALSURFACE;
					VectorCopy(ent->origin, ent->oldorigin);
				}
			}
			else
				continue;
			model = NULL;

			modelflags = state->effects>>24;
		}
		else
		{
			if (CL_FilterModelindex(state->modelindex, state->frame))
				continue;

			model = cl.model_precache[state->modelindex];
			if (!model)
			{
				Con_DPrintf("Bad modelindex (%i)\n", state->modelindex);
				continue;
			}
			if (model->loadstate != MLS_LOADED)
			{
				if (model->loadstate == MLS_NOTLOADED)
					Mod_LoadModel(model, MLV_WARN);
				continue;	//still waiting for it to load, don't poke anything here
			}

			//DP extension. .modelflags (which is sent in the high parts of effects) allows to specify exactly the q1-compatible flags.
			//the extra bit allows for setting to 0.
			//note that hexen2 has additional flags which cannot be expressed.
			modelflags = state->effects>>24;
			if (!(state->effects & EF_NOMODELFLAGS))
				modelflags |= model->flags;
		}

#ifdef HAVE_LEGACY
		if (cl.model_precache_vwep[0] && state->modelindex2 < MAX_VWEP_MODELS)
		{
			if (state->modelindex == cl_playerindex && cl.model_precache_vwep[0]->loadstate == MLS_LOADED &&
				state->modelindex2 && cl.model_precache_vwep[state->modelindex2] && cl.model_precache_vwep[state->modelindex2]->loadstate == MLS_LOADED)
			{
				model = cl.model_precache_vwep[0];
				model2 = cl.model_precache_vwep[state->modelindex2];
			}
			else
				model2 = NULL;
		}
		else
#endif
			if (state->modelindex2 && state->modelindex2 < MAX_PRECACHE_MODELS)
			model2 = cl.model_precache[state->modelindex2];
		else
			model2 = NULL;


		if (r_ignoreentpvs.ival || !model)
		{
			ent->pvscache.num_leafs = 0;
#if defined(Q2BSPS) || defined(Q3BSPS) || defined(TERRAIN)
			ent->pvscache.areanum = 0;
			ent->pvscache.areanum2 = 0;
			ent->pvscache.headnode = 0;
#endif
		}
		else
		{
			/*bsp model size*/
			if (model->type == mod_brush && (state->angles[0]||state->angles[1]||state->angles[2]))
			{
				int i;
				float v;
				float max;
				//q2 method, works best with origin brushes.
				max = 0;
				for (i=0 ; i<3 ; i++)
				{
					v =fabs( model->mins[i]);
					if (v > max)
						max = v;
					v =fabs( model->maxs[i]);
					if (v > max)
						max = v;
				}
				for (i=0 ; i<3 ; i++)
				{
					absmin[i] = ent->origin[i] - max;
					absmax[i] = ent->origin[i] + max;
				}
			}
			else
			{
				VectorAdd(model->mins, ent->origin, absmin);
				VectorAdd(model->maxs, ent->origin, absmax);
			}
			cl.worldmodel->funcs.FindTouchedLeafs(cl.worldmodel, &ent->pvscache, absmin, absmax);
		}

		cl_numvisedicts++;

		ent->forcedshader = NULL;

		ent->keynum = state->number;

		if (cl_r2g.value && state->modelindex == cl_rocketindex && cl_rocketindex != -1 && cl_grenadeindex != -1)
			model = cl.model_precache[cl_grenadeindex];
		ent->model = model;

		ent->flags = 0;
		if ((state->dpflags & RENDER_EXTERIORMODEL) || r_refdef.playerview->viewentity == state->number)
			ent->flags |= RF_EXTERNALMODEL;
		if (state->dpflags & RENDER_VIEWMODEL)
		{
			ent->flags |= RF_WEAPONMODEL|Q2RF_MINLIGHT|RF_DEPTHHACK;
			if (state->effects & DPEF_NOGUNBOB)
				ent->flags |= RF_WEAPONMODELNOBOB;
		}
		if (state->effects & NQEF_ADDITIVE)
			ent->flags |= RF_ADDITIVE;
		if (state->effects & EF_NODEPTHTEST)
			ent->flags |= RF_NODEPTHTEST;
		if (state->effects & EF_NOSHADOW)
			ent->flags |= RF_NOSHADOW;
		if (state->trans < 0xfe)
		{
			ent->shaderRGBAf[3] = state->trans/(float)0xfe;
			ent->flags |= RF_TRANSLUCENT;
		}
		else
			ent->shaderRGBAf[3] = 1;

/*		if (le->origin[2] < r_refdef.waterheight != le->lastorigin[2] < r_refdef.waterheight)
		{
			P_RunParticleEffectTypeString(le->origin, NULL, 1, "te_watertransition");
		}
*/
		// set colormap
		if (state->dpflags & RENDER_COLORMAPPED)
		{
			ent->topcolour    = (state->colormap>>4) & 0xf;
			ent->bottomcolour = (state->colormap>>0) & 0xf;
		}
		else if (state->colormap > 0 && state->colormap <= cl.allocated_client_slots)
		{
			ent->playerindex = state->colormap-1;
#ifdef HEXEN2
			ent->h2playerclass = cl.players[ent->playerindex].h2playerclass;
#endif
			ent->topcolour    = cl.players[ent->playerindex].dtopcolor;
			ent->bottomcolour = cl.players[ent->playerindex].dbottomcolor;
		}

		// set skin
		ent->skinnum = state->skinnum;

#ifdef HEXEN2
		ent->abslight = state->abslight;
		ent->drawflags = state->hexen2flags;
#endif

		CL_LerpNetFrameState(&ent->framestate, le);

#ifdef PEXT_SCALE
		//set scale
		ent->scale = state->scale/16.0;
#endif
		if (state->colormod[0] == 32 && state->colormod[1] == 32 && state->colormod[2] == 32)
			ent->shaderRGBAf[0] = ent->shaderRGBAf[1] = ent->shaderRGBAf[2] = 1;
		else
		{
			ent->flags |= RF_FORCECOLOURMOD;
			ent->shaderRGBAf[0] = (state->colormod[0]*8.0f)/256;
			ent->shaderRGBAf[1] = (state->colormod[1]*8.0f)/256;
			ent->shaderRGBAf[2] = (state->colormod[2]*8.0f)/256;
		}
		VectorScale(state->glowmod, 8.0/256.0, ent->glowmod);

#ifdef PEXT_FATNESS
		//set trans
		ent->fatness = state->fatness/16.0;
#endif

		//swap items with sprites if desired.
		if (gl_simpleitems.ival && ent->skinnum >= 0 && ent->skinnum < countof(model->simpleskin) && model)
		{
			if (!model->simpleskin[ent->skinnum])
			{
				char basename[64], name[MAX_QPATH];
				COM_FileBase(model->name, basename, sizeof(basename));
				if (!strncmp(model->name, "maps/", 5))
					Q_snprintfz(name, sizeof(name), "textures/bmodels/simple_%s_%i.tga", basename, ent->skinnum);
				else
					Q_snprintfz(name, sizeof(name), "textures/models/simple_%s_%i.tga", basename, ent->skinnum);
				model->simpleskin[ent->skinnum] = R_RegisterShader(name, 0, va("{\nnomipmaps\nprogram defaultsprite#MASK=0.5\nsurfaceparm noshadows\nsurfaceparm nodlight\nsort seethrough\n{\nmap \"%s\"\nalphafunc ge128\n}\n}\n", name));
			}
			VectorCopy(le->angles, angles);

			if (R_GetShaderSizes(model->simpleskin[ent->skinnum], NULL, NULL, false) > 0)
			{
				float tr[2];
				ent->forcedshader = model->simpleskin[ent->skinnum];
				ent->rtype = RT_SPRITE;
				ent->scale *= 16;

				tr[0] = sin(le->angles[1] * M_PI / 180.0);
				tr[1] = cos(le->angles[1] * M_PI / 180.0);
				ent->origin[1] += tr[0] * (model->maxs[0] + model->mins[0])*0.5 + tr[1] * (model->maxs[1] + model->mins[1])*0.5;
				ent->origin[0] += tr[1] * (model->maxs[1] + model->mins[1])*0.5 - tr[0] * (model->maxs[0] + model->mins[0])*0.5;
				ent->origin[2] += model->mins[2];

				ent->origin[2] += ent->scale;

				if (cl_item_bobbing.value)
					ent->origin[2] += 5+sin(cl.time*3+(ent->origin[0]+ent->origin[1])/8)*5.5;	//don't let it into the ground
			}
			else if (modelflags & MF_ROTATE)
			{	//surely there's a more sane way to handle this.
				angles[0] = 0;
				angles[1] = autorotate;
				angles[2] = 0;

				if (cl_item_bobbing.value)
					ent->origin[2] += 5+sin(cl.time*3+(state->origin[0]+state->origin[1])/8)*5.5;	//don't let it into the ground
			}
		}
		// rotate pickup objects locally
		else if (modelflags & MF_ROTATE)
		{
			angles[0] = 0;
			angles[1] = autorotate;
			angles[2] = 0;

			if (cl_item_bobbing.value)
				ent->origin[2] += 5+sin(cl.time*3+(state->origin[0]+state->origin[1])/8)*5.5;	//don't let it into the ground
		}
		else
		{
			for (i=0 ; i<3 ; i++)
			{
				angles[i] = le->angles[i];
			}
		}

		VectorCopy(angles, ent->angles);
		if (model->type == mod_alias)
			AngleVectorsMesh(angles, ent->axis[0], ent->axis[1], ent->axis[2]);
		else
			AngleVectors(angles, ent->axis[0], ent->axis[1], ent->axis[2]);
		VectorInverse(ent->axis[1]);

		/*if this entity is in a player's slot...*/
		if (ent->keynum <= cl.allocated_client_slots)
		{
			if (!cl.playerview[0].nolocalplayer)
				ent->keynum += MAX_EDICTS;
		}

		if (state->tagindex == 0xffff)
		{
			if (state->tagentity)
			{
				ent->rtype = RT_PORTALCAMERA;
				ent->keynum = state->tagentity;
			}
			else
			{
				ent->rtype = RT_PORTALSURFACE;
				VectorCopy(ent->origin, ent->oldorigin);
			}
		}
		else if (state->tagentity)
		{	//ent is attached to a tag, rotate this ent accordingly.
			CL_RotateAroundTag(ent, state->number, state->tagentity, state->tagindex);
		}

#ifdef RAGDOLL
		if (model && (model->dollinfo || le->skeletalobject))
			rag_updatedeltaent(&csqc_world, ent, le);
#endif
		ent->framestate.g[FS_REG].frame[0] &= ~0x8000;
		ent->framestate.g[FS_REG].frame[1] &= ~0x8000;

		CLQ1_AddShadow(ent);
		CLQ1_AddPowerupShell(ent, false, state->effects);

		if (model2)
			CL_AddVWeapModel (ent, model2);

		//figure out which trail this entity is using
		if (model)
		{
			trailef = model->particletrail;
			trailidx = model->traildefaultindex;
		}
		else
		{
			trailef = P_INVALID;
			trailidx = P_INVALID;
		}
		if ((state->effects & EF_HASPARTICLETRAIL) || modelflags)
			P_DefaultTrail (state->effects, modelflags, &trailef, &trailidx);
		if (state->u.q1.traileffectnum)
			trailef = CL_TranslateParticleFromServer(state->u.q1.traileffectnum);

		if (state->u.q1.emiteffectnum)
			P_EmitEffect (ent->origin, ent->axis, MDLF_EMITFORWARDS, CL_TranslateParticleFromServer(state->u.q1.emiteffectnum), &(le->emitstate));
		else if (model && model->particleeffect != P_INVALID && cls.allow_anyparticles && gl_part_flame.ival)
			P_EmitEffect (ent->origin, ent->axis, model->engineflags, model->particleeffect, &(le->emitstate));

		// add automatic particle trails
		if (!model || (!(modelflags&~MF_ROTATE) && trailef < 0))
			continue;

		if (!cls.allow_anyparticles && !(modelflags & ~MF_ROTATE))
			continue;

		if (le->isnew)
		{
			le->isnew = false;
			pe->DelinkTrailstate(&(cl.lerpents[state->number].trailstate));
			pe->DelinkTrailstate(&(cl.lerpents[state->number].emitstate));
			continue;		// not in last message
		}

		VectorCopy(le->lastorigin, old_origin);
		for (i=0 ; i<3 ; i++)
		{
			if ( fabs(old_origin[i] - ent->origin[i]) > 128)
			{	// no trail if too far
				VectorCopy (ent->origin, old_origin);
				break;
			}
		}

		//and emit it
//		if (lasttime != cl.currentpacktime)
		{
			if (trailef == P_INVALID || pe->ParticleTrail (old_origin, ent->origin, trailef, timestep, ent->keynum, ent->axis, &(le->trailstate)))
				if (model->traildefaultindex >= 0)
					pe->ParticleTrailIndex(old_origin, ent->origin, P_INVALID, timestep, trailidx, 0, &(le->trailstate));

			//dlights are not so customisable.
			if (r_rocketlight.value && (modelflags & MF_ROCKET) && !(state->lightpflags & (PFLAGS_FULLDYNAMIC|PFLAGS_CORONA)))
			{
				float rad = 0;
				extern cvar_t r_rocketlight_colour;
				rad = r_rocketlight_colour.vec4[3];
				rad += r_lightflicker.value?((flicker + state->number)&31):0;

				dl = CL_AllocDlight (state->number);
				memcpy(dl->axis, ent->axis, sizeof(dl->axis));
				VectorCopy (ent->origin, dl->origin);
				dl->die = (float)cl.time;
				if (modelflags & MF_ROCKET)
					dl->origin[2] += 1; // is this even necessary
				dl->radius = rad * r_rocketlight.value;
				VectorCopy(r_rocketlight_colour.vec4, dl->color);
			}
		}
	}
#ifdef CSQC_DAT
	CSQC_DeltaEnd();
#endif

	CLQ1_AddVisibleBBoxes();

#ifdef RTLIGHTS
	R_EditLights_DrawLights();
#endif
}

/*
=========================================================================

PROJECTILE PARSING / LINKING

=========================================================================
*/

typedef struct
{
	int		modelindex;
	vec3_t	origin;
	vec3_t	angles;
} projectile_t;

#define	MAX_PROJECTILES	32
projectile_t	cl_projectiles[MAX_PROJECTILES];
int				cl_num_projectiles;

extern int cl_spikeindex;

void CL_ClearProjectiles (void)
{
	cl_num_projectiles = 0;
}

/*
=====================
CL_ParseProjectiles

Nails are passed as efficient temporary entities
=====================
*/
void CL_ParseProjectiles (int modelindex, qboolean nails2)
{
	int		i, c, j;
	qbyte	bits[6];
	projectile_t	*pr;

	c = MSG_ReadByte ();
	for (i=0 ; i<c ; i++)
	{
		if (nails2)
			MSG_ReadByte();
		for (j=0 ; j<6 ; j++)
			bits[j] = MSG_ReadByte ();

		if (cl_num_projectiles == MAX_PROJECTILES)
			continue;

		pr = &cl_projectiles[cl_num_projectiles];
		cl_num_projectiles++;

		pr->modelindex = modelindex;
		pr->origin[0] = ( ( bits[0] + ((bits[1]&15)<<8) ) <<1) - 4096;
		pr->origin[1] = ( ( (bits[1]>>4) + (bits[2]<<4) ) <<1) - 4096;
		pr->origin[2] = ( ( bits[3] + ((bits[4]&15)<<8) ) <<1) - 4096;
		pr->angles[0] = 360*(((int)bits[4]>>4)/16.0f + 1/32.0f);
		pr->angles[1] = 360*(int)bits[5]/256.0f;
	}
}

/*
=============
CL_LinkProjectiles

=============
*/
void CL_LinkProjectiles (void)
{
	int		i;
	projectile_t	*pr;
	entity_t		*ent;

	for (i=0, pr=cl_projectiles ; i<cl_num_projectiles ; i++, pr++)
	{
		if (pr->modelindex < 1)
			continue;

		// grab an entity to fill in
		if (cl_numvisedicts == cl_maxvisedicts)
			break;		// object list is full
		ent = &cl_visedicts[cl_numvisedicts];
		cl_numvisedicts++;
		memset(ent, 0, sizeof(*ent));

		ent->model = cl.model_precache[pr->modelindex];
		ent->playerindex = -1;
		ent->topcolour = TOP_DEFAULT;
		ent->bottomcolour = BOTTOM_DEFAULT;
		ent->framestate.g[FS_REG].lerpweight[0] = 1;

#ifdef PEXT_SCALE
		ent->scale = 1;
#endif

		ent->shaderRGBAf[0] = 1;
		ent->shaderRGBAf[1] = 1;
		ent->shaderRGBAf[2] = 1;
		ent->shaderRGBAf[3] = 1;

		VectorCopy (pr->origin, ent->origin);
		VectorCopy (pr->angles, ent->angles);

		AngleVectorsMesh(ent->angles, ent->axis[0], ent->axis[1], ent->axis[2]);
		VectorInverse(ent->axis[1]);
	}
}

//========================================

extern	int		cl_spikeindex, cl_playerindex, cl_flagindex, cl_rocketindex, cl_grenadeindex;

entity_t *CL_NewTempEntity (void);

static int MVD_TranslateFlags(int src)
{
	int dst = 0;

	if (src & DF_EFFECTS)
		dst |= PF_EFFECTS;
	if (src & DF_SKINNUM)
		dst |= PF_SKINNUM;
	if (src & DF_DEAD)
		dst |= PF_DEAD;
	if (src & DF_GIB)
		dst |= PF_GIB;
	if (src & DF_WEAPONFRAME)
		dst |= PF_WEAPONFRAME;
	if (src & DF_MODEL)
		dst |= PF_MODEL;

	return dst;
}

/*
===================
CL_ParsePlayerinfo
===================
*/
extern int parsecountmod, oldparsecountmod;
extern double parsecounttime;

void CL_ParseClientdata (void);
void CL_MVDUpdateSpectator(void)
{
	CL_ParseClientdata();
}


void CLQW_ParsePlayerinfo (void)
{
	float			msec;
	unsigned int			flags;
	player_info_t	*info;
	player_state_t	*state, *oldstate;
	unsigned int			num;
	int			i;
	int newf;
	vec3_t		org, dist;

	if (cls.fteprotocolextensions2&PEXT2_LONGINDEXES)
		num = MSG_ReadUInt64 ();
	else
		num = MSG_ReadByte ();
	if (num >= MAX_CLIENTS)
		Host_EndGame ("CL_ParsePlayerinfo: bad num");

	info = &cl.players[num];

	oldstate = &cl.inframes[oldparsecountmod].playerstate[num];
	state = &cl.inframes[parsecountmod].playerstate[num];

	if (cls.demoplayback == DPB_MVD)
	{
#ifdef QUAKESTATS
		int i;
		const char *viewmodel = NULL;
		static struct {
			const char *vmdl;
			const char *vwep;
		} vwep_mapping[] =
		{
			{"progs/v_axe.mdl",	"progs/w_axe.mdl"},
			{"progs/v_shot.mdl",	"progs/w_shot.mdl"},
			{"progs/v_shot2.mdl",	"progs/w_shot2.mdl"},
			{"progs/v_nail.mdl",	"progs/w_nail.mdl"},
			{"progs/v_nail2.mdl",	"progs/w_nail2.mdl"},
			{"progs/v_rock.mdl",	"progs/w_rock.mdl"},
			{"progs/v_rock2.mdl",	"progs/w_rock2.mdl"},
			{"progs/v_light.mdl",	"progs/w_light.mdl"},
		};
#endif
		player_state_t	dummy;
		if (!cl.parsecount || info->prevcount > cl.parsecount || cl.parsecount - info->prevcount >= UPDATE_BACKUP - 1)
		{
			memset(&dummy, 0, sizeof(dummy));
			oldstate = &dummy;
		}
		else
		{
			oldstate = &cl.inframes[info->prevcount & UPDATE_MASK].playerstate[num];
		}
		memcpy(state, oldstate, sizeof(player_state_t));
		info->prevcount = cl.parsecount;

#ifdef QUAKESTATS
		if (cls.findtrack && info->stats[STAT_HEALTH] > 0)
		{	//FIXME: is this still needed with the autotrack stuff?
			Cam_Lock(&cl.playerview[0], num);
			cls.findtrack = false;
		}
#endif

		flags = MSG_ReadShort ();
		state->flags = MVD_TranslateFlags(flags);

		state->messagenum = cl.parsecount;
		state->command.msec = 0;
		state->command.impulse = 0;

#ifdef QUAKESTATS
		i = cl.players[num].stats[STAT_WEAPONMODELI];
		if (i>0&&i<MAX_PRECACHE_MODELS && cl.model_name_vwep[0])
			viewmodel = cl.model_name[i];
		if(viewmodel)
		{
			for (i = 0; i < countof(vwep_mapping); i++)
			{
				if (!strcmp(viewmodel, vwep_mapping[i].vmdl))
				{
					viewmodel = vwep_mapping[i].vwep;
					for (i = 1; i < countof(cl.model_name_vwep); i++)
					{
						if (!cl.model_name_vwep[i])
							break;
						if (!strcmp(viewmodel, cl.model_name_vwep[i]))
						{
							state->command.impulse = i;
							break;
						}
					}
					break;
				}
			}
		}
#endif

		state->frame = MSG_ReadByte ();

		state->state_time = parsecounttime;

		for (i = 0; i < 3; i++)
		{
			if (flags & (DF_ORIGINX << i))
			{
				if (cls.ezprotocolextensions1 & EZPEXT1_FLOATENTCOORDS)
					state->origin[i] = MSG_ReadCoordFloat ();
				else
					state->origin[i] = MSG_ReadCoord ();
			}
		}

		VectorSubtract(state->origin, oldstate->origin, dist);
		VectorScale(dist, 1/(cl.inframes[parsecountmod].packet_entities.servertime - cl.inframes[oldparsecountmod].packet_entities.servertime), state->velocity);
		VectorCopy (state->origin, state->predorigin);

		for (i = 0; i < 3; i++)
		{
			if (flags & (DF_ANGLEX << i))
			{
				state->command.angles[i] = MSG_ReadShort();
			}
			state->viewangles[i] = state->command.angles[i] * (360.0/65536);
		}

		if (flags & DF_MODEL)
			state->modelindex = MSG_ReadByte ();

		if (flags & DF_SKINNUM)
			state->skinnum = MSG_ReadByte ();

		if (flags & DF_EFFECTS)
			state->effects = MSG_ReadByte ();

		if (flags & DF_WEAPONFRAME)
			state->weaponframe = MSG_ReadByte ();

		VectorSet(state->szmins, -16, -16, -24);
		VectorSet(state->szmaxs, 16, 16, 32);
		state->scale = 1;
		state->alpha = 255;
		state->fatness = 0;

		state->colourmod[0] = 32;
		state->colourmod[1] = 32;
		state->colourmod[2] = 32;

		state->gravitydir[0] = 0;
		state->gravitydir[1] = 0;
		state->gravitydir[2] = -1;

		state->pm_type = PM_NORMAL;

#ifdef QUAKESTATS
		TP_ParsePlayerInfo(oldstate, state, info);

		//can't CL_SetStatInt as we don't know if its actually us or not
		cl.players[num].stats[STAT_WEAPONFRAME] = state->weaponframe;
		cl.players[num].statsf[STAT_WEAPONFRAME] = state->weaponframe;
		for (i = 0; i < cl.splitclients; i++)
		{
			playerview_t *pv = &cl.playerview[i];
			if (pv->cam_spec_track == num)
			{
				pv->stats[STAT_WEAPONFRAME] = state->weaponframe;
				pv->statsf[STAT_WEAPONFRAME] = state->weaponframe;
			}
		}
#endif

		//add a new splitscreen autotrack view if we can
		if (cl.splitclients < MAX_SPLITS && !cl.players[num].spectator)
		{
			if (cl.splitclients < cl_splitscreen.value+1)
			{
				for (i = 0; i < cl.splitclients; i++)
				{
					playerview_t *pv = &cl.playerview[i];
					if (pv->cam_state != CAM_FREECAM && pv->cam_spec_track == num)
						return;
				}

				if (i == cl.splitclients)
				{
					playerview_t *pv = &cl.playerview[cl.splitclients++];
					Cam_Lock(pv, num);
				}
			}
		}
		return;
	}

	flags = (unsigned short)MSG_ReadShort ();

	if (cls.fteprotocolextensions & (PEXT_HULLSIZE|PEXT_TRANS|PEXT_SCALE|PEXT_FATNESS))
	{
		if (flags & PF_EXTRA_PFS)
			flags |= MSG_ReadByte()<<16;
	}
	else
		flags = (flags & 0x3fff) | ((flags & 0xc000)<<8);

	state->flags = flags;

	state->messagenum = cl.parsecount;
	if (cls.ezprotocolextensions1 & EZPEXT1_FLOATENTCOORDS)
	{
		org[0] = MSG_ReadCoordFloat ();
		org[1] = MSG_ReadCoordFloat ();
		org[2] = MSG_ReadCoordFloat ();
	}
	else
	{
		org[0] = MSG_ReadCoord ();
		org[1] = MSG_ReadCoord ();
		org[2] = MSG_ReadCoord ();
	}

	VectorCopy(org, state->origin);

	newf = MSG_ReadByte ();
	if (state->frame != newf)
	{
//		state->lerpstarttime = realtime;
		state->frame = newf;
	}

	// the other player's last move was likely some time
	// before the packet was sent out, so accurately track
	// the exact time it was valid at
	if (flags & PF_MSEC)
	{
		extern cvar_t cl_demospeed;
		msec = MSG_ReadByte ();
		if (cls.demoplayback)
			state->state_time = parsecounttime - msec*0.001 * cl_demospeed.value;
		else
			state->state_time = parsecounttime - msec*0.001;
	}
	else
	{
		msec = 0;
		state->state_time = parsecounttime;
	}

	if (flags & PF_COMMAND)
	{
		MSGQW_ReadDeltaUsercmd (&nullcmd, &state->command, cl.protocol_qw);

		state->viewangles[0] = state->command.angles[0] * (360.0/65536);
		state->viewangles[1] = state->command.angles[1] * (360.0/65536);
		state->viewangles[2] = state->command.angles[2] * (360.0/65536);

		if (!(cls.z_ext & Z_EXT_VWEP))
			state->command.impulse = 0;
	}

	for (i=0 ; i<3 ; i++)
	{
		if (flags & (PF_VELOCITY1<<i) )
			state->velocity[i] = MSG_ReadShort();
		else
			state->velocity[i] = 0;
	}
	if (flags & PF_MODEL)
		state->modelindex = MSG_ReadByte ();
	else
		state->modelindex = cl_playerindex;

	if (flags & PF_SKINNUM)
	{
		state->skinnum = MSG_ReadByte ();
		if (state->skinnum & (1<<7) && (flags & PF_MODEL))
		{
			state->modelindex+=256;
			state->skinnum -= (1<<7);
		}
	}
	else
		state->skinnum = 0;

	if (flags & PF_EFFECTS)
		state->effects = MSG_ReadByte ();
	else
		state->effects = 0;

	if (flags & PF_WEAPONFRAME)
		state->weaponframe = MSG_ReadByte ();
	else
		state->weaponframe = 0;

	VectorSet(state->szmins, -16, -16, -24);
	VectorSet(state->szmaxs, 16, 16, 32);
	state->scale = 1;
	state->alpha = 255;
	state->fatness = 0;

	state->gravitydir[0] = 0;
	state->gravitydir[1] = 0;
	state->gravitydir[2] = -1;

#ifdef PEXT_SCALE
	if ((flags & PF_SCALE) && (cls.fteprotocolextensions & PEXT_SCALE))
		state->scale = MSG_ReadByte()/50.0;
#endif
#ifdef PEXT_TRANS
	if ((flags & PF_TRANS) && (cls.fteprotocolextensions & PEXT_TRANS))
		state->alpha = MSG_ReadByte();
#endif
#ifdef PEXT_FATNESS
	if ((flags & PF_FATNESS) && (cls.fteprotocolextensions & PEXT_FATNESS))
		state->fatness = MSG_ReadChar();
#endif
#ifdef PEXT_HULLSIZE
	if ((cls.fteprotocolextensions & PEXT_HULLSIZE) && (flags & PF_HULLSIZE_Z))
	{
		int num;
		num = MSG_ReadByte();

		if (!cl.worldmodel || cl.worldmodel->fromgame != fg_quake)
		{
			VectorScale(state->szmins, num/56.0f, state->szmins);
			VectorScale(state->szmaxs, num/56.0f, state->szmaxs);
		}
		else
		{
			VectorCopy(cl.worldmodel->hulls[num&(MAX_MAP_HULLSM-1)].clip_mins, state->szmins);
			VectorCopy(cl.worldmodel->hulls[num&(MAX_MAP_HULLSM-1)].clip_maxs, state->szmaxs);
		}
		if (num & 128)
		{	//this hack is for hexen2.
			state->szmaxs[2] -= state->szmins[2];
			state->szmins[2] = 0;
		}
	}
	//should be passed to player move func.
#endif
	if (cls.z_ext & Z_EXT_PF_ONGROUND)
		state->onground = !!(flags & PF_ONGROUND);
	else
		state->onground = false;

	if ((cls.fteprotocolextensions & PEXT_COLOURMOD) && (flags & PF_COLOURMOD))
	{
		state->colourmod[0] = MSG_ReadByte();
		state->colourmod[1] = MSG_ReadByte();
		state->colourmod[2] = MSG_ReadByte();
	}
	else
	{
		state->colourmod[0] = 32;
		state->colourmod[1] = 32;
		state->colourmod[2] = 32;
	}

	//if we have no solidity info, guess.
	if (!(cls.z_ext & Z_EXT_PF_SOLID))
	{
		if (cl.players[num].spectator || state->flags & PF_DEAD)
			state->flags &= ~PF_SOLID;
		else
			state->flags |= PF_SOLID;
	}

	if (cls.z_ext & Z_EXT_PM_TYPE)
	{
		int pm_code;

		pm_code = (flags&PF_PMC_MASK) >> PF_PMC_SHIFT;
		if (pm_code == PMC_NORMAL || pm_code == PMC_NORMAL_JUMP_HELD)
		{
			if (flags & PF_DEAD)
				state->pm_type = PM_DEAD;
			else
			{
				state->pm_type = PM_NORMAL;
				state->jump_held = (pm_code == PMC_NORMAL_JUMP_HELD);
			}
		}
		else if (pm_code == PMC_OLD_SPECTATOR)
			state->pm_type = PM_OLD_SPECTATOR;
		else
		{
			if (cls.z_ext & Z_EXT_PM_TYPE_NEW)
			{
				if (pm_code == PMC_SPECTATOR)
					state->pm_type = PM_SPECTATOR;
				else if (pm_code == PMC_FLY)
					state->pm_type = PM_FLY;
				else if (pm_code == PMC_NONE)
					state->pm_type = PM_NONE;
				else if (pm_code == PMC_FREEZE)
					state->pm_type = PM_FREEZE;
				else if (pm_code == PMC_WALLWALK)
					state->pm_type = PM_WALLWALK;
				else {
					// future extension?
					goto guess_pm_type;
				}
			}
			else
			{
				// future extension?
				goto guess_pm_type;
			}
		}
	}
	else
	{
guess_pm_type:
		if (cl.players[num].spectator)
			state->pm_type = PM_OLD_SPECTATOR;
		else if (flags & PF_DEAD)
			state->pm_type = PM_DEAD;
		else
			state->pm_type = PM_NORMAL;
	}

#ifdef QUAKESTATS
	TP_ParsePlayerInfo(oldstate, state, info);

	//can't CL_SetStatInt as we don't know if its actually us or not
	for (i = 0; i < cl.splitclients; i++)
	{
		playerview_t *pv = &cl.playerview[i];
		if ((pv->spectator?pv->cam_spec_track:pv->playernum) == num)
		{
			pv->stats[STAT_WEAPONFRAME] = state->weaponframe;
			pv->statsf[STAT_WEAPONFRAME] = state->weaponframe;
		}
	}
#endif

	if (cl.worldmodel && cl.do_lerp_players && cl_predict_players.ival)
	{
		player_state_t exact;
		msec -= 1000 * (cls.latency*cl_predict_players_latency.value-cl_predict_players_nudge.value);
//		msec = 1000*((realtime - cls.latency + 0.02) - state->state_time);
		// predict players movement
		state->command.msec = bound(0, msec, 255);

		//FIXME: flag these and do the pred elsewhere.
		CL_SetSolidEntities();
		CL_SetSolidPlayers();
		CL_PredictUsercmd (0, num+1, state, &exact, &state->command);	//uses player 0's maxspeed/grav...
		VectorCopy (exact.origin, state->predorigin);
	}
	else
		VectorCopy (state->origin, state->predorigin);
}

/*
void CL_ParseClientPersist(void)
{
	player_info_t	*info;
	int flags;
	flags = MSG_ReadShort();
	info = &cl.players[lastplayerinfo];
	if (flags & 1)
		info->vweapindex = MSG_ReadShort();
}
*/

/*
================
CL_AddFlagModels

Called when the CTF flags are set
================
*/
void CL_AddFlagModels (entity_t *ent, int team)
{
	int		i;
	float	f;
	vec3_t	v_forward, v_right, v_up;
	entity_t	*newent;
	vec3_t	angles;
	float offs = 0;

	if (cl_flagindex == -1)
		return;

	for (i = 0; i < FRAME_BLENDS; i++)
	{
		if (!ent->framestate.g[FS_REG].lerpweight[i])
			continue;
		f = 14;
		if (ent->framestate.g[FS_REG].frame[i] >= 29 && ent->framestate.g[FS_REG].frame[i] <= 40) {
			if (ent->framestate.g[FS_REG].frame[i] >= 29 && ent->framestate.g[FS_REG].frame[i] <= 34) { //axpain
				if      (ent->framestate.g[FS_REG].frame[i] == 29) f = f + 2;
				else if (ent->framestate.g[FS_REG].frame[i] == 30) f = f + 8;
				else if (ent->framestate.g[FS_REG].frame[i] == 31) f = f + 12;
				else if (ent->framestate.g[FS_REG].frame[i] == 32) f = f + 11;
				else if (ent->framestate.g[FS_REG].frame[i] == 33) f = f + 10;
				else if (ent->framestate.g[FS_REG].frame[i] == 34) f = f + 4;
			} else if (ent->framestate.g[FS_REG].frame[i] >= 35 && ent->framestate.g[FS_REG].frame[i] <= 40) { // pain
				if      (ent->framestate.g[FS_REG].frame[i] == 35) f = f + 2;
				else if (ent->framestate.g[FS_REG].frame[i] == 36) f = f + 10;
				else if (ent->framestate.g[FS_REG].frame[i] == 37) f = f + 10;
				else if (ent->framestate.g[FS_REG].frame[i] == 38) f = f + 8;
				else if (ent->framestate.g[FS_REG].frame[i] == 39) f = f + 4;
				else if (ent->framestate.g[FS_REG].frame[i] == 40) f = f + 2;
			}
		} else if (ent->framestate.g[FS_REG].frame[i] >= 103 && ent->framestate.g[FS_REG].frame[i] <= 118) {
			if      (ent->framestate.g[FS_REG].frame[i] >= 103 && ent->framestate.g[FS_REG].frame[i] <= 104) f = f + 6;  //nailattack
			else if (ent->framestate.g[FS_REG].frame[i] >= 105 && ent->framestate.g[FS_REG].frame[i] <= 106) f = f + 6;  //light
			else if (ent->framestate.g[FS_REG].frame[i] >= 107 && ent->framestate.g[FS_REG].frame[i] <= 112) f = f + 7;  //rocketattack
			else if (ent->framestate.g[FS_REG].frame[i] >= 112 && ent->framestate.g[FS_REG].frame[i] <= 118) f = f + 7;  //shotattack
		}

		offs += f * ent->framestate.g[FS_REG].lerpweight[i];
	}

	newent = CL_NewTempEntity ();
	newent->model = cl.model_precache[cl_flagindex];
	newent->skinnum = team;
	newent->keynum = ent->keynum;
	newent->flags |= ent->flags;

	AngleVectors (ent->angles, v_forward, v_right, v_up);
	v_forward[2] = -v_forward[2]; // reverse z component
	for (i=0 ; i<3 ; i++)
		newent->origin[i] = ent->origin[i] - offs*v_forward[i] + 22*v_right[i];
	newent->origin[2] -= 16;

	VectorCopy (ent->angles, newent->angles);
	newent->angles[2] -= 45;

	VectorCopy(newent->angles, angles);
	AngleVectorsMesh(angles, newent->axis[0], newent->axis[1], newent->axis[2]);
	VectorInverse(newent->axis[1]);
}

void CL_AddVWeapModel(entity_t *player, model_t *model)
{
	entity_t	*newent;
//	vec3_t	angles;
	if (!model)
		return;
	newent = CL_NewTempEntity ();

	newent->keynum = player->keynum;
	newent->flags |= player->flags;

	VectorCopy(player->origin, newent->origin);
	VectorCopy(player->angles, newent->angles);
	newent->skinnum = player->skinnum;
	newent->model = model;
	newent->framestate = player->framestate;

	AngleVectors(newent->angles, newent->axis[0], newent->axis[1], newent->axis[2]);
	VectorInverse(newent->axis[1]);
}

/*
=============
CL_LinkPlayers

Create visible entities in the correct position
for all current players
=============
*/
vec3_t nametagorg[MAX_CLIENTS];
qboolean nametagseen[MAX_CLIENTS];
void CL_LinkPlayers (void)
{
	int pnum;
	int				j;
	player_info_t	*info;
	player_state_t	*state;
	player_state_t	exact;
	double			playertime;
	entity_t		*ent;
	float			msec;
	inframe_t		*frame;
	int				oldphysent;
	vec3_t			angles;
	qboolean		predictplayers;
	model_t			*model;
	static int		flickertime;
	static int		flicker;
	float			predictmsmult = 1000*cl_predict_players_frac.value;
#ifdef HAVE_LEGACY
	int				modelindex2;
#endif
	extern cvar_t	cl_demospeed;
	int displayseq;

	if (!cl.worldmodel || cl.worldmodel->loadstate != MLS_LOADED)
		return;

	if (cl.paused)
		predictmsmult = 0;
	if (cls.demoplayback)
		predictmsmult *= cl_demospeed.value;

	playertime = realtime - cls.latency*cl_predict_players_latency.value + cl_predict_players_nudge.value;
	if (playertime > realtime)
		playertime = realtime;

	if (cl.demonudge < 0)
		displayseq = cl.lerpentssequence;
	else
		displayseq = cl.validsequence;
	frame = &cl.inframes[displayseq&UPDATE_MASK];

	predictplayers = cl_predict_players.ival;
	if (cls.demoplayback == DPB_MVD)
		predictplayers = false;

	for (j=0, info=cl.players, state=frame->playerstate ; j < cl.allocated_client_slots
		; j++, info++, state++)
	{
		nametagseen[j] = false;

		if (state->messagenum != displayseq)
		{
#ifdef CSQC_DAT
			CSQC_DeltaPlayer(j, NULL);
#endif
			continue;	// not present this frame
		}

		CL_UpdateNetFrameLerpState(false, state->frame, 0, 0, &cl.lerpplayers[j], 0);
		cl.lerpplayers[j].sequence = cl.lerpentssequence;

#ifdef CSQC_DAT
		if (CSQC_DeltaPlayer(j, state))
			continue;
#endif

		if (info->spectator || state->modelindex >= countof(cl.model_precache))
			continue;

		//the extra modelindex check is to stop lame mods from using vweps with rings
#ifdef HAVE_LEGACY
		if (state->command.impulse && cl.model_precache_vwep[0] && cl.model_precache_vwep[0]->type != mod_dummy && state->modelindex == cl_playerindex)
		{
			model = cl.model_precache_vwep[0];
			modelindex2 = state->command.impulse;
		}
		else
#endif
		{
			model = cl.model_precache[state->modelindex];
#ifdef HAVE_LEGACY
			modelindex2 = 0;
#endif
		}

		// spawn light flashes, even ones coming from invisible objects
		if (r_powerupglow.value && !(r_powerupglow.value == 2 && j == cl.playerview[0].playernum)
			&& (state->effects & (EF_BLUE|EF_RED|EF_GREEN|EF_BRIGHTLIGHT|EF_DIMLIGHT)))
		{
			vec3_t colour;
			float radius;
			colour[0] = 0;
			colour[1] = 0;
			colour[2] = 0;
			radius = 0;

			if (state->effects & EF_BRIGHTLIGHT)
			{
				radius = max(radius,r_brightlight_colour.vec4[3]);
				if (!(state->effects & (EF_RED|EF_GREEN|EF_BLUE)))
					VectorAdd(colour, r_brightlight_colour.vec4, colour);
			}
			if (state->effects & EF_DIMLIGHT)
			{
				radius = max(radius,r_dimlight_colour.vec4[3]);
				if (!(state->effects & (EF_RED|EF_GREEN|EF_BLUE)))
					VectorAdd(colour, r_dimlight_colour.vec4, colour);
			}
			if (state->effects & EF_BLUE)
			{
				radius = max(radius,r_bluelight_colour.vec4[3]);
				VectorAdd(colour, r_bluelight_colour.vec4, colour);
			}
			if (state->effects & EF_RED)
			{
				radius = max(radius,r_redlight_colour.vec4[3]);
				VectorAdd(colour, r_redlight_colour.vec4, colour);
			}
			if (state->effects & EF_GREEN)
			{
				radius = max(radius,r_greenlight_colour.vec4[3]);
				VectorAdd(colour, r_greenlight_colour.vec4, colour);
			}

			if (radius)
			{
				vec3_t org;
				VectorCopy(state->origin, org);
				//make the light appear at the predicted position rather than anywhere else.
				for (pnum = 0; pnum < cl.splitclients; pnum++)
					if (cl.playerview[pnum].playernum == j)
						VectorCopy(cl.playerview[pnum].simorg, org);
				if (model)
				{
					org[2] += model->mins[2];
					org[2] += 32;
				}
				if (r_lightflicker.value)
				{
					pnum = realtime*20;
					if (flickertime != pnum)
					{
						flickertime = pnum;
						flicker = rand();
					}
					radius += (flicker+j)&31;
				}
				CL_NewDlight(j+1, org, radius, 0.1, colour[0], colour[1], colour[2])->flags &= ~LFLAG_FLASHBLEND;
			}
		}

		if (state->modelindex < 1)
			continue;

		if (CL_FilterModelindex(state->modelindex, state->frame))
			continue;
/*
		if (!Cam_DrawPlayer(j))
			continue;
*/
		// grab an entity to fill in
		if (cl_numvisedicts == cl_maxvisedicts)
			break;		// object list is full
		ent = &cl_visedicts[cl_numvisedicts];
		cl_numvisedicts++;
		memset(ent, 0, sizeof(*ent));
		ent->keynum = j+1;
		ent->model = model;

		ent->skinnum = state->skinnum;

		CL_LerpNetFrameState(&ent->framestate,	&cl.lerpplayers[j]);

		// set colormap
		ent->playerindex = j;
		ent->topcolour	  = info->dtopcolor;
		ent->bottomcolour = info->dbottomcolor;
#ifdef HEXEN2
		ent->h2playerclass = info->h2playerclass;
#endif

#ifdef PEXT_SCALE
		ent->scale = state->scale;
#endif
		ent->glowmod[0] = ent->glowmod[1] = ent->glowmod[2] = 1;
		ent->shaderRGBAf[0] = state->colourmod[0]/32.0f;
		ent->shaderRGBAf[1] = state->colourmod[1]/32.0f;
		ent->shaderRGBAf[2] = state->colourmod[2]/32.0f;
		ent->shaderRGBAf[3] = state->alpha/255.0f;
		if (state->alpha != 255)
			ent->flags |= RF_TRANSLUCENT;

		ent->fatness = state->fatness;
		//
		// angles
		//
		angles[PITCH] = -state->viewangles[PITCH]/3;
		angles[YAW] = state->viewangles[YAW];
		angles[ROLL] = 0;
		angles[ROLL] = V_CalcRoll (angles, state->velocity)*4;

		if (j+1 == r_refdef.playerview->viewentity || (r_refdef.playerview->cam_state == CAM_EYECAM && r_refdef.playerview->cam_spec_track == j))
			ent->flags |= RF_EXTERNALMODEL;
		// the player object gets added with flags | 2
		for (pnum = 0; pnum < cl.splitclients; pnum++)
		{
			playerview_t *pv = &cl.playerview[pnum];
			if (j == pv->playernum)
			{
/*				if (cl.spectator)
				{
					cl_numvisedicts--;
					continue;
				}
*/				angles[0] = -1*pv->viewangles[0] / 3;
				angles[1] = pv->viewangles[1];
				angles[2] = pv->viewangles[2];
				ent->origin[0] = pv->simorg[0];
				ent->origin[1] = pv->simorg[1];
				ent->origin[2] = pv->simorg[2]+pv->crouch;
			}
		}

		if (model && model->type == mod_alias)
		{
			angles[0]*=r_meshpitch.value;	//carmack screwed up when he added alias models - they pitch the wrong way.
			angles[2]*=r_meshroll.value;	//hexen2 screwed it up even more - they roll the wrong way.
		}
		VectorCopy(angles, ent->angles);
		AngleVectors(angles, ent->axis[0], ent->axis[1], ent->axis[2]);
		VectorInverse(ent->axis[1]);

		// only predict half the move to minimize overruns
		msec = predictmsmult*(playertime - state->state_time);

		if (pnum < cl.splitclients)
		{	//this is a local player
		}
		else if (cl.do_lerp_players)
		{
			lerpents_t *le = &cl.lerpplayers[j];
			VectorCopy (le->origin, ent->origin);

			VectorCopy(le->angles, ent->angles);
			ent->angles[0] /= 3;
			AngleVectors(ent->angles, ent->axis[0], ent->axis[1], ent->axis[2]);
			VectorInverse(ent->axis[1]);
		}
		else if (msec <= 0 || (!predictplayers))
		{
			VectorCopy (state->origin, ent->origin);
//Con_DPrintf ("nopredict\n");
		}
		else
		{
			// predict players movement
			if (msec > 250)
				msec = 250;
			state->command.msec = msec;
//Con_DPrintf ("predict: %i\n", msec);

			oldphysent = pmove.numphysent;
			CL_SetSolidPlayers ();
			CL_PredictUsercmd (0, j+1, state, &exact, &state->command);	//uses player 0's maxspeed/grav...
			pmove.numphysent = oldphysent;
			VectorCopy (exact.origin, ent->origin);
		}

		VectorCopy(ent->origin, nametagorg[j]);
		nametagseen[j] = true;

		if (state->effects & QWEF_FLAG1)
			CL_AddFlagModels (ent, 0);
		else if (state->effects & QWEF_FLAG2)
			CL_AddFlagModels (ent, 1);
#ifdef HAVE_LEGACY
		if (modelindex2)
			CL_AddVWeapModel (ent, cl.model_precache_vwep[modelindex2]);
#endif

		CLQ1_AddShadow(ent);
		CLQ1_AddPowerupShell(ent, false, state->effects);

		if ((r_showbboxes.ival & 3) == 3)
		{
			vec3_t min, max;
			shader_t *s = R_RegisterShader("bboxshader", SUF_NONE, NULL);
			if (s)
			{
				VectorAdd(state->origin, pmove.player_mins, min);
				VectorAdd(state->origin, pmove.player_maxs, max);
				CLQ1_AddOrientedCube(s, min, max, NULL, 0.1, 0, 0, 1);

				VectorAdd(ent->origin, pmove.player_mins, min);
				VectorAdd(ent->origin, pmove.player_maxs, max);
				CLQ1_AddOrientedCube(s, min, max, NULL, 0, 0, 0.1, 1);
			}
		}


		if (r_torch.ival)
		{
			dlight_t *dl;
			dl = CL_NewDlight(j+1, ent->origin, 300, r_torch.ival, 0.5, 0.5, 0.2);
			dl->flags |= LFLAG_SHADOWMAP|LFLAG_FLASHBLEND;
			dl->fov = 60;
			angles[0] *= 3;
			angles[1] += sin(realtime)*8;
			angles[0] += cos(realtime*1.13)*5;
			AngleVectors(angles, dl->axis[0], dl->axis[1], dl->axis[2]);
		}
	}
}

void CL_LinkViewModel(void)
{
#ifdef QUAKESTATS
	extern cvar_t r_viewpreselgun;
	entity_t	ent;

	unsigned int plnum;
	unsigned int playereffects;
	float alpha;
	playerview_t *pv = r_refdef.playerview;
	const char *preselectedmodelname;

	extern cvar_t cl_gunx, cl_guny, cl_gunz;
	extern cvar_t cl_gunanglex, cl_gunangley, cl_gunanglez;

	if (r_drawviewmodel.value <= 0 || !Cam_DrawViewModel(r_refdef.playerview))
		return;

#ifdef Q2CLIENT
	if (cls.protocol == CP_QUAKE2)
	{
		//generate root matrix..
		VectorCopy(pv->simorg, r_refdef.weaponmatrix[3]);
		AngleVectors(pv->simangles, r_refdef.weaponmatrix[0], r_refdef.weaponmatrix[1], r_refdef.weaponmatrix[2]);
		VectorInverse(r_refdef.weaponmatrix[1]);
		memcpy(r_refdef.weaponmatrix_bob, r_refdef.weaponmatrix, sizeof(r_refdef.weaponmatrix_bob));

		V_ClearEntity(&ent);
		ent.model = pv->vm.oldmodel;

		ent.framestate.g[FS_REG].frame[0] = pv->vm.prevframe;
		ent.framestate.g[FS_REG].frame[1] = pv->vm.oldframe;
		ent.framestate.g[FS_REG].frametime[0] = cl.time-pv->vm.lerptime;
		ent.framestate.g[FS_REG].frametime[1] = cl.time-pv->vm.oldlerptime;
		ent.framestate.g[FS_REG].lerpweight[0] = (cl.time - pv->vm.lerptime)*10;
		ent.framestate.g[FS_REG].lerpweight[1] = 1 - ent.framestate.g[FS_REG].lerpweight[0];

		ent.flags |= RF_WEAPONMODEL|RF_DEPTHHACK|RF_NOSHADOW;
		if (pv->handedness == 1)
			ent.flags |= RF_XFLIP;
		else if (pv->handedness == 2)
			return;

		ent.shaderRGBAf[0] = ent.shaderRGBAf[1] = ent.shaderRGBAf[2] = 1;
		ent.shaderRGBAf[3] = bound(0, r_drawviewmodel.value, 1);

		V_AddEntity (&ent);
		return;
	}
#endif

	if (!r_drawentities.ival)
		return;

	if ((r_refdef.playerview->stats[STAT_ITEMS] & IT_INVISIBILITY) && r_drawviewmodelinvis.value <= 0)
		return;

	if (r_refdef.playerview->stats[STAT_HEALTH] <= 0)
		return;

	if (cl.intermissionmode != IM_NONE)
		return;

	if (pv->stats[STAT_WEAPONMODELI] <= 0 || pv->stats[STAT_WEAPONMODELI] >= MAX_PRECACHE_MODELS)
		return;

	if (r_drawviewmodel.value > 0 && r_drawviewmodel.value < 1)
		alpha = r_drawviewmodel.value;
	else
		alpha = 1;

	if ((pv->stats[STAT_ITEMS] & IT_INVISIBILITY)
		&& r_drawviewmodelinvis.value > 0
		&& r_drawviewmodelinvis.value < 1)
		alpha *= r_drawviewmodelinvis.value;

	//FIXME: scale alpha by the player's alpha too

	if (alpha <= 0)
		return;

	V_ClearEntity(&ent);

#ifdef PEXT_SCALE
	ent.scale = 1;
#endif

	ent.origin[0] = cl_gunz.value;
	ent.origin[1] = -cl_gunx.value;
	ent.origin[2] = -cl_guny.value;

	ent.angles[0] = cl_gunanglex.value;
	ent.angles[1] = cl_gunangley.value;
	ent.angles[2] = cl_gunanglez.value;

	ent.glowmod[0] = ent.glowmod[1] = ent.glowmod[2] = 1;
	ent.shaderRGBAf[0] = ent.shaderRGBAf[1] = ent.shaderRGBAf[2] = 1;
	ent.shaderRGBAf[3] = alpha;
	if (alpha != 1)
	{
		ent.flags |= RF_TRANSLUCENT;
	}

	preselectedmodelname = r_viewpreselgun.ival?IN_GetPreselectedViewmodelName(pv-cl.playerview):NULL;
	if (preselectedmodelname)
		ent.model = Mod_ForName(preselectedmodelname, MLV_SILENT);
	else
		ent.model = NULL;
	if (!ent.model)
	ent.model = cl.model_precache[pv->stats[STAT_WEAPONMODELI]];
	if (!ent.model)
	{
		pv->vm.oldmodel = NULL;
		return;
	}

#ifdef HLCLIENT
	if (!CLHL_AnimateViewEntity(&ent))
#endif
	{
		//if the model changed, reset everything.
		if (ent.model != pv->vm.oldmodel)
		{
			pv->vm.oldmodel = ent.model;
			pv->vm.oldframe = pv->vm.prevframe = pv->stats[STAT_WEAPONFRAME];
			pv->vm.oldlerptime = pv->vm.lerptime = cl.time;
			pv->vm.frameduration = 0.1;
		}
		//if the frame changed, update the oldframe to lerp into the new frame
		else if (pv->stats[STAT_WEAPONFRAME] != pv->vm.prevframe)
		{
			pv->vm.oldframe = pv->vm.prevframe;
			pv->vm.prevframe = pv->stats[STAT_WEAPONFRAME];
			pv->vm.oldlerptime = pv->vm.lerptime;

			pv->vm.frameduration = (cl.time - pv->vm.lerptime);
			if (pv->vm.frameduration < 0.01)//no faster than 100 times a second... to avoid divide by zero
				pv->vm.frameduration = 0.01;
			if (pv->vm.frameduration > 0.2)	//no slower than 5 times a second
				pv->vm.frameduration = 0.2;
			pv->vm.lerptime = cl.time;
		}
		//work out the blend fraction
		ent.framestate.g[FS_REG].frame[0] = pv->vm.prevframe;
		ent.framestate.g[FS_REG].frame[1] = pv->vm.oldframe;
		ent.framestate.g[FS_REG].frametime[0] = cl.time - pv->vm.lerptime;
		ent.framestate.g[FS_REG].frametime[1] = cl.time - pv->vm.oldlerptime;
		ent.framestate.g[FS_REG].lerpweight[0] = (cl.time-pv->vm.lerptime)/pv->vm.frameduration;
		ent.framestate.g[FS_REG].lerpweight[0] = bound(0, ent.framestate.g[FS_REG].lerpweight[0], 1);
		ent.framestate.g[FS_REG].lerpweight[1] = 1-ent.framestate.g[FS_REG].lerpweight[0];
	}

	ent.flags |= RF_WEAPONMODEL|RF_DEPTHHACK|RF_NOSHADOW;

	plnum = -1;
	if (pv->spectator)
		plnum = Cam_TrackNum(pv);
	if (plnum == -1)
		plnum = r_refdef.playerview->playernum;
	playereffects = 0;
	if (r_refdef.playerview->nolocalplayer && plnum < cl.maxlerpents)
	{
		if (plnum+1 < cl.maxlerpents)
		{
			lerpents_t *le = &cl.lerpents[plnum+1];
			if (le->entstate)
			{
				playereffects = le->entstate->effects;
#ifdef HEXEN2
				if (!le->entstate->modelindex || (le->entstate->hexen2flags & DRF_TRANSLUCENT))
				{	//urgh.
					ent.shaderRGBAf[3] *= .5;
					ent.flags |= RF_TRANSLUCENT;
				}
#endif
			}
		}
	}
	else if (plnum < cl.allocated_client_slots)
		playereffects = cl.inframes[parsecountmod].playerstate[plnum].effects;

	if (playereffects & DPEF_NOGUNBOB)
		ent.flags |= RF_WEAPONMODELNOBOB;

/*	ent.topcolour = TOP_DEFAULT;//cl.players[plnum].ttopcolor;
	ent.bottomcolour = cl.players[plnum].tbottomcolor;
	ent.h2playerclass = cl.players[plnum].h2playerclass;
*/
	CLQ1_AddPowerupShell(V_AddEntity(&ent), true, playereffects);

	//small hack to mask depth so only the front faces of the weaponmodel appear (no glitchy intra faces).
	if (alpha < 1 && qrenderer == QR_OPENGL)
	{
		ent.forcedshader = 	R_RegisterShader("viewmodeldepthmask", SUF_NONE,
				"{\n"
					"noshadows\n"
					"surfaceparm nodlight\n"
					"{\n"
						"map $whiteimage\n"
						"maskcolor\n"
						"depthwrite\n"
					"}\n"
				"}\n"
				);
		ent.shaderRGBAf[3] = 1;
		ent.flags &= ~RF_TRANSLUCENT;
		V_AddEntity(&ent);
		ent.forcedshader = NULL;
		ent.shaderRGBAf[3] = alpha;
		ent.flags |= RF_TRANSLUCENT;
	}
#endif
}

//======================================================================

/*
===============
CL_SetSolid

Builds all the pmove physents for the current frame
===============
*/
void CL_SetSolidEntities (void)
{
	int		i;
	inframe_t	*frame;
	packet_entities_t	*pak;
	entity_state_t		*state;
	physent_t			*pent;
	model_t				*mod;

	VALGRIND_MAKE_MEM_UNDEFINED(&pmove, sizeof(pmove));
#ifdef CSQC_DAT
	pmove.world = &csqc_world;
#endif

	memset(&pmove.physents[0], 0, sizeof(physent_t));
	pmove.physents[0].model = cl.worldmodel;
	VectorClear (pmove.physents[0].origin);
	pmove.physents[0].info = 0;
	pmove.numphysent = 1;

	frame = &cl.inframes[cl.validsequence&UPDATE_MASK];
	pak = &frame->packet_entities;

	for (i=0 ; i<pak->num_entities ; i++)
	{
		state = &pak->entities[i];

		if (state->solidsize==ES_SOLID_NOT)
			continue;

		if (state->solidsize == ES_SOLID_BSP)
		{	/*bsp model size*/
			if (state->modelindex <= 0)
				continue;
			mod = cl.model_precache[state->modelindex];
			if (!mod || mod->loadstate != MLS_LOADED)
				continue;
			/*vanilla protocols have no 'solid' information. all entities get assigned ES_SOLID_BSP, even if its not actually solid.
			so we need to make sure that item pickups are not erroneously considered solid, but doors etc are.
			normally, ONLY inline models are considered solid when we have no solid info.
			monsters will always be non-solid, too.
			*/
			if (!(cls.fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS) && mod->numsubmodels <= 1)
				continue;
	
			pent = &pmove.physents[pmove.numphysent];
			memset(pent, 0, sizeof(physent_t));
			pent->model = mod;
			VectorCopy (state->angles, pent->angles);
			pent->angles[0]*=r_meshpitch.value;
			pent->angles[2]*=r_meshroll.value;
		}
		else
		{
			pent = &pmove.physents[pmove.numphysent];
			memset(pent, 0, sizeof(physent_t));
			pent->info = state->number;
			/*don't bother with angles*/
			COM_DecodeSize(state->solidsize, pent->mins, pent->maxs);
		}
		if (++pmove.numphysent == MAX_PHYSENTS)
			break;
		VectorCopy(state->origin, pent->origin);
		pent->info = state->number;

		switch((int)state->skinnum)
		{
		case 0:
			break;
		case Q1CONTENTS_LADDER:
			pent->nonsolid = true;
			pent->forcecontentsmask = FTECONTENTS_LADDER;
			break;
		case Q1CONTENTS_SKY:
			pent->nonsolid = true;
			pent->forcecontentsmask = FTECONTENTS_SKY;
			break;
		case Q1CONTENTS_LAVA:
			pent->nonsolid = true;
			pent->forcecontentsmask = FTECONTENTS_LAVA;
			break;
		case Q1CONTENTS_SLIME:
			pent->nonsolid = true;
			pent->forcecontentsmask = FTECONTENTS_SLIME;
			break;
		case Q1CONTENTS_WATER:
			pent->nonsolid = true;
			pent->forcecontentsmask = FTECONTENTS_WATER;
			break;
		}
	}

}

/*
===
Calculate the new position of players, without other player clipping

We do this to set up real player prediction.
Players are predicted twice, first without clipping other players,
then with clipping against them.
This sets up the first phase.
===
*/
void CL_SetUpPlayerPrediction(qboolean dopred)
{
	int				j;
	player_state_t	*state;
	player_state_t	exact;
	double			playertime;
	int				msec;
	inframe_t			*frame;
	struct predicted_player *pplayer;
	extern cvar_t cl_nopred, cl_demospeed;
	float predictmsmult = 1000*cl_predict_players_frac.value;

	int s;

	playertime = realtime - cls.latency*cl_predict_players_latency.value + cl_predict_players_nudge.value;
	if (playertime > realtime)
		playertime = realtime;

	if (cl_nopred.value || /*cls.demoplayback ||*/ cl.paused || cl.worldmodel->loadstate != MLS_LOADED)
		return;

	if (cls.demoplayback)
		predictmsmult *= cl_demospeed.value;

	frame = &cl.inframes[cl.parsecount&UPDATE_MASK];

	for (j=0, pplayer = predicted_players, state=frame->playerstate;
		j < cl.allocated_client_slots;
		j++, pplayer++, state++)
	{

		pplayer->active = false;

		if (state->messagenum != cl.parsecount)
			continue;	// not present this frame

		if (!state->modelindex)
			continue;

		pplayer->active = true;
		pplayer->flags = state->flags;

		// note that the local players are special, since they move locally
		// we use their last predicted postition
		for (s = 0; s < cl.splitclients; s++)
		{
			if (j == cl.playerview[s].playernum)
			{
				VectorCopy(cl.inframes[cls.netchan.outgoing_sequence&UPDATE_MASK].playerstate[cl.playerview[s].playernum].origin, pplayer->origin);
				break;
			}
		}
		if (s == cl.splitclients)
		{
			// only predict half the move to minimize overruns
			msec = predictmsmult*(playertime - state->state_time);
			if (msec <= 0 ||
				!cl_predict_players.ival ||
				!dopred)
			{
				VectorCopy (state->origin, pplayer->origin);
	//Con_DPrintf ("nopredict\n");
			}
			else
			{
				// predict players movement
				if (msec > 250)
					msec = 250;
				state->command.msec = msec;
	//Con_DPrintf ("predict: %i\n", msec);

				CL_PredictUsercmd (0, j+1, state, &exact, &state->command);
				VectorCopy (exact.origin, pplayer->origin);
			}
		}
	}
}

/*
===============
CL_SetSolid

Builds all the pmove physents for the current frame
Note that CL_SetUpPlayerPrediction() must be called first!
pmove must be setup with world and solid entity hulls before calling
(via CL_PredictMove)
===============
*/
void CL_SetSolidPlayers (void)
{
	int		j;
	struct predicted_player *pplayer;
	physent_t *pent;

	if (!cl_solid_players.ival)
		return;

	pent = pmove.physents + pmove.numphysent;

	if (pmove.numphysent == MAX_PHYSENTS)	//too many.
		return;

	for (j=0, pplayer = predicted_players; j < cl.allocated_client_slots;	j++, pplayer++)
	{
		if (!pplayer->active)
			continue;	// not present this frame
		if (!(pplayer->flags & PF_SOLID))
			continue;

		memset(pent, 0, sizeof(physent_t));
		VectorCopy(pplayer->origin, pent->origin);
		pent->info = j+1;
		VectorCopy(pmove.player_mins, pent->mins);
		VectorCopy(pmove.player_maxs, pent->maxs);
		if (++pmove.numphysent == MAX_PHYSENTS)	//we just hit 88 miles per hour.
			break;
		pent++;
	}
}

/*
===============
CL_EmitEntities

Builds the visedicts array for cl.time

Made up of: clients, packet_entities, nails, and tents
===============
*/
void CL_ClearEntityLists(void)
{
	cl_framecount++;
	if (cl_expandvisents || cl_numvisedicts+128 >= cl_maxvisedicts)
	{
		int newnum = cl_maxvisedicts + 256;
		entity_t *n = BZ_Realloc(cl_visedicts, newnum * sizeof(*n));
		if (n)
		{
			cl_visedicts = n;
			cl_maxvisedicts = newnum;
		}
		cl_expandvisents = false;
	}
	cl_numvisedicts = 0;
	cl_numstrisidx = 0;
	cl_numstrisvert = 0;
	cl_numstris = 0;
}
void CL_FreeVisEdicts(void)
{
	cl_framecount++;
	BZ_Free(cl_visedicts);
	cl_visedicts = NULL;
	cl_maxvisedicts = 0;
	cl_numvisedicts = 0;
}
/*
static void CL_WaterSplashes(void)
{
	int i;
	entity_t *ent;
	vec3_t org;

	static unsigned int ltime;
	unsigned int ntime = cl.time*1000;
	if (ntime - ltime < 200)
		return;
	ltime = ntime;

	for (i = 0; i < cl_numvisedicts; i++)
	{
		ent = &cl_visedicts[i];

		if (ent->model)
		{
			if (ent->origin[2] + ent->model->mins[2] < r_refdef.waterheight &&
				ent->origin[2] + ent->model->maxs[2] > r_refdef.waterheight)
			{
				org[0] = ent->origin[0];
				org[1] = ent->origin[1];
				org[2] = r_refdef.waterheight;
				P_RunParticleEffectTypeString(org, NULL, 1, "te_watertransition");
			}
		}
	}
}
*/
void CL_EmitEntities (void)
{
	if (cls.state != ca_active)
		return;

	CL_DecayLights ();

#ifdef Q2CLIENT
	if (cls.protocol == CP_QUAKE2)
	{
		CL_ClearEntityLists();
		CLQ2_AddEntities();
		return;
	}
#endif
	if (!cl.validsequence)
		return;

	CL_ClearEntityLists();

	CL_LinkPlayers ();
	CL_LinkPacketEntities ();
	CL_LinkProjectiles ();
	CL_UpdateTEnts ();

//	CL_WaterSplashes();
}











void CL_ClearPredict(void)
{
	memset(predicted_players, 0, sizeof(predicted_players));
}

