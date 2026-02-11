#include "quakedef.h"

#ifndef CLIENTONLY

#define Q2EDICT_NUM(i) (q2edict_t*)((char *)ge->edicts+i*ge->edict_size)

#ifndef Q2SERVER
void SV_WriteFrameToClient (client_t *client, sizebuf_t *msg)
{
}
void SV_BuildClientFrame (client_t *client)
{
}
#else

q2entity_state_t *svs_client_entities;//[Q2UPDATE_BACKUP*MAX_PACKET_ENTITIES];
int svs_num_client_entities;
int svs_next_client_entities;

q2entity_state_t	sv_baselines[Q2MAX_EDICTS];

/*
=============================================================================

Encode a client frame onto the network channel

=============================================================================
*/

static void MSG_WriteSizeQ2E(sizebuf_t *sb, int solid)
{	//urgh...
	if (solid != ES_SOLID_BSP && solid != ES_SOLID_NOT)
		solid = ((solid & 255)<<0)	//recode fte's sizes to q2ex's...
		      | ((solid & 255)<<8)
		      | (((solid>>8) & 255)<<16)
		      | ((((solid>>16) & 65535) - 32768+32)<<24);
	MSG_WriteLong(sb, solid);
}
/*
==================
MSG_WriteDeltaEntity

Writes part of a packetentities message.
Can delta from either a baseline or a previous packet_entity
==================
*/
void MSGQ2_WriteDeltaEntity (q2entity_state_t *from, q2entity_state_t *to, sizebuf_t *msg, qboolean force, qboolean newentity, qboolean q2ex)
{
	quint64_t		bits;

	if (!to->number)
		Sys_Error ("Unset entity number");
	if (to->number >= Q2MAX_EDICTS)
		Sys_Error ("Entity number >= MAX_EDICTS");

// send an update
	bits = 0;

	if (to->number >= 256)
		bits |= Q2U_NUMBER16;		// number8 is implicit otherwise

	if (to->origin[0] != from->origin[0])
		bits |= Q2U_ORIGIN1;
	if (to->origin[1] != from->origin[1])
		bits |= Q2U_ORIGIN2;
	if (to->origin[2] != from->origin[2])
		bits |= Q2U_ORIGIN3;

	if ( to->angles[0] != from->angles[0] )
		bits |= Q2U_ANGLE1;		
	if ( to->angles[1] != from->angles[1] )
		bits |= Q2U_ANGLE2;
	if ( to->angles[2] != from->angles[2] )
		bits |= Q2U_ANGLE3;
		
	if ( to->skinnum != from->skinnum )
	{
		if ((unsigned)to->skinnum < 256)
			bits |= Q2U_SKIN8;
		else if ((unsigned)to->skinnum < 0x10000)
			bits |= Q2U_SKIN16;
		else
			bits |= (Q2U_SKIN8|Q2U_SKIN16);
	}
		
	if ( to->frame != from->frame )
	{
		if (to->frame < 256)
			bits |= Q2U_FRAME8;
		else
			bits |= Q2U_FRAME16;
	}

	if ( to->effects != from->effects )
	{
		if ((uint64_t)to->effects > 0xffffffffu)
		{	//this encoding is weird, Q2UEX_EFFECTS64 without any of the other flags is equivelent to the 8|16==32bit flags, so is pointless without the extra ones.
			if (to->effects < (uint64_t)256<<32)
				bits |= Q2U_EFFECTS8|Q2UEX_EFFECTS64;
			else if (to->effects < (uint64_t)0x8000<<32)
				bits |= Q2U_EFFECTS16|Q2UEX_EFFECTS64;
			else
				bits |= Q2U_EFFECTS8|Q2U_EFFECTS16|Q2UEX_EFFECTS64;
		}
		else if (to->effects < 256)
			bits |= Q2U_EFFECTS8;
		else if (to->effects < 0x8000)
			bits |= Q2U_EFFECTS16;
		else
			bits |= Q2U_EFFECTS8|Q2U_EFFECTS16;
	}
	
	if ( to->renderfx != from->renderfx )
	{
		if (to->renderfx < 256)
			bits |= Q2U_RENDERFX8;
		else if (to->renderfx < 0x8000)
			bits |= Q2U_RENDERFX16;
		else
			bits |= Q2U_RENDERFX8|Q2U_RENDERFX16;
	}
	
	if ( to->solid != from->solid )
		bits |= Q2U_SOLID;

	// event is not delta compressed, just 0 compressed
	if ( to->event  )
		bits |= Q2U_EVENT;
	
	if ( to->modelindex != from->modelindex )
	{
		bits |= Q2U_MODEL;
		if (to->modelindex > 0xff)
			bits |= Q2UX_INDEX16;
	}
	if ( to->modelindex2 != from->modelindex2 )
	{
		bits |= Q2U_MODEL2;
		if (to->modelindex2 > 0xff)
			bits |= Q2UX_INDEX16;
	}
	if ( to->modelindex3 != from->modelindex3 )
	{
		bits |= Q2U_MODEL3;
		if (to->modelindex3 > 0xff)
			bits |= Q2UX_INDEX16;
	}
	if ( to->modelindex4 != from->modelindex4 )
	{
		bits |= Q2U_MODEL4;
		if (to->modelindex4 > 0xff)
			bits |= Q2UX_INDEX16;
	}

	if ( to->sound != from->sound /*||vol  or attn*/)
	{
		bits |= Q2U_SOUND;
		if (to->sound > 0xff && !q2ex)
			bits |= Q2UX_INDEX16;
	}


//	if (to->alpha != from->alpha) bits |= Q2UEX_ALPHA;
//	if (to->scale != from->scale)bits |= Q2UEX_SCALE;
//	if (to->instance != from->instance) bits |= Q2UEX_INSTANCE;
//	if (to->owner != from->owner) bits |= Q2UEX_OWNER;
//	if (to->oldframe != from->oldframe) bits |= Q2UEX_OLDFRAME;

	if (newentity || (to->renderfx & Q2RF_BEAM))
		bits |= Q2U_OLDORIGIN;

	//
	// write the message
	//
	if (!bits && !force)
		return;		// nothing to send!

	//----------

	if (bits & 0xff00000000)
		bits |= Q2UEX_MOREBITS4 | Q2U_MOREBITS3 | Q2U_MOREBITS2 | Q2U_MOREBITS1;
	else if (bits & 0xff000000)
		bits |= Q2U_MOREBITS3 | Q2U_MOREBITS2 | Q2U_MOREBITS1;
	else if (bits & 0x00ff0000)
		bits |= Q2U_MOREBITS2 | Q2U_MOREBITS1;
	else if (bits & 0x0000ff00)
		bits |= Q2U_MOREBITS1;

	MSG_WriteByte (msg,	bits&255 );

	if (bits & 0xff00000000)
	{
		MSG_WriteByte (msg,	(bits>>8)&255 );
		MSG_WriteByte (msg,	(bits>>16)&255 );
		MSG_WriteByte (msg,	(bits>>24)&255 );
		MSG_WriteByte (msg,	(bits>>32)&255 );
	}
	else if (bits & 0xff000000)
	{
		MSG_WriteByte (msg,	(bits>>8)&255 );
		MSG_WriteByte (msg,	(bits>>16)&255 );
		MSG_WriteByte (msg,	(bits>>24)&255 );
	}
	else if (bits & 0x00ff0000)
	{
		MSG_WriteByte (msg,	(bits>>8)&255 );
		MSG_WriteByte (msg,	(bits>>16)&255 );
	}
	else if (bits & 0x0000ff00)
	{
		MSG_WriteByte (msg,	(bits>>8)&255 );
	}

	//----------

	if (bits & Q2U_NUMBER16)
		MSG_WriteShort (msg, to->number);
	else
		MSG_WriteByte (msg,	to->number);

	if (bits & Q2U_MODEL)
	{
		if (bits & Q2UX_INDEX16)
			MSG_WriteShort	(msg,	to->modelindex);
		else
			MSG_WriteByte	(msg,	to->modelindex);
	}
	if (bits & Q2U_MODEL2)
	{
		if (bits & Q2UX_INDEX16)
			MSG_WriteShort	(msg,	to->modelindex2);
		else
			MSG_WriteByte	(msg,	to->modelindex2);
	}
	if (bits & Q2U_MODEL3)
	{
		if (bits & Q2UX_INDEX16)
			MSG_WriteShort	(msg,	to->modelindex3);
		else
			MSG_WriteByte	(msg,	to->modelindex3);
	}
	if (bits & Q2U_MODEL4)
	{
		if (bits & Q2UX_INDEX16)
			MSG_WriteShort	(msg,	to->modelindex4);
		else
			MSG_WriteByte	(msg,	to->modelindex4);
	}

	if (bits & Q2U_FRAME8)
		MSG_WriteByte (msg, to->frame);
	if (bits & Q2U_FRAME16)
		MSG_WriteShort (msg, to->frame);

	if ((bits & Q2U_SKIN8) && (bits & Q2U_SKIN16))		//used for laser colors
		MSG_WriteLong (msg, to->skinnum);
	else if (bits & Q2U_SKIN8)
		MSG_WriteByte (msg, to->skinnum);
	else if (bits & Q2U_SKIN16)
		MSG_WriteShort (msg, to->skinnum);

	if (bits & Q2UEX_EFFECTS64)
	{
		MSG_WriteLong (msg, to->effects);
		if ( (bits & (Q2U_EFFECTS8|Q2U_EFFECTS16)) == (Q2U_EFFECTS8|Q2U_EFFECTS16) )
			MSG_WriteLong (msg, (quint64_t)to->effects>>32);
		else if (bits & Q2U_EFFECTS8)
			MSG_WriteByte (msg, (quint64_t)to->effects>>32);
		else if (bits & Q2U_EFFECTS16)
			MSG_WriteShort (msg, (quint64_t)to->effects>>32);
	}
	else
	{
		if ( (bits & (Q2U_EFFECTS8|Q2U_EFFECTS16)) == (Q2U_EFFECTS8|Q2U_EFFECTS16) )
			MSG_WriteLong (msg, to->effects);
		else if (bits & Q2U_EFFECTS8)
			MSG_WriteByte (msg, to->effects);
		else if (bits & Q2U_EFFECTS16)
			MSG_WriteShort (msg, to->effects);
	}

	if ( (bits & (Q2U_RENDERFX8|Q2U_RENDERFX16)) == (Q2U_RENDERFX8|Q2U_RENDERFX16) )
		MSG_WriteLong (msg, to->renderfx);
	else if (bits & Q2U_RENDERFX8)
		MSG_WriteByte (msg, to->renderfx);
	else if (bits & Q2U_RENDERFX16)
		MSG_WriteShort (msg, to->renderfx);

	if (q2ex)
	{
		if (bits & Q2U_SOLID)
			MSG_WriteSizeQ2E(msg, to->solid);

		if (!to->solid)
		{	//demos reportedly compress these... not that it makes a difference.
			if (bits & Q2U_ORIGIN1)
				MSG_WriteCoord (msg, to->origin[0]);
			if (bits & Q2U_ORIGIN2)
				MSG_WriteCoord (msg, to->origin[1]);
			if (bits & Q2U_ORIGIN3)
				MSG_WriteCoord (msg, to->origin[2]);

			if (bits & Q2U_OLDORIGIN)
			{
				MSG_WriteCoord (msg, to->old_origin[0]);
				MSG_WriteCoord (msg, to->old_origin[1]);
				MSG_WriteCoord (msg, to->old_origin[2]);
			}
		}
		else
		{
			if (bits & Q2U_ORIGIN1)
				MSG_WriteFloat (msg, to->origin[0]);
			if (bits & Q2U_ORIGIN2)
				MSG_WriteFloat (msg, to->origin[1]);
			if (bits & Q2U_ORIGIN3)
				MSG_WriteFloat (msg, to->origin[2]);

			if (bits & Q2U_OLDORIGIN)
			{
				MSG_WriteFloat (msg, to->old_origin[0]);
				MSG_WriteFloat (msg, to->old_origin[1]);
				MSG_WriteFloat (msg, to->old_origin[2]);
			}
		}

		if (bits & Q2U_ANGLE1)
			MSG_WriteFloat(msg, to->angles[0]);	//blatent overkill.
		if (bits & Q2U_ANGLE2)
			MSG_WriteFloat(msg, to->angles[1]);
		if (bits & Q2U_ANGLE3)
			MSG_WriteFloat(msg, to->angles[2]);
	}
	else
	{
		if (bits & Q2U_ORIGIN1)
			MSG_WriteCoord (msg, to->origin[0]);
		if (bits & Q2U_ORIGIN2)
			MSG_WriteCoord (msg, to->origin[1]);
		if (bits & Q2U_ORIGIN3)
			MSG_WriteCoord (msg, to->origin[2]);

		if (bits & Q2U_ANGLE1)
			MSG_WriteAngle(msg, to->angles[0]);
		if (bits & Q2U_ANGLE2)
			MSG_WriteAngle(msg, to->angles[1]);
		if (bits & Q2U_ANGLE3)
			MSG_WriteAngle(msg, to->angles[2]);

		if (bits & Q2U_OLDORIGIN)
		{
			MSG_WriteCoord (msg, to->old_origin[0]);
			MSG_WriteCoord (msg, to->old_origin[1]);
			MSG_WriteCoord (msg, to->old_origin[2]);
		}
	}

	if (bits & Q2U_SOUND)
	{
		if (q2ex)
		{
#if 1
			MSG_WriteShort	(msg,	to->sound&0x3fff);
#else
			MSG_WriteShort	(msg,	to->sound&0x3fff | ((to->soundvol!=1)?0x4000:0))  | ((to->soundattn!=3)?0x8000:0));
			MSG_WriteByte	(msg,	to->soundvol*255);
			MSG_WriteByte	(msg,	to->soundattn);	//this is normally a /64, but oh well...
#endif
		}
		else
		{
			if (bits & Q2UX_INDEX16)
				MSG_WriteShort	(msg,	to->sound);
			else
				MSG_WriteByte	(msg,	to->sound);
		}
	}

	if (bits & Q2U_EVENT)
		MSG_WriteByte (msg, to->event);

	if (!q2ex)
		if (bits & Q2U_SOLID)
		{
			if (msg->prim.flags & NPQ2_SOLID32)
				MSG_WriteLong(msg, to->solid);
			else
				MSG_WriteSize16(msg, to->solid);
		}

	if (bits & Q2UEX_ALPHA)		MSG_WriteByte(msg, 0);
	if (bits & Q2UEX_SCALE)		MSG_WriteByte(msg, 0);
	if (bits & Q2UEX_INSTANCE)	MSG_WriteByte(msg, 0);
	if (bits & Q2UEX_OWNER)		MSG_WriteShort(msg, 0);
	if (bits & Q2UEX_OLDFRAME)	MSG_WriteShort(msg, 0);
}


