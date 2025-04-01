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

#ifndef __EVENTLOOP_H__
#define __EVENTLOOP_H__

/*
===============================================================================

	The event loop receives events from the system and dispatches them to
	the various parts of the engine. The event loop also handles journaling.
	The file system copies .cfg files to the journaled file.

===============================================================================
*/

const int MAX_PUSHED_EVENTS =	64;

class idEventLoop {
public:
					idEventLoop( void );
					~idEventLoop( void );

	void			Init( void );

					// Closes the journal file if needed.
	void			Shutdown( void );

					// It is possible to get an event at the beginning of a frame that
					// has a time stamp lower than the last event from the previous frame.
	sysEvent_t		GetEvent( void );

					// Dispatches all pending events and returns the current time.
	int				RunEventLoop( bool commandExecution = true );

					// Gets the current time in a way that will be journaled properly,
					// as opposed to Sys_Milliseconds(), which always reads a real timer.
	int				Milliseconds( void );

					// Returns the journal level, 1 = record, 2 = play back.
	int				JournalLevel( void ) const;

					// Adds given event to the event queue
					// Can be used for hooking mouse in menu for automation or tests
	void			PushEvent( sysEvent_t *event );

					// Journal file.
	idFile *		com_journalFile;
	idFile *		com_journalDataFile;

private:
					// all events will have this subtracted from their time
	int				initialTimeOffset;

	int				com_pushedEventsHead, com_pushedEventsTail;
	sysEvent_t		com_pushedEvents[MAX_PUSHED_EVENTS];

	static idCVar	com_journal;

	sysEvent_t		GetRealEvent( void );
	void			ProcessEvent( sysEvent_t ev );
};

extern	idEventLoop	*eventLoop;

#endif /* !__EVENTLOOP_H__ */
