#include "quakedef.h"
#include "protocol.h"

extern cvar_t sv_cullentities_trace_prediction_time;

int EntityState_DeltaBits(const entity_state_t *o, const entity_state_t *n)
{
	unsigned int bits;
	// if o is not active, delta from default
	if (o->active != ACTIVE_NETWORK)
		o = &defaultstate;
	bits = 0;
	if (fabs(n->origin[0] - o->origin[0]) > (1.0f / 256.0f))
		bits |= E_ORIGIN1;
	if (fabs(n->origin[1] - o->origin[1]) > (1.0f / 256.0f))
		bits |= E_ORIGIN2;
	if (fabs(n->origin[2] - o->origin[2]) > (1.0f / 256.0f))
		bits |= E_ORIGIN3;
	if ((unsigned char) (n->angles[0] * (256.0f / 360.0f)) != (unsigned char) (o->angles[0] * (256.0f / 360.0f)))
		bits |= E_ANGLE1;
	if ((unsigned char) (n->angles[1] * (256.0f / 360.0f)) != (unsigned char) (o->angles[1] * (256.0f / 360.0f)))
		bits |= E_ANGLE2;
	if ((unsigned char) (n->angles[2] * (256.0f / 360.0f)) != (unsigned char) (o->angles[2] * (256.0f / 360.0f)))
		bits |= E_ANGLE3;
	if ((n->modelindex ^ o->modelindex) & 0x00FF)
		bits |= E_MODEL1;
	if ((n->modelindex ^ o->modelindex) & 0xFF00)
		bits |= E_MODEL2;
	if ((n->frame ^ o->frame) & 0x00FF)
		bits |= E_FRAME1;
	if ((n->frame ^ o->frame) & 0xFF00)
		bits |= E_FRAME2;
	if ((n->effects ^ o->effects) & 0x00FF)
		bits |= E_EFFECTS1;
	if ((n->effects ^ o->effects) & 0xFF00)
		bits |= E_EFFECTS2;
	if (n->colormap != o->colormap)
		bits |= E_COLORMAP;
	if (n->skin != o->skin)
		bits |= E_SKIN;
	if (n->alpha != o->alpha)
		bits |= E_ALPHA;
	if (n->scale != o->scale)
		bits |= E_SCALE;
	if (n->glowsize != o->glowsize)
		bits |= E_GLOWSIZE;
	if (n->glowcolor != o->glowcolor)
		bits |= E_GLOWCOLOR;
	if (n->flags != o->flags)
		bits |= E_FLAGS;
	if (n->tagindex != o->tagindex || n->tagentity != o->tagentity)
		bits |= E_TAGATTACHMENT;
	if (n->light[0] != o->light[0] || n->light[1] != o->light[1] || n->light[2] != o->light[2] || n->light[3] != o->light[3])
		bits |= E_LIGHT;
	if (n->lightstyle != o->lightstyle)
		bits |= E_LIGHTSTYLE;
	if (n->lightpflags != o->lightpflags)
		bits |= E_LIGHTPFLAGS;

	if (bits)
	{
		if (bits &  0xFF000000)
			bits |= 0x00800000;
		if (bits &  0x00FF0000)
			bits |= 0x00008000;
		if (bits &  0x0000FF00)
			bits |= 0x00000080;
	}
	return bits;
}

void EntityState_WriteExtendBits(sizebuf_t *msg, unsigned int bits)
{
	MSG_WriteByte(msg, bits & 0xFF);
	if (bits & 0x00000080)
	{
		MSG_WriteByte(msg, (bits >> 8) & 0xFF);
		if (bits & 0x00008000)
		{
			MSG_WriteByte(msg, (bits >> 16) & 0xFF);
			if (bits & 0x00800000)
				MSG_WriteByte(msg, (bits >> 24) & 0xFF);
		}
	}
}

