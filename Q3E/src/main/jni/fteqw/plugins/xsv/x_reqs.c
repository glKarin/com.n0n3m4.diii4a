#include "../plugin.h"
#include <math.h>

#include "qux.h"
#undef strncpy

XRequest XRequests [256];

#ifdef XBigReqExtensionName
int	X_BigReqCode;
#endif

void XR_MapWindow(xclient_t *cl, xReq *request);
void XR_UnmapWindow(xclient_t *cl, xReq *request);


#define	GXclear			0x0		/* 0 */
#define GXand			0x1		/* src AND dst */
#define GXandReverse		0x2		/* src AND NOT dst */
#define GXcopy			0x3		/* src */
#define GXandInverted		0x4		/* NOT src AND dst */
#define	GXnoop			0x5		/* dst */
#define GXxor			0x6		/* src XOR dst */
#define GXor			0x7		/* src OR dst */
#define GXnor			0x8		/* NOT src AND NOT dst */
#define GXequiv			0x9		/* NOT src XOR dst */
#define GXinvert		0xa		/* NOT dst */
#define GXorReverse		0xb		/* src OR NOT dst */
#define GXcopyInverted		0xc		/* NOT src */
#define GXorInverted		0xd		/* NOT src OR dst */
#define GXnand			0xe		/* NOT src OR NOT dst */
#define GXset			0xf		/* 1 */

#define GCFunc(src, dst, fnc, out, setval)	\
switch(fnc)							\
{									\
case GXclear:						\
	out = 0;						\
	break;							\
case GXand:							\
	out = src&dst;					\
	break;							\
case GXandReverse:					\
	out = src&~dst;					\
	break;							\
case GXcopy:						\
	out = src;						\
	break;							\
case GXandInverted:					\
	out = ~src&dst;					\
	break;							\
case GXnoop:						\
	out = dst;						\
	break;							\
case GXxor:							\
	out = src^dst;					\
	break;							\
case GXor:							\
	out = src|dst;					\
	break;							\
case GXnor:							\
	out = ~src&~dst;				\
	break;							\
case GXequiv:						\
	out = ~src^dst;					\
	break;							\
case GXinvert:						\
	out = ~dst;						\
	break;							\
case GXorReverse:					\
	out = src|~dst;					\
	break;							\
case GXcopyInverted:				\
	out = ~src;						\
	break;							\
case GXorInverted:					\
	out = ~src|dst;					\
	break;							\
case GXnand:						\
	out = ~src|~dst;				\
	break;							\
case GXset:							\
	out = setval;					\
	break;							\
}
void XW_ClearArea(xwindow_t *wnd, int xp, int yp, int width, int height);

void XR_QueryExtension (xclient_t *cl, xReq *request)
{
	char extname[256];
	xQueryExtensionReply rep;
	xQueryExtensionReq *req = (xQueryExtensionReq *)request;

	if (req->nbytes > sizeof(extname)-1)
		req->nbytes = sizeof(extname)-1;
	memcpy(extname, (char *)(req+1), req->nbytes);
	extname[req->nbytes] = '\0';

#ifdef XBigReqExtensionName
	if (X_BigReqCode && !strcmp(extname, XBigReqExtensionName))
	{
		rep.major_opcode	= X_BigReqCode;
		rep.present			= true;
		rep.first_event		= 0;
		rep.first_error		= 0;
	}
	else
#endif
	{
		Con_Printf("x11sv plugin: Extension %s not supported\n", extname);
		rep.major_opcode	= 0;
		rep.present			= false;
		rep.first_event		= 0;
		rep.first_error		= 0;
	}

    rep.type			= X_Reply;
    rep.pad1			= 0;
    rep.sequenceNumber	= cl->requestnum;
    rep.length			= 0;
    rep.pad3			= 0;
    rep.pad4			= 0;
    rep.pad5			= 0;
    rep.pad6			= 0;
    rep.pad7			= 0;

	X_SendData(cl, &rep, sizeof(rep));
}


void XW_ExposeWindowRegionInternal(xwindow_t *root, int x, int y, int width, int height)
{
	int nx,ny,nw,nh;
	xEvent ev;
	if (!root->mapped || root->inputonly)
		return;


	ev.u.u.type						= VisibilityNotify;
	ev.u.u.detail					= 0;
	ev.u.u.sequenceNumber			= 0;
	ev.u.visibility.window			= root->res.id;
	ev.u.visibility.state			= VisibilityUnobscured;
	ev.u.visibility.pad1			= 0;
	ev.u.visibility.pad2			= 0;
	ev.u.visibility.pad3			= 0;

	X_SendNotificationMasked(&ev, root, VisibilityChangeMask);

	ev.u.u.type						= Expose;
	ev.u.u.detail					= 0;
	ev.u.u.sequenceNumber			= 0;
	ev.u.expose.window				= root->res.id;
	ev.u.expose.x					= x;
	ev.u.expose.y					= y;
	ev.u.expose.width				= width;
	ev.u.expose.height				= height;
	ev.u.expose.count				= false;	//other expose events following (none - rewrite to group these then send all in one go...)
	ev.u.expose.pad2				= 0;

	X_SendNotificationMasked(&ev, root, ExposureMask);

	if (root->buffer && root != rootwindow)
	{
//		XW_ClearArea(root, 0, 0, root->width, root->height);
//		free(root->buffer);
//		root->buffer = NULL;
	}

	for (root = root->child; root; root = root->sibling)
	{
		if (!root->mapped || root->inputonly)
			continue;

		//subtract the minpos
		nx = x - root->xpos;	
		nw = width;
		ny = y - root->ypos;
		nh = height;

		//cap new minpos to the child window.
		if (nx < 0)
		{
			nw += nx;
			nx = 0;
		}
		if (ny < 0)
		{
			nh += ny;
			ny = 0;
		}

		//cap new maxpos
		if (nx+nw > x + root->width)
			nw = x+root->width - nx;
		if (ny+nh > y + root->height)
			nh = y+root->height - ny;

		if (nw > 0 && nh > 0)	//make sure some is valid.
			XW_ExposeWindowRegionInternal(root, nx, ny, nw, nh);
	}
}

void XW_ExposeWindow(xwindow_t *root, int x, int y, int width, int height)
{//we have to go back to the root so we know the exact region, and can expose our sibling's windows.
	while(root)
	{
		x += root->xpos;
		y += root->ypos;
		root = root->parent;
	}
	
	XW_ExposeWindowRegionInternal(rootwindow, x, y, width, height);
}

void XR_ListExtensions (xclient_t *cl, xReq *request)
{
	char buffer[8192];
	xListExtensionsReply *rep = (xListExtensionsReply *)buffer;
	char *out;

    rep->type			= X_Reply;
    rep->nExtensions	= 0;
    rep->sequenceNumber	= cl->requestnum;
    rep->length			= 0;
    rep->pad2			= 0;
    rep->pad3			= 0;
    rep->pad4			= 0;
    rep->pad5			= 0;
    rep->pad6			= 0;
    rep->pad7			= 0;

	out = (char *)(rep+1);

#ifdef XBigReqExtensionName
	rep->nExtensions++;
	strcpy(out, XBigReqExtensionName);
	out+=strlen(out)+1;
#endif

	rep->length = (out-(char *)(rep+1) + 3)/4;


	X_SendData(cl, rep, sizeof(xListExtensionsReply) + rep->length*4);
}

void XR_SetCloseDownMode(xclient_t *cl, xReq *request)
{
	xSetCloseDownModeReq *req = (xSetCloseDownModeReq*)request;

	switch(req->mode)
	{
	case DestroyAll:
	case RetainPermanent:
	case RetainTemporary:
		break;
	default:
		X_SendError(cl, BadValue, req->mode, X_SetCloseDownMode, 0);
		return;
	}
	cl->closedownmode = req->mode;
}

void XR_GetAtomName (xclient_t *cl, xReq *request)
{
	xResourceReq *req = (xResourceReq*)request;
	char buffer[8192];
	xGetAtomNameReply *rep = (xGetAtomNameReply*)buffer;

	xatom_t	*xa;

	if (XS_GetResource(req->id, (void**)&xa) != x_atom)
	{
		X_SendError(cl, BadAtom, req->id, X_GetAtomName, 0);
		return;
	}

	rep->type			= X_Reply;
	rep->pad1			= 0;
	rep->sequenceNumber	= cl->requestnum;
	rep->length			= (strlen(xa->atomname)+3)/4;
	rep->nameLength		= strlen(xa->atomname);
	rep->pad2			= 0;
	rep->pad3			= 0;
	rep->pad4			= 0;
	rep->pad5			= 0;
	rep->pad6			= 0;
	rep->pad7			= 0;
	strcpy((char *)(rep+1), xa->atomname);

	X_SendData(cl, rep, sizeof(*rep)+rep->length*4);
}

void XR_InternAtom (xclient_t *cl, xReq *request)
{
	xInternAtomReq *req = (xInternAtomReq*)request;
	xInternAtomReply rep;
	char atomname[1024];
	Atom atom;

	if (req->nbytes >= sizeof(atomname))
	{	//exceeded that limit then...
		X_SendError(cl, BadImplementation, 0, X_InternAtom, 0);
		return;
	}

	strncpy(atomname, (char *)(req+1), req->nbytes);
	atomname[req->nbytes] = '\0';

	atom = XS_FindAtom(atomname);
	if (atom == None && !req->onlyIfExists)
	{
		atom = XS_NewResource();
		XS_CreateAtom(atom, atomname, NULL);	//global atom...
	}

	rep.type	= X_Reply;
	rep.pad1	= 0;
	rep.sequenceNumber	= cl->requestnum;
	rep.length	= 0;
	rep.atom	= atom;
	rep.pad2	= 0;
	rep.pad3	= 0;
	rep.pad4	= 0;
	rep.pad5	= 0;
	rep.pad6	= 0;

	X_SendData(cl, &rep, sizeof(rep));
}

void XR_GetProperty (xclient_t *cl, xReq *request)
{
	xGetPropertyReq *req = (xGetPropertyReq*)request;
	char buffer[8192];
	xwindow_t *wnd;
	int datalen;
	int format;
	int trailing;
	xGetPropertyReply *rep = (xGetPropertyReply*)buffer;
	Atom proptype;

	if (XS_GetResource(req->window, (void**)&wnd) != x_window)
	{	//wait a minute, That's not a window!!!
		X_SendError(cl, BadWindow, req->window, X_GetProperty, 0);
		return;
	}
	if (XS_GetResource(req->property, (void**)NULL) != x_atom)
	{	//whoops
		X_SendError(cl, BadAtom, req->property, X_GetProperty, 0);
		return;
	}

	if (req->longLength > sizeof(buffer) - sizeof(req)/4)
		req->longLength = sizeof(buffer) - sizeof(req)/4;

	datalen = XS_GetProperty(wnd, req->property, &proptype, (char *)(rep+1), req->longLength*4, req->longOffset*4, &trailing, &format);

	rep->type			= X_Reply;
    rep->format			= format;
    rep->propertyType	= proptype;
    rep->sequenceNumber	= cl->requestnum;
    rep->length			= (datalen+3)/4;
	//rep->propertyType	= None;
    rep->bytesAfter		= trailing;
	if (format)
		rep->nItems			= datalen / (format/8);
	else
		rep->nItems		= 0;
    rep->pad1			= 0;
    rep->pad2			= 0;
    rep->pad3			= 0;

	X_SendData(cl, rep, rep->length*4 + sizeof(*rep));

	if (req->delete)
	{
		xEvent ev;

		XS_DeleteProperty(wnd, req->property);

		ev.u.u.type						= PropertyNotify;
		ev.u.u.detail					= 0;
		ev.u.u.sequenceNumber			= 0;
		ev.u.property.window			= req->window;
		ev.u.property.atom				= req->property;
		ev.u.property.time				= plugfuncs->GetMilliseconds();
		ev.u.property.state				= PropertyDelete;

		ev.u.property.pad1				= 0;
		ev.u.property.pad2				= 0;

		X_SendNotificationMasked(&ev, wnd, PropertyChangeMask);
	}
}

void XR_ListProperties(xclient_t *cl, xReq *request)
{
	xproperty_t *xp;
	xResourceReq *req = (xResourceReq*)request;
	char buffer[65536];
	xwindow_t *wnd;
	xListPropertiesReply *rep = (xListPropertiesReply*)buffer;
	Atom *out = (Atom *)(rep+1);

	if (XS_GetResource(req->id, (void**)&wnd) != x_window)
	{	//wait a minute, That's not a window!!!
		X_SendError(cl, BadWindow, req->id, X_GetProperty, 0);
		return;
	}


	rep->type			= X_Reply;
    rep->sequenceNumber	= cl->requestnum;
    rep->length			= 0;
	rep->nProperties	= 0;
    rep->pad1			= 0;
    rep->pad2			= 0;
    rep->pad3			= 0;
	rep->pad4			= 0;
	rep->pad5			= 0;
	rep->pad6			= 0;
	rep->pad7			= 0;

	for (xp = wnd->properties; xp; xp = xp->next)
	{
		rep->nProperties++;
		*out = xp->atomid;
	}

	rep->length = rep->nProperties;

	X_SendData(cl, rep, rep->length*4 + sizeof(*rep));
}

void XR_ChangeProperty (xclient_t *cl, xReq *request)
{
	xChangePropertyReq *req = (xChangePropertyReq*)request;
	int len;

	xatom_t *atom;
	xwindow_t *wnd;

	if (XS_GetResource(req->window, (void**)&wnd) != x_window)
	{	//wait a minute, That's not a window!!!
		X_SendError(cl, BadWindow, req->window, X_ChangeProperty, 0);
		return;
	}

	if (XS_GetResource(req->property, (void**)&atom) != x_atom)
	{
		X_SendError(cl, BadAtom, req->property, X_ChangeProperty, 0);
		return;
	}

	len = req->nUnits * (req->format/8);

	if (req->mode == PropModeReplace)
		XS_SetProperty(wnd, req->property, req->type, (char *)(req+1), len, req->format);
	else if (req->mode == PropModePrepend)
	{
		X_SendError(cl, BadImplementation, req->window, X_ChangeProperty, 0);
		return;
	}
	else if (req->mode == PropModeAppend)
	{
		char hugebuffer[65536];
		int trailing;
		int format, datalen;
		Atom proptype;


		datalen = XS_GetProperty(wnd, req->property, &proptype, hugebuffer, sizeof(hugebuffer), 0, &trailing, &format);
		if (datalen+len > sizeof(hugebuffer))
		{
			X_SendError(cl, BadImplementation, req->window, X_ChangeProperty, 0);
			return;
		}
		memcpy(hugebuffer + datalen, (char *)(req+1), len);

		XS_SetProperty(wnd, req->property, proptype, hugebuffer, datalen+len, format);
	}

	{
		xEvent ev;

		ev.u.u.type						= PropertyNotify;
		ev.u.u.detail					= 0;
		ev.u.u.sequenceNumber			= 0;
		ev.u.property.window			= req->window;
		ev.u.property.atom				= req->property;
		ev.u.property.time				= plugfuncs->GetMilliseconds();
		ev.u.property.state				= PropertyNewValue;

		ev.u.property.pad1				= 0;
		ev.u.property.pad2				= 0;

		X_SendNotificationMasked(&ev, wnd, PropertyChangeMask);
	}
}

void XR_DeleteProperty(xclient_t *cl, xReq *request)
{
	xDeletePropertyReq *req = (xDeletePropertyReq*)request;

	xwindow_t *wnd;

	if (XS_GetResource(req->window, (void**)&wnd) != x_window)
	{	//wait a minute, That's not a window!!!
		X_SendError(cl, BadWindow, req->window, X_DeleteProperty, 0);
		return;
	}

	if (XS_GetResource(req->property, (void**)NULL) != x_atom)
	{
		X_SendError(cl, BadAtom, req->property, X_DeleteProperty, 0);
		return;
	}

	XS_DeleteProperty(wnd, req->property);

	{
		xEvent ev;

		ev.u.u.type						= PropertyNotify;
		ev.u.u.detail					= 0;
		ev.u.u.sequenceNumber			= 0;
		ev.u.property.window			= req->window;
		ev.u.property.atom				= req->property;
		ev.u.property.time				= plugfuncs->GetMilliseconds();
		ev.u.property.state				= PropertyDelete;

		ev.u.property.pad1				= 0;
		ev.u.property.pad2				= 0;

		X_SendNotificationMasked(&ev, wnd, PropertyChangeMask);
	}
}

