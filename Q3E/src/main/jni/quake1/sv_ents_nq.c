#include "quakedef.h"
#include "protocol.h"

qbool EntityFrameQuake_WriteFrame(sizebuf_t *msg, int maxsize, int numstates, const entity_state_t **states)
{
	prvm_prog_t *prog = SVVM_prog;
	const entity_state_t *s;
	entity_state_t baseline;
	int i, bits;
	sizebuf_t buf;
	unsigned char data[128];
	qbool success = false;

	// prepare the buffer
	memset(&buf, 0, sizeof(buf));
	buf.data = data;
	buf.maxsize = sizeof(data);

	for (i = 0;i < numstates;i++)
	{
		s = states[i];
		if(PRVM_serveredictfunction((&prog->edicts[s->number]), SendEntity))
			continue;

		// prepare the buffer
		SZ_Clear(&buf);

// send an update
		bits = 0;
		if (s->number >= 256)
			bits |= U_LONGENTITY;
		if (s->flags & RENDER_STEP)
			bits |= U_STEP;
		if (s->flags & RENDER_VIEWMODEL)
			bits |= U_VIEWMODEL;
		if (s->flags & RENDER_GLOWTRAIL)
			bits |= U_GLOWTRAIL;
		if (s->flags & RENDER_EXTERIORMODEL)
			bits |= U_EXTERIORMODEL;

		// LadyHavoc: old stuff, but rewritten to have more exact tolerances
		baseline = prog->edicts[s->number].priv.server->baseline;
		if (baseline.origin[0] != s->origin[0])
			bits |= U_ORIGIN1;
		if (baseline.origin[1] != s->origin[1])
			bits |= U_ORIGIN2;
		if (baseline.origin[2] != s->origin[2])
			bits |= U_ORIGIN3;
		if (baseline.angles[0] != s->angles[0])
			bits |= U_ANGLE1;
		if (baseline.angles[1] != s->angles[1])
			bits |= U_ANGLE2;
		if (baseline.angles[2] != s->angles[2])
			bits |= U_ANGLE3;
		if (baseline.colormap != s->colormap)
			bits |= U_COLORMAP;
		if (baseline.skin != s->skin)
			bits |= U_SKIN;
		if (baseline.frame != s->frame)
		{
			bits |= U_FRAME;
			if (s->frame & 0xFF00)
				bits |= U_FRAME2;
		}
		if (baseline.effects != s->effects)
		{
			bits |= U_EFFECTS;
			if (s->effects & 0xFF00)
				bits |= U_EFFECTS2;
		}
		if (baseline.modelindex != s->modelindex)
		{
			bits |= U_MODEL;
			if ((s->modelindex & 0xFF00) && sv.protocol != PROTOCOL_NEHAHRABJP && sv.protocol != PROTOCOL_NEHAHRABJP2 && sv.protocol != PROTOCOL_NEHAHRABJP3)
				bits |= U_MODEL2;
		}
		if (baseline.alpha != s->alpha)
			bits |= U_ALPHA;
		if (baseline.scale != s->scale)
			bits |= U_SCALE;
		if (baseline.glowsize != s->glowsize)
			bits |= U_GLOWSIZE;
		if (baseline.glowcolor != s->glowcolor)
			bits |= U_GLOWCOLOR;
		if (!VectorCompare(baseline.colormod, s->colormod))
			bits |= U_COLORMOD;

		// if extensions are disabled, clear the relevant update flags
		if (sv.protocol == PROTOCOL_QUAKE || sv.protocol == PROTOCOL_NEHAHRAMOVIE)
			bits &= 0x7FFF;
		if (sv.protocol == PROTOCOL_NEHAHRAMOVIE)
			if (s->alpha != 255 || s->effects & EF_FULLBRIGHT)
				bits |= U_EXTEND1;

		// write the message
		if (bits >= 16777216)
			bits |= U_EXTEND2;
		if (bits >= 65536)
			bits |= U_EXTEND1;
		if (bits >= 256)
			bits |= U_MOREBITS;
		bits |= U_SIGNAL;

		{
			ENTITYSIZEPROFILING_START(msg, states[i]->number, bits);

			MSG_WriteByte (&buf, bits);
			if (bits & U_MOREBITS)		MSG_WriteByte(&buf, bits>>8);
			if (sv.protocol != PROTOCOL_NEHAHRAMOVIE)
			{
				if (bits & U_EXTEND1)	MSG_WriteByte(&buf, bits>>16);
				if (bits & U_EXTEND2)	MSG_WriteByte(&buf, bits>>24);
			}
			if (bits & U_LONGENTITY)	MSG_WriteShort(&buf, s->number);
			else						MSG_WriteByte(&buf, s->number);

			if (bits & U_MODEL)
			{
				if (sv.protocol == PROTOCOL_NEHAHRABJP || sv.protocol == PROTOCOL_NEHAHRABJP2 || sv.protocol == PROTOCOL_NEHAHRABJP3)
					MSG_WriteShort(&buf, s->modelindex);
				else
					MSG_WriteByte(&buf, s->modelindex);
			}
			if (bits & U_FRAME)			MSG_WriteByte(&buf, s->frame);
			if (bits & U_COLORMAP)		MSG_WriteByte(&buf, s->colormap);
			if (bits & U_SKIN)			MSG_WriteByte(&buf, s->skin);
			if (bits & U_EFFECTS)		MSG_WriteByte(&buf, s->effects);
			if (bits & U_ORIGIN1)		MSG_WriteCoord(&buf, s->origin[0], sv.protocol);
			if (bits & U_ANGLE1)		MSG_WriteAngle(&buf, s->angles[0], sv.protocol);
			if (bits & U_ORIGIN2)		MSG_WriteCoord(&buf, s->origin[1], sv.protocol);
			if (bits & U_ANGLE2)		MSG_WriteAngle(&buf, s->angles[1], sv.protocol);
			if (bits & U_ORIGIN3)		MSG_WriteCoord(&buf, s->origin[2], sv.protocol);
			if (bits & U_ANGLE3)		MSG_WriteAngle(&buf, s->angles[2], sv.protocol);
			if (bits & U_ALPHA)			MSG_WriteByte(&buf, s->alpha);
			if (bits & U_SCALE)			MSG_WriteByte(&buf, s->scale);
			if (bits & U_EFFECTS2)		MSG_WriteByte(&buf, s->effects >> 8);
			if (bits & U_GLOWSIZE)		MSG_WriteByte(&buf, s->glowsize);
			if (bits & U_GLOWCOLOR)		MSG_WriteByte(&buf, s->glowcolor);
			if (bits & U_COLORMOD)		{int c = ((int)bound(0, s->colormod[0] * (7.0f / 32.0f), 7) << 5) | ((int)bound(0, s->colormod[1] * (7.0f / 32.0f), 7) << 2) | ((int)bound(0, s->colormod[2] * (3.0f / 32.0f), 3) << 0);MSG_WriteByte(&buf, c);}
			if (bits & U_FRAME2)		MSG_WriteByte(&buf, s->frame >> 8);
			if (bits & U_MODEL2)		MSG_WriteByte(&buf, s->modelindex >> 8);

			// the nasty protocol
			if ((bits & U_EXTEND1) && sv.protocol == PROTOCOL_NEHAHRAMOVIE)
			{
				if (s->effects & EF_FULLBRIGHT)
				{
					MSG_WriteFloat(&buf, 2); // QSG protocol version
					MSG_WriteFloat(&buf, s->alpha <= 0 ? 0 : (s->alpha >= 255 ? 1 : s->alpha * (1.0f / 255.0f))); // alpha
					MSG_WriteFloat(&buf, 1); // fullbright
				}
				else
				{
					MSG_WriteFloat(&buf, 1); // QSG protocol version
					MSG_WriteFloat(&buf, s->alpha <= 0 ? 0 : (s->alpha >= 255 ? 1 : s->alpha * (1.0f / 255.0f))); // alpha
				}
			}

			// if the commit is full, we're done this frame
			if (msg->cursize + buf.cursize > maxsize)
			{
				// next frame we will continue where we left off
				break;
			}
			// write the message to the packet
			SZ_Write(msg, buf.data, buf.cursize);
			success = true;
			ENTITYSIZEPROFILING_END(msg, s->number, bits);
		}
	}
	return success;
}
