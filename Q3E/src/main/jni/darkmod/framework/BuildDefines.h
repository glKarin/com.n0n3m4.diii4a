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

	Preprocessor settings for compiling different versions.

===============================================================================
*/

// =========================== memory debugging defines =======================
// note: you can temporarily modify them right here to enable/disable something

// Enables debug version of idHeap.
// All allocations via Mem_Alloc and the like are tagged with FILE/LINE data.
// It allows to:
//    1. "memoryDump" or "memoryDumpCompressed" --- dump all memory blocks used to file
//    2. after exit --- writes memory leaks to tdm_main_leak_size.txt
// Note: has additional effect when used with ID_REDIRECT_NEWDELETE (see below)
//#define ID_DEBUG_MEMORY

// Redirects standard C++ new/delete to Mem_Alloc and the like (which used idHeap).
// Defines global operator new/delete, so that normal allocations like "new CGrabber" go through idHeap too.
// Note: if both ID_REDIRECT_NEWDELETE and ID_DEBUG_MEMORY are defined,
// then "new" keyword is redefined with macro (needed to capture FILE/LINE info).
//#define ID_REDIRECT_NEWDELETE

// Enables debug guards against using uninitialized data.
// It does the following:
//    1. fills all idHeap-allocated memory with 0xCD trash
//    2. tries to detect which members of game objects (inherited from idClass) are not initialized properly
//#define ID_DEBUG_UNINITIALIZED_MEMORY

// Allows to use GameTypeInfo.h generated from game headers by TypeInfo parser.
// This makes functions declared in gamesys/TypeInfo.h work properly, and as the result:
//    1. text representation of game state is dumped to file when game is saved (and compared on load)
//    2. messages about uninitialized members show member names
//#define ID_USE_TYPEINFO

// P.S. The following features always work:
//    "com_showMemoryUsage 1" --- display memory usage stats on screen
// ============================================================================

// if enabled, the console won't toggle upon ~, unless you start the binary with +set com_allowConsole 1
// Ctrl+Alt+~ will always toggle the console no matter what
#ifndef ID_CONSOLE_LOCK
	#if defined(_WIN32) || defined(MACOS_X)
		#ifdef _DEBUG
			#define ID_CONSOLE_LOCK 0
		#else
			#define ID_CONSOLE_LOCK 1
		#endif
	#else
		#define ID_CONSOLE_LOCK 1
	#endif
#endif

// useful for network debugging, turns off 'LAN' checks, all IPs are classified 'internet'
#ifndef ID_NOLANADDRESS
	#define ID_NOLANADDRESS 0
#endif

#ifndef ID_ENABLE_CURL
	#define ID_ENABLE_CURL 1
#endif

// verify checksums in clientinfo traffic
// NOTE: this makes the network protocol incompatible
#ifndef ID_CLIENTINFO_TAGS
	#define ID_CLIENTINFO_TAGS 0
#endif

// don't define ID_ALLOW_TOOLS when we don't want tool code in the executable.
#if defined( _WIN32 )
#ifndef NO_MFC
#define	ID_ALLOW_TOOLS
#endif
#endif

#ifndef ID_OPENAL
#	define ID_OPENAL 1
#endif