void XR_GetSelectionOwner (xclient_t *cl, xReq *request)
{
	xResourceReq *req = (xResourceReq *)request;
	xGetSelectionOwnerReply reply;
	xatom_t *atom;

	if (XS_GetResource(req->id, (void**)&atom) != x_atom)
	{
		X_SendError(cl, BadAtom, req->id, X_GetSelectionOwner, 0);
		return;
	}

	if (XS_GetResource(atom->selectionownerwindowid, (void**)NULL) != x_window)	//make sure the window still exists.
	{
		atom->selectionownerwindowid = None;
	}

	reply.type				= X_Reply;
    reply.sequenceNumber	= cl->requestnum;
    reply.length			= 0;
	reply.owner				= atom->selectionownerwindowid;
    reply.pad1				= 0;
    reply.pad2				= 0;
    reply.pad3				= 0;
	reply.pad4				= 0;
	reply.pad5				= 0;
	reply.pad6				= 0;

	X_SendData(cl, &reply, sizeof(reply));
}
void XR_SetSelectionOwner (xclient_t *cl, xReq *request)
{
	xSetSelectionOwnerReq *req = (xSetSelectionOwnerReq *)request;
	xatom_t *atom;
	xwindow_t *window;

	if (XS_GetResource(req->selection, (void**)&atom) != x_atom)
	{
		X_SendError(cl, BadAtom, req->selection, X_SetSelectionOwner, 0);
		return;
	}

	if (XS_GetResource(req->window, (void**)&window) != x_window)	//make sure the window still exists.
	{
		X_SendError(cl, BadWindow, req->window, X_SetSelectionOwner, 0);
		return;
	}

	if (req->window)
	{
		atom->selectionownerwindowid = req->window;
		atom->selectionownerclient = cl;
	}
	else
	{
		atom->selectionownerwindowid = None;
		atom->selectionownerclient = NULL;
	}
}

void XR_ConvertSelection (xclient_t *cl, xReq *request)
{
	xConvertSelectionReq *req = (void*)request;
	xatom_t *atom = XS_GetResourceType(req->selection, x_atom);
	xEvent rep;

	if (atom && atom->selectionownerwindowid)
	{	//forward the request to the selection's owner
		rep.u.u.type = SelectionRequest;
		rep.u.u.detail = 0;
		rep.u.u.sequenceNumber = 0;
		rep.u.selectionRequest.time = req->time;
		rep.u.selectionRequest.owner = atom->selectionownerwindowid;
		rep.u.selectionRequest.target = req->target;
		rep.u.selectionRequest.property = req->property;
		rep.u.selectionRequest.requestor = req->requestor;
		rep.u.selectionRequest.selection = req->selection;
		X_SendData(atom->selectionownerclient, &rep, sizeof(rep));
	}
	else
	{	//return it back to the sender, or so
		rep.u.u.type = SelectionNotify;
		rep.u.u.detail = 0;
		rep.u.u.sequenceNumber = 0;
		rep.u.selectionNotify.time = req->time;
		rep.u.selectionNotify.target = req->target;
		rep.u.selectionNotify.property = req->property;
		rep.u.selectionNotify.requestor = req->requestor;
		rep.u.selectionNotify.selection = req->selection;
		X_SendData(cl, &rep, sizeof(rep));
	}
}


extern int x_windowwithcursor;

void XR_GetInputFocus (xclient_t *cl, xReq *request)
{
	xGetInputFocusReply rep;
	extern xwindow_t *xfocusedwindow;

    rep.type			= X_Reply;
    rep.revertTo		= None;
    rep.sequenceNumber	= cl->requestnum;
    rep.length			= 0;
    rep.focus			= xfocusedwindow?xfocusedwindow->res.id:None;
    rep.pad1			= 0;
    rep.pad2 			= 0;
    rep.pad3			= 0;
    rep.pad4			= 0;
    rep.pad5			= 0;


	X_SendData(cl, &rep, sizeof(rep));
}

void XR_SetInputFocus (xclient_t *cl, xReq *request)
{
	extern xwindow_t *xfocusedwindow;
	xResourceReq	*req = (xResourceReq *)request;
	xwindow_t *wnd;

	if (XS_GetResource(req->id, (void**)&wnd) != x_window)
	{
		X_SendError(cl, BadDrawable, req->id, X_SetInputFocus, 0);
		return;
	}
	
	xfocusedwindow = wnd;

	X_EvalutateFocus(NotifyWhileGrabbed);
}

void XR_QueryBestSize (xclient_t *cl, xReq *request)
{
	xQueryBestSizeReq	*req = (xQueryBestSizeReq	*)request;
	xQueryBestSizeReply rep;

	if (req->class == CursorShape && req->drawable != rootwindow->res.id)
	{
		X_SendError(cl, BadDrawable, req->drawable, X_QueryBestSize, req->class);
		return;
	}
	else if (req->class != CursorShape)
	{
		X_SendError(cl, BadImplementation, req->drawable, X_QueryBestSize, req->class);
		return;
	}

	rep.type			= X_Reply;
	rep.pad1			= 0;
	rep.sequenceNumber	= cl->requestnum;
	rep.length			= 0;
	rep.width			= req->width;
	rep.height			= req->height;
	rep.pad3			= 0;
	rep.pad4			= 0;
	rep.pad5			= 0;
	rep.pad6			= 0;
	rep.pad7			= 0;

	X_SendData(cl, &rep, sizeof(rep));
}

void XR_GetGeometry (xclient_t *cl, xReq *request)
{ 
	xResourceReq *req = (xResourceReq *)request;
	xGetGeometryReply	rep;
	xresource_t *drawable;

	xwindow_t *wnd;
	xpixmap_t *pm;

	rep.type			= X_Reply;
	rep.depth			= 24;
	rep.sequenceNumber	= cl->requestnum;
	rep.length			= 0;
	rep.root			= 0;
	rep.x				= 0;
	rep.y				= 0;
	rep.width			= 0;
	rep.height			= 0;
	rep.borderWidth		= 0;
	rep.pad1			= 0;
	rep.pad2			= 0;
	rep.pad3			= 0;

	switch(XS_GetResource(req->id, (void**)&drawable))
	{
	case x_window:
		wnd = (xwindow_t*)drawable;
		rep.x = wnd->xpos;
		rep.y = wnd->ypos;
		rep.borderWidth = 0;	//fixme
		rep.width = wnd->width;
		rep.height = wnd->height;
		rep.root = rootwindow->res.id;
		break;
	case x_pixmap:
		pm = (xpixmap_t*)drawable;
		rep.width = pm->width;
		rep.height = pm->height;
		break;
	default:
		X_SendError(cl, BadDrawable, req->id, X_GetGeometry, 0);
		return;
	}


	X_SendData(cl, &rep, sizeof(rep));
}

void XR_CreateWindow (xclient_t *cl, xReq *request)
{
	xCreateWindowReq *req = (xCreateWindowReq *)request;
	xwindow_t *parent;
	xwindow_t *wnd;
	CARD32 *parameters;

	if (req->class == InputOnly && req->depth != 0)
	{
		X_SendError(cl, BadMatch, req->wid, X_CreateWindow, 0);
		return;
	}
	if (XS_GetResource(req->wid, (void**)&parent) != x_none)
	{
		X_SendError(cl, BadIDChoice, req->wid, X_CreateWindow, 0);
		return;
	}

	if (XS_GetResource(req->parent, (void**)&parent) != x_window)
	{
		X_SendError(cl, BadWindow, req->parent, X_CreateWindow, 0);
		return;
	}

	wnd = XS_CreateWindow(req->wid, cl, parent, req->x, req->y, req->width, req->height);

	if (req->depth != 0)
		wnd->depth = req->depth;
	else
		wnd->depth = parent->depth;

	if (req->class == CopyFromParent)
		wnd->inputonly = parent->inputonly;
	else
		wnd->inputonly = (req->class == InputOnly);

	//FIXME: Depth must be valid
	//FIXME: visual id must be valid.

	parameters = (CARD32 *)(req+1);
	if (req->mask & CWBackPixmap)
	{
		wnd->backpixmap = NULL;
		if (XS_GetResource(*parameters, (void**)&wnd->backpixmap) != x_pixmap)
		{
			if (*parameters)
				X_SendError(cl, BadPixmap, *parameters, X_CreateWindow, 0);
		}
		else
			wnd->backpixmap->references++;
		parameters++;
	}
	if (req->mask & CWBackPixel)//
	{
		wnd->backpixel = *parameters;
		parameters++;
	}
	if (req->mask & CWBorderPixmap)
		parameters+=0;
	if (req->mask & CWBorderPixel)//
	{
		wnd->borderpixel = *parameters;
		parameters++;
	}
	if (req->mask & CWBitGravity)//
	{
		wnd->bitgravity = *parameters;
		parameters++;
	}
	if (req->mask & CWWinGravity)
		wnd->bitgravity = *parameters++;
	if (req->mask & CWBackingStore)
		parameters++;	//ignored
	if (req->mask & CWBackingPlanes)
		parameters+=0;
	if (req->mask & CWBackingPixel)
		parameters+=0;
	if (req->mask & CWOverrideRedirect)
		wnd->overrideredirect = *parameters++;
	else
		wnd->overrideredirect = false;
	if (req->mask & CWSaveUnder)
		parameters++;
	if (req->mask & CWEventMask)//
	{
		xnotificationmask_t *nm;
		nm = malloc(sizeof(xnotificationmask_t));
		nm->client = cl;
		nm->next = NULL;
		nm->mask = *parameters;
		wnd->notificationmask = nm;
		parameters++;

		wnd->notificationmasks = 0;
		for (nm = wnd->notificationmask; nm; nm = nm->next)
			wnd->notificationmasks |= nm->mask;
	}
	if (req->mask & CWDontPropagate)
		wnd->donotpropagate = *parameters++;
	if (req->mask & CWColormap)//
	{
		wnd->colormap = *parameters;
		parameters++;
	}
	if (req->mask & CWCursor)
		parameters++;

	#define CWBackPixmap		(1L<<0)
#define CWBackPixel		(1L<<1)
#define CWBorderPixmap		(1L<<2)
#define CWBorderPixel           (1L<<3)
#define CWBitGravity		(1L<<4)
#define CWWinGravity		(1L<<5)
#define CWBackingStore          (1L<<6)
#define CWBackingPlanes	        (1L<<7)
#define CWBackingPixel	        (1L<<8)
#define CWOverrideRedirect	(1L<<9)
#define CWSaveUnder		(1L<<10)
#define CWEventMask		(1L<<11)
#define CWDontPropagate	        (1L<<12)
#define CWColormap		(1L<<13)
#define CWCursor	        (1L<<14)
/*
    CARD8 depth;
    Window wid;
	Window parent;
    INT16 x B16, y B16;
    CARD16 width B16, height B16, borderWidth B16;  
#if defined(__cplusplus) || defined(c_plusplus)
    CARD16 c_class B16;
#else
    CARD16 class B16;
#endif
    VisualID visual B32;
    CARD32 mask B32;
*/

	if (wnd->inputonly)
		return;


	{
		xEvent ev;

		ev.u.u.type						= CreateNotify;
		ev.u.u.detail					= 0;
		ev.u.u.sequenceNumber			= 0;
		ev.u.createNotify.parent		= wnd->parent->res.id;
		ev.u.createNotify.window		= wnd->res.id;
		ev.u.createNotify.x				= wnd->xpos;
		ev.u.createNotify.y				= wnd->ypos;
		ev.u.createNotify.width			= wnd->width;
		ev.u.createNotify.height		= wnd->height; 
		ev.u.createNotify.borderWidth	= req->borderWidth;
		ev.u.createNotify.override		= wnd->overrideredirect;
		ev.u.createNotify.bpad			= 0;

		X_SendNotificationMasked (&ev, wnd, SubstructureNotifyMask);
	}

/*	{
		xEvent ev;

		ev.u.u.type						= MapRequest;
		ev.u.u.detail					= 0;
		ev.u.u.sequenceNumber			= 0;
		ev.u.mapRequest.window			= wnd->res.id;
		ev.u.mapRequest.parent			= wnd->parent->res.id;

		X_SendNotificationMasked(&ev, wnd, SubstructureRedirectMask);
	}*/
/*	{
		xEvent ev;

		ev.u.u.type						= GraphicsExpose;
		ev.u.u.detail					= 0;
		ev.u.u.sequenceNumber			= 0;
		ev.u.expose.window				= wnd->res.id;
		ev.u.expose.x					= 0;
		ev.u.expose.y					= 0;
		ev.u.expose.width				= wnd->width;
		ev.u.expose.height				= wnd->height;
		ev.u.expose.count				= 0;	//matching expose events after this one
		ev.u.expose.pad2				= 0;

		X_SendNotificationMasked(&ev, wnd, ExposureMask);
	}*/
}

void XR_ChangeWindowAttributes (xclient_t *cl, xReq *request)
{
	CARD32 *parameters;
	xChangeWindowAttributesReq *req = (xChangeWindowAttributesReq *)request;
	xwindow_t *wnd;

	if (XS_GetResource(req->window, (void**)&wnd) != x_window)
	{
		X_SendError(cl, BadWindow, req->window, X_ChangeWindowAttributes, 0);
		return;
	}

	parameters = (CARD32 *)(req+1);

	if (req->valueMask & CWBackPixmap)
	{
		if (wnd->backpixmap)
			wnd->backpixmap->references--;
		wnd->backpixmap = NULL;
		if (XS_GetResource(*parameters, (void**)&wnd->backpixmap) != x_pixmap)
		{
			if (*parameters)
				X_SendError(cl, BadPixmap, *parameters, X_ChangeWindowAttributes, 0);
		}
		else
			wnd->backpixmap->references++;
		parameters++;
	}

	if (req->valueMask & CWBackPixel)
	{
		if (wnd->backpixmap)
			wnd->backpixmap->references--;
		wnd->backpixmap = NULL;
		wnd->backpixel = *parameters++;
	}

	if (req->valueMask & CWBorderPixmap)
	{
		X_SendError(cl, BadImplementation, 0, X_ChangeWindowAttributes, 0);
/*		wnd->borderpixmap = NULL;
		if (XS_GetResource(*parameters, (void**)&wnd->borderpixmap) != x_pixmap)
		{
			if (*parameters)
				X_SendError(cl, BadPixmap, *parameters, X_ChangeWindowAttributes, 0);
		}
		else
			wnd->backpixmap->references++;
*/		parameters++;
	}

	if (req->valueMask & CWBorderPixel)
		wnd->borderpixel = *parameters++;
	
	if (req->valueMask & CWBitGravity)
	{
		wnd->bitgravity = *parameters++;
	}

	if (req->valueMask & CWWinGravity)
	{
		wnd->wingravity = *parameters++;
	}

	if (req->valueMask & CWBackingStore)
	{
//		X_SendError(cl, BadImplementation, 0, X_ChangeWindowAttributes, 0);
		parameters++;	//ignore
	}

	if (req->valueMask & CWBackingPlanes)
	{
		X_SendError(cl, BadImplementation, 0, X_ChangeWindowAttributes, 0);
		parameters++;
	}

	if (req->valueMask & CWBackingPixel)
	{
		X_SendError(cl, BadImplementation, 0, X_ChangeWindowAttributes, 0);
		parameters++;
	}

	if (req->valueMask & CWOverrideRedirect)
	{
		wnd->overrideredirect = *parameters++;
	}

	if (req->valueMask & CWSaveUnder)
	{
//		X_SendError(cl, BadImplementation, 0, X_ChangeWindowAttributes, 0);
		parameters++;
	}

	if (req->valueMask & CWEventMask)
	{
		xnotificationmask_t *nm;

		if (*parameters & (SubstructureRedirectMask | ResizeRedirectMask))
		{	//you're only allowed one client with that one at a time.
			for (nm = wnd->notificationmask; nm; nm = nm->next)
			{
				if (nm->mask & (*parameters))
					if (nm->client != cl)
						break;
			}
		}
		else
			nm = NULL;
		if (nm)	//client has this one.
			X_SendError(cl, BadAccess, *parameters, X_ChangeWindowAttributes, CWEventMask);
		else
		{
			for (nm = wnd->notificationmask; nm; nm = nm->next)
			{
				if (nm->client == cl)
					break;
			}
			if (!nm)
			{
				nm = malloc(sizeof(xnotificationmask_t));
				nm->next = wnd->notificationmask;
				wnd->notificationmask = nm;
				nm->client = cl;
			}
			nm->mask = *parameters;

			wnd->notificationmasks = 0;
			for (nm = wnd->notificationmask; nm; nm = nm->next)
				wnd->notificationmasks |= nm->mask;
		}
		parameters++;
	}

	if (req->valueMask & CWDontPropagate)
	{
		wnd->donotpropagate = *parameters++;
	}

	if (req->valueMask & CWColormap)
	{
		X_SendError(cl, BadImplementation, 0, X_ChangeWindowAttributes, 0);
		parameters++;
	}

	if (req->valueMask & CWCursor)
	{
//		X_SendError(cl, BadImplementation, 0, X_ChangeWindowAttributes, 0);
		parameters++;
	}

	xrefreshed=true;

	if (req->valueMask > CWCursor)	//anything else is an error on some implementation's part.
		X_SendError(cl, BadImplementation, 0, X_ChangeWindowAttributes, 0);

//	XW_ClearArea(wnd, 0, 0, wnd->width, wnd->height);
}

