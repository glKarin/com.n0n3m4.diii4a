//network interface+main module

//this code should work okay as a host server for Xepher.
//issue these two commands and Xephyr will do all the stuff we don't support properly.
//DISPLAY=127.0.0.1:0 Xephyr :1
//wmaker -display :1 (or xterm or whatever)

#include "../plugin.h"
static plugnetfuncs_t *netfuncs;
static plugsubconsolefuncs_t *confuncs;

#include "qux.h"
#include "../engine.h"

#undef MULTITHREAD

float mousecursor_x, mousecursor_y;

static const int baseport = 6000;
static xclient_t *xclients;
static qhandle_t xlistensocket = -1;

xwindow_t *xfocusedwindow;

qboolean xrefreshed;
qboolean xscreenmodified;	//texture updated

xclient_t *xgrabbedclient;	//clients can ask the server to ignore other clients
extern xwindow_t *xpgrabbedwindow;

#define MAXREQUESTSIZE	65535

int ctrldown, altdown;

#ifndef K_CTRL
int K_BACKSPACE;
int K_CTRL;
int K_ALT;
int K_MOUSE1;
int K_MOUSE2;
int K_MOUSE3;
int K_MOUSE4;
int K_MOUSE5;
#endif

int QKeyToScan(int qkey)
{	//X11 uses some variation of hardware scancodes.
	//custom keymaps tend to ignore the server and use some other table instead.
	//so we need to try to match what most servers expect
	switch(qkey)
	{
//	case K_:			return 1;
//	case K_:			return 2;
//	case K_:			return 3;
//	case K_:			return 4;
//	case K_:			return 5;
//	case K_:			return 6;
//	case K_:			return 7;
//	case K_:			return 8;
//	case K_:			return 9;
	case '1':			return 10;
	case '2':			return 11;
	case '3':			return 12;
	case '4':			return 13;
	case '5':			return 14;
	case '6':			return 15;
	case '7':			return 16;
	case '8':			return 17;
	case '9':			return 18;
	case '-':			return 19;
	case '+':			return 20;
	case '=':			return 21;
	case K_BACKSPACE:	return 22;

	case K_TAB:			return 23;
	case 'q':			return 24;
	case 'w':			return 25;
	case 'e':			return 26;
	case 'r':			return 27;
	case 't':			return 28;
	case 'y':			return 29;
	case 'u':			return 30;
	case 'i':			return 31;
	case 'o':			return 32;
	case 'p':			return 33;
	case '[':			return 34;
	case ']':			return 35;
	case K_ENTER:		return 36;

	case K_LCTRL:		return 37;
	case 'a':			return 38;
	case 's':			return 39;
	case 'd':			return 40;
	case 'f':			return 41;
	case 'g':			return 42;
	case 'h':			return 43;
	case 'j':			return 44;
	case 'k':			return 45;
	case 'l':			return 46;
	case ';':			return 47;
	case '\'':			return 48;
	case '`':			return 49;

	case K_LSHIFT:		return 50;
	case '#':			return 51;
	case 'z':			return 52;
	case 'x':			return 53;
	case 'c':			return 54;
	case 'v':			return 55;
	case 'b':			return 56;
	case 'n':			return 57;
	case 'm':			return 58;
	case ',':			return 59;
	case '.':			return 60;
	case '/':			return 61;
	case K_RSHIFT:		return 62;

	case K_KP_STAR:		return 63;
	case K_LALT:		return 64;
	case K_SPACE:		return 65;
	case K_CAPSLOCK:	return 66;
	case K_F1:			return 67;
	case K_F2:			return 68;
	case K_F3:			return 69;
	case K_F4:			return 70;
	case K_F5:			return 71;
	case K_F6:			return 72;
	case K_F7:			return 73;
	case K_F8:			return 74;
	case K_F9:			return 75;
	case K_F10:			return 76;

	case K_KP_NUMLOCK:	return 77;
	case K_SCRLCK:		return 78;
	case K_KP_HOME:		return 79;
	case K_KP_UPARROW:	return 80;
	case K_KP_PGUP:		return 81;
	case K_KP_MINUS:	return 82;
	case K_KP_LEFTARROW:return 83;
	case K_KP_5:		return 84;
	case K_KP_RIGHTARROW:return 85;
	case K_KP_PLUS:		return 86;
	case K_KP_END:		return 87;
	case K_KP_DOWNARROW:return 88;
	case K_KP_PGDN:		return 89;
	case K_KP_INS:		return 90;
	case K_KP_DEL:		return 91;

//	case K_L3SHIFT:		return 92;
//	case K_:			return 93;
	case '\\':			return 94;
	case K_F11:			return 95;
	case K_F12:			return 96;
//	case K_:			return 97;
//	case K_KATAKANA:	return 98;
//	case K_HIRAGANA:	return 99;
//	case K_HENKAN_MODE:	return 100;
//	case K_HIRAGANA_KATAKANA:return 101;
//	case K_MUHENKAN:	return 102;
//	case K_:			return 103;
	case K_KP_ENTER:	return 104;
	case K_RCTRL:		return 105;
	case K_KP_SLASH:	return 106;
	case K_PRINTSCREEN:	return 107;
//	case K_L3SHIFT:		return 108;
//	case K_LINEFEED:	return 109;
	case K_HOME:		return 110;
	case K_UPARROW:		return 111;
	case K_PGUP:		return 112;
	case K_LEFTARROW:	return 113;
	case K_RIGHTARROW:	return 114;
	case K_END:			return 115;
	case K_DOWNARROW:	return 116;
	case K_PGDN:		return 117;
	case K_INS:			return 118;
	case K_DEL:			return 119;

//	case K_:			return 120;
	case K_MM_VOLUME_MUTE:return 121;
	case K_VOLDOWN:		return 122;
	case K_VOLUP:		return 123;
	case K_POWER:		return 124;
	case K_KP_EQUALS:	return 125;
//	case K_PLUSMINUS:	return 126;
	case K_PAUSE:		return 127;
//	case K_LAUNCHA:		return 128;
//	case K_KP_DECIMAL:	return 129;
//	case K_HANGUL:		return 130;
//	case K_HANGUL_HANJA:return 131;
//	case K_:			return 132;
	case K_LWIN:		return 133;
	case K_RWIN:		return 134;
	case K_APP:			return 135;
//	case K_CANCEL:		return 136;
//	case K_REDO:		return 137;
//	case K_SUNPROPS:	return 138;
//	case K_UNDO:		return 139;
//	case K_SUNFRONT:	return 140;
//	case K_COPY:		return 141;
//	case K_OPEN:		return 142;
//	case K_PASTE:		return 143;
//	case K_FIND:		return 144;
//	case K_CUT:			return 145;
//	case K_HELP:		return 146;
//	case K_MENUKB:		return 147;
//	case K_CALCULATOR:	return 148;



	default:			return 0;
	}
}

void X_SendData(xclient_t *cl, void *data, int datalen)
{
#ifdef MULTITHREADWIN32
	if (cl->threadhandle)
		EnterCriticalSection(&cl->delecatesection);
#endif

	if (cl->outbufferlen + datalen > cl->outbuffermaxlen)
	{	//extend buffer size
		cl->outbuffermaxlen = cl->outbufferlen + datalen + 1024;
		cl->outbuffer = realloc(cl->outbuffer, cl->outbuffermaxlen);
	}

	memcpy(cl->outbuffer+cl->outbufferlen, data, datalen);
	cl->outbufferlen += datalen;

#ifdef MULTITHREADWIN32
	if (cl->threadhandle)
		LeaveCriticalSection(&cl->delecatesection);
#endif
}

void X_SendNotification(xEvent *data)
{
	xclient_t *cl;
	for (cl = xclients; cl; cl = cl->nextclient)
	{
		if (cl->stillinitialising)
			continue;
		if (cl->tobedropped)
			continue;
		if (cl->outbufferlen > MAXREQUESTSIZE*4)
			continue;

		data->u.u.sequenceNumber = cl->requestnum;
		X_SendData(cl, data, sizeof(xEvent));
	}
}


qboolean X_NotifcationMaskPresent(xwindow_t *window, int mask, xclient_t *notfor)
{
	xnotificationmask_t *nm;
	nm = window->notificationmask;

	if (mask == SubstructureNotifyMask || mask == SubstructureRedirectMask)
	{
		window = window->parent;
//		for(;window;window=window->parent)
		{
			for (nm = window->notificationmask; nm; nm = nm->next)
			{
				if (nm->mask & mask)
					if (nm->client != notfor)
						return true;
			}
		}
	}

	else
	{
		for (nm = window->notificationmask; nm; nm = nm->next)
		{
			if (nm->mask & mask)
				if (nm->client != notfor)
					return true;
		}
	}

	return false;
}

