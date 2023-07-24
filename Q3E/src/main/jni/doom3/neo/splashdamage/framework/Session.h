// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __SESSION_H__
#define __SESSION_H__

/*
===============================================================================

	The session is the glue that holds games together between levels.

===============================================================================
*/

typedef enum {
	MSG_OK,
	MSG_OKCANCEL,
	MSG_YESNO,
	MSG_DOWNLOAD_YESNO,
	MSG_NEED_PASSWORD,	
	MSG_ABORT,
	MSG_NEED_AUTH,
} msgBoxType_t;


struct backgroundDownload_t;
class idUserInterface;
class idRenderWorld;
class idSoundWorld;
class idDemoFile;

class idSession {
public:
	virtual			~idSession() {}

	// Called in an orderly fashion at system startup,
	// so commands, cvars, files, etc are all available.
	virtual	void	Init() = 0;

	// Shut down the session.
	virtual	void	Shutdown() = 0;

	// Called on errors and game exits.
	virtual void	Stop() = 0;

	// Redraws the screen, handling games, guis, console, etc
	// during normal once-a-frame updates, outOfSequence will be false,
	// but when the screen is updated in a modal manner, as with utility
	// output, the mouse cursor will be released if running windowed.
	virtual void	UpdateScreen( bool outOfSequence = true ) = 0;

	// Called when console prints happen, allowing the loading screen
	// to redraw if enough time has passed.
	virtual void	PacifierUpdate() = 0;

	// Called every frame, possibly spinning in place if we are
	// above maxFps, or we haven't advanced at least one demo frame.
	// Returns the number of milliseconds since the last frame.
	virtual void	Frame() = 0;

	// Returns true if a multiplayer game is running.
	// CVars and commands are checked differently in multiplayer mode.
	virtual bool	IsMultiplayer() = 0;

	// Processes the given event.
	virtual	bool	ProcessEvent( const sdSysEvent* event ) = 0;

	// Activates the main menu
	virtual void	StartMenu( void ) = 0;
	virtual void	ExitMenu( void ) = 0;

	// Updates gui and dispatched events to it
	virtual void	GuiFrameEvents( bool outOfSequence ) = 0;

	// Tell the game to display a message box
	virtual void	MessageBox( msgBoxType_t type, const wchar_t *message, const char *titleDef ) = 0;
	
	virtual void	SetPlayingSoundWorld() = 0;

	// this is used by the sound system when an OnDemand sound is loaded, so the game action
	// doesn't advance and get things out of sync
	virtual void	TimeHitch( int msec ) = 0;

	virtual bool	MapSpawned() = 0;

#ifdef EB_WITH_PB
	virtual const char*	GetCurrentMapName( void ) = 0;
#endif /* EB_WITH_PB */

	// The render world and sound world used for this session.
	idRenderWorld *	rw;
	idSoundWorld *	sw;

	// The renderer and sound system will write changes to writeDemo.
	// Demos can be recorded and played at the same time when splicing.
	idDemoFile *	readDemo;
	idDemoFile *	writeDemo;
};

extern	idSession *	session;

#endif /* !__SESSION_H__ */
