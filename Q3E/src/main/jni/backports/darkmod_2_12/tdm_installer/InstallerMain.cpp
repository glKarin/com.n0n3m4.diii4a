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
#include "GuiFluidAutoGen.h"
#include "GuiGlobal.h"
#include "OsUtils.h"

//entry point on Linux (and on Windows with Console subsystem)
int main(int argc, char **argv) {
	bool unattended = false;
	bool help = false;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--unattended") == 0)
			unattended = true;
		if (strcmp(argv[i], "--help") == 0)
			help = true;
	}

	int ret = 0;
	if (help) {
		unattended = false;
		//create GUI designed in FLUID
		FluidGuiHelp();
		//additional initialization (adjust style)
		GuiInitHelp();
		//show the window
		g_HelpWindow->show();
		//enter event loop of FLTK
		ret = Fl::run();
	}
	else {
		//ensure that e.g. log file is written to directory with installer
		OsUtils::InitArgs(argv[0]);
		OsUtils::SetCwd(OsUtils::GetExecutableDir());

		//create all GUI designed in FLUID
		FluidAllGui();

		//additional GUI initialization (out of FLUID)
		GuiInitAll();

		//display the window
		g_Window->show();

		if (unattended) {
			//run everything with default options without human interaction
			GuiUnattended(argc, argv);
		}
		else {
			//run some checks immediately after start
			Fl::add_timeout(0.3, GuiLoaded);
			//enter event loop of FLTK
			ret = Fl::run();
		}
	}

	//window closed -> exit
	return ret;
}

#if defined(_WIN32) && !defined(_CONSOLE)
//entry point on Windows with GUI subsystem
int WINAPI aWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	return main(__argc, __argv);
}
#endif
