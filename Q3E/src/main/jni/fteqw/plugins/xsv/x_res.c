#include "../plugin.h"

#include "qux.h"
#include "../../engine/qclib/hash.h"


hashtable_t restable;
bucket_t *resbuckets[1357];	//strange number to help the hashing.

xwindow_t *rootwindow;

xresource_t *resources;

qbyte *xscreen;
extern qboolean xscreenmodified;
short xscreenwidth;
short xscreenheight;

int baseres;

void XS_ClearParent(xwindow_t *wnd);

int XS_GetResource(int id, void **data)
{
	xresource_t *res;
	if (id < 0)
		return x_none;

	res = Hash_GetKey(&restable, id);

	if (res)
	{
		if (data)
			*data = res;
		return res->restype;
	}
	return x_none;
}
void *XS_GetResourceType(int id, int requiredtype)
{
	xresource_t *res;
	if (id < 0)
		return NULL;

	res = Hash_GetKey(&restable, id);

	if (res && res->restype == requiredtype)
		return res;
	return NULL;
}

Atom XS_FindAtom(char *name)
{
	xresource_t *res;
	
	for (res = resources; res; res = res->next)
	{
		if (res->restype == x_atom)
		{
			if (!strcmp(((xatom_t*)res)->atomname, name))
				return res->id;
		}
	}

	return None;
}

void XS_DestroyResource(xresource_t *res)
{
	qboolean nofree = false;
	if (res->restype == x_pixmap)
	{
		xpixmap_t *pm = (xpixmap_t *)res;

		if (pm->references)
			nofree = true;
		if (!pm->linked)
			return;

		pm->linked = false;
	}
	if (res->restype == x_gcontext)
	{
/*		xgcontext_t *gc = (xgcontext_t *)res;

		gc->references--;
		if (gc->references > 0)
			return;
*/
	}
	if (res->restype == x_window)
	{
		extern xwindow_t *xfocusedwindow, *xpgrabbedwindow;
		xwindow_t *wnd = (xwindow_t *)res;
		while(wnd->child)
			XS_DestroyResource((xresource_t *)(wnd->child));

		if (xfocusedwindow == wnd)
			xfocusedwindow = wnd->parent;
		if (xpgrabbedwindow == wnd)
			xpgrabbedwindow = wnd->parent;
	

		if (wnd->mapped)
		{
			XW_ExposeWindow(wnd, 0, 0, wnd->width, wnd->height);
		}


		{
			xEvent ev;

			xrefreshed = true;

			ev.u.u.type						= DestroyNotify;
			ev.u.u.detail					= 0;
			ev.u.u.sequenceNumber			= 0;
			ev.u.destroyNotify.event		= wnd->res.id;	//should be the window that has the recieve attribute.
			ev.u.destroyNotify.window		= wnd->res.id;
			X_SendNotificationMasked (&ev, wnd, StructureNotifyMask);
			X_SendNotificationMasked (&ev, wnd, SubstructureNotifyMask);
		}

		XS_ClearParent(wnd);
	}
	if (!res->prev)
	{
		resources = res->next;
		if (res->next)
			res->next->prev = NULL;
	}
	else
	{
		res->prev->next = res->next;
		res->next->prev = res->prev;
	}

//	XS_FindAtom("");

	Hash_RemoveKey(&restable, res->id);
	if (!nofree)
		free(res);

//	XS_FindAtom("");
}

void XS_DestroyResourcesOfClient(xclient_t *cl)
{
	xresource_t *res;
	xnotificationmask_t *nm, *nm2;

	if (xgrabbedclient == cl)
		xgrabbedclient = NULL;
	if (xpointergrabclient == cl)
		xpointergrabclient = NULL;

	for (res = resources; res;)
	{
		if (res->restype == x_window)
		{	//clear the event masks
			nm = ((xwindow_t *)res)->notificationmask;
			if (nm)
			{
				if (nm->client == cl)
				{
					((xwindow_t *)res)->notificationmask = nm->next;
					free(nm);
				}
				else
				{
					for (; nm->next; nm = nm->next)
					{
						if (nm->next->client == cl || !cl)
						{
							nm2 = nm->next;
							nm->next = nm->next->next;
							free(nm2);
							break;
						}
					}
				}
			}
		}
		if (res->owner == cl)
		{
			XS_DestroyResource(res);

			res = resources;	//evil!!!
			continue;
		}
		res = res->next;
	}
}

