//history is implemented using a ringbuffer
typedef struct
{
	float timestamp;
	int brushmodel;
	int wasdelete;
	int id;
	brushface_t *brushdata;	//list of faces
	patchvert_t *patchdata;	//list of control points
	union
	{
		struct
		{
			int numfaces;
			int contents;
		} brushinfo;
		patchinfo_t patchinfo;
	};
} history_t;
history_t *historyring;
int historyring_length;
int history_min;	//oldest we can go
int history;		//updated on each change
int history_max;	//max value for redo

//when replaying history, ids get changed, which makes things fun. this updates selection state for edited brushes.
void(int modelidx, int old, int new) history_remap =
{
	for (int i = history_min; i < history_max; i++)
	{
		history_t *h = &historyring[i & (historyring_length-1)];
		if (h->id == old)
			h->id = new;
	}
	
	int i = brush_isselected(modelidx, old);
	if (i)
	{
		i--;
		brush_selected(modelidx, old, -1, FALSE);
		brush_selected(modelidx, new, -1, TRUE);
		selectedbrushes[i].id = new;
	}
};

void() brush_undo =
{
	if (history == history_min)
		print("Nothing to undo.\n");
	do
	{
		if (history <= history_min)
			return;
		history--;

		history_t *h = &historyring[history & (historyring_length-1)];

		//we're undoing, so deletes create and creates delete. weird, yes.
		if (h->wasdelete)
		{
			int newid;
			if (h->patchdata)
				newid = patch_create(h->brushmodel, 0, h->patchdata, h->patchinfo);
			else
				newid = brush_create(h->brushmodel, h->brushdata, h->brushinfo.numfaces, h->brushinfo.contents);
			history_remap(h->brushmodel, h->id, newid);
			h->id = newid;
		}
		else
		{
			brush_delete(h->brushmodel, h->id);
			h->id = 0;
		}
		if (history == history_min)
			break;	//as far back as we can go, more timestamps are invalid
	} while (historyring[(history-1) & (historyring_length-1)].timestamp == h->timestamp);
};
void() brush_redo =
{
	if (history == history_max)
		print("Nothing to redo.");
	do
	{
		if (history >= history_max)
			return;

		history_t *h = &historyring[history & (historyring_length-1)];

		//we're redoing stuff that has previously been done. yay.
		if (h->wasdelete)
		{
			brush_delete(h->brushmodel, h->id);
			h->id = 0;
		}
		else
		{
			int newid;
			if (h->patchdata)
				newid = patch_create(h->brushmodel, 0, h->patchdata, h->patchinfo);
			else
				newid = brush_create(h->brushmodel, h->brushdata, h->brushinfo.numfaces, h->brushinfo.contents);
			history_remap(h->brushmodel, h->id, newid);
			h->id = newid;
		}
		history++;
		if (history == history_max)
			return;	//don't poke beyond the end...
	} while (historyring[history & (historyring_length-1)].timestamp == h->timestamp);
};

history_t *() history_allocate =	//returns a new history entry.
{
	if (!historyring)
	{
		history_min = history_max = history;
		historyring_length = 512;
		historyring = memalloc(sizeof(*historyring)*historyring_length);
	}
	history_t *h = &historyring[history & (historyring_length-1)];
	
	history++;
	history_max = history;	//always break any pending redos
	if (history_min < history_max - historyring_length)
		history_min = history_max - historyring_length;
	
	h->timestamp = time;
	memfree(h->brushdata);
	h->brushdata = __NULL__;
	return h;
};
void(history_t *h) history_deallocate =	//cancels a newly created history entry, in case something went wrong.
{
	history--;
	if (h == &historyring[history & (historyring_length-1)])
		history_max--;
};
//create and journal.
int(int mod, brushface_t *faces, int numfaces, int contents, float selectnow) brush_history_create =
{
	history_t *h = history_allocate();

	h->brushdata = memalloc(sizeof(brushface_t) * numfaces);
	memcpy(h->brushdata, faces, sizeof(brushface_t) * numfaces);
	h->brushinfo.numfaces = numfaces;
	h->brushinfo.contents = contents;
	h->brushmodel = mod;
	h->wasdelete = FALSE;

	h->id = brush_create(h->brushmodel, h->brushdata, h->brushinfo.numfaces, h->brushinfo.contents);

	if (!h->id)
		history_deallocate(h);
	else if (selectnow)
		brush_select(mod, h->id);
	return h->id;
};
//create and journal.
int(int mod, patchvert_t *vert, patchinfo_t info, float selectnow) patch_history_create =
{
	int totalcp = info.cpwidth * info.cpheight;
	history_t *h = history_allocate();

	h->patchdata = memalloc(sizeof(patchvert_t) * totalcp);
	memcpy(h->patchdata, vert, sizeof(patchvert_t) * totalcp);
	h->patchinfo = info;
	h->brushmodel = mod;
	h->wasdelete = FALSE;

	h->id = patch_create(h->brushmodel, 0, vert, info);

	if (!h->id)
		history_deallocate(h);
	else if (selectnow)
		brush_select(mod, h->id);
	return h->id;
};
//delete and journal.
void(int mod, int id) brush_history_delete =
{
	int numfaces = brush_get(mod, id, __NULL__, 0, __NULL__);
	int numcp = patch_getcp(mod, id, __NULL__, 0, __NULL__);
	brush_deselect(mod, id);

	if (!numfaces && !numcp)
		return;	//doesn't exist or something, some kind of error. we can't log a delete if it didn't delete anything, if only because we can't recreate it if its undone.
		
	history_t *h = history_allocate();

	h->brushmodel = mod;
	h->id = id;
	h->wasdelete = TRUE;

	if (numcp)
	{
		h->patchdata = memalloc(sizeof(patchvert_t) * numcp);
		patch_getcp(mod, id, h->patchdata, numcp, &h->patchinfo);
	}
	else
	{
		h->brushdata = memalloc(sizeof(brushface_t) * numfaces);
		h->brushinfo.numfaces = brush_get(mod, id, h->brushdata, numfaces, &h->brushinfo.contents);
	}
	brush_delete(mod, id);
};

int(int mod, int id, brushface_t *faces, int numfaces, int contents) brush_history_edit =
{
	int wasselected = brush_isselected(mod, id);
	if (wasselected)
		selectedbrushes[wasselected-1].id = -id;	//hack so that brush_history_delete won't remove this entry.
	brush_history_delete(mod, id);
	id = brush_history_create(mod, faces, numfaces, contents, FALSE);
	if (wasselected)
	{
		selectedbrushes[wasselected-1].id = id;
		brush_selected(mod, id, -1, TRUE);
	}
	return id;
};
int(int mod, int id, patchvert_t *vert, patchinfo_t info) patch_history_edit =
{
	int wasselected = brush_isselected(mod, id);
	if (wasselected)
		selectedbrushes[wasselected-1].id = -id;	//hack so that brush_history_delete won't remove this entry.
	brush_history_delete(mod, id);
	id = patch_history_create(mod, vert, info, FALSE);
	if (wasselected)
	{
		selectedbrushes[wasselected-1].id = id;
		brush_selected(mod, id, -1, TRUE);
	}
	return id;
};