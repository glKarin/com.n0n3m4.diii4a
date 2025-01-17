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

#include "quakedef.h"
#include "sv_demo.h"

extern cvar_t sv_airaccel_qw_stretchfactor;
extern cvar_t sv_qcstats;
extern cvar_t sv_warsowbunny_airforwardaccel;
extern cvar_t sv_warsowbunny_accel;
extern cvar_t sv_warsowbunny_topspeed;
extern cvar_t sv_warsowbunny_turnaccel;
extern cvar_t sv_warsowbunny_backtosideratio;
extern cvar_t sv_onlycsqcnetworking;
extern cvar_t sv_cullentities_trace_entityocclusion;
extern cvar_t sv_cullentities_trace_samples_players;
extern cvar_t sv_cullentities_trace_eyejitter;
extern cvar_t sv_cullentities_trace_expand;
extern cvar_t sv_cullentities_trace_delay_players;
extern cvar_t sv_cullentities_trace_spectators;

/*
=============================================================================

EVENT MESSAGES

=============================================================================
*/

/*
=================
SV_ClientPrint

Sends text across to be displayed
FIXME: make this just a stuffed echo?
=================
*/
void SV_ClientPrint(const char *msg)
{
	if (host_client->netconnection)
	{
		MSG_WriteByte(&host_client->netconnection->message, svc_print);
		MSG_WriteString(&host_client->netconnection->message, msg);
	}
}

/*
=================
SV_ClientPrintf

Sends text across to be displayed
FIXME: make this just a stuffed echo?
=================
*/
void SV_ClientPrintf(const char *fmt, ...)
{
	va_list argptr;
	char msg[MAX_INPUTLINE];

	va_start(argptr,fmt);
	dpvsnprintf(msg,sizeof(msg),fmt,argptr);
	va_end(argptr);

	SV_ClientPrint(msg);
}

/*
=================
SV_BroadcastPrint

Sends text to all active clients
=================
*/
void SV_BroadcastPrint(const char *msg)
{
	int i;
	client_t *client;

	for (i = 0, client = svs.clients;i < svs.maxclients;i++, client++)
	{
		if (client->active && client->netconnection)
		{
			MSG_WriteByte(&client->netconnection->message, svc_print);
			MSG_WriteString(&client->netconnection->message, msg);
		}
	}

	if (sv_echobprint.integer && !host_isclient.integer)
		Con_Print(msg);
}

/*
=================
SV_BroadcastPrintf

Sends text to all active clients
=================
*/
void SV_BroadcastPrintf(const char *fmt, ...)
{
	va_list argptr;
	char msg[MAX_INPUTLINE];

	va_start(argptr,fmt);
	dpvsnprintf(msg,sizeof(msg),fmt,argptr);
	va_end(argptr);

	SV_BroadcastPrint(msg);
}

/*
=================
SV_ClientCommands

Send text over to the client to be executed
=================
*/
void SV_ClientCommands(const char *fmt, ...)
{
	va_list argptr;
	char string[MAX_INPUTLINE];

	if (!host_client->netconnection)
		return;

	va_start(argptr,fmt);
	dpvsnprintf(string, sizeof(string), fmt, argptr);
	va_end(argptr);

	MSG_WriteByte(&host_client->netconnection->message, svc_stufftext);
	MSG_WriteString(&host_client->netconnection->message, string);
}

/*
==================
SV_StartParticle

Make sure the event gets sent to all clients
==================
*/
void SV_StartParticle (vec3_t org, vec3_t dir, int color, int count)
{
	int i;

	if (sv.datagram.cursize > MAX_PACKETFRAGMENT-18)
		return;
	MSG_WriteByte (&sv.datagram, svc_particle);
	MSG_WriteCoord (&sv.datagram, org[0], sv.protocol);
	MSG_WriteCoord (&sv.datagram, org[1], sv.protocol);
	MSG_WriteCoord (&sv.datagram, org[2], sv.protocol);
	for (i=0 ; i<3 ; i++)
		MSG_WriteChar (&sv.datagram, (int)bound(-128, dir[i]*16, 127));
	MSG_WriteByte (&sv.datagram, count);
	MSG_WriteByte (&sv.datagram, color);
	SV_FlushBroadcastMessages();
}

/*
==================
SV_StartEffect

Make sure the event gets sent to all clients
==================
*/
void SV_StartEffect (vec3_t org, int modelindex, int startframe, int framecount, int framerate)
{
	if (modelindex >= 256 || startframe >= 256)
	{
		if (sv.datagram.cursize > MAX_PACKETFRAGMENT-19)
			return;
		MSG_WriteByte (&sv.datagram, svc_effect2);
		MSG_WriteCoord (&sv.datagram, org[0], sv.protocol);
		MSG_WriteCoord (&sv.datagram, org[1], sv.protocol);
		MSG_WriteCoord (&sv.datagram, org[2], sv.protocol);
		MSG_WriteShort (&sv.datagram, modelindex);
		MSG_WriteShort (&sv.datagram, startframe);
		MSG_WriteByte (&sv.datagram, framecount);
		MSG_WriteByte (&sv.datagram, framerate);
	}
	else
	{
		if (sv.datagram.cursize > MAX_PACKETFRAGMENT-17)
			return;
		MSG_WriteByte (&sv.datagram, svc_effect);
		MSG_WriteCoord (&sv.datagram, org[0], sv.protocol);
		MSG_WriteCoord (&sv.datagram, org[1], sv.protocol);
		MSG_WriteCoord (&sv.datagram, org[2], sv.protocol);
		MSG_WriteByte (&sv.datagram, modelindex);
		MSG_WriteByte (&sv.datagram, startframe);
		MSG_WriteByte (&sv.datagram, framecount);
		MSG_WriteByte (&sv.datagram, framerate);
	}
	SV_FlushBroadcastMessages();
}

/*
==================
SV_StartSound

Each entity can have eight independant sound sources, like voice,
weapon, feet, etc.

Channel 0 is an auto-allocate channel, the others override anything
already running on that entity/channel pair.

An attenuation of 0 will play full volume everywhere in the level.
Larger attenuations will drop off.  (max 4 attenuation)

==================
*/
void SV_StartSound (prvm_edict_t *entity, int channel, const char *sample, int nvolume, float attenuation, qbool reliable, float speed)
{
	prvm_prog_t *prog = SVVM_prog;
	sizebuf_t *dest;
	int sound_num, field_mask, i, ent, speed4000;

	dest = (reliable ? &sv.reliable_datagram : &sv.datagram);

	if (nvolume < 0 || nvolume > 255)
	{
		Con_Printf ("SV_StartSound: volume = %i\n", nvolume);
		return;
	}

	if (attenuation < 0 || attenuation > 4)
	{
		Con_Printf ("SV_StartSound: attenuation = %f\n", attenuation);
		return;
	}

	if (!IS_CHAN(channel))
	{
		Con_Printf ("SV_StartSound: channel = %i\n", channel);
		return;
	}

	channel = CHAN_ENGINE2NET(channel);

	if (sv.datagram.cursize > MAX_PACKETFRAGMENT-21)
		return;

// find precache number for sound
	sound_num = SV_SoundIndex(sample, 1);
	if (!sound_num)
		return;

	ent = PRVM_NUM_FOR_EDICT(entity);

	speed4000 = (int)floor(speed * 4000.0f + 0.5f);
	field_mask = 0;
	if (nvolume != DEFAULT_SOUND_PACKET_VOLUME)
		field_mask |= SND_VOLUME;
	if (attenuation != DEFAULT_SOUND_PACKET_ATTENUATION)
		field_mask |= SND_ATTENUATION;
	if (speed4000 && speed4000 != 4000)
		field_mask |= SND_SPEEDUSHORT4000;
	if (ent >= 8192 || channel < 0 || channel > 7)
		field_mask |= SND_LARGEENTITY;
	if (sound_num >= 256)
		field_mask |= SND_LARGESOUND;

// directed messages go only to the entity they are targeted on
	MSG_WriteByte (dest, svc_sound);
	MSG_WriteByte (dest, field_mask);
	if (field_mask & SND_VOLUME)
		MSG_WriteByte (dest, nvolume);
	if (field_mask & SND_ATTENUATION)
		MSG_WriteByte (dest, (int)(attenuation*64));
	if (field_mask & SND_SPEEDUSHORT4000)
		MSG_WriteShort (dest, speed4000);
	if (field_mask & SND_LARGEENTITY)
	{
		MSG_WriteShort (dest, ent);
		MSG_WriteChar (dest, channel);
	}
	else
		MSG_WriteShort (dest, (ent<<3) | channel);
	if ((field_mask & SND_LARGESOUND) || sv.protocol == PROTOCOL_NEHAHRABJP2 || sv.protocol == PROTOCOL_NEHAHRABJP3)
		MSG_WriteShort (dest, sound_num);
	else
		MSG_WriteByte (dest, sound_num);
	for (i = 0;i < 3;i++)
		MSG_WriteCoord (dest, PRVM_serveredictvector(entity, origin)[i]+0.5*(PRVM_serveredictvector(entity, mins)[i]+PRVM_serveredictvector(entity, maxs)[i]), sv.protocol);

	// TODO do we have to do anything here when dest is &sv.reliable_datagram?
	if(!reliable)
		SV_FlushBroadcastMessages();
}