void XS_LinkResource(xresource_t *res)
{
	res->next = resources;
	resources = res;

	res->prev = NULL;
	if (res->next)
		res->next->prev = res;
	Hash_AddKey(&restable, res->id, res, malloc(sizeof(bucket_t)));
}

xatom_t *XS_CreateAtom(Atom atomid, char *name, xclient_t *owner)
{
	xatom_t *at;
	int nlen = strlen(name);
	at = malloc(sizeof(xatom_t)+nlen);
	at->res.owner = owner;
	at->res.restype = x_atom;
	at->res.id = atomid;
	memcpy(at->atomname, name, nlen+1);

	XS_LinkResource(&at->res);

	return at;
}

void XS_ClearParent(xwindow_t *wnd)
{
	xwindow_t *sib;

	xwindow_t *parent = wnd->parent;

	if (!parent)
		return;

	wnd->parent = NULL;

	if (parent->child == wnd)
	{
		parent->child = wnd->sibling;
		return;
	}

	for (sib = parent->child; sib; sib = sib->sibling)
	{
		if (sib->sibling == wnd)
		{
			sib->sibling = wnd->sibling;
			return;
		}
	}
}

int XS_GetProperty(xwindow_t *wnd, Atom atomid, Atom *atomtype, char *output, int maxlen, int offset, int *extrabytes, int *format)
{
	xproperty_t *xp;
	for (xp = wnd->properties; xp; xp = xp->next)
	{
		if (xp->atomid == atomid)
		{
			if (maxlen > xp->datalen - offset)
				maxlen = xp->datalen - offset;
			memcpy(output, xp->data + offset, maxlen);
			if (maxlen < 0)
				maxlen = 0;
			*extrabytes = xp->datalen - maxlen - offset;
			*format = xp->format;
			*atomtype = xp->type;
			if (*extrabytes < 0)
				*extrabytes = 0;
			return maxlen;
		}
	}
	*format = 0;
	*extrabytes = 0;
	*atomtype = None;
	return 0;
}

void XS_DeleteProperty(xwindow_t *wnd, Atom atomid)
{
	xproperty_t *xp, *prev = NULL;
	for (xp = wnd->properties; xp; xp = xp->next)
	{
		if (xp->atomid == atomid)
		{
			if (prev)
				prev->next = xp->next;
			else
				wnd->properties = xp->next;

			free(xp);
			return;
		}
		prev = xp;
	}
}

void XS_SetProperty(xwindow_t *wnd, Atom atomid, Atom atomtype, char *data, int datalen, int format)
{
	xproperty_t *prop;
	XS_DeleteProperty(wnd, atomid);

	prop = malloc(sizeof(xproperty_t) + datalen);
	prop->atomid = atomid;
	prop->format = format;
	prop->type = atomtype;

	memcpy(prop->data, data, datalen);
	prop->data[datalen] = '\0';
	prop->datalen = datalen;

	prop->next = wnd->properties;
	wnd->properties = prop;
}