int X_SendNotificationMasked(xEvent *data, xwindow_t *window, unsigned int mask)
{
	int count=0;
	xclient_t *cl;
	xnotificationmask_t *nm;

	xwindow_t *child = window;

	if (mask == SubstructureNotifyMask || mask == SubstructureRedirectMask)
	{
		for (cl = xclients; cl; cl = cl->nextclient)
		{
			//don't send to if...
			if (cl->stillinitialising)
				continue;
			if (cl->tobedropped)
				continue;
			if (cl->outbufferlen > MAXREQUESTSIZE*4)
			{
				cl->tobedropped = true;
				continue;
			}
			window = child->parent;
			if (window)
//			for (window = child; window; window = window->parent)
			{
				for (nm = window->notificationmask; nm; nm = nm->next)
				{
					if (nm->client != cl)
						continue;
					if (!(nm->mask & mask))
						continue;

					data->u.reparent.event = window->res.id;	//so the request/notification/whatever knows who asked for it.

					data->u.u.sequenceNumber = cl->requestnum;
					X_SendData(cl, data, sizeof(xEvent));
					count++;
					break;
				}
//				if (nm)
//					break;
			}
		}
	}
	else
	{
		for (nm = window->notificationmask; nm; nm = nm->next)
		{
			if (!(nm->mask & mask))
				continue;
			cl = nm->client;

			if (cl->stillinitialising)
				continue;
			if (cl->tobedropped)
				continue;
			if (cl->outbufferlen > MAXREQUESTSIZE*4)
			{
				cl->tobedropped = true;
				continue;
			}

			data->u.u.sequenceNumber = cl->requestnum;
			X_SendData(cl, data, sizeof(xEvent));
			count++;
		}
	}
	return count;
}

int X_SendInputNotification(xEvent *data, xwindow_t *window, unsigned int mask)
{
	int count=0;
	xclient_t *cl;
	xnotificationmask_t *nm;

	xwindow_t *child = window;
	xwindow_t *focus;

	//we go all the way to the root if needed.

	for (cl = xclients; cl; cl = cl->nextclient)
	{
		//don't send to if...
		if (cl->stillinitialising)
			continue;
		if (cl->tobedropped)
			continue;
		if (cl->outbufferlen > MAXREQUESTSIZE*4)
		{
			cl->tobedropped = true;
			continue;
		}
		window = child->parent;

		for (window = child; window; window = window->parent)
		{
			for (nm = window->notificationmask; nm; nm = nm->next)
			{
				if (nm->client != cl)
					continue;
				if (!(nm->mask & mask))
					continue;

//				Con_Printf("Sending input %i\n", data->u.u.type);

				if (data->u.u.type == FocusIn || data->u.u.type == FocusOut)
				{
					data->u.u.sequenceNumber = cl->requestnum;
					X_SendData(cl, data, sizeof(xEvent));
					count++;
					break;
				}

				data->u.keyButtonPointer.event = window->res.id;	//so the request/notification/whatever knows who asked for it.
				data->u.keyButtonPointer.eventX = data->u.keyButtonPointer.rootX;
				data->u.keyButtonPointer.eventY = data->u.keyButtonPointer.rootY;
				for (window = window; window; window = window->parent)	//adjust event's xpos/ypos
				{
					data->u.keyButtonPointer.eventX -= window->xpos;
					data->u.keyButtonPointer.eventY -= window->ypos;
				}

				if (data->u.u.type == EnterNotify || data->u.u.type == LeaveNotify)
				{
					data->u.enterLeave.flags &= ~ELFlagFocus;

					focus = xfocusedwindow;
					while(focus)
					{
						if (focus->res.id == data->u.enterLeave.event)
						{
							data->u.enterLeave.flags |= ELFlagFocus;
							break;
						}
						focus = focus->parent;
					}
				}

				data->u.u.sequenceNumber = cl->requestnum;
				if (data->u.keyButtonPointer.event == data->u.keyButtonPointer.child)
				{
					data->u.keyButtonPointer.child = None;
					X_SendData(cl, data, sizeof(xEvent));
					data->u.keyButtonPointer.child = data->u.keyButtonPointer.event;
				}
				else
					X_SendData(cl, data, sizeof(xEvent));
				count++;
				break;
			}
			if (nm || (window->donotpropagate & mask))
				break;
		}
	}
	return count;
}

void X_SendError(xclient_t *cl, int errorcode, int assocresource, int major, int minor)
{
	xError err;
	err.type			= X_Error;
	err.errorCode		= errorcode;
    err.sequenceNumber	= cl->requestnum;       /* the nth request from this client */
    err.resourceID		= assocresource;
    err.minorCode		= minor;
    err.majorCode		= major;
    err.pad1			= 0;
    err.pad3			= 0;
    err.pad4			= 0;
    err.pad5			= 0;
    err.pad6			= 0;
    err.pad7			= 0;

	X_SendData(cl, &err, sizeof(err));
}

int X_NewRIDBase(void)
{
	xclient_t *cl;
	int ridbase = 0x200000;
	while(ridbase)	//it'll wrap at some point...
	{
		for (cl = xclients; cl; cl = cl->nextclient)
		{
			if (cl->ridbase == ridbase)	//someone has this range...
			{
				ridbase+=0x200000;
				break;
			}
		}
		if (!cl)
			return ridbase;
	}

	//err... bugger... that could be problematic...
	//try again, but start allocating half quantities and hope a client drops soon...

	ridbase = 0x200000;
	while(ridbase)
	{
		for (cl = xclients; cl; cl = cl->nextclient)
		{
			if (cl->ridbase == ridbase)	//someone has this range...
			{
				ridbase+=0x100000;
				break;;
			}
		}
		if (!cl)
			return ridbase;
	}

	if (ridbase)
		return ridbase;

	return 0;
}

void X_SendIntialResponse(xclient_t *cl)
{
	int rid;
	char buffer[8192];
	xConnSetupPrefix *prefix;
	xConnSetup	*setup;
	char *vendor;
	xPixmapFormat *pixmapformats;
	xnotificationmask_t *nm;

	xWindowRoot *root;
	xDepth *depth;
	xVisualType *visualtype;

	rid = X_NewRIDBase();
	cl->ridbase = rid;

	if (!rid)
	{
		prefix = (xConnSetupPrefix *)buffer;
		prefix->success = 0;
		prefix->lengthReason = 22;
		prefix->majorVersion = 11;	//protocol version.
		prefix->minorVersion = 0;
		prefix->length = (prefix->lengthReason/4+3)&~3;
		strcpy((char *)(prefix+1), "No free resource range");
		X_SendData(cl, prefix, sizeof(prefix)+(prefix->length+1)*4);
		cl->tobedropped = true;
	}
	else
	{
		prefix = (xConnSetupPrefix *)buffer;
		prefix->success = 1;
		prefix->lengthReason = 0;
		prefix->majorVersion = 11;	//protocol version.
		prefix->minorVersion = 0;

		setup = (xConnSetup	*)(prefix+1);
		setup->release				= 0;//build_number();		//our version number
		setup->ridBase				= rid;
		setup->ridMask				= 0x1fffff;
		setup->motionBufferSize		= 1;
		setup->maxRequestSize		= MAXREQUESTSIZE;
		setup->numRoots				= 1;			//we only have one display. so only one root window please.
		setup->imageByteOrder		= LSBFirst;        /* LSBFirst, MSBFirst */
		setup->bitmapBitOrder		= LSBFirst;        /* LeastSignificant, MostSign...*/
		setup->bitmapScanlineUnit	= 32,     /* 8, 16, 32 */
		setup->bitmapScanlinePad	= 32;     /* 8, 16, 32 */
		setup->minKeyCode			= 1;
		setup->maxKeyCode			= 255;

		vendor = (char *)(setup+1);
		strcpy(vendor, "FTE X");
		setup->nbytesVendor = (strlen(vendor)+3)&~3;

		pixmapformats = (xPixmapFormat *)(vendor + setup->nbytesVendor);
		setup->numFormats = 0;

	/*	pixmapformats[setup->numFormats].depth = 16;
		pixmapformats[setup->numFormats].bitsPerPixel = 16;
		pixmapformats[setup->numFormats].scanLinePad = 16;
		pixmapformats[setup->numFormats].pad1=0;
		pixmapformats[setup->numFormats].pad2=0;
		setup->numFormats++;*/

		pixmapformats[setup->numFormats].depth = 24;
		pixmapformats[setup->numFormats].bitsPerPixel = 32;
		pixmapformats[setup->numFormats].scanLinePad = 32;
		pixmapformats[setup->numFormats].pad1=0;
		pixmapformats[setup->numFormats].pad2=0;
		setup->numFormats++;

		root = (xWindowRoot *)(pixmapformats + setup->numFormats);
		root->windowId			= rootwindow->res.id;
		root->defaultColormap	= 32;
		root->whitePixel		= 0xffffff;
		root->blackPixel		= 0;
		root->currentInputMask	= 0;   
		for (nm = rootwindow->notificationmask; nm; nm = nm->next)
			root->currentInputMask |= nm->mask;
		root->pixWidth			= rootwindow->width;
		root->pixHeight			= rootwindow->height;
		root->mmWidth			= rootwindow->width/3;
		root->mmHeight			= rootwindow->height/3;
		root->minInstalledMaps	= 1;
		root->maxInstalledMaps	= 1;
		root->rootVisualID		= 0x22;
		root->backingStore		= 0;
		root->saveUnders		= false;
		root->rootDepth			= 24;
		root->nDepths = 0;

		depth = (xDepth*)(root + 1);
		depth->depth = 24;
		depth->pad1 = 0;
		depth->nVisuals = 1;
		depth->pad2 = 0;
		root->nDepths++;

		visualtype = (xVisualType*)(depth+1);
		visualtype->visualID = root->rootVisualID;
		visualtype->class		= TrueColor;
		visualtype->bitsPerRGB	= 24;
		visualtype->colormapEntries = 256;
		visualtype->redMask		= 0xff0000;
		visualtype->greenMask	= 0x00ff00;
		visualtype->blueMask	= 0x0000ff;
		visualtype->pad = 0;

		visualtype++;
		prefix->length = ((char *)visualtype - (char *)setup)/4;

		X_SendData(cl, prefix, (char *)visualtype - (char *)prefix);
	}
}