/*
=============
SV_EmitPacketEntities

Writes a delta update of an entity_state_t list to the message.
=============
*/
void SVQ2_EmitPacketEntities (q2client_frame_t *from, q2client_frame_t *to, sizebuf_t *msg, qboolean q2ex)
{
	q2entity_state_t	*oldent, *newent;
	int		oldindex, newindex;
	int		oldnum, newnum;
	int		from_num_entities;
	int		bits;

	MSG_WriteByte (msg, svcq2_packetentities);

	if (!from)
		from_num_entities = 0;
	else
		from_num_entities = from->num_entities;

	newindex = 0;
	oldindex = 0;
	while (newindex < to->num_entities || oldindex < from_num_entities)
	{
		if (newindex >= to->num_entities)
		{
			newent = NULL;	//shh compiler, shh...
			newnum = 9999;
		}
		else
		{
			newent = &svs_client_entities[(to->first_entity+newindex)%svs_num_client_entities];
			newnum = newent->number;
		}

		if (oldindex >= from_num_entities)
		{
			oldent = NULL;	//shh compiler, shh...
			oldnum = 9999;
		}
		else
		{
			oldent = &svs_client_entities[(from->first_entity+oldindex)%svs_num_client_entities];
			oldnum = oldent->number;
		}

		if (newnum == oldnum)
		{	// delta update from old position
			// because the force parm is false, this will not result
			// in any bytes being emited if the entity has not changed at all
			// note that players are always 'newentities', this updates their oldorigin always
			// and prevents warping
			if (msg->cursize+128 > msg->maxsize)
				memcpy(newent, oldent, sizeof(*newent));	//too much data, so set the ent up as the same as the old, so it's sent next frame
			else
				MSGQ2_WriteDeltaEntity (oldent, newent, msg, false, newent->number <= svs.allocated_client_slots, q2ex);
			oldindex++;
			newindex++;
			continue;
		}

		if (newnum < oldnum)
		{	// this is a new entity, send it from the baseline

			if (msg->cursize+128 > msg->maxsize)
			{	//might cause the packet to overflow
				//so strip out this ent, we can add it next frame if it's still relevent
				to->num_entities--;
				memmove(newent, newent+1, sizeof(*newent) * (to->num_entities-newindex));
			}
			else
			{
				MSGQ2_WriteDeltaEntity (&sv_baselines[newnum], newent, msg, true, true, q2ex);
				newindex++;
			}
			continue;
		}

		if (newnum > oldnum)
		{	// the old entity isn't present in the new message
			bits = Q2U_REMOVE;
			if (oldnum >= 256)
				bits |= Q2U_NUMBER16 | Q2U_MOREBITS1;

			MSG_WriteByte (msg,	bits&255 );
			if (bits & 0x0000ff00)
				MSG_WriteByte (msg,	(bits>>8)&255 );

			if (bits & Q2U_NUMBER16)
				MSG_WriteShort (msg, oldnum);
			else
				MSG_WriteByte (msg, oldnum);

			oldindex++;
			continue;
		}
	}

	MSG_WriteShort (msg, 0);	// end of packetentities

#if 0
	if (numprojs)
		SV_EmitProjectileUpdate(msg);
#endif
}