void XS_RaiseWindow(xwindow_t *wnd)
{
	xwindow_t *bigger;
	if (!wnd->parent)
		return;

	bigger = wnd->parent->child;
	if (bigger == wnd)
	{
		wnd->parent->child = wnd->sibling;
		bigger = wnd->sibling;
		if (!bigger)
		{
			wnd->parent->child = wnd;
			//ah well, it was worth a try
			return;
		}
	}
	else
	{
		while(bigger->sibling != wnd)
			bigger = bigger->sibling;

		bigger->sibling = wnd->sibling;	//unlink
	}
	while(bigger->sibling)
		bigger = bigger->sibling;

	bigger->sibling = wnd;
	wnd->sibling = NULL;
}
static qboolean XS_IsAncestor(xwindow_t *w, xwindow_t *check)
{
	xwindow_t *p;
	if (w)
	for (p = w->parent; p; p = p->parent)
	{
		if (p == check)
			return true;
	}
	return false;
}
void XS_SetParent(xwindow_t *wnd, xwindow_t *parent)
{
	if (XS_IsAncestor(parent, wnd))
		parent = wnd==rootwindow?NULL:rootwindow;
	if (wnd == parent)
		parent = wnd==rootwindow?NULL:rootwindow;
	XS_ClearParent(wnd);
	if (parent)
	{
		if (!parent->child)
			parent->child = wnd;
		else
		{
			xwindow_t *sib;
			sib = parent->child;
			while(sib->sibling)
			{
				sib = sib->sibling;
			}
			sib->sibling = wnd;
			wnd->sibling = NULL;
		}
//		wnd->sibling = parent->child;
//		parent->child = wnd;
	}
	wnd->parent = parent;

	XS_RaiseWindow(wnd);
}

void X_Resize(xwindow_t *wnd, int newx, int newy, int neww, int newh)
{
	xEvent ev;

/*	if (wnd->xpos == newx && wnd->ypos == newy)
	{
		ev.u.u.type							= ResizeRequest;
		ev.u.u.detail						= 0;
		ev.u.u.sequenceNumber				= 0;
		ev.u.resizeRequest.window			= wnd->res.id;
		ev.u.resizeRequest.width			= wnd->width;
		ev.u.resizeRequest.height			= wnd->height;

		X_SendNotificationMasked(&ev, wnd, StructureNotifyMask);
		X_SendNotificationMasked(&ev, wnd, SubstructureNotifyMask);

		return;
	}*/

	wnd->xpos = newx;
	wnd->ypos = newy;

	if ((wnd->width != neww || wnd->height != newh) && wnd->buffer)
	{
		free(wnd->buffer);
		wnd->buffer = NULL;
	}
	wnd->width = neww;
	wnd->height = newh;

	if (wnd->mapped)
		xrefreshed = true;

	ev.u.u.type							= ConfigureNotify;
	ev.u.u.detail						= 0;
	ev.u.u.sequenceNumber				= 0;
	ev.u.configureNotify.event			= wnd->res.id;
	ev.u.configureNotify.window			= wnd->res.id;
	ev.u.configureNotify.aboveSibling	= None;
	ev.u.configureNotify.x				= wnd->xpos;
	ev.u.configureNotify.y				= wnd->ypos;
	ev.u.configureNotify.width			= wnd->width;
	ev.u.configureNotify.height			= wnd->height;
	ev.u.configureNotify.borderWidth	= 0;
	ev.u.configureNotify.override		= wnd->overrideredirect;
	ev.u.configureNotify.bpad			= 0;

	X_SendNotificationMasked(&ev, wnd, StructureNotifyMask);
	X_SendNotificationMasked(&ev, wnd, SubstructureNotifyMask);
}

xwindow_t *XS_CreateWindow(int wid, xclient_t *owner, xwindow_t *parent, short x, short y, short width, short height)
{
	xwindow_t *neww;
	neww = malloc(sizeof(xwindow_t));
	memset(neww, 0, sizeof(xwindow_t));

	neww->res.id = wid;
	neww->res.owner = owner;
	neww->res.restype = x_window;

	if (width < 0)
	{
		width*=-1;
		x-=width;
	}
	if (height < 0)
	{
		height*=-1;
		x-=height;
	}

	neww->xpos = (CARD16)x;
	neww->ypos = (CARD16)y;
	neww->width = width;
	neww->height = height;
	neww->buffer = NULL;
	neww->depth = 24;

	XS_SetParent(neww, parent);

	XS_LinkResource(&neww->res);
	return neww;
}