void XR_ConfigureWindow (xclient_t *cl, xReq *request)
{
	int newx, newy, neww, newh, sibid, newbw, stackmode;
	xConfigureWindowReq *req = (xConfigureWindowReq *)request;
	xwindow_t *wnd;

	CARD32 *parm;

	if (XS_GetResource(req->window, (void**)&wnd) != x_window)
	{
		X_SendError(cl, BadWindow, req->window, X_ConfigureWindow, 0);
		return;
	}

	if (!wnd->parent)	//root window.
	{	//can't resize this one.
		X_SendError(cl, BadWindow, req->window, X_ConfigureWindow, 0);
		return;
	}

	parm = (CARD32 *)(req+1);

    if (req->mask & CWX)
		newx = *parm++;
	else
		newx=wnd->xpos;
	if (req->mask & CWY)
		newy = *parm++;
	else
		newy=wnd->ypos;

	if (req->mask & CWWidth)
		neww = *parm++;
	else
		neww=wnd->width;

	if (wnd->width <= 0)
		wnd->width = 1;

	if (req->mask & CWHeight)
		newh = *parm++;
	else
		newh=wnd->height;

	if (req->mask & CWBorderWidth)
		newbw = *parm++;
	else
		newbw = 0;
	if (req->mask & CWSibling)
		sibid = *parm++;
	else
		sibid = None;
	if (req->mask & CWStackMode)
		stackmode = *parm++;
	else
		stackmode = Above;

	if (!wnd->overrideredirect && X_NotifcationMaskPresent(wnd, SubstructureRedirectMask, cl))
	{
		xEvent ev;

		ev.u.u.type							= ConfigureRequest;
		ev.u.u.detail						= stackmode;
		ev.u.u.sequenceNumber				= 0;
		ev.u.configureRequest.parent		= wnd->parent->res.id;
		ev.u.configureRequest.window		= wnd->res.id;
		ev.u.configureRequest.sibling		= sibid;
		ev.u.configureRequest.x				= newx;
		ev.u.configureRequest.y				= newy;
		ev.u.configureRequest.width			= neww;
		ev.u.configureRequest.height		= newh;
		ev.u.configureRequest.borderWidth	= newbw;
		ev.u.configureRequest.valueMask		= req->mask;
		ev.u.configureRequest.pad1			= 0;

		X_SendNotificationMasked(&ev, wnd, SubstructureRedirectMask);
	}
	else
		X_Resize(wnd, newx, newy, neww, newh);
}

void XR_ReparentWindow (xclient_t *cl, xReq *request)
{
	qboolean wasmapped;
	xEvent ev;
	xReparentWindowReq *req = (xReparentWindowReq *)request;
	xwindow_t *wnd, *parent;

	if (XS_GetResource(req->window, (void**)&wnd) != x_window)
	{
		X_SendError(cl, BadWindow, req->window, X_ReparentWindow, 0);
		return;
	}
	if (XS_GetResource(req->parent, (void**)&parent) != x_window)
	{
		X_SendError(cl, BadWindow, req->parent, X_ReparentWindow, 0);
		return;
	}

	if (wnd->mapped)
	{
		XR_UnmapWindow(cl, request);
		wasmapped = true;
	}
	else
		wasmapped = false;

	ev.u.u.type						= ReparentNotify;
	ev.u.u.detail					= 0;
	ev.u.reparent.override = wnd->overrideredirect;
	ev.u.reparent.window = wnd->res.id;
	ev.u.reparent.parent = wnd->parent->res.id;
	ev.u.reparent.x = req->x;
	ev.u.reparent.y = req->y;

	X_SendNotificationMasked (&ev, wnd, SubstructureNotifyMask);

	XS_SetParent(wnd, parent);
	wnd->xpos = req->x;
	wnd->ypos = req->y;

	X_SendNotificationMasked (&ev, wnd, SubstructureNotifyMask);	//and again, now that we have the new parent.

	ev.u.reparent.event = wnd->res.id;
	X_SendNotificationMasked (&ev, wnd, StructureNotifyMask);

	if (wasmapped)
		XR_MapWindow(cl, request);
}

void XR_DestroyWindow (xclient_t *cl, xReq *request)
{
	xResourceReq *req = (xResourceReq *)request;
	xwindow_t *wnd;

	if (XS_GetResource(req->id, (void**)&wnd) != x_window)
	{
		X_SendError(cl, BadWindow, req->id, X_DestroyWindow, 0);
		return;
	}
	if (!wnd->res.owner)	//root window.
		return;
	XS_DestroyResource(&wnd->res);
}
void XR_QueryTree (xclient_t *cl, xReq *request)
{
	char buffer[8192];
	xResourceReq *req = (xResourceReq *)request;
	xQueryTreeReply *rep = (xQueryTreeReply*)buffer;

	xwindow_t *wnd;

	Window	*cwnd;


	if (XS_GetResource(req->id, (void**)&wnd) != x_window)
	{
		X_SendError(cl, BadWindow, req->id, X_DestroyWindow, 0);
		return;
	}

	//FIXME: be careful of the count of children overflowing buffer.

	rep->type	= X_Reply;
	rep->pad1	= 0;
	rep->sequenceNumber	= cl->requestnum;
	rep->length	= 0;
	rep->root	= rootwindow->res.id;	//we only have one root
	if (wnd->parent)
		rep->parent	= wnd->parent->res.id;
	else
		rep->parent	= 0;
	rep->nChildren	= 0;
	rep->pad2	= 0;
	rep->pad3	= 0;
	rep->pad4	= 0;
	rep->pad5	= 0;

	cwnd = (Window*)(rep+1);

	for (wnd = wnd->child ; wnd ; wnd = wnd->sibling)
	{
		*cwnd++ = wnd->res.id;
		rep->nChildren++;
	}

	rep->length = rep->nChildren;

	X_SendData(cl, rep, sizeof(*rep)+rep->length*4);
}

void XR_GetWindowAttributes (xclient_t *cl, xReq *request)
{
	xnotificationmask_t *nm;
	xResourceReq *req = (xResourceReq *)request;
	xwindow_t *wnd;

	xGetWindowAttributesReply rep;

	if (XS_GetResource(req->id, (void**)&wnd) != x_window)
	{
		X_SendError(cl, BadWindow, req->id, X_GetWindowAttributes, 0);
		return;
	}

	rep.type				= X_Reply;
	rep.backingStore		= 2;
	rep.sequenceNumber		= cl->requestnum;
	rep.visualID			= 0x22;
	rep.class				= wnd->inputonly;
	rep.bitGravity			= wnd->bitgravity;
	rep.winGravity			= wnd->wingravity;
	rep.backingBitPlanes	= wnd->depth;
	rep.backingPixel		= wnd->backpixel;
	rep.saveUnder			= 1;
	rep.mapInstalled		= !!wnd->buffer;
	rep.mapState			= wnd->mapped*2;
	rep.override			= wnd->overrideredirect;
	rep.colormap			= wnd->colormap;
	rep.yourEventMask		= 0;
	rep.allEventMasks		= 0;
	for (nm = wnd->notificationmask; nm; nm = nm->next)
	{
		if (nm->client == cl)
			rep.yourEventMask = nm->mask;
		rep.allEventMasks |= nm->mask;
	}
	rep.doNotPropagateMask	= wnd->donotpropagate;
	rep.pad					= 0;

	rep.length = (sizeof(xGetWindowAttributesReply) - sizeof(xGenericReply) + 3)/4;

	X_SendData(cl, &rep, sizeof(xGetWindowAttributesReply));
}


