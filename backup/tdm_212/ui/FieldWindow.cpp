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

#include "precompiled.h"
#pragma hdrstop



#include "DeviceContext.h"
#include "Window.h"
#include "UserInterfaceLocal.h"
#include "FieldWindow.h"


void idFieldWindow::CommonInit() {
	cursorPos = 0;
	lastTextLength = 0;
	lastCursorPos = 0;
	paintOffset = 0;
	showCursor = false;
}

idFieldWindow::idFieldWindow(idDeviceContext *d, idUserInterfaceLocal *g) : idWindow(d, g) {
	dc = d;
	gui = g;
	CommonInit();
}

idFieldWindow::idFieldWindow(idUserInterfaceLocal *g) : idWindow(g) {
	gui = g;
	CommonInit();
}

idFieldWindow::~idFieldWindow() {

}

bool idFieldWindow::ParseInternalVar(const char *_name, idParser *src) {
	if (idStr::Icmp(_name, "cursorvar") == 0) {
		ParseString(src, cursorVar);
		return true;
	}
	if (idStr::Icmp(_name, "showcursor") == 0) {
		showCursor = src->ParseBool();
		return true;
	}
	return idWindow::ParseInternalVar(_name, src);
}


void idFieldWindow::CalcPaintOffset(int len) {
	lastCursorPos = cursorPos;
	lastTextLength = len;
	paintOffset = 0;
	int tw = dc->TextWidth(text, textScale, -1);
	if (tw < textRect.w) {
		return;
	}
	while (tw > textRect.w && len > 0) {
		tw = dc->TextWidth(text, textScale, --len);
		paintOffset++;
	}
}


void idFieldWindow::Draw(int time, float x, float y) {
	float scale = textScale;
	int len = text.Length();
	cursorPos = gui->State().GetInt( cursorVar );
	if (len != lastTextLength || cursorPos != lastCursorPos) {
		CalcPaintOffset(len);
	}
	idRectangle rect = textRect;
	if (paintOffset >= len) {
		paintOffset = 0;
	}
	if (cursorPos > len) {
		cursorPos = len;
	}
	dc->DrawText(&text[paintOffset], scale, 0, foreColor, rect, false, ((flags & WIN_FOCUS) || showCursor) ? cursorPos - paintOffset : -1);
}

