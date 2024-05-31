#include "quakedef.h"
#include "protocol.h"

static double anim_reducetime(double t, double frameduration, double maxtime)
{
	if(t < 0) // clamp to non-negative
		return 0;
	if(t <= maxtime) // time can be represented normally
		return t;
	if(frameduration == 0) // don't like dividing by zero
		return t;
	if(maxtime <= 2 * frameduration) // if two frames don't fit, we better not do this
		return t;
	t -= frameduration * ceil((t - maxtime) / frameduration);
	// now maxtime - frameduration < t <= maxtime
	return t;
}

// see VM_SV_frameduration
static double anim_frameduration(model_t *model, int framenum)
{
	if (!model || !model->animscenes || framenum < 0 || framenum >= model->numframes)
		return 0;
	if(model->animscenes[framenum].framerate)
		return model->animscenes[framenum].framecount / model->animscenes[framenum].framerate;
	return 0;
}

void EntityFrame5_FreeDatabase(entityframe5_database_t *d)
{
	// all the [maxedicts] memory is allocated at once, so there's only one
	// thing to free
	if (d->maxedicts)
		Mem_Free(d->deltabits);
	Mem_Free(d);
}

static void EntityFrame5_ExpandEdicts(entityframe5_database_t *d, int newmax)
{
	if (d->maxedicts < newmax)
	{
		unsigned char *data;
		int oldmaxedicts = d->maxedicts;
		int *olddeltabits = d->deltabits;
		unsigned char *oldpriorities = d->priorities;
		int *oldupdateframenum = d->updateframenum;
		entity_state_t *oldstates = d->states;
		unsigned char *oldvisiblebits = d->visiblebits;
		d->maxedicts = newmax;
		data = (unsigned char *)Mem_Alloc(sv_mempool, d->maxedicts * sizeof(int) + d->maxedicts * sizeof(unsigned char) + d->maxedicts * sizeof(int) + d->maxedicts * sizeof(entity_state_t) + (d->maxedicts+7)/8 * sizeof(unsigned char));
		d->deltabits = (int *)data;data += d->maxedicts * sizeof(int);
		d->priorities = (unsigned char *)data;data += d->maxedicts * sizeof(unsigned char);
		d->updateframenum = (int *)data;data += d->maxedicts * sizeof(int);
		d->states = (entity_state_t *)data;data += d->maxedicts * sizeof(entity_state_t);
		d->visiblebits = (unsigned char *)data;data += (d->maxedicts+7)/8 * sizeof(unsigned char);
		if (oldmaxedicts)
		{
			memcpy(d->deltabits, olddeltabits, oldmaxedicts * sizeof(int));
			memcpy(d->priorities, oldpriorities, oldmaxedicts * sizeof(unsigned char));
			memcpy(d->updateframenum, oldupdateframenum, oldmaxedicts * sizeof(int));
			memcpy(d->states, oldstates, oldmaxedicts * sizeof(entity_state_t));
			memcpy(d->visiblebits, oldvisiblebits, (oldmaxedicts+7)/8 * sizeof(unsigned char));
			// the previous buffers were a single allocation, so just one free
			Mem_Free(olddeltabits);
		}
	}
}

