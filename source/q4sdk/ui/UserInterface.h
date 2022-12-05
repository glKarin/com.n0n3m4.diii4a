// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __USERINTERFACE_H__
#define __USERINTERFACE_H__

struct wrapInfo_t {
	int lastWhitespace;
	int maxIndex;
	wrapInfo_t ( void ) {
		lastWhitespace = -1;
		maxIndex = -1;
	}
};

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
	virtual						~idUserInterface( void ) {}

								// Returns the name of the gui.
	virtual const char *		Name( void ) const = 0;

								// Returns a comment on the gui.
	virtual const char *		Comment( void ) const = 0;

								// Returns true if the gui is interactive.
	virtual bool				IsInteractive() const = 0;

// RAVEN BEGIN
// bdube: added
								// Changes the interactive of the gui
	virtual void				SetInteractive ( bool interactive ) = 0 ;
// RAVEN END

	virtual bool				IsUniqued() const = 0;

	virtual void				SetUniqued( bool b ) = 0;
								// returns false if it failed to load
	virtual bool				InitFromFile( const char *qpath, bool rebuild = true, bool cache = true ) = 0;

								// handles an event, can return an action string, the caller interprets
								// any return and acts accordingly
	virtual const char *		HandleEvent( const sysEvent_t *event, int time, bool *updateVisuals = NULL ) = 0;

								// handles a named event
	virtual void				HandleNamedEvent( const char *eventName ) = 0;

								// repaints the ui
	virtual void				Redraw( int time ) = 0;

								// repaints the cursor
	virtual void				DrawCursor( void ) = 0;

								// Provides read access to the idDict that holds this gui's state.
	virtual const idDict &		State( void ) const = 0;

								// Removes a gui state variable
	virtual void				DeleteStateVar( const char *varName ) = 0;

								// Sets a gui state variable.
	virtual void				SetStateString( const char *varName, const char *value ) = 0;
	virtual void				SetStateBool( const char *varName, const bool value ) = 0;
	virtual void				SetStateInt( const char *varName, const int value ) = 0;
	virtual void				SetStateFloat( const char *varName, const float value ) = 0;
	virtual void				SetStateVector( const char *varName, const idVec3& vector ) = 0;
	virtual void				SetStateVec4( const char *varName, const idVec4& vector ) = 0;
// RAVEN BEGIN
// bdube: added way to clear state
	virtual void				ClearState( void ) = 0;
// rjohnson: added
	virtual void				DeleteState( const char *varName ) = 0;

	virtual idVec4				GetLightColor ( void ) = 0;

								// Gets a gui state variable
	virtual const char*			GetStateString( const char *varName, const char* defaultString = "" ) const = 0;
	virtual bool				GetStateBool( const char *varName, const char* defaultString = "0" ) const  = 0;
	virtual int					GetStateInt( const char *varName, const char* defaultString = "0" ) const = 0;
	virtual float				GetStateFloat( const char *varName, const char* defaultString = "0" ) const = 0;
	virtual idVec3				GetStateVector( const char *varName, const char* defaultString = "0 0 0" ) const = 0;
	virtual idVec4				GetStateVec4( const char *varName, const char* defaultString = "0 0 0 0" ) const = 0;

// jscott: added
	virtual class idWindow *	GetDesktop( void ) const = 0;
// RAVEN END

								// The state has changed and the gui needs to update from the state idDict.
	virtual void				StateChanged( int time, bool redraw = false ) = 0;

								// Activated the gui.
	virtual const char *		Activate( bool activate, int time ) = 0;

								// Triggers the gui and runs the onTrigger scripts.
	virtual void				Trigger( int time ) = 0;

	virtual	void				ReadFromDemo( class idDemoFile *f ) = 0;
	virtual	void				WriteToDemo( class idDemoFile *f ) = 0;

	virtual bool				WriteToSaveGame( idFile *savefile ) const = 0;
	virtual bool				ReadFromSaveGame( idFile *savefile ) = 0;
	virtual void				SetKeyBindingNames( void ) = 0;

	virtual void				SetCursor( float x, float y ) = 0;
	virtual float				CursorX( void ) = 0;
	virtual float				CursorY( void ) = 0;

// RAVEN BEGIN
// mekberg: Returns the index of the string where width in pixels <= specified val. Can return index of last whitespace.
	virtual bool				GetMaxTextIndex( const char *windowName, const char *text, wrapInfo_t& wrapInfo ) const = 0;

// mwhitlock: Xenon texture streaming
#if defined(_XENON)
	virtual const idList<idMaterial*>& GetMaterialsList(void) = 0;
#endif
// RAVEN END

// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
#if defined(_RV_MEM_SYS_SUPPORT)
	virtual bool				IsLevelLoadReferenced( void ) = 0;
	virtual void				SetLevelLoadReferenced( bool refd ) = 0;
#endif
// RAVEN END
};


class idUserInterfaceManager {
public:
	virtual						~idUserInterfaceManager( void ) {}

	virtual void				Init( void ) = 0;
	virtual void				Shutdown( void ) = 0;
	virtual void				Touch( const char *name ) = 0;
	virtual void				WritePrecacheCommands( idFile *f ) = 0;

								// Sets the size for 640x480 adjustment.
	virtual void				SetSize( float width, float height ) = 0;

	virtual void				BeginLevelLoad( void ) = 0;
	virtual void				EndLevelLoad( void ) = 0;
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
#if defined(_RV_MEM_SYS_SUPPORT)
	virtual void				FlushGUIs( void ) = 0;
#endif
// RAVEN END

								// Reloads changed guis, or all guis.
	virtual void				Reload( bool all ) = 0;

								// lists all guis
	virtual void				ListGuis( void ) const = 0;

								// Returns true if gui exists.
	virtual bool				CheckGui( const char *qpath ) const = 0;

								// Allocates a new gui.
	virtual idUserInterface *	Alloc( void ) const = 0;

								// De-allocates a gui.. ONLY USE FOR PRECACHING
	virtual void				DeAlloc( idUserInterface *gui ) = 0;

								// Returns NULL if gui by that name does not exist.
	virtual idUserInterface *	FindGui( const char *qpath, bool autoLoad = false, bool needUnique = false, bool forceUnique = false ) = 0;

								// Returns the index into the global gui list
	virtual int					GuiIndex( idUserInterface *gui ) = 0;

								// Returns the gui at location index, or allocates a new one at location index
	virtual idUserInterface *	FindGuiByIndex( int index ) = 0;

								// Clears out the in game guis before loading a renderdemo
	virtual void				ClearGameGuis( void ) = 0;

								// Allocates a new GUI list handler
	virtual	idListGUI *			AllocListGUI( void ) const = 0;

								// De-allocates a list gui
	virtual void				FreeListGUI( idListGUI *listgui ) = 0;
	
// RAVEN BEGIN
// rjohnson: added option for guis to always think
	virtual void				RunAlwaysThinkGUIs ( int time ) = 0;
// bdube: embedded icons
	virtual void				RegisterIcon ( const char* code, const char* shader, int x = -1, int y = -1, int w = -1, int h = -1 ) = 0;	
// RAVEN END
};

extern idUserInterfaceManager *	uiManager;

#endif /* !__USERINTERFACE_H__ */
