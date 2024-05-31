#include "quakedef.h"
#include "protocol.h"

// NOTE: this only works with DP5 protocol and upwards. For lower protocols
// (including QUAKE), no packet loss handling for CSQC is done, which makes
// CSQC basically useless.
// Always use the DP5 protocol, or a higher one, when using CSQC entities.
static void EntityFrameCSQC_LostAllFrames(client_t *client)
{
	prvm_prog_t *prog = SVVM_prog;
	// mark ALL csqc entities as requiring a FULL resend!
	// I know this is a bad workaround, but better than nothing.
	int i, n;
	prvm_edict_t *ed;

	n = client->csqcnumedicts;
	for(i = 0; i < n; ++i)
	{
		if(client->csqcentityscope[i] & SCOPE_EXISTED_ONCE)
		{
			ed = prog->edicts + i;
			client->csqcentitysendflags[i] |= 0xFFFFFF;  // FULL RESEND. We can't clear SCOPE_ASSUMED_EXISTING yet as this would cancel removes on a rejected send attempt.
			if (!PRVM_serveredictfunction(ed, SendEntity))  // If it was ever sent to that client as a CSQC entity...
				client->csqcentityscope[i] |= SCOPE_ASSUMED_EXISTING;  // FORCE REMOVE.
		}
	}
}
void EntityFrameCSQC_LostFrame(client_t *client, int framenum)
{
	// marks a frame as lost
	int i, j;
	qbool valid;
	int ringfirst, ringlast;
	static int recoversendflags[MAX_EDICTS]; // client only
	csqcentityframedb_t *d;

	if(client->csqcentityframe_lastreset < 0)
		return;
	if(framenum < client->csqcentityframe_lastreset)
		return; // no action required, as we resent that data anyway

	// is our frame out of history?
	ringfirst = client->csqcentityframehistory_next; // oldest entry
	ringlast = (ringfirst + NUM_CSQCENTITYDB_FRAMES - 1) % NUM_CSQCENTITYDB_FRAMES; // most recently added entry

	valid = false;
	
	for(j = 0; j < NUM_CSQCENTITYDB_FRAMES; ++j)
	{
		d = &client->csqcentityframehistory[(ringfirst + j) % NUM_CSQCENTITYDB_FRAMES];
		if(d->framenum < 0)
			continue;
		if(d->framenum == framenum)
			break;
		else if(d->framenum < framenum)
			valid = true;
	}
	if(j == NUM_CSQCENTITYDB_FRAMES)
	{
		if(valid) // got beaten, i.e. there is a frame < framenum
		{
			// a non-csqc frame got lost... great
			return;
		}
		else
		{
			// a too old frame got lost... sorry, cannot handle this
			Con_DPrintf("CSQC entity DB: lost a frame too early to do any handling (resending ALL)...\n");
			Con_DPrintf("Lost frame = %d\n", framenum);
			Con_DPrintf("Entity DB = %d to %d\n", client->csqcentityframehistory[ringfirst].framenum, client->csqcentityframehistory[ringlast].framenum);
			EntityFrameCSQC_LostAllFrames(client);
			client->csqcentityframe_lastreset = -1;
		}
		return;
	}

	// so j is the frame that got lost
	// ringlast is the frame that we have to go to
	ringfirst = (ringfirst + j) % NUM_CSQCENTITYDB_FRAMES;
	if(ringlast < ringfirst)
		ringlast += NUM_CSQCENTITYDB_FRAMES;
	
	memset(recoversendflags, 0, sizeof(recoversendflags));

	for(j = ringfirst; j <= ringlast; ++j)
	{
		d = &client->csqcentityframehistory[j % NUM_CSQCENTITYDB_FRAMES];
		if(d->framenum < 0)
		{
			// deleted frame
		}
		else if(d->framenum < framenum)
		{
			// a frame in the past... should never happen
			Con_Printf("CSQC entity DB encountered a frame from the past when recovering from PL...?\n");
		}
		else if(d->framenum == framenum)
		{
			// handling the actually lost frame now
			for(i = 0; i < d->num; ++i)
			{
				int sf = d->sendflags[i];
				int ent = d->entno[i];
				if(sf < 0) // remove
					recoversendflags[ent] |= -1; // all bits, including sign
				else if(sf > 0)
					recoversendflags[ent] |= sf;
			}
		}
		else
		{
			// handling the frames that followed it now
			for(i = 0; i < d->num; ++i)
			{
				int sf = d->sendflags[i];
				int ent = d->entno[i];
				if(sf < 0) // remove
				{
					recoversendflags[ent] = 0; // no need to update, we got a more recent remove (and will fix it THEN)
					break; // no flags left to remove...
				}
				else if(sf > 0)
					recoversendflags[ent] &= ~sf; // no need to update these bits, we already got them later
			}
		}
	}

	for(i = 0; i < client->csqcnumedicts; ++i)
	{
		if(recoversendflags[i] < 0)
			client->csqcentityscope[i] |= SCOPE_ASSUMED_EXISTING;  // FORCE REMOVE.
		else
			client->csqcentitysendflags[i] |= recoversendflags[i];
	}
}
static int EntityFrameCSQC_AllocFrame(client_t *client, int framenum)
{
	int ringfirst = client->csqcentityframehistory_next; // oldest entry
	client->csqcentityframehistory_next += 1;
	client->csqcentityframehistory_next %= NUM_CSQCENTITYDB_FRAMES;
	client->csqcentityframehistory[ringfirst].framenum = framenum;
	client->csqcentityframehistory[ringfirst].num = 0;
	return ringfirst;
}
static void EntityFrameCSQC_DeallocFrame(client_t *client, int framenum)
{
	int ringfirst = client->csqcentityframehistory_next; // oldest entry
	int ringlast = (ringfirst + NUM_CSQCENTITYDB_FRAMES - 1) % NUM_CSQCENTITYDB_FRAMES; // most recently added entry
	if(framenum == client->csqcentityframehistory[ringlast].framenum)
	{
		client->csqcentityframehistory[ringlast].framenum = -1;
		client->csqcentityframehistory[ringlast].num = 0;
		client->csqcentityframehistory_next = ringlast;
	}
	else
		Con_Printf("Trying to dealloc the wrong entity frame\n");
}

