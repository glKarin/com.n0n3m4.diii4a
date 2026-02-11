/*
Copyright (C) 2006-2007 Mark Olsen

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include <exec/exec.h>
#include <intuition/intuition.h>
#include <intuition/extensions.h>
#include <intuition/intuitionbase.h>
#include <devices/input.h>

#include <proto/exec.h>
#include <proto/intuition.h>

#include <clib/alib_protos.h>

#include "quakedef.h"
#include "input.h"

#include "in_morphos.h"

struct InputEvent imsgs[MAXIMSGS];
extern struct IntuitionBase *IntuitionBase;
extern struct Window *window;
extern struct Screen *screen;

int imsglow = 0;
int imsghigh = 0;

extern qboolean mouse_active;

static struct Interrupt InputHandler;

static struct Interrupt InputHandler;
static struct MsgPort *inputport = 0;
static struct IOStdReq *inputreq = 0;
static BYTE inputret = -1;

#define DEBUGRING(x)

void INS_Shutdown(void)
{
	if (inputret == 0)
	{
		inputreq->io_Data = (void *)&InputHandler;
		inputreq->io_Command = IND_REMHANDLER;
		DoIO((struct IORequest *)inputreq);

		CloseDevice((struct IORequest *)inputreq);

		inputret = -1;
	}

	if (inputreq)
	{
		DeleteStdIO(inputreq);

		inputreq = 0;
	}

	if (inputport)
	{
		DeletePort(inputport);

		inputport = 0;
	}
}

void INS_ReInit()
{
/*	Cvar_Register (&m_filter, "input controls");*/

	if (inputport)
		return;

	inputport = CreatePort(0, 0);
	if (inputport == 0)
	{
		IN_Shutdown();
		Sys_Error("Unable to create message port");
	}

	inputreq = CreateStdIO(inputport);
	if (inputreq == 0)
	{
		IN_Shutdown();
		Sys_Error("Unable to create IO request");
	}

	inputret = OpenDevice("input.device", 0, (struct IORequest *)inputreq, 0);
	if (inputret != 0)
	{
		IN_Shutdown();
		Sys_Error("Unable to open input.device");
	}

	InputHandler.is_Node.ln_Type = NT_INTERRUPT;
	InputHandler.is_Node.ln_Pri = 100;
	InputHandler.is_Node.ln_Name = "FTEQW input handler";
	InputHandler.is_Data = 0;
	InputHandler.is_Code = (void(*)())&myinputhandler;
	inputreq->io_Data = (void *)&InputHandler;
	inputreq->io_Command = IND_ADDHANDLER;
	DoIO((struct IORequest *)inputreq);
}

void INS_Init(void)
{
	INS_ReInit();
}

//IN_KeyEvent is threadsafe (for one other thread, anyway)
void INS_ProcessInputMessage(struct InputEvent *msg, qboolean consumemotion)
{
	int key;
	qboolean down;
	int code;

	if ((window->Flags & WFLG_WINDOWACTIVE))
	{
		if (msg->ie_Class == IECLASS_NEWMOUSE)
		{
			key = 0;

			if (msg->ie_Code == NM_WHEEL_UP)
				key = K_MWHEELUP;
			else if (msg->ie_Code == NM_WHEEL_DOWN)
				key = K_MWHEELDOWN;

			if (msg->ie_Code == NM_BUTTON_FOURTH)
			{
				IN_KeyEvent(0, true, K_MOUSE4, 0);
			}
			else if (msg->ie_Code == (NM_BUTTON_FOURTH|IECODE_UP_PREFIX))
			{
				IN_KeyEvent(0, false, K_MOUSE4, 0);
			}

			if (key)
			{
				IN_KeyEvent(0, true, key, 0);
				IN_KeyEvent(0, false, key, 0);
			}

		}
		else if (msg->ie_Class == IECLASS_RAWKEY)
		{
			down = !(msg->ie_Code&IECODE_UP_PREFIX);
			code = msg->ie_Code & ~IECODE_UP_PREFIX;

			key = 0;
			if (code <= 255)
				key = keyconv[code];

			if (key)
				IN_KeyEvent(0, down, key, key);
			else
			{
//				if (developer.value)
//					printf("Unknown key %d\n", msg->ie_Code);
			}
		}

		else if (msg->ie_Class == IECLASS_RAWMOUSE)
		{
			if (msg->ie_Code == IECODE_LBUTTON)
				IN_KeyEvent(0, true, K_MOUSE1, 0);
			else if (msg->ie_Code == (IECODE_LBUTTON|IECODE_UP_PREFIX))
				IN_KeyEvent(0, false, K_MOUSE1, 0);
			else if (msg->ie_Code == IECODE_RBUTTON)
				IN_KeyEvent(0, true, K_MOUSE2, 0);
			else if (msg->ie_Code == (IECODE_RBUTTON|IECODE_UP_PREFIX))
				IN_KeyEvent(0, false, K_MOUSE2, 0);
			else if (msg->ie_Code == IECODE_MBUTTON)
				IN_KeyEvent(0, true, K_MOUSE3, 0);
			else if (msg->ie_Code == (IECODE_MBUTTON|IECODE_UP_PREFIX))
				IN_KeyEvent(0, false, K_MOUSE3, 0);

			if (in_windowed_mouse.ival)
			{
				if (consumemotion)
				{
					IN_MouseMove(0, 0, msg->ie_position.ie_xy.ie_x, msg->ie_position.ie_xy.ie_y, 0, 0);

#if 0
					msg->ie_Class = IECLASS_NULL;
#else
					msg->ie_position.ie_xy.ie_x = 0;
					msg->ie_position.ie_xy.ie_y = 0;
#endif
				}
			}
		}
	}
}