/*
==================
SV_StartPointSound

Nearly the same logic as SV_StartSound, except an origin
instead of an entity is provided and channel is omitted.

The entity sent to the client is 0 (world) and the channel
is 0 (CHAN_AUTO).  SND_LARGEENTITY will never occur in this
function, therefore the check for it is omitted.

==================
*/
void SV_StartPointSound (vec3_t origin, const char *sample, int nvolume, float attenuation, float speed)
{
	int sound_num, field_mask, i, speed4000;

	if (nvolume < 0 || nvolume > 255)
	{
		Con_Printf ("SV_StartPointSound: volume = %i\n", nvolume);
		return;
	}

	if (attenuation < 0 || attenuation > 4)
	{
		Con_Printf ("SV_StartPointSound: attenuation = %f\n", attenuation);
		return;
	}

	if (sv.datagram.cursize > MAX_PACKETFRAGMENT-21)
		return;

	// find precache number for sound
	sound_num = SV_SoundIndex(sample, 1);
	if (!sound_num)
		return;

	speed4000 = (int)(speed * 40.0f);
	field_mask = 0;
	if (nvolume != DEFAULT_SOUND_PACKET_VOLUME)
		field_mask |= SND_VOLUME;
	if (attenuation != DEFAULT_SOUND_PACKET_ATTENUATION)
		field_mask |= SND_ATTENUATION;
	if (sound_num >= 256)
		field_mask |= SND_LARGESOUND;
	if (speed4000 && speed4000 != 4000)
		field_mask |= SND_SPEEDUSHORT4000;

// directed messages go only to the entity they are targeted on
	MSG_WriteByte (&sv.datagram, svc_sound);
	MSG_WriteByte (&sv.datagram, field_mask);
	if (field_mask & SND_VOLUME)
		MSG_WriteByte (&sv.datagram, nvolume);
	if (field_mask & SND_ATTENUATION)
		MSG_WriteByte (&sv.datagram, (int)(attenuation*64));
	if (field_mask & SND_SPEEDUSHORT4000)
		MSG_WriteShort (&sv.datagram, speed4000);
	// Always write entnum 0 for the world entity
	MSG_WriteShort (&sv.datagram, (0<<3) | 0);
	if (field_mask & SND_LARGESOUND)
		MSG_WriteShort (&sv.datagram, sound_num);
	else
		MSG_WriteByte (&sv.datagram, sound_num);
	for (i = 0;i < 3;i++)
		MSG_WriteCoord (&sv.datagram, origin[i], sv.protocol);
	SV_FlushBroadcastMessages();
}

/*
===============================================================================

FRAME UPDATES

===============================================================================
*/

/*
=============================================================================

The PVS must include a small area around the client to allow head bobbing
or other small motion on the client side.  Otherwise, a bob might cause an
entity that should be visible to not show up, especially when the bob
crosses a waterline.

=============================================================================
*/

static qbool SV_PrepareEntityForSending (prvm_edict_t *ent, entity_state_t *cs, int enumber)
{
	prvm_prog_t *prog = SVVM_prog;
	int i;
	unsigned int sendflags;
	unsigned int version;
	unsigned int modelindex, effects, flags, glowsize, lightstyle, lightpflags, light[4], specialvisibilityradius;
	unsigned int customizeentityforclient;
	unsigned int sendentity;
	float f;
	prvm_vec_t *v;
	vec3_t cullmins, cullmaxs;
	model_t *model;

	// fast path for games that do not use legacy entity networking
	// note: still networks clients even if they are legacy
	sendentity = PRVM_serveredictfunction(ent, SendEntity);
	if (sv_onlycsqcnetworking.integer && !sendentity && enumber > svs.maxclients)
		return false;

	// this 2 billion unit check is actually to detect NAN origins
	// (we really don't want to send those)
	if (!(VectorLength2(PRVM_serveredictvector(ent, origin)) < 2000000000.0*2000000000.0))
		return false;

	// EF_NODRAW prevents sending for any reason except for your own
	// client, so we must keep all clients in this superset
	effects = (unsigned)PRVM_serveredictfloat(ent, effects);

	// we can omit invisible entities with no effects that are not clients
	// LadyHavoc: this could kill tags attached to an invisible entity, I
	// just hope we never have to support that case
	i = (int)PRVM_serveredictfloat(ent, modelindex);
	modelindex = (i >= 1 && i < MAX_MODELS && PRVM_serveredictstring(ent, model) && *PRVM_GetString(prog, PRVM_serveredictstring(ent, model)) && sv.models[i]) ? i : 0;

	flags = 0;
	i = (int)(PRVM_serveredictfloat(ent, glow_size) * 0.25f);
	glowsize = (unsigned char)bound(0, i, 255);
	if (PRVM_serveredictfloat(ent, glow_trail))
		flags |= RENDER_GLOWTRAIL;
	if (PRVM_serveredictedict(ent, viewmodelforclient))
		flags |= RENDER_VIEWMODEL;

	v = PRVM_serveredictvector(ent, color);
	f = v[0]*256;
	light[0] = (unsigned short)bound(0, f, 65535);
	f = v[1]*256;
	light[1] = (unsigned short)bound(0, f, 65535);
	f = v[2]*256;
	light[2] = (unsigned short)bound(0, f, 65535);
	f = PRVM_serveredictfloat(ent, light_lev);
	light[3] = (unsigned short)bound(0, f, 65535);
	lightstyle = (unsigned char)PRVM_serveredictfloat(ent, style);
	lightpflags = (unsigned char)PRVM_serveredictfloat(ent, pflags);

	if (gamemode == GAME_TENEBRAE)
	{
		// tenebrae's EF_FULLDYNAMIC conflicts with Q2's EF_NODRAW
		if (effects & 16)
		{
			effects &= ~16;
			lightpflags |= PFLAGS_FULLDYNAMIC;
		}
		// tenebrae's EF_GREEN conflicts with DP's EF_ADDITIVE
		if (effects & 32)
		{
			effects &= ~32;
			light[0] = (int)(0.2*256);
			light[1] = (int)(1.0*256);
			light[2] = (int)(0.2*256);
			light[3] = 200;
			lightpflags |= PFLAGS_FULLDYNAMIC;
		}
	}

	specialvisibilityradius = 0;
	if (lightpflags & PFLAGS_FULLDYNAMIC)
		specialvisibilityradius = max(specialvisibilityradius, light[3]);
	if (glowsize)
		specialvisibilityradius = max(specialvisibilityradius, glowsize * 4);
	if (flags & RENDER_GLOWTRAIL)
		specialvisibilityradius = max(specialvisibilityradius, 100);
	if (effects & (EF_BRIGHTFIELD | EF_MUZZLEFLASH | EF_BRIGHTLIGHT | EF_DIMLIGHT | EF_RED | EF_BLUE | EF_FLAME | EF_STARDUST))
	{
		if (effects & EF_BRIGHTFIELD)
			specialvisibilityradius = max(specialvisibilityradius, 80);
		if (effects & EF_MUZZLEFLASH)
			specialvisibilityradius = max(specialvisibilityradius, 100);
		if (effects & EF_BRIGHTLIGHT)
			specialvisibilityradius = max(specialvisibilityradius, 400);
		if (effects & EF_DIMLIGHT)
			specialvisibilityradius = max(specialvisibilityradius, 200);
		if (effects & EF_RED)
			specialvisibilityradius = max(specialvisibilityradius, 200);
		if (effects & EF_BLUE)
			specialvisibilityradius = max(specialvisibilityradius, 200);
		if (effects & EF_FLAME)
			specialvisibilityradius = max(specialvisibilityradius, 250);
		if (effects & EF_STARDUST)
			specialvisibilityradius = max(specialvisibilityradius, 100);
	}

	// early culling checks
	// (final culling is done by SV_MarkWriteEntityStateToClient)
	customizeentityforclient = PRVM_serveredictfunction(ent, customizeentityforclient);
	if (!customizeentityforclient && enumber > svs.maxclients && (!modelindex && !specialvisibilityradius))
		return false;

	*cs = defaultstate;
	cs->active = ACTIVE_NETWORK;
	cs->number = enumber;
	VectorCopy(PRVM_serveredictvector(ent, origin), cs->origin);
	VectorCopy(PRVM_serveredictvector(ent, angles), cs->angles);
	cs->flags = flags;
	cs->effects = effects;
	cs->colormap = (unsigned)PRVM_serveredictfloat(ent, colormap);
	cs->modelindex = modelindex;
	cs->skin = (unsigned)PRVM_serveredictfloat(ent, skin);
	cs->frame = (unsigned)PRVM_serveredictfloat(ent, frame);
	cs->viewmodelforclient = PRVM_serveredictedict(ent, viewmodelforclient);
	cs->exteriormodelforclient = PRVM_serveredictedict(ent, exteriormodeltoclient);
	cs->nodrawtoclient = PRVM_serveredictedict(ent, nodrawtoclient);
	cs->drawonlytoclient = PRVM_serveredictedict(ent, drawonlytoclient);
	cs->customizeentityforclient = customizeentityforclient;
	cs->tagentity = PRVM_serveredictedict(ent, tag_entity);
	cs->tagindex = (unsigned char)PRVM_serveredictfloat(ent, tag_index);
	cs->glowsize = glowsize;
	cs->traileffectnum = PRVM_serveredictfloat(ent, traileffectnum);

	// don't need to init cs->colormod because the defaultstate did that for us
	//cs->colormod[0] = cs->colormod[1] = cs->colormod[2] = 32;
	v = PRVM_serveredictvector(ent, colormod);
	if (VectorLength2(v))
	{
		i = (int)(v[0] * 32.0f);cs->colormod[0] = bound(0, i, 255);
		i = (int)(v[1] * 32.0f);cs->colormod[1] = bound(0, i, 255);
		i = (int)(v[2] * 32.0f);cs->colormod[2] = bound(0, i, 255);
	}

	// don't need to init cs->glowmod because the defaultstate did that for us
	//cs->glowmod[0] = cs->glowmod[1] = cs->glowmod[2] = 32;
	v = PRVM_serveredictvector(ent, glowmod);
	if (VectorLength2(v))
	{
		i = (int)(v[0] * 32.0f);cs->glowmod[0] = bound(0, i, 255);
		i = (int)(v[1] * 32.0f);cs->glowmod[1] = bound(0, i, 255);
		i = (int)(v[2] * 32.0f);cs->glowmod[2] = bound(0, i, 255);
	}

	cs->modelindex = modelindex;

	cs->alpha = 255;
	f = (PRVM_serveredictfloat(ent, alpha) * 255.0f);
	if (f)
	{
		i = (int)f;
		cs->alpha = (unsigned char)bound(0, i, 255);
	}
	// halflife
	f = (PRVM_serveredictfloat(ent, renderamt));
	if (f)
	{
		i = (int)f;
		cs->alpha = (unsigned char)bound(0, i, 255);
	}

	cs->scale = 16;
	f = (PRVM_serveredictfloat(ent, scale) * 16.0f);
	if (f)
	{
		i = (int)f;
		cs->scale = (unsigned char)bound(0, i, 255);
	}

	cs->glowcolor = 254;
	f = PRVM_serveredictfloat(ent, glow_color);
	if (f)
		cs->glowcolor = (int)f;

	if (PRVM_serveredictfloat(ent, fullbright))
		cs->effects |= EF_FULLBRIGHT;

	f = PRVM_serveredictfloat(ent, modelflags);
	if (f)
		cs->effects |= ((unsigned int)f & 0xff) << 24;

	if (PRVM_serveredictfloat(ent, movetype) == MOVETYPE_STEP || ((int)PRVM_serveredictfloat(ent, flags) & FL_MONSTER))
		cs->flags |= RENDER_STEP;
	if (cs->number != sv.writeentitiestoclient_cliententitynumber && (cs->effects & EF_LOWPRECISION) && cs->origin[0] >= -32768 && cs->origin[1] >= -32768 && cs->origin[2] >= -32768 && cs->origin[0] <= 32767 && cs->origin[1] <= 32767 && cs->origin[2] <= 32767)
		cs->flags |= RENDER_LOWPRECISION;
	if (PRVM_serveredictfloat(ent, colormap) >= 1024)
		cs->flags |= RENDER_COLORMAPPED;
	if (cs->viewmodelforclient)
		cs->flags |= RENDER_VIEWMODEL; // show relative to the view

	if (PRVM_serveredictfloat(ent, sendcomplexanimation))
	{
		cs->flags |= RENDER_COMPLEXANIMATION;
		if (PRVM_serveredictfloat(ent, skeletonindex) >= 1)
			cs->skeletonobject = ent->priv.server->skeleton;
		cs->framegroupblend[0].frame = PRVM_serveredictfloat(ent, frame);
		cs->framegroupblend[1].frame = PRVM_serveredictfloat(ent, frame2);
		cs->framegroupblend[2].frame = PRVM_serveredictfloat(ent, frame3);
		cs->framegroupblend[3].frame = PRVM_serveredictfloat(ent, frame4);
		cs->framegroupblend[0].start = PRVM_serveredictfloat(ent, frame1time);
		cs->framegroupblend[1].start = PRVM_serveredictfloat(ent, frame2time);
		cs->framegroupblend[2].start = PRVM_serveredictfloat(ent, frame3time);
		cs->framegroupblend[3].start = PRVM_serveredictfloat(ent, frame4time);
		cs->framegroupblend[1].lerp = PRVM_serveredictfloat(ent, lerpfrac);
		cs->framegroupblend[2].lerp = PRVM_serveredictfloat(ent, lerpfrac3);
		cs->framegroupblend[3].lerp = PRVM_serveredictfloat(ent, lerpfrac4);
		cs->framegroupblend[0].lerp = 1.0f - cs->framegroupblend[1].lerp - cs->framegroupblend[2].lerp - cs->framegroupblend[3].lerp;
		cs->frame = 0; // don't need the legacy frame
	}

	cs->light[0] = light[0];
	cs->light[1] = light[1];
	cs->light[2] = light[2];
	cs->light[3] = light[3];
	cs->lightstyle = lightstyle;
	cs->lightpflags = lightpflags;

	cs->specialvisibilityradius = specialvisibilityradius;

	// calculate the visible box of this entity (don't use the physics box
	// as that is often smaller than a model, and would not count
	// specialvisibilityradius)
	if ((model = SV_GetModelByIndex(modelindex)) && (model->type != mod_null))
	{
		float scale = cs->scale * (1.0f / 16.0f);
		if (cs->angles[0] || cs->angles[2]) // pitch and roll
		{
			VectorMA(cs->origin, scale, model->rotatedmins, cullmins);
			VectorMA(cs->origin, scale, model->rotatedmaxs, cullmaxs);
		}
		else if (cs->angles[1] || ((effects | model->effects) & EF_ROTATE))
		{
			VectorMA(cs->origin, scale, model->yawmins, cullmins);
			VectorMA(cs->origin, scale, model->yawmaxs, cullmaxs);
		}
		else
		{
			VectorMA(cs->origin, scale, model->normalmins, cullmins);
			VectorMA(cs->origin, scale, model->normalmaxs, cullmaxs);
		}
	}
	else
	{
		// if there is no model (or it could not be loaded), use the physics box
		VectorAdd(cs->origin, PRVM_serveredictvector(ent, mins), cullmins);
		VectorAdd(cs->origin, PRVM_serveredictvector(ent, maxs), cullmaxs);
	}
	if (specialvisibilityradius)
	{
		cullmins[0] = min(cullmins[0], cs->origin[0] - specialvisibilityradius);
		cullmins[1] = min(cullmins[1], cs->origin[1] - specialvisibilityradius);
		cullmins[2] = min(cullmins[2], cs->origin[2] - specialvisibilityradius);
		cullmaxs[0] = max(cullmaxs[0], cs->origin[0] + specialvisibilityradius);
		cullmaxs[1] = max(cullmaxs[1], cs->origin[1] + specialvisibilityradius);
		cullmaxs[2] = max(cullmaxs[2], cs->origin[2] + specialvisibilityradius);
	}

	// calculate center of bbox for network prioritization purposes
	VectorMAM(0.5f, cullmins, 0.5f, cullmaxs, cs->netcenter);

	// if culling box has moved, update pvs cluster links
	if (!VectorCompare(cullmins, ent->priv.server->cullmins) || !VectorCompare(cullmaxs, ent->priv.server->cullmaxs))
	{
		VectorCopy(cullmins, ent->priv.server->cullmins);
		VectorCopy(cullmaxs, ent->priv.server->cullmaxs);
		// a value of -1 for pvs_numclusters indicates that the links are not
		// cached, and should be re-tested each time, this is the case if the
		// culling box touches too many pvs clusters to store, or if the world
		// model does not support FindBoxClusters
		ent->priv.server->pvs_numclusters = -1;
		if (sv.worldmodel && sv.worldmodel->brush.FindBoxClusters)
		{
			i = sv.worldmodel->brush.FindBoxClusters(sv.worldmodel, cullmins, cullmaxs, MAX_ENTITYCLUSTERS, ent->priv.server->pvs_clusterlist);
			if (i <= MAX_ENTITYCLUSTERS)
				ent->priv.server->pvs_numclusters = i;
		}
	}

	// we need to do some csqc entity upkeep here
	// get self.SendFlags and clear them
	// (to let the QC know that they've been read)
	if (sendentity)
	{
		sendflags = (unsigned int)PRVM_serveredictfloat(ent, SendFlags);
		PRVM_serveredictfloat(ent, SendFlags) = 0;
		// legacy self.Version system
		if ((version = (unsigned int)PRVM_serveredictfloat(ent, Version)))
		{
			if (sv.csqcentityversion[enumber] != version)
				sendflags = 0xFFFFFF;
			sv.csqcentityversion[enumber] = version;
		}
		// move sendflags into the per-client sendflags
		if (sendflags)
			for (i = 0;i < svs.maxclients;i++)
				svs.clients[i].csqcentitysendflags[enumber] |= sendflags;
		// mark it as inactive for non-csqc networking
		cs->active = ACTIVE_SHARED;
	}

	return true;
}