qboolean XWindows_TendToClient(xclient_t *cl)	//true says drop
{
	int err;
	int len;
	unsigned int inlen;
	char *input;

	if (!xgrabbedclient || xgrabbedclient == cl)	//someone grabbed the server
	if (cl->outbufferlen < 256 && !cl->tobedropped)	//don't accept new messages if we still have a lot to write.
	{
#ifdef MULTITHREADWIN32
		if (!cl->threadhandle)
#endif
		{
			if (cl->inbuffermaxlen - cl->inbufferlen < 1000)	//do we need to expand this message?
			{
				char *newbuffer;
				cl->inbuffermaxlen += 1000;
				newbuffer = malloc(cl->inbuffermaxlen);
				if (cl->inbuffer)
				{
					memcpy(newbuffer, cl->inbuffer, cl->inbufferlen);
					free(cl->inbuffer);
				}
				cl->inbuffer = newbuffer;
			}
			len = cl->inbuffermaxlen - cl->inbufferlen;
//Con_Printf("recving\n");
			len = netfuncs->Recv(cl->socket, cl->inbuffer + cl->inbufferlen, len);
//Con_Printf("recved %i\n", len);
			if (len == 0)	//connection was closed. bummer.
			{
//Con_Printf("Closed\n");
				return true;
			}
			if (len > 0)
			{
				cl->inbufferlen += len;
			}
			else
			{
				err = len;
				if (err != N_WOULDBLOCK)
				{
					Con_Printf("X read error %i\n", err);
					cl->tobedropped = true;
				}
			}
		}
#ifdef MULTITHREADWIN32
		else
			EnterCriticalSection(&cl->delecatesection);
#endif

//		if (len > 0)	//the correct version
		if (cl->inbufferlen > 0)				//temp
		{
			input = cl->inbuffer;
nextmessage:
			inlen = cl->inbufferlen - (input - cl->inbuffer);
			if (cl->stillinitialising)
			{
				if (inlen >= sizeof(xConnClientPrefix))
				{
					xConnClientPrefix *prefix = (xConnClientPrefix *)input;
					cl->stillinitialising = false;
					if (prefix->byteOrder != 'l')	//egad no. horrible.
					{
#ifdef MULTITHREADWIN32
						LeaveCriticalSection(&cl->delecatesection);
#endif
						return true;
					}
					if (prefix->majorVersion != 11)	//x11 only. Sorry.
					{
#ifdef MULTITHREADWIN32
						LeaveCriticalSection(&cl->delecatesection);
#endif
						return true;
					}
					if (prefix->minorVersion != 0)	//we don't know of any variations.
					{
#ifdef MULTITHREADWIN32
						LeaveCriticalSection(&cl->delecatesection);
#endif
						return true;
					}
					/*if (prefix->nbytesAuthProto != 0)	//we can't handle this
					{
#ifdef MULTITHREADWIN32
						LeaveCriticalSection(&cl->delecatesection);
#endif
						return true;
					}
					if (prefix->nbytesAuthString != 0)	//we can't handle this
					{
#ifdef MULTITHREADWIN32
						LeaveCriticalSection(&cl->delecatesection);
#endif
						return true;
					}*/

					if (inlen >= sizeof(*prefix) + ((prefix->nbytesAuthProto+3)&~3) + ((prefix->nbytesAuthString+3)&~3))
					{
						input += sizeof(*prefix) + ((prefix->nbytesAuthProto+3)&~3) + ((prefix->nbytesAuthString+3)&~3);
						X_SendIntialResponse(cl);
						goto nextmessage;
					}
				}
			}
			else if (inlen >= sizeof(xReq))
			{
				unsigned int rlen;
				xReq *req;
				req = (xReq *)input;

				rlen = req->length;
				if (!rlen && inlen >= sizeof(xReq)+sizeof(CARD32))	//BIG-REQUESTS says that if the length of a request is 0, then there's an extra 32bit int with the correct length imediatly after the 0.
					rlen = *(CARD32 *)(req+1);
				if (rlen && inlen >= rlen*4)
				{
					cl->requestnum++;

					if (/*req->reqType < 0 || req->reqType >= 256 ||*/ !XRequests[req->reqType])
					{
	//					Con_Printf("X request %i, len %i - NOT SUPPORTED\n", req->reqType, rlen*4);

						//this is a minimal implementation...
						X_SendError(cl, BadImplementation, 0, req->reqType, 0);
//						cl->tobedropped = true;
					}
					else
					{
//						Con_Printf("X request %i, len %i\n", req->reqType, rlen*4);

//Con_Printf("Request %i\n", req->reqType);
//						Z_CheckSentinals();
						if (!req->length)
						{
							int rt, data;

							rt = req->reqType;	//save these off
							data = req->data;

							req = (xReq *)((char *)req+sizeof(CARD32));	//adjust correctly.

							req->reqType = rt;	//and restore them into the space taken by the longer size.
							req->data	= data;
							req->length = 0;	//Don't rely on this. This isn't really needed.

							XRequests[req->reqType](cl, req);
						}
						else
							XRequests[req->reqType](cl, req);
//						Z_CheckSentinals();
//Con_Printf("Done request\n");
					}

					input += rlen*4;

					goto nextmessage;
				}
			}

			len = input - cl->inbuffer;
			memmove(cl->inbuffer, input, cl->inbufferlen - len);
			cl->inbufferlen -= len;
		}
#ifdef MULTITHREADWIN32
		if (cl->threadhandle)
			LeaveCriticalSection(&cl->delecatesection);
#endif
	}

	if (cl->outbufferlen)	//still send if grabbed. don't let things build up this side.
	{
#ifdef MULTITHREADWIN32
		if (!cl->threadhandle)
#endif
		{
			len = cl->outbufferlen;
			if (len > 8000)
				len = 8000;
			len = netfuncs->Send(cl->socket, cl->outbuffer, len);
			if (len>0)
			{
				memmove(cl->outbuffer, cl->outbuffer+len, cl->outbufferlen - len);
				cl->outbufferlen -= len;
			}
			if (len == 0)
				cl->tobedropped = true;
			if (len < 0)
			{
				if (len != N_WOULDBLOCK)
				{
					Con_Printf("X send error %i\n", len);
					return true;
				}
			}
		}
	}
	else if ((!xgrabbedclient || xgrabbedclient == cl) && cl->tobedropped)
		return true;	//grabbed servers do not allow altering state if a client drops
	return false;
}

#ifdef MULTITHREAD

#ifdef _WIN32
DWORD WINAPI X_RunClient(void *parm)
#else
void X_RunClient(void *parm)
#endif
{
	char buffer[8192*64];
	int read, len, err;
	xclient_t *cl = parm;

	while(cl->threadhandle)
	{
		if (cl->tobedropped)
		{	//don't bother reading more.
			read = 0;
		}
		else
		{
			read = recv(cl->socket, buffer, sizeof(buffer), 0);
			if (read<0 && !cl->outbufferlen)
			{
				if (qerrno != EWOULDBLOCK)
					cl->tobedropped = true;
				else
				{
					Sleep(1);
					continue;
				}
			}
		}

#ifdef MULTITHREADWIN32
		EnterCriticalSection(&cl->delecatesection);
#endif

		if (read > 0)
		{
			if (cl->inbuffermaxlen < cl->inbufferlen+read)	//expand in buffer
			{
				cl->inbuffermaxlen = cl->inbufferlen+read + 1000;	//add breathing room.
				cl->inbuffer = realloc(cl->inbuffer, cl->inbuffermaxlen);
			}
			memcpy(cl->inbuffer+cl->inbufferlen, buffer, read);
			cl->inbufferlen += read;
		}
		else if (!read)	//no more socket.
			cl->tobedropped = true;
		else
		{ 	//error of some sort
			err = qerrno;
			if (err != EWOULDBLOCK)
				cl->tobedropped = true;
		}

		if (cl->outbufferlen)
		{
			len = cl->outbufferlen;
			if (len > 8000)
				len = 8000;
			len = send(cl->socket, cl->outbuffer, len, 0);	//move out of critical section?
			if (len>0)
			{
				memmove(cl->outbuffer, cl->outbuffer+len, cl->outbufferlen - len);
				cl->outbufferlen -= len;
			}
			if (len == 0)
			{
				cl->tobedropped = true;
				cl->outbufferlen=0;
			}
			if (len < 0)
			{
				err = qerrno;
				if (err != EWOULDBLOCK)
				{
					cl->tobedropped = true;
					cl->outbufferlen=0;
				}
			}
		}

#ifdef MULTITHREADWIN32
		LeaveCriticalSection(&cl->delecatesection);
#endif
	}

	DeleteCriticalSection (&cl->delecatesection);

	closesocket(cl->socket);
	if (cl->inbuffer)
		free(cl->inbuffer);
	if (cl->outbuffer)
		free(cl->outbuffer);
	free(cl);

#ifdef MULTITHREADWIN32
	return 0;
#endif
}