static int EntityState5_Priority(entityframe5_database_t *d, int stateindex)
{
	int limit, priority;
	entity_state_t *s = NULL; // hush compiler warning by initializing this
	// if it is the player, update urgently
	if (stateindex == d->viewentnum)
		return ENTITYFRAME5_PRIORITYLEVELS - 1;
	// priority increases each frame no matter what happens
	priority = d->priorities[stateindex] + 1;
	// players get an extra priority boost
	if (stateindex <= svs.maxclients)
		priority++;
	// remove dead entities very quickly because they are just 2 bytes
	if (d->states[stateindex].active != ACTIVE_NETWORK)
	{
		priority++;
		return bound(1, priority, ENTITYFRAME5_PRIORITYLEVELS - 1);
	}
	// certain changes are more noticable than others
	if (d->deltabits[stateindex] & (E5_FULLUPDATE | E5_ATTACHMENT | E5_MODEL | E5_FLAGS | E5_COLORMAP))
		priority++;
	// find the root entity this one is attached to, and judge relevance by it
	for (limit = 0;limit < 256;limit++)
	{
		s = d->states + stateindex;
		if (s->flags & RENDER_VIEWMODEL)
			stateindex = d->viewentnum;
		else if (s->tagentity)
			stateindex = s->tagentity;
		else
			break;
		if (d->maxedicts < stateindex)
			EntityFrame5_ExpandEdicts(d, (stateindex+256)&~255);
	}
	if (limit >= 256)
		Con_DPrintf("Protocol: Runaway loop recursing tagentity links on entity %i\n", stateindex);
	// now that we have the parent entity we can make some decisions based on
	// distance from the player
	if (VectorDistance(d->states[d->viewentnum].netcenter, s->netcenter) < 1024.0f)
		priority++;
	return bound(1, priority, ENTITYFRAME5_PRIORITYLEVELS - 1);
}

static int EntityState5_DeltaBits(const entity_state_t *o, const entity_state_t *n)
{
	unsigned int bits = 0;
	if (n->active == ACTIVE_NETWORK)
	{
		if (o->active != ACTIVE_NETWORK)
			bits |= E5_FULLUPDATE;
		if (!VectorCompare(o->origin, n->origin))
			bits |= E5_ORIGIN;
		if (!VectorCompare(o->angles, n->angles))
			bits |= E5_ANGLES;
		if (o->modelindex != n->modelindex)
			bits |= E5_MODEL;
		if (o->frame != n->frame)
			bits |= E5_FRAME;
		if (o->skin != n->skin)
			bits |= E5_SKIN;
		if (o->effects != n->effects)
			bits |= E5_EFFECTS;
		if (o->flags != n->flags)
			bits |= E5_FLAGS;
		if (o->alpha != n->alpha)
			bits |= E5_ALPHA;
		if (o->scale != n->scale)
			bits |= E5_SCALE;
		if (o->colormap != n->colormap)
			bits |= E5_COLORMAP;
		if (o->tagentity != n->tagentity || o->tagindex != n->tagindex)
			bits |= E5_ATTACHMENT;
		if (o->light[0] != n->light[0] || o->light[1] != n->light[1] || o->light[2] != n->light[2] || o->light[3] != n->light[3] || o->lightstyle != n->lightstyle || o->lightpflags != n->lightpflags)
			bits |= E5_LIGHT;
		if (o->glowsize != n->glowsize || o->glowcolor != n->glowcolor)
			bits |= E5_GLOW;
		if (o->colormod[0] != n->colormod[0] || o->colormod[1] != n->colormod[1] || o->colormod[2] != n->colormod[2])
			bits |= E5_COLORMOD;
		if (o->glowmod[0] != n->glowmod[0] || o->glowmod[1] != n->glowmod[1] || o->glowmod[2] != n->glowmod[2])
			bits |= E5_GLOWMOD;
		if (n->flags & RENDER_COMPLEXANIMATION)
		{
			if ((o->skeletonobject.model && o->skeletonobject.relativetransforms) != (n->skeletonobject.model && n->skeletonobject.relativetransforms))
			{
				bits |= E5_COMPLEXANIMATION;
			}
			else if (o->skeletonobject.model && o->skeletonobject.relativetransforms)
			{
				if(o->modelindex != n->modelindex)
					bits |= E5_COMPLEXANIMATION;
				else if(o->skeletonobject.model->num_bones != n->skeletonobject.model->num_bones)
					bits |= E5_COMPLEXANIMATION;
				else if(memcmp(o->skeletonobject.relativetransforms, n->skeletonobject.relativetransforms, o->skeletonobject.model->num_bones * sizeof(*o->skeletonobject.relativetransforms)))
					bits |= E5_COMPLEXANIMATION;
			}
			else if (memcmp(o->framegroupblend, n->framegroupblend, sizeof(o->framegroupblend)))
			{
				bits |= E5_COMPLEXANIMATION;
			}
		}
		if (o->traileffectnum != n->traileffectnum)
			bits |= E5_TRAILEFFECTNUM;
	}
	else
		if (o->active == ACTIVE_NETWORK)
			bits |= E5_FULLUPDATE;
	return bits;
}