static struct
{
	KeySym keysym[8];
} keyboardmapping[256] =
{
	{{0}},
	{{0}},
	{{0}},
	{{0}},
	{{0}},
	{{0}},
	{{0}},
	{{0}},
	{{0}},
	{{XK_Escape, NoSymbol, XK_Escape}},	//10
	{{XK_1, XK_exclam, XK_1, XK_exclam, XK_onesuperior, XK_exclamdown, XK_onesuperior}},	//10	//11
	{{XK_2, XK_quotedbl, XK_2, XK_quotedbl, XK_twosuperior}},//, XK_oneeighth, XK_twosuperior}},	//12
	{{XK_3, XK_sterling, XK_3, XK_sterling, XK_threesuperior, XK_sterling, XK_threesuperior}},	//13
	{{XK_4, XK_dollar, XK_4, XK_dollar}},//, XK_EuroSign, XK_onequarter, XK_EuroSign}},	//14
	{{XK_5, XK_percent, XK_5, XK_percent, XK_onehalf}},//, XK_threeeighths, XK_onehalf}},	//15
	{{XK_6, XK_asciicircum, XK_6, XK_asciicircum, XK_threequarters}},//, XK_fiveeighths, XK_threequarters}},	//16
	{{XK_7, XK_ampersand, XK_7, XK_ampersand, XK_braceleft}},//, XK_seveneighths, XK_braceleft}},	//17
	{{XK_8, XK_asterisk, XK_8, XK_asterisk, XK_bracketleft}},//, XK_trademark, XK_bracketleft}},	//18
	{{XK_9, XK_parenleft, XK_9, XK_parenleft, XK_bracketright, XK_plusminus, XK_bracketright}},	//19
    {{XK_0, XK_parenright, XK_0, XK_parenright, XK_braceright, XK_degree, XK_braceright}},	//10
    {{XK_minus, XK_underscore, XK_minus, XK_underscore, XK_backslash, XK_questiondown, XK_backslash}},	//20
    {{XK_equal, XK_plus, XK_equal, XK_plus}},//, XK_dead_cedilla, XK_dead_ogonek, XK_dead_cedilla}},	//21
    {{XK_BackSpace, XK_BackSpace, XK_BackSpace, XK_BackSpace}},	//22
    {{XK_Tab}},//, XK_ISO_Left_Tab, XK_Tab, XK_ISO_Left_Tab}},	//23
    {{XK_q, XK_Q, XK_q, XK_Q, XK_at}},//, XK_Greek_OMEGA, XK_at}},	//24
    {{XK_w, XK_W, XK_w, XK_W}},//, XK_lstroke, XK_Lstroke, XK_lstroke}},	//25
    {{XK_e, XK_E, XK_e, XK_E, XK_e, XK_E, XK_e}},	//26
    {{XK_r, XK_R, XK_r, XK_R, XK_paragraph, XK_registered, XK_paragraph}},	//27
    {{XK_t, XK_T, XK_t, XK_T}},//, XK_tslash, XK_Tslash, XK_tslash}},	//28
    {{XK_y, XK_Y, XK_y, XK_Y}},//, XK_leftarrow, XK_yen, XK_leftarrow}},	//29
     {{XK_u, XK_U, XK_u, XK_U}},//, XK_downarrow, XK_uparrow, XK_downarrow}},	//30
     {{XK_i, XK_I, XK_i, XK_I}},//, XK_rightarrow, XK_idotless, XK_rightarrow}},	//31
     {{XK_o, XK_O, XK_o, XK_O, XK_oslash, XK_Oslash, XK_oslash}},	//32
     {{XK_p, XK_P, XK_p, XK_P, XK_thorn, XK_THORN, XK_thorn}},	//33
     {{XK_bracketleft, XK_braceleft, XK_bracketleft, XK_braceleft}},//, XK_dead_diaeresis, XK_dead_abovering, XK_dead_diaeresis}},	//34
     {{XK_bracketright, XK_braceright, XK_bracketright, XK_braceright}},//, XK_dead_tilde, XK_dead_macron, XK_dead_tilde}},	//35
     {{XK_Return, NoSymbol, XK_Return}},	//36
     {{XK_Control_L, NoSymbol, XK_Control_L}},	//37
     {{XK_a, XK_A, XK_a, XK_A, XK_ae, XK_AE, XK_ae}},	//38
     {{XK_s, XK_S, XK_s, XK_S, XK_ssharp, XK_section, XK_ssharp}},	//39
     {{XK_d, XK_D, XK_d, XK_D, XK_eth, XK_ETH, XK_eth}},	//40
     {{XK_f, XK_F, XK_f, XK_F}},//, XK_dstroke, XK_ordfeminine, XK_dstroke}},	//41
     {{XK_g, XK_G, XK_g, XK_G}},//, XK_eng, XK_ENG, XK_eng}},	//42
     {{XK_h, XK_H, XK_h, XK_H}},//, XK_hstroke, XK_Hstroke, XK_hstroke}},	//43
     {{XK_j, XK_J, XK_j, XK_J}},//, XK_dead_hook, XK_dead_horn, XK_dead_hook}},	//44
     {{XK_k, XK_K, XK_k, XK_K}},//, XK_kra, XK_ampersand, XK_kra}},	//45
     {{XK_l, XK_L, XK_l, XK_L}},//, XK_lstroke, XK_Lstroke, XK_lstroke}},	//46
     {{XK_semicolon, XK_colon, XK_semicolon, XK_colon}},//, XK_dead_acute, XK_dead_doubleacute, XK_dead_acute}},	//47
     {{XK_apostrophe, XK_at, XK_apostrophe, XK_at}},//, XK_dead_circumflex, XK_dead_caron, XK_dead_circumflex}},	//48
     {{XK_grave, XK_notsign, XK_grave, XK_notsign, XK_bar, XK_bar, XK_bar}},	//49
     {{XK_Shift_L, NoSymbol, XK_Shift_L}},	//50
     {{XK_numbersign, XK_asciitilde, XK_numbersign, XK_asciitilde}},//, XK_dead_grave, XK_dead_breve, XK_dead_grave}},	//51
     {{XK_z, XK_Z, XK_z, XK_Z, XK_guillemotleft, XK_less, XK_guillemotleft}},	//52
     {{XK_x, XK_X, XK_x, XK_X, XK_guillemotright, XK_greater, XK_guillemotright}},	//53
     {{XK_c, XK_C, XK_c, XK_C, XK_cent, XK_copyright, XK_cent}},	//54
     {{XK_v, XK_V, XK_v, XK_V}},//, XK_leftdoublequotemark, XK_leftsinglequotemark, XK_leftdoublequotemark}},	//55
     {{XK_b, XK_B, XK_b, XK_B}},//, XK_rightdoublequotemark, XK_rightsinglequotemark, XK_rightdoublequotemark}},	//56
     {{XK_n, XK_N, XK_n, XK_N, XK_n, XK_N, XK_n}},	//57
     {{XK_m, XK_M, XK_m, XK_M, XK_mu, XK_masculine, XK_mu}},	//58
     {{XK_comma, XK_less, XK_comma, XK_less}},//, XK_horizconnector, XK_multiply, XK_horizconnector}},	//59
     {{XK_period, XK_greater, XK_period, XK_greater, XK_periodcentered, XK_division, XK_periodcentered}},	//60
     {{XK_slash, XK_question, XK_slash, XK_question}},//, XK_dead_belowdot, XK_dead_abovedot, XK_dead_belowdot}},	//61
     {{XK_Shift_R, NoSymbol, XK_Shift_R}},	//62
     {{XK_KP_Multiply, XK_KP_Multiply, XK_KP_Multiply, XK_KP_Multiply, XK_KP_Multiply, XK_KP_Multiply}},//, XK_XF86ClearGrab}},	//63
     {{XK_Alt_L, XK_Meta_L, XK_Alt_L, XK_Meta_L}},	//64
     {{XK_space, NoSymbol, XK_space}},	//65
     {{XK_Caps_Lock, NoSymbol, XK_Caps_Lock}},	//66
     {{XK_F1, XK_F1, XK_F1, XK_F1, XK_F1, XK_F1}},//, XK_XF86Switch_VT_1}},	//67
     {{XK_F2, XK_F2, XK_F2, XK_F2, XK_F2, XK_F2}},//, XK_XF86Switch_VT_2}},	//68
     {{XK_F3, XK_F3, XK_F3, XK_F3, XK_F3, XK_F3}},//, XK_XF86Switch_VT_3}},	//69
     {{XK_F4, XK_F4, XK_F4, XK_F4, XK_F4, XK_F4}},//, XK_XF86Switch_VT_4}},	//70
     {{XK_F5, XK_F5, XK_F5, XK_F5, XK_F5, XK_F5}},//, XK_XF86Switch_VT_5}},	//71
     {{XK_F6, XK_F6, XK_F6, XK_F6, XK_F6, XK_F6}},//, XK_XF86Switch_VT_6}},	//72
     {{XK_F7, XK_F7, XK_F7, XK_F7, XK_F7, XK_F7}},//, XK_XF86Switch_VT_7}},	//73
     {{XK_F8, XK_F8, XK_F8, XK_F8, XK_F8, XK_F8}},//, XK_XF86Switch_VT_8}},	//74
     {{XK_F9, XK_F9, XK_F9, XK_F9, XK_F9, XK_F9}},//, XK_XF86Switch_VT_9}},	//75
     {{XK_F10, XK_F10, XK_F10, XK_F10, XK_F10, XK_F10}},//, XK_XF86Switch_VT_10}},	//76
     {{XK_Num_Lock, NoSymbol, XK_Num_Lock}},	//77
     {{XK_Scroll_Lock, NoSymbol, XK_Scroll_Lock}},	//78
     {{XK_KP_Home, XK_KP_7, XK_KP_Home, XK_KP_7}},	//79
     {{XK_KP_Up, XK_KP_8, XK_KP_Up, XK_KP_8}},	//80
     {{XK_KP_Prior, XK_KP_9, XK_KP_Prior, XK_KP_9}},	//81
     {{XK_KP_Subtract, XK_KP_Subtract, XK_KP_Subtract, XK_KP_Subtract, XK_KP_Subtract, XK_KP_Subtract}},//, XK_XF86Prev_VMode}},	//82
     {{XK_KP_Left, XK_KP_4, XK_KP_Left, XK_KP_4}},	//83
     {{XK_KP_Begin, XK_KP_5, XK_KP_Begin, XK_KP_5}},	//84
     {{XK_KP_Right, XK_KP_6, XK_KP_Right, XK_KP_6}},	//85
     {{XK_KP_Add, XK_KP_Add, XK_KP_Add, XK_KP_Add, XK_KP_Add, XK_KP_Add}},//, XK_XF86Next_VMode}},	//86
     {{XK_KP_End, XK_KP_1, XK_KP_End, XK_KP_1}},	//87
     {{XK_KP_Down, XK_KP_2, XK_KP_Down, XK_KP_2}},	//88
     {{XK_KP_Next, XK_KP_3, XK_KP_Next, XK_KP_3}},	//89
     {{XK_KP_Insert, XK_KP_0, XK_KP_Insert, XK_KP_0}},	//90
     {{XK_KP_Delete, XK_KP_Decimal, XK_KP_Delete, XK_KP_Decimal}},	//91
     {{0}},//XK_ISO_Level3_Shift, NoSymbol, XK_ISO_Level3_Shift}},	//92
     {{0}}, //93
     {{XK_backslash, XK_bar, XK_backslash, XK_bar, XK_bar, XK_brokenbar, XK_bar}},	//94
     {{XK_F11, XK_F11, XK_F11, XK_F11, XK_F11, XK_F11}},//, XK_XF86Switch_VT_11}},	//95
     {{XK_F12, XK_F12, XK_F12, XK_F12, XK_F12, XK_F12}},//, XK_XF86Switch_VT_12}},	//96
     {{0}},	//97
     {{XK_Katakana, NoSymbol, XK_Katakana}},	//98
     {{XK_Hiragana, NoSymbol, XK_Hiragana}},	//99
    {{XK_Henkan_Mode, NoSymbol, XK_Henkan_Mode}},	//100
    {{XK_Hiragana_Katakana, NoSymbol, XK_Hiragana_Katakana}},	//101
    {{XK_Muhenkan, NoSymbol, XK_Muhenkan}},	//102
    {{0}}, //103
    {{XK_KP_Enter, NoSymbol, XK_KP_Enter}},	//104
    {{XK_Control_R, NoSymbol, XK_Control_R}},	//105
    {{XK_KP_Divide, XK_KP_Divide, XK_KP_Divide, XK_KP_Divide, XK_KP_Divide, XK_KP_Divide}},//, XK_XF86Ungrab}},	//106
    {{XK_Print, XK_Sys_Req, XK_Print, XK_Sys_Req}},	//107
    {{0}},//XK_ISO_Level3_Shift, XK_Multi_key, XK_ISO_Level3_Shift, XK_Multi_key}},	//108
    {{XK_Linefeed, NoSymbol, XK_Linefeed}},	//109
    {{XK_Home, NoSymbol, XK_Home}},	//110
    {{XK_Up, NoSymbol, XK_Up}},	//111
    {{XK_Prior, NoSymbol, XK_Prior}},	//112
    {{XK_Left, NoSymbol, XK_Left}},	//113
    {{XK_Right, NoSymbol, XK_Right}},	//114
    {{XK_End, NoSymbol, XK_End}},	//115
    {{XK_Down, NoSymbol, XK_Down}},	//116
    {{XK_Next, NoSymbol, XK_Next}},	//117
    {{XK_Insert, NoSymbol, XK_Insert}},	//118
    {{XK_Delete, NoSymbol, XK_Delete}},	//119
/*
    120
    121         0x1008ff12 (XF86AudioMute)      0x0000 (NoSymbol)       0x1008ff12 (XF86AudioMute)
    122         0x1008ff11 (XF86AudioLowerVolume)       0x0000 (NoSymbol)       0x1008ff11 (XF86AudioLowerVolume)
    123         0x1008ff13 (XF86AudioRaiseVolume)       0x0000 (NoSymbol)       0x1008ff13 (XF86AudioRaiseVolume)
    124         0x1008ff2a (XF86PowerOff)       0x0000 (NoSymbol)       0x1008ff2a (XF86PowerOff)
    125         0xffbd (KP_Equal)       0x0000 (NoSymbol)       0xffbd (KP_Equal)
    126         0x00b1 (plusminus)      0x0000 (NoSymbol)       0x00b1 (plusminus)
    127         0xff13 (Pause)  0xff6b (Break)  0xff13 (Pause)  0xff6b (Break)
    128         0x1008ff4a (XF86LaunchA)        0x0000 (NoSymbol)       0x1008ff4a (XF86LaunchA)
    129         0xffae (KP_Decimal)     0xffae (KP_Decimal)     0xffae (KP_Decimal)     0xffae (KP_Decimal)
    130         0xff31 (Hangul) 0x0000 (NoSymbol)       0xff31 (Hangul)
    131         0xff34 (Hangul_Hanja)   0x0000 (NoSymbol)       0xff34 (Hangul_Hanja)
    132
    133         0xffeb (Super_L)        0x0000 (NoSymbol)       0xffeb (Super_L)
    134         0xffec (Super_R)        0x0000 (NoSymbol)       0xffec (Super_R)
    135         0xff67 (Menu)   0x0000 (NoSymbol)       0xff67 (Menu)
    136         0xff69 (Cancel) 0x0000 (NoSymbol)       0xff69 (Cancel)
    137         0xff66 (Redo)   0x0000 (NoSymbol)       0xff66 (Redo)
    138         0x1005ff70 (SunProps)   0x0000 (NoSymbol)       0x1005ff70 (SunProps)
    139         0xff65 (Undo)   0x0000 (NoSymbol)       0xff65 (Undo)
    140         0x1005ff71 (SunFront)   0x0000 (NoSymbol)       0x1005ff71 (SunFront)
    141         0x1008ff57 (XF86Copy)   0x0000 (NoSymbol)       0x1008ff57 (XF86Copy)
    142         0x1008ff6b (XF86Open)   0x0000 (NoSymbol)       0x1008ff6b (XF86Open)
    143         0x1008ff6d (XF86Paste)  0x0000 (NoSymbol)       0x1008ff6d (XF86Paste)
    144         0xff68 (Find)   0x0000 (NoSymbol)       0xff68 (Find)
    145         0x1008ff58 (XF86Cut)    0x0000 (NoSymbol)       0x1008ff58 (XF86Cut)
    146         0xff6a (Help)   0x0000 (NoSymbol)       0xff6a (Help)
    147         0x1008ff65 (XF86MenuKB) 0x0000 (NoSymbol)       0x1008ff65 (XF86MenuKB)
    148         0x1008ff1d (XF86Calculator)     0x0000 (NoSymbol)       0x1008ff1d (XF86Calculator)
    149
    150         0x1008ff2f (XF86Sleep)  0x0000 (NoSymbol)       0x1008ff2f (XF86Sleep)
    151         0x1008ff2b (XF86WakeUp) 0x0000 (NoSymbol)       0x1008ff2b (XF86WakeUp)
    152         0x1008ff5d (XF86Explorer)       0x0000 (NoSymbol)       0x1008ff5d (XF86Explorer)
    153         0x1008ff7b (XF86Send)   0x0000 (NoSymbol)       0x1008ff7b (XF86Send)
    154
    155         0x1008ff8a (XF86Xfer)   0x0000 (NoSymbol)       0x1008ff8a (XF86Xfer)
    156         0x1008ff41 (XF86Launch1)        0x0000 (NoSymbol)       0x1008ff41 (XF86Launch1)
    157         0x1008ff42 (XF86Launch2)        0x0000 (NoSymbol)       0x1008ff42 (XF86Launch2)
    158         0x1008ff2e (XF86WWW)    0x0000 (NoSymbol)       0x1008ff2e (XF86WWW)
    159         0x1008ff5a (XF86DOS)    0x0000 (NoSymbol)       0x1008ff5a (XF86DOS)
    160         0x1008ff2d (XF86ScreenSaver)    0x0000 (NoSymbol)       0x1008ff2d (XF86ScreenSaver)
    161         0x1008ff74 (XF86RotateWindows)  0x0000 (NoSymbol)       0x1008ff74 (XF86RotateWindows)
    162         0x1008ff7f (XF86TaskPane)       0x0000 (NoSymbol)       0x1008ff7f (XF86TaskPane)
    163         0x1008ff19 (XF86Mail)   0x0000 (NoSymbol)       0x1008ff19 (XF86Mail)
    164         0x1008ff30 (XF86Favorites)      0x0000 (NoSymbol)       0x1008ff30 (XF86Favorites)
    165         0x1008ff33 (XF86MyComputer)     0x0000 (NoSymbol)       0x1008ff33 (XF86MyComputer)
    166         0x1008ff26 (XF86Back)   0x0000 (NoSymbol)       0x1008ff26 (XF86Back)
    167         0x1008ff27 (XF86Forward)        0x0000 (NoSymbol)       0x1008ff27 (XF86Forward)
    168
    169         0x1008ff2c (XF86Eject)  0x0000 (NoSymbol)       0x1008ff2c (XF86Eject)
    170         0x1008ff2c (XF86Eject)  0x1008ff2c (XF86Eject)  0x1008ff2c (XF86Eject)  0x1008ff2c (XF86Eject)
    171         0x1008ff17 (XF86AudioNext)      0x0000 (NoSymbol)       0x1008ff17 (XF86AudioNext)
    172         0x1008ff14 (XF86AudioPlay)      0x1008ff31 (XF86AudioPause)     0x1008ff14 (XF86AudioPlay)      0x1008ff31 (XF86AudioPause)
    173         0x1008ff16 (XF86AudioPrev)      0x0000 (NoSymbol)       0x1008ff16 (XF86AudioPrev)
    174         0x1008ff15 (XF86AudioStop)      0x1008ff2c (XF86Eject)  0x1008ff15 (XF86AudioStop)      0x1008ff2c (XF86Eject)
    175         0x1008ff1c (XF86AudioRecord)    0x0000 (NoSymbol)       0x1008ff1c (XF86AudioRecord)
    176         0x1008ff3e (XF86AudioRewind)    0x0000 (NoSymbol)       0x1008ff3e (XF86AudioRewind)
    177         0x1008ff6e (XF86Phone)  0x0000 (NoSymbol)       0x1008ff6e (XF86Phone)
    178
    179         0x1008ff81 (XF86Tools)  0x0000 (NoSymbol)       0x1008ff81 (XF86Tools)
    180         0x1008ff18 (XF86HomePage)       0x0000 (NoSymbol)       0x1008ff18 (XF86HomePage)
    181         0x1008ff73 (XF86Reload) 0x0000 (NoSymbol)       0x1008ff73 (XF86Reload)
    182         0x1008ff56 (XF86Close)  0x0000 (NoSymbol)       0x1008ff56 (XF86Close)
    183
    184
    185         0x1008ff78 (XF86ScrollUp)       0x0000 (NoSymbol)       0x1008ff78 (XF86ScrollUp)
    186         0x1008ff79 (XF86ScrollDown)     0x0000 (NoSymbol)       0x1008ff79 (XF86ScrollDown)
    187         0x0028 (parenleft)      0x0000 (NoSymbol)       0x0028 (parenleft)
    188         0x0029 (parenright)     0x0000 (NoSymbol)       0x0029 (parenright)
    189         0x1008ff68 (XF86New)    0x0000 (NoSymbol)       0x1008ff68 (XF86New)
    190         0xff66 (Redo)   0x0000 (NoSymbol)       0xff66 (Redo)
    191         0x1008ff81 (XF86Tools)  0x0000 (NoSymbol)       0x1008ff81 (XF86Tools)
    192         0x1008ff45 (XF86Launch5)        0x0000 (NoSymbol)       0x1008ff45 (XF86Launch5)
    193         0x1008ff46 (XF86Launch6)        0x0000 (NoSymbol)       0x1008ff46 (XF86Launch6)
    194         0x1008ff47 (XF86Launch7)        0x0000 (NoSymbol)       0x1008ff47 (XF86Launch7)
    195         0x1008ff48 (XF86Launch8)        0x0000 (NoSymbol)       0x1008ff48 (XF86Launch8)
    196         0x1008ff49 (XF86Launch9)        0x0000 (NoSymbol)       0x1008ff49 (XF86Launch9)
    197
    198         0x1008ffb2 (XF86AudioMicMute)   0x0000 (NoSymbol)       0x1008ffb2 (XF86AudioMicMute)
    199         0x1008ffa9 (XF86TouchpadToggle) 0x0000 (NoSymbol)       0x1008ffa9 (XF86TouchpadToggle)
    200         0x1008ffb0 (XF86TouchpadOn)     0x0000 (NoSymbol)       0x1008ffb0 (XF86TouchpadOn)
    201         0x1008ffb1 (XF86TouchpadOff)    0x0000 (NoSymbol)       0x1008ffb1 (XF86TouchpadOff)
    202
    203         0xff7e (Mode_switch)    0x0000 (NoSymbol)       0xff7e (Mode_switch)
    204         0x0000 (NoSymbol)       0xffe9 (Alt_L)  0x0000 (NoSymbol)       0xffe9 (Alt_L)
    205         0x0000 (NoSymbol)       0xffe7 (Meta_L) 0x0000 (NoSymbol)       0xffe7 (Meta_L)
    206         0x0000 (NoSymbol)       0xffeb (Super_L)        0x0000 (NoSymbol)       0xffeb (Super_L)
    207         0x0000 (NoSymbol)       0xffed (Hyper_L)        0x0000 (NoSymbol)       0xffed (Hyper_L)
    208         0x1008ff14 (XF86AudioPlay)      0x0000 (NoSymbol)       0x1008ff14 (XF86AudioPlay)
    209         0x1008ff31 (XF86AudioPause)     0x0000 (NoSymbol)       0x1008ff31 (XF86AudioPause)
    210         0x1008ff43 (XF86Launch3)        0x0000 (NoSymbol)       0x1008ff43 (XF86Launch3)
    211         0x1008ff44 (XF86Launch4)        0x0000 (NoSymbol)       0x1008ff44 (XF86Launch4)
    212         0x1008ff4b (XF86LaunchB)        0x0000 (NoSymbol)       0x1008ff4b (XF86LaunchB)
    213         0x1008ffa7 (XF86Suspend)        0x0000 (NoSymbol)       0x1008ffa7 (XF86Suspend)
    214         0x1008ff56 (XF86Close)  0x0000 (NoSymbol)       0x1008ff56 (XF86Close)
    215         0x1008ff14 (XF86AudioPlay)      0x0000 (NoSymbol)       0x1008ff14 (XF86AudioPlay)
    216         0x1008ff97 (XF86AudioForward)   0x0000 (NoSymbol)       0x1008ff97 (XF86AudioForward)
    217
    218         0xff61 (Print)  0x0000 (NoSymbol)       0xff61 (Print)
    219
    220         0x1008ff8f (XF86WebCam) 0x0000 (NoSymbol)       0x1008ff8f (XF86WebCam)
    221
    222
    223         0x1008ff19 (XF86Mail)   0x0000 (NoSymbol)       0x1008ff19 (XF86Mail)
    224         0x1008ff8e (XF86Messenger)      0x0000 (NoSymbol)       0x1008ff8e (XF86Messenger)
    225         0x1008ff1b (XF86Search) 0x0000 (NoSymbol)       0x1008ff1b (XF86Search)
    226         0x1008ff5f (XF86Go)     0x0000 (NoSymbol)       0x1008ff5f (XF86Go)
    227         0x1008ff3c (XF86Finance)        0x0000 (NoSymbol)       0x1008ff3c (XF86Finance)
    228         0x1008ff5e (XF86Game)   0x0000 (NoSymbol)       0x1008ff5e (XF86Game)
    229         0x1008ff36 (XF86Shop)   0x0000 (NoSymbol)       0x1008ff36 (XF86Shop)
    230
    231         0xff69 (Cancel) 0x0000 (NoSymbol)       0xff69 (Cancel)
    232         0x1008ff03 (XF86MonBrightnessDown)      0x0000 (NoSymbol)       0x1008ff03 (XF86MonBrightnessDown)
    233         0x1008ff02 (XF86MonBrightnessUp)        0x0000 (NoSymbol)       0x1008ff02 (XF86MonBrightnessUp)
    234         0x1008ff32 (XF86AudioMedia)     0x0000 (NoSymbol)       0x1008ff32 (XF86AudioMedia)
    235         0x1008ff59 (XF86Display)        0x0000 (NoSymbol)       0x1008ff59 (XF86Display)
    236         0x1008ff04 (XF86KbdLightOnOff)  0x0000 (NoSymbol)       0x1008ff04 (XF86KbdLightOnOff)
    237         0x1008ff06 (XF86KbdBrightnessDown)      0x0000 (NoSymbol)       0x1008ff06 (XF86KbdBrightnessDown)
    238         0x1008ff05 (XF86KbdBrightnessUp)        0x0000 (NoSymbol)       0x1008ff05 (XF86KbdBrightnessUp)
    239         0x1008ff7b (XF86Send)   0x0000 (NoSymbol)       0x1008ff7b (XF86Send)
    240         0x1008ff72 (XF86Reply)  0x0000 (NoSymbol)       0x1008ff72 (XF86Reply)
    241         0x1008ff90 (XF86MailForward)    0x0000 (NoSymbol)       0x1008ff90 (XF86MailForward)
    242         0x1008ff77 (XF86Save)   0x0000 (NoSymbol)       0x1008ff77 (XF86Save)
    243         0x1008ff5b (XF86Documents)      0x0000 (NoSymbol)       0x1008ff5b (XF86Documents)
    244         0x1008ff93 (XF86Battery)        0x0000 (NoSymbol)       0x1008ff93 (XF86Battery)
    245         0x1008ff94 (XF86Bluetooth)      0x0000 (NoSymbol)       0x1008ff94 (XF86Bluetooth)
    246         0x1008ff95 (XF86WLAN)   0x0000 (NoSymbol)       0x1008ff95 (XF86WLAN)
    247
    248
    249
    250
    251
  */
};