static void SV_PrepareEntitiesForSending(void)
{
	prvm_prog_t *prog = SVVM_prog;
	int e;
	prvm_edict_t *ent;
	// send all entities that touch the pvs
	sv.numsendentities = 0;
	sv.sendentitiesindex[0] = NULL;
	memset(sv.sendentitiesindex, 0, prog->num_edicts * sizeof(*sv.sendentitiesindex));
	for (e = 1, ent = PRVM_NEXT_EDICT(prog->edicts);e < prog->num_edicts;e++, ent = PRVM_NEXT_EDICT(ent))
	{
		if (!ent->free && SV_PrepareEntityForSending(ent, sv.sendentities + sv.numsendentities, e))
		{
			sv.sendentitiesindex[e] = sv.sendentities + sv.numsendentities;
			sv.numsendentities++;
		}
	}
}

#define MAX_LINEOFSIGHTTRACES 64

qbool SV_CanSeeBox(int numtraces, vec_t eyejitter, vec_t enlarge, vec_t entboxexpand, vec3_t eye, vec3_t entboxmins, vec3_t entboxmaxs)
{
	prvm_prog_t *prog = SVVM_prog;
	float pitchsign;
	float alpha;
	float starttransformed[3], endtransformed[3];
	float boxminstransformed[3], boxmaxstransformed[3];
	float localboxcenter[3], localboxextents[3], localboxmins[3], localboxmaxs[3];
	int blocked = 0;
	int traceindex;
	int originalnumtouchedicts;
	int numtouchedicts = 0;
	int touchindex;
	matrix4x4_t matrix, imatrix;
	model_t *model;
	prvm_edict_t *touch;
	static prvm_edict_t *touchedicts[MAX_EDICTS];
	vec3_t eyemins, eyemaxs, start;
	vec3_t boxmins, boxmaxs;
	vec3_t clipboxmins, clipboxmaxs;
	vec3_t endpoints[MAX_LINEOFSIGHTTRACES];

	numtraces = min(numtraces, MAX_LINEOFSIGHTTRACES);

	// jitter the eye location within this box
	eyemins[0] = eye[0] - eyejitter;
	eyemaxs[0] = eye[0] + eyejitter;
	eyemins[1] = eye[1] - eyejitter;
	eyemaxs[1] = eye[1] + eyejitter;
	eyemins[2] = eye[2] - eyejitter;
	eyemaxs[2] = eye[2] + eyejitter;
	// expand the box a little
	boxmins[0] = (enlarge+1) * entboxmins[0] - enlarge * entboxmaxs[0] - entboxexpand;
	boxmaxs[0] = (enlarge+1) * entboxmaxs[0] - enlarge * entboxmins[0] + entboxexpand;
	boxmins[1] = (enlarge+1) * entboxmins[1] - enlarge * entboxmaxs[1] - entboxexpand;
	boxmaxs[1] = (enlarge+1) * entboxmaxs[1] - enlarge * entboxmins[1] + entboxexpand;
	boxmins[2] = (enlarge+1) * entboxmins[2] - enlarge * entboxmaxs[2] - entboxexpand;
	boxmaxs[2] = (enlarge+1) * entboxmaxs[2] - enlarge * entboxmins[2] + entboxexpand;

	VectorMAM(0.5f, boxmins, 0.5f, boxmaxs, endpoints[0]);
	for (traceindex = 1;traceindex < numtraces;traceindex++)
		VectorSet(endpoints[traceindex], lhrandom(boxmins[0], boxmaxs[0]), lhrandom(boxmins[1], boxmaxs[1]), lhrandom(boxmins[2], boxmaxs[2]));

	// calculate sweep box for the entire swarm of traces
	VectorCopy(eyemins, clipboxmins);
	VectorCopy(eyemaxs, clipboxmaxs);
	for (traceindex = 0;traceindex < numtraces;traceindex++)
	{
		clipboxmins[0] = min(clipboxmins[0], endpoints[traceindex][0]);
		clipboxmins[1] = min(clipboxmins[1], endpoints[traceindex][1]);
		clipboxmins[2] = min(clipboxmins[2], endpoints[traceindex][2]);
		clipboxmaxs[0] = max(clipboxmaxs[0], endpoints[traceindex][0]);
		clipboxmaxs[1] = max(clipboxmaxs[1], endpoints[traceindex][1]);
		clipboxmaxs[2] = max(clipboxmaxs[2], endpoints[traceindex][2]);
	}

	// get the list of entities in the sweep box
	if (sv_cullentities_trace_entityocclusion.integer)
		numtouchedicts = SV_EntitiesInBox(clipboxmins, clipboxmaxs, MAX_EDICTS, touchedicts);
	if (numtouchedicts > MAX_EDICTS)
	{
		// this never happens
		Con_Printf("SV_EntitiesInBox returned %i edicts, max was %i\n", numtouchedicts, MAX_EDICTS);
		numtouchedicts = MAX_EDICTS;
	}
	// iterate the entities found in the sweep box and filter them
	originalnumtouchedicts = numtouchedicts;
	numtouchedicts = 0;
	for (touchindex = 0;touchindex < originalnumtouchedicts;touchindex++)
	{
		touch = touchedicts[touchindex];
		if (PRVM_serveredictfloat(touch, solid) != SOLID_BSP)
			continue;
		model = SV_GetModelFromEdict(touch);
		if (!model || !model->brush.TraceLineOfSight)
			continue;
		// skip obviously transparent entities
		alpha = PRVM_serveredictfloat(touch, alpha);
		if (alpha && alpha < 1)
			continue;
		if ((int)PRVM_serveredictfloat(touch, effects) & EF_ADDITIVE)
			continue;
		touchedicts[numtouchedicts++] = touch;
	}

	// now that we have a filtered list of "interesting" entities, fire each
	// ray against all of them, this gives us an early-out case when something
	// is visible (which it often is)

	for (traceindex = 0;traceindex < numtraces;traceindex++)
	{
		VectorSet(start, lhrandom(eyemins[0], eyemaxs[0]), lhrandom(eyemins[1], eyemaxs[1]), lhrandom(eyemins[2], eyemaxs[2]));
		// check world occlusion
		if (sv.worldmodel && sv.worldmodel->brush.TraceLineOfSight)
			if (!sv.worldmodel->brush.TraceLineOfSight(sv.worldmodel, start, endpoints[traceindex], boxmins, boxmaxs))
				continue;
		for (touchindex = 0;touchindex < numtouchedicts;touchindex++)
		{
			touch = touchedicts[touchindex];
			model = SV_GetModelFromEdict(touch);
			if(model && model->brush.TraceLineOfSight)
			{
				// get the entity matrix
				pitchsign = SV_GetPitchSign(prog, touch);
				Matrix4x4_CreateFromQuakeEntity(&matrix, PRVM_serveredictvector(touch, origin)[0], PRVM_serveredictvector(touch, origin)[1], PRVM_serveredictvector(touch, origin)[2], pitchsign * PRVM_serveredictvector(touch, angles)[0], PRVM_serveredictvector(touch, angles)[1], PRVM_serveredictvector(touch, angles)[2], 1);
				Matrix4x4_Invert_Simple(&imatrix, &matrix);
				// see if the ray hits this entity
				Matrix4x4_Transform(&imatrix, start, starttransformed);
				Matrix4x4_Transform(&imatrix, endpoints[traceindex], endtransformed);
				Matrix4x4_Transform(&imatrix, boxmins, boxminstransformed);
				Matrix4x4_Transform(&imatrix, boxmaxs, boxmaxstransformed);
				// transform the AABB to local space
				VectorMAM(0.5f, boxminstransformed, 0.5f, boxmaxstransformed, localboxcenter);
				localboxextents[0] = fabs(boxmaxstransformed[0] - localboxcenter[0]);
				localboxextents[1] = fabs(boxmaxstransformed[1] - localboxcenter[1]);
				localboxextents[2] = fabs(boxmaxstransformed[2] - localboxcenter[2]);
				localboxmins[0] = localboxcenter[0] - localboxextents[0];
				localboxmins[1] = localboxcenter[1] - localboxextents[1];
				localboxmins[2] = localboxcenter[2] - localboxextents[2];
				localboxmaxs[0] = localboxcenter[0] + localboxextents[0];
				localboxmaxs[1] = localboxcenter[1] + localboxextents[1];
				localboxmaxs[2] = localboxcenter[2] + localboxextents[2];
				if (!model->brush.TraceLineOfSight(model, starttransformed, endtransformed, localboxmins, localboxmaxs))
				{
					blocked++;
					break;
				}
			}
		}
		// check if the ray was blocked
		if (touchindex < numtouchedicts)
			continue;
		// return if the ray was not blocked
		return true;
	}

	// no rays survived
	return false;
}

