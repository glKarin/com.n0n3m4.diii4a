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

#ifndef __COMMON_H__
#define __COMMON_H__

/*
==============================================================

  Common

==============================================================
*/

#define	MAX_PRINT_MSG_SIZE	16 * 1024
#include <atomic>

typedef enum {
	EDITOR_NONE					= 0,
	EDITOR_RADIANT				= BIT(1),
	EDITOR_GUI					= BIT(2),
	//EDITOR_DEBUGGER				= BIT(3),
	EDITOR_SCRIPT				= BIT(4),
	EDITOR_LIGHT				= BIT(5),
	EDITOR_SOUND				= BIT(6),
	EDITOR_DECL					= BIT(7),
	EDITOR_AF					= BIT(8),
	EDITOR_PARTICLE				= BIT(9),
	//EDITOR_PDA					= BIT(10),
	EDITOR_AAS					= BIT(11),
	EDITOR_MATERIAL				= BIT(12),

	EDITOR_RUNPARTICLE			= BIT(13),
} toolFlag_t;

#define STRTABLE_ID				"#str_"
#define STRTABLE_ID_LENGTH		5

extern idCVar		com_version;
extern idCVar		com_skipRenderer;
extern idCVar		com_asyncInput;
extern idCVar		com_asyncSound;
extern idCVar		com_purgeAll;
extern idCVar		com_developer;
extern idCVar		com_allowConsole;
extern idCVar		com_logFile;
extern idCVar		com_speeds;
extern idCVar		com_showFPS;
extern idCVar		com_showFPSavg;
extern idCVar		com_showMemoryUsage;
extern idCVar		com_showAsyncStats;
extern idCVar		com_showSoundDecoders;
extern idCVar		com_makingBuild;
extern idCVar		com_updateLoadSize;
extern idCVar		com_timescale;
//extern idCVar		com_videoRam;

extern int			time_gameFrame;			// game logic time
extern int			time_gameDraw;			// game present time
extern int			time_frontend;			// renderer frontend time
extern int			time_frontendLast;		// renderer frontend time
extern int			time_backend;			// renderer backend time
extern int			time_backendLast;		// renderer backend time

extern int			com_frameTime;			// time moment of the current frame in milliseconds
extern int			com_frameDelta;			// time elapsed since previous frame in milliseconds
extern std::atomic<int>	com_ticNumber;		// 60 hz tics, incremented by async function
extern int			com_editors;			// current active editor(s)
extern bool			com_editorActive;		// true if an editor has focus

#ifdef _WIN32
const char			DMAP_MSGID[] = "DMAPOutput";
const char			DMAP_DONE[] = "DMAPDone";
extern HWND			com_hwndMsg;
extern bool			com_outputMsg;
#endif

struct MemInfo_t {
	idStr			filebase;

	int64			total;
	int64			assetTotals;

	// memory manager totals
	int64			memoryManagerTotal;

	// subsystem totals
	int64			gameSubsystemTotal;
	int64			renderSubsystemTotal;

	// asset totals
	int64			imageAssetsTotal;
	int64			modelAssetsTotal;
	int64			soundAssetsTotal;
};

class I18N;

class idCommon {
public:
	virtual						~idCommon( void ) {}

								// Initialize everything.
								// if the OS allows, pass argc/argv directly (without executable name)
								// otherwise pass the command line in a single string (without executable name)
	virtual void				Init( int argc, const char **argv, const char *cmdline ) = 0;

								// Shuts down everything.
	virtual void				Shutdown( void ) = 0;

								// Shuts down everything.
	virtual void				Quit( void ) = 0;

								// Returns true if common initialization is complete.
	virtual bool				IsInitialized( void ) const = 0;

								// Called repeatedly as the foreground thread for rendering and game logic.
	virtual void				Frame( void ) = 0;

								// Called repeatedly by blocking function calls with GUI interactivity.
	virtual void				GUIFrame( bool execCmd, bool network ) = 0;

								// Called 60 times a second from a background thread for sound mixing,
								// and input generation. Not called until idCommon::Init() has completed.
	virtual void				Async( void ) = 0;

								// Checks for and removes command line "+set var arg" constructs.
								// If match is NULL, all set commands will be executed, otherwise
								// only a set with the exact name.  Only used during startup.
								// set once to clear the cvar from +set for early init code
	virtual void				StartupVariable( const char *match, bool once ) = 0;

								// Initializes a tool with the given dictionary.
	virtual void				InitTool( const toolFlag_t tool, const idDict *dict ) = 0;

