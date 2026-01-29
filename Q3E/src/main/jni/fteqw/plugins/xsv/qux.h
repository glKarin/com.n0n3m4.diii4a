#include "../plugin.h"

/*
#ifdef _WIN32	//for multithreaded reading
#define BOOL WINDOWSSUCKS_BOOL
#define INT32 WINDOWSSUCKS_INT32
#include "winquake.h"
#undef BOOL
#undef INT32

#define MULTITHREADWIN32
#endif

#ifdef MULTITHREADWIN32
#define MULTITHREAD
#endif
*/

#include "Xproto.h"
#include "x.h"

#include "bigreqstr.h"

#define XK_MISCELLANY
#define XK_LATIN1
#include "keysymdef.h"

typedef struct xclient_s {
	int closedownmode;
	qhandle_t socket;
	int inbufferlen;
	int outbufferlen;
	int inbuffermaxlen;
	int outbuffermaxlen;
	char *outbuffer;
	char *inbuffer;
	qboolean tobedropped;	//dropped when no more to send.
	qboolean stillinitialising;
	unsigned int requestnum;
	unsigned int ridbase;
	struct xclient_s *nextclient;

#ifdef MULTITHREADWIN32
	HANDLE threadhandle;
	CRITICAL_SECTION		delecatesection;
#endif
} xclient_t;

extern xclient_t *xgrabbedclient;	//stops reading other clients
extern xclient_t *xpointergrabclient;

typedef struct xproperty_s {
	Atom atomid;
	Atom type;
	int datalen;
	struct xproperty_s *next;
	int format;
	char data[1];
} xproperty_t;

typedef struct xnotificationmask_s {
	xclient_t *client;
	unsigned int mask;
	struct xnotificationmask_s *next;
} xnotificationmask_t;

typedef struct xresource_s {
	enum {x_none, x_window, x_gcontext, x_pixmap, x_font, x_atom} restype;
	int id;
	xclient_t *owner;

	struct xresource_s *next, *prev;
} xresource_t;
typedef struct xpixmap_s {
	xresource_t res;

	int references;
	qboolean linked;

	int width;
	int height;
	int depth;

	qbyte *data;
} xpixmap_t;
typedef struct xatom_s {
	xresource_t res;
	int selectionownerwindowid;
	xclient_t *selectionownerclient;
	char atomname[1]; //must be last
} xatom_t;
typedef struct xwindow_s {
	xresource_t res;

	int xpos;
	int ypos;
	int width;
	int height;
	char *buffer;
	int bg;
	struct xwindow_s *parent;
	struct xwindow_s *child;
	struct xwindow_s *sibling;

	xpixmap_t *backpixmap;
	unsigned int backpixel;
	unsigned int borderpixel;
	int bitgravity, wingravity;
	unsigned int donotpropagate;
	int colormap;

	qboolean mapped;
	qboolean overrideredirect;

	qboolean inputonly;

	int depth;

	xproperty_t *properties;
	xnotificationmask_t *notificationmask;
	unsigned int notificationmasks;
} xwindow_t;

typedef struct xfont_s
{
	xresource_t res;
	int depth;

	char name[256];

	int rowwidth;
	int rowheight;

	int charwidth;
	int charheight;

	unsigned int data[4];
} xfont_t;

typedef struct xgcontext_s
{
	xresource_t res;
	int depth;

	int function;
	int fgcolour;
	int bgcolour;

	xfont_t *font;
} xgcontext_t;

extern xwindow_t *rootwindow;
extern qboolean xrefreshed;	//something onscreen changed.

int XS_GetResource(int id, void **data);
void *XS_GetResourceType(int id, int requiredtype);
void XS_SetProperty(xwindow_t *wnd, Atom atomid, Atom atomtype, char *data, int datalen, int format);
int XS_GetProperty(xwindow_t *wnd, Atom atomid, Atom *type, char *output, int maxlen, int offset, int *extrabytes, int *format);
void XS_DeleteProperty(xwindow_t *wnd, Atom atomid);
xatom_t *XS_CreateAtom(Atom atomid, char *name, xclient_t *owner);
Atom XS_FindAtom(char *name);
xgcontext_t *XS_CreateGContext(int id, xclient_t *owner, xresource_t *drawable);
int XS_NewResource(void);
xwindow_t *XS_CreateWindow(int wid, xclient_t *owner, xwindow_t *parent, short x, short y, short width, short height);
void X_Resize(xwindow_t *wnd, int newx, int newy, int neww, int newh);
void XS_SetParent(xwindow_t *wnd, xwindow_t *parent);
xpixmap_t *XS_CreatePixmap(int id, xclient_t *owner, int width, int height, int depth);
xfont_t *XS_CreateFont(int id, xclient_t *owner, char *fontname);
void XS_CreateInitialResources(void);
void XS_DestroyResource(xresource_t *res);
void XS_DestroyResourcesOfClient(xclient_t *cl);


void XW_ExposeWindow(xwindow_t *root, int x, int y, int width, int height);
void XW_ClearArea(xwindow_t *wnd, int xp, int yp, int width, int height);



typedef void (*XRequest) (xclient_t *cl, xReq *request);
extern XRequest XRequests [256];

void X_SendData(xclient_t *cl, void *data, int datalen);
void X_SendNotification(xEvent *data);
int X_SendNotificationMasked(xEvent *data, xwindow_t *window, unsigned int mask);
qboolean X_NotifcationMaskPresent(xwindow_t *window, int mask, xclient_t *notfor);
void X_SendError(xclient_t *cl, int errorcode, int assocresource, int major, int minor);
void X_InitRequests(void);

void X_EvalutateCursorOwner(int movemode);
void X_EvalutateFocus(int movemode);


extern qbyte *xscreen;
extern short xscreenwidth;
extern short xscreenheight;

#ifndef K_CTRL
extern int K_BACKSPACE;
extern int K_CTRL;
extern int K_ALT;
extern int K_MOUSE1;
extern int K_MOUSE2;
extern int K_MOUSE3;
extern int K_MOUSE4;
extern int K_MOUSE5;
#endif