//[515]: we use only one array per-client for SendEntity feature
// TODO: add some handling for entity send priorities, to better deal with huge
// amounts of csqc networked entities
qbool EntityFrameCSQC_WriteFrame (sizebuf_t *msg, int maxsize, int numnumbers, const unsigned short *numbers, int framenum)
{
	prvm_prog_t *prog = SVVM_prog;
	int num, number, end, sendflags;
	qbool sectionstarted = false;
	const unsigned short *n;
	prvm_edict_t *ed;
	client_t *client = svs.clients + sv.writeentitiestoclient_clientnumber;
	int dbframe = EntityFrameCSQC_AllocFrame(client, framenum);
	csqcentityframedb_t *db = &client->csqcentityframehistory[dbframe];

	if(client->csqcentityframe_lastreset < 0)
		client->csqcentityframe_lastreset = framenum;

	maxsize -= 24; // always fit in an empty svc_entities message (for packet loss detection!)

	// make sure there is enough room to store the svc_csqcentities byte,
	// the terminator (0x0000) and at least one entity update
	if (msg->cursize + 32 >= maxsize)
		return false;

	if (client->csqcnumedicts < prog->num_edicts)
		client->csqcnumedicts = prog->num_edicts;

	number = 1;
	for (num = 0, n = numbers;num < numnumbers;num++, n++)
	{
		end = *n;
		for (;number < end;number++)
		{
			client->csqcentityscope[number] &= ~SCOPE_WANTSEND;
			if (client->csqcentityscope[number] & SCOPE_ASSUMED_EXISTING)
				client->csqcentityscope[number] |= SCOPE_WANTREMOVE;
			client->csqcentitysendflags[number] = 0xFFFFFF;
		}
		ed = prog->edicts + number;
		client->csqcentityscope[number] &= ~SCOPE_WANTSEND;
		if (PRVM_serveredictfunction(ed, SendEntity))
			client->csqcentityscope[number] |= SCOPE_WANTUPDATE;
		else
		{
			if (client->csqcentityscope[number] & SCOPE_ASSUMED_EXISTING)
				client->csqcentityscope[number] |= SCOPE_WANTREMOVE;
			client->csqcentitysendflags[number] = 0xFFFFFF;
		}
		number++;
	}
	end = client->csqcnumedicts;
	for (;number < end;number++)
	{
		client->csqcentityscope[number] &= ~SCOPE_WANTSEND;
		if (client->csqcentityscope[number] & SCOPE_ASSUMED_EXISTING)
			client->csqcentityscope[number] |= SCOPE_WANTREMOVE;
		client->csqcentitysendflags[number] = 0xFFFFFF;
	}

	// now try to emit the entity updates
	// (FIXME: prioritize by distance?)
	end = client->csqcnumedicts;
	for (number = 1;number < end;number++)
	{
		if (!(client->csqcentityscope[number] & SCOPE_WANTSEND))
			continue;
		if(db->num >= NUM_CSQCENTITIES_PER_FRAME)
			break;
		ed = prog->edicts + number;
		if (client->csqcentityscope[number] & SCOPE_WANTREMOVE)  // Also implies ASSUMED_EXISTING.
		{
			// A removal. SendFlags have no power here.
			// write a remove message
			// first write the message identifier if needed
			if(!sectionstarted)
			{
				sectionstarted = 1;
				MSG_WriteByte(msg, svc_csqcentities);
			}
			// write the remove message
			{
				ENTITYSIZEPROFILING_START(msg, number, 0);
				MSG_WriteShort(msg, (unsigned short)number | 0x8000);
				client->csqcentityscope[number] &= ~(SCOPE_WANTSEND | SCOPE_ASSUMED_EXISTING);
				client->csqcentitysendflags[number] = 0xFFFFFF; // resend completely if it becomes active again
				db->entno[db->num] = number;
				db->sendflags[db->num] = -1;
				db->num += 1;
				ENTITYSIZEPROFILING_END(msg, number, 0);
			}
			if (msg->cursize + 17 >= maxsize)
				break;
		}
		else
		{
			// save the cursize value in case we overflow and have to rollback
			int oldcursize = msg->cursize;

			// An update.
			sendflags = client->csqcentitysendflags[number];
			// Nothing to send? FINE.
			if (!sendflags)
				continue;
			// If it's a new entity, always assume sendflags 0xFFFFFF.
			if (!(client->csqcentityscope[number] & SCOPE_ASSUMED_EXISTING))
				sendflags = 0xFFFFFF;

			// write an update
			if (PRVM_serveredictfunction(ed, SendEntity))
			{
				if(!sectionstarted)
					MSG_WriteByte(msg, svc_csqcentities);
				{
					int oldcursize2 = msg->cursize;
					ENTITYSIZEPROFILING_START(msg, number, sendflags);
					MSG_WriteShort(msg, number);
					msg->allowoverflow = true;
					PRVM_G_INT(OFS_PARM0) = sv.writeentitiestoclient_cliententitynumber;
					PRVM_G_FLOAT(OFS_PARM1) = sendflags;
					PRVM_serverglobaledict(self) = number;
					prog->ExecuteProgram(prog, PRVM_serveredictfunction(ed, SendEntity), "Null SendEntity\n");
					msg->allowoverflow = false;
					if(!PRVM_G_FLOAT(OFS_RETURN))
					{
						// Send rejected by CSQC. This means we want to remove it.
						// CSQC requests we remove this one.
						if (client->csqcentityscope[number] & SCOPE_ASSUMED_EXISTING)
						{
							msg->cursize = oldcursize2;
							msg->overflowed = false;
							MSG_WriteShort(msg, (unsigned short)number | 0x8000);
							client->csqcentityscope[number] &= ~(SCOPE_WANTSEND | SCOPE_ASSUMED_EXISTING);
							client->csqcentitysendflags[number] = 0;
							db->entno[db->num] = number;
							db->sendflags[db->num] = -1;
							db->num += 1;
							// and take note that we have begun the svc_csqcentities
							// section of the packet
							sectionstarted = 1;
							ENTITYSIZEPROFILING_END(msg, number, 0);
							if (msg->cursize + 17 >= maxsize)
								break;
						}
						else
						{
							// Nothing to do. Just don't do it again.
							msg->cursize = oldcursize;
							msg->overflowed = false;
							client->csqcentityscope[number] &= ~SCOPE_WANTSEND;
							client->csqcentitysendflags[number] = 0;
						}
						continue;
					}
					else if(PRVM_G_FLOAT(OFS_RETURN) && msg->cursize + 2 <= maxsize)
					{
						// an update has been successfully written
						client->csqcentitysendflags[number] = 0;
						db->entno[db->num] = number;
						db->sendflags[db->num] = sendflags;
						db->num += 1;
						client->csqcentityscope[number] &= ~SCOPE_WANTSEND;
						client->csqcentityscope[number] |= SCOPE_EXISTED_ONCE | SCOPE_ASSUMED_EXISTING;
						// and take note that we have begun the svc_csqcentities
						// section of the packet
						sectionstarted = 1;
						ENTITYSIZEPROFILING_END(msg, number, sendflags);
						if (msg->cursize + 17 >= maxsize)
							break;
						continue;
					}
				}
			}
			// self.SendEntity returned false (or does not exist) or the
			// update was too big for this packet - rollback the buffer to its
			// state before the writes occurred, we'll try again next frame
			msg->cursize = oldcursize;
			msg->overflowed = false;
		}
	}
	if (sectionstarted)
	{
		// write index 0 to end the update (0 is never used by real entities)
		MSG_WriteShort(msg, 0);
	}

	if(db->num == 0)
		// if no single ent got added, remove the frame from the DB again, to allow
		// for a larger history
		EntityFrameCSQC_DeallocFrame(client, framenum);
	
	return sectionstarted;
}