void XR_GetKeyboardMapping (xclient_t *cl, xReq *request)
{//fixme: send the XK equivelents.
	xGetKeyboardMappingReq *req = (xGetKeyboardMappingReq *)request;
	char buffer[8192];
	xGetKeyboardMappingReply *rep = (xGetKeyboardMappingReply *)buffer;
	int i, y, x;
	int *syms = ((int *)(rep+1));

	rep->type				= X_Reply;
	rep->keySymsPerKeyCode	= countof(keyboardmapping[0].keysym);
	rep->sequenceNumber		= cl->requestnum;
	rep->length				= 0;
	rep->pad2				= 0;
	rep->pad3				= 0;
	rep->pad4				= 0;
	rep->pad5				= 0;
	rep->pad6				= 0;
	rep->pad7				= 0;

	for (i = 0; i < req->count; i++)
	{
		y = req->firstKeyCode+i;
		if (y >= countof(keyboardmapping))
			break;
		for (x = 0; x < rep->keySymsPerKeyCode; x++)
			*syms++ = keyboardmapping[y].keysym[x];
	}
	rep->length = i*rep->keySymsPerKeyCode;

	X_SendData(cl, rep, sizeof(*rep)+rep->length*4);
}

static struct
{
	KEYCODE keysym[8];
} modifiermapping[] =
{	//these are scancodes
	{{0x32, 0x32}},
	{{0x42}},
	{{0x24, 0x69}},
	{{0x40, 0xcd}},
	{{0x4d}},
	{{0}},
	{{0x85, 0x86, 0xce, 0xcf}},
	{{0x5c, 0xcb}},
};
void XR_GetModifierMapping (xclient_t *cl, xReq *request)
{//fixme: send the XK equivelents.
//	xReq *req = (xReq *)request;
	char buffer[8192];
	xGetModifierMappingReply *rep = (xGetModifierMappingReply *)buffer;
	int x, y;
	KEYCODE *syms = ((KEYCODE *)(rep+1));

	rep->type				= X_Reply;
	rep->numKeyPerModifier	= countof(modifiermapping[0].keysym);
	rep->sequenceNumber		= cl->requestnum;
	rep->length				= (8*rep->numKeyPerModifier * sizeof(KEYCODE) + 3)/4;
	rep->pad2				= 0;
	rep->pad3				= 0;
	rep->pad4				= 0;
	rep->pad5				= 0;
	rep->pad6				= 0;

	for (y = 0; y < countof(modifiermapping); y++)
	{
		for (x = 0; x < rep->numKeyPerModifier; x++)
		{
			*syms++ = modifiermapping[y].keysym[x];
		}
	}

	X_SendData(cl, rep, sizeof(*rep)+rep->length*4);
}

void XR_QueryPointer (xclient_t *cl, xReq *request)
{
	extern int x_mousex, x_mousey, x_mousestate;
	xQueryPointerReply rep;

	rep.type			= X_Reply;
	rep.sameScreen		= 1;
	rep.sequenceNumber	= cl->requestnum;
	rep.length	= 0;
	rep.root	= rootwindow->res.id;
	rep.child	= rootwindow->res.id;
	rep.rootX	= x_mousex;
	rep.rootY	= x_mousey;
	rep.winX	= x_mousex;
	rep.winY	= x_mousey;
	rep.mask	= 0;
	rep.pad1	= 0;
	rep.pad		= 0;

	if ((x_mousestate) & Button1Mask)
		rep.mask |= Button1MotionMask;
	if ((x_mousestate) & Button2Mask)
		rep.mask |= Button2MotionMask;
	if ((x_mousestate) & Button3Mask)
		rep.mask |= Button3MotionMask;
	if ((x_mousestate) & Button4Mask)
		rep.mask |= Button4MotionMask;
	if ((x_mousestate) & Button5Mask)
		rep.mask |= Button5MotionMask;

	X_SendData(cl, &rep, sizeof(rep));
}

void XR_CreateCursor (xclient_t *cl, xReq *request)
{
//	xCreateCursorReq *req = (xCreateCursorReq *)request;

	//	X_SendError(cl, BadImplementation, 0, req->reqType, 0);
}
void XR_CreateGlyphCursor (xclient_t *cl, xReq *request)
{
//	xCreateGlyphCursorReq *req = (xCreateGlyphCursorReq *)request;

//	char buffer[8192];
//	xGetKeyboardMappingReply *rep = (xGetKeyboardMappingReply *)buffer;

//	X_SendError(cl, BadImplementation, 0, req->reqType, 0);

//	X_SendError(cl, BadAlloc, req->id, X_DestroyWindow, 0);
//	X_SendError(cl, BadFont, req->id, X_DestroyWindow, 0);
//	X_SendError(cl, BadValue, req->id, X_DestroyWindow, 0);
}
void XR_FreeCursor (xclient_t *cl, xReq *request)
{
//	X_SendError(cl, BadImplementation, 0, req->reqType, 0);
//	X_SendError(cl, BadValue, req->id, X_DestroyWindow, 0);
}
void XR_RecolorCursor (xclient_t *cl, xReq *request)
{
}

void XR_GrabButton (xclient_t *cl, xReq *request)
{
}
void XR_UngrabButton (xclient_t *cl, xReq *request)
{
}

void XR_ChangeGCInternal(unsigned int mask, xgcontext_t *gc, CARD32 *param)
{
	if (mask & GCFunction)
		gc->function = *param++;
	if (mask & GCPlaneMask)
		param++;
	if (mask & GCForeground)
		gc->fgcolour = *param++;
	if (mask & GCBackground)
		gc->bgcolour = *param++;
	if (mask & GCLineWidth)
		param++;
	if (mask & GCLineStyle)
		param++;
	if (mask & GCCapStyle)
		param++;
	if (mask & GCJoinStyle)
		param++;
	if (mask & GCFillStyle)
		param++;
	if (mask & GCFillRule)
		param++;
	if (mask & GCTile)
		param++;
	if (mask & GCStipple)
		param++;
	if (mask & GCTileStipXOrigin)
		param++;
	if (mask & GCTileStipYOrigin)
		param++;
	if (mask & GCFont)
	{
		void *font = NULL;
		if (XS_GetResource(*param++, &font) != x_font)
			font = NULL;
		gc->font = font;
	}
	if (mask & GCSubwindowMode)
		param++;
	if (mask & GCGraphicsExposures)
		param++;
	if (mask & GCClipXOrigin)
		param++;
	if (mask & GCClipYOrigin)
		param++;
	if (mask & GCClipMask)
		param++;
	if (mask & GCDashOffset)
		param++;
	if (mask & GCDashList)
		param++;
	if (mask & GCArcMode)
		param++;
}
void XR_CopyGCInternal(unsigned int mask, xgcontext_t *dest, xgcontext_t *src)
{
	int param=0;
	if (mask & GCFunction)
		dest->function = src->function;
	if (mask & GCPlaneMask)
		param++;
	if (mask & GCForeground)
		dest->fgcolour = src->fgcolour;
	if (mask & GCBackground)
		dest->bgcolour = src->fgcolour;
	if (mask & GCLineWidth)
		param++;
	if (mask & GCLineStyle)
		param++;
	if (mask & GCCapStyle)
		param++;
	if (mask & GCJoinStyle)
		param++;
	if (mask & GCFillStyle)
		param++;
	if (mask & GCFillRule)
		param++;
	if (mask & GCTile)
		param++;
	if (mask & GCStipple)
		param++;
	if (mask & GCTileStipXOrigin)
		param++;
	if (mask & GCTileStipYOrigin)
		param++;
	if (mask & GCFont)
	{
		dest->font = src->font;
	}
	if (mask & GCSubwindowMode)
		param++;
	if (mask & GCGraphicsExposures)
		param++;
	if (mask & GCClipXOrigin)
		param++;
	if (mask & GCClipYOrigin)
		param++;
	if (mask & GCClipMask)
		param++;
	if (mask & GCDashOffset)
		param++;
	if (mask & GCDashList)
		param++;
	if (mask & GCArcMode)
		param++;
}
void XR_ChangeGC(xclient_t *cl, xReq *request)
{
	xChangeGCReq *req = (xChangeGCReq *)request;
	xgcontext_t *gc;

	if (XS_GetResource(req->gc, (void**)&gc) != x_gcontext)
	{
		X_SendError(cl, BadGC, req->gc, X_FreeGC, 0);
		return;
	}

	XR_ChangeGCInternal(req->mask, gc, (CARD32	*)(req + 1));
}

void XR_CopyGC(xclient_t *cl, xReq *request)
{
	xCopyGCReq *req = (xCopyGCReq *)request;
	xgcontext_t *dest, *src;

	if (XS_GetResource(req->dstGC, (void**)&dest) != x_gcontext)
	{
		X_SendError(cl, BadGC, req->dstGC, X_FreeGC, 0);
		return;
	}
	if (XS_GetResource(req->srcGC, (void**)&src) != x_gcontext)
	{
		X_SendError(cl, BadGC, req->srcGC, X_FreeGC, 0);
		return;
	}

	XR_CopyGCInternal(req->mask, dest, src);
}

void XR_CreateGC(xclient_t *cl, xReq *request)
{
	xCreateGCReq *req = (xCreateGCReq *)request;
	xresource_t *drawable;

	if (XS_GetResource(req->gc, (void**)&drawable) != x_none)
	{
//		if (req->gc == cl->ridbase&&drawable->owner)
//			XS_DestroyResourcesOfClient(drawable->owner);
//		else
		{
			X_SendError(cl, BadIDChoice, req->gc, X_CreateGC, 0);
			return;
		}
	}
	XS_GetResource(req->drawable, (void**)&drawable);
	/*if (drawable->restype != x_window && drawable->restype != x_gcontext)
	{
		X_SendError(cl, BadDrawable, req->drawable, X_CreateGC, 0);
		return;
	}*/

	XR_ChangeGCInternal(req->mask, XS_CreateGContext(req->gc, cl, drawable), (CARD32	*)(req + 1));
}

void XR_FreeGC(xclient_t *cl, xReq *request)
{
	xResourceReq *req = (xResourceReq *)request;
	xresource_t *gc;
	if (XS_GetResource(req->id, (void**)&gc) != x_gcontext)
	{
		X_SendError(cl, BadGC, req->id, X_FreeGC, 0);
		return;
	}

	XS_DestroyResource(gc);
}

void XW_ClearArea(xwindow_t *wnd, int xp, int yp, int width, int height)
{
	if (!wnd->buffer)
	{
		if (wnd->width*wnd->height<=0)
			wnd->buffer = malloc(1);
		else
			wnd->buffer = malloc(wnd->width*wnd->height*4);
	}

	if (xp < 0)
	{
		width += xp;
		xp = 0;
	}
	if (xp>wnd->width)
		xp = wnd->width;
	if (yp < 0)
	{
		height += yp;
		xp = 0;
	}
	if (yp>wnd->height)
		yp = wnd->height;
	if (width+xp > wnd->width)
		width = wnd->width - xp;
	if (height+yp > wnd->height)
		height = wnd->height - yp;

	if (wnd->backpixmap && wnd->backpixmap->width && wnd->backpixmap->height)
	{
		int x, xs;
		int y, ys;
		unsigned int *out;
		unsigned int *in;

		out = (unsigned int *)wnd->buffer + xp +yp*wnd->width;
		in = (unsigned int *)wnd->backpixmap->data;

		for (y = 0, ys = 0; y < height; y++)
		{
			for (x = 0; x < width; x+=wnd->backpixmap->width)
			{
				//when do we stop?
				xs = wnd->backpixmap->width;
				if (xs > wnd->width-x-1)
					xs = wnd->width-x-1;
				for (; xs > 0; xs--)
				{
					out[x+xs] = in[xs+ys*wnd->backpixmap->width];
				}
			}
			out += wnd->width;
			ys++;
			if (ys >= wnd->backpixmap->height)
				ys = 0;
		}
	}
	else
	{
		int x;
		int y;
		unsigned int *out;

		out = (unsigned int *)wnd->buffer + xp +yp*wnd->width;

		for (y = yp; y < height; y++)
		{
			for (x = xp; x < width; x++)
			{
				out[x] = wnd->backpixel;
			}
			out+=wnd->width;
		}
	}
}

void XW_CopyArea(unsigned int *dest, int dx, int dy, int dwidth, int dheight, unsigned int *source, int sx, int sy, int swidth, int sheight, int cwidth, int cheight, xgcontext_t *gc)
{
	int x, y;

	//tlcap on dest
	if (dx < 0)
	{
		cwidth += dx;
		dx = 0;
	}
	if (dy < 0)
	{
		cheight += dy;
		dy = 0;
	}

	//tlcap on source
	if (sx < 0)
	{
		cwidth += sx;
		sx = 0;
	}
	if (sy < 0)
	{
		cheight += sy;
		sy = 0;
	}

	//brcap on dest
	if (cwidth > dwidth - dx)
		cwidth = dwidth - dx;

	if (cheight > dheight - dy)
		cheight = dheight - dy;

	//brcap on source
	if (cwidth > swidth - sx)
		cwidth = swidth - sx;

	if (cheight > sheight - sy)
		cheight = sheight - sy;

	if (cwidth<=0)
		return;
	if (cheight<=0)
		return;

	dest += dx+dy*dwidth;
	source += sx+sy*swidth;

	for (y = 0; y < cheight; y++)
	{
		for (x = 0; x < cwidth;x++)
		{
			GCFunc(gc->fgcolour, dest[x], gc->function, source[x], 0xffffff);
		}
		dest += dwidth;
		source += swidth;
	}
}

void XR_ClearArea(xclient_t *cl, xReq *request)
{//FIXME: Should be area rather than entire window
	xClearAreaReq *req = (xClearAreaReq *)request;
	xwindow_t *wnd;

	if (XS_GetResource(req->window, (void**)&wnd) != x_window)
	{
		X_SendError(cl, BadWindow, req->window, X_ClearArea, 0);
		return;
	}

	if (req->x < 0)
	{
		if (req->width)
			req->width += req->x;
		req->x = 0;
	}
	if (req->y < 0)
	{
		if (req->height)
			req->height += req->y;
		req->y = 0;
	}

	if (!req->width || req->width + req->x > wnd->width)
	{
		if (req->width)
			req->width = wnd->width - req->x;
		req->width = wnd->width - req->x;
	}
	if (!req->height || req->height + req->y > wnd->height)
	{
		if (req->height)
			req->height = wnd->height - req->y;
		req->height = wnd->height - req->y;
	}

	XW_ClearArea(wnd, req->x, req->y, req->width, req->height);

	if (req->exposures)
		XW_ExposeWindowRegionInternal(wnd, req->x, req->y, req->width, req->height);

	xrefreshed=true;
}

void XR_CopyArea(xclient_t *cl, xReq *request)	//from and to pixmap or drawable.
{
	xCopyAreaReq *req = (xCopyAreaReq *)request;

	xresource_t *drawable;
	xgcontext_t *gc;


	unsigned int *outbuffer;
	unsigned int *inbuffer;
	int inwidth;
	int inheight;
	int outwidth;
	int outheight;

	if (XS_GetResource(req->gc, (void**)&gc) == x_none)
	{
		X_SendError(cl, BadGC, req->gc, X_CopyArea, 0);
		return;
	}


	switch (XS_GetResource(req->srcDrawable, (void**)&drawable))
	{
	default:
		X_SendError(cl, BadDrawable, req->srcDrawable, X_CopyArea, 0);
		return;

	case x_window:
		{
			xwindow_t *wnd;
			wnd = (xwindow_t *)drawable;
			if (!wnd->buffer)
			{
				wnd->buffer = malloc(wnd->width*wnd->height*4);
				XW_ClearArea(wnd, 0, 0, wnd->width, wnd->height);
			}

			inwidth = wnd->width;
			inheight = wnd->height;
			inbuffer = (unsigned int *)wnd->buffer;
		}
		break;

	case x_pixmap:
		{
			xpixmap_t *pm;
			pm = (xpixmap_t *)drawable;
			if (!pm->data)
			{
				pm->data = malloc(pm->width*pm->height*4);
				memset(pm->data, rand(), pm->width*pm->height*4);
			}

			inwidth = pm->width;
			inheight = pm->height;
			inbuffer = (unsigned int *)pm->data;
		}
		break;
	}

	switch (XS_GetResource(req->dstDrawable, (void**)&drawable))
	{
	default:
		X_SendError(cl, BadDrawable, req->dstDrawable, X_CopyArea, 0);
		return;

	case x_window:
		{
			xwindow_t *wnd;
			wnd = (xwindow_t *)drawable;
			if (!wnd->buffer)
			{
				wnd->buffer = malloc(wnd->width*wnd->height*4);
				XW_ClearArea(wnd, 0, 0, wnd->width, wnd->height);
			}

			outwidth = wnd->width;
			outheight = wnd->height;
			outbuffer = (unsigned int *)wnd->buffer;
		}
		break;

	case x_pixmap:
		{
			xpixmap_t *pm;
			pm = (xpixmap_t *)drawable;
			if (!pm->data)
			{
				pm->data = malloc(pm->width*pm->height*4);
				memset(pm->data, rand(), pm->width*pm->height*4);
			}

			outwidth = pm->width;
			outheight = pm->height;
			outbuffer = (unsigned int *)pm->data;
		}
		break;
	}

	XW_CopyArea(outbuffer, req->dstX, req->dstY, outwidth, outheight, inbuffer, req->srcX, req->srcY, inwidth, inheight, req->width, req->height, gc);

	xrefreshed=true;
}

