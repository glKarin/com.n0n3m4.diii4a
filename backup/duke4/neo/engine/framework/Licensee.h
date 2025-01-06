/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).  

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

/*
===============================================================================

	Definitions for information that is related to a licensee's game name and location.

===============================================================================
*/

#define GAME_NAME						"Duke Nukem Forever"		// appears on window titles and errors

#define ENGINE_VERSION					"Duke Nukem Forever 0.01"	// printed in console
#define BUILD_NUMBER					1

#define BUILD_TIME						__DATE__ " " __TIME__

// paths
#define	CD_BASEDIR						"Doom"
#ifdef ID_DEMO_BUILD
	#define BASE_GAMEDIR					"demo"
#else
	#define	BASE_GAMEDIR					"duke4base"
#endif

// filenames
#define	CD_EXE							"doom.exe"
#define CONFIG_FILE						"DNFConfig.cfg"

// base folder where the source code lives
#define SOURCE_CODE_BASE_FOLDER			"neo"


// default idnet host address
#ifndef IDNET_HOST
	#define IDNET_HOST					"idnet.ua-corp.com"
#endif

// default idnet master port
#ifndef IDNET_MASTER_PORT
	#define IDNET_MASTER_PORT			"27650"
#endif

// default network server port
#ifndef PORT_SERVER
	#define	PORT_SERVER					27666
#endif

// broadcast scan this many ports after PORT_SERVER so a single machine can run multiple servers
#define	NUM_SERVER_PORTS				4

// see ASYNC_PROTOCOL_VERSION
// use a different major for each game
#define ASYNC_PROTOCOL_MAJOR			1

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
#define EDITOR_DEFAULT_PROJECT			"duketool.qe4"
#define EDITOR_REGISTRY_KEY				"DukeRadiant"
#define EDITOR_WINDOWTEXT				"Duke's Tool"
#define EDITOR_CFG_FILE					"duketool.cfg"

// win32 info
#define WIN32_CONSOLE_CLASS				"Duke Nukem Forever WinConsole"
#define	WIN32_WINDOW_CLASS_NAME			"DNF"
#define	WIN32_FAKE_WINDOW_CLASS_NAME	"DNF_WGL_FAKE"

// Linux info
#ifdef ID_DEMO_BUILD
	#define LINUX_DEFAULT_PATH			"/usr/local/games/doom3-demo"
#else
	#define LINUX_DEFAULT_PATH			"/usr/local/games/doom3"
#endif

// CD Key file info
// goes into BASE_GAMEDIR whatever the fs_game is set to
// two distinct files for easier win32 installer job
#define CDKEY_FILE						"dukekey"
#define XPKEY_FILE						"xpkey"
#define CDKEY_TEXT						"\n// Do not give this file to ANYONE.\n" \
										"// 3drealms will NEVER ask you to send this file to them.\n"

#define CONFIG_SPEC						"config.spec"