void SV_MarkWriteEntityStateToClient(entity_state_t *s, client_t *client)
{
	prvm_prog_t *prog = SVVM_prog;
	int isbmodel;
	model_t *model;
	prvm_edict_t *ed;
	if (sv.sententitiesconsideration[s->number] == sv.sententitiesmark)
		return;
	sv.sententitiesconsideration[s->number] = sv.sententitiesmark;
	sv.writeentitiestoclient_stats_totalentities++;

	if (s->customizeentityforclient)
	{
		PRVM_serverglobalfloat(time) = sv.time;
		PRVM_serverglobaledict(self) = s->number;
		PRVM_serverglobaledict(other) = sv.writeentitiestoclient_cliententitynumber;
		prog->ExecuteProgram(prog, s->customizeentityforclient, "customizeentityforclient: NULL function");
		if(!PRVM_G_FLOAT(OFS_RETURN) || !SV_PrepareEntityForSending(PRVM_EDICT_NUM(s->number), s, s->number))
			return;
	}

	// never reject player
	if (s->number != sv.writeentitiestoclient_cliententitynumber)
	{
		// check various rejection conditions
		if (s->nodrawtoclient == sv.writeentitiestoclient_cliententitynumber)
			return;
		if (s->drawonlytoclient && s->drawonlytoclient != sv.writeentitiestoclient_cliententitynumber)
			return;
		if (s->effects & EF_NODRAW)
			return;
		// LadyHavoc: only send entities with a model or important effects
		if (!s->modelindex && s->specialvisibilityradius == 0)
			return;

		isbmodel = (model = SV_GetModelByIndex(s->modelindex)) != NULL && model->name[0] == '*';
		// viewmodels don't have visibility checking
		if (s->viewmodelforclient)
		{
			if (s->viewmodelforclient != sv.writeentitiestoclient_cliententitynumber)
				return;
		}
		else if (s->tagentity)
		{
			// tag attached entities simply check their parent
			if (!sv.sendentitiesindex[s->tagentity])
				return;
			SV_MarkWriteEntityStateToClient(sv.sendentitiesindex[s->tagentity], client);
			if (sv.sententities[s->tagentity] != sv.sententitiesmark)
				return;
		}
		// always send world submodels in newer protocols because they don't
		// generate much traffic (in old protocols they hog bandwidth)
		// but only if sv_cullentities_nevercullbmodels is off
		else if (!(s->effects & EF_NODEPTHTEST) && (!isbmodel || !sv_cullentities_nevercullbmodels.integer || sv.protocol == PROTOCOL_QUAKE || sv.protocol == PROTOCOL_QUAKEDP || sv.protocol == PROTOCOL_NEHAHRAMOVIE))
		{
			// entity has survived every check so far, check if visible
			ed = PRVM_EDICT_NUM(s->number);

			// if not touching a visible leaf
			if (sv_cullentities_pvs.integer && !r_novis.integer && !r_trippy.integer && sv.writeentitiestoclient_pvs)
			{
				if (ed->priv.server->pvs_numclusters < 0)
				{
					// entity too big for clusters list
					if (sv.worldmodel && sv.worldmodel->brush.BoxTouchingPVS && !sv.worldmodel->brush.BoxTouchingPVS(sv.worldmodel, sv.writeentitiestoclient_pvs, ed->priv.server->cullmins, ed->priv.server->cullmaxs))
					{
						sv.writeentitiestoclient_stats_culled_pvs++;
						return;
					}
				}
				else
				{
					int i;
					// check cached clusters list
					for (i = 0;i < ed->priv.server->pvs_numclusters;i++)
						if (CHECKPVSBIT(sv.writeentitiestoclient_pvs, ed->priv.server->pvs_clusterlist[i]))
							break;
					if (i == ed->priv.server->pvs_numclusters)
					{
						sv.writeentitiestoclient_stats_culled_pvs++;
						return;
					}
				}
			}

			// or not seen by random tracelines
			if (sv_cullentities_trace.integer && !isbmodel && sv.worldmodel && sv.worldmodel->brush.TraceLineOfSight && !r_trippy.integer && (client->frags != -666 || sv_cullentities_trace_spectators.integer))
			{
				int samples =
					s->number <= svs.maxclients
						? sv_cullentities_trace_samples_players.integer
						:
					s->specialvisibilityradius
						? sv_cullentities_trace_samples_extra.integer
						: sv_cullentities_trace_samples.integer;

				if(samples > 0)
				{
					int eyeindex;
					for (eyeindex = 0;eyeindex < sv.writeentitiestoclient_numeyes;eyeindex++)
						if(SV_CanSeeBox(samples, sv_cullentities_trace_eyejitter.value, sv_cullentities_trace_enlarge.value, sv_cullentities_trace_expand.value, sv.writeentitiestoclient_eyes[eyeindex], ed->priv.server->cullmins, ed->priv.server->cullmaxs))
							break;
					if(eyeindex < sv.writeentitiestoclient_numeyes)
						svs.clients[sv.writeentitiestoclient_clientnumber].visibletime[s->number] =
							host.realtime + (
								s->number <= svs.maxclients
									? sv_cullentities_trace_delay_players.value
									: sv_cullentities_trace_delay.value
							);
					else if ((float)host.realtime > svs.clients[sv.writeentitiestoclient_clientnumber].visibletime[s->number])
					{
						sv.writeentitiestoclient_stats_culled_trace++;
						return;
					}
				}
			}
		}
	}

	// this just marks it for sending
	// FIXME: it would be more efficient to send here, but the entity
	// compressor isn't that flexible
	sv.writeentitiestoclient_stats_visibleentities++;
	sv.sententities[s->number] = sv.sententitiesmark;
}