void EntityState_WriteFields(const entity_state_t *ent, sizebuf_t *msg, unsigned int bits)
{
	if (sv.protocol == PROTOCOL_DARKPLACES2)
	{
		if (bits & E_ORIGIN1)
			MSG_WriteCoord16i(msg, ent->origin[0]);
		if (bits & E_ORIGIN2)
			MSG_WriteCoord16i(msg, ent->origin[1]);
		if (bits & E_ORIGIN3)
			MSG_WriteCoord16i(msg, ent->origin[2]);
	}
	else
	{
		// LadyHavoc: have to write flags first, as they can modify protocol
		if (bits & E_FLAGS)
			MSG_WriteByte(msg, ent->flags);
		if (ent->flags & RENDER_LOWPRECISION)
		{
			if (bits & E_ORIGIN1)
				MSG_WriteCoord16i(msg, ent->origin[0]);
			if (bits & E_ORIGIN2)
				MSG_WriteCoord16i(msg, ent->origin[1]);
			if (bits & E_ORIGIN3)
				MSG_WriteCoord16i(msg, ent->origin[2]);
		}
		else
		{
			if (bits & E_ORIGIN1)
				MSG_WriteCoord32f(msg, ent->origin[0]);
			if (bits & E_ORIGIN2)
				MSG_WriteCoord32f(msg, ent->origin[1]);
			if (bits & E_ORIGIN3)
				MSG_WriteCoord32f(msg, ent->origin[2]);
		}
	}
	if ((sv.protocol == PROTOCOL_DARKPLACES1 || sv.protocol == PROTOCOL_DARKPLACES2 || sv.protocol == PROTOCOL_DARKPLACES3 || sv.protocol == PROTOCOL_DARKPLACES4) && (ent->flags & RENDER_LOWPRECISION))
	{
		if (bits & E_ANGLE1)
			MSG_WriteAngle8i(msg, ent->angles[0]);
		if (bits & E_ANGLE2)
			MSG_WriteAngle8i(msg, ent->angles[1]);
		if (bits & E_ANGLE3)
			MSG_WriteAngle8i(msg, ent->angles[2]);
	}
	else
	{
		if (bits & E_ANGLE1)
			MSG_WriteAngle16i(msg, ent->angles[0]);
		if (bits & E_ANGLE2)
			MSG_WriteAngle16i(msg, ent->angles[1]);
		if (bits & E_ANGLE3)
			MSG_WriteAngle16i(msg, ent->angles[2]);
	}
	if (bits & E_MODEL1)
		MSG_WriteByte(msg, ent->modelindex & 0xFF);
	if (bits & E_MODEL2)
		MSG_WriteByte(msg, (ent->modelindex >> 8) & 0xFF);
	if (bits & E_FRAME1)
		MSG_WriteByte(msg, ent->frame & 0xFF);
	if (bits & E_FRAME2)
		MSG_WriteByte(msg, (ent->frame >> 8) & 0xFF);
	if (bits & E_EFFECTS1)
		MSG_WriteByte(msg, ent->effects & 0xFF);
	if (bits & E_EFFECTS2)
		MSG_WriteByte(msg, (ent->effects >> 8) & 0xFF);
	if (bits & E_COLORMAP)
		MSG_WriteByte(msg, ent->colormap);
	if (bits & E_SKIN)
		MSG_WriteByte(msg, ent->skin);
	if (bits & E_ALPHA)
		MSG_WriteByte(msg, ent->alpha);
	if (bits & E_SCALE)
		MSG_WriteByte(msg, ent->scale);
	if (bits & E_GLOWSIZE)
		MSG_WriteByte(msg, ent->glowsize);
	if (bits & E_GLOWCOLOR)
		MSG_WriteByte(msg, ent->glowcolor);
	if (sv.protocol == PROTOCOL_DARKPLACES2)
		if (bits & E_FLAGS)
			MSG_WriteByte(msg, ent->flags);
	if (bits & E_TAGATTACHMENT)
	{
		MSG_WriteShort(msg, ent->tagentity);
		MSG_WriteByte(msg, ent->tagindex);
	}
	if (bits & E_LIGHT)
	{
		MSG_WriteShort(msg, ent->light[0]);
		MSG_WriteShort(msg, ent->light[1]);
		MSG_WriteShort(msg, ent->light[2]);
		MSG_WriteShort(msg, ent->light[3]);
	}
	if (bits & E_LIGHTSTYLE)
		MSG_WriteByte(msg, ent->lightstyle);
	if (bits & E_LIGHTPFLAGS)
		MSG_WriteByte(msg, ent->lightpflags);
}