void EntityState5_WriteUpdate(int number, const entity_state_t *s, int changedbits, sizebuf_t *msg)
{
	prvm_prog_t *prog = SVVM_prog;
	unsigned int bits = 0;
	//model_t *model;

	if (s->active != ACTIVE_NETWORK)
	{
		ENTITYSIZEPROFILING_START(msg, s->number, 0);
		MSG_WriteShort(msg, number | 0x8000);
		ENTITYSIZEPROFILING_END(msg, s->number, 0);
	}
	else
	{
		if (PRVM_serveredictfunction((&prog->edicts[s->number]), SendEntity))
			return;

		bits = changedbits;
		if ((bits & E5_ORIGIN) && (!(s->flags & RENDER_LOWPRECISION) || s->exteriormodelforclient || s->tagentity || s->viewmodelforclient || (s->number >= 1 && s->number <= svs.maxclients) || s->origin[0] <= -4096.0625 || s->origin[0] >= 4095.9375 || s->origin[1] <= -4096.0625 || s->origin[1] >= 4095.9375 || s->origin[2] <= -4096.0625 || s->origin[2] >= 4095.9375))
		// maybe also add: ((model = SV_GetModelByIndex(s->modelindex)) != NULL && model->name[0] == '*')
			bits |= E5_ORIGIN32;
			// possible values:
			//   negative origin:
			//     (int)(f * 8 - 0.5) >= -32768
			//          (f * 8 - 0.5) >  -32769
			//           f            >  -4096.0625
			//   positive origin:
			//     (int)(f * 8 + 0.5) <=  32767
			//          (f * 8 + 0.5) <   32768
			//           f * 8 + 0.5) <   4095.9375
		if ((bits & E5_ANGLES) && !(s->flags & RENDER_LOWPRECISION))
			bits |= E5_ANGLES16;
		if ((bits & E5_MODEL) && s->modelindex >= 256)
			bits |= E5_MODEL16;
		if ((bits & E5_FRAME) && s->frame >= 256)
			bits |= E5_FRAME16;
		if (bits & E5_EFFECTS)
		{
			if (s->effects & 0xFFFF0000)
				bits |= E5_EFFECTS32;
			else if (s->effects & 0xFFFFFF00)
				bits |= E5_EFFECTS16;
		}
		if (bits >= 256)
			bits |= E5_EXTEND1;
		if (bits >= 65536)
			bits |= E5_EXTEND2;
		if (bits >= 16777216)
			bits |= E5_EXTEND3;
		{
			ENTITYSIZEPROFILING_START(msg, s->number, bits);
			MSG_WriteShort(msg, number);
			MSG_WriteByte(msg, bits & 0xFF);
			if (bits & E5_EXTEND1)
				MSG_WriteByte(msg, (bits >> 8) & 0xFF);
			if (bits & E5_EXTEND2)
				MSG_WriteByte(msg, (bits >> 16) & 0xFF);
			if (bits & E5_EXTEND3)
				MSG_WriteByte(msg, (bits >> 24) & 0xFF);
			if (bits & E5_FLAGS)
				MSG_WriteByte(msg, s->flags);
			if (bits & E5_ORIGIN)
			{
				if (bits & E5_ORIGIN32)
				{
					MSG_WriteCoord32f(msg, s->origin[0]);
					MSG_WriteCoord32f(msg, s->origin[1]);
					MSG_WriteCoord32f(msg, s->origin[2]);
				}
				else
				{
					MSG_WriteCoord13i(msg, s->origin[0]);
					MSG_WriteCoord13i(msg, s->origin[1]);
					MSG_WriteCoord13i(msg, s->origin[2]);
				}
			}
			if (bits & E5_ANGLES)
			{
				if (bits & E5_ANGLES16)
				{
					MSG_WriteAngle16i(msg, s->angles[0]);
					MSG_WriteAngle16i(msg, s->angles[1]);
					MSG_WriteAngle16i(msg, s->angles[2]);
				}
				else
				{
					MSG_WriteAngle8i(msg, s->angles[0]);
					MSG_WriteAngle8i(msg, s->angles[1]);
					MSG_WriteAngle8i(msg, s->angles[2]);
				}
			}
			if (bits & E5_MODEL)
			{
				if (bits & E5_MODEL16)
					MSG_WriteShort(msg, s->modelindex);
				else
					MSG_WriteByte(msg, s->modelindex);
			}
			if (bits & E5_FRAME)
			{
				if (bits & E5_FRAME16)
					MSG_WriteShort(msg, s->frame);
				else
					MSG_WriteByte(msg, s->frame);
			}
			if (bits & E5_SKIN)
				MSG_WriteByte(msg, s->skin);
			if (bits & E5_EFFECTS)
			{
				if (bits & E5_EFFECTS32)
					MSG_WriteLong(msg, s->effects);
				else if (bits & E5_EFFECTS16)
					MSG_WriteShort(msg, s->effects);
				else
					MSG_WriteByte(msg, s->effects);
			}
			if (bits & E5_ALPHA)
				MSG_WriteByte(msg, s->alpha);
			if (bits & E5_SCALE)
				MSG_WriteByte(msg, s->scale);
			if (bits & E5_COLORMAP)
				MSG_WriteByte(msg, s->colormap);
			if (bits & E5_ATTACHMENT)
			{
				MSG_WriteShort(msg, s->tagentity);
				MSG_WriteByte(msg, s->tagindex);
			}
			if (bits & E5_LIGHT)
			{
				MSG_WriteShort(msg, s->light[0]);
				MSG_WriteShort(msg, s->light[1]);
				MSG_WriteShort(msg, s->light[2]);
				MSG_WriteShort(msg, s->light[3]);
				MSG_WriteByte(msg, s->lightstyle);
				MSG_WriteByte(msg, s->lightpflags);
			}
			if (bits & E5_GLOW)
			{
				MSG_WriteByte(msg, s->glowsize);
				MSG_WriteByte(msg, s->glowcolor);
			}
			if (bits & E5_COLORMOD)
			{
				MSG_WriteByte(msg, s->colormod[0]);
				MSG_WriteByte(msg, s->colormod[1]);
				MSG_WriteByte(msg, s->colormod[2]);
			}
			if (bits & E5_GLOWMOD)
			{
				MSG_WriteByte(msg, s->glowmod[0]);
				MSG_WriteByte(msg, s->glowmod[1]);
				MSG_WriteByte(msg, s->glowmod[2]);
			}
			if (bits & E5_COMPLEXANIMATION)
			{
				if (s->skeletonobject.model && s->skeletonobject.relativetransforms)
				{
					int numbones = s->skeletonobject.model->num_bones;
					int bonenum;
					short pose7s[7];
					MSG_WriteByte(msg, 4);
					MSG_WriteShort(msg, s->modelindex);
					MSG_WriteByte(msg, numbones);
					for (bonenum = 0;bonenum < numbones;bonenum++)
					{
						Matrix4x4_ToBonePose7s(s->skeletonobject.relativetransforms + bonenum, 64, pose7s);
						MSG_WriteShort(msg, pose7s[0]);
						MSG_WriteShort(msg, pose7s[1]);
						MSG_WriteShort(msg, pose7s[2]);
						MSG_WriteShort(msg, pose7s[3]);
						MSG_WriteShort(msg, pose7s[4]);
						MSG_WriteShort(msg, pose7s[5]);
						MSG_WriteShort(msg, pose7s[6]);
					}
				}
				else
				{
					model_t *model = SV_GetModelByIndex(s->modelindex);
					if (s->framegroupblend[3].lerp > 0)
					{
						MSG_WriteByte(msg, 3);
						MSG_WriteShort(msg, s->framegroupblend[0].frame);
						MSG_WriteShort(msg, s->framegroupblend[1].frame);
						MSG_WriteShort(msg, s->framegroupblend[2].frame);
						MSG_WriteShort(msg, s->framegroupblend[3].frame);
						MSG_WriteShort(msg, (int)(anim_reducetime(sv.time - s->framegroupblend[0].start, anim_frameduration(model, s->framegroupblend[0].frame), 65.535) * 1000.0));
						MSG_WriteShort(msg, (int)(anim_reducetime(sv.time - s->framegroupblend[1].start, anim_frameduration(model, s->framegroupblend[1].frame), 65.535) * 1000.0));
						MSG_WriteShort(msg, (int)(anim_reducetime(sv.time - s->framegroupblend[2].start, anim_frameduration(model, s->framegroupblend[2].frame), 65.535) * 1000.0));
						MSG_WriteShort(msg, (int)(anim_reducetime(sv.time - s->framegroupblend[3].start, anim_frameduration(model, s->framegroupblend[3].frame), 65.535) * 1000.0));
						MSG_WriteByte(msg, s->framegroupblend[0].lerp * 255.0f);
						MSG_WriteByte(msg, s->framegroupblend[1].lerp * 255.0f);
						MSG_WriteByte(msg, s->framegroupblend[2].lerp * 255.0f);
						MSG_WriteByte(msg, s->framegroupblend[3].lerp * 255.0f);
					}
					else if (s->framegroupblend[2].lerp > 0)
					{
						MSG_WriteByte(msg, 2);
						MSG_WriteShort(msg, s->framegroupblend[0].frame);
						MSG_WriteShort(msg, s->framegroupblend[1].frame);
						MSG_WriteShort(msg, s->framegroupblend[2].frame);
						MSG_WriteShort(msg, (int)(anim_reducetime(sv.time - s->framegroupblend[0].start, anim_frameduration(model, s->framegroupblend[0].frame), 65.535) * 1000.0));
						MSG_WriteShort(msg, (int)(anim_reducetime(sv.time - s->framegroupblend[1].start, anim_frameduration(model, s->framegroupblend[1].frame), 65.535) * 1000.0));
						MSG_WriteShort(msg, (int)(anim_reducetime(sv.time - s->framegroupblend[2].start, anim_frameduration(model, s->framegroupblend[2].frame), 65.535) * 1000.0));
						MSG_WriteByte(msg, s->framegroupblend[0].lerp * 255.0f);
						MSG_WriteByte(msg, s->framegroupblend[1].lerp * 255.0f);
						MSG_WriteByte(msg, s->framegroupblend[2].lerp * 255.0f);
					}
					else if (s->framegroupblend[1].lerp > 0)
					{
						MSG_WriteByte(msg, 1);
						MSG_WriteShort(msg, s->framegroupblend[0].frame);
						MSG_WriteShort(msg, s->framegroupblend[1].frame);
						MSG_WriteShort(msg, (int)(anim_reducetime(sv.time - s->framegroupblend[0].start, anim_frameduration(model, s->framegroupblend[0].frame), 65.535) * 1000.0));
						MSG_WriteShort(msg, (int)(anim_reducetime(sv.time - s->framegroupblend[1].start, anim_frameduration(model, s->framegroupblend[1].frame), 65.535) * 1000.0));
						MSG_WriteByte(msg, s->framegroupblend[0].lerp * 255.0f);
						MSG_WriteByte(msg, s->framegroupblend[1].lerp * 255.0f);
					}
					else
					{
						MSG_WriteByte(msg, 0);
						MSG_WriteShort(msg, s->framegroupblend[0].frame);
						MSG_WriteShort(msg, (int)(anim_reducetime(sv.time - s->framegroupblend[0].start, anim_frameduration(model, s->framegroupblend[0].frame), 65.535) * 1000.0));
					}
				}
			}
			if (bits & E5_TRAILEFFECTNUM)
				MSG_WriteShort(msg, s->traileffectnum);
			ENTITYSIZEPROFILING_END(msg, s->number, bits);
		}
	}
}

