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
#pragma once

#include "CommandLine.h"
#include <functional>
#include <map>
#include <float.h>

class Fl_Progress;
class Fl_Widget;
class Fl_Window;


class ProgressIndicatorGui : public ZipSync::ProgressIndicator {
	Fl_Progress *_progressWidget = nullptr;
	std::string _lastProgressText;

	Fl_Widget *_labelWidget = nullptr;
	Fl_Window *_mainWindow = nullptr;
	std::string _lastLabelText;
	double _startTime = DBL_MAX;
	double _lastUpdateTime = DBL_MAX;

	//set this to nonzero to generate interruption on next progress callback
	static int InterruptFlag;

public:
	~ProgressIndicatorGui();
	ProgressIndicatorGui(Fl_Progress *widget);

	void AttachRemainsLabel(Fl_Widget *label);
	void AttachMainWindow(Fl_Window *window);
	int Update(double globalRatio, std::string globalComment, double localRatio = -1.0, std::string localComment = "") override;

	static void Interrupt(int code = 1) { InterruptFlag = code; }
};


class GuiDeactivateGuard {
	std::map<Fl_Widget*, bool> _widgetToOldActive;

	//number of guards active right now: we forbid closing window if it is positive
	static int DeactivatedCount;

public:
	~GuiDeactivateGuard();
	GuiDeactivateGuard(Fl_Widget *blockedPage, std::initializer_list<Fl_Widget*> exceptThese);
	void Rollback();

	static bool IsAnyActive() { return DeactivatedCount > 0; }

	GuiDeactivateGuard(const GuiDeactivateGuard&) = delete;
	GuiDeactivateGuard& operator= (const GuiDeactivateGuard&) = delete;
};
