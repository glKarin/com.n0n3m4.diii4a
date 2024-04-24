#include "quakedef.h"
#include "meshqueue.h"

typedef struct meshqueue_s
{
	struct meshqueue_s *next;
	void (*callback)(const entity_render_t *ent, const rtlight_t *rtlight, int numsurfaces, int *surfaceindices);
	const entity_render_t *ent;
	int surfacenumber;
	const rtlight_t *rtlight;
	float dist;
	dptransparentsortcategory_t category;
}
meshqueue_t;

int trans_sortarraysize;
meshqueue_t **trans_hash = NULL;
meshqueue_t ***trans_hashpointer = NULL;

float mqt_viewplanedist;
float mqt_viewmaxdist;
meshqueue_t *mqt_array;
int mqt_count;
int mqt_total;

void R_MeshQueue_BeginScene(void)
{
	mqt_count = 0;
	mqt_viewplanedist = DotProduct(r_refdef.view.origin, r_refdef.view.forward);
	mqt_viewmaxdist = 0;
}

void R_MeshQueue_AddTransparent(dptransparentsortcategory_t category, const vec3_t center, void (*callback)(const entity_render_t *ent, const rtlight_t *rtlight, int numsurfaces, int *surfacelist), const entity_render_t *ent, int surfacenumber, const rtlight_t *rtlight)
{
	meshqueue_t *mq;
	if (mqt_count >= mqt_total || !mqt_array)
	{
		int newtotal = max(1024, mqt_total * 2);
		meshqueue_t *newarray = (meshqueue_t *)Mem_Alloc(cls.permanentmempool, newtotal * sizeof(meshqueue_t));
		if (mqt_array)
		{
			memcpy(newarray, mqt_array, mqt_total * sizeof(meshqueue_t));
			Mem_Free(mqt_array);
		}
		mqt_array = newarray;
		mqt_total = newtotal;
	}
	mq = &mqt_array[mqt_count++];
	mq->callback = callback;
	mq->ent = ent;
	mq->surfacenumber = surfacenumber;
	mq->rtlight = rtlight;
	mq->category = category;
	if (r_transparent_useplanardistance.integer)
		mq->dist = DotProduct(center, r_refdef.view.forward) - mqt_viewplanedist;
	else
		mq->dist = VectorDistance(center, r_refdef.view.origin);
	mq->next = NULL;
	mqt_viewmaxdist = max(mqt_viewmaxdist, mq->dist);
}

void R_MeshQueue_RenderTransparent(void)
{
	int i, hashindex, maxhashindex, batchnumsurfaces;
	float distscale;
	const entity_render_t *ent;
	const rtlight_t *rtlight;
	void (*callback)(const entity_render_t *ent, const rtlight_t *rtlight, int numsurfaces, int *surfaceindices);
	int batchsurfaceindex[MESHQUEUE_TRANSPARENT_BATCHSIZE];
	meshqueue_t *mqt;

	if (!mqt_count)
		return;

	// check for bad cvars
	if (r_transparent_sortarraysize.integer < 1 || r_transparent_sortarraysize.integer > 32768)
		Cvar_SetValueQuick(&r_transparent_sortarraysize, bound(1, r_transparent_sortarraysize.integer, 32768));
	if (r_transparent_sortmindist.integer < 0 || r_transparent_sortmindist.integer >= r_transparent_sortmaxdist.integer)
		Cvar_SetValueQuick(&r_transparent_sortmindist, 0);
	if (r_transparent_sortmaxdist.integer < r_transparent_sortmindist.integer || r_transparent_sortmaxdist.integer > 32768)
		Cvar_SetValueQuick(&r_transparent_sortmaxdist, bound(r_transparent_sortmindist.integer, r_transparent_sortmaxdist.integer, 32768));

	// update hash array
	if (trans_sortarraysize != r_transparent_sortarraysize.integer)
	{
		trans_sortarraysize = r_transparent_sortarraysize.integer;
		if (trans_hash)
			Mem_Free(trans_hash);
		trans_hash = (meshqueue_t **)Mem_Alloc(cls.permanentmempool, sizeof(meshqueue_t *) * trans_sortarraysize); 
		if (trans_hashpointer)
			Mem_Free(trans_hashpointer);
		trans_hashpointer = (meshqueue_t ***)Mem_Alloc(cls.permanentmempool, sizeof(meshqueue_t **) * trans_sortarraysize); 
	}

	// build index
	memset(trans_hash, 0, sizeof(meshqueue_t *) * trans_sortarraysize);
	for (i = 0; i < trans_sortarraysize; i++)
		trans_hashpointer[i] = &trans_hash[i];
	distscale = (trans_sortarraysize - 1) / min(mqt_viewmaxdist, r_transparent_sortmaxdist.integer);
	maxhashindex = trans_sortarraysize - 1;
	for (i = 0, mqt = mqt_array; i < mqt_count; i++, mqt++)
	{
		switch(mqt->category)
		{
		default:
		case TRANSPARENTSORT_HUD:
			hashindex = 0;
			break;
		case TRANSPARENTSORT_DISTANCE:
			// this could use a reduced range if we need more categories
			hashindex = bound(0, (int)(bound(0, mqt->dist - r_transparent_sortmindist.integer, r_transparent_sortmaxdist.integer) * distscale), maxhashindex);
			break;
		case TRANSPARENTSORT_SKY:
			hashindex = maxhashindex;
			break;
		}
		// link to tail of hash chain (to preserve render order)
		mqt->next = NULL;
		*trans_hashpointer[hashindex] = mqt;
		trans_hashpointer[hashindex] = &mqt->next;
	}
	callback = NULL;
	ent = NULL;
	rtlight = NULL;
	batchnumsurfaces = 0;

	// draw
	for (i = maxhashindex; i >= 0; i--)
	{
		if (trans_hash[i])
		{
			for (mqt = trans_hash[i]; mqt; mqt = mqt->next)
			{
				if (ent != mqt->ent || rtlight != mqt->rtlight || callback != mqt->callback || batchnumsurfaces >= MESHQUEUE_TRANSPARENT_BATCHSIZE)
				{
					if (batchnumsurfaces)
						callback(ent, rtlight, batchnumsurfaces, batchsurfaceindex);
					batchnumsurfaces = 0;
					ent = mqt->ent;
					rtlight = mqt->rtlight;
					callback = mqt->callback;
				}
				batchsurfaceindex[batchnumsurfaces++] = mqt->surfacenumber;
			}
		}
	}
	if (batchnumsurfaces)
		callback(ent, rtlight, batchnumsurfaces, batchsurfaceindex);
	mqt_count = 0;
}