xgcontext_t *XS_CreateGContext(int id, xclient_t *owner, xresource_t *drawable)
{
	xgcontext_t *newgc;
	newgc = malloc(sizeof(xgcontext_t));
	memset(newgc, 0, sizeof(xgcontext_t));

	newgc->res.id = id;
	newgc->res.owner = owner;
	newgc->res.restype = x_gcontext;

	newgc->function = GXcopy;
//	newgc->planemask = ~0;
	newgc->bgcolour = 1;
	newgc->fgcolour = 0;
//	newgc->line_width = 0;
//	newgc->line_style = linesolid;
//	newgc->cap_style = capbutt;
//	newgc->join_style = joinmitter;
//	newgc->fill_style = fillsolid;
//	newgc->fill_rule = evenoddrule;
//	newgc->arc_mode = arcpieslice;
//	newgc->tile = somepixmap;
//	newgc->stipple somepixmap;
//	newgc->ts_x_origin = 0;
//	newgc->ts_y_origin = 0;
//	newgc->font = 0;
//	newgc->subwindow_mode = clipbychildren;
//	newgc->graphics_exposures = true;
//	newgc->clip_x_origin = 0;
//	newgc->clip_y_origin = 0;
//	newgc->clip_mask = None;
//	newgc->dash_offset = 0;
//	newgc->dashes = 4;

	XS_LinkResource(&newgc->res);

	return newgc;
}

xpixmap_t *XS_CreatePixmap(int id, xclient_t *owner, int width, int height, int depth)
{
	xpixmap_t *newpm;
	int i;
	unsigned char *c;
	newpm = malloc(sizeof(xpixmap_t) + (width*height*4));
	memset(newpm, 0, sizeof(xpixmap_t));

	for(i = 0, c = (char*)(newpm+1); i < width*height*4; i++)
	{
		c[i] = rand();
	}

	newpm->res.id = id;
	newpm->res.owner = owner;
	newpm->res.restype = x_pixmap;

	if (id>0)
	{
		newpm->linked=true;	//destroyed when last reference AND this are cleared
		XS_LinkResource(&newpm->res);
	}

	newpm->data = (qbyte*)(newpm+1);
	newpm->width = width;
	newpm->height = height;
	newpm->depth = depth;

	return newpm;
}

#include <stdio.h>
xfont_t *XS_CreateFont(int id, xclient_t *owner, char *fontname)
{
	xfont_t *newfont;

	FILE *f;
	int len;
	int width;
	int height;
	int i;

	unsigned int *out;
	unsigned char *in, *insrc;

	f = fopen("xfont.raw", "rb");
	if (f)
	{
		fseek(f, 0, SEEK_END);
		len = ftell(f);
		fseek(f, 0, SEEK_SET);

		if (len == 256*256*3)
		{
			width = height = 256;
		}
		else if (len == 128*128*3)
			width = height = 128;
		else
		{	//urm, no idea.
			fclose(f);
			return NULL;
		}
	}
	else
	{
		width = height = 256;
		len = width*height*3;
	}

	newfont = malloc(sizeof(xfont_t) + (width*height*4));
	memset(newfont, 0, sizeof(xfont_t));

	newfont->res.id = id;
	newfont->res.owner = owner;
	newfont->res.restype = x_font;

	XS_LinkResource(&newfont->res);

	newfont->rowwidth = width;
	newfont->rowheight = height;
	newfont->charwidth = width/16;
	newfont->charheight = height/16;
	newfont->depth = 32;

	in = insrc = malloc(len);
	if (f)
	{
		fread(in, len, 1, f);
		fclose(f);
	}
	else
		memset(in, 0, len);
	out = newfont->data;

	for (i = 0; i < width*height; i++)
	{
		*out = (in[0]) + (in[1]<<8) + (in[2]<<16) + (255u<<24);
		out++;
		in+=3;
	}

	free(insrc);

	return newfont;
}