void INS_Commands(void)
{
}
void INS_EnumerateDevices(void *ctx, void(*callback)(void *ctx, const char *type, const char *devicename, unsigned int *qdevid))
{
}
void INS_Move (void)
{
}

char keyconv[] =
{
	'`', /* 0 */
	'1',
	'2',
	'3',
	'4',
	'5',
	'6',
	'7',
	'8',
	'9',
	'0', /* 10 */
	'-',
	'=',
	0,
	0,
	K_INS,
	'q',
	'w',
	'e',
	'r',
	't', /* 20 */
	'y',
	'u',
	'i',
	'o',
	'p',
	'[',
	']',
	0,
	K_KP_END,
	K_KP_DOWNARROW, /* 30 */
	K_KP_PGDN,
	'a',
	's',
	'd',
	'f',
	'g',
	'h',
	'j',
	'k',
	'l', /* 40 */
	';',
	'\'',
	'\\',
	0,
	K_KP_LEFTARROW,
	K_KP_5,
	K_KP_RIGHTARROW,
	'<',
	'z',
	'x', /* 50 */
	'c',
	'v',
	'b',
	'n',
	'm',
	',',
	'.',
	'/',
	0,
	K_KP_DEL, /* 60 */
	K_KP_HOME,
	K_KP_UPARROW,
	K_KP_PGUP,
	' ',
	K_BACKSPACE,
	K_TAB,
	K_KP_ENTER,
	K_ENTER,
	K_ESCAPE,
	K_DEL, /* 70 */
	K_INS,
	K_PGUP,
	K_PGDN,
	K_KP_MINUS,
	K_F11,
	K_UPARROW,
	K_DOWNARROW,
	K_RIGHTARROW,
	K_LEFTARROW,
	K_F1, /* 80 */
	K_F2,
	K_F3,
	K_F4,
	K_F5,
	K_F6,
	K_F7,
	K_F8,
	K_F9,
	K_F10,
	0, /* 90 */
	0,
	K_KP_SLASH,
	0,
	K_KP_PLUS,
	0,
	K_SHIFT,
	K_SHIFT,
	K_CAPSLOCK,
	K_CTRL,
	K_ALT, /* 100 */
	K_ALT,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	K_PAUSE, /* 110 */
	K_F12,
	K_HOME,
	K_END,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 120 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 130 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 140 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 150 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 160 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 170 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 180 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 190 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 200 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 210 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 220 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 230 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 240 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, /* 250 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0
};

struct InputEvent *myinputhandler_real(void);

struct EmulLibEntry myinputhandler =
{
	TRAP_LIB, 0, (void(*)(void))myinputhandler_real
};

struct InputEvent *myinputhandler_real()
{
	struct InputEvent *moo = (struct InputEvent *)REG_A0;

	struct InputEvent *coin;

	int screeninfront;

	coin = moo;

	if (screen)
	{
#if 0
		if (IntuitionBase->LibNode.lib_Version > 50 || (IntuitionBase == 50 && IntuitionBase->LibNode.lib_Revision >= 56))
			GetAttr(screen, SA_Displayed, &screeninfront);
		else
#endif
			screeninfront = screen==IntuitionBase->FirstScreen;
	}
	else
		screeninfront = 1;

	do
	{
		if (coin->ie_Class == IECLASS_RAWMOUSE || coin->ie_Class == IECLASS_RAWKEY || coin->ie_Class == IECLASS_NEWMOUSE)
		{
			INS_ProcessInputMessage(coin, screeninfront && window->MouseX > 0 && window->MouseY > 0);
		}

		coin = coin->ie_NextEvent;
	} while(coin);

	return moo;
}

