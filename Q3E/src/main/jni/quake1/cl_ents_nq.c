#include "quakedef.h"
#include "protocol.h"

void EntityFrameQuake_ReadEntity(int bits)
{
	int num;
	entity_t *ent;
	entity_state_t s;

	if (bits & U_MOREBITS)
		bits |= (MSG_ReadByte(&cl_message)<<8);
	if ((bits & U_EXTEND1) && cls.protocol != PROTOCOL_NEHAHRAMOVIE)
	{
		bits |= MSG_ReadByte(&cl_message) << 16;
		if (bits & U_EXTEND2)
			bits |= MSG_ReadByte(&cl_message) << 24;
	}

	if (bits & U_LONGENTITY)
		num = (unsigned short) MSG_ReadShort(&cl_message);
	else
		num = MSG_ReadByte(&cl_message);

	if (num >= MAX_EDICTS)
		Host_Error("EntityFrameQuake_ReadEntity: entity number (%i) >= MAX_EDICTS (%i)", num, MAX_EDICTS);
	if (num < 1)
		Host_Error("EntityFrameQuake_ReadEntity: invalid entity number (%i)", num);

	if (cl.num_entities <= num)
	{
		cl.num_entities = num + 1;
		if (num >= cl.max_entities)
			CL_ExpandEntities(num);
	}

	ent = cl.entities + num;

	// note: this inherits the 'active' state of the baseline chosen
	// (state_baseline is always active, state_current may not be active if
	// the entity was missing in the last frame)
	if (bits & U_DELTA)
		s = ent->state_current;
	else
	{
		s = ent->state_baseline;
		s.active = ACTIVE_NETWORK;
	}

	cl.isquakeentity[num] = true;
	if (cl.lastquakeentity < num)
		cl.lastquakeentity = num;
	s.number = num;
	s.time = cl.mtime[0];
	s.flags = 0;
	if (bits & U_MODEL)
	{
		if (cls.protocol == PROTOCOL_NEHAHRABJP || cls.protocol == PROTOCOL_NEHAHRABJP2 || cls.protocol == PROTOCOL_NEHAHRABJP3)
							s.modelindex = (unsigned short) MSG_ReadShort(&cl_message);
		else
							s.modelindex = (s.modelindex & 0xFF00) | MSG_ReadByte(&cl_message);
	}
	if (bits & U_FRAME)		s.frame = (s.frame & 0xFF00) | MSG_ReadByte(&cl_message);
	if (bits & U_COLORMAP)	s.colormap = MSG_ReadByte(&cl_message);
	if (bits & U_SKIN)		s.skin = MSG_ReadByte(&cl_message);
	if (bits & U_EFFECTS)	s.effects = (s.effects & 0xFF00) | MSG_ReadByte(&cl_message);
	if (bits & U_ORIGIN1)	s.origin[0] = MSG_ReadCoord(&cl_message, cls.protocol);
	if (bits & U_ANGLE1)	s.angles[0] = MSG_ReadAngle(&cl_message, cls.protocol);
	if (bits & U_ORIGIN2)	s.origin[1] = MSG_ReadCoord(&cl_message, cls.protocol);
	if (bits & U_ANGLE2)	s.angles[1] = MSG_ReadAngle(&cl_message, cls.protocol);
	if (bits & U_ORIGIN3)	s.origin[2] = MSG_ReadCoord(&cl_message, cls.protocol);
	if (bits & U_ANGLE3)	s.angles[2] = MSG_ReadAngle(&cl_message, cls.protocol);
	if (bits & U_STEP)		s.flags |= RENDER_STEP;
	if (bits & U_ALPHA)		s.alpha = MSG_ReadByte(&cl_message);
	if (bits & U_SCALE)		s.scale = MSG_ReadByte(&cl_message);
	if (bits & U_EFFECTS2)	s.effects = (s.effects & 0x00FF) | (MSG_ReadByte(&cl_message) << 8);
	if (bits & U_GLOWSIZE)	s.glowsize = MSG_ReadByte(&cl_message);
	if (bits & U_GLOWCOLOR)	s.glowcolor = MSG_ReadByte(&cl_message);
	if (bits & U_COLORMOD)	{int c = MSG_ReadByte(&cl_message);s.colormod[0] = (unsigned char)(((c >> 5) & 7) * (32.0f / 7.0f));s.colormod[1] = (unsigned char)(((c >> 2) & 7) * (32.0f / 7.0f));s.colormod[2] = (unsigned char)((c & 3) * (32.0f / 3.0f));}
	if (bits & U_GLOWTRAIL) s.flags |= RENDER_GLOWTRAIL;
	if (bits & U_FRAME2)	s.frame = (s.frame & 0x00FF) | (MSG_ReadByte(&cl_message) << 8);
	if (bits & U_MODEL2)	s.modelindex = (s.modelindex & 0x00FF) | (MSG_ReadByte(&cl_message) << 8);
	if (bits & U_VIEWMODEL)	s.flags |= RENDER_VIEWMODEL;
	if (bits & U_EXTERIORMODEL)	s.flags |= RENDER_EXTERIORMODEL;

	// LadyHavoc: to allow playback of the Nehahra movie
	if (cls.protocol == PROTOCOL_NEHAHRAMOVIE && (bits & U_EXTEND1))
	{
		// LadyHavoc: evil format
		int i = (int)MSG_ReadFloat(&cl_message);
		int j = (int)(MSG_ReadFloat(&cl_message) * 255.0f);
		if (i == 2)
		{
			i = (int)MSG_ReadFloat(&cl_message);
			if (i)
				s.effects |= EF_FULLBRIGHT;
		}
		if (j < 0)
			s.alpha = 0;
		else if (j == 0 || j >= 255)
			s.alpha = 255;
		else
			s.alpha = j;
	}

	ent->state_previous = ent->state_current;
	ent->state_current = s;
	if (ent->state_current.active == ACTIVE_NETWORK)
	{
		CL_MoveLerpEntityStates(ent);
		cl.entities_active[ent->state_current.number] = true;
	}

	if (cl_message.badread)
		Host_Error("EntityFrameQuake_ReadEntity: read error");
}

void EntityFrameQuake_ISeeDeadEntities(void)
{
	int num, lastentity;
	if (cl.lastquakeentity == 0)
		return;
	lastentity = cl.lastquakeentity;
	cl.lastquakeentity = 0;
	for (num = 0;num <= lastentity;num++)
	{
		if (cl.isquakeentity[num])
		{
			if (cl.entities_active[num] && cl.entities[num].state_current.time == cl.mtime[0])
			{
				cl.isquakeentity[num] = true;
				cl.lastquakeentity = num;
			}
			else
			{
				cl.isquakeentity[num] = false;
				cl.entities_active[num] = ACTIVE_NOT;
				cl.entities[num].state_current = defaultstate;
				cl.entities[num].state_current.number = num;
			}
		}
	}
}