#endif

void XWindows_TendToClients(void)
{
	xclient_t *cl, *prev=NULL;
	qhandle_t newclient;

	if (xlistensocket >= 0)
	{
		newclient = netfuncs->Accept(xlistensocket, NULL, 0);
		if (newclient >= 0)
		{
			cl = malloc(sizeof(xclient_t));
			memset(cl, 0, sizeof(xclient_t));
			cl->socket = newclient;
			cl->nextclient = xclients;
			cl->stillinitialising = 1;
			xclients = cl;


#ifdef MULTITHREADWIN32
			InitializeCriticalSection (&cl->delecatesection);
			{DWORD tid;
			cl->threadhandle = CreateThread(NULL, 0, X_RunClient, cl, 0, &tid);
			}

			if (!cl->threadhandle)
				DeleteCriticalSection (&cl->delecatesection);

			if (ioctlsocket(cl->socket, FIONBIO, &_false) == -1)
				Sys_Error("Nonblocking failed\n");
#endif
		}
	}

	for (cl = xclients; cl; cl = cl->nextclient)
	{
		if (XWindows_TendToClient(cl))
		{
			if (prev)
			{
				prev->nextclient = cl->nextclient;
			}
			else
				xclients = cl->nextclient;

			XS_DestroyResourcesOfClient(cl);

#ifdef MULTITHREADWIN32
			if (cl->threadhandle)
			{
				cl->threadhandle = NULL;
				break;
			}
#endif
			netfuncs->Close(cl->socket);
			if (cl->inbuffer)
				free(cl->inbuffer);
			if (cl->outbuffer)
				free(cl->outbuffer);
			free(cl);
			break;
		}

		prev = cl;
	}
}

#ifdef UNIXSOCKETS
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
int XWindows_UnixListen(int x11display)
{
	char lockfile[256];
	char socketfile[256];
	int lock_fd, ret;

	Q_snprintf(lockfile, sizeof(lockfile), "/tmp/.X%i-lock", x11display);
	Q_snprintf(socketfile, sizeof(socketfile), "/tmp/.X11-unix/X%i", x11display);

	lock_fd = open(lockfile, O_RDONLY | O_CREAT, 0600);
	if (lock_fd == -1)
		return -1;	//can't do it, jim

	// try to acquire lock
	ret = flock(lock_fd, LOCK_EX | LOCK_NB);
	if (ret != 0)
	{
		close(lock_fd);
		return -1;
	}

	// remove socket file
	unlink(socketfile);

	return netfuncs->TCPListen(va("unix://%s", socketfile), baseport+x11display, 3);
}
#endif

void XWindows_Startup(int x11display)	//initialise the server socket and do any initial setup as required.
{
	if (xlistensocket < 0)
	{
#ifdef UNIXSOCKETS
		if (x11display < 0)
		{
			while(xlistensocket < 0)
				xlistensocket = XWindows_UnixListen(++x11display);
		}
		else
			xlistensocket = XWindows_UnixListen(x11display);
#else
		if (x11display < 0)
			x11display = 0;
		xlistensocket = netfuncs->TCPListen(NULL, baseport+x11display, 3);
#endif
		if (xlistensocket < 0)
		{
			xlistensocket = -1;
			Con_Printf("Failed to create tcp listen socket\n");
			return;
		}

		X_InitRequests();
		XS_CreateInitialResources();

		system(va("DISPLAY=127.0.0.1:%i /usr/bin/x-terminal-emulator &", x11display));
	}

//	Menu_Control(MENU_GRAB);
}

extern int x_windowwithfocus;
extern int x_windowwithcursor;
void XWindows_RefreshWindow(xwindow_t *wnd)
{
	xwindow_t *p;
	short xpos;
	short ypos;
	unsigned int *out, *in;

	int x, y;
	int maxx, maxy;

	if (wnd->inputonly)	//no thanks.
		return;

	xpos = 0;
	ypos = 0;
	for (p = wnd->parent; p; p = p->parent)
	{
		xpos += p->xpos;
		ypos += p->ypos;
	}

	y = ypos + wnd->ypos;
	maxy = y + wnd->height;
	if (y < ypos+wnd->ypos)
	{
		y = ypos+wnd->ypos;
	}
	if (y < 0)
		y = 0;
	if (maxy >= xscreenheight)
		maxy = xscreenheight-1;

	if (!wnd->mapped)//&&rand()&1)
	{	//unmapped windows are invisible.
		return;
	}


	{
		/*if (x_windowwithcursor == wnd->res.id)
		{
			for (; y < maxy; y++)
			{
				x = xpos + wnd->xpos;
				maxx = x + wnd->width;
				if (x < xpos+wnd->xpos)
				{
					x = xpos+wnd->xpos;
				}
				if (x < 0)
					x = 0;
				if (maxx > xscreenwidth)
					maxx = xscreenwidth;

				out = (unsigned int *)xscreen + (x+(y*xscreenwidth));

				for (; x < maxx; x++)
				{	
					*out++ = ((rand()&0xff)<<16)|((rand()&0xff)<<8)|(rand() & 0xff);
				}
			}
		}
		else */if (wnd->buffer)// && x_windowwithfocus != wnd->res.id)
		{
			for (; y < maxy; y++)
			{
				x = xpos + wnd->xpos;
				maxx = x + wnd->width;
				if (x < xpos+wnd->xpos)
				{
					x = xpos+wnd->xpos;
				}
				if (x < 0)
					x = 0;
				if (maxx > xscreenwidth)
					maxx = xscreenwidth;

				out = (unsigned int *)xscreen + (x+(y*xscreenwidth));
				in = (unsigned int *)wnd->buffer + (x-xpos-wnd->xpos) + (y-ypos-wnd->ypos)*wnd->width;

				for (; x < maxx; x++)
				{	
					*out++ = *in++;
				}
			}
		}
		else
		{


			for (; y < maxy; y++)
			{
				x = xpos + wnd->xpos;
				maxx = x + wnd->width;
				if (x < xpos+wnd->xpos)
				{
					x = xpos+wnd->xpos;
				}
				if (x < 0)
				{
					x = 0;
				}
				if (maxx > xscreenwidth)
					maxx = xscreenwidth;

				out = (unsigned int *)xscreen + (x+(y*xscreenwidth));
				for (; x < maxx; x++)
				{	
					*out++ = wnd->backpixel;
				}
			}
		}
	}

	wnd = wnd->child;
	while(wnd)
	{
		XWindows_RefreshWindow(wnd);
		wnd = wnd->sibling;
	}
}
/*
void XWindows_DrawWindowTree(xwindow_t *wnd, short xofs, short yofs)
{
	int x, y;
	int maxx, maxy;
	unsigned int *out;

	if (wnd->res.owner)
	{
		y = yofs + wnd->ypos;
		maxy = y + wnd->width;
		if (y < 0)
		{
			y = 0;
		}
		if (maxy >= xscreenheight)
			maxy = xscreenheight-1;
		for (y = 0; y < wnd->height; y++)
		{
			x = xofs + wnd->xpos;
			maxx = x + wnd->height;
			if (x < 0)
			{
				x = 0;
			}
			if (maxx >= xscreenwidth)
				maxx = xscreenwidth-1;

			out = (unsigned int *)xscreen + (x+(y*xscreenwidth));

			for (; x < maxx; x++)
			{	
				*out = rand();
				out++;
			}
		}
	}

	xofs += wnd->xpos;
	yofs += wnd->ypos;

	wnd = wnd->child;
	while(wnd)
	{
		XWindows_DrawWindowTree(wnd, xofs, yofs);
		wnd = wnd->sibling;
	}
}
*/

//quakie functions
void XWindows_Init(void)
{
//	Cmd_AddCommand("startx", XWindows_Startup);
}

int x_mousex;
int x_mousey;
int x_mousestate;
int x_windowwithcursor;
int x_windowwithfocus;

int mousestate;

