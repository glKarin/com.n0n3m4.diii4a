// Copyright (C) 2007 Id Software, Inc.
//

/*
===============================================================================

	Preprocessor settings for compiling different versions.

===============================================================================
*/

#include "BuildDefines.inc"

/*
	Enable this define for a monolithic build. Furthermore change the game
	dll to a static lib:

	dlls -> game -> Properties -> General -> Configuration Type: Static Library (.lib)

	Also remove all project dependencies from the "game" project and add
	dependencies on the "game" and "libGameDecl" projects to the "ETQW" project.

	Note that ID_ALLOW_TOOLS is automatically disabled when using a monolithic build
	because otherwise the tool code will try to load any existing game dll and all
	the static variables from the game dll will map onto the static variables in
	the game lib causing double initialization and other problems.
*/
//#define MONOLITHIC

/*
	Set to 1 for memory logging and memory reports.
	Memory logging inly works with a monolithic build.

	Use the "memoryReport" console command to get a full report with allocations per:

	1. source code folder
	2. source code file
	3. source code line

	All memory allocations from idLib are added to whoever is calling the idLib
	functionality that causes the allocation. For instance all idList memory used
	by the guis is actually listed under the guis folder/files.

*/
#if 0
#define ID_DEBUG_MEMORY
#define ID_REDIRECT_NEWDELETE
#define ID_REDIRECT_BLOCK_ALLOC
#endif

//#define ID_DEBUG_UNINITIALIZED_MEMORY
//#define SD_DEBUG_TRISURFMEMORY
//#define SD_DEBUG_INDEXBUFFERS

//#define SD_USE_INDEX_SIZE_16
//#define SD_USE_DRAWVERT_SIZE_32
//#define SD_USE_HASHINDEX_16

// runs the absolute minimum - its used for megagen for instance
//#define SD_SLIMLINE_ENGINE

#if !defined( SD_SLIMLINE_ENGINE ) && !defined( SD_PUBLIC_TOOLS )
#define SD_SUPPORT_VOIP
#endif // SD_SLIMLINE_BUILD

// enable this define to make the idStr and idWStr allocators thread safe
#ifndef ID_THREAD_SAFE_STR
	#define ID_THREAD_SAFE_STR
#endif

// useful for network debugging, turns off 'LAN' checks, all IPs are classified 'internet'
#ifndef ID_NOLANADDRESS
	#define ID_NOLANADDRESS 0
#endif

// build an exe with no CVAR_CHEAT controls
#ifndef ID_ALLOW_CHEATS
	#define ID_ALLOW_CHEATS 0
#endif

#ifndef ID_ENABLE_CURL
	#if defined( _XENON ) || defined( SD_SLIMLINE_ENGINE )
		#define ID_ENABLE_CURL 0
	#else
		#define ID_ENABLE_CURL 1
	#endif
#endif

// fake a pure client. useful to connect an all-debug client to a server
#ifndef ID_FAKE_PURE
	#define ID_FAKE_PURE 0
#endif

// support gamecode pak checksum remapping
// we will likely enable this for OSX as well
#if defined( __linux__ ) || defined( MACOS_X )
	#define ID_PURE_REMAP 1
#endif

// (initially from Aspyr) - TTimo
// not required on Linux, we only use it for better compatibility when running the Linux binary on BSD < 7.0
// OSX has problems with TLS. Apparently Apple's gcc supports it but the binary format doesn't (mach-o)
#if defined( __linux__ ) || defined( MACOS_X )
	#define USE_PTHREAD_TLS
#endif

// verify checksums in clientinfo traffic
// NOTE: this makes the network protocol incompatible
#ifndef ID_CLIENTINFO_TAGS
	#define ID_CLIENTINFO_TAGS 0
#endif

// put defines in here which should only be used for the ranked server builds
//#define SD_RANKED_SERVER_BUILD
#if defined( SD_RANKED_SERVER_BUILD )
	#define SD_PUBLIC_BUILD
	#define SD_LITE_SERVER
	#define SD_RESTRICTED_FILESYSTEM
#endif

//#define SD_LITE_SERVER
#if defined( SD_LITE_SERVER )
	#define ID_DEDICATED
#endif

#ifndef SD_SUPPORT_UNSMOOTHEDTANGENTS
	#define SD_SUPPORT_UNSMOOTHEDTANGENTS 1
#endif

#ifndef SD_SDNET_STUB
	#if defined( _XENON ) || defined( SD_SLIMLINE_ENGINE )
		#define SD_SDNET_STUB
	#endif
#endif

// adds a com_watchDog cvar, checked from the async sound thread ( 16ms ticks ), triggers a break if main thread stalls for more than cvar msec value
#if !defined( ID_STALL_WATCHDOG ) && defined( _DEBUG )
	#define ID_STALL_WATCHDOG
#endif

// safeguard makes sure com_speeds doesn't count time multiple times
#if !defined( ID_COM_SPEEDS_SAFEGUARD ) && defined( _DEBUG )
	#define ID_COM_SPEEDS_SAFEGUARD
#endif

#if !defined( ID_CONDITIONAL_ASSERT )
	#if !defined( _WIN32 )
		#define ID_CONDITIONAL_ASSERT
	#endif
#endif

// put defines here which should only be used for QA builds
//#define SD_QA_BUILD
#if defined( SD_QA_BUILD )
	#define SD_PUBLIC_BUILD
#endif

// put defines here which should only be used for sd playtest builds
//#define SD_PLAYTEST_BUILD
#if defined( SD_PLAYTEST_BUILD )
	#define SD_ENCRYPTED_FILE_IO
#endif

