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
#include "GuiPageConfirm.h"
#include "GuiFluidAutoGen.h"
#include "GuiPageInstall.h"


void cb_Confirm_ButtonBack(Fl_Widget *self) {
	bool customVersion = g_Settings_CheckCustomVersion->value();
	if (customVersion) {
		g_Wizard->value(g_PageVersion);
	}
	else {
		g_Wizard->value(g_PageSettings);
	}
}

void cb_Confirm_ButtonStart(Fl_Widget *self) {
	Install_MetaPerformInstall();
}