#if MAX_LEVELNETWORKEYES > 0
#define MAX_EYE_RECURSION 1 // increase if recursion gets supported by portals
void SV_AddCameraEyes(void)
{
	prvm_prog_t *prog = SVVM_prog;
	int e, i, j, k;
	prvm_edict_t *ed;
	int cameras[MAX_LEVELNETWORKEYES];
	vec3_t camera_origins[MAX_LEVELNETWORKEYES];
	int eye_levels[MAX_CLIENTNETWORKEYES] = {0};
	int n_cameras = 0;
	vec3_t mi, ma;

	// check line of sight to portal entities and add them to PVS
	for (e = 1, ed = PRVM_NEXT_EDICT(prog->edicts);e < prog->num_edicts;e++, ed = PRVM_NEXT_EDICT(ed))
	{
		if (!ed->free)
		{
			if(PRVM_serveredictfunction(ed, camera_transform))
			{
				PRVM_serverglobalfloat(time) = sv.time;
				PRVM_serverglobaledict(self) = e;
				PRVM_serverglobaledict(other) = sv.writeentitiestoclient_cliententitynumber;
				VectorCopy(sv.writeentitiestoclient_eyes[0], PRVM_serverglobalvector(trace_endpos));
				VectorCopy(sv.writeentitiestoclient_eyes[0], PRVM_G_VECTOR(OFS_PARM0));
				VectorClear(PRVM_G_VECTOR(OFS_PARM1));
				prog->ExecuteProgram(prog, PRVM_serveredictfunction(ed, camera_transform), "QC function e.camera_transform is missing");
				if(!VectorCompare(PRVM_serverglobalvector(trace_endpos), sv.writeentitiestoclient_eyes[0]))
				{
					VectorCopy(PRVM_serverglobalvector(trace_endpos), camera_origins[n_cameras]);
					cameras[n_cameras] = e;
					++n_cameras;
					if(n_cameras >= MAX_LEVELNETWORKEYES)
						break;
				}
			}
		}
	}

	if(!n_cameras)
		return;

	// i is loop counter, is reset to 0 when an eye got added
	// j is camera index to check
	for(i = 0, j = 0; sv.writeentitiestoclient_numeyes < MAX_CLIENTNETWORKEYES && i < n_cameras; ++i, ++j, j %= n_cameras)
	{
		if(!cameras[j])
			continue;
		ed = PRVM_EDICT_NUM(cameras[j]);
		VectorAdd(PRVM_serveredictvector(ed, origin), PRVM_serveredictvector(ed, mins), mi);
		VectorAdd(PRVM_serveredictvector(ed, origin), PRVM_serveredictvector(ed, maxs), ma);
		for(k = 0; k < sv.writeentitiestoclient_numeyes; ++k)
		if(eye_levels[k] <= MAX_EYE_RECURSION)
		{
			if(SV_CanSeeBox(sv_cullentities_trace_samples_extra.integer, sv_cullentities_trace_eyejitter.value, sv_cullentities_trace_enlarge.value, sv_cullentities_trace_expand.value, sv.writeentitiestoclient_eyes[k], mi, ma))
				svs.clients[sv.writeentitiestoclient_clientnumber].visibletime[cameras[j]] = host.realtime + sv_cullentities_trace_delay.value;

			// bones_was_here: this use of visibletime doesn't conflict because sv_cullentities_trace doesn't consider portal entities
			// the explicit cast prevents float precision differences that cause the condition to fail
			if ((float)host.realtime <= svs.clients[sv.writeentitiestoclient_clientnumber].visibletime[cameras[j]])
			{
				eye_levels[sv.writeentitiestoclient_numeyes] = eye_levels[k] + 1;
				VectorCopy(camera_origins[j], sv.writeentitiestoclient_eyes[sv.writeentitiestoclient_numeyes]);
				// Con_Printf("added eye %d: %f %f %f because we can see %f %f %f .. %f %f %f from eye %d\n", j, sv.writeentitiestoclient_eyes[sv.writeentitiestoclient_numeyes][0], sv.writeentitiestoclient_eyes[sv.writeentitiestoclient_numeyes][1], sv.writeentitiestoclient_eyes[sv.writeentitiestoclient_numeyes][2], mi[0], mi[1], mi[2], ma[0], ma[1], ma[2], k);
				sv.writeentitiestoclient_numeyes++;
				cameras[j] = 0;
				i = 0;
				break;
			}
		}
	}
}
#else
void SV_AddCameraEyes(void)
{
}
#endif

/*
=============
SV_CleanupEnts

=============
*/
static void SV_CleanupEnts (void)
{
	prvm_prog_t *prog = SVVM_prog;
	int		e;
	prvm_edict_t	*ent;

	ent = PRVM_NEXT_EDICT(prog->edicts);
	for (e=1 ; e<prog->num_edicts ; e++, ent = PRVM_NEXT_EDICT(ent))
		PRVM_serveredictfloat(ent, effects) = (int)PRVM_serveredictfloat(ent, effects) & ~EF_MUZZLEFLASH;
}