// put defines in here which should only be used for the private beta builds
//#define SD_PRIVATE_BETA_BUILD
#if defined( SD_PRIVATE_BETA_BUILD )
	#define SD_ENCRYPTED_FILE_IO
	#define SD_PUBLIC_BUILD
	#if !defined( ID_DEDICATED )
		#define SD_STUB_ASYNC_SERVER_BUILD
	#endif
	//#define SD_DISABLE_DEMO_PLAYBACK
	#define SD_SDNET_FORCE_LAN_AUTH
	#define SD_EXPIRE_BUILD
#endif

// put defines in here which should only be used for the public beta builds
//#define SD_PUBLIC_BETA_BUILD
#if defined( SD_PUBLIC_BETA_BUILD )
	#define SD_ENCRYPTED_FILE_IO
	#define SD_RESTRICTED_FILESYSTEM
	#define SD_PUBLIC_BUILD
	#if !defined( ID_DEDICATED )
		#define SD_STUB_ASYNC_SERVER_BUILD
	#endif
	//#define SD_SDNET_FORCE_LAN_AUTH
	#define SD_EXPIRE_BUILD
#endif

// if this is defined, the executable positively won't work with any paks other
// than the demo pak, even if productid is present.
//#define SD_DEMO_BUILD
#if defined( SD_DEMO_BUILD )
	#define SD_RESTRICTED_FILESYSTEM
	#define SD_PUBLIC_BUILD
#endif

#if !defined( SD_DEMO_BUILD ) && !defined( SD_PUBLIC_TOOLS )
	#define SD_SUPPORT_REPEATER
#endif

// put defines in here which should only be changed for a public build
//#define SD_PUBLIC_BUILD
#if defined( SD_PUBLIC_BUILD )
	#define SD_SUPPORT_RESTRICTED_USER_PATHS
	#define SD_MINIMAL_MEMORY_USAGE
#endif

#if !defined( COLLISION_USE_SHORT_EDGES )
	#define COLLISION_USE_SHORT_EDGES
#endif

#if defined( SD_MINIMAL_MEMORY_USAGE )
	#if !defined( SD_USE_INDEX_SIZE_16 )
		#define SD_USE_INDEX_SIZE_16
	#endif
	#if !defined( SD_USE_DRAWVERT_SIZE_32 )
		#define SD_USE_DRAWVERT_SIZE_32
	#endif

	#if !defined( COLLISION_SHORT_INDICES )
		#define COLLISION_SHORT_INDICES
	#endif
#endif

// buffer certain file loads using idFile_Buffered
#define SD_BUFFERED_FILE_LOADS

// always run with auth enabled now (but only when it is not the game's demo)
#if !defined( SD_DEMO_BUILD )
	#define SD_SDNET_REQUIRE_AUTH
#endif

// punkbuster support
#if !defined( _XENON ) && !defined( _WIN64 ) && !defined( SD_PUBLIC_TOOLS ) && !defined( SD_SDK_BUILD )
	#define EB_WITH_PB
	#define __WITH_PB__
#endif // EB_WITH_PB

// Bink support
#if !defined( SD_ENABLE_BINK )
	#if !defined( SD_LITE_SERVER )
		#if defined( _WIN32 ) || defined ( MACOS_X )
			#define SD_ENABLE_BINK
		#endif
	#endif
#endif

// Massive support
#if !defined( MASSIVE )
	#if !defined( SD_PRIVATE_BETA_BUILD ) && !defined( SD_LITE_SERVER ) && !defined( SD_PUBLIC_TOOLS )
		#if defined( _WIN32 ) && !defined( _XENON ) && !defined( _WIN64 )
			#define MASSIVE
		#endif
	#endif /* !SD_PRIVATE_BETA_BUILD */
#endif

#if !defined( SD_ENABLE_BINK )
	#if !defined( SD_LITE_SERVER )
		#if defined( _WIN32 )
			#define SD_ENABLE_BINK
		#endif
	#endif
#endif

#if !defined( SD_PUBLIC_BUILD ) || defined( SD_PRIVATE_BETA_BUILD ) || defined( SD_QA_BUILD )
	#define ID_RELEASE_ASSERT_ON_FATAL_ERROR
#endif

// don't define ID_ALLOW_TOOLS when we don't want tool code in the executable.
#if defined( _WIN32 ) && !defined( SD_MINIMAL_MEMORY_USAGE ) && !defined( ID_DEDICATED ) && !defined( SD_DEMO_BUILD ) && !defined( _XENON ) && !defined( MONOLITHIC )
	#define	ID_ALLOW_TOOLS
#endif

// if enabled, the console won't toggle upon ~, unless you start the binary with +set com_allowConsole 1
// Ctrl+Alt+~ will always toggle the console no matter what
#if !defined( ID_CONSOLE_LOCK )
	#if defined( SD_PUBLIC_BUILD )
		#define ID_CONSOLE_LOCK 1
	#else
		#define ID_CONSOLE_LOCK 0
	#endif
#endif

#if !defined( SD_ALTERNATIVE_CONSOLE_KEY )
	#if !defined( GSS_ENABLED ) && !defined( _DEBUG )
		#define SD_ALTERNATIVE_CONSOLE_KEY 0
	#endif
#endif

// we don't care that the server launcher could load other content
#if defined( SD_SERVER_LAUNCHER )
	#if defined( SD_RESTRICTED_FILESYSTEM )
		#undef SD_RESTRICTED_FILESYSTEM
	#endif
#endif

#if defined( SD_PUBLIC_TOOLS )
#ifdef SD_MINIMAL_MEMORY_USAGE
	#undef SD_MINIMAL_MEMORY_USAGE
#endif
#endif
