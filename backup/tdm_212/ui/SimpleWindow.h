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

#ifndef __SIMPLEWIN_H__
#define __SIMPLEWIN_H__

#include "Window.h"

class idUserInterfaceLocal;
class idDeviceContext;
class idSimpleWindow;

class idSimpleWindow {
	friend class idWindow;
public:
					idSimpleWindow(idWindow* win);
	virtual			~idSimpleWindow();
	void			Redraw(float x, float y);
	void			StateChanged( bool redraw );

	idStr			name;

	idWinVar *		GetThisWinVarByName(const char *varname);
    intptr_t		GetWinVarOffset(idWinVar *wv, drawWin_t* owner);
	size_t			Size();

	idWindow*		GetParent ( void ) { return mParent; }

	virtual void	WriteToSaveGame( idFile *savefile );
	virtual void	ReadFromSaveGame( idFile *savefile );

protected:
	void 			CalcClientRect(float xofs, float yofs);
	void 			SetupTransforms(float x, float y);
	void 			DrawBackground(const idRectangle &drawRect);
	void 			DrawBorderAndCaption(const idRectangle &drawRect);

	idUserInterfaceLocal *gui;
	idDeviceContext *dc;
	int 			flags;
	idRectangle 	drawRect;			// overall rect
	idRectangle 	clientRect;			// client area
	idRectangle 	textRect;
	idVec2			origin;
	int 			fontNum;
	float 			matScalex;
	float 			matScaley;
	float 			borderSize;
	int 			textAlign;
	float 			textAlignx;
	float 			textAligny;
	int				textShadow;

	idWinStr		text;
	idWinBool		visible;
	idWinRectangle	rect;				// overall rect
	idWinVec4		backColor;
	idWinVec4		matColor;
	idWinVec4		foreColor;
	idWinVec4		borderColor;
	idWinFloat		textScale;
	idWinFloat		rotate;
	idWinVec2		shear;
	idWinBackground	backGroundName;

	const idMaterial* background;
	
	idWindow *		mParent;

	idWinBool	hideCursor;

	idGuiSourceLocation srcLocation;
};

#endif /* !__SIMPLEWIN_H__ */