/*
==================
SV_WriteClientdataToMessage

==================
*/
void SV_WriteClientdataToMessage (client_t *client, prvm_edict_t *ent, sizebuf_t *msg, int *stats)
{
	prvm_prog_t *prog = SVVM_prog;
	int		bits;
	int		i;
	prvm_edict_t	*other;
	int		items;
	vec3_t	punchvector;
	int		viewzoom;
	const char *s;
	float	*statsf = (float *)stats;
	float gravity;

//
// send a damage message
//
	if (PRVM_serveredictfloat(ent, dmg_take) || PRVM_serveredictfloat(ent, dmg_save))
	{
		other = PRVM_PROG_TO_EDICT(PRVM_serveredictedict(ent, dmg_inflictor));
		MSG_WriteByte (msg, svc_damage);
		MSG_WriteByte (msg, (int)PRVM_serveredictfloat(ent, dmg_save));
		MSG_WriteByte (msg, (int)PRVM_serveredictfloat(ent, dmg_take));
		for (i=0 ; i<3 ; i++)
			MSG_WriteCoord (msg, PRVM_serveredictvector(other, origin)[i] + 0.5*(PRVM_serveredictvector(other, mins)[i] + PRVM_serveredictvector(other, maxs)[i]), sv.protocol);

		PRVM_serveredictfloat(ent, dmg_take) = 0;
		PRVM_serveredictfloat(ent, dmg_save) = 0;
	}

//
// send the current viewpos offset from the view entity
//
	SV_SetIdealPitch ();		// how much to look up / down ideally

// a fixangle might get lost in a dropped packet.  Oh well.
	if(PRVM_serveredictfloat(ent, fixangle))
	{
		// angle fixing was requested by global thinking code...
		// so store the current angles for later use
		VectorCopy(PRVM_serveredictvector(ent, angles), host_client->fixangle_angles);
		host_client->fixangle_angles_set = true;

		// and clear fixangle for the next frame
		PRVM_serveredictfloat(ent, fixangle) = 0;
	}

	if (host_client->fixangle_angles_set)
	{
		MSG_WriteByte (msg, svc_setangle);
		for (i=0 ; i < 3 ; i++)
			MSG_WriteAngle (msg, host_client->fixangle_angles[i], sv.protocol);
		host_client->fixangle_angles_set = false;
	}

	// the runes are in serverflags, pack them into the items value, also pack
	// in the items2 value for mission pack huds
	// (used only in the mission packs, which do not use serverflags)
	items = (int)PRVM_serveredictfloat(ent, items)
		| (((int)PRVM_serveredictfloat(ent, items2) & ((1<<9)-1)) << 23)
		| (((int)PRVM_serverglobalfloat(serverflags) & ((1<<4)-1)) << 28);

	VectorCopy(PRVM_serveredictvector(ent, punchvector), punchvector);

	// cache weapon model name and index in client struct to save time
	// (this search can be almost 1% of cpu time!)
	s = PRVM_GetString(prog, PRVM_serveredictstring(ent, weaponmodel));
	if (strcmp(s, client->weaponmodel))
	{
		dp_strlcpy(client->weaponmodel, s, sizeof(client->weaponmodel));
		client->weaponmodelindex = SV_ModelIndex(s, 1);
	}

	viewzoom = (int)(PRVM_serveredictfloat(ent, viewzoom) * 255.0f);
	if (viewzoom == 0)
		viewzoom = 255;

	bits = 0;

	if ((int)PRVM_serveredictfloat(ent, flags) & FL_ONGROUND)
		bits |= SU_ONGROUND;
	if (PRVM_serveredictfloat(ent, waterlevel) >= 2)
		bits |= SU_INWATER;
	if (PRVM_serveredictfloat(ent, idealpitch))
		bits |= SU_IDEALPITCH;

	for (i=0 ; i<3 ; i++)
	{
		if (PRVM_serveredictvector(ent, punchangle)[i])
			bits |= (SU_PUNCH1<<i);
		if (sv.protocol != PROTOCOL_QUAKE && sv.protocol != PROTOCOL_QUAKEDP && sv.protocol != PROTOCOL_NEHAHRAMOVIE && sv.protocol != PROTOCOL_NEHAHRABJP && sv.protocol != PROTOCOL_NEHAHRABJP2 && sv.protocol != PROTOCOL_NEHAHRABJP3)
			if (punchvector[i])
				bits |= (SU_PUNCHVEC1<<i);
		if (PRVM_serveredictvector(ent, velocity)[i])
			bits |= (SU_VELOCITY1<<i);
	}

	gravity = PRVM_serveredictfloat(ent, gravity);if (!gravity) gravity = 1.0f;

	memset(stats, 0, sizeof(int[MAX_CL_STATS]));
	stats[STAT_VIEWHEIGHT] = (int)PRVM_serveredictvector(ent, view_ofs)[2];
	stats[STAT_ITEMS] = items;
	stats[STAT_WEAPONFRAME] = (int)PRVM_serveredictfloat(ent, weaponframe);
	stats[STAT_ARMOR] = (int)PRVM_serveredictfloat(ent, armorvalue);
	stats[STAT_WEAPON] = client->weaponmodelindex;
	stats[STAT_HEALTH] = (int)PRVM_serveredictfloat(ent, health);
	stats[STAT_AMMO] = (int)PRVM_serveredictfloat(ent, currentammo);
	stats[STAT_SHELLS] = (int)PRVM_serveredictfloat(ent, ammo_shells);
	stats[STAT_NAILS] = (int)PRVM_serveredictfloat(ent, ammo_nails);
	stats[STAT_ROCKETS] = (int)PRVM_serveredictfloat(ent, ammo_rockets);
	stats[STAT_CELLS] = (int)PRVM_serveredictfloat(ent, ammo_cells);
	stats[STAT_ACTIVEWEAPON] = (int)PRVM_serveredictfloat(ent, weapon);
	stats[STAT_VIEWZOOM] = viewzoom;
	stats[STAT_TOTALSECRETS] = (int)PRVM_serverglobalfloat(total_secrets);
	stats[STAT_TOTALMONSTERS] = (int)PRVM_serverglobalfloat(total_monsters);
	// the QC bumps these itself by sending svc_'s, so we have to keep them
	// zero or they'll be corrected by the engine
	//stats[STAT_SECRETS] = PRVM_serverglobalfloat(found_secrets);
	//stats[STAT_MONSTERS] = PRVM_serverglobalfloat(killed_monsters);

	if(!sv_qcstats.integer)
	{
		statsf[STAT_MOVEVARS_AIRACCEL_QW_STRETCHFACTOR] = sv_airaccel_qw_stretchfactor.value;
		statsf[STAT_MOVEVARS_AIRCONTROL_PENALTY] = sv_aircontrol_penalty.value;
		statsf[STAT_MOVEVARS_AIRSPEEDLIMIT_NONQW] = sv_airspeedlimit_nonqw.value;		
		statsf[STAT_MOVEVARS_AIRSTRAFEACCEL_QW] = sv_airstrafeaccel_qw.value;
		statsf[STAT_MOVEVARS_AIRCONTROL_POWER] = sv_aircontrol_power.value;
		// movement settings for prediction
		// note: these are not sent in protocols with lower MAX_CL_STATS limits
		stats[STAT_MOVEFLAGS] = MOVEFLAG_VALID
			| (sv_gameplayfix_q2airaccelerate.integer ? MOVEFLAG_Q2AIRACCELERATE : 0)
			| (sv_gameplayfix_nogravityonground.integer ? MOVEFLAG_NOGRAVITYONGROUND : 0)
			| (sv_gameplayfix_gravityunaffectedbyticrate.integer ? MOVEFLAG_GRAVITYUNAFFECTEDBYTICRATE : 0)
		;
		statsf[STAT_MOVEVARS_WARSOWBUNNY_AIRFORWARDACCEL] = sv_warsowbunny_airforwardaccel.value;
		statsf[STAT_MOVEVARS_WARSOWBUNNY_ACCEL] = sv_warsowbunny_accel.value;
		statsf[STAT_MOVEVARS_WARSOWBUNNY_TOPSPEED] = sv_warsowbunny_topspeed.value;
		statsf[STAT_MOVEVARS_WARSOWBUNNY_TURNACCEL] = sv_warsowbunny_turnaccel.value;
		statsf[STAT_MOVEVARS_WARSOWBUNNY_BACKTOSIDERATIO] = sv_warsowbunny_backtosideratio.value;
		statsf[STAT_MOVEVARS_AIRSTOPACCELERATE] = sv_airstopaccelerate.value;
		statsf[STAT_MOVEVARS_AIRSTRAFEACCELERATE] = sv_airstrafeaccelerate.value;
		statsf[STAT_MOVEVARS_MAXAIRSTRAFESPEED] = sv_maxairstrafespeed.value;
		statsf[STAT_MOVEVARS_AIRCONTROL] = sv_aircontrol.value;
		statsf[STAT_FRAGLIMIT] = fraglimit.value;
		statsf[STAT_TIMELIMIT] = timelimit.value;
		statsf[STAT_MOVEVARS_FRICTION] = sv_friction.value;	
		statsf[STAT_MOVEVARS_WATERFRICTION] = sv_waterfriction.value >= 0 ? sv_waterfriction.value : sv_friction.value;
		statsf[STAT_MOVEVARS_TICRATE] = sys_ticrate.value;
		statsf[STAT_MOVEVARS_TIMESCALE] = host_timescale.value;
		statsf[STAT_MOVEVARS_GRAVITY] = sv_gravity.value;
		statsf[STAT_MOVEVARS_STOPSPEED] = sv_stopspeed.value;
		statsf[STAT_MOVEVARS_MAXSPEED] = sv_maxspeed.value;
		statsf[STAT_MOVEVARS_SPECTATORMAXSPEED] = sv_maxspeed.value; // FIXME: QW has a separate cvar for this
		statsf[STAT_MOVEVARS_ACCELERATE] = sv_accelerate.value;
		statsf[STAT_MOVEVARS_AIRACCELERATE] = sv_airaccelerate.value >= 0 ? sv_airaccelerate.value : sv_accelerate.value;
		statsf[STAT_MOVEVARS_WATERACCELERATE] = sv_wateraccelerate.value >= 0 ? sv_wateraccelerate.value : sv_accelerate.value;
		statsf[STAT_MOVEVARS_ENTGRAVITY] = gravity;
		statsf[STAT_MOVEVARS_JUMPVELOCITY] = sv_jumpvelocity.value;
		statsf[STAT_MOVEVARS_EDGEFRICTION] = sv_edgefriction.value;
		statsf[STAT_MOVEVARS_MAXAIRSPEED] = sv_maxairspeed.value;
		statsf[STAT_MOVEVARS_STEPHEIGHT] = sv_stepheight.value;
		statsf[STAT_MOVEVARS_AIRACCEL_QW] = sv_airaccel_qw.value;
		statsf[STAT_MOVEVARS_AIRACCEL_SIDEWAYS_FRICTION] = sv_airaccel_sideways_friction.value;
	}

	if (sv.protocol == PROTOCOL_QUAKE || sv.protocol == PROTOCOL_QUAKEDP || sv.protocol == PROTOCOL_NEHAHRAMOVIE || sv.protocol == PROTOCOL_NEHAHRABJP || sv.protocol == PROTOCOL_NEHAHRABJP2 || sv.protocol == PROTOCOL_NEHAHRABJP3 || sv.protocol == PROTOCOL_DARKPLACES1 || sv.protocol == PROTOCOL_DARKPLACES2 || sv.protocol == PROTOCOL_DARKPLACES3 || sv.protocol == PROTOCOL_DARKPLACES4 || sv.protocol == PROTOCOL_DARKPLACES5)
	{
		if (stats[STAT_VIEWHEIGHT] != DEFAULT_VIEWHEIGHT) bits |= SU_VIEWHEIGHT;
		bits |= SU_ITEMS;
		if (stats[STAT_WEAPONFRAME]) bits |= SU_WEAPONFRAME;
		if (stats[STAT_ARMOR]) bits |= SU_ARMOR;
		bits |= SU_WEAPON;
		// FIXME: which protocols support this?  does PROTOCOL_DARKPLACES3 support viewzoom?
		if (sv.protocol == PROTOCOL_DARKPLACES2 || sv.protocol == PROTOCOL_DARKPLACES3 || sv.protocol == PROTOCOL_DARKPLACES4 || sv.protocol == PROTOCOL_DARKPLACES5)
			if (viewzoom != 255)
				bits |= SU_VIEWZOOM;
	}

	if (bits >= 65536)
		bits |= SU_EXTEND1;
	if (bits >= 16777216)
		bits |= SU_EXTEND2;

	// send the data
	MSG_WriteByte (msg, svc_clientdata);
	MSG_WriteShort (msg, bits);
	if (bits & SU_EXTEND1)
		MSG_WriteByte(msg, bits >> 16);
	if (bits & SU_EXTEND2)
		MSG_WriteByte(msg, bits >> 24);

	if (bits & SU_VIEWHEIGHT)
		MSG_WriteChar (msg, stats[STAT_VIEWHEIGHT]);

	if (bits & SU_IDEALPITCH)
		MSG_WriteChar (msg, (int)PRVM_serveredictfloat(ent, idealpitch));

	for (i=0 ; i<3 ; i++)
	{
		if (bits & (SU_PUNCH1<<i))
		{
			if (sv.protocol == PROTOCOL_QUAKE || sv.protocol == PROTOCOL_QUAKEDP || sv.protocol == PROTOCOL_NEHAHRAMOVIE || sv.protocol == PROTOCOL_NEHAHRABJP || sv.protocol == PROTOCOL_NEHAHRABJP2 || sv.protocol == PROTOCOL_NEHAHRABJP3)
				MSG_WriteChar(msg, (int)PRVM_serveredictvector(ent, punchangle)[i]);
			else
				MSG_WriteAngle16i(msg, PRVM_serveredictvector(ent, punchangle)[i]);
		}
		if (bits & (SU_PUNCHVEC1<<i))
		{
			if (sv.protocol == PROTOCOL_DARKPLACES1 || sv.protocol == PROTOCOL_DARKPLACES2 || sv.protocol == PROTOCOL_DARKPLACES3 || sv.protocol == PROTOCOL_DARKPLACES4)
				MSG_WriteCoord16i(msg, punchvector[i]);
			else
				MSG_WriteCoord32f(msg, punchvector[i]);
		}
		if (bits & (SU_VELOCITY1<<i))
		{
			if (sv.protocol == PROTOCOL_QUAKE || sv.protocol == PROTOCOL_QUAKEDP || sv.protocol == PROTOCOL_NEHAHRAMOVIE || sv.protocol == PROTOCOL_NEHAHRABJP || sv.protocol == PROTOCOL_NEHAHRABJP2 || sv.protocol == PROTOCOL_NEHAHRABJP3 || sv.protocol == PROTOCOL_DARKPLACES1 || sv.protocol == PROTOCOL_DARKPLACES2 || sv.protocol == PROTOCOL_DARKPLACES3 || sv.protocol == PROTOCOL_DARKPLACES4)
				MSG_WriteChar(msg, (int)(PRVM_serveredictvector(ent, velocity)[i] * (1.0f / 16.0f)));
			else
				MSG_WriteCoord32f(msg, PRVM_serveredictvector(ent, velocity)[i]);
		}
	}

	if (bits & SU_ITEMS)
		MSG_WriteLong (msg, stats[STAT_ITEMS]);

	if (sv.protocol == PROTOCOL_DARKPLACES5)
	{
		if (bits & SU_WEAPONFRAME)
			MSG_WriteShort (msg, stats[STAT_WEAPONFRAME]);
		if (bits & SU_ARMOR)
			MSG_WriteShort (msg, stats[STAT_ARMOR]);
		if (bits & SU_WEAPON)
			MSG_WriteShort (msg, stats[STAT_WEAPON]);
		MSG_WriteShort (msg, stats[STAT_HEALTH]);
		MSG_WriteShort (msg, stats[STAT_AMMO]);
		MSG_WriteShort (msg, stats[STAT_SHELLS]);
		MSG_WriteShort (msg, stats[STAT_NAILS]);
		MSG_WriteShort (msg, stats[STAT_ROCKETS]);
		MSG_WriteShort (msg, stats[STAT_CELLS]);
		MSG_WriteShort (msg, stats[STAT_ACTIVEWEAPON]);
		if (bits & SU_VIEWZOOM)
			MSG_WriteShort (msg, bound(0, stats[STAT_VIEWZOOM], 65535));
	}
	else if (sv.protocol == PROTOCOL_QUAKE || sv.protocol == PROTOCOL_QUAKEDP || sv.protocol == PROTOCOL_NEHAHRAMOVIE || sv.protocol == PROTOCOL_NEHAHRABJP || sv.protocol == PROTOCOL_NEHAHRABJP2 || sv.protocol == PROTOCOL_NEHAHRABJP3 || sv.protocol == PROTOCOL_DARKPLACES1 || sv.protocol == PROTOCOL_DARKPLACES2 || sv.protocol == PROTOCOL_DARKPLACES3 || sv.protocol == PROTOCOL_DARKPLACES4)
	{
		if (bits & SU_WEAPONFRAME)
			MSG_WriteByte (msg, stats[STAT_WEAPONFRAME]);
		if (bits & SU_ARMOR)
			MSG_WriteByte (msg, stats[STAT_ARMOR]);
		if (bits & SU_WEAPON)
		{
			if (sv.protocol == PROTOCOL_NEHAHRABJP || sv.protocol == PROTOCOL_NEHAHRABJP2 || sv.protocol == PROTOCOL_NEHAHRABJP3)
				MSG_WriteShort (msg, stats[STAT_WEAPON]);
			else
				MSG_WriteByte (msg, stats[STAT_WEAPON]);
		}
		MSG_WriteShort (msg, stats[STAT_HEALTH]);
		MSG_WriteByte (msg, stats[STAT_AMMO]);
		MSG_WriteByte (msg, stats[STAT_SHELLS]);
		MSG_WriteByte (msg, stats[STAT_NAILS]);
		MSG_WriteByte (msg, stats[STAT_ROCKETS]);
		MSG_WriteByte (msg, stats[STAT_CELLS]);
		if (gamemode == GAME_HIPNOTIC || gamemode == GAME_ROGUE || gamemode == GAME_QUOTH || IS_OLDNEXUIZ_DERIVED(gamemode))
		{
			for (i = 0;i < 32;i++)
				if (stats[STAT_ACTIVEWEAPON] & (1<<i))
					break;
			MSG_WriteByte (msg, i);
		}
		else
			MSG_WriteByte (msg, stats[STAT_ACTIVEWEAPON]);
		if (bits & SU_VIEWZOOM)
		{
			if (sv.protocol == PROTOCOL_DARKPLACES2 || sv.protocol == PROTOCOL_DARKPLACES3 || sv.protocol == PROTOCOL_DARKPLACES4)
				MSG_WriteByte (msg, bound(0, stats[STAT_VIEWZOOM], 255));
			else
				MSG_WriteShort (msg, bound(0, stats[STAT_VIEWZOOM], 65535));
		}
	}
}

