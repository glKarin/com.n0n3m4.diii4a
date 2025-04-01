/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
#include "../../idlib/precompiled.h"
#include "posix_public.h"

typedef struct poll_keyboard_event_s
{
	int key;
	bool state;	
} poll_keyboard_event_t;

typedef struct poll_mouse_event_s
{
	int action;
	int value;
} poll_mouse_event_t;

#ifdef __ANDROID__ //karin: from original DIII4A ???
#define MAX_POLL_EVENTS		(50000) // 50 * 1000
#else
#define MAX_POLL_EVENTS 50
#endif
#define POLL_EVENTS_HEADROOM 2 // some situations require to add several events
static poll_keyboard_event_t poll_events_keyboard[MAX_POLL_EVENTS + POLL_EVENTS_HEADROOM];
static int poll_keyboard_event_count;
static poll_mouse_event_t poll_events_mouse[MAX_POLL_EVENTS + POLL_EVENTS_HEADROOM];
static int poll_mouse_event_count;

/*
==========
Posix_AddKeyboardPollEvent
==========
*/
bool Posix_AddKeyboardPollEvent(int key, bool state) {
	if (poll_keyboard_event_count >= MAX_POLL_EVENTS + POLL_EVENTS_HEADROOM) {
#ifdef __ANDROID__
        common->Warning("poll_keyboard_event_count exceeded MAX_POLL_EVENT + POLL_EVENTS_HEADROOM\n");
        return false;
#else
		common->FatalError("poll_keyboard_event_count exceeded MAX_POLL_EVENT + POLL_EVENTS_HEADROOM\n");
#endif
	}
	poll_events_keyboard[poll_keyboard_event_count].key = key;
	poll_events_keyboard[poll_keyboard_event_count++].state = state;
	if (poll_keyboard_event_count >= MAX_POLL_EVENTS) {
		common->DPrintf("WARNING: reached MAX_POLL_EVENT poll_keyboard_event_count\n");
		return false;
	}
	return true;
}

bool Posix_CanAddKeyboardPollEvent() {
	return poll_keyboard_event_count < MAX_POLL_EVENTS;
}

/*
==========
Posix_AddMousePollEvent
==========
*/
bool Posix_AddMousePollEvent(int action, int value) {
	if (poll_mouse_event_count >= MAX_POLL_EVENTS + POLL_EVENTS_HEADROOM) {
#ifdef __ANDROID__
        common->Warning("poll_mouse_event_count exceeded MAX_POLL_EVENT + POLL_EVENTS_HEADROOM\n");
        return false;
#else
		common->FatalError("poll_mouse_event_count exceeded MAX_POLL_EVENT + POLL_EVENTS_HEADROOM\n");
#endif
	}
	poll_events_mouse[poll_mouse_event_count].action = action;
	poll_events_mouse[poll_mouse_event_count++].value = value;
	if (poll_mouse_event_count >= MAX_POLL_EVENTS) {
		common->DPrintf("WARNING: reached MAX_POLL_EVENT poll_mouse_event_count\n");
		return false;
	}
	return true;
}

bool Posix_CanAddMousePollEvent() {
	return poll_mouse_event_count < MAX_POLL_EVENTS;
}

/*
===========================================================================
polled from GetDirectUsercmd
async input polling is obsolete
we have a single entry point for both mouse and keyboard
the mouse/keyboard seperation is API legacy
===========================================================================
*/

int Sys_PollKeyboardInputEvents( void ) {
	return poll_keyboard_event_count;
}

int Sys_ReturnKeyboardInputEvent( const int n, int &key, bool &state ) {
	if ( n >= poll_keyboard_event_count ) {
		return 0;
	}
	key = poll_events_keyboard[n].key;
	state = poll_events_keyboard[n].state;
	return 1;
}

void Sys_EndKeyboardInputEvents( void ) {
	//isn't this were it's supposed to be, was missing some key strokes with it set below
	poll_keyboard_event_count = 0;	
 }

int Sys_PollMouseInputEvents( void ) {
#if 0 //moved to the Sys_End functions
	poll_keyboard_event_count = 0;
	poll_mouse_event_count = 0;
#endif

	// that's OS specific, implemented in osx/ and linux/
	Posix_PollInput( );

	return poll_mouse_event_count;
}

int	Sys_ReturnMouseInputEvent( const int n, int &action, int &value )
{
	if ( n>=poll_mouse_event_count ) {
		return 0;
	}
	action = poll_events_mouse[ n ].action;
	value = poll_events_mouse[ n ].value;
	return 1;
}

void Sys_EndMouseInputEvents( void ) {
	// moved out of the Sys_PollMouseInputEvents
	poll_mouse_event_count = 0;
}