/*
=============
SV_WritePlayerstateToClient

=============
*/
void SVQ2_WritePlayerstateToClient (client_t *client, int seat, int extflags, q2client_frame_t *from, q2client_frame_t *to, sizebuf_t *msg)
{
	int				i;
	int				pflags;
	q2player_state_t	*ps, *ops;
	q2player_state_t	dummy;
	int				statbits;
	unsigned int pext = client->fteprotocolextensions;
	unsigned int q2e = client->protocol == SCP_QUAKE2EX;


	ps = &to->ps[seat];
	if (!from)
	{
		memset (&dummy, 0, sizeof(dummy));
		ops = &dummy;
	}
	else
		ops = &from->ps[seat];

	//
	// determine what needs to be sent
	//
	pflags = 0;

	if (pext & PEXT_SPLITSCREEN)
		if (!from || from->clientnum[seat] != to->clientnum[seat])
			pflags |= Q2FTEPS_CLIENTNUM;

	if (ps->pmove.pm_type != ops->pmove.pm_type)
		pflags |= Q2PS_M_TYPE;

	if (ps->pmove.origin[0] != ops->pmove.origin[0]
		|| ps->pmove.origin[1] != ops->pmove.origin[1]
		|| ps->pmove.origin[2] != ops->pmove.origin[2] )
		pflags |= Q2PS_M_ORIGIN;

	if (ps->pmove.velocity[0] != ops->pmove.velocity[0]
		|| ps->pmove.velocity[1] != ops->pmove.velocity[1]
		|| ps->pmove.velocity[2] != ops->pmove.velocity[2] )
		pflags |= Q2PS_M_VELOCITY;

	if (ps->pmove.pm_time != ops->pmove.pm_time)
		pflags |= Q2PS_M_TIME;

	if (ps->pmove.pm_flags != ops->pmove.pm_flags)
		pflags |= Q2PS_M_FLAGS;

	if (ps->pmove.gravity != ops->pmove.gravity)
		pflags |= Q2PS_M_GRAVITY;

	if (ps->pmove.delta_angles[0] != ops->pmove.delta_angles[0]
		|| ps->pmove.delta_angles[1] != ops->pmove.delta_angles[1]
		|| ps->pmove.delta_angles[2] != ops->pmove.delta_angles[2] )
		pflags |= Q2PS_M_DELTA_ANGLES;


	if (ps->viewoffset[0] != ops->viewoffset[0]
		|| ps->viewoffset[1] != ops->viewoffset[1]
		|| ps->viewoffset[2] != ops->viewoffset[2] )
		pflags |= Q2PS_VIEWOFFSET;

	if (ps->viewangles[0] != ops->viewangles[0]
		|| ps->viewangles[1] != ops->viewangles[1]
		|| ps->viewangles[2] != ops->viewangles[2] )
		pflags |= Q2PS_VIEWANGLES;

	if (ps->kick_angles[0] != ops->kick_angles[0]
		|| ps->kick_angles[1] != ops->kick_angles[1]
		|| ps->kick_angles[2] != ops->kick_angles[2] )
		pflags |= Q2PS_KICKANGLES;

	if (ps->blend[0] != ops->blend[0]
		|| ps->blend[1] != ops->blend[1]
		|| ps->blend[2] != ops->blend[2]
		|| ps->blend[3] != ops->blend[3] )
		pflags |= Q2PS_BLEND;

	if (ps->fov != ops->fov)
		pflags |= Q2PS_FOV;

	if (ps->rdflags != ops->rdflags)
		pflags |= Q2PS_RDFLAGS;

	if (ps->gunframe != ops->gunframe)
		pflags |= Q2PS_WEAPONFRAME;

	if (ps->gunindex != ops->gunindex)
		pflags |= Q2PS_WEAPONINDEX;

	if (pext & PEXT_MODELDBL)
	{
		if ((pflags & Q2PS_WEAPONINDEX) && ps->gunindex > 0xff)
			pflags |= Q2FTEPS_INDEX16;
		if ((pflags & Q2PS_WEAPONFRAME) && ps->gunframe > 0xff)
			pflags |= Q2FTEPS_INDEX16;
	}

	if (pflags > 0xffff)
		pflags |= Q2PS_EXTRABITS;

	//
	// write it
	//
	MSG_WriteByte (msg, svcq2_playerinfo);
	MSG_WriteShort (msg, pflags&0xffff);
	if (pflags & Q2PS_EXTRABITS)
	{
		if (q2e)
			MSG_WriteShort (msg, pflags>>16);
		else
			MSG_WriteByte (msg, pflags>>16);
	}

	//
	// write the pmove_state_t
	//
	if (pflags & Q2PS_M_TYPE)
	{
		i = ps->pmove.pm_type;
		if (q2e)
		{	//sigh... q2e added some extra pmove types that we don't support, and not on the end. :(
			switch((q2pmtype_t)i)
			{
			case Q2PM_NORMAL:		i = Q2EPM_NORMAL;		break;
			//case Q2PM_GRAPPLE:	i = Q2EPM_GRAPPLE;		break;
			case Q2PM_SPECTATOR:	i = Q2EPM_SPECTATOR;	break;
			//case Q2PM_SPECTATOR2:	i = Q2EPM_SPECTATOR2;	break;
			case Q2PM_DEAD:			i = Q2EPM_DEAD;			break;
			case Q2PM_GIB:			i = Q2EPM_GIB;			break;
			case Q2PM_FREEZE:		i = Q2EPM_FREEZE;		break;
			}
		}
		MSG_WriteByte (msg, i);
	}

	if (q2e)
	{
		if (pflags & Q2PS_M_ORIGIN)
		{
			MSG_WriteFloat (msg, ps->pmove.origin[0]/8.0);
			MSG_WriteFloat (msg, ps->pmove.origin[1]/8.0);
			MSG_WriteFloat (msg, ps->pmove.origin[2]/8.0);
		}

		if (pflags & Q2PS_M_VELOCITY)
		{
			MSG_WriteFloat (msg, ps->pmove.velocity[0]/8.0);
			MSG_WriteFloat (msg, ps->pmove.velocity[1]/8.0);
			MSG_WriteFloat (msg, ps->pmove.velocity[2]/8.0);
		}
	}
	else
	{
		if (pflags & Q2PS_M_ORIGIN)
		{
			MSG_WriteShort (msg, ps->pmove.origin[0]);
			MSG_WriteShort (msg, ps->pmove.origin[1]);
			MSG_WriteShort (msg, ps->pmove.origin[2]);
		}

		if (pflags & Q2PS_M_VELOCITY)
		{
			MSG_WriteShort (msg, ps->pmove.velocity[0]);
			MSG_WriteShort (msg, ps->pmove.velocity[1]);
			MSG_WriteShort (msg, ps->pmove.velocity[2]);
		}
	}

	if (pflags & Q2PS_M_TIME)
	{
		if (q2e)
			MSG_WriteShort (msg, ps->pmove.pm_time);
		else
			MSG_WriteByte (msg, ps->pmove.pm_time);
	}

	if (pflags & Q2PS_M_FLAGS)
	{
		if (q2e)
			MSG_WriteShort (msg, ps->pmove.pm_flags);
		else
			MSG_WriteByte (msg, ps->pmove.pm_flags);
	}

	if (pflags & Q2PS_M_GRAVITY)
		MSG_WriteShort (msg, ps->pmove.gravity);

	if (pflags & Q2PS_M_DELTA_ANGLES)
	{
		if (q2e)
		{
			MSG_WriteFloat (msg, SHORT2ANGLE(ps->pmove.delta_angles[0]));
			MSG_WriteFloat (msg, SHORT2ANGLE(ps->pmove.delta_angles[1]));
			MSG_WriteFloat (msg, SHORT2ANGLE(ps->pmove.delta_angles[2]));
		}
		else
		{
			MSG_WriteShort (msg, ps->pmove.delta_angles[0]);
			MSG_WriteShort (msg, ps->pmove.delta_angles[1]);
			MSG_WriteShort (msg, ps->pmove.delta_angles[2]);
		}
	}

	//
	// write the rest of the player_state_t
	//
	if (pflags & Q2PS_VIEWOFFSET)
	{
		if (q2e)
		{
			MSG_WriteShort (msg, ps->viewoffset[0]*16);
			MSG_WriteShort (msg, ps->viewoffset[1]*16);
			MSG_WriteShort (msg, (ps->viewoffset[2]-DEFAULT_VIEWHEIGHT)*16);
			MSG_WriteChar (msg, DEFAULT_VIEWHEIGHT/*ps->pmove.viewheight*/);
		}
		else
		{
			MSG_WriteChar (msg, ps->viewoffset[0]*4);
			MSG_WriteChar (msg, ps->viewoffset[1]*4);
			MSG_WriteChar (msg, ps->viewoffset[2]*4);
		}
	}


	if (pflags & Q2PS_VIEWANGLES)
	{
		if (q2e)
		{
			MSG_WriteFloat (msg, ps->viewangles[0]);
			MSG_WriteFloat (msg, ps->viewangles[1]);
			MSG_WriteFloat (msg, ps->viewangles[2]);
		}
		else
		{
			MSG_WriteAngle16 (msg, ps->viewangles[0]);
			MSG_WriteAngle16 (msg, ps->viewangles[1]);
			MSG_WriteAngle16 (msg, ps->viewangles[2]);
		}
	}

	if (pflags & Q2PS_KICKANGLES)
	{
		if (q2e)
		{
			MSG_WriteShort (msg, ps->kick_angles[0]*1024);
			MSG_WriteShort (msg, ps->kick_angles[1]*1024);
			MSG_WriteShort (msg, ps->kick_angles[2]*1024);
		}
		else
		{
			MSG_WriteChar (msg, ps->kick_angles[0]*4);
			MSG_WriteChar (msg, ps->kick_angles[1]*4);
			MSG_WriteChar (msg, ps->kick_angles[2]*4);
		}
	}

	if (pflags & Q2PS_WEAPONINDEX)
	{
		if (q2e)
			MSG_WriteShort(msg, (0/*gunskin*/<<13) | ps->gunindex);
		else
		{
			if (pflags & Q2FTEPS_INDEX16)
				MSG_WriteShort(msg, ps->gunindex);
			else
				MSG_WriteByte (msg, ps->gunindex);
		}
	}

	if (pflags & Q2PS_WEAPONFRAME)
	{
		if (q2e)
		{
			unsigned short fl = ps->gunframe&0x1ff;
			if (ps->gunoffset[0]) fl |= 1<<9;
			if (ps->gunoffset[1]) fl |= 1<<10;
			if (ps->gunoffset[2]) fl |= 1<<11;
			if (ps->gunangles[0]) fl |= 1<<12;
			if (ps->gunangles[1]) fl |= 1<<13;
			if (ps->gunangles[2]) fl |= 1<<14;
			//if (ps->gunrate[2]) fl |= 1<<15;

			MSG_WriteShort (msg, fl);
			if (fl & (1<<9))	MSG_WriteFloat(msg, ps->gunoffset[0]);
			if (fl & (1<<10))	MSG_WriteFloat(msg, ps->gunoffset[1]);
			if (fl & (1<<11))	MSG_WriteFloat(msg, ps->gunoffset[2]);
			if (fl & (1<<12))	MSG_WriteFloat(msg, ps->gunangles[0]);
			if (fl & (1<<13))	MSG_WriteFloat(msg, ps->gunangles[1]);
			if (fl & (1<<14))	MSG_WriteFloat(msg, ps->gunangles[2]);
			//if (fl & (1<<15))	MSG_WriteByte(msg, ps->gunrate);
		}
		else
		{
			if (pflags & Q2FTEPS_INDEX16)
				MSG_WriteShort (msg, ps->gunframe);
			else
				MSG_WriteByte (msg, ps->gunframe);
			MSG_WriteChar (msg, ps->gunoffset[0]*4);
			MSG_WriteChar (msg, ps->gunoffset[1]*4);
			MSG_WriteChar (msg, ps->gunoffset[2]*4);
			MSG_WriteChar (msg, ps->gunangles[0]*4);
			MSG_WriteChar (msg, ps->gunangles[1]*4);
			MSG_WriteChar (msg, ps->gunangles[2]*4);
		}
	}

	if (pflags & Q2PS_BLEND)
	{
		MSG_WriteByte (msg, ps->blend[0]*255);
		MSG_WriteByte (msg, ps->blend[1]*255);
		MSG_WriteByte (msg, ps->blend[2]*255);
		MSG_WriteByte (msg, ps->blend[3]*255);
	}

	if (pflags & Q2PS_FOV)
		MSG_WriteByte (msg, ps->fov);
	if (pflags & Q2PS_RDFLAGS)
		MSG_WriteByte (msg, ps->rdflags);

	// send stats
	statbits = 0;
	for (i=0 ; i<min(32,Q2MAX_STATS); i++)
		if (ps->stats[i] != ops->stats[i])
			statbits |= 1<<i;
	MSG_WriteLong (msg, statbits);
	for (i=0 ; i<min(32,Q2MAX_STATS) ; i++)
		if (statbits & (1<<i) )
			MSG_WriteShort (msg, ps->stats[i]);

	if (q2e)
	{
		statbits = 0;
		for (i=0 ; i<Q2MAX_STATS-32 ; i++)
			if (ps->stats[32+i] != ops->stats[32+i])
				statbits |= 1<<i;
		MSG_WriteLong (msg, statbits);
		for (i=0 ; i<Q2MAX_STATS-32 ; i++)
			if (statbits & (1<<i) )
				MSG_WriteShort (msg, ps->stats[32+i]);

		if (pflags & Q2EXPS_DAMAGEBLEND)
		{
			MSG_WriteByte(msg, 0*255);
			MSG_WriteByte(msg, 0*255);
			MSG_WriteByte(msg, 0*255);
			MSG_WriteByte(msg, 0*255);
		}
		if (pflags & Q2EXPS_TEAMID)
			MSG_WriteByte(msg, 0);
	}
	else
	{
		if ((extflags & Q2PSX_CLIENTNUM) || (pflags & Q2FTEPS_CLIENTNUM))
			MSG_WriteByte(msg, to->clientnum[seat]);
	}
}