qbool EntityFrame5_WriteFrame(sizebuf_t *msg, int maxsize, entityframe5_database_t *d, int numstates, const entity_state_t **states, int viewentnum, unsigned int movesequence, qbool need_empty)
{
	prvm_prog_t *prog = SVVM_prog;
	const entity_state_t *n;
	int i, num, l, framenum, packetlognumber, priority;
	sizebuf_t buf;
	unsigned char data[128];
	entityframe5_packetlog_t *packetlog;

	if (prog->max_edicts > d->maxedicts)
		EntityFrame5_ExpandEdicts(d, prog->max_edicts);

	framenum = d->latestframenum + 1;
	d->viewentnum = viewentnum;

	// if packet log is full, mark all frames as lost, this will cause
	// it to send the lost data again
	for (packetlognumber = 0;packetlognumber < ENTITYFRAME5_MAXPACKETLOGS;packetlognumber++)
		if (d->packetlog[packetlognumber].packetnumber == 0)
			break;
	if (packetlognumber == ENTITYFRAME5_MAXPACKETLOGS)
	{
		Con_DPrintf("EntityFrame5_WriteFrame: packetlog overflow for a client, resetting\n");
		EntityFrame5_LostFrame(d, framenum);
		packetlognumber = 0;
	}

	// prepare the buffer
	memset(&buf, 0, sizeof(buf));
	buf.data = data;
	buf.maxsize = sizeof(data);

	// detect changes in states
	num = 1;
	for (i = 0;i < numstates;i++)
	{
		n = states[i];
		// mark gaps in entity numbering as removed entities
		for (;num < n->number;num++)
		{
			// if the entity used to exist, clear it
			if (CHECKPVSBIT(d->visiblebits, num))
			{
				CLEARPVSBIT(d->visiblebits, num);
				d->deltabits[num] = E5_FULLUPDATE;
				d->priorities[num] = max(d->priorities[num], 8); // removal is cheap
				d->states[num] = defaultstate;
				d->states[num].number = num;
			}
		}
		// update the entity state data
		if (!CHECKPVSBIT(d->visiblebits, num))
		{
			// entity just spawned in, don't let it completely hog priority
			// because of being ancient on the first frame
			d->updateframenum[num] = framenum;
			// initial priority is a bit high to make projectiles send on the
			// first frame, among other things
			d->priorities[num] = max(d->priorities[num], 4);
		}
		SETPVSBIT(d->visiblebits, num);
		d->deltabits[num] |= EntityState5_DeltaBits(d->states + num, n);
		d->priorities[num] = max(d->priorities[num], 1);
		d->states[num] = *n;
		d->states[num].number = num;
		// advance to next entity so the next iteration doesn't immediately remove it
		num++;
	}
	// all remaining entities are dead
	for (;num < d->maxedicts;num++)
	{
		if (CHECKPVSBIT(d->visiblebits, num))
		{
			CLEARPVSBIT(d->visiblebits, num);
			d->deltabits[num] = E5_FULLUPDATE;
			d->priorities[num] = max(d->priorities[num], 8); // removal is cheap
			d->states[num] = defaultstate;
			d->states[num].number = num;
		}
	}

	// if there isn't at least enough room for an empty svc_entities,
	// don't bother trying...
	if (buf.cursize + 11 > buf.maxsize)
		return false;

	// build lists of entities by priority level
	memset(d->prioritychaincounts, 0, sizeof(d->prioritychaincounts));
	l = 0;
	for (num = 0;num < d->maxedicts;num++)
	{
		if (d->priorities[num])
		{
			if (d->deltabits[num])
			{
				if (d->priorities[num] < (ENTITYFRAME5_PRIORITYLEVELS - 1))
					d->priorities[num] = EntityState5_Priority(d, num);
				l = num;
				priority = d->priorities[num];
				if (d->prioritychaincounts[priority] < ENTITYFRAME5_MAXSTATES)
					d->prioritychains[priority][d->prioritychaincounts[priority]++] = num;
			}
			else
				d->priorities[num] = 0;
		}
	}

	packetlog = NULL;
	// write stat updates
	if (sv.protocol != PROTOCOL_QUAKE && sv.protocol != PROTOCOL_QUAKEDP && sv.protocol != PROTOCOL_NEHAHRAMOVIE && sv.protocol != PROTOCOL_NEHAHRABJP && sv.protocol != PROTOCOL_NEHAHRABJP2 && sv.protocol != PROTOCOL_NEHAHRABJP3 && sv.protocol != PROTOCOL_DARKPLACES1 && sv.protocol != PROTOCOL_DARKPLACES2 && sv.protocol != PROTOCOL_DARKPLACES3 && sv.protocol != PROTOCOL_DARKPLACES4 && sv.protocol != PROTOCOL_DARKPLACES5)
	{
		for (i = 0;i < MAX_CL_STATS && msg->cursize + 6 + 11 <= maxsize;i++)
		{
			if (host_client->statsdeltabits[i>>3] & (1<<(i&7)))
			{
				host_client->statsdeltabits[i>>3] &= ~(1<<(i&7));
				// add packetlog entry now that we have something for it
				if (!packetlog)
				{
					packetlog = d->packetlog + packetlognumber;
					packetlog->packetnumber = framenum;
					packetlog->numstates = 0;
					memset(packetlog->statsdeltabits, 0, sizeof(packetlog->statsdeltabits));
				}
				packetlog->statsdeltabits[i>>3] |= (1<<(i&7));
				if (host_client->stats[i] >= 0 && host_client->stats[i] < 256)
				{
					MSG_WriteByte(msg, svc_updatestatubyte);
					MSG_WriteByte(msg, i);
					MSG_WriteByte(msg, host_client->stats[i]);
					l = 1;
				}
				else
				{
					MSG_WriteByte(msg, svc_updatestat);
					MSG_WriteByte(msg, i);
					MSG_WriteLong(msg, host_client->stats[i]);
					l = 1;
				}
			}
		}
	}

	// only send empty svc_entities frame if needed
	if(!l && !need_empty)
		return false;

	// add packetlog entry now that we have something for it
	if (!packetlog)
	{
		packetlog = d->packetlog + packetlognumber;
		packetlog->packetnumber = framenum;
		packetlog->numstates = 0;
		memset(packetlog->statsdeltabits, 0, sizeof(packetlog->statsdeltabits));
	}

	// write state updates
	if (developer_networkentities.integer >= 10)
		Con_Printf("send: svc_entities %i\n", framenum);
	d->latestframenum = framenum;
	MSG_WriteByte(msg, svc_entities);
	MSG_WriteLong(msg, framenum);
	if (sv.protocol != PROTOCOL_QUAKE && sv.protocol != PROTOCOL_QUAKEDP && sv.protocol != PROTOCOL_NEHAHRAMOVIE && sv.protocol != PROTOCOL_DARKPLACES1 && sv.protocol != PROTOCOL_DARKPLACES2 && sv.protocol != PROTOCOL_DARKPLACES3 && sv.protocol != PROTOCOL_DARKPLACES4 && sv.protocol != PROTOCOL_DARKPLACES5 && sv.protocol != PROTOCOL_DARKPLACES6)
		MSG_WriteLong(msg, movesequence);
	for (priority = ENTITYFRAME5_PRIORITYLEVELS - 1;priority >= 0 && packetlog->numstates < ENTITYFRAME5_MAXSTATES;priority--)
	{
		for (i = 0;i < d->prioritychaincounts[priority] && packetlog->numstates < ENTITYFRAME5_MAXSTATES;i++)
		{
			num = d->prioritychains[priority][i];
			n = d->states + num;
			if (d->deltabits[num] & E5_FULLUPDATE)
				d->deltabits[num] = E5_FULLUPDATE | EntityState5_DeltaBits(&defaultstate, n);
			buf.cursize = 0;
			EntityState5_WriteUpdate(num, n, d->deltabits[num], &buf);
			// if the entity won't fit, try the next one
			if (msg->cursize + buf.cursize + 2 > maxsize)
				continue;
			// write entity to the packet
			SZ_Write(msg, buf.data, buf.cursize);
			// mark age on entity for prioritization
			d->updateframenum[num] = framenum;
			// log entity so deltabits can be restored later if lost
			packetlog->states[packetlog->numstates].number = num;
			packetlog->states[packetlog->numstates].bits = d->deltabits[num];
			packetlog->numstates++;
			// clear deltabits and priority so it won't be sent again
			d->deltabits[num] = 0;
			d->priorities[num] = 0;
		}
	}
	MSG_WriteShort(msg, 0x8000);

	return true;
}