void SV_FlushBroadcastMessages(void)
{
	int i;
	client_t *client;
	if (sv.datagram.cursize <= 0)
		return;
	for (i = 0, client = svs.clients;i < svs.maxclients;i++, client++)
	{
		if (!client->begun || !client->netconnection || client->unreliablemsg.cursize + sv.datagram.cursize > client->unreliablemsg.maxsize || client->unreliablemsg_splitpoints >= (int)(sizeof(client->unreliablemsg_splitpoint)/sizeof(client->unreliablemsg_splitpoint[0])))
			continue;
		SZ_Write(&client->unreliablemsg, sv.datagram.data, sv.datagram.cursize);
		client->unreliablemsg_splitpoint[client->unreliablemsg_splitpoints++] = client->unreliablemsg.cursize;
	}
	SZ_Clear(&sv.datagram);
}

static void SV_WriteUnreliableMessages(client_t *client, sizebuf_t *msg, int maxsize, int maxsize2)
{
	// scan the splitpoints to find out how many we can fit in
	int numsegments, j, split;
	if (!client->unreliablemsg_splitpoints)
		return;
	// always accept the first one if it's within 1024 bytes, this ensures
	// that very big datagrams which are over the rate limit still get
	// through, just to keep it working
	for (numsegments = 1;numsegments < client->unreliablemsg_splitpoints;numsegments++)
		if (msg->cursize + client->unreliablemsg_splitpoint[numsegments] > maxsize)
			break;
	// the first segment gets an exemption from the rate limiting, otherwise
	// it could get dropped consistently due to a low rate limit
	if (numsegments == 1)
		maxsize = maxsize2;
	// some will fit, so add the ones that will fit
	split = client->unreliablemsg_splitpoint[numsegments-1];
	// note this discards ones that were accepted by the segments scan but
	// can not fit, such as a really huge first one that will never ever
	// fit in a packet...
	if (msg->cursize + split <= maxsize)
		SZ_Write(msg, client->unreliablemsg.data, split);
	// remove the part we sent, keeping any remaining data
	client->unreliablemsg.cursize -= split;
	if (client->unreliablemsg.cursize > 0)
		memmove(client->unreliablemsg.data, client->unreliablemsg.data + split, client->unreliablemsg.cursize);
	// adjust remaining splitpoints
	client->unreliablemsg_splitpoints -= numsegments;
	for (j = 0;j < client->unreliablemsg_splitpoints;j++)
		client->unreliablemsg_splitpoint[j] = client->unreliablemsg_splitpoint[numsegments + j] - split;
}