/*
==================
SV_WriteFrameToClient
==================
*/
void SVQ2_WriteFrameToClient (client_t *client, sizebuf_t *msg)
{
	q2client_frame_t		*frame, *oldframe;
	int					lastframe;
	client_t			*split;
	int seat;
	int extflags = 0;

//Com_Printf ("%i -> %i\n", client->lastframe, sv.framenum);
	// this is the frame we are creating
	frame = &client->frameunion.q2frames[sv.framenum & Q2UPDATE_MASK];

	if (client->delta_sequence <= 0)
	{	// client is asking for a retransmit
		oldframe = NULL;
		lastframe = -1;
	}
	else if (sv.framenum - client->delta_sequence >= (Q2UPDATE_BACKUP - 3) )
	{	// client hasn't gotten a good message through in a long time
//		Com_Printf ("%s: Delta request from out-of-date packet.\n", client->name);
		oldframe = NULL;
		lastframe = -1;
	}
	else
	{	// we have a valid message to delta from
		oldframe = &client->frameunion.q2frames[client->delta_sequence & Q2UPDATE_MASK];
		lastframe = client->delta_sequence;
	}

	MSG_WriteByte (msg, svcq2_frame);
	MSG_WriteLong (msg, sv.framenum);
	MSG_WriteLong (msg, lastframe);	// what we are delta'ing from
	MSG_WriteByte (msg, client->chokecount&0xff);	// rate dropped packets
	extflags |= Q2PSX_OLD;
	client->chokecount = 0;

	if (client->protocol == SCP_QUAKE2EX)
	{
		for (split = client, seat = 0; split; split = split->controlled, seat++)
		{
			// send over the areabits, private per-seat
			MSG_WriteByte (msg, frame->areabytes);
			SZ_Write (msg, frame->areabits, frame->areabytes);

			// delta encode the playerstate
			SVQ2_WritePlayerstateToClient (client, seat, extflags, oldframe, frame, msg);
		}
	}
	else
	{
		// send over the areabits, shared between all seats
		MSG_WriteByte (msg, frame->areabytes);
		SZ_Write (msg, frame->areabits, frame->areabytes);

		// delta encode the playerstate
		for (split = client, seat = 0; split; split = split->controlled, seat++)
			SVQ2_WritePlayerstateToClient (client, seat, extflags, oldframe, frame, msg);
	}

	// delta encode the entities
	SVQ2_EmitPacketEntities (oldframe, frame, msg, client->protocol==SCP_QUAKE2EX);
}