int XS_NewResource (void)
{
	return baseres++;
}
void XS_CreateInitialResources(void)
{
	static xpixmap_t pm;
	static unsigned int rawpm[8*9] = {
		0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
		0x000000, 0x000000, 0xffffff, 0x000000, 0xffffff, 0x000000, 0x000000, 0x000000,
		0x000000, 0xffffff, 0x000000, 0x000000, 0x000000, 0xffffff, 0x000000, 0x000000,
		0xffffff, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0xffffff, 0x000000,
		0xffffff, 0x000000, 0xffffff, 0xffffff, 0xffffff, 0x000000, 0xffffff, 0x000000,
		0xffffff, 0x000000, 0x000000, 0xffffff, 0x000000, 0x000000, 0xffffff, 0x000000,
		0x000000, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0x000000, 0x000000,
		0x000000, 0x000000, 0x000000, 0xffffff, 0x000000, 0x000000, 0x000000, 0x000000,
		0x000000, 0x000000, 0x000000, 0xffffff, 0x000000, 0x000000, 0x000000, 0x000000};

	if (baseres)
		return;
	baseres=1;

	xscreenwidth = 640;
	xscreenheight = 480;
	xscreen = realloc(xscreen, xscreenwidth*4 * xscreenheight);
	xscreenmodified = true;

	XS_DestroyResourcesOfClient(NULL);

	memset(resbuckets, 0, sizeof(resbuckets));
	Hash_InitTable(&restable, sizeof(resbuckets)/sizeof(resbuckets[0]), resbuckets);

	resources = NULL;


	/*these are the 'always created' atoms*/
	XS_CreateAtom(baseres++, "PRIMARY", NULL);
	XS_CreateAtom(baseres++, "SECONDARY", NULL);
	XS_CreateAtom(baseres++, "ARC", NULL);
	XS_CreateAtom(baseres++, "ATOM", NULL);
	XS_CreateAtom(baseres++, "BITMAP", NULL);
	XS_CreateAtom(baseres++, "CARDINAL", NULL);
	XS_CreateAtom(baseres++, "COLORMAP", NULL);
	XS_CreateAtom(baseres++, "CURSOR", NULL);
	XS_CreateAtom(baseres++, "CUT_BUFFER0", NULL);
	XS_CreateAtom(baseres++, "CUT_BUFFER1", NULL);
	XS_CreateAtom(baseres++, "CUT_BUFFER2", NULL);
	XS_CreateAtom(baseres++, "CUT_BUFFER3", NULL);
	XS_CreateAtom(baseres++, "CUT_BUFFER4", NULL);
	XS_CreateAtom(baseres++, "CUT_BUFFER5", NULL);
	XS_CreateAtom(baseres++, "CUT_BUFFER6", NULL);
	XS_CreateAtom(baseres++, "CUT_BUFFER7", NULL);
	XS_CreateAtom(baseres++, "DRAWABLE", NULL);
	XS_CreateAtom(baseres++, "FONT", NULL);
	XS_CreateAtom(baseres++, "INTEGER", NULL);
	XS_CreateAtom(baseres++, "PIXMAP", NULL);
	XS_CreateAtom(baseres++, "POINT", NULL);
	XS_CreateAtom(baseres++, "RECTANGLE", NULL);
	XS_CreateAtom(baseres++, "RESOURCE_MANAGER", NULL);
	XS_CreateAtom(baseres++, "RGB_COLOR_MAP", NULL);
	XS_CreateAtom(baseres++, "RGB_BEST_MAP", NULL);
	XS_CreateAtom(baseres++, "RGB_BLUE_MAP", NULL);
	XS_CreateAtom(baseres++, "RGB_DEFAULT_MAP", NULL);
	XS_CreateAtom(baseres++, "RGB_GRAY_MAP", NULL);
	XS_CreateAtom(baseres++, "RGB_GREEN_MAP", NULL);
	XS_CreateAtom(baseres++, "RGB_RED_MAP", NULL);
	XS_CreateAtom(baseres++, "STRING", NULL);
	XS_CreateAtom(baseres++, "VISUALID", NULL);
	XS_CreateAtom(baseres++, "WINDOW", NULL);
	XS_CreateAtom(baseres++, "WM_COMMAND", NULL);
	XS_CreateAtom(baseres++, "WM_HINTS", NULL);
	XS_CreateAtom(baseres++, "WM_CLIENT_MACHINE", NULL);
	XS_CreateAtom(baseres++, "WM_ICON_NAME", NULL);
	XS_CreateAtom(baseres++, "WM_ICON_SIZE", NULL);
	XS_CreateAtom(baseres++, "WM_NAME", NULL);
	XS_CreateAtom(baseres++, "WM_NORMAL_HINTS", NULL);
	XS_CreateAtom(baseres++, "WM_SIZE_HINTS", NULL);
	XS_CreateAtom(baseres++, "WM_ZOOM_HINTS", NULL);
	XS_CreateAtom(baseres++, "MIN_SPACE", NULL);
	XS_CreateAtom(baseres++, "NORM_SPACE", NULL);
	XS_CreateAtom(baseres++, "MAX_SPACE", NULL);
	XS_CreateAtom(baseres++, "END_SPACE", NULL);
	XS_CreateAtom(baseres++, "SUPERSCRIPT_X", NULL);
	XS_CreateAtom(baseres++, "SUPERSCRIPT_Y", NULL);
	XS_CreateAtom(baseres++, "SUBSCRIPT_X", NULL);
	XS_CreateAtom(baseres++, "SUBSCRIPT_Y", NULL);
	XS_CreateAtom(baseres++, "UNDERLINE_POSITION", NULL);
	XS_CreateAtom(baseres++, "UNDERLINE_THICKNESS", NULL);
	XS_CreateAtom(baseres++, "STRIKEOUT_ASCENT", NULL);
	XS_CreateAtom(baseres++, "STRIKEOUT_DESCENT", NULL);
	XS_CreateAtom(baseres++, "ITALIC_ANGLE", NULL);
	XS_CreateAtom(baseres++, "X_HEIGHT", NULL);
	XS_CreateAtom(baseres++, "QUAD_WIDTH", NULL);
	XS_CreateAtom(baseres++, "WEIGHT", NULL);
	XS_CreateAtom(baseres++, "POINT_SIZE", NULL);
	XS_CreateAtom(baseres++, "RESOLUTION", NULL);
	XS_CreateAtom(baseres++, "COPYRIGHT", NULL);
	XS_CreateAtom(baseres++, "NOTICE", NULL);
	XS_CreateAtom(baseres++, "FONT_NAME", NULL);
	XS_CreateAtom(baseres++, "FAMILY_NAME", NULL);
	XS_CreateAtom(baseres++, "FULL_NAME", NULL);
	XS_CreateAtom(baseres++, "CAP_HEIGHT", NULL);
	XS_CreateAtom(baseres++, "WM_CLASS", NULL);
	XS_CreateAtom(baseres++, "WM_TRANSIENT_FOR", NULL);

	rootwindow = XS_CreateWindow(baseres++, NULL, NULL, 0, 0, xscreenwidth, xscreenheight);
	rootwindow->depth = 24;
	rootwindow->mapped = true;


	memset(&pm, 0, sizeof(pm));
	pm.linked = true;
	pm.data = (qbyte *)rawpm;
	pm.depth = 24;
	pm.width = 8;
	pm.height = 9;
	pm.references = 2;
	pm.res.restype = x_pixmap;

	rootwindow->backpixmap = &pm;

	XW_ClearArea(rootwindow, 0, 0, rootwindow->width, rootwindow->height);

	xrefreshed = true;
}