void EntityState_WriteUpdate(const entity_state_t *ent, sizebuf_t *msg, const entity_state_t *delta)
{
	prvm_prog_t *prog = SVVM_prog;
	unsigned int bits;
	if (ent->active == ACTIVE_NETWORK)
	{
		// entity is active, check for changes from the delta
		if ((bits = EntityState_DeltaBits(delta, ent)))
		{
			// write the update number, bits, and fields
			ENTITYSIZEPROFILING_START(msg, ent->number, bits);
			MSG_WriteShort(msg, ent->number);
			EntityState_WriteExtendBits(msg, bits);
			EntityState_WriteFields(ent, msg, bits);
			ENTITYSIZEPROFILING_END(msg, ent->number, bits);
		}
	}
	else
	{
		// entity is inactive, check if the delta was active
		if (delta->active == ACTIVE_NETWORK)
		{
			// write the remove number
			ENTITYSIZEPROFILING_START(msg, ent->number, 0);
			MSG_WriteShort(msg, ent->number | 0x8000);
			ENTITYSIZEPROFILING_END(msg, ent->number, 0);
		}
	}
}

// (server) adds a entity_frame to the database, for future reference
void EntityFrame_AddFrame_Server(entityframe_database_t *d, vec3_t eye, int framenum, int numentities, const entity_state_t **entitydata)
{
	int n, e;
	entity_frameinfo_t *info;

	VectorCopy(eye, d->eye);

	// figure out how many entity slots are used already
	if (d->numframes)
	{
		n = d->frames[d->numframes - 1].endentity - d->frames[0].firstentity;
		if (n + numentities > MAX_ENTITY_DATABASE || d->numframes >= MAX_ENTITY_HISTORY)
		{
			// ran out of room, dump database
			EntityFrame_ClearDatabase(d);
		}
	}

	info = &d->frames[d->numframes];
	info->framenum = framenum;
	e = -1000;
	// make sure we check the newly added frame as well, but we haven't incremented numframes yet
	for (n = 0;n <= d->numframes;n++)
	{
		if (e >= d->frames[n].framenum)
		{
			if (e == framenum)
				Con_Print("EntityFrame_AddFrame: tried to add out of sequence frame to database\n");
			else
				Con_Print("EntityFrame_AddFrame: out of sequence frames in database\n");
			return;
		}
		e = d->frames[n].framenum;
	}
	// if database still has frames after that...
	if (d->numframes)
		info->firstentity = d->frames[d->numframes - 1].endentity;
	else
		info->firstentity = 0;
	info->endentity = info->firstentity + numentities;
	d->numframes++;

	n = info->firstentity % MAX_ENTITY_DATABASE;
	e = MAX_ENTITY_DATABASE - n;
	if (e > numentities)
		e = numentities;
	memcpy(d->entitydata + n, entitydata, sizeof(entity_state_t) * e);
	if (numentities > e)
		memcpy(d->entitydata, entitydata + e, sizeof(entity_state_t) * (numentities - e));
}