void XR_MapWindow(xclient_t *cl, xReq *request)
{
	xResourceReq *req = (xResourceReq *)request;

	xwindow_t *wnd;
	if (XS_GetResource(req->id, (void**)&wnd) != x_window)
	{
		X_SendError(cl, BadWindow, req->id, X_MapWindow, 0);
		return;
	}

//	if (wnd->mapped)
//		return;

	if (!wnd->overrideredirect && X_NotifcationMaskPresent(wnd, SubstructureRedirectMask, cl))
	{
		xEvent ev;

		ev.u.u.type						= MapRequest;
		ev.u.u.detail					= 0;
		ev.u.u.sequenceNumber			= 0;
		ev.u.mapRequest.parent			= wnd->parent->res.id;
		ev.u.mapRequest.window			= wnd->res.id;

		X_SendNotificationMasked(&ev, wnd, SubstructureRedirectMask);

		return;
	}

	if (!wnd->buffer)
		XW_ClearArea(wnd, 0, 0, wnd->width, wnd->height);

	wnd->mapped = true;

	{
		xEvent ev;

		ev.u.u.type						= MapNotify;
		ev.u.u.detail					= 0;
		ev.u.u.sequenceNumber			= 0;
		ev.u.mapNotify.event			= wnd->res.id;
		ev.u.mapNotify.window			= wnd->res.id;
		ev.u.mapNotify.override			= wnd->overrideredirect;
		ev.u.mapNotify.pad1				= 0;
		ev.u.mapNotify.pad2				= 0;
		ev.u.mapNotify.pad3				= 0;

		X_SendNotificationMasked(&ev, wnd, StructureNotifyMask);
		X_SendNotificationMasked(&ev, wnd, SubstructureNotifyMask);
	}

/*	{
		xEvent ev;

		ev.u.u.type						= GraphicsExpose;
		ev.u.u.detail					= 0;
		ev.u.u.sequenceNumber			= 0;
		ev.u.mapNotify.window			= wnd->res.id;
		ev.u.mapNotify.override			= false;
		ev.u.mapNotify.pad1				= 0;
		ev.u.mapNotify.pad2				= 0;
		ev.u.mapNotify.pad3				= 0;

		X_SendNotificationMasked(&ev, wnd, ExposureMask);
	}*/

	XW_ExposeWindowRegionInternal(wnd, 0, 0, wnd->width, wnd->height);
	/*

	while(wnd->mapped)
	{
		xEvent ev;

		ev.u.u.type						= VisibilityNotify;
		ev.u.u.detail					= 0;
		ev.u.u.sequenceNumber			= 0;
		ev.u.visibility.window			= wnd->res.id;
		ev.u.visibility.state			= 0;
		ev.u.visibility.pad1			= 0;
		ev.u.visibility.pad2			= 0;
		ev.u.visibility.pad3			= 0;

		X_SendNotificationMasked(&ev, wnd, VisibilityChangeMask);
	}

	{
		xEvent ev;

		ev.u.u.type						= Expose;
		ev.u.u.detail					= 0;
		ev.u.u.sequenceNumber			= 0;
		ev.u.expose.window				= wnd->res.id;
		ev.u.expose.x					= 0;
		ev.u.expose.y					= 0;
		ev.u.expose.width				= wnd->width;
		ev.u.expose.height				= wnd->height;
		ev.u.expose.count				= false;	//other expose events following
		ev.u.expose.pad2				= 0;

		X_SendNotificationMasked(&ev, wnd, ExposureMask);
	}*/

	xrefreshed = true;
}

void XR_UnmapWindow(xclient_t *cl, xReq *request)
{
	xResourceReq *req = (xResourceReq *)request;

	xwindow_t *wnd;
	if (XS_GetResource(req->id, (void**)&wnd) != x_window)
	{
		X_SendError(cl, BadWindow, req->id, X_UnmapWindow, 0);
		return;
	}

	if (!wnd->mapped || !wnd->parent)
		return;

	wnd->mapped = false;
	xrefreshed=true;

	XW_ExposeWindow(wnd, 0, 0, wnd->width, wnd->height);

	{
		xEvent ev;

		ev.u.u.type						= UnmapNotify;
		ev.u.u.detail					= 0;
		ev.u.u.sequenceNumber			= 0;
		ev.u.unmapNotify.event			= wnd->res.id;
		ev.u.unmapNotify.window			= wnd->res.id;
		ev.u.unmapNotify.fromConfigure	= 0;
		ev.u.unmapNotify.pad1			= 0;
		ev.u.unmapNotify.pad2			= 0;
		ev.u.unmapNotify.pad3			= 0;

		X_SendNotificationMasked(&ev, wnd, StructureNotifyMask);
		X_SendNotificationMasked(&ev, wnd, SubstructureNotifyMask);
	}
}

void XR_MapSubwindows(xclient_t *cl, xReq *request)
{
	xResourceReq *req = (xResourceReq *)request;
	xwindow_t *wnd;

	if (XS_GetResource(req->id, (void**)&wnd) != x_window)
	{
		X_SendError(cl, BadWindow, req->id, X_MapWindow, 0);
		return;
	}

	for (wnd = wnd->child; wnd; wnd = wnd->sibling)
	{
		req->id = wnd->res.id;
		XR_MapWindow(cl, request);
	}
}

void XR_CreatePixmap(xclient_t *cl, xReq *request)
{
	xCreatePixmapReq *req = (xCreatePixmapReq *)request;

	xresource_t *res;
	xpixmap_t *newpix;
	
	switch(XS_GetResource(req->drawable, (void**)&res))
	{
	case x_window:
	case x_pixmap:
		break;
	default:
		X_SendError(cl, BadDrawable, req->drawable, X_CreatePixmap, 0);
		return;
	}

	//depth must be one of the depths supported by the drawable's root window
	if (req->depth != 24 && req->depth != 32)
	{
//		X_SendError(cl, BadValue, req->depth, X_CreatePixmap, 0);
//		return;
	}

	if (XS_GetResource(req->pid, (void**)&newpix) != x_none)
	{
		X_SendError(cl, BadIDChoice, req->pid, X_CreatePixmap, 0);
	}
	XS_CreatePixmap(req->pid, cl, req->width, req->height, req->depth);
}

void XR_FreePixmap(xclient_t *cl, xReq *request)
{
	xResourceReq *req = (xResourceReq *)request;
	xresource_t *pm;
	if (XS_GetResource(req->id, (void**)&pm) != x_pixmap)
	{
		X_SendError(cl, BadPixmap, req->id, X_FreePixmap, 0);
		return;
	}

	XS_DestroyResource(pm);
}

void XR_PutImage(xclient_t *cl, xReq *request)
{
	unsigned char *out;
	unsigned char *in;
	xPutImageReq *req = (xPutImageReq *)request;
	xresource_t *drawable;
	xgcontext_t *gc;
	int i;

	int drwidth;
	int drheight;
	int drdepth;
	unsigned char *drbuffer;

	if (XS_GetResource(req->drawable, (void**)&drawable) == x_none)
	{
		X_SendError(cl, BadDrawable, req->drawable, X_PutImage, 0);
		return;
	}

	if (XS_GetResource(req->gc, (void**)&gc) == x_none)
	{
		X_SendError(cl, BadGC, req->gc, X_PutImage, 0);
		return;
	}


	if (drawable->restype == x_window)
	{
		xwindow_t *wnd;
		wnd = (xwindow_t *)drawable;
		if (!wnd->buffer)
		{
			wnd->buffer = malloc(wnd->width*wnd->height*4);
			XW_ClearArea(wnd, 0, 0, wnd->width, wnd->height);
		}

		drwidth = wnd->width;
		drheight = wnd->height;
		drbuffer = wnd->buffer;
		drdepth = wnd->depth;
	}
	else if (drawable->restype == x_pixmap)
	{
		xpixmap_t *pm;
		pm = (xpixmap_t *)drawable;
		if (!pm->data)
		{
			pm->data = malloc(pm->width*pm->height*4);
			memset(pm->data, rand(), pm->width*pm->height*4);
		}

		drwidth = pm->width;
		drheight = pm->height;
		drbuffer = pm->data;
		drdepth = pm->depth;
	}
	else
	{
		X_SendError(cl, BadDrawable, req->drawable, X_PutImage, 0);
		return;
	}

	xrefreshed = true;

	if (req->dstX < 0)
	{
		req->width += req->dstX;
		req->dstX = 0;
	}
	if (req->dstY < 0)
	{
		req->height += req->dstY;
		req->dstY = 0;
	}

	if (req->width > drwidth - req->dstX)
		req->width = drwidth - req->dstX;

	if (req->height > drheight - req->dstY)
		req->height = drheight - req->dstY;

	in = (qbyte *)(req+1);

	//my own bugs...
	if (req->leftPad != 0)
	{
		X_SendError(cl, BadImplementation, req->drawable, X_PutImage, req->format);
		return;
	}

	if (req->format == XYBitmap)
	{	//bitmaps are just a 'mask' specifying which pixels get foreground and which get background.
		int bnum;
		unsigned int *o4;
		bnum=0;
		if (req->depth == 1)
		{
			while(req->height)
			{
				bnum += req->leftPad;

				out = drbuffer + (req->dstX + req->dstY*drwidth)*4;
				o4 = (unsigned int*)out;
				for (i = 0; i < req->width; i++)
				{
					if (in[bnum>>8]&(1<<(bnum&7)))
						o4[i] = gc->fgcolour;
					else
						o4[i] = gc->bgcolour;
					bnum++;
				}
				bnum += req->width;

				req->height--;
				req->dstY++;
			}
		}
		else
			X_SendError(cl, BadMatch, req->drawable, X_PutImage, 0);
	}
	else if (req->depth != drdepth)	/*depth must match*/
	{
		X_SendError(cl, BadMatch, req->drawable, X_PutImage, 0);
	}
	else if (req->format == ZPixmap)	//32 bit network bandwidth (hideous)
	{
		if (req->leftPad != 0)
		{
			X_SendError(cl, BadMatch, req->drawable, X_PutImage, 0);
			return;
		}

		if (req->depth == 1)
		{
			unsigned int *o4;
			int bnum = 0;
			while(req->height)
			{
				out = drbuffer + (req->dstX + req->dstY*drwidth)*4;
				o4 = (unsigned int*)out;
				for (i = 0; i < req->width; i++, bnum++)
				{
					if (in[bnum>>8]&(1<<(bnum&7)))
						o4[i] = 0xffffff;
					else
						o4[i] = 0;
				}

				req->height--;
				req->dstY++;
			}
		}
		else
		{
			while(req->height)
			{
				unsigned int *i4, *o4;
				out = drbuffer + (req->dstX + req->dstY*drwidth)*4;
				i4 = (unsigned int*)in;
				o4 = (unsigned int*)out;
				for (i = 0; i < req->width; i++)
				{
					o4[i] = i4[i];
				}
	/*			for (i = 0; i < req->width; i++)
				{
					out[i*4+0] = in[i*4+0];
					out[i*4+1] = in[i*4+1];
					out[i*4+2] = in[i*4+2];
				}
	*/			in += req->width*4;

				req->height--;
				req->dstY++;
			}
		}
	}
	else if (req->format == XYPixmap)
	{
		while(req->height)
		{
			unsigned int *o4;
			out = drbuffer + (req->dstX + req->dstY*drwidth)*4;
			o4 = (unsigned int*)out;
			for (i = 0; i < req->width; i++)
			{
				if (in[i>>3] & (1u<<(i&7)))
					o4[i] = rand();
				else
					o4[i] = rand();
			}
			in += (req->width+7)/8;

			req->height--;
			req->dstY++;
		}
	}
	else
	{
		X_SendError(cl, BadImplementation, req->drawable, X_PutImage, req->format);
	}
}
void XR_GetImage(xclient_t *cl, xReq *request)
{
	unsigned char *out, *data;
	unsigned char *in;
	xGetImageReq *req = (xGetImageReq *)request;
	xresource_t *drawable;

	xGetImageReply rep;

	int drwidth;
	int drheight;
	unsigned char *drbuffer;

	if (XS_GetResource(req->drawable, (void**)&drawable) == x_none)
	{
		X_SendError(cl, BadDrawable, req->drawable, X_GetImage, 0);
		return;
	}


	if (drawable->restype == x_window)
	{
		xwindow_t *wnd;
		wnd = (xwindow_t *)drawable;
		if (!wnd->buffer)
		{
			wnd->buffer = malloc(wnd->width*wnd->height*4);
			XW_ClearArea(wnd, 0, 0, wnd->width, wnd->height);
		}

		drwidth = wnd->width;
		drheight = wnd->height;
		drbuffer = wnd->buffer;

		rep.visual			= 0x22;
	}
	else if (drawable->restype == x_pixmap)
	{
		xpixmap_t *pm;
		pm = (xpixmap_t *)drawable;
		if (!pm->data)
		{
			pm->data = malloc(pm->width*pm->height*4);
			memset(pm->data, rand(), pm->width*pm->height*4);
		}

		drwidth = pm->width;
		drheight = pm->height;
		drbuffer = pm->data;

		rep.visual			= 0;
	}
	else
	{
		X_SendError(cl, BadDrawable, req->drawable, X_GetImage, 0);
		return;
	}

	if (req->x < 0)
	{
		X_SendError(cl, BadValue, req->drawable, X_GetImage, 0);
		return;
	}
	if (req->y < 0)
	{
		X_SendError(cl, BadValue, req->drawable, X_GetImage, 0);
		return;
	}

	if (req->width > drwidth - req->x)
	{
		X_SendError(cl, BadValue, req->drawable, X_GetImage, 0);
		return;
	}

	if (req->height > drheight - req->y)
	{
		X_SendError(cl, BadValue, req->drawable, X_GetImage, 0);
		return;
	}

	data = out = alloca(req->width*4*req->height);
	if (req->format == 2)	//32 bit network bandwidth (hideous)
	{
		while(req->height)
		{
			in = drbuffer + (req->x + req->y*drwidth)*4;
#if 1
			memcpy(out, in, req->width*4);
#else
			for (i = 0; i < req->width; i++)
			{
				out[i*4+0] = in[i*4+0];
				out[i*4+1] = in[i*4+1];
				out[i*4+2] = in[i*4+2];
			}
#endif
			out += req->width*4;

			req->height--;
			req->y++;
		}
	}
	else
	{
		X_SendError(cl, BadImplementation, req->drawable, X_GetImage, req->format);
		return;
	}

	rep.type			= X_Reply;
	rep.sequenceNumber	= cl->requestnum;
	rep.length			= (out-data+3)/4;
	rep.depth			= 24;
	rep.pad3			= 0;
	rep.pad4			= 0;
	rep.pad5			= 0;
	rep.pad6			= 0;
	rep.pad7			= 0;

	X_SendData(cl, &rep, sizeof(rep));
	X_SendData(cl, data, rep.length*4);
}

void XW_PolyLine(unsigned int *dbuffer, int dwidth, int dheight, int x1, int x2, int y1, int y2, xgcontext_t *gc)
{
	//FIXME: cap to region.
	int len;

	int dx, dy;

	if (x1 < 0)
		return;
	if (x2 < 0)
		return;
	if (y1 < 0)
		return;
	if (y2 < 0)
		return;

	if (x1 >= dwidth)
		x1 = dwidth-1;
	if (x2 >= dwidth)
		x2 = dwidth-1;

	if (y1 >= dheight)
		y1 = dheight-1;
	if (y2 >= dheight)
		y2 = dheight-1;

	dx = (x2 - x1);
	dy = (y2 - y1);
	len = (int)sqrt(dx*dx+dy*dy);

	if (!len)
		return;

	x1<<=16;
	y1<<=16;
	dx=(dx<<16)/len;
	dy=(dy<<16)/len;

	for (; len ; len--)
	{
		GCFunc(gc->fgcolour, dbuffer[(x1>>16)+dwidth*(y1>>16)], gc->function, dbuffer[(x1>>16)+dwidth*(y1>>16)], 0xffffff);
		x1+=dx;
		y1+=dy;
	}
}

