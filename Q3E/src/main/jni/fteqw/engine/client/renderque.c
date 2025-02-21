//this is to render transparent things in a distance oriented order

#include "quakedef.h"
#include "renderque.h"

#define NUMGRADUATIONS 0x400
static renderque_t *freerque;
static renderque_t *activerque;
static renderque_t *initialque;

static renderque_t *distlastarque[NUMGRADUATIONS];
static renderque_t *distrque[NUMGRADUATIONS];

int rqmaxgrad, rqmingrad;

int rquesize = 0x2000;
float rquepivot;

void RQ_BeginFrame(void)
{
	rquepivot = DotProduct(r_refdef.vieworg, vpn);
}

void RQ_AddDistReorder(void (*render) (int count, void **objects, void *objtype), void *object, void *objtype, float *pos)
{
	int dist;
	renderque_t *rq;
	if (!freerque)
	{
		render(1, &object, objtype);
		return;
	}

	dist = DotProduct(pos, vpn)-rquepivot;

	if (dist > rqmaxgrad)
	{
		if (dist >= NUMGRADUATIONS)
			dist = NUMGRADUATIONS-1;
		rqmaxgrad = dist;
	}
	if (dist < rqmingrad)
	{
		if (dist < 0)	//hmm... value wrapped? shouldn't happen
			return;
//			dist = 0;
		rqmingrad = dist;
	}

	rq = freerque;
	freerque = freerque->next;
	rq->next = NULL;
	if (distlastarque[dist])
		distlastarque[dist]->next = rq;
	distlastarque[dist] = rq;

	rq->render = render;
	rq->data1 = object;
	rq->data2 = objtype;

	if (!distrque[dist])
		distrque[dist] = rq;
}
/*
void RQ_RenderDistAndClear(void)
{
	int i;
	renderque_t *rq;
	for (i = rqmaxgrad; i>=rqmingrad; i--)
//	for (i = rqmingrad; i<=rqmaxgrad; i++)
	{
		for (rq = distrque[i]; rq; rq=rq->next)	
		{
			rq->render(1, &rq->data1, rq->data2);
		}
		if (distlastarque[i])
		{
			distlastarque[i]->next = freerque;
			freerque = distrque[i];
			distrque[i] = NULL;
			distlastarque[i] = NULL;
		}
	}
	rqmaxgrad=0;
	rqmingrad = NUMGRADUATIONS-1;
}
*/
void RQ_RenderBatchClear(void)
{
#define SLOTS 512
	void *slot[SLOTS];
	void *typeptr = NULL;
	int maxslot = SLOTS;
	void (*lr) (int count, void **objects, void *objtype) = NULL;
	int i;
	renderque_t *rq;

	for (i = rqmaxgrad; i>=rqmingrad; i--)
//	for (i = rqmingrad; i<=rqmaxgrad; i++)
	{
		for (rq = distrque[i]; rq; rq=rq->next)	
		{
			if (!maxslot || rq->render != lr || typeptr != rq->data2)
			{
				if (maxslot != SLOTS)
					lr(SLOTS - maxslot, &slot[maxslot], typeptr);
				maxslot = SLOTS;
			}
			
			slot[--maxslot] = rq->data1;
			typeptr = rq->data2;
			lr  = rq->render;
		}
		if (distlastarque[i])
		{
			distlastarque[i]->next = freerque;
			freerque = distrque[i];
			distrque[i] = NULL;
			distlastarque[i] = NULL;
		}
	}
	if (maxslot != SLOTS)
		lr(SLOTS - maxslot, &slot[maxslot], typeptr);
	rqmaxgrad=0;
	rqmingrad = NUMGRADUATIONS-1;
}

//render without clearing
void RQ_RenderBatch(void)
{
#define SLOTS 512
	void *slot[SLOTS];
	void *typeptr = NULL;
	int maxslot = SLOTS;
	void (*lr) (int count, void **objects, void *objtype) = NULL;
	int i;
	renderque_t *rq;

	for (i = rqmaxgrad; i>=rqmingrad; i--)
//	for (i = rqmingrad; i<=rqmaxgrad; i++)
	{
		for (rq = distrque[i]; rq; rq=rq->next)	
		{
			if (!maxslot || rq->render != lr || typeptr != rq->data2)
			{
				if (maxslot != SLOTS)
					lr(SLOTS - maxslot, &slot[maxslot], typeptr);
				maxslot = SLOTS;
			}
			
			slot[--maxslot] = rq->data1;
			typeptr = rq->data2;
			lr  = rq->render;
		}
	}
	if (maxslot != SLOTS)
		lr(SLOTS - maxslot, &slot[maxslot], typeptr);
}


void RQ_Shutdown(void)
{
	Z_Free(initialque);
	initialque = NULL;
	freerque = NULL;
}

void RQ_Init(void)
{
	int		i;

	if (initialque)
		return;

	initialque = (renderque_t *) Z_Malloc (rquesize * sizeof(renderque_t));


	freerque = &initialque[0];
	activerque = NULL;

	for (i=0 ;i<rquesize-1 ; i++)
		initialque[i].next = &initialque[i+1];
	initialque[rquesize-1].next = NULL;
}
