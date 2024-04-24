#include "quakedef.h"
#include "protocol.h"

int EntityState_ReadExtendBits(void)
{
	unsigned int bits;
	bits = MSG_ReadByte(&cl_message);
	if (bits & 0x00000080)
	{
		bits |= MSG_ReadByte(&cl_message) << 8;
		if (bits & 0x00008000)
		{
			bits |= MSG_ReadByte(&cl_message) << 16;
			if (bits & 0x00800000)
				bits |= MSG_ReadByte(&cl_message) << 24;
		}
	}
	return bits;
}

void EntityState_ReadFields(entity_state_t *e, unsigned int bits)
{
	if (cls.protocol == PROTOCOL_DARKPLACES2)
	{
		if (bits & E_ORIGIN1)
			e->origin[0] = MSG_ReadCoord16i(&cl_message);
		if (bits & E_ORIGIN2)
			e->origin[1] = MSG_ReadCoord16i(&cl_message);
		if (bits & E_ORIGIN3)
			e->origin[2] = MSG_ReadCoord16i(&cl_message);
	}
	else
	{
		if (bits & E_FLAGS)
			e->flags = MSG_ReadByte(&cl_message);
		if (e->flags & RENDER_LOWPRECISION)
		{
			if (bits & E_ORIGIN1)
				e->origin[0] = MSG_ReadCoord16i(&cl_message);
			if (bits & E_ORIGIN2)
				e->origin[1] = MSG_ReadCoord16i(&cl_message);
			if (bits & E_ORIGIN3)
				e->origin[2] = MSG_ReadCoord16i(&cl_message);
		}
		else
		{
			if (bits & E_ORIGIN1)
				e->origin[0] = MSG_ReadCoord32f(&cl_message);
			if (bits & E_ORIGIN2)
				e->origin[1] = MSG_ReadCoord32f(&cl_message);
			if (bits & E_ORIGIN3)
				e->origin[2] = MSG_ReadCoord32f(&cl_message);
		}
	}
	if ((cls.protocol == PROTOCOL_DARKPLACES5 || cls.protocol == PROTOCOL_DARKPLACES6) && !(e->flags & RENDER_LOWPRECISION))
	{
		if (bits & E_ANGLE1)
			e->angles[0] = MSG_ReadAngle16i(&cl_message);
		if (bits & E_ANGLE2)
			e->angles[1] = MSG_ReadAngle16i(&cl_message);
		if (bits & E_ANGLE3)
			e->angles[2] = MSG_ReadAngle16i(&cl_message);
	}
	else
	{
		if (bits & E_ANGLE1)
			e->angles[0] = MSG_ReadAngle8i(&cl_message);
		if (bits & E_ANGLE2)
			e->angles[1] = MSG_ReadAngle8i(&cl_message);
		if (bits & E_ANGLE3)
			e->angles[2] = MSG_ReadAngle8i(&cl_message);
	}
	if (bits & E_MODEL1)
		e->modelindex = (e->modelindex & 0xFF00) | (unsigned int) MSG_ReadByte(&cl_message);
	if (bits & E_MODEL2)
		e->modelindex = (e->modelindex & 0x00FF) | ((unsigned int) MSG_ReadByte(&cl_message) << 8);
	if (bits & E_FRAME1)
		e->frame = (e->frame & 0xFF00) | (unsigned int) MSG_ReadByte(&cl_message);
	if (bits & E_FRAME2)
		e->frame = (e->frame & 0x00FF) | ((unsigned int) MSG_ReadByte(&cl_message) << 8);
	if (bits & E_EFFECTS1)
		e->effects = (e->effects & 0xFF00) | (unsigned int) MSG_ReadByte(&cl_message);
	if (bits & E_EFFECTS2)
		e->effects = (e->effects & 0x00FF) | ((unsigned int) MSG_ReadByte(&cl_message) << 8);
	if (bits & E_COLORMAP)
		e->colormap = MSG_ReadByte(&cl_message);
	if (bits & E_SKIN)
		e->skin = MSG_ReadByte(&cl_message);
	if (bits & E_ALPHA)
		e->alpha = MSG_ReadByte(&cl_message);
	if (bits & E_SCALE)
		e->scale = MSG_ReadByte(&cl_message);
	if (bits & E_GLOWSIZE)
		e->glowsize = MSG_ReadByte(&cl_message);
	if (bits & E_GLOWCOLOR)
		e->glowcolor = MSG_ReadByte(&cl_message);
	if (cls.protocol == PROTOCOL_DARKPLACES2)
		if (bits & E_FLAGS)
			e->flags = MSG_ReadByte(&cl_message);
	if (bits & E_TAGATTACHMENT)
	{
		e->tagentity = (unsigned short) MSG_ReadShort(&cl_message);
		e->tagindex = MSG_ReadByte(&cl_message);
	}
	if (bits & E_LIGHT)
	{
		e->light[0] = (unsigned short) MSG_ReadShort(&cl_message);
		e->light[1] = (unsigned short) MSG_ReadShort(&cl_message);
		e->light[2] = (unsigned short) MSG_ReadShort(&cl_message);
		e->light[3] = (unsigned short) MSG_ReadShort(&cl_message);
	}
	if (bits & E_LIGHTSTYLE)
		e->lightstyle = MSG_ReadByte(&cl_message);
	if (bits & E_LIGHTPFLAGS)
		e->lightpflags = MSG_ReadByte(&cl_message);

	if (developer_networkentities.integer >= 2)
	{
		Con_Printf("ReadFields e%i", e->number);

		if (bits & E_ORIGIN1)
			Con_Printf(" E_ORIGIN1 %f", e->origin[0]);
		if (bits & E_ORIGIN2)
			Con_Printf(" E_ORIGIN2 %f", e->origin[1]);
		if (bits & E_ORIGIN3)
			Con_Printf(" E_ORIGIN3 %f", e->origin[2]);
		if (bits & E_ANGLE1)
			Con_Printf(" E_ANGLE1 %f", e->angles[0]);
		if (bits & E_ANGLE2)
			Con_Printf(" E_ANGLE2 %f", e->angles[1]);
		if (bits & E_ANGLE3)
			Con_Printf(" E_ANGLE3 %f", e->angles[2]);
		if (bits & (E_MODEL1 | E_MODEL2))
			Con_Printf(" E_MODEL %i", e->modelindex);

		if (bits & (E_FRAME1 | E_FRAME2))
			Con_Printf(" E_FRAME %i", e->frame);
		if (bits & (E_EFFECTS1 | E_EFFECTS2))
			Con_Printf(" E_EFFECTS %i", e->effects);
		if (bits & E_ALPHA)
			Con_Printf(" E_ALPHA %f", e->alpha / 255.0f);
		if (bits & E_SCALE)
			Con_Printf(" E_SCALE %f", e->scale / 16.0f);
		if (bits & E_COLORMAP)
			Con_Printf(" E_COLORMAP %i", e->colormap);
		if (bits & E_SKIN)
			Con_Printf(" E_SKIN %i", e->skin);

		if (bits & E_GLOWSIZE)
			Con_Printf(" E_GLOWSIZE %i", e->glowsize * 4);
		if (bits & E_GLOWCOLOR)
			Con_Printf(" E_GLOWCOLOR %i", e->glowcolor);

		if (bits & E_LIGHT)
			Con_Printf(" E_LIGHT %i:%i:%i:%i", e->light[0], e->light[1], e->light[2], e->light[3]);
		if (bits & E_LIGHTPFLAGS)
			Con_Printf(" E_LIGHTPFLAGS %i", e->lightpflags);

		if (bits & E_TAGATTACHMENT)
			Con_Printf(" E_TAGATTACHMENT e%i:%i", e->tagentity, e->tagindex);
		if (bits & E_LIGHTSTYLE)
			Con_Printf(" E_LIGHTSTYLE %i", e->lightstyle);
		Con_Print("\n");
	}
}