void XR_PolyLine(xclient_t *cl, xReq *request)
{
	xPolyLineReq *req = (xPolyLineReq *)request;

	xresource_t *drawable;
	xgcontext_t *gc;

	int pointsleft;

	int drwidth;
	int drheight;
	unsigned char *drbuffer;
	INT16 start[2], end[2];

	INT16 *points;
	points = (INT16 *)(req+1);



	if (XS_GetResource(req->drawable, (void**)&drawable) == x_none)
	{
		X_SendError(cl, BadDrawable, req->drawable, X_PolyRectangle, 0);
		return;
	}

	if (XS_GetResource(req->gc, (void**)&gc) == x_none)
	{
		X_SendError(cl, BadGC, req->gc, X_PolyRectangle, 0);
		return;
	}

	if (drawable->restype == x_window)
	{
		xwindow_t *wnd;
		wnd = (xwindow_t *)drawable;
		if (!wnd->buffer)
		{
			wnd->buffer = malloc(wnd->width*wnd->height*4);
			XW_ClearArea(wnd, 0, 0, wnd->width, wnd->height);
		}

		drwidth = wnd->width;
		drheight = wnd->height;
		drbuffer = wnd->buffer;
	}
	else if (drawable->restype == x_pixmap)
	{
		xpixmap_t *pm;
		pm = (xpixmap_t *)drawable;
		if (!pm->data)
		{
			pm->data = malloc(pm->width*pm->height*4);
			memset(pm->data, rand(), pm->width*pm->height*4);
		}

		drwidth = pm->width;
		drheight = pm->height;
		drbuffer = pm->data;
	}
	else
	{
		X_SendError(cl, BadDrawable, req->drawable, X_PolyRectangle, 0);
		return;
	}

	xrefreshed = true;

	if (req->reqType == X_PolySegment)
	{
		for (pointsleft = req->length/2-3; pointsleft>0; pointsleft--)
		{
			XW_PolyLine((unsigned int *)drbuffer, drwidth, drheight, points[0], points[2], points[1], points[3], gc);
			points+=4;
		}
	}
	else
	{
		end[0] = 0;
		end[1] = 0;

		for (pointsleft = req->length-3; pointsleft>0; pointsleft-=2)
		{
			if (req->coordMode == 1/*Previous*/)
			{
				start[0] = end[0] + points[0];
				start[1] = end[1] + points[1];
				end[0] = start[0] + points[2];
				end[1] = start[1] + points[3];
			}
			else
			{
				start[0] = points[0];
				start[1] = points[1];
				end[0] = points[2];
				end[1] = points[3];
			}
			XW_PolyLine((unsigned int *)drbuffer, drwidth, drheight, start[0], end[0], start[1], end[1], gc);
			points+=4;
		}
	}
}

void XR_FillPoly(xclient_t *cl, xReq *request)
{
	xFillPolyReq *req = (xFillPolyReq *)request;
	INT16 *points = (INT16*)(req+1);
	int numpoints = req->length-4;

	xresource_t *drawable;
	xgcontext_t *gc;

	int drwidth;
	int drheight;
	unsigned char *drbuffer;
	INT16 start[2], end[2];

	points = (INT16 *)(req+1);

	

	if (XS_GetResource(req->drawable, (void**)&drawable) == x_none)
	{
		X_SendError(cl, BadDrawable, req->drawable, X_PolyRectangle, 0);
		return;
	}

	if (XS_GetResource(req->gc, (void**)&gc) == x_none)
	{
		X_SendError(cl, BadGC, req->gc, X_PolyRectangle, 0);
		return;
	}

	if (drawable->restype == x_window)
	{
		xwindow_t *wnd;
		wnd = (xwindow_t *)drawable;
		if (!wnd->buffer)
		{
			wnd->buffer = malloc(wnd->width*wnd->height*4);
			XW_ClearArea(wnd, 0, 0, wnd->width, wnd->height);
		}

		drwidth = wnd->width;
		drheight = wnd->height;
		drbuffer = wnd->buffer;
	}
	else if (drawable->restype == x_pixmap)
	{
		xpixmap_t *pm;
		pm = (xpixmap_t *)drawable;
		if (!pm->data)
		{
			pm->data = malloc(pm->width*pm->height*4);
			memset(pm->data, rand(), pm->width*pm->height*4);
		}

		drwidth = pm->width;
		drheight = pm->height;
		drbuffer = pm->data;
	}
	else
	{
		X_SendError(cl, BadDrawable, req->drawable, X_PolyRectangle, 0);
		return;
	}

	xrefreshed = true;



	{
		start[0] = points[(numpoints*2)-2];
		start[1] = points[(numpoints*2)-1];
		while (numpoints-->0)
		{
			if (req->coordMode == 1/*Previous*/)
			{
				end[0] = start[0] + points[0];
				end[1] = start[1] + points[1];
			}
			else
			{
				end[0] = points[0];
				end[1] = points[1];
			}
			points+=2;

(void)drbuffer, (void)drwidth, (void)drheight;
//			XW_PolyLine((unsigned int *)drbuffer, drwidth, drheight, start[0], start[1], end[0], end[1], gc);
			points++;

			start[0] = end[0];
			start[1] = end[1];
		}
	}
}

void XR_PolyRectangle(xclient_t *cl, xReq *request)
{
	unsigned int *out;
	xPolyRectangleReq *req = (xPolyRectangleReq *)request;
	xresource_t *drawable;
	xgcontext_t *gc;
	int i;

	short *rect;
	int rectnum;

	int drwidth;
	int drheight;
	unsigned char *drbuffer;



	if (XS_GetResource(req->drawable, (void**)&drawable) == x_none)
	{
		X_SendError(cl, BadDrawable, req->drawable, X_PolyRectangle, 0);
		return;
	}

	if (XS_GetResource(req->gc, (void**)&gc) == x_none)
	{
		X_SendError(cl, BadGC, req->gc, X_PolyRectangle, 0);
		return;
	}

	if (drawable->restype == x_window)
	{
		xwindow_t *wnd;
		wnd = (xwindow_t *)drawable;
		if (!wnd->buffer)
		{
			wnd->buffer = malloc(wnd->width*wnd->height*4);
			XW_ClearArea(wnd, 0, 0, wnd->width, wnd->height);
		}

		drwidth = wnd->width;
		drheight = wnd->height;
		drbuffer = wnd->buffer;
	}
	else if (drawable->restype == x_pixmap)
	{
		xpixmap_t *pm;
		pm = (xpixmap_t *)drawable;
		if (!pm->data)
		{
			pm->data = malloc(pm->width*pm->height*4);
			memset(pm->data, rand(), pm->width*pm->height*4);
		}

		drwidth = pm->width;
		drheight = pm->height;
		drbuffer = pm->data;
	}
	else
	{
		X_SendError(cl, BadDrawable, req->drawable, X_PolyRectangle, 0);
		return;
	}

	xrefreshed = true;


	rect = (short *)(req+1);
	for (rectnum = req->length/2 - 1; rectnum>0; rectnum--)
	{

//		Con_Printf("polyrect %i %i %i %i %i\n", req->drawable, rect[0], rect[1], rect[2], rect[3]);

		if (rect[2] < 0)
		{
			rect[2] *= -1;
		}
		if (rect[3] < 0)
		{
			rect[3] *= -1;
		}

		if (rect[0] < 0)
		{
			rect[2] += rect[0];
			rect[0] = 0;
		}
		if (rect[0] >= drwidth)
			rect[0] = drwidth-1;
		if (rect[1] < 0)
		{
			rect[3] += rect[1];
			rect[1] = 0;
		}
		if (rect[1] >= drheight)
			rect[1] = drheight-1;

		if (rect[0] + rect[2] > drwidth)
			rect[2] = drwidth - rect[0];
		if (rect[1] + rect[3] > drheight)
			rect[3] = drheight - rect[1];
		if (request->reqType == X_PolyFillRectangle)	//fill
		{
			while(rect[3])
			{
				out = (unsigned int *)drbuffer + (rect[0] + rect[1]*drwidth);
				for (i = 0; i < rect[2]; i++)
				{
					GCFunc(gc->fgcolour, *(char *)&out[i], gc->function, *(char *)&out[i], 0xff);
				}

				rect[3]--;
				rect[1]++;
			}
		}
		else	//outline
		{
			//top
			out = (unsigned int *)drbuffer + (rect[0] + rect[1]*drwidth);
			for (i = 1; i < rect[2]-1; i++)
			{
				GCFunc(gc->fgcolour, out[i], gc->function, out[i], 0xffffff);
			}

			//bottom
			if (rect[3]-1)
			{
				out = (unsigned int *)drbuffer + (rect[0] + (rect[1]+rect[3]-1)*drwidth);
				for (i = 1; i < rect[2]-1; i++)
				{
					GCFunc(gc->fgcolour, out[i], gc->function, out[i], 0xffffff);
				}
			}

			//left
			out = (unsigned int *)drbuffer + (rect[0] + rect[1]*drwidth);
			for (i = 0; i < rect[3]; i++)
			{
				GCFunc(gc->fgcolour, out[i*drwidth], gc->function, out[i*drwidth], 0xffffff);
			}

			//right
			if (rect[2]-1)
			{
				out = (unsigned int *)drbuffer + (rect[0]+rect[2]-1 + rect[1]*drwidth);
				for (i = 0; i < rect[3]; i++)
				{
					GCFunc(gc->fgcolour, out[i*drwidth], gc->function, out[i*drwidth], 0xffffff);
				}
			}
		}

		rect+=4;
	}
}

void XR_PolyPoint(xclient_t *cl, xReq *request)
{
	xPolyPointReq *req = (xPolyPointReq *)request;

	unsigned int *out;
	xresource_t *drawable;
	xgcontext_t *gc;

	short *point;
	int pointnum;

	int drwidth;
	int drheight;
	unsigned char *drbuffer;

	short lastpoint[2];

	if (XS_GetResource(req->drawable, (void**)&drawable) == x_none)
	{
		X_SendError(cl, BadDrawable, req->drawable, X_PolyPoint, 0);
		return;
	}

	if (XS_GetResource(req->gc, (void**)&gc) != x_gcontext)
	{
		X_SendError(cl, BadGC, req->gc, X_PolyPoint, 0);
		return;
	}

	if (drawable->restype == x_window)
	{
		xwindow_t *wnd;
		wnd = (xwindow_t *)drawable;
		if (!wnd->buffer)
		{
			wnd->buffer = malloc(wnd->width*wnd->height*4);
			XW_ClearArea(wnd, 0, 0, wnd->width, wnd->height);
		}

		drwidth = wnd->width;
		drheight = wnd->height;
		drbuffer = wnd->buffer;
	}
	else if (drawable->restype == x_pixmap)
	{
		xpixmap_t *pm;
		pm = (xpixmap_t *)drawable;
		if (!pm->data)
		{
			pm->data = malloc(pm->width*pm->height*4);
			memset(pm->data, rand(), pm->width*pm->height*4);
		}

		drwidth = pm->width;
		drheight = pm->height;
		drbuffer = pm->data;
	}
	else
	{
		X_SendError(cl, BadDrawable, req->drawable, X_PolyPoint, 0);
		return;
	}

	point = (short*)(req+1);

	if (req->coordMode)	//relative
	{
		lastpoint[0] = 0;	//do the absolute stuff
		lastpoint[1] = 0;
		for (pointnum = 1; pointnum>0; pointnum--)
		{
			lastpoint[0] += point[0];	//do the absolute stuff
			lastpoint[1] += point[1];

			out = (unsigned int *)drbuffer + lastpoint[0] + lastpoint[1]*drwidth;
			GCFunc(gc->fgcolour, *out, gc->function, *out, 0xffffff);
			point+=2;
		}

	}
	else	//absolute
	{
		for (pointnum = req->length-1; pointnum>0; pointnum--)
		{
			if (!(point[0] < 0 || point[1] < 0 || point[0] >= drwidth || point[1] >= drheight))
			{
				out = (unsigned int *)drbuffer + point[0] + point[1]*drwidth;
				GCFunc(gc->fgcolour, *out, gc->function, *out, 0xffffff);
			}

			point+=2;
		}
	}
}

void Draw_CharToDrawable (int num, unsigned int *drawable, int x, int y, int width, int height, xgcontext_t *gc)
{
	int		row, col;
	unsigned int	*source;
	int		drawline;
	int		i;

	int s, e;

	xfont_t *font;

	font = gc->font;
	if (!font || font->res.restype != x_font)
		return;

	s = 0;
	e = font->charwidth;
	if (x<0)
		s = s - x;

	if (x > width - e)
		e = width - x;

	if (s > e)
		return;
//	if (y >= height-e)
//		return;

//	if (y < -font->charheight)
//		return;

	if (y <= 0)
		drawable += x;
	else
		drawable += (width*y) + x;

	row = num>>4;
	col = num&15;
	source = font->data + (row*font->rowwidth*font->charheight) + (col*font->charwidth);
	if (y < 0)
		source -= font->rowwidth*y;

	drawline = height-y;
	if (drawline > font->charheight)
		drawline = font->charheight;

	if (y < 0)
			drawline += y;

	while (drawline-->=0)
	{
		for (i=s ; i<e ; i++)
			if (((qbyte*)(&source[i]))[3] > 128 && source[i])
//				GCFunc(gc->fgcolour, drawable[i], gc->function, drawable[i], source[i]);
				drawable[i] = source[i];
		source += font->rowwidth;
		drawable += width;
	}
}

void XR_PolyText(xclient_t *cl, xReq *request)
{
	unsigned char *str;
  
	xPolyTextReq	*req = (xPolyTextReq*)request;
	
	xresource_t *drawable;
	xgcontext_t *gc;


	int drwidth;
	int drheight;
	unsigned char *drbuffer;

	short charnum;
	short numchars;

	int xpos, ypos;

	if (XS_GetResource(req->drawable, (void**)&drawable) == x_none)
	{
		X_SendError(cl, BadDrawable, req->drawable, req->reqType, 0);
		return;
	}

	if (XS_GetResource(req->gc, (void**)&gc) != x_gcontext)
	{
		X_SendError(cl, BadGC, req->gc, req->reqType, 0);
		return;
	}
	if (!gc->font)
	{
		X_SendError(cl, BadGC, req->gc, req->reqType, 0);
		return;
	}

	if (drawable->restype == x_window)
	{
		xwindow_t *wnd;
		wnd = (xwindow_t *)drawable;
		if (!wnd->buffer)
		{
			wnd->buffer = malloc(wnd->width*wnd->height*4);
			XW_ClearArea(wnd, 0, 0, wnd->width, wnd->height);
		}

		drwidth = wnd->width;
		drheight = wnd->height;
		drbuffer = wnd->buffer;
	}
	else if (drawable->restype == x_pixmap)
	{
		xpixmap_t *pm;
		pm = (xpixmap_t *)drawable;
		if (!pm->data)
		{
			pm->data = malloc(pm->width*pm->height*4);
			memset(pm->data, rand(), pm->width*pm->height*4);
		}

		drwidth = pm->width;
		drheight = pm->height;
		drbuffer = pm->data;
	}
	else
	{
		X_SendError(cl, BadDrawable, req->drawable, req->reqType, 0);
		return;
	}

	xrefreshed = true;

	xpos = req->x;
	ypos = req->y-gc->font->charheight/2;

	str = (char*)(req+1);

	if (req->reqType == X_ImageText16 || req->reqType == X_ImageText8)
	{
		while(1)
		{
			charnum = 0;
			charnum |= *str++;
			if (req->reqType == X_ImageText16)
				charnum |= (*str++)<<8;
			if (!charnum)
				return;

			Draw_CharToDrawable(charnum&255, (unsigned int *)drbuffer, xpos, ypos, drwidth, drheight, gc);

			xpos += gc->font->charwidth;
		}
	}
	else
	{
		numchars = *(short*) str;
		str+=2;
		while(1)
		{
			charnum = 0;
			if (req->reqType == X_PolyText16)
				charnum |= (*str++)<<8;
			charnum |= *str++;
			if (!numchars--)
				return;

			Draw_CharToDrawable(charnum, (unsigned int *)drbuffer, xpos, ypos, drwidth, drheight, gc);

			xpos += gc->font->charwidth;
		}
	}
}

void XR_OpenFont(xclient_t *cl, xReq *request)	//basically ignored. We only support one font...
{
	xOpenFontReq *req = (xOpenFontReq *)request;
	char *name;

	name = (char *)(req+1);

	XS_CreateFont(req->fid, cl, name);
}

void XR_CloseFont(xclient_t *cl, xReq *request)	//basically ignored. We only support one font...
{
	xResourceReq *req = (xResourceReq *)request;
	void *font = XS_GetResourceType(req->id, x_font);
	if (font)
		XS_DestroyResource(font);
}

void XR_ListFonts(xclient_t *cl, xReq *request)	//basically ignored. We only support one font...
{
//	xListFontsReq *req = (xListFontsReq *)request;
	int buffer[256];
	xListFontsReply *reply = (xListFontsReply *)buffer;

	reply->type	= X_Reply;
    reply->sequenceNumber	= cl->requestnum;
    reply->length	= 0;
    reply->nFonts	= 0;

	X_SendData(cl, reply, sizeof(xGenericReply)+reply->length*4);
}

