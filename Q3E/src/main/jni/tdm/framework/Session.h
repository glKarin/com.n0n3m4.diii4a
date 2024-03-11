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

#ifndef __SESSION_H__
#define __SESSION_H__

/*
===============================================================================

	The session is the glue that holds games together between levels.

===============================================================================
*/

// needed by the gui system for the load game menu
typedef struct {
	short		health;
	short		heartRate;
	short		stamina;
	short		combat;
} logStats_t;

static const int	MAX_LOGGED_STATS = 60 * 120;		// log every half second 

typedef enum {
	MSG_OK,
	MSG_ABORT,
	MSG_OKCANCEL,
	MSG_YESNO,
	MSG_PROMPT,
	MSG_INFO,
	MSG_WAIT
} msgBoxType_t;

typedef const char * (*HandleGuiCommand_t)( const char * );

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

	virtual void	StartFrontendThread() = 0;
	virtual void	TerminateFrontendThread() = 0;
	virtual bool	IsFrontend() const = 0;

	// Redraws the screen, handling games, guis, console, etc
	// during normal once-a-frame updates, outOfSequence will be false,
	// but when the screen is updated in a modal manner, as with utility
	// output, the mouse cursor will be released if running windowed.
	virtual void	UpdateScreen( bool outOfSequence = true ) = 0;

	// Called when it's time to update the Loading Bar progress.
	virtual void	PacifierUpdate(loadkey_t key, int count) = 0; // grayman #3763

	// Called every frame, possibly spinning in place if we are
	// above maxFps, or we haven't advanced at least one demo frame.
	// Returns the number of milliseconds since the last frame.
	virtual void	Frame() = 0;

	// Processes the given event.
	virtual	bool	ProcessEvent( const sysEvent_t *event ) = 0;

	// Activates the main menu
	virtual void	StartMenu( bool playIntro = false ) = 0;

	virtual void	SetGUI( idUserInterface *gui, HandleGuiCommand_t handle ) = 0;

	enum GuiType {
		gtActive,
		gtMainMenu,
		gtLoading,
		//gtInGame,
		//gtMsg,
		//gtTest,
	};
	virtual idUserInterface* GetGui(GuiType type) const = 0;

	// stgatilov: for clicking GUI buttons in automation
	// windowName --- the name of GUI window (see idWindows::name)
	// scriptNum --- index of script (see idWindow::ON_ACTION = 2 and similar)
	virtual bool	RunGuiScript(const char *windowName, int scriptNum = 2) = 0;

	// stgatilov: delete main menu GUI
	// when menu is brought up next time, it will be created from scratch
	virtual void	ResetMainMenu() = 0;

	// stgatilov: used when proceeding to next mission in campaign
	// saves that when menu is created next time, briefing state should be started
	virtual void	SetMainMenuStartAtBriefing() = 0;

	// Updates gui and dispatched events to it
	virtual void	GuiFrameEvents() = 0;

	// fires up the optional GUI event, also returns them if you set wait to true
	// if MSG_PROMPT and wait, returns the prompt string or NULL if aborted
	// network tells wether one should still run the network loop in a wait dialog
	virtual const char *ShowMessageBox( msgBoxType_t type, const char *message, const char *title = NULL, bool wait = false, const char *fire_yes = NULL, const char *fire_no = NULL, bool network = false ) = 0;
	virtual void	StopBox( void ) = 0;
	// monitor this download in a progress box to either abort or completion
	virtual void	DownloadProgressBox( backgroundDownload_t *bgl, const char *title, int progress_start = 0, int progress_end = 100 ) = 0;

	virtual void	SetPlayingSoundWorld() = 0;

	// this is used by the sound system when an OnDemand sound is loaded, so the game action
	// doesn't advance and get things out of sync
	virtual void	TimeHitch( int msec ) = 0;

	virtual const char *GetCurrentMapName( void ) = 0;

	virtual int		GetSaveGameVersion( void ) = 0;
    
	virtual void    RunGameTic(int timestepMs = USERCMD_MSEC, bool minorTic = false) = 0;
	virtual void	ActivateFrontend() = 0;
	virtual void	WaitForFrontendCompletion() = 0;
	virtual void    ExecuteFrameCommand(const char *command, bool delayed) = 0;
	virtual void    ExecuteDelayedFrameCommands() = 0;

	// The render world and sound world used for this session.
	idRenderWorld *	rw;
	idSoundWorld *	sw;

	// The renderer and sound system will write changes to writeDemo.
	// Demos can be recorded and played at the same time when splicing.
	idDemoFile *	readDemo;
	idDemoFile *	writeDemo;
	int				renderdemoVersion;
};

extern	idSession *	session;

#endif /* !__SESSION_H__ */
