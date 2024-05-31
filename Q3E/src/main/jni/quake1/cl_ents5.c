#include "quakedef.h"
#include "protocol.h"

static void EntityState5_ReadUpdate(entity_state_t *s, int number)
{
	int bits;
	int startoffset = cl_message.readcount;
	int bytes = 0;
	bits = MSG_ReadByte(&cl_message);
	if (bits & E5_EXTEND1)
	{
		bits |= MSG_ReadByte(&cl_message) << 8;
		if (bits & E5_EXTEND2)
		{
			bits |= MSG_ReadByte(&cl_message) << 16;
			if (bits & E5_EXTEND3)
				bits |= MSG_ReadByte(&cl_message) << 24;
		}
	}
	if (bits & E5_FULLUPDATE)
	{
		*s = defaultstate;
		s->active = ACTIVE_NETWORK;
	}
	if (bits & E5_FLAGS)
		s->flags = MSG_ReadByte(&cl_message);
	if (bits & E5_ORIGIN)
	{
		if (bits & E5_ORIGIN32)
		{
			s->origin[0] = MSG_ReadCoord32f(&cl_message);
			s->origin[1] = MSG_ReadCoord32f(&cl_message);
			s->origin[2] = MSG_ReadCoord32f(&cl_message);
		}
		else
		{
			s->origin[0] = MSG_ReadCoord13i(&cl_message);
			s->origin[1] = MSG_ReadCoord13i(&cl_message);
			s->origin[2] = MSG_ReadCoord13i(&cl_message);
		}
	}
	if (bits & E5_ANGLES)
	{
		if (bits & E5_ANGLES16)
		{
			s->angles[0] = MSG_ReadAngle16i(&cl_message);
			s->angles[1] = MSG_ReadAngle16i(&cl_message);
			s->angles[2] = MSG_ReadAngle16i(&cl_message);
		}
		else
		{
			s->angles[0] = MSG_ReadAngle8i(&cl_message);
			s->angles[1] = MSG_ReadAngle8i(&cl_message);
			s->angles[2] = MSG_ReadAngle8i(&cl_message);
		}
	}
	if (bits & E5_MODEL)
	{
		if (bits & E5_MODEL16)
			s->modelindex = (unsigned short) MSG_ReadShort(&cl_message);
		else
			s->modelindex = MSG_ReadByte(&cl_message);
	}
	if (bits & E5_FRAME)
	{
		if (bits & E5_FRAME16)
			s->frame = (unsigned short) MSG_ReadShort(&cl_message);
		else
			s->frame = MSG_ReadByte(&cl_message);
	}
	if (bits & E5_SKIN)
		s->skin = MSG_ReadByte(&cl_message);
	if (bits & E5_EFFECTS)
	{
		if (bits & E5_EFFECTS32)
			s->effects = (unsigned int) MSG_ReadLong(&cl_message);
		else if (bits & E5_EFFECTS16)
			s->effects = (unsigned short) MSG_ReadShort(&cl_message);
		else
			s->effects = MSG_ReadByte(&cl_message);
	}
	if (bits & E5_ALPHA)
		s->alpha = MSG_ReadByte(&cl_message);
	if (bits & E5_SCALE)
		s->scale = MSG_ReadByte(&cl_message);
	if (bits & E5_COLORMAP)
		s->colormap = MSG_ReadByte(&cl_message);
	if (bits & E5_ATTACHMENT)
	{
		s->tagentity = (unsigned short) MSG_ReadShort(&cl_message);
		s->tagindex = MSG_ReadByte(&cl_message);
	}
	if (bits & E5_LIGHT)
	{
		s->light[0] = (unsigned short) MSG_ReadShort(&cl_message);
		s->light[1] = (unsigned short) MSG_ReadShort(&cl_message);
		s->light[2] = (unsigned short) MSG_ReadShort(&cl_message);
		s->light[3] = (unsigned short) MSG_ReadShort(&cl_message);
		s->lightstyle = MSG_ReadByte(&cl_message);
		s->lightpflags = MSG_ReadByte(&cl_message);
	}
	if (bits & E5_GLOW)
	{
		s->glowsize = MSG_ReadByte(&cl_message);
		s->glowcolor = MSG_ReadByte(&cl_message);
	}
	if (bits & E5_COLORMOD)
	{
		s->colormod[0] = MSG_ReadByte(&cl_message);
		s->colormod[1] = MSG_ReadByte(&cl_message);
		s->colormod[2] = MSG_ReadByte(&cl_message);
	}
	if (bits & E5_GLOWMOD)
	{
		s->glowmod[0] = MSG_ReadByte(&cl_message);
		s->glowmod[1] = MSG_ReadByte(&cl_message);
		s->glowmod[2] = MSG_ReadByte(&cl_message);
	}
	if (bits & E5_COMPLEXANIMATION)
	{
		skeleton_t *skeleton;
		const model_t *model;
		int modelindex;
		int type;
		int bonenum;
		int numbones;
		short pose7s[7];
		type = MSG_ReadByte(&cl_message);
		switch(type)
		{
		case 0:
			s->framegroupblend[0].frame = MSG_ReadShort(&cl_message);
			s->framegroupblend[1].frame = 0;
			s->framegroupblend[2].frame = 0;
			s->framegroupblend[3].frame = 0;
			s->framegroupblend[0].start = cl.time - (unsigned short)MSG_ReadShort(&cl_message) * (1.0f / 1000.0f);
			s->framegroupblend[1].start = 0;
			s->framegroupblend[2].start = 0;
			s->framegroupblend[3].start = 0;
			s->framegroupblend[0].lerp = 1;
			s->framegroupblend[1].lerp = 0;
			s->framegroupblend[2].lerp = 0;
			s->framegroupblend[3].lerp = 0;
			break;
		case 1:
			s->framegroupblend[0].frame = MSG_ReadShort(&cl_message);
			s->framegroupblend[1].frame = MSG_ReadShort(&cl_message);
			s->framegroupblend[2].frame = 0;
			s->framegroupblend[3].frame = 0;
			s->framegroupblend[0].start = cl.time - (unsigned short)MSG_ReadShort(&cl_message) * (1.0f / 1000.0f);
			s->framegroupblend[1].start = cl.time - (unsigned short)MSG_ReadShort(&cl_message) * (1.0f / 1000.0f);
			s->framegroupblend[2].start = 0;
			s->framegroupblend[3].start = 0;
			s->framegroupblend[0].lerp = MSG_ReadByte(&cl_message) * (1.0f / 255.0f);
			s->framegroupblend[1].lerp = MSG_ReadByte(&cl_message) * (1.0f / 255.0f);
			s->framegroupblend[2].lerp = 0;
			s->framegroupblend[3].lerp = 0;
			break;
		case 2:
			s->framegroupblend[0].frame = MSG_ReadShort(&cl_message);
			s->framegroupblend[1].frame = MSG_ReadShort(&cl_message);
			s->framegroupblend[2].frame = MSG_ReadShort(&cl_message);
			s->framegroupblend[3].frame = 0;
			s->framegroupblend[0].start = cl.time - (unsigned short)MSG_ReadShort(&cl_message) * (1.0f / 1000.0f);
			s->framegroupblend[1].start = cl.time - (unsigned short)MSG_ReadShort(&cl_message) * (1.0f / 1000.0f);
			s->framegroupblend[2].start = cl.time - (unsigned short)MSG_ReadShort(&cl_message) * (1.0f / 1000.0f);
			s->framegroupblend[3].start = 0;
			s->framegroupblend[0].lerp = MSG_ReadByte(&cl_message) * (1.0f / 255.0f);
			s->framegroupblend[1].lerp = MSG_ReadByte(&cl_message) * (1.0f / 255.0f);
			s->framegroupblend[2].lerp = MSG_ReadByte(&cl_message) * (1.0f / 255.0f);
			s->framegroupblend[3].lerp = 0;
			break;
		case 3:
			s->framegroupblend[0].frame = MSG_ReadShort(&cl_message);
			s->framegroupblend[1].frame = MSG_ReadShort(&cl_message);
			s->framegroupblend[2].frame = MSG_ReadShort(&cl_message);
			s->framegroupblend[3].frame = MSG_ReadShort(&cl_message);
			s->framegroupblend[0].start = cl.time - (unsigned short)MSG_ReadShort(&cl_message) * (1.0f / 1000.0f);
			s->framegroupblend[1].start = cl.time - (unsigned short)MSG_ReadShort(&cl_message) * (1.0f / 1000.0f);
			s->framegroupblend[2].start = cl.time - (unsigned short)MSG_ReadShort(&cl_message) * (1.0f / 1000.0f);
			s->framegroupblend[3].start = cl.time - (unsigned short)MSG_ReadShort(&cl_message) * (1.0f / 1000.0f);
			s->framegroupblend[0].lerp = MSG_ReadByte(&cl_message) * (1.0f / 255.0f);
			s->framegroupblend[1].lerp = MSG_ReadByte(&cl_message) * (1.0f / 255.0f);
			s->framegroupblend[2].lerp = MSG_ReadByte(&cl_message) * (1.0f / 255.0f);
			s->framegroupblend[3].lerp = MSG_ReadByte(&cl_message) * (1.0f / 255.0f);
			break;
		case 4:
			if (!cl.engineskeletonobjects)
				cl.engineskeletonobjects = (skeleton_t *) Mem_Alloc(cls.levelmempool, sizeof(*cl.engineskeletonobjects) * MAX_EDICTS);
			skeleton = &cl.engineskeletonobjects[number];
			modelindex = MSG_ReadShort(&cl_message);
			model = CL_GetModelByIndex(modelindex);
			numbones = MSG_ReadByte(&cl_message);
			if (model && numbones != model->num_bones)
				Host_Error("E5_COMPLEXANIMATION: model has different number of bones than network packet describes\n");
			if (!skeleton->relativetransforms || skeleton->model != model)
			{
				skeleton->model = model;
				skeleton->relativetransforms = (matrix4x4_t *) Mem_Realloc(cls.levelmempool, skeleton->relativetransforms, sizeof(*skeleton->relativetransforms) * numbones);
				for (bonenum = 0;bonenum < numbones;bonenum++)
					skeleton->relativetransforms[bonenum] = identitymatrix;
			}
			for (bonenum = 0;bonenum < numbones;bonenum++)
			{
				pose7s[0] = (short)MSG_ReadShort(&cl_message);
				pose7s[1] = (short)MSG_ReadShort(&cl_message);
				pose7s[2] = (short)MSG_ReadShort(&cl_message);
				pose7s[3] = (short)MSG_ReadShort(&cl_message);
				pose7s[4] = (short)MSG_ReadShort(&cl_message);
				pose7s[5] = (short)MSG_ReadShort(&cl_message);
				pose7s[6] = (short)MSG_ReadShort(&cl_message);
				Matrix4x4_FromBonePose7s(skeleton->relativetransforms + bonenum, 1.0f / 64.0f, pose7s);
			}
			s->skeletonobject = *skeleton;
			break;
		default:
			Host_Error("E5_COMPLEXANIMATION: Parse error - unknown type %i\n", type);
			break;
		}
	}
	if (bits & E5_TRAILEFFECTNUM)
		s->traileffectnum = (unsigned short) MSG_ReadShort(&cl_message);


	bytes = cl_message.readcount - startoffset;
	if (developer_networkentities.integer >= 2)
	{
		Con_Printf("ReadFields e%i (%i bytes)", number, bytes);

		if (bits & E5_ORIGIN)
			Con_Printf(" E5_ORIGIN %f %f %f", s->origin[0], s->origin[1], s->origin[2]);
		if (bits & E5_ANGLES)
			Con_Printf(" E5_ANGLES %f %f %f", s->angles[0], s->angles[1], s->angles[2]);
		if (bits & E5_MODEL)
			Con_Printf(" E5_MODEL %i", s->modelindex);
		if (bits & E5_FRAME)
			Con_Printf(" E5_FRAME %i", s->frame);
		if (bits & E5_SKIN)
			Con_Printf(" E5_SKIN %i", s->skin);
		if (bits & E5_EFFECTS)
			Con_Printf(" E5_EFFECTS %i", s->effects);
		if (bits & E5_FLAGS)
		{
			Con_Printf(" E5_FLAGS %i (", s->flags);
			if (s->flags & RENDER_STEP)
				Con_Print(" STEP");
			if (s->flags & RENDER_GLOWTRAIL)
				Con_Print(" GLOWTRAIL");
			if (s->flags & RENDER_VIEWMODEL)
				Con_Print(" VIEWMODEL");
			if (s->flags & RENDER_EXTERIORMODEL)
				Con_Print(" EXTERIORMODEL");
			if (s->flags & RENDER_LOWPRECISION)
				Con_Print(" LOWPRECISION");
			if (s->flags & RENDER_COLORMAPPED)
				Con_Print(" COLORMAPPED");
			if (s->flags & RENDER_SHADOW)
				Con_Print(" SHADOW");
			if (s->flags & RENDER_LIGHT)
				Con_Print(" LIGHT");
			if (s->flags & RENDER_NOSELFSHADOW)
				Con_Print(" NOSELFSHADOW");
			Con_Print(")");
		}
		if (bits & E5_ALPHA)
			Con_Printf(" E5_ALPHA %f", s->alpha / 255.0f);
		if (bits & E5_SCALE)
			Con_Printf(" E5_SCALE %f", s->scale / 16.0f);
		if (bits & E5_COLORMAP)
			Con_Printf(" E5_COLORMAP %i", s->colormap);
		if (bits & E5_ATTACHMENT)
			Con_Printf(" E5_ATTACHMENT e%i:%i", s->tagentity, s->tagindex);
		if (bits & E5_LIGHT)
			Con_Printf(" E5_LIGHT %i:%i:%i:%i %i:%i", s->light[0], s->light[1], s->light[2], s->light[3], s->lightstyle, s->lightpflags);
		if (bits & E5_GLOW)
			Con_Printf(" E5_GLOW %i:%i", s->glowsize * 4, s->glowcolor);
		if (bits & E5_COLORMOD)
			Con_Printf(" E5_COLORMOD %f:%f:%f", s->colormod[0] / 32.0f, s->colormod[1] / 32.0f, s->colormod[2] / 32.0f);
		if (bits & E5_GLOWMOD)
			Con_Printf(" E5_GLOWMOD %f:%f:%f", s->glowmod[0] / 32.0f, s->glowmod[1] / 32.0f, s->glowmod[2] / 32.0f);
		if (bits & E5_COMPLEXANIMATION)
			Con_Printf(" E5_COMPLEXANIMATION");
		if (bits & E5_TRAILEFFECTNUM)
			Con_Printf(" E5_TRAILEFFECTNUM %i", s->traileffectnum);
		Con_Print("\n");
	}
}