// (client) adds a entity_frame to the database, for future reference
void EntityFrame_AddFrame_Client(entityframe_database_t *d, vec3_t eye, int framenum, int numentities, const entity_state_t *entitydata)
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

// (client) reads a frame from network stream
void EntityFrame_CL_ReadFrame(void)
{
	int i, number, removed;
	entity_frame_t *f, *delta;
	entity_state_t *e, *old, *oldend;
	entity_t *ent;
	entityframe_database_t *d;
	if (!cl.entitydatabase)
		cl.entitydatabase = EntityFrame_AllocDatabase(cls.levelmempool);
	d = cl.entitydatabase;
	f = &d->framedata;
	delta = &d->deltaframe;

	EntityFrame_Clear(f, NULL, -1);

	// read the frame header info
	f->time = cl.mtime[0];
	number = MSG_ReadLong(&cl_message);
	f->framenum = MSG_ReadLong(&cl_message);
	CL_NewFrameReceived(f->framenum);
	f->eye[0] = MSG_ReadFloat(&cl_message);
	f->eye[1] = MSG_ReadFloat(&cl_message);
	f->eye[2] = MSG_ReadFloat(&cl_message);
	EntityFrame_AckFrame(d, number);
	EntityFrame_FetchFrame(d, number, delta);
	old = delta->entitydata;
	oldend = old + delta->numentities;
	// read entities until we hit the magic 0xFFFF end tag
	while ((number = (unsigned short) MSG_ReadShort(&cl_message)) != 0xFFFF && !cl_message.badread)
	{
		if (cl_message.badread)
			Host_Error("EntityFrame_Read: read error");
		removed = number & 0x8000;
		number &= 0x7FFF;
		if (number >= MAX_EDICTS)
			Host_Error("EntityFrame_Read: number (%i) >= MAX_EDICTS (%i)", number, MAX_EDICTS);

		// seek to entity, while copying any skipped entities (assume unchanged)
		while (old < oldend && old->number < number)
		{
			if (f->numentities >= MAX_ENTITY_DATABASE)
				Host_Error("EntityFrame_Read: entity list too big");
			f->entitydata[f->numentities] = *old++;
			f->entitydata[f->numentities++].time = cl.mtime[0];
		}
		if (removed)
		{
			if (old < oldend && old->number == number)
				old++;
			else
				Con_Printf("EntityFrame_Read: REMOVE on unused entity %i\n", number);
		}
		else
		{
			if (f->numentities >= MAX_ENTITY_DATABASE)
				Host_Error("EntityFrame_Read: entity list too big");

			// reserve this slot
			e = f->entitydata + f->numentities++;

			if (old < oldend && old->number == number)
			{
				// delta from old entity
				*e = *old++;
			}
			else
			{
				// delta from defaults
				*e = defaultstate;
			}

			if (cl.num_entities <= number)
			{
				cl.num_entities = number + 1;
				if (number >= cl.max_entities)
					CL_ExpandEntities(number);
			}
			cl.entities_active[number] = true;
			e->active = ACTIVE_NETWORK;
			e->time = cl.mtime[0];
			e->number = number;
			EntityState_ReadFields(e, EntityState_ReadExtendBits());
		}
	}
	while (old < oldend)
	{
		if (f->numentities >= MAX_ENTITY_DATABASE)
			Host_Error("EntityFrame_Read: entity list too big");
		f->entitydata[f->numentities] = *old++;
		f->entitydata[f->numentities++].time = cl.mtime[0];
	}
	EntityFrame_AddFrame_Client(d, f->eye, f->framenum, f->numentities, f->entitydata);

	memset(cl.entities_active, 0, cl.num_entities * sizeof(unsigned char));
	number = 1;
	for (i = 0;i < f->numentities;i++)
	{
		for (;number < f->entitydata[i].number && number < cl.num_entities;number++)
		{
			if (cl.entities_active[number])
			{
				cl.entities_active[number] = false;
				cl.entities[number].state_current.active = ACTIVE_NOT;
			}
		}
		if (number >= cl.num_entities)
			break;
		// update the entity
		ent = &cl.entities[number];
		ent->state_previous = ent->state_current;
		ent->state_current = f->entitydata[i];
		CL_MoveLerpEntityStates(ent);
		// the entity lives again...
		cl.entities_active[number] = true;
		number++;
	}
	for (;number < cl.num_entities;number++)
	{
		if (cl.entities_active[number])
		{
			cl.entities_active[number] = false;
			cl.entities[number].state_current.active = ACTIVE_NOT;
		}
	}
}


// (client) returns the frame number of the most recent frame recieved
int EntityFrame_MostRecentlyRecievedFrameNum(entityframe_database_t *d)
{
	if (d->numframes)
		return d->frames[d->numframes - 1].framenum;
	else
		return -1;
}