void X_MoveCursorWindow(xwindow_t *ew, int mx, int my, int movemode)
{
	xEvent ev;

#define MAX_WINDOW_CHAIN 64
	int od, nd;
	int d, i;
	xwindow_t *ow;
	xwindow_t *nw = ew;
	xwindow_t *oc[MAX_WINDOW_CHAIN];
	xwindow_t *nc[MAX_WINDOW_CHAIN];


	if (!nw)
		nw = rootwindow;

	/*its already got it*/
	if (nw->res.id == x_windowwithcursor)
		return;

	if (XS_GetResource(x_windowwithcursor, (void**)&ow) != x_window)
		return;

	//build the window chains into a simple list
	od = 0;
	while(ow && od < MAX_WINDOW_CHAIN)
	{
		oc[od++] = ow;
		ow = ow->parent;
	}

	nd = 0;
	while(nw && nd < MAX_WINDOW_CHAIN)
	{
		nc[nd++] = nw;
		nw = nw->parent;
	}

	//both chains have the root at the end
	//walk from the parent (last) window up to the top. if they diverge then we have the relevent common ancestor
	for (d = 0; d < nd && d < od; )
	{
		d++;
		if (nc[nd-d] != oc[od-d])
			break;
	}
	nd -= d;
	od -= d;

	if (!nd)
	{
		/*moved to a parent*/

		//LeaveNotify with detail Inferior is generated on A.
		ev.u.u.type					= LeaveNotify;
		ev.u.u.detail				= NotifyInferior;
		ev.u.enterLeave.time		= plugfuncs->GetMilliseconds();
		ev.u.enterLeave.root		= rootwindow->res.id;
		ev.u.enterLeave.event		= oc[0]->res.id;
		ev.u.enterLeave.child		= None;
		ev.u.enterLeave.rootX		= mx;
		ev.u.enterLeave.rootY		= my;
		ev.u.enterLeave.eventX		= mx - oc[0]->xpos;
		ev.u.enterLeave.eventY		= my - oc[0]->ypos;
		ev.u.enterLeave.state		= mousestate;
		ev.u.enterLeave.mode		= movemode;
		ev.u.enterLeave.flags		= ELFlagSameScreen;
		X_SendInputNotification(&ev, oc[0], LeaveWindowMask);

		//EnterNotify with detail Virtual is generated on each window between A and B exclusive (in that order).
		for (i = od-1; i > 0; i--)
		{
			ev.u.u.type					= EnterNotify;
			ev.u.u.detail				= NotifyVirtual;
			ev.u.enterLeave.time		= plugfuncs->GetMilliseconds();
			ev.u.enterLeave.root		= rootwindow->res.id;
			ev.u.enterLeave.event		= oc[i]->res.id;
			ev.u.enterLeave.child		= oc[i-1]->res.id;
			ev.u.enterLeave.rootX		= mx;
			ev.u.enterLeave.rootY		= my;
			ev.u.enterLeave.eventX		= mx - oc[i]->xpos;
			ev.u.enterLeave.eventY		= my - oc[i]->ypos;
			ev.u.enterLeave.state		= mousestate;
			ev.u.enterLeave.mode		= movemode;
			ev.u.enterLeave.flags		= ELFlagSameScreen;
			X_SendInputNotification(&ev, oc[i], EnterWindowMask);
		}

		//EnterNotify with detail Ancestor is generated on B.
		ev.u.u.type					= EnterNotify;
		ev.u.u.detail				= NotifyInferior;
		ev.u.enterLeave.time		= plugfuncs->GetMilliseconds();
		ev.u.enterLeave.root		= rootwindow->res.id;
		ev.u.enterLeave.event		= nc[0]->res.id;
		ev.u.enterLeave.child		= None;
		ev.u.enterLeave.rootX		= mx;
		ev.u.enterLeave.rootY		= my;
		ev.u.enterLeave.eventX		= mx - nc[0]->xpos;
		ev.u.enterLeave.eventY		= my - nc[0]->ypos;
		ev.u.enterLeave.state		= mousestate;
		ev.u.enterLeave.mode		= movemode;
		ev.u.enterLeave.flags		= ELFlagSameScreen;
		X_SendInputNotification(&ev, nc[0], EnterWindowMask);
	}
	else if (!od)
	{
		/*moved to a child*/
		//LeaveNotify with detail Ancestor is generated on A.
		ev.u.u.type					= LeaveNotify;
		ev.u.u.detail				= NotifyAncestor;
		ev.u.enterLeave.time		= plugfuncs->GetMilliseconds();
		ev.u.enterLeave.root		= rootwindow->res.id;
		ev.u.enterLeave.event		= oc[0]->res.id;
		ev.u.enterLeave.child		= None;
		ev.u.enterLeave.rootX		= mx;
		ev.u.enterLeave.rootY		= my;
		ev.u.enterLeave.eventX		= mx - oc[0]->xpos;
		ev.u.enterLeave.eventY		= my - oc[0]->ypos;
		ev.u.enterLeave.state		= mousestate;
		ev.u.enterLeave.mode		= movemode;
		ev.u.enterLeave.flags		= ELFlagSameScreen;
		X_SendInputNotification(&ev, oc[0], LeaveWindowMask);

		//LeaveNotify with detail Virtual is generated on each window between A and B exclusive (in that order).
		for (i = 1; i < nd; i++)
		{
			ev.u.u.type					= LeaveNotify;
			ev.u.u.detail				= NotifyVirtual;
			ev.u.enterLeave.time		= plugfuncs->GetMilliseconds();
			ev.u.enterLeave.root		= rootwindow->res.id;
			ev.u.enterLeave.event		= nc[i]->res.id;
			ev.u.enterLeave.child		= nc[i-1]->res.id;
			ev.u.enterLeave.rootX		= mx;
			ev.u.enterLeave.rootY		= my;
			ev.u.enterLeave.eventX		= mx - nc[i]->xpos;
			ev.u.enterLeave.eventY		= my - nc[i]->ypos;
			ev.u.enterLeave.state		= mousestate;
			ev.u.enterLeave.mode		= movemode;
			ev.u.enterLeave.flags		= ELFlagSameScreen;
			X_SendInputNotification(&ev, nc[i], LeaveWindowMask);
		}

		//EnterNotify with detail Inferior is generated on B.
		ev.u.u.type					= EnterNotify;
		ev.u.u.detail				= NotifyInferior;
		ev.u.enterLeave.time		= plugfuncs->GetMilliseconds();
		ev.u.enterLeave.root		= rootwindow->res.id;
		ev.u.enterLeave.event		= nc[0]->res.id;
		ev.u.enterLeave.child		= None;
		ev.u.enterLeave.rootX		= mx;
		ev.u.enterLeave.rootY		= my;
		ev.u.enterLeave.eventX		= mx - nc[0]->xpos;
		ev.u.enterLeave.eventY		= my - nc[0]->ypos;
		ev.u.enterLeave.state		= mousestate;
		ev.u.enterLeave.mode		= movemode;
		ev.u.enterLeave.flags		= ELFlagSameScreen;
		X_SendInputNotification(&ev, nc[0], EnterWindowMask);
	}
	else
	{
		/*moved up then down*/

		//LeaveNotify with detail Nonlinear is generated on A.
		ev.u.u.type					= LeaveNotify;
		ev.u.u.detail				= NotifyNonlinear;
		ev.u.enterLeave.time		= plugfuncs->GetMilliseconds();
		ev.u.enterLeave.root		= rootwindow->res.id;
		ev.u.enterLeave.event		= oc[0]->res.id;
		ev.u.enterLeave.child		= None;
		ev.u.enterLeave.rootX		= mx;
		ev.u.enterLeave.rootY		= my;
		ev.u.enterLeave.eventX		= mx - oc[0]->xpos;
		ev.u.enterLeave.eventY		= my - oc[0]->ypos;
		ev.u.enterLeave.state		= mousestate;
		ev.u.enterLeave.mode		= movemode;
		ev.u.enterLeave.flags		= ELFlagSameScreen;
		X_SendInputNotification(&ev, oc[0], LeaveWindowMask);

		//LeaveNotify with detail NonlinearVirtual is generated on each window between A and C exclusive (in that order).
		for (i = 1; i < nd; i++)
		{
			ev.u.u.type					= LeaveNotify;
			ev.u.u.detail				= NotifyNonlinearVirtual;
			ev.u.enterLeave.time		= plugfuncs->GetMilliseconds();
			ev.u.enterLeave.root		= rootwindow->res.id;
			ev.u.enterLeave.event		= nc[i]->res.id;
			ev.u.enterLeave.child		= nc[i-1]->res.id;
			ev.u.enterLeave.rootX		= mx;
			ev.u.enterLeave.rootY		= my;
			ev.u.enterLeave.eventX		= mx - nc[i]->xpos;
			ev.u.enterLeave.eventY		= my - nc[i]->ypos;
			ev.u.enterLeave.state		= mousestate;
			ev.u.enterLeave.mode		= movemode;
			ev.u.enterLeave.flags		= ELFlagSameScreen;
			X_SendInputNotification(&ev, nc[i], LeaveWindowMask);
		}
		//EnterNotify with detail NonlinearVirtual is generated on each window between C and B exclusive (in that order).
		for (i = od-1; i > 0; i--)
		{
			ev.u.u.type					= EnterNotify;
			ev.u.u.detail				= NotifyNonlinearVirtual;
			ev.u.enterLeave.time		= plugfuncs->GetMilliseconds();
			ev.u.enterLeave.root		= rootwindow->res.id;
			ev.u.enterLeave.event		= oc[i]->res.id;
			ev.u.enterLeave.child		= oc[i-1]->res.id;
			ev.u.enterLeave.rootX		= mx;
			ev.u.enterLeave.rootY		= my;
			ev.u.enterLeave.eventX		= mx - oc[i]->xpos;
			ev.u.enterLeave.eventY		= my - oc[i]->ypos;
			ev.u.enterLeave.state		= mousestate;
			ev.u.enterLeave.mode		= movemode;
			ev.u.enterLeave.flags		= ELFlagSameScreen;
			X_SendInputNotification(&ev, oc[i], EnterWindowMask);
		}

		//EnterNotify with detail Nonlinear is generated on B.
		ev.u.u.type					= EnterNotify;
		ev.u.u.detail				= NotifyNonlinear;
		ev.u.enterLeave.time		= plugfuncs->GetMilliseconds();
		ev.u.enterLeave.root		= rootwindow->res.id;
		ev.u.enterLeave.event		= nc[0]->res.id;
		ev.u.enterLeave.child		= None;
		ev.u.enterLeave.rootX		= mx;
		ev.u.enterLeave.rootY		= my;
		ev.u.enterLeave.eventX		= mx - nc[0]->xpos;
		ev.u.enterLeave.eventY		= my - nc[0]->ypos;
		ev.u.enterLeave.state		= mousestate;
		ev.u.enterLeave.mode		= movemode;
		ev.u.enterLeave.flags		= ELFlagSameScreen;
		X_SendInputNotification(&ev, nc[0], EnterWindowMask);
	}
}

