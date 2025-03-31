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

#ifndef __EDITWINDOW_H__
#define __EDITWINDOW_H__

#include "Window.h"

const int MAX_EDITFIELD = 4096;

class idUserInterfaceLocal;
class idSliderWindow;

class idEditWindow : public idWindow {
public:
						idEditWindow(idUserInterfaceLocal *gui);
						idEditWindow(idDeviceContext *d, idUserInterfaceLocal *gui);
	virtual 			~idEditWindow() override;

	virtual void		Draw( int time, float x, float y ) override;
	virtual const char *HandleEvent( const sysEvent_t *event, bool *updateVisuals ) override;
	virtual void		PostParse() override;
	virtual void		GainFocus() override;
	virtual size_t		Allocated() override { return idWindow::Allocated(); }
	
	virtual idWinVar *	GetThisWinVarByName(const char *varname) override;
	
	virtual void 		HandleBuddyUpdate(idWindow *buddy) override;
	virtual void		Activate(bool activate, idStr &act) override;
	
	virtual void		RunNamedEvent( const char* eventName ) override;
	
private:

	virtual bool		ParseInternalVar(const char *name, idParser *src) override;

	void				InitCvar();
						// true: read the updated cvar from cvar system
						// false: write to the cvar system
						// force == true overrides liveUpdate 0
	void				UpdateCvar( bool read, bool force = false );
	
	void				CommonInit();
	void				EnsureCursorVisible();
	void				InitScroller( bool horizontal );
	
	int					maxChars;
	int					paintOffset;
	int					cursorPos;
	int					cursorLine;
	int					cvarMax;
	bool				wrap;
	bool				readonly;
	bool				numeric;
	idStr				sourceFile;
	idStr				placeholder;
	idVec4				placeholderColor;
	idSliderWindow *	scroller;
	idList<int>			breaks;
	int					textIndex;
	int					lastTextLength;
	bool				forceScroll;	
	idWinBool			password;

	idWinStr			cvarStr;
	idCVar *			cvar;

	idWinBool			liveUpdate;
	idWinStr			cvarGroup;
};

#endif /* !__EDITWINDOW_H__ */
