
#ifndef __COMMON_H__
#define __COMMON_H__

/*
==============================================================

  Common

==============================================================
*/


// RAVEN BEGIN
// mekberg: added more save types
typedef enum {
	ST_REGULAR,
	ST_QUICK,
	ST_AUTO,
	ST_CHECKPOINT,
} saveType_t;
// RAVEN END

typedef enum {
	EDITOR_NONE					= 0,
	EDITOR_RADIANT				= BIT(1),
	EDITOR_GUI					= BIT(2),
	EDITOR_DEBUGGER				= BIT(3),
	EDITOR_SCRIPT				= BIT(4),
	EDITOR_LIGHT				= BIT(5),
	EDITOR_SOUND				= BIT(6),
	EDITOR_DECL					= BIT(7),
	EDITOR_AF					= BIT(8),
	EDITOR_PDA					= BIT(9),
	EDITOR_FX					= BIT(10),
	EDITOR_REVERB				= BIT(11),
	EDITOR_PLAYBACKS			= BIT(12),
	EDITOR_MODVIEW				= BIT(13),
	EDITOR_LOGVIEW				= BIT(14),
	EDITOR_ENTVIEW				= BIT(15),
	EDITOR_MATERIAL				= BIT(16),

	// Just flags to prevent caching of unneeded assets
	EDITOR_AAS					= BIT(17),
	EDITOR_RENDERBUMP			= BIT(18),
	EDITOR_SPAWN_GUI			= BIT(19),

	// Specifies that a decl validation run is happening
	EDITOR_DECL_VALIDATING		= BIT(20),

	EDITOR_ALL					= -1
};
// RAVEN END

#define MAX_OUTPUT_HISTORY		16

#define STRTABLE_ID				"#str_"
#define STRTABLE_ID_LENGTH		5

extern idCVar		com_version;
extern idCVar		com_skipRenderer;
extern idCVar		com_asyncSound;
extern idCVar		com_machineSpec;
extern idCVar		com_purgeAll;
extern idCVar		com_developer;
extern idCVar		com_allowConsole;
extern idCVar		com_speeds;
extern idCVar		com_showFPS;
extern idCVar		com_showMemoryUsage;
extern idCVar		com_showAsyncStats;
extern idCVar		com_showSoundDecoders;
extern idCVar		com_makingBuild;
extern idCVar		com_skipUltraQuality;
extern idCVar		com_updateLoadSize;
extern idCVar		com_videoRam;

// RAVEN BEGIN
// ksergent: added bundler 
extern idCVar		com_Bundler;

#ifndef _XENON
// nrausch: generate rdf's for xenon load screens
extern idCVar		com_MakeLoadScreens;
#endif

// rjohnson: added quick load
extern idCVar		com_QuickLoad;
// rjohnson: added limits stuff
extern idCVar		com_Limits;
// jsinger: added build for binary lexer
extern idCVar		com_BinaryWrite;
extern idCVar		com_BinaryRead;
// jsinger: added to support serialization/deserialization of binary decls
#ifdef RV_BINARYDECLS
extern idCVar		com_BinaryDeclRead;
#endif
// jsinger: added to support loading of all decls from a single file
#ifdef RV_SINGLE_DECL_FILE
extern idCVar		com_SingleDeclFile;
extern idCVar		com_WriteSingleDeclFIle;
#endif
// jshepard: warning suppresion
extern idCVar		com_uniqueWarnings;
// amccarthy: show Mem_Alloc tag statistics
extern idCVar		com_showMemAllocTags;
#if defined(_XENON)
// mwhitlock: changes for Xenon to enable us to use texture resources from .xpr bundles.
extern idCVar		com_showXenTexCacheStats;
extern idCVar		com_showXenHardwareTimers;
// ksergent: show thread usage.
extern idCVar		com_showXenThreadUsage;
#endif // _XENON
extern idCVar		sys_lang;
// RAVEN END

extern int			time_gameFrame;			// game logic time
extern int			time_gameDraw;			// game present time
extern int			time_frontend;			// renderer frontend time
extern int			time_backend;			// renderer backend time
// RAVEN BEGIN
extern int			time_waiting;			// game logic time
// RAVEN END

extern int			com_frameTime;			// time for the current frame in milliseconds
extern volatile int	com_ticNumber;			// 60 hz tics, incremented by async function

// RAVEN BEGIN
// bdube: added timing dict
extern bool			com_debugHudActive;		// The debug hud is active in the game
// RAVEN END

#ifdef _WINDOWS
const char			DMAP_MSGID[] = "DMAPOutput";
const char			DMAP_DONE[] = "DMAPDone";
#endif

// RAVEN BEGIN
// bdube: forward declarations
class idInterpreter;
class idProgram;

// converted to a class so the idStr gets constructed
class MemInfo {
public:
					MemInfo( void );

	idStr			filebase;

	int				total;
	int				assetTotals;

	// asset totals
	int				imageAssetsTotal;
	int				modelAssetsTotal;
	int				soundAssetsTotal;
// RAVEN BEGIN
	int				collAssetsTotal;
	int				animsAssetsTotal;
	int				aasAssetsTotal;

