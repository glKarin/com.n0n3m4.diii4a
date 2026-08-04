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

class idWindow;

class idUserInterfaceLocal : public idUserInterface {
	friend class idUserInterfaceManagerLocal;
public:
								idUserInterfaceLocal();
	virtual						~idUserInterfaceLocal() override;

	virtual const char *		Name() const override;
	virtual const char *		Comment() const override;
	virtual bool				IsInteractive() const override;
	virtual bool				InitFromFile( const char *qpath, bool rebuild = true ) override;
	virtual const char *		HandleEvent( const sysEvent_t *event, int time, bool *updateVisuals ) override;
	virtual void				HandleNamedEvent( const char* namedEvent ) override;
	virtual void				Redraw( int time ) override;
	virtual void				DrawCursor() override;
	virtual const idDict &		State() const override;
	virtual void				DeleteStateVar( const char *varName ) override;
	virtual void				SetStateString( const char *varName, const char *value ) override;
	virtual void				SetStateBool( const char *varName, const bool value ) override;
	virtual void				SetStateInt( const char *varName, const int value ) override;
	virtual void				SetStateFloat( const char *varName, const float value ) override;

	// Gets a gui state variable
	virtual const char*			GetStateString( const char *varName, const char* defaultString = "" ) const override;
	virtual bool				GetStateBool( const char *varName, const char* defaultString = "0" ) const override;
	virtual int					GetStateInt( const char *varName, const char* defaultString = "0" ) const override;
	virtual float				GetStateFloat( const char *varName, const char* defaultString = "0" ) const override;

	virtual void				StateChanged( int time, bool redraw ) override;
	virtual const char *		Activate( bool activate, int time ) override;
	virtual void				Trigger( int time ) override;
	virtual void				ReadFromDemoFile( class idDemoFile *f ) override;
	virtual void				WriteToDemoFile( class idDemoFile *f ) override;
	virtual bool				WriteToSaveGame( idFile *savefile ) const override;
	virtual bool				ReadFromSaveGame( idFile *savefile ) override;
	virtual void				SetKeyBindingNames( void ) override;
	virtual bool				IsUniqued() const override { return uniqued; };
	virtual void				SetUniqued( bool b ) override { uniqued = b; };
	virtual void				SetCursor( float x, float y ) override;

	virtual float				CursorX() override { return cursorX; }
	virtual float				CursorY() override { return cursorY; }
	virtual const char*			RunGuiScript(const char *windowName, int scriptNum) override;
	virtual bool				ResetWindowTime(const char *windowName, int startTime = 0) override;
	virtual void				UpdateSubtitles() override;

	size_t						Size();

	idDict *					GetStateDict() { return &state; }

	const char *				GetSourceFile( void ) const { return source; }
	ID_TIME_T						GetTimeStamp( void ) const { return timeStamp; }

	idWindow *					GetDesktop() const { return desktop; }
	void						SetBindHandler( idWindow *win ) { bindHandler = win; }
	bool						Active() const { return active; }
	int							GetTime() const { return time; }
	void						SetTime( int _time ) { time = _time; }

	void						ClearRefs() { refs = 0; }
	void						AddRef() { refs++; }
	int							GetRefs() { return refs; }

	void						RecurseSetKeyBindingNames( idWindow *window );
	idStr						&GetPendingCmd() { return pendingCmd; };
	idStr						&GetReturnCmd() { return returnCmd; };

private:
	bool						active;
	bool						loading;
	bool						interactive;
	bool						uniqued;

	idDict						presetDefines;
	idDict						defines;
	idDict						state;
	idWindow *					desktop;
	idWindow *					bindHandler;

	idStr						source;
	idStr						activateStr;
	idStr						pendingCmd;
	idStr						returnCmd;
	ID_TIME_T						timeStamp;

	float						cursorX;
	float						cursorY;

	int							time;

	int							refs;

	//stgatilov #2454: We can show several active subtitles simultaneously.
	// Each of them gets into one of few "slots".
	// Here we store information about slots between updates.
	idList<SubtitleMatch>		subtitleSlots;
};

class idUserInterfaceManagerLocal : public idUserInterfaceManager {
	friend class idUserInterfaceLocal;

public:
	virtual void				Init() override;
	virtual void				Shutdown() override;
	virtual void				Touch( const char *name ) override;
	virtual void				WritePrecacheCommands( idFile *f ) override;
	virtual void				SetSize( float width, float height ) override;
	virtual void				BeginLevelLoad() override;
	virtual void				EndLevelLoad() override;
	virtual void				Reload( bool all ) override;
	virtual void				ListGuis() const override;
	virtual bool				CheckGui( const char *qpath ) const override;
	virtual idUserInterface *	Alloc( void ) const override;
	virtual void				DeAlloc( idUserInterface *gui ) override;
	virtual idUserInterface *	FindGui( const char *qpath, bool autoLoad = false, bool needInteractive = false, bool forceUnique = false, idDict presetDefines = {} ) override;
	virtual idUserInterface *	FindDemoGui( const char *qpath ) override;
	virtual	idListGUI *			AllocListGUI( void ) const override;
	virtual void				FreeListGUI( idListGUI *listgui ) override;
	virtual bool				IsBindHandlerActive() const override;

private:
	idRectangle					screenRect;
	idDeviceContext				dc;

	idList<idUserInterfaceLocal*> guis;
	idList<idUserInterfaceLocal*> demoGuis;

};