void X_EvalutateCursorOwner(int movemode)
{
	xEvent ev;
	xwindow_t *cursorowner, *wnd, *use;
	float mx, my;
	int wcx;
	int wcy;

	extern xwindow_t *xpconfinewindow;

	{
		mx = mousecursor_x;
		my = mousecursor_y;
	}
	if (mx >= xscreenwidth)
		mx = xscreenwidth-1;
	if (my >= xscreenheight)
		my = xscreenheight-1;
	if (mx < 0)
		mx = 0;
	if (my < 0)
		my = 0;

	if (xpconfinewindow)	//don't leave me!
	{
		cursorowner = xpconfinewindow;

		wcx = 0; wcy = 0;

		for (wnd = cursorowner; wnd; wnd = wnd->parent)
		{
			wcx += wnd->xpos;
			wcy += wnd->ypos;
		}

		if (movemode == NotifyNormal)
			movemode = NotifyWhileGrabbed;
	}
	else
	{
		cursorowner = rootwindow;
		wcx = 0; wcy = 0;
		while(1)
		{
			use = NULL;
			//find the last window that contains the pointer (lower windows come first)
			for (wnd = cursorowner->child; wnd; wnd = wnd->sibling)
			{
				if (/*!wnd->inputonly && */wnd->mapped)
					if (wcx+wnd->xpos <= mx && wcx+wnd->xpos+wnd->width >= mx)
					{
						if (wcy+wnd->ypos <= my && wcy+wnd->ypos+wnd->height >= my)
						{
							use = wnd;
						}
					}
			}
			wnd = use;

			if (wnd)
			{
				cursorowner = wnd;
				wcx += wnd->xpos;
				wcy += wnd->ypos;
				continue;
			}
			break;
		}
	}

	if (mx != x_mousex || my != x_mousey || x_mousestate != mousestate || x_windowwithcursor != cursorowner->res.id)
	{
		int mask = 0;
//		extern qboolean	keydown[256];

//		Con_Printf("move %i %i\n", mx, my);

		X_MoveCursorWindow(cursorowner, mx, my, movemode);
		x_windowwithcursor = cursorowner->res.id;

		if (mx != x_mousex || my != x_mousey)
		{
			mask |= PointerMotionMask;
			if (mousestate)
				mask |= ButtonMotionMask;
		}
		if ((mousestate^x_mousestate) & Button1Mask)
			mask |= Button1MotionMask;
		if ((mousestate^x_mousestate) & Button2Mask)
			mask |= Button2MotionMask;
		if ((mousestate^x_mousestate) & Button3Mask)
			mask |= Button3MotionMask;
		if ((mousestate^x_mousestate) & Button4Mask)
			mask |= Button4MotionMask;
		if ((mousestate^x_mousestate) & Button5Mask)
			mask |= Button5MotionMask;

		x_mousex = mx;
		x_mousey = my;
		x_mousestate = mousestate;

		for (; cursorowner && mask; cursorowner = cursorowner->parent)
		{	//same window

			if (cursorowner->notificationmasks & mask)
			{
				ev.u.keyButtonPointer.child		= x_windowwithcursor;

/*				#define ButtonPress		4
#define ButtonRelease		5
#define MotionNotify		6
*/
				ev.u.u.type						= MotionNotify;
				ev.u.u.detail					= 0;
				ev.u.u.sequenceNumber			= 0;
				ev.u.keyButtonPointer.time		= plugfuncs->GetMilliseconds();
				ev.u.keyButtonPointer.root		= rootwindow->res.id;
				ev.u.keyButtonPointer.event		= cursorowner->res.id;
				ev.u.keyButtonPointer.child		= (x_windowwithcursor == cursorowner->res.id)?None:x_windowwithcursor;
				ev.u.keyButtonPointer.rootX		= mx;
				ev.u.keyButtonPointer.rootY		= my;
				ev.u.keyButtonPointer.eventX	= mx - cursorowner->xpos;
				ev.u.keyButtonPointer.eventY	= my - cursorowner->ypos;
				ev.u.keyButtonPointer.state		= mousestate;
				ev.u.keyButtonPointer.sameScreen= true;

				X_SendNotificationMasked(&ev, cursorowner, cursorowner->notificationmasks&mask);

				mask &= ~cursorowner->notificationmasks;
			}
		}
	}
}

void X_EvalutateFocus(int movemode)
{
	xEvent ev;
	xwindow_t *fo, *po, *wnd;

	if (XS_GetResource(x_windowwithcursor, (void**)&po) != x_window)
		po = rootwindow;

//	xfocusedwindow = NULL;


	if (!xfocusedwindow)
	{
		if (XS_GetResource(x_windowwithcursor, (void**)&fo) != x_window)
			fo = rootwindow;
	}
	else
	{
		fo = xfocusedwindow;
	}

	if (x_windowwithfocus != fo->res.id)
	{
		ev.u.u.detail					= 0;
		ev.u.u.sequenceNumber			= 0;
		ev.u.focus.mode					= movemode;
		{
			xwindow_t *a,*b;
			int d1,d2;

			if (XS_GetResource(x_windowwithfocus, (void**)&wnd) != x_window)
				wnd = rootwindow;

			x_windowwithfocus = fo->res.id;

			//count how deep the windows are
			for (a = wnd,d1=0; a; a = a->parent)
				d1++;
			for (b = fo,d2=0; b; b = b->parent)
				d2++;

			a = wnd;
			b = fo;

			if (d1>d2)
			{
				while(d1>d2)	//a is too deep
				{
					a = a->parent;
					d1--;
				}
			}
			else
			{
				while(d2>d1)
				{
					b = b->parent;
					d2--;
				}
			}
			while(a != b)	//find the common ancestor.
			{
				a = a->parent;
				b = b->parent;
			}

			ev.u.enterLeave.mode = movemode;
			ev.u.enterLeave.flags = ELFlagSameScreen;		/* sameScreen and focus booleans, packed together */

			//the cursor moved from a to b via:
//			if (!a)	//changed screen...
//			{
//			} else
			if (a != wnd && b != fo)
			{	//changed via a common root, indirectly.

//When the focus moves from window A to window B, window C is
//their least common ancestor, and the pointer is in window P:

//o    If P is an inferior of A, FocusOut with detail Pointer
//     is generated on each window from P up to but not
//     including A (in order).

	 //FIXME

//o    FocusOut with detail Nonlinear is generated on A.

				ev.u.u.type						= FocusOut;
				ev.u.u.detail					= NotifyNonlinear;
				ev.u.focus.window				= wnd->res.id;
				X_SendInputNotification(&ev, wnd, FocusChangeMask);




//o    FocusOut with detail NonlinearVirtual is generated on
//     each window between A and C exclusive (in order).

				for (a = wnd->parent; a != b; a = a->parent)
				{
					ev.u.u.type						= FocusOut;
					ev.u.u.detail					= NotifyNonlinearVirtual;
					ev.u.focus.window				= a->res.id;
					X_SendInputNotification(&ev, a, FocusChangeMask);
				}

//o    FocusIn with detail NonlinearVirtual is generated on
//     each window between C and B exclusive (in order).

				for (; b != fo; )
				{
					ev.u.u.type						= FocusIn;
					ev.u.u.detail					= NotifyNonlinearVirtual;
					ev.u.focus.window		= a->res.id;
					X_SendInputNotification(&ev, a, FocusChangeMask);

					for (a = fo; ; a = a->parent)	//we need to go through the children.
					{
						if (a->parent == b)
						{
							b = a;
							break;
						}
					}
				}

//o    FocusIn with detail Nonlinear is generated on B.

				ev.u.u.type						= FocusIn;
				ev.u.u.detail					= NotifyNonlinear;
				ev.u.focus.window		= fo->res.id;
				X_SendInputNotification(&ev, fo, FocusChangeMask);

//o    If P is an inferior of B, FocusIn with detail Pointer
//     is generated on each window below B down to and includ-
//     ing P (in order).


	//FIXME:

			}
			else if (a == wnd)
			{	//b is a child of a

//When the focus moves from window A to window B, B is an
//inferior of A, and the pointer is in window P:

//o    If P is an inferior of A but P is not an inferior of B
//     or an ancestor of B, FocusOut with detail Pointer is
//     generated on each window from P up to but not including
//     A (in order).

	//FIXME

//o    FocusOut with detail Inferior is generated on A.

				ev.u.u.type						= FocusOut;
				ev.u.u.detail					= NotifyInferior;
				ev.u.focus.window		= wnd->res.id;
				X_SendInputNotification(&ev, wnd, FocusChangeMask);

//o    FocusIn with detail Virtual is generated on each window
//     between A and B exclusive (in order).

				if (wnd != fo)
				for (b = wnd; ; )
				{
					for (a = fo; ; a = a->parent)	//we need to go through the children.
					{
						if (a->parent == b)
						{
							b = a;
							break;
						}
					}
					if (b == fo)
						break;

					ev.u.u.type						= FocusIn;
					ev.u.u.detail					= NotifyVirtual;
					ev.u.focus.window		= b->res.id;
					X_SendInputNotification(&ev, b, FocusChangeMask);
				}

//o    FocusIn with detail Ancestor is generated on B.

				ev.u.u.type						= FocusIn;
				ev.u.u.detail					= NotifyAncestor;
				ev.u.focus.window		= fo->res.id;
				X_SendInputNotification(&ev, fo, FocusChangeMask);
			}
			else// if (b == cursorowner)
			{	//a is a child of b

//When the focus moves from window A to window B, A is an
//inferior of B, and the pointer is in window P:

//o    FocusOut with detail Ancestor is generated on A.

				ev.u.u.type						= FocusOut;
				ev.u.u.detail					= NotifyAncestor;
				ev.u.focus.window				= wnd->res.id;
				X_SendInputNotification(&ev, wnd, FocusChangeMask);

//o    FocusOut with detail Virtual is generated on each win-
//     dow between A and B exclusive (in order).

				for (b = wnd; ; )
				{
					b = b->parent;
					if (b == fo)
						break;

					ev.u.u.type						= FocusOut;
					ev.u.u.detail					= NotifyVirtual;
					ev.u.focus.window				= a->res.id;
					X_SendInputNotification(&ev, a, FocusChangeMask);
				}


//o    FocusIn with detail Inferior is generated on B.

				ev.u.u.type						= FocusIn;
				ev.u.u.detail					= NotifyInferior;
				ev.u.focus.window				= fo->res.id;
				X_SendInputNotification(&ev, fo, FocusChangeMask);

//o    If P is an inferior of B but P is not A or an inferior
//     of A or an ancestor of A, FocusIn with detail Pointer
//     is generated on each window below B down to and includ-
//     ing P (in order).

	//FIXME: code missing
			}
		}
	}
}

