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
#ifndef __FIELDWINDOW_H
#define __FIELDWINDOW_H

#include "Window.h"


class idFieldWindow : public idWindow {
public:
	idFieldWindow(idUserInterfaceLocal *gui);
	idFieldWindow(idDeviceContext *d, idUserInterfaceLocal *gui);
	virtual ~idFieldWindow() override;

	virtual void Draw(int time, float x, float y) override;
	
private:
	virtual bool ParseInternalVar(const char *name, idParser *src) override;
	void CommonInit();
	void CalcPaintOffset(int len);
	int cursorPos;
	int lastTextLength;
	int lastCursorPos;
	int paintOffset;
	bool showCursor;
	idStr cursorVar;
};

#endif // __FIELDWINDOW_H