								// Activates or deactivates a tool.
	virtual void				ActivateTool( bool active ) = 0;

	enum eConfigExport
	{
		eConfigExport_cvars,
		eConfigExport_keybinds,
		eConfigExport_padbinds,
		eConfigExport_all,
	};

								// Writes the user's configuration to a file
								// greebo: Added the basePath option to allow for more control
								// STiFU #4797: Added the enum to allow exporting cvars and keybinds separately
								// stgatilov: does not do anything if nothing was modified (except for "all" mode)
	virtual void				WriteConfigToFile( const char *filename, eConfigExport configexport, const char *basePath = "fs_savepath") = 0;

								// Begins redirection of console output to the given buffer.
	virtual void				BeginRedirect( char *buffer, int buffersize, void (*flush)( const char * ) ) = 0;

								// Stops redirection of console output.
	virtual void				EndRedirect( void ) = 0;

								// Update the screen with every message printed.
	virtual void				SetRefreshOnPrint( bool set ) = 0;

								// Prints message to the console, which may cause a screen update if com_refreshOnPrint is set.
	virtual void				Printf( const char *fmt, ... )id_attribute((format(printf,2,3))) = 0;

								// Same as Printf, with a more usable API - Printf pipes to this.
	virtual void				VPrintf( const char *fmt, va_list arg ) = 0;

								//Prints current C++ call stack to console
	virtual void				PrintCallStack( void ) = 0;

								// Prints message that only shows up if the "developer" cvar is set,
								// and NEVER forces a screen update, which could cause reentrancy problems.
	virtual void				DPrintf( const char *fmt, ... ) id_attribute((format(printf,2,3))) = 0;

								// Prints WARNING %s message and adds the warning message to a queue for printing later on.
	virtual void				Warning( const char *fmt, ... ) id_attribute((format(printf,2,3))) = 0;

								// Prints WARNING %s message in yellow that only shows up if the "developer" cvar is set.
	virtual void				DWarning( const char *fmt, ...) id_attribute((format(printf,2,3))) = 0;

								// Prints all queued warnings.
	virtual void				PrintWarnings( void ) = 0;

								// Removes all queued warnings.
	virtual void				ClearWarnings( const char *reason ) = 0;

								// Issues a C++ throw. Normal errors just abort to the game loop,
								// which is appropriate for media or dynamic logic errors.
	virtual void				Error( const char *fmt, ... ) id_attribute((format(printf,2,3))) = 0;
	virtual void				DoError( const char *msg, int code ) = 0;

								// Fatal errors quit all the way to a system dialog box, which is appropriate for
								// static internal errors or cases where the system may be corrupted.
	virtual void				FatalError( const char *fmt, ... ) id_attribute((format(printf,2,3))) = 0;
	virtual void				DoFatalError( const char *msg, int code ) = 0;

								// Needed during frontend/backend split, so that errors are properly propagated
								// and don't run into threading issues
	virtual void				SetErrorIndirection( bool enable ) = 0;

								// greebo: Provides access to I18N-related methods
	virtual I18N*				GetI18N() = 0;

						        // take a string like "#str_12345" and return a translated version of it.
						        // Shortcut to GetI18N()->Translate()
	virtual const char *		Translate( const char * str ) = 0;

								// Returns key bound to the command
	virtual const char *		KeysFromBinding( const char *bind ) = 0;

								// Returns the binding bound to the key
	virtual const char *		BindingFromKey( const char *key ) = 0; 

								// Directly sample a button.
	virtual int					ButtonState( int key ) = 0;

								// Directly sample a keystate.
	virtual int					KeyState( int key ) = 0;

	                            // Agent Jones #3766 - Check if we have a game window. If we don't
								// then the main thread is not running.
	virtual bool                WindowAvailable(void) = 0;

								// stgatilov: returns current position in log file (with console messages)
								// returns 0 if log file is not enabled
	virtual int					GetConsoleMarker(void) = 0;
								// stgatilov: returns contents of log file (with console messages)
								// the substring from begin to end is returned
	virtual idStr				GetConsoleContents(int begin, int end) = 0;

};

extern idCommon *		common;

class ErrorReportedException {
private:
	idStr msg;
	int   code;
	bool  fatal;
public:
	ErrorReportedException(const char* msg, int code, bool fatal) : msg(msg), code(code), fatal(fatal) {}
	const idStr& ErrorMessage() const { return msg; }
	int			 ErrorCode() const { return code; }
	bool		 IsFatalError() const { return fatal; }
};

#endif /* !__COMMON_H__ */
