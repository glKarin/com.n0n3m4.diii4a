#ifndef RENDERQUE_H
#define RENDERQUE_H

void RQ_BeginFrame(void);
void RQ_AddDistReorder(void (*render) (int count, void **objects, void *objtype), void *object, void *objtype, float *pos);

void RQ_RenderBatchClear(void);
void RQ_RenderBatch(void);

typedef struct renderque_s
{
	struct renderque_s *next;
	void (*render) (int count, void **objects, void *objtype);
	void *data1;
	void *data2;
} renderque_t;

#endif