// (server) writes a frame to network stream
qbool EntityFrame_WriteFrame(sizebuf_t *msg, int maxsize, entityframe_database_t *d, int numstates, const entity_state_t **states, int viewentnum)
{
	prvm_prog_t *prog = SVVM_prog;
	int i, onum, number;
	entity_frame_t *o = &d->deltaframe;
	const entity_state_t *ent, *delta;
	vec3_t eye;

	d->latestframenum++;

	VectorClear(eye);
	for (i = 0;i < numstates;i++)
	{
		ent = states[i];
		if (ent->number == viewentnum)
		{
			VectorSet(eye, ent->origin[0], ent->origin[1], ent->origin[2] + 22);
			break;
		}
	}

	EntityFrame_AddFrame_Server(d, eye, d->latestframenum, numstates, states);

	EntityFrame_FetchFrame(d, d->ackframenum, o);

	MSG_WriteByte (msg, svc_entities);
	MSG_WriteLong (msg, o->framenum);
	MSG_WriteLong (msg, d->latestframenum);
	MSG_WriteFloat (msg, eye[0]);
	MSG_WriteFloat (msg, eye[1]);
	MSG_WriteFloat (msg, eye[2]);

	onum = 0;
	for (i = 0;i < numstates;i++)
	{
		ent = states[i];
		number = ent->number;

		if (PRVM_serveredictfunction((&prog->edicts[number]), SendEntity))
			continue;
		for (;onum < o->numentities && o->entitydata[onum].number < number;onum++)
		{
			// write remove message
			MSG_WriteShort(msg, o->entitydata[onum].number | 0x8000);
		}
		if (onum < o->numentities && (o->entitydata[onum].number == number))
		{
			// delta from previous frame
			delta = o->entitydata + onum;
			// advance to next entity in delta frame
			onum++;
		}
		else
		{
			// delta from defaults
			delta = &defaultstate;
		}
		EntityState_WriteUpdate(ent, msg, delta);
	}
	for (;onum < o->numentities;onum++)
	{
		// write remove message
		MSG_WriteShort(msg, o->entitydata[onum].number | 0x8000);
	}
	MSG_WriteShort(msg, 0xFFFF);

	return true;
}