void XWindows_Draw(void)
{
	{
		X_EvalutateCursorOwner(NotifyNormal);
	}

	XWindows_TendToClients();

/*	if (rand()&15 == 15)
		xrefreshed = true;*/

//	memset(xscreen, 0, xscreenwidth*4*xscreenheight);

	XWindows_TendToClients();

//	XW_ExposeWindow(rootwindow, 0, 0, rootwindow->width, rootwindow->height);

//	XWindows_DrawWindowTree(rootwindow, 0, 0);
//	if (xrefreshed)
	{
		XWindows_RefreshWindow(rootwindow);
		xrefreshed = false;
		xscreenmodified = true; 
//		Con_Printf("updated screen\n");
	}

	{
		unsigned int *out = (unsigned int *)xscreen + (x_mousex+(x_mousey*xscreenwidth));
		*out = rand();
//		out[64] = rand();
	}

	XWindows_TendToClients();

//	Con_DrawNotify();

	XWindows_TendToClients();
}

void XWindows_KeyDown(int key)
{
	if (!key)	//hrm
		return;
/*
	if (key == 'q' || (key == K_BACKSPACE && ctrldown && altdown))	//kill off the server
	{	//explicit kill
		Menu_Control(MENU_CLEAR);
		return;
	}
*/
	if (key == K_CTRL)
		ctrldown = true;
	if (key == K_ALT)
		altdown = true;


	{
		xEvent ev;
		xwindow_t *wnd;

		X_EvalutateCursorOwner(NotifyNormal);

		X_EvalutateFocus(NotifyNormal);

		if (key == K_MOUSE1)
		{
			ev.u.u.type						= ButtonPress;
			ev.u.u.detail					= Button1;
			mousestate						|= Button1Mask;
			ev.u.keyButtonPointer.state		= mousestate;
			ev.u.keyButtonPointer.child		= x_windowwithcursor;
		}
		else if (key == K_MOUSE3)
		{
			ev.u.u.type						= ButtonPress;
			ev.u.u.detail					= Button2;
			mousestate						|= Button2Mask;
			ev.u.keyButtonPointer.state		= mousestate;
			ev.u.keyButtonPointer.child		= x_windowwithcursor;
		}
		else if (key == K_MOUSE2)
		{
			ev.u.u.type						= ButtonPress;
			ev.u.u.detail					= Button3;
			mousestate						|= Button3Mask;
			ev.u.keyButtonPointer.state		= mousestate;
			ev.u.keyButtonPointer.child		= x_windowwithcursor;
		}
		else if (key == K_MOUSE4)
		{
			ev.u.u.type						= ButtonPress;
			ev.u.u.detail					= Button4;
			mousestate						|= Button4Mask;
			ev.u.keyButtonPointer.state		= mousestate;
			ev.u.keyButtonPointer.child		= x_windowwithcursor;
		}
		else if (key == K_MOUSE5)
		{
			ev.u.u.type						= ButtonPress;
			ev.u.u.detail					= Button5;
			mousestate						|= Button5Mask;
			ev.u.keyButtonPointer.state		= mousestate;
			ev.u.keyButtonPointer.child		= x_windowwithcursor;
		}
		else
		{
			ev.u.u.type						= KeyPress;
			ev.u.u.detail					= QKeyToScan(key);
			if (!ev.u.u.detail)
				return;	//urm, never mind
			ev.u.keyButtonPointer.state		= 0;
			ev.u.keyButtonPointer.child		= x_windowwithfocus;
		}
		ev.u.u.sequenceNumber			= 0;
		ev.u.keyButtonPointer.time		= plugfuncs->GetMilliseconds();
		ev.u.keyButtonPointer.rootX		= x_mousex;
		ev.u.keyButtonPointer.rootY		= x_mousey;
		ev.u.keyButtonPointer.sameScreen= true;
		ev.u.keyButtonPointer.pad1		= 0;

//		Con_Printf("key %i, %i %i\n", key, x_mousex, x_mousey);

		if (0)//xpointergrabclient)
		{
			ev.u.keyButtonPointer.event		= ev.u.keyButtonPointer.child;
			ev.u.keyButtonPointer.eventX	= ev.u.keyButtonPointer.rootX;
			ev.u.keyButtonPointer.eventY	= ev.u.keyButtonPointer.rootY;
			if (XS_GetResource(x_windowwithcursor, (void**)&wnd) == x_window)
			{
				ev.u.u.sequenceNumber = xpointergrabclient->requestnum;
				while(wnd)
				{
					ev.u.keyButtonPointer.eventX -= wnd->xpos;
					ev.u.keyButtonPointer.eventY -= wnd->ypos;
					wnd = wnd->parent;
				}
				X_SendData(xpointergrabclient, &ev, sizeof(ev));
			}
		}
		else if (XS_GetResource(ev.u.keyButtonPointer.child, (void**)&wnd) == x_window)
			X_SendInputNotification(&ev, wnd, (ev.u.u.type==ButtonPress)?ButtonPressMask:KeyPressMask);
	}
}
void XWindows_Keyup(int key)
{
	if (key == K_CTRL)
		ctrldown = false;
	if (key == K_ALT)
		altdown = false;

	{
		xEvent ev;
		xwindow_t *wnd;

		X_EvalutateCursorOwner(NotifyNormal);

		X_EvalutateFocus(NotifyNormal);

		if (key == K_MOUSE1)
		{
			ev.u.u.type						= ButtonRelease;
			ev.u.keyButtonPointer.state		= mousestate;
			ev.u.u.detail					= Button1;
			mousestate						&= ~Button1Mask;
			ev.u.keyButtonPointer.state		= mousestate;
			ev.u.keyButtonPointer.child		= x_windowwithcursor;
		}
		else if (key == K_MOUSE3)
		{
			ev.u.u.type						= ButtonRelease;
			ev.u.keyButtonPointer.state		= mousestate;
			ev.u.u.detail					= Button2;
			mousestate						&= ~Button2Mask;
			ev.u.keyButtonPointer.state		= mousestate;
			ev.u.keyButtonPointer.child		= x_windowwithcursor;
		}
		else if (key == K_MOUSE2)
		{
			ev.u.u.type						= ButtonRelease;
			ev.u.keyButtonPointer.state		= mousestate;
			ev.u.u.detail					= Button3;
			mousestate						&= ~Button3Mask;
			ev.u.keyButtonPointer.state		= mousestate;
			ev.u.keyButtonPointer.child		= x_windowwithcursor;
		}
		else if (key == K_MOUSE4)
		{
			ev.u.u.type						= ButtonRelease;
			ev.u.keyButtonPointer.state		= mousestate;
			ev.u.u.detail					= Button4;
			mousestate						&= ~Button4Mask;
			ev.u.keyButtonPointer.state		= mousestate;
			ev.u.keyButtonPointer.child		= x_windowwithcursor;
		}
		else if (key == K_MOUSE5)
		{
			ev.u.u.type						= ButtonRelease;
			ev.u.keyButtonPointer.state		= mousestate;
			ev.u.u.detail					= Button5;
			mousestate						&= ~Button5Mask;
			ev.u.keyButtonPointer.state		= mousestate;
			ev.u.keyButtonPointer.child		= x_windowwithcursor;
		}
		else
		{
			ev.u.u.type					= KeyRelease;
			ev.u.u.detail				= QKeyToScan(key);
			if (!ev.u.u.detail)
				return;	//urm, never mind
			ev.u.keyButtonPointer.child		= x_windowwithfocus;
		}
		ev.u.u.sequenceNumber			= 0;
		ev.u.keyButtonPointer.time		= plugfuncs->GetMilliseconds();
		ev.u.keyButtonPointer.rootX		= x_mousex;
		ev.u.keyButtonPointer.rootY		= x_mousey;
		ev.u.keyButtonPointer.state		= 0;
		ev.u.keyButtonPointer.sameScreen= true;
		ev.u.keyButtonPointer.pad1		= 0;

//		Con_Printf("keyup %i, %i %i\n", key, x_mousex, x_mousey);

		if (xpointergrabclient)
		{
			ev.u.keyButtonPointer.event		= ev.u.keyButtonPointer.child;
			ev.u.keyButtonPointer.eventX	= ev.u.keyButtonPointer.rootX;
			ev.u.keyButtonPointer.eventY	= ev.u.keyButtonPointer.rootY;
			if (XS_GetResource(x_windowwithcursor, (void**)&wnd) == x_window)
			{
				ev.u.u.sequenceNumber = xpointergrabclient->requestnum;
				while(wnd)
				{
					ev.u.keyButtonPointer.eventX -= wnd->xpos;
					ev.u.keyButtonPointer.eventY -= wnd->ypos;
					wnd = wnd->parent;
				}
				X_SendData(xpointergrabclient, &ev, sizeof(ev));
			}
		}
		else if (XS_GetResource(ev.u.keyButtonPointer.child, (void**)&wnd) == x_window)
		{
			X_SendInputNotification(&ev, wnd, (ev.u.u.type==ButtonRelease)?ButtonReleaseMask:KeyReleaseMask);
		}
	}
}

