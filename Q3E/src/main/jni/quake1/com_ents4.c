#include "quakedef.h"
#include "protocol.h"

entityframe4_database_t *EntityFrame4_AllocDatabase(mempool_t *pool)
{
	entityframe4_database_t *d;
	d = (entityframe4_database_t *)Mem_Alloc(pool, sizeof(*d));
	d->mempool = pool;
	EntityFrame4_ResetDatabase(d);
	return d;
}

void EntityFrame4_FreeDatabase(entityframe4_database_t *d)
{
	int i;
	for (i = 0;i < MAX_ENTITY_HISTORY;i++)
		if (d->commit[i].entity)
			Mem_Free(d->commit[i].entity);
	if (d->referenceentity)
		Mem_Free(d->referenceentity);
	Mem_Free(d);
}

void EntityFrame4_ResetDatabase(entityframe4_database_t *d)
{
	int i;
	d->referenceframenum = -1;
	for (i = 0;i < MAX_ENTITY_HISTORY;i++)
		d->commit[i].numentities = 0;
	for (i = 0;i < d->maxreferenceentities;i++)
		d->referenceentity[i] = defaultstate;
}

entity_state_t *EntityFrame4_GetReferenceEntity(entityframe4_database_t *d, int number)
{
	if (d->maxreferenceentities <= number)
	{
		int oldmax = d->maxreferenceentities;
		entity_state_t *oldentity = d->referenceentity;
		d->maxreferenceentities = (number + 15) & ~7;
		d->referenceentity = (entity_state_t *)Mem_Alloc(d->mempool, d->maxreferenceentities * sizeof(*d->referenceentity));
		if (oldentity)
		{
			memcpy(d->referenceentity, oldentity, oldmax * sizeof(*d->referenceentity));
			Mem_Free(oldentity);
		}
		// clear the newly created entities
		for (;oldmax < d->maxreferenceentities;oldmax++)
		{
			d->referenceentity[oldmax] = defaultstate;
			d->referenceentity[oldmax].number = oldmax;
		}
	}
	return d->referenceentity + number;
}

void EntityFrame4_AddCommitEntity(entityframe4_database_t *d, const entity_state_t *s)
{
	// resize commit's entity list if full
	if (d->currentcommit->maxentities <= d->currentcommit->numentities)
	{
		entity_state_t *oldentity = d->currentcommit->entity;
		d->currentcommit->maxentities += 8;
		d->currentcommit->entity = (entity_state_t *)Mem_Alloc(d->mempool, d->currentcommit->maxentities * sizeof(*d->currentcommit->entity));
		if (oldentity)
		{
			memcpy(d->currentcommit->entity, oldentity, d->currentcommit->numentities * sizeof(*d->currentcommit->entity));
			Mem_Free(oldentity);
		}
	}
	d->currentcommit->entity[d->currentcommit->numentities++] = *s;
}

int EntityFrame4_AckFrame(entityframe4_database_t *d, int framenum, int servermode)
{
	int i, j, found;
	entity_database4_commit_t *commit;
	if (framenum == -1)
	{
		// reset reference, but leave commits alone
		d->referenceframenum = -1;
		for (i = 0;i < d->maxreferenceentities;i++)
			d->referenceentity[i] = defaultstate;
		// if this is the server, remove commits
		for (i = 0, commit = d->commit;i < MAX_ENTITY_HISTORY;i++, commit++)
			commit->numentities = 0;
		found = true;
	}
	else if (d->referenceframenum == framenum)
		found = true;
	else
	{
		found = false;
		for (i = 0, commit = d->commit;i < MAX_ENTITY_HISTORY;i++, commit++)
		{
			if (commit->numentities && commit->framenum <= framenum)
			{
				if (commit->framenum == framenum)
				{
					found = true;
					d->referenceframenum = framenum;
					if (developer_networkentities.integer >= 3)
					{
						for (j = 0;j < commit->numentities;j++)
						{
							entity_state_t *s = EntityFrame4_GetReferenceEntity(d, commit->entity[j].number);
							if (commit->entity[j].active != s->active)
							{
								if (commit->entity[j].active == ACTIVE_NETWORK)
									Con_Printf("commit entity %i has become active (modelindex %i)\n", commit->entity[j].number, commit->entity[j].modelindex);
								else
									Con_Printf("commit entity %i has become inactive (modelindex %i)\n", commit->entity[j].number, commit->entity[j].modelindex);
							}
							*s = commit->entity[j];
						}
					}
					else
						for (j = 0;j < commit->numentities;j++)
							*EntityFrame4_GetReferenceEntity(d, commit->entity[j].number) = commit->entity[j];
				}
				commit->numentities = 0;
			}
		}
	}
	if (developer_networkentities.integer >= 1)
	{
		Con_Printf("ack ref:%i database updated to: ref:%i commits:", framenum, d->referenceframenum);
		for (i = 0;i < MAX_ENTITY_HISTORY;i++)
			if (d->commit[i].numentities)
				Con_Printf(" %i", d->commit[i].framenum);
		Con_Print("\n");
	}
	return found;
}
