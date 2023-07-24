// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __COMMON_PUBLIC_H__
#define __COMMON_PUBLIC_H__

// tool tips
typedef struct toolTip_s {
	int id;
	char *tip;
} toolTip_t;

#define	PROC_FILE_ID			"mapProcFile010"
#define PROC_FILE_EXT			"proc"
#define PROCB_FILE_EXT			"procb"
#define ENTITY_FILE_EXT			"entities"
#define BOT_ENTITY_FILE_EXT		"bot_entities"

#define STUFF_FILE_EXT			".clust"
#define STUFFB_FILE_EXT			".clustb"
#define STUFF_FILE_ID			"Version 2"

// shared between the renderer, game, and Maya export DLL
#define MD5_VERSION_STRING		"MD5Version"
#define MD5_MESH_EXT			"md5mesh"
#define MD5_ANIM_EXT			"md5anim"
#define MD5_CAMERA_EXT			"md5camera"

const int MD5_VERSION			= 11;

#define _arraycount( array ) ( sizeof( array ) / sizeof( array[ 0 ] ) )

class idInterpreter;
class idProgram;
class sdDeclLocStr;
class idSoundWorld;

struct vidmode_t {
	const char*		description;
	int				width, height;
	int				aspectRatio;
	bool			available;
};


typedef enum {
	TOOL_NOCACHE_MEDIA,
	TOOL_NOLOAD_IMAGES,
	TOOL_MAX
} eToolFlag_t;

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

								// Called 60 times a second from a background thread for sound mixing,
								// and input generation. Not called until idCommon::Init() has completed.
	virtual void				Async( void ) = 0;

								// Checks for and removes command line "+set var arg" constructs.
								// If match is NULL, all set commands will be executed, otherwise
								// only a set with the exact name.  Only used during startup.
	virtual bool				StartupVariable( const char *match ) = 0;

								// Removes a set commandline arg for the specified variable
	virtual void				ClearStartupVariable( const char* match ) = 0;

								// Writes the user's configuration to a file
	virtual void				WriteConfigToFile( const char *filename, bool writeBindings = true, bool writeCVars = true ) = 0;

								// Writes cvars with the given flags to a file.
	virtual void				WriteFlaggedCVarsToFile( const char *filename, int flags, const char *setCmd ) = 0;

								// Begins redirection of console output to the given buffer.
	virtual void				BeginRedirect( char *buffer, int buffersize, void *user, void (*flush)( void *, const char * ) ) = 0;

								// Stops redirection of console output.
	virtual void				EndRedirect( void ) = 0;

								// Update the screen with every message printed.
	virtual void				SetRefreshOnPrint( bool set ) = 0;

								// Prints message to the console, which may cause a screen update if refreshOnPrint is set.
	virtual void				Printf( const char *fmt, ... ) = 0;

								// Prints message to the console, never causes a screen update.
	virtual void				TPrintf( const char *fmt, ... ) = 0;

								// Same as Printf, with a more usable API - Printf pipes to this.
	virtual void				VPrintf( const char *fmt, va_list arg ) = 0;

								// Prints message that only shows up if the "developer" cvar is set,
								// and NEVER forces a screen update, which could cause reentrancy problems.
	virtual void				DPrintf( const char *fmt, ... ) = 0;

								// Prints WARNING %s message and adds the warning message to a queue for printing later on.
	virtual void				Warning( const char *fmt, ... ) = 0;

								// Prints WARNING %s message and adds the warning message to a queue for printing later on, never causes a screen update.
	virtual void				TWarning( const char *fmt, ... ) = 0;

								// Prints WARNING %s message in yellow that only shows up if the "developer" cvar is set.
	virtual void				DWarning( const char *fmt, ...) = 0;

								// Prints all queued warnings.
	virtual void				PrintWarnings( void ) = 0;

								// Removes all queued warnings.
	virtual void				ClearWarnings( const char *reason ) = 0;

								// Issues a C++ throw. Normal errors just abort to the game loop,
								// which is appropriate for media or dynamic logic errors.
	virtual void				Error( const char *fmt, ... ) = 0;

								// Fatal errors quit all the way to a system dialog box, which is appropriate for
								// static internal errors or cases where the system may be corrupted.
	virtual void				FatalError( const char *fmt, ... ) = 0;

	virtual void				PrintLoadingMessage( const char *msg ) = 0;

	virtual void				EnableWarnings( void ) = 0;
	virtual void				DisableWarnings( void ) = 0;

	virtual void				PacifierUpdate( void ) = 0;
	
	virtual void				UpdateLevelLoadScreen( const wchar_t* status ) = 0;

								// Localization of the current language
	virtual const idLangDict*	GetLanguageDict( void ) = 0;

								// arguments is a list of strings that will be formatted into the result
	virtual idWStr				LocalizeText( const char* declName, const idWStrList& arguments = idWStrList() ) = 0;
	virtual idWStr				LocalizeText( const sdDeclLocStr* loc, const idWStrList& arguments = idWStrList() ) = 0;

	virtual int					GetNumVideoModes( void ) const = 0;
	virtual vidmode_t&			GetVideoMode( int index ) const = 0;

	virtual idSoundWorld*		GetGameSoundWorld( void ) = 0;
	virtual idSoundWorld*		GetMenuSoundWorld( void ) = 0;

	virtual void				WriteConfigs( void ) = 0;
};

extern idCommon *				common;

#endif /* !__COMMON_PUBLIC_H__ */