void SV_WriteEntitiesToClient(client_t *client, prvm_edict_t *clent, sizebuf_t *msg, int maxsize)
{
	prvm_prog_t *prog = SVVM_prog;
	qbool need_empty = false;
	int i, numsendstates, numcsqcsendstates;
	entity_state_t *s;
	prvm_edict_t *camera;
	qbool success;
	vec3_t eye;

	// if there isn't enough space to accomplish anything, skip it
	if (msg->cursize + 25 > maxsize)
		return;

	sv.writeentitiestoclient_msg = msg;
	sv.writeentitiestoclient_clientnumber = client - svs.clients;

	sv.writeentitiestoclient_stats_culled_pvs = 0;
	sv.writeentitiestoclient_stats_culled_trace = 0;
	sv.writeentitiestoclient_stats_visibleentities = 0;
	sv.writeentitiestoclient_stats_totalentities = 0;
	sv.writeentitiestoclient_numeyes = 0;

	// get eye location
	sv.writeentitiestoclient_cliententitynumber = PRVM_EDICT_TO_PROG(clent); // LadyHavoc: for comparison purposes
	camera = PRVM_EDICT_NUM( client->clientcamera );
	VectorAdd(PRVM_serveredictvector(camera, origin), PRVM_serveredictvector(clent, view_ofs), eye);
	// get the PVS values for the eye location, later FatPVS calls will merge
	if (sv.worldmodel && sv.worldmodel->brush.FatPVS)
		sv.worldmodel->brush.FatPVS(sv.worldmodel, eye, 8, &sv.writeentitiestoclient_pvs, sv_mempool, false);
	else
		sv.writeentitiestoclient_pvs = NULL;

	// add the eye to a list for SV_CanSeeBox tests
	VectorCopy(eye, sv.writeentitiestoclient_eyes[sv.writeentitiestoclient_numeyes]);
	sv.writeentitiestoclient_numeyes++;

	// calculate predicted eye origin for SV_CanSeeBox tests
	if (sv_cullentities_trace_prediction.integer)
	{
		vec_t predtime = bound(0, host_client->ping, sv_cullentities_trace_prediction_time.value);
		vec3_t predeye;
		VectorMA(eye, predtime, PRVM_serveredictvector(camera, velocity), predeye);
		if (SV_CanSeeBox(1, 0, 0, 0, eye, predeye, predeye))
		{
			VectorCopy(predeye, sv.writeentitiestoclient_eyes[sv.writeentitiestoclient_numeyes]);
			sv.writeentitiestoclient_numeyes++;
		}
		//if (!sv.writeentitiestoclient_useprediction)
		//	Con_DPrintf("Trying to walk into solid in a pingtime... not predicting for culling\n");
	}

	SV_AddCameraEyes();

	// build PVS from the new eyes
	if (sv.worldmodel && sv.worldmodel->brush.FatPVS)
		for(i = 1; i < sv.writeentitiestoclient_numeyes; ++i)
			sv.worldmodel->brush.FatPVS(sv.worldmodel, sv.writeentitiestoclient_eyes[i], 8, &sv.writeentitiestoclient_pvs, sv_mempool, sv.writeentitiestoclient_pvs != NULL);

	sv.sententitiesmark++;

	for (i = 0;i < sv.numsendentities;i++)
		SV_MarkWriteEntityStateToClient(sv.sendentities + i, client);

	numsendstates = 0;
	numcsqcsendstates = 0;
	for (i = 0;i < sv.numsendentities;i++)
	{
		s = &sv.sendentities[i];
		if (sv.sententities[s->number] == sv.sententitiesmark)
		{
			if(s->active == ACTIVE_NETWORK)
			{
				if (s->exteriormodelforclient)
				{
					if (s->exteriormodelforclient == sv.writeentitiestoclient_cliententitynumber)
						s->flags |= RENDER_EXTERIORMODEL;
					else
						s->flags &= ~RENDER_EXTERIORMODEL;
				}
				sv.writeentitiestoclient_sendstates[numsendstates++] = s;
			}
			else if(sv.sendentities[i].active == ACTIVE_SHARED)
				sv.writeentitiestoclient_csqcsendstates[numcsqcsendstates++] = s->number;
			else
				Con_Printf("entity %d is in sv.sendentities and marked, but not active, please breakpoint me\n", s->number);
		}
	}

	if (sv_cullentities_stats.integer)
		Con_Printf("client \"%s\" entities: %d total, %d visible, %d culled by: %d pvs %d trace\n", client->name, sv.writeentitiestoclient_stats_totalentities, sv.writeentitiestoclient_stats_visibleentities, sv.writeentitiestoclient_stats_culled_pvs + sv.writeentitiestoclient_stats_culled_trace, sv.writeentitiestoclient_stats_culled_pvs, sv.writeentitiestoclient_stats_culled_trace);

	if(client->entitydatabase5)
		need_empty = EntityFrameCSQC_WriteFrame(msg, maxsize, numcsqcsendstates, sv.writeentitiestoclient_csqcsendstates, client->entitydatabase5->latestframenum + 1);
	else
		EntityFrameCSQC_WriteFrame(msg, maxsize, numcsqcsendstates, sv.writeentitiestoclient_csqcsendstates, 0);

	// force every 16th frame to be not empty (or cl_movement replay takes
	// too long)
	// BTW, this should normally not kick in any more due to the check
	// below, except if the client stopped sending movement frames
	if(client->num_skippedentityframes >= 16)
		need_empty = true;

	// help cl_movement a bit more
	if(client->movesequence != client->lastmovesequence)
		need_empty = true;
	client->lastmovesequence = client->movesequence;

	if (client->entitydatabase5)
		success = EntityFrame5_WriteFrame(msg, maxsize, client->entitydatabase5, numsendstates, sv.writeentitiestoclient_sendstates, client - svs.clients + 1, client->movesequence, need_empty);
	else if (client->entitydatabase4)
	{
		success = EntityFrame4_WriteFrame(msg, maxsize, client->entitydatabase4, numsendstates, sv.writeentitiestoclient_sendstates);
		Protocol_WriteStatsReliable();
	}
	else if (client->entitydatabase)
	{
		success = EntityFrame_WriteFrame(msg, maxsize, client->entitydatabase, numsendstates, sv.writeentitiestoclient_sendstates, client - svs.clients + 1);
		Protocol_WriteStatsReliable();
	}
	else
	{
		success = EntityFrameQuake_WriteFrame(msg, maxsize, numsendstates, sv.writeentitiestoclient_sendstates);
		Protocol_WriteStatsReliable();
	}

	if(success)
		client->num_skippedentityframes = 0;
	else
		++client->num_skippedentityframes;
}
