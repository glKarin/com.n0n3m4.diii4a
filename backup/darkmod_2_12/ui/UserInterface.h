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

#ifndef __USERINTERFACE_H__
#define __USERINTERFACE_H__

/*
===============================================================================

	Draws an interactive 2D surface.
	Used for all user interaction with the game.

===============================================================================
*/

class idFile;
class idDemoFile;


class idUserInterface {
public:
	virtual						~idUserInterface() {};

								// Returns the name of the gui.
	virtual const char *		Name() const = 0;

								// Returns a comment on the gui.
	virtual const char *		Comment() const = 0;

								// Returns true if the gui is interactive.
	virtual bool				IsInteractive() const = 0;

	virtual bool				IsUniqued() const = 0;

	virtual void				SetUniqued( bool b ) = 0;
								// returns false if it failed to load
	virtual bool				InitFromFile( const char *qpath, bool rebuild = true ) = 0;

								// handles an event, can return an action string, the caller interprets
								// any return and acts accordingly
	virtual const char *		HandleEvent( const sysEvent_t *event, int time, bool *updateVisuals = NULL ) = 0;

								// handles a named event
	virtual void				HandleNamedEvent( const char *eventName ) = 0;

								// repaints the ui
	virtual void				Redraw( int time ) = 0;

								// repaints the cursor
	virtual void				DrawCursor() = 0;

								// Provides read access to the idDict that holds this gui's state.
	virtual const idDict &		State() const = 0;

								// Removes a gui state variable
	virtual void				DeleteStateVar( const char *varName ) = 0;

								// Sets a gui state variable.
	virtual void				SetStateString( const char *varName, const char *value ) = 0;
	virtual void				SetStateBool( const char *varName, const bool value ) = 0;
	virtual void				SetStateInt( const char *varName, const int value ) = 0;
	virtual void				SetStateFloat( const char *varName, const float value ) = 0;

								// Gets a gui state variable
	virtual const char*			GetStateString( const char *varName, const char* defaultString = "" ) const = 0;
	virtual bool				GetStateBool( const char *varName, const char* defaultString = "0" ) const  = 0;
	virtual int					GetStateInt( const char *varName, const char* defaultString = "0" ) const = 0;
	virtual float				GetStateFloat( const char *varName, const char* defaultString = "0" ) const = 0;

								// The state has changed and the gui needs to update from the state idDict.
	virtual void				StateChanged( int time, bool redraw = false ) = 0;

								// Activated the gui.
	virtual const char *		Activate( bool activate, int time ) = 0;

								// Triggers the gui and runs the onTrigger scripts.
	virtual void				Trigger( int time ) = 0;

	virtual	void				ReadFromDemoFile( class idDemoFile *f ) = 0;
	virtual	void				WriteToDemoFile( class idDemoFile *f ) = 0;

	virtual bool				WriteToSaveGame( idFile *savefile ) const = 0;
	virtual bool				ReadFromSaveGame( idFile *savefile ) = 0;
	virtual void				SetKeyBindingNames( void ) = 0;

	virtual void				SetCursor( float x, float y ) = 0;
	virtual float				CursorX() = 0;
	virtual float				CursorY() = 0;

	//stgatilov: allows clicking GUI buttons in automation
	virtual const char*			RunGuiScript(const char *windowName, int scriptNum) = 0;

	//stgatilov: has same effect as "resetTime" command in GUI script.
	virtual bool				ResetWindowTime(const char *windowName, int startTime = 0) = 0;

	//stgatilov #2454: fetch subtitles from sound world and update GUI variables accordingly
	virtual void				UpdateSubtitles() = 0;
};


class idUserInterfaceManager {
public:
	virtual						~idUserInterfaceManager( void ) {};

	virtual void				Init() = 0;
	virtual void				Shutdown() = 0;
	virtual void				Touch( const char *name ) = 0;
	virtual void				WritePrecacheCommands( idFile *f ) = 0;

								// Sets the size for 640x480 adjustment.
	virtual void				SetSize( float width, float height ) = 0;

	virtual void				BeginLevelLoad() = 0;
	virtual void				EndLevelLoad() = 0;

								// Reloads changed guis, or all guis.
	virtual void				Reload( bool all ) = 0;

								// lists all guis
	virtual void				ListGuis() const = 0;

								// Returns true if gui exists.
	virtual bool				CheckGui( const char *qpath ) const = 0;

								// Allocates a new gui.
	virtual idUserInterface *	Alloc( void ) const = 0;

								// De-allocates a gui.. ONLY USE FOR PRECACHING
	virtual void				DeAlloc( idUserInterface *gui ) = 0;

								// Returns NULL if gui by that name does not exist.
	virtual idUserInterface *	FindGui(
		const char *qpath,
		bool autoLoad = false, bool needUnique = false, bool forceUnique = false,
		idDict presetDefines = {}
	) = 0;

								// Returns NULL if gui by that name does not exist.
	virtual idUserInterface *	FindDemoGui( const char *qpath ) = 0;

								// Allocates a new GUI list handler
	virtual	idListGUI *			AllocListGUI( void ) const = 0;

								// De-allocates a list gui
	virtual void				FreeListGUI( idListGUI *listgui ) = 0;

								// hack: needed for gamepad support
	virtual bool				IsBindHandlerActive() const = 0;
};

extern idUserInterfaceManager *	uiManager;

#endif /* !__USERINTERFACE_H__ */