/*
=======================
SV_SendClientDatagram
=======================
*/
static void SV_SendClientDatagram (client_t *client)
{
	int clientrate, maxrate, maxsize, maxsize2, downloadsize;
	sizebuf_t msg;
	int stats[MAX_CL_STATS];
	static unsigned char sv_sendclientdatagram_buf[NET_MAXMESSAGE];
	double timedelta;

	// obey rate limit by limiting packet frequency if the packet size
	// limiting fails
	// (usually this is caused by reliable messages)
	if (!NetConn_CanSend(client->netconnection))
		return;

	// PROTOCOL_DARKPLACES5 and later support packet size limiting of updates
	maxrate = max(NET_MINRATE, sv_maxrate.integer);
	if (sv_maxrate.integer != maxrate)
		Cvar_SetValueQuick(&sv_maxrate, maxrate);

	// clientrate determines the 'cleartime' of a packet
	// (how long to wait before sending another, based on this packet's size)
	clientrate = bound(NET_MINRATE, client->rate, maxrate);

	switch (sv.protocol)
	{
	case PROTOCOL_QUAKE:
	case PROTOCOL_QUAKEDP:
	case PROTOCOL_NEHAHRAMOVIE:
	case PROTOCOL_NEHAHRABJP:
	case PROTOCOL_NEHAHRABJP2:
	case PROTOCOL_NEHAHRABJP3:
	case PROTOCOL_QUAKEWORLD:
		// no packet size limit support on Quake protocols because it just
		// causes missing entities/effects
		// packets are simply sent less often to obey the rate limit
		maxsize = 1024;
		maxsize2 = 1024;
		break;
	case PROTOCOL_DARKPLACES1:
	case PROTOCOL_DARKPLACES2:
	case PROTOCOL_DARKPLACES3:
	case PROTOCOL_DARKPLACES4:
		// no packet size limit support on DP1-4 protocols because they kick
		// the client off if they overflow, and miss effects
		// packets are simply sent less often to obey the rate limit
		maxsize = sizeof(sv_sendclientdatagram_buf);
		maxsize2 = sizeof(sv_sendclientdatagram_buf);
		break;
	default:
		// DP5 and later protocols support packet size limiting which is a
		// better method than limiting packet frequency as QW does
		//
		// at very low rates (or very small sys_ticrate) the packet size is
		// not reduced below 128, but packets may be sent less often

		// how long are bursts?
		timedelta = host_client->rate_burstsize / (double)client->rate;

		// how much of the burst do we keep reserved?
		timedelta *= 1 - net_burstreserve.value;

		// only try to use excess time
		timedelta = bound(0, host.realtime - host_client->netconnection->cleartime, timedelta);

		// but we know next packet will be in sys_ticrate, so we can use up THAT bandwidth
		timedelta += sys_ticrate.value;

		// note: packet overhead (not counted in maxsize) is 28 bytes
		maxsize = (int)(clientrate * timedelta) - 28;

		// put it in sound bounds
		maxsize = bound(128, maxsize, 1400);
		maxsize2 = 1400;

		// csqc entities can easily exceed 128 bytes, so disable throttling in
		// mods that use csqc (they are likely to use less bandwidth anyway)
		if((net_usesizelimit.integer == 1) ? (sv.csqc_progsize > 0) : (net_usesizelimit.integer < 1))
			maxsize = maxsize2;

		break;
	}

	if (LHNETADDRESS_GetAddressType(&host_client->netconnection->peeraddress) == LHNETADDRESSTYPE_LOOP && !host_limitlocal.integer)
	{
		// for good singleplayer, send huge packets
		maxsize = sizeof(sv_sendclientdatagram_buf);
		maxsize2 = sizeof(sv_sendclientdatagram_buf);
		// never limit frequency in singleplayer
		clientrate = 1000000000;
	}

	// while downloading, limit entity updates to half the packet
	// (any leftover space will be used for downloading)
	if (host_client->download_file)
		maxsize /= 2;

	msg.data = sv_sendclientdatagram_buf;
	msg.maxsize = sizeof(sv_sendclientdatagram_buf);
	msg.cursize = 0;
	msg.allowoverflow = false;

	if (host_client->begun)
	{
		// the player is in the game
		MSG_WriteByte (&msg, svc_time);
		MSG_WriteFloat (&msg, sv.time);

		// add the client specific data to the datagram
		SV_WriteClientdataToMessage (client, client->edict, &msg, stats);
		// now update the stats[] array using any registered custom fields
		VM_SV_UpdateCustomStats(client, client->edict, &msg, stats);
		// set host_client->statsdeltabits
		Protocol_UpdateClientStats (stats);

		// add as many queued unreliable messages (effects) as we can fit
		// limit effects to half of the remaining space
		if (client->unreliablemsg.cursize)
			SV_WriteUnreliableMessages (client, &msg, maxsize/2, maxsize2);

		// now write as many entities as we can fit, and also sends stats
		SV_WriteEntitiesToClient (client, client->edict, &msg, maxsize);
	}
	else if (host.realtime > client->keepalivetime)
	{
		// the player isn't totally in the game yet
		// send small keepalive messages if too much time has passed
		// (may also be sending downloads)
		client->keepalivetime = host.realtime + 5;
		MSG_WriteChar (&msg, svc_nop);
	}

	// if a download is active, see if there is room to fit some download data
	// in this packet
	downloadsize = min(maxsize*2,maxsize2) - msg.cursize - 7;
	if (host_client->download_file && host_client->download_started && downloadsize > 0)
	{
		fs_offset_t downloadstart;
		unsigned char data[1400];
		downloadstart = FS_Tell(host_client->download_file);
		downloadsize = min(downloadsize, (int)sizeof(data));
		downloadsize = FS_Read(host_client->download_file, data, downloadsize);
		// note this sends empty messages if at the end of the file, which is
		// necessary to keep the packet loss logic working
		// (the last blocks may be lost and need to be re-sent, and that will
		//  only occur if the client acks the empty end messages, revealing
		//  a gap in the download progress, causing the last blocks to be
		//  sent again)
		MSG_WriteChar (&msg, svc_downloaddata);
		MSG_WriteLong (&msg, downloadstart);
		MSG_WriteShort (&msg, downloadsize);
		if (downloadsize > 0)
			SZ_Write (&msg, data, downloadsize);
	}

	// reliable only if none is in progress
	if(client->sendsignon != 2 && !client->netconnection->sendMessageLength)
		SV_WriteDemoMessage(client, &(client->netconnection->message), false);
	// unreliable
	SV_WriteDemoMessage(client, &msg, false);

// send the datagram
	NetConn_SendUnreliableMessage (client->netconnection, &msg, sv.protocol, clientrate, client->rate_burstsize, client->sendsignon == 2);
	if (client->sendsignon == 1 && !client->netconnection->message.cursize)
		client->sendsignon = 2; // prevent reliable until client sends prespawn (this is the keepalive phase)
}

/*
=======================
SV_UpdateToReliableMessages
=======================
*/
static void SV_UpdateToReliableMessages (void)
{
	prvm_prog_t *prog = SVVM_prog;
	int i, j;
	client_t *client;
	const char *name;
	const char *model;
	const char *skin;
	int clientcamera;

// check for changes to be sent over the reliable streams
	for (i = 0, host_client = svs.clients;i < svs.maxclients;i++, host_client++)
	{
		// update the host_client fields we care about according to the entity fields
		host_client->edict = PRVM_EDICT_NUM(i+1);

		// DP_SV_CLIENTNAME
		name = PRVM_GetString(prog, PRVM_serveredictstring(host_client->edict, netname));
		if (name == NULL)
			name = "";
		// always point the string back at host_client->name to keep it safe
		//strlcpy (host_client->name, name, sizeof (host_client->name));
		if (name != host_client->name) // prevent buffer overlap SIGABRT on Mac OSX
			dp_strlcpy (host_client->name, name, sizeof (host_client->name));
		SV_Name(i);

		// DP_SV_CLIENTCOLORS
		host_client->colors = (int)PRVM_serveredictfloat(host_client->edict, clientcolors);
		if (host_client->old_colors != host_client->colors)
		{
			host_client->old_colors = host_client->colors;
			// send notification to all clients
			MSG_WriteByte (&sv.reliable_datagram, svc_updatecolors);
			MSG_WriteByte (&sv.reliable_datagram, i);
			MSG_WriteByte (&sv.reliable_datagram, host_client->colors);
		}

		// NEXUIZ_PLAYERMODEL
		model = PRVM_GetString(prog, PRVM_serveredictstring(host_client->edict, playermodel));
		if (model == NULL)
			model = "";
		// always point the string back at host_client->name to keep it safe
		//strlcpy (host_client->playermodel, model, sizeof (host_client->playermodel));
		if (model != host_client->playermodel) // prevent buffer overlap SIGABRT on Mac OSX
			dp_strlcpy (host_client->playermodel, model, sizeof (host_client->playermodel));
		PRVM_serveredictstring(host_client->edict, playermodel) = PRVM_SetEngineString(prog, host_client->playermodel);

		// NEXUIZ_PLAYERSKIN
		skin = PRVM_GetString(prog, PRVM_serveredictstring(host_client->edict, playerskin));
		if (skin == NULL)
			skin = "";
		// always point the string back at host_client->name to keep it safe
		//strlcpy (host_client->playerskin, skin, sizeof (host_client->playerskin));
		if (skin != host_client->playerskin) // prevent buffer overlap SIGABRT on Mac OSX
			dp_strlcpy (host_client->playerskin, skin, sizeof (host_client->playerskin));
		PRVM_serveredictstring(host_client->edict, playerskin) = PRVM_SetEngineString(prog, host_client->playerskin);

		// TODO: add an extension name for this [1/17/2008 Black]
		clientcamera = PRVM_serveredictedict(host_client->edict, clientcamera);
		if (clientcamera > 0)
		{
			int oldclientcamera = host_client->clientcamera;
			if (clientcamera >= prog->max_edicts || PRVM_EDICT_NUM(clientcamera)->free)
				clientcamera = PRVM_NUM_FOR_EDICT(host_client->edict);
			host_client->clientcamera = clientcamera;

			if (oldclientcamera != host_client->clientcamera && host_client->netconnection)
			{
				MSG_WriteByte(&host_client->netconnection->message, svc_setview);
				MSG_WriteShort(&host_client->netconnection->message, host_client->clientcamera);
			}
		}

		// frags
		host_client->frags = (int)PRVM_serveredictfloat(host_client->edict, frags);
		if(IS_OLDNEXUIZ_DERIVED(gamemode))
			if(!host_client->begun && host_client->netconnection)
				host_client->frags = -666;
		if (host_client->old_frags != host_client->frags)
		{
			host_client->old_frags = host_client->frags;
			// send notification to all clients
			MSG_WriteByte (&sv.reliable_datagram, svc_updatefrags);
			MSG_WriteByte (&sv.reliable_datagram, i);
			MSG_WriteShort (&sv.reliable_datagram, host_client->frags);
		}
	}

	for (j = 0, client = svs.clients;j < svs.maxclients;j++, client++)
		if (client->netconnection && (client->begun || client->clientconnectcalled)) // also send MSG_ALL to people who are past ClientConnect, but not spawned yet
			SZ_Write (&client->netconnection->message, sv.reliable_datagram.data, sv.reliable_datagram.cursize);

	SZ_Clear (&sv.reliable_datagram);
}


/*
=======================
SV_SendClientMessages
=======================
*/
void SV_SendClientMessages(void)
{
	int i, prepared = false;

	if (sv.protocol == PROTOCOL_QUAKEWORLD)
		Sys_Error("SV_SendClientMessages: no quakeworld support\n");

	SV_FlushBroadcastMessages();

// update frags, names, etc
	SV_UpdateToReliableMessages();

// build individual updates
	for (i = 0, host_client = svs.clients;i < svs.maxclients;i++, host_client++)
	{
		if (!host_client->active)
			continue;
		if (!host_client->netconnection)
			continue;

		if (host_client->netconnection->message.overflowed)
		{
			SV_DropClient (true, "Buffer overflow in net message");	// if the message couldn't send, kick off
			continue;
		}

		if (!prepared)
		{
			prepared = true;
			// only prepare entities once per frame
			SV_PrepareEntitiesForSending();
		}
		SV_SendClientDatagram(host_client);
	}

// clear muzzle flashes
	SV_CleanupEnts();
}