void XR_QueryFont(xclient_t *cl, xReq *request)	//basically ignored. We only support one font...
{
	xResourceReq *req = (xResourceReq *)request;
	char buffer[8192];
	int i;
	xCharInfo *ci;
	xQueryFontReply *rep = (xQueryFontReply *)buffer;
	xfont_t	*font = XS_GetResourceType(req->id, x_font);
	if (!font)
	{
		X_SendError(cl, BadFont, req->id, req->reqType, 0);
		return;
	}

	rep->type			= X_Reply;
	rep->pad1			= 0;
	rep->sequenceNumber	= cl->requestnum;
	rep->length			= 0;  /* definitely > 0, even if "nCharInfos" is 0 */

	rep->minBounds.leftSideBearing	= 0;
	rep->minBounds.rightSideBearing	= 0;
	rep->minBounds.characterWidth	= font->charwidth;
	rep->minBounds.ascent			= font->charheight/2;
	rep->minBounds.descent			= font->charheight/2;
	rep->minBounds.attributes		= 0;

#ifndef WORD64
	rep->walign1		= 0;
#endif
	rep->maxBounds.leftSideBearing	= 0;
	rep->maxBounds.rightSideBearing	= 0;
	rep->maxBounds.characterWidth	= font->charwidth;
	rep->maxBounds.ascent			= font->charheight/2;
	rep->maxBounds.descent			= font->charheight/2;
	rep->maxBounds.attributes		= 0;
#ifndef WORD64
	rep->walign2		= 0;
#endif
	rep->minCharOrByte2	= 0;
	rep->maxCharOrByte2	= 0;
	rep->defaultChar	= 0;
	rep->nFontProps		= 0;  /* followed by this many xFontProp structures */
	rep->drawDirection	= 0;
	rep->minByte1		= 0;
	rep->maxByte1		= 0;
	rep->allCharsExist	= 0;
	rep->fontAscent		= font->charheight/2;
	rep->fontDescent	= font->charheight/2;
	rep->nCharInfos		= 255; /* followed by this many xCharInfo structures */

	rep->length = ((sizeof(xQueryFontReply) - sizeof(xGenericReply)) + rep->nFontProps*sizeof(xFontProp) + rep->nCharInfos*sizeof(xCharInfo))/4;

	ci = (xCharInfo*)(rep+1);
	for (i = 0; i < rep->nCharInfos; i++)
	{
		ci[i].leftSideBearing = 0;
		ci[i].rightSideBearing = 0;
		ci[i].characterWidth = font->charwidth;
		ci[i].ascent = font->charheight/2;
		ci[i].descent = font->charheight/2;
	}

	X_SendData(cl, rep, sizeof(xGenericReply)+rep->length*4);
}

//esentually just throw it back at them.
void XR_AllocColor(xclient_t *cl, xReq *request)
{
	xAllocColorReq *req = (xAllocColorReq *)request;
	xAllocColorReply rep;
	unsigned char rgb[3] = {req->red>>8, req->green>>8, req->blue>>8};

	rep.type			= X_Reply;
	rep.pad1			= 0;
	rep.sequenceNumber	= cl->requestnum;
	rep.length			= 0;
	rep.red				= req->red;
	rep.green			= req->green;
	rep.blue			= req->blue;
	rep.pad2			= 0;
	rep.pixel			= (rgb[0]<<16) | (rgb[1]<<8) | (rgb[2]);
	rep.pad3			= 0;
	rep.pad4			= 0;
	rep.pad5			= 0;

	X_SendData(cl, &rep, sizeof(rep));
}

void XR_QueryColors(xclient_t *cl, xReq *request)
{
	xQueryColorsReq *req = (xQueryColorsReq *)request;
	xQueryColorsReply rep;
	xrgb rgb[65536];
	int n;
	int *pixel = (int*)(req+1);

	rep.type			= X_Reply;
	rep.pad1			= 0;
	rep.sequenceNumber	= cl->requestnum;
	rep.length			= 0;
	rep.nColors			= 0;
	rep.pad2			= 0;
	rep.pad3			= 0;
	rep.pad4			= 0;
	rep.pad5			= 0;
	rep.pad6			= 0;
	rep.pad7			= 0;

	for (n = 0; n < req->length - sizeof(*req)/4; n++)
	{
		rgb[n].red = ((pixel[n]>>16)&0xff)<<8;
		rgb[n].green = ((pixel[n]>>8)&0xff)<<8;
		rgb[n].blue = ((pixel[n]>>0)&0xff)<<8;
		rgb[n].pad = 0;
	}
	rep.nColors = n;
	rep.length = (sizeof(xrgb)*n+3)/4;

	X_SendData(cl, &rep, sizeof(rep));
	X_SendData(cl, rgb, rep.length*4);
}

void XR_LookupColor(xclient_t *cl, xReq *request)
{
	typedef struct  {
		char *name;
		float r, g, b;
	} colour_t;
	char colourname[256];
	colour_t *c, colour[] = {
		{"black",	0,0,0},
		{"grey",	0.5f,0.5f,0.5f},
		{"gray",	0.5f,0.5f,0.5f},	//wmaker uses this one. humour it.
		{"gray90",	0.9f,0.9f,0.9f},
		{"gray80",	0.8f,0.8f,0.8f},
		{"gray70",	0.7f,0.7f,0.7f},
		{"gray60",	0.6f,0.6f,0.6f},
		{"gray50",	0.5f,0.5f,0.5f},
		{"gray40",	0.4f,0.4f,0.4f},
		{"gray30",	0.3f,0.3f,0.3f},
		{"gray20",	0.2f,0.2f,0.2f},
		{"gray10",	0.1f,0.1f,0.1f},
		{"grey10",	0.1f,0.1f,0.1f},
		{"white",	1,1,1},
		{"red",		1,0,0},
		{"green",	0,1,0},
		{"blue",	0,0,1},
		{"blue4",	0,0,0.4f},
		{NULL}
	};

	xLookupColorReq *req = (xLookupColorReq *)request;
	xLookupColorReply rep;

	if (req->nbytes >= sizeof(colourname))
	{
		X_SendError(cl, BadName, 0, X_LookupColor, 0);
		return;
	}
	memcpy(colourname, (char *)(req+1), req->nbytes);
	colourname[req->nbytes] = '\0';

	for (c = colour; c->name; c++)
	{
		if (!strcasecmp(c->name, colourname))
		{
			break;
		}
	}

	if (!c->name)
	{
		X_SendError(cl, BadName, 0, X_LookupColor, 0);
		return;
	}


	rep.type			= X_Reply;
	rep.pad1			= 0;
	rep.sequenceNumber	= cl->requestnum;
	rep.length			= 0;
	rep.exactRed		= (unsigned short)(c->r*0xffffu);
	rep.exactGreen		= (unsigned short)(c->g*0xffffu);
	rep.exactBlue		= (unsigned short)(c->b*0xffffu);
	rep.screenRed		= rep.exactRed;
	rep.screenGreen		= rep.exactGreen;
	rep.screenBlue		= rep.exactBlue;
	rep.pad3			= 0;
	rep.pad4			= 0;
	rep.pad5			= 0;

	X_SendData(cl, &rep, sizeof(rep));
}

//get's keyboard status stuff.
void XR_GetKeyboardControl(xclient_t *cl, xReq *request)
{
	xGetKeyboardControlReply rep;

	rep.type				= X_Reply;
	rep.globalAutoRepeat	= false;
	rep.sequenceNumber		= cl->requestnum;
	rep.length				= 5;
	rep.ledMask				= 0;
	rep.keyClickPercent		= 0;
	rep.bellPercent			= 0;
	rep.bellPitch			= 0;
	rep.bellDuration		= 0;
	rep.pad					= 0;
	memset(rep.map, 0, sizeof(rep.map));	//per key map

	X_SendData(cl, &rep, sizeof(rep));
}

void XR_WarpPointer(xclient_t *cl, xReq *request)
{
//	xWarpPointerReq *req = (xWarpPointerReq *)request;
//	req->m
}

#ifdef XBigReqExtensionName
void XR_BigReq(xclient_t *cl, xReq *request)
{
	xBigReqEnableReply rep;

	rep.type				= X_Reply;
	rep.pad0				= 0;
	rep.sequenceNumber		= cl->requestnum;
	rep.length				= 0;
	rep.max_request_size	= 65535*1000;
	rep.pad1				= 0;
	rep.pad2				= 0;
	rep.pad3				= 0;
	rep.pad4				= 0;
	rep.pad5				= 0;

	X_SendData(cl, &rep, sizeof(rep));
}
#endif

#define MAXREQUESTSIZE	65535

//cl -> cl protocol
void XR_SendEvent (xclient_t *cl, xReq *request)
{
	int count;
	xSendEventReq *req = (xSendEventReq *)request;
	xwindow_t *wnd = XS_GetResourceType(req->destination, x_window);

	if (!wnd)
	{
		X_SendError(cl, BadWindow, req->destination, X_SendEvent, 0);
		return;
	}

	if (!req->eventMask)	//goes to owner.
	{
		req->event.u.u.sequenceNumber = cl->requestnum;
		X_SendData(wnd->res.owner, &req->event, sizeof(req->event));
	}
	else
	{
		xnotificationmask_t *nm;
		count = 0;
		while(!count)
		{
			for (nm = wnd->notificationmask; nm; nm = nm->next)
			{
				if (!(nm->mask & req->eventMask))
					continue;
				cl = nm->client;

				if (cl->stillinitialising)
					continue;

				count++;

				if (cl->tobedropped)
					continue;
				if (cl->outbufferlen > MAXREQUESTSIZE*4)
				{
					cl->tobedropped = true;
					continue;
				}

				req->event.u.u.sequenceNumber = cl->requestnum;
				X_SendData(cl, &req->event, sizeof(xEvent));
			}
			if (req->propagate)
				wnd = wnd->parent;
			else
				break;
		}
	}
}

void XR_GrabServer (xclient_t *cl, xReq *request)
{
	xgrabbedclient = cl;
}
void XR_UngrabServer (xclient_t *cl, xReq *request)
{
	xgrabbedclient = NULL;
}

xclient_t *xpointergrabclient;
xwindow_t *xpgrabbedwindow;
xwindow_t *xpconfinewindow;
unsigned int xpointergrabmask;
CARD32 xpointergrabcursor;
void XR_GrabPointer (xclient_t *cl, xReq *request)
{
	xGrabPointerReq *req = (xGrabPointerReq *)request;
	xGrabPointerReply reply;

	reply.type				= X_Reply;
    reply.status			= 0;
    reply.sequenceNumber 	= cl->requestnum;
    reply.length 			= 0;
    reply.pad1 				= 0;
    reply.pad2				= 0;
    reply.pad3 				= 0;
    reply.pad4 				= 0;
    reply.pad5				= 0;
    reply.pad6				= 0;

	if (xpointergrabclient && xpointergrabclient != cl)
	{	//you can't have it.
//		if (pointerstatus == Frozen)
//			reply.status			= GrabFrozen;
//		else
			reply.status			= AlreadyGrabbed;
		X_SendData(cl, &reply, sizeof(reply));
		return;
	}

	xpointergrabclient = cl;
	XS_GetResource(req->grabWindow, (void**)&xpgrabbedwindow);
	XS_GetResource(req->confineTo, (void**)&xpconfinewindow);
	xpointergrabmask = req->eventMask;
	xpointergrabcursor = req->cursor;
//	xpointergrabtime = req->time;

	X_EvalutateCursorOwner(NotifyGrab);


	reply.status			= GrabSuccess;
	X_SendData(cl, &reply, sizeof(reply));
}
void XR_ChangeActivePointerGrab (xclient_t *cl, xReq *request)
{
	xChangeActivePointerGrabReq *req = (xChangeActivePointerGrabReq *)request;

	if (xpointergrabclient != cl)
	{	//its not yours to change
		return;
	}

	xpointergrabmask = req->eventMask;
	xpointergrabcursor = req->cursor;
//	xpointergrabtime = req->time;
}
void XR_UngrabPointer (xclient_t *cl, xReq *request)
{
	xpointergrabclient = NULL;
	xpgrabbedwindow = NULL;
	xpconfinewindow = NULL;

	X_EvalutateCursorOwner(NotifyUngrab);
}

void XR_SetClipRectangles (xclient_t *cl, xReq *request)
{
	xSetClipRectanglesReq *req = (xSetClipRectanglesReq*)request;
	xgcontext_t *gc;
	if (XS_GetResource(req->gc, (void**)&gc) != x_gcontext)
	{
		X_SendError(cl, BadGC, req->gc, X_FreeGC, 0);
		return;
	}
	Con_DPrintf("XR_SetClipRectangles is not implemented\n");
}

void XR_NoOperation (xclient_t *cl, xReq *request)
{
}

void X_InitRequests(void)
{
	int ExtentionCode = X_NoOperation+1;

	memset(XRequests, 0, sizeof(XRequests));

	XRequests[X_QueryExtension] = XR_QueryExtension; 
	XRequests[X_ListExtensions] = XR_ListExtensions;
	XRequests[X_SetCloseDownMode] = XR_SetCloseDownMode;
	XRequests[X_GetProperty] = XR_GetProperty; 
	XRequests[X_ChangeProperty] = XR_ChangeProperty;
	XRequests[X_DeleteProperty] = XR_DeleteProperty;
	XRequests[X_ListProperties] = XR_ListProperties;
	XRequests[X_SetInputFocus] = XR_SetInputFocus;
	XRequests[X_GetInputFocus] = XR_GetInputFocus;
	XRequests[X_QueryBestSize] = XR_QueryBestSize;
	XRequests[X_CreateWindow] = XR_CreateWindow;
	XRequests[X_DestroyWindow] = XR_DestroyWindow;
	XRequests[X_QueryTree] = XR_QueryTree;
	XRequests[X_ChangeWindowAttributes] = XR_ChangeWindowAttributes;
	XRequests[X_GetWindowAttributes] = XR_GetWindowAttributes;
	XRequests[X_CreateGC] = XR_CreateGC;
	XRequests[X_ChangeGC] = XR_ChangeGC;
	XRequests[X_CopyGC] = XR_CopyGC;
	XRequests[X_FreeGC] = XR_FreeGC;
	XRequests[X_CreatePixmap] = XR_CreatePixmap;
	XRequests[X_FreePixmap] = XR_FreePixmap;
	XRequests[X_MapWindow] = XR_MapWindow;
	XRequests[X_MapSubwindows] = XR_MapSubwindows;
	XRequests[X_UnmapWindow] = XR_UnmapWindow;
	XRequests[X_ClearArea] = XR_ClearArea;
	XRequests[X_CopyArea] = XR_CopyArea;
	XRequests[X_InternAtom] = XR_InternAtom;
	XRequests[X_GetAtomName] = XR_GetAtomName;
	XRequests[X_PutImage] = XR_PutImage;
	XRequests[X_GetImage] = XR_GetImage;
	XRequests[X_PolyRectangle] = XR_PolyRectangle;
	XRequests[X_PolyFillRectangle] = XR_PolyRectangle;
	XRequests[X_PolyPoint] = XR_PolyPoint;
	XRequests[X_PolyLine] = XR_PolyLine;
	XRequests[X_PolySegment] = XR_PolyLine;
	XRequests[X_QueryPointer] = XR_QueryPointer;
//	XRequests[X_ChangeKeyboardMapping] = XR_ChangeKeyboardMapping;
//	XRequests[X_SetModifierMapping] = XR_ChangeKeyboardMapping;
	XRequests[X_GetKeyboardMapping] = XR_GetKeyboardMapping;
	XRequests[X_GetKeyboardControl] = XR_GetKeyboardControl;
	XRequests[X_GetModifierMapping] = XR_GetModifierMapping;
	XRequests[X_AllocColor] = XR_AllocColor;
	XRequests[X_LookupColor] = XR_LookupColor;
	XRequests[X_QueryColors] = XR_QueryColors;
	XRequests[X_GetGeometry] = XR_GetGeometry;
	XRequests[X_CreateCursor] = XR_CreateCursor;
	XRequests[X_CreateGlyphCursor] = XR_CreateGlyphCursor;
	XRequests[X_RecolorCursor] = XR_RecolorCursor;
	XRequests[X_FreeCursor] = XR_FreeCursor;

	XRequests[X_GrabButton] = XR_GrabButton;
	XRequests[X_UngrabButton] = XR_UngrabButton;

	XRequests[X_WarpPointer] = XR_WarpPointer;
	XRequests[X_ListFonts] = XR_ListFonts;
	XRequests[X_OpenFont] = XR_OpenFont;
	XRequests[X_CloseFont] = XR_CloseFont;
	XRequests[X_QueryFont] = XR_QueryFont;
	XRequests[X_PolyText8] = XR_PolyText;
	XRequests[X_PolyText16] = XR_PolyText;
	XRequests[X_ImageText8] = XR_PolyText;
	XRequests[X_ImageText16] = XR_PolyText;

	XRequests[X_SetClipRectangles] = XR_SetClipRectangles;

	XRequests[X_ConfigureWindow] = XR_ConfigureWindow;
	XRequests[X_ReparentWindow] = XR_ReparentWindow;

	XRequests[X_GrabServer] = XR_GrabServer;
	XRequests[X_UngrabServer] = XR_UngrabServer;
	XRequests[X_GrabPointer] = XR_GrabPointer;
	XRequests[X_ChangeActivePointerGrab] = XR_ChangeActivePointerGrab;
	XRequests[X_UngrabPointer] = XR_UngrabPointer;


	XRequests[X_SendEvent] = XR_SendEvent;

	XRequests[X_ConvertSelection] = XR_ConvertSelection;
	XRequests[X_GetSelectionOwner] = XR_GetSelectionOwner;
	XRequests[X_SetSelectionOwner] = XR_SetSelectionOwner;

	XRequests[X_GrabKey] = XR_NoOperation;
	XRequests[X_AllowEvents] = XR_NoOperation;
	XRequests[X_FillPoly] = XR_FillPoly;
	XRequests[X_NoOperation] = XR_NoOperation;

#ifdef XBigReqExtensionName
	X_BigReqCode=ExtentionCode++;
	XRequests[X_BigReqCode] = XR_BigReq;
#endif
}