/*
=============================================================================

Build a client frame structure

=============================================================================
*/

/*
=============
SV_BuildClientFrame

Decides which entities are going to be visible to the client, and
copies off the playerstat and areabits.
=============
*/
void SVQ2_Ents_Init(void);
void SVQ2_BuildClientFrame (client_t *client)
{
	int		e, i;
	vec3_t org[MAX_SPLITS];
	int		clientarea[MAX_SPLITS];
	q2edict_t	*clent[MAX_SPLITS];
	client_t	*split;
	q2edict_t	*ent;
	q2client_frame_t	*frame;
	q2entity_state_t	*state;
	int		l;
	int		seat;
	int		c_fullsend;
	pvsbuffer_t	clientpvs;
	qbyte	*clientphs = NULL;
	int		seats;

	if (client->state < cs_spawned)
		return;

	SVQ2_Ents_Init();

	clientpvs.buffer = alloca(clientpvs.buffersize=sv.world.worldmodel->pvsbytes);

#if 0
	numprojs = 0; // no projectiles yet
#endif

	// this is the frame the client will be acking (EVIL HACKS!)
	frame = &client->frameunion.q2frames[client->netchan.outgoing_sequence & Q2UPDATE_MASK];

	frame->senttime = realtime*1000; // save it for ping calc later

	// this is the frame we are creating
	frame = &client->frameunion.q2frames[sv.framenum & Q2UPDATE_MASK];

	// grab the current player_state_t
	for (seat = 0, split = client; split; split = split->controlled, seat++)
	{
		int		clientcluster;

		clent[seat] = split->q2edict;
		frame->clientnum[seat] = split - svs.clients;

		if (!split->q2edict->client)
		{	//shouldn't happen
			VectorClear(org[seat]);
			clientarea[seat] = 0;
			memset(&frame->ps[seat], 0, sizeof(frame->ps[seat]));
			frame->ps[seat].pmove.pm_type = Q2PM_FREEZE;
			continue;
		}

		// find the client's PVS
		for (i=0 ; i<3 ; i++)
			org[seat][i] = clent[seat]->client->ps.pmove.origin[i]*0.125 + clent[seat]->client->ps.viewoffset[i];

		clientcluster = sv.world.worldmodel->funcs.ClusterForPoint (sv.world.worldmodel, org[seat], &clientarea[seat]);

		// calculate the visible areas
		frame->areabytes = sv.world.worldmodel->funcs.WriteAreaBits (sv.world.worldmodel, frame->areabits, sizeof(frame->areabits), clientarea[seat], seat != 0);

		sv.world.worldmodel->funcs.FatPVS(sv.world.worldmodel, org[seat], &clientpvs, seat!=0);
		if (seat==0)	//FIXME
			clientphs = sv.world.worldmodel->funcs.ClusterPHS (sv.world.worldmodel, clientcluster, NULL);

		frame->ps[seat] = clent[seat]->client->ps;
		if (sv.paused)
			frame->ps[seat].pmove.pm_type = Q2PM_FREEZE;
	}
	seats = seat;
	for (; seat < MAX_SPLITS; seat++)
	{
		memset(&frame->ps[seat], 0, sizeof(frame->ps[seat]));
		frame->clientnum[seat] = 0xff;	//invalid
	}

	// build up the list of visible entities
	frame->num_entities = 0;
	frame->first_entity = svs_next_client_entities;

	c_fullsend = 0;

	for (e=1 ; e<ge->num_edicts ; e++)
	{
		ent = Q2EDICT_NUM(e);

		// ignore ents without visible models
		if (ent->svflags & SVF_NOCLIENT)
			continue;

		// ignore ents without visible models unless they have an effect
		if (!ent->s.modelindex && !ent->s.effects && !ent->s.sound
			&& !ent->s.event)
			continue;

		for (seat = 0; seat < seats; seat++)
		{
			// ignore if not touching a PV leaf
			if (ent != clent[seat])
			{
				// check area
				if (!sv.world.worldmodel->funcs.AreasConnected (sv.world.worldmodel, clientarea[seat], ent->areanum))
				{	// doors can legally straddle two areas, so
					// we may need to check another one
					if (!ent->areanum2
						|| !sv.world.worldmodel->funcs.AreasConnected (sv.world.worldmodel, clientarea[seat], ent->areanum2))
						continue;		// blocked by a door
				}

				// beams just check one point for PHS
				if (ent->s.renderfx & Q2RF_BEAM)
				{
					l = ent->clusternums[0];
					if ( !(clientphs[l >> 3] & (1 << (l&7) )) )
						continue;
				}
				else
				{
					// FIXME: if an ent has a model and a sound, but isn't
					// in the PVS, only the PHS, clear the model

					if (ent->num_clusters == -1)
					{	// too many leafs for individual check, go by headnode
						pvscache_t cache;
						cache.num_leafs = -1;
						cache.headnode = ent->headnode;
						if (!sv.world.worldmodel->funcs.EdictInFatPVS(sv.world.worldmodel, &cache, clientpvs.buffer, NULL))
							continue;
						c_fullsend++;
					}
					else
					{	// check individual leafs
						for (i=0 ; i < ent->num_clusters ; i++)
						{
							l = ent->clusternums[i];
							if (clientpvs.buffer[l >> 3] & (1 << (l&7) ))
								break;
						}
						if (i == ent->num_clusters)
							continue;		// not visible
					}

					if (!ent->s.modelindex)
					{	// don't send sounds if they will be attenuated away
						vec3_t	delta;
						float	len;

						VectorSubtract (org[seat], ent->s.origin, delta);
						len = Length (delta);
						if (len > 400)
							continue;
					}
				}
			}
			break;
		}
		if (seat == seats)
			continue;	//not visible to any seat
		seat = 0; //FIXME

		// add it to the circular client_entities array
		state = &svs_client_entities[svs_next_client_entities%svs_num_client_entities];
		if (ent->s.number != e)
		{
			Con_DPrintf ("FIXING ENT->S.NUMBER!!!\n");
			ent->s.number = e;
		}
		*state = ent->s;

		// don't mark players missiles as solid
		if (ent->owner == clent[seat])
			state->solid = 0;

		svs_next_client_entities++;
		frame->num_entities++;
	}
}

void SVQ2_BuildBaselines(void)
{
	unsigned int e;
	q2edict_t	*ent;
	q2entity_state_t	*base;

	if (!ge)
		return;

	for (e=1 ; e<ge->num_edicts ; e++)
	{
		ent = Q2EDICT_NUM(e);
		base = &ent->s;

		if (base->modelindex || base->sound || base->effects)
			sv_baselines[e] = *base;
	}
}

void SVQ2_Ents_Init(void)
{
	extern cvar_t	maxclients;
	if (!svs_client_entities)
	{
		svs_num_client_entities = maxclients.value*Q2UPDATE_BACKUP*64;
		svs_client_entities = Z_Malloc (sizeof(entity_state_t)*svs_num_client_entities);
	}
}
void SVQ2_Ents_Shutdown(void)
{
	if (svs_client_entities)
	{
		Z_Free(svs_client_entities);
		svs_client_entities = NULL;
		svs_num_client_entities = 0;
	}
}
#endif

#endif