void EntityFrame5_CL_ReadFrame(void)
{
	int n, enumber, framenum;
	entity_t *ent;
	entity_state_t *s;
	// read the number of this frame to echo back in next input packet
	framenum = MSG_ReadLong(&cl_message);
	CL_NewFrameReceived(framenum);
	if (cls.protocol != PROTOCOL_QUAKE && cls.protocol != PROTOCOL_QUAKEDP && cls.protocol != PROTOCOL_NEHAHRAMOVIE && cls.protocol != PROTOCOL_DARKPLACES1 && cls.protocol != PROTOCOL_DARKPLACES2 && cls.protocol != PROTOCOL_DARKPLACES3 && cls.protocol != PROTOCOL_DARKPLACES4 && cls.protocol != PROTOCOL_DARKPLACES5 && cls.protocol != PROTOCOL_DARKPLACES6)
		cls.servermovesequence = MSG_ReadLong(&cl_message);
	// read entity numbers until we find a 0x8000
	// (which would be remove world entity, but is actually a terminator)
	while ((n = (unsigned short)MSG_ReadShort(&cl_message)) != 0x8000 && !cl_message.badread)
	{
		// get the entity number
		enumber = n & 0x7FFF;
		// we may need to expand the array
		if (cl.num_entities <= enumber)
		{
			cl.num_entities = enumber + 1;
			if (enumber >= cl.max_entities)
				CL_ExpandEntities(enumber);
		}
		// look up the entity
		ent = cl.entities + enumber;
		// slide the current into the previous slot
		ent->state_previous = ent->state_current;
		// read the update
		s = &ent->state_current;
		if (n & 0x8000)
		{
			// remove entity
			*s = defaultstate;
		}
		else
		{
			// update entity
			EntityState5_ReadUpdate(s, enumber);
		}
		// set the cl.entities_active flag
		cl.entities_active[enumber] = (s->active == ACTIVE_NETWORK);
		// set the update time
		s->time = cl.mtime[0];
		// fix the number (it gets wiped occasionally by copying from defaultstate)
		s->number = enumber;
		// check if we need to update the lerp stuff
		if (s->active == ACTIVE_NETWORK)
			CL_MoveLerpEntityStates(&cl.entities[enumber]);
		// print extra messages if desired
		if (developer_networkentities.integer >= 2 && cl.entities[enumber].state_current.active != cl.entities[enumber].state_previous.active)
		{
			if (cl.entities[enumber].state_current.active == ACTIVE_NETWORK)
				Con_Printf("entity #%i has become active\n", enumber);
			else if (cl.entities[enumber].state_previous.active)
				Con_Printf("entity #%i has become inactive\n", enumber);
		}
	}
}