entityframe5_database_t *EntityFrame5_AllocDatabase(mempool_t *pool)
{
	int i;
	entityframe5_database_t *d;
	d = (entityframe5_database_t *)Mem_Alloc(pool, sizeof(*d));
	d->latestframenum = 0;
	for (i = 0;i < d->maxedicts;i++)
		d->states[i] = defaultstate;
	return d;
}

static int packetlog5cmp(const void *a_, const void *b_)
{
	const entityframe5_packetlog_t *a = (const entityframe5_packetlog_t *) a_;
	const entityframe5_packetlog_t *b = (const entityframe5_packetlog_t *) b_;
	return a->packetnumber - b->packetnumber;
}

void EntityFrame5_LostFrame(entityframe5_database_t *d, int framenum)
{
	int i, j, l, bits;
	entityframe5_changestate_t *s;
	entityframe5_packetlog_t *p;
	static unsigned char statsdeltabits[(MAX_CL_STATS+7)/8];
	static int deltabits[MAX_EDICTS];
	entityframe5_packetlog_t *packetlogs[ENTITYFRAME5_MAXPACKETLOGS];

	for (i = 0, p = d->packetlog;i < ENTITYFRAME5_MAXPACKETLOGS;i++, p++)
		packetlogs[i] = p;
	qsort(packetlogs, sizeof(*packetlogs), ENTITYFRAME5_MAXPACKETLOGS, packetlog5cmp);

	memset(deltabits, 0, sizeof(deltabits));
	memset(statsdeltabits, 0, sizeof(statsdeltabits));
	for (i = 0; i < ENTITYFRAME5_MAXPACKETLOGS; i++)
	{
		p = packetlogs[i];

		if (!p->packetnumber)
			continue;

		if (p->packetnumber <= framenum)
		{
			for (j = 0, s = p->states;j < p->numstates;j++, s++)
				deltabits[s->number] |= s->bits;

			for (l = 0;l < (MAX_CL_STATS+7)/8;l++)
				statsdeltabits[l] |= p->statsdeltabits[l];

			p->packetnumber = 0;
		}
		else
		{
			for (j = 0, s = p->states;j < p->numstates;j++, s++)
				deltabits[s->number] &= ~s->bits;
			for (l = 0;l < (MAX_CL_STATS+7)/8;l++)
				statsdeltabits[l] &= ~p->statsdeltabits[l];
		}
	}

	for(i = 0; i < d->maxedicts; ++i)
	{
		bits = deltabits[i] & ~d->deltabits[i];
		if(bits)
		{
			d->deltabits[i] |= bits;
			// if it was a very important update, set priority higher
			if (bits & (E5_FULLUPDATE | E5_ATTACHMENT | E5_MODEL | E5_COLORMAP))
				d->priorities[i] = max(d->priorities[i], 4);
			else
				d->priorities[i] = max(d->priorities[i], 1);
		}
	}

	for (l = 0;l < (MAX_CL_STATS+7)/8;l++)
		host_client->statsdeltabits[l] |= statsdeltabits[l];
		// no need to mask out the already-set bits here, as we do not
		// do that priorities stuff
}

void EntityFrame5_AckFrame(entityframe5_database_t *d, int framenum)
{
	int i;
	// scan for packets made obsolete by this ack and delete them
	for (i = 0;i < ENTITYFRAME5_MAXPACKETLOGS;i++)
		if (d->packetlog[i].packetnumber <= framenum)
			d->packetlog[i].packetnumber = 0;
}
