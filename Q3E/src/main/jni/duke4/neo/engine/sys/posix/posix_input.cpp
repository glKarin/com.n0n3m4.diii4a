/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#include "../../../idlib/precompiled.h"
#include "posix_public.h"

typedef struct poll_keyboard_event_s {
	int key;
	bool state;
} poll_keyboard_event_t;

typedef struct poll_mouse_event_s {
	int action;
	int value;
} poll_mouse_event_t;

#ifdef __ANDROID__ //karin: from original DIII4A ???
#define MAX_POLL_EVENTS		(50000) // 50 * 1000
#else
#define MAX_POLL_EVENTS		(50)
#endif
#define POLL_EVENTS_HEADROOM	2 // some situations require to add several events
static poll_keyboard_event_t poll_events_keyboard[MAX_POLL_EVENTS + POLL_EVENTS_HEADROOM];
static int poll_keyboard_event_count;
static poll_mouse_event_t poll_events_mouse[MAX_POLL_EVENTS + POLL_EVENTS_HEADROOM];
static int poll_mouse_event_count;

/*
==========
Posix_AddKeyboardPollEvent
==========
*/
bool Posix_AddKeyboardPollEvent(int key, bool state)
{
	if (poll_keyboard_event_count >= MAX_POLL_EVENTS + POLL_EVENTS_HEADROOM)
#ifdef __ANDROID__
	{
		common->Warning("poll_keyboard_event_count exceeded MAX_POLL_EVENT + POLL_EVENTS_HEADROOM\n");
		return false;
	}
#else
		common->FatalError("poll_keyboard_event_count exceeded MAX_POLL_EVENT + POLL_EVENTS_HEADROOM\n");
#endif

	poll_events_keyboard[poll_keyboard_event_count].key = key;
	poll_events_keyboard[poll_keyboard_event_count++].state = state;

	if (poll_keyboard_event_count >= MAX_POLL_EVENTS) {
		common->DPrintf("WARNING: reached MAX_POLL_EVENT poll_keyboard_event_count\n");
		return false;
	}

	return true;
}

/*
==========
Posix_AddMousePollEvent
==========
*/
bool Posix_AddMousePollEvent(int action, int value)
{
	if (poll_mouse_event_count >= MAX_POLL_EVENTS + POLL_EVENTS_HEADROOM)
#ifdef __ANDROID__
	{
		common->Warning("poll_mouse_event_count exceeded MAX_POLL_EVENT + POLL_EVENTS_HEADROOM\n");
		return false;
	}
#else
		common->FatalError("poll_mouse_event_count exceeded MAX_POLL_EVENT + POLL_EVENTS_HEADROOM\n");
#endif

	poll_events_mouse[poll_mouse_event_count].action = action;
	poll_events_mouse[poll_mouse_event_count++].value = value;

	if (poll_mouse_event_count >= MAX_POLL_EVENTS) {
		common->DPrintf("WARNING: reached MAX_POLL_EVENT poll_mouse_event_count\n");
		return false;
	}

	return true;
}

/*
===========================================================================
polled from GetDirectUsercmd
async input polling is obsolete
we have a single entry point for both mouse and keyboard
the mouse/keyboard seperation is API legacy
===========================================================================
*/

int Sys_PollKeyboardInputEvents(void)
{
	return poll_keyboard_event_count;
}

int Sys_ReturnKeyboardInputEvent(const int n, int &key, bool &state)
{
	if (n >= poll_keyboard_event_count) {
		return 0;
	}

	key = poll_events_keyboard[n].key;
	state = poll_events_keyboard[n].state;
	return 1;
}

void Sys_EndKeyboardInputEvents(void)
{
	//isn't this were it's supposed to be, was missing some key strokes with it set below
	poll_keyboard_event_count = 0;
}

int Sys_PollMouseInputEvents(void)
{
#if 0 //moved to the Sys_End functions
	poll_keyboard_event_count = 0;
	poll_mouse_event_count = 0;
#endif

	// that's OS specific, implemented in osx/ and linux/
	Posix_PollInput();

	return poll_mouse_event_count;
}

int	Sys_ReturnMouseInputEvent(const int n, int &action, int &value)
{
	if (n>=poll_mouse_event_count) {
		return 0;
	}

	action = poll_events_mouse[ n ].action;
	value = poll_events_mouse[ n ].value;
	return 1;
}

void Sys_EndMouseInputEvents(void)
{
	// moved out of the Sys_PollMouseInputEvents
	poll_mouse_event_count = 0;
}

/*
============================================================================
EVENT LOOP
============================================================================
*/

#define	MAX_QUED_EVENTS		256
#define	MASK_QUED_EVENTS	( MAX_QUED_EVENTS - 1 )

static sysEvent_t eventQue[MAX_QUED_EVENTS];
static int eventHead, eventTail;

/*
================
Posix_QueEvent

ptr should either be null, or point to a block of data that can be freed later
================
*/
void Posix_QueEvent(sysEventType_t type, int value, int value2,
					int ptrLength, void *ptr)
{
	sysEvent_t *ev;

	ev = &eventQue[eventHead & MASK_QUED_EVENTS];

	if (eventHead - eventTail >= MAX_QUED_EVENTS) {
		common->Printf("Posix_QueEvent: overflow\n");

		// we are discarding an event, but don't leak memory
		// TTimo: verbose dropped event types?
		if (ev->evPtr) {
			Mem_Free(ev->evPtr);
			ev->evPtr = NULL;
		}

		eventTail++;
	}

	eventHead++;

	ev->evType = type;
	ev->evValue = value;
	ev->evValue2 = value2;
	ev->evPtrLength = ptrLength;
	ev->evPtr = ptr;

#if 0
	common->Printf("Event %d: %d %d\n", ev->evType, ev->evValue, ev->evValue2);
#endif
}

/*
================
Sys_GetEvent
================
*/
sysEvent_t Sys_GetEvent(void)
{
	static sysEvent_t ev;

	// return if we have data
	if (eventHead > eventTail) {
		eventTail++;
		return eventQue[(eventTail - 1) & MASK_QUED_EVENTS];
	}

	// return the empty event with the current time
	memset(&ev, 0, sizeof(ev));

	return ev;
}

/*
================
Sys_ClearEvents
================
*/
void Sys_ClearEvents(void)
{
	eventHead = eventTail = 0;
}


/*
called during frame loops, pacifier updates etc.
this is only for console input polling and misc mouse grab tasks
the actual mouse and keyboard input is in the Sys_Poll logic
*/
void Sys_GenerateEvents(void)
{
#if 0
	char *s;

	if ((s = Posix_ConsoleInput())) {
		char *b;
		int len;

		len = strlen(s) + 1;
		b = (char *)Mem_Alloc(len);
		strcpy(b, s);
		Posix_QueEvent(SE_CONSOLE, 0, 0, len, b);
	}
#endif
}

