#include "quakedef.h"
#include "protocol.h"

qbool EntityFrame4_WriteFrame(sizebuf_t *msg, int maxsize, entityframe4_database_t *d, int numstates, const entity_state_t **states)
{
	prvm_prog_t *prog = SVVM_prog;
	const entity_state_t *e, *s;
	entity_state_t inactiveentitystate;
	int i, n, startnumber;
	sizebuf_t buf;
	unsigned char data[128];

	// if there isn't enough space to accomplish anything, skip it
	if (msg->cursize + 24 > maxsize)
		return false;

	// prepare the buffer
	memset(&buf, 0, sizeof(buf));
	buf.data = data;
	buf.maxsize = sizeof(data);

	for (i = 0;i < MAX_ENTITY_HISTORY;i++)
		if (!d->commit[i].numentities)
			break;
	// if commit buffer full, just don't bother writing an update this frame
	if (i == MAX_ENTITY_HISTORY)
		return false;
	d->currentcommit = d->commit + i;

	// this state's number gets played around with later
	inactiveentitystate = defaultstate;

	d->currentcommit->numentities = 0;
	d->currentcommit->framenum = ++d->latestframenumber;
	MSG_WriteByte(msg, svc_entities);
	MSG_WriteLong(msg, d->referenceframenum);
	MSG_WriteLong(msg, d->currentcommit->framenum);
	if (developer_networkentities.integer >= 10)
	{
		Con_Printf("send svc_entities num:%i ref:%i (database: ref:%i commits:", d->currentcommit->framenum, d->referenceframenum, d->referenceframenum);
		for (i = 0;i < MAX_ENTITY_HISTORY;i++)
			if (d->commit[i].numentities)
				Con_Printf(" %i", d->commit[i].framenum);
		Con_Print(")\n");
	}
	if (d->currententitynumber >= prog->max_edicts)
		startnumber = 1;
	else
		startnumber = bound(1, d->currententitynumber, prog->max_edicts - 1);
	MSG_WriteShort(msg, startnumber);
	// reset currententitynumber so if the loop does not break it we will
	// start at beginning next frame (if it does break, it will set it)
	d->currententitynumber = 1;
	for (i = 0, n = startnumber;n < prog->max_edicts;n++)
	{
		if (PRVM_serveredictfunction((&prog->edicts[n]), SendEntity))
			continue;
		// find the old state to delta from
		e = EntityFrame4_GetReferenceEntity(d, n);
		// prepare the buffer
		SZ_Clear(&buf);
		// entity exists, build an update (if empty there is no change)
		// find the state in the list
		for (;i < numstates && states[i]->number < n;i++);
		// make the message
		s = states[i];
		if (s->number == n)
		{
			// build the update
			EntityState_WriteUpdate(s, &buf, e);
		}
		else
		{
			inactiveentitystate.number = n;
			s = &inactiveentitystate;
			if (e->active == ACTIVE_NETWORK)
			{
				// entity used to exist but doesn't anymore, send remove
				MSG_WriteShort(&buf, n | 0x8000);
			}
		}
		// if the commit is full, we're done this frame
		if (msg->cursize + buf.cursize > maxsize - 4)
		{
			// next frame we will continue where we left off
			break;
		}
		// add the entity to the commit
		EntityFrame4_AddCommitEntity(d, s);
		// if the message is empty, skip out now
		if (buf.cursize)
		{
			// write the message to the packet
			SZ_Write(msg, buf.data, buf.cursize);
		}
	}
	d->currententitynumber = n;

	// remove world message (invalid, and thus a good terminator)
	MSG_WriteShort(msg, 0x8000);
	// write the number of the end entity
	MSG_WriteShort(msg, d->currententitynumber);
	// just to be sure
	d->currentcommit = NULL;

	return true;
}
