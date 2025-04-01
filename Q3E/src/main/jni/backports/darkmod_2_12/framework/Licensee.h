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

/*
===============================================================================

	Definitions for information that is related to a licensee's game name and location.

===============================================================================
*/

#define GAME_NAME		"The Dark Mod"	// appears on window titles and errors

// greebo: Defines the darkmod release version
#define TDM_VERSION_MAJOR	2
#define TDM_VERSION_MINOR	12
#define GAME_API_VERSION	TDM_VERSION_MAJOR * 100 + TDM_VERSION_MINOR	// e.g. 210
#define ENGINE_VERSION		"TDM 2.12"	// printed in console

// paths
#define	BASE_GAMEDIR					"base"
#define BASE_TDM					"darkmod"

// filenames
#define CONFIG_FILE					"Darkmod.cfg"
#define KEYBINDS_FILE				"DarkmodKeybinds.cfg"
#define PADBINDS_FILE				"DarkmodPadbinds.cfg"

// base folder where the source code lives
#define SOURCE_CODE_BASE_FOLDER				"darkmod_src"


// default idnet host address
#ifndef IDNET_HOST
	#define IDNET_HOST				"tdmnet.thedarkmod.com"
#endif

// default idnet master port
#ifndef IDNET_MASTER_PORT
	#define IDNET_MASTER_PORT			"27650"
#endif

// default network server port
#ifndef PORT_SERVER
	#define	PORT_SERVER				27666
#endif

// broadcast scan this many ports after PORT_SERVER so a single machine can run multiple servers
#define	NUM_SERVER_PORTS				4

// see ASYNC_PROTOCOL_VERSION
// use a different major for each game
#define ASYNC_PROTOCOL_MAJOR				1

// Savegame Version
// Update when you can no longer maintain compatibility with previous savegames
// NOTE: a seperate core savegame version and game savegame version could be useful
// 16: Doom v1.1
// 17: Doom v1.2 / D3XP. Can still read old v16 with defaults for new data
#define SAVEGAME_VERSION				17

// <= Doom v1.1: 1. no DS_VERSION token ( default )
// Doom v1.2: 2
#define RENDERDEMO_VERSION				2

// editor info
#define EDITOR_DEFAULT_PROJECT				"tdm.qe4"
#define EDITOR_REGISTRY_KEY				"DarkRadiant"
#define EDITOR_WINDOWTEXT				"DarkEdit"

// win32 info
#define WIN32_CONSOLE_CLASS				"TDM WinConsole"
#define	WIN32_WINDOW_CLASS_NAME				"TDM"
#define	WIN32_FAKE_WINDOW_CLASS_NAME			"TDM_WGL_FAKE"

// Linux info
#define LINUX_DEFAULT_PATH				"."	

//#define CONFIG_SPEC					"config.spec"
