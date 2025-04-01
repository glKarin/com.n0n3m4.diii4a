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

class Fl_Widget;

//true if "--unattended" argument was passed (i.e. no human interaction intended)
extern bool UnattendedMode;


const char *GuiChooseDirectory(const char *message);

enum GuiMessageBoxFlags {
	mbfTypeMask        = 0x00F0,
	mbfTypeError       = 0x0010,	//error message
	mbfTypeMessage     = 0x0020,	//info message
	mbfTypeWarning     = 0x0030,	//warning message
	mbfButtonsMask     = 0x000F,
	mbfButtonsOk       = 0x0001,	//one button: Ok
	mbfButtonsYesNo    = 0x0002,	//two buttons: Ok and Cancel
	mbfDefaultCancel   = 0x8000,	//if set, "Cancel"/stop is the default choice

	//displays error message with one button: Ok = stop
	mbfError = mbfTypeError | mbfButtonsOk | mbfDefaultCancel,
	//displays info message with one button: Ok = continue
	mbfMessage = mbfTypeMessage | mbfButtonsOk,
	//displays warning message with buttons: Ok = continue (default) and Cancel = stop
	mbfWarningMinor = mbfTypeWarning | mbfButtonsYesNo,
	//displays warning message with buttons: Ok = continue and Cancel = stop (default)
	mbfWarningMajor = mbfTypeWarning | mbfButtonsYesNo | mbfDefaultCancel,
};
//returns INDEX of pressed button: Ok = 0, Cancel = 1
int GuiMessageBox(GuiMessageBoxFlags flags, const char *message, const char *buttonOk = nullptr, const char *buttonCancel = nullptr);

void GuiSetStyles(Fl_Widget *rootWindow);