/*qboolean QDECL X11_MenuEvent(int eventtype, int param, float mousecursor_x, float mousecursor_y)
{
	mousecursor_x = args[2];
	mousecursor_y = args[3];
	switch(args[0])
	{
	case 0:	//draw
		XWindows_Draw();
		break;
	case 1:	//keydown
		XWindows_KeyDown(args[1]);
		break;
	case 2:	//keyup
		XWindows_Keyup(args[1]);
		break;
	case 3:	//menu closed (this is called even if we change it).
		break;
	}

	return 0;
}*/

static int X11_ExecuteCommand(qboolean isinsecure)
{
	char cmd[256];
	cmdfuncs->Argv(0, cmd, sizeof(cmd));
	if (!strcmp("startx", cmd))
	{
		if (confuncs)
		{
			const char *console = "x11";
			if (confuncs->GetConsoleFloat(console, "iswindow") != true)
			{
				confuncs->SetConsoleString(console, "title", "X11");
				confuncs->SetConsoleFloat(console, "iswindow", true);
				confuncs->SetConsoleFloat(console, "forceutf8", true);
				confuncs->SetConsoleFloat(console, "linebuffered", false);
				confuncs->SetConsoleFloat(console, "maxlines", 0);
				confuncs->SetConsoleFloat(console, "wnd_x", 0);
				confuncs->SetConsoleFloat(console, "wnd_y", 0);
				confuncs->SetConsoleFloat(console, "wnd_w", 640);
				confuncs->SetConsoleFloat(console, "wnd_h", 480);
				confuncs->SetConsoleString(console, "footer", "");
			}
			confuncs->SetConsoleString(console, "backvideomap", "x11");
			confuncs->SetActive(console);
		}
		else
		{
			cmdfuncs->Argv(1, cmd, sizeof(cmd));
			XWindows_Startup(*cmd?atoi(cmd):-1);
		}
		return 1;
	}
	return 0;
}

static void QDECL X11_Tick(double realtime, double gametime)
{
	XWindows_TendToClients();
}

static void *XWindows_Create(const char *medianame)	//initialise the server socket and do any initial setup as required.
{
	if (!strcmp(medianame, "x11"))
	{
		XWindows_Startup(-1);
		return xscreen;
	}
	return NULL;
}

static qboolean VARGS XWindows_DisplayFrame(void *ctx, qboolean nosound, qboolean forcevideo, double mediatime, void (QDECL *uploadtexture)(void *ectx, uploadfmt_t fmt, int width, int height, void *data, void *palette), void *ectx)
{
	XWindows_Draw();

	if (forcevideo || xscreenmodified)
		uploadtexture(ectx, TF_BGRX32, xscreenwidth, xscreenheight, xscreen, NULL);
	xscreenmodified = false;

	return true;
}

static void XWindows_Shutdown(void *ctx)
{
	netfuncs->Close(xlistensocket);
	xlistensocket = -1;
}

static qboolean XWindows_SetSize (void *ctx, int width, int height)
{
	qbyte *ns;
	if (width < 64 || height < 64 || width > 16384 || height > 16384)
		return false;

	ns = realloc(xscreen, width*4*height);
	if (ns)
	{
		xscreen = ns;
		xscreenwidth = width;
		xscreenheight = height;
		xscreenmodified = true;

		if (rootwindow)
		{
			X_Resize(rootwindow, 0, 0, width, height);
			XW_ExposeWindow(rootwindow, 0, 0, rootwindow->width, rootwindow->height);
		}
		return true;
	}
	return false;
}

static void XWindows_GetSize (void *ctx, int *width, int *height)	//retrieves the screen-space size
{
	*width = xscreenwidth;
	*height = xscreenheight;
}

static void XWindows_CursorMove (void *ctx, float posx, float posy)
{
	mousecursor_x = (int)(posx * xscreenwidth);
	mousecursor_y = (int)(posy * xscreenheight);
}

static void XWindows_Key (void *ctx, int code, int unicode, int isup)
{
	if (isup)
		XWindows_Keyup(code);
	else
		XWindows_KeyDown(code);
}

media_decoder_funcs_t decoderfuncs =
{
	sizeof(media_decoder_funcs_t),
	"x11",
	XWindows_Create,
	XWindows_DisplayFrame,
	XWindows_Shutdown,
	NULL,//rewind

	XWindows_CursorMove,
	XWindows_Key,
	XWindows_SetSize,
	XWindows_GetSize,
	NULL//changestream
};


qboolean Plug_Init(void)
{
	netfuncs = plugfuncs->GetEngineInterface(plugnetfuncs_name, sizeof(*netfuncs));
	confuncs = plugfuncs->GetEngineInterface(plugsubconsolefuncs_name, sizeof(*confuncs));

	if (!netfuncs ||
		!plugfuncs->ExportFunction("ExecuteCommand", X11_ExecuteCommand) ||
//		!plugfuncs->ExportFunction("MenuEvent", X11_MenuEvent) ||
		!plugfuncs->ExportFunction("Tick", X11_Tick))
	{
		Con_Printf("XServer plugin failed\n");
		return false;
	}

	if (!plugfuncs->ExportInterface("Media_VideoDecoder", &decoderfuncs, sizeof(decoderfuncs)))
	{
		Con_Printf("XServer plugin failed: Engine doesn't support media decoder plugins\n");
		return false;
	}

	Con_Printf("XServer plugin started\n");

	cmdfuncs->AddCommand("startx");

#ifndef K_CTRL
	K_CTRL			= pKey_GetKeyCode("ctrl");
	K_ALT			= pKey_GetKeyCode("alt");
	K_MOUSE1		= pKey_GetKeyCode("mouse1");
	K_MOUSE2		= pKey_GetKeyCode("mouse2");
	K_MOUSE3		= pKey_GetKeyCode("mouse3");
	K_MOUSE4		= pKey_GetKeyCode("mouse4");
	K_MOUSE5		= pKey_GetKeyCode("mouse5");
	K_BACKSPACE		= pKey_GetKeyCode("backspace");
/*
	K_UPARROW		= Key_GetKeyCode("uparrow");
	K_DOWNARROW		= Key_GetKeyCode("downarrow");
	K_ENTER			= Key_GetKeyCode("enter");
	K_DEL			= Key_GetKeyCode("del");
	K_ESCAPE		= Key_GetKeyCode("escape");
	K_PGDN			= Key_GetKeyCode("pgdn");
	K_PGUP			= Key_GetKeyCode("pgup");
	K_SPACE			= Key_GetKeyCode("space");
	K_LEFTARROW		= Key_GetKeyCode("leftarrow");
	K_RIGHTARROW	= Key_GetKeyCode("rightarrow");
*/
#endif
	return true;
}