	int				imageAssetsCount;
	int				modelAssetsCount;
	int				soundAssetsCount;
	int				collAssetsCount;
	int				animsAssetsCount;
	int				aasAssetsCount;
};
// RAVEN END

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

// RAVEN BEGIN
	virtual	int					GetUserCmdHz( void ) const = 0;

	virtual int					GetUserCmdMSec( void ) const = 0;

								// Returns com_frameTime - which is 0 if a command is added to the command line
	virtual int					GetFrameTime( void ) const = 0;

								// returns if the game is processing the last frame when it processes multiple frames
	virtual bool				IsRenderableGameFrame( void ) const = 0;

	virtual void				SetRenderableGameFrame( bool in ) = 0;

								// returns the last message from common->Error
	virtual const char			*GetErrorMessage( void ) const = 0;

								// Initializes a tool with the given dictionary.
	virtual void				InitTool( const int tool, const idDict *dict ) = 0;

								// Returns true if an editor has focus
	virtual bool				IsToolActive( void ) const = 0;

								// Returns an interface to source control
	virtual class rvISourceControl *GetSourceControl( void ) = 0;
// RAVEN END

// RAVEN BEGIN
// dluetscher: added the following method to initialize each of the memory heaps 
#ifdef _RV_MEM_SYS_SUPPORT
	virtual void				InitHeaps( void ) = 0;		// initializes each of the memory heaps for use
	virtual void				ShutdownHeaps( void ) = 0;	// shuts down each of the memory heaps from further use
#endif
// RAVEN END

								// Activates or deactivates a tool.
	virtual void				ActivateTool( bool active ) = 0;

								// Writes the user's configuration to a file
	virtual void				WriteConfigToFile( const char *filename ) = 0;

								// Writes cvars with the given flags to a file.
	virtual void				WriteFlaggedCVarsToFile( const char *filename, int flags, const char *setCmd ) = 0;

// RAVEN BEGIN
// bdube: new exports
								// Modview thinks in the middle of a game frame
	virtual void				ModViewThink ( void ) = 0;	
	
// rjohnson: added option for guis to always think
	virtual void				RunAlwaysThinkGUIs ( int time ) = 0;

								// Debbugger hook to check if a breakpoint has been hit
	virtual void				DebuggerCheckBreakpoint ( idInterpreter* interpreter, idProgram* program, int instructionPointer ) = 0;

// scork: need to test if validating to catch some model errors that would stop the validation and convert to warnings...
	virtual bool				DoingDeclValidation( void ) = 0;
// scork: guess
	virtual void				SetCrashReportAutoSendString( const char *psString ) = 0;

	virtual void				LoadToolsDLL( void ) = 0;
	virtual void				UnloadToolsDLL( void ) = 0;
// RAVEN END

								// Begins redirection of console output to the given buffer.
	virtual void				BeginRedirect( char *buffer, int buffersize, void (*flush)( const char * ), bool rcon = false ) = 0;

								// Stops redirection of console output.
	virtual void				EndRedirect( void ) = 0;

								// Update the screen with every message printed.
	virtual void				SetRefreshOnPrint( bool set ) = 0;

								// Prints message to the console, which may cause a screen update if com_refreshOnPrint is set.
	virtual void				Printf( const char *fmt, ... )id_attribute((format(printf,2,3))) = 0;

								// Same as Printf, with a more usable API - Printf pipes to this.
	virtual void				VPrintf( const char *fmt, va_list arg ) = 0;

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

								// Fatal errors quit all the way to a system dialog box, which is appropriate for
								// static internal errors or cases where the system may be corrupted.
	virtual void				FatalError( const char *fmt, ... ) id_attribute((format(printf,2,3))) = 0;

// RAVEN BEGIN
								// Brings up notepad with the warnings generated while running the game
	virtual void				DumpWarnings( void ) = 0;

								// Returns the localised string of the token, of the token if it does not begin with #str_
	virtual const char *		GetLocalizedString( const char *token, int langIndex = -1 ) = 0;

								// Returns the localised string at position 'index'
	virtual const idLangKeyValue * GetLocalizedString( int index, int langIndex = -1 ) = 0;

								// Returns the number of languages the game found
	virtual int					GetNumLanguages( void ) const = 0;

								// Returns the number of strings in the English langdict
	virtual int					GetNumLocalizedStrings( void ) const = 0;
	
								// Returns the name of the language
	virtual const char *		GetLanguage( int index ) const = 0;

								// Returns whether the language has VO
	virtual bool				LanguageHasVO( int index ) const = 0;

								// Returns key bound to the command
	virtual const char *		KeysFromBinding( const char *bind ) = 0;

								// Returns the binding bound to the key
	virtual const char *		BindingFromKey( const char *key ) = 0; 

								// Directly sample a button.
	virtual int					ButtonState( int key ) = 0;
	
								// Directly sample a keystate.
	virtual int					KeyState( int key ) = 0;

// mekberg: added
	virtual int					GetRModeForMachineSpec( int machineSpec ) const = 0;
	virtual void				SetDesiredMachineSpec( int machineSpec ) = 0;
// RAVEN END

								// returns true if we are currently executing an rcon operation
	virtual bool				IsRCon( void ) const = 0;
};

extern idCommon *				common;

#endif /* !__COMMON_H__